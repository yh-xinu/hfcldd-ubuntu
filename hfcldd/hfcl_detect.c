/*
 * hfcl_detect.c
 * Copyright (C) 2007, 2016, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char det_rcsid[] = "$Id: hfcl_detect.c,v 1.115.2.42.2.39.2.4.2.5.6.31.2.3.2.33.2.31.2.22.2.13.2.8.2.18.2.2 2016/02/19 03:05:27 mhayashi Exp $";

#include "hfcldd.h"
#include "hfcl_tbol.h"
#include "hfcl_strategy.h"
#include "hfcl_stra_trace.h"
#include "hfcl_timer_recovery.h"
#include "hfcl_ioctl.h"
#include "hfcl_top.h"
#include "hfcl_detect.h"
#include "hfcl_handler.h"
#include "hfcl_mlpf.h"
#include "hfcl_version.h"
#include "hfcmpcfg.h"
#include "hfcddwwn.h"
#include "hfcldd_conf.h"
#include "hfcl_hand_timer_trace.h"

#include "hfcldd_fx.h"
#include "hfcl_strategy_fx.h"
#include "hfcl_stra_trace_fx.h"
#include "hfcl_timer_recovery_fx.h"
#include "hfcl_ioctl_fx.h"
#include "hfcl_top_fx.h"
#include "hfcl_detect_fx.h"
#include "hfcl_handler_fx.h"
#include "hfcl_mlpf_fx.h"
#include "hfcl_hand_timer_trace_fx.h"
#include "hfcl_npiv_fx.h"

#ifndef _HFC_NO_RASLOG
#include "hraslog.h"
#endif

void structdump( int loc, uchar *p, int size );
#ifndef _HFC_DEBUG	/* FCLNX-0510	*/
	#define	STRUCTDUMP( LOC, PTR, SIZE ) { while(0) {}; }
#else
	#define STRUCTDUMP( LOC, PTR, SIZE ) { structdump( LOC, PTR, SIZE ); }
#endif			/* FCLNX-0510	*/


/* PCI memory space address map */
const struct pkg_map hfc_pkg_map[3] = {
	{/* FPP */
		{ 
		  /* IOSPACE map */	/* no : name		 			   size */
			{	0x0000,			/* 0  :  HFC_IOSPACE_ZERO			4B	*/
				0x0001, 		/* 1  :  HFC_IOSPACE_LSICODE		1B	*/
				0x0003, 		/* 2  :  HFC_IOSPACE_LSIREV			1B	*/
				0x0004, 		/* 3  :  HFC_IOSPACE_OFS4			4B	*/
				0x0005, 		/* 4  :  HFC_IOSPACE_PKCODE			1B	*/
				0x0007, 		/* 5  :  HFC_IOSPACE_PKREV			1B	*/
				0x0008, 		/* 6  :  HFC_IOSPACE_CHNO			1B	*/
				0x0010, 		/* 7  :  HFC_IOSPACE_STATUS0		4B	*/
				0x0014, 		/* 8  :  HFC_IOSPACE_STATUS1		4B	*/
				0x0018, 		/* 9  :  HFC_IOSPACE_ERRDETAIL0		4B	*/
				0x001C, 		/* 10 :  HFC_IOSPACE_ERRDETAIL1		4B	*/
				0x0020, 		/* 11 :  HFC_IOSPACE_INTA			4B	*/
				0x0024, 		/* 12 :  HFC_IOSPACE_MPINTAC		4B	*/
				0x0028, 		/* 13 :  HFC_IOSPACE_INTA_MSK		4B	*/
				0x002c, 		/* 14 :  HFC_IOSPACE_INTA_RST		4B	*/
				0x0040, 		/* 15 :  HFC_IOSPACE_CMDRES			1B	*/
				0x0041, 		/* 16 :  HFC_IOSPACE_CMDCTL			1B	*/
				0x0042, 		/* 17 :  HFC_IOSPACE_CMDBOOT		1B	*/
				0x0043, 		/* 18 :  HFC_IOSPACE_CMDLED			1B	*/
				0x0044, 		/* 19 :  HFC_IOSPACE_CMDFMEM		1B	*/
				0x0045, 		/* 20 :  HFC_IOSPACE_CMDFCIF		1B	*/
				0x0047, 		/* 21 :  HFC_IOSPACE_CMDMODE		1B	*/
				0x00F0, 		/* 22 :  HFC_IOSPACE_TRCA0			2B	*/
				0x00F2, 		/* 23 :  HFC_IOSPACE_TRCA1			2B	*/
				0x02F8, 		/* 24 :  HFC_IOSPACE_RAMMSK			1B	*/
				0x02FC, 		/* 25 :  HFC_IOSPACE_RAMADR			4B	*/
				0x0200, 		/* 26 :  HFC_IOSPACE_HTYP			1B	*/
				0x0201, 		/* 27 :  HFC_IOSPACE_OTYP			1B	*/
				0x0202, 		/* 28 :  HFC_IOSPACE_HWINF			1B	*/
				0x0203, 		/* 29 :  HFC_IOSPACE_LAPC			1B	*/
				0x0000, 		/* 30 :  HFC_IOSPACE_CMDSCAN		1B  */
				0x0000, 		/* 31 :  */
				0x0000, 		/* 32 :  */
				0x0000, 		/* 33 :  */
				0x0000, 		/* 34 :  */
				0x0000, 		/* 35 :  */
				0x0000, 		/* 36 :  */
				0x0000, 		/* 37 :  */
				0x0000, 		/* 38 :  */
				0x0000, 		/* 39 :  */
				0x0400, 		/* 40 :  HFC_IOSPACE_FRAMEA			4B	*/
				0x0500, 		/* 41 :  HFC_IOSPACE_INDAREA		4B	*/
				0x0600, 		/* 42 :  HFC_IOSPACE_INDAREA_P		4B	*/
				0x0800, 		/* 43 :  HFC_IOSPACE_SCANAREA		4B	*/
				0x0000, 		/* 44 :  */
				0x0000, 		/* 45 :  */
				0x0300, 		/* 46 :  HFC_IOSPACE_CA_POSTRESULT	4B	*/
				0x0304, 		/* 47 :  HFC_IOSPACE_CA_MPCHK_CODE	4B	*/
				0x030c, 		/* 48 :  HFC_IOSPACE_CA_FLAG		1B	*/
				0x0310, 		/* 49 :  HFC_IOSPACE_CA_INIT_ADDR0	4B	*/
				0x0314, 		/* 50 :  HFC_IOSPACE_CA_INIT_ADDR1	4B	*/
				0x0318, 		/* 51 :  HFC_IOSPACE_CA_RSTINFO		1B	*/
				0x0319, 		/* 52 :  HFC_IOSPACE_CA_ALPA		1B	*/
				0x031d, 		/* 53 :  HFC_IOSPACE_CA_PORTNO		1B  */
				0x031c, 		/* 54 :  HFC_IOSPACE_CA_POST		1B	*/
				0x0350,			/* 55 :  HFC_IOSPACE_CA_WWPN0		4B  */
				0x0354,			/* 56 :  HFC_IOSPACE_CA_WWPN1		4B  */
				0x0358,			/* 57 :  HFC_IOSPACE_CA_WWNN0		4B  */
				0x035c,			/* 58 :  HFC_IOSPACE_CA_WWNN1		4B  */
				0x03c0, 		/* 59 :  HFC_IOSPACE_DRV_USED		4B	*/
				0x03c4, 		/* 60 :  HFC_IOSPACE_DRV_USED1		4B	*/
				0x03c8, 		/* 61 :  HFC_IOSPACE_DRV_USED2		4B	*/
				0x03cc, 		/* 62 :  HFC_IOSPACE_DRV_USED3		4B	*/
				0x031a,			/* 63 :  HFC_IOSPACE_CA_LNKSPD		1B  */
				0x031b,			/* 64 :  HFC_IOSPACE_CA_CNTTYP		1B  */
				0x0000,			/* 65 :  */
				0x0000,			/* 66 :  */
				0x0000,			/* 67 :  */
				0x0000,			/* 68 :  */
				0x0000,			/* 69 :  */
				0x0000,			/* 70 :  */
				0x0000,			/* 71 :  */
				0x0000			/* 72 :  */
	  		}
		}
	},

	/* FIVE */
	{
		{
			/* IOSPACE map */	/* no : name		 			   size */
			{	0x0000,			/* 0  :  HFC_IOSPACE_ZERO			4B	*/
				0x0001, 		/* 1  :  HFC_IOSPACE_LSICODE		1B	*/
				0x0003, 		/* 2  :  HFC_IOSPACE_LSIREV			1B	*/
				0x0004, 		/* 3  :  HFC_IOSPACE_OFS4			4B	*/
				0x0005, 		/* 4  :  HFC_IOSPACE_PKCODE			1B	*/
				0x0007, 		/* 5  :  HFC_IOSPACE_PKREV			1B	*/
				0x0008, 		/* 6  :  HFC_IOSPACE_CHNO			1B	*/
				0x0010, 		/* 7  :  HFC_IOSPACE_STATUS0		4B	*/
				0x0014, 		/* 8  :  HFC_IOSPACE_STATUS1		4B	*/
				0x0018, 		/* 9  :  HFC_IOSPACE_ERRDETAIL0		4B	*/
				0x0000, 		/* 10 :  It is not possible to use it	*/
				0x00a0, 		/* 11 :  HFC_IOSPACE_INTA			4B	*/
				0x00a4, 		/* 12 :  HFC_IOSPACE_MPINTAC		4B	*/
				0x00a8, 		/* 13 :  HFC_IOSPACE_INTA_MSK		4B	*/
				0x00ac, 		/* 14 :  HFC_IOSPACE_INTA_RST		4B	*/
				0x0030, 		/* 15 :  HFC_IOSPACE_CMDRES			1B	*/
				0x0031, 		/* 16 :  HFC_IOSPACE_CMDCTL			1B	*/
				0x0032, 		/* 17 :  HFC_IOSPACE_CMDBOOT		1B	*/
				0x0038, 		/* 18 :  HFC_IOSPACE_CMDLED		(*)	4B	*/
				0x0034, 		/* 19 :  HFC_IOSPACE_CMDFMEM		1B	*/
				0x003C, 		/* 20 :  HFC_IOSPACE_CMDFCIF	(*)	1B	*/
				0x0037, 		/* 21 :  HFC_IOSPACE_CMDMODE		1B	*/
				0x00f0, 		/* 22 :  HFC_IOSPACE_TRCA0			2B	*/
				0x00f2, 		/* 23 :  HFC_IOSPACE_TRCA1			2B	*/
				0x02f8, 		/* 24 :  HFC_IOSPACE_RAMMSK			1B	*/
				0x02fC, 		/* 25 :  HFC_IOSPACE_RAMADR			4B	*/
				0x001c, 		/* 26 :  HFC_IOSPACE_HTYP			1B	*/
				0x001d, 		/* 27 :  HFC_IOSPACE_OTYP			1B	*/
				0x001e, 		/* 28 :  HFC_IOSPACE_HWINF			1B	*/
				0x001f, 		/* 29 :  HFC_IOSPACE_LAPC			1B	*/
				0x0036,			/* 30 :  HFC_IOSPACE_CMDSCAN		1B  */
				0x02fb, 		/* 31 :  HFC_IOSPACE_IDFLGEN		1B	*/
				0x0000, 		/* 32 :  */
				0x0000, 		/* 33 :  */
				0x0000, 		/* 34 :  */
				0x0000, 		/* 35 :  */
				0x0000, 		/* 36 :  */
				0x0000, 		/* 37 :  */
				0x0000, 		/* 38 :  */
				0x0000, 		/* 39 :  */
				0x0500, 		/* 40 :  HFC_IOSPACE_FRAMEA			4B	*/
				0x0600, 		/* 41 :  HFC_IOSPACE_INDAREA		4B	*/
				0x0700, 		/* 42 :  HFC_IOSPACE_INDAREA_P		4B	*/
				0x1000, 		/* 43 :  HFC_IOSPACE_SCANAREA		4B	*/
				0x0000, 		/* 44 :  */
				0x0000, 		/* 45 :  */
				0x0300, 		/* 46 :  HFC_IOSPACE_CA_POSTRESULT	4B	*/
				0x0304, 		/* 47 :  HFC_IOSPACE_CA_MPCHK_CODE	4B	*/
				0x030c, 		/* 48 :  HFC_IOSPACE_CA_FLAG		1B	*/
				0x0310, 		/* 49 :  HFC_IOSPACE_CA_INIT_ADDR0	4B	*/
				0x0314, 		/* 50 :  HFC_IOSPACE_CA_INIT_ADDR1	4B	*/
				0x0318, 		/* 51 :  HFC_IOSPACE_CA_RSTINFO		1B	*/
				0x0319, 		/* 52 :  HFC_IOSPACE_CA_ALPA		1B	*/
				0x031d, 		/* 53 :  HFC_IOSPACE_CA_PORTNO		1B	*/
				0x031c, 		/* 54 :  HFC_IOSPACE_CA_POST		1B	*/
				0x0350, 		/* 55 :  HFC_IOSPACE_CA_WWPN0		4B	*/
				0x0354, 		/* 56 :  HFC_IOSPACE_CA_WWPN1		4B	*/
				0x0358, 		/* 57 :  HFC_IOSPACE_CA_WWNN0		4B	*/
				0x035c, 		/* 58 :  HFC_IOSPACE_CA_WWNN1		4B	*/
				0x0360, 		/* 59 :  HFC_IOSPACE_DRV_USED		4B	*/
				0x0364, 		/* 60 :  HFC_IOSPACE_DRV_USED1		4B	*/
				0x0368, 		/* 61 :  HFC_IOSPACE_DRV_USED2		4B	*/
				0x036c, 		/* 62 :  HFC_IOSPACE_DRV_USED3		4B	*/
				0x031a, 		/* 63 :  HFC_IOSPACE_CA_LNKSPD		1B  */
				0x031b, 		/* 64 :  HFC_IOSPACE_CA_CNTTYP		1B  */
				0x031e, 		/* 65 :  */ /* FCLNX-GPL-112 */
				0x0370, 		/* 66 :  HFC_IOSPACE_FW_SUPPORT		1B  */
				0x0000, 		/* 67 :  */
				0x0000, 		/* 68 :  */
				0x0000, 		/* 69 :  */
				0x0000, 		/* 70 :  */
				0x0000, 		/* 71 :  */
				0x031f,			/* 72 :  HFC_IOSPACE_LINK_INI_OPT   1B  *//* FCLNX-GPL-FX-409 */
				0x0000  		/* 73 :  */
	  		}
		}
	},
	/* FIVE-EX */
	{  
		{
			/* IOSPACE map */ /* no : name                       size */
			{	0x0000,         /* 0  :  HFC_IOSPACE_ZERO           4B  */
				0x0001,         /* 1  :  HFC_IOSPACE_LSICODE        1B  */
				0x0003,         /* 2  :  HFC_IOSPACE_LSIREV         1B  */
				0x0004,         /* 3  :  HFC_IOSPACE_OFS4           4B  */
				0x0005,         /* 4  :  HFC_IOSPACE_PKCODE         1B  */
				0x0007,         /* 5  :  HFC_IOSPACE_PKREV          1B  */
				0x0008,         /* 6  :  HFC_IOSPACE_CHNO           1B  */
				0x0010,         /* 7  :  HFC_IOSPACE_STATUS0        4B  */
				0x0014,         /* 8  :  HFC_IOSPACE_STATUS1        4B  */
				0x0018,         /* 9  :  HFC_IOSPACE_ERRDETAIL0     4B  */
				0x0000,         /* 10 :  It is not possible to use it   */
				0x00a0,         /* 11 :  HFC_IOSPACE_INTA           4B  */
				0x00a4,         /* 12 :  HFC_IOSPACE_MPINTAC        4B  */
				0x00a8,         /* 13 :  HFC_IOSPACE_INTA_MSK       4B  */
				0x00ac,         /* 14 :  HFC_IOSPACE_INTA_RST       4B  */
				0x0030,         /* 15 :  HFC_IOSPACE_CMDRES         1B  */
				0x0031,         /* 16 :  HFC_IOSPACE_CMDCTL         1B  */
				0x0032,         /* 17 :  HFC_IOSPACE_CMDBOOT        1B  */
				0x0038,         /* 18 :  HFC_IOSPACE_CMDLED     (*) 4B  */
				0x0034,         /* 19 :  HFC_IOSPACE_CMDFMEM        1B  */
				0x003C,         /* 20 :  HFC_IOSPACE_CMDFCIF    (*) 4B  */
				0x0000,         /* 21 :  */
				0x00f0,         /* 22 :  HFC_IOSPACE_TRCA0          2B  */
				0x00f2,         /* 23 :  HFC_IOSPACE_TRCA1          2B  */
				0x02f8,         /* 24 :  HFC_IOSPACE_RAMMSK         1B  */
				0x02fC,         /* 25 :  HFC_IOSPACE_RAMADR         4B  */
				0x001c,         /* 26 :  HFC_IOSPACE_HTYP           1B  */
				0x001d,         /* 27 :  HFC_IOSPACE_OTYP           1B  */
				0x001e,         /* 28 :  HFC_IOSPACE_HWINF          1B  */
				0x001f,         /* 29 :  HFC_IOSPACE_LAPC           1B  */
				0x0000,         /* 30 :  */
				0x02fb,         /* 31 :  HFC_IOSPACE_IDFLGEN        1B  */
				0x0000,         /* 32 :  */
				0x0000,         /* 33 :  */
				0x0000,         /* 34 :  */
				0x0000,         /* 35 :  */
				0x0000,         /* 36 :  */
				0x0000,         /* 37 :  */
				0x0000,         /* 38 :  */
				0x0000,         /* 39 :  */
				0x0500,         /* 40 :  HFC_IOSPACE_FRAMEA         4B  */
				0x0600,         /* 41 :  HFC_IOSPACE_INDAREA        4B  */
				0x0700,         /* 42 :  HFC_IOSPACE_INDAREA_P      4B  */
				0x1000,         /* 43 :  HFC_IOSPACE_SCANAREA       4B  */
				0x0000,         /* 44 :  */
				0x0000,         /* 45 :  */
				0x0300,         /* 46 :  HFC_IOSPACE_CA_POSTRESULT  4B  */
				0x0304,         /* 47 :  HFC_IOSPACE_CA_MPCHK_CODE  4B  */
				0x030c,         /* 48 :  HFC_IOSPACE_CA_FLAG        1B  */
				0x0310,         /* 49 :  HFC_IOSPACE_CA_INIT_ADDR0  4B  */
				0x0314,         /* 50 :  HFC_IOSPACE_CA_INIT_ADDR1  4B  */
				0x0318,         /* 51 :  HFC_IOSPACE_CA_RSTINFO     1B  */
				0x0319,         /* 52 :  HFC_IOSPACE_CA_ALPA        1B  */
				0x031d,         /* 53 :  HFC_IOSPACE_CA_PORTNO      1B  */
				0x031c,         /* 54 :  HFC_IOSPACE_CA_POST        1B  */
				0x0350,         /* 55 :  HFC_IOSPACE_CA_WWPN0       4B  */
				0x0354,         /* 56 :  HFC_IOSPACE_CA_WWPN1       4B  */
				0x0358,         /* 57 :  HFC_IOSPACE_CA_WWNN0       4B  */
				0x035c,         /* 58 :  HFC_IOSPACE_CA_WWNN1       4B  */
				0x0360,         /* 59 :  HFC_IOSPACE_DRV_USED       4B  */
				0x0364,         /* 60 :  HFC_IOSPACE_DRV_USED1      4B  */
				0x0368,         /* 61 :  HFC_IOSPACE_DRV_USED2      4B  */
				0x036c,         /* 62 :  HFC_IOSPACE_DRV_USED3      4B  */
				0x031a,         /* 63 :  HFC_IOSPACE_CA_LNKSPD      1B  */
				0x031b,         /* 64 :  HFC_IOSPACE_CA_CNTTYP      1B  */
				0x031e, 		/* 65 :  */ /* FCLNX-GPL-112 */
				0x0370,         /* 66 :  HFC_IOSPACE_FW_SUPPORT		1B  */
				0x0000,         /* 67 :  */
				0x0000,         /* 68 :  */
				0x0000,         /* 69 :  */
				0x0884,         /* 70 :  HFC_IOSPACE_DUMP_CMD       4B  */
				0x083f,			/* 71 :  HFC_IOSPACE_KCMD_IPRES     1B  */
				0x031f,			/* 72 :  HFC_IOSPACE_LINK_INI_OPT   1B  *//* FCLNX-GPL-FX-409 */
				0x0000          /* 73 :  */
			}
		}
	}
};


/* AL-PA# */
const uchar posmap_lisa[128] = {
	0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x0f, 0x10,
	0x17, 0x18, 0x1b, 0x1d, 0x1e, 0x1f, 0x23, 0x25,
	0x26, 0x27, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x39, 0x3a,
	0x3c, 0x43, 0x45, 0x46, 0x47, 0x49, 0x4a, 0x4b,
	0x4c, 0x4d, 0x4e, 0x51, 0x52, 0x53, 0x54, 0x55,
	0x56, 0x59, 0x5a, 0x5c, 0x63, 0x65, 0x66, 0x67,
	0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x71, 0x72,
	0x73, 0x74, 0x75, 0x76, 0x79, 0x7a, 0x7c, 0x80,
	0x81, 0x82, 0x84, 0x88, 0x8f, 0x90, 0x97, 0x98,
	0x9b, 0x9d, 0x9e, 0x9f, 0xa3, 0xa5, 0xa6, 0xa7,
	0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xb1, 0xb2,
	0xb3, 0xb4, 0xb5, 0xb6, 0xb9, 0xba, 0xbc, 0xc3,
	0xc5, 0xc6, 0xc7, 0xc9, 0xca, 0xcb, 0xcc, 0xcd,
	0xce, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd9,
	0xda, 0xdc, 0xe0, 0xe1, 0xe2, 0xe4, 0xe8, 0xef
};


extern const char package_ver[16]; 		/* package version               */

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
struct scsi_transport_template *hfc_fc_attach_transport=NULL;
struct scsi_transport_template *hfc_vport_fc_attach_transport=NULL;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

extern struct file_operations hfc_fops;

const struct cr_PartsNumber cr_pn[] = {				/* FCLNX-0329 */
	{ 0x85, 1, 0, "3", "3", "-C", "HFC0401",   },	/* FIVE 2G 1Port */
	{ 0x85, 2, 0, "3", "2", "-B", "HFC0402",   },	/* FIVE 2G 2Port */
	{ 0x85, 4, 0, "3", "1", "-A", "HFC0404",   },	/* FIVE 2G 4Port */
	{ 0x82, 1, 0, "3", "2", "-B", "HFC0401",   },	/* FIVE 4G 1Port */
	{ 0x82, 2, 0, "3", "1", "-A", "HFC0402",   },	/* FIVE 4G 2Port */
	{ 0x86, 1, 0, "3", "2", "-B", "HFC0401",   },	/* FIVE 4G 1Port */
	{ 0x86, 2, 0, "3", "1", "-A", "HFC0402",   },	/* FIVE 4G 2Port */
	{ 0x87, 1, 0, "3", "1", "-A", "HFC1001",   },	/* FIVE 10G */
	{ 0x88, 1, 0, "3", "2", "-B", "HFC0401-C", },	/* Type3 Combo 1Port */
	{ 0x88, 2, 0, "3", "1", "-A", "HFC0402-C", },	/* Type3 Combo 2Port */
	{ 0x89, 1, 0, "3", "1", "-A", "HFC0401-C", },	/* FCSWM 1Port */
	{ 0x8a, 1, 0, "3", "2", "-B", "HFC0401-M", },	/* MEZANINE 1Port */
	{ 0x8a, 2, 0, "3", "1", "-A", "HFC0402-M", },	/* MEZANINE 2Port */
	{ 0x8b, 1, 0, "3", "2", "-B", "HFC0401-M", },	/* Enhanced MEZANINE 1Port *//* FCLNX-GPL-054 */
	{ 0x8b, 2, 0, "3", "1", "-A", "HFC0402-M", },	/* Enhanced MEZANINE 2Port *//* FCLNX-GPL-054 */
	{ 0x8f, 2, 0, "3", "1", "-A", "HFC0402-N", },	/* FC-GW 2Port */	/* FC-GW */
	{ 0x8f, 1, 0, "3", "2", "-B", "HFC0401-N", },	/* FC-GW 1Port */	/* FC-GW */
	{ 0x84, 1, 0, "3", "2", "-B", "HFC0401-E", },   /* FIVE 4G PCIe 1 port */    /* FCLNX-GPL-054 */
	{ 0x84, 2, 0, "3", "1", "-A", "HFC0402-E", },   /* FIVE 4G PCIe 2 port */    /* FCLNX-GPL-054 */
	{ 0,    0, 0, "",  "",  "",   "UNKNOWN"    }	/* END */
};


/* driver information */
struct manage_info hfc_manage_info;			/* driver information */
int instance = 0;
int hfc_major = 0;
int hba_num = 0;


int adapter_bindings = 0;
struct narrow_dev hfc_narrow_dev;												/* FCLNX-0392 */

static char *hfclddconf = NULL;

#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
extern struct miscdevice hfc_miscdev;
#endif


#if _HFC_ERROR_INJ
int hfc_debug_ioerr_code = 0;
#endif

#ifndef _HFC_NO_RASLOG
struct hraslogopt_st hraslogopt;
#endif

uint	raslog_install = 2;

#include "hfcl_moduleparam.h"

void print_module_parameter(void);
int hfc_read_hfcbios(struct adap_info *ap);
/* FCLNX-GPL-204 STR */
int hfc_proc_info_pfb(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
const char *hfc_info_pfb(struct Scsi_Host *host);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
int hfc_strategy_pfb_lck(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
#else
int hfc_strategy_pfb(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */
//int hfc_strategy_pfb(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
int hfc_eh_abort_pfb(struct scsi_cmnd *cmnd);
int hfc_eh_device_reset_pfb(struct scsi_cmnd *cmnd);
int hfc_eh_target_reset_pfb(struct scsi_cmnd *cmnd);				/* FCLNX-GPL-0449 */
int hfc_eh_bus_reset_pfb(struct scsi_cmnd *cmnd);
/* FCLNX-GPL-204 END */

#define MIN_IOBASE_LEN          0x100


int hfc_pci_conf(struct adap_info *ap)
{
	struct pci_dev *pdev	= NULL;
	unsigned long base0, len0, flag0, base4;
/*	ushort pci_reg; */ /* FCLNX-GPL-306 */
	uchar  basic = 0;
	
	HFC_ENTRY("hfc_pci_conf");
																	/* @MLPF */


	pdev = ap->pci_cfginf;
	
	base4 = pci_resource_start(pdev, 4);
	if(base4 == 0)
		basic = 1;
																	/* @MLPF */
	if ( basic == 1 || HFC_MMODE_CHECK_SHADOW(ap) )
	{
		/* Initialize PCI config register */ 
		hfc_set_hw_mcw_cfg(ap); /* FCLNX-GPL-306 */
	}
																	/* @MLPF */


	HFC_DBGPRT( "  hfcldd : hfc_pci_conf - allocate pci_resource\n"); 
	
	base0 = pci_resource_start(pdev, 0);
	len0 = pci_resource_len(pdev, 0);
	flag0 = pci_resource_flags(pdev, 0);
	HFC_DBGPRT("PCI resource 0 base0:%x, len0:%x, flag0:%x \n", (uint)base0, (uint)len0, (uint)flag0);

	if (!(flag0 & IORESOURCE_MEM)) {
		HFC_DBGPRT("  scsi(%ld): region #0 not a PIO resource, aborting\n",
				ap->host_no);

		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCA, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;
	}

	if (len0 < MIN_IOBASE_LEN) {
		HFC_DBGPRT("  scsi(%d): Invalid PCI I/O region size, aborting\n",
				(uint)ap->host_no);

		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCB, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;
	}

	if (pci_request_regions(pdev, "hfcldd")) {
		HFC_DBGPRT("  scsi(%d): Failed to reserve PCI_MEMORY_SPACE\n",
				(uint)ap->host_no);

		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCC, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;

	}

	ap->mem_base_addr = (ulong)ioremap(base0, len0);
	if (!ap->mem_base_addr){
		HFC_DBGPRT(" can not remap memory mapped IO \n");
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCD, NULL, 0) ;/* FCLNX-GPL-161 */
		goto iospace_error_exit;
	}
	
	HFC_DBGPRT(" hfcldd : hfc_attach - config pci devices end \n"); 
	HFC_DBGPRT( "  scsi(%ld): Success to reserve PCI_MEMORY_SPACE\n",
				(ulong)ap->host_no);
	
	if ( hfc_mlpf_setup_lparmode(ap) == HFC_MLPF_DISABLE )
		return (-ENODEV);
	
	/* initialize PCI config register */ 
	if ( (!(HFC_MMODE_CHECK_SHARED(ap)) || HFC_MMODE_CHECK_SHADOW(ap) ) )
	{
		hfc_set_hw_mcw_pci(ap); /* FCLNX-GPL-306 */
	}
	else{	/*** Set only LSI rev ***/ /* FCLNX-GPL-579 */
		if (ap->pkg.type == HFC_PKTYPE_FIVE_EX) {
			ap->pkg.lsi_rev = (uchar)hfc_read_reg(ap, HFC_IOSPACE_LSIREV, 0x01);
		}
	}
																	/* @MLPF */

	HFC_EXIT("hfc_pci_conf");
	
    return (0);
    
iospace_error_exit:
        pci_release_regions(pdev);

resource_error_exit:

	return (-ENOMEM);
}

extern int hfc_allocate_memory(struct adap_info *ap, int internal);
extern int hfc_allocate_dma(struct adap_info *ap);
extern int hfc_free_memory(struct adap_info *ap, int internal);


int hfc_attach(struct adap_info *ap)
{
	HFC_ENTRY("hfc_attach");

	/* set adap_info */
	HFC_DBGPRT( "  hfcldd : hfc_attach - init waitqueue head\n"); 
	init_waitqueue_head(&ap->mb_event);
	init_waitqueue_head(&ap->ioctl_event);
	init_waitqueue_head(&ap->ioexe_event);
	init_waitqueue_head(&ap->mck_event);
	init_waitqueue_head(&ap->init_event);																/* FCLNX_007 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	init_waitqueue_head(&ap->rport_event);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	init_waitqueue_head(&ap->mb_lock_event);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	sema_init(&ap->ioctl_sem, 1) ;
	sema_init(&ap->sem, 1) ;
#else
	init_MUTEX(&ap->ioctl_sem) ;
	init_MUTEX(&ap->sem) ;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */


	/* Initialize spinlock */
	HFC_DBGPRT( "  hfcldd : hfc_attach - init spin_lock\n"); 
	spin_lock_init(&ap->adap_lock);

	/* Allocate table area (Non DMA area)  */
	HFC_DBGPRT( "  hfcldd : hfc_attach - allocate memory area\n"); 
	if ( hfc_allocate_memory(ap, FALSE) ) goto attach_error_exit;

	/* allocate init_table, xob, xrb, mailbox, seg_info, soft_log (DMA area) */

	HFC_DBGPRT( "  hfcldd : hfc_attach - allocate DMA area\n"); 
	if ( hfc_allocate_dma(ap) )    goto attach_error_exit;
	
	HFC_EXIT("hfc_attach");
	return (0);
	
attach_error_exit:
    hfc_free_memory(ap, FALSE);

	HFC_EXIT("hfc_attach");
	return (1);
}


int hfc_allocate_memory(struct adap_info *ap, int internal)
{
	int 	i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	struct scatterlist *sgl=NULL;
#endif

	HFC_EXIT("hfc_allocate_memory");

	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate hardware trace area\n"); 

	if (ap->trc_ptr == NULL) {
		ap->trc_ptr = (struct hfctrace *)hfc_kmalloc(ap, (sizeof(struct hfctrace) * ap->trc_max),GFP_ATOMIC); /* FCLNX-XX */
		if (ap->trc_ptr == NULL) {
			hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3E, NULL, 0) ;/* FCLNX-GPL-161 */
			goto attachmem_error_exit;
		}
	}
	memset( ap->trc_ptr, 0, (sizeof(struct hfctrace) * ap->trc_max) );
	
	/* get target_scan */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate target_scan\n"); 
	if(ap->target_scan == NULL) {
		ap->target_scan = (struct target_scan *)hfc_kmalloc(ap, (sizeof(struct target_scan)*MAX_TARGET_PROBE), GFP_ATOMIC);
		if(ap->target_scan == NULL) {
			hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4B, NULL, 0) ;/* FCLNX-GPL-161 */
			goto attachmem_error_exit;
		}
	}
	memset( ap->target_scan, 0, (sizeof(struct target_scan)*MAX_TARGET_PROBE) );

	/* get bind_target_id */	/* FCLNX-GPL-0447 */

	/* allocate hfc_pkt */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate hfc_pkt \n"); 
	if (ap->pkt_pool[0] == NULL) {															/* FCLNX-0521 */
		for (i=0;i<HFC_PKT_POOL_NUM;i++) {
			ap->pkt_pool[i] = (struct hfc_pkt *)hfc_kmalloc(ap, (sizeof(struct hfc_pkt)*HFC_PKT_POOL_SIZE), GFP_ATOMIC);

			if(ap->pkt_pool[i] == NULL) {
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4C, NULL, 0) ;/* FCLNX-GPL-161 */
				goto attachmem_error_exit;
			}
			memset( ap->pkt_pool[i], 0, sizeof(struct hfc_pkt)*HFC_PKT_POOL_SIZE );
		}
	}

	/* allocate target_info */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate target_info \n"); 
	if (internal != TRUE) {
	    for (i=0;i<ap->max_target;i++) {
			if (ap -> target_arg[i] == NULL) {
				ap -> target_arg[i] = (struct target_info *)hfc_kmalloc(ap, sizeof(struct target_info), GFP_ATOMIC);
				if (ap->target_arg[i] == NULL){
					hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4B, NULL, 0) ;/* FCLNX-GPL-161 */
					goto attachmem_error_exit;
				}
			}
			memset( ap->target_arg[i], 0, sizeof(struct target_info) );
		}
	}

	/* allocate scsi_cmnd for Task Management */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_cmnd \n"); 
	ap->cmnd_pool = (struct scsi_cmnd *)hfc_kmalloc(ap, (sizeof(struct scsi_cmnd)*ap->cmnd_num), GFP_ATOMIC);
	if(ap->cmnd_pool == NULL){
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;/* FCLNX-GPL-161 */
		goto attachmem_error_exit;
	}
	memset( ap->cmnd_pool, 0, sizeof(struct scsi_cmnd)*ap->cmnd_num );

	/* allocate scsi_device for Task Management */	/* FCLNX-GPL-0343 */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_device \n"); 
	if(ap->cmnd_pool != NULL) {
		for(i=0;i<ap->cmnd_num;i++){
			if( ap->cmnd_pool[i].device == NULL ){
				ap->cmnd_pool[i].device = (struct scsi_device *)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC);
				if (ap->cmnd_pool[i].device == NULL){
					hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
					goto attachmem_error_exit;
				}
			}
			memset( ap->cmnd_pool[i].device, 0, sizeof(struct scsi_device) );
		}
	}												/* FCLNX-GPL-0343 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	/* allocate scsi_cmnd->cmnd area for Task Management */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_cmnd->cmnd[] area \n"); 
	if(ap->cmnd_pool != NULL) {
		for(i=0;i<ap->cmnd_num;i++){
			if(ap->cmnd_pool[i].cmnd == NULL){
				ap->cmnd_pool[i].cmnd = (uchar *)hfc_kmalloc(ap, 16, GFP_ATOMIC);
				if (ap->cmnd_pool[i].cmnd == NULL){
					hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
					goto attachmem_error_exit;
				}
			}
			memset( ap->cmnd_pool[i].cmnd, 0, 16 );
		}
	}
#endif												/* FCLNX-GPL-0343 */

	/* allocate scsi_cmnd for ioctl */		/* FCLNX-GPL-0343 */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_cmnd for ioctl\n"); 
	ap->ioctl_cmnd = (struct scsi_cmnd *)hfc_kmalloc(ap, (sizeof(struct scsi_cmnd)), GFP_ATOMIC);
	if(ap->ioctl_cmnd == NULL){
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;/* FCLNX-GPL-161 */
		goto attachmem_error_exit;
	}
	memset( ap->ioctl_cmnd, 0, sizeof(struct scsi_cmnd) );

	/* allocate scsi_device for ioctl */	/* FCLNX-GPL-0343 */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_device for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->device == NULL){
			ap->ioctl_cmnd->device = (struct scsi_device *)hfc_kmalloc(ap, sizeof(struct scsi_device), GFP_ATOMIC);
			if (ap->ioctl_cmnd->device == NULL){
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
				goto attachmem_error_exit;
			}
			memset( ap->ioctl_cmnd->device, 0, sizeof(struct scsi_device) );
		}
	}										/* FCLNX-GPL-0343 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	/* allocate scsi_cmnd->cmnd area for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_cmnd->cmnd[] area for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->cmnd == NULL){
			ap->ioctl_cmnd->cmnd = (uchar *)hfc_kmalloc(ap, 16, GFP_ATOMIC);
			if (ap->ioctl_cmnd->cmnd == NULL){
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
				goto attachmem_error_exit;
			}
			memset( ap->ioctl_cmnd->cmnd, 0, 16 );
		}
	}

	/* allocate request_queue for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_device for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->device != NULL){
			if(ap->ioctl_cmnd->device->request_queue == NULL){
				ap->ioctl_cmnd->device->request_queue = (struct request_queue *)hfc_kmalloc(ap, sizeof(struct request_queue), GFP_ATOMIC);
				if (ap->ioctl_cmnd->device->request_queue == NULL){
					hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
					goto attachmem_error_exit;
				}
				memset( ap->ioctl_cmnd->device->request_queue, 0, sizeof(struct request_queue) );
			}
		}
	}

	/* allocate dev_info for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scsi_device for ioctl\n"); 
	ap->ioctl_dev = (struct dev_info *)hfc_kmalloc(ap, sizeof(struct dev_info), GFP_ATOMIC);
	if (ap->ioctl_dev == NULL){
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
		goto attachmem_error_exit;
	}
	memset( ap->ioctl_dev, 0, sizeof(struct dev_info) );

	/* allocate scatterlist for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate scatterlist for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->sdb.table.sgl == NULL){
			ap->ioctl_cmnd->sdb.table.sgl = (struct scatterlist *)hfc_kmalloc(ap, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM, GFP_ATOMIC);
			if (ap->ioctl_cmnd->sdb.table.sgl == NULL){
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
				goto attachmem_error_exit;
			}
		}
		memset( ap->ioctl_cmnd->sdb.table.sgl, 0, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM );
	}

	/* allocate data buffer (DMA area) for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_allocate_memory - allocate data buffer for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		sgl = ap->ioctl_cmnd->sdb.table.sgl;
		for(i=0;i<HFC_SCATTERLIST_NUM;i++){
			if( sgl != NULL ){
				if( sgl->page_link == (unsigned long)0 ){
					sgl->page_link = (unsigned long)hfc_dma_alloc_coherent(ap, &ap->pci_cfginf->dev, HFC_PAGE_SIZE, &sgl->dma_address,GFP_ATOMIC);
					if (sgl->page_link == (unsigned long)0){
						hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
						goto attachmem_error_exit;
					}
				}
				memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
				sgl->length = HFC_PAGE_SIZE;
				sgl++;
			}
			else {
				break;
			}
		}
	}

#endif												/* FCLNX-GPL-0343 */

	HFC_DBGPRT( "  hfcldd : hfc_allocate_memory - allocate DMA area\n"); 

	HFC_EXIT("hfc_allocate_memory");

	return (0); 

attachmem_error_exit:
	hfc_free_memory(ap, FALSE);

	return (1); /* fail */
}



int hfc_allocate_dma(struct adap_info *ap)
{
	int 				an = 0;
	dma_addr_t			bus_addr_table[10];
	void				*vir_addr_table[10];
	int					size_table[10];
	int					err;	
	int				sn = 0; 
	struct pci_dev		*pdev = NULL;
	int 				seg_info_cnt, i, num, cnt;
	uint64_t			work_adr;
	struct seg_info			*seg;	

	HFC_ENTRY("hfc_allocate_dma");

	pdev = ap->pci_cfginf;
	
	/* allocate f/w init table (4096 Bytes) */

	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate fw_init_table \n");
	size_table[an] = sizeof(struct fw_init_tbl);

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->fw_init_p = (struct fw_init_tbl *)vir_addr_table[an];
	err=0x38;
	if (ap->fw_init_p == NULL) { 	/* failed? */
		goto alloc_error_exit;
	}                                                                                         

	HFC_DBGPRT("  (init table) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->fw_init_p, (ulong)ap->padr_init, size_table[an]);
			
	memset((char *)ap->fw_init_p, 0, (int)size_table[an]);
	ap->padr_init = bus_addr_table[an]; 
	an++;

	
	/* allocate xob table (4096 * 4 Bytes) */

	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate xob \n");
	size_table[an] = (sizeof(struct xob)) * ap->xob_max;

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->xob = (struct xob *)vir_addr_table[an];
	err=0x39;
	if (ap->xob == NULL) {	/* failed ? */
		goto alloc_error_exit;
	}

	memset((char *)ap->xob, 0, (int)size_table[an]);
	ap->fw_init_p->xob_num = ap->xob_max;
	ap->phys_xob = bus_addr_table[an];

	/* set each page top address to f/w init table */

	cnt=0;
	work_adr = (uint64_t)ap->phys_xob;
	for (i=0;i<(int)(ap->xob_max/(HFC_PAGE_SIZE/sizeof(struct xob)));i++) {
		hfc_write_val( ap->fw_init_p->fw_bus_addr[cnt], work_adr );
		work_adr += HFC_PAGE_SIZE;
		cnt++;
	}

	HFC_DBGPRT("  (xob) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->xob, (ulong)ap->phys_xob, size_table[an]);
	an++;
	
	/* allocate for xrb table */

	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate xrb \n");

	size_table[an] = (sizeof(struct xrb)) * ap->xrb_max;

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->xrb = (struct xrb *)vir_addr_table[an];
	err=0x3A;
	if (ap->xrb == NULL) {
		goto alloc_error_exit;
	}

	memset((char *)ap->xrb, 0, (int)size_table[an]);
	ap->fw_init_p->xrb_num = ap->xrb_max;
	ap->phys_xrb = bus_addr_table[an];


	/* set each page top address to f/w init table */
	
	work_adr = (uint64_t)ap->phys_xrb;
	for (i=0;i<(int)(ap->xrb_max/(HFC_PAGE_SIZE/sizeof(struct xrb)));i++) {
		hfc_write_val( ap->fw_init_p->fw_bus_addr[cnt], work_adr );
		work_adr += HFC_PAGE_SIZE;
		cnt++;
	}

	HFC_DBGPRT("  (xrb) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->xrb, (ulong)ap->phys_xrb, size_table[an]);
	an++;

	/* allocate mailbox */

	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate mailbox \n");
	size_table[an] = sizeof(struct mailbox);

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->mb = (struct mailbox *)vir_addr_table[an];
	err=0x3B;
	if (ap->mb == NULL) {
		goto alloc_error_exit;
	}

	memset((char *)ap->mb, 0, (int)size_table[an]);
	ap->phys_mb = bus_addr_table[an];

	HFC_DBGPRT("  (mailbox) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->mb, (ulong)ap->phys_mb, size_table[an]);
	an++;

	/* allocate mailbox parameter */
	
 	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate mailbox parameter \n");
	size_table[an] = 4096 * 2;

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->mb_parm = (uchar *)vir_addr_table[an];
	err=0x3C;
	if (ap->mb_parm == NULL) {
		goto alloc_error_exit;
	}
	ap->phys_mb_parm = bus_addr_table[an];
	memset((char *)ap->mb_parm, 0, 4096 * 2);	
	HFC_DBGPRT("  (init table) logical=%lx, physical=%lx, size = %d \n",
			(ulong)ap->xob, (ulong)ap->phys_xob, size_table[an]);
	an++;

	/* allocate soft log area */

 	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate soft log area \n");
	for (i=0; i<ap->slog_max ;i++) {									/* FCWIN_0013 */

		ap->slog_addr[i] = (uchar *)hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)HFC_PAGE_SIZE, &ap->phys_slog[i],GFP_ATOMIC);

		if (ap->slog_addr[i] == NULL) {
			err = 0x3D;
			goto alloc_error_exit;
		}		
		memset((char *)ap->slog_addr[i], 0, HFC_PAGE_SIZE);		
		hfc_write_val( ap->fw_init_p->fw_bus_addr[cnt], (uint64_t)ap->phys_slog[i] );

		HFC_DBGPRT("  (soft log area [%d]) logical=%lx, physical=%lx\n",
		i, (ulong)ap->slog_addr[i], (ulong)ap->phys_slog[i]);
		
		cnt++;
	}
	sn=i;
	ap->slog = (uchar *)ap->slog_addr[0];
	
	
	HFC_DBGPRT("  (soft log area) logical=%lx, physical=%lx, size = %d \n",
		(ulong)ap->slog, (ulong)ap->phys_slog[i], HFC_PAGE_SIZE);
	
	/* allocate seg_info */

 	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - allocate seg_info \n");
	seg_info_cnt = (ap->dma_max / HFC_PAGE_SIZE) +
         			((ap->dma_max/ HFC_PAGE_SIZE) /  (HFC_PAGE_SIZE / sizeof(struct seg_info)));
	size_table[an] =	HFC_ALIGNPG (seg_info_cnt * sizeof(struct seg_info) );

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->seg_info = (struct seg_info *)vir_addr_table[an];
	err=0x55;
	if (ap->seg_info == NULL) {
		goto alloc_error_exit;
	}
	
	memset((char *)ap->seg_info, 0, (int)size_table[an]);
	ap->seg_phys_addr =bus_addr_table[an];

	HFC_DBGPRT("  (seg_info) logical=%lx, physical=%lx, size = %d seg_info_cnt=%d \n",
			(ulong)ap->seg_info, (ulong)ap->seg_phys_addr, size_table[an], seg_info_cnt);

	an++;

	/* allocate iov_map */

	size_table[an] = (ap->dma_max/HFC_PAGE_SIZE/8); 

	vir_addr_table[an] = hfc_dma_alloc_coherent(ap, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);

	ap->iov_map = (uint *)vir_addr_table[an];
	err=0x56;
	if (ap->iov_map == NULL) {
			goto alloc_error_exit;
	}
	memset((char *)ap->iov_map, 0, (int)size_table[an]); /* clear iov_map */
	ap->phys_iov = bus_addr_table[an];
	HFC_DBGPRT(" (iov_map) logical = %lx, physical = %lx, size = %d cnt = %d\n",
		(ulong)ap->iov_map, (ulong)bus_addr_table[an], size_table[an], ap->iov_map_cnt);

	an++;

	/* initialize iov_map */

 	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - initialize iov_map \n");
	for ( i = 0; i < ap->iov_map_cnt; i++ ){
		ap->iov_map[i/32] |= (0x80000000 >> (i%32));
	}

	/* initialize seg_info */

 	HFC_DBGPRT( "  hfcldd : hfc_alloc_dma - initialize seg_info \n");
	num = HFC_PAGE_SIZE / sizeof(struct seg_info);
	cnt = ap->iov_map_cnt + (ap->iov_map_cnt / num);
	cnt /= num;

	HFC_DBGPRT( "hfcldd : seg_info  cnt = %d, num = %d \n", cnt, num);

	for(i=1;i<=cnt;i++) {
		seg = &ap->seg_info[(i*num)-1]; 
		seg->seg_len = HFC_SEG_LEN_F;		/* FCWIN-0079 */
		hfc_write_val(seg->seg_addr, (uint64_t)(ap->seg_phys_addr + (i * HFC_PAGE_SIZE)));
	}

	HFC_EXIT("hfc_allocate_dma");

	return (0); /* Normal end */

	
alloc_error_exit:
 	HFC_DBGPRT(" hfcldd : hfc_alloc_dma - exit with error (alloc_error_exit) \n");
	hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, err, NULL, 0) ;/* FCLNX-GPL-161 */
	hfc_free_dma(ap);
	HFC_EXIT("hfc_allocate_dma (error) ");
	return (1);

}

void hfc_free_mpap(struct adap_info *ap)
{
	struct mp_adap_info *mpap;
	ulong  flags = 0; /* FCLNX-GPL-177 */

	HFC_ENTRY("hfc_free_mpap");

	/* Remove adap_info from mp_adap_info */
	mpap = ap->mp_adap_info;

	if(mpap == NULL) return;
	
	/* Lock "mpap" */
	HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
	
	if ( mpap->port_cnt == 1) {
		mpap->ap = NULL;
		mpap->port_cnt--;
		
		/* Lock "hfc_manage_info" */ /* FCLNX-GPL-177 */
		spin_lock_irqsave(&hfc_manage_info.hfcmp_lock, flags);

		/* Remove mp_adap_info from hfc_manage_info */
		if ( hfc_manage_info.mp_adap_cnt == 1) {
			if ( hfc_manage_info.port_cnt == 0) { /* Not any FIVE-FX ports */
				if(hfc_manage_info.hfcldd_mp_mod) {
					hfc_manage_info.npubp->hfc_free_mp_table(); /* Free FPP/FIVE/FIVE-EX/FIVE-FX common area. */
				}
			}
			hfc_manage_info.mp_adap_info = NULL;
		}
		else {
			if (hfc_manage_info.mp_adap_info == mpap) {
				hfc_manage_info.mp_adap_info = mpap->next;
			}
			else {
				mpap->prev->next = mpap->next;
				if(mpap->next != NULL) /* FCLNX-GPL-247 */
				{
					mpap->next->prev = mpap->prev;
				}
			}
		}

		hfc_manage_info.mp_adap_cnt--;
		
		/* Unlock "hfc_manage_info" */ /* FCLNX-GPL-177 */
		spin_unlock_irqrestore(&hfc_manage_info.hfcmp_lock, flags);

		hfc_kfree(ap, mpap->hw_log);
		
		/* Unlock "mpap" */
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
		hfc_kfree(ap, mpap);
	}
	else {
		if (mpap->ap == ap)  /* FCLNX-GPL-247 start */
		{
			mpap->ap = ap->next;
		}
		else
		{
			ap->prev->next = ap->next;
			if(ap->next != NULL)
			{
				ap->next->prev = ap->prev;
			}
		} /* FCLNX-GPL-247 end */

		mpap->port_cnt--;

		/* Unlock "mpap" */
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	}
	

}


int hfc_free_memory(struct adap_info *ap, int internal)
{
	int i;
	struct target_info *target;
	struct scatterlist	*sgl=NULL;
	
	/* Release trace, bind_target_id, hfc_pkt, target_id */
	/* Release scsi_device for Task Management */
	if(ap->cmnd_pool != NULL) {						/* FCLNX-GPL-0343 */
		for(i=0;i<ap->cmnd_num;i++){
			if(ap->cmnd_pool[i].device != NULL){
				memset( ap->cmnd_pool[i].device, 0, sizeof(struct scsi_device) );
				hfc_kfree(ap, ap->cmnd_pool[i].device);
				ap->cmnd_pool[i].device = NULL;
			}
		}
	}												/* FCLNX-GPL-0343 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	/* Release scsi_cmnd->cmnd area for Task Management */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release scsi_cmnd->cmnd[] area \n"); 
	if(ap->cmnd_pool != NULL) {
		for(i=0;i<ap->cmnd_num;i++){
			if(ap->cmnd_pool[i].cmnd != NULL){
				memset( ap->cmnd_pool[i].cmnd, 0, 16 );
				hfc_kfree(ap, ap->cmnd_pool[i].cmnd);
				ap->cmnd_pool[i].cmnd = NULL;
			}
		}
	}
#endif												/* FCLNX-GPL-0343 */

	/* Release scsi_cmnd for Task Management */
	if (ap->cmnd_pool != NULL) {
		memset( ap->cmnd_pool, 0, sizeof(struct scsi_cmnd)*ap->cmnd_num );
		hfc_kfree(ap, ap->cmnd_pool);
		ap->cmnd_pool = NULL;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)	/* FCLNX-GPL-0343 */
	/* Release data buffer (DMA area) for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release data buffer for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		sgl = ap->ioctl_cmnd->sdb.table.sgl;
		for(i=0;i<HFC_SCATTERLIST_NUM;i++){
			if( sgl != NULL ){
				if( sgl->page_link != (unsigned long)0 ){
					sgl->page_link &= ~0x02;
					memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
					hfc_dma_free_coherent(ap, &ap->pci_cfginf->dev, (size_t)HFC_PAGE_SIZE, (void *)sgl->page_link, (dma_addr_t)sgl->dma_address);
					sgl->page_link = (unsigned long)0;
				}
				sgl++;
			}
			else {
				break;
			}
		}
	}
	
	/* release scatterlist for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release scatterlist for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->sdb.table.sgl != NULL){
			memset( ap->ioctl_cmnd->sdb.table.sgl, 0, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM );
			hfc_kfree(ap, ap->ioctl_cmnd->sdb.table.sgl);
			ap->ioctl_cmnd->sdb.table.sgl = NULL;
		}
		
	}
	
	/* release request_queue for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release request_queue for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->device != NULL){
			if(ap->ioctl_cmnd->device->request_queue != NULL){
				memset( ap->ioctl_cmnd->device->request_queue, 0, sizeof(struct request_queue) );
				hfc_kfree(ap, ap->ioctl_cmnd->device->request_queue);
				ap->ioctl_cmnd->device->request_queue = NULL;
			}
		}
	}

	/* release dev_info for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release dev_info for ioctl\n"); 
	if(ap->ioctl_dev != NULL) {
		memset( ap->ioctl_dev, 0, sizeof(struct dev_info) );
		hfc_kfree(ap, ap->ioctl_dev);
		ap->ioctl_dev = NULL;
	}
	
	/* release scsi_cmnd->cmnd area for ioctl */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release scsi_cmnd->cmnd[] area for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->cmnd != NULL){
			memset( ap->ioctl_cmnd->cmnd, 0, 16 );
			hfc_kfree(ap, ap->ioctl_cmnd->cmnd);
			ap->ioctl_cmnd->cmnd = NULL;
		}
	}
#endif

	/* release scsi_device for ioctl */	/* FCLNX-GPL-0343 */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release scsi_device for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL) {
		if(ap->ioctl_cmnd->device != NULL){
			memset( ap->ioctl_cmnd->device, 0, sizeof(struct scsi_device) );
			hfc_kfree(ap, ap->ioctl_cmnd->device);
			ap->ioctl_cmnd->device = NULL;
		}
	}									/* FCLNX-GPL-0343 */

	/* release scsi_cmnd for ioctl */		/* FCLNX-GPL-0343 */
	HFC_DBGPRT(" hfcldd : hfc_free_memory - release scsi_cmnd for ioctl\n"); 
	if(ap->ioctl_cmnd != NULL){
		memset( ap->ioctl_cmnd, 0, sizeof(struct scsi_cmnd) );
		hfc_kfree(ap, ap->ioctl_cmnd);
		ap->ioctl_cmnd = NULL;
	}
	
	/* FCLNX-GPL-0343 */

	if (internal != TRUE) {
		for (i=0;i<MAX_TARGET_PROBE;i++) {
			
			target = ap -> target_arg[i];
			if (target != NULL) {
//				if (hfc_manage_info.hfcldd_mp_mod) {
//					hfc_manage_info.npubp->hfc_free_dev(target);
				hfc_free_dev(target);			/* FCLNX-GPL-0343 */
//				}
				
				memset( ap -> target_arg[i], 0, sizeof(struct target_info) );
				hfc_kfree(ap, ap->target_arg[i]);
				ap->target_arg[i] = NULL;
			}
		}
	}
	
	for (i=0;i<HFC_PKT_POOL_NUM;i++) {			/* FCLNX-0521 */
		if (ap->pkt_pool[i] != NULL) {
			memset( ap->pkt_pool[i], 0, sizeof(struct hfc_pkt)*HFC_PKT_POOL_SIZE );
			hfc_kfree(ap, ap->pkt_pool[i]);
			ap->pkt_pool[i] = NULL;
		}
	}
	
	if (ap->target_scan != NULL) {
		memset(ap->target_scan, 0, (sizeof(struct target_scan)*MAX_TARGET_PROBE) );
		hfc_kfree(ap, ap->target_scan);
		ap->target_scan = NULL;
	}
	
	if (ap->trc_ptr != NULL) {
		memset(ap->trc_ptr, 0, (sizeof(struct hfctrace) * ap->trc_max) );
		hfc_kfree(ap, ap->trc_ptr);
		ap->trc_ptr = NULL;
	}
	
	return(0);
}


int hfc_free_dma(struct adap_info *ap)
{
	struct pci_dev		*pdev = NULL;
	int 				i;
	int 				seg_info_cnt;

	size_t size;

	pdev = ap->pci_cfginf;

	HFC_ENTRY("hfc_free_dma");	

	if ( ap->fw_init_p != NULL ) {
		/* Release f/w init table (4096 Bytes) */
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free fw_init_tbl \n");
		memset((char *)ap->fw_init_p, 0, (int)sizeof(struct fw_init_tbl));

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)sizeof(struct fw_init_tbl),(void *)ap->fw_init_p, (dma_addr_t)ap->padr_init);

		ap->fw_init_p = NULL;
	}
	
	if (ap->xob != NULL) {
		/* Release xob table (4096 * 4 Bytes) */
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free xob_tbl \n");
		memset((char *)ap->xob, 0, (int)(sizeof(struct xob) * ap->xob_max));

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)(sizeof(struct xob) * ap->xob_max),(void *)ap->xob, (dma_addr_t)ap->phys_xob);

		ap->xob = NULL;
	}
	
	if (ap->xrb != NULL) {
		/* Release xrb table */
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free xrb tbl \n");
		memset((char *)ap->xrb, 0, (int)(sizeof(struct xrb) * ap->xrb_max));

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)(sizeof(struct xrb) * ap->xrb_max),(void *)ap->xrb, (dma_addr_t)ap->phys_xrb);

		ap->xrb = NULL;
	}
	
	if (ap->mb != NULL) {
		/* Release mailbox */
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free fw_mailbox \n");
		memset((char *)ap->mb, 0, (int)sizeof(struct mailbox));

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)sizeof(struct mailbox), (void *)ap->mb, (dma_addr_t)ap->phys_mb);

		ap->mb = NULL;
	}
	
	if (ap->mb_parm != NULL) {
		/* Release mailbox parameter */
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free mailbox parameter \n");
		memset((char *)ap->mb_parm, 0, (int)(4096 * 2) );

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)(4096 * 2), (void *)ap->mb_parm, (dma_addr_t)ap->phys_mb_parm);

		ap->mb_parm = NULL;
	}
	
	/* Release soft log area */
	HFC_DBGPRT(" hfcldd : hfc_free_dma - free soft log area \n");
	for (i=0; i<ap->slog_max; i++) { 													/* FCLNX_0013 */
		if (ap->slog_addr[i] != NULL) {
			memset((char *)ap->slog_addr[i], 0, (int)HFC_PAGE_SIZE );

			hfc_dma_free_coherent(ap, &pdev->dev, (size_t)HFC_PAGE_SIZE, (void *)ap->slog_addr[i], (dma_addr_t)ap->phys_slog[i] );

			ap->slog_addr[i] = NULL;
		}
	}

	if (ap->seg_info != NULL) {
		/* Release seg_info */
		seg_info_cnt = (ap->dma_max / HFC_PAGE_SIZE) +
    	     			((ap->dma_max/ HFC_PAGE_SIZE) /  (HFC_PAGE_SIZE / sizeof(struct seg_info)));
		size =	HFC_ALIGNPG (seg_info_cnt * sizeof(struct seg_info) );
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free seg_info \n");
		memset((char *)ap->seg_info, 0, (int)size);

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)size, (void *)ap->seg_info, (dma_addr_t)ap->seg_phys_addr);

		ap->seg_info = NULL;
	}
	
	if (ap->iov_map != NULL) {
		/* Release iov_map */
		size = ap->dma_max/HFC_PAGE_SIZE/8;
		HFC_DBGPRT(" hfcldd : hfc_free_dma - free iov_map \n");
		memset((char *)ap->iov_map, 0, (int)size );

		hfc_dma_free_coherent(ap, &pdev->dev, (size_t)size, (void *)ap->iov_map, (dma_addr_t)ap->phys_iov);

		ap->iov_map = NULL;
	}

	HFC_EXIT("hfc_free_dma");

	return (0); /* Normal end */
}


void hfc_detach(struct adap_info *ap)
{
	/* Release DMA memory area */
	hfc_free_dma(ap);
	
	if (hfc_manage_info.hfcldd_mp_mod) {					/* FCLNX-GPL-204 */
		hfc_manage_info.npubp->hfc_remove_lgpath(ap);
	}
	
	if(HFC_MMODE_CHECK_MLPF(ap)&&(ap->hg_cca_p != NULL)){	/* FCLNX-GPL-494 */
		hfc_free_mlpf_cca(ap);
	}													/* FCLNX-GPL-494 */
	
	/* Release adapter memory area */
	hfc_free_memory(ap, FALSE);
}

void hfc_set_fw_init_tbl(struct adap_info *ap)
{
	int		i;
	uchar			pre_conf=0, dd_support=(HFC_DDSP_OPTERR9E | HFC_PORTID_GUARD_CTL);		/* FCLNX-GPL-570 *//* FCLNX-GPL-FX-409*/
	HFC_DBGPRT( "hfcldd : set_fw_init_tbl - HFC_ENTRY \n");
	HFC_ENTRY("hfc_set_hw_init_tbl");
	
	/* +00-01 max exchange */
	hfc_write_val( ap->fw_init_p->max_exchange, MAX_EXCHANGE);

	/* +02-03 max port */
	hfc_write_val( ap->fw_init_p->max_port,     ap->max_target);

	/* +04 revision	*/
	hfc_write_val( ap->fw_init_p->init_tbl_rev, 0);
	if((hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1)) & 0x20){		/* FCLNX-GPL-570 */
		dd_support |= HFC_DDSP_LINKUP_DELAY;
	}
	hfc_write_val( ap->fw_init_p->dd_support_info, dd_support);		/* FCLNX-0273 *//* FCLNX-GPL-570 */

	if ( ap->pkg.type == HFC_PKTYPE_FPP )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0x4B, 0x1);
	}
	else if( ap->pkg.type == HFC_PKTYPE_FIVE )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xAF, 0x1);
	}
	else /* FIVE-EX */
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xCA, 0x1);
	}

	
	hfc_write_val( ap->fw_init_p->init_tbl_rev, 1);
	
	hfc_write_val( ap->fw_init_p->fw_rnid.n_port_name, ap->ww_name);
	hfc_write_val( ap->fw_init_p->fw_rnid.node_name, ap->node_name);
																			/* FCLNX-0299 */	


	/* +18-1F pointer to Mailbox top */
	hfc_write_val( ap->fw_init_p->mb_addr, (uint64_t)ap->phys_mb);
	
	/* +20-21 XOB Queue HFC_ENTRY number */
	hfc_write_val( ap->fw_init_p->xob_num,  ap->xob_max);
	/* +24-25 XRB Queue HFC_ENTRY number */
	hfc_write_val( ap->fw_init_p->xrb_num,  ap->xrb_max);
	/* +28-29 Soft Log page number */
	hfc_write_val( ap->fw_init_p->slog_num, (ap->slog_max / (HFC_PAGE_SIZE / HFC_SLOG_LEN)));
	/* +2A-2B Soft Log HFC_ENTRY length */
	hfc_write_val( ap->fw_init_p->slog_len, HFC_SLOG_LEN);

	/* +180 trace information  */
	hfc_write_val( ap->fw_init_p->trc_info.trc_num, 0x01 );
	hfc_write_val( ap->fw_init_p->trc_info.inst,    0x04 );

	for (i=0;i<4;i++) {
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].seg_no,        i );
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].resv0,         0x00 );

		if ((i==0) && !(ap->fw_parm & HFC_FWP_SEGTRC_V))														/* FCWIN-0192 */
				hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].mode, 	 0x60 );	/* FCWIN-0192 */
		else
				hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].mode, 	 0xf8 );

		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].ctl,           0x00 );
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].top_port_id,   0x00000000 );
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].bottom_port_id,0x00ffffff );
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[i].resv1,         0x00000000 );
	}
	hfc_write_val( ap->fw_init_p->trc_info.trc_seg[0].seg_no, 0x80 );	

	/* for FIVE-EX pass1 only. */ /* This function do NOP for other type HBA. */
	hfc_send_msi_info(ap);

	HFC_EXIT("hfc_set_hw_init_tbl");
}

#define HFC_VPD_WAITCNT 			5000000
#define CHECKSUM_FPP_ADD_ADDR		0x55
#define CHECKSUM_FIVE_ADD_ADDR		0x63
#define CHECKSUM_FIVE_EX_ADD_ADDR	0x73 /* FIVE-EX */
#define VPDDATA_LEN                 0x80
#define VPD_ADDRESS_FLAG_MASK       0x80
#define HFC_CAPABILITIES_POINTER    0x34
#define HFC_CAPABILITIES_ADDR       0x40

/* We must lock mpap, before calling this Function. */
int hfc_get_vpd(struct adap_info *ap, uchar *vpd_buf)
{

	char  *vpd_ptr;
	uchar capabilities_list_addr = 0;
	uint i=0;
	ushort offset=0,adr=0;
	uchar sum=0;
	uint  nwords=0;
	uint value,value_bak;
	int err;
	uint chk_sum = CHECKSUM_FPP_ADD_ADDR;
	
	HFC_ENTRY("hfc_get_vpd");

	if  ( (!(HFC_MMODE_CHECK_SHARED(ap)) || HFC_MMODE_CHECK_SHADOW(ap) ) )
	{
		/* Read status register */
		value = (uint) hfc_read_cnfg(ap, HFC_HOST_STAT_CMD, 4);
		HFC_4B_TO_4L(value_bak,value);

		if (!(value & 0x00100000)) {	/* bit 4(capabilities list) = 0? */
			/* VPD is not supported */
			err = 0x71;
			goto getvpd_error_exit;
		}
		
		/* Read capabilities list(0x34) */
		value = (uint) hfc_read_cnfg(ap, HFC_CAPABILITIES_POINTER, 4 );
		HFC_4B_TO_4L(value_bak,value);

		if ((value & 0x000000FF) != HFC_CAPABILITIES_ADDR) {
			/* Abnormal end (data is not 0x40) */
			err = 0x72;
			goto getvpd_error_exit;
		}

		if( (ap->pkg.type == HFC_PKTYPE_FPP) ||
			(ap->pkg.type == HFC_PKTYPE_FIVE) )
		{
			capabilities_list_addr = (value & 0xFF);
			
			/* Read VPD data */
			value = (uint) hfc_read_cnfg(ap, capabilities_list_addr, 4);
			HFC_4B_TO_4L(value_bak,value);

			/* ID != VPD(3)? */
			if ((value & 0x000000FF) != 3) {
				err = 0x73;
				goto getvpd_error_exit;
			}
			
		}
		else /* FIVE-EX */
		{
			/* Search Capabitilies ID */
			value = pci_find_capability(ap->pci_cfginf, PCI_CAP_ID_VPD);
			if(value == 0)  /* ERR End  */
			{
				/* Cannot find "PCI_CAP_ID_VPD". */
				err = 0x73;
				goto getvpd_error_exit;
			}
			else /* Normal End */
			{
				capabilities_list_addr = value;
			}
		}

		vpd_ptr = (char *)vpd_buf; 
		offset = 0;
		do {
			value = 0;
			adr = 0x0000 | offset;
			hfc_write_cnfg(ap, (capabilities_list_addr + 0x2), 2, adr);

			i = 0;
			while (i != HFC_VPD_WAITCNT) {
				value = (uint) hfc_read_cnfg(ap, capabilities_list_addr, 4);
				HFC_4B_TO_4L(value_bak,value);

				if (value & 0x80000000) {
					break;
				}

			    udelay(10);
				i++;
			}
			if (i == HFC_VPD_WAITCNT) {
				/* Flag has not changed */
				err = 0x74;
				goto getvpd_error_exit;
			}

			value = 0;
			value = (uint) hfc_read_cnfg(ap, (capabilities_list_addr + 0x4), 4);
			HFC_4B_TO_4L(value_bak,value);

			/* Store data in reverse order */
			*vpd_ptr++ = value         & 0xFF;
			*vpd_ptr++ = (value >> 8)  & 0xFF;
			*vpd_ptr++ = (value >> 16) & 0xFF;
			*vpd_ptr++ = (value >> 24) & 0xFF;
			
			offset += 4;
		} while (offset != VPDDATA_LEN);

		/* Caliculate checksum */			  
		if ( ap->pkg.type == HFC_PKTYPE_FPP )
		{
			chk_sum = CHECKSUM_FPP_ADD_ADDR;
		}
		else if(ap->pkg.type == HFC_PKTYPE_FIVE)
		{
			chk_sum = CHECKSUM_FIVE_ADD_ADDR;
		}
		else /* FIVE-EX */
		{
			chk_sum = CHECKSUM_FIVE_EX_ADD_ADDR;
		}
		
		vpd_ptr = (char *)vpd_buf;
		for (sum = 0, nwords = 0; nwords <= chk_sum; nwords++) {
			sum += *vpd_ptr++;
		}

		/* Checksum = 0? */
		if (sum != 0) {
			err = 0x75;
			goto getvpd_error_exit;
		}
	}
	else /* This case is considered for Guest LPAR */
	{
		hfc_mlpf_get_mmio_hg(ap, ap->mp_adap_info->vpd_buf, HFC_IOHGSPC_VPDAREA, HFC_IOHGSPC_VPDAREA_LEN);
	}

	HFC_EXIT("hfc_get_vpd");

	return(0);

getvpd_error_exit:
	/* Get driver Log */
	hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, err, NULL, 0) ;/* FCLNX-GPL-161 */

	if( (ap->pkg.type == HFC_PKTYPE_FPP) ||
		(ap->pkg.type == HFC_PKTYPE_FIVE))
	{
		/* Issue initial reset command */
		hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDRES,( char )0x1,(char)0x04);

		/* Wait 2ms */
		mdelay(2);

		/* Force link-status into disable state */
		if ( ap->pkg.type == HFC_PKTYPE_FPP )
		{
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDFCIF,( char )0x1,(char)0x80);
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
			hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDLED,( char )0x1,HFC_WAKE_UP_FAILURE);
		}
		else if( ap->pkg.type == HFC_PKTYPE_FIVE )
		{
			if(!(HFC_MMODE_CHECK_SHARED(ap))){
				hfc_write_reg(ap,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
			}
			else{
				hfc_mlpf_set_fcif(ap, 0x80808080);	/* FCLNX-GPL-399 */
			}
			hfc_write_reg(ap,( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000000 );
			if(!(HFC_MMODE_CHECK_SHARED(ap))){
				hfc_write_reg(
					ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
			}
			else{
				hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
			}
		}
		/* Err End */
		return (1);
	}
	else /* FIVE-EX */
	{
		/* Normal End */
		return (0);
	}
}


int hfc_config_hw_set(struct adap_info *ap, uint retry_maxcnt)
{
	uchar		stat_chkerrflg;
	uchar		post_chkerrflg;
	uint		retry_cnt;
	uchar		dmp_data[32];
	uint		errlog_data[4];
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} value;

	uchar		err_code = 0;
	uint 		status_bak = 0;
	ushort		result;

	HFC_ENTRY("hfc_config_hw_set");


	memset(dmp_data, 0, sizeof(dmp_data));
    memset(errlog_data, 0, sizeof(errlog_data));

	/* Set 0x00 to CMD_RES (0x40) */
    hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x00);

	mdelay(250); /* 1ms wait */									/* FCLNX_0012 */


	for (retry_cnt = 0; retry_cnt < (retry_maxcnt + 1); retry_cnt++) {

		HFC_DBGPRT( "config_hw_set - retry loop = %d\n", retry_cnt);

		/* Initialize */
		stat_chkerrflg = 0;
		post_chkerrflg = 0;
		err_code = 0;
		
		value.l = 0;
		status_bak = 0;
		/* Read status information (4 bytes) */
		status_bak = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4 );
		HFC_4B_TO_4L(value.l,status_bak);
		
		HFC_DBGPRT( "hfc_read_reg HFC_IOSPACE_STATUS[%x] = 0x%08x.\n",
					HFC_IOSPACE_STATUS0, status_bak );

		if (status_bak != 0x80000000) { /* status is normal? */

			HFC_DBGPRT( "config_hw_set - firmware status error\n");
		
			/* Still executing reset process? */
			if ((value.c[0] & HFC_PCI_RESETCHK) != 0x00) {
				
				HFC_DBGPRT( "config_hw_set - wakeup failure (HFC_PCI_RESETCHK) \n");

				/* Set LED to Wake up Falure */
				hfc_write_reg(ap,( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
				hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_WAKE_UP_FAILURE);
				stat_chkerrflg = 1;
				err_code = 1;
			}
			else {
				/* Detect PCI error? */
				if ((value.c[1] & HFC_PCI_PCIERR_DETECTED) == HFC_PCI_PCIERR_DETECTED) {
					
					/* Set LED to Wake up Falure */
					
					HFC_DBGPRT( "config_hw_set - pci error was detected (HFC_PCI_PCIERR_DETECTED) \n");	

					hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
					hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_WAKE_UP_FAILURE);
					stat_chkerrflg = 1;
					err_code = 2;
				}
				else {
					/* EXGMCK and BOOTRUN check ? */
					if ((value.c[1] & HFC_PCI_PCICHK) == HFC_PCI_PCICHK) {
						
						/* Set LED to Wake up Falure */
						HFC_DBGPRT( "config_hw_set - pci check was detected (HFC_PCI_PCICHK) \n");	

						hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
						hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_WAKE_UP_FAILURE);
						stat_chkerrflg = 1;
						err_code = 3;
					}
					else {
						/* EXGMCK check ? */
						if ((value.c[1] & HFC_PCI_EXGMCK) == HFC_PCI_EXGMCK) {

							/* Set LED to POST up Falure */
							HFC_DBGPRT( "config_hw_set - exigent machine check was detected (HFC_PCI_EXGMCK) \n");	

							hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
							hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_POST_FAILURE);
							stat_chkerrflg = 1;
							err_code = 4;
						}
						else {
							/* BOOTRUN check? */
							if ((value.c[1] & HFC_PCI_BOOTRUN) == HFC_PCI_BOOTRUN) {
								
								HFC_DBGPRT( "config_hw_set - bootrun check was detected (HFC_PCI_BOOTRUN) \n");	
								
								hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
								hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_WAKE_UP_FAILURE);
								stat_chkerrflg = 1;
								err_code = 5;
							}
							else {
								/* FUNC_STOP check? */
								if ((value.c[1] & HFC_PCI_FCNSTOP) == HFC_PCI_FCNSTOP) {

									HFC_DBGPRT( "config_hw_set - function stop was detected (HFC_PCI_FCNSTOP) \n");	
			
									hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
									hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_WAKE_UP_FAILURE);
									stat_chkerrflg = 1;
									err_code = 6;
								}
								else {
									/* CH not Ready check? */
									if ((value.c[2] & HFC_PCI_CH_NOT_READY) == HFC_PCI_CH_NOT_READY) {

										/* Set LED to POST up Falure */

										HFC_DBGPRT( "config_hw_set - CH was not ready (HFC_PCI_CH_NOT_READY) \n");	

										hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
										hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_POST_FAILURE);
										stat_chkerrflg = 1;
										err_code = 7;
									}
								}
							}
						}
					}
				}
			}
		}
		/* Normal case */
		if (stat_chkerrflg == 0) {
			
			/* Read first 2bytes of POST result */
			result = (ushort) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 0x2);

			HFC_DBGPRT( "config_hw_set - Port result = %02x \n", result);
			
			if ((result & 0xff00) == 0x8000) { /* Is first byte 0x80? */
				
				if ((result & 0x00ff) == 0x00FF) { /* Normal end? */
					
					HFC_DBGPRT( "config_hw_set - Port status is normal \n");
					break;
				} else {							/* Otherwise error */
					post_chkerrflg = 1;
					err_code = 9;
				}
			}
			else {
				/* Set LED to POST up Falure */
				HFC_DBGPRT( "config_hw_set - Port status is not normal \n");

				hfc_write_reg(ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x1,(char)0x44);
				hfc_write_reg(ap, HFC_IOSPACE_CMDLED, 0x1, HFC_POST_FAILURE);
				post_chkerrflg = 1;
				err_code = 8;
			}
		}

		HFC_DBGPRT( "config_hw_set() - err_code_2 = 0x%x.",err_code);
		if(err_code != 0){ 
			hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x35, NULL, 0) ;/* FCLNX-GPL-161 */
		}			
		/* Retry count reaches retry maxcnt */
		if (retry_cnt == retry_maxcnt) {
			
			/* Status information error? */
			if (stat_chkerrflg == 1) {
				HFC_DBGPRT( "config_hw_set() - stat_chkerrflg=1, errcode = 0x%x.",err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));

				/* Collect error log information (16 bytes from STATUS) */
				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4);
				errlog_data[1] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS1, 0x4);
				errlog_data[2] = (uint) hfc_read_reg(ap, HFC_IOSPACE_ERRDETAIL0, 0x4);
				errlog_data[3] = (uint) hfc_read_reg(ap, HFC_IOSPACE_ERRDETAIL1, 0x4);

				/* Create error log data */
				dmp_data[0]  = (uchar)(errlog_data[0] >> 24);
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16);
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 );
				dmp_data[3]  = (uchar)(errlog_data[0]      );
				dmp_data[4]  = (uchar)(errlog_data[1] >> 24);
				dmp_data[5]  = (uchar)(errlog_data[1] >> 16);
				dmp_data[6]  = (uchar)(errlog_data[1] >> 8 );
				dmp_data[7]  = (uchar)(errlog_data[1]      );
				dmp_data[8]  = (uchar)(errlog_data[2] >> 24);
				dmp_data[9]  = (uchar)(errlog_data[2] >> 16);
				dmp_data[10] = (uchar)(errlog_data[2] >> 8 );
				dmp_data[11] = (uchar)(errlog_data[2]      );
				dmp_data[12] = (uchar)(errlog_data[3] >> 24);
				dmp_data[13] = (uchar)(errlog_data[3] >> 16);
				dmp_data[14] = (uchar)(errlog_data[3] >> 8 );
				dmp_data[15] = (uchar)(errlog_data[3]      );

				
				hfc_logout(ap, (uint)0x35, HFC_ERRLOG_TYPE_IMLLOG);
				
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x35, dmp_data, 16) ;
			}
			/* Post-check error? */
			else if (post_chkerrflg == 1) {
				HFC_DBGPRT(  "config_hw_set() - post_chkerrflg=1, errcode = 0x%x.", err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));

				/* Read POST information */
				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 4);

				/* Create error log data */
				dmp_data[0]  = (uchar)(errlog_data[0] >> 24) ;
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16) ;
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 ) ;
				dmp_data[3]  = (uchar)(errlog_data[0]	   ) ;

				
				hfc_logout(ap, (uint)0x36, HFC_ERRLOG_TYPE_IMLLOG);
				
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x36, dmp_data, 16) ;
			}

			/* Abnormal End */
			return(-1);
		}
		/* Forced MCK */
		hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, 0x1, 0x08);

		/* CTLRES */
		hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x02);

		/* Wait 1ms*/
		mdelay(1);
		
		/* Initiate reboot */
		hfc_write_reg(ap, HFC_IOSPACE_CMDBOOT, 0x1, 0x40);

		/* Wait 500ms */
		mdelay(500);	/* FCLNX_0012 */
    }

	HFC_EXIT("hfc_config_hw_set");

    return(0);
}

 
int hfc_config_hw_set_five(struct adap_info *ap, uint retry_maxcnt)
{
	uchar		stat_chkerrflg;
	uchar		post_chkerrflg;
	uint		retry_cnt;
	uchar		dmp_data[32];
	uint		errlog_data[4];
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} value;

	uchar		err_code = 0;
	uint 		status_bak = 0;
	uchar       exe_reset = 0;

	HFC_ENTRY("hfc_config_hw_set_five");
	
	/* Initialize */
	memset(dmp_data, 0, sizeof(dmp_data));
	memset(errlog_data, 0, sizeof(errlog_data));

	/* Set 0x00 to CMD_RES (0x30) */
	hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x00);

	mdelay(1000); /* Wait 1000ms */

//	if ( ( hfc_manage_info.hfcplus_enable ) && 
	if ( ( hfc_manage_info.hfcldd_mp_mod) &&
		 ( ap->isol_force == HFC_CHKSTP_FRC_ISOL )){ /* - FCLNX-546 - *//* FCLNX-GPL-147 */ /* FCLNX-GPL-349 */
		/* Stop optical transmission */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(ap,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
		}
		else{
			hfc_mlpf_set_fcif(ap, 0x80808080);	/* FCLNX-GPL-399 */
		}
		/* Turn LED (Yellow and Green) off */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(
				ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
		}
		else{
			hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
		}
		return 0;
	} /* FCLNX-GPL-147 */

//skip_cmd_res : 

	for (retry_cnt = 0; retry_cnt < (retry_maxcnt + 1); retry_cnt++) {

		stat_chkerrflg = 0;
		post_chkerrflg = 0;
		err_code = 0;
		value.l = 0;
		status_bak = 0;
		
		/* Read status information (4 bytes) */
		status_bak = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4 );
		HFC_4B_TO_4L(value.l,status_bak);

		HFC_DBGPRT( "hfc_read_reg HFC_IOSPACE_STATUS[%x] = 0x%08x.",
					HFC_IOSPACE_STATUS0, status_bak);
		
		if (status_bak != 0x80000000) { /* status is normal? */

			/* Still executing reset process? */
			if ((value.c[0] & HFC_PCI_RESETCHK) != 0x00) {

				if(!(HFC_MMODE_CHECK_SHARED(ap))){
					hfc_write_reg(
						ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
				}
				else{
					hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
				}
				stat_chkerrflg = 1;
				err_code = 1;
			}
			else {
				/* Detect PCI error? */
				if (((value.c[1] & HFC_PCI_PCIERR_DETECTED) == HFC_PCI_PCIERR_DETECTED) && ( exe_reset != 1)){

					/* PCI error status reset */                      /*  FCWIN-0223 */
					hfc_write_cnfg(ap, 0x06, 0x2, 0xffff);
					hfc_write_cnfg(ap, 0x6c, 0x4, 0xffffffff);
					hfc_write_cnfg(ap, 0x70, 0x4, 0x0fffffff);
					exe_reset = 1;                                      /*  FCWIN-0223 */

					/* Set LED to Wake up Falure */
					if(!(HFC_MMODE_CHECK_SHARED(ap))){
						hfc_write_reg(
							ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
					}
					else{
						hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
					}
					stat_chkerrflg = 1;
					err_code = 2;
				}
				else {
					/* EXGMCK and BOOTRUN check ? */
					if ((value.c[1] & HFC_PCI_PCICHK) == HFC_PCI_PCICHK) {

						/* Set LED to Wake up Falure */
						if(!(HFC_MMODE_CHECK_SHARED(ap))){
							hfc_write_reg(
								ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
						}
						else{
							hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
						}
						stat_chkerrflg = 1;
						err_code = 3;
					}
					else {
						/* EXGMCK check */
						if ((value.c[1] & HFC_PCI_EXGMCK) == HFC_PCI_EXGMCK) {

							/* Set LED to POST up Falure */
							if(!(HFC_MMODE_CHECK_SHARED(ap))){
								hfc_write_reg(
									ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
							}
							else{
								hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
							}
							stat_chkerrflg = 1;
							err_code = 4;
						}
						else {
							/* BOOTRUN check */
							if ((value.c[1] & HFC_PCI_BOOTRUN) == HFC_PCI_BOOTRUN) {

								/* Set LED to Wake up Falure */
								if(!(HFC_MMODE_CHECK_SHARED(ap))){
									hfc_write_reg(
										ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
								}
								else{
									hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
								}
								stat_chkerrflg = 1;
								err_code = 5;
							}
							else {
								/* FUNC_STOP check */
								if ((value.c[1] & HFC_PCI_FCNSTOP) == HFC_PCI_FCNSTOP) {

									/* Set LED to Wake up Falure */
									if(!(HFC_MMODE_CHECK_SHARED(ap))){
										hfc_write_reg(
											ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
									}
									else{
										hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
									}
									stat_chkerrflg = 1;
									err_code = 6;
								}
								else {
									/* CH not Ready check */
									if ((value.c[2] & HFC_PCI_CH_NOT_READY) == HFC_PCI_CH_NOT_READY) {

										/* Set LED to POST up Falure */
										if(!(HFC_MMODE_CHECK_SHARED(ap))){
											hfc_write_reg(
												ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
										}
										else{
											hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
										}
										stat_chkerrflg = 1;
										err_code = 7;
									}
								}
							}
						}
					}
				}
			}
		}
		/* Normal case */
		if (stat_chkerrflg == 0) {
			ushort result;

			/* Read first 2bytes of POST result */
			result = (ushort) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 0x2);
			if ((result & 0xff00) == 0x8000) {/* Is first byte 0x80? */

				if ((result & 0x00ff) == 0x00FF) {/* Normal end? */

					break;
				} else { /* Otherwise error */
					post_chkerrflg = 1;
					err_code = 9;
				}
			}
			else {

				/* Set LED to POST up Falure */
				if(!(HFC_MMODE_CHECK_SHARED(ap))){
					hfc_write_reg(
						ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
				}
				else{
					hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
				}
				post_chkerrflg = 1;
				err_code = 8;
			}
		}

		HFC_DBGPRT(  "config_hw_set_five() - err_code_2 = 0x%x.",err_code);

		/* Retry count reaches retry maxcnt */
		if (retry_cnt == retry_maxcnt) {

			/* Status information error? */
			if (stat_chkerrflg == 1) {
				HFC_DBGPRT(  "CONFIG_HW_set_five() - stat_chkerrflg=1, errcode = 0x%x.",err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));

				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4);
				errlog_data[1] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS1, 0x4);
				errlog_data[2] = (uint) hfc_read_reg(ap, HFC_IOSPACE_ERRDETAIL0, 0x4);

				dmp_data[0]  = (uchar)(errlog_data[0] >> 24);
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16);
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 );
				dmp_data[3]  = (uchar)(errlog_data[0]      );
				dmp_data[4]  = (uchar)(errlog_data[1] >> 24);
				dmp_data[5]  = (uchar)(errlog_data[1] >> 16);
				dmp_data[6]  = (uchar)(errlog_data[1] >> 8 );
				dmp_data[7]  = (uchar)(errlog_data[1]      );
				dmp_data[8]  = (uchar)(errlog_data[2] >> 24);
				dmp_data[9]  = (uchar)(errlog_data[2] >> 16);
				dmp_data[10] = (uchar)(errlog_data[2] >> 8 );
				dmp_data[11] = (uchar)(errlog_data[2]      );
				dmp_data[12] = (uchar)(errlog_data[3] >> 24);
				dmp_data[13] = (uchar)(errlog_data[3] >> 16);
				dmp_data[14] = (uchar)(errlog_data[3] >> 8 );
				dmp_data[15] = (uchar)(errlog_data[3]      );


				hfc_logout(ap, (uint)0x35, HFC_ERRLOG_TYPE_IMLLOG);

				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x35, dmp_data, 16) ;
			}
			/* Post-check error? */
			else if (post_chkerrflg == 1) {
				HFC_DBGPRT(  "CONFIG_HW_set_five() - post_chkerrflg=1, errcode = 0x%x.", err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));
				
				/* Read POST information */
				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 0x32);

				/* Create error log data */
				dmp_data[0]  = (uchar)(errlog_data[0] >> 24) ;
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16) ;
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 ) ;
				dmp_data[3]  = (uchar)(errlog_data[0]	   ) ;

				hfc_logout(ap, (uint)0x36, HFC_ERRLOG_TYPE_IMLLOG);

				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x36, dmp_data, 16) ;
			}

			/* Abnormal End */
			return(-1);
		}
		/* Forced MCK */
		hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, 0x1, 0x08);

		/* Reset all interrupt factor */
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, 0xffffffff );      /*  FCLNX-0172 */

		if (HFC_MMODE_CHECK_SHADOW(ap)){				/* FCLNX-GPL-317 */
			if( ap->port_no == 0 ){
				hfc_write_reg_ext(ap, ( uint )0x10a0,( char )0x4, 0x00000000 );
			}
			else if( ap->port_no == 1 ){
				hfc_write_reg_ext(ap, ( uint )0x10b0,( char )0x4, 0x00000000 );
			}
		}												/* FCLNX-GPL-317 */

		/* CTLRES */
		hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x02);

		/* 1ms wait */
		mdelay(1);

		/* Initiate reboot */ /* FCLNX-GPL-274 start *//* FCLNX-GPL-309 start */
		if( HFC_MMODE_CHECK_MLPF(ap) )
		{	
			/* Clear Communication area of WS. */
			hfc_reset_start(ap, HFC_WSCA_CLEAR);
			/* We reboot the same as MCK recovery. */
			hfc_reset_start(ap, HFC_REBOOT);
		}
		else
		{	/* Basic */
			hfc_write_reg(ap, HFC_IOSPACE_CMDBOOT, 0x1, 0x40);
		}
		/* FCLNX-GPL-274 end *//* FCLNX-GPL-309 end */

		if( HFC_MMODE_CHECK_MLPF(ap) )                                  /* FCLNX-0428 */
			hfc_reset_start(ap, HFC_SET_MLPF_MODE);

		/* Wait 500ms */
		mdelay(500);
    }

	exe_reset = 0;

	HFC_EXIT("hfc_config_hw_set_five");

    return(0);
}


int hfc_query_pktype(struct adap_info *ap)
{
	uchar	wk_iport;
	uchar	wk_one_core;
	int 	func_rc = 0; /* This Function return "func_rc". */

	HFC_ENTRY("hfc_query_pktype");
	
	HFC_DBGPRT( " memory map base addr = %lx\n", ap->mem_base_addr);

	/* Read package code from adr = 0x005	*/
	ap->pkg.code = (uchar) hfc_read_reg_ext(ap, 0x005, 0x1);

	HFC_DBGPRT( " package code = %01x\n", ap->pkg.code);

	if( ap->pkg.type == HFC_PKTYPE_FPP )
	{
		/* Set number of port */
		ap->pkg.port = 1;
		/* Set core number */
		ap->pkg.core_no = 0;
		/* Set One Core Mode */ /* FCLNX-GPL-233 */
		ap->pkg.one_core = TRUE;
	}
	else if(ap->pkg.type == HFC_PKTYPE_FIVE)
	{
		/* Read port number from addr = 0x01E */
		wk_iport = (uchar) hfc_read_reg_ext(ap, 0x01E, 0x1);
		wk_iport = (wk_iport & 0x30) >> 4;

		switch (wk_iport) {
			case 0x0:
				ap->pkg.port = 1;
				break;
			case 0x1:
				ap->pkg.port = 2;
				break;
			case 0x3:
				ap->pkg.port = 4;
				break;
			default:
				/* This "wk_iport" is unkown.  */
				func_rc = -1;
				break;
		}
		
		/* Set core number */
		ap->pkg.core_no = 0;
		/* Set One Core Mode */ /* FCLNX-GPL-233 */
		ap->pkg.one_core = TRUE;
	}
	else /* FIVE-EX */
	{
		/* Read form "0x01e(HWINF)".*/
		wk_iport = (uchar)hfc_read_reg(ap, HFC_IOSPACE_HWINF, 0x1);
		if (!(wk_iport & 0x10)) {
			ap->pkg.port = 1;
		}
		else{
			ap->pkg.port = 2;
		}

		/* Set core number */
		ap->pkg.core_no = (uchar)hfc_read_reg(ap, HFC_IOSPACE_CHNO, 0x1);
		/* Set LSI revision */ /* FCLNX-GPL-220 */ /* FCLNX-GPL-283 */
/*		ap->pkg.lsi_rev = (uchar)hfc_read_reg(ap, HFC_IOSPACE_LSIREV, 0x01); */

		/* Check One Core Mode */ /* FCLNX-GPL-233 start */
		wk_one_core = (uchar)hfc_read_reg(ap, HFC_IOSPACE_HWINF, 0x1);
		if ( wk_one_core & 0x20 )
		{	/* One core mode */
			ap->pkg.one_core = TRUE;
		}
		else
		{	/* Dual core mode */
			ap->pkg.one_core = FALSE;
		}
		/* FCLNX-GPL-233 end */
	}

	HFC_EXIT("hfc_query_pktype");
	
	return func_rc;
}


int hfc_release_adp(struct adap_info *ap)
{

	uint				i;
	int exec;
	struct target_info	*target;
//	struct wtimer		*w_timer;
//	struct dev_info		*dev;

	HFC_ENTRY("hfc_release_adp");

	/* A stop check of a target */
	for (i=0;i<ap->max_target;i++) {
		target = ap->target_arg[i];

		if (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)) {
			exec = hfc_clear_target_info( ap, target, TRUE, FALSE );
			
			if (exec)
				return(exec);											/* FCLNX-0459 */
		}
	}

	if ((test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)	|| 
		test_bit(HFC_WAIT_NMSRV, (ulong *)&ap->status)		||
		test_bit(HFC_WAIT_GPNID, (ulong *)&ap->status)		|| 
		test_bit(HFC_MCK_RECOVERY, (ulong *)&ap->status) ) &&
		(!test_bit(HFC_SUSPEND, (ulong *)&ap->status))){				/* FCLNX-GPL-306 */
		return(9);														/* FCLNX-0459 */
	}

	if((!test_bit(HFC_ISOL, (ulong *)&ap->status))){
		/* Set interrupt mask OFF */
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0 );

		/* Set close(offline) request (Set 0x42000000 to E_FRAMEA) */
		hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_OFFLINE);
	}
	
	/* Cancel mailbox request */
	unlock_mailbox(ap);													/* FCLNX-GPL-318 */

	/* Stop timer (adap_info) */
	hfc_reset_all_timer(ap);											/* FCLNX-GPL-353 */

//	set_bit(HFC_DETACH, (ulong *)&ap->attach_status);
//	clear_bit(HFC_ATTACH, (ulong *)&ap->attach_status);

	if ( test_bit(HFC_ENABLE, (ulong *)&ap->status) ) {
		if(test_bit(HFC_SUSPEND, (ulong *)&ap->status)){	/* FCLNX-GPL-306 */
			ap->isol_detail = 0;
			ap->status = 0;
			set_bit(HFC_SUSPEND, (ulong *)&ap->status);		/* FCLNX-GPL-306 */
		}
		else{
			ap->status = 0;
		}
		set_bit(HFC_ENABLE, (ulong *)&ap->status);
	}
	else {
		ap->status = 0;
	}
	
	HFC_EXIT("hfc_release_adp");
	return (0);

}


int hfc_release(struct Scsi_Host *host)
{
	ulong				flags = 0;	/* FCLNX-0274 */
	struct adap_info	*ap;
	struct pci_dev		*pdev = NULL;
	int wait=0,pci_fail=0;
	uint				read_reg = 0 ;

	HFC_ENTRY("hfc_release");
	
	ap = (struct adap_info *)host->hostdata;
	/* If ap is not configured or opened, return (abend) */
	if ( ap == NULL ) {										/* FCWIN_0014 */
		return 0;
	}
	pdev = ap->pci_cfginf;

	HFC_ADAPLOCK_IRQSAVE(flags);

	/* Closing process is in progress */
	set_bit(HFC_WAIT_CLOSE, (ulong *)&ap->status);
	ap->initialize = 0;
	
	set_bit(HFC_DETACH, (ulong *)&ap->attach_status);		/* FCLNX-0459 */
	clear_bit(HFC_ATTACH, (ulong *)&ap->attach_status);		/* FCLNX-0459 */
	
	do {													/* FCLNX-0459 */
		wait=0;
		
		if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-204 */
			wait = hfc_manage_info.npubp->hfc_wait_mp_ioend(ap);
		}
		
		if (!wait) {
			wait = hfc_release_adp(ap);
		}
		
		/* sleep for a while */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		msleep(1);
		HFC_ADAPLOCK_IRQSAVE(flags);
	} while (wait);											/* FCLNX-0459 */
	
	HFC_ADAPUNLOCK_IRQRESTORE(flags); /* FCLNX-GPL-125 */
	
	read_reg = hfc_read_reg_ext( ap,(uint)0, (char)0x4) ;
	
	if( read_reg == 0xffffffff )
	{
		pci_fail = -1;
	}
	
	/* Release interrupts */
	hfc_free_interrupts(ap, ap->msi_flag, pci_fail);				/* FCLNX-GPL306 */
	
	HFC_ADAPLOCK_IRQSAVE(flags); /* FCLNX-GPL-125 */
	
	if(hfc_manage_info.hfcplus_enable)						/* FCLNX-0506 */
		hfc_manage_info.npubp->hfc_free_errcnt_info(ap);	/* FCLNX-0506 */

	HFC_ADAPUNLOCK_IRQRESTORE(flags);						/* FCLNX-GPL-0370 */

	/* Release memory area */
	hfc_detach(ap);

	pci_release_regions(ap->pci_cfginf);
	iounmap((void *)(ap->mem_base_addr));

	/* Remove adap_info from mp_adap_info */
	hfc_free_mpap(ap);

	set_bit(HFC_CLOSED, (ulong *)&ap->open_status);

	hfc_manage_info.adap_info_arg[ap->dev_minor] = NULL;					/* FCLNX-0317 */

//	if(hfc_manage_info.mp_adap_info == NULL){
	if ( (hfc_manage_info.mp_adap_info == NULL) &&
		 (hfc_manage_info.pp == NULL)) {
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )		/* FCLNX-GPL-FX-435 */
		unregister_chrdev(hfc_manage_info.major, "hfcldd");
#endif
		hfc_manage_info.instance = 0;
	}

	HFC_EXIT("hfc_release");
	
	return (0);

}


int hfc_start_adapter(struct adap_info *ap)
{
	struct mp_adap_info *mpap, *mpap_prev, *new_mpap;
	uint i;
	uchar	pkrev_format[17] = "ABCDEFGHJKLMNPQR",adap_id[16];					/* FCLNX-0274 */
	struct	hfc_vpd			*vpd_ptr=NULL;
	struct	hfc_vpd_five	*vpdf_ptr=NULL;
	struct	hfc_vpd_five_ex	*vpdex_ptr=NULL;
	unsigned long 			wk_buf;
	unsigned long long		wkp;
	uchar 					pkrev_data, buf[16], fail=0;
	int						dma_size;
	int 		err = 0;
	uint		status_bak = 0;
	ulong					flags = 0;
	uchar addr[4];											/* FCLNX-GPL-319 */
	uint wk;												/* FCLNX-GPL-319 */
	uint					hyp_status;						/* FCLNX-GPL-393 */
	uint					hvm_support = 0;
					 	
	HFC_ENTRY("hfc_start_adapter");

	/* Set fw_init_tbl */
	HFC_DBGPRT( " set hw init table \n");
	hfc_set_fw_init_tbl(ap);

	/* Initialize adap_id */
	HFC_DBGPRT( " initialize adap_id \n");
																	/* @MLPF  */
	if ( HFC_MMODE_CHECK_SHARED(ap) )
		hfc_mlpf_get_mmio_hg(ap, adap_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
	else {
		if (ap->pkg.code == 0x8f) {
			if( hfc_read_flash(ap, 0x20018, 8, &adap_id[0]) ){ /* only FC-GW FCLNX-0405 */
				return(1); /* FCLNX-GPL-116 */
			}
			if( hfc_read_flash(ap, 0x20028, 8, &adap_id[8]) ){ /* only FC-GW FCLNX-0405 */
				return(1); /* FCLNX-GPL-116 */
			}
		}
		else {
			if( hfc_read_flash(ap, 0x54, 4, addr)){			/* FCLNX-GPL-319 */
				return (1);
			}
			HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
			if(hfc_read_flash(ap, wk, 16, adap_id)){
				return(1);
			}
		}													/* FCLNX-GPL-319 */
	}																/* @MLPF  */

	/* Initialize mp_adap_info */

	/* SpinLock "hfc_manage_info" */ /* FCLNX-GPL-177 */
	HFC_DBGPRT( " start hfc manage info settings \n");
	spin_lock_irqsave(&hfc_manage_info.hfcmp_lock, flags);
	
	mpap_prev = NULL;
	mpap = hfc_manage_info.mp_adap_info;

	if( (ap->pkg.type == HFC_PKTYPE_FPP) ||
		(ap->pkg.type == HFC_PKTYPE_FIVE) )
	{
		while (mpap != NULL)
		{
			if ( !memcmp( mpap->adap_id, adap_id, sizeof(adap_id)) )
			{
				break;
			}
			mpap_prev = mpap;
			mpap = mpap->next;
		}
	}
	else /* FIVE-EX */
	{
		while (mpap != NULL)
		{
			if ( !memcmp( mpap->adap_id, adap_id, sizeof(adap_id)) )
			{
				if( mpap->core_no == ap->pkg.core_no )
				{
					break;
				}
			}
			mpap_prev = mpap;
			mpap = mpap->next;
		}
	}
	
	HFC_DBGPRT( " ** mpap null case \n");
	if ( mpap == NULL ) {

		/* Create new mp_adap_info area */
		HFC_DBGPRT( "allocate new mp adap info area\n");

		for (i=0;i<100;i++) {
			if ((new_mpap = (void *) hfc_kmalloc(ap, sizeof(struct mp_adap_info), GFP_ATOMIC)) )
				break;
			
			/* Retry after 100ms if allocation failed */
/*			mdelay(100); */
			schedule_timeout(HZ/10+1); /* Wait 100ms */ /* FCLNX-GPL-177 */
		}

		if (new_mpap == NULL) {
			/* Allocation failed after 100 times retries */
			hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x9B, NULL, 0) ;/* FCLNX-GPL-161 */
			/* Unlock "hfc_manage_info" */ /* FCLNX-GPL-177 */
			spin_unlock_irqrestore(&hfc_manage_info.hfcmp_lock, flags);
			return (1); 
		}
		HFC_DBGPRT( "hfcl_manage_info pointer = %lx\n", (ulong) new_mpap);

		/* Clear mp_adap_info */
		memset(new_mpap, 0, sizeof(struct mp_adap_info));
		init_waitqueue_head(&new_mpap->lock_event);

		HFC_DBGPRT( "allocate hw log area \n");
		/* Allocate hw_log area */
		for (i=0;i<100;i++) {
			if ((new_mpap->hw_log = ((uint *) hfc_kmalloc(ap, HFC_HWLOG_SIZE, GFP_ATOMIC))) )
				break;

			/* Retry after 100ms when allocation failed */
/*			mdelay(100); */
			schedule_timeout(HZ/10+1); /* Wait 100ms */ /* FCLNX-GPL-177 */
		}

		if (new_mpap->hw_log == NULL) {
		/* Allocation failed after 100 times retries */
			hfc_kfree(ap, new_mpap);
			/* Unlock "hfc_manage_info" */ /* FCLNX-GPL-177 */
			spin_unlock_irqrestore(&hfc_manage_info.hfcmp_lock, flags);
			return (1); 
		}

		memset(new_mpap->hw_log, 0, HFC_HWLOG_SIZE);		/* FCWIN-0107 */
		HFC_DBGPRT( "hw_log = %lx\n", (ulong) new_mpap->hw_log);

		/* Copy adap_id */
		memcpy(new_mpap->adap_id, adap_id, sizeof(adap_id));
		
		/* Copy core number */
		new_mpap->core_no =  ap->pkg.core_no; /* for FIVE-EX */
		
		if (mpap_prev == NULL) {
			hfc_manage_info.mp_adap_info = new_mpap;
		}
		else {
			mpap_prev->next = new_mpap;
			new_mpap->prev = mpap_prev;
		}
		hfc_manage_info.mp_adap_cnt++;
		mpap = new_mpap;
	}
	
	/* Unlock "hfc_manage_info" */ /* FCLNX-GPL-177 */
	HFC_DBGPRT( " finish hfc manage info settings \n");
	spin_unlock_irqrestore(&hfc_manage_info.hfcmp_lock, flags);

	/* Lock mpap */ /* FCLNX-GPL-177 */
	HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);

	HFC_DBGPRT( " ** enquee adap info to mp_adap_info \n");
	/* Enqueue adap_info into mp_adap_info */
	if (mpap->ap != NULL) {
		struct adap_info *wk_ap;
		wk_ap = mpap->ap;
		while (wk_ap->next != NULL) {

			if (wk_ap == ap)
				break;

			wk_ap = wk_ap->next;
		}

		if (wk_ap != ap) {
			wk_ap->next = ap;
			ap->prev = wk_ap;
			ap->next = NULL;
			mpap->port_cnt++;
		}
		else {/* Already enqueued? */	
			HFC_DBGPRT("hfcldd%d : Execute internal online.\n", ap->dev_minor);
		}
	}
	else {
		mpap->ap = ap;
		ap->prev = NULL;
		ap->next = NULL;
		mpap->port_cnt++;
	}

	ap->mp_adap_info = mpap;

	if (!test_bit(HFC_HWINIT_COMP, (ulong *)&mpap->status)) {	/* Initialization finished? */
		/* Lock mp_adap_info, set HW_INITIALIZE and release hfc_lock */
		set_bit(HFC_HW_INITIALIZE, (ulong *)&mpap->lock);
		mpap->locked_ap = ap;

		/* Set sys_rev */
																	/* @MLPF STR */
		if ( HFC_MMODE_CHECK_SHARED(ap) ) {
			uint wkint_sysrev1;
			uint wkint_sysrev2;

			wkint_sysrev1 = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_SYSREV0, 0x4 );
			HFC_4L_TO_4B(wkint_sysrev2,wkint_sysrev1);
			HFC_MEMCPY(buf,(uchar*)&wkint_sysrev2,4);
			
			HFC_4B_TO_4L(mpap->sys_rev, (*(uint*)buf));
		}
		else {
			if( hfc_read_flash(ap, 0, 4, buf) ){ /* FCLNX-GPL-116 */
				goto start_adapter_error_exit;
			}
			HFC_4B_TO_4L(mpap->sys_rev, (*(uint*)buf));
		}
																	/* @MLPF END */

		/* Set vpd information */
		{
		int skip=0;

																	/* @MLPF STR */
			if (!skip)
			{
				err = hfc_get_vpd( ap, mpap->vpd_buf );
				if(err) goto start_adapter_error_exit; 
				if (HFC_MMODE_CHECK_SHADOW(ap))
				{
					/* Shadow LPAR read VPD DATA from mp_adap_info and set it to MMIO-HG */
					hfc_mlpf_set_mmio_hg(ap, mpap->vpd_buf, HFC_IOHGSPC_VPDAREA, HFC_IOHGSPC_VPDAREA_LEN);
				}
					
			}
																	/* @MLPF END */
		}

		fail=0;
		if (ap->pkg.type == HFC_PKTYPE_FPP) {							/* FPP */
			/* Update vpd information */
			vpd_ptr = (struct hfc_vpd *)mpap->vpd_buf;
			strcpy(vpd_ptr->driver_ver, hfc_manage_info.package_ver);
			vpd_ptr->driver_len = (uchar) strlen(vpd_ptr->driver_ver);

			/* Read PK-REV (addr = 0x007) */
			pkrev_data = (uchar) hfc_read_reg(ap, HFC_IOSPACE_PKREV, 0x1);
			/* Store PK_REV into ec_value */
			vpd_ptr->ec_value[0] = pkrev_format[pkrev_data];
			vpd_ptr->fw_ver=mpap->sys_rev;
			strncpy(mpap->model_name,"HFC0201",16);						/* FCLNX-0292 */
			
			if ((hfc_config_hw_set(ap, HFC_CONFIG_HW_CHECK_RETRY)) != 0)
				fail=1;

			vpd_ptr->ww_name  = ap->ww_name;							/* FCLNX-0299 */
		}
		else if (ap->pkg.type == HFC_PKTYPE_FIVE) {	/* FIVE */
			uchar parts_value[HFC_IOHGSPC_PARTSNUM_LEN];
			memset(parts_value, 0,HFC_IOHGSPC_PARTSNUM_LEN);
			/* Update vpd information */
			vpdf_ptr = (struct hfc_vpd_five *)mpap->vpd_buf;
			strcpy(vpdf_ptr->driver_ver, hfc_manage_info.package_ver);
			vpdf_ptr->driver_len = (uchar) strlen(vpdf_ptr->driver_ver);

			/* Read PK-REV (addr = 0x007) */
			pkrev_data = (uchar) hfc_read_reg(ap, HFC_IOSPACE_PKREV, 0x1);
			vpdf_ptr->ec_level = pkrev_format[pkrev_data];
	
			i=0;
			while (cr_pn[i].pk_code && cr_pn[i].pk_port) {
				struct cr_PartsNumber *pn_ptr = (struct cr_PartsNumber *) &cr_pn[i];
				
				if ( (ap->pkg.code == pn_ptr->pk_code)
				  && (ap->pkg.port == pn_ptr->pk_port) ) {
					memset(buf,0,sizeof(buf));
					strncpy(buf, pn_ptr->header, 1);
					strncpy(buf+1, vpdf_ptr->pn_value, 5);
					strncpy(buf+6, pn_ptr->pk, 1);
					strncpy(buf+7, vpdf_ptr->pn_value+6, 2);
					strncpy(buf+9, pn_ptr->hwinf, 2);
					strncpy(vpdf_ptr->pn_value, buf, sizeof(vpdf_ptr->pn_value));
					strncpy(mpap->model_name, pn_ptr->model_name, 16);	/* FCLNX-0292 */
					break;
				}
				i++;
			}

			if (!cr_pn[i].pk_code || !cr_pn[i].pk_port) {	/* Not hit ? */				/* FCLNX-0263 */
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCF, NULL, 0) ;/* FCLNX-GPL-161 */
				goto start_adapter_error_exit;											/* FCLNX-0263 */
			}
			
			vpdf_ptr->fw_ver = mpap->sys_rev;
			
			if (HFC_MMODE_CHECK_SHADOW(ap))
			{
				strncpy(parts_value, vpdf_ptr->pn_value, HFC_IOHGSPC_PARTSNUM_LEN);
				hfc_mlpf_set_mmio_hg(ap, parts_value, HFC_IOHGSPC_PARTSNUM0, HFC_IOHGSPC_PARTSNUM_LEN);
			}
		
			if(hfc_manage_info.instance == 0) {
				HFC_DBGPRT(" mck_point = %d\n", ap->mck_point);
				if(ap->mck_point == HFC_BEFORE_POSTCHK) {        /* FCLNX-0533 */
					HFC_DBGPRT("call hfc_occurred_mck \n");
					hfc_occurred_mck(ap,ap->mck_point);
				}       /* FCLNX-0533 */
			}
	
			                                                                            /* FCLNX-0370 */
			if( !HFC_MMODE_CHECK_SHARED(ap) || 
			   ( HFC_MMODE_CHECK_SHADOW(ap) && ! HFC_MMODE_CHECK_REBOOT(ap) ) )
			{
				if ((hfc_config_hw_set_five(ap, HFC_CONFIG_HW_CHECK_RETRY)) != 0)
					fail=1;
			}                                                                           /* FCLNX-0370 */
			
			if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )             /* FCLNX-0386 */
			{
				
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				/* Read HYPER status before reset int_a_reg.     */
				if( hyp_status & ( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML | HFC_HG_HYPSTATUS_FMCK ) )
					hfc_mlpf_hwerr_int(ap, hyp_status);
			}
			
			if ( HFC_MMODE_CHECK_SHARED(ap) )                                           /* FCLNX-0384 */
			{
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);			/* FCLNX-GPL-393 */
				hvm_support = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
				
				if(hvm_support & HFC_HG_LPAR_ISOLATION_SUPPORT){
					if(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP)
						fail = 1;
				}
				else{
					if (!(hyp_status & HFC_HG_HYPSTATUS_ENABLE))	/* FCLNX-GPL-427 */
						fail = 1;
				}
			}
			vpdf_ptr->ww_name = ap->ww_name;							/* FCLNX-0299 */
		}
		else if(ap->pkg.type == HFC_PKTYPE_FIVE_EX) /* FIVE-EX */
		{
			uchar parts_value[HFC_IOHGSPC_PARTSNUM_LEN];
			memset(parts_value, 0,HFC_IOHGSPC_PARTSNUM_LEN);
			/* Update vpd information */
			vpdex_ptr = (struct hfc_vpd_five_ex *)mpap->vpd_buf;
			strcpy(vpdex_ptr->driver_ver, hfc_manage_info.package_ver);
			vpdex_ptr->driver_len = (uchar) strlen(vpdex_ptr->driver_ver);

			/* Read PK-REV (addr = 0x007) */
			pkrev_data = (uchar) hfc_read_reg(ap, HFC_IOSPACE_PKREV, 0x1);
			vpdex_ptr->ec_level = pkrev_format[pkrev_data];
			vpdex_ptr->fw_ver = mpap->sys_rev;
			
			/* We never need changing the format of PartsNumber(vpdex_ptr->pn_value). */
			
			/* Get Model Name from VPD */ /* FCLNX-GPL-94 */
			strncpy(mpap->model_name, vpdex_ptr->v0_value, vpdex_ptr->v0_len);
			
			if (HFC_MMODE_CHECK_SHADOW(ap))
			{
				strncpy(parts_value, vpdex_ptr->pn_value, HFC_IOHGSPC_PARTSNUM_LEN);
				hfc_mlpf_set_mmio_hg(ap, parts_value, HFC_IOHGSPC_PARTSNUM0, HFC_IOHGSPC_PARTSNUM_LEN);
			}
		
			if(hfc_manage_info.instance == 0) {
				HFC_DBGPRT(" mck_point = %d\n", ap->mck_point);
				if(ap->mck_point == HFC_BEFORE_POSTCHK) {        /* FCLNX-0533 */
					HFC_DBGPRT("call hfc_occurred_mck \n");
					hfc_occurred_mck(ap,ap->mck_point);
				}       /* FCLNX-0533 */
			}
			                                                                            /* FCLNX-0370 */
			if( !HFC_MMODE_CHECK_SHARED(ap) || 
			   ( HFC_MMODE_CHECK_SHADOW(ap) && ! HFC_MMODE_CHECK_REBOOT(ap) ) )
			{
				if ((hfc_config_hw_set_five_ex(ap, HFC_CONFIG_HW_CHECK_RETRY)) != 0)
					fail=1;
			}                                                                           /* FCLNX-0370 */
			
			if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )             /* FCLNX-0386 */
			{
				
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
				/* Read HYPER status before reset int_a_reg.     */
				if( hyp_status & ( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML | HFC_HG_HYPSTATUS_FMCK ) )
					hfc_mlpf_hwerr_int(ap, hyp_status);
			}
			
			if ( HFC_MMODE_CHECK_SHARED(ap) )                                           /* FCLNX-0384 */
			{
				hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);			/* FCLNX-GPL-393 */
				hvm_support = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
				
				if(hvm_support & HFC_HG_LPAR_ISOLATION_SUPPORT){
					if(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP)
						fail = 1;
				}
				else{
					if (!(hyp_status & HFC_HG_HYPSTATUS_ENABLE))	/* FCLNX-GPL-427 */
						fail = 1;
				}
			}
			vpdex_ptr->ww_name = ap->ww_name; /* FCLNX-0299 */
		}
		else {
			HFC_DBGPRT( "FindAdapter() - Invalid Package Type.");
			fail=1;
		}

		if (!fail) {
			dma_size = sizeof(dma_addr_t);
			wkp = ap->padr_init;
			if(dma_size == 8){
				wkp >>=32;
			}
			else{
				wkp = 0;
			}
			HFC_DBGPRT(" ** wkp = %llx, %llx dma_size = %d\n",
				(unsigned long long)wkp, (unsigned long long)ap->padr_init, dma_size);

			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x4, wkp);
			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x4, ap->padr_init);

			wk_buf = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x04);
			HFC_DBGPRT( "init addr = %lx",wk_buf);
			wk_buf = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x04);
			HFC_DBGPRT( "** %lx \n",wk_buf);

			hfc_write_reg(ap, HFC_IOSPACE_CA_FLAG, 0x1, 0x00);
			
//			ap->fw_support = (uint) hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1);	/* FCLNX-GPL-FX-366 */
			
			/* SET LINK_INI_OPT(0x31f) */
			/* FCLNX-GPL-FX-366 */
			hfc_reset_start(ap, HFC_SET_LINK_INI_OPT);
			
			if((hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1)) & 0x80)		/* FCLNX-GPL-311 */
				set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support);

			if((hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1)) & 0x20){		/* FCLNX-GPL-570 */
				set_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support);
				hfc_write_reg(ap, HFC_IOSPACE_LINK_INI_OPT, 0x1, HFC_DISABLE_LINKINI_DELAY);
			}																/* FCLNX-GPL-570 */

			if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )     /* FCLNX-0328 */
				hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_SHADOW_UP);
			else
			{
				if ( ap->isol_force == HFC_NO_FRC_ISOL ){				/* FCLNX-GPL-393 */
					/* Initiate firmware */
					hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_FW_START);
				}				
			}

			/* Normal end */
			set_bit(HFC_HWINIT_COMP, (ulong *)&mpap->status);
		}
		else {
			/* Abnormal end */
			set_bit(HFC_HWINIT_COMP, (ulong *)&mpap->status);
			set_bit(HFC_HWINIT_FAIL, (ulong *)&mpap->status);
			set_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ) ;	/* FCLNX-GPL-424 */

		}
	}
	else {

		if( ap->pkg.type == HFC_PKTYPE_FPP)
		{
			/* NOP */
		}
		else if( ap->pkg.type == HFC_PKTYPE_FIVE)
		{
			status_bak = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4 );
			
			if (status_bak & 0x00800000)
			{
				hfc_write_cnfg(ap, 0x06, 0x2, 0xffff);
				hfc_write_cnfg(ap, 0x6c, 0x4, 0xffffffff);
				hfc_write_cnfg(ap, 0x70, 0x4, 0x0fffffff);
				HFC_DBGPRT("exec pci reset.");
			}
			
			/* Reset interrupt factor */
			if( !HFC_MMODE_CHECK_SHARED(ap) || 
			   ( HFC_MMODE_CHECK_SHADOW(ap) && ! HFC_MMODE_CHECK_REBOOT(ap) ) )     /* FCLNX-0391 */
			{
				/* INT_A rst */
				hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, 0xffffffff );
			}
			
			vpdf_ptr = (struct hfc_vpd_five *)mpap->vpd_buf;
			vpdf_ptr->ww_name = ap->ww_name;
		}
		else /* FIVE-EX */
		{
			/* Reset interrupt factor */
			if( !HFC_MMODE_CHECK_SHARED(ap) || 
			   ( HFC_MMODE_CHECK_SHADOW(ap) && ! HFC_MMODE_CHECK_REBOOT(ap) ) )     /* FCLNX-0391 */
			{
				/* INT_A rst */
				hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, 0xffffffff );
				/* Clear PCI Err Status */
				hfc_clear_sticky_bit(ap);
			}
			
			vpdex_ptr = (struct hfc_vpd_five_ex *)mpap->vpd_buf;
			vpdex_ptr->ww_name = ap->ww_name;
		}
		
		if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )             /* FCLNX-0386 */
		{
			
			hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
			/* read HYPER status before reset int_a_reg.     */
			if( hyp_status & ( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML | HFC_HG_HYPSTATUS_FMCK ) )
				hfc_mlpf_hwerr_int(ap, hyp_status);
		}
		
		if ( HFC_MMODE_CHECK_SHARED(ap) )                                           /* FCLNX-0384 */
		{
			hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);			/* FCLNX-GPL-393 */
			hvm_support = hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
			
			if(hvm_support & HFC_HG_LPAR_ISOLATION_SUPPORT){
				if(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP)
					fail = 1;
			}
			else{
				if (!(hyp_status & HFC_HG_HYPSTATUS_ENABLE))	/* FCLNX-GPL-427 */
					fail = 1;
			}
		}
		
		if (!fail) {															/* FCLNX-0228 STR */
			dma_size = sizeof(dma_addr_t);
			wkp = ap->padr_init;
			if(dma_size == 8){
				wkp >>=32;
			}
			else{
				wkp = 0;
			}
			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x4, wkp);
			hfc_write_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x4, ap->padr_init);

			wk_buf = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_INIT_ADDR0, 0x04);
			HFC_DBGPRT( "init addr = %lx",wk_buf);
			wk_buf = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_INIT_ADDR1, 0x04);
			HFC_DBGPRT( "** %lx \n",wk_buf);

			hfc_write_reg(ap, HFC_IOSPACE_CA_FLAG, 0x1, 0x00);
			
//			ap->fw_support = (uint) hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1);	/* FCLNX-GPL-FX-366 */
			
			/* SET LINK_INI_OPT(0x31f) */
			/* FCLNX-GPL-FX-366 */
			hfc_reset_start(ap, HFC_SET_LINK_INI_OPT);

			if((hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1)) & 0x80)		/* FCLNX-GPL-311 */
				set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support);

			if((hfc_read_reg(ap, HFC_IOSPACE_FW_SUPPORT, 0x1)) & 0x20){		/* FCLNX-GPL-570 */
				set_bit(HFC_SUPPORT_LINKINI_DELAY, (ulong *)&ap->fw_support);
				hfc_write_reg(ap, HFC_IOSPACE_LINK_INI_OPT, 0x1, HFC_DISABLE_LINKINI_DELAY);
			}																/* FCLNX-GPL-570 */

			if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )     /* FCLNX-0328 */
				hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_SHADOW_UP);
			else
			{
				if ( ap->isol_force == HFC_NO_FRC_ISOL ){				/* FCLNX-GPL-393 */
					/* Initiate firmware */
					hfc_write_reg(ap, HFC_IOSPACE_FRAMEA, 0x4, HFC_FRAMEA_FW_START);
				}				
			}
			
		} /* FCLNX-0228 END */
		else{
			set_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ) ;	/* FCLNX-GPL-424 */
		}
		
	}
	
//	if ( (!hfc_manage_info.hfcplus_enable) ||
//	    (( hfc_manage_info.hfcplus_enable ) && ( ap->isol_force == HFC_NO_FRC_ISOL )) ){  /* - FCLNX-546 - */
	if (  ( !hfc_manage_info.hfcldd_mp_mod ) || /* FCLNX-GPL-349 */
		((  hfc_manage_info.hfcldd_mp_mod ) &&  ( ap->isol_force == HFC_NO_FRC_ISOL )) ){
	    
		set_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status);
	}
		
	if ( test_bit(HFC_HWINIT_FAIL, (ulong *)&mpap->status)){

		HFC_DBGPRT( " hfcldd : hfc_start_adapter - disable interrupt\n");
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, 0 ); 
		
		if( HFC_MMODE_CHECK_SHADOW(ap) )
		{
			hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD0, NULL, 0) ;/* FCLNX-GPL-161 */
			hfc_mlpf_change_state(ap, HFC_HG_HYPSTATUS_ENABLE, HFC_DISABLE_HYPER_STATE );
			hfc_write_hg_reg(ap, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_CSTPEND);     /* FCLNX-0388 */
		}

	}
	else {
		set_bit(HFC_ATTACH, (ulong *)&ap->attach_status);
	}																			/* FCLNX-0228 END */
	
	if ( ( HFC_MMODE_CHECK_SHARED(ap) )&&(test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status ))){
		if(hfc_mlpf_check_state(ap, HFC_HG_LPAR_LIVEMIG_SUPPORT, HFC_CHECK_HVM_SUPPORT )){	/* FCLNX-GPL-489*/
			hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )(HFC_MLPF_REC_END | HFC_MLPF_HWERR) );
		}
	}																					/* FCLNX-GPL-489*/

	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */
	HFC_EXIT("hfc_start_adapter");

    return (0); 

start_adapter_error_exit:

	HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */
	hfc_free_mpap(ap);
    return (1); 

}


int hfc_initialize(struct adap_info *ap, int immdt_cmd)
{
	unsigned long flags = 0;

	HFC_ENTRY("hfc_initialize");
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if ( !test_bit(HFC_ENABLE, (ulong *)&ap->status) ) { /* HBA initialization has not finished? */
		HFC_DBGPRT( "hfcldd : hfc_initialize- HFC_ENABLE=0.");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return TRUE;
	}

	if ( !test_bit(HFC_NEED_LINK_INIT, (ulong *)&ap->status) ) { /* Need link initialization? */
		HFC_DBGPRT( "hfcldd : hfc_initialize - HFC_NEED_LINK_INIT=0.\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return TRUE;
	}

	/* Start target detection */
	HFC_DBGPRT(" hfcldd : hfc_initialize - start target detection adap_status = %lx\n",(ulong)ap->status); 

	clear_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_WAIT_ISOL_LINKUP_CNT, (ulong *)&ap->status);
		hfc_w_stop( ap, HFC_WLINKUP_CNT_TMR );
	}
#endif	/* FCLNX-GPL-FX-424 */

	hfc_w_stop( ap, HFC_LINKUP_TMR );
	hfc_w_stop( ap, HFC_LINKUP2_TMR );						/* FCLNX-0241 */

	ap->initialize = 1;
	atomic_set(&ap->int_a_poll, 0); 						/* FCLNX_0029 */
	if ( hfc_issue_linkini(ap) ) {
		HFC_DBGPRT(" hfcldd : hfc_initialize - issue link initialize fail.\n");
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return TRUE;
	}

	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	if(hfc_manage_info.instance == 1) {
		if(ap->mck_point == HFC_BEFORE_LINKINITIALIZE) {	/* FCLNX-0533 */
			hfc_occurred_mck(ap,ap->mck_point);
		}													/* FCLNX-0533 */
	}

	/* Enable intterupt */
	HFC_DBGPRT( " hfcldd : hfc_initialize - enable interrupt\n");
																	/* @MLPF STR */
	if (HFC_MMODE_CHECK_SHARED(ap))
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type] );
	else
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask[ap->pkg.type] );
																	/* @MLPF END */

	HFC_DBGPRT(" hfcldd : hfc_initialize - interruptible sleep on (mailbox event) \n");

	if(!immdt_cmd){ /* FCLNX-0514 */
		hfc_sleep_on(&ap->init_event, &ap->int_a_poll );								/* FCLNX-0269 */
	}
	else{
		ap->initialize = 0;
	}
	ap->no_target = 0;										/* FCLNX-GPL-570 */
	
	if (hfc_manage_info.hfcldd_mp_mod) {					/* FCLNX-GPL-204 */
		int count=300000;				/* 5min (max) */	/* FCLNX-GPL-466 */

		while (count) {
			if ( !HFC_SEMAPHORE_LOCK(ap->sem) ) {
				hfc_manage_info.npubp->hfc_mp_scan_dev(ap);
				HFC_SEMAPHORE_UNLOCK(ap->sem) ;
				break;
			}

			msleep(1);
			count--;
		}

		if (!count) {
			HFC_ERRPRT("hfcldd%d : HFC_SEMAPHORE_LOCK is invalid.\n", ap->dev_minor);
		}													/* FCLNX-GPL-466 */
	}

	if(hfc_manage_info.instance == 1) {
		if(ap->mck_point == HFC_AFTER_LINKINITIALIZE) {		/* FCLNX-0533 */
			hfc_occurred_mck(ap,ap->mck_point);
		}													/* FCLNX-0533 */
	}

	HFC_ADAPLOCK_IRQSAVE(flags);
	atomic_set(&ap->int_a_poll, 0);							/* FCLNX_0029 */
	ap->initialize = 0;		

	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	HFC_EXIT("hfc_initialize");
	
	return TRUE;
	
}


void hfc_wwnverify_linkup (struct adap_info *ap, struct target_info *target, uint mb_resp_status, uint64_t ww_name )
{
	int i,j;
	int login_req = 0;
	uchar count;											
	int sc;
	uchar hit;
				
	HFC_ENTRY("hfc_wwnverify_linkup");

	if (mb_resp_status) {
		/* Link initialization has failed */
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
		return;
	}

	for(i=0 ; i<MAX_TARGET_PROBE ; i++)					/* FCWIN-0082STR*/
	{
		target = hfc_hash_target_valid(ap, i);
		if (target != NULL)
		{
			set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
			clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */

			if ( (ap->connect_type == HFC_SWITCH ) 			/* FCWIN-0146 */
				|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {/* FCWIN-0185 */
				set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);
				ap->next_gidpn = TRUE;

				/* FCLNX-GPL-038 */
//				hfc_issue_gidpn( ap, target );	/* Issue GID_PN */			
			}
			else {
				set_bit(HFC_WWN_VALID, (ulong *)&target->flags);				/* ??? */
				hfc_enque_login_req(ap, target);			/* FCWIN-0146 */
			}
		}
	}														/* FCWIN-0082END*/

	set_bit(HFC_LOGIN_DELAY, (ulong *)&ap->status );								/* FCLNX-0279 */
	hfc_watchdog_enter( ap, NULL, NULL, 0, HFC_LOGIN_DELAY_TMR, 0, 1);				/* FCLNX-0279 */
	hfc_watchdog_enter( ap, NULL, NULL, 0, HFC_LOGIN_DELAY_TMR, ap->login_wait, 0 );/* FCLNX-0279 */

	/* Clear target scan */
	HFC_DBGPRT( "target_scan = %lx\n",(ulong)ap->target_scan );
	memset(ap->target_scan,0, (sizeof(struct target_scan)*MAX_TARGET_PROBE) );

	switch (ap->connect_type) {
		case HFC_SWITCH :	/* FC-SW */
			HFC_DBGPRT("wwnverify_linkup() - Connect FC-SW.");
			set_bit(HFC_NEED_NMSRV, (ulong *)&ap->status);							/* request GID_FT */

			break;

		case HFC_AL :	/* FC-AL (without SW) */

			if (ap->scsi_id & 0x00ffff00) {											/** FCWIN-0185 **/
				HFC_DBGPRT("wwnverify_linkup() - Connect FC-AL (FC-SW).");
				set_bit(HFC_NEED_NMSRV, (ulong *)&ap->status);	/* request GID_FT */
				break;
			}																		/*@ FCWIN-0185 @*/

			HFC_DBGPRT("wwnverify_linkup() - Connect FC-AL(Non SW).");
			/* Create the list of SCSI_ID (HFC_WWN_VALID = 0 or 1) */
			if ( (hfc_read_val( ap->fw_init_p->fw_iocinfo.flag ) & HFC_POSMAP_VALID )
			  && (!(hfc_read_val( ap->fw_init_p->fw_iocinfo.flag ) & HFC_POSMAP_LISA)) ) {
				HFC_DBGPRT("wwnverify_linkup() - ***\n");
				sc=0;
				/* Set position_map valid */
				count = (int) hfc_read_val(ap->fw_init_p->pos_map[0]);
				for (i=0;i<count; i++) {
					if (ap->scsi_id != ap->fw_init_p->pos_map[i+1]) {
						ap->target_scan[sc].flags |= HFC_SCAN_SCV;
						ap->target_scan[sc].scsi_id = ap->fw_init_p->pos_map[i+1];
						sc++;
					}
				}
			}
			else {
				sc=0;
				HFC_DBGPRT("wwnverify_linkup() - @@@\n");
				/* Register all AL_PN number */
				for (i=0;i<127;i++) {
					if (ap->scsi_id != posmap_lisa[i+1]) {
						ap->target_scan[sc].flags  |= HFC_SCAN_SCV;
						ap->target_scan[sc].scsi_id = posmap_lisa[i+1];
						sc++;
					}
				}
			}
			login_req = FALSE;
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				hit=0;

				if (!(ap->target_scan[i].flags & HFC_SCAN_SCV))
					continue;
				
				for (j=0;j<(int) ap->max_target;j++) {

					target = ap->target_arg[j];
					if(target != NULL) {
						if (!test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags))
							continue;

						HFC_DBGPRT( "wwnverify_login() - target_arg = %d, %d flags = %d\n",
										i, j, (uint)target->flags);

						if (ap->target_scan[i].scsi_id == target->scsi_id) {
							/* Target_info has already exsist */
							HFC_DBGPRT( "wwnverify_linkup() - target info has already existed*** \n");
							HFC_DBGPRT( "(wwpn) ww_name (from mailbox) %llx, target_name = %llx\n",
													(unsigned long long)ww_name, (unsigned long long)target->ww_name );

							if( ww_name == 0){
								HFC_DBGPRT(" wwnverify_linkup() - ww-name has not changed \n");
								ap->target_scan[i].flags &= ~HFC_SCAN_SCV;
								hit=1;
								break; 
							}
							if( (ww_name != 0) && (target->ww_name == ww_name) ){
								HFC_DBGPRT(" wwnverify_linkup() - ww-name has changed \n");
								ap->target_scan[i].flags &= ~HFC_SCAN_SCV;
								hit=1;
								break; 
							}
						}
					}
				}

				if (!hit) {
					
					HFC_DBGPRT("wwnverify_linkup() - target did not hit \n");
					target = hfc_add_target_info( ap, ap->target_scan[i].scsi_id );
					if(target != NULL){
					/* request LOGIN */
					
					HFC_DBGPRT("wwnverify_linkup() - target existed \n");
						set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
						clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap,target);
						login_req = TRUE;
					}
					else{
						HFC_DBGPRT("wwnverify_linkup() - target info does not exist\n"); 
						/* There is no space for target_info */
						ap->target_scan[i].flags &= ~HFC_SCAN_SCV;      /* FCWIN-0148 */
					}
				}
				else {
					if ( !test_bit(HFC_WWN_VALID, (ulong *)&target->flags) ){
						set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
						clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
						hfc_enque_login_req(ap,target);
						login_req = TRUE;
					}
				}
			}
			if(login_req == FALSE){
				if(ap->initialize != 0){
					ap->no_target = 1;									/* FCLNX-GPL-570 */
//					hfc_wake_up(&ap->init_event,&ap->int_a_poll);		/* FCLNX-GPL-570 */
				}
				HFC_DBGPRT(" ap->initialize = %d\n", ap->initialize);
			}
			break;

		case HFC_PT2PT :							/* P2P */
			HFC_DBGPRT("wwnverify_linkup() - Connect P2P.");

			if (!ap->target_cnt) {
				/* Create target_info */
				target = hfc_add_target_info(ap, hfc_read_val( ap->fw_init_p->fw_iocinfo.p2p_port_id));

				set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
				clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);	/* FCLNX-GPL-038 */
				hfc_enque_login_req(ap,target);
			}
			else {
				target = ap->target_arg[0];
			}

			break;

		default :
			HFC_DBGPRT( "wwnverify_linkup() - Invalid connect_type=%d.",ap->connect_type);
	}

	HFC_EXIT("hfc_wwnverify_linkup");

}


void hfc_wwnverify_linkup_timeout(struct adap_info *ap, struct target_info *target, uint mb_resp_status)
{															  /* FCWIN-0082 */
	int i,rsp;

	HFC_ENTRY("hfc_wwnverify_linkup_timeout");

	if ( target != NULL ) {
		HFC_DBGPRT(  "wwnverify_linkup_timeout() - delete target (pseq#=%d).",target->pseq);

		/* Release specified target_info */
		if (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)) {
			rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );  			

		}
	}
	else {
		HFC_DBGPRT(  "wwnverify_linkup_timeout() - delete all target.");

		/* Release all target_info */
		for (i=0;i<(int)ap->max_target;i++) {
			target = ap->target_arg[i];
			if(target != NULL) {
				if (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)) {
					rsp = hfc_clear_target_info( ap, target, TRUE, TRUE );
					
				}
			}
		}
	}

	HFC_EXIT("hfc_wwnverify_linkup_timeout");

}


void hfc_wwnverify_gidft(struct adap_info *ap, struct target_info *target, uint mb_resp_status)
{
	struct FS_ACC *fs_acc = (struct FS_ACC *) ap->mb_parm;
	uint	scsi_id,i,port_num;
	int    no_target=1;
	int 	detect_own_scsi_id; /* FCLNX-GPL-117 */

	HFC_ENTRY("hfc_wwnverify_gidft");

	/* Clear target_can */
	memset(ap->target_scan,0,(sizeof(struct target_scan)*MAX_TARGET_PROBE));

	if (mb_resp_status) {
		/* GID_FT has failed. Impossible to detect target */
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
		HFC_DBGPRT( "wwnverify_gidft() - error end<mb_resp_status=0x%x>.",mb_resp_status);
		return;
	}

	/* Read device number */
	port_num = (uint) hfc_read_val( ap->mb->mb_resp.type.drvioctl2.gid_ft.port_number);
//	port_num = ( port_num < 508 ) ? port_num : 508 ;

	if (!port_num) {
		/* No target for this FC-SW */
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
		HFC_DBGPRT( "wwnverify_gidft() - non target");
		return;
	}

	if ( port_num > MAX_TARGET_PROBE ) port_num = MAX_TARGET_PROBE;			/* FCLNX-GPL-117 */


	HFC_DBGPRT(" port_num = %d\n",port_num);

	detect_own_scsi_id = 0;								/* FCLNX-GPL-117 */
	for (i=0;i<port_num;i++) {
		scsi_id = (uint) hfc_read_val( fs_acc->port_id[i] )& 0x00ffffff;        /* FCWIN-0276 */
		
		if (ap->filter_target == HFC_ENABLE_FILTERTGT) { /* FCLNX-GPL-491 Filtering Login Target */
			if ((ap->scsi_id & 0x00ffff00) == (scsi_id & 0x00ffff00)) { /* domain,area */
				/* AccessGateway switch same physical port */
				continue; 
			}
		}
		
		if(ap->scsi_id != scsi_id) {
			ap->target_scan[i].flags  |= HFC_SCAN_SCV | HFC_SCAN_NEED;	
			ap->target_scan[i].scsi_id = scsi_id;                           /* FCWIN-0276 */
			set_bit(HFC_NEED_GPNID, (ulong *)&ap->status);
			no_target=0;
//			if (port_num >= MAX_TARGET_PROBE)
//				break;

		}
		else{
			detect_own_scsi_id = 1;						/* FCLNX-GPL-117 */
		} 									/* FCLNX-GPL-117 */

		if ( i == (MAX_TARGET_PROBE - 2) ){					/* FCLNX-GPL-117 */
			if ( detect_own_scsi_id == 0 )					/* FCLNX-GPL-117 */
				break;							/* FCLNX-GPL-117 */
		} 									/* FCLNX-GPL-117 */

	}

	if(no_target){	/* FCLNX-0165 */
		HFC_DBGPRT(" target is not found\n");
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
	}


	HFC_EXIT("hfc_wwnverify_gidft");

}


void hfc_wwnverify_gpnid(struct adap_info *ap, struct target_info *target, uint mb_resp_status)
{															  /* FCWIN-0082 */
	struct target_scan *tscan,*next_scan=NULL;
	int hit=0,i,j;
	int no_target=0;

	HFC_DBGPRT("hfc_wwnverify_gpnid");

	HFC_DBGPRT("gpnid ** mb_resp_status = %x\n",mb_resp_status);

	for (i=0;i<MAX_TARGET_PROBE;i++) {
		tscan = &ap->target_scan[i];

		if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_WAIT))
							 == (HFC_SCAN_SCV | HFC_SCAN_WAIT) ) {

			HFC_DBGPRT("gpnid ** tscan->flags = %x\n", tscan->flags);
			
			if ( hfc_read_val( ap->mb->mb_init.type.drvioctl1.un.gpn_id.port_id )
					== tscan->scsi_id) {
				tscan->flags &= ~(HFC_SCAN_NEED | HFC_SCAN_WAIT);
				tscan->flags |= HFC_SCAN_COMP;
				
				if (!mb_resp_status) {		

					tscan->wwpn = hfc_read_val( 
						ap->mb->mb_resp.type.drvioctl1.gpn_id.port_name_hi );
					tscan->wwpn <<= 32;
					tscan->wwpn |= hfc_read_val( 
						ap->mb->mb_resp.type.drvioctl1.gpn_id.port_name_lo );
					HFC_DBGPRT("gpnid ** tscan->wwpn = %llx\n", (unsigned long long)tscan->wwpn);
				}
				else {
					/* Target is invalid */
					tscan->flags |= HFC_SCAN_FAIL;
					tscan->flags &= ~HFC_SCAN_SCV;				/* FCLNX-GPL-277 */
					HFC_DBGPRT("gpnid ** HFC_SCAN_FAIL\n");
				}
				hit++;
			}
		}

		if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_NEED))
				== (HFC_SCAN_SCV | HFC_SCAN_NEED)) {
			/* Store pointer of target_scan which will issue GPN_ID next */
			next_scan = tscan;
			set_bit(HFC_NEED_GPNID, (ulong *)&ap->status);
		}
	}

	HFC_DBGPRT(" hit = %d",hit);


	if ( !next_scan ) {	/* Find target to issue GPN_ID next */
		HFC_DBGPRT("next_scan finished \n");
		no_target=1;

		for (i=0;i<MAX_TARGET_PROBE;i++) {
			tscan = &ap->target_scan[i];

			if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_COMP | HFC_SCAN_FAIL))
							  == (HFC_SCAN_SCV | HFC_SCAN_COMP)) {
				
				int new=TRUE;

				for (j=0;j<(int)(ap->max_target);j++) {
					target = ap->target_arg[j];

					if(target != NULL){
						/* If target has already exists, no need to make new target. */
						if ( test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) 
							&& ( target->ww_name == tscan->wwpn ) ) {
						
							tscan->flags &= ~HFC_SCAN_SCV;
							new = FALSE;
							HFC_DBGPRT("target hit @@ \n");

							if (target->scsi_id != tscan->scsi_id)					/* FCWIN-0166 */
							{														/* FCWIN-0166 */
								set_bit(HFC_NEED_GIDPN, (ulong *)&target->status);	/* FCWIN-0166 */
								ap -> next_gidpn = TRUE;							/* FCWIN-0166 */
								no_target = 0;
							}														/* FCWIN-0166 */
							else if(!test_bit(HFC_WWN_VALID, (ulong *)&target->flags)){	/* ??? */
								set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
								clear_bit(HFC_NEED_CANCEL, (ulong *)&target->status);					/* FCLNX-GPL-038 */
								
								clear_bit(HFC_SCN_WLINKUP, (ulong *)&target->status);					/* FCLNX-GPL-038 */
								hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE);	/* FCLNX-GPL-038 */
								hfc_enque_login_req(ap,target);
								no_target = 0; 
							}
							
							break;
						}

						/* If this target is in creating process, no need to create new target */
						if (  test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags) &&
							  !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) 
							&& ( target->scsi_id == tscan->scsi_id ) ){
							tscan->flags &= ~HFC_SCAN_SCV;
							new = FALSE;
							no_target = 0;
							break;
						}	
					}
				}

				if ( new == TRUE ) {/* Make new target if there is no target with the same WWN */
					
					HFC_DBGPRT("add target scsi_id=%d wwpn= %llx\n", tscan->scsi_id, (unsigned long long)tscan->wwpn);

					if ( (target = hfc_add_target_info(ap, tscan->scsi_id)) != NULL ) {
						set_bit(HFC_NEED_LOGIN, (ulong *)&target->status);
						hfc_enque_login_req(ap,target);
						no_target = 0;
						
					} 
				}

			}
		}
	}

	HFC_DBGPRT("no_target = %d\n",no_target);

	if(no_target){
		HFC_DBGPRT(" verify_gpnid no_target; target is not found\n");
		if(ap->initialize != 0){
			HFC_DBGPRT("wake up detect forcefully\n");
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
	}

	HFC_DBGPRT("hfc_wwnverify_gpnid");
}


void hfc_wwnverify_login(struct adap_info *ap, struct target_info *target, uint mb_resp_status, uint64_t ww_name)
{
	int i=0,j,rtn,hit,tid=0,empty_hit=0;	/* FCLNX-GPL-0447 */

	HFC_ENTRY("hfc_wwnverify_login");

	HFC_DBGPRT( "wwnverify_login() login normal end \n");

	for (j=0;j<MAX_TARGET_PROBE;j++) {										/* FCWIN-0148 */
		if ( (ap->target_scan[j].flags & HFC_SCAN_SCV)
		  && (ap->target_scan[j].scsi_id == target->scsi_id) ){			  
			ap->target_scan[j].flags |= HFC_SCAN_LOGIN;			/* LOGIN has finished */
			HFC_DBGPRT( "wwnverify_login() - target has already logined");
		}
	}


	switch (mb_resp_status) {
	case 0 :
		HFC_DBGPRT( "wwnverify_login() check login timing \n");

		/* Is this target in making process?*/
		if ( test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags) &&		/* FCLNX-GPL-0447 */
			 !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags) ){
			empty_hit=0;
			
			/* If this target is not reserved with persistent binding, assign empty ID */
			HFC_DBGPRT( " allocate vacant id (without persistent binding) \n");
			
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				if ( !hfc_hash_target_valid(ap, i) ) {	/* Not allocated */
					empty_hit=1;
					tid = i;
					break;
				}
			}
			
			if (empty_hit) {
				target->target_id      = tid;
				target->ww_name        = hfc_read_val(ap->mb->mb_resp.type.drvioctl1.login.port_name);
				target->node_name      = hfc_read_val(ap->mb->mb_resp.type.drvioctl1.login.node_name);
				target->fc_class_mask  = (ushort) hfc_read_val(ap -> mb -> mb_resp.type.drvioctl1.login.class_mask);
				target->device_flags   = (ushort) hfc_read_val(ap -> mb -> mb_resp.type.drvioctl1.login.parameter );
				target->max_frame_size = (ushort) hfc_read_val(ap -> mb -> mb_resp.type.drvioctl1.login.max_frame_size);
				set_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags);
				ap->tid_map[tid]       = target->pseq;
				for (i=0;i<8;i++) {
					if (target->fc_class_mask & (0x80>>i))
						target->fc_class = 8-i;
				}
				
				target->ap             = ap;
				target->dev            = NULL;
				target->group_id       = 0xff;
				target->attribute      = 0xff;
				target->path_id        = 0xff;
				target->lg_target      = NULL;
				
				HFC_DBGPRT(" empty_hit : tid=%d, ww_name = %llx, node_name=%llx flag=%x, preq=%d\n",
						target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name,
						target->flags, target->pseq);
				
//				if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-204 */	/* FCLNX-GPL-0449 */
//					hfc_manage_info.npubp->hfc_write_retries(ap, target);
//				}
				
				if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-204 */
					hfc_manage_info.npubp->hfc_update_attribute(ap, target);
				}
			}
			else {
				/* LOGIN succeeded, but there is no target with specified target ID. */
				HFC_DBGPRT( "wwnverify_login() login succeeded, but the target id does not exist\n");
				HFC_DBGPRT("    tid=%d ww_name=%llx, node_name=%llx, flag=%x, pseq=%d\n",
							target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name, target->flags, target->pseq);
				rtn = hfc_clear_target_info( ap, target, TRUE, TRUE );
			}
		}	/* FCLNX-GPL-0447 */
		
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status ) ) {
			set_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
			/* FCLNX-GPL-205 */
			atomic_set(&ap->rport_event_wait, 1);	/* FCLNX-GPL-259,565 */
			wake_up_interruptible(&ap->rport_event);							/* FCLNX-GPL-259 */
		}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
		break;


	case SCS_LOGIN_WWCHG :							/* WWN mismatch */
		HFC_DBGPRT("wwnverify_login() - login ww_name change.");

		/* LOGIN succeeded, but target->ww_name does not match with WNN in LOGIN parameter */
		rtn = hfc_clear_target_info( ap, target, TRUE, TRUE );

		hfc_wwnverify_linkup(ap, NULL, 0, ww_name);			/* Search target once more */
		break;

	default :
		/* Remove target_info due to abnormal end of LOGIN process*/
		HFC_DBGPRT( "wwnverify_login() - login fail.");
		hfc_clear_target_info( ap, target, TRUE, TRUE );	
	}

	hit = 0;
	for (j=0;j<MAX_TARGET_PROBE;j++) {
		if ( (ap->target_scan[j].flags & (HFC_SCAN_SCV | HFC_SCAN_LOGIN)) == HFC_SCAN_SCV)
			hit = 1;	/* There remain targets needs LOGIN */
	}

	if (!hit){
		HFC_DBGPRT(  "set ap initialize = 0\n");
		if(ap->initialize != 0){
			hfc_wake_up(&ap->init_event,&ap->int_a_poll);
		}
	}
	
	HFC_EXIT("hfc_wwnverify_login");

}


void hfc_wwnverify_scn(struct adap_info *ap, struct target_info *target, uint mb_resp_status)
{

	HFC_ENTRY("hfc_wwnverify_scn");

	if (    (ap->connect_type == HFC_SWITCH ) /* FC-SW ? */
		|| ((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00)) ) {/* FCWIN-0185 */
		if (!test_bit(HFC_WAIT_NMSRV, (ulong *)&ap->status) ) {
			set_bit(HFC_NEED_NMSRV, (ulong *)&ap->status);		/* Reserve GID_FT */
		}
	}

	HFC_EXIT("hfc_wwnverify_scn");
}


struct target_info *hfc_add_target_info( struct adap_info *ap, uint64_t scsi_id )
{
	struct target_info *target;
	uint pseq;

	HFC_ENTRY("hfc_add_target_info");

	if ( (pseq = hfc_uniq_seq_num( ap )) == 0xFFFFFFFF )
		return (NULL);

	target = ap->target_arg[pseq];

	memset( target, 0, sizeof(struct target_info) );
	set_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags);
	target -> pseq      = (uchar) pseq;
	target -> scsi_id   = (uint64_t) scsi_id;
	ap->target_cnt++;


	HFC_DBGPRT( " target exist target = %lx\n", *((ulong *)&target));
	HFC_DBGPRT( "target  = %d, %d, %d \n",(uint)target->flags, (uint)target->pseq, (uint)target->scsi_id);

	HFC_EXIT("hfc_add_target_info");

	return (target);
}



int hfc_clear_target_info( struct adap_info *ap, struct target_info *target, int check, int pmsg )
{
	uint tid,pseq;
	struct dev_info		*dev=NULL;

	HFC_ENTRY("hfc_clear_target_info");

	tid  = target->target_id ;
	pseq = target->pseq ;
	
	set_bit(HFC_WAIT_TARGET_STOP, (ulong *)&target->status);	/* Set target-stop-waiting bit */

	if (check == TRUE) {
		dev = target->dev;										/* FCLNX-GPL-0343 */
		while( dev != NULL){
			if ( (dev->lustat & HFC_WAIT_ABORT)		||
				 (dev->lustat & HFC_NEED_ABORT)		||
				 (dev->lustat & HFC_WAIT_LUN_RESET)	||
				 (dev->lustat & HFC_NEED_LUN_RESET) ){
					HFC_DBGPRT("clear_target_info() - lustat[%d] = 0x%x.", dev->lun, dev->lustat);
					return (1);
			}
			dev = dev->next;
		}														/* FCLNX-GPL-0343 */

//		for (i=0;i<MAX_DEV_CNT;i++) {
//			if ( target->lustat[i] ) {

//				HFC_DBGPRT("clear_target_info() - lustat[%d] = 0x%x.", i, target->lustat[i]);
//				return (1);
//			}
//		}

		if ( target -> we_que_cnt ) {

			HFC_DBGPRT("clear_target_info() - we_que_cnt = 0x%x.", target -> we_que_cnt);
			return (2);		/* SCSI initiation process is in progress */
		}

		if ( target -> wx_que_cnt ) {

			HFC_DBGPRT("clear_target_info() - wx_que_cnt = 0x%x.", target -> we_que_cnt);
			return (3);		/* Waiting SCSI response */
		}


		while (( test_bit(HFC_WAIT_LOGIN, (ulong *)&target->status)		|| 
				test_bit(HFC_WAIT_CANCEL, (ulong *)&target->status)		|| 		/* FCLNX-GPL-038 */
				test_bit(HFC_WAIT_PDISC, (ulong *)&target->status)		|| 
				test_bit(HFC_WAIT_TARGET_RESET, (ulong *)&target->status) ||
				test_bit(HFC_WAIT_GIDPN, (ulong *)&target->status) )	&&
				(!test_bit(HFC_SUSPEND, (ulong *)&ap->status))){				/* FCLNX-GPL-306 */
				
			HFC_DBGPRT( "clear_target_info() - target->status = 0x%x.", target->status);
			return (4);		/* Other process is using mailbox now */
		}

	}

	if ((hfc_manage_info.hfcldd_mp_mod)&&(!test_bit(HFC_SUSPEND, (ulong *)&ap->status))) {		/* FCLNX-GPL-204 *//* FCLNX-GPL-306 */
		hfc_manage_info.npubp->hfc_fo_check_and_offline(target, pmsg);
	}

	/* Delete Target_info */
	hfc_watchdog_enter(ap, target, NULL, 0, HFC_DELAY_TMR,      0, TRUE);	/* Cancel Timer */
	hfc_watchdog_enter(ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0, TRUE);	/* Cancel Timer */

	hfc_deque_next_dstart( ap, target );		/* Release Next dstart waiting state FCLNX-GPL-453 */
	hfc_deque_login_req( ap, target );			/* Release LOGIN waiting state */
	hfc_deque_pdisc_req( ap, target );			/* Release PDISC waiting state */
	target->status = 0 ;
	clear_bit( HFC_NEED_TARGET_RESET, (ulong *)&target->target_reset );		/* FCLNX-GPL-036 */
	clear_bit(HFC_WWN_VALID, (ulong *)&target->flags); /* temp */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status ) ) {
		set_bit( HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status );	/* FCLNX-GPL-206 */
		atomic_set(&ap->rport_event_wait, 1);		/* FCLNX-GPL-259,565 */
		wake_up_interruptible(&ap->rport_event);						/* FCLNX-GPL-259 */
	}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	if ( (test_bit(HFC_TARGETINF_VALID, (ulong *)&target->flags)
		 && !test_bit(HFC_DEVFLG_VALID, (ulong *)&target->flags))
		|| test_bit(HFC_TARGET_GHOST, (ulong *)&target->flags) )					/* FCLNX-0405 */
	{
		hfc_release_seq_num(ap, target->pseq);
		target->flags = 0;
		ap->target_cnt--;
	}

	HFC_EXIT("hfc_clear_target_info");

	return (0);
}

/*
 * Function: hfc_info
 *
 * Purpose: Output driver infotmation to syslog
 *
 * Arguments:
 *  host  - Pointer to Scsi_Host
 *
 * Returns:
 *
 * Notes:   Pointer to output character string
 */
const char *hfc_info (struct Scsi_Host *host)
{
	static char buffer[512];
	struct hfc_vpd	*vpd_info;
	struct adap_info	*ap;
	uint	sys_rev=0;	/* FCLNX-GPL-112 *//* FCLNX-GPL-465 */

	ap = (struct adap_info *)host->hostdata;
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_info(host));
	}
	
	buffer[0] = '\0';

	vpd_info = (struct hfc_vpd *)ap->mp_adap_info->vpd_buf;

	memset(&buffer[0], 0, sizeof(buffer));

	sprintf(&buffer[strlen(buffer)], "Hitachi PCI to Fibre Channel Host Adapter: device %02x:%02x.%02x IRQ %d \n",
				ap->pci_cfginf->bus->number, 
				PCI_SLOT(ap->pci_cfginf->devfn),
				PCI_FUNC(ap->pci_cfginf->devfn),
				ap->pci_cfginf->irq);
	if( ap->mp_adap_info != NULL ){
		sys_rev = ap->mp_adap_info->sys_rev;	/* FCLNX-GPL-112 *//* FCLNX-GPL-465 */
	}
	sprintf(&buffer[strlen(buffer)], "      Firmware version %x, Driver version %s \n",
				sys_rev, hfc_manage_info.package_ver);	/* FCLNX-GPL-112 */

//	sprintf(&buffer[strlen(buffer)], "  (%016lx)\n",jiffies);									/* FCLNX-0266 *//* FCLNX-GPL-474 */

	if(ap->defparam)     sprintf(&buffer[strlen(buffer)], "       default parameter is enabled\n");

	if(ap->narrowmap == 1)		sprintf(&buffer[strlen(buffer)], "       narrow mode(lun) is enabled\n");	/* DPM */
	else if(ap->narrowmap == 2)	sprintf(&buffer[strlen(buffer)], "       narrow mode(wwpn) is enabled\n");	/* FC-GW */

	sprintf(&buffer[strlen(buffer)], "        hfcl%d-wwpn=0x%llx",ap->instance,(unsigned long long)ap->ww_name);	/* FCLNX-GPL-474 */
	sprintf(&buffer[strlen(buffer)], "  (%016lx)\n",jiffies);														/* FCLNX-GPL-474 */
	
	if ( ( ap->isol_force == HFC_PRT_FRC_ISOL ) || (ap->isol_force == HFC_SHARED_PRT_FRC_ISOL ) ){			/* FCLNX-GPL-393 */
			sprintf(&buffer[strlen(buffer)], "        Adapter WWPN  0x%llx was forced to be isolated.\n",
			(unsigned long long)ap->ww_name);	
	}

	if ( ap->isol_force == HFC_CHKSTP_FRC_ISOL ){	/* FCLNX-GPL-147*/
			sprintf(&buffer[strlen(buffer)], "        Adapter WWPN  0x%llx was forced to be isolated.(CHK-STP).\n",
			(unsigned long long)ap->ww_name);
	}

	return (buffer);
}

 
#undef SPRINTF
// #define SPRINTF(args...) { if (pos < (buffer + length)) pos += sprintf (pos, ## args); }
#define SPRINTF(args...)	{  	len = snprintf (buf, 256, ## args); \
								if (len > 256) { \
									len  = sprintf (buf, "No output: not enough space available to output to string character\n"); \
								} \
								if (pos + len > offset + length){ \
									len = offset + length - pos; \
									if( proc_type != HFC_PROC_INFO_TYPE ){ \
										return(pos); \
									} \
								} \
								if (pos + len < offset) { \
									pos += len; \
								} \
								else { \
									if (pos < offset) { \
										partial = offset - pos; \
										pos += partial; \
										len  -= partial; \
										if (len > 0) { \
											memcpy(data, buf+partial, len); \
											pos += len; \
											data += len; \
										} \
									} \
									else if (len > 0) { \
										memcpy(data, buf, len); \
										pos += len; \
										data += len; \
									} \
								} \
							}

int
hfc_proc_info(char *buffer, char **start, off_t offset, int length, int hostno, int inout)
{
	struct Scsi_Host 	*host;
	struct adap_info 	*ap;
	int					i; /* FCLNX-GPL-177 */

	host = NULL;
	/* Find the specified Host */
	for(i=0; i<MAX_ADAP_CNT; i++) /* FCLNX-GPL-177 */
	{
		ap = hfc_manage_info.adap_info_arg[i];
		if(ap->hosts->host_no == hostno)
		{
			host = ap->hosts;
			return hfc_proc_info_k26(host, buffer, start, offset, length, inout);
		}
	}
	
	return -ESRCH;

}	

/* FCLNX-GPL-564 start */
/*
 * Function:    hfc_proc_info_com
 *
 * Purpose:
 *
 * Arguments:
 *  host		- Pointer to Scsi_Host
 *  buffer		- Data input/output pointer
 *  offset		- Offset from buffer start address
 *  length		- Buffer length
 *
 * Returns:   
 *    = 0		- No Data
 *    > 0       	- Data length
 * Notes:
 */
static int
hfc_proc_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length, uint proc_type)
{
	struct hfc_vpd			*vpd_info  = NULL;
	struct hfc_vpd_five		*vpdf_info = NULL;
	struct hfc_vpd_five_ex	*vpdex_info= NULL;
	struct pci_dev			*slot_dev  = NULL; /* FCLNX-GPL-180 */
	struct adap_info	*ap;
	struct target_info	*tp;
	short				vendor_id, device_id;
	int					i,j,hit;
	uchar				pre_conf=0;
	int					rtn;
	char				buf[256];
	char				*data = buffer;
	int					len, partial, pos = 0;
	struct target_info	*target;
	uint				status;	/* FCLNX-GPL-393 */
#ifdef HFC_HVM_DEBUG
	int size;
	uchar *tmp_cca;
#endif /* HFC_HVM_DEBUG */
	
	ap = (struct adap_info *)host->hostdata;
	
	SPRINTF ("Hitachi PCI to Fibre Channel Host Bus Adapter\n");
	
	if(ap->pkg.type == HFC_PKTYPE_FPP){
		vpd_info = (struct hfc_vpd *)ap->mp_adap_info->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpd_info->driver_ver, vpd_info->fw_ver);
	
	}
	else if(ap->pkg.type == HFC_PKTYPE_FIVE){
		vpdf_info = (struct hfc_vpd_five *)ap->mp_adap_info->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpdf_info->driver_ver, hfc_get_sysrev(ap));
		/* FCLNX-GPL-112 */
	}
	else {/* FIVE-EX */
		vpdex_info = (struct hfc_vpd_five_ex *)ap->mp_adap_info->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpdex_info->driver_ver, hfc_get_sysrev(ap));
		/* FCLNX-GPL-112 */
	}

	
	SPRINTF ("  Package_ID              = 0x%02x\n",ap->pkg.code);
	SPRINTF ("  Special file name       = hfcldd%d\n", ap->dev_minor);
	
	SPRINTF ("  Major_number            = %d\n", ap->dev_major);
	SPRINTF ("  Minor_number            = %d\n", ap->dev_minor);			
	SPRINTF ("  Instance_number         = %d\n",ap->instance);						
	SPRINTF ("  Host# = %d, Unique id   = %d \n", host->host_no, host->unique_id);
	SPRINTF ("  PCI memory space address= 0x%08llx (%d)\n", (unsigned long long)ap->mem_base_addr, (int)sizeof(dma_addr_t));
	
	SPRINTF("  Adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_read_cnfg(ap, 0x00, 0x2);
	device_id = (ushort) hfc_read_cnfg(ap, 0x02, 0x2 );
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	SPRINTF ("   Port name              =  %llx \n", (unsigned long long)ap->ww_name);
	SPRINTF ("   Node name              =  %llx \n", (unsigned long long)ap->node_name);
	SPRINTF ("   DID                    =  %06llx \n", (unsigned long long)ap->scsi_id);
	
	if ( ap->pkg.type == HFC_PKTYPE_FPP )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0x4B, 0x1);
	}
	else if( ap->pkg.type == HFC_PKTYPE_FIVE )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xAF, 0x1);
	}
	else /* FIVE-EX */
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xCA, 0x1);
	}
	
	if( (pre_conf == 0x01) || (pre_conf == 0x03) ){
		SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)ap->org_ww_name);
		SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)ap->org_node_name);
		SPRINTF ("   Add port name          =  %llx \n", (unsigned long long)ap->add_ww_name);
		SPRINTF ("   Add node name          =  %llx \n", (unsigned long long)ap->add_node_name);
		SPRINTF ("   hfc_pxe_boot           =  %d \n", hfc_pxe_boot);			
	}
	
	if ( HFC_MMODE_CHECK_MLPF(ap) ) {
		SPRINTF ("   MLPF MODE              =  %x \n", ap->mlpf_mode);	
		if ( !(HFC_MMODE_CHECK_DEDICATE(ap)) ) {
			SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)ap->org_ww_name);
			SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)ap->org_node_name);
		}
		SPRINTF ("   VFC port name          =  %llx \n", (unsigned long long)ap->vfc_ww_name);
		SPRINTF ("   VFC node name          =  %llx \n", (unsigned long long)ap->vfc_node_name);
		if ( !(HFC_MMODE_CHECK_DEDICATE(ap)) ) {
			SPRINTF ("   RID                    =  %x \n", ap->rid);
		}
	}
	
	SPRINTF ("   adapter ID             =  ");
	for(i=0; i<16; i++) SPRINTF("%02x",ap->mp_adap_info->adap_id[i]);
		SPRINTF("\n");
		
	SPRINTF ("   port number            =  %d\n", ap->port_no);
	
	if(ap->pkg.type == HFC_PKTYPE_FPP){
		char wkbuf[VPD_PN_LEN+13];  /* +13=NULL */						/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   manufacturer ID        =  ");
		for(i=0; i<vpd_info->mn_len;i++)
			 SPRINTF("%c",vpd_info->mn_value[i]);
		SPRINTF("\n");
		
		SPRINTF ("   parts number           =  ");
		memset(wkbuf,0,sizeof(wkbuf));									/* FCLNX-0337 *//* FCLNX-0368 */
		memcpy(wkbuf,vpd_info->pn_value,VPD_PN_LEN);					/* FCLNX-0337 *//* FCLNX-0368 */
		SPRINTF("%s\n",wkbuf);											/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   ec level               =  ");
		SPRINTF("%c",vpd_info->ec_value[0]);
		SPRINTF("\n");		
		
		SPRINTF ("   model name             =  %s \n", ap->mp_adap_info->model_name);	/* FCLNX-0329 */
		
	}else if(ap->pkg.type == HFC_PKTYPE_FIVE){
		char wkbuf[VPD_PN_LEN+13];  /* +13=NULL */						/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   manufacturer ID        =  ");
		for(i=0; i<vpdf_info->mn_len;i++)
				SPRINTF("%c",vpdf_info->mn_value[i]);
		SPRINTF("\n");
		
		SPRINTF ("   parts number           =  ");
		memset(wkbuf,0,sizeof(wkbuf));									/* FCLNX-0337 *//* FCLNX-0368 */
		memcpy(wkbuf,vpdf_info->pn_value,VPD_PN_LEN);					/* FCLNX-0337 *//* FCLNX-0368 */
		SPRINTF("%s\n",wkbuf);											/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   ec level               =  ");
		SPRINTF("%c",vpdf_info->ec_level);
		SPRINTF("\n");
		
		SPRINTF ("   model name             =  %s \n", ap->mp_adap_info->model_name);		/* FCLNX-0329 */
		SPRINTF ("   location               =  %02x:%02x.%02x\n", 							/* FCLNX-0404 */
												ap->pci_cfginf->bus->number,				/* FCLNX-0404 */
												PCI_SLOT(ap->pci_cfginf->devfn),			/* FCLNX-0404 */
												PCI_FUNC(ap->pci_cfginf->devfn));			/* FCLNX-0404 */
	}else { /* FIVE-EX */
		char wkbuf[VPD_PN_LEN+13];  /* +13=NULL */						/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   manufacturer ID        =  ");
		for(i=0; i<vpdex_info->mn_len;i++)
				SPRINTF("%c",vpdex_info->mn_value[i]);
		SPRINTF("\n");
		
		SPRINTF ("   parts number           =  ");
		memset(wkbuf,0,sizeof(wkbuf));									/* FCLNX-0337 *//* FCLNX-0368 */
		memcpy(wkbuf,vpdex_info->pn_value,VPD_PN_LEN);					/* FCLNX-0337 *//* FCLNX-0368 */
		SPRINTF("%s\n",wkbuf);											/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   ec level               =  ");
		SPRINTF("%c",vpdex_info->ec_level);
		SPRINTF("\n");
		
		SPRINTF ("   model name             =  %s \n", ap->mp_adap_info->model_name);		/* FCLNX-0329 */
		SPRINTF ("   location               =  %02x:%02x.%02x\n", 							/* FCLNX-0404 */
												ap->pci_cfginf->bus->number,				/* FCLNX-0404 */
												PCI_SLOT(ap->pci_cfginf->devfn),			/* FCLNX-0404 */
												PCI_FUNC(ap->pci_cfginf->devfn));			/* FCLNX-0404 */
	}
	
	slot_dev = hfc_get_slot_dev(ap); /* FCLNX-GPL-180 start */
	if(slot_dev != NULL)
	{
		SPRINTF ("   slot location          =  %02x:%02x.%02x\n",
										slot_dev->bus->number,
										PCI_SLOT(slot_dev->devfn),
										PCI_FUNC(slot_dev->devfn));
	} /* FCLNX-GPL-180 end */

	if( ap->raslog_install == 0 ){
		SPRINTF ("   Raslog version         =  raslog-%d.%d.%d-%d \n", ap->raslog_ver, ap->raslog_rev,
						ap->raslog_rver, ap->raslog_wver);
	}

	SPRINTF ("  Current Information \n");
	
	SPRINTF ("   Connection Type        = ");
	switch(ap->connect_type) {
		case HFC_SWITCH	:	if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){
								SPRINTF (" Point to Point (fabric) \n");
							}
							else{
								SPRINTF (" - \n");
							}
							break;
		case HFC_AL		:	if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){
								if (ap -> scsi_id & 0x00ffff00) {
									SPRINTF (" FC-AL (fabric) \n");
								}
								else{
									SPRINTF (" FC-AL \n");
								}
							}
							else{
								SPRINTF (" - \n");
							}
							break;
		
		case HFC_PT2PT	:	if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){
								SPRINTF (" Point to Point \n");
							}
							else{
								SPRINTF (" - \n");
							}
							break;
		default			:	SPRINTF (" - \n");
	}
	
	SPRINTF ("   Link Speed             = ");
	if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){
		SPRINTF (" %dGbps\n", (ap->max_data_rate/100));
	}
	else{
		SPRINTF (" - \n");
	}
	
	SPRINTF ("   Max Transfer Size      =  %dMB\n", (ap->dma_max/(1024*1024)) );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		SPRINTF ("   Link Down Time         =  %dsec\n", ap->dev_loss_tmo);
	} else {
		SPRINTF ("   Link Down Time         =  %dsec\n", ap->linkup_tmo);
	}
#else
	SPRINTF ("   Link Down Time         =  %dsec\n", ap->linkup_tmo);
#endif
	SPRINTF ("   Reset Delay Time       =  %dsec\n", ap->scsi_reset_delay);
	SPRINTF ("   Machine Check Retry Count  =  %d\n", ap->max_mck_cnt);			/* FCLNX_011 */
	SPRINTF ("   Preferred AL-PA Number =  %02x\n", ap->pref_alpa);				/* FCLNX_011 */
	SPRINTF ("   Reset Timeout          =  %dsec\n", ap->target_reset_tmo);  	/* FCLNX_011 */
	SPRINTF ("   Abort timeout          =  %dsec\n", ap->abort_tmo);			/* FCLNX_011 */
	SPRINTF ("   Queue depth            =  %d\n", ap->queue_depth);
	SPRINTF ("   Enable Message         =  %d\n", ap->wmsg);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", ap->dev_loss_tmo);
	} else {
		SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", ap->linkup2_tmo);				/* FCLNX-0241*/
	}
#else
	SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", ap->linkup2_tmo);				/* FCLNX-0241*/
#endif

	SPRINTF ("   Login delay            =  %dsec\n", ap->login_wait);				/* FCLNX-0243*/
	
	SPRINTF ("   Max target number      =  %d\n", ap->max_target);
	SPRINTF ("   Max xob number         =  %d\n", ap->xob_max);
	SPRINTF ("   Max xrb number         =  %d\n", ap->xrb_max);
	SPRINTF ("   Max soft log number    =  %d\n", ap->slog_max);
	SPRINTF ("   Max trace number       =  %d\n", ap->trc_max);
	SPRINTF ("   Max pkt number         =  %d\n", ap->pkt_num);
	SPRINTF ("   Max can queue          =  %d\n", ap->can_queue);
	SPRINTF ("   Max sg tablesize       =  %d\n", ap->sg_tblsize);
	SPRINTF ("   Max cmnd number        =  %d\n", ap->cmnd_num);
	SPRINTF ("   Minus timeout          =  %d\n", ap->minus_tout);
	SPRINTF ("   Scsi allowed           =  %d\n", ap->scsi_allowed);
	SPRINTF ("   Max cmd per lun        =  %d\n", ap->cmd_per_lun);
	SPRINTF ("   Max sectors            =  %d\n", ap->max_sectors);
	SPRINTF ("   Target reset flag      =  %d\n", ap->enable_tgtrst);
	SPRINTF ("   Mailbox Time-Out Retry =  %d\n", ap->to_reset_retry);		/* FCLNX-GPL-349 */
	SPRINTF ("   Login retry count      =  %d\n", ap->login_retry);					/* FCLNX-GPL-0343 */
	SPRINTF ("   Ioctl scsi cmd timeout =  %d\n", ap->ioctl_scsi_timeout);			/* FCLNX-GPL-0343 */
	SPRINTF ("   Els retry count        =  %d\n", ap->els_retry);					/* FCLNX-GPL-0343 */
	SPRINTF ("   Limit Log              =  %d\n", ap->limit_log);			/* FCLNX-GPL-491 */
	SPRINTF ("   Filtering Login Target =  %d\n", ap->filter_target);		/* FCLNX-GPL-491 */
	if ( HFC_MMODE_CHECK_MLPF(ap) ) {
		SPRINTF ("   Disable statistics     =  %d\n", ap->hg_stats_disable);	/* FCLNX-GPL-494 */
	}

#if _HFC_DEBUG_DET_00
	SPRINTF ("   Seg info trace mode    =  %d\n", ap->fw_parm);
#endif

#ifdef HFC_STAR
	SPRINTF ("   Log File Select        =  %d\n", ap->log_file); /* FCLNX-GPL-547,563 */
#endif /* HFC_STAR */

	/*SPRINTF ("   Enable failover       = no \n");  */				/* FCLNX_011 */
	SPRINTF ("   Total number of the     \n");
	SPRINTF ("        target devices    =  %d \n", ap->target_cnt);

	SPRINTF ("   Current adapter status =  %08x (",ap->status);				/* FCLNX_011 */
	if(test_bit(HFC_ENABLE, (ulong *)&ap->status)) SPRINTF ("enable ");		/* FCLNX_011 */
	if(test_bit(HFC_ONLINE, (ulong *)&ap->status)) SPRINTF ("online ");		/* FCLNX_011 */
	if(test_bit(HFC_CHK_STOP, (ulong *)&ap->status)) SPRINTF ("check stop");/* FCLNX_011 */
	SPRINTF (")\n");														/* FCLNX_011 */

	/* FCLNX-GPL-147*/
	SPRINTF ("   FC Port Status         =  ");								/* FCLNX_0538 */
	switch(hfc_get_adap_status(ap)){	/* FCLNX-GPL-428 */
		case HFC_WAITLINKUP:
			SPRINTF ("WaitLinkUp");									/* FCLNX-0488 */
			break;
		case HFC_LINKUP:
			SPRINTF ("LinkUp");										/* FCLNX-0488 */
			break;
		case HFC_CHKSTP_E:
			SPRINTF ("Isolate(CHK-STP)");						/* FCLNX-0592 *//* FCLNX-699 */
			break;
		case HFC_LINKDOWN:
			SPRINTF ("LinkDown");									/* FCLNX-0488 */
			break;
		case HFC_ISOLATE_C:
			SPRINTF ("Isolate(C)");											/* FCLNX-0488 */
			break;
		case HFC_ISOLATE_E:
			SPRINTF ("Isolate(E)");										/* FCLNX-0488 */
			break;
		case HFC_SFPFAIL:
			SPRINTF ("Isolate(SFP Fail)");								/* FCLNX-0488 */
			break;
		case HFC_SFPNOTSUPPORT:
			SPRINTF ("Isolate(SFP not support)");							/* FCLNX-0488 */
			break;
		case HFC_SFPDOWN:
			SPRINTF ("Isolate(SFP Down)");									/* FCLNX-0488 */
			break;
		default:
			SPRINTF ("Unknown status");										/* FCLNX-0488 */
			break;
	}
	SPRINTF ("\n");
	/* FCLNX-GPL-147*/
	
	SPRINTF ("   Path Change Queue Count =  %08x   \n",ap->retry_hfcp_count);
	
	SPRINTF ("   Current interrupt type = ");
	switch(ap->msi_flag){
		case HFC_INT_TYPE_INTX:
			SPRINTF ("Legacy Mode");
			break;
		case HFC_INT_TYPE_MSI:
			SPRINTF ("MSI Mode");
			break;
		case HFC_INT_TYPE_MSIX:
			SPRINTF ("MSI-X Mode");
			break;
		default:
			SPRINTF ("Invalid");
			break;
	}
	SPRINTF ("\n");

	SPRINTF ("  \n");
	
	if( proc_type == HFC_PROC_INFO_TYPE ){
		SPRINTF ("  Device Information \n");
		i=0;
		
		for(j=0; j<(ap->max_target); j++){
		
			tp = ap->target_arg[j];
			while( tp != NULL){

				if( test_bit(HFC_TARGETINF_VALID, (ulong *)&tp->flags)){

					SPRINTF ("    target id [");
					if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
						SPRINTF("%d",tp->target_id);
					}
					else{ 
						SPRINTF("-");
					}
					SPRINTF ("] :");
					if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
						SPRINTF (" port name = %llx node name = %llx DID = %02llx ",
							(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
					}
					else{
						SPRINTF (" port name =    -    node name =   -     DID = %02llx",(unsigned long long)tp->scsi_id );                            
					}
					
					i++;
				
					SPRINTF (" (pseq = %02d flags=%08x status=%08x)\n", tp->pseq, tp->flags, tp->status);
					
				}
				
				tp = tp->next;
			}
		}

		SPRINTF ("\n");
	}
	
	SPRINTF ("  FC persistent binding information \n");
	SPRINTF ("   automap is ");
	if(ap->automap == 1){
		SPRINTF (" ON (find configuration automatically) \n");
		
		for(i=0; i<MAX_ADAP_CNT; i++){
			if(hfc_manage_info.adap_bind[i] != -1){
				SPRINTF("   hfcl%d-wwpn=%llx\n",i,(unsigned long long)hfc_manage_info.adap_bind[i]);
			}
		}
	}
	else{
	
		SPRINTF (" OFF (establish configuration based on the presistent binding information\n");			
		SPRINTF ("  \n");

		hit=0;
		for(i=0; i<MAX_ADAP_CNT;i++){
			if(hfc_manage_info.adap_bind[i] != -1){
				SPRINTF ("   hfcl%d-wwpn=%llx\n", i, (unsigned long long)hfc_manage_info.adap_bind[i]);
				hit++;
			}
		}
	}
	
	if (hfc_manage_info.hfcldd_mp_mod) {					/* FCLNX-GPL-204 */
		data = hfc_manage_info.npubp->hfc_proc_info_option(ap, data, offset, length, &pos);
	}
	else
	{
		SPRINTF ("   HFC-PCM                       = OFF\n");
		if (hfc_manage_info.hfcplus_enable)                /* FCLNX-0642 */
                        SPRINTF("   E-Option                      = ON\n");             /* FCLNX-0642 */
	}
	
	/* for Debug Mode */ /* FCLNX-GPL-236 */
	if( ap->debug_func != 0x00 )
	{
		SPRINTF("\n");
		SPRINTF("  Debug information \n");
		SPRINTF("   Debug mode           = 0x%x\n", ap->debug_func);
	}
	/* for Debug Mode(mem leak check) */
	if( ap->debug_func & HFC_DEBUG_MEM_LEAK )
	{
		/* system */
		SPRINTF("   kmalloc_cnt          = %d\n", atomic_read(&hfc_manage_info.kmalloc_cnt));
		SPRINTF("   dma_alloc_cnt        = %d\n", atomic_read(&hfc_manage_info.dma_alloc_cnt));
		SPRINTF("   pci_alloc_cnt        = %d\n", atomic_read(&hfc_manage_info.pci_alloc_cnt));
		SPRINTF("   host_alloc_cnt       = %d\n", atomic_read(&hfc_manage_info.host_alloc_cnt));
		/* adapter port */
		for(i=0; i<hfc_manage_info.instance; i++){
			SPRINTF("   kmalloc_cnt_ap[%d]    = %d\n", i, atomic_read(&hfc_manage_info.kmalloc_cnt_ap[i]));
			SPRINTF("   dma_alloc_cnt_ap[%d]  = %d\n", i, atomic_read(&hfc_manage_info.dma_alloc_cnt_ap[i]));
			SPRINTF("   pci_alloc_cnt_ap[%d]  = %d\n", i, atomic_read(&hfc_manage_info.pci_alloc_cnt_ap[i]));
		}
	}
	
	if( ap->issue_d3hot != 0 )	/* FCLNX-GPL-306 */
	{
		SPRINTF("\n");
		SPRINTF("   Issue D3 Hot          = ON\n");
	}								/* FCLNX-GPL-306 */
	
	/* FCLNX-GPL-393 */
	SPRINTF ("   Isolate setting of HBA port:\n");
	if( HFC_MMODE_CHECK_SHADOW(ap) ){
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support)){
			SPRINTF ("    FW Support Bit                = On\n");
		}
		else{
			SPRINTF ("    FW Support Bit                = OFF\n");
		}
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)){
			SPRINTF ("    HVM Support Bit               = On\n");
		}
		else{
			SPRINTF ("    HVM Support Bit               = OFF\n");		
		}
	}
	if ( HFC_MMODE_CHECK_SHARED(ap) ){
		
		status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
		SPRINTF ("    Current Hyper status          = %08x\n",status);
		status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_LPARSTATUS, 0x4);
		SPRINTF ("    Current LPAR status           = %08x\n",status);
		status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
		SPRINTF ("    Current HVM SUPPORT bit       = %08x\n",status);
		status = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		SPRINTF ("    Current DRV SUPPORT bit       = %08x\n",status);
	}
	/* FCLNX-GPL-393 */
	
	if(((ap->ld_err_limit_s)||(ap->if_err_limit )||(ap->to_err_limit )||(ap->rt_err_enable))
	 && (!(hfc_manage_info.hfcldd_mp_mod))){	/* FCLNX-GPL-349 *//* FCLNX-GPL-434 */
		SPRINTF ("   Isolate setting of HBA port:\n");
		if (ap->hba_isolation == HFC_ISOL_START){						/* FCLNX-GPL-393 */
			SPRINTF ("    HBA Isolation                = On\n");
		}
		else{
			SPRINTF ("    HBA Isolation                = Off\n");
		}																/* FCLNX-GPL-349 */
		if (ap->ld_err_limit_s) {
			SPRINTF ("    Linkdown Error(S)            Limit:%d  Count:%d\n", ap->ld_err_limit_s, ap->ld_err_count_s);
		}
		if (ap->if_err_limit ) {
			SPRINTF ("    Interface Error              Limit:%d  Count:%d\n", ap->if_err_limit, ap->if_err_count);
		}
		if (ap->to_err_limit ) {
			SPRINTF ("    TimeOut Error                Limit:%d  Count:%d\n", ap->to_err_limit, ap->to_err_count);
		}
		if (ap->rt_err_enable) {
			if(ap->c_err == HFC_ISOLATE_RT){
				SPRINTF ("    TimeOutReset Error           = Enable (Error occurred)\n");
			}
			else{
				SPRINTF ("    TimeOutReset Error           = Enable (No error)\n");
			}
		}
		
		if( proc_type == HFC_PROC_INFO_TYPE ){
			if((ap->connect_type == HFC_SWITCH)||((ap->connect_type == HFC_AL) && (ap -> scsi_id & 0x00ffff00))) {
				for(i=0; i<(ap->max_target); i++){	
					target = hfc_hash_target_valid(ap, i);
					if( target != NULL){
						if (ap->ld_err_limit_s ){
							SPRINTF ("   Isolate setting of Target port(Target: %d,  WWPN: %llx):\n", target->target_id, (unsigned long long)target->ww_name);
							SPRINTF ("    Linkdown Error(S)            Limit:%d  Count:%d\n", ap->ld_err_limit_s, target->tgt_ld_err_count_s);
						}
					}
				}
			}
		}
	}																							/* FCLNX-GPL-349 */

#ifdef HFC_HVM_DEBUG
	if ( HFC_MMODE_CHECK_MLPF(ap) ) {
		if(ap->hg_cca_p != NULL){
			SPRINTF ("-- HG CCA Statisitcs -- \n");
			SPRINTF ("   version          =  0x%x\n", ap->hg_cca_p->version);
			SPRINTF ("   valid            =  0x%x\n", ap->hg_cca_p->valid);
			SPRINTF ("   size             =  %d\n", ap->hg_cca_p->size);
			SPRINTF ("   statistics_cnt   =  %lld\n", ap->hg_cca_p->statistics_cnt);
			SPRINTF ("   io_exec          =  %lld\n", ap->hg_cca_p->io_exec);
			SPRINTF ("   io_end           =  %lld\n", ap->hg_cca_p->io_end);
			SPRINTF ("   io_exec          =  %lld\n", ap->hg_cca_p->io_exec);
			SPRINTF ("   io_err           =  %lld\n", ap->hg_cca_p->io_err);
			SPRINTF ("   xob_full         =  %lld\n", ap->hg_cca_p->xob_full);
			SPRINTF ("   iov_full         =  %lld\n", ap->hg_cca_p->iov_full);
			SPRINTF ("   frame_full       =  %lld\n", ap->hg_cca_p->frame_full);
			SPRINTF ("   page_over        =  %lld\n", ap->hg_cca_p->page_over);
			SPRINTF ("   tx_frame         =  %lld\n", ap->hg_cca_p->tx_frame);
			SPRINTF ("   tx_word          =  %lld\n", ap->hg_cca_p->tx_word);
			SPRINTF ("   rx_frame         =  %lld\n", ap->hg_cca_p->rx_frame);
			SPRINTF ("   rx_word          =  %lld\n", ap->hg_cca_p->rx_word);
			
			
			SPRINTF ("-- HG CCA DUMP start -- \n");
		
			size = sizeof(struct hg_cca);
			tmp_cca = NULL;
			tmp_cca = ap->hg_cca_p;
			
			if(tmp_cca != NULL){
				for ( i = 0; j < ( size / 8 ); i+=8 ) {
					SPRINTF ("%02x%02x%02x%02x%02x%02x%02x%02x\n",
						tmp_cca[i],tmp_cca[i+1],tmp_cca[i+2],tmp_cca[i+4],
						tmp_cca[i+4],tmp_cca[i+5],tmp_cca[i+6],tmp_cca[i+7]);
				}
			}
		
		SPRINTF ("-- HG CCA DUMP end -- \n");
		}
	}
#endif

	SPRINTF ("  --- \n");
	
	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);
}

#if 0
static int
hfc_target_lu_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length)
{
	struct adap_info	*ap;
	struct target_info	*tp;
	int					i,j;
	int					rtn;
	char				buf[256];
	char				*data = buffer;
	int					len, partial, pos = 0;
	short				vendor_id, device_id;
	uchar				pre_conf=0;
	
	ap = (struct adap_info *)host->hostdata;
	
	SPRINTF ("Hitachi PCI to Fibre Channel Host Bus Adapter\n");
	SPRINTF ("  Special file name       = hfcldd%d\n", ap->dev_minor);
	
	SPRINTF ("  Major_number            = %d\n", ap->dev_major);
	SPRINTF ("  Minor_number            = %d\n", ap->dev_minor);			
	SPRINTF ("  Instance_number         = %d\n",ap->instance);						
	SPRINTF ("  Host# = %d, Unique id   = %d \n", host->host_no, host->unique_id);
	
	SPRINTF("  Adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_read_cnfg(ap, 0x00, 0x2);
	device_id = (ushort) hfc_read_cnfg(ap, 0x02, 0x2 );
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	SPRINTF ("   Port name              =  %llx \n", (unsigned long long)ap->ww_name);
	SPRINTF ("   Node name              =  %llx \n", (unsigned long long)ap->node_name);
	if ( ap->pkg.type == HFC_PKTYPE_FPP )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0x4B, 0x1);
	}
	else if( ap->pkg.type == HFC_PKTYPE_FIVE )
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xAF, 0x1);
	}
	else /* FIVE-EX */
	{
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xCA, 0x1);
	}
	
	if( (pre_conf == 0x01) || (pre_conf == 0x03) ){
		SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)ap->org_ww_name);
		SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)ap->org_node_name);
		SPRINTF ("   Add port name          =  %llx \n", (unsigned long long)ap->add_ww_name);
		SPRINTF ("   Add node name          =  %llx \n", (unsigned long long)ap->add_node_name);
	}
	
	if ( HFC_MMODE_CHECK_MLPF(ap) ) {
		SPRINTF ("   MLPF MODE              =  %x \n", ap->mlpf_mode);	
		if ( !(HFC_MMODE_CHECK_DEDICATE(ap)) ) {
			SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)ap->org_ww_name);
			SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)ap->org_node_name);
		}
		SPRINTF ("   VFC port name          =  %llx \n", (unsigned long long)ap->vfc_ww_name);
		SPRINTF ("   VFC node name          =  %llx \n", (unsigned long long)ap->vfc_node_name);
		if ( !(HFC_MMODE_CHECK_DEDICATE(ap)) ) {
			SPRINTF ("   RID                    =  %x \n", ap->rid);
		}
	}
	SPRINTF ("  \n");
	
	SPRINTF ("  Device Information \n");
	i=0;
	
	for(j=0; j<(ap->max_target); j++){
	
		tp = ap->target_arg[j];
		while( tp != NULL){

			if( test_bit(HFC_TARGETINF_VALID, (ulong *)&tp->flags)){

				SPRINTF ("    target id [");
				if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
					SPRINTF("%d",tp->target_id);
				}
				else{ 
					SPRINTF("-");
				}
				SPRINTF ("] :");
				if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
					SPRINTF (" port name = %llx node name = %llx DID = %02llx ",
						(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
				}
				else{
					SPRINTF (" port name =    -    node name =   -     DID = %02llx",(unsigned long long)tp->scsi_id );                            
				}
				
				i++;
				
				SPRINTF (" (pseq = %02d flags=%08x status=%08x)\n", tp->pseq, tp->flags, tp->status);
				
			}
				
			tp = tp->next;
		}
	}
	
	SPRINTF ("\n");
	
	
	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);
}
#endif

/* FCLNX-GPL-564 end */

/* FCLNX-GPL-564 start */
/*
 * Function:    hfc_proc_info_k26
 *
 * Purpose:
 *
 * Arguments:
 *  host		- Pointer to Scsi_Host
 *  buffer		- Data input/output pointer
 *  start		- Data output pointer
 *  offset		- Offset from buffer start address
 *  length		- Buffer length
 *  inout		- TRUE (RD), FALSE (WR)
 *
 * Returns:   
 *    < 0		- Error number
 *    >=0       	- Data length
 * Notes:
 */
int
hfc_proc_info_k26(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
{
	struct adap_info	*ap=NULL;
	
	if (!host){ /* This host is not found */
		return (-EINVAL);
	}
	if (inout) {
		return (length);
	}
	if (start) {
		*start = buffer;
	}
	
	ap = (struct adap_info *)host->hostdata;
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_proc_info_com(host, buffer, offset, length, HFC_PROC_INFO_TYPE));
	}
	
	return hfc_proc_info_com(host, buffer, offset, length, HFC_PROC_INFO_TYPE);
}
/* FCLNX-GPL-564 end */


int hfc_chk_conf_val(int min, int max, int val)
{
	if( val == -1) return 0;  /* not defined */
	if( (min <= val) && (val <= max) ){
		return (1);
	}

	return 0;

}


int hfc_chk_conf_ls(int min, int max, int val)
{
	if( val == -1) return 0;  /* not defined */
	if( (min <= val) && (val <= max) ){
		if(val == 0)  return (1); /* 0  : Auto   */
		if(val == 1)  return (1); /* 1  : 1Gbps  */
		if(val == 2)  return (1); /* 2  : 2Gbps  */
		if(val == 4)  return (1); /* 4  : 4Gbps  */
		if(val == 8)  return (1); /* 8  : 8Gbps  */
		if(val == 16) return (1); /* 16 : 16Gbps */
	}
	return 0;
}



int hfc_chk_conf_mt(int min, int max, int val)
{
	if( val == -1) return 0;  /* not defined */
	if( (min <= val) && (val <= max) ){
		if(val == 1) return (1); /* 1 :  1MB */
		if(val == 4) return (1); /* 4 :  4MB */
		if(val == 8) return (1); /* 8 :  8MB */
		if(val ==16) return (1); /*16 : 16MB */
		if(val ==32) return (1); /*32 : 32MB */
	}
	return 0;
}

void hfc_set_topology(struct adap_info *ap)
{
	uchar cnv[3]={HFC_UNKN,HFC_PT2PT,HFC_AL}; /* connection type */

	if(ap->defparam) return;

    /* 
	   Attention!! : The BIOS data might already been set. 
                                                           */

	if(hfc_chk_conf_val(1,2,hfc_connection_type)){ /* in case global parameter is set */
		ap->topology  = cnv[hfc_connection_type];	/* 1:P2P, 3:FC-AL */
	}

	/* in case local parameter is set */
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(1,2, hfc0_connection_type) ) ap->topology  =  cnv[hfc0_connection_type]; break;
		case  1 : if( hfc_chk_conf_val(1,2, hfc1_connection_type) ) ap->topology  =  cnv[hfc1_connection_type]; break;
		case  2 : if( hfc_chk_conf_val(1,2, hfc2_connection_type) ) ap->topology  =  cnv[hfc2_connection_type]; break;
		case  3 : if( hfc_chk_conf_val(1,2, hfc3_connection_type) ) ap->topology  =  cnv[hfc3_connection_type]; break;
		case  4 : if( hfc_chk_conf_val(1,2, hfc4_connection_type) ) ap->topology  =  cnv[hfc4_connection_type]; break;
		case  5 : if( hfc_chk_conf_val(1,2, hfc5_connection_type) ) ap->topology  =  cnv[hfc5_connection_type]; break;
		case  6 : if( hfc_chk_conf_val(1,2, hfc6_connection_type) ) ap->topology  =  cnv[hfc6_connection_type]; break;
		case  7 : if( hfc_chk_conf_val(1,2, hfc7_connection_type) ) ap->topology  =  cnv[hfc7_connection_type]; break;
		case  8 : if( hfc_chk_conf_val(1,2, hfc8_connection_type) ) ap->topology  =  cnv[hfc8_connection_type]; break;
		case  9 : if( hfc_chk_conf_val(1,2, hfc9_connection_type) ) ap->topology  =  cnv[hfc9_connection_type]; break;
		case 10 : if( hfc_chk_conf_val(1,2,hfc10_connection_type) ) ap->topology  = cnv[hfc10_connection_type]; break;
		case 11 : if( hfc_chk_conf_val(1,2,hfc11_connection_type) ) ap->topology  = cnv[hfc11_connection_type]; break;
		case 12 : if( hfc_chk_conf_val(1,2,hfc12_connection_type) ) ap->topology  = cnv[hfc12_connection_type]; break;
		case 13 : if( hfc_chk_conf_val(1,2,hfc13_connection_type) ) ap->topology  = cnv[hfc13_connection_type]; break;
		case 14 : if( hfc_chk_conf_val(1,2,hfc14_connection_type) ) ap->topology  = cnv[hfc14_connection_type]; break;
		case 15 : if( hfc_chk_conf_val(1,2,hfc15_connection_type) ) ap->topology  = cnv[hfc15_connection_type]; break;
		case 16 : if( hfc_chk_conf_val(1,2,hfc16_connection_type) ) ap->topology  = cnv[hfc16_connection_type]; break;
		case 17 : if( hfc_chk_conf_val(1,2,hfc17_connection_type) ) ap->topology  = cnv[hfc17_connection_type]; break;
		case 18 : if( hfc_chk_conf_val(1,2,hfc18_connection_type) ) ap->topology  = cnv[hfc18_connection_type]; break;
		case 19 : if( hfc_chk_conf_val(1,2,hfc19_connection_type) ) ap->topology  = cnv[hfc19_connection_type]; break;
		case 20 : if( hfc_chk_conf_val(1,2,hfc20_connection_type) ) ap->topology  = cnv[hfc20_connection_type]; break;
		case 21 : if( hfc_chk_conf_val(1,2,hfc21_connection_type) ) ap->topology  = cnv[hfc21_connection_type]; break;
		case 22 : if( hfc_chk_conf_val(1,2,hfc22_connection_type) ) ap->topology  = cnv[hfc22_connection_type]; break;
		case 23 : if( hfc_chk_conf_val(1,2,hfc23_connection_type) ) ap->topology  = cnv[hfc23_connection_type]; break;
		case 24 : if( hfc_chk_conf_val(1,2,hfc24_connection_type) ) ap->topology  = cnv[hfc24_connection_type]; break;
		case 25 : if( hfc_chk_conf_val(1,2,hfc25_connection_type) ) ap->topology  = cnv[hfc25_connection_type]; break;
		case 26 : if( hfc_chk_conf_val(1,2,hfc26_connection_type) ) ap->topology  = cnv[hfc26_connection_type]; break;
		case 27 : if( hfc_chk_conf_val(1,2,hfc27_connection_type) ) ap->topology  = cnv[hfc27_connection_type]; break;
		case 28 : if( hfc_chk_conf_val(1,2,hfc28_connection_type) ) ap->topology  = cnv[hfc28_connection_type]; break;
		case 29 : if( hfc_chk_conf_val(1,2,hfc29_connection_type) ) ap->topology  = cnv[hfc29_connection_type]; break;
		case 30 : if( hfc_chk_conf_val(1,2,hfc30_connection_type) ) ap->topology  = cnv[hfc30_connection_type]; break;
		case 31 : if( hfc_chk_conf_val(1,2,hfc31_connection_type) ) ap->topology  = cnv[hfc31_connection_type]; break;
	}

	/* in case local parameter is set */
	if( hfc_chk_conf_val( 1,2,hfcmp_connection_type[ap->instance]) ) {
		ap->topology  = cnv[ hfcmp_connection_type[ap->instance] ];
		HFC_DBGPRT("connect_type <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("connect_type = %d\n", ap->topology);

}


void hfc_set_linkspeed(struct adap_info *ap)
{


	if(ap->defparam) return;

	if(hfc_chk_conf_ls(1,8,hfc_link_speed)){	/* in case global parameter is set */
		ap->linkspeed = hfc_link_speed;			/* 1:1Gbps, 2:2Gbps, 4:4Gbps 8:8Gbps*/
	}

	switch(ap->instance){
		case  0 : if( hfc_chk_conf_ls(1,8, hfc0_link_speed) ) ap->linkspeed =  hfc0_link_speed; break;
		case  1 : if( hfc_chk_conf_ls(1,8, hfc1_link_speed) ) ap->linkspeed =  hfc1_link_speed; break;
		case  2 : if( hfc_chk_conf_ls(1,8, hfc2_link_speed) ) ap->linkspeed =  hfc2_link_speed; break;
		case  3 : if( hfc_chk_conf_ls(1,8, hfc3_link_speed) ) ap->linkspeed =  hfc3_link_speed; break;
		case  4 : if( hfc_chk_conf_ls(1,8, hfc4_link_speed) ) ap->linkspeed =  hfc4_link_speed; break;
		case  5 : if( hfc_chk_conf_ls(1,8, hfc5_link_speed) ) ap->linkspeed =  hfc5_link_speed; break;
		case  6 : if( hfc_chk_conf_ls(1,8, hfc6_link_speed) ) ap->linkspeed =  hfc6_link_speed; break;
		case  7 : if( hfc_chk_conf_ls(1,8, hfc7_link_speed) ) ap->linkspeed =  hfc7_link_speed; break;
		case  8 : if( hfc_chk_conf_ls(1,8, hfc8_link_speed) ) ap->linkspeed =  hfc8_link_speed; break;
		case  9 : if( hfc_chk_conf_ls(1,8, hfc9_link_speed) ) ap->linkspeed =  hfc9_link_speed; break;
		case 10 : if( hfc_chk_conf_ls(1,8,hfc10_link_speed) ) ap->linkspeed = hfc10_link_speed; break;
		case 11 : if( hfc_chk_conf_ls(1,8,hfc11_link_speed) ) ap->linkspeed = hfc11_link_speed; break;
		case 12 : if( hfc_chk_conf_ls(1,8,hfc12_link_speed) ) ap->linkspeed = hfc12_link_speed; break;
		case 13 : if( hfc_chk_conf_ls(1,8,hfc13_link_speed) ) ap->linkspeed = hfc13_link_speed; break;
		case 14 : if( hfc_chk_conf_ls(1,8,hfc14_link_speed) ) ap->linkspeed = hfc14_link_speed; break;
		case 15 : if( hfc_chk_conf_ls(1,8,hfc15_link_speed) ) ap->linkspeed = hfc15_link_speed; break;
		case 16 : if( hfc_chk_conf_ls(1,8,hfc16_link_speed) ) ap->linkspeed = hfc16_link_speed; break;
		case 17 : if( hfc_chk_conf_ls(1,8,hfc17_link_speed) ) ap->linkspeed = hfc17_link_speed; break;
		case 18 : if( hfc_chk_conf_ls(1,8,hfc18_link_speed) ) ap->linkspeed = hfc18_link_speed; break;
		case 19 : if( hfc_chk_conf_ls(1,8,hfc19_link_speed) ) ap->linkspeed = hfc19_link_speed; break;
		case 20 : if( hfc_chk_conf_ls(1,8,hfc20_link_speed) ) ap->linkspeed = hfc20_link_speed; break;
		case 21 : if( hfc_chk_conf_ls(1,8,hfc21_link_speed) ) ap->linkspeed = hfc21_link_speed; break;
		case 22 : if( hfc_chk_conf_ls(1,8,hfc22_link_speed) ) ap->linkspeed = hfc22_link_speed; break;
		case 23 : if( hfc_chk_conf_ls(1,8,hfc23_link_speed) ) ap->linkspeed = hfc23_link_speed; break;
		case 24 : if( hfc_chk_conf_ls(1,8,hfc24_link_speed) ) ap->linkspeed = hfc24_link_speed; break;
		case 25 : if( hfc_chk_conf_ls(1,8,hfc25_link_speed) ) ap->linkspeed = hfc25_link_speed; break;
		case 26 : if( hfc_chk_conf_ls(1,8,hfc26_link_speed) ) ap->linkspeed = hfc26_link_speed; break;
		case 27 : if( hfc_chk_conf_ls(1,8,hfc27_link_speed) ) ap->linkspeed = hfc27_link_speed; break;
		case 28 : if( hfc_chk_conf_ls(1,8,hfc28_link_speed) ) ap->linkspeed = hfc28_link_speed; break;
		case 29 : if( hfc_chk_conf_ls(1,8,hfc29_link_speed) ) ap->linkspeed = hfc29_link_speed; break;
		case 30 : if( hfc_chk_conf_ls(1,8,hfc30_link_speed) ) ap->linkspeed = hfc30_link_speed; break;
		case 31 : if( hfc_chk_conf_ls(1,8,hfc31_link_speed) ) ap->linkspeed = hfc31_link_speed; break;
	}

	if( hfc_chk_conf_ls(1,8,hfcmp_link_speed[ap->instance] ) ) {
		ap->linkspeed = hfcmp_link_speed[ap->instance];
		HFC_DBGPRT("link speed <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("link speed = %d\n", ap->linkspeed);
}


void hfc_set_max_transfer(struct adap_info *ap)
{


	ap->max_transfer = HFC_MAX_TRANSFER; /* AUTO : default */

	/* max transfer */
	if(ap->defparam){
		ap->dma_max = ap->max_transfer*0x100000;
		return;
	}

	if(hfc_chk_conf_mt(0,32,hfc_max_transfer)){ /* in case global parameter is set */
		ap->max_transfer = hfc_max_transfer; 
	}
	switch(ap->instance){ /* FCLNX-GPL-119 */
		case  0 : if( hfc_chk_conf_mt(0,32, hfc0_max_transfer) ) ap->max_transfer =  hfc0_max_transfer; break;
		case  1 : if( hfc_chk_conf_mt(0,32, hfc1_max_transfer) ) ap->max_transfer =  hfc1_max_transfer; break;
		case  2 : if( hfc_chk_conf_mt(0,32, hfc2_max_transfer) ) ap->max_transfer =  hfc2_max_transfer; break;
		case  3 : if( hfc_chk_conf_mt(0,32, hfc3_max_transfer) ) ap->max_transfer =  hfc3_max_transfer; break;
		case  4 : if( hfc_chk_conf_mt(0,32, hfc4_max_transfer) ) ap->max_transfer =  hfc4_max_transfer; break;
		case  5 : if( hfc_chk_conf_mt(0,32, hfc5_max_transfer) ) ap->max_transfer =  hfc5_max_transfer; break;
		case  6 : if( hfc_chk_conf_mt(0,32, hfc6_max_transfer) ) ap->max_transfer =  hfc6_max_transfer; break;
		case  7 : if( hfc_chk_conf_mt(0,32, hfc7_max_transfer) ) ap->max_transfer =  hfc7_max_transfer; break;
		case  8 : if( hfc_chk_conf_mt(0,32, hfc8_max_transfer) ) ap->max_transfer =  hfc8_max_transfer; break;
		case  9 : if( hfc_chk_conf_mt(0,32, hfc9_max_transfer) ) ap->max_transfer =  hfc9_max_transfer; break;
		case 10 : if( hfc_chk_conf_mt(0,32,hfc10_max_transfer) ) ap->max_transfer = hfc10_max_transfer; break;
		case 11 : if( hfc_chk_conf_mt(0,32,hfc11_max_transfer) ) ap->max_transfer = hfc11_max_transfer; break;
		case 12 : if( hfc_chk_conf_mt(0,32,hfc12_max_transfer) ) ap->max_transfer = hfc12_max_transfer; break;
		case 13 : if( hfc_chk_conf_mt(0,32,hfc13_max_transfer) ) ap->max_transfer = hfc13_max_transfer; break;
		case 14 : if( hfc_chk_conf_mt(0,32,hfc14_max_transfer) ) ap->max_transfer = hfc14_max_transfer; break;
		case 15 : if( hfc_chk_conf_mt(0,32,hfc15_max_transfer) ) ap->max_transfer = hfc15_max_transfer; break;
		case 16 : if( hfc_chk_conf_mt(0,32,hfc16_max_transfer) ) ap->max_transfer = hfc16_max_transfer; break;
		case 17 : if( hfc_chk_conf_mt(0,32,hfc17_max_transfer) ) ap->max_transfer = hfc17_max_transfer; break;
		case 18 : if( hfc_chk_conf_mt(0,32,hfc18_max_transfer) ) ap->max_transfer = hfc18_max_transfer; break;
		case 19 : if( hfc_chk_conf_mt(0,32,hfc19_max_transfer) ) ap->max_transfer = hfc19_max_transfer; break;
		case 20 : if( hfc_chk_conf_mt(0,32,hfc20_max_transfer) ) ap->max_transfer = hfc20_max_transfer; break;
		case 21 : if( hfc_chk_conf_mt(0,32,hfc21_max_transfer) ) ap->max_transfer = hfc21_max_transfer; break;
		case 22 : if( hfc_chk_conf_mt(0,32,hfc22_max_transfer) ) ap->max_transfer = hfc22_max_transfer; break;
		case 23 : if( hfc_chk_conf_mt(0,32,hfc23_max_transfer) ) ap->max_transfer = hfc23_max_transfer; break;
		case 24 : if( hfc_chk_conf_mt(0,32,hfc24_max_transfer) ) ap->max_transfer = hfc24_max_transfer; break;
		case 25 : if( hfc_chk_conf_mt(0,32,hfc25_max_transfer) ) ap->max_transfer = hfc25_max_transfer; break;
		case 26 : if( hfc_chk_conf_mt(0,32,hfc26_max_transfer) ) ap->max_transfer = hfc26_max_transfer; break;
		case 27 : if( hfc_chk_conf_mt(0,32,hfc27_max_transfer) ) ap->max_transfer = hfc27_max_transfer; break;
		case 28 : if( hfc_chk_conf_mt(0,32,hfc28_max_transfer) ) ap->max_transfer = hfc28_max_transfer; break;
		case 29 : if( hfc_chk_conf_mt(0,32,hfc29_max_transfer) ) ap->max_transfer = hfc29_max_transfer; break;
		case 30 : if( hfc_chk_conf_mt(0,32,hfc30_max_transfer) ) ap->max_transfer = hfc30_max_transfer; break;
		case 31 : if( hfc_chk_conf_mt(0,32,hfc31_max_transfer) ) ap->max_transfer = hfc31_max_transfer; break;
	}

	if( hfc_chk_conf_mt(0,32,hfcmp_max_transfer[ap->instance] ) ) {
		ap->max_transfer = hfcmp_max_transfer[ap->instance];
		HFC_DBGPRT("max_transfer <-- hfcldd.conf\n");
	}

	ap->dma_max = ap->max_transfer*0x100000;
	HFC_DBGPRT("max_transfer = %d, dma_max = %d\n", ap->max_transfer, ap->dma_max);

}


void hfc_set_linkup_tmo(struct adap_info *ap)
{
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		ap->linkup_tmo = HFC_LINKUP_TO;
	else
		ap->linkup_tmo = HFC_PCM_LINKUP_TO;

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		return;
#endif

	if(ap->defparam) return;

	/* link down timer  */
	if(hfc_chk_conf_val(0,60,hfc_link_down)){ /* in case global parameter is set */
		ap->linkup_tmo = hfc_link_down; 

	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down) ) ap->linkup_tmo =  hfc0_link_down; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down) ) ap->linkup_tmo =  hfc1_link_down; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_link_down) ) ap->linkup_tmo =  hfc2_link_down; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down) ) ap->linkup_tmo =  hfc3_link_down; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down) ) ap->linkup_tmo =  hfc4_link_down; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down) ) ap->linkup_tmo =  hfc5_link_down; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down) ) ap->linkup_tmo =  hfc6_link_down; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down) ) ap->linkup_tmo =  hfc7_link_down; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down) ) ap->linkup_tmo =  hfc8_link_down; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down) ) ap->linkup_tmo =  hfc9_link_down; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down) ) ap->linkup_tmo = hfc10_link_down; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down) ) ap->linkup_tmo = hfc11_link_down; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down) ) ap->linkup_tmo = hfc12_link_down; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down) ) ap->linkup_tmo = hfc13_link_down; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down) ) ap->linkup_tmo = hfc14_link_down; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down) ) ap->linkup_tmo = hfc15_link_down; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down) ) ap->linkup_tmo = hfc16_link_down; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down) ) ap->linkup_tmo = hfc17_link_down; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down) ) ap->linkup_tmo = hfc18_link_down; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down) ) ap->linkup_tmo = hfc19_link_down; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down) ) ap->linkup_tmo = hfc20_link_down; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down) ) ap->linkup_tmo = hfc21_link_down; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down) ) ap->linkup_tmo = hfc22_link_down; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down) ) ap->linkup_tmo = hfc23_link_down; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down) ) ap->linkup_tmo = hfc24_link_down; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down) ) ap->linkup_tmo = hfc25_link_down; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down) ) ap->linkup_tmo = hfc26_link_down; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down) ) ap->linkup_tmo = hfc27_link_down; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down) ) ap->linkup_tmo = hfc28_link_down; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down) ) ap->linkup_tmo = hfc29_link_down; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down) ) ap->linkup_tmo = hfc30_link_down; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down) ) ap->linkup_tmo = hfc31_link_down; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_link_down[ap->instance]) ) {
		ap->linkup_tmo = hfcmp_link_down[ap->instance];
		HFC_DBGPRT("linkup_timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("linkup_timeout = %d \n", ap->linkup_tmo);
}


void hfc_set_linkup2_tmo(struct adap_info *ap)
{

	ap->linkup2_tmo = HFC_LINKUP2_TO; /* default = 15s */
	if(ap->defparam) return;

	/* link down timer  */
	if(hfc_chk_conf_val(0,60,hfc_link_down2)){ /* in case global parameter is set */
		ap->linkup2_tmo = hfc_link_down2; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down2) ) ap->linkup2_tmo =  hfc0_link_down2; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down2) ) ap->linkup2_tmo =  hfc2_link_down2; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down2) ) ap->linkup2_tmo =  hfc3_link_down2; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down2) ) ap->linkup2_tmo =  hfc4_link_down2; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down2) ) ap->linkup2_tmo =  hfc5_link_down2; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down2) ) ap->linkup2_tmo =  hfc6_link_down2; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down2) ) ap->linkup2_tmo =  hfc7_link_down2; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down2) ) ap->linkup2_tmo =  hfc8_link_down2; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down2) ) ap->linkup2_tmo =  hfc9_link_down2; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down2) ) ap->linkup2_tmo = hfc10_link_down2; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down2) ) ap->linkup2_tmo = hfc11_link_down2; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down2) ) ap->linkup2_tmo = hfc12_link_down2; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down2) ) ap->linkup2_tmo = hfc13_link_down2; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down2) ) ap->linkup2_tmo = hfc14_link_down2; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down2) ) ap->linkup2_tmo = hfc15_link_down2; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down2) ) ap->linkup2_tmo = hfc16_link_down2; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down2) ) ap->linkup2_tmo = hfc17_link_down2; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down2) ) ap->linkup2_tmo = hfc18_link_down2; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down2) ) ap->linkup2_tmo = hfc19_link_down2; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down2) ) ap->linkup2_tmo = hfc20_link_down2; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down2) ) ap->linkup2_tmo = hfc21_link_down2; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down2) ) ap->linkup2_tmo = hfc22_link_down2; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down2) ) ap->linkup2_tmo = hfc23_link_down2; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down2) ) ap->linkup2_tmo = hfc24_link_down2; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down2) ) ap->linkup2_tmo = hfc25_link_down2; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down2) ) ap->linkup2_tmo = hfc26_link_down2; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down2) ) ap->linkup2_tmo = hfc27_link_down2; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down2) ) ap->linkup2_tmo = hfc28_link_down2; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down2) ) ap->linkup2_tmo = hfc29_link_down2; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down2) ) ap->linkup2_tmo = hfc30_link_down2; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down2) ) ap->linkup2_tmo = hfc31_link_down2; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_link_down2[ap->instance]) ) {
		ap->linkup2_tmo = hfcmp_link_down2[ap->instance];
		HFC_DBGPRT("linkup_timeout2 <-- hfcldd.conf\n");
	}

}


void hfc_set_reset_delay(struct adap_info *ap)
{

	ap->scsi_reset_delay = HFC_DELAY_TO; /* default = 7s */

	if(ap->defparam) return;

	/* link reset delay timer  */

	if(hfc_chk_conf_val(0,60,hfc_reset_delay)){ /* in case global parameter is set */
		ap->scsi_reset_delay = hfc_reset_delay; 

	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_reset_delay) ) ap->scsi_reset_delay =  hfc0_reset_delay; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_reset_delay) ) ap->scsi_reset_delay =  hfc1_reset_delay; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_reset_delay) ) ap->scsi_reset_delay =  hfc2_reset_delay; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_reset_delay) ) ap->scsi_reset_delay =  hfc3_reset_delay; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_reset_delay) ) ap->scsi_reset_delay =  hfc4_reset_delay; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_reset_delay) ) ap->scsi_reset_delay =  hfc5_reset_delay; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_reset_delay) ) ap->scsi_reset_delay =  hfc6_reset_delay; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_reset_delay) ) ap->scsi_reset_delay =  hfc7_reset_delay; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_reset_delay) ) ap->scsi_reset_delay =  hfc8_reset_delay; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_reset_delay) ) ap->scsi_reset_delay =  hfc9_reset_delay; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_reset_delay) ) ap->scsi_reset_delay = hfc10_reset_delay; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_reset_delay) ) ap->scsi_reset_delay = hfc11_reset_delay; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_reset_delay) ) ap->scsi_reset_delay = hfc12_reset_delay; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_reset_delay) ) ap->scsi_reset_delay = hfc13_reset_delay; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_reset_delay) ) ap->scsi_reset_delay = hfc14_reset_delay; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_reset_delay) ) ap->scsi_reset_delay = hfc15_reset_delay; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_reset_delay) ) ap->scsi_reset_delay = hfc16_reset_delay; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_reset_delay) ) ap->scsi_reset_delay = hfc17_reset_delay; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_reset_delay) ) ap->scsi_reset_delay = hfc18_reset_delay; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_reset_delay) ) ap->scsi_reset_delay = hfc19_reset_delay; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_reset_delay) ) ap->scsi_reset_delay = hfc20_reset_delay; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_reset_delay) ) ap->scsi_reset_delay = hfc21_reset_delay; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_reset_delay) ) ap->scsi_reset_delay = hfc22_reset_delay; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_reset_delay) ) ap->scsi_reset_delay = hfc23_reset_delay; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_reset_delay) ) ap->scsi_reset_delay = hfc24_reset_delay; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_reset_delay) ) ap->scsi_reset_delay = hfc25_reset_delay; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_reset_delay) ) ap->scsi_reset_delay = hfc26_reset_delay; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_reset_delay) ) ap->scsi_reset_delay = hfc27_reset_delay; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_reset_delay) ) ap->scsi_reset_delay = hfc28_reset_delay; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_reset_delay) ) ap->scsi_reset_delay = hfc29_reset_delay; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_reset_delay) ) ap->scsi_reset_delay = hfc30_reset_delay; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_reset_delay) ) ap->scsi_reset_delay = hfc31_reset_delay; break;
	}
	
	if( hfc_chk_conf_val(0,60,hfcmp_reset_delay[ap->instance]) ) {
		ap->scsi_reset_delay = hfcmp_reset_delay[ap->instance];
		HFC_DBGPRT("reset_delay <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("reset_delay = %d \n", ap->scsi_reset_delay);
}



void hfc_set_mck_count(struct adap_info *ap)
{
	ap->max_mck_cnt = HFC_MCKERR_CNT; /* default = 8 */
	
	if(ap->defparam) return;
	
	/* machine check recovery retry count */

	if( hfc_chk_conf_val(0,10, hfc_mck_retry)){ /* in case global parameter is set */
		ap->max_mck_cnt = hfc_mck_retry; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_mck_retry) ) ap->max_mck_cnt =  hfc0_mck_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_mck_retry) ) ap->max_mck_cnt =  hfc1_mck_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_mck_retry) ) ap->max_mck_cnt =  hfc2_mck_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_mck_retry) ) ap->max_mck_cnt =  hfc3_mck_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_mck_retry) ) ap->max_mck_cnt =  hfc4_mck_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_mck_retry) ) ap->max_mck_cnt =  hfc5_mck_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_mck_retry) ) ap->max_mck_cnt =  hfc6_mck_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_mck_retry) ) ap->max_mck_cnt =  hfc7_mck_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_mck_retry) ) ap->max_mck_cnt =  hfc8_mck_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_mck_retry) ) ap->max_mck_cnt =  hfc9_mck_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_mck_retry) ) ap->max_mck_cnt = hfc10_mck_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_mck_retry) ) ap->max_mck_cnt = hfc11_mck_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_mck_retry) ) ap->max_mck_cnt = hfc12_mck_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_mck_retry) ) ap->max_mck_cnt = hfc13_mck_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_mck_retry) ) ap->max_mck_cnt = hfc14_mck_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_mck_retry) ) ap->max_mck_cnt = hfc15_mck_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_mck_retry) ) ap->max_mck_cnt = hfc16_mck_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_mck_retry) ) ap->max_mck_cnt = hfc17_mck_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_mck_retry) ) ap->max_mck_cnt = hfc18_mck_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_mck_retry) ) ap->max_mck_cnt = hfc19_mck_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_mck_retry) ) ap->max_mck_cnt = hfc20_mck_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_mck_retry) ) ap->max_mck_cnt = hfc21_mck_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_mck_retry) ) ap->max_mck_cnt = hfc22_mck_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_mck_retry) ) ap->max_mck_cnt = hfc23_mck_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_mck_retry) ) ap->max_mck_cnt = hfc24_mck_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_mck_retry) ) ap->max_mck_cnt = hfc25_mck_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_mck_retry) ) ap->max_mck_cnt = hfc26_mck_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_mck_retry) ) ap->max_mck_cnt = hfc27_mck_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_mck_retry) ) ap->max_mck_cnt = hfc28_mck_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_mck_retry) ) ap->max_mck_cnt = hfc29_mck_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_mck_retry) ) ap->max_mck_cnt = hfc30_mck_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_mck_retry) ) ap->max_mck_cnt = hfc31_mck_retry; break;
	}

	if( hfc_chk_conf_val(0,10,hfcmp_mck_retry[ap->instance]) ) {
		ap->max_mck_cnt = hfcmp_mck_retry[ap->instance];
		HFC_DBGPRT("max machine check count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max machine check count = %d \n", ap->max_mck_cnt);
}



void hfc_set_pref_alpa(struct adap_info *ap)
{
	ap->pref_alpa = 0x01; /* default = 0x01 */
	
	if(ap->defparam) return;
		
	/* preferred ALPA */
	
	if(hfc_chk_conf_val(0x01,0xef,hfc_preferred_alpa)){ /* in case global parameter is set */
		ap->pref_alpa = hfc_preferred_alpa; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0x01,0xef, hfc0_preferred_alpa) ) ap->pref_alpa =  hfc0_preferred_alpa; break;
		case  1 : if( hfc_chk_conf_val(0x01,0xef, hfc1_preferred_alpa) ) ap->pref_alpa =  hfc1_preferred_alpa; break;
		case  2 : if( hfc_chk_conf_val(0x01,0xef, hfc2_preferred_alpa) ) ap->pref_alpa =  hfc2_preferred_alpa; break;
		case  3 : if( hfc_chk_conf_val(0x01,0xef, hfc3_preferred_alpa) ) ap->pref_alpa =  hfc3_preferred_alpa; break;
		case  4 : if( hfc_chk_conf_val(0x01,0xef, hfc4_preferred_alpa) ) ap->pref_alpa =  hfc4_preferred_alpa; break;
		case  5 : if( hfc_chk_conf_val(0x01,0xef, hfc5_preferred_alpa) ) ap->pref_alpa =  hfc5_preferred_alpa; break;
		case  6 : if( hfc_chk_conf_val(0x01,0xef, hfc6_preferred_alpa) ) ap->pref_alpa =  hfc6_preferred_alpa; break;
		case  7 : if( hfc_chk_conf_val(0x01,0xef, hfc7_preferred_alpa) ) ap->pref_alpa =  hfc7_preferred_alpa; break;
		case  8 : if( hfc_chk_conf_val(0x01,0xef, hfc8_preferred_alpa) ) ap->pref_alpa =  hfc8_preferred_alpa; break;
		case  9 : if( hfc_chk_conf_val(0x01,0xef, hfc9_preferred_alpa) ) ap->pref_alpa =  hfc9_preferred_alpa; break;
		case 10 : if( hfc_chk_conf_val(0x01,0xef,hfc10_preferred_alpa) ) ap->pref_alpa = hfc10_preferred_alpa; break;
		case 11 : if( hfc_chk_conf_val(0x01,0xef,hfc11_preferred_alpa) ) ap->pref_alpa = hfc11_preferred_alpa; break;
		case 12 : if( hfc_chk_conf_val(0x01,0xef,hfc12_preferred_alpa) ) ap->pref_alpa = hfc12_preferred_alpa; break;
		case 13 : if( hfc_chk_conf_val(0x01,0xef,hfc13_preferred_alpa) ) ap->pref_alpa = hfc13_preferred_alpa; break;
		case 14 : if( hfc_chk_conf_val(0x01,0xef,hfc14_preferred_alpa) ) ap->pref_alpa = hfc14_preferred_alpa; break;
		case 15 : if( hfc_chk_conf_val(0x01,0xef,hfc15_preferred_alpa) ) ap->pref_alpa = hfc15_preferred_alpa; break;
		case 16 : if( hfc_chk_conf_val(0x01,0xef,hfc16_preferred_alpa) ) ap->pref_alpa = hfc16_preferred_alpa; break;
		case 17 : if( hfc_chk_conf_val(0x01,0xef,hfc17_preferred_alpa) ) ap->pref_alpa = hfc17_preferred_alpa; break;
		case 18 : if( hfc_chk_conf_val(0x01,0xef,hfc18_preferred_alpa) ) ap->pref_alpa = hfc18_preferred_alpa; break;
		case 19 : if( hfc_chk_conf_val(0x01,0xef,hfc19_preferred_alpa) ) ap->pref_alpa = hfc19_preferred_alpa; break;
		case 20 : if( hfc_chk_conf_val(0x01,0xef,hfc20_preferred_alpa) ) ap->pref_alpa = hfc20_preferred_alpa; break;
		case 21 : if( hfc_chk_conf_val(0x01,0xef,hfc21_preferred_alpa) ) ap->pref_alpa = hfc21_preferred_alpa; break;
		case 22 : if( hfc_chk_conf_val(0x01,0xef,hfc22_preferred_alpa) ) ap->pref_alpa = hfc22_preferred_alpa; break;
		case 23 : if( hfc_chk_conf_val(0x01,0xef,hfc23_preferred_alpa) ) ap->pref_alpa = hfc23_preferred_alpa; break;
		case 24 : if( hfc_chk_conf_val(0x01,0xef,hfc24_preferred_alpa) ) ap->pref_alpa = hfc24_preferred_alpa; break;
		case 25 : if( hfc_chk_conf_val(0x01,0xef,hfc25_preferred_alpa) ) ap->pref_alpa = hfc25_preferred_alpa; break;
		case 26 : if( hfc_chk_conf_val(0x01,0xef,hfc26_preferred_alpa) ) ap->pref_alpa = hfc26_preferred_alpa; break;
		case 27 : if( hfc_chk_conf_val(0x01,0xef,hfc27_preferred_alpa) ) ap->pref_alpa = hfc27_preferred_alpa; break;
		case 28 : if( hfc_chk_conf_val(0x01,0xef,hfc28_preferred_alpa) ) ap->pref_alpa = hfc28_preferred_alpa; break;
		case 29 : if( hfc_chk_conf_val(0x01,0xef,hfc29_preferred_alpa) ) ap->pref_alpa = hfc29_preferred_alpa; break;
		case 30 : if( hfc_chk_conf_val(0x01,0xef,hfc30_preferred_alpa) ) ap->pref_alpa = hfc30_preferred_alpa; break;
		case 31 : if( hfc_chk_conf_val(0x01,0xef,hfc31_preferred_alpa) ) ap->pref_alpa = hfc31_preferred_alpa; break;
	}
	
	if( hfc_chk_conf_val(0x01,0xef,hfcmp_preferred_alpa[ap->instance]) ) {
		ap->pref_alpa = hfcmp_preferred_alpa[ap->instance];
		HFC_DBGPRT("preferred alpa <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("preferred alpa = %d \n", ap->pref_alpa);
}


void hfc_set_target_timeout(struct adap_info *ap)
{
	ap->target_reset_tmo = HFC_TARGET_RST_TO; /* default = 20s */

	/* target reset timeout */
	
	if(ap->defparam) return;
	
	if(hfc_chk_conf_val(0,60,hfc_reset_timeout)){ /* in case global parameter is set */
		ap->target_reset_tmo = hfc_reset_timeout;
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_reset_timeout) ) ap->target_reset_tmo =  hfc0_reset_timeout; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_reset_timeout) ) ap->target_reset_tmo =  hfc1_reset_timeout; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_reset_timeout) ) ap->target_reset_tmo =  hfc2_reset_timeout; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_reset_timeout) ) ap->target_reset_tmo =  hfc3_reset_timeout; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_reset_timeout) ) ap->target_reset_tmo =  hfc4_reset_timeout; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_reset_timeout) ) ap->target_reset_tmo =  hfc5_reset_timeout; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_reset_timeout) ) ap->target_reset_tmo =  hfc6_reset_timeout; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_reset_timeout) ) ap->target_reset_tmo =  hfc7_reset_timeout; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_reset_timeout) ) ap->target_reset_tmo =  hfc8_reset_timeout; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_reset_timeout) ) ap->target_reset_tmo =  hfc9_reset_timeout; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_reset_timeout) ) ap->target_reset_tmo = hfc10_reset_timeout; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_reset_timeout) ) ap->target_reset_tmo = hfc11_reset_timeout; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_reset_timeout) ) ap->target_reset_tmo = hfc12_reset_timeout; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_reset_timeout) ) ap->target_reset_tmo = hfc13_reset_timeout; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_reset_timeout) ) ap->target_reset_tmo = hfc14_reset_timeout; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_reset_timeout) ) ap->target_reset_tmo = hfc15_reset_timeout; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_reset_timeout) ) ap->target_reset_tmo = hfc16_reset_timeout; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_reset_timeout) ) ap->target_reset_tmo = hfc17_reset_timeout; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_reset_timeout) ) ap->target_reset_tmo = hfc18_reset_timeout; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_reset_timeout) ) ap->target_reset_tmo = hfc19_reset_timeout; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_reset_timeout) ) ap->target_reset_tmo = hfc20_reset_timeout; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_reset_timeout) ) ap->target_reset_tmo = hfc21_reset_timeout; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_reset_timeout) ) ap->target_reset_tmo = hfc22_reset_timeout; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_reset_timeout) ) ap->target_reset_tmo = hfc23_reset_timeout; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_reset_timeout) ) ap->target_reset_tmo = hfc24_reset_timeout; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_reset_timeout) ) ap->target_reset_tmo = hfc25_reset_timeout; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_reset_timeout) ) ap->target_reset_tmo = hfc26_reset_timeout; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_reset_timeout) ) ap->target_reset_tmo = hfc27_reset_timeout; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_reset_timeout) ) ap->target_reset_tmo = hfc28_reset_timeout; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_reset_timeout) ) ap->target_reset_tmo = hfc29_reset_timeout; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_reset_timeout) ) ap->target_reset_tmo = hfc30_reset_timeout; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_reset_timeout) ) ap->target_reset_tmo = hfc31_reset_timeout; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_reset_timeout[ap->instance]) ) {
		ap->target_reset_tmo = hfcmp_reset_timeout[ap->instance];
		HFC_DBGPRT("target reset timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("target reset timeout = %d \n", ap->target_reset_tmo);
}



void hfc_set_abort_timeout(struct adap_info *ap)
{
	
	ap->abort_tmo = HFC_ABORT_ACA_TO;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,60,hfc_abort_timeout)){ /* in case global parameter is set */
		ap->abort_tmo = hfc_abort_timeout; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_abort_timeout) ) ap->abort_tmo =  hfc0_abort_timeout; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_abort_timeout) ) ap->abort_tmo =  hfc1_abort_timeout; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_abort_timeout) ) ap->abort_tmo =  hfc2_abort_timeout; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_abort_timeout) ) ap->abort_tmo =  hfc3_abort_timeout; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_abort_timeout) ) ap->abort_tmo =  hfc4_abort_timeout; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_abort_timeout) ) ap->abort_tmo =  hfc5_abort_timeout; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_abort_timeout) ) ap->abort_tmo =  hfc6_abort_timeout; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_abort_timeout) ) ap->abort_tmo =  hfc7_abort_timeout; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_abort_timeout) ) ap->abort_tmo =  hfc8_abort_timeout; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_abort_timeout) ) ap->abort_tmo =  hfc9_abort_timeout; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_abort_timeout) ) ap->abort_tmo = hfc10_abort_timeout; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_abort_timeout) ) ap->abort_tmo = hfc11_abort_timeout; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_abort_timeout) ) ap->abort_tmo = hfc12_abort_timeout; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_abort_timeout) ) ap->abort_tmo = hfc13_abort_timeout; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_abort_timeout) ) ap->abort_tmo = hfc14_abort_timeout; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_abort_timeout) ) ap->abort_tmo = hfc15_abort_timeout; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_abort_timeout) ) ap->abort_tmo = hfc16_abort_timeout; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_abort_timeout) ) ap->abort_tmo = hfc17_abort_timeout; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_abort_timeout) ) ap->abort_tmo = hfc18_abort_timeout; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_abort_timeout) ) ap->abort_tmo = hfc19_abort_timeout; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_abort_timeout) ) ap->abort_tmo = hfc20_abort_timeout; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_abort_timeout) ) ap->abort_tmo = hfc21_abort_timeout; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_abort_timeout) ) ap->abort_tmo = hfc22_abort_timeout; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_abort_timeout) ) ap->abort_tmo = hfc23_abort_timeout; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_abort_timeout) ) ap->abort_tmo = hfc24_abort_timeout; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_abort_timeout) ) ap->abort_tmo = hfc25_abort_timeout; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_abort_timeout) ) ap->abort_tmo = hfc26_abort_timeout; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_abort_timeout) ) ap->abort_tmo = hfc27_abort_timeout; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_abort_timeout) ) ap->abort_tmo = hfc28_abort_timeout; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_abort_timeout) ) ap->abort_tmo = hfc29_abort_timeout; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_abort_timeout) ) ap->abort_tmo = hfc30_abort_timeout; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_abort_timeout) ) ap->abort_tmo = hfc31_abort_timeout; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_abort_timeout[ap->instance]) ) {
		ap->abort_tmo = hfcmp_abort_timeout[ap->instance];
		HFC_DBGPRT("abort timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("abort timeout = %d \n", ap->abort_tmo );

}


void hfc_set_seg_trace(struct adap_info *ap)
{
	
	ap->fw_parm = 0;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,1,hfc_seg_trace)){ /* in case global parameter is set */
		ap->fw_parm = hfc_seg_trace; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,1, hfc0_seg_trace) ) ap->fw_parm =  hfc0_seg_trace; break;
		case  1 : if( hfc_chk_conf_val(0,1, hfc1_seg_trace) ) ap->fw_parm =  hfc1_seg_trace; break;
		case  2 : if( hfc_chk_conf_val(0,1, hfc2_seg_trace) ) ap->fw_parm =  hfc2_seg_trace; break;
		case  3 : if( hfc_chk_conf_val(0,1, hfc3_seg_trace) ) ap->fw_parm =  hfc3_seg_trace; break;
		case  4 : if( hfc_chk_conf_val(0,1, hfc4_seg_trace) ) ap->fw_parm =  hfc4_seg_trace; break;
		case  5 : if( hfc_chk_conf_val(0,1, hfc5_seg_trace) ) ap->fw_parm =  hfc5_seg_trace; break;
		case  6 : if( hfc_chk_conf_val(0,1, hfc6_seg_trace) ) ap->fw_parm =  hfc6_seg_trace; break;
		case  7 : if( hfc_chk_conf_val(0,1, hfc7_seg_trace) ) ap->fw_parm =  hfc7_seg_trace; break;
		case  8 : if( hfc_chk_conf_val(0,1, hfc8_seg_trace) ) ap->fw_parm =  hfc8_seg_trace; break;
		case  9 : if( hfc_chk_conf_val(0,1, hfc9_seg_trace) ) ap->fw_parm =  hfc9_seg_trace; break;
		case 10 : if( hfc_chk_conf_val(0,1,hfc10_seg_trace) ) ap->fw_parm = hfc10_seg_trace; break;
		case 11 : if( hfc_chk_conf_val(0,1,hfc11_seg_trace) ) ap->fw_parm = hfc11_seg_trace; break;
		case 12 : if( hfc_chk_conf_val(0,1,hfc12_seg_trace) ) ap->fw_parm = hfc12_seg_trace; break;
		case 13 : if( hfc_chk_conf_val(0,1,hfc13_seg_trace) ) ap->fw_parm = hfc13_seg_trace; break;
		case 14 : if( hfc_chk_conf_val(0,1,hfc14_seg_trace) ) ap->fw_parm = hfc14_seg_trace; break;
		case 15 : if( hfc_chk_conf_val(0,1,hfc15_seg_trace) ) ap->fw_parm = hfc15_seg_trace; break;
		case 16 : if( hfc_chk_conf_val(0,1,hfc16_seg_trace) ) ap->fw_parm = hfc16_seg_trace; break;
		case 17 : if( hfc_chk_conf_val(0,1,hfc17_seg_trace) ) ap->fw_parm = hfc17_seg_trace; break;
		case 18 : if( hfc_chk_conf_val(0,1,hfc18_seg_trace) ) ap->fw_parm = hfc18_seg_trace; break;
		case 19 : if( hfc_chk_conf_val(0,1,hfc19_seg_trace) ) ap->fw_parm = hfc19_seg_trace; break;
		case 20 : if( hfc_chk_conf_val(0,1,hfc20_seg_trace) ) ap->fw_parm = hfc20_seg_trace; break;
		case 21 : if( hfc_chk_conf_val(0,1,hfc21_seg_trace) ) ap->fw_parm = hfc21_seg_trace; break;
		case 22 : if( hfc_chk_conf_val(0,1,hfc22_seg_trace) ) ap->fw_parm = hfc22_seg_trace; break;
		case 23 : if( hfc_chk_conf_val(0,1,hfc23_seg_trace) ) ap->fw_parm = hfc23_seg_trace; break;
		case 24 : if( hfc_chk_conf_val(0,1,hfc24_seg_trace) ) ap->fw_parm = hfc24_seg_trace; break;
		case 25 : if( hfc_chk_conf_val(0,1,hfc25_seg_trace) ) ap->fw_parm = hfc25_seg_trace; break;
		case 26 : if( hfc_chk_conf_val(0,1,hfc26_seg_trace) ) ap->fw_parm = hfc26_seg_trace; break;
		case 27 : if( hfc_chk_conf_val(0,1,hfc27_seg_trace) ) ap->fw_parm = hfc27_seg_trace; break;
		case 28 : if( hfc_chk_conf_val(0,1,hfc28_seg_trace) ) ap->fw_parm = hfc28_seg_trace; break;
		case 29 : if( hfc_chk_conf_val(0,1,hfc29_seg_trace) ) ap->fw_parm = hfc29_seg_trace; break;
		case 30 : if( hfc_chk_conf_val(0,1,hfc30_seg_trace) ) ap->fw_parm = hfc30_seg_trace; break;
		case 31 : if( hfc_chk_conf_val(0,1,hfc31_seg_trace) ) ap->fw_parm = hfc31_seg_trace; break;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_seg_trace[ap->instance]) ) {
		ap->fw_parm = hfcmp_seg_trace[ap->instance];
		HFC_DBGPRT("seg info trace flag <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("seg info trace flag = %d \n", ap->fw_parm );

	if(ap->fw_parm != 0){
		hfc_write_val( ap->fw_init_p->trc_info.trc_seg[0].mode, 0xf8);
	}

}


void hfc_set_queue_depth(struct adap_info *ap)
{

	ap->queue_depth = HFC_DEFAULT_QUEUE_DEPTH;

	if(ap->defparam) return;

	/* abort task set timeout */

	if(hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc_queue_depth)){ /* in case global parameter is set */
		ap->queue_depth = hfc_queue_depth; 

	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc0_queue_depth) ) ap->queue_depth =  hfc0_queue_depth; break;
		case  1 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc1_queue_depth) ) ap->queue_depth =  hfc1_queue_depth; break;
		case  2 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc2_queue_depth) ) ap->queue_depth =  hfc2_queue_depth; break;
		case  3 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc3_queue_depth) ) ap->queue_depth =  hfc3_queue_depth; break;
		case  4 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc4_queue_depth) ) ap->queue_depth =  hfc4_queue_depth; break;
		case  5 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc5_queue_depth) ) ap->queue_depth =  hfc5_queue_depth; break;
		case  6 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc6_queue_depth) ) ap->queue_depth =  hfc6_queue_depth; break;
		case  7 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc7_queue_depth) ) ap->queue_depth =  hfc7_queue_depth; break;
		case  8 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc8_queue_depth) ) ap->queue_depth =  hfc8_queue_depth; break;
		case  9 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc9_queue_depth) ) ap->queue_depth =  hfc9_queue_depth; break;
		case 10 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc10_queue_depth) ) ap->queue_depth = hfc10_queue_depth; break;
		case 11 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc11_queue_depth) ) ap->queue_depth = hfc11_queue_depth; break;
		case 12 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc12_queue_depth) ) ap->queue_depth = hfc12_queue_depth; break;
		case 13 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc13_queue_depth) ) ap->queue_depth = hfc13_queue_depth; break;
		case 14 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc14_queue_depth) ) ap->queue_depth = hfc14_queue_depth; break;
		case 15 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc15_queue_depth) ) ap->queue_depth = hfc15_queue_depth; break;
		case 16 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc16_queue_depth) ) ap->queue_depth = hfc16_queue_depth; break;
		case 17 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc17_queue_depth) ) ap->queue_depth = hfc17_queue_depth; break;
		case 18 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc18_queue_depth) ) ap->queue_depth = hfc18_queue_depth; break;
		case 19 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc19_queue_depth) ) ap->queue_depth = hfc19_queue_depth; break;
		case 20 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc20_queue_depth) ) ap->queue_depth = hfc20_queue_depth; break;
		case 21 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc21_queue_depth) ) ap->queue_depth = hfc21_queue_depth; break;
		case 22 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc22_queue_depth) ) ap->queue_depth = hfc22_queue_depth; break;
		case 23 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc23_queue_depth) ) ap->queue_depth = hfc23_queue_depth; break;
		case 24 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc24_queue_depth) ) ap->queue_depth = hfc24_queue_depth; break;
		case 25 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc25_queue_depth) ) ap->queue_depth = hfc25_queue_depth; break;
		case 26 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc26_queue_depth) ) ap->queue_depth = hfc26_queue_depth; break;
		case 27 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc27_queue_depth) ) ap->queue_depth = hfc27_queue_depth; break;
		case 28 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc28_queue_depth) ) ap->queue_depth = hfc28_queue_depth; break;
		case 29 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc29_queue_depth) ) ap->queue_depth = hfc29_queue_depth; break;
		case 30 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc30_queue_depth) ) ap->queue_depth = hfc30_queue_depth; break;
		case 31 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc31_queue_depth) ) ap->queue_depth = hfc31_queue_depth; break;
	}

	if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfcmp_queue_depth[ap->instance]) ) { /* FCLNX-GPL-555 */
		ap->queue_depth = hfcmp_queue_depth[ap->instance];
		HFC_DBGPRT("queue depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("queue depth = %d \n", ap->queue_depth );
}


void hfc_set_enable_target_reset(struct adap_info *ap)
{
	
	ap->enable_tgtrst = HFC_DEFAULT_TARGET_RESET;
	return;

#if 0
	if(ap->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_enable_tgtrst)){ /* in case global parameter is set */
		ap->enable_tgtrst = hfc_enable_tgtrst; 
	}

	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,1, hfc0_enable_tgtrst) ) ap->enable_tgtrst =  hfc0_enable_tgtrst; break;
		case  1 : if( hfc_chk_conf_val(0,1, hfc1_enable_tgtrst) ) ap->enable_tgtrst =  hfc1_enable_tgtrst; break;
		case  2 : if( hfc_chk_conf_val(0,1, hfc2_enable_tgtrst) ) ap->enable_tgtrst =  hfc2_enable_tgtrst; break;
		case  3 : if( hfc_chk_conf_val(0,1, hfc3_enable_tgtrst) ) ap->enable_tgtrst =  hfc3_enable_tgtrst; break;
		case  4 : if( hfc_chk_conf_val(0,1, hfc4_enable_tgtrst) ) ap->enable_tgtrst =  hfc4_enable_tgtrst; break;
		case  5 : if( hfc_chk_conf_val(0,1, hfc5_enable_tgtrst) ) ap->enable_tgtrst =  hfc5_enable_tgtrst; break;
		case  6 : if( hfc_chk_conf_val(0,1, hfc6_enable_tgtrst) ) ap->enable_tgtrst =  hfc6_enable_tgtrst; break;
		case  7 : if( hfc_chk_conf_val(0,1, hfc7_enable_tgtrst) ) ap->enable_tgtrst =  hfc7_enable_tgtrst; break;
		case  8 : if( hfc_chk_conf_val(0,1, hfc8_enable_tgtrst) ) ap->enable_tgtrst =  hfc8_enable_tgtrst; break;
		case  9 : if( hfc_chk_conf_val(0,1, hfc9_enable_tgtrst) ) ap->enable_tgtrst =  hfc9_enable_tgtrst; break;
		case 10 : if( hfc_chk_conf_val(0,1,hfc10_enable_tgtrst) ) ap->enable_tgtrst = hfc10_enable_tgtrst; break;
		case 11 : if( hfc_chk_conf_val(0,1,hfc11_enable_tgtrst) ) ap->enable_tgtrst = hfc11_enable_tgtrst; break;
		case 12 : if( hfc_chk_conf_val(0,1,hfc12_enable_tgtrst) ) ap->enable_tgtrst = hfc12_enable_tgtrst; break;
		case 13 : if( hfc_chk_conf_val(0,1,hfc13_enable_tgtrst) ) ap->enable_tgtrst = hfc13_enable_tgtrst; break;
		case 14 : if( hfc_chk_conf_val(0,1,hfc14_enable_tgtrst) ) ap->enable_tgtrst = hfc14_enable_tgtrst; break;
		case 15 : if( hfc_chk_conf_val(0,1,hfc15_enable_tgtrst) ) ap->enable_tgtrst = hfc15_enable_tgtrst; break;
		case 16 : if( hfc_chk_conf_val(0,1,hfc16_enable_tgtrst) ) ap->enable_tgtrst = hfc16_enable_tgtrst; break;
		case 17 : if( hfc_chk_conf_val(0,1,hfc17_enable_tgtrst) ) ap->enable_tgtrst = hfc17_enable_tgtrst; break;
		case 18 : if( hfc_chk_conf_val(0,1,hfc18_enable_tgtrst) ) ap->enable_tgtrst = hfc18_enable_tgtrst; break;
		case 19 : if( hfc_chk_conf_val(0,1,hfc19_enable_tgtrst) ) ap->enable_tgtrst = hfc19_enable_tgtrst; break;
		case 20 : if( hfc_chk_conf_val(0,1,hfc20_enable_tgtrst) ) ap->enable_tgtrst = hfc20_enable_tgtrst; break;
		case 21 : if( hfc_chk_conf_val(0,1,hfc21_enable_tgtrst) ) ap->enable_tgtrst = hfc21_enable_tgtrst; break;
		case 22 : if( hfc_chk_conf_val(0,1,hfc22_enable_tgtrst) ) ap->enable_tgtrst = hfc22_enable_tgtrst; break;
		case 23 : if( hfc_chk_conf_val(0,1,hfc23_enable_tgtrst) ) ap->enable_tgtrst = hfc23_enable_tgtrst; break;
		case 24 : if( hfc_chk_conf_val(0,1,hfc24_enable_tgtrst) ) ap->enable_tgtrst = hfc24_enable_tgtrst; break;
		case 25 : if( hfc_chk_conf_val(0,1,hfc25_enable_tgtrst) ) ap->enable_tgtrst = hfc25_enable_tgtrst; break;
		case 26 : if( hfc_chk_conf_val(0,1,hfc26_enable_tgtrst) ) ap->enable_tgtrst = hfc26_enable_tgtrst; break;
		case 27 : if( hfc_chk_conf_val(0,1,hfc27_enable_tgtrst) ) ap->enable_tgtrst = hfc27_enable_tgtrst; break;
		case 28 : if( hfc_chk_conf_val(0,1,hfc28_enable_tgtrst) ) ap->enable_tgtrst = hfc28_enable_tgtrst; break;
		case 29 : if( hfc_chk_conf_val(0,1,hfc29_enable_tgtrst) ) ap->enable_tgtrst = hfc29_enable_tgtrst; break;
		case 30 : if( hfc_chk_conf_val(0,1,hfc30_enable_tgtrst) ) ap->enable_tgtrst = hfc30_enable_tgtrst; break;
		case 31 : if( hfc_chk_conf_val(0,1,hfc31_enable_tgtrst) ) ap->enable_tgtrst = hfc31_enable_tgtrst; break;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_enable_tgtrst[ap->instance]) ) {
		ap->enable_tgtrst = hfcmp_enable_tgtrst[ap->instance];
		HFC_DBGPRT("enable target reset <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("enable target reset = %d \n", ap->enable_tgtrst );
#endif

}


void hfc_set_max_target(struct adap_info *ap)
{

	ap->max_target = MAX_TARGET_PROBE;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc_max_target)){ /* in case global parameter is set */
		ap->max_target = hfc_max_target; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc0_max_target) ) ap->max_target =  hfc0_max_target; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc1_max_target) ) ap->max_target =  hfc1_max_target; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc2_max_target) ) ap->max_target =  hfc2_max_target; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc3_max_target) ) ap->max_target =  hfc3_max_target; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc4_max_target) ) ap->max_target =  hfc4_max_target; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc5_max_target) ) ap->max_target =  hfc5_max_target; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc6_max_target) ) ap->max_target =  hfc6_max_target; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc7_max_target) ) ap->max_target =  hfc7_max_target; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc8_max_target) ) ap->max_target =  hfc8_max_target; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc9_max_target) ) ap->max_target =  hfc9_max_target; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc10_max_target) ) ap->max_target = hfc10_max_target; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc11_max_target) ) ap->max_target = hfc11_max_target; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc12_max_target) ) ap->max_target = hfc12_max_target; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc13_max_target) ) ap->max_target = hfc13_max_target; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc14_max_target) ) ap->max_target = hfc14_max_target; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc15_max_target) ) ap->max_target = hfc15_max_target; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc16_max_target) ) ap->max_target = hfc16_max_target; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc17_max_target) ) ap->max_target = hfc17_max_target; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc18_max_target) ) ap->max_target = hfc18_max_target; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc19_max_target) ) ap->max_target = hfc19_max_target; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc20_max_target) ) ap->max_target = hfc20_max_target; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc21_max_target) ) ap->max_target = hfc21_max_target; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc22_max_target) ) ap->max_target = hfc22_max_target; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc23_max_target) ) ap->max_target = hfc23_max_target; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc24_max_target) ) ap->max_target = hfc24_max_target; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc25_max_target) ) ap->max_target = hfc25_max_target; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc26_max_target) ) ap->max_target = hfc26_max_target; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc27_max_target) ) ap->max_target = hfc27_max_target; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc28_max_target) ) ap->max_target = hfc28_max_target; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc29_max_target) ) ap->max_target = hfc29_max_target; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc30_max_target) ) ap->max_target = hfc30_max_target; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc31_max_target) ) ap->max_target = hfc31_max_target; break;
	}

	if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfcmp_max_target[ap->instance]) ) {
		ap->max_target =  hfcmp_max_target[ap->instance];
		HFC_DBGPRT("max target number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max target number = %d \n", ap->max_target );

}



void hfc_set_xob_max(struct adap_info *ap)
{
	
	ap->xob_max = MAX_XOB_NUM;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,MAX_XOB_NUM,hfc_xob_max)){ /* in case global parameter is set */
		ap->xob_max = hfc_xob_max; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc0_xob_max) ) ap->xob_max =  hfc0_xob_max; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc1_xob_max) ) ap->xob_max =  hfc1_xob_max; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc2_xob_max) ) ap->xob_max =  hfc2_xob_max; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc3_xob_max) ) ap->xob_max =  hfc3_xob_max; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc4_xob_max) ) ap->xob_max =  hfc4_xob_max; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc5_xob_max) ) ap->xob_max =  hfc5_xob_max; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc6_xob_max) ) ap->xob_max =  hfc6_xob_max; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc7_xob_max) ) ap->xob_max =  hfc7_xob_max; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc8_xob_max) ) ap->xob_max =  hfc8_xob_max; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc9_xob_max) ) ap->xob_max =  hfc9_xob_max; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc10_xob_max) ) ap->xob_max = hfc10_xob_max; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc11_xob_max) ) ap->xob_max = hfc11_xob_max; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc12_xob_max) ) ap->xob_max = hfc12_xob_max; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc13_xob_max) ) ap->xob_max = hfc13_xob_max; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc14_xob_max) ) ap->xob_max = hfc14_xob_max; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc15_xob_max) ) ap->xob_max = hfc15_xob_max; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc16_xob_max) ) ap->xob_max = hfc16_xob_max; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc17_xob_max) ) ap->xob_max = hfc17_xob_max; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc18_xob_max) ) ap->xob_max = hfc18_xob_max; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc19_xob_max) ) ap->xob_max = hfc19_xob_max; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc20_xob_max) ) ap->xob_max = hfc20_xob_max; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc21_xob_max) ) ap->xob_max = hfc21_xob_max; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc22_xob_max) ) ap->xob_max = hfc22_xob_max; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc23_xob_max) ) ap->xob_max = hfc23_xob_max; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc24_xob_max) ) ap->xob_max = hfc24_xob_max; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc25_xob_max) ) ap->xob_max = hfc25_xob_max; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc26_xob_max) ) ap->xob_max = hfc26_xob_max; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc27_xob_max) ) ap->xob_max = hfc27_xob_max; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc28_xob_max) ) ap->xob_max = hfc28_xob_max; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc29_xob_max) ) ap->xob_max = hfc29_xob_max; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc30_xob_max) ) ap->xob_max = hfc30_xob_max; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc31_xob_max) ) ap->xob_max = hfc31_xob_max; break;
	}

	if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfcmp_xob_max[ap->instance]) ) {
		ap->xob_max = hfcmp_xob_max[ap->instance];
		HFC_DBGPRT("max xob number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max xob number = %d \n", ap->xob_max );

}


void hfc_set_xrb_max(struct adap_info *ap)
{

	ap->xrb_max = MAX_XRB_NUM;

	if(ap->defparam) return;

	/* abort task set timeout */

	if(hfc_chk_conf_val(0,MAX_XRB_NUM,hfc_xrb_max)){ /* in case global parameter is set */
		ap->xrb_max = hfc_xrb_max; 

	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc0_xrb_max) ) ap->xrb_max =  hfc0_xrb_max; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc1_xrb_max) ) ap->xrb_max =  hfc1_xrb_max; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc2_xrb_max) ) ap->xrb_max =  hfc2_xrb_max; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc3_xrb_max) ) ap->xrb_max =  hfc3_xrb_max; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc4_xrb_max) ) ap->xrb_max =  hfc4_xrb_max; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc5_xrb_max) ) ap->xrb_max =  hfc5_xrb_max; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc6_xrb_max) ) ap->xrb_max =  hfc6_xrb_max; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc7_xrb_max) ) ap->xrb_max =  hfc7_xrb_max; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc8_xrb_max) ) ap->xrb_max =  hfc8_xrb_max; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc9_xrb_max) ) ap->xrb_max =  hfc9_xrb_max; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc10_xrb_max) ) ap->xrb_max = hfc10_xrb_max; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc11_xrb_max) ) ap->xrb_max = hfc11_xrb_max; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc12_xrb_max) ) ap->xrb_max = hfc12_xrb_max; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc13_xrb_max) ) ap->xrb_max = hfc13_xrb_max; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc14_xrb_max) ) ap->xrb_max = hfc14_xrb_max; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc15_xrb_max) ) ap->xrb_max = hfc15_xrb_max; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc16_xrb_max) ) ap->xrb_max = hfc16_xrb_max; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc17_xrb_max) ) ap->xrb_max = hfc17_xrb_max; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc18_xrb_max) ) ap->xrb_max = hfc18_xrb_max; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc19_xrb_max) ) ap->xrb_max = hfc19_xrb_max; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc20_xrb_max) ) ap->xrb_max = hfc20_xrb_max; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc21_xrb_max) ) ap->xrb_max = hfc21_xrb_max; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc22_xrb_max) ) ap->xrb_max = hfc22_xrb_max; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc23_xrb_max) ) ap->xrb_max = hfc23_xrb_max; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc24_xrb_max) ) ap->xrb_max = hfc24_xrb_max; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc25_xrb_max) ) ap->xrb_max = hfc25_xrb_max; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc26_xrb_max) ) ap->xrb_max = hfc26_xrb_max; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc27_xrb_max) ) ap->xrb_max = hfc27_xrb_max; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc28_xrb_max) ) ap->xrb_max = hfc28_xrb_max; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc29_xrb_max) ) ap->xrb_max = hfc29_xrb_max; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc30_xrb_max) ) ap->xrb_max = hfc30_xrb_max; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc31_xrb_max) ) ap->xrb_max = hfc31_xrb_max; break;
	}

	if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfcmp_xrb_max[ap->instance]) ) {
		ap->xrb_max = hfcmp_xrb_max[ap->instance];
		HFC_DBGPRT("max xrb number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max xrb number = %d \n", ap->xrb_max );

}



void hfc_set_slog_max(struct adap_info *ap)
{
	
	ap->slog_max = HFC_SOFT_LOG_PAGE;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc_slog_max)){ /* in case global parameter is set */
		ap->slog_max = hfc_slog_max; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc0_slog_max) ) ap->slog_max =  hfc0_slog_max; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc1_slog_max) ) ap->slog_max =  hfc1_slog_max; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc2_slog_max) ) ap->slog_max =  hfc2_slog_max; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc3_slog_max) ) ap->slog_max =  hfc3_slog_max; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc4_slog_max) ) ap->slog_max =  hfc4_slog_max; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc5_slog_max) ) ap->slog_max =  hfc5_slog_max; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc6_slog_max) ) ap->slog_max =  hfc6_slog_max; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc7_slog_max) ) ap->slog_max =  hfc7_slog_max; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc8_slog_max) ) ap->slog_max =  hfc8_slog_max; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc9_slog_max) ) ap->slog_max =  hfc9_slog_max; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc10_slog_max) ) ap->slog_max = hfc10_slog_max; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc11_slog_max) ) ap->slog_max = hfc11_slog_max; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc12_slog_max) ) ap->slog_max = hfc12_slog_max; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc13_slog_max) ) ap->slog_max = hfc13_slog_max; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc14_slog_max) ) ap->slog_max = hfc14_slog_max; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc15_slog_max) ) ap->slog_max = hfc15_slog_max; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc16_slog_max) ) ap->slog_max = hfc16_slog_max; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc17_slog_max) ) ap->slog_max = hfc17_slog_max; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc18_slog_max) ) ap->slog_max = hfc18_slog_max; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc19_slog_max) ) ap->slog_max = hfc19_slog_max; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc20_slog_max) ) ap->slog_max = hfc20_slog_max; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc21_slog_max) ) ap->slog_max = hfc21_slog_max; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc22_slog_max) ) ap->slog_max = hfc22_slog_max; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc23_slog_max) ) ap->slog_max = hfc23_slog_max; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc24_slog_max) ) ap->slog_max = hfc24_slog_max; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc25_slog_max) ) ap->slog_max = hfc25_slog_max; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc26_slog_max) ) ap->slog_max = hfc26_slog_max; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc27_slog_max) ) ap->slog_max = hfc27_slog_max; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc28_slog_max) ) ap->slog_max = hfc28_slog_max; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc29_slog_max) ) ap->slog_max = hfc29_slog_max; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc30_slog_max) ) ap->slog_max = hfc30_slog_max; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc31_slog_max) ) ap->slog_max = hfc31_slog_max; break;
	}

	if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfcmp_slog_max[ap->instance]) ) {
		ap->slog_max =  hfcmp_slog_max[ap->instance];
		HFC_DBGPRT("slog max <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("slog max = %d \n", ap->slog_max );
}



void hfc_set_trc_max(struct adap_info *ap)
{
	
	ap->trc_max = HFC_MAX_TRCCNT;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc_trc_max)){ /* in case global parameter is set */
		ap->trc_max = hfc_trc_max; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc0_trc_max) ) ap->trc_max =  hfc0_trc_max; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc1_trc_max) ) ap->trc_max =  hfc1_trc_max; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc2_trc_max) ) ap->trc_max =  hfc2_trc_max; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc3_trc_max) ) ap->trc_max =  hfc3_trc_max; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc4_trc_max) ) ap->trc_max =  hfc4_trc_max; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc5_trc_max) ) ap->trc_max =  hfc5_trc_max; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc6_trc_max) ) ap->trc_max =  hfc6_trc_max; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc7_trc_max) ) ap->trc_max =  hfc7_trc_max; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc8_trc_max) ) ap->trc_max =  hfc8_trc_max; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc9_trc_max) ) ap->trc_max =  hfc9_trc_max; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc10_trc_max) ) ap->trc_max = hfc10_trc_max; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc11_trc_max) ) ap->trc_max = hfc11_trc_max; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc12_trc_max) ) ap->trc_max = hfc12_trc_max; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc13_trc_max) ) ap->trc_max = hfc13_trc_max; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc14_trc_max) ) ap->trc_max = hfc14_trc_max; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc15_trc_max) ) ap->trc_max = hfc15_trc_max; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc16_trc_max) ) ap->trc_max = hfc16_trc_max; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc17_trc_max) ) ap->trc_max = hfc17_trc_max; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc18_trc_max) ) ap->trc_max = hfc18_trc_max; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc19_trc_max) ) ap->trc_max = hfc19_trc_max; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc20_trc_max) ) ap->trc_max = hfc20_trc_max; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc21_trc_max) ) ap->trc_max = hfc21_trc_max; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc22_trc_max) ) ap->trc_max = hfc22_trc_max; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc23_trc_max) ) ap->trc_max = hfc23_trc_max; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc24_trc_max) ) ap->trc_max = hfc24_trc_max; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc25_trc_max) ) ap->trc_max = hfc25_trc_max; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc26_trc_max) ) ap->trc_max = hfc26_trc_max; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc27_trc_max) ) ap->trc_max = hfc27_trc_max; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc28_trc_max) ) ap->trc_max = hfc28_trc_max; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc29_trc_max) ) ap->trc_max = hfc29_trc_max; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc30_trc_max) ) ap->trc_max = hfc30_trc_max; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc31_trc_max) ) ap->trc_max = hfc31_trc_max; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfcmp_trc_max[ap->instance]) ) {
		ap->trc_max =  hfcmp_trc_max[ap->instance];
		HFC_DBGPRT("trace max <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("trace max = %d \n", ap->trc_max );
}



void hfc_set_pkt_num(struct adap_info *ap)
{
	
	ap->pkt_num = HFC_PKT_NUM;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(1,HFC_PKT_NUM,hfc_pkt_num)){ /* in case global parameter is set */
		ap->pkt_num = hfc_pkt_num; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc0_pkt_num) ) ap->pkt_num =  hfc0_pkt_num; break;
		case  1 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc1_pkt_num) ) ap->pkt_num =  hfc1_pkt_num; break;
		case  2 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc2_pkt_num) ) ap->pkt_num =  hfc2_pkt_num; break;
		case  3 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc3_pkt_num) ) ap->pkt_num =  hfc3_pkt_num; break;
		case  4 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc4_pkt_num) ) ap->pkt_num =  hfc4_pkt_num; break;
		case  5 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc5_pkt_num) ) ap->pkt_num =  hfc5_pkt_num; break;
		case  6 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc6_pkt_num) ) ap->pkt_num =  hfc6_pkt_num; break;
		case  7 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc7_pkt_num) ) ap->pkt_num =  hfc7_pkt_num; break;
		case  8 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc8_pkt_num) ) ap->pkt_num =  hfc8_pkt_num; break;
		case  9 : if( hfc_chk_conf_val(1,HFC_PKT_NUM, hfc9_pkt_num) ) ap->pkt_num =  hfc9_pkt_num; break;
		case 10 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc10_pkt_num) ) ap->pkt_num = hfc10_pkt_num; break;
		case 11 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc11_pkt_num) ) ap->pkt_num = hfc11_pkt_num; break;
		case 12 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc12_pkt_num) ) ap->pkt_num = hfc12_pkt_num; break;
		case 13 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc13_pkt_num) ) ap->pkt_num = hfc13_pkt_num; break;
		case 14 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc14_pkt_num) ) ap->pkt_num = hfc14_pkt_num; break;
		case 15 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc15_pkt_num) ) ap->pkt_num = hfc15_pkt_num; break;
		case 16 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc16_pkt_num) ) ap->pkt_num = hfc16_pkt_num; break;
		case 17 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc17_pkt_num) ) ap->pkt_num = hfc17_pkt_num; break;
		case 18 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc18_pkt_num) ) ap->pkt_num = hfc18_pkt_num; break;
		case 19 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc19_pkt_num) ) ap->pkt_num = hfc19_pkt_num; break;
		case 20 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc20_pkt_num) ) ap->pkt_num = hfc20_pkt_num; break;
		case 21 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc21_pkt_num) ) ap->pkt_num = hfc21_pkt_num; break;
		case 22 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc22_pkt_num) ) ap->pkt_num = hfc22_pkt_num; break;
		case 23 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc23_pkt_num) ) ap->pkt_num = hfc23_pkt_num; break;
		case 24 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc24_pkt_num) ) ap->pkt_num = hfc24_pkt_num; break;
		case 25 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc25_pkt_num) ) ap->pkt_num = hfc25_pkt_num; break;
		case 26 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc26_pkt_num) ) ap->pkt_num = hfc26_pkt_num; break;
		case 27 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc27_pkt_num) ) ap->pkt_num = hfc27_pkt_num; break;
		case 28 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc28_pkt_num) ) ap->pkt_num = hfc28_pkt_num; break;
		case 29 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc29_pkt_num) ) ap->pkt_num = hfc29_pkt_num; break;
		case 30 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc30_pkt_num) ) ap->pkt_num = hfc30_pkt_num; break;
		case 31 : if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfc31_pkt_num) ) ap->pkt_num = hfc31_pkt_num; break;
	}

	if( hfc_chk_conf_val(1,HFC_PKT_NUM,hfcmp_pkt_num[ap->instance]) ) {
		ap->pkt_num =  hfcmp_pkt_num[ap->instance];
		HFC_DBGPRT("hfc_pkt_number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("hfc_pkt_number = %d \n", ap->pkt_num );
}



void hfc_set_can_queue(struct adap_info *ap)
{
	
	ap->can_queue = HFC_DEFAULT_CAN_QUEUE;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc_can_queue)){ /* in case global parameter is set */
		ap->can_queue = hfc_can_queue; 
	}

	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc0_can_queue) ) ap->can_queue =  hfc0_can_queue; break;
		case  1 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc1_can_queue) ) ap->can_queue =  hfc1_can_queue; break;
		case  2 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc2_can_queue) ) ap->can_queue =  hfc2_can_queue; break;
		case  3 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc3_can_queue) ) ap->can_queue =  hfc3_can_queue; break;
		case  4 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc4_can_queue) ) ap->can_queue =  hfc4_can_queue; break;
		case  5 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc5_can_queue) ) ap->can_queue =  hfc5_can_queue; break;
		case  6 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc6_can_queue) ) ap->can_queue =  hfc6_can_queue; break;
		case  7 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc7_can_queue) ) ap->can_queue =  hfc7_can_queue; break;
		case  8 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc8_can_queue) ) ap->can_queue =  hfc8_can_queue; break;
		case  9 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE, hfc9_can_queue) ) ap->can_queue =  hfc9_can_queue; break;
		case 10 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc10_can_queue) ) ap->can_queue = hfc10_can_queue; break;
		case 11 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc11_can_queue) ) ap->can_queue = hfc11_can_queue; break;
		case 12 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc12_can_queue) ) ap->can_queue = hfc12_can_queue; break;
		case 13 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc13_can_queue) ) ap->can_queue = hfc13_can_queue; break;
		case 14 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc14_can_queue) ) ap->can_queue = hfc14_can_queue; break;
		case 15 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc15_can_queue) ) ap->can_queue = hfc15_can_queue; break;
		case 16 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc16_can_queue) ) ap->can_queue = hfc16_can_queue; break;
		case 17 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc17_can_queue) ) ap->can_queue = hfc17_can_queue; break;
		case 18 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc18_can_queue) ) ap->can_queue = hfc18_can_queue; break;
		case 19 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc19_can_queue) ) ap->can_queue = hfc19_can_queue; break;
		case 20 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc20_can_queue) ) ap->can_queue = hfc20_can_queue; break;
		case 21 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc21_can_queue) ) ap->can_queue = hfc21_can_queue; break;
		case 22 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc22_can_queue) ) ap->can_queue = hfc22_can_queue; break;
		case 23 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc23_can_queue) ) ap->can_queue = hfc23_can_queue; break;
		case 24 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc24_can_queue) ) ap->can_queue = hfc24_can_queue; break;
		case 25 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc25_can_queue) ) ap->can_queue = hfc25_can_queue; break;
		case 26 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc26_can_queue) ) ap->can_queue = hfc26_can_queue; break;
		case 27 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc27_can_queue) ) ap->can_queue = hfc27_can_queue; break;
		case 28 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc28_can_queue) ) ap->can_queue = hfc28_can_queue; break;
		case 29 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc29_can_queue) ) ap->can_queue = hfc29_can_queue; break;
		case 30 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc30_can_queue) ) ap->can_queue = hfc30_can_queue; break;
		case 31 : if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc31_can_queue) ) ap->can_queue = hfc31_can_queue; break;
	}

	if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfcmp_can_queue[ap->instance]) ) {
		ap->can_queue =  hfcmp_can_queue[ap->instance];
		HFC_DBGPRT("max can queue <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max can_queue = %d \n", ap->can_queue );
}



void hfc_set_sg_tblsize(struct adap_info *ap)
{
	
	ap->sg_tblsize = HFC_SG_TABLESIZE;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc_sg_tblsize)){ /* in case global parameter is set */
		ap->sg_tblsize = hfc_sg_tblsize; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc0_sg_tblsize) ) ap->sg_tblsize =  hfc0_sg_tblsize; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc1_sg_tblsize) ) ap->sg_tblsize =  hfc1_sg_tblsize; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc2_sg_tblsize) ) ap->sg_tblsize =  hfc2_sg_tblsize; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc3_sg_tblsize) ) ap->sg_tblsize =  hfc3_sg_tblsize; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc4_sg_tblsize) ) ap->sg_tblsize =  hfc4_sg_tblsize; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc5_sg_tblsize) ) ap->sg_tblsize =  hfc5_sg_tblsize; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc6_sg_tblsize) ) ap->sg_tblsize =  hfc6_sg_tblsize; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc7_sg_tblsize) ) ap->sg_tblsize =  hfc7_sg_tblsize; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc8_sg_tblsize) ) ap->sg_tblsize =  hfc8_sg_tblsize; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc9_sg_tblsize) ) ap->sg_tblsize =  hfc9_sg_tblsize; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc10_sg_tblsize) ) ap->sg_tblsize = hfc10_sg_tblsize; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc11_sg_tblsize) ) ap->sg_tblsize = hfc11_sg_tblsize; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc12_sg_tblsize) ) ap->sg_tblsize = hfc12_sg_tblsize; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc13_sg_tblsize) ) ap->sg_tblsize = hfc13_sg_tblsize; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc14_sg_tblsize) ) ap->sg_tblsize = hfc14_sg_tblsize; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc15_sg_tblsize) ) ap->sg_tblsize = hfc15_sg_tblsize; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc16_sg_tblsize) ) ap->sg_tblsize = hfc16_sg_tblsize; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc17_sg_tblsize) ) ap->sg_tblsize = hfc17_sg_tblsize; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc18_sg_tblsize) ) ap->sg_tblsize = hfc18_sg_tblsize; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc19_sg_tblsize) ) ap->sg_tblsize = hfc19_sg_tblsize; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc20_sg_tblsize) ) ap->sg_tblsize = hfc20_sg_tblsize; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc21_sg_tblsize) ) ap->sg_tblsize = hfc21_sg_tblsize; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc22_sg_tblsize) ) ap->sg_tblsize = hfc22_sg_tblsize; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc23_sg_tblsize) ) ap->sg_tblsize = hfc23_sg_tblsize; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc24_sg_tblsize) ) ap->sg_tblsize = hfc24_sg_tblsize; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc25_sg_tblsize) ) ap->sg_tblsize = hfc25_sg_tblsize; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc26_sg_tblsize) ) ap->sg_tblsize = hfc26_sg_tblsize; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc27_sg_tblsize) ) ap->sg_tblsize = hfc27_sg_tblsize; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc28_sg_tblsize) ) ap->sg_tblsize = hfc28_sg_tblsize; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc29_sg_tblsize) ) ap->sg_tblsize = hfc29_sg_tblsize; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc30_sg_tblsize) ) ap->sg_tblsize = hfc30_sg_tblsize; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc31_sg_tblsize) ) ap->sg_tblsize = hfc31_sg_tblsize; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfcmp_sg_tblsize[ap->instance]) ) {
		ap->sg_tblsize =  hfcmp_sg_tblsize[ap->instance];
		HFC_DBGPRT("sg tablesize <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("sg tablesize = %d \n", ap->sg_tblsize );
}


void hfc_set_cmnd_num(struct adap_info *ap)
{
	
	ap->cmnd_num = HFC_CMND_NUM;
	if(ap->defparam) return;


	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_CMND_NUM,hfc_cmnd_num)){ /* in case global parameter is set */
		ap->cmnd_num = hfc_cmnd_num; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc0_cmnd_num) ) ap->cmnd_num =  hfc0_cmnd_num; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc1_cmnd_num) ) ap->cmnd_num =  hfc1_cmnd_num; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc2_cmnd_num) ) ap->cmnd_num =  hfc2_cmnd_num; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc3_cmnd_num) ) ap->cmnd_num =  hfc3_cmnd_num; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc4_cmnd_num) ) ap->cmnd_num =  hfc4_cmnd_num; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc5_cmnd_num) ) ap->cmnd_num =  hfc5_cmnd_num; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc6_cmnd_num) ) ap->cmnd_num =  hfc6_cmnd_num; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc7_cmnd_num) ) ap->cmnd_num =  hfc7_cmnd_num; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc8_cmnd_num) ) ap->cmnd_num =  hfc8_cmnd_num; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc9_cmnd_num) ) ap->cmnd_num =  hfc9_cmnd_num; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc10_cmnd_num) ) ap->cmnd_num = hfc10_cmnd_num; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc11_cmnd_num) ) ap->cmnd_num = hfc11_cmnd_num; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc12_cmnd_num) ) ap->cmnd_num = hfc12_cmnd_num; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc13_cmnd_num) ) ap->cmnd_num = hfc13_cmnd_num; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc14_cmnd_num) ) ap->cmnd_num = hfc14_cmnd_num; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc15_cmnd_num) ) ap->cmnd_num = hfc15_cmnd_num; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc16_cmnd_num) ) ap->cmnd_num = hfc16_cmnd_num; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc17_cmnd_num) ) ap->cmnd_num = hfc17_cmnd_num; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc18_cmnd_num) ) ap->cmnd_num = hfc18_cmnd_num; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc19_cmnd_num) ) ap->cmnd_num = hfc19_cmnd_num; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc20_cmnd_num) ) ap->cmnd_num = hfc10_cmnd_num; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc21_cmnd_num) ) ap->cmnd_num = hfc21_cmnd_num; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc22_cmnd_num) ) ap->cmnd_num = hfc22_cmnd_num; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc23_cmnd_num) ) ap->cmnd_num = hfc23_cmnd_num; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc24_cmnd_num) ) ap->cmnd_num = hfc24_cmnd_num; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc25_cmnd_num) ) ap->cmnd_num = hfc25_cmnd_num; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc26_cmnd_num) ) ap->cmnd_num = hfc26_cmnd_num; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc27_cmnd_num) ) ap->cmnd_num = hfc27_cmnd_num; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc28_cmnd_num) ) ap->cmnd_num = hfc28_cmnd_num; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc29_cmnd_num) ) ap->cmnd_num = hfc29_cmnd_num; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc30_cmnd_num) ) ap->cmnd_num = hfc30_cmnd_num; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc31_cmnd_num) ) ap->cmnd_num = hfc31_cmnd_num; break;
	}

	if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfcmp_cmnd_num[ap->instance]) ) {
		ap->cmnd_num =  hfcmp_cmnd_num[ap->instance];
		HFC_DBGPRT("cmnd num <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("cmnd num = %d \n", ap->cmnd_num );
}


/*
 * Function:    hfc_set_minus_tout
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap         -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_set_minus_tout(struct adap_info *ap)
{
	
	ap->minus_tout = HFC_MINUS_TIMOUT;
	if(ap->defparam) return;


	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc_minus_tout)){ /* in case global parameter is set */
		ap->minus_tout = hfc_minus_tout; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc0_minus_tout) ) ap->minus_tout =  hfc0_minus_tout; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc1_minus_tout) ) ap->minus_tout =  hfc1_minus_tout; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc2_minus_tout) ) ap->minus_tout =  hfc2_minus_tout; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc3_minus_tout) ) ap->minus_tout =  hfc3_minus_tout; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc4_minus_tout) ) ap->minus_tout =  hfc4_minus_tout; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc5_minus_tout) ) ap->minus_tout =  hfc5_minus_tout; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc6_minus_tout) ) ap->minus_tout =  hfc6_minus_tout; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc7_minus_tout) ) ap->minus_tout =  hfc7_minus_tout; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc8_minus_tout) ) ap->minus_tout =  hfc8_minus_tout; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc9_minus_tout) ) ap->minus_tout =  hfc9_minus_tout; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc10_minus_tout) ) ap->minus_tout = hfc10_minus_tout; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc11_minus_tout) ) ap->minus_tout = hfc11_minus_tout; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc12_minus_tout) ) ap->minus_tout = hfc12_minus_tout; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc13_minus_tout) ) ap->minus_tout = hfc13_minus_tout; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc14_minus_tout) ) ap->minus_tout = hfc14_minus_tout; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc15_minus_tout) ) ap->minus_tout = hfc15_minus_tout; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc16_minus_tout) ) ap->minus_tout = hfc16_minus_tout; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc17_minus_tout) ) ap->minus_tout = hfc17_minus_tout; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc18_minus_tout) ) ap->minus_tout = hfc18_minus_tout; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc19_minus_tout) ) ap->minus_tout = hfc19_minus_tout; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc20_minus_tout) ) ap->minus_tout = hfc20_minus_tout; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc21_minus_tout) ) ap->minus_tout = hfc21_minus_tout; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc22_minus_tout) ) ap->minus_tout = hfc22_minus_tout; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc23_minus_tout) ) ap->minus_tout = hfc23_minus_tout; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc24_minus_tout) ) ap->minus_tout = hfc24_minus_tout; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc25_minus_tout) ) ap->minus_tout = hfc25_minus_tout; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc26_minus_tout) ) ap->minus_tout = hfc26_minus_tout; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc27_minus_tout) ) ap->minus_tout = hfc27_minus_tout; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc28_minus_tout) ) ap->minus_tout = hfc28_minus_tout; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc29_minus_tout) ) ap->minus_tout = hfc29_minus_tout; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc30_minus_tout) ) ap->minus_tout = hfc30_minus_tout; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc31_minus_tout) ) ap->minus_tout = hfc31_minus_tout; break;
	}

	if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfcmp_minus_tout[ap->instance]) ) {
		ap->minus_tout =  hfcmp_minus_tout[ap->instance];
		HFC_DBGPRT("minus timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("minus timeout = %d \n", ap->minus_tout );
	
}


void hfc_set_scsi_allowed(struct adap_info *ap)
{
	
	ap->scsi_allowed = HFC_SCSI_ALLOWED;
	if(ap->defparam) return;
	
	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,30,hfc_scsi_allowed)){ /* in case global parameter is set */
		ap->scsi_allowed = hfc_scsi_allowed; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,30, hfc0_scsi_allowed) ) ap->scsi_allowed =  hfc0_scsi_allowed; break;
		case  1 : if( hfc_chk_conf_val(0,30, hfc1_scsi_allowed) ) ap->scsi_allowed =  hfc1_scsi_allowed; break;
		case  2 : if( hfc_chk_conf_val(0,30, hfc2_scsi_allowed) ) ap->scsi_allowed =  hfc2_scsi_allowed; break;
		case  3 : if( hfc_chk_conf_val(0,30, hfc3_scsi_allowed) ) ap->scsi_allowed =  hfc3_scsi_allowed; break;
		case  4 : if( hfc_chk_conf_val(0,30, hfc4_scsi_allowed) ) ap->scsi_allowed =  hfc4_scsi_allowed; break;
		case  5 : if( hfc_chk_conf_val(0,30, hfc5_scsi_allowed) ) ap->scsi_allowed =  hfc5_scsi_allowed; break;
		case  6 : if( hfc_chk_conf_val(0,30, hfc6_scsi_allowed) ) ap->scsi_allowed =  hfc6_scsi_allowed; break;
		case  7 : if( hfc_chk_conf_val(0,30, hfc7_scsi_allowed) ) ap->scsi_allowed =  hfc7_scsi_allowed; break;
		case  8 : if( hfc_chk_conf_val(0,30, hfc8_scsi_allowed) ) ap->scsi_allowed =  hfc8_scsi_allowed; break;
		case  9 : if( hfc_chk_conf_val(0,30, hfc9_scsi_allowed) ) ap->scsi_allowed =  hfc9_scsi_allowed; break;
		case 10 : if( hfc_chk_conf_val(0,30,hfc10_scsi_allowed) ) ap->scsi_allowed = hfc10_scsi_allowed; break;
		case 11 : if( hfc_chk_conf_val(0,30,hfc11_scsi_allowed) ) ap->scsi_allowed = hfc11_scsi_allowed; break;
		case 12 : if( hfc_chk_conf_val(0,30,hfc12_scsi_allowed) ) ap->scsi_allowed = hfc12_scsi_allowed; break;
		case 13 : if( hfc_chk_conf_val(0,30,hfc13_scsi_allowed) ) ap->scsi_allowed = hfc13_scsi_allowed; break;
		case 14 : if( hfc_chk_conf_val(0,30,hfc14_scsi_allowed) ) ap->scsi_allowed = hfc14_scsi_allowed; break;
		case 15 : if( hfc_chk_conf_val(0,30,hfc15_scsi_allowed) ) ap->scsi_allowed = hfc15_scsi_allowed; break;
		case 16 : if( hfc_chk_conf_val(0,30,hfc16_scsi_allowed) ) ap->scsi_allowed = hfc16_scsi_allowed; break;
		case 17 : if( hfc_chk_conf_val(0,30,hfc17_scsi_allowed) ) ap->scsi_allowed = hfc17_scsi_allowed; break;
		case 18 : if( hfc_chk_conf_val(0,30,hfc18_scsi_allowed) ) ap->scsi_allowed = hfc18_scsi_allowed; break;
		case 19 : if( hfc_chk_conf_val(0,30,hfc19_scsi_allowed) ) ap->scsi_allowed = hfc19_scsi_allowed; break;
		case 20 : if( hfc_chk_conf_val(0,30,hfc20_scsi_allowed) ) ap->scsi_allowed = hfc20_scsi_allowed; break;
		case 21 : if( hfc_chk_conf_val(0,30,hfc21_scsi_allowed) ) ap->scsi_allowed = hfc21_scsi_allowed; break;
		case 22 : if( hfc_chk_conf_val(0,30,hfc22_scsi_allowed) ) ap->scsi_allowed = hfc22_scsi_allowed; break;
		case 23 : if( hfc_chk_conf_val(0,30,hfc23_scsi_allowed) ) ap->scsi_allowed = hfc23_scsi_allowed; break;
		case 24 : if( hfc_chk_conf_val(0,30,hfc24_scsi_allowed) ) ap->scsi_allowed = hfc24_scsi_allowed; break;
		case 25 : if( hfc_chk_conf_val(0,30,hfc25_scsi_allowed) ) ap->scsi_allowed = hfc25_scsi_allowed; break;
		case 26 : if( hfc_chk_conf_val(0,30,hfc26_scsi_allowed) ) ap->scsi_allowed = hfc26_scsi_allowed; break;
		case 27 : if( hfc_chk_conf_val(0,30,hfc27_scsi_allowed) ) ap->scsi_allowed = hfc27_scsi_allowed; break;
		case 28 : if( hfc_chk_conf_val(0,30,hfc28_scsi_allowed) ) ap->scsi_allowed = hfc28_scsi_allowed; break;
		case 29 : if( hfc_chk_conf_val(0,30,hfc29_scsi_allowed) ) ap->scsi_allowed = hfc29_scsi_allowed; break;
		case 30 : if( hfc_chk_conf_val(0,30,hfc30_scsi_allowed) ) ap->scsi_allowed = hfc30_scsi_allowed; break;
		case 31 : if( hfc_chk_conf_val(0,30,hfc31_scsi_allowed) ) ap->scsi_allowed = hfc31_scsi_allowed; break;
	}

	if( hfc_chk_conf_val(0,30,hfcmp_scsi_allowed[ap->instance]) ) {
		ap->scsi_allowed =  hfcmp_scsi_allowed[ap->instance];
		HFC_DBGPRT("scsi_allowed <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("scsi_allowed = %d \n", ap->scsi_allowed );
}

/* FCLNX-GPL-0343 */
void hfc_set_login_retry(struct adap_info *ap)
{
	ap->login_retry = HFC_LOGIN_RETRY; /* default = 3 */
	
	if(ap->defparam) return;
	
	/* machine check recovery retry count */

	if( hfc_chk_conf_val(0,10, hfc_login_retry)){ /* in case global parameter is set */
		ap->login_retry = hfc_login_retry; 
		
	}

#if 0
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_login_retry) ) ap->login_retry =  hfc0_login_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_login_retry) ) ap->login_retry =  hfc1_login_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_login_retry) ) ap->login_retry =  hfc2_login_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_login_retry) ) ap->login_retry =  hfc3_login_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_login_retry) ) ap->login_retry =  hfc4_login_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_login_retry) ) ap->login_retry =  hfc5_login_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_login_retry) ) ap->login_retry =  hfc6_login_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_login_retry) ) ap->login_retry =  hfc7_login_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_login_retry) ) ap->login_retry =  hfc8_login_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_login_retry) ) ap->login_retry =  hfc9_login_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_login_retry) ) ap->login_retry = hfc10_login_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_login_retry) ) ap->login_retry = hfc11_login_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_login_retry) ) ap->login_retry = hfc12_login_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_login_retry) ) ap->login_retry = hfc13_login_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_login_retry) ) ap->login_retry = hfc14_login_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_login_retry) ) ap->login_retry = hfc15_login_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_login_retry) ) ap->login_retry = hfc16_login_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_login_retry) ) ap->login_retry = hfc17_login_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_login_retry) ) ap->login_retry = hfc18_login_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_login_retry) ) ap->login_retry = hfc19_login_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_login_retry) ) ap->login_retry = hfc20_login_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_login_retry) ) ap->login_retry = hfc21_login_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_login_retry) ) ap->login_retry = hfc22_login_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_login_retry) ) ap->login_retry = hfc23_login_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_login_retry) ) ap->login_retry = hfc24_login_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_login_retry) ) ap->login_retry = hfc25_login_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_login_retry) ) ap->login_retry = hfc26_login_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_login_retry) ) ap->login_retry = hfc27_login_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_login_retry) ) ap->login_retry = hfc28_login_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_login_retry) ) ap->login_retry = hfc29_login_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_login_retry) ) ap->login_retry = hfc30_login_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_login_retry) ) ap->login_retry = hfc31_login_retry; break;
	}
#endif

	if( hfc_chk_conf_val(0,10,hfcmp_login_retry[ap->instance]) ) {
		ap->login_retry = hfcmp_login_retry[ap->instance];
		HFC_DBGPRT("max login retry count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max login retry count = %d \n", ap->login_retry);
}


/* FCLNX-GPL-0343 */
void hfc_set_els_retry(struct adap_info *ap)
{
	ap->els_retry = HFC_ELS_RETRY; /* default = 3 */
	
	if(ap->defparam) return;
	
	/* machine check recovery retry count */

	if( hfc_chk_conf_val(0,10, hfc_els_retry)){ /* in case global parameter is set */
		ap->els_retry = hfc_els_retry; 
		
	}

#if 0
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_els_retry) ) ap->els_retry =  hfc0_els_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_els_retry) ) ap->els_retry =  hfc1_els_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_els_retry) ) ap->els_retry =  hfc2_els_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_els_retry) ) ap->els_retry =  hfc3_els_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_els_retry) ) ap->els_retry =  hfc4_els_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_els_retry) ) ap->els_retry =  hfc5_els_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_els_retry) ) ap->els_retry =  hfc6_els_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_els_retry) ) ap->els_retry =  hfc7_els_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_els_retry) ) ap->els_retry =  hfc8_els_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_els_retry) ) ap->els_retry =  hfc9_els_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_els_retry) ) ap->els_retry = hfc10_els_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_els_retry) ) ap->els_retry = hfc11_els_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_els_retry) ) ap->els_retry = hfc12_els_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_els_retry) ) ap->els_retry = hfc13_els_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_els_retry) ) ap->els_retry = hfc14_els_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_els_retry) ) ap->els_retry = hfc15_els_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_els_retry) ) ap->els_retry = hfc16_els_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_els_retry) ) ap->els_retry = hfc17_els_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_els_retry) ) ap->els_retry = hfc18_els_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_els_retry) ) ap->els_retry = hfc19_els_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_els_retry) ) ap->els_retry = hfc20_els_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_els_retry) ) ap->els_retry = hfc21_els_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_els_retry) ) ap->els_retry = hfc22_els_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_els_retry) ) ap->els_retry = hfc23_els_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_els_retry) ) ap->els_retry = hfc24_els_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_els_retry) ) ap->els_retry = hfc25_els_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_els_retry) ) ap->els_retry = hfc26_els_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_els_retry) ) ap->els_retry = hfc27_els_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_els_retry) ) ap->els_retry = hfc28_els_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_els_retry) ) ap->els_retry = hfc29_els_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_els_retry) ) ap->els_retry = hfc30_els_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_els_retry) ) ap->els_retry = hfc31_els_retry; break;
	}
#endif

	if( hfc_chk_conf_val(0,10,hfcmp_els_retry[ap->instance]) ) {
		ap->els_retry = hfcmp_els_retry[ap->instance];
		HFC_DBGPRT("max els retry count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max els retry count = %d \n", ap->els_retry);
}

/* FCLNX-GPL-0343 */
void hfc_set_ioctl_scsi_timeout(struct adap_info *ap)
{
	ap->ioctl_scsi_timeout = HFC_IOCTL_SCSI_TIMEOUT; /* default = 3 */
	
	if(ap->defparam) return;
	
	/* machine check recovery retry count */

	if( hfc_chk_conf_val(1,120, hfc_ioctl_scsi_timeout)){ /* in case global parameter is set */
		ap->ioctl_scsi_timeout = hfc_ioctl_scsi_timeout; 
		
	}

#if 0
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(1,120, hfc0_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc0_ioctl_scsi_timeout; break;
		case  1 : if( hfc_chk_conf_val(1,120, hfc1_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc1_ioctl_scsi_timeout; break;
		case  2 : if( hfc_chk_conf_val(1,120, hfc2_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc2_ioctl_scsi_timeout; break;
		case  3 : if( hfc_chk_conf_val(1,120, hfc3_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc3_ioctl_scsi_timeout; break;
		case  4 : if( hfc_chk_conf_val(1,120, hfc4_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc4_ioctl_scsi_timeout; break;
		case  5 : if( hfc_chk_conf_val(1,120, hfc5_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc5_ioctl_scsi_timeout; break;
		case  6 : if( hfc_chk_conf_val(1,120, hfc6_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc6_ioctl_scsi_timeout; break;
		case  7 : if( hfc_chk_conf_val(1,120, hfc7_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc7_ioctl_scsi_timeout; break;
		case  8 : if( hfc_chk_conf_val(1,120, hfc8_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc8_ioctl_scsi_timeout; break;
		case  9 : if( hfc_chk_conf_val(1,120, hfc9_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout =  hfc9_ioctl_scsi_timeout; break;
		case 10 : if( hfc_chk_conf_val(1,120,hfc10_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc10_ioctl_scsi_timeout; break;
		case 11 : if( hfc_chk_conf_val(1,120,hfc11_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc11_ioctl_scsi_timeout; break;
		case 12 : if( hfc_chk_conf_val(1,120,hfc12_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc12_ioctl_scsi_timeout; break;
		case 13 : if( hfc_chk_conf_val(1,120,hfc13_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc13_ioctl_scsi_timeout; break;
		case 14 : if( hfc_chk_conf_val(1,120,hfc14_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc14_ioctl_scsi_timeout; break;
		case 15 : if( hfc_chk_conf_val(1,120,hfc15_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc15_ioctl_scsi_timeout; break;
		case 16 : if( hfc_chk_conf_val(1,120,hfc16_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc16_ioctl_scsi_timeout; break;
		case 17 : if( hfc_chk_conf_val(1,120,hfc17_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc17_ioctl_scsi_timeout; break;
		case 18 : if( hfc_chk_conf_val(1,120,hfc18_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc18_ioctl_scsi_timeout; break;
		case 19 : if( hfc_chk_conf_val(1,120,hfc19_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc19_ioctl_scsi_timeout; break;
		case 20 : if( hfc_chk_conf_val(1,120,hfc20_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc20_ioctl_scsi_timeout; break;
		case 21 : if( hfc_chk_conf_val(1,120,hfc21_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc21_ioctl_scsi_timeout; break;
		case 22 : if( hfc_chk_conf_val(1,120,hfc22_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc22_ioctl_scsi_timeout; break;
		case 23 : if( hfc_chk_conf_val(1,120,hfc23_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc23_ioctl_scsi_timeout; break;
		case 24 : if( hfc_chk_conf_val(1,120,hfc24_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc24_ioctl_scsi_timeout; break;
		case 25 : if( hfc_chk_conf_val(1,120,hfc25_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc25_ioctl_scsi_timeout; break;
		case 26 : if( hfc_chk_conf_val(1,120,hfc26_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc26_ioctl_scsi_timeout; break;
		case 27 : if( hfc_chk_conf_val(1,120,hfc27_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc27_ioctl_scsi_timeout; break;
		case 28 : if( hfc_chk_conf_val(1,120,hfc28_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc28_ioctl_scsi_timeout; break;
		case 29 : if( hfc_chk_conf_val(1,120,hfc29_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc29_ioctl_scsi_timeout; break;
		case 30 : if( hfc_chk_conf_val(1,120,hfc30_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc30_ioctl_scsi_timeout; break;
		case 31 : if( hfc_chk_conf_val(1,120,hfc31_ioctl_scsi_timeout) ) ap->ioctl_scsi_timeout = hfc31_ioctl_scsi_timeout; break;
	}
#endif

	if( hfc_chk_conf_val(1,120,hfcmp_ioctl_scsi_timeout[ap->instance]) ) {
		ap->ioctl_scsi_timeout = hfcmp_ioctl_scsi_timeout[ap->instance];
		HFC_DBGPRT("max ioctl scsi command timeout period <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max ioctl scsi command timeout period = %d \n", ap->ioctl_scsi_timeout);
}

void hfc_set_cmd_per_lun(struct adap_info *ap)															/* FCLNX-283 STR*/
{
	
	ap->cmd_per_lun = HFC_CMD_PER_LUN;

	if(ap->defparam) return;
	
	if(hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc_cmd_per_lun)){ /* in case global parameter is set */
		ap->cmd_per_lun = hfc_cmd_per_lun; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc0_cmd_per_lun) ) ap->cmd_per_lun =  hfc0_cmd_per_lun; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc1_cmd_per_lun) ) ap->cmd_per_lun =  hfc1_cmd_per_lun; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc2_cmd_per_lun) ) ap->cmd_per_lun =  hfc2_cmd_per_lun; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc3_cmd_per_lun) ) ap->cmd_per_lun =  hfc3_cmd_per_lun; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc4_cmd_per_lun) ) ap->cmd_per_lun =  hfc4_cmd_per_lun; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc5_cmd_per_lun) ) ap->cmd_per_lun =  hfc5_cmd_per_lun; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc6_cmd_per_lun) ) ap->cmd_per_lun =  hfc6_cmd_per_lun; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc7_cmd_per_lun) ) ap->cmd_per_lun =  hfc7_cmd_per_lun; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc8_cmd_per_lun) ) ap->cmd_per_lun =  hfc8_cmd_per_lun; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc9_cmd_per_lun) ) ap->cmd_per_lun =  hfc9_cmd_per_lun; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc10_cmd_per_lun) ) ap->cmd_per_lun = hfc10_cmd_per_lun; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc11_cmd_per_lun) ) ap->cmd_per_lun = hfc11_cmd_per_lun; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc12_cmd_per_lun) ) ap->cmd_per_lun = hfc12_cmd_per_lun; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc13_cmd_per_lun) ) ap->cmd_per_lun = hfc13_cmd_per_lun; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc14_cmd_per_lun) ) ap->cmd_per_lun = hfc14_cmd_per_lun; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc15_cmd_per_lun) ) ap->cmd_per_lun = hfc15_cmd_per_lun; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc16_cmd_per_lun) ) ap->cmd_per_lun = hfc16_cmd_per_lun; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc17_cmd_per_lun) ) ap->cmd_per_lun = hfc17_cmd_per_lun; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc18_cmd_per_lun) ) ap->cmd_per_lun = hfc18_cmd_per_lun; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc19_cmd_per_lun) ) ap->cmd_per_lun = hfc19_cmd_per_lun; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc20_cmd_per_lun) ) ap->cmd_per_lun = hfc20_cmd_per_lun; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc21_cmd_per_lun) ) ap->cmd_per_lun = hfc21_cmd_per_lun; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc22_cmd_per_lun) ) ap->cmd_per_lun = hfc22_cmd_per_lun; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc23_cmd_per_lun) ) ap->cmd_per_lun = hfc23_cmd_per_lun; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc24_cmd_per_lun) ) ap->cmd_per_lun = hfc24_cmd_per_lun; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc25_cmd_per_lun) ) ap->cmd_per_lun = hfc25_cmd_per_lun; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc26_cmd_per_lun) ) ap->cmd_per_lun = hfc26_cmd_per_lun; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc27_cmd_per_lun) ) ap->cmd_per_lun = hfc27_cmd_per_lun; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc28_cmd_per_lun) ) ap->cmd_per_lun = hfc28_cmd_per_lun; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc29_cmd_per_lun) ) ap->cmd_per_lun = hfc29_cmd_per_lun; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc30_cmd_per_lun) ) ap->cmd_per_lun = hfc30_cmd_per_lun; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc31_cmd_per_lun) ) ap->cmd_per_lun = hfc31_cmd_per_lun; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfcmp_cmd_per_lun[ap->instance]) ) {
		ap->cmd_per_lun =  hfcmp_cmd_per_lun[ap->instance];
		HFC_DBGPRT("cmd_per_lun <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("cmd_per_lun = %d \n", ap->cmd_per_lun );
}																										/* FCLNX-283 END */



void hfc_set_max_sectors(struct adap_info *ap)															/* FCLNX-283 STR */
{
	
	ap->max_sectors = HFC_MAX_SECTORS;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc_max_sectors)){ /* in case global parameter is set */
		ap->max_sectors = hfc_max_sectors; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc0_max_sectors) ) ap->max_sectors =  hfc0_max_sectors; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc1_max_sectors) ) ap->max_sectors =  hfc1_max_sectors; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc2_max_sectors) ) ap->max_sectors =  hfc2_max_sectors; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc3_max_sectors) ) ap->max_sectors =  hfc3_max_sectors; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc4_max_sectors) ) ap->max_sectors =  hfc4_max_sectors; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc5_max_sectors) ) ap->max_sectors =  hfc5_max_sectors; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc6_max_sectors) ) ap->max_sectors =  hfc6_max_sectors; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc7_max_sectors) ) ap->max_sectors =  hfc7_max_sectors; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc8_max_sectors) ) ap->max_sectors =  hfc8_max_sectors; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc9_max_sectors) ) ap->max_sectors =  hfc9_max_sectors; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc10_max_sectors) ) ap->max_sectors = hfc10_max_sectors; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc11_max_sectors) ) ap->max_sectors = hfc11_max_sectors; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc12_max_sectors) ) ap->max_sectors = hfc12_max_sectors; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc13_max_sectors) ) ap->max_sectors = hfc13_max_sectors; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc14_max_sectors) ) ap->max_sectors = hfc14_max_sectors; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc15_max_sectors) ) ap->max_sectors = hfc15_max_sectors; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc16_max_sectors) ) ap->max_sectors = hfc16_max_sectors; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc17_max_sectors) ) ap->max_sectors = hfc17_max_sectors; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc18_max_sectors) ) ap->max_sectors = hfc18_max_sectors; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc19_max_sectors) ) ap->max_sectors = hfc19_max_sectors; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc20_max_sectors) ) ap->max_sectors = hfc20_max_sectors; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc21_max_sectors) ) ap->max_sectors = hfc21_max_sectors; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc22_max_sectors) ) ap->max_sectors = hfc22_max_sectors; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc23_max_sectors) ) ap->max_sectors = hfc23_max_sectors; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc24_max_sectors) ) ap->max_sectors = hfc24_max_sectors; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc25_max_sectors) ) ap->max_sectors = hfc25_max_sectors; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc26_max_sectors) ) ap->max_sectors = hfc26_max_sectors; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc27_max_sectors) ) ap->max_sectors = hfc27_max_sectors; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc28_max_sectors) ) ap->max_sectors = hfc28_max_sectors; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc29_max_sectors) ) ap->max_sectors = hfc29_max_sectors; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc30_max_sectors) ) ap->max_sectors = hfc30_max_sectors; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc31_max_sectors) ) ap->max_sectors = hfc31_max_sectors; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfcmp_max_sectors[ap->instance]) ) {
		ap->max_sectors =  hfcmp_max_sectors[ap->instance];
		HFC_DBGPRT("max_sectors <-- hfcldd.conf\n");
	}
	
	if( ap->max_sectors > 0 ){
		 ap->hosts->max_sectors = ap->max_sectors;
	}

	HFC_DBGPRT("max_sectors = %d \n", ap->max_sectors );
}																										/* FCLNX-283 END */


void hfc_set_vary_io(struct adap_info *ap)																/* FCLNX-283 STR */
{
	
	ap->vary_io = 0;

	if(ap->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,HFC_VARY_IO,hfc_vary_io)){ /* in case global parameter is set */
		ap->vary_io = hfc_vary_io; 
		
	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc0_vary_io) ) ap->vary_io =  hfc0_vary_io; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc1_vary_io) ) ap->vary_io =  hfc1_vary_io; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc2_vary_io) ) ap->vary_io =  hfc2_vary_io; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc3_vary_io) ) ap->vary_io =  hfc3_vary_io; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc4_vary_io) ) ap->vary_io =  hfc4_vary_io; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc5_vary_io) ) ap->vary_io =  hfc5_vary_io; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc6_vary_io) ) ap->vary_io =  hfc6_vary_io; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc7_vary_io) ) ap->vary_io =  hfc7_vary_io; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc8_vary_io) ) ap->vary_io =  hfc8_vary_io; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc9_vary_io) ) ap->vary_io =  hfc9_vary_io; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc10_vary_io) ) ap->vary_io = hfc10_vary_io; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc11_vary_io) ) ap->vary_io = hfc11_vary_io; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc12_vary_io) ) ap->vary_io = hfc12_vary_io; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc13_vary_io) ) ap->vary_io = hfc13_vary_io; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc14_vary_io) ) ap->vary_io = hfc14_vary_io; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc15_vary_io) ) ap->vary_io = hfc15_vary_io; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc16_vary_io) ) ap->vary_io = hfc16_vary_io; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc17_vary_io) ) ap->vary_io = hfc17_vary_io; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc18_vary_io) ) ap->vary_io = hfc18_vary_io; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc19_vary_io) ) ap->vary_io = hfc19_vary_io; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc20_vary_io) ) ap->vary_io = hfc20_vary_io; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc21_vary_io) ) ap->vary_io = hfc21_vary_io; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc22_vary_io) ) ap->vary_io = hfc22_vary_io; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc23_vary_io) ) ap->vary_io = hfc23_vary_io; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc24_vary_io) ) ap->vary_io = hfc24_vary_io; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc25_vary_io) ) ap->vary_io = hfc25_vary_io; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc26_vary_io) ) ap->vary_io = hfc26_vary_io; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc27_vary_io) ) ap->vary_io = hfc27_vary_io; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc28_vary_io) ) ap->vary_io = hfc28_vary_io; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc29_vary_io) ) ap->vary_io = hfc29_vary_io; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc30_vary_io) ) ap->vary_io = hfc30_vary_io; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc31_vary_io) ) ap->vary_io = hfc31_vary_io; break;
	}

	if( hfc_chk_conf_val(0,HFC_VARY_IO,hfcmp_vary_io[ap->instance]) ) {
		ap->vary_io =  hfcmp_vary_io[ap->instance];
		HFC_DBGPRT("vary_io <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("vary_io = %d \n", ap->vary_io );
}																										/* FCLNX-283 END */

void hfc_set_lun_reset_delay(struct adap_info *ap)/* FCLNX-0506 */ 	/* FCLNX-GPL-038 */
{

	ap->lun_reset_delay = HFC_LUN_DELAY; /* default = 0s */

	if(ap->defparam) return;

	/* link reset delay timer  */

	if(hfc_chk_conf_val(0,60,hfc_lun_reset_delay)){ /* in case global parameter is set */
		ap->lun_reset_delay = hfc_lun_reset_delay; 

	}
	switch(ap->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_lun_reset_delay) ) ap->lun_reset_delay =  hfc0_lun_reset_delay; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_lun_reset_delay) ) ap->lun_reset_delay =  hfc1_lun_reset_delay; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_lun_reset_delay) ) ap->lun_reset_delay =  hfc2_lun_reset_delay; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_lun_reset_delay) ) ap->lun_reset_delay =  hfc3_lun_reset_delay; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_lun_reset_delay) ) ap->lun_reset_delay =  hfc4_lun_reset_delay; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_lun_reset_delay) ) ap->lun_reset_delay =  hfc5_lun_reset_delay; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_lun_reset_delay) ) ap->lun_reset_delay =  hfc6_lun_reset_delay; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_lun_reset_delay) ) ap->lun_reset_delay =  hfc7_lun_reset_delay; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_lun_reset_delay) ) ap->lun_reset_delay =  hfc8_lun_reset_delay; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_lun_reset_delay) ) ap->lun_reset_delay =  hfc9_lun_reset_delay; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_lun_reset_delay) ) ap->lun_reset_delay = hfc10_lun_reset_delay; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_lun_reset_delay) ) ap->lun_reset_delay = hfc11_lun_reset_delay; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_lun_reset_delay) ) ap->lun_reset_delay = hfc12_lun_reset_delay; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_lun_reset_delay) ) ap->lun_reset_delay = hfc13_lun_reset_delay; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_lun_reset_delay) ) ap->lun_reset_delay = hfc14_lun_reset_delay; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_lun_reset_delay) ) ap->lun_reset_delay = hfc15_lun_reset_delay; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_lun_reset_delay) ) ap->lun_reset_delay = hfc16_lun_reset_delay; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_lun_reset_delay) ) ap->lun_reset_delay = hfc17_lun_reset_delay; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_lun_reset_delay) ) ap->lun_reset_delay = hfc18_lun_reset_delay; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_lun_reset_delay) ) ap->lun_reset_delay = hfc19_lun_reset_delay; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_lun_reset_delay) ) ap->lun_reset_delay = hfc20_lun_reset_delay; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_lun_reset_delay) ) ap->lun_reset_delay = hfc21_lun_reset_delay; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_lun_reset_delay) ) ap->lun_reset_delay = hfc22_lun_reset_delay; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_lun_reset_delay) ) ap->lun_reset_delay = hfc23_lun_reset_delay; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_lun_reset_delay) ) ap->lun_reset_delay = hfc24_lun_reset_delay; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_lun_reset_delay) ) ap->lun_reset_delay = hfc25_lun_reset_delay; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_lun_reset_delay) ) ap->lun_reset_delay = hfc26_lun_reset_delay; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_lun_reset_delay) ) ap->lun_reset_delay = hfc27_lun_reset_delay; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_lun_reset_delay) ) ap->lun_reset_delay = hfc28_lun_reset_delay; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_lun_reset_delay) ) ap->lun_reset_delay = hfc29_lun_reset_delay; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_lun_reset_delay) ) ap->lun_reset_delay = hfc30_lun_reset_delay; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_lun_reset_delay) ) ap->lun_reset_delay = hfc31_lun_reset_delay; break;
	}
	
	if( hfc_chk_conf_val(0,60,hfcmp_lun_rst_delay[ap->instance]) ) {
		ap->lun_reset_delay = hfcmp_lun_rst_delay[ap->instance];
		HFC_DBGPRT("lun_reset_delay <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("lun_reset_delay = %d \n", ap->lun_reset_delay);
}																	/* FCLNX-GPL-038 */

void hfc_set_abort_t_restrain(struct adap_info *ap) /* FCLNX-0506 */
{
	ap->abort_t_restrain = 0;

	if(ap->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_abort_t_restrain)){ /* in case global parameter is set*/
			ap->abort_t_restrain = hfc_abort_t_restrain;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_abort_t_restrain[ap->instance]) ) {
		ap->abort_t_restrain =	hfcmp_abort_t_restrain[ap->instance];
		HFC_DBGPRT("abort_t_restrain <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("abort_t_restrain = %d \n", ap->abort_t_restrain );
} /* FCLNX-0506 */

void hfc_set_login_restrain(struct adap_info *ap) /* FCLNX-0506 */
{
	ap->login_restrain = 0;

	if(ap->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_login_restrain)){ /* in case global parameter is set*/
			ap->login_restrain = hfc_login_restrain;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_login_restrain[ap->instance]) ) {
		ap->login_restrain =	hfcmp_login_restrain[ap->instance];
		HFC_DBGPRT("login_restrain <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("login_restrain = %d \n", ap->login_restrain );
} /* FCLNX-0506 */ 

void hfc_set_mck_point(struct adap_info *ap){   /* FCLNX-0533 */

        int mck_point_valid = 0;
        int value;

        ap->mck_point = HFC_NO_MCK_POINT;

        if (hfc_param_search("hfc_mck_point", &value)) hfc_mck_point = value;

        mck_point_valid =       hfc_chk_conf_val(HFC_NO_MCK_POINT,HFC_MAX_MCK_POINT, hfc_mck_point);

        if(ap->defparam) return;

        if (mck_point_valid == 1){
                ap->mck_point = hfc_mck_point;
        }

        HFC_DBGPRT( "hfc_mck_point=%d\n",ap->mck_point);
        return;
}       /* FCLNX-0533 */

/* Set INT type (INTx or MSI or MSI-X)  */
void hfc_set_msi_enable(struct adap_info *ap)
{
	ap->msi_enable = HFC_INT_TYPE_INTX; /* Default: HFC_INT_TYPE_INTX == 0 */
	if(ap->defparam) return; /* Force Default Parameter*/

	if(hfc_chk_conf_val(0,2,hfc_msi_enable)){	/* in case global parameter is set */
		ap->msi_enable = hfc_msi_enable;			/* 0:INTx, 1:MSI, 2:MSI-X */
	}
	
	/* in case local parameter is set */
	if( hfc_chk_conf_val(0,2,hfcmp_msi_enable[ap->instance]) ) {
		ap->msi_enable = hfcmp_msi_enable[ap->instance];		/* 0:INTx, 1:MSI, 2:MSI-X */
		HFC_DBGPRT("msi_enable <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("MSI enable = %d\n", ap->msi_enable);
}

/* Set Max Value of "PCIe IP Core SRAM ERR(CE) Count" */
void hfc_set_pcie_sram_ce_count(struct adap_info *ap){
	
	/* Set Default Parameter */
	ap->max_pcie_sram_ce_cnt = HFC_PCIE_SRAM_CE_CNT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(0,16,hfc_pcie_sram_ce)){
		ap->max_pcie_sram_ce_cnt = hfc_pcie_sram_ce;
	}
	/* in case port parameter is set*/
	if(hfc_chk_conf_val(0,16,hfcmp_pcie_sram_ce[ap->instance]) ) {
		ap->max_pcie_sram_ce_cnt = hfcmp_pcie_sram_ce[ap->instance];
	}
	return;
}
/* Set Max Value of "Core ERR(CE) Count" */
void hfc_set_core_ce_count(struct adap_info *ap){

	/* Set Default Parameter */
	ap->max_core_ce_cnt = HFC_CORE_CE_CNT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(0,16,hfc_core_ce)){
		ap->max_core_ce_cnt = hfc_core_ce;
	}
	/* in case port parameter is set*/
	if(hfc_chk_conf_val(0,16,hfcmp_core_ce[ap->instance]) ) {
		ap->max_core_ce_cnt = hfcmp_core_ce[ap->instance];
	}
	return;
}
/* Set "1:Do" or "0:Not"  dummy read. (for MSI/MSI-X) */
void hfc_set_inta_dummy_read(struct adap_info *ap){

	/* Set Default Parameter */
	ap->inta_dummy_read = HFC_DUMMY_READ_OFF;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0,1,hfc_inta_dummy_read)){
		ap->inta_dummy_read = hfc_inta_dummy_read;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0,1,hfcmp_inta_dummy_read[ap->instance]) ) {
		ap->inta_dummy_read = hfcmp_inta_dummy_read[ap->instance];
	}
	return;
}
/* Set the number of max HW log page count.(0 - 16) */
void hfc_set_max_hwlog_cnt(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->max_hwlog_cnt = HFC_HWLOG_CNT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0,16,hfc_max_hwlog_cnt)){
		ap->max_hwlog_cnt = hfc_max_hwlog_cnt;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0,16,hfcmp_max_hwlog_cnt[ap->instance]) ) {
		ap->max_hwlog_cnt = hfcmp_max_hwlog_cnt[ap->instance];
	}
	return;
}

/* Set Debug mode */
void hfc_set_debug_func(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->debug_func = 0x00;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfc_debug_func)){
		ap->debug_func = (uchar)hfc_debug_func;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfcmp_debug_func[ap->instance]) ) {
		ap->debug_func = (uchar)hfcmp_debug_func[ap->instance];
	}
	return;
}

/* Set Issue D3 Hot in suspend Process */	/* FCLNX-GPL-306 */
void hfc_set_issue_d3hot(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->issue_d3hot = 0x00;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, 1, hfc_issue_d3hot)){
		ap->issue_d3hot = (uchar)hfc_issue_d3hot;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0, 1, hfcmp_issue_d3hot[ap->instance]) ) {
		ap->issue_d3hot = (uchar)hfcmp_issue_d3hot[ap->instance];
	}
	return;
}											/* FCLNX-GPL-306 */

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* Set rport control */
void hfc_set_sysfs_control(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->sysfs_control = 0x00;
	set_bit(HFC_SYSFS_RPORT, (ulong *)&ap->sysfs_control); /* FCLNX-GPL-207 */
	set_bit(HFC_SYSFS_STATISTICS, (ulong *)&ap->sysfs_control); /* FCLNX-GPL-207 */
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfc_sysfs_control)){
		ap->sysfs_control = (uchar)hfc_sysfs_control;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfcmp_sysfs_control[ap->instance]) ) {
		ap->sysfs_control = (uchar)hfcmp_sysfs_control[ap->instance];
	}
	return;
}

/* Set dev_loss_tmo */								/* FCLNX-GPL-260 */
void hfc_set_dev_loss_tmo(struct adap_info *ap)
{
	/* Set Default Parameter */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !(hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		ap->dev_loss_tmo = HFC_DEF_DEV_LOSS_TMO;
	else
		ap->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
#else
	ap->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
#endif
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		/* link down timer  */
		if(hfc_chk_conf_val(0,60,hfc_link_down)){ /* in case global parameter is set */
			ap->dev_loss_tmo = hfc_link_down; 
		}
		switch(ap->instance){
			case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down) ) ap->dev_loss_tmo =  hfc0_link_down; break;
			case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down) ) ap->dev_loss_tmo =  hfc1_link_down; break;
			case  2 : if( hfc_chk_conf_val(0,60, hfc2_link_down) ) ap->dev_loss_tmo =  hfc2_link_down; break;
			case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down) ) ap->dev_loss_tmo =  hfc3_link_down; break;
			case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down) ) ap->dev_loss_tmo =  hfc4_link_down; break;
			case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down) ) ap->dev_loss_tmo =  hfc5_link_down; break;
			case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down) ) ap->dev_loss_tmo =  hfc6_link_down; break;
			case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down) ) ap->dev_loss_tmo =  hfc7_link_down; break;
			case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down) ) ap->dev_loss_tmo =  hfc8_link_down; break;
			case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down) ) ap->dev_loss_tmo =  hfc9_link_down; break;
			case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down) ) ap->dev_loss_tmo = hfc10_link_down; break;
			case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down) ) ap->dev_loss_tmo = hfc11_link_down; break;
			case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down) ) ap->dev_loss_tmo = hfc12_link_down; break;
			case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down) ) ap->dev_loss_tmo = hfc13_link_down; break;
			case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down) ) ap->dev_loss_tmo = hfc14_link_down; break;
			case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down) ) ap->dev_loss_tmo = hfc15_link_down; break;
			case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down) ) ap->dev_loss_tmo = hfc16_link_down; break;
			case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down) ) ap->dev_loss_tmo = hfc17_link_down; break;
			case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down) ) ap->dev_loss_tmo = hfc18_link_down; break;
			case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down) ) ap->dev_loss_tmo = hfc19_link_down; break;
			case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down) ) ap->dev_loss_tmo = hfc20_link_down; break;
			case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down) ) ap->dev_loss_tmo = hfc21_link_down; break;
			case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down) ) ap->dev_loss_tmo = hfc22_link_down; break;
			case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down) ) ap->dev_loss_tmo = hfc23_link_down; break;
			case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down) ) ap->dev_loss_tmo = hfc24_link_down; break;
			case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down) ) ap->dev_loss_tmo = hfc25_link_down; break;
			case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down) ) ap->dev_loss_tmo = hfc26_link_down; break;
			case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down) ) ap->dev_loss_tmo = hfc27_link_down; break;
			case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down) ) ap->dev_loss_tmo = hfc28_link_down; break;
			case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down) ) ap->dev_loss_tmo = hfc29_link_down; break;
			case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down) ) ap->dev_loss_tmo = hfc30_link_down; break;
			case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down) ) ap->dev_loss_tmo = hfc31_link_down; break;
		}

		if( hfc_chk_conf_val(0,60,hfcmp_link_down[ap->instance]) ) {
			ap->dev_loss_tmo = hfcmp_link_down[ap->instance];
			HFC_DBGPRT("linkup_timeout <-- hfcldd.conf\n");
		}
	} else {
		/* in case global parameter is set */
		if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfc_dev_loss_tmo)){
			ap->dev_loss_tmo = (uchar)hfc_dev_loss_tmo;
		}
		/* in case port parameter is set */
		if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfcmp_dev_loss_tmo[ap->instance]) ) {
			ap->dev_loss_tmo = (uchar)hfcmp_dev_loss_tmo[ap->instance];
		}
	}
#else
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfc_dev_loss_tmo)){
		ap->dev_loss_tmo = (uchar)hfc_dev_loss_tmo;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfcmp_dev_loss_tmo[ap->instance]) ) {
		ap->dev_loss_tmo = (uchar)hfcmp_dev_loss_tmo[ap->instance];
	}
#endif
	return;
}													/* FCLNX-GPL-260 */

/* FCLNX-GPL-565 start *//* FCLNX-GPL-575 start */
void hfc_set_scan_finished_tmo(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->scan_finished_tmo = HFC_SCAN_FINISHED_TMO;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_SCAN_FINISHED_TMO, HFC_MAX_SCAN_FINISHED_TMO, hfc_scan_finished_tmo)){
		ap->scan_finished_tmo = hfc_scan_finished_tmo;
	}
	return;
}
/* FCLNX-GPL-565 end *//* FCLNX-GPL-575 end */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/* Set HBA Isolation" */		/* FCLNX-GPL-349 */
void hfc_set_hba_isolation(struct adap_info *ap)
{	
	/* Set Default Parameter */
	ap->hba_isolation = HFC_ISOL_START;

	/* in case Force_Default_Parameter is ON */
	if(ap->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(HFC_ISOL_STOP,HFC_ISOL_START,hfc_hba_isolation)){
		ap->hba_isolation = hfc_hba_isolation;
	}
	return;
}								/* FCLNX-GPL-349 */

/* Set Linkdown(s) Limit  */	/* FCLNX-GPL-349 */
void hfc_set_ld_err_limit_s(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->ld_err_limit_s = HFC_MIN_LD_ERR_LIMIT_S;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_LD_ERR_LIMIT_S,HFC_MAX_LD_ERR_LIMIT_S,hfc_ld_err_limit_s)){
		ap->ld_err_limit_s = hfc_ld_err_limit_s;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_LD_ERR_LIMIT_S,HFC_MAX_LD_ERR_LIMIT_S,hfcmp_ld_err_limit_s[ap->instance]) ) {
		ap->ld_err_limit_s = hfcmp_ld_err_limit_s[ap->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set interface Error Limit */	/* FCLNX-GPL-349 */
void hfc_set_if_err_limit(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->if_err_limit = HFC_MIN_IF_ERR_LIMIT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_IF_ERR_LIMIT,HFC_MAX_IF_ERR_LIMIT,hfc_if_err_limit)){
		ap->if_err_limit = hfc_if_err_limit;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_IF_ERR_LIMIT,HFC_MAX_IF_ERR_LIMIT,hfcmp_if_err_limit[ap->instance]) ) {
		ap->if_err_limit = hfcmp_if_err_limit[ap->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set Time-Out Error Limit */	/* FCLNX-GPL-349 */
void hfc_set_to_err_limit(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->to_err_limit = HFC_MIN_TO_ERR_LIMIT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_TO_ERR_LIMIT,HFC_MAX_TO_ERR_LIMIT,hfc_to_err_limit)){
		ap->to_err_limit = hfc_to_err_limit;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_TO_ERR_LIMIT,HFC_MAX_TO_ERR_LIMIT,hfcmp_to_err_limit[ap->instance]) ) {
		ap->to_err_limit = hfcmp_to_err_limit[ap->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set Mailbox Time-Out Retry  *//* FCLNX-GPL-349 */
void hfc_set_to_reset_retry(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->to_reset_retry = HFC_TO_RESET_RETRY;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_TO_RESET_RETRY_MIN,HFC_TO_RESET_RETRY_MAX,hfc_to_reset_retry)){
		ap->to_reset_retry = hfc_to_reset_retry;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_TO_RESET_RETRY_MIN,HFC_TO_RESET_RETRY_MAX,hfcmp_to_reset_retry[ap->instance]) ) {
		ap->to_reset_retry = hfcmp_to_reset_retry[ap->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set Time-Out Reset Error */	/* FCLNX-GPL-349 */
void hfc_set_rt_err_enable(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->rt_err_enable = HFC_RT_ERR_NOT_SPPRTD;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_RT_ERR_NOT_SPPRTD,HFC_RT_ERR_SPPRTD,hfc_rt_err_enable)){
		ap->rt_err_enable = hfc_rt_err_enable;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_RT_ERR_NOT_SPPRTD,HFC_RT_ERR_SPPRTD,hfcmp_rt_err_enable[ap->instance]) ) {
		ap->rt_err_enable = hfcmp_rt_err_enable[ap->instance];
	}
}								/* FCLNX-GPL-349 */


/* Set limit log */				/* FCLNX-GPL-491 */
void hfc_set_limit_log(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->limit_log = HFC_DISABLE_LIMITLOG;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc_limit_log)){
		ap->limit_log = hfc_limit_log;
	}
	switch(ap->instance){
		case  0 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc0_limit_log)) ap->limit_log =  hfc0_limit_log; break;
		case  1 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc1_limit_log)) ap->limit_log =  hfc1_limit_log; break;
		case  2 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc2_limit_log)) ap->limit_log =  hfc2_limit_log; break;
		case  3 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc3_limit_log)) ap->limit_log =  hfc3_limit_log; break;
		case  4 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc4_limit_log)) ap->limit_log =  hfc4_limit_log; break;
		case  5 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc5_limit_log)) ap->limit_log =  hfc5_limit_log; break;
		case  6 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc6_limit_log)) ap->limit_log =  hfc6_limit_log; break;
		case  7 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc7_limit_log)) ap->limit_log =  hfc7_limit_log; break;
		case  8 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc8_limit_log)) ap->limit_log =  hfc8_limit_log; break;
		case  9 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG, hfc9_limit_log)) ap->limit_log =  hfc9_limit_log; break;
		case 10 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc10_limit_log)) ap->limit_log = hfc10_limit_log; break;
		case 11 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc11_limit_log)) ap->limit_log = hfc11_limit_log; break;
		case 12 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc12_limit_log)) ap->limit_log = hfc12_limit_log; break;
		case 13 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc13_limit_log)) ap->limit_log = hfc13_limit_log; break;
		case 14 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc14_limit_log)) ap->limit_log = hfc14_limit_log; break;
		case 15 : if(hfc_chk_conf_val(HFC_DISABLE_LIMITLOG,HFC_ENABLE_LIMITLOG,hfc15_limit_log)) ap->limit_log = hfc15_limit_log; break;
	}
}								/* FCLNX-GPL-491 */

/* Set filter target */				/* FCLNX-GPL-491 */
void hfc_set_filter_target(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->filter_target = HFC_DISABLE_FILTERTGT;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc_filter_target)){
		ap->filter_target = hfc_filter_target;
	}
	switch(ap->instance){
		case  0 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc0_filter_target)) ap->filter_target =  hfc0_filter_target; break;
		case  1 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc1_filter_target)) ap->filter_target =  hfc1_filter_target; break;
		case  2 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc2_filter_target)) ap->filter_target =  hfc2_filter_target; break;
		case  3 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc3_filter_target)) ap->filter_target =  hfc3_filter_target; break;
		case  4 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc4_filter_target)) ap->filter_target =  hfc4_filter_target; break;
		case  5 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc5_filter_target)) ap->filter_target =  hfc5_filter_target; break;
		case  6 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc6_filter_target)) ap->filter_target =  hfc6_filter_target; break;
		case  7 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc7_filter_target)) ap->filter_target =  hfc7_filter_target; break;
		case  8 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc8_filter_target)) ap->filter_target =  hfc8_filter_target; break;
		case  9 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT, hfc9_filter_target)) ap->filter_target =  hfc9_filter_target; break;
		case 10 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc10_filter_target)) ap->filter_target = hfc10_filter_target; break;
		case 11 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc11_filter_target)) ap->filter_target = hfc11_filter_target; break;
		case 12 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc12_filter_target)) ap->filter_target = hfc12_filter_target; break;
		case 13 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc13_filter_target)) ap->filter_target = hfc13_filter_target; break;
		case 14 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc14_filter_target)) ap->filter_target = hfc14_filter_target; break;
		case 15 : if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc15_filter_target)) ap->filter_target = hfc15_filter_target; break;
	}
	/* FCLNX-GPL-FX-478 >>> */
	if (hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfcmp_filter_target[ap->instance])) {
		ap->filter_target = hfcmp_filter_target[ap->instance];
		HFC_DBGPRT("filter_target <-- hfcldd.conf\n");
	}
	/* <<< FCLNX-GPL-FX-478 */
}								/* FCLNX-GPL-491 */

/* Set statistics for Virtage */				/* FCLNX-GPL-494 */
void hfc_set_hg_stats_disable(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->hg_stats_disable = HFC_ENABLE_HGSTATS;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_ENABLE_HGSTATS,HFC_DISABLE_HGSTATS,hfc_hg_stats_disable)){
		ap->hg_stats_disable = hfc_hg_stats_disable;
	}
}								/* FCLNX-GPL-494 */

/* FCLNX-GPL-547 start */
/* Set a choice of log files  */
void hfc_set_log_file(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->log_file = 0;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, HFC_LOGFILE_TYPE_MAX, hfc_log_file)){
		ap->log_file = hfc_log_file;
	}
	return;
}

/* Set max lu number */
void hfc_set_max_lun(struct adap_info *ap)
{
	/* Set Default Parameter */
	ap->max_lun = HFC_MAX_LUN;
	/* in case Force_Default_Parameter is ON */
	if(ap->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(1, HFC_MAX_LUN, hfc_max_lun)){
		ap->max_lun = hfc_max_lun;
	}
	return;
}
/* FCLNX-GPL-547 end */


/* FCLNX-GPL-575 */
void hfc_set_rport_lu_scan(struct adap_info *ap)
{
	ap->rport_lu_scan = HFC_ENABLE_RPORT_LU_SCAN; /* default = 1 */
	
	if(ap->defparam) return;
	
	/* Control lu scan with rport function */

	if( hfc_chk_conf_val(0,1, hfc_rport_lu_scan)){ /* in case global parameter is set */
		ap->rport_lu_scan = hfc_rport_lu_scan; 
	}

	if( hfc_chk_conf_val(0,1,hfcmp_rport_lu_scan[ap->instance]) ) {
		ap->rport_lu_scan = hfcmp_rport_lu_scan[ap->instance];
		HFC_DBGPRT("max control change_queue_depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("rport lu scan mode = %d \n", ap->rport_lu_scan);
}


/* FCLNX-GPL-574 */
void hfc_set_ctl_change_qdepth(struct adap_info *ap)
{
	ap->ctl_change_qdepth = HFC_DISABLE_CTL_CHANGE_QDEPTH; /* default = 0 */
	
	if(ap->defparam) return;
	
	/* Control change_queue_depth entry point */

	if( hfc_chk_conf_val(0,1, hfc_ctl_change_qdepth)){ /* in case global parameter is set */
		ap->ctl_change_qdepth = hfc_ctl_change_qdepth; 
		
	}

	if( hfc_chk_conf_val(0,1,hfcmp_ctl_change_qdepth[ap->instance]) ) {
		ap->ctl_change_qdepth = hfcmp_ctl_change_qdepth[ap->instance];
		HFC_DBGPRT("max control change_queue_depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("control change_queue_depth = %d \n", ap->ctl_change_qdepth);
}



/* FIVE-FX Variable setting */
void hfc_fx_set_topology(struct port_info *pp)
{
#if 0
	uchar cnv[3]={HFC_UNKN,HFC_PT2PT,HFC_AL}; /* connection type */

	if(pp->defparam) return;

    /* 
	   Attention!! : The BIOS data might already been set. 
                                                           */

	if(hfc_chk_conf_val(1,2,hfc_connection_type)){ /* in case global parameter is set */
		pp->topology  = cnv[hfc_connection_type];	/* 1:P2P, 3:FC-AL */
	}

	/* in case local parameter is set */
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(1,2, hfc0_connection_type) ) pp->topology  =  cnv[hfc0_connection_type]; break;
		case  1 : if( hfc_chk_conf_val(1,2, hfc1_connection_type) ) pp->topology  =  cnv[hfc1_connection_type]; break;
		case  2 : if( hfc_chk_conf_val(1,2, hfc2_connection_type) ) pp->topology  =  cnv[hfc2_connection_type]; break;
		case  3 : if( hfc_chk_conf_val(1,2, hfc3_connection_type) ) pp->topology  =  cnv[hfc3_connection_type]; break;
		case  4 : if( hfc_chk_conf_val(1,2, hfc4_connection_type) ) pp->topology  =  cnv[hfc4_connection_type]; break;
		case  5 : if( hfc_chk_conf_val(1,2, hfc5_connection_type) ) pp->topology  =  cnv[hfc5_connection_type]; break;
		case  6 : if( hfc_chk_conf_val(1,2, hfc6_connection_type) ) pp->topology  =  cnv[hfc6_connection_type]; break;
		case  7 : if( hfc_chk_conf_val(1,2, hfc7_connection_type) ) pp->topology  =  cnv[hfc7_connection_type]; break;
		case  8 : if( hfc_chk_conf_val(1,2, hfc8_connection_type) ) pp->topology  =  cnv[hfc8_connection_type]; break;
		case  9 : if( hfc_chk_conf_val(1,2, hfc9_connection_type) ) pp->topology  =  cnv[hfc9_connection_type]; break;
		case 10 : if( hfc_chk_conf_val(1,2,hfc10_connection_type) ) pp->topology  = cnv[hfc10_connection_type]; break;
		case 11 : if( hfc_chk_conf_val(1,2,hfc11_connection_type) ) pp->topology  = cnv[hfc11_connection_type]; break;
		case 12 : if( hfc_chk_conf_val(1,2,hfc12_connection_type) ) pp->topology  = cnv[hfc12_connection_type]; break;
		case 13 : if( hfc_chk_conf_val(1,2,hfc13_connection_type) ) pp->topology  = cnv[hfc13_connection_type]; break;
		case 14 : if( hfc_chk_conf_val(1,2,hfc14_connection_type) ) pp->topology  = cnv[hfc14_connection_type]; break;
		case 15 : if( hfc_chk_conf_val(1,2,hfc15_connection_type) ) pp->topology  = cnv[hfc15_connection_type]; break;
		case 16 : if( hfc_chk_conf_val(1,2,hfc16_connection_type) ) pp->topology  = cnv[hfc16_connection_type]; break;
		case 17 : if( hfc_chk_conf_val(1,2,hfc17_connection_type) ) pp->topology  = cnv[hfc17_connection_type]; break;
		case 18 : if( hfc_chk_conf_val(1,2,hfc18_connection_type) ) pp->topology  = cnv[hfc18_connection_type]; break;
		case 19 : if( hfc_chk_conf_val(1,2,hfc19_connection_type) ) pp->topology  = cnv[hfc19_connection_type]; break;
		case 20 : if( hfc_chk_conf_val(1,2,hfc20_connection_type) ) pp->topology  = cnv[hfc20_connection_type]; break;
		case 21 : if( hfc_chk_conf_val(1,2,hfc21_connection_type) ) pp->topology  = cnv[hfc21_connection_type]; break;
		case 22 : if( hfc_chk_conf_val(1,2,hfc22_connection_type) ) pp->topology  = cnv[hfc22_connection_type]; break;
		case 23 : if( hfc_chk_conf_val(1,2,hfc23_connection_type) ) pp->topology  = cnv[hfc23_connection_type]; break;
		case 24 : if( hfc_chk_conf_val(1,2,hfc24_connection_type) ) pp->topology  = cnv[hfc24_connection_type]; break;
		case 25 : if( hfc_chk_conf_val(1,2,hfc25_connection_type) ) pp->topology  = cnv[hfc25_connection_type]; break;
		case 26 : if( hfc_chk_conf_val(1,2,hfc26_connection_type) ) pp->topology  = cnv[hfc26_connection_type]; break;
		case 27 : if( hfc_chk_conf_val(1,2,hfc27_connection_type) ) pp->topology  = cnv[hfc27_connection_type]; break;
		case 28 : if( hfc_chk_conf_val(1,2,hfc28_connection_type) ) pp->topology  = cnv[hfc28_connection_type]; break;
		case 29 : if( hfc_chk_conf_val(1,2,hfc29_connection_type) ) pp->topology  = cnv[hfc29_connection_type]; break;
		case 30 : if( hfc_chk_conf_val(1,2,hfc30_connection_type) ) pp->topology  = cnv[hfc30_connection_type]; break;
		case 31 : if( hfc_chk_conf_val(1,2,hfc31_connection_type) ) pp->topology  = cnv[hfc31_connection_type]; break;
	}

	/* in case local parameter is set */
	if( hfc_chk_conf_val( 1,2,hfcmp_connection_type[pp->instance]) ) {
		pp->topology  = cnv[ hfcmp_connection_type[pp->instance] ];
		HFC_DBGPRT("connect_type <-- hfcldd.conf\n");
	}

#endif
	HFC_DBGPRT("connect_type = %d\n", pp->topology);

}


void hfc_fx_set_linkspeed(struct port_info *pp)
{
#if 0
	if(pp->defparam) return;

	if(hfc_chk_conf_ls(1,8,hfc_link_speed)){	/* in case global parameter is set */
		pp->linkspeed = hfc_link_speed;			/* 1:1Gbps, 2:2Gbps, 4:4Gbps 8:8Gbps*/
	}

	switch(pp->instance){
		case  0 : if( hfc_chk_conf_ls(1,8, hfc0_link_speed) ) pp->linkspeed =  hfc0_link_speed; break;
		case  1 : if( hfc_chk_conf_ls(1,8, hfc1_link_speed) ) pp->linkspeed =  hfc1_link_speed; break;
		case  2 : if( hfc_chk_conf_ls(1,8, hfc2_link_speed) ) pp->linkspeed =  hfc2_link_speed; break;
		case  3 : if( hfc_chk_conf_ls(1,8, hfc3_link_speed) ) pp->linkspeed =  hfc3_link_speed; break;
		case  4 : if( hfc_chk_conf_ls(1,8, hfc4_link_speed) ) pp->linkspeed =  hfc4_link_speed; break;
		case  5 : if( hfc_chk_conf_ls(1,8, hfc5_link_speed) ) pp->linkspeed =  hfc5_link_speed; break;
		case  6 : if( hfc_chk_conf_ls(1,8, hfc6_link_speed) ) pp->linkspeed =  hfc6_link_speed; break;
		case  7 : if( hfc_chk_conf_ls(1,8, hfc7_link_speed) ) pp->linkspeed =  hfc7_link_speed; break;
		case  8 : if( hfc_chk_conf_ls(1,8, hfc8_link_speed) ) pp->linkspeed =  hfc8_link_speed; break;
		case  9 : if( hfc_chk_conf_ls(1,8, hfc9_link_speed) ) pp->linkspeed =  hfc9_link_speed; break;
		case 10 : if( hfc_chk_conf_ls(1,8,hfc10_link_speed) ) pp->linkspeed = hfc10_link_speed; break;
		case 11 : if( hfc_chk_conf_ls(1,8,hfc11_link_speed) ) pp->linkspeed = hfc11_link_speed; break;
		case 12 : if( hfc_chk_conf_ls(1,8,hfc12_link_speed) ) pp->linkspeed = hfc12_link_speed; break;
		case 13 : if( hfc_chk_conf_ls(1,8,hfc13_link_speed) ) pp->linkspeed = hfc13_link_speed; break;
		case 14 : if( hfc_chk_conf_ls(1,8,hfc14_link_speed) ) pp->linkspeed = hfc14_link_speed; break;
		case 15 : if( hfc_chk_conf_ls(1,8,hfc15_link_speed) ) pp->linkspeed = hfc15_link_speed; break;
		case 16 : if( hfc_chk_conf_ls(1,8,hfc16_link_speed) ) pp->linkspeed = hfc16_link_speed; break;
		case 17 : if( hfc_chk_conf_ls(1,8,hfc17_link_speed) ) pp->linkspeed = hfc17_link_speed; break;
		case 18 : if( hfc_chk_conf_ls(1,8,hfc18_link_speed) ) pp->linkspeed = hfc18_link_speed; break;
		case 19 : if( hfc_chk_conf_ls(1,8,hfc19_link_speed) ) pp->linkspeed = hfc19_link_speed; break;
		case 20 : if( hfc_chk_conf_ls(1,8,hfc20_link_speed) ) pp->linkspeed = hfc20_link_speed; break;
		case 21 : if( hfc_chk_conf_ls(1,8,hfc21_link_speed) ) pp->linkspeed = hfc21_link_speed; break;
		case 22 : if( hfc_chk_conf_ls(1,8,hfc22_link_speed) ) pp->linkspeed = hfc22_link_speed; break;
		case 23 : if( hfc_chk_conf_ls(1,8,hfc23_link_speed) ) pp->linkspeed = hfc23_link_speed; break;
		case 24 : if( hfc_chk_conf_ls(1,8,hfc24_link_speed) ) pp->linkspeed = hfc24_link_speed; break;
		case 25 : if( hfc_chk_conf_ls(1,8,hfc25_link_speed) ) pp->linkspeed = hfc25_link_speed; break;
		case 26 : if( hfc_chk_conf_ls(1,8,hfc26_link_speed) ) pp->linkspeed = hfc26_link_speed; break;
		case 27 : if( hfc_chk_conf_ls(1,8,hfc27_link_speed) ) pp->linkspeed = hfc27_link_speed; break;
		case 28 : if( hfc_chk_conf_ls(1,8,hfc28_link_speed) ) pp->linkspeed = hfc28_link_speed; break;
		case 29 : if( hfc_chk_conf_ls(1,8,hfc29_link_speed) ) pp->linkspeed = hfc29_link_speed; break;
		case 30 : if( hfc_chk_conf_ls(1,8,hfc30_link_speed) ) pp->linkspeed = hfc30_link_speed; break;
		case 31 : if( hfc_chk_conf_ls(1,8,hfc31_link_speed) ) pp->linkspeed = hfc31_link_speed; break;
	}

	if( hfc_chk_conf_ls(1,8,hfcmp_link_speed[pp->instance] ) ) {
		pp->linkspeed = hfcmp_link_speed[pp->instance];
		HFC_DBGPRT("link speed <-- hfcldd.conf\n");
	}

#endif
	HFC_DBGPRT("link speed = %d\n", pp->linkspeed);
}


void hfc_fx_set_max_transfer(struct port_info *pp)
{
	pp->max_transfer = HFC_MAX_TRANSFER; /* AUTO : default */

	/* max transfer */
	if(pp->defparam){
		pp->dma_max = pp->max_transfer*0x100000;
		return;
	}

	if(hfc_chk_conf_mt(0,32,hfc_max_transfer)){ /* in case global parameter is set */
		pp->max_transfer = hfc_max_transfer; 
	}
	switch(pp->instance){ /* FCLNX-GPL-119 */
		case  0 : if( hfc_chk_conf_mt(0,32, hfc0_max_transfer) ) pp->max_transfer =  hfc0_max_transfer; break;
		case  1 : if( hfc_chk_conf_mt(0,32, hfc1_max_transfer) ) pp->max_transfer =  hfc1_max_transfer; break;
		case  2 : if( hfc_chk_conf_mt(0,32, hfc2_max_transfer) ) pp->max_transfer =  hfc2_max_transfer; break;
		case  3 : if( hfc_chk_conf_mt(0,32, hfc3_max_transfer) ) pp->max_transfer =  hfc3_max_transfer; break;
		case  4 : if( hfc_chk_conf_mt(0,32, hfc4_max_transfer) ) pp->max_transfer =  hfc4_max_transfer; break;
		case  5 : if( hfc_chk_conf_mt(0,32, hfc5_max_transfer) ) pp->max_transfer =  hfc5_max_transfer; break;
		case  6 : if( hfc_chk_conf_mt(0,32, hfc6_max_transfer) ) pp->max_transfer =  hfc6_max_transfer; break;
		case  7 : if( hfc_chk_conf_mt(0,32, hfc7_max_transfer) ) pp->max_transfer =  hfc7_max_transfer; break;
		case  8 : if( hfc_chk_conf_mt(0,32, hfc8_max_transfer) ) pp->max_transfer =  hfc8_max_transfer; break;
		case  9 : if( hfc_chk_conf_mt(0,32, hfc9_max_transfer) ) pp->max_transfer =  hfc9_max_transfer; break;
		case 10 : if( hfc_chk_conf_mt(0,32,hfc10_max_transfer) ) pp->max_transfer = hfc10_max_transfer; break;
		case 11 : if( hfc_chk_conf_mt(0,32,hfc11_max_transfer) ) pp->max_transfer = hfc11_max_transfer; break;
		case 12 : if( hfc_chk_conf_mt(0,32,hfc12_max_transfer) ) pp->max_transfer = hfc12_max_transfer; break;
		case 13 : if( hfc_chk_conf_mt(0,32,hfc13_max_transfer) ) pp->max_transfer = hfc13_max_transfer; break;
		case 14 : if( hfc_chk_conf_mt(0,32,hfc14_max_transfer) ) pp->max_transfer = hfc14_max_transfer; break;
		case 15 : if( hfc_chk_conf_mt(0,32,hfc15_max_transfer) ) pp->max_transfer = hfc15_max_transfer; break;
		case 16 : if( hfc_chk_conf_mt(0,32,hfc16_max_transfer) ) pp->max_transfer = hfc16_max_transfer; break;
		case 17 : if( hfc_chk_conf_mt(0,32,hfc17_max_transfer) ) pp->max_transfer = hfc17_max_transfer; break;
		case 18 : if( hfc_chk_conf_mt(0,32,hfc18_max_transfer) ) pp->max_transfer = hfc18_max_transfer; break;
		case 19 : if( hfc_chk_conf_mt(0,32,hfc19_max_transfer) ) pp->max_transfer = hfc19_max_transfer; break;
		case 20 : if( hfc_chk_conf_mt(0,32,hfc20_max_transfer) ) pp->max_transfer = hfc20_max_transfer; break;
		case 21 : if( hfc_chk_conf_mt(0,32,hfc21_max_transfer) ) pp->max_transfer = hfc21_max_transfer; break;
		case 22 : if( hfc_chk_conf_mt(0,32,hfc22_max_transfer) ) pp->max_transfer = hfc22_max_transfer; break;
		case 23 : if( hfc_chk_conf_mt(0,32,hfc23_max_transfer) ) pp->max_transfer = hfc23_max_transfer; break;
		case 24 : if( hfc_chk_conf_mt(0,32,hfc24_max_transfer) ) pp->max_transfer = hfc24_max_transfer; break;
		case 25 : if( hfc_chk_conf_mt(0,32,hfc25_max_transfer) ) pp->max_transfer = hfc25_max_transfer; break;
		case 26 : if( hfc_chk_conf_mt(0,32,hfc26_max_transfer) ) pp->max_transfer = hfc26_max_transfer; break;
		case 27 : if( hfc_chk_conf_mt(0,32,hfc27_max_transfer) ) pp->max_transfer = hfc27_max_transfer; break;
		case 28 : if( hfc_chk_conf_mt(0,32,hfc28_max_transfer) ) pp->max_transfer = hfc28_max_transfer; break;
		case 29 : if( hfc_chk_conf_mt(0,32,hfc29_max_transfer) ) pp->max_transfer = hfc29_max_transfer; break;
		case 30 : if( hfc_chk_conf_mt(0,32,hfc30_max_transfer) ) pp->max_transfer = hfc30_max_transfer; break;
		case 31 : if( hfc_chk_conf_mt(0,32,hfc31_max_transfer) ) pp->max_transfer = hfc31_max_transfer; break;
	}

	if( hfc_chk_conf_mt(0,32,hfcmp_max_transfer[pp->instance] ) ) {
		pp->max_transfer = hfcmp_max_transfer[pp->instance];
		HFC_DBGPRT("max_transfer <-- hfcldd.conf\n");
	}

	pp->dma_max = pp->max_transfer*0x100000;
	HFC_DBGPRT("max_transfer = %d, dma_max = %d\n", pp->max_transfer, pp->dma_max);

}



void hfc_fx_set_linkup_tmo(struct port_info *pp)
{
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		pp->linkup_tmo = HFC_LINKUP_TO;
	else
		pp->linkup_tmo = HFC_PCM_LINKUP_TO;

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		return;
#endif

	if(pp->defparam) return;

	/* link down timer  */
	if(hfc_chk_conf_val(0,60,hfc_link_down)){ /* in case global parameter is set */
		pp->linkup_tmo = hfc_link_down; 

	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down) ) pp->linkup_tmo =  hfc0_link_down; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down) ) pp->linkup_tmo =  hfc1_link_down; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_link_down) ) pp->linkup_tmo =  hfc2_link_down; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down) ) pp->linkup_tmo =  hfc3_link_down; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down) ) pp->linkup_tmo =  hfc4_link_down; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down) ) pp->linkup_tmo =  hfc5_link_down; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down) ) pp->linkup_tmo =  hfc6_link_down; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down) ) pp->linkup_tmo =  hfc7_link_down; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down) ) pp->linkup_tmo =  hfc8_link_down; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down) ) pp->linkup_tmo =  hfc9_link_down; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down) ) pp->linkup_tmo = hfc10_link_down; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down) ) pp->linkup_tmo = hfc11_link_down; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down) ) pp->linkup_tmo = hfc12_link_down; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down) ) pp->linkup_tmo = hfc13_link_down; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down) ) pp->linkup_tmo = hfc14_link_down; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down) ) pp->linkup_tmo = hfc15_link_down; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down) ) pp->linkup_tmo = hfc16_link_down; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down) ) pp->linkup_tmo = hfc17_link_down; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down) ) pp->linkup_tmo = hfc18_link_down; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down) ) pp->linkup_tmo = hfc19_link_down; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down) ) pp->linkup_tmo = hfc20_link_down; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down) ) pp->linkup_tmo = hfc21_link_down; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down) ) pp->linkup_tmo = hfc22_link_down; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down) ) pp->linkup_tmo = hfc23_link_down; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down) ) pp->linkup_tmo = hfc24_link_down; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down) ) pp->linkup_tmo = hfc25_link_down; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down) ) pp->linkup_tmo = hfc26_link_down; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down) ) pp->linkup_tmo = hfc27_link_down; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down) ) pp->linkup_tmo = hfc28_link_down; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down) ) pp->linkup_tmo = hfc29_link_down; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down) ) pp->linkup_tmo = hfc30_link_down; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down) ) pp->linkup_tmo = hfc31_link_down; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_link_down[pp->instance]) ) {
		pp->linkup_tmo = hfcmp_link_down[pp->instance];
		HFC_DBGPRT("linkup_timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("linkup_timeout = %d \n", pp->linkup_tmo);
}

#if 0	/* FIVE-FX */
void hfc_fx_set_linkup2_tmo(struct port_info *pp)
{

	pp->mck_rcv_tmo = HFC_LINKUP2_TO; /* default = 15s */
	if(pp->defparam) return;

	/* link down timer  */
	if(hfc_chk_conf_val(0,60,hfc_link_down2)){ /* in case global parameter is set */
		pp->mck_rcv_tmo = hfc_link_down2; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down2) ) pp->mck_rcv_tmo =  hfc0_link_down2; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down2) ) pp->mck_rcv_tmo =  hfc2_link_down2; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down2) ) pp->mck_rcv_tmo =  hfc3_link_down2; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down2) ) pp->mck_rcv_tmo =  hfc4_link_down2; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down2) ) pp->mck_rcv_tmo =  hfc5_link_down2; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down2) ) pp->mck_rcv_tmo =  hfc6_link_down2; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down2) ) pp->mck_rcv_tmo =  hfc7_link_down2; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down2) ) pp->mck_rcv_tmo =  hfc8_link_down2; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down2) ) pp->mck_rcv_tmo =  hfc9_link_down2; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down2) ) pp->mck_rcv_tmo = hfc10_link_down2; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down2) ) pp->mck_rcv_tmo = hfc11_link_down2; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down2) ) pp->mck_rcv_tmo = hfc12_link_down2; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down2) ) pp->mck_rcv_tmo = hfc13_link_down2; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down2) ) pp->mck_rcv_tmo = hfc14_link_down2; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down2) ) pp->mck_rcv_tmo = hfc15_link_down2; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down2) ) pp->mck_rcv_tmo = hfc16_link_down2; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down2) ) pp->mck_rcv_tmo = hfc17_link_down2; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down2) ) pp->mck_rcv_tmo = hfc18_link_down2; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down2) ) pp->mck_rcv_tmo = hfc19_link_down2; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down2) ) pp->mck_rcv_tmo = hfc20_link_down2; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down2) ) pp->mck_rcv_tmo = hfc21_link_down2; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down2) ) pp->mck_rcv_tmo = hfc22_link_down2; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down2) ) pp->mck_rcv_tmo = hfc23_link_down2; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down2) ) pp->mck_rcv_tmo = hfc24_link_down2; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down2) ) pp->mck_rcv_tmo = hfc25_link_down2; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down2) ) pp->mck_rcv_tmo = hfc26_link_down2; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down2) ) pp->mck_rcv_tmo = hfc27_link_down2; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down2) ) pp->mck_rcv_tmo = hfc28_link_down2; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down2) ) pp->mck_rcv_tmo = hfc29_link_down2; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down2) ) pp->mck_rcv_tmo = hfc30_link_down2; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down2) ) pp->mck_rcv_tmo = hfc31_link_down2; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_link_down2[pp->instance]) ) {
		pp->mck_rcv_tmo = hfcmp_link_down2[pp->instance];
		HFC_DBGPRT("linkup_timeout2 <-- hfcldd.conf\n");
	}

}
#endif		/* FIVE-FX */

void hfc_fx_set_reset_delay(struct port_info *pp)
{

	pp->scsi_reset_delay = HFC_SCSI_RESET_DELAY_MIN; /* default = 0s */

	if(pp->defparam) return;

	/* link reset delay timer  */

	if(hfc_chk_conf_val(0,60,hfc_reset_delay)){ /* in case global parameter is set */
		pp->scsi_reset_delay = hfc_reset_delay; 

	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_reset_delay) ) pp->scsi_reset_delay =  hfc0_reset_delay; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_reset_delay) ) pp->scsi_reset_delay =  hfc1_reset_delay; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_reset_delay) ) pp->scsi_reset_delay =  hfc2_reset_delay; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_reset_delay) ) pp->scsi_reset_delay =  hfc3_reset_delay; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_reset_delay) ) pp->scsi_reset_delay =  hfc4_reset_delay; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_reset_delay) ) pp->scsi_reset_delay =  hfc5_reset_delay; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_reset_delay) ) pp->scsi_reset_delay =  hfc6_reset_delay; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_reset_delay) ) pp->scsi_reset_delay =  hfc7_reset_delay; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_reset_delay) ) pp->scsi_reset_delay =  hfc8_reset_delay; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_reset_delay) ) pp->scsi_reset_delay =  hfc9_reset_delay; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_reset_delay) ) pp->scsi_reset_delay = hfc10_reset_delay; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_reset_delay) ) pp->scsi_reset_delay = hfc11_reset_delay; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_reset_delay) ) pp->scsi_reset_delay = hfc12_reset_delay; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_reset_delay) ) pp->scsi_reset_delay = hfc13_reset_delay; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_reset_delay) ) pp->scsi_reset_delay = hfc14_reset_delay; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_reset_delay) ) pp->scsi_reset_delay = hfc15_reset_delay; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_reset_delay) ) pp->scsi_reset_delay = hfc16_reset_delay; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_reset_delay) ) pp->scsi_reset_delay = hfc17_reset_delay; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_reset_delay) ) pp->scsi_reset_delay = hfc18_reset_delay; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_reset_delay) ) pp->scsi_reset_delay = hfc19_reset_delay; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_reset_delay) ) pp->scsi_reset_delay = hfc20_reset_delay; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_reset_delay) ) pp->scsi_reset_delay = hfc21_reset_delay; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_reset_delay) ) pp->scsi_reset_delay = hfc22_reset_delay; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_reset_delay) ) pp->scsi_reset_delay = hfc23_reset_delay; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_reset_delay) ) pp->scsi_reset_delay = hfc24_reset_delay; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_reset_delay) ) pp->scsi_reset_delay = hfc25_reset_delay; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_reset_delay) ) pp->scsi_reset_delay = hfc26_reset_delay; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_reset_delay) ) pp->scsi_reset_delay = hfc27_reset_delay; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_reset_delay) ) pp->scsi_reset_delay = hfc28_reset_delay; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_reset_delay) ) pp->scsi_reset_delay = hfc29_reset_delay; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_reset_delay) ) pp->scsi_reset_delay = hfc30_reset_delay; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_reset_delay) ) pp->scsi_reset_delay = hfc31_reset_delay; break;
	}
	
	if( hfc_chk_conf_val(0,60,hfcmp_reset_delay[pp->instance]) ) {
		pp->scsi_reset_delay = hfcmp_reset_delay[pp->instance];
		HFC_DBGPRT("reset_delay <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("reset_delay = %d \n", pp->scsi_reset_delay);
}



void hfc_fx_set_mck_count(struct port_info *pp)
{
	pp->max_mck_cnt = HFC_FX_DF_MAX_MCK_CNT; /* default = 8 */
	
	if(pp->defparam) return;
	
	/* machine check recovery retry count */

	if( hfc_chk_conf_val(0,10, hfc_mck_retry)){ /* in case global parameter is set */
		pp->max_mck_cnt = hfc_mck_retry; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_mck_retry) ) pp->max_mck_cnt =  hfc0_mck_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_mck_retry) ) pp->max_mck_cnt =  hfc1_mck_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_mck_retry) ) pp->max_mck_cnt =  hfc2_mck_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_mck_retry) ) pp->max_mck_cnt =  hfc3_mck_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_mck_retry) ) pp->max_mck_cnt =  hfc4_mck_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_mck_retry) ) pp->max_mck_cnt =  hfc5_mck_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_mck_retry) ) pp->max_mck_cnt =  hfc6_mck_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_mck_retry) ) pp->max_mck_cnt =  hfc7_mck_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_mck_retry) ) pp->max_mck_cnt =  hfc8_mck_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_mck_retry) ) pp->max_mck_cnt =  hfc9_mck_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_mck_retry) ) pp->max_mck_cnt = hfc10_mck_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_mck_retry) ) pp->max_mck_cnt = hfc11_mck_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_mck_retry) ) pp->max_mck_cnt = hfc12_mck_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_mck_retry) ) pp->max_mck_cnt = hfc13_mck_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_mck_retry) ) pp->max_mck_cnt = hfc14_mck_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_mck_retry) ) pp->max_mck_cnt = hfc15_mck_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_mck_retry) ) pp->max_mck_cnt = hfc16_mck_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_mck_retry) ) pp->max_mck_cnt = hfc17_mck_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_mck_retry) ) pp->max_mck_cnt = hfc18_mck_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_mck_retry) ) pp->max_mck_cnt = hfc19_mck_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_mck_retry) ) pp->max_mck_cnt = hfc20_mck_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_mck_retry) ) pp->max_mck_cnt = hfc21_mck_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_mck_retry) ) pp->max_mck_cnt = hfc22_mck_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_mck_retry) ) pp->max_mck_cnt = hfc23_mck_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_mck_retry) ) pp->max_mck_cnt = hfc24_mck_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_mck_retry) ) pp->max_mck_cnt = hfc25_mck_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_mck_retry) ) pp->max_mck_cnt = hfc26_mck_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_mck_retry) ) pp->max_mck_cnt = hfc27_mck_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_mck_retry) ) pp->max_mck_cnt = hfc28_mck_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_mck_retry) ) pp->max_mck_cnt = hfc29_mck_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_mck_retry) ) pp->max_mck_cnt = hfc30_mck_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_mck_retry) ) pp->max_mck_cnt = hfc31_mck_retry; break;
	}

	if( hfc_chk_conf_val(0,10,hfcmp_mck_retry[pp->instance]) ) {
		pp->max_mck_cnt = hfcmp_mck_retry[pp->instance];
		HFC_DBGPRT("max machine check count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max machine check count = %d \n", pp->max_mck_cnt);
}



void hfc_fx_set_pref_alpa(struct port_info *pp)
{
	pp->pref_alpa = 0x01; /* default = 0x01 */
	
	if(pp->defparam) return;
		
	/* preferred ALPA */
	
	if(hfc_chk_conf_val(0x01,0xef,hfc_preferred_alpa)){ /* in case global parameter is set */
		pp->pref_alpa = hfc_preferred_alpa; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0x01,0xef, hfc0_preferred_alpa) ) pp->pref_alpa =  hfc0_preferred_alpa; break;
		case  1 : if( hfc_chk_conf_val(0x01,0xef, hfc1_preferred_alpa) ) pp->pref_alpa =  hfc1_preferred_alpa; break;
		case  2 : if( hfc_chk_conf_val(0x01,0xef, hfc2_preferred_alpa) ) pp->pref_alpa =  hfc2_preferred_alpa; break;
		case  3 : if( hfc_chk_conf_val(0x01,0xef, hfc3_preferred_alpa) ) pp->pref_alpa =  hfc3_preferred_alpa; break;
		case  4 : if( hfc_chk_conf_val(0x01,0xef, hfc4_preferred_alpa) ) pp->pref_alpa =  hfc4_preferred_alpa; break;
		case  5 : if( hfc_chk_conf_val(0x01,0xef, hfc5_preferred_alpa) ) pp->pref_alpa =  hfc5_preferred_alpa; break;
		case  6 : if( hfc_chk_conf_val(0x01,0xef, hfc6_preferred_alpa) ) pp->pref_alpa =  hfc6_preferred_alpa; break;
		case  7 : if( hfc_chk_conf_val(0x01,0xef, hfc7_preferred_alpa) ) pp->pref_alpa =  hfc7_preferred_alpa; break;
		case  8 : if( hfc_chk_conf_val(0x01,0xef, hfc8_preferred_alpa) ) pp->pref_alpa =  hfc8_preferred_alpa; break;
		case  9 : if( hfc_chk_conf_val(0x01,0xef, hfc9_preferred_alpa) ) pp->pref_alpa =  hfc9_preferred_alpa; break;
		case 10 : if( hfc_chk_conf_val(0x01,0xef,hfc10_preferred_alpa) ) pp->pref_alpa = hfc10_preferred_alpa; break;
		case 11 : if( hfc_chk_conf_val(0x01,0xef,hfc11_preferred_alpa) ) pp->pref_alpa = hfc11_preferred_alpa; break;
		case 12 : if( hfc_chk_conf_val(0x01,0xef,hfc12_preferred_alpa) ) pp->pref_alpa = hfc12_preferred_alpa; break;
		case 13 : if( hfc_chk_conf_val(0x01,0xef,hfc13_preferred_alpa) ) pp->pref_alpa = hfc13_preferred_alpa; break;
		case 14 : if( hfc_chk_conf_val(0x01,0xef,hfc14_preferred_alpa) ) pp->pref_alpa = hfc14_preferred_alpa; break;
		case 15 : if( hfc_chk_conf_val(0x01,0xef,hfc15_preferred_alpa) ) pp->pref_alpa = hfc15_preferred_alpa; break;
		case 16 : if( hfc_chk_conf_val(0x01,0xef,hfc16_preferred_alpa) ) pp->pref_alpa = hfc16_preferred_alpa; break;
		case 17 : if( hfc_chk_conf_val(0x01,0xef,hfc17_preferred_alpa) ) pp->pref_alpa = hfc17_preferred_alpa; break;
		case 18 : if( hfc_chk_conf_val(0x01,0xef,hfc18_preferred_alpa) ) pp->pref_alpa = hfc18_preferred_alpa; break;
		case 19 : if( hfc_chk_conf_val(0x01,0xef,hfc19_preferred_alpa) ) pp->pref_alpa = hfc19_preferred_alpa; break;
		case 20 : if( hfc_chk_conf_val(0x01,0xef,hfc20_preferred_alpa) ) pp->pref_alpa = hfc20_preferred_alpa; break;
		case 21 : if( hfc_chk_conf_val(0x01,0xef,hfc21_preferred_alpa) ) pp->pref_alpa = hfc21_preferred_alpa; break;
		case 22 : if( hfc_chk_conf_val(0x01,0xef,hfc22_preferred_alpa) ) pp->pref_alpa = hfc22_preferred_alpa; break;
		case 23 : if( hfc_chk_conf_val(0x01,0xef,hfc23_preferred_alpa) ) pp->pref_alpa = hfc23_preferred_alpa; break;
		case 24 : if( hfc_chk_conf_val(0x01,0xef,hfc24_preferred_alpa) ) pp->pref_alpa = hfc24_preferred_alpa; break;
		case 25 : if( hfc_chk_conf_val(0x01,0xef,hfc25_preferred_alpa) ) pp->pref_alpa = hfc25_preferred_alpa; break;
		case 26 : if( hfc_chk_conf_val(0x01,0xef,hfc26_preferred_alpa) ) pp->pref_alpa = hfc26_preferred_alpa; break;
		case 27 : if( hfc_chk_conf_val(0x01,0xef,hfc27_preferred_alpa) ) pp->pref_alpa = hfc27_preferred_alpa; break;
		case 28 : if( hfc_chk_conf_val(0x01,0xef,hfc28_preferred_alpa) ) pp->pref_alpa = hfc28_preferred_alpa; break;
		case 29 : if( hfc_chk_conf_val(0x01,0xef,hfc29_preferred_alpa) ) pp->pref_alpa = hfc29_preferred_alpa; break;
		case 30 : if( hfc_chk_conf_val(0x01,0xef,hfc30_preferred_alpa) ) pp->pref_alpa = hfc30_preferred_alpa; break;
		case 31 : if( hfc_chk_conf_val(0x01,0xef,hfc31_preferred_alpa) ) pp->pref_alpa = hfc31_preferred_alpa; break;
	}
	
	if( hfc_chk_conf_val(0x01,0xef,hfcmp_preferred_alpa[pp->instance]) ) {
		pp->pref_alpa = hfcmp_preferred_alpa[pp->instance];
		HFC_DBGPRT("preferred alpa <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("preferred alpa = %d \n", pp->pref_alpa);
}


void hfc_fx_set_target_timeout(struct port_info *pp)
{
	pp->target_reset_tmo = HFC_TARGET_RST_TO; /* default = 20s */

	/* target reset timeout */
	
	if(pp->defparam) return;
	
	if(hfc_chk_conf_val(0,60,hfc_reset_timeout)){ /* in case global parameter is set */
		pp->target_reset_tmo = hfc_reset_timeout;
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_reset_timeout) ) pp->target_reset_tmo =  hfc0_reset_timeout; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_reset_timeout) ) pp->target_reset_tmo =  hfc1_reset_timeout; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_reset_timeout) ) pp->target_reset_tmo =  hfc2_reset_timeout; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_reset_timeout) ) pp->target_reset_tmo =  hfc3_reset_timeout; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_reset_timeout) ) pp->target_reset_tmo =  hfc4_reset_timeout; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_reset_timeout) ) pp->target_reset_tmo =  hfc5_reset_timeout; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_reset_timeout) ) pp->target_reset_tmo =  hfc6_reset_timeout; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_reset_timeout) ) pp->target_reset_tmo =  hfc7_reset_timeout; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_reset_timeout) ) pp->target_reset_tmo =  hfc8_reset_timeout; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_reset_timeout) ) pp->target_reset_tmo =  hfc9_reset_timeout; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_reset_timeout) ) pp->target_reset_tmo = hfc10_reset_timeout; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_reset_timeout) ) pp->target_reset_tmo = hfc11_reset_timeout; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_reset_timeout) ) pp->target_reset_tmo = hfc12_reset_timeout; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_reset_timeout) ) pp->target_reset_tmo = hfc13_reset_timeout; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_reset_timeout) ) pp->target_reset_tmo = hfc14_reset_timeout; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_reset_timeout) ) pp->target_reset_tmo = hfc15_reset_timeout; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_reset_timeout) ) pp->target_reset_tmo = hfc16_reset_timeout; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_reset_timeout) ) pp->target_reset_tmo = hfc17_reset_timeout; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_reset_timeout) ) pp->target_reset_tmo = hfc18_reset_timeout; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_reset_timeout) ) pp->target_reset_tmo = hfc19_reset_timeout; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_reset_timeout) ) pp->target_reset_tmo = hfc20_reset_timeout; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_reset_timeout) ) pp->target_reset_tmo = hfc21_reset_timeout; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_reset_timeout) ) pp->target_reset_tmo = hfc22_reset_timeout; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_reset_timeout) ) pp->target_reset_tmo = hfc23_reset_timeout; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_reset_timeout) ) pp->target_reset_tmo = hfc24_reset_timeout; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_reset_timeout) ) pp->target_reset_tmo = hfc25_reset_timeout; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_reset_timeout) ) pp->target_reset_tmo = hfc26_reset_timeout; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_reset_timeout) ) pp->target_reset_tmo = hfc27_reset_timeout; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_reset_timeout) ) pp->target_reset_tmo = hfc28_reset_timeout; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_reset_timeout) ) pp->target_reset_tmo = hfc29_reset_timeout; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_reset_timeout) ) pp->target_reset_tmo = hfc30_reset_timeout; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_reset_timeout) ) pp->target_reset_tmo = hfc31_reset_timeout; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_reset_timeout[pp->instance]) ) {
		pp->target_reset_tmo = hfcmp_reset_timeout[pp->instance];
		HFC_DBGPRT("target reset timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("target reset timeout = %d \n", pp->target_reset_tmo);
}



void hfc_fx_set_abort_timeout(struct port_info *pp)
{
	
	pp->abort_tmo = HFC_ABORT_ACA_TO;

	if(pp->defparam) return;

	/* abort task set timeout */
	
	if(hfc_chk_conf_val(0,60,hfc_abort_timeout)){ /* in case global parameter is set */
		pp->abort_tmo = hfc_abort_timeout; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_abort_timeout) ) pp->abort_tmo =  hfc0_abort_timeout; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_abort_timeout) ) pp->abort_tmo =  hfc1_abort_timeout; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_abort_timeout) ) pp->abort_tmo =  hfc2_abort_timeout; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_abort_timeout) ) pp->abort_tmo =  hfc3_abort_timeout; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_abort_timeout) ) pp->abort_tmo =  hfc4_abort_timeout; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_abort_timeout) ) pp->abort_tmo =  hfc5_abort_timeout; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_abort_timeout) ) pp->abort_tmo =  hfc6_abort_timeout; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_abort_timeout) ) pp->abort_tmo =  hfc7_abort_timeout; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_abort_timeout) ) pp->abort_tmo =  hfc8_abort_timeout; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_abort_timeout) ) pp->abort_tmo =  hfc9_abort_timeout; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_abort_timeout) ) pp->abort_tmo = hfc10_abort_timeout; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_abort_timeout) ) pp->abort_tmo = hfc11_abort_timeout; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_abort_timeout) ) pp->abort_tmo = hfc12_abort_timeout; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_abort_timeout) ) pp->abort_tmo = hfc13_abort_timeout; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_abort_timeout) ) pp->abort_tmo = hfc14_abort_timeout; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_abort_timeout) ) pp->abort_tmo = hfc15_abort_timeout; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_abort_timeout) ) pp->abort_tmo = hfc16_abort_timeout; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_abort_timeout) ) pp->abort_tmo = hfc17_abort_timeout; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_abort_timeout) ) pp->abort_tmo = hfc18_abort_timeout; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_abort_timeout) ) pp->abort_tmo = hfc19_abort_timeout; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_abort_timeout) ) pp->abort_tmo = hfc20_abort_timeout; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_abort_timeout) ) pp->abort_tmo = hfc21_abort_timeout; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_abort_timeout) ) pp->abort_tmo = hfc22_abort_timeout; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_abort_timeout) ) pp->abort_tmo = hfc23_abort_timeout; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_abort_timeout) ) pp->abort_tmo = hfc24_abort_timeout; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_abort_timeout) ) pp->abort_tmo = hfc25_abort_timeout; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_abort_timeout) ) pp->abort_tmo = hfc26_abort_timeout; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_abort_timeout) ) pp->abort_tmo = hfc27_abort_timeout; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_abort_timeout) ) pp->abort_tmo = hfc28_abort_timeout; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_abort_timeout) ) pp->abort_tmo = hfc29_abort_timeout; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_abort_timeout) ) pp->abort_tmo = hfc30_abort_timeout; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_abort_timeout) ) pp->abort_tmo = hfc31_abort_timeout; break;
	}

	if( hfc_chk_conf_val(0,60,hfcmp_abort_timeout[pp->instance]) ) {
		pp->abort_tmo = hfcmp_abort_timeout[pp->instance];
		HFC_DBGPRT("abort timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("abort timeout = %d \n", pp->abort_tmo );

}


void hfc_fx_set_seg_trace(struct port_info *pp)
{
#if 0
	pp->fw_parm = 0;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_seg_trace)){ /* in case global parameter is set */
		pp->fw_parm = hfc_seg_trace; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,1, hfc0_seg_trace) ) pp->fw_parm =  hfc0_seg_trace; break;
		case  1 : if( hfc_chk_conf_val(0,1, hfc1_seg_trace) ) pp->fw_parm =  hfc1_seg_trace; break;
		case  2 : if( hfc_chk_conf_val(0,1, hfc2_seg_trace) ) pp->fw_parm =  hfc2_seg_trace; break;
		case  3 : if( hfc_chk_conf_val(0,1, hfc3_seg_trace) ) pp->fw_parm =  hfc3_seg_trace; break;
		case  4 : if( hfc_chk_conf_val(0,1, hfc4_seg_trace) ) pp->fw_parm =  hfc4_seg_trace; break;
		case  5 : if( hfc_chk_conf_val(0,1, hfc5_seg_trace) ) pp->fw_parm =  hfc5_seg_trace; break;
		case  6 : if( hfc_chk_conf_val(0,1, hfc6_seg_trace) ) pp->fw_parm =  hfc6_seg_trace; break;
		case  7 : if( hfc_chk_conf_val(0,1, hfc7_seg_trace) ) pp->fw_parm =  hfc7_seg_trace; break;
		case  8 : if( hfc_chk_conf_val(0,1, hfc8_seg_trace) ) pp->fw_parm =  hfc8_seg_trace; break;
		case  9 : if( hfc_chk_conf_val(0,1, hfc9_seg_trace) ) pp->fw_parm =  hfc9_seg_trace; break;
		case 10 : if( hfc_chk_conf_val(0,1,hfc10_seg_trace) ) pp->fw_parm = hfc10_seg_trace; break;
		case 11 : if( hfc_chk_conf_val(0,1,hfc11_seg_trace) ) pp->fw_parm = hfc11_seg_trace; break;
		case 12 : if( hfc_chk_conf_val(0,1,hfc12_seg_trace) ) pp->fw_parm = hfc12_seg_trace; break;
		case 13 : if( hfc_chk_conf_val(0,1,hfc13_seg_trace) ) pp->fw_parm = hfc13_seg_trace; break;
		case 14 : if( hfc_chk_conf_val(0,1,hfc14_seg_trace) ) pp->fw_parm = hfc14_seg_trace; break;
		case 15 : if( hfc_chk_conf_val(0,1,hfc15_seg_trace) ) pp->fw_parm = hfc15_seg_trace; break;
		case 16 : if( hfc_chk_conf_val(0,1,hfc16_seg_trace) ) pp->fw_parm = hfc16_seg_trace; break;
		case 17 : if( hfc_chk_conf_val(0,1,hfc17_seg_trace) ) pp->fw_parm = hfc17_seg_trace; break;
		case 18 : if( hfc_chk_conf_val(0,1,hfc18_seg_trace) ) pp->fw_parm = hfc18_seg_trace; break;
		case 19 : if( hfc_chk_conf_val(0,1,hfc19_seg_trace) ) pp->fw_parm = hfc19_seg_trace; break;
		case 20 : if( hfc_chk_conf_val(0,1,hfc20_seg_trace) ) pp->fw_parm = hfc20_seg_trace; break;
		case 21 : if( hfc_chk_conf_val(0,1,hfc21_seg_trace) ) pp->fw_parm = hfc21_seg_trace; break;
		case 22 : if( hfc_chk_conf_val(0,1,hfc22_seg_trace) ) pp->fw_parm = hfc22_seg_trace; break;
		case 23 : if( hfc_chk_conf_val(0,1,hfc23_seg_trace) ) pp->fw_parm = hfc23_seg_trace; break;
		case 24 : if( hfc_chk_conf_val(0,1,hfc24_seg_trace) ) pp->fw_parm = hfc24_seg_trace; break;
		case 25 : if( hfc_chk_conf_val(0,1,hfc25_seg_trace) ) pp->fw_parm = hfc25_seg_trace; break;
		case 26 : if( hfc_chk_conf_val(0,1,hfc26_seg_trace) ) pp->fw_parm = hfc26_seg_trace; break;
		case 27 : if( hfc_chk_conf_val(0,1,hfc27_seg_trace) ) pp->fw_parm = hfc27_seg_trace; break;
		case 28 : if( hfc_chk_conf_val(0,1,hfc28_seg_trace) ) pp->fw_parm = hfc28_seg_trace; break;
		case 29 : if( hfc_chk_conf_val(0,1,hfc29_seg_trace) ) pp->fw_parm = hfc29_seg_trace; break;
		case 30 : if( hfc_chk_conf_val(0,1,hfc30_seg_trace) ) pp->fw_parm = hfc30_seg_trace; break;
		case 31 : if( hfc_chk_conf_val(0,1,hfc31_seg_trace) ) pp->fw_parm = hfc31_seg_trace; break;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_seg_trace[pp->instance]) ) {
		pp->fw_parm = hfcmp_seg_trace[pp->instance];
		HFC_DBGPRT("seg info trace flag <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("seg info trace flag = %d \n", pp->fw_parm );

#if 0
	if(pp->fw_parm != 0){
		hfc_fx_write_val( core->fw_init_p->trc_info.trc_seg[0].mode, 0xf8);
	}
#endif
#endif
}


void hfc_fx_set_queue_depth(struct port_info *pp)
{

	pp->queue_depth = HFC_DEFAULT_QUEUE_DEPTH;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc_queue_depth)){ /* in case global parameter is set */
		pp->queue_depth = hfc_queue_depth; 

	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc0_queue_depth) ) pp->queue_depth =  hfc0_queue_depth; break;
		case  1 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc1_queue_depth) ) pp->queue_depth =  hfc1_queue_depth; break;
		case  2 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc2_queue_depth) ) pp->queue_depth =  hfc2_queue_depth; break;
		case  3 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc3_queue_depth) ) pp->queue_depth =  hfc3_queue_depth; break;
		case  4 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc4_queue_depth) ) pp->queue_depth =  hfc4_queue_depth; break;
		case  5 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc5_queue_depth) ) pp->queue_depth =  hfc5_queue_depth; break;
		case  6 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc6_queue_depth) ) pp->queue_depth =  hfc6_queue_depth; break;
		case  7 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc7_queue_depth) ) pp->queue_depth =  hfc7_queue_depth; break;
		case  8 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc8_queue_depth) ) pp->queue_depth =  hfc8_queue_depth; break;
		case  9 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH, hfc9_queue_depth) ) pp->queue_depth =  hfc9_queue_depth; break;
		case 10 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc10_queue_depth) ) pp->queue_depth = hfc10_queue_depth; break;
		case 11 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc11_queue_depth) ) pp->queue_depth = hfc11_queue_depth; break;
		case 12 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc12_queue_depth) ) pp->queue_depth = hfc12_queue_depth; break;
		case 13 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc13_queue_depth) ) pp->queue_depth = hfc13_queue_depth; break;
		case 14 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc14_queue_depth) ) pp->queue_depth = hfc14_queue_depth; break;
		case 15 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc15_queue_depth) ) pp->queue_depth = hfc15_queue_depth; break;
		case 16 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc16_queue_depth) ) pp->queue_depth = hfc16_queue_depth; break;
		case 17 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc17_queue_depth) ) pp->queue_depth = hfc17_queue_depth; break;
		case 18 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc18_queue_depth) ) pp->queue_depth = hfc18_queue_depth; break;
		case 19 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc19_queue_depth) ) pp->queue_depth = hfc19_queue_depth; break;
		case 20 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc20_queue_depth) ) pp->queue_depth = hfc20_queue_depth; break;
		case 21 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc21_queue_depth) ) pp->queue_depth = hfc21_queue_depth; break;
		case 22 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc22_queue_depth) ) pp->queue_depth = hfc22_queue_depth; break;
		case 23 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc23_queue_depth) ) pp->queue_depth = hfc23_queue_depth; break;
		case 24 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc24_queue_depth) ) pp->queue_depth = hfc24_queue_depth; break;
		case 25 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc25_queue_depth) ) pp->queue_depth = hfc25_queue_depth; break;
		case 26 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc26_queue_depth) ) pp->queue_depth = hfc26_queue_depth; break;
		case 27 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc27_queue_depth) ) pp->queue_depth = hfc27_queue_depth; break;
		case 28 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc28_queue_depth) ) pp->queue_depth = hfc28_queue_depth; break;
		case 29 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc29_queue_depth) ) pp->queue_depth = hfc29_queue_depth; break;
		case 30 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc30_queue_depth) ) pp->queue_depth = hfc30_queue_depth; break;
		case 31 : if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfc31_queue_depth) ) pp->queue_depth = hfc31_queue_depth; break;
	}

	if( hfc_chk_conf_val(1,HFC_MAX_QUEUE_DEPTH,hfcmp_queue_depth[pp->instance]) ) {
		pp->queue_depth = hfcmp_queue_depth[pp->instance];
		HFC_DBGPRT("queue depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("queue depth = %d \n", pp->queue_depth );
}


void hfc_fx_set_enable_target_reset(struct port_info *pp)
{
	
	pp->enable_tgtrst = HFC_DEFAULT_TARGET_RESET;
	return;

#if 0
	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_enable_tgtrst)){ /* in case global parameter is set */
		pp->enable_tgtrst = hfc_enable_tgtrst; 
	}

	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,1, hfc0_enable_tgtrst) ) pp->enable_tgtrst =  hfc0_enable_tgtrst; break;
		case  1 : if( hfc_chk_conf_val(0,1, hfc1_enable_tgtrst) ) pp->enable_tgtrst =  hfc1_enable_tgtrst; break;
		case  2 : if( hfc_chk_conf_val(0,1, hfc2_enable_tgtrst) ) pp->enable_tgtrst =  hfc2_enable_tgtrst; break;
		case  3 : if( hfc_chk_conf_val(0,1, hfc3_enable_tgtrst) ) pp->enable_tgtrst =  hfc3_enable_tgtrst; break;
		case  4 : if( hfc_chk_conf_val(0,1, hfc4_enable_tgtrst) ) pp->enable_tgtrst =  hfc4_enable_tgtrst; break;
		case  5 : if( hfc_chk_conf_val(0,1, hfc5_enable_tgtrst) ) pp->enable_tgtrst =  hfc5_enable_tgtrst; break;
		case  6 : if( hfc_chk_conf_val(0,1, hfc6_enable_tgtrst) ) pp->enable_tgtrst =  hfc6_enable_tgtrst; break;
		case  7 : if( hfc_chk_conf_val(0,1, hfc7_enable_tgtrst) ) pp->enable_tgtrst =  hfc7_enable_tgtrst; break;
		case  8 : if( hfc_chk_conf_val(0,1, hfc8_enable_tgtrst) ) pp->enable_tgtrst =  hfc8_enable_tgtrst; break;
		case  9 : if( hfc_chk_conf_val(0,1, hfc9_enable_tgtrst) ) pp->enable_tgtrst =  hfc9_enable_tgtrst; break;
		case 10 : if( hfc_chk_conf_val(0,1,hfc10_enable_tgtrst) ) pp->enable_tgtrst = hfc10_enable_tgtrst; break;
		case 11 : if( hfc_chk_conf_val(0,1,hfc11_enable_tgtrst) ) pp->enable_tgtrst = hfc11_enable_tgtrst; break;
		case 12 : if( hfc_chk_conf_val(0,1,hfc12_enable_tgtrst) ) pp->enable_tgtrst = hfc12_enable_tgtrst; break;
		case 13 : if( hfc_chk_conf_val(0,1,hfc13_enable_tgtrst) ) pp->enable_tgtrst = hfc13_enable_tgtrst; break;
		case 14 : if( hfc_chk_conf_val(0,1,hfc14_enable_tgtrst) ) pp->enable_tgtrst = hfc14_enable_tgtrst; break;
		case 15 : if( hfc_chk_conf_val(0,1,hfc15_enable_tgtrst) ) pp->enable_tgtrst = hfc15_enable_tgtrst; break;
		case 16 : if( hfc_chk_conf_val(0,1,hfc16_enable_tgtrst) ) pp->enable_tgtrst = hfc16_enable_tgtrst; break;
		case 17 : if( hfc_chk_conf_val(0,1,hfc17_enable_tgtrst) ) pp->enable_tgtrst = hfc17_enable_tgtrst; break;
		case 18 : if( hfc_chk_conf_val(0,1,hfc18_enable_tgtrst) ) pp->enable_tgtrst = hfc18_enable_tgtrst; break;
		case 19 : if( hfc_chk_conf_val(0,1,hfc19_enable_tgtrst) ) pp->enable_tgtrst = hfc19_enable_tgtrst; break;
		case 20 : if( hfc_chk_conf_val(0,1,hfc20_enable_tgtrst) ) pp->enable_tgtrst = hfc20_enable_tgtrst; break;
		case 21 : if( hfc_chk_conf_val(0,1,hfc21_enable_tgtrst) ) pp->enable_tgtrst = hfc21_enable_tgtrst; break;
		case 22 : if( hfc_chk_conf_val(0,1,hfc22_enable_tgtrst) ) pp->enable_tgtrst = hfc22_enable_tgtrst; break;
		case 23 : if( hfc_chk_conf_val(0,1,hfc23_enable_tgtrst) ) pp->enable_tgtrst = hfc23_enable_tgtrst; break;
		case 24 : if( hfc_chk_conf_val(0,1,hfc24_enable_tgtrst) ) pp->enable_tgtrst = hfc24_enable_tgtrst; break;
		case 25 : if( hfc_chk_conf_val(0,1,hfc25_enable_tgtrst) ) pp->enable_tgtrst = hfc25_enable_tgtrst; break;
		case 26 : if( hfc_chk_conf_val(0,1,hfc26_enable_tgtrst) ) pp->enable_tgtrst = hfc26_enable_tgtrst; break;
		case 27 : if( hfc_chk_conf_val(0,1,hfc27_enable_tgtrst) ) pp->enable_tgtrst = hfc27_enable_tgtrst; break;
		case 28 : if( hfc_chk_conf_val(0,1,hfc28_enable_tgtrst) ) pp->enable_tgtrst = hfc28_enable_tgtrst; break;
		case 29 : if( hfc_chk_conf_val(0,1,hfc29_enable_tgtrst) ) pp->enable_tgtrst = hfc29_enable_tgtrst; break;
		case 30 : if( hfc_chk_conf_val(0,1,hfc30_enable_tgtrst) ) pp->enable_tgtrst = hfc30_enable_tgtrst; break;
		case 31 : if( hfc_chk_conf_val(0,1,hfc31_enable_tgtrst) ) pp->enable_tgtrst = hfc31_enable_tgtrst; break;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_enable_tgtrst[pp->instance]) ) {
		pp->enable_tgtrst = hfcmp_enable_tgtrst[pp->instance];
		HFC_DBGPRT("enable target reset <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("enable target reset = %d \n", pp->enable_tgtrst );
#endif

}


void hfc_fx_set_max_target(struct port_info *pp)
{

	pp->max_target = MAX_TARGET_PROBE;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc_max_target)){ /* in case global parameter is set */
		pp->max_target = hfc_max_target; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc0_max_target) ) pp->max_target =  hfc0_max_target; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc1_max_target) ) pp->max_target =  hfc1_max_target; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc2_max_target) ) pp->max_target =  hfc2_max_target; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc3_max_target) ) pp->max_target =  hfc3_max_target; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc4_max_target) ) pp->max_target =  hfc4_max_target; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc5_max_target) ) pp->max_target =  hfc5_max_target; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc6_max_target) ) pp->max_target =  hfc6_max_target; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc7_max_target) ) pp->max_target =  hfc7_max_target; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc8_max_target) ) pp->max_target =  hfc8_max_target; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE, hfc9_max_target) ) pp->max_target =  hfc9_max_target; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc10_max_target) ) pp->max_target = hfc10_max_target; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc11_max_target) ) pp->max_target = hfc11_max_target; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc12_max_target) ) pp->max_target = hfc12_max_target; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc13_max_target) ) pp->max_target = hfc13_max_target; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc14_max_target) ) pp->max_target = hfc14_max_target; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc15_max_target) ) pp->max_target = hfc15_max_target; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc16_max_target) ) pp->max_target = hfc16_max_target; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc17_max_target) ) pp->max_target = hfc17_max_target; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc18_max_target) ) pp->max_target = hfc18_max_target; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc19_max_target) ) pp->max_target = hfc19_max_target; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc20_max_target) ) pp->max_target = hfc20_max_target; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc21_max_target) ) pp->max_target = hfc21_max_target; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc22_max_target) ) pp->max_target = hfc22_max_target; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc23_max_target) ) pp->max_target = hfc23_max_target; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc24_max_target) ) pp->max_target = hfc24_max_target; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc25_max_target) ) pp->max_target = hfc25_max_target; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc26_max_target) ) pp->max_target = hfc26_max_target; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc27_max_target) ) pp->max_target = hfc27_max_target; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc28_max_target) ) pp->max_target = hfc28_max_target; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc29_max_target) ) pp->max_target = hfc29_max_target; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc30_max_target) ) pp->max_target = hfc30_max_target; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfc31_max_target) ) pp->max_target = hfc31_max_target; break;
	}

	if( hfc_chk_conf_val(0,MAX_TARGET_PROBE,hfcmp_max_target[pp->instance]) ) {
		pp->max_target =  hfcmp_max_target[pp->instance];
		HFC_DBGPRT("max target number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max target number = %d \n", pp->max_target );

}



void hfc_fx_set_xob_max(struct port_info *pp)
{
	
	pp->xob_max = MAX_XOB_NUM;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,MAX_XOB_NUM,hfc_xob_max)){ /* in case global parameter is set */
		pp->xob_max = hfc_xob_max; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc0_xob_max) ) pp->xob_max =  hfc0_xob_max; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc1_xob_max) ) pp->xob_max =  hfc1_xob_max; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc2_xob_max) ) pp->xob_max =  hfc2_xob_max; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc3_xob_max) ) pp->xob_max =  hfc3_xob_max; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc4_xob_max) ) pp->xob_max =  hfc4_xob_max; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc5_xob_max) ) pp->xob_max =  hfc5_xob_max; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc6_xob_max) ) pp->xob_max =  hfc6_xob_max; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc7_xob_max) ) pp->xob_max =  hfc7_xob_max; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc8_xob_max) ) pp->xob_max =  hfc8_xob_max; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_XOB_NUM, hfc9_xob_max) ) pp->xob_max =  hfc9_xob_max; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc10_xob_max) ) pp->xob_max = hfc10_xob_max; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc11_xob_max) ) pp->xob_max = hfc11_xob_max; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc12_xob_max) ) pp->xob_max = hfc12_xob_max; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc13_xob_max) ) pp->xob_max = hfc13_xob_max; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc14_xob_max) ) pp->xob_max = hfc14_xob_max; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc15_xob_max) ) pp->xob_max = hfc15_xob_max; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc16_xob_max) ) pp->xob_max = hfc16_xob_max; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc17_xob_max) ) pp->xob_max = hfc17_xob_max; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc18_xob_max) ) pp->xob_max = hfc18_xob_max; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc19_xob_max) ) pp->xob_max = hfc19_xob_max; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc20_xob_max) ) pp->xob_max = hfc20_xob_max; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc21_xob_max) ) pp->xob_max = hfc21_xob_max; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc22_xob_max) ) pp->xob_max = hfc22_xob_max; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc23_xob_max) ) pp->xob_max = hfc23_xob_max; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc24_xob_max) ) pp->xob_max = hfc24_xob_max; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc25_xob_max) ) pp->xob_max = hfc25_xob_max; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc26_xob_max) ) pp->xob_max = hfc26_xob_max; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc27_xob_max) ) pp->xob_max = hfc27_xob_max; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc28_xob_max) ) pp->xob_max = hfc28_xob_max; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc29_xob_max) ) pp->xob_max = hfc29_xob_max; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc30_xob_max) ) pp->xob_max = hfc30_xob_max; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfc31_xob_max) ) pp->xob_max = hfc31_xob_max; break;
	}

	if( hfc_chk_conf_val(0,MAX_XOB_NUM,hfcmp_xob_max[pp->instance]) ) {
		pp->xob_max = hfcmp_xob_max[pp->instance];
		HFC_DBGPRT("max xob number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max xob number = %d \n", pp->xob_max );

}


void hfc_fx_set_xrb_max(struct port_info *pp)
{

	pp->xrb_max = MAX_XRB_NUM;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,MAX_XRB_NUM,hfc_xrb_max)){ /* in case global parameter is set */
		pp->xrb_max = hfc_xrb_max; 

	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc0_xrb_max) ) pp->xrb_max =  hfc0_xrb_max; break;
		case  1 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc1_xrb_max) ) pp->xrb_max =  hfc1_xrb_max; break;
		case  2 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc2_xrb_max) ) pp->xrb_max =  hfc2_xrb_max; break;
		case  3 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc3_xrb_max) ) pp->xrb_max =  hfc3_xrb_max; break;
		case  4 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc4_xrb_max) ) pp->xrb_max =  hfc4_xrb_max; break;
		case  5 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc5_xrb_max) ) pp->xrb_max =  hfc5_xrb_max; break;
		case  6 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc6_xrb_max) ) pp->xrb_max =  hfc6_xrb_max; break;
		case  7 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc7_xrb_max) ) pp->xrb_max =  hfc7_xrb_max; break;
		case  8 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc8_xrb_max) ) pp->xrb_max =  hfc8_xrb_max; break;
		case  9 : if( hfc_chk_conf_val(0,MAX_XRB_NUM, hfc9_xrb_max) ) pp->xrb_max =  hfc9_xrb_max; break;
		case 10 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc10_xrb_max) ) pp->xrb_max = hfc10_xrb_max; break;
		case 11 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc11_xrb_max) ) pp->xrb_max = hfc11_xrb_max; break;
		case 12 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc12_xrb_max) ) pp->xrb_max = hfc12_xrb_max; break;
		case 13 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc13_xrb_max) ) pp->xrb_max = hfc13_xrb_max; break;
		case 14 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc14_xrb_max) ) pp->xrb_max = hfc14_xrb_max; break;
		case 15 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc15_xrb_max) ) pp->xrb_max = hfc15_xrb_max; break;
		case 16 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc16_xrb_max) ) pp->xrb_max = hfc16_xrb_max; break;
		case 17 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc17_xrb_max) ) pp->xrb_max = hfc17_xrb_max; break;
		case 18 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc18_xrb_max) ) pp->xrb_max = hfc18_xrb_max; break;
		case 19 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc19_xrb_max) ) pp->xrb_max = hfc19_xrb_max; break;
		case 20 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc20_xrb_max) ) pp->xrb_max = hfc20_xrb_max; break;
		case 21 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc21_xrb_max) ) pp->xrb_max = hfc21_xrb_max; break;
		case 22 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc22_xrb_max) ) pp->xrb_max = hfc22_xrb_max; break;
		case 23 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc23_xrb_max) ) pp->xrb_max = hfc23_xrb_max; break;
		case 24 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc24_xrb_max) ) pp->xrb_max = hfc24_xrb_max; break;
		case 25 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc25_xrb_max) ) pp->xrb_max = hfc25_xrb_max; break;
		case 26 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc26_xrb_max) ) pp->xrb_max = hfc26_xrb_max; break;
		case 27 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc27_xrb_max) ) pp->xrb_max = hfc27_xrb_max; break;
		case 28 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc28_xrb_max) ) pp->xrb_max = hfc28_xrb_max; break;
		case 29 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc29_xrb_max) ) pp->xrb_max = hfc29_xrb_max; break;
		case 30 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc30_xrb_max) ) pp->xrb_max = hfc30_xrb_max; break;
		case 31 : if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfc31_xrb_max) ) pp->xrb_max = hfc31_xrb_max; break;
	}

	if( hfc_chk_conf_val(0,MAX_XRB_NUM,hfcmp_xrb_max[pp->instance]) ) {
		pp->xrb_max = hfcmp_xrb_max[pp->instance];
		HFC_DBGPRT("max xrb number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max xrb number = %d \n", pp->xrb_max );

}



void hfc_fx_set_slog_max(struct port_info *pp)
{
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		pp->slog_max = HFC_SOFT_LOG_PAGE;
	}
	else {
		pp->slog_max = HFC_VPORT_SOFT_LOG_PAGE;
	}

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc_slog_max)){ /* in case global parameter is set */
		pp->slog_max = hfc_slog_max; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc0_slog_max) ) pp->slog_max =  hfc0_slog_max; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc1_slog_max) ) pp->slog_max =  hfc1_slog_max; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc2_slog_max) ) pp->slog_max =  hfc2_slog_max; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc3_slog_max) ) pp->slog_max =  hfc3_slog_max; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc4_slog_max) ) pp->slog_max =  hfc4_slog_max; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc5_slog_max) ) pp->slog_max =  hfc5_slog_max; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc6_slog_max) ) pp->slog_max =  hfc6_slog_max; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc7_slog_max) ) pp->slog_max =  hfc7_slog_max; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc8_slog_max) ) pp->slog_max =  hfc8_slog_max; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE, hfc9_slog_max) ) pp->slog_max =  hfc9_slog_max; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc10_slog_max) ) pp->slog_max = hfc10_slog_max; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc11_slog_max) ) pp->slog_max = hfc11_slog_max; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc12_slog_max) ) pp->slog_max = hfc12_slog_max; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc13_slog_max) ) pp->slog_max = hfc13_slog_max; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc14_slog_max) ) pp->slog_max = hfc14_slog_max; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc15_slog_max) ) pp->slog_max = hfc15_slog_max; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc16_slog_max) ) pp->slog_max = hfc16_slog_max; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc17_slog_max) ) pp->slog_max = hfc17_slog_max; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc18_slog_max) ) pp->slog_max = hfc18_slog_max; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc19_slog_max) ) pp->slog_max = hfc19_slog_max; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc20_slog_max) ) pp->slog_max = hfc20_slog_max; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc21_slog_max) ) pp->slog_max = hfc21_slog_max; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc22_slog_max) ) pp->slog_max = hfc22_slog_max; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc23_slog_max) ) pp->slog_max = hfc23_slog_max; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc24_slog_max) ) pp->slog_max = hfc24_slog_max; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc25_slog_max) ) pp->slog_max = hfc25_slog_max; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc26_slog_max) ) pp->slog_max = hfc26_slog_max; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc27_slog_max) ) pp->slog_max = hfc27_slog_max; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc28_slog_max) ) pp->slog_max = hfc28_slog_max; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc29_slog_max) ) pp->slog_max = hfc29_slog_max; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc30_slog_max) ) pp->slog_max = hfc30_slog_max; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfc31_slog_max) ) pp->slog_max = hfc31_slog_max; break;
	}

	if( hfc_chk_conf_val(0,HFC_SOFT_LOG_PAGE,hfcmp_slog_max[pp->instance]) ) {
		pp->slog_max =  hfcmp_slog_max[pp->instance];
		HFC_DBGPRT("slog max <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("slog max = %d \n", pp->slog_max );
}



void hfc_fx_set_trc_max(struct port_info *pp)
{
	
	pp->trc_max = HFC_MAX_TRCCNT;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc_trc_max)){ /* in case global parameter is set */
		pp->trc_max = hfc_trc_max; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc0_trc_max) ) pp->trc_max =  hfc0_trc_max; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc1_trc_max) ) pp->trc_max =  hfc1_trc_max; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc2_trc_max) ) pp->trc_max =  hfc2_trc_max; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc3_trc_max) ) pp->trc_max =  hfc3_trc_max; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc4_trc_max) ) pp->trc_max =  hfc4_trc_max; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc5_trc_max) ) pp->trc_max =  hfc5_trc_max; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc6_trc_max) ) pp->trc_max =  hfc6_trc_max; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc7_trc_max) ) pp->trc_max =  hfc7_trc_max; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc8_trc_max) ) pp->trc_max =  hfc8_trc_max; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT, hfc9_trc_max) ) pp->trc_max =  hfc9_trc_max; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc10_trc_max) ) pp->trc_max = hfc10_trc_max; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc11_trc_max) ) pp->trc_max = hfc11_trc_max; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc12_trc_max) ) pp->trc_max = hfc12_trc_max; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc13_trc_max) ) pp->trc_max = hfc13_trc_max; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc14_trc_max) ) pp->trc_max = hfc14_trc_max; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc15_trc_max) ) pp->trc_max = hfc15_trc_max; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc16_trc_max) ) pp->trc_max = hfc16_trc_max; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc17_trc_max) ) pp->trc_max = hfc17_trc_max; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc18_trc_max) ) pp->trc_max = hfc18_trc_max; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc19_trc_max) ) pp->trc_max = hfc19_trc_max; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc20_trc_max) ) pp->trc_max = hfc20_trc_max; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc21_trc_max) ) pp->trc_max = hfc21_trc_max; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc22_trc_max) ) pp->trc_max = hfc22_trc_max; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc23_trc_max) ) pp->trc_max = hfc23_trc_max; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc24_trc_max) ) pp->trc_max = hfc24_trc_max; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc25_trc_max) ) pp->trc_max = hfc25_trc_max; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc26_trc_max) ) pp->trc_max = hfc26_trc_max; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc27_trc_max) ) pp->trc_max = hfc27_trc_max; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc28_trc_max) ) pp->trc_max = hfc28_trc_max; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc29_trc_max) ) pp->trc_max = hfc29_trc_max; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc30_trc_max) ) pp->trc_max = hfc30_trc_max; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfc31_trc_max) ) pp->trc_max = hfc31_trc_max; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_TRCCNT,hfcmp_trc_max[pp->instance]) ) {
		pp->trc_max =  hfcmp_trc_max[pp->instance];
		HFC_DBGPRT("trace max <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("trace max = %d \n", pp->trc_max );
}



void hfc_fx_set_pkt_num(struct port_info *pp)
{
	if (!pp->core_num) {
		pp->pre_pkt_num = 0;
		return;
	}
	
	pp->pre_pkt_num = 4*pp->core_num;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc_pkt_num)){ /* in case global parameter is set */
		pp->pre_pkt_num = hfc_pkt_num*pp->core_num; 
	}

	if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfcmp_pkt_num[pp->instance]) ) {
		pp->pre_pkt_num = hfcmp_pkt_num[pp->instance]*pp->core_num; 
		HFC_DBGPRT("hfc_pre_pkt_number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("hfc_pkt_number = %d \n", pp->pre_pkt_num );
}



void hfc_fx_set_rsv_pkt_num(struct port_info *pp)
{
	if (!pp->core_num) {
		pp->rsv_pkt_num = 0;
		return;
	}
	
	pp->rsv_pkt_num = 4*pp->core_num;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc_rsv_pkt_num)){ /* in case global parameter is set */
		pp->rsv_pkt_num = hfc_rsv_pkt_num*pp->core_num; 
	}

	if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfcmp_rsv_pkt_num[pp->instance]) ) {
		pp->rsv_pkt_num = hfcmp_rsv_pkt_num[pp->instance]*pp->core_num; 
		HFC_DBGPRT("hfc_rsv_pkt_number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("hfc_rsv_pkt_number = %d \n", pp->pre_pkt_num );
}



void hfc_fx_set_pm_pkt_num(struct port_info *pp)
{
	if (!pp->core_num) {
		pp->pm_pkt_num = 0;
		return;
	}
	
	pp->pm_pkt_num = HFC_DEFAULT_CAN_QUEUE*pp->core_num;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc_pm_pkt_num)){ /* in case global parameter is set */
		pp->pm_pkt_num = hfc_pm_pkt_num*pp->core_num; 
	}

	if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfcmp_pm_pkt_num[pp->instance]) ) {
		pp->pm_pkt_num = hfcmp_pm_pkt_num[pp->instance]*pp->core_num; 
		HFC_DBGPRT("hfc_pm_pkt_number <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("hfc_pm_pkt_number = %d \n", pp->pm_pkt_num );
}


void hfc_fx_set_can_queue(struct port_info *pp)
{
	
	pp->can_queue = HFC_DEFAULT_CAN_QUEUE*pp->core_num;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfc_can_queue)){ /* in case global parameter is set */
		pp->can_queue = hfc_can_queue*pp->core_num; 
	}

	if( hfc_chk_conf_val(1,HFC_DEFAULT_CAN_QUEUE,hfcmp_can_queue[pp->instance]) ) {
		pp->can_queue =  hfcmp_can_queue[pp->instance]*pp->core_num;
		HFC_DBGPRT("max can queue <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max can_queue = %d \n", pp->can_queue );
}



void hfc_fx_set_sg_tblsize(struct port_info *pp)
{
	
	pp->sg_tblsize = HFC_SG_TABLESIZE;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc_sg_tblsize)){ /* in case global parameter is set */
		pp->sg_tblsize = hfc_sg_tblsize; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc0_sg_tblsize) ) pp->sg_tblsize =  hfc0_sg_tblsize; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc1_sg_tblsize) ) pp->sg_tblsize =  hfc1_sg_tblsize; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc2_sg_tblsize) ) pp->sg_tblsize =  hfc2_sg_tblsize; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc3_sg_tblsize) ) pp->sg_tblsize =  hfc3_sg_tblsize; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc4_sg_tblsize) ) pp->sg_tblsize =  hfc4_sg_tblsize; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc5_sg_tblsize) ) pp->sg_tblsize =  hfc5_sg_tblsize; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc6_sg_tblsize) ) pp->sg_tblsize =  hfc6_sg_tblsize; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc7_sg_tblsize) ) pp->sg_tblsize =  hfc7_sg_tblsize; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc8_sg_tblsize) ) pp->sg_tblsize =  hfc8_sg_tblsize; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE, hfc9_sg_tblsize) ) pp->sg_tblsize =  hfc9_sg_tblsize; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc10_sg_tblsize) ) pp->sg_tblsize = hfc10_sg_tblsize; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc11_sg_tblsize) ) pp->sg_tblsize = hfc11_sg_tblsize; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc12_sg_tblsize) ) pp->sg_tblsize = hfc12_sg_tblsize; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc13_sg_tblsize) ) pp->sg_tblsize = hfc13_sg_tblsize; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc14_sg_tblsize) ) pp->sg_tblsize = hfc14_sg_tblsize; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc15_sg_tblsize) ) pp->sg_tblsize = hfc15_sg_tblsize; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc16_sg_tblsize) ) pp->sg_tblsize = hfc16_sg_tblsize; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc17_sg_tblsize) ) pp->sg_tblsize = hfc17_sg_tblsize; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc18_sg_tblsize) ) pp->sg_tblsize = hfc18_sg_tblsize; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc19_sg_tblsize) ) pp->sg_tblsize = hfc19_sg_tblsize; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc20_sg_tblsize) ) pp->sg_tblsize = hfc20_sg_tblsize; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc21_sg_tblsize) ) pp->sg_tblsize = hfc21_sg_tblsize; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc22_sg_tblsize) ) pp->sg_tblsize = hfc22_sg_tblsize; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc23_sg_tblsize) ) pp->sg_tblsize = hfc23_sg_tblsize; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc24_sg_tblsize) ) pp->sg_tblsize = hfc24_sg_tblsize; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc25_sg_tblsize) ) pp->sg_tblsize = hfc25_sg_tblsize; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc26_sg_tblsize) ) pp->sg_tblsize = hfc26_sg_tblsize; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc27_sg_tblsize) ) pp->sg_tblsize = hfc27_sg_tblsize; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc28_sg_tblsize) ) pp->sg_tblsize = hfc28_sg_tblsize; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc29_sg_tblsize) ) pp->sg_tblsize = hfc29_sg_tblsize; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc30_sg_tblsize) ) pp->sg_tblsize = hfc30_sg_tblsize; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfc31_sg_tblsize) ) pp->sg_tblsize = hfc31_sg_tblsize; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_SG_TABLESIZE,hfcmp_sg_tblsize[pp->instance]) ) {
		pp->sg_tblsize =  hfcmp_sg_tblsize[pp->instance];
		HFC_DBGPRT("sg tablesize <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("sg tablesize = %d \n", pp->sg_tblsize );
}


void hfc_fx_set_cmnd_num(struct port_info *pp)
{
	
	pp->cmnd_num = HFC_CMND_NUM;
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_CMND_NUM,hfc_cmnd_num)){ /* in case global parameter is set */
		pp->cmnd_num = hfc_cmnd_num; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc0_cmnd_num) ) pp->cmnd_num =  hfc0_cmnd_num; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc1_cmnd_num) ) pp->cmnd_num =  hfc1_cmnd_num; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc2_cmnd_num) ) pp->cmnd_num =  hfc2_cmnd_num; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc3_cmnd_num) ) pp->cmnd_num =  hfc3_cmnd_num; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc4_cmnd_num) ) pp->cmnd_num =  hfc4_cmnd_num; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc5_cmnd_num) ) pp->cmnd_num =  hfc5_cmnd_num; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc6_cmnd_num) ) pp->cmnd_num =  hfc6_cmnd_num; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc7_cmnd_num) ) pp->cmnd_num =  hfc7_cmnd_num; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc8_cmnd_num) ) pp->cmnd_num =  hfc8_cmnd_num; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_CMND_NUM, hfc9_cmnd_num) ) pp->cmnd_num =  hfc9_cmnd_num; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc10_cmnd_num) ) pp->cmnd_num = hfc10_cmnd_num; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc11_cmnd_num) ) pp->cmnd_num = hfc11_cmnd_num; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc12_cmnd_num) ) pp->cmnd_num = hfc12_cmnd_num; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc13_cmnd_num) ) pp->cmnd_num = hfc13_cmnd_num; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc14_cmnd_num) ) pp->cmnd_num = hfc14_cmnd_num; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc15_cmnd_num) ) pp->cmnd_num = hfc15_cmnd_num; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc16_cmnd_num) ) pp->cmnd_num = hfc16_cmnd_num; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc17_cmnd_num) ) pp->cmnd_num = hfc17_cmnd_num; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc18_cmnd_num) ) pp->cmnd_num = hfc18_cmnd_num; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc19_cmnd_num) ) pp->cmnd_num = hfc19_cmnd_num; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc20_cmnd_num) ) pp->cmnd_num = hfc10_cmnd_num; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc21_cmnd_num) ) pp->cmnd_num = hfc21_cmnd_num; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc22_cmnd_num) ) pp->cmnd_num = hfc22_cmnd_num; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc23_cmnd_num) ) pp->cmnd_num = hfc23_cmnd_num; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc24_cmnd_num) ) pp->cmnd_num = hfc24_cmnd_num; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc25_cmnd_num) ) pp->cmnd_num = hfc25_cmnd_num; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc26_cmnd_num) ) pp->cmnd_num = hfc26_cmnd_num; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc27_cmnd_num) ) pp->cmnd_num = hfc27_cmnd_num; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc28_cmnd_num) ) pp->cmnd_num = hfc28_cmnd_num; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc29_cmnd_num) ) pp->cmnd_num = hfc29_cmnd_num; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc30_cmnd_num) ) pp->cmnd_num = hfc30_cmnd_num; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfc31_cmnd_num) ) pp->cmnd_num = hfc31_cmnd_num; break;
	}

	if( hfc_chk_conf_val(0,HFC_CMND_NUM,hfcmp_cmnd_num[pp->instance]) ) {
		pp->cmnd_num =  hfcmp_cmnd_num[pp->instance];
		HFC_DBGPRT("cmnd num <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("cmnd num = %d \n", pp->cmnd_num );
}


/*
 * Function:    hfc_fx_set_minus_tout
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_set_minus_tout(struct port_info *pp)
{
	
	pp->minus_tout = HFC_MINUS_TIMOUT;
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc_minus_tout)){ /* in case global parameter is set */
		pp->minus_tout = hfc_minus_tout; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc0_minus_tout) ) pp->minus_tout =  hfc0_minus_tout; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc1_minus_tout) ) pp->minus_tout =  hfc1_minus_tout; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc2_minus_tout) ) pp->minus_tout =  hfc2_minus_tout; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc3_minus_tout) ) pp->minus_tout =  hfc3_minus_tout; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc4_minus_tout) ) pp->minus_tout =  hfc4_minus_tout; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc5_minus_tout) ) pp->minus_tout =  hfc5_minus_tout; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc6_minus_tout) ) pp->minus_tout =  hfc6_minus_tout; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc7_minus_tout) ) pp->minus_tout =  hfc7_minus_tout; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc8_minus_tout) ) pp->minus_tout =  hfc8_minus_tout; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT, hfc9_minus_tout) ) pp->minus_tout =  hfc9_minus_tout; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc10_minus_tout) ) pp->minus_tout = hfc10_minus_tout; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc11_minus_tout) ) pp->minus_tout = hfc11_minus_tout; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc12_minus_tout) ) pp->minus_tout = hfc12_minus_tout; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc13_minus_tout) ) pp->minus_tout = hfc13_minus_tout; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc14_minus_tout) ) pp->minus_tout = hfc14_minus_tout; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc15_minus_tout) ) pp->minus_tout = hfc15_minus_tout; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc16_minus_tout) ) pp->minus_tout = hfc16_minus_tout; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc17_minus_tout) ) pp->minus_tout = hfc17_minus_tout; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc18_minus_tout) ) pp->minus_tout = hfc18_minus_tout; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc19_minus_tout) ) pp->minus_tout = hfc19_minus_tout; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc20_minus_tout) ) pp->minus_tout = hfc20_minus_tout; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc21_minus_tout) ) pp->minus_tout = hfc21_minus_tout; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc22_minus_tout) ) pp->minus_tout = hfc22_minus_tout; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc23_minus_tout) ) pp->minus_tout = hfc23_minus_tout; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc24_minus_tout) ) pp->minus_tout = hfc24_minus_tout; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc25_minus_tout) ) pp->minus_tout = hfc25_minus_tout; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc26_minus_tout) ) pp->minus_tout = hfc26_minus_tout; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc27_minus_tout) ) pp->minus_tout = hfc27_minus_tout; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc28_minus_tout) ) pp->minus_tout = hfc28_minus_tout; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc29_minus_tout) ) pp->minus_tout = hfc29_minus_tout; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc30_minus_tout) ) pp->minus_tout = hfc30_minus_tout; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfc31_minus_tout) ) pp->minus_tout = hfc31_minus_tout; break;
	}

	if( hfc_chk_conf_val(0,HFC_MINUS_TIMOUT,hfcmp_minus_tout[pp->instance]) ) {
		pp->minus_tout =  hfcmp_minus_tout[pp->instance];
		HFC_DBGPRT("minus timeout <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("minus timeout = %d \n", pp->minus_tout );
	
}


void hfc_fx_set_scsi_allowed(struct port_info *pp)
{
	
	pp->scsi_allowed = HFC_SCSI_ALLOWED;
	
	if(pp->defparam) return;
	
	if(hfc_chk_conf_val(0,30,hfc_scsi_allowed)){ /* in case global parameter is set */
		pp->scsi_allowed = hfc_scsi_allowed; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,30, hfc0_scsi_allowed) ) pp->scsi_allowed =  hfc0_scsi_allowed; break;
		case  1 : if( hfc_chk_conf_val(0,30, hfc1_scsi_allowed) ) pp->scsi_allowed =  hfc1_scsi_allowed; break;
		case  2 : if( hfc_chk_conf_val(0,30, hfc2_scsi_allowed) ) pp->scsi_allowed =  hfc2_scsi_allowed; break;
		case  3 : if( hfc_chk_conf_val(0,30, hfc3_scsi_allowed) ) pp->scsi_allowed =  hfc3_scsi_allowed; break;
		case  4 : if( hfc_chk_conf_val(0,30, hfc4_scsi_allowed) ) pp->scsi_allowed =  hfc4_scsi_allowed; break;
		case  5 : if( hfc_chk_conf_val(0,30, hfc5_scsi_allowed) ) pp->scsi_allowed =  hfc5_scsi_allowed; break;
		case  6 : if( hfc_chk_conf_val(0,30, hfc6_scsi_allowed) ) pp->scsi_allowed =  hfc6_scsi_allowed; break;
		case  7 : if( hfc_chk_conf_val(0,30, hfc7_scsi_allowed) ) pp->scsi_allowed =  hfc7_scsi_allowed; break;
		case  8 : if( hfc_chk_conf_val(0,30, hfc8_scsi_allowed) ) pp->scsi_allowed =  hfc8_scsi_allowed; break;
		case  9 : if( hfc_chk_conf_val(0,30, hfc9_scsi_allowed) ) pp->scsi_allowed =  hfc9_scsi_allowed; break;
		case 10 : if( hfc_chk_conf_val(0,30,hfc10_scsi_allowed) ) pp->scsi_allowed = hfc10_scsi_allowed; break;
		case 11 : if( hfc_chk_conf_val(0,30,hfc11_scsi_allowed) ) pp->scsi_allowed = hfc11_scsi_allowed; break;
		case 12 : if( hfc_chk_conf_val(0,30,hfc12_scsi_allowed) ) pp->scsi_allowed = hfc12_scsi_allowed; break;
		case 13 : if( hfc_chk_conf_val(0,30,hfc13_scsi_allowed) ) pp->scsi_allowed = hfc13_scsi_allowed; break;
		case 14 : if( hfc_chk_conf_val(0,30,hfc14_scsi_allowed) ) pp->scsi_allowed = hfc14_scsi_allowed; break;
		case 15 : if( hfc_chk_conf_val(0,30,hfc15_scsi_allowed) ) pp->scsi_allowed = hfc15_scsi_allowed; break;
		case 16 : if( hfc_chk_conf_val(0,30,hfc16_scsi_allowed) ) pp->scsi_allowed = hfc16_scsi_allowed; break;
		case 17 : if( hfc_chk_conf_val(0,30,hfc17_scsi_allowed) ) pp->scsi_allowed = hfc17_scsi_allowed; break;
		case 18 : if( hfc_chk_conf_val(0,30,hfc18_scsi_allowed) ) pp->scsi_allowed = hfc18_scsi_allowed; break;
		case 19 : if( hfc_chk_conf_val(0,30,hfc19_scsi_allowed) ) pp->scsi_allowed = hfc19_scsi_allowed; break;
		case 20 : if( hfc_chk_conf_val(0,30,hfc20_scsi_allowed) ) pp->scsi_allowed = hfc20_scsi_allowed; break;
		case 21 : if( hfc_chk_conf_val(0,30,hfc21_scsi_allowed) ) pp->scsi_allowed = hfc21_scsi_allowed; break;
		case 22 : if( hfc_chk_conf_val(0,30,hfc22_scsi_allowed) ) pp->scsi_allowed = hfc22_scsi_allowed; break;
		case 23 : if( hfc_chk_conf_val(0,30,hfc23_scsi_allowed) ) pp->scsi_allowed = hfc23_scsi_allowed; break;
		case 24 : if( hfc_chk_conf_val(0,30,hfc24_scsi_allowed) ) pp->scsi_allowed = hfc24_scsi_allowed; break;
		case 25 : if( hfc_chk_conf_val(0,30,hfc25_scsi_allowed) ) pp->scsi_allowed = hfc25_scsi_allowed; break;
		case 26 : if( hfc_chk_conf_val(0,30,hfc26_scsi_allowed) ) pp->scsi_allowed = hfc26_scsi_allowed; break;
		case 27 : if( hfc_chk_conf_val(0,30,hfc27_scsi_allowed) ) pp->scsi_allowed = hfc27_scsi_allowed; break;
		case 28 : if( hfc_chk_conf_val(0,30,hfc28_scsi_allowed) ) pp->scsi_allowed = hfc28_scsi_allowed; break;
		case 29 : if( hfc_chk_conf_val(0,30,hfc29_scsi_allowed) ) pp->scsi_allowed = hfc29_scsi_allowed; break;
		case 30 : if( hfc_chk_conf_val(0,30,hfc30_scsi_allowed) ) pp->scsi_allowed = hfc30_scsi_allowed; break;
		case 31 : if( hfc_chk_conf_val(0,30,hfc31_scsi_allowed) ) pp->scsi_allowed = hfc31_scsi_allowed; break;
	}

	if( hfc_chk_conf_val(0,30,hfcmp_scsi_allowed[pp->instance]) ) {
		pp->scsi_allowed =  hfcmp_scsi_allowed[pp->instance];
		HFC_DBGPRT("scsi_allowed <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("scsi_allowed = %d \n", pp->scsi_allowed );
}

/* FCLNX-GPL-0343 */
void hfc_fx_set_login_retry(struct port_info *pp)
{
#if 0
	pp->login_retry = HFC_LOGIN_RETRY; /* default = 3 */
	
	if(pp->defparam) return;
	
	if( hfc_chk_conf_val(0,10, hfc_login_retry)){ /* in case global parameter is set */
		pp->login_retry = hfc_login_retry; 
		
	}

	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_login_retry) ) pp->login_retry =  hfc0_login_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_login_retry) ) pp->login_retry =  hfc1_login_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_login_retry) ) pp->login_retry =  hfc2_login_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_login_retry) ) pp->login_retry =  hfc3_login_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_login_retry) ) pp->login_retry =  hfc4_login_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_login_retry) ) pp->login_retry =  hfc5_login_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_login_retry) ) pp->login_retry =  hfc6_login_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_login_retry) ) pp->login_retry =  hfc7_login_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_login_retry) ) pp->login_retry =  hfc8_login_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_login_retry) ) pp->login_retry =  hfc9_login_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_login_retry) ) pp->login_retry = hfc10_login_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_login_retry) ) pp->login_retry = hfc11_login_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_login_retry) ) pp->login_retry = hfc12_login_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_login_retry) ) pp->login_retry = hfc13_login_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_login_retry) ) pp->login_retry = hfc14_login_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_login_retry) ) pp->login_retry = hfc15_login_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_login_retry) ) pp->login_retry = hfc16_login_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_login_retry) ) pp->login_retry = hfc17_login_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_login_retry) ) pp->login_retry = hfc18_login_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_login_retry) ) pp->login_retry = hfc19_login_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_login_retry) ) pp->login_retry = hfc20_login_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_login_retry) ) pp->login_retry = hfc21_login_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_login_retry) ) pp->login_retry = hfc22_login_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_login_retry) ) pp->login_retry = hfc23_login_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_login_retry) ) pp->login_retry = hfc24_login_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_login_retry) ) pp->login_retry = hfc25_login_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_login_retry) ) pp->login_retry = hfc26_login_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_login_retry) ) pp->login_retry = hfc27_login_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_login_retry) ) pp->login_retry = hfc28_login_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_login_retry) ) pp->login_retry = hfc29_login_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_login_retry) ) pp->login_retry = hfc30_login_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_login_retry) ) pp->login_retry = hfc31_login_retry; break;
	}

	if( hfc_chk_conf_val(0,10,hfcmp_login_retry[pp->instance]) ) {
		pp->login_retry = hfcmp_login_retry[pp->instance];
		HFC_DBGPRT("max login retry count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max login retry count = %d \n", pp->login_retry);
#endif
}


/* FCLNX-GPL-0343 */
void hfc_fx_set_els_retry(struct port_info *pp)
{
#if 0
	pp->els_retry = HFC_ELS_RETRY; /* default = 3 */
	
	if(pp->defparam) return;
	
	if( hfc_chk_conf_val(0,10, hfc_els_retry)){ /* in case global parameter is set */
		pp->els_retry = hfc_els_retry; 
		
	}

	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,10, hfc0_els_retry) ) pp->els_retry =  hfc0_els_retry; break;
		case  1 : if( hfc_chk_conf_val(0,10, hfc1_els_retry) ) pp->els_retry =  hfc1_els_retry; break;
		case  2 : if( hfc_chk_conf_val(0,10, hfc2_els_retry) ) pp->els_retry =  hfc2_els_retry; break;
		case  3 : if( hfc_chk_conf_val(0,10, hfc3_els_retry) ) pp->els_retry =  hfc3_els_retry; break;
		case  4 : if( hfc_chk_conf_val(0,10, hfc4_els_retry) ) pp->els_retry =  hfc4_els_retry; break;
		case  5 : if( hfc_chk_conf_val(0,10, hfc5_els_retry) ) pp->els_retry =  hfc5_els_retry; break;
		case  6 : if( hfc_chk_conf_val(0,10, hfc6_els_retry) ) pp->els_retry =  hfc6_els_retry; break;
		case  7 : if( hfc_chk_conf_val(0,10, hfc7_els_retry) ) pp->els_retry =  hfc7_els_retry; break;
		case  8 : if( hfc_chk_conf_val(0,10, hfc8_els_retry) ) pp->els_retry =  hfc8_els_retry; break;
		case  9 : if( hfc_chk_conf_val(0,10, hfc9_els_retry) ) pp->els_retry =  hfc9_els_retry; break;
		case 10 : if( hfc_chk_conf_val(0,10,hfc10_els_retry) ) pp->els_retry = hfc10_els_retry; break;
		case 11 : if( hfc_chk_conf_val(0,10,hfc11_els_retry) ) pp->els_retry = hfc11_els_retry; break;
		case 12 : if( hfc_chk_conf_val(0,10,hfc12_els_retry) ) pp->els_retry = hfc12_els_retry; break;
		case 13 : if( hfc_chk_conf_val(0,10,hfc13_els_retry) ) pp->els_retry = hfc13_els_retry; break;
		case 14 : if( hfc_chk_conf_val(0,10,hfc14_els_retry) ) pp->els_retry = hfc14_els_retry; break;
		case 15 : if( hfc_chk_conf_val(0,10,hfc15_els_retry) ) pp->els_retry = hfc15_els_retry; break;
		case 16 : if( hfc_chk_conf_val(0,10,hfc16_els_retry) ) pp->els_retry = hfc16_els_retry; break;
		case 17 : if( hfc_chk_conf_val(0,10,hfc17_els_retry) ) pp->els_retry = hfc17_els_retry; break;
		case 18 : if( hfc_chk_conf_val(0,10,hfc18_els_retry) ) pp->els_retry = hfc18_els_retry; break;
		case 19 : if( hfc_chk_conf_val(0,10,hfc19_els_retry) ) pp->els_retry = hfc19_els_retry; break;
		case 20 : if( hfc_chk_conf_val(0,10,hfc20_els_retry) ) pp->els_retry = hfc20_els_retry; break;
		case 21 : if( hfc_chk_conf_val(0,10,hfc21_els_retry) ) pp->els_retry = hfc21_els_retry; break;
		case 22 : if( hfc_chk_conf_val(0,10,hfc22_els_retry) ) pp->els_retry = hfc22_els_retry; break;
		case 23 : if( hfc_chk_conf_val(0,10,hfc23_els_retry) ) pp->els_retry = hfc23_els_retry; break;
		case 24 : if( hfc_chk_conf_val(0,10,hfc24_els_retry) ) pp->els_retry = hfc24_els_retry; break;
		case 25 : if( hfc_chk_conf_val(0,10,hfc25_els_retry) ) pp->els_retry = hfc25_els_retry; break;
		case 26 : if( hfc_chk_conf_val(0,10,hfc26_els_retry) ) pp->els_retry = hfc26_els_retry; break;
		case 27 : if( hfc_chk_conf_val(0,10,hfc27_els_retry) ) pp->els_retry = hfc27_els_retry; break;
		case 28 : if( hfc_chk_conf_val(0,10,hfc28_els_retry) ) pp->els_retry = hfc28_els_retry; break;
		case 29 : if( hfc_chk_conf_val(0,10,hfc29_els_retry) ) pp->els_retry = hfc29_els_retry; break;
		case 30 : if( hfc_chk_conf_val(0,10,hfc30_els_retry) ) pp->els_retry = hfc30_els_retry; break;
		case 31 : if( hfc_chk_conf_val(0,10,hfc31_els_retry) ) pp->els_retry = hfc31_els_retry; break;
	}

	if( hfc_chk_conf_val(0,10,hfcmp_els_retry[pp->instance]) ) {
		pp->els_retry = hfcmp_els_retry[pp->instance];
		HFC_DBGPRT("max els retry count <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max els retry count = %d \n", pp->els_retry);
#endif
}

/* FCLNX-GPL-0343 */
void hfc_fx_set_ioctl_scsi_timeout(struct port_info *pp)
{
	pp->ioctl_scsi_timeout = HFC_IOCTL_SCSI_TIMEOUT; /* default = 30s */
	
	if(pp->defparam) return;
	
	if( hfc_chk_conf_val(1,120, hfc_ioctl_scsi_timeout)){ /* in case global parameter is set */
		pp->ioctl_scsi_timeout = hfc_ioctl_scsi_timeout; 
		
	}

#if 0
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(1,120, hfc0_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc0_ioctl_scsi_timeout; break;
		case  1 : if( hfc_chk_conf_val(1,120, hfc1_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc1_ioctl_scsi_timeout; break;
		case  2 : if( hfc_chk_conf_val(1,120, hfc2_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc2_ioctl_scsi_timeout; break;
		case  3 : if( hfc_chk_conf_val(1,120, hfc3_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc3_ioctl_scsi_timeout; break;
		case  4 : if( hfc_chk_conf_val(1,120, hfc4_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc4_ioctl_scsi_timeout; break;
		case  5 : if( hfc_chk_conf_val(1,120, hfc5_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc5_ioctl_scsi_timeout; break;
		case  6 : if( hfc_chk_conf_val(1,120, hfc6_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc6_ioctl_scsi_timeout; break;
		case  7 : if( hfc_chk_conf_val(1,120, hfc7_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc7_ioctl_scsi_timeout; break;
		case  8 : if( hfc_chk_conf_val(1,120, hfc8_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc8_ioctl_scsi_timeout; break;
		case  9 : if( hfc_chk_conf_val(1,120, hfc9_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout =  hfc9_ioctl_scsi_timeout; break;
		case 10 : if( hfc_chk_conf_val(1,120,hfc10_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc10_ioctl_scsi_timeout; break;
		case 11 : if( hfc_chk_conf_val(1,120,hfc11_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc11_ioctl_scsi_timeout; break;
		case 12 : if( hfc_chk_conf_val(1,120,hfc12_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc12_ioctl_scsi_timeout; break;
		case 13 : if( hfc_chk_conf_val(1,120,hfc13_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc13_ioctl_scsi_timeout; break;
		case 14 : if( hfc_chk_conf_val(1,120,hfc14_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc14_ioctl_scsi_timeout; break;
		case 15 : if( hfc_chk_conf_val(1,120,hfc15_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc15_ioctl_scsi_timeout; break;
		case 16 : if( hfc_chk_conf_val(1,120,hfc16_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc16_ioctl_scsi_timeout; break;
		case 17 : if( hfc_chk_conf_val(1,120,hfc17_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc17_ioctl_scsi_timeout; break;
		case 18 : if( hfc_chk_conf_val(1,120,hfc18_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc18_ioctl_scsi_timeout; break;
		case 19 : if( hfc_chk_conf_val(1,120,hfc19_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc19_ioctl_scsi_timeout; break;
		case 20 : if( hfc_chk_conf_val(1,120,hfc20_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc20_ioctl_scsi_timeout; break;
		case 21 : if( hfc_chk_conf_val(1,120,hfc21_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc21_ioctl_scsi_timeout; break;
		case 22 : if( hfc_chk_conf_val(1,120,hfc22_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc22_ioctl_scsi_timeout; break;
		case 23 : if( hfc_chk_conf_val(1,120,hfc23_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc23_ioctl_scsi_timeout; break;
		case 24 : if( hfc_chk_conf_val(1,120,hfc24_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc24_ioctl_scsi_timeout; break;
		case 25 : if( hfc_chk_conf_val(1,120,hfc25_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc25_ioctl_scsi_timeout; break;
		case 26 : if( hfc_chk_conf_val(1,120,hfc26_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc26_ioctl_scsi_timeout; break;
		case 27 : if( hfc_chk_conf_val(1,120,hfc27_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc27_ioctl_scsi_timeout; break;
		case 28 : if( hfc_chk_conf_val(1,120,hfc28_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc28_ioctl_scsi_timeout; break;
		case 29 : if( hfc_chk_conf_val(1,120,hfc29_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc29_ioctl_scsi_timeout; break;
		case 30 : if( hfc_chk_conf_val(1,120,hfc30_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc30_ioctl_scsi_timeout; break;
		case 31 : if( hfc_chk_conf_val(1,120,hfc31_ioctl_scsi_timeout) ) pp->ioctl_scsi_timeout = hfc31_ioctl_scsi_timeout; break;
	}
#endif

	if( hfc_chk_conf_val(1,120,hfcmp_ioctl_scsi_timeout[pp->instance]) ) {
		pp->ioctl_scsi_timeout = hfcmp_ioctl_scsi_timeout[pp->instance];
		HFC_DBGPRT("max ioctl scsi command timeout period <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("max ioctl scsi command timeout period = %d \n", pp->ioctl_scsi_timeout);
}

void hfc_fx_set_cmd_per_lun(struct port_info *pp)															/* FCLNX-283 STR*/
{
	
	pp->cmd_per_lun = HFC_CMD_PER_LUN;

	if(pp->defparam) return;
	
	if(hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc_cmd_per_lun)){ /* in case global parameter is set */
		pp->cmd_per_lun = hfc_cmd_per_lun; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc0_cmd_per_lun) ) pp->cmd_per_lun =  hfc0_cmd_per_lun; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc1_cmd_per_lun) ) pp->cmd_per_lun =  hfc1_cmd_per_lun; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc2_cmd_per_lun) ) pp->cmd_per_lun =  hfc2_cmd_per_lun; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc3_cmd_per_lun) ) pp->cmd_per_lun =  hfc3_cmd_per_lun; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc4_cmd_per_lun) ) pp->cmd_per_lun =  hfc4_cmd_per_lun; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc5_cmd_per_lun) ) pp->cmd_per_lun =  hfc5_cmd_per_lun; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc6_cmd_per_lun) ) pp->cmd_per_lun =  hfc6_cmd_per_lun; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc7_cmd_per_lun) ) pp->cmd_per_lun =  hfc7_cmd_per_lun; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc8_cmd_per_lun) ) pp->cmd_per_lun =  hfc8_cmd_per_lun; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN, hfc9_cmd_per_lun) ) pp->cmd_per_lun =  hfc9_cmd_per_lun; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc10_cmd_per_lun) ) pp->cmd_per_lun = hfc10_cmd_per_lun; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc11_cmd_per_lun) ) pp->cmd_per_lun = hfc11_cmd_per_lun; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc12_cmd_per_lun) ) pp->cmd_per_lun = hfc12_cmd_per_lun; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc13_cmd_per_lun) ) pp->cmd_per_lun = hfc13_cmd_per_lun; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc14_cmd_per_lun) ) pp->cmd_per_lun = hfc14_cmd_per_lun; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc15_cmd_per_lun) ) pp->cmd_per_lun = hfc15_cmd_per_lun; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc16_cmd_per_lun) ) pp->cmd_per_lun = hfc16_cmd_per_lun; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc17_cmd_per_lun) ) pp->cmd_per_lun = hfc17_cmd_per_lun; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc18_cmd_per_lun) ) pp->cmd_per_lun = hfc18_cmd_per_lun; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc19_cmd_per_lun) ) pp->cmd_per_lun = hfc19_cmd_per_lun; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc20_cmd_per_lun) ) pp->cmd_per_lun = hfc20_cmd_per_lun; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc21_cmd_per_lun) ) pp->cmd_per_lun = hfc21_cmd_per_lun; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc22_cmd_per_lun) ) pp->cmd_per_lun = hfc22_cmd_per_lun; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc23_cmd_per_lun) ) pp->cmd_per_lun = hfc23_cmd_per_lun; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc24_cmd_per_lun) ) pp->cmd_per_lun = hfc24_cmd_per_lun; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc25_cmd_per_lun) ) pp->cmd_per_lun = hfc25_cmd_per_lun; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc26_cmd_per_lun) ) pp->cmd_per_lun = hfc26_cmd_per_lun; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc27_cmd_per_lun) ) pp->cmd_per_lun = hfc27_cmd_per_lun; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc28_cmd_per_lun) ) pp->cmd_per_lun = hfc28_cmd_per_lun; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc29_cmd_per_lun) ) pp->cmd_per_lun = hfc29_cmd_per_lun; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc30_cmd_per_lun) ) pp->cmd_per_lun = hfc30_cmd_per_lun; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfc31_cmd_per_lun) ) pp->cmd_per_lun = hfc31_cmd_per_lun; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_CMD_PER_LUN,hfcmp_cmd_per_lun[pp->instance]) ) {
		pp->cmd_per_lun =  hfcmp_cmd_per_lun[pp->instance];
		HFC_DBGPRT("cmd_per_lun <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("cmd_per_lun = %d \n", pp->cmd_per_lun );
}																										/* FCLNX-283 END */



void hfc_fx_set_max_sectors(struct port_info *pp)															/* FCLNX-283 STR */
{
	
	pp->max_sectors = HFC_MAX_SECTORS;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc_max_sectors)){ /* in case global parameter is set */
		pp->max_sectors = hfc_max_sectors; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc0_max_sectors) ) pp->max_sectors =  hfc0_max_sectors; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc1_max_sectors) ) pp->max_sectors =  hfc1_max_sectors; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc2_max_sectors) ) pp->max_sectors =  hfc2_max_sectors; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc3_max_sectors) ) pp->max_sectors =  hfc3_max_sectors; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc4_max_sectors) ) pp->max_sectors =  hfc4_max_sectors; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc5_max_sectors) ) pp->max_sectors =  hfc5_max_sectors; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc6_max_sectors) ) pp->max_sectors =  hfc6_max_sectors; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc7_max_sectors) ) pp->max_sectors =  hfc7_max_sectors; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc8_max_sectors) ) pp->max_sectors =  hfc8_max_sectors; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS, hfc9_max_sectors) ) pp->max_sectors =  hfc9_max_sectors; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc10_max_sectors) ) pp->max_sectors = hfc10_max_sectors; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc11_max_sectors) ) pp->max_sectors = hfc11_max_sectors; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc12_max_sectors) ) pp->max_sectors = hfc12_max_sectors; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc13_max_sectors) ) pp->max_sectors = hfc13_max_sectors; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc14_max_sectors) ) pp->max_sectors = hfc14_max_sectors; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc15_max_sectors) ) pp->max_sectors = hfc15_max_sectors; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc16_max_sectors) ) pp->max_sectors = hfc16_max_sectors; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc17_max_sectors) ) pp->max_sectors = hfc17_max_sectors; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc18_max_sectors) ) pp->max_sectors = hfc18_max_sectors; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc19_max_sectors) ) pp->max_sectors = hfc19_max_sectors; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc20_max_sectors) ) pp->max_sectors = hfc20_max_sectors; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc21_max_sectors) ) pp->max_sectors = hfc21_max_sectors; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc22_max_sectors) ) pp->max_sectors = hfc22_max_sectors; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc23_max_sectors) ) pp->max_sectors = hfc23_max_sectors; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc24_max_sectors) ) pp->max_sectors = hfc24_max_sectors; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc25_max_sectors) ) pp->max_sectors = hfc25_max_sectors; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc26_max_sectors) ) pp->max_sectors = hfc26_max_sectors; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc27_max_sectors) ) pp->max_sectors = hfc27_max_sectors; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc28_max_sectors) ) pp->max_sectors = hfc28_max_sectors; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc29_max_sectors) ) pp->max_sectors = hfc29_max_sectors; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc30_max_sectors) ) pp->max_sectors = hfc30_max_sectors; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfc31_max_sectors) ) pp->max_sectors = hfc31_max_sectors; break;
	}

	if( hfc_chk_conf_val(0,HFC_MAX_MAX_SECTORS,hfcmp_max_sectors[pp->instance]) ) {
		pp->max_sectors =  hfcmp_max_sectors[pp->instance];
		HFC_DBGPRT("max_sectors <-- hfcldd.conf\n");
	}
	
	if( pp->max_sectors > 0 ){
		 pp->hosts->max_sectors = pp->max_sectors;
	}

	HFC_DBGPRT("max_sectors = %d \n", pp->max_sectors );
}																										/* FCLNX-283 END */


void hfc_fx_set_vary_io(struct port_info *pp)																/* FCLNX-283 STR */
{
	
	pp->vary_io = 0;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,HFC_VARY_IO,hfc_vary_io)){ /* in case global parameter is set */
		pp->vary_io = hfc_vary_io; 
		
	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc0_vary_io) ) pp->vary_io =  hfc0_vary_io; break;
		case  1 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc1_vary_io) ) pp->vary_io =  hfc1_vary_io; break;
		case  2 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc2_vary_io) ) pp->vary_io =  hfc2_vary_io; break;
		case  3 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc3_vary_io) ) pp->vary_io =  hfc3_vary_io; break;
		case  4 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc4_vary_io) ) pp->vary_io =  hfc4_vary_io; break;
		case  5 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc5_vary_io) ) pp->vary_io =  hfc5_vary_io; break;
		case  6 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc6_vary_io) ) pp->vary_io =  hfc6_vary_io; break;
		case  7 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc7_vary_io) ) pp->vary_io =  hfc7_vary_io; break;
		case  8 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc8_vary_io) ) pp->vary_io =  hfc8_vary_io; break;
		case  9 : if( hfc_chk_conf_val(0,HFC_VARY_IO, hfc9_vary_io) ) pp->vary_io =  hfc9_vary_io; break;
		case 10 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc10_vary_io) ) pp->vary_io = hfc10_vary_io; break;
		case 11 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc11_vary_io) ) pp->vary_io = hfc11_vary_io; break;
		case 12 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc12_vary_io) ) pp->vary_io = hfc12_vary_io; break;
		case 13 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc13_vary_io) ) pp->vary_io = hfc13_vary_io; break;
		case 14 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc14_vary_io) ) pp->vary_io = hfc14_vary_io; break;
		case 15 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc15_vary_io) ) pp->vary_io = hfc15_vary_io; break;
		case 16 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc16_vary_io) ) pp->vary_io = hfc16_vary_io; break;
		case 17 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc17_vary_io) ) pp->vary_io = hfc17_vary_io; break;
		case 18 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc18_vary_io) ) pp->vary_io = hfc18_vary_io; break;
		case 19 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc19_vary_io) ) pp->vary_io = hfc19_vary_io; break;
		case 20 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc20_vary_io) ) pp->vary_io = hfc20_vary_io; break;
		case 21 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc21_vary_io) ) pp->vary_io = hfc21_vary_io; break;
		case 22 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc22_vary_io) ) pp->vary_io = hfc22_vary_io; break;
		case 23 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc23_vary_io) ) pp->vary_io = hfc23_vary_io; break;
		case 24 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc24_vary_io) ) pp->vary_io = hfc24_vary_io; break;
		case 25 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc25_vary_io) ) pp->vary_io = hfc25_vary_io; break;
		case 26 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc26_vary_io) ) pp->vary_io = hfc26_vary_io; break;
		case 27 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc27_vary_io) ) pp->vary_io = hfc27_vary_io; break;
		case 28 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc28_vary_io) ) pp->vary_io = hfc28_vary_io; break;
		case 29 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc29_vary_io) ) pp->vary_io = hfc29_vary_io; break;
		case 30 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc30_vary_io) ) pp->vary_io = hfc30_vary_io; break;
		case 31 : if( hfc_chk_conf_val(0,HFC_VARY_IO,hfc31_vary_io) ) pp->vary_io = hfc31_vary_io; break;
	}

	if( hfc_chk_conf_val(0,HFC_VARY_IO,hfcmp_vary_io[pp->instance]) ) {
		pp->vary_io =  hfcmp_vary_io[pp->instance];
		HFC_DBGPRT("vary_io <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("vary_io = %d \n", pp->vary_io );
}																										/* FCLNX-283 END */

void hfc_fx_set_lun_reset_delay(struct port_info *pp)/* FCLNX-0506 */ 	/* FCLNX-GPL-038 */
{

	pp->lun_reset_delay = HFC_LUN_DELAY; /* default = 0s */

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,60,hfc_lun_reset_delay)){ /* in case global parameter is set */
		pp->lun_reset_delay = hfc_lun_reset_delay; 

	}
	switch(pp->instance){
		case  0 : if( hfc_chk_conf_val(0,60, hfc0_lun_reset_delay) ) pp->lun_reset_delay =  hfc0_lun_reset_delay; break;
		case  1 : if( hfc_chk_conf_val(0,60, hfc1_lun_reset_delay) ) pp->lun_reset_delay =  hfc1_lun_reset_delay; break;
		case  2 : if( hfc_chk_conf_val(0,60, hfc2_lun_reset_delay) ) pp->lun_reset_delay =  hfc2_lun_reset_delay; break;
		case  3 : if( hfc_chk_conf_val(0,60, hfc3_lun_reset_delay) ) pp->lun_reset_delay =  hfc3_lun_reset_delay; break;
		case  4 : if( hfc_chk_conf_val(0,60, hfc4_lun_reset_delay) ) pp->lun_reset_delay =  hfc4_lun_reset_delay; break;
		case  5 : if( hfc_chk_conf_val(0,60, hfc5_lun_reset_delay) ) pp->lun_reset_delay =  hfc5_lun_reset_delay; break;
		case  6 : if( hfc_chk_conf_val(0,60, hfc6_lun_reset_delay) ) pp->lun_reset_delay =  hfc6_lun_reset_delay; break;
		case  7 : if( hfc_chk_conf_val(0,60, hfc7_lun_reset_delay) ) pp->lun_reset_delay =  hfc7_lun_reset_delay; break;
		case  8 : if( hfc_chk_conf_val(0,60, hfc8_lun_reset_delay) ) pp->lun_reset_delay =  hfc8_lun_reset_delay; break;
		case  9 : if( hfc_chk_conf_val(0,60, hfc9_lun_reset_delay) ) pp->lun_reset_delay =  hfc9_lun_reset_delay; break;
		case 10 : if( hfc_chk_conf_val(0,60,hfc10_lun_reset_delay) ) pp->lun_reset_delay = hfc10_lun_reset_delay; break;
		case 11 : if( hfc_chk_conf_val(0,60,hfc11_lun_reset_delay) ) pp->lun_reset_delay = hfc11_lun_reset_delay; break;
		case 12 : if( hfc_chk_conf_val(0,60,hfc12_lun_reset_delay) ) pp->lun_reset_delay = hfc12_lun_reset_delay; break;
		case 13 : if( hfc_chk_conf_val(0,60,hfc13_lun_reset_delay) ) pp->lun_reset_delay = hfc13_lun_reset_delay; break;
		case 14 : if( hfc_chk_conf_val(0,60,hfc14_lun_reset_delay) ) pp->lun_reset_delay = hfc14_lun_reset_delay; break;
		case 15 : if( hfc_chk_conf_val(0,60,hfc15_lun_reset_delay) ) pp->lun_reset_delay = hfc15_lun_reset_delay; break;
		case 16 : if( hfc_chk_conf_val(0,60,hfc16_lun_reset_delay) ) pp->lun_reset_delay = hfc16_lun_reset_delay; break;
		case 17 : if( hfc_chk_conf_val(0,60,hfc17_lun_reset_delay) ) pp->lun_reset_delay = hfc17_lun_reset_delay; break;
		case 18 : if( hfc_chk_conf_val(0,60,hfc18_lun_reset_delay) ) pp->lun_reset_delay = hfc18_lun_reset_delay; break;
		case 19 : if( hfc_chk_conf_val(0,60,hfc19_lun_reset_delay) ) pp->lun_reset_delay = hfc19_lun_reset_delay; break;
		case 20 : if( hfc_chk_conf_val(0,60,hfc20_lun_reset_delay) ) pp->lun_reset_delay = hfc20_lun_reset_delay; break;
		case 21 : if( hfc_chk_conf_val(0,60,hfc21_lun_reset_delay) ) pp->lun_reset_delay = hfc21_lun_reset_delay; break;
		case 22 : if( hfc_chk_conf_val(0,60,hfc22_lun_reset_delay) ) pp->lun_reset_delay = hfc22_lun_reset_delay; break;
		case 23 : if( hfc_chk_conf_val(0,60,hfc23_lun_reset_delay) ) pp->lun_reset_delay = hfc23_lun_reset_delay; break;
		case 24 : if( hfc_chk_conf_val(0,60,hfc24_lun_reset_delay) ) pp->lun_reset_delay = hfc24_lun_reset_delay; break;
		case 25 : if( hfc_chk_conf_val(0,60,hfc25_lun_reset_delay) ) pp->lun_reset_delay = hfc25_lun_reset_delay; break;
		case 26 : if( hfc_chk_conf_val(0,60,hfc26_lun_reset_delay) ) pp->lun_reset_delay = hfc26_lun_reset_delay; break;
		case 27 : if( hfc_chk_conf_val(0,60,hfc27_lun_reset_delay) ) pp->lun_reset_delay = hfc27_lun_reset_delay; break;
		case 28 : if( hfc_chk_conf_val(0,60,hfc28_lun_reset_delay) ) pp->lun_reset_delay = hfc28_lun_reset_delay; break;
		case 29 : if( hfc_chk_conf_val(0,60,hfc29_lun_reset_delay) ) pp->lun_reset_delay = hfc29_lun_reset_delay; break;
		case 30 : if( hfc_chk_conf_val(0,60,hfc30_lun_reset_delay) ) pp->lun_reset_delay = hfc30_lun_reset_delay; break;
		case 31 : if( hfc_chk_conf_val(0,60,hfc31_lun_reset_delay) ) pp->lun_reset_delay = hfc31_lun_reset_delay; break;
	}
	
	if( hfc_chk_conf_val(0,60,hfcmp_lun_rst_delay[pp->instance]) ) {
		pp->lun_reset_delay = hfcmp_lun_rst_delay[pp->instance];
		HFC_DBGPRT("lun_reset_delay <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("lun_reset_delay = %d \n", pp->lun_reset_delay);
}																	/* FCLNX-GPL-038 */

void hfc_fx_set_abort_t_restrain(struct port_info *pp) /* FCLNX-0506 */
{
	pp->abort_t_restrain = 0;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_abort_t_restrain)){ /* in case global parameter is set*/
			pp->abort_t_restrain = hfc_abort_t_restrain;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_abort_t_restrain[pp->instance]) ) {
		pp->abort_t_restrain =	hfcmp_abort_t_restrain[pp->instance];
		HFC_DBGPRT("abort_t_restrain <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("abort_t_restrain = %d \n", pp->abort_t_restrain );
} /* FCLNX-0506 */

void hfc_fx_set_login_restrain(struct port_info *pp) /* FCLNX-0506 */
{
	pp->login_restrain = 0;
}

void hfc_fx_set_tgtrst_restrain(struct port_info *pp)
{
	pp->tgtrst_restrain = 0;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_tgtrst_restrain)){ /* in case global parameter is set*/
		pp->tgtrst_restrain = hfc_tgtrst_restrain;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_tgtrst_restrain[pp->instance]) ) {
		pp->tgtrst_restrain = hfcmp_tgtrst_restrain[pp->instance];
		HFC_DBGPRT("tgtrst_restrain <-- hfcldd.conf\n");
	}
	
	HFC_DBGPRT("tgtrst_restrain = %d \n", pp->tgtrst_restrain );
}

void hfc_fx_set_mck_point(struct port_info *pp)
{
	pp->mck_point = HFC_NO_MCK_POINT;
	
	if(pp->defparam) {
		HFC_DBGPRT( "mck_point=%d\n",pp->mck_point);
		return;
	}
	
	if(hfc_chk_conf_val(HFC_NO_MCK_POINT,HFC_MAX_MCK_POINT,hfc_mck_point)){ /* in case global parameter is set*/
		pp->mck_point = hfc_mck_point;
	}
	
	HFC_DBGPRT("mck_point = %d \n", pp->mck_point );
}

/* Set INT type (INTx or MSI or MSI-X)  */
void hfc_fx_set_msi_enable(struct port_info *pp)
{
	pp->msi_enable = HFC_INT_TYPE_MSIX; /* Default: HFC_INT_TYPE_MSIX == 0 */
	if(pp->defparam) return; /* Force Default Parameter*/

	if(hfc_chk_conf_val(0,2,hfc_msi_enable)){		/* in case global parameter is set */	/* FCLNX-GPL-FX-203 */
		pp->msi_enable = hfc_msi_enable;			/* 0:INTx, 1:MSI, 2:MSI-X  */
	}
	
	/* in case local parameter is set */
	if( hfc_chk_conf_val(0,2,hfcmp_msi_enable[pp->instance]) ) {
		pp->msi_enable = hfcmp_msi_enable[pp->instance];	/* 0:INTx, 1:MSI, 2:MSI-X */	/* FCLNX-GPL-FX-203 */
		HFC_DBGPRT("msi_enable <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("MSI enable = %d\n", pp->msi_enable);
}

/* Set Max Value of "PCIe IP Core SRAM ERR(CE) Count" */
void hfc_fx_set_pcie_sram_ce_count(struct port_info *pp){
	
	/* Set Default Parameter */
	pp->max_pcie_sram_ce_cnt = HFC_PCIE_SRAM_CE_CNT;
	
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(0,6,hfc_pcie_sram_ce_fx)){
		pp->max_pcie_sram_ce_cnt = hfc_pcie_sram_ce_fx;
	}
	/* in case port parameter is set*/
	if(hfc_chk_conf_val(0,6,hfcmp_pcie_sram_ce_fx[pp->instance]) ) {
		pp->max_pcie_sram_ce_cnt = hfcmp_pcie_sram_ce_fx[pp->instance];
	}
	
	return;
}
/* Set Max Value of "Core ERR(CE) Count" */
void hfc_fx_set_core_ce_count(struct port_info *pp){

	/* Set Default Parameter */
	pp->max_core_ce_cnt = HFC_CORE_CE_CNT;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(0,16,hfc_core_ce)){
		pp->max_core_ce_cnt = hfc_core_ce;
	}
	/* in case port parameter is set*/
	if(hfc_chk_conf_val(0,16,hfcmp_core_ce[pp->instance]) ) {
		pp->max_core_ce_cnt = hfcmp_core_ce[pp->instance];
	}
	return;
}
/* Set "1:Do" or "0:Not"  dummy read. (for MSI/MSI-X) */
void hfc_fx_set_inta_dummy_read(struct port_info *pp){

	/* Set Default Parameter */
	pp->inta_dummy_read = HFC_DUMMY_READ_OFF;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0,1,hfc_inta_dummy_read)){
		pp->inta_dummy_read = hfc_inta_dummy_read;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0,1,hfcmp_inta_dummy_read[pp->instance]) ) {
		pp->inta_dummy_read = hfcmp_inta_dummy_read[pp->instance];
	}
	return;
}
/* Set the number of max HW log page count.(0 - 16) */
void hfc_fx_set_max_hwlog_cnt(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->max_hwlog_cnt = HFC_FX_HWLOG_CNT;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0,16,hfc_max_hwlog_cnt)){
		pp->max_hwlog_cnt = hfc_max_hwlog_cnt;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0,16,hfcmp_max_hwlog_cnt[pp->instance]) ) {
		pp->max_hwlog_cnt = hfcmp_max_hwlog_cnt[pp->instance];
	}
	return;
}

/* Set Debug mode */
void hfc_fx_set_debug_func(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->debug_func = 0;
	
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0x0000, 0xffff, hfc_debug_func)){
		pp->debug_func = (uint)hfc_debug_func;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0x0000, 0xffff, hfcmp_debug_func[pp->instance]) ) {
		pp->debug_func = (uint)hfcmp_debug_func[pp->instance];
	}
	
	if((pp->debug_func & 0x80)&&((pp->mlpf_mode & HFC_MMODE_SHADOW )))
		pp->core_deg_mode   = HFC_FX_CORE_DEG_DISABLE;	/* FCLNX-GPL-FX-414 */
	
	return;
}

/* Set Issue D3 Hot in suspend Process */	/* FCLNX-GPL-306 */
void hfc_fx_set_issue_d3hot(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->issue_d3hot = 0x00;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, 1, hfc_issue_d3hot)){
		pp->issue_d3hot = (uchar)hfc_issue_d3hot;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0, 1, hfcmp_issue_d3hot[pp->instance]) ) {
		pp->issue_d3hot = (uchar)hfcmp_issue_d3hot[pp->instance];
	}
	return;
}											/* FCLNX-GPL-306 */

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* Set rport control */
void hfc_fx_set_sysfs_control(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->sysfs_control = 0x00;
	set_bit(HFC_SYSFS_RPORT, (ulong *)&pp->sysfs_control); /* FCLNX-GPL-207 */
	set_bit(HFC_SYSFS_STATISTICS, (ulong *)&pp->sysfs_control); /* FCLNX-GPL-207 */
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfc_sysfs_control)){
		pp->sysfs_control = (uchar)hfc_sysfs_control;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(0x00, 0xff, hfcmp_sysfs_control[pp->instance]) ) {
		pp->sysfs_control = (uchar)hfcmp_sysfs_control[pp->instance];
	}
	return;
}

/* Set dev_loss_tmo */								/* FCLNX-GPL-260 */
void hfc_fx_set_dev_loss_tmo(struct port_info *pp)
{
	/* Set Default Parameter */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		pp->dev_loss_tmo = HFC_DEF_DEV_LOSS_TMO;
	else
		pp->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
#else
	pp->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
#endif
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		/* link down timer  */
		if(hfc_chk_conf_val(0,60,hfc_link_down)){ /* in case global parameter is set */
			pp->dev_loss_tmo = hfc_link_down; 

		}
		switch(pp->instance){
			case  0 : if( hfc_chk_conf_val(0,60, hfc0_link_down) ) pp->dev_loss_tmo =  hfc0_link_down; break;
			case  1 : if( hfc_chk_conf_val(0,60, hfc1_link_down) ) pp->dev_loss_tmo =  hfc1_link_down; break;
			case  2 : if( hfc_chk_conf_val(0,60, hfc2_link_down) ) pp->dev_loss_tmo =  hfc2_link_down; break;
			case  3 : if( hfc_chk_conf_val(0,60, hfc3_link_down) ) pp->dev_loss_tmo =  hfc3_link_down; break;
			case  4 : if( hfc_chk_conf_val(0,60, hfc4_link_down) ) pp->dev_loss_tmo =  hfc4_link_down; break;
			case  5 : if( hfc_chk_conf_val(0,60, hfc5_link_down) ) pp->dev_loss_tmo =  hfc5_link_down; break;
			case  6 : if( hfc_chk_conf_val(0,60, hfc6_link_down) ) pp->dev_loss_tmo =  hfc6_link_down; break;
			case  7 : if( hfc_chk_conf_val(0,60, hfc7_link_down) ) pp->dev_loss_tmo =  hfc7_link_down; break;
			case  8 : if( hfc_chk_conf_val(0,60, hfc8_link_down) ) pp->dev_loss_tmo =  hfc8_link_down; break;
			case  9 : if( hfc_chk_conf_val(0,60, hfc9_link_down) ) pp->dev_loss_tmo =  hfc9_link_down; break;
			case 10 : if( hfc_chk_conf_val(0,60,hfc10_link_down) ) pp->dev_loss_tmo = hfc10_link_down; break;
			case 11 : if( hfc_chk_conf_val(0,60,hfc11_link_down) ) pp->dev_loss_tmo = hfc11_link_down; break;
			case 12 : if( hfc_chk_conf_val(0,60,hfc12_link_down) ) pp->dev_loss_tmo = hfc12_link_down; break;
			case 13 : if( hfc_chk_conf_val(0,60,hfc13_link_down) ) pp->dev_loss_tmo = hfc13_link_down; break;
			case 14 : if( hfc_chk_conf_val(0,60,hfc14_link_down) ) pp->dev_loss_tmo = hfc14_link_down; break;
			case 15 : if( hfc_chk_conf_val(0,60,hfc15_link_down) ) pp->dev_loss_tmo = hfc15_link_down; break;
			case 16 : if( hfc_chk_conf_val(0,60,hfc16_link_down) ) pp->dev_loss_tmo = hfc16_link_down; break;
			case 17 : if( hfc_chk_conf_val(0,60,hfc17_link_down) ) pp->dev_loss_tmo = hfc17_link_down; break;
			case 18 : if( hfc_chk_conf_val(0,60,hfc18_link_down) ) pp->dev_loss_tmo = hfc18_link_down; break;
			case 19 : if( hfc_chk_conf_val(0,60,hfc19_link_down) ) pp->dev_loss_tmo = hfc19_link_down; break;
			case 20 : if( hfc_chk_conf_val(0,60,hfc20_link_down) ) pp->dev_loss_tmo = hfc20_link_down; break;
			case 21 : if( hfc_chk_conf_val(0,60,hfc21_link_down) ) pp->dev_loss_tmo = hfc21_link_down; break;
			case 22 : if( hfc_chk_conf_val(0,60,hfc22_link_down) ) pp->dev_loss_tmo = hfc22_link_down; break;
			case 23 : if( hfc_chk_conf_val(0,60,hfc23_link_down) ) pp->dev_loss_tmo = hfc23_link_down; break;
			case 24 : if( hfc_chk_conf_val(0,60,hfc24_link_down) ) pp->dev_loss_tmo = hfc24_link_down; break;
			case 25 : if( hfc_chk_conf_val(0,60,hfc25_link_down) ) pp->dev_loss_tmo = hfc25_link_down; break;
			case 26 : if( hfc_chk_conf_val(0,60,hfc26_link_down) ) pp->dev_loss_tmo = hfc26_link_down; break;
			case 27 : if( hfc_chk_conf_val(0,60,hfc27_link_down) ) pp->dev_loss_tmo = hfc27_link_down; break;
			case 28 : if( hfc_chk_conf_val(0,60,hfc28_link_down) ) pp->dev_loss_tmo = hfc28_link_down; break;
			case 29 : if( hfc_chk_conf_val(0,60,hfc29_link_down) ) pp->dev_loss_tmo = hfc29_link_down; break;
			case 30 : if( hfc_chk_conf_val(0,60,hfc30_link_down) ) pp->dev_loss_tmo = hfc30_link_down; break;
			case 31 : if( hfc_chk_conf_val(0,60,hfc31_link_down) ) pp->dev_loss_tmo = hfc31_link_down; break;
		}

		if( hfc_chk_conf_val(0,60,hfcmp_link_down[pp->instance]) ) {
			pp->dev_loss_tmo = hfcmp_link_down[pp->instance];
			HFC_DBGPRT("linkup_timeout <-- hfcldd.conf\n");
		}
	}
#else
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfc_dev_loss_tmo)){
		pp->dev_loss_tmo = (uchar)hfc_dev_loss_tmo;
	}
	/* in case port parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_DEV_LOSS_TMO, HFC_MAX_DEV_LOSS_TMO, hfcmp_dev_loss_tmo[pp->instance]) ) {
		pp->dev_loss_tmo = (uchar)hfcmp_dev_loss_tmo[pp->instance];
	}

#endif
	return;
}													/* FCLNX-GPL-260 */

/* FCLNX-GPL-565 start *//* FCLNX-GPL-575 start */
void hfc_fx_set_scan_finished_tmo(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->scan_finished_tmo = HFC_SCAN_FINISHED_TMO;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_SCAN_FINISHED_TMO, HFC_MAX_SCAN_FINISHED_TMO, hfc_scan_finished_tmo)){
		pp->scan_finished_tmo = hfc_scan_finished_tmo;
	}
	return;
}
/* FCLNX-GPL-565 end *//* FCLNX-GPL-575 end */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/* Set HBA Isolation" */		/* FCLNX-GPL-349 */
void hfc_fx_set_hba_isolation(struct port_info *pp)
{	
	/* Set Default Parameter */
	pp->hba_isolation = HFC_ISOL_START;

	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set*/
	if(hfc_chk_conf_val(HFC_ISOL_STOP,HFC_ISOL_START,hfc_hba_isolation)){
		pp->hba_isolation = hfc_hba_isolation;
	}
	return;
}								/* FCLNX-GPL-349 */

/* Set Linkdown(s) Limit  */	/* FCLNX-GPL-349 */
void hfc_fx_set_ld_err_limit_s(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->ld_err_limit_s = HFC_MIN_LD_ERR_LIMIT_S;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_LD_ERR_LIMIT_S,HFC_MAX_LD_ERR_LIMIT_S,hfc_ld_err_limit_s)){
		pp->ld_err_limit_s = hfc_ld_err_limit_s;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_LD_ERR_LIMIT_S,HFC_MAX_LD_ERR_LIMIT_S,hfcmp_ld_err_limit_s[pp->instance]) ) {
		pp->ld_err_limit_s = hfcmp_ld_err_limit_s[pp->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set interface Error Limit */	/* FCLNX-GPL-349 */
void hfc_fx_set_if_err_limit(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->if_err_limit = HFC_MIN_IF_ERR_LIMIT;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_IF_ERR_LIMIT,HFC_MAX_IF_ERR_LIMIT,hfc_if_err_limit)){
		pp->if_err_limit = hfc_if_err_limit;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_IF_ERR_LIMIT,HFC_MAX_IF_ERR_LIMIT,hfcmp_if_err_limit[pp->instance]) ) {
		pp->if_err_limit = hfcmp_if_err_limit[pp->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set Time-Out Error Limit */	/* FCLNX-GPL-349 */
void hfc_fx_set_to_err_limit(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->to_err_limit = HFC_MIN_TO_ERR_LIMIT;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_MIN_TO_ERR_LIMIT,HFC_MAX_TO_ERR_LIMIT,hfc_to_err_limit)){
		pp->to_err_limit = hfc_to_err_limit;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_MIN_TO_ERR_LIMIT,HFC_MAX_TO_ERR_LIMIT,hfcmp_to_err_limit[pp->instance]) ) {
		pp->to_err_limit = hfcmp_to_err_limit[pp->instance];
	}
}								/* FCLNX-GPL-349 */

/* Set Mailbox Time-Out Retry  *//* FCLNX-GPL-349 */
void hfc_fx_set_to_reset_retry(struct port_info *pp)
{
#if 0
	/* Set Default Parameter */
	pp->to_reset_retry = HFC_TO_RESET_RETRY;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_TO_RESET_RETRY_MIN,HFC_TO_RESET_RETRY_MAX,hfc_to_reset_retry)){
		pp->to_reset_retry = hfc_to_reset_retry;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_TO_RESET_RETRY_MIN,HFC_TO_RESET_RETRY_MAX,hfcmp_to_reset_retry[pp->instance]) ) {
		pp->to_reset_retry = hfcmp_to_reset_retry[pp->instance];
	}
#endif
}								/* FCLNX-GPL-349 */

/* Set Time-Out Reset Error */	/* FCLNX-GPL-349 */
void hfc_fx_set_rt_err_enable(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->rt_err_enable = HFC_RT_ERR_NOT_SPPRTD;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_RT_ERR_NOT_SPPRTD,HFC_RT_ERR_SPPRTD,hfc_rt_err_enable)){
		pp->rt_err_enable = hfc_rt_err_enable;
	}
	/* in case local parameter is set */
	if( hfc_chk_conf_val(HFC_RT_ERR_NOT_SPPRTD,HFC_RT_ERR_SPPRTD,hfcmp_rt_err_enable[pp->instance]) ) {
		pp->rt_err_enable = hfcmp_rt_err_enable[pp->instance];
	}
}								/* FCLNX-GPL-349 */


/* Set limit log */				/* FCLNX-GPL-491 */
void hfc_fx_set_limit_log(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->limit_log = 0;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, 2, hfc_limit_log)){
		pp->limit_log = hfc_limit_log;
	}
}								/* FCLNX-GPL-491 */

/* Set filter target */				/* FCLNX-GPL-491 */
void hfc_fx_set_filter_target(struct port_info *pp)
{
#if 0
	/* Set Default Parameter */
	pp->filter_target = HFC_DISABLE_FILTERTGT;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_DISABLE_FILTERTGT,HFC_ENABLE_FILTERTGT,hfc_filter_target)){
		pp->filter_target = hfc_filter_target;
	}
#endif
}								/* FCLNX-GPL-491 */

/* Set statistics for Virtage */				/* FCLNX-GPL-494 */
void hfc_fx_set_hg_stats_disable(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->hg_stats_disable = HFC_ENABLE_HGSTATS;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_ENABLE_HGSTATS,HFC_DISABLE_HGSTATS,hfc_hg_stats_disable)){
		pp->hg_stats_disable = hfc_hg_stats_disable;
	}
}								/* FCLNX-GPL-494 */


void hfc_fx_set_rport_lu_scan(struct port_info *pp)
{
	pp->rport_lu_scan = HFC_ENABLE_RPORT_LU_SCAN; /* default = 1 */
	
	if(pp->defparam) return;
	
	/* Control lu scan with rport function */

	if( hfc_chk_conf_val(0,1, hfc_rport_lu_scan)){ /* in case global parameter is set */
		pp->rport_lu_scan = hfc_rport_lu_scan; 
	}

	if( hfc_chk_conf_val(0,1,hfcmp_rport_lu_scan[pp->instance]) ) {
		pp->rport_lu_scan = hfcmp_rport_lu_scan[pp->instance];
		HFC_DBGPRT("max control change_queue_depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("rport lu scan mode = %d \n", pp->rport_lu_scan);
}


/* FCLNX-GPL-574 */
void hfc_fx_set_ctl_change_qdepth(struct port_info *pp)
{
	pp->ctl_change_qdepth = HFC_DISABLE_CTL_CHANGE_QDEPTH; /* default = 0 */
	
	if(pp->defparam) return;
	
	/* Control change_queue_depth entry point */

	if( hfc_chk_conf_val(0,1, hfc_ctl_change_qdepth)){ /* in case global parameter is set */
		pp->ctl_change_qdepth = hfc_ctl_change_qdepth; 
		
	}

	if( hfc_chk_conf_val(0,1,hfcmp_ctl_change_qdepth[pp->instance]) ) {
		pp->ctl_change_qdepth = hfcmp_ctl_change_qdepth[pp->instance];
		HFC_DBGPRT("max control change_queue_depth <-- hfcldd.conf\n");
	}

	HFC_DBGPRT("control change_queue_depth = %d \n", pp->ctl_change_qdepth);
}

void hfc_fx_set_core_control(struct port_info *pp)
{
	pp->core_control = HFC_FX_CORECTL_ENHANCE_RR;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_CORECTL_ENHANCE_RR,HFC_FX_CORECTL_CPU_TO_CORE,hfc_core_control)){ /* in case global parameter is set*/
		pp->core_control = hfc_core_control;
	}

	if( hfc_chk_conf_val(HFC_FX_CORECTL_ENHANCE_RR,HFC_FX_CORECTL_CPU_TO_CORE,hfcmp_core_control[pp->instance]) ) {
		pp->core_control = hfcmp_core_control[pp->instance];
	}
}

void hfc_fx_set_cc_cnt(struct port_info *pp)
{
	pp->cc_cnt = HFC_FX_DEF_CC_CNT;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_CC_CNT, HFC_FX_MAX_CC_CNT, hfc_cc_cnt)){ /* in case global parameter is set*/
		pp->cc_cnt = hfc_cc_cnt;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_CC_CNT, HFC_FX_MAX_CC_CNT, hfcmp_cc_cnt[pp->instance]) ) {
		pp->cc_cnt = hfcmp_cc_cnt[pp->instance];
	}
}

void hfc_fx_set_cc_size(struct port_info *pp)
{
	pp->cc_size = HFC_FX_DEF_CC_SIZE;

	if(pp->defparam) return;

	/* FCLNX-GPL-FX-292, 302 */
	if(hfc_chk_conf_val(HFC_FX_MIN_CC_SIZE, HFC_FX_MAX_CC_SIZE, hfc_cc_size)){ /* in case global parameter is set*/
		pp->cc_size = hfc_cc_size;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_CC_SIZE, HFC_FX_MAX_CC_SIZE, hfcmp_cc_size[pp->instance]) ) {
		pp->cc_size = hfcmp_cc_size[pp->instance];
	}
}

void hfc_fx_set_cc_core(struct port_info *pp)
{
	pp->cc_core = HFC_FX_DEF_CC_CORE;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0x01, HFC_FX_DEF_CC_CORE, hfc_cc_core)){ /* in case global parameter is set*/
		pp->cc_core = hfc_cc_core;
	}

	if( hfc_chk_conf_val(0x01, HFC_FX_DEF_CC_CORE, hfcmp_cc_core[pp->instance]) ) {
		pp->cc_core = hfcmp_cc_core[pp->instance];
	}
}

void hfc_fx_set_link_reset(struct port_info *pp)
{
	pp->link_reset = HFC_FX_LINK_RESET_MULTI;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_LINK_RESET_MULTI,HFC_FX_LINK_RESET_SINGLE,hfc_link_reset)){ /* in case global parameter is set*/
		pp->link_reset = hfc_link_reset;
	}

	if( hfc_chk_conf_val(HFC_FX_LINK_RESET_MULTI,HFC_FX_LINK_RESET_SINGLE,hfcmp_link_reset[pp->instance]) ) {
		pp->link_reset = hfcmp_link_reset[pp->instance];
	}
}

void hfc_fx_set_vport_count(struct port_info *pp)
{
	pp->max_vport_count = HFC_FX_DF_VPORT_COUNT;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_VPORT_COUNT,HFC_FX_MAX_VPORT_COUNT,hfc_vport_count)){ /* in case global parameter is set*/
		pp->max_vport_count = hfc_vport_count;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_VPORT_COUNT,HFC_FX_MAX_VPORT_COUNT,hfcmp_vport_count[pp->instance]) ) {
		pp->max_vport_count = hfcmp_vport_count[pp->instance];
	}
}

void hfc_fx_set_frame_count(struct port_info *pp)
{
	pp->max_frame_count = HFC_FX_DF_FRAME_COUNT;	/* FCLNX-GPL-FX-241,272 */

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_FRAME_COUNT,HFC_FX_MAX_FRAME_COUNT,hfc_frame_count)){ /* in case global parameter is set*/
		pp->max_frame_count = hfc_frame_count;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_FRAME_COUNT,HFC_FX_MAX_FRAME_COUNT,hfcmp_frame_count[pp->instance]) ) {
		pp->max_frame_count = hfcmp_frame_count[pp->instance];
	}
}

void hfc_fx_set_mq_num(struct port_info *pp)
{
	pp->mq_num = HFC_FX_DF_MQ_NUM;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_MQ_NUM,HFC_FX_MAX_MQ_NUM,hfc_mq_num)){ /* in case global parameter is set*/
		pp->mq_num = hfc_mq_num;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_MQ_NUM,HFC_FX_MAX_MQ_NUM,hfcmp_mq_num[pp->instance]) ) {
		pp->mq_num = hfcmp_mq_num[pp->instance];
	}
}


void hfc_fx_set_rdtsc(struct port_info *pp)
{
	pp->pm_control = HFC_FX_PM_OFF;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0,1,hfc_rdtsc)){ /* in case global parameter is set*/
		pp->pm_control = hfc_rdtsc;
	}

	if( hfc_chk_conf_val(0,1,hfcmp_rdtsc[pp->instance]) ) {
		pp->pm_control = hfcmp_rdtsc[pp->instance];
	}
}

void hfc_fx_set_intdisable(struct port_info *pp)
{
	if(pp->mlpf_mode & HFC_MMODE_SHADOW )
		hfc_fx_write_reg_ext(pp, 0x23e, 0x01, (uchar)0x00);
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(0x00,0xff,hfc_intdisable)){ /* in case global parameter is set*/
		hfc_fx_write_reg_ext(pp, 0x23e, 0x01, (uchar)hfc_intdisable);
	}

	if( hfc_chk_conf_val(0x00,0xff,hfcmp_intdisable[pp->instance]) ) {
		hfc_fx_write_reg_ext(pp, 0x23e, 0x01, (uchar)hfcmp_intdisable[pp->instance]);
	}
}

void hfc_fx_set_intenable(struct port_info *pp)
{
	if(pp->mlpf_mode & HFC_MMODE_SHADOW )
		hfc_fx_write_reg_ext(pp, 0x23f, 0x01, (uchar)0x00);
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(0x00,0xff,hfc_intenable)){ /* in case global parameter is set*/
		hfc_fx_write_reg_ext(pp, 0x23f, 0x01, (uchar)hfc_intenable);
	}

	if( hfc_chk_conf_val(0x00,0xff,hfcmp_intdisable[pp->instance]) ) {
		hfc_fx_write_reg_ext(pp, 0x23f, 0x01, (uchar)hfcmp_intenable[pp->instance]);
	}
}

/* FCLNX-GPL-FX-014 Start */
void hfc_fx_set_total_abort_to(struct port_info *pp)
{
	pp->total_abort_to = HFC_FX_DISABLE_TOTAL_ABORT_TO;
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_TOTAL_ABORT_TO,HFC_FX_MAX_TOTAL_ABORT_TO,hfc_total_abort_to)){ /* in case global parameter is set*/
		pp->total_abort_to = hfc_total_abort_to;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_TOTAL_ABORT_TO,HFC_FX_MAX_TOTAL_ABORT_TO,hfcmp_total_abort_to[pp->instance]) ) {
		pp->total_abort_to = hfcmp_total_abort_to[pp->instance];
	}
}

void hfc_fx_set_total_tgtrst_to(struct port_info *pp)
{
	pp->total_tgtrst_to = HFC_FX_DISABLE_TOTAL_TGTRST_TO;
	
	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_TOTAL_TGTRST_TO,HFC_FX_MAX_TOTAL_TGTRST_TO,hfc_total_tgtrst_to)){ /* in case global parameter is set*/
		pp->total_tgtrst_to = hfc_total_tgtrst_to;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_TOTAL_TGTRST_TO,HFC_FX_MAX_TOTAL_TGTRST_TO,hfcmp_total_tgtrst_to[pp->instance]) ) {
		pp->total_tgtrst_to = hfcmp_total_tgtrst_to[pp->instance];
	}
}
/* FCLNX-GPL-FX-014 End */

/* Set NPIV mode *//* FCLNX-GPL-FX-137 */
void hfc_fx_set_npiv_enable(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->npiv_mode = 0;
	
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, 2, hfc_npiv_enable)){
		if (hfc_npiv_enable == 1) {
			pp->npiv_mode = HFC_NPIV_ENABLE;
		}
		else if (hfc_npiv_enable == 2) {
			pp->npiv_mode = HFC_NPIV_ENABLE;
			pp->npiv_mode |= HFC_NPIV_EXT_MODE;
		}
	}
	return;
}/* FCLNX-GPL-FX-137 */

/* FCLNX-GPL-FX-147 */
void hfc_fx_set_max_io(struct port_info *pp)
{
	pp->max_io = HFC_FX_DEFAULT_MAX_IO;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_DEFAULT_MAX_IO, HFC_FX_MAX_IO, hfc_maxio)){ /* in case global parameter is set*/
		pp->max_io = hfc_maxio;
	}

	if( hfc_chk_conf_val(HFC_FX_DEFAULT_MAX_IO, HFC_FX_MAX_IO, hfcmp_maxio[pp->instance]) ) {
		pp->max_io = hfcmp_maxio[pp->instance];
	}
}
/* FCLNX-GPL-FX-147 */

/* FCLNX-GPL-FX-446 >>> */
void hfc_fx_set_login_seq_retry_cnt(struct port_info *pp)
{
	pp->login_seq_retry_cnt = HFC_FX_DEFAULT_LOGIN_SEQ_RETRY_CNT;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(HFC_FX_MIN_LOGIN_SEQ_RETRY_CNT,HFC_FX_MAX_LOGIN_SEQ_RETRY_CNT,hfc_login_seq_retry_cnt)){ /* in case global parameter is set*/
		pp->login_seq_retry_cnt = hfc_login_seq_retry_cnt;
	}

	if( hfc_chk_conf_val(HFC_FX_MIN_LOGIN_SEQ_RETRY_CNT,HFC_FX_MAX_LOGIN_SEQ_RETRY_CNT,hfcmp_login_seq_retry_cnt[pp->instance])){
		pp->login_seq_retry_cnt = hfcmp_login_seq_retry_cnt[pp->instance];
	}
	return;
}
/* FCLNX-GPL-FX-446 <<< */

void hfc_fx_set_mq_enable(struct port_info *pp)
{
	pp->mq_mode = 0;

	if(pp->defparam) return;

	if(hfc_chk_conf_val(0, 1, hfc_mq_enable)){ /* in case global parameter is set*/
		if (hfc_mq_enable == 1) {
			pp->mq_mode = HFC_MQ_ENABLE;
		}
	}

	if( hfc_chk_conf_val(0, 1, hfcmp_mq_enable[pp->instance]) ) {
		if (hfcmp_mq_enable[pp->instance] == 1) {
			pp->mq_mode = HFC_MQ_ENABLE;
		}
	}
}


/* FCLNX-GPL-547 start */
/* Set a choice of log files  */
void hfc_fx_set_log_file(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->log_file = 0;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(0, HFC_LOGFILE_TYPE_MAX, hfc_log_file)){
		pp->log_file = hfc_log_file;
	}
	return;
}

/* Set max lu number */
void hfc_fx_set_max_lun(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->max_lun = HFC_MAX_LUN;
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam) return;
	/* in case global parameter is set */
	if(hfc_chk_conf_val(1, HFC_MAX_LUN, hfc_max_lun)){
		pp->max_lun = hfc_max_lun;
	}
	return;
}
/* FCLNX-GPL-547 end */

/* Set CPU MAP set mode *//* FCLNX-GPL-FX-420 */
void hfc_fx_set_cpu_map(struct port_info *pp)
{
	/* Set Default Parameter */
	pp->cpu_map = HFC_VEC_CPU_MAP_ENABLE;
	
	/* in case Force_Default_Parameter is ON */
	if(pp->defparam){
		return;
	}
	/* in case global parameter is set */
	if(hfc_chk_conf_val(HFC_VEC_CPU_MAP_ENABLE, HFC_VEC_CPU_MAP_DISABLE, hfc_cpu_map)){
		pp->cpu_map = hfc_cpu_map;
	}
	return;
}

void hfc_conf_setup(struct adap_info *ap)
{

	hfc_set_topology(ap);			/* get topology					ap->topology */
	hfc_set_linkspeed(ap);			/* get linkspeed				ap->linkspeed */
	hfc_set_max_transfer(ap);		/* get max tarnsfer length		ap->max_transfer, ap->dma_max */
	ap->iov_map_cnt = ap->dma_max / HFC_PAGE_SIZE;

	hfc_set_linkup_tmo(ap); 		/* get linkup timeout time		ap->linkup_tmo;*/
	hfc_set_linkup2_tmo(ap);		/* get linkup timeout time after MCK    ap->linkup2_tmo;*/ /* FCLNX-0241 */
	hfc_set_reset_delay(ap);		/* get reset delay				ap->scsi_reset_delay */
	hfc_set_mck_count(ap);			/* get machine check count		ap->max_mck_cnt */

	hfc_set_pref_alpa(ap);			/* get preferred alpa			ap->pref_alpa */
	ap->host_alpa = ap->pref_alpa;

	hfc_set_target_timeout(ap); 	/* get target reset timeout 	ap->target_reset_tmo */
	hfc_set_abort_timeout(ap);		/* get abort timeout			ap->abort_tmo*/

	hfc_set_seg_trace(ap);			/* get seg info trace mode		ap->fw_parm */
	hfc_set_queue_depth(ap);		/* get queue_depth 				ap->queue_depth */
	hfc_set_enable_target_reset(ap);/* get enable target reset mode ap->enable_tgtrst */

	hfc_set_max_target(ap);			/* max target number			ap->max_target */
	hfc_set_xob_max(ap);			/* xob max						ap->xob_max */
	hfc_set_xrb_max(ap);			/* xrb max						ap->xrb_max */
	hfc_set_slog_max(ap);			/* softlog max					ap->slog_max */
	hfc_set_trc_max(ap);			/* trace max					ap->trc_max */
	hfc_set_pkt_num(ap);			/* hfc_pkt max					ap->pkt_num */
	hfc_set_can_queue(ap);			/* can queue depth				ap->can_queue */
	hfc_set_sg_tblsize(ap);			/* sg table size				ap->sg_tablesize */
	hfc_set_cmnd_num(ap);			/* cmnd num						ap->cmnd_num */
	hfc_set_minus_tout(ap);			/* minus timeout				ap->minus_timeout */
	hfc_set_scsi_allowed(ap);		/* Scsi_Cmnd->allowed 			ap->scsi_allowed */
	hfc_set_login_retry(ap);		/* mailbox login retry 			ap->login_retry */		/* FCLNX-GPL-0343 */
	hfc_set_els_retry(ap);			/* mailbox els retry 			ap->els_retry */		/* FCLNX-GPL-0343 */
	hfc_set_ioctl_scsi_timeout(ap);	/* ioctl timeout period 		ap->ioctl_scsi_timeout */	/* FCLNX-GPL-0343 */
	hfc_set_cmd_per_lun(ap);		/* cmd_per_lun					ap->cmd_per_lun */
	hfc_set_max_sectors(ap);		/* max_sectors					ap->max_sectors */
	hfc_set_lun_reset_delay(ap);	/* Delay after LUN Reset 		ap->tmt_delay			*/ /* FCLNX-0506 */	/* FCLNX-GPL-038 */
	hfc_set_abort_t_restrain(ap);	/* Restrain of AbortT.S issue	ap->abort_t_restrain	*/ /* FCLNX-0506 */
	hfc_set_login_restrain(ap);		/* Restrain of Login isuue		ap->login_restrain		*/ /* FCLNX-0506 */
	hfc_set_mck_point(ap);  /* FCLNX-0533 */
	hfc_set_pcie_sram_ce_count(ap);	/* Max PCIe IP Core SRAM ERR(CE) Count	ap->max_pcie_sram_ce_cnt	*/
	hfc_set_core_ce_count(ap);		/* Max Core ERR(CE) Count				ap->max_core_ce_cnt			*/
	hfc_set_msi_enable(ap);			/* INT type(INTx or MSI or MSI-X)		ap->msi_enable*/
	hfc_set_inta_dummy_read(ap);	/* "1:Do" or "0:Not" dummy read(for MSI/MSI-X) ap->inta_dummy_read */
	hfc_set_max_hwlog_cnt(ap);		/* Set the number of max HW log page count.(0 - 16) */
	hfc_set_debug_func(ap);			/* Set Debug mode */
	hfc_set_issue_d3hot(ap);		/* Set Issue D3 Hot */	/* FCLNX-GPL-306 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_set_sysfs_control(ap);		/* Set rport control */
	hfc_set_dev_loss_tmo(ap);		/* Set dev_loss_tmo  */	/* FCLNX-GPL-260 */
	hfc_set_scan_finished_tmo(ap);	/* Set scan_finished time out */ /* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	hfc_set_hba_isolation(ap);		/* Set HBA isolation	 		ap->hba_isolation	*/	/* FCLNX-GPL-349 */
	if (!( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ))				/* FCLNX-GPL-349 */
	{
		hfc_set_ld_err_limit_s(ap);		/* Set Linkdown(s) Limit 		ap->ld_err_limit_s	*/
		hfc_set_if_err_limit(ap);		/* Set interface Error Limit 	ap->if_err_limit	*/
		hfc_set_to_err_limit(ap);		/* Set Time-Out Error Limit		ap->to_err_limit	*/
		hfc_set_to_reset_retry(ap);		/* Set Mailbox Time-Out Retry	ap->to_reset_retry	*/
		hfc_set_rt_err_enable(ap);		/* Set Time-Out Reset Error		ap->rt_err_enable	*/
	}																						/* FCLNX-GPL-349 */

	hfc_set_limit_log(ap);				/* Set limit log				ap->limit_log */		/* FCLNX-GPL-491 */
	hfc_set_filter_target(ap);			/* Set filter target			ap->filter_target */	/* FCLNX-GPL-491 */
	hfc_set_hg_stats_disable(ap);		/* Set statistics for Virtage	ap->hg_stats_disable */	/* FCLNX-GPL-494 */

	hfc_set_log_file(ap);		/* Set a choice of log files */ /* FCLNX-GPL-547 */
	hfc_set_max_lun(ap);		/* Set max lu number */ /* FCLNX-GPL-547 */

	hfc_set_ctl_change_qdepth(ap);		/* Set change_queue_depth control ap->ctl_change_qdepth *//* FCLNX-GPL-574 */
	hfc_set_rport_lu_scan(ap);			/* Set rport lu scan control ap->rport_lu_scan	 	*/	/* FCLNX-GPL-575 */

	ap->wmsg = hfc_message_enable;
	ap->errlog_max	= HFC_MAX_ERRLOG_CNT;

	ap->scsi_time_out = hfc_scsi_time_out;

	/* originally set by modules.conf. This part is still tentative */
	ap->hosts->max_id 		= 255; 		/* modules.conf ??? */
	ap->hosts->max_lun 		= ap->max_lun; /* the number of lun is more than 256 *//* FCLNX-GPL-343,547 */
	ap->hosts->max_channel 	= 1;
	ap->hosts->max_cmd_len 	= 16;	
	ap->hosts->this_id 		= 255;
	ap->hosts->can_queue 	= ap->can_queue;
	ap->hosts->n_io_port		= 0xff;
	ap->hosts->base		= (unsigned long)ap->mem_base_addr;
	ap->hosts->cmd_per_lun  = ap->cmd_per_lun;		/* FCLNX-283 */
	ap->hosts->sg_tablesize  = ap->sg_tblsize;		/* FCLNX-283 */

	ap->hosts->max_cmd_len = 255;

	HFC_EXIT("hfc_conf_setup");

	return;	
}


int hfc_search_adapter_number(struct adap_info *ap)
{
	int i;
	uchar buf[64];
	uint wk;
	uchar pre_conf;
	uchar wkchar_wwn0 = 0;
	uchar wkchar_wwn1 = 0;
	uchar err_wwpn[16];		/* FCLNX-GPL-150 */
	uchar addr[4];																		/* FCLNX-GPL-319 */

	uint64_t pxe_add_ww_name;	/* FCLNX-XXX */
	uint64_t swap_add_ww_name; /* FCLNX-GPL-180 */
	
	HFC_ENTRY("hfc_set_config");
	
	HFC_DBGPRT("pkg code=%d , port = %d \n", ap->pkg.code, ap->pkg.port );

 	if(ap->pkg.type == HFC_PKTYPE_FPP){ /* FPP? */
		pre_conf = (uchar) hfc_read_cnfg (ap, 0x4B, 0x1);
 	}
 	else if(ap->pkg.type == HFC_PKTYPE_FIVE){ /* FIVE */
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xAF, 0x1);
	}
	else{ /* FIVE-EX */
		pre_conf = (uchar) hfc_read_cnfg (ap, 0xCA, 0x1);
	}


	HFC_DBGPRT("function number = %d\n",ap->port_no);
		
	if ( HFC_MMODE_CHECK_BASIC(ap) )
	{
		if( hfc_read_flash(ap, 0x54, 4, addr)){											/* FCLNX-GPL-319 */
			return (-1);
		}
		HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
		if(hfc_read_flash(ap, wk+16*ap->port_no, 16, buf)){
			return(-1);
		}																				/* FCLNX-GPL-319 */
	
		/* Read WWPN and WWNN from flash. */
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
		ap->ww_name = wk;
		ap->ww_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
		ap->ww_name |= wk;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[8])));
		ap->node_name = wk;
		ap->node_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[12])));
		ap->node_name |= wk;

		HFC_MEMCPY(err_wwpn+0,  &buf[0], 4); /* FCLNX-GPL-161 */
		HFC_MEMCPY(err_wwpn+4,  &buf[4], 4); /* FCLNX-GPL-161 */
		
		HFC_DBGPRT("ww_name=%llx, node_name=%llx\n",(unsigned long long)ap->ww_name, (unsigned long long)ap->node_name);

		ap->org_ww_name = ap->ww_name;
		ap->org_node_name = ap->node_name;

		/*** Restore this HBA's slot_no ***/
		/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
		swap_add_ww_name = hfc_restore_add_wwn(ap);
		
		/*** check add_ww_name ***/
		if(swap_add_ww_name != 0)
		{
			/* restore "add_ww_name" */
			ap->add_ww_name   = swap_add_ww_name;
			ap->add_node_name = swap_add_ww_name +1;
			ap->ww_name       = ap->add_ww_name;
			ap->node_name     = ap->add_node_name;
			
			/* Set pre_conf bit */ /* FCLNX-GPL-269 start */
		 	if(ap->pkg.type == HFC_PKTYPE_FPP)
		 	{	/* FPP */
				hfc_write_cnfg(ap, 0x4B, 0x1, 0x01);
 			}
		 	else if(ap->pkg.type == HFC_PKTYPE_FIVE)
		 	{	/* FIVE */
				hfc_write_cnfg(ap, 0xAF, 0x1, 0x01);
			}
			else
			{	/* FIVE-EX */
				hfc_write_cnfg(ap, 0xCA, 0x1, 0x01);
			}
			/* FCLNX-GPL-269 end */
		}
		else /* FCLNX-GPL-180 end */
		{
			if( (pre_conf == 0x01) || (pre_conf == 0x03) ){

				if( hfc_read_flash(ap, 0x20018 + 16*ap->port_no, 8, buf) ){
					return (-1); /* FCLNX-GPL-116 */
				}

				HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
				ap->add_ww_name = wk;
				ap->add_ww_name <<= 32;
				HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
				ap->add_ww_name |= wk;
				ap->add_node_name=ap->add_ww_name + 1;		

				HFC_MEMCPY(err_wwpn+8,  &buf[0], 4); /* FCLNX-GPL-161 */
				HFC_MEMCPY(err_wwpn+12, &buf[4], 4); /* FCLNX-GPL-161 */
				
				if( ((buf[0]&0xf0) == 0x50) || ((buf[0]&0xf0) == 0x20) ){ /* FCLNX-GPL-076 */

					ap->ww_name   = ap->add_ww_name;				/* FCLNX-0299 */
					ap->node_name = ap->add_node_name;				/* FCLNX-0299 */
					
					/*** save "add_ww_name" ***/
					/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
					hfc_backup_add_wwn(ap, ap->add_ww_name);
				}
				else {
					HFC_ERRPRT("hfcldd : 'Additional WWPN is invalid ' addWWPN = 0x%llx orgWWPN = 0x%llx\n", 
						(unsigned long long)ap->add_ww_name, (unsigned long long)ap->org_ww_name);	/* FCLNX-GPL-076 */

					hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, 0xD7, err_wwpn, 16); /* FCLNX-GPL-161 */

					ap->add_ww_name   = ap->ww_name;
					ap->add_node_name = ap->node_name;

					return (MAX_ADAP_CNT + 1);                          /* FCLNX-0376 */
				}		
			}
		}
	}	
	else {															/* @MLPF */
		hfc_mlpf_setup_wwn(ap);
		
		wkchar_wwn0 = (uchar)(ap->vfc_ww_name >> 56);
		wkchar_wwn1 = (uchar)(ap->vfc_node_name >> 56);
		
		wkchar_wwn0 &= 0xf0;										/* FCLNX-0338 */
		wkchar_wwn1 &= 0xf0;										/* FCLNX-0338 */
		
		if( ( (wkchar_wwn0 != 0x50) && (wkchar_wwn0 != 0x20) ) ||
			( (wkchar_wwn1 != 0x50) && (wkchar_wwn1 != 0x20) ) )
		{
			HFC_DBGPRT("hfcldd @MLPF: WWN check error \n"); 

			HFC_ERRPRT("hfcldd : 'Additional VFC WWPN is invalid ' addWWPN = 0x%llx orgWWPN = 0x%llx\n", 
				(unsigned long long)wkchar_wwn0, (unsigned long long)ap->org_ww_name);

			memset(buf, 0, 16);								/* FCLNX-GPL-161 */
			HFC_MEMCPY(buf, (uchar*)&ap->org_ww_name, 8);	/* FCLNX-GPL-161 */
			HFC_4L_TO_4B(wk, (*(uint*)(&buf[0])));			/* FCLNX-GPL-161 */
			HFC_MEMCPY(err_wwpn+0,  &buf[0], 4); 			/* FCLNX-GPL-161 */
			HFC_4L_TO_4B(wk, (*(uint*)(&buf[4])));			/* FCLNX-GPL-161 */
			HFC_MEMCPY(err_wwpn+4, &buf[4], 4); 			/* FCLNX-GPL-161 */

	
			HFC_MEMCPY(buf, (uchar*)&ap->vfc_ww_name, 8);	/* FCLNX-GPL-161 */
			HFC_4L_TO_4B(wk, (*(uint*)(&buf[0])));			/* FCLNX-GPL-161 */
			HFC_MEMCPY(err_wwpn+8,  &buf[0], 4); 			/* FCLNX-GPL-161 */
			HFC_4L_TO_4B(wk, (*(uint*)(&buf[4])));			/* FCLNX-GPL-161 */
			HFC_MEMCPY(err_wwpn+12, &buf[4], 4);			/* FCLNX-GPL-161 */

			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, 0xD7, err_wwpn, 16); /* FCLNX-GPL-161 */
			
			return (MAX_ADAP_CNT + 1);								/* FCLNX-0376 */

		}
	}																/* @MLPF */

	/* --------------------------------------------------------------------- FCLNX-XXX */
	if(hfc_pxe_boot != 0){

		if( hfc_read_flash(ap, 0x20018 + 16*ap->port_no, 8, buf) ){ /* FCLNX-GPL-116 */
			return (-1);
		}

		HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
		pxe_add_ww_name = wk;
		pxe_add_ww_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
		pxe_add_ww_name |= wk;

		wkchar_wwn0 = (uchar)(pxe_add_ww_name >> 56);
		wkchar_wwn0 &= 0xf0;

		if ( (wkchar_wwn0 != 0x50) && (wkchar_wwn0 != 0x20) ){ /* Error case */
			ap->ww_name = ap->org_ww_name;
			ap->node_name = ap->org_node_name;
		}

	}
	/* --------------------------------------------------------------------- FCLNX-XXX */


	for(i=0; i<MAX_ADAP_CNT; i++) {
		if(ap->ww_name == hfc_manage_info.adap_bind[i]){
			HFC_DBGPRT("adapter info is hit, i = %d, wwpn=%llx\n", i, (unsigned long long)ap->ww_name);
			return (i); /* hit */
		} 
	}

	HFC_EXIT("hfc_set_config");
	return (-1); 
}


int hfc_get_adapter_port_no(struct adap_info *ap)
{
	int func_no;
	ulong pci_status;
	
	HFC_ENTRY("hfc_get_adapter_port_no");
	
 	if(ap->pkg.type == HFC_PKTYPE_FPP) { /* FPP? */
		ap->port_no = 0; /* set port number to '0' */
 	}
 	else if(ap->pkg.type == HFC_PKTYPE_FIVE) { /* FIVE */
 		pci_status = (ulong) hfc_read_cnfg (ap, 0x6C, 0x4);
 		HFC_DBGPRT("pci status = %x\n",(uint)pci_status);
		func_no = (pci_status & 0x00000007);
		ap->port_no = func_no; 
	}
	else { /* FIVE-EX */ 
		ap->port_no = PCI_FUNC(ap->pci_cfginf->devfn);
		 /* FIVE-EX */
	}
	
	HFC_EXIT("hfc_set_config");
	return (ap->port_no); 
}


int hfc_get_adapter_bindings(void)
{
	char path[32];
	uint64_t value1;
	uint64_t value2;
	int i, hit=0, hit_max=-1;
	int hit1, hit2;

	HFC_ENTRY("hfc_get_adapter_bindings");


	for(i=0;i<MAX_ADAP_CNT;i++) hfc_manage_info.adap_bind[i] = -1;

	if((hfclddopts == NULL) && (hfclddconf == NULL)) return (-1);

	/* Make sure adapter number has already registered. */
	
	if(hfclddconf == NULL) {
		for (i=0;i<MAX_ADAP_CNT;i++) {
			sprintf(path,"hfcl%d-wwpn",i);
			/* Get wwpn from modules.conf */
			if( hfc_parse_string(hfclddopts, path, &value1) ){
				hfc_manage_info.adap_bind[i] = value1;
				HFC_DBGPRT("hit adapter no=%d  wwpn %llx  in modules.conf\n",
				i, (unsigned long long)hfc_manage_info.adap_bind[i] );
				hit_max = i;
				hit++;
			}
		}
	}
	else if(hfclddopts == NULL) {
		for (i=0;i<MAX_ADAP_CNT;i++) {
			sprintf(path,"hfcl%d-wwpn",i);
			/* Get wwpn from hfcldd.conf */
			if( hfc_parse_string(hfclddconf, path, &value2) ){
				hfc_manage_info.adap_bind[i] = value2;
				HFC_DBGPRT("hit adapter no=%d  wwpn %llx  in hfcldd.conf\n",
				i, (unsigned long long)hfc_manage_info.adap_bind[i] );
				hit_max = i;
				hit++;
			}
		}
	}
	else {
		for (i=0;i<MAX_ADAP_CNT;i++) {
			sprintf(path,"hfcl%d-wwpn",i);
			/* Get wwpn from modules.conf */
			hit1 = hfc_parse_string(hfclddopts, path, &value1);
			/* Get wwpn from hfcldd.conf */
			hit2 = hfc_parse_string(hfclddconf, path, &value2);
			
			if ((hit1 != 0) && (hit2 == 0)) {
				hfc_manage_info.adap_bind[i] = value1;
				HFC_DBGPRT("hit adapter no=%d  wwpn %llx  in modules.conf\n",
				i, (unsigned long long)hfc_manage_info.adap_bind[i] );
				hit_max = i;
				hit++;
			}
			else if ( hit1 || hit2 ) {
				hfc_manage_info.adap_bind[i] = value2;

				HFC_DBGPRT("hit adapter no=%d  wwpn %llx  in hfcldd.conf\n",
				i, (unsigned long long)hfc_manage_info.adap_bind[i] );
				hit_max = i;
				hit++;
			}
		}
	}

	HFC_EXIT("hfc_get_adapter_bindings");

	return (hit_max);

}



void hfc_param_setup(void)
{
	int value=0;
	int i;
	char buf[32];

	HFC_ENTRY("hfc_param_setup");

	/* "hfc<x>_connection_type" */
	if (hfc_param_search("hfc_connection_type", &value)) hfc_connection_type = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_connection_type[i] = -1;
		sprintf(buf,"hfc%d_connection_type",i);
		if (hfc_param_search(buf, &value)) hfcmp_connection_type[i] = value;
	}

	/* "hfc<x>_link_speed" */
	if (hfc_param_search("hfc_link_speed", &value)) hfc_link_speed = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_link_speed[i] = -1;
		sprintf(buf,"hfc%d_link_speed",i);
		if (hfc_param_search(buf, &value)) hfcmp_link_speed[i] = value;
	}
	
	/* "hfc<x>_max_transfer" */
	if (hfc_param_search("hfc_max_transfer", &value)) hfc_max_transfer = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_max_transfer[i] = -1;
		sprintf(buf,"hfc%d_max_transfer",i);
		if (hfc_param_search(buf, &value)) hfcmp_max_transfer[i] = value;
	}

	/* "hfc<x>_link_down" */
	if (hfc_param_search("hfc_link_down", &value)) hfc_link_down = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_link_down[i] = -1;
		sprintf(buf,"hfc%d_link_down",i);
		if (hfc_param_search(buf, &value)) hfcmp_link_down[i] = value;
	}

	/* "hfc<x>_link_down" */					/* FCLNX-241 */
	if (hfc_param_search("hfc_link_down2", &value)) hfc_link_down2 = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_link_down2[i] = -1;
		sprintf(buf,"hfc%d_link_down2",i);
		if (hfc_param_search(buf, &value)) hfcmp_link_down2[i] = value;
	}								/* FCLNX-0241 */

	/* "hfc<x>_reset_delay" */
	if (hfc_param_search("hfc_reset_delay", &value)) hfc_reset_delay = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_reset_delay[i] = -1;
		sprintf(buf,"hfc%d_reset_delay",i);
		if (hfc_param_search(buf, &value)) hfcmp_reset_delay[i] = value;
	}

	/* "hfc<x>_mck_retry" */
	if (hfc_param_search("hfc_mck_retry", &value)) hfc_mck_retry = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_mck_retry[i] = -1;
		sprintf(buf,"hfc%d_mck_retry",i);
		if (hfc_param_search(buf, &value)) hfcmp_mck_retry[i] = value;
	}

	/* "hfc<x>_preferred_alpa" */
	if (hfc_param_search("hfc_preferred_alpa", &value)) hfc_preferred_alpa = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_preferred_alpa[i] = -1;
		sprintf(buf,"hfc%d_preferred_alpa",i);
		if (hfc_param_search(buf, &value)) hfcmp_preferred_alpa[i] = value;
	}

	/* "hfc<x>_reset_timeout" */
	if (hfc_param_search("hfc_reset_timeout", &value)) hfc_reset_timeout = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_reset_timeout[i] = -1;
		sprintf(buf,"hfc%d_reset_timeout",i);
		if (hfc_param_search(buf, &value)) hfcmp_reset_timeout[i] = value;
	}

	/* "hfc<x>_abort_timeout" */
	if (hfc_param_search("hfc_abort_timeout", &value)) hfc_abort_timeout = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_abort_timeout[i] = -1;
		sprintf(buf,"hfc%d_abort_timeout",i);
		if (hfc_param_search(buf, &value)) hfcmp_abort_timeout[i] = value;
	}

	/* "hfc<x>_enable_tgtrst" */
	if (hfc_param_search("hfc_enable_tgtrst", &value)) hfc_enable_tgtrst = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_enable_tgtrst[i] = -1;
		sprintf(buf,"hfc%d_enable_tgtrst",i);
		if (hfc_param_search(buf, &value)) hfcmp_enable_tgtrst[i] = value;
	}

	/* "hfc<x>_queue_depth" */
	if (hfc_param_search("hfc_queue_depth", &value)) hfc_queue_depth = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_queue_depth[i] = -1;
		sprintf(buf,"hfc%d_queue_depth",i);
		if (hfc_param_search(buf, &value)) hfcmp_queue_depth[i] = value;
	}

	/* "hfc<x>_seg_trace" */
	if (hfc_param_search("hfc_seg_trace", &value)) hfc_seg_trace = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_seg_trace[i] = -1;
		sprintf(buf,"hfc%d_seg_trace",i);
		if (hfc_param_search(buf, &value)) hfcmp_seg_trace[i] = value;
	}

	/* "hfc<x>_max_target" */
	if (hfc_param_search("hfc_max_target", &value)) hfc_max_target = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_max_target[i] = -1;
		sprintf(buf,"hfc%d_max_target",i);
		if (hfc_param_search(buf, &value)) hfcmp_max_target[i] = value;
	}

	/* "hfc<x>_xob_max" */
	if (hfc_param_search("hfc_xob_max", &value)) hfc_xob_max = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_xob_max[i] = -1;
		sprintf(buf,"hfc%d_xob_max",i);
		if (hfc_param_search(buf, &value)) hfcmp_xob_max[i] = value;
	}

	/* "hfc<x>_xrb_max" */
	if (hfc_param_search("hfc_xrb_max", &value)) hfc_xrb_max = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_xrb_max[i] = -1;
		sprintf(buf,"hfc%d_xrb_max",i);
		if (hfc_param_search(buf, &value)) hfcmp_xrb_max[i] = value;
	}

	/* "hfc<x>_slog_max" */
	if (hfc_param_search("hfc_slog_max", &value)) hfc_slog_max = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_slog_max[i] = -1;
		sprintf(buf,"hfc%d_slog_max",i);
		if (hfc_param_search(buf, &value)) hfcmp_slog_max[i] = value;
	}

	/* "hfc<x>_trc_max" */
	if (hfc_param_search("hfc_trc_max", &value)) hfc_trc_max = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_trc_max[i] = -1;
		sprintf(buf,"hfc%d_trc_max",i);
		if (hfc_param_search(buf, &value)) hfcmp_trc_max[i] = value;
	}

	/* "hfc<x>_pkt_num" */
	if (hfc_param_search("hfc_pkt_num", &value)) hfc_pkt_num = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_pkt_num[i] = -1;
		sprintf(buf,"hfc%d_pkt_num",i);
		if (hfc_param_search(buf, &value)) hfcmp_pkt_num[i] = value;
	}
	
	/* "hfc<x>_rsv_pkt_num" */
	if (hfc_param_search("hfc_rsv_pkt_num", &value)) hfc_rsv_pkt_num = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_rsv_pkt_num[i] = -1;
		sprintf(buf,"hfc%d_rsv_pkt_num",i);
		if (hfc_param_search(buf, &value)) hfcmp_rsv_pkt_num[i] = value;
	}

	/* "hfc<x>_pm_pkt_num" */
	if (hfc_param_search("hfc_pm_pkt_num", &value)) hfc_pm_pkt_num = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_pm_pkt_num[i] = -1;
		sprintf(buf,"hfc%d_pm_pkt_num",i);
		if (hfc_param_search(buf, &value)) hfcmp_pm_pkt_num[i] = value;
	}

	/* "hfc<x>_can_queue" */
	if (hfc_param_search("hfc_can_queue", &value)) hfc_can_queue = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_can_queue[i] = -1;
		sprintf(buf,"hfc%d_can_queue",i);
		if (hfc_param_search(buf, &value)) hfcmp_can_queue[i] = value;
	}

	/* "hfc<x>_sg_tblsize" */
	if (hfc_param_search("hfc_sg_tblsize", &value)) hfc_sg_tblsize = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_sg_tblsize[i] = -1;
		sprintf(buf,"hfc%d_sg_tblsize",i);
		if (hfc_param_search(buf, &value)) hfcmp_sg_tblsize[i] = value;
	}

	/* "hfc<x>_cmnd_num" */
	if (hfc_param_search("hfc_cmnd_num", &value)) hfc_cmnd_num = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_cmnd_num[i] = -1;
		sprintf(buf,"hfc%d_cmnd_num",i);
		if (hfc_param_search(buf, &value)) hfcmp_cmnd_num[i] = value;
	}

	/* "hfc<x>_minus_tout" */
	if (hfc_param_search("hfc_minus_tout", &value)) hfc_minus_tout = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_minus_tout[i] = -1;
		sprintf(buf,"hfc%d_minus_tout",i);
		if (hfc_param_search(buf, &value)) hfcmp_minus_tout[i] = value;
	}

	/* "hfc<x>_scsi_allowed" */
	if (hfc_param_search("hfc_scsi_allowed", &value)) hfc_scsi_allowed = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_scsi_allowed[i] = -1;
		sprintf(buf,"hfc%d_scsi_allowed",i);
		if (hfc_param_search(buf, &value)) hfcmp_scsi_allowed[i] = value;
	}

	/* "hfc<x>_login_retry" */	/* FCLNX-GPL-0343 */
	if (hfc_param_search("hfc_login_retry", &value)) hfc_login_retry = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_login_retry[i] = -1;
		sprintf(buf,"hfc%d_login_retry",i);
		if (hfc_param_search(buf, &value)) hfcmp_login_retry[i] = value;
	}

	/* "hfc<x>_els_retry" */	/* FCLNX-GPL-0343 */
	if (hfc_param_search("hfc_els_retry", &value)) hfc_els_retry = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_els_retry[i] = -1;
		sprintf(buf,"hfc%d_els_retry",i);
		if (hfc_param_search(buf, &value)) hfcmp_els_retry[i] = value;
	}

	/* "hfc<x>_ioctl_scsi_timeout" */	/* FCLNX-GPL-0343 */
	if (hfc_param_search("hfc_ioctl_scsi_timeout", &value)) hfc_ioctl_scsi_timeout = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_ioctl_scsi_timeout[i] = -1;
		sprintf(buf,"hfc%d_ioctl_scsi_timeout",i);
		if (hfc_param_search(buf, &value)) hfcmp_ioctl_scsi_timeout[i] = value;
	}

	/* "hfc<x>_cmd_per_lun" */													/* FCLNX-0283 */
	if (hfc_param_search("hfc_cmd_per_lun", &value)) hfc_cmd_per_lun = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_cmd_per_lun[i] = -1;
		sprintf(buf,"hfc%d_cmd_per_lun",i);
		if (hfc_param_search(buf, &value)) hfcmp_cmd_per_lun[i] = value;
	}

	/* "hfc<x>_max_sectors" */
	if (hfc_param_search("hfc_max_sectors", &value)) hfc_max_sectors = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_max_sectors[i] = -1;
		sprintf(buf,"hfc%d_max_sectors",i);
		if (hfc_param_search(buf, &value)) hfcmp_max_sectors[i] = value;
	}
	
	/* "hfc<x>_abort_t_restrain" */
	if (hfc_param_search("hfc_abort_t_restrain", &value)) hfc_abort_t_restrain = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_abort_t_restrain[i] = -1;
		sprintf(buf,"hfc%d_abort_t_restrain",i);
		if (hfc_param_search(buf, &value)) hfcmp_abort_t_restrain[i] = value;
	}
	
	/* "hfc<x>_tgtrst_restrain" */
	if (hfc_param_search("hfc_tgtrst_restrain", &value)) hfc_tgtrst_restrain = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_tgtrst_restrain[i] = -1;
		sprintf(buf,"hfc%d_tgtrst_restrain",i);
		if (hfc_param_search(buf, &value)) hfcmp_tgtrst_restrain[i] = value;
	}
	
	/* "hfc<x>_login_restrain" */
	if (hfc_param_search("hfc_login_restrain", &value)) hfc_login_restrain = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_login_restrain[i] = -1;
		sprintf(buf,"hfc%d_login_restrain",i);
		if (hfc_param_search(buf, &value)) hfcmp_login_restrain[i] = value;
	}

	/* "hfc<x>_lun_reset_delay" */	/* FCLNX-GPL-038 */
	if (hfc_param_search("hfc_lun_reset_delay", &value)) hfc_lun_reset_delay = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_lun_rst_delay[i] = -1;
		sprintf(buf,"hfc%d_lun_reset_delay",i);
		if (hfc_param_search(buf, &value)) hfcmp_lun_rst_delay[i] = value;
	}								/* FCLNX-GPL-038 */

	/* "hfc_pcie_sram_ce","hfc<x>_pcie_sram_ce" */
	if (hfc_param_search("hfc_pcie_sram_ce", &value)) hfc_pcie_sram_ce = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_pcie_sram_ce[i] = -1; /* Default Value */
		sprintf(buf,"hfc%d_pcie_sram_ce",i);
		if (hfc_param_search(buf, &value)) hfcmp_pcie_sram_ce[i] = value;
	}
	
	/* "hfc_pcie_sram_ce","hfc<x>_pcie_sram_ce_fx" */
	if (hfc_param_search("hfc_pcie_sram_ce_fx", &value)) hfc_pcie_sram_ce_fx = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_pcie_sram_ce_fx[i] = -1; /* Default Value */
		sprintf(buf,"hfc%d_pcie_sram_ce_fx",i);
		if (hfc_param_search(buf, &value)) hfcmp_pcie_sram_ce_fx[i] = value;
	}

	/* "hfc_core_ce","hfc<x>_core_ce" */
	if (hfc_param_search("hfc_core_ce", &value)) hfc_core_ce = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_core_ce[i] = -1; /* Default Value */
		sprintf(buf,"hfc%d_core_ce",i);
		if (hfc_param_search(buf, &value)) hfcmp_core_ce[i] = value;
	}
	
	/* "hfc_mck_point" */
	if (hfc_param_search("hfc_mck_point", &value)) hfc_mck_point = value;
	
	/* "hfc_msi_enable", "hfc<x>_msi_enable" */
	if (hfc_param_search("hfc_msi_enable", &value)) hfc_msi_enable = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_msi_enable[i] = -1;
		sprintf(buf,"hfc%d_msi_enable",i);
		if (hfc_param_search(buf, &value)) hfcmp_msi_enable[i] = value;
	}
	
	/* "hfc_inta_dummy_read", "hfc<x>_inta_dummy_read" */
	if (hfc_param_search("hfc_inta_dummy_read", &value)) hfc_inta_dummy_read = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_inta_dummy_read[i] = -1;
		sprintf(buf,"hfc%d_inta_dummy_read",i);
		if (hfc_param_search(buf, &value)) hfcmp_inta_dummy_read[i] = value;
	}

	/* "hfc_max_hwlog_cnt", "hfc<x>_max_hwlog_cnt " */
	if (hfc_param_search("hfc_max_hwlog_cnt", &value)) hfc_max_hwlog_cnt = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_max_hwlog_cnt[i] = -1;
		sprintf(buf,"hfc%d_max_hwlog_cnt",i);
		if (hfc_param_search(buf, &value)) hfcmp_max_hwlog_cnt[i] = value;
	}
	
	/* "hfc_debug_func", "hfc<x>_debug_func" */
	if (hfc_param_search("hfc_debug_func", &value)) hfc_debug_func = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_debug_func[i] = -1;
		sprintf(buf,"hfc%d_debug_func",i);
		if (hfc_param_search(buf, &value)) hfcmp_debug_func[i] = value;
	}
	
	/* "hfc_issue_d3hot", "hfc<x>_issue_d3hot" */	/* FCLNX-GPL-306 */
	if (hfc_param_search("hfc_issue_d3hot", &value)) hfc_issue_d3hot = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_issue_d3hot[i] = -1;
		sprintf(buf,"hfc%d_issue_d3hot",i);
		if (hfc_param_search(buf, &value)) hfcmp_issue_d3hot[i] = value;
	}												/* FCLNX-GPL-306 */
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* "hfc_sysfs_control", "hfc<x>_sysfs_control" */
	if (hfc_param_search("hfc_sysfs_control", &value)) hfc_sysfs_control = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_sysfs_control[i] = -1;
		sprintf(buf,"hfc%d_sysfs_control",i);
		if (hfc_param_search(buf, &value)) hfcmp_sysfs_control[i] = value;
	}
	
	/* "hfc_dev_loss_tmo", "hfc<x>_dev_loss_tmo" */		/* FCLNX-GPL-260 */
	if (hfc_param_search("hfc_dev_loss_tmo", &value)) hfc_dev_loss_tmo= value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_dev_loss_tmo[i] = -1;
		sprintf(buf,"hfc%d_dev_loss_tmo",i);
		if (hfc_param_search(buf, &value)) hfcmp_dev_loss_tmo[i] = value;
	}													/* FCLNX-GPL-260 */
	
	/* hfc_scan_finished_tmo */ /* FCLNX-GPL-565 */
	if (hfc_param_search("hfc_scan_finished_tmo", &value)) hfc_scan_finished_tmo= value;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	/* "hfc_rport_lu_scan", "hfc<x>_rport_lu_scan" */		/* FCLNX-GPL-575 */
	if (hfc_param_search("hfc_rport_lu_scan", &value)) hfc_rport_lu_scan= value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_rport_lu_scan[i] = -1;
		sprintf(buf,"hfc%d_rport_lu_scan",i);
		if (hfc_param_search(buf, &value)) hfcmp_rport_lu_scan[i] = value;
	}													/* FCLNX-GPL-575 */
	
	/* hfc_limit_log *//* FCLNX-GPL-491 */
	if (hfc_param_search("hfc_limit_log", &value)) hfc_limit_log = value;
	
	/* hfc_filter_target *//* FCLNX-GPL-491 */
	if (hfc_param_search("hfc_filter_target", &value)) hfc_filter_target = value;
	/* FCLNX-GPL-FX-478 >>> */
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_filter_target[i] = -1;
		sprintf(buf,"hfc%d_filter_target",i);
		if (hfc_param_search(buf, &value)) hfcmp_filter_target[i] = value;
	}
	/* >>> FCLNX-GPL-FX-478 */

	/* hfc_hg_stats_disable *//* FCLNX-GPL-494 */
	if (hfc_param_search("hfc_hg_stats_disable", &value)) hfc_hg_stats_disable = value;
	
	/* "hfc_hba_isolation" */								/* FCLNX-GPL-349 */
	if (hfc_param_search("hfc_hba_isolation", &value)) hfc_hba_isolation = value;
															/* FCLNX-GPL-349 */
	
	/* "hfc_npiv_enable" */								/* FCLNX-GPL-FX-137 */
	if (hfc_param_search("hfc_npiv_enable", &value)) hfc_npiv_enable = value;
														/* FCLNX-GPL-FX-137 */
	
	if (!( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info )){	/* FCLNX-GPL-349 */

		/* "hfc_ld_err_limit_s", "hfc<x>_ld_err_limit_s" */	/* FCLNX-GPL-349 */
		if (hfc_param_search("hfc_ld_err_limit_s", &value)) hfc_ld_err_limit_s = value;
		for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
			hfcmp_ld_err_limit_s[i] = -1;
			sprintf(buf,"hfc%d_ld_err_limit_s",i);
			if (hfc_param_search(buf, &value)) hfcmp_ld_err_limit_s[i] = value;
		}													/* FCLNX-GPL-349 */
	
		/* "hfc_if_err_limit", "hfc<x>_if_err_limit" */		/* FCLNX-GPL-349 */
		if (hfc_param_search("hfc_if_err_limit", &value)) hfc_if_err_limit = value;
		for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
			hfcmp_if_err_limit[i] = -1;
			sprintf(buf,"hfc%d_if_err_limit",i);
			if (hfc_param_search(buf, &value)) hfcmp_if_err_limit[i] = value;
		}													/* FCLNX-GPL-349 */
	
		/* "hfc_to_err_limit", "hfc<x>_to_err_limit" */		/* FCLNX-GPL-349 */
		if (hfc_param_search("hfc_to_err_limit", &value)) hfc_to_err_limit = value;
		for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
			hfcmp_to_err_limit[i] = -1;
			sprintf(buf,"hfc%d_to_err_limit",i);
			if (hfc_param_search(buf, &value)) hfcmp_to_err_limit[i] = value;
		}													/* FCLNX-GPL-349 */
	
		/* "hfc_to_reset_retry", "hfc<x>_to_reset_retry" */	/* FCLNX-GPL-349 */
		if (hfc_param_search("hfc_to_reset_retry", &value)) hfc_to_reset_retry = value;
		for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
			hfcmp_to_reset_retry[i] = -1;
			sprintf(buf,"hfc%d_to_reset_retry",i);
			if (hfc_param_search(buf, &value)) hfcmp_to_reset_retry[i] = value;
		}													/* FCLNX-GPL-349 */
	
		/* "hfc_rt_err_enable", "hfc<x>_rt_err_enable" */	/* FCLNX-GPL-349 */
		if (hfc_param_search("hfc_rt_err_enable", &value)) hfc_rt_err_enable = value;
		for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
			hfcmp_rt_err_enable[i] = -1;
			sprintf(buf,"hfc%d_rt_err_enable",i);
			if (hfc_param_search(buf, &value)) hfcmp_rt_err_enable[i] = value;
		}													/* FCLNX-GPL-349 */
	}														/* FCLNX-GPL-349 */
	
	/* "hfc_ctl_change_qdepth", "hfc<x>_ctl_change_qdepth" */		/* FCLNX-GPL-574 */
	if (hfc_param_search("hfc_ctl_change_qdepth", &value)) hfc_ctl_change_qdepth= value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_ctl_change_qdepth[i] = -1;
		sprintf(buf,"hfc%d_ctl_change_qdepth",i);
		if (hfc_param_search(buf, &value)) hfcmp_ctl_change_qdepth[i] = value;
	}													/* FCLNX-GPL-574 */
	
	/* "hfc_core_control", "hfc<x>_core_control" */
	if (hfc_param_search("hfc_core_control", &value)) hfc_core_control = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_core_control[i] = -1;
		sprintf(buf,"hfc%d_core_control",i);
		if (hfc_param_search(buf, &value)) hfcmp_core_control[i] = value;
	}
	
	/* "hfc_cc_cnt", "hfc<x>_cc_cnt" */
	if (hfc_param_search("hfc_cc_cnt", &value)) hfc_cc_cnt = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_cc_cnt[i] = -1;
		sprintf(buf,"hfc%d_cc_cnt",i);
		if (hfc_param_search(buf, &value)) hfcmp_cc_cnt[i] = value;
	}
	
	/* "hfc_cc_size", "hfc<x>_cc_size" */
	if (hfc_param_search("hfc_cc_size", &value)) hfc_cc_size = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_cc_size[i] = -1;
		sprintf(buf,"hfc%d_cc_size",i);
		if (hfc_param_search(buf, &value)) hfcmp_cc_size[i] = value;
	}
	
	/* "hfc_cc_core", "hfc<x>_cc_core" */
	if (hfc_param_search("hfc_cc_core", &value)) hfc_cc_core = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_cc_core[i] = -1;
		sprintf(buf,"hfc%d_cc_core",i);
		if (hfc_param_search(buf, &value)) hfcmp_cc_core[i] = value;
	}
	
	/* "hfc_link_reset", "hfc<x>_link_reset" */
	if (hfc_param_search("hfc_link_reset", &value)) hfc_link_reset = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_link_reset[i] = -1;
		sprintf(buf,"hfc%d_link_reset",i);
		if (hfc_param_search(buf, &value)) hfcmp_link_reset[i] = value;
	}
	
	/* "hfc_vport_count", "hfc<x>_vport_count" */
	if (hfc_param_search("hfc_vport_count", &value)) hfc_vport_count = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_vport_count[i] = -1;
		sprintf(buf,"hfc%d_vport_count",i);
		if (hfc_param_search(buf, &value)) hfcmp_vport_count[i] = value;
	}
	
	/* "hfc_frame_count", "hfc<x>_frame_count" */
	if (hfc_param_search("hfc_frame_count", &value)) hfc_frame_count = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_frame_count[i] = -1;
		sprintf(buf,"hfc%d_frame_count",i);
		if (hfc_param_search(buf, &value)) hfcmp_frame_count[i] = value;
	}
	
	/* "hfc_mq_num", "hfc<x>_mq_num" */
	if (hfc_param_search("hfc_mq_num", &value)) hfc_mq_num = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_mq_num[i] = -1;
		sprintf(buf,"hfc%d_mq_num",i);
		if (hfc_param_search(buf, &value)) hfcmp_mq_num[i] = value;
	}
	
	/* "hfc_rdtsc", "hfc<x>_rdtsc" */
	if (hfc_param_search("hfc_rdtsc", &value)) hfc_rdtsc = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_rdtsc[i] = -1;
		sprintf(buf,"hfc%d_rdtsc",i);
		if (hfc_param_search(buf, &value)) hfcmp_rdtsc[i] = value;
	}
	
	/* "hfc_intdisable", "hfc<x>_intdisable" */
	if (hfc_param_search("hfc_intdisable", &value)) hfc_intdisable = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_intdisable[i] = -1;
		sprintf(buf,"hfc%d_intdisable",i);
		if (hfc_param_search(buf, &value)) hfcmp_intdisable[i] = value;
	}
	
	/* "hfc_intenable", "hfc<x>_intenable" */
	if (hfc_param_search("hfc_intenable", &value)) hfc_intenable = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_intenable[i] = -1;
		sprintf(buf,"hfc%d_intenable",i);
		if (hfc_param_search(buf, &value)) hfcmp_intenable[i] = value;
	}
	
	/* "hfc_total_abort_to", "hfc<x>_total_abort_to" */
	if (hfc_param_search("hfc_total_abort_to", &value)) hfc_total_abort_to = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_total_abort_to[i] = -1;
		sprintf(buf,"hfc%d_total_abort_to",i);
		if (hfc_param_search(buf, &value)) hfcmp_total_abort_to[i] = value;
	}
	
	/* "hfc_total_tgtrst_to", "hfc<x>_total_tgtrst_to" */
	if (hfc_param_search("hfc_total_tgtrst_to", &value)) hfc_total_tgtrst_to = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_total_tgtrst_to[i] = -1;
		sprintf(buf,"hfc%d_total_tgtrst_to",i);
		if (hfc_param_search(buf, &value)) hfcmp_total_tgtrst_to[i] = value;
	}
	
	/* "hfc_max_io", "hfc<x>_maxio" *//* FCLNX-GPL-FX-147 */
	if (hfc_param_search("hfc_maxio", &value)) hfc_maxio = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_maxio[i] = -1;
		sprintf(buf,"hfc%d_maxio",i);
		if (hfc_param_search(buf, &value)) hfcmp_maxio[i] = value;
	}
	
	/* "hfc_login_seq_retry_cnt", "hfc<x>_login_seq_retry_cnt" *//* FCLNX-GPL-FX-463 */
	if (hfc_param_search("hfc_login_seq_retry_cnt", &value)) hfc_login_seq_retry_cnt = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_login_seq_retry_cnt[i] = -1;
		sprintf(buf,"hfc%d_login_seq_retry_cnt",i);
		if (hfc_param_search(buf, &value)) hfcmp_login_seq_retry_cnt[i] = value;
	}
	
	/* "hfc_mq_enable", "hfc<x>_mq_enable" */
	if (hfc_param_search("hfc_mq_enable", &value)) hfc_mq_enable = value;
	for (i=0;i<HFC_MAX_INSTANCE_CNT;i++) {
		hfcmp_mq_enable[i] = -1;
		sprintf(buf,"hfc%d_mq_enable",i);
		if (hfc_param_search(buf, &value)) hfcmp_mq_enable[i] = value;
	}
	
	/* "hfc_cpu_map" */
	if (hfc_param_search("hfc_cpu_map", &value)) hfc_cpu_map = value;
	
	HFC_EXIT("hfc_param_setup");
}



int hfc_param_search(char *search_str, int *value)
{
	int      hit=0;
	int      len;											/* FCLNX-GPL-467 */
	uint64_t str_value;
//	char     path[32];
	char     path[274];										/* FCLNX-GPL-467 */
	
	len = strlen(search_str);								/* FCLNX-GPL-467 */
	if (len > 272 ) {
		return (hit);
	}
	
	sprintf(path, search_str);
	if((hfclddconf!=NULL)&&(hfc_parse_string(hfclddconf, path, &str_value))) {
		hit = 1;
		*value = str_value;
		HFC_DBGPRT("hit %s = %d  in hfcldd.conf\n", search_str, *value);
	}
	
	return (hit);
}


int hfc_read_hfcbios(struct adap_info *ap)
{
	uchar disable_b[4];
	uchar drvctl[4];
	uint bios_ofs = 0xd8000 + (0x100 * ap->port_no);
	int i,entry;													/* FCLNX-0392 */
	uint wk;														/* FCLNX-0392 */
	uchar buf[16];													/* FCLNX-0392 */
																	/* @MLPF STR */
	uint wkint_bios_parm1;
	uint wkint_bios_parm2;

	if ( HFC_MMODE_CHECK_MLPF(ap) ) {								/* FCLNX-0365 */
		wkint_bios_parm1 = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_EFI_OP_TBL1, 0x4 );
		HFC_4L_TO_4B(wkint_bios_parm2,wkint_bios_parm1);
		HFC_MEMCPY(disable_b,(uchar*)&wkint_bios_parm2,4);
	}
	else{
		if( hfc_read_flash(ap, (bios_ofs+4), 4, disable_b) ){ /* FCLNX-GPL-116 */
			return(1);
		}
		/* FCLNX-GPL-FX-366 */
		if( hfc_read_flash(ap, (bios_ofs+0xa8), 4, drvctl) ){
			return(1);
		}
																	/* @MLPF END */
	}
	
	ap->automap  = hfc_automap;
	ap->narrowmap = hfc_narrowmap;									/* FCLNX-0392 */
	
	ap->defparam = 0;
	if ( (disable_b[1] != 0xFF) && (disable_b[1] != 0x00) ) {
		HFC_DBGPRT("Force bindings disable state\n");

		if (disable_b[1] & 0x80) {	/* Forced Persistent Binding into disable state */
			ap->automap = 1;
//			if (hfc_manage_info.hfcldd_mp_mod) hfc_manage_info.npubp->hfc_set_automap(ap);	/* FCLNX-GPL-0447 */
		}

		if (disable_b[1] & 0x40) {	/* Forced default parameter into disable state */
			ap->defparam = 1;
		}
	}
	else {
		HFC_DBGPRT("binding depends on etc.module.conf\n");
	}

	/* Get LOGIN delay time */
//#if defined(HFC_X8664_SLES11SP3) || defined(HFC_X8664_SLES12)
//	ap->login_wait = HFC_DEF_LOGIN_WAIT;	/* 0s */
//#else
	ap->login_wait = 2;	/* 2s */
//#endif
//	if (!ap->defparam) {
		if ( (disable_b[2] != 0xFF) && (disable_b[2] != 0x00) ) {
			HFC_DBGPRT("use LOGIN delay set by HBA BIOS\n");

			if(disable_b[2] & 0x80) {
				ap->login_wait = disable_b[2] & 0x7f;		/* FCLNX-0515 */
			}
		}
		else {
			HFC_DBGPRT("LOGIN delay time is set followed by modprobe.conf\n");
		}
//	}
	
	ap->linkspeed = 0;	/* Auto */
//	if (!ap->defparam) {
		/* Get Link Speed */
		if ( disable_b[0] != 0xFF ){
			HFC_DBGPRT("use Link Speed set by HBA BIOS\n");

			if ( hfc_chk_conf_ls(0,8,disable_b[0]) ) {
				ap->linkspeed = disable_b[0];
			}
			else {
				hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD1, NULL, 0) ;/* FCLNX-GPL-161 */
			}
		}
		else {
			HFC_DBGPRT("Link Speed is set followed by modprobe.conf\n");
		}
//	}

																	/* @MLPF STR */
	if ( HFC_MMODE_CHECK_MLPF(ap) )									/* FCLNX-0365 */
	{
		wkint_bios_parm1 = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_EFI_OP_TBL0, 0x4 );
		HFC_4L_TO_4B(wkint_bios_parm2,wkint_bios_parm1);
		HFC_MEMCPY(disable_b,(uchar*)&wkint_bios_parm2,4);
	}
	else {
		if( hfc_read_flash(ap, (bios_ofs+0), 4, disable_b) ){ /* FCLNX-GPL-116 */
			return(1);
		}
																	/* @MLPF END */
	}

	/* Get Connection Type */
	ap->topology = 0;	/* Auto */
//	if (!ap->defparam) {
		if(disable_b[3] != 0xFF) {
			HFC_DBGPRT("Sets Connection Type from BIOS\n");
			
			if((disable_b[3] == 0x00) || (disable_b[3] == 0x01) || (disable_b[3] == 0x03) ) {
				 ap->topology = disable_b[3];
			}
			else {
				 HFC_ERRPRT("hfcldd : 'Connection Type Parameter set in HBA BIOS' = %d is invalid.\n", disable_b[3]);
			}
		}
		else {
			HFC_DBGPRT("Connection Type depends on etc.module.conf\n");
		}
//	}
	
	/* Get Spinup Delay */														/* FCLNX-0404 */
	ap->spinup_delay = 0;			/* Disable */								/* FCLNX-0404 */
	if(disable_b[1] != 0xFF) {													/* FCLNX-0404 */
		HFC_DBGPRT("Spinup Delay from BIOS\n");									/* FCLNX-0404 */
		if((disable_b[1] & 0x20) == 0x20)										/* FCLNX-0404 */
			 ap->spinup_delay = 1;	/* Enable */								/* FCLNX-0404 */
	}																			/* FCLNX-0404 */
	
	HFC_DBGPRT("hfcldd%d : forced PB disable   = %d.\n",ap->dev_minor, ap->automap);
	HFC_DBGPRT("hfcldd%d : forced default parm = %d.\n",ap->dev_minor, ap->defparam);
	HFC_DBGPRT("hfcldd%d : login Delay time    = %d.\n",ap->dev_minor, ap->login_wait);
	HFC_DBGPRT("hfcldd%d : connection Type     = %d.\n",ap->dev_minor, ap->topology);
	HFC_DBGPRT("hfcldd%d : link speed          = %d.\n",ap->dev_minor, ap->linkspeed);
	HFC_DBGPRT("hfcldd%d : spinup delay        = %d.\n",ap->dev_minor, ap->spinup_delay);	/* FCLNX-0404 */
	
	/* Seve Boot Priority */
	memset(ap->boot_priority,0,sizeof(ap->boot_priority));						/* FCLNX-0392 */
	if (ap->narrowmap) {														/* FCLNX-0392 */

		if ( HFC_MMODE_CHECK_MLPF(ap) )											/* FCLNX-0410 */
		{
			wkint_bios_parm1 = (uint) hfc_read_hg_reg(ap, HFC_IOHGSPC_EFI_OP_TBL0, 0x4 );
			HFC_4L_TO_4B(wkint_bios_parm2,wkint_bios_parm1);
			HFC_MEMCPY(disable_b,(uchar*)&wkint_bios_parm2,4);
		}
		else {																	/* FCLNX-0410 */
			if( hfc_read_flash(ap, (bios_ofs+0), 4, disable_b) ){ /* FCLNX-GPL-116 */
				return(1); 
			}
		}

		for (i=0;i<4;i++) {	
			if (disable_b[i] != 0xff)
				break;
		}
		
		if ((i < sizeof(disable_b))				 /* Check Byte0-3!=ALL'F' */
			&& ((disable_b[1] & 0x90) == 0x90)) {/* Valid Boot Priority   */
			entry=0;
			
			for (entry=0;entry<8;entry++) {

				if ( HFC_MMODE_CHECK_MLPF(ap) )									/* FCLNX-0410 */
				{
					for (i=0;i<16;i++) {
						buf[i] = hfc_read_reg_hg_ext(ap,
									 ap->lparmode.hg_map->iosp.reg[HFC_IOHGSPC_EFI_OP_TBL0]
									 +(0x10*(entry+1))+i, 1);
					}
					
					/* Get WWPN */
					ap->boot_priority[entry].ww_name = (*(uint*)(&buf[12]));	/* Attention!! */
					ap->boot_priority[entry].ww_name <<= 32;					/* Attention!! */
					ap->boot_priority[entry].ww_name |= (*(uint*)(&buf[8]));	/* Attention!! */
					
					/* Get LUN */
					ap->boot_priority[entry].lun = buf[6];						/* Attention!! */
				}
				else {															/* FCLNX-0410 */
					if( hfc_read_flash(ap, (bios_ofs+0x10*(entry+1)), 16, buf) ){ /* FCLNX-GPL-116 */
						return(1);
					}
					
					/* Get WWPN */
					HFC_4B_TO_4L(wk, (*(uint*)(&buf[8])));
					ap->boot_priority[entry].ww_name = wk;
					ap->boot_priority[entry].ww_name <<= 32;
					HFC_4B_TO_4L(wk, (*(uint*)(&buf[12])));
					ap->boot_priority[entry].ww_name |= wk;

					/* Get LUN */
					ap->boot_priority[entry].lun = buf[7];
				}

			}
		}
	}																			/* FCLNX-0392 */
	/* FCLNX-GPL-FX-366 */
	if( HFC_MMODE_CHECK_SHARED(ap) && !(HFC_MMODE_CHECK_SHADOW(ap) ) ) {
		ap->drvctl = drvctl[0];
	}
	
	return(0);
}


char hfc_cnvc(char C)										  /* FCWIN-0081 */
{
	char c = C;
	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 'a');

	return c;
}



int hfc_parse_string(char *string, char *keyword, uint64_t *value)/* FCWIN-0081 */
{
	char *cptr,*kptr;
	int index;
	int stringlen=0,keywordlen=0;

	/* Get string's length */
	cptr = string;
	while (*cptr) {
		cptr++;
		stringlen++;
	}

	/* Get keyword length */
	cptr = keyword;
	while (*cptr) {
		cptr++;
		keywordlen++;
	}

	if (keywordlen > stringlen) {

		/* keyword length > string length */
		/*  That means this string does not include the specified keyword */ 
		return 0;
	}

	/* Now setup and start the compare */
	cptr = string;

ContinueSearch:

	/* Skip spaces and tabs */
	while (*cptr == ' ' || *cptr == '\t') {
		cptr++;
	}

	if (*cptr == '\0') {

		/* End of this string */
		return 0;
	}

	kptr = keyword;
	while (hfc_cnvc(*cptr) == hfc_cnvc(*kptr)) {
		
		cptr++;
		kptr++;

		if (*(cptr - 1) == '\0') {

			/* End of string */
	
			return 0;
		}
	}

	cptr++;
	kptr++;

	if (*(kptr - 1) == '\0') {

		/* May have a match backup and check for blank or equals */


		cptr--;
		while (*cptr == ' ' || *cptr == '\t') {
			cptr++;
		}

		/* Found a match.  Make sure there are equals */

		if (*cptr != '=') {

			/* Move to the next semicolon */

			while (*cptr) {
				if (*cptr++ == ';') {
					goto ContinueSearch;
				}
			}
			return 0;
		}

		/* Skip the equals sign */

		cptr++;

		/* Skip white space */

		while ((*cptr == ' ') || (*cptr == '\t')) {
			cptr++;
		}

		if (*cptr == '\0') {

			/* Early end of string, return not found */

			return 0;
		}

		if (*cptr == ';') {

			/* This isn't it either */

			cptr++;
			goto ContinueSearch;
		}

		*value = 0;
		if ((*cptr == '0') && (hfc_cnvc(*(cptr + 1)) == 'x')) {

			/* Value is in Hex.  Skip the "0x" */

			cptr += 2;
			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ':') { /* changed */
					 break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					*value = (16 * (*value)) + (*(cptr + index) - '0');
				} else {
					if ((hfc_cnvc(*(cptr + index)) >= 'a') && (hfc_cnvc(*(cptr + index)) <= 'f')) {
						*value = (16 * (*value)) + (hfc_cnvc(*(cptr + index)) - 'a') + 10;
					} else {

						/* Syntax error, return not found */

						return 0;
					}
				}
			}
		} else {

			/* Value is in decimal */

			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ':') {  /* changed */
					break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					*value = (10 * (*value)) + (*(cptr + index) - '0');
				} else {

					/* Syntax error return not found */

					return 0;
				}
			}
		}

		return 1;
	} else {

		/* Not a match check for ';' to continue search */

        while (*cptr) {
            if (*cptr++ == ':') {	/* changed */
                goto ContinueSearch;
            }
        }

        return 0;
    }
}



int hfc_convert_string(char *string, uint64_t *value)
{
	char *cptr;
	int index;

	cptr = string;

		*value = 0;
		if ((*cptr == '0') && (hfc_cnvc(*(cptr + 1)) == 'x')) {

			/* Value is in Hex.  Skip the "0x" */

			cptr += 2;
			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ':') { /* changed */
					 break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					*value = (16 * (*value)) + (*(cptr + index) - '0');
				} else {
					if ((hfc_cnvc(*(cptr + index)) >= 'a') && (hfc_cnvc(*(cptr + index)) <= 'f')) {
						*value = (16 * (*value)) + (hfc_cnvc(*(cptr + index)) - 'a') + 10;
					} else {

						/* Syntax error, return not found */

						return 0;
					}
				}
			}
		} else {

			/* Value is in Decimal */

			for (index = 0; *(cptr + index); index++) {

				if (*(cptr + index) == ' ' ||
					*(cptr + index) == '\t' ||
					*(cptr + index) == ':') {  /* changed */
					break;
				}

				if ((*(cptr + index) >= '0') && (*(cptr + index) <= '9')) {
					*value = (10 * (*value)) + (*(cptr + index) - '0');
				} else {

					/* Syntax error return not found */

					return 0;
				}
			}
		}

		return 1;

}

char *hfc_delete_space(char *string){	/* FCLNX-GPL-311 */
	
	char	*cptr = NULL;
	int		index = 0;
	
	cptr = string;
	
	while (*cptr) {
		cptr++;
		index++;
	}

	cptr = string;
	
	while  ((index != 0) && ((*(cptr+index) == ' ') || (*(cptr+index) == '\0'))){
		if (*(cptr+index) == ' ')
			*(cptr+index)  = '\0';
		index--;
	}
	return(string);
}										/* FCLNX-GPL-311 */

#if 0		/* FCLNX-GPL-0449 */
char *hfc_get_write_retries(void)
{
	return (write_retries);
}
#endif

void hfc_set_pub_symbol_list(void)
{

	struct pub_symbol_list		*pubp;
	
	HFC_ENTRY("hfc_set_pub_symbol_list");
	
	pubp  = hfc_get_pub_symbol_list();
	
	/* detect */
	pubp->hfc_param_search				=	hfc_param_search;
	pubp->hfc_cnvc						=	hfc_cnvc;
	pubp->hfc_parse_string				=	hfc_parse_string;
	pubp->hfc_convert_string			=	hfc_convert_string;
	pubp->hfc_initialize				=	hfc_initialize;				/* FCLNX-0526 */
	pubp->hfc_chk_stop					=	hfc_chk_stop;				/* FCLNX-0534 */
	pubp->hfc_issue_forced_mck			=	hfc_issue_forced_mck;		/* FCLNX-0534 */
	pubp->hfc_chk_conf_val				=	hfc_chk_conf_val;
	pubp->hfc_config_hw_set_five		=	hfc_config_hw_set_five;
	
	/* strategy */
	pubp->hfc_strategy_pg				=	hfc_strategy_pg;
	pubp->hfc_eh_abort_pg				=	hfc_eh_abort_pg;
	pubp->hfc_eh_device_reset_pg		=	hfc_eh_device_reset_pg;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	pubp->hfc_eh_target_reset_pg		=	hfc_eh_target_reset_pg;		/* FCLNX-GPL-0343 */
#endif
	pubp->hfc_eh_bus_reset_pg			=	hfc_eh_bus_reset_pg;
	pubp->hfc_iodone					=	hfc_iodone;
	pubp->hfc_strategy_hfcp				=	hfc_strategy_hfcp;
	pubp->hfc_get_new_hfcp				=	hfc_get_new_hfcp;
	pubp->hfc_start						=	hfc_start;					/* FCLNX-0429 */
	pubp->hfc_cancel_scsi_cmd			=	hfc_cancel_scsi_cmd;		/* FCLNX-0429 */
	pubp->hfc_get_new_cmnd				=	hfc_get_new_cmnd;
	pubp->hfc_dummy_copy				=	hfc_dummy_copy;
	pubp->hfc_enqueue_wx_que			=	hfc_enqueue_wx_que;			/* FCLNX-GPL-FX-486 */
	pubp->hfc_enque_next_dstart			=	hfc_enque_next_dstart;		/* FCLNX-GPL-FX-486 */
	
	/* timer_recovery*/
	pubp->hfc_reset_adap_info			=	hfc_reset_adap_info;
	pubp->hfc_reset_start				= 	hfc_reset_start;
	pubp->hfc_errlog					=	hfc_errlog;
	pubp->hfc_watchdog					= 	hfc_watchdog;
	pubp->hfc_force_linkdown			=	hfc_force_linkdown;
	pubp->hfc_force_linkdown_recovery	=	hfc_force_linkdown_recovery;

	/* top */
	pubp->hfc_hash_target_valid			=	hfc_hash_target_valid;
	pubp->hfc_hash_target_info			=	hfc_hash_target_info;
	pubp->hfc_hash_target_info_wwn		=	hfc_hash_target_info_wwn;
	pubp->hfc_read_tbl					=	hfc_read_tbl;
	pubp->hfc_issue_relogin				=	hfc_issue_relogin;			/* FCLNX-0429 */
	pubp->hfc_enque_login_req			=	hfc_enque_login_req;		/* FCLNX-0429 */
	pubp->lock_mailbox					=	lock_mailbox;				/* FCLNX-0526 */
	pubp->unlock_mailbox				=	unlock_mailbox;				/* FCLNX-0526 */
	pubp->hfc_write_reg_ext				=	hfc_write_reg_ext;			/* FCLNX-0526 */
	pubp->hfc_read_reg_ext				=	hfc_read_reg_ext;
	pubp->hfc_trace						=	hfc_trace;
	pubp->hfc_watchdog_enter			= 	hfc_watchdog_enter;
	pubp->hfc_search_dev_info			=	hfc_search_dev_info;		/* FCLNX-GPL-449 */
	pubp->hfc_kmalloc					=	hfc_kmalloc;				/* FCLNX-GPL-204 */
	pubp->hfc_kfree						=	hfc_kfree;					/* FCLNX-GPL-204 */
	pubp->hfc_mp_watchdog_enter			=	hfc_mp_watchdog_enter;		/* FCLNX-GPL-471 */
	
	/* ioctl */
	pubp->hfc_sciocmd					=	hfc_sciocmd;
	pubp->hfc_ioctl_iodone				=	hfc_ioctl_iodone;
	pubp->structdump					=	structdump;

	/* mlpf */
	pubp->hfc_read_reg_hg_ext			=	hfc_read_reg_hg_ext;		/* FCLNX-GPL-451 */
	pubp->hfc_write_reg_hg_ext			=	hfc_write_reg_hg_ext;		/* FCLNX-GPL-451 */

	pubp->hfc_reset_all_timer			=	hfc_reset_all_timer;
	pubp->_hfc_wake_up					=	_hfc_wake_up;
	
	
	/* detect_fx */
	pubp->hfc_fx_param_search				=	hfc_fx_param_search;
	pubp->hfc_fx_cnvc						=	hfc_fx_cnvc;
	pubp->hfc_fx_parse_string				=	hfc_fx_parse_string;
	pubp->hfc_fx_convert_string				=	hfc_fx_convert_string;
	pubp->hfc_fx_initialize					=	hfc_fx_initialize;
	pubp->hfc_fx_chk_stop					=	hfc_fx_chk_stop;
	pubp->hfc_fx_issue_forced_mck			=	hfc_fx_issue_forced_mck;
	pubp->hfc_chk_conf_val					=	hfc_chk_conf_val;
	pubp->hfc_fx_config_hw_set_five_fx		=	hfc_fx_config_hw_set_five_fx;
	
	/* strategy_fx */
	pubp->hfc_fx_strategy_pg				=	hfc_fx_strategy_pg;
	pubp->hfc_fx_eh_abort_pg				=	hfc_fx_eh_abort_pg;
	pubp->hfc_fx_eh_device_reset_pg			=	hfc_fx_eh_device_reset_pg;
	pubp->hfc_fx_eh_target_reset_pg			=	hfc_fx_eh_target_reset_pg;
	pubp->hfc_fx_eh_bus_reset_pg			=	hfc_fx_eh_bus_reset_pg;
	pubp->hfc_fx_set_cmnd_res				=	hfc_fx_set_cmnd_res;
	pubp->hfc_fx_iodone						=	hfc_fx_iodone;
	pubp->hfc_fx_strategy_port				=	hfc_fx_strategy_port;
	pubp->hfc_fx_strategy_core				=	hfc_fx_strategy_core;
	pubp->hfc_fx_get_new_hfcp				=	hfc_fx_get_new_hfcp;
	pubp->hfc_fx_start						=	hfc_fx_start;
	pubp->hfc_fx_cancel_scsi_cmd			=	hfc_fx_cancel_scsi_cmd;
	pubp->hfc_fx_get_new_cmnd				=	hfc_fx_get_new_cmnd;
	pubp->hfc_fx_dummy_copy					=	hfc_fx_dummy_copy;
	pubp->hfc_fx_issue_devrst_cscsi			=	hfc_fx_issue_devrst_cscsi;
	pubp->hfc_fx_issue_tgtrst_cscsi			=	hfc_fx_issue_tgtrst_cscsi;
	pubp->hfc_fx_enque_next_dstart			=	hfc_fx_enque_next_dstart;	/* FCLNX-GPL-FX-387 */
	pubp->hfc_fx_enqueue_wx_que				=	hfc_fx_enqueue_wx_que;		/* FCLNX-GPL-FX-387 */
	
	/* timer_recovery_fx */
	pubp->hfc_fx_reset_port_info			=	hfc_fx_reset_port_info;
	pubp->hfc_fx_reset_start				= 	hfc_fx_reset_start;
	pubp->hfc_fx_errlog						=	hfc_fx_errlog;
	pubp->hfc_fx_watchdog					= 	hfc_fx_watchdog;
	pubp->hfc_fx_force_linkdown				=	hfc_fx_force_linkdown;
	pubp->hfc_fx_force_linkdown_recovery	=	hfc_fx_force_linkdown_recovery;

	/* top_fx */
	pubp->hfc_fx_hash_target_valid			=	hfc_fx_hash_target_valid;
	pubp->hfc_fx_hash_target_info			=	hfc_fx_hash_target_info;
	pubp->hfc_fx_hash_target_info_wwn		=	hfc_fx_hash_target_info_wwn;
	pubp->hfc_fx_read_tbl					=	hfc_fx_read_tbl;
	pubp->hfc_fx_enque_plogi_req			=	hfc_fx_enque_plogi_req;
	pubp->hfc_fx_enque_prli_req				=	hfc_fx_enque_prli_req;
	pubp->lock_mailbox						=	lock_mailbox;
	pubp->unlock_mailbox					=	unlock_mailbox;
	pubp->hfc_fx_write_reg_ext				=	hfc_fx_write_reg_ext;
	pubp->hfc_fx_read_reg_ext				=	hfc_fx_read_reg_ext;
	pubp->hfc_fx_trace						=	hfc_fx_trace;
	pubp->hfc_fx_watchdog_enter				= 	hfc_fx_watchdog_enter;
	pubp->hfc_fx_search_dev_info			=	hfc_fx_search_dev_info;
	pubp->hfc_fx_kmalloc					=	hfc_fx_kmalloc;
	pubp->hfc_fx_kfree						=	hfc_fx_kfree;
	pubp->hfc_fx_mp_watchdog_enter			=	hfc_fx_mp_watchdog_enter;
	
	/* ioctl_fx */
	pubp->hfc_fx_sciocmd					=	hfc_fx_sciocmd;
	pubp->hfc_fx_ioctl_iodone				=	hfc_fx_ioctl_iodone;
	
	/* mlpf_fx */
	pubp->hfc_fx_read_reg_hg_ext			=	hfc_fx_read_reg_hg_ext;		/* FCLNX-GPL-451 */
	pubp->hfc_fx_write_reg_hg_ext			=	hfc_fx_write_reg_hg_ext;		/* FCLNX-GPL-451 */

	pubp->hfc_fx_reset_all_timer			=	hfc_fx_reset_all_timer;
	pubp->_hfc_fx_wake_up					=	_hfc_fx_wake_up;
	
	
	HFC_EXIT("hfc_set_pub_symbol_list");
	

}



int hfc_check_nonpub_symbol(void)
{
	int	rtn = 0;
	
	HFC_ENTRY("hfc_check_nonpub_symbol");
	
	/* detect */
	if (hfc_manage_info.npubp->hfc_get_mp_manage_info == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_check_hop == NULL)						rtn++;	/* FCLNX-0429 */
	if (hfc_manage_info.npubp->hfc_mp_scan_dev == NULL)						rtn++;	/* FCLNX-0521 */
	if (hfc_manage_info.npubp->hfc_make_lgpath == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_remove_lgpath == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_update_lgtarget == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_update_lgdev == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_search_and_update_target == NULL)		rtn++;
	if (hfc_manage_info.npubp->hfc_search_and_update_dev == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_search_dev_compare == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_automap_lgpath == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_mp_scsi_host_rescan == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_free_mp_table == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_wait_mp_ioend == NULL)					rtn++;	/* FCLNX-0459 */
	if (hfc_manage_info.npubp->hfc_write_retries == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_update_attribute == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_proc_info_lun == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_proc_info_option == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fo_check_and_offline == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_forced_offline_e == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_forced_offline_c == NULL)				rtn++;	/* FCLNX-0147 */
	if (hfc_manage_info.npubp->hfc_mp_queue_depth == NULL)					rtn++;	/* FCLNX-0521 */
	if (hfc_manage_info.npubp->hfc_isolparam_setup == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_isolconf_setup == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_allocate_errcnt_info == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_free_errcnt_info == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_set_retry_cnt == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_mp_module_init == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_set_parm == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_scsi_host_alloc == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_scsi_add_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_scsi_scan_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_scsi_remove_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_free_and_allocate_errcnt_info == NULL)	rtn++;	/* FCLNX-GPL-FX-314 */

	/* strategy */
	if (hfc_manage_info.npubp->hfc_mp_strategy == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_abort == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_device_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_mp_target_reset == NULL)					rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_mp_bus_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_retry_strategy == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_ioerror_check == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_queue_check == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_queue_count == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_iodone == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_check_io_reset_complete == NULL)			rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_check_dev_reset_complete == NULL)		rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_check_bus_reset_complete == NULL)		rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_make_fcinfo == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_set_scsi_cmd_tmr == NULL)				rtn++;	/* FCLNX-GPL-0449 */
	
	/* top */
	if (hfc_manage_info.npubp->hfc_convert_rptluns == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_check_luconfig == NULL)					rtn++;	/* FCLNX-611 */
	
	/* timer_recovery */
	if (hfc_manage_info.npubp->hfc_errlog_mp == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_watched_errcount == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_clear_errinfo == NULL)					rtn++;	/* FCLNX-0147 */
	
	/* ioctl */
	if (hfc_manage_info.npubp->hfc_ioctl_mp == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_rd_param == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_wr_param == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_lu_map == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_setpath == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_mp_path_health == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_mp_lgtarget_map == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_mp_lgpath_info1 == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_read_isolparam == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_read_retry_cnt == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_get_isolparam == NULL)					rtn++;	/* FCLNX-0147 */

	/* diag */
	if (hfc_manage_info.npubp->read_dev_info_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->read_lg_target_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_dev_info_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info1_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info2_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_failover_info_mp == NULL)				rtn++;

	if (hfc_manage_info.npubp->hfc_mp_proc_info_pfb == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_info_pfb == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_proc_info_pfb_com == NULL)			rtn++;	/* FCLNX-GPL-FX-481 */
	if (hfc_manage_info.npubp->hfc_mp_search_pfb_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_strategy_pfb == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_abort_pfb == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_device_reset_pfb == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_mp_lun_reset == NULL)					rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_mp_target_reset_pfb == NULL)				rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_mp_bus_reset_pfb == NULL)				rtn++;	/* FCLNX-GPL-204 */
	
	/* version */
	if (hfc_manage_info.npubp->hfc_get_pcm_rcsid == NULL)					rtn++;
	
	
	/* detect_fx */
	if (hfc_manage_info.npubp->hfc_fx_mp_scan_dev == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_make_lgpath == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_remove_lgpath == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_host_rescan == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_wait_mp_ioend == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_write_retries == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_update_attribute == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_proc_info_lun == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_proc_info_option == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_fo_check_and_offline == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_forced_offline_e == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_forced_offline_c == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_queue_depth == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_isolconf_setup == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_allocate_errcnt_info == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_free_errcnt_info == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_set_retry_cnt == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_set_parm == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_host_alloc == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_add_host == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_scan_host == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_remove_host == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_mp_enable == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_slave_alloc == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_slave_destroy == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_change_queue_depth == NULL)		rtn++;
	if (hfc_manage_info.npubp->hfc_fx_free_and_allocate_errcnt_info == NULL)rtn++;	/* FCLNX-GPL-FX-314 */
	
	/* strategy_fx */
	if (hfc_manage_info.npubp->hfc_fx_mp_strategy == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_abort == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_device_reset == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_target_reset == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_bus_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_retry_strategy == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_ioerror_check == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_queue_check == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_queue_count == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_iodone == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_issuing_io_reset == NULL)		rtn++;	/* FCLNX-GPL-FX-333 */
	if (hfc_manage_info.npubp->hfc_fx_check_io_reset_complete == NULL)		rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete == NULL)		rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete == NULL)		rtn++;
	if (hfc_manage_info.npubp->hfc_fx_make_fcinfo == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_set_scsi_cmd_tmr == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_set_cmnd_res == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_intr_xrb == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_tmp_dev_check == NULL)				rtn++; /* FCLNX-GPL-FX-269 */
	
	/* top_fx */
	if (hfc_manage_info.npubp->hfc_fx_convert_rptluns == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_luconfig == NULL)				rtn++;
	
	/* timer_recovery_fx */
	if (hfc_manage_info.npubp->hfc_fx_errlog_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_watched_errcount == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_clear_errinfo == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_deque_retry_hfcp == NULL)				rtn++;
	
	/* ioctl_fx */
	if (hfc_manage_info.npubp->hfc_fx_ioctl_mp == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_rd_param == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_wr_param == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lu_map == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_setpath == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_path_health == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lgtarget_map == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lgpath_info1 == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_read_isolparam == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_read_retry_cnt == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_get_isolparam == NULL)				rtn++;

	/* diag_fx */
	if (hfc_manage_info.npubp->read_fx_dev_info_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->read_fx_lg_target_info_mp == NULL)			rtn++;
	if (hfc_manage_info.npubp->read_fx_lg_dev_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_fx_lg_path_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_fx_lg_path_info1_mp == NULL)			rtn++;
	if (hfc_manage_info.npubp->read_fx_lg_path_info2_mp == NULL)			rtn++;
	if (hfc_manage_info.npubp->read_fx_failover_info_mp == NULL)			rtn++;

	if (hfc_manage_info.npubp->hfc_fx_mp_proc_info_pfb == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_proc_info_pfb_com == NULL)			rtn++;	/* FCLNX-GPL-FX-481 */
	if (hfc_manage_info.npubp->hfc_fx_mp_strategy_pfb == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lun_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_target_reset_pfb == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_bus_reset_pfb == NULL)				rtn++;
	
	/* version_fx */
	if (hfc_manage_info.npubp->hfc_fx_get_pcm_rcsid == NULL)				rtn++;
	
	
	HFC_DBGPRT("hfc_check_nonpub_symbol() -- null symbol = %d\n", rtn);
	HFC_EXIT("hfc_check_nonpub_symbol");	
	if ( rtn != 0 ){
		HFC_DBGPRT("hfc_check_nonpub_symbol() -- null symbol = %d\n", rtn);
	}
	else
	{
		HFC_INFPRT("Success at loading hfcldd_mp module.\n");
	}
	return (rtn);
}

/* Alocate a dev_info for a lun *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0369 */
static int
hfc_slave_alloc(struct scsi_device *sdev)
{
	struct adap_info	*ap = NULL;
	struct dev_info		*dev = NULL;
	struct target_info	*target = NULL;
	ulong				flags = 0;
	
	ap = (struct adap_info *)sdev->host->hostdata;
	
	if (ap != NULL) {
		if ( (!strcmp(ap->name, "port_info")) || (!strcmp(ap->name, "lport_info")) ) {
			/* FIVE-FX */
			return (hfc_fx_slave_alloc(sdev));
		}
	}
	
	HFC_DBGPRT("hfc_slave_alloc scsi (%d:%d:%d:%d)\n",
		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		ap = (struct adap_info *)sdev->host->hostdata;
		if( ap == NULL ) return(0);

		HFC_ADAPLOCK_IRQSAVE(flags);
		target = hfc_hash_target_info( ap, (uint)sdev->id );
		if( target != NULL){
			dev = (struct dev_info *)hfc_get_dev_info(target, sdev->lun);
			if( dev == NULL ){
				dev = (struct dev_info *)hfc_kmalloc(ap, sizeof(struct dev_info), GFP_ATOMIC);
				if(dev == NULL){
					hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x3C, NULL, 0) ;
				}
				else{
					memset( dev, 0, sizeof(struct dev_info) );
					dev->target_id = sdev->id;
					dev->lun = sdev->lun;
			
					sdev->hostdata = dev;
			
					if (target->dev == NULL) {
						target->dev   = dev;
						dev->prev = NULL;
						dev->next = NULL;
					}
					else {
						struct dev_info *term_dev = target->dev;
				
						while (term_dev->next != NULL)
							term_dev = term_dev->next;

						term_dev->next = dev;
						dev->prev = term_dev;
						dev->next = NULL;
					}
					dev->target = target;
				}
			}
			else{
				sdev->hostdata = dev;
			}
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
	}

	return 0;
}

/* Release a dev_info when the lun does not exist *//* FCLNX-GPL-0343 */
static void
hfc_slave_destroy(struct scsi_device *sdev)
{
	struct adap_info	*ap = NULL;
	struct dev_info		*dev = NULL, *temp_dev = NULL;
	struct target_info	*target = NULL;
	ulong				flags = 0;
	
	ap = (struct adap_info *)sdev->host->hostdata;
	
	if (ap != NULL) {
		if ( (!strcmp(ap->name, "port_info")) || (!strcmp(ap->name, "lport_info")) ) {
			/* FIVE-FX */
			hfc_fx_slave_destroy(sdev);
			return;
		}
	}
	
	HFC_DBGPRT("hfc_slave_destroy scsi (%d:%d:%d:%d)\n",
		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		ap = (struct adap_info *)sdev->host->hostdata;
		if( ap == NULL ) return;

		dev = (struct dev_info *)sdev->hostdata;
		if( dev != NULL ){
			if( !test_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags) ){
				HFC_ADAPLOCK_IRQSAVE(flags);
				target = hfc_hash_target_valid( ap, (uint)sdev->id );
				if( target != NULL){
					temp_dev = target->dev;
					while( temp_dev != NULL ){
						if( temp_dev == dev )
							break;
						temp_dev = temp_dev->next;
					}
					if( temp_dev != NULL){
						if( temp_dev == target->dev ){
							target->dev = temp_dev->next;
						}
						else{
							temp_dev = temp_dev->prev;
							if( temp_dev != NULL){
								temp_dev->next = dev->next;
							}
						}
					}
				}
				memset( dev, 0, sizeof(struct dev_info) );
				hfc_kfree(ap, sdev->hostdata);
				sdev->hostdata = NULL;
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
			}
		}
	}
	return; 
}

/* FCLNX-GPL-565 start *//* FCLNX-GPL-575 start */
static void
hfc_lu_scan_start(struct adap_info *ap)
{
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	int i;
	ulong flags = 0;
	struct target_info	*target=NULL;
	
	if ( test_bit(HFC_SYSFS_RPORT, (ulong *)&ap->sysfs_control) ) {
		if ( !hfc_manage_info.hfcldd_mp_mod ) {		/*FCLNX-GPL-315*/
			hfc_start_rport(ap); /* FCLNX-GPL-306 *//* FCLNX-GPL-310 */
			
			HFC_ADAPLOCK_IRQSAVE(flags);					/* FCLNX-GPL-397 */
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				target = hfc_hash_target_info(ap, i);
				if( target != NULL ){
					set_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
				}
			}
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			atomic_set(&ap->rport_event_wait, 1);
			wake_up_interruptible(&ap->rport_event);		/* FCLNX-GPL-397 */
		}
	}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	return;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
static int
hfc_scan_finished(struct Scsi_Host *shost, unsigned long time)
{
	struct adap_info *ap=NULL;
	
	if( shost == NULL )
	{	/* Incorrect pointer */
		return 1; 
	}
	
	ap = (struct adap_info *)shost->hostdata;
	if( ap == NULL )
	{	/* Incorrect pointer */
		return 1; 
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_scan_finished(shost,time));
	}
	
	if( time >= ap->scan_finished_tmo * HZ )
	{	/* Time out */
		return 1; 
	}
	/* [Note]                                                             */
	/* This checking is secure from a wraparound of "(ulong)jiffies".     */
	/* Reason : We use "time" value. time = (ulong)jiffies - (ulong)start */
	/* Example: time = (ulong)0x07 - (ulong)0xfffffffe = (ulong)0x09      */
	
	
	if( atomic_read(&ap->rport_event_wait) == 0 )
	{	/* Scan Finished. */
		return 1; 
	}

	return 0;
}

static void
hfc_scan_start(struct Scsi_Host *shost)
{
	struct adap_info    *ap    =NULL;
	
	HFC_ENTRY("hfc_scan_start");
	
	if( shost == NULL )
	{	/* Incorrect pointer */
		return;
	}
	
	ap = (struct adap_info *)shost->hostdata;
	if( ap == NULL )
	{	/* Incorrect pointer */
		return;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_scan_start(shost);
		return;
	}
	
	hfc_lu_scan_start(ap);
	
	HFC_EXIT("hfc_scan_start");
	
	return;
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
/* FCLNX-GPL-565 end *//* FCLNX-GPL-575 end */


static int
hfc_slave_configure(struct scsi_device *sdev)
{
	/* FCLNX-GPL-255 start */
	struct adap_info *ap=NULL;
	int queue_depth, qd;
	struct dev_info	 *dev=NULL;
	ulong			 flags = 0;
	struct request_queue	*rq=NULL;	/* FCLNX-GPL-409 */
	struct fc_rport *rport=NULL;	/* FCLNX-GPL-FX-472 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		rport = starget_to_rport(sdev->sdev_target);
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
#endif
	
	ap = (struct adap_info *)sdev->host->hostdata;
	
	if ( (!strcmp(ap->name, "port_info")) || (!strcmp(ap->name, "lport_info")) ) {
		/* FIVE-FX */
		return (hfc_fx_slave_configure(sdev));
	}
	
	HFC_DBGPRT("hfc_slave_configure scsi (%d:%d:%d:%d)\n",
		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	dev = (struct dev_info *)sdev->hostdata;
//	rport = dev->target->rport;
	
	if (hfc_manage_info.hfcldd_mp_mod) {										/* FCLNX-GPL-204 */
		if (hfc_manage_info.npubp->hfc_mp_queue_depth(sdev, &qd)) {				/* FCLNX-0521 */
			queue_depth = qd;
		}
		else {
			/* hfcldd_mp ON & HFC-PCM OFF */
//			ap = (struct adap_info *)sdev->host->hostdata;
			queue_depth = ap->queue_depth;
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
				if(rport != NULL)
					rport->dev_loss_tmo = ap->dev_loss_tmo;
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
#endif
		}
	}
	else {
		/* hfcldd_mp OFF */
//		ap = (struct adap_info *)sdev->host->hostdata;
		queue_depth = ap->queue_depth;
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
			if(rport != NULL)
				rport->dev_loss_tmo = ap->dev_loss_tmo;
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
#endif
	}
	/* FCLNX-GPL-255 end */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	if(sdev->tagged_supported) {
		scsi_activate_tcq(sdev, queue_depth);
	}
	else{
		scsi_deactivate_tcq(sdev, queue_depth);
	}
#else
	scsi_change_queue_depth(sdev, queue_depth);
#endif /* FCLNX-GPL-FX-496 end */

	if (( hfc_manage_info.lg_target_info == NULL ) || (!hfc_manage_info.hfcldd_mp_mod)) {
		/* Validate the dev_info since lun was recognized *//* FCLNX-GPL-0343 */
		HFC_ADAPLOCK_IRQSAVE(flags);
		dev = (struct dev_info *)sdev->hostdata;
		set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);					/* FCLNX-GPL-0343 */
	}
	rq = sdev->request_queue;								/* FCLNX-GPL-409 */
	if( rq != NULL ){
		blk_queue_rq_timed_out( rq, NULL );
	}														/* FCLNX-GPL-409 */

	HFC_EXIT("hfc_slave_configure");

	return (0);
}

#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
static int hfc_change_queue_depth(struct scsi_device *sdev, int qdepth, int reason)	/* FCLNX-GPL-450 */
#else
static int hfc_change_queue_depth(struct scsi_device *sdev, int qdepth)	
#endif
{
	struct adap_info	*ap=NULL;		/* FCLNX-GPL-574 */
	
	ap = (struct adap_info *)sdev->host->hostdata;

	if (ap != NULL) {
		if ( (!strcmp(ap->name, "port_info")) || (!strcmp(ap->name, "lport_info")) ) {
			/* FIVE-FX */
#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
			return (hfc_fx_change_queue_depth(sdev, qdepth, reason));
#else
			return (hfc_fx_change_queue_depth(sdev, qdepth));
#endif
		}
		else {
#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
			if( (ap->ctl_change_qdepth)&&(reason != SCSI_QDEPTH_DEFAULT) ){
				return -EOPNOTSUPP;
			}
#endif
		}
	}									/* FCLNX-GPL-574 */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), qdepth);
#else
	scsi_change_queue_depth(sdev, qdepth);
#endif /* FCLNX-GPL-FX-496 end */

	return sdev->queue_depth;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
static int hfc_change_queue_type(struct scsi_device *sdev, int tag_type)			/* FCLNX-GPL-450*/
{
	if (sdev->tagged_supported) {
		scsi_set_tag_type(sdev, tag_type);

		if (tag_type)
			scsi_activate_tcq(sdev, sdev->queue_depth);
		else
			scsi_deactivate_tcq(sdev, sdev->queue_depth);
	}
	else {
		tag_type = 0;
	}
	
	return tag_type;
}
#endif /* FCLNX-GPL-FX-496 end */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
static ssize_t
hfcldd_conf_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;	/* FCLNX-GPL-622 *//* FCLNX-GPL-FX-410 */
}

static ssize_t
hfcldd_conf_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	if(hfclddconf){		/* FCLNX-676*//* FCLNX-GPL-147 *//* FCLNX-GPL-622 *//* FCLNX-GPL-FX-410 */
		if(count > 4096){
			memcpy(hfclddconf, buf, 4096);
		}else{
			memcpy(hfclddconf, buf, count);
		}
	}
	return strlen(buf);
}

/* FCLNX-GPL-564 start */
static ssize_t
hfcldd_proc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	off_t    offset        = 0;
	int      max_length    = PAGE_SIZE; /* kernel's PAGE_SIZE */
	struct adap_info	*ap;
	
	struct Scsi_Host *host = class_to_shost(dev);
	
	if (!host){ /* This host is not found */
		return 0;
	}
	
	ap = (struct adap_info *)host->hostdata;
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_proc_info_com(host, buf, offset, max_length, HFC_SYSFS_INFO_TYPE));
	}else if(!strcmp(ap->name, "virt_adap_info")) {	/* FCLNX-GPL-FX-481 >>> */
		return (hfc_manage_info.npubp->hfc_mp_proc_info_pfb_com(host, buf, offset, max_length, HFC_SYSFS_INFO_TYPE));
	}else if(!strcmp(ap->name, "lport_info")) {
		return (hfc_manage_info.npubp->hfc_fx_mp_proc_info_pfb_com(host, buf, offset, max_length, HFC_SYSFS_INFO_TYPE));
	}	/* >>> FCLNX-GPL-FX-481 */
	
	return hfc_proc_info_com(host, buf, offset, max_length, HFC_SYSFS_INFO_TYPE);
}
/* FCLNX-GPL-564 end */

#if 0
static ssize_t
hfcldd_target_lu_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	off_t    offset        = 0;
	int      max_length    = PAGE_SIZE; /* kernel's PAGE_SIZE */
	struct adap_info	*ap;
	
	struct Scsi_Host *host = class_to_shost(dev);
	
	if (!host){ /* This host is not found */
		return 0;
	}
	
	ap = (struct adap_info *)host->hostdata;
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_target_lu_info_com(host, buf, offset, max_length));
	}
	
	return hfc_target_lu_info_com(host, buf, offset, max_length);
}
#endif

#ifdef HFC_STAR /* FCLNX-GPL-547 Start */

static ssize_t
hfc_fx_fwrev_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	uint             fw_ver    = 0;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	
	if (pp == NULL){
		return 0;
	}
	
	fw_ver   = hfc_fx_get_sysrev(pp->region_arg[pp->rid]->core_arg[0]);
	
	return snprintf(buf, PAGE_SIZE, "%06x\n", fw_ver);
}

static ssize_t
hfc_fx_link_state_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	
	if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	else if( !test_bit(HFC_PS_ONLINE , (ulong *)&pp->status) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	else if( test_bit(HFC_PS_WAIT_LINKUP,(ulong *)&pp->status ) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	
	return snprintf(buf, PAGE_SIZE, "Link Up\n");
}

static ssize_t
hfc_fx_link_down_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", pp->linkup_tmo);
}

static ssize_t
hfc_fx_can_queue_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", pp->can_queue);
}

static ssize_t
hfc_fx_queue_depth_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", pp->queue_depth);
}

static ssize_t
hfc_fx_max_lun_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", pp->max_lun);
}

static ssize_t
hfc_fwrev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	struct hfc_vpd   *vpd_info = NULL;
	uint             fw_ver    = 0;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	
	if (ap == NULL){
		return 0;
	}

	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_fwrev_show(host, buf));
	}
	
	if (ap->mp_adap_info == NULL){
		return 0;
	}
	
	
	if(ap->pkg.type == HFC_PKTYPE_FPP){
		vpd_info = (struct hfc_vpd *)ap->mp_adap_info->vpd_buf;
		fw_ver   = vpd_info->fw_ver;
	}
	else { /* FIVE, FIVE-EX */
		fw_ver   = hfc_get_sysrev(ap);
	}
	
	return snprintf(buf, PAGE_SIZE, "%06x\n", fw_ver);
}

static ssize_t
hfc_link_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_link_state_show(host, buf));
	}
	
	if (ap->mp_adap_info == NULL){
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	
	
	if( test_bit(HFC_HWCHKSTOP , (ulong *)&ap->mp_adap_info->status) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	else if( !test_bit(HFC_ONLINE , (ulong *)&ap->status) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	else if( test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ) ) {
		return snprintf(buf, PAGE_SIZE, "Link Down\n");
	}
	
	return snprintf(buf, PAGE_SIZE, "Link Up\n");
}

static ssize_t
hfc_link_down_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_link_down_show(host, buf));
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ap->linkup_tmo);
}

static ssize_t
hfc_can_queue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_can_queue_show(host, buf));
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ap->can_queue);
}

static ssize_t
hfc_queue_depth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_queue_depth_show(host, buf));
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ap->queue_depth);
}

static ssize_t
hfc_max_lun_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_max_lun_show(host, buf));
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", ap->max_lun);
}


/* These attributes are Read Only. */
static DEVICE_ATTR(fwrev, S_IRUGO, hfc_fwrev_show, NULL);
static DEVICE_ATTR(link_state, S_IRUGO, hfc_link_state_show, NULL);
static DEVICE_ATTR(hfc_link_down, S_IRUGO, hfc_link_down_show, NULL);
static DEVICE_ATTR(hfc_can_queue, S_IRUGO, hfc_can_queue_show, NULL);
static DEVICE_ATTR(hfc_queue_depth, S_IRUGO, hfc_queue_depth_show, NULL);
static DEVICE_ATTR(hfc_max_lun, S_IRUGO, hfc_max_lun_show, NULL);

#endif /* HFC_STAR */ /* FCLNX-GPL-547 End */

static DEVICE_ATTR(hfcldd_conf, S_IRUGO | S_IWUSR, hfcldd_conf_show, hfcldd_conf_store);
static DEVICE_ATTR(hfcldd_proc, S_IRUGO, hfcldd_proc_show, NULL); /* FCLNX-GPL-564 */
static DEVICE_ATTR(vhfcldd_proc, S_IRUGO, hfcldd_proc_show, NULL); /* FCLNX-GPL-FX-481 */
//static DEVICE_ATTR(hfcldd_target_lu_info, S_IRUGO, hfcldd_target_lu_info_show, NULL);

/* FCLNX-GPL-FX-420 */
static ssize_t
hfc_numa_node_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", dev_to_node(dev));
}
static DEVICE_ATTR(numa_node, S_IRUGO , hfc_numa_node_show, NULL);


static struct hfc_intr_entry* hfc_find_intr_entry_by_irq(int irq){
	struct port_info    *pp	= NULL;
	struct hfc_intr_entry *entry = NULL;
	int pp_id = 0;
	int entry_id =0;

	HFC_DBGPRT("hfc_find_intr_entry_by_irq irq= %d\n",irq);
	
    for (pp_id=0; pp_id<MAX_ADAP_CNT; pp_id++) {
		if (hfc_manage_info.port_info_arg[pp_id] == NULL) { continue; }
		pp = hfc_manage_info.port_info_arg[pp_id];
		for(entry_id=0; entry_id<HFC_FX_MSIX_NVEC; entry_id++){
			entry = &pp->intr_entries[entry_id];
			if(entry->vector != irq){ continue; }
			return entry;
		}
	}

    return NULL;
}

static ssize_t
hfc_selected_irq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&hfc_manage_info.selected_irq));
}
static ssize_t hfc_selected_irq_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int val;
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		/* check argument */
    	if (sscanf(buf, "%d", &val) != 1){
    		return -EINVAL;
    	}

    	if (val < 0){
    		return -EINVAL;
    	}

   		if (hfc_find_intr_entry_by_irq(val)==NULL){
    		return -EINVAL;
    	}

   		 atomic_set(&hfc_manage_info.selected_irq, val);

		return strlen(buf);
	}
	
	return -EINVAL;

}
static DEVICE_ATTR(selected_irq, (S_IRUGO|S_IWUSR) , hfc_selected_irq_show, hfc_selected_irq_store);


static ssize_t
hfc_cpu_affinity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct hfc_intr_entry *entry = NULL;
	int selected_irq = 0;
	int len = 0;
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		selected_irq = atomic_read(&hfc_manage_info.selected_irq);

    	if (selected_irq < 0){
    		return -EINVAL;
    	}

    	entry = hfc_find_intr_entry_by_irq(selected_irq);
    	if(!entry){
    		return -EINVAL;
    	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
		len = cpumask_scnprintf(buf, PAGE_SIZE-2, &entry->affinity);
#else
		len = cpumap_print_to_pagebuf(false, buf, &entry->affinity);
#endif /* FCLNX-GPL-FX-496 end */
		buf[len++] = '\n';
		buf[len] = '\0';
		return len;
	}
	
	return 0;
}

static ssize_t hfc_cpu_affinity_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	struct cpuinfo_x86 	*cpuinfo=NULL;
	struct hfc_intr_entry *entry = NULL;
	struct cpumask mask;
	uint32_t			cpu;
	int selected_irq = 0;
	int err;
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */

		selected_irq = atomic_read(&hfc_manage_info.selected_irq);

    	if (selected_irq < 0){
    		return -EINVAL;
    	}

    	entry = hfc_find_intr_entry_by_irq(selected_irq);
    	if(!entry){
    		return -EINVAL;
    	}

		memset(&mask, 0, sizeof(struct cpumask));

    	// use __bitmap_parse as workaround
    	err = __bitmap_parse(buf, PAGE_SIZE, 0, mask.bits, nr_cpumask_bits);
    	if(err != 0){
    		return err;
    	}

    	cpumask_clear(&entry->affinity);

    	for_each_present_cpu(cpu) {
			cpuinfo = &cpu_data(cpu);
			if(!cpumask_test_cpu(cpu, &mask)) { continue; }
	  	 		cpumask_set_cpu(cpu, &entry->affinity);
   		}

		return strlen(buf);
	}
	
	return 0;

}
static DEVICE_ATTR(cpu_affinity, (S_IRUGO|S_IWUSR), hfc_cpu_affinity_show, hfc_cpu_affinity_store);
/* FCLNX-GPL-FX-420 */

static ssize_t
hfc_fx_selected_target_id_show(struct Scsi_Host *host, char *buf)
{
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&pp->selected_target_id));
}

static ssize_t
hfc_selected_target_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_selected_target_id_show(host, buf));
	}
	
	return snprintf(buf, PAGE_SIZE, "%d\n", atomic_read(&ap->selected_target_id));
}

static ssize_t hfc_selected_target_id_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int val;
	struct Scsi_Host *host     = class_to_shost(dev);
	struct adap_info *ap       = NULL;
	struct port_info *pp       = NULL;
	
	if (host == NULL){ /* This host is not found */
		return 0;
	}
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	/* check argument */
    if (sscanf(buf, "%d", &val) != 1){
    	return -EINVAL;
    }

    if (val < 0){
    	return -EINVAL;
    }
    
    if (val >= MAX_TARGET_PROBE){
    	return -EINVAL;
    }
    
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
    	pp = (struct port_info *)host->hostdata;
	
		if (pp == NULL){
			return 0;
		}

		atomic_set(&pp->selected_target_id, val);

		return strlen(buf);
	}
	
	atomic_set(&ap->selected_target_id, val);
	
	return strlen(buf);

}
static DEVICE_ATTR(selected_target_id, (S_IRUGO|S_IWUSR) , hfc_selected_target_id_show, hfc_selected_target_id_store);

static int
hfc_target_status_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length)
{
	struct adap_info	*ap;
	struct target_info	*tp;
	int					rtn;
	char				buf[256];
	char				*data = buffer;
	int					len, partial, pos = 0;
	short				vendor_id, device_id;
	int 				selected_target_id = 0;
	int					proc_type = HFC_SYSFS_INFO_TYPE;	/* FCLNX-GPL-FX-483 */
	
	ap = (struct adap_info *)host->hostdata;
	
	if (ap == NULL){
		return 0;
	}
	
	SPRINTF ("Hitachi PCI to Fibre Channel Host Bus Adapter\n");
	SPRINTF ("  Special file name       = hfcldd%d\n", ap->dev_minor);
	SPRINTF ("  Adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_read_cnfg(ap, 0x00, 0x2);
	device_id = (ushort) hfc_read_cnfg(ap, 0x02, 0x2 );
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	
	SPRINTF ("  Device Information \n");
    
    selected_target_id = atomic_read(&ap->selected_target_id);
    
    if (selected_target_id < 0){
    	return -EINVAL;
    }
    
    if (selected_target_id >= MAX_TARGET_PROBE){
    	return -EINVAL;
    }
	
	tp = hfc_hash_target_valid(ap, selected_target_id);
	if( tp != NULL){

		if( test_bit(HFC_TARGETINF_VALID, (ulong *)&tp->flags)){

			SPRINTF ("    target id [");
			if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
				SPRINTF("%d",tp->target_id);
			}
			else{ 
				SPRINTF("-");
			}
			SPRINTF ("] :");
			if(test_bit(HFC_WWN_VALID, (ulong *)&tp->flags)){
				SPRINTF (" port name = %llx node name = %llx DID = %02llx ",
					(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
			}
			else{
				SPRINTF (" port name =    -    node name =   -     DID = %02llx",(unsigned long long)tp->scsi_id );                            
			}
			
			
			SPRINTF (" (pseq = %02d flags=%08x status=%08x)\n", tp->pseq, tp->flags, tp->status);
			
		}
		if (hfc_manage_info.hfcldd_mp_mod) {					/* FCLNX-GPL-204 *//* FCLNX-GPL-FX-483 */
			data = hfc_manage_info.npubp->hfc_proc_info_lun(ap, tp, data, offset, length, &pos);
		}
	}
	
	SPRINTF ("\n");
	
	
	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);
}

static ssize_t
hfc_target_status_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	off_t    offset        = 0;
	int      max_length    = PAGE_SIZE; /* kernel's PAGE_SIZE */
	struct adap_info	*ap;
	
	struct Scsi_Host *host = class_to_shost(dev);
	
	if (!host){ /* This host is not found */
		return 0;
	}
	
	ap = (struct adap_info *)host->hostdata;
	if (ap == NULL){
		return 0;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_target_status_info_com(host, buf, offset, max_length));
	}else if(!strcmp(ap->name, "virt_adap_info")) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}else if(!strcmp(ap->name, "lport_info")) {
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */
	
	return hfc_target_status_info_com(host, buf, offset, max_length);
}

static DEVICE_ATTR(target_status, S_IRUGO, hfc_target_status_show, NULL);

struct device_attribute *hfcldd_host_attrs[] = {
	&dev_attr_hfcldd_conf,
	&dev_attr_hfcldd_proc,	/* FCLNX-GPL-564 */
	&dev_attr_numa_node,	/* FCLNX-GPL-FX-420 */
	&dev_attr_selected_irq,	/* FCLNX-GPL-FX-420 */
	&dev_attr_cpu_affinity,	/* FCLNX-GPL-FX-420 */
//	&dev_attr_hfcldd_target_lu_info,
	&dev_attr_selected_target_id,	/* FCLNX-GPL-FX-420 */
	&dev_attr_target_status,	/* FCLNX-GPL-FX-420 */
#ifdef HFC_STAR /* FCLNX-GPL-547 Start */
	&dev_attr_fwrev,
	&dev_attr_link_state,
	&dev_attr_hfc_link_down,
	&dev_attr_hfc_can_queue,
	&dev_attr_hfc_queue_depth,
	&dev_attr_hfc_max_lun,
#endif /* HFC_STAR */ /* FCLNX-GPL-547 End */
	NULL,
};
/* FCLNX-GPL-FX-481 >>> */
struct device_attribute *hfcldd_host_attrs_pfb[] = {
	&dev_attr_hfcldd_conf,
	&dev_attr_vhfcldd_proc,	/* FCLNX-GPL-564 */
	&dev_attr_numa_node,	/* FCLNX-GPL-FX-420 */
	&dev_attr_selected_irq,	/* FCLNX-GPL-FX-420 */
	&dev_attr_cpu_affinity,	/* FCLNX-GPL-FX-420 */
//	&dev_attr_hfcldd_target_lu_info,
	&dev_attr_selected_target_id,	/* FCLNX-GPL-FX-420 */
	&dev_attr_target_status,	/* FCLNX-GPL-FX-420 */
#ifdef HFC_STAR /* FCLNX-GPL-547 Start */
	&dev_attr_fwrev,
	&dev_attr_link_state,
	&dev_attr_hfc_link_down,
	&dev_attr_hfc_can_queue,
	&dev_attr_hfc_queue_depth,
	&dev_attr_hfc_max_lun,
#endif /* HFC_STAR */ /* FCLNX-GPL-547 End */
	NULL,
};
/* >>> FCLNX-GPL-FX-481 */
#else
static ssize_t
hfcldd_conf_show(struct class_device *cdev, char *buf)
{
//	printk ("hfcldd_conf_show()\n");
	return snprintf(buf, sizeof(char)*CFG_ENT_SIZE, "%s\n", hfclddconf);
}

static ssize_t
hfcldd_conf_store(struct class_device *cdev, const char *buf, size_t count)
{
	if(hfclddconf)		/* FCLNX-676*//* FCLNX-GPL-147 */
		memcpy(hfclddconf, buf, sizeof(char)*CFG_ENT_SIZE);
//	printk ("hfcldd_conf_store()\n");
	return strlen(buf);
}

static CLASS_DEVICE_ATTR(hfcldd_conf, S_IRUGO | S_IWUSR, hfcldd_conf_show, hfcldd_conf_store);

struct class_device_attribute *hfcldd_host_attrs[] = {
 &class_device_attr_hfcldd_conf,
 NULL,
};
/* FCLNX-GPL-FX-481 >>> */
struct class_device_attribute *hfcldd_host_attrs_pfb[] = {
 &class_device_attr_hfcldd_conf,
 NULL,
};
/* >>> FCLNX-GPL-FX-481 */
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
DEF_SCSI_QCMD(hfc_strategy)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

//static struct scsi_host_template hfcldd_driver_template = {
struct scsi_host_template hfcldd_driver_template = {

	.proc_dir=					NULL,					
	.proc_name=					"hfcldd",
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
//	.proc_info=					hfc_proc_info_k26,			
#else
	.proc_info=					hfc_proc_info_k26,			
#endif
	.name=						"Hitachi PCI Fibre Channel Adapter",
	.info=						hfc_info,	
	.slave_configure=			hfc_slave_configure,
	.slave_alloc=				hfc_slave_alloc,
	.slave_destroy=				hfc_slave_destroy,
#ifdef SYSFS_SUPPORT		/* FCLNX-GPL-575 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	.scan_finished=				hfc_scan_finished,	/* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
	.scan_start=				hfc_scan_start, 	/* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */	/* FCLNX-GPL-575 */
	.change_queue_depth=		hfc_change_queue_depth,						/* FCLNX-GPL-450 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	.change_queue_type=			hfc_change_queue_type,						/* FCLNX-GPL-450 */
#endif /* FCLNX-GPL-FX-496 end */
	.queuecommand=				hfc_strategy,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17) && !defined(HFC_X8664_SLES10) /* FCLNX-0309 */
	.eh_strategy_handler=		NULL,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)							/* FCLNX-GPL-0343 */
	.eh_abort_handler=			NULL,	
#else
	.eh_abort_handler=			hfc_eh_abort,	
#endif																		/* FCLNX-GPL-0343 */
	.eh_device_reset_handler=	hfc_eh_device_reset,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	.eh_target_reset_handler=	hfc_eh_target_reset,						/* FCLNX-GPL-0343 */
#endif
	.eh_bus_reset_handler=		hfc_eh_bus_reset,
	.eh_host_reset_handler=		NULL,	
	.bios_param=				hfc_biosparam,	
	.can_queue=					1,
	.this_id=					-1,	
	.sg_tablesize=				HFC_SG_TABLESIZE,
	.cmd_per_lun=				HFC_CMD_PER_LUN,
	.use_clustering=			ENABLE_CLUSTERING,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	.shost_attrs=				hfcldd_host_attrs,
#endif /* KERNEL_VERSION(2,6,16) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.use_blk_tags =				1,
#endif
};


/* FCLNX-GPL-575 */
static struct scsi_host_template hfcldd_driver_template_mp = {

	.proc_dir=					NULL,					
	.proc_name=					"hfcldd",
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
//	.proc_info=					hfc_proc_info_k26,			
#else
	.proc_info=					hfc_proc_info_k26,
#endif
	.name=						"Hitachi PCI Fibre Channel Adapter",
	.info=						hfc_info,	
	.slave_configure=			hfc_slave_configure,
	.slave_alloc=				hfc_slave_alloc,
	.slave_destroy=				hfc_slave_destroy,
	.change_queue_depth=		hfc_change_queue_depth,						/* FCLNX-GPL-450 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	.change_queue_type=			hfc_change_queue_type,						/* FCLNX-GPL-450 */
#endif /* FCLNX-GPL-FX-496 end */
	.queuecommand=				hfc_strategy,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17) && !defined(HFC_X8664_SLES10) /* FCLNX-0309 */
	.eh_strategy_handler=		NULL,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)							/* FCLNX-GPL-0343 */
	.eh_abort_handler=			NULL,	
#else
	.eh_abort_handler=			hfc_eh_abort,	
#endif																		/* FCLNX-GPL-0343 */
	.eh_device_reset_handler=	hfc_eh_device_reset,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	.eh_target_reset_handler=	hfc_eh_target_reset,						/* FCLNX-GPL-0343 */
#endif
	.eh_bus_reset_handler=		hfc_eh_bus_reset,
	.eh_host_reset_handler=		NULL,	
	.bios_param=				hfc_biosparam,	
	.can_queue=					1,
	.this_id=					-1,	
	.sg_tablesize=				HFC_SG_TABLESIZE,
	.cmd_per_lun=				HFC_CMD_PER_LUN,
	.use_clustering=			ENABLE_CLUSTERING,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	.shost_attrs=				hfcldd_host_attrs,
#endif /* KERNEL_VERSION(2,6,16) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.use_blk_tags =				1,
#endif
};		/* FCLNX-GPL-575 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
DEF_SCSI_QCMD(hfc_strategy_pfb)
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

static struct scsi_host_template hfcldd_driver_template_platform_bus = {		/* FCLNX-GPL-204 */

	.proc_dir=					NULL,					
	.proc_name=					"vhfcldd",	
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
//	.proc_info=					hfc_proc_info_pfb,			
#else
	.proc_info=					hfc_proc_info_pfb,			
#endif
	.name=						"Hitachi PCI Fibre Channel Adapter",
	.info=						hfc_info_pfb,
	.slave_configure=			hfc_slave_configure,
	.slave_alloc=				hfc_slave_alloc,
	.slave_destroy=				hfc_slave_destroy,
	.change_queue_depth=		hfc_change_queue_depth,						/* FCLNX-GPL-450 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	.change_queue_type=			hfc_change_queue_type,						/* FCLNX-GPL-450 */
#endif /* FCLNX-GPL-FX-496 end */
	.queuecommand=				hfc_strategy_pfb,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17) && !defined(HFC_X8664_SLES10) /* FCLNX-0309 */
	.eh_strategy_handler=		NULL,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)							/* FCLNX-GPL-0343 */
	.eh_abort_handler=			NULL,	
#else
	.eh_abort_handler=			hfc_eh_abort_pfb,	
#endif																		/* FCLNX-GPL-0343 */
	.eh_device_reset_handler=	hfc_eh_device_reset_pfb,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
	.eh_target_reset_handler=	hfc_eh_target_reset_pfb,					/* FCLNX-GPL-0449 */
#endif
	.eh_bus_reset_handler=		hfc_eh_bus_reset_pfb,
	.eh_host_reset_handler=		NULL,	
	.bios_param=				hfc_biosparam,	
	.can_queue=					1,
	.this_id=					-1,	
	.sg_tablesize=				HFC_SG_TABLESIZE,
	.cmd_per_lun=				HFC_CMD_PER_LUN,
	.use_clustering=			ENABLE_CLUSTERING,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	.shost_attrs=				hfcldd_host_attrs_pfb,						/* FCLNX-GPL-FX-481 */
#endif /* KERNEL_VERSION(2,6,16) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	.use_blk_tags =				1,
#endif
};


int
hfc_biosparam(struct scsi_device *sdev, struct block_device *bdev, sector_t capacity, int geom[])
{
	int heads, sectors, cylinders;

	heads = 64;
	sectors = 32;
	cylinders = (unsigned long)capacity / (heads * sectors);

	if(cylinders > 1024) {
		heads = 255;
		sectors = 63;
		cylinders = (unsigned long)capacity / (heads * sectors);
	}
 
	geom[0] = heads;
	geom[1] = sectors;
	geom[2] = cylinders;

	return 0;
}

/* FIVE-FX */
/*
 * Function: hfc_probe_one
 *
 * Purpose: Driver initialization
 *
 * Arguments:
 *  pdev	 - Pointer to pdev 
 *  id		 - PCI device ID
 *
 * Returns: 
 *  0         - Normal end
 *  Otherwise - Error
 *
 * Notes:
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
int
#else
int __devinit
#endif
hfc_probe_one(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct pub_symbol_list			*pubp;
	struct nonpub_symbol_list		*npubp;
	struct mp_manage_info			*hfc_mp_manage_info=NULL;
	
	int	adapter_max = -1;
	int error = -ENODEV;
	
	HFC_DBGPRT("hfc_probe_one is called. \n");
	HFC_DBGPRT("pdev->device = %x \n", pdev->device);
	
	hfclddconf = (char *)hfcldd_cnf();
	
	if(instance == 0){
#ifndef _HFC_NO_RASLOG
		hraslogopt.func_code = OPEN_RASLOG;
		raslog_install = _hraslogserv( &hraslogopt );
		if( raslog_install == 1 ){
			HFC_WRNPRT("hfcldd : HFC_ERR9 FC Adapter Driver error (ErrNo:0xC7) Failed to open hraslog\n");
		}
		else if( raslog_install == 2 ){
			HFC_INFPRT("hfcldd : Raslog module is not loaded.\n");
		}
		else if( raslog_install == 0 ){
			HFC_INFPRT("hfcldd : Raslog version is raslog-%d.%d.%d-%d.\n", hraslogopt.ver, hraslogopt.rev,
						hraslogopt.rver, hraslogopt.wver);
		}
#endif
		instance++;
		
		memset( &hfc_manage_info, 0, sizeof(hfc_manage_info) );
		memset( &hfc_narrow_dev, 0, sizeof(hfc_narrow_dev) );
		
		/* Set structure character name */
		strcpy(hfc_manage_info.name, "manage_info");
		
		pubp  = hfc_get_pub_symbol_list();
		npubp = hfc_get_nonpub_symbol_list();
		hfc_manage_info.pubp  = pubp;
		hfc_manage_info.npubp = npubp;
		hfc_manage_info.hfcldd_driver_template = &hfcldd_driver_template;
		hfc_manage_info.hfcldd_driver_template_mp = &hfcldd_driver_template_mp;
		hfc_manage_info.hfcldd_driver_template_platform_bus = &hfcldd_driver_template_platform_bus;
		hfc_set_pub_symbol_list();
		if(!hfc_check_nonpub_symbol()) {
			hfc_manage_info.hfcldd_mp_mod = HFCLDD_MP_MOD_ENABLE;
			hfc_mp_manage_info = npubp->hfc_get_mp_manage_info();
			hfc_manage_info.mp_manage_info = hfc_mp_manage_info;
			hfc_mp_manage_info->manage_info = &hfc_manage_info;
			try_module_get(hfc_mp_manage_info->hfcldd_mp);
		}

		strcpy( hfc_manage_info.package_ver, package_ver);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
		sema_init(&hfc_manage_info.sem, 1);
#else
		init_MUTEX(&hfc_manage_info.sem);
#endif
		spin_lock_init(&hfc_manage_info.hfcmp_lock);
		spin_lock_init(&hfc_manage_info.hfcmp_fx_lock);

		hfc_param_setup();
		adapter_max = hfc_get_adapter_bindings();
		
		/* FCLNX-GPL-204 */
		
		if(adapter_max>=0) hba_num = adapter_max+1;
		hfc_version();
		
		/* FCLNX-GPL-547 start */
		if(hfc_chk_conf_val(0, HFC_LOGFILE_TYPE_MAX, hfc_log_file)){
			hfc_manage_info.log_file = hfc_log_file;
		}
		/* FCLNX-GPL-547 end */
	}

	if(hfc_manage_info.hfcldd_mp_mod) {
		if(hfc_manage_info.npubp->hfc_mp_module_init(instance, hfclddconf, 1, hfc_narrowmap)) {
			return error;
		}
	}
	
	if ((hfc_manage_info.lg_target_info == NULL) && (hfc_manage_info.hfcldd_mp_mod)) {
		/* hfcldd_mp module is unavailable. */
		if (hfc_mp_manage_info) {
			module_put(hfc_mp_manage_info->hfcldd_mp);
			hfc_manage_info.hfcldd_mp_mod = HFCLDD_MP_MOD_DISABLE;
			hfc_manage_info.mp_manage_info = NULL;
		}
	}
	
	if( ( pdev->device == HFC_PCI_DEVICE_ID_300A )		/* FIVE 1 port 0x300A */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300B )		/* FIVE 2 port 0x300B */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300C )		/* FIVE		   0x300C */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300D )		/* FIVE		   0x300D */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_3020) ){	/* FIVE-EX	   0x3020 */
		error = hfc_ex_probe_one( pdev, id );
	}
	else if(pdev->device == HFC_PCI_DEVICE_ID_3070){
		error = hfc_fx_probe_one( pdev, id , hfclddconf);
	}
	
	return error;

}

/*
 * Function: hfc_ex_probe_one
 *
 * Purpose: 4/8 Gbps FC-HBA Driver initialization
 *
 * Arguments:
 *  pdev	 - Pointer to pdev 
 *  id		 - PCI device ID
 *
 * Returns: 
 *  0         - Normal end
 *  Otherwise - Error
 *
 * Notes:
 */
int 
hfc_ex_probe_one(struct pci_dev *pdev, const struct pci_device_id *id)	/* FIVE-FX */
{
	struct Scsi_Host *host=NULL,*hsd_host=NULL,*pfb_host=NULL;		/* FCLNX-0429 */
	struct adap_info *ap = NULL;
	
	int			error = -ENODEV;
	int			reg_chrdev = 0, rtn;
	uint64_t 	dma_mask;
	ushort		bind_err = FALSE;				/* FCLNX-0634 */
	uint		wk_err_num ;					/* FCLNX-GPL-116 */
	ulong		flags = 0;						/* FCLNX-GPL-397 */
	uchar		vrt_host_alloc = 0;				/* FCLNX-GPL-473 */
	int i;

	HFC_DBGPRT("hfcldd[%d] found on PCI bus %i, dev %i\n",
		(int)id->driver_data, pdev->bus->number, PCI_SLOT(pdev->devfn));

	rtn = pci_enable_device(pdev);
	if(rtn) {
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x9A, (uchar *)&rtn, 4) ;/* FCLNX-GPL-161 */
		goto hfc_probe_enable_error;
	}
	
	HFC_DBGPRT(" hfcldd : pci device is enabled irq=%d\n",pdev->irq);

	error = -ENOMEM;

	HFC_DBGPRT(" hfcldd : befire scsi_host_alloc\n");

	/* FCLNX-GPL-565 start *//* FCLNX-GPL-575 start */
	if ( hfc_manage_info.hfcldd_mp_mod ) {
		host = hfc_scsi_host_alloc(&hfcldd_driver_template_mp, sizeof(struct adap_info));
	} 
	else {
		if( hfc_rport_lu_scan != 1){
			host = hfc_scsi_host_alloc(&hfcldd_driver_template_mp, sizeof(struct adap_info));
		}
		else if( hfc_rport_lu_scan == 1){
			host = hfc_scsi_host_alloc(&hfcldd_driver_template, sizeof(struct adap_info));
		}
	}
	/* FCLNX-GPL-565 end *//* FCLNX-GPL-575 end */
	
	if ( host == NULL ){
		hfc_errlog(NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x37, NULL, 0) ;/* FCLNX-GPL-161 */
		goto hfc_host_alloc_error;
	}

	HFC_DBGPRT( "  hfcldd : hfc_detect - adap_info pointer was set\n"); 
	ap = (struct adap_info *)host->hostdata;
	memset(ap, 0, sizeof(struct adap_info));
	
	/* Set structure character name */
	strcpy(ap->name, "adap_info");
	
	ap->hfclddconf = hfclddconf;
	ap->raslog_install = raslog_install;
#ifndef _HFC_NO_RASLOG
	if( raslog_install == 0 ){
		ap->raslog_ver = hraslogopt.ver;
		ap->raslog_rev = hraslogopt.rev;
		ap->raslog_rver = hraslogopt.rver;
		ap->raslog_wver = hraslogopt.wver;
	}
#endif

	/* Set SRAM CE LOG ID */ /* FCLNX-GPL-116 */
	wk_err_num = 0xfffe1100;
	HFC_4L_TO_4B(ap->ce_log.err_num, wk_err_num);

	/* set adap_info */

#ifdef __x86_64			
	/* set dma_mask */
	if ((rtn=pci_set_dma_mask(pdev, 0xffffffffffffffffULL))==0) {
		HFC_DBGPRT(" hfcldd : Using 64bit DMA\n");
	} else if ((rtn=pci_set_dma_mask(pdev, 0xffffffffUL))==0) {
		HFC_DBGPRT(" hfcldd : Using 32bit DMA\n");
	} else {
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC3, (uchar *)&rtn, 4) ;/* FCLNX-GPL-161 */
		goto hfc_probe_error;
	}
#endif			
			
	rtn=0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	dma_mask = DMA_BIT_MASK(64); /* FCLNX-GPL-564 start */
	if (sizeof(dma_addr_t) > 4) {
		if (pci_set_dma_mask(pdev, DMA_BIT_MASK(64)) == 0) {
			if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64))) {
				rtn=pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32));
				dma_mask = DMA_BIT_MASK(32);
			}
		}
		else
		{
			dma_mask = DMA_BIT_MASK(32);
			rtn=pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
		}
	}
	else
	{
		dma_mask = DMA_BIT_MASK(32);
		rtn=pci_set_dma_mask(pdev, DMA_BIT_MASK(32)); /* FCLNX-GPL-564 end */
	}
#else
	dma_mask = DMA_64BIT_MASK;
	if (sizeof(dma_addr_t) > 4) {
		if (pci_set_dma_mask(pdev, DMA_64BIT_MASK) == 0) {
			if (pci_set_consistent_dma_mask(pdev, DMA_64BIT_MASK)) {
				rtn=pci_set_consistent_dma_mask(pdev, DMA_32BIT_MASK);
				dma_mask = DMA_32BIT_MASK;
			}
		}
		else
		{
			dma_mask = DMA_32BIT_MASK;
			rtn=pci_set_dma_mask(pdev, DMA_32BIT_MASK);
		}
	}
	else
	{
		dma_mask = DMA_32BIT_MASK;
		rtn=pci_set_dma_mask(pdev, DMA_32BIT_MASK);
	}
#endif
	if(rtn){
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x9B, (uchar *)&rtn, 4) ;/* FCLNX-GPL-161 */
		goto hfc_probe_error;
	}
	
	ap->dma_mask = dma_mask;	/* FCLNX-0671 *//* FCLNX-GPL-204 */

	ap->manage_info = &hfc_manage_info;
	
	ap->hosts = host;
	ap->host_no = host->host_no;
	ap->pci_cfginf = pdev;
			
	if (hfc_shadow == 1)
		ap->mlpf_mode |= HFC_MMODE_SHADOW;

	if(hfc_query_devid(ap)){ /* Get Device ID (FPP or FIVE or FIVE-EX ?) */
		/* ERR(Unknown Device) */
		goto hfc_probe_error;
	}
	
	rtn = hfc_pci_conf(ap);
	
	if ( HFC_MMODE_CHECK_SHARED(ap) ) {
		if( (ap->pkg.type == HFC_PKTYPE_FPP) ||
			(ap->pkg.type == HFC_PKTYPE_FIVE))
		{
			ap->lparmode.frame_cnt = 3;						/* FCLNX-0375 */
		}
		else /* FIVE-EX */
		{
			ap->lparmode.frame_cnt = 4;
		}
	}
	else {
		ap->lparmode.frame_cnt = 4;						/* FCLNX-0375 */
	}
	
	if (rtn != 0) {
		if( rtn != (-ENODEV) )															/* FCLNX-0357 */
		{
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC4, NULL, 0) ;/* FCLNX-GPL-161 */
		}																				/* FCLNX-0357 */
		goto hfc_probe_error;
	}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	host->transportt = hfc_fc_attach_transport;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	if(hfc_query_pktype(ap)){
		goto hfc_pktype_error;
	}

	hfc_get_adapter_port_no(ap);		/* Read adapter port# */
	if( hfc_read_hfcbios(ap) ){				/* Read hbabios data */
		goto hfc_attach_error; /* FCLNX-GPL-116 */
	}

	rtn = hfc_search_adapter_number(ap);

	if ( rtn == MAX_ADAP_CNT + 1) { /* Incorrect Instance number is specified */
		/* FCLNX-0376 */
		if ( !HFC_MMODE_CHECK_BASIC(ap) ) {
			hfc_mlpf_change_state(ap, HFC_HG_LPRSTATUS_ISVALID, HFC_ENABLE_LPAR_STATE);
			hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_SPACE, HFC_DISABLE_LPAR_STATE);
			hfc_mlpf_change_state(ap, HFC_HG_LPRDETAIL_SHADOW_DED, HFC_ENABLE_LPAR_STATE);
		}
		goto hfc_attach_error;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) 								/* FCLNX-0476 */
	else if (rtn == -1) {
		ap->instance = hba_num;
		hba_num++;
#else
	else if ( (rtn == -1) || (( ap->defparam == 1 )&&(ap->automap == 1)) ) { /* instance number is not specified or Force Default Setting */ /* FCLNX-0634 */
		for (i=0; i<MAX_ADAP_CNT; i++) {
			if ((hfc_manage_info.adap_info_arg[i] == NULL) && (hfc_manage_info.port_info_arg[i] == NULL)) {
				if((( ap->defparam == 1 )&&( ap->automap == 1 ))
				/* FCLNX-0634 */
				||(hfc_manage_info.adap_bind[i] == -1))
					break;
			}
		}
		if (i == MAX_ADAP_CNT)
			goto hfc_attach_error;
		ap->instance = i;
	}
	else if (hfc_manage_info.adap_info_arg[rtn] != NULL){	/*The instance number is also attached.*/ /* FCLNX-0634 */
		bind_err = TRUE;
		hfc_errlog(NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,0x50,NULL,0) ;/* FCLNX-0634 *//* FCLNX-GPL-161 */
		for (i=0; i<MAX_ADAP_CNT; i++) {
			if(hfc_manage_info.adap_info_arg[i] == NULL){
				break;
			}
		}
		if (i == MAX_ADAP_CNT)
			goto hfc_attach_error;
		ap->instance=i;
#endif
	}
										/* FCLNX-0476 */
	else {	/* instance number is specified */
		ap->instance = rtn;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
	host->unique_id = hfc_manage_info.instance; 	/* FCLNX-0476 */
	ap->unique_id	= hfc_manage_info.instance; 	/* FCLNX-0476 */
	ap->dev_minor	= hfc_manage_info.instance; 	/* FCLNX-0476 */
#else
	if(( ap->defparam ==1 )&&( ap->automap== 1 )){	/* FCLNX-0630 */
		host->unique_id = hfc_manage_info.instance; 	/* FCLNX-0476 */
		ap->unique_id	= hfc_manage_info.instance; 	/* FCLNX-0476 */
		ap->dev_minor	= hfc_manage_info.instance; 	/* FCLNX-0476 */
	}
	else{	/* FCLNX-0630 */ 
		host->unique_id = ap->instance;                 /* FCLNX-0453 */
		ap->unique_id   = ap->instance;                 /* FCLNX-0453 */
		ap->dev_minor   = ap->instance;                 /* FCLNX-0453 */
	}  	/* FCLNX-0630 */
#endif

	/* FCLNX-0429 */	/* FCLNX-0651 */
	if (hfc_manage_info.hfcldd_mp_mod) {			/* FCLNX-GPL-204 */
		vrt_host_alloc = hfc_manage_info.npubp->hfc_mp_scsi_host_alloc(&hfcldd_driver_template_platform_bus, 
															host, ap, &hsd_host, &pfb_host);	/* FCLNX-GPL-473 */
		if(vrt_host_alloc == HFC_VRTHOST_ALLOC_FAIL) {	/* FCLNX-GPL-473 */
			goto hfc_probe_error;
		}
	}												/* FCLNX-GPL-204 */

	HFC_DBGPRT("instance_num = %d ap->instance = %d\n",instance, ap->instance);
	hfc_conf_setup(ap);

	if(HFC_MMODE_CHECK_MLPF(ap)&&(ap->hg_stats_disable == HFC_ENABLE_HGSTATS)){	/* FCLNX-GPL-494 */
		if(hfc_alloc_mlpf_cca(ap)){
			ap->hg_stats_disable = HFC_DISABLE_HGSTATS;
		}
		hfc_mlpf_cca_setup(ap);	/* FCLNX-GPL-507 */
	}																			/* FCLNX-GPL-494 */
			

	if (hfc_manage_info.hfcldd_mp_mod) {		/* FCLNX-0651*/
		hfc_manage_info.npubp->hfc_set_retry_cnt(ap); 					/* FCLNX-0534 */
	}	/* FCLNX-0651*/

//	if((hfc_manage_info.hfcplus_enable)&&(HFC_MMODE_CHECK_BASIC(ap))){	/* FCLNX-0506 */
	if(hfc_manage_info.hfcldd_mp_mod){					/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_isolconf_setup(ap);  /* FCLNX-0506 */
		if(hfc_manage_info.npubp->hfc_allocate_errcnt_info(ap,GFP_KERNEL)){	/* FCLNX-GPL-FX-314 */
			goto hfc_attach_error;
		}
		if( HFC_MMODE_CHECK_SHARED(ap) ){	/* FCLNX-GPL-393 */
			hfc_mlpf_check_isol_psycalport(ap);								/* FCLNX-GPL-393 */
		}																	/* FCLNX-GPL-393 */
	}
	else {
		if( HFC_MMODE_CHECK_SHARED(ap) ){	/* FCLNX-GPL-393 */
			hfc_mlpf_check_isol_psycalport(ap);								/* FCLNX-GPL-393 */
			if(!( HFC_MMODE_CHECK_SHADOW(ap) ))
				hfc_mlpf_set_errorlimit(ap);
		}																	/* FCLNX-GPL-393 */
//		ap->login_retry=HFC_LOGIN_RETRY;		/* FCLNX-GPL-0343 */
//		ap->els_retry=HFC_ELS_RETRY;			/* FCLNX-GPL-0343 */
//		ap->to_reset_retry=HFC_TO_RESET_RETRY;	/* FCLNX-GPL-349 */
		ap->scsi_to_retry=HFC_SCSI_TO_RETRY;
	}

	if (hfc_manage_info.hfcldd_mp_mod) {		/* FCLNX-0651 *//* FCLNX-GPL-204 */
		hfc_manage_info.npubp->hfc_mp_set_parm(host, ap, hsd_host, pfb_host, dma_mask); /* FCLNX-0521 */
	}

	if( hfc_attach(ap) ){
		HFC_ERRPRT("hfcldd :  Failed to allocate adapter resource\n");
		goto hfc_attach_error;
	}

	/* Initialize package type and fw_init table */
	HFC_DBGPRT(" hfcldd : hfcl_probe_one - initialize package type \n");

	if ( hfc_start_adapter(ap) ) {
		goto hfc_attach_error;
	}

	set_bit(HFC_ENABLE, (ulong *)&ap->status);							/* hfcends */
	ap->open_status = 0;												/* hfcends */

   /* Register special file */
	HFC_DBGPRT(" hfcldd : hfc_detect - set device special file\n");
	if (hfc_manage_info.instance == 0) {
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
		if (misc_register(&hfc_miscdev)) {
			HFC_DBGPRT(" hfcldd : hfc_ex_probe_one - misc_register failed\n");
		} else {	/* FCLNX-GPL-FX-492 start */
			HFC_DBGPRT(" hfcldd : hfc_ex_probe_one - misc_register success\n");
			hfc_major = MISC_MAJOR;
		}			/* FCLNX-GPL-FX-492 end */
#else
		hfc_major = register_chrdev(0, "hfcldd", &hfc_fops);
		if (0 > hfc_major) {
			HFC_DBGPRT("%s(): register_chrdev rc=%d\n", 	__func__, hfc_major);
			reg_chrdev = 1;
		}
		HFC_DBGPRT(" hfcldd : hfc_detect - major # = %d\n", hfc_major);
#endif
	}
						
	ap->dev_major = hfc_major;										/* FCLNX_001 */
	HFC_DBGPRT(" hfcldd : hfc_detect - minor # = %d\n", ap->dev_minor);
	hfc_manage_info.major = hfc_major;								/* FCLNX_001 */
	hfc_manage_info.adap_info_arg[ap->dev_minor] = ap;

	hfcldd_driver_template.can_queue = ap->can_queue;
	hfcldd_driver_template.sg_tablesize = ap->sg_tblsize;

	HFC_DBGPRT("hfcldd : hfcl_detect hfc_manage_info.instance = %d\n",
		hfc_manage_info.instance);
	hfc_manage_info.instance++;
	
	pci_set_drvdata(pdev,host);				/* FCLNX-0290 */
	HFC_DBGPRT(" hfcldd : hfc_detect scsi-add-host\n");
	rtn = scsi_add_host(host, &pdev->dev);
	if(rtn) {
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC6, (uchar *)&rtn, 4) ;/* FCLNX-GPL-161 */
		goto error_disable_adapter;
	}							/* FCLNX-0290 */
	
	if (hfc_manage_info.hfcldd_mp_mod) {		/* FCLNX-GPL-204 */
		if ( hfc_manage_info.npubp->hfc_mp_scsi_add_host(host, ap, hsd_host, pfb_host) ){
			goto error_disable_adapter;
		}
	}											/* FCLNX-GPL-204 */
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Set fixed fc host attributes */
	hfc_fc_host_init(host, ap);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	/* Set Interrrupts(INTx or MSI or MSI-X) */
	ap->msi_flag = hfc_set_interrupts(ap, ap->msi_enable);
	if(ap->msi_flag < 0){ /* err */
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4F, (uchar *)&ap->msi_flag, 4) ;/* FCLNX-GPL-161 */
		goto hfc_irq_error;
	}
	
	/* Don't get ADAPLOCK before calling this function */
	rtn =  hfc_skip_link_init(ap, bind_err);	/* FCLNX-GPL-521 */
	if( rtn == 1 ) /* FCLNX-GPL-306  */
	{
		goto skip_rport_start;
	}
	else if( rtn == 2 ){
//		if( hfc_rport_lu_scan == 1) {
//			hfc_lu_scan_start(ap);	/* FCLNX-GPL-521,565 *//* FCLNX-GPL-575 */
//		}
		goto skip_link_init;	/* FCLNX-GPL-575 */
	}	/* FCLNX-GPL-521 */
	
	HFC_DBGPRT( " ** hfcldd : hfc_detect - start hfc_initialize \n"); 
	hfc_initialize(ap, 0);				/* FCLNX-0514 */
	
	HFC_ADAPLOCK_IRQSAVE(flags);										/* FCLNX-GPL-349 */
	if(ap->hba_isolation == HFC_ISOL_START){
		if(!(hfc_check_hba_isolation(ap))){ 										/* FCLNX-GPL-349 */
			ap->hba_isolation = HFC_ISOL_STOP;
		}
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);									/* FCLNX-GPL-349 */

	if(hfc_manage_info.hfcldd_mp_mod) {
		hfc_manage_info.npubp->hfc_make_lgpath();								/* FCLNX-GPL-204 */
	}
	
	HFC_DBGPRT(" hfcldd : hfc_scan_host\n");
	
	if(hfc_manage_info.hfcldd_mp_mod) {											/* FCLNX-GPL-204 */
		if(vrt_host_alloc == HFC_VRTHOST_ALLOC_SUCCS)							/* FCNLNX-GPL-473 */
			hfc_manage_info.npubp->hfc_mp_scsi_scan_host(host, ap, hsd_host, pfb_host);			/* FCLNX-0429 */
	}
	
	scsi_scan_host(host);
	
	if(hfc_manage_info.hfcldd_mp_mod) {
		hfc_manage_info.npubp->hfc_mp_scsi_host_rescan();						/* FCLNX-GPL-204 */
	}

skip_link_init:

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if ( !hfc_manage_info.hfcldd_mp_mod ) {
		if (!test_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status )) { /* FCLNX-GPL-FX-491 */
			hfc_lu_scan_start(ap);
		}
	}
#endif
#endif /* SYSFS_SUPPORT */

skip_rport_start:

	if(HFC_MMODE_CHECK_SHADOW(ap))	/* FCLNX-GPL-393 */
	{
		hfc_mlpf_check_isol_support(ap);
	}								/* FCLNX-GPL-393 */
	
/* FCLNX-GPL-505 */
	if(HFC_MMODE_CHECK_SHARED(ap)){	/* FCLNX-GPL-489 */
		hfc_mlpf_change_state(ap, HFC_HG_LPAR_LIVEMIG_SUPPORT, HFC_ENABLE_DRV_SUPPORT);
	}								/* FCLNX-GPL-489 */
/* FCLNX-GPL-505 */
	
	HFC_EXIT("hfc_detect");
	
	return 0;

hfc_irq_error:
error_disable_adapter:

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_stop_rport(ap); /* FCLNX-GPL-306 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	if(reg_chrdev != 0){
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
		unregister_chrdev(hfc_major, "hfcldd");
#endif
	}

hfc_attach_error:
//	if(hfc_manage_info.hfcplus_enable){	/* FCLNX-0506 */
	if ( hfc_manage_info.hfcldd_mp_mod ){	/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_free_errcnt_info(ap);		/* FCLNX-0506 */
	}
hfc_pktype_error:
	pci_release_regions(pdev);
	iounmap((void *)(ap->mem_base_addr));

hfc_probe_error:
	if (hsd_host != NULL) {												/* FCLNX-0429 */
		hfc_manage_info.npubp->hfc_mp_scsi_host_put(hsd_host);			/* FCLNX-0429 */
	}
	
	hfc_scsi_host_put(host);
		
hfc_host_alloc_error:
	pci_disable_device(pdev);

hfc_probe_enable_error:

	return error;
}

/*
 * Function: hfc_remove_one
 *
 * Purpose: Driver release 
 *
 * Arguments:
 *  pdev	 - Pointer to pdev 
 *
 * Returns: -
 *
 * Notes:
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,8,0)
void
#else
void __devexit
#endif
hfc_remove_one(struct pci_dev *pdev)
{
	if( ( pdev->device == HFC_PCI_DEVICE_ID_300A )		/* FIVE 1 port 0x300A */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300B )		/* FIVE 2 port 0x300B */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300C )		/* FIVE		   0x300C */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_300D )		/* FIVE		   0x300D */
	 ||	( pdev->device == HFC_PCI_DEVICE_ID_3020) ){	/* FIVE-EX	   0x3020 */
		hfc_ex_remove_one( pdev );
	}
	else{
		hfc_fx_remove_one( pdev );
	}

}
/*
 * Function: hfc_ex_remove_one
 *
 * Purpose: 4/8 Gbps FC-HBA Driver release 
 *
 * Arguments:
 *  pdev	 - Pointer to pdev 
 *
 * Returns: -
 *
 * Notes:
 */
void
hfc_ex_remove_one(struct pci_dev *pdev)
{
	struct adap_info *ap;
	unsigned long	flags = 0;
	
	struct Scsi_Host *host = pci_get_drvdata(pdev);
	
	ap = (struct adap_info *)host->hostdata;

	HFC_DBGPRT("hfcldd hfcl_remove_one\n");

	HFC_ADAPLOCK_IRQSAVE(flags);
	if( hfc_pcibus_chk(ap) != 0 )
	{	/* "PCI BUS ERR" has happen. */
		if( !test_bit(HFC_CHK_STOP, (ulong *)&ap->status ) ){
			HFC_ISSUE_CSTP_PCIERR(ap, FALSE);
		}
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	fc_remove_host(host);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	scsi_remove_host(host);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_stop_rport(ap); /* FCLNX-GPL-306 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	if(hfc_manage_info.hfcldd_mp_mod) {									/* FCLNX-GPL-204 */
		hfc_manage_info.npubp->hfc_mp_scsi_remove_host(host);			/* FCLNX-0429 */
	}
	
	hfc_release(host);
	pci_disable_device(pdev);
	hfc_scsi_host_put(host);

	pci_set_drvdata(pdev,NULL);
}

#if 0																	/* FCLNX-GPL-204 */
int hfc_probe_internal(struct adap_info *ap)
{
	int rtn=0;
	
	/* loading hfcldd_conf.o */

	hfclddconf = (char *)hfcldd_cnf();

	if (!hfclddconf) {
		HFC_DBGPRT("hfcldd%d : hfc_probe_internal - hfclddconf pointer = 0.\n",ap->dev_minor);
	}


	hfc_param_setup();

	if ( (ap->pkg.vender_id == (ushort) hfc_read_cnfg (ap, HFC_HOST_VENDER_ID, 0x2))
	  && (ap->pkg.device_id == (ushort) hfc_read_cnfg (ap, HFC_HOST_DEVICE_ID, 0x2)) ) {
		uchar code,reg;
		
		code = ap->pkg.code;
		reg  = (uchar) hfc_read_reg_ext(ap, 0x005, 0x1);

		if (code == 0x86)
			code &= 0xfb;

		if (reg  == 0x86)
			reg  &= 0xfb;

		if (code != reg)
			rtn = EIO;
	}
	else {
		rtn = EIO;
	}

	if (rtn)
		return (rtn);

	hfc_get_adapter_port_no(ap);		/* Read adapter port# */
	if( hfc_read_hfcbios(ap) ){				/* Read hbabios data */
		return (EIO); /* FCLNX-GPL-116 */
	}
	hfc_search_adapter_number(ap);		/* Read wwn,add wwn  */
	hfc_conf_setup(ap);					/* Read conf         */

	/* Area allocation required (except DMA area ) */
	HFC_DBGPRT( "  hfcldd : hfc_probe_internal - allocate memory area\n"); 
	if ( hfc_allocate_memory(ap, TRUE) ) {
		HFC_ERRPRT("hfcldd%d :  Failed to allocate adapter memory resource.\n",ap->dev_minor);
		return (EIO);
	}

	/* Allocate init_table, xob, xrb, mailbox, seg_info, soft_log (for dma) */

	HFC_DBGPRT( "  hfcldd : hfc_probe_internal - allocate DMA area\n"); 
	if ( hfc_allocate_dma(ap) ) {
		HFC_ERRPRT("hfcldd%d :  Failed to allocate adapter DMA resource.\n",ap->dev_minor);
		hfc_free_memory(ap, TRUE);
		return (EIO);
	}

	if ( hfc_start_adapter(ap) ) {
		HFC_DBGPRT( " ** hfcldd : hfc_probe_internal - Failed hfc_start_adapter.\n"); 
		hfc_free_dma(ap);
		hfc_free_memory(ap, TRUE);
		return (EIO);
	}

	/* Get Persistent Binding Infomation */
	if (ap->automap == 0) {
		hfc_get_target_bindings(ap);
	}

	return (rtn);
}



int hfc_remove_internal(struct adap_info *ap)
{
	hfc_release_adp(ap);
	
	/* Release DMA memory area */
	hfc_free_dma(ap);
	
	/* Release adapter memory area */
	hfc_free_memory(ap, TRUE);
	return (0);
}
#endif																	/* FCLNX-GPL-204 */

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)

static void hfc_get_starget_node_name(struct scsi_target *starget);
static void hfc_get_starget_port_name(struct scsi_target *starget);
static void hfc_get_starget_port_id(struct scsi_target *starget);
static void hfc_get_host_port_id(struct Scsi_Host *host);
static void hfc_get_host_port_type(struct Scsi_Host *host);
static void hfc_get_host_port_state(struct Scsi_Host *host);
static void hfc_get_host_speed(struct Scsi_Host *host);
static struct fc_host_statistics * hfc_get_statistics(struct Scsi_Host *host);
static void hfc_reset_statistics(struct Scsi_Host *host);
static int hfc_issue_lip(struct Scsi_Host *host);

static void hfc_terminate_rport_io(struct fc_rport *rport);
static int hfc_vport_disable(struct fc_vport *fc_vport, bool disable);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
static void hfc_set_rport_loss_tmo(struct fc_rport *rport, uint32_t timeout); /* FCLNX-GPL-564 */
#endif

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
static void hfc_dev_loss_tmo_callbk(struct fc_rport *rport);
#endif

static void hfc_get_host_fabric_name(struct Scsi_Host *host);

struct fc_function_template hfcldd_fc_function_template = {

	.show_host_node_name=			1,
	.show_host_port_name=			1,
	.show_host_supported_classes=	1,
	.show_host_supported_fc4s=		1,
	.show_host_supported_speeds=	1,
	.show_host_maxframe_size= 		1,
	.get_host_port_id=				hfc_get_host_port_id,
	.show_host_port_id=				1,
	.get_host_port_type=			hfc_get_host_port_type,
	.show_host_port_type=			1,
	.get_host_port_state=			hfc_get_host_port_state,
	.show_host_port_state=			1,
	.get_host_speed=				hfc_get_host_speed,
	.show_host_speed=				1,
	.get_fc_host_stats=				hfc_get_statistics,
	.reset_fc_host_stats=			hfc_reset_statistics,
	
	.dd_fcrport_size=				sizeof(struct target_info),

	.get_starget_node_name=			hfc_get_starget_node_name,
	.show_starget_node_name=		1,
	.get_starget_port_name=			hfc_get_starget_port_name,
	.show_starget_port_name=		1,
	.get_starget_port_id=			hfc_get_starget_port_id,
	.show_starget_port_id=			1,
	
	.show_rport_dev_loss_tmo=		1,
	.issue_fc_host_lip=				hfc_issue_lip,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	.set_rport_dev_loss_tmo=		hfc_set_rport_loss_tmo, /* FCLNX-GPL-564 */
	.show_rport_dev_loss_tmo=		1,
#endif

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	.dev_loss_tmo_callbk=			hfc_dev_loss_tmo_callbk,
#endif	/* FCLNX-GPL-FX-424 */

	.get_host_fabric_name= 			hfc_get_host_fabric_name,
	.show_host_fabric_name= 		1,
	
	.terminate_rport_io=			hfc_terminate_rport_io,
	
	.dd_fcvport_size=				sizeof(struct port_info *),
	
	.vport_disable=					hfc_vport_disable,
};

struct fc_function_template hfcldd_vport_fc_function_template = {

	.show_host_node_name=			1,
	.show_host_port_name=			1,
	.show_host_supported_classes=	1,
	.show_host_supported_fc4s=		1,
	.show_host_supported_speeds=	1,
	.show_host_maxframe_size= 		1,
	.get_host_port_id=				hfc_get_host_port_id,
	.show_host_port_id=				1,
	.get_host_port_type=			hfc_get_host_port_type,
	.show_host_port_type=			1,
	.get_host_port_state=			hfc_get_host_port_state,
	.show_host_port_state=			1,
	.get_host_speed=				hfc_get_host_speed,
	.show_host_speed=				1,
	.get_fc_host_stats=				hfc_get_statistics,
	.reset_fc_host_stats=			hfc_reset_statistics,
	
	.dd_fcrport_size=				sizeof(struct target_info),

	.get_starget_node_name=			hfc_get_starget_node_name,
	.show_starget_node_name=		1,
	.get_starget_port_name=			hfc_get_starget_port_name,
	.show_starget_port_name=		1,
	.get_starget_port_id=			hfc_get_starget_port_id,
	.show_starget_port_id=			1,
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	.set_rport_dev_loss_tmo=		hfc_set_rport_loss_tmo, /* FCLNX-GPL-564 */
	.show_rport_dev_loss_tmo=		1,
#endif

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	.dev_loss_tmo_callbk=			hfc_dev_loss_tmo_callbk,
#endif	/* FCLNX-GPL-FX-424 */

	.get_host_fabric_name= 			hfc_get_host_fabric_name,
	.show_host_fabric_name= 		1,

	.terminate_rport_io=			hfc_terminate_rport_io,
	
	.dd_fcvport_size=				sizeof(struct port_info *),
	
	.vport_disable=					hfc_vport_disable,
};

void hfc_get_starget_node_name(struct scsi_target *starget)
{
	struct Scsi_Host *host;
	struct adap_info *ap;
	struct target_info *target;
	ulong flags = 0;

	host = dev_to_shost(starget->dev.parent);
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_starget_node_name");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_starget_node_name() - ap null\n");
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_starget_node_name(starget);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if (host != NULL) {
		target = hfc_hash_target_info(ap, starget->id );
		
		if (target != NULL) {
			fc_starget_node_name(starget) = target->node_name;
		}
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_starget_port_name(struct scsi_target *starget)
{
	struct Scsi_Host *host;
	struct adap_info *ap;
	struct target_info *target;
	ulong flags = 0;
	
	host = dev_to_shost(starget->dev.parent);
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_starget_port_name");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_starget_port_name() - ap null\n");
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_starget_port_name(starget);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if (host != NULL) {
		target = hfc_hash_target_info(ap, starget->id );
		
		if (target != NULL) {
			fc_starget_port_name(starget) = target->ww_name;
		}
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_starget_port_id(struct scsi_target *starget)
{
	struct Scsi_Host *host;
	struct adap_info *ap;
	struct target_info *target;
	ulong flags = 0;

	host = dev_to_shost(starget->dev.parent);
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_starget_port_id");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_starget_port_id() - ap null\n");
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return ;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_starget_port_id(starget);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if (host != NULL) {
		target = hfc_hash_target_info(ap, starget->id );
		
		if (target != NULL) {
			fc_starget_port_id(starget) = target->scsi_id;
		}
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_host_port_id(struct Scsi_Host *host)
{
	struct adap_info *ap;
	ulong flags = 0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_host_port_id");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_host_port_id() - ap null\n");
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return ;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_host_port_id(host);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	fc_host_port_id(host) = ap->scsi_id;
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_host_port_type(struct Scsi_Host *host)
{
	struct adap_info *ap;
	ulong flags = 0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_host_port_type");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_host_port_type() - ap null\n");
		fc_host_port_type(host) = FC_PORTTYPE_UNKNOWN;
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_host_port_type(host);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if( ap->connect_type == HFC_PT2PT ){			/* PtoP & NotSwitch */
		fc_host_port_type(host) = FC_PORTTYPE_PTP ;
		HFC_DBGPRT("hfc_get_host_port_type() - FC_PORTTYPE_PTP\n");
	}
	else if ( ap->connect_type == HFC_SWITCH ){		/* PtoP & Switch */
		fc_host_port_type(host) = FC_PORTTYPE_NPORT ;
		HFC_DBGPRT("hfc_get_host_port_type() - FC_PORTTYPE_NPORT\n");
	}
	else if ( ap->connect_type == HFC_AL ){
		if ( ap -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
			fc_host_port_type(host) = FC_PORTTYPE_NLPORT ;
			HFC_DBGPRT("hfc_get_host_port_type() - FC_PORTTYPE_NLPORT\n");
		}
		else {										/* AL & NotSwitch */
			fc_host_port_type(host) = FC_PORTTYPE_LPORT ;
			HFC_DBGPRT("hfc_get_host_port_type() - FC_PORTTYPE_LPORT\n");
		}
	}
	else {
		fc_host_port_type(host) = FC_PORTTYPE_UNKNOWN ;
		HFC_DBGPRT("hfc_get_host_port_type() - FC_PORTTYPE_UNKNOWN\n");
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_host_port_state(struct Scsi_Host *host)
{
	struct adap_info *ap;
	ulong flags = 0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_port_state");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_host_port_state() - ap null\n");
		fc_host_port_state(host) = FC_PORTSTATE_UNKNOWN;
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_host_port_state(host);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if( test_bit(HFC_HWCHKSTOP , (ulong *)&ap->mp_adap_info->status) ) {
		fc_host_port_state(host) = FC_PORTSTATE_ERROR ;
		HFC_DBGPRT("hfc_get_host_port_state() - FC_PORTSTATE_ERROR\n");
	}
	else if( !test_bit(HFC_ONLINE , (ulong *)&ap->status) ) {
		fc_host_port_state(host) = FC_PORTSTATE_LINKDOWN ;
		HFC_DBGPRT("hfc_get_host_port_state() - FC_PORTSTATE_LINKDOWN_1\n");
	}
	else if( test_bit(HFC_WAIT_LINKUP,(ulong *)&ap->status ) ) {
		fc_host_port_state(host) = FC_PORTSTATE_LINKDOWN ;
		HFC_DBGPRT("hfc_get_host_port_state() - FC_PORTSTATE_LINKDOWN_2\n");
	}
	else {
		fc_host_port_state(host) = FC_PORTSTATE_ONLINE ;
		HFC_DBGPRT("hfc_get_host_port_state() - FC_PORTSTATE_ONLINE\n");
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}

void hfc_get_host_speed(struct Scsi_Host *host)
{
	struct adap_info *ap;
	ulong flags = 0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_host_speed");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_host_speed() - ap null\n");
		fc_host_port_type(host) = FC_PORTSPEED_UNKNOWN;
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_host_speed(host);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	if( !test_bit(HFC_ONLINE, (ulong *)&ap->status) ) {
		fc_host_speed(host) = FC_PORTSPEED_UNKNOWN ;
		HFC_DBGPRT("hfc_get_host_speed() - HFC_ONLINE\n");
	}
	else if( ap->max_data_rate == HFC_100MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_1GBIT ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_1GBIT\n");
	}
	else if( ap->max_data_rate == HFC_200MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_2GBIT ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_2GBIT\n");
	}
	else if( ap->max_data_rate == HFC_400MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_4GBIT ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_4GBIT\n");
	}
	else if( ap->max_data_rate == HFC_800MBS ) {
		fc_host_speed(host) = 0x10 ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_8GBIT\n");
	}
	else if( ap->max_data_rate == HFC_1000MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_10GBIT ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_10GBIT\n");
	}
	else {
		fc_host_speed(host) = FC_PORTSPEED_UNKNOWN ;
		HFC_DBGPRT("hfc_get_host_speed() - FC_PORTSPEED_UNKNOWN\n");
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
}


void hfc_get_host_fabric_name(struct Scsi_Host *host)
{
	struct adap_info *ap;
	u64 node_name =0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_host_fabric_name");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_host_fabric_name() - ap null\n");
		fc_host_port_type(host) = FC_PORTSPEED_UNKNOWN;
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_get_host_fabric_name(host);
		return;
	}
	
	fc_host_fabric_name(host) = node_name;
	
	return;
}

struct fc_host_statistics *hfc_get_statistics(struct Scsi_Host *host)
{
	struct adap_info	*ap;
	int					rtn = 0;
	ulong				seconds;
	ulong 				flags = 0;

	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_get_statistics");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_get_statistics() - ap null\n");
		return NULL;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return 0;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_get_statistics(host));
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if ( !test_bit(HFC_SYSFS_STATISTICS, (ulong *)&ap->sysfs_control) ) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return NULL;
	}

	if ( ap->fw_init_p->func2 & HFC_FWF_STATCCA ) {		/* FCLNX-GPL-261 */
		/* Confirms new statistical information of F/W in CCA */
		hfc_hba_port_statistics_new( ap );
	}
	else {
		/* When F/W unsupports new statistical information, Mailbox is used. */
		if ( !(lock_try_mailbox( ap )) ) {
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			HFC_DBGPRT("hfc_get_statistics() - mailbox lock fail\n");
			return NULL;
		}

		/* Create a mailbox control block based on CHNGRNID(DRVLOGB0) */
		ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;
		ap->mb->mb_init.sub_cmd = HFC_MBSCMD_CTLPORTSTAT;
		ap->mb_results = 0;
		hfc_write_val( ap->mb->mb_init.dependent_code, 0x0000 );

		HFC_ADAPUNLOCK_IRQRESTORE(flags);

		/* Mailbox processing */ /* FCLNX-GPL-243 */
		if( ( rtn = hfc_mailbox_proc( ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry ) ) == 0 ) {
			seconds = get_seconds();

			HFC_ADAPLOCK_IRQSAVE(flags);

			if (seconds < ap->reset_stat_time)
				ap->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - ((uint64_t)1 + (uint64_t)ap->reset_stat_time));
			else
				ap->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - (uint64_t)ap->reset_stat_time);

			ap->port_statistics.tx_frames =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.tx_frames );
			ap->port_statistics.tx_words =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.tx_words );
			ap->port_statistics.rx_frames =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.rx_frames );
			ap->port_statistics.rx_words =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.rx_words );
			ap->port_statistics.lip_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.lip_count );
			ap->port_statistics.nos_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.nos_count );
			ap->port_statistics.error_frames =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.error_frames );
			ap->port_statistics.dumped_frames =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.dumped_framed );
			ap->port_statistics.link_failure_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.link_failure_count );
			ap->port_statistics.loss_of_sync_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.loss_of_sync_count );
			ap->port_statistics.loss_of_signal_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.loss_of_signal_count );
			ap->port_statistics.prim_seq_protocol_err_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.primitive_seq_protocol_err_count );
			ap->port_statistics.invalid_tx_word_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.invalid_tx_word_count );
			ap->port_statistics.invalid_crc_count =
					(uint64_t)hfc_read_val( ap->mb->mb_resp.type.drvlogb0.control_portstatistics.invalid_crc_count );
		}
		else {
			HFC_ADAPLOCK_IRQSAVE(flags);
		}
	
		unlock_mailbox( ap );	/* Unlock mailbox */
	}							/* FCLNX-GPL-261 */

	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	if (rtn) {
		HFC_DBGPRT("hfc_get_statistics() - mailbox fail, rtn = 0x%08x\n", rtn);
		return NULL;
	}
	else {
		return &ap->port_statistics;
	}
}

void hfc_reset_statistics(struct Scsi_Host *host)
{
	struct adap_info	*ap;
	int					rtn = 0;
	ulong 				flags = 0;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_reset_statistics");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_reset_statistics() - ap null\n");
		return;
	}
	
	if((!strcmp(ap->name, "virt_adap_info"))||(!strcmp(ap->name, "lport_info"))) {	/* FCLNX-GPL-FX-483 >>> */
		return;
	}	/* >>> FCLNX-GPL-FX-483 */

	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		hfc_fx_reset_statistics(host);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if ( !test_bit(HFC_SYSFS_STATISTICS, (ulong *)&ap->sysfs_control) ) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return ;
	}
	
	if ( !(lock_try_mailbox( ap )) ) {
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		HFC_DBGPRT("hfc_reset_statistics - mailbox lock fail\n");
		return ;
	}
	
	/* Create a mailbox control block based on CHNGRNID(DRVLOGB0) */
	ap->mb->mb_init.command = HFC_MBCMD_LOGTRACE;
	ap->mb->mb_init.sub_cmd = HFC_MBSCMD_CTLPORTSTAT;
	ap->mb_results = 0;
	hfc_write_val( ap->mb->mb_init.dependent_code, 0x8000 );
	
	/* Reset I/O Statistical information */ /* FCLNX-GPL-261 */
	ap->tx_frames = 0;
	ap->tx_words  = 0;
	ap->rx_frames = 0;
	ap->rx_words  = 0;
	/* FCLNX-GPL-261 */
	
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	/* Mailbox processing */ /* FCLNX-GPL-243 */
	if( ( rtn = hfc_mailbox_proc( ap, HFC_MB_TMR, HFC_MB_PROC_TO, ap->els_retry ) ) == 0 ) {
		
		HFC_ADAPLOCK_IRQSAVE(flags);
		
		ap->reset_stat_time = get_seconds();
	}
	else {
		HFC_ADAPLOCK_IRQSAVE(flags);
	}
	
	unlock_mailbox( ap );	/* Unlock mailbox */
	
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	if (rtn) {
		HFC_DBGPRT("hfc_reset_statistics() - mailbox fail, rtn = 0x%08x\n", rtn);
		return;
	}
	else {
		return;
	}
}

int hfc_issue_lip(struct Scsi_Host *host)
{
	struct adap_info	*ap;
	
	ap = (struct adap_info *)host->hostdata;
	
	HFC_ENTRY("hfc_reset_statistics");
	
	if (ap == NULL) {
		HFC_DBGPRT("hfc_issue_lip() - ap null\n");
		return -EIO;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return hfc_fx_issue_lip(host);
	}
	return -ENOTTY;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
/* FCLNX-GPL-564 start */
static void
hfc_set_rport_loss_tmo(struct fc_rport *rport, uint32_t timeout)
{
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
		if (timeout)
		{
			if(timeout > INT_MAX )
				rport->dev_loss_tmo = INT_MAX;
			else
				rport->dev_loss_tmo = timeout;
		}
		else
		{
			rport->dev_loss_tmo = 1;
		}
	}
#endif
	
	return;
}
/* FCLNX-GPL-564 end */
#endif

/* FCLNX-GPL-FX-424 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
void hfc_dev_loss_tmo_callbk(struct fc_rport *rport)
{
	struct Scsi_Host *host = rport_to_shost(rport);
	struct adap_info		*ap = NULL;		/* Pointer to an adap_info	*/
	struct target_info		*target = *(struct target_info **)rport->dd_data; /* Pointer to a target_info	*/
	ulong flags = 0;

	if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) /* FCLNX-GPL-FX-472 */
		return ;

	if (!host){ /* This host is not found */
		return ;
	}

	ap = (struct adap_info *)host->hostdata;
	if (!ap){ /* This adap_info is not found */
		return ;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return (hfc_fx_dev_loss_tmo_callbk(host, rport));
	}
	
	/* FIVE-EX */
	HFC_ADAPLOCK_IRQSAVE(flags);
	if( target ){
		clear_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
	}
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	return;
}
#endif


void hfc_terminate_rport_io(struct fc_rport *rport)
{
	return;
}

int hfc_vport_disable(struct fc_vport *fc_vport, bool disable)
{
	struct adap_info	*ap;
	
	HFC_ENTRY("hfc_vport_disable");
	
	ap = *(struct adap_info **)fc_vport->dd_data;
	if (ap == NULL) {
		HFC_DBGPRT( "hfcldd : hfc_vport_disable - ap null"); 
		return -EIO;
	}
	
	if (!strcmp(ap->name, "port_info")) {
		/* FIVE-FX */
		return hfc_fx_vport_disable(fc_vport, disable);
	}
	return -ENOTTY;
}


void hfc_rport_add(struct adap_info *ap, struct target_info *target)
{
	struct fc_rport *rport;
	struct fc_rport_identifiers rport_ids;
	
	HFC_ENTRY("hfc_rport_add");
	
	if (!target) { /* FCLNX-GPL-205 */
		HFC_DBGPRT("hfc_rport_add() - target null\n");
		return;
	}
	
	if (target->rport != NULL) {
		HFC_DBGPRT("hfc_rport_add() - rport not null\n");
		return;
	}
	
	rport_ids.node_name = target->node_name;
	rport_ids.port_name = target->ww_name;
	rport_ids.port_id = target->scsi_id;
	rport_ids.roles = FC_RPORT_ROLE_UNKNOWN;
	
	rport = fc_remote_port_add(ap->hosts, 0, &rport_ids);
	clear_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
	
	target->rport = rport;
	
	if (!rport) {
		HFC_DBGPRT("hfc_rport_add() - fc_remote_port_add failed\n");
		return;
	}
	
#if !( defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7) )
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ) /* FCLNX-GPL-FX-472 */
		rport->dev_loss_tmo = ap->dev_loss_tmo;	/* FCLNX-GPL-260 */
#endif
	
	rport_ids.roles |= FC_RPORT_ROLE_FCP_TARGET;
	
	if (rport_ids.roles !=  FC_RPORT_ROLE_UNKNOWN) {
		fc_remote_port_rolechg(rport, rport_ids.roles);
	}
	
	*((struct target_info **)rport->dd_data) = target;
	
//	HFC_DBGPRT("hfc_rport_add() - port_state=%d, rid=%d\n", (uint)rport->port_state, ap->rid);
	
	HFC_EXIT("hfc_rport_add");
}

void hfc_rport_delete(struct target_info *target)
{
	HFC_ENTRY("hfc_rport_delete");
	HFC_ERRPRT("  \n");
	
	if (!target) { /* FCLNX-GPL-205 */
		HFC_DBGPRT("hfc_rport_delete() - target null\n");
		return;
	}
	
	if( target->rport != NULL ){
		target->dev_loss_tmo = target->rport->dev_loss_tmo;
		target->fast_io_fail_tmo = target->rport->fast_io_fail_tmo;
	}
	
	if ((target->rport != NULL) && 	!test_bit(HFC_WAIT_CLOSE, (ulong *)&target->ap->status)) {
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.lg_target_info ) ){ /* FCLNX-GPL-FX-472 */
			if( test_bit( HFC_CHK_STOP, (ulong *)&target->ap->status) || test_bit( HFC_ISOL, (ulong *)&target->ap->status) )
			{
				target->rport->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
			}
		}
#endif
#ifdef HFC_UBUNTU
		set_bit(HFC_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
		fc_remote_port_delete(target->rport);
		target->rport = NULL;
		HFC_ERRPRT("  \n");
	}
	
	HFC_EXIT("hfc_rport_delete");
}

void hfc_fc_host_init(struct Scsi_Host *host, struct adap_info *ap)
{
	fc_host_node_name(host)			= ap->node_name;
	fc_host_port_name(host)			= ap->ww_name;
	fc_host_supported_classes(host) = HFC_SUPPORT_CLASS;
	
	memset(fc_host_supported_fc4s(host), 0,sizeof(fc_host_supported_fc4s(host)));
	fc_host_supported_fc4s(host)[2] = 0x01;
	
	fc_host_supported_speeds(host) = 0;
	if( ap->pkg.type == HFC_PKTYPE_FPP )
		fc_host_supported_speeds(host) |= (	FC_PORTSPEED_1GBIT	|
											FC_PORTSPEED_2GBIT	);
	else if( ap->pkg.type == HFC_PKTYPE_FIVE )
		fc_host_supported_speeds(host) |= (	FC_PORTSPEED_1GBIT	|
											FC_PORTSPEED_2GBIT	|
											FC_PORTSPEED_4GBIT	);
	else /* FIVE-EX */
		fc_host_supported_speeds(host) |= (	FC_PORTSPEED_1GBIT	|
											FC_PORTSPEED_2GBIT	|
											FC_PORTSPEED_4GBIT	|
											0x10	);	/* FC_PORTSPEED_8GBIT */

	fc_host_maxframe_size(host) = HFC_PORT_MAX_FRAME;
	fc_host_max_npiv_vports(host) = 0;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	fc_host_dev_loss_tmo(host) = ap->dev_loss_tmo;	/* FCLNX-GPL-564 */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

	ap->reset_stat_time = get_seconds();
}
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

void hfc_reset_all_timer (struct adap_info *ap)
{
	int						i;
	struct target_info		*target;
	struct wtimer			*w_timer;
	struct dev_info			*dev;	/* FCLNX-GPL-353 */

	/* Stop timer (adap_info) */
	for(i=0; i< HFC_MAX_TMR ; i++){
		switch(i){
			case HFC_ELS_TMR :
			case HFC_LINKINIT_TMR :
			case HFC_MB_TMR :
				w_timer = &ap->mb_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
				
			case HFC_REBOOT_DELAY_TMR :
			case HFC_DIAG_DELAY_TMR :
				w_timer = &ap->reboot_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN0_TMR :
				w_timer = &ap->loop_dev_info[0].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN1_TMR :
				w_timer = &ap->loop_dev_info[1].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN2_TMR :
				w_timer = &ap->loop_dev_info[2].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			
			case HFC_LUN3_TMR :
				w_timer = &ap->loop_dev_info[3].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN4_TMR :
				w_timer = &ap->loop_dev_info[4].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN5_TMR :
				w_timer = &ap->loop_dev_info[5].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN6_TMR :
				w_timer = &ap->loop_dev_info[6].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LUN7_TMR :
				w_timer = &ap->loop_dev_info[7].loop_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_LINKUP_TMR     :
			case HFC_LINKUP2_TMR    :								/* FCLNX-241*/
				w_timer = &ap->linkup_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_TARGET_RST_TMR: 
				w_timer = &ap->loop_dev_info[0].loop_wdog;
				break;												/* FCLNX-0274 */

			case HFC_WLINKUP_CNT_TMR    :							/* FCLNX-GPL-FX-424 */
				w_timer = &ap->ld_err_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_ABORT_TMR: 
				w_timer = &ap->loop_dev_info[0].loop_wdog;
				break;												/* FCLNX-0274 */

			case HFC_SCSI_CMD_TMR:
				w_timer = &ap->loop_dev_info[0].loop_wdog;
				break;

			case HFC_MCK_DELAY_TMR :
				w_timer = &ap->mck_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_MCKINT_TMR :
				w_timer = &ap->mckint_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			
			case HFC_LOGIN_DELAY_TMR :								/* FCLNX-0270 */
				w_timer = &ap->lgdelay_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;												/* FCLNX-0270 */

			case HFC_MLPF_FMCK_TMR :
				w_timer = &ap->fmck_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_MLPF_MCKEND_TMR :
				w_timer = &ap->mckend_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;

			case HFC_MLPF_FCSTP_TMR :
				w_timer = &ap->fcstp_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_LDLERR_TMR :	/*FCLNX-0506*/
				w_timer = &ap->ldlerr_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_LDSERR_TMR :	/*FCLNX-0506*/
				w_timer = &ap->ldserr_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_IFERR_TMR :	/*FCLNX-0506*/
				w_timer = &ap->iferr_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_TOERR_TMR :	/*FCLNX-0506*/
				w_timer = &ap->toerr_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_ISOLATE_DELAY_TMR :
				w_timer = &ap->fwisol_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			case HFC_MLPF_ISOLEND_TMR :
				w_timer = &ap->isolend_wdog;
				if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, NULL, NULL, 0, i, 0,1);
				break;
			default:
				break;
		}
	}

	for (i=0;i<ap->max_target;i++){
		target = ap -> target_arg[i];

		/* Cancel scn linkup timer */	
		w_timer = &target->scnlinkup_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
		hfc_watchdog_enter( ap, target, NULL, 0, HFC_SCN_LINKUP_TMR, 0,1);

		/* Cancel delay timer */
		w_timer = &target->delay_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
		hfc_watchdog_enter( ap, target, NULL, 0, HFC_DELAY_TMR, 0,1);

		/* cancel wexec tmer */
		w_timer = &target->wexec_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, target, NULL, 0, HFC_WEXEC_TMR, 0,1);

		/* Cancel isolate timer for Target*/	/*FCLNX-GPL-327*/
		w_timer = &target->tgt_ldlerr_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_watchdog_enter( ap, target, NULL, 0, HFC_TGT_LDLERR_TMR, 0,1);

		w_timer = &target->tgt_ldserr_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_watchdog_enter( ap, target, NULL, 0, HFC_TGT_LDSERR_TMR, 0,1);
		/*FCLNX-GPL-327*/

		/* cancel restart tmer */													/* FCLNX-GPL-328	*//* FCLNX-GPL-353 */
		w_timer = &target->restart_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
					hfc_watchdog_enter( ap, target, NULL, 0, HFC_RESTART_TMR, 0,1);

		dev = target->dev;															/* FCLNX-GPL-038	*/
		if(dev != NULL) {
			/* stop LUN Reset Delay Timer */
//			hfc_manage_info.npubp->hfc_all_clear_dev_info(ap, dev);					/* FCLNX-GPL-0343	*/
			hfc_all_clear_dev_info(ap, dev);										/* FCLNX-GPL-0343	*/
		}																			/* FCLNX-GPL-038	*/
	}
}

/*
 * Function:    hfc_set_interrupts
 *
 * Purpose:     This function sets interrupts.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *  type       - INT type ( INTx, MSI, MSI-X )
 *
 * Returns:     Running INT type.
 *                0: INTx
 *                1: MSI
 *                2: MSI-X
 *               -1: Faild
 *
 * Notes:       
 */
int hfc_set_interrupts(struct adap_info *ap, int type)
{
	/* Constants */
	const uchar pkg_type	= ap->pkg.type;
	const uchar pass1		= 0x00;
	/* Values */
	int flagINTx = FALSE; /* FALSE:don't use INTx, TRUE: use INTx */
	int msi_flag = -1; /* This function return "msi_flag". */
	struct pci_dev *pdev = ap->pci_cfginf;
	uint pos;
	uint wkc;
	int err;
	int i,j;
	uchar logdata[16];
	
	switch(type){
		/****** INT type is INTx ************************/
		case HFC_INT_TYPE_INTX:
			flagINTx = TRUE;
			break;
		
		/****** INT type is MSI *************************/
		case HFC_INT_TYPE_MSI:
			/* Judgement for refusing FIVE MSI */	/* FCLNX-GPL-228 start */
			if( ap->pkg.type == HFC_PKTYPE_FIVE )
			{	/* for only FIVE */
				if( ap->debug_func & HFC_DEBUG_FIVE_MSI )
				{	/* Permit using FIVE MSI */
					/* NOP */
				}
				else
				{	/* Refuse using FIVE MSI.*/
					memset(logdata, 0, 16);
					logdata[0] = 0x07 ;
					hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
					flagINTx = TRUE;
					break;
				}
			}										/* FCLNX-GPL-228 end */
			
			/* Search Capabitilies ID */
			pos = pci_find_capability(pdev, PCI_CAP_ID_MSI);
			if(pos == 0){	/* This HBA don't support MSI.*/
				memset(logdata, 0, 16);
				logdata[0] = 0x01 ;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				flagINTx = TRUE;
				break;
			}
			
			/* Enable MSI */
			err = pci_enable_msi(pdev);
			if(err != 0){	/* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x02 ;
				logdata[1] = (uchar)err ;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				flagINTx = TRUE;
				break;
			}
			
			/* Enable IRQ */
			err = request_irq(pdev->irq, hfc_intr, 0, "hfcldd", (void *)ap);
			if(err != 0){	/* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x03 ;
				logdata[1] = (uchar)err ;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				/* free MSI */
				pci_disable_msi(pdev);
				/* Try INTx */
				flagINTx = TRUE;
				break;
			}
			
			/* No err */
			msi_flag = HFC_INT_TYPE_MSI;
			break;
		
		/****** INT type is MSI-X ***********************/
		case HFC_INT_TYPE_MSIX:
			/*  kernel version */
#if			LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16)
				/* ToDo: Waraning Message(type:MSI-X) */
				flagINTx = TRUE; /* Don't support MSI-X.*/
				break;
#endif		/* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,16) */

			/* Search Capabitilies ID */
			pos = pci_find_capability(pdev, PCI_CAP_ID_MSIX);
			if(pos == 0){ /* This HBA don't support MSI-X.*/
				memset(logdata, 0, 16);
				logdata[0] = 0x04 ;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				flagINTx = TRUE;
				break;
			}
			
			/* Set msix_entry */
			for(i=0; i<HFC_MSIX_NVEC; i++){
				ap->entries[i].entry = i;
			}
			
			/* for FIVE-EX PASS1 only */
			if((pkg_type == HFC_PKTYPE_FIVE_EX)&&(ap->pkg.lsi_rev == pass1)){ /* FCLNX-GPL-220 */
				ap->entries[0].entry = 30;	/* Vec#30 */
				ap->entries[1].entry = 31;	/* Vec#31 */
			}
			
			/* Enable MSI-X */
			err = pci_enable_msix(pdev, ap->entries, HFC_MSIX_NVEC);
			if(err != 0){	/* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x05 ;
				logdata[1] = (uchar)err ;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				flagINTx = TRUE;
				break;
			}
			
			/* for FIVE-EX PASS1 only */
			if((pkg_type == HFC_PKTYPE_FIVE_EX)&&(ap->pkg.lsi_rev == pass1)){ /* FCLNX-GPL-220 */
				/* interrupt disable OFF */ /* Only "FIVE-EX pass1" needs this process. */
				wkc = (uint) hfc_read_cnfg(ap, HFC_HOST_STAT_CMD, 0x4);
				hfc_write_cnfg(ap, HFC_HOST_STAT_CMD, 0x2, (wkc & 0xfbff));
				/* change the endian of msix vector table */
				hfc_set_msix_table(ap);
			}
			
			/* Enable IRQ */
			for(i=0; i<HFC_MSIX_NVEC; i++){
				err = request_irq(ap->entries[i].vector, hfc_intr, 0, "hfcldd", (void *)ap);
				if(err != 0){	/* There are some err. */
					memset(logdata, 0, 16);
					logdata[0] = 0x06 ;
					logdata[1] = (uchar)err ;
					hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
					/* free IRQ */
					for(j=0; j<i; j++){
						free_irq(ap->entries[i].vector, ap);
					}
					/* free MSI-X */
					pci_disable_msix(pdev);
					/* Try INTx */
					flagINTx = TRUE;
					break;
				}
			}
			if(flagINTx == TRUE){
				break;
			}
			
			/* No err */
			msi_flag = HFC_INT_TYPE_MSIX;
			break;

		/****** INT type is unknown *******************/
		default:
			/* Nop */
			break;
	}

	/* Enable INTx */
	if(flagINTx == TRUE){
		/* Enable IRQ*/
//		err = request_irq(pdev->irq, hfc_intr, IRQF_DISABLED|IRQF_SHARED, "hfcldd", (void *)ap);
		err = request_irq(pdev->irq, hfc_intr, IRQF_SHARED, "hfcldd", (void *)ap);
		if(err != 0){
			/* There are some err. */
			msi_flag = -1;
		}
		else{
			/* There are  No  err. */
			msi_flag = HFC_INT_TYPE_INTX;
		}
	}
	
	return msi_flag;
}

/*
 * Function:    hfc_free_interrupts
 *
 * Purpose:     This Function releases interrupts.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *  type       - INT type ( INTx, MSI, MSI-X )
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_free_interrupts(struct adap_info *ap, int type, int pci_fail)	/* FCLNX-GPL-306 */
{
	/* Constants */
	const uchar pkg_type	= ap->pkg.type;
	const uchar pass1		= 0x00;
	/* Values */
	struct pci_dev *pdev = ap->pci_cfginf;
	int i;
	
	switch(type){
		/****** INT type is INTx ***********************/
		case	HFC_INT_TYPE_INTX:
			if(pdev->irq){
				free_irq(pdev->irq, ap);
			}
			else{
				/* NOP */
			}
			break;
		
		/****** INT type is MSI ************************/
		case	HFC_INT_TYPE_MSI:
			free_irq(pdev->irq, ap);
			if(pci_fail == 0){	/* FCLNX-GPL-306 */
				pci_disable_msi(pdev);
			}
			break;
		
		/****** INT type is MSI-X *********************/
		case	HFC_INT_TYPE_MSIX:
			
			for(i=0; i < HFC_MSIX_NVEC; i++){
				free_irq(ap->entries[i].vector, ap);
			}

			/* for FIVE-EX PASS1 only */
			if((pkg_type == HFC_PKTYPE_FIVE_EX)&&(ap->pkg.lsi_rev == pass1)){ /* FCLNX-GPL-220 */
				hfc_reset_msix_table(ap);
			}

			if(pci_fail == 0){	/* FCLNX-GPL-306 */
				pci_disable_msix(pdev);
			}
			break;
			
		/****** INT type is unknown *******************/
		default:
			/* NOP */
			break;
	}
	
	return;
}

/*
 * Function:    hfc_send_msi_info
 *
 * Purpose:     This Function sends MSI information to FW.
 *				It's for FIVE-EX PASS1 only.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:
 *				It's for FIVE-EX pass1 only.
 *				This function do NOP for other type HBA.
 */
void hfc_send_msi_info(struct adap_info *ap)
{
	/* Constants */
	const uchar pkg_type		= ap->pkg.type;
	const uchar pass1			= 0x00;
	const uint msi_capability	= 0x48;
	const uint offset			= 0x60;
	const uint data_length		= 0x04;
	/* Values */
	struct pci_dev *pdev = ap->pci_cfginf;
	int i;
	int err;
	uint addr;
	uint data;
	void *ptr;

	/* for FIVE-EX PASS1 only */
	if((pkg_type == HFC_PKTYPE_FIVE_EX)&&(ap->pkg.lsi_rev == pass1)){ /* FCLNX-GPL-220 */

		/* for MSI only */
		if(ap->msi_enable == HFC_INT_TYPE_MSI){
		
			/* Enable MSI */
			err = pci_enable_msi(pdev);
			if(err != 0){	/* There are some err. */
				HFC_DBGPRT("hfc_send_msi_info: pci_enable_msi() faild.\n");
			}
			
			/* Send "MSI info" to fw_init_table from CFG space. */
			for(i=0; i<4; i++){
				/* read MSI information from CFG space */
				addr = msi_capability + data_length * i;
				data = (uint)hfc_read_cnfg(ap, addr, data_length);
				
				/* write MSI information to "fw_init_tbl, offset:0x60". */
				ptr = (void *)((uchar *)ap->fw_init_p + (offset + data_length * i));
				hfc_write_tbl( ptr, data_length, data);
			}
			
			/* Disable MSI */
			pci_disable_msi(pdev);
		}
		else{
			/* NOP */
		}
	}
	else{
		/* NOP */
	}

	return;
}


/*
 * Function:    hfc_set_msix_table
 *
 * Purpose:     This Function sets MSI-X vector tables.
 *				It's for FIVE-EX PASS1 only.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_set_msix_table(struct adap_info *ap)
{
	/* Constants */
	const uint vec0_addr		= 0x0c00;
	const uint vec18_addr		= 0x0d20;
	const uint vec30_addr		= 0x0de0;
	const uint vec31_addr		= 0x0df0;
	const uint msg_data_offset	= 0x08;
	const uint msg_length		= 0x10;
	/* Values */
	int		i;
	uint	addr;
	uint	msg_addr;
	uint	msg_data;

	/**** Change MSI-X Address Endian **************************/
	hfc_change_msix_addr(ap, vec30_addr);	/* use for XRB    */
	hfc_change_msix_addr(ap, vec31_addr);	/* use for others */
	
	/**** (XRB) Read MSI-X vector table. ***********************/
	addr = vec30_addr;
	msg_addr = hfc_read_reg_ext(ap, addr, 0x04);
	addr = addr + msg_data_offset;
	msg_data = hfc_read_reg_ext(ap, addr, 0x04);
	
	/**** (XRB) Copy MSI-X vector table. ***********************/
	/* vector#18 */
	addr = vec18_addr;
	hfc_write_reg_ext(ap, addr, 0x04, msg_addr);
	addr = addr + msg_data_offset;
	hfc_write_reg_ext(ap, addr, 0x04, msg_data);

	/**** (Others) Read MSI-X vector table. ********************/
	addr = vec31_addr;
	msg_addr = hfc_read_reg_ext(ap, addr, 0x04);
	addr = addr + msg_data_offset;
	msg_data = hfc_read_reg_ext(ap, addr, 0x04);

	/**** (Others) Copy MSI-X vector table. ********************/
	for(i=0; i<18; i++){
		/* vector#0 - vector#17 */
		addr = vec0_addr + msg_length * i;
		hfc_write_reg_ext(ap, addr, 0x04, msg_addr);
		addr = addr + msg_data_offset;
		hfc_write_reg_ext(ap, addr, 0x04, msg_data);
	}
	for(i=19; i<30; i++){
		/* vector#19 - vector#29 */
		addr = vec0_addr + msg_length * i;
		hfc_write_reg_ext(ap, addr, 0x04, msg_addr);
		addr = addr + msg_data_offset;
		hfc_write_reg_ext(ap, addr, 0x04, msg_data);
	}
	
	return;
}

/*
 * Function:    hfc_reset_msix_table
 *
 * Purpose:     This Function resets MSI-X vector tables.
 *				It's for FIVE-EX PASS1 only.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_reset_msix_table(struct adap_info *ap)
{
	/* Constants */
	const uint vec0_addr		= 0x0c00;
	const uint msg_data_offset	= 0x08;
	const uint msg_length		= 0x10;
	const uint all_zero			= 0x00000000;
	/* Values */
	int		i;
	uint	addr;
	uint	read_data;
	uint	write_data;

	/****** Delete MSI-X vector table. *******************/
	for(i=0; i<30; i++){
		/* vector#0 - vector#29 */
		addr = vec0_addr + msg_length * i;
		hfc_write_reg_ext(ap, addr, 0x04, all_zero);
		addr = addr + msg_data_offset;
		hfc_write_reg_ext(ap, addr, 0x04, all_zero);
	}

	/**** Change MSI-X Address Endian **************************/
	/* for vector#30 and vector#31 */
	for(i=30; i<32; i++){
		addr = vec0_addr + msg_length * i;
		/* Read MSI-X vector from PCI memory space */
		read_data = hfc_read_reg_ext(ap, addr, 0x04);
		/* Change Endian */
		HFC_4B_TO_4L(write_data, read_data);
		/* Write MSI-X vector to PCI memory space */
		hfc_write_reg_ext(ap, addr, 0x04, write_data);
	}

	return;
}

/*
 * Function:    hfc_change_msix_addr
 *
 * Purpose:     This Function changes the Endian of MSI-X msg_address.
 *				It's for FIVE-EX PASS1 only.
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_change_msix_addr(struct adap_info *ap, uint addr)
{
	/* Values */
	uint read_data;
	uint write_data;

	/* Read MSI-X vector from PCI memory space */
	read_data = hfc_read_reg_ext(ap, addr, 0x04);
	/* Change Endian */
	HFC_4B_TO_4L(write_data, read_data);
	/* Create MSI-X addr */
	write_data &= 0x000fffff;
	write_data |= 0xfee00000; /* MSI_ADDRESS_HEADER is 0xfee */
	/* Write MSI-X vector to PCI memory space */
	hfc_write_reg_ext(ap, addr, 0x04, write_data);

	return;
}

/*
 * Function:    hfc_clear_status_five_ex
 *
 * Purpose:
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_clear_status_five_ex(struct adap_info *ap)
{
	/* Stock these data for analyzing. */
	/* We can read-out these, if we used "dddu" or some "tools". */
	ap->hw_init_status0 = hfc_read_reg(ap,HFC_IOSPACE_STATUS0, 0x4 );	/* Read-out:STATUS 12byte */
	ap->hw_init_status1 = hfc_read_reg(ap,HFC_IOSPACE_STATUS1, 0x4 );
	ap->hw_init_detail0 = hfc_read_reg(ap,HFC_IOSPACE_ERRDETAIL0, 0x4 );
	ap->hw_init_pcierr  = hfc_read_reg_ext(ap, 0x13b0, 0x4 );			/* Read-out:PCI Err Status */

	hfc_write_reg_ext(ap, 0x83f, 0x1, 0x0f);	/* Issue IPRES */
	hfc_write_reg(ap, HFC_IOSPACE_INTA_RST, 0x4, 0xffffffff);	/* Reset Interrupts.  */

	return;
}

/*
 * Function:    hfc_set_mcw_five_ex
 *
 * Purpose:
 *
 * Arguments:
 *  ap         - pointer to adap_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_set_mcw_five_ex(struct adap_info *ap)
{
	const uchar pass1 = 0x00; /* FCLNX-GPL-220 */
	uchar wk_char;
	
	/* Set "IP-Core SRAM ECC check disable" */
	if (ap->pkg.lsi_rev == pass1 ) { /* FCLNX-GPL-220 */
		hfc_write_reg_ext(ap, 0x890, 0x4, 0x001f0000);
	}
	else { /* FIVE-EX Pass 2 & Pass 2.1 */ /* FCLNX-GPL-220 */
		 hfc_write_reg_ext(ap, 0x890, 0x4, 0x001c0000);
	}


	/* Set "HVCHPTSTP" */
	wk_char = hfc_read_reg_ext(ap,  0x18, 0x1);
	wk_char |= 0x02;
	hfc_write_reg_ext(ap, 0x18, 0x1, wk_char);

	/* Set "maximam system bus outbound read size" */
/*	wk_char = hfc_read_reg_ext(ap,  0x920, 0x1); */	/* FCLNX-GPL-220 */
/*	wk_char |= 0x40; */								/* FCLNX-GPL-220 */
/*	hfc_write_reg_ext(ap, 0x920, 0x1, wk_char); */	/* FCLNX-GPL-220 */

	/* Set "maximam system bus outbound write size" */
/*	wk_char = hfc_read_reg_ext(ap, 0x921, 0x1); */	/* FCLNX-GPL-220 */
/*	wk_char |= 0x40; */								/* FCLNX-GPL-220 */
/*	hfc_write_reg_ext(ap, 0x921, 0x1, wk_char); */	/* FCLNX-GPL-220 */

	/* Set "non-posted transaction Timeout" */
/*	wk_char = hfc_read_reg_ext(ap, 0x9a3, 0x1); */	/* FCLNX-GPL-220 */
/*	wk_char |= 0x70; */								/* FCLNX-GPL-220 */
/*	hfc_write_reg_ext(ap, 0x9a3, 0x1, wk_char); */	/* FCLNX-GPL-220 */

	/* Set "HMCWPCINFMCK" */ /* FCLNX-GPL-141 */
/*	wk_char = hfc_read_reg_ext(ap, 0x260, 0x1); */
/*	wk_char |= 0x40; */
/*	hfc_write_reg_ext(ap, 0x260, 0x1, wk_char); */

	return;
}


int hfc_config_hw_set_five_ex(struct adap_info *ap, uint retry_maxcnt)
{
	uchar		stat_chkerrflg;
	uchar		post_chkerrflg;
	uint		retry_cnt;
	uchar		dmp_data[32];
	uint		errlog_data[4];
	union {
		uint	l;
		ushort	s[2];
		uchar	c[4];
	} value;

	uchar		err_code = 0;
	uint 		status_bak = 0;
/*	uchar       exe_reset = 0; */ /* FCLNX-GPL-303 */
	uchar		wk_char = 0;
	uint		dummy_read_reg=0;	/* FCLNX-GPL-FX-195 */

	HFC_ENTRY("hfc_config_hw_set_five_ex");
	
	/* Initialize */
	memset(dmp_data, 0, sizeof(dmp_data));
	memset(errlog_data, 0, sizeof(errlog_data));

	/* Set 0x00 to CMD_RES (0x30) */
	hfc_write_reg(ap, HFC_IOSPACE_CMDRES, 0x1, 0x00);

	mdelay(1000); /* Wait 1000ms */

	/* FCLNX-GPL-227 start */
/*	if( ap->debug_func & HFC_DEBUG_LINK_WIDTH_CHK ) */ /* FCLNX-GPL-246 */
/*	{ */
		hfc_pcie_link_width_chk(ap);
/*	} */
	/* FCLNX-GPL-227 end */

//	if ( ( hfc_manage_info.hfcplus_enable ) && 
	if ( ( hfc_manage_info.hfcldd_mp_mod ) &&	/* FCLNX-GPL-349 */
		 ( ap->isol_force == HFC_CHKSTP_FRC_ISOL )){ /* - FCLNX-546 - *//* FCLNX-GPL-147 */
		/* Stop optical transmission */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(ap,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
		}
		else{
			hfc_mlpf_set_fcif(ap, 0x80808080);	/* FCLNX-GPL-399 */
		}
		/* Turn LED (Yellow and Green) off */
		if(!(HFC_MMODE_CHECK_SHARED(ap))){
			hfc_write_reg(
				ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
		}
		else{
			hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
		}
		return 0;
	} /* FCLNX-GPL-147 */


	for (retry_cnt = 0; retry_cnt < (retry_maxcnt + 1); retry_cnt++) {

		stat_chkerrflg = 0;
		post_chkerrflg = 0;
		err_code = 0;
		value.l = 0;
		status_bak = 0;
		
		/* Read status information (4 bytes) */
		status_bak = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4 );
		HFC_4B_TO_4L(value.l,status_bak);

		HFC_DBGPRT( "hfc_read_reg HFC_IOSPACE_STATUS[%x] = 0x%08x.",
					HFC_IOSPACE_STATUS0, status_bak);
		
		if (status_bak != 0x80000000) { /* status is normal? */

			/* Still executing reset process? */
			if ((value.c[0] & HFC_PCI_RESETCHK) != 0x00) {

				if(!(HFC_MMODE_CHECK_SHARED(ap))){
					hfc_write_reg(
						ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
				}
				else{
					hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
				}
				stat_chkerrflg = 1;
				err_code = 1;
			}
			else {
				/* FCLNX-GPL-303 */ /* Never check "PCIERR_DETECTED" on FIVE-EX */
				
				/* EXGMCK and BOOTRUN check ? */
				if ((value.c[1] & HFC_PCI_PCICHK) == HFC_PCI_PCICHK) {
					/* ERR end */
					/* Set LED to Wake up Falure */
					if(!(HFC_MMODE_CHECK_SHARED(ap))){
						hfc_write_reg(
							ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
					}
					else{
						hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
					}
					stat_chkerrflg = 1;
					err_code = 3;
				}
				else {
					/* EXGMCK check */
					if ((value.c[1] & HFC_PCI_EXGMCK) == HFC_PCI_EXGMCK) {
						/* ERR end */
						/* Set LED to POST up Falure */
						if(!(HFC_MMODE_CHECK_SHARED(ap))){
							hfc_write_reg(
								ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
						}
						else{
							hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
						}
						stat_chkerrflg = 1;
						err_code = 4;
					}
					else {
						/* BOOTRUN check */
						if ((value.c[1] & HFC_PCI_BOOTRUN) == HFC_PCI_BOOTRUN) {
							/* ERR end */
							/* Set LED to Wake up Falure */
							if(!(HFC_MMODE_CHECK_SHARED(ap))){
								hfc_write_reg(
									ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
							}
							else{
								hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
							}
							stat_chkerrflg = 1;
							err_code = 5;
						}
						else {
							/* FUNC_STOP check */
							if ((value.c[1] & HFC_PCI_FCNSTOP) == HFC_PCI_FCNSTOP) {
								/* ERR end */
								/* Set LED to Wake up Falure */
								if(!(HFC_MMODE_CHECK_SHARED(ap))){
									hfc_write_reg(
										ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
								}
								else{
									hfc_mlpf_set_led(ap, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
								}
								stat_chkerrflg = 1;
								err_code = 6;
							}
							else {
								/* CH not Ready check */
								if ((value.c[2] & HFC_PCI_CH_NOT_READY) == HFC_PCI_CH_NOT_READY) {
									/* ERR end */
									/* Set LED to POST up Falure */
									if(!(HFC_MMODE_CHECK_SHARED(ap))){
										hfc_write_reg(
											ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
									}
									else{
										hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
									}
									stat_chkerrflg = 1;
									err_code = 7;
								}
							}
						}
					}
				}
			}
		}
		
		/* Normal case */
		if (stat_chkerrflg == 0) {
			ushort result;

			/* Read first 2bytes of POST result */
			result = (ushort) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 0x2);
			if ((result & 0xff00) == 0x8000) {/* Is first byte 0x80? */

				if ((result & 0x00ff) == 0x00FF) {/* Normal end? */
					/* Normal end */
					break;
				} else { /* Otherwise error */
					post_chkerrflg = 1;
					err_code = 9;
				}
			}
			else {
				/* ERR end */
				/* Set LED to POST up Falure */ /* We use "HFC_POST_FAILURE_FIVE" for FIVE-EX */
				if(!(HFC_MMODE_CHECK_SHARED(ap))){
					hfc_write_reg(
						ap, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_POST_FAILURE_FIVE);
				}
				else{
					hfc_mlpf_set_led(ap, HFC_POST_FAILURE_FIVE);	/* FCLNX-GPL-399 */
				}
				post_chkerrflg = 1;
				err_code = 8;
			}
		}

		HFC_DBGPRT(  "config_hw_set_five_ex() - err_code_2 = 0x%x.",err_code);

		/* Retry count reaches retry maxcnt */
		if (retry_cnt == retry_maxcnt) {

			/* Status information error? */
			if (stat_chkerrflg == 1) {
				HFC_DBGPRT(  "config_hw_set_five_ex() - stat_chkerrflg=1, errcode = 0x%x.",err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));

				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4);
				errlog_data[1] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS1, 0x4);
				errlog_data[2] = (uint) hfc_read_reg(ap, HFC_IOSPACE_ERRDETAIL0, 0x4);
				/* Create Err Log data */
				dmp_data[0]  = (uchar)(errlog_data[0] >> 24);
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16);
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 );
				dmp_data[3]  = (uchar)(errlog_data[0]      );
				dmp_data[4]  = (uchar)(errlog_data[1] >> 24);
				dmp_data[5]  = (uchar)(errlog_data[1] >> 16);
				dmp_data[6]  = (uchar)(errlog_data[1] >> 8 );
				dmp_data[7]  = (uchar)(errlog_data[1]      );
				dmp_data[8]  = (uchar)(errlog_data[2] >> 24);
				dmp_data[9]  = (uchar)(errlog_data[2] >> 16);
				dmp_data[10] = (uchar)(errlog_data[2] >> 8 );
				dmp_data[11] = (uchar)(errlog_data[2]      );
				dmp_data[12] = (uchar)(errlog_data[3] >> 24);
				dmp_data[13] = (uchar)(errlog_data[3] >> 16);
				dmp_data[14] = (uchar)(errlog_data[3] >> 8 );
				dmp_data[15] = (uchar)(errlog_data[3]      );
				/* H/W log */
				hfc_logout(ap, (uint)0x35, HFC_ERRLOG_TYPE_IMLLOG);
				/* Driver log */
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x35, dmp_data, 16) ;
			}
			/* Post-check error? */
			else if (post_chkerrflg == 1) {
				HFC_DBGPRT(  "CONFIG_HW_set_five_ex() - post_chkerrflg=1, errcode = 0x%x.", err_code );

				memset(dmp_data, 0, sizeof(dmp_data));
				memset(errlog_data, 0, sizeof(errlog_data));
				
				/* Read POST information */
				errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_CA_POSTRESULT, 0x32);

				/* Create error log data */
				dmp_data[0]  = (uchar)(errlog_data[0] >> 24) ;
				dmp_data[1]  = (uchar)(errlog_data[0] >> 16) ;
				dmp_data[2]  = (uchar)(errlog_data[0] >> 8 ) ;
				dmp_data[3]  = (uchar)(errlog_data[0]	   ) ;
				/* H/W log */
				hfc_logout(ap, (uint)0x36, HFC_ERRLOG_TYPE_IMLLOG);
				/* Driver log */
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x36, dmp_data, 16) ;
			}

			/* Abnormal End */
			return(-1);
		}
		
		/* FCLNX-GPL-236 start */
		if( ap->debug_func & HFC_DEBUG_POST_LOGOUT )
		{	/* Debug Mode */ /* Get Log "ErrNo:0xf1" */
			memset(dmp_data, 0, sizeof(dmp_data));
			memset(errlog_data, 0, sizeof(errlog_data));

			errlog_data[0] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS0, 0x4);
			errlog_data[1] = (uint) hfc_read_reg(ap, HFC_IOSPACE_STATUS1, 0x4);
			errlog_data[2] = (uint) hfc_read_reg(ap, HFC_IOSPACE_ERRDETAIL0, 0x4);
			/* Create Err Log data */
			dmp_data[0]  = (uchar)(errlog_data[0] >> 24);
			dmp_data[1]  = (uchar)(errlog_data[0] >> 16);
			dmp_data[2]  = (uchar)(errlog_data[0] >> 8 );
			dmp_data[3]  = (uchar)(errlog_data[0]      );
			dmp_data[4]  = (uchar)(errlog_data[1] >> 24);
			dmp_data[5]  = (uchar)(errlog_data[1] >> 16);
			dmp_data[6]  = (uchar)(errlog_data[1] >> 8 );
			dmp_data[7]  = (uchar)(errlog_data[1]      );
			dmp_data[8]  = (uchar)(errlog_data[2] >> 24);
			dmp_data[9]  = (uchar)(errlog_data[2] >> 16);
			dmp_data[10] = (uchar)(errlog_data[2] >> 8 );
			dmp_data[11] = (uchar)(errlog_data[2]      );
			dmp_data[12] = (uchar)(errlog_data[3] >> 24);
			dmp_data[13] = (uchar)(errlog_data[3] >> 16);
			dmp_data[14] = (uchar)(errlog_data[3] >> 8 );
			dmp_data[15] = (uchar)(errlog_data[3]      );
			/* H/W log */
			hfc_logout(ap, (uint)0xf1, HFC_ERRLOG_TYPE_IMLLOG);
			/* Driver log */
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_EVNT3, 0xf1, dmp_data, 16) ;
		}
		/* FCLNX-GPL-236 end */
		
		/* Forced MCK */
		hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, 0x1, 0x08);

		/* Reset all interrupt factor */
		hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_RST,( char )0x4, 0xffffffff );      /*  FCLNX-0172 */

		if (HFC_MMODE_CHECK_SHADOW(ap)){				/* FCLNX-GPL-317 */
			if(ap->mp_adap_info != NULL) {
				if( ap->mp_adap_info->port_cnt == 2 ){
					if( ap->port_no%2 == 0 ){
						hfc_write_reg_ext(ap, ( uint )0x10a0,( char )0x4, 0x00000000 );
					}
					else{
						hfc_write_reg_ext(ap, ( uint )0x10b0,( char )0x4, 0x00000000 );
					}
				}
				else if( ap->mp_adap_info->port_cnt == 1 ){
					hfc_write_reg_ext(ap, ( uint )0x10a0,( char )0x4, 0x00000000 );
				}
			}
		}												/* FCLNX-GPL-317 */

		/* Clear GR0-F */ /* FCLNX-GPL-220 */
		hfc_reset_start( ap, HFC_GR_CLEAR );
		
		/* Recovery Reset (CTLRES / PONRES) */ /* FCLNX-GPL-220 */
		hfc_reset_start(ap, HFC_CTLRST);

		/* Address 0x00 register dummy read *//* FCLNX-GPL-FX-195 */
		dummy_read_reg = (uint)hfc_read_reg(ap, (uint)HFC_IOSPACE_ZERO, (char)0x4);

		/* 1ms wait */
		mdelay(1);

		/* Initiate reboot *//* FCLNX-GPL-309 start */
		if( HFC_MMODE_CHECK_MLPF(ap) )
		{	
			/* Clear Communication area of WS. */
			hfc_reset_start(ap, HFC_WSCA_CLEAR);
			/* We reboot the same as MCK recovery. */
			hfc_reset_start(ap, HFC_REBOOT);
		}
		else
		{	/* Basic */
			hfc_write_reg(ap, HFC_IOSPACE_CMDBOOT, 0x1, 0x08); /*  for FIVE-EX */
		}
		/* FCLNX-GPL-309 end */

		/* Wait 100ms */
		mdelay(100);
		
		/* Function start & start trace */
		hfc_write_reg(ap, HFC_IOSPACE_CMDCTL, 0x1, 0xa0);

		if( HFC_MMODE_CHECK_MLPF(ap) )                                  /* FCLNX-0428 */
			hfc_reset_start(ap, HFC_SET_MLPF_MODE);

		/* Wait 500ms */
		mdelay(500);
    }

/*	exe_reset = 0; */ /* FCLNX-GPL-303 */
	
	/* PCI error status reset */
	hfc_clear_sticky_bit(ap);
	
	/* Disable REQID#0 */ /* FCLNX-GPL-294 start */
	wk_char = hfc_read_reg_ext(ap,  0x222, 0x1);
	wk_char |= 0x80;
	hfc_write_reg_ext(ap, 0x222, 0x1, wk_char);
	/* FCLNX-GPL-294 end */

	HFC_EXIT("hfc_config_hw_set_five_ex");

    return(0);
}

int hfc_query_devid(struct adap_info *ap)
{
	uint	vender_id;
	uint	device_id;
	uint	sub_system_id;

	memset(&ap->pkg, 0, sizeof(ap->pkg));

	/* Get "VenderID" */
	vender_id = hfc_read_cnfg(ap, HFC_HOST_VENDER_ID, 0x2);
	ap->pkg.vender_id = (ushort)vender_id;
	/* Get "DeviceID" */
	device_id = hfc_read_cnfg(ap, HFC_HOST_DEVICE_ID, 0x2);
	ap->pkg.device_id = (ushort)device_id;
	/* Get "SubSystemID" */
	sub_system_id = hfc_read_cnfg(ap, HFC_HOST_SUB_SYSTEM_ID, 0x2);
	ap->pkg.sub_system_id = (ushort)sub_system_id;

	switch (ap->pkg.device_id) {
		case 0x3009 :
			/* FPP */
			ap->pkg.type = HFC_PKTYPE_FPP;
			ap->pkg.map  = (struct pkg_map *) &hfc_pkg_map[0];
			break;

		case 0x300a :
		case 0x300b :
		case 0x300c :
		case 0x300d :
			/* FIVE */
			ap->pkg.type = HFC_PKTYPE_FIVE;
			ap->pkg.map  = (struct pkg_map *) &hfc_pkg_map[1];
			break;

		case 0x3020 :
			/* FIVE-EX */
			ap->pkg.type = HFC_PKTYPE_FIVE_EX;
			ap->pkg.map  = (struct pkg_map *) &hfc_pkg_map[2];
			break;

		default :
			/* This device_id is unkown.  */
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCE, NULL, 0) ;/* FCLNX-GPL-161 */

			return(-1);
	}
	return(0);
}

/* FCLNX-GPL-112 */
/*
 * Function:  hfc_get_sysrev
 *
 * Purpose:
 *
 * Arguments:
 *  ap        - pointer to an adap_info structure
 *
 * Returns:
 *  Current FW revision (uint)
 *
 * Notes:
 *  This
 */
uint hfc_get_sysrev( struct adap_info *ap)
{
	uint sysrev;

	/* Get current FW sysrev info from mp_adap_info */

	if ( ap == NULL ){				 /* FCLNX-GPL-161 */
		sysrev = 0xeeeeeeee; return(sysrev);
	}
	if ( ap->mp_adap_info == NULL) { /* FCLNX-GPL-161 */
		sysrev = 0xefefefef; return(sysrev);
	}

	sysrev = ap->mp_adap_info->sys_rev;

	if ( ap->fw_init_p != NULL) {
		if ( hfc_read_val( ap->fw_init_p->func ) & HFC_FWF_ONLINEUP ) {
		/* Get current FW sysrev info from FW_INIT_TBL when FW online-update function is supported */
			sysrev = hfc_read_val(ap->fw_init_p->fls_hdr.sys_rev);
		}
	}

	return (sysrev);

}

/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
/*
 * Function:  hfc_get_slot_dev
 *
 * Purpose:   Identify pointer to this slot's pci_dev structure
 *
 * Arguments:
 *  ap        - pointer to an adap_info structure
 *
 * Returns:   
 *            - error case  : NULL
 *            - succeed case: pointer to the pci_dev
 *
 * Notes:
 *
 */
struct pci_dev *hfc_get_slot_dev(struct adap_info *ap)
{
	struct pci_bus *bus			= ap->pci_cfginf->bus;
	struct pci_bus *parent_bus	= NULL;
	struct pci_dev *slot_dev	= NULL;

	if(bus == NULL)
	{	/* abnormal case */
		return slot_dev;
	}

	if(ap->pkg.type == HFC_PKTYPE_FPP)
	{
		/* This BUS's parent is the "slot bus" */
		slot_dev = bus->self;
	}
	else if(ap->pkg.type == HFC_PKTYPE_FIVE)
	{
		/* check the package code */
		switch(ap->pkg.code)
		{
			case 0x82: /* FIVE  4G HBA   */
			case 0x85: /* FIVE  2G HBA   */
			case 0x86: /* FIVE  4G HBA   */
			case 0x87: /* FIVE 10G HBA   */
				
				/* PCI-X type */
				/* This BUS's parent is the "slot bus" */
				slot_dev = bus->self;
				break;
				
			default: /* others */
				
				/* PCIe type */
				/* This BUS's grand parent is the "slot bus" */
				parent_bus = bus->parent;
				if(parent_bus != NULL)
				{
					slot_dev = parent_bus->self;
				}
				break;
		}
	}
	else /* FIVE-EX */
	{
		/* This BUS's parent is the "slot bus" */
		slot_dev = bus->self;
	}
	
	return slot_dev;
}

/* FCLNX-GPL-201 */
/*
 * Function:  hfc_restore_add_wwn
 *
 * Purpose:
 *
 * Arguments:
 *  ap        - pointer to an adap_info structure
 *
 * Returns:   
 *            - Succeed : add_ww_name
 *            - Failed  : 0
 *
 * Notes:
 *
 */
uint64_t hfc_restore_add_wwn(struct adap_info *ap)
{
	struct   pci_dev         *slot_dev;
	struct   slot_ww_name    *slot_wwn;
	int      domain_no; /* 4byte */
	uchar    bus_no;    /* 1byte */
	uchar    devfn;     /* 1byte */
	uint64_t add_ww_name = 0x00;
	int i;
	
	/*** Get the "BUS:DEV.FUNC" of this slot  ***/
	slot_dev = hfc_get_slot_dev(ap);
	if(slot_dev != NULL)
	{
/*		domain_no = pci_domain_nr(slot_dev->bus); */ /* for Multi domain */
		domain_no = 0;
		bus_no    = slot_dev->bus->number;
		devfn     = slot_dev->devfn;
	}
	else
	{	/* abnormal case */
		return add_ww_name;
	}
	
	
	/*** Search the "add_ww_name" of this port ***/
	slot_wwn = hfc_manage_info.slot_add_wwn;
	for(i=0; i<MAX_ADAP_CNT; i++)
	{
		/* check the "domain" of this slot */
		if(slot_wwn[i].domain_no != domain_no)
		{	/* unmatched */
			continue; /* go next */
		}
		
		/* check the "BUS" of this slot */
		if(slot_wwn[i].bus_no != bus_no)
		{	/* unmatched */
			continue; /* go next */
		}
		
		/* check the "DEV.FUNC" of this slot */
		if(slot_wwn[i].devfn != devfn)
		{	/* unmatched */
			continue; /* go next */
		}
		
		/* check the port# of this port */
		if(slot_wwn[i].port_no != ap->port_no)
		{	/* unmatched */
			continue; /* go next */
		}

		/* Succeed Case */
		/* restore data */
		add_ww_name = slot_wwn[i].ww_name;
		break;
	}
	
	return add_ww_name;
}

/* FCLNX-GPL-201 */
/*
 * Function:  hfc_backup_add_wwn
 *
 * Purpose:
 *
 * Arguments:
 *  ap        - pointer to an adap_info structure
 *
 * Returns:   
 *            - Succeed :  0
 *            - Failed  : -1
 *
 * Notes:
 *
 */
int hfc_backup_add_wwn(struct adap_info *ap, uint64_t add_ww_name)
{
	struct  pci_dev         *slot_dev;
	struct  slot_ww_name    *slot_wwn;
	int     domain_no; /* 4byte */
	uchar   bus_no;    /* 1byte */
	uchar   devfn;     /* 1byte */
	int     err_code = -1;
	int     i;
	
	/*** Get "BUS:DEV.FUNC" of this slot  ***/
	slot_dev = hfc_get_slot_dev(ap);
	if(slot_dev != NULL)
	{
/*		domain_no = pci_domain_nr(slot_dev->bus); */ /* for Multi domain */
		domain_no = 0;
		bus_no    = slot_dev->bus->number;
		devfn     = slot_dev->devfn;
	}
	else
	{	/* abnormal case */
		return err_code;
	}
	
	/*** Search a space ***/
	slot_wwn = hfc_manage_info.slot_add_wwn;
	for(i=0; i<MAX_ADAP_CNT; i++)
	{
		/* check the status of this structure */
		if( slot_wwn[i].ww_name != 0x00 )
		{	/* used */
			continue; /* go next */
		}

		/* Succeed Case */
		/* wirte data */
		slot_wwn[i].domain_no = domain_no;
		slot_wwn[i].bus_no    = bus_no;
		slot_wwn[i].devfn     = devfn;
		slot_wwn[i].port_no   = ap->port_no;
		slot_wwn[i].ww_name   = add_ww_name;
		err_code = 0; /* No err */
		break;
	}
	
	return err_code;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
void hfc_kthread_stop(struct adap_info *ap)
{
	atomic_set(&ap->rport_event_wait, 1); /* FCLNX-GPL-565 */
	kthread_stop(ap->worker_thread); /* FCLNX-GPL-259, 276 */
	return;
}

int hfc_do_rport(void *p)
{
	struct adap_info	*ap = p;
	struct target_info	*target;
	ulong				flags = 0;
	uchar				add, del;  /* FCLNX-GPL-206 */
	int					i=0,rc=0;  /* FCLNX-GPL-205 *//* FCLNX-GPL-310 */
	
	HFC_ENTRY("hfc_do_rport");
	
	while (!kthread_should_stop()) {
		rc = wait_event_interruptible(ap->rport_event, atomic_read(&ap->rport_event_wait)!=0 );	/* FCLNX-GPL-259,310,565 */
	
		if((kthread_should_stop()) || (rc))										/* FCLNX-GPL-310 */
			break;
		
		for (i=0;i<MAX_TARGET_PROBE;i++) { /* FCLNX-GPL-205 start */
			HFC_ADAPLOCK_IRQSAVE(flags);
			target = hfc_hash_target_valid(ap, i); /* FCLNX-GPL-207 */
			/* delete rport */ /* FCLNx-GPL-206 start */
			del = FALSE;
			if(target){
				if ( test_bit(HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status ) ) {
					del = TRUE;
					HFC_DBGPRT("hfc_do_rport - delete: instance=%d, target_id=%d, rid=%d\n",
						ap->instance, target->target_id, ap->rid);
				}
				clear_bit( HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status );
			} /* FCLNx-GPL-206 end */
			
			/* add rport */
			add = FALSE;
			if(target){
				if ( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) ){ /* FCLNX-GPL-207 */
					if ( test_bit(HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status ) ) {
						add = TRUE;
						HFC_DBGPRT("hfc_do_rport - add: instance=%d, target_id=%d, rid=%d\n",
							ap->instance, target->target_id, ap->rid);
					}
				}
				clear_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
			}
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			
			if (del == TRUE) { /* FCLNX-GPL-206 start */
				hfc_rport_delete(target);
			} /* FCLNX-GPL-206 end */
			if (add == TRUE) {
				hfc_rport_add(ap, target);
			}
		} /* FCLNX-GPL-205 end */
		
		/* try again rport status check */
		HFC_ADAPLOCK_IRQSAVE(flags);
		for (i=0;i<MAX_TARGET_PROBE;i++) {
			target = hfc_hash_target_valid(ap, i);
			
			del = FALSE;
			if(target){
				if ( test_bit(HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status ) ) {
					del = TRUE;
				}
			}
			
			add = FALSE;
			if(target){
				if ( test_bit(HFC_WWN_VALID, (ulong *)&target->flags) ){
					if ( test_bit(HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status ) ) {
						add = TRUE;
					}
				}
			}
		}
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		
		if ((del == FALSE) && (add == FALSE))
			atomic_set(&ap->rport_event_wait, 0);	/* FCLNX-GPL-259,565 */
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	clear_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status );
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	HFC_EXIT("hfc_do_rport");
	
	return 0;
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/* FCLNX-GPL-204 STR */
/*
 * Function:    hfc_proc_info_pfb
 *
 * Purpose:
 *
 * Arguments:
 *  host		- Pointer to Scsi_Host
 *  buffer		- Data input/output pointer
 *  start		- Data output pointer
 *  offset		- Offset from buffer start address
 *  length		- Buffer length
 *  inout		- TRUE (RD), FALSE (WR)
 *
 * Returns:   
 *    < 0		- Error number
 *    >=0       - Dara length
 * Notes:
 */
int hfc_proc_info_pfb(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
{
	return ( hfc_manage_info.npubp->hfc_mp_proc_info_pfb(host, buffer, start, offset, length, inout) );
}

/*
 * Function: hfc_info
 *
 * Purpose: Output driver infotmation to syslog
 *
 * Arguments:
 *  host  - Pointer to Scsi_Host
 *
 * Returns:
 *
 * Notes:   Pointer to output character string
 */
const char *hfc_info_pfb (struct Scsi_Host *host)
{
	return ( hfc_manage_info.npubp->hfc_mp_info_pfb(host) );
}

/*
 * Function:    hfc_strategy
 *
 * Purpose:     Initiate SCSI command 
 *
 * Arguments:   
 *  cmnd		Pointer to Scsi_Cmnd
 *  iodone		Pointer to done() (Return SCSI response)
 *
 * Returns:     
 *  			0 :  Completed SCSI initiation successfully 
 *  			EIO : Adap_info or dev_info does not exist or invalid.
 * 				(Only for ioctl)
 *
 * Notes:       Caller should be in process level or interruption level.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
int hfc_strategy_pfb_lck(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
#else
int hfc_strategy_pfb(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */
{
	return ( hfc_manage_info.npubp->hfc_mp_strategy_pfb(cmnd,iodone) );
}

int hfc_eh_abort_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_mp_abort_pfb(cmnd) );
}

int hfc_eh_device_reset_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_mp_device_reset_pfb(cmnd) );
}

int hfc_eh_target_reset_pfb(struct scsi_cmnd *cmnd)					/* FCLNX-GPL-0449 */
{
	return ( hfc_manage_info.npubp->hfc_mp_target_reset_pfb(cmnd) );
}

int hfc_eh_bus_reset_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_mp_bus_reset_pfb(cmnd) );
}

/* FCLNX-GPL-204 END */


/* FCLNX-GPL-227 */ /* FCLNX-GPL-246 */
/*
 * Function:  hfc_pcie_link_width_chk
 *
 * Purpose:   Check pcie_link_width for FIVE-EX
 *
 * Arguments:
 *  ap        - pointer to an adap_info structure
 *
 * Returns:   
 *            - Succeed :  0
 *            - Failed  : -1
 *
 * Notes:     ap is not NULL
 *
 */
int
hfc_pcie_link_width_chk(struct adap_info *ap)
{
	const uchar pcie_X1      = 0x01;
	int         rc           = 0;
	ushort      cfg_data     = 0x0000;
	uchar       negotiated_link_width = 0x00;

	/* Get negotiated_link_width (cfgadr 7F-7Eh ,bit9-4) */
	cfg_data   = (ushort)hfc_read_cnfg(ap, 0x7e, 0x2);
	cfg_data >>= 4;
	cfg_data  &= 0x003f;
	negotiated_link_width = (uchar)cfg_data;
	
	/* for Debug */
	HFC_DBGPRT("hfcldd%d: negotiated_link_width = %d\n", ap->dev_minor, negotiated_link_width);

	/* Data check */
	if( negotiated_link_width == pcie_X1 )
	{	/* Err Case (LinkWidth = x1) */
		/* FCLNX-GPL-246 start */
		if (ap->pkg.lsi_rev == 0x02)
		{	/* FIVE-EX Pass2.1 */
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xD9, NULL, 0);
		}
		else
		{	/* FIVE-EX Pass2.2 (Pass1.0, Pass2.0, and others) */ /* FCLNX-GPL-263 */
			hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR2, 0xDA, NULL, 0);
		}
		/* FCLNX-GPL-246 end */
		rc = -1;
	}
	else
	{
		/* No Err */
		rc = 0;
	}
	
	return rc;
}

#if 0 /* FCLNX-GPL-306 *//* FCLNX-GPL-429 */
/* FCLNX-GPL-306 */
/*
 * Function:    hfc_suspend_one
 *
 * Purpose:     This is the suspend entry point.
 *
 * Arguments:   
 *  pdev		Pointer to PCI device
 *  msg			Power Management message
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       
 */
int
hfc_suspend_one(struct pci_dev *pdev, pm_message_t msg)
{
	struct Scsi_Host *host = NULL;
	struct adap_info *ap   = NULL;
	ulong flags=0;
	int pci_fail=0;
	
	
	HFC_DBGPRT( "hfcldd : hfc_suspend_one - Start.\n");

	/*** NULL pointer check ***/
	if( pdev == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_suspend_one - pdev is NULL.\n");
		goto hfc_suspend_err;
	}
	
	host = pci_get_drvdata(pdev);
	if(host == NULL)
	{
		HFC_DBGPRT( "hfcldd : hfc_suspend_one - host is NULL.\n");
		goto hfc_suspend_err;
	}
	ap = (struct adap_info *)host->hostdata;
	if(ap == NULL)
	{
		HFC_DBGPRT( "hfcldd : hfc_suspend_one - ap is NULL.\n");
		goto hfc_suspend_err;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	/*** Check the staus of this port. ***/
	if( test_bit(HFC_SUSPEND, (ulong *)&ap->status) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - This is Adapter is SUSPEND.\n",ap->dev_minor);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		goto hfc_suspend_err;
	}
	
	set_bit(HFC_SUSPEND, (ulong *)&ap->status);
	ap->pm_event = msg.event;
	
	pci_fail = hfc_pcibus_chk(ap);

	/*** Do suspend ***/
	switch( msg.event )
	{
		case PM_EVENT_ON:
			/* NOP */
			HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - PM_EVENT_ON.\n",ap->dev_minor);
			break;

		case PM_EVENT_FREEZE:
		case PM_EVENT_SUSPEND:
			HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - PM_EVENT_FREEZE or PM_EVENT_SUSPEND.\n",ap->dev_minor);
			/* Bring down the device and stop kthread. */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			hfc_suspend_driver(ap, host);
			HFC_ADAPLOCK_IRQSAVE(flags);
			
			/* suspend PCI device */
			if((!test_bit(HFC_ISOL, (ulong *)&ap->status)))
			{	/* FCLNX-GPL-428 */
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
				if( hfc_suspend_pci(ap, pdev, pci_fail) )
				{
					HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - Failed hfc_suspend_pci.\n",ap->dev_minor);
					goto hfc_suspend_err;
				}
				HFC_ADAPLOCK_IRQSAVE(flags);
			}
			
			/* Change power state */
			if( msg.event == PM_EVENT_SUSPEND)
			{	/* for only PM_EVENT_SUSPEND */
				HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - PM_EVENT_SUSPEND.\n",ap->dev_minor);
				if(( ap->issue_d3hot)&&(pci_fail == 0)){
					HFC_ADAPUNLOCK_IRQRESTORE(flags);
					/* Set power state. This function call "msleep".*/
					if( pci_set_power_state(pdev, PCI_D3hot) )
					{
						goto hfc_suspend_err;
					}
					HFC_ADAPLOCK_IRQSAVE(flags);
				}
			}
			break;

		default:
			HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - Default.\n",ap->dev_minor);
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			goto hfc_suspend_err;
	}
	HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - End.\n",ap->dev_minor);
	/* No Err */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	return 0;
	
hfc_suspend_err:
	HFC_DBGPRT( "hfcldd%d : hfc_suspend_one - hfc_suspend_err.\n",ap->dev_minor);
	return -EIO;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_resume_one
 *
 * Purpose:     This is the resume entry point.
 *
 * Arguments:   
 *  pdev		Pointer to PCI device
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       
 */
int
hfc_resume_one(struct pci_dev *pdev)
{
	struct Scsi_Host *host = NULL;
	struct adap_info *ap   = NULL;
	ulong flags=0;

	HFC_DBGPRT( "hfcldd : hfc_resume_one - Start.\n");

	if( pdev == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_resume_one - pdev is NULL.\n");
		goto hfc_resume_err;
	}

	host = pci_get_drvdata(pdev);
	if( host == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_resume_one - host is NULL.\n");
		goto hfc_resume_err;
	}
	
	ap = (struct adap_info *)host->hostdata;
	if( ap == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_resume_one - ap is NULL.\n");
		goto hfc_resume_err;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if( !test_bit(HFC_SUSPEND, (ulong *)&ap->status) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_resume_one - This Adapter is not SUSPEND.\n",ap->dev_minor);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		goto hfc_resume_err;
	}
	clear_bit(HFC_SUSPEND, (ulong *)&ap->status);
	
	switch( ap->pm_event )
	{
		case PM_EVENT_ON:
			/* NOP */
			HFC_DBGPRT( "hfcldd%d : hfc_resume_one - PM_EVENT_ON.\n",ap->dev_minor);
			break;

		case PM_EVENT_FREEZE:
		case PM_EVENT_SUSPEND:
			HFC_DBGPRT( "hfcldd%d : hfc_resume_one - PM_EVENT_FREEZE,PM_EVENT_SUSPEND.\n",ap->dev_minor);
			/* Change power state */
			if( ap->pm_event == PM_EVENT_SUSPEND)
			{	/* for only PM_EVENT_SUSPEND */
				HFC_DBGPRT( "hfcldd%d : hfc_resume_one - PM_EVENT_SUSPEND.\n",ap->dev_minor);
				if( ap->issue_d3hot){
					HFC_DBGPRT( "hfcldd%d : hfc_resume_one - D3_HOT ON.\n",ap->dev_minor);
					HFC_ADAPUNLOCK_IRQRESTORE(flags);
					if( pci_set_power_state(pdev, PCI_D0) )
					{
						goto hfc_resume_err;
					}
				}
				else{
					HFC_ADAPUNLOCK_IRQRESTORE(flags);
				}
			}
			else{
				HFC_ADAPUNLOCK_IRQRESTORE(flags);
			}
			
			/* resume PCI device */
			if( hfc_resume_pci(ap, pdev) )
			{
				HFC_DBGPRT( "hfcldd%d : hfc_resume_one - Failed hfc_resume_pci.\n",ap->dev_minor);
				goto hfc_resume_err;
			}
			
			/* Start the device and start kthread. */
			hfc_resume_driver(ap, host);
			HFC_ADAPLOCK_IRQSAVE(flags);
			
			break;

		default:
			HFC_DBGPRT( "hfcldd%d : hfc_resume_one - default.\n",ap->dev_minor);
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
			goto hfc_resume_err;
	}
	
	/* No Err */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	HFC_DBGPRT( "hfcldd%d : hfc_resume_one - End.\n",ap->dev_minor);
	return 0;
	
hfc_resume_err:
	HFC_DBGPRT( "hfcldd%d : hfc_resume_one - hfc_resume_err.\n",ap->dev_minor);
	return -EIO;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_suspend_driver
 *
 * Purpose:     Bring down the device and stop kthread
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *  host		The pointer for scsi_host information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "ap" and "host" are not NULL pointer.
 */
void
hfc_suspend_driver(struct adap_info *ap, struct Scsi_Host *host)
{
	ulong flags = 0;
	int wait=0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_suspend_driver - Start.\n",ap->dev_minor);
	
	/*** Lock ***/
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	/*** Closing process is in progress ***/
	set_bit(HFC_WAIT_CLOSE, (ulong *)&ap->status);
	ap->initialize = 0;
	clear_bit(HFC_ATTACH, (ulong *)&ap->attach_status);		/* FCLNX-0459 */

	do {													/* FCLNX-0459 */
		wait=0;
		
		if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-204 */
			wait = hfc_manage_info.npubp->hfc_wait_mp_ioend(ap);
		}
		
		if (!wait) {
			HFC_DBGPRT( "hfcldd%d : hfc_suspend_driver - called hfc_releasse.\n",ap->dev_minor);
			wait = hfc_release_adp(ap);
		}
		
		/* sleep for a while */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		msleep(1);
		hfc_intr(0, (void *)ap); /* Check interrupts by Drivers self Because, Not Interrupt for Kernel in suspend process*/
		HFC_ADAPLOCK_IRQSAVE(flags);
	} while (wait);											/* FCLNX-0459 */
	
	/*** Unlock ***/
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/*** stop kthread for rport ***/
	hfc_stop_rport(ap);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */	
	/*** put Scsi_Host ***/
	hfc_scsi_host_put(host);

	HFC_DBGPRT( "hfcldd%d : hfc_suspend_driver - End.\n",ap->dev_minor);
	return;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_suspend_pci
 *
 * Purpose:     suspend PCI device
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *  pdev		The pointer for pci device
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       "ap" and "pdev" are not NULL pointer.
 */
int
hfc_suspend_pci(struct adap_info *ap, struct pci_dev *pdev, int pci_fail)
{
	ulong flags = 0;

	HFC_DBGPRT( "hfcldd%d : hfc_suspend_pci - Start.\n",ap->dev_minor);

	/* Free IRQ. This function call "spin_lock_irqsave()". */
	hfc_free_interrupts(ap, ap->msi_flag, pci_fail);
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	ap->is_busmaster = pdev->is_busmaster;
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	/* Save state of pdev. This function call "spin_lock_irqsave()". */
	if(pci_fail == 0){
		if( pci_save_state(pdev) )
		{
			HFC_DBGPRT( "hfcldd%d : hfc_suspend_pci - Failed pci_save_state.\n",ap->dev_minor);
			return -EIO;
		}
	}

	/* Disable pdev. This function call "spin_lock_irqsave()". */
	if(pci_fail == 0){
		pci_disable_device(pdev);
	}
	
	HFC_DBGPRT( "hfcldd%d : hfc_suspend_pci - End.\n",ap->dev_minor);
	/* No Err */
	return 0;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_resume_driver
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *  host		The pointer for scsi_host information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "ap" and "host" are not NULL pointer.
 */
void
hfc_resume_driver(struct adap_info *ap, struct Scsi_Host *host)
{
	ulong flags = 0;
	struct mp_adap_info *mpap=NULL;
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	mpap=ap->mp_adap_info;
	
	HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - start.\n",ap->dev_minor);
	
	/*** get Scsi_Host ***/
	if ( scsi_host_get(host) == NULL )
	{	/* Err case */
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - Not get scsi_host.\n",ap->dev_minor);
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return;
	}
		
	/*** Reset xob,xrb ***/
	hfc_reset_adap_info(ap);
	
	/*** Set MCW ***/
	if ( (!(HFC_MMODE_CHECK_SHARED(ap)) || HFC_MMODE_CHECK_SHADOW(ap) ) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - set mcw.\n",ap->dev_minor);
		hfc_set_hw_mcw_cfg(ap);
		hfc_set_hw_mcw_pci(ap);
	}
	
	else{	/*** Set only LSI rev ***/ /* FCLNX-GPL-579 */
		if (ap->pkg.type == HFC_PKTYPE_FIVE_EX) {
			ap->pkg.lsi_rev = (uchar)hfc_read_reg(ap, HFC_IOSPACE_LSIREV, 0x01);
		}
	}
	
	/*** Sets RID (for only shared mode) ***/
	if ( HFC_MMODE_CHECK_SHARED(ap) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - set rid.\n",ap->dev_minor);
		ap->rid = (uint)hfc_read_hg_reg(ap, HFC_IOHGSPC_RID, 0x4 );
	}

	/*** Restart HW/FW ***/
	if(mpap->ap == ap)
	{	/* This Function is the port0 on this core. */
		/* This core needs HW setup. */
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - Clear mpap->status.\n",ap->dev_minor);
		HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
		mpap->status = 0;
		mpap->mck_err_cnt = 0;
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	}
	
	/*** clear errcnt_info ***/
//	if(hfc_manage_info.hfcplus_enable){
	if ( hfc_manage_info.hfcldd_mp_mod ){	/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_clear_errinfo(ap);
	}
	else{
		hfc_clear_errinfo_i(ap);
	}
	
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	
	if ( hfc_start_adapter(ap) )
	{	/* Err case */
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - Failed Start Adapter.\n",ap->dev_minor);
		return;
	}
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/*** start kthread for rport ***/
	if ( !hfc_manage_info.hfcldd_mp_mod ) {		/*FCLNX-GPL-315*/
		hfc_start_rport(ap);
	}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	/*** Link Initialize(FRAME_A) ***/
	/* check */
	if( hfc_skip_link_init(ap, FALSE) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - skip linkinitialize.\n",ap->dev_minor);
		return;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	/* polling start */
	ap->int_check = TRUE;
	hfc_w_start( ap, HFC_INT_CHECK_TMR );
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	/* issue Link Initialize */
	hfc_initialize(ap, 0);
	
	/* polling stop */
	HFC_ADAPLOCK_IRQSAVE(flags);
	if(ap->hba_isolation == HFC_ISOL_START){
		if(hfc_check_hba_isolation(ap) == HFC_ISOL_STOP){ 										/* FCLNX-GPL-349 */
			ap->hba_isolation = HFC_ISOL_STOP;
		}
	}
	ap->int_check = FALSE; /* Stop HFC_INT_CHECK_TMR */
	hfc_w_stop( ap, HFC_INT_CHECK_TMR );
	HFC_ADAPUNLOCK_IRQRESTORE(flags);

	HFC_DBGPRT( "hfcldd%d : hfc_resume_driver - End.\n",ap->dev_minor);
	return;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_resume_pci
 *
 * Purpose:     resume PCI device
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *  pdev		The pointer for pci device
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       "ap" and "pdev" are not NULL pointer.
 */
int
hfc_resume_pci(struct adap_info *ap, struct pci_dev *pdev)
{
	ulong flags=0;
	int msi_flag=0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_resume_pci - Start.\n",ap->dev_minor);
	
	pci_restore_state(pdev); /* FCLNX-GPL-564 */
	
	/* Enable pdev. This function call "msleep()". */
	if( pci_enable_device(pdev) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_resume_pci - Failed pci_enable_device.\n",ap->dev_minor);
		return -EIO;
	}
	
	HFC_ADAPLOCK_IRQSAVE(flags);
	
	if (ap->is_busmaster)
	{
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		HFC_DBGPRT( "hfcldd%d : hfc_resume_pci - Called pci_set_master.\n",ap->dev_minor);
		pci_set_master(pdev);
	}
	else{
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
	}
	
	/* Set Interrrupts(INTx or MSI or MSI-X) */
	msi_flag = hfc_set_interrupts(ap, ap->msi_enable);
	HFC_ADAPLOCK_IRQSAVE(flags);
	ap->msi_flag = msi_flag;
	
	if(ap->msi_flag < 0)
	{	/* err case */
		HFC_DBGPRT( "hfcldd%d : hfc_resume_pci - Failed to set MSI for Kernel.\n",ap->dev_minor);
		hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4F, (uchar *)&ap->msi_flag, 4) ;/* FCLNX-GPL-161 */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
		return -EIO;
	}
	
	/* No Err */
	HFC_ADAPUNLOCK_IRQRESTORE(flags);
	HFC_DBGPRT( "hfcldd%d : hfc_resume_pci - End.\n",ap->dev_minor);
	return 0;
	
}
#endif	/* FCLNX-GPL-429 */

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_set_hw_mcw_cfg()
 *
 * Purpose:     Set MCW (cfg space)
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "ap" is not NULL pointer.
 *
 */
void 
hfc_set_hw_mcw_cfg(struct adap_info *ap)
{
	ushort pci_reg;
	
	pci_reg = (ushort) hfc_read_cnfg (ap, HFC_HOST_STAT_CMD, 0x2);
	hfc_write_cnfg(ap, HFC_HOST_STAT_CMD, 0x2, (uint64_t)(pci_reg|HFC_HSC_MSS|HFC_HSC_MB));
	
	if ( (ushort) hfc_read_cnfg (ap, HFC_HOST_VENDER_ID, 0x2) == 0x1054 )	/* FIVE FCWIN-0217 */
	{
		if (ap->pkg.type == HFC_PKTYPE_FIVE)
		{
			uchar lsi_rev = (uchar)hfc_read_cnfg(ap, 0x4a, 0x01);
			lsi_rev &= 0x07;
			if ( lsi_rev == 0x02 )
			{
				ushort      wkc;
			
				// Set registers for FIVE Pass2				/* @@ */
				
				// It is set by Max Outstanding Split Transaction one
				wkc = (ushort)hfc_read_cnfg(ap, 0x6a, 0x02);
				wkc &= 0xff8f;
				hfc_write_cnfg(ap, 0x6a, 0x02, wkc);
			
				// Clear Base Address Register (Upper 32-bit) with 0
				hfc_write_cnfg(ap, 0x14, 0x04, 0x00000000);
				
				// Conventional PCI (not PCI-X)? 
				wkc = (ushort)hfc_read_cnfg(ap, 0x94, 0x02);
				wkc &= 0x000f;
				if ( wkc == 0x0000 )
				{
					// Set Disable REQ64# Assertion
					wkc = (ushort)hfc_read_cnfg(ap, 0x90, 0x02);
					wkc |= 0x0001;
					hfc_write_cnfg(ap, 0x90, 0x02, wkc);
				}
			}
		}
	}
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_set_hw_mcw_pci()
 *
 * Purpose:     Set MCW (pci space)
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "ap" is not NULL pointer.
 *
 */
void
hfc_set_hw_mcw_pci(struct adap_info *ap)
{
	ushort pci_reg;
	
	pci_reg = (ushort) hfc_read_cnfg (ap, HFC_HOST_STAT_CMD, 0x2);
	hfc_write_cnfg(ap, HFC_HOST_STAT_CMD, 0x2, (uint64_t)(pci_reg|HFC_HSC_MSS|HFC_HSC_MB));

	if ( (ushort) hfc_read_cnfg (ap, HFC_HOST_VENDER_ID, 0x2) == 0x1054 )	/* FIVE FCWIN-0217 */
	{
		if(ap->pkg.type == HFC_PKTYPE_FPP)
		{
			/* NOP */
		}
		else if(ap->pkg.type == HFC_PKTYPE_FIVE)
		{
			ushort      wkc;
			
			uchar lsi_rev = (uchar)hfc_read_cnfg(ap, 0x4a, 0x01);
			lsi_rev &= 0x07;
			if ( lsi_rev == 0x02 )
			{
			
				// Set registers for FIVE Pass2				/* @@ */

				// Set MCW HFMToCKDSBL
				wkc = (uchar)hfc_read_reg_ext(ap, 0x217, 0x01);
				wkc |= 0x40;
				hfc_write_reg_ext(ap, 0x217, 0x01, wkc);
			
				// Set MCW HMCWBUREQSEL
				wkc = (uchar)hfc_read_reg_ext(ap, 0x229, 0x01);
				wkc |= 0x08;
				hfc_write_reg_ext(ap, 0x229, 0x01, wkc);		/* @@ */
			}
			// Set MCW HMCWBG(0-1)								/* FCLNX-0367 */
			wkc = (uchar)hfc_read_reg_ext(ap, 0x228, 0x01);
			wkc |= 0x30;
			hfc_write_reg_ext(ap, 0x228, 0x01, wkc);			/* FCLNX-0367 */
		}
		else  /* FIVE-EX */
		{
			/* Set LSI revision */ /* FCLNX-GPL-283 */
			ap->pkg.lsi_rev = (uchar)hfc_read_reg(ap, HFC_IOSPACE_LSIREV, 0x01);

			/* Clear any status registers before H/W init.  */
			hfc_clear_status_five_ex(ap);
			/* Set MCW */
			hfc_set_mcw_five_ex(ap);
		}
	}
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-306 */
/*
 * Function:    hfc_start_rport()
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "host" and "host->hostdata" are not NULL pointer.
 *              Caller must unlock adap_lock
 */
void
hfc_start_rport( struct adap_info *ap )
{
	ulong flags = 0;
	struct task_struct	*tmp_thread;
	
	/* Startup the kernel thread for this host adapter. */
	if ( test_bit(HFC_SYSFS_RPORT, (ulong *)&ap->sysfs_control) ) {
		if (!test_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status )) {
			tmp_thread = kthread_run(hfc_do_rport, ap, "hfc_worker_%d", ap->instance);
			if (IS_ERR(tmp_thread)) {
				HFC_ADAPLOCK_IRQSAVE(flags);
				clear_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status );
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xBF, NULL, 0) ;
			}
			else {
				HFC_ADAPLOCK_IRQSAVE(flags);
				set_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status );
			}
			ap->worker_thread = tmp_thread;		/* FCLNX-GPL-333 */
			HFC_ADAPUNLOCK_IRQRESTORE(flags);
		}
	}
	else {
		HFC_ADAPLOCK_IRQSAVE(flags);
		clear_bit( HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status );
		HFC_ADAPUNLOCK_IRQRESTORE(flags);
	}
	
	return;
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_skip_link_init()
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *
 * Returns:     
 *  			TRUE  : Skip "Link initialize".
 *  			FALSE : Don't skip "Link initialize".
 *
 * Notes:       "ap" and "ap->mp_adap_info" are not NULL pointer.
 */
int
hfc_skip_link_init(struct adap_info *ap, ushort bind_err)
{
	struct mp_adap_info *mpap = ap->mp_adap_info;
	ulong                flags = 0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Start.\n",ap->dev_minor);
	
	if((hfc_pxe_boot != 0)|| (bind_err == TRUE) ){          /* FCLNX-0634 */
		HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Bind Error.\n",ap->dev_minor);
		return 1;
	}
	
	mpap = ap->mp_adap_info; /* FCLNX-GPL-196 */
	
	if ( ap->isol_force == HFC_PRT_FRC_ISOL ){		/* Force port isolated state *//* - FCLNX-546 - */
		HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Force port isolated state.\n",ap->dev_minor);
		set_bit( HFC_ISOL, (ulong *)&ap->status );
		ap->isol_detail = HFC_ISOLATE_PORT_C;
		return 1;
	}
	else if (ap->isol_force == HFC_CHKSTP_FRC_ISOL ){	/* FCLNX-0147 */
		HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Force CHKSTOP isolated state.\n",ap->dev_minor);
		
		set_bit( HFC_ISOL, (ulong *)&ap->status );
		ap->isol_detail = HFC_ISOLATE_CHKSTP;	/* FCLNX-0147 */
		
		HFC_ADAPLOCK_IRQSAVE(flags);
			
		HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */
		set_bit( HFC_HWISOL, (ulong *)&mpap->status );	
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY); /* FCLNX-GPL-177 */
		
		HFC_ADAPUNLOCK_IRQRESTORE(flags);

		return 1;
	}
	else if (ap->isol_force == HFC_SHARED_PRT_FRC_ISOL ){
		HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Physical port is isolated.\n",ap->dev_minor);
		
		set_bit( HFC_ISOL, (ulong *)&ap->status );
		hfc_write_reg(ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )0x00000300 );
			
		return 2;	/* FCLNX-GPL-521 */
	}
	
	if( test_bit(HFC_HWCHKSTOP, (ulong *)&mpap->status) ) /* FCLNX-GPL-196 */
	{	/* This core's state was "check stop". */
		/* So, we chage this port's state to "check stop". */
		HFC_ADAPLOCK_IRQSAVE(flags);	/* FCLNX-GPL-FX-466 */
		HFC_ISSUE_CSTP(ap, FALSE, HFC_ABEND_FCSTP_IML );						/* FCLNX-GPL-316 */
		HFC_ADAPUNLOCK_IRQRESTORE(flags);	/* FCLNX-GPL-FX-466 */
		/* And, we skip "link inititalize". */
		HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - Hardware Status is CHKSTOP.\n",ap->dev_minor);
		return 1;
	}
	
	if( !test_bit(HFC_ATTACH, (ulong *)&ap->attach_status ) ){					/* FCLNX-0228 */
		HFC_DBGPRT( " ** hfcldd : hfc_detect - skip hfc_initialize \n");
		return 1;
	} else {
		if ( HFC_MMODE_CHECK_SHADOW(ap) && HFC_MMODE_CHECK_REBOOT(ap) )
		{	/* Skip link initlialization and open int_a_mask */
			HFC_DBGPRT( " ** hfcldd : hfc_detect - skip hfc_initialize by mlpf reboot\n"); 
			hfc_write_reg( ap, ( uint )HFC_IOSPACE_INTA_MSK,( char )0x4, ( int )hfc_inta_mask_mlpf[ap->pkg.type] ); 
			return 1;
		}
	}
	HFC_DBGPRT( "hfcldd%d : hfc_skip_link_init - End.\n",ap->dev_minor);
	
	/* Don't skip "Link initialize". */
	return 0;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-306 */
/*
 * Function:    hfc_stop_rport()
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "host" and "host->hostdata" are not NULL pointer.
 *              Caller must unlock adap_lock
 */
void
hfc_stop_rport(struct adap_info *ap)
{
	
	if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&ap->kthread_status ) ) {
		hfc_kthread_stop(ap);
	}

}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

int hfc_check_hba_isolation(struct adap_info *ap)				/* FCLNX-GPL-349 */
{
	uint logdata[4];
	
	if( (!(HFC_MMODE_CHECK_SHARED(ap))) || (HFC_MMODE_CHECK_SHADOW(ap) ) ){
		if (ap->pkg.type == HFC_PKTYPE_FPP)
			return(0);
		
		if ((ap->pkg.port <= 1)||(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))){
			return(1);
		}
		else {
			if ((ap->ld_err_limit_s)||(ap->if_err_limit)||(ap->to_err_limit)||(ap->rt_err_enable)) {
				logdata[0] = ap->ld_err_limit_s;
				logdata[1] = ap->if_err_limit;
				logdata[2] = ap->to_err_limit;
				logdata[3] = ap->rt_err_enable;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xD6, (uchar *)&logdata[0], 16);
			}
			return(0);
		}
	}
	else{
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&ap->fw_support))
			return(1);
		else{
			if ((ap->ld_err_limit_s)||(ap->if_err_limit)||(ap->to_err_limit)||(ap->rt_err_enable)) {
				logdata[0] = ap->ld_err_limit_s;
				logdata[1] = ap->if_err_limit;
				logdata[2] = ap->to_err_limit;
				logdata[3] = ap->rt_err_enable;
				hfc_errlog(ap, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xD6, (uchar *)&logdata[0], 16);
			}
			return(0);
		}
	}
}																/* FCLNX-GPL-349 */

/* FCLNX-GPL-428 */	
/*
 * Function:    hfc_get_adap_status
 *
 * Purpose:     
 *
 * Arguments:   
 *  ap        - Pointer to adap_info 
 *
 * Returns:     adapter status
 *
 * Notes:       
 */
int	hfc_get_adap_status(struct adap_info *ap){
	int    rtn =0;
	uint   hyp_status;
	
	/* FCLNX-GPL-147 */
	if(!HFC_MMODE_CHECK_SHARED(ap)){
		switch(ap->isol_detail){
			case HFC_NO_ISOLATE:
				if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){     //FCLNX-0488
					if(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)||test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
						rtn = HFC_WAITLINKUP;
					}
					else{ 	/* FCLNX-0488 */
						rtn = HFC_LINKUP;		   /* FCLNX-0488 */
					}
				}
				else{
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;	/* FCLNX-0488 *//* FCLNX-699 */
					}
					else if(test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
						rtn = HFC_WAITLINKUP;
					}
					else{
						rtn = HFC_LINKDOWN;
					}
				}
				break;
			case HFC_ISOLATE_PORT_C:
				if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
					rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
				}
				else {
					rtn = HFC_ISOLATE_C; 				/* FCLNX-0488 */
				}
				break;
			case HFC_ISOLATE_PORT_E:
				if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
					rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
				}
				else {
					rtn = HFC_ISOLATE_E;			/* FCLNX-0488 */
				}
				break;
			case HFC_ISOLATE_SFPFAIL:
				if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
					rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
				}
				else {
					rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
				}
				break;
			case HFC_ISOLATE_SFPNOTSUPPORT:
				if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
					rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
				}
				else {
					rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
				}
				break;
			case HFC_ISOLATE_SFPDOWN:
				if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
					rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
				}
				else {
					rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
				}
				break;
			case HFC_ISOLATE_CHKSTP:
				rtn = HFC_CHKSTP_E;				/* FCLNX-0488 */
				break;
			default:
				rtn = HFC_UNKNOWN_STATUS;			/* FCLNX-0488 */
		}
	}
	else{	/* FCLNX-GPL-428 */
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&ap->fw_support)){
	
			hyp_status = hfc_read_hg_reg(ap, HFC_IOHGSPC_HYPSTATUS, 0x4);
	
			if(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP){
				rtn = HFC_CHKSTP_E;
			}
			else if((hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)||
			(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)||
			(hfc_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL)){
				if(hfc_mlpf_check_state(ap, HFC_HG_HYPSTATUS_ISOLCMD, HFC_CHECK_HYPER_STATE )){
					rtn = HFC_ISOLATE_C;
				}
				else{
					rtn = HFC_ISOLATE_E;
				}
			}
			else{
				switch(ap->isol_detail){
					case HFC_NO_ISOLATE:
						if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){     //FCLNX-0488
							if(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)||test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
								rtn = HFC_WAITLINKUP;
							}
							else{ 	/* FCLNX-0488 */
								rtn = HFC_LINKUP;		   /* FCLNX-0488 */
							}
						}
						else{
							if(test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
								rtn = HFC_WAITLINKUP;
							}
							else{
								rtn = HFC_LINKDOWN;
							}
						}
						break;
					case HFC_ISOLATE_SFPFAIL:
						rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
						break;
					case HFC_ISOLATE_SFPNOTSUPPORT:
						rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
						break;
					case HFC_ISOLATE_SFPDOWN:
						rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
						break;					
				}
			}
		}
		else{
			switch(ap->isol_detail){
				case HFC_NO_ISOLATE:
					if(test_bit(HFC_ONLINE, (ulong *)&ap->status)){     //FCLNX-0488
						if(test_bit(HFC_WAIT_LINKUP, (ulong *)&ap->status)||test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
							rtn = HFC_WAITLINKUP;
						}
						else{ 	/* FCLNX-0488 */
							rtn = HFC_LINKUP;		   /* FCLNX-0488 */
						}
					}
					else{
						if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
							rtn = HFC_CHKSTP_E;	/* FCLNX-0488 *//* FCLNX-699 */
						}
						else if(test_bit(HFC_WAIT_LINK_INIT, (ulong *)&ap->status)){
							rtn = HFC_WAITLINKUP;
						}
						else{
							rtn = HFC_LINKDOWN;
						}
					}
					break;
				case HFC_ISOLATE_PORT_C:
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
					}
					else {
						rtn = HFC_ISOLATE_C; 				/* FCLNX-0488 */
					}
					break;
				case HFC_ISOLATE_PORT_E:
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
					}
					else {
						rtn = HFC_ISOLATE_E;			/* FCLNX-0488 */
					}
					break;
				case HFC_ISOLATE_SFPFAIL:
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
					}
					else {
						rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
					}
					break;
				case HFC_ISOLATE_SFPNOTSUPPORT:
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
					}
					else {
						rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
					}
					break;
				case HFC_ISOLATE_SFPDOWN:
					if(test_bit(HFC_HWCHKSTOP, (ulong *)&ap->mp_adap_info->status)){	/* FCLNX-GPL-393 */
						rtn = HFC_CHKSTP_E;			/* FCLNX-0488 */
					}
					else {
						rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
					}
					break;
				case HFC_ISOLATE_CHKSTP:
					rtn = HFC_CHKSTP_E;				/* FCLNX-0488 */
					break;
				default:
					rtn = HFC_UNKNOWN_STATUS;			/* FCLNX-0488 */
			}
		}
	}
	return(rtn);
}
	/* FCLNX-GPL-428 */
