/*
 * hfcl_ioctl_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char ioctl_fx_rcsid[] = "$Id: hfcl_ioctl_fx.c,v 1.1.2.20.2.9.2.5.2.7 2015/12/24 21:13:28 toyo Exp $";

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
/**       8.SCSI ADpp_STAT          OK /OK                                   **/
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

#include "hfcldd_fx.h"
#include "hfcl_strategy_fx.h"
#include "hfcl_timer_recovery_fx.h"
#include "hfcl_ioctl_fx.h"
#include "hfcl_top_fx.h"
#include "hfcl_detect_fx.h"
#include "hfcl_hand_timer_trace_fx.h"

#include "hfcl_tbol.h"                  //FCLNX-0178
#include "hfcddwwn.h"                   //FCLNX-0178
#include "hfcldd_conf.h"

#include "hfcl_mlpf.h"					/* FCLNX-0506 */
#include "hfcl_mlpf_fx.h"
#include "hfcl_npiv_fx.h"

extern uint	raslog_install;				/* FCLNX-467	*/

extern int hfc_fx_diag( void *, struct port_info * );
extern const struct cr_PartsNumber cr_pn[];				/* FCLNX-0329 */

/*-- global variable --*/
void structdump( int loc, uchar *p, int size );
static int hfc_fx_rtn_scsi_cmnd( struct scsi_cmnd *cmnd, void *arg, uint64_t *buffer, int type );

int hfc_fx_scsi_scan( struct port_info *pp, void *arg );


/* The fixed number for dump identification */
#define DMP_INQU			0x020
#define DMP_SCSICMD			0x021
#define DMP_SNSBUF			0x022
#define DMP_REQBUF			0x023
#define DMP_IOCMD			0x060
#define DMP_GIDFT			0x080
#define DMP_QWWN			0x100
#define DMP_GPNID			0x101
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
#define DMP_FC4STAT			0x550
#define DMP_FLASH_READ		0x600
#define DMP_FLASH_WRITE		0x601
#define DMP_WWN_INFO		0x610
#define DMP_MP_TARGET		0x710


/* Macro for dump functions */
#ifndef _HFC_DEBUG	/* FCLNX-0510	*/
	#define	STRUCTDUMP( LOC, PTR, SIZE ) { while(0) {}; }
#else
	#define STRUCTDUMP( LOC, PTR, SIZE ) { structdump( LOC, PTR, SIZE ); }
#endif			/* FCLNX-0510	*/


#define HFC_FX_STRATEGY( _pp, _SRB )		hfc_fx_strategy_pg( _pp, _SRB )

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

#if !( defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
/*
 * Function:    hfc_fx_open
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
int hfc_fx_open( struct inode *inode, struct file *file ) {

	struct 	port_info	*pp;	/* A pointer to an port_info */
	int		major;
	int		minor;
	int		rtn = 0;			/* A return code of a function */

//	HFC_ENTRY("hfc_fx_open") ;
	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);
	
	/* Find port_info from minor number */
	pp = hfc_manage_info.port_info_arg[ minor ];

	if( pp == NULL ) {
		HFC_DBGPRT(" hfcldd : hfc_fx_open pp=NULL\n");
		/* No corresponding port_info -> error */
		rtn = -EIO;
		return rtn;
	}
	if ( ( pp -> dev_major != major ) || ( pp -> dev_minor != minor ) ) {
		HFC_DBGPRT(" hfcldd : hfc_fx_open major minor\n");
		/* Major and minor number are unmatched with those stored in port_info. */
		rtn = -EIO;
		return rtn;
	}

	if ( !test_bit(HFC_PS_ENABLE,(ulong *)&pp->status) )  {
		HFC_DBGPRT(" hfcldd%d : hfc_fx_open not ENABLE\n",pp -> dev_minor);
		rtn = -EIO;
		return rtn;
	}
	/* Acquire lock to handle port_info */
	if( HFC_SEMAPHORE_LOCK(pp->sem) ) 
		return -ERESTARTSYS ;
	
	if( (file->f_flags & O_WRONLY) || (file->f_flags & O_RDWR) )
	{
		if( test_bit(HFC_FX_WRITE_PROCESS, (ulong *)&pp->ioctl_status) )
		{
			HFC_DBGPRT(" hfcldd%d : hfc_open HFC_FX_WRITE_PROCESS\n",pp->dev_minor);
			HFC_SEMAPHORE_UNLOCK(pp->sem) ;
			return -EBUSY ;
		}
		pp->open_file = file;
		set_bit(HFC_FX_WRITE_PROCESS, (ulong *)&pp->ioctl_status) ;
	}
	
	pp -> open_status = HFC_OPENED ;

	/* Increment open count */
	pp -> open_cnt++;
	
//	HFC_DBGPRT(" hfcldd%d : hfc_fx_open before UNLOCK\n",pp -> dev_minor);

	HFC_SEMAPHORE_UNLOCK(pp->sem) ;

//	HFC_EXIT("hfc_fx_open") ;
	return rtn;
}	


/*
 * Function:    hfc_fx_close
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_close( struct inode *inode, struct file *file ) {

	struct 	port_info	*pp;			/* A pointer to an adapter information structure */
	int		major;
	int		minor;
	int		rtn = 0;					/* A return code of a function */

//	HFC_ENTRY("hfc_fx_close") ;

	major = MAJOR(inode->i_rdev);
	minor = MINOR(inode->i_rdev);


	/* Find port_info from minor number */
	pp = hfc_manage_info.port_info_arg[ minor ];

	if( pp == NULL ) {
		/* No corresponding port_info -> error */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x00 );
		rtn = -EIO;
		return rtn;
	}

	if ( ( pp -> dev_major != major ) || ( pp -> dev_minor != minor ) ) {
		/* Major and minor number are unmatched with those stored in port_info. */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x01 );
		rtn = -EIO;
		return rtn;
	}

	if ( pp -> open_status != HFC_OPENED ) {
		/* Return error if adapter is not opened */
		HFC_DBGPRT("ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_CLOSE, 0x02 );
		rtn = -EIO;
		return rtn;
	}

	/* Acquire lock to handle port_info */
	if( HFC_SEMAPHORE_LOCK(pp->sem) ) 
		return -ERESTARTSYS ;
	
	if( pp->open_file == file )
	{
		clear_bit(HFC_FX_WRITE_PROCESS, (ulong *)&pp->ioctl_status) ;
		pp->open_file = NULL;
	}
	
	/* Decrement open count */
	pp -> open_cnt--;

	/* Make open_status close if open_cnt is zero */
	if ( pp -> open_cnt == 0 ) {
		pp -> open_status = HFC_CLOSED;
	}

	HFC_SEMAPHORE_UNLOCK(pp->sem) ;
	
//	HFC_EXIT("hfc_fx_close") ;
	
	return rtn;
}	
#endif

/*
 * Function:    hfc_fx_ioctl
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
 *	  OK /OK	SCSI ADpp_STAT	 -
 *	  OK /OK	SCSI PAYLOAD	 -
 *	  OK /OK	HBAAPI			 -
 */


#ifdef CONFIG_COMPAT
int hfc_fx_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg, int ioctl32 )
#else
int hfc_fx_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
#endif
{
	struct port_info	*pp=NULL;		/* Pointer to an port_info */
	int		major;
	int		minor;
	int		rtn = 0;					/* Return code of a function */
	int		rc = 0;

	uint	errlog_no ;
	uint	i, hit=0;
	struct	hfc_linux_ioctl_header *ioctl_header;

//	HFC_DBGPRT("hfc_fx_ioctl\n") ;

	if( _IOC_TYPE(cmd) != HFC_IOC_MAGIC ) 
	{
		HFC_ERRPRT("hfc_fx_ioctl: Command Check(Cmd=0x%x,Type=0x%x,Magic=0x%x) \n",
					cmd,_IOC_TYPE(cmd),HFC_IOC_MAGIC) ;
		HFC_ERRPRT("HFC_FNC_DIAG0=0x%x \n",(uint)HFC_FNC_DIAG0);
		return( -ENOTTY ) ;
	}
	/* kernel 5.0+: access_ok no longer takes type argument */
	if( !access_ok((void *)arg, _IOC_SIZE(cmd)) )
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

	/* Search port_info */
	for (i=0; i<MAX_ADAP_CNT; i++) {
		pp = hfc_manage_info.port_info_arg[i];
		if (pp != NULL) {
			if (pp->dev_minor == minor) {
				hit = 1;
				break;
			}
		}
	}

	if( ( pp == NULL ) || (hit == 0) ){
		/* No corresponding port_info -> error */
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
			HFC_TRC_IOCTL, 0x06 );
		rtn = -EINVAL;
		return rtn;
	}

	/* Major and minor number are unmatched with those stored in port_info. */
	if ( pp->dev_major != major ) {
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

	if( ( (cmd != HFC_FNC_DIAG0) && (cmd != HFC_FNC_SC_PRINFO) ) && test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) {	//FCLNX-0149
		/* Error if adapter is in recovery process except and not in diag operation */		//FCLNX-0149
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",			//FCLNX-0149
				HFC_TRC_IOCTL, 0x03 );														//FCLNX-0149
		rtn = -EIO;																			//FCLNX-0149
		return rtn;																			//FCLNX-0149
	}																						//FCLNX-0149

	if(!(test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) ) ) {							//FCLNX-0294
		if (!((cmd == HFC_FNC_ADP_ENABLE)
		   || (cmd == HFC_FNC_ADP_DISABLE)
		   || (cmd == HFC_FNC_SC_PRINFO)
		   || (cmd == HFC_FNC_DIAG0)
		   || (cmd == IOCTL_HFC_WWN_INFO))) {
			HFC_ERRPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",			//FCLNX-0294
				HFC_TRC_IOCTL, 0x04 );														//FCLNX-0294
			rtn = -EIO;																		//FCLNX-0294
			return rtn;																		//FCLNX-0294
		}
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
			if(pp->initialize != 0) {
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
	if( HFC_SEMAPHORE_LOCK(pp->sem) ) 
		return -ERESTARTSYS ;
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Set ioctl mode( ioctl32 or ioctl64 ) */
	pp->ioctl32 = ioctl32; /* FCLNX-GPL-234 */
#endif
	
	/* Handle each ioctl command */
	switch ( cmd ) {

	case HFC_FNC_SC_INQU:		/* Inquiry about SCSI device */
		rc = hfc_fx_inquiry( pp, (void *)arg );
		break;

	case HFC_FNC_SC_CMD:		/* Issue SCSI command directly */
		rc = hfc_fx_sciocmd( pp, (void *)arg, FALSE, 0 );			/* FCLNX-0223 */
		break;

	case HFC_FNC_SC_NMSRV:		/* Inquiry about all port IDs connected to a name server */
		rc = hfc_fx_get_all_port_ids( pp, (void *)arg );
		break;

	case HFC_FNC_SC_GWWN:		/* Inquiry about port ID with specific WWN to a name server */
		rc = hfc_fx_get_sid_from_wwpn( pp, (void *)arg );			/* FCLNX-0405 */
		break;

	case HFC_FNC_SC_GSID:		
		rc = hfc_fx_get_wwpn_from_sid( pp, (void *)arg );			/* FCLNX-0405 */
		break;

	case HFC_FNC_SC_PAYLD:
		rc = hfc_fx_payload( pp, (void *)arg );
		break;

	case HFC_FNC_SC_GRNID:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;
		rc = hfc_fx_get_rnid( pp, (void *)arg );
		break;

	case HFC_FNC_SC_SRNID:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;
		rc = hfc_fx_set_rnid( pp, (void *)arg );
		break;

	case HFC_FNC_SC_APSTAT:
		rc = hfc_fx_adap_stat( pp, (void *)arg );
		break;

	case HFC_FNC_SC_API:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )	goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fx_hba_api( pp, (void *)arg );
		break;

	case HFC_FNC_SC_TGTMAP:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fx_target_mapping( pp, (void *)arg );
		break;

#if 0
	case HFC_FNC_SC_PBIND:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fx_fcp_binding( pp, (void *)arg );
		break;
#endif

	case HFC_FNC_SC_SPT:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fx_support( pp, (void *)arg );
		break;
	case HFC_FNC_SC_PRINFO:
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0228 */
		rc = hfc_fx_procinfo( pp, (void *)arg );
		break;

	case HFC_FNC_SC_FC4STAT:												/* FCLNX-0404 */
//		if ( !(pp->fw_init_p->func & HFC_FWF_HBAAPI) )  goto CMDSKIP;		/* FCLNX-0404 */
		rc = hfc_fx_fc4stat( pp, (void *)arg );								/* FCLNX-0404 */
		break;																/* FCLNX-0404 */
#if 0
	case HFC_FNC_ADP_ENABLE:
		rc = hfc_fx_adapter_enable( pp, (void *)arg );
		break;
	case HFC_FNC_ADP_DISABLE:
		rc = hfc_fx_adapter_disable( pp, (void *)arg );
		break;
	case HFC_FNC_SCSI_SCAN:
		rc = hfc_fx_scsi_scan( pp, (void *)arg );
		break;
#endif

	case HFC_FNC_DIAG0:
		rc = hfc_fx_diag( (void *)arg, pp );
		break;
#if 0
	case IOCTL_HFC_WWN_INFO:
		rc = hfc_fx_wwn_info( pp, (void *)arg );
		break;
#endif
	case HFC_FNC_MP_TGTMAP:
		rc = hfc_fx_mp_target_map( pp, (void *)arg );
		break;	

	case HFC_FNC_READ_APPARAM:                      //FCLNX-0488
		rc = hfc_fx_read_apparam( pp, (void *)arg);    //FCLNX-0488
		break;                                          //FCLNX-0488

	case HFC_FNC_HBA_ISOLATION:								/* FCLNX-GPL-147 */
		rc = hfc_fx_isolate_operation( pp, (void *)arg );		/* FCLNX-GPL-147 */
		break;												/* FCLNX-GPL-147 */

	default:			/* An undefined IOCTL demand */
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			rc = hfc_manage_info.npubp->hfc_fx_ioctl_mp( pp, cmd, (void *)arg );
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
CMDSKIP:
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x, cmd=0x%04x) \n",
				   HFC_TRC_IOCTL, 0x05, cmd);
		errlog_no = (uchar)_IOC_NR(cmd) ;	/* FCLNX-0489	*/
		if( pp->raslog_install )
			HFC_INFPRT("hfcldd%d: HFC_ERR9 FC Adppter Driver error (ErrNo:0x68, ioctl_code = %d)\n", pp->dev_minor, errlog_no); 
		else
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x68, (uchar *)&errlog_no, 4) ;
		HFC_SEMAPHORE_UNLOCK(pp->sem) ;
		return ( -ENOTTY );
	}

	HFC_SEMAPHORE_UNLOCK(pp->sem) ;

