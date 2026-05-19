/*
 * hfcl_top_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_top_fx.h,v 1.1.2.12.2.1.2.3.2.1 2015/03/05 02:19:42 toyo Exp $
 */


#ifndef _H_HFCW_TOP_FX				/* Double definition prevention */
#define _H_HFCW_TOP_FX
/*--------------------------------------------------------------------------*/
/*	   Hitachi FC card adaptor Linux driver  			    */
/*		Top-level processing, Mailbox processing Header file	    */
/*								            */
/*		 NAME : hfcl_top_fx.h					    */
/*									    */
/*--------------------------------------------------------------------------*/
extern void hfc_fx_copy_iocinfo( struct port_info *, struct core_info *);
extern void hfc_fx_copy_master_to_slave( struct port_info *, struct core_info * );
extern struct target_info_fx *hfc_fx_hash_target_valid( struct port_info *, uint );
extern struct target_info_fx *hfc_fx_hash_target_info( struct port_info *, uint );
extern struct target_info_fx *hfc_fx_hash_target_info_wwn(struct port_info *, uint64_t );
extern struct target_info_fx *hfc_fx_hash_target_info_wwn_no_flag(struct port_info *, uint64_t );
extern struct target_info_fx *hfc_fx_pseq_target_info_fx(struct port_info *ap, uint pseq );
extern uint hfc_fx_uniq_seq_num( struct port_info * );
extern void hfc_fx_release_seq_num( struct port_info *, int );
extern uint64_t hfc_fx_read_cnfg( struct port_info *, uint , char );
extern void hfc_fx_write_cnfg( struct port_info *, uint , char , uint64_t );
extern uint64_t hfc_fx_read_reg_ext(struct port_info *, uint , char );
extern void hfc_fx_write_reg_ext(struct port_info *, uint , char , uint64_t );
extern uint hfc_fx_read_ramid(struct port_info *, uint , uchar , uchar, uint );
extern uint hfc_fx_write_ramid(struct port_info *, uint, uchar, uchar, uint );	/* FCLNX-GPL-FX-145 */
extern int hfc_fx_read_flash(struct port_info *pp, int offset, int size, uchar *buf);
extern uint64_t hfc_fx_read_tbl( void *, char );
extern void hfc_fx_write_tbl( void *, char , uint64_t );
extern int hfc_fx_write_indarea(struct port_info *pp, int core_no, int offset, uint data, int rammask);
extern void lock_fx_mailbox( struct port_info *pp);
extern int lock_fx_try_mailbox( struct port_info * );
extern void unlock_fx_mailbox( struct port_info *pp);
extern int lock_fx_try_mpap( struct mp_adap_info *mpap );
extern void start_fx_next_mailbox( struct port_info *, struct hfc_pkt_fx *hfcp_to );
extern void issue_fx_next_mailbox( struct port_info *pp, struct hfc_pkt_fx *hfcp_to );
extern int  decide_fx_next_mailbox( struct port_info *, ushort *mb_tid );
extern void hfc_fx_mailbox_initiate( struct port_info *, struct core_info *, uint );
extern int hfc_fx_mailbox_response( struct port_info *, struct core_info *, uint * );
extern int hfc_fx_mb_passthrough(struct port_info *, struct core_info *, ushort , ulong ,int, uint );
extern int hfc_fx_mb_passthrough_rsp(struct port_info *, struct core_info *, uint);
extern int hfc_fx_payld_response(struct port_info *, struct core_info *,
                          uchar *, uchar *);
extern void hfc_fx_initialize_failed(struct port_info *, struct core_info *, struct target_info_fx *);
extern int hfc_fx_all_core_start(struct port_info *);
extern int hfc_fx_mailbox_proc( struct port_info *, struct core_info *, ushort, ulong, int );
extern int hfc_fx_issue_linkini( struct port_info * );
extern int hfc_fx_issue_gidft( struct port_info * );
extern int hfc_fx_issue_gidpn( struct port_info *, struct target_info_fx * );
extern int hfc_fx_issue_gpnid( struct port_info *, uint );
extern int hfc_fx_issue_relogin( struct port_info *,struct target_info_fx * );
extern int hfc_fx_issue_mihlog( struct port_info *, struct hfc_pkt_fx * );
//extern int hfc_fx_send_gpnid(struct port_info *pp);
extern int hfc_fx_send_login(struct port_info *pp);
extern int hfc_fx_send_pdisc(struct port_info *pp, struct core_info *core );
extern void hfc_fx_enque_plogi_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_deque_plogi_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_enque_prli_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_deque_prli_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_enque_pdisc_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_deque_pdisc_req(struct port_info *,struct target_info_fx *);
extern void hfc_fx_trace(struct port_info *,struct core_info *,uchar ,uchar *,uchar );
extern int hfc_fx_watchdog_enter( struct port_info *pp, struct core_info *core, struct target_info_fx *target,
						struct hfc_pkt_fx *hfcp, uint lun, uchar timer_id, 		/* FCLNX-GPL-0343 */
						uint tout, int cancel);

