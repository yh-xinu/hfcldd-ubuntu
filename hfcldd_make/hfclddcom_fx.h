/*
 * hfclddcom_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfclddcom_fx.h,v 1.1.2.10.2.2.2.1.2.2 2015/03/06 06:30:29 toyo Exp $
 */

#ifndef _H_HFCDDCOM_FX
#define _H_HFCDDCOM_FX


/* PCI bus IO spase FRAMEA field */
#define HFC_FX_FRAMEA_FW_START			( 0x40000000 )
#define HFC_FX_FRAMEA_FW_START_PORT		( 0x41000000 )
#define HFC_FX_FRAMEA_FW_ISOL_PORT		( 0x43000000 )
#define HFC_FX_FRAMEA_ENQ_XOB			( 0x44000000 )
#define HFC_FX_FRAMEA_MB_INIT			( 0xc0000000 )
#define HFC_FX_FRAMEA_MB_CORE_START		( 0xc1000000 )
#define HFC_FX_FRAMEA_MB_SHADOW_UP		( 0xc2000000 )	/* FCLNX-GPL-FX-393 */
#define HFC_FX_FRAMEA_INT_RSP			( 0x20000000 )
#define HFC_FX_FRAMEA_OFFLINE			( 0x42000000 )
#define HFC_FX_FRAMEA_ONLINEUP			( 0x4c000000 )
#define HFC_FX_FRAMEA_FW_TRC_MODE		( 0x4f000000 )

static const uint hfc_status_of_core[MAX_CORE_PROBE] = {
	0x0c10, /* HFC_IOSPACE_CORE0_STATUS0, Core 0 */
	0x0d10, /* HFC_IOSPACE_CORE1_STATUS0, Core 1 */
	0x0e10, /* HFC_IOSPACE_CORE2_STATUS0, Core 2 */
	0x0f10  /* HFC_IOSPACE_CORE3_STATUS0, Core 3 */
};

#define HFC_FX_HWERR_MPCHK			0x00000008
#define HFC_FX_HWERR_T3				0x00000004

/************************************************************************/
/* SubSystemID														*/
/************************************************************************/
#define	HFC_HOST_SUB_SYSTEM_ID_FX1P	0x3070	/* FIVE-FX */
#define	HFC_HOST_SUB_SYSTEM_ID_FX2P	0x3071	/* FIVE-FX */
#define	HFC_HOST_SUB_SYSTEM_ID_FX4P	0x3072	/* FIVE-FX */

/************************************************************************/
/* Async. command														*/
/************************************************************************/
#define HFC_FX_FRAMEA_CHANGE_STATE		( 0x60000000 )

#define HFC_FX_CHANGE_STATE_LINKINI		0x04
#define HFC_FX_CHANGE_STATE_LINKINI_ERR	0x05
#define HFC_FX_CHANGE_STATE_LINKUP		0x0c
#define HFC_FX_CHANGE_STATE_LINKDOWN	0x0d

/* for MLPF */
#define HFC_FX_FRAMEA_FW_START_MCK		( 0x50000000 )
#define HFC_FX_FRAMEA_LOGOUT			( 0x52000000 )
#define HFC_FX_FRAMEA_SHADOW_DOWN		( 0x58000000 )
#define HFC_FX_FRAMEA_SHADOW_UP			( 0x59000000 )

#define HFC_FX_FRAMEA_PORTSTATISTICS	( 0x68000000 )

/*----------------------------------------*/
/*--  PCI INT_Vector register			--*/
/*----------------------------------------*/
#define HFC_FX_HWERR_INT			0x04040404

#define HFC_FX_HBROADMCK			0x00000800

/*----------------------------------------*/
/*--  PCI INT_A register				--*/
/*----------------------------------------*/
#define HFC_FX_HWERR_INT_A			0xe0000000
#define HFC_FX_HWERR_MCK			0x80000000
#define HFC_FX_HWERR_PCI			0x60000000
#define	HFC_FX_HWERR_PCIUCEF		0x40000000
#define	HFC_FX_HWERR_PCIUCEN		0x20000000
#define	HFC_FX_HWERR_PCICE			0x10000000
#define HFC_FX_PCIE_SRAM_CE			0x08000000

// PORT STAUTS (offset 0x11)
#define HFC_FX_PCI_PCIERR_DETECTED	0x80
#define HFC_FX_PCI_EXGMCK			0x40
#define HFC_FX_PCI_BOOTRUN			0x20
#define HFC_FX_PCI_FCNSTOP			0x10
#define HFC_FX_PCI_TRACER_STOP		0x08

#define HFC_FX_PCI_PCICHK	(HFC_FX_PCI_EXGMCK | HFC_FX_PCI_BOOTRUN)

// PORT STAUTS (offset 0x12)
#define HFC_FX_PCI_CH_NOT_READY	0x80
#define HFC_FX_PCI_MPCHK		0x08
#define HFC_FX_CORE_CHKSTP		0x01

#define HFC_FX_WAKE_UP_FAILURE_FIVE	0x88

/************************************************************************
 * Config H/W check Retry Count
 ************************************************************************/
#define HFC_CONFIG_HW_CHECK_RETRY_INIT	5
#define HFC_CONFIG_HW_CHECK_RETRY_RECV	3	/* FCLNX-GPL-FX-215 */

/************************************************************************/
/* struct fw_init_tbl_fx												*/
/*		  f/w interface table of FIVE-FX								*/
/************************************************************************/
struct fw_init_tbl_fx {
	uint				dd_support_info ;		/* +0 drv support info	*/
#define	HFC_DDSP_DEFAULT	0x00000000
	uchar				init_tbl_rev ;			/* +4  init_table rev	*/
#define	HFC_FWINIT_REV_20	0x20
	uchar				flag ;					/* +5  max target num	*/
	uchar				resv0[2];				/* +6  					*/
	uint64_t			mb_addr ;				/* +8 Mailbox address	*/
	uint				xob_num ;				/* +10 xob num			*/
	uint				xrb_num ;				/* +14 xrb num			*/
	uint				slog_num ;				/* +18 soft log page num*/
	struct {
		uchar			core_no ;				/* +1c core no			*/
		uchar			master_core_no;			/* +1d master core no	*/
		uchar			available_core;			/* +1e available core no	*/
		uchar			resv1;					/* +1f					*/
	} core_info ;
	uchar				resv2[10] ;	 			/* +20					*/
	uchar				spma_mac_address[6] ;	/* +2a SPMA MAC Address */
	uchar				resv3[16] ;				/* +30					*/
	struct {									/* +40 fw link initialize */
		uchar			connect_type ;			/* see! adap_info.connect_type	*/
		uchar			trans_rate ;			/* +41 					*/
#define HFC_FX_TRUNKN	0x00					/* unknown				*/
#define HFC_FX_1GBPS	0x01					/* 100MB/s				*/
#define HFC_FX_2GBPS	0x02					/* 200MB/s				*/
#define HFC_FX_4GBPS	0x04					/* 400MB/s				*/
#define HFC_FX_8GBPS	0x08					/* 800MB/s				*/
#define HFC_FX_10GBPS	0x0a					/* 1GB/s				*/
#define HFC_FX_16GBPS	0x10					/* 1.6GB/s				*/
#define HFC_FX_40GBPS	0x28					/* 1.6GB/s				*/
#define HFC_16GBPS	16							/* 1600MB/s				*/
#define HFC_40GBPS	40							/* 4000MB/s				*/
															/* @MLPF STR */
		uchar			resv3;					/* +42  for MLPF        */
#define HFC_MLPF_MODE       0x80
#define HFC_NPIV_VALID      0x40
															/* @MLPF END */
		uchar			configure_flag ;		/* +43 					*/
#define	HFC_FX_FABRIC_VALID 	0x80
#define HFC_FX_PID_VALID		0x40
#define	HFC_FX_P2P_PID_VALID	0x20
#define HFC_FX_VF_ID_VALID		0x08
#define	HFC_FX_ALPA_VALID 		0x04

//#define	HFC_PNAME_VALID		0x10
//#define	HFC_NNAME_VALID		0x08

#define	HFC_FX_POSMAP_VALID		0x02
#define	HFC_FX_POSMAP_LISA		0x01
		uchar			resv4 ;					/* +44 */
		uchar			fabric_param ;			/* +45 */
		uchar			resv5 ;					/* +46 */
		uchar			assign_alpa ;			/* +47 */
		uchar			alpa_count ;			/* +48 */
		uchar			p2p_tgt_port_id[3] ;	/* +49 */
		uchar			resv6[2] ;				/* +4c */
		ushort			vf_id ;					/* +4e */
		uchar			resv7;					/* +50 */
		uchar			self_port_id[3];		/* +51 */
		uchar			resv8[12];				/* +54 */
		uchar			address_mode ;			/* +60 */
		uchar			resv9 ;					/* +61 */
		ushort			fcoe_vid ;				/* +62 */
		uchar			resv10[2];				/* +64 */
		uchar			mac_address[6] ;		/* +66 */
		uchar			resv11;					/* +6c */
		uchar			fc_map[3] ;				/* +6d */
		uint64_t		fabric_name	;			/* +70 */
		uchar			resv12[3];				/* +78 */
		uchar			d ;						/* +7b */
		uchar			resv13[4];				/* +7c */
	}fw_iocinfo ;
	uchar				pos_map[160] ;			/* +80 */
	uchar				active_alpa[32] ;		/* +120 */

	struct { /* SFP Information  */
		uchar			sfp_status;             /* +140 SFP Status */
#define	HFC_FX_SFP_INSTALL     0x80
#define	HFC_FX_SFP_VALID       0x40
		uchar			resv1[15];              /* +141 - +150 */
		uchar			sfp_type_name[HFC_SFP_TYPE_NAME_LEN];      /* +150 ASCII  */
		uchar			sfp_serial_no[HFC_SFP_SERIAL_NO_LEN];      /* +160 ASCII  */
		uchar			sfp_date_code[HFC_SFP_DATE_CODE_LEN];      /* +170 ASCII  */
		uint			sfp_validation_code;    /* +178 Hex    */
		uchar			resv7[4];               /* +17C - +17F */
	}sfp_info;

