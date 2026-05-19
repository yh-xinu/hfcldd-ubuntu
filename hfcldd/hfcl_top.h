/*
 * hfcl_top.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_top.h,v 1.6.2.7.20.4.6.4.2.2.6.3.2.1.2.1.2.1 2015/03/05 02:19:42 toyo Exp $
 */


#ifndef _H_HFCW_TOP				/* Double definition prevention */
#define _H_HFCW_TOP
/*--------------------------------------------------------------------------*/
/*	   Hitachi FC card adaptor Linux driver  			    */
/*		Top-level processing, Mailbox processing Header file	    */
/*								            */
/*		 NAME : hfcl_top.h					    */
/*									    */
/*--------------------------------------------------------------------------*/
extern void hfc_copy_iocinfo( struct adap_info * );
extern struct target_info *hfc_hash_target_valid( struct adap_info *, uint );
extern struct target_info *hfc_hash_target_info( struct adap_info *, uint );
extern struct target_info *hfc_hash_target_info_wwn(struct adap_info *, uint64_t );
extern struct target_info *hfc_hash_target_info_wwn_no_flag(struct adap_info *, uint64_t );
extern struct target_info *hfc_pseq_target_info(struct adap_info *ap, uint pseq );
extern uint hfc_uniq_seq_num( struct adap_info * );
extern void hfc_release_seq_num( struct adap_info *, int );
extern uint64_t hfc_read_cnfg( struct adap_info *, uint , char );
extern void hfc_write_cnfg( struct adap_info *, uint , char , uint64_t );
extern uint64_t hfc_read_reg_ext(struct adap_info *, uint , char );
extern void hfc_write_reg_ext(struct adap_info *, uint , char , uint64_t );
extern int hfc_read_flash(struct adap_info *ap, int offset, int size, uchar *buf);
extern uint64_t hfc_read_tbl( void *, char );
extern void hfc_write_tbl( void *, char , uint64_t );
extern void lock_mailbox( struct adap_info * );
extern int lock_try_mailbox( struct adap_info * );
extern void unlock_mailbox( struct adap_info * );
extern int lock_try_mpap( struct mp_adap_info *mpap );
extern void hfc_inta_mask_set( struct adap_info *, uint );
extern void start_next_mailbox( struct adap_info * );
extern void hfc_mailbox_initiate( struct adap_info *, uint );
extern int hfc_mailbox_response( struct adap_info *, uchar *, uint * );
extern int hfc_mb_passthrough(struct adap_info *, ushort , ulong ,int, uint );
extern int hfc_mb_passthrough_rsp(struct adap_info *,uint);
extern int hfc_mailbox_proc( struct adap_info *, ushort, ulong, int );
extern int hfc_issue_linkini( struct adap_info * );
extern int hfc_issue_gidft( struct adap_info * );
extern int hfc_issue_gidpn( struct adap_info *, struct target_info * );
extern int hfc_issue_gpnid( struct adap_info *, uint );
extern int hfc_issue_relogin( struct adap_info *,struct target_info * );
extern int hfc_issue_pdisc( struct adap_info *,struct target_info * );
extern int hfc_issue_mihlog( struct adap_info *,
						 struct target_info *, struct hfc_pkt * );
extern int hfc_send_gpnid(struct adap_info *ap);
extern int hfc_send_login(struct adap_info *ap);
extern int hfc_send_pdisc(struct adap_info *ap);
extern void hfc_enque_login_req(struct adap_info *,struct target_info *);
extern void hfc_deque_login_req(struct adap_info *,struct target_info *);
extern void hfc_enque_pdisc_req(struct adap_info *,struct target_info *);
extern void hfc_deque_pdisc_req(struct adap_info *,struct target_info *);
extern void hfc_trace(struct adap_info *,uchar ,uchar *,uchar );
extern int hfc_watchdog_enter( struct adap_info *ap, struct target_info *target,
						struct hfc_pkt *hfcp, uint lun, uchar timer_id, 		/* FCLNX-GPL-0343 */
						uint tout, int cancel);

extern void hfc_check_target_info( struct adap_info * );
extern void hfc_dump_hex( char *,void *, int );

extern void _hfc_sleep_on(wait_queue_head_t *, atomic_t *condition);
extern void _hfc_wake_up(wait_queue_head_t *, atomic_t *condition);
extern void hfc_issue_int_a_rst(struct adap_info *ap, uint int_a_rst, uint int_a_reg);