extern void hfc_fx_check_target_info_fx( struct port_info * );
extern void hfc_fx_dump_hex( char *,void *, int );

extern void _hfc_fx_sleep_on(wait_queue_head_t *, atomic_t *condition);
extern void _hfc_fx_wake_up(wait_queue_head_t *, atomic_t *condition);
extern void hfc_fx_issue_int_a_rst(struct port_info *pp, struct core_info *core, 
	uint int_a_rst, uint int_a_reg);

extern void *hfc_fx_kmalloc(struct port_info *pp, size_t size, gfp_t flag );
extern void hfc_fx_kfree(struct port_info *pp, const void *block );
extern void *hfc_fx_dma_alloc_coherent(struct port_info *pp, struct device *dev,
							size_t size, dma_addr_t *dma_handle, gfp_t gfp);
extern void hfc_fx_dma_free_coherent(struct port_info *pp, struct device *dev,
							size_t size, void *vaddr, dma_addr_t dma_handle);
extern void *hfc_fx_pci_alloc_consistent(struct port_info *pp, struct pci_dev *pdev,
							size_t size, dma_addr_t *dma_addrp);
extern void hfc_fx_pci_free_consistent(struct port_info *pp, struct pci_dev *pdev,
							size_t size, void *cpu_addr, dma_addr_t dma_addr);
extern struct Scsi_Host *hfc_fx_scsi_host_alloc(struct scsi_host_template *sht, int privsize);
extern void hfc_fx_scsi_host_put(struct Scsi_Host *shost);

extern ulong hfc_fx_remap_pci_bar( struct pci_dev *pdev, int bar ); /* FCLNX-GPL-154 */
extern void hfc_fx_unmap_pci_bar( struct pci_dev *pdev, ulong base_addr ); /* FCLNX-GPL-154 */
extern uint64_t hfc_fx_read_reg_ext2(
							struct port_info *pp,
							ulong base_addr,
							uint offset,
							char reg_size
							); /* FCLNX-GPL-154 */
extern void hfc_fx_write_reg_ext2(
						struct port_info *pp,
						ulong base_addr,
						uint offset,
						char reg_size,
						uint64_t data
						); /* FCLNX-GPL-154 */

extern uint64_t hfc_fx_read_stat_cca(struct port_info  *pp,  uint adr);	/* FCLNX-GPL-261 */
extern void hfc_fx_hba_port_statistics_new(struct port_info  *pp );		/* FCLNX-GPL-261 */

extern struct dev_info_fx *hfc_fx_get_dev_info_fx(struct target_info_fx *target, uint lun);		/* FCLNX-GPL-0343 */

extern int hfc_fx_mp_watchdog_enter( struct port_info *pp, struct core_info *core, struct target_info_fx *target,		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
						struct hfc_pkt_fx *hfcp, struct dev_info_fx *dev, uint lun, uchar timer_id, 
						uint tout, int cancel);											/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_fx_clear_dev_info_fx(struct dev_info_fx *dev);									/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_fx_all_clear_dev_info_fx(struct port_info *pp, struct dev_info_fx *dev);			/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_fx_set_dev_info_fx(struct dev_info_fx *dev);										/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern struct dev_info_fx *hfc_fx_search_dev_info(struct target_info_fx *target, short lun_id);	/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_fx_free_dev(struct target_info_fx *target);					/* FCLNX-GPL-0343 */