//	HFC_EXIT("hfc_fx_ioctl") ;

	return -rc;
}	/* end of hfc_fx_ioctl */


/*
 * Function:    hfc_fx_adap_attr
 *
 * Purpose:     Handle an ioctl start demand from HBAAPI Benda library.
 *
 * Arguments:   
 *  pp     - Pointer to an port_info 
 *  arg    - Pointer to a data area
 *
 * Returns:     
 *  0      - Normal end 
 *  EFAULT - Failed to copy or attach data
 *  EINVAL - Invalid parameters
 *  ENODEV - No device or target
 *
 * Notes:       
 *  It is called by hfc_fx_ioctl()(HFCHBAAPI)
 */
int hfc_fx_adap_attr( struct port_info *pp, struct hfc_ioctl_api *api ) {

	struct region_info			*rp;
	struct core_info			*core;
	struct hfc_ioctl_adap_attr	*adap_attr;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	uint	hw_data[2];
	
	HFC_ENTRY("HFC_ADAP_attr");
	HFC_DBGPRT("HFC_ADAP_attr");

	adap_attr = (struct hfc_ioctl_adap_attr *)hfc_fx_kmalloc(pp, sizeof(struct hfc_ioctl_adap_attr), GFP_ATOMIC);	/* FCLNX_GPL-147 */
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
		hfc_fx_kfree(pp, adap_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	
	rp = pp->region_arg[pp->rid];
	core = rp->core_arg[pp->master_core_no];
	
	/* Set each attribute value about an adapter */
	strncpy( adap_attr->manufacturer, "HITACHI", HFC_MANUFACTURE_SIZE);
	strncpy( adap_attr->serialnumber, "00000000", HFC_SERIALNUMBER_SIZE);
	adap_attr->node_wwn = pp->node_name ;
	memset( adap_attr->node_symbolic_name, 0, HFC_NODE_SYMBOL_SIZE);
	strncpy( adap_attr->driver_version, hfc_manage_info.package_ver, HFC_DD_VER_SIZE);
	memset( adap_attr->option_rom_version, 0, HFC_ROM_VER_SIZE);
	sprintf( adap_attr->firmware_version, "%08x", hfc_fx_get_sysrev(core) ); /* FCLNX-GPL-112 */
	adap_attr->vendor_specific_id = 0;

	adap_attr->number_of_ports = (MAX_CORE_PROBE_FX/pp->core_num);

	/* Read H/W information (8 bytes from address 0x000 ) */
	hw_data[0] = hfc_fx_read_reg(pp, HFC_IOSPACE_ZERO, 0x4);
	hw_data[1] = hfc_fx_read_reg(pp, HFC_IOSPACE_OFS4, 0x4);

	/* Set H/W version */
	sprintf( adap_attr->hardware_version, "%08x", hw_data[0] );
	sprintf( &adap_attr->hardware_version[8], "%08x", hw_data[1] );

	/* Setup data from an internal data area into hfc_ioctl_adap_attr structure */
	if ( COPYOUT( (char *)adap_attr, (char *)(ulong)api->addr, sizeof(struct hfc_ioctl_adap_attr) ) != 0 )
	{
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_APATTR, 0x01 );
		hfc_fx_kfree(pp, adap_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	
	hfc_fx_kfree(pp, adap_attr);																/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_EXIT("HFC_ADAP_attr");

	return (0);
}


/*
 * Function:    hfc_fx_port_attr
 *
 * Purpose:     Issue ioctl from HBAAPI vendor library
 *
 * Arguments:   
 *  pp 			- Pointer to port_info 
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
 *  			- This function is called by hfc_fx_ioctl()(HFCHBAAPI)
 */

int hfc_fx_port_attr( struct port_info *pp, struct hfc_ioctl_api *api ) {

	struct hfc_ioctl_port_attr *port_attr;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	struct target_info_fx *target;				/* target_info_fx for this device	    */

	uint	wk_data;
	uint	discport_num = 0 ;
	uint	i;
	ulong	flags = 0 ;

	HFC_ENTRY("hfc_fx_port_attr");
	HFC_DBGPRT("hfc_fx_port_attr");

	port_attr = (struct hfc_ioctl_port_attr *)hfc_fx_kmalloc(pp, sizeof(struct hfc_ioctl_port_attr), GFP_ATOMIC);	/* FCLNX_GPL-147 */
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
		hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}

	if ( port_attr->flags == HFC_GET_ADAP_PORT ) 
	{		/* GetAdppterPortAttributes */

		/* Set each attribute value about an adapter */
		port_attr->node_wwn = pp->node_name ;
		port_attr->port_wwn = pp->ww_name ;
		port_attr->port_fcid = (uint)(pp->scsi_id & 0x00ffffff);
		port_attr->port_support_class_of_service = HFC_SUPPORT_CLASS ;	/* Class3 */
		memset( port_attr->port_symbol_name, 0, HFC_NODE_SYMBOL_SIZE);
		port_attr->port_max_frame_size = HFC_PORT_MAX_FRAME ;

		/* Set port type */
		if( pp->connect_type == HFC_FX_PT2PT ){			/* PtoP & NotSwitch */
			port_attr->port_type = HFC_PORTTYPE_PTP ;
		}
		else if ( pp->connect_type == HFC_FX_SWITCH ){		/* PtoP & Switch */
			port_attr->port_type = HFC_PORTTYPE_NPORT ;
		}
		else if ( pp->connect_type == HFC_FX_AL ){
			if ( pp -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
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
		if( test_bit(HFC_PS_ISOL , (ulong *)&pp->status) )
			port_attr->port_state = HFC_PORTSTATE_ERROR ;
		else if( !test_bit(HFC_PS_ONLINE , (ulong *)&pp->status) )
			port_attr->port_state = HFC_PORTSTATE_LINKDOWN ;
		else if( test_bit(HFC_PS_WAIT_LINKUP,(ulong *)&pp->status )||test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) )	//FCLNX-0160 FCLNX-GPL-FX-005
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
		if( pp->pkg.type == HFC_PKTYPE_FPP )
			port_attr->port_supported_speed = HFC_PORTSPEED_2GBIT ;
		else if( pp->pkg.type == HFC_PKTYPE_FIVE )
			port_attr->port_supported_speed = HFC_PORTSPEED_4GBIT ;
		else if( pp->pkg.type == HFC_PKTYPE_FIVE_EX )/* FIVE-EX */
			port_attr->port_supported_speed = HFC_PORTSPEED_8GBIT ;
		else
			port_attr->port_supported_speed = HFC_PORTSPEED_16GBIT ;

		/* Port speed */			
		if( pp->max_data_rate == HFC_100MBS )
			port_attr->port_speed = HFC_PORTSPEED_1GBIT ;
		else if( pp->max_data_rate == HFC_200MBS )
			port_attr->port_speed = HFC_PORTSPEED_2GBIT ;
		else if( pp->max_data_rate == HFC_400MBS )
			port_attr->port_speed = HFC_PORTSPEED_4GBIT ;
		else if( pp->max_data_rate == HFC_800MBS )
			port_attr->port_speed = HFC_PORTSPEED_8GBIT ;
		else if( pp->max_data_rate == HFC_1000MBS )
			port_attr->port_speed = HFC_PORTSPEED_10GBIT ;
		else if( pp->max_data_rate == HFC_1600MBS )
			port_attr->port_speed = HFC_PORTSPEED_16GBIT ;
		else
			port_attr->port_speed = HFC_PORTSPEED_UNKNOWN ;

		/* Search target_info_fx and report number of DiscPort and ww_name */
		for (i = 0; i < MAX_TARGET_PROBE; i++) 
		{
			target = hfc_fx_hash_target_info(pp, i); /* check HFC_TARGETINF_VALID & HFC_TF_DEVFLG_VALID & HFC_TF_WWN_VALID */

			if (target != NULL) 												//FCLNX-0151
			{																	//FCLNX-0151
				if( (test_bit(HFC_PS_ONLINE,     (ulong *)&pp->status))			//FCLNX-0151
				  && (!test_bit(HFC_PS_WAIT_LINKUP,(ulong *)&pp->status ))			//FCLNX-0151
				  && (!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))	//FCLNX-GPL-FX-005
				  && (!test_bit(HFC_TS_SCN_WLINKUP,(ulong *)&target->status)) )	//FCLNX-0151
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
		HFC_PORTLOCK_IRQSAVE(pp,flags);
		/* Is designated parameter SCSI_ID or ww_name ? */
		/*   SCSI_ID -> Identify target_info_fx from target_arg[] by SCSI_ID */
		/*   ww_name -> Identify target_info_fx from target_arg[] by WW_NAME */
		if( api->valid & HFC_API_SCSI_ID_VALID ){
			if ( ( target = hfc_fx_hash_target_info( pp, api->scsi_id ) ) == NULL ) {
				/* Return target_info_fx. Return error if target_info_fx is not found */
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_SC_PTATTR, 0x01 );
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				return ENODEV ;
			}
		}
		else if( api->valid & HFC_API_WWN_VALID ){
			if ( ( target = hfc_fx_hash_target_info_wwn( pp, api->world_wide_name ) ) == NULL ) {
				/* Return target_info_fx. Return error if target_info_fx is not found */
				HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
						HFC_TRC_IOCTL_SC_PTATTR, 0x02 );
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
				return ENODEV ;
			}
		}
		else {
			HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_SC_PTATTR, 0x03 );
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
			return EINVAL;
		}
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

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
		if( pp->connect_type == HFC_FX_PT2PT ){			/* PtoP & NotSwitch */
			port_attr->port_type = HFC_PORTTYPE_PTP ;
		}
		else if ( pp->connect_type == HFC_FX_SWITCH ){		/* PtoP & Switch */
			port_attr->port_type = HFC_PORTTYPE_NPORT ;
		}
		else if ( pp->connect_type == HFC_FX_AL ){
			if ( pp -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
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
		hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EINVAL;
	}

	/* Setup data from an internal data area into hfc_ioctl_adap_attr structure */
	if ( COPYOUT( (char *)port_attr, (char *)(ulong)api->addr, sizeof(struct hfc_ioctl_port_attr) ) != 0 ){
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_PTATTR, 0x05 );
		hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}

	hfc_fx_kfree(pp, port_attr);															/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	HFC_EXIT("hfc_fx_port_attr") ;

	return (0);
}



/*
 * Function:    hfc_fx_hba_api
 *
 * Purpose:     Issue ioctl from HBAAPI vendor library
 *
 * Arguments:   
 *  pp 			- Pointer to port_info 
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
 *  			- This function is called by hfc_fx_ioctl()(HFCHBAAPI)
 */
int hfc_fx_hba_api( struct port_info *pp, void *arg ) {

	struct hfc_ioctl_api api;

	int		rc = 0;
	int		rtn = 0;

	HFC_ENTRY("hfc_fx_hba_api");		

	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if ( COPYIN( (char *)arg, (char *)&api, sizeof(struct hfc_ioctl_api) ) != 0 ){
		HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
				HFC_TRC_IOCTL_SC_API, 0x00 );
		return EFAULT;
	}

#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(pp->ioctl32) {
		api.addr = api.addr & 0xffffffffU;
	}
#endif
	
	/* Handle each command */
	switch ( api.sub_cmd ) 
	{
		case HFC_GET_ADAPTER_ATTR:		/* GetAdppterAttributes */
			rtn = hfc_fx_adap_attr( pp, (void *)&api ) ;
			break;

		case HFC_GET_PORT_ATTR:
			rtn = hfc_fx_port_attr( pp, (void *)&api ) ;
			break;

		default:
			HFC_DBGPRT("HFCLDD(IOCTL): ioctl error(trcid=0x%04x, subid=0x%04x) \n",
					HFC_TRC_IOCTL_SC_API, 0x01 );
			return EINVAL;
	}

	/* Return error when HFC_ADAP_attr or hfc_fx_port_attr returns error */
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

	HFC_EXIT("hfc_fx_hba_api");
	/* Set return code */
	if ( rtn == 0 ) rtn = rc;
	return rtn;
}


/*
 * Function:    hfc_fx_ioctl_iodone
 *
 * Purpose:     Completes SCSI command issued by IOCTL
 *
 * Arguments:   cmnd -  A pointer to a struct scsi_cmnd 
 *
 * Returns:     none
 *
 * Notes:       
 */
void hfc_fx_ioctl_iodone( struct scsi_cmnd *cmnd ) {
	struct port_info *pp;

	pp = (struct port_info *)CMND_HOSTDATA(cmnd);
	
	set_bit(HFC_IOCTL_BUSY, (ulong *)&pp->ioctl_lock);
	hfc_fx_wake_up(&pp->ioctl_event, &pp->ioctl_event_wait);				/* FCLNX-0296 */

	return;
} 

/*
 * Function:    hfc_fx_ioctl_sleep
 *
 * Purpose:     Wait to complete IOCTL command
 *
 * Arguments:   cmnd - A pointer to a struct scsi_cmnd structure
 *
 * Returns:     none
 *
 * Notes:       
 */
void hfc_fx_ioctl_sleep( struct scsi_cmnd *cmnd ) {
	struct port_info *pp;

	pp = (struct port_info *)CMND_HOSTDATA(cmnd);
	hfc_fx_sleep_on(&pp->ioctl_event, &pp->ioctl_event_wait);				/* FCLNX-0296 */
	clear_bit(HFC_IOCTL_BUSY, (ulong *)&pp->ioctl_lock);

	return;
} 


/*
 * Function:    hfc_fx_rtn_scsi_cmnd
 *
 * Purpose:     Read data from struct scsi_cmnd 
 *
 * Arguments:   
 *  cmnd        - A pointer to a scsi_cmnd 
 *  arg         - A pointer to data area
 *                (Scsi_status of hfc_fx_ioctl_inquiry or hfc_fx_ioctl_cdb)
 *  data_length - Data length of inquiry data (return data)
 *
 * Returns:     
 *   0         		  - Normal end
 *   Other than zero  - Error end
 *
 * Notes:       
 */
