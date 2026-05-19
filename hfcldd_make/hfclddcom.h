/*
 * hfclddcom.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfclddcom.h,v 1.11.2.7.2.7.4.1.6.15.2.1.2.5.2.11.2.10.2.2.2.1.2.3 2015/04/27 10:27:06 toyo Exp $
 */

#ifndef _H_HFCDDCOM
#define _H_HFCDDCOM

/**************************************************************************************************/
/* literal																						  */
/**************************************************************************************************/
#define MAX_EXCHANGE		256			/* max exchange num				*/
#define MAX_MP_ADAP_CNT		256			/* max mp_adap_info num			*/
#define MAX_ADAP_CNT		256			/* max adap_info num 			*/
#define MAX_TARGET_PROBE	256			/* max target_info num			*/
#define MAX_TARGET_HASH		16			/* target_info hash nom			*/

#define MAX_REGION_PROBE	32			/* max region_info num			*//* FIVE-FX */
#define MAX_CORE_PROBE		4			/* max core_info num			*//* FIVE-FX */
#define	MAX_EXT_VPORT_PROBE	256			/* max ext vport_info num		*//* FIVE-FX */

#define MAX_DEV_CNT 		0x800	 	/* max lun count		 		*//* FCLNX-GPL-0343 *//* FCLNX-GPL-0449 */
#define MAX_XOB_NUM 		256 		/* XOB num						*/
#define MAX_XRB_NUM 		256 		/* XRB num						*/
#define HASH_T_NUM			16			/* hash num in response queue	*/
#define MAX_LOOP_LUN		8			/* max lun in Loop test 		*/
#define	MAX_FW_BUS_CNT		128			/* Bus-addr in fw_init_tbl		*/
										/* used xob/xrb/slog			*/
#define HFC_SLOG_LEN		1024		/* Soft Log length				*/
#define HFC_SOFT_LOG_PAGE	64			/* Soft Log(default)			*/
#define HFC_XOB_PER_PAGE	32			/* xob num in page				*/
#define HFC_XRB_PER_PAGE	8			/* xrb num in page				*/
#define HFC_SEG_PER_PAGE	256			/* seg_info in page 			*/
#define HFC_CMD_IU_PLEN		32			/* FCP_CMD_IU payload length 	*/

#define HFC_PAGE_SIZE		4096		/* page size 					*/
#define HFC_BITS_PER_WORD	32			/* bit num in int 				*/
#define HFC_BYTE_PER_WORD	4			/* byte num in int				*/
#define	HFC_ALL_ONE			0xffffffff	/* ALL'1'						*/

/* FCLNX-0542 */
#define HFC_SCSI_TO_RETRY	0			/* Default to Retry of SCSI Command */	/* FCLNX-0523 */
#define HFC_SCSI_TO_RETRY_MIN	0		/* Minimum of Retry for SCSI Command */	/* FCLNX-0523 */
#define HFC_SCSI_TO_RETRY_MAX	10		/* Max of Retry for SCSI Command */		/* FCLNX-0523 */
/* FCLNX-0542 */

/* FCLNX-GPL-311 */
#define HFC_SFP_TYPE_NAME_LEN		16
#define HFC_SFP_SERIAL_NO_LEN		16
#define HFC_SFP_DATE_CODE_LEN		8
/* FCLNX-GPL-311 */

#define HFC_VPORT_SOFT_LOG_PAGE		16	/* Soft Log(default)			*/

/*----------------------------------------*/
/*--  errlog type						--*/
/*----------------------------------------*/
#define	HFC_ERRLOG_TYPE_NONE	0		/* non fw_slog 					*/
#define HFC_ERRLOG_TYPE_XRB		1		/* check xrb.esw & fw_slog		*/
#define	HFC_ERRLOG_TYPE_MBRESP	2		/* check mb_resp & fw_slog		*/
#define	HFC_ERRLOG_TYPE_MBINT	3		/* check mb_int  & fw_slog		*/
#define	HFC_ERRLOG_TYPE_MCK		4		/* mckint H/W log 				*/
#define	HFC_ERRLOG_TYPE_CHKSTP	5		/* check stop log				*/
#define	HFC_ERRLOG_TYPE_IMLLOG	6		/* iml log						*/
#define	HFC_ERRLOG_TYPE_TOUTLOG	7		/* T-OUT Log					*/
#define HFC_ERRLOG_TYPE_CANCEL	8		/* <> T-OUT for MIHLOG			*/
#define HFC_ERRLOG_TYPE_IMLLOG_DIAG 9   /* iml log from diag			*/
#define HFC_ERRLOG_TYPE_RASLOG_EXIST 10 /* RASLOG exists				*/
#define HFC_ERRLOG_TYPE_IODONE_WARN1 11 /* Warning message in hfc_iodone */
#define HFC_ERRLOG_TYPE_IODONE_WARN2 12 /* Warning message in hfc_iodone */
#define HFC_ERRLOG_TYPE_IODONE_WARN3 13 /* Warning message in hfc_iodone */
#define HFC_ERRLOG_TYPE_IODONE_WARN4 14 /* Warning message in hfc_iodone */
#define HFC_ERRLOG_TYPE_IODONE_WARN5 15 /* Warning message in hfc_iodone */
#define	HFC_ERRLOG_TYPE_MBINIT	16		/* check mb_initiate			*/
#define HFC_ERRLOG_TYPE_SRAMCE  17      /* IPCore or F/W 1bit CE		*/	
#define HFC_ERRLOG_TYPE_LINKINCLOG 18	/* Link Incident Log for FIVE-FX */
#define HFC_ERRLOG_TYPE_LINKUPTOUT 19	/* Link Up Timer is timed out.	*//* FCLNX-GPL-FX-139 */
#define HFC_ERRLOG_TYPE_TGT_NOTFOUND 20	/* The target is not found.		*//* FCLNX-GPL-FX-139 */
/*----------------------------------------*/
/*-- hw_log type						--*/
/*----------------------------------------*/
#define	HFC_HW_LOG_NONE			0			/* not used */
#define	HFC_IML_LOG				1
#define	HFC_MCK_LOG				2
#define	HFC_CHK_LOG				3
/*----------------------------------------*/
/*--  ABEND code						--*/
/*----------------------------------------*/
#define HFC_ABEND_XRB_INVALID	0x01	/* xrb valid = 0				*/
#define HFC_ABEND_SERR 			0x02	/* int_a register (pci xx)		*/
#define HFC_ABEND_MCK_INT		0x03	/* int_a register (MCK)			*/
#define HFC_ABEND_MPCHK			0x04	/* int_a register (MCK & MPCHK) */
#define	HFC_ABEND_CCC			0x05	/* xrb fsb=ccc or xcc!=80		*/
#define HFC_ABEND_MB_TOUT		0x06	/* mailbox timeout				*/
#define HFC_ABEND_MB_RSPERR		0x07	/* mailbox resp error			*/
#define HFC_ABEND_T3			0x08	/* Time Out 3					*/
#define HFC_ABEND_PERR			0x09	/* int_a register (pci xx)		*/
#define HFC_ABEND_SPERR			0x0a	/* int_a register (pci xx)		*/
#define HFC_ABEND_INTA_ZERO		0x0c	/* int_a register all 0			*/
#define HFC_ABEND_EEH_FAIL		0x0d	/* failed EEH recovery (reset)	*/
#define HFC_ABEND_TOUTCHK_XOB	0x0e	/* remain xob (TOUT)			*/
#define HFC_ABEND_HFCPKT_CHK	0x0f	/* hfc_pkt address invalid		*/
#define HFC_ABEND_T3_NO_MCKINT	0x0b	/* Time Out 3 (no mck)			*/

#define HFC_ABEND_FWUP_COMP_ERR	 0x80	/* FW Update failed(Comp.Err)	*/
#define HFC_ABEND_FWUP_ERASE_ERR 0x81	/* FW Update failed(Erase Err)	*/

#define HFC_ABEND_EVENT		0xf0		/* EVENT						*/
#define HFC_ABEND_PAYLD		0xf1		/* PAYLD						*/

/* extra abend code for MLPF */
#define HFC_ABEND_FCSTP             0x10
#define HFC_ABEND_FCSTP_IML         0x11
#define HFC_ABEND_CSTP_END          0x12
#define HFC_ABEND_FMCK              0x13
#define HFC_ABEND_GLPAR             0x14
#define HFC_ABEND_RID_INVALID       0x15
#define HFC_ABEND_MCKEND            0x16
#define HFC_ABEND_PIC_ERROR			0x17	/* PIC resp error			*//* FCLNX-GPL-576 */

#define HFC_ABEND_MCK_RESUME		0x18	/* MCK Recovery resume		*//* FCLNX-GPL-FX-146 *//* FCLNX-GPL-613 */

#define HFC_ABEND_ISOL				0x20
#define HFC_ABEND_LINK_RESET		0x21	/* ISSUE LINK RESET			*//* FIVE-FX */

#define HFC_ABEND_SRAM_CE			0x33	/* PCIe IP Core ERR(CE)			*/

/*----------------------------------------*/
/*--  mailbox retry count				--*/
/*----------------------------------------*/
#define HFC_LINK_INIT_RETRY		3
#define HFC_LOGIN_RETRY			3
/* FCLNX-0542 */
#define HFC_LOGIN_RETRY_MIN	0			/* Minimum of Retry for LOGIN */			/* FCLNX-0523 */
#define HFC_LOGIN_RETRY_MAX	10			/* Max of Retry for LOGIN */				/* FCLNX-0523 */
/* FCLNX-0542 */
#define	HFC_ELS_RETRY			3
/* FCLNX-0542 */
#define HFC_ELS_RETRY_MIN	0			/* Minimum of Retry for ELS */				/* FCLNX-0523 */
#define HFC_ELS_RETRY_MAX	10			/* Max of Retry for ELS */					/* FCLNX-0523 */
#define HFC_TO_RESET_RETRY		3		/* Retry of LOGIN After SCSI T.O */ 		/* FCLNX-0523 */
#define HFC_TO_RESET_RETRY_MIN	1		/* Minimum of Retry for LOGIN After SCSI T.O */	/* FCLNX-0523 *//* FCLNX-GPL-372,377 */
#define HFC_TO_RESET_RETRY_MAX	3		/* Max of Retry for LOGIN After SCSI T.O */		/* FCLNX-0523 *//* FCLNX-GPL-372,377 */
/* FCLNX-0542 */

