/*
 * hfcl_top.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char top_rcsid[] = "$Id: hfcl_top.c,v 1.37.2.14.2.11.2.2.2.3.6.15.6.10.2.7.2.4.2.1.2.2.2.3 2015/06/03 08:26:24 toyo Exp $";

#include "hfcldd.h"
#include "hfcl_mlpf.h"
#include "hfcl_tbol.h"
#include "hfcl_strategy.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_top.h"

extern struct manage_info hfc_manage_info;

uint hfc_uniq_seq_num( struct adap_info *ap );
void hfc_release_seq_num( struct adap_info *ap, int num );

int hfc_send_gpnid(struct adap_info *ap);
int hfc_send_login(struct adap_info *ap);
int hfc_send_pdisc(struct adap_info *ap);
int hfc_send_gidpn(struct adap_info *ap);
uchar hfc_first_timeout(struct adap_info *ap, struct wtimer *wtime,
		int *delay_num, int *link_num, int *rst_num, int *abt_num); 

void hfc_top_trace(	
	uchar               id,
	uchar               sub_id,
	struct adap_info    *ap,
	struct target_info  *target,
	uint64_t            etc1,
	uint64_t            etc2,
	uint64_t            etc3);


/*
 * Function:    hfc_copy_iocinfo
 *
 * Purpose:     Copy information from IOCINFO to adap_info.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *
 * Execution level: user/kernel/interrupt
 *
 * Notes:       Lock adap_info.
 *              <Setup the elements of ap : 
 *					ap->scsi_id,ww_name,node_name,used_nmsrv, 
 *					connect_type,max_data_rate >
 */
void hfc_copy_iocinfo( struct adap_info *ap )
{
	uchar flag;
	uint64_t port_id;

	flag    = (char    ) hfc_read_val( ap->fw_init_p->fw_iocinfo.flag );
	port_id = (uint64_t) hfc_read_val( ap->fw_init_p->fw_iocinfo.port_id );

	if ( flag & HFC_ALPA_VALID )						/* AL_PA			*/
		ap->scsi_id = (port_id >> 24) & 0xff;

	if ( flag & HFC_PID_VALID )							/* SCSI_ID			*/
		ap->scsi_id = port_id & 0xffffff;

	ap->host_alpa = (uchar) (ap->scsi_id & 0xff);

	ap->used_nmsrv = FALSE;
	ap->connect_type = (uchar) hfc_read_val( ap->fw_init_p->fw_iocinfo.connect_type );

	/* Setup max data rate to adap_info */
	switch ( hfc_read_val( ap->fw_init_p->fw_iocinfo.trans_rate ) ) {
		case HFC_10GBPS: ap->max_data_rate = HFC_1000MBS; break;
		case HFC_8GBPS : ap->max_data_rate = HFC_800MBS ; break;
		case HFC_4GBPS : ap->max_data_rate = HFC_400MBS ; break;
		case HFC_2GBPS : ap->max_data_rate = HFC_200MBS ; break;
		case HFC_1GBPS : ap->max_data_rate = HFC_100MBS ; break;
		default:		 ap->max_data_rate = HFC_100MBS ;
	}

	return;
}


/*
 * Function:    hfc_hash_target_valid
 *
 * Purpose:     Return the pointer to corresponding target_info
 *
 * Arguments:   
 *  p          - Pointer to adap_info
 *  target_id  - Target ID
 *
 * Returns:     
 *  != NULL    - Pointer to target_info
 *   = NULL    - No target_info exists
 *
 * Notes:       
 */
struct target_info *hfc_hash_target_valid(struct adap_info *ap, uint target_id )
{
	register struct target_info *target;

	target = ap->target_arg[ ap->tid_map[target_id] ];	

	while (target != NULL)
	{
		if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
		  && test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) )
		{
			/* Is target_id valid? */
			if (target->target_id == target_id)
			{
				break;
			}
		}
		
		target = target->next;
	}
	
	return (target);
}


/*
 * Function:    hfc_hash_target_info
 *
 * Purpose:     Return the pointer to the target_info for specified target_id
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  target_id  -
 *
 * Returns:     
 *  != NULL    - target_info pointer
 *  = NULL     - no target_info.
 *
 * Notes:       
 */
struct target_info *hfc_hash_target_info(struct adap_info *ap, uint target_id )
{
	register struct target_info *target;
	
	if ((target = hfc_hash_target_valid(ap, target_id)) != NULL)
	{
		if ( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )
			return (target);
	}
	
	return (NULL);
}


/*
 * Function:    hfc_pseq_target_info
 *
 * Purpose:     Return the pointer to the target_info for specified pseq#
 *
 * Arguments:   
 *  ap         - Pointer to adap_info structure
 *  psecq      - PSEQ#
 *
 * Returns:     
 *  != NULL    - target_info pointer
 *  = NULL     - no target_info.
 *
 * Notes:       
 */
struct target_info *hfc_pseq_target_info(struct adap_info *ap, uint pseq )
{
	register struct target_info *target;

	if ( pseq >= ap->max_target )
		return (NULL);

	target = ap -> target_arg[pseq];

	if( target != NULL )
	{
		if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags) )
			return (target);           /* Is target_info valid? */
	}

	return (NULL);
}


/*
 * Function:    hfc_wwpn_target_info
 *
 * Purpose:     Return the pointer to the target_info for specified pseq#
 *
 * Arguments:   
 *  ap         - Adap_info structure pointer
 *  ww_name    - 
 *
 * Returns:     
 *  != NULL    - target_info pointer
 *  = NULL     - no target_info.
 *
 *
 * Notes:       
 */
struct target_info *hfc_hash_target_info_wwn(struct adap_info *ap, uint64_t ww_name )
{
	struct target_info *target;
	uint i;

	for ( i=0; i<MAX_TARGET_PROBE; i++ )
	{
		target = ap->target_arg[ ap->tid_map[i] ];
		
		while (target != NULL)
		{
			if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
			 &&  test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)
			 &&  test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )
			{
				/* Is target_id valid */
				if (target->ww_name == ww_name)
					return (target);
			}

			target = target->next;
		}
	}
	
	return (target);
}


/*
 * Function:    hfc_hash_target_info_wwn_no_flag
 *
 * Purpose:     Return the pointer to the target_info for specified pseq#
 *
 * Arguments:   
 *  ap         - Adap_info structure pointer
 *  ww_name    - 
 *
 * Returns:     
 *  != NULL    - target_info pointer
 *  = NULL     - no target_info.
 *
 *
 * Notes:       
 */
struct target_info *hfc_hash_target_info_wwn_no_flag(struct adap_info *ap, uint64_t ww_name )
{
	struct target_info *target;
	uint i;

	for ( i=0; i<MAX_TARGET_PROBE; i++ )
	{
		target = ap->target_arg[ ap->tid_map[i] ];
		
		while (target != NULL)
		{
			if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
			 &&  test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) )
			{
				/* Is target_id valid */
				if (target->ww_name == ww_name)
					return (target);
			}

			target = target->next;
		}
	}
	
	return (target);
}


/*
 * Function:    hfc_uniq_seq_num
 *
 * Purpose:     Return unique sequence number for specified target
 *
 * Arguments:   
 *  ap         - Adap_info structure pointer
 *
 * Returns:     
 *  Value of 0 or more - seq#
 *
 * Notes:       
 */
uint hfc_uniq_seq_num( struct adap_info *ap )
{
	uint i;

	for ( i=0; i<ap->max_target; i++ )
	{
		if ( !( ap->seq_num_tbl[i / 32] & ( 1 << (i % 32)) ) ) {
			ap->seq_num_tbl[ i / 32 ] |= 1 << (i % 32);
			return ( i );
		}
	}
	return 0xffffffff;
}


/*
 * Function:    hfc_release_seq_num
 *
 * Purpose:     Release sequence number for target
 *
 * Arguments:   
 *  ap         - adap_info 
 *  num        - seq#
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_release_seq_num( struct adap_info *ap, int num )
{
	ap->seq_num_tbl[ (num / 32) ] &= ~( 1 << (num % 32) );
}


/*
 * Function:    hfc_read_cfg
 *
 * Purpose:     PCI configuration register read
 *
 * Arguments:   
 *  ap         - adap_info
 *  offset     - register offset
 *  reg_size   - read data size(1/2/4/8)
 *
 * Returns:    - readout of FPP register
 *  reg_size = 1 - least significant 1 byte
 *  reg_size = 2 - least significant 2 bytes
 *  reg_size = 4 - 4 bytes
 *
 * Execution level : user/kernel/interrupt
 *
 * Notes:      - Lock adap_info before calling this function
 */
uint64_t hfc_read_cnfg( struct adap_info *ap, uint offset, char reg_size )
{
	unsigned char	 byte;
	uint16_t word;
	uint32_t dword, data=0;

	switch( reg_size ){
	case 1:
		pci_read_config_byte( ap->pci_cfginf, offset, &byte );
		data = (uint32_t)byte;
		break;
	case 2:
		pci_read_config_word( ap->pci_cfginf, offset, &word );
		data = (uint32_t)word;
		break;
	case 4:
		pci_read_config_dword( ap->pci_cfginf, offset, &dword );
		data = (uint32_t)dword;
		break;
	}
	
	return( data );
}


/*
 * Function:    hfc_write_cnfg
 *
 * Purpose:     PCI configuration register write
 *
 * Arguments:   
 *  ap         - adap_info 
 *  offset     - register offset
 *  reg_size   - write size(1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_write_cnfg( struct adap_info *ap, uint offset,
										 char reg_size, uint64_t data )
{
	switch( reg_size ){
	case 1:
		pci_write_config_byte( ap->pci_cfginf, offset, (unsigned char)data );
		break;
	case 2:
		pci_write_config_word( ap->pci_cfginf, offset, (uint16_t)data );
		break;
	case 4:
		pci_write_config_dword( ap->pci_cfginf, offset, (uint32_t)data );
		break;
	default:
		break;
	}
}


/*
 * Function:    hfc_read_reg_ext
 *
 * Purpose:     FPP register lead 
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *  offset     - register offset address
 *  size       - read size(1/2/4 Bytes)
 *
 * Returns:     FPP register data
 *  reg_size = 1  least significant 1 byte
 *  reg_size = 2  least significant 2 bytes
 *  reg_size = 4  4 bytes
 *
 * Notes:  - Lock adap_info before calling this function
 *           Recommend to call hfc_read_reg() rather than calling this function
 */
uint64_t hfc_read_reg_ext(struct adap_info *ap, uint offset, char reg_size)
{
	uchar *ptr;
	ushort data16=0,rtn16=0;					/* FCLNX-0659 */
	uint   data32=0,rtn=0;

	if ( offset % reg_size ) {
		return (0);
	}

	ptr = ((uchar *) ap->mem_base_addr) + offset;

	switch (reg_size) {
		case 1: rtn = readb( (uchar  *) ptr );
				break;

		case 2: data16 = readw( (ushort *) ptr );
				HFC_2B_TO_2L(rtn16, data16);	/* FCLNX-0659 */
				rtn = rtn16;					/* FCLNX-0659 */
				break;

		case 4: data32 = readl( (uint   *) ptr );
				HFC_4B_TO_4L(rtn, data32);
				break;

		default: 
				break;
	}

	return(rtn);
}


/*
 * Function:    hfc_write_reg_ext
 *
 * Purpose:     PCI memory write
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *  offset     - register offset address
 *  reg_size   - write size (1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       Lock adap_info before calling this function
 *           	Recommend to call hfc_write_reg() rather than calling this function
 */
void hfc_write_reg_ext(struct adap_info *ap, uint offset,
									 char reg_size, uint64_t data)
{
	uchar *ptr;
	ushort data16_1,data16_2;
	uint   data32_1,data32_2;

	if ( offset % reg_size ) {
		return;
	}

	ptr = ((uchar *) ap->mem_base_addr) + offset;

	switch (reg_size) {
		case 1: writeb( (unsigned)data, (uchar  *) ptr ); 
				break;
		case 2: data16_1 = (ushort) data;
				HFC_2B_TO_2L(data16_2, data16_1);
				writew( (unsigned)data16_2, (uchar  *) ptr );
				break;
		case 4: data32_1 = (uint) data;
				HFC_4B_TO_4L(data32_2, data32_1);
				writel( (unsigned)data32_2, (uchar  *) ptr );
				break;

		default: 
				break;
	}

	return;
}


/*
 * Function:    hfc_read_flash
 *
 * Purpose:     Read flash memory 
 *
 * Arguments:   
 *  ap         - adap_info 
 *  offset     - flash offset address
 *  size       - Read size
 *  buf        - Readout data buffer pointer
 *
 * Returns:     
 *  TRUE       - Normal end
 *  FALSE      - Abnormal end
 *
 * Notes:       Lock adap_info before calling this function
 */
int hfc_read_flash(struct adap_info *ap, int offset, int size, uchar *buf)
{
	uint adr=0;
	uchar logdata[16];
	uint  wk_data[4];
	uint data1,data2,i;
	uint wk4;

	if ( ap->pkg.type == HFC_PKTYPE_FPP ) {
		adr = 0xa000000;							/* flash start address */
	}
	else {
		
		if(ap->pkg.type == HFC_PKTYPE_FIVE_EX) {
			/*** flag check (40sec Log-out) ***/ /* FCLNX-GPL-116 */
			i=0;
			while( hfc_read_reg(ap,HFC_IOSPACE_RAMADR,1) & 0x80)
			{
				/* 1ms wait */
				mdelay(1); 

				if( i == 40000 ){ /* 40sec */  /* FCLNX-GPL-134 */
					wk4 = 0x00000001;
					HFC_4L_TO_4B(wk_data[0], offset);
					HFC_4L_TO_4B(wk_data[1], size);
					HFC_4L_TO_4B(wk_data[2], wk4);
					wk_data[3] = 0;
					memcpy(logdata, (uchar*)wk_data, 16);
					
					hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ;/* FCLNX-GPL-161 */
						
/*					return(EIO); */ /* FCLNX-GPL-134 */
				}
				i++;
			}
		}

		/* Enable indirect access flag */
		hfc_write_reg(ap, HFC_IOSPACE_IDFLGEN,1,0x08);	/* @1.75 */

		/*** flag check (40sec Log-out) ***/ /* FCLNX-GPL-116 */
		i=0;
		while( hfc_read_reg(ap,HFC_IOSPACE_RAMADR,1) & 0x80)
		{
			/* 1ms wait */
			mdelay(1); 

			if( i == 40000 ){ /* 40sec */ /* FCLNX-GPL-134 */
				wk4 = 0x00000002;
				HFC_4L_TO_4B(wk_data[0], offset);
				HFC_4L_TO_4B(wk_data[1], size);
				HFC_4L_TO_4B(wk_data[2], wk4);
				wk_data[3] = 0;
				memcpy(logdata, (uchar*)wk_data, 16);
				
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ; /* FCLNX-GPL-161 */
					
/*				return(EIO); */ /* FCLNX-GPL-134 */
			}
			i++;
		}

		/* Notes: 
		 *  HFC_IOSPACE_RAMADR with bit 0=0 shows locked status in hardware. 
		 *  Other function can not access this flash area until releasing lock
		 */
		adr = 0xc000000;							/* Flash start address */
	}
	
	adr += offset;
	hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,1,0x20);		/* Type2 */

