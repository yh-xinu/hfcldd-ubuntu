/*
 * hfcl_mlpf_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_mlpf_fx.h,v 1.1.2.2.2.1.2.1.2.1 2015/03/05 02:19:41 toyo Exp $
 */

#ifndef _H_HFCL_MLPF_FX
#define _H_HFCL_MLPF_FX

extern struct manage_info hfc_manage_info;
/*extern char hfc_fx_ver[8][64] ;*/

#define HFC_FX_MMODE_CHECK_OK                 1
#define HFC_FX_MMODE_CHECK_ERROR              0
extern int HFC_FX_MMODE_CHECK_BASIC(
	struct port_info            *pp);

extern int HFC_FX_MMODE_CHECK_MLPF(
	struct port_info            *pp);

extern int HFC_FX_MMODE_CHECK_SHARED(
	struct port_info            *pp);

extern int HFC_FX_MMODE_CHECK_REBOOT(
	struct port_info            *pp);

extern int HFC_FX_MMODE_CHECK_SHADOW(
	struct port_info            *pp);

extern int HFC_FX_MMODE_CHECK_DEDICATE(
	struct port_info            *pp);

#define HFC_MLPF_ENABLE                    0
#define HFC_MLPF_DISABLE                   1
extern int hfc_fx_mlpf_setup_lparmode(
	struct port_info            *pp);

#define HFC_MLPF_VFC_VALID                 1
extern uint hfc_fx_mlpf_setup_wwn(
	struct port_info            *pp);

/*--------------------------------------------------------------------------*/	/* @MLPF STR */
/* Name    : hfc_fx_read_hg_reg() / hfc_fx_write_hg_reg()                         */
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
/* Note    : Secure the lock of port_info                                   */
/*--------------------------------------------------------------------------*/
																			/* @MLPF END */

// FCLNX-0374
#define hfc_fx_read_hg_reg( _PP, _REGNO, _SIZE ) \
	( hfc_fx_read_reg_hg_ext((_PP), ((struct port_info*)(_PP))->lparmode.hg_map->iosp.reg[_REGNO], (_SIZE)) )

#define hfc_fx_write_hg_reg( _PP, _REGNO, _SIZE, _DATA ) \
	( hfc_fx_write_reg_hg_ext((_PP), ((struct port_info*)(_PP))->lparmode.hg_map->iosp.reg[_REGNO], (_SIZE), (_DATA)) )


// When forced mck is beaten from hfc_fx_abend(), the following macros are used
#define HFC_FX_ISSUE_FMCK(PP, CORE, TYPE) {										\
		if ( HFC_FX_MMODE_CHECK_SHARED( (PP) ) )								\
			hfc_fx_mlpf_issue_ffmck( (PP), (CORE), (TYPE) );					\
		else																	\
			hfc_fx_issue_forced_mck( (PP), (CORE), (TYPE) );					\
}

// When check stop is beaten by pci bus error, the following macros are used
#define HFC_FX_ISSUE_CSTP_PCIERR(PP) {										\
		if ( ( HFC_FX_MMODE_CHECK_SHADOW( (PP) ) ) || ( !( HFC_FX_MMODE_CHECK_SHARED( (PP) ) ) ) )	\
			hfc_fx_chk_stop( (PP) );										\
		else																	\
			hfc_fx_mlpf_cstpend_int(pp);											\
}																			/* FCLNX-GPL-400 */

// When check stop is beaten, the following macros are used
#define HFC_FX_ISSUE_CSTP(PP, TYPE) {											\
		if ( ( HFC_FX_MMODE_CHECK_SHADOW( (PP) ) ) || ( !( HFC_FX_MMODE_CHECK_SHARED( (PP) ) ) ) )	\
			hfc_fx_chk_stop( (PP) );											\
		else																	\
			hfc_fx_mlpf_issue_ffcstp( (PP), (TYPE) );							\
}																			/* FCLNX-GPL-316 *//* FCLNX-GPL-FX-427 */

/* FCLNX-GPL-494 Get Statistics for Virtage */
#define HFC_FX_MLPF_STATISTICS_START(_PP)						\
if (!HFC_FX_MMODE_CHECK_BASIC((_PP)))							\
{															\
	if ((_PP)->hg_cca_p != NULL) {							\
		(_PP)->hg_cca_p->io_exec = (_PP)->scsi_exec_cnt;	\
		(_PP)->hg_cca_p->statistics_cnt++;					\
	}														\
}

