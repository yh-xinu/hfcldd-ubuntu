/*
 * hfcl_handler_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_handler_fx.h,v 1.1.2.7.2.4.2.2 2015/02/04 08:32:48 toyo Exp $
 */

#ifndef _H_HFCL_INTR_FX
#define _H_HFCL_INTR_FX

/* extern irqreturn_t hfc_fx_intr(int irq, void *dev_id); */
/* extern irqreturn_t hfc_fx_intr_msi(int irq, void *dev_id); */
extern irqreturn_t hfc_fx_intr_xrb(int irq, void *intr_entry);
extern irqreturn_t hfc_fx_intr_share(int irq, void *intr_entry);

extern void hfc_fx_hand_trace(uchar id, uchar sub_id, struct port_info *pp, struct target_info_fx *target, uint64_t etc1, uint64_t etc2, uint64_t etc3);


#define HFC_ISSUE_ABORT                 0x01
#define HFC_ISSUE_TARGET_RESET          0x02
#define HFC_ISSUE_LOGIN                 0x03	/* FCLNX-0500 */
#define HFC_ISSUE_LUN_RESET             0x04
#define HFC_ISSUE_CSCSI_LU_WITHOUT_DMA  0x05	/* FCLNX-GPL-FX-014 */
#define HFC_ISSUE_CSCSI_LU_WAIT_DMA     0x06	/* FCLNX-GPL-FX-014 */
#define HFC_ISSUE_CSCSI_TGT_WITHOUT_DMA 0x07	/* FCLNX-GPL-FX-014 */
#define HFC_ISSUE_CSCSI_TGT_WAIT_DMA    0x08	/* FCLNX-GPL-FX-014 */
/*-- return value of hfc_fx_check_shutdown_flush()     --*/
#define HFC_SHUTDOWN_COMPLETE       0x00
#define HFC_SHUTDOWN_IMCOMPLETE     0x01
#define HFC_SHUTDOWN_SRB_NONE       0x02

/*-- response code value of hfc_fx_xrp_resp() --*/
#define HFC_XRBRESP_IOCMD_NORMAL	0x00
#define HFC_XRBRESP_IOCMD_LINK_ERR	0x01
#define HFC_XRBRESP_TMCMD_NORMAL	0x02
#define HFC_XRBRESP_TMCMD_ERR		0x04
#define HFC_XRBRESP_TMCMD_LINK_ERR	0x08
#define HFC_XRBRESP_ABEND			0x10
#define HFC_XRBRESP_INTERNAL_RETRY	0x20


/* ---------------------------------------------------------------------------------*/

void hfc_fx_mb_resp(struct port_info *pp, struct core_info *core, struct region_info *rp);

extern int io_flush(int x);  /* temp !!*/
extern void hfc_fx_link_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_mihlog_resp(struct port_info *pp, struct core_info *core);
extern int hfc_fx_xrb_resp(struct port_info *pp, struct region_info *rp, struct core_info *core, uint64_t time, uint cpuno, struct hfc_pkt_fx *(*wait_iodone_hfcp));
extern uint hfc_fx_link_chk(struct port_info *pp, struct target_info_fx *target, struct hfc_pkt_fx *hfcp, struct xrb_fx *xrb);

extern void hfc_fx_add_port_id_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_delete_port_id_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_load_ch_trace_log_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_cancel_scsi_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_core_start_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_offline_mb_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_diag_resp(struct port_info *pp, struct core_info *core );	/* FCLNX-GPL-FX-126 */
extern void hfc_fx_shadow_up_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_flogi_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_plogi_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_plogi_fabric_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_plogi_target_resp(struct port_info *pp, struct core_info *core );
extern void hfc_fx_pdisc_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_frmsndrcv_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_prli_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_prlo_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_scr_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_logo_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_gcs_id_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_gid_pn_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_gpn_id_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_gid_ft_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_rft_id_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_rff_id_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_gpn_ft_resp(struct port_info *pp, struct core_info *core);
extern void hfc_fx_mb_intreq(struct port_info *pp, struct core_info *core, struct region_info *rp);
extern void hfc_fx_correctabled_error_intreq(struct port_info *pp, struct core_info *core);
extern void hfc_fx_comp_online_update_intreq(struct port_info *pp, struct core_info *core);
extern void hfc_fx_linkdown_intreq(
	struct port_info *pp,
	struct core_info *core,
	uchar	mig);
extern void hfc_fx_linkup_intreq(	struct port_info        *pp,
	struct core_info		*core);
extern void hfc_fx_receive_flogi_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern void hfc_fx_receive_plogi_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern void hfc_fx_receive_pdisc_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern void hfc_fx_receive_frame_intreq(struct port_info	*pp,
	struct core_info	*core,
	struct mailbox_fx	*mbox);
extern uchar hfc_fx_prli_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern uchar hfc_fx_prlo_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern uchar hfc_fx_rscn_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);
extern uchar hfc_fx_logo_intreq(struct port_info        *pp,
	struct core_info		*core,
	struct mailbox_fx       *mbox);

#define HFC_LINK_CHK_OK            0
#define HFC_LINK_CHK_ERROR         1
#define HFC_LINK_CHK_CCC           2

extern int hfc_fx_scsi_chk(struct port_info *pp, struct target_info_fx *target, struct hfc_pkt_fx *hfcp, struct xrb_fx *xrb);	/* FCLNX-GPL-FX-332 */
extern int hfc_fx_task_mgm_chk(struct port_info *pp, struct hfc_pkt_fx *hfcp, struct xrb_fx *xrb);
extern void hfc_fx_notify_tout(struct port_info *pp, struct core_info *core, struct target_info_fx *target, uint lun, uchar type);	/* FCLNX-GPL-0596 */
extern void hfc_fx_deque_we_que(struct port_info *pp, struct target_info_fx *target, struct hfc_pkt_fx *hfcp);
extern void hfc_fx_mb_errlog(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
	uchar trc_id, uint rc_passthrouh);

extern void hfc_fx_timeout_by_mb_delay(struct port_info *pp);
extern void hfc_fx_timeout_by_mb_intvl(struct port_info *pp, struct core_info *core);
extern void hfc_fx_timeout_by_mailbox(struct port_info *pp, struct core_info *core);

extern void hfc_fx_issue_intl_start(struct port_info *pp, struct region_info *rp, struct core_info *core, struct target_info_fx *target);
extern uint hfc_fx_frame_a_data(struct mailbox_fx *mbox, uchar type, ushort frame_type, uchar rid);
extern void hfc_fx_core_ce_event(struct port_info *pp);
extern uint hfc_fx_identify_int_factor(struct port_info *pp, int vector, int type);
extern void hfc_fx_fw_online_update_complete(struct port_info *pp);
extern int hfc_fx_check_linkresp_param(struct port_info *pp, struct core_info *core);
extern struct port_info *hfc_fx_get_port_info_by_portid(struct port_info *pp, struct region_info *rp, struct core_info *core);

#endif