/*  Never use "HW auto address increment", since supported FIVE-EX.  */
/*	hfc_write_reg(ap, HFC_IOSPACE_RAMADR,4,adr); */		/* Set adderss */

	i=0;
	while (size) {
		int j=0;
		
		/* Always set RAMADR for FPP,FIVE,FIVE-EX */
		hfc_write_reg(ap, HFC_IOSPACE_RAMADR,4,adr);

		while (1) {
			/* Read data from the top of an indirect memory area + offset 0x100 */
			data1 = (uint)hfc_read_reg_ext( ap,
					 ap->pkg.map->iosp.reg[HFC_IOSPACE_INDAREA]+(adr % 0x100), 4);

			if ( data1 != 0 || j >= 1)
				break;
				
			/* if read data is '0' then retry */
			hfc_write_reg(ap, HFC_IOSPACE_RAMADR,4,adr);	/* Address re-setting */
			j++;
		}

		HFC_4B_TO_4L(data2, data1);
		adr+=4;

		if (size < 4) {
			memcpy(&buf[i],&data2,size);
			i+=size;
			size=0;
		}
		else {
			memcpy(&buf[i],&data2,4);
			i+=4;
			size-=4;
		}
	}
	hfc_write_reg(ap, HFC_IOSPACE_RAMMSK,1,0x00);

	if ( ap->pkg.type == HFC_PKTYPE_FPP )
		hfc_write_reg(ap, HFC_IOSPACE_RAMADR,4,0);
	else
	{
		/* Unlock indirect RAM access lock */
		hfc_write_reg(ap, HFC_IOSPACE_RAMADR,4,0x80000000);
		/* Disable indirect access flag */
		hfc_write_reg(ap, HFC_IOSPACE_IDFLGEN,1,0x00);	/* @1.75 */
	}
	
	return(0);
}


/*
 * Function:    hfc_read_tbl
 *
 * Purpose:     Read data from memory and convert data from big endian to little endian order
 *              Use data read for communcation area with FW, such as 
 *               fw_init_tbl, Mailbox, XOB, and XRB.
 *
 * Arguments:   
 *  ptr        - Read data address
 *  size       - Read data size(1/2/4/8)
 *
 * Returns:     Read data(right-aligned)
 *
 * Notes:       
 */
uint64_t hfc_read_tbl( void *ptr, char size )

{
	ushort	 data16=0;												   /* FCLNX-0659 */
	uint	 data32=0;												   /* FCLNX-0659 */
	uint64_t data64=0;

	switch(size) { /* FCLNX-669 */
		case 1:
			data64 = *((uchar *) ptr);
			break;
		case 2:
			HFC_2B_TO_2L(data16, (*((ushort *) ptr)));		/* FCLNX-0659 */
			data64 = (uint64_t)data16;						/* FCLNX-0659 */
			break;
		case 4:
			HFC_4B_TO_4L(data32, (*((uint *) ptr)));		/* FCLNX-0659 */
			data64 = (uint64_t)data32;						/* FCLNX-0659 */
			break;
		case 8:
			HFC_8B_TO_8L(data64, (*((uint64_t *) ptr)));
			break;
		default: 
			break;
	}

	return (data64);
}


/*
 * Function:    hfc_write_tbl
 *
 * Purpose:     Convert data from little endian to big endian and write converted data to memory
 *              Use data write for communcation area with FW, such as 
 *               fw_init_tbl, Mailbox, XOB, and XRB.
 *
 * Arguments:   
 *  ptr        - Write data address
 *  size       - Write data size(1/2/4/8)
 *  data       - Write data
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_write_tbl( void *ptr, char size, uint64_t data64 )
{
ushort data16 = (ushort) data64;
uint   data32 = (uint)   data64;

	switch(size) {
		case 1:  *((uchar *) ptr) = (uchar) data64;				break;
		case 2:  HFC_2L_TO_2B((*((ushort   *) ptr)),data16 );	break;
		case 4:  HFC_4L_TO_4B((*((uint     *) ptr)),data32 );	break;
		case 8:  HFC_8L_TO_8B((*((uint64_t *) ptr)),data64 );	break;
		default: 
				break;
	}
	return ;
}


/*
 * Function:    lock_mailbox
 *
 * Purpose:     Lock and clear mailbox 
 *
 * Arguments:   
 *  ap         -
 *
 * Returns:     
 *
 * Notes:       
 */
void lock_mailbox( struct adap_info *ap )
{
	int	rc;
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */
	
	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	rc = lock_try_mailbox( ap );
	while ( rc == 0 ) {				/* MailBox lock failure */
		set_bit( HFC_WAIT_LOCK_MB, (ulong *)&ap->mb_lock );

		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		hfc_sleep_on(&(ap->mb_lock_event), &(ap->mb_lock_event_wait));	/* FCLNX-0296 */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
		clear_bit( HFC_WAIT_LOCK_MB, (ulong *)&ap->mb_lock );
		rc = lock_try_mailbox( ap );
	}

	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
	HFC_BZERO( ( char * )( ap -> mb ), sizeof( struct mailbox ) / 8 );
	return;
}


/*
 * Function:    hfc_inta_mask_set
 *
 * Purpose:     Control interrupt mask 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  open_mask  - Bit mask to open interrupt mask register
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_inta_mask_set( struct adap_info *ap, uint open_mask ) {

	hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )open_mask );

	return;
}


/*
 * Function:    lock_try_mailbox
 *
 * Purpose:     Try to lock mailbox and clear the area if the lock succeeded.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  !=0        - Lock success (Mailbox is cleared)
 *  =0         - Lock failure
 *
 * Notes:       
 */
int lock_try_mailbox( struct adap_info *ap )
{
	int b;

	if ( !(b = test_and_set_bit(HFC_MAILBOX_BUSY, (ulong *)&( ap -> mb_lock ) ) ) ){
		HFC_BZERO( ( char * )( ap -> mb ), sizeof( struct mailbox ) / 8 );
		b=1;
	}
	else b=0;

	return (b);
}


/*
 * Function:    unlock_mailbox
 *
 * Purpose:     Unlock mailbox 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *
 * Notes:       
 */
void unlock_mailbox( struct adap_info *ap )
{

	HFC_MAILBOX_UNLOCK( ap, HFC_MAILBOX_BUSY);
	HFC_DBGPRT( "unlock_mailbox ap->status : %x\n", ap->status);
	
	if( test_bit( HFC_WAIT_LOCK_MB, (ulong *)&ap->mb_lock ) )
	{
		HFC_DBGPRT( "unlock_mailbox wake up event\n");
		hfc_wake_up( &ap->mb_lock_event, &ap->mb_lock_event_wait );					/* FCLNX-0296 */
	}

	if( !test_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status ) ) {
		HFC_DBGPRT( "unlock_mailbox start next mailbox 1 \n");
		start_next_mailbox( ap );									/* FCWIN-0297 */
	}
	else {
		if ( !ap->initialize ) {
			if ( (ap->connect_type == HFC_SWITCH )
				|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00)) ) {
				HFC_DBGPRT( "unlock_mailbox start next mailbox 2\n");
				start_next_mailbox( ap );							/* FCWIN-0297 */
			}
		}
	}																/* FCLNX-0270 */
	
	return;
}


/*
 * Function:    lock_try_mpap
 *
 * Purpose:     Try to lock mp_adap_info.
 *
 * Arguments:   
 *  ap         - pointer to adap_info 
 *
 * Returns:     
 *  !=0        - Lock success (The mp_adap_info area is cleared)
 *  = 0        - Lock failure
 *
 * Notes:       
 */
int lock_try_mpap( struct mp_adap_info *mpap )
{
	int b;

	if ( !(b = test_and_set_bit(HFC_MP_ADAP_BUSY, (ulong *)&mpap->tbl_lock ) ) ){
		b=1;
	}
	else b=0;

	return (b);
}


/*
 * Function:    start_next_mailbox
 *
 * Purpose:     Initiate the target waiting mailbox lock.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *
 * Returns:     
 *
 * Notes:       Lock adap_info before calling this function
 */
void start_next_mailbox( struct adap_info *ap )
{

	if ( ap -> next_gidpn == TRUE ) {				/* Waiting GIDPN */
		if ( !hfc_send_gidpn( ap ) )
			return;
	}

	if ( ap -> login_target != NULL ) {				/* Waiting LOGIN */
		if ( hfc_send_login( ap ) )
			return;
	}

	if ( ap -> next_tstart != NULL ) {				/* Waiting PDISC */
		if ( hfc_send_pdisc( ap ) )
			return;
	}

	if( test_bit( HFC_NEED_NMSRV, (ulong *)&ap -> status ) ){/* Waiting GID_FT */
		( void )hfc_issue_gidft( ap );
		return;
	}

	if( test_bit( HFC_NEED_GPNID, (ulong *)&ap -> status ) ){/* Waiting GPN_ID */
		if ( hfc_send_gpnid( ap ) )
			return;
	}
}


/*
 * Function:    hfc_mailbox_initiate
 *
 * Purpose:     Initiate Mailbox request
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  caller     - Caller level 
 *                HFC_MB_PROL  Process level
 *                HFC_MB_INTL  Interrupt level
 *
 * Returns:     
 *
 * Notes:       Secure the lock of adap_info
 */
void hfc_mailbox_initiate( struct adap_info *ap, uint caller )
{

	set_bit(caller, (ulong *)&ap->mb_status);
	ap -> mb_resp	  = 0; 	
	ap -> mb_results  = 0;

	HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_mailbox_initiate - entry\n", ap->dev_minor);

	/* Mailbox start */
	hfc_write_reg( ap, ( uint )HFC_IOSPACE_FRAMEA, 4, HFC_FRAMEA_MB_INIT );

	HFC_DBGPRT("hfcldd : hfcl_top, hfc_mailbox_initiate - exit\n");
	
	return;
}

#define MAILBOX_TEMPORARY_BUSY 1
#define MAILBOX_ERROR 2
#define MAILBOX_FATAL -1


/*
 * Function:    hfc_mailbox_response
 *
 * Purpose:     Evaluate Mailbox response
 *
 * Arguments:   
 *  ap             - Pointer to adap_info
 *  xcc            - Pointer in area where condition code is stored
 *  fsb_error_code - Pointer in area where status byte and error code are stored
 *
 * Returns:     
 *  0                       - Normal end
 *  MAILBOX_ERROR           - Abnormal end
 *  MAILBOX_FATAL           - Unable to operate
 *  MAILBOX_TEMPORARY_BUSY  - Busy state
 *
 * Notes:       	Lock Mailbox before calling 
 */
int hfc_mailbox_response( struct adap_info *ap, uchar *xcc, uint *fsb_error_code )
{
	int rtn = 0;
	uchar fsb;

	/* Is mailbox response valid? */
	if ( hfc_read_val(ap->mb->mb_resp.flag) != 0x80 ) /* valid bit off? */
		return MAILBOX_FATAL;

	/* Store condition code */
	*xcc = (uchar) hfc_read_val( ap -> mb -> mb_resp.xcc );
    *fsb_error_code = 0;

	switch ( *xcc ) {
	case 0x80:	/* FPP completed SCSI operation */
		fsb = (uchar) hfc_read_val( ap -> mb -> mb_resp.fsb );
		if ( fsb != 0 ) {
			*fsb_error_code = ( fsb << 24 ) +
				( (uint) hfc_read_val( ap->mb->mb_resp.err_code[0] ) << 16 ) +
				( (uint) hfc_read_val( ap->mb->mb_resp.err_code[1] ) << 8 ) +
				  (uint) hfc_read_val( ap->mb->mb_resp.err_code[2] );
			rtn = MAILBOX_ERROR;

			if ( fsb == 0x04 )
				rtn = MAILBOX_FATAL;
		}
		ap -> link_dead_cnt = 0;
		break;
	case 0x82:/* FPP is temporarily busy	*/
		rtn = MAILBOX_TEMPORARY_BUSY;
		break;
	case 0x83:/* FPP is inoperative */
		rtn = MAILBOX_FATAL;
		break;
	default:
		rtn = MAILBOX_FATAL;
	}

	hfc_write_val( ap->mb->mb_resp.flag, 0 ); /* Clear response */
	return rtn;
}


/*
 * Function:    hfc_mb_passthru
 *
 * Purpose:     Start mailbox
 *
 * Arguments:   
 *  ap         - adap_info 
 *  timer_id   - Timer ID
 *  restart    - Timer value
 *  retry      - Retry count in temporary busy 
 *  caller     -
 *
 * Returns:     
 *  0          - Start success
 *  EIOF       - Start failure (MCK recovery process is in progress )
 *
 * Notes:       
 */
int hfc_mb_passthrough(struct adap_info *ap, ushort timer_id, ulong restart, int retry, uint caller)
{
	uint rtn;

        if( (test_bit( HFC_WAIT_T3, (ulong *)&ap->status ) ) ||
            (test_bit( HFC_MCK_RECOVERY, (ulong *)&ap->status ) ) ||
            (test_bit( HFC_ISOL_RECOVERY, (ulong *)&ap->status) ) ||
            (test_bit( HFC_ISOL, (ulong *)&ap->status) ) ||	/* FCLNX-GPL-572 */
            !hfc_mlpf_check_normal_hypsts(ap))	/* FCLNX-GPL-428 */
                return (EIOF) ; /* Abnormal end */

	/* Start watchdog timer */
	if ( (rtn = hfc_watchdog_enter( ap , NULL, NULL, 0, (uchar)timer_id, restart, FALSE)) )
	{
		/* This timer ID is already registered or timer ID is invalid */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB3, NULL, 0) ;
		return (EIOF);
	}

	ap -> mb_retry_cnt  = retry;
	ap -> mb_retry_tid  = timer_id;
	ap -> mb_retry_tout = restart;

	hfc_mailbox_initiate( ap, caller );
	return (0);
}


