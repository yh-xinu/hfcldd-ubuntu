/*
 * hfcl_moduleparam.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_moduleparam.h,v 1.14.2.14.2.2.2.1.2.1.6.10.4.6.2.7.2.8.2.1.2.2.2.3 2015/08/22 06:25:30 toyo Exp $
 */

/* insmod hfcldd hfclddopts=verbose */
int hfc_automap 				= 1;
char *hfclddopts 			= NULL;
int hfc_pxe_boot				= 0;
int hfc_shadow				= 0;

int hfc_narrowmap 			= 0;			/* FCLNX-0392 */

int hfc_scsi_time_out		= 0;

int hfc_connection_type		= -1;
int hfc0_connection_type		= -1;
int hfc1_connection_type		= -1;
int hfc2_connection_type     = -1;
int hfc3_connection_type		= -1;
int hfc4_connection_type		= -1;
int hfc5_connection_type		= -1;
int hfc6_connection_type		= -1;
int hfc7_connection_type		= -1;
int hfc8_connection_type		= -1;
int hfc9_connection_type		= -1;
int hfc10_connection_type	= -1;
int hfc11_connection_type	= -1;
int hfc12_connection_type	= -1;
int hfc13_connection_type	= -1;
int hfc14_connection_type	= -1; 
int hfc15_connection_type	= -1; 
int hfc16_connection_type	= -1;
int hfc17_connection_type	= -1;
int hfc18_connection_type    = -1;
int hfc19_connection_type	= -1;
int hfc20_connection_type	= -1;
int hfc21_connection_type	= -1;
int hfc22_connection_type	= -1;
int hfc23_connection_type	= -1;
int hfc24_connection_type	= -1;
int hfc25_connection_type	= -1;
int hfc26_connection_type	= -1;
int hfc27_connection_type	= -1;
int hfc28_connection_type	= -1;
int hfc29_connection_type	= -1;
int hfc30_connection_type	= -1; 
int hfc31_connection_type	= -1; 
int hfcmp_connection_type[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_link_speed			= -1;	/* Auto */
int hfc0_link_speed			= -1;	
int hfc1_link_speed			= -1;	
int hfc2_link_speed			= -1;	
int hfc3_link_speed			= -1;	
int hfc4_link_speed			= -1;
int hfc5_link_speed			= -1;
int hfc6_link_speed			= -1;
int hfc7_link_speed			= -1;
int hfc8_link_speed			= -1;
int hfc9_link_speed			= -1;
int hfc10_link_speed			= -1;
int hfc11_link_speed			= -1;
int hfc12_link_speed			= -1;
int hfc13_link_speed			= -1;
int hfc14_link_speed			= -1;
int hfc15_link_speed			= -1;
int hfc16_link_speed			= -1;
int hfc17_link_speed			= -1;
int hfc18_link_speed   		= -1;
int hfc19_link_speed			= -1;
int hfc20_link_speed			= -1;
int hfc21_link_speed			= -1;
int hfc22_link_speed			= -1;
int hfc23_link_speed			= -1;
int hfc24_link_speed			= -1;
int hfc25_link_speed			= -1;
int hfc26_link_speed			= -1;
int hfc27_link_speed			= -1;
int hfc28_link_speed			= -1;
int hfc29_link_speed			= -1;
int hfc30_link_speed			= -1; 
int hfc31_link_speed			= -1; 
int hfcmp_link_speed[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_max_transfer			= -1;	/* Auto */
int hfc0_max_transfer		= -1;	
int hfc1_max_transfer		= -1;	
int hfc2_max_transfer		= -1;	
int hfc3_max_transfer		= -1;	
int hfc4_max_transfer		= -1;
int hfc5_max_transfer		= -1;
int hfc6_max_transfer		= -1;
int hfc7_max_transfer		= -1;
int hfc8_max_transfer		= -1;
int hfc9_max_transfer		= -1;
int hfc10_max_transfer		= -1;
int hfc11_max_transfer		= -1;
int hfc12_max_transfer		= -1;
int hfc13_max_transfer		= -1;
int hfc14_max_transfer		= -1;
int hfc15_max_transfer		= -1;
int hfc16_max_transfer		= -1;
int hfc17_max_transfer		= -1;
int hfc18_max_transfer  		= -1;
int hfc19_max_transfer		= -1;
int hfc20_max_transfer		= -1;
int hfc21_max_transfer		= -1;
int hfc22_max_transfer		= -1;
int hfc23_max_transfer		= -1;
int hfc24_max_transfer		= -1;
int hfc25_max_transfer		= -1;
int hfc26_max_transfer		= -1;
int hfc27_max_transfer		= -1;
int hfc28_max_transfer		= -1;
int hfc29_max_transfer		= -1;
int hfc30_max_transfer		= -1; 
int hfc31_max_transfer		= -1; 
int hfcmp_max_transfer[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_link_down			= -1;	/* Auto */
int hfc0_link_down			= -1;	
int hfc1_link_down			= -1;	
int hfc2_link_down			= -1;	
int hfc3_link_down			= -1;	
int hfc4_link_down			= -1;
int hfc5_link_down			= -1;
int hfc6_link_down			= -1;
int hfc7_link_down			= -1;
int hfc8_link_down			= -1;
int hfc9_link_down			= -1;
int hfc10_link_down			= -1;
int hfc11_link_down			= -1;
int hfc12_link_down			= -1;
int hfc13_link_down			= -1;
int hfc14_link_down			= -1;
int hfc15_link_down			= -1;
int hfc16_link_down			= -1;
int hfc17_link_down			= -1;
int hfc18_link_down  		= -1;
int hfc19_link_down			= -1;
int hfc20_link_down			= -1;
int hfc21_link_down			= -1;
int hfc22_link_down			= -1;
int hfc23_link_down			= -1;
int hfc24_link_down			= -1;
int hfc25_link_down			= -1;
int hfc26_link_down			= -1;
int hfc27_link_down			= -1;
int hfc28_link_down			= -1;
int hfc29_link_down			= -1;
int hfc30_link_down			= -1; 
int hfc31_link_down			= -1; 
int hfcmp_link_down[HFC_MAX_INSTANCE_CNT] = {0};

/* FCLNX-0241 */									
int hfc_link_down2			= -1;	/* Auto */
int hfc0_link_down2			= -1;	
int hfc1_link_down2			= -1;	
int hfc2_link_down2			= -1;	
int hfc3_link_down2			= -1;	
int hfc4_link_down2			= -1;
int hfc5_link_down2			= -1;
int hfc6_link_down2			= -1;
int hfc7_link_down2			= -1;
int hfc8_link_down2			= -1;
int hfc9_link_down2			= -1;
int hfc10_link_down2			= -1;
int hfc11_link_down2			= -1;
int hfc12_link_down2			= -1;
int hfc13_link_down2			= -1;
int hfc14_link_down2			= -1;
int hfc15_link_down2			= -1;
int hfc16_link_down2			= -1;
int hfc17_link_down2			= -1;
int hfc18_link_down2  		= -1;
int hfc19_link_down2			= -1;
int hfc20_link_down2			= -1;
int hfc21_link_down2			= -1;
int hfc22_link_down2			= -1;
int hfc23_link_down2			= -1;
int hfc24_link_down2			= -1;
int hfc25_link_down2			= -1;
int hfc26_link_down2			= -1;
int hfc27_link_down2			= -1;
int hfc28_link_down2			= -1;
int hfc29_link_down2			= -1;
int hfc30_link_down2			= -1; 
int hfc31_link_down2			= -1; 
int hfcmp_link_down2[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-0241 */

int hfc_reset_delay			= -1;	/* Auto */
int hfc0_reset_delay			= -1;	
int hfc1_reset_delay			= -1;	
int hfc2_reset_delay			= -1;	
int hfc3_reset_delay			= -1;	
int hfc4_reset_delay			= -1;
int hfc5_reset_delay			= -1;
int hfc6_reset_delay			= -1;
int hfc7_reset_delay			= -1;
int hfc8_reset_delay			= -1;
int hfc9_reset_delay			= -1;
int hfc10_reset_delay		= -1;
int hfc11_reset_delay		= -1;
int hfc12_reset_delay		= -1;
int hfc13_reset_delay		= -1;
int hfc14_reset_delay		= -1;
int hfc15_reset_delay		= -1;
int hfc16_reset_delay		= -1;
int hfc17_reset_delay		= -1;
int hfc18_reset_delay  		= -1;
int hfc19_reset_delay		= -1;
int hfc20_reset_delay		= -1;
int hfc21_reset_delay		= -1;
int hfc22_reset_delay		= -1;
int hfc23_reset_delay		= -1;
int hfc24_reset_delay		= -1;
int hfc25_reset_delay		= -1;
int hfc26_reset_delay		= -1;
int hfc27_reset_delay		= -1;
int hfc28_reset_delay		= -1;
int hfc29_reset_delay		= -1;
int hfc30_reset_delay		= -1; 
int hfc31_reset_delay		= -1; 
int hfcmp_reset_delay[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_mck_retry			= -1;	/* Auto */
int hfc0_mck_retry			= -1;	
int hfc1_mck_retry			= -1;	
int hfc2_mck_retry			= -1;	
int hfc3_mck_retry			= -1;	
int hfc4_mck_retry			= -1;
int hfc5_mck_retry			= -1;
int hfc6_mck_retry			= -1;
int hfc7_mck_retry			= -1;
int hfc8_mck_retry			= -1;
int hfc9_mck_retry			= -1;
int hfc10_mck_retry			= -1;
int hfc11_mck_retry			= -1;
int hfc12_mck_retry			= -1;
int hfc13_mck_retry			= -1;
int hfc14_mck_retry			= -1;
int hfc15_mck_retry			= -1;
int hfc16_mck_retry			= -1;
int hfc17_mck_retry			= -1;
int hfc18_mck_retry  		= -1;
int hfc19_mck_retry			= -1;
int hfc20_mck_retry			= -1;
int hfc21_mck_retry			= -1;
int hfc22_mck_retry			= -1;
int hfc23_mck_retry			= -1;
int hfc24_mck_retry			= -1;
int hfc25_mck_retry			= -1;
int hfc26_mck_retry			= -1;
int hfc27_mck_retry			= -1;
int hfc28_mck_retry			= -1;
int hfc29_mck_retry			= -1;
int hfc30_mck_retry			= -1; 
int hfc31_mck_retry			= -1; 
int hfcmp_mck_retry[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_preferred_alpa		= -1;	/* Auto */
int hfc0_preferred_alpa		= -1;	
int hfc1_preferred_alpa		= -1;	
int hfc2_preferred_alpa		= -1;	
int hfc3_preferred_alpa		= -1;	
int hfc4_preferred_alpa		= -1;
int hfc5_preferred_alpa		= -1;
int hfc6_preferred_alpa		= -1;
int hfc7_preferred_alpa		= -1;
int hfc8_preferred_alpa		= -1;
int hfc9_preferred_alpa		= -1;
int hfc10_preferred_alpa		= -1;
int hfc11_preferred_alpa		= -1;
int hfc12_preferred_alpa		= -1;
int hfc13_preferred_alpa		= -1;
int hfc14_preferred_alpa		= -1;
int hfc15_preferred_alpa		= -1;
int hfc16_preferred_alpa		= -1;
int hfc17_preferred_alpa		= -1;
int hfc18_preferred_alpa  	= -1;
int hfc19_preferred_alpa		= -1;
int hfc20_preferred_alpa		= -1;
int hfc21_preferred_alpa		= -1;
int hfc22_preferred_alpa		= -1;
int hfc23_preferred_alpa		= -1;
int hfc24_preferred_alpa		= -1;
int hfc25_preferred_alpa		= -1;
int hfc26_preferred_alpa		= -1;
int hfc27_preferred_alpa		= -1;
int hfc28_preferred_alpa		= -1;
int hfc29_preferred_alpa		= -1;
int hfc30_preferred_alpa		= -1; 
int hfc31_preferred_alpa		= -1; 
int hfcmp_preferred_alpa[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_reset_timeout		= -1;	/* Auto */
int hfc0_reset_timeout		= -1;	
int hfc1_reset_timeout		= -1;	
int hfc2_reset_timeout		= -1;	
int hfc3_reset_timeout		= -1;	
int hfc4_reset_timeout		= -1;
int hfc5_reset_timeout		= -1;
int hfc6_reset_timeout		= -1;
int hfc7_reset_timeout		= -1;
int hfc8_reset_timeout		= -1;
int hfc9_reset_timeout		= -1;
int hfc10_reset_timeout		= -1;
int hfc11_reset_timeout		= -1;
int hfc12_reset_timeout		= -1;
int hfc13_reset_timeout		= -1;
int hfc14_reset_timeout		= -1;
int hfc15_reset_timeout		= -1;
int hfc16_reset_timeout		= -1;
int hfc17_reset_timeout		= -1;
int hfc18_reset_timeout  	= -1;
int hfc19_reset_timeout		= -1;
int hfc20_reset_timeout		= -1;
int hfc21_reset_timeout		= -1;
int hfc22_reset_timeout		= -1;
int hfc23_reset_timeout		= -1;
int hfc24_reset_timeout		= -1;
int hfc25_reset_timeout		= -1;
int hfc26_reset_timeout		= -1;
int hfc27_reset_timeout		= -1;
int hfc28_reset_timeout		= -1;
int hfc29_reset_timeout		= -1;
int hfc30_reset_timeout		= -1; 
int hfc31_reset_timeout		= -1; 
int hfcmp_reset_timeout[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_abort_timeout		= -1;	/* Auto */
int hfc0_abort_timeout		= -1;	
int hfc1_abort_timeout		= -1;	
int hfc2_abort_timeout		= -1;	
int hfc3_abort_timeout		= -1;	
int hfc4_abort_timeout		= -1;
int hfc5_abort_timeout		= -1;
int hfc6_abort_timeout		= -1;
int hfc7_abort_timeout		= -1;
int hfc8_abort_timeout		= -1;
int hfc9_abort_timeout		= -1;
int hfc10_abort_timeout		= -1;
int hfc11_abort_timeout		= -1;
int hfc12_abort_timeout		= -1;
int hfc13_abort_timeout		= -1;
int hfc14_abort_timeout		= -1;
int hfc15_abort_timeout		= -1;
int hfc16_abort_timeout		= -1;
int hfc17_abort_timeout		= -1;
int hfc18_abort_timeout  	= -1;
int hfc19_abort_timeout		= -1;
int hfc20_abort_timeout		= -1;
int hfc21_abort_timeout		= -1;
int hfc22_abort_timeout		= -1;
int hfc23_abort_timeout		= -1;
int hfc24_abort_timeout		= -1;
int hfc25_abort_timeout		= -1;
int hfc26_abort_timeout		= -1;
int hfc27_abort_timeout		= -1;
int hfc28_abort_timeout		= -1;
int hfc29_abort_timeout		= -1;
int hfc30_abort_timeout		= -1; 
int hfc31_abort_timeout		= -1; 
int hfcmp_abort_timeout[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_enable_tgtrst		= -1; /* enable target reset */
int hfc0_enable_tgtrst		= -1;
int hfc1_enable_tgtrst		= -1;
int hfc2_enable_tgtrst		= -1;
int hfc3_enable_tgtrst		= -1;
int hfc4_enable_tgtrst		= -1;
int hfc5_enable_tgtrst		= -1;
int hfc6_enable_tgtrst		= -1;
int hfc7_enable_tgtrst		= -1;
int hfc8_enable_tgtrst		= -1;
int hfc9_enable_tgtrst		= -1;
int hfc10_enable_tgtrst		= -1;
int hfc11_enable_tgtrst		= -1;
int hfc12_enable_tgtrst		= -1;
int hfc13_enable_tgtrst		= -1;
int hfc14_enable_tgtrst		= -1;
int hfc15_enable_tgtrst		= -1;
int hfc16_enable_tgtrst		= -1;
int hfc17_enable_tgtrst		= -1;
int hfc18_enable_tgtrst  	= -1;
int hfc19_enable_tgtrst		= -1;
int hfc20_enable_tgtrst		= -1;
int hfc21_enable_tgtrst		= -1;
int hfc22_enable_tgtrst		= -1;
int hfc23_enable_tgtrst		= -1;
int hfc24_enable_tgtrst		= -1;
int hfc25_enable_tgtrst		= -1;
int hfc26_enable_tgtrst		= -1;
int hfc27_enable_tgtrst		= -1;
int hfc28_enable_tgtrst		= -1;
int hfc29_enable_tgtrst		= -1;
int hfc30_enable_tgtrst		= -1; 
int hfc31_enable_tgtrst		= -1; 
int hfcmp_enable_tgtrst[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_queue_depth			= -1; /* queue depth */
int hfc0_queue_depth			= -1;
int hfc1_queue_depth			= -1;
int hfc2_queue_depth			= -1;
int hfc3_queue_depth			= -1;
int hfc4_queue_depth			= -1;
int hfc5_queue_depth			= -1;
int hfc6_queue_depth			= -1;
int hfc7_queue_depth			= -1;
int hfc8_queue_depth			= -1;
int hfc9_queue_depth			= -1;
int hfc10_queue_depth		= -1;
int hfc11_queue_depth		= -1;
int hfc12_queue_depth		= -1;
int hfc13_queue_depth		= -1;
int hfc14_queue_depth		= -1;
int hfc15_queue_depth		= -1;
int hfc16_queue_depth		= -1;
int hfc17_queue_depth		= -1;
int hfc18_queue_depth  		= -1;
int hfc19_queue_depth		= -1;
int hfc20_queue_depth		= -1;
int hfc21_queue_depth		= -1;
int hfc22_queue_depth		= -1;
int hfc23_queue_depth		= -1;
int hfc24_queue_depth		= -1;
int hfc25_queue_depth		= -1;
int hfc26_queue_depth		= -1;
int hfc27_queue_depth		= -1;
int hfc28_queue_depth		= -1;
int hfc29_queue_depth		= -1;
int hfc30_queue_depth		= -1; 
int hfc31_queue_depth		= -1; 
int hfcmp_queue_depth[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_seg_trace			= -1;
int hfc0_seg_trace			= -1;
int hfc1_seg_trace			= -1;
int hfc2_seg_trace			= -1;
int hfc3_seg_trace			= -1;
int hfc4_seg_trace			= -1;
int hfc5_seg_trace			= -1;
int hfc6_seg_trace			= -1;
int hfc7_seg_trace			= -1;
int hfc8_seg_trace			= -1;
int hfc9_seg_trace			= -1;
int hfc10_seg_trace			= -1;
int hfc11_seg_trace			= -1;
int hfc12_seg_trace			= -1;
int hfc13_seg_trace			= -1;
int hfc14_seg_trace			= -1;
int hfc15_seg_trace			= -1;
int hfc16_seg_trace			= -1;
int hfc17_seg_trace			= -1;
int hfc18_seg_trace  		= -1;
int hfc19_seg_trace			= -1;
int hfc20_seg_trace			= -1;
int hfc21_seg_trace			= -1;
int hfc22_seg_trace			= -1;
int hfc23_seg_trace			= -1;
int hfc24_seg_trace			= -1;
int hfc25_seg_trace			= -1;
int hfc26_seg_trace			= -1;
int hfc27_seg_trace			= -1;
int hfc28_seg_trace			= -1;
int hfc29_seg_trace			= -1;
int hfc30_seg_trace			= -1; 
int hfc31_seg_trace			= -1; 
int hfcmp_seg_trace[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_message_enable		= 1;

int hfc_max_target			= -1;
int hfc0_max_target			= -1;
int hfc1_max_target			= -1;
int hfc2_max_target			= -1;
int hfc3_max_target			= -1;
int hfc4_max_target			= -1;
int hfc5_max_target			= -1;
int hfc6_max_target			= -1;
int hfc7_max_target			= -1;
int hfc8_max_target			= -1;
int hfc9_max_target			= -1;
int hfc10_max_target			= -1;
int hfc11_max_target			= -1;
int hfc12_max_target			= -1;
int hfc13_max_target			= -1;
int hfc14_max_target			= -1;
int hfc15_max_target			= -1;
int hfc16_max_target			= -1;
int hfc17_max_target			= -1;
int hfc18_max_target  		= -1;
int hfc19_max_target			= -1;
int hfc20_max_target			= -1;
int hfc21_max_target			= -1;
int hfc22_max_target			= -1;
int hfc23_max_target			= -1;
int hfc24_max_target			= -1;
int hfc25_max_target			= -1;
int hfc26_max_target			= -1;
int hfc27_max_target			= -1;
int hfc28_max_target			= -1;
int hfc29_max_target			= -1;
int hfc30_max_target			= -1; 
int hfc31_max_target			= -1; 
int hfcmp_max_target[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_xob_max			= -1;
int hfc0_xob_max			= -1;
int hfc1_xob_max			= -1;
int hfc2_xob_max			= -1;
int hfc3_xob_max			= -1;
int hfc4_xob_max			= -1;
int hfc5_xob_max			= -1;
int hfc6_xob_max			= -1;
int hfc7_xob_max			= -1;
int hfc8_xob_max			= -1;
int hfc9_xob_max			= -1;
int hfc10_xob_max		= -1;
int hfc11_xob_max		= -1;
int hfc12_xob_max		= -1;
int hfc13_xob_max		= -1;
int hfc14_xob_max		= -1;
int hfc15_xob_max		= -1;
int hfc16_xob_max		= -1;
int hfc17_xob_max		= -1;
int hfc18_xob_max	  	= -1;
int hfc19_xob_max		= -1;
int hfc20_xob_max		= -1;
int hfc21_xob_max		= -1;
int hfc22_xob_max		= -1;
int hfc23_xob_max		= -1;
int hfc24_xob_max		= -1;
int hfc25_xob_max		= -1;
int hfc26_xob_max		= -1;
int hfc27_xob_max		= -1;
int hfc28_xob_max		= -1;
int hfc29_xob_max		= -1;
int hfc30_xob_max		= -1; 
int hfc31_xob_max		= -1; 
int hfcmp_xob_max[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_xrb_max			= -1;
int hfc0_xrb_max			= -1;
int hfc1_xrb_max			= -1;
int hfc2_xrb_max			= -1;
int hfc3_xrb_max			= -1;
int hfc4_xrb_max			= -1;
int hfc5_xrb_max			= -1;
int hfc6_xrb_max			= -1;
int hfc7_xrb_max			= -1;
int hfc8_xrb_max			= -1;
int hfc9_xrb_max			= -1;
int hfc10_xrb_max		= -1;
int hfc11_xrb_max		= -1;
int hfc12_xrb_max		= -1;
int hfc13_xrb_max		= -1;
int hfc14_xrb_max		= -1;
int hfc15_xrb_max		= -1;
int hfc16_xrb_max		= -1;
int hfc17_xrb_max		= -1;
int hfc18_xrb_max  		= -1;
int hfc19_xrb_max		= -1;
int hfc20_xrb_max		= -1;
int hfc21_xrb_max		= -1;
int hfc22_xrb_max		= -1;
int hfc23_xrb_max		= -1;
int hfc24_xrb_max		= -1;
int hfc25_xrb_max		= -1;
int hfc26_xrb_max		= -1;
int hfc27_xrb_max		= -1;
int hfc28_xrb_max		= -1;
int hfc29_xrb_max		= -1;
int hfc30_xrb_max		= -1; 
int hfc31_xrb_max		= -1; 
int hfcmp_xrb_max[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_slog_max			= -1;
int hfc0_slog_max		= -1;
int hfc1_slog_max		= -1;
int hfc2_slog_max		= -1;
int hfc3_slog_max		= -1;
int hfc4_slog_max		= -1;
int hfc5_slog_max		= -1;
int hfc6_slog_max		= -1;
int hfc7_slog_max		= -1;
int hfc8_slog_max		= -1;
int hfc9_slog_max		= -1;
int hfc10_slog_max		= -1;
int hfc11_slog_max		= -1;
int hfc12_slog_max		= -1;
int hfc13_slog_max		= -1;
int hfc14_slog_max		= -1;
int hfc15_slog_max		= -1;
int hfc16_slog_max		= -1;
int hfc17_slog_max		= -1;
int hfc18_slog_max  		= -1;
int hfc19_slog_max		= -1;
int hfc20_slog_max		= -1;
int hfc21_slog_max		= -1;
int hfc22_slog_max		= -1;
int hfc23_slog_max		= -1;
int hfc24_slog_max		= -1;
int hfc25_slog_max		= -1;
int hfc26_slog_max		= -1;
int hfc27_slog_max		= -1;
int hfc28_slog_max		= -1;
int hfc29_slog_max		= -1;
int hfc30_slog_max		= -1; 
int hfc31_slog_max		= -1; 
int hfcmp_slog_max[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_trc_max			= -1;
int hfc0_trc_max			= -1;
int hfc1_trc_max			= -1;
int hfc2_trc_max			= -1;
int hfc3_trc_max			= -1;
int hfc4_trc_max			= -1;
int hfc5_trc_max			= -1;
int hfc6_trc_max			= -1;
int hfc7_trc_max			= -1;
int hfc8_trc_max			= -1;
int hfc9_trc_max			= -1;
int hfc10_trc_max		= -1;
int hfc11_trc_max		= -1;
int hfc12_trc_max		= -1;
int hfc13_trc_max		= -1;
int hfc14_trc_max		= -1;
int hfc15_trc_max		= -1;
int hfc16_trc_max		= -1;
int hfc17_trc_max		= -1;
int hfc18_trc_max  		= -1;
int hfc19_trc_max		= -1;
int hfc20_trc_max		= -1;
int hfc21_trc_max		= -1;
int hfc22_trc_max		= -1;
int hfc23_trc_max		= -1;
int hfc24_trc_max		= -1;
int hfc25_trc_max		= -1;
int hfc26_trc_max		= -1;
int hfc27_trc_max		= -1;
int hfc28_trc_max		= -1;
int hfc29_trc_max		= -1;
int hfc30_trc_max		= -1; 
int hfc31_trc_max		= -1; 
int hfcmp_trc_max[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_pkt_num			= -1;
int hfc0_pkt_num			= -1;
int hfc1_pkt_num			= -1;
int hfc2_pkt_num			= -1;
int hfc3_pkt_num			= -1;
int hfc4_pkt_num			= -1;
int hfc5_pkt_num			= -1;
int hfc6_pkt_num			= -1;
int hfc7_pkt_num			= -1;
int hfc8_pkt_num			= -1;
int hfc9_pkt_num			= -1;
int hfc10_pkt_num		= -1;
int hfc11_pkt_num		= -1;
int hfc12_pkt_num		= -1;
int hfc13_pkt_num		= -1;
int hfc14_pkt_num		= -1;
int hfc15_pkt_num		= -1;
int hfc16_pkt_num		= -1;
int hfc17_pkt_num		= -1;
int hfc18_pkt_num  		= -1;
int hfc19_pkt_num		= -1;
int hfc20_pkt_num		= -1;
int hfc21_pkt_num		= -1;
int hfc22_pkt_num		= -1;
int hfc23_pkt_num		= -1;
int hfc24_pkt_num		= -1;
int hfc25_pkt_num		= -1;
int hfc26_pkt_num		= -1;
int hfc27_pkt_num		= -1;
int hfc28_pkt_num		= -1;
int hfc29_pkt_num		= -1;
int hfc30_pkt_num		= -1; 
int hfc31_pkt_num		= -1; 
int hfcmp_pkt_num[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_can_queue			= -1;
int hfc0_can_queue			= -1;
int hfc1_can_queue			= -1;
int hfc2_can_queue			= -1;
int hfc3_can_queue			= -1;
int hfc4_can_queue			= -1;
int hfc5_can_queue			= -1;
int hfc6_can_queue			= -1;
int hfc7_can_queue			= -1;
int hfc8_can_queue			= -1;
int hfc9_can_queue			= -1;
int hfc10_can_queue			= -1;
int hfc11_can_queue			= -1;
int hfc12_can_queue			= -1;
int hfc13_can_queue			= -1;
int hfc14_can_queue			= -1;
int hfc15_can_queue			= -1;
int hfc16_can_queue			= -1;
int hfc17_can_queue			= -1;
int hfc18_can_queue  		= -1;
int hfc19_can_queue			= -1;
int hfc20_can_queue			= -1;
int hfc21_can_queue			= -1;
int hfc22_can_queue			= -1;
int hfc23_can_queue			= -1;
int hfc24_can_queue			= -1;
int hfc25_can_queue			= -1;
int hfc26_can_queue			= -1;
int hfc27_can_queue			= -1;
int hfc28_can_queue			= -1;
int hfc29_can_queue			= -1;
int hfc30_can_queue			= -1; 
int hfc31_can_queue			= -1; 
int hfcmp_can_queue[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_sg_tblsize			= -1;
int hfc0_sg_tblsize			= -1;
int hfc1_sg_tblsize			= -1;
int hfc2_sg_tblsize			= -1;
int hfc3_sg_tblsize			= -1;
int hfc4_sg_tblsize			= -1;
int hfc5_sg_tblsize			= -1;
int hfc6_sg_tblsize			= -1;
int hfc7_sg_tblsize			= -1;
int hfc8_sg_tblsize			= -1;
int hfc9_sg_tblsize			= -1;
int hfc10_sg_tblsize			= -1;
int hfc11_sg_tblsize			= -1;
int hfc12_sg_tblsize			= -1;
int hfc13_sg_tblsize			= -1;
int hfc14_sg_tblsize			= -1;
int hfc15_sg_tblsize			= -1;
int hfc16_sg_tblsize			= -1;
int hfc17_sg_tblsize			= -1;
int hfc18_sg_tblsize  		= -1;
int hfc19_sg_tblsize			= -1;
int hfc20_sg_tblsize			= -1;
int hfc21_sg_tblsize			= -1;
int hfc22_sg_tblsize			= -1;
int hfc23_sg_tblsize			= -1;
int hfc24_sg_tblsize			= -1;
int hfc25_sg_tblsize			= -1;
int hfc26_sg_tblsize			= -1;
int hfc27_sg_tblsize			= -1;
int hfc28_sg_tblsize			= -1;
int hfc29_sg_tblsize			= -1;
int hfc30_sg_tblsize			= -1; 
int hfc31_sg_tblsize			= -1; 
int hfcmp_sg_tblsize[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cmnd_num				= -1;
int hfc0_cmnd_num			= -1;
int hfc1_cmnd_num			= -1;
int hfc2_cmnd_num			= -1;
int hfc3_cmnd_num			= -1;
int hfc4_cmnd_num			= -1;
int hfc5_cmnd_num			= -1;
int hfc6_cmnd_num			= -1;
int hfc7_cmnd_num			= -1;
int hfc8_cmnd_num			= -1;
int hfc9_cmnd_num			= -1;
int hfc10_cmnd_num			= -1;
int hfc11_cmnd_num			= -1;
int hfc12_cmnd_num			= -1;
int hfc13_cmnd_num			= -1;
int hfc14_cmnd_num			= -1;
int hfc15_cmnd_num			= -1;
int hfc16_cmnd_num			= -1;
int hfc17_cmnd_num			= -1;
int hfc18_cmnd_num  			= -1;
int hfc19_cmnd_num			= -1;
int hfc20_cmnd_num			= -1;
int hfc21_cmnd_num			= -1;
int hfc22_cmnd_num			= -1;
int hfc23_cmnd_num			= -1;
int hfc24_cmnd_num			= -1;
int hfc25_cmnd_num			= -1;
int hfc26_cmnd_num			= -1;
int hfc27_cmnd_num			= -1;
int hfc28_cmnd_num			= -1;
int hfc29_cmnd_num			= -1;
int hfc30_cmnd_num			= -1; 
int hfc31_cmnd_num			= -1; 
int hfcmp_cmnd_num[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_minus_tout			= -1;
int hfc0_minus_tout			= -1;
int hfc1_minus_tout			= -1;
int hfc2_minus_tout			= -1;
int hfc3_minus_tout			= -1;
int hfc4_minus_tout			= -1;
int hfc5_minus_tout			= -1;
int hfc6_minus_tout			= -1;
int hfc7_minus_tout			= -1;
int hfc8_minus_tout			= -1;
int hfc9_minus_tout			= -1;
int hfc10_minus_tout			= -1;
int hfc11_minus_tout			= -1;
int hfc12_minus_tout			= -1;
int hfc13_minus_tout			= -1;
int hfc14_minus_tout			= -1;
int hfc15_minus_tout			= -1;
int hfc16_minus_tout			= -1;
int hfc17_minus_tout			= -1;
int hfc18_minus_tout  		= -1;
int hfc19_minus_tout			= -1;
int hfc20_minus_tout			= -1;
int hfc21_minus_tout			= -1;
int hfc22_minus_tout			= -1;
int hfc23_minus_tout			= -1;
int hfc24_minus_tout			= -1;
int hfc25_minus_tout			= -1;
int hfc26_minus_tout			= -1;
int hfc27_minus_tout			= -1;
int hfc28_minus_tout			= -1;
int hfc29_minus_tout			= -1;
int hfc30_minus_tout			= -1; 
int hfc31_minus_tout			= -1; 
int hfcmp_minus_tout[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_scsi_allowed			= -1;
int hfc0_scsi_allowed		= -1;
int hfc1_scsi_allowed		= -1;
int hfc2_scsi_allowed		= -1;
int hfc3_scsi_allowed		= -1;
int hfc4_scsi_allowed		= -1;
int hfc5_scsi_allowed		= -1;
int hfc6_scsi_allowed		= -1;
int hfc7_scsi_allowed		= -1;
int hfc8_scsi_allowed		= -1;
int hfc9_scsi_allowed		= -1;
int hfc10_scsi_allowed		= -1;
int hfc11_scsi_allowed		= -1;
int hfc12_scsi_allowed		= -1;
int hfc13_scsi_allowed		= -1;
int hfc14_scsi_allowed		= -1;
int hfc15_scsi_allowed		= -1;
int hfc16_scsi_allowed		= -1;
int hfc17_scsi_allowed		= -1;
int hfc18_scsi_allowed  		= -1;
int hfc19_scsi_allowed		= -1;
int hfc20_scsi_allowed		= -1;
int hfc21_scsi_allowed		= -1;
int hfc22_scsi_allowed		= -1;
int hfc23_scsi_allowed		= -1;
int hfc24_scsi_allowed		= -1;
int hfc25_scsi_allowed		= -1;
int hfc26_scsi_allowed		= -1;
int hfc27_scsi_allowed		= -1;
int hfc28_scsi_allowed		= -1;
int hfc29_scsi_allowed		= -1;
int hfc30_scsi_allowed		= -1; 
int hfc31_scsi_allowed		= -1; 
int hfcmp_scsi_allowed[HFC_MAX_INSTANCE_CNT] = {0};

/* FCLNX-GPL-0343 */
int hfc_login_retry		= -1;	/* Auto */
int hfc0_login_retry		= -1;
int hfc1_login_retry		= -1;
int hfc2_login_retry		= -1;
int hfc3_login_retry		= -1;
int hfc4_login_retry		= -1;
int hfc5_login_retry		= -1;
int hfc6_login_retry		= -1;
int hfc7_login_retry		= -1;
int hfc8_login_retry		= -1;
int hfc9_login_retry		= -1;
int hfc10_login_retry		= -1;
int hfc11_login_retry		= -1;
int hfc12_login_retry		= -1;
int hfc13_login_retry		= -1;
int hfc14_login_retry		= -1;
int hfc15_login_retry		= -1;
int hfc16_login_retry		= -1;
int hfc17_login_retry		= -1;
int hfc18_login_retry  	= -1;
int hfc19_login_retry		= -1;
int hfc20_login_retry		= -1;
int hfc21_login_retry		= -1;
int hfc22_login_retry		= -1;
int hfc23_login_retry		= -1;
int hfc24_login_retry		= -1;
int hfc25_login_retry		= -1;
int hfc26_login_retry		= -1;
int hfc27_login_retry		= -1;
int hfc28_login_retry		= -1;
int hfc29_login_retry		= -1;
int hfc30_login_retry		= -1;
int hfc31_login_retry		= -1;
int hfcmp_login_retry[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-0343 */

/* FCLNX-GPL-0343 */
int hfc_els_retry		= -1;	/* Auto */
int hfc0_els_retry		= -1;
int hfc1_els_retry		= -1;
int hfc2_els_retry		= -1;
int hfc3_els_retry		= -1;
int hfc4_els_retry		= -1;
int hfc5_els_retry		= -1;
int hfc6_els_retry		= -1;
int hfc7_els_retry		= -1;
int hfc8_els_retry		= -1;
int hfc9_els_retry		= -1;
int hfc10_els_retry		= -1;
int hfc11_els_retry		= -1;
int hfc12_els_retry		= -1;
int hfc13_els_retry		= -1;
int hfc14_els_retry		= -1;
int hfc15_els_retry		= -1;
int hfc16_els_retry		= -1;
int hfc17_els_retry		= -1;
int hfc18_els_retry  	= -1;
int hfc19_els_retry		= -1;
int hfc20_els_retry		= -1;
int hfc21_els_retry		= -1;
int hfc22_els_retry		= -1;
int hfc23_els_retry		= -1;
int hfc24_els_retry		= -1;
int hfc25_els_retry		= -1;
int hfc26_els_retry		= -1;
int hfc27_els_retry		= -1;
int hfc28_els_retry		= -1;
int hfc29_els_retry		= -1;
int hfc30_els_retry		= -1;
int hfc31_els_retry		= -1;
int hfcmp_els_retry[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-0343 */

/* FCLNX-GPL-0343 */
int hfc_ioctl_scsi_timeout		= -1;	/* Auto */
int hfc0_ioctl_scsi_timeout		= -1;
int hfc1_ioctl_scsi_timeout		= -1;
int hfc2_ioctl_scsi_timeout		= -1;
int hfc3_ioctl_scsi_timeout		= -1;
int hfc4_ioctl_scsi_timeout		= -1;
int hfc5_ioctl_scsi_timeout		= -1;
int hfc6_ioctl_scsi_timeout		= -1;
int hfc7_ioctl_scsi_timeout		= -1;
int hfc8_ioctl_scsi_timeout		= -1;
int hfc9_ioctl_scsi_timeout		= -1;
int hfc10_ioctl_scsi_timeout		= -1;
int hfc11_ioctl_scsi_timeout		= -1;
int hfc12_ioctl_scsi_timeout		= -1;
int hfc13_ioctl_scsi_timeout		= -1;
int hfc14_ioctl_scsi_timeout		= -1;
int hfc15_ioctl_scsi_timeout		= -1;
int hfc16_ioctl_scsi_timeout		= -1;
int hfc17_ioctl_scsi_timeout		= -1;
int hfc18_ioctl_scsi_timeout  	= -1;
int hfc19_ioctl_scsi_timeout		= -1;
int hfc20_ioctl_scsi_timeout		= -1;
int hfc21_ioctl_scsi_timeout		= -1;
int hfc22_ioctl_scsi_timeout		= -1;
int hfc23_ioctl_scsi_timeout		= -1;
int hfc24_ioctl_scsi_timeout		= -1;
int hfc25_ioctl_scsi_timeout		= -1;
int hfc26_ioctl_scsi_timeout		= -1;
int hfc27_ioctl_scsi_timeout		= -1;
int hfc28_ioctl_scsi_timeout		= -1;
int hfc29_ioctl_scsi_timeout		= -1;
int hfc30_ioctl_scsi_timeout		= -1;
int hfc31_ioctl_scsi_timeout		= -1;
int hfcmp_ioctl_scsi_timeout[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-0343 */

int hfc_max_sectors			= -1;
int hfc0_max_sectors		= -1;
int hfc1_max_sectors		= -1;
int hfc2_max_sectors		= -1;
int hfc3_max_sectors		= -1;
int hfc4_max_sectors		= -1;
int hfc5_max_sectors		= -1;
int hfc6_max_sectors		= -1;
int hfc7_max_sectors		= -1;
int hfc8_max_sectors		= -1;
int hfc9_max_sectors		= -1;
int hfc10_max_sectors		= -1;
int hfc11_max_sectors		= -1;
int hfc12_max_sectors		= -1;
int hfc13_max_sectors		= -1;
int hfc14_max_sectors		= -1;
int hfc15_max_sectors		= -1;
int hfc16_max_sectors		= -1;
int hfc17_max_sectors		= -1;
int hfc18_max_sectors  		= -1;
int hfc19_max_sectors		= -1;
int hfc20_max_sectors		= -1;
int hfc21_max_sectors		= -1;
int hfc22_max_sectors		= -1;
int hfc23_max_sectors		= -1;
int hfc24_max_sectors		= -1;
int hfc25_max_sectors		= -1;
int hfc26_max_sectors		= -1;
int hfc27_max_sectors		= -1;
int hfc28_max_sectors		= -1;
int hfc29_max_sectors		= -1;
int hfc30_max_sectors		= -1; 
int hfc31_max_sectors		= -1; 
int hfcmp_max_sectors[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cmd_per_lun			= -1;
int hfc0_cmd_per_lun		= -1;
int hfc1_cmd_per_lun		= -1;
int hfc2_cmd_per_lun		= -1;
int hfc3_cmd_per_lun		= -1;
int hfc4_cmd_per_lun		= -1;
int hfc5_cmd_per_lun		= -1;
int hfc6_cmd_per_lun		= -1;
int hfc7_cmd_per_lun		= -1;
int hfc8_cmd_per_lun		= -1;
int hfc9_cmd_per_lun		= -1;
int hfc10_cmd_per_lun		= -1;
int hfc11_cmd_per_lun		= -1;
int hfc12_cmd_per_lun		= -1;
int hfc13_cmd_per_lun		= -1;
int hfc14_cmd_per_lun		= -1;
int hfc15_cmd_per_lun		= -1;
int hfc16_cmd_per_lun		= -1;
int hfc17_cmd_per_lun		= -1;
int hfc18_cmd_per_lun  		= -1;
int hfc19_cmd_per_lun		= -1;
int hfc20_cmd_per_lun		= -1;
int hfc21_cmd_per_lun		= -1;
int hfc22_cmd_per_lun		= -1;
int hfc23_cmd_per_lun		= -1;
int hfc24_cmd_per_lun		= -1;
int hfc25_cmd_per_lun		= -1;
int hfc26_cmd_per_lun		= -1;
int hfc27_cmd_per_lun		= -1;
int hfc28_cmd_per_lun		= -1;
int hfc29_cmd_per_lun		= -1;
int hfc30_cmd_per_lun		= -1; 
int hfc31_cmd_per_lun		= -1; 
int hfcmp_cmd_per_lun[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_vary_io			= -1;
int hfc0_vary_io		= -1;
int hfc1_vary_io		= -1;
int hfc2_vary_io		= -1;
int hfc3_vary_io		= -1;
int hfc4_vary_io		= -1;
int hfc5_vary_io		= -1;
int hfc6_vary_io		= -1;
int hfc7_vary_io		= -1;
int hfc8_vary_io		= -1;
int hfc9_vary_io		= -1;
int hfc10_vary_io		= -1;
int hfc11_vary_io		= -1;
int hfc12_vary_io		= -1;
int hfc13_vary_io		= -1;
int hfc14_vary_io		= -1;
int hfc15_vary_io		= -1;
int hfc16_vary_io		= -1;
int hfc17_vary_io		= -1;
int hfc18_vary_io  		= -1;
int hfc19_vary_io		= -1;
int hfc20_vary_io		= -1;
int hfc21_vary_io		= -1;
int hfc22_vary_io		= -1;
int hfc23_vary_io		= -1;
int hfc24_vary_io		= -1;
int hfc25_vary_io		= -1;
int hfc26_vary_io		= -1;
int hfc27_vary_io		= -1;
int hfc28_vary_io		= -1;
int hfc29_vary_io		= -1;
int hfc30_vary_io		= -1; 
int hfc31_vary_io		= -1; 
int hfcmp_vary_io[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_lun_reset_delay			= -1;	/* FCLNX-GPL-038 */
int hfc0_lun_reset_delay		= -1;
int hfc1_lun_reset_delay		= -1;
int hfc2_lun_reset_delay		= -1;
int hfc3_lun_reset_delay		= -1;
int hfc4_lun_reset_delay		= -1;
int hfc5_lun_reset_delay		= -1;
int hfc6_lun_reset_delay		= -1;
int hfc7_lun_reset_delay		= -1;
int hfc8_lun_reset_delay		= -1;
int hfc9_lun_reset_delay		= -1;
int hfc10_lun_reset_delay		= -1;
int hfc11_lun_reset_delay		= -1;
int hfc12_lun_reset_delay		= -1;
int hfc13_lun_reset_delay		= -1;
int hfc14_lun_reset_delay		= -1;
int hfc15_lun_reset_delay		= -1;
int hfc16_lun_reset_delay		= -1;
int hfc17_lun_reset_delay		= -1;
int hfc18_lun_reset_delay  		= -1;
int hfc19_lun_reset_delay		= -1;
int hfc20_lun_reset_delay		= -1;
int hfc21_lun_reset_delay		= -1;
int hfc22_lun_reset_delay		= -1;
int hfc23_lun_reset_delay		= -1;
int hfc24_lun_reset_delay		= -1;
int hfc25_lun_reset_delay		= -1;
int hfc26_lun_reset_delay		= -1;
int hfc27_lun_reset_delay		= -1;
int hfc28_lun_reset_delay		= -1;
int hfc29_lun_reset_delay		= -1;
int hfc30_lun_reset_delay		= -1; 
int hfc31_lun_reset_delay		= -1; 						/* FCLNX-GPL-038 */
int hfcmp_lun_rst_delay[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_abort_t_restrain							= -1;	/*FCLNX-0506*/
int hfcmp_abort_t_restrain[HFC_MAX_INSTANCE_CNT]	= {0};	/*FCLNX-0506*/

int hfc_tgtrst_restrain								= -1;
int hfcmp_tgtrst_restrain[HFC_MAX_INSTANCE_CNT]		= {0};

int hfc_login_restrain							= -1;		/*FCLNX-0506*/
int hfcmp_login_restrain[HFC_MAX_INSTANCE_CNT]	= {0};		/*FCLNX-0506*/

uchar hfc_mck_point = 0;	/*FCLNX-0535*/

/* PCIe IP Core SRAM ERR(CE) */
int hfc_pcie_sram_ce = -1;
int hfcmp_pcie_sram_ce[HFC_MAX_INSTANCE_CNT] = {0};

/* FIVE-FX PCIe IP Core SRAM ERR(CE) */
int hfc_pcie_sram_ce_fx = -1;
int hfcmp_pcie_sram_ce_fx[HFC_MAX_INSTANCE_CNT] = {0};

/* Core ERR(CE) */
int hfc_core_ce = -1;
int hfcmp_core_ce[HFC_MAX_INSTANCE_CNT] = {0};

/* INT type */
int hfc_msi_enable = -1;
int hfcmp_msi_enable[HFC_MAX_INSTANCE_CNT] = {0};

/* INT_A dummy read "1:Do" or "0:Not" dummy read. (for MSI/MSI-X) */
int hfc_inta_dummy_read = -1;
int hfcmp_inta_dummy_read[HFC_MAX_INSTANCE_CNT] = {0};

/* Max HW log page count.(0 - 16) */
int hfc_max_hwlog_cnt = -1;
int hfcmp_max_hwlog_cnt[HFC_MAX_INSTANCE_CNT] = {0};

/* Debug mode */
int hfc_debug_func = -1;
int hfcmp_debug_func[HFC_MAX_INSTANCE_CNT] = {0};

/* Set Issue D3 Hot in suspend Process */
int hfc_issue_d3hot = -1;
int hfcmp_issue_d3hot[HFC_MAX_INSTANCE_CNT] = {0};

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* sysfs control */
int hfc_sysfs_control = -1;
int hfcmp_sysfs_control[HFC_MAX_INSTANCE_CNT] = {0};
/* dev_loss_tmo (1 - 255) */
int hfc_dev_loss_tmo = -1;
int hfcmp_dev_loss_tmo[HFC_MAX_INSTANCE_CNT] = {0};
/* scan_finished time out value (0 - 3600) */ /* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
int hfc_scan_finished_tmo = -1;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
/* rport lu scan control */	/* FCLNX-GPL-575 */
int hfc_rport_lu_scan = 1;
int hfcmp_rport_lu_scan[HFC_MAX_INSTANCE_CNT] = {0};

/* FCLNX-GPL-349 */
/* HBA Isolation */
int hfc_hba_isolation			= -1;

/* Linkdown(s) Limit */
int hfc_ld_err_limit_s			= -1;
int hfcmp_ld_err_limit_s[HFC_MAX_INSTANCE_CNT] = {0};

/* interface Error Limit */
int hfc_if_err_limit				= -1;
int hfcmp_if_err_limit[HFC_MAX_INSTANCE_CNT] = {0};

/* Time-Out Error Limit */
int hfc_to_err_limit				= -1;
int hfcmp_to_err_limit[HFC_MAX_INSTANCE_CNT] = {0};

/* Mailbox Time-Out Retry */
int hfc_to_reset_retry			= -1;
int hfcmp_to_reset_retry[HFC_MAX_INSTANCE_CNT] = {0};

/* Time-Out Reset Error */
int hfc_rt_err_enable			= -1;
int hfcmp_rt_err_enable[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-349 */

/* Limit Log *//* FCLNX-GPL-FX-366 */
int hfc_limit_log				= -1;
int hfc0_limit_log				= -1;
int hfc1_limit_log				= -1;
int hfc2_limit_log				= -1;
int hfc3_limit_log				= -1;
int hfc4_limit_log				= -1;
int hfc5_limit_log				= -1;
int hfc6_limit_log				= -1;
int hfc7_limit_log				= -1;
int hfc8_limit_log				= -1;
int hfc9_limit_log				= -1;
int hfc10_limit_log				= -1;
int hfc11_limit_log				= -1;
int hfc12_limit_log				= -1;
int hfc13_limit_log				= -1;
int hfc14_limit_log				= -1;
int hfc15_limit_log				= -1;
/* FCLNX-GPL-491 */

/* Filtering Login Target *//* FCLNX-GPL-FX-366 */
int hfc_filter_target			= -1;
int hfc0_filter_target			= -1;
int hfc1_filter_target			= -1;
int hfc2_filter_target			= -1;
int hfc3_filter_target			= -1;
int hfc4_filter_target			= -1;
int hfc5_filter_target			= -1;
int hfc6_filter_target			= -1;
int hfc7_filter_target			= -1;
int hfc8_filter_target			= -1;
int hfc9_filter_target			= -1;
int hfc10_filter_target			= -1;
int hfc11_filter_target			= -1;
int hfc12_filter_target			= -1;
int hfc13_filter_target			= -1;
int hfc14_filter_target			= -1;
int hfc15_filter_target			= -1;
int hfcmp_filter_target[HFC_MAX_INSTANCE_CNT] = {0};	/* FCLNX-GPL-FX-478 */
/* FCLNX-GPL-491 */

/* Statistics for Virtage */
char hfc_hg_stats_disable		= -1;
/* FCLNX-GPL-491 */

/* FCLNX-GPL-547 start */
/* log file mode */
static char hfc_log_file			= -1;

/* log file mode */
static int hfc_max_lun				= -1;
/* FCLNX-GPL-547 end */

int hfc_ctl_change_qdepth 		= -1;
int hfcmp_ctl_change_qdepth[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-574 */

int hfc_core_control 			= -1;
int hfcmp_core_control[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cc_cnt 					= -1;
int hfcmp_cc_cnt[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cc_size 				= -1;
int hfcmp_cc_size[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cc_core 				= -1;
int hfcmp_cc_core[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_link_reset				= -1;
int hfcmp_link_reset[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_vport_count				= -1;
int hfcmp_vport_count[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_frame_count				= -1;
int hfcmp_frame_count[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_rss_vector_count		= -1;
int hfcmp_rss_vector_count[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_rdtsc					= -1;
int hfcmp_rdtsc[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_intdisable				= -1;
int hfcmp_intdisable[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_intenable				= -1;
int hfcmp_intenable[HFC_MAX_INSTANCE_CNT] = {0};

/* FCLNX-GPL-FX-014 Start */
int hfc_total_abort_to				= -1;
int hfcmp_total_abort_to[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_total_tgtrst_to				= -1;
int hfcmp_total_tgtrst_to[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-FX-014 End */

/* FCLNX-GPL-FX-147 */
int hfc_maxio				= -1;
int hfcmp_maxio[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-FX-147 */

/* FCLNX-GPL-FX-446 >>> */
int hfc_login_seq_retry_cnt			= -1;
int hfcmp_login_seq_retry_cnt[HFC_MAX_INSTANCE_CNT] = {0};
/* FCLNX-GPL-FX-446 <<< */

int hfc_npiv_enable					= -1;	/* FCLNX-GPL-FX-137 */

int hfc_pm_pkt_num					= -1;
int hfcmp_pm_pkt_num[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_rsv_pkt_num					= -1;
int hfcmp_rsv_pkt_num[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_mq_enable					= -1;
int hfcmp_mq_enable[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_mq_num					= -1;
int hfcmp_mq_num[HFC_MAX_INSTANCE_CNT] = {0};

int hfc_cpu_map					= -1;

#if _HFC_ERROR_INJ								/* FCLNX-0246 */
int hfc_debug_ioerr			= 0;										/* FCLNX-0246 */
#endif																			/* FCLNX-0246 */

//static char *write_retries			= NULL;									/* FCLNX-GPL-0449 */

module_param(hfc_message_enable,int, S_IRUGO);

module_param(hfclddopts, charp, S_IRUGO);
MODULE_PARM_DESC(hfclddopts,
                "Additional driver options and persistent binding info.");
                
module_param(hfc_automap, int, S_IRUGO);
MODULE_PARM_DESC(hfc_automap,
                "Driver automap support: 0 means automap is disabled ; 1 means automap is enabled. ");

module_param(hfc_pxe_boot, int, S_IRUGO);
MODULE_PARM_DESC(hfc_pxe_boot,
		"Local PXE boot support: 0 means pxe boot mode is disabled, 1 means pxe boot mode is enabled.");

module_param(hfc_shadow, int, S_IRUGO);
MODULE_PARM_DESC(hfc_shadow,
		"Shadow LPAR: 0 means The LPAR is Guest LPAR, 1 means The LPAR is Shadow LPAR.");

module_param(hfc_narrowmap, int, S_IRUGO);									/* FCLNX-0392 */
MODULE_PARM_DESC(hfc_narrowmap,													/* FCLNX-0392 */
                "Driver narrowmap support: 0 means narrowmap is disabled ; 1 means narrowmap is enabled. ");

module_param(hfc_scsi_time_out, int, S_IRUGO);
MODULE_PARM_DESC(hfc_scsi_time_out,
		"scsi time_out support: 0 means error code is DID_ERROR, 1 means error code is DID_TIME_OUT.");

/* connection type */
module_param(hfc_connection_type, int, S_IRUGO);
MODULE_PARM_DESC(hfc_connection_type,
                "Driver connection type for all adapter : 0 to auto; 1 to Point to Point; 2 to FC-SW; 3 to FC-AL. ");

module_param(hfc0_connection_type, int, S_IRUGO);
module_param(hfc1_connection_type, int, S_IRUGO);
module_param(hfc2_connection_type, int, S_IRUGO);
module_param(hfc3_connection_type, int, S_IRUGO);
module_param(hfc4_connection_type, int, S_IRUGO);
module_param(hfc5_connection_type, int, S_IRUGO);
module_param(hfc6_connection_type, int, S_IRUGO);
module_param(hfc7_connection_type, int, S_IRUGO);
module_param(hfc8_connection_type, int, S_IRUGO);
module_param(hfc9_connection_type, int, S_IRUGO);
module_param(hfc10_connection_type, int, S_IRUGO);
module_param(hfc11_connection_type, int, S_IRUGO);
module_param(hfc12_connection_type, int, S_IRUGO);
module_param(hfc13_connection_type, int, S_IRUGO);
module_param(hfc14_connection_type, int, S_IRUGO);
module_param(hfc15_connection_type, int, S_IRUGO);
module_param(hfc16_connection_type, int, S_IRUGO);
module_param(hfc17_connection_type, int, S_IRUGO);
module_param(hfc18_connection_type, int, S_IRUGO);
module_param(hfc19_connection_type, int, S_IRUGO);
module_param(hfc20_connection_type, int, S_IRUGO);
module_param(hfc21_connection_type, int, S_IRUGO);
module_param(hfc22_connection_type, int, S_IRUGO);
module_param(hfc23_connection_type, int, S_IRUGO);
module_param(hfc24_connection_type, int, S_IRUGO);
module_param(hfc25_connection_type, int, S_IRUGO);
module_param(hfc26_connection_type, int, S_IRUGO);
module_param(hfc27_connection_type, int, S_IRUGO);
module_param(hfc28_connection_type, int, S_IRUGO);
module_param(hfc29_connection_type, int, S_IRUGO);
module_param(hfc30_connection_type, int, S_IRUGO);
module_param(hfc31_connection_type, int, S_IRUGO);

/* link speed  */
module_param(hfc_link_speed, int, S_IRUGO);
MODULE_PARM_DESC(hfc_link_speed,
                "Driver link speed for all adapter : 0 to auto; 1 to 1Gbps; 2 to 2Gbps. ");
module_param(hfc0_link_speed, int, S_IRUGO);
module_param(hfc1_link_speed, int, S_IRUGO);
module_param(hfc2_link_speed, int, S_IRUGO);
module_param(hfc3_link_speed, int, S_IRUGO);
module_param(hfc4_link_speed, int, S_IRUGO);
module_param(hfc5_link_speed, int, S_IRUGO);
module_param(hfc6_link_speed, int, S_IRUGO);
module_param(hfc7_link_speed, int, S_IRUGO);
module_param(hfc8_link_speed, int, S_IRUGO);
module_param(hfc9_link_speed, int, S_IRUGO);
module_param(hfc10_link_speed, int, S_IRUGO);
module_param(hfc11_link_speed, int, S_IRUGO);
module_param(hfc12_link_speed, int, S_IRUGO);
module_param(hfc13_link_speed, int, S_IRUGO);
module_param(hfc14_link_speed, int, S_IRUGO);
module_param(hfc15_link_speed, int, S_IRUGO);
module_param(hfc16_link_speed, int, S_IRUGO);
module_param(hfc17_link_speed, int, S_IRUGO);
module_param(hfc18_link_speed, int, S_IRUGO);
module_param(hfc19_link_speed, int, S_IRUGO);
module_param(hfc20_link_speed, int, S_IRUGO);
module_param(hfc21_link_speed, int, S_IRUGO);
module_param(hfc22_link_speed, int, S_IRUGO);
module_param(hfc23_link_speed, int, S_IRUGO);
module_param(hfc24_link_speed, int, S_IRUGO);
module_param(hfc25_link_speed, int, S_IRUGO);
module_param(hfc26_link_speed, int, S_IRUGO);
module_param(hfc27_link_speed, int, S_IRUGO);
module_param(hfc28_link_speed, int, S_IRUGO);
module_param(hfc29_link_speed, int, S_IRUGO);
module_param(hfc30_link_speed, int, S_IRUGO);
module_param(hfc31_link_speed, int, S_IRUGO);

/* max transfer  */
module_param(hfc_max_transfer, int, S_IRUGO);
MODULE_PARM_DESC(hfc_max_transfer,
                "Driver max transfer length for all adapter : 1/4/8/16 GB. ");
module_param(hfc0_max_transfer, int, S_IRUGO);
module_param(hfc1_max_transfer, int, S_IRUGO);
module_param(hfc2_max_transfer, int, S_IRUGO);
module_param(hfc3_max_transfer, int, S_IRUGO);
module_param(hfc4_max_transfer, int, S_IRUGO);
module_param(hfc5_max_transfer, int, S_IRUGO);
module_param(hfc6_max_transfer, int, S_IRUGO);
module_param(hfc7_max_transfer, int, S_IRUGO);
module_param(hfc8_max_transfer, int, S_IRUGO);
module_param(hfc9_max_transfer, int, S_IRUGO);
module_param(hfc10_max_transfer, int, S_IRUGO);
module_param(hfc11_max_transfer, int, S_IRUGO);
module_param(hfc12_max_transfer, int, S_IRUGO);
module_param(hfc13_max_transfer, int, S_IRUGO);
module_param(hfc14_max_transfer, int, S_IRUGO);
module_param(hfc15_max_transfer, int, S_IRUGO);
module_param(hfc16_max_transfer, int, S_IRUGO);
module_param(hfc17_max_transfer, int, S_IRUGO);
module_param(hfc18_max_transfer, int, S_IRUGO);
module_param(hfc19_max_transfer, int, S_IRUGO);
module_param(hfc20_max_transfer, int, S_IRUGO);
module_param(hfc21_max_transfer, int, S_IRUGO);
module_param(hfc22_max_transfer, int, S_IRUGO);
module_param(hfc23_max_transfer, int, S_IRUGO);
module_param(hfc24_max_transfer, int, S_IRUGO);
module_param(hfc25_max_transfer, int, S_IRUGO);
module_param(hfc26_max_transfer, int, S_IRUGO);
module_param(hfc27_max_transfer, int, S_IRUGO);
module_param(hfc28_max_transfer, int, S_IRUGO);
module_param(hfc29_max_transfer, int, S_IRUGO);
module_param(hfc30_max_transfer, int, S_IRUGO);
module_param(hfc31_max_transfer, int, S_IRUGO);

/* link down  */
module_param(hfc_link_down, int, S_IRUGO);
MODULE_PARM_DESC(hfc_link_down,
                "Driver link down timeout  for all adapter : 0 - 60s (default 15s)");
module_param(hfc0_link_down, int, S_IRUGO);
module_param(hfc1_link_down, int, S_IRUGO);
module_param(hfc2_link_down, int, S_IRUGO);
module_param(hfc3_link_down, int, S_IRUGO);
module_param(hfc4_link_down, int, S_IRUGO);
module_param(hfc5_link_down, int, S_IRUGO);
module_param(hfc6_link_down, int, S_IRUGO);
module_param(hfc7_link_down, int, S_IRUGO);
module_param(hfc8_link_down, int, S_IRUGO);
module_param(hfc9_link_down, int, S_IRUGO);
module_param(hfc10_link_down, int, S_IRUGO);
module_param(hfc11_link_down, int, S_IRUGO);
module_param(hfc12_link_down, int, S_IRUGO);
module_param(hfc13_link_down, int, S_IRUGO);
module_param(hfc14_link_down, int, S_IRUGO);
module_param(hfc15_link_down, int, S_IRUGO);
module_param(hfc16_link_down, int, S_IRUGO);
module_param(hfc17_link_down, int, S_IRUGO);
module_param(hfc18_link_down, int, S_IRUGO);
module_param(hfc19_link_down, int, S_IRUGO);
module_param(hfc20_link_down, int, S_IRUGO);
module_param(hfc21_link_down, int, S_IRUGO);
module_param(hfc22_link_down, int, S_IRUGO);
module_param(hfc23_link_down, int, S_IRUGO);
module_param(hfc24_link_down, int, S_IRUGO);
module_param(hfc25_link_down, int, S_IRUGO);
module_param(hfc26_link_down, int, S_IRUGO);
module_param(hfc27_link_down, int, S_IRUGO);
module_param(hfc28_link_down, int, S_IRUGO);
module_param(hfc29_link_down, int, S_IRUGO);
module_param(hfc30_link_down, int, S_IRUGO);
module_param(hfc31_link_down, int, S_IRUGO);

/* link down  2*/ /* FCLNX-0241*/
module_param(hfc_link_down2, int, S_IRUGO);
MODULE_PARM_DESC(hfc_link_down2,
                "Driver link down timeout2  for all adapter : 0 - 60s (default 15s)");
module_param(hfc0_link_down2, int, S_IRUGO);
module_param(hfc1_link_down2, int, S_IRUGO);
module_param(hfc2_link_down2, int, S_IRUGO);
module_param(hfc3_link_down2, int, S_IRUGO);
module_param(hfc4_link_down2, int, S_IRUGO);
module_param(hfc5_link_down2, int, S_IRUGO);
module_param(hfc6_link_down2, int, S_IRUGO);
module_param(hfc7_link_down2, int, S_IRUGO);
module_param(hfc8_link_down2, int, S_IRUGO);
module_param(hfc9_link_down2, int, S_IRUGO);
module_param(hfc10_link_down2, int, S_IRUGO);
module_param(hfc11_link_down2, int, S_IRUGO);
module_param(hfc12_link_down2, int, S_IRUGO);
module_param(hfc13_link_down2, int, S_IRUGO);
module_param(hfc14_link_down2, int, S_IRUGO);
module_param(hfc15_link_down2, int, S_IRUGO);
module_param(hfc16_link_down2, int, S_IRUGO);
module_param(hfc17_link_down2, int, S_IRUGO);
module_param(hfc18_link_down2, int, S_IRUGO);
module_param(hfc19_link_down2, int, S_IRUGO);
module_param(hfc20_link_down2, int, S_IRUGO);
module_param(hfc21_link_down2, int, S_IRUGO);
module_param(hfc22_link_down2, int, S_IRUGO);
module_param(hfc23_link_down2, int, S_IRUGO);
module_param(hfc24_link_down2, int, S_IRUGO);
module_param(hfc25_link_down2, int, S_IRUGO);
module_param(hfc26_link_down2, int, S_IRUGO);
module_param(hfc27_link_down2, int, S_IRUGO);
module_param(hfc28_link_down2, int, S_IRUGO);
module_param(hfc29_link_down2, int, S_IRUGO);
module_param(hfc30_link_down2, int, S_IRUGO);
module_param(hfc31_link_down2, int, S_IRUGO);
/* FCLNX-0241 */

/* reset delay  */
module_param(hfc_reset_delay, int, S_IRUGO);
MODULE_PARM_DESC(hfc_reset_delay,
                "Driver reset delay  for all adapter : 0 - 60s (default 10s)");
module_param(hfc0_reset_delay, int, S_IRUGO);
module_param(hfc1_reset_delay, int, S_IRUGO);
module_param(hfc2_reset_delay, int, S_IRUGO);
module_param(hfc3_reset_delay, int, S_IRUGO);
module_param(hfc4_reset_delay, int, S_IRUGO);
module_param(hfc5_reset_delay, int, S_IRUGO);
module_param(hfc6_reset_delay, int, S_IRUGO);
module_param(hfc7_reset_delay, int, S_IRUGO);
module_param(hfc8_reset_delay, int, S_IRUGO);
module_param(hfc9_reset_delay, int, S_IRUGO);
module_param(hfc10_reset_delay, int, S_IRUGO);
module_param(hfc11_reset_delay, int, S_IRUGO);
module_param(hfc12_reset_delay, int, S_IRUGO);
module_param(hfc13_reset_delay, int, S_IRUGO);
module_param(hfc14_reset_delay, int, S_IRUGO);
module_param(hfc15_reset_delay, int, S_IRUGO);
module_param(hfc16_reset_delay, int, S_IRUGO);
module_param(hfc17_reset_delay, int, S_IRUGO);
module_param(hfc18_reset_delay, int, S_IRUGO);
module_param(hfc19_reset_delay, int, S_IRUGO);
module_param(hfc20_reset_delay, int, S_IRUGO);
module_param(hfc21_reset_delay, int, S_IRUGO);
module_param(hfc22_reset_delay, int, S_IRUGO);
module_param(hfc23_reset_delay, int, S_IRUGO);
module_param(hfc24_reset_delay, int, S_IRUGO);
module_param(hfc25_reset_delay, int, S_IRUGO);
module_param(hfc26_reset_delay, int, S_IRUGO);
module_param(hfc27_reset_delay, int, S_IRUGO);
module_param(hfc28_reset_delay, int, S_IRUGO);
module_param(hfc29_reset_delay, int, S_IRUGO);
module_param(hfc30_reset_delay, int, S_IRUGO);
module_param(hfc31_reset_delay, int, S_IRUGO);

/* lun reset delay  */
module_param(hfc_lun_reset_delay, int, S_IRUGO);
MODULE_PARM_DESC(hfc_lun_reset_delay,
                "Driver lun reset delay  for all adapter : 0 - 60s (default 0s)");
module_param(hfc0_lun_reset_delay, int, S_IRUGO);
module_param(hfc1_lun_reset_delay, int, S_IRUGO);
module_param(hfc2_lun_reset_delay, int, S_IRUGO);
module_param(hfc3_lun_reset_delay, int, S_IRUGO);
module_param(hfc4_lun_reset_delay, int, S_IRUGO);
module_param(hfc5_lun_reset_delay, int, S_IRUGO);
module_param(hfc6_lun_reset_delay, int, S_IRUGO);
module_param(hfc7_lun_reset_delay, int, S_IRUGO);
module_param(hfc8_lun_reset_delay, int, S_IRUGO);
module_param(hfc9_lun_reset_delay, int, S_IRUGO);
module_param(hfc10_lun_reset_delay, int, S_IRUGO);
module_param(hfc11_lun_reset_delay, int, S_IRUGO);
module_param(hfc12_lun_reset_delay, int, S_IRUGO);
module_param(hfc13_lun_reset_delay, int, S_IRUGO);
module_param(hfc14_lun_reset_delay, int, S_IRUGO);
module_param(hfc15_lun_reset_delay, int, S_IRUGO);
module_param(hfc16_lun_reset_delay, int, S_IRUGO);
module_param(hfc17_lun_reset_delay, int, S_IRUGO);
module_param(hfc18_lun_reset_delay, int, S_IRUGO);
module_param(hfc19_lun_reset_delay, int, S_IRUGO);
module_param(hfc20_lun_reset_delay, int, S_IRUGO);
module_param(hfc21_lun_reset_delay, int, S_IRUGO);
module_param(hfc22_lun_reset_delay, int, S_IRUGO);
module_param(hfc23_lun_reset_delay, int, S_IRUGO);
module_param(hfc24_lun_reset_delay, int, S_IRUGO);
module_param(hfc25_lun_reset_delay, int, S_IRUGO);
module_param(hfc26_lun_reset_delay, int, S_IRUGO);
module_param(hfc27_lun_reset_delay, int, S_IRUGO);
module_param(hfc28_lun_reset_delay, int, S_IRUGO);
module_param(hfc29_lun_reset_delay, int, S_IRUGO);
module_param(hfc30_lun_reset_delay, int, S_IRUGO);
module_param(hfc31_lun_reset_delay, int, S_IRUGO);

/* MCK recovery  */
module_param(hfc_mck_retry, int, S_IRUGO);
MODULE_PARM_DESC(hfc_mck_retry,
                "Driver machine check recovery  for all adapter : 0 - 10 (default 8 times)");
module_param(hfc0_mck_retry, int, S_IRUGO);
module_param(hfc1_mck_retry, int, S_IRUGO);
module_param(hfc2_mck_retry, int, S_IRUGO);
module_param(hfc3_mck_retry, int, S_IRUGO);
module_param(hfc4_mck_retry, int, S_IRUGO);
module_param(hfc5_mck_retry, int, S_IRUGO);
module_param(hfc6_mck_retry, int, S_IRUGO);
module_param(hfc7_mck_retry, int, S_IRUGO);
module_param(hfc8_mck_retry, int, S_IRUGO);
module_param(hfc9_mck_retry, int, S_IRUGO);
module_param(hfc10_mck_retry, int, S_IRUGO);
module_param(hfc11_mck_retry, int, S_IRUGO);
module_param(hfc12_mck_retry, int, S_IRUGO);
module_param(hfc13_mck_retry, int, S_IRUGO);
module_param(hfc14_mck_retry, int, S_IRUGO);
module_param(hfc15_mck_retry, int, S_IRUGO);
module_param(hfc16_mck_retry, int, S_IRUGO);
module_param(hfc17_mck_retry, int, S_IRUGO);
module_param(hfc18_mck_retry, int, S_IRUGO);
module_param(hfc19_mck_retry, int, S_IRUGO);
module_param(hfc20_mck_retry, int, S_IRUGO);
module_param(hfc21_mck_retry, int, S_IRUGO);
module_param(hfc22_mck_retry, int, S_IRUGO);
module_param(hfc23_mck_retry, int, S_IRUGO);
module_param(hfc24_mck_retry, int, S_IRUGO);
module_param(hfc25_mck_retry, int, S_IRUGO);
module_param(hfc26_mck_retry, int, S_IRUGO);
module_param(hfc27_mck_retry, int, S_IRUGO);
module_param(hfc28_mck_retry, int, S_IRUGO);
module_param(hfc29_mck_retry, int, S_IRUGO);
module_param(hfc30_mck_retry, int, S_IRUGO);
module_param(hfc31_mck_retry, int, S_IRUGO);

/* Preferred AL-PA  */
module_param(hfc_preferred_alpa, int, S_IRUGO);
module_param(hfc0_preferred_alpa, int, S_IRUGO);
module_param(hfc1_preferred_alpa, int, S_IRUGO);
module_param(hfc2_preferred_alpa, int, S_IRUGO);
module_param(hfc3_preferred_alpa, int, S_IRUGO);
module_param(hfc4_preferred_alpa, int, S_IRUGO);
module_param(hfc5_preferred_alpa, int, S_IRUGO);
module_param(hfc6_preferred_alpa, int, S_IRUGO);
module_param(hfc7_preferred_alpa, int, S_IRUGO);
module_param(hfc8_preferred_alpa, int, S_IRUGO);
module_param(hfc9_preferred_alpa, int, S_IRUGO);
module_param(hfc10_preferred_alpa, int, S_IRUGO);
module_param(hfc11_preferred_alpa, int, S_IRUGO);
module_param(hfc12_preferred_alpa, int, S_IRUGO);
module_param(hfc13_preferred_alpa, int, S_IRUGO);
module_param(hfc14_preferred_alpa, int, S_IRUGO);
module_param(hfc15_preferred_alpa, int, S_IRUGO);
module_param(hfc16_preferred_alpa, int, S_IRUGO);
module_param(hfc17_preferred_alpa, int, S_IRUGO);
module_param(hfc18_preferred_alpa, int, S_IRUGO);
module_param(hfc19_preferred_alpa, int, S_IRUGO);
module_param(hfc20_preferred_alpa, int, S_IRUGO);
module_param(hfc21_preferred_alpa, int, S_IRUGO);
module_param(hfc22_preferred_alpa, int, S_IRUGO);
module_param(hfc23_preferred_alpa, int, S_IRUGO);
module_param(hfc24_preferred_alpa, int, S_IRUGO);
module_param(hfc25_preferred_alpa, int, S_IRUGO);
module_param(hfc26_preferred_alpa, int, S_IRUGO);
module_param(hfc27_preferred_alpa, int, S_IRUGO);
module_param(hfc28_preferred_alpa, int, S_IRUGO);
module_param(hfc29_preferred_alpa, int, S_IRUGO);
module_param(hfc30_preferred_alpa, int, S_IRUGO);
module_param(hfc31_preferred_alpa, int, S_IRUGO);

/* reset timeout  */
module_param(hfc_reset_timeout, int, S_IRUGO);
module_param(hfc0_reset_timeout, int, S_IRUGO);
module_param(hfc1_reset_timeout, int, S_IRUGO);
module_param(hfc2_reset_timeout, int, S_IRUGO);
module_param(hfc3_reset_timeout, int, S_IRUGO);
module_param(hfc4_reset_timeout, int, S_IRUGO);
module_param(hfc5_reset_timeout, int, S_IRUGO);
module_param(hfc6_reset_timeout, int, S_IRUGO);
module_param(hfc7_reset_timeout, int, S_IRUGO);
module_param(hfc8_reset_timeout, int, S_IRUGO);
module_param(hfc9_reset_timeout, int, S_IRUGO);
module_param(hfc10_reset_timeout, int, S_IRUGO);
module_param(hfc11_reset_timeout, int, S_IRUGO);
module_param(hfc12_reset_timeout, int, S_IRUGO);
module_param(hfc13_reset_timeout, int, S_IRUGO);
module_param(hfc14_reset_timeout, int, S_IRUGO);
module_param(hfc15_reset_timeout, int, S_IRUGO);
module_param(hfc16_reset_timeout, int, S_IRUGO);
module_param(hfc17_reset_timeout, int, S_IRUGO);
module_param(hfc18_reset_timeout, int, S_IRUGO);
module_param(hfc19_reset_timeout, int, S_IRUGO);
module_param(hfc20_reset_timeout, int, S_IRUGO);
module_param(hfc21_reset_timeout, int, S_IRUGO);
module_param(hfc22_reset_timeout, int, S_IRUGO);
module_param(hfc23_reset_timeout, int, S_IRUGO);
module_param(hfc24_reset_timeout, int, S_IRUGO);
module_param(hfc25_reset_timeout, int, S_IRUGO);
module_param(hfc26_reset_timeout, int, S_IRUGO);
module_param(hfc27_reset_timeout, int, S_IRUGO);
module_param(hfc28_reset_timeout, int, S_IRUGO);
module_param(hfc29_reset_timeout, int, S_IRUGO);
module_param(hfc30_reset_timeout, int, S_IRUGO);
module_param(hfc31_reset_timeout, int, S_IRUGO);

/* abort timeout  */
module_param(hfc_abort_timeout , int, S_IRUGO);
module_param(hfc0_abort_timeout , int, S_IRUGO);
module_param(hfc1_abort_timeout , int, S_IRUGO);
module_param(hfc2_abort_timeout , int, S_IRUGO);
module_param(hfc3_abort_timeout , int, S_IRUGO);
module_param(hfc4_abort_timeout , int, S_IRUGO);
module_param(hfc5_abort_timeout , int, S_IRUGO);
module_param(hfc6_abort_timeout , int, S_IRUGO);
module_param(hfc7_abort_timeout , int, S_IRUGO);
module_param(hfc8_abort_timeout , int, S_IRUGO);
module_param(hfc9_abort_timeout , int, S_IRUGO);
module_param(hfc10_abort_timeout , int, S_IRUGO);
module_param(hfc11_abort_timeout , int, S_IRUGO);
module_param(hfc12_abort_timeout , int, S_IRUGO);
module_param(hfc13_abort_timeout , int, S_IRUGO);
module_param(hfc14_abort_timeout , int, S_IRUGO);
module_param(hfc15_abort_timeout , int, S_IRUGO);
module_param(hfc16_abort_timeout, int, S_IRUGO);
module_param(hfc17_abort_timeout, int, S_IRUGO);
module_param(hfc18_abort_timeout, int, S_IRUGO);
module_param(hfc19_abort_timeout, int, S_IRUGO);
module_param(hfc20_abort_timeout, int, S_IRUGO);
module_param(hfc21_abort_timeout, int, S_IRUGO);
module_param(hfc22_abort_timeout, int, S_IRUGO);
module_param(hfc23_abort_timeout, int, S_IRUGO);
module_param(hfc24_abort_timeout, int, S_IRUGO);
module_param(hfc25_abort_timeout, int, S_IRUGO);
module_param(hfc26_abort_timeout, int, S_IRUGO);
module_param(hfc27_abort_timeout, int, S_IRUGO);
module_param(hfc28_abort_timeout, int, S_IRUGO);
module_param(hfc29_abort_timeout, int, S_IRUGO);
module_param(hfc30_abort_timeout, int, S_IRUGO);
module_param(hfc31_abort_timeout, int, S_IRUGO);

/* enable target reset  */
module_param(hfc_enable_tgtrst, int, S_IRUGO);
MODULE_PARM_DESC(hfc_enable_tgtrst,
                "Driver target reset process is enabled for all adapters : 0 or 1 (default 0)");
module_param(hfc0_enable_tgtrst, int, S_IRUGO);
module_param(hfc1_enable_tgtrst, int, S_IRUGO);
module_param(hfc2_enable_tgtrst, int, S_IRUGO);
module_param(hfc3_enable_tgtrst, int, S_IRUGO);
module_param(hfc4_enable_tgtrst, int, S_IRUGO);
module_param(hfc5_enable_tgtrst, int, S_IRUGO);
module_param(hfc6_enable_tgtrst, int, S_IRUGO);
module_param(hfc7_enable_tgtrst, int, S_IRUGO);
module_param(hfc8_enable_tgtrst, int, S_IRUGO);
module_param(hfc9_enable_tgtrst, int, S_IRUGO);
module_param(hfc10_enable_tgtrst, int, S_IRUGO);
module_param(hfc11_enable_tgtrst, int, S_IRUGO);
module_param(hfc12_enable_tgtrst, int, S_IRUGO);
module_param(hfc13_enable_tgtrst, int, S_IRUGO);
module_param(hfc14_enable_tgtrst, int, S_IRUGO);
module_param(hfc15_enable_tgtrst, int, S_IRUGO);
module_param(hfc16_enable_tgtrst, int, S_IRUGO);
module_param(hfc17_enable_tgtrst, int, S_IRUGO);
module_param(hfc18_enable_tgtrst, int, S_IRUGO);
module_param(hfc19_enable_tgtrst, int, S_IRUGO);
module_param(hfc20_enable_tgtrst, int, S_IRUGO);
module_param(hfc21_enable_tgtrst, int, S_IRUGO);
module_param(hfc22_enable_tgtrst, int, S_IRUGO);
module_param(hfc23_enable_tgtrst, int, S_IRUGO);
module_param(hfc24_enable_tgtrst, int, S_IRUGO);
module_param(hfc25_enable_tgtrst, int, S_IRUGO);
module_param(hfc26_enable_tgtrst, int, S_IRUGO);
module_param(hfc27_enable_tgtrst, int, S_IRUGO);
module_param(hfc28_enable_tgtrst, int, S_IRUGO);
module_param(hfc29_enable_tgtrst, int, S_IRUGO);
module_param(hfc30_enable_tgtrst, int, S_IRUGO);
module_param(hfc31_enable_tgtrst, int, S_IRUGO);


/* queue depth  */
module_param(hfc_queue_depth, int, S_IRUGO);
MODULE_PARM_DESC(hfc_queue_depth,
                "Driver queue depth for all adapters :  0 - 32 (default 32)");
module_param(hfc0_queue_depth, int, S_IRUGO);
module_param(hfc1_queue_depth, int, S_IRUGO);
module_param(hfc2_queue_depth, int, S_IRUGO);
module_param(hfc3_queue_depth, int, S_IRUGO);
module_param(hfc4_queue_depth, int, S_IRUGO);
module_param(hfc5_queue_depth, int, S_IRUGO);
module_param(hfc6_queue_depth, int, S_IRUGO);
module_param(hfc7_queue_depth, int, S_IRUGO);
module_param(hfc8_queue_depth, int, S_IRUGO);
module_param(hfc9_queue_depth, int, S_IRUGO);
module_param(hfc10_queue_depth, int, S_IRUGO);
module_param(hfc11_queue_depth, int, S_IRUGO);
module_param(hfc12_queue_depth, int, S_IRUGO);
module_param(hfc13_queue_depth, int, S_IRUGO);
module_param(hfc14_queue_depth, int, S_IRUGO);
module_param(hfc15_queue_depth, int, S_IRUGO);
module_param(hfc16_queue_depth, int, S_IRUGO);
module_param(hfc17_queue_depth, int, S_IRUGO);
module_param(hfc18_queue_depth, int, S_IRUGO);
module_param(hfc19_queue_depth, int, S_IRUGO);
module_param(hfc20_queue_depth, int, S_IRUGO);
module_param(hfc21_queue_depth, int, S_IRUGO);
module_param(hfc22_queue_depth, int, S_IRUGO);
module_param(hfc23_queue_depth, int, S_IRUGO);
module_param(hfc24_queue_depth, int, S_IRUGO);
module_param(hfc25_queue_depth, int, S_IRUGO);
module_param(hfc26_queue_depth, int, S_IRUGO);
module_param(hfc27_queue_depth, int, S_IRUGO);
module_param(hfc28_queue_depth, int, S_IRUGO);
module_param(hfc29_queue_depth, int, S_IRUGO);
module_param(hfc30_queue_depth, int, S_IRUGO);
module_param(hfc31_queue_depth, int, S_IRUGO);


/* sef info trace  */
module_param(hfc_seg_trace, int, S_IRUGO);
MODULE_PARM_DESC(hfc_seg_trace,
                "Driver seg info trace switch for all adapters :  0 or 1 (default 0)");
module_param(hfc0_seg_trace, int, S_IRUGO);
module_param(hfc1_seg_trace, int, S_IRUGO);
module_param(hfc2_seg_trace, int, S_IRUGO);
module_param(hfc3_seg_trace, int, S_IRUGO);
module_param(hfc4_seg_trace, int, S_IRUGO);
module_param(hfc5_seg_trace, int, S_IRUGO);
module_param(hfc6_seg_trace, int, S_IRUGO);
module_param(hfc7_seg_trace, int, S_IRUGO);
module_param(hfc8_seg_trace, int, S_IRUGO);
module_param(hfc9_seg_trace, int, S_IRUGO);
module_param(hfc10_seg_trace, int, S_IRUGO);
module_param(hfc11_seg_trace, int, S_IRUGO);
module_param(hfc12_seg_trace, int, S_IRUGO);
module_param(hfc13_seg_trace, int, S_IRUGO);
module_param(hfc14_seg_trace, int, S_IRUGO);
module_param(hfc15_seg_trace, int, S_IRUGO);
module_param(hfc16_seg_trace, int, S_IRUGO);
module_param(hfc17_seg_trace, int, S_IRUGO);
module_param(hfc18_seg_trace, int, S_IRUGO);
module_param(hfc19_seg_trace, int, S_IRUGO);
module_param(hfc20_seg_trace, int, S_IRUGO);
module_param(hfc21_seg_trace, int, S_IRUGO);
module_param(hfc22_seg_trace, int, S_IRUGO);
module_param(hfc23_seg_trace, int, S_IRUGO);
module_param(hfc24_seg_trace, int, S_IRUGO);
module_param(hfc25_seg_trace, int, S_IRUGO);
module_param(hfc26_seg_trace, int, S_IRUGO);
module_param(hfc27_seg_trace, int, S_IRUGO);
module_param(hfc28_seg_trace, int, S_IRUGO);
module_param(hfc29_seg_trace, int, S_IRUGO);
module_param(hfc30_seg_trace, int, S_IRUGO);
module_param(hfc31_seg_trace, int, S_IRUGO);

/* max_target */
module_param(hfc_max_target, int, S_IRUGO);
module_param(hfc0_max_target, int, S_IRUGO);
module_param(hfc1_max_target, int, S_IRUGO);
module_param(hfc2_max_target, int, S_IRUGO);
module_param(hfc3_max_target, int, S_IRUGO);
module_param(hfc4_max_target, int, S_IRUGO);
module_param(hfc5_max_target, int, S_IRUGO);
module_param(hfc6_max_target, int, S_IRUGO);
module_param(hfc7_max_target, int, S_IRUGO);
module_param(hfc8_max_target, int, S_IRUGO);
module_param(hfc9_max_target, int, S_IRUGO);
module_param(hfc10_max_target, int, S_IRUGO);
module_param(hfc11_max_target, int, S_IRUGO);
module_param(hfc12_max_target, int, S_IRUGO);
module_param(hfc13_max_target, int, S_IRUGO);
module_param(hfc14_max_target, int, S_IRUGO);
module_param(hfc15_max_target, int, S_IRUGO);
module_param(hfc16_max_target, int, S_IRUGO);
module_param(hfc17_max_target, int, S_IRUGO);
module_param(hfc18_max_target, int, S_IRUGO);
module_param(hfc19_max_target, int, S_IRUGO);
module_param(hfc20_max_target, int, S_IRUGO);
module_param(hfc21_max_target, int, S_IRUGO);
module_param(hfc22_max_target, int, S_IRUGO);
module_param(hfc23_max_target, int, S_IRUGO);
module_param(hfc24_max_target, int, S_IRUGO);
module_param(hfc25_max_target, int, S_IRUGO);
module_param(hfc26_max_target, int, S_IRUGO);
module_param(hfc27_max_target, int, S_IRUGO);
module_param(hfc28_max_target, int, S_IRUGO);
module_param(hfc29_max_target, int, S_IRUGO);
module_param(hfc30_max_target, int, S_IRUGO);
module_param(hfc31_max_target, int, S_IRUGO);

/* max xob  */
module_param(hfc_xob_max, int, S_IRUGO);
MODULE_PARM_DESC(hfc_max_target,
                "Driver max xob number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_xob_max, int, S_IRUGO);
module_param(hfc1_xob_max, int, S_IRUGO);
module_param(hfc2_xob_max, int, S_IRUGO);
module_param(hfc3_xob_max, int, S_IRUGO);
module_param(hfc4_xob_max, int, S_IRUGO);
module_param(hfc5_xob_max, int, S_IRUGO);
module_param(hfc6_xob_max, int, S_IRUGO);
module_param(hfc7_xob_max, int, S_IRUGO);
module_param(hfc8_xob_max, int, S_IRUGO);
module_param(hfc9_xob_max, int, S_IRUGO);
module_param(hfc10_xob_max, int, S_IRUGO);
module_param(hfc11_xob_max, int, S_IRUGO);
module_param(hfc12_xob_max, int, S_IRUGO);
module_param(hfc13_xob_max, int, S_IRUGO);
module_param(hfc14_xob_max, int, S_IRUGO);
module_param(hfc15_xob_max, int, S_IRUGO);
module_param(hfc16_xob_max, int, S_IRUGO);
module_param(hfc17_xob_max, int, S_IRUGO);
module_param(hfc18_xob_max, int, S_IRUGO);
module_param(hfc19_xob_max, int, S_IRUGO);
module_param(hfc20_xob_max, int, S_IRUGO);
module_param(hfc21_xob_max, int, S_IRUGO);
module_param(hfc22_xob_max, int, S_IRUGO);
module_param(hfc23_xob_max, int, S_IRUGO);
module_param(hfc24_xob_max, int, S_IRUGO);
module_param(hfc25_xob_max, int, S_IRUGO);
module_param(hfc26_xob_max, int, S_IRUGO);
module_param(hfc27_xob_max, int, S_IRUGO);
module_param(hfc28_xob_max, int, S_IRUGO);
module_param(hfc29_xob_max, int, S_IRUGO);
module_param(hfc30_xob_max, int, S_IRUGO);
module_param(hfc31_xob_max, int, S_IRUGO);

/* xrb max  */
module_param(hfc_xrb_max, int, S_IRUGO);
MODULE_PARM_DESC(hfc_xrb_max,
                "Driver max xrb number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_xrb_max, int, S_IRUGO);
module_param(hfc1_xrb_max, int, S_IRUGO);
module_param(hfc2_xrb_max, int, S_IRUGO);
module_param(hfc3_xrb_max, int, S_IRUGO);
module_param(hfc4_xrb_max, int, S_IRUGO);
module_param(hfc5_xrb_max, int, S_IRUGO);
module_param(hfc6_xrb_max, int, S_IRUGO);
module_param(hfc7_xrb_max, int, S_IRUGO);
module_param(hfc8_xrb_max, int, S_IRUGO);
module_param(hfc9_xrb_max, int, S_IRUGO);
module_param(hfc10_xrb_max, int, S_IRUGO);
module_param(hfc11_xrb_max, int, S_IRUGO);
module_param(hfc12_xrb_max, int, S_IRUGO);
module_param(hfc13_xrb_max, int, S_IRUGO);
module_param(hfc14_xrb_max, int, S_IRUGO);
module_param(hfc15_xrb_max, int, S_IRUGO);
module_param(hfc16_xrb_max, int, S_IRUGO);
module_param(hfc17_xrb_max, int, S_IRUGO);
module_param(hfc18_xrb_max, int, S_IRUGO);
module_param(hfc19_xrb_max, int, S_IRUGO);
module_param(hfc20_xrb_max, int, S_IRUGO);
module_param(hfc21_xrb_max, int, S_IRUGO);
module_param(hfc22_xrb_max, int, S_IRUGO);
module_param(hfc23_xrb_max, int, S_IRUGO);
module_param(hfc24_xrb_max, int, S_IRUGO);
module_param(hfc25_xrb_max, int, S_IRUGO);
module_param(hfc26_xrb_max, int, S_IRUGO);
module_param(hfc27_xrb_max, int, S_IRUGO);
module_param(hfc28_xrb_max, int, S_IRUGO);
module_param(hfc29_xrb_max, int, S_IRUGO);
module_param(hfc30_xrb_max, int, S_IRUGO);
module_param(hfc31_xrb_max, int, S_IRUGO);

/* set slog_max  */
module_param(hfc_slog_max, int, S_IRUGO);
MODULE_PARM_DESC(hfc_slog_max,
                "Driver max soft log number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_slog_max, int, S_IRUGO);
module_param(hfc1_slog_max, int, S_IRUGO);
module_param(hfc2_slog_max, int, S_IRUGO);
module_param(hfc3_slog_max, int, S_IRUGO);
module_param(hfc4_slog_max, int, S_IRUGO);
module_param(hfc5_slog_max, int, S_IRUGO);
module_param(hfc6_slog_max, int, S_IRUGO);
module_param(hfc7_slog_max, int, S_IRUGO);
module_param(hfc8_slog_max, int, S_IRUGO);
module_param(hfc9_slog_max, int, S_IRUGO);
module_param(hfc10_slog_max, int, S_IRUGO);
module_param(hfc11_slog_max, int, S_IRUGO);
module_param(hfc12_slog_max, int, S_IRUGO);
module_param(hfc13_slog_max, int, S_IRUGO);
module_param(hfc14_slog_max, int, S_IRUGO);
module_param(hfc15_slog_max, int, S_IRUGO);
module_param(hfc16_slog_max, int, S_IRUGO);
module_param(hfc17_slog_max, int, S_IRUGO);
module_param(hfc18_slog_max, int, S_IRUGO);
module_param(hfc19_slog_max, int, S_IRUGO);
module_param(hfc20_slog_max, int, S_IRUGO);
module_param(hfc21_slog_max, int, S_IRUGO);
module_param(hfc22_slog_max, int, S_IRUGO);
module_param(hfc23_slog_max, int, S_IRUGO);
module_param(hfc24_slog_max, int, S_IRUGO);
module_param(hfc25_slog_max, int, S_IRUGO);
module_param(hfc26_slog_max, int, S_IRUGO);
module_param(hfc27_slog_max, int, S_IRUGO);
module_param(hfc28_slog_max, int, S_IRUGO);
module_param(hfc29_slog_max, int, S_IRUGO);
module_param(hfc30_slog_max, int, S_IRUGO);
module_param(hfc31_slog_max, int, S_IRUGO);

/* set trace number  */
module_param(hfc_trc_max, int, S_IRUGO);
MODULE_PARM_DESC(hfc_trc_max,
                "Driver trace area number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_trc_max, int, S_IRUGO);
module_param(hfc1_trc_max, int, S_IRUGO);
module_param(hfc2_trc_max, int, S_IRUGO);
module_param(hfc3_trc_max, int, S_IRUGO);
module_param(hfc4_trc_max, int, S_IRUGO);
module_param(hfc5_trc_max, int, S_IRUGO);
module_param(hfc6_trc_max, int, S_IRUGO);
module_param(hfc7_trc_max, int, S_IRUGO);
module_param(hfc8_trc_max, int, S_IRUGO);
module_param(hfc9_trc_max, int, S_IRUGO);
module_param(hfc10_trc_max, int, S_IRUGO);
module_param(hfc11_trc_max, int, S_IRUGO);
module_param(hfc12_trc_max, int, S_IRUGO);
module_param(hfc13_trc_max, int, S_IRUGO);
module_param(hfc14_trc_max, int, S_IRUGO);
module_param(hfc15_trc_max, int, S_IRUGO);
module_param(hfc16_trc_max, int, S_IRUGO);
module_param(hfc17_trc_max, int, S_IRUGO);
module_param(hfc18_trc_max, int, S_IRUGO);
module_param(hfc19_trc_max, int, S_IRUGO);
module_param(hfc20_trc_max, int, S_IRUGO);
module_param(hfc21_trc_max, int, S_IRUGO);
module_param(hfc22_trc_max, int, S_IRUGO);
module_param(hfc23_trc_max, int, S_IRUGO);
module_param(hfc24_trc_max, int, S_IRUGO);
module_param(hfc25_trc_max, int, S_IRUGO);
module_param(hfc26_trc_max, int, S_IRUGO);
module_param(hfc27_trc_max, int, S_IRUGO);
module_param(hfc28_trc_max, int, S_IRUGO);
module_param(hfc29_trc_max, int, S_IRUGO);
module_param(hfc30_trc_max, int, S_IRUGO);
module_param(hfc31_trc_max, int, S_IRUGO);

/* set pkt num  */
module_param(hfc_pkt_num, int, S_IRUGO);
MODULE_PARM_DESC(hfc_pkt_num,
                "Driver hfc_pkt number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_pkt_num, int, S_IRUGO);
module_param(hfc1_pkt_num, int, S_IRUGO);
module_param(hfc2_pkt_num, int, S_IRUGO);
module_param(hfc3_pkt_num, int, S_IRUGO);
module_param(hfc4_pkt_num, int, S_IRUGO);
module_param(hfc5_pkt_num, int, S_IRUGO);
module_param(hfc6_pkt_num, int, S_IRUGO);
module_param(hfc7_pkt_num, int, S_IRUGO);
module_param(hfc8_pkt_num, int, S_IRUGO);
module_param(hfc9_pkt_num, int, S_IRUGO);
module_param(hfc10_pkt_num, int, S_IRUGO);
module_param(hfc11_pkt_num, int, S_IRUGO);
module_param(hfc12_pkt_num, int, S_IRUGO);
module_param(hfc13_pkt_num, int, S_IRUGO);
module_param(hfc14_pkt_num, int, S_IRUGO);
module_param(hfc15_pkt_num, int, S_IRUGO);
module_param(hfc16_pkt_num, int, S_IRUGO);
module_param(hfc17_pkt_num, int, S_IRUGO);
module_param(hfc18_pkt_num, int, S_IRUGO);
module_param(hfc19_pkt_num, int, S_IRUGO);
module_param(hfc20_pkt_num, int, S_IRUGO);
module_param(hfc21_pkt_num, int, S_IRUGO);
module_param(hfc22_pkt_num, int, S_IRUGO);
module_param(hfc23_pkt_num, int, S_IRUGO);
module_param(hfc24_pkt_num, int, S_IRUGO);
module_param(hfc25_pkt_num, int, S_IRUGO);
module_param(hfc26_pkt_num, int, S_IRUGO);
module_param(hfc27_pkt_num, int, S_IRUGO);
module_param(hfc28_pkt_num, int, S_IRUGO);
module_param(hfc29_pkt_num, int, S_IRUGO);
module_param(hfc30_pkt_num, int, S_IRUGO);
module_param(hfc31_pkt_num, int, S_IRUGO);

/* can_queue  */
module_param(hfc_can_queue, int, S_IRUGO);
MODULE_PARM_DESC(hfc_can_queue,
                "Driver can_queue number for all adapters :  0 or 1 (default 0)");
module_param(hfc0_can_queue, int, S_IRUGO);
module_param(hfc1_can_queue, int, S_IRUGO);
module_param(hfc2_can_queue, int, S_IRUGO);
module_param(hfc3_can_queue, int, S_IRUGO);
module_param(hfc4_can_queue, int, S_IRUGO);
module_param(hfc5_can_queue, int, S_IRUGO);
module_param(hfc6_can_queue, int, S_IRUGO);
module_param(hfc7_can_queue, int, S_IRUGO);
module_param(hfc8_can_queue, int, S_IRUGO);
module_param(hfc9_can_queue, int, S_IRUGO);
module_param(hfc10_can_queue, int, S_IRUGO);
module_param(hfc11_can_queue, int, S_IRUGO);
module_param(hfc12_can_queue, int, S_IRUGO);
module_param(hfc13_can_queue, int, S_IRUGO);
module_param(hfc14_can_queue, int, S_IRUGO);
module_param(hfc15_can_queue, int, S_IRUGO);
module_param(hfc16_can_queue, int, S_IRUGO);
module_param(hfc17_can_queue, int, S_IRUGO);
module_param(hfc18_can_queue, int, S_IRUGO);
module_param(hfc19_can_queue, int, S_IRUGO);
module_param(hfc20_can_queue, int, S_IRUGO);
module_param(hfc21_can_queue, int, S_IRUGO);
module_param(hfc22_can_queue, int, S_IRUGO);
module_param(hfc23_can_queue, int, S_IRUGO);
module_param(hfc24_can_queue, int, S_IRUGO);
module_param(hfc25_can_queue, int, S_IRUGO);
module_param(hfc26_can_queue, int, S_IRUGO);
module_param(hfc27_can_queue, int, S_IRUGO);
module_param(hfc28_can_queue, int, S_IRUGO);
module_param(hfc29_can_queue, int, S_IRUGO);
module_param(hfc30_can_queue, int, S_IRUGO);
module_param(hfc31_can_queue, int, S_IRUGO);

/* sg_tblsize  */
module_param(hfc_sg_tblsize, int, S_IRUGO);
MODULE_PARM_DESC(hfc_sg_tblsize,
                "Driver sg table size for all adapters :  0 or 1 (default 0)");
module_param(hfc0_sg_tblsize, int, S_IRUGO);
module_param(hfc1_sg_tblsize, int, S_IRUGO);
module_param(hfc2_sg_tblsize, int, S_IRUGO);
module_param(hfc3_sg_tblsize, int, S_IRUGO);
module_param(hfc4_sg_tblsize, int, S_IRUGO);
module_param(hfc5_sg_tblsize, int, S_IRUGO);
module_param(hfc6_sg_tblsize, int, S_IRUGO);
module_param(hfc7_sg_tblsize, int, S_IRUGO);
module_param(hfc8_sg_tblsize, int, S_IRUGO);
module_param(hfc9_sg_tblsize, int, S_IRUGO);
module_param(hfc10_sg_tblsize, int, S_IRUGO);
module_param(hfc11_sg_tblsize, int, S_IRUGO);
module_param(hfc12_sg_tblsize, int, S_IRUGO);
module_param(hfc13_sg_tblsize, int, S_IRUGO);
module_param(hfc14_sg_tblsize, int, S_IRUGO);
module_param(hfc15_sg_tblsize, int, S_IRUGO);
module_param(hfc16_sg_tblsize, int, S_IRUGO);
module_param(hfc17_sg_tblsize, int, S_IRUGO);
module_param(hfc18_sg_tblsize, int, S_IRUGO);
module_param(hfc19_sg_tblsize, int, S_IRUGO);
module_param(hfc20_sg_tblsize, int, S_IRUGO);
module_param(hfc21_sg_tblsize, int, S_IRUGO);
module_param(hfc22_sg_tblsize, int, S_IRUGO);
module_param(hfc23_sg_tblsize, int, S_IRUGO);
module_param(hfc24_sg_tblsize, int, S_IRUGO);
module_param(hfc25_sg_tblsize, int, S_IRUGO);
module_param(hfc26_sg_tblsize, int, S_IRUGO);
module_param(hfc27_sg_tblsize, int, S_IRUGO);
module_param(hfc28_sg_tblsize, int, S_IRUGO);
module_param(hfc29_sg_tblsize, int, S_IRUGO);
module_param(hfc30_sg_tblsize, int, S_IRUGO);
module_param(hfc31_sg_tblsize, int, S_IRUGO);

/* set cmnd_num  */
module_param(hfc_cmnd_num, int, S_IRUGO);
MODULE_PARM_DESC(hfc_cmnd_num,
                "Driver cmnd num for all adapters :  0 or 1 (default 0)");
module_param(hfc0_cmnd_num, int, S_IRUGO);
module_param(hfc1_cmnd_num, int, S_IRUGO);
module_param(hfc2_cmnd_num, int, S_IRUGO);
module_param(hfc3_cmnd_num, int, S_IRUGO);
module_param(hfc4_cmnd_num, int, S_IRUGO);
module_param(hfc5_cmnd_num, int, S_IRUGO);
module_param(hfc6_cmnd_num, int, S_IRUGO);
module_param(hfc7_cmnd_num, int, S_IRUGO);
module_param(hfc8_cmnd_num, int, S_IRUGO);
module_param(hfc9_cmnd_num, int, S_IRUGO);
module_param(hfc10_cmnd_num, int, S_IRUGO);
module_param(hfc11_cmnd_num, int, S_IRUGO);
module_param(hfc12_cmnd_num, int, S_IRUGO);
module_param(hfc13_cmnd_num, int, S_IRUGO);
module_param(hfc14_cmnd_num, int, S_IRUGO);
module_param(hfc15_cmnd_num, int, S_IRUGO);
module_param(hfc16_cmnd_num, int, S_IRUGO);
module_param(hfc17_cmnd_num, int, S_IRUGO);
module_param(hfc18_cmnd_num, int, S_IRUGO);
module_param(hfc19_cmnd_num, int, S_IRUGO);
module_param(hfc20_cmnd_num, int, S_IRUGO);
module_param(hfc21_cmnd_num, int, S_IRUGO);
module_param(hfc22_cmnd_num, int, S_IRUGO);
module_param(hfc23_cmnd_num, int, S_IRUGO);
module_param(hfc24_cmnd_num, int, S_IRUGO);
module_param(hfc25_cmnd_num, int, S_IRUGO);
module_param(hfc26_cmnd_num, int, S_IRUGO);
module_param(hfc27_cmnd_num, int, S_IRUGO);
module_param(hfc28_cmnd_num, int, S_IRUGO);
module_param(hfc29_cmnd_num, int, S_IRUGO);
module_param(hfc30_cmnd_num, int, S_IRUGO);
module_param(hfc31_cmnd_num, int, S_IRUGO);

/* set minus_timeout  */
module_param(hfc_minus_tout, int, S_IRUGO);
MODULE_PARM_DESC(hfc_minus_tout,
                "Driver minus timeout for all adapters :  0 or 1 (default 0)");
module_param(hfc0_minus_tout, int, S_IRUGO);
module_param(hfc1_minus_tout, int, S_IRUGO);
module_param(hfc2_minus_tout, int, S_IRUGO);
module_param(hfc3_minus_tout, int, S_IRUGO);
module_param(hfc4_minus_tout, int, S_IRUGO);
module_param(hfc5_minus_tout, int, S_IRUGO);
module_param(hfc6_minus_tout, int, S_IRUGO);
module_param(hfc7_minus_tout, int, S_IRUGO);
module_param(hfc8_minus_tout, int, S_IRUGO);
module_param(hfc9_minus_tout, int, S_IRUGO);
module_param(hfc10_minus_tout, int, S_IRUGO);
module_param(hfc11_minus_tout, int, S_IRUGO);
module_param(hfc12_minus_tout, int, S_IRUGO);
module_param(hfc13_minus_tout, int, S_IRUGO);
module_param(hfc14_minus_tout, int, S_IRUGO);
module_param(hfc15_minus_tout, int, S_IRUGO);
module_param(hfc16_minus_tout, int, S_IRUGO);
module_param(hfc17_minus_tout, int, S_IRUGO);
module_param(hfc18_minus_tout, int, S_IRUGO);
module_param(hfc19_minus_tout, int, S_IRUGO);
module_param(hfc20_minus_tout, int, S_IRUGO);
module_param(hfc21_minus_tout, int, S_IRUGO);
module_param(hfc22_minus_tout, int, S_IRUGO);
module_param(hfc23_minus_tout, int, S_IRUGO);
module_param(hfc24_minus_tout, int, S_IRUGO);
module_param(hfc25_minus_tout, int, S_IRUGO);
module_param(hfc26_minus_tout, int, S_IRUGO);
module_param(hfc27_minus_tout, int, S_IRUGO);
module_param(hfc28_minus_tout, int, S_IRUGO);
module_param(hfc29_minus_tout, int, S_IRUGO);
module_param(hfc30_minus_tout, int, S_IRUGO);
module_param(hfc31_minus_tout, int, S_IRUGO);

/* set scsi_allowed  */
module_param(hfc_scsi_allowed, int, S_IRUGO);
MODULE_PARM_DESC(hfc_scsi_allowed,
                "Driver scsi allowed(retry)for all adapters :  0 or 1 (default 0)");
module_param(hfc0_scsi_allowed, int, S_IRUGO);
module_param(hfc1_scsi_allowed, int, S_IRUGO);
module_param(hfc2_scsi_allowed, int, S_IRUGO);
module_param(hfc3_scsi_allowed, int, S_IRUGO);
module_param(hfc4_scsi_allowed, int, S_IRUGO);
module_param(hfc5_scsi_allowed, int, S_IRUGO);
module_param(hfc6_scsi_allowed, int, S_IRUGO);
module_param(hfc7_scsi_allowed, int, S_IRUGO);
module_param(hfc8_scsi_allowed, int, S_IRUGO);
module_param(hfc9_scsi_allowed, int, S_IRUGO);
module_param(hfc10_scsi_allowed, int, S_IRUGO);
module_param(hfc11_scsi_allowed, int, S_IRUGO);
module_param(hfc12_scsi_allowed, int, S_IRUGO);
module_param(hfc13_scsi_allowed, int, S_IRUGO);
module_param(hfc14_scsi_allowed, int, S_IRUGO);
module_param(hfc15_scsi_allowed, int, S_IRUGO);
module_param(hfc16_scsi_allowed, int, S_IRUGO);
module_param(hfc17_scsi_allowed, int, S_IRUGO);
module_param(hfc18_scsi_allowed, int, S_IRUGO);
module_param(hfc19_scsi_allowed, int, S_IRUGO);
module_param(hfc20_scsi_allowed, int, S_IRUGO);
module_param(hfc21_scsi_allowed, int, S_IRUGO);
module_param(hfc22_scsi_allowed, int, S_IRUGO);
module_param(hfc23_scsi_allowed, int, S_IRUGO);
module_param(hfc24_scsi_allowed, int, S_IRUGO);
module_param(hfc25_scsi_allowed, int, S_IRUGO);
module_param(hfc26_scsi_allowed, int, S_IRUGO);
module_param(hfc27_scsi_allowed, int, S_IRUGO);
module_param(hfc28_scsi_allowed, int, S_IRUGO);
module_param(hfc29_scsi_allowed, int, S_IRUGO);
module_param(hfc30_scsi_allowed, int, S_IRUGO);
module_param(hfc31_scsi_allowed, int, S_IRUGO);

/* mailbox login retry  *//* FCLNX-GPL-0343 */
module_param(hfc_login_retry, int, S_IRUGO);
MODULE_PARM_DESC(hfc_login_retry,
                "Driver mailbox login retry for all adapter : 0 - 10 (default 3 times)");
module_param(hfc0_login_retry, int, S_IRUGO);
module_param(hfc1_login_retry, int, S_IRUGO);
module_param(hfc2_login_retry, int, S_IRUGO);
module_param(hfc3_login_retry, int, S_IRUGO);
module_param(hfc4_login_retry, int, S_IRUGO);
module_param(hfc5_login_retry, int, S_IRUGO);
module_param(hfc6_login_retry, int, S_IRUGO);
module_param(hfc7_login_retry, int, S_IRUGO);
module_param(hfc8_login_retry, int, S_IRUGO);
module_param(hfc9_login_retry, int, S_IRUGO);
module_param(hfc10_login_retry, int, S_IRUGO);
module_param(hfc11_login_retry, int, S_IRUGO);
module_param(hfc12_login_retry, int, S_IRUGO);
module_param(hfc13_login_retry, int, S_IRUGO);
module_param(hfc14_login_retry, int, S_IRUGO);
module_param(hfc15_login_retry, int, S_IRUGO);
module_param(hfc16_login_retry, int, S_IRUGO);
module_param(hfc17_login_retry, int, S_IRUGO);
module_param(hfc18_login_retry, int, S_IRUGO);
module_param(hfc19_login_retry, int, S_IRUGO);
module_param(hfc20_login_retry, int, S_IRUGO);
module_param(hfc21_login_retry, int, S_IRUGO);
module_param(hfc22_login_retry, int, S_IRUGO);
module_param(hfc23_login_retry, int, S_IRUGO);
module_param(hfc24_login_retry, int, S_IRUGO);
module_param(hfc25_login_retry, int, S_IRUGO);
module_param(hfc26_login_retry, int, S_IRUGO);
module_param(hfc27_login_retry, int, S_IRUGO);
module_param(hfc28_login_retry, int, S_IRUGO);
module_param(hfc29_login_retry, int, S_IRUGO);
module_param(hfc30_login_retry, int, S_IRUGO);
module_param(hfc31_login_retry, int, S_IRUGO);
/* FCLNX-GPL-0343 */

/* mailbox els retry  *//* FCLNX-GPL-0343 */
module_param(hfc_els_retry, int, S_IRUGO);
MODULE_PARM_DESC(hfc_els_retry,
                "Driver mailbox els retry for all adapter : 0 - 10 (default 3 times)");
module_param(hfc0_els_retry, int, S_IRUGO);
module_param(hfc1_els_retry, int, S_IRUGO);
module_param(hfc2_els_retry, int, S_IRUGO);
module_param(hfc3_els_retry, int, S_IRUGO);
module_param(hfc4_els_retry, int, S_IRUGO);
module_param(hfc5_els_retry, int, S_IRUGO);
module_param(hfc6_els_retry, int, S_IRUGO);
module_param(hfc7_els_retry, int, S_IRUGO);
module_param(hfc8_els_retry, int, S_IRUGO);
module_param(hfc9_els_retry, int, S_IRUGO);
module_param(hfc10_els_retry, int, S_IRUGO);
module_param(hfc11_els_retry, int, S_IRUGO);
module_param(hfc12_els_retry, int, S_IRUGO);
module_param(hfc13_els_retry, int, S_IRUGO);
module_param(hfc14_els_retry, int, S_IRUGO);
module_param(hfc15_els_retry, int, S_IRUGO);
module_param(hfc16_els_retry, int, S_IRUGO);
module_param(hfc17_els_retry, int, S_IRUGO);
module_param(hfc18_els_retry, int, S_IRUGO);
module_param(hfc19_els_retry, int, S_IRUGO);
module_param(hfc20_els_retry, int, S_IRUGO);
module_param(hfc21_els_retry, int, S_IRUGO);
module_param(hfc22_els_retry, int, S_IRUGO);
module_param(hfc23_els_retry, int, S_IRUGO);
module_param(hfc24_els_retry, int, S_IRUGO);
module_param(hfc25_els_retry, int, S_IRUGO);
module_param(hfc26_els_retry, int, S_IRUGO);
module_param(hfc27_els_retry, int, S_IRUGO);
module_param(hfc28_els_retry, int, S_IRUGO);
module_param(hfc29_els_retry, int, S_IRUGO);
module_param(hfc30_els_retry, int, S_IRUGO);
module_param(hfc31_els_retry, int, S_IRUGO);
/* FCLNX-GPL-0343 */

/* ioctl timeout period  *//* FCLNX-GPL-0343 */
module_param(hfc_ioctl_scsi_timeout, int, S_IRUGO);
MODULE_PARM_DESC(hfc_ioctl_scsi_timeout,
                "Driver ioctl scsi command timeout period for all adapter : 1 - 120 (default 30s)");
module_param(hfc0_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc1_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc2_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc3_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc4_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc5_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc6_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc7_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc8_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc9_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc10_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc11_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc12_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc13_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc14_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc15_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc16_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc17_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc18_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc19_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc20_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc21_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc22_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc23_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc24_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc25_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc26_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc27_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc28_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc29_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc30_ioctl_scsi_timeout, int, S_IRUGO);
module_param(hfc31_ioctl_scsi_timeout, int, S_IRUGO);
/* FCLNX-GPL-0343 */

/* Limit Log *//* FCLNX-GPL-510 *//* FCLNX-GPL-FX-366 */
module_param(hfc_limit_log, int, S_IRUGO);
MODULE_PARM_DESC(hfc_limit_log,
                "Driver limit to output log for all adapter : 0 - 1 (default 0)");
module_param(hfc0_limit_log , int, S_IRUGO);
module_param(hfc1_limit_log , int, S_IRUGO);
module_param(hfc2_limit_log , int, S_IRUGO);
module_param(hfc3_limit_log , int, S_IRUGO);
module_param(hfc4_limit_log , int, S_IRUGO);
module_param(hfc5_limit_log , int, S_IRUGO);
module_param(hfc6_limit_log , int, S_IRUGO);
module_param(hfc7_limit_log , int, S_IRUGO);
module_param(hfc8_limit_log , int, S_IRUGO);
module_param(hfc9_limit_log , int, S_IRUGO);
module_param(hfc10_limit_log , int, S_IRUGO);
module_param(hfc11_limit_log , int, S_IRUGO);
module_param(hfc12_limit_log , int, S_IRUGO);
module_param(hfc13_limit_log , int, S_IRUGO);
module_param(hfc14_limit_log , int, S_IRUGO);
module_param(hfc15_limit_log , int, S_IRUGO);

/* Filtering Login Target *//* FCLNX-GPL-510 *//* FCLNX-GPL-FX-366 */
module_param(hfc_filter_target , int, S_IRUGO);
MODULE_PARM_DESC(hfc_filter_target,
                "Driver filter target to issue Login : 0 - 1 (default 0)");
module_param(hfc0_filter_target , int, S_IRUGO);
module_param(hfc1_filter_target , int, S_IRUGO);
module_param(hfc2_filter_target , int, S_IRUGO);
module_param(hfc3_filter_target , int, S_IRUGO);
module_param(hfc4_filter_target , int, S_IRUGO);
module_param(hfc5_filter_target , int, S_IRUGO);
module_param(hfc6_filter_target , int, S_IRUGO);
module_param(hfc7_filter_target , int, S_IRUGO);
module_param(hfc8_filter_target , int, S_IRUGO);
module_param(hfc9_filter_target , int, S_IRUGO);
module_param(hfc10_filter_target , int, S_IRUGO);
module_param(hfc11_filter_target , int, S_IRUGO);
module_param(hfc12_filter_target , int, S_IRUGO);
module_param(hfc13_filter_target , int, S_IRUGO);
module_param(hfc14_filter_target , int, S_IRUGO);
module_param(hfc15_filter_target , int, S_IRUGO);

/* set cmd_per_lun   */
module_param(hfc_cmd_per_lun , int, S_IRUGO);
MODULE_PARM_DESC(hfc_cmd_per_lun ,
                "Driver cmd_per_lun for all adapters :  0 or 1 (default 0)");
module_param(hfc0_cmd_per_lun , int, S_IRUGO);
module_param(hfc1_cmd_per_lun , int, S_IRUGO);
module_param(hfc2_cmd_per_lun , int, S_IRUGO);
module_param(hfc3_cmd_per_lun , int, S_IRUGO);
module_param(hfc4_cmd_per_lun , int, S_IRUGO);
module_param(hfc5_cmd_per_lun , int, S_IRUGO);
module_param(hfc6_cmd_per_lun , int, S_IRUGO);
module_param(hfc7_cmd_per_lun , int, S_IRUGO);
module_param(hfc8_cmd_per_lun , int, S_IRUGO);
module_param(hfc9_cmd_per_lun , int, S_IRUGO);
module_param(hfc10_cmd_per_lun , int, S_IRUGO);
module_param(hfc11_cmd_per_lun , int, S_IRUGO);
module_param(hfc12_cmd_per_lun , int, S_IRUGO);
module_param(hfc13_cmd_per_lun , int, S_IRUGO);
module_param(hfc14_cmd_per_lun , int, S_IRUGO);
module_param(hfc15_cmd_per_lun , int, S_IRUGO);
module_param(hfc16_cmd_per_lun , int, S_IRUGO);
module_param(hfc17_cmd_per_lun , int, S_IRUGO);
module_param(hfc18_cmd_per_lun , int, S_IRUGO);
module_param(hfc19_cmd_per_lun , int, S_IRUGO);
module_param(hfc20_cmd_per_lun , int, S_IRUGO);
module_param(hfc21_cmd_per_lun , int, S_IRUGO);
module_param(hfc22_cmd_per_lun , int, S_IRUGO);
module_param(hfc23_cmd_per_lun , int, S_IRUGO);
module_param(hfc24_cmd_per_lun , int, S_IRUGO);
module_param(hfc25_cmd_per_lun , int, S_IRUGO);
module_param(hfc26_cmd_per_lun , int, S_IRUGO);
module_param(hfc27_cmd_per_lun , int, S_IRUGO);
module_param(hfc28_cmd_per_lun , int, S_IRUGO);
module_param(hfc29_cmd_per_lun , int, S_IRUGO);
module_param(hfc30_cmd_per_lun , int, S_IRUGO);
module_param(hfc31_cmd_per_lun , int, S_IRUGO);

/* set max_sectors   */
module_param(hfc_max_sectors , int, S_IRUGO);
MODULE_PARM_DESC(hfc_max_sectors ,
                "Driver max_sectors for all adapters :  0 or 1 (default 0)");
module_param(hfc0_max_sectors , int, S_IRUGO);
module_param(hfc1_max_sectors , int, S_IRUGO);
module_param(hfc2_max_sectors , int, S_IRUGO);
module_param(hfc3_max_sectors , int, S_IRUGO);
module_param(hfc4_max_sectors , int, S_IRUGO);
module_param(hfc5_max_sectors , int, S_IRUGO);
module_param(hfc6_max_sectors , int, S_IRUGO);
module_param(hfc7_max_sectors , int, S_IRUGO);
module_param(hfc8_max_sectors , int, S_IRUGO);
module_param(hfc9_max_sectors , int, S_IRUGO);
module_param(hfc10_max_sectors , int, S_IRUGO);
module_param(hfc11_max_sectors , int, S_IRUGO);
module_param(hfc12_max_sectors , int, S_IRUGO);
module_param(hfc13_max_sectors , int, S_IRUGO);
module_param(hfc14_max_sectors , int, S_IRUGO);
module_param(hfc15_max_sectors , int, S_IRUGO);
module_param(hfc16_max_sectors , int, S_IRUGO);
module_param(hfc17_max_sectors , int, S_IRUGO);
module_param(hfc18_max_sectors , int, S_IRUGO);
module_param(hfc19_max_sectors , int, S_IRUGO);
module_param(hfc20_max_sectors , int, S_IRUGO);
module_param(hfc21_max_sectors , int, S_IRUGO);
module_param(hfc22_max_sectors , int, S_IRUGO);
module_param(hfc23_max_sectors , int, S_IRUGO);
module_param(hfc24_max_sectors , int, S_IRUGO);
module_param(hfc25_max_sectors , int, S_IRUGO);
module_param(hfc26_max_sectors , int, S_IRUGO);
module_param(hfc27_max_sectors , int, S_IRUGO);
module_param(hfc28_max_sectors , int, S_IRUGO);
module_param(hfc29_max_sectors , int, S_IRUGO);
module_param(hfc30_max_sectors , int, S_IRUGO);
module_param(hfc31_max_sectors , int, S_IRUGO);

/* set vary_io   */
module_param(hfc_vary_io , int, S_IRUGO);
MODULE_PARM_DESC(hfc_vary_io ,
                "Driver vary_io for all adapters :  0 or 1 (default 0)");
module_param(hfc0_vary_io , int, S_IRUGO);
module_param(hfc1_vary_io , int, S_IRUGO);
module_param(hfc2_vary_io , int, S_IRUGO);
module_param(hfc3_vary_io , int, S_IRUGO);
module_param(hfc4_vary_io , int, S_IRUGO);
module_param(hfc5_vary_io , int, S_IRUGO);
module_param(hfc6_vary_io , int, S_IRUGO);
module_param(hfc7_vary_io , int, S_IRUGO);
module_param(hfc8_vary_io , int, S_IRUGO);
module_param(hfc9_vary_io , int, S_IRUGO);
module_param(hfc10_vary_io , int, S_IRUGO);
module_param(hfc11_vary_io , int, S_IRUGO);
module_param(hfc12_vary_io , int, S_IRUGO);
module_param(hfc13_vary_io , int, S_IRUGO);
module_param(hfc14_vary_io , int, S_IRUGO);
module_param(hfc15_vary_io , int, S_IRUGO);
module_param(hfc16_vary_io , int, S_IRUGO);
module_param(hfc17_vary_io , int, S_IRUGO);
module_param(hfc18_vary_io , int, S_IRUGO);
module_param(hfc19_vary_io , int, S_IRUGO);
module_param(hfc20_vary_io , int, S_IRUGO);
module_param(hfc21_vary_io , int, S_IRUGO);
module_param(hfc22_vary_io , int, S_IRUGO);
module_param(hfc23_vary_io , int, S_IRUGO);
module_param(hfc24_vary_io , int, S_IRUGO);
module_param(hfc25_vary_io , int, S_IRUGO);
module_param(hfc26_vary_io , int, S_IRUGO);
module_param(hfc27_vary_io , int, S_IRUGO);
module_param(hfc28_vary_io , int, S_IRUGO);
module_param(hfc29_vary_io , int, S_IRUGO);
module_param(hfc30_vary_io , int, S_IRUGO);
module_param(hfc31_vary_io , int, S_IRUGO);

/* set debug_func */ /* FCLNX-GPL-236 */
//module_param(hfc_debug_func , int, S_IRUGO);
MODULE_PARM_DESC(hfc_debug_func ,
                "Driver debug mode for all adapters :  0x00-0xff (default 0)");
/* set issue_d3hot */ /* FCLNX-GPL-306 */
//module_param(hfc_issue_d3hot , int, S_IRUGO);
MODULE_PARM_DESC(hfc_issue_d3hot ,
                "Driver debug mode for all adapters :  0x00-0xff (default 0)");

/* FCLNX-GPL-547 start */
#if 0
module_param(hfc_limit_log , byte, S_IRUGO);
MODULE_PARM_DESC(hfc_limit_log ,
                "Limit Log for all adapters :  0 or 1 (default 0)");
#endif

module_param(hfc_log_file , byte, S_IRUGO);
MODULE_PARM_DESC(hfc_log_file ,
                "Choice of log files for all adapters :  0,1,2,3,4 or 5 (default 0)");

module_param(hfc_max_lun , int, S_IRUGO);
MODULE_PARM_DESC(hfc_max_lun ,
                "Max lu number for all adapters :  0x00-0xffff (default 0xffff)");
/* FCLNX-GPL-547 end */

/* FCLNX-GPL-565 start */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* scan_finished time out value */
module_param(hfc_scan_finished_tmo , int, S_IRUGO);
MODULE_PARM_DESC(hfc_scan_finished_tmo ,
                "scan_finished time out value for all adapters :  0-3600 (default 30)");
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
/* FCLNX-GPL-565 end */

#if _HFC_ERROR_INJ														/* FCLNX-0246 */
MODULE_PARM(hfc_debug_ioerr, "i");
MODULE_PARM_DESC(hfc_debug_ioerr,
                "Issue FC or SCSI I/O Error 1-99:FC Error, 100-199:Disk Error (default 0)");
#endif 																		/* FCLNX-0246 */

#if 0		/* FCLNX-GPL-0449 */
module_param(write_retries, charp, S_IRUGO);
MODULE_PARM_DESC(write_retries,
                "Write command retries parameter for all adapter ");
#endif

module_param(hfc_cpu_map , int, S_IRUGO);

module_param(hfc_debug_func , int, S_IRUGO);
module_param(hfc_issue_d3hot , int, S_IRUGO);
