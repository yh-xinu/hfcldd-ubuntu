/*
 * hfcl_stra_trace.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_stra_trace.h,v 1.7.8.7.2.1.6.1.14.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:48 toyo Exp $
 */


/*------ trace sub_id ---------------*/

#define HFC_TRC_BUILDIO         0x71
#define HFC_TRC_STARTIO         0x72
#define HFC_TRC_SRB_SET         0x73
#define HFC_TRC_ABORT_CMD       0x75
#define HFC_TRC_ABORT           0x76
#define HFC_TRC_RESET           0x77
#define HFC_TRC_TGT_RESET       0x74	/* FCLNX-GPL-0343 */
#define HFC_TRC_BUS_RESET       0x78
#define HFC_TRC_SHUTDOWN        0x79
#define HFC_TRC_FLUSH           0x7a
#define HFC_TRC_ISSUE_TMGM      0x7b
#define HFC_TRC_RESETBUS        0x7c
#define HFC_TRC_LUN_RESET       0x7d	/* FCLNX-GPL-0343 */
#define HFC_TRC_IODONE          0x7f

#define	HFC_TRC_STRATEGY        0x81
#define	HFC_TRC_START           0x82
#define	HFC_TRC_CAN_XOB         0x83
#define	HFC_TRC_CAN_WE          0x84
#define HFC_TRC_GET_IOV         0x85
#define	HFC_TRC_RES_CHK         0x87
#define	HFC_TRC_IOVUP           0x88
#define	HFC_TRC_DMA_MAP         0x89
#define	HFC_TRC_MK_CMD          0x8a
#define	HFC_TRC_ENQ_XOB         0x8b
#define HFC_TRC_FRAME_CHK       0x8c
#define	HFC_TRC_CAN_WX			0x8d
#define	HFC_TRC_CLR_WR			0x8e

#define HFC_TRC_RST_NF			0x98

#define HFC_TRC_ENQ_WX			0xcC
#define HFC_TRC_RST_CBACK		0xcD
#define HFC_TRC_RST_FREE		0xcE

/*--------------------------------------*/
struct trc_com {
	uchar			id ;				/* +00 */
	uchar			sub_id ;			/* +01 */
	uchar			cmnd ;				/* +02 */
	uchar			timeout ;			/* +03 */
	ushort			lun_id ;			/* +04 */	/* FCLNX-0659 *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar			target_id ;			/* +06 */	/* FCLNX-0659 */
//	uchar			lun_id ;						/* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uchar			use_sg ;			/* +07 */
	uint			adap_status ;		/* +08 */
	uint			target_status ;		/* +0c */
	uchar			target_flags ;		/* +10 */
	uchar			target_pseq ;		/* +11 */
	ushort			target_dev_flags ;	/* +12 */
	uchar			lunstat ;			/* +14 */
	uchar			open_status ;		/* +15 */
	ushort			rsv1 ;				/* +16 */
};

/*---- trace format --------------------*/
struct stra_trc1 {
	struct trc_com		com ;

	uint			a_scsi_id ;			/* +18 */	/* adaptor scsi_id */
	uint			d_scsi_id ;			/* +1c */	/* target  scsi_id */
	ushort			wx_que_cnt ;		/* +20 */	/* Scsi start waiting count */
	ushort			we_que_cnt ;		/* +22 */	/* Scsi response waiting count */
	uchar			cmd_status ;		/* +24 */	/* Hfc_pkt status information */
	uchar			rsv1[2] ;			/* +25 */									/* FCLNX-0659 */
	uchar			cmd_xob ;			/* +27 */	/* Hfc_pkt allocation xob# */	/* FCLNX-0659 */
	uint			cmd_timeout ;		/* +28 */	/* Hfc_pkt timeout time */
	uint			cmd_flags ;			/* +2c */	/* Hfc_pkt control flag  */
	uint			cmd_iov_no ;		/* +30 */	/* hfc_pkt - seg_info# */
	uint			cmd_iov_cnt ;		/* +34 */	/* hfc_pkt - Number of seg_info */
	uint			adap_status ;		/* +38 */	/* hfc_pkt adap_status */
	uint			pkt_flag ;			/* +3c */	/* scsi_pkt flag byte */
	uint			pkt_time ;			/* +40 */	/* scsi_pkt Time setting value */
	uint			pkt_resid ;			/* +44 */	/* scsi_pkt The remainder byte */
	uint			pkt_state ;			/* +48 */	/* scsi_pkt state */
	uint			serial_number ;		/* +4c */	/* scsi_pkt statistics */
	uint			pkt_result ;		/* +50 */	/* scsi_pkt result */
	uint64_t		scsi_exec_cnt ;		/* +54 */	/* Number of scsi start demands */
	uint64_t		scsi_end_cnt ;		/* +5c */	/* Number of scsi responses */
	uint64_t		scsi_err_cnt ;		/* +64 */	/* Number of abnormal terminations of scsi */
};