/*
 * Function:    hfc_mb_passthrough_rsp
 *
 * Purpose:     Start mailbox
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  caller     - Caller level 
 *                HFC_MB_PROL    Process level
 *                HFC_MB_INTL    Interrupt level
 *                HFC_MB_POLLING Polling procss 
 * Returns:     
 *  0          - Normal end
 *  EIOF       - Abnormal end
 *  ETIMEDOUT  - Mailbox timeout occured
 *  ERETRY     - F/W Busy response
 *
 * Notes:       Lock mailbox before calling this function
 */
int hfc_mb_passthrough_rsp(struct adap_info *ap, uint caller)
{
	int rtn;								/* return code */
	struct mblog {
		uint fsb_error_code;				/* status byte, error code */
		uchar xcc;							/* condition code */
	} mblog;

	/* Does mailbox request timeout? */
	if ( ap -> mb_resp == HFC_MBR_TIMEOUT ) {
		ap -> mb_status = 0;
		set_bit(HFC_MB_LOCK, (ulong *)&ap -> mb_status );
		return (ap -> mb_results = HFC_MBPASS_TIMEDOUT) ;  		/*-- FCLNX-016 --*/
	}

	/* Stop timer and delete ID */
	hfc_watchdog_enter( ap , NULL, NULL, 0, (uchar) ap->mb_retry_tid, 0, TRUE);

	/* Check mailbox response */
	if ( ( rtn = hfc_mailbox_response( ap,
			 &( mblog.xcc ), &( mblog.fsb_error_code ) ) ) == 0 ) {
		ap -> mb_status = 0;
		set_bit(HFC_MB_LOCK, (ulong *)&ap -> mb_status );
		return (ap -> mb_results = 0);
	}

	ap -> mb_results = ( int )mblog.fsb_error_code;

	/* FSB demands retry or TEMPORARY_BUSY */
	if ( (mblog.fsb_error_code & 0x80000000 )
	 || (rtn == MAILBOX_TEMPORARY_BUSY) ) {
		if ( !ap -> mb_retry_cnt ) {			/* retry out */
			ap->mb_status = 0;
			set_bit(HFC_MB_LOCK, (ulong *)&ap->mb_status );
			return (HFC_MBPASS_RETRY_OVER) ;	/* Abnormal end	*/ /*-- FCLNX-016 --*/
		}

		ap -> mb_resp	   = 0;	/* Clear response */
		ap -> mb_results   = 0;
		ap -> mb_retry_cnt--;

		if ( hfc_mb_passthrough( ap,
					ap->mb_retry_tid,
					ap->mb_retry_tout,
					ap->mb_retry_cnt,
					caller ) ){
			ap->mb_status = 0;
			set_bit(HFC_MB_LOCK, (ulong *)&ap -> mb_status );
			return (HFC_MBPASS_RETRY_FAIL);		/*-- FCLNX-016 --*/
		}
		return (HFC_MBPASS_WAIT_RETRY);			/*-- FCLNX-016 --*/
	}

	if ( rtn == MAILBOX_FATAL ) {
		if (caller != HFC_MB_INTL) {
			hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_MBRESP,
			 	ERRID_HFCP_ERR6, 0x52, ( uchar * )&mblog, sizeof( struct mblog ) );
			hfc_abend( ap, HFC_ABEND_MB_RSPERR );
		}
		return HFC_MB_FATAL;					/*-- FCLNX-016 --*/
	}

	return HFC_MBPASS_ERROR;					/*-- FCLNX-016 --*/
}	


/*
 * Function:    hfc_mailbox_proc
 *
 * Purpose:     Completion is waited for by starting the mailbox
 * Arguments:   
 *  ap          - Pointer to adap_info 
 *  target_info - Pointer to target
 *  timer_id    - timer ID
 *  restart     - Timer value
 *  retry       - Retry count in temporary busy 
 *
 * Returns:     
 *  0           - Normal end
 *  ETIMEDOUT   - Timeout occured at mailbox initiation 
 *  EIO         - Other errors occurred
 *
 * context:     user
 *
 * Notes:       Caller must lock adap_lock,mbxini_lock.
 *              This function clears ap->mb_status and mailbox response flag
 */
int hfc_mailbox_proc( struct adap_info *ap,
						ushort timer_id,ulong restart, int retry )
{
	int rtn=0, wrtn;			/* return code */
	int cnt;					/* retry counter */
	struct mblog {
		uint fsb_error_code;	/* status byte, error code */
		uchar xcc;				/* condition code */
	} mblog;
	ulong 				flags = 0;	/* FCLNX-GPL-FX-466 */

	HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
	if(!(test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ) ){				/* FCLNX-0228 STR */
		HFC_DBGPRT(" hfcldd : hfcl_top; hfc_mailbox_proc - hfc_hwinit_fail \n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		return(EIO);
	}																			/* FCLNX-0228 END */

	if( test_bit(HFC_CHK_STOP, (ulong *)&ap->status) ||
		test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status) ||
		test_bit(HFC_ISOL, (ulong *)&ap->status) ||
		test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status) ||
		!hfc_mlpf_check_normal_hypsts(ap) ){	/* FCLNX-GPL-428 */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		
		return(EIO);
	}
			
	for ( cnt = retry; cnt >= 0; cnt -- )										/* FCLNX-GPL-0343 */
	{
		/* Set watchdog timer */
		if ( (wrtn = hfc_watchdog_enter( ap,NULL,NULL, 0,timer_id,restart,0 )) )
		{
			/* Timer ID is already registered or invalid */
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB4, NULL, 0) ;
		}

		if( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status ) ||
			test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status ) )
		{
			/* Machine check recovery process is in progress */
			if( !test_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status ) )
			{
				/* Stop timer and delete ID */
				hfc_watchdog_enter( ap,NULL,NULL,0,timer_id,0,1 );

				/* Return EIO except link initialization case */
				HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
				return(EIO) ;
			}

			/* Wait until Machine check recovery is completed. */
			ap->mck_on_sleep = 1;
			HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
			hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait);					/* FCLNX-0296 */
			HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
			ap->mck_on_sleep = 0;

			/* Return EIO if the status is check stop after retry */
			if(test_bit(HFC_CHK_STOP, (ulong *)&ap->status ) )
			{
				/* Stop timer and delete ID */
				hfc_watchdog_enter( ap,NULL, NULL, 0,timer_id,0,1 );
				HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
				return( EIO ) ;
			}
			
			if ( test_bit(HFC_ISOL, (ulong *)&ap->status ) ){
				/* Stop timer and delete ID */
				hfc_watchdog_enter( ap,NULL,NULL, 0,timer_id,0,1 );
				HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
				return( EIO ) ;
			}
		}

		/* Start mailbox  */
		hfc_mailbox_initiate( ap, HFC_MB_PROL );

		/* Wait for mailbox request completion */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		hfc_sleep_on(&ap->mb_event, &ap->mb_event_wait);
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
		/* Stop timer and delete ID */
		hfc_watchdog_enter( ap,NULL,NULL,0,timer_id,0,1 );
		
		/* Record time */
		ap->mb_prol_sleep_end_time = (uint)jiffies; /* FCLNX-GPL-243 */
		
		/* Watchdog timer timeout? */
		if ( ap -> mb_resp == HFC_MBR_TIMEOUT )
		{
			hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT,ERRID_HFCP_EVNT4, 0x51, NULL, 0 );
			hfc_abend( ap, HFC_ABEND_MB_TOUT );
			ap -> mb_resp = 0;					/* Clear */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
			return (ap -> mb_results = ETIMEDOUT );
		}

		/* Check Mailbox response */
		if ( ( rtn = hfc_mailbox_response( ap,	
				&( mblog.xcc ), &( mblog.fsb_error_code ) ) ) == 0 )
		{
			ap->mb_status = 0;	/* clear */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
			return ( ap -> mb_results = 0 ) ;
		}

		/* FSB demands retry? */
		if ( mblog.fsb_error_code & 0x80000000 )
			continue;

		/* Retry only in TEMPORARY_BUSY case */
		if ( rtn != MAILBOX_TEMPORARY_BUSY )
			break;
	}

	ap->mb_status = 0;	/* Clear */
	if ( rtn == MAILBOX_FATAL ) {
		hfc_errlog( ap, NULL, NULL, HFC_ERRLOG_TYPE_MBRESP,
			 ERRID_HFCP_ERR6, 0x52, ( uchar * )&mblog, sizeof( struct mblog ) );
		hfc_abend( ap, HFC_ABEND_MB_RSPERR );
	}

	ap -> mb_results = ( int )mblog.fsb_error_code;
	HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */

	return EIO;

}	


/*
 * Function:    hfc_issue_linkini
 *
 * Purpose:     link initialization 
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as ap->mb_status
 *               Link initialization is in progress : HFC_WAIT_LINK_INIT
 *               Link initialization ended normally : HFC_ONLINE
 *               Link initialization failed         : HFC_WAIT_LINKUP
 */
int hfc_issue_linkini( struct adap_info *ap )
{
	uint rtn;
	uchar alpa;

	HFC_DBGPRT(" hfcldd : hfc_top 0; hfc_issue_linkini - entry \n"); 
	
	hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x00, ap, 0, 0, 0, 0 );

	if( test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status ) ){
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x81, ap, 0, 0, 0, 0 );
		return -1 ;
	}
	
	if( test_bit(HFC_ISOL, (ulong *)&ap->status ) ){ /* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x86, ap, 0, 0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x85, ap, 0, 0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_ISOL_RECOVERY, (ulong *)&ap->status ) ){
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x84, ap, 0, 0, 0, 0 );
		return -1 ;
	}

	if ( !(lock_try_mailbox( ap )) ) {
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x82, ap, 0, 0, 0, 0 );	
		return -1;									/* MailBox lock fail	*/
	}

	/* Start timer */
	hfc_w_stop( ap, HFC_LINKINIT_TMR );
	if ( (rtn = hfc_w_start( ap, HFC_LINKINIT_TMR )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_LINKINIT, 0x83, ap, 0, 0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FPPCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_LINKINIT );
	alpa = (ap->host_alpa) ? ap->host_alpa : ap->pref_alpa;						/* FCLNX-646 */
	hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.al_pa, alpa );	/* FCLNX-646 */
	hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_speed, ap->linkspeed );
	hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.connect_type, ap->topology ); 

	/* FCLNX-GPL-FX-366 */
	if( HFC_MMODE_CHECK_SHARED(ap) && !(HFC_MMODE_CHECK_SHADOW(ap) ) ) {
		if ((ap->drvctl & HFC_LOGIN_TARGET_FILTER_EXT)  &&
			((uchar)hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1) & 0x10)) {
			
			if( test_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support) ){		/* FCLNX-GPL-570 */
				/* Invalidate PortID Guard */
				hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_ini_opt, 0x40 | HFC_DISABLE_LINKINI_DELAY);
			}else{
				hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_ini_opt, 0x40 );
			}
		}
		else {
			/* PortID Guard */
			if( test_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support) ){		/* FCLNX-GPL-570 */
				hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_ini_opt, HFC_DISABLE_LINKINI_DELAY ); 
			
			}else{
				hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_ini_opt, 0 );
			}
		}
	}else if( test_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support) ){		/* FCLNX-GPL-570 */
		hfc_write_val( ap -> mb -> mb_init.type.drvctl.un.link_init.link_ini_opt, HFC_DISABLE_LINKINI_DELAY ); 
	}																			/* FCLNX-GPL-570 */

	/* Clear NEED_LINK_INIT and set WAIT_LINK_INIT */
	set_bit( HFC_WAIT_LINK_INIT, (ulong *)&ap->status );
	clear_bit( HFC_NEED_LINK_INIT, (ulong *)&ap->status );
	clear_bit( HFC_ONLINE, (ulong *)&ap->status );
	ap -> mb_retry_cnt  = HFC_LINK_INIT_RETRY ;
	ap -> mb_retry_tid  = HFC_LINKINIT_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_mailbox_initiate( ap, HFC_MB_INTL );

	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_issue_linkini - exit \n"); 

	return 0 ;
}


/*
 * Function:    hfc_issue_gidft
 *
 * Purpose:     Issue GID_FT 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  = 0        - Start succeeded
 *  !=0          Start failed
 *               (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * Notes:        Lock adap_info before calling this function
 */
int hfc_issue_gidft( struct adap_info *ap )
{
	uint did,rtn;
	struct drv_ioctl2 *drv_ioctl2_p;
	
	HFC_DBGPRT("hfc: hfc_issue_gidft start \n");

	hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x00, ap, 0, 0, 0, 0 );		

	if( !(test_bit(HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x81, ap, 0, 0, 0, 0 );	
		return -1;
	}
	
	if( test_bit(HFC_ISOL, (ulong *)&ap->status ) ){ /* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x85, ap, 0, 0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x84, ap, 0, 0, 0, 0 );	
		return -1;
	}

	if ( !(lock_try_mailbox( ap )) ) {	/* Mailbox lock failed		*/
		hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x82, ap, 0, 0, 0, 0 );	
		return -1;
	}

	/* Start timer */
	hfc_w_stop( ap, HFC_ELS_TMR );
	if ( (rtn = hfc_w_start( ap, HFC_ELS_TMR )) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB6, NULL, 0) ;
		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_GIDFT, 0x83, ap, 0, 0, 0, 0 );	
		return -1;
	}

	/* Clear mp_parm */
	memset( ap->mb_parm, 0, sizeof(struct FS_ACC) );

	/* Setup mailbox control block */
	drv_ioctl2_p = &ap->mb->mb_init.type.drvioctl2;
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FCCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_NMSRV );

	did = HFC_DIR_SERV_ID ;
	if( ap->connect_type == HFC_AL )
		did |= HFC_DIR_SERV_ID  << 24;

	hfc_write_val( drv_ioctl2_p->adr.d_id, did );
	hfc_write_val( drv_ioctl2_p->payload_length, 20 );
	hfc_write_val( drv_ioctl2_p->flag, 0 );
	hfc_write_val( drv_ioctl2_p->response_length, HFC_PAGE_SIZE );
	hfc_write_val( drv_ioctl2_p->response_list_address, ap->phys_mb_parm );
	hfc_write_val( drv_ioctl2_p->un.gid_ft.rev, 1 );				/* FC_GS_2	  */
	hfc_write_val( drv_ioctl2_p->un.gid_ft.fs_type, 0xfc );			/* FC_CT_DIR_SERV */
	hfc_write_val( drv_ioctl2_p->un.gid_ft.fs_sub, 0x02 );			/* FC_NAME_SERVER */
	hfc_write_val( drv_ioctl2_p->un.gid_ft.options, 0 );
	hfc_write_val( drv_ioctl2_p->un.gid_ft.com_rsp_code, 0x171 );	/* FC_NS_GID_FT   */
	hfc_write_val( drv_ioctl2_p->un.gid_ft.max_res_size, 4064/4 );
	hfc_write_val( drv_ioctl2_p->un.gid_ft.type, 0x08 );			/* FCPH_TYPE_FCP  */

	/* Wait NMSRV response */
	set_bit(HFC_WAIT_NMSRV, (ulong *)&ap-> status );
	clear_bit(HFC_NEED_NMSRV, (ulong *)&ap-> status );

	/* Setup callback information and retry count */
	ap -> mb_retry_cnt  = ap->els_retry ;		/* FCLNX-0523 */
	ap -> mb_retry_tid  = HFC_ELS_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_mailbox_initiate(ap, HFC_MB_INTL);
	return 0;
}	


