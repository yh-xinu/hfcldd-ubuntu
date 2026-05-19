
/*
 * hfcl_top_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char top_fx_rcsid[] = "$Id: hfcl_top_fx.c,v 1.1.2.46.2.7.2.4.2.9 2015/08/05 11:48:37 toyo Exp $";

#include "hfcldd.h"
#include "hfcl_mlpf.h"
#include "hfcl_tbol.h"
#include "hfcl_strategy.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_top.h"
#include "hfcl_ioctl.h"

#include "hfcldd_fx.h"
#include "hfcl_detect_fx.h"
#include "hfcl_strategy_fx.h"
#include "hfcl_stra_trace_fx.h"
#include "hfcl_timer_recovery_fx.h"
#include "hfcl_top_fx.h"
#include "hfcl_mlpf_fx.h"
#include "hfcl_npiv_fx.h"
#include "hfcl_hand_timer_trace_fx.h"

extern struct manage_info hfc_manage_info;

uint hfc_fx_uniq_seq_num( struct port_info *pp );
void hfc_fx_release_seq_num( struct port_info *pp, int num );

int hfc_fx_send_gpnid(struct port_info *pp, struct core_info *core);
int hfc_fx_send_login(struct port_info *pp);
int hfc_fx_send_pdisc(struct port_info *pp, struct core_info *core);
int hfc_fx_send_gidpn(struct port_info *pp, struct core_info *core);
uchar hfc_fx_first_timeout(struct port_info *pp, struct wtimer_fx *wtime,
		int *delay_num, int *link_num, int *rst_num, int *abt_num); 

void hfc_fx_top_trace(	
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx  *target,
	struct hfc_pkt_fx		*hfcp,
	uint64_t				etc1,
	uint64_t				etc2,
	uint64_t				etc3);


/*
 * Function:    hfc_fx_copy_iocinfo
 *
 * Purpose:     Copy information from IOCINFO to port_info.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *
 * Execution level: user/kernel/interrupt
 *
 * Notes:       Lock port_info.
 *              <Setup the elements of pp : 
 *					pp->scsi_id,ww_name,node_name,used_nmsrv, 
 *					connect_type,max_data_rate >
 */
void hfc_fx_copy_iocinfo( struct port_info *pp, struct core_info *core )
{
#if 0
	uchar flag;
	uint64_t port_id;

	flag    = (char    ) hfc_fx_read_val( core->fw_init_p->fw_iocinfo.configure_flag );
	port_id = (uint64_t) hfc_fx_read_val( core->fw_init_p->fw_iocinfo.port_id );

	if ( flag & HFC_FX_ALPA_VALID )						/* AL_PA			*/
		pp->scsi_id = (port_id >> 24) & 0xff;

	if ( flag & HFC_FX_PID_VALID )							/* SCSI_ID			*/
		pp->scsi_id = port_id & 0xffffff;
#endif

	struct core_info	*wk_core=NULL;
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {	/* FCLNX-GPL-FX-206 */
		/* wk_core is physical core*/
		wk_core =  pp->pport->region_arg[0]->core_arg[core->core_no];
	}
	else {
		wk_core = core;
	}
	
	pp->host_alpa = (uchar) wk_core->fw_init_p->fw_iocinfo.assign_alpa;
	pp->used_nmsrv = FALSE;
	pp->connect_type = (uchar) wk_core->fw_init_p->fw_iocinfo.connect_type;

	/* Setup max data rate to port_info */
	switch ( hfc_fx_read_val( wk_core->fw_init_p->fw_iocinfo.trans_rate ) ) {
		case HFC_40GBPS: pp->max_data_rate = HFC_4000MBS; break;
		case HFC_16GBPS: pp->max_data_rate = HFC_1600MBS; break;
		case HFC_10GBPS: pp->max_data_rate = HFC_1000MBS; break;
		case HFC_8GBPS : pp->max_data_rate = HFC_800MBS ; break;
		case HFC_4GBPS : pp->max_data_rate = HFC_400MBS ; break;
		case HFC_2GBPS : pp->max_data_rate = HFC_200MBS ; break;
		case HFC_1GBPS : pp->max_data_rate = HFC_100MBS ; break;
		default:		 pp->max_data_rate = HFC_100MBS ;
	}

	return;
}


/*
 * Function:    hfc_fx_copy_master_to_slave
 *
 * Purpose:     Copy information of init_table to other core_info's init_table.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *
 * Execution level: user/kernel/interrupt
 *
 * Notes:       Lock port_info.
 *              <Setup the elements of pp : 
 *					pp->scsi_id,ww_name,node_name,used_nmsrv, 
 *					connect_type,max_data_rate >
 */
void hfc_fx_copy_master_to_slave( struct port_info *pp, struct core_info *core )
{
	uint	i;
	struct core_info	*core_tmp=NULL;

	for( i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core_tmp = pp->region_arg[pp->rid]->core_arg[i];
		if( core_tmp != NULL ){
			if( core_tmp->core_no != core->core_no ){
				memcpy( &core_tmp->fw_init_p->fw_iocinfo.connect_type, &core->fw_init_p->fw_iocinfo.connect_type, 64);
				memcpy( core_tmp->fw_init_p->pos_map, core->fw_init_p->pos_map, 160);
				memcpy( core_tmp->fw_init_p->active_alpa, core->fw_init_p->active_alpa, 32);
				memcpy( &core_tmp->fw_init_p->sfp_info.sfp_status, &core->fw_init_p->sfp_info.sfp_status, 0x40); /* FCLNX-GPL-FX-149 */
			}
		}
	}

	return;
}


/*
 * Function:    hfc_fx_hash_target_valid
 *
 * Purpose:     Return the pointer to corresponding target_info_fx
 *
 * Arguments:   
 *  p          - Pointer to port_info
 *  target_id  - Target ID
 *
 * Returns:     
 *  != NULL    - Pointer to target_info_fx
 *   = NULL    - No target_info_fx exists
 *
 * Notes:       
 */
struct target_info_fx *hfc_fx_hash_target_valid(struct port_info *pp, uint target_id )
{
	register struct target_info_fx *target;

	target = pp->target_arg[ pp->tid_map[target_id] ];	

	while (target != NULL)
	{
		if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) )
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
 * Function:    hfc_fx_hash_target_info
 *
 * Purpose:     Return the pointer to the target_info_fx for specified target_id
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *  target_id  -
 *
 * Returns:     
 *  != NULL    - target_info_fx pointer
 *  = NULL     - no target_info_fx.
 *
 * Notes:       
 */
struct target_info_fx *hfc_fx_hash_target_info(struct port_info *pp, uint target_id )
{
	register struct target_info_fx *target;
	
	if ((target = hfc_fx_hash_target_valid(pp, target_id)) != NULL)
	{
		if ( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) )
			return (target);
	}
	
	return (NULL);
}


/*
 * Function:    hfc_fx_pseq_target_info_fx
 *
 * Purpose:     Return the pointer to the target_info_fx for specified pseq#
 *
 * Arguments:   
 *  pp         - Pointer to port_info structure
 *  psecq      - PSEQ#
 *
 * Returns:     
 *  != NULL    - target_info_fx pointer
 *  = NULL     - no target_info_fx.
 *
 * Notes:       
 */
struct target_info_fx *hfc_fx_pseq_target_info_fx(struct port_info *pp, uint pseq )
{
	register struct target_info_fx *target;

	if ( pseq >= pp->max_target )
		return (NULL);

	target = pp -> target_arg[pseq];

	if( target != NULL )
	{
		return (target);           /* Is target_info_fx valid? */
	}

	return (NULL);
}


/*
 * Function:    hfc_fx_hash_target_info_wwn
 *
 * Purpose:     Return the pointer to the target_info_fx for specified wwpn
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  ww_name    - 
 *
 * Returns:     
 *  != NULL    - target_info_fx pointer
 *  = NULL     - no target_info_fx.
 *
 *
 * Notes:       
 */
struct target_info_fx *hfc_fx_hash_target_info_wwn(struct port_info *pp, uint64_t ww_name )
{
	struct target_info_fx *target;
	uint i;

	for ( i=0; i<MAX_TARGET_PROBE; i++ )
	{
		target = pp->target_arg[ pp->tid_map[i] ];
		
		while (target != NULL)
		{
			if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)
			 &&  test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) )
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
 * Function:    hfc_fx_hash_target_info_wwn_no_flag
 *
 * Purpose:     Return the pointer to the target_info_fx for specified wwpn
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  ww_name    - 
 *
 * Returns:     
 *  != NULL    - target_info_fx pointer
 *  = NULL     - no target_info_fx.
 *
 *
 * Notes:       
 */
struct target_info_fx *hfc_fx_hash_target_info_wwn_no_flag(struct port_info *pp, uint64_t ww_name )
{
	struct target_info_fx *target=NULL;
	uint i;

	for ( i=0; i<MAX_TARGET_PROBE; i++ )
	{
		target = pp->target_arg[ pp->tid_map[i] ];
		
		while (target != NULL)
		{
			if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) )
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
 * Function:    hfc_fx_uniq_seq_num
 *
 * Purpose:     Return unique sequence number for specified target
 *
 * Arguments:   
 *  pp         - Adap_info structure pointer
 *
 * Returns:     
 *  Value of 0 or more - seq#
 *
 * Notes:       
 */
uint hfc_fx_uniq_seq_num( struct port_info *pp )
{
	uint i;

	for ( i=0; i<pp->max_target; i++ )
	{
		if ( !( pp->seq_num_tbl[i / 32] & ( 1 << (i % 32)) ) ) {
			pp->seq_num_tbl[ i / 32 ] |= 1 << (i % 32);
			return ( i );
		}
	}
	return 0xffffffff;
}


/*
 * Function:    hfc_fx_release_seq_num
 *
 * Purpose:     Release sequence number for target
 *
 * Arguments:   
 *  pp         - port_info 
 *  num        - seq#
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_release_seq_num( struct port_info *pp, int num )
{
	pp->seq_num_tbl[ (num / 32) ] &= ~( 1 << (num % 32) );
}


/*
 * Function:    hfc_fx_read_cfg
 *
 * Purpose:     PCI configuration register read
 *
 * Arguments:   
 *  pp         - port_info
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
 * Notes:      - Lock port_info before calling this function
 */
uint64_t hfc_fx_read_cnfg( struct port_info *pp, uint offset, char reg_size )
{
	unsigned char	 byte;
	uint16_t word;
	uint32_t dword, data=0;
	
//	HFC_DBGPRT("hfcldd%d hfc_fx_read_cnfg start\n",pp->dev_minor);

	switch( reg_size ){
	case 1:
		pci_read_config_byte( pp->pci_cfginf, offset, &byte );
		data = (uint32_t)byte;
		break;
	case 2:
		pci_read_config_word( pp->pci_cfginf, offset, &word );
		data = (uint32_t)word;
		break;
	case 4:
		pci_read_config_dword( pp->pci_cfginf, offset, &dword );
		data = (uint32_t)dword;
		break;
	}
	
//	HFC_DBGPRT("hfcldd%d hfc_fx_read_cnfg end\n",pp->dev_minor);
	
	return( data );
}


/*
 * Function:    hfc_fx_write_cnfg
 *
 * Purpose:     PCI configuration register write
 *
 * Arguments:   
 *  pp         - port_info 
 *  offset     - register offset
 *  reg_size   - write size(1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_write_cnfg( struct port_info *pp, uint offset,
										 char reg_size, uint64_t data )
{
	switch( reg_size ){
	case 1:
		pci_write_config_byte( pp->pci_cfginf, offset, (unsigned char)data );
		break;
	case 2:
		pci_write_config_word( pp->pci_cfginf, offset, (uint16_t)data );
		break;
	case 4:
		pci_write_config_dword( pp->pci_cfginf, offset, (uint32_t)data );
		break;
	default:
		break;
	}
}


/*
 * Function:    hfc_fx_read_reg_ext
 *
 * Purpose:     FPP register lead 
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  offset     - register offset address
 *  size       - read size(1/2/4 Bytes)
 *
 * Returns:     FPP register data
 *  reg_size = 1  least significant 1 byte
 *  reg_size = 2  least significant 2 bytes
 *  reg_size = 4  4 bytes
 *
 * Notes:  - Lock port_info before calling this function
 *           Recommend to call hfc_fx_read_reg() rather than calling this function
 */
uint64_t hfc_fx_read_reg_ext(struct port_info *pp, uint offset, char reg_size)
{
	uchar *ptr;
	ushort data16=0,rtn16=0;					/* FCLNX-0659 */
	uint   data32=0,rtn=0;

	if ( offset % reg_size ) {
		return (0);
	}

	ptr = ((uchar *) pp->mem_base_addr) + offset;

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
 * Function:    hfc_fx_write_reg_ext
 *
 * Purpose:     PCI memory write
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  offset     - register offset address
 *  reg_size   - write size (1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 *           	Recommend to call hfc_fx_write_reg() rather than calling this function
 */
void hfc_fx_write_reg_ext(struct port_info *pp, uint offset,
									 char reg_size, uint64_t data)
{
	uchar *ptr;
	ushort data16_1,data16_2;
	uint   data32_1,data32_2;

	if ( offset % reg_size ) {
		return;
	}

	ptr = ((uchar *) pp->mem_base_addr) + offset;

	switch (reg_size) {
	case 1:
		writeb( (unsigned)data, (uchar  *) ptr );
		break;
	case 2:
		data16_1 = (ushort) data;
		HFC_2B_TO_2L(data16_2, data16_1);
		writew( (unsigned)data16_2, (uchar  *) ptr );
		break;
	case 4:
		data32_1 = (uint) data;
		HFC_4B_TO_4L(data32_2, data32_1);
		writel( (unsigned)data32_2, (uchar  *) ptr );
		break;

	default: 
		break;
	}

	return;
}

/*============================================================================*/
/* NAME        : hfc_fx_read_ramid()
 * Aargument   : pp        ---- Address for struct port_info
 *             : addr      ---- RAM Address(Add RAM Base Address)
 *             : mask      ---- RAM Mask(Access Type)
 *             : tcore     ---- Access Target Core#
 * Return      : Read Data
 *                --Error Case
 *                  0xFFFFFFFF: Undefined Area(HW Specific)
 *                  0xFEFEFEFE: Indirect Access Error (Case 1)
 *                  0xFDFDFDFD: Indirect Access Error (Case 2)
 * Description : Interface Libraly for Device RAM Indirect Read Access
 */
/*============================================================================*/
uint hfc_fx_read_ramid(struct port_info *pp, uint addr, uchar mask, uchar tcore, uint indirect_access )
{
	uint	Data	= 0;

	/*==========================================================================
	 *    Argument Check
	 *========================================================================*/
	if (!pp)	{ return  HFCFX_HL_DATA_RAMIE1; }
	
	if( !indirect_access ){				/* FCLNX-GPL-FX-162 */
		return  HFCFX_HL_DATA_RAMIE1;
	}									/* FCLNX-GPL-FX-162 */
	
	/*==========================================================================
	 *    Read RAM Data
	 *========================================================================*/
	/*==== Set Target Core ===================================================*/
	if (tcore & ~HFC_INTE_CORE_MSK) {
//		HFCFX_MMIO_W1(pp, TGTCORE, 0);		/* FCLNX-GPL-FX-173 */
	} else {
		HFCFX_MMIO_W1(pp, TGTCORE, tcore);
	}
	/*==== Set RAM Access Type ===============================================*/
	HFCFX_MMIO_W1(pp, RAMMSK, mask);
	/*==== Set RAM Access Address ============================================*/
	HFCFX_MMIO_W4(pp, RAMADR, addr);
	/*==== Read RAM Data =====================================================*/
	Data = HFCFX_MMIO_R4(pp, RAMAREA);
	/*==========================================================================
	 *    Complate Sequence for RAM INDIRECT Access
	 *========================================================================*/

	return Data;
}

/* FCLNX-GPL-FX-145 */
/*============================================================================*/
/* NAME        : hfc_fx_write_ramid()
 * Aargument   : pp        ---- Address for struct port_info
 *             : addr      ---- RAM Address(Add RAM Base Address)
 *             : mask      ---- RAM Mask(Access Type)
 *             : tcore     ---- Access Target Core#
 *             : data      ---- Write Data
 * Return      : Error Status
 *                  0x00000000: Nomal End
 *                --Error Case
 *                  0xFEFEFEFE: Indirect Access Error (Case 1)
 *                  0xFDFDFDFD: Indirect Access Error (Case 2)
 * Description : Interface Libraly for Device RAM Indirect Write Access
 */
/*============================================================================*/
uint hfc_fx_write_ramid(struct port_info *pp, uint addr, uchar mask, uchar tcore, uint data)
{
	/*==========================================================================
	 *    Argument Check
	 *========================================================================*/
	if (!pp)	{ return  HFCFX_HL_DATA_RAMIE1; }
	/*==========================================================================
	 *    Check RAM Indirect Access
	 *========================================================================*/
	/*==== Check RAM Access Flag =============================================*/
	if (HFCFX_MMIO_R4(pp, RAMADR) & HFCFX_MMIO_RAMADR_FLG) {
		return  HFCFX_HL_DATA_RAMIE1;
	}
	/*==== Set Enable RAM Access =============================================*/
	HFCFX_MMIO_W1(pp, IDFLGEN, HFCFX_MMIO_IDFLGEN_IDFLGEN);
	/*==== Check RAM Access ==================================================*/
	if (HFCFX_MMIO_R4(pp, RAMADR) & HFCFX_MMIO_RAMADR_FLG) {
		/* Clear Flag */
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00);		/* FCLNX-GPL-FX-162 */
		return  HFCFX_HL_DATA_RAMIE2;
	}
	/*==========================================================================
	 *    Read RAM Data
	 *========================================================================*/
	/*==== Set Target Core ===================================================*/
	if (tcore & ~HFC_INTE_CORE_MSK) {
		HFCFX_MMIO_W1(pp, TGTCORE, 0);
	} else {
		HFCFX_MMIO_W1(pp, TGTCORE, tcore);
	}
	/*==== Set RAM Access Type ===============================================*/
	HFCFX_MMIO_W1(pp, RAMMSK, mask);
	/*==== Set RAM Access Address ============================================*/
	HFCFX_MMIO_W4(pp, RAMADR, addr);
	/*==== Read RAM Data =====================================================*/
	HFCFX_MMIO_W4(pp, RAMAREA, data);
	/*==========================================================================
	 *    Complate Sequence for RAM INDIRECT Access
	 *========================================================================*/
	/**** Finalize process of indirect access ****/
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,  0x01, 0x00);	
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,  0x04, 0x80000000);
	/* Clear indirect access flag */	
	hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN, 0x01, 0x00); 

	return (uint)0;
}
/*==== End of Function "hfc_fx_write_ramid()" ================================*/


/*
 * Function:    hfc_fx_read_flash
 *
 * Purpose:     Read flash memory 
 *
 * Arguments:   
 *  pp         - port_info 
 *  offset     - flash offset address
 *  size       - Read size
 *  buf        - Readout data buffer pointer
 *
 * Returns:
 *  FALSE      - Normal end
 *  TRUE       - Error
 *
 * Notes:       Lock port_info before calling this function
 */
int hfc_fx_read_flash(struct port_info *pp, int offset, int size, uchar *buf)
{
	uint adr=0;
	uchar logdata[16];
	uint  wk_data[4];
	uint data1,data2,i;
	uint wk4;

//	HFC_DBGPRT(" hfcldd%d : hfc_fx_read_flash : pkg.type=%d.\n",pp->instance, pp->pkg.type);

	if ( pp->pkg.type == HFC_PKTYPE_FPP ) {
		adr = 0xa000000;							/* flash start address */
	}
	else {
		
		if( (pp->pkg.type == HFC_PKTYPE_FIVE_EX)||
			(pp->pkg.type == HFC_PKTYPE_FIVE_FX)  ){
//			HFC_DBGPRT(" hfcldd%d : hfc_fx_read_flash : pkg.type is FIVE_EX or FIVE-FX\n", pp->instance);
			/*** flag check (40sec Log-out) ***/ /* FCLNX-GPL-116 */
			i=0;
			while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
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
					
					hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ;/* FCLNX-GPL-161 */
						
/*					return(EIO); */ /* FCLNX-GPL-134 */
				}
				i++;
			}
		}

		/* Enable indirect access flag */
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x08);	/* @1.75 */

		/*** flag check (40sec Log-out) ***/ /* FCLNX-GPL-116 */
		i=0;
		while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
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
				
				hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ; /* FCLNX-GPL-161 */
					
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
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,1,0x20);		/* Type2 */

/*  Never use "HW auto address increment", since supported FIVE-EX.  */
/*	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,adr); */		/* Set adderss */

	i=0;
	while (size) {
		int j=0;
		
		/* Always set RAMADR for FPP,FIVE,FIVE-EX */
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,adr);

		while (1) {
			/* Read data from the top of an indirect memory area + offset 0x100 */
			data1 = (uint)hfc_fx_read_reg_ext( pp,
					 pp->pkg.map->iosp.reg[HFC_IOSPACE_INDAREA]+(adr % 0x100), 4);

			if ( data1 != 0 || j >= 1)
				break;
				
			/* if read data is '0' then retry */
			hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,adr);	/* Address re-setting */
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
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,1,0x00);

	if ( pp->pkg.type == HFC_PKTYPE_FPP )
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,0);
	else
	{
		/* Unlock indirect RAM access lock */
		hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,0x80000000);
		/* Disable indirect access flag */
		hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x00);	/* @1.75 */
	}
	
	return(0);
}

/*
 * Function:    hfc_fx_read_tbl
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
uint64_t hfc_fx_read_tbl( void *ptr, char size )

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
 * Function:    hfc_fx_write_tbl
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
void hfc_fx_write_tbl( void *ptr, char size, uint64_t data64 )
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
 * Function:    hfc_fx_write_indarea
 *
 * Purpose:     Write indirect area
 *
 * Arguments:   
 *  pp         - port_info
 *  core_no    - core no
 *  rammask    - RAMMSK
 *   TYPE0     - 0x80
 *   TYPE1     - 0x40
 *   TYPE2     - 0x20
 *   TYPE3     - 0x10
 *  offset     - indirect area offset address
 *  data       - Writeout data (4byte)
 *
 * Returns:
 *  FALSE      - Normal end
 *  TRUE       - Error
 *
 * Notes:       Lock port_info before calling this function
 */
int hfc_fx_write_indarea(struct port_info *pp, int core_no, int offset, uint data, int rammask)
{
	uint adr=0;
	uchar logdata[16];
	uint  wk_data[4];
	uint i;
	uint wk4=0;

	HFC_DBGPRT(" hfcldd%d : hfc_fx_write_indarea\n",pp->instance);

	/*** flag check (5sec Log-out) ***/
	i=0;
	while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
	{
		mdelay(1000); /* 1sec */
		if( i == 5 ){ /* 5sec */
			wk4 = 0x00000001;
			HFC_4L_TO_4B(wk_data[0], offset);
			HFC_4L_TO_4B(wk_data[1], i);
			HFC_4L_TO_4B(wk_data[2], wk4);
			wk_data[3] = 0;
			memcpy(logdata, (uchar*)wk_data, 16);
			hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ;
			/* return(EIO); */
		}
		i++;
	}

	/* Enable indirect access flag */
	hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x08);

	/*** flag check (5sec Log-out) ***/
	i=0;
	while( hfc_fx_read_reg(pp,HFC_IOSPACE_RAMADR,1) & 0x80)
	{
		mdelay(1000); /* 1sec */
		if( i == 5 ){ /* 5sec */
			wk4 = 0x00000002;
			HFC_4L_TO_4B(wk_data[0], offset);
			HFC_4L_TO_4B(wk_data[1], i);
			HFC_4L_TO_4B(wk_data[2], wk4);
			wk_data[3] = 0;
			memcpy(logdata, (uchar*)wk_data, 16);
			hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xC9, logdata, 16) ;
			/* return(EIO); */
		}
		i++;
	}

	/* Notes: 
	 *  HFC_IOSPACE_RAMADR with bit 0=0 shows locked status in hardware. 
	 *  Other function can not access this flash area until releasing lock
	 */

	/* Set TGTCORE to access core resource */
	hfc_fx_write_reg(pp, HFC_IOSPACE_TGTCORE,1,core_no);

	adr = offset;
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,1,rammask);

	i=0;

	/* Always set RAMADR */
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,adr);
	HFC_DBGPRT(" hfc_fx_write_indarea: adr=%x, indarea=%x, data=%x\n",adr,pp->pkg.map->iosp.reg[HFC_IOSPACE_INDAREA],data);

	/* Write data */
	hfc_fx_write_reg_ext(
		pp,
		pp->pkg.map->iosp.reg[HFC_IOSPACE_INDAREA]+(adr & 0x7f),
		4,
		data);

	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMMSK,1,0x00);
	/* Unlock indirect RAM access lock */
	hfc_fx_write_reg(pp, HFC_IOSPACE_RAMADR,4,0x80000000);
	/* Disable indirect access flag */
	hfc_fx_write_reg(pp, HFC_IOSPACE_IDFLGEN,1,0x00);
	return(0);
}

/*
 * Function:    lock_fx_mailbox
 *
 * Purpose:     Lock and clear mailbox 
 *
 * Arguments:   
 *  pp         -
 *
 * Returns:     
 *
 * Notes:       
 */
void lock_fx_mailbox( struct port_info *pp )
{
	int	rc=0,i=0;
	struct core_info* core=NULL;
	ulong		flags = 0;		/* FCLNX-GPL-FX-466 */
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	rc = lock_fx_try_mailbox( pp );
	while ( rc == 0 ) {				/* MailBox lock failure */
		set_bit( HFC_WAIT_LOCK_MB, (ulong *)&pp->region_arg[pp->rid]->mb_lock );

		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		hfc_fx_sleep_on(&(pp->region_arg[pp->rid]->mb_lock_event), &(pp->region_arg[pp->rid]->mb_lock_event_wait));	/* FCLNX-0296 */
		HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		clear_bit( HFC_WAIT_LOCK_MB, (ulong *)&pp->region_arg[pp->rid]->mb_lock );
		rc = lock_fx_try_mailbox( pp );
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		HFC_BZERO( ( char * )( core -> mb ), sizeof( struct mailbox_fx ) / 8 );
	}
	return;
}


/*
 * Function:    lock_try_fx_mailbox
 *
 * Purpose:     Try to lock mailbox and clear the area if the lock succeeded.
 *
 * Arguments:   
 *  pp         - Pointer to core_info
 *
 * Returns:     
 *  !=0        - Lock success (Mailbox is cleared)
 *  =0         - Lock failure
 *
 * Notes:       
 */
int lock_fx_try_mailbox( struct port_info *pp )
{
	int b=0,i=0;
	struct core_info *core=NULL;
	
	if(pp == NULL){
		HFC_DBGPRT("lock_fx_try_mailbox : pp=NULL");
		return(b);
	}

	if ( !(b = test_and_set_bit(HFC_MAILBOX_BUSY, (ulong *)&( pp->region_arg[pp->rid]->mb_lock ) ) ) ){
		if(pp->region_arg[pp->rid] == NULL){
			HFC_DBGPRT("lock_fx_try_mailbox : rinfo=NULL");
			 return(0);
		}
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			if(core == NULL){
				HFC_DBGPRT("lock_fx_try_mailbox : core=NULL");
				return (0);
			}
			HFC_BZERO( ( char * )( core -> mb ), sizeof( struct mailbox_fx ) / 8 );
		}
		b=1;
		pp->region_arg[pp->rid]->mb_pp = pp;
	}else b=0;

	return (b);
}


/*
 * Function:    unlock_fx_mailbox
 *
 * Purpose:     Unlock mailbox 
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *
 * Notes:       
 */
