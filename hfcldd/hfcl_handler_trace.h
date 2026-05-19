/*
 * hfcl_handler_trace.h 
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_handler_trace.h,v 1.2.8.5.28.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:48 toyo Exp $
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
#define HFC_TRC_GIDPNRSP        0xad			/* FCWIN-0082 */

/*-----------------------------------------------------------------*/
/*                         trace format                            */
/* The member name of each structure uses the following prefixes   */
/*                                                                 */
/* a_   : aValue acquired from member of adap_info                 */
/* t_   : Value acquired from member of target_info                */
/* s_   : Value acquired from member of Srb                        */
/* h_   : Value acquired from member of hfc_pkt                    */
/*                                                                 */
/*-----------------------------------------------------------------*/

struct hand_trc1 {
	uchar                       id ;
	uchar                       sub_id ;
	uchar                       resv1[2] ;
	uint                        a_status ;
	uint64_t                    a_scsi_id ;
	uint                        int_a_reg;
	uchar                       resv2[100];
	uint64_t                    current_time;
};

struct hand_trc2 {
	uchar                       id;
	uchar                       sub_id ;
	ushort                      mb_retry_cnt ;
	uint                        a_status ;
	uint64_t                    a_scsi_id ;
	uint                        passthrough_rsp;
	uchar                       t_flags;
	uchar                       t_pseq ;
	uchar                       mb_status;
	uchar                       resv1[1];
	uint                        t_status;
	uint                        t_id;
	uint64_t                    a_next_tstart;
	uint64_t                    t_ww_name;
	uint64_t                    t_node_name;
	uchar                       mb_resp[64];
	uint64_t                    current_time;
};

struct hand_trc3 {
	uchar                       id;
	uchar                       sub_id ;
	ushort                      xrb_no ;
	uint                        a_status ;
	uint64_t                    a_scsi_id;
	uint                        xrb_outp ;
	uint                        xrb_inp ;
	ushort                      xrb_cnt ;
	uchar                       xrb[70] ;
	uchar                       resv1[24] ;
	uint64_t                    current_time;
};

struct hand_trc4 {
	uchar                       id;
	uchar                       sub_id ;
	ushort                      xrb_no ;
	uint                        a_status ;
	uint64_t                    a_scsi_id ;
	uint                        t_status;
	uchar                       t_flags;
	uchar                       t_pseq ;
	uchar                       fcp_status;
	uchar                       scsi_status;
	uchar                       t_wait_abort[32];
	uint64_t                    srb;
	uchar                       s_function;
	uchar                       s_status;
	uchar                       s_target;
//	uchar                       s_lun;			/* FCLNX-GPL-0343 */
	uchar						resv0;			/* FCLNX-GPL-0343 */
	uchar                       s_quetag;
	uchar                       s_queaction;
	uchar                       s_cdblen;
	uchar                       s_senselen;
	uint                        s_flags;
	uint                        s_datatransferlength;
	uint64_t                    hfcp;
	uint                        h_adap_status;
	uint                        h_iov_no;
	uint                        h_iov_cnt;
	uint                        t_we_que_cnt;
	ushort						s_lun;			/* FCLNX-GPL-0343 */
	uchar                       resv1[14];		/* FCLNX-GPL-0343 */
	uint64_t                    current_time;
};

struct hand_trc5 {
	uchar                       id;
	uchar                       sub_id;
	ushort                      xrb_no;
	uint                        a_status;
	uint64_t                    a_scsi_id;
	uint                        t_status;
	uchar                       t_flags;
	uchar                       t_pseq;
	ushort                      timer_id;
	uint64_t                    srb;
	uchar                       s_function;
	uchar                       s_status;
	uchar                       s_target;
//	uchar                       s_lun;			/* FCLNX-GPL-0343 */
	uchar						resv0;			/* FCLNX-GPL-0343 */
	ushort						s_lun;			/* FCLNX-GPL-0343 */
	uchar                       resv1[82];		/* FCLNX-GPL-0343 */
	uint64_t                    current_time;
};

struct hand_trc6 {
	uchar                       id;
	uchar                       sub_id;
	uchar                       resv1[2];
	uint                        a_status ;
	uint64_t                    a_scsi_id ;
	uint                        int_a_reg;
	uint                        detail_reg;
	uint64_t                    status_reg;
	uint                        link_dead_cnt ;
	uint                        pci_err_cnt ;
	uint                        mck_err_cnt ;
	uchar                       resv2[76] ;
	uint64_t                    current_time;
};