/*
 * Function:    hfc_issue_gidpn
 *
 * Purpose:     Issue GID_PN
 *
 * Arguments:   
 *  ap         - Pointer to adap_info 
 *  target     - SCSI ID for GPN_ID 
 *
 * Returns:     
 *  = 0        - Start succeeded
 *  !=0          Start failed
 *               (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * Notes:       
 */
int hfc_issue_gidpn( struct adap_info *ap, struct target_info *target ) 
{
	uint did,rtn;

	hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x00, ap, target, 0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_issue_gidpn start \n");

	if( !(test_bit( HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x81, ap, target, 0, 0, 0 );
		return -1;
	}
	
	if( test_bit(HFC_ISOL, (ulong *)&ap->status ) ){ /* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x85, ap, target, 0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x84, ap, target, 0, 0, 0 );	
		return -1;
	}

	if ( !(lock_try_mailbox( ap ) )) {	/* Mailbox lock failed */
		hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x82, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Start timer */
	hfc_w_stop( ap, HFC_ELS_TMR );
	if ( (rtn = hfc_w_start( ap, HFC_ELS_TMR )) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB7, NULL, 0) ;

		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_GIDPN, 0x83, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FCCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_GIDPN );
	hfc_write_val( ap -> mb -> mb_init.pseq_no, target -> pseq );

	did = HFC_DIR_SERV_ID ;
	if( ap->connect_type == HFC_AL )
		did |= HFC_DIR_SERV_ID  << 24;

	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.adr.d_id, did );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.payload_length, 24 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.rev, 1 );		 /*FC_GS_2*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.fs_type, 0xfc ); /*FC_CT_DIR_SERV*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.fs_sub, 0x02 );	 /*FC_NAME_SERVER*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.options, 0 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.com_rsp_code, 0x121 );/*FC_NS_GID_PN*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.max_res_size, 1 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gid_pn.port_name, (target -> ww_name));
	ap -> mb_results = 0;

	clear_bit( HFC_NEED_GIDPN, (ulong *)&target -> status );
	set_bit( HFC_WAIT_GIDPN, (ulong *)&target -> status );

	/* Setup callback information and retry count */
	ap -> mb_retry_cnt  = ap->els_retry ;	/* FCLNX-0523 */
	ap -> mb_retry_tid  = HFC_ELS_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_mailbox_initiate( ap, HFC_MB_INTL );
	return 0;
}	


/*
 * Function:    hfc_issue_gpnid
 *
 * Purpose:     Issue GPN_ID
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  scsi_id    - SCSI ID for GPN_ID 
 *
 * Returns:     
 *  = 0        - Start succeeded
 *  !=0          Start failed
 *               (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 * Notes:       
 */
int hfc_issue_gpnid( struct adap_info *ap, uint scsi_id )
{
	uint did,rtn;

	hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x00, ap, 0, 0, 0, 0 );	
	
	HFC_DBGPRT("hfc: hfc_issue_gpnid start \n");

	if( !(test_bit( HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x81, ap, 0, 0, 0, 0 );	
		return -1;
	}
	
	if( test_bit(HFC_ISOL, (ulong *)&ap->status ) ){ /* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x85, ap, 0, 0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x84, ap, 0, 0, 0, 0 );	
		return -1;
	}

	if ( !(lock_try_mailbox( ap )) ) {			/* Mailbox lock failed		*/
		hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x82, ap, 0, 0, 0, 0 );	
		return -1;
	}

	/* Start timer */
	hfc_w_stop( ap, HFC_ELS_TMR );
	if ( (rtn = hfc_w_start( ap, HFC_ELS_TMR )) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB9, NULL, 0) ;
		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_GPNID, 0x83, ap, 0, 0, 0, 0 );	
		return -1;
	}

	/* Setup mailbox control block */
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FCCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_GPNID );

	did = HFC_DIR_SERV_ID ;
	if( ap->connect_type == HFC_AL )
		did |= HFC_DIR_SERV_ID  << 24;

	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.adr.d_id, did );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.payload_length, 20 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.rev, 1 );		 /*FC_GS_2*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.fs_type, 0xfc ); /*FC_CT_DIR_SERV*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.fs_sub, 0x02 );	 /*FC_NAME_SERVER*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.options, 0 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.com_rsp_code, 0x112 );/*FC_NS_GPN_ID*/
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.max_res_size, 2 );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.gpn_id.port_id, (scsi_id & 0xffffff));

	clear_bit( HFC_NEED_GPNID, (ulong *)&ap -> status );
	set_bit( HFC_WAIT_GPNID, (ulong *)&ap -> status );

	/* Setup callback information and retry count */
	ap -> mb_retry_cnt  = ap->els_retry ;		/* FCLNX-0523 */
	ap -> mb_retry_tid  = HFC_ELS_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default Value */

	/* Start Mailbox */
	hfc_mailbox_initiate( ap, HFC_MB_INTL );
	return 0;
}	


/*
 * Function:    hfc_issue_relogin
 *
 * Purpose:     Reissue Login request
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Target is not online, or failed to acquire lock
 *
 * Notes:       
 */
int hfc_issue_relogin( struct adap_info *ap,struct target_info *target )
{
uint rtn,did;
uint hash;
struct hfc_pkt  *hfcp = NULL;

	hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x00, ap, target, 0, 0, 0 );	
	
	HFC_DBGPRT("hfcldd%d: hfc_issue_relogin start.\n", ap->dev_minor);

	if( !(test_bit( HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x81, ap, target, 0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_ISOL, (ulong *)&ap -> status )  ){	/* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x8a, ap, target, 0, 0, 0 );
		return -1;
	}

	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x89, ap, target, 0, 0, 0 );	
		return -1;
	}

	if( (test_bit( HFC_WAIT_LINKUP, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x85, ap, target, 0, 0, 0 );
		return -1;
	}

	if( (test_bit( HFC_SCN_WLINKUP, (ulong *)&target -> status ) ) 
	&& !(test_bit( HFC_NEED_CANCEL, (ulong *)&target -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x86, ap, target, 0, 0, 0 );
		return -1;
	}

	if( (test_bit( HFC_WAIT_LOGIN, (ulong *)&target -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x82, ap, target, 0, 0, 0 );
		return -1 ;
	}

	if( (test_bit( HFC_WAIT_CANCEL, (ulong *)&target -> status ) ) ){			/* FCLNX-GPL-038 */
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x88, ap, target, 0, 0, 0 );
		return -1 ;
	}																			/* FCLNX-GPL-038 */

	if ( !(lock_try_mailbox( ap )) ) {			/* Mailbox lock fail */
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x83, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Start watchdog timer */
	if ( ( rtn = hfc_watchdog_enter( ap,target,NULL, 0,HFC_ELS_TMR,0,FALSE ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB8, NULL, 0) ;
		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x84, ap, target, 0, 0, 0 );
		return -1;
	}

	hfc_deque_login_req( ap, target );

	/* Cancel XOB initiation of this target */
	if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)		/* FCLNX-GPL-367 *//* FCLNX-GPL-380 */
		&& test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags)
		&& test_bit(HFC_WWN_VALID, (ulong *)&target->flags) )
	{
		hfc_cancel_xob( ap, target, 0, NULL, HFC_FLASH_TARGET);		/* FCLNX-0095 */
		hfc_top_trace( HFC_TRC_ISSUE_RELOGIN, 0x87, ap, target, 0, 0, 0 );
	}																/* FCLNX-GPL-367 *//* FCLNX-GPL-380 */

	/* Setup mailbox control block */
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FCCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_LOGIN );

	ap->login_type = HFC_SCAN_DEVICE;	/* FCLNX-0571 */

	if( (hfc_read_val( ap->fw_init_p->func) & HFC_FWF_EXTPLOGI)
		&& (test_bit(HFC_NEED_CANCEL, (ulong *)&target->status)) ){
		hfc_write_val(ap->mb->mb_init.dependent_code, 0x8000 );
		ap->login_type = HFC_CANCEL_LOGIN;	/* FCLNX-0571 */
	}


	hfc_write_val( ap -> mb -> mb_init.pseq_no, target -> pseq );
	did = ( uint )( target -> scsi_id & 0x00ffffff );

	if ( ap -> connect_type == HFC_AL )
		did |= ( target -> scsi_id & 0x00ff ) << 24;

	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.adr.d_id, did);
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.payload_length, 20);
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.pagelen, 16);
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.payload_length, 20);
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.fscsi, 0x08);
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.prli, 0x20);
	/* Set FCP read transfer ready and SCSI Initiator */
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.parameter, 
	(0x0002 |
	 0x0020 |
	 0x0080 |
	 0x0100));

	/* Establish an image pair. */
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.un.login.flag, (0x20<<8));

	/*
	 * Turn on mailbox flag if HFC_NEED_CANCE is set
	 */

	/* Set target status to LOGIN wait and cancel XOB initiation of all devices under this target */
	if( test_bit( HFC_NEED_CANCEL, (ulong *)&target -> status ) ){			/* FCLNX-GPL-038 */
		set_bit( HFC_WAIT_CANCEL, (ulong *)&target -> status );
		clear_bit( HFC_NEED_CANCEL, (ulong *)&target -> status );
	}
	else {
		set_bit( HFC_WAIT_LOGIN, (ulong *)&target -> status );
		clear_bit( HFC_NEED_LOGIN, (ulong *)&target -> status );
	}																		/* FCLNX-GPL-038 */

	/* Setup callback information and retry count */	
	ap -> mb_retry_cnt  = ap->login_retry ;         /* FCLNX-0545 */
	if( test_bit( HFC_WAIT_LOGIN, (ulong *)&target -> status ) ){			/* FCLNX-GPL-389 */
		for (hash=0;hash<HASH_T_NUM;hash++)	/* FCLNX-0579 */
		{
			if (target->we_que_top[hash] != NULL)
			{	/* hfcp exists in queue */
				hfcp = target->we_que_top[hash];
				
				while( hfcp != NULL ){
					if((test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags))){
						ap->mb_retry_cnt = ap->to_reset_retry;				/* FCLNX-GPL-452 */
						
						if(!hfc_manage_info.hfcldd_mp_mod && ap->to_reset_retry)/* FCLNX-GPL-452 */
							ap->mb_retry_cnt--;								/* FCLNX-GPL-452 */
						
						ap->login_type = HFC_CANCEL_IO;	/* FCLNX-0571 */
					}
					
					hfcp = hfcp->cmd_forw ;
				}
			}
		}	/* FCLNX-0579 */
	}																		/* FCLNX-GPL-389 */


	ap -> mb_retry_tid  = HFC_ELS_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default value */

	hfc_mailbox_initiate( ap, HFC_MB_INTL );	/* Mailbox start */
	return 0;
}


/*
 * Function:    hfc_issue_pdisc
 *
 * Purpose:     Issue PDISC
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Target is not online, or failed to acquire lock
 *
 * Notes:       Lock adap_info before calling this function
 */
int hfc_issue_pdisc( struct adap_info *ap,struct target_info *target )
{
	int rtn,did;

	hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x00, ap, target, 0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_issue_pdisc start \n");

	if( !(test_bit( HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x81, ap, target, 0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_ISOL, (ulong *)&ap -> status ) ){	/* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x86, ap, target, 0, 0, 0 );
		return -1;
	}

	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x85, ap, target, 0, 0, 0 );	
		return -1;
	}

	if( !(test_bit( HFC_WAIT_PDISC, (ulong *)&target -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x84, ap, target, 0, 0, 0 );
		return -1 ;
	}

	if ( !(lock_try_mailbox( ap ) ) ) {			/* Mailbox lock fail */
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x82, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Start watchdog timer */
	if ( (rtn = hfc_watchdog_enter( ap,target,NULL, 0,HFC_ELS_TMR,0,FALSE ) ) )
	{
		/* Timer ID is already registered or invalud */
		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_PDISC, 0x83, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_write_val( ap -> mb -> mb_init.command, HFC_MBCMD_FCCTL );
	hfc_write_val( ap -> mb -> mb_init.sub_cmd, HFC_MBSCMD_PDISC );
	hfc_write_val( ap -> mb -> mb_init.pseq_no, target -> pseq );


	did = ( uint )( target -> scsi_id & 0x00ffffff );
	if ( ap -> connect_type == HFC_AL )
		did |= ( target -> scsi_id & 0x00ff ) << 24;

	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.adr.d_id, did );
	hfc_write_val( ap -> mb -> mb_init.type.drvioctl1.payload_length, 0 );


	/* Set WAIT_PDISC and clear NEED_PDISC */
	set_bit(HFC_WAIT_PDISC, (ulong *)&target -> status );
	clear_bit( HFC_NEED_PDISC, (ulong *)&target -> status );

	 /* Setup callback information and retry count */
	ap -> mb_retry_cnt  = ap->login_retry ;		/* FCLNX-0523 */
	ap -> mb_retry_tid  = HFC_ELS_TMR ;
	ap -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_mailbox_initiate( ap, HFC_MB_INTL );
	return 0;
}	/* end of hfc_issue_pdisc */


/*
 * Function:    hfc_issue_mihlog
 *
 * Purpose:     Data collection of timeout MIH LOG
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *  hfcp       - Pointer to hfc_pkt
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock adap_info before calling this function
 */
int hfc_issue_mihlog( struct adap_info *ap,
						 struct target_info *target, struct hfc_pkt *hfcp )
{
	uint rtn;
	uint64_t	hfcp_ad;

	hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x00, ap, target, 0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_issue_mihlog start \n");

	if( !(test_bit( HFC_ONLINE, (ulong *)&ap -> status ) ) ){
		hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x81, ap, target, 0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_ISOL, (ulong *)&ap -> status ) ){	/* FCLNX-GPL-572 */
		hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x85, ap, target, 0, 0, 0 );
		return -1;
	}

	if(!hfc_mlpf_check_normal_hypsts(ap)){	/* FCLNX-GPL-428 */
		hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x84, ap, target, 0, 0, 0 );	
		return -1;
	}

	if ( !(lock_try_mailbox( ap )) ) {			/* Mailbox lock failed */
		hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x82, ap, target, 0, 0, 0 );
		return -1;
	}

	/* Set and start watchdog timer */
	hfc_w_stop( ap, HFC_MB_TMR );
	if ( (rtn = hfc_w_start( ap, HFC_MB_TMR ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_mailbox ( ap );
		hfc_top_trace( HFC_TRC_ISSUE_MIHLOG, 0x83, ap, target, 0, 0, 0 );
		return -1;
	}

	set_bit(HFC_WAIT_MIHLOG, (ulong *)&ap->status);	/*FCLNX-0506*/

	/* Setup mailbox control block */
	hfc_write_val( ap->mb->mb_init.command, HFC_MBCMD_LOGTRACE );
	hfc_write_val( ap->mb->mb_init.sub_cmd, HFC_MBSCMD_MIHLOG );
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x0010 );		/* t-out log */
	memset(&ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work, 0, 
				sizeof(ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work));
	if( hfcp != NULL)
	{
		if( hfcp->cmd_pkt != NULL)
		{
			hfcp_ad = (ulong) hfcp;
			ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.hfc_pkt   = (uint64_t)hfcp_ad;
			ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.target_id = (ushort)hfcp->target_id;
			ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.lun_id    = (ushort)hfcp->lun_id ;
			
			if( HFC_MMODE_CHECK_SHARED(ap)  && !(HFC_MMODE_CHECK_SHADOW(ap) ) )                 /* FCLNX-0371 */
			{
				ap->mb->mb_init.type.drvlogb0.uni.mih_log.drv_work.rid       = (uchar)ap->rid;
			}                                                                                   /* FCLNX-0371 */
		}
	}
	
	/* Setup callback information and retry count */
	ap -> mb_retry_cnt  = ap->els_retry ;		/* FCLNX-0523 */
	ap -> mb_retry_tid  = HFC_MB_TMR ;
	ap -> mb_retry_tout = 0 ;					/* default value */

	/* Mailbox start */
	hfc_mailbox_initiate(ap, HFC_MB_INTL);
	return 0;
}	