void unlock_fx_mailbox( struct port_info *pp)
{
	HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY);
	HFC_DBGPRT( "unlock_fx_mailbox pp->status : %x\n", pp->status);
	
	if( test_bit( HFC_WAIT_LOCK_MB, (ulong *)&pp->region_arg[pp->rid]->mb_lock ) )
	{
		HFC_DBGPRT( "unlock_fx_mailbox wake up event\n");
		hfc_fx_wake_up( &pp->region_arg[pp->rid]->mb_lock_event, &pp->region_arg[pp->rid]->mb_lock_event_wait );
	}
	
	return;
}


/*
 * Function:    start_fx_next_mailbox
 *
 * Purpose:     Initiate the target waiting mailbox lock.
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 */
void start_fx_next_mailbox( struct port_info *pp, struct hfc_pkt_fx *hfcp_to)
{
	int tmp_issue_mailbox=0;
	ushort mb_tid=0;
	
	tmp_issue_mailbox = decide_fx_next_mailbox(pp, &mb_tid);
	
	HFC_DBGPRT("hfcldd%d start_fx_next_mailbox - tmp_issue_mailbox=%d pp->issue_mailbox = %d\n", 
		pp->dev_minor,tmp_issue_mailbox, pp->issue_mailbox);
	
	HFC_DBGPRT("hfcldd%d start_fx_next_mailbox - pp->status=%08x pp->status_detail1 = %08x pp->status_detail2 = %08x\n", 
		pp->dev_minor,pp->status, pp -> status_detail1, pp -> status_detail2);
	
	if((!test_bit(HFC_MAILBOX_BUSY, (ulong *)&pp->region_arg[pp->rid]->mb_lock ))
	 && (((test_bit( HFC_PD_MB_DELAY, (ulong *)&pp -> status_detail1 )&&(tmp_issue_mailbox != pp->issue_mailbox))
	 ||  (!test_bit( HFC_PD_MB_DELAY, (ulong *)&pp -> status_detail1 )&&(tmp_issue_mailbox != 0))))){
		pp->issue_mailbox = tmp_issue_mailbox;
		set_bit(HFC_PD_MB_DELAY, (ulong *)&pp->status_detail1 );
		hfc_fx_watchdog_enter(pp, NULL, NULL, hfcp_to, 0, HFC_FX_MB_DELAY_TMR, pp->mb_timer[mb_tid].delay, FALSE);
		HFC_DBGPRT("hfcldd%d start_fx_next_mailbox restart\n", pp->dev_minor);
	}
	
	return;
}


/*
 * Function:    issue_fx_next_mailbox
 *
 * Purpose:     Initiate the target waiting mailbox lock.
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 */
void issue_fx_next_mailbox( struct port_info *pp, struct hfc_pkt_fx *hfcp_to )
{
	struct target_info_fx *target;
	struct dev_info_fx *dev;
	uint lp=0, mb_issue=0;
	ushort mb_tid=0;
	
	
	HFC_DBGPRT("hfcldd%d start_fx_next_mailbox start\n", pp->dev_minor);
	pp->linkdown_occurred = 0 ;	/* FCLNX-GPL-FX-174 */
	
	switch(pp->issue_mailbox){
		case HFC_NMB_CANCEL_SCSI_T_WITHOUT_DMA:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(( target != NULL )&&(test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status ))){	/* FCLNX-GPL-FX-014 */
					( void )hfc_fx_all_cancel_scsi(pp,target,NULL,NULL,HFC_CANCEL_ITNEXUS,HFC_CANCEL_WITHOUT_DMA);	/* FCLNX-GPL-FX-014 */
					break;
				}
			}
			break;
		case HFC_NMB_CANCEL_SCSI_T_WAIT_DMA:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(( target != NULL )&&(test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status ))){
					( void )hfc_fx_all_cancel_scsi(pp,target,NULL,NULL,HFC_CANCEL_ITNEXUS,HFC_CANCEL_WAIT_DMA);
					break;
				}
			}
			break;
		case HFC_NMB_CANCEL_SCSI_D_WITHOUT_DMA:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(target != NULL){
					dev = target->dev;										/* FCLNX-GPL-0343 */
					while(dev != NULL){
						if(test_bit(HFC_DS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat )){
							( void )hfc_fx_all_cancel_scsi(pp,target,dev,NULL,HFC_CANCEL_ITLNEXUS,HFC_CANCEL_WITHOUT_DMA);
							mb_issue=1;
							break;
						}
						dev = dev->next;
					}
					if(mb_issue==1) break;
				}
			}
			break;
		case HFC_NMB_CANCEL_SCSI_D_WAIT_DMA:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(target != NULL){
					dev = target->dev;										/* FCLNX-GPL-0343 */
					while(dev != NULL){
						if(test_bit(HFC_DS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat )){
							( void )hfc_fx_all_cancel_scsi(pp,target,dev,NULL,HFC_CANCEL_ITLNEXUS,HFC_CANCEL_WAIT_DMA);
							mb_issue=1;
							break;
						}
						dev = dev->next;
					}
					if(mb_issue==1) break;
				}
			}
			break;
		case HFC_NMB_OFFLINE_MB:
			if( test_bit( HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp -> status_detail2 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_all_offline_mb(pp);
			break;
		case HFC_NMB_CORE_START:
			if( test_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_all_core_start(pp);
			break;
		case HFC_NMB_LINK_INI:
			if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1))	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_linkini(pp);
			break;
		case HFC_NMB_FLOGI:
			if( test_bit(HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_flogi(pp);
			break;
		case HFC_NMB_LOGO_FCSW:
			if( test_bit( HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_LOGO);
			break;
		case HFC_NMB_DEL_PORTID:
			if( test_bit( HFC_PD_NEED_DEL_PORTID, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_all_del_portid(pp);
			break;
		case HFC_NMB_ADD_PORTID:
			if( test_bit( HFC_PD_NEED_ADD_PORTID, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_all_add_portid(pp);
			break;
		case HFC_NMB_SCR:
			if( test_bit( HFC_PD_NEED_SCR, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_SCR);
			break;
		case HFC_NMB_PLOGI_NS:
			if( test_bit( HFC_PD_NEED_PLOGI_N, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_plogi(pp, NULL);
			break;
		case HFC_NMB_RFTID:
			if( test_bit( HFC_PD_NEED_RFTID, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_RFT_ID);
			break;
		case HFC_NMB_RFFID:
			if( test_bit( HFC_PD_NEED_RFFID, (ulong *)&pp -> status_detail1 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_RFF_ID);
			break;
		case HFC_NMB_GPNFT:
			if( test_bit( HFC_PD_NEED_GPNFT, (ulong *)&pp -> status_detail2 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_frmsndrcv(pp, NULL, 0, HFC_SNDRCV_GPN_FT);
			break;
		case HFC_NMB_PRLI_T:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(( target != NULL )&&(test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status ))){
					( void )hfc_fx_issue_frmsndrcv(pp, target, 0, HFC_SNDRCV_PRLI);
					break;
				}
			}
			break;
		case HFC_NMB_PLOGI_T:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(( target != NULL )&&(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status ))){
					( void )hfc_fx_issue_plogi(pp,target);
					break;
				}
			}
			break;
		case HFC_NMB_LOGO_T:
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
			{
				target = pp->target_arg[lp];
				if(( target != NULL )&&(test_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status ))){
					( void )hfc_fx_issue_frmsndrcv(pp, target, 0, HFC_SNDRCV_LOGO);
					break;
				}
			}
			break;
		case HFC_NMB_MIHLOG:
			if( test_bit( HFC_PD_NEED_MIHLOG, (ulong *)&pp -> status_detail2 ) )	/* FCLNX-GPL-FX-434 */
				( void )hfc_fx_issue_mihlog(pp, hfcp_to);
			break;
		case HFC_NMB_LOAD_CH_TRACE:
			( void )hfc_fx_issue_load_ch_trclog( pp );
			break;
	}
	
	if(decide_fx_next_mailbox(pp, &mb_tid)){
		atomic_set(&pp->check_mbreq, 1);
		HFC_DBGPRT("hfcldd%d issue_fx_next_mailbox - atomic_set(pp->check_mbreq, 1)\n", pp->dev_minor);
	}
	else{
		atomic_set(&pp->check_mbreq, 0);
		HFC_DBGPRT("hfcldd%d issue_fx_next_mailbox - atomic_set(pp->check_mbreq, 0)\n", pp->dev_minor);
	}
	
	HFC_DBGPRT("hfcldd%d issue_fx_next_mailbox end\n", pp->dev_minor);

}

/*
 * Function:    decide_fx_next_mailbox
 *
 * Purpose:     decide next mailbox.
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *
 * Returns:     
 *	0		   - No request to issue mailbox;
 *
 * Notes:       Lock port_info before calling this function
 */
int decide_fx_next_mailbox( struct port_info *pp , ushort *mb_tid)
{
	struct target_info_fx	*target;
	struct dev_info_fx		*dev;
	struct port_info		*wkpp;
	uchar	target_cancel=FALSE;
	uchar	target_cancel_wdma=FALSE;
	uchar	device_cancel=FALSE;
	uchar	device_cancel_wdma=FALSE;
	uchar	target_prli=FALSE;
	uchar	target_plogi=FALSE;
	uchar	target_logo=FALSE;
	uint	lp;
	
	HFC_DBGPRT("hfcldd%d start_fx_next_mailbox start\n", pp->dev_minor);

	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = pp->target_arg[lp];
		if( target != NULL )
		{
			if(test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status )){	/* FCLNX-GPL-FX-014 */
				target_cancel=TRUE;
				break;
			}
			if(test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status )){	/* FCLNX-GPL-FX-014 */
				target_cancel_wdma=TRUE;
				break;
			}
			if(test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status )){
				target_prli=TRUE;
			}
			if(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status )){
				target_plogi=TRUE;
			}
			if(test_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status )){
				target_logo=TRUE;
			}
			
#ifdef _HFC_DEBUG
			/* FCLNX-GPL-FX-446 : Validate target status >>> */
			{
				uchar nr = 0;
				if (test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status))		nr++;
				if (test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status))		nr++;
				if (test_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status))	nr++;

				if (nr > 1)
					HFC_DBGPRT("hfcldd%d : %s - Invalid target status, tid=%d, status=%08x\n",
							   pp->dev_minor, __FUNCTION__, target->target_id, target->status);
			}
			/* FCLNX-GPL-FX-446 <<< */
#endif	/* _HFC_DEBUG */

			dev = target->dev;										/* FCLNX-GPL-0343 */
			while(( dev != NULL)&&(device_cancel != TRUE)&&(device_cancel_wdma != TRUE)){
				if(test_bit(HFC_DS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat)){	/* FCLNX-GPL-FX-014 */
					device_cancel=TRUE;
				}
				if(test_bit(HFC_DS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat)){	/* FCLNX-GPL-FX-014 */
					device_cancel_wdma=TRUE;
				}
				dev = dev->next;
			}
		}														/* FCLNX-GPL-0343 */
	}
	if((target_cancel)||(target_cancel_wdma)||(device_cancel)||(device_cancel_wdma)){
		*mb_tid = HFC_MBTIME_CAN_SCSI;
		if(target_cancel)		return(HFC_NMB_CANCEL_SCSI_T_WITHOUT_DMA);
		if(target_cancel_wdma)	return(HFC_NMB_CANCEL_SCSI_T_WAIT_DMA);
		if(device_cancel)		return(HFC_NMB_CANCEL_SCSI_D_WITHOUT_DMA);
		if(device_cancel_wdma)	return(HFC_NMB_CANCEL_SCSI_D_WAIT_DMA);
	}

	if( test_bit( HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp -> status_detail2 ) ){	/* Send FLOGI */
		*mb_tid = HFC_MBTIME_OFFLINE;
		return(HFC_NMB_OFFLINE_MB);
	}
	
	if( test_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp -> status_detail1 ) ){	/* Send CORE START */
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			*mb_tid = HFC_MBTIME_CORE_START;
			return(HFC_NMB_CORE_START);
		}
		else {
			/* virtual port */
			if (!(test_bit( HFC_PS_WAIT_LINKUP, (ulong *)&pp->pport->status )) &&
				!(test_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->pport->status )) &&
				!(test_bit( HFC_PD_AFTER_LINKUP, (ulong *)&pp->pport->status_detail1 )) &&
				 (test_bit( HFC_PS_ONLINE, (ulong *)&pp->pport->status ))) {
				
				/* physical port link up */
				HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - physical port link up\n");
				*mb_tid = HFC_MBTIME_CORE_START;
				return(HFC_NMB_CORE_START);
			}
			else if (!(pp->pport->status & (~(0x00000001 << HFC_PS_ENABLE))) &&
					(!pp->pport->status_detail1) &&
					(!pp->pport->status_detail2)) {
				
				/* physical port link down */
				HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - physical port link down\n");
				*mb_tid = HFC_MBTIME_CORE_START;
				return(HFC_NMB_CORE_START);
			}
			else {
				/* during link initialaize */
				HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - during link initialaize\n");
				return(0);
			}
		}
	}
	
	if( test_bit( HFC_PD_NEED_MIHLOG, (ulong *)&pp -> status_detail2 ) ){		/* Send MIHLOG *//* FCLNX-GPL-FX-153 */
		*mb_tid = HFC_MBTIME_MIHLOG;
		return(HFC_NMB_MIHLOG);
	}
	
	if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp -> status_detail1 ) ){	/* Send FLOGI */
		*mb_tid = HFC_MBTIME_LINK_INI;
		return(HFC_NMB_LINK_INI);
	}
	
	if( test_bit( HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp -> status_detail1 ) ){	/* Send FLOGI */
		*mb_tid = HFC_MBTIME_LOGO;
		return(HFC_NMB_LOGO_FCSW);
	}
	
	if( test_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp -> status_detail1 ) ){	/* Send FLOGI */
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - physical port\n");
			*mb_tid = HFC_MBTIME_FLOGI;
			return(HFC_NMB_FLOGI);
		}
		else {
			/* virtual port */
			if (!(test_bit( HFC_PS_WAIT_LINKUP, (ulong *)&pp->pport->status )) &&
				!(test_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->pport->status )) &&
				!(test_bit( HFC_PD_AFTER_LINKUP, (ulong *)&pp->pport->status_detail1 )) &&
				 (test_bit( HFC_PS_ONLINE, (ulong *)&pp->pport->status ))) {
				
				if (HFC_FX_MIN_PORT_IN_REGION(pp)) {
					/* physical port link up */
					HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - physical port link up\n");
					*mb_tid = HFC_MBTIME_FLOGI;
					return(HFC_NMB_FLOGI);
				}
				else {
					if ((wkpp = HFC_FX_GET_MIN_PORT_IN_REGION(pp)) != NULL) {
						if (!(test_bit( HFC_PS_WAIT_LINKUP, (ulong *)&wkpp->status )) &&
							!(test_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&wkpp->status )) &&
							!(test_bit( HFC_PD_AFTER_LINKUP, (ulong *)&wkpp->status_detail1 )) &&
							 (test_bit( HFC_PS_ONLINE, (ulong *)&wkpp->status ))) {
							/* minimum port in region link up */
							HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - minimum port in region link up\n");
							*mb_tid = HFC_MBTIME_FLOGI;
							return(HFC_NMB_FLOGI);
						}
						else {
							/* minimum port in region link down */
							HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - minimum port in region link down\n");
							return(0);
						}
					}
					else {
						/* minimum port not exist */
						HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - minimum port not exist\n");
						return(0);
					}
				}
			}
			else {
				/* physical port link down */
				HFC_DBGPRT("hfcldd : decide_fx_next_mailbox - physical port link down\n");
				return(0);
			}
		}
	}
	
	if( test_bit( HFC_PD_NEED_DEL_PORTID, (ulong *)&pp -> status_detail1 ) ){	/* Send Add Port_id */
		*mb_tid = HFC_MBTIME_DEL_PORTID;
		return(HFC_NMB_DEL_PORTID);
	}
	
	if( test_bit( HFC_PD_NEED_ADD_PORTID, (ulong *)&pp -> status_detail1 ) ){	/* Send Add Port_id */
		*mb_tid = HFC_MBTIME_ADD_PORTID;
		return(HFC_NMB_ADD_PORTID);
	}
	
	if( test_bit( HFC_PD_NEED_SCR, (ulong *)&pp -> status_detail1 ) ){		/* Send SCR */
		*mb_tid = HFC_MBTIME_SCR;
		return(HFC_NMB_SCR);
	}
	
	if( test_bit( HFC_PD_NEED_PLOGI_N, (ulong *)&pp -> status_detail1 ) ){	/* Send PLOGI */
		*mb_tid = HFC_MBTIME_PLOGI;
		return(HFC_NMB_PLOGI_NS);
	}
	
	if( test_bit( HFC_PD_NEED_RFTID, (ulong *)&pp -> status_detail1 ) ){		/* Send RFT_ID */
		*mb_tid = HFC_MBTIME_RFT_ID;
		return(HFC_NMB_RFTID);
	}
	
	if( test_bit( HFC_PD_NEED_RFFID, (ulong *)&pp -> status_detail1 ) ){		/* Send RFF_ID */
		*mb_tid = HFC_MBTIME_RFF_ID;
		return(HFC_NMB_RFFID);
	}
	
	if( test_bit( HFC_PD_NEED_GPNFT, (ulong *)&pp -> status_detail2 ) ){		/* Waiting GID_FT */
		*mb_tid = HFC_MBTIME_GPN_FT;
		return(HFC_NMB_GPNFT);
	}
	
	if(target_prli){
		*mb_tid = HFC_MBTIME_PRLI;
		return(HFC_NMB_PRLI_T);
	}
	if(target_plogi){
		*mb_tid = HFC_MBTIME_PLOGI;
		return(HFC_NMB_PLOGI_T);
	}
	if(target_logo){
		*mb_tid = HFC_MBTIME_LOGO;
		return(HFC_NMB_LOGO_T);
	}
	
//	if( test_bit( HFC_PD_NEED_LOAD_CH_TRACE, (ulong *)&pp -> status_detail2 ) ){		/* Waiting GID_FT */
//		*mb_tid = HFC_MBTIME_LOADCHTRC;
//		return(HFC_NMB_LOAD_CH_TRACE);
//	}
	
//	if( test_bit( HFC_PD_NEED_GPNID, (ulong *)&pp -> status_detail1 ) ){		/* Waiting GPN_ID */
//		if ( hfc_fx_send_gpnid( pp, core ) )
//			return;
//	}
	return(0);
	
	HFC_DBGPRT("hfcldd%d start_fx_next_mailbox end\n", pp->dev_minor);

}


/*
 * Function:    hfc_fx_mailbox_initiate
 *
 * Purpose:     Initiate Mailbox request
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
 *  caller     - Caller level 
 *                HFC_MB_PROL  Process level
 *                HFC_MB_INTL  Interrupt level
 *
 * Returns:     
 *
 * Notes:       Secure the lock of port_info
 */
void hfc_fx_mailbox_initiate( struct port_info *pp, struct core_info *core, uint caller )
{
	uint	frame_a=0;

	set_bit(caller, (ulong *)&core->mb_status);
	core -> mb_resp	  = 0; 	
	core -> mb_results  = 0;
	

	HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mailbox_initiate - entry\n", pp->dev_minor);
	
	if(test_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 )
	|| test_bit( HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1 )){	/* FCLNX-GPL-FX-425 */
		frame_a = HFC_FX_FRAMEA_MB_CORE_START;
	}
	else if(test_bit( HFC_PD_NEED_SHADOW_UP, (ulong *)&pp->status_detail2 )
	|| test_bit( HFC_PD_WAIT_SHADOW_UP, (ulong *)&pp->status_detail2 )){	/* FCLNX-GPL-FX-393,425 Start */
		frame_a = HFC_FX_FRAMEA_MB_SHADOW_UP;
	}	/* FCLNX-GPL-FX-393 End */
	else{
		frame_a = HFC_FX_FRAMEA_MB_INIT;
	}
	frame_a |= (uint)(pp->rid << 16);


//#ifdef _HFC_FX_PROTO		/* FCLNX-GPL-FX-164 */
	HFC_DBGPRT(" hfcldd%d: hfc_fx_mailbox_initiate: Mailbox core%d :\n",pp->dev_minor, core->core_no);
//#endif					/* FCLNX-GPL-FX-164 */

	/* Mailbox start */
	hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_FRAMEA,
						  (char)0x4, (int)frame_a, HFC_FX_CORE_OFFSET40);

	if(core -> mb_retry_tout ==  0){	/* FCLNX-GPL-FX-139 Start */
		if( HFC_MBCMD_SNDRCV != hfc_fx_read_val( core->mb->mb_init.mb_code  ) ){
			hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBREQ, 0);
		}else{
			hfc_fx_mb_trace(pp, core, HFC_MBTRC_FRMSND_REQ, 0);
		}
	}	/* FCLNX-GPL-FX-139 End */
	
	HFC_DBGPRT("hfcldd : hfcl_top, hfc_fx_mailbox_initiate frame_a = %08x- exit\n",frame_a);
	
	return;
}

#define MAILBOX_TEMPORARY_BUSY 1
#define MAILBOX_ERROR 2
#define MAILBOX_FATAL -1


/*
 * Function:    hfc_fx_mailbox_response
 *
 * Purpose:     Evaluate Mailbox response
 *
 * Arguments:   
 *  pp             - Pointer to port_info
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
int hfc_fx_mailbox_response( struct port_info *pp, struct core_info *core, uint *fsb_error_code )
{
	int rtn = 0;
	uchar fsb;

	/* Is mailbox response valid? */
	if ( hfc_fx_read_val(core->mb->mb_resp.flag) != 0x80 ) /* valid bit off? */
		return MAILBOX_FATAL;

    *fsb_error_code = 0;
    fsb = (uchar) hfc_fx_read_val( core -> mb -> mb_resp.fsb );

	if ( fsb != 0 ) {
		*fsb_error_code = ( fsb << 24 ) +
			( (uint) hfc_fx_read_val( core->mb->mb_resp.err_code[0] ) << 16 ) +
			( (uint) hfc_fx_read_val( core->mb->mb_resp.err_code[1] ) << 8 ) +
			  (uint) hfc_fx_read_val( core->mb->mb_resp.err_code[2] );
		rtn = MAILBOX_ERROR;
		HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mailbox_response -  MAILBOX_ERROR fsb_errcode=%0x\n", pp->dev_minor, *fsb_error_code);
//		pp -> link_dead_cnt = 0;
	}

	hfc_fx_write_val( core->mb->mb_resp.flag, 0 ); /* Clear response */
	
	HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mailbox_response - end rtn = %d\n", pp->dev_minor, rtn);
	
	return rtn;
}


/*
 * Function:    hfc_fx_mb_passthru
 *
 * Purpose:     Start mailbox
 *
 * Arguments:   
 *  pp         - port_info 
 *	core	   - core_info
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
int hfc_fx_mb_passthrough(struct port_info *pp, struct core_info *core, ushort timer_id, ulong restart, int retry, uint caller)
{
	int rtn=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ timer_id ];
	
	if( (test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) ) ||
		(test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ) ||
		(test_bit( HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2) ) ||
		(test_bit( HFC_PS_ISOL, (ulong *)&pp->status) ) ||	/* FCLNX-GPL-572 */
		!hfc_fx_mlpf_check_normal_hypsts(pp))
		return (EIOF) ; /* Abnormal end */

	/* Start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (rtn = hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* This timer ID is already registered or timer ID is invalid */
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB3, NULL, 0) ;
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - Timer Start Fail \n"); 
		return (EIOF);
	}
	
	memset( &core -> payload->receive_payload, 0, (uint)HFC_RECV_PAYLOADL_MAX );
	
	core -> mb_retry_cnt  = retry;
	core -> mb_retry_tid  = timer_id;
	core -> mb_retry_tout = restart;
	
	HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough - normal end\n", pp->dev_minor);

	hfc_fx_mailbox_initiate( pp, core, caller );
	return (0);
}


