/*
 * hfcl_detect_fx.h
 * Copyright (C) 2007, 2016, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_detect_fx.h,v 1.1.2.7.2.3.2.4.2.5.2.1 2016/02/18 05:15:41 mhayashi Exp $
 */


#ifndef _H_HFCL_DETECT_FX
#define _H_HFCL_DETECT_FX

extern int hfc_fx_probe_one(struct pci_dev *pdev, const struct pci_device_id *id, char *p);
extern void hfc_fx_remove_one(struct pci_dev *pdev);
extern int hfc_fx_biosparam(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int geom[]);

extern int hfc_fx_proc_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length, uint proc_type);
extern int hfc_fx_proc_info_k26(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
extern int hfc_fx_release_adp(struct port_info *pp);
extern int hfc_fx_release(struct Scsi_Host *host);
extern const char *hfc_fx_info(struct Scsi_Host *host);
extern int hfc_fx_target_lu_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length);
extern int hfc_fx_target_status_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length);

extern int hfc_fx_slave_configure(struct scsi_device *sdev);
extern int hfc_fx_slave_alloc(struct scsi_device *sdev);
extern void hfc_fx_slave_destroy(struct scsi_device *sdev);
#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
extern int hfc_fx_change_queue_depth(struct scsi_device *sdev, int qdepth, int reason);
#else
extern int hfc_fx_change_queue_depth(struct scsi_device *sdev, int qdepth);
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
extern int hfc_fx_change_queue_type(struct scsi_device *sdev, int tag_type);
#endif /* FCLNX-GPL-FX-496 end */
extern void hfc_fx_lu_scan_start(struct port_info *pp);
extern int hfc_fx_scan_finished(struct Scsi_Host *shost, unsigned long time);
extern void hfc_fx_scan_start(struct Scsi_Host *shost);

extern void hfc_fx_clear_target_info_fx( struct port_info *pp, struct target_info_fx *target, int pmsg );

extern int hfc_fx_attach(struct port_info *pp);
extern int hfc_fx_pci_conf(struct port_info *pp);
extern int hfc_fx_check_cs_disable(struct port_info *pp, struct core_info *core);

extern int hfc_fx_allocate_memory(struct port_info *pp, int rid);
extern int hfc_fx_allocate_memory_core(struct port_info *pp, int rid, int core_no);
extern int hfc_fx_allocate_dma(struct port_info *pp, int rid, int core_no);
extern int hfc_fx_free_memory(struct port_info *pp, int rid);
extern int hfc_fx_free_memory_core(struct port_info *pp, int rid, int core_no);
extern int hfc_fx_free_dma(struct port_info *pp, int rid, int core_no);
extern void hfc_fx_detach(struct port_info *pp);

extern void hfc_fx_pci_err_status_clear_five_fx(struct port_info *pp);	/* FCLNX-GPL-FX-145 */

extern int hfc_fx_query_pktype(struct port_info *pp);
extern int hfc_fx_start_adapter(struct port_info *pp);

extern int hfc_fx_core_start(struct port_info *pp, int immdt_cmd);

extern void hfc_fx_get_bindtid(struct port_info *pp);
extern int hfc_fx_initialize(struct port_info *pp, int immdt_cmd);
extern int hfc_fx_check_inta(struct port_info *pp);

extern void hfc_fx_wwnverify_linkup (struct port_info *pp, struct target_info_fx *target, 
	struct core_info *core, uint mb_resp_status, uint64_t ww_name);
extern void hfc_fx_wwnverify_linkup_timeout(struct port_info *pp, struct target_info_fx *target, uint mb_resp_status);
extern void hfc_fx_set_linkup2_tmo(struct port_info *pp);
extern void hfc_fx_wwnverify_gidft(struct port_info *pp, struct target_info_fx *target, 
	struct core_info *core, uint mb_resp_status);
extern void hfc_fx_wwnverify_gpnid(struct port_info *pp, struct target_info_fx *target, 
	struct core_info *core, uint mb_resp_status);
extern void hfc_fx_wwnverify_receive_plogi(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name);
extern void hfc_fx_wwnverify_plogi(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name);
extern void hfc_fx_wwnverify_prli(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name);
extern void hfc_fx_wwnverify_login(struct port_info *pp, struct target_info_fx *target, uint mb_resp_status, uint64_t ww_name);
extern void hfc_fx_wwnverify_scn(struct port_info *pp, struct target_info_fx *target, uint mb_resp_status);
extern void hfc_fx_wwnverify_gpnft(struct port_info *pp, struct target_info_fx *target, 
	struct core_info *core, uint mb_resp_status);

extern struct target_info_fx *hfc_fx_add_target_info_fx( struct port_info *pp, uint64_t scsi_id );


extern int hfc_fx_chk_conf_val(int min, int max, int val);
extern int hfc_fx_chk_conf_ls(int min, int max, int val);
extern int hfc_fx_chk_conf_mt(int min, int max, int val);