#define HFC_TO_RESET_RETRY_MIN_PCM	0	/* Minimum of Retry for LOGIN After SCSI T.O for HFC-PCM */	/* FCLNX-0745 */
#define HFC_TO_RESET_RETRY_MAX_PCM	10	/* Max of Retry for LOGIN After SCSI T.O for HFC-PCM */		/* FCLNX-0745 */

#define	HFC_IOCTL_SCSI_TIMEOUT		30		/* FCLNX-GPL-0343 */
/* FCLNX-0542 */
#define HFC_IOCTL_SCSI_TIMEOUT_MIN	1		/* Minimum of ioctl timeout period */	/* FCLNX-GPL-0343 */
#define HFC_IOCTL_SCSI_TIMEOUT_MAX	120		/* Max of ioctl timeout period */		/* FCLNX-GPL-0343 */

/*----------------------------------------*/
/*-- PCI configration register			--*/
/*----------------------------------------*/
#define HFC_PCICFG_BASE0		0x10
#define HFC_PCICFG_BASE1		0x14
#define HFC_PCICFG_BASE2		0x18
/*----------------------------------------*/
/*-- PCI bus I/O register offset addr	--*/
/*----------------------------------------*/
									/* ADR :			 FPP ,	 FIVE ,FIVE-EX,FIVE-FX */
#define HFC_IOSPACE_ZERO				0			/* 0x0000,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_LSICODE				1			/* 0x0001,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_LSIREV				2			/* 0x0003,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_OFS4				3			/* 0x0004,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_PKCODE				4			/* 0x0005,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_PKREV				5			/* 0x0007,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CHNO				6			/* 0x0008,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_STATUS0				7			/* 0x0010,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_STATUS1				8			/* 0x0014,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_ERRDETAIL0			9			/* 0x0018,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_ERRDETAIL1			10			/* 0x001c,	  X   ,	  <-  ,	  <-   */

#define HFC_IOSPACE_INTA				11			/* 0x0020,	0x00a0,	  <-  ,	  <-   */
#define HFC_IOSPACE_MPINTAC				12			/* 0x0024,	0x00a4,	  <-  ,	  <-   */
#define HFC_IOSPACE_INTA_MSK			13			/* 0x0028,	0x00a8,	  <-  ,	  <-   */
#define HFC_IOSPACE_INTA_RST			14			/* 0x002c,	0x00ac,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDRES				15			/* 0x0040,	0x0030,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDCTL				16			/* 0x0041,	0x0031,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDBOOT				17			/* 0x0042,	0x0032,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDLED				18			/* 0x0043,	0x0038,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDFMEM				19			/* 0x0044,	0x0034,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDFCIF				20			/* 0x0045,	0x003c,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDMODE				21			/* 0x0047,	0x0037,	  X   ,	  <-   */
#define HFC_IOSPACE_TRCA0				22			/* 0x00f0,	0x00f0,	  <-  ,	  <-   */
#define HFC_IOSPACE_TRCA1				23			/* 0x00f2,	0x00f2,	  <-  ,	  <-   */
#define HFC_IOSPACE_RAMMSK				24			/* 0x02f8,	0x02f8,	  <-  ,	  <-   */
#define HFC_IOSPACE_RAMADR				25			/* 0x02fc,	0x02fc,	  <-  ,	  <-   */
#define HFC_IOSPACE_HTYP				26			/* 0x0200,	0x001c,	  <-  ,	  <-   */
#define HFC_IOSPACE_OTYP				27			/* 0x0201,	0x001d,	  <-  ,	  <-   */
#define HFC_IOSPACE_HWINF				28			/* 0x0202,	0x001e,	  <-  ,	  <-   */
#define HFC_IOSPACE_LAPC				29			/* 0x0203,	0x001f,	  <-  ,	  <-   */
#define HFC_IOSPACE_CMDSCAN				30			/*   X,   	0x0036,	  X   ,	  <-   */
#define HFC_IOSPACE_IDFLGEN				31			/*   X,   	0x02fb,	  <-  ,	  <-   */
#define HFC_IOSPACE_TGTCORE				32			/*	 X,		  <-  ,	  <-  , 0x02f0 */

#define HFC_IOSPACE_FRAMEA				40			/* 0x0400,	0x0500,	  <-  ,	  <-   */
#define HFC_IOSPACE_INDAREA				41			/* 0x0500,	0x0600,	  <-  ,	  <-   */
#define HFC_IOSPACE_INDAREA_P			42			/* 0x0600,	0x0700,	  <-  ,	  <-   */
#define HFC_IOSPACE_SCANAREA			43			/* 0x0800,	0x1000,	  <-  ,	  <-   */

#define HFC_IOSPACE_CA_POSTRESULT		46			/* 0x0300,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_MPCHK_CODE		47			/* 0x0304,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_FLAG				48			/* 0x030c,	  <-  ,	  <-  ,	0x0324 */
#define HFC_IOSPACE_CA_INIT_ADDR0		49			/* 0x0310,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_INIT_ADDR1		50			/* 0x0314,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_RSTINFO			51			/* 0x0318,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_ALPA				52			/* 0x0319,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_PORTNO			53			/*	 X   ,	0x031d,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_POST				54			/* 0x031c,	  <-  ,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_WWPN0			55			/*	 X   ,	0x0350,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_WWPN1			56			/*	 X   ,	0x0354,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_WWNN0			57			/*	 X   ,	0x0358,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_WWNN1			58			/*	 X   ,	0x035c,	  <-  ,	  <-   */
#define HFC_IOSPACE_DRV_USED0			59			/* 0x03c0,	0x0360,	  <-  ,	  <-   */
#define HFC_IOSPACE_DRV_USED1			60			/* 0x03c4,	0x0364,	  <-  ,	  <-   */
#define HFC_IOSPACE_DRV_USED2			61			/* 0x03c8,	0x0368,	  <-  ,	  <-   */
#define HFC_IOSPACE_DRV_USED3			62			/* 0x03cc,	0x036c,	  <-  ,	  <-   */
#define HFC_IOSPACE_CA_LNKSPD			63			/* 0x031a,	0x031a,	  <-  ,	  <-   */	/* FCWIN-0200 */
#define HFC_IOSPACE_CA_CNTTYP			64			/* 0x031b,	0x031b,	  <-  ,	  <-   */	/* FCWIN-0200 */
#define HFC_IOSPACE_CA_ONUP_STATE		65			/* 0x031e,	       	      ,	  <-   */	/* FCLNX-GPL-112 */
#define HFC_IOSPACE_FW_SUPPORT			66			/*	 X   ,	0x370 ,	  <-  ,	  <-   */	/* FCLNX_GPL-146 */
#define HFC_IOSPACE_DUMP_CMD			70			/*	 X   ,	  X   ,	0x0884,	  <-   */
#define HFC_IOSPACE_KCMD_IPRES			71			/*	 X   ,	  X   ,	0x083f,	  <-   */
#define HFC_IOSPACE_LINK_INI_OPT	 	72			/*	 X  ,	0x031f,   <-	  <-   */	/* FCLNX_GPL-FX-366 */

#define HFC_IOSPACE_INT_VECTOR			73			/*	 X   ,	  X   ,	  X   ,	0x00e0 */
#define HFC_IOSPACE_INT_VECTOR_RST		74			/*	 X   ,	  X   ,	  X   ,	0x00e4 */
#define HFC_IOSPACE_NAKED_INT0			75			/*	 X   ,	  X   ,	  X   ,	0x00f0 */
#define	HFC_IOSPACE_PTYP0				76			/*	 X   ,	  X   ,	  X   ,	0x0110 */

#define HFC_IOSPACE_MAINPF				79			/*	 X   ,	  X   ,	  X   ,	0x0150 */
#define HFC_IOSPACE_CORE0_STATUS0		80			/*	 X   ,	  X   ,	  X   ,	0x0c10 */
#define HFC_IOSPACE_CORE0_CMD1			81			/*	 X   ,	  X   ,	  X   ,	0x0c31 */
#define HFC_IOSPACE_CORE1_STATUS0		82			/*	 X   ,	  X   ,	  X   ,	0x0d10 */
#define HFC_IOSPACE_CORE1_CMD1			83			/*	 X   ,	  X   ,	  X   ,	0x0d31 */
#define HFC_IOSPACE_CORE2_STATUS0		84			/*	 X   ,	  X   ,	  X   ,	0x0e10 */
#define HFC_IOSPACE_CORE2_CMD1			85			/*	 X   ,	  X   ,	  X   ,	0x0e31 */
#define HFC_IOSPACE_CORE3_STATUS0		86			/*	 X   ,	  X   ,	  X   ,	0x0f10 */
#define HFC_IOSPACE_CORE3_CMD1			87			/*	 X   ,	  X   ,	  X   ,	0x0f31 */
#define HFC_IOSPACE_FRAMEB				88			/*	 X   ,	  X   ,	  X   ,	0x0540 */
#define HFC_IOSPACE_FRAMEA1				89			/*	 X   ,	  X   ,	  X   ,	0x0580 */
#define HFC_IOSPACE_FRAMEB1				90			/*	 X   ,	  X   ,	  X   ,	0x05c0 */
#define HFC_IOSPACE_CORE0_CMD2			91			/*	 X   ,	  X   ,	  X   ,	0x0c32 */

