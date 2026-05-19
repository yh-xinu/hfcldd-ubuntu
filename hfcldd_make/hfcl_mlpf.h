/*
 * hfcl_mlpf.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_mlpf.h,v 1.2.2.6.28.3.2.5.6.1.2.1.2.1 2015/02/04 08:32:48 toyo Exp $
 */

#ifndef _H_HFCL_MLPF
#define _H_HFCL_MLPF

extern struct manage_info hfc_manage_info;
/*extern char hfc_ver[8][64] ;*/

#define HFC_MMODE_CHECK_OK                 1
#define HFC_MMODE_CHECK_ERROR              0
extern int HFC_MMODE_CHECK_BASIC(
	struct adap_info            *ap);

extern int HFC_MMODE_CHECK_MLPF(
	struct adap_info            *ap);

extern int HFC_MMODE_CHECK_SHARED(
	struct adap_info            *ap);

extern int HFC_MMODE_CHECK_REBOOT(
	struct adap_info            *ap);

extern int HFC_MMODE_CHECK_SHADOW(
	struct adap_info            *ap);

extern int HFC_MMODE_CHECK_DEDICATE(
	struct adap_info            *ap);

#define HFC_MLPF_ENABLE                    0
#define HFC_MLPF_DISABLE                   1
extern int hfc_mlpf_setup_lparmode(
	struct adap_info            *ap);

#define HFC_MLPF_VFC_VALID                 1
extern uint hfc_mlpf_setup_wwn(
	struct adap_info            *ap);

/*--------------------------------------------------------------------------*/	/* @MLPF STR */
/* Name    : hfc_read_hg_reg() / hfc_write_hg_reg()                         */
/* Func    : MIO-HG space lead/light                                        */
/* Arg     : _AP                            : Adap_info structure pointer   */
/*           _REGNO                         : Register registration number  */
/*           _SIZE                          : Reading size(1/2/4)           */
/*           _DATA                          : Writing data(Write Only)      */
/* Response: MMIO-HG register data                                          */
/*            reg_size = 1 : The low rank 1 byte                            */
/*            reg_size = 2 : 2 bytes in the under                           */
/*            reg_size = 4 : 4 byte                                         */
/* context : -                                                              */
/* Note    : Secure the lock of adap_info                                   */
/*--------------------------------------------------------------------------*/
																			/* @MLPF END */

// FCLNX-0374
#define hfc_read_hg_reg( _AP, _REGNO, _SIZE ) \
	( hfc_read_reg_hg_ext((_AP), ((struct adap_info*)(_AP))->lparmode.hg_map->iosp.reg[_REGNO], (_SIZE)) )

#define hfc_write_hg_reg( _AP, _REGNO, _SIZE, _DATA ) \
	( hfc_write_reg_hg_ext((_AP), ((struct adap_info*)(_AP))->lparmode.hg_map->iosp.reg[_REGNO], (_SIZE), (_DATA)) )


// When forced mck is beaten from hfc_abend(), the following macros are used
#define HFC_ISSUE_FMCK(AP, TYPE) {												\
		if ( HFC_MMODE_CHECK_SHARED( (AP) ) )									\
			hfc_mlpf_issue_ffmck( (AP), (TYPE) );								\
		else																	\
			hfc_issue_forced_mck( (AP), (TYPE) );								\
}

// When check stop is beaten by pci bus error, the following macros are used
#define HFC_ISSUE_CSTP_PCIERR(AP, MPLOCK) {										\
		if ( ( HFC_MMODE_CHECK_SHADOW( (AP) ) ) || ( !( HFC_MMODE_CHECK_SHARED( (AP) ) ) ) )	\
			hfc_chk_stop( (AP), (MPLOCK) );										\
		else																	\
			hfc_mlpf_cstpend_int(ap);											\
}																			/* FCLNX-GPL-400 */

// When check stop is beaten, the following macros are used
#define HFC_ISSUE_CSTP(AP, MPLOCK, TYPE) {										\
		if ( HFC_MMODE_CHECK_SHADOW( (AP) ) )									\
			hfc_mlpf_issue_ffcstp( (AP), (TYPE) );								\
		else if( !(HFC_MMODE_CHECK_SHARED( (AP) ) ) )							\
			hfc_chk_stop( (AP), (MPLOCK) );										\
}																			/* FCLNX-GPL-316 */

