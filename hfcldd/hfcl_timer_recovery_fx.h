/*
 * hfcl_timer_recovery_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_timer_recovery_fx.h,v 1.1.2.9.2.2.2.1 2015/02/04 08:32:56 toyo Exp $
 */


extern void hfc_fx_hwerr_int(struct port_info *pp,
					uint int_a_reg,
					uint status_reg0,
					uint status_reg1,
					uint detail_reg,
					uchar abend_code);

extern void hfc_fx_errlog( struct port_info *pp,
				struct core_info		*core,
				struct target_info_fx	*target,
				struct hfc_pkt_fx		*hfcp,
				uchar					type,
				uint					err_id,
				uint					err_num,
				uchar					*data,
				ushort					data_len );

extern void hfc_fx_abend(
	struct port_info        *pp,
	struct core_info		*core,
	uchar                   type);

extern void hfc_fx_logout(
	struct port_info        *pp,
	uint                    errno,
	uchar                   mode);

extern void hfc_fx_save_hwlog_five_fx(
	struct port_info *pp, 
	struct core_info *core, 
	uint hfc_errno, 
	uchar mode);

extern void hfc_fx_save_pcie_sram_log(struct port_info *pp);

extern int hfc_fx_pcibus_chk(struct port_info *pp);

extern void hfc_fx_mck_recovery(struct port_info *pp, uchar type); 

extern void hfc_fx_mck_recovery_five_fx(struct port_info *pp,uchar type);

extern int hfc_fx_mck_port_recovery(struct port_info	*pp);

extern void hfc_fx_chk_stop(struct port_info *pp);

/* kernel 4.15+: timer callback uses struct timer_list * */
extern void hfc_fx_watchdog(struct timer_list *t);

extern void hfc_fx_reset_start(struct port_info *,uchar); 

extern void hfc_fx_reset_port_info(struct port_info *);

extern int hfc_fx_toutchk_xob(
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,    
	uint					lun,
	uchar					mode);

extern void hfc_fx_hand2_trace(
	uchar					id,
	uchar					sub_id,
	struct port_info		*pp,
	struct region_info		*rp,
	struct core_info		*core,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp,
	uint64_t				etc1,
	uint64_t				etc2,
	uint64_t				etc3);

extern void hfc_fx_reset_watchdog(struct port_info *pp);

extern int hfc_fx_issue_forced_mck(struct port_info *pp, struct core_info *core, uchar type);

extern int hfc_fx_sfp_port_recovery(struct port_info *pp, int immdt_cmd);
extern void hfc_fx_occurred_mck(struct port_info *pp, uchar point);

extern void hfc_fx_clear_sticky_bit(struct port_info *pp);
extern void hfc_fx_pcie_sram_ce_recovery(struct port_info *pp);
extern void hfc_fx_set_sram_ce_log(struct port_info *pp);
extern int hfc_fx_force_linkdown( struct port_info *pp , uchar proc);		/* FCLNX-GPL-FX-043 */
extern int hfc_fx_force_linkdown_recovery(struct port_info *pp);
extern void hfc_fx_mck_prepare(struct port_info *pp, uchar type);

extern void hfc_fx_clear_errinfo_i( struct port_info *pp );
extern int hfc_fx_get_isolparam_i( struct port_info *pp, struct hfc_isol_info *isolinfo, uchar pcm );	/* FCLNX_GPL-393 */
extern int hfc_fx_check_errcount(struct port_info *pp);	/* FCLNX_GPL-xxx */
extern void hfc_fx_watched_errcount_i(struct port_info *pp, struct target_info_fx *target, uchar err_flag);
extern int hfc_fx_check_cmnd_timeout(struct port_info *pp, struct core_info *core, struct target_info_fx *target, ushort *lun);	/* FCLNX_GPL-430 *//* FCLNX-GPL-FX-014 */

extern void hfc_fx_error_common(struct port_info *pp, struct core_info *core, hfc_fx_errfmt1_t *err1, struct hfc_pkt_fx *hfcp);
extern void hfc_fx_errsave( struct port_info *pp,
				  int errlog_info_p,	
				  uint err_num,		
				  struct hfc_err_rec *err_rec,
				  uint cnt,
				  uint offset);
extern uint hfc_fx_raslog( struct port_info *pp,
				  struct core_info *core,
				  struct hfc_err_rec *err_rec,
				  char 	*ras_error_id1,
				  char 	*ras_error_id2,
				  char	 *resource_name,
				  struct hfc_pkt_fx		*hfcp,
				  struct scsi_cmnd		*cmnd,
				  uchar	 type);

extern void hfc_fx_timeout_by_scnlinkup(
	struct port_info		*pp,
	struct target_info_fx	*target);

extern void hfc_fx_timeout_by_reset(
	struct port_info		*pp,
	struct target_info_fx	*target,
	struct hfc_pkt_fx		*hfcp);
extern void hfc_fx_timeout_by_restart(
	struct port_info        *pp,
	struct target_info_fx 	*target,
	struct dev_info_fx		*dev,
	struct hfc_pkt_fx		*hfcp);

extern void hfc_fx_change_vport_isol_state(struct port_info *pp);
extern int hfc_fx_skip_vport_errlog(struct port_info *pp, uint err_num);
extern void hfc_fx_wdog_linkup_tmr(struct port_info *pp, struct core_info *core, ushort timer_id);