extern void *hfc_kmalloc(struct adap_info *ap, size_t size, gfp_t flag );
extern void hfc_kfree(struct adap_info *ap, const void *block );
extern void *hfc_dma_alloc_coherent(struct adap_info *ap, struct device *dev,
							size_t size, dma_addr_t *dma_handle, gfp_t gfp);
extern void hfc_dma_free_coherent(struct adap_info *ap, struct device *dev,
							size_t size, void *vaddr, dma_addr_t dma_handle);
extern void *hfc_pci_alloc_consistent(struct adap_info *ap, struct pci_dev *pdev,
							size_t size, dma_addr_t *dma_addrp);
extern void hfc_pci_free_consistent(struct adap_info *ap, struct pci_dev *pdev,
							size_t size, void *cpu_addr, dma_addr_t dma_addr);
extern struct Scsi_Host *hfc_scsi_host_alloc(struct scsi_host_template *sht, int privsize);
extern void hfc_scsi_host_put(struct Scsi_Host *shost);

extern ulong hfc_remap_pci_bar( struct pci_dev *pdev, int bar ); /* FCLNX-GPL-154 */
extern void hfc_unmap_pci_bar( struct pci_dev *pdev, ulong base_addr ); /* FCLNX-GPL-154 */
extern uint64_t hfc_read_reg_ext2(
							struct adap_info *ap,
							ulong base_addr,
							uint offset,
							char reg_size
							); /* FCLNX-GPL-154 */
extern void hfc_write_reg_ext2(
						struct adap_info *ap,
						ulong base_addr,
						uint offset,
						char reg_size,
						uint64_t data
						); /* FCLNX-GPL-154 */

extern uint64_t hfc_read_stat_cca(struct adap_info  *ap,  uint adr);	/* FCLNX-GPL-261 */
extern void hfc_hba_port_statistics_new(struct adap_info  *ap );		/* FCLNX-GPL-261 */

extern struct dev_info *hfc_get_dev_info(struct target_info *target, uint lun);		/* FCLNX-GPL-0343 */

extern int hfc_mp_watchdog_enter( struct adap_info *ap, struct target_info *target,		/* FCLNX-0627 *//* FCLNX-GPL-0343 */
						struct hfc_pkt *hfcp, struct dev_info *dev, uint lun, uchar timer_id, 
						uint tout, int cancel);											/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_clear_dev_info(struct dev_info *dev);									/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_all_clear_dev_info(struct adap_info *ap, struct dev_info *dev);			/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_set_dev_info(struct dev_info *dev);										/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern struct dev_info *hfc_search_dev_info(struct target_info *target, struct hfc_pkt *hfcp);	/* FCLNX-0627 *//* FCLNX-GPL-0343 */
extern void hfc_free_dev(struct target_info *target);					/* FCLNX-GPL-0343 */

/* trace */
#define HFC_TRC_ISSUE_LINKINIT		0x51
#define HFC_TRC_ISSUE_GIDFT			0x52
#define HFC_TRC_ISSUE_GIDPN			0x53
#define HFC_TRC_ISSUE_GPNID			0x54
#define HFC_TRC_ISSUE_RELOGIN		0x55
#define HFC_TRC_ISSUE_PDISC			0x56
#define HFC_TRC_ISSUE_MIHLOG		0x57
#define HFC_TRC_ADD_TARGET			0x5c
#define HFC_TRC_COMMIT_TARGET		0x5d
#define HFC_TRC_DEL_TARGET			0x5e

#define HFC_MBPASS_SUCCESS         0
#define HFC_MBPASS_WAIT_RETRY      1
#define HFC_MBPASS_TIMEDOUT        2
#define HFC_MBPASS_RETRY_OVER      3
#define HFC_MBPASS_RETRY_FAIL      4
#define HFC_MBPASS_ERROR           5
#define HFC_MB_FATAL               6
#define HFC_LINKUPTMR_TO           7
#define HFC_MBPASS_RETRY_OVER_GPNFT 8	/* FCLNX-GPL-FX-139 */