extern void hfc_fx_conf_setup(struct port_info *pp);


extern int hfc_fx_search_adapter_number(struct port_info *pp);
extern char hfc_fx_cnvc(char C);
extern int hfc_fx_parse_string(char *string, char *keyword, uint64_t *value);
extern int hfc_fx_convert_string(char *string, uint64_t *value);
extern void hfc_fx_set_pub_symbol_list(void);
extern int hfc_fx_check_nonpub_symbol(void);

extern int hfc_fx_param_search(char *search_str, int *value);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_fx_rport_add(struct port_info *pp, struct target_info_fx *target);
extern void hfc_fx_rport_delete(struct target_info_fx *target);
extern void hfc_fx_fc_host_init(struct Scsi_Host *host, struct port_info *pp);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
extern void hfc_fx_dev_loss_tmo_callbk(struct Scsi_Host *host, struct fc_rport *rport);
#endif

#endif
#endif /* SYSFS_SUPPORT */

extern void hfc_fx_reset_all_timer (struct port_info *pp);

extern int hfc_fx_set_interrupts(struct port_info *pp, int type);
extern int hfc_fx_set_intr_entry(struct port_info *pp,int type);
extern void hfc_fx_free_interrupts(struct port_info *pp, int type, int nvec, int pci_fail);
extern void hfc_fx_clear_status(struct port_info *pp);
extern void hfc_fx_set_mcw_five_ex(struct port_info *pp);
extern int hfc_fx_check_failed_core(struct port_info *pp, int rest_retry_cnt);
extern int hfc_fx_core_chk_stop(struct port_info *pp, struct core_info *core, uint err_id);
extern int hfc_fx_config_hw_set_five_fx(struct port_info *pp, uint retry_maxcnt);
extern int hfc_fx_query_devid(struct port_info *pp);

extern uint hfc_fx_get_sysrev(struct core_info *core);
extern struct pci_dev *hfc_fx_get_slot_dev(struct port_info *pp);
extern uint64_t hfc_fx_restore_add_wwn(struct port_info *pp);
extern int hfc_fx_backup_add_wwn(struct port_info *pp, uint64_t add_ww_name);
extern int hfc_fx_pcie_link_width_chk(struct port_info *pp);
extern int hfc_fx_get_vpd(struct port_info *pp, uchar *vpd_buf);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern void hfc_fx_kthread_stop(struct port_info *pp);
extern int hfc_fx_do_rport(void *p);
extern void hfc_fx_set_rport_status(struct port_info *pp, struct region_info *rp);
extern int hfc_fx_scan_rport_status(struct port_info *pp, struct region_info *rp);
extern void hfc_fx_start_rport( struct port_info *pp );
extern void hfc_fx_stop_rport(struct port_info *pp);

extern void hfc_fx_get_starget_node_name(struct scsi_target *starget);
extern void hfc_fx_get_starget_port_name(struct scsi_target *starget);
extern void hfc_fx_get_starget_port_id(struct scsi_target *starget);
extern void hfc_fx_get_host_port_id(struct Scsi_Host *host);
extern void hfc_fx_get_host_port_type(struct Scsi_Host *host);
extern void hfc_fx_get_host_port_state(struct Scsi_Host *host);
extern void hfc_fx_get_host_speed(struct Scsi_Host *host);
extern void hfc_fx_get_host_fabric_name(struct Scsi_Host *host);
extern struct fc_host_statistics * hfc_fx_get_statistics(struct Scsi_Host *v);
extern void hfc_fx_reset_statistics(struct Scsi_Host *host);
extern int hfc_fx_issue_lip(struct Scsi_Host *host);
#endif
#endif /* SYSFS_SUPPORT */

extern void hfc_fx_set_hw_mcw_cfg(struct port_info *pp);
extern void hfc_fx_set_hw_mcw_pci(struct port_info *pp);
extern int hfc_fx_skip_link_init(struct port_info *pp, ushort bind_err);
extern char *hfc_fx_delete_space(char *string);
extern int hfc_fx_check_hba_isolation(struct port_info *pp);
extern int hfc_fx_get_adap_status(struct port_info *pp);
extern void hfc_fx_determine_master_core(struct port_info *pp, struct region_info *rp);
extern void hfc_fx_set_fw_init_tbl(struct port_info *pp);
extern void hfc_fx_assign_core_no(struct port_info *pp, struct dev_info_fx *dev);
extern int hfc_fx_mq_attach(struct port_info *pp);
extern void hfc_fx_mq_detach(struct port_info *pp);
extern void hfc_fx_mq_change_target_info(struct port_info *pp, struct target_info_fx *target);
extern void hfc_fx_mq_copy_iocinfo(struct port_info *pp);	/* FCLNX-GPL-FX-206 */
extern void hfc_fx_calc_cpu_num(struct port_info *pp);	/* FCLNX-GPL-FX-201 */

#endif                          /* !INCLUDE_HFCDETECT_FX */