#define HFC_IOSPACE_HPMFLGEN			94			/*	 X   ,	  X   ,	  X   ,	0x0153 */
#define HFC_IOSPACE_HPMFLG				95			/*	 X   ,	  X   ,	  X   ,	0x0154 */
#define HFC_IOSPACE_HPMFLGPFNO			96			/*	 X   ,	  X   ,	  X   ,	0x0157 */
#define HFC_IOSPACE_INT_1				97			/*	 X   ,	  X   ,	  X   ,	0x00b0 *//* core#1 */
#define HFC_IOSPACE_MPINT_1C			98			/*	 X   ,	  X   ,	  X   ,	0x00b4 *//* core#1 */
#define HFC_IOSPACE_INT_1_MSK			99			/*	 X   ,	  X   ,	  X   ,	0x00b8 *//* core#1 */
#define HFC_IOSPACE_INT_1_RST			100			/*	 X   ,	  X   ,	  X   ,	0x00bc *//* core#1 */
#define HFC_IOSPACE_INT_2				101			/*	 X   ,	  X   ,	  X   ,	0x00c0 *//* core#2 */
#define HFC_IOSPACE_MPINT_2C			102			/*	 X   ,	  X   ,	  X   ,	0x00c4 *//* core#2 */
#define HFC_IOSPACE_INT_2_MSK			103			/*	 X   ,	  X   ,	  X   ,	0x00c8 *//* core#2 */
#define HFC_IOSPACE_INT_2_RST			104			/*	 X   ,	  X   ,	  X   ,	0x00cc *//* core#2 */
#define HFC_IOSPACE_INT_3				105			/*	 X   ,	  X   ,	  X   ,	0x00d0 *//* core#3 */
#define HFC_IOSPACE_MPINT_3C			106			/*	 X   ,	  X   ,	  X   ,	0x00d4 *//* core#3 */
#define HFC_IOSPACE_INT_3_MSK			107			/*	 X   ,	  X   ,	  X   ,	0x00d8 *//* core#3 */
#define HFC_IOSPACE_INT_3_RST			108			/*	 X   ,	  X   ,	  X   ,	0x00dc *//* core#3 */
#define HFC_IOSPACE_MSIXVSHORT			109			/*	 X   ,	  X   ,	  X   ,	0x00e8 */

#define HFC_IOSPACE_CORE1_CA_POSTRESULT	112			/*	 X   ,	  X   ,	  X   ,	0x0380 *//* core#1 */
#define HFC_IOSPACE_CORE2_CA_POSTRESULT	113			/*	 X   ,	  X   ,	  X   ,	0x0400 *//* core#2 */
#define HFC_IOSPACE_CORE3_CA_POSTRESULT	114			/*	 X   ,	  X   ,	  X   ,	0x0480 *//* core#3 */
#define HFC_IOSPACE_CA_INIT1_ADDR0		115			/*	 X   ,	  X   ,	  X   ,	0x0390 *//* core#1 */
#define HFC_IOSPACE_CA_INIT1_ADDR1		116			/*	 X   ,	  X   ,	  X   ,	0x0394 *//* core#1 */
#define HFC_IOSPACE_CA_INIT2_ADDR0		117			/*	 X   ,	  X   ,	  X   ,	0x0410 *//* core#2 */
#define HFC_IOSPACE_CA_INIT2_ADDR1		118			/*	 X   ,	  X   ,	  X   ,	0x0414 *//* core#2 */
#define HFC_IOSPACE_CA_INIT3_ADDR0		119			/*	 X   ,	  X   ,	  X   ,	0x0490 *//* core#3 */
#define HFC_IOSPACE_CA_INIT3_ADDR1		120			/*	 X   ,	  X   ,	  X   ,	0x0494 *//* core#3 */
#define HFC_IOSPACE_CA_FLAG1			121			/*	 X   ,	  X   ,	  X   ,	0x03a4 *//* core#1 */
#define HFC_IOSPACE_CA_FLAG2			122			/*	 X   ,	  X   ,	  X   ,	0x0424 *//* core#2 */
#define HFC_IOSPACE_CA_FLAG3			123			/*	 X   ,	  X   ,	  X   ,	0x04a4 *//* core#3 */

#define HFC_IOSPACE_RSS_INTA			124			/*	X   ,	  X   ,	  X   ,	0x3000 *//* Multi Queue */
#define HFC_IOSPACE_RSS_INTA_RST		125			/*	X   ,	  X   ,	  X   ,	0x3004 *//* Multi Queue */
#define HFC_IOSPACE_RSS_INT_VECTOR		126			/*	X   ,	  X   ,	  X   ,	0x3008 *//* Multi Queue */
#define HFC_IOSPACE_RSS_INT_VECTOR_RST	127			/*	X   ,	  X   ,	  X   ,	0x300c *//* Multi Queue */

/* PCI bus IO spase FRAMEA field */
#define HFC_FRAMEA_FW_START			( 0x40000000 )
#define HFC_FRAMEA_FW_START_PORT	( 0x41000000 )  /* FCWIN-0514 */
#define HFC_FRAMEA_FW_ISOL_PORT		( 0x43000000 )	/* FCLNX_GPL-146 */
#define HFC_FRAMEA_ENQ_XOB			( 0x44000000 )
#define HFC_FRAMEA_MB_INIT			( 0xc0000000 )
#define HFC_FRAMEA_INT_RSP			( 0x20000000 )
#define HFC_FRAMEA_OFFLINE			( 0x42000000 )	/* FCWIN-0099 */
#define HFC_FRAMEA_ONLINEUP			( 0x4c000000 ) /* FCLNX-GPL-112 */

/* for MLPF */
#define HFC_FRAMEA_FW_START_MCK		( 0x50000000 )
#define HFC_FRAMEA_LOGOUT			( 0x52000000 )
#define HFC_FRAMEA_SHADOW_DOWN		( 0x58000000 )
#define HFC_FRAMEA_SHADOW_UP		( 0x59000000 )

#define HFC_FRAMEA_PORTSTATISTICS	( 0x68000000 )
/*----------------------------------------*/
/*--  PCI INT_A register				--*/
/*----------------------------------------*/
#define HFC_HWERR_INT				0xf0000000
#define HFC_HWERR_MCK				0x80000000
#define HFC_HWERR_PCI				0x70000000
#define	HFC_HWERR_SERR				0x40000000
#define	HFC_HWERR_PERR				0x20000000
#define	HFC_HWERR_SPERR				0x10000000
#define HFC_PCIE_SRAM_CE			0x00400000
#define HFC_MBRSP_INT				0x00004000
#define HFC_MBINT_REQ				0x00008000
#define HFC_XRB_RSP					0x00002000
#define HFC_MPINT3					0x00001000

#define HFC_HWERR_MPCHK				0x00000800

#define HFC_MLPF_REC_END			0x00000200  /* @MLPF */
#define HFC_MLPF_HWERR				0x00000100  /* @MLPF */
#define HFC_MLPF_HWERR_INT			0xf0000300  /* @MLPF */
/*--- error detail register --*/
#define HFC_HWERR_T3				0x00200000

static const uint hfc_inta_mask[5] = {
									0x00000000, /* PKTYPE is none */
									0xf000e000, /* 1 HFC_PKTYPE_FPP */
									0xf000e000, /* 2 HFC_PKTYPE_FIVE */
									0x8040e000, /* 3 HFC_PKTYPE_FIVE_EX */
									0x8800e000  /* 4 HFC_PKTYPE_FIVE_FX *//* FCLNX-GPL-FX-115 */
								};
static const uint hfc_inta_mask_mlpf[5] = {
									0x00000000, /* PKTYPE is none */
									0xf000e300, /* 1 HFC_PKTYPE_FPP */
									0xf000e300, /* 2 HFC_PKTYPE_FIVE */
									0x8040e300, /* 3 HFC_PKTYPE_FIVE_EX */
									0x8800e100  /* 4 HFC_PKTYPE_FIVE_FX *//* FCLNX-GPL-FX-120 */
								};
/*----------------------------------------*/
/*--  PCI MSI-X vector					--*/
/*----------------------------------------*/
#define HFC_MSIX_XRB				 0
/*----------------------------------------*/
/*--  Connfig H/W check Retry Count		--*/
/*----------------------------------------*/
#define HFC_CONFIG_HW_CHECK_RETRY	5
#define HFC_FX_CONFIG_HW_CHECK_RETRY	5	/* FCLNX-GPL-FX-215 */

/* Config Configuration Command */
#define	HFC_HOST_VENDER_ID	 	0x00
#define	HFC_HOST_DEVICE_ID	 	0x02
#define	HFC_HOST_STAT_CMD	 	0x04
#define	HFC_HOST_SUB_SYSTEM_ID	0x2e	/* FIVE-EX */
/*************************************/
/* PCI Configuration register        */
/*************************************/
#define HFC_HSC_IOS				0x01		/* enable IO space			*/
#define HFC_HSC_MSS				0x02		/* enable memory space		*/
#define HFC_HSC_MB 				0x04		/* bus master enable		*/
#define HFC_HSC_PER				0x40		/* parity error response	*/
#define HFC_HSC_SERR			0x0100		/* system error response	*/
#define HFC_PCI_STOP_FUNC		0x40		/* stop Function			*/

/* STATUS0 */
#define HFC_PCI_HPONRES			0x10
#define HFC_PCI_HGENRES			0x08
#define HFC_PCI_HINIRES			0x04
#define HFC_PCI_HCTLRES			0x02
#define HFC_PCI_HCHKRES			0x01

/* STATUS1 */
#define HFC_PCI_PCIERR_DETECTED	0x80
#define HFC_PCI_EXGMCK			0x40
#define HFC_PCI_BOOTRUN			0x20
#define HFC_PCI_FCNSTOP			0x10
#define HFC_PCI_TRACER_STOP		0x08