/*
 * Function:    hfc_fx_mb_passthrough_rsp
 *
 * Purpose:     Start mailbox
 *
 * Arguments:   
 *  pp         - Pointer to port_info 
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
int hfc_fx_mb_passthrough_rsp(struct port_info *pp, struct core_info *core, uint caller)
{
	int rtn, mb_code=0;						/* return code *//* FCLNX-GPL-FX-17 */
	int rtn_pyld = 0;						/* return code of Sndrcv */
	struct mblog_fx {
		uint fsb_error_code;				/* status byte, error code */
		uchar xcc;							/* condition code */
	} mblog_fx;
	uchar fc_class=0;						/* FCLNX-GPL-FX-17 */
	uint payload_cmd=0, payload_type=0;		/* FCLNX-GPL-FX-096 */
	struct target_info_fx	*target=NULL;	/* FCLNX-GPL-FX-112 */
	struct mb_timer_t	*mb_timer = NULL;	/* FCLNX-GPL-FX-179 */

	/* Does mailbox request timeout? */
	if ( core -> mb_resp == HFC_MBR_TIMEOUT ) {
		core -> mb_status = 0;
		set_bit(HFC_MB_LOCK, (ulong *)&core -> mb_status );
		HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - HFC_MBPASS_TIMEDOUT end\n", pp->dev_minor);
		return (core -> mb_results = HFC_MBPASS_TIMEDOUT) ;
	}

	/* Stop timer and delete ID */
	hfc_fx_watchdog_enter( pp , core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, TRUE);

	/* Check mailbox response */
	if ( ( rtn = hfc_fx_mailbox_response( pp, core, 
			 &( mblog_fx.fsb_error_code ) ) ) == 0 ) {
		/* Check FrameSendRecv payload response */
		if ((rtn_pyld = hfc_fx_payld_response(pp, core, NULL, NULL)) == PAYLD_ACCEPT)
		{
			if (HFC_MBCMD_SNDRCV != hfc_fx_read_val(core->mb->mb_init.mb_code)) {	/* FCLNX-GPL-FX-139 Start */
				hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBRSP, 0);
			}else {
				hfc_fx_mb_trace(pp, core, HFC_MBTRC_FRMSND_RSP, rtn_pyld);
			}	/* FCLNX-GPL-FX-139 End */
			core -> mb_status = 0;
			set_bit(HFC_MB_LOCK, (ulong *)&core -> mb_status );
			HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - SUCCESS end\n", pp->dev_minor);
			return (core->mb_results = 0);
		}
	}

	if (HFC_MBCMD_SNDRCV != hfc_fx_read_val(core->mb->mb_init.mb_code)) {	/* FCLNX-GPL-FX-139 Start */
		hfc_fx_mb_trace(pp, core, HFC_MBTRC_MBRSP, 0);
	}else {
		hfc_fx_mb_trace(pp, core, HFC_MBTRC_FRMSND_RSP, rtn_pyld);
	}	/* FCLNX-GPL-FX-139 End */
	
	core -> mb_results = ( int )mblog_fx.fsb_error_code;

	/* FSB demands retry or Payload Not Accept */
	if (((mblog_fx.fsb_error_code & 0x80000000) || (rtn_pyld != PAYLD_ACCEPT))
	     || pp->mailbox_force_retry)
	{
		if (caller != HFC_MB_INTL) {
			/* Mailbox for ioctl */
			return (HFC_MBPASS_RETRY_OVER);
		}
		
		target = hfc_fx_pseq_target_info_fx(pp, pp->mailbox_pseq);	/* FCLNX-GPL-FX-112 Start */
		if((target!=NULL)&&(test_bit(HFC_TS_CANCEL_SCSI_TARGET, (ulong *)&target->status))){
			HFC_4B_TO_4L(mb_code, core->mb->mb_resp.mb_code);
			if( ((mb_code & 0xffff0000) == HFC_MBCMD_PLOGI)
			|| (((mb_code & 0xffff0000) == HFC_MBCMD_SNDRCV)&&(HFC_PRLI_REQDATA0 == (uchar) hfc_fx_read_val( core->payload->send_payload.data0[0]))))
			return (HFC_MBPASS_RETRY_OVER);
		}															/* FCLNX-GPL-FX-112 End */
		
		if(rtn_pyld != PAYLD_ACCEPT){	/* FCLNX-GPL-FX-096 Start */
			payload_cmd = (uchar) hfc_fx_read_val( core->payload->send_payload.data0[0]);			
			if(payload_cmd == HFC_GXX_REQDATA0){
				payload_type = ( (ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[0] ) << 8 ) +
					(ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[1] );
				if((payload_type == HFC_GIDFT_REQDATA8)||(payload_type == HFC_GPNFT_REQDATA8)){
					HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - RETRY_OVER by GPN_FT or GID_FT rejected.\n", pp->dev_minor);
					return (HFC_MBPASS_RETRY_OVER_GPNFT);	/* FCLNX-GPL-FX-139 */
				}
			}
		}	/* FCLNX-GPL-FX-096 End */
		
		/* FCLNX-GPL-FX-17 Start */
		if(((mblog_fx.fsb_error_code & 0x00ffffff) == HFC_FSB_ERR_CLASS_NOT_SUPPORT1)
		|| ((mblog_fx.fsb_error_code & 0x00ffffff) == HFC_FSB_ERR_CLASS_NOT_SUPPORT2)){
			HFC_4B_TO_4L(mb_code, core->mb->mb_resp.mb_code);
			if(((mb_code & 0xffff0000) == HFC_MBCMD_FLOGI)
			|| ((mb_code & 0xffff0000) == HFC_MBCMD_PLOGI)
			|| ((mb_code & 0xffff0000) == HFC_MBCMD_SNDRCV)
			|| ((mb_code & 0xffff0000) == HFC_MBCMD_PDISC)){
				fc_class = hfc_fx_read_val( core->mb-> mb_init.type.cmn_ctl.fc_class);
				if(fc_class == HFC_FC_CLASS3){
					hfc_fx_write_val( core->mb->mb_init.type.cmn_ctl.fc_class, HFC_FC_CLASS2);
					HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - Change the fc_class to HFC_FC_CLASS2\n", pp->dev_minor);
				}else{
					hfc_fx_write_val( core->mb->mb_init.type.cmn_ctl.fc_class, HFC_FC_CLASS3);
					HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - Change the fc_class to HFC_FC_CLASS3\n", pp->dev_minor);
				}
			}
		}
		/* FCLNX-GPL-FX-17 End */
		
		/* FCLNX-GPL-FX-179 */
		if( (mblog_fx.fsb_error_code & 0x80ffffff) == 0x80ff1024) {
			HFC_4B_TO_4L(mb_code, core->mb->mb_resp.mb_code);
			if( (mb_code & 0xffff0000) == HFC_MBCMD_FLOGI ){
				mb_timer = &pp->mb_timer[ HFC_MBTIME_FLOGI ];
				HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_passthrough_rsp - mb_code = %08x mb_timer->retry = %02x flogi_retry_change = %d\n",
					pp->dev_minor, mb_code, mb_timer->retry, pp->flogi_retry_change); 
				if( (!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT)) && ( pp->flogi_retry_change == 0) ){
					hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
					if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, HFC_FX_MB_LOOPBACK_RETRY ) ))
					{
						/* Timer ID is already registered or invalid */
		 		  	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;
					}
					set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
					HFC_DBGPRT(" hfcldd%d : hfc_fx_top ; hfc_fx_mb_passthrough_rsp - Flogi loopback retry \n",pp->dev_minor); 
					pp->flogi_retry_change = 1;
				}
			}
		}
		/* FCLNX-GPL-FX-179 */
		
		if((pp->mb_timer[core->mb_retry_tid].retry & HFC_FX_MBRTY_POLICY_CNT) ){
			HFC_DBGPRT(" hfcldd : hfc_fx_top ; hfc_fx_mb_passthrough_rsp - retry_tid = %d HFC_FX_MBRTY_POLICY_CNT \n",core->mb_retry_tid); 
			/* retry if it is less than retry count. */
			if (core->mb_retry_cnt == 0) {			/* retry out	*/
				clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);	/* error end */
			}
			core->mb_retry_cnt--;
		}
		
		if ( test_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1) ) {
			core->mb_callback = caller;
			core -> mb_retry_tout = pp->mb_timer[core->mb_retry_tid].tout;	/* FCLNX-GPL-FX-139 */
			set_bit(HFC_CS_MB_RETRY_DELAY, (ulong *)&core->status);	/* FCLNX-GPL-FX-161 */
			hfc_fx_w_stop(pp, core, HFC_FX_MB_RETRY_DELAY_TMR);
			hfc_fx_w_start(pp, core, HFC_FX_MB_RETRY_DELAY_TMR, 1);
			HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - HFC_MBPASS_WAIT_RETRY2 end\n", pp->dev_minor);
			return HFC_MBPASS_WAIT_RETRY;
		}else{
			HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - HFC_MBPASS_RETRY_OVER2 end\n", pp->dev_minor);
			return HFC_MBPASS_RETRY_OVER ;	/* error end */
		}
	}

	if ( rtn == MAILBOX_FATAL ) {
		if (caller != HFC_MB_INTL) {
			/* Mailbox for ioctl */
			return (HFC_MBPASS_RETRY_OVER);
		}
		HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - HFC_MB_FATAL end\n", pp->dev_minor);
		return HFC_MB_FATAL;
	}
	
	HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mb_passthrough_rsp - HFC_MBPASS_ERROR end\n", pp->dev_minor);
	return HFC_MBPASS_ERROR;
}


int hfc_fx_payld_response(struct port_info *pp, struct core_info * core,
                          uchar *reason_code, uchar *code_explanation)
{
	struct mailbox_fx	*mbox = NULL ;
	ushort				payload_type = 0;
	uchar				payload_cmd = 0;
	struct payload_fx	*payload = NULL;
	uchar				payload_resp;
	ushort				payload_length;
	ushort				resp_code = 0 ;

	mbox = core->mb;

	HFC_DBGPRT("hfc_fx_payld_response - entry ");
	
	if (HFC_MBCMD_SNDRCV != hfc_fx_read_val(mbox->mb_init.mb_code)) {
		HFC_DBGPRT("hfc_fx_payld_response - it is not FrameSndRcv");
		return PAYLD_ACCEPT; /* it's necessary to check since it is not FrameSndRcv */
	}
	
	if (reason_code && code_explanation) {
		*reason_code = 0;
		*code_explanation = 0;
	}
	
	payload = core->payload;
	
	payload_cmd = payload->send_payload.data0[0];
	payload_length = (ushort)hfc_fx_read_val(mbox->mb_resp.type.frmsndrcv.recv_payload_length) ;

 	switch (payload_cmd)
	{
	case HFC_PRLI_REQDATA0 :
		payload_resp = (uchar)hfc_fx_read_val( payload->receive_payload.type.prli.data1[0]) ;
		if (payload_resp == HFC_PAYLOAD_ACCEPT) {	/* LS_ACC */
			if (payload_length == HFC_PRLI_RLENGTH) {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLI_REQDATA0 PAYLD_ACCEPT");
				return PAYLD_ACCEPT;
			} else {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLI_REQDATA0 PAYLD_INVLLEN");
				return PAYLD_INVLLEN;
			}
		}
		else{	/* LS_RJT */
			if (reason_code != NULL) {
				*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.prli.data1[5]) ;
			}
			if (code_explanation != NULL) {
				*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.prli.data1[6]) ;
			}
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLI_REQDATA0 PAYLD_REJECT");
			return PAYLD_REJECT;
		}
		break;
	case HFC_PRLO_REQDATA0 :
		payload_resp = (uchar)hfc_fx_read_val( payload->receive_payload.type.prlo.data1[0]) ;
		if (payload_resp == HFC_PAYLOAD_ACCEPT) {	/* LS_ACC */
			if (payload_length == HFC_PRLO_RLENGTH) {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLO_REQDATA0 PAYLD_ACCEPT");
				return PAYLD_ACCEPT;
			} else {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLO_REQDATA0 PAYLD_INVLLEN");
				return PAYLD_INVLLEN;
			}
		}
		else{	/* LS_RJT */
			if (reason_code != NULL) {
				*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.prlo.data1[5]) ;
			}
			if (code_explanation != NULL) {
				*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.prlo.data1[6]) ;
			}
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_PRLO_REQDATA0 PAYLD_REJECT");
			return PAYLD_REJECT;
		}
		break;
	case HFC_SCR_REQDATA0 :
		payload_resp = payload->receive_payload.type.scr.data1[0];

		if (payload_resp == HFC_PAYLOAD_ACCEPT) { // LS_ACC
			if (payload_length == HFC_SCR_RLENGTH) {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_SCR_REQDATA0 PAYLD_ACCEPT");
				return PAYLD_ACCEPT;
			} else {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_SCR_REQDATA0 PAYLD_ACCEPT");
				return PAYLD_INVLLEN;
			}
		} else { // LS_RJT
			if (reason_code != NULL) {
				*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.scr.data1[5]) ;
			}
			if (code_explanation != NULL) {
				*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.scr.data1[6]) ;
			}
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_SCR_REQDATA0 PAYLD_REJECT");
			return PAYLD_REJECT;
		}
		break;
	case HFC_LOGO_REQDATA0 :
		payload_resp = payload->receive_payload.type.logo.data1[0];

		if (payload_resp == HFC_PAYLOAD_ACCEPT) { // LS_ACC
			if (payload_length == HFC_LOGO_RLENGTH) {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_LOGO_REQDATA0 PAYLD_ACCEPT");
				return PAYLD_ACCEPT;
			} else {
				HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_LOGO_REQDATA0 PAYLD_INVLLEN");
				return PAYLD_INVLLEN;
			}
		} else { // LS_RJT
			if (reason_code != NULL) {
				*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.logo.data1[5]) ;
			}
			if (code_explanation != NULL) {
				*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.logo.data1[6]) ;
			}
			HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_LOGO_REQDATA0 PAYLD_REJECT");
			return PAYLD_REJECT;
		}
		break;
	case HFC_GXX_REQDATA0 :
		payload_type = ( (ushort)hfc_fx_read_val( payload->send_payload.type.gxx.data1[0] ) << 8 ) +
				(ushort)hfc_fx_read_val( payload->send_payload.type.gxx.data1[1] );
		HFC_DBGPRT("hfc_fx_frmsndrcv_resp payload_type %04x\n",payload_type);
		switch ( payload_type )
		{
			case HFC_GCSID_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gcs_id.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gcs_id.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
						if( payload_length == HFC_GCSID_RLENGTH ){
							HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
							return PAYLD_ACCEPT;
						}
						else{	/* Invalid Length */
							HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_INVLLEN");
							return PAYLD_INVLLEN;
						}
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gcs_id.data1[5]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gcs_id.data1[6]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_GIDPN_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_pn.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_pn.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
						if( payload_length == HFC_GIDPN_RLENGTH ){
							HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
							return PAYLD_ACCEPT;
						}
						else{	/* Invalid Length */
							return PAYLD_INVLLEN;
							HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_INVLLEN");
						}
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_pn.data1[5]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_pn.data1[6]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_GPNID_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_id.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_id.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
					if( payload_length == HFC_GPNID_RLENGTH ){
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
						return PAYLD_ACCEPT;
					}
					else{	/* Invalid Length */
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_INVLLEN");
						return PAYLD_INVLLEN;
					}
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_id.data1[13]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_id.data1[14]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_GIDFT_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_ft.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_ft.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
					return PAYLD_ACCEPT;
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_ft.data1[13]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gid_ft.data1[14]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_RFTID_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rft_id.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rft_id.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
					if( payload_length == HFC_RFTID_RLENGTH ){
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
						return PAYLD_ACCEPT;
					}
					else{	/* Invalid Length */
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_INVLLEN");
						return PAYLD_INVLLEN;
					}
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rft_id.data1[5]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rft_id.data1[6]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_RFFID_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rff_id.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rff_id.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
					if( payload_length == HFC_RFFID_RLENGTH ){
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
						return PAYLD_ACCEPT;
					}
					else{	/* Invalid Length */
						HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_INVLLEN");
						return PAYLD_INVLLEN;
					}
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rff_id.data1[5]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.rff_id.data1[6]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
			case HFC_GPNFT_REQDATA8 :
				resp_code = ( (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_ft.data1[8] ) << 8 ) +
					  (ushort) hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_ft.data1[9] ) ;
				payload_length = (ushort)hfc_fx_read_val( mbox -> mb_resp.type.frmsndrcv.recv_payload_length) ;
				if( resp_code == HFC_PAYLOAD_GXX_ACCEPT ){	/* LS_ACC */
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_ACCEPT");
					return PAYLD_ACCEPT;
				}
				else{	/* LS_RJT */
					if (reason_code != NULL) {
						*reason_code = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_ft.data1[13]) ;
					}
					if (code_explanation != NULL) {
						*code_explanation = (uchar)hfc_fx_read_val( payload->receive_payload.type.gxx.sub_type.gpn_ft.data1[14]) ;
					}
					HFC_DBGPRT("hfc_fx_frmsndrcv_resp - HFC_GXX_REQDATA0 PAYLD_REJECT");
					return PAYLD_REJECT;
				}
				break;
		}
		
	default:
		break;	
	}
	HFC_DBGPRT("hfc_fx_frmsndrcv_resp - Unknown Payload Type PAYLD_REJECT");
	return PAYLD_REJECT;
}

void hfc_fx_initialize_failed(struct port_info * pp, struct core_info *core, struct target_info_fx *target)
{
	int		lp=0,i=0;
	struct target_info_fx 	*target_work=NULL ;
	struct core_info		*core_wk=NULL ;
	
	HFC_DBGPRT("hfcldd%d hfc_fx_initialize_failed start\n", pp->dev_minor);
	
	/* FCLNX-GPL-FX-005 */
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKINIT_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKUP_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_MCK_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */
	clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
	clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);
	
	/* Cancel pending SCSI processes under this adapter */
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target_work = hfc_fx_hash_target_info(pp, lp);
		if( target_work != NULL )
		{
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core_wk = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				hfc_fx_cancel_scsi_cmd(pp,core_wk,target_work,0, NULL, SCS_LINKUP_TO, HFC_CSCSI_ERROR,	/* FCLNX-0429 */
					TRUE, TRUE, HFC_FLASH_TARGET);
			}
			target_work->status = HFC_NON_STATUS;
		}
	}
	hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);
	/* FCLNX-GPL-FX-005 */
	
	switch(pp->issue_mailbox){
		case HFC_NMB_LINK_INI:
		case HFC_NMB_FLOGI:
			break;
		case HFC_NMB_ADD_PORTID:
		case HFC_NMB_SCR:
		case HFC_NMB_PLOGI_NS:
			set_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);
			set_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1);
			atomic_set(&pp->check_mbreq, 1);
			break;
		case HFC_NMB_PLOGI_T:
			set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status );
			atomic_set(&pp->check_mbreq, 1);
			break;
	}
	HFC_DBGPRT("hfcldd%d hfc_fx_initialize_failed end\n", pp->dev_minor);
}


/*
 * Function:    hfc_fx_all_core_start
 *
 * Purpose:     ALL Core_Start
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (CHK-STOP)
 *               0 ended normally : 
 *               CORE_START failed         : HFC_WAIT_CORE_START
 */
int hfc_fx_all_core_start(struct port_info *pp)
{
	int i=0, success_core_start = 0;
	struct core_info * core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_CORE_START ];
	
	HFC_ENTRY("hfc_fx_all_core_start");
		
//	HFC_DBGPRT("hfc_fx_issue_core_start_all() - Start.");
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CORE_START, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	/* mailbox lock */
	if (lock_fx_try_mailbox(pp) == 0) {
		/* mailbox lock NG */
		HFC_DBGPRT("hfc_fx_issue_core_start_all() - lock_fx_try_mailbox() NG.");
		hfc_fx_top_trace
			(HFC_FX_TRC_ISSUE_CORE_START, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );

		return -1;
	}

//	HFC_DBGPRT("hfc_fx_issue_core_start_all() - 1");

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_CORE_START, 0x02, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_all_core_start - Timer Start Fail \n"); 
			return -1;
		}
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
//	HFC_DBGPRT("hfc_fx_issue_core_start_all() - 2");
	/* Start core_start mailbox for all the core */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */

		if (hfc_fx_issue_core_start(pp, core) != 0) {
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_CORE_START, 0x03, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			/* Forced-Checkstop of the core and logout */
			hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
		} else {
			/* Start Success */
			success_core_start++;
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
//			if( test_bit(HFC_IN_LDR_DUMP, (ulong *)&pp->flags) ){	/* TBD */
//				break; /* dump for 1 core */
//			}
		}
	}
	
//	HFC_DBGPRT("hfc_fx_issue_core_start_all() - 3");

	if (success_core_start == 0) { /* all the core_start failed. --> CHK-STOP */
		HFC_DBGPRT("hfc_fx_issue_core_start_all() - core start() NG.");
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x04, pp, pp->region_arg[pp->rid], core, NULL, NULL,
			0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		clear_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1);
		return -1;
	}

	set_bit(HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1);
	clear_bit(HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1);

//	HFC_DBGPRT("hfc_fx_issue_core_start_all() - 4");
	HFC_EXIT("hfc_fx_all_core_start");
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CORE_START, 0x10, pp, pp->region_arg[pp->rid], core, NULL, NULL,
		0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_mailbox_proc
 *
 * Purpose:     Completion is waited for by starting the mailbox
 * Arguments:   
 *  pp          - Pointer to port_info 
 *  target_info_fx - Pointer to target
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
 *              This function clears pp->mb_status and mailbox response flag
 */
int hfc_fx_mailbox_proc( struct port_info *pp, struct core_info *core,
						ushort timer_id, ulong restart, int retry )
{
	int rtn=0, wrtn;			/* return code */
	int cnt;					/* retry counter */
	struct mblog {
		uint fsb_error_code;	/* status byte, error code */
		uchar xcc;				/* condition code */
	} mblog;
	uchar fc_class=0;						/* FCLNX-GPL-FX-326 */
	ulong		flags = 0;		/* FCLNX-GPL-FX-466 */
	
	HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */

	if(!(test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) ) ){
		HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_mailbox_proc - hfc_fx_hwinit_fail \n");
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return(EIO);
	}

	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) ||
		test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status) ||
		test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ||
		test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2) ||
		!hfc_fx_mlpf_check_normal_hypsts(pp) ){	/* FCLNX-GPL-428 */
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		return(EIO);
	}
			
	for ( cnt = retry; cnt >= 0; cnt -- )
	{
		/* Set watchdog timer */
		hfc_fx_watchdog_enter( pp,core,NULL,NULL, 0,timer_id,restart,1 );
		if ( (wrtn = hfc_fx_watchdog_enter( pp,core,NULL,NULL, 0,timer_id,restart,0 )) )
		{
			/* Timer ID is already registered or invalid */
			hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB4, NULL, 0) ;
		}

		if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status ) ||
			test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) )
		{
			/* Machine check recovery process is in progress */
			if( !test_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1 ) )
			{
				/* Stop timer and delete ID */
				hfc_fx_watchdog_enter( pp,core,NULL,NULL,0,timer_id,0,1 );

				/* Return EIO except link initialization case */
				HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
				return(EIO) ;
			}

			/* Wait until Machine check recovery is completed. */
			pp->mck_on_sleep = 1;
			HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
			hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);	
			HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
			pp->mck_on_sleep = 0;

			/* Return EIO if the status is check stop after retry */
			if(test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) )
			{
				/* Stop timer and delete ID */
				hfc_fx_watchdog_enter( pp,core,NULL, NULL, 0,timer_id,0,1 );
				HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
				return( EIO ) ;
			}
			
		}

		/* Start mailbox  */
		hfc_fx_mailbox_initiate( pp, core, HFC_MB_PROL );

		/* Wait for mailbox request completion */
		HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		hfc_fx_sleep_on(&pp->mb_event, &pp->mb_event_wait);
		HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);	/* FCLNX-GPL-FX-466 */
		/* Stop timer and delete ID */
		hfc_fx_watchdog_enter( pp,core,NULL,NULL,0,timer_id,0,1 );
		
		/* Record time */
		pp->mb_prol_sleep_end_time = (uint)jiffies; /* FCLNX-GPL-243 */
		
		/* Watchdog timer timeout? */
		if ( core -> mb_resp == HFC_MBR_TIMEOUT )
		{
			hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT,ERRID_HFCP_EVNT4, 0x51, NULL, 0 );
			hfc_fx_abend( pp, core, HFC_ABEND_MB_TOUT );
			core -> mb_resp = 0;					/* Clear */
			HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
			return (core -> mb_results = ETIMEDOUT );
		}

		/* Check Mailbox response */
		if ( ( rtn = hfc_fx_mailbox_response( pp, core,
				&( mblog.fsb_error_code ) ) ) == 0 )
		{
			core->mb_status = 0;	/* clear */
			HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
			return ( core -> mb_results = 0 ) ;
		}

		/* FSB demands retry? */
		if ( mblog.fsb_error_code & 0x80000000 ){
			if(((mblog.fsb_error_code & 0x00ffffff) == HFC_FSB_ERR_CLASS_NOT_SUPPORT1)
			|| ((mblog.fsb_error_code & 0x00ffffff) == HFC_FSB_ERR_CLASS_NOT_SUPPORT2)){	/* FCLNX-GPL-FX-326 Start */
				fc_class = hfc_fx_read_val( core->mb-> mb_init.type.cmn_ctl.fc_class);
				if(fc_class == HFC_FC_CLASS3){
					hfc_fx_write_val( core->mb->mb_init.type.cmn_ctl.fc_class, HFC_FC_CLASS2);
					HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mailbox_proc - Change the fc_class to HFC_FC_CLASS2\n", pp->dev_minor);
				}else{
					hfc_fx_write_val( core->mb->mb_init.type.cmn_ctl.fc_class, HFC_FC_CLASS3);
					HFC_DBGPRT("hfcldd%d : hfcl_top, hfc_fx_mailbox_proc - Change the fc_class to HFC_FC_CLASS3\n", pp->dev_minor);
				}				
			}	/* FCLNX-GPL-FX-326 End */
			
			continue;
		}

		/* Retry only in TEMPORARY_BUSY case */
		if ( rtn != MAILBOX_TEMPORARY_BUSY )
			break;
	}

	core->mb_status = 0;	/* Clear */
	if ( rtn == MAILBOX_FATAL ) {
		hfc_fx_errlog( pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBRESP,
			 ERRID_HFCP_ERR6, 0x52, ( uchar * )&mblog, sizeof( struct mblog ) );
		hfc_fx_abend( pp, core, HFC_ABEND_MB_RSPERR );
	}

	core -> mb_results = ( int )mblog.fsb_error_code;
	HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);

	return EIO;

}	


/*
 * Function:    hfc_fx_issue_core_start
 *
 * Purpose:     Core_Start
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as core->mb_status
 *               CORE_START is in progress : HFC_WAIT_CORE_START
 *               CORE_START ended normally : 
 *               CORE_START failed         : HFC_WAIT_CORE_START
 */
int hfc_fx_issue_core_start( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_CORE_START ];

	HFC_ENTRY("hfc_fx_issue_core_start");
//	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - entry \n"); 
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - core=NULL \n"); 
		return -1;
	}
	
	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x22, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - MCKPROC \n"); 
		return -1 ;
	}

	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x23, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );	
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - Hyper Status Fail \n"); 
		return -1;
	}

	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x28, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x24, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - ISOL REC PROC \n"); 
		return -1 ;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x25, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - core->mb=NULL \n"); 
		return -1;
	}
	
//	HFC_DBGPRT(" hfcldd : hfc_fx_top 1; hfc_fx_issue_core_start - entry \n"); 

	/* Start timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CORE_START, 0x27, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_core_start - Timer Start Fail \n"); 
		return -1;
	}

//ifdef _HFC_FX_PROTO	/* FCLNX-GPL-FX-164 */
//	HFC_DBGPRT(" hfcldd%d: hfc_fx_probe_one: Init table of core%d :\n",pp->dev_minor,core->core_no);
//	structdump( 0xf0, (uchar *)core->fw_init_p, sizeof(struct fw_init_tbl_fx) );
//#endif				/* FCLNX-GPL-FX-164 */

//	HFC_DBGPRT(" hfcldd : hfc_fx_top 2; hfc_fx_issue_core_start - entry \n"); 

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_CORESTART );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	hfc_fx_write_val( mb -> mb_init.type.core_start.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	hfc_fx_write_val( mb -> mb_init.type.core_start.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val( mb -> mb_init.type.core_start.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */

	hfc_fx_write_val( mb -> mb_init.type.core_start.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.core_start.self_wwnn, (pp -> node_name));

	/* Clear NEED_LINK_INIT and set WAIT_LINK_INIT */
	set_bit( HFC_PD_WAIT_CORE_START, (ulong *)&pp->status_detail1 );
//	clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );	/* TBD */
	if(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT){
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	core -> mb_retry_tid  = HFC_MBTIME_CORE_START;
	core -> mb_retry_tout = 0 ;					/* Default value */
	
//	HFC_DBGPRT("hfcldd%d core_no = %d\n",pp->dev_minor, core->core_no);
	
	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CORE_START, 0x30, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
//	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_core_start - exit \n"); 
	HFC_EXIT("hfc_fx_issue_core_start");

	return 0 ;
}


/*
 * Function:    hfc_fx_all_add_portid
 *
 * Purpose:     add portid all core
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (CHK-STOP)
 *               0 ended normally : 
 *               CORE_START failed         : HFC_WAIT_CORE_START
 */
int hfc_fx_all_add_portid(struct port_info *pp)
{
	int i=0, success_add_portid = 0;
	struct core_info * core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_ADD_PORTID ];
	
	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	if (lock_fx_try_mailbox(pp) == 0) {
		HFC_DBGPRT("hfc_fx_all_add_portid() - lock_fx_try_mailbox() NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x01, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

		return -1;
	}

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_ADD_PORTID, 0x02, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_all_add_portid - Timer Start Fail \n"); 
			return -1;
		}
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;

		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */

		if (hfc_fx_issue_add_portid(pp, core) != 0) {
			if (core->core_no == pp->master_core_no) {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x03, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
				success_add_portid = 0;
				break;
			} else {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x04, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
				hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
			}
		} else {
			hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x05, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
			success_add_portid++;
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
//			if (pp->flags & HFC_IN_LDR_DUMP) {
//				break;
//			}
		}
	}
	
	if (success_add_portid == 0) {
		HFC_DBGPRT("hfc_fx_all_add_portid() - all core NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x06, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return -1;
	}

	set_bit(HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1);
	clear_bit(HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1);

	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_ADD_PORTID, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_issue_add_portid
 *
 * Purpose:     add Port_id
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as core->mb_status
 *               Add Port_id is in progress : HFC_WAIT_ADD_PORTID
 *               Add Port_id ended normally : 
 *               Add Port_id failed         : HFC_WAIT_LINKUP
 */
int hfc_fx_issue_add_portid( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_ADD_PORTID ];
	uint	sid;

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_add_portid - entry \n"); 
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_add_portid - core=NULL \n"); 
		return -1;
	}

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x22, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x23, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x24, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x25, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}

	if( !test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x26, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x27, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	/* Start timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_ADD_PORTID, 0x28, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_ADDPORTID );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( mb -> mb_init.type.add_port_id.trans_s_id, sid);
	hfc_fx_write_val( mb -> mb_init.type.add_port_id.self_wwpn, (pp -> ww_name));

	/* Clear NEED_ADD_PORTID and set WAIT_ADD_PORTID */
	if(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT){
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	core -> mb_retry_tid  = HFC_MBTIME_ADD_PORTID ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );

	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_add_portid - exit \n"); 
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_ADD_PORTID, 0x30, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0 ;
}


/*
 * Function:    hfc_fx_all_del_portid
 *
 * Purpose:     del portid all core
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (CHK-STOP)
 *               0 ended normally : 
 *               CORE_START failed         : HFC_WAIT_CORE_START
 */
int hfc_fx_all_del_portid(struct port_info *pp)
{
	int i=0, success_del_portid = 0;
	struct core_info * core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_DEL_PORTID ];
	
	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	if (lock_fx_try_mailbox(pp) == 0) {
		HFC_DBGPRT("hfc_fx_all_del_portid() - lock_fx_try_mailbox() NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x01, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

		return -1;
	}

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_DEL_PORTID, 0x02, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_del_portid - Timer Start Fail \n"); 
			return -1;
		}
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;

		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */

		if (hfc_fx_issue_del_portid(pp, core) != 0) {
			if (core->core_no == pp->master_core_no) {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x03, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
				success_del_portid = 0;
				break;
			} else {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x04, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
				hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
			}
		} else {
			hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x05, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

			success_del_portid++;
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
//			if (pp->flags & HFC_IN_LDR_DUMP) {
//				break;
//			}
		}
	}
	
	if (success_del_portid == 0) {
		HFC_DBGPRT("hfc_fx_all_del_portid() - all core NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x06, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return -1;
	}

	set_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1);
	clear_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1);

	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_DEL_PORTID, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_issue_del_portid
 *
 * Purpose:     delete Port_id
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as core->mb_status
 *               Delete Port_id is in progress : HFC_WAIT_ADD_PORTID
 *               Delete Port_id ended normally : 
 *               Delete Port_id failed         : HFC_WAIT_LINKUP
 */
