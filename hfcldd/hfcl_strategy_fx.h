/*
 * hfcl_strategy_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_strategy_fx.h,v 1.1.2.4.2.4.2.2 2015/02/04 08:32:55 toyo Exp $
 */


#ifndef _H_HFCSTRATEGY_FX				/* Double definition prevention */
#define _H_HFCSTRATEGY_FX

/*-- prototype --*/

extern void hfc_fx_cancel_scsi_cmd(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	uint						lun,
	struct hfc_pkt_fx			*hfcp,
	uint						adap_status,
	uchar						inh_altpath,
	uchar						we_que_cancel,
	uchar						tm_que_cancel,
	uchar						type);

extern int hfc_fx_cancel_xob(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	uint						lun,
	struct hfc_pkt_fx			*hfcp,
	uchar						type);

void hfc_fx_cancel_xrb(
	struct port_info		*pp,
	struct target_info_fx	*target,
	uchar					type);
/* FCLNX-GPL-FX-228,272 */

extern void hfc_fx_cancel_weque(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	uint						lun,
	struct hfc_pkt_fx			*hfcp,
	uint						adap_status,
	uchar						inh_altpath,
	uchar						type);

extern void hfc_fx_cancel_wxque(
	struct port_info			*pp,
	struct core_info			*core,
	struct target_info_fx		*target,
	uint						lun,
	struct hfc_pkt_fx			*hfcp,
	uint						adap_status,
	uchar						inh_altpath,
	uchar						type);

extern int hfc_fx_strategy(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
extern int hfc_fx_strategy_pg(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));

extern struct hfc_pkt_fx *hfc_fx_get_new_hfcp(struct port_info *pp);
extern struct hfc_pkt_fx *hfc_fx_get_new_rsv_hfcp(struct port_info *pp);
extern struct hfc_pm_pkt_fx *hfc_fx_get_new_pm_hfcp(struct port_info *pp);
extern int hfc_fx_strategy_port(struct hfc_pkt_fx *hfcp, int core_no); /* FCLNX-GPL-FX-266 */
extern void hfc_fx_strategy_core(struct hfc_pkt_fx *hfcp);

