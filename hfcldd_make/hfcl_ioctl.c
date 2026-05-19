/*
 * hfcl_ioctl.c
 * Copyright (C) 2007, 2016, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char ioctl_rcsid[] = "$Id: hfcl_ioctl.c,v 1.45.2.23.2.20.2.2.2.3.6.8.2.2.2.8.2.17.2.3.2.1.2.4.2.5.2.2 2016/02/19 04:27:53 mhayashi Exp $";

/******************************************************************************/
/**                                                                          **/
/**  name    =  hfcl_ioctl.c                                                 **/
/**                                                                          **/
/**  func    =  An ioctl command function                                    **/
/**    IOCTL command to support                                              **/
/**                                 FPP/FIVE                                 **/
/**     <SCSI protocol driver ioctl>                                         **/
/**       1.SCSI INQUIRY            OK /OK                                   **/
/**       2.SCSI COMMAND            OK /OK                                   **/
/**       3.SCSI GID_FT             OK /OK                                   **/
/**       4.SCSI GID_PN             OK /OK                                   **/
/**       5.SCSI PAYLOAD            OK /OK                                   **/
/**       6.SCSI GET_RNID           OK /OK                                   **/
/**       7.SCSI SET_RNID           OK /OK                                   **/
/**       8.SCSI ADAP_STAT          OK /OK                                   **/
/**       9.HBAAPI (?)              OK /OK                                   **/
/**     <diag operation(diagnosis / maintenance connection)>                 **/
/**       1.hfcl_diag()                                                      **/
/**                                                                          **/
/******************************************************************************/

#include "hfcldd.h"
#include "hfcl_detect.h"
#include "hfcl_strategy.h"
#include "hfcl_top.h"
#include "hfcl_ioctl.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_hand_timer_trace.h"

#include "hfcl_tbol.h"                  //FCLNX-0178
#include "hfcddwwn.h"                   //FCLNX-0178
#include "hfcldd_conf.h"

#include "hfcl_mlpf.h"					/* FCLNX-0506 */

#include "hfcl_ioctl_fx.h"
#include "hfcldd_fx.h"

extern uint	raslog_install;				/* FCLNX-467	*/

extern int hfc_diag( void *, struct adap_info * );
extern const struct cr_PartsNumber cr_pn[];				/* FCLNX-0329 */

/*-- global variable --*/
void structdump( int loc, uchar *p, int size );
static int hfc_rtn_scsi_cmnd( struct scsi_cmnd *cmnd, void *arg, uint64_t *buffer, int type );

int hfc_scsi_scan( struct adap_info *ap, void *arg );


/*
 * Function:    structdump
 *
 * Purpose:     A domain dump function
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
void structdump( int loc, uchar *p, int size ) {
	int j, k;
	int n=0;
	uint ar[ 4 ] = { 0, 0, 0, 0 };
	uint ar_wk[ 4 ] = { 0, 0, 0, 0 };
	unsigned int *ip = ( unsigned int * )p;

	HFC_DBGPRT("-- STRUCT DUMP (dumpid=%04x) start -- \n", loc);

	for ( j = 0; j < ( size / 4 ); j ++ ) {
		if ( j % 4 == 0 ) { ar[ 0 ] = *ip; n = 1; }
		if ( j % 4 == 1 ) { ar[ 1 ] = *ip; n = 2; }
		if ( j % 4 == 2 ) { ar[ 2 ] = *ip; n = 3; }
		if ( j % 4 == 3 ) {
			ar[ 3 ] = *ip; n = 0;
			HFC_4L_TO_4B(ar_wk[ 0 ], ar[ 0 ]);
			HFC_4L_TO_4B(ar_wk[ 1 ], ar[ 1 ]);
			HFC_4L_TO_4B(ar_wk[ 2 ], ar[ 2 ]);
			HFC_4L_TO_4B(ar_wk[ 3 ], ar[ 3 ]);
			HFC_DBGPRT(" 0x%05x(%07d) : %08x %08x %08x %08x \n",(j-3)*4,(j-3)*4, ar_wk[ 0 ], ar_wk[ 1 ], ar_wk[ 2 ], ar_wk[ 3 ] );
			ar[ 0 ] = ar[ 1 ] = ar[ 2 ] = ar[ 3 ] = 0;
		}
		ip ++;
	}
	k = j;
	if ( ( j = size - j * 4 ) || ( k % 4 != 0 ) ) {
		for ( p = ( unsigned char * )ip; j > 0; j -- ) {
			ar[ n ] |= *p << ( j * 8 );
			p ++;
		}
		HFC_4L_TO_4B(ar_wk[ 0 ], ar[ 0 ]);
		HFC_4L_TO_4B(ar_wk[ 1 ], ar[ 1 ]);
		HFC_4L_TO_4B(ar_wk[ 2 ], ar[ 2 ]);
		HFC_4L_TO_4B(ar_wk[ 3 ], ar[ 3 ]);
		HFC_DBGPRT("*0x%05x(%07d) : %08x %08x %08x %08x \n",j*4,j*4, ar_wk[ 0 ], ar_wk[ 1 ], ar_wk[ 2 ], ar_wk[ 3 ] );
	}

	HFC_DBGPRT("HFCLDD(IOCTL): -- STRUCT DUMP (dumpid=%04x) end -- \n", loc);
}


/*
 * Function:    memorydump
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
void memorydump( uchar *dump_txt, uchar *p, int size ) {
	int i,j;
	uchar printbuf[128];
	
	uchar buf_wk[128];
	
	HFC_DBGPRT("-- MEMORY DUMP : %s -- \n", dump_txt);
	HFC_DBGPRT("-- FROM ADDRESS : %p -- %x byte dump \n", p, size);
	
	HFC_DBGPRT("OFFSET  |00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |0123456789ABCDEF\n");
	HFC_DBGPRT("--------+------------------------------------------------+-----------------\n");
	
	for (i = 0 ; i < size ; i += 16)
	{
		sprintf(printbuf, "%08x|", i);
		
		for (j = 0 ; j < 16 ; j++)
		{
			if (i+j>=size)
			{
				strcat(printbuf, "   ");
			} else
			{
				sprintf(buf_wk, "%02x ", p[i+j]);
				strcat(printbuf, buf_wk);
			}
		}
		
		strcat(printbuf, "|");
		
		for (j = 0 ; j < 16 ; j++)
		{
			if (i+j>=size)
			{
				strcat(printbuf, " ");
			} else if (p[i+j] < 0x20 || p[i+j] > 0x7f)
			{
				strcat(printbuf, ".");
			} else
			{
				sprintf(buf_wk, "%c", p[i+j]);
				strcat(printbuf, buf_wk);
			}
		}
			HFC_DBGPRT("%s\n", printbuf);
	}
}

/* The fixed number for dump identification */
#define DMP_INQU			0x020
#define DMP_SCSICMD			0x021
#define DMP_SNSBUF			0x022
#define DMP_REQBUF			0x023
#define DMP_IOCMD			0x060
#define DMP_GIDFT			0x080
#define DMP_QWWN			0x100
#define DMP_PAYLD			0x140
#define DMP_PAYLOAD			0x142
#define DMP_RESPONSE		0x143
#define DMP_FRMPALRD		0x144
#define DMP_RSPPALRD		0x145
#define DMP_GRNID			0x160
#define DMP_SRNID			0x161
#define DMP_APSTAT			0x162
#define DMP_HBA_API			0x400
#define DMP_ADAP_ATTR		0x401
#define DMP_PORT_ATTR		0x402
#define DMP_TGTMAP			0x510
#define DMP_PBIND			0x520
#define DMP_SPT				0x530
#define DMP_ADPE			0x531
#define DMP_ADPD			0x532
#define DMP_DKSCAN			0x533
#define DMP_PRINFO			0x540
#define DMP_ADINFO			0x541
#define DMP_SCSIHST			0x542
#define DMP_FC4STAT			0x550   //FCLNX-0404
#define DMP_FLASH_READ		0x600   //FCLNX-0178
#define DMP_FLASH_WRITE		0x601   //FCLNX-0178
#define DMP_WWN_INFO		0x610   //FCLNX-0178
#define DMP_MP_TARGET		0x710


/* Macro for dump functions */
#ifndef _HFC_DEBUG	/* FCLNX-0510	*/
	#define	STRUCTDUMP( LOC, PTR, SIZE ) { while(0) {}; }
#else
	#define STRUCTDUMP( LOC, PTR, SIZE ) { structdump( LOC, PTR, SIZE ); }
#endif			/* FCLNX-0510	*/


#define HFC_STRATEGY( _AP, _SRB )		hfc_strategy_pg( _AP, _SRB )

#define HFC_IOCMD_COPYOUT( caller_64bit, arg, iocmd, iocmd64 )		\
{									\
	if ( caller_64bit ) {						\
		iocmd64.status_validity = iocmd.status_validity;	\
		iocmd64.scsi_bus_status = iocmd.scsi_bus_status;	\
		iocmd64.adapter_status = iocmd.adapter_status;		\
		iocmd64.adap_q_status = iocmd.adap_q_status;		\
		iocmd64.add_device_status = iocmd.add_device_status;	\
		rc = COPYOUT( (char *)&iocmd64, (char *)arg, sizeof( struct scsi_iocmd64 ) );	\
		STRUCTDUMP( DMP_IOCMD64, ( uchar * )&iocmd64, sizeof( struct scsi_iocmd64 ) );	\
	} else {							\
		rc = COPYOUT( ( char * )&iocmd, ( char * )arg, sizeof( struct scsi_iocmd32 ) );	\
		STRUCTDUMP( DMP_IOCMD32, ( uchar * )&iocmd, sizeof( struct scsi_iocmd32 ) );	\
	}								\
}

/*
 * Function:    hfc_open_common
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *  0 : Normal end
 *
 * Notes:       
 */
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
int hfc_open_common(struct inode *inode, struct file *file ) {

	struct	adap_info	*ap;	/* A pointer to an adap_info */
	struct	port_info	*pp;	/* P pointer to an adap_info */
	int		major;
	int		minor;
	int		rtn = 0;
	
//	HFC_ENTRY("hfc_open_common");
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	ap = hfc_manage_info.adap_info_arg[ minor ];
	pp = hfc_manage_info.port_info_arg[ minor ];
	
	if( ap != NULL ) {
		/* FIVE EX open */
		rtn = hfc_open(inode, file );
		return rtn;
	}
	
	if( pp != NULL ){
		/* FIVE FX open */
		rtn = hfc_fx_open(inode, file );
		return rtn;
	}
	
	return ( -EIO );
}
#endif
 
/*
 * Function:    hfc_open
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *  0 : Normal end
 *
 * Notes:       
 */
int hfc_open( struct inode *inode, struct file *file ) {
	int		major;
	int		rtn = 0;			/* A return code of a function */
	
#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	struct 	adap_info	*ap;	/* A pointer to an adap_info */
	struct  mp_adap_info    *mpap;
	int		minor;

	HFC_ENTRY("hfc_open") ;
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	/* Find adap_info from minor number */
	ap = hfc_manage_info.adap_info_arg[ minor ];

	if( ap == NULL ) {
		HFC_DBGPRT(" hfcldd : hfc_open ap=NULL\n");
		/* No corresponding adap_info -> error */
		rtn = -EIO;
		return rtn;
	}
	if ( ( ap -> dev_major != major ) || ( ap -> dev_minor != minor ) ) {
		HFC_DBGPRT(" hfcldd : hfc_open major minor\n");
		/* Major and minor number are unmatched with those stored in adap_info. */
		rtn = -EIO;
		return rtn;
	}

	if ( !test_bit(HFC_ENABLE,(ulong *)&ap->status) )  {
		HFC_DBGPRT(" hfcldd%d : hfc_open not ENABLE\n",ap -> dev_minor);
		rtn = -EIO;
		return rtn;
	}
	/* Acquire lock to handle adap_info */
	if( HFC_SEMAPHORE_LOCK(ap->sem) ) 
		return -ERESTARTSYS ;
		
	mpap = ap->mp_adap_info;
	HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);

	if( (file->f_flags & O_WRONLY) || (file->f_flags & O_RDWR) )
	{
		if( test_bit(HFC_WRITE_PROCESS, (ulong *)&mpap->status) )
		{
			HFC_DBGPRT(" hfcldd%d : hfc_open HFC_WRITE_PROCESS\n",ap -> dev_minor);
			HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
			HFC_SEMAPHORE_UNLOCK(ap->sem) ;
			return -EBUSY ;
		}
		mpap->open_file = file;
		set_bit(HFC_WRITE_PROCESS, (ulong *)&mpap->status) ;
	}
	mpap->open_cnt++;
	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	
	ap -> open_status = HFC_OPENED ;

	/* Increment open count */
	ap -> open_cnt++;
	
	HFC_DBGPRT(" hfcldd%d : hfc_open before UNLOCK\n",ap -> dev_minor);

	HFC_SEMAPHORE_UNLOCK(ap->sem) ;

	HFC_EXIT("hfc_open") ;
	return rtn;
#endif

	rtn = 0;			/* A return code of a function */

	HFC_ENTRY("hfc_open") ;
	major = MAJOR(inode->i_rdev);
	
	if ( hfc_manage_info.major != major ) {
		HFC_DBGPRT(" hfcldd : hfc_open major minor\n");
		/* Major number are unmatched with those stored in manage_info. */
		rtn = -EIO;
		return rtn;
	}
	
	/* Acquire lock to handle manage_info */
	if( HFC_SEMAPHORE_LOCK(hfc_manage_info.sem) ) 
		return -ERESTARTSYS ;
	
	if( (file->f_flags & O_WRONLY) || (file->f_flags & O_RDWR) )
	{
#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
		if( test_bit(HFC_WRITE_PROCESS, (ulong *)&hfc_manage_info.status) )
		{
			HFC_DBGPRT(" hfcldd : hfc_open HFC_WRITE_PROCESS\n");
			HFC_SEMAPHORE_UNLOCK(hfc_manage_info.sem) ;
			return -EBUSY ;
		}
		hfc_manage_info.open_file = file;
		set_bit(HFC_WRITE_PROCESS, (ulong *)&hfc_manage_info.status) ;
#endif
	}
	hfc_manage_info.open_status = HFC_OPENED ;
	
	/* Increment open count */
	hfc_manage_info.open_cnt++;
	
	HFC_DBGPRT(" hfcldd : hfc_open before UNLOCK\n");

	HFC_SEMAPHORE_UNLOCK(hfc_manage_info.sem) ;

	HFC_EXIT("hfc_open") ;
	return rtn;

}	


/*
 * Function:    hfc_close_common
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
int hfc_close_common( struct inode *inode, struct file *file ){
	
	struct	adap_info	*ap;			/* A pointer to an adapter information structure */
	struct	port_info	*pp;			/* P pointer to an adapter information structure */
	int		rtn = 0;
	int		major;
	int		minor;
	
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	ap = hfc_manage_info.adap_info_arg[ minor ];
	pp = hfc_manage_info.port_info_arg[ minor ];
	
	if( ap != NULL ) {
		rtn = hfc_close(inode, file );
		return rtn ;
	}
	
	if( pp != NULL ) {
		rtn = hfc_fx_close(inode, file );
		return rtn ;
	}
	
	return ( -EIO );
}
#endif

/*
 * Function:    hfc_close
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_close( struct inode *inode, struct file *file ) {
	int		major;
	int		rtn = 0;					/* A return code of a function */
	
#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	struct 	adap_info	*ap;			/* A pointer to an adapter information structure */
	struct  mp_adap_info    *mpap;
	int		minor;

	HFC_ENTRY("hfc_close") ;

	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);


	/* Find adap_info from minor number */
	ap = hfc_manage_info.adap_info_arg[ minor ];

	if( ap == NULL ) {
		/* No corresponding adap_info -> error */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x00 );
		rtn = -EIO;
		return rtn;
	}

	if ( ( ap -> dev_major != major ) || ( ap -> dev_minor != minor ) ) {
		/* Major and minor number are unmatched with those stored in adap_info. */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x01 );
		rtn = -EIO;
		return rtn;
	}

	if ( ap -> open_status != HFC_OPENED ) {
		/* Return error if adapter is not opened */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x02 );
		rtn = -EIO;
		return rtn;
	}

	/* Acquire lock to handle adap_info */
	if( HFC_SEMAPHORE_LOCK(ap->sem) ) 
		return -ERESTARTSYS ;

	mpap = ap->mp_adap_info;
	HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
	mpap->open_cnt--;

	if( mpap -> open_file == file )
	{
		clear_bit(HFC_WRITE_PROCESS, (ulong *)&mpap->status) ;
		mpap->open_file = NULL;
	}

	/* Decrement open count */
	ap -> open_cnt--;

	/* Make open_status close if open_cnt is zero */
	if ( ap -> open_cnt == 0 ) {
		ap -> open_status = HFC_CLOSED;
	}

	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	HFC_SEMAPHORE_UNLOCK(ap->sem) ;
	
	HFC_EXIT("hfc_close") ;
	
	return rtn;
#endif

	HFC_ENTRY("hfc_close") ;

	major = MAJOR(inode->i_rdev);
	
	if ( hfc_manage_info.major != major ) {
		/* Major number are unmatched with those stored in manage_info. */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x01 );
		rtn = -EIO;
		return rtn;
	}
	
	if ( hfc_manage_info.open_status != HFC_OPENED ) {
		/* Return error if hfcldd is not opened */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x02 );
		rtn = -EIO;
		return rtn;
	}
	
	/* Acquire lock to handle manage_info */
	if( HFC_SEMAPHORE_LOCK(hfc_manage_info.sem) ) 
		return -ERESTARTSYS ;

#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	if( hfc_manage_info.open_file == file )
	{
		clear_bit(HFC_WRITE_PROCESS, (ulong *)&hfc_manage_info.status) ;
		hfc_manage_info.open_file = NULL;
	}
#endif
	
	/* Decrement open count */
	hfc_manage_info.open_cnt--;
	
	/* Make open_status close if open_cnt is zero */
	if ( hfc_manage_info.open_cnt == 0 ) {
		hfc_manage_info.open_status = HFC_CLOSED;
	}
	
	HFC_SEMAPHORE_UNLOCK(hfc_manage_info.sem) ;
	
	HFC_EXIT("hfc_close") ;
	
	return rtn;
}	

/*
 * Function:    hfc_ioctl
 *
 * Purpose:     An SCSI protocol driver IOCTL routine
 *
 * Arguments:   
 *  devno   : Major/Minor number
 *  cmd     : Required command
 *  arg     : Pointer to a data area
 *  devflag : Caller (User/kernel)
 *  chan    : Unused
 *  ext     : Operation mode
 *            0-normal mode
 *            1-diag mode
 *
 * Returns:     
 *  0 : Normal end
 *
 * Notes:       
 *  IOCTL commands to support
 *	  OK /OK	SCSI INQUIRY	 - 
 *	  OK /OK	SCSI COMMAND	 - 
 *	  OK /OK	SCSI GID_FT		 - 
 *	  OK /OK	SCSI GID_PN		 - 
 *	  OK /OK	SCSI PAYLOAD	 -
 *	  OK /OK	SCSI GET_RNID	 -
 *	  OK /OK	SCSI SET_RNID	 -
 *	  OK /OK	SCSI ADAP_STAT	 -
 *	  OK /OK	SCSI PAYLOAD	 -
 *	  OK /OK	HBAAPI			 -
 */


#ifdef CONFIG_COMPAT
int hfc_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg, int ioctl32 )
#else
int hfc_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
#endif
{
	struct	adap_info		*ap = NULL;		/* Pointer to an adap_info */
	struct	port_info		*pp = NULL;		/* Pointer to a port_info */
	struct	mp_adap_info	*mpap;			/* Pointer to a mp_adap_info */
	int		major;
	int		minor;
	int		rtn = 0;					/* Return code of a function */
	int		rc = 0;

	uint	errlog_no ;
	uint	i, hit=0;
	struct	hfc_linux_ioctl_header *ioctl_header;
	
	HFC_ENTRY("hfc_ioctl") ;

	if( _IOC_TYPE(cmd) != HFC_IOC_MAGIC ) 
	{
		HFC_DBGPRT("hfc_ioctl: Command Check(Cmd=0x%x,Type=0x%x,Magic=0x%x) \n",
					cmd,_IOC_TYPE(cmd),HFC_IOC_MAGIC) ;
		HFC_DBGPRT("HFC_FNC_DIAG0=0x%x \n",(uint)HFC_FNC_DIAG0);
		return( -ENOTTY ) ;
	}
	if( !access_ok(VERIFY_WRITE,(void *)arg,_IOC_SIZE(cmd) ) )
	{
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_DIAG, 0x00 );
		HFC_DBGPRT("_IOC_SIZE=%d \n",_IOC_SIZE(cmd));
		return ( -EFAULT ) ;
	}

	major = MAJOR(inode->i_rdev);
#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	minor = MINOR(inode->i_rdev);
#endif

	ioctl_header = (struct hfc_linux_ioctl_header *)kmalloc(sizeof(struct hfc_linux_ioctl_header), GFP_ATOMIC);
	memset(ioctl_header, 0, sizeof(struct hfc_linux_ioctl_header));

	/* Copy ioctl header data to an internal buffer */
	if ( COPYIN( (char *)arg, (char *)ioctl_header, sizeof(struct hfc_linux_ioctl_header) ) != 0 )
	{
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_DIAG, 0x10 );
		kfree(ioctl_header);
		return EFAULT;
	}

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
	minor = ioctl_header->minor;
#endif
	kfree(ioctl_header);

	/* Search adap_info */
	for (i=0; i<MAX_ADAP_CNT; i++) {
		ap = hfc_manage_info.adap_info_arg[i];
		if ( ap != NULL ) {
			if( ap->dev_minor == minor ) {
				hit = 1;
				break;
			}
		}
	}
	
	/* Search port_info */
	if( hit == 0 ){
		for (i=0; i<MAX_ADAP_CNT; i++) {
			pp = hfc_manage_info.port_info_arg[i];
			if (pp != NULL) {
				if( pp->dev_minor == minor ) {
					hit = 1;
					break;
				}
			}
		}
	}
	
	if (((ap == NULL) && (pp == NULL)) || (hit==0) ) {
		/* No corresponding adap_info -> error */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL, 0x06 );
		rtn = -EINVAL;
		return rtn;
	}
	
	if (pp != NULL) {
		/* FIVE-FX */
#ifdef CONFIG_COMPAT
		return hfc_fx_ioctl(inode, file, cmd, arg, ioctl32);
#else
		return hfc_fx_ioctl(inode, file, cmd, arg);
#endif
	}

	mpap = ap->mp_adap_info;
	if( mpap == NULL ) {
		/* No corresponding mp_adap_info -> error */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL, 0x07 );
		rtn = -EINVAL;
		return rtn;
	}

#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
	if ( ( ap -> dev_major != major ) || ( ap -> dev_minor != minor ) ) {
		/* Major and minor number are unmatched with those stored in adap_info. */
#else
	if ( ap->dev_major != major ) {
#endif
		/* Major are unmatched with those stored in adap_info. */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL, 0x01 );
		rtn = -EINVAL;
		return rtn;
	}

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
	if ( hfc_manage_info.open_status != HFC_OPENED ) {
		/* Error return if open_status is not opened */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL, 0x02 );
		rtn = -EINVAL;
		return rtn;
	}
#endif

	if( ( (cmd != HFC_FNC_DIAG0) && (cmd != HFC_FNC_SC_PRINFO) ) && test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) {	//FCLNX-0149
		/* Error if adapter is in recovery process except and not in diag operation */		//FCLNX-0149
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",				//FCLNX-0149
				HFC_TRC_IOCTL, 0x03 );														//FCLNX-0149
		rtn = -EIO;																			//FCLNX-0149
		return rtn;																			//FCLNX-0149
	}																						//FCLNX-0149

	if(!(test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ) ) {							//FCLNX-0294
		if (!((cmd == HFC_FNC_ADP_ENABLE)
		   || (cmd == HFC_FNC_ADP_DISABLE)
		   || (cmd == HFC_FNC_SC_PRINFO)
		   || (cmd == HFC_FNC_DIAG0)
		   || (cmd == IOCTL_HFC_WWN_INFO))) {
			HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",			//FCLNX-0294
				HFC_TRC_IOCTL, 0x04 );														//FCLNX-0294
			rtn = -EIO;																		//FCLNX-0294
			return rtn;																		//FCLNX-0294
		}
	}

	switch ( cmd ) {
		case HFC_FNC_ADP_ENABLE:
		case HFC_FNC_ADP_DISABLE:
		case HFC_FNC_SCSI_SCAN:	
		case IOCTL_HFC_WWN_INFO: 
		case HFC_FNC_READ_APPARAM:
			HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
			if( test_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status) )
			{
				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
				return -EBUSY ;
			}
			else{
				set_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status);
			}
			HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
			break;
		default:
			break;
	}

	switch ( cmd ) {													/* FCLNX-GPL-466 */
		case HFC_FNC_SC_INQU:		/* Issue SCSI command */
		case HFC_FNC_SC_CMD:		/* Issue SCSI command */
		case HFC_FNC_SC_NMSRV:		/* Issue Mailbox */
		case HFC_FNC_SC_GWWN:		/* Issue Mailbox */
		case HFC_FNC_SC_GSID:		/* Issue Mailbox */
		case HFC_FNC_SC_PAYLD:		/* Issue Mailbox */
		case HFC_FNC_SC_GRNID:		/* Issue Mailbox */
		case HFC_FNC_SC_SRNID:		/* Issue Mailbox */
		case HFC_FNC_SC_APSTAT:		/* Issue Mailbox */

		case HFC_FNC_SC_API:		/* Get Link Info */
		case HFC_FNC_SC_TGTMAP:		/* Get Link Info */
		case HFC_FNC_SC_PBIND:		/* Get Link Info */
		case HFC_FNC_SCSI_SCAN:		/* Issue SCSI command */
			if(ap->initialize != 0) {
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL, 0x06 );
				rtn = -EIO;
				return rtn;
			}
			break;
		default:
			break;
	}																	/* FCLNX-GPL-466 */

	/* Acquire lock */
	if( HFC_SEMAPHORE_LOCK(ap->sem) ) 
		return -ERESTARTSYS ;
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Set ioctl mode( ioctl32 or ioctl64 ) */
	ap->ioctl32 = ioctl32; /* FCLNX-GPL-234 */