/*
 * Function:    hfc_send_gidpn
 *
 * Purpose:     Search target to issue GID_PN and initiate GID_PN
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *   0         - Initiated GIDPN
 *  !0         - Failed to initiate GIDPN
 *
 * Notes:       
 */
int hfc_send_gidpn( struct adap_info *ap )
{
	struct target_info	*target=NULL;
	int	i, rtn=-1;

	/* Search target to issue GPN_ID */
	for (i=0 ; i<(int) ap->max_target ; i++) {
		target = ap->target_arg[i] ;
		if ( (target = hfc_pseq_target_info(ap,i)) != NULL )
			if( test_bit( HFC_NEED_GIDPN, (ulong *)&target->status ) )

				break ;
	}

	if( target != NULL ) {
		rtn = hfc_issue_gidpn(ap,target);
		ap -> next_gidpn = TRUE;
	}
	else
		ap -> next_gidpn = FALSE;

	return rtn;
}


/*
 * Function:    hfc_send_gpnid
 *
 * Purpose:     Search target to issue GPN_ID and initiate GPN_ID
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *                 (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * context:     DIRQL
 *
 * Notes:       
 */
int hfc_send_gpnid(struct adap_info *ap)
{
	int i;

	for (i=0;i<MAX_TARGET_PROBE;i++) {

		if ( ap->target_scan[i].flags & HFC_SCAN_NEED ) {
			if ( hfc_issue_gpnid( ap, ap->target_scan[i].scsi_id ) )
				return (-1);

			/* Mailbox start success */
			ap->target_scan[i].flags &= ~HFC_SCAN_NEED;
			ap->target_scan[i].flags |= HFC_SCAN_WAIT;
			return (1);
		}
	}

	return(0);
}


/*
 * Function:    hfc_send_login
 *
 * Purpose:     Initiate LOGIN to the target which is in LOGIN initiation wait status (HFC_NEED_LOGIN)
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *                 (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * Notes:       Dequeue target_info from queue and handle next target_info if the target state
 *              of target_info is in LOGIN initiation wait status (HFC_NEED_LOGIN)
 *
 */
int hfc_send_login(struct adap_info *ap)
{
	struct target_info *target;

	target = ap -> login_target;

	while ( target != NULL ) {
		if( test_bit( HFC_NEED_LOGIN, (ulong *)&target->status ) || test_bit( HFC_NEED_CANCEL, (ulong *)&target->status ) ){	/* FCLNX-GPL-038 */
			if ( hfc_issue_relogin( ap, target ) )
				return (-1);						/* Failed to start mailbox */

			return (1);								/* Succeeded to start mailbox */
		}

		/* Target state is HFC_NEED_LOGIN. Dequeue the target from LOGIN queue */
		hfc_deque_login_req( ap, target );
		target = ap -> login_target;
	}
 
	return(0); /* Failed to initiate mailbox */
}


/*
 * Function:    hfc_send_pdisc
 *
 * Purpose:     Issue PDISC to the target which is in PDISC initiation wait status (HFC_NEED_PDISC)
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *
 * context:     DIRQL
 *
 * Notes:       Dequeue target_info from queue and handle next target_info if the state
 *              of target_info is in PDISC initiation wait status (HFC_NEED_PDISC)
 *
 */
int hfc_send_pdisc(struct adap_info *ap)
{
	struct target_info *target;

	target = ap -> next_tstart;

	while ( target != NULL ) {
		if( test_bit( HFC_NEED_PDISC, (ulong *)&target->status ) ){
			if ( hfc_issue_pdisc( ap, target ) )
				return (-1);						/* Failed to start mailbox */

			hfc_deque_pdisc_req( ap, target );
			return (1);								/* Succeeded to start mailbox */
		}

		/* Target state is HFC_NEED_PDISC. Dequeue the target from PDISC queue */
		hfc_deque_pdisc_req( ap, target );
		target = ap -> next_tstart;
	}

	return(0);										/* There is no MailBox start */
}


/*
 * Function:    hfc_enque_login_req
 *
 * Purpose:     Enqueue target_info to LOGIN queue if LOGIN is unable to initiate.
 *
 * Arguments:   
 *  ap          - Pointer to adap_info
 *  target      - Pointer to target_info
 *
 * Returns:     -
 *
 * Notes:       Do nothing if the specified target_info has already existed in 
 *              LOGIN queue. 
 *              
 */
void hfc_enque_login_req(struct adap_info *ap,struct target_info *target)
{
	struct target_info *wk_tgt;

	wk_tgt = ap->login_target;
	if (wk_tgt == NULL) {
		ap->login_target = target;					/* Move to the top of the queue */
		target->login_next = NULL;
	}
	else {
		int hit=1;

		while (wk_tgt != target) {					/* Does this target exist in queue? */
			if (wk_tgt->login_next == NULL) {
				hit = 0;							/* All targets are unmatched */
				break;
			}
			wk_tgt = wk_tgt->login_next;
		}

		if (!hit) {								
			wk_tgt->login_next = target;			/* Add new target to the end of the queue */
			target->login_next = NULL;
		}
	}
}


/*
 * Function:    hfc_deque_login_req
 *
 * Purpose:     Dequeue target_info from LOGIN queue.
 *              All targets are removed from queue when no pointers are specified 
 *              as arguments, ap and target.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_deque_login_req(struct adap_info *ap,struct target_info *target)
{
	struct target_info *wk_tgt,*pr_tgt;

	pr_tgt = NULL;
	wk_tgt = ap->login_target;
	while (wk_tgt != NULL) {
		if (target == NULL) {									
			ap->login_target = NULL;				/* Dequeue all targets */
			pr_tgt = wk_tgt->login_next;
			wk_tgt->login_next = NULL;
			wk_tgt = pr_tgt;
		}
		else {
			if (wk_tgt == target) {					/* Find target? */
				if (pr_tgt == NULL)					/* Queue top?   */
					ap->login_target   = wk_tgt->login_next;
				else
					pr_tgt->login_next = wk_tgt->login_next;

				wk_tgt->login_next=NULL;
				break;								/* return */
			}

			pr_tgt = wk_tgt;					
			wk_tgt = wk_tgt->login_next;			/* Go to next target */
		}
	}
}


/*
 * Function:    hfc_enque_pdisc_req
 *
 * Purpose:     Enqueue target_info to PDISC queue if PDISC is unable to initiate.
 *
 * Arguments:   
 *  ap         - Pointer to adap_info
 *  target     - Pointer to target_info
 *
 * Returns:     
 *
 * Notes:       Do nothing if the specified target_info has already existed in 
 *              PDISC queue. 
 */
void hfc_enque_pdisc_req(struct adap_info *ap,struct target_info *target)
{
	struct target_info *wk_tgt;

	wk_tgt = ap->next_tstart;
	if (wk_tgt == NULL) {
		ap->next_tstart = target;					/* Move to the top of the queue	*/
		target->pdisc_next = NULL;
	}
	else {
		int hit=1;

		while (wk_tgt != target) {					/* Does this target exist in queue? */
			if (wk_tgt->pdisc_next == NULL) {
				hit = 0;							/* All targets are unmatched */
				break;
			}
			wk_tgt = wk_tgt->pdisc_next;
		}

		if (!hit) {									
			wk_tgt->pdisc_next = target;			/* Add new target to the end of the queue */
			target->pdisc_next = NULL;
		}
	}
}


/*
 * Function:    hfc_deque_pdisc_req
 *
 * Purpose:     Dequeue target_info from PDISC queue.
 *              All targets are removed from queue when no pointers are specified 
 *              as arguments, ap and target.
 * Arguments:   
 *  ap          Pointer to adap_info
 *  target      Pointer to target_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_deque_pdisc_req(struct adap_info *ap,struct target_info *target)
{
	struct target_info *wk_tgt,*pr_tgt;

	pr_tgt = NULL;
	wk_tgt = ap->next_tstart;
	while (wk_tgt != NULL) {
		if (target == NULL) {						/* Dequeue all targets */
			ap->next_tstart = NULL;
			pr_tgt = wk_tgt->pdisc_next;
			wk_tgt->pdisc_next = NULL;
			wk_tgt = pr_tgt;
		}
		else {
			if (wk_tgt == target) {					/* Find target? */
				if (pr_tgt == NULL)					/* Queue top */
					ap->next_tstart    = wk_tgt->pdisc_next;
				else
					pr_tgt->pdisc_next = wk_tgt->pdisc_next;

				wk_tgt->pdisc_next = NULL;
				break;								/* return */
			}

			pr_tgt = wk_tgt;
			wk_tgt = wk_tgt->pdisc_next;			/* Go to next target */
		}
	}
}


/*
 * Function:    hfc_trace
 *
 * Purpose:     Collect trace data
 *
 * Arguments:   
 *  ap          Pointer to adap_info
 *  id          Trace id
 *  trc         
 *  level       1 : caller is ioctl, 0 : call is other function
 *
 * Returns:     -
 *
 * Notes:       
 */
void hfc_trace(struct adap_info *ap,uchar id,uchar *trc,uchar level)
{
	uint64_t	time=0;

	if( ap == NULL )
		return ;

	ap->trc_ptr[ap->trc_num].trc_id = id ;

	HFC_MEMCPY(ap->trc_ptr[ap->trc_num].trc_data,trc,
				sizeof(ap->trc_ptr[ap->trc_num].trc_data)) ;

	time = (uint64_t)jiffies;								/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	HFC_8L_TO_8B(ap->trc_ptr[ap->trc_num].trc_time, time);	/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
//	ap->trc_ptr[ap->trc_num].trc_time = (uint64_t)jiffies;

#if _HFC_DEBUG_TOP_00
	hfc_dump_hex("hfc_tarce()",&ap->trc_ptr[ap->trc_num],16);
#endif

	ap->trc_num++ ;
	if( ap->trc_num >= ap->trc_max )
		ap->trc_num = 0 ;
}


#define HFC_WDG_ADP	1		/* Unit of HBA    */
#define HFC_WDG_TGT	2		/* Unit of target */
#define HFC_WDG_DEV	3		/* Unit of LU     */
/*
 * Function:    hfc_watchdog_enter
 *
 * Purpose:     Start and Stop watch dog timer 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info (*)
 *  target     - Pointer to target_info 
 *                HFC_ABORT_TMR/HFC_TARGET_RST_TMR (*)
 *  hfcp       - Pointer to hfc_pkt  (*)
 *  lun        - lun# 
 *                HFC_ABORT_TMR (*)
 *  timer_id   - Timer ID
 *  tout       - Time out value (sec) ( 0:default value)
 *  cancel     - FALSE : Start timer, TRUE : Stop(Calcel) timer
 *
 * Returns:     
 *    0        - Start/Stop watch dog timer successfully.
 *    1        - Timer with this timer id has already started or canceled.
 *    2        - Timer is invalid.
 *    3        - Parameters are invalid.  (Needs parameters)
 *
 *
 * Notes:      (*) means parameter is required.
 */
int hfc_watchdog_enter( struct adap_info *ap, struct target_info *target,
						struct hfc_pkt *hfcp, uint lun, uchar timer_id, 	/* FCLNX-GPL-038 *//* FCLNX-GPL-0343 */
						uint tout, int cancel)
{
	struct wtimer *w_timer;
	uint d_time=0;
	int tmo_work=0;		/* FCLNX-GPL-260 */

	/* Search target */
	if(cancel == 0)
	{
		switch (timer_id)
		{
			case HFC_ELS_TMR :			
								d_time = (HFC_ELS_TO * HZ);
								w_timer = &ap->mb_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->mb_wdog.dog, hfc_watchdog, 0);
								ap->mb_retry_tid = timer_id;
								break;

			case HFC_LINKINIT_TMR :		
								d_time = (HFC_LINKINT_TO * HZ);
								w_timer = &ap->mb_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								ap->mb_retry_tid = timer_id;
								break;

			case HFC_MB_TMR :			
								if( test_bit(HFC_DIAG_PROGRESS, (ulong *)&ap->mp_adap_info->status ) ){
									d_time = (HFC_MB_DIAG_TO * HZ);
								}
								else if(tout){ /* FCLNX-GPL-243 start */
									d_time = (tout * HZ);
								} /* FCLNX-GPL-243 end */
								else{
									d_time = (HFC_MB_TO * HZ);
								}
								w_timer = &ap->mb_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->mb_wdog.dog, hfc_watchdog, 0);
								ap->mb_retry_tid = timer_id;
								break;

			case HFC_CTLRST_DELAY_TMR : 										/* FCLNX-0279 */
			case HFC_REBOOT_DELAY_TMR :	
								if (tout)										/* FCLNX-0279 */
									d_time = (tout * HZ);						/* FCLNX-0279 */
								else											/* FCLNX-0279 */
									d_time = (HFC_REBOOT_DELAY_TO * HZ);
								w_timer = &ap->reboot_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->reboot_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_MCK_DELAY_TMR :	
								d_time = (HFC_MCK_DELAY_TO * HZ);
								w_timer = &ap->mck_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->mck_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_MCKINT_TMR :												/* FCLNX-0275 */
								d_time = (HFC_MCKINT_TO * HZ);
								w_timer = &ap->mckint_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->mckint_wdog.dog, hfc_watchdog, 0);
								break;											/* FCLNX-0275 */