extern int hfc_fx_issue_core_start( struct port_info *, struct core_info * );
extern int hfc_fx_issue_add_portid( struct port_info *, struct core_info * );
extern int hfc_fx_issue_del_portid( struct port_info *, struct core_info * );
extern int hfc_fx_issue_load_ch_trclog( struct port_info * );
extern int hfc_fx_issue_cancel_scsi( struct port_info *pp, struct core_info *core, struct target_info_fx *target, struct dev_info_fx *dev, struct hfc_pkt_fx *hfcp, uchar logid, uchar cancel_nexus, uchar cancel_ctl);
extern int hfc_fx_all_shadow_up(struct port_info *pp);
extern int hfc_fx_issue_shadow_up( struct port_info *, struct core_info * );
extern int hfc_fx_all_offline_mb( struct port_info *pp );
extern int hfc_fx_issue_offline_mb( struct port_info *pp, struct core_info *core );
extern int hfc_fx_issue_flogi( struct port_info *  );
extern int hfc_fx_issue_plogi( struct port_info *, struct target_info_fx * );
extern int hfc_fx_issue_pdisc( struct port_info *, struct target_info_fx * );
extern int hfc_fx_issue_frmsndrcv( struct port_info *, struct target_info_fx *target, uint scsi_id, uchar rctl );
extern int hfc_fx_make_prli( struct port_info *, struct core_info *, struct target_info_fx * );
extern int hfc_fx_make_prlo( struct port_info *, struct core_info * );
extern int hfc_fx_make_scr( struct port_info *, struct core_info * );
extern int hfc_fx_make_logo( struct port_info *, struct core_info *, struct target_info_fx * );
extern int hfc_fx_make_gcs_id( struct port_info *, struct core_info * );
extern int hfc_fx_make_gid_pn( struct port_info *, struct core_info *, struct target_info_fx * );
extern int hfc_fx_make_gpn_id( struct port_info *, struct core_info *, uint scsi_id );
extern int hfc_fx_make_gid_ft( struct port_info *, struct core_info * );
extern int hfc_fx_make_rft_id( struct port_info *, struct core_info * );
extern int hfc_fx_make_rff_id( struct port_info *, struct core_info * );
extern int hfc_fx_make_gpn_ft( struct port_info *, struct core_info * );
extern int hfc_fx_issue_change_state( struct port_info *, uchar state );
extern int hfc_fx_issue_prli( struct port_info *, struct target_info_fx * );
//extern int hfc_fx_send_gidpn( struct port_info *, struct core_info * );
//extern int hfc_fx_send_gpnid(struct port_info *, struct core_info *, uint scsi_id );
extern int hfc_fx_send_gpnid(struct port_info *, struct core_info * );
extern int hfc_fx_send_plogi(struct port_info *, struct core_info *);
extern int hfc_fx_send_prli(struct port_info *, struct core_info *);
extern void hfc_fx_change_portstat_linkup(struct port_info *pp, struct core_info *core);
extern void hfc_fx_change_portstat_linkdown(struct port_info *pp, struct core_info *core);
extern int hfc_fx_all_add_portid(struct port_info *pp );
extern int hfc_fx_all_del_portid(struct port_info *pp );
extern int hfc_fx_all_cancel_scsi(struct port_info *pp, struct target_info_fx *target, struct dev_info_fx *dev, struct hfc_pkt_fx *hfcp, uchar cancel_nexus,uchar cancel_ctl);
extern uint hfc_getBitPow8(uchar bitMap);
extern uint hfc_getBitNum8(uchar bitMap);
extern void hfc_fx_mb_trace(struct port_info *pp, struct core_info *core, uchar flag, uchar resp_code);	/* FCLNX-GPL-FX-139 */

#define HFC_CANCEL_PNEXUS	0x80
#define HFC_CANCEL_INEXUS	0x08
#define HFC_CANCEL_ITNEXUS	0x0C
#define HFC_CANCEL_ITLNEXUS	0x0E
#define HFC_CANCEL_ITENEXUS	0x0D

#define HFC_CANCEL_WAIT_DMA		0x00	/* FCLNX-GPL-FX-014 */
#define HFC_CANCEL_WITHOUT_DMA	0x40	/* FCLNX-GPL-FX-014 */
#define HFC_CANCEL_MIHREQ		0xC0	/* FCLNX-GPL-FX-014 */

#define HFC_LOGID_SOFTLOG	0x10
#define HFC_LOGID_LINKINC	0x11

/*--- TRACE ID ---------------------*/

#define HFC_FX_TRC_ISSUE_LINKINIT		0x51	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_GIDFT			0x52	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_GIDPN			0x53	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_GPNID			0x54	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_RELOGIN		0x55	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_PDISC			0x56	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_MIHLOG			0x57	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ADD_TARGET			0x58	/* TOP_TRC Format2 */
#define HFC_FX_TRC_COMMIT_TARGET		0x59	/* TOP_TRC Format2 */
#define HFC_FX_TRC_DEL_TARGET			0x5a	/* TOP_TRC Format2 */

#define HFC_FX_TRC_ISSUE_CORE_START		0x5b	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_ADD_PORTID		0x5c	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_DEL_PORTID		0x5d	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_CANCEL_SCSI	0x5e	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_FLOGI			0x5f	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_PLOGI			0x60	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_PRLI			0x61	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_FRMSNDRCV		0x62	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_PRLI			0x63	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_PRLO			0x64	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_SCR				0x65	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_LOGO			0x66	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_GCS_ID			0x67	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_GID_PN			0x68	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_GID_FT			0x69	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_RFT_ID			0x6a	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_RFF_ID			0x6b	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_CHANGE_STATE	0x6c	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_GPN_ID			0x6d	/* TOP_TRC Format1 */
#define HFC_FX_TRC_MAKE_GPN_FT			0x6e	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_LOADCHTRC		0x6f	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_OFFLINE_MB		0x70	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_SHADOW_UP		0x71	/* TOP_TRC Format1 */
#define HFC_FX_TRC_ISSUE_DIAG			0x72	/* TOP_TRC Format1 */


struct top_fx_trc1 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				seq_no ;			/* +02 */
	uchar				c_core_no ;			/* +04 */
	uchar				mb_retry_cnt ;		/* +05 */
	uchar				mb_status;			/* +06 */
	uchar				a_status ;			/* +07 */
	uint				a_status_d1 ;		/* +08 */
	uint				a_status_d2 ;		/* +0c */
	uchar				resv1[2];			/* +10 */
	uchar				a_next_gidpn;		/* +12 */
	uchar				t_pseq;				/* +13 */
	uchar				t_flag;				/* +14 */
	uchar				t_status[3];		/* +15 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	uchar				t_id;				/* +1c */
	uchar				t_scsi_id[3];		/* +1d */
	uint64_t			t_ww_name;			/* +20 */
	uchar				mb_init[80];		/* +28-77 */