#endif

	ap->open_status = HFC_OPENED ;
	ap->open_cnt++;
	
	/* Handle each ioctl command */
	switch ( cmd ) {
	case HFC_FNC_SC_INQU:		/* Inquiry about SCSI device */
		rc = hfc_inquiry( ap, (void *)arg );
		break;

	case HFC_FNC_SC_CMD:		/* Issue SCSI command directly */
		rc = hfc_sciocmd( ap, (void *)arg, FALSE, 0 );			/* FCLNX-0223 */
		break;

	case HFC_FNC_SC_NMSRV:		/* Inquiry about all port IDs connected to a name server */
		rc = hfc_get_all_port_ids( ap, (void *)arg );
		break;

	case HFC_FNC_SC_GWWN:		/* Inquiry about port ID with specific WWN to a name server */
		rc = hfc_get_sid_from_wwpn( ap, (void *)arg );			/* FCLNX-0405 */
		break;

	case HFC_FNC_SC_GSID:		
		rc = hfc_get_wwpn_from_sid( ap, (void *)arg );			/* FCLNX-0405 */
		break;

	case HFC_FNC_SC_PAYLD:
		rc = hfc_payload( ap, (void *)arg );
		break;

	case HFC_FNC_SC_GRNID:
		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;
		rc = hfc_get_rnid( ap, (void *)arg );
		break;

	case HFC_FNC_SC_SRNID:
		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;
		rc = hfc_set_rnid( ap, (void *)arg );
		break;

	case HFC_FNC_SC_APSTAT:
		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;
		if ( ap->fw_init_p->func2 & HFC_FWF_STATCCA ) {		/* FCLNX-GPL-261 */
			/* Confirms new statistical information of F/W in CCA */
			rc = hfc_adap_stat_new( ap, (void *)arg );
		}
		else {
			/* When F/W unsupports new statistical information, Mailbox is used. */
			rc = hfc_adap_stat( ap, (void *)arg );
		}													/* FCLNX-GPL-261 */
		break;
	case HFC_FNC_SC_API:
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_hba_api( ap, (void *)arg );
		break;
	case HFC_FNC_SC_TGTMAP:
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_target_mapping( ap, (void *)arg );
		break;
	case HFC_FNC_SC_PBIND:
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fcp_binding( ap, (void *)arg );
		break;
	case HFC_FNC_SC_SPT:
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_support( ap, (void *)arg );
		break;
	case HFC_FNC_SC_PRINFO:
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_procinfo( ap, (void *)arg );
		break;	
	case HFC_FNC_SC_FC4STAT:												/* FCLNX-0404 */
//		if ( !(ap->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0404 */
		rc = hfc_fc4stat( ap, (void *)arg );								/* FCLNX-0404 */
		break;																/* FCLNX-0404 */

	case HFC_FNC_ADP_ENABLE:
		rc = hfc_adapter_enable( ap, (void *)arg );
		break;
	case HFC_FNC_ADP_DISABLE:
		rc = hfc_adapter_disable( ap, (void *)arg );
		break;
	case HFC_FNC_SCSI_SCAN:													/* FCLNX-0303 */
		rc = hfc_scsi_scan( ap, (void *)arg );								/* FCLNX-0303 */
		break;
																			/* hotplug */

	case HFC_FNC_DIAG0:
		rc = hfc_diag( (void *)arg, ap );
		break;

	case IOCTL_HFC_WWN_INFO:                        //FCLNX-0178
		rc = hfc_wwn_info( ap, (void *)arg );       //FCLNX-0178
		break;                                      //FCLNX-0178

	case HFC_FNC_MP_TGTMAP:
		rc = hfc_mp_target_map( ap, (void *)arg );
		break;	

	case HFC_FNC_READ_APPARAM:                      //FCLNX-0488
		rc = hfc_read_apparam( ap, (void *)arg);    //FCLNX-0488
		break;                                          //FCLNX-0488

	case HFC_FNC_HBA_ISOLATION:								/* FCLNX-GPL-147 */
		rc = hfc_isolate_operation( ap, (void *)arg );		/* FCLNX-GPL-147 */
		break;												/* FCLNX-GPL-147 */

	default:			/* An undefined IOCTL demand */
		if(hfc_manage_info.hfcldd_mp_mod) {
			rc = hfc_manage_info.npubp->hfc_ioctl_mp( ap, cmd, (void *)arg );
			if (rc == ENOTTY) goto CMDSKIP;
			break;
		}
		else {
			if( _IOC_NR(cmd) == 16 ) {
				HFC_DBGPRT(" HFC_FNC_MP_RDPARM -- nop\n");
				rc = 0;
				break;
			}
		}
//CMDSKIP:
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL, 0x05 );
		errlog_no = (uchar)_IOC_NR(cmd) ;	/* FCLNX-0489	*/
		if( ap->raslog_install )
			HFC_INFPRT("hfcldd%d: HFC_ERR9 FC Adapter Driver error (ErrNo:0x68, ioctl_code = %d)\n", ap->dev_minor, errlog_no); 
		else
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x68, (uchar *)&errlog_no, 4) ;
		
		ap->ioctl32 = 0;
		ap->open_cnt--;
		if ( ap->open_cnt == 0 ) {
			ap->open_status = HFC_CLOSED;
		}
		
		HFC_SEMAPHORE_UNLOCK(ap->sem) ;
		return ( -ENOTTY );
	}
	switch ( cmd ) {
		case HFC_FNC_ADP_ENABLE:
		case HFC_FNC_ADP_DISABLE:
		case HFC_FNC_SCSI_SCAN:	
		case IOCTL_HFC_WWN_INFO: 
		case HFC_FNC_READ_APPARAM:
			if( mpap != NULL ){
				if( test_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status) )
				{
					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
					clear_bit(HFC_IOCTL_WR_CHECK, (ulong *)&mpap->status);
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
				}
			}
			break;
		default:
			break;
	}
	
	ap->ioctl32 = 0;
	ap->open_cnt--;
	if ( ap->open_cnt == 0 ) {
		ap->open_status = HFC_CLOSED;
	}
	
	HFC_SEMAPHORE_UNLOCK(ap->sem) ;

	HFC_EXIT("hfc_ioctl") ;
	return -rc;

CMDSKIP:
	HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL, 0x05 );
	errlog_no = (uchar)_IOC_NR(cmd) ;
	if( ap->raslog_install )
		HFC_INFPRT("hfcldd%d: HFC_ERR9 FC Adapter Driver error (ErrNo:0x68, ioctl_code = %d)\n", ap->dev_minor, errlog_no); 
	else
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x68, (uchar *)&errlog_no, 4) ;
		
	ap->ioctl32 = 0;
	ap->open_cnt--;
	if ( ap->open_cnt == 0 ) {
		ap->open_status = HFC_CLOSED;
	}
	HFC_SEMAPHORE_UNLOCK(ap->sem) ;
	
	return ( -ENOTTY );
}	/* end of hfc_ioctl */


//#ifdef HAVE_UNLOCKED_IOCTL
#ifdef CONFIG_COMPAT
long hfc_ioctl32(struct file *file, unsigned int cmd, unsigned long arg)
{
	long      rval;
	const int ioctl32 = 1; /* ioctl32 Mode */ /* FCLNX-GPL-234 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	rval = hfc_ioctl(file->f_dentry->d_inode, file, cmd, arg, ioctl32);
#else
	rval = hfc_ioctl(file->f_path.dentry->d_inode, file, cmd, arg, ioctl32);
#endif /* FCLNX-GPL-FX-496 end */

	return rval;
}

long hfc_ioctl64(struct file *file, unsigned int cmd, unsigned long arg)
{
	long      rval;
	const int ioctl32 = 0; /* ioctl64 Mode */ /* FCLNX-GPL-234 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	rval = hfc_ioctl(file->f_dentry->d_inode, file, cmd, arg, ioctl32);
#else
	rval = hfc_ioctl(file->f_path.dentry->d_inode, file, cmd, arg, ioctl32);
#endif /* FCLNX-GPL-FX-496 end */

	return rval;
}

#endif

/*
 * Function:   hfc_ioctl_common
 *
 * Purpose:     EX ioctl or FX ioctl.
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */

#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
#ifdef CONFIG_COMPAT
long hfc_ioctl_common(struct inode *inode, struct file *file, int cmd, unsigned long arg, int ioctl32)
{
	struct	adap_info	*ap;
	struct	port_info	*pp;
	long	rval;
	int		major;
	int		minor;
	
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	ap = hfc_manage_info.adap_info_arg[ minor ];
	pp = hfc_manage_info.port_info_arg[ minor ];
	
	if( ap != NULL ){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
		rval = hfc_ioctl(file->f_dentry->d_inode, file, cmd, arg, ioctl32);
#else
		rval = hfc_ioctl(file->f_path.dentry->d_inode, file, cmd, arg, ioctl32);
#endif /* FCLNX-GPL-FX-496 end */
		return rval;
	}
	
	if( pp != NULL ){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
		rval = hfc_fx_ioctl(file->f_dentry->d_inode, file, cmd, arg, ioctl32);
#else
		rval = hfc_fx_ioctl(file->f_path.dentry->d_inode, file, cmd, arg, ioctl32);
#endif /* FCLNX-GPL-FX-496 end */
		return rval;
	}
	
	return ( -EIO );
}
#else
int hfc_ioctl_common(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
	struct	adap_info	*ap;
	struct	port_info	*pp;
	long	rval;
	int		major;
	int		minor;
	
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	ap = hfc_manage_info.adap_info_arg[ minor ];
	pp = hfc_manage_info.port_info_arg[ minor ];
	
	if( ap != NULL ){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
		rval = hfc_ioctl(file->f_dentry->d_inode, file, cmd, arg);
#else
		rval = hfc_ioctl(file->f_path.dentry->d_inode, file, cmd, arg);
#endif /* FCLNX-GPL-FX-496 end */
		return rval;
	}
	
	if( pp != NULL ){
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
		rval = hfc_fx_ioctl(file->f_dentry->d_inode, file, cmd, arg);
#else
		rval = hfc_fx_ioctl(file->f_path.dentry->d_inode, file, cmd, arg);
#endif /* FCLNX-GPL-FX-496 end */
		return rval;
	}
	
	return ( -EIO );
}
#endif
#endif

/*
 * Function:    hfc_adap_attr
 *
 * Purpose:     Handle an ioctl start demand from HBAAPI Benda library.
 *
 * Arguments:   
 *  ap     - Pointer to an adap_info 
 *  arg    - Pointer to a data area
 *
 * Returns:     
 *  0      - Normal end 
 *  EFAULT - Failed to copy or attach data
 *  EINVAL - Invalid parameters
 *  ENODEV - No device or target
 *
 * Notes:       
 *  It is called by hfc_ioctl()(HFCHBAAPI)
 */
int hfc_adap_attr( struct adap_info *ap, struct hfc_ioctl_api *api ) {

	struct hfc_ioctl_adap_attr *adap_attr;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	struct mp_adap_info	*mpap=NULL;			/* FCLNX-GPL-0330 */
	struct adap_info	*wk_ap=NULL;		/* FCLNX-GPL-0330 */
	uint				flag = FALSE, i, ports=0;				/* FCLNX-GPL-0330 */

	uint	hw_data[2];
	
	HFC_ENTRY("hfc_adap_attr");

	adap_attr = (struct hfc_ioctl_adap_attr *)hfc_kmalloc(ap, sizeof(struct hfc_ioctl_adap_attr), GFP_ATOMIC);	/* FCLNX_GPL-147 */
																					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	if (adap_attr == NULL) {														/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				HFC_TRC_IOCTL_SC_APATTR, 0x09 );									/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return ENOMEM;																/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	}																				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	memset(adap_attr, 0, sizeof(struct hfc_ioctl_adap_attr));						/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if ( COPYIN( (char *)(ulong)api->addr, (char *)adap_attr, sizeof(struct hfc_ioctl_adap_attr) ) != 0 )
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_APATTR, 0x00 );
		hfc_kfree(ap, adap_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}

	/* Set each attribute value about an adapter */
	strncpy( adap_attr->manufacturer, "HITACHI", HFC_MANUFACTURE_SIZE);
	strncpy( adap_attr->serialnumber, "00000000", HFC_SERIALNUMBER_SIZE);
	adap_attr->node_wwn = ap->node_name ;
	memset( adap_attr->node_symbolic_name, 0, HFC_NODE_SYMBOL_SIZE);
	strncpy( adap_attr->driver_version, hfc_manage_info.package_ver, HFC_DD_VER_SIZE);
	memset( adap_attr->option_rom_version, 0, HFC_ROM_VER_SIZE);
	sprintf( adap_attr->firmware_version, "%08x", hfc_get_sysrev(ap) ); /* FCLNX-GPL-112 */
	adap_attr->vendor_specific_id = 0;

	/* Check core mode. ("One core mode" or "Dual core mode") */ /* FCLNX-GPL-233 start */
	if( ap->pkg.one_core == TRUE )
	{	/* One core mode */
		adap_attr->number_of_ports = ap->mp_adap_info->port_cnt;
	}
	else
	{	/* Dual core mode */
		for(i=0;i<MAX_ADAP_CNT;i++)								/* FCLNX-GPL-0330 */
		{
			wk_ap = hfc_manage_info.adap_info_arg[i];
			if( wk_ap != NULL ){
				mpap = wk_ap->mp_adap_info;
				if( mpap != NULL)
				{
					/* Lock "mpap" */
					HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
					if( mpap == ap->mp_adap_info )
					{
						HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
						continue;
					}
					else{
						if( wk_ap->pci_cfginf != NULL ){
							if( (ap->pci_cfginf->bus->number == wk_ap->pci_cfginf->bus->number)
				  			  &&(PCI_SLOT(ap->pci_cfginf->devfn) == PCI_SLOT(wk_ap->pci_cfginf->devfn)))
							{
								flag = TRUE;
								ports = mpap->port_cnt;
								HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
								break;
							}
						}
					}
					HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
				}
			}
		}
		adap_attr->number_of_ports = ap->mp_adap_info->port_cnt;
		if( flag == TRUE ){
			adap_attr->number_of_ports += ports;
		}														/* FCLNX-GPL-0330 */
//		adap_attr->number_of_ports = ap->mp_adap_info->port_cnt * 2;
	}
	/* FCLNX-GPL-233 end */

	/* Read H/W information (8 bytes from address 0x000 ) */
	hw_data[0] = hfc_read_reg(ap, HFC_IOSPACE_ZERO, 0x4);
	hw_data[1] = hfc_read_reg(ap, HFC_IOSPACE_OFS4, 0x4);

	/* Set H/W version */
	sprintf( adap_attr->hardware_version, "%08x", hw_data[0] );
	sprintf( &adap_attr->hardware_version[8], "%08x", hw_data[1] );

	/* Setup data from an internal data area into hfc_ioctl_adap_attr structure */
	if ( COPYOUT( (char *)adap_attr, (char *)(ulong)api->addr, sizeof(struct hfc_ioctl_adap_attr) ) != 0 )
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_APATTR, 0x01 );
		hfc_kfree(ap, adap_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	
	hfc_kfree(ap, adap_attr);																/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_EXIT("hfc_adap_attr");
	
	return (0);
}


/*
 * Function:    hfc_port_attr
 *
 * Purpose:     Issue ioctl from HBAAPI vendor library
 *
 * Arguments:   
 *  ap 			- Pointer to adap_info 
 *  arg			- Pointer to a data area
 *  devflag		- Caller (User/kernel)
 *
 * Returns:     
 *  	0       - Normal end 
 *  	EFAULT  - Failed to copy or attach data
 *  	EINVAL  - Invalid parameters
 *  	ENODEV  - No device or target
 *
 * Notes:       
 *  			- This function is called by hfc_ioctl()(HFCHBAAPI)
 */

int hfc_port_attr( struct adap_info *ap, struct hfc_ioctl_api *api ) {

	struct hfc_ioctl_port_attr *port_attr;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	struct target_info *target;				/* target_info for this device	    */

	uint	wk_data;
	uint	discport_num = 0 ;
	uint	i;
	ulong	flags = 0 ;

	HFC_ENTRY("hfc_port_attr");

	port_attr = (struct hfc_ioctl_port_attr *)hfc_kmalloc(ap, sizeof(struct hfc_ioctl_port_attr), GFP_ATOMIC);	/* FCLNX_GPL-147 */
																					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	if (port_attr == NULL) {														/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				HFC_TRC_IOCTL_SC_PTATTR, 0x09 );									/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return ENOMEM;																/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	}																				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
																					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	memset(port_attr, 0, sizeof(struct hfc_ioctl_port_attr));						/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if ( COPYIN( (char *)(ulong)api->addr, (char *)port_attr, sizeof(struct hfc_ioctl_port_attr) ) != 0 )
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_PTATTR, 0x00 );
		hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}

	if ( port_attr->flags == HFC_GET_ADAP_PORT ) 
	{		/* GetAdapterPortAttributes */

		/* Set each attribute value about an adapter */
		port_attr->node_wwn = ap->node_name ;
		port_attr->port_wwn = ap->ww_name ;
		port_attr->port_fcid = (uint)(ap->scsi_id & 0x00ffffff);
		port_attr->port_support_class_of_service = HFC_SUPPORT_CLASS ;	/* Class3 */
		memset( port_attr->port_symbol_name, 0, HFC_NODE_SYMBOL_SIZE);
		port_attr->port_max_frame_size = HFC_PORT_MAX_FRAME ;

		/* Set port type */
		if( ap->connect_type == HFC_PT2PT ){			/* PtoP & NotSwitch */
			port_attr->port_type = HFC_PORTTYPE_PTP ;
		}
		else if ( ap->connect_type == HFC_SWITCH ){		/* PtoP & Switch */
			port_attr->port_type = HFC_PORTTYPE_NPORT ;
		}
		else if ( ap->connect_type == HFC_AL ){
			if ( ap -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
				port_attr->port_type = HFC_PORTTYPE_NLPORT ;
			}
			else {										/* AL & NotSwitch */
				port_attr->port_type = HFC_PORTTYPE_LPORT ;
			}
		}
		else {
			port_attr->port_type = HFC_PORTTYPE_UNKNOWN ;
		}

		/* Set state type */
		if( test_bit(HFC_HWCHKSTOP , (ulong *)&ap->mp_adap_info->status) )
			port_attr->port_state = HFC_PORTSTATE_ERROR ;
		else if( !test_bit(HFC_ONLINE , (ulong *)&ap->status) )
			port_attr->port_state = HFC_PORTSTATE_LINKDOWN ;
		else if( test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ) )	//FCLNX-0160
			port_attr->port_state = HFC_PORTSTATE_LINKDOWN ;			//FCLNX-0160
		else
			port_attr->port_state = HFC_PORTSTATE_ONLINE ;

#define HFC_SUPPORT_FC4 0x00000100
		/* Port supports FC4 types */
		memset(port_attr->port_support_fc4_types, 0, 32);
		wk_data = HFC_SUPPORT_FC4 ;
		memcpy(port_attr->port_support_fc4_types, (uchar*)&wk_data, 4);

#define HFC_ACTIVE_FC4 0x00000100
		/*  Port supports Active FC4 types */
		memset(port_attr->port_active_fc4_types, 0, 32);
		wk_data = HFC_ACTIVE_FC4 ;
		memcpy(port_attr->port_active_fc4_types, (uchar*)&wk_data, 4);

		/* Supported port speed */
		if( ap->pkg.type == HFC_PKTYPE_FPP )
			port_attr->port_supported_speed = HFC_PORTSPEED_2GBIT ;
		else if( ap->pkg.type == HFC_PKTYPE_FIVE )
			port_attr->port_supported_speed = HFC_PORTSPEED_4GBIT ;
		else /* FIVE-EX */
			port_attr->port_supported_speed = HFC_PORTSPEED_8GBIT ;

		/* Port speed */			
		if( ap->max_data_rate == HFC_100MBS )
			port_attr->port_speed = HFC_PORTSPEED_1GBIT ;
		else if( ap->max_data_rate == HFC_200MBS )
			port_attr->port_speed = HFC_PORTSPEED_2GBIT ;
		else if( ap->max_data_rate == HFC_400MBS )
			port_attr->port_speed = HFC_PORTSPEED_4GBIT ;
		else if( ap->max_data_rate == HFC_800MBS )
			port_attr->port_speed = HFC_PORTSPEED_8GBIT ;
		else if( ap->max_data_rate == HFC_1000MBS )
			port_attr->port_speed = HFC_PORTSPEED_10GBIT ;
		else
			port_attr->port_speed = HFC_PORTSPEED_UNKNOWN ;

		/* Search target_info and report number of DiscPort and ww_name */
		for (i = 0; i < MAX_TARGET_PROBE; i++) 
		{
			target = hfc_hash_target_info(ap, i); /* check HFC_TARGETINF_VALID & HFC_DEVFLG_VALID & HFC_WWN_VALID */

			if (target != NULL) 												//FCLNX-0151
			{																	//FCLNX-0151
				if( (test_bit(HFC_ONLINE,     (ulong *)&ap->status))			//FCLNX-0151
				  && (!test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ))			//FCLNX-0151
				  && (!test_bit(HFC_SCN_WLINKUP,(ulong *)&target->status)) )	//FCLNX-0151
				{																//FCLNX-0151
					/* Report ww_name information of a target structure */
					port_attr->disc_wwn[discport_num] = target->ww_name ;
					/* Increment a number of ww_name */
					discport_num++;
				}																//FCLNX-0151
			}
		}
		port_attr->num_of_disc_port = discport_num ;
	}
	else if ( port_attr->flags == HFC_GET_DISC_PORT )		/* GetDiscoveredPortAttributes */
	{
		HFC_ADAPLOCK_IRQSAVE(flags);
		/* Is designated parameter SCSI_ID or ww_name ? */
		/*   SCSI_ID -> Identify target_info from target_arg[] by SCSI_ID */
		/*   ww_name -> Identify target_info from target_arg[] by WW_NAME */
		if( api->valid & HFC_API_SCSI_ID_VALID ){
			if ( ( target = hfc_hash_target_info( ap, api->scsi_id ) ) == NULL ) {
				/* Return target_info. Return error if target_info is not found */
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_SC_PTATTR, 0x01 );
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
				hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				return ENODEV ;
			}
		}
		else if( api->valid & HFC_API_WWN_VALID ){
			if ( ( target = hfc_hash_target_info_wwn( ap, api->world_wide_name ) ) == NULL ) {
				/* Return target_info. Return error if target_info is not found */
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_SC_PTATTR, 0x02 );
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
				hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				return ENODEV ;
			}
		}
		else {
			HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_SC_PTATTR, 0x03 );
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
			return EINVAL;
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);

		/* Report each attribute value about a counterpart port */
		port_attr->node_wwn = target->node_name ;
		port_attr->port_wwn = target->ww_name ;
		port_attr->port_fcid = (uint)(target->scsi_id & 0x00ffffff);
		port_attr->port_support_class_of_service = (uint)(target -> fc_class_mask << 1) ;
		memset( port_attr->port_symbol_name, 0, HFC_NODE_SYMBOL_SIZE);
		port_attr->port_supported_speed = 0 ;
		port_attr->port_speed = 0 ;
		port_attr->port_max_frame_size = (uint)target->max_frame_size ;

		/* Set Port Type report */
		if( ap->connect_type == HFC_PT2PT ){			/* PtoP & NotSwitch */
			port_attr->port_type = HFC_PORTTYPE_PTP ;
		}
		else if ( ap->connect_type == HFC_SWITCH ){		/* PtoP & Switch */
			port_attr->port_type = HFC_PORTTYPE_NPORT ;
		}
		else if ( ap->connect_type == HFC_AL ){
			if ( ap -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
				port_attr->port_type = HFC_PORTTYPE_NLPORT ;
			}
			else {										/* AL & NotSwitch */
				port_attr->port_type = HFC_PORTTYPE_LPORT ;
			}
		}
		else {
			port_attr->port_type = HFC_PORTTYPE_UNKNOWN ;
		}

		/* Set Port State */
		port_attr->port_state = HFC_PORTSTATE_UNKNOWN ;

		/* Port supports FC4 types */
		memset(port_attr->port_support_fc4_types, 0, 32);

		/* Port supports Active FC4 types */
		memset(port_attr->port_active_fc4_types, 0, 32);

	}
	else
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_PTATTR, 0x04 );
		hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EINVAL;
	}

	/* Setup data from an internal data area into hfc_ioctl_adap_attr structure */
	if ( COPYOUT( (char *)port_attr, (char *)(ulong)api->addr, sizeof(struct hfc_ioctl_port_attr) ) != 0 ){
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_PTATTR, 0x05 );
		hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}

	hfc_kfree(ap, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_EXIT("hfc_port_attr") ;
	
	return (0);
}



/*
 * Function:    hfc_hba_api
 *
 * Purpose:     Issue ioctl from HBAAPI vendor library
 *
 * Arguments:   
 *  ap 			- Pointer to adap_info 
 *  arg			- Pointer to a data area
 *  devflag		- Caller (User/kernel)
 *
 * Returns:     
 *  	0       - Normal end 
 *  	EFAULT  - Failed to copy or attach data
 *  	EINVAL  - Invalid parameters
 *  	ENODEV  - No device or target
 *
 * Notes:       
 *  			- This function is called by hfc_ioctl()(HFCHBAAPI)
 */
int hfc_hba_api( struct adap_info *ap, void *arg ) {

	struct hfc_ioctl_api api;

	int		rc = 0;
	int		rtn = 0;

	HFC_ENTRY("hfc_hba_api");		

	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if ( COPYIN( (char *)arg, (char *)&api, sizeof(struct hfc_ioctl_api) ) != 0 ){
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_API, 0x00 );
		return EFAULT;
	}