extern int hfc_fx_eh_abort(struct scsi_cmnd *cmnd );
extern int hfc_fx_eh_device_reset(struct scsi_cmnd *cmnd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
extern int hfc_fx_eh_target_reset(struct scsi_cmnd *cmnd);
#endif

extern int hfc_fx_eh_bus_reset(struct scsi_cmnd *cmnd);
extern int hfc_fx_eh_abort_pg(struct scsi_cmnd *cmnd );
extern int hfc_fx_eh_device_reset_pg(struct scsi_cmnd *cmnd);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
extern int hfc_fx_eh_target_reset_pg(struct scsi_cmnd *cmnd);
#endif

extern int hfc_fx_eh_bus_reset_pg(struct scsi_cmnd *cmnd);
extern void hfc_fx_set_cmnd_res(
	struct port_info		*pp,
	struct core_info		*core,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp,
	uint					result );
extern void hfc_fx_iodone(
	struct port_info		*pp,
	struct core_info		*core,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp );
extern int hfc_fx_cmd_retry_check(
	struct port_info		*pp,
	struct scsi_cmnd		*cmnd,
	struct hfc_pkt_fx		*hfcp,
	uchar  					write_retries);	/* FCLNX-GPL-FX-325 */

extern struct hfc_pkt_fx *hfc_fx_refer_reset_buf(struct core_info *core, 
	struct	hfc_pkt_fx *tgt_reset);

extern void hfc_fx_deque_wx_que(struct core_info *core,
	struct hfc_pkt_fx *hfcp);
extern void hfc_fx_enqueue_wx_que(struct core_info *core, struct hfc_pkt_fx *hfcp);

extern void hfc_fx_mq_cancel_scsi_cmd(struct port_info	*pp,
	struct target_info_fx	*target,
	uint					lun,
	struct hfc_pkt_fx		*hfcp,
	uint					adap_status,
	uchar					inh_altpath,
	uchar					cancel_scsi_cmd,
	uchar					wx_que_cancel,
	uchar					xob_cancel,
	uchar					we_que_cancel,
	uchar					tm_que_cancel,
	uchar					type);

/* FCLNX-GPL-FX-274, 285 */
extern void hfc_fx_mq_cancel_xrb(struct port_info	*pp,
	struct target_info_fx	*target,
	uchar					type);

/* FCLNX-GPL-FX-014 Start */
extern int hfc_fx_issue_devrst_cscsi(
	struct port_info            *pp,
	struct target_info_fx       *target,
	struct dev_info_fx          *dev,
	uint						flags);

extern int hfc_fx_issue_tgtrst_cscsi(
	struct port_info            *pp,
	struct target_info_fx       *target,
	struct dev_info_fx          *dev,
	uint						flags);

#define HFC_RST_PRG_RUNNING		1
#define	HFC_RST_PRG_ERROR_END	2
#define	HFC_RST_PRG_NORMAL_END	3

extern int hfc_fx_check_reset_progress(
	struct port_info        *pp,
	struct target_info_fx   *target,
	struct dev_info_fx      *dev);
/* FCLNX-GPL-FX-014 End */
	
extern void hfc_fx_issue_task_mgm(
	struct port_info		*pp,
	struct core_info		*core,	
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	struct dev_info_fx		*dev,
	uchar					mode);

extern void hfc_fx_search_hfcp( struct port_info *pp, 
			struct core_info *core, 
			struct hfc_pkt_fx *hfcp);

//extern void hfc_fx_enqueue_wx(struct core_info *core, struct hfc_pkt_fx *hfcp);
extern struct scsi_cmnd *hfc_fx_get_new_cmnd(struct port_info *pp);
extern void hfc_fx_dummy_copy(struct port_info *pp, struct scsi_cmnd *cmnd, struct scsi_cmnd *dummy_cmnd );

/* iodone request_type */
#define hfc_fx_REQUEST_NEXT_TARGET         0x01
#define hfc_fx_REQUEST_NEXT_LUN            0x02

#define HFC_PP_FX_STATUS_TEST(b ,pp)			(pp->status & (0x00000001 << b))
#define HFC_PP_FX_STATUS_DETAIL1_TEST(b ,pp)	(pp->status_detail1 & (0x00000001 << b))
#define HFC_PP_FX_STATUS_DETAIL2_TEST(b ,pp)	(pp->status_detail2 & (0x00000001 << b))
#define HFC_PP_FX_ATTACH_STATUS_TEST(b ,pp)		(pp->attach_status & (0x00000001 << b))
#define HFC_TG_FX_STATUS_TEST(b, target)		(target->status & (0x00000001 << b))
#define HFC_TG_FX_FLAGS_TEST(b, target)			(target->flags & (0x00000001 << b))
#define HFC_HFCP_FX_CFLAG_TEST(b, hfcp)			(hfcp->cmd_flags & (0x00000001 << b))

extern void hfc_fx_start(struct port_info	*pp,
						struct region_info	*rp,
						struct core_info	*core,
						struct target_info_fx	*target);

extern void hfc_fx_deque_next_dstart(struct port_info		*pp,
									struct region_info		*rp,
									struct core_info		*core,
									struct target_info_fx	*target);

extern void hfc_fx_enque_next_dstart(struct port_info		*pp,
									struct region_info		*rp,
									struct core_info		*core,
									struct target_info_fx	*target);

/************************************************************************/
/* struct free_iov_map													*/
/*        The bit position and the bit length of a continuous iov_map	*/
/*        empty area are set											*/
/*        Return value of hfc_fx_get_free_iov								*/
/************************************************************************/
/*
struct free_iov_map {
	uint	free_cnt ;				* Number of continuous empty areas		*
	uint	free_pos ;				* Head title of continuous empty area 	*
};
*/

#define hfc_fx_GET_WAIT_ABORT_FLAG(t, lun) ((t)->wait_abort[(lun)/8] >> (7 - ((lun) % 8)) & 0x01)
#define hfc_fx_SET_WAIT_ABORT_FLAG(t, lun, val) ((t)->wait_abort[(lun)/8] &= \
	(((val) ? 0x01 : 0x00) << (7 - ((lun) % 8))))
#endif				/* !INCLUDE_HFCSTRATEGY */
