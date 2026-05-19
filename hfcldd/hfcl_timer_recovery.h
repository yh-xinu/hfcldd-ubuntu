/*
 * hfcl_timer_recovery.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_timer_recovery.h,v 1.8.2.6.2.4.12.6.6.4.2.3.6.1.2.1.2.3 2015/02/04 08:32:56 toyo Exp $
 */


extern void hfc_hwerr_int(struct adap_info *ap,
					uint int_a_reg,
					uint status_reg0,
					uint status_reg1,
					uint detail_reg);

extern void hfc_errlog( struct adap_info *ap,
				struct target_info	*target,
				struct hfc_pkt		*hfcp,
				uchar				type,
				uint				err_id,
				uint				err_num,
				uchar				*data,
				ushort				data_len );

extern void hfc_abend(
	struct adap_info        *ap,
	uchar                   type);

extern void hfc_logout(                                                /* FCWIN-0220 */
	struct adap_info        *ap,
	uint                    errno,
	uchar                   mode);

extern void hfc_fpp_logout( struct adap_info *ap, uint errno, uchar mode );

extern void hfc_five_logout( struct adap_info *ap, uint errno, uchar mode );

extern void hfc_five_ex_logout( struct adap_info *ap, uint errno, uchar mode );

extern void hfc_save_pcie_sram_log(struct adap_info *ap);

extern int hfc_pcibus_chk(struct adap_info *ap);

extern void hfc_mck_recovery(struct adap_info *ap, uchar type); /* FCLNX-GPL-209 */

extern void hfc_mck_recovery_fpp(struct adap_info *ap,uchar type);

extern void hfc_mck_recovery_five(struct adap_info *ap,uchar type);

extern int hfc_mck_port_recovery(struct adap_info	*ap);

extern void hfc_chk_stop(struct adap_info *ap, uchar lock);		/* FCLNX-0279 */

extern void hfc_watchdog(struct wtimer *w_timer);

extern void hfc_reset_start(struct adap_info *,uchar); 

extern void hfc_reset_adap_info(struct adap_info *);

extern int hfc_toutchk_xob(
	struct adap_info	*ap,
	struct target_info	*target,
	struct hfc_pkt		*hfcp,    
	uint				lun,									/* FCLNX-GPL-0343 */
	uchar				mode);

extern void hfc_hand2_trace(
	uchar               id,
	uchar               sub_id,
	struct adap_info    *ap,
	struct target_info  *target,
	struct hfc_pkt		*hfcp,
	uint64_t            etc1,
	uint64_t            etc2,
	uint64_t            etc3);

extern void hfc_reset_watchdog(struct adap_info *ap);

extern int hfc_issue_forced_mck(struct adap_info *ap, uchar type);

extern int hfc_sfp_port_recovery(struct adap_info *ap, int immdt_cmd);		/* FCLNX-0514*/
extern void hfc_occurred_mck(struct adap_info *ap, uchar point);	 /* FCLNX-0535 */

extern void hfc_clear_sticky_bit(struct adap_info *ap);
extern void hfc_pcie_sram_ce_recovery(struct adap_info *ap);
extern void hfc_set_sram_ce_log(struct adap_info *ap);
extern int hfc_force_linkdown( struct adap_info *ap , uchar proc, uchar skip);		/* FCLNX_GPL-105 */	/* FCLNX-GPL-147 */
extern int hfc_force_linkdown_recovery(struct adap_info *ap);						/* FCLNX_GPL-105 */	/* FCLNX-GPL-147 */
extern int hfc_force_linkdown_recovery_port(struct adap_info *ap);					/* FCLNX_GPL-105 */	/* FCLNX-GPL-147 */	/* FCLNX-GPL-402 */
extern void hfc_mck_prepare(struct adap_info *ap); /* FCLNX-GPL-209 */

extern void hfc_clear_errinfo_i( struct adap_info *ap );	/* FCLNX_GPL-xxx */
extern int hfc_get_isolparam_i( struct adap_info *ap, struct hfc_isol_info *isolinfo, uchar pcm );	/* FCLNX_GPL-393 */
extern void hfc_check_errcount(struct adap_info *ap);	/* FCLNX_GPL-xxx */
extern void hfc_watched_errcount_i(struct adap_info *ap, struct target_info *target, uchar err_flag);
extern int hfc_check_cmnd_timeout(struct adap_info *ap, struct target_info *target);	/* FCLNX_GPL-430 */

#if 0

/* FCLNX-GPL-547,563 start */
#if defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG)

static enum log_files
hfc_log_file_selector(uint err_id, uchar f_select, enum log_parts log_part)
{
	enum log_files log_file = HFC_SYSLOG;
	uchar alarm             = FALSE;

	/* Table size check */
	if( err_id  < ERRID_HFCP_TBL_END )
	{	/* Table size is correct. */
		alarm = alarm_tbl[err_id];
	}
	
	/* Table size check */
	if( f_select < HFC_MAX_LOG_FILE_SELECTS )
	{	/* Table size is correct. */
		/* Select Non-Alarm Log or Alarm Log Table */
		if( alarm == TRUE ) {
			log_file = log_file_tbl_alm[f_select][log_part];
		} else {
			log_file = log_file_tbl_non[f_select][log_part];
		}
	}
	
	return ( log_file );
}

#define HFC_PRT_COM(PRT_FUNC, err_id, f_select, log_part, fmt, args...)	\
{									\
	switch( hfc_log_file_selector(err_id, f_select, log_part) )	\
	{								\
		case HFC_SYSLOG:					\
			PRT_FUNC(fmt, ##args);				\
			break;						\
		case HFC_DRV_LOG:					\
			printk(KERN_DRV_INFO fmt, ##args);		\
			break;						\
		case HFC_ALL_LOG:					\
		default:						\
			PRT_FUNC(fmt, ##args);				\
			printk(KERN_DRV_INFO fmt, ##args);		\
			break;						\
	}								\
}
#endif

#define HFC_ERRPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_ERRPRT, err_id, f_select, log_part, fmt, ##args)

#define HFC_WRNPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_WRNPRT, err_id, f_select, log_part, fmt, ##args)

#define HFC_INFPRT_S(err_id, f_select, log_part, fmt, args...)		\
	HFC_PRT_COM(HFC_INFPRT, err_id, f_select, log_part, fmt, ##args)

#else /* defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG) */

#define HFC_ERRPRT_S(err_id, f_select, log_part, fmt, args...) HFC_ERRPRT(fmt, ##args)
#define HFC_WRNPRT_S(err_id, f_select, log_part, fmt, args...) HFC_WRNPRT(fmt, ##args)
#define HFC_INFPRT_S(err_id, f_select, log_part, fmt, args...) HFC_INFPRT(fmt, ##args)

#endif /* defined(HFC_STAR) && defined(CONFIG_NAS_SCSI_DRVLOG) */
/* FCLNX-GPL-547,563 end */