/* @MLPF */
			case HFC_MLPF_FMCK_TMR :    /* Shadow LPAR only */
								d_time = (HFC_MLPF_FMCK_STO * HZ);
								
								w_timer = &ap->fmck_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);
								timer_setup(&ap->fmck_wdog.dog, hfc_watchdog, 0);
								break;
			case  HFC_MLPF_MCKEND_TMR :
								if( HFC_MMODE_CHECK_SHADOW(ap) )
									d_time = (HFC_MLPF_MCKEND_STO * HZ);
								else
									d_time = (HFC_MLPF_MCKEND_GTO * HZ);
								w_timer = &ap->mckend_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);
								timer_setup(&ap->mckend_wdog.dog, hfc_watchdog, 0);
								break;
			case  HFC_MLPF_FCSTP_TMR :  /* Shadow LPAR only */
								d_time = (HFC_MLPF_FCSTP_STO * HZ);
								
								w_timer = &ap->fcstp_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);
								timer_setup(&ap->fcstp_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_LUN0_TMR :
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[0].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN1_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[1].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN2_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[2].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN3_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[3].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN4_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[4].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN5_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[5].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN6_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[6].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LUN7_TMR :	
								d_time = (HFC_LOOP_TEST_TO * HZ);
								w_timer = &ap->loop_dev_info[7].loop_wdog;
								timer_setup(&w_timer->dog, hfc_watchdog, 0);
								break;

			case HFC_LINKUP_TMR	:		
								tmo_work = ap->linkup_tmo;							/* FCLNX-GPL-260 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
								if ( !hfc_manage_info.hfcldd_mp_mod ) {
									if ( ap->linkup_tmo ) {
										tmo_work = tmo_work -1;
									}
								}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
								d_time = ( tmo_work * HZ);							/* FCLNX-GPL-260 */
								w_timer = &ap->linkup_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->linkup_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_LINKUP2_TMR    :												/* FCLNX-241 */
								d_time = ( ap->linkup2_tmo * HZ);
								w_timer = &ap->linkup_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 		
								timer_setup(&ap->linkup_wdog.dog, hfc_watchdog, 0);
								break;												/* FCLNX-241 */

			case HFC_WLINKUP_CNT_TMR    :											/* FCLNX-GPL-FX-424 */
								d_time = ( ap->dev_loss_tmo * HZ);
								w_timer = &ap->ld_err_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 		
								timer_setup(&ap->ld_err_wdog.dog, hfc_watchdog, 0);
								break;												/* FCLNX-241 */

			case HFC_SCN_LINKUP_TMR	: 			
								if ( target == NULL )
									return (3);		
								tmo_work = ap->linkup_tmo;							/* FCLNX-GPL-260 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
								if ( !hfc_manage_info.hfcldd_mp_mod ) {
									if ( ap->linkup_tmo ) {
										tmo_work = tmo_work -1;
									}
								}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
								d_time = ( tmo_work * HZ);							/* FCLNX-GPL-260 */
								w_timer = &target->scnlinkup_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&target->scnlinkup_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_DELAY_TMR :
								if ( target == NULL )
									return (3);		

								d_time = (ap->scsi_reset_delay * HZ);
								w_timer = &target->delay_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&target->delay_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_WEXEC_TMR :
								if ( target == NULL )
									return (3);		

								d_time = (HFC_WEXEC_TO * HZ);
								w_timer = &target->wexec_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&target->wexec_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_TARGET_RST_TMR :
								if ( target == NULL )
									return (3);		/* argument is abnormal		*/
								if ( hfcp == NULL )
									return (3);		/* argument is abnormal		*/

								d_time = (ap->target_reset_tmo * HZ);
								w_timer = &hfcp->cmd_timeout;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&hfcp->cmd_timeout.dog, hfc_watchdog, 0);
								break;

			case HFC_ABORT_TMR :
								if ( target == NULL )
								 	return (3);		/* argument is abnormal		*/
								if ( hfcp == NULL )
									return (3);		/* argument is abnormal		*/

								if( HFC_HFCP_CFLAG_TEST(CFLAG_ABORT, hfcp) ){	/* FCLNX-GPL-0343 */
									d_time = (ap->abort_tmo * HZ);
								}
								else if( HFC_HFCP_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ){
//									d_time = (ap->lun_reset_tmo * HZ);
									d_time = (ap->abort_tmo * HZ);
								}												/* FCLNX-GPL-0343 */
								w_timer = &hfcp->cmd_timeout;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&hfcp->cmd_timeout.dog, hfc_watchdog, 0);
								break;

			case HFC_SCSI_CMD_TMR :
								if ( hfcp == NULL )
									return (3);
									
								d_time = tout;		/* FCLNX-0429 */
								w_timer = &hfcp->cmd_timeout;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&hfcp->cmd_timeout.dog, hfc_watchdog, 0);
								break;

			case HFC_DIAG_DELAY_TMR :
								d_time = (HFC_REBOOT_DELAY_TO * HZ);
								w_timer = &ap->reboot_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->reboot_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_MPAP_LOCK_TMR:
								d_time = (HFC_WAIT_MPAPLOCK_TO * HZ);
								w_timer = &ap->mpap_lock_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->mpap_lock_wdog.dog, hfc_watchdog, 0);
								break;

			case HFC_LOGIN_DELAY_TMR:												/* FCLNX-0243 */
								d_time = (tout * HZ);
								w_timer = &ap->lgdelay_wdog;						/* FCLNX-0270 */
								if( w_timer->timer_flag & HFC_TIMER_VALID )
								return (1); 			/* It doesn't stop */
								timer_setup(&ap->lgdelay_wdog.dog, hfc_watchdog, 0);					/* FCLNX-0270 */
								break;  											/* FCLNX-0243 */
			case HFC_LDLERR_TMR:
				 				d_time = (ap->ldl_errcnt_info->current_tmr_time * HZ);
								w_timer = &ap->ldlerr_wdog; /* FCLNX-0506 */
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								timer_setup(&ap->ldlerr_wdog.dog, hfc_watchdog, 0); /* FCLNX-0506 */
								break;
			case HFC_LDSERR_TMR:
								d_time = (ap->lds_errcnt_info->current_tmr_time * HZ);
								w_timer = &ap->ldserr_wdog; /* FCLNX-0506 */
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								timer_setup(&ap->ldserr_wdog.dog, hfc_watchdog, 0); /* FCLNX-0506 */
								break;
			case HFC_IFERR_TMR:
								d_time = (ap->if_errcnt_info->current_tmr_time * HZ);
								w_timer = &ap->iferr_wdog; /* FCLNX-0506 */
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								timer_setup(&ap->iferr_wdog.dog, hfc_watchdog, 0); /* FCLNX-0506 */
								break;
			case HFC_TOERR_TMR:
								d_time = (ap->to_errcnt_info->current_tmr_time * HZ);
								w_timer = &ap->toerr_wdog; /* FCLNX-0506 */
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								timer_setup(&ap->toerr_wdog.dog, hfc_watchdog, 0); /* FCLNX-0506 */
								break;
			case HFC_ISOLATE_DELAY_TMR:
								if (tout)										/* FCLNX-0279 */
									d_time = (tout * HZ);						/* FCLNX-0279 */
								else											/* FCLNX-0279 */
									d_time = (HFC_FW_ISOL_TO * HZ);
								w_timer = &ap->fwisol_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->fwisol_wdog.dog, hfc_watchdog, 0);
								break;
			case HFC_INT_CHECK_TMR:												/* FCLNX-GPL-306 */
								d_time = 1 * HZ;
								w_timer = &ap->int_chk_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->int_chk_wdog.dog, hfc_watchdog, 0);
								break;
			case HFC_TGT_LDLERR_TMR:												/* FCLNX-GPL-327 */
								if ( target == NULL )
									return (3);
				 				d_time = (target->tgt_ldl_errcnt_info->current_tmr_time * HZ);
								w_timer = &target->tgt_ldlerr_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&target->tgt_ldlerr_wdog.dog, hfc_watchdog, 0);
								break;
			case HFC_TGT_LDSERR_TMR:												/* FCLNX-GPL-327 */
								if ( target == NULL )
									return (3);
								d_time = (target->tgt_lds_errcnt_info->current_tmr_time * HZ);
								w_timer = &target->tgt_ldserr_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);
								timer_setup(&target->tgt_ldserr_wdog.dog, hfc_watchdog, 0);
								break;
			case HFC_RESTART_TMR:												/* FCLNX-GPL-328 */
								if ( target == NULL )
									return (3);		/* argument is abnormal		*/

								d_time = (tout * HZ);
								w_timer = &target->restart_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		/* It doesn't stop */
								timer_setup(&target->restart_wdog.dog, hfc_watchdog, 0);
								break;											/* FCLNX-GPL-328 */
			case HFC_MLPF_ISOLEND_TMR:
								if (tout)										/* FCLNX-0279 */
									d_time = (tout * HZ);						/* FCLNX-0279 */
								else											/* FCLNX-0279 */
									d_time = (HFC_MLPF_ISOLEND_GTO * HZ);
								w_timer = &ap->isolend_wdog;
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1);		
								timer_setup(&ap->isolend_wdog.dog, hfc_watchdog, 0);
								break;
			default :
								return (2);			/* Invalid TIMER ID		*/
		}
		if(!(&w_timer->dog))
		{        
        	 HFC_DBGPRT("watchdog_enter() - w_timer = NULL before init \n");
		}

		w_timer->ap = ap;
		w_timer->ap_dev_minor = ap->dev_minor;										/* FCLNX-0322 */
		w_timer->target = target;
		w_timer->timer_id = timer_id;
		w_timer->hfcpk = hfcp;
		if( hfcp != NULL ){
			w_timer->dev = hfcp->dev;							/* FCLNX-GPL-047 *//* FCLNX-GPL-0343 */
		}
		w_timer->dog.expires = jiffies + d_time;
		/* kernel 4.15+: timer_list.data removed */
		/* kernel 4.15+: callback set by timer_setup; explicit assign removed */
		w_timer->timer_flag |= HFC_TIMER_VALID;
		switch (timer_id)										/* FCLNX-0312 */
		{
			case HFC_SCSI_CMD_TMR :
				if (!tout) {
					w_timer->dog.function = NULL;
					w_timer->timer_flag &= ~HFC_TIMER_VALID;
					break;
				} else {
					add_timer(&w_timer->dog);
					break;
				}
			default :
				add_timer(&w_timer->dog);
				break;
		}														/* FCLNX-0312 */

	}
	else{
		switch (timer_id)
		{
			case HFC_ELS_TMR :		
			case HFC_LINKINIT_TMR :		
			case HFC_MB_TMR :	

								w_timer = &ap->mb_wdog;
								if(!(&w_timer->dog)){
									    hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBB, NULL, 0) ;
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_CTLRST_DELAY_TMR : 										/* FCLNX-0279 */
			case HFC_REBOOT_DELAY_TMR :	
			case HFC_DIAG_DELAY_TMR :
								w_timer = &ap->reboot_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_MCK_DELAY_TMR :	
								w_timer = &ap->mck_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_MCKINT_TMR :													/* FCLNX-0275 */
								w_timer = &ap->mckint_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;												/* FCLNX-0275 */

			case HFC_MLPF_FMCK_TMR :
								w_timer = &ap->fmck_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_MLPF_MCKEND_TMR :
								w_timer = &ap->mckend_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_MLPF_FCSTP_TMR :
								w_timer = &ap->fcstp_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN0_TMR :
								w_timer = &ap->loop_dev_info[0].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN1_TMR :	
								w_timer = &ap->loop_dev_info[1].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN2_TMR :	
								w_timer = &ap->loop_dev_info[2].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN3_TMR :	
								w_timer = &ap->loop_dev_info[3].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN4_TMR :	
								w_timer = &ap->loop_dev_info[4].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_LUN5_TMR :	
								w_timer = &ap->loop_dev_info[5].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN6_TMR :	
								w_timer = &ap->loop_dev_info[6].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LUN7_TMR :	
								w_timer = &ap->loop_dev_info[7].loop_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_LINKUP_TMR	:
			case HFC_LINKUP2_TMR:															/* FCLNX-0241 */
								w_timer = &ap->linkup_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_SCN_LINKUP_TMR	: 			
								w_timer = &target->scnlinkup_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_WLINKUP_CNT_TMR    :											/* FCLNX-GPL-FX-424 */
								w_timer = &ap->ld_err_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;												/* FCLNX-241 */

			case HFC_DELAY_TMR :
								w_timer = &target->delay_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
								
			case HFC_WEXEC_TMR :
								w_timer = &target->wexec_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
								
			case HFC_TARGET_RST_TMR :
								if ( target == NULL )
									return (3);		
								if ( hfcp == NULL )
									return (3);		
								w_timer = &hfcp->cmd_timeout;
								if (!(&w_timer->dog))			/* FCLNX-0429 */
									break;						/* FCLNX-0429 */
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_ABORT_TMR :
								if ( target == NULL )
								 	return (3);		
								if ( hfcp == NULL )
									return (3);	
								w_timer = &hfcp->cmd_timeout;
								if (!(&w_timer->dog))			/* FCLNX-0429 */
									break;						/* FCLNX-0429 */
								if (w_timer->dog.function != NULL) {
									if( w_timer->timer_flag & HFC_TIMER_VALID )
										del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_SCSI_CMD_TMR :
								if ( hfcp == NULL )
									return (3);
								w_timer = &hfcp->cmd_timeout;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									if( w_timer->timer_flag & HFC_TIMER_VALID )
										del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

			case HFC_MPAP_LOCK_TMR :
								w_timer = &ap->mpap_lock_wdog;
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;

														/* HFC_LOGIN_DELAY */
			case HFC_LOGIN_DELAY_TMR :													/* FCLNX-0243 */
								w_timer = &ap->lgdelay_wdog;							/* FCLNX-0270 */
								if(!(&w_timer->dog)){
										break;
								}
								if (w_timer->dog.function != NULL) {
								del_timer(&w_timer->dog);
								w_timer->dog.function =  NULL;
		/* kernel 4.15+: timer_list.data removed */
								w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;													/* FCLNX-0243 */
			case HFC_LDLERR_TMR:
								w_timer = &ap->ldlerr_wdog;      /* FCLNX-0506 */
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_LDSERR_TMR:
								w_timer = &ap->ldserr_wdog; 	 /* FCLNX-0506 */
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_IFERR_TMR:
								w_timer = &ap->iferr_wdog;		/* FCLNX-0506 */
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_TOERR_TMR:
								w_timer = &ap->toerr_wdog;		/* FCLNX-0270 */
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_ISOLATE_DELAY_TMR :
								w_timer = &ap->fwisol_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_INT_CHECK_TMR:		/* FCLNX-GPL-306 */
								w_timer = &ap->int_chk_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_TGT_LDLERR_TMR:							/* FCLNX-GPL-327 */
								w_timer = &target->tgt_ldlerr_wdog;
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_TGT_LDSERR_TMR:							/* FCLNX-GPL-327 */
								w_timer = &target->tgt_ldserr_wdog;
								if(!(&w_timer->dog)){
									break;
								}
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			case HFC_RESTART_TMR:		/* FCLNX-GPL-328 */
								if ( target == NULL )
								 	return (3);		
								w_timer = &target->restart_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;	/* FCLNX-GPL-328 */
			case HFC_MLPF_ISOLEND_TMR :
								w_timer = &ap->isolend_wdog;
								if (w_timer->dog.function != NULL) {
									del_timer(&w_timer->dog);
									w_timer->dog.function =  NULL;
			/* kernel 4.15+: timer_list.data removed */
									w_timer->timer_flag &= ~HFC_TIMER_VALID;
								}
								break;
			default :
								return (2);			/* Invalid TIMER ID		*/
		}
	}
	
	return(0);

}


/*
 * Function:    hfc_top_trace
 *
 * Purpose:     Collect trace data for hfcl_top
 *
 * Arguments:   
 *  id         - Trace ID 
 *  sub_id     - Trace additional ID
 *  ap         - Pointer to adap_info 
 *  target     - Pointer to target_info 
 *  etc1/2/3   - Arguments (Depends on Trace ID) 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_top_trace(
	uchar               id,
	uchar               sub_id,
	struct adap_info    *ap,
	struct target_info  *target,
	uint64_t            etc1,
	uint64_t            etc2,
	uint64_t            etc3)
{
	uchar               trc_wk[128] ;
	struct top_trc1    *trc1 ;
	struct top_trc2    *trc2 ;
//	struct dev_info		*dev=NULL;
	
	memset(trc_wk,0,128) ;
	
    HFC_DBGPRT("hfc_top_trace() - start (ID=%x, sub_id=%x). \n"
						,id, sub_id);

	
/*----------------------------------------------*/
/*                  TRACE1                      */
/* etc1 = NULL                                  */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/

	if( (id == HFC_TRC_ISSUE_LINKINIT) ||
		(id == HFC_TRC_ISSUE_GIDFT) ||
		(id == HFC_TRC_ISSUE_GIDPN) ||
		(id == HFC_TRC_ISSUE_GPNID)||
		(id == HFC_TRC_ISSUE_RELOGIN) ||
		(id == HFC_TRC_ISSUE_PDISC) ||
		(id == HFC_TRC_ISSUE_MIHLOG))
	{/*-- trace format 1 --*/
		trc1 = (struct top_trc1 *)trc_wk ;
		
		trc1->id = id ;
		trc1->sub_id = sub_id ;
		if( ap != NULL )
		{
			trc1->mb_retry_cnt = ap->mb_retry_cnt;
			trc1->a_status = ap->status;
			trc1->a_scsi_id = ap->scsi_id;
			trc1->mb_status = (uchar)ap->mb_status ;
//			trc1->a_timer_flag = 0 ;
//			trc1->a_login_target = (ulong)ap->login_target;
//			trc1->a_next_tstart = (ulong)ap->next_tstart;
			trc1->a_next_gidpn = ap->next_gidpn ;
			memcpy(trc1->mb_init,(uchar *)&(ap->mb->mb_init.command),64);
		}
		if( target != NULL )
		{
			trc1->t_flags = target->flags ;
			trc1->t_pseq = target->pseq ;
			trc1->t_id = (uchar)target->target_id ;
			trc1->t_status = target->status;
			trc1->t_ww_name = target->ww_name;
		}
	}/*-- trace format 1 --*/
	
	if( (id == HFC_TRC_ADD_TARGET) ||
		(id == HFC_TRC_COMMIT_TARGET) ||
		(id == HFC_TRC_DEL_TARGET))
	{/*-- trace format 2 --*/
		/* etc1=bind_type */
		/* etc2=lu# */
		trc2 = (struct top_trc2 *)trc_wk ;
		
		trc2->id = id ;
		trc2->sub_id = sub_id ;
		if( ap != NULL )
		{
			trc2->a_status = ap->status;
			trc2->a_scsi_id = ap->scsi_id;
		}
		if( target != NULL )
		{
			trc2->t_flags = target->flags ;
			trc2->t_pseq = target->pseq ;
			trc2->t_id = (uchar)target->target_id ;
			trc2->t_fc_class = target->fc_class ;
			trc2->t_status = target->status;
			trc2->t_fc_class_mask = target->fc_class_mask;
			trc2->t_device_flags = target->device_flags;
			trc2->t_max_frame_size = target->max_frame_size;
			trc2->bind_type  = (uchar)etc1;
			trc2->e_lu       = (uchar)etc2;

			trc2->t_ww_name = target->ww_name;
			trc2->t_node_name = target->node_name;
			trc2->t_we_que_cnt = target->we_que_cnt;
//			dev = (struct dev_info *)hfc_search_dev_info( target, etc1 );
//			if( dev != NULL ){
//				trc2->e_lustat = dev->lustat;
//			}
		}
	}/*-- trace format 2 --*/

	hfc_trace(ap,id,&trc_wk[1],0);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* Debug routine															*/
/*--------------------------------------------------------------------------*/
/*
 * Function:    hfc_dump_hex
 *
 * Purpose:     Memory Hex dump 
 *
 * Arguments:   
 *  string     - Dump header(string)
 *  in         - Pointer to dump area
 *  byte       - Length of dump area
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_dump_hex( char string[],void *in, int size )
{
	static char hex[] = "0123456789abcdef";
	char *p,buf[128],*inp = in;
	int i;

	memset(buf,0,sizeof(buf));

	p = buf;

	for (i = 0; i < size; i++, inp++)
	{
		if ((i%16)==0)
		{
			*p++ = ' ';
			*p++ = '0';
			*p++ = 'x';
			*p++ = hex[(i >> 12) & 0x0f];
			*p++ = hex[(i >>  8) & 0x0f];
			*p++ = hex[(i >>  4) & 0x0f];
			*p++ = hex[(i	   ) & 0x0f];
			*p++ = ' ';
			*p++ = '[';
			*p++ = ' ';
		}

		*p++ = hex[(*inp >> 4) & 0x0f];
		*p++ = hex[*inp & 0x0f];

		if ((i % 16)==15)
		{
			*p++ = ' ';
			*p++ = ']';
			*p++ = '\n';
			HFC_DEFPRT("%s",buf);
			p = buf;
		}
		else
		{
			if ((i % 4)==3)
				*p++ = ' ';
		}
	}

	if (i % 16)
	{
		*p++ = ' ';
		*p++ = ']';
		*p++ = '\n';
		HFC_DEFPRT("%s",buf);
	}
}



void _hfc_sleep_on(wait_queue_head_t *event, atomic_t *condition)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	wait_event((wait_queue_head_t)*event, atomic_read(condition));
#else
	wait_event(*event,atomic_read(condition));
#endif
	atomic_dec(condition);
}


void _hfc_wake_up(wait_queue_head_t *event, atomic_t *condition)
{
	atomic_inc(condition);
	wake_up((wait_queue_head_t *)event);
}

/*
 * Function:    hfc_issue_int_a_rst
 *
 * Purpose:     This Function is used to issue INT_A reset.
 *              INTx     : This function execute dummy read.
 *              MSI,MSI-X: This function do not execute dummy read.
 *
 * Arguments:   
 *  ap         - pointer to adap_info
 *  int_a_rst  - 
 *  int_a_reg  - 
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_issue_int_a_rst(struct adap_info *ap, uint int_a_rst, uint int_a_reg)
{
	/* Values */
	uint after_int_a;
	int lp;
	int int_a_cnt = 0;
	int multi_int_flag = FALSE;
	
	/*** FIVE-EX, MSI patch ********/ /* FCLNX-GPL-144 */
	/* for only FIVE-EX */
	if( ap->pkg.type == HFC_PKTYPE_FIVE_EX ){
		/* for only MSI */
		if( ap->msi_flag == HFC_INT_TYPE_MSI ) {
			/* for only "BASIC" */ /* MLPF never use native MSI. */
			if ( HFC_MMODE_CHECK_BASIC(ap) )
			{
				for( lp = 0; lp < 32; lp ++ ) {
					if( test_bit( lp, (ulong *)&int_a_reg ) ) {
						int_a_cnt++;
					}
				}
				if( int_a_cnt >= 2 ) {
					multi_int_flag = TRUE;
					/* Close INTA Mask */
					hfc_write_reg( ap, HFC_IOSPACE_INTA_MSK, (char)0x4, 0x00000000 );
				}
			}
		}
	}
	
	/*** Reset PCI space to clear INT_A register ***********/
	hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_RST,( char )0x4, int_a_rst);

	/*** Dummy read ****************************************/
	/* Check INT type ( INTx or MSI or MSI-X ). */
	switch( ap->msi_flag ){
		case HFC_INT_TYPE_INTX:
			/* dummy read */
			after_int_a = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;
			
		case HFC_INT_TYPE_MSI:
		case HFC_INT_TYPE_MSIX:
			/* Check the paramater. */
			if(ap->inta_dummy_read != HFC_DUMMY_READ_OFF){
				/* dummy read */
				after_int_a = (uint)hfc_read_reg( ap, (uint)HFC_IOSPACE_INTA, (char)0x4);
			}
			else{
				/* NOP */
			}
			break;

		default:
			/* NOP */
			break;
	}

	/*** FIVE-EX, MSI patch ********/ /* FCLNX-GPL-144 */
	if( multi_int_flag == TRUE ) {
		/* Open INTA Mask */
		hfc_write_reg( ap, HFC_IOSPACE_INTA_MSK, (char)0x4, hfc_inta_mask[ap->pkg.type] );
	}
	
	return;
}