/* STATUS2 */
#define HFC_PCI_CH_NOT_READY	0x80
#define HFC_PCI_MPCHK			0x08
#define HFC_PCI_PORT_CHKSTOP	0x01

/* STATUS3 */
#define HFC_PCI_COMMON_MCK		0x20 /* FIVE-EX */

/* ERROR DETAIL0 */
#define HFC_PCI_HPUMCKST		0x20 /* FIVE-EX */
#define HFC_PCI_HBUMCKST		0x04 /* FIVE-EX */
#define HFC_PCI_HTUMCKST		0x02 /* FIVE-EX */
#define HFC_PCI_HRUMCKST		0x01 /* FIVE-EX */

/* ERROR DETAIL1 */
#define HFC_PCI_XHMPCK			0x10 /* FIVE-EX */
#define HFC_PCI_XHTO3CK			0x20 /* FIVE-EX */

/* ERROR DETAIL2 */
#define HFC_PCI_USERMCK			0x80 /* FIVE-EX */

/* Config H/W check */
#define HFC_WAKE_UP_FAILURE		0x88
#define HFC_POST_FAILURE		0x84
#define HFC_CORE_CHKSTOP		0x00000100
#define HFC_CONFIG_HWDMY_WAITCNT	43100000	/* 500ms Wait Count */
#define HFC_PCI_RESETCHK	   (HFC_PCI_HPONRES | HFC_PCI_HGENRES | HFC_PCI_HINIRES | HFC_PCI_HCTLRES | HFC_PCI_HCHKRES)
#define HFC_PCI_PCICHK		   (HFC_PCI_EXGMCK | HFC_PCI_BOOTRUN)

#define HFC_WAKE_UP_FAILURE_FIVE	0x88888888
#define HFC_POST_FAILURE_FIVE		0x84848484

/*----------------------------------------*/					/* @MLPF STR */
/*-- MMIO-HG register offset addr		--*/
/*----------------------------------------*/
										/* Ver :	01-00	*/
#define HFC_IOHGSPC_ZERO			0			/* 0x0000	*/
#define HFC_IOHGSPC_LENGTH			1			/* 0x0000	*/
#define HFC_IOHGSPC_VERSION			2			/* 0x0004	*/
#define HFC_IOHGSPC_HYPSTATUS		3			/* 0x0008	*/
#define HFC_IOHGSPC_CMNDREG			4			/* 0x000c	*/
#define HFC_IOHGSPC_VFCWWPN0		5			/* 0x0010	*/
#define HFC_IOHGSPC_VFCWWPN1		6			/* 0x0014	*/
#define HFC_IOHGSPC_VFCWWNN0		7			/* 0x0018	*/
#define HFC_IOHGSPC_VFCWWNN1		8			/* 0x001c	*/
#define HFC_IOHGSPC_RID				9			/* 0x0020	*/
#define HFC_IOHGSPC_MLPFDDVER		10			/* 0x0024	*/
#define HFC_IOHGSPC_LPARSTATUS		11			/* 0x0028	*/
#define HFC_IOHGSPC_HYPINTDETAIL    12          /* 0x002c   */ /* FCLNX-GPL-427 */

#define HFC_IOHGSPC_INDACTYPE		18			/* 0x0031	*/
#define HFC_IOHGSPC_INDACSIZE		19			/* 0x0032	*/
#define HFC_IOHGSPC_INDACC0			20			/* 0x0030	*/
#define HFC_IOHGSPC_INDACC1			21			/* 0x0034	*/
#define HFC_IOHGSPC_INDACC2			22			/* 0x0038	*/
#define HFC_IOHGSPC_INDACC3			23			/* 0x003c	*/

#define HFC_IOHGSPC_PARTSNUM0		24			/* 0x0040	*/
#define HFC_IOHGSPC_PARTSNUM1		25			/* 0x0044	*/
#define HFC_IOHGSPC_PARTSNUM2		26			/* 0x0048	*/
#define HFC_IOHGSPC_PARTSNUM3		27			/* 0x004c	*/

#define HFC_IOHGSPC_SYSREV0			28			/* 0x0050	*/
#define HFC_IOHGSPC_SYSREV1			29			/* 0x0054	*/
#define HFC_IOHGSPC_SYSREV2			30			/* 0x0058	*/
#define HFC_IOHGSPC_SYSREV3			31			/* 0x005c	*/

#define HFC_IOHGSPC_ADAPID0			32			/* 0x0060	*/
#define HFC_IOHGSPC_ADAPID1			33			/* 0x0064	*/
#define HFC_IOHGSPC_ADAPID2			34			/* 0x0068	*/
#define HFC_IOHGSPC_ADAPID3			35			/* 0x006c	*/

#define HFC_IOHGSPC_HVM_SUPPORT		36			/* 0x0070	*//* FCLNX-GPL-140*/
#define HFC_IOHGSPC_DRV_SUPPORT		37			/* 0x0074	*//* FCLNX-GPL-140*/

#define HFC_IOHGSPC_VPDAREA			40			/* 0x0080	*/
#define HFC_IOHGSPC_INDAREA			41			/* 0x0100	*/

#define HFC_IOHGSPC_LDS_LIMIT		42			/* 0x0180	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_LDS_COUNT		43			/* 0x0182	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_FCIF_LIMIT		44			/* 0x0184	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_FCIF_COUNT		45			/* 0x0186	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_TO_LIMIT		46			/* 0x0188   *//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_TO_COUNT		47			/* 0x018a	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_RST_LIMIT		48			/* 0x018c	*//* FCLNX-GPL-393 */
#define HFC_IOHGSPC_RST_COUNT		49			/* 0x018e	*//* FCLNX-GPL-393 */

#define HFC_IOHGSPC_EFI_INFO0		50			/* 0x01f0	*/
#define HFC_IOHGSPC_EFI_INFO1		51			/* 0x01f4	*/
#define HFC_IOHGSPC_EFI_INFO2		52			/* 0x01f8	*/
#define HFC_IOHGSPC_EFI_INFO3		53			/* 0x01fc	*/
#define HFC_IOHGSPC_EFI_OP_TBL0		54			/* 0x0200	*/
#define HFC_IOHGSPC_EFI_OP_TBL1		55			/* 0x0204	*/

#define HFC_IOHGSPC_HGCCA_ADDR0		56			/* 0x0190	*//* FCLNX-GPL-494 */
#define HFC_IOHGSPC_HGCCA_ADDR1		57			/* 0x0194	*//* FCLNX-GPL-494 */
#define HFC_IOHGSPC_HGCCA_FLAG		58			/* 0x0198	*//* FCLNX-GPL-494 */

/* MMIO-HG Ver=0200 */
#define HFC_IOHGSPC_HYP_STATUS0		59			/* 0x0408	*/
#define HFC_IOHGSPC_CMD_REG0		60			/* 0x040c	*/
#define HFC_IOHGSPC_HYP_INTDETAIL0	61			/* 0x042c	*/
#define HFC_IOHGSPC_HYP_STATUS1		62			/* 0x0448	*/
#define HFC_IOHGSPC_CMD_REG1		63			/* 0x044c	*/
#define HFC_IOHGSPC_HYP_INTDETAIL1	64			/* 0x046c	*/
#define HFC_IOHGSPC_HYP_STATUS2		65			/* 0x0488	*/
#define HFC_IOHGSPC_CMD_REG2		66			/* 0x048c	*/
#define HFC_IOHGSPC_HYP_INTDETAIL2	67			/* 0x04ac	*/
#define HFC_IOHGSPC_HYP_STATUS3		68			/* 0x04c8	*/
#define HFC_IOHGSPC_CMD_REG3		69			/* 0x04cc	*/
#define HFC_IOHGSPC_HYP_INTDETAIL3	70			/* 0x04ec	*/

/*----------------------------------------*/
/*-- MMIO-HG register offset length		--*/
/*----------------------------------------*/
#define HFC_IOHGSPC_PARTSNUM_LEN	0x10
#define HFC_IOHGSPC_SYSREV_LEN		0x4
#define HFC_IOHGSPC_ADAPID_LEN		0x10
#define HFC_IOHGSPC_VPDAREA_LEN		0x80
#define HFC_IOHGSPC_INDAREA_LEN		0x80
#define HFC_IOHGSPC_EFI_OP_TBL_LEN	0x100

/*************************************/
/* MMIO-HG register settings */
/*************************************/
/* HyperStatus area */
#define HFC_HG_HYPSTATUS_ENABLE		0x80000000
#define HFC_HG_HYPSTATUS_MCK		0x40000000
#define HFC_HG_HYPSTATUS_REBOOT		0x20000000
#define HFC_HG_HYPSTATUS_AVAILABLE	0x10000000	/* FCLNX-GPL-427 */
#define HFC_HG_HYPSTATUS_MODE		0x08000000
#define HFC_HG_HYPSTATUS_FDISABLE	0x04000000
#define HFC_HG_HYPSTATUS_CSTPEND	0x02000000
#define HFC_HG_HYPSTATUS_MCKLOG		0x01000000
#define HFC_HG_HYPSTATUS_FMCK		0x00400000
#define HFC_HG_HYPSTATUS_FCSTP		0x00200000
#define HFC_HG_HYPSTATUS_FCSTP_IML	0x00100000
#define HFC_HG_HYPSTATUS_WAIT_ISOL		0x00080000 /* FCLNX-GPL-427 */
#define HFC_HG_HYPSTATUS_WAIT_RCV_ISOL	0x00040000 /* FCLNX-GPL-427 */
#define HFC_HG_HYPSTATUS_ISOL			0x00020000 /* FCLNX-GPL-427 */
#define HFC_HG_HYPSTATUS_ISOLCMD		0x00010000 /* FCLNX-GPL-427 */
#define HFC_HG_HYPSTATUS_ENABLE_RAMACC	0x00008000 /* FCLNX-GPL-FX-376 */
#define HFC_HG_HYPSTATUS_LOCKED_RAMACC	0x00000080 /* FCLNX-GPL-FX-376 */