#define HFC_FX_MLPF_STATISTICS_END(_PP)						\
if (!HFC_FX_MMODE_CHECK_BASIC((_PP)))							\
{															\
	if ((_PP)->hg_cca_p != NULL) {							\
		(_PP)->hg_cca_p->io_end = (_PP)->scsi_end_cnt;		\
		(_PP)->hg_cca_p->io_err = (_PP)->scsi_err_cnt;		\
		(_PP)->hg_cca_p->statistics_cnt++;					\
	}														\
}

#define hfc_fx_write_hg_reg_core( _PP, _CORENO, _REGNO, _SIZE, _DATA, _OFFSET ) \
	( hfc_fx_write_reg_hg_ext((_PP), ((struct port_info*)(_PP))->lparmode.hg_map->iosp.reg[_REGNO] + _CORENO*_OFFSET, (_SIZE), (_DATA)) )

#define hfc_fx_read_hg_reg_core( _PP, _CORENO, _REGNO, _SIZE, _OFFSET ) \
	( hfc_fx_read_reg_hg_ext((_PP), ((struct port_info*)(_PP))->lparmode.hg_map->iosp.reg[_REGNO] + _CORENO*_OFFSET, (_SIZE)) )

extern void hfc_fx_mlpf_set_mmio_hg(
	struct port_info            *pp,
	uchar                       *buf,
	uint                        offset,
	uint                        length);

extern void hfc_fx_mlpf_get_mmio_hg(
	struct port_info            *pp,
	uchar                       *vpd_buf,
	uint                        offset,
	uint                        length);

#define HFC_MLPF_CONFIG_CHECK_OK           0
#define HFC_MLPF_CONFIG_CHECK_ERROR        1
extern int hfc_fx_mlpf_config_check(
	struct port_info            *pp,
	struct core_info			*core);

#define HFC_CHECK_HYPER_STATE              1
#define HFC_CHECK_LPAR_STATE               2
#define HFC_CHECK_HVM_SUPPORT              3	/* FCLNX-GPL-489 */


extern int hfc_fx_mlpf_check_state_port(
	struct port_info            *pp,
	uint                        status,
	uint                        type);