/*
 * Function:    hfc_kmalloc
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*ap
 *				size
 *				flag
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_kmalloc(struct adap_info *ap, size_t size, gfp_t flag )
{
	void *addr = NULL;
	
	addr = kmalloc(size, flag);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.kmalloc_cnt);
	
	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		/* Always count up. */
		atomic_inc(&hfc_manage_info.kmalloc_cnt_ap[ap->instance]);

		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: kmalloc ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}
	}
	
	return(addr);
}



/*
 * Function:    hfc_kfree
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*ap
 *				addr
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_kfree(struct adap_info *ap, const void *block )
{
	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: kfree ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", block); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}

		/* Always count down. */
		atomic_dec(&hfc_manage_info.kmalloc_cnt_ap[ap->instance]);
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.kmalloc_cnt);
	
	kfree(block);
	return;
}

/*
 * Function:    hfc_dma_alloc_coherent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*ap
 *				*dev
 *				size
 *				*dma_handle
 *				gfp
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_dma_alloc_coherent(struct adap_info *ap, struct device *dev,
						size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	void *addr = NULL;
	
	addr = dma_alloc_coherent(dev, size, dma_handle, gfp);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.dma_alloc_cnt);
	
	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		/* Always count up. */
		atomic_inc(&hfc_manage_info.dma_alloc_cnt_ap[ap->instance]);

		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: dma_alloc_coherent ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}
	}
	
	return(addr);
}



/*
 * Function:    hfc_dma_free_coherent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*ap
 *				*dev
 *				size
 *				*vaddr
 *				dma_handle
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_dma_free_coherent(struct adap_info *ap, struct device *dev,
						size_t size, void *vaddr, dma_addr_t dma_handle)
{
	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: dma_free_coherent ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", vaddr); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}
		
		/* Always count down. */
		atomic_dec(&hfc_manage_info.dma_alloc_cnt_ap[ap->instance]);
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.dma_alloc_cnt);
	
	dma_free_coherent(dev, size, vaddr, dma_handle);
	return;
}

/*
 * Function:    hfc_pci_alloc_consistent
 *
 * Purpose:    
 *
 *
 * Arguments:  
 *				*ap
 *				*dev
 *				size
 *				*dma_addrp
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_pci_alloc_consistent(struct adap_info *ap, struct pci_dev *pdev,
							size_t size, dma_addr_t *dma_addrp)
{
	void *addr = NULL;
	
	/* kernel 5.18+: pci_alloc_consistent removed; use dma_alloc_coherent */
	addr = dma_alloc_coherent(&pdev->dev, size, dma_addrp, GFP_KERNEL);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.pci_alloc_cnt);

	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		/* Always count up. */
		atomic_inc(&hfc_manage_info.pci_alloc_cnt_ap[ap->instance]);
		
		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: pci_alloc_consistent ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}
	}
	
	return(addr);
}


/*
 * Function:    hfc_pci_free_consistent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*ap
 *				*pdev
 *				size
 *				*cpu_addr
 *				dma_addr
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_pci_free_consistent(struct adap_info *ap, struct pci_dev *pdev,
						size_t size, void *cpu_addr, dma_addr_t dma_addr)
{
	if(ap != NULL) /* FCLNX-GPL-168 */
	{
		if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_ERRPRT("hfcldd%d: pci_free_consistent ", ap->instance);
			HFC_ERRPRT("Addr=0x%p ", cpu_addr); /* FCLNX-GPL-149 */
			HFC_ERRPRT("\n" );
		}
		
		/* Always count down. */
		atomic_dec(&hfc_manage_info.pci_alloc_cnt_ap[ap->instance]);
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.pci_alloc_cnt);
	
	/* kernel 5.18+: pci_free_consistent removed; use dma_free_coherent */
	dma_free_coherent(&pdev->dev, size, cpu_addr, dma_addr);
	return;
}


/*
 * Function:    hfc_scsi_host_alloc
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*sht
 *				privsize
 *
 * Returns:     struct Scsi_Host *
 *
 * Notes:       
 */