int hfc_fx_rtn_scsi_cmnd( struct scsi_cmnd *cmnd, void *arg, uint64_t *buffer, int type ) {
	int   rtn = 0;                    /* A return code of this function */

	struct wrk_hfc_fx_ioctl {
		uchar    scsi_status;
		uchar    resid_flags;
		ushort   resid;
		uchar    rsv2[3];
		uchar    adapter_status;
		uint64_t sense_ptr;
		uchar    rsv3[2];
		ushort   sense_len;
	};
	struct     wrk_hfc_fx_ioctl   *wrk;
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
 * Function:    hfc_fx_inquiry
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
 * Notes:       It is called by hfc_fx_ioctl()
 */
int hfc_fx_inquiry( struct port_info *pp, void *arg ) {
	int    RC  = 0;				/* return code  */

	struct hfc_ioctl_inquiry inquiry;		/* hfc_fx_ioctl_inquiry structure */
	struct scsi_cmnd          *cmnd;		/* A pointer to a struct scsi_cmnd structure */
	struct target_info_fx *target;             /* A pointer to a target_info_fx structure */
	ulong              flags = 0;           /* for spin_lock/unlock */
	static uint        cnt   = 0;           /* for serial_number    */
#define MAX_CNT            0xffffffff
	ushort             data_length = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	int					data_buf_num = 0, i;
	struct scatterlist	*sgl=NULL, *pre_sgl=NULL;
	struct dev_info_fx		*dev=NULL;
#else
	int                cmnd_device_alloc_flag =0;	//FCLNX-0147
#endif												/* FCLNX-GPL-0343 */

	HFC_ENTRY("hfc_fx_inquiry") ;
	
	/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
	if( COPYIN( (char *)arg, (char *)&inquiry, sizeof(struct hfc_ioctl_inquiry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x00 );
		return EFAULT;
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(pp->ioctl32) {
		inquiry.buffer    = inquiry.buffer    & 0xffffffffU;
		inquiry.sense_ptr = inquiry.sense_ptr & 0xffffffffU;
	}
#endif
	
//	STRUCTDUMP( DMP_INQU, (uchar *)&inquiry, sizeof(struct hfc_ioctl_inquiry) );

	HFC_PORTLOCK_IRQSAVE(pp,flags);
	if ( (( target = hfc_fx_hash_target_info_wwn( pp, inquiry.Port_WWN ) ) == NULL )
	  || (inquiry.lun_id >= MAX_DEV_CNT) ) {
		/* Return target_info_fx if found. Error return if target_info_fx is not found */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x01 );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return ENODEV ;
	}
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	/* Make sure whether an inquiry data area is set */
	if( (inquiry.data_length==0) || (inquiry.buffer==0) ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x02 );
		return EINVAL;
	}

	cmnd = pp->ioctl_cmnd;
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
	if( ( cmnd = (struct scsi_cmnd *)hfc_fx_kmalloc(pp, sizeof(struct scsi_cmnd), GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x03 );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd, 0, sizeof(struct scsi_cmnd) );

	/* Setting of a struct scsi_cmnd structure */
	cmnd_device_alloc_flag = 0;
	if ( (cmnd->device = (struct scsi_device*)hfc_fx_kmalloc(pp, sizeof(struct scsi_device), GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate scsi_cmnd */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x07 );
		hfc_fx_kfree(pp, cmnd );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd->device, 0, sizeof(struct scsi_device) );
	cmnd_device_alloc_flag = 1;
#endif		/* FCLNX-GPL-0343 */

	cmnd->device->host = pp->hosts;

	/* kernel 5.15+: scsi_cmnd->serial_number removed; field unused */
	(void)cnt; cnt=(cnt+1)%MAX_CNT;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)			/* FCLNX-GPL-0343 */
	dev = (struct dev_info_fx *)hfc_fx_get_dev_info_fx(target, inquiry.lun_id);

	if( dev == NULL ){
		dev = pp->ioctl_dev;
		memset( dev, 0, sizeof(struct dev_info_fx) );
		dev->lun = (uint)inquiry.lun_id;
		dev->target_id = target ->  target_id;
		set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-333 Start */
		if(hfc_manage_info.npubp->hfc_fx_check_issuing_io_reset(target, dev)){
			HFC_ERRPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x10 );
			return EAGAIN;
		}
	}	/* FCLNX-GPL-FX-333 End */
	
	cmnd -> device -> hostdata = (struct dev_info_fx *)dev;

	cmnd -> device -> request_queue -> rq_timeout = pp->ioctl_scsi_timeout * HZ;	/* FCLNX-GPL-0368 */
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
	if( ( cmnd->request_buffer  = hfc_fx_kmalloc(pp, cmnd -> request_bufflen, GFP_ATOMIC) )==NULL ) {
		/* Return error if failed to allocate buffer area  */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x04 );
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_fx_kfree(pp, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd );
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


	RC = HFC_FX_STRATEGY( cmnd, (void *) hfc_fx_ioctl_iodone );
	/* Need to sleep here. (iodone is executed even if error occurred in hfc_fx_strategy)  */
	hfc_fx_ioctl_sleep( ( struct scsi_cmnd * )cmnd );

	if( RC ) {
		/* When an error occurred in strategy() */
		HFC_DBGPRT("When an error occurred in strategy()");
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_fx_kfree(pp, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd->request_buffer );
		hfc_fx_kfree(pp, cmnd );
#endif			/* FCLNX-GPL-0343 */
		return EIO;
	}
//	STRUCTDUMP( DMP_SCSICMD, (uchar *)cmnd    , sizeof(struct scsi_cmnd) );

	RC = hfc_fx_rtn_scsi_cmnd( cmnd, &inquiry.scsi_status, (uint64_t *)(ulong)inquiry.buffer, TRUE );
	if( RC==EFAULT ) {
		/* Write back failed for sense_buffer or request_buffer */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x05 );
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_fx_kfree(pp, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd->request_buffer );
		hfc_fx_kfree(pp, cmnd );
#endif			/* FCLNX-GPL-0343 */
		return EFAULT;
	}

	/* Write back data into hfc_fx_ioctl_inquiry structure */
	if( COPYOUT( (char *)&inquiry, (char *)arg, sizeof(struct hfc_ioctl_inquiry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x06 );
		RC = EFAULT;
	}
//	STRUCTDUMP( DMP_INQU, (uchar *)&inquiry, sizeof(struct hfc_ioctl_inquiry) );

#if 0			/* FCLNX-GPL-0343 */
	if ( cmnd_device_alloc_flag == 1 ) {		//FCLNX-0147 FCLNX-0423
		hfc_fx_kfree(pp, cmnd->device );					//FCLNX-0147 FCLNX-0423
	}											//FCLNX-0147 FCLNX-0423
	hfc_fx_kfree(pp, cmnd->request_buffer );				//FCLNX-0423
	hfc_fx_kfree(pp, cmnd );								//FCLNX-0423
#endif			/* FCLNX-GPL-0343 */

	return RC;
}	/* end of hfc_fx_inquiry */


/*
 * Function:    hfc_fx_sciocmd
 *
 * Purpose:     Publish the SCSI command that a user pppointed
 *
 * Arguments:   
 *  pp        - A pointer to an port_info structure
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
 *              This function is called by hfc_fx_ioctl() (SCIOLCMD)
 */
int hfc_fx_sciocmd( struct port_info *pp, void *arg, int internal, int timeout) {
	int RC = 0;				/* return code */

	struct hfc_ioctl_cdb cdb;				/* hfc_fx_ioctl_cdb structure            */
	struct scsi_cmnd   *cmnd;				/* A pointer to a struct scsi_cmnd structure    */
	struct target_info_fx *target;				/* A pointer to a target_info_fx structure  */
	ulong              flags = 0;			/* for spin_lock/unlock           */
#define MAX_CNT            0xffffffff

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	int					data_buf_num = 0, i;
	struct scatterlist	*sgl=NULL, *pre_sgl=NULL;
	struct dev_info_fx		*dev = NULL;
#else
	int                cmnd_device_alloc_flag =0;	//FCLNX-0147
#endif
	ulong				page_link_addr[HFC_SCATTERLIST_NUM]={0};	/* FCLNX-GPL-FX-473 */

	HFC_ENTRY("hfc_fx_sciocmd");

	if (internal != TRUE) {
		/* Copy data to an internal data area as hfc_ioctl_adap_attr structure */
		if ( COPYIN( (char *)arg, (char *)&cdb, sizeof(struct hfc_ioctl_cdb) ) != 0 ) {
		  HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x00 );
		  return EFAULT;
		}
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		/* Lower 32bit is effective for pointers */
		if(pp->ioctl32) {
			cdb.buffer    = cdb.buffer    & 0xffffffffU;
			cdb.sense_ptr = cdb.sense_ptr & 0xffffffffU;
		}
#endif
		HFC_PORTLOCK_IRQSAVE(pp,flags);
	}
	else {
		cdb = *((struct hfc_ioctl_cdb *) arg);
	}

	target = hfc_fx_hash_target_info_wwn( pp, cdb.Port_WWN );

	if (internal != TRUE)
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	if (target == NULL || (cdb.lun_id >= MAX_DEV_CNT)) {
		/* Return target_info_fx if found. Error return if target_info_fx is not found */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x01 );
		return ENODEV ;
	}
	
//	STRUCTDUMP( DMP_IOCMD, (uchar *)&cdb, sizeof(struct hfc_ioctl_cdb) );
	
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

	if( (cdb.data_length==0) || (cdb.data_length>pp->dma_max) || (cdb.buffer==0) ) {
		/* Transfer data length is longer than pp->dma_max */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x03 );
		return EINVAL;
	}

	cmnd = pp->ioctl_cmnd;
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
	if( ( cmnd = (struct scsi_cmnd *)hfc_fx_kmalloc(pp, sizeof(struct scsi_cmnd), GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate scsi_cmnd */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x04 );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd, 0, sizeof(struct scsi_cmnd) );

	/* Setting of a struct scsi_cmnd structure */
	cmnd_device_alloc_flag = 0;
	if ( (cmnd->device = (struct scsi_device*)hfc_fx_kmalloc(pp, sizeof(struct scsi_device), GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate cmnd->device */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x08 );
		hfc_fx_kfree(pp, cmnd );
		return ENOMEM;
	}
	memset( (struct scsi_cmnd *)cmnd->device, 0, sizeof(struct scsi_device) );
	cmnd_device_alloc_flag = 1;
#endif

	cmnd->device->host = pp->hosts;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	dev = (struct dev_info_fx *)hfc_fx_get_dev_info_fx(target, cdb.lun_id);

	if( dev == NULL ){
		dev = pp->ioctl_dev;
		memset( dev, 0, sizeof(struct dev_info_fx) );
		dev->lun = (uint)cdb.lun_id;
		dev->target_id = target ->  target_id;
		set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){	/* FCLNX-GPL-FX-333 Start */
		if(hfc_manage_info.npubp->hfc_fx_check_issuing_io_reset(target, dev)){
			HFC_ERRPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_INQ, 0x10 );
			return EAGAIN;
		}
	}	/* FCLNX-GPL-FX-333 End */
	
	cmnd -> device -> hostdata = (struct dev_info_fx *)dev;

	cmnd -> device -> request_queue -> rq_timeout = pp->ioctl_scsi_timeout * HZ;	/* FCLNX-GPL-0368 */
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
	if( ( cmnd->request_buffer  = hfc_fx_kmalloc(pp, cdb.data_length, GFP_ATOMIC) )==NULL ) {
		/* Error if failed to allocate request_buffer area */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x05 );
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
			hfc_fx_kfree(pp, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd );
		return ENOMEM;
	}
	memset( cmnd->request_buffer, 0, cdb.data_length );
#else
	if( ( cmnd->sense_buffer  = hfc_fx_kmalloc(pp, SCSI_SENSE_BUFFERSIZE, GFP_ATOMIC) )==NULL ) {		/* FCLNX-GPL-396 */
		/* Error if failed to allocate request_buffer area */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x09 );
		hfc_fx_kfree(pp, cmnd->sense_buffer );					//FCLNX-0423
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
	/* kernel 5.4+: scsi_cmnd->tag removed; tag_q field ignored */

	RC = HFC_FX_STRATEGY( cmnd, (void *) hfc_fx_ioctl_iodone );
	/* Need to sleep here. (iodone is executed even if error occurred in hfc_fx_strategy)  */
	hfc_fx_ioctl_sleep( ( struct scsi_cmnd * )cmnd );
	if( RC ) {
		/* When an error occurred in strategy() */
#if 0			/* FCLNX-GPL-0343 */
		if ( cmnd_device_alloc_flag == 1 ) {	//FCLNX-0147
		hfc_fx_kfree(pp, cmnd->device );					//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd->request_buffer );
		hfc_fx_kfree(pp, cmnd );
#endif
		hfc_fx_kfree(pp, cmnd->sense_buffer );		/* FCLNX-GPL-396 */
		return EIO;
	}
//	STRUCTDUMP( DMP_SCSICMD, (uchar *)cmnd    , sizeof(struct scsi_cmnd) );

	RC = hfc_fx_rtn_scsi_cmnd( cmnd, &cdb.scsi_status, (uint64_t *)(ulong)cdb.buffer,
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
			hfc_fx_kfree(pp, cmnd->device );				//FCLNX-0147
		}										//FCLNX-0147
		hfc_fx_kfree(pp, cmnd->request_buffer );
		hfc_fx_kfree(pp, cmnd );
#endif
		hfc_fx_kfree(pp, cmnd->sense_buffer );		/* FCLNX-GPL-396 */
		return EFAULT;
	}
	
	if (( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
		&& (cmnd->cmnd[0] == 0xA0) ) {
			if(!(hfc_manage_info.npubp->hfc_fx_check_issuing_io_reset(target, dev))){	/* FCLNX-GPL-FX-333 Start */
				hfc_manage_info.npubp->hfc_fx_check_luconfig(target, cmnd);
			}	/* FCLNX-GPL-FX-333 End */
	}																		/* FCLNX-GPL-449 */
	
	if (internal != TRUE) {						//FCLNX-GPL-0324,0329
		/* Write back data to hfc_fx_ioctl_cdb */
		if( COPYOUT( (char *)&cdb, (char *)arg, sizeof(struct hfc_ioctl_cdb) ) != 0 ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SCICMD, 0x07 );
			RC =EFAULT;
		}
	}
	else {
		*((struct hfc_ioctl_cdb *) arg) = cdb;
	}