/* FCLNX-GPL-494 Get Statistics for Virtage */
#define HFC_MLPF_STATISTICS_START(_AP)						\
if (!HFC_MMODE_CHECK_BASIC((_AP)))							\
{															\
	if ((_AP)->hg_cca_p != NULL) {							\
		(_AP)->hg_cca_p->io_exec = (_AP)->scsi_exec_cnt;	\
		(_AP)->hg_cca_p->statistics_cnt++;					\
	}														\
}

#define HFC_MLPF_STATISTICS_END(_AP)						\
if (!HFC_MMODE_CHECK_BASIC((_AP)))							\
{															\
	if ((_AP)->hg_cca_p != NULL) {							\
		(_AP)->hg_cca_p->io_end = (_AP)->scsi_end_cnt;		\
		(_AP)->hg_cca_p->io_err = (_AP)->scsi_err_cnt;		\
		(_AP)->hg_cca_p->statistics_cnt++;					\
	}														\
}

extern void hfc_mlpf_set_mmio_hg(
	struct adap_info            *ap,
	uchar                       *buf,
	uint                        offset,
	uint                        length);

extern void hfc_mlpf_get_mmio_hg(
	struct adap_info            *ap,
	uchar                       *vpd_buf,
	uint                        offset,
	uint                        length);

#define HFC_MLPF_CONFIG_CHECK_OK           0
#define HFC_MLPF_CONFIG_CHECK_ERROR        1
extern int hfc_mlpf_config_check(
	struct adap_info            *ap);

#define HFC_CHECK_HYPER_STATE              1
#define HFC_CHECK_LPAR_STATE               2
#define HFC_CHECK_HVM_SUPPORT              3	/* FCLNX-GPL-489 */


extern int hfc_mlpf_check_state(
	struct adap_info            *ap,
	uint                        status,
	uint                        type);

#define HFC_ENABLE_HYPER_STATE              0xC0
#define HFC_DISABLE_HYPER_STATE             0x40
#define HFC_ENABLE_LPAR_STATE               0x80
#define HFC_DISABLE_LPAR_STATE              0x00
#define HFC_ENABLE_DRV_SUPPORT              0x0C	/* FCLNX-GPL-489 */
#define HFC_DISABLE_DRV_SUPPORT             0x04	/* FCLNX-GPL-489 */

#define HFC_MLPF_FFMCK              0x20100000
#define HFC_MLPF_FFCSTP             0x20110000
#define HFC_MLPF_FFCSTP_IML         0x20120000
#define HFC_MLPF_CSTPEND            0x20130000
#define HFC_MLPF_F_ISOLATE_ERR      0x20140000	/* FCLNX-GPL-392 */
#define HFC_MLPF_F_ISOLATE_CMD      0x20150000	/* FCLNX-GPL-392 */
#define HFC_MLPF_RECOV_ISOLATE      0x20160000	/* FCLNX-GPL-392 */
#define HFC_MLPF_RECOV_ISOL_END     0x20170000	/* FCLNX-GPL-392 */
#define HFC_MLPF_F_ISOLATE_END      0x20180000	/* FCLNX-GPL-427 */
#define HFC_MLPF_SET_INDACC         0x20200000
#define HFC_MLPF_RESET_INDACC       0x20210000
#define HFC_MLPF_INDACC_FLG         0x80000000

#define HFC_MLPF_INDACC_SUCCESS     0x00000000
#define HFC_MLPF_INDACC_DISABLE     0x00000001
#define HFC_MLPF_INDACC_USED        0x00000002

#define HFC_MLPF_INDACC_READ        0x00000001
#define HFC_MLPF_INDACC_WRITE       0x00000002
extern void hfc_mlpf_change_state(
	struct adap_info            *ap,
	uint                        status,
	uint                        type);

extern uint64_t hfc_read_reg_hg_ext(
	struct adap_info            *ap,
	uint                        offset,
	char                        reg_size);

extern void hfc_write_reg_hg_ext(
	struct adap_info            *ap,
	uint                        offset,
	char                        reg_size,
	uint64_t                    data);

extern void hfc_mlpf_pci_error(
	struct adap_info            *ap,
	uchar                       type);

extern void hfc_mlpf_mck_recovery(
	struct adap_info            *ap,
	uchar                        type);