/* HyperStatus for core */
#define HFC_HG_HYPSTATUS_CORE_ENABLE	0x80000000
#define HFC_HG_HYPSTATUS_CORE_FDISABLE	0x04000000	/* FCLNX-GPL-FX-438 */

/* FCLNX-GPL-427 */
#define HFC_HYPSTS_NORMAL_MASK	( \
	HFC_HG_HYPSTATUS_AVAILABLE  | HFC_HG_HYPSTATUS_MCK | \
	HFC_HG_HYPSTATUS_CSTPEND | HFC_HG_HYPSTATUS_FMCK|   \
	HFC_HG_HYPSTATUS_FCSTP   | HFC_HG_HYPSTATUS_FCSTP_IML | \
	HFC_HG_HYPSTATUS_WAIT_ISOL | HFC_HG_HYPSTATUS_WAIT_RCV_ISOL | \
	HFC_HG_HYPSTATUS_ISOL | HFC_HG_HYPSTATUS_ISOLCMD )

/* Hyper condition */ /* FCLNX-GPL-427 */
#define HFC_HYPCONDITION_NONE			0
#define HFC_HYPCONDITION_NORMAL			1
#define HFC_HYPCONDITION_MCK			2
#define HFC_HYPCONDITION_CSTP			3
#define HFC_HYPCONDITION_ISOL			4
#define HFC_HYPCONDITION_WAIT_ISOL		5
#define HFC_HYPCONDITION_WAIT_ISOLRCV	6

/* HyperIntDetail area */ /* FCLNX-GPL-427 */
#define HFC_HG_HYPINTDET_FMCK			0x80000000
#define HFC_HG_HYPINTDET_FCSTP			0x40000000
#define HFC_HG_HYPINTDET_MCK			0x20000000
#define HFC_HG_HYPINTDET_MCK_END		0x10000000
#define HFC_HG_HYPINTDET_CSTP_END		0x08000000
#define HFC_HG_HYPINTDET_LINKEND		0x04000000
#define HFC_HG_HYPINTDET_F_ISOLERR		0x00800000
#define HFC_HG_HYPINTDET_F_ISOLCMD		0x00400000
#define HFC_HG_HYPINTDET_F_ISOL_END		0x00200000
#define HFC_HG_HYPINTDET_RCV_ISOL		0x00100000
#define HFC_HG_HYPINTDET_RCV_ISOL_END	0x00080000
#define HFC_HG_HYPINTDET_MIG_END		0x00040000 /* FCLNX-GPL-489 */
#define HFC_HG_HYPINTDET_MIG_RCV		0x00020000 /* FCLNX-GPL-489 */

#define HFC_HG_HYPINTDET_GUEST_SUPPORT  ( \
	HFC_HG_HYPINTDET_FMCK | HFC_HG_HYPINTDET_FCSTP | HFC_HG_HYPINTDET_MCK | \
	HFC_HG_HYPINTDET_MCK_END | HFC_HG_HYPINTDET_CSTP_END | \
	HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD | \
	HFC_HG_HYPINTDET_F_ISOL_END | HFC_HG_HYPINTDET_RCV_ISOL | HFC_HG_HYPINTDET_RCV_ISOL_END | \
	HFC_HG_HYPINTDET_MIG_END | HFC_HG_HYPINTDET_MIG_RCV )/* FCLNX-GPL-489 */

#define HFC_HG_HYPINTDET_GUEST_SUPPORT_FX  ( \
	HFC_HG_HYPINTDET_FMCK | HFC_HG_HYPINTDET_FCSTP | HFC_HG_HYPINTDET_MCK | \
	HFC_HG_HYPINTDET_MCK_END | HFC_HG_HYPINTDET_CSTP_END | HFC_HG_HYPINTDET_LINKEND | \
	HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD | \
	HFC_HG_HYPINTDET_F_ISOL_END | HFC_HG_HYPINTDET_RCV_ISOL | HFC_HG_HYPINTDET_RCV_ISOL_END | \
	HFC_HG_HYPINTDET_MIG_END | HFC_HG_HYPINTDET_MIG_RCV )/* FCLNX-GPL-489 */

/* LPAR Status area */
#define HFC_HG_LPRSTATUS_UNSHARABLE		0x80000000
#define HFC_HG_LPRSTATUS_LINKDOWN		0x40000000
#define HFC_HG_LPRSTATUS_ISVALID		0x20000000
#define HFC_HG_LPRSTATUS_ISOLSUPPRT		0x08000000				/* FCLNX-GPL-393 */
#define HFC_HG_LPRSTATUS_MPID_ENABLE	0x00800000				/* FCLNX-GPL-393 */
#define HFC_HG_LPRDETAIL_SPACE			0x0000ff00              /* FCLNX-0353 */
#define HFC_HG_LPRDETAIL_PRIVATE		0x00002000
#define HFC_HG_LPRDETAIL_FCAL			0x00003000
#define HFC_HG_LPRDETAIL_NO_NPIV		0x00004000              /* FCLNX-0353*/
#define HFC_HG_LPRDETAIL_WWN_INVALID	0x00000100              /* FCLNX-xxxx */
#define HFC_HG_LPRDETAIL_SHADOW_DED		0x00000200              /* FCLNX-xxxx */
#define HFC_HG_LPRSTATUS_SFP_FAIL		0x00000300 				/* FCLNX-GPL-489 */
#define HFC_HG_LPRSTATUS_SFP_NOTSUPT	0x00000400 				/* FCLNX-GPL-489 */
#define HFC_HG_LPRSTATUS_SFP_DOWN		0x00000500 				/* FCLNX-GPL-489 */
/* LPAR Status area for FIVE-EX */
#define HFC_HG_LPRDETAIL_FCSW			0x00008000
#define HFC_HG_LPRDETAIL_NPIV			0x00004000
#define HFC_HG_LPRDETAIL_FCSWPTOP		0x00002000				/* FCLNX-GPL-FX-386 */
#define HFC_HG_LPRALPACNT_SPACE			0x000000ff
#define HFC_HG_LPRDETAIL_FPORT			0x00801000				/* FCLNX-GPL-FX-386 */
#define HFC_HG_LPRDETAIL_MULTIALPA		0x00803000				/* FCLNX-GPL-FX-386 */
#define HFC_HG_CONNECTIONTYPE_MASK		0x0080f000				/* FCLNX-GPL-FX-386 */

/* LPAR HVM/Driver Supoprt Bit */ /* FCLNX-GPL-140 */
#define HFC_HG_FIVE_EX_SUPPORT			0x80000000
#define HFC_HG_FIVE_FX_SUPPORT			0x00800000				/* FCLNX-GPL-FX-120 */
#define HFC_HG_MULTI_ALPA_SUPPORT		0x40000000
#define HFC_HG_GUEST_FWUPDATE_SUPPORT	0x20000000
#define HFC_HG_LPAR_MIGRATION_SUPPORT	0x10000000				/* FCLNX-GPL-306 */
//#define HFC_HG_LPAR_ISOLATION_SUPPORT	0x08000000				/* FCLNX-GPL-349 This bit must not be used. */
#define HFC_HG_LPAR_ISOLATION_SUPPORT	0x04000000				/* FCLNX-GPL-349 */
#define HFC_HG_LPAR_LIVEMIG_SUPPORT		0x02000000				/* FCLNX-GPL-489 */
#define HFC_HG_LPAR_STATISTICS_SUPPORT  0x01000000   			/* FCLNX-GPL-494 */
#define HFC_HG_SUPPORT_COREDEDICATE		0x00400000				/* FCLNX-GPL-FX-438 */
																/* @MLPF END */
/* MMIO-HG command register */
#define HFC_MLPF_FFMCK              0x20100000
#define HFC_MLPF_FFCSTP             0x20110000
#define HFC_MLPF_FFCSTP_IML         0x20120000
#define HFC_MLPF_CSTPEND            0x20130000
#define HFC_MLPF_F_ISOLATE_ERR      0x20140000	/* FCLNX-GPL-393 */
#define HFC_MLPF_F_ISOLATE_CMD      0x20150000	/* FCLNX-GPL-393 */
#define HFC_MLPF_RECOV_ISOLATE      0x20160000	/* FCLNX-GPL-393 */
#define HFC_MLPF_RECOV_ISOL_END     0x20170000	/* FCLNX-GPL-393 */
#define HFC_MLPF_F_ISOLATE_END      0x20180000	/* FCLNX-GPL-427 */
#define HFC_MLPF_SET_INDACC         0x20200000
#define HFC_MLPF_RESET_INDACC       0x20210000

#define HFC_MLPF_INDACC_FLG         0x80000000
#define HFC_MLPF_INDACC_ADR         0x0000FFFF
/*************************************/
/* FCP 								 */
/*************************************/
#define HFC_DIR_SERV_ID 0x00FFFFFC 		/* Directory Server(Name Server)*/

/*************************************/
/* return code						 */
/*************************************/
/*-- hfc_cancel_scsi_cmd() --*/
#define HFC_FLASH_TARGET		0x1		/* cancel for target 			*/
#define HFC_FLASH_DEV			0x2		/* cancel for lun				*/
#define HFC_FLASH_PACKET		0x3		/* cancel for packet			*/
#define HFC_FLASH_ADAP			0x4		/* cancel for HBA port			*//* FCLNX-GPL-FX-228,272 */


#define HFC_CSCSI_ERROR			0		/* FCLNX-0429 */
#define HFC_CSCSI_INHALT		1		/* FCLNX-0429 */
#define HFC_CSCSI_RESET			2		/* FCLNX-0429 */