int hfc_fx_issue_del_portid( struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mb = NULL;
	uint	sid;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_DEL_PORTID ];

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_del_portid - entry \n"); 
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_del_portid - core=NULL \n"); 
		return -1;
	}

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x22, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x23, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x24, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
		return -1 ;
	}

	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x25, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x27, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
		return -1;
	}
	
	/* Start timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_DEL_PORTID, 0x28, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_DELPORTID );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( mb -> mb_init.type.delete_port_id.trans_s_id, sid);
	hfc_fx_write_val( mb -> mb_init.type.delete_port_id.self_wwpn, (pp -> ww_name));

	/* Clear NEED_LINK_INIT and set WAIT_LINK_INIT */
//	set_bit( HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1 );		/* TBD */
//	clear_bit( HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1 );	/* TBD */
	if(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT){
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	core -> mb_retry_tid  = HFC_MBTIME_DEL_PORTID ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );

	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_add_portid - exit \n"); 
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_DEL_PORTID, 0x30, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0 ;
}


/*
 * Function:    hfc_fx_issue_mihlog
 *
 * Purpose:     Data collection of timeout MIH LOG
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock port_info before calling this function
 */
int hfc_fx_issue_mihlog( struct port_info *pp,
						 struct hfc_pkt_fx *hfcp)
{	
	uint64_t	hfcp_ad;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	uint	did, sid, mb_command, logid=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_MIHLOG ];

	
	if(hfcp != NULL){
		logid = (uint)HFC_LOGID_SOFTLOG;
	}else{
		logid = (uint)HFC_LOGID_LINKINC;
	}

	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_MIHLOG, 0x00, pp, NULL, NULL, NULL, hfcp,
		0, 0, 0 );

	HFC_DBGPRT("hfc: hfc_fx_issue_mihlog start \n");
	
//	if( test_bit( HFC_PS_ISOL, (ulong *)&pp -> status ) ){	/* FCLNX-GPL-572 *//* FCLNX-GPL-FX-067 */
//		hfc_fx_top_trace(
//			HFC_FX_TRC_ISSUE_MIHLOG, 0x01, pp, NULL, NULL, NULL, hfcp,
//			0, 0, 0 );
//		return -1;
//	}/* FCLNX-GPL-FX-067 */
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_MIHLOG, 0x02, pp, NULL, NULL, NULL, hfcp,
			0, 0, 0 );	
		return -1;
	}
	
	rp = pp->region_arg[pp->rid];
	if(hfcp != NULL){
		core = hfcp->core;
	}
	else if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}

	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_MIHLOG, 0x03, pp, NULL, NULL, NULL, hfcp,
			0, 0, 0 );
		return -1;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_MIHLOG, 0x04, pp, core->rp, core, NULL, hfcp,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock failed */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_MIHLOG, 0x05, pp, core->rp, core, NULL, hfcp,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_MIHLOG, 0x06, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_mihlog - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout  ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_MIHLOG, 0x07, pp, core->rp, core, NULL, hfcp,
			0, 0, 0 );
		return -1;
	}

	clear_bit(HFC_PD_NEED_MIHLOG, (ulong *)&pp->status_detail2 );
	set_bit(HFC_PD_WAIT_MIHLOG, (ulong *)&pp->status_detail2);	/*FCLNX-0506*/

	/* Setup mailbox control block */
	mb_command = (HFC_MBCMD_MIHLOG | logid );
	hfc_fx_write_val( mb -> mb_init.mb_code, mb_command );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( mb -> mb_init.type.mih_log.trans_s_id, sid);
	
	memset(&mb->mb_init.type.mih_log.driver_used_area, 0, 
				sizeof(mb->mb_init.type.mih_log.driver_used_area));
	if( hfcp != NULL)
	{
		if( hfcp->cmd_pkt != NULL)
		{
			did = ( uint )( hfcp -> target -> scsi_id & 0x00ffffff );
			hfc_fx_write_val( mb -> mb_init.type.mih_log.trans_d_id, did);
			hfcp_ad = (ulong) hfcp;
			mb->mb_init.type.mih_log.driver_used_area   = (uint64_t)hfcp_ad;
		}
	}
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_MIHLOG;
	core -> mb_retry_tout = 0 ;					/* default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_MIHLOG, 0x10, pp, core->rp, core, NULL, hfcp,
		0, 0, 0 );
	
	return 0;
}	


/*
 * Function:    hfc_fx_issue_load_ch_trclog
 *
 * Purpose:     Load CH Trace Log
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as core->mb_status
 *               Delete Port_id is in progress : HFC_WAIT_LD_CH_TRCLG
 *               Delete Port_id ended normally : 
 *               Delete Port_id failed         : HFC_WAIT_LINKUP
 */
int hfc_fx_issue_load_ch_trclog( struct port_info *pp )
{
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	uint	sid;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_LOADCHTRC ];

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_del_portid - entry \n"); 
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_LOADCHTRC, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x02, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x03, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x04, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}


	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x05, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x06, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x07, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );	
		return -1;									/* MailBox lock fail	*/
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_LOADCHTRC, 0x08, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_load_ch_trclog - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Start timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout  )) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LOADCHTRC, 0x09, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_LDCHTRC );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( mb -> mb_init.type.delete_port_id.trans_s_id, sid);
	hfc_fx_write_val( mb -> mb_init.type.delete_port_id.self_wwpn, (pp -> ww_name));

	/* Clear NEED_LINK_INIT and set WAIT_LINK_INIT */
	set_bit( HFC_PD_WAIT_ADD_PORTID, (ulong *)&pp->status_detail1 );
	clear_bit( HFC_PD_NEED_ADD_PORTID, (ulong *)&pp->status_detail1 );
//	core -> mb_retry_cnt  = HFC_LINK_INIT_RETRY ;
	core -> mb_retry_tid  = HFC_MBTIME_LOADCHTRC ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );

	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_add_portid - exit \n"); 
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_LOADCHTRC, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0 ;
}


/*
 * Function:    hfc_fx_all_cancel_scsi
 *
 * Purpose:     ALL CANCEL_SCSI
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *	target	   - pointer to target_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (F-MCK)
 *               0 ended normally : 
 *               CANCEL_SCSI failed      : HFC_WAIT_CANCEL_SCSI
 */
int hfc_fx_all_cancel_scsi(struct port_info *pp, struct target_info_fx *target, struct dev_info_fx *dev, struct hfc_pkt_fx *hfcp, uchar cancel_nexus, uchar cancel_ctl)
{
	int i=0, success_cancel_scsi = 0;
	struct core_info * core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_CAN_SCSI ];
	
	HFC_DBGPRT("hfc_fx_all_cancel_scsi() - Start.");
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	/* mailbox lock */
	if (lock_fx_try_mailbox(pp) == 0) {
		/* mailbox lock NG */
		HFC_DBGPRT("hfc_fx_all_cancel_scsi() - lock_fx_try_mailbox() NG.");
		hfc_fx_top_trace
			(HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );

		return -1;
	}

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x02, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_all_cancel_scsi - Timer Start Fail \n"); 
			return -1;
		}
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Start core_start mailbox for all the core */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */
		/* for FW I/O performance at LOOP *//* FCLNX-GPL-FX-148,163 */
		if (((pp->connect_type == HFC_FX_AL)||(pp->connect_type == HFC_FX_MULTI_ALPA)) && (i != pp->master_core_no))
			continue;

		if ((hfc_fx_issue_cancel_scsi(pp, core, target, dev, hfcp, HFC_LOGID_SOFTLOG, cancel_nexus, cancel_ctl)) != 0) {
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x03, pp, pp->region_arg[pp->rid], core, target, NULL,
				0, 0, 0 );
			/* Forced-Checkstop of the core and logout */
			hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
		} else {
			/* Start Success */
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
			success_cancel_scsi++;
//			if( test_bit(HFC_IN_LDR_DUMP, (ulong *)&pp->flags) ){	/* TBD */
//				break; /* dump for 1 core */
//			}
		}
	}
	
	if (success_cancel_scsi == 0) { /* all the core_start failed. --> CHK-STOP */
		HFC_DBGPRT("hfc_fx_all_cancel_scsi() - core start() NG.");
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x04, pp, pp->region_arg[pp->rid], core, target, NULL,
			0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		return -1;
	}

	if(cancel_nexus == HFC_CANCEL_ITLNEXUS){
		if(cancel_ctl == HFC_CANCEL_WAIT_DMA){	/* FCLNX-GPL-FX-014 */
			set_bit(HFC_DS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_DS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
		}else{
			set_bit(HFC_DS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_DS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&dev->lustat);	/* FCLNX-GPL-FX-014 */
		}
	}
	else if(cancel_nexus == HFC_CANCEL_ITNEXUS){
		if(cancel_ctl == HFC_CANCEL_WAIT_DMA){	/* FCLNX-GPL-FX-014 */
			set_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
		}else{
			set_bit(HFC_TS_WAIT_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
			clear_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
		}
	}
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x10, pp, pp->region_arg[pp->rid], core, target, NULL,
		0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_issue_cancel_scsi
 *
 * Purpose:     Request to cancel SCSI command to FW
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock port_info before calling this function
 */
int hfc_fx_issue_cancel_scsi( struct port_info *pp,
						 struct core_info *core,
						 struct target_info_fx *target,
						 struct dev_info_fx *dev,
						 struct hfc_pkt_fx *hfcp,
						 uchar logid,
						 uchar cancel_nexus,
						 uchar cancel_ctl)
{
	struct mailbox_fx	*mb = NULL;
	uint	mb_command, sid, did;
	short lun_id=0;
	uchar	id=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_CAN_SCSI ];
	uint64_t	hfcp_ad;
	struct port_info		*vpp;
	struct target_info_fx	*target_wk;
	struct core_info		*core_wk;
	int						i;

	HFC_DBGPRT("hfc: hfc_fx_issue_cancel_scsi start \n");

	if( !(test_bit( HFC_PS_ONLINE, (ulong *)&pp -> status ) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x22, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x23, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x24, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout  ) ) )
	{
		/* Timer ID is already registered or invalid */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x25, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	memset(&mb->mb_init.type.cscsi.driver_used_area, 0, 
				sizeof(mb->mb_init.type.cscsi.driver_used_area));

	if(cancel_ctl == HFC_CANCEL_WAIT_DMA){	/* FCLNX-GPL-FX-014 */
		mb_command = HFC_MBCMD_CSCSI_WAIT_DMA;	/* FCLNX-GPL-FX-112 */
	}else if((cancel_ctl == HFC_CANCEL_MIHREQ)&&((hfcp !=NULL)&&(core == hfcp->core))){
		mb_command = (HFC_MBCMD_CSCSIMIH | (uint)logid );
		hfcp_ad = (ulong) hfcp;
		mb->mb_init.type.cscsi.driver_used_area = (uint64_t)hfcp_ad;
	}else{
		mb_command = HFC_MBCMD_CSCSI_WITHOUT_DMA;	/* FCLNX-GPL-FX-112 */
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, mb_command );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.cs_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	
	hfc_fx_write_val( mb -> mb_init.type.cscsi.frm_ctl, 0);
	hfc_fx_write_val( mb -> mb_init.type.cscsi.fc_class, HFC_FC_CLASS3);
	hfc_fx_write_val( mb -> mb_init.type.cscsi.cnexus, cancel_nexus);
	hfc_fx_write_val( mb -> mb_init.type.cscsi.plogi_param, pp->plogi_param);
	hfc_fx_write_val( mb -> mb_init.type.cscsi.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	
	switch(cancel_nexus){
		case HFC_CANCEL_PNEXUS		:
			break;
		case HFC_CANCEL_ITLNEXUS	:
		case HFC_CANCEL_ITENEXUS	:
			if(cancel_nexus == HFC_CANCEL_ITLNEXUS){
				lun_id = (ushort)dev->lun;
				if(target != NULL){
					hfc_fx_cancel_xob(pp, core, target,lun_id,NULL,HFC_FLASH_DEV);	/* FCLNX-GPL-FX-222 */
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {	/* FCLNX-GPL-FX-274, 285 */
						for (i=1; i<=pp->max_vport_count; i++) {
							vpp = pp->vport_ptr[i].vport_arg;
							if (vpp == NULL)
								continue;
							if (pp->region_arg[vpp->rid] == NULL)
								continue;
							target_wk = vpp->target_arg[target->pseq];
							if (target_wk == NULL)
								continue;
							if ((core_wk = pp->region_arg[vpp->rid]->core_arg[core->core_no]) == NULL)
								continue;
							
							HFC_DBGPRT("hfcldd: hfc_fx_issue_cancel_scsi - mq HFC_FLASH_DEV\n");
							hfc_fx_cancel_xob(pp, core_wk, target_wk, lun_id, NULL, HFC_FLASH_DEV);
						}
					}		/* FCLNX-GPL-FX-274 */
				}
				HFC_DBGPRT("hfcldd: hfc_fx_issue_cancel_scsi - lun_id = %04x\n",lun_id);
				HFC_2L_TO_2B(mb -> mb_init.type.cscsi.fcp_lun.lun, lun_id);
			}else if(cancel_nexus == HFC_CANCEL_ITENEXUS){
				hfcp_ad = (ulong) hfcp;
				mb->mb_init.type.cscsi.driver_used_area = (uint64_t)hfcp_ad;
			}
		case HFC_CANCEL_ITNEXUS		:
			if(target != NULL){
				if(cancel_nexus == HFC_CANCEL_ITNEXUS){
					hfc_fx_cancel_xob(pp, core, target,0,NULL,HFC_FLASH_TARGET);	/* FCLNX-GPL-FX-222 */
					if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {	/* FCLNX-GPL-FX-274, 285 */
						for (i=1; i<=pp->max_vport_count; i++) {
							vpp = pp->vport_ptr[i].vport_arg;
							if (vpp == NULL)
								continue;
							if (pp->region_arg[vpp->rid] == NULL)
								continue;
							target_wk = vpp->target_arg[target->pseq];
							if (target_wk == NULL)
								continue;
							if ((core_wk = pp->region_arg[vpp->rid]->core_arg[core->core_no]) == NULL)
								continue;
							
							HFC_DBGPRT("hfcldd: hfc_fx_issue_cancel_scsi - mq HFC_FLASH_TARGET\n");
							hfc_fx_cancel_xob(pp, core_wk, target_wk, 0, NULL, HFC_FLASH_TARGET);
						}
					}	/* FCLNX-GPL-FX-274, 285 */
				}
				did = ( uint )( target -> scsi_id & 0x00ffffff );
				HFC_DBGPRT("hfcldd: hfc_fx_issue_cancel_scsi - target->scsi_id = %08x\n",did);
				id = (uchar)(did >> 16 );
				hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.d_id[0], id);
				id = (uchar)(did >> 8 );
				hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.d_id[1], id);
				id = (uchar)did;
				hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.d_id[2], id);
			}
			else{
				HFC_DBGPRT("hfcldd: hfc_fx_issue_cancel_scsi - target=NULL\n");
				return(-1);
			}
		case HFC_CANCEL_INEXUS		:
			sid = ( uint )( pp -> scsi_id & 0x00ffffff );
			id = (uchar)(sid >> 16 );
			hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.s_id[0], id);
			id = (uchar)(sid >> 8 );
			hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.s_id[1], id);
			id = (uchar)sid;
			hfc_fx_write_val( mb -> mb_init.type.cscsi.fcph_hdr.s_id[2], id);
			break;
	}
	
	/* Setup callback information and retry count */
	if(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT){
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	core -> mb_retry_tid  = HFC_MBTIME_CAN_SCSI ;
	core -> mb_retry_tout = 0 ;					/* default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_CANCEL_SCSI, 0x30, pp, core->rp, core, target, NULL,
		0, 0, 0 );
	
	return 0;
	
}	


/*
 * Function:    hfc_fx_issue_linkini
 *
 * Purpose:     link initialization 
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as pp->mb_status
 *               Link initialization is in progress : HFC_WAIT_LINK_INIT
 *               Link initialization ended normally : HFC_PS_ONLINE
 *               Link initialization failed         : HFC_WAIT_LINKUP
 */
int hfc_fx_issue_linkini( struct port_info *pp )
{
	uchar alpa;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_LINK_INI ];
	
	HFC_ENTRY("hfc_fx_issue_linkini");
//	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_linkini - entry \n"); 
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_LINKINIT, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x02, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x03, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x04, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}

	if ( test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status )&&(!(HFC_FX_MMODE_CHECK_SHARED(pp)))){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x05, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x06, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
//	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_linkini - 1 \n");

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x07, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x08, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );	
		return -1;									/* MailBox lock fail	*/
	}
//	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_linkini - 2 \n");

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_LINKINIT, 0x09, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_linkini - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Start the timer of Link_ini Mailbox */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR , mb_timer->tout  )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_LINKINIT, 0x0a, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_linkini - link_speed=%x, connect_type=%x \n", pp->linkspeed,pp->topology);

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_LINKINIT );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	alpa = (pp->host_alpa) ? pp->host_alpa : pp->pref_alpa;	
	hfc_fx_write_val( mb -> mb_init.type.link_init.al_pa, alpa );	
	hfc_fx_write_val( mb -> mb_init.type.link_init.link_speed, pp->linkspeed );
	
	if ((pp->topology == HFC_FX_PT2PT) && (pp->multiple_portid)) { /* FCLNX-GPL-FX-135 Start */
		mb->mb_init.type.link_init.connect_type = HFC_FX_F_PORT;
	}
	else if ((pp->topology == HFC_FX_AL) && (pp->multiple_portid)) {
		mb->mb_init.type.link_init.connect_type = HFC_FX_MULTI_ALPA;
	} else if((pp->topology == 0) && (HFC_FX_NPIV_ENABLE(pp) || HFC_FX_MMODE_CHECK_SHADOW(pp))){	/* FCLNX-GPL-FX-137 */
		mb->mb_init.type.link_init.connect_type = HFC_FX_PT2PT;	/* FCLNX-GPL-FX-137 */
	} else {
		hfc_write_val( mb->mb_init.type.link_init.connect_type, pp->topology ); 
	} /* FCLNX-GPL-FX-135 End */
	
	hfc_fx_write_val( mb -> mb_init.type.link_init.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.link_init.self_wwnn, (pp -> node_name));

	/* Clear NEED_LINK_INIT and set WAIT_LINK_INIT */
	clear_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1 );
//	clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );
//	core -> mb_retry_cnt  = HFC_LINK_INIT_RETRY ;
	core -> mb_retry_tid  = HFC_MBTIME_LINK_INI ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );

//	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_linkini - exit \n"); 
	HFC_EXIT("hfc_fx_issue_linkini");
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_LINKINIT, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );

	return 0 ;
}


/*
 * Function:    hfc_fx_all_shadow_up
 *
 * Purpose:     issue shadow_up to all core
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (CHK-STOP)
 *               0 ended normally : 
 */
int hfc_fx_all_shadow_up(struct port_info *pp)
{
	int 				i=0, success_shadow_up = 0;
	struct core_info 	*core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_SHADOW_UP ];
	
	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	if (lock_fx_try_mailbox(pp) == 0) {
		HFC_DBGPRT("hfc_fx_all_shadow_up() - lock_fx_try_mailbox() NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x01, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );

		return -1;
	}

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_SHADOW_UP, 0x02, pp, NULL, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_all_shadow_up - Timer Start Fail \n"); 
			return -1;
		}
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;

		if( hfc_fx_check_cs_disable(pp, core) )
			continue;

		if (hfc_fx_issue_shadow_up(pp, core) != 0) {
			if (core->core_no == pp->master_core_no) {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x03, pp, NULL, NULL, NULL, NULL,
					0, 0, 0 );
				success_shadow_up = 0;
				break;
			} else {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x04, pp, NULL, NULL, NULL, NULL,
					0, 0, 0 );
				hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
			}
		} else {
			hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x05, pp, NULL, NULL, NULL, NULL,
				0, 0, 0 );
			success_shadow_up++;
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
//			if (pp->flags & HFC_IN_LDR_DUMP) {
//				break;
//			}
		}
	}
	
	if (success_shadow_up == 0) {
		HFC_DBGPRT("hfc_fx_all_shadow_up() - all core NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x06, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return -1;
	}

	set_bit(HFC_PD_WAIT_SHADOW_UP, (ulong *)&pp->status_detail2);
	clear_bit(HFC_PD_NEED_SHADOW_UP, (ulong *)&pp->status_detail2);

	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_SHADOW_UP, 0x10, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_issue_shadow_up
 *
 * Purpose:     Shadow up
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as core->mb_status
 *               Shadow_up is in progress : HFC_WAIT_SHADOW_UP
 *               Shadow_up ended normally : 
 *               Shadow_up failed         : HFC_WAIT_LINKUP
 */
int hfc_fx_issue_shadow_up( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_SHADOW_UP ];

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_shadow_up - entry \n"); 
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_shadow_up - core=NULL \n"); 
		return -1;
	}

	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x22, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x23, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x24, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x25, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	/* Start timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_SHADOW_UP, 0x26, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_SHADOWUP );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	if(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT){
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	core -> mb_retry_tid  = HFC_MBTIME_SHADOW_UP ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );

	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_SHADOW_UP, 0x30, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_shadowd_up - exit \n"); 

	return 0 ;
}


/*
 * Function:    hfc_fx_all_offline_mb
 *
 * Purpose:     issue offline mb for all core
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       
 *               -1  : NG (CHK-STOP)
 *               0 ended normally : 
 *               CORE_START failed         : HFC_WAIT_CORE_START
 */
int hfc_fx_all_offline_mb(struct port_info *pp)
{
	int i=0, success_offline_mb = 0;
	struct core_info * core = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_OFFLINE ];
	struct target_info_fx *target;
	
	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x00, pp, NULL, NULL, NULL, NULL,
					  0, 0, 0 );

	if (lock_fx_try_mailbox(pp) == 0) {
		HFC_DBGPRT("hfc_fx_all_offline_mb() - lock_fx_try_mailbox() NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x01, pp, NULL, NULL, NULL, NULL,
						  0, 0, 0 );
		return -1;
	}

	/* watchdog timer by not the number of the retry but retry continuation time */
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x02, pp, pp->region_arg[pp->rid], core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_all_offline_mb - Timer Start Fail \n"); 
			return -1;
		}
	}
//	else{	/* FCLNX-GPL-FX-059 */
//		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
//	}		/* FCLNX-GPL-FX-059 */

	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

	/* Clear all PD_NEED flags */
	HFC_CLEAR_PD_NEED(pp);
	HFC_DBGPRT("pp->status_detail1=%08x, pp->status_detail2=%08x\n",pp->status_detail1,pp->status_detail2);
	/* Clear all TS_NEED flags */
	for (i=0;i<pp->max_target;i++) {
		target = pp->target_arg[i];
		if (target != NULL){
			HFC_CLEAR_TS_NEED(target);
			HFC_DBGPRT("target->status=%08x\n",target->status);
		}
	}

	/* Issue Offline_mb for all core */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;

		if( hfc_fx_check_cs_disable(pp, core) )
			continue;

		if (hfc_fx_issue_offline_mb(pp, core) != 0) {
			if (core->core_no == pp->master_core_no) {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x03, pp, NULL, NULL, NULL, NULL,
								  0, 0, 0 );
				success_offline_mb = 0;
				break;
			} else {
				hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x04, pp, NULL, NULL, NULL, NULL,
								  0, 0, 0 );
				hfc_fx_core_chk_stop(pp, core, (uint)0x2f);
			}
			if( (!HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) )
				hfc_fx_chk_stop( pp );	/* FCLNX-GPL-FX-385 */
		} else {
			hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x05, pp, NULL, NULL, NULL, NULL,
							  0, 0, 0 );
			success_offline_mb++;
			set_bit(HFC_CS_WAIT_MAILBOX, (ulong *)&core->status);
//			if (pp->flags & HFC_IN_LDR_DUMP) {
//				break;
//			}
		}
	}
	
	if( core != NULL ){	/* FCLNX-GPL-FX-059 */
		if( mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT ){
			core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
		}
	}					/* FCLNX-GPL-FX-059 */
	
	if (success_offline_mb == 0) {
		HFC_DBGPRT("hfc_fx_all_offline_mb() - all core NG.\n");
		hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x06, pp, NULL, NULL, NULL, NULL,
						  0, 0, 0 );
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		hfc_fx_initialize_failed(pp, core, NULL);
		
		return -1;
	}

	/* Set WAIT_OFFLINE_MB, clear NEED_OFFLINE_MB and PS_ONLINE */
	set_bit(HFC_PD_WAIT_OFFLINE_MB, (ulong *)&pp->status_detail2);
	clear_bit(HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp->status_detail2);
	clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );

	hfc_fx_top_trace( HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x10, pp, NULL, NULL, NULL, NULL,
					  0, 0, 0 );
	return 0;
}


/*
 * Function:    hfc_fx_issue_offline_mb
 *
 * Purpose:     Issue OFFLINE_MB
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *	core	   - pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Start failure
 *
 * context : user
 *
 * Notes:       Return the following value as pp->mb_status
 *               Offline_mb is in progress : HFC_PD_WAIT_OFFLINE_MB
 *               Offline_mb failed         : HFC_PD_NEED_OFFLINE_MB
 */