	struct {
		uint64_t	tx_frames ;							/* +180 */
		uint64_t	tx_words ;							/* +188 */
		uint64_t	rx_frames ;							/* +190 */
		uint64_t	rx_words ;							/* +198 */
		uint64_t	lip_count ;							/* +1A0 */
		uint64_t	nos_count ;							/* +1A8 */
		uint64_t	error_frames ;						/* +1B0 */
		uint64_t	dumped_frames ;						/* +1B8 */
		uint64_t	link_failure_count ;				/* +1C0 */
		uint64_t	loss_of_sync_count ;				/* +1C8 */
		uint64_t	loss_of_signal_count ;				/* +1D0 */
		uint64_t	primitive_seq_protocol_err_count ;	/* +1D8 */
		uint64_t	invalid_tx_word_count ;				/* +1E0 */
		uint64_t	invalid_crc_count ;					/* +1E8 */
		uchar  		resv8[15];							/* +1F0 - +1FE */
		uchar		fw_store_count;						/* +1FF */
	}portstatistics ;

	uint64_t			fw_bus_addr[MAX_FW_BUS_CNT] ;
												/* +200 - +xxx */
												/* xob/xrb/slog queue   */
												/* address (page addr)	*/
#define HFC_FX_INIT_TABLE_XOB_OFFSET	0x00
#define HFC_FX_INIT_TABLE_XRB_OFFSET	(0x40/sizeof(uint64_t))
#define HFC_FX_INIT_TABLE_SLOG_OFFSET	(0x180/sizeof(uint64_t))

	struct {									/* FLASH header format for online update */
		uint            sys_rev;                /* +600 Current F/W version */
		uint            online_up_rev;          /* +604 online update rev   */
		uchar           resv1[40];              /* +608 - +630  */
		uint            mcw_rev;                /* +630 MCW rev */
		uchar           resv2[12];              /* +634 - +640  */
		uint            config_rev;             /* +640 Config Register rev */
		uchar           resv3[108];             /* +644 - +6B0  */
		uint            hss_rev;                /* +6B0 HSS rev */
		uchar           resv4[12];              /* +6B4 - +6C0  */
		uint            utl_rev;                /* +6C0 UTL rev */
		uchar           resv5[60];              /* +6C4 - +700  */
	}fls_hdr;

	uchar				resv13[256] ;			/* +700 - +800 */
	uchar				resv14[2048];			/* +800 */
};

/************************************************************************/
/* struct seg_info_fx													*/
/*		  data segment infomation of FIVE-FX							*/
/************************************************************************/
struct seg_info_fx {
	uint				xob_segno ;		/* byte 0	:xob no				*/
										/* byte 1-3 :seg_info no		*/
										/*  sequence# in xob			*/
	uint				seg_len ;

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_) || defined(_LINUX_COM)

/* little Endian */
#define HFC_SEG_LEN_F		0x00000080	/* seg_addr is next seg_info_list addr */
#define HFC_SEG_LEN_L		0x00000040	/* last seg_info_list addr		*/
#define HFC_SEL_LEN_MASK	0xffffff3f	/* Mask							*/

#else

/* Big Endian */
#define HFC_SEG_LEN_F		0x80000000	/* seg_addr is next seg_info_list addr */
#define HFC_SEG_LEN_L		0x40000000	/* last seg_info_list addr		*/
#define HFC_SEL_LEN_MASK	0x3fffffff	/* Mask							*/

#endif

	uint64_t			seg_addr ;		/* HFC_SEG_LEN_L = 0 :			*/
										/*	data bus address			*/
										/* HFC_SEG_LEN_L = 1 :			*/
										/*	next seg_info_list			*/
										/*	 bus address				*/
};

/************************************************************************/
/* FCP structures														*/
/************************************************************************/

/* FCP-LUN structure */
typedef struct fcp_lun_fx {
	ushort	lun ;					/* lun#		(byte1)		*//* FCLNX-GPL-0343 */
	uchar	rsv2_7[6] ;				/* reserved (byte2-7)	*/
} fcp_lun_fx_t ;

/* FCP-CNTL structure */
typedef struct fcp_cntl_fx {
	uchar	rsv0;					/* reserved */

	uchar	qtype;					/* tag-q type */
#define	HFC_QTYPE_SIMPLE_Q	0		/* simple queue */
#define	HFC_QTYPE_HEAD_OF_Q	1		/* head of queue */
#define	HFC_QTYPE_ORDERED_Q	2		/* ordered queue */
#define	HFC_QTYPE_ACA_Q		4		/* ACA queue */
#define	HFC_QTYPE_UNTAGGED	5		/* untagged */

	uchar	task_mgm ;
#define HFC_TARM_TASK	0x80		/* terminate task */
#define HFC_CLEAR_ACA	0x40		/* clear aca */
#define HFC_TARGET_RST	0x20		/* target reset */
#define HFC_LUN_RST		0x10		/* lun reset */
#define HFC_CLEAR_TASK	0x04		/* clear task set */
#define HFC_ABORT_TASK	0x02		/* abort task set */

	uchar	data_type ;
#define HFC_READ_DATA	0x02		/* read data */
#define HFC_WRITE_DATA	0x01		/* write data */
} fcp_cntl_fx_t;

/*
 * FCP-CMD-IU Payload
 */
typedef struct fcp_cmd_iu_fx {
	fcp_lun_fx_t	fcp_lun;		/* FCP-LUN	*/
	fcp_cntl_fx_t	fcp_cntl;		/* FCP-CNTL */
	uchar		fcp_cdb[16];	/* FCP-CDB	*/
	uint		fcp_dl;			/* FCP-DL	*/
} fcp_cmd_iu_fx_t;

/* FCPRB */
typedef struct resp_iu_hdr {
	uint				resv0 ;			/* +0 - +3 */
	uint				resv1 ;			/* +4 - +7 */
	ushort				retry_delay ;	/* +8 RETRY DELAY TIMER */
	uchar				fcp_status2 ;	/* +A RSP_FLAG					*/
#define HFC_XRB_RESID_UNDER 	0x08  	/* Data underrun				*/
#define HFC_XRB_RESID_OVER		0x04  	/* Data overrun					*/
#define HFC_XRB_SNSLEN_VALID	0x02  	/* sense_length valid			*/
#define HFC_XRB_RSPLEN_VALID	0x01  	/* response_length valid		*/
	uchar				scsi_status ;	/* +B SCSI STATUS CODE			*/
	uint				resid ;			/* +C FCP_RESID : remain count	*/
	uint				sns_len ;		/* +10 FCP_SNS_LEN : Sense Length */
	uint				resp_len ;		/* +14 FCP_RSP_LEN : resp_info length */
} resp_iu_hdr_t ;

/* VFT_Header */
typedef struct {
	uchar				exrctl ;		/* +0 ExRCTL */
	uchar				resv1 ;			/* +1 */
	ushort				prio_vfid ;		/* +3 Priority/VF_ID */
	uchar				hopct ;			/* +4 HopCt */
	uchar				resv2[1] ;		/* +5 */
	ushort				receive_payload_max_length ; /* +6 - 7 */
} vft_hdr_t ;

/************************************************************************/
/* struct xob_fx														*/
/*		  xob format of FIVE-FX											*/
/************************************************************************/

struct xob_fx {
	uchar				flag ;					/* +0 XOB_Flag */
#define HFC_XOB_OLD			0x00	/* former format					*/
#define HFC_XOB_I			0x01	/* initiater mode xob format		*/
#define HFC_XOB_TD			0x02	/* target mode data transfer order	*/
#define HFC_XOB_TR			0x03	/* target mode status send order	*/
#define HFC_XOB_TC3			0x07	/* REC/SRR response order			*/
#define HFC_XOB_C			0x08					/* SCSI CANCEL c_xob format	*//* FCLNX-GPL-FX-014 */
#define HFC_XOB_CI			(HFC_XOB_C | HFC_XOB_I)	/* Reset CMD + SCSI CANCEL ci_xob format *//* FCLNX-GPL-FX-014 */

	uchar				skip ;					/* +1 XOB_SKIP */
#define HFC_XOB_SKIP		0x80	/* =1 xob queue						*/
									/* =0 reset/cancel/DID_NOT			*/

	ushort				seg_cnt ;	/* +2 total SEG-INFO Count */
	ushort				prli_parm ;				/* +4 PRLI_PARM<2-3> */
	ushort				mfsize ;				/* +6 MFSIZE */
	uchar				request_pcp ;			/* +8 Request PCP */
	uchar				trans_d_id[3] ;			/* +9 Trans D_ID */
	uchar				queue_no ;				/* +0C Queue_no	*/
	uchar				trans_s_id[3] ;			/* +0D Trans S_ID */
	uint64_t			drv_work ;				/* +10 Driver Used Area */
	uint64_t			seg_5th ;				/* +18 SEG-INFO-LIST Address (SEG-INFO #4) */
	fcp_cmd_iu_fx_t		fcp_cmd ;				/* +20 FCP_CMND_IU */
	struct seg_info		seg_info_xob[4] ;		/* +40 SEG-INFO #0-#3 */
} ;

/* FCLNX-GPL-FX-014 Start */
struct c_xob_fx {
	uchar				flag ;					/* +0 XOB_Flag */
	uchar				skip ;					/* +1 XOB_SKIP */
	ushort				seg_cnt ;				/* +2 total SEG-INFO Count */
	ushort				prli_parm ;				/* +4 PRLI_PARM<2-3> */
	ushort				mfsize ;				/* +6 MFSIZE */
	uchar				request_pcp ;			/* +8 Request PCP */
	uchar				trans_d_id[3] ;			/* +9 Trans D_ID */
	uchar				queue_no ;				/* +0C Queue_no	*/
	uchar				trans_s_id[3] ;			/* +0D Trans S_ID */
	uint64_t			drv_work ;				/* +10 Driver Used Area */
	uchar				resv1[8];				/* +18 reserve */
	fcp_cmd_iu_fx_t		fcp_cmd ;				/* +20 FCP_CMND_IU */
	uchar				cancel_ctl;				/* +40 CANCEL_CTL */
	uchar				cancel_nexus;			/* +41 CANCEL_Nexus */
	uchar				resv2[6];				/* +42 reserve */					
	uint64_t			cancel_drv_work ;		/* +48 Driver Used Area for cancel target*/
	uchar				resv3[48];				/* +50 reserve */					
} ;
/* FCLNX-GPL-FX-014 End*/


/************************************************************************/
/* struct xrb_fx														*/
/*		  xrb format of FIVE-FX											*/
/************************************************************************/