/*-- hfc_resource_chk() --*/
#define HFC_XOB_EMPTY		1			/* success (xob expty)			*/
#define HFC_XOB_FULL		4			/* xob full 					*/
#define HFC_SCMD_FULL		5			/* scmd_buf full				*/
#define HFC_IOVMAP_FULL		6			/* DMA mapping Bus address full */
#define HFC_DMA_MAX_OVER	7			/* DMA Size Over				*/
#define	HFC_PAGE_OVER		-1			/* PAGE CNT Over..return(EIO)	*/


/************************************************************************/
/* struct fw_init_tbl													*/
/*		  f/w interface table 											*/
/************************************************************************/
struct fw_init_tbl {
	ushort				max_exchange ;			/* +0  max exchange num	*/
	ushort				max_port ;				/* +2  max target num	*/
	uchar				init_tbl_rev ;			/* +3  init_table rev	*/	/* @hbaapi */
#define	HFC_FWINIT_REV_1	0x01
	uchar				dd_support_info ;		/* +5 drv support info	*/
#define	HFC_DDSP_OPTERR9E	0x80
#define HFC_DDSP_LINKUP_DELAY	0x08			/* FCLNX-GPL-570		*/
#define HFC_PORTID_GUARD_CTL	0x04			/* FCLNX-GPL-FX-366 	*/
	uchar				resv0[10] ;
/*	uchar				resv0[11] ; */
/*	uchar				resv0[12] ; */										/* @hbaapi */
	uint64_t			next ;					/* +10 next(not used)	*/
	uint64_t			mb_addr ;				/* +18 Mailbox address	*/
	ushort				xob_num ;				/* +20 xob num			*/
	ushort				resv1 ;
	ushort				xrb_num ;				/* +24 xrb num			*/
	ushort				resv2 ;
	ushort				slog_num ;				/* +28 soft log page num*/
	ushort				slog_len ;				/* +2A soft log length  */
	uchar				resv3[4] ;	 			/* +2C					*/
	uchar				func ; 					/* +30			   1.98 */
#define HFC_FWF_GPNID		0x80				/*	 support GPN_ID  	*/
#define HFC_FWF_HBAAPI		0x40				/*	 support HBA API	*/
#define HFC_FWF_FIXLINK		0x20				/*	 support Link Speed */
												/*             /Topology*/
#define HFC_FWF_EXTPLOGI 	0x10				/*	 Extended PLOGI		*/
#define HFC_FWF_PORTRCV		0x04				/* support Port Recovery*/       /* FCLNX-0514 */
#define HFC_FWF_ONLINEUP	0x02				/* online update FCLNX-GPL-112 */
#define HFC_FWF_SFPINF		0x01				/*   SFP infomation     */	/* FCLNX_GPL-146 */
	uchar				func2 ; 				/* +31					*/  /* FCLNX-GPL-261 */
#define HFC_FWF_STATCCA		0x80				/*	 Statistics in CCA	*/  /* FCLNX-GPL-261 */
#define HFC_FWF_ISOLHVM		0x40				/*	 support Port Isolation	for Shared Mode */ /* FCLNX-GPL-393 */
	uchar				resv32[6] ;	 			/* +32					*/  /* FCLNX-GPL-261 */
	struct {									/* +38 fw link initialize */
		uchar			connect_type ;			/* see! adap_info.connect_type	*/
		uchar			trans_rate ;			/* +39 */
#define HFC_1GBPS	1							/* 100MB/s				*/
#define HFC_2GBPS	2							/* 200MB/s				*/
#define HFC_4GBPS	4							/* 400MB/s				*/
#define HFC_8GBPS	8							/* 800MB/s				*/
#define HFC_10GBPS	10							/* 1GB/s				*/
															/* @MLPF STR */
		uchar			npiv_valid;				/* +3A  for MLPF        */
#define HFC_MLPF_MODE       0x80
#define HFC_NPIV_VALID      0x40
															/* @MLPF END */
		uchar			flag ;					/* +3B */
#define	HFC_FABRIC_VALID 	0x80
#define	HFC_ALPA_VALID 		0x40
#define HFC_PID_VALID		0x20
#define	HFC_PNAME_VALID		0x10
#define	HFC_NNAME_VALID		0x08
#define	HFC_P2P_PID_VALID	0x04
#define	HFC_POSMAP_VALID	0x02
#define	HFC_POSMAP_LISA		0x01
		uint			port_id ;				/* +3C */
		uint64_t		port_name ;				/* +40 */
		uint64_t		node_name ;				/* +48 */
		uint			p2p_port_id ;			/* +50 */
		uint			resv5 ;					/* +54 */
	}fw_iocinfo ;
	uchar				resv6[40];				/* +58 */
	uchar				pos_map[160] ;			/* +80 */
	uint				xob_inp ;				/* +120 */
	uint				xob_outp ;				/* +124 */
	uint				xrb_inp ;				/* +128 */
	uint				xrb_outp ;				/* +12c */
//	uchar				resv7[80] ; 			/* +130 */

	uchar				ios_mode_valid;			/* +130 */	/* FCLNX_GPL-146 */
	uchar				resv14[3];				/* +131 */
	uint				ios_mem_size;			/* +134 */	/* FCLNX_GPL-146 */
	uint64_t			ios_mem_addr;			/* +138 */	/* FCLNX_GPL-146 */

	struct { /* SFP Information  */							/* FCLNX_GPL-146 */
		uchar			sfp_status;             /* +140 SFP Status */
#define	HFC_SFP_INSTALL     0x80
#define	HFC_SFP_VALID       0x40
		uchar			resv1[15];              /* +141 - +150 */
		uchar			sfp_type_name[HFC_SFP_TYPE_NAME_LEN];      /* +150 ASCII  */
		uchar			sfp_serial_no[HFC_SFP_SERIAL_NO_LEN];      /* +160 ASCII  */
		uchar			sfp_date_code[HFC_SFP_DATE_CODE_LEN];      /* +170 ASCII  */
		uint			sfp_validation_code;    /* +178 Hex    */
		uchar			resv2[4];               /* +17C        */
	}sfp_info;												/* FCLNX_GPL-146 */

	struct fw_trc_info	trc_info ;
	uint64_t			fw_bus_addr[MAX_FW_BUS_CNT] ;
												/* +200 - +xxx */
												/* xob/xrb/slog queue   */
												/* address (page addr)	*/

//	uchar				resv8[256] ; 			/* +600 */
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

/* @hbaapi  */
	struct {									/* RNID info */
		uchar			resv9[4] ;				/* +700 - +704 */
		uchar			node_id_fmt ;			/* +704 Node Identification Data Format */
		uchar			com_nid_len ;			/* +705 Common Identification Data Format */
		uchar			resv10 ;				/* +706 */
		uchar			spec_nid_len ;			/* +707 Soecific Identification Data Format */
		uint64_t		n_port_name ;			/* +708 N_Port_Name */
		uint64_t		node_name ;				/* +710 Node_Name */
		uint64_t		vendor_unique[2] ;		/* +718 Vendor Unique */
		uint			node_type ;				/* +728 Associated Type */
		uint			port_number ;			/* +72c Physical Port Number */
		uint			num_att_nodes ;			/* +730 Number of Attached Nodes*/
		uchar			node_mgmt ;				/* +734 Node Management */
		uchar			ip_version ;			/* +735 IP Version */
		ushort			udp_port ;				/* +736 UDP/TCP Port Number */
		uint64_t		ip_addr[2] ;			/* +738 IP Address */
		uchar			resv11[2] ;				/* +748 */
		ushort			disc_flags ;			/* +74a Vendor Specific */
		uchar			resv12[4] ;				/* +74c - +74f */
	}fw_rnid ;
	uchar				resv13[48] ;			/* +750 - +77f */
/* @hbaapi  */
};