struct stra_trc2 {
	struct trc_com		com ;

	uint			d_scsi_id ;			/* +18 */	/* target  scsi_id */
	ushort			wx_que_cnt ;		/* +1c */	/* Scsi start waiting count */
	ushort			we_que_cnt ;		/* +1e */	/* Scsi response waiting count */
	ushort			xob_no ;			/* +20 */
	uchar			xob_exec_cnt ;		/* +22 */
	uchar			xob_wait_exec_cnt ;	/* +23 */
	uint			iov_no ;			/* +24 */
	uint			iov_map_cnt ;		/* +28 */
	uint			save_xob_outp ;		/* +2c */
	uint			xob_outp_end;		/* +30 */
	uint			xob_outp;			/* +34 */
	uint			xob_inp;			/* +38 */
	uint64_t		new_login_need ;	/* +3c */
	uchar			resv1[6];			/* +44 */	/* FCLNX-0659 */
	ushort			next_dstart_cnt ;	/* +4a */	/* FCLNX-0659 */
	uint			frame_chkp ;		/* +4c */
	uint			frame_inp ;			/* +50 */
	uchar			cmd_status ;		/* +54 */	/* Hfc_pkt status information */
	uchar			cmnd ;				/* +55 */
	uchar			rsv2;				/* +56 */	/* FCLNX-0659 */
	uchar			cmd_xob ;			/* +57 */	/* Hfc_pkt allocation xob# */ /* FCLNX-0659 */
	uint			cmd_timeout ;		/* +58 */	/* Hfc_pkt timeout time    */
	uint			cmd_flags ;			/* +5c */	/* Hfc_pkt control flag    */
	uint			cmd_iov_no ;		/* +60 */	/* hfc_pkt - seg_info#     */
	uint			cmd_iov_cnt ;		/* +64 */	/* hfc_pkt - Number of seg_info */
	uint			cmd_data_size;		/* +68 */	/* Forwarding data size    */
	uint			adap_status ;		/* +6c */	/* hfc_pkt adap_status     */
};

struct stra_trc3 {
	struct trc_com	com ;

	uint			a_scsi_id ;			/* +18 */	/* adaptor scsi_id */
	uint			d_scsi_id ;			/* +1c */	/* target  scsi_id */
	uint			xob_outp ;			/* +20 */
	uint			xob_inp ;			/* +24 */
	uchar			type ;				/* +28 */	/* cancel type 	*/
	uchar			rsv1 ;				/* +29 */	
	ushort			lun;				/* +2a */	/* lun#		 	*//* FCLNX-GPL-0343 *//* FCLNX-GPL-0382 */
	uint			adap_status;		/* +2c */	/* adap_status(argument) */
	uint			we_que_cnt;			/* +30 */
	uint			wx_que_cnt;			/* +34 */
};

struct stra_trc4 {
	struct trc_com	com ;

