/* hfcl_hand_timer_trace.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_hand_timer_trace.h,v 1.7.2.7.12.1.6.3.6.4.2.4.6.1.2.1.2.1.2.1 2015/04/26 05:09:58 toyo Exp $
 */

/*--- TRACE ID ---------------------*/

#define HFC_TRC_HANDLER         0x91
#define HFC_TRC_MBRESP          0x92
#define HFC_TRC_MBINT           0x93
#define HFC_TRC_XRBRSP          0x94
#define HFC_TRC_SCSI_CHK        0x95
#define HFC_TRC_MGM_CHK         0x96
#define HFC_TRC_LINK_CHK        0x97
#define HFC_TRC_CHKSTP          0x98
#define HFC_TRC_DEQ_WE          0x99
#define HFC_TRC_LINKRSP         0x9a
#define HFC_TRC_LGINRSP         0x9b
#define HFC_TRC_ABEND           0x9c
#define HFC_TRC_PDISCRSP        0x9d
#define HFC_TRC_MCKREC          0x9e
#define HFC_TRC_GIDFTRSP        0x9f
#define HFC_TRC_MIHLGRSP        0xa1
#define HFC_TRC_HWERR           0xa2
#define HFC_TRC_WDOG            0xa5
#define HFC_TRC_GPNIDRSP        0xa6
#define HFC_TRC_LINKDOWN_INT    0xa7
#define HFC_TRC_LINKUP_INT      0xa8
#define HFC_TRC_PLOGI_INT       0xa9
#define HFC_TRC_LOGO_INT        0xaa
#define HFC_TRC_SCN_INT         0xab
#define HFC_TRC_RSCN_INT        0xac
#define HFC_TRC_GIDPNRSP        0xad
#define HFC_TRC_FORCE_ISOL       0xb0	/* FCLNX-0147 */
#define HFC_TRC_FORCE_ISOL_REC   0xb1	/* FCLNX-0147 */
#define HFC_TRC_FORCE_ISOL_REC_P 0xb2	/* FCLNX-0147 */
#define HFC_TRC_MLPF_INT		0xC0	/* FCLNX-GPL-393 */
#define HFC_TRC_MLPF_HWERR_INT	0xC1	/* FCLNX-GPL-393 */
#define HFC_TRC_MLPF_FORCE_ISOL	0xC2	/* FCLNX-GPL-393 */
#define HFC_TRC_MLPF_RECV_ISOL	0xC3	/* FCLNX-GPL-393 */
#define HFC_TRC_MLPF_HWERR_INT_DET	0xC4	/* FCLNX-GPL-427 */
#define HFC_TRC_MLPF_MIGRATION	0xC5	/* FCLNX-GPL-489 */
#define HFC_TRC_CHECK_ERRCOUNT	0xD2	/* FCLNX-GPL-349 */

/*--- hfc_reset_start() parm(type)  ----*/
#define HFC_CTLRST              0x01
#define HFC_REBOOT              0x02
#define HFC_F_START             0x03
#define HFC_FW_START            0x04
#define HFC_INI_RESET           0x05
#define HFC_SET_INIADR          0x06
#define HFC_SET_WS80            0x07
#define HFC_SET_WS40            0x08
#define HFC_WSCA_CLEAR          0x09
#define HFC_SET_WS04            0x0a
#define HFC_GR_CLEAR            0x0b    /* FCLNX-GPL-220 */

#define HFC_SET_MLPF_MODE       0x10    /* FCLNX-0379 */
#define HFC_SET_LINK_INI_OPT    0x12	/* FCLNX-GPL-FX-366 */