/************************************************************************/
/* struct seg_info														*/
/*		  data segment infomation										*/
/************************************************************************/
struct seg_info {
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
/* struct xob															*/
/*		  xob format													*/
/************************************************************************/
/*
 * FCP-LUN structure
 */
typedef struct fcp_lun {
//	uchar	rsv0 ;					/* reserved (byte0)		*/
	ushort	lun ;					/* lun#		(byte1)		*//* FCLNX-GPL-0343 */
	uchar	rsv2_7[6] ;				/* reserved (byte2-7)	*/
} fcp_lun_t ;

/*
 * FCP-CNTL structure
 */
typedef struct fcp_cntl {
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
} fcp_cntl_t;

/*
 * FCP-CMD-IU Payload
 */
typedef struct fcp_cmd_iu {
	fcp_lun_t	fcp_lun;		/* FCP-LUN	*/
	fcp_cntl_t	fcp_cntl;		/* FCP-CNTL */
	uchar		fcp_cdb[16];	/* FCP-CDB	*/
	int			fcp_dl;			/* FCP-DL	*/
} fcp_cmd_iu_t;

typedef struct {
		uint64_t 		hfc_pkt ;		/* hfc_pkt pointer	*/
		ushort			target_id ;
		ushort			lun_id ;						  /* FCWIN-0153 */
		ushort			req_no ;		/* request#          FCWIN-0189 */
		//ushort			rsv    ;		/* rev               FCWIN-0189 */
		uchar			rid;            /* FCLNX-0371 */
		uchar           rsv;
} drv_used_t ;

struct xob {
	uchar				flag ;
#define HFC_XOB_VALID		0x80	/* =1 xob queue						*/
									/* =0 reset/cancel/DID_NOT			*/
	uchar				resv0[3] ;
	uchar				skip ;
#define HFC_XOB_SKIP		0x80	/* =0 xob queue						*/
									/* =0 reset/cancel/DID_NOT			*/
	uchar				resv1 ;
	uchar				bflag ;
	uchar				cflag ;
	drv_used_t			drv_work ;	/* driver work area 				*/
									/*  copy xrb						*/
	uint				d_info ;	/* DEST_INFO  +0  : AL_PA			*/
									/*			  +1-3: Port_ID(SCSI_ID)*/
	uchar				pseq ;		/* copy target_info.pseq			*/
	uchar				resv2 ;
	ushort				class ;
	uint64_t			seg_1st ;	/* first seg_info bus address		*/
	uint				seg_cnt ;	/* seg_info num 					*/
									/*   only HFC_SEG_LEN_F=0			*/
	uint				burst_len ;
	uchar				resv3[16] ;
	fcp_cmd_iu_t		fcp_cmd ;
	struct seg_info		seg_info_xob[2] ;	/* 1st/2nd seg_info copy	*/
};

/************************************************************************/
/* struct xrb															*/
/*		  xrb format													*/
/************************************************************************/
struct xrb {
	struct {
		uint			resv0 ;
		uint			resv1 ;
		uchar			fcp_status0 ;	/* Not used 					*/
		uchar			fcp_status1 ;	/* Not used 					*/
		uchar			fcp_status2 ;	/* FCP_STATUS					*/
#define HFC_XRB_RESID_UNDER 	0x08  	/* Data underrun				*/
#define HFC_XRB_RESID_OVER		0x04  	/* Data overrunn				*/
#define HFC_XRB_SNSLEN_VALID	0x02  	/* sense_length valid			*/
#define HFC_XRB_RSPLEN_VALID	0x01  	/* response_length valid		*/
		uchar			scsi_status ;	/* see! scsi_status				*/
		uint			resid ;			/* remain count					*/
		uint			sns_len ;		/* Sense Length					*/
		uint			resp_len ;		/* resp_info length			*/
		uchar			resp_info[360]; /* Response infomation			*/
										/* driver chack +3byte.			*/
	}resp_iu  ;
	uchar				flag ;
#define HFC_XRB_VALID		0x80		/* =0 xob dequeue				*/
	uchar				xcc ;
#define HFC_XCC_END			0x80		/* Normal end					*/
#define HFC_XCC_BUSY		0x82		/* Temp_Busy					*/
#define HFC_XCC_DEAD		0x83		/* Dead							*/
	uchar				esw ;
#define HFC_ESW_MEINT_REQ	0x80		/* req logout (valid FSB)		*/
#define HFC_ESW_MEINT_REPO	0x08		/* get soft_log (valid SSN)		*/
	uchar				resv2 ;
	uchar				skip ;
#define HFC_XRB_SKIP		0x80		/* =0 xob dequeue				*/
	uchar				resv3[3];
	uchar				ssn ;			/* page# in soft_log area 		*/
	uchar				son ;			/* offset in page				*/
	ushort				sbc ;			/* soft_log length				*/
	uchar				fsb ;			/* FPP status Byte				*/
#define HFC_FSB_IL			0x40		/* IL							*/
#define HFC_FSB_PC			0x20		/* PC							*/
#define HFC_FSB_CDC			0x08		/* CDC							*/
#define HFC_FSB_CCC			0x04		/* CCC							*/
#define HFC_FSB_ICC			0x02		/* ICC							*/
#define HFC_FSB_MASK		0x2E		/* PC/CDC/CCC/ICC				*/
	uchar				err_code[3] ;
#define HFC_ICC_TRANSPORT_FAULT	0x02
#define HFC_ICC_TIMEOUT			0x03
#define HFC_ICC_NO_RESP			0x04
	drv_used_t			drv_work ;		/* driver work area				*/
										/*  from xob					*/
	uchar				resv4[96];
};


/************************************************************************/
/* Mailbox Structure													*/
/************************************************************************/
/************************************************************************/
/* PORT_ID(SCSI_ID)														*/
/************************************************************************/
struct FS_ACC {
	uchar  rev;
	uchar  in_id[ 3 ];
	uchar  fs_type;
	uchar  fs_sub;
	uchar  options;
	uchar  resv0;
	ushort com_rsp_code;
	ushort max_res_size;
	uchar  resv1;
	uchar  reason;
	uchar  rsn_exp;
	uchar  vender;
	uint   port_id[ 508 ];
	uint   rsv9[ 512 ];					/* aline page baundary */
};


/************************************************************************/
/* struct mb_cmd1/mb_rsp1												*/
/*		  mailbox initiate(DRVIOCTL1)									*/
/*		  (1) LOGIN														*/
/*		  (2) ELS(PRLO/GCS_ID,PDISC,GID_PN) 							*/
/************************************************************************/

struct mailbox {
	struct initiate {
		/* command code */
		uchar command;
#define HFC_MBCMD_LOGTRACE		0x80
#define HFC_MBCMD_FPPCTL		0x90
#define HFC_MBCMD_MAINT			0xa0
#define HFC_MBCMD_FCCTL			0xb0
		/* sub-command code */
		uchar sub_cmd;
#define HFC_MBSCMD_EVENT		0x00
#define HFC_MBSCMD_LINKINIT 	0x00
#define HFC_MBSCMD_SETFWTR		0x04
#define HFC_MBSCMD_CTLPORTSTAT	0x07	/* @hbaapi */
#define HFC_MBSCMD_CHNGRNID		0x08	/* @hbaapi */
#define HFC_MBSCMD_MIHLOG		0x15
#define HFC_MBSCMD_DIAG			0x16
#define HFC_MBSCMD_LOGIN		0x20
#define HFC_MBSCMD_LDCHTRC		0x20
#define HFC_MBSCMD_SMEINT		0x21
#define HFC_MBSCMD_FORCSLOG		0x22
#define HFC_MBSCMD_PRLO			0x25
#define HFC_MBSCMD_GCSID		0x22
#define HFC_MBSCMD_PDISC		0x23
#define HFC_MBSCMD_GIDPN		0x24
#define HFC_MBSCMD_GPNID		0x26	/* @dyntrk */
#define HFC_MBSCMD_NMSRV		0x40
#define HFC_MBSCMD_SNDRCV		0x60
#define HFC_MBSCMD_INITMDSET	0x80
#define HFC_MBSCMD_FCPMDSET		0x81
		ushort dependent_code;
		uchar  pseq_no;
		uchar  resv0[ 3 ];
		union {
			/* DRVCTL */
			struct drv_ctl{
				uchar resv0[ 8 ];
				union {
					/* link-initialize */
					struct {
						uchar al_pa;
						uchar link_speed;	/* FCWIN-0081 */
						uchar connect_type;	/* FCWIN-0081 */
						uchar link_ini_opt; /* FCLNX-GPL-570 */
#define HFC_DISABLE_LINKINI_DELAY 0x80		/* FCLNX-GPL-570 */
						uchar resv0[ 40 ];	/* FCWIN-0081 *//* FCLNX-GPL-570 */
					} link_init;
					struct {
						uchar magic_num[8];
						uchar mode[3];
#define HFC_INITMDSET_FCP	 0x000000
#define HFC_INITMDSET_FCPIOS 0x000101
#define HFC_INITMDSET_WRAP	 0x000201
						uchar al_pa;
						uint  connect_type;
						uint64_t wwpn;
						uint64_t wwnn;
						uint buf_size;
						uint xrdy_div;
						uint param;
					} init_mode_set;
					struct {
						uint act_ctl;
						uint il_ctl;
						uint seq_ctl;
						uint rsp_delay;
						uchar resv[ 28 ];
					} fcp_mode_set;
				} un;
			} drvctl;
			/* DRVMAINT */
			struct drv_maint{
				uchar resv0[ 52 ];
			} drvmaint;
			/* DRVIOCTL0 */
			struct drv_ioctl0{
				uchar resv0[ 8 ];
				union {
					/* Mailbox(event) */
					struct {
						uchar 		resv0[ 4 ];
						uint 		event_map;
						uint64_t	event_address;
						uchar		resv[28];
					} mbevent;
				} un;
			} drvioctl0;
			/* DRVIOCTL1 */
			struct drv_ioctl1{
				union {
					uchar al_pa;
					uint d_id ;
				}adr;
				ushort payload_length;
				uchar resv1[ 2 ];
				union {
					/* login */
					struct login{
						uchar prli;
						uchar pagelen;
						ushort payload_length;
						uchar fscsi;
						uchar extype;
						ushort flag;
						uint orig_proc_assoc;
						uint resp_proc_assoc;
						uint parameter ;
#define TASK_RETRY_IDENTIFICATION_REQUESTED ( 0x00000200 )
#define RETRY ( 0x00000100 )
#define CONFIRMED_COMPLETION_ALLOWED ( 0x00000080 )
#define DATA_OVERLAY_ALLOWED ( 0x00000040 )
#define INITIATOR_FUNCTION ( 0x00000020 )
#define TARGET_FUNCTION ( 0x00000010 )
#define READ_FCP_XFER_RDY_DISABLED ( 0x00000002 )
#define WRITE_FCP_XFER_RDY_DISABLED ( 0x00000001 )
						uchar resv[ 4 ];
					} login;
					/* prlo */
					struct els_prlo{
						uchar prlo;
						uchar pagelen;
						ushort payload_length;
						uchar fscsi;
						uchar extype;
						ushort flag;
						uint orig_proc_assoc;
						uint resp_proc_assoc;
						uint parameter;
						uchar resv[ 4 ];
					} prlo;
					/* gcs_id */
					struct fc_gcs_id{
						uint  port_id ;
						uchar resv[ 20 ];
					} gcs_id;
					/* gid_pn */
					struct qwwn{
						uchar rev;
						uchar in_id[ 3 ];
						uchar fs_type;
						uchar fs_sub;
						uchar options;
						uchar resv0;
						ushort com_rsp_code;
						ushort max_res_size;
						uchar resv1;
						uchar reason;
						uchar rsn_exp;
						uchar vender;
						uint64_t port_name;
					} gid_pn;
					/* gpn_id */
					struct fc_gpn_id{
						uchar rev;
						uchar in_id[ 3 ];
						uchar fs_type;
						uchar fs_sub;
						uchar options;
						uchar resv0;
						ushort com_rsp_code;
						ushort max_res_size;
						uchar resv1;
						uchar reason;
						uchar rsn_exp;
						uchar vender;
						uint  port_id;
						uchar resv[ 4 ];
					} gpn_id;
				} un;
				uchar resv2[ 20 ];
			} drvioctl1;
			/* DRVIOCTL2 */
			struct drv_ioctl2{
				union {
					uchar al_pa;
					uint d_id ;
				}adr;
				ushort payload_length;
				uchar resv1[ 2 ];
				uchar flag;
				uchar resv2[ 3 ];
				uint response_length;
				uint64_t response_list_address;
				union {
					/* gid_ft */
					struct nmsrv{
						uchar rev;
						uchar in_id[ 3 ];
						uchar fs_type;
						uchar fs_sub;
						uchar options;
						uchar resv0;
						ushort com_rsp_code;
						ushort max_res_size;
						uchar resv1;
						uchar reason;
						uchar rsn_exp;
						uchar vender;
						uchar resv2;
						uchar domain;
						uchar area;
						uchar type;
						uchar resv3[ 8 ];
					} gid_ft;
				} un;
			} drvioctl2;
			/* DRVIOCTL3 */
			struct drv_ioctl3{
				uchar resv1[ 8 ];			/* @hbaapi */

