/*
 * hfcl_stra_trace_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_stra_trace_fx.h,v 1.1.2.3.2.2.2.1.2.1 2015/05/25 09:26:17 toyo Exp $
 */

/*------ trace sub_id ---------------*/

#define HFC_FX_TRC_TGT_RESET	0x74	/* STRA_TRC Format1 */
#define HFC_FX_TRC_ABORT		0x76	/* STRA_TRC Format1 */
#define HFC_FX_TRC_RESET		0x77	/* STRA_TRC Format1 */
#define HFC_FX_TRC_BUS_RESET	0x78	/* STRA_TRC Format1 */
#define HFC_FX_TRC_ISSUE_TMGM	0x7b	/* STRA_TRC Format1 */
#define HFC_FX_TRC_LUN_RESET	0x7d	/* STRA_TRC Format1 */
#define HFC_FX_TRC_IODONE		0x7f	/* STRA_TRC Format1 */
#define	HFC_FX_TRC_STRATEGY		0x81	/* STRA_TRC Format1 */
#define	HFC_FX_TRC_START		0x82	/* STRA_TRC Format2 */
#define	HFC_FX_TRC_CAN_XOB		0x83	/* STRA_TRC Format3 */
#define	HFC_FX_TRC_CAN_WE		0x84	/* STRA_TRC Format3 */
#define	HFC_FX_TRC_RES_CHK		0x87	/* STRA_TRC Format4 */
#define	HFC_FX_TRC_IOVUP		0x88	/* STRA_TRC Format4 */
#define	HFC_FX_TRC_DMA_MAP		0x89	/* STRA_TRC Format4 */
#define	HFC_FX_TRC_CAN_WX		0x8d	/* STRA_TRC Format3 */


/*---- trace format --------------------*/
struct stra_fx_trc1 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				cmnd0;				/* +05 */
	ushort				lun_id;				/* +06 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uchar				a_status;			/* +10 */
	uchar				timeout;			/* +11 */
	uchar				d_flags;			/* +12 */
	uchar				d_status;			/* +13 */
	uchar				t_flag;				/* +14 */
	uchar				t_status[3];		/* +15 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	uchar				t_id;				/* +1c */
	uchar				t_scsi_id[3];		/* +1d */
	ushort				ct_wx_que_cnt;		/* +20 */
	ushort				ct_we_que_cnt;		/* +22 */
	uchar				c_status;			/* +24 */
	uchar				resv1[1];			/* +25 */
	uchar				t_pseq;				/* +26 */
	uchar				use_seg;			/* +27 */
	uint				retries;			/* +28 */
	uint				allowed;			/* +2c */
	uchar				cmnd[16];			/* +30 */
	uint				resid;				/* +40 */
	uint				serial_number;		/* +44 */
	uint				result;				/* +48 */
	uint				h_cmd_flags;		/* +4c */
	uint				h_adap_status;		/* +50 */
	uint				h_iov_no;			/* +54 */
	uint				h_iov_cnt;			/* +58 */
	uchar				h_cmd_xob;			/* +5c */
	uchar				h_rid;				/* +5d */
	uchar				resv2[2];			/* +5e */
	uint64_t			c_scsi_exec_cnt;	/* +60 */
	uint64_t			c_scsi_end_cnt;		/* +68 */
	uint64_t			hfcp;				/* +70 */
//	uint64_t			current_time;		/* +78-7f */
};

struct stra_fx_trc2 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				r_rid;				/* +05 */
	uchar				t_id;				/* +06 */
	uchar				scsi_cmnd;			/* +07 *//* FCLNX-GPL-FX-061 */
	ushort				ct_wx_que_cnt;		/* +08 */
	ushort				ct_we_que_cnt;		/* +0a */
	ushort				c_drv_next_xob;		/* +0c */
	ushort				c_drv_next_xrb;		/* +0e */
	uchar				resv2[1];			/* +10 */
	uchar				c_initial_xob_no;	/* +11 *//* FCLNX-GPL-FX-061 */
	uchar				c_xob_w_exec_cnt;	/* +12 */
	uchar				c_next_dstart_cnt;	/* +13 */
	uint				c_iov_no;			/* +14 */
	uint				c_frame_inp;		/* +18 */
	uint				c_frame_start_xob;	/* +1c */
	uint				h_data_size;		/* +20 */
	ushort				lun_id;				/* +24 *//* FCLNX-GPL-FX-202 */
	uchar				t_pseq;				/* +26 */
	uchar				use_seg;			/* +27 */
	uint				retries;			/* +28 */
	uint				allowed;			/* +2c */
	uchar				cmnd[16];			/* +30 */
	uint				resid;				/* +40 */
	uint				serial_number;		/* +44 */
	uint				result;				/* +48 */
	uint				h_cmd_flags;		/* +4c */
	uint				h_adap_status;		/* +50 */
	uint				h_iov_no;			/* +54 */
	uint				h_iov_cnt;			/* +58 */
	uchar				h_cmd_xob;			/* +5c */
	uchar				h_rid;				/* +5d */
	uchar				resv4[2];			/* +5e */
	uint64_t			c_scsi_exec_cnt;	/* +60 */
	uint64_t			c_scsi_end_cnt;		/* +68 */
	uint64_t			hfcp;				/* +70 */
