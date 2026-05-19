/*
 * hfcl_detect.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_detect.h,v 1.16.2.8.2.2.4.1.6.13.2.2.2.6.2.10.2.8.2.1.2.2.2.2 2015/04/21 11:49:02 toyo Exp $
 */


#ifndef _H_HFCL_DETECT
#define _H_HFCL_DETECT

#define HFC_ALIGN08( _AAA ) (((_AAA)+0x7) & ~0x7)
#define HFC_ALIGN16( _AAA ) (((_AAA)+0xF) & ~0xF)
#define HFC_ALIGNPG( _AAA ) ((((_AAA)+(HFC_PAGE_SIZE-1))/HFC_PAGE_SIZE)*HFC_PAGE_SIZE)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
extern int hfc_probe_one(struct pci_dev *pdev, const struct pci_device_id *id);
extern void hfc_remove_one(struct pci_dev *pdev);
#else
extern int __devinit hfc_probe_one(struct pci_dev *pdev, const struct pci_device_id *id);
extern void __devexit hfc_remove_one(struct pci_dev *pdev);
#endif

extern int hfc_ex_probe_one(struct pci_dev *pdev, const struct pci_device_id *id);
extern void hfc_ex_remove_one(struct pci_dev *pdev);
extern int hfc_biosparam(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int geom[]);