/* xcrb : XCRB */
typedef struct xcrb {
	uchar				type ;		/* +0 XRB_Type						*/
#define HFC_XRB_OLD		0x00		/* former format					*/
#define HFC_XRB_I		0x01		/* initiater mode xrb format		*/
#define HFC_XRB_TC		0x02		/* target mode command receive notification format */
#define HFC_XRB_TE		0x03		/* target mode data transfer complete notification format */
#define HFC_XRB_TR		0x04		/* target mode status send complete notification format */
#define HFC_XRB_TC3		0x07		/* REC/SRR receive complete notification format	*/
	uchar				skip ;		/* +1 XRB_SKIP						*/
#define HFC_XRB_SKIP	0x80		/* =0 xrb dequeue					*/
	uchar				resv1[6] ;	/* +2 - +7 							*/
	uchar				esw ;		/* +8 ESW 							*/
#define HFC_ESW_MEINT_REPO	0x08	/*    MEINT_REPORT : get soft_log (valid SSN)	*/
	uchar				softlog ;	/* +9 Softlog#						*/
	ushort				sbc ;		/* +A SBC							*/
	uchar				fsb ;		/* +C FSB 							*/
#define HFC_FSB_RETRY	0x80		/* Retry							*/
#define HFC_FSB_IL		0x40		/* IL								*/
#define HFC_FSB_PC		0x20		/* PC								*/
#define HFC_FSB_ICC		0x02		/* ICC								*/
#define HFC_FSB_MASK	0x2E		/* PC/CDC/CCC/ICC					*/
	uchar				err_code[3] ;	/* +D ERRCODE 					*/
#define HFC_ICC_TRANSPORT_FAULT	0x02
#define HFC_ICC_TIMEOUT	0x03
#define HFC_ICC_NO_RESP	0x04
	uint64_t			hfc_pkt ;	/* +10 Driver Used Area				*/
	uchar				resv2[1] ;	/* +18								*/
	uchar				trans_s_id[3] ;	/* +19 - +1B Trans S_ID			*/
	uchar				resv3[4] ;	/* +1C - +1F						*/
} xcrb_t ;

/* xcrbchk : first 4byte of XCRB */
typedef struct xcrbchk {
	uchar				resv[3] ;	/* +0 - +2 */
	uchar				valid ;		/* +3 XRB_Valid */
} xcrbchk_t ;

/* xrb_fx */
struct xrb_fx {
	xcrb_t				xcrb ;			/* +0 XCRB */
	resp_iu_hdr_t		resp_iu1 ;		/* +20 FCPRB #1 */
	uchar				resv1[68];		/* +38 - +7B */
	xcrbchk_t			xcrbchk ;		/* +7C - 7F */
	uchar				resv2[24] ;		/* +80 - 97 */
	uchar				fcp_info[360] ;	/* +98 FCP_INFO */
} ;



/************************************************************************/
/* Mailbox Structure													*/
/************************************************************************/
/************************************************************************/
/* PORT_ID(SCSI_ID)														*/
/************************************************************************/
struct FS_ACC_FX {
		uchar	resv1[8] ;		/* +0 - +7 */
		ushort	rsp_code ;		/* +8 Response Code */
#define	HFC_FS_ACC	0x8002		/* Accept Response */
#define	HFC_FS_RJT	0x8001		/* Reject Response */
		uchar	resv2[6] ;		/* +A - +F */
		uint	port_id[2032] ; /* +10 - +7FC */
} ;

/* FC-PH header */
typedef struct fcph_hdr {
	uchar				rctl ;					/* +0 RCTL				*/
#define HFC_FRMSNDRCV_FCGS	0x02
#define HFC_RCVFRMRSP_FCGS	0x03
#define HFC_FRMSNDRCV_ELS	0x22
#define HFC_RCVFRMRSP_ELS	0x23

	uchar				d_id[3] ;				/* +1 D_ID				*/
	uchar				cs_ctl ;				/* +4 CS_CTL			*/
	uchar				s_id[3] ;				/* +5 S_ID				*/
} fcph_hdr_t ;

/* Ethernet Header (16byte) */
typedef struct eth_hdr16b {
	uchar				resv1[8] ;				/* +0 RESV				*/
	ushort				tpid ;					/* +8 TPID(802.1Q Tag)	*/
	uchar				request_pcp ;			/* +10 Request PCP		*/
	uchar				resv2 ;					/* +B					*/
	ushort				lentype ;				/* +C Length/Type		*/
	uchar				ver ;					/* +E Ver : Use former 4 bit	*/
	uchar				resv3 ;					/* +F					*/
} eth_hdr16b_t ;

/* MB_INTRQ : Defined in Receive Payload */
typedef struct affected_portid {
	uchar				resv1[2] ;				/* +0 Reserved			*/
	uint				rscn_event_qualifier;	/* +2 RSCN Event Qualifier */
	ushort				address_format ;		/* +6 Address Format	*/
} affected_portid ;

/************************************************************************/
/* struct mb_cmd1/mb_rsp1												*/
/*		  mailbox initiate(DRVIOCTL1)									*/
/*		  (1) LOGIN														*/
/*		  (2) ELS(PRLO/GCS_ID,PDISC,GID_PN) 							*/
/************************************************************************/