#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(ap->ioctl32) {
		api.addr = api.addr & 0xffffffffU;
	}
#endif

	/* Handle each command */
	switch ( api.sub_cmd ) 
	{
		case HFC_GET_ADAPTER_ATTR:		/* GetAdapterAttributes */
			rtn = hfc_adap_attr( ap, (void *)&api ) ;
			break;

		case HFC_GET_PORT_ATTR:
			rtn = hfc_port_attr( ap, (void *)&api ) ;
			break;

		default:
			HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_SC_API, 0x01 );
			return EINVAL;
	}

	/* Return error when hfc_adap_attr or hfc_port_attr returns error */
	if ( rtn != 0 )
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_API, 0x02 );
		return rtn;
	}

	/* Setup data from an internal data area into hfc_ioctl_adap_attr structure */
	if ( COPYOUT( (char *)&api, (char *)arg, sizeof(struct hfc_ioctl_api) ) != 0 ){
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_API, 0x03 );
		rc = EFAULT;
	}

	HFC_EXIT("hfc_hba_api");
	/* Set return code */
	if ( rtn == 0 ) rtn = rc;
	return rtn;
}


/*
 * Function:    hfc_ioctl_iodone
 *
 * Purpose:     Completes SCSI command issued by IOCTL
 *
 * Arguments:   cmnd -  A pointer to a struct scsi_cmnd 
 *
 * Returns:     none
 *
 * Notes:       
 */
void hfc_ioctl_iodone( struct scsi_cmnd *cmnd ) {
	struct adap_info *ap;

	ap = (struct adap_info *)CMND_HOSTDATA(cmnd);
	
	set_bit(HFC_IOCTL_BUSY, (ulong *)&ap->ioctl_lock);
	hfc_wake_up(&ap->ioctl_event, &ap->ioctl_event_wait);				/* FCLNX-0296 */
	return;
} 

/*
 * Function:    hfc_ioctl_sleep
 *
 * Purpose:     Wait to complete IOCTL command
 *
 * Arguments:   cmnd - A pointer to a struct scsi_cmnd structure
 *
 * Returns:     none
 *
 * Notes:       
 */
void hfc_ioctl_sleep( struct scsi_cmnd *cmnd ) {
	struct adap_info *ap;

	ap = (struct adap_info *)CMND_HOSTDATA(cmnd);
	hfc_sleep_on(&ap->ioctl_event, &ap->ioctl_event_wait);				/* FCLNX-0296 */
	clear_bit(HFC_IOCTL_BUSY, (ulong *)&ap->ioctl_lock);
	return;
} 


/*
 * Function:    hfc_rtn_scsi_cmnd
 *
 * Purpose:     Read data from struct scsi_cmnd 
 *
 * Arguments:   
 *  cmnd        - A pointer to a scsi_cmnd 
 *  arg         - A pointer to data area
 *                (Scsi_status of hfc_ioctl_inquiry or hfc_ioctl_cdb)
 *  data_length - Data length of inquiry data (return data)
 *
 * Returns:     
 *   0         		  - Normal end
 *   Other than zero  - Error end
 *
 * Notes:       
 */
int hfc_rtn_scsi_cmnd( struct scsi_cmnd *cmnd, void *arg, uint64_t *buffer, int type ) {
	int   rtn = 0;                    /* A return code of this function */

	struct wrk_hfc_ioctl {
		uchar    scsi_status;
		uchar    resid_flags;
		ushort   resid;
		uchar    rsv2[3];
		uchar    adapter_status;
		uint64_t sense_ptr;
		uchar    rsv3[2];
		ushort   sense_len;
	};
	struct     wrk_hfc_ioctl   *wrk;
	ushort     sense_length   = 0;    /* Length of an automatic sense domain */
	ushort     data_length    = 0;    /* Data length of inquiry data */
	uchar      scsi_status    = 0x00; /* scsi_status */
	uchar      did_err        = 0x00; /* adap_status */
	
	uint		data_buf_num = 0, i;
	struct scatterlist		*sgl=NULL;

	wrk         = arg;

	scsi_status = (uchar) (cmnd -> result & 0x000000ff);
	did_err     = (uchar) ((cmnd -> result & 0x00ff0000) >> 16);    /* FCLNX-0633 */

	wrk -> scsi_status    = scsi_status;
	wrk -> resid_flags    = 0x00;
	wrk -> resid          = 0;
	wrk -> adapter_status = did_err;

	if( cmnd->result ) {
		/* Error exists (Scsi_status is not zero, and/or did_ok is not DID_OK */
		switch ( did_err ) {
			case DID_BAD_TARGET:
			case DID_NO_CONNECT:
				rtn = ENODEV;
				break;
			case DID_BUS_BUSY:
				rtn = EBUSY;
				break;
			case DID_TIME_OUT:
				rtn = ETIMEDOUT;
				break;
			default:
				rtn = EIO;
		}
	
		/* Copy auto sense data if auto sense data is returned */
		if( ( wrk->sense_len>0 ) && ( wrk->sense_ptr!=0 ) && ( (scsi_status==HFC_SCSISTAT_CHECK_CONDITION) || (scsi_status==HFC_SCSISTAT_COMMAND_TERMINATED) ) ) {
			sense_length = ( wrk->sense_len>SCSI_SENSE_BUFFERSIZE ) ? SCSI_SENSE_BUFFERSIZE : wrk->sense_len;
			
			if (type == TRUE) {
				if( COPYOUT( (char *)(ulong)cmnd->sense_buffer, (char *)(ulong)wrk->sense_ptr, sense_length ) != 0 ) {	/* FCLNX-GPL-478 */
					return EFAULT;
				} 
			}
			else {
				memcpy((char *)(ulong)wrk->sense_ptr, (char *)(ulong)cmnd->sense_buffer, sense_length);	/* FCLNX-GPL-478 */
			}
			
			STRUCTDUMP( DMP_SNSBUF, (char *)(ulong)wrk->sense_ptr, sense_length );
		}
	}
	else {
		/* No error */
		/* kernel 5.x+: sdb.resid → resid_len */
		if( cmnd->resid_len == cmnd->sdb.length ) {
			/* There are no inquiry data */
			rtn = EIO;
		}
		else {
			/* Copy inquiry data */
			/* kernel 5.x+: sdb.resid → resid_len */
			if( cmnd->resid_len > 0 ) {
				wrk -> resid_flags = HFC_RESID_UNDERFLOW;
				wrk -> resid	   = (ushort)cmnd->resid_len;
			}
			/* kernel 5.x+: sdb.resid → resid_len */
			data_length = cmnd->sdb.length - cmnd->resid_len;
			
			if (type == TRUE) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
				data_buf_num = data_length/HFC_PAGE_SIZE;
				if( data_length % HFC_PAGE_SIZE ){
					data_buf_num++;
				}
				sgl = cmnd->sdb.table.sgl;
				if( data_buf_num ){
					for(i=0;i<data_buf_num;i++){
						sgl->dma_length = HFC_PAGE_SIZE;
						if( i == (data_buf_num-1) ){
							if( data_length % HFC_PAGE_SIZE ){
								sgl->dma_length = (data_length % HFC_PAGE_SIZE);
							}
						}
						sgl->page_link &= ~0x02;
						if( COPYOUT( (char *)sgl->page_link, ( (uchar *)(ulong)buffer+(HFC_PAGE_SIZE*i) ), sgl->dma_length ) != 0 ) {
							return EFAULT;
						}
						sgl++;
					}
				}
#else
				if( COPYOUT( (char *)cmnd->request_buffer, (char *)buffer, data_length ) != 0 ) {
					return EFAULT;
				}
#endif
			}
			else {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)		/* FCLNX-GPL-0343 */
				data_buf_num = data_length/HFC_PAGE_SIZE;
				if( data_length % HFC_PAGE_SIZE ){
					data_buf_num++;
				}
				sgl = cmnd->sdb.table.sgl;
				if( data_buf_num ){
					for(i=0;i<data_buf_num;i++){
						sgl->dma_length = HFC_PAGE_SIZE;
						if( i == (data_buf_num-1) ){
							if( data_length % HFC_PAGE_SIZE ){
								sgl->dma_length = (data_length % HFC_PAGE_SIZE);
							}
						}
						sgl->page_link &= ~0x02;
						memcpy( ( (uchar *)(ulong)buffer+(HFC_PAGE_SIZE*i) ), (char *)sgl->page_link, sgl->dma_length );
						sgl++;
					}
				}
#else
				memcpy((char *)buffer, (char *)cmnd->request_buffer, data_length);
#endif
			}

			STRUCTDUMP( DMP_REQBUF, (char *)buffer , data_length );
		}
	}
	

	return rtn;
}


/*
 * Function:    hfc_inquiry
 *
 * Purpose:     Issue SCSI inquiry command
 *
 * Arguments:   
 *  cmnd        - A pointer to a scsi_cmnd 
 *  arg         - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  EINVAL    - Target device is not started, or data area is not ready
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred.
 *
 * Notes:       It is called by hfc_ioctl()
 */
int hfc_inquiry( struct adap_info *ap, void *arg ) {
	int    RC  = 0;				/* return code  */

	struct hfc_ioctl_inquiry inquiry;		/* hfc_ioctl_inquiry structure */
	struct scsi_cmnd          *cmnd;		/* A pointer to a struct scsi_cmnd structure */
	struct target_info *target;             /* A pointer to a target_info structure */
	ulong              flags = 0;           /* for spin_lock/unlock */
	static uint        cnt   = 0;           /* for serial_number    */
#define MAX_CNT            0xffffffff
	ushort             data_length = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	int					data_buf_num = 0, i;
	struct scatterlist	*sgl=NULL, *pre_sgl=NULL;
	struct dev_info		*dev=NULL;
#else
	int                cmnd_device_alloc_flag =0;	//FCLNX-0147
#endif												/* FCLNX-GPL-0343 */

	HFC_ENTRY("hfc_inquiry") ;
	
	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if( COPYIN( (char *)arg, (char *)&inquiry, sizeof(struct hfc_ioctl_inquiry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x00 );
		return EFAULT;
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(ap->ioctl32) {
		inquiry.buffer    = inquiry.buffer    & 0xffffffffU;
		inquiry.sense_ptr = inquiry.sense_ptr & 0xffffffffU;
	}
#endif
	
	STRUCTDUMP( DMP_INQU, (uchar *)&inquiry, sizeof(struct hfc_ioctl_inquiry) );

	HFC_ADAPLOCK_IRQSAVE(flags);
	if ( (( target = hfc_hash_target_info_wwn( ap, inquiry.Port_WWN ) ) == NULL )
	  || (inquiry.lun_id >= MAX_DEV_CNT) ) {
		/* Return target_info if found. Error return if target_info is not found */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x01 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return ENODEV ;
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	/* Make sure whether an inquiry data area is set */
	if( (inquiry.data_length==0) || (inquiry.buffer==0) ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x02 );
		return EINVAL;
	}

	cmnd = ap->ioctl_cmnd;
	if( cmnd == NULL ){
		/* Return error if failed to allocate */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x03 );
		return ENOMEM;
	}
	
	if( cmnd->cmnd == NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x07 );
		return ENOMEM;
	}
	
	if( cmnd->device == NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x08 );
		return ENOMEM;
	}

#if 0		/* FCLNX-GPL-0343 */
	/* Allocate scsi_cmnd structure */
	if( ( cmnd = (struct scsi_cmnd *)hfc_kmalloc(ap, sizeof(struct scsi_cmnd), GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x03 );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd, 0, sizeof(struct scsi_cmnd) );

	/* Setting of a struct scsi_cmnd structure */
	cmnd_device_alloc_flag = 0;
	if ( (cmnd->device = (struct scsi_device*)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate scsi_cmnd */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x07 );
		hfc_kfree(ap, cmnd );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd->device, 0, sizeof(struct scsi_device) );
	cmnd_device_alloc_flag = 1;
#endif		/* FCLNX-GPL-0343 */

	cmnd->device->host = ap->hosts;

	/* kernel 5.15+: scsi_cmnd->serial_number removed; field unused */
	(void)cnt; cnt=(cnt+1)%MAX_CNT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)			/* FCLNX-GPL-0343 */
	dev = (struct dev_info *)hfc_get_dev_info(target, inquiry.lun_id);

	if( dev == NULL ){
		dev = ap->ioctl_dev;
		memset( dev, 0, sizeof(struct dev_info) );
		dev->lun = (uint)inquiry.lun_id;
		dev->target_id = target ->  target_id;
		set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
	}
	cmnd -> device -> hostdata = (struct dev_info *)dev;

	cmnd -> device -> request_queue -> rq_timeout = ap->ioctl_scsi_timeout * HZ;	/* FCLNX-GPL-0368 */
	cmnd -> sdb.length = inquiry.data_length;
	cmnd -> transfersize = inquiry.data_length;
	data_buf_num = inquiry.data_length / HFC_PAGE_SIZE;
	if( (inquiry.data_length % HFC_PAGE_SIZE) ){
		data_buf_num++;
	}
	cmnd -> sdb.table.nents = data_buf_num;
	memset( cmnd->cmnd, 0, 16 );
	sgl = cmnd -> sdb.table.sgl;
	if ( cmnd -> sdb.table.nents > 0 )
	{
		for ( i = 0; i < cmnd -> sdb.table.nents; i++ )
		{
			sgl -> dma_length = HFC_PAGE_SIZE ;
			sgl->page_link &= ~0x02;
			memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
			pre_sgl = sgl;
			sgl++;
		}
		if ( inquiry.data_length % HFC_PAGE_SIZE != 0 )
		{
			pre_sgl->dma_length = inquiry.data_length % HFC_PAGE_SIZE ;
		}
		pre_sgl->page_link |= 0x02;
	}
	else{
		sgl->dma_length = 0 ;
	}
#else
	cmnd -> timeout_per_command = 30 * HZ;
	cmnd -> transfersize        = inquiry.data_length;	//FCLNX-0184
	cmnd -> request_bufflen     = inquiry.data_length;	//FCLNX-0184
#endif														/* FCLNX-GPL-0343 */

	CMND_TARGET(cmnd)           = target ->  target_id;
	CMND_LUN(cmnd)              = (uint)inquiry.lun_id;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)			/* FCLNX-GPL-0343 */
	if( ( cmnd->request_buffer  = hfc_kmalloc(ap, cmnd -> request_bufflen, GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate buffer area  */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x04 );
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_kfree(ap, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd );
		return ENOMEM;
	}
	memset( cmnd->request_buffer, 0, cmnd -> request_bufflen );
#endif													/* FCLNX-GPL-0343 */

	cmnd -> sc_data_direction   = SCSI_DATA_READ;
	/*                   INQUIRY Command
	 *+=====-=======-=======-=======-=======-=======-=======-=======-=======+
	 *|  Bit|   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
	 *|Byte |       |       |       |       |       |       |       |       |
	 *|=====+===============================================================|
	 *| 0   |                           Operation Code (12h)                |
	 *|-----+---------------------------------------------------------------|
	 *| 1   | Logical Unit Number   |                  Reserved     | EVPD  |
	 *|-----+---------------------------------------------------------------|
	 *| 2   |                           Page Code                           |
	 *|-----+---------------------------------------------------------------|
	 *| 3   |                           Reserved                            |
	 *|-----+---------------------------------------------------------------|
	 *| 4   |                           Allocation Length                   |
	 *|-----+---------------------------------------------------------------|
	 *| 5   |                           Control                             |
	 *+=====================================================================+*/
	/* Setting of an SCSI command */
	cmnd -> cmd_len             = 6;
	cmnd -> cmnd[0]             = INQUIRY;
	if( inquiry.extended ) {
		cmnd -> cmnd[1]           = 0x01;
		cmnd -> cmnd[2]           = inquiry.page_code;
	}
	else {
		cmnd -> cmnd[1]           = 0;
		cmnd -> cmnd[2]           = 0;
	}
	cmnd -> cmnd[3]             = 0;
	data_length = ( inquiry.data_length>255 ) ? 255 : inquiry.data_length;	//FCLNX-0184
	cmnd -> cmnd[4]             = (uchar)data_length;
	cmnd -> cmnd[5]             = inquiry.flags;


	RC = HFC_STRATEGY( cmnd, (void *) hfc_ioctl_iodone );
	/* Need to sleep here. (iodone is executed even if error occurred in hfc_strategy)  */
	hfc_ioctl_sleep( ( struct scsi_cmnd * )cmnd );

	if( RC ) {
		/* When an error occurred in strategy() */
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_kfree(ap, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd->request_buffer );
		hfc_kfree(ap, cmnd );
#endif			/* FCLNX-GPL-0343 */
		return EIO;
	}
	STRUCTDUMP( DMP_SCSICMD, (uchar *)cmnd    , sizeof(struct scsi_cmnd) );

	RC = hfc_rtn_scsi_cmnd( cmnd, &inquiry.scsi_status, (uint64_t *)(ulong)inquiry.buffer, TRUE );
	if( RC==EFAULT ) {
		/* Write back failed for sense_buffer or request_buffer */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x05 );
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_kfree(ap, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd->request_buffer );
		hfc_kfree(ap, cmnd );
#endif			/* FCLNX-GPL-0343 */
		return EFAULT;
	}

	/* Write back data into hfc_ioctl_inquiry structure */
	if( COPYOUT( (char *)&inquiry, (char *)arg, sizeof(struct hfc_ioctl_inquiry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x06 );
		RC = EFAULT;
	}
	STRUCTDUMP( DMP_INQU, (uchar *)&inquiry, sizeof(struct hfc_ioctl_inquiry) );

#if 0			/* FCLNX-GPL-0343 */
	if ( cmnd_device_alloc_flag == 1 ) {		//FCLNX-0147 FCLNX-0423
		hfc_kfree(ap, cmnd->device );					//FCLNX-0147 FCLNX-0423
	}											//FCLNX-0147 FCLNX-0423
	hfc_kfree(ap, cmnd->request_buffer );				//FCLNX-0423
	hfc_kfree(ap, cmnd );								//FCLNX-0423
#endif			/* FCLNX-GPL-0343 */
	return RC;
}	/* end of hfc_inquiry */


/*
 * Function:    hfc_sciocmd
 *
 * Purpose:     Publish the SCSI command that a user appointed
 *
 * Arguments:   
 *  ap        - A pointer to an adap_info structure
 *  arg       - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  EINVAL    - Target device is not started, or data area is not ready
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred. *
 * 
 * Notes:       Issue SCSI command to a device directly
 *              This function is called by hfc_ioctl() (SCIOLCMD)
 */
int hfc_sciocmd( struct adap_info *ap, void *arg, int internal, int timeout) {
	int RC = 0;				/* return code */

	struct hfc_ioctl_cdb cdb;				/* hfc_ioctl_cdb structure            */
	struct scsi_cmnd   *cmnd;				/* A pointer to a struct scsi_cmnd structure    */
	struct target_info *target;				/* A pointer to a target_info structure  */
	ulong              flags = 0;			/* for spin_lock/unlock           */
#define MAX_CNT            0xffffffff

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	int					data_buf_num = 0, i;
	struct scatterlist	*sgl=NULL, *pre_sgl=NULL;
	struct dev_info		*dev = NULL;
#else
	int                cmnd_device_alloc_flag =0;	//FCLNX-0147
#endif
	ulong				page_link_addr[HFC_SCATTERLIST_NUM]={0};	/* FCLNX-GPL-FX-473 */

	HFC_ENTRY("hfc_sciocmd");

	if (internal != TRUE) {
		/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
		if ( COPYIN( (char *)arg, (char *)&cdb, sizeof(struct hfc_ioctl_cdb) ) != 0 ) {
		  HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x00 );
		  return EFAULT;
		}
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		/* Lower 32bit is effective for pointers */
		if(ap->ioctl32) {
			cdb.buffer    = cdb.buffer    & 0xffffffffU;
			cdb.sense_ptr = cdb.sense_ptr & 0xffffffffU;
		}
#endif
		HFC_ADAPLOCK_IRQSAVE(flags);
	}
	else {
		cdb = *((struct hfc_ioctl_cdb *) arg);
	}

	target = hfc_hash_target_info_wwn( ap, cdb.Port_WWN );

	if (internal != TRUE)
		HFC_ADAPUNLOCK_IRQRESTORE(flags);

	if (target == NULL || (cdb.lun_id >= MAX_DEV_CNT)) {
		/* Return target_info if found. Error return if target_info is not found */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x01 );
		return ENODEV ;
	}
	
	STRUCTDUMP( DMP_IOCMD, (uchar *)&cdb, sizeof(struct hfc_ioctl_cdb) );
	
	if (hfc_manage_info.lg_target_info) {										/* FCLNX-0408 */
		if( (cdb.flags & SCSI_PATH_HEALTH)
		 && (target->attribute == HFC_ATTR_UNCONTROLLED) ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_PTHEALTH, 0x0e );
			return ENODEV;
		}
	}																			/* FCLNX-0408 */
	
	/* Make sure whether an inquiry data region is set correctly */
	if( (cdb.command_length==0) || (cdb.command_length>16) ) {
		/* Command length is not valid */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x02 );
		return EINVAL;
	}

	if( (cdb.data_length==0) || (cdb.data_length>ap->dma_max) || (cdb.buffer==0) ) {
		/* Transfer data length is longer than ap->dma_max */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x03 );
		return EINVAL;
	}

	cmnd = ap->ioctl_cmnd;
	if( cmnd == NULL ){
		/* Return error if failed to allocate */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x03 );
		return ENOMEM;
	}
	
	if( cmnd->cmnd == NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x07 );
		return ENOMEM;
	}
	
	if( cmnd->device == NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x08 );
		return ENOMEM;
	}

#if 0			/* FCLNX-GPL-0343 */
	/* Allocate scsi_cmnd structure */
	if( ( cmnd = (struct scsi_cmnd *)hfc_kmalloc(ap, sizeof(struct scsi_cmnd), GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate scsi_cmnd */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x04 );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd, 0, sizeof(struct scsi_cmnd) );

	/* Setting of a struct scsi_cmnd structure */
	cmnd_device_alloc_flag = 0;
	if ( (cmnd->device = (struct scsi_device*)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate cmnd->device */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x08 );
		hfc_kfree(ap, cmnd );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd->device, 0, sizeof(struct scsi_device) );
	cmnd_device_alloc_flag = 1;
#endif

	cmnd->device->host = ap->hosts;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	dev = (struct dev_info *)hfc_get_dev_info(target, cdb.lun_id);

	if( dev == NULL ){
		dev = ap->ioctl_dev;
		memset( dev, 0, sizeof(struct dev_info) );
		dev->lun = (uint)cdb.lun_id;
		dev->target_id = target ->  target_id;
		set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
	}
	cmnd -> device -> hostdata = (struct dev_info *)dev;

	cmnd -> device -> request_queue -> rq_timeout = ap->ioctl_scsi_timeout * HZ;	/* FCLNX-GPL-0368 */
	cmnd -> sdb.length = cdb.data_length;
	cmnd -> transfersize = cdb.data_length;
	data_buf_num = cdb.data_length / HFC_PAGE_SIZE;
	if( (cdb.data_length % HFC_PAGE_SIZE) ){
		data_buf_num++;
	}
	cmnd -> sdb.table.nents = data_buf_num;
	memset( cmnd->cmnd, 0, 16 );
	sgl = cmnd -> sdb.table.sgl;
	if ( cmnd -> sdb.table.nents > 0 )
	{
		for ( i = 1; i < cmnd -> sdb.table.nents; i++ )
		{	/* FCLNX-GPL-FX-470 */
			sgl -> dma_length = HFC_PAGE_SIZE ;
			sgl->page_link &= ~0x02;
			memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
			if( cdb.flags & SCSI_WRITE_CDB ){
				page_link_addr[i-1] = sgl->page_link;	/* FCLNX-GPL-FX-473 */
				memcpy((char *)sgl->page_link, (char *)(char *)((ulong)cdb.buffer + HFC_PAGE_SIZE * (i-1)), sgl->dma_length);	/* FCLNX-GPL-FX-473 */
				sg_set_buf(sgl, (char *)(char *)((ulong)cdb.buffer + HFC_PAGE_SIZE * (i-1)), sgl->dma_length);
			}	/* FCLNX-GPL-FX-470 */
			pre_sgl = sgl;
			sgl++;
		}
		if ( cdb.data_length % HFC_PAGE_SIZE != 0 )
		{
			/* FCLNX-GPL-FX-470 Start */
			sgl->dma_length = cdb.data_length % HFC_PAGE_SIZE ;
			sgl->page_link &= ~0x02;
			memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
			if( cdb.flags & SCSI_WRITE_CDB ){
				page_link_addr[i-1] = sgl->page_link;	/* FCLNX-GPL-FX-473 */
				memcpy((char *)sgl->page_link, (char *)((ulong)cdb.buffer + HFC_PAGE_SIZE * (cmnd -> sdb.table.nents -1)), sgl->dma_length);	/* FCLNX-GPL-FX-473 */
				sg_set_buf(sgl, (char *)((ulong)cdb.buffer + HFC_PAGE_SIZE * (cmnd -> sdb.table.nents -1)), sgl->dma_length);
			}
			pre_sgl = sgl;
		}
		if(pre_sgl != NULL)pre_sgl->page_link |= 0x02;
		/* FCLNX-GPL-FX-470 End */
	}else{
		sgl->dma_length = 0 ;
	}
#else
	cmnd -> timeout_per_command = 30 * HZ;
	cmnd -> transfersize        = cdb.data_length;	//FCLNX-0184
	cmnd -> request_bufflen     = cdb.data_length;	//FCLNX-0184
#endif

	CMND_TARGET(cmnd)           = target ->  target_id;
	CMND_LUN(cmnd)              = (uint) cdb.lun_id;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	if( ( cmnd->request_buffer  = hfc_kmalloc(ap, cdb.data_length, GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate request_buffer area */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x05 );
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_kfree(ap, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd );
		return ENOMEM;
	}
	memset( cmnd->request_buffer, 0, cdb.data_length );
#else
	if( ( cmnd->sense_buffer  = hfc_kmalloc(ap, SCSI_SENSE_BUFFERSIZE, GFP_ATOMIC) )==NULL ) {		/* FCLNX-GPL-396 */
		/* Error if failed to allocate request_buffer area */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x09 );
		hfc_kfree(ap, cmnd->sense_buffer );					//FCLNX-0423
		return ENOMEM;
	}
	memset( cmnd->sense_buffer, 0, SCSI_SENSE_BUFFERSIZE );											/* FCLNX-GPL-396 */
#endif

	if( cdb.flags & SCSI_READ_CDB )
		cmnd -> sc_data_direction = SCSI_DATA_READ;
	else
		cmnd -> sc_data_direction = SCSI_DATA_WRITE;

	/* Setting of an SCSI command */
	cmnd   -> cmd_len           = cdb.command_length;
	HFC_MEMCPY( cmnd->cmnd, cdb.cdb, cdb.command_length );
	cmnd   -> tag               = cdb.tag_q;

	RC = HFC_STRATEGY( cmnd, (void *) hfc_ioctl_iodone );
	/* Need to sleep here. (iodone is executed even if error occurred in hfc_strategy)  */
	hfc_ioctl_sleep( ( struct scsi_cmnd * )cmnd );
	if( RC ) {
		/* When an error occurred in strategy() */
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
		hfc_kfree(ap, cmnd->device );					//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd->request_buffer );
		hfc_kfree(ap, cmnd );
#endif
		hfc_kfree(ap, cmnd->sense_buffer );		/* FCLNX-GPL-396 */
		return EIO;
	}
	STRUCTDUMP( DMP_SCSICMD, (uchar *)cmnd    , sizeof(struct scsi_cmnd) );

	RC = hfc_rtn_scsi_cmnd( cmnd, &cdb.scsi_status, (uint64_t *)(ulong)cdb.buffer,
									((internal != TRUE) ? TRUE : FALSE) );	//FCLNX-GPL-0324,0329
	if( cdb.flags & SCSI_WRITE_CDB ){	/* FCLNX-GPL-FX-473 */
		sgl = cmnd -> sdb.table.sgl;
		for(i=0;i<HFC_SCATTERLIST_NUM;i++){
			if( sgl == NULL ) break;
			if( page_link_addr[i] != 0 ){
				sgl->page_link = page_link_addr[i];
			}
			sgl ++;
		}
	}	/* FCLNX-GPL-FX-473 */
					
	if( RC==EFAULT ) {
		/* Write back failed for sense_buffer or request_buffer */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x06 );
#if 0				/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_kfree(ap, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_kfree(ap, cmnd->request_buffer );
		hfc_kfree(ap, cmnd );
#endif
		hfc_kfree(ap, cmnd->sense_buffer );		/* FCLNX-GPL-396 */
		return EFAULT;
	}
	
	if ( (hfc_manage_info.hfcldd_mp_mod)									/* FCLNX-GPL-449 */
		&& (hfc_manage_info.lg_target_info)
		&& (cmnd->cmnd[0] == 0xA0) ) {
			hfc_manage_info.npubp->hfc_check_luconfig(target, cmnd);
	}																		/* FCLNX-GPL-449 */
	
	if (internal != TRUE) {						//FCLNX-GPL-0324,0329
		/* Write back data to hfc_ioctl_cdb */
		if( COPYOUT( (char *)&cdb, (char *)arg, sizeof(struct hfc_ioctl_cdb) ) != 0 ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x07 );
			RC =EFAULT;
		}
	}
	else {
		*((struct hfc_ioctl_cdb *) arg) = cdb;
	}

	STRUCTDUMP( DMP_IOCMD, (uchar *)&cdb, sizeof(struct hfc_ioctl_cdb) );