/*--- type field ---*/
#define TYPE_0                  0x80
#define TYPE_0_PTY              0x81    /* FCWIN-0220 */
#define TYPE_1                  0x40
#define TYPE_01                 0xc0
#define TYPE_2                  0x20
#define TYPE_3					0x10	/* FIVE-EX */
#define TYPE_ELOG               0xf0 
#define TYPE_PCI                0xff
#define TYPE_CFG                0xfe
#define	TYPE_CFG0				0xed	/* FIVE-EX */
#define	TYPE_CFG1				0xec	/* FIVE-EX */
#define TYPE_TRC0               0xfd
#define TYPE_TRC1               0xfc
#define TYPE_TRC2				0xef	/* FIVE-EX */
#define TYPE_TRC3				0xee	/* FIVE-EX */
#define TYPE_ILS                0xfb
#define TYPE_SEQ                0xfa
#define TYPE_BCFG               0xf9
#define TYPE_EVT0               0x01
#define TYPE_EVT1               0x02    /* FCWIN-0220 STR*/
#define TYPE_EVT2               0x03
#define TYPE_EVT3               0x04
#define TYPE_FRM0               0x05
#define TYPE_FRM1               0x06
#define TYPE_FRM2               0x07
#define TYPE_FRM3               0x08
/* #define TYPE_ROTATE             0x10 */
#define TYPE_ERRID				0x11	/* FIVE-EX */
#define TYPE_RSV                0x7f    /* FCWIN-0220 END*/
/*------------------*/
#define REG_RAMMSK              0x02f8
#define REG_RAMADR              0x02fc	
#define REG_INDAREA             0x0500

#define REG_INDFLASH            0x0500
#define REG_INDAREA_FIVE        0x0600  /* FCWIN-234 */

/*-----------------------------------------------------------------*/
/*                         trace format                            */
/* The member name of each structure uses the following prefixes   */
/*                                                                 */
/* a_   : Value acquired from member of adap_info                  */
/* t_   : Value acquired from member of target_info                */
/* s_   : Value acquired from member of Srb                        */
/* h_   : Value acquired from member of hfc_pkt                    */
/*                                                                 */
/*-----------------------------------------------------------------*/
struct errlog_t {
	uint	type;			/* Error type */
	char	errmsg[64];
};

struct hraslog_t {
	uint	type;			/* Error type */
	char	alart1[3];
	char	alart2[3];
};

struct hraslog_errid {
	uchar	ppid[4];
	uchar	alartcode[2];
	uchar	errno[2];
};

typedef struct mem_type {
	uchar	type ;
	uint	reg_adr ;
	uint	size ;
}Type_mem ;

#define     HFC_DUMP_DATA_NUM   16

struct err_trc1 {
	uchar				id ;				/* +00 */
	uchar				sub_id ;			/* +01 */
	uchar				resv1[2] ;			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				int_a_reg;			/* +10 */
	uchar				resv2[96];			/* +14-73 */
//	uint64_t			current_time;
};

struct err_trc2 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				mb_retry_cnt ;		/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				passthrough_rsp;	/* +10 */
	uchar				t_flags;			/* +14 */
	uchar				t_pseq ;			/* +15 */
	uchar				mb_status;			/* +16 */
	uchar				resv1[1];			/* +17 */
	uint				t_status;			/* +18 */
	uchar				resv2[2];			/* +1c *//* FCLNX-0659 */
	ushort				t_id;				/* +1e *//* FCLNX-0659 */
//	uint64_t			a_next_tstart;		
	uint64_t			t_ww_name;			/* +20 */
	uint64_t			t_node_name;		/* +28 */
	uchar				mb_resp[68];		/* +30-73 */
//	uint64_t			current_time;
};

struct err_trc3 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				xrb_no ;			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id;			/* +08 */
	uint				xrb_outp ;			/* +10 */
	uint				xrb_inp ;			/* +14 */
	ushort				xrb_cnt ;			/* +18 */
	uchar				xrb[70] ;			/* +1a-5f */
	uchar				resv1[16] ;			/* +60-6f */
	uint				adap_status;		/* +70 */
	uchar				resv2[2];			/* +74 *//* FCLNX-0659 */
	ushort				xob_no;				/* +76 *//* FCLNX-0659 */
//	uint64_t			current_time;
};

