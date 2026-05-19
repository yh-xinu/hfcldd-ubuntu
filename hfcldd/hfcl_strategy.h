/*
 * hfcl_strategy.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_strategy.h,v 1.10.8.7.2.3.12.1.6.2.2.2.6.1.2.1.2.2 2015/02/04 08:32:55 toyo Exp $
 */


#ifndef _H_HFCSTRATEGY				/* Double definition prevention */
#define _H_HFCSTRATEGY

/*-- prototype --*/

extern void hfc_check_bus_status(
	struct adap_info        *ap);

extern void hfc_cancel_scsi_cmd(
	struct adap_info            *ap,
	struct target_info          *target,
	uint                        lun,
	struct hfc_pkt              *hfcp,
	uint                        adap_status,
	uchar                       inh_altpath,
	uchar                       we_que_cancel,
	uchar			   			tm_que_cancel,
	uchar                       type);

extern int hfc_cancel_xob(
	struct adap_info            *ap,
	struct target_info          *target,
	uint                         lun,
	struct hfc_pkt              *hfcp,
	uchar                        type);

extern void hfc_cancel_weque(
	struct adap_info            *ap,
	struct target_info          *target,
	uint                        lun,
	struct hfc_pkt              *hfcp,
	uint                        adap_status,
	uchar                       inh_altpath,
	uchar                       type);

extern void hfc_cancel_wxque(
	struct adap_info            *ap,
	struct target_info          *target,
	uint                        lun,
	struct hfc_pkt              *hfcp,
	uint                        adap_status,
	uchar                       inh_altpath,
	uchar                       type);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
int hfc_strategy_lck(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
#else
int hfc_strategy(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

extern int hfc_strategy_pg(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));

extern struct hfc_pkt *hfc_get_new_hfcp(struct adap_info *ap);
extern int hfc_strategy_hfcp(struct hfc_pkt *hfcp);

extern int hfc_eh_abort(struct scsi_cmnd *cmnd );
extern int hfc_eh_device_reset(struct scsi_cmnd *cmnd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
extern int hfc_lun_reset(struct scsi_cmnd *cmnd);				/* FCLNX-GPL-0343 */
extern int hfc_eh_target_reset(struct scsi_cmnd *cmnd);			/* FCLNX-GPL-0343 */
#endif

extern int hfc_eh_bus_reset(struct scsi_cmnd *cmnd);
extern int hfc_eh_abort_pg(struct scsi_cmnd *cmnd );
extern int hfc_eh_device_reset_pg(struct scsi_cmnd *cmnd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
extern int hfc_eh_target_reset_pg(struct scsi_cmnd *cmnd);		/* FCLNX-GPL-0343 */
#endif

extern int hfc_eh_bus_reset_pg(struct scsi_cmnd *cmnd);
extern void hfc_iodone(struct adap_info *ap, struct scsi_cmnd *cmnd, struct hfc_pkt *hfcp); /* FCLNX-0429 */

extern struct hfc_pkt *hfc_refer_reset_buf(struct target_info *target, 
	struct	hfc_pkt *tgt_reset);

extern void hfc_deque_wx_que(struct target_info *target, 
	struct hfc_pkt *hfcp);
extern void hfc_enqueue_wx_que(struct target_info *target, struct hfc_pkt *hfcp);

	
extern void hfc_issue_task_mgm(
	struct adap_info            *ap,
	struct target_info          *target,
	struct hfc_pkt              *hfcp,
	uint                        lun,
	uchar                       mode);

extern void hfc_search_hfcp( struct adap_info *ap, 
			struct target_info *target, 
			struct hfc_pkt *hfcp);

extern void hfc_enqueue_wx(struct target_info *target, struct hfc_pkt *hfcp);
extern struct scsi_cmnd *hfc_get_new_cmnd(struct adap_info *ap);
extern void hfc_dummy_copy(struct adap_info *ap, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd );

/* iodone request_type */
#define HFC_REQUEST_NEXT_TARGET         0x01
#define HFC_REQUEST_NEXT_LUN            0x02

#define HFC_AP_STATUS_TEST(b,ap)		test_bit(b, (ulong *)&ap->status)
#define HFC_TG_STATUS_TEST(b, target)		test_bit(b, (ulong *)&target->status)
#define HFC_HFCP_CFLAG_TEST(b, hfcp)		test_bit(b, (ulong *)&hfcp->cmd_flags)
#define HFC_TG_FLAGS_TEST(b, target)		test_bit(b, (ulong *)&target->flags)

extern int hfc_start(struct adap_info    *ap ,
               struct target_info  *target,
               struct hfc_pkt      *pkt);



extern void hfc_deque_next_dstart(struct adap_info *ap,struct target_info *target);
extern void hfc_enque_next_dstart(struct adap_info *ap,struct target_info *target);

/************************************************************************/
/* struct free_iov_map													*/
/*        The bit position and the bit length of a continuous iov_map	*/
/*        empty area are set											*/
/*        Return value of hfc_get_free_iov								*/
/************************************************************************/
/*
struct free_iov_map {
	uint	free_cnt ;				* Number of continuous empty areas		*
	uint	free_pos ;				* Head title of continuous empty area 	*
};
*/

#define HFC_GET_WAIT_ABORT_FLAG(t, lun) ((t)->wait_abort[(lun)/8] >> (7 - ((lun) % 8)) & 0x01)
#define HFC_SET_WAIT_ABORT_FLAG(t, lun, val) ((t)->wait_abort[(lun)/8] &= \
	(((val) ? 0x01 : 0x00) << (7 - ((lun) % 8))))
#endif				/* !INCLUDE_HFCSTRATEGY */