struct Scsi_Host *
hfc_scsi_host_alloc(struct scsi_host_template *sht, int privsize)
{
	struct Scsi_Host *addr = NULL;
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.host_alloc_cnt);
	
	addr = scsi_host_alloc(sht, privsize);
	
	return(addr);
}


/*
 * Function:    hfc_scsi_host_put
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*shost
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_scsi_host_put(struct Scsi_Host *shost)
{
	/* Always count down. */
	atomic_dec(&hfc_manage_info.host_alloc_cnt);
	
	scsi_host_put(shost);
	return;
}

/* FCLNX-GPL-154 */
/*
 * Function : hfc_remap_pci_bar
 *
 * Purpose : Assigne PCI memory BAR1
 *
 * Arguments :
 *  pdev       - pointer to pci_dev
 *  bar        - Base Address
 * Returns :
 *    Base Address : 
 *      0x00       : Err Case
 * Notes:
 *             pdev is not NULL
 */
ulong
hfc_remap_pci_bar( struct pci_dev *pdev, int bar )
{
	struct Scsi_Host *host = NULL;
	struct adap_info *ap   = NULL;
	ulong  base, len, flag;
	ulong  base_addr;
	
	HFC_ENTRY("hfc_remap_pci_bar");

	host = pci_get_drvdata(pdev);
	if( host != NULL )
	{
		ap = (struct adap_info *)host->hostdata;
	}
	else
	{	/* "host" is NULL */
		ap = NULL;
	}

	flag = pci_resource_flags(pdev, bar);
	if (!(flag & IORESOURCE_MEM))
	{	/* Err at pci_resource_flags()  */ /* Sometime "ap" will be NULL */
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCA, NULL, 0) ;
		goto error_case;
	}
	
	base = pci_resource_start(pdev, bar);
	if ( base == 0 )
	{	/* Err at pci_resource_start() */ /* Sometime "ap" will be NULL */
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCB, NULL, 0) ;
		goto error_case;
	}

	len       = pci_resource_len(pdev, bar);
	base_addr = (ulong)ioremap(base, len);
	if ( base_addr == 0x00 )
	{	/* Err at ioremap */ /* Sometime "ap" will be NULL */
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCD, NULL, 0) ;
		goto error_case;
	}

	HFC_EXIT("hfc_setup_pci_bar");
	return  base_addr;

error_case:

	HFC_EXIT("hfc_setup_pci_bar");
	base_addr = 0x00;
	return  base_addr;
}

/* FCLNX-GPL-154 */
/*
 * Function : hfc_unmap_pci_bar
 *
 * Purpose : Release PCI BAR
 *
 * Arguments :
 *  pdev       - pointer to pci_dev
 *  bar        - Base Address
 * Returns :
 *             None
 *
 * Notes:
 *             pdev is not NULL
 */
void
hfc_unmap_pci_bar( struct pci_dev *pdev, ulong base_addr )
{
	HFC_ENTRY("hfc_remap_pci_bar");
	
	if ( base_addr != 0x00 )
	{
		iounmap((void *)(base_addr));
	}
	
	HFC_EXIT("hfc_unmap_pci_bar");

	return;
}

/* FCLNX-GPL-154 */
/*
 * Function:    hfc_read_reg_ext2
 *
 * Purpose:     FPP register lead 
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *  offset     - register offset address
 *  size       - read size(1/2/4 Bytes)
 *
 * Returns:     FPP register data
 *  reg_size = 1  least significant 1 byte
 *  reg_size = 2  least significant 2 bytes
 *  reg_size = 4  4 bytes
 *
 * Notes:  - Lock adap_info before calling this function
 *           Recommend to call hfc_read_reg() rather than calling this function
 */
uint64_t hfc_read_reg_ext2(
							struct adap_info *ap,
							ulong base_addr,
							uint offset,
							char reg_size
							)
{
	uchar *ptr;
	ushort data16=0,rtn16=0;					/* FCLNX-0659 */
	uint   data32=0,rtn=0;

	if ( offset % reg_size ) {
		return (0);
	}

	ptr = ((uchar *) base_addr) + offset;

	switch (reg_size) {
		case 1: rtn = readb( (uchar  *) ptr );
				break;

		case 2: data16 = readw( (ushort *) ptr );
				HFC_2B_TO_2L(rtn16, data16);	/* FCLNX-0659 */
				rtn = rtn16;					/* FCLNX-0659 */
				break;

		case 4: data32 = readl( (uint   *) ptr );
				HFC_4B_TO_4L(rtn, data32);
				break;

		default: 
				break;
	}

	return(rtn);
}

/* FCLNX-GPL-154 */
/*
 * Function:    hfc_write_reg_ext2
 *
 * Purpose:     PCI memory write
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *  offset     - register offset address
 *  reg_size   - write size (1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       Lock adap_info before calling this function
 *           	Recommend to call hfc_write_reg() rather than calling this function
 */
void hfc_write_reg_ext2(
						struct adap_info *ap,
						ulong base_addr,
						uint offset,
						char reg_size,
						uint64_t data
						)
{
	uchar *ptr;
	ushort data16_1,data16_2;
	uint   data32_1,data32_2;

	if ( offset % reg_size ) {
		return;
	}

	ptr = ((uchar *) base_addr) + offset;

	switch (reg_size) {
		case 1: writeb( (unsigned)data, (uchar  *) ptr ); 
				break;
		case 2: data16_1 = (ushort) data;
				HFC_2B_TO_2L(data16_2, data16_1);
				writew( (unsigned)data16_2, (uchar  *) ptr );
				break;
		case 4: data32_1 = (uint) data;
				HFC_4B_TO_4L(data32_2, data32_1);
				writel( (unsigned)data32_2, (uchar  *) ptr );
				break;

		default: 
				break;
	}

	return;
}

/* FCLNX-GPL-261  New method : Statistical information acquisition */
/*
 * Function:    hfc_read_stat_cca
 *
 * Purpose:     Reads Statistical information
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *  adr        - register offset address
 *
 * Returns:     
 *
 * Notes:       Lock adap_info before calling this function
 */
uint64_t hfc_read_stat_cca(struct adap_info  *ap,  uint adr)
{
	uint64_t ret  = 0;
	uint     wk   = 0;
	
	wk   = (uint) hfc_read_reg_ext(ap, adr, 0x4);
	ret  = wk;
	ret <<= 32;
	wk   = (uint) hfc_read_reg_ext(ap, (adr + 4), 0x4);
	ret  += wk;
	
	return ret;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-261  New method : HBA Port Statistical information acquisition */
/*
 * Function:    hfc_hba_port_statistics_new
 *
 * Purpose:     Sets Statistical information into port_statistics structure
 *
 * Arguments:   
 *  ap         - adap_info structure pointer
 *
 * Returns:     
 *
 * Notes:       Lock adap_info before calling this function
 */
void hfc_hba_port_statistics_new(
	struct adap_info        *ap )
{
	ulong				seconds;

	seconds = ktime_get_real_seconds();

	if (seconds < ap->reset_stat_time)
		ap->port_statistics.seconds_since_last_reset = (uint64_t)seconds - ((uint64_t)1 + (uint64_t)ap->reset_stat_time);
	else
		ap->port_statistics.seconds_since_last_reset = (uint64_t)seconds - (uint64_t)ap->reset_stat_time;

	/* I/O Statistical information */
	ap->port_statistics.tx_frames = ap->tx_frames;
	ap->port_statistics.tx_words  = ap->tx_words;
	ap->port_statistics.rx_frames = ap->rx_frames;
	ap->port_statistics.rx_words  = ap->rx_words;

	/* Error Statistical information from CCA */
	ap->port_statistics.lip_count				= hfc_read_stat_cca(ap, 0x400);
	ap->port_statistics.nos_count				= hfc_read_stat_cca(ap, 0x408);
	ap->port_statistics.error_frames			= hfc_read_stat_cca(ap, 0x410);
	ap->port_statistics.dumped_frames			= hfc_read_stat_cca(ap, 0x418);
	ap->port_statistics.link_failure_count		= hfc_read_stat_cca(ap, 0x420);
	ap->port_statistics.loss_of_sync_count		= hfc_read_stat_cca(ap, 0x428);
	ap->port_statistics.loss_of_signal_count	= hfc_read_stat_cca(ap, 0x430);
	ap->port_statistics.prim_seq_protocol_err_count 
												= hfc_read_stat_cca(ap, 0x438);
	ap->port_statistics.invalid_tx_word_count	= hfc_read_stat_cca(ap, 0x440);
	ap->port_statistics.invalid_crc_count		= hfc_read_stat_cca(ap, 0x448);
	
	return;
}
#endif
#endif

/*
 * Function:    hfc_get_dev_info
 *
 * Purpose:     Get dev_info
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
struct dev_info *hfc_get_dev_info(struct target_info *target, uint lun)		/* FCLNX-GPL-0343 */
{
	struct dev_info		*dev=NULL;

	if(target == NULL) return (NULL);

	dev = target->dev;
	while (dev != NULL) {
		if ( dev->lun == lun ) {
			/* found dev_info */
			break;
		}
		dev = dev->next;
	}	

	return (dev);
	
}																			/* FCLNX-GPL-0343 */

/*
 * Function:    hfc_mp_watchdog_enter
 *
 * Purpose:     Start and Stop watch dog timer 
 *
 * Arguments:   
 *  ap         - Pointer to adap_info (*)
 *  target     - Pointer to target_info 
 *                HFC_ABORT_TMR/HFC_TARGET_RST_TMR (*)
 *  hfcp       - Pointer to hfc_pkt  (*)
 *  lun        - lun# 
 *                HFC_ABORT_TMR (*)
 *  timer_id   - Timer ID
 *  tout       - Time out value (sec) ( 0:default value)
 *  cancel     - FALSE : Start timer, TRUE : Stop(Calcel) timer
 *
 * Returns:     
 *    0        - Start/Stop watch dog timer successfully.
 *    1        - Timer with this timer id has already started or canceled.
 *    2        - Timer is invalid.
 *    3        - Parameters are invalid.  (Needs parameters)
 *
 *
 * Notes:      (*) means parameter is required.
 */
int hfc_mp_watchdog_enter( struct adap_info *ap, struct target_info *target,
						struct hfc_pkt *hfcp, struct dev_info *dev, uint lun, uchar timer_id, 	/* FCLNX-0627 *//* FCLNX-GPL-0343 */
						uint tout, int cancel)
{
	struct wtimer *w_timer;
	uint d_time=0;

	/* Search target */
	if(cancel == 0)
	{
		switch (timer_id)
		{
			case HFC_DELAY_TMR_DEV:	
								if( dev == NULL )
									return (3);
								d_time = (ap->lun_reset_delay * HZ);
								w_timer = &dev->lun_delay_wdog; 
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								timer_setup(&dev->lun_delay_wdog.dog, hfc_watchdog, 0); 
								break;

			default :
								return (2);			/* Invalid TIMER ID		*/
		}
		if(!(&w_timer->dog))
		{        
        	 HFC_DBGPRT("watchdog_enter() - w_timer = NULL before init \n");
		}

		w_timer->ap = ap;
		w_timer->ap_dev_minor = ap->dev_minor;
		w_timer->target = target;
		w_timer->timer_id = timer_id;
		w_timer->hfcpk = hfcp;
		w_timer->dev = dev;
		w_timer->dog.expires = jiffies + d_time;
		/* kernel 4.15+: timer_list.data removed */
		/* kernel 4.15+: callback set by timer_setup; explicit assign removed */
		w_timer->timer_flag |= HFC_TIMER_VALID;
		add_timer(&w_timer->dog);

	}
	else{
		switch (timer_id)
		{
			case HFC_DELAY_TMR_DEV:	
				if( dev == NULL )
					break;
				w_timer = &dev->lun_delay_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					return(3);						/* FCLNX-0648 */ /* FCLNX-0657 */
				/* kernel 6.x: &w_timer->dog is always non-NULL (embedded struct); check removed */
				if (w_timer->dog.function != NULL) {
					del_timer(&w_timer->dog);
					w_timer->dog.function =  NULL;
		/* kernel 4.15+: timer_list.data removed */
					w_timer->timer_flag &= ~HFC_TIMER_VALID;
				}
				break;

			default :
				return (2);			/* Invalid TIMER ID		*/
		}
	}
	
	return(0);

}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

/*
 * Function:    hfc_clear_dev_info
 *
 * Purpose:     Reset dev_info->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_clear_dev_info(struct dev_info *dev)								/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	if(dev == NULL) return;	
	clear_bit( HFC_LUN_RESET_DELAY_TO, (ulong *)&dev->io_status );
	if(!hfc_manage_info.hfcldd_mp_mod)					/* FCLNX-GPL-FX-152 */
		dev->lustat &= ~HFC_LUNRST_DELAY;

	return;
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

void hfc_all_clear_dev_info(struct adap_info *ap, struct dev_info *dev)		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	while (dev != NULL) {
		/* stop LUN Reset Delay Timer */
		hfc_mp_watchdog_enter(ap, NULL, NULL, dev, 0, HFC_DELAY_TMR_DEV, 0, TRUE);
		
		hfc_clear_dev_info(dev);
		dev = dev->next;
	}
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */


/*
 * Function:    hfc_set_dev_info
 *
 * Purpose:     Set dev_info->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_set_dev_info(struct dev_info *dev)									/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	if(dev == NULL) return;	
	set_bit( HFC_LUN_RESET_DELAY_TO, (ulong *)&dev->io_status );
	if(!hfc_manage_info.hfcldd_mp_mod)					/* FCLNX-GPL-FX-152 */
		dev->lustat |= HFC_LUNRST_DELAY;
	
	return;
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

/*
 * Function:    hfc_search_dev_info
 *
 * Purpose:     Set dev_info->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
struct dev_info *hfc_search_dev_info(struct target_info *target, struct hfc_pkt *hfcp)		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{
	struct dev_info		*dev=NULL;

	if(target == NULL) return (NULL);
	if(hfcp == NULL) return (NULL);
	dev = target->dev;
	while (dev != NULL) {
		if ( dev->lun == hfcp->lun_id ) {
			/* found dev_info */
			break;
		}
		dev = dev->next;
	}	

	return (dev);
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

/*
 * Function:    hfc_free_dev
 *
 * Purpose:     
 *
 * Arguments:   
 *  target     - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_free_dev(struct target_info *target)
{
	struct dev_info *dev,*dev_next;
	struct adap_info *ap;
	
	HFC_ENTRY("hfc_free_dev");
	
	ap = target->ap;
	dev = target->dev;
	while (dev != NULL) {
		dev_next = dev->next;
		hfc_kfree(ap, dev);
		dev = dev_next;
	}
}