				uint64_t send_address;
				uint64_t receive_address;
				uchar resv2[ 28 ];
			} drvioctl3;
			struct drv_logb0{
				union{
					struct{
						drv_used_t drv_work;
						uchar resv0[8];
					}mih_log;
					uchar resv0[ 32 ] ;
				}uni;
				uchar resv[ 28 ];
			} drvlogb0 ;	/* @030131 */
			struct drv_logb1{
				uchar resv0[16] ;
				uint64_t log_list_addr ;
				uchar resv[ 28 ];
			} drvlogb1 ;
			/* DRVLOOP */
			struct drv_loop{
				uchar resv0[16] ;
				uchar resv1[ 36 ];
			} drvloop ;
			/*--- size control ---*/
			struct
			{
				uchar resvx[504];
			};
		} type;
	} mb_init;
	/*---- mailbox response -----------------------------------*/
	struct response{
		uchar				flag ;			/* see ! xrb				*/
		uchar				xcc ;			/* see ! xrb				*/
		uchar				esw ;			/* see ! xrb				*/
		uchar				resv0 ;			/* see ! xrb				*/
		uchar				resv1[4];		/* see ! xrb				*/
		uchar				ssn ;			/* soft_log page# 			*/
		uchar				son ;			/*          offset in page  */
		ushort				sbc ;			/*          byte count 		*/
		uchar				fsb ;			/* FPP status Byte			*/
		uchar				err_code[3] ;
		union {
			union {
				struct {
					uchar command;
					uchar sub_cmd;
					ushort	resv2;
				}cmd ;
			}respcmd ;
			/* DRVIOCTL1 */
			union {
				struct {
					ushort	cmd ;					/* FCWIN-0182 */
					ushort	dependent_code;			/* FCWIN-0182 */
					ushort	max_frame_size ;
					uchar	resv0 ;
					uchar	class_mask ;
					uint64_t port_name ;
					uint64_t node_name ;
					uchar	ls_acc ;
					uchar pagelen;
					ushort payload_length;
					uchar fscsi;
					uchar extype;
					ushort flag;
					uint orig_proc_assoc;
					uint resp_proc_assoc;
					uint parameter;
					ushort send_frame_size; /* FCLNX-GPL-261 */
					uchar resv1[2];         /* FCLNX-GPL-261 */
				} login;
				struct {
					uint	cmd ;
					uint	resv0 ;
					uint64_t port_name ;
					uint64_t node_name ;
					uchar	ls_acc ;
					uchar pagelen;
					ushort payload_length;
					uchar fscsi;
					uchar extype;
					ushort flag;
					uint orig_proc_assoc;
					uint resp_proc_assoc;
					uchar resv1[ 4 ];
				} prlo;
				struct {
					uint	cmd ;
					uint	class ;
				}gcs_id ;
				struct {
					uint	cmd ;
					uint	resv0 ;
					uint64_t port_name ;
					uint64_t node_name ;
				}pdisc ;
				struct {
					uint	cmd ;
					uint port_id;
				}gid_pn;
				struct {
					uint	cmd ;
					uint port_name_hi ;
					uint port_name_lo ;
				}gpn_id ;
			} drvioctl1;
			/* DRVIOCTL2 */
			union {
				struct {
					uint	cmd ;
					uint port_number;
				} gid_ft;
			} drvioctl2;
			union {
				struct {
					uint		cmd ;

					uchar		resv0[4];
					drv_used_t	drv_work ;
					uchar		ssn ;
					uchar		son ;
					ushort		sbc ;
					uchar		resv1[4] ;
				}mih_log ;				/* @030131 */
				struct {
					uint	cmd ;
					uchar	reslt ;
					uchar	resv0[3] ;
				}diag ;
				struct {
					uint	cmd ;
					uchar	resv0[12] ;
					uint64_t	tx_frames ;
					uint64_t	tx_words ;
					uint64_t	rx_frames ;
					uint64_t	rx_words ;
					uint64_t	lip_count ;
					uint64_t	nos_count ;
					uint64_t	error_frames ;
					uint64_t	dumped_framed ;
					uint64_t	link_failure_count ;
					uint64_t	loss_of_sync_count ;
					uint64_t	loss_of_signal_count ;
					uint64_t	primitive_seq_protocol_err_count ;
					uint64_t	invalid_tx_word_count ;
					uint64_t	invalid_crc_count ;
				}control_portstatistics ;
			} drvlogb0 ;
			union {
				struct {
					struct {
						uint	cmd;
						uint	resv0;
						struct seg_no {
							ushort	sp;
							ushort	ep;
							uchar	ctl;
							uchar	resv0;
							ushort	cp;
						}seg_ptr[4];
					}trc_ptr;
					struct seg_no frm_trc_ptr;
				}ldch_trc_ptr;
				struct {
					uint	cmd;
				}smint_ptr;
			} drvlogb1 ;
			/* size control */
			struct {
				uchar resvx[ 496 ];
			};
		} type;
	} mb_resp;
/************************************************************************/
/*		  mailbox int_req(FWINTREQ0)									*/
/*		  (1) SCN/RSCN/LOGO/PLOGI 										*/
/*		  (2) LinkUp/LinkDown											*/
/************************************************************************/
	/* mailbox interrupt */
	struct interrupt {
		union {
			struct {
				uchar	int_code ;
#define HFC_MBINT0			0xb0
#define HFC_MBINT2 			0xa0
				uchar	sub_int_code ;
#define	HFC_MBINT_SCN		0x01
#define	HFC_MBINT_RSCN		0x02
#define	HFC_MBINT_LOGO		0x03
#define	HFC_MBINT_PLOGI		0x04
#define	HFC_MBINT_LINKUP	0x80
#define	HFC_MBINT_LINKDOWN	0x88
#define HFC_MBINT_REPO_ASYN_EVENT	0x10
#define HFC_MBINT_ONLINEUP	0x20				/* FIVE-EX */
				uchar	esw ;					/* see! xrb esw					*/
				uchar	resv0 ;
				ushort	detail ;
#define HFC_MCK_PCHK		0x2001				/* FCLNX_GPL-0109 */
#define HFC_SFP_NOT_SUPPORT 0xE001
#define HFC_SFP_RECV_ERR    0xE102
#define HFC_SFP_TRAN_ERR    0xE103
#define HFC_SFP_DOWN        0xE104
				ushort	resv1 ;
				uchar	ssn ;
				uchar	son ;
				ushort	sbc ;
				uchar	rfv ;
				uchar	resv2 ;
				ushort	length ;
				union {
					struct {
						uchar	resv0[ 496 ] ;
						uchar	els_type ;
						uchar	page ;
						ushort	pay_len ;
						uint	n_port_id[511] ;
					}scn ;
					struct {
						uchar	resv0[ 496 ] ;
						uchar	els_type ;
						uchar	page ;
						ushort	pay_len ;
						uint	n_port_id[511] ;
					}rscn ;
					struct {
						uchar	resv0[ 496 ] ;
						uchar	els_type ;
						uchar	page ;
						ushort	pay_len ;
						uint	n_port_id ;
						uint64_t port_name ;
					}logo ;
					struct {
						uint64_t port_name ;
						uint64_t node_name ;
					} plogi;
					struct {
						uchar	fsynd;
						uchar	far[3];
					} ce;
				}un ;
			} fwintreq0;
			struct {
				uchar	resv0[16] ;
			} fwintreq1;
			struct {
				uchar resvx[ 2560 ];
			};
		} type;
	} mb_intreq;
	struct intresponse {
		uchar	resv[512] ;
	}mb_intresp;
};


/************************************************************************/
/* VPD Area																*/
/************************************************************************/
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

/* vpd format for FIVE-FX */
struct hfc_vpd_five_fx {
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

/************************************************************************/	/* @MLPF STR */
/* MMIO-HG mem_space map												*/
/************************************************************************/
struct hg_map {
	struct {
		int reg[128];
	}iosp;
};
struct hg_cca {     /* FCLNX-GPL-494 Get Statistics for Virtage */
	uchar		version;
	uchar		valid;
#define HFC_FWSTATISTICS_VALID		0x80
	ushort		size;
	uint		rsv0;
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

#endif				/* !INCLUDE _H_HFCDDCOM */