//	uint64_t			current_time;		/* +78-7f */
};


/* Macro definition */
/*
 * Function:    hfc_fx_w_start() / hfc_fx_w_stop()
 *
 * Purpose:     Execution of watch dog timer registration/release
 *
 * Arguments:   
 *  pp         - port_info structure pointer
 *  core       - core_info structure pointer
 *  timer_id   - TIMER ID
 *
 * Returns:     
 *  0          - Watch_dog registration/cancellation success
 *  1          - Unjustified TIMER ID
 *  2          - It watch_dog registration/already has canceled
 *
 * * context:   user / kernel / interrupt
 *
 * Notes:       (1) TIMER that uses hfc_fx_watchdog_enter the second argument
 *                  cannot use this macro
 */
#define hfc_fx_w_start( _PP, _CORE, _TIMEID, _TIME ) (hfc_fx_watchdog_enter( _PP,_CORE,NULL,NULL,0,_TIMEID,_TIME,0))
#define hfc_fx_w_stop( _PP, _CORE, _TIMEID )	(hfc_fx_watchdog_enter( _PP,_CORE,NULL,NULL,0,_TIMEID,0,1))


/*
 * Function:    hfc_fx_read_reg() / hfc_fx_write_reg()
 *
 * Purpose:     PCI memory read
 *              The register offset is made a table, and the difference of the register map is absorbed
 *
 * Arguments:   
 *  _AP        - Adap_info structure pointer
 *  _REGNO     - Register registration number
 *  _SIZE      - Reading size(1/2/4)
 *  _DATA      - Writing data
 *
 * Returns:     FPP register data
 *  reg_size = 1 - low rank one byte
 *  reg_size = 2 - Two bytes in the under
 *  Reg_size = 4 - 4 bytes
 *
 * Notes:       Secure the lock of port_info
 */

#define hfc_fx_read_reg( _PP, _REGNO, _SIZE ) \
	( hfc_fx_read_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO], (_SIZE)) )

#define hfc_fx_read_reg_core( _PP, _CORENO, _REGNO, _SIZE, _OFFSET ) \
	( hfc_fx_read_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO] + _CORENO*_OFFSET, (_SIZE)) )

#define hfc_fx_write_reg( _PP, _REGNO, _SIZE, _DATA ) \
	( hfc_fx_write_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO], (_SIZE), (_DATA)) )

#define hfc_fx_write_reg_core( _PP, _CORENO, _REGNO, _SIZE, _DATA, _OFFSET ) \
	( hfc_fx_write_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO] + _CORENO*_OFFSET, (_SIZE), (_DATA)) )

#define hfc_fx_read_reg_rss_core( _PP, _CORENO, _RSS, _REGNO, _SIZE, _OFFSET, _RSSOFFSET ) \
	( hfc_fx_read_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO] + _CORENO*_OFFSET + _RSS*_RSSOFFSET, (_SIZE)) )

#define hfc_fx_write_reg_rss_core( _PP, _CORENO, _RSS, _REGNO, _SIZE, _DATA, _OFFSET, _RSSOFFSET ) \
	( hfc_fx_write_reg_ext((_PP), ((struct port_info*)(_PP))->pkg.map->iosp.reg[_REGNO] + _CORENO*_OFFSET+ _RSS*_RSSOFFSET, (_SIZE), (_DATA)) )


#define	HFC_FX_CORE_OFFSET10	0x10
#define	HFC_FX_CORE_OFFSET40	0x40
#define	HFC_FX_CORE_OFFSET80	0x80
#define HFC_FX_CORE_OFFSET100	0x100

/*
 * Function:    hfc_fx_read_val() / hfc_fx_write_val()
 *
 * Purpose:     It makes it from data reading BigEndian from the memory to LittleEndian
 *              It uses it for RD/WR in the communication area (fw_init_tbl, Mailbox, XOB, and XRB, etc.) with F/W
 *              <Differ with hfc_fx_read_tbl >
 *               The RD/WR area automatically acquires the number of bytes for the variable area with sizeof of 
 *               the specified variable
 *
 * Arguments:   
 *  _AAA       - RD/WR variable(substance)
 *  _DATA      - Writing data
 *
 * Returns:     
 *  hfc_fx_read_val  - Data read from variable
 *  hfc_fx_write_val - nothing
 *
 * Notes:       
 */
#define hfc_fx_read_val( _AAA ) \
	(pp->manage_info->pubp->hfc_fx_read_tbl( (void *)&(_AAA), sizeof(_AAA)))

#define hfc_fx_write_val( _AAA, _DATA) \
	(hfc_fx_write_tbl( &(_AAA), sizeof(_AAA), _DATA ))

struct gidft_port {
	uchar	ctl ;
	uchar	port_id[3] ;
} ;

struct gpnft_port {
	
	uchar	ctl ;
	uchar	port_id[3] ;
	uchar	resrv1[4] ;
	uint64_t	port_name;
} ;


#define PAYLD_ACCEPT	0
#define PAYLD_REJECT	1
#define PAYLD_INVLLEN	2