extern int hfc_fx_mlpf_check_state_core(
	struct port_info            *pp,
	uint	                    core_no,
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
#define HFC_MLPF_CMD_MCKRECCMP      0x20190000	/* FCLNX-GPL-FX-376 */
#define HFC_MLPF_CMD_LINKINICMP     0x201A0000	/* FCLNX-GPL-FX-376 */
#define HFC_MLPF_CMD_CSTPCLEAR      0x201B0000	/* FCLNX-GPL-FX-376 */
#define HFC_MLPF_SET_INDACC         0x20200000
#define HFC_MLPF_RESET_INDACC       0x20210000
#define HFC_MLPF_INDACC_FLG         0x80000000
#define HFC_MLPF_INDACC_MCKLOG      0x01	/* FCLNX-GPL-FX-391 */
#define HFC_MLPF_INDACC_CSTPLOG     0x02	/* FCLNX-GPL-FX-391 */
#define HFC_MLPF_INDACC_IMLFAIL     0x03	/* FCLNX-GPL-FX-391 */
#define HFC_MLPF_INDACC_EFILOG      0x04	/* FCLNX-GPL-FX-391 */
#define HFC_MLPF_INDACC_LINKDOWN    0x05	/* FCLNX-GPL-FX-391 */

#define HFC_FX_MLPF_PORT_CSTPEND    0x20133100
#define HFC_FX_MLPF_CORE_CSTPEND    0x20132f00

#define HFC_MLPF_INDACC_SUCCESS     0x00000000
#define HFC_MLPF_INDACC_DISABLE     0x00000001
#define HFC_MLPF_INDACC_USED        0x00000002

#define HFC_MLPF_INDACC_READ        0x00000001
#define HFC_MLPF_INDACC_WRITE       0x00000002
extern void hfc_fx_mlpf_change_state_port(
	struct port_info            *pp,
	uint                        status,
	uint                        type);

/* FCLNX-GPL-FX-376 */
extern void hfc_fx_mlpf_change_state_core(
	struct port_info            *pp,
	uint                        core_no,
	uint                        status,
	uint                        type);
/* FCLNX-GPL-FX-376 */

extern uint64_t hfc_fx_read_reg_hg_ext(
	struct port_info            *pp,
	uint                        offset,
	char                        reg_size);

extern void hfc_fx_write_reg_hg_ext(
	struct port_info            *pp,
	uint                        offset,
	char                        reg_size,
	uint64_t                    data);

extern void hfc_fx_mlpf_pci_error(
	struct port_info            *pp,
	uchar                       type);

extern uchar hfc_fx_mlpf_intr(
	struct port_info            *pp,
	struct core_info			*core,
	uint                        int_vector);	/* FCLNX-GPL-FX-386	*/

extern void hfc_fx_mlpf_hwerr_int_detail(
	struct port_info     *pp,
	struct core_info     *core,
	unsigned int         hyp_status,
	unsigned int         hyp_int_detail);

extern void hfc_fx_mlpf_mck_recovery_glpar(
	struct port_info            *pp);

extern void hfc_fx_mlpf_mckend_int(
	struct port_info            *pp);

extern void hfc_fx_mlpf_mckend_int_glpar(
	struct port_info            *pp);

extern void hfc_fx_mlpf_linkend_int_glpar(
	struct port_info            *pp);

extern void hfc_fx_mlpf_issue_ffcstp(
	struct port_info            *pp,
	uchar                       type);

extern void hfc_fx_mlpf_issue_ffmck(
	struct port_info            *pp,
	struct core_info			*core,
	uchar                       type);

/* FCLNX-GPL-393 */
extern int hfc_fx_mlpf_issue_fisolate(
	struct port_info            *pp,
	uchar                       proc);
/* FCLNX-GPL-393 */

/* FCLNX-GPL-393 */
extern int hfc_fx_mlpf_issue_recov_isolate(
	struct port_info            *pp);
/* FCLNX-GPL-393 */

extern void hfc_fx_mlpf_cstpend_int(
	struct port_info            *pp);

extern void hfc_fx_mlpf_isol_start_glpar( /* FCLNX-GPL-427 */
	struct port_info     *pp,
	unsigned int         hyp_status);

extern void hfc_fx_mlpf_isol_start_slpar( /* FCLNX-GPL-427 */
	struct port_info     *pp,
	unsigned int         hyp_status);

extern void hfc_fx_mlpf_isol_end_glpar( /* FCLNX-GPL-427 */
	struct port_info            *pp,
	unsigned int         hyp_status);

extern void hfc_fx_mlpf_isol_recovery_start_glpar( /* FCLNX-GPL-427 */
	struct port_info	*pp,
	unsigned int		hyp_status);

extern void hfc_fx_mlpf_isol_recovery_start_slpar( /* FCLNX-GPL-427 */
	struct port_info	*pp,
	unsigned int		hyp_status);

extern void hfc_fx_mlpf_isol_recovery_end_glpar( /* FCLNX-GPL-427 */
	struct port_info	*pp,
	struct core_info	*core,
	unsigned int		hyp_status,
	int					fw_start);

extern uint hfc_fx_mlpf_check_hypcondition( /* FCLNX-GPL-427 */
	unsigned int hyp_status);

extern int hfc_fx_mlpf_check_normal_hypsts( /* FCLNX-GPL-427 */
	struct port_info    *pp);

extern void hfc_fx_mlpf_errlog_slpar(
	struct port_info        *pp,
	struct core_info        *core,
	uchar  ssn,
	uchar  son,
	uchar  log_type);	/* FCLNX-GPL-FX-391 */

extern void hfc_fx_mlpf_errlog_glpar(
	struct port_info        *pp);

extern uint hfc_fx_mlpf_indacc(
	struct port_info        *pp,
	struct core_info        *core,
	uchar  ssn,
	uchar  son,
	uchar  log_type);	/* FCLNX-GPL-FX-391 */

extern void hfc_fx_mlpf_check_isol_psycalport(
	struct port_info        *pp);	/* FCLNX-GPL-393 */
	
extern void hfc_fx_mlpf_check_isol_support(
	struct port_info            *pp);	/* FCLNX-GPL-393 */
	
extern void hfc_fx_mlpf_set_errorlimit(
	struct port_info            *pp);	/* FCLNX-GPL-393 */

extern void hfc_fx_mlpf_set_led(
	struct port_info            *pp,
	uint64_t					data);	/* FCLNX-GPL-399 */
	
extern void hfc_fx_mlpf_set_fcif(
	struct port_info            *pp,
	uint64_t					data);	/* FCLNX-GPL-399 */
	
extern void hfc_fx_mlpf_migration_end(
	struct port_info    *pp,
	struct core_info	*core);			/* FCLNX-GPL-489 */
	
extern void hfc_fx_mlpf_migration_recovery(
	struct port_info    *pp,
	struct core_info	*core,
	uint				hyp_status);	/* FCLNX-GPL-489 */
	
extern int hfc_fx_alloc_mlpf_cca(
	struct port_info    *pp);			/* FCLNX-GPL-494 */

extern void hfc_fx_free_mlpf_cca(
	struct port_info    *pp);			/* FCLNX-GPL-494 */

extern void hfc_fx_mlpf_cca_setup(
	struct port_info    *pp);			/* FCLNX-GPL-494 */

#endif /* _H_HFCL_MLPF*/