int hfc_fx_issue_offline_mb( struct port_info *pp, struct core_info *core)
{
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_OFFLINE ];
	uint				sid=0;

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_offline_mb - entry\n");

	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x21, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_offline_mb - core=NULL \n"); 
		return -1 ;
	}
	
	if( test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status )
	||	test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x22, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x23, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}

	if ( test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x24, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1 ;
	}

	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x25, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x26, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_offline_mb - 1 \n");
	
	/* Start the timer of Offline_mb Mailbox */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR , mb_timer->tout  )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x27, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_offline_mb - 3 \n");

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_OFFLINEMB );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	hfc_fx_write_val( mb -> mb_init.type.offline_mb.vft_hdr.exrctl, 0x50);
	hfc_fx_write_val( mb -> mb_init.type.offline_mb.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);	
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( mb -> mb_init.type.offline_mb.fcph_hdr.s_id[0], (uchar)(sid >> 16));
    hfc_fx_write_val( mb -> mb_init.type.offline_mb.fcph_hdr.s_id[1], (uchar)(sid >> 8));
    hfc_fx_write_val( mb -> mb_init.type.offline_mb.fcph_hdr.s_id[2], (uchar)sid);
	hfc_fx_write_val( mb -> mb_init.type.offline_mb.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.offline_mb.self_wwnn, (pp -> node_name));

//	core -> mb_retry_cnt  = HFC_LINK_INIT_RETRY ;
	core -> mb_retry_tid  = HFC_MBTIME_OFFLINE ;
	core -> mb_retry_tout = 0 ; /* Default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_OFFLINE_MB, 0x30, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT(" hfcldd : hfcl_top; hfc_fx_issue_offline_mb - exit \n"); 

	return 0 ;
}


/*
 * Function:    hfc_fx_issue_flogi
 *
 * Purpose:     Send Fabric Login
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *	core	   - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock port_info before calling this function
 */
int hfc_fx_issue_flogi( struct port_info *pp )
{
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_FLOGI ];

	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_FLOGI, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_issue_flogi start \n");

	if( !(test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_PS_ISOL, (ulong *)&pp -> status ) ){	/* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x02, pp,  NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x03, pp,  NULL, NULL, NULL, NULL,
			0, 0, 0 );	
		return -1;
	}
	
	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x04, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x05, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock failed */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x06, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_FLOGI, 0x07, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_flogi - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

	/* Start the timer of Fabric login Mailbox */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout )) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FLOGI, 0x08, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	pp->flogi_retry_change = 0;				/* FCLNX-GPL-FX-179 */
	clear_bit(HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1);
	set_bit(HFC_PD_WAIT_FLOGI, (ulong *)&pp->status_detail1);

	/* Setup mailbox control block */
	if (HFC_FX_MIN_PORT_IN_REGION(pp)) {
		/* FLOGI */
		hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_FLOGI );
	}
	else {
		/* FDISC */
		hfc_fx_write_val( mb -> mb_init.mb_code, (HFC_MBCMD_FLOGI | 0x00000080) );
	}
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	hfc_fx_write_val( mb -> mb_init.region_no, pp->rid ); 
	
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.cs_ctl, 0);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.s_id[0], 0);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.s_id[1], 0);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.s_id[2], 0);
	
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fcph_hdr.d_id[2], 0xfe);

	hfc_fx_write_val( mb -> mb_init.type.flogi.frm_ctl, 0x00);
	hfc_fx_write_val( mb -> mb_init.type.flogi.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val( mb -> mb_init.type.flogi.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	hfc_fx_write_val( mb -> mb_init.type.flogi.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.flogi.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mb -> mb_init.type.flogi.flogi_param, pp -> flogi_param);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;	
	core -> mb_retry_tid  = HFC_MBTIME_FLOGI ;
	core -> mb_retry_tout = 0 ;

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_FLOGI, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;
	
}	


/*
 * Function:    hfc_fx_issue_plogi
 *
 * Purpose:     Send PLOGI
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock port_info before calling this function
 */
int hfc_fx_issue_plogi( struct port_info *pp, struct target_info_fx *target )
{
	uint sid=0, did=0;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_PLOGI ];
	uchar	id=0;
	
	HFC_ENTRY("hfc_fx_issue_plogi");
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_PLOGI, 0x00, pp, NULL, NULL, target, NULL,
		0, 0, 0 );
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start \n");

	if( !(test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x01, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_PS_ISOL, (ulong *)&pp -> status )  ){	/* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x02, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start 1\n");

	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x03, pp, NULL, NULL, target, NULL,
			0, 0, 0 );	
		return -1;
	}
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start 2\n");
	
	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x04, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		HFC_DBGPRT("hfc: hfc_fx_issue_plog lock failed \n");
		return -1;
	}
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start 3\n");

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x05, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock failed */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x06, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start 4\n");

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_PLOGI, 0x07, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_plog - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PLOGI, 0x08, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
//	HFC_DBGPRT("hfc: hfc_fx_issue_plog start 5\n");

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_PLOGI );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	if( test_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1) ){
		HFC_DBGPRT("hfc: hfc_fx_issue_plog start HFC_NEED_PLOGI_N\n");
		hfc_fx_write_val( mb -> mb_init.type.plogi.fc_class, HFC_FC_CLASS2);
		sid = ( uint )( pp -> scsi_id & 0x00ffffff );
		did = ( uint )( 0x00fffffc );
		
		clear_bit(HFC_PD_NEED_PLOGI_N, (ulong *)&pp->status_detail1);
		set_bit(HFC_PD_WAIT_PLOGI_N, (ulong *)&pp->status_detail1);
	}
	else{
		if( target != NULL ){
			if( test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status) ){
				HFC_DBGPRT("hfc: hfc_fx_issue_plog start HFC_NEED_PLOGI_T\n");
				hfc_fx_write_val( mb -> mb_init.type.plogi.fc_class, HFC_FC_CLASS3);
				sid = ( uint )( pp -> scsi_id & 0x00ffffff );
				did = ( uint )( target -> scsi_id & 0x00ffffff );
		
				clear_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				set_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status);
				pp->mailbox_pseq = target->pseq;
				target->login_seq_retry_cnt--;	/* FCLNX-GPL-FX-476 */
			}
		}
		else if( target == NULL ){
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_PLOGI, 0x09, pp, core->rp, core, target, NULL,
				0, 0, 0 );
			return -1;
		}
	}
	
	hfc_fx_write_val( mb -> mb_init.type.plogi.vft_hdr.exrctl, 0x50);
	
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.cs_ctl, 0);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.plogi.fcph_hdr.d_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.plogi.frame_ctl, 0);

	hfc_fx_write_val( mb -> mb_init.type.plogi.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.plogi.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mb -> mb_init.type.plogi.plogi_param, pp -> plogi_param);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_PLOGI ;
	core -> mb_retry_tout = 0 ;					/* default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_PLOGI, 0x10, pp, core->rp, core, target, NULL,
		0, 0, 0 );
	
	HFC_EXIT("hfc_fx_issue_plogi");
	
	return 0;
	
}	


/*
 * Function:    hfc_fx_issue_pdisc
 *
 * Purpose:     Send PDISC
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *  hfcp       - Pointer to hfc_pkt_fx
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock port_info before calling this function
 */
int hfc_fx_issue_pdisc( struct port_info *pp, struct target_info_fx *target )
{
	uint sid, did;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct mailbox_fx	*mb = NULL;
	uchar	id=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_PDISC ];


	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_PDISC, 0x00, pp, NULL, NULL, target, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_issue_pdisc start \n");

	if( !(test_bit( HFC_PS_ONLINE, (ulong *)&pp -> status ) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x01, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( test_bit( HFC_PS_ISOL, (ulong *)&pp -> status ) ){	/* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x02, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x03, pp, NULL, NULL, target, NULL,
			0, 0, 0 );	
		return -1;
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x04, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x05, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x06, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock failed */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x07, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_ISSUE_PDISC, 0x08, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_issue_pdisc - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_PDISC, 0x09, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	set_bit(HFC_TS_WAIT_PDISC, (ulong *)&target->status);	/*FCLNX-0506*/
	clear_bit( HFC_TS_NEED_PDISC, (ulong *)&target->status );
	pp->mailbox_pseq = target->pseq;

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_PDISC );
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );
//	hfc_fx_write_val( mb -> mb_init.type.pdisc.trans_s_id, sid);
	did = ( uint )( target -> scsi_id & 0x00ffffff );
	did |= (0x22 << 24);
//	hfc_fx_write_val( mb -> mb_init.type.pdisc.trans_d_id, did);

	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.cs_ctl, 0);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fcph_hdr.d_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.pdisc.frame_ctl, pp->frame_ctl);
	hfc_fx_write_val( mb -> mb_init.type.pdisc.fc_class, HFC_FC_CLASS3);	/* FCLNX-GPL-FX-037 */
	hfc_fx_write_val( mb -> mb_init.type.pdisc.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	hfc_fx_write_val( mb -> mb_init.type.pdisc.self_wwpn, (pp -> ww_name));
	hfc_fx_write_val( mb -> mb_init.type.pdisc.self_wwnn, (pp -> node_name));
	hfc_fx_write_val( mb -> mb_init.type.pdisc.plogi_param, pp -> plogi_param);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_PDISC ;
	core -> mb_retry_tout = 0 ;					/* default value */

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_PDISC, 0x10, pp, core->rp, core, target, NULL,
		0, 0, 0 );
	
	return 0;
	
}	


/*
 * Function:    hfc_fx_issue_frmsndrcv
 *
 * Purpose:     Send FRMSNDRCV
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  type	   - Command Type of FRMSNDRCV
 *  rctl       - RCTL Type of FRMSNDRCV
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_issue_frmsndrcv( struct port_info *pp,  
	struct target_info_fx *target, uint scsi_id, uchar rctl )
{
	struct mailbox_fx	*mb = NULL;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	uint				sid=0;
	uchar	id=0;
	uint64_t	receive_pyld_add=0;

	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x00, pp, NULL, NULL, target, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_issue_framsndrcv start \n");

	if( !(test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x01, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){ /* FCLNX-GPL-572 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x02, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1 ;
	}

	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x03, pp, NULL, NULL, target, NULL,
			0, 0, 0 );	
		return -1;
	}
	
	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x04, pp, rp, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x05, pp, rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock failed */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x06, pp, rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	/* Setup mailbox control block */
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_SNDRCV );
	sid = ( uint )( pp -> scsi_id & 0x00ffffff );

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.cs_ctl, 0);
	id = (uchar)(sid >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.s_id[0], id);
	id = (uchar)(sid >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.s_id[1], id);
	id = (uchar)sid;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.s_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload, 
			(uint64_t)core->phys_payload  );		/* Bus address of Send payload part  */
	
	receive_pyld_add = (uint64_t)(core->phys_payload+(uint)HFC_SEND_PAYLOADL_MAX);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.receive_payload, 
			(uint64_t)receive_pyld_add  );			/* Bus address of Receive payload part  */
	
	if(rctl != HFC_SNDRCV_GPN_FT){
		hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 
			0x800  );			/* Receive Payload Max Length   */
	}else{
		hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.receive_payload_max_length, 
			0x2000  );			/* Receive Payload Max Length   */		
	}

	switch ( rctl ) {
		/* Extended Link Service Frame sended with FRMSNDRCV */
		case HFC_SNDRCV_PRLI:				/* Frame_type : PRLI	*/
			hfc_fx_make_prli( pp, core, target );
			break;
		case HFC_SNDRCV_PRLO:				/* Frame_type : PRLO	*/
			hfc_fx_make_prlo( pp, core );
			break;
		case HFC_SNDRCV_SCR	:				/* Frame_type : SCR		*/
			hfc_fx_make_scr( pp, core );
			break;
		case HFC_SNDRCV_LOGO:				/* Frame_type : LOGO	*/
			hfc_fx_make_logo( pp, core, target);
			break;
		case HFC_SNDRCV_AUTH_REJECT:		/* Frame_type : AUTH_Reject	*/
//			hfc_fx_make_auth_reject( pp, core );
			break;
		case HFC_SNDRCV_AUTH_NEGO:			/* Frame_type : AUTH_Negitiate	*/
//			hfc_fx_make_auth_nego( pp, core );
			break;
		case HFC_SNDRCV_DHCHAP_CHALLENGE:	/* Frame_type : DHCHAP_Challeng	*/
//			hfc_fx_make_dhchap_challenge( pp, core );
			break;
		case HFC_SNDRCV_DHCHAP_REPLY:		/* Frame_type : DHCHAP_Reply	*/
//			hfc_fx_make_dhchap_reply( pp, core );
			break;
		case HFC_SNDRCV_DHCHAP_SUCCESS:		/* Frame_type : DHCHAP_Success	*/
//			hfc_fx_make_dhchap_success( pp, core );
			break;
		case HFC_SNDRCV_EVFP_SYNC:			/* Frame_type : EVFP_SYNC		*/
//			hfc_fx_make_evfp_sync( pp, core );
			break;
		case HFC_SNDRCV_EVFP_COMMIT:		/* Frame_type : EVFP_COMMIT		*/
//			hfc_fx_make_evfp_commit( pp, core );
			break;

		/* FC-GS Frame sended with FTMSNDRCV */
		case HFC_SNDRCV_GCS_ID:				/* Frame_type : GCS_ID	*/
			hfc_fx_make_gcs_id( pp, core );
			break;
		case HFC_SNDRCV_GID_PN:				/* Frame_type : GID_PN	*/
			hfc_fx_make_gid_pn( pp, core, target );
			break;
		case HFC_SNDRCV_GPN_ID:				/* Frame_type : GPN_ID	*/
			hfc_fx_make_gpn_id( pp, core, scsi_id );
			break;
		case HFC_SNDRCV_GID_FT:				/* Frame_type : GID_FT	*/
			hfc_fx_make_gid_ft( pp, core );
			break;
		case HFC_SNDRCV_RFT_ID:				/* Frame_type : RFT_ID	*/
			hfc_fx_make_rft_id( pp, core );
			break;
		case HFC_SNDRCV_RFF_ID:				/* Frame_type : RFF_ID	*/
			hfc_fx_make_rff_id( pp, core );
			break;
		case HFC_SNDRCV_GPN_FT:				/* Frame_type : GPN_FT	*/
			hfc_fx_make_gpn_ft( pp, core );
			break;

		default:
			break;
	}

//	HFC_DBGPRT("hfc_fx_issue_frmsndrcv mailbox dump\n");
//	structdump( 0xee, (uchar *)core->mb, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_issue_frmsndrcv mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, 4096 );

	/* Mailbox start */
	hfc_fx_mailbox_initiate(pp, core, HFC_MB_INTL);

	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_FRMSNDRCV, 0x10, pp, rp, core, target, NULL,
		0, 0, 0 );
	
	return 0;
	
}	


/*
 * Function:    hfc_fx_make_prli
 *
 * Purpose:     make payload of PRLI
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_prli( struct port_info *pp, struct core_info *core, struct target_info_fx *target )
{
	struct mailbox_fx	*mb = NULL;
	uint	did=0, id=0;
	struct payload_fx	*pyload=NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_PRLI];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_PRLI, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_prli start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLI, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	pyload = core -> payload;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLI, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( target == NULL ){
		HFC_DBGPRT("hfc: hfc_fx_make_prli  \n");
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLI, 0x03, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_PRLI, 0x04, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_prli - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_PRLI ;
	core -> mb_retry_tout = 0 ;					/* default value */
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLI, 0x05, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_PRLI_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS3);	/* FCLNX-GPL-FX-037 */
	did = ( uint )( target -> scsi_id & 0x00ffffff );
		
	clear_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
	set_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status);
	pp->mailbox_pseq = target->pseq;

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);

	/* Setup mailbox Send_Payload */
	if( pyload != NULL ){
		hfc_fx_write_val( pyload->send_payload.data0[0], HFC_PRLI_REQDATA0 );
		hfc_fx_write_val( pyload->send_payload.data0[1], 0x10 );
		hfc_fx_write_val( pyload->send_payload.data0[3], 0x14 );
		hfc_fx_write_val( pyload->send_payload.data0[4], 0x08 );
		hfc_fx_write_val( pyload->send_payload.data0[6], 0x20 );
		hfc_fx_write_val( pyload->send_payload.type.prli.prli_param_req, 0x000001a2 );
	}

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_PRLI, 0x10, pp, core->rp, core, target, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_prlo
 *
 * Purpose:     make payload of PRLO
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_prlo( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_PRLO];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_PRLO, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_prlo start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLO, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLO, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_PRLO, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_prlo - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_PRLO ;
	core -> mb_retry_tout = 0 ;					/* default value */
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_PRLO, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_PRLO_SLENGTH);

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	
	/* Setup mailbox Send_Payload */
	hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_PRLO_REQDATA0 );
	hfc_fx_write_val( core -> payload->send_payload.data0[1], 0x10 );
	hfc_fx_write_val( core -> payload->send_payload.data0[3], 0x14 );
	hfc_fx_write_val( core -> payload->send_payload.type.prlo.data1[0], 0x08 );

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_PRLO, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_scr
 *
 * Purpose:     make payload of SCR
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_scr( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	uint	did;
	uchar	id=0;
	struct payload_fx	*pyload=NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_SCR ];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_SCR, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_scr start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_SCR, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_SCR, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_SCR, 0x03, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_scr - Timer Start Fail \n"); 
		return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_SCR ;
	core -> mb_retry_tout = 0 ;					/* default value */

	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_SCR, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	set_bit(HFC_PD_WAIT_SCR, (ulong *)&pp->status_detail1);
	clear_bit(HFC_PD_NEED_SCR, (ulong *)&pp->status_detail1);
	did = 0x00fffffd;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_SCR_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	
	/* Setup mailbox Send_Payload */
	pyload = core -> payload;	/* TBD */
	if( pyload != NULL ){
		hfc_fx_write_val( pyload->send_payload.data0[0], HFC_SCR_REQDATA0 );
		hfc_fx_write_val( pyload->send_payload.data0[7], 0x03 );
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_SCR, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );

	return 0;

}


/*
 * Function:    hfc_fx_make_logo
 *
 * Purpose:     make payload of LOGO
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_logo( struct port_info *pp, struct core_info *core, struct target_info_fx *target)
{
	struct mailbox_fx	*mb = NULL;
	uint	nport_id=0;
	uint sid=0, did=0, id=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[ HFC_MBTIME_LOGO ];
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_LOGO, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_logo start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_LOGO, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_LOGO, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_LOGO, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_logo - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_LOGO ;
	core -> mb_retry_tout = 0 ;					/* default value */

	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_LOGO, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_LOGO_SLENGTH);

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_ELS);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	if(test_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1)){	
		HFC_DBGPRT("hfc: hfc_fx_make_logo start HFC_PD_NEED_LOGO_FCSW\n");
		hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
		sid = ( uint )( pp -> scsi_id & 0x00ffffff );
		did = ( uint )( 0x00fffffe );
		
		clear_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);
		set_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1);
	}
	else{
		if( target != NULL ){
			if( test_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status) ){
				HFC_DBGPRT("hfc: hfc_fx_make_logo HFC_NEED_LOGO_T\n");
				hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS3);
				sid = ( uint )( pp -> scsi_id & 0x00ffffff );
				did = ( uint )( target -> scsi_id & 0x00ffffff );
				
				clear_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);
				set_bit(HFC_TS_WAIT_LOGO_TGT, (ulong *)&target->status);
				
				pp->mailbox_pseq = target->pseq;
			}
		}
	}
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */

	/* Setup mailbox Send_Payload */
	hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_LOGO_REQDATA0 );	/* TBD */
	nport_id = ( uint )( pp -> scsi_id & 0x00ffffff );
	id = 0;
	id = (uchar)(nport_id >> 16 );											/* FCLNX-GPL-FX-069 */
	hfc_fx_write_val( core -> payload->send_payload.data0[5], id);			/* FCLNX-GPL-FX-069 */
	id = (uchar)(nport_id >> 8 );											/* FCLNX-GPL-FX-069 */
	hfc_fx_write_val( core -> payload->send_payload.data0[6], id);			/* FCLNX-GPL-FX-069 */
	id = (uchar)nport_id;													/* FCLNX-GPL-FX-069 */
	hfc_fx_write_val( core -> payload->send_payload.data0[7], id);			/* FCLNX-GPL-FX-069 */
	hfc_fx_write_val( core -> payload->send_payload.type.logo.nport_name, pp->node_name );
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_LOGO, 0x10, pp, core->rp, core, target, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_gcs_id
 *
 * Purpose:     make payload of GCS_ID
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_gcs_id( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	uint	port_id=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_GCS_ID];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GCS_ID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_gcs_id start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GCS_ID, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GCS_ID, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_GCS_ID, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_gcs_id - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);

	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_GCS_ID ;
	core -> mb_retry_tout = 0 ;					/* default value */	

	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GCS_ID, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_GCSID_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);	/* FCLNX-GPL-FX-037 */
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */

	/* Setup mailbox Send_Payload */
	hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[0], 0xfc);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[1], 0x02);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[4], 0x01);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[5], 0x14);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[7], 0x01);
	port_id = ( uint )( pp -> scsi_id & 0x00ffffff );
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.gcs_id.port_id, port_id );

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GCS_ID, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_gid_pn
 *
 * Purpose:     make payload of GID_PN
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_gid_pn( struct port_info *pp, struct core_info *core, struct target_info_fx *target )
{
	struct mailbox_fx	*mb = NULL;
	uint64_t	port_name=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_GID_PN];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GID_PN, 0x00, pp, NULL, NULL, target, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_gid_pn start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_PN, 0x01, pp, NULL, NULL, target, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_PN, 0x02, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_GID_PN, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_gid_pn - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_GID_PN ;
	core -> mb_retry_tout = 0 ;					/* default value */	

	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_PN, 0x04, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	pp->mailbox_pseq = target -> pseq;

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_GIDPN_SLENGTH);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);

	/* Setup mailbox Send_Payload */
	hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( core -> payload->send_payload.data0[4], 0xfc);
	hfc_fx_write_val( core -> payload->send_payload.data0[5], 0x02);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[0], 0x01);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[1], 0x21);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[2], 0x01);
	
	port_name = ( uint64_t )target -> ww_name;
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.gid_pn.nport_name, port_name );
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GID_PN, 0x10, pp, core->rp, core, target, NULL,
		0, 0, 0 );
			
	return 0;

}


/*
 * Function:    hfc_fx_make_gpn_id
 *
 * Purpose:     make payload of GPN_ID
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_gpn_id( struct port_info *pp, struct core_info *core, uint scsi_id )
{
	struct mailbox_fx	*mb = NULL;
	uint	port_id=0;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_GPN_ID];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GPN_ID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_gpn_id start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_ID, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_ID, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_GPN_ID, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_gpn_id - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_GPN_ID ;
	core -> mb_retry_tout = 0 ;					/* default value */	

	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_ID, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_SNDRCV_GIDFT );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_GPNID_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);

	/* Setup mailbox Send_Payload */
	hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_GXX_REQDATA0);
	hfc_fx_write_val( core -> payload->send_payload.data0[4], 0xfc);
	hfc_fx_write_val( core -> payload->send_payload.data0[5], 0x02);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[0], 0x01);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[1], 0x12);
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[2], 0x01);
	port_id = ( uint )( scsi_id & 0x00ffffff );
	hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.gpn_id.port_id, port_id );

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GPN_ID, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_gid_ft
 *
 * Purpose:     make payload of GID_FT
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_gid_ft( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct payload_fx	*pyload = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_GID_FT];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GID_FT, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_gid_ft start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_FT, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	pyload = core->payload;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_FT, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_GID_FT, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_gid_ft - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_GID_FT ;
	core -> mb_retry_tout = 0 ;					/* default value */	
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GID_FT, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	set_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2);
	clear_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
	
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_SNDRCV_GIDFT );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_GIDFT_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);

	/* Setup mailbox Send_Payload */
	if( pyload != NULL ){
		hfc_fx_write_val( pyload->send_payload.data0[0], HFC_GXX_REQDATA0);
		hfc_fx_write_val( pyload->send_payload.data0[4], 0xfc);
		hfc_fx_write_val( pyload->send_payload.data0[5], 0x02);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[0], 0x01);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[1], 0x71);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[2], 0x01);
		hfc_fx_write_val( pyload->send_payload.type.gxx.sub_type.gid_ft.data2[3], 0x08);
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GID_FT, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_rft_id
 *
 * Purpose:     make payload of RFT_ID
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_rft_id( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	uint	port_id=0, did;
	uchar	id=0;
	struct payload_fx	*pyload=NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_RFT_ID];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_RFT_ID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_rft_id start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFT_ID, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	pyload = core -> payload;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFT_ID, 0x02, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_RFT_ID, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_rft_id - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_RFT_ID ;
	core -> mb_retry_tout = 0 ;					/* default value */	
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFT_ID, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	clear_bit(HFC_PD_NEED_RFTID, (ulong *)&pp->status_detail1);
	set_bit(HFC_PD_WAIT_RFTID, (ulong *)&pp->status_detail1);
	did = ( uint )( 0x00fffffc );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_RFTID_SLENGTH);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	/* Setup mailbox Send_Payload */
	if( pyload != NULL ){
		hfc_fx_write_val( pyload->send_payload.data0[0], HFC_GXX_REQDATA0);
		hfc_fx_write_val( pyload->send_payload.data0[4], 0xfc);
		hfc_fx_write_val( pyload->send_payload.data0[5], 0x02);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[0], 0x02);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[1], 0x17);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[3], 0x01);
		port_id = ( uint )( pp -> scsi_id & 0x00ffffff );
		hfc_fx_write_val( pyload->send_payload.type.gxx.sub_type.rft_id.port_identifier, port_id );

		hfc_fx_write_val( pyload->send_payload.type.gxx.sub_type.rft_id.data2[2], 0x01);	/* payload +0x16 */
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_RFT_ID, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_rff_id
 *
 * Purpose:     make payload of RFF_ID
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_rff_id( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	uint	port_id=0, did;
	uchar	id=0;
	struct payload_fx	*pyload=NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_RFF_ID];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_RFF_ID, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_rff_id start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFF_ID, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	pyload = core -> payload;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFF_ID, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_RFF_ID, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_rff_id - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_RFF_ID ;
	core -> mb_retry_tout = 0 ;					/* default value */	
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_RFF_ID, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_RFFID_SLENGTH);

	clear_bit(HFC_PD_NEED_RFFID, (ulong *)&pp->status_detail1);
	set_bit(HFC_PD_WAIT_RFFID, (ulong *)&pp->status_detail1);
	did = ( uint )( 0x00fffffc );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	id = (uchar)(did >> 16 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], id);
	id = (uchar)(did >> 8 );
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], id);
	id = (uchar)did;
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], id);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS2);
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 

	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);	/* FCLNX-GPL-FX-222 */
	
	/* Setup mailbox Send_Payload */
	if( pyload != NULL ){
		hfc_fx_write_val( core -> payload->send_payload.data0[0], HFC_GXX_REQDATA0);
		hfc_fx_write_val( core -> payload->send_payload.data0[4], 0xfc);
		hfc_fx_write_val( core -> payload->send_payload.data0[5], 0x02);
		hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[0], 0x02);
		hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[1], 0x1f);
		hfc_fx_write_val( core -> payload->send_payload.type.gxx.data1[3], 0x01);
		port_id = ( uint )( pp -> scsi_id & 0x00ffffff );
		hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.rff_id.port_identifier, port_id );

		hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.rff_id.data2[2], 0x02);	/* payload +0x16 */
		hfc_fx_write_val( core -> payload->send_payload.type.gxx.sub_type.rff_id.data2[3], 0x08);	/* payload +0x17 */
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_RFF_ID, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_make_gpn_ft
 *
 * Purpose:     make payload of GPN_FT
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_make_gpn_ft( struct port_info *pp, struct core_info *core )
{
	struct mailbox_fx	*mb = NULL;
	struct payload_fx	*pyload = NULL;
	struct mb_timer_t	*mb_timer = &pp->mb_timer[HFC_MBTIME_GPN_FT];

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GPN_FT, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_make_gpn_ft start \n");
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_FT, 0x01, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	mb = core->mb;
	pyload = core->payload;
	
	if( mb == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_FT, 0x02, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!(mb_timer->retry & HFC_FX_MBRTY_POLICY_CNT) ){
		hfc_fx_w_stop(pp, NULL, HFC_FX_MB_RETRY_TMR);
		if ((hfc_fx_w_start( pp, core, HFC_FX_MB_RETRY_TMR, (mb_timer->retry & HFC_FX_MBRTY_VAL_MASK))) )
		{
			/* Timer ID is already registered or invalid */
	   	 	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB5, NULL, 0) ;

			unlock_fx_mailbox ( pp );
			hfc_fx_top_trace(
				HFC_FX_TRC_MAKE_GPN_FT, 0x03, pp, core->rp, core, NULL, NULL,
				0, 0, 0 );
			HFC_DBGPRT(" hfcldd : hfc_fx_top 0; hfc_fx_make_gpn_ft - Timer Start Fail \n"); 
			return -1;
		}
	}else{
		core -> mb_retry_cnt  = mb_timer->retry & HFC_FX_MBRTY_VAL_MASK ;
	}
	
	set_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
	
	/* Setup callback information and retry count */