//	STRUCTDUMP( DMP_IOCMD, (uchar *)&cdb, sizeof(struct hfc_ioctl_cdb) );

#if 0				/* FCLNX-GPL-0343 */
	if ( cmnd_device_alloc_flag == 1 ) {		//FCLNX-0147 FCLNX-0423
		hfc_fx_kfree(pp, cmnd->device );					//FCLNX-0147 FCLNX-0423
	}											//FCLNX-0147 FCLNX-0423
	hfc_fx_kfree(pp, cmnd->request_buffer );				//FCLNX-0423
	hfc_fx_kfree(pp, cmnd );								//FCLNX-0423
#endif
	hfc_fx_kfree(pp, cmnd->sense_buffer );		/* FCLNX-GPL-396 */

	return RC;
}  


/*
 * Function:    hfc_fx_get_all_port_ids
 *
 * Purpose:     Get all port IDs (SCSI_ID) connected to name server
 *
 * Arguments:   
 *  pp        - A pointer to an port_info structure
 *  arg       - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area.
 *  EINVAL    - Adppter is not connected to fabric, or is not in Public AL.
 *              Setup error in port list area.
 *              Setup of CT_IU is error 
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred. *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLNMSRV)
 */
int hfc_fx_get_all_port_ids( struct port_info *pp, void *arg ) {
	struct hfc_ioctl_gidft		gidft;
	struct send_payload			*payload;
	struct receive_payload		*response;
	struct core_info			*core;
	
	dma_addr_t	payload_busaddr;		/* bus address of payload */
	dma_addr_t	response_busaddr;		/* bus address of response */
	int			rtn = 0;
	uint		scsi_id;
	uint		sid;
	uchar		id=0;
	ushort		portnum_by_rcvlen=0;
	uchar		ctl=0;
	uint		i,port_num=0;
	uchar		logdata[16];
	ulong		flags = 0;			/* FCLNX-GPL-FX-353 */

	HFC_ENTRY("hfc_get_all_port_ids");
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	if( !test_bit( HFC_PS_ONLINE, (ulong *)&pp->status ) ) {
		/* Adapter is not online */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x00 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return EIO;
	}
	
	if( !(( pp->connect_type==HFC_FX_SWITCH ) || ( (pp->connect_type==HFC_FX_AL) && (pp->scsi_id & 0x00ffff00) )) ) {
		/* Error if adapter is not connected to fabric directly, or not in public AL */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x01 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return EINVAL;
	}
	
	/* Copy data to an internal data area as hfc_ioctl_gidft structure */
	if( COPYIN( (char *)arg, (char *)&gidft, sizeof(struct hfc_ioctl_gidft) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x02 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return EFAULT;
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(pp->ioctl32) {
		gidft.scsi_id_list = gidft.scsi_id_list & 0xffffffffU;
	}
#endif
	
	STRUCTDUMP( DMP_GIDFT, (uchar *)&gidft, sizeof(struct hfc_ioctl_gidft) );
	
	if( (gidft.list_len<4) || (gidft.list_len>4064) || (gidft.scsi_id_list==0) ) {
		/* Make sure pointer to SCSI_ID data area and length for SCSI_ID is correct */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x03 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return EINVAL;
	}
	
	/* Clear status */
	gidft.adapter_status = 0x00;
	gidft.set_flags      = 0x00;
	gidft.scsi_id_size   = 0x00;
	gidft.num_ids        = 0;
	
	
	/*
	 * Allocate payload area
	 */
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	payload  = (struct send_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), &payload_busaddr );
	if( payload == NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x04 );
		logdata[0] = 0x80;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)payload & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x05 );
		logdata[0] = 0x81;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)payload,  (uint)sizeof(struct send_payload) );


	/*
	 * Allocate response area
	 */
	response = (struct receive_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), &response_busaddr  );
	if(response == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x06 );
		logdata[0] = 0x82;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)response & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x07 );
		logdata[0] = 0x83;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)response, (uint)sizeof(struct receive_payload));
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	/* Lock mailbox */
	if ( !(lock_fx_try_mailbox( pp )) ) {
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x08 );
		return EIO;
	}
	
	core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	
	/* Create a mailbox control block based on FRMSNDRCV */
	
	/* Mailbox Request Header */
	hfc_fx_write_val(core->mb->mb_init.mb_code, HFC_MBCMD_SNDRCV_GIDFT);
	hfc_fx_write_val(core->mb->mb_init.timer, pp->mb_timer[ HFC_MBTIME_GID_FT ].tout-1 );
	
	/* Common Control */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.frame_ctl, 0);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload_length, HFC_GIDFT_SLENGTH);
	
	/* VFT_Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 0x800);
	
	/* FC-PH Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, 0);
	sid = (uint)(pp->scsi_id & 0x00ffffff);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[2], id);
	
	/* Payload Address, Receive Payload Address */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload, (uint64_t)payload_busaddr);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.receive_payload, (uint64_t)response_busaddr);
	
	/* Make payload */
	hfc_fx_write_val( payload->data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( payload->data0[4], 0xfc);
	hfc_fx_write_val( payload->data0[5], 0x02);
	hfc_fx_write_val( payload->type.gxx.data1[0], 0x01);
	hfc_fx_write_val( payload->type.gxx.data1[1], 0x71);
	hfc_fx_write_val( payload->type.gxx.data1[2], 0x01);
	hfc_fx_write_val( payload->type.gxx.sub_type.gid_ft.data2[3], 0x08);
	
	STRUCTDUMP( DMP_PAYLOAD,  (uchar *)payload,  sizeof(struct send_payload) );
	
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
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	rtn = hfc_fx_mailbox_proc(pp, core, HFC_FX_MB_RSP_TMR,
		pp->mb_timer[HFC_MBTIME_GID_FT].tout, pp->mb_timer[HFC_MBTIME_GID_FT].retry);
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	if( rtn == 0 ) {
		STRUCTDUMP( DMP_RESPONSE, (uchar *)response, sizeof(struct receive_payload) );
		
		portnum_by_rcvlen = (ushort)hfc_fx_read_val(core->mb->mb_resp.type.frmsndrcv.recv_payload_length) ;
		portnum_by_rcvlen -= 0x10;
		portnum_by_rcvlen /= 4;
		
		/* Count device number */
		for(i=0 ; i < HFC_FX_MAX_PORTID ; i++){
			ctl = hfc_fx_read_val(response->type.gxx.sub_type.gid_ft.portid[i].ctl);
			port_num++;
			if ( ctl & 0x80 ) {
				/*---- Detect last Device on PortID Field of GID_FT Response -----*/
				break;
			}
			if (port_num >= portnum_by_rcvlen) {
				break;
			}
		}
		
		if (port_num) {
			if ( port_num > MAX_TARGET_PROBE ) port_num = MAX_TARGET_PROBE;
		}
		HFC_DBGPRT("hfc_fx_get_all_port_ids() port_num = %d\n",port_num);
		
		gidft.scsi_id_size = 4;
		gidft.num_ids = port_num;
		
		for (i=0;i<port_num;i++) {
			scsi_id = 0;
			scsi_id = (uint)(hfc_fx_read_val(response->type.gxx.sub_type.gid_ft.portid[i].port_id[0]) << 16 ) +
				(uint)(hfc_fx_read_val(response->type.gxx.sub_type.gid_ft.portid[i].port_id[1]) << 8 ) +
				(uint)(hfc_fx_read_val(response->type.gxx.sub_type.gid_ft.portid[i].port_id[2]) );
			
			HFC_DBGPRT("hfc_fx_get_all_port_ids() port_id[%d] = %08x\n",i, scsi_id);
			
			scsi_id &= 0x00ffffff;
			
			if( COPYOUT( (char *)&scsi_id, (char *)(ulong)(gidft.scsi_id_list+i*4), 4 ) != 0 ) {
				HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x09 );
				rtn = EFAULT;
				break;
			}
		}
	}
	
	unlock_fx_mailbox(pp);   /* Unlock mailbox */
	
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
	/* FCLNX-GPL-FX-353 End */
	
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);

	/* Write-back data to hfc_ioctl_gidft structure  */
	if( COPYOUT( (char *)&gidft, (char *)arg, sizeof( struct hfc_ioctl_gidft ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GIDFT, 0x0a );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_GIDFT, (uchar *)&gidft, sizeof(struct hfc_ioctl_gidft) );
	
	HFC_EXIT("hfc_get_all_port_ids");
	
	return rtn;
}


/*
 * Function:    hfc_fx_get_sid_from_wwpn
 *
 * Purpose:     Search port ID(SCSI_ID) with specific WWN
 *
 * Arguments:   
 *  pp        - Pointer to an port_info structure 
 *  arg       - A pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area.
 *  EINVAL    - Adppter is not connected to fabric, or is not in Public AL.
 *              Setup error in port list area.
 *  ENOMEM    - Lack of required memory
 *  ENODEV    - Device does not reply (No device)
 *  EBUSY     - Device busy
 *  ETIMEDOUT - Timeout
 *  EIO       - Error occured in strategy() or other error occurred. 
 *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLQWWN)
 */
int hfc_fx_get_sid_from_wwpn( struct port_info *pp, void *arg ) {
	struct hfc_ioctl_gidpn		get_sid;
	struct send_payload			*payload;
	struct receive_payload		*response;
	struct target_info_fx		*target;
	struct core_info			*core;
	struct region_info			*rp;
	
	dma_addr_t	payload_busaddr;		/* bus address of payload */
	dma_addr_t	response_busaddr;		/* bus address of response */
	int			rtn = 0;
	uint		sid;
	uchar		id=0;
	uchar		logdata[16];
	ulong		flags = 0;

	HFC_ENTRY("hfc_fx_get_sid_from_wwpn");
	
	/* Copy data to an internal data area as hfc_ioctl_gidft structure */
	if ( COPYIN( (char *)arg, (char *)&get_sid, sizeof(struct hfc_ioctl_gidpn) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x00 );
		return EFAULT;
	}
	
	STRUCTDUMP( DMP_QWWN, (uchar *)&get_sid, sizeof(struct hfc_ioctl_gidpn) );
	
	if ( get_sid.world_wide_name == 0 ) {
		/* Error if WWN is not specified */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x01 );
		return EINVAL;
	}
	
	/* Clear status */
	get_sid.adapter_status = 0x00;
	get_sid.scsi_id        = 0;
	
	rp = pp->region_arg[pp->rid];
	
	if( !(( pp->connect_type==HFC_FX_SWITCH ) || ( (pp->connect_type==HFC_FX_AL) && (pp->scsi_id & 0x00ffff00) )) ) {
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		/* Search target_info and find SCSI ID if adapter is not in fabric or in public AL */
		if ( ( target = hfc_fx_hash_target_info_wwn( pp, get_sid.world_wide_name ) ) != NULL ) {
			get_sid.scsi_id = (uint64_t)target->scsi_id ;
		}
		else	/* Return ENODEV if target_info is not found */
		{
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x02 );
			rtn = ENODEV;
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		if( COPYOUT( (char *)&get_sid, (char *)arg, sizeof( struct hfc_ioctl_gidpn ) ) != 0 ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x03 );
			rtn = EFAULT;
		}
		
		return rtn;
	}
	
	/*
	 * Allocate payload area
	 */
	payload  = (struct send_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), &payload_busaddr );
	if( payload == NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x04 );
		logdata[0] = 0x90;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)payload & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x05 );
		logdata[0] = 0x91;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)payload,  (uint)sizeof(struct send_payload) );


	/*
	 * Allocate response area
	 */
	response = (struct receive_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), &response_busaddr  );
	if(response == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x06 );
		logdata[0] = 0x92;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)response & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x07 );
		logdata[0] = 0x93;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)response, (uint)sizeof(struct receive_payload));


	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	/* Lock mailbox */
	if ( !(lock_fx_try_mailbox( pp )) ) {
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x08 );
		return EIO;
	}
	
	core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	
	/* Create a mailbox control block based on FRMSNDRCV */
	
	/* Mailbox Request Header */
	hfc_fx_write_val(core->mb->mb_init.mb_code, HFC_MBCMD_SNDRCV);
	hfc_fx_write_val(core->mb->mb_init.timer, pp->mb_timer[ HFC_MBTIME_GID_PN ].tout-1 );
	
	/* Common Control */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.frame_ctl, 0);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload_length, HFC_GIDPN_SLENGTH);
	
	/* VFT_Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 0x800);
	
	/* FC-PH Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, 0);
	sid = (uint)(pp->scsi_id & 0x00ffffff);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	
	/* Payload Address, Receive Payload Address */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload, (uint64_t)payload_busaddr);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.receive_payload, (uint64_t)response_busaddr);
	
	/* Make payload */
	hfc_fx_write_val( payload->data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( payload->data0[4], 0xfc);
	hfc_fx_write_val( payload->data0[5], 0x02);
	hfc_fx_write_val( payload->type.gxx.data1[0], 0x01);
	hfc_fx_write_val( payload->type.gxx.data1[1], 0x21);
	hfc_fx_write_val( payload->type.gxx.data1[2], 0x01);
	hfc_fx_write_val( payload->type.gxx.sub_type.gid_pn.nport_name, get_sid.world_wide_name );
	
	STRUCTDUMP( DMP_PAYLOAD,  (uchar *)payload,  sizeof(struct send_payload) );
	
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
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	rtn = hfc_fx_mailbox_proc(pp, core, HFC_FX_MB_RSP_TMR,
		pp->mb_timer[HFC_MBTIME_GID_PN].tout, pp->mb_timer[HFC_MBTIME_GID_PN].retry);
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	
	if( rtn == 0 ) {
		STRUCTDUMP( DMP_RESPONSE, (uchar *)response, sizeof(struct receive_payload) );
		
		get_sid.scsi_id = ((uint)hfc_fx_read_val( response->type.gxx.sub_type.gid_pn.port_id[0] ) << 16) +
						((uint)hfc_fx_read_val( response->type.gxx.sub_type.gid_pn.port_id[1] ) << 8) +
						 (uint)hfc_fx_read_val( response->type.gxx.sub_type.gid_pn.port_id[2] );
	}
	
	unlock_fx_mailbox(pp);   /* Unlock mailbox */
	
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	/* FCLNX-GPL-FX-353 End */
	
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);

	/* Write-back data to hfc_ioctl_gidft structure  */
	if( COPYOUT( (char *)&get_sid, (char *)arg, sizeof( struct hfc_ioctl_gidpn ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GWWN, 0x0a );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_QWWN, (uchar *)&get_sid, sizeof(struct hfc_ioctl_gidpn) );
	
	HFC_EXIT("hfc_fx_get_sid_from_wwpn");
	
	return rtn;
}