#if 0				/* FCLNX-GPL-0343 */
	if ( cmnd_device_alloc_flag == 1 ) {		//FCLNX-0147 FCLNX-0423
		hfc_kfree(ap, cmnd->device );					//FCLNX-0147 FCLNX-0423
	}											//FCLNX-0147 FCLNX-0423
	hfc_kfree(ap, cmnd->request_buffer );				//FCLNX-0423
	hfc_kfree(ap, cmnd );								//FCLNX-0423
#endif
	hfc_kfree(ap, cmnd->sense_buffer );		/* FCLNX-GPL-396 */
	return RC;
}  


/*
 * Function:    hfc_get_all_port_ids
 *
 * Purpose:     Get all port IDs (SCSI_ID) connected to name server
 *
 * Arguments:   
 *  ap        - A pointer to an adap_info structure
 *  arg       - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area.
 *  EINVAL    - Adapter is not connected to fabric, or is not in Public AL.
 *              Setup error in port list area.
 *              Setup of CT_IU is error 
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred. *
 * Notes:       This function is called by hfc_ioctl() (SCIOLNMSRV)
 */
int hfc_get_all_port_ids( struct adap_info *ap, void *arg ) {
	int		rtn = 0;
	int		rc  = 0,rc1 = 0;

	struct hfc_ioctl_gidft	gidft;
	struct FS_ACC           *fs_acc;
	dma_addr_t	        fs_acc_busaddr;

	uint	uiwork;
	ushort	uswork;
	uchar   log_err;
	int     i;
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */


	HFC_ENTRY("hfc_get_all_port_ids");

	if( !(( ap ->connect_type==HFC_SWITCH ) || ( (ap->connect_type==HFC_AL) && (ap->scsi_id & 0x00ffff00) )) ) {
		/* Error if adapter is not connected to fabric directly, or not in public AL */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x00 );
		return EINVAL;
	}

	/* Copy data to an internal data area as hfc_ioctl_gidft structure */
	if( COPYIN( (char *)arg, (char *)&gidft, sizeof(struct hfc_ioctl_gidft) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x01 );
		return EFAULT;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(ap->ioctl32) {
		gidft.scsi_id_list = gidft.scsi_id_list & 0xffffffffU;
	}
#endif
	
	STRUCTDUMP( DMP_GIDFT, (uchar *)&gidft, sizeof(struct hfc_ioctl_gidft) );
	
	if( (gidft.list_len<4) || (gidft.list_len>4064) || (gidft.scsi_id_list==0) ) {
		/* Make sure pointer to SCSI_ID data area and length for SCSI_ID is correct */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x02 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		return EINVAL;
	}
	
	/* Clear status */
	gidft.adapter_status = 0x00;
	gidft.set_flags      = 0x00;
	gidft.scsi_id_size   = 0x00;
	gidft.num_ids        = 0;
	/*
	 * Allocate and page-map the area for payload
	 *
	 */
	/* Allocate and page-map fs_acc */
	fs_acc = (struct FS_ACC *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf, 8192, &fs_acc_busaddr);
	if( fs_acc == NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x03 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		return ENOMEM;
	}
	if ( (ulong)fs_acc & 0x0fff ) {
		/* FS-ACCx2 is not on a 4kB boudnary */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		hfc_pci_free_consistent(ap, ap->pci_cfginf, 8192, (void *)fs_acc, fs_acc_busaddr );
		hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0x58, (uchar *)&fs_acc, sizeof(void *) );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x04 );
		return ENOMEM;
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	BZERO( (char *)fs_acc, 8192 );
	lock_mailbox( ap );	/* Lock mailbox */
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	/* Create mailbox control block based on DRVIOCTL2/SCIOLNMSRV */
	ap -> mb -> mb_init.command = HFC_MBCMD_FCCTL;
	ap -> mb -> mb_init.sub_cmd = HFC_MBSCMD_NMSRV;
	ap -> mb_results = 0;
	uiwork = HFC_DIR_SERV_ID;
	if ( ap->connect_type == HFC_AL ) uiwork |= HFC_DIR_SERV_ID << 24;
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.adr.d_id,   uiwork );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.payload_length, 20 );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.flag,            0 );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.response_length,       (  uint  )HFC_PAGE_SIZE  );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.response_list_address, (uint64_t)fs_acc_busaddr );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.rev,        1 );	/*FC_GS_2       */
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.fs_type, 0xfc );	/*FC_CT_DIR_SERV*/
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.fs_sub,  0x02 );	/*FC_NAME_SERVER*/
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.options,    0 );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.com_rsp_code, 0x0171 ); /*FC_NS_GID_FT  */
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.max_res_size, (ushort)(gidft.list_len/4) );
	hfc_write_val( ap->mb->mb_init.type.drvioctl2.un.gid_ft.type,    0x08 );	/*FCPH_TYPE_FCP */

	/* 
	 * Section 4.4.4 in FC-GS-2 describes that root control is not always required. 
	 * Section 18.2 in FC-PH shows root control.
	 * r_ctl = 0x02
	 */

	/* 
	 * FC4_TYPE field is related to FC type code in frame header (Refer Table 34,35,36 in FC-PH)
	 * Section 4.4.8 in FC-GS-2 describes FC4_TYPE should fibre channel service type for directory 
	 * service in FC-GS-2.
	 * fc4_type = FCPH_TYPE_FC_SRVS
	 */ 

	/* Mailbox processing */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	rc = hfc_mailbox_proc( ap, HFC_ELS_TMR, (gidft.timeout_value ? gidft.timeout_value : HFC_ELS_TO), ap->els_retry );	/* FCLNX-0523 */
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	
	if( rc == 0 ) {
		if( (ushort)hfc_read_val( fs_acc->com_rsp_code ) == HFC_GS_ACC ) {
			/* Completed? */
			gidft.scsi_id_size = 4;
			
			uswork = (ushort)hfc_read_val( fs_acc->max_res_size );
			if ( uswork != 0 ) {
				/* Set necessary size if area is too small to read all ports. */
				gidft.set_flags |= NM_LIST_SHORT;
				gidft.list_len  += uswork * 4;
			}
			
			/* Get the number of the port ID and delete a mark of a last entry */
			gidft.num_ids = ( gidft.num_ids > 508*2 ) ? 508*2 : (uint)hfc_read_val( ap->mb->mb_resp.type.drvioctl2.gid_ft.port_number );
			if( gidft.num_ids>0 ) {
				i = 0;
				if( gidft.num_ids > 508 ) i++;
				fs_acc[ i ].port_id[ gidft.num_ids-508*i-1 ] &= 0x00ffffff;
				
				/* Copy port ID to a buffer(caller side) */							/* FCLNX-0405 */
				for (i=0;i<gidft.num_ids;i++) {
					uiwork = (uint)hfc_read_val(fs_acc[(i < 508) ? 0 : 1].port_id[(i < 508) ? i : (i-508)]);
					uiwork &= 0x00ffffff;
					
					rc1 = COPYOUT( (char *)&uiwork, (char *)(ulong)(gidft.scsi_id_list+i*4), 4 );
					
					if(rc1!=0) {
						/* Unlock mailbox */
						unlock_mailbox( ap );
						HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
						
						hfc_pci_free_consistent(ap, ap->pci_cfginf, 8192, (void *)fs_acc, fs_acc_busaddr );
						
						HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x05 );
						return EFAULT;
					}
				}																	/* FCLNX-0405 */
			}
			ap -> used_nmsrv = TRUE;	/* Set name server availability */
		}
		else {
			/* Name server refused a request. Set a return code based on a reason code */
			log_err = TRUE;
			switch( fs_acc->reason ) {
			case HFC_GS_INV_CODE:    /* CT_IU command cord is invalid */
			case HFC_GS_INV_VERS:    /* CT_IU version is invalid      */
			case HFC_GS_INV_IU_SIZE: /* IIU size is invalid           */
			case HFC_GS_CMD_NOT_SPT: /* CT_IU command is not supported*/
				rtn = EINVAL;
				break;
			case HFC_GS_BSY:
				rtn = EBUSY;
				break;
			case HFC_GS_FAILED:
				if( fs_acc->rsn_exp == HFS_GS_EXP_NO_FC4S )  {
					/* Do not collect error log if name server shows FCP device is not registered */
					log_err = FALSE;
					rtn     = ENXIO;
				}
				else
					rtn = EIO;
				break;
			default:
				rtn = EIO;
			}
			if( log_err ) hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR6, 0x59, (uchar *)&( fs_acc->reason ), sizeof(uchar) );
		}
	}

	if( rtn == 0 ) rtn = rc;  
	unlock_mailbox( ap );   /* Unlock mailbox */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	hfc_pci_free_consistent(ap, ap->pci_cfginf, 8192, (void *)fs_acc, fs_acc_busaddr);

	/* Write-back data to hfc_ioctl_gidft structure  */
	if( COPYOUT( (char *)&gidft, (char *)arg, sizeof( struct hfc_ioctl_gidft ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x06 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_GIDFT, (uchar *)&gidft, sizeof(struct hfc_ioctl_gidft) );

	return rtn;
}	


/*
 * Function:    hfc_get_sid_from_wwpn
 *
 * Purpose:     Search port ID(SCSI_ID) with specific WWN
 *
 * Arguments:   
 *  ap        - Pointer to an adap_info structure 
 *  arg       - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area.
 *  EINVAL    - Adapter is not connected to fabric, or is not in Public AL.
 *              Setup error in port list area.
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred. 
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLQWWN)
 */
int hfc_get_sid_from_wwpn( struct adap_info *ap, void *arg ) {
	int		rtn = 0;

	struct hfc_ioctl_gidpn	get_wwn;
	struct target_info	*target;
	
	uint    uiwork;
	ulong   flags = 0 ;


	HFC_ENTRY("hfc_get_sid_from_wwpn");

	/* Copy data to an internal data area as hfc_ioctl_gidpn structure */
	if ( COPYIN( (char *)arg, (char *)&get_wwn, sizeof(struct hfc_ioctl_gidpn) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_QWWN, (uchar *)&get_wwn, sizeof(struct hfc_ioctl_gidpn) );
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	if ( get_wwn.world_wide_name == 0 ) {
		/* Error if WWN is not specified */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x01 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		return EINVAL;
	}


	/* Clear status */
	get_wwn.adapter_status = 0x00;
	get_wwn.scsi_id        = 0;
	if ( ( ap->connect_type==HFC_SWITCH ) || ( (ap->connect_type==HFC_AL) && (ap->scsi_id & 0x00ffff00) ) ) {
		/*
		 * Initiate mailbox process to issue GID_PN to fubric if this adapter is connected to fubric 
		 * directly or is in public AL
		 */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		lock_mailbox( ap );	/* Lock mailbox */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

		/* Assemble a mailbox control block */
		ap -> mb -> mb_init.command = HFC_MBCMD_FCCTL;
		ap -> mb -> mb_init.sub_cmd = HFC_MBSCMD_GIDPN;
		ap -> mb_results = 0;
		uiwork = HFC_DIR_SERV_ID;
		if( ap -> connect_type == HFC_AL ) uiwork |= HFC_DIR_SERV_ID << 24;
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.adr.d_id,        uiwork );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.payload_length,      24 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.rev,        1 );		/*FC_GS_2       */
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.fs_type, 0xfc );		/*FC_CT_DIR_SERV*/
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.fs_sub,  0x02 );		/*FC_NAME_SERVER*/
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.options,    0 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.com_rsp_code, 0x0121 );	/*FC_NS_GID_PN  */
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.max_res_size, 0x0001 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gid_pn.port_name,    get_wwn.world_wide_name );

		/* 
		 * Section 4.4.4 in FC-GS-2 describes that root control is not always required. 
		 * Section 18.2 in FC-PH shows root control.
		 * r_ctl = 0x02
		 */

		/* 
		 * FC4_TYPE field is related to FC type code in frame header (Refer Table 34,35,36 in FC-PH)
		 * Section 4.4.8 in FC-GS-2 describes FC4_TYPE should fibre channel service type for directory 
		 * service in FC-GS-2.
		 * fc4_type = FCPH_TYPE_FC_SRVS
	 	*/ 

		/* Mailbox processing */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		rtn = hfc_mailbox_proc( ap, HFC_ELS_TMR, HFC_ELS_TO, ap->els_retry );	/* FCLNX-0523 */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
		if( rtn == 0 ) {
			/* Setting of PORT_ID(SCSI_ID) */
			get_wwn.scsi_id = (uint64_t)( hfc_read_val(ap->mb->mb_resp.type.drvioctl1.gid_pn.port_id) & 0x00ffffff );
			/* Set name server availability */
			ap -> used_nmsrv = TRUE;
		}
		unlock_mailbox( ap );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	}
	else
	{
		/* Search target_info and find SCSI ID if adapter is not in fabric or in public AL */
		if ( ( target = hfc_hash_target_info_wwn( ap, get_wwn.world_wide_name ) ) != NULL ) {
			get_wwn.scsi_id = (uint64_t) target -> scsi_id ;
		}
		else	/* Return ENODEV if target_info is not found */
		{
			/* HFC_ADAPUNLOCK_IRQRESTORE(flags); */ /* FCLNX-GPL-FX-505 */
			rtn = ENODEV;
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
	}


	/* Write back data to hfc_ioctl_gidpn structure */
	if( COPYOUT( (char *)&get_wwn, (char *)arg, sizeof(struct hfc_ioctl_gidpn) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_QWWN, (uchar *)&get_wwn, sizeof(struct hfc_ioctl_gidpn) );

	return rtn;
}	


/*
 * Function:    hfc_get_wwpn_from_sid
 *
 * Purpose:     Search port ID(SCSI_ID) and find corresponding WWPN
 *
 * Arguments:   
 *  ap        - Pointer to an adap_info structure 
 *  arg       - A pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  EINVAL    - Target device is not started, or data area is not ready
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred.

 *
 * Notes:       
 */
int hfc_get_wwpn_from_sid( struct adap_info *ap, void *arg ) {		/* FCLNX-0405 */
	int		rtn=0,lp=0,hit=0;

	struct hfc_ioctl_gpnid	get_sid;
	struct target_info	*target;
	uint    uiwork;
    ulong   flags=0 ;


	HFC_ENTRY("hfc_get_wwpn_from_sid");

	/* Copy data to an internal data area as hfc_ioctl_gidpn structure */
	if ( COPYIN( (char *)arg, (char *)&get_sid, sizeof(struct hfc_ioctl_gpnid) ) != 0 ) {
		return EFAULT;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	if ( get_sid.scsi_id == 0 ) {
		/* Error if SCSI ID is not set */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		return EINVAL;
	}

	/* Clear status  */
	get_sid.adapter_status  = 0x00;
	get_sid.world_wide_name = 0;
	if ( ( ap->connect_type==HFC_SWITCH ) || ( (ap->connect_type==HFC_AL) && (ap->scsi_id & 0x00ffff00) ) ) {
		/*
		 * Initiate mailbox process to issue GID_PN to fubric if this adapter is connected to fubric 
		 * directly or is in public AL
		 */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		lock_mailbox( ap );	/* Lock mailbox */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

		/* Create mailbox control block */
		ap -> mb -> mb_init.command = HFC_MBCMD_FCCTL;
		ap -> mb -> mb_init.sub_cmd = HFC_MBSCMD_GPNID;
		ap -> mb_results = 0;
		uiwork = HFC_DIR_SERV_ID;
		if( ap -> connect_type == HFC_AL ) uiwork |= HFC_DIR_SERV_ID << 24;
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.adr.d_id,        uiwork );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.payload_length,      20 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.rev,        1 );		/*FC_GS_2       */
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.fs_type, 0xfc );		/*FC_CT_DIR_SERV*/
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.fs_sub,  0x02 );		/*FC_NAME_SERVER*/
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.options,    0 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.com_rsp_code, 0x0112 );	/*FC_NS_GPN_ID  */
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.max_res_size, 0x0002 );
		hfc_write_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.port_id, (get_sid.scsi_id & 0xffffff) );

		/* 
		 * Section 4.4.4 in FC-GS-2 describes that root control is not always required. 
		 * Section 18.2 in FC-PH shows root control.
		 * r_ctl = 0x02
		 */

		/* 
		 * FC4_TYPE field is related to FC type code in frame header (Refer Table 34,35,36 in FC-PH)
		 * Section 4.4.8 in FC-GS-2 describes FC4_TYPE should fibre channel service type for directory 
		 * service in FC-GS-2.
		 * fc4_type = FCPH_TYPE_FC_SRVS
	 	*/ 

		/* Mailbox processing */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		rtn = hfc_mailbox_proc( ap, HFC_ELS_TMR, HFC_ELS_TO, ap->els_retry );	/* FCLNX-0523 */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
		if( rtn == 0 ) {
			/* Setting of WORLD WIDE PORT NAME */
			get_sid.world_wide_name  = hfc_read_val(ap->mb->mb_resp.type.drvioctl1.gpn_id.port_name_hi );
			get_sid.world_wide_name <<= 32;
			get_sid.world_wide_name |= hfc_read_val(ap->mb->mb_resp.type.drvioctl1.gpn_id.port_name_lo );

			/* Set name server availability */
			ap -> used_nmsrv = TRUE;
		}
		unlock_mailbox( ap );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	}
	else
	{
		/* Search target_info and find SCSI ID if adapter is not in fabric or in public AL */
		hit=0;
		
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)										/* FC-GW */
		{
			target = hfc_hash_target_info(ap, lp);
		
			if (target != NULL)
			{
				if (target->scsi_id == get_sid.scsi_id) {
					get_sid.world_wide_name = (uint64_t) target -> ww_name ;
					hit = 1;
					break;
				}
			}
		}
		
		if (!hit) {	/* When target_info is not found, return ENODEV */
			rtn = ENODEV;
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
	}

	/* Write back data to hfc_ioctl_gidpn structure */
	if( COPYOUT( (char *)&get_sid, (char *)arg, sizeof(struct hfc_ioctl_gpnid) ) != 0 ) {
		rtn = EFAULT;
	}

	return rtn;
}	


/*
 * Function:    hfc_payload
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to an adap_info structure 
 *  arg       - A pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  EINVAL    - Target device is not started, or data area is not ready
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred.
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLPAYLD)
 */