	uint			a_scsi_id ;			/* +18 */	/* adaptor scsi_id */
	uint			xob_outp ;			/* +1c */
	uint			xob_inp ;			/* +20 */
	uchar			xob_exec_cnt ;		/* +24 */
	uchar			xob_wait_exec_cnt ;	/* +25 */
	ushort			rsv0;				/* +26 */
	ushort			wx_que_cnt ;		/* +28 */	/* Scsi start waiting count */
	ushort			we_que_cnt ;		/* +2a */	/* Scsi response waiting count */
	uint			iov_no ;			/* +2c */
	uint			iov_map_cnt ;		/* +30 */
	uint			save_xob_outp ;		/* +34 */
	uint			xob_outp_end;		/* +38 */
	uint			frame_chkp ;		/* +3c */
	uint			frame_inp ;			/* +40 */
	uint			hfcp_iov_no ;		/* +44 */
	uint			hfcp_iov_cnt ;		/* +48 */
	uint			cmd_flags ;			/* +4c */	/* Hfc_pkt control flag  */
	uint			cmd_dmacount ;		/* +50 */	/* Data transfer length  */
	uint			cmd_cookiecnt ;		/* +54 */	/* cookie count          */
	ushort			xob_no ;			/* +58 */	/* Hfc_pkt allocation xob# */
	uchar			rsv1;				/* +5a */
	uchar			type ;				/* +5b */	/* 0:alloc,1:cancel      */
	uint			pos ;				/* +5c */	/* bit position          */
	uint			cnt ;				/* +60 */	/* bit count             */
};

struct stra_trc5 {
	struct trc_com	com ;

	uint			d_scsi_id ;			/* +18 */	/* target  scsi_id */
	uint			xob_outp ;			/* +1c */
	ushort			xob_no ;			/* +20 */
	uchar			xob_exec_cnt ;		/* +22 */
	uchar			xob_wait_exec_cnt ;	/* +23 */
	ushort			wx_que_cnt ;		/* +24 */	/* Scsi start waiting count */
	ushort			we_que_cnt ;		/* +26 */	/* Scsi response waiting count */
	uint			fcp_cntl ;			/* +28 */
	uint			fcp_dl ;			/* +2c */
	uchar			cdb[16] ;			/* +30 */
	uchar			xob[48] ;			/* +40-6f */
};

struct stra_trc6 {
	struct trc_com	com ;

	uint			cmd_flags ;			/* +18 */	/* Hfc_pkt control flag  */
	ushort			cmd_cdblen ;		/* +1c */
	ushort			cmd_scblen ;		/* +1e */
	uint			adap_status ;		/* +20 */
	uint			cmd_cookie ;		/* +24 */
	uint			cmd_ncookie ;		/* +28 */
	uint			cmd_cookiecnt ;		/* +2c */
	uint			cmd_nwin ;			/* +30 */
	uint			cmd_curwin ;		/* +34 */
	uint64_t		cmd_dma_offset ;	/* +38 */
	uint64_t		cmd_dma_len ;		/* +40 */
	uint64_t		cmd_timeout ;		/* +48 */	/* Hfc_pkt timeout time */
	uint			cmd_iov_no ;		/* +50 */	/* hfc_pkt - seg_info#	*/
	uint			cmd_iov_cnt ;		/* +54 */	/* hfc_pkt - Number of seg_info */
	ushort			cmd_xob ;			/* +58 */	/* Hfc_pkt allocation xob# */
	uchar			cmd_status ;		/* +5a */	/* Hfc_pkt status information */
	uchar			rsv1 ;				/* +5b */
	uint			cmd_dmacount ;		/* +60 */	/* Data transfer length		   */
	uint64_t		cmd_forw ;			/* +64 */
	uint64_t		cmd_prev ;			/* +6c */
	uint			pkt_resid ;			/* +74 */	/* scsi_pkt remainder byte */
};

struct stra_trc7 {
	struct trc_com	com ;

	uint			cap ;
	uint			whom ;
	uint			rtn ;
	uint			dma_max ;
	uint			burst_size ;
	uint			scsi_options ;
	uint			linkup_tmo ;
	uint			scsi_reset_deley ;
	uint			need_prop_update ;
	uint			target_scsi_options ;
	uint			cap_info ;
};

/*dmp*/ /*32bitonly*/
struct stra_dump_srb {
	uchar                   id;
	uchar                   sub_id;
	uchar                   resv1[6];
	uint64_t                srb_address;
	uchar                   srb_dump[96];
	uchar                   resv2[8];
};

struct stra_dump_hfcp {
	uchar                   id;
	uchar                   sub_id;
	uchar                   resv1[6];
	uint64_t                hfcp_address;
	uchar                   hfcp_dump[96];
	uchar                   resv2[8];
};

struct stra_dump_weque {
	uchar                   id;
	uchar                   sub_id;
	uchar                   target_id;
	uchar                   hash;
	uint                    we_que_cnt;
	uint                    dmp_pkt[28];
};