struct mailbox_fx {
	/* mb_req : Mailbox Request ----------------------------------------*/
	struct mbreq {
		/* Mailbox Request Header */
		uint			mb_code ;				/* +0 Mailbox code		*/
#define HFC_MBCMD_CORESTART		0x60200000		/* Core_Start			*/
#define HFC_MBCMD_OFFLINEMB		0x60210000		/* Offline_mb			*/
#define HFC_MBCMD_SHADOWUP		0x60300000		/* Shadow Up			*/
#define HFC_MBCMD_ADDPORTID		0x60400000		/* add PORT_ID			*/
#define HFC_MBCMD_DELPORTID		0x60410000		/* delete PORT_ID		*/
#define HFC_MBCMD_MIHLOG		0x80150000		/* MIH LOG				*/
#define	HFC_MIHLOG_LOGID_SOFT		  0x10		/* MIH LOG : Soft Log	*/
#define	HFC_MIHLOG_LOGID_LINKIN		  0x11		/* MIH LOG : Link incident */
#define HFC_MBCMD_DIAG			0x80160000		/* DIAG 				*/
#define HFC_MBCMD_DIAGREG		0x80160011		/* DIAG REG(BOOT)		*/
#define HFC_MBCMD_DIAGMEM		0x80160012		/* DIAG	MEM				*/
#define HFC_MBCMD_DIAGILOOP		0x80160013		/* DIAG	I-LOOP			*/
#define HFC_MBCMD_DIAGDMA		0x80160014		/* DIAG	DMA				*/
#define HFC_MBCMD_DIAGELOOP		0x80160015		/* DIAG	E-LOOP			*/
#define HFC_MBCMD_LDCHTRC		0x80200000		/* Load CH Trace LOG	*/
#define HFC_MBCMD_LINKINIT		0x90000000		/* Link_ini				*/
#define HFC_MBCMD_FLOGI			0xB0300000		/* FLOGI				*/
#define HFC_MBCMD_PLOGI			0xB0310000		/* PLOGI				*/
#define HFC_MBCMD_PDISC			0xB0320000		/* PDISC				*/
#define HFC_MBCMD_CSCSI_WAIT_DMA	0xB0380000		/* CANCEL_SCSI			*/
#define HFC_MBCMD_CSCSIMIH		0xB0388000		/* CANCEL_SCSI+MIH LOG	*/
#define HFC_MBCMD_CSCSI_WITHOUT_DMA	0xB0384000		/* CANCEL_SCSI without wait for DMA Response */
#define HFC_MBCMD_SNDRCV		0xB0600000		/* FRMSNDRCV			*/
#define HFC_MBCMD_SNDRCV_GIDFT	0xB0600200		/* FRMSNDRCV:GID-FT/GPN-ID	*/

/* Extended Link Service Frame sended with FRMSNDRCV */
#define HFC_SNDRCV_PRLI				 0x01		/* Frame_type : PRLI	*/
#define HFC_SNDRCV_PRLO				 0x02		/* Frame_type : PRLO	*/
#define HFC_SNDRCV_SCR				 0x03		/* Frame_type : SCR		*/
#define HFC_SNDRCV_LOGO				 0x04		/* Frame_type : LOGO	*/
#define HFC_SNDRCV_AUTH_REJECT		 0x05		/* Frame_type : AUTH_Reject	*/
#define HFC_SNDRCV_AUTH_NEGO		 0x06		/* Frame_type : AUTH_Negitiate	*/
#define HFC_SNDRCV_DHCHAP_CHALLENGE	 0x07		/* Frame_type : DHCHAP_Challeng	*/
#define HFC_SNDRCV_DHCHAP_REPLY		 0x08		/* Frame_type : DHCHAP_Reply	*/
#define HFC_SNDRCV_DHCHAP_SUCCESS	 0x09		/* Frame_type : DHCHAP_Success	*/
#define HFC_SNDRCV_EVFP_SYNC		 0x0a		/* Frame_type : EVFP_SYNC		*/
#define HFC_SNDRCV_EVFP_COMMIT		 0x0b		/* Frame_type : EVFP_COMMIT		*/

/* FC-GS Frame sended with FRMSNDRCV */
#define HFC_SNDRCV_GCS_ID			 0x11		/* Frame_type : GCS_ID	*/
#define HFC_SNDRCV_GID_PN			 0x12		/* Frame_type : GID_PN	*/
#define HFC_SNDRCV_GPN_ID			 0x13		/* Frame_type : GPN_ID	*/
#define HFC_SNDRCV_GID_FT			 0x14		/* Frame_type : GID_FT	*/
#define HFC_SNDRCV_RFT_ID			 0x15		/* Frame_type : RFT_ID	*/
#define HFC_SNDRCV_RFF_ID			 0x16		/* Frame_type : RFF_ID	*/
#define HFC_SNDRCV_GPN_FT			 0x17		/* Frame_type : GPN_FT	*/
		uchar			region_no ;				/* +4 Region No 		*/
		uchar			resv1[2] ;				/* +5 					*/
#define	HFC_MBREQ_HDR_RSV	0x00
		uchar			timer ;					/* +7 Timer				*/
#define HFC_TIMER_FRMSNDRCV		20				/* FRMSNDRCV : 20s		*/
		/* type : Request type */
		union {
			/* common cntrol information */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv0[502] ;			/* +a - +1FF			*/
			}cmn_ctl ;
			/* core_start */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv1[2] ;				/* +A - +B				*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t	self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv3[448] ;			/* +40 - +1FF			*/
			} core_start ;
			/* offline_mb */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv1[2] ;				/* +A - +B				*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t	self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv3[448] ;			/* +40 - +1FF			*/
			} offline_mb ;
			/* add_port_id */
			struct {
				uchar	resv1[4] ;				/* +8 - +B				*/
				uint	trans_s_id ;			/* +C Trans S_ID		*/
				uint64_t	self_wwpn ;			/* +10 Self WWPN		*/
				uchar	resv2[488] ;			/* +18 - +1FF			*/
			} add_port_id ;
			/* delete_port_id */
			struct {
				uchar	resv1[4] ;				/* +8 - +B				*/
				uint	trans_s_id ;			/* +C Trans S_ID		*/
				uint64_t	self_wwpn ;			/* +10 Self WWPN		*/
				uchar	resv2[488] ;			/* +18 - +1FF			*/
			} delete_port_id ;
			/* mih_log */
			struct {
				uint	trans_d_id ;			/* +8 Trans D_ID		*/
				uint	trans_s_id ;			/* +C Trans S_ID		*/
				uint64_t	driver_used_area ;	/* +10 Driver Used Area	*/
				uchar	resv1[488] ;			/* +18 - +1FF			*/
			} mih_log ;
			/* load_ch_trace_log : Load CH Trace LOG */
			struct {
				uint64_t	logo_list_addr ;	/* +8 Logout_List_Address	*/
				uchar	resv1[496] ;			/* +08 - +1FF			*/
			} load_ch_trace_log ;
			/* cscsi : CANCEL_SCSI */
			struct {
				uchar	frm_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_CLASS			*/
				uchar	resv1[2] ;				/* +A - +B				*//* FCLNX-GPL-FX-222 */
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr ;		/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uchar	resv3[8] ;				/* +30 - +37			*/
				uchar	resv4[8] ;				/* +38 - +3F			*/
				uchar	cnexus ;				/* +40 CANCEL_Nexus		*/
#define	HFC_MBREQ_CSCSI_P	0x80				/* P Nexus				*/
#define	HFC_MBREQ_CSCSI_I	0x08				/* I Nexus				*/
#define	HFC_MBREQ_CSCSI_T	0x0C				/* T Nexus				*/
#define	HFC_MBREQ_CSCSI_L	0x0E				/* L Nexus				*/
#define	HFC_MBREQ_CSCSI_E	0x0D				/* E Nexus				*/
				uchar	resv5 ;					/* +41					*/
				uchar	resv6 ;					/* +42					*/
				uchar	plogi_param ;			/* +43 PLOGI_Param		*/
				uint	resv7 ;					/* +44 - +47			*/
				fcp_lun_fx_t	fcp_lun ;		/* +48 FCP_LUN			*/
				uint64_t	driver_used_area;	/* +50 Driver Used Area */
				uchar	resv8[424] ;			/* +58 - +1FF			*/
			} cscsi ;
			/* linkini : Link_ini */
			struct {
				uchar	al_pa ;					/* +8 AL-PA				*/
				uchar	link_speed ;			/* +9 Link_Speed		*/
#define HFC_MBREQ_LI_1GBPS		1				/* 100MB/s				*/
#define HFC_MBREQ_LI_2GBPS		2				/* 200MB/s				*/
#define HFC_MBREQ_LI_4GBPS		4				/* 400MB/s				*/
#define HFC_MBREQ_LI_8GBPS		8				/* 800MB/s				*/
#define HFC_MBREQ_LI_10GBPS		10				/* 1GB/s				*/
#define HFC_MBREQ_LI_16GBPS		16				/* 1600MB/s				*/
#define HFC_MBREQ_LI_40GBPS		40				/* 4000MB/s				*/
				uchar	connect_type ;			/* +A Con_Type			*/
#define HFC_MBREQ_CONTYPE_AUTO	0x00			/* Auto					*/
#define HFC_MBREQ_CONTYPE_P2P	0x01			/* Point to Point		*/
#define HFC_MBREQ_CONTYPE_AL	0x03			/* Arbitrated Loop		*/
#define HFC_MBREQ_CONTYPE_FPORT	0xF0			/* F_PORT				*/
				uchar	resv1[9] ;				/* +B - +13				*/
				eth_hdr16b_t eth_hdr ;			/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				uchar	cna_flag ;				/* +28 CNA_Flag			*/
#define HFC_VID_CTRL_NOSET	0x00				/* Don't set VID		*/
#define HFC_VID_CTRL_SUGGEST	0x40			/* Suggest VID to FCF-MAC	*/
#define HFC_VID_CTRL_FORCE	0x80				/* Force use preset VID	*/
				uchar	resv3 ;					/* +29					*/
				ushort	pref_vid ;				/* +2A Preferred VID	*/
				uchar	resv4[4] ;				/* +2C - +2F			*/
				uint64_t self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv5[448] ;			/* +40 - +1FF			*/
			} link_init ;
			/* flogi : FLOGI */
			struct {
				uchar	frm_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv1[2] ;				/* +A - +B				*/
				vft_hdr_t vft_hdr ;				/* +C VFT_Header		*/
				eth_hdr16b_t eth_hdr ;			/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv3[3] ;				/* +40 - +42			*/
				uchar	flogi_param ;			/* +43 FLOGI_Param (Common Service Parameter)	*/
#define FLOGI_PARAM_QBC			0x80			/* Query Buffer Condition(QBC) */
#define FLOGI_PARAM_SECURITY	0x40			/* Security 			*/
#define FLOGI_PARAM_VF			0x20			/* Virtual Fabrics		*/
#define FLOGI_PARAM_NPIV		0x10			/* NPIV					*/
				uchar	resv4[4] ;				/* +44 - +47			*/
				uchar	resv5[440] ;			/* +48 - +1FF			*/
			} flogi ;
			/* plogi : PLOGI */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv1[2] ;				/* +A - +B				*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t	self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv3[3] ;				/* +40 - +42			*/
				uchar	plogi_param ;			/* +43 PLOGI_Param (Common Service Parameter)	*/
#define PLOGI_PARAM_QBC	0x80					/* Query Buffer Condition(QBC) */
#define PLOGI_PARAM_SECURITY	0x40			/* Security 			*/
				uchar	resv4[4] ;				/* +44 - +47			*/
				uchar	resv5[440] ;			/* +48 - +1FF			*/
			} plogi ;
			/* pdisc : PDISC */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				uchar	resv1[2] ;				/* +A - +B				*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv2[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	self_wwpn ;			/* +30 Self WWPN		*/
				uint64_t	self_wwnn ;			/* +38 Self WWNN		*/
				uchar	resv3[3] ;				/* +40 - +42			*/
				uchar	plogi_param ;			/* +43 PLOGI_Param (Common Service Parameter)	*/
				uchar	resv4[4] ;				/* +44 - +47			*/
				uchar	resv5[440] ;			/* +48 - +1FF			*/
			} pdisc ;
			/* frmsndrcv : FRMSNDRCV */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +A Payload Length	*/