struct top_trc1 {
	uchar             id;					/* +00 */
	uchar             sub_id ;				/* +01 */
	ushort            mb_retry_cnt ;		/* +02 */
	uint              a_status ;			/* +04 */
	uint64_t          a_scsi_id ;			/* +08 */
	uchar             t_flags;				/* +10 */
	uchar             t_pseq ;				/* +11 */
	uchar             mb_status;			/* +12 */
	uchar             resv1;				/* +13 */
//	uint              a_timer_flag;			
//	uint64_t          a_login_target;		
//	uint64_t          a_next_tstart;		
	uchar             a_next_gidpn;			/* +14 */
	uchar             rsv3[2];				/* +15 */
	uchar             t_id;					/* +17 */
	uint              t_status;				/* +18 */
	uint64_t          t_ww_name;			/* +1c */
	uchar             mb_init[64];			/* +24-77 */
};

struct top_trc2 {
	uchar             id;					/* +00 */
	uchar             sub_id ;				/* +01 */
	ushort            rsv1 ;				/* +02 */
	uint              a_status ;			/* +04 */
	uint64_t          a_scsi_id ;			/* +08 */
	uchar             t_flags;				/* +10 */
	uchar             t_pseq ;				/* +11 */
	uchar             t_id ;				/* +12 */
	uchar             t_fc_class ;			/* +13 */
	uint              t_status ;			/* +14 */
	ushort            t_fc_class_mask ;		/* +18 */
	ushort            t_device_flags ;		/* +1a */
	ushort            t_max_frame_size ;	/* +1c */
	uchar             bind_type ;			/* +1e */
	uchar             e_lu ;				/* +1f */
	uint64_t          t_ww_name;			/* +20 */
	uint64_t          t_node_name;			/* +28 */
	uint              t_we_que_cnt;			/* +30 */
	uint              e_lustat ;			/* +34 */
	uchar             rsv[56];				/* +38-6f */
};

/* Macro definition */
/*
 * Function:    hfc_w_start() / hfc_w_stop()
 *
 * Purpose:     Execution of watch dog timer registration/release
 *
 * Arguments:   
 *  ap         - Adap_info structure pointer
 *  timer_id   - TIMER ID
 *
 * Returns:     
 *  0          - Watch_dog registration/cancellation success
 *  1          - Unjustified TIMER ID
 *  2          - It watch_dog registration/already has canceled
 *
 * * context:   user / kernel / interrupt
 *
 * Notes:       (1) TIMER that uses hfc_watchdog_enter the second argument
 *                  cannot use this macro
 */
#define hfc_w_start( _AP, _TIMEID ) (hfc_watchdog_enter( _AP,NULL,NULL,0,_TIMEID,0,0))
#define hfc_w_stop( _AP, _TIMEID )	(hfc_watchdog_enter( _AP,NULL,NULL,0,_TIMEID,0,1))


/*
 * Function:    hfc_read_reg() / hfc_write_reg()
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
 * Notes:       Secure the lock of adap_info
 */
#define hfc_read_reg( _AP, _REGNO, _SIZE ) \
	( hfc_read_reg_ext((_AP), ((struct adap_info*)(_AP))->pkg.map->iosp.reg[_REGNO], (_SIZE)) )

#define hfc_write_reg( _AP, _REGNO, _SIZE, _DATA ) \
	( hfc_write_reg_ext((_AP), ((struct adap_info*)(_AP))->pkg.map->iosp.reg[_REGNO], (_SIZE), (_DATA)) )


/*
 * Function:    hfc_read_val() / hfc_write_val()
 *
 * Purpose:     It makes it from data reading BigEndian from the memory to LittleEndian
 *              It uses it for RD/WR in the communication area (fw_init_tbl, Mailbox, XOB, and XRB, etc.) with F/W
 *              <Differ with hfc_read_tbl >
 *               The RD/WR area automatically acquires the number of bytes for the variable area with sizeof of 
 *               the specified variable
 *
 * Arguments:   
 *  _AAA       - RD/WR variable(substance)
 *  _DATA      - Writing data
 *
 * Returns:     
 *  hfc_read_val  - Data read from variable
 *  hfc_write_val - nothing
 *
 * Notes:       
 */
#define hfc_read_val( _AAA ) \
	(ap->manage_info->pubp->hfc_read_tbl( &(_AAA), sizeof(_AAA)))

#define hfc_write_val( _AAA, _DATA) \
	(hfc_write_tbl( &(_AAA), sizeof(_AAA), _DATA ))



/* hfc_mb_passthrough()            */
/* hfc_mb_passthrough_rsp() Response value */
#define EIOF		1
#define ERETRY		2

#endif	/* !INCLUDE_H_HFCW_TOP */