struct err_trc4 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				xrb_no ;			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				t_status;			/* +10 */
	uchar				t_flags;			/* +14 */
	uchar				t_pseq ;			/* +15 */
	uchar				fcp_status;			/* +16 */
	uchar				scsi_status;		/* +17 */
	uchar				t_wait_abort[32];	/* +18-37 */
	uint64_t			srb;				/* +38 */
	uchar				s_function;			/* +40 */
	uchar				s_status;			/* +41 */
	uchar				s_target;			/* +42 */
//	uchar				s_lun;						 /* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar				resv0;				/* +43 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar				s_quetag;			/* +44 */
	uchar				s_queaction;		/* +45 */
	uchar				s_cdblen;			/* +46 */
	uchar				s_senselen;			/* +47 */
	uint				s_flags;			/* +48 */
	uint				s_datatransferlength;	/* +4c */
	uint64_t			hfcp;				/* +50 */
	uint				h_adap_status;		/* +58 */
	uint				h_iov_no;			/* +5c */
	uint				h_iov_cnt;			/* +60 */
	uint				t_we_que_cnt;		/* +64 */
	uint				result;				/* +68 */
	uint				resid;				/* +6c */
	uint				serial_number;		/* +70 */
	ushort				s_lun;				/* +74 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar				resv1[2];			/* +76 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
//	uint64_t			current_time;
};

struct err_trc5 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				xrb_no;				/* +02 */
	uint				a_status;			/* +04 */
	uint64_t			a_scsi_id;			/* +08 */
	uint				t_status;			/* +10 */
	uchar				t_flags;			/* +14 */
	uchar				t_pseq;				/* +15 */
	ushort				timer_id;			/* +16 */
	uint64_t			srb;				/* +18 */
	uchar				s_function;			/* +20 */
	uchar				s_status;			/* +21 */
	uchar				s_target;			/* +22 */
//	uchar				s_lun;				/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar				resv0;				/* +23 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	ushort				s_lun;				/* +24 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar				resv1[82];			/* +26-77 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
//	uint64_t			current_time;
};

struct err_trc6 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	uchar				resv1[2];			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				int_a_reg;			/* +10 */
	uint				 detail_reg;		/* +14 */
	uint64_t			status_reg;			/* +18 */
	uint				link_dead_cnt ;		/* +20 */
	uint				pci_err_cnt ;		/* +24 */
	uint				mck_err_cnt ;		/* +28 */
	uchar				resv2[76] ;			/* +2c-77 */
//	uint64_t			current_time;
};

struct err_trc7 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	uchar				resv1[2];			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				etc1;				/* +10 */
	uint				etc2;				/* +14 */
	uint				etc3;				/* +18 */
	uchar				isol_detail;		/* +1c */
	uchar				c_err;				/* +1d */
	uchar				resv2[2];			/* +1e */
	uint				mpap_sts;			/* +20 */
	uint				link_dead_cnt ;		/* +24 */
	uint				int_a_reg;			/* +28 */
	uint				pci_err_cnt ;		/* +2c */
	uint				mck_err_cnt ;		/* +30 */
	uchar				resv3[68] ;			/* +34-77 */
//	uint64_t			current_time;
};

/* FCLNX-GPL-393 */
struct err_trc8 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	uchar				resv1[2];			/* +02 */
	uint				a_status ;			/* +04 */
	uint64_t			a_scsi_id ;			/* +08 */
	uint				hyp_status;			/* +10 */
	uint				etc2;				/* +14 */
	uint				etc3;				/* +18 */
	uint				resv2[2];			/* +1c */
	uint				mpap_sts;			/* +20 */
	uint				link_dead_cnt ;		/* +24 */
	uint				int_a_reg;			/* +28 */
	uint				pci_err_cnt ;		/* +2c */
	uint				mck_err_cnt ;		/* +30 */
	uchar				resv3[68] ;			/* +34-77 */
//	uint64_t			current_time;
};
/* FCLNX-GPL-393 */