/*
 * Function:    hfc_fx_get_wwpn_from_sid
 *
 * Purpose:     Search port ID(SCSI_ID) and find corresponding WWPN
 *
 * Arguments:   
 *  pp        - Pointer to an port_info structure 
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
int hfc_fx_get_wwpn_from_sid( struct port_info *pp, void *arg ) {
	struct hfc_ioctl_gpnid		get_wwn;
	struct send_payload			*payload;
	struct receive_payload		*response;
	struct target_info_fx		*target;
	struct core_info			*core;
	struct region_info			*rp;
	
	dma_addr_t	payload_busaddr;		/* bus address of payload */
	dma_addr_t	response_busaddr;		/* bus address of response */
	int			rtn=0,lp=0,hit=0;
	uint		sid;
	uchar		id=0;
	uchar		logdata[16];
	ulong		flags = 0;

	HFC_ENTRY("hfc_fx_get_wwpn_from_sid");
	
	/* Copy data to an internal data area as hfc_ioctl_gidpn structure */
	if ( COPYIN( (char *)arg, (char *)&get_wwn, sizeof(struct hfc_ioctl_gpnid) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x00 );
		return EFAULT;
	}
	
	STRUCTDUMP( DMP_GPNID, (uchar *)&get_wwn, sizeof(struct hfc_ioctl_gpnid) );
	
	if ( get_wwn.scsi_id == 0 ) {
		/* Error if SCSI ID is not set */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x01 );
		return EINVAL;
	}
	
	/* Clear status */
	get_wwn.adapter_status  = 0x00;
	get_wwn.world_wide_name = 0;
	
	rp = pp->region_arg[pp->rid];
	
	if( !(( pp->connect_type==HFC_FX_SWITCH ) || ( (pp->connect_type==HFC_FX_AL) && (pp->scsi_id & 0x00ffff00) )) ) {
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		/* Search target_info and find SCSI ID if adapter is not in fabric or in public AL */
		hit=0;
		
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)										/* FC-GW */
		{
			target = hfc_fx_hash_target_info(pp, lp);
		
			if (target != NULL)
			{
				if (target->scsi_id == get_wwn.scsi_id) {
					get_wwn.world_wide_name = (uint64_t) target->ww_name ;
					hit = 1;
					break;
				}
			}
		}
		
		if (!hit) {	/* When target_info is not found, return ENODEV */
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x02 );
			rtn = ENODEV;
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		if( COPYOUT( (char *)&get_wwn, (char *)arg, sizeof( struct hfc_ioctl_gpnid ) ) != 0 ) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x03 );
			rtn = EFAULT;
		}
		
		return rtn;
	}
	
	/*
	 * Allocate payload area
	 */
	payload  = (struct send_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), &payload_busaddr );
	if( payload == NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x04 );
		logdata[0] = 0xa0;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)payload & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x05 );
		logdata[0] = 0xa1;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)payload,  (uint)sizeof(struct send_payload) );


	/*
	 * Allocate response area
	 */
	response = (struct receive_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), &response_busaddr  );
	if(response == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x06 );
		logdata[0] = 0xa2;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	if( (ulong)response & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x07 );
		logdata[0] = 0xa3;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		return ENOMEM;
	}
	BZERO( (char *)response, (uint)sizeof(struct receive_payload));


	/* Lock mailbox */
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	if ( !(lock_fx_try_mailbox( pp )) ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x08 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return EIO;
	}
	
	core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	
	/* Create a mailbox control block based on FRMSNDRCV */
	
	/* Mailbox Request Header */
	hfc_fx_write_val(core->mb->mb_init.mb_code, HFC_MBCMD_SNDRCV);
	hfc_fx_write_val(core->mb->mb_init.timer, pp->mb_timer[ HFC_MBTIME_GPN_ID ].tout-1 );
	
	/* Common Control */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.frame_ctl, 0);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload_length, HFC_GPNID_SLENGTH);
	
	/* VFT_Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 0x800);
	
	/* FC-PH Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, 0);
	sid = (uint)(pp->scsi_id & 0x00ffffff);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	
	/* Payload Address, Receive Payload Address */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload, (uint64_t)payload_busaddr);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.receive_payload, (uint64_t)response_busaddr);
	
	/* Make payload */
	hfc_fx_write_val( payload->data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( payload->data0[4], 0xfc);
	hfc_fx_write_val( payload->data0[5], 0x02);
	hfc_fx_write_val( payload->type.gxx.data1[0], 0x01);
	hfc_fx_write_val( payload->type.gxx.data1[1], 0x12);
	hfc_fx_write_val( payload->type.gxx.data1[2], 0x01);
	hfc_fx_write_val( payload->type.gxx.sub_type.gpn_id.port_id, (get_wwn.scsi_id & 0xffffff) );
	
	STRUCTDUMP( DMP_PAYLOAD,  (uchar *)payload,  sizeof(struct send_payload) );
	
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
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	rtn = hfc_fx_mailbox_proc(pp, core, HFC_FX_MB_RSP_TMR,
		pp->mb_timer[HFC_MBTIME_GPN_ID].tout, pp->mb_timer[HFC_MBTIME_GPN_ID].retry);
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	
	if( rtn == 0 ) {
		STRUCTDUMP( DMP_RESPONSE, (uchar *)response, sizeof(struct receive_payload) );
		
		get_wwn.scsi_id = hfc_fx_read_val( response->type.gxx.sub_type.gpn_id.port_name ) ;
	}
	
	unlock_fx_mailbox(pp);   /* Unlock mailbox */
	
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);	/* FCLNX-GPL-FX-466 */
	/* FCLNX-GPL-FX-353 End */
	
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);

	/* Write-back data to hfc_ioctl_gidft structure  */
	if( COPYOUT( (char *)&get_wwn, (char *)arg, sizeof( struct hfc_ioctl_gidpn ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_GPNID, 0x0a );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_GPNID, (uchar *)&get_wwn, sizeof(struct hfc_ioctl_gpnid) );
	
	HFC_EXIT("hfc_fx_get_wwpn_from_sid");
	
	return rtn;
}


/*
 * Function:    hfc_fx_payload
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to an port_info structure 
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
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLPAYLD)
 */
int hfc_fx_payload( struct port_info *pp, void *arg ) {
	struct hfc_ioctl_payload	payld;
	struct send_payload			*payload;
	struct receive_payload		*response;
	struct core_info			*core;
	
	dma_addr_t	payload_busaddr;		/* bus address of payload */
	dma_addr_t	response_busaddr;		/* bus address of response */
	int			rtn = 0;
	uint		sid, did;
	uchar		id=0;
	uchar		logdata[16];
	ulong		flags = 0;		/* FCLNX-GPL-FX-353 */

	HFC_ENTRY("hfc_fx_payload");
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	if( !test_bit( HFC_PS_ONLINE, (ulong *)&pp->status ) ) {
		/* Adapter is not online */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x00 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		return EIO;
	}
	
	if( test_bit( HFC_PS_ISOL, (ulong *)&pp->status ) ) {
		/* Adapter is isolated, or executing port isolation.*/
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x01 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		return EIO;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)) {
		/* Adapter is not normal status in mlpf shared mode. */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x02 );
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		return EIO;
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	/* Copy data to an internal data area as hfc_ioctl_payload structure */
	if( COPYIN( (char *)arg, (char *)&payld, sizeof(struct hfc_ioctl_payload) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x03 );
		return EFAULT;
	}
	
#if defined(__x86_64)  &&  LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Lower 32bit is effective for pointers */
	if(pp->ioctl32) {
		payld.payld_buffer    = payld.payld_buffer    & 0xffffffffU;
		payld.response_buffer = payld.response_buffer & 0xffffffffU;
	}
#endif
	
	STRUCTDUMP( DMP_PAYLD, (uchar *)&payld, sizeof(struct hfc_ioctl_payload) );
	STRUCTDUMP( DMP_PAYLD, (uchar *)(ulong)payld.payld_buffer, payld.payld_size );

	if( (payld.payld_size>2048) || (payld.response_size>2048) ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x04 );
		return EINVAL;
	}

	/*
	 * Allocate payload area
	 */
	payload  = (struct send_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), &payload_busaddr );
	if( payload == NULL ) {
		logdata[0] = 0xb0;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x05 );
		return ENOMEM;
	}
	if( (ulong)payload & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		logdata[0] = 0xb1;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x06 );
		return ENOMEM;
	}
	BZERO( (char *)payload,  (uint)sizeof(struct send_payload) );


	/*
	 * Allocate response area
	 */
	response = (struct receive_payload *)hfc_fx_pci_alloc_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), &response_busaddr  );
	if(response == NULL) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr );
		logdata[0] = 0xb2;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x07 );
		return ENOMEM;
	}
	if( (ulong)response & 0x0fff ) {
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		logdata[0] = 0xb3;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x08 );
		return ENOMEM;
	}
	BZERO( (char *)response, (uint)sizeof(struct receive_payload));
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	/* Lock mailbox */
	if ( !(lock_fx_try_mailbox( pp )) ) {
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload, payload_busaddr);
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr);
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x09 );
		return EIO;
	}
	
	core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	
	/* Create a mailbox control block based on FRMSNDRCV */
	
	/* Mailbox Request Header */
	hfc_fx_write_val(core->mb->mb_init.mb_code, HFC_MBCMD_SNDRCV);
	hfc_fx_write_val(core->mb->mb_init.timer, HFC_TIMER_FRMSNDRCV-1 );
	
	/* Common Control */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.frame_ctl, 0);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fc_class, (uchar)payld.class);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload_length, (ushort)payld.payld_size);
	
	/* VFT_Header */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 0x800);
	
	/* FC-PH Header */
//	csctl_sid = (uint)( pp->scsi_id & 0x00ffffff);
//	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, csctl_sid);
//	rctl_did = (uint)((payld.ctl << 24) | (payld.scsi_id & 0x00ffffff));
//	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.rctl, rctl_did);
	
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, 0);
	sid = (uint)(pp->scsi_id & 0x00ffffff);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.rctl, (uchar)payld.ctl);
	did = (uint)(payld.scsi_id & 0x00ffffff);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);
	
	/* Payload Address, Receive Payload Address */
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.payload, (uint64_t)payload_busaddr);
	hfc_fx_write_val(core->mb->mb_init.type.frmsndrcv.receive_payload, (uint64_t)response_busaddr);
	
	/* Copy Paylod */
	if( COPYIN( (uchar *)(ulong)payld.payld_buffer, (uchar *)payload, payld.payld_size ) != 0 ) {
		unlock_fx_mailbox(pp);
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload,  payload_busaddr  );
		hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr );
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x0a );
		return EFAULT;
	}
	
	STRUCTDUMP( DMP_PAYLOAD,  (uchar *)payload,  sizeof(struct send_payload) );
	
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
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	rtn = hfc_fx_mailbox_proc(pp, core, HFC_FX_MB_RSP_TMR, HFC_TIMER_FRMSNDRCV, pp->els_retry);
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	if( rtn == 0 ) {
		STRUCTDUMP( DMP_RESPONSE, (uchar *)response, sizeof(struct receive_payload) );
		if( COPYOUT( (uchar *)response, (char *)(ulong)payld.response_buffer, payld.response_size ) != 0 ) {
			rtn = EFAULT;
		}
		STRUCTDUMP( DMP_PAYLD, (uchar *)(ulong)payld.response_buffer, payld.response_size );
	}
	unlock_fx_mailbox(pp);
	
	/* FCLNX-GPL-FX-353 Start */
	if(!test_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 ))
		start_fx_next_mailbox(pp, NULL);
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	/* FCLNX-GPL-FX-353 End */
	
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct send_payload), (void *)payload,  payload_busaddr  );
	hfc_fx_pci_free_consistent(pp, pp->pci_cfginf, (uint)sizeof(struct receive_payload), (void *)response, response_busaddr );

	/* Write back data to hfc_ioctl_payload structure */
	if( COPYOUT( (char *)&payld, (char *)arg, sizeof( struct hfc_ioctl_payload ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PAYLD, 0x08 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_PAYLD, (uchar *)&payld, sizeof(struct hfc_ioctl_payload) );
	
	HFC_EXIT("hfc_fx_payload");
	
	return rtn;
}


/*
 * Function:    hfc_fx_get_rnid
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLCHBA)
 */

int hfc_fx_get_rnid( struct port_info *pp, void *arg ) {
	int    rtn =ENOTTY;  

	return rtn;
}


/*
 * Function:    hfc_fx_set_rnid
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLCHBA)
 */

int hfc_fx_set_rnid( struct port_info *pp, void *arg ) {
	int    rtn =ENOTTY;  

	return rtn;
}


/*
 * Function:    hfc_fx_adap_stat
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLCHBA)
 */

int hfc_fx_adap_stat( struct port_info *pp, void *arg ) {
	int    i,j,rtn =0;

	struct hfc_ioctl_adap_stat adapstat;
	struct region_info *rp = NULL;
	struct core_info *core[MAX_CORE_PROBE_FX] = {0};
	uchar  storecnt[MAX_CORE_PROBE_FX] = {0};
	uchar  storeflag[MAX_CORE_PROBE_FX] = {0};
	ulong  flags = 0;
	uint   port_exec;
	
	HFC_ENTRY("hfc_fx_adap_stat");
	
	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_ioctl_adap_stat structure */
	if( COPYIN( (char *)arg, (char *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x00 );
		return EFAULT;
	}
//	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );

	if(test_bit( HFC_PS_ISOL, (ulong *)&pp->status)){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x01 );
		return EIO;
	}
	
	if(test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x02 );
		return EIO;
	}
	rp = pp->region_arg[pp->rid];
	if(rp == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x03 );
		return 0;
	}
	
	port_exec = 0 ;
	port_exec |= (uint)(0x00001000) ;
	port_exec |= (uint)( (pp->rid << 16) & 0x00ff0000) ;
	port_exec |= HFC_FRAMEA_PORTSTATISTICS ;
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core[i] = rp->core_arg[i];
		if(!hfc_fx_check_cs_disable(pp, core[i])){	/* FCLNX-GPL-FX-438 */
			storecnt[i] = core[i]->fw_init_p->portstatistics.fw_store_count;
			storeflag[i] = 1;
			HFC_DBGPRT("storeflag[%d] = 1\n",i);
//			hfc_fx_write_reg_ext(pp,( uint )hfc_framea_of_core[i],(char) 0x4, (uint) port_exec);
			hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
				(char)0x4, (int)port_exec, HFC_FX_CORE_OFFSET40);
		}
	}
	if(storeflag[0] == 0 && storeflag[1] == 0 && storeflag[2] == 0 && storeflag[3] == 0){
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return EIO;
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	for(j=0 ; j<5000 ; j++){
		if(storeflag[0] == 0 && storeflag[1] == 0 && storeflag[2] == 0 && storeflag[3] == 0){
			HFC_DBGPRT( "msleep = %d \n", j );
			goto CMDSKIP;
		}
		msleep(1);

		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(storecnt[i] != core[i]->fw_init_p->portstatistics.fw_store_count){
				storeflag[i] = 0;
			}
		}
	}
	HFC_DBGPRT( "msleep = %d \n", j );