#define HFC_SEND_PAYLOADL_MAX 0x0800
#define HFC_RECV_PAYLOADL_MAX 0x2000

struct payload_fx {
	/* send payload ----------------------------------------------------*/
	struct send_payload {
#define HFC_PRLI_SLENGTH	0x0014			/* PRLI Request Length		*/
#define HFC_PRLO_SLENGTH	0x0014			/* PRLO Request Length		*/
#define HFC_SCR_SLENGTH		0x0008			/* SCR Request Length		*/
#define HFC_LOGO_SLENGTH	0x0010			/* LOGO Request Length		*/
#define HFC_GCSID_SLENGTH	0x0014			/* GCS_ID Request Length	*/
#define HFC_GIDPN_SLENGTH	0x0018			/* GID_PN Request Length	*/
#define HFC_GPNID_SLENGTH	0x0014			/* GPN_ID Request Length	*/
#define HFC_GIDFT_SLENGTH	0x0014			/* GID_FT Request Length	*/
#define HFC_RFTID_SLENGTH	0x0034			/* RFT_ID Request Length	*/
#define HFC_RFFID_SLENGTH	0x0018			/* RFF_ID Request Length	*/
#define HFC_GPNFT_SLENGTH	0x0014			/* GPN_FT Request Length	*/
		uchar		data0[8];				/* +0 - +7					*/
//		uchar		data20[4];				/* +4 - +7					*/
											/* FRMSNFRCV Common field	*/

#define HFC_PRLI_REQDATA0	0x20			/* PRLI Request Data[0]		*/
#define HFC_PRLO_REQDATA0	0x21			/* PRLO Request Data[0]		*/
#define HFC_SCR_REQDATA0	0x62			/* SCR Request Data[0]		*/
#define HFC_LOGO_REQDATA0	0x05			/* LOGO Request Data[0]		*/
#define HFC_GXX_REQDATA0	0x01			/* GXX_XX Request Data[0]	*/
#define HFC_GCSID_REQDATA8	0x0114			/* GXX_XX Request Data[8-9]	*/
#define HFC_GIDPN_REQDATA8	0x0121			/* GID_PN Request Data[8-9]	*/
#define HFC_GPNID_REQDATA8	0x0112			/* GPN_ID Request Data[8-9]	*/
#define HFC_GIDFT_REQDATA8	0x0171			/* GID_FT Request Data[8-9]	*/
#define HFC_GPNFT_REQDATA8	0x0172			/* GPN_FT Request Data[8-9]	*/
#define HFC_RFTID_REQDATA8	0x0217			/* RFT_ID Request Data[8-9]	*/
#define HFC_RFFID_REQDATA8	0x021f			/* RFF_ID Request Data[8-9]	*/
		/* type : Request type */
		union {
			/* PRLI */
			struct {
				uchar	data1[8] ;				/* +8 - +F				*/
				uint	prli_param_req ;		/* +10 - +13 			*/
				uchar	resv1[2028] ;			/* +14 - +7FF			*/
			} prli ;
			/* PRLO */
			struct {
				uchar	data1[12] ;				/* +8  - +13			*/
				uchar	resv1[2028] ;			/* +14 - +7FF			*/
			} prlo ;
			/* SCR */
			struct {
//				uchar	data1[4] ;				/* +8 - +B				*/
				uchar	resv1[2040] ;			/* +8 - +7FF			*/
			} scr ;
			/* LOGO */
			struct {
				uint64_t	nport_name ;		/* +8 - +F				*/
				uchar	resv1[2032] ;			/* +10 - +7FF			*/
			} logo ;
			/* GXX */
			struct {
				uchar data1[8];					/* +8 - +f				*/
				union {
					/* GCS_ID */
					struct {
						uint	port_id ;				/* +10 - +13 			*/
						uchar	resv1[2028] ;			/* +14 - +7FF			*/
					} gcs_id ;
					/* GID_PN */
					struct {
						uint64_t	nport_name ;		/* +10 - +17			*/
						uchar	resv1[2024] ;			/* +18 - + +7FF 		*/
					} gid_pn ;
					/* GPN_ID */
					struct {
						uint	port_id ;				/* +10 - +13 			*/
						uchar	resv1[2028] ;			/* +14 - +7FF			*/
					} gpn_id ;
					/* GID_FT */
					struct {
						uchar	data2[4] ;				/* +10 - +13 			*/
						uchar	data3[2028] ;			/* +14 - +7FF			*/
					} gid_ft ;
					/* RFT_ID */
					struct {
						uint	port_identifier ;		/* +10 - +13			*/
						uchar	data2[32] ;				/* +14 - +33			*/
						uchar	resv1[1996] ;			/* +34 - +7FF			*/
					} rft_id ;
					/* RFF_ID */
					struct {
						uint	port_identifier ;		/* +10 - +13			*/
						uchar	data2[4] ;				/* +14 - +17			*/
						uchar	resv1[2024] ;			/* +18 - +7FF			*/
					} rff_id ;
					/* GPN_FT */
					struct {
						uchar	data2[4] ;				/* +10 - +13 			*/
						uchar	data3[2028] ;			/* +14 - +7FF			*/
					} gpn_ft ;
				} sub_type;
			} gxx ;
		} type ;
	} send_payload ;
	/* Receive Payload -------------------------------------------------*/
	struct receive_payload {
#define HFC_PAYLOAD_ACCEPT	0x02
#define	HFC_PAYLOAD_REJECT	0x01
#define HFC_PAYLOAD_GXX_ACCEPT	0x8002
#define	HFC_PAYLOAD_GXX_REJECT	0x8001

#define HFC_LS_RJT_LENGTH	0x0008			/* LS_RJT Length   			*/
#define HFC_LS_ACC_LENGTH	0x0004			/* LS_ACC(NULL ACC) Length	*/
#define HFC_PRLI_RLENGTH	0x0014			/* PRLI Response Length		*/
#define HFC_PRLO_RLENGTH	0x0014			/* PRLO Response Length		*/
#define HFC_SCR_RLENGTH		0x0004			/* SCR Response Length		*/
#define HFC_LOGO_RLENGTH	0x0004			/* LOGO Response Length		*/
#define HFC_GS_RJT_LENGTH	0x0010			/* FC-GS Reject Length		*/
#define HFC_GCSID_RLENGTH	0x0014			/* GCS_ID Response Length	*/
#define HFC_GIDPN_RLENGTH	0x0014			/* GID_PN Response Length	*/
#define HFC_GPNID_RLENGTH	0x0018			/* GPN_ID Response Length	*/
#define HFC_GIDFT_RLENGTH	0x0800			/* GID_FT Response Length	*/
#define	HFC_GPNFT_RLENGTH	0x8000			/* GPN_FT Response length	*/
#define HFC_RFTID_RLENGTH	0x0010			/* RFT_ID Response Length	*/
#define HFC_RFFID_RLENGTH	0x0010			/* RFF_ID Response Length	*/
		/* type : Response Type */
		union {
			/* PRLI */
			struct {
				uchar	data1[16] ;				/* +0 - +f				*/
				uint	prli_param_resp ;		/* +10 - +13 			*/
				uchar	resv1[8178] ;			/* +14 - +1FFF			*/
			} prli ;
			/* PRLO */
			struct {
				uchar	data1[20] ;				/* +0 - +13				*/
				uchar	resv1[8178] ;			/* +14 - +1FFF			*/
			} prlo ;
			/* SCR */
			struct {
				uchar	data1[8] ;				/* +0 - +7				*/
				uchar	resv1[8184] ;			/* +8 - +1FFF			*/
			} scr ;
			/* LOGO */
			struct {
				uchar	data1[8] ;				/* +0 - +7				*/
				uchar	resv1[8184] ;			/* +8 - +1FFF			*/
			} logo ;
			/* GXX */
			struct {
				union {
					/* GCS_ID */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						uint64_t	class_of_service ;	/* +10 - +17	*/
						uchar		resv1[8168] ;		/* +18 - +1FFF	*/
					} gcs_id ;
					/* GID_PN */
					struct {
						uchar		data1[17] ;			/* +0 - +10 	*/
						uchar		port_id[3] ;		/* +11 - +13	*/
						uchar		resv1[8172] ;		/* +14 - +1FFF	*/
					} gid_pn ;
					/* GPN_ID */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						uint64_t	port_name ;			/* +10 - +17	*/
						uchar		resv1[8168] ;		/* +18 - +1FFF	*/
					} gpn_id ;
#define	HFC_FX_MAX_PORTID 511	/* FCLNX-GPL-FX-154 */
					/* GID_FT */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						struct gidft_port portid[2044];	/* +10  - +1FFF */
					} gid_ft ;
					/* RFT_ID */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						uchar		resv1[8176] ;		/* +10 - +1FFF	*/
					} rft_id ;
					/* RFF_ID */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						uchar		resv1[8176] ;		/* +10 - +1FFF	*/
					} rff_id ;
					/* GPN_FT */
					struct {
						uchar		data1[16] ;			/* +0 - +f 		*/
						struct gpnft_port portid[511] ;	/* +10  - +1FFF	*/
					} gpn_ft ;
				} sub_type;
			} gxx ;
		} type ;
	} receive_payload ;
};