//	core -> mb_retry_cnt  = pp->els_retry ;		/* FCLNX-0523 */
	core -> mb_retry_tid  = HFC_MBTIME_GPN_FT ;
	core -> mb_retry_tout = 0 ;					/* default value */
	
	/* Set and start watchdog timer */
	hfc_fx_w_stop( pp, core, HFC_FX_MB_RSP_TMR );
	if ( (hfc_fx_w_start( pp, core, HFC_FX_MB_RSP_TMR, mb_timer->tout ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBA, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_MAKE_GPN_FT, 0x04, pp, core->rp, core, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	memset( &core -> payload->send_payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX );
	set_bit(HFC_PD_WAIT_GPNFT, (ulong *)&pp->status_detail2);
	clear_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
	
	hfc_fx_write_val( mb -> mb_init.mb_code, HFC_MBCMD_SNDRCV );
	/* Setup Valid Length of Mailbox Send_Payload  */
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.payload_length, HFC_GPNFT_SLENGTH);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.rctl, HFC_FRMSNDRCV_FCGS);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[0], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[1], 0xff);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fcph_hdr.d_id[2], 0xfc);
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.fc_class, HFC_FC_CLASS3);	/* FCLNX-GPL-FX-074 */
	hfc_fx_write_val( mb -> mb_init.timer, mb_timer->tout-1 ); 
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.frame_ctl, 0);
	
	hfc_fx_write_val( mb -> mb_init.type.frmsndrcv.vft_hdr.exrctl, 0x50);

	/* Setup mailbox Send_Payload */
	if( pyload != NULL ){
		hfc_fx_write_val( pyload->send_payload.data0[0], HFC_GXX_REQDATA0);
		hfc_fx_write_val( pyload->send_payload.data0[4], 0xfc);
		hfc_fx_write_val( pyload->send_payload.data0[5], 0x02);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[0], 0x01);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[1], 0x72);
		hfc_fx_write_val( pyload->send_payload.type.gxx.data1[2], 0x08);
		hfc_fx_write_val( pyload->send_payload.type.gxx.sub_type.gid_ft.data2[3], 0x08);
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_GPN_FT, 0x10, pp, core->rp, core, NULL, NULL,
		0, 0, 0 );
	
	return 0;

}


/*
 * Function:    hfc_fx_issue_change_state
 *
 * Purpose:     make payload of CHANGE_STATE
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  core       - Pointer to core_info
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Adapter is not online, or failed to lock maiobox
 *
 * Notes:        Lock mb_lock before calling this function
 */
int hfc_fx_issue_change_state( struct port_info *pp, uchar state )
{
	struct core_info	*core = NULL;
	uint				i;
	uint				frame_a;

	hfc_fx_top_trace(
		HFC_FX_TRC_MAKE_CHANGE_STATE, 0x00, pp, NULL, NULL, NULL, NULL,
		0, 0, 0 );
	
	HFC_DBGPRT("hfc: hfc_fx_issue_change_state start \n");
	
	if (HFC_FX_MQ_VIRTUAL_PORT(pp))		/* FCLNX-GPL-206 */
		hfc_fx_mq_copy_iocinfo(pp);{
	}
	
	frame_a = HFC_FX_FRAMEA_CHANGE_STATE;
	frame_a |= (uint)(pp->rid << 16 );
	frame_a |= (uint)(state << 8);
	if(pp->region_arg[pp->rid] != NULL){
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			core = NULL;
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				core = pp->region_arg[pp->rid]->core_arg[i];
				if (core == NULL){
					hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
				}
				else{
					if( !hfc_fx_check_cs_disable(pp, core) ){	/* FCLNX-GPL-FX-438 */
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
							(char)0x4, (int)frame_a, HFC_FX_CORE_OFFSET40);
						HFC_DBGPRT("hfc: hfc_fx_issue_change_state frame_a = %08x\n",frame_a);
					}
				}
			}
		}
	}
	
//	clear_bit(HFC_NEED_CHANGE_STATE, (ulong *)&pp->status1);

	return 0;

}

int hfc_fx_issue_relogin(struct port_info *pp, struct target_info_fx *target){
	return 0;
}

/*
 * Function:    hfc_fx_issue_prli
 *
 * Purpose:     Reissue plogi request
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *
 * Returns:     
 *  = 0        - Normal end
 *  !=0        - Target is not online, or failed to acquire lock
 *
 * Notes:       
 */
int hfc_fx_issue_prli( struct port_info *pp, struct target_info_fx *target )
{
//	uint did;
	uint hash;
//	struct mailbox_fx	*mb = NULL;
	struct core_info	*core = NULL;
	struct region_info	*rp = NULL;
	struct hfc_pkt_fx	*hfcp = NULL;

	rp = pp->region_arg[pp->rid];
	if( rp != NULL ){
		core = rp->core_arg[ pp->master_core_no ];
	}
	
	if( core == NULL ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x86, pp, NULL, NULL, NULL, NULL,
			0, 0, 0 );
		return -1;
	}
	
	hfc_fx_top_trace(
		HFC_FX_TRC_ISSUE_PRLI, 0x00, pp, core->rp, core, target, NULL,
		0, 0, 0 );	
	
	HFC_DBGPRT("hfcldd%d: hfc_fx_issue_relogin start.\n", pp->dev_minor);
	
	if( !(test_bit( HFC_PS_ONLINE, (ulong *)&pp -> status ) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x81, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if(!hfc_fx_mlpf_check_normal_hypsts(pp)){	/* FCLNX-GPL-428 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x89, pp, core->rp, core, target, NULL,
			0, 0, 0 );	
		return -1;
	}

	if( (test_bit( HFC_PS_WAIT_LINKUP, (ulong *)&pp -> status ) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x85, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	if( (test_bit( HFC_TS_SCN_WLINKUP, (ulong *)&target -> status ) ) 
	&& !(test_bit( HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target -> status ) ) ){	/* FCLNX-GPL-FX-014 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x86, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
	
	if( (test_bit( HFC_TS_WAIT_PLOGI, (ulong *)&target -> status ) ) ){
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x82, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1 ;
	}

	if( (test_bit( HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target -> status ) ) ){			/* FCLNX-GPL-038 */	/* FCLNX-GPL-FX-014 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x88, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1 ;
	}																			/* FCLNX-GPL-038 */

#if 0
	if ( !(lock_fx_try_mailbox( pp )) ) {			/* Mailbox lock fail */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x83, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}

	/* Start watchdog timer */
	if ( ( hfc_fx_watchdog_enter( pp,target,NULL, 0,HFC_FX_MB_RSP_TMR,0,FALSE ) ) )
	{
		/* Timer ID is already registered or invalud */
    	hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xB8, NULL, 0) ;
		unlock_fx_mailbox ( pp );
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x84, pp, core->rp, core, target, NULL,
			0, 0, 0 );
		return -1;
	}
#endif

	hfc_fx_deque_prli_req( pp, target );

#if 0
	/* Cancel XOB initiation of this target */
	if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)
		&& test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) )
	{
		hfc_fx_cancel_xob( pp, core, target, 0, NULL, HFC_FLASH_TARGET);		/* FCLNX-0095 */
		hfc_fx_top_trace(
			HFC_FX_TRC_ISSUE_RELOGIN, 0x87, pp, core->rp, core, target, NULL,
			0, 0, 0 );
	}																/* FCLNX-GPL-367 *//* FCLNX-GPL-380 */

	/* Setup mailbox control block */
	hfc_fx_write_val( core -> mb -> mb_init.mb_code, HFC_MBCMD_SNDRCV );

	pp->login_type = HFC_SCAN_DEVICE;	/* FCLNX-0571 */

	if( (hfc_fx_read_val( core->fw_init_p->func) & HFC_FWF_EXTPLOGI)
		&& (test_bit(HFC_TS_NEED_CANCEL_SCSI_WITHOUT_DMA, (ulong *)&target->status)) ){
		hfc_fx_write_val(core->mb->mb_init.dependent_code, 0x8000 );
		pp->login_type = HFC_CANCEL_LOGIN;	/* FCLNX-0571 */
	}


//	hfc_fx_write_val( core -> mb -> mb_init.pseq_no, target -> pseq );
	did = ( uint )( target -> scsi_id & 0x00ffffff );

	if ( pp -> connect_type == HFC_FX_AL )
		did |= ( target -> scsi_id & 0x00ff ) << 24;

	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.adr.d_id, did);
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.payload_length, 20);
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.pagelen, 16);
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.payload_length, 20);
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.fscsi, 0x08);
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.prli, 0x20);
	/* Set FCP read transfer ready and SCSI Initiator */
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.parameter, 
	(0x0002 |
	 0x0020 |
	 0x0080 |
	 0x0100));

	/* Establish an image pair. */
	hfc_fx_write_val( core -> mb -> mb_init.type.drvioctl1.un.login.flag, (0x20<<8));

	/*
	 * Turn on mailbox flag if HFC_NEED_CANCE is set
	 */
#endif
	/* Set target status to LOGIN wait and cancel XOB initiation of all devices under this target */
	set_bit( HFC_TS_WAIT_PRLI, (ulong *)&target -> status );
	clear_bit( HFC_TS_NEED_PRLI, (ulong *)&target -> status );

	/* Setup callback information and retry count */	
	core -> mb_retry_cnt  = pp->login_retry ;         /* FCLNX-0545 */
	if( test_bit( HFC_TS_WAIT_PLOGI, (ulong *)&target -> status ) ){			/* FCLNX-GPL-389 */
		for (hash=0;hash<HASH_T_NUM;hash++)	/* FCLNX-0579 */
		{
			if (target->core_queue[core->core_no].we_que_top[hash] != NULL)
			{	/* hfcp exists in queue */
				hfcp = target->core_queue[core->core_no].we_que_top[hash];
				
				while( hfcp != NULL ){
					if((test_bit(CFLAG_TIMEOUT, (ulong *)&hfcp->cmd_flags))&&(!( hfcp->cmd_flags & CFLAG_RESET_ANY ))){	/* FCLNX-GPL-FX-091 */
						core->mb_retry_cnt = pp->to_reset_retry;				/* FCLNX-GPL-452 */
						
						if(!hfc_manage_info.hfcldd_mp_mod && pp->to_reset_retry)/* FCLNX-GPL-452 */
							core->mb_retry_cnt--;								/* FCLNX-GPL-452 */
						
						pp->login_type = HFC_CANCEL_IO;	/* FCLNX-0571 */
					}
					
					hfcp = hfcp->cmd_forw ;
				}
			}
		}	/* FCLNX-0579 */
	}																		/* FCLNX-GPL-389 */


	core -> mb_retry_tid  = HFC_MBTIME_PRLI ;
	core -> mb_retry_tout = 0 ;					/* Default value */

	hfc_fx_mailbox_initiate( pp, core, HFC_MB_INTL );	/* Mailbox start */
	return 0;
}


/*
 * Function:    hfc_fx_send_gidpn
 *
 * Purpose:     Search target to issue GID_PN and initiate GID_PN
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *   0         - Initiated GIDPN
 *  !0         - Failed to initiate GIDPN
 *
 * Notes:       
 */
int hfc_fx_send_gidpn( struct port_info *pp, struct core_info *core )
{
	int	rtn=-1;
#if 0
	struct target_info_fx	*target=NULL;
	int	i;

	/* Search target to issue GPN_ID */
	for (i=0 ; i<(int) pp->max_target ; i++) {
		target = pp->target_arg[i] ;
		if ( (target = hfc_fx_pseq_target_info_fx(pp,i)) != NULL )
//			if( test_bit( HFC_T_NEED_GIDPN, (ulong *)&target->status ) )
				break ;
	}

	if( target != NULL ) {
		rtn = hfc_fx_issue_frmsndrcv( pp, target, 0, HFC_SNDRCV_GID_PN );
//		rtn = hfc_fx_issue_gidpn(pp,target);
		pp -> next_gidpn = TRUE;
	}
	else
		pp -> next_gidpn = FALSE;
#endif
	return rtn;
}


/*
 * Function:    hfc_fx_send_gpnid
 *
 * Purpose:     Search target to issue GPN_ID and initiate GPN_ID
 *
 * Arguments:   
 *  pp         - Pointer to port_info
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
int hfc_fx_send_gpnid(struct port_info *pp, struct core_info *core )
{
#if 0
	int i;

	for (i=0;i<MAX_TARGET_PROBE;i++) {

		if ( pp->target_scan[i].flags & HFC_SCAN_NEED ) {
			if( hfc_fx_issue_frmsndrcv( pp, NULL, pp->target_scan[i].scsi_id, HFC_SNDRCV_GPN_ID ) )
//			if ( hfc_fx_issue_gpnid( pp, pp->target_scan[i].scsi_id ) )
				return (-1);

			/* Mailbox start success */
			pp->target_scan[i].flags &= ~HFC_SCAN_NEED;
			pp->target_scan[i].flags |= HFC_SCAN_WAIT;
			return (1);
		}
	}
#endif
	return(0);
}


/*
 * Function:    hfc_fx_send_plogi
 *
 * Purpose:     Initiate PLOGI to the target which is in PLOGI initiation wait status (HFC_NEED_PLOGI_T)
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *                 (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * Notes:       Dequeue target_info_fx from queue and handle next target_info_fx if the target state
 *              of target_info_fx is in PLOGI initiation wait status (HFC_NEED_PLOGI)
 *
 */
int hfc_fx_send_plogi(struct port_info *pp, struct core_info *core)
{
	struct target_info_fx *target=NULL;

//	target = pp -> plogi_target;	/* TBD */

	while ( target != NULL ) {
		if( test_bit( HFC_TS_NEED_PLOGI, (ulong *)&target->status ) ){
			if ( hfc_fx_issue_plogi( pp, target ) )
				return (-1);						/* Failed to start mailbox */

			return (1);								/* Succeeded to start mailbox */
		}

		/* Target state is HFC_NEED_PLOGI. Dequeue the target from LOGIN queue */
		hfc_fx_deque_plogi_req( pp, target );
//		target = pp -> plogi_target;	/* TBD */
	}
 
	return(0); /* Failed to initiate mailbox */
}


/*
 * Function:    hfc_fx_send_prli
 *
 * Purpose:     Initiate PRLI to the target which is in PRLI initiation wait status (HFC_NEED_PRLI)
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *                 (Satus is not ONLINE, Mailbox lock failure and timer registration failure)
 *
 * Notes:       Dequeue target_info_fx from queue and handle next target_info_fx if the target state
 *              of target_info_fx is in PRLI initiation wait status (HFC_NEED_PRLI)
 *
 */
int hfc_fx_send_prli(struct port_info *pp, struct core_info *core)
{
	struct target_info_fx *target=NULL;

//	target = pp -> prli_target;		/* TBD */
	
	HFC_DBGPRT("hfc_fx_send_prli start\n");
	
	while ( target != NULL ) {
		if( test_bit( HFC_TS_NEED_PRLI, (ulong *)&target->status ) ){
			if( hfc_fx_issue_frmsndrcv( pp, target, 0, HFC_SNDRCV_PRLI ) )
//			if ( hfc_fx_issue_prli( pp, target, core ) )
				return (-1);						/* Failed to start mailbox */

			return (1);								/* Succeeded to start mailbox */
		}

		/* Target state is HFC_NEED_PRLI. Dequeue the target from PRLI queue */
		hfc_fx_deque_prli_req( pp, target );
//		target = pp -> prli_target;		/* TBD */
	}
 
	return(0); /* Failed to initiate mailbox */
}


/*
 * Function:    hfc_fx_send_pdisc
 *
 * Purpose:     Issue PDISC to the target which is in PDISC initiation wait status (HFC_NEED_PDISC)
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *
 * Returns:     
 *  = 1        - Succeeded to initiate mailbox
 *  = 0        - Failed to initiate mailbox
 *  = -1       - Unable to initiate mailbox
 *
 * context:     DIRQL
 *
 * Notes:       Dequeue target_info_fx from queue and handle next target_info_fx if the state
 *              of target_info_fx is in PDISC initiation wait status (HFC_NEED_PDISC)
 *
 */
int hfc_fx_send_pdisc(struct port_info *pp, struct core_info *core)
{
	struct target_info_fx *target;

	target = pp -> next_tstart;

	while ( target != NULL ) {
		if( test_bit( HFC_TS_NEED_PDISC, (ulong *)&target->status ) ){
			if ( hfc_fx_issue_pdisc( pp, target ) )
				return (-1);						/* Failed to start mailbox */

			hfc_fx_deque_pdisc_req( pp, target );
			return (1);								/* Succeeded to start mailbox */
		}

		/* Target state is HFC_NEED_PDISC. Dequeue the target from PDISC queue */
		hfc_fx_deque_pdisc_req( pp, target );
		target = pp -> next_tstart;
	}

	return(0);										/* There is no MailBox start */
}


/*
 * Function:    hfc_fx_enque_plogi_req
 *
 * Purpose:     Enqueue target_info_fx to PLOGI queue if PLOGI is unable to initiate.
 *
 * Arguments:   
 *  pp          - Pointer to port_info
 *  target      - Pointer to target_info_fx
 *
 * Returns:     -
 *
 * Notes:       Do nothing if the specified target_info_fx has already existed in 
 *              PLOGI queue. 
 *              
 */
void hfc_fx_enque_plogi_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt=NULL;

//	wk_tgt = pp->plogi_target;	/* TBD */
	if (wk_tgt == NULL) {
		HFC_DBGPRT("hfc_fx_enque_plogi_req wk_tgt == NULL\n");
//		pp->plogi_target = target;					/* Move to the top of the queue */	/* TBD */
		target->plogi_next = NULL;
	}
	else {
		int hit=1;
		HFC_DBGPRT("hfc_fx_enque_plogi_req wk_tgt != NULL\n");

		while (wk_tgt != target) {					/* Does this target exist in queue? */
			if (wk_tgt->plogi_next == NULL) {
				hit = 0;							/* All targets are unmatched */
				break;
			}
			wk_tgt = wk_tgt->plogi_next;
		}

		if (!hit) {								
			wk_tgt->plogi_next = target;			/* Add new target to the end of the queue */
			target->plogi_next = NULL;
		}
	}
}


/*
 * Function:    hfc_fx_deque_plogi_req
 *
 * Purpose:     Dequeue target_info_fx from PLOGI queue.
 *              All targets are removed from queue when no pointers are specified 
 *              as arguments, pp and target.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_deque_plogi_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt=NULL,*pr_tgt;

	pr_tgt = NULL;
//	wk_tgt = pp->plogi_target;	/* TBD */
	while (wk_tgt != NULL) {
		if (target == NULL) {									
//			pp->plogi_target = NULL;				/* Dequeue all targets */	/* TBD */
			pr_tgt = wk_tgt->plogi_next;
			wk_tgt->plogi_next = NULL;
			wk_tgt = pr_tgt;
		}
		else {
			if (wk_tgt == target) {					/* Find target? */
#if 0	/* TBD */
				if (pr_tgt == NULL)					/* Queue top?   */
					pp->plogi_target   = wk_tgt->plogi_next;
				else
					pr_tgt->plogi_next = wk_tgt->plogi_next;
#endif

				wk_tgt->plogi_next=NULL;
				break;								/* return */
			}

			pr_tgt = wk_tgt;					
			wk_tgt = wk_tgt->plogi_next;			/* Go to next target */
		}
	}
}


/*
 * Function:    hfc_fx_enque_prli_req
 *
 * Purpose:     Enqueue target_info_fx to PRLI queue if PRLI is unable to initiate.
 *
 * Arguments:   
 *  pp          - Pointer to port_info
 *  target      - Pointer to target_info_fx
 *
 * Returns:     -
 *
 * Notes:       Do nothing if the specified target_info_fx has already existed in 
 *              LOGIN queue. 
 *              
 */
void hfc_fx_enque_prli_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt=NULL;

//	wk_tgt = pp->prli_target;		/* TBD */
	if (wk_tgt == NULL) {
		HFC_DBGPRT("hfc_fx_enque_prli_req wk_tgt == NULL\n");
//		pp->prli_target = target;					/* Move to the top of the queue */		/* TBD */
		target->prli_next = NULL;
	}
	else {
		int hit=1;
		HFC_DBGPRT("hfc_fx_enque_prli_req wk_tgt != NULL\n");

		while (wk_tgt != target) {					/* Does this target exist in queue? */
			if (wk_tgt->prli_next == NULL) {
				hit = 0;							/* All targets are unmatched */
				break;
			}
			wk_tgt = wk_tgt->prli_next;
		}

		if (!hit) {								
			wk_tgt->prli_next = target;				/* Add new target to the end of the queue */
			target->prli_next = NULL;
		}
	}
}


/*
 * Function:    hfc_fx_deque_prli_req
 *
 * Purpose:     Dequeue target_info_fx from PRLI queue.
 *              All targets are removed from queue when no pointers are specified 
 *              as arguments, pp and target.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_deque_prli_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt=NULL,*pr_tgt;
	
	HFC_DBGPRT("hfc_fx_deque_prli_req start\n");

	pr_tgt = NULL;
//	wk_tgt = pp->prli_target;		/* TBD */
	while (wk_tgt != NULL) {
		if (target == NULL) {									
//			pp->prli_target = NULL;				/* Dequeue all targets */		/* TBD */
			pr_tgt = wk_tgt->prli_next;
			wk_tgt->prli_next = NULL;
			wk_tgt = pr_tgt;
		}
		else {
			if (wk_tgt == target) {					/* Find target? */
#if 0	/* TBD */
				if (pr_tgt == NULL)					/* Queue top?   */
					pp->prli_target   = wk_tgt->prli_next;
				else
					pr_tgt->prli_next = wk_tgt->prli_next;
#endif

				wk_tgt->prli_next=NULL;
				break;								/* return */
			}

			pr_tgt = wk_tgt;					
			wk_tgt = wk_tgt->prli_next;			/* Go to next target */
		}
	}
}


/*
 * Function:    hfc_fx_enque_pdisc_req
 *
 * Purpose:     Enqueue target_info_fx to PDISC queue if PDISC is unable to initiate.
 *
 * Arguments:   
 *  pp         - Pointer to port_info
 *  target     - Pointer to target_info_fx
 *
 * Returns:     
 *
 * Notes:       Do nothing if the specified target_info_fx has already existed in 
 *              PDISC queue. 
 */