#define HFC_LOGO_LENGTH		0x14
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv1[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	payload ;			/* +30 Payload Address	*/
				uint64_t	receive_payload ;	/* +38 Receive Payload Address	*/
				uchar	resv2[448] ;			/* +40 - +1FF			*/
			} frmsndrcv ;
		} type ;
	} mb_init ;
	/* mb_resp : Mailbox Response --------------------------------------*/
	struct mbresp {
		/* Mailbox Response Header */
		uchar			flag ;					/* +0 FLAG				*/
#define	HFC_MBRESFLAG_VALID 0x80
		uchar			resv2[3] ;				/* +1 - +3				*/
		uchar			esw ;					/* +4 ESW				*/
#define HFC_MB_MAINT_REPORT 0x08				/* ESW : MAINT_REPORT	*/
		uchar			ssn		 ;				/* +5 Softlog#			*/
		ushort			sbc ;					/* +6 SBC				*/
		uint			mb_code ;				/* +8 Mailbox code		*/
		uchar			fsb ;					/* +C FSB				*/
#define HFC_FSB_RETRY	0x80					/* Retry				*/
#define HFC_FSB_IL		0x40					/* IL					*/
#define HFC_FSB_PC		0x20					/* PC					*/
#define HFC_FSB_ICC		0x02					/* ICC					*/
#define HFC_FSB_MASK	0x2E					/* PC/CDC/CCC/ICC		*/
		uchar			err_code[3] ;			/* +D ERRCODE			*/
/* SFP Failure */
#define HFC_FSB_ERR_LINKINI1	0xE01001
#define HFC_FSB_ERR_LINKINI2	0xE11002
#define HFC_FSB_ERR_LINKINI3	0xE11003
#define HFC_FSB_ERR_LINKINI4	0xE11004

/* Class not support */
#define HFC_FSB_ERR_CLASS_NOT_SUPPORT1	0xD00205
#define HFC_FSB_ERR_CLASS_NOT_SUPPORT2	0xD00305
		/* type : Response Type */
		union {
			/* mih : MIH LOG */
			struct {
				uchar	resv1 ;					/* +10					*/
				uchar	mih_slog ;				/* +11 MIH_Softlog# area start entry num	*/
				ushort	mih_sbc ;				/* +12 MIH_SBC : Log size (byte)	*/
				uchar	resv2[492] ;			/* +14 - +1FF			*/
			} mih_log ;
			/* diag : DIAG */
//			struct {
//				uchar	result ;				/* +10 Result */
//#define HFC_SFP_VALID		0x00				/* SFP VALID			*/
#define HFC_SFP_NOT_INSTALLED	0x80			/* SFP is not installed	*/
#define HFC_SFP_IF			0x40				/* SFP interface error	*/
#define HFC_SFP_NOT_SUPPORTED	0x20			/* SFP type is not supported */
#define HFC_SFP_ALARM		0x10				/* SFP alarm is turned on */
//				uchar	resv[3] ;				/* +11 - +13			*/
//			} diag ;
			/* load_ch_trace_log */
			struct {
				uchar	fw_trc_ptr[8] ;			/* +10 FW Trace Pointer	*/
				/* (SEG#00) */
				ushort	sp0 ;					/* +18 SP	(SEG#00)	*/
				ushort	ep0	;					/* +1A EP	(SEG#00)	*/
				uchar	ctl0 ;					/* +1C CTL	(SEG#00)	*/
				uchar	resv0 ;					/* +1D					*/
				ushort	cp0 ;					/* +1E EP	(SEG#00)	*/
				/* (SEG#01) */
				ushort	sp1 ;					/* +20 SP	(SEG#01)	*/
				ushort	ep1	;					/* +22 EP	(SEG#01)	*/
				uchar	ctl1 ;					/* +24 CTL	(SEG#01)	*/
				uchar	resv1 ;					/* +25					*/
				ushort	cp1 ;					/* +26 EP	(SEG#01)	*/
				/* (SEG#02) */
				ushort	sp2 ;					/* +28 SP	(SEG#02)	*/
				ushort	ep2	;					/* +2A EP	(SEG#02)	*/
				uchar	ctl2 ;					/* +2C CTL	(SEG#02)	*/
				uchar	resv2 ;					/* +2D					*/
				ushort	cp2 ;					/* +2E EP	(SEG#02)	*/
				/* (SEG#03) */
				ushort	sp3 ;					/* +30 SP	(SEG#03)	*/
				ushort	ep3	;					/* +32 EP	(SEG#03)	*/
				uchar	ctl3 ;					/* +34 CTL	(SEG#03)	*/
				uchar	resv3 ;					/* +35					*/
				ushort	cp3 ;					/* +36 EP	(SEG#03)	*/
				/* Frame Trace Pointer */
				ushort	sp4 ;					/* +38 SP	(SEG#03)	*/
				ushort	ep4	;					/* +3A EP	(SEG#03)	*/
				uchar	ctl4 ;					/* +3C CTL	(SEG#03)	*/
				uchar	resv4 ;					/* +3D					*/
				ushort	cp4 ;					/* +3E EP	(SEG#03)	*/
			} load_ch_trace_log ;
			/* cscsi : CANCEL_SCSI */
			struct {
				uchar	resv1 ;					/* +10					*/
				uchar	mih_softlog ;			/* +11 MIH_Softlog#		*/
				ushort	mih_sbc ;				/* +12 MIH_SBC			*/
				uchar	resv2[492] ;			/* +14 - +1FF			*/
			} cscsi ;
			/* linkini : Link_ini */
			struct {
				uchar	connect_type ;			/* +10 Connection_type	*/
				uchar	trans_rate ;			/* +11 Transfer_rate	*/
				uchar	link_config_flag ;		/* +12 Link_Configure_Flag */
				uchar	assign_alpa ;			/* +13 Assign ALPA		*/
				uchar	alpa_count ;			/* +14 ALPA_Count		*/
				uchar	link_con_type ;			/* +15 Link_Con_Type	*/
#define HFC_VF_ID_VALID	0x08					/* =1 VF_ID Valid		*/
#define HFC_FX_ALPA_VALID	0x04				/* =1 Active ALPA/Acquired ALPA Valid	*/
//#define HFC_POSMAP_VALID	0x02				/* =1 position-map Valid	*/
#define HFC_LOOP_PROCEDURE	0x01				/* =0 LILP, =1 LISA	(Loop procedure)	*/
				ushort	vf_id ;					/* +16 VF_ID (exclude first 4bit)		*/
				uchar	resv1[2] ;				/* +18 - +19			*/
				uchar	fcoe_vid ;				/* +1A FCoE VID (exclude first 4bit)	*/
				uchar	fpsp ;					/* +1C FP, SP (first 2bit)	*/
#define HFC_FPMA		0x80					/* FP bit of Discovery Advertisement frame */
#define HFC_SPMA		0x40					/* SP bit of Discovery Advertisement frame */
				uchar	resv2[5] ;				/* +1D - +21 			*/
				uchar	mac_address[6] ;		/* +22 MAC address		*/
				uchar	resv3[5] ;				/* +28 - +2C			*/
				uchar	fc_map[3] ;				/* +2D FC-MAP			*/
				uint64_t	fabric_name ;		/* +30 Fabric_Name		*/
				uchar	resv4[3] ;				/* +38 - +3A			*/
				uchar	d ;						/* +3B D (last 1bit)	*/
#define HFC_DBIT		0x01 ;					/* D bit of FIP FKA_ADV_Period descriptor in Discovery Advertisement frame */
				uint	fka_adv_period ;		/* +3C FKA_ADV_PERIOD	*/
				uchar	acquired_alpa[32] ;		/* +40 Acquired ALPA	*/
				uchar	resv5[32] ;				/* +60 - +7F			*/
				uchar	position_map[128] ;		/* +80 - +FF			*/
				uchar	resv6[256] ;			/* +100 - +1FF			*/
			} linkini ;
			/* flogi : FLOGI */
			struct {
				uchar	class ;					/* +10 Class			*/
				uchar	resv1 ;					/* +11					*/
				ushort	flogi_max_frame_size ;	/* +12 FLOGI_Max_Frame_Size	*/
				uchar	resv2 ;					/* +14 MLPF_Flag		*/
#define HFC_MLPF_FLAG_MLPF	0x80				/* =1 FC/CNA Share		*/
#define HFC_NPIV_VALID	0x40					/* =1 NPIV Valid		*/
				uchar	flogi_config_flag ;		/* +15 FLOGI_Configure_Flag	*/
#define HFC_FL_FABRIC_EXIST	0x80				/* =1 Fabric exist		*/
#define HFC_FL_PID_VALID	0x40				/* =1 PORT_ID Valid		*/
#define HFC_FL_P2P_PID_VALID	0x20			/* =1 PtoP_Target_PORT_ID Valid	*/
				uchar	resv3 ;					/* +16					*/
				uchar	flogi_rsp_param ;		/* +17 FLOGI_RSP_Param	*/
				uchar	resv4 ;					/* +18					*/
				uchar	recv_d_id[3] ;			/* +19 Recv D_ID		*/
				uchar	resv5 ;					/* +1C					*/
				uchar	recv_s_id[3] ;			/* +1D Recv S_ID		*/
				uint64_t	target_wwpn ;		/* +20 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +28 Target WWNN		*/
				uchar	resv6 ;					/* +30					*/
				uchar	assign_portid[3] ;		/* +31 Assign Port_ID	*/
				uchar	resv7 ;					/* +34					*/
				uchar	p2p_tgt_port_id[3] ;	/* +35 PtoP_Target_PORT_ID	*/
				uint	r_a_tov ;				/* +38 R_A_TOV			*/
				uchar	resv8[452] ;			/* +3C - +1FF			*/
			} flogi ;
			/* plogi : PLOGI */
			struct {
				uchar	class ;					/* +10 Class			*/
#define HFC_PLOGI_CLASS6	0x20				/* Class 6 Support		*/
#define HFC_PLOGI_CLASS4	0x08				/* Class 4 Support		*/
#define HFC_PLOGI_CLASS3	0x04				/* Class 3 Support		*/
#define HFC_PLOGI_CLASS2	0x02				/* Class 2 Support		*/
#define HFC_PLOGI_CLASS1	0x01				/* Class 1 Support		*/
				uchar	resv1 ;					/* +11					*/
				ushort	plogi_max_frame_size ;	/* +12 PLOGI_Max_Frame_Size	*/
				uchar	resv2[3] ;				/* +14 - +16			*/
				uchar	plogi_param ;			/* +17 PLOGI_Param		*/
				uchar	resv3 ;					/* +18					*/
				uchar	recv_d_id[3] ;				/* +19 Recv D_ID		*/
				uchar	resv4 ;					/* +1C					*/
				uchar	recv_s_id[3] ;				/* +1D Recv S_ID		*/
				uint64_t	target_wwpn ;		/* +20 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +28 Target WWNN		*/
				uchar	resv5[464] ;			/* +30 - +1FF			*/
			} plogi ;
			/* pdisc : PDISC */
			struct {
				uchar	class ;					/* +10 Class			*/
				uchar	resv1 ;					/* +11					*/
				ushort	plogi_max_frame_size ;	/* +12 PLOGI_Max_Frame_Size	*/
				uchar	resv2[3] ;				/* +14 - +16			*/
				uchar	plogi_param ;			/* +17 PLOGI_Param		*/
				uchar	resv3 ;					/* +18					*/
				uchar	recv_d_id[3] ;				/* +19 Recv D_ID		*/
				uchar	resv4 ;					/* +1C					*/
				uchar	recv_s_id[3] ;				/* +1D Recv S_ID		*/
				uint64_t	target_wwpn ;		/* +20 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +28 Target WWNN		*/
				uchar	resv5[464] ;			/* +30 - +1FF			*/
			} pdisc ;
			/* frmsndrcv : FRMSNDRCV */
			struct {
				uchar	resv1[2] ;				/* +10 - +11			*/
				ushort	recv_payload_length ;	/* +12 Receive Payload length	*/
				uchar	resv2[492] ;			/* +14 - +1FF			*/
			} frmsndrcv ;
		} type ;
	} mb_resp ;
	/* mb_intreq : Mailbox Interrupt -----------------------------------*/
	struct mb_interrupt {
		/* Interrupt Request Header */
		uchar			mb_code[4] ;			/* +0					*/
#define HFC_MBINTREQ_DCE		0xA010			/* Detect Correctable Error	*/
#define HFC_MBINTREQ_COU		0xA020			/* Complete Online Update	*/
#define HFC_MBINTREQ_LINKUP		0xB080			/* Link Up				*/
#define HFC_MBINTREQ_LINKDOWN	0xB088			/* Link Down			*/
#define HFC_MBINTREQ_RCVFRM		0xB010			/* Receive Frame		*/
#define HFC_MBINTREQ_RCVFLOGI	0xB006			/* Receive FLOGI		*/
#define HFC_MBINTREQ_RCVPLOGI	0xB004			/* Receive PLOGI		*/
#define HFC_MBINTREQ_RCVPDISC	0xB005			/* Receive PDISC		*/
		uchar			esw ;					/* +4 ESW				*/
		uchar			softlog ;				/* +5 Softlog#			*/
		ushort			sbc ;					/* +6 SBC				*/
		/* type : Mailbox Interrupt Type */
		union {
			/* detect_correctable_error : Detect Correctable Error */
			struct {
				uchar	fsynd ;					/* +8 FSYND				*/
				uchar	far[3] ;				/* +9 FAR				*/
				uchar	resv1[2548] ;			/* +C - +BFF			*/
			} detect_correctable_error ;
			/* complete_online_update : Complete Online Update */
			struct {
				uint	sysrev ;				/* +8 New SYSREV		*/
				uchar	resv1[2548] ;			/* +C - +9FF			*/
			} complete_online_update ;
			/* linkup : Link Up */
			struct {
				uchar	resv1[8] ;				/* +8 - +F				*/
				uchar	con_type ;				/* +10 Connection_type	*/
				uchar	trans_rate ;			/* +11 Transfer_rate	*/
				uchar	link_config_flag ;		/* +12 Link_Configure_Flag	*/
				uchar	assign_alpa ;			/* Assign ALPA			*/
				uchar	alpa_count ;			/* ALPA_Count			*/
				uchar	link_con_type ;			/* Link_Con_Type		*/
				/*		HFC_VF_ID_VALID
						HFC_ALPA_VALID
						HFC_POSMAP_VALID
						HFC_LOOP_PROCEDURE								*/
				ushort	vf_id ;					/* +16 VF_ID (exclude first 4bit)		*/
				uchar	resv2[2] ;				/* +18 - +19			*/
				ushort	fcoe_vid ;				/* +1A FCoE VID (exclude first 4bit)	*/
				uchar	fpsp[2] ;				/* +1C FP, SP (first 2bit) : HFC_FPMA, HFC_SPMA	*/
				uchar	resv3[4] ;				/* +1D - +21 			*/
				uchar	mac_address[6] ;		/* +22 MAC address		*/
				uchar	resv4[5] ;				/* +28 - +2C			*/
				uchar	fc_map[3] ;				/* +2D FC-MAP			*/
				uint64_t	fabric_name ;		/* +30 Fabric_Name		*/
				uchar	resv5[3] ;				/* +38 - +3A			*/
				uchar	d ;						/* +3B D (last 1bit) : HFC_DBIT	*/
				uint	fka_adv_period ;		/* +3C FKA_ADV_PERIOD	*/
				uchar	acquired_alpa[32] ;		/* +40 Acquired ALPA	*/
				uchar	resv6[32] ;				/* +60 - +7f			*/
				uchar	position_map[128] ;		/* +80 - +FF			*/
				uchar	resv7[2304] ;			/* +100 - +9FF			*/
			} linkup ;
			/* linkdown : Link Down */
			struct {
				ushort	down_detail ;			/* +8 Down_Detail		*/
#define HFC_LINKDOWN_NOSYNC	0x0103				/* NO SIGNAL, NO SYNC	*/
#define HFC_LINKDOWN_NOSPS	0x0104				/* Received NOS Primitive Sequence	*/
#define HFC_LINKDOWN_LRTO	0x0105				/* Link Reset Procedure Time Out */
#define HFC_LINKDOWN_IPS	0x0106				/* Received Invalid Primitve Sequence */
#define HFC_LINKDOWN_LIPF8	0x0107				/* Received LIP (F8)	*/
#define HFC_LINKDOWN_OLSPS	0x1111				/* Received OLS Primitive Sequence	*/
#define HFC_LINKDOWN_LIPF7	0x1112				/* Received LIP (F7)	*/
#define HFC_LINKDOWN_LPB	0x1113				/* Received LPB (Bypassed)	*/
#define HFC_LINKDOWN_RSV	0x2001				/* Reserved				*/
#define HFC_LINKDOWN_DATO	0x3001				/* Discovery Advertisement Receive Time Out	*/
#define HFC_LINKDOWN_EFCVL	0x3002				/* Received ENode FIP Clear Virtual Links	*/
#define HFC_LINKDOWN_VFCVL	0x3003				/* Received VN_Port FIP Clear Virtual Links	*/
#define HFC_LINKDOWN_SFP_NOT_SUPPORTED	0xE001	/* Not supported SFP module is installed	*/
#define HFC_LINKDOWN_SFP_PK_REPLACE		0xE102	/* SFP Module Error (PK Replacement)	*/
#define HFC_LINKDOWN_SFP_LIGHTMODULE_REPLACE	0xE103	/* SFP Module Error (Light Module Replacement) */
#define HFC_LINKDOWN_SFP_NOT_INSTALLED	0xE104	/* SFP Module is not Installed	*/
				uchar	resv1[6] ;				/* +A - +F				*/
				uchar	resv2[2544] ;			/* +10 - +9FF			*/
			} linkdown ;
			/* rcvfrm : Receive Frame */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +A Receive Frame Payload Length	*/
#define HFC_INTREQ_PRLI_LENGTH			0x14
#define HFC_INTREQ_PRLO_LENGTH			0x14
#define HFC_INTREQ_LOGO_LENGTH			0x10
				uchar	resv1[4] ;				/* +C - +F				*/
				vft_hdr_t	vft_hdr ;			/* +10 VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +18 Ethernet Header	*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uchar	resv2[464] ;			/* +30 - +1FF			*/
#define HFC_INTREQ_ELS_PRLI			0x20
#define HFC_INTREQ_ELS_PRLO			0x21
#define HFC_INTREQ_ELS_RSCN			0x61
#define HFC_INTREQ_ELS_LOGO			0x05
#define HFC_INTREQ_ELS_EVFP			0x7f
#define HFC_INTREQ_ELS_AUTH_ELS		0x90
				/* type : Response Payload */
				union {
					uchar	els_command;		/* +200 ELS_Command code in Payload */
					struct {
						uchar	data1[16];		/* +200 - +20f			*//* FCLNX-GPL-FX-252,272 */
						uint	prli_param_req;	/* +210 - +203			*/
						uchar	resv1[2028];	/* +214 - +9ff			*/
					} prli;
					struct {
						uchar	data1[20];		/* +200 - +213			*//* FCLNX-GPL-FX-252,272 */
						uchar	resv1[2028];	/* +214 - +9ff			*/
					} prlo;
					struct {
						uchar	data1[2] ;			/* +200 - +201 		*//* FCLNX-GPL-FX-252,272 */
						ushort	scn_cnt ;			/* +202 - +203			*/
						uint	portid_page[511];	/* +204 - +9ff 		*/
					} rscn;
					struct {
						uchar	data1[5];		/* +200 - +204			*//* FCLNX-GPL-FX-252,272 */
						uchar	n_port_id[3];	/* +205 - +207			*/
						uint64_t  n_port_name;	/* +208 - +20f			*/
						uchar	resv2[2032];	/* +210 - +9ff			*/
					} logo;
				} subtype;
//				uchar	resv3[2047] ;			/* +A00 Receive Frame Payload	*/
			} rcvfrm ;
			/* rcvflogi : Receive FLOGI */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +A Receive Frame Payload Length	*/
				uchar	resv1[4] ;				/* +C - +F				*/
				vft_hdr_t	vft_hdr ;			/* +10 VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +18 Ethernet Header	*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uchar	resv2 ;					/* +30 MPLF_Flag		*/
				uchar	flogi_conf_flag;		/* +31 FLOGI_Configure_Flag */
				ushort	flogi_max_frame_size ;	/* +32 FLOGI_Max_Frame_Size	*/
				uchar	resv3[3] ;				/* +34 - +36			*/
				uchar	flogi_rsp_param ;		/* +37 FLOGI_RSP_Param	*/
				uint64_t	target_wwpn ;		/* +38 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +40 Target WWNN		*/
				uchar	resv4 ;					/* +48 					*/
				uchar	assign_portid[3] ;		/* +49 Assign Port_ID	*/
				uchar	resv5 ;					/* +4C					*/
				uchar	p2p_tgt_port_id[3] ;	/* +4D PtoP_Target_PORT_ID	*/
				uchar	resv6[2480] ;			/* +50 - +9ff			*/
			} rcvflogi ;
			/* rcvplogi : Receive PLOGI */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +A Receive Frame Payload Length	*/
				uchar	resv1[4] ;				/* +C - +F				*/
				vft_hdr_t	vft_hdr ;			/* +10 VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +18 Ethernet Header	*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uchar	class ;					/* +30 Class			*/
				uchar	resv2 ;					/* +31					*/
				ushort	plogi_max_frame_size ;	/* +32 PLOGI_Max_Frame_Size	*/
				uchar	resv3[3] ;				/* +34 - +36			*/
				uchar	plogi_param ;			/* +37 PLOGI_Param		*/
				uint64_t	target_wwpn ;		/* +38 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +40 Target WWNN		*/
				uchar	resv4[2488] ;			/* +48 - +9ff			*/
			} rcvplogi ;
			/* rcvpdisc : Receive PDISC */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +A Receive Frame Payload Length	*/
				uchar	resv1[4] ;				/* +C - +F				*/
				vft_hdr_t	vft_hdr ;			/* +10 VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +18 Ethernet Header	*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uchar	class ;					/* +30 Class			*/
				uchar	resv2 ;					/* +31					*/
				ushort	plogi_max_frame_size ;	/* +32 PLOGI_Max_Frame_Size	*/
				uchar	resv3[3] ;				/* +34 - +36			*/
				uchar	plogi_param ;			/* +37 PLOGI_Param		*/
				uint64_t	target_wwpn ;		/* +38 Target WWPN		*/
				uint64_t	target_wwnn ;		/* +40 Target WWNN		*/
				uchar	resv4[2488] ;			/* +48 - +9ff			*/
			} rcvpdisc ;
		} type ;
	} mb_intreq ;
	/* mb_intresp : Mailbox Interrupt Response -------------------------*/
	struct mb_intresponse {
		/* Interrupt Response Header */
		uint			mb_code ;				/* +0 Mailbox code		*/
#define HFC_MBINTRSP_DCE	0x2000A010			/* Detect Correctable Error	*/
#define HFC_MBINTRSP_COU	0x2000A020			/* Complete Online Update	*/
#define HFC_MBINTRSP_LINKUP	0x2000B080			/* Link Up				*/
#define HFC_MBINTRSP_LINKDOWN	0x2000B088		/* Link Down			*/
#define HFC_MBINTRSP_RCVFRM	0xB0100000			/* Receive Frame		*/
#define HFC_MBINTRSP_RCVFLOGI	0xB0060000		/* Receive FLOGI		*/
#define HFC_MBINTRSP_RCVPLOGI	0xB0040000		/* Receive PLOGI		*/
#define HFC_MBINTRSP_RCVPDISC	0xB0050000		/* Interrupt Response	*/

/*#define HFC_INTRSPMB		0xB0000000			/\* Receive Frame/FLOGI/PLOGI/PDISC */
		uchar			resv1[4] ;				/* +4 ESW			*/
		/* type : Mailbox Interrupt Response Type */
		uchar			frame_ctl ;				/* +8 Frame_CTL			*/
		uchar			fc_class ;				/* +9 FC_Class			*/
		ushort			payload_length ;		/* +a Payload Length	*/
		vft_hdr_t		vft_hdr ;				/* +C VFT_Header		*/
		eth_hdr16b_t	eth_hdr16b ;			/* +14 Ethernet Header	*/
		uchar			resv2[3] ;				/* +24 - +26			*/
		uchar			param;					/* +27					*/
		fcph_hdr_t		fcph_hdr ;				/* +28 FC-PH Header		*/
		uint64_t		self_wwpn ;				/* +30 Target WWPN		*/
		uint64_t		self_wwnn ;				/* +38 Target WWNN		*/
		uchar			payload[8] ;			/* +40 Payload Contents	*/
#define HFC_REASON_CODE_LOGICAL_ERROR 0x03		/* Reason Code : Logical Error */
		uchar			resv3[440] ;			/* +48 - +1FF			*/

#if 0
		union {
			/* rcvfrm : Receive Frame */
			struct {
				uchar	frame_ctl ;				/* +8 Frame_CTL			*/
				uchar	fc_class ;				/* +9 FC_Class			*/
				ushort	payload_length ;		/* +a Payload Length	*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv1[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	payload ;			/* +30 Payload Address	*/
				uchar	resv2[456] ;			/* +38 - +1FF			*/
			} rcvfrm ;
			/* rcvflogi : Receive FLOGI */
			struct {
				uchar	frame_ctl ;				/* +4 Frame_CTL			*/
				uchar	fc_class ;				/* +5 FC_Class			*/
				ushort	payload_length ;		/* +6 Payload Length	*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv1[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	payload ;			/* +30 Payload Address	*/
				uchar	resv2[456] ;			/* +38 - +1FF			*/
			} rcvflogi ;
			/* rcvplogi	: Receive PLOGI	*/
			struct {
				uchar	frame_ctl ;				/* +4 Frame_CTL			*/
				uchar	fc_class ;				/* +5 FC_Class			*/
				ushort	payload_length ;		/* +6 Payload Length	*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv1[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	payload ;			/* +30 Payload Address	*/
				uchar	resv2[456] ;			/* +38 - +1FF			*/
			} rcvplogi ;
			/* rcvpdisc : Receive PDISC */
			struct {
				uchar	frame_ctl ;				/* +4 Frame_CTL			*/
				uchar	fc_class ;				/* +5 FC_Class			*/
				ushort	payload_length ;		/* +6 Payload Length	*/
				vft_hdr_t	vft_hdr ;			/* +C VFT_Header		*/
				eth_hdr16b_t	eth_hdr16b ;	/* +14 Ethernet Header	*/
				uchar	resv1[4] ;				/* +24 - +27			*/
				fcph_hdr_t	fcph_hdr ;			/* +28 FC-PH Header		*/
				uint64_t	payload ;			/* +30 Payload Address	*/
				uchar	resv2[456] ;			/* +38 - +1FF			*/
			} rcvpdisc ;
		} type ;
#endif
	} mb_intresp;
};