struct rcvfrm_payload_fx {
	uchar		data0[2048];				/* +0 - +2047					*/
};

/*==============================================================================
 *    Payload Defines
 *============================================================================*/
#define	HFC_MBX_REQ_MAX_PAYLOAD	0x0800
#define	HFC_MBX_RSP_MAX_PAYLOAD	0x2000
/*==== PORT_ID Defines =======================================================*/
#define HFC_MBX_PID_DOMAIN	0x00FF0000
#define HFC_MBX_PID_AREA	0x0000FF00
#define HFC_MBX_PID_PORT	0x000000FF
#define HFC_MBX_PID_PORTID	(HFC_MBX_PID_DOMAIN \
	                        |HFC_MBX_PID_AREA \
							|HFC_MBX_PID_PORT)
typedef struct hfc_mbx_byte_acccess_port_id {
	uchar	ctl;
	uchar	pid[3];
}	hfc_mbx_bypid_t;
typedef struct hfc_mbx_affected_port_id {
	uchar	ctl;
	uchar	domain;
	uchar	area;
	uchar	port;
}	hfc_mbx_afpid_t;
typedef struct hfc_mbx_dwd_port_id {
	union {
		ulong			dw_pid;
		hfc_mbx_bypid_t	by_pid;
		hfc_mbx_afpid_t af_pid;
	};
}	hfc_mbx_pid_t;
/*==== Service Parameter Defines =============================================*/
#define HFC_MBX_SP_CREATE_IMAGE_PAIR		0x20
#define HFC_MBX_SP_IMAGE_ESTABLISHED		HFC_MBX_SP_CREATE_IMAGE_PAIR
#define HFC_MBX_SP_TASK_RETRY_ID			0x0200
#define	HFC_MBX_SP_RETRY					0x0100
#define HFC_MBX_SP_CONF_COMPLETION			0x0080
#define HFC_MBX_SP_DATA_OVERLAY				0x0040
#define HFC_MBX_SP_INITIATOR_FUNC			0x0020
#define HFC_MBX_SP_TARGET_FUNC				0x0010
#define HFC_MBX_SP_READ_XFER_RDY_DISABLED	0x0002
#define HFC_MBX_SP_WRITE_XFER_RDY_DISABLED	0x0001