void hfc_fx_enque_pdisc_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt;

	wk_tgt = pp->next_tstart;
	if (wk_tgt == NULL) {
		pp->next_tstart = target;					/* Move to the top of the queue	*/
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
 * Function:    hfc_fx_deque_pdisc_req
 *
 * Purpose:     Dequeue target_info_fx from PDISC queue.
 *              All targets are removed from queue when no pointers are specified 
 *              as arguments, pp and target.
 * Arguments:   
 *  pp          Pointer to port_info
 *  target      Pointer to target_info_fx
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_deque_pdisc_req(struct port_info *pp,struct target_info_fx *target)
{
	struct target_info_fx *wk_tgt,*pr_tgt;

	pr_tgt = NULL;
	wk_tgt = pp->next_tstart;
	while (wk_tgt != NULL) {
		if (target == NULL) {						/* Dequeue all targets */
			pp->next_tstart = NULL;
			pr_tgt = wk_tgt->pdisc_next;
			wk_tgt->pdisc_next = NULL;
			wk_tgt = pr_tgt;
		}
		else {
			if (wk_tgt == target) {					/* Find target? */
				if (pr_tgt == NULL)					/* Queue top */
					pp->next_tstart    = wk_tgt->pdisc_next;
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
 * Function:    hfc_fx_trace
 *
 * Purpose:     Collect trace data
 *
 * Arguments:   
 *  pp          Pointer to port_info
 *  id          Trace id
 *  trc         
 *  level       1 : caller is ioctl, 0 : call is other function
 *
 * Returns:     -
 *
 * Notes:       
 */
void hfc_fx_trace(struct port_info *pp, struct core_info *core, uchar id, uchar *trc, uchar level)
{
	uint64_t	time=0;

	if (pp == NULL)
		return ;
	
	if (core) {
		core->trc_ptr[core->trc_num].trc_id = id ;

		HFC_MEMCPY(core->trc_ptr[core->trc_num].trc_data,trc,
					sizeof(core->trc_ptr[core->trc_num].trc_data)) ;

		time = (uint64_t)jiffies;
		HFC_8L_TO_8B(core->trc_ptr[core->trc_num].trc_time, time);

		core->trc_num++ ;
		if( core->trc_num >= core->trc_max )
			core->trc_num = 0 ;
		
		pp->seq_no++;
	}
	else {
		pp->trc_ptr[pp->trc_num].trc_id = id ;

		HFC_MEMCPY(pp->trc_ptr[pp->trc_num].trc_data,trc,
					sizeof(pp->trc_ptr[pp->trc_num].trc_data)) ;

		time = (uint64_t)jiffies;
		HFC_8L_TO_8B(pp->trc_ptr[pp->trc_num].trc_time, time);

		pp->trc_num++ ;
		if( pp->trc_num >= pp->trc_max )
			pp->trc_num = 0 ;
		
		pp->seq_no++;
	}
}


/* FCLNX-GPL-FX-139 Start */
/*
 * Function:    hfc_fx_mb_trace
 *
 * Purpose:     Collect mailbox trace data
 *
 * Arguments:   
 *  pp          Pointer to port_info
 *  flag        
 *
 * Returns:     -
 *
 * Notes:       
 */
void hfc_fx_mb_trace(struct port_info *pp, struct core_info *core, uchar flag, uchar resp_code)
{
	uint64_t				time=0;
	struct	hfc_mb_trace	*cur,*prev;
	struct mailbox_fx		*mb = NULL;
	uint					mb_code=0, cur_error_code=0, prev_error_code=0;
	ushort					tmp_mb_code=0, mbint_code=0, payload_type=0, prev_mb_code=0, prev_payload_type=0, prev_mb_retry_cnt=0;
	uchar					mb_int0=0, mb_int1=0, count_up=1, payload_cmd=0, rctl=0;
		
	if ( (pp == NULL) || (core == NULL) )
		return ;
	
	cur = &pp->mb_trace[pp->current_mbtrc_no];
	mb = core->mb;
	time = (uint64_t)jiffies;
	
	if(pp->current_mbtrc_no){
		prev = &pp->mb_trace[pp->current_mbtrc_no-1];
	}else{
		prev = &pp->mb_trace[HFC_FX_MAX_MB_TRACE-1];
	}
	
	switch(flag){
		case HFC_MBTRC_MBREQ:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - Set the trace of mailbox request other than FRMSNDRCV.\n", pp->dev_minor);
			cur->flag=flag;
			HFC_4B_TO_4L(mb_code, mb->mb_init.mb_code);
			tmp_mb_code = (mb_code & 0xffff0000) >> 16;
			HFC_2L_TO_2B(cur->trc.mb_req.mb_command, tmp_mb_code);
			cur->trc.mb_req.core_no = core->core_no;
			break;
		case HFC_MBTRC_MBRSP:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - Set the trace of mailbox response other than FRMSNDRCV.\n", pp->dev_minor);
			HFC_4B_TO_4L(mb_code, mb->mb_resp.mb_code);
			tmp_mb_code = (mb_code & 0xffff0000) >> 16;
			HFC_2L_TO_2B(prev_mb_code, prev->trc.mb_rsp.mb_command);
			cur_error_code =
			( (uint) hfc_fx_read_val( mb->mb_resp.err_code[0] ) << 16 ) +
			( (uint) hfc_fx_read_val( mb->mb_resp.err_code[1] ) << 8 ) +
			  (uint) hfc_fx_read_val( mb->mb_resp.err_code[2] );
			prev_error_code = 
			(prev->trc.mb_rsp.errcode[0] << 16) +
			(prev->trc.mb_rsp.errcode[1] << 8) +
			prev->trc.mb_rsp.errcode[2];
			if ( ( ( tmp_mb_code ) == prev_mb_code )
			  && ( core->core_no == prev->trc.mb_rsp.core_no )
			  && ( mb->mb_resp.fsb == prev->trc.mb_rsp.fsb )
			  && ( cur_error_code == prev_error_code ) 
			  && ( flag == prev->flag )){
				HFC_2L_TO_2B(prev_mb_retry_cnt, prev->trc.mb_rsp.mb_retry_cnt);
				prev_mb_retry_cnt++;
				HFC_2B_TO_2L(prev->trc.mb_rsp.mb_retry_cnt, prev_mb_retry_cnt);
				time = (uint64_t)jiffies & 0x00000000ffffffff;
				HFC_4L_TO_4B(prev->time, time);
				count_up=0;
			}else{
				cur->flag=flag;
				HFC_2L_TO_2B(cur->trc.mb_rsp.mb_command, tmp_mb_code);
				cur->trc.mb_rsp.core_no = core->core_no;
				cur->trc.mb_rsp.mb_retry_cnt = 0;
				cur->trc.mb_rsp.fsb = mb->mb_resp.fsb;
				cur->trc.mb_rsp.errcode[0] = mb->mb_resp.err_code[0];
				cur->trc.mb_rsp.errcode[1] = mb->mb_resp.err_code[1];
				cur->trc.mb_rsp.errcode[2] = mb->mb_resp.err_code[2];
			}
			break;
		case HFC_MBTRC_FRMSND_REQ:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - Set the trace of FRMSNDRCV request.\n", pp->dev_minor);
			cur->flag=flag;
			payload_cmd = core->payload->send_payload.data0[0];
			cur->trc.frmsndrcv_req.payload_cmd = payload_cmd;
			if( payload_cmd == HFC_GXX_REQDATA0 ){
				payload_type = ( (ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[0] ) << 8 ) +
					(ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[1] );
				HFC_2L_TO_2B(cur->trc.frmsndrcv_req.payload_type, payload_type);
			}
			break;
		case HFC_MBTRC_FRMSND_RSP:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - Set the trace of FRMSNDRCV response.\n", pp->dev_minor);
			payload_cmd = core->payload->send_payload.data0[0];
			if( payload_cmd == HFC_GXX_REQDATA0 ){
				payload_type = ( (ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[0] ) << 8 ) +
					(ushort)hfc_fx_read_val( core->payload->send_payload.type.gxx.data1[1] );
			}
			cur_error_code =
			( (uint) hfc_fx_read_val( mb->mb_resp.err_code[0] ) << 16 ) +
			( (uint) hfc_fx_read_val( mb->mb_resp.err_code[1] ) << 8 ) +
			  (uint) hfc_fx_read_val( mb->mb_resp.err_code[2] );
			prev_error_code = 
			(prev->trc.frmsndrcv_rsp.errcode[0] << 16) +
			(prev->trc.frmsndrcv_rsp.errcode[1] << 8) +
			prev->trc.frmsndrcv_rsp.errcode[2];
			HFC_2L_TO_2B(prev_payload_type, prev->trc.frmsndrcv_rsp.payload_type);
			if ( ( ( payload_cmd ) == prev->trc.frmsndrcv_rsp.payload_cmd )
			  && (( payload_cmd != HFC_GXX_REQDATA0 )||( payload_type == prev_payload_type ) )
			  && ( mb->mb_resp.fsb == prev->trc.frmsndrcv_rsp.fsb )
			  && ( cur_error_code == prev_error_code ) 
			  && ( flag == prev->flag )){
				HFC_2L_TO_2B(prev_mb_retry_cnt, prev->trc.frmsndrcv_rsp.mb_retry_cnt);
				prev_mb_retry_cnt++;
				HFC_2B_TO_2L(prev->trc.mb_rsp.mb_retry_cnt, prev_mb_retry_cnt);
				time = (uint64_t)jiffies & 0x00000000ffffffff;
				HFC_4L_TO_4B(prev->time, time);
				count_up=0;
			}else{
				cur->flag=flag;
				HFC_2L_TO_2B(cur->trc.frmsndrcv_rsp.payload_type, payload_type);
				cur->trc.frmsndrcv_rsp.payload_cmd = payload_cmd;
				cur->trc.frmsndrcv_rsp.resp_code = resp_code;
				cur->trc.frmsndrcv_rsp.mb_retry_cnt = 0;
				cur->trc.frmsndrcv_rsp.fsb = mb->mb_resp.fsb;
				cur->trc.frmsndrcv_rsp.errcode[0] = mb->mb_resp.err_code[0];
				cur->trc.frmsndrcv_rsp.errcode[1] = mb->mb_resp.err_code[1];
				cur->trc.frmsndrcv_rsp.errcode[2] = mb->mb_resp.err_code[2];
			}
			break;
		case HFC_MBTRC_MBINT:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - Set the trace of mailbox interrupt request.\n", pp->dev_minor);
			cur->flag=flag;
			mb_int0 = hfc_fx_read_val( mb->mb_intreq.mb_code[0] );
			mb_int1 = hfc_fx_read_val( mb->mb_intreq.mb_code[1] );
			mbint_code = (ushort)(mb_int0 << 8) + (ushort)(mb_int1);
			HFC_2L_TO_2B(cur->trc.mb_intreq.mbint_code, mbint_code);
			if( mbint_code == HFC_MBINTREQ_RCVFRM ){
				rctl = hfc_fx_read_val( mb->mb_intreq.type.rcvfrm.fcph_hdr.rctl );
				cur->trc.mb_intreq.rctl = rctl;
				if(rctl == HFC_FRMSNDRCV_ELS){
					cur->trc.mb_intreq.els_command = hfc_fx_read_val( mb->mb_intreq.type.rcvfrm.subtype.els_command );
				}
			}
			break;
		default:
			HFC_DBGPRT(" hfcldd%d : hfc_fx_mb_trace - The trace flag is undefind.\n", pp->dev_minor);
			count_up=0;
			break;
	}
	if(count_up){
		time = (uint64_t)jiffies & 0x00000000ffffffff;
		HFC_4L_TO_4B(cur->time, time);
		pp->current_mbtrc_no++ ;
		if( pp->current_mbtrc_no >= HFC_FX_MAX_MB_TRACE ) pp->current_mbtrc_no = 0 ;
	}
}
/* FCLNX-GPL-FX-139 End */


#define HFC_WDG_ADP	1		/* Unit of HBA    */
#define HFC_WDG_TGT	2		/* Unit of target */
#define HFC_WDG_DEV	3		/* Unit of LU     */
/*
 * Function:    hfc_fx_watchdog_enter
 *
 * Purpose:     Start and Stop watch dog timer 
 *
 * Arguments:
 *  pp         - Pointer to port_info (*)
 *  core       - Pointer to core_info (*)
 *  target     - Pointer to target_info_fx 
 *                HFC_FX_ABORT_TMR/HFC_FX_TARGET_RST_TMR (*)
 *  hfcp       - Pointer to hfc_pkt_fx  (*)
 *  lun        - lun# 
 *                HFC_FX_ABORT_TMR (*)
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
int hfc_fx_watchdog_enter(struct port_info *pp,
						struct core_info *core,
						struct target_info_fx *target,
						struct hfc_pkt_fx *hfcp, uint lun, uchar timer_id, 	/* FCLNX-GPL-038 *//* FCLNX-GPL-0343 */
						uint tout, int cancel)
{
	struct wtimer_fx *w_timer;
	uint d_time=0;
	int tmo_work=0;		/* FCLNX-GPL-260 */

	/* Search target */
	if(cancel == 0)
	{
		switch (timer_id)
		{
			case HFC_FX_LINKINIT_TMR :		
				d_time = (pp->link_initialize_tmo * HZ);
				w_timer = &pp->link_init_wdog;
				break;

			case HFC_FX_MB_RSP_TMR :			
				if ( core == NULL )
					return (3);		
//				if( test_bit(HFC_DIAG_PROGRESS, (ulong *)&pp->mp_adap_info->status ) ){
//					d_time = (HFC_FX_MB_DIAG_TO * HZ);
//				}
//				else if(tout){ /* FCLNX-GPL-243 start */
				if(tout){
					d_time = (tout * HZ);
				} /* FCLNX-GPL-243 end */
				else{
					d_time = (HFC_FX_MB_TO * HZ);
				}
				w_timer = &core->core_mb_wdog;
//				core->mb_retry_tid = timer_id;
				break;

			case HFC_FX_MB_RETRY_TMR :	
				d_time = (tout * HZ);
				w_timer = &pp->mb_retry_wdog;
				break;

			case HFC_FX_MB_DELAY_TMR :	
				d_time = tout * HZ;
				w_timer = &pp->mb_delay_wdog;
				break;

			case HFC_FX_MB_RETRY_DELAY_TMR :	
				if ( core == NULL )
					return (3);		
				d_time = (pp->mb_timer[core->mb_retry_tid].intvl * HZ);
				w_timer = &core->mb_retry_intvl_wdog;
				break;

			case HFC_FX_CTLRST_DELAY_TMR : 						/* FCLNX-0279 */
			case HFC_FX_REBOOT_DELAY_TMR :	
				if (tout)										/* FCLNX-0279 */
					d_time = (tout * HZ);						/* FCLNX-0279 */
				else											/* FCLNX-0279 */
					d_time = (HFC_FX_REBOOT_DELAY_TO * HZ);
				w_timer = &pp->reboot_wdog;
				break;

			case HFC_FX_MCKINT_TMR :							/* FCLNX-0275 */
				d_time = (HFC_FX_MCKINT_TO * HZ);
				w_timer = &pp->mckint_wdog;
				break;											/* FCLNX-0275 */

/* @MLPF */
			case HFC_FX_MLPF_FMCK_TMR :    /* Shadow LPAR only */
				d_time = (HFC_FX_MLPF_FMCK_STO * HZ);
								
				w_timer = &pp->fmck_wdog;
				break;

			case  HFC_FX_MLPF_FCSTP_TMR :  /* Shadow LPAR only */
				d_time = (HFC_MLPF_FCSTP_STO * HZ);
								
				w_timer = &pp->fcstp_wdog;
				break;

			case HFC_FX_LINKUP_TMR	:
				tmo_work = pp->linkup_tmo;							/* FCLNX-GPL-260 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
				if ( !hfc_manage_info.hfcldd_mp_mod ) {
					if ( pp->linkup_tmo ) {
						tmo_work = tmo_work -1;
					}
				}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
				d_time = ( tmo_work * HZ);							/* FCLNX-GPL-260 */
				w_timer = &pp->linkup_wdog;
				break;

			case HFC_FX_WLINKUP_MCK_TMR    :						/* FCLNX-241 */
				d_time = ( pp->mck_rcv_tmo * HZ);
				w_timer = &pp->linkup_wdog;
				break;												/* FCLNX-241 */

			case HFC_FX_SCN_LINKUP_TMR	: 			
				if ( target == NULL )
					return (3);
				tmo_work = pp->linkup_tmo;							/* FCLNX-GPL-260 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
				if ( !hfc_manage_info.hfcldd_mp_mod ) {
					if ( pp->linkup_tmo ) {
						tmo_work = tmo_work -1;
					}
				}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
				d_time = ( tmo_work * HZ);							/* FCLNX-GPL-260 */
				w_timer = &target->scnlinkup_wdog;
				break;

			case HFC_FX_WLINKUP_CNT_TMR	:							/* FCLNX-GPL-FX-424 */
				tmo_work = pp->dev_loss_tmo;
				d_time = ( tmo_work * HZ);
				w_timer = &pp->ld_err_wdog;
				break;

			case HFC_FX_DELAY_TMR :
				if ( target == NULL )
					return (3);		

				d_time = (pp->scsi_reset_delay * HZ);
				w_timer = &target->delay_wdog;
				break;

			case HFC_FX_WEXEC_TMR :
				if ( core == NULL )
					return (3);		

				d_time = (HFC_FX_WEXEC_TO * HZ);
				w_timer = &core->wexec_wdog;
				break;

			case HFC_FX_TARGET_RST_TMR :
				if ( target == NULL )
					return (3);		/* argument is abnormal		*/
				if ( hfcp == NULL )
					return (3);		/* argument is abnormal		*/

				d_time = (pp->target_reset_tmo * HZ);
				w_timer = &hfcp->cmd_timeout;
				break;

			case HFC_FX_ABORT_TMR :
				if ( target == NULL )
					return (3);		/* argument is abnormal		*/
				if ( hfcp == NULL )
					return (3);		/* argument is abnormal		*/

				if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_ABORT, hfcp) ){	/* FCLNX-GPL-0343 */
					d_time = (pp->abort_tmo * HZ);
				}
				else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_LUN_RESET, hfcp) ){
//					d_time = (pp->lun_reset_tmo * HZ);
					d_time = (pp->abort_tmo * HZ);
				}												/* FCLNX-GPL-0343 */
				w_timer = &hfcp->cmd_timeout;
				break;

			case HFC_FX_SCSI_CMD_TMR :
				if ( hfcp == NULL )
					return (3);
									
				d_time = tout;		/* FCLNX-0429 */
				w_timer = &hfcp->cmd_timeout;
				break;

			/* FCLNX-GPL-FX-016 Start */
			case HFC_FX_CANCEL_SCSI_TMR :
				if ( target == NULL )
					return (3);		/* argument is abnormal		*/
				if ( hfcp == NULL )
					return (3);		/* argument is abnormal		*/
				
				if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WITHOUT_DMA, hfcp) ){	/* FCLNX-GPL-0343 */
					d_time = (pp->mb_timer[ HFC_MBTIME_MIHLOG ].tout * HZ);
				}else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_LU_WAIT_DMA, hfcp) ){
					d_time = (pp->abort_tmo * HZ);
				}else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WITHOUT_DMA, hfcp) ){
					d_time = (pp->mb_timer[ HFC_MBTIME_MIHLOG ].tout * HZ);
				}else if( HFC_HFCP_FX_CFLAG_TEST(CFLAG_CSCSI_TGT_WAIT_DMA, hfcp) ){
					if (test_bit( HFC_TS_WAIT_TGTRSP, (ulong *)&target->status )){	/* FCLNX-GPL-FX-112 */
						d_time = (pp->target_reset_tmo * HZ);
					}else{
						d_time = (pp->mb_timer[ HFC_MBTIME_MIHLOG ].tout * HZ);
					}																/* FCLNX-GPL-FX-112 */
				}
				w_timer = &hfcp->cmd_timeout;
				break;
				
			case HFC_FX_TOTAL_TGTRST_TMR :
				if ( target == NULL )
					return (3);
				d_time = ( pp->total_tgtrst_to * HZ);
				w_timer = &target->total_tgtrst_wdog;
				break;
			/* FCLNX-GPL-FX-016 End */

			case HFC_FX_DIAG_DELAY_TMR :
				d_time = (tout * HZ);
				w_timer = &pp->reboot_wdog;
				break;

			case HFC_FX_LOGIN_DELAY_TMR:							/* FCLNX-0243 */
				d_time = (tout * HZ);
				w_timer = &pp->lgdelay_wdog;						/* FCLNX-0270 */
				break;  											/* FCLNX-0243 */

			case HFC_FX_LDLERR_TMR:
				d_time = (pp->ldl_errcnt_info->current_tmr_time * HZ);
				w_timer = &pp->ldlerr_wdog; /* FCLNX-0506 */
				break;

			case HFC_FX_LDSERR_TMR:
				d_time = (pp->lds_errcnt_info->current_tmr_time * HZ);
				w_timer = &pp->ldserr_wdog; /* FCLNX-0506 */
				break;

			case HFC_FX_IFERR_TMR:
				d_time = (pp->if_errcnt_info->current_tmr_time * HZ);
				w_timer = &pp->iferr_wdog; /* FCLNX-0506 */
				break;

			case HFC_FX_TOERR_TMR:
				d_time = (pp->to_errcnt_info->current_tmr_time * HZ);
				w_timer = &pp->toerr_wdog; /* FCLNX-0506 */
				break;

			case HFC_FX_INT_CHECK_TMR:							/* FCLNX-GPL-306 */
				d_time = tout * HZ;
				w_timer = &pp->int_chk_wdog;
				break;

			case HFC_FX_TGT_LDLERR_TMR:							/* FCLNX-GPL-327 */
				if ( target == NULL )
					return (3);
				d_time = (target->tgt_ldl_errcnt_info->current_tmr_time * HZ);
				w_timer = &target->tgt_ldlerr_wdog;
				break;

			case HFC_FX_TGT_LDSERR_TMR:							/* FCLNX-GPL-327 */
				if ( target == NULL )
					return (3);
				d_time = (target->tgt_lds_errcnt_info->current_tmr_time * HZ);
				w_timer = &target->tgt_ldserr_wdog;
				break;

			case HFC_FX_RESTART_TMR:							/* FCLNX-GPL-328 */
				if ( target == NULL )
					return (3);		/* argument is abnormal		*/

				d_time = (tout * HZ);
				w_timer = &target->restart_wdog;
				break;											/* FCLNX-GPL-328 */

			case HFC_FX_MLPF_ISOLEND_TMR:
				if (tout)										/* FCLNX-0279 */
					d_time = (tout * HZ);						/* FCLNX-0279 */
				else											/* FCLNX-0279 */
					d_time = (HFC_FX_MLPF_ISOLEND_GTO * HZ);
				w_timer = &pp->isolend_wdog;
				break;

			default :
				return (2);			/* Invalid TIMER ID		*/
		}
		
		if( w_timer->timer_flag & HFC_TIMER_VALID ){
			mod_timer_pending(&w_timer->dog, jiffies + d_time);			/* FCLNX-GPL-FX-299 */
			return (0);
		}
		init_timer(&w_timer->dog);
		
		if(!(&w_timer->dog))
		{        
        	HFC_DBGPRT("watchdog_enter() - w_timer = NULL before init \n");
			return (2);
		}
//		HFC_DBGPRT("watchdog_enter() - timer_id = %d d_time=%d\n",timer_id,d_time);
		w_timer->pp = pp;
		w_timer->ap_dev_minor = pp->dev_minor;					/* FCLNX-0322 */
		w_timer->core = core;
		w_timer->target = target;
		w_timer->timer_id = timer_id;
		w_timer->hfcpk = hfcp;
		w_timer->wdog_time = (uint)(d_time/HZ);					/* FCLNX-GPL-FX-061 */
		if( hfcp != NULL ){
			w_timer->dev = hfcp->dev;							/* FCLNX-GPL-047 *//* FCLNX-GPL-0343 */
		}else{
			if(target != NULL)
				w_timer->dev = hfc_fx_search_dev_info(target, lun);
		}
		w_timer->dog.expires = jiffies + d_time;
		w_timer->dog.data = (unsigned long) w_timer;
		w_timer->dog.function = (void (*) (unsigned long))hfc_fx_watchdog;
		w_timer->timer_flag |= HFC_TIMER_VALID;
		switch (timer_id)										/* FCLNX-0312 */
		{
			case HFC_FX_SCSI_CMD_TMR :
				if (!tout) {
					w_timer->dog.function = NULL;
					w_timer->timer_flag &= ~HFC_TIMER_VALID;
					break;
				} else {
					add_timer(&w_timer->dog);
					break;
				}
			default :
				HFC_DBGPRT("watchdog_enter() - timer_id = %d d_time=%d\n",timer_id,d_time);
				add_timer(&w_timer->dog);
				break;
		}														/* FCLNX-0312 */

	}
	else{
		switch (timer_id)
		{
			case HFC_FX_MB_RSP_TMR :	
				if ( core == NULL )
					return (3);		
				w_timer = &core->core_mb_wdog;
				if(!(&w_timer->dog)){
				    hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBB, NULL, 0) ;
					return (2);
				}
				break;

			case HFC_FX_LINKINIT_TMR :		
				w_timer = &pp->link_init_wdog;
				if(!(&w_timer->dog)){
				    hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_MBINIT, ERRID_HFCP_EVNT3, 0xBB, NULL, 0) ;
					return (2);
				}
				break;

			case HFC_FX_MB_RETRY_TMR :	
				w_timer = &pp->mb_retry_wdog;
				clear_bit(HFC_PD_MB_KEEP_RETRY, (ulong *)&pp->status_detail1);
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_MB_DELAY_TMR :	
				w_timer = &pp->mb_delay_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_MB_RETRY_DELAY_TMR :	
				if ( core == NULL )
					return (3);		
				w_timer = &core->mb_retry_intvl_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_CTLRST_DELAY_TMR : 							/* FCLNX-0279 */
			case HFC_FX_REBOOT_DELAY_TMR :	
			case HFC_FX_DIAG_DELAY_TMR :
				w_timer = &pp->reboot_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_MCKINT_TMR :								/* FCLNX-0275 */
				w_timer = &pp->mckint_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;												/* FCLNX-0275 */

			case HFC_FX_MLPF_FMCK_TMR :
				w_timer = &pp->fmck_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_MLPF_FCSTP_TMR :
				w_timer = &pp->fcstp_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_LINKUP_TMR	:
			case HFC_FX_WLINKUP_MCK_TMR:							/* FCLNX-0241 */
				w_timer = &pp->linkup_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_SCN_LINKUP_TMR	: 			
				w_timer = &target->scnlinkup_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_WLINKUP_CNT_TMR	:							/* FCLNX-GPL-FX-424 */
				w_timer = &pp->ld_err_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_DELAY_TMR :
				w_timer = &target->delay_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_WEXEC_TMR :
				if ( core == NULL )
					return (3);		
				w_timer = &core->wexec_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_TARGET_RST_TMR :
				if ( target == NULL )
					return (3);		
				if ( hfcp == NULL )
					return (3);		
				w_timer = &hfcp->cmd_timeout;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_ABORT_TMR :
				if ( target == NULL )
				 	return (3);		
				if ( hfcp == NULL )
					return (3);	
				w_timer = &hfcp->cmd_timeout;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_SCSI_CMD_TMR :
				if ( hfcp == NULL )
					return (3);
				w_timer = &hfcp->cmd_timeout;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			/* FCLNX-GPL-FX-014 Start */
			case HFC_FX_CANCEL_SCSI_TMR :
				if ( target == NULL )
					return (3);		
				if ( hfcp == NULL )
					return (3);
				w_timer = &hfcp->cmd_timeout;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_TOTAL_TGTRST_TMR	:
				if ( target == NULL )
				 	return (3);		
				w_timer = &target->total_tgtrst_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;
			/* FCLNX-GPL-FX-014 End */

														/* HFC_LOGIN_DELAY */
			case HFC_FX_LOGIN_DELAY_TMR :				/* FCLNX-0243 */
				w_timer = &pp->lgdelay_wdog;			/* FCLNX-0270 */
				if(!(&w_timer->dog)){
					return (2);
				}
				break;							/* FCLNX-0243 */

			case HFC_FX_LDLERR_TMR:
				w_timer = &pp->ldlerr_wdog;      /* FCLNX-0506 */
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_LDSERR_TMR:
				w_timer = &pp->ldserr_wdog; 	 /* FCLNX-0506 */
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_IFERR_TMR:
				w_timer = &pp->iferr_wdog;		/* FCLNX-0506 */
				if(!(&w_timer->dog)){
					return (2);
				}
				break;


			case HFC_FX_TOERR_TMR:
				w_timer = &pp->toerr_wdog;		/* FCLNX-0270 */
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_INT_CHECK_TMR:			/* FCLNX-GPL-306 */
				w_timer = &pp->int_chk_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_TGT_LDLERR_TMR:							/* FCLNX-GPL-327 */
				w_timer = &target->tgt_ldlerr_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_TGT_LDSERR_TMR:							/* FCLNX-GPL-327 */
				w_timer = &target->tgt_ldserr_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			case HFC_FX_RESTART_TMR:		/* FCLNX-GPL-328 */
				if ( target == NULL )
					return (3);		
				w_timer = &target->restart_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;	/* FCLNX-GPL-328 */

			case HFC_FX_MLPF_ISOLEND_TMR :
				w_timer = &pp->isolend_wdog;
				if(!(&w_timer->dog)){
					return (2);
				}
				break;

			default :
				return (2);			/* Invalid TIMER ID		*/
		}
		if( !(w_timer->timer_flag & HFC_TIMER_VALID) )
			return (1);
		if (w_timer->dog.function != NULL) {
			del_timer(&w_timer->dog);
			w_timer->dog.function =  NULL;
			w_timer->dog.data = (unsigned long) NULL;
			w_timer->timer_flag &= ~HFC_TIMER_VALID;
			if( timer_id != HFC_FX_SCSI_CMD_TMR ){
				HFC_DBGPRT("watchdog_enter() - stop timer_id = %d\n",timer_id);
			}
		}
	}

	if(cancel)
		pp->wtimer_cnt[timer_id]--;
	else
		pp->wtimer_cnt[timer_id]++;

	return(0);

}