/*---- Mailbox Frame Individual Field Macros ---------------------------------*/
#define HFC_MBXREQ_TB(P, FLD)		((P)->mb_init.type.##FLD)
#define HFC_MBXRSP_TB(P, FLD)		((P)->mb_resp.type.##FLD)
#define HFC_MBXINT_TB(P, FLD)		((P)->mb_intreq.type.##FLD)
#define HFC_INTRSP_TB(P, FLD)		((P)->mb_intresp.##FLD)
/*---- Mailbox Frame Individual Field Read Access(with Reverse Endian)Macros -*/
#define HFC_MBXREQ_FR(P, FLD)		(hfc_read_val(HFC_MBXREQ_TB(P, FLD)))
#define HFC_MBXRSP_FR(P, FLD)		(hfc_read_val(HFC_MBXRSP_TB(P, FLD)))
#define HFC_MBXINT_FR(P, FLD)		(hfc_read_val(HFC_MBXINT_TB(P, FLD)))
#define HFC_INTRSP_FR(P, FLD)		(hfc_read_val(HFC_INTRSP_TB(P, FLD)))
/*---- Mailbox Frame Individual Field Write Access(with Reverse Endian)Macros */
#define HFC_MBXREQ_FW(P, FLD, D)	(hfc_write_val(HFC_MBXREQ_TB(P, FLD), D))
#define HFC_MBXRSP_FW(P, FLD, D)	(hfc_write_val(HFC_MBXRSP_TB(P, FLD), D))
#define HFC_MBXINT_FW(P, FLD, D)	(hfc_write_val(HFC_MBXINT_TB(P, FLD), D))
#define HFC_INTRSP_FW(P, FLD, D)	(hfc_write_val(HFC_INTRSP_TB(P, FLD), D))

