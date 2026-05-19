/*
 * hfcl_handler.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_handler.h,v 1.3.8.8.18.5.6.2.2.3.6.1.2.1.2.1 2015/02/04 08:32:47 toyo Exp $
 */

#ifndef _H_HFCL_INTR
#define _H_HFCL_INTR

extern irqreturn_t hfc_intr(int irq, void *dev_id);

extern void hfc_hand_trace(uchar id, uchar sub_id, struct adap_info *ap, struct target_info *target, uint64_t etc1, uint64_t etc2, uint64_t etc3);


#define HFC_ISSUE_ABORT                 0x01
#define HFC_ISSUE_TARGET_RESET          0x02
#define HFC_ISSUE_LOGIN                 0x03	/* FCLNX-0500 */
/*-- return value of hfc_check_shutdown_flush()     --*/
#define HFC_SHUTDOWN_COMPLETE       0x00
#define HFC_SHUTDOWN_IMCOMPLETE     0x01
#define HFC_SHUTDOWN_SRB_NONE       0x02

/* ---------------------------------------------------------------------------------*/

void hfc_mb_resp(struct adap_info *ap);

extern int io_flush(int x);  /* temp !!*/
extern void hfc_login_resp(struct adap_info *ap);
extern void hfc_pdisc_resp(struct adap_info *ap);
extern void hfc_link_resp(struct adap_info *ap);
extern void hfc_gidft_resp(struct adap_info *ap);
extern void hfc_gidpn_resp(struct adap_info *ap);
extern void hfc_gpnid_resp(struct adap_info *ap);
extern void hfc_mihlog_resp(struct adap_info *ap);
extern void hfc_mb_intreq(struct adap_info *ap);
extern void hfc_linkdown_intreq(struct adap_info *ap, uchar mig);/* FCLNX-GPL-489 */
extern void hfc_linkup_intreq(struct adap_info *ap);
extern void hfc_plogi_intreq(struct adap_info *ap, struct mailbox *mbox);
extern void hfc_logo_intreq(struct adap_info *ap, struct mailbox *mbox);
extern void hfc_scn_intreq(struct adap_info *ap, struct mailbox *mbox);
extern void hfc_rscn_intreq(struct adap_info *ap, struct mailbox *mbox);
extern void hfc_xrb_resp(struct adap_info *ap, uint xrb_out_no, uint xrb_in_no);
extern uint hfc_link_chk(struct adap_info *ap, struct target_info *target, struct hfc_pkt *hfcp);

#define HFC_LINK_CHK_OK            0
#define HFC_LINK_CHK_ERROR         1
#define HFC_LINK_CHK_CCC           2

extern void hfc_scsi_chk(struct adap_info *ap, struct target_info *target, struct hfc_pkt *hfcp);
extern void hfc_task_mgm_chk(struct adap_info *ap, struct hfc_pkt *hfcp);
extern void hfc_notify_tout(struct adap_info *ap, struct target_info *target);
extern void hfc_deque_we_que(struct adap_info *ap, struct target_info *target, struct hfc_pkt *hfcp);
//extern uint hfc_check_shutdown_flush(struct adap_info *ap, struct target_info *target, uchar lun);	/* FCLNX-GPL-0343 */
extern void hfc_mb_errlog(struct adap_info *ap, struct target_info *target, uchar trc_id, uint rc_passthrouh);

extern void hfc_issue_intl_start(struct adap_info *ap,struct target_info *target);

extern uint hfc_frame_a_data(struct mailbox *mbox, uchar type, uchar resp_code);
extern void hfc_core_ce_event(struct adap_info *ap);
extern uint hfc_identify_int_factor(struct adap_info *ap, int vector, int type);
extern void hfc_fw_online_update_complete(struct adap_info *ap); /* FCLNX-GPL-112 */

#endif