CMDSKIP:
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	memset(&adapstat,0,sizeof(struct hfc_ioctl_adap_stat));
	adapstat.tx_frames				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.tx_frames );
	adapstat.tx_words				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.tx_words );
	adapstat.rx_frames				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.rx_frames );
	adapstat.rx_words				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.rx_words );
	adapstat.lip_count				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.lip_count );
	adapstat.nos_count				=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.nos_count );
	adapstat.link_failure_count		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.link_failure_count );
	adapstat.loss_of_sync_count		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.loss_of_sync_count );
	adapstat.loss_of_signal_count	=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.loss_of_signal_count );
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		adapstat.error_frames		+=(uint64_t)hfc_fx_read_val( core[i]->fw_init_p->portstatistics.error_frames );
		adapstat.invalid_crc_count	+=(uint64_t)hfc_fx_read_val( core[i]->fw_init_p->portstatistics.invalid_crc_count );
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	if( COPYOUT( ( char * )&adapstat, ( char * )arg, sizeof( struct hfc_ioctl_adap_stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_APSTAT, 0x02 );
		rtn = EFAULT;
	}
//	STRUCTDUMP( DMP_APSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );
	
	return rtn;
}

/* FCLNX-GPL-261 */
/*
 * Function:    hfc_fx_adap_stat_new
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() (SCIOLCHBA)
 */

int hfc_fx_adap_stat_new( struct port_info *pp, void *arg ) {
	int    rtn =0;
#if 0
	struct hfc_ioctl_adap_stat adapstat;

	HFC_ENTRY("hfc_fx_adap_stat_new");

	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_fx_ioctl_adap_stat structure */
	if( COPYIN( (char *)arg, (char *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_ppSTAT_NEW, 0x00 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_ppSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );


	adapstat.adapter_status = 0x00;   /* Set zero */

	adapstat.seconds_since_last_reset = 0;
	adapstat.tx_frames                       = (uint64_t)pp->tx_frames;
	adapstat.tx_words                        = (uint64_t)pp->tx_words;
	adapstat.rx_frames                       = (uint64_t)pp->rx_frames;
	adapstat.rx_words                        = (uint64_t)pp->rx_words;
	adapstat.lip_count                       = (uint64_t)hfc_fx_read_stat_cca(pp, 0x400);
	adapstat.nos_count                       = (uint64_t)hfc_fx_read_stat_cca(pp, 0x408);
	adapstat.error_frames                    = (uint64_t)hfc_fx_read_stat_cca(pp, 0x410);
	adapstat.dumped_frames                   = (uint64_t)hfc_fx_read_stat_cca(pp, 0x418);
	adapstat.link_failure_count              = (uint64_t)hfc_fx_read_stat_cca(pp, 0x420);
	adapstat.loss_of_sync_count              = (uint64_t)hfc_fx_read_stat_cca(pp, 0x428);
	adapstat.loss_of_signal_count            = (uint64_t)hfc_fx_read_stat_cca(pp, 0x430);
	adapstat.primitive_seq_protocol_err_count= (uint64_t)hfc_fx_read_stat_cca(pp, 0x438);
	adapstat.invalid_tx_word_count           = (uint64_t)hfc_fx_read_stat_cca(pp, 0x440);
	adapstat.invalid_crc_count               = (uint64_t)hfc_fx_read_stat_cca(pp, 0x448);

	/* Write back data to hfc_fx_ioctl_adap_stat structure */
	if( COPYOUT( ( char * )&adapstat, ( char * )arg, sizeof( struct hfc_ioctl_adap_stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_ppSTAT_NEW, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_ppSTAT, (uchar *)&adapstat, sizeof(struct hfc_ioctl_adap_stat) );
#endif
	return rtn;
}
/* FCLNX-GPL-261 */

/*
 * Function:    hfc_fx_target_mapping
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */

int hfc_fx_target_mapping( struct port_info *pp, void *arg ) {
	int    rtn = 0;
	
	struct hfc_ioctl_tgt_map  tgtmap;
	struct hfc_ioctl_tgt_map *tgtmap_ptr;
	struct target_info_fx       *target;
	uint   i,n,m = 0;
	ulong  flags = 0;


	HFC_ENTRY("hfc_fx_target_mpaping");

	/* Copy data to an internal data area as hfc_fx_ioctl_tgt_map structure */
	if( COPYIN( (char *)arg, (char *)&tgtmap, sizeof(struct hfc_ioctl_tgt_map) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x00 );
		return EFAULT;
	}
//	STRUCTDUMP( DMP_TGTMAP, (uchar *)&tgtmap, sizeof(struct hfc_ioctl_tgt_map) );

	n = tgtmap.hedder[0].number_of_entries;
	if( ( tgtmap_ptr = hfc_fx_kmalloc(pp, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry), GFP_ATOMIC) )==NULL ) {
		/* Error if unable to allocate target map */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x02 );
		return ENOMEM;
	}
	memset( tgtmap_ptr, 0, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) );
	tgtmap_ptr->hedder[0].version           = tgtmap.hedder[0].version;
	tgtmap_ptr->hedder[0].flags             = tgtmap.hedder[0].flags;
	tgtmap_ptr->hedder[0].number_of_entries = tgtmap.hedder[0].number_of_entries;


	HFC_PORTLOCK_IRQSAVE(pp,flags);
	for( i=0; i<MAX_TARGET_PROBE; i++ ) {
		target = pp -> target_arg[i];
		if( target != NULL ) {                                                        //FCLNX-0150
			if( test_bit(HFC_TF_DEVFLG_VALID,(ulong *)&target->flags) ) {                //FCLNX-0150
				if( m < n ){                                                          //FCLNX-0150
					tgtmap_ptr -> entry[m].scsiid.target_id    = target -> target_id; //FCLNX-0150
					tgtmap_ptr -> entry[m].fcpid.node_wwn      = target -> node_name; //FCLNX-0150
					tgtmap_ptr -> entry[m].fcpid.port_wwn      = target -> ww_name;   //FCLNX-0150
					tgtmap_ptr -> entry[m].scsiid.target_valid = HFC_TGTWWN_VALID;    //FCLNX-0150
					if( test_bit(HFC_TF_WWN_VALID,(ulong *)&target->flags)   //I become it when I inspect HFC_TF_WWN_VALID whether "SCSI ID is effective" //FCLNX-0150
							&& (test_bit(HFC_PS_ONLINE,     (ulong *)&pp->status))           //FCLNX-0151
							&& (!test_bit(HFC_PS_WAIT_LINKUP,(ulong *)&pp->status ))         //FCLNX-0151
							&& (!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status))	 //FCLNX-GPL-FX-005
							&& (!test_bit(HFC_TS_SCN_WLINKUP,(ulong *)&target->status)) ) {  //FCLNX-0151
						tgtmap_ptr -> entry[m].scsiid.target_valid |= HFC_SCSIID_VALID; //add a bit     //FCLNX-0150
						tgtmap_ptr -> entry[m].fcpid.scsi_id       = (uint)target -> scsi_id;           //FCLNX-0150
					}                                                                 //FCLNX-0150
				}                                                                     //FCLNX-0150
				m++;
			}                                                                         //FCLNX-0150
		}                                                                             //FCLNX-0150
	}
	tgtmap_ptr->hedder[0].number_of_target = m;
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	/* Write back data to hfc_fx_ioctl_tgt_map structure */
	if( COPYOUT( ( char * )tgtmap_ptr, ( char * )arg, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_TGTMAP, 0x03 );
		rtn = EFAULT;
	}
//	STRUCTDUMP( DMP_TGTMAP, (uchar *)tgtmap_ptr, sizeof(struct hfc_tgtmap_hedder)+n*sizeof(struct hfc_tgtmap_entry) );
	hfc_fx_kfree(pp, tgtmap_ptr );

	return rtn;
}


/*
 * Function:    hfc_fx_fcp_binding
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */


int hfc_fx_fcp_binding( struct port_info *pp, void *arg ) {
	int    rtn =ENOTTY;

	return rtn;
}


/*
 * Function:    hfc_fx_support (Unsupported function)
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */

int hfc_fx_support( struct port_info *pp, void *arg ) {
	int    rtn =0;
	struct hfc_ioctl_spt spt;


	HFC_ENTRY("hfc_fx_support");

	/* FCLNX-510 Deletes checking ONLINE bit */
	/* Copy data to an internal data area as hfc_fx_ioctl_spt structure */
	if( COPYIN( (char *)arg, (char *)&spt, sizeof(struct hfc_ioctl_spt) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SPT, 0x00 );
		return EFAULT;
	}
//	STRUCTDUMP( DMP_SPT, (uchar *)&spt, sizeof(struct hfc_ioctl_spt) );


	/* Write back data to hfc_fx_ioctl_sptstructure */
	if( COPYOUT( ( char * )&spt, ( char * )arg, sizeof( struct hfc_ioctl_spt ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_SPT, 0x02 );
		rtn = EFAULT;
	}
//	STRUCTDUMP( DMP_SPT, (uchar *)&spt, sizeof(struct hfc_ioctl_spt) );

	return rtn;
}


/*
 * Function:    hfc_fx_pron_info
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */

int hfc_fx_procinfo( struct port_info *pp, void *arg ) {
	int    rtn =0;
	int    i;
	ulong  flags = 0;
	struct hfc_ioctl_proc_info *procinfo;										/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	struct hfc_vpd			*vpd_info	= NULL;									/* FCLNX-0404 */
	struct hfc_vpd_five		*vpdf_info	= NULL;									/* FCLNX-0404 */
	struct hfc_vpd_five_ex	*vpdex_info	= NULL;
	struct hfc_vpd_five_fx	*vpdfx_info	= NULL;
	struct port_info	*pport;
	struct region_info	*rp = NULL;

	HFC_ENTRY("hfc_fx_proc_info");
	
	/* host was not founded */
	if( pp->hosts==NULL ){
		return (-EINVAL);
	}

	procinfo = (struct hfc_ioctl_proc_info *)hfc_fx_kmalloc(pp, sizeof(struct hfc_ioctl_proc_info), GFP_ATOMIC);
	HFC_DBGPRT("alloc hfc_fx_proc_info\n");																			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	if (procinfo == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x09 );
		return ENOMEM;
	}

	memset(procinfo, 0, sizeof(struct hfc_ioctl_proc_info));					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */

	/* Copy data to an internal data area as hfc_fx_ioctl_proc_info  structure */
	if( COPYIN( (char *)arg, (char *)procinfo, sizeof(struct hfc_ioctl_proc_info) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x00 );
		hfc_fx_kfree(pp, procinfo);														/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		return EFAULT;
	}
	
	if (procinfo->vport_no != 0) {	/* FCLLNX-GPL-FX-196 */
		if (procinfo->vport_no > pp->max_vport_count) {
			HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x05 );
			hfc_fx_kfree(pp, procinfo);
			return EIO;
		}
	}
	
	/* save physical port pointer */
	pport = pp;
	
	if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){	/* FCLNX-GPL-FX-385 */
		rp = pport->region_arg[0];
	}else{
		rp = pport->region_arg[pp->rid];
	}	/* FCLNX-GPL-FX-385 */
	
	if(rp == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x02 );
		hfc_fx_kfree(pport, procinfo);
		return EIO;
	}
	
	HFC_ALLLOCK_IRQSAVE(pport, rp, flags);
	
	pp = pport->vport_ptr[procinfo->vport_no].vport_arg;
	if (pp == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x03 );
		HFC_ALLUNLOCK_IRQRESTORE(pport, rp, flags);
		hfc_fx_kfree(pport, procinfo);
		pp = pport;
		return EIO;
	}
	
	if (HFC_PP_FX_STATUS_DETAIL2_TEST(HFC_PD_WAIT_CLOSE, pp)) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x04 );
		HFC_ALLUNLOCK_IRQRESTORE(pport, rp, flags);
		hfc_fx_kfree(pport, procinfo);
		pp = pport;
		return EIO;
	}
	