typedef struct hfc_mbp_service_param {
	uchar	type_code;
	uchar	type_code_ex;
	uchar	flags;
	uchar	reserved[11];
	ushort	device_flags;
}	hfc_svc_param_t;
/*==== Payload Common Filed Address ==========================================*/
#define HFC_MBX_PAY_ADDR(P, R)	((uint64_t)(P) + HFC_MBX_##R)
#define HFC_MBX_PAY_OFF
#define HFC_MBX_PY_TYPE			0x0000
 /*==== ELS Payload Address ==================================================*/
#define HFC_MBX_LS_CMDCODE		0x0000
#define HFC_MBX_LS_RSPCODE		0x0000
#define HFC_MBX_LS_REASON		0x0005
#define HFC_MBX_LS_REASONEX		0x0006
typedef struct hfc_mbp_cmn_els {
	uchar  code;
	uchar  page;
	ushort len;
}	hfc_cmn_els_t;
typedef struct hfc_mbp_rsp_ls_rjt {
	hfc_cmn_els_t	els;
	uchar			reserved4;
	uchar			reason;
	uchar			reason_ex;
	uchar			reserved7;
}	hfc_rsp_lsrjt_t;
#define HFC_MBX_LS_RC_LOGERR	0x03
#define HFC_MBX_LS_RC_UABLREQ	0x09
#define HFC_MBX_LS_RC_CMDNSP	0x0B
#define HFC_MBX_LS_RE_NOADD		0x00
#define HFC_MBX_LS_RE_SPERR		0x01
#define HFC_MBX_LS_RE_INVPNAME	0x0D
#define HFC_MBX_LS_RE_INVNNAME	0x0E	
#define HFC_MBX_LS_RE_INVCMNSP	0x0F
#define HFC_MBX_LS_RE_REQNOSUP	0x2F
/*---- PRLI Command ----------------------------------------------------------*/
#define HFC_MBX_LS_PRLI_PAGE	0x0001
#define HFC_MBX_LS_PRLI_PAYL	0x0002
#define HFC_MBX_LS_PRLI_SPPW0	0x0004
#define HFC_MBX_LS_PRLI_SPPW(W)	((uint64_t)(HFC_MBX_LS_PRLI_SPPW0 + (W)* 4))
#define HFC_MBX_LS_PRLI_SPPWB(W, B)	((uint64_t)(HFC_MBX_LS_PRLI_SPPW(W) + (B)))
#define HFC_MBX_LS_PRLI_PARAM	0x0010
#define HFC_NEGO_PRLI_PRAM(REQ, RSP)	( (((REQ) & (RSP)) & 0x0180)\
	                                     |((RSP) & 0x0030)\
										 |0x0002)