struct fx_cca {
	ushort		post_result;					/* +0					*/
	ushort		free_xrb;						/* +2					*/
	uint		mpck_code;						/* +4					*/
	uchar		resv1[8];						/* +8					*/
	uint64_t	init_addr;						/* +10					*/
	uint64_t	mem_info_addr;					/* +18					*/
	uint		sel_info;						/* +20					*/
	uchar		opmode;							/* +24					*/
	uchar		mlpf_mode;						/* +25					*/
	uchar		force_mck_code;					/* +26					*/
	uchar		online_update_state;			/* +27					*/
	uint		fw_support_info	;				/* +28					*/
	uchar		resv2[84];						/* +2c					*/
};


/************************************************************************/
/* VPD Area																*/
/************************************************************************/
#if 0
#define VPD_PN_LEN 11				  /* Parts Number length FCLNX-0337 *//* FCLNX-0368 */

struct hfc_vpd {
	uchar typeid;
	uchar typeid_len[2];
	uchar typeid_value[48];

	uchar typevpd_rid;
	uchar typevpd_len[2];

	uchar mn_code[2]; 
	uchar mn_len;
	uchar mn_value[7];

	uchar pn_code[2];
	uchar pn_len;
	uchar pn_value[VPD_PN_LEN];		/* FCLNX-0337 *//* FCLNX-0368 */

	uchar ec_code[2];
	uchar ec_len;
	uchar ec_value[1];				/* ver1.5 */

	uchar rv_code[2];
	uchar rv_len;
	uchar rv_chksum;
	uchar rv_reservd[41];			/* ver1.5 */
	uchar typeend_id;

	uchar driver_len;
	uchar driver_ver[16];			/* ZO */
	uint  fw_ver;					/* Z1 */
	uint64_t ww_name;				/* Z2 (ver1.6) */
};
/* FIVE */
struct hfc_vpd_five {
	uchar typeid;
	uchar typeid_len[2];
	uchar typeid_value[51];
	uchar typeid_rsv[10];

	uchar typevpd_rid;
	uchar typevpd_len[2];

	uchar mn_code[2]; 
	uchar mn_len;
	uchar mn_value[7];

	uchar pn_code[2];
	uchar pn_len;
	uchar pn_value[VPD_PN_LEN];			/* FCLNX-0274 *//* FCLNX-0337 *//* FCLNX-0368 */
	uchar pn_rsv[5];				/* FCLNX-0274 */

	uchar rv_code[2];
	uchar rv_len;
	uchar rv_chksum;
	uchar rv_reservd[27];			/* ver1.5 */
	uchar typeend_id;

	uchar driver_len;
	uchar driver_ver[16];			/* ZO */
	uint  fw_ver;					/* Z1 */
	uint64_t ww_name;				/* Z2 (ver1.6) */
	uchar ec_level;
};

/* vpd format for FIVE-EX */
struct hfc_vpd_five_ex {
	uchar typeid;
	uchar typeid_len[2];
	uchar typeid_value[60];
	uchar typeid_rsv[1];

	uchar typevpd_rid;
	uchar typevpd_len[2];

	uchar mn_code[2]; 
	uchar mn_len;
	uchar mn_value[7];

	uchar pn_code[2];
	uchar pn_len;
	uchar pn_value[11]; /* Parts Number */
	uchar pn_rsv[5];

	uchar v0_code[2];
	uchar v0_len;
	uchar v0_value[13];  /* Model Name */
	
	uchar rv_code[2];
	uchar rv_len;
	uchar rv_chksum;
	uchar rv_reservd[12];
	
	uchar typeend_id;

	uchar driver_len;
	uchar driver_ver[16];			/* ZO */
	uint  fw_ver;					/* Z1 */
	uint64_t ww_name;				/* Z2 (ver1.6) */
	uchar ec_level;
};
/************************************************************************/
/* FPP/FIVE package map													*/
/************************************************************************/
struct pkg_map {
	struct {
		int reg[128];				/* FCWIN-0200 */
	}iosp;
};
#endif