extern int hfc_proc_info_k26(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
extern int hfc_proc_info_pfb(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
extern int hfc_release(struct Scsi_Host *host);
extern const char *hfc_info(struct Scsi_Host *host);

extern int hfc_clear_target_info( struct adap_info *ap, struct target_info *target, int check, int pmsg );

extern int hfc_attach(struct adap_info *ap);
extern int hfc_pci_conf(struct adap_info *ap);

extern int hfc_allocate_dma(struct adap_info *ap);
extern int hfc_free_dma(struct adap_info *ap);

extern int hfc_query_pktype(struct adap_info *ap);
extern int hfc_start_adapter(struct adap_info *ap);

extern void hfc_get_bindtid(struct adap_info *ap);
extern int hfc_initialize(struct adap_info *ap, int immdt_cmd);
extern int hfc_check_inta(struct adap_info *ap);

extern int hfc_config_hw_set(struct adap_info *ap, uint retry_maxcnt);
extern int hfc_config_hw_set_five(struct adap_info *ap, uint retry_maxcnt);

extern void hfc_wwnverify_linkup (struct adap_info *ap, struct target_info *target, uint mb_resp_status, uint64_t ww_name);
extern void hfc_wwnverify_linkup_timeout(struct adap_info *ap, struct target_info *target, uint mb_resp_status);
extern void hfc_wwnverify_gidft(struct adap_info *ap, struct target_info *target, uint mb_resp_status);
extern void hfc_wwnverify_gpnid(struct adap_info *ap, struct target_info *target, uint mb_resp_status);
extern void hfc_wwnverify_login(struct adap_info *ap, struct target_info *target, uint mb_resp_status, uint64_t ww_name);
extern void hfc_wwnverify_scn(struct adap_info *ap, struct target_info *target, uint mb_resp_status);

extern struct target_info *hfc_add_target_info( struct adap_info *ap, uint64_t scsi_id );


extern int hfc_chk_conf_val(int min, int max, int val);
extern int hfc_chk_conf_ls(int min, int max, int val);
extern int hfc_chk_conf_mt(int min, int max, int val);

extern void hfc_conf_setup(struct adap_info *ap);

extern void hfc_set_topology(struct adap_info *ap);
extern void hfc_set_linkspeed(struct adap_info *ap);
extern void hfc_set_max_transfer(struct adap_info *ap);
extern void hfc_set_linkup_tmo(struct adap_info *ap);
extern void hfc_set_linkup2_tmo(struct adap_info *ap);
extern void hfc_set_reset_delay(struct adap_info *ap);
extern void hfc_set_mck_count(struct adap_info *ap);
extern void hfc_set_pref_alpa(struct adap_info *ap);
extern void hfc_set_target_timeout(struct adap_info *ap);
extern void hfc_set_abort_timeout(struct adap_info *ap);
extern void hfc_set_seg_trace(struct adap_info *ap);
extern void hfc_set_queue_depth(struct adap_info *ap);
extern void hfc_set_enable_target_reset(struct adap_info *ap);
extern void hfc_set_max_target(struct adap_info *ap);
extern void hfc_set_xob_max(struct adap_info *ap);
extern void hfc_set_xrb_max(struct adap_info *ap);
extern void hfc_set_slog_max(struct adap_info *ap);
extern void hfc_set_trc_max(struct adap_info *ap);
extern void hfc_set_pkt_num(struct adap_info *ap);
extern void hfc_set_can_queue(struct adap_info *ap);
extern void hfc_set_sg_tblsize(struct adap_info *ap);
extern void hfc_set_cmnd_num(struct adap_info *ap);
extern void hfc_set_minus_tout(struct adap_info *ap);
extern void hfc_set_scsi_allowed(struct adap_info *ap);
extern void hfc_set_login_retry(struct adap_info *ap);		/* FCLNX-GPL-0343 */
extern void hfc_set_els_retry(struct adap_info *ap);		/* FCLNX-GPL-0343 */
extern void hfc_set_ioctl_scsi_timeout(struct adap_info *ap);	/* FCLNX-GPL-0343 */
extern void hfc_set_cmd_per_lun(struct adap_info *ap);
extern void hfc_set_max_sectors(struct adap_info *ap);
extern void hfc_set_vary_io(struct adap_info *ap);
extern void hfc_set_lun_reset_delay(struct adap_info *ap);
extern void hfc_set_abort_t_restrain(struct adap_info *ap);
extern void hfc_set_login_restrain(struct adap_info *ap);
extern void hfc_set_mck_point(struct adap_info *ap);
extern void hfc_set_msi_enable(struct adap_info *ap);
extern void hfc_set_pcie_sram_ce_count(struct adap_info *ap);
extern void hfc_set_core_ce_count(struct adap_info *ap);
extern void hfc_set_inta_dummy_read(struct adap_info *ap);
extern void hfc_set_max_hwlog_cnt(struct adap_info *ap);
extern void hfc_set_debug_func(struct adap_info *ap);
extern void hfc_set_issue_d3hot(struct adap_info *ap);
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_set_sysfs_control(struct adap_info *ap);
extern void hfc_set_dev_loss_tmo(struct adap_info *ap);
extern void hfc_set_scan_finished_tmo(struct adap_info *ap);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
extern void hfc_set_hba_isolation(struct adap_info *ap);	/* FCLNX-GPL-349 */
extern void hfc_set_ld_err_limit_s(struct adap_info *ap);	/* FCLNX-GPL-349 */
extern void hfc_set_if_err_limit(struct adap_info *ap);		/* FCLNX-GPL-349 */
extern void hfc_set_to_err_limit(struct adap_info *ap);		/* FCLNX-GPL-349 */
extern void hfc_set_to_reset_retry(struct adap_info *ap);	/* FCLNX-GPL-349 */
extern void hfc_set_rt_err_enable(struct adap_info *ap);	/* FCLNX-GPL-349 */
extern void hfc_set_limit_log(struct adap_info *ap);		/* FCLNX-GPL-491 */
extern void hfc_set_filter_target(struct adap_info *ap);	/* FCLNX-GPL-491 */
extern void hfc_set_hg_stats_disable(struct adap_info *ap);	/* FCLNX-GPL-494 */
extern void hfc_set_rport_lu_scan(struct adap_info *ap);	/* FCLNX-GPL-575 */
extern void hfc_set_ctl_change_qdepth(struct adap_info *ap);/* FCLNX-GPL-574 */

extern int hfc_search_adapter_number(struct adap_info *ap);
extern int hfc_get_adapter_bindings(void);
extern char hfc_cnvc(char C);
extern int hfc_parse_string(char *string, char *keyword, uint64_t *value);
extern int hfc_convert_string(char *string, uint64_t *value);
//extern char *hfc_get_write_retries(void);					/* FCLNX-GPL-0449 */
extern void hfc_set_pub_symbol_list(void);
extern int hfc_check_nonpub_symbol(void);

extern void hfc_free_mpap(struct adap_info *ap);
extern void hfc_param_setup(void);
extern int hfc_param_search(char *search_str, int *value);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_rport_add(struct adap_info *ap, struct target_info *target);
extern void hfc_rport_delete(struct target_info *target);
extern void hfc_fc_host_init(struct Scsi_Host *host, struct adap_info *ap);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

extern void hfc_reset_all_timer (struct adap_info *ap);

extern int hfc_set_interrupts(struct adap_info *ap, int type);
extern void hfc_free_interrupts(struct adap_info *ap, int type, int pci_fail);	/* FCLNX-GPL-306 */
extern void hfc_send_msi_info(struct adap_info *ap);
extern void hfc_set_msix_table(struct adap_info *ap);
extern void hfc_reset_msix_table(struct adap_info *ap);
extern void hfc_change_msix_addr(struct adap_info *ap, uint addr);
extern void hfc_clear_status_five_ex(struct adap_info *ap);
extern void hfc_set_mcw_five_ex(struct adap_info *ap);
extern int hfc_config_hw_set_five_ex(struct adap_info *ap, uint retry_maxcnt);
extern int hfc_query_devid(struct adap_info *ap);

extern uint hfc_get_sysrev( struct adap_info *ap); /* FCLNX-GPL-112 */
extern struct pci_dev *hfc_get_slot_dev(struct adap_info *ap); /* FCLNX-GPL-180 */
extern uint64_t hfc_restore_add_wwn(struct adap_info *ap); /* FCLNX-GPL-201 */
extern int hfc_backup_add_wwn(struct adap_info *ap, uint64_t add_ww_name); /* FCLNX-GPL-201 */
extern int hfc_pcie_link_width_chk(struct adap_info *ap); /* FCLNX-GPL-227 */
extern int hfc_get_vpd(struct adap_info *ap, uchar *vpd_buf);	/* FCLNX-GPL-489 */

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_kthread_stop(struct adap_info *ap);
extern int hfc_do_rport(void *p);
extern void hfc_start_rport( struct adap_info *ap );				/* FCLNX-GPL-306 */
extern void hfc_stop_rport(struct adap_info *ap);					/* FCLNX-GPL-306 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

//extern int hfc_suspend_one(struct pci_dev *pdev, pm_message_t msg); /* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
//extern int hfc_resume_one(struct pci_dev *pdev); 					/* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
extern void hfc_set_hw_mcw_cfg(struct adap_info *ap); 				/* FCLNX-GPL-306 */
extern void hfc_set_hw_mcw_pci(struct adap_info *ap); 				/* FCLNX-GPL-306 */
extern int hfc_skip_link_init(struct adap_info *ap, ushort bind_err);	/* FCLNX-GPL-306 */
//extern void hfc_suspend_driver(struct adap_info *ap, struct Scsi_Host *host);	/* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
//extern int hfc_suspend_pci(struct adap_info *ap, struct pci_dev *pdev, int pci_fail);	/* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
//extern void hfc_resume_driver(struct adap_info *ap, struct Scsi_Host *host);	/* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
//extern int hfc_resume_pci(struct adap_info *ap, struct pci_dev *pdev);	/* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
extern char *hfc_delete_space(char *string);
extern int hfc_check_hba_isolation(struct adap_info *ap);			/* FCLNX-GPL-349 */
extern int hfc_get_adap_status(struct adap_info *ap);				/* FCLNX-GPL-428 */


/* FIVE-FX */
extern void hfc_fx_set_topology(struct port_info *pp);
extern void hfc_fx_set_linkspeed(struct port_info *pp);
extern void hfc_fx_set_max_transfer(struct port_info *pp);
extern void hfc_fx_set_linkup_tmo(struct port_info *pp);
//extern void hfc_fx_set_linkup2_tmo(struct port_info *pp);
extern void hfc_fx_set_reset_delay(struct port_info *pp);
extern void hfc_fx_set_mck_count(struct port_info *pp);
extern void hfc_fx_set_pref_alpa(struct port_info *pp);
extern void hfc_fx_set_target_timeout(struct port_info *pp);
extern void hfc_fx_set_abort_timeout(struct port_info *pp);
extern void hfc_fx_set_seg_trace(struct port_info *pp);
extern void hfc_fx_set_queue_depth(struct port_info *pp);
extern void hfc_fx_set_enable_target_reset(struct port_info *pp);
extern void hfc_fx_set_max_target(struct port_info *pp);
extern void hfc_fx_set_xob_max(struct port_info *pp);
extern void hfc_fx_set_xrb_max(struct port_info *pp);
extern void hfc_fx_set_slog_max(struct port_info *pp);
extern void hfc_fx_set_trc_max(struct port_info *pp);
extern void hfc_fx_set_pkt_num(struct port_info *pp);
extern void hfc_fx_set_rsv_pkt_num(struct port_info *pp);
extern void hfc_fx_set_pm_pkt_num(struct port_info *pp);
extern void hfc_fx_set_can_queue(struct port_info *pp);
extern void hfc_fx_set_sg_tblsize(struct port_info *pp);
extern void hfc_fx_set_cmnd_num(struct port_info *pp);
extern void hfc_fx_set_minus_tout(struct port_info *pp);
extern void hfc_fx_set_scsi_allowed(struct port_info *pp);
extern void hfc_fx_set_login_retry(struct port_info *pp);
extern void hfc_fx_set_els_retry(struct port_info *pp);
extern void hfc_fx_set_ioctl_scsi_timeout(struct port_info *pp);
extern void hfc_fx_set_cmd_per_lun(struct port_info *pp);
extern void hfc_fx_set_max_sectors(struct port_info *pp);
extern void hfc_fx_set_vary_io(struct port_info *pp);
extern void hfc_fx_set_lun_reset_delay(struct port_info *pp);
extern void hfc_fx_set_abort_t_restrain(struct port_info *pp);
extern void hfc_fx_set_login_restrain(struct port_info *pp);
extern void hfc_fx_set_tgtrst_restrain(struct port_info *pp);
extern void hfc_fx_set_mck_point(struct port_info *pp);
extern void hfc_fx_set_msi_enable(struct port_info *pp);
extern void hfc_fx_set_pcie_sram_ce_count(struct port_info *pp);
extern void hfc_fx_set_core_ce_count(struct port_info *pp);
extern void hfc_fx_set_inta_dummy_read(struct port_info *pp);
extern void hfc_fx_set_max_hwlog_cnt(struct port_info *pp);
extern void hfc_fx_set_debug_func(struct port_info *pp);
extern void hfc_fx_set_issue_d3hot(struct port_info *pp);
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_fx_set_sysfs_control(struct port_info *pp);
extern void hfc_fx_set_dev_loss_tmo(struct port_info *pp);
extern void hfc_fx_set_scan_finished_tmo(struct port_info *pp);
#endif
#endif /* SYSFS_SUPPORT */
extern void hfc_fx_set_hba_isolation(struct port_info *pp);
extern void hfc_fx_set_ld_err_limit_s(struct port_info *pp);
extern void hfc_fx_set_if_err_limit(struct port_info *pp);
extern void hfc_fx_set_to_err_limit(struct port_info *pp);
extern void hfc_fx_set_to_reset_retry(struct port_info *pp);
extern void hfc_fx_set_rt_err_enable(struct port_info *pp);
extern void hfc_fx_set_limit_log(struct port_info *pp);
extern void hfc_fx_set_filter_target(struct port_info *pp);
extern void hfc_fx_set_rport_lu_scan(struct port_info *ap);		/* FCLNX-GPL-575 */
extern void hfc_fx_set_hg_stats_disable(struct port_info *pp);
extern void hfc_fx_set_ctl_change_qdepth(struct port_info *pp);
extern void hfc_fx_set_core_control(struct port_info *pp);
extern void hfc_fx_set_cc_cnt(struct port_info *pp);
extern void hfc_fx_set_cc_size(struct port_info *pp);
extern void hfc_fx_set_cc_core(struct port_info *pp);
extern void hfc_fx_set_link_reset(struct port_info *pp);
extern void hfc_fx_set_vport_count(struct port_info *pp);
extern void hfc_fx_set_frame_count(struct port_info *pp);
extern void hfc_fx_set_mq_num(struct port_info *pp);
extern void hfc_fx_set_rdtsc(struct port_info *pp);
extern void hfc_fx_set_intdisable(struct port_info *pp);
extern void hfc_fx_set_intenable(struct port_info *pp);
extern void hfc_fx_set_total_abort_to(struct port_info *pp);	/* FCLNX-GPL-FX-014 */
extern void hfc_fx_set_total_tgtrst_to(struct port_info *pp);	/* FCLNX-GPL-FX-014 */
extern void hfc_fx_set_npiv_enable(struct port_info *pp);		/* FCLNX-GPL-FX-137 */
extern void hfc_fx_set_max_io(struct port_info *pp);			/* FCLNX-GPL-FX-147 */
extern void hfc_fx_set_mq_enable(struct port_info *pp);
extern void hfc_fx_set_log_file(struct port_info *pp);
extern void hfc_fx_set_max_lun(struct port_info *pp);
extern void hfc_fx_set_cpu_map(struct port_info *pp);			/* FCLNX-GPL-FX-420 */
extern void hfc_fx_set_login_seq_retry_cnt(struct port_info *pp);	/* FCLNX-GPL-FX-446 */

extern int hfc_proc_info(char *buffer, char **start, off_t offset, int length, int hostno, int inout);
extern int hfc_get_adapter_port_no(struct adap_info *ap);
#endif                          /* !INCLUDE_HFCDETECT */