//	uint64_t			current_time;		/* +78-7f */
};

struct stra_fx_trc3 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				cmnd0;				/* +05 */
	ushort				lun_id;				/* +06 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uchar				a_status;			/* +10 */
	uchar				timeout;			/* +11 */
	uchar				d_flags;			/* +12 */
	uchar				d_status;			/* +13 */
	uchar				t_flag;				/* +14 */
	uchar				t_status[3];		/* +15 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	uchar				t_id;				/* +1c */
	uchar				t_scsi_id[3];		/* +1d */
	ushort				ct_wx_que_cnt;		/* +20 */
	ushort				ct_we_que_cnt;		/* +22 */
	uchar				c_status;			/* +24 */
	uchar				resv1[1];			/* +25 */
	uchar				t_pseq;				/* +26 */
	uchar				use_seg;			/* +27 */
	uint				retries;			/* +28 */
	uint				allowed;			/* +2c */
	uchar				cmnd[16];			/* +30 */
	uint				resid;				/* +40 */
	uint				serial_number;		/* +44 */
	uint				result;				/* +48 */
	uint				h_cmd_flags;		/* +4c */
	uint				h_adap_status;		/* +50 */
	uint				h_iov_no;			/* +54 */
	uint				h_iov_cnt;			/* +58 */
	uchar				h_cmd_xob;			/* +5c */
	uchar				h_rid;				/* +5d */
	uchar				resv2[2];			/* +5e */
	uint64_t			c_scsi_exec_cnt;	/* +60 */
	uint64_t			c_scsi_end_cnt;		/* +68 */
	uint64_t			hfcp;				/* +70 */
//	uint64_t			current_time;		/* +78-7f */
};

struct stra_fx_trc4 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				r_rid;				/* +05 */
	uchar				t_id;				/* +06 */
	uchar				resv1[1];			/* +07 */
	ushort				ct_wx_que_cnt;		/* +08 */
	ushort				ct_we_que_cnt;		/* +0a */
	ushort				c_drv_next_xob;		/* +0c */
	ushort				c_drv_next_xrb;		/* +0e */
	uchar				resv2[1];			/* +10 */
	uchar				c_xob_exec_cnt;		/* +11 */
	uchar				c_xob_w_exec_cnt;	/* +12 */
	uchar				c_next_dstart_cnt;	/* +13 */
	uint				c_iov_no;			/* +14 */
	uint				c_frame_inp;		/* +18 */
	uint				c_frame_start_xob;	/* +1c */
	uint				h_data_size;		/* +20 */
	ushort				lun_id;				/* +24 *//* FCLNX-GPL-FX-202 */
	uchar				t_pseq;				/* +26 */
	uchar				use_seg;			/* +27 */
	uint				retries;			/* +28 */
	uint				allowed;			/* +2c */
	uchar				cmnd[16];			/* +30 */
	uint				resid;				/* +40 */
	uint				serial_number;		/* +44 */
	uint				result;				/* +48 */
	uint				h_cmd_flags;		/* +4c */
	uint				h_adap_status;		/* +50 */
	uint				h_iov_no;			/* +54 */
	uint				h_iov_cnt;			/* +58 */
	uchar				h_cmd_xob;			/* +5c */
	uchar				h_rid;				/* +5d */
	uchar				resv4[2];			/* +5e */
	uint64_t			c_scsi_exec_cnt;	/* +60 */
	uint64_t			c_scsi_end_cnt;		/* +68 */
	uint64_t			hfcp;				/* +70 */
//	uint64_t			current_time;		/* +78-7f */
};