typedef struct hfc_mbp_cmn_prli {
	hfc_cmn_els_t	els;
	hfc_svc_param_t	spram;
}	hfc_cmn_prli_t;
/*---- PRLO Command ----------------------------------------------------------*/
#define HFC_MBX_LS_PRLO_PAGE	0x0001
#define HFC_MBX_LS_PRLO_PAYL	0x0002
#define HFC_MBX_LS_PRLO_PARAM	0x0010
typedef struct hfc_mbp_cmn_prlo {
	hfc_cmn_els_t	els;
	hfc_svc_param_t	spram;
}	hfc_cmn_prlo_t;
/*---- LOGO Command ----------------------------------------------------------*/
#define HFC_MBX_LS_LOGO_NPORTID	0x0005
#define HFC_MBX_LS_LOGO_NPORTNM	0x0008
typedef struct hfc_mbp_req_logo {
	hfc_cmn_els_t	els;
	hfc_mbx_pid_t	n_port_id;
	uint64_t		n_port_name;
}	hfc_req_logo_t;
typedef struct hfc_mbp_rsp_logo {
	hfc_cmn_els_t	els;
}	hfc_rsp_logo_t;
/*---- RSCN Command ----------------------------------------------------------*/
#define HFC_MBX_LS_RSCN_PAGE	0x0001
#define HFC_MBX_LS_RSCN_PAYL	0x0002
#define HFC_MBX_LS_RSCN_PORT0	0x0004
#define HFC_MBX_LS_RSCN_PYLMAX	0x0400
#define HFC_MBX_LS_RSCN_PRTMAX	(HFC_MBX_LS_RSCN_PYLMAX - HFC_MBX_LS_RSCN_PORT0)/4
typedef struct hfc_mbp_req_rscn {
	hfc_cmn_els_t	els;
	hfc_mbx_pid_t	port_id_page[HFC_MBX_LS_RSCN_PRTMAX];
}	hfc_req_rscn_t;
typedef struct hfc_mbp_rsp_rscn {
	hfc_cmn_els_t	els;
}	hfc_rsp_rscn_t;
#define HFC_MBX_LS_RSCN_EQ					0x3C000000
#define HFC_MBX_LS_RSCN_EQ_NONE				0x00000000
#define HFC_MBX_LS_RSCN_EQ_CHG_SW_CONFIGG	0x10000000
#define HFC_MBX_LS_RSCN_EQ_CHG_SRVR_OBJ		0x0C000000
#define HFC_MBX_LS_RSCN_EQ_CHG_P_ATTRIBUTE	0x08000000
#define HFC_MBX_LS_RSCN_EQ_CHG_NSRVR_OBJ	0x04000000
#define HFC_MBX_LS_RSCN_AF					0x03000000
#define HFC_MBX_LS_RSCN_AF_FABLIC			0x03000000
#define HFC_MBX_LS_RSCN_AF_DOMAIN			0x02000000
#define HFC_MBX_LS_RSCN_AF_AREA				0x01000000
#define HFC_MBX_LS_RSCN_AF_PORT				0x00000000
#define HFC_MBX_LS_RSCN_DOMAIN				HFC_MBX_PID_DOMAIN
#define HFC_MBX_LS_RSCN_AREA				HFC_MBX_PID_AREA
#define HFC_MBX_LS_RSCN_PORT				HFC_MBX_PID_PORT
/*---- EVFP Command ----------------------------------------------------------*/
#define HFC_MBX_LS_EVFP_SYNC	0x01
#define HFC_MBX_LS_EVFP_COMMIT	0x02
typedef struct hfc_mbp_cmn_evfp {
	hfc_cmn_els_t	els;
	uchar			ver;
	uchar			msg;
	ushort			tid;
	uint64_t		name;
	ushort			rsv;
	ushort			len;
}	hfc_cmn_evfp_t;

/*---- AUTH-ELS Command ------------------------------------------------------*/
#define HFC_MBX_LS_AUTH_REJECT		0x0A
#define HFC_MBX_LS_AUTH_NEGO		0x0B
#define HFC_MBX_LS_AUTH_CHALLENGE	0x10
#define HFC_MBX_LS_AUTH_REPLY		0x11
#define HFC_MBX_LS_AUTH_SUCCESS		0x12
typedef struct hfc_mbp_req_auth {
	uchar			code;
	uchar			flag;
	uchar			msg;
	uchar			ver;
	ulong			len;
	ulong			tid;
}	hfc_req_auth_t;
typedef struct hfc_mbp_rsp_auth {
	uchar			code;
	uchar			flag;
	uchar			msg;
	uchar			ver;
}	hfc_rsp_auth_t;

/*==== FC-GS Payload Address =================================================*/
#define HFC_MBX_GS_CMDCODE		0x0008
#define HFC_MBX_GS_RSPCODE		0x0008
#define HFC_MBX_GS_REASON		0x000D
#define HFC_MBX_GS_REASONEX		0x000E
/* CU_IU header defined in FC-GS				  							  */
#define HFC_MBX_GS_REV_2		0x01
#define HFC_MBX_GS_REV_5		0x03
#define HFC_MBX_GS_TYP_DIR_SERV	0xFC	/* Directory service type			  */
#define HFC_MBX_GS_SUB_N_SERV	0x02	/* Simple Name Server				  */
typedef struct hfc_mbp_cmn_gs {
    uchar		rev;					/* The revision of FC-GS			  */
    uchar		in_id[3];
    uchar		type;					/* Fibre channel service type		  */
    uchar		sub_type;				/* Fibre Channel Service Subtype	  */
    uchar		options;
    uchar		reserved7;				/* Reserved(Offset:0x07)			  */
    ushort		code;					/* Command/Response Code field        */
    ushort		max_resid;
    uchar		flg_id;
    uchar		reason;
    uchar		reason_ex;
    uchar		vend_unq;
} hfc_cmn_gs_t;

/* hfc_fx_mb_passthrough()            */
/* hfc_fx_mb_passthrough_rsp() Response value */
#define EIOF		1
#define ERETRY		2

#endif	/* !INCLUDE_H_HFCW_TOP */