/*
 * Function:    hfc_fx_top_trace
 *
 * Purpose:     Collect trace data for hfcl_top
 *
 * Arguments:   
 *  id         - Trace ID 
 *  sub_id     - Trace additional ID
 *  pp         - Pointer to port_info 
 *  target     - Pointer to target_info_fx 
 *  etc1/2/3   - Arguments (Depends on Trace ID) 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_top_trace(
	uchar					id,
	uchar 					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	uint64_t				etc1,
	uint64_t				etc2,
	uint64_t				etc3)
{
	uchar					trc_wk[128] ;
	uchar					*ptr;
	uchar					buf[4];
	struct top_fx_trc1		*trc1 ;
	
	memset(trc_wk,0,128) ;

/*----------------------------------------------*/
/*                  TRACE1                      */
/* etc1 = NULL                                  */
/* etc2 = NULL                                  */
/* etc3 = NULL                                  */
/*----------------------------------------------*/
	if ((id == HFC_FX_TRC_ISSUE_LINKINIT)		||
		(id == HFC_FX_TRC_ISSUE_OFFLINE_MB)		||
		(id == HFC_FX_TRC_ISSUE_GIDFT)			||
		(id == HFC_FX_TRC_ISSUE_GIDPN	)		||
		(id == HFC_FX_TRC_ISSUE_GPNID)			||
		(id == HFC_FX_TRC_ISSUE_RELOGIN)		||
		(id == HFC_FX_TRC_ISSUE_PDISC)			||
		(id == HFC_FX_TRC_ISSUE_MIHLOG	)		||
		(id == HFC_FX_TRC_ISSUE_CORE_START)		||
		(id == HFC_FX_TRC_ISSUE_ADD_PORTID)		||
		(id == HFC_FX_TRC_ISSUE_DEL_PORTID)		||
		(id == HFC_FX_TRC_ISSUE_CANCEL_SCSI)	||
		(id == HFC_FX_TRC_ISSUE_FLOGI)			||
		(id == HFC_FX_TRC_ISSUE_PLOGI)			||
		(id == HFC_FX_TRC_ISSUE_PRLI)			||
		(id == HFC_FX_TRC_ISSUE_FRMSNDRCV)		||
		(id == HFC_FX_TRC_MAKE_PRLI)			||
		(id == HFC_FX_TRC_MAKE_PRLO)			||
		(id == HFC_FX_TRC_MAKE_SCR)				||
		(id == HFC_FX_TRC_MAKE_LOGO)			||
		(id == HFC_FX_TRC_MAKE_GCS_ID)			||
		(id == HFC_FX_TRC_MAKE_GID_PN)			||
		(id == HFC_FX_TRC_MAKE_GID_FT)			||
		(id == HFC_FX_TRC_MAKE_RFT_ID)			||
		(id == HFC_FX_TRC_MAKE_RFF_ID)			||
		(id == HFC_FX_TRC_MAKE_CHANGE_STATE)	||
		(id == HFC_FX_TRC_ISSUE_LOADCHTRC)		||
		(id == HFC_FX_TRC_ISSUE_SHADOW_UP)		||
		(id == HFC_FX_TRC_MAKE_GPN_FT)			||
		(id == HFC_FX_TRC_ISSUE_DIAG)			||
		(id == HFC_FX_TRC_MAKE_GPN_ID)			)
	{/*-- trace format 2 --*/
		trc1 = (struct top_fx_trc1 *)trc_wk ;
		
		trc1->id = id ;
		trc1->sub_id = sub_id ;
		if (pp != NULL)
		{
			HFC_2L_TO_2B(trc1->seq_no, pp->seq_no);
			trc1->a_status = (uchar)pp->status;
			HFC_4L_TO_4B(trc1->a_status_d1, pp->status_detail1);
			HFC_4L_TO_4B(trc1->a_status_d2, pp->status_detail2);
			HFC_4L_TO_4B(buf, pp->scsi_id);
			HFC_MEMCPY(&trc1->a_scsi_id[0], &buf[1], 3);
			trc1->a_next_gidpn = (uchar)pp->next_gidpn;
		}
		if (rp != NULL)
		{
			trc1->r_rid = rp->rid;
		}
		if (core != NULL)
		{
			trc1->c_core_no = core->core_no;
			trc1->mb_status = (uchar)core->mb_status;
			trc1->mb_retry_cnt = core->mb_retry_cnt;
			ptr = (uchar *)&core->mb->mb_init;
			HFC_MEMCPY(trc1->mb_init, ptr, 80);
		}
		if (target != NULL) {
			trc1->t_flag = (uchar)target->flags;
			HFC_4L_TO_4B(trc1->t_status, target->status);
			trc1->t_id = (uchar)target->target_id;
			HFC_4L_TO_4B(buf, target->scsi_id);
			HFC_MEMCPY(&trc1->t_scsi_id[0], &buf[1], 3);
			HFC_8L_TO_8B(trc1->t_ww_name, target->ww_name);
			trc1->t_pseq = target->pseq;
		}
	}/*-- trace format 1 --*/

	hfc_fx_trace(pp, NULL, id, &trc_wk[1], 0);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
/* Debug routine															*/
/*--------------------------------------------------------------------------*/
/*
 * Function:    hfc_fx_dump_hex
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
void hfc_fx_dump_hex( char string[],void *in, int size )
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



void _hfc_fx_sleep_on(wait_queue_head_t *event, atomic_t *condition)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	wait_event((wait_queue_head_t)*event, atomic_read(condition));
#else
	wait_event(*event,atomic_read(condition));
#endif
	atomic_dec(condition);
}


void _hfc_fx_wake_up(wait_queue_head_t *event, atomic_t *condition)
{
	atomic_inc(condition);
	wake_up((wait_queue_head_t *)event);
}

/*
 * Function:    hfc_fx_issue_int_a_rst
 *
 * Purpose:     This Function is used to issue INT_A reset.
 *              INTx     : This function execute dummy read.
 *              MSI,MSI-X: This function do not execute dummy read.
 *
 * Arguments:   
 *  pp         - pointer to port_info
 *  core	   - pointer to core_info
 *  int_a_rst  - 
 *  int_a_reg  - 
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_issue_int_a_rst(struct port_info *pp, struct core_info *core, uint int_a_rst, uint int_a_reg)
{
	/* Values */
	uint after_int_a;
	int lp,i;
	int int_a_cnt = 0;
	int multi_int_flag = FALSE;
	
	/*** FIVE-EX, MSI patch ********/ /* FCLNX-GPL-144 */
	/* for only FIVE-EX */
	if( pp->pkg.type == HFC_PKTYPE_FIVE_EX ){
		/* for only MSI */
		if( pp->msi_flag == HFC_INT_TYPE_MSI ) {
			/* for only "BASIC" */ /* MLPF never use native MSI. */
			if ( HFC_FX_MMODE_CHECK_BASIC(pp) )
			{
				for( lp = 0; lp < 32; lp ++ ) {
					if( test_bit( lp, (ulong *)&int_a_reg ) ) {
						int_a_cnt++;
					}
				}
				if( int_a_cnt >= 2 ) {
					multi_int_flag = TRUE;
					/* Close INTA Mask */
					for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
						if(pp->region_arg[pp->rid] != NULL){
							if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
								hfc_fx_write_reg_core(pp, core->core_no, (uint)HFC_IOSPACE_INTA_MSK,
									(char)0x4, ( int )0x00000000, HFC_FX_CORE_OFFSET10);
							}
						}
					}
				}
			}
		}
	}
	
	/*** Reset PCI space to clear INT_A register ***********/
	hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, int_a_rst );
	hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_1_RST,( char )0x4, int_a_rst );
	hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_2_RST,( char )0x4, int_a_rst );
	hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_INT_3_RST,( char )0x4, int_a_rst );

	/*** Dummy read ****************************************/
	/* Check INT type ( INTx or MSI or MSI-X ). */
	switch( pp->msi_flag ){
		case HFC_INT_TYPE_INTX:
			/* dummy read */
			after_int_a = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INTA, (char)0x4);
			break;
			
		case HFC_INT_TYPE_MSI:
		case HFC_INT_TYPE_MSIX:
			/* Check the paramater. */
			if(pp->inta_dummy_read != HFC_DUMMY_READ_OFF){
				/* dummy read */
				after_int_a = (uint)hfc_fx_read_reg( pp, (uint)HFC_IOSPACE_INTA, (char)0x4);
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
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  (char)0x4, hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}
	
	return;
}

/*
 * Function:    hfc_fx_kmalloc
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*pp
 *				size
 *				flag
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_fx_kmalloc(struct port_info *pp, size_t size, gfp_t flag )
{
	void *addr = NULL;
	
	addr = kmalloc(size, flag);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.kmalloc_cnt);
	
	if(pp != NULL)
	{
		if(pp->pport != NULL) /* FCLNX-GPL-168 */
		{
			/* Always count up. */
			atomic_inc(&hfc_manage_info.kmalloc_cnt_ap[pp->pport->instance]);

			if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
			{
				HFC_DBGPRT("hfcldd%d: kmalloc ", pp->pport->instance);
				HFC_DBGPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
				HFC_DBGPRT("\n" );
			}
		}
	}
	
	return(addr);
}



/*
 * Function:    hfc_fx_kfree
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*pp
 *				addr
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_kfree(struct port_info *pp, const void *block )
{
	if(pp != NULL)
	{
		if(pp->pport != NULL) /* FCLNX-GPL-168 */
		{
			if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
			{
				HFC_DBGPRT("hfcldd%d: kfree ", pp->pport->instance);
				HFC_DBGPRT("Addr=0x%p ", block); /* FCLNX-GPL-149 */
				HFC_DBGPRT("\n" );
			}

			/* Always count down. */
			atomic_dec(&hfc_manage_info.kmalloc_cnt_ap[pp->pport->instance]);
		}
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.kmalloc_cnt);
	
	kfree(block);
	return;
}

/*
 * Function:    hfc_fx_dma_alloc_coherent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*pp
 *				*dev
 *				size
 *				*dma_handle
 *				gfp
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_fx_dma_alloc_coherent(struct port_info *pp, struct device *dev,
						size_t size, dma_addr_t *dma_handle, gfp_t gfp)
{
	void *addr = NULL;
	
	addr = dma_alloc_coherent(dev, size, dma_handle, gfp);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.dma_alloc_cnt);
	
	if(pp->pport != NULL) /* FCLNX-GPL-168 */
	{
		/* Always count up. */
		atomic_inc(&hfc_manage_info.dma_alloc_cnt_ap[pp->pport->instance]);

		if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_DBGPRT("hfcldd%d: dma_alloc_coherent ", pp->pport->instance);
			HFC_DBGPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
			HFC_DBGPRT("\n" );
		}
	}
	
	return(addr);
}



/*
 * Function:    hfc_fx_dma_free_coherent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*pp
 *				*dev
 *				size
 *				*vaddr
 *				dma_handle
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_dma_free_coherent(struct port_info *pp, struct device *dev,
						size_t size, void *vaddr, dma_addr_t dma_handle)
{
	if(pp->pport != NULL) /* FCLNX-GPL-168 */
	{
		if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_DBGPRT("hfcldd%d: dma_free_coherent ", pp->pport->instance);
			HFC_DBGPRT("Addr=0x%p ", vaddr); /* FCLNX-GPL-149 */
			HFC_DBGPRT("\n" );
		}
		
		/* Always count down. */
		atomic_dec(&hfc_manage_info.dma_alloc_cnt_ap[pp->pport->instance]);
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.dma_alloc_cnt);
	
	dma_free_coherent(dev, size, vaddr, dma_handle);
	return;
}

/*
 * Function:    hfc_fx_pci_alloc_consistent
 *
 * Purpose:    
 *
 *
 * Arguments:  
 *				*pp
 *				*dev
 *				size
 *				*dma_addrp
 *
 * Returns:     void *
 *
 * Notes:       
 */
void *hfc_fx_pci_alloc_consistent(struct port_info *pp, struct pci_dev *pdev,
							size_t size, dma_addr_t *dma_addrp)
{
	void *addr = NULL;
	
	addr = pci_alloc_consistent(pdev, size, dma_addrp);
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.pci_alloc_cnt);

	if(pp->pport != NULL) /* FCLNX-GPL-168 */
	{
		/* Always count up. */
		atomic_inc(&hfc_manage_info.pci_alloc_cnt_ap[pp->pport->instance]);
		
		if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_DBGPRT("hfcldd%d: pci_alloc_consistent ", pp->pport->instance);
			HFC_DBGPRT("Addr=0x%p ", addr); /* FCLNX-GPL-149 */
			HFC_DBGPRT("\n" );
		}
	}
	
	return(addr);
}


/*
 * Function:    hfc_fx_pci_free_consistent
 *
 * Purpose:    
 *
 *
 * Arguments:   
 *				*pp
 *				*pdev
 *				size
 *				*cpu_addr
 *				dma_addr
 *
 * Returns:     None
 *
 * Notes:       
 */
void hfc_fx_pci_free_consistent(struct port_info *pp, struct pci_dev *pdev,
						size_t size, void *cpu_addr, dma_addr_t dma_addr)
{
	if(pp->pport != NULL) /* FCLNX-GPL-168 */
	{
		if( pp->pport->debug_func & HFC_DEBUG_MEM_LEAK )
		{
			HFC_DBGPRT("hfcldd%d: pci_free_consistent ", pp->pport->instance);
			HFC_DBGPRT("Addr=0x%p ", cpu_addr); /* FCLNX-GPL-149 */
			HFC_DBGPRT("\n" );
		}
		
		/* Always count down. */
		atomic_dec(&hfc_manage_info.pci_alloc_cnt_ap[pp->pport->instance]);
	}
	
	/* Always count down. */
	atomic_dec(&hfc_manage_info.pci_alloc_cnt);
	
	pci_free_consistent(pdev, size, cpu_addr, dma_addr);
	return;
}


/*
 * Function:    hfc_fx_scsi_host_alloc
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
hfc_fx_scsi_host_alloc(struct scsi_host_template *sht, int privsize)
{
	struct Scsi_Host *addr = NULL;
	
	/* Always count up. */
	atomic_inc(&hfc_manage_info.host_alloc_cnt);
	
	addr = scsi_host_alloc(sht, privsize);
	
	return(addr);
}


/*
 * Function:    hfc_fx_scsi_host_put
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
void hfc_fx_scsi_host_put(struct Scsi_Host *shost)
{
	/* Always count down. */
	atomic_dec(&hfc_manage_info.host_alloc_cnt);
	
	scsi_host_put(shost);
	return;
}

/* FCLNX-GPL-154 */
/*
 * Function : hfc_fx_remap_pci_bar
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
hfc_fx_remap_pci_bar( struct pci_dev *pdev, int bar )
{
	struct Scsi_Host *host = NULL;
	struct port_info *pp   = NULL;
	ulong  base, len, flag;
	ulong  base_addr;
	
	HFC_ENTRY("hfc_fx_remap_pci_bar");

	host = pci_get_drvdata(pdev);
	if( host != NULL )
	{
		pp = (struct port_info *)host->hostdata;
	}
	else
	{	/* "host" is NULL */
		pp = NULL;
	}

	flag = pci_resource_flags(pdev, bar);
	if (!(flag & IORESOURCE_MEM))
	{	/* Err at pci_resource_flags()  */ /* Sometime "pp" will be NULL */
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCA, NULL, 0) ;
		goto error_case;
	}
	
	base = pci_resource_start(pdev, bar);
	if ( base == 0 )
	{	/* Err at pci_resource_start() */ /* Sometime "pp" will be NULL */
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCB, NULL, 0) ;
		goto error_case;
	}

	len       = pci_resource_len(pdev, bar);
	base_addr = (ulong)ioremap(base, len);
	if ( base_addr == 0x00 )
	{	/* Err at ioremap */ /* Sometime "pp" will be NULL */
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCD, NULL, 0) ;
		goto error_case;
	}

	HFC_EXIT("hfc_fx_setup_pci_bar");
	return  base_addr;

error_case:

	HFC_EXIT("hfc_fx_setup_pci_bar");
	base_addr = 0x00;
	return  base_addr;
}

/* FCLNX-GPL-154 */
/*
 * Function : hfc_fx_unmap_pci_bar
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
hfc_fx_unmap_pci_bar( struct pci_dev *pdev, ulong base_addr )
{
	HFC_ENTRY("hfc_fx_remap_pci_bar");
	
	if ( base_addr != 0x00 )
	{
		iounmap((void *)(base_addr));
	}
	
	HFC_EXIT("hfc_fx_unmap_pci_bar");

	return;
}

/* FCLNX-GPL-154 */
/*
 * Function:    hfc_fx_read_reg_ext2
 *
 * Purpose:     FIVE-FX register read 
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  offset     - register offset address
 *  size       - read size(1/2/4 Bytes)
 *
 * Returns:     FIVE-FX register data
 *  reg_size = 1  least significant 1 byte
 *  reg_size = 2  least significant 2 bytes
 *  reg_size = 4  4 bytes
 *
 * Notes:  - Lock port_info before calling this function
 *           Recommend to call hfc_fx_read_reg() rather than calling this function
 */
uint64_t hfc_fx_read_reg_ext2(
							struct port_info *pp,
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
 * Function:    hfc_fx_write_reg_ext2
 *
 * Purpose:     PCI memory write
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  offset     - register offset address
 *  reg_size   - write size (1/2/4 Bytes)
 *  data       - write data
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 *           	Recommend to call hfc_fx_write_reg() rather than calling this function
 */
void hfc_fx_write_reg_ext2(
						struct port_info *pp,
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
 * Function:    hfc_fx_read_stat_cca
 *
 * Purpose:     Reads Statistical information
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  adr        - register offset address
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 */
uint64_t hfc_fx_read_stat_cca(struct port_info  *pp,  uint adr)
{
	uint64_t ret  = 0;
	uint     wk   = 0;
	
	wk   = (uint) hfc_fx_read_reg_ext(pp, adr, 0x4);
	ret  = wk;
	ret <<= 32;
	wk   = (uint) hfc_fx_read_reg_ext(pp, (adr + 4), 0x4);
	ret  += wk;
	
	return ret;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-261  New method : HBA Port Statistical information acquisition */
/*
 * Function:    hfc_fx_hba_port_statistics_new
 *
 * Purpose:     Sets Statistical information into port_statistics structure
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *
 * Returns:     
 *
 * Notes:       Lock port_info before calling this function
 */
void hfc_fx_hba_port_statistics_new(
	struct port_info        *pp )
{
	ulong				seconds;

	seconds = get_seconds();

	if (seconds < pp->reset_stat_time)
		pp->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - ((uint64_t)1 + (uint64_t)pp->reset_stat_time));
	else
		pp->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - (uint64_t)pp->reset_stat_time);

	/* I/O Statistical information */
	pp->port_statistics.tx_frames = pp->tx_frames[0];
	pp->port_statistics.tx_words  = pp->tx_words[0];
	pp->port_statistics.rx_frames = pp->rx_frames[0];
	pp->port_statistics.rx_words  = pp->rx_words[0];

	/* Error Statistical information from CCA */
	pp->port_statistics.lip_count				= hfc_fx_read_stat_cca(pp, 0x400);
	pp->port_statistics.nos_count				= hfc_fx_read_stat_cca(pp, 0x408);
	pp->port_statistics.error_frames			= hfc_fx_read_stat_cca(pp, 0x410);
	pp->port_statistics.dumped_frames			= hfc_fx_read_stat_cca(pp, 0x418);
	pp->port_statistics.link_failure_count		= hfc_fx_read_stat_cca(pp, 0x420);
	pp->port_statistics.loss_of_sync_count		= hfc_fx_read_stat_cca(pp, 0x428);
	pp->port_statistics.loss_of_signal_count	= hfc_fx_read_stat_cca(pp, 0x430);
	pp->port_statistics.prim_seq_protocol_err_count 
												= hfc_fx_read_stat_cca(pp, 0x438);
	pp->port_statistics.invalid_tx_word_count	= hfc_fx_read_stat_cca(pp, 0x440);
	pp->port_statistics.invalid_crc_count		= hfc_fx_read_stat_cca(pp, 0x448);
	
	return;
}
#endif
#endif

/*
 * Function:    hfc_fx_get_dev_info_fx
 *
 * Purpose:     Get dev_info_fx
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
struct dev_info_fx *hfc_fx_get_dev_info_fx(struct target_info_fx *target, uint lun)		/* FCLNX-GPL-0343 */
{
	struct dev_info_fx		*dev=NULL;

	if(target == NULL) return (NULL);

	dev = target->dev;
	while (dev != NULL) {
		if ( dev->lun == lun ) {
			/* found dev_info_fx */
			break;
		}
		dev = dev->next;
	}	

	return (dev);
	
}																			/* FCLNX-GPL-0343 */

/*
 * Function:    hfc_fx_mp_watchdog_enter
 *
 * Purpose:     Start and Stop watch dog timer 
 *
 * Arguments:   
 *  pp         - Pointer to port_info (*)
 *  target     - Pointer to target_info_fx 
 *                HFC_FX_ABORT_TMR/HFC_FX_TARGET_RST_TMR (*)
 *  hfcp       - Pointer to hfc_pkt_fx  (*)
 *  lun        - lun# 
 *                HFC_FX_ABORT_TMR (*)
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
int hfc_fx_mp_watchdog_enter( struct port_info *pp, struct core_info *core, struct target_info_fx *target,
						struct hfc_pkt_fx *hfcp, struct dev_info_fx *dev, uint lun, uchar timer_id, 	/* FCLNX-0627 *//* FCLNX-GPL-0343 */
						uint tout, int cancel)
{
	struct wtimer_fx *w_timer;
	uint d_time=0;
	
	/* Search target */
	if(cancel == 0)
	{
		switch (timer_id)
		{
			case HFC_FX_DELAY_TMR_DEV:	
								if( dev == NULL )
									return (3);
								d_time = (pp->lun_reset_delay * HZ);
								w_timer = &dev->lun_delay_wdog; 
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								init_timer(&dev->lun_delay_wdog.dog); 
								break;
			/* FCLNX-GPL-FX-014 Start */
			case HFC_FX_TOTAL_ABORT_TMR:
								if( dev == NULL )
									return (3);
								d_time = (pp->total_abort_to* HZ);
								w_timer = &dev->total_abort_wdog; 
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								init_timer(&dev->total_abort_wdog.dog); 
								break;
			/* FCLNX-GPL-FX-014 End */
			case HFC_FX_PATH_RETRY_TMR:			/* HFC-PCM */
								if( dev == NULL )
									return (3);
								d_time = (HFC_FX_PATH_RETRY_TO * HZ);
								w_timer = &dev->path_retry_wdog; 
								if( w_timer->timer_flag & HFC_TIMER_VALID )
									return (1); 					/* It doesn't stop */
								init_timer(&dev->path_retry_wdog.dog); 
								break;

			default :
								return (2);			/* Invalid TIMER ID		*/
		}
		if(!(&w_timer->dog))
		{        
        	 HFC_DBGPRT("watchdog_enter() - w_timer = NULL before init \n");
		}

		w_timer->pp = pp;
		w_timer->ap_dev_minor = pp->dev_minor;	/* FCLNX-GPL-FX-014 */
		w_timer->core = core;	/* FCLNX-GPL-FX-014 */
		w_timer->target = target;
		w_timer->timer_id = timer_id;
		w_timer->hfcpk = hfcp;
		w_timer->dev = dev;
		w_timer->dog.expires = jiffies + d_time;
		w_timer->dog.data = (unsigned long) w_timer;
		w_timer->dog.function = (void (*) (unsigned long))hfc_fx_watchdog;
		w_timer->timer_flag |= HFC_TIMER_VALID;
		add_timer(&w_timer->dog);

	}
	else{
		switch (timer_id)
		{
			case HFC_FX_DELAY_TMR_DEV:	
				if( dev == NULL )
					break;
				w_timer = &dev->lun_delay_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					return(3);						/* FCLNX-0648 */ /* FCLNX-0657 */
				if(!(&w_timer->dog)){
					break;
				}
				if (w_timer->dog.function != NULL) {
					del_timer(&w_timer->dog);
					w_timer->dog.function =  NULL;
					w_timer->dog.data = (unsigned long) NULL;
					w_timer->timer_flag &= ~HFC_TIMER_VALID;
				}
				break;

			/* FCLNX-GPL-FX-014 Start */
			case HFC_FX_TOTAL_ABORT_TMR:
				if( dev == NULL )
					break;
				w_timer = &dev->total_abort_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					return(3);						/* FCLNX-0648 */ /* FCLNX-0657 */
				if(!(&w_timer->dog)){
					break;
				}
				if (w_timer->dog.function != NULL) {
					del_timer(&w_timer->dog);
					w_timer->dog.function =  NULL;
					w_timer->dog.data = (unsigned long) NULL;
					w_timer->timer_flag &= ~HFC_TIMER_VALID;
				}
				break;
			/* FCLNX-GPL-FX-014 End */
			case HFC_FX_PATH_RETRY_TMR:
				if( dev == NULL )
					break;
				w_timer = &dev->path_retry_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					return(3);						/* FCLNX-0648 */ /* FCLNX-0657 */
				if(!(&w_timer->dog)){
					break;
				}
				if (w_timer->dog.function != NULL) {
					del_timer(&w_timer->dog);
					w_timer->dog.function =  NULL;
					w_timer->dog.data = (unsigned long) NULL;
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
 * Function:    hfc_fx_clear_dev_info_fx
 *
 * Purpose:     Reset dev_info_fx->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_clear_dev_info_fx(struct dev_info_fx *dev)								/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	if(dev == NULL) return;	
	clear_bit( HFC_LUN_RESET_DELAY_TO, (ulong *)&dev->io_status );
	if(!hfc_manage_info.hfcldd_mp_mod)
		clear_bit(HFC_DS_LUNRST_DELAY, (ulong *)&dev->lustat);					/* FCLNX-GPL-FX-152 */

	return;
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

void hfc_fx_all_clear_dev_info_fx(struct port_info *pp, struct dev_info_fx *dev)		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	while (dev != NULL) {
		/* stop LUN Reset Delay Timer */
		hfc_fx_mp_watchdog_enter(pp, NULL, NULL, NULL, dev, 0, HFC_FX_DELAY_TMR_DEV, 0, TRUE);
		hfc_fx_mp_watchdog_enter(pp, NULL, NULL, NULL, dev, 0, HFC_FX_TOTAL_ABORT_TMR, 0, TRUE);	/* FCLNX-GPL-FX-014 */
		
		hfc_fx_clear_dev_info_fx(dev);
		dev = dev->next;
	}
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */


/*
 * Function:    hfc_fx_set_dev_info_fx
 *
 * Purpose:     Set dev_info_fx->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_set_dev_info_fx(struct dev_info_fx *dev)									/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{

	if(dev == NULL) return;	
	set_bit( HFC_LUN_RESET_DELAY_TO, (ulong *)&dev->io_status );
	if(!hfc_manage_info.hfcldd_mp_mod)
		set_bit(HFC_DS_LUNRST_DELAY, (ulong *)&dev->lustat);					/* FCLNX-GPL-FX-152 */

	return;
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

/*
 * Function:    hfc_fx_search_dev_info
 *
 * Purpose:     Set dev_info_fx->io_status
 *
 * Arguments:   
 *  w_timer    -
 *
 * Returns:     
 *
 * Notes:       
 */
struct dev_info_fx *hfc_fx_search_dev_info(struct target_info_fx *target, short lun_id)		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
{
	struct dev_info_fx		*dev=NULL;

	if(target == NULL) return (NULL);
	dev = target->dev;
	while (dev != NULL) {
		if ( dev->lun == lun_id ) {
			/* found dev_info_fx */
			break;
		}
		dev = dev->next;
	}	

	return (dev);
	
}																			/* FCLNX-0627 *//* FCLNX-GPL-0343 */

/*
 * Function:    hfc_fx_free_dev
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
void hfc_fx_free_dev(struct target_info_fx *target)
{
	struct dev_info_fx *dev,*dev_next;
	struct port_info *pp;
	int i;
	
	HFC_ENTRY("hfc_fx_free_dev");
	
	pp = target->pp;
	dev = target->dev;
	while (dev != NULL) {
		dev_next = dev->next;
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if (dev->dummy_cmnd[i] != NULL) {
				memset( dev->dummy_cmnd[i], 0, sizeof(struct dummy_scsi_cmnd) );
				hfc_fx_kfree(pp, dev->dummy_cmnd[i]);
				dev->dummy_cmnd[i] = NULL;
				HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release dev->dummy_cmnd logical=%lx\n",
					(ulong)dev->dummy_cmnd[i]);
			}
		}
		memset( dev, 0, sizeof(struct dev_info_fx) );
		hfc_fx_kfree(pp, dev);
		dev = dev_next;
	}
}

int hfc_fx_issue_gidpn(struct port_info *pp,struct target_info_fx *target)
{
	return 0;
}

uint hfc_getBitPow8(uchar bitMap)
{
	uint cnt = 0;
	for (cnt = 0; cnt < 8; cnt++) {
		if (bitMap & (0x01 << cnt)) { return cnt; }
	}
	return 0;
}

uint hfc_getBitNum8(uchar bitMap)
{
	uint cnt = 0;
	for (cnt = 0; cnt < 8; cnt++) {
		if (bitMap & (0x80 >> cnt)) { return cnt; }
	}
	return 0;
}