int hfc_payload( struct adap_info *ap, void *arg ) {
	int		rtn = 0;

	struct hfc_ioctl_payload	payld;
	struct PAYLD	*payload;
	struct PAYLD	*response;
	dma_addr_t	payload_busaddr;		/* bus address of payload */
	dma_addr_t	response_busaddr;		/* bus address of response */
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */



	HFC_ENTRY("hfc_payload");
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	if( !test_bit( HFC_ONLINE, (ulong *)&ap->status ) ) {
		/* Adapter is not online */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x00 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-505 */
		return EIO;
	}
	
	if( test_bit( HFC_ISOL, (ulong *)&ap->status ) ) {	/* FCLNX-GPL-572 */
		/* Adapter is isolated, or executing port isolation.*/
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x00 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-505 */
		return EIO;
	}													/* FCLNX-GPL-572 */
	
	if(!hfc_mlpf_check_normal_hypsts(ap)) {		/* FCLNX-GPL-428 */
		/* Adapter is not normal status in mlpf shared mode. */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x00 );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-505 */
		return EIO;
	}											/* FCLNX-GPL-428 */
	
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */

	/* Copy data to an internal data area as hfc_ioctl_payload structure */
	if( COPYIN( (char *)arg, (char *)&payld, sizeof(struct hfc_ioctl_payload) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x01 );
		return EFAULT;
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(ap->ioctl32) {
		payld.payld_buffer    = payld.payld_buffer    & 0xffffffffU;
		payld.response_buffer = payld.response_buffer & 0xffffffffU;
	}
#endif
	
	STRUCTDUMP( DMP_PAYLD, (uchar *)&payld, sizeof(struct hfc_ioctl_payload) );

	if( (payld.payld_size>2048) || (payld.response_size>2048) ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x02 );
		return EINVAL;
	}

	/*
	 * Allocate payload area
	 */
	payload  = ( struct PAYLD *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), &payload_busaddr );
	if( payload == NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x03 );
		return ENOMEM;
	}
	if( (ulong)payload & 0x0fff ) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)payload, payload_busaddr );
		hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x5a, ( uchar * )&payload,  sizeof(struct PAYLD *) );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x04 );
		return ENOMEM;
	}
	BZERO( (char *)payload,  (uint)sizeof(struct PAYLD) );


	/*
	 * Allocate response area
	 */
	response = ( struct PAYLD *)hfc_pci_alloc_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), &response_busaddr  );
	if(response == NULL) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x05 );
		return ENOMEM;
	}
	if( (ulong)response & 0x0fff ) {
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)payload, payload_busaddr);
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x06 );
		hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x5b, ( uchar * )&response, sizeof(struct PAYLD *) );
		return ENOMEM;
	}
	BZERO( (char *)response, (uint)sizeof(struct PAYLD));


	lock_mailbox( ap );	/* Lock mailbox */
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	/* Create a mailbox control block based on FRMSNDRCV(DRVIOCTL3) */
	ap -> mb -> mb_init.command = HFC_MBCMD_FCCTL;
	ap -> mb -> mb_init.sub_cmd = HFC_MBSCMD_SNDRCV;
	ap -> mb_results = 0;
	hfc_write_val( ap->mb->mb_init.type.drvioctl3.send_address,   (uint64_t)payload_busaddr  );/* Bus address of payload part  */
	hfc_write_val( ap->mb->mb_init.type.drvioctl3.receive_address,(uint64_t)response_busaddr );/* Bus address of response part */
	hfc_write_val( payload -> flags,          (uchar) payld.flags );
	hfc_write_val( payload -> payld_class,    (uchar) payld.class );
	hfc_write_val( payload -> payload_length, (ushort)payld.payld_size );
	hfc_write_val( payload -> dst.d_id,       (uint) (payld.scsi_id & 0x00ffffff) );
	hfc_write_val( payload -> dst.rctl,       (uchar) payld.ctl  );
	hfc_write_val( payload -> type,           (uchar) payld.type );
	STRUCTDUMP( DMP_FRMPALRD, (uchar *)(ulong)payld.payld_buffer, payld.payld_size );
	if( COPYIN( (uchar *)(ulong)payld.payld_buffer, (uchar *)&(payload->frame_payload), payld.payld_size ) != 0 ) {
		unlock_mailbox( ap );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)payload,  payload_busaddr  );
		hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)response, response_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x07 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_PAYLOAD,  (uchar *)payload,                sizeof(struct PAYLD) );
	STRUCTDUMP( DMP_FRMPALRD, (uchar *)payload->frame_payload, payld.payld_size );


	/* 
	 * Section 4.4.4 in FC-GS-2 describes that root control is not always required. 
	 * Section 18.2 in FC-PH shows root control.
	 * r_ctl = 0x02
	 */

	/* 
	 * FC4_TYPE field is related to FC type code in frame header (Refer Table 34,35,36 in FC-PH)
	 * Section 4.4.8 in FC-GS-2 describes FC4_TYPE should fibre channel service type for directory 
	 * service in FC-GS-2.
	 * fc4_type = FCPH_TYPE_FC_SRVS
	 */ 

	/* Mailbox processing */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	rtn = hfc_mailbox_proc( ap, HFC_ELS_TMR, HFC_ELS_TO, ap->els_retry );	/* FCLNX-0523 */
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	if( rtn == 0 ) {

		int resp_size = ( response->payload_length > payld.response_size ) ? payld.response_size : response->payload_length;
		STRUCTDUMP( DMP_RSPPALRD, (uchar *)&(response->frame_payload), resp_size );
		if( COPYOUT( (uchar *)&(response->frame_payload), (char *)(ulong)payld.response_buffer, resp_size ) != 0 ) {
			rtn = EFAULT;
		}
		STRUCTDUMP( DMP_RESPONSE, (uchar *)response,                   sizeof(struct PAYLD) );
		STRUCTDUMP( DMP_RSPPALRD, (uchar *)&(response->frame_payload), resp_size );
	}
	unlock_mailbox( ap );
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)payload,  payload_busaddr  );
	hfc_pci_free_consistent(ap, ap->pci_cfginf, (uint)sizeof(struct PAYLD), (void *)response, response_busaddr );

	/* Write back data to hfc_ioctl_payload structure */
	if( COPYOUT( (char *)&payld, (char *)arg, sizeof( struct hfc_ioctl_payload ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x08 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_PAYLD, (uchar *)&payld, sizeof(struct hfc_ioctl_payload) );

	return rtn;
}


/*
 * Function:    hfc_get_rnid
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLCHBA)
 */

int hfc_get_rnid( struct adap_info *ap, void *arg ) {
	int    rtn =0;  
	struct hfc_ioctl_rnid rnid;


	HFC_ENTRY("hfc_get_rnid");

	/* Copy data to an internal data area as hfc_ioctl_rnid structure */
	if( COPYIN( (char *)arg, (char *)&rnid, sizeof(struct hfc_ioctl_rnid) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GRNID, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_GRNID, (uchar *)&rnid, sizeof(struct hfc_ioctl_rnid) );
	STRUCTDUMP( DMP_GRNID, (uchar *)ap->fw_init_p, 1872 );

	/* Copy RNID info in fw_init_table to hfc_ioctl_rnid structure */
	rnid.adapter_status   = 0x00; 		/* Set zero because there is no corresponding data */
	hfc_write_val( rnid.common_node_len , ap->fw_init_p->fw_rnid.com_nid_len      );
	hfc_write_val( rnid.spcfic_node_len , ap->fw_init_p->fw_rnid.spec_nid_len     );
	hfc_write_val( rnid.port_name       , ap->fw_init_p->fw_rnid.n_port_name      );
	hfc_write_val( rnid.node_name       , ap->fw_init_p->fw_rnid.node_name        );
	hfc_write_val( rnid.vendor_unique[0], ap->fw_init_p->fw_rnid.vendor_unique[0] );
	hfc_write_val( rnid.vendor_unique[1], ap->fw_init_p->fw_rnid.vendor_unique[1] );
	hfc_write_val( rnid.node_type       , ap->fw_init_p->fw_rnid.node_type        );
	hfc_write_val( rnid.port_number     , ap->fw_init_p->fw_rnid.port_number      );
	hfc_write_val( rnid.num_att_nodes   , ap->fw_init_p->fw_rnid.num_att_nodes    );
	hfc_write_val( rnid.node_mgmt       , ap->fw_init_p->fw_rnid.node_mgmt        );
	hfc_write_val( rnid.ip_ver          , ap->fw_init_p->fw_rnid.ip_version       );
	hfc_write_val( rnid.udp_port        , ap->fw_init_p->fw_rnid.udp_port         );
	hfc_write_val( rnid.ip_addr[0]      , ap->fw_init_p->fw_rnid.ip_addr[0]       );
	hfc_write_val( rnid.ip_addr[1]      , ap->fw_init_p->fw_rnid.ip_addr[1]       );
	hfc_write_val( rnid.disc_flags      , ap->fw_init_p->fw_rnid.disc_flags       );

	/* Write back data to hfc_ioctl_rnid structure */
	if( COPYOUT( ( char * )&rnid, ( char * )arg, sizeof( struct hfc_ioctl_rnid ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GRNID, 0x01 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_GRNID, (uchar *)&rnid, sizeof(struct hfc_ioctl_rnid) );

	return rtn;
}


/*
 * Function:    hfc_set_rnid
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLCHBA)
 */

int hfc_set_rnid( struct adap_info *ap, void *arg ) {
	int    rtn =0;  
	struct hfc_ioctl_rnid rnid;
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */


	HFC_ENTRY("hfc_set_rnid");

	if( !test_bit(HFC_ONLINE, (ulong *)&ap->status) ) {
		/* Adapter is not online */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SRNID, 0x01 );
		return EIO;
	}
	
	if( test_bit(HFC_ISOL, (ulong *)&ap->status) ) {	/* FCLNX-GPL-572 */
		/* Adapter is isolated, or executing port isolation.*/
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SRNID, 0x01 );
		return EIO;
	}													/* FCLNX-GPL-572 */
	
	if(!hfc_mlpf_check_normal_hypsts(ap)) {		/* FCLNX-GPL-428 */
		/* Adapter is not normal status in mlpf shared mode. */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x00 );
		return EIO;
	}											/* FCLNX-GPL-428 */
	
	/* Copy data to an internal data area as hfc_ioctl_rnid structure */
	if( COPYIN( (char *)arg, (char *)&rnid, sizeof(struct hfc_ioctl_rnid) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SRNID, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_SRNID, (uchar *)&rnid, sizeof(struct hfc_ioctl_rnid) );


	/* SET_RNID is not supported. Return EINVAL */
	return EINVAL;

	rnid.adapter_status = 0x00;
	/* Copy hfc_ioctl_rnid structure data into RNID information in fw_init_table */
	              ap->fw_init_p->fw_rnid.com_nid_len  =    rnid.common_node_len   ;
	              ap->fw_init_p->fw_rnid.spec_nid_len =    rnid.spcfic_node_len   ;
	hfc_write_val(ap->fw_init_p->fw_rnid.n_port_name,      rnid.port_name );
	hfc_write_val(ap->fw_init_p->fw_rnid.node_name,        rnid.node_name );
	hfc_write_val(ap->fw_init_p->fw_rnid.vendor_unique[0], rnid.vendor_unique[0] );
	hfc_write_val(ap->fw_init_p->fw_rnid.vendor_unique[1], rnid.vendor_unique[1] );
	hfc_write_val(ap->fw_init_p->fw_rnid.node_type,        rnid.node_type );
	hfc_write_val(ap->fw_init_p->fw_rnid.port_number,      rnid.port_number      );
	hfc_write_val(ap->fw_init_p->fw_rnid.num_att_nodes,    rnid.num_att_nodes    );
	              ap->fw_init_p->fw_rnid.node_mgmt   =     rnid.node_mgmt  ;
	              ap->fw_init_p->fw_rnid.ip_version  =     rnid.ip_ver     ;
	hfc_write_val(ap->fw_init_p->fw_rnid.udp_port,         rnid.udp_port  );
	hfc_write_val(ap->fw_init_p->fw_rnid.ip_addr[0],       rnid.ip_addr[0]);
	hfc_write_val(ap->fw_init_p->fw_rnid.ip_addr[1],       rnid.ip_addr[1]);
	hfc_write_val(ap->fw_init_p->fw_rnid.disc_flags,       rnid.disc_flags);

	lock_mailbox( ap );	/* Lock mailbox */
	/* Create a mailbox control block based on CHNGRNID(DRVLOGB0) */
	ap -> mb -> mb_init.command = HFC_MBCMD_LOGTRACE;
	ap -> mb -> mb_init.sub_cmd = HFC_MBSCMD_CHNGRNID;
	ap -> mb_results = 0;
	/* Initiate Mailbox */ /* FCLNX-GPL-243 */
	if( ( rtn = hfc_mailbox_proc( ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry ) ) == 0 ) {/* FCLNX-0523 */
		/* Set status */
		/* Set zero in rnid.adapter_status because there is no corresponding data */
		//	  rnid.adapter_status = 0;
	}
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */


	/* Write back data to hfc_ioctl_rnid structure */
	if( COPYOUT( ( char * )&rnid, ( char * )arg, sizeof( struct hfc_ioctl_rnid ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SRNID, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_SRNID, (uchar *)&rnid, sizeof(struct hfc_ioctl_rnid) );

	return rtn;
}


/*
 * Function:    hfc_adap_stat
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLCHBA)
 */

int hfc_adap_stat( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_adap_stat adapstat;
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */


	HFC_ENTRY("hfc_adap_stat");

	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_ioctl_adap_stat structure */
	if( COPYIN( (char *)arg, (char *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );


	adapstat.adapter_status = 0x00;   /* Set zero */

	lock_mailbox( ap );	/* Lock mailbox */
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */

	/* Create a mailbox control block based on CHNGRNID(DRVLOGB0) */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_CTLPORTSTAT;
	ap -> mb_results = 0;
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );
	/* Mailbox processing */ /* FCLNX-GPL-243 */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	if( ( rtn = hfc_mailbox_proc( ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry ) ) == 0 ) {/* FCLNX-0523 */
		adapstat.seconds_since_last_reset = 0;
		adapstat.tx_frames                       =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.tx_frames );
		adapstat.tx_words                        =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.tx_words );
		adapstat.rx_frames                       =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.rx_frames );
		adapstat.rx_words                        =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.rx_words );
		adapstat.lip_count                       =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.lip_count );
		adapstat.nos_count                       =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.nos_count );
		adapstat.error_frames                    =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.error_frames );
		adapstat.dumped_frames                   =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.dumped_framed );
		adapstat.link_failure_count              =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.link_failure_count );
		adapstat.loss_of_sync_count              =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.loss_of_sync_count );
		adapstat.loss_of_signal_count            =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.loss_of_signal_count );
		adapstat.primitive_seq_protocol_err_count=(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.primitive_seq_protocol_err_count );
		adapstat.invalid_tx_word_count           =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.invalid_tx_word_count );
		adapstat.invalid_crc_count               =(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.invalid_crc_count );
	}
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	unlock_mailbox( ap );	/* Unlock mailbox */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */

	/* Write back data to hfc_ioctl_adap_stat structure */
	if( COPYOUT( ( char * )&adapstat, ( char * )arg, sizeof( struct hfc_ioctl_adap_stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );

	return rtn;
}

/* FCLNX-GPL-261 */
/*
 * Function:    hfc_adap_stat_new
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() (SCIOLCHBA)
 */

int hfc_adap_stat_new( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_adap_stat adapstat;


	HFC_ENTRY("hfc_adap_stat_new");

	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_ioctl_adap_stat structure */
	if( COPYIN( (char *)arg, (char *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT_NEW, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );


	adapstat.adapter_status = 0x00;   /* Set zero */

	adapstat.seconds_since_last_reset = 0;
	adapstat.tx_frames                       = (uint64_t)ap->tx_frames;
	adapstat.tx_words                        = (uint64_t)ap->tx_words;
	adapstat.rx_frames                       = (uint64_t)ap->rx_frames;
	adapstat.rx_words                        = (uint64_t)ap->rx_words;
	adapstat.lip_count                       = (uint64_t)hfc_read_stat_cca(ap, 0x400);
	adapstat.nos_count                       = (uint64_t)hfc_read_stat_cca(ap, 0x408);
	adapstat.error_frames                    = (uint64_t)hfc_read_stat_cca(ap, 0x410);
	adapstat.dumped_frames                   = (uint64_t)hfc_read_stat_cca(ap, 0x418);
	adapstat.link_failure_count              = (uint64_t)hfc_read_stat_cca(ap, 0x420);
	adapstat.loss_of_sync_count              = (uint64_t)hfc_read_stat_cca(ap, 0x428);
	adapstat.loss_of_signal_count            = (uint64_t)hfc_read_stat_cca(ap, 0x430);
	adapstat.primitive_seq_protocol_err_count= (uint64_t)hfc_read_stat_cca(ap, 0x438);
	adapstat.invalid_tx_word_count           = (uint64_t)hfc_read_stat_cca(ap, 0x440);
	adapstat.invalid_crc_count               = (uint64_t)hfc_read_stat_cca(ap, 0x448);

	/* Write back data to hfc_ioctl_adap_stat structure */
	if( COPYOUT( ( char * )&adapstat, ( char * )arg, sizeof( struct hfc_ioctl_adap_stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT_NEW, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );

	return rtn;
}
/* FCLNX-GPL-261 */

/*
 * Function:    hfc_target_mapping
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */

int hfc_target_mapping( struct adap_info *ap, void *arg ) {
	int    rtn = 0;
	struct hfc_ioctl_tgt_map  tgtmap;
	struct hfc_ioctl_tgt_map *tgtmap_ptr;
	struct target_info       *target;
	uint   i,n,m = 0;
	ulong  flags = 0;


	HFC_ENTRY("hfc_target_mapping");

	/* Copy data to an internal data area as hfc_ioctl_tgt_map structure */
	if( COPYIN( (char *)arg, (char *)&tgtmap, sizeof(struct hfc_ioctl_tgt_map) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_TGTMAP, (uchar *)&tgtmap, sizeof(struct hfc_ioctl_tgt_map) );

	n = tgtmap.hedder[0].number_of_entries;
	if( ( tgtmap_ptr = hfc_kmalloc(ap, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry), GFP_ATOMIC) )==NULL ) {
		/* Error if unable to allocate target map */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x02 );
		return ENOMEM;
	}
	memset( tgtmap_ptr, 0, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) );
	tgtmap_ptr->hedder[0].version           = tgtmap.hedder[0].version;
	tgtmap_ptr->hedder[0].flags             = tgtmap.hedder[0].flags;
	tgtmap_ptr->hedder[0].number_of_entries = tgtmap.hedder[0].number_of_entries;


	HFC_ADAPLOCK_IRQSAVE(flags);
	for( i=0; i<MAX_TARGET_PROBE; i++ ) {
		target = ap -> target_arg[i];
		if( target != NULL ) {                                                        //FCLNX-0150
			if( test_bit(HFC_DEVFLG_VALID,(ulong *)&target->flags) ) {                //FCLNX-0150
				if( m < n ){                                                          //FCLNX-0150
					tgtmap_ptr -> entry[m].scsiid.target_id    = target -> target_id; //FCLNX-0150
					tgtmap_ptr -> entry[m].fcpid.node_wwn      = target -> node_name; //FCLNX-0150
					tgtmap_ptr -> entry[m].fcpid.port_wwn      = target -> ww_name;   //FCLNX-0150
					tgtmap_ptr -> entry[m].scsiid.target_valid = HFC_TGTWWN_VALID;    //FCLNX-0150
					if( test_bit(HFC_WWN_VALID,(ulong *)&target->flags)   //I become it when I inspect HFC_WWN_VALID whether "SCSI ID is effective" //FCLNX-0150
							&& (test_bit(HFC_ONLINE,     (ulong *)&ap->status))           //FCLNX-0151
							&& (!test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ))         //FCLNX-0151
							&& (!test_bit(HFC_SCN_WLINKUP,(ulong *)&target->status)) ) {  //FCLNX-0151
						tgtmap_ptr -> entry[m].scsiid.target_valid |= HFC_SCSIID_VALID; //add a bit     //FCLNX-0150
						tgtmap_ptr -> entry[m].fcpid.scsi_id       = (uint)target -> scsi_id;           //FCLNX-0150
					}                                                                 //FCLNX-0150
				}                                                                     //FCLNX-0150
				m++;
			}                                                                         //FCLNX-0150
		}                                                                             //FCLNX-0150
	}
	tgtmap_ptr->hedder[0].number_of_target = m;
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	/* Write back data to hfc_ioctl_tgt_map structure */
	if( COPYOUT( ( char * )tgtmap_ptr, ( char * )arg, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x03 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_TGTMAP, (uchar *)tgtmap_ptr, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) );
	hfc_kfree(ap, tgtmap_ptr );

	return rtn;
}


/*
 * Function:    hfc_fcp_binding
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */


int hfc_fcp_binding( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_fcp_binding  fcpbind;
	struct hfc_ioctl_fcp_binding *fcpbind_ptr;
	uint   n = 0;	/* FCLNX-GPL-0447 */


	HFC_ENTRY("hfc_fcp_binding");

	if( !test_bit(HFC_ONLINE, (ulong *)&ap->status) ) {
		/* Adapter is not online */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PBIND, 0x01 );
		return EIO;
	}
	/* Copy data to an internal data area as hfc_ioctl_fcp_binding structure */
	if( COPYIN( (char *)arg, (char *)&fcpbind, sizeof(struct hfc_ioctl_fcp_binding) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PBIND, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_PBIND, (uchar *)&fcpbind, sizeof(struct hfc_ioctl_fcp_binding) );

	n = fcpbind.hedder[0].number_of_entries;
	if( ( fcpbind_ptr = hfc_kmalloc(ap, sizeof(struct hfc_fcpbind_hedder)+n*sizeof(struct hfc_fcpbind_entry), GFP_ATOMIC) )==NULL ) {
		/* Error if unable to allocate fcp binding area */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PBIND, 0x02 );
		return ENOMEM;
	}
	memset( fcpbind_ptr, 0, sizeof(struct hfc_fcpbind_hedder)+n*sizeof(struct hfc_fcpbind_entry) );
	fcpbind_ptr->hedder[0].version           = fcpbind.hedder[0].version;
	fcpbind_ptr->hedder[0].flags             = fcpbind.hedder[0].flags;
	fcpbind_ptr->hedder[0].number_of_entries = fcpbind.hedder[0].number_of_entries;

	/* Write back data to hfc_ioctl_fcp_binding structure */
	if( COPYOUT( ( char * )fcpbind_ptr, ( char * )arg, sizeof(struct hfc_fcpbind_hedder)+n*sizeof(struct hfc_fcpbind_entry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PBIND, 0x03 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_PBIND, (uchar *)fcpbind_ptr, sizeof(struct hfc_fcpbind_hedder)+n*sizeof(struct hfc_fcpbind_entry) );
	hfc_kfree(ap, fcpbind_ptr );

	return rtn;
}


/*
 * Function:    hfc_support (Unsupported function)
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */

int hfc_support( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_spt spt;


	HFC_ENTRY("hfc_support");

	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_ioctl_spt structure */
	if( COPYIN( (char *)arg, (char *)&spt, sizeof(struct hfc_ioctl_spt) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SPT, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_SPT, (uchar *)&spt, sizeof(struct hfc_ioctl_spt) );


	/* Write back data to hfc_ioctl_sptstructure */
	if( COPYOUT( ( char * )&spt, ( char * )arg, sizeof( struct hfc_ioctl_spt ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SPT, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_SPT, (uchar *)&spt, sizeof(struct hfc_ioctl_spt) );

	return rtn;
}


/*
 * Function:    hfc_pron_info
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */

int hfc_procinfo( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_proc_info *procinfo;										/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	struct hfc_vpd			*vpd_info	= NULL;											/* FCLNX-0404 */
	struct hfc_vpd_five		*vpdf_info	= NULL;										/* FCLNX-0404 */
	struct hfc_vpd_five_ex	*vpdex_info	= NULL;

	HFC_ENTRY("hfc_proc_info");

	/* host was not founded */
	if( ap->hosts==NULL ){
		return (-EINVAL);
	}

	procinfo = (struct hfc_ioctl_proc_info *)hfc_kmalloc(ap, sizeof(struct hfc_ioctl_proc_info), GFP_ATOMIC);
																				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	if (procinfo == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x09 );
		return ENOMEM;
	}

	memset(procinfo, 0, sizeof(struct hfc_ioctl_proc_info));					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	/* Copy data to an internal data area as hfc_ioctl_proc_info  structure */
	if( COPYIN( (char *)arg, (char *)procinfo, sizeof(struct hfc_ioctl_proc_info) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x00 );
		hfc_kfree(ap, procinfo);														/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	STRUCTDUMP( DMP_PRINFO, (uchar *)procinfo, sizeof(struct hfc_ioctl_proc_info) );

	/* Copy information in adap_info to hfc_ioctl_proc_info structure  */
	procinfo->version = 0x00;
	procinfo->flags   = 0x00;

	procinfo->dev_major = ap -> dev_major;
	procinfo->dev_minor = ap -> dev_minor;
	procinfo->instance  = ap -> instance;
	procinfo->host_no   = ap -> hosts -> host_no;
	procinfo->unique_id = ap -> hosts -> unique_id;
	procinfo->vender_id = ap -> pkg.vender_id;
	procinfo->device_id = ap -> pkg.device_id;

	procinfo->ww_name   = ap -> ww_name;
	procinfo->node_name = ap -> node_name;
	procinfo->scsi_id   = ap -> scsi_id;
	strncpy(procinfo->model_name, ap->mp_adap_info->model_name, 16);				/* FCLNX-0329 */
	strncpy(procinfo->driver_ver,hfc_manage_info.package_ver,16);				/* FCLNX-0329 */
	memcpy(procinfo->adap_id, ap->mp_adap_info->adap_id, 16);					/* FCLNX-0329 */
	procinfo->firmware_ver = hfc_get_sysrev(ap);									/* FCLNX-GPL-112 */

	memset(procinfo->parts_number,0,16);											/* FCLNX-0404 */
	if(ap->pkg.type == HFC_PKTYPE_FPP) {										/* FCLNX-0404 */
		vpd_info = (struct hfc_vpd *)ap->mp_adap_info->vpd_buf;					/* FCLNX-0404 */
		memcpy(procinfo->parts_number,vpd_info->pn_value,VPD_PN_LEN);			/* FCLNX-0404 */
		procinfo->ec_level = vpd_info->ec_value[0];								/* FCLNX-0404 */
	}																			/* FCLNX-0404 */
	else if(ap->pkg.type == HFC_PKTYPE_FIVE) {									/* FCLNX-0404 */
		vpdf_info = (struct hfc_vpd_five *)ap->mp_adap_info->vpd_buf;			/* FCLNX-0404 */
		memcpy(procinfo->parts_number,vpdf_info->pn_value,VPD_PN_LEN);			/* FCLNX-0404 */
		procinfo->ec_level = vpdf_info->ec_level;								/* FCLNX-0404 */
	}																			/* FCLNX-0404 */
	else { /* FIVE-EX */
		vpdex_info = (struct hfc_vpd_five_ex *)ap->mp_adap_info->vpd_buf;
		memcpy(procinfo->parts_number,vpdex_info->pn_value,VPD_PN_LEN);
		procinfo->ec_level = vpdex_info->ec_level;
	}
	procinfo->pkgtype         = ap->pkg.type;									/* FCLNX-0404 */
	procinfo->pkgcode         = ap->pkg.code;									/* FCLNX-0404 */
	procinfo->bus_dev_func[0] = ap->pci_cfginf->bus->number;						/* FCLNX-0404 */
	procinfo->bus_dev_func[1] = PCI_SLOT(ap->pci_cfginf->devfn);					/* FCLNX-0404 */
	procinfo->bus_dev_func[2] = PCI_FUNC(ap->pci_cfginf->devfn);					/* FCLNX-0404 */
	procinfo->port_no         = ap->port_no;										/* FCLNX-0404 */
	memcpy(procinfo->opt_vendor_name,ap->opt_vendor_name,32);					/* FCLNX-0404 */
	memcpy(procinfo->opt_parts_number,ap->opt_parts_number,32);					/* FCLNX-0404 */
	memcpy(procinfo->opt_serial_number,ap->opt_serial_number,32);				/* FCLNX-0404 */

	switch( ap->connect_type ){
	case HFC_SWITCH : 
		if( test_bit(HFC_ONLINE, (ulong *)&ap->status) ) procinfo->connection_type = HFC_CON_TYP_PtoPF;
		break;
	case HFC_PT2PT  :
		if( test_bit(HFC_ONLINE, (ulong *)&ap->status) ) procinfo->connection_type = HFC_CON_TYP_PtoP;
		break;
	case HFC_AL     :
		if( test_bit(HFC_ONLINE, (ulong *)&ap->status) ){
			if( ap->scsi_id & 0x00ffff00){
				procinfo->connection_type = HFC_CON_TYP_FcAlF;
			}
			else{
				procinfo->connection_type = HFC_CON_TYP_FcAl;
			}
		}
		break;
	default         :
		procinfo->connection_type = HFC_CON_TYP_NONE;
	}
	procinfo->max_data_rate = ap->max_data_rate;

	procinfo->login_delay       = ap->login_wait;								/* FCLNX-0404 */
	procinfo->spinup_delay      = ap->spinup_delay;								/* FCLNX-0404 */
	procinfo->automap           = ap->automap;									/* FCLNX-0404 */
	procinfo->pref_alpa         = ap->pref_alpa;									/* FCLNX-0404 */
	procinfo->xob_max           = ap->xob_max;									/* FCLNX-0404 */
	procinfo->xrb_max           = ap->xrb_max;									/* FCLNX-0404 */
	procinfo->slog_max          = ap->slog_max;									/* FCLNX-0404 */
	procinfo->dma_max           = ap->dma_max;									/* FCLNX-0404 */
	procinfo->queue_depth       = ap->queue_depth;								/* FCLNX-0404 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		procinfo->linkup_tmo        = ap->dev_loss_tmo;
	else
		procinfo->linkup_tmo        = ap->linkup_tmo;								/* FCLNX-0404 */
#else
	procinfo->linkup_tmo        = ap->linkup_tmo;								/* FCLNX-0404 */
#endif
	procinfo->scsi_reset_delay  = ap->scsi_reset_delay;							/* FCLNX-0404 */
	procinfo->target_reset_tmo  = ap->target_reset_tmo;							/* FCLNX-0404 */
	procinfo->abort_tmo         = ap->abort_tmo;									/* FCLNX-0404 */
	procinfo->max_target        = ap->max_target;								/* FCLNX-0404 */
	procinfo->trc_max           = ap->trc_max;									/* FCLNX-0404 */
	procinfo->max_mck_cnt       = ap->max_mck_cnt;								/* FCLNX-0404 */
	procinfo->wmsg              = ap->wmsg;										/* FCLNX-0404 */
	procinfo->linkup2_tmo       = ap->linkup2_tmo;								/* FCLNX-0404 */
	procinfo->pkt_num           = ap->pkt_num;									/* FCLNX-0404 */
	procinfo->can_queue         = ap->can_queue;									/* FCLNX-0404 */
	procinfo->sg_tblsize        = ap->sg_tblsize;								/* FCLNX-0404 */
	procinfo->cmnd_num          = ap->cmnd_num;									/* FCLNX-0404 */
	procinfo->minus_tout        = ap->minus_tout;								/* FCLNX-0404 */
	procinfo->scsi_allowed      = ap->scsi_allowed;								/* FCLNX-0404 */
	procinfo->cmd_per_lun       = ap->cmd_per_lun;								/* FCLNX-0404 */
	procinfo->max_sectors       = ap->max_sectors;								/* FCLNX-0404 */
	procinfo->scsi_timeout_fail = ap->scsi_timeout_fail;							/* FCLNX-0404 */
	procinfo->mlpf_mode     	   = ap->mlpf_mode;					                        /* FCLNX-0488 */
	procinfo->highconf_opt	   = ap->manage_info->hfcplus_enable;    /* FCLNX-0488 */

	procinfo->port_status = hfc_get_adap_status(ap);	/* FCLNX-GPL-428 */
	
	procinfo->abort_t_restrain 	= ap->abort_t_restrain;		/*FCLNX-0506*/
	procinfo->msi_flag			= ap->msi_flag;				/* FCLNX-GPL-126 */
	procinfo->lun_reset_delay 	= ap->lun_reset_delay;		/*FCLNX-0506*/
	procinfo->target_reset_mode	= ap->enable_tgtrst;		/* Get Target Reset Mode On/Off */ /* FCLNX-0660 */

	procinfo->limit_log			= ap->limit_log;								/* FCLNX-GPL-491 */
	procinfo->filter_target		= ap->filter_target;							/* FCLNX-GPL-491 */
	procinfo->hg_stats_disable	= ap->hg_stats_disable;							/* FCLNX-GPL-494 */
	
	/* Write back data to hfc_ioctl_proc_info structure */
	if( COPYOUT( ( char * )procinfo, ( char * )arg, sizeof( struct hfc_ioctl_proc_info ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x01 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_PRINFO, (uchar *)procinfo, sizeof(struct hfc_ioctl_proc_info) );

	hfc_kfree(ap, procinfo);											/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	return rtn;
}

/*
 * Function:    hfc_fc4stat
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */
int hfc_fc4stat( struct adap_info *ap, void *arg ) {							/* FCLNX-0404 */
	int    rtn = 0;
	struct hfc_ioctl_fc4stat fc4stat;

	HFC_ENTRY("hfc_fc4stat");

	/* Host was not found */
	if( ap->hosts==NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x00 );
		return (-EINVAL);
	}

	/* Copy data to an internal data area as hfc_ioctl_fc4stat  structure */
	if( COPYIN( (char *)arg, (char *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x01 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_FC4STAT, (uchar *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) );
	
	fc4stat.inputrequests   = ap->inputrequests;
	fc4stat.outputrequests  = ap->outputrequests;
	fc4stat.controlrequests = ap->controlrequests;
	fc4stat.inputmegabytes  = ap->inputbytes >> 20;
	fc4stat.outputmegabytes = ap->outputbytes >> 20;

	/* Write back data to hfc_ioctl_fc4stat structure */	
	if( COPYOUT( ( char * )&fc4stat, ( char * )arg, sizeof( struct hfc_ioctl_fc4stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_FC4STAT, (uchar *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) );
	return rtn;
}																				/* FCLNX-0404 */

/*
 * Function:    hfc_adapter_enable
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */
																/* hotplug */
int hfc_adapter_enable( struct adap_info *ap, void *arg ) {
	int		rtn = 0;
#if 0															/* FCLNX-GPL-204 */
	ulong  flags = 0;
	struct hfc_ioctl_adp_enable	adpparm;

	HFC_ENTRY("hfc_adapter_enable");

	/* Copy data to an internal data area as hfc_ioctl_adp_enable  structure */
	if ( COPYIN( (char *)arg, (char *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_ENABLE, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_ADPE, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );

	HFC_ADAPLOCK_IRQSAVE(flags);
	if( test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ){
		HFC_DBGPRT( " ** hfcldd : hfc_adapter_enable - HFC_ATTACH=1.\n"); 
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return (EIO);
	}
	
	rtn = hfc_probe_internal(ap);
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	if (rtn) {
		return (rtn);
	}
	
	hfc_initialize(ap, 0);
	
	/* Write back data to hfc_ioctl_adp_enable structure */
	if( COPYOUT( (char *)&adpparm, (char *)arg, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_ENABLE, 0x02 );
		rtn = EFAULT;
	}
	
	STRUCTDUMP( DMP_ADPE, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );
#endif															/* FCLNX-GPL-204 */
	rtn = EIO;													/* FCLNX-GPL-204 */

	return rtn;
}	


/*
 * Function:    hfc_adapter_disable
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */
																	/* hotplug */
int hfc_adapter_disable( struct adap_info *ap, void *arg ) {
	int		rtn = 0;													/* FCLNX-0459 */
#if 0																/* FCLNX-GPL-204 */
	int		wait = 0;
	ulong  flags = 0;
	struct hfc_ioctl_adp_enable	adpparm;

	HFC_ENTRY("hfc_adapter_disable");

	/* Copy data to an internal data area as hfc_ioctl_adp_enable structure */
	if ( COPYIN( (char *)arg, (char *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_DISABLE, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_ADPD, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );

	HFC_ADAPLOCK_IRQSAVE(flags);

	if( test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ){
		do {																	/* FCLNX-0459 */
			wait = hfc_remove_internal(ap);
			
			/* sleep for a while */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			msleep(1);
			HFC_ADAPLOCK_IRQSAVE(flags);
		} while (wait);															/* FCLNX-0459 */
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	/* Write back data to hfc_ioctl_adp_enable structure */
	if( COPYOUT( (char *)&adpparm, (char *)arg, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_DISABLE, 0x02 );
		rtn = EFAULT;
	}
	
	STRUCTDUMP( DMP_ADPD, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );
#endif															/* FCLNX-GPL-204 */
	rtn = EIO;													/* FCLNX-GPL-204 */

	return rtn;
}	/* end of hfc_adapter_disable */


/*
 * Function:    hfc_scsi_scan
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */
int hfc_scsi_scan( struct adap_info *ap, void *arg ) {
	int		rtn = 0;
	struct hfc_ioctl_adp_enable	adpparm;

	HFC_ENTRY("hfc_scsi_scan");

	/* Copy data to an internal data area as hfc_ioctl_adp_enable structure */
	if ( COPYIN( (char *)arg, (char *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_DKSCAN, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_DKSCAN, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );

	rtn = EINVAL;																/* FCLNX-GPL-204 */
#if 0																			/* FCLNX-GPL-204 */

	{
		ulong	flags = 0;
		struct Scsi_Host *shost,*hsdldd_shost;									/* FCLNX-0429 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		struct device *parent;
#endif
		int		tid,lun;														/* FCLNX-0316 */

		HFC_ADAPLOCK_IRQSAVE(flags);
		shost = ap->hosts;
		hsdldd_shost = ap->hfchsd_host;											/* FCLNX-0429 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		parent = &shost->shost_gendev;
#endif
		HFC_ADAPUNLOCK_IRQRESTORE(flags);

		if (hfc_manage_info.npubp->hfc_mp_scsi_scan_target) {					/* FCLNX-0429 */
			for ( tid=0; tid<hsdldd_shost->max_id; tid++ ) {					/* FCLNX-0429 */
				for( lun=0; lun<hsdldd_shost->max_lun; lun++ ) {				/* FCLNX-0429 */
					hfc_manage_info.npubp->hfc_mp_scsi_scan_target(hsdldd_shost, 0, tid, lun, 1);
				}																/* FCLNX-0429 */
			}																	/* FCLNX-0429 */
		}
		
		for ( tid=0; tid<shost->max_id; tid++ ) {								/* FCLNX-0316 */
			for( lun=0; lun<shost->max_lun; lun++ ) {							/* FCLNX-0316 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
				scsi_scan_target(parent, 0, tid, lun, 1);
#else
				scsi_scan_target(shost, 0, tid, lun, 1);						/* FCLNX-0316 */
#endif
			}																	/* FCLNX-0316 */
		}																		/* FCLNX-0316 */
	}

#endif																			/* FCLNX-GPL-204 */

	/* Write back data to hfc_ioctl_adp_enable structure */
	if( COPYOUT( (char *)&adpparm, (char *)arg, sizeof(struct hfc_ioctl_adp_enable) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_DKSCAN, 0x02 );
		rtn = EFAULT;
	}
	
	STRUCTDUMP( DMP_DKSCAN, (uchar *)&adpparm, sizeof(struct hfc_ioctl_adp_enable) );

	return rtn;
}	


/*
 * Function:    hfc_mp_target_map
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl() 
 */
int hfc_mp_target_map( struct adap_info *ap, void *arg ) {
	int    rtn =0;
	struct hfc_mp_target	tgtmap;
	struct hfc_mp_target	*tgtmapp;
	struct target_info		*target;
	uint   i,n,m = 0;
	ulong  flags = 0;

	HFC_ENTRY("hfc_mp_target_map");

	if( COPYIN( (char *)arg, (char *)&tgtmap, sizeof(struct hfc_mp_target) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_MP_TARGET, (uchar *)&tgtmap, sizeof(struct hfc_mp_target) );
	
	n = tgtmap.header[0].entry_count;
	if( ( tgtmapp = hfc_kmalloc(ap, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry), GFP_ATOMIC) )==NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x01 );
		return ENOMEM;
	}
	
	memset( tgtmapp, 0, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) );
	tgtmapp->header[0].entry_count = tgtmap.header[0].entry_count;


	HFC_ADAPLOCK_IRQSAVE(flags);
	for( i=0; i<MAX_TARGET_PROBE; i++ ) {
		target = ap->target_arg[i];
		if( target != NULL ) {
			if( test_bit(HFC_DEVFLG_VALID,(ulong *)&target->flags) ) {
				if( m < n ){
					set_bit(HFC_MP_WWN_VALID,(ulong *)&tgtmapp->entry[m].flags);
					tgtmapp->entry[m].nodename    = target->node_name;
					tgtmapp->entry[m].portname    = target->ww_name;
					tgtmapp->entry[m].attribute   = target->attribute;
					tgtmapp->entry[m].groupid     = target->group_id;
					tgtmapp->entry[m].targetid    = target->target_id;
					tgtmapp->entry[m].pathid      = target->path_id;
					tgtmapp->entry[m].instance    = ap->instance;
					if( test_bit(HFC_WWN_VALID,(ulong *)&target->flags)
						&& (test_bit(HFC_ONLINE,      (ulong *)&ap->status))
						&& (!test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ))
						&& (!test_bit(HFC_SCN_WLINKUP,(ulong *)&target->status)) ) {
						set_bit(HFC_MP_LINKUP,(ulong *)&tgtmapp->entry[m].flags);
					}
					if( test_bit(HFC_TARGET_GHOST,(ulong *)&target->flags) ) {			/* FCLNX-0405 */
						set_bit(HFC_MP_TGT_GHOST,(ulong *)&tgtmapp->entry[m].flags);	/* FCLNX-0405 */
					}
				}
				m++;
			}
		}
	}
	tgtmapp->header[0].target_count = m;
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	if( COPYOUT( ( char * )tgtmapp, ( char * )arg, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_MP_TARGET, (uchar *)tgtmapp, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) );
	hfc_kfree(ap, tgtmapp );

	HFC_EXIT("hfc_mp_target_map");
	return rtn;
}

/*
 * Function:    hfc_read_apparam
 *
 * Purpose:
 *
 * Arguments:
 *  ap        - Pointer to adap_info
 *  arg       - Pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl()
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
int hfc_read_apparam( struct adap_info *ap, void *arg) {

    HFC_ENTRY("hfc_read_apparam");                 //FCLNX-0488

    HFC_EXIT("hfc_read_apparam");                   //FCLNX-0488
    return (-EINVAL);
}

#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */

int hfc_read_apparam( struct adap_info *ap, void *arg) {
	int value=0;
	int rtn=0;
	char buf[32];
	int linkup_tmo;
	int linkup2_tmo=HFC_LINKUP2_TO;	/* FCLNX-0588 */
	int max_mck_cnt=HFC_MCKERR_CNT;	/* FCLNX-0588 */
	int scsi_allowed=HFC_SCSI_ALLOWED;	/* FCLNX-0588 */
	int abort_tmo=HFC_ABORT_ACA_TO;		/* FCLNX-0588 */
	int target_reset_tmo=HFC_TARGET_RST_TO;	/* FCLNX-0588 */
	int scsi_reset_delay=HFC_DELAY_TO;	/* FCLNX-0588 */
	uint lun_reset_delay=0;
	uchar enable_tgtrst=HFC_DEFAULT_TARGET_RESET;	/* FCLNX-0588 */
	uchar abort_t_restrain=0;

	HFC_ENTRY("hfc_read_apparam");				   //FCLNX-0488

	if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) /* FCLNX-GPL-FX-472 */
		linkup_tmo=HFC_PCM_LINKUP_TO;
	else
		linkup_tmo=HFC_LINKUP_TO;

	if(ap->defparam) return(0);

	/* Read adapter binding information for hotplug */ /* FCLNX-GPL-193 */
	hfc_get_adapter_bindings(); 


 	/* Timer value for linkup wait */
	if (hfc_param_search("hfc_link_down", &value)){
		if(hfc_chk_conf_val(0,60,value)){
			linkup_tmo = value;											/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_link_down",ap->instance);							/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){										/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			linkup_tmo = value;											/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	/* Timer value for linkup wait	(MCK) */
	if (hfc_param_search("hfc_link_down2", &value)){
		if(hfc_chk_conf_val(0,60,value)){
			linkup2_tmo = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;												/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_link_down2",ap->instance);							/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){										/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			linkup2_tmo = value;											/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	/* Max MCKINT Error */
	if (hfc_param_search("hfc_mck_retry", &value)){ 						/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,10,value)){
			max_mck_cnt = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_mck_retry",ap->instance);							/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){										/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,10,value)){
			max_mck_cnt = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	/* scsi allowed */
	if (hfc_param_search("hfc_scsi_allowed", &value)){ 						/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,30,value)){
			scsi_allowed = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_scsi_allowed",ap->instance);							/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){									 	/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,30,value)){
			scsi_allowed = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	
	/* abort timeout */
	if (hfc_param_search("hfc_abort_timeout", &value)){ 					/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			abort_tmo = value;											/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;												/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_abort_timeout",ap->instance);						/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){										/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			abort_tmo = value;											/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;												/*FCLNX-0506*/
		}
	}
	
	/* target reset timeout */
	if (hfc_param_search("hfc_reset_timeout", &value)){						/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			target_reset_tmo = value;
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_reset_timeout",ap->instance);						/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){ 									/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			target_reset_tmo = value;									/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	
	/* Supress SCSI after linkup */
	if (hfc_param_search("hfc_reset_delay", &value)){ 						/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			scsi_reset_delay = value;
		}
		else{
			rtn = EINVAL;
			goto invalid_status;										/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_reset_delay",ap->instance);						/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){									/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			scsi_reset_delay = value;									/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;										/*FCLNX-0506*/
		}
	}
	
	/* Supress SCSI after task management command*/
	if (hfc_param_search("hfc_lun_reset_delay", &value)){				/*FCLNX-0506*/	/* FCLNX-GPL-038 */
		if(hfc_chk_conf_val(0,60,value)){
			lun_reset_delay = value;									/* FCLNX-GPL-038 */
		}
		else{
			rtn = EINVAL;
			goto invalid_status;										/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_lun_reset_delay",ap->instance);					/*FCLNX-0506*/	/* FCLNX-GPL-038 */
	if (hfc_param_search(buf, &value)){									/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,60,value)){
			lun_reset_delay = value;									/*FCLNX-0506*/	/* FCLNX-GPL-038 */
		}
		else{
			rtn = EINVAL;
			goto invalid_status;										/*FCLNX-0506*/
		}
	}
	
	/* Target Reset Effective/invalidity */
	if (hfc_param_search("hfc_enable_tgtrst", &value)){						/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,1,value)){
			enable_tgtrst = value;
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_enable_tgtrst",ap->instance);						/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){ 									/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,1,value)){
			enable_tgtrst = value;										/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	
	
	/* Abort Task Set restrain */
	if (hfc_param_search("hfc_abort_t_restrain", &value)){					/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,1,value)){
			abort_t_restrain = value;
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	sprintf(buf,"hfc%d_abort_t_restrain",ap->instance);						/*FCLNX-0506*/
	if (hfc_param_search(buf, &value)){										/*FCLNX-0506*/
		if(hfc_chk_conf_val(0,1,value)){
			abort_t_restrain = value;									/*FCLNX-0506*/
		}
		else{
			rtn = EINVAL;
			goto invalid_status;													/*FCLNX-0506*/
		}
	}
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		if(hfc_manage_info.npubp->hfc_read_retry_cnt(ap)){
			rtn = EINVAL;
        	goto invalid_status;                                /*FCLNX-0506*/	
		}
	}

	if(hfc_manage_info.hfcldd_mp_mod) {		 /*FCLNX-0608*/
		if(hfc_manage_info.npubp->hfc_read_isolparam(ap)){		/*FCLNX-0506*/ 
	    		rtn = EINVAL;
	    		goto invalid_status;						/*FCLNX-0506*/
		}
	}
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		ap->dev_loss_tmo		= linkup_tmo;					/*FCLNX-0506*/
	} else {
		ap->linkup_tmo			= linkup_tmo;					/*FCLNX-0506*/
		ap->linkup2_tmo			= linkup2_tmo;					/*FCLNX-0506*/		
	}
#else
	ap->linkup_tmo			= linkup_tmo;					/*FCLNX-0506*/
	ap->linkup2_tmo			= linkup2_tmo;					/*FCLNX-0506*/
#endif
	ap->max_mck_cnt			= max_mck_cnt;					/*FCLNX-0506*/
	ap->scsi_allowed		= scsi_allowed;					/*FCLNX-0506*/
	ap->abort_tmo			= abort_tmo;					/*FCLNX-0506*/
	ap->target_reset_tmo	= target_reset_tmo;				/*FCLNX-0506*/
	ap->scsi_reset_delay	= scsi_reset_delay;				/*FCLNX-0506*/
	ap->lun_reset_delay		= lun_reset_delay;				/*FCLNX-0506*/	/* FCLNX-GPL-038 */
	ap->enable_tgtrst		= enable_tgtrst;				/*FCLNX-0506*/
	ap->abort_t_restrain	= abort_t_restrain;				/*FCLNX-0506*/
	
invalid_status:
	HFC_EXIT("hfc_read_apparam");                  /* FCLNX-0488 */
	return (rtn); 				 //FCLNX-0488

}

#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */


/*
 * Function:    hfc_isolate_operation
 *
 * Purpose:
 *
 * Arguments:
 *  ap        - Pointer to adap_info
 *  arg       - Pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_ioctl()
 */
int hfc_isolate_operation( struct adap_info *ap, void *arg ) {	/* FCLNX-GPL-147 */
	int rtn=0,i;
	struct hfc_isol_info *isolinfo;
	unsigned long			flags = 0;
	struct mp_adap_info		*mpap;

	HFC_ENTRY("hfc_isolate_operation"); 								/* FCLNX-0488 */
	
	isolinfo = (struct hfc_isol_info *)hfc_kmalloc(ap, sizeof(struct hfc_isol_info), GFP_ATOMIC);
																		/* FCLNX_GPL-0151 */
	if (isolinfo == NULL) {												/* FCLNX_GPL-0151 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n",		/* FCLNX_GPL-0151 */
			 HFC_TRC_IOCTL_HFC_HBA_ISOLATION, 0x09 );					/* FCLNX_GPL-0151 */
		return (ENOMEM);												/* FCLNX_GPL-0151 */
	}																	/* FCLNX_GPL-0151 */
																		/* FCLNX_GPL-0151 */
	memset(isolinfo, 0, sizeof(struct hfc_isol_info));					/* FCLNX_GPL-0151 */

	if( COPYIN( (char *)arg, (char *)isolinfo, sizeof(struct hfc_isol_info) ) != 0 ) {	/* FCLNX-0488 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_HFC_HBA_ISOLATION, 0x00 );	//FCLNX-0488
		hfc_kfree(ap, isolinfo);												/* FCLNX_GPL-0151 */
		return (EFAULT);												/* FCLNX-0488 */
	}

	if(isolinfo->version!=0) {
		hfc_kfree(ap, isolinfo);												/* FCLNX_GPL-0151 */
		return(EINVAL);													/* FCLNX-0488 */
	}

	/* FPP Not Suppot */
	if(ap->pkg.type == HFC_PKTYPE_FPP) {
		hfc_kfree(ap, isolinfo);												/* FCLNX_GPL-0151 */
		return(EINVAL);
	}

	switch(isolinfo->set_opr){
	case HFC_READ_ISOLPARAM:											/* FCLNX-0488 */

		if(hfc_manage_info.hfcldd_mp_mod) {
			rtn = hfc_manage_info.npubp->hfc_get_isolparam(ap, isolinfo);	/* FCLNX-0488 */
		}
		else {
			rtn = hfc_get_isolparam_i(ap, isolinfo, 0);					/* FCLNX_GPL-393 */
		}
		break;															/* FCLNX-0488 */

	case HFC_FORCE_ISOLATE: 											/* FCLNX-0488 */

		if ( !(HFC_MMODE_CHECK_SHARED(ap)) ){				/* FCLNX-GPL-393 */
			if(ap->pkg.port <= 1) {											/* FCLNX-0583 */
				/* Isotate Adapter */
				ap->c_err = HFC_ISOLATE_FA;					/* FCLNX-0549 */
			}
			else {
				/* Isotate Adapter port */
				if (!(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))) {
					hfc_kfree(ap, isolinfo);										/* FCLNX_GPL-0151 */
					return(EINVAL);
				}
				
				ap->c_err = HFC_ISOLATE_FP;
			}

			HFC_ADAPLOCK_IRQSAVE(flags);

			/* only DMA transfer ends */
			rtn = hfc_force_linkdown(ap, TRUE, FALSE);

			if (!rtn) {
				if(ap->pkg.port > 1) {										/* FCLNX-0583 */
					HFC_ADAPUNLOCK_IRQRESTORE(flags);
					
					/* Wait until DMA transfer ends */
					/* Wait 6s */
					for (i=0;i<6000;i++)
						msleep(1);
					
					HFC_ADAPLOCK_IRQSAVE(flags);
				}
				
				/* only SCSI I/O, Mailbox, Timer cancel */
				rtn = hfc_force_linkdown(ap, TRUE, TRUE);
			}			
		}
		else
		{
			HFC_ADAPLOCK_IRQSAVE(flags);				/* FCLNX-GPL-416 */
			rtn = hfc_mlpf_issue_fisolate(ap, HFC_ISSUE_ISOLREQ_CMD);
		}																/* FCLNX-GPL-393 */
		
		if ( hfc_manage_info.lg_target_info ) {							/* FCLNX-0625 */
			if ( ap->retry_hfcp_top )
				hfc_manage_info.npubp->hfc_retry_strategy(ap);
			
			if ( hfc_manage_info.wait_reset_mp ) {						/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_check_dev_reset_complete();	/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_check_bus_reset_complete();	/* FCLNX-0429 */
			}
		}

		HFC_ADAPUNLOCK_IRQRESTORE(flags);				/* FCLNX-GPL-416 */

		break;															/* FCLNX-0488 */

	case HFC_RECOV_ISOLATE:												/* FCLNX-0488 */
		
		if (ap->isol_force == HFC_CHKSTP_FRC_ISOL) {	/* check stop */
			hfc_kfree(ap, isolinfo);											/* FCLNX_GPL-0151 */
			return(EINVAL);
		}
		
		mpap = ap->mp_adap_info;
		rtn = 0;
		
		if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
			hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
		}														/* FCLNX-GPL-331 */
		else{
			hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
		}
		
		if ( !(HFC_MMODE_CHECK_SHARED(ap)) ){	/* FCLNX-GPL-393 */
			if ( test_bit(HFC_HWISOL, (ulong *)&mpap->status) ) { /* FCLNX-556 */
				HFC_ADAPLOCK_IRQSAVE(flags);
				rtn = hfc_force_linkdown_recovery(ap);
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
			}
			else if((test_bit(HFC_ISOL, (ulong *)&ap->status)) || (isolinfo->force_recv==1)) {
			
				/* port recovery FRAME_A is not supported */
				if ( !(ap->fw_init_p->func & HFC_FWF_PORTRCV)
				 && (!(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)))) {
					hfc_kfree(ap, isolinfo);										/* FCLNX_GPL-0151 */
					return(ENOTTY);
				}
				/* call hfc_force_linkdown_port_recovery */
				HFC_ADAPLOCK_IRQSAVE(flags);
				atomic_set(&ap->int_a_poll, 0);							/* FCLNX_0029 */
				ap->initialize = 1;
				/* Lock "mpap" */
				HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);					/* FCLNX_GPL-402 */
				rtn = hfc_force_linkdown_recovery_port(ap);
				HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);							/* FCLNX-0276 *//* FCLNX_GPL-402 */
				if(!(rtn)){	/* FCLNX_GPL-402 */
					if(!(isolinfo->immdt_cmd)){ /* FCLNX-0514 */
						HFC_ADAPUNLOCK_IRQRESTORE(flags);
						hfc_sleep_on(&ap->init_event, &ap->int_a_poll );								/* FCLNX-0269 */
						HFC_ADAPLOCK_IRQSAVE(flags);
					}
				}
				atomic_set(&ap->int_a_poll, 0);							/* FCLNX_0029 */
				ap->initialize = 0;		
				ap->no_target = 0;										/* FCLNX-GPL-570 */
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
				
				if ( hfc_manage_info.hfcldd_mp_mod ) {
					hfc_manage_info.npubp->hfc_mp_scan_dev(ap);			/* FC-GW 0331 */
					hfc_manage_info.npubp->hfc_make_lgpath();
				}	/* FCLNX_GPL-402 */
			}
		}
		else {
			HFC_ADAPLOCK_IRQSAVE(flags);								/* FCLNX-GPL-416 */
			atomic_set(&ap->int_a_poll, 0);							/* FCLNX_0029 *//* FCLNX-GPL-521 */
			ap->initialize = 1;											/* FCLNX-GPL-521 */
//			if(test_bit(HFC_ISOL, (ulong *)&ap->status))				/* FCLNX-GPL-414 */
			rtn = hfc_mlpf_issue_recov_isolate(ap);
			if(!(rtn)){	/* FCLNX_GPL-402 */							/* FCLNX-GPL-521 */
				if(!(isolinfo->immdt_cmd)){ /* FCLNX-0514 */
					HFC_ADAPUNLOCK_IRQRESTORE(flags);
					hfc_sleep_on(&ap->init_event, &ap->int_a_poll );								/* FCLNX-0269 */
					HFC_ADAPLOCK_IRQSAVE(flags);
				}
			}														/* FCLNX-GPL-521 */
			atomic_set(&ap->int_a_poll, 0);							/* FCLNX_0029 *//* FCLNX-GPL-521 */
			ap->initialize = 0;										/* FCLNX-GPL-521 */
			ap->no_target = 0;										/* FCLNX-GPL-570 */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);							/* FCLNX-GPL-416 */
			if ( hfc_manage_info.hfcldd_mp_mod ) {					/* FCLNX-GPL-521 */
				hfc_manage_info.npubp->hfc_mp_scan_dev(ap);			/* FC-GW 0331 */
				hfc_manage_info.npubp->hfc_make_lgpath();
			}	/* FCLNX_GPL-402 */									/* FCLNX-GPL-521 */
		}

		break;															/* FCLNX-0488 */

	case HFC_STOP_ISOLATE:												/* FCLNX-GPL-349 */
		HFC_ADAPLOCK_IRQSAVE(flags);
		if(hfc_check_hba_isolation(ap)){								/* FCLNX-GPL-414 */
			ap->hba_isolation = HFC_ISOL_STOP;
		}else{
			rtn = EINVAL;
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		break;
	
	case HFC_START_ISOLATE:												/* FCLNX-GPL-349 */
		HFC_ADAPLOCK_IRQSAVE(flags);									/* FCLNX-GPL-414 */
		hfc_start_isolate(ap);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		break;															/* FCLNX-GPL-349 */


	default:
		hfc_kfree(ap, isolinfo);												/* FCLNX_GPL-0151 */
		return(EINVAL);													/* FCLNX-0488 */
	}

	if( COPYOUT( (char *)isolinfo, (char *)arg, sizeof(struct hfc_isol_info) ) != 0 ) {	/* FCLNX-0488 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_HFC_HBA_ISOLATION, 0x02 );	//FCLNX-0488
		rtn = EFAULT;								/* FCLNX-0488 */
	}

	HFC_EXIT("hfc_isolate_operation");				/* FCLNX-0488 */

	hfc_kfree(ap, isolinfo);													/* FCLNX_GPL-0151 */
	return(rtn);									/* FCLNX-0488 */
}	/* FCLNX-GPL-147 */


#ifdef HFC_DEBUG_HOTPLG
int hfc_debug_scsi_parameter( struct adap_info *ap, void *arg ) {

	struct Scsi_Host	*shost;
	struct scsi_device	*wksdev;

	struct adap_info	*wkap;
	struct target_info	*wktg;
	struct dev_info		*wkdp;
	int tid,lun,i,j;
	int instance;
	ulong  flags = 0;

	HFC_ENTRY("hfc_debug_scsi_parameter");

	for (instance=0;instance<MAX_ADAP_CNT;instance++) {
		wkap = hfc_manage_info.adap_info_arg[ instance ];
		
		if (wkap == NULL)
			continue;

		shost = wkap->hosts;
		spin_lock_irq(shost->host_lock);

		print_shost(shost);

		HFC_ERRPRT(" shost->host_blocked offset      = %lx.\n", (((ulong)(&shost->host_blocked)) - ((ulong) shost)));
		HFC_ERRPRT(" shost->max_host_blocked offset  = %lx.\n", (((ulong)(&shost->max_host_blocked)) - ((ulong) shost)));

		spin_unlock_irq(shost->host_lock);

		scsi_unblock_requests(shost);

	}


	HFC_EXIT("hfc_debug_scsi_parameter");
	return 0;
}
#endif /* HFC_DEBUG_HOTPLG */


//-----------------------------------------------------------------------
//FCLNX-0178 From here
#define IDFLGEN_ADR     (0x2fbU)
#define RAMMSK_ADR      (0x2f8U)
#define RAMADR_ADR      (0x2fcU)
#define RAMDT_FPP       (0x500U)
#define RAMDT_FIVE      (0x600U)
#define CMDFMEM_FPP     (0x044U)
#define CMDFMEM_FIVE    (0x034U)
#define ERASE_ADR       (0x011U)
#define RST_REQBSY_ADDR (0xC12U)

/*
 * Function:    hfc_ioctl_flash_init
 *
 * Purpose:     Flash-ROM Init
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *
 * Returns:     
 *  0         - Normal end
 *  -1        - Abend
 *
 * Notes:       
 */
int hfc_ioctl_flash_init( struct adap_info *ap )
{
	int rtn = 0;
	uint ramadr;

	HFC_ENTRY(__func__);
	if ( ap->pkg.type != HFC_PKTYPE_FPP ) {
		/* Set indirect access flag to enable */
		hfc_write_reg_ext(ap, IDFLGEN_ADR, 1, 0x08);
	}
	ramadr = (uint)hfc_read_reg_ext(ap,RAMADR_ADR,4);

	/* FCLNX-0298 The access of the same port is permitted.*/
	if ( ((ramadr & 0x80000000U) != 0) && ((ramadr & 0x00000003U) != PCI_FUNC(ap->pci_cfginf->devfn))) {
		/* Clear indirect access flag enable */
		hfc_write_reg_ext(ap, IDFLGEN_ADR, 1, 0x00);
		rtn = -1;
	}   
	HFC_EXIT(__func__) ;
	return rtn;
}

/*
 * Function:    hfc_ioctl_flash_read
 *
 * Purpose:     Flash-ROM Read
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *  offset    - 
 *  size      - 
 *  buf       - 
 *
 * Returns:     
 *  0         - Normal end
 *
 * Notes:       
 */
int hfc_ioctl_flash_read( struct adap_info *ap, int offset, int size, uchar *buf )
{
	int rtn = 0;
	uint adr=0;
	uint ramdt;
	uint dataL;
	uint dataB;
	uint i;
	int  j;

	HFC_ENTRY(__func__);
	if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
		adr = 0xa000000U;                           /* Flash start address */
		ramdt = RAMDT_FPP;
	} else {
		adr = 0xc000000U;                           /* Flash start address */
		ramdt = RAMDT_FIVE;
	}
	adr += offset;
	hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x20);     /* Type2 */
	i=0;
	while( size>0 ) {
		j=0;
		while (1) {
			hfc_write_reg_ext(ap, RAMADR_ADR, 4, adr);      /* Address setting */
			dataL = (uint)hfc_read_reg_ext( ap,ramdt+(adr % 0x80), 4);
			if ( dataL != 0 || j >= 1)
				break;
			j++;
		}
		HFC_4L_TO_4B(dataB, dataL);
		if ( size < 4 ) {
			memcpy(&buf[i],&dataB,size);
			i+=size;
			size=0;
			adr+=size;
		} else {
			memcpy(&buf[i],&dataB,4);
			i+=4;
			size-=4;
			adr+=4;
		}
	}
	hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x00);     /* Type2 clear */
	HFC_EXIT(__func__) ;
	return rtn;
}

/*
 * Function:    hfc_ioctl_flash_erase
 *
 * Purpose:     Flash-ROM Erase (only sector)
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *  offset    - 
 *
 * Returns:     
 *  0         - Normal end
 *
 * Notes:       
 */
int hfc_ioctl_flash_erase( struct adap_info *ap, int offset )
{
	int rtn = 0;
	uint adr;
	uint cmdfmem;
	int i;
	int erase_flag;

	HFC_ENTRY(__func__);
	if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
		adr = 0xa000000U;                           /* Flash start address */
		cmdfmem = CMDFMEM_FPP;
	} else {
		adr = 0xc000000U;                           /* Flash start address */
		cmdfmem = CMDFMEM_FIVE;
	}
	adr += offset;
	hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x20);     /* Type2 */
	hfc_write_reg_ext(ap, RAMADR_ADR, 4, adr);      /* Address setting */
	hfc_write_reg_ext(ap, cmdfmem, 1, 0x40);        /* Sector erase set */

	erase_flag = 0;
	for ( i=0; i<12; i++ ) {    /* 12sec wait */
		uchar data;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(1*HZ);
		data=(uchar)hfc_read_reg_ext(ap, ERASE_ADR, 1);
		if ( (data & 0x01) == 0 ) {
			erase_flag = 1;
			break;
		}
	}
	if ( erase_flag == 0 ) {
		hfc_write_reg_ext(ap, RST_REQBSY_ADDR, 1, 0x00);    /* Flash REQBSY flag reset */
		rtn = -1;
	}
	hfc_write_reg_ext(ap, cmdfmem, 1, 0x20);        /* Reset instructions */
	hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x00);     /* Type2 clear */
	HFC_EXIT(__func__) ;
	return rtn;
}

/*
 * Function:    hfc_ioctl_flash_write
 *
 * Purpose:     Flash-ROM Write
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *  offset    - 
 *  size      - 
 *  buf       - 
 *
 * Returns:     
 *  0         - Normal end
 *
 * Notes:       
 */
int hfc_ioctl_flash_write( struct adap_info *ap, int offset, int size, uchar *buf )
{
	int rtn = 0;
	uint adr=0;
	uint ramdt;
	uint dataL;
	uint dataB;
	uint data;
	uint i;
	uint j;

	HFC_ENTRY(__func__);

	if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
		hfc_write_reg_ext(ap, CMDFMEM_FPP, 1, 0x20);/* Reset instructions */
		adr = 0xa000000U;                           /* Flash start address */
		ramdt = RAMDT_FPP;
	} else {
		adr = 0xc000000U;                           /* Flash start address */
		ramdt = RAMDT_FIVE;
	}
	adr += offset;
	hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x20);     /* Type2 */
	i=0;
	while( size>0 ) {
		if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
			for(j=0; j<4; j++) {
				hfc_write_reg_ext(ap, RAMADR_ADR, 4, adr);		/* Address setting */
				hfc_write_reg_ext(ap, ramdt + j, 1, buf[i+j] );
			}
			hfc_write_reg_ext(ap, CMDFMEM_FPP, 1, 0x20);		/* Reset instructions */
			i+=4;
			size-=4;
			adr+=4;
		} else {
			hfc_write_reg_ext(ap, RAMADR_ADR, 4, adr);		/* Address setting */
			dataL = *((uint*)&buf[i]);
			HFC_4L_TO_4B(dataB, dataL);
			hfc_write_reg_ext(ap, ramdt + (adr % 0x80), 4, dataB );
			data=(uchar)hfc_read_reg_ext(ap, ERASE_ADR, 1);	/* FCLNX-0318 Dummy Read */
			i+=4;
			size-=4;
			adr+=4;
		}
	}
    hfc_write_reg_ext(ap, RAMMSK_ADR, 1, 0x00);     /* Type2 clear */
    HFC_EXIT(__func__) ;
    return rtn;
}