//	STRUCTDUMP( DMP_PRINFO, (uchar *)procinfo, sizeof(struct hfc_ioctl_proc_info) );
	/* Copy information in port_info to hfc_fx_ioctl_proc_info structure  */
	procinfo->version = 0x00;
	procinfo->flags   = 0x00;
	procinfo->dev_major = pp -> dev_major;
	procinfo->dev_minor = pp -> dev_minor;
	procinfo->instance  = pp -> instance;
	procinfo->host_no   = pp -> hosts -> host_no;
	procinfo->unique_id = pp -> hosts -> unique_id;
	procinfo->vender_id = pp -> pkg.vender_id;
	procinfo->device_id = pp -> pkg.device_id;

	procinfo->ww_name   = pp -> ww_name;
	procinfo->node_name = pp -> node_name;
	procinfo->scsi_id   = pp -> scsi_id;
	strncpy(procinfo->model_name, pp->model_name, 16);							/* FCLNX-0329 */
	strncpy(procinfo->driver_ver,hfc_manage_info.package_ver,16);				/* FCLNX-0329 */
	memcpy(procinfo->adap_id, pp->adap_id, 16);									/* FCLNX-0329 */
	procinfo->firmware_ver = hfc_fx_get_sysrev(pp->region_arg[pp->rid]->core_arg[0]);									/* FCLNX-GPL-112 */
	memset(procinfo->parts_number,0,16);										/* FCLNX-0404 */
	if(pp->pkg.type == HFC_PKTYPE_FPP) {										/* FCLNX-0404 */
		vpd_info = (struct hfc_vpd *)pp->vpd_buf;								/* FCLNX-0404 */
		memcpy(procinfo->parts_number,vpd_info->pn_value,VPD_PN_LEN);			/* FCLNX-0404 */
		procinfo->ec_level = vpd_info->ec_value[0];								/* FCLNX-0404 */
	}																			/* FCLNX-0404 */
	else if(pp->pkg.type == HFC_PKTYPE_FIVE) {									/* FCLNX-0404 */
		vpdf_info = (struct hfc_vpd_five *)pp->vpd_buf;							/* FCLNX-0404 */
		memcpy(procinfo->parts_number,vpdf_info->pn_value,VPD_PN_LEN);			/* FCLNX-0404 */
		procinfo->ec_level = vpdf_info->ec_level;								/* FCLNX-0404 */
	}																			/* FCLNX-0404 */
	else if(pp->pkg.type == HFC_PKTYPE_FIVE_EX){
		vpdex_info = (struct hfc_vpd_five_ex *)pp->vpd_buf;
		memcpy(procinfo->parts_number,vpdex_info->pn_value,VPD_PN_LEN);
		procinfo->ec_level = vpdex_info->ec_level;
	}
	else { /* FIVE-FX */
		vpdfx_info = (struct hfc_vpd_five_fx *)pp->vpd_buf;
		memcpy(procinfo->parts_number,vpdfx_info->pn_value,VPD_PN_LEN);
		procinfo->ec_level = vpdfx_info->ec_level;
	}
	procinfo->pkgtype         = pp->pkg.type;									/* FCLNX-0404 */
	procinfo->pkgcode         = pp->pkg.code;									/* FCLNX-0404 */
	procinfo->bus_dev_func[0] = pp->pci_cfginf->bus->number;					/* FCLNX-0404 */
	procinfo->bus_dev_func[1] = PCI_SLOT(pp->pci_cfginf->devfn);				/* FCLNX-0404 */
	procinfo->bus_dev_func[2] = PCI_FUNC(pp->pci_cfginf->devfn);				/* FCLNX-0404 */
	procinfo->port_no         = pp->port_no;									/* FCLNX-0404 */
	memcpy(procinfo->opt_vendor_name,pp->opt_vendor_name,32);					/* FCLNX-0404 */
	memcpy(procinfo->opt_parts_number,pp->opt_parts_number,32);					/* FCLNX-0404 */
	memcpy(procinfo->opt_serial_number,pp->opt_serial_number,32);				/* FCLNX-0404 */

	switch( pp->connect_type ){
	case HFC_FX_SWITCH : 
		if( test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) procinfo->connection_type = HFC_CON_TYP_PtoPF;
		break;
	case HFC_FX_PT2PT  :
		if( test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) procinfo->connection_type = HFC_CON_TYP_PtoP;
		break;
	case HFC_FX_AL     :
		if( test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ){
			if( pp->scsi_id & 0x00ffff00){
				procinfo->connection_type = HFC_CON_TYP_FcAlF;
			}
			else{
				procinfo->connection_type = HFC_CON_TYP_FcAl;
			}
		}
		break;
	case HFC_FX_MULTI_ALPA :
		if( test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) procinfo->connection_type = HFC_CON_TYP_MULTIAL;	/* FCLNX-GPL-FX-135 */
		break;
	case HFC_FX_F_PORT     :
		if( test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) procinfo->connection_type = HFC_CON_TYP_FPORT;
		break;
	default         :
		procinfo->connection_type = HFC_CON_TYP_NONE;
	}
	procinfo->core_num      = pp->core_num;
	procinfo->max_data_rate = pp->max_data_rate;
	procinfo->multiple_portid   = pp->multiple_portid;							/* FCLNX-GPL-FX-135*/

	procinfo->login_delay       = pp->login_wait;								/* FCLNX-0404 */
	procinfo->spinup_delay      = pp->spinup_delay;								/* FCLNX-0404 */
	procinfo->automap           = pp->automap;									/* FCLNX-0404 */
	procinfo->pref_alpa         = pp->pref_alpa;								/* FCLNX-0404 */
	procinfo->xob_max           = pp->xob_max;									/* FCLNX-0404 */
	procinfo->xrb_max           = pp->xrb_max;									/* FCLNX-0404 */
	procinfo->slog_max          = pp->slog_max;									/* FCLNX-0404 */
	procinfo->dma_max           = pp->dma_max;									/* FCLNX-0404 */
	procinfo->queue_depth       = pp->queue_depth;								/* FCLNX-0404 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		procinfo->linkup_tmo        = pp->dev_loss_tmo;
	else
		procinfo->linkup_tmo        = pp->linkup_tmo;								/* FCLNX-0404 */
#else
	procinfo->linkup_tmo        = pp->linkup_tmo;								/* FCLNX-0404 */
#endif
	procinfo->scsi_reset_delay  = pp->scsi_reset_delay;							/* FCLNX-0404 */
	procinfo->target_reset_tmo  = pp->target_reset_tmo;							/* FCLNX-0404 */
	procinfo->abort_tmo         = pp->abort_tmo;								/* FCLNX-0404 */
	procinfo->max_target        = pp->max_target;								/* FCLNX-0404 */
	procinfo->trc_max           = pp->trc_max;									/* FCLNX-0404 */
	procinfo->max_mck_cnt       = pp->max_mck_cnt;								/* FCLNX-0404 */
	procinfo->wmsg              = pp->wmsg;										/* FCLNX-0404 */
	procinfo->linkup2_tmo       = pp->mck_rcv_tmo;								/* FCLNX-0404 */
	procinfo->pkt_num           = pp->pm_pkt_num;								/* FCLNX-0404 */
	procinfo->can_queue         = pp->can_queue;								/* FCLNX-0404 */
	procinfo->sg_tblsize        = pp->sg_tblsize;								/* FCLNX-0404 */
	procinfo->cmnd_num          = pp->cmnd_num;									/* FCLNX-0404 */
	procinfo->minus_tout        = pp->minus_tout;								/* FCLNX-0404 */
	procinfo->scsi_allowed      = pp->scsi_allowed;								/* FCLNX-0404 */
	procinfo->cmd_per_lun       = pp->cmd_per_lun;								/* FCLNX-0404 */
	procinfo->max_sectors       = pp->max_sectors;								/* FCLNX-0404 */
	procinfo->scsi_timeout_fail = pp->scsi_timeout_fail;							/* FCLNX-0404 */
	procinfo->mlpf_mode     	   = pp->mlpf_mode;					                        /* FCLNX-0488 */
	procinfo->highconf_opt	   = pp->manage_info->hfcplus_enable;    /* FCLNX-0488 */

	procinfo->port_status = hfc_fx_get_adap_status(pp);	/* FCLNX-GPL-428 */
	
	procinfo->abort_t_restrain 	= pp->abort_t_restrain;		/*FCLNX-0506*/
	if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {			/* FCLNX-GPL-FX-203 */
		procinfo->msi_flag		= HFC_INT_TYPE_MSIX;
	}
	else {
		procinfo->msi_flag		= pp->msi_flag;				/* FCLNX-GPL-126 */
	}														/* FCLNX-GPL-FX-203 */
	procinfo->lun_reset_delay 	= pp->lun_reset_delay;		/*FCLNX-0506*/
	procinfo->target_reset_mode	= pp->enable_tgtrst;		/* Get Target Reset Mode On/Off */ /* FCLNX-0660 */

	procinfo->limit_log			= pp->limit_log;								/* FCLNX-GPL-491 */
	procinfo->filter_target		= pp->filter_target;							/* FCLNX-GPL-491 */
	procinfo->hg_stats_disable	= pp->hg_stats_disable;							/* FCLNX-GPL-494 */
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		procinfo->scsi_exec_cnt_c[i] = pp->region_arg[pp->rid]->core_arg[i]->scsi_exec_cnt;
	}
	procinfo->scsi_exec_cnt_p = pp->scsi_exec_cnt;
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		procinfo->scsi_end_cnt_c[i] = pp->region_arg[pp->rid]->core_arg[i]->scsi_end_cnt;
	}
	procinfo->scsi_end_cnt_p = pp->scsi_end_cnt;
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		procinfo->xrb_resp_cnt_c[i] = pp->region_arg[pp->rid]->core_arg[i]->xrb_resp_cnt;
	}
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		procinfo->we_que_cnt_c[i] = pp->region_arg[pp->rid]->core_arg[i]->we_que_cnt_all;
	}
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if (hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])) {		/* FCLNX-GPL-FX-438 */
			set_bit( HFC_CS_CHK_STOP, (ulong *)&procinfo->core_status[i]);
		}
	}
	procinfo->tgtrst_restrain = pp->tgtrst_restrain;
	procinfo->max_vport_count = pp->max_vport_count;
	procinfo->hfc_pm_pkt_size = sizeof(struct hfc_pm_pkt_fx);
	
	memcpy((uchar*)&procinfo->ecid[0], (uchar*)&pp->ecid[0], 64);
	
	procinfo->npiv_mode = pp->npiv_mode;
	
#if 0	
	HFC_DBGPRT("procinfo->version      = %x\n",procinfo->version);
	HFC_DBGPRT("procinfo->flags        = %x\n",procinfo->flags);
	HFC_DBGPRT("procinfo->dev_major    = %d\n",procinfo->dev_major);
	HFC_DBGPRT("procinfo->dev_minor    = %d\n",procinfo->dev_minor);
	HFC_DBGPRT("procinfo->instance     = %d\n",procinfo->instance);
	HFC_DBGPRT("procinfo->host_no      = %d\n",procinfo->host_no);
	HFC_DBGPRT("procinfo->unique_id    = 0x%016llx",(unsigned long long)procinfo->unique_id);
	HFC_DBGPRT("procinfo->vender_id    = 0x%016llx\n",(unsigned long long)procinfo->vender_id);
	HFC_DBGPRT("procinfo->device_id    = 0x%016llx\n",(unsigned long long)procinfo->device_id);
	HFC_DBGPRT("procinfo->ww_name      = 0x%016llx\n",(unsigned long long)procinfo->ww_name);
	HFC_DBGPRT("procinfo->node_name    = 0x%016llx\n",(unsigned long long)procinfo->node_name);
	HFC_DBGPRT("procinfo->scsi_id      = 0x%016llx\n",(unsigned long long)procinfo->scsi_id);
	HFC_DBGPRT("procinfo->model_name   = %x\n",procinfo->model_name);
	HFC_DBGPRT("procinfo->driver_ver   = %x\n",procinfo->driver_ver);
	HFC_DBGPRT("procinfo->adap_id      = %x\n",procinfo->adap_id);
	HFC_DBGPRT("procinfo->firmware_ver = 0x%016llx\n",(unsigned long long)procinfo->firmware_ver);
	HFC_DBGPRT("procinfo->pkgtype      = %x\n",procinfo->pkgtype);
	HFC_DBGPRT("procinfo->pkgcode      = %x\n",procinfo->pkgcode);
	HFC_DBGPRT("procinfo->bus_dev_func[0]  = %x\n",procinfo->bus_dev_func[0]);
	HFC_DBGPRT("procinfo->bus_dev_func[1]  = %x\n",procinfo->bus_dev_func[1]);
	HFC_DBGPRT("procinfo->bus_dev_func[2]  = %x\n",procinfo->bus_dev_func[2]);
	HFC_DBGPRT("procinfo->trc_max      = %d(0x%08x)\n",procinfo->trc_max,procinfo->trc_max);
	HFC_DBGPRT("procinfo->limit_log    = %d(0x%08x)\n",procinfo->limit_log,procinfo->limit_log);
	HFC_DBGPRT("procinfo->tgtrst_restrain  = %d(0x%08x)\n",procinfo->tgtrst_restrain,procinfo->tgtrst_restrain);
	HFC_DBGPRT("procinfo->max_vport_count  = %d(0x%08x)\n",procinfo->max_vport_count,procinfo->max_vport_count);
	HFC_DBGPRT("procinfo->hfcpkt_size      = %d(0x%08x)\n",procinfo->hfcpkt_size,procinfo->hfcpkt_size);
#endif

	if (pp->pm_control == HFC_FX_PM_ON) {
		procinfo->pm_pkt_num    = pp->pm_pkt_num;
	}
	else {
		procinfo->pm_pkt_num    = 0;
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pport, rp, flags);
	
	/* Write back data to hfc_fx_ioctl_proc_info structure */
	if( COPYOUT( ( char * )procinfo, ( char * )arg, sizeof( struct hfc_ioctl_proc_info ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_PRINFO, 0x01 );
		rtn = EFAULT;
	}
//	STRUCTDUMP( DMP_PRINFO, (uchar *)procinfo, sizeof(struct hfc_ioctl_proc_info) );

	hfc_fx_kfree(pport, procinfo);											/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	pp = pport;
	
	return rtn;
}