/************************************************************************/	/* @MLPF STR */
/* MMIO-HG mem_space map												*/
/************************************************************************/
struct hg_map_fx {
	uint			mmio_hg_len ;				/* +0 MMIO-HG length	*/
	uint			mmio_hg_ver ;				/* +4 MMIO-HG version	*/
	uint			hyp_stat ;					/* +8 Hyper status		*/
	uint			command_reg ;				/* +C Command Register	*/
	uint64_t		wwpn ;						/* +10 WWPN				*/
	uint64_t		wwnn ;						/* +18 WWNN				*/
	uint			rid ;						/* +20 RID				*/
	uint			mlpf_drv_ver ;				/* +24 MLPF Driver version	*/
	uint			lpar_stat ;					/* +28 LPAR status		*/
	uint			hyp_int_detail ;			/* +2C HyperIntDetail	*/
	uchar			mmio_hg_ind_acc_ctl[16] ;	/* +30 MMIO-HG indirect access control */
	uchar			parts_num[16] ;				/* +40 Parts Number		*/
	uchar			sysrev[16] ;				/* +50 SYSREV			*/
	uchar			adap_id[16] ;				/* +60 adap_id			*/
	uint			hvm_support ;				/* +70 HVM Support		*/
	uint			drv_support	;				/* +74 Drv Support		*/
	uchar			resv1[8] ;					/* +78 - +7F			*/
	uchar			vpd_area[128] ;				/* +80 VPD area			*/
	uchar			mmio_hg_ind_acc_fld[128] ;	/* +100 MMIO-HG indirect access field */
	uint			err_lim_ld ;				/* +180 ErrorLimit(Link Down)	*/
	uint			err_lim_fc ;				/* +184 ErrorLimit(FC Interface)	*/
	uint			err_lim_scsito ;			/* +188 ErrorLimit(SCSI timeout)	*/
	uint			err_lim_scsires ;			/* +18C	ErrorLimit(SCSI Reset)	*/
	uint64_t		stat_area ;					/* +190 Physical Addresses of Statistics Information Area */
	uchar			flag ;						/* +198 Flag			*/
	uchar			resv2[55] ;					/* +199 - +1DF			*/
	uchar			resv3[16] ;					/* +1E0 - +1EF			*/
	uchar			efi_event_info[16] ;		/* +1F0 EFI Event Info	*/
	uchar			efi_opt_tbl[512] ;			/* +200 EFI Option Table */
#if 0
	uint			cmd_data0 ;					/* +300 Cmd_Data0(Debug)	*/
	uint			cmd_data1 ;					/* +304 Cmd_Data1(Debug)	*/
	uint			cmd_data2 ;					/* +308 Cmd_Data2(Debug)	*/
	uint			cmd_data3 ;					/* +308 Cmd_Data3(Debug)	*/
	uchar			resv4[144] ;				/* +310 - +39F				*/
	/* 0 */
	uint			hyp_int_detail0	;			/* +3A0 HyperIntDetail_0	*/
	uchar			resv5[4] ;					/* +3A4 - +3A7				*/
	uint			hyp_stat_0 ;				/* +3A8 Hyper stats_0		*/
	uint			cmd_reg_0 ;					/* +3AC	Command Register_0 	*/
	/* 1 */
	uint			hyp_int_detail1	;			/* +3B0 HyperIntDetail_0	*/
	uchar			resv6[4] ;					/* +3B4 - +3A7				*/
	uint			hyp_stat_1 ;				/* +3B8 Hyper stats_1		*/
	uint			cmd_reg_1 ;					/* +3BC	Command Register_1	*/
	/* 2 */
	uint			hyp_int_detail2	;			/* +3C0 HyperIntDetail_2	*/
	uchar			resv7[4] ;					/* +3C4 - +3A7				*/
	uint			hyp_stat_2 ;				/* +3C8 Hyper stats_2		*/
	uint			cmd_reg_2 ;					/* +3CC	Command Register_2	*/
	/* 3 */
	uint			hyp_int_detail3	;			/* +3D0 HyperIntDetail_3	*/
	uchar			resv8[4] ;					/* +3D4 - +3A7				*/
	uint			hyp_stat_3 ;				/* +3D8 Hyper stats_3		*/
	uint			cmd_reg_3 ;					/* +3DC	Command Register_3	*/
	uchar			resv9[32] ;					/* +3E0	- +3FF				*/
#endif

	/* Core#0 Register */
	uchar			resv10[8]	;				/* +400 - +407				*/
	uint			hyper_status0 ;				/* +408 Hyper_status_0		*/
	uint			command_register0 ;			/* +40C Command_Register_0	*/
	uchar			resv11[16] ;				/* +410 - +41F				*/
	uchar			resv12[12] ;				/* +420 - +42B				*/
	uint			hyper_int_detail0 ;			/* +42C	HyperIntDetail_0	*/
	uchar			resv13[16] ;				/* +430 - +43F				*/
	/* Core#1 Register */
	uchar			resv14[8]	;				/* +440 - +447				*/
	uint			hyper_status1 ;				/* +448 Hyper_status_1		*/
	uint			command_register1 ;			/* +44C Command_Register_1	*/
	uchar			resv15[16] ;				/* +450 - +45F				*/
	uchar			resv16[12] ;				/* +460 - +46B				*/
	uint			hyper_int_detail1 ;			/* +46C	HyperIntDetail_1	*/
	uchar			resv17[16] ;				/* +470 - +47F				*/
	/* Core#2 Register */
	uchar			resv18[8]	;				/* +480 - +487				*/
	uint			hyper_status2 ;				/* +488 Hyper_status_2		*/
	uint			command_register2 ;			/* +48C Command_Register_2	*/
	uchar			resv19[16] ;				/* +490 - +49F				*/
	uchar			resv20[12] ;				/* +4A0 - +4AB				*/
	uint			hyper_int_detail2 ;			/* +4AC	HyperIntDetail_2	*/
	uchar			resv21[16] ;				/* +4B0 - +4BF				*/
	/* Core#3 Register */
	uchar			resv22[8]	;				/* +4C0 - +4C7				*/
	uint			hyper_status3 ;				/* +4C8 Hyper_status_3		*/
	uint			command_register3 ;			/* +4CC Command_Register_3	*/
	uchar			resv23[16] ;				/* +4D0 - +4DF				*/
	uchar			resv24[12] ;				/* +4E0 - +4EB				*/
	uint			hyper_int_detail3 ;			/* +4EC	HyperIntDetail_3	*/
	uchar			resv25[16] ;				/* +4F0 - +4FF				*/
};

struct hg_cca_fx {     /* FCLNX-GPL-494 Get Statistics for Virtage *//* FCLNX-GPL-FX-433 */
	uchar		version;
	uchar		valid;
#define HFC_FWSTATISTICS_VALID		0x80
	ushort		size;
	ushort		uni_cnt;
	uchar		rid;
	uchar		cnum;
	uint64_t	statistics_cnt;
	uint64_t	io_exec;
	uint64_t	io_end;
	uint64_t	io_err;
	uint64_t	xob_full;
	uint64_t	iov_full;
	uint64_t	frame_full;
	uint64_t	page_over;
	uint64_t	tx_frame;
	uint64_t	tx_word;
	uint64_t	rx_frame;
	uint64_t	rx_word;
	uint64_t	rsv1;
	uint64_t	rsv2;
	uint64_t	rsv3;
};
																			/* @MLPF END */

/************************************************************************/
/* FLASH ROM															*/
/************************************************************************/
struct flash_param {
	uchar		Reserved_00;					/* +00		*/
	uchar		flag;							/* +01		*/
	uchar		bvc_entry_num;					/* +02		*/
	uchar		connection_type;				/* +03		*/
	uchar		data_rate;						/* +04		*/
	uchar		param_for_os_driver;			/* +05		*/
	uchar		login_delay_time;				/* +06		*/
	uchar		mode;							/* +07		*/
	uchar		Reserved_08[8];					/* +08		*/
	uchar		bvc_table[128];					/* +10		*/
	uchar		signature[3];					/* +90		*/
	uchar		efi_table_rev;					/* +93		*/
	uchar		total_number_of_LU[2];			/* +94		*/
	uchar		max_count_LU[2];				/* +96		*/
	uchar		Login_retry_count;				/* +98		*/
	uchar		Linkup_timer;					/* +99		*/
	uchar		mailbox_rsp_timer;				/* +9a		*/
	uchar		mailbox_retry_count;			/* +9b		*/
	uchar		scsi_rsp_timer;					/* +9c		*/
	uchar		scsi_retry_count;				/* +9d		*/
	uchar		scsi_retry_timer;				/* +9e		*/
	uchar		link_initialize_rsp_timer;		/* +9f		*/
	uchar		efi_support_flag;				/* +a0		*/
	uchar		spinup_delay_time;				/* +a1		*/
	uchar		Login_retry_timer;				/* +a2		*/
	uchar		Log_control;					/* +a3		*/
	uchar		padding[4];						/* +a4		*/
	uchar		DriverCtrlFX;					/* +a8		*/
	uchar		VF_Flag;						/* +a9		*/
	uchar		FCSP_Flag;						/* +aa		*/
	uchar		reserved_AB;					/* +ab		*/
	uchar		Conn_Type_Option;				/* +ac		*//* FCLNX-GPL-FX-135 */
	uchar		Ver_Manager[19];				/* +ad		*//* FCLNX-GPL-FX-135 */
	uchar		link_initialize_timer;			/* +c0		*/
	uchar		MCKLinkup_Timer;				/* +c1		*/
	uchar		Linkup_Time;					/* +c2		*/
	uchar		PlogiWait_Timer;				/* +c3		*/
	uchar		mailbox_delay_time[28];			/* +c4		*/
	uchar		mailbox_rsp_timer_grp1;			/* +e0		*/
	uchar		mailbox_rsp_timer_grp2;			/* +e1		*/
	uchar		mailbox_rsp_timer_grp3;			/* +e2		*/
	uchar		mailbox_rsp_timer_grp4;			/* +e3		*/
	uchar		mailbox_rsp_timer_grp5;			/* +e4		*/
	uchar		mailbox_rsp_timer_grp6;			/* +e5		*/
	uchar		mailbox_rsp_timer_grp7;			/* +e6		*/
	uchar		mailbox_rsp_timer_grp8;			/* +e7		*/
	uchar		mailbox_retry_count_grp1;		/* +e8		*/
	uchar		mailbox_retry_count_grp2;		/* +e9		*/
	uchar		mailbox_retry_count_grp3;		/* +ea		*/
	uchar		mailbox_retry_count_grp4;		/* +eb		*/
	uchar		mailbox_retry_count_grp5;		/* +ec		*/
	uchar		mailbox_retry_count_grp6;		/* +ed		*/
	uchar		mailbox_retry_count_grp7;		/* +ee		*/
	uchar		mailbox_retry_count_grp8;		/* +ef		*/
	uchar		mailbox_retry_delay_grp1;		/* +f0		*/
	uchar		mailbox_retry_delay_grp2;		/* +f1		*/
	uchar		mailbox_retry_delay_grp3;		/* +f2		*/
	uchar		mailbox_retry_delay_grp4;		/* +f3		*/
	uchar		mailbox_retry_delay_grp5;		/* +f4		*/
	uchar		mailbox_retry_delay_grp6;		/* +f5		*/
	uchar		mailbox_retry_delay_grp7;		/* +f6		*/
	uchar		mailbox_retry_delay_grp8;		/* +f7		*/
	uchar		Reserved_F8[8];					/* +f8		*/
	uchar		PearPassword[40];				/* +100		*/
	uchar		pad_128[8];						/* +128		*/
	uchar		LocalPassword[40];				/* +130		*/
	uchar		pad_158[8];						/* +158		*/
	uchar		Reserved_160[160];				/* +160		*/
};

#endif				/* !INCLUDE _H_HFCDDCOM */