/*
 * Function:    hfc_ioctl_flash_fini
 *
 * Purpose:     Flash-ROM Fini
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *
 * Returns:     
 *  0         - Normal end
 *
 * Notes:       
 */
int hfc_ioctl_flash_fini( struct adap_info *ap )
{
	int rtn = 0;
	HFC_ENTRY(__func__);
	/* Indirect RAM access lock cancellation */
	hfc_write_reg_ext(ap, RAMADR_ADR, 4, 0x80000000);
	/* Remove indirect access flag enable */
	if ( ap->pkg.type != HFC_PKTYPE_FPP ) {
		hfc_write_reg_ext(ap, IDFLGEN_ADR, 1, 0x00);
	}
	HFC_EXIT(__func__) ;
	return rtn;
}

/*
 * Function:    hfc_wwn_info
 *
 * Purpose:     WWN/BOOT information setting
 *
 * Arguments:   
 *  ap        - Pointer to an adap_info structure
 *  arg       - Pointer to a data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Fail to copy data to buffer, or unable to attach data
 *  EIO       - Other errors occurred
 *
 * Notes:       
 */
int hfc_wwn_info( struct adap_info *ap, void *arg ) {

#define HFC_WWN_INFO_ORGWWN_OFFSET    (0x10000U)
#define HFC_WWN_INFO_ORGWWN_SIZE      (16*4U)       /* Only for read for 4port */
#define HFC_WWN_INFO_ADDWWN_OFFSET    (0x20000U)
#define HFC_WWN_INFO_BIOSWWN_OFFSET   (0xD8000U)
#define HFC_WWN_INFO_SECTOR_SIZE      (32*1024U)    /* 1 sector */
#define HFC_WWN_INFO_ERR_VER_CMD      (0x04U)   
#define HFC_WWN_INFO_ERR_BIOS_ENABLE  (0x05U)   
#define HFC_WWN_INFO_ERR_ADDWWN_ZERO  (0x06U)   
#define HFC_WWN_INFO_ERR_TGTWWN_ZERO  (0x07U)   
#define HFC_WWN_INFO_ERR_FLASH        (0xF0U)   

#define HFC_WWN_INFO_SET_ERR(code)    {wwn_info->resp_code = IOCTL_WWN_ERR | code;}

	int    rtn =0;
	int    frtn =0;
	int    par_cnt;
	int    pri_cnt;
	int    i;
	int    flash_open_flag=0;
	int    target_set_flag=0;
	struct hfc_ioctl_wwn    *wwn_info;		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uchar* adrbuf     =NULL;
	uchar* adrbufback =NULL;
	uchar* biosbuf    =NULL;
	uchar* biosbufback=NULL;
	uchar* portbuf;
	uint64_t addWWPN_uint64_L;
	uint64_t addWWNN_uint64_L;
	uint64_t addWWNN_uint64_B;
	ushort addWWN_Table_Rev=0;
	struct hfc_bios_info bios_info; /* FCLNX-GPL-056 */
        	

	HFC_ENTRY(__func__);
	/* Host was not found */
	if( ap->hosts==NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0xff );
		return (-EINVAL);
	}

	wwn_info = (struct hfc_ioctl_wwn *)hfc_kmalloc(ap, sizeof(struct hfc_ioctl_wwn), GFP_ATOMIC);		/* FCLNX_GPL-147 */
																			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	if (wwn_info == NULL) {													/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n",			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				 HFC_TRC_IOCTL_WWN_INFO, 0x09 );							/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return (ENOMEM);													/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	}																		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
																			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	memset(wwn_info, 0, sizeof(struct hfc_ioctl_wwn));						/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	/* Copy data to an internal data area as hfc_ioctl_wwn structure */
	if( COPYIN( (char *)arg, (char *)wwn_info, sizeof(struct hfc_ioctl_wwn) ) != 0 ) { /* FCLNX-GPL-202 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x00 );
		hfc_kfree(ap, wwn_info);													/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	STRUCTDUMP( DMP_WWN_INFO, (uchar *)wwn_info, sizeof(struct hfc_ioctl_wwn) ); /* FCLNX-GPL-202 */

	if( ( adrbuf = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
		/* Error if unable to allocate address buffer  */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x01);
		rtn = ENOMEM;
		goto HFC_WWN_INFO_FREE_END;
	}
	if( ( biosbuf = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
		/* Error if unable to allocate bios buffer  */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x02);
		rtn = ENOMEM;
		goto HFC_WWN_INFO_FREE_END;
	}

	wwn_info->resp_code = NORMAL_END;
	HFC_DBGPRT( "%s: command=0x%02x\n",__func__,wwn_info->command);
	switch ( wwn_info->command ) {
	case GET_WWN_INFO    :       /* Get wwn & boot information */
		wwn_info->version = VERSION_0;
		/* Init flash */
		frtn = hfc_ioctl_flash_init( ap );
		if ( frtn != 0 ) {
			wwn_info->resp_code = IOCTL_WWN_BUSY;
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x10 );
			goto HFC_WWN_INFO_END;
		} else {
			flash_open_flag=1;
		}

		/* Read orgWWN */
		frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ORGWWN_OFFSET, HFC_WWN_INFO_ORGWWN_SIZE, adrbuf );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x11 );
			goto HFC_WWN_INFO_END;
		}
		/* Original WWPN and original WWNN */
		memcpy((char*)&wwn_info->orgWWPN, adrbuf+16*ap->port_no  , 8);
		memcpy((char*)&wwn_info->orgWWNN, adrbuf+16*ap->port_no+8, 8);
		/* Read additional WWPN */
		frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x12 );
			goto HFC_WWN_INFO_END;
		}
		addWWN_Table_Rev = (adrbuf[0]<<8)+adrbuf[1];
		HFC_DBGPRT( "addWWN_Table_Rev=%04x\n", addWWN_Table_Rev );
		portbuf = adrbuf + 0x18U + 0x10U * ap->port_no;
		for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
			/* Additional WWPN and additional WWNN */
			memcpy((char*)&wwn_info->add_info[par_cnt].addWWPN, &portbuf[0x0], 8);
			HFC_8B_TO_8L(addWWPN_uint64_L , *((uint64_t*)&portbuf[0x0]) );
			addWWNN_uint64_L = addWWPN_uint64_L + 1;
			HFC_8L_TO_8B(addWWNN_uint64_B , addWWNN_uint64_L );
			memcpy((char*)&wwn_info->add_info[par_cnt].addWWNN, &addWWNN_uint64_B, 8);
		}
		/* Read BIOSWWNN */
		frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x13 );
			goto HFC_WWN_INFO_END;
		}
		portbuf = biosbuf + 0x100U * ap->port_no;

		/* flag byte */
		wwn_info->bios_enable = portbuf[0x1];

		/* model_name and fw_version */
		strncpy(wwn_info->model_name, ap->mp_adap_info->model_name, 16);			/* FCLNX-0329 */
		wwn_info->fw_version = hfc_get_sysrev(ap);								/* FCLNX-GPL-112 */

		/* location (Bus/Dev/Func) */
		wwn_info->location[0] = ap->pci_cfginf->bus->number;
		wwn_info->location[1] = PCI_SLOT(ap->pci_cfginf->devfn);
		wwn_info->location[2] = PCI_FUNC(ap->pci_cfginf->devfn);
		for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
			for( pri_cnt=0; pri_cnt<MAX_PRIORITY; pri_cnt++ ) {
				/* target WWN and LUN */
				memcpy((char*)&wwn_info->add_info[par_cnt].targetWWN[pri_cnt], &portbuf[0x18+0x10*pri_cnt], 8);
//				wwn_info->add_info[par_cnt].targetLUN[pri_cnt] = portbuf[0x17+0x10*pri_cnt];

				wwn_info->add_info[par_cnt].targetLUN[pri_cnt] = /* FCLNX-GPL-XX */
					portbuf[0x16 + 0x10 * pri_cnt] *256 + portbuf[0x17 + 0x10 * pri_cnt]; /* FCLNX-GPL-056 */


			}
		}
		break;
	case PUT_WWN_INFO    :       /* Put wwn infomation                       */
	case PUT_BOOT_INFO   :       /* Put boot infomation                      */
	case PUT_WWNBT_INFO  :       /* Put wwn and boot infomation                */
	case CLEAR_WWN_INFO  :       /* Clear wwn and boot infomation              */
	case CLEAR_BOOT_INFO :       /* Clear wwn and boot infomation              */
		/* VERSION CHECK */			/* FCLNX-GPL-056 */