/*
 * Function:    hfc_fx_fc4stat
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */
int hfc_fx_fc4stat( struct port_info *pp, void *arg ) {							/* FCLNX-0404 */
	int    rtn = 0;
	struct hfc_ioctl_fc4stat fc4stat;

	HFC_ENTRY("hfc_fx_fc4stat");

	/* Host was not found */
	if( pp->hosts==NULL ){
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x00 );
		return (-EINVAL);
	}

	/* Copy data to an internal data area as hfc_fx_ioctl_fc4stat  structure */
	if( COPYIN( (char *)arg, (char *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x01 );
		return EFAULT;
	}
	STRUCTDUMP( DMP_FC4STAT, (uchar *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) );
	
	fc4stat.inputrequests   = pp->inputrequests;
	fc4stat.outputrequests  = pp->outputrequests;
	fc4stat.controlrequests = pp->controlrequests;
	fc4stat.inputmegabytes  = pp->inputbytes >> 20;
	fc4stat.outputmegabytes = pp->outputbytes >> 20;

	/* Write back data to hfc_fx_ioctl_fc4stat structure */	
	if( COPYOUT( ( char * )&fc4stat, ( char * )arg, sizeof( struct hfc_ioctl_fc4stat ) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_SC_FC4STAT, 0x02 );
		rtn = EFAULT;
	}
	STRUCTDUMP( DMP_FC4STAT, (uchar *)&fc4stat, sizeof(struct hfc_ioctl_fc4stat) );

	return rtn;
}																				/* FCLNX-0404 */


/*
 * Function:    hfc_fx_adapter_enable
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */
																/* hotplug */
int hfc_fx_adapter_enable( struct port_info *pp, void *arg ) {
	return ENOTTY;
}


/*
 * Function:    hfc_fx_adapter_disable
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */
																	/* hotplug */
int hfc_fx_adapter_disable( struct port_info *pp, void *arg ) {
	return ENOTTY;
}


/*
 * Function:    hfc_fx_scsi_scan
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */
int hfc_fx_scsi_scan( struct port_info *pp, void *arg ) {
	return ENOTTY;
}


/*
 * Function:    hfc_fx_mp_target_map
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *  arg       - Pointer to data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl() 
 */
int hfc_fx_mp_target_map( struct port_info *pp, void *arg ) {
	int    rtn =0;
	uint   i,n,m = 0;
	ulong  flags = 0;
	
	struct hfc_mp_target	tgtmap;
	struct hfc_mp_target	*tgtmpap;
	struct target_info_fx	*target;
	struct region_info		*rp;

	HFC_ENTRY("hfc_fx_mp_target_map");

	if( COPYIN( (char *)arg, (char *)&tgtmap, sizeof(struct hfc_mp_target) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x00 );
		return EFAULT;
	}
//	STRUCTDUMP( DMP_MP_TARGET, (uchar *)&tgtmap, sizeof(struct hfc_fx_mp_target) );
	
	n = tgtmap.header[0].entry_count;
	if( ( tgtmpap = hfc_fx_kmalloc(pp, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry), GFP_ATOMIC) )==NULL ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x01 );
		return ENOMEM;
	}
	
	rp = pp->region_arg[pp->rid];
	if(rp == NULL) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x03 );
		return EIO;
	}
	
	memset( tgtmpap, 0, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) );
	tgtmpap->header[0].entry_count = tgtmap.header[0].entry_count;
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for( i=0; i<MAX_TARGET_PROBE; i++ ) {
		target = pp->target_arg[i];
		if( target != NULL ) {
			if( test_bit(HFC_TF_DEVFLG_VALID,(ulong *)&target->flags) ) {
				if( m < n ){
					set_bit(HFC_MP_WWN_VALID,(ulong *)&tgtmpap->entry[m].flags);
					tgtmpap->entry[m].nodename    = target->node_name;
					tgtmpap->entry[m].portname    = target->ww_name;
					tgtmpap->entry[m].attribute   = target->attribute;
					tgtmpap->entry[m].groupid     = target->group_id;
					tgtmpap->entry[m].targetid    = target->target_id;
					tgtmpap->entry[m].pathid      = target->path_id;
					tgtmpap->entry[m].instance    = pp->instance;
					
					if( (test_bit(HFC_TF_WWN_VALID,(ulong *)&target->flags)) 
						&& (!test_bit(HFC_TS_SCN_WLINKUP,(ulong *)&target->status)) ) {
						
						if ((pp->status & (~((0x00000001 << HFC_PS_ENABLE) |
							(0x00000001 << HFC_PS_ONLINE) |
							(0x00000001 << HFC_PS_CONNECTED)))) ||
							(pp->status_detail1) ||
							(pp->status_detail2)) {
							/* status busy */
						}
						else {
							/* link up */
							set_bit(HFC_MP_LINKUP,(ulong *)&tgtmpap->entry[m].flags);
						}
					}
				}
				m++;
			}
		}
	}
	tgtmpap->header[0].target_count = m;
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	if( COPYOUT( ( char * )tgtmpap, ( char * )arg, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) ) != 0 ) {
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_MP_TGTMAP, 0x02 );
		rtn = EFAULT;
	}
//	STRUCTDUMP( DMP_MP_TARGET, (uchar *)tgtmpap, sizeof(struct mp_target_header)+n*sizeof(struct mp_target_entry) );
	hfc_fx_kfree(pp, tgtmpap );

	HFC_EXIT("hfc_fx_mp_target_map");

	return rtn;
}

/*
 * Function:    hfc_fx_read_apparam
 *
 * Purpose:
 *
 * Arguments:
 *  pp        - Pointer to port_info
 *  arg       - Pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl()
 */
 
int hfc_fx_read_apparam( struct port_info *pp, void *arg) {
	int rtn =ENOTTY;

	return rtn;
}

/*
 * Function:    hfc_fx_isolate_operation
 *
 * Purpose:
 *
 * Arguments:
 *  pp        - Pointer to port_info
 *  arg       - Pointer to data area
 *
 * Returns:
 *  0         - Normal end
 *  EFAULT    - Failed to copy data into internal buffer area
 *  ETIMEDOUT - Time out
 *  EIO       - Other errors occurred
 *
 * Notes:       This function is called by hfc_fx_ioctl()
 */
int hfc_fx_isolate_operation( struct port_info *pp, void *arg ) {	/* FCLNX-GPL-147 */
	int rtn=0;
	struct hfc_isol_info *isolinfo;
	struct region_info   *rp;
	unsigned long			flags = 0;
	struct hfc_pkt_fx		*issue_hfcp_top=NULL;

	HFC_ENTRY("hfc_fx_isolate_operation"); 								/* FCLNX-0488 */
	
	isolinfo = (struct hfc_isol_info *)hfc_fx_kmalloc(pp, sizeof(struct hfc_isol_info), GFP_ATOMIC);
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
		hfc_fx_kfree(pp, isolinfo);												/* FCLNX_GPL-0151 */
		return (EFAULT);												/* FCLNX-0488 */
	}

	if(isolinfo->version!=0) {
		hfc_fx_kfree(pp, isolinfo);												/* FCLNX_GPL-0151 */
		return(EINVAL);													/* FCLNX-0488 */
	}

	rp = pp->region_arg[pp->rid];

	switch(isolinfo->set_opr){
	case HFC_READ_ISOLPARAM:											/* FCLNX-0488 */

		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			rtn = hfc_manage_info.npubp->hfc_fx_get_isolparam(pp, isolinfo);	/* FCLNX-0488 */
		}
		else {
			rtn = hfc_fx_get_isolparam_i(pp, isolinfo, 0);					/* FCLNX_GPL-393 */
		}
		break;															/* FCLNX-0488 */

	case HFC_FORCE_ISOLATE: 											/* FCLNX-0488 */

		if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) ){				/* FCLNX-GPL-393 */

			pp->c_err = HFC_ISOLATE_FA;

			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

			/* only DMA transfer ends */
			rtn = hfc_fx_force_linkdown(pp, TRUE);	/* FCLNX-GPL-FX-043 */
		}
		else
		{
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			rtn = hfc_fx_mlpf_issue_fisolate(pp, HFC_ISSUE_ISOLREQ_CMD);
		}																/* FCLNX-GPL-393 */
		
		if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable()) {	/* FCLNX-0625 */
			hfc_manage_info.npubp->hfc_fx_deque_retry_hfcp(pp, &issue_hfcp_top);
			
			if(issue_hfcp_top != NULL){
				HFC_ALLCOREUNLOCK(rp);	/* FCLNX-GPL-FX-331 */
				HFC_PORTUNLOCK(pp);		/* FCLNX-GPL-FX-331 */
				hfc_manage_info.npubp->hfc_fx_retry_strategy(issue_hfcp_top);
				HFC_PORTLOCK(pp);		/* FCLNX-GPL-FX-331 */
				HFC_ALLCORELOCK(rp);	/* FCLNX-GPL-FX-331 */
			}
			
			if ( hfc_manage_info.wait_reset_mp_fx ) {						/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete();	/* FCLNX-0429 */
				hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete();	/* FCLNX-0429 */
			}
		}
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

		break;															/* FCLNX-0488 */

	case HFC_RECOV_ISOLATE:												/* FCLNX-0488 */
		
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
		}														/* FCLNX-GPL-331 */
		else{
			hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
		}
		
		if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) ){	/* FCLNX-GPL-393 */
			if(test_bit(HFC_PS_ISOL, (ulong*)&pp->status)){	/* FCLNX-GPL-FX-072 */
				HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
				clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);	/* FCLNX-GPL-FX-146 */
				rtn = hfc_fx_force_linkdown_recovery(pp);
				HFC_DBGPRT("hfc_fx_force_linkdown_recovery\n");
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			}	/* FCLNX-GPL-FX-072 */
		}
		else {
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			atomic_set(&pp->int_a_poll, 0);							/* FCLNX_0029 *//* FCLNX-GPL-521 */
			pp->initialize = 1;											/* FCLNX-GPL-521 */
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
				pp->after_isolrec = 1;								/* FCLNX-GPL-FX-462 */
			rtn = hfc_fx_mlpf_issue_recov_isolate(pp);
			HFC_DBGPRT("hfc_fx_mlpf_issue_recov_isolate\n");
			if(!(rtn)){	/* FCLNX_GPL-402 */							/* FCLNX-GPL-521 */
				if(!(isolinfo->immdt_cmd)){ /* FCLNX-0514 */
					HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
					hfc_fx_sleep_on(&pp->init_event, &pp->int_a_poll );								/* FCLNX-0269 */
					HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
				}
			}														/* FCLNX-GPL-521 */
			atomic_set(&pp->int_a_poll, 0);							/* FCLNX_0029 *//* FCLNX-GPL-521 */
			pp->initialize = 0;										/* FCLNX-GPL-521 */
			pp->no_target = 0;										/* FCLNX-GPL-570 */
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
				pp->after_isolrec = 0;								/* FCLNX-GPL-FX-462 */
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_mp_scan_dev(pp);			/* FC-GW 0331 */
				hfc_manage_info.npubp->hfc_fx_make_lgpath();
			}	/* FCLNX_GPL-402 */									/* FCLNX-GPL-521 */
		}

		break;															/* FCLNX-0488 */

	case HFC_STOP_ISOLATE:												/* FCLNX-GPL-349 */
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		if(hfc_fx_check_hba_isolation(pp)){								/* FCLNX-GPL-414 */
			pp->hba_isolation = HFC_ISOL_STOP;
		}else{
			
			rtn = EINVAL;
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		break;
	
	case HFC_START_ISOLATE:												/* FCLNX-GPL-349 */
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		hfc_fx_start_isolate(pp);
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		break;															/* FCLNX-GPL-349 */


	default:
		hfc_fx_kfree(pp, isolinfo);												/* FCLNX_GPL-0151 */
		return(EINVAL);													/* FCLNX-0488 */
	}

	if( COPYOUT( (char *)isolinfo, (char *)arg, sizeof(struct hfc_isol_info) ) != 0 ) {	/* FCLNX-0488 */
		HFC_DBGPRT( "ioctl error(trcid=0x%04x, subid=0x%04x) \n", HFC_TRC_IOCTL_HFC_HBA_ISOLATION, 0x02 );	//FCLNX-0488
		rtn = EFAULT;								/* FCLNX-0488 */
	}

	HFC_EXIT("hfc_fx_isolate_operation");				/* FCLNX-0488 */

	hfc_fx_kfree(pp, isolinfo);													/* FCLNX_GPL-0151 */

	return(rtn);									/* FCLNX-0488 */
}	/* FCLNX-GPL-147 */


/*
 * Function:    hfc_fx_wwn_info
 *
 * Purpose:     WWN/BOOT information setting
 *
 * Arguments:   
 *  pp        - Pointer to an port_info structure
 *  arg       - Pointer to a data area
 *
 * Returns:     
 *  0         - Normal end
 *  EFAULT    - Fail to copy data to buffer, or unable to attach data
 *  EIO       - Other errors occurred
 *
 * Notes:       
 */
int hfc_fx_wwn_info( struct port_info *pp, void *arg ) {
	return ENOTTY;
}
void hfc_fx_start_isolate(struct port_info *pp){
	uchar pre_status = 0;										/* FCLNX-GPL-349 */
	pre_status = pp->hba_isolation;
	if (pre_status == HFC_ISOL_STOP){
		if(hfc_fx_check_hba_isolation(pp) == HFC_ISOL_START){
			pp->hba_isolation = HFC_ISOL_START;
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
			}														/* FCLNX-GPL-331 */
			else{
				hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
			}
		}
	}
}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
void hfc_fx_change_dev_loss_tmo(struct port_info *pp)
{
	int					i=0;
	struct port_info	*wkpp;
	struct region_info	*rp = NULL;
	struct region_info	*wkrp = NULL;

	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		return;

	HFC_ENTRY("hfc_fx_change_dev_loss_tmo");
	
	/* physical port */
	rp = pp->region_arg[0];
	hfc_fx_chgange_rport_dev_loss_tmo(pp,rp);
		
	/* virtual port */
	for (i=1; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		if (HFC_FX_MQ_VIRTUAL_PORT(wkpp))
			continue;
		
		wkrp = pp->region_arg[wkpp->rid];
		if (wkrp == NULL)
			continue;
		
		hfc_fx_chgange_rport_dev_loss_tmo(wkpp,wkrp);
	}
	
	HFC_EXIT("hfc_fx_change_dev_loss_tmo");
	
	return ;
}

void hfc_fx_chgange_rport_dev_loss_tmo(struct port_info *pp, struct region_info *rp)
{
	struct target_info_fx	*target;
//	ulong				flags = 0;
	int					i=0;
	
	for (i=0;i<MAX_TARGET_PROBE;i++) {
//		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		target = hfc_fx_hash_target_valid(pp, i);
		/* delete rport */
		if(target){
			if( target->rport != NULL ){
				target->rport->dev_loss_tmo = pp->dev_loss_tmo;
			}
		}
		
//		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	}
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
#endif