extern uchar hfc_mlpf_intr(
	struct adap_info            *ap,
	uint                        int_a_reg);

extern void hfc_mlpf_hwerr_int(
	struct adap_info     *ap,
	unsigned int         hyp_status);

extern void hfc_mlpf_hwerr_int_detail(
	struct adap_info     *ap,
	unsigned int         hyp_status,
	unsigned int         hyp_int_detail);

extern void hfc_mlpf_mck_recovery_glpar(
	struct adap_info            *ap);

extern void hfc_mlpf_mckend_int(
	struct adap_info            *ap);

extern void hfc_mlpf_mckend_int_glpar(
	struct adap_info            *ap);

extern void hfc_mlpf_fcstp_int(
	struct adap_info            *ap,
	uchar                       type);

extern void hfc_mlpf_issue_ffcstp(
	struct adap_info            *ap,
	uchar                       type);

extern void hfc_mlpf_issue_ffmck(
	struct adap_info            *ap,
	uchar                       type);

/* FCLNX-GPL-393 */
extern int hfc_mlpf_issue_fisolate(
	struct adap_info            *ap,
	uchar                       proc);
/* FCLNX-GPL-393 */

/* FCLNX-GPL-393 */
extern int hfc_mlpf_issue_recov_isolate(
	struct adap_info            *ap);
/* FCLNX-GPL-393 */

extern void hfc_mlpf_cstpend_int(
	struct adap_info            *ap);

extern void hfc_mlpf_isol_start_glpar( /* FCLNX-GPL-427 */
	struct adap_info     *ap,
	unsigned int         hyp_status);

extern void hfc_mlpf_isol_start_slpar( /* FCLNX-GPL-427 */
	struct adap_info     *ap,
	unsigned int         hyp_status);

extern void hfc_mlpf_isol_end_glpar( /* FCLNX-GPL-427 */
	struct adap_info            *ap,
	unsigned int         hyp_status);

extern void hfc_mlpf_isol_recovery_start_glpar( /* FCLNX-GPL-427 */
	struct adap_info	*ap,
	unsigned int		hyp_status);

extern void hfc_mlpf_isol_recovery_start_slpar( /* FCLNX-GPL-427 */
	struct adap_info	*ap,
	unsigned int		hyp_status);

extern void hfc_mlpf_isol_recovery_end_glpar( /* FCLNX-GPL-427 */
	struct adap_info	*ap,
	unsigned int		hyp_status,
	int					fw_start);

extern uint hfc_mlpf_check_hypcondition( /* FCLNX-GPL-427 */
	unsigned int hyp_status);

extern int hfc_mlpf_check_normal_hypsts( /* FCLNX-GPL-427 */
	struct adap_info    *ap);

extern void hfc_mlpf_errlog_slpar(
	struct adap_info        *ap);

extern void hfc_mlpf_errlog_glpar(
	struct adap_info        *ap);

extern uint hfc_mlpf_indacc(
	struct adap_info        *ap);

extern void hfc_mlpf_check_isol_psycalport(
	struct adap_info        *ap);	/* FCLNX-GPL-393 */
	
extern void hfc_mlpf_check_isol_support(
	struct adap_info            *ap);	/* FCLNX-GPL-393 */
	
extern void hfc_mlpf_set_errorlimit(
	struct adap_info            *ap);	/* FCLNX-GPL-393 */

extern void hfc_mlpf_set_led(
	struct adap_info            *ap,
	uint64_t					data);	/* FCLNX-GPL-399 */
	
extern void hfc_mlpf_set_fcif(
	struct adap_info            *ap,
	uint64_t					data);	/* FCLNX-GPL-399 */
	
extern void hfc_mlpf_migration_end(
	struct adap_info    *ap);			/* FCLNX-GPL-489 */
	
extern void hfc_mlpf_migration_recovery(
	struct adap_info    *ap,
	uint				hyp_status);	/* FCLNX-GPL-489 */
	
extern int hfc_alloc_mlpf_cca(
	struct adap_info    *ap);			/* FCLNX-GPL-494 */

extern void hfc_free_mlpf_cca(
	struct adap_info    *ap);			/* FCLNX-GPL-494 */

extern void hfc_mlpf_cca_setup(
	struct adap_info    *ap);			/* FCLNX-GPL-494 */

#endif /* _H_HFCL_MLPF*/