//		if(wwn_info->version != VERSION_0) {
//			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_VER_CMD);
//			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x20 );
//			goto HFC_WWN_INFO_END;
//		}
		if( ( adrbufback = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
			/* Error if failed to allocate adrbufback */
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x02 );
			rtn = ENOMEM;
			goto HFC_WWN_INFO_FREE_END;
		}
		if( ( biosbufback = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
			/* Error if failed to allocate biosbufback */
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x02 );
			rtn = ENOMEM;
			goto HFC_WWN_INFO_FREE_END;
		}
		/* Flash init */
		frtn = hfc_ioctl_flash_init( ap );
		if ( frtn != 0 ) {
			wwn_info->resp_code = IOCTL_WWN_BUSY;
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x21 );
			goto HFC_WWN_INFO_END;
		} else {
			flash_open_flag=1;
		}
		/* Create addWWPN and addWWNN data */
		if ( (wwn_info->command == PUT_WWN_INFO) || 
			(wwn_info->command == PUT_WWNBT_INFO) || 
			(wwn_info->command == CLEAR_WWN_INFO) ) {
			frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x22 );
				goto HFC_WWN_INFO_END;
			}
			addWWN_Table_Rev = (adrbuf[0]<<8)+adrbuf[1];
			HFC_DBGPRT( "addWWN_Table_Rev=%04x\n", addWWN_Table_Rev );
			if ( addWWN_Table_Rev == 0xffffU ) {
				memset(adrbuf,0U,HFC_WWN_INFO_SECTOR_SIZE);
			}
			memcpy(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE);   /* Read backup */
			portbuf = adrbuf + 0x18U + 0x10U * ap->port_no;
			if ( wwn_info->command != CLEAR_WWN_INFO ) {     /* PUT_WWN_INFO and PUT_WWNBT_INFO */
				for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
					/* addWWPN and addWWNN */
					if ( (wwn_info->add_info[par_cnt].addWWPN == 0) ||
							(wwn_info->add_info[par_cnt].addWWNN == 0) ){
						HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_ADDWWN_ZERO);
						HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x23 );
						goto HFC_WWN_INFO_END;
					}
					memcpy(&portbuf[0x0], (char*)&wwn_info->add_info[par_cnt].addWWPN, 8);

				}
			} else {
				for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
					/* addWWPN and addWWNN */
					for(i=0;i<8;i++) {
						portbuf[0+i] = 0x00;
					}
				}
			}
		}
		/* Create BIOSWWPN data */
		if ( (wwn_info->command == PUT_BOOT_INFO) || 
				(wwn_info->command == PUT_WWNBT_INFO) || 
				(wwn_info->command == CLEAR_BOOT_INFO) ) {
			frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x29 );
				goto HFC_WWN_INFO_END;
			}
			portbuf = biosbuf + 0x100U * ap->port_no;
			memcpy(biosbufback,biosbuf,HFC_WWN_INFO_SECTOR_SIZE);   /* Read backup */
			if ( wwn_info->command != CLEAR_BOOT_INFO ) {    /* PUT_BOOT_INFO & PUT_WWNBT_INFO */

				portbuf[0x1] = wwn_info->bios_enable;            /* Set FLAG Byte */

				target_set_flag=0;
				for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
					for( pri_cnt=0; pri_cnt<MAX_PRIORITY; pri_cnt++ ) {
						/* targetWWN & targetLUN */
						if ( wwn_info->add_info[par_cnt].targetWWN[pri_cnt] != 0 ) {
							memcpy(&portbuf[0x18+0x10*pri_cnt], (char*)&wwn_info->add_info[par_cnt].targetWWN[pri_cnt], 8);
//							portbuf[0x17+0x10*pri_cnt] = (uchar)( wwn_info->add_info[par_cnt].targetLUN[pri_cnt] & 0x00ffU      );
							portbuf[0x16+0x10*pri_cnt] /* FCLNX-GPL-XX */
								= (uchar)( (wwn_info->add_info[par_cnt].targetLUN[pri_cnt] & 0xff00U)>>8 );
							portbuf[0x17+0x10*pri_cnt] = 
								(uchar)( wwn_info->add_info[par_cnt].targetLUN[pri_cnt] &0x00ffU);
							
							target_set_flag=1;
						}
					}
				}

			} else {
				portbuf[0x1] &= 0x7FU;
				for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
					for( pri_cnt=0; pri_cnt<MAX_PRIORITY; pri_cnt++ ) {
						/* targetWWN and targetLUN */
						for(i=0;i<10;i++) { /* LUN(2)+WWN(8)=10 */
							portbuf[0x16+i+0x10*pri_cnt] = 0x00;
						}
					}
				}
			}
		}	
		/* Check data and write data all together at the end in the process */
		/* Write addWWPN and addWWNN */
		if ( (wwn_info->command == PUT_WWN_INFO) || 
				(wwn_info->command == PUT_WWNBT_INFO) || 
				(wwn_info->command == CLEAR_WWN_INFO) ) {

			if ( memcmp(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE) == 0 ) {  

				HFC_DBGPRT(" %s: FLASH: addWWPN/addWWNN none changed \n",__func__ );
			} else {
				memcpy(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE);   
				frtn = hfc_ioctl_flash_erase( ap, HFC_WWN_INFO_ADDWWN_OFFSET );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x25 );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_write( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x26 );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x27 );
					goto HFC_WWN_INFO_END;
				}
				if ( memcmp(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE) != 0 ) {  //compare for write backup
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x28 );
					goto HFC_WWN_INFO_END;
				}
			}
		}
	
		/* Write BIOSWWPN */
		if ( (wwn_info->command == PUT_BOOT_INFO) || 
				(wwn_info->command == PUT_WWNBT_INFO) || 
				(wwn_info->command == CLEAR_BOOT_INFO) ) {

			if ( memcmp(biosbufback,biosbuf,HFC_WWN_INFO_SECTOR_SIZE) == 0 ) {  

				HFC_DBGPRT(" %s: FLASH: none changed \n",__func__ );
			} else {
				memcpy(biosbufback,biosbuf,HFC_WWN_INFO_SECTOR_SIZE);   
				frtn = hfc_ioctl_flash_erase( ap, HFC_WWN_INFO_BIOSWWN_OFFSET );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x2c );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_write( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x2d );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x2e );
					goto HFC_WWN_INFO_END;
				}
				if ( memcmp(biosbufback,biosbuf,HFC_WWN_INFO_SECTOR_SIZE) != 0 ) {  //compare for write backup
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x2f );
					goto HFC_WWN_INFO_END;
				}
			}
		}

		break;

	/*------------------------------------------------------------------------- FCLNX-GPL-056 */	 
	case GET_ALL_INFO			:	/* Get all boot information */
		
		/* VERSION CHECK */
		if(wwn_info->version != VERSION_1) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_VER_CMD);
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x50 );
			goto HFC_WWN_INFO_END;
		}

		/* Init flash */
		frtn = hfc_ioctl_flash_init( ap );
		if ( frtn != 0 ) {
			wwn_info->resp_code = IOCTL_WWN_BUSY;
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x51 );
			goto HFC_WWN_INFO_END;
		} else {
			flash_open_flag=1;
		}	

		/* Read all bios data */
		frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x52 );
			goto HFC_WWN_INFO_END;
		}

		portbuf = biosbuf + 0x100U * ap->port_no;


		memset( (char *)&bios_info, 0,  sizeof(struct hfc_bios_info));
		memcpy( (char *)&bios_info, portbuf, 16);
		for( i = 0; i < BCV_TABLE_ENTRY; i++ ){
			memcpy( (char *)&bios_info.BCV_Table[i].wwpn, &portbuf[0x18 + 0x10 * i], 8);
			bios_info.BCV_Table[i].lun = portbuf[0x16 + 0x10 * i] * 256 + portbuf[0x17 + 0x10 * i];
		}
		memcpy( (char *)&bios_info.expansion_control[0],  &portbuf[0x90], 112);

		/* Write back to hfc_ioctl_wwn structure */
		if( COPYOUT( (uchar *)&bios_info, ( char * )wwn_info->boot_info_p, sizeof(struct hfc_bios_info) ) != 0 ) {
			HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x53 );
			rtn = EFAULT;
		}
                break;

	case PUT_ALL_INFO			:			/* Put all boot information */
	case PUT_ALL_INFO_AND_AWWN	:			/* Put all boot information and Put addWWN */
	case PUT_BOOT_INFO_FROM_ALLDATA		:	/* Put boot information from all boot info */
	case PUT_WWNBT_INFO_FROM_ALLDATA	:	/* Put boot and addWWN from all boot info */
	
		/* VERSION CHECK */
		if(wwn_info->version != VERSION_1) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_VER_CMD);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x54 );
			goto HFC_WWN_INFO_END;
		}

		if( ( adrbufback = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
			/* Error if failed to allocate adrbufback */
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x55 );
			rtn = ENOMEM;
			goto HFC_WWN_INFO_FREE_END;
		}
		if( ( biosbufback = hfc_kmalloc(ap, HFC_WWN_INFO_SECTOR_SIZE, GFP_ATOMIC) )==NULL ) {
			/* Error if failed to allocate biosbufback */
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x56 );
			rtn = ENOMEM;
			goto HFC_WWN_INFO_FREE_END;
		}

		/* Init flash */
		frtn = hfc_ioctl_flash_init( ap );
		if ( frtn != 0 ) {
			wwn_info->resp_code = IOCTL_WWN_BUSY;
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x57 );
			goto HFC_WWN_INFO_END;
		} else {
			flash_open_flag=1;
		}	

		/* Read all bios data */
		frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x58 );
			goto HFC_WWN_INFO_END;
		}

		if( COPYIN( ( char * )wwn_info->boot_info_p, (char *)&bios_info, sizeof(struct hfc_bios_info) ) != 0 ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x59 );
			hfc_kfree(ap, wwn_info);													/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
			return EFAULT;
		}

		memcpy(biosbufback, biosbuf, HFC_WWN_INFO_SECTOR_SIZE);   /* Read backup */

		if ( (wwn_info->command == PUT_ALL_INFO) ||
		     (wwn_info->command == PUT_ALL_INFO_AND_AWWN) ||
		     (wwn_info->command == PUT_BOOT_INFO_FROM_ALLDATA) ||
		     (wwn_info->command == PUT_WWNBT_INFO_FROM_ALLDATA) ){
			

			portbuf = biosbuf + 0x100U * ap->port_no;
	
			if ( (wwn_info->command == PUT_ALL_INFO) ||
			     (wwn_info->command == PUT_ALL_INFO_AND_AWWN) ){				

				memcpy(portbuf,	(char *)&bios_info, 16);
				for( pri_cnt=0; pri_cnt<BCV_TABLE_ENTRY; pri_cnt++ ){
					memcpy(&portbuf[0x18 + 0x10 * pri_cnt], (char*)&(bios_info.BCV_Table[pri_cnt].wwpn), 8);
					portbuf[0x16 + 0x10 * pri_cnt]
						= (uchar)( (bios_info.BCV_Table[pri_cnt].lun & 0xff00U)>>8 );	
					portbuf[0x17 + 0x10 * pri_cnt]
						= (uchar)( (bios_info.BCV_Table[pri_cnt].lun & 0x00ffU) );
				}
				memcpy(&portbuf[0x90], (char *)&bios_info.expansion_control[0], 112);	


			}else{
				
				portbuf[0x1] = bios_info.FLAG;
				
				target_set_flag=0;
				for( pri_cnt=0; pri_cnt<BCV_TABLE_ENTRY; pri_cnt++ ) {
					/* targetWWN & targetLUN */
					if ( bios_info.BCV_Table[pri_cnt].wwpn != 0 ) {
						memcpy(&portbuf[0x18 + 0x10 * pri_cnt], (char*)&(bios_info.BCV_Table[pri_cnt].wwpn), 8);

						portbuf[0x16 + 0x10 * pri_cnt]
							= (uchar)( (bios_info.BCV_Table[pri_cnt].lun & 0xff00U)>>8 );
						portbuf[0x17 + 0x10 * pri_cnt] 
							= (uchar)( (bios_info.BCV_Table[pri_cnt].lun & 0x00ffU) );

						target_set_flag=1;

					}
					
				}

			}

		}


		if ( (wwn_info->command == PUT_ALL_INFO_AND_AWWN ) ||
		     (wwn_info->command == PUT_WWNBT_INFO_FROM_ALLDATA) )
		{
			
			frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5a );
				goto HFC_WWN_INFO_END;
			}
			
			addWWN_Table_Rev = (adrbuf[0]<<8)+adrbuf[1];
			HFC_DBGPRT( "addWWN_Table_Rev=%04x\n", addWWN_Table_Rev );
			if ( addWWN_Table_Rev == 0xffffU ) {
				memset(adrbuf,0U,HFC_WWN_INFO_SECTOR_SIZE);
			}
			
			memcpy(adrbufback, adrbuf, HFC_WWN_INFO_SECTOR_SIZE);   /* Read backup */
			portbuf = adrbuf + 0x18U + 0x10U * ap->port_no;

			for( par_cnt=0; par_cnt<MAX_PARTITION; par_cnt++ ) {
				/* addWWPN and addWWNN */
				if ( (wwn_info->add_info[par_cnt].addWWPN == 0) ||
	 					(wwn_info->add_info[par_cnt].addWWNN == 0) ){
							HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_ADDWWN_ZERO);
							HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5b );
						goto HFC_WWN_INFO_END;
				}
				memcpy(&portbuf[0x0], (char*)&wwn_info->add_info[par_cnt].addWWPN, 8);
			}
			

			if ( memcmp(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE) == 0 ) {  

				HFC_DBGPRT( "%s: FLASH: addWWPN/addWWNN none changed \n",__func__ );

			}
			else
			{
				memcpy(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE);   
				frtn = hfc_ioctl_flash_erase( ap, HFC_WWN_INFO_ADDWWN_OFFSET );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5c );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_write( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5d );
					goto HFC_WWN_INFO_END;
				}
				frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_ADDWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, adrbuf );
				if ( frtn != 0 ) {
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5e );
					goto HFC_WWN_INFO_END;
				}
				if ( memcmp(adrbufback,adrbuf,HFC_WWN_INFO_SECTOR_SIZE) != 0 ) {  //compare for write backup
					HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
					HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x5f );
					goto HFC_WWN_INFO_END;
				}
					
			}	
		}
	
		if ( memcmp(biosbufback,biosbuf,HFC_WWN_INFO_SECTOR_SIZE) == 0 ) {

				HFC_DBGPRT( "%s: FLASH: none changed \n",__func__ );
		}
		else
		{
							
			memcpy(biosbufback, biosbuf, HFC_WWN_INFO_SECTOR_SIZE);
			frtn = hfc_ioctl_flash_erase( ap, HFC_WWN_INFO_BIOSWWN_OFFSET );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x60 );
				goto HFC_WWN_INFO_END;
			}
		
			frtn = hfc_ioctl_flash_write( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x61 );
				goto HFC_WWN_INFO_END;
			}
		
			frtn = hfc_ioctl_flash_read( ap, HFC_WWN_INFO_BIOSWWN_OFFSET, HFC_WWN_INFO_SECTOR_SIZE, biosbuf );
			if ( frtn != 0 ) {
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x62 );
				goto HFC_WWN_INFO_END;
			}
					
			if ( memcmp(biosbufback, biosbuf, HFC_WWN_INFO_SECTOR_SIZE) != 0 ) {  //compare for write backup
				HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x63 );
				goto HFC_WWN_INFO_END;
			}
		}

		break;	
		
	default:
		HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_VER_CMD);
	}

HFC_WWN_INFO_END:
	if ( flash_open_flag == 1 ) {
		frtn = hfc_ioctl_flash_fini( ap );
		if ( frtn != 0 ) {
			HFC_WWN_INFO_SET_ERR(HFC_WWN_INFO_ERR_FLASH);
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x40 );
		}
		flash_open_flag = 0;
	}
	/* Write back data to hfc_ioctl_wwn structure */
	if( COPYOUT( ( char * )wwn_info, ( char * )arg, sizeof(struct hfc_ioctl_wwn) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_WWN_INFO, 0x21 );
		rtn = EFAULT;
	}
HFC_WWN_INFO_FREE_END:
	if ( adrbuf != NULL ) {
		hfc_kfree(ap, adrbuf );
		adrbuf = NULL;
	}
	if ( biosbuf != NULL ) {
		hfc_kfree(ap, biosbuf );
		biosbuf = NULL;
	}
	if ( adrbufback != NULL ) {
		hfc_kfree(ap, adrbufback );
		adrbufback = NULL;
	}
	if ( biosbufback != NULL ) {
		hfc_kfree(ap, biosbufback );
		biosbufback = NULL;
	}
	STRUCTDUMP( DMP_WWN_INFO, (uchar *)wwn_info, sizeof(struct hfc_ioctl_wwn) ); /* FCLNX-GPL-202 */
	hfc_kfree(ap, wwn_info);													/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_EXIT(__func__) ;
	return rtn;
}


#ifdef HFC_DEBUG_HOTPLG
/*
 * Function:    print_sdev
 *
 * Purpose:     print struct scsi_device
 *
 * Arguments:   
 *  sdev       -
 *
 * Returns:     
 *
 * Notes:       
 */
void print_sdev(struct scsi_device *sdev)
{
	char wkc[512];
	int i;
	

	HFC_ERRPRT("================================================================\n");
	HFC_ERRPRT(" print struct scsi_device\n");
	HFC_ERRPRT(" host%d:channel%d:td%d:lun%d\n",   sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	HFC_ERRPRT("    sdev->device_busy = 0x%x\n",sdev->device_busy);

	HFC_ERRPRT("    sdev->queue_depth = 0x%x\n",sdev->queue_depth);
	HFC_ERRPRT("    sdev->last_queue_full_depth = 0x%x\n",sdev->last_queue_full_depth);
	HFC_ERRPRT("    sdev->last_queue_full_count = 0x%x\n",sdev->last_queue_full_count);

	HFC_ERRPRT("    sdev->id = 0x%x\n",sdev->id);
	HFC_ERRPRT("    sdev->lun = 0x%x\n",sdev->lun);
	HFC_ERRPRT("    sdev->channel = 0x%x\n",sdev->channel);
	HFC_ERRPRT("    sdev->manufacturer = 0x%x\n",sdev->manufacturer);
	HFC_ERRPRT("    sdev->sector_size = 0x%x\n",sdev->sector_size);

	HFC_ERRPRT("    sdev->devfs_name = %s\n",sdev->devfs_name);

	HFC_ERRPRT("    sdev->type = 0x%x\n",sdev->type);
	HFC_ERRPRT("    sdev->scsi_level = 0x%x\n",sdev->scsi_level);
	HFC_ERRPRT("    sdev->inq_periph_qual = 0x%x\n",sdev->inq_periph_qual);
	HFC_ERRPRT("    sdev->inquiry_len = 0x%x\n",sdev->inquiry_len);

	memset(wkc,0,sizeof(wkc));
	for(i=0;i<sizeof(wkc)/2;i++) {
		if (i>=sdev->inquiry_len)
			break;
		
		if ((sdev->inquiry[i] >> 4) <= 9)
			wkc[i*2] = '0'+ (sdev->inquiry[i] >> 4);
		else
			wkc[i*2] = 'a'+ (sdev->inquiry[i] >> 4);
		
		if ((sdev->inquiry[i] & 0xf) <= 9)
			wkc[i*2+1] = '0'+ (sdev->inquiry[i] & 0xf);
		else
			wkc[i*2+1] = 'a'+ (sdev->inquiry[i] & 0xf);
	}

	HFC_ERRPRT("    sdev->inquiry = %s\n",wkc);
	HFC_ERRPRT("    sdev->vendor = %s\n",sdev->vendor);
	HFC_ERRPRT("    sdev->model = %s\n",sdev->model);
	HFC_ERRPRT("    sdev->rev = %s\n",sdev->rev);
	HFC_ERRPRT("    sdev->current_tag = 0x%d\n",sdev->current_tag);

	HFC_ERRPRT("    sdev->sdev_target = 0x%lx\n",(ulong)sdev->sdev_target);

	HFC_ERRPRT("    sdev->sdev_bflags = 0x%x\n",sdev->sdev_bflags);

	HFC_ERRPRT("    sdev->writeable = %d\n",sdev->writeable);
	HFC_ERRPRT("    sdev->removable = %d\n",sdev->removable);
	HFC_ERRPRT("    sdev->changed = %d\n",sdev->changed);
	HFC_ERRPRT("    sdev->busy = %d\n",sdev->busy);
	HFC_ERRPRT("    sdev->lockable = %d\n",sdev->lockable);
	HFC_ERRPRT("    sdev->locked = %d\n",sdev->locked);
	HFC_ERRPRT("    sdev->borken = %d\n",sdev->borken);
	HFC_ERRPRT("    sdev->disconnect = %d\n",sdev->disconnect);
	HFC_ERRPRT("    sdev->soft_reset = %d\n",sdev->soft_reset);
	HFC_ERRPRT("    sdev->sdtr = %d\n",sdev->sdtr);
	HFC_ERRPRT("    sdev->wdtr = %d\n",sdev->wdtr);
	HFC_ERRPRT("    sdev->ppr = %d\n",sdev->ppr);
	HFC_ERRPRT("    sdev->tagged_supported = %d\n",sdev->tagged_supported);
	HFC_ERRPRT("    sdev->simple_tags = %d\n",sdev->simple_tags);
	HFC_ERRPRT("    sdev->ordered_tags = %d\n",sdev->ordered_tags);
	HFC_ERRPRT("    sdev->single_lun = %d\n",sdev->single_lun);
	HFC_ERRPRT("    sdev->was_reset = %d\n",sdev->was_reset);
	HFC_ERRPRT("    sdev->expecting_cc_ua = %d\n",sdev->expecting_cc_ua);
	HFC_ERRPRT("    sdev->use_10_for_rw = %d\n",sdev->use_10_for_rw);
	HFC_ERRPRT("    sdev->use_10_for_ms = %d\n",sdev->use_10_for_ms);
	HFC_ERRPRT("    sdev->skip_ms_page_8 = %d\n",sdev->skip_ms_page_8);
	HFC_ERRPRT("    sdev->skip_ms_page_3f = %d\n",sdev->skip_ms_page_3f);
	HFC_ERRPRT("    sdev->use_192_bytes_for_3f = %d\n",sdev->use_192_bytes_for_3f);
	HFC_ERRPRT("    sdev->no_start_on_add = %d\n",sdev->no_start_on_add);
	HFC_ERRPRT("    sdev->allow_restart = %d\n",sdev->allow_restart);

	HFC_ERRPRT("    sdev->device_blocked = 0x%x\n",sdev->device_blocked);
	HFC_ERRPRT("    sdev->max_device_blocked = 0x%x\n",sdev->max_device_blocked);
	HFC_ERRPRT("    sdev->timeout = 0x%x\n",sdev->timeout);
	HFC_ERRPRT("================================================================\n");

}


/*
 * Function:    print_starget
 *
 * Purpose:     print struct scsi_device 
 *
 * Arguments:   
 *  starget    -
 *
 * Returns:     
 *
 * Notes:       
 */
void print_starget(struct scsi_target *starget)
{
	HFC_ERRPRT("================================================================\n");
	HFC_ERRPRT(" print struct scsi_target\n");
	HFC_ERRPRT("    starget->starget_sdev_user = %llx\n",(unsigned long long)(ulong)starget->starget_sdev_user);
	HFC_ERRPRT("    starget->channel = %d\n",starget->channel);
	HFC_ERRPRT("    starget->id = %d\n",starget->id);
	HFC_ERRPRT("    starget->create = %d\n",starget->create);
	HFC_ERRPRT("    starget->starget_data[0] = %ld\n",starget->starget_data[0]);
	HFC_ERRPRT("================================================================\n");
}

/*
 * Function:    print_shost
 *
 * Purpose:     print struct scsi_device 
 *
 * Arguments:   
 *  shost      -
 *
 * Returns:     
 *
 * Notes:       
 */
void print_shost(struct Scsi_Host *shost)
{
	HFC_ERRPRT("================================================================\n");
	HFC_ERRPRT(" print struct Scsi_Host\n");

	HFC_ERRPRT("    shost->__devices.next = %lx\n",(ulong)shost->__devices.next);
	HFC_ERRPRT("    shost->__devices.prev = %lx\n",(ulong)shost->__devices.prev);
	
	HFC_ERRPRT("    shost->free_list.next = %lx\n",(ulong)shost->free_list.next);
	HFC_ERRPRT("    shost->free_list.prev = %lx\n",(ulong)shost->free_list.prev);
	HFC_ERRPRT("    shost->starved_list.next = %lx\n",(ulong)shost->starved_list.next);
	HFC_ERRPRT("    shost->starved_list.prev = %lx\n",(ulong)shost->starved_list.prev);

	HFC_ERRPRT("    shost->eh_cmd_q.next = %lx\n",(ulong)shost->eh_cmd_q.next);
	HFC_ERRPRT("    shost->eh_cmd_q.prev = %lx\n",(ulong)shost->eh_cmd_q.prev);
	
	HFC_ERRPRT("    shost->eh_active = %d\n",shost->eh_active);
	HFC_ERRPRT("    shost->eh_kill = %d\n",shost->eh_kill);

	HFC_ERRPRT("    shost->host_busy = %d\n",shost->host_busy);
	HFC_ERRPRT("    shost->host_failed = %d\n",shost->host_failed);
    
	HFC_ERRPRT("    shost->host_no = %d\n",shost->host_no);
	HFC_ERRPRT("    shost->resetting = %d\n",shost->resetting);
	HFC_ERRPRT("    shost->last_reset = %ld\n",shost->last_reset);

	HFC_ERRPRT("    shost->max_id = %d\n",shost->max_id);
	HFC_ERRPRT("    shost->max_lun = %d\n",shost->max_lun);
	HFC_ERRPRT("    shost->max_channel = %d\n",shost->max_channel);

	HFC_ERRPRT("    shost->unique_id = %d\n",shost->unique_id);
	HFC_ERRPRT("    shost->max_cmd_len = %d\n",shost->max_cmd_len);

	HFC_ERRPRT("    shost->this_id = %d\n",shost->this_id);
	HFC_ERRPRT("    shost->can_queue = %d\n",shost->can_queue);
	HFC_ERRPRT("    shost->cmd_per_lun = %d\n",shost->cmd_per_lun);
	HFC_ERRPRT("    shost->sg_tablesize = %d\n",shost->sg_tablesize);
	HFC_ERRPRT("    shost->max_sectors = %d\n",shost->max_sectors);
	HFC_ERRPRT("    shost->dma_boundary = %ld\n",shost->dma_boundary);

	HFC_ERRPRT("    shost->unchecked_isa_dma = %d\n",shost->unchecked_isa_dma);
	HFC_ERRPRT("    shost->use_clustering = %d\n",shost->use_clustering);
	HFC_ERRPRT("    shost->use_blk_tcq = %d\n",shost->use_blk_tcq);

	HFC_ERRPRT("    shost->host_self_blocked = %d\n",shost->host_self_blocked);
    
	HFC_ERRPRT("    shost->reverse_ordering = %d\n",shost->reverse_ordering);
	HFC_ERRPRT("    shost->host_blocked = %d\n",shost->host_blocked);
	HFC_ERRPRT("    shost->max_host_blocked = %d\n",shost->max_host_blocked);

	HFC_ERRPRT("    shost->base = %ld\n",shost->base);
	HFC_ERRPRT("    shost->io_port = %ld\n",shost->io_port);
	HFC_ERRPRT("    shost->n_io_port = %d\n",shost->n_io_port);
	HFC_ERRPRT("    shost->dma_channel = %d\n",shost->dma_channel);
	HFC_ERRPRT("    shost->irq = %d\n",shost->irq);

	HFC_ERRPRT("    shost->shost_state=%ld\n",shost->shost_state);

	HFC_ERRPRT("    shost->sht_legacy_list.next = %lx\n", (unsigned long) shost->sht_legacy_list.next);
	HFC_ERRPRT("    shost->sht_legacy_list.prev = %lx\n", (unsigned long) shost->sht_legacy_list.prev);

	HFC_ERRPRT("    shost->hostdata[0] = %ld\n",shost->hostdata[0]);

	HFC_ERRPRT("================================================================\n");
}


/*
 * Function:    hfc_dump_sdev
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - pointer to an adap_info structure
 *  arg       - pointer to a structure changing data
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Inside copy processing failure, a of memory touch it and fail
 *  EIO       - Other errors occurred
 *
 * Notes:       
 */
void hfc_dump_sdev( struct scsi_device *sdev )
{
struct scsi_device *wksdev,*wksdev2;
struct Scsi_Host *shost;

	wksdev = sdev;
	shost = sdev->host;

	HFC_ERRPRT(" print struct scsi_device in sdev->siblings\n");
	list_for_each_entry(wksdev, &shost->__devices, siblings) {
		print_sdev(wksdev);
	}

	HFC_ERRPRT(" print struct scsi_device in sdev->same_target_siblings\n");
	list_for_each_entry(wksdev, &shost->__devices, siblings) {
		if ( (wksdev->channel == sdev->channel)
		  && (wksdev->id      == sdev->id) ) {
			list_for_each_entry(wksdev2, &wksdev->same_target_siblings, same_target_siblings) {
				print_sdev(wksdev2);
			}
			break;
		}
	}
	
	HFC_ERRPRT(" print struct scsi_target\n");
	print_starget(sdev->sdev_target);

	HFC_ERRPRT(" print struct Scsi_Host\n");
	print_shost(sdev->host);
}
#endif /* HFC_DEBUG_HOTPLG */
void hfc_start_isolate(struct adap_info *ap){
	uchar pre_status = 0;
	
	pre_status = ap->hba_isolation;
	if (pre_status == HFC_ISOL_STOP){
		if(hfc_check_hba_isolation(ap) == HFC_ISOL_START){
			ap->hba_isolation = HFC_ISOL_START;
			if(hfc_manage_info.hfcldd_mp_mod){	/* FCLNX-0625 *//* FCLNX-GPL-331 */
				hfc_manage_info.npubp->hfc_clear_errinfo(ap);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_clear_errinfo_i(ap);								/* FCLNX-GPL-349 */
			}
		}
	}
}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
void hfc_change_dev_loss_tmo(struct adap_info *ap)
{
	struct target_info	*target;
//	ulong				flags = 0;
	int					i=0;

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) /* FCLNX-GPL-FX-472 */
		return;

	HFC_ENTRY("hfc_change_dev_loss_tmo");
	
//	HFC_ADAPLOCK_IRQSAVE(flags);
	for (i=0;i<MAX_TARGET_PROBE;i++) { 
		target = hfc_hash_target_valid(ap, i);
		if( target != NULL ){
			if ( target->rport != NULL){
				target->rport->dev_loss_tmo = ap->dev_loss_tmo;
			}
		}
	}
//	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	HFC_EXIT("hfc_change_dev_loss_tmo");
	
	return ;
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
#endif
