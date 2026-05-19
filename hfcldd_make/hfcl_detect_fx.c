/*
 * hfcl_detect_fx.c
 * Copyright (C) 2007, 2016, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char det_fx_rcsid[] = "$Id: hfcl_detect_fx.c,v 1.1.2.66.2.12.2.5.2.15.2.2 2016/02/19 03:05:29 mhayashi Exp $";

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
const struct pkg_map hfc_fx_pkg_map[1] = {
	/* FIVE-FX */
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
				0x02f0,         /* 32 :  HFC_IOSPACE_TGTCORE        1B  */
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
				0x0324,         /* 48 :  HFC_IOSPACE_CA_FLAG        1B  */
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
				0x0327, 		/* 65 :  HFC_IOSPACE_CA_ONUP_STATE      */
				0x0370,         /* 66 :  HFC_IOSPACE_FW_SUPPORT		1B  */
				0x0000,         /* 67 :  */
				0x0000,         /* 68 :  */
				0x0000,         /* 69 :  */
				0x0884,         /* 70 :  HFC_IOSPACE_DUMP_CMD       4B  */
				0x083f,			/* 71 :  HFC_IOSPACE_KCMD_IPRES     1B  */
				0x0000,         /* 72 :  */
				0x00e0,         /* 73 :  HFC_IOSPACE_INT_VECTOR     4B  */
				0x00e4,         /* 74 :  HFC_IOSPACE_INT_VECTOR_RST 4B  */
				0x00f0,         /* 75 :  HFC_IOSPACE_NAKED_INT0     4B  */
				0x0110,			/* 76 :  HFC_IOSPACE_PTYP0          4B  */
				0x0000,         /* 77 :  */
				0x0000,         /* 78 :  */
				0x0150,         /* 79 :  HFC_IOSPACE_MAINPF         4B  */
				0x0c10,         /* 80 :  HFC_IOSPACE_CORE0_STATUS0  4B  */
				0x0c31,         /* 81 :  HFC_IOSPACE_CORE0_CMD1     4B  */
				0x0d10,         /* 82 :  HFC_IOSPACE_CORE1_STATUS0  4B  */
				0x0d31,         /* 83 :  HFC_IOSPACE_CORE1_CMD1     4B  */
				0x0e10,         /* 84 :  HFC_IOSPACE_CORE2_STATUS0  4B  */
				0x0e31,         /* 85 :  HFC_IOSPACE_CORE2_CMD1     4B  */
				0x0f10,         /* 86 :  HFC_IOSPACE_CORE3_STATUS0  4B  */
				0x0f31,         /* 87 :  HFC_IOSPACE_CORE3_CMD1     4B  */
				0x0540,         /* 88 :  HFC_IOSPACE_FRAMEB         4B  */
				0x0580,         /* 89 :  HFC_IOSPACE_FRAMEA1        4B  */
				0x05c0,         /* 90 :  HFC_IOSPACE_FRAMEB1        4B  */
				0x0c32,         /* 91 :  HFC_IOSPACE_CORE0_CMD2     4B	*/
				0x0000,         /* 92 :  */
				0x0000,         /* 93 :  */
				0x0153,         /* 94 :  HFC_IOSPACE_HPMFLGEN       1B  */
				0x0154,         /* 95 :  HFC_IOSPACE_HPMFLG         1B  */
				0x0157,         /* 96 :  HFC_IOSPACE_HPMFLGPFNO     1B  */
				0x00b0,         /* 97 :  HFC_IOSPACE_INT_1          4B  */
				0x00b4,         /* 98 :  HFC_IOSPACE_MPINT_1C       4B  */
				0x00b8,         /* 99 :  HFC_IOSPACE_INT_1_MSK      4B  */
				0x00bc,         /* 100:  HFC_IOSPACE_INT_1_RST      4B  */
				0x00c0,         /* 101:  HFC_IOSPACE_INT_2          4B  */
				0x00c4,         /* 102:  HFC_IOSPACE_MPINT_2C       4B  */
				0x00c8,         /* 103:  HFC_IOSPACE_INT_2_MSK      4B  */
				0x00cc,         /* 104:  HFC_IOSPACE_INT_2_RST      4B  */
				0x00d0,         /* 105:  HFC_IOSPACE_INT_3          4B  */
				0x00d4,         /* 106:  HFC_IOSPACE_MPINT_3C       4B  */
				0x00d8,         /* 107:  HFC_IOSPACE_INT_3_MSK      4B  */
				0x00dc,         /* 108:  HFC_IOSPACE_INT_3_RST      4B  */
				0x00e8,         /* 109:  HFC_IOSPACE_MSIXVSHORT     4B  */
				0x0000,         /* 110 :  */
				0x0000,         /* 111 :  */
				0x0380,         /* 112:  HFC_IOSPACE_CORE1_CA_POSTRESULT  4B  */
				0x0400,         /* 113:  HFC_IOSPACE_CORE2_CA_POSTRESULT  4B  */
				0x0480,         /* 114:  HFC_IOSPACE_CORE3_CA_POSTRESULT  4B  */
				0x0390,			/* 115:	 HFC_IOSPACE_CA_INIT1_ADDR0 4B	*/
				0x0394,			/* 116:	 HFC_IOSPACE_CA_INIT1_ADDR1 4B	*/
				0x0410,			/* 117:	 HFC_IOSPACE_CA_INIT2_ADDR0 4B	*/
				0x0414,			/* 118:	 HFC_IOSPACE_CA_INIT2_ADDR1 4B	*/
				0x0490,			/* 119:	 HFC_IOSPACE_CA_INIT3_ADDR0 4B	*/
				0x0494,			/* 120:	 HFC_IOSPACE_CA_INIT3_ADDR1 4B	*/
				0x038c,			/* 121:	 HFC_IOSPACE_CA_FLAG1		4B	*/
				0x040c,			/* 122:	 HFC_IOSPACE_CA_FLAG2		4B	*/
				0x048c,			/* 123:	 HFC_IOSPACE_CA_FLAG3		4B	*/
				0x3000,			/* 124:	 HFC_IOSPACE_RSS_INTA		4B	*//* Multi Queue */
				0x3004,			/* 125:	 HFC_IOSPACE_RSS_INTA_RST	4B	*//* Multi Queue */
				0x3008,			/* 126:	 HFC_IOSPACE_RSS_INT_VECTOR	4B	*//* Multi Queue */
				0x300c,			/* 127:	 HFC_IOSPACE_RSS_INT_VECTOR_RST	4B	*//* Multi Queue */
			}
		}
	}
};


/* AL-PA# */
const uchar posmap_lisa_fx[128] = {
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

//struct scsi_transport_template *hfc_fx_fc_attach_transport=NULL;


#if 0
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
#endif

/* driver information */
extern struct manage_info hfc_manage_info;			/* driver information */
extern struct file_operations hfc_fops;
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
extern struct miscdevice hfc_miscdev;
#endif
extern int instance;
extern int hfc_major;
extern int hba_num;
extern int hfc_pxe_boot;
extern int hfc_automap;
extern int hfc_narrowmap;
extern int hfc_scsi_time_out;
extern int hfc_message_enable;
extern int hfc_shadow;
extern int hfc_rport_lu_scan;
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
extern struct scsi_transport_template *hfc_fc_attach_transport;
#endif
#endif /* SYSFS_SUPPORT */

extern int adapter_bindings;
extern struct narrow_dev hfc_narrow_dev;					/* FCLNX-0392 */

char *hfclddconf;


#if _HFC_ERROR_INJ
int hfc_fx_debug_ioerr_code = 0;
#endif

#ifndef _HFC_NO_RASLOG
extern struct hraslogopt_st hraslogopt;
#endif


extern uint	raslog_install;

extern void print_module_parameter(void);
int hfc_fx_read_hfcbios(struct port_info *pp);
#if 0
/* FCLNX-GPL-204 STR */
int hfc_fx_proc_info_pfb(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout);
int hfc_fx_strategy_pfb(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *));
int hfc_fx_eh_abort_pfb(struct scsi_cmnd *cmnd);
int hfc_fx_eh_device_reset_pfb(struct scsi_cmnd *cmnd);
int hfc_fx_eh_target_reset_pfb(struct scsi_cmnd *cmnd);				/* FCLNX-GPL-0449 */
int hfc_fx_eh_bus_reset_pfb(struct scsi_cmnd *cmnd);
/* FCLNX-GPL-204 END */
#endif

#define MIN_IOBASE_LEN          0x100

int hfc_fx_check_cs_disable(struct port_info *pp, struct core_info *core){															\
	if (( (!(pp->mlpf_mode & HFC_MMODE_MLPF )) || (pp->mlpf_mode & HFC_MMODE_DEDICATE) ) || (pp->mlpf_mode & HFC_MMODE_SHADOW ) ){	\
		return(core->status & (0x00000001 << HFC_CS_CHK_STOP));  									\
	}else{																					\
		return((core->status & (0x00000001 << HFC_CS_CHK_STOP)) |									\
		(!(core->status & (0x00000001 << HFC_CS_CORE_ENABLE)))); 								\
	}
}
/* FCLNX-GPL-FX-438 */


int hfc_fx_pci_conf(struct port_info *pp)
{
	struct pci_dev *pdev	= NULL;
	unsigned long base0, len0, flag0, base4;
/*	ushort pci_reg; */ /* FCLNX-GPL-306 */
	uchar  basic = 0;
	
	HFC_ENTRY("hfc_fx_pci_conf");
																	/* @MLPF */


	pdev = pp->pci_cfginf;
	
	base4 = pci_resource_start(pdev, 4);
	if(base4 == 0)
		basic = 1;
																	/* @MLPF */
	if ( basic == 1 || HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		/* Initialize PCI config register */ 
		hfc_fx_set_hw_mcw_cfg(pp); /* FCLNX-GPL-306 */
	}
																	/* @MLPF */


	HFC_DBGPRT( "  hfcldd : hfc_fx_pci_conf - allocate pci_resource\n"); 
	
	base0 = pci_resource_start(pdev, 0);
	len0 = pci_resource_len(pdev, 0);
	flag0 = pci_resource_flags(pdev, 0);
	HFC_DBGPRT("PCI resource 0 base0:%x, len0:%x, flag0:%x \n", (uint)base0, (uint)len0, (uint)flag0);

	if (!(flag0 & IORESOURCE_MEM)) {
		HFC_DBGPRT("  scsi(%ld): region #0 not a PIO resource, aborting\n",
				pp->host_no);

		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCA, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;
	}

	if (len0 < MIN_IOBASE_LEN) {
		HFC_DBGPRT("  scsi(%d): Invalid PCI I/O region size, aborting\n",
				(uint)pp->host_no);

		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCB, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;
	}

	if (pci_request_regions(pdev, "hfcldd")) {
		HFC_DBGPRT("  scsi(%d): Failed to reserve PCI_MEMORY_SPACE\n",
				(uint)pp->host_no);

		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCC, NULL, 0) ;/* FCLNX-GPL-161 */

		goto resource_error_exit;

	}

	pp->mem_base_addr = (ulong)ioremap(base0, len0);
	if (!pp->mem_base_addr){
		HFC_DBGPRT(" can not remap memory mpaped IO \n");
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCD, NULL, 0) ;/* FCLNX-GPL-161 */
		goto iospace_error_exit;
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_attach - config pci devices end \n"); 
	HFC_DBGPRT( "  scsi(%ld): Success to reserve PCI_MEMORY_SPACE\n",
				(ulong)pp->host_no);
	
	if ( hfc_fx_mlpf_setup_lparmode(pp) == HFC_MLPF_DISABLE )
		return (-ENODEV);
	
	/* initialize PCI config register */ 
	if ( (!(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
	{
		hfc_fx_set_hw_mcw_pci(pp); /* FCLNX-GPL-306 */
	}
	else{	/*** Set only LSI rev ***/ /* FCLNX-GPL-579 */
		if (pp->pkg.type == HFC_PKTYPE_FIVE_EX) {
			pp->pkg.lsi_rev = (uchar)hfc_fx_read_reg(pp, HFC_IOSPACE_LSIREV, 0x01);
		}
	}
																	/* @MLPF */

	HFC_EXIT("hfc_fx_pci_conf");
	
    return (0);
    
iospace_error_exit:
        pci_release_regions(pdev);

resource_error_exit:

	return (-ENOMEM);
}

int hfc_fx_attach(struct port_info *pp)
{
	int 	i;
	
	HFC_ENTRY("hfc_fx_attach");

	/* set port_info */
	init_waitqueue_head(&pp->mb_event);
	init_waitqueue_head(&pp->ioctl_event);
	init_waitqueue_head(&pp->ioexe_event);
	init_waitqueue_head(&pp->mck_event);
	init_waitqueue_head(&pp->init_event);
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	init_waitqueue_head(&pp->rport_event);
#endif
#endif /* SYSFS_SUPPORT */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	sema_init(&pp->ioctl_sem, 1) ;
	sema_init(&pp->sem, 1) ;
#else
	init_MUTEX(&pp->ioctl_sem) ;
	init_MUTEX(&pp->sem) ;
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

	/* Initialize spinlock */
	spin_lock_init(&pp->port_lock);

	/* Allocate table area (Non DMA area)  */
	if (hfc_fx_allocate_memory(pp, pp->rid)) goto attach_error_exit;
	
	if (HFC_FX_EXT_VPORT_EXIST(pp->region_arg[pp->rid])) {
		/* two or more vports exists in region */
		return (0);
	}
	
	for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if (hfc_fx_allocate_memory_core(pp, pp->rid, i)) goto attach_error_exit;
	}
	
	/* allocate init_table, xob, xrb, mailbox, seg_info, soft_log (DMA area) */
	for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if ( hfc_fx_allocate_dma(pp, pp->rid, i) )    goto attach_error_exit;
	}
	
	/* Initialize spinlock for core */
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] == NULL) goto attach_error_exit;
		if(pp->region_arg[pp->rid]->core_arg[i] == NULL)  goto attach_error_exit;
		spin_lock_init(&pp->region_arg[pp->rid]->core_arg[i]->core_lock);
	}
	
	HFC_EXIT("hfc_fx_attach");
	return (0);
	
attach_error_exit:
	for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		hfc_fx_free_dma(pp, pp->rid, i);
	}
	
	for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		hfc_fx_free_memory_core(pp, pp->rid, i);
	}
	hfc_fx_free_memory(pp, pp->rid);

	HFC_EXIT("hfc_fx_attach");
	return (1);
}


int hfc_fx_allocate_memory(struct port_info *pp, int rid)
{
	struct	scatterlist		*sgl=NULL;
	struct	hfc_pkt_fx		*hfcp;
	int 	i;
	uchar	logdata[16];

	HFC_ENTRY("hfc_fx_allocate_memory");
	memset(logdata, 0, 16);
	
	/* allocate region_info */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate region_info \n"); 
	if (pp->pport->region_arg[rid] != NULL) {
		pp->region_arg[rid] = pp->pport->region_arg[rid];
	}
	else if (pp->region_arg[rid] == NULL) {
		pp->region_arg[rid] = (struct region_info *)hfc_fx_kmalloc(pp, sizeof(struct region_info), GFP_KERNEL);
		if (pp->region_arg[rid] == NULL){
			logdata[0] = 0x01;
			goto attachmem_error_exit;
		}
		memset( pp->region_arg[rid], 0, sizeof(struct region_info) );
		pp->region_arg[rid]->pport = pp->pport;
		pp->region_arg[rid]->rid = rid;
		/* Set structure character name */
		strcpy(pp->region_arg[rid]->name, "region_info");
		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate region_info logical=%lx\n",
			(ulong)pp->region_arg[rid]);
		init_waitqueue_head(&pp->region_arg[rid]->mb_lock_event);
	}
	pp->region_arg[rid]->port_num++;

	/* allocate trace area for port_info */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate trace area for port_info\n"); 
	if (pp->trc_ptr == NULL) {
		pp->trc_ptr = (struct hfctrace *)hfc_fx_kmalloc(pp, (sizeof(struct hfctrace) * pp->trc_max),GFP_KERNEL);
		if (pp->trc_ptr == NULL) {
			logdata[0] = 0x02;
			goto attachmem_error_exit;
		}
		memset( pp->trc_ptr, 0, (sizeof(struct hfctrace) * pp->trc_max) );
//		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate trace area for port_info logical=%lx\n",
//			(ulong)pp->trc_ptr);
	}

	/* allocate target_scan */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate target_scan\n"); 
	if(pp->target_scan == NULL) {
		pp->target_scan = (struct target_scan *)hfc_fx_kmalloc(pp, (sizeof(struct target_scan)*MAX_TARGET_PROBE), GFP_KERNEL);
		if(pp->target_scan == NULL) {
			logdata[0] = 0x03;
			goto attachmem_error_exit;
		}
		memset( pp->target_scan, 0, (sizeof(struct target_scan)*MAX_TARGET_PROBE) );
//		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate target_scan logical=%lx\n",
//			(ulong)pp->target_scan);
	}

	/* allocate hfc_pkt */
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		for (i=0;i<pp->pre_pkt_num;i++) {
			hfcp = (struct hfc_pkt_fx *)hfc_fx_kmalloc(pp, sizeof(struct hfc_pkt_fx), GFP_KERNEL);
			if (hfcp == NULL) {
				logdata[0] = 0x14;
				goto attachmem_error_exit;
			}
			memset( hfcp, 0, sizeof(struct hfc_pkt_fx) );
			if (pp->pkt_top == NULL) {
				pp->pkt_top  = hfcp;
				pp->pkt_end  = hfcp;
				pp->pkt_next = hfcp;
			}
			else {
				pp->pkt_end->pkt_next = hfcp;
				hfcp->pkt_prev = pp->pkt_end;
				pp->pkt_end = hfcp;
			}
			hfcp->pkt_no = pp->pkt_num;
			pp->pkt_num++;
			if (pp->pkt_num >= pp->can_queue)
				break;
		}
	}
	
	/* allocate reserve hfc_pkt */
	if (pp->rsv_pkt_pool == NULL) {
		pp->rsv_pkt_pool = (struct hfc_pkt_fx *)hfc_fx_kmalloc(pp, (sizeof(struct hfc_pkt_fx)*pp->rsv_pkt_num), GFP_KERNEL);
		if(pp->rsv_pkt_pool == NULL) {
			logdata[0] = 0x15;
			goto attachmem_error_exit;
		}
		memset( pp->rsv_pkt_pool, 0, sizeof(struct hfc_pkt_fx)*pp->rsv_pkt_num );
	}

	/* allocate hfc_pm_pkt */
	if (pp->pm_control == HFC_FX_PM_ON) {
		if (pp->pm_pkt_pool == NULL) {
			pp->pm_pkt_pool = (struct hfc_pm_pkt_fx *)hfc_fx_kmalloc(pp, (sizeof(struct hfc_pm_pkt_fx)*pp->pm_pkt_num), GFP_KERNEL);
			if(pp->pm_pkt_pool == NULL) {
				logdata[0] = 0x16;
				goto attachmem_error_exit;
			}
			memset( pp->pm_pkt_pool, 0, sizeof(struct hfc_pm_pkt_fx)*pp->pm_pkt_num );
		}
	}

	/* allocate hw_log area */
	if (HFC_FX_PHYSICAL_PORT(pp)) {
//		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate hw_log area\n");
		if (pp->hw_log == NULL) {
			pp->hw_log = (uint *)hfc_fx_kmalloc(pp, HFC_FX_HWLOG_SIZE, GFP_KERNEL);
			if (pp->hw_log == NULL){
				logdata[0] = 0x08;
				goto attachmem_error_exit;
			}
			memset( pp->hw_log, 0, HFC_FX_HWLOG_SIZE );
//			HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate hw_log area logical=%lx\n",
//				(ulong)pp->hw_log);
		}
	}

	/* allocate vport_info for npiv */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate vport_info for npiv\n"); 
	if ((HFC_FX_PHYSICAL_PORT(pp) && (pp->vport_ptr == NULL))) {
		pp->vport_ptr = (struct vport_info *)hfc_fx_kmalloc(pp, (sizeof(struct vport_info)*(pp->max_vport_count+1)),GFP_KERNEL);
		if (pp->vport_ptr == NULL) {
			logdata[0] = 0x12;
			goto attachmem_error_exit;
		}
		memset( pp->vport_ptr, 0, (sizeof(struct vport_info)*(pp->max_vport_count+1)) );
//		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate vport_info for npiv logical=%lx\n",
//			(ulong)pp->vport_ptr);
	}

	if (HFC_FX_VIRTUAL_PORT(pp))
		return (0);

	/* allocate scsi_cmnd for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_cmnd for ioctl\n"); 
	pp->ioctl_cmnd = (struct scsi_cmnd *)hfc_fx_kmalloc(pp, (sizeof(struct scsi_cmnd)), GFP_KERNEL);
	if (pp->ioctl_cmnd == NULL){
		logdata[0] = 0x09;
		goto attachmem_error_exit;
	}
	memset( pp->ioctl_cmnd, 0, sizeof(struct scsi_cmnd) );
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_cmnd for ioctl logical=%lx\n",
//		(ulong)pp->ioctl_cmnd);

	/* allocate scsi_device for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_device for ioctl\n"); 
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->device == NULL){
			pp->ioctl_cmnd->device = (struct scsi_device *)hfc_fx_kmalloc(pp, sizeof(struct scsi_device), GFP_KERNEL);
			if (pp->ioctl_cmnd->device == NULL){
				logdata[0] = 0x0a;
				goto attachmem_error_exit;
			}
			memset( pp->ioctl_cmnd->device, 0, sizeof(struct scsi_device) );
//			HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_device for ioctl logical=%lx\n",
//				(ulong)pp->ioctl_cmnd->device);
		}
	}

	/* allocate scsi_cmnd->cmnd area for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_cmnd->cmnd[] area for ioctl\n"); 
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->cmnd == NULL){
			pp->ioctl_cmnd->cmnd = (uchar *)hfc_fx_kmalloc(pp, 16, GFP_KERNEL);
			if (pp->ioctl_cmnd->cmnd == NULL){
				logdata[0] = 0x0b;
				goto attachmem_error_exit;
			}
			memset( pp->ioctl_cmnd->cmnd, 0, 16 );
//			HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_cmnd->cmnd area for ioctl logical=%lx\n",
//				(ulong)pp->ioctl_cmnd->cmnd);
		}
	}

	/* allocate request_queue for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_device for ioctl\n"); 
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->device != NULL){
			if(pp->ioctl_cmnd->device->request_queue == NULL){
				pp->ioctl_cmnd->device->request_queue = (struct request_queue *)hfc_fx_kmalloc(pp, sizeof(struct request_queue), GFP_KERNEL);
				if (pp->ioctl_cmnd->device->request_queue == NULL){
					logdata[0] = 0x0c;
					goto attachmem_error_exit;
				}
				memset( pp->ioctl_cmnd->device->request_queue, 0, sizeof(struct request_queue) );
//				HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate request_queue for ioctl logical=%lx\n",
//					(ulong)pp->ioctl_cmnd->device->request_queue);
			}
		}
	}

	/* allocate dev_info_fx for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scsi_device for ioctl\n");
	if (pp->ioctl_dev == NULL) {
		pp->ioctl_dev = (struct dev_info_fx *)hfc_fx_kmalloc(pp, sizeof(struct dev_info_fx), GFP_KERNEL);
		if (pp->ioctl_dev == NULL){
			logdata[0] = 0x0d;
			goto attachmem_error_exit;
		}
		memset( pp->ioctl_dev, 0, sizeof(struct dev_info_fx) );
//		HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate dev_info_fx for ioctl logical=%lx\n",
//			(ulong)pp->ioctl_dev);
	}
	
	/* allocate scatterlist for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scatterlist for ioctl\n"); 
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->sdb.table.sgl == NULL){
			pp->ioctl_cmnd->sdb.table.sgl = (struct scatterlist *)hfc_fx_kmalloc(pp, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM, GFP_KERNEL);
			if (pp->ioctl_cmnd->sdb.table.sgl == NULL){
				logdata[0] = 0x0e;
				goto attachmem_error_exit;
			}
			memset( pp->ioctl_cmnd->sdb.table.sgl, 0, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM );
//			HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate scatterlist for ioctl logical=%lx\n",
//				(ulong)pp->ioctl_cmnd->sdb.table.sgl);
		}
	}
	
	/* allocate data buffer (DMA area) for ioctl */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate data buffer for ioctl\n"); 
	if(pp->ioctl_cmnd != NULL) {
		sgl = pp->ioctl_cmnd->sdb.table.sgl;
		for(i=0;i<HFC_SCATTERLIST_NUM;i++){
			if( sgl != NULL ){
				if( sgl->page_link == (unsigned long)0 ){
					sgl->page_link = (unsigned long)hfc_fx_dma_alloc_coherent(pp, &pp->pci_cfginf->dev, HFC_PAGE_SIZE, &sgl->dma_address,GFP_ATOMIC);
					if (sgl->page_link == (unsigned long)0){
						logdata[0] = 0x0f;
						goto attachmem_error_exit;
					}
					memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
//					HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate data buffer (DMA area) for ioctl logical=%lx\n",
//						(ulong)sgl->page_link);
				}
				sgl->length = HFC_PAGE_SIZE;
				sgl++;
			}
			else {
				break;
			}
		}
	}

	HFC_EXIT("hfc_fx_allocate_memory");

	return (0); 

attachmem_error_exit:
	hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
	hfc_fx_free_memory(pp, FALSE);

	return (1); /* fail */
}

int hfc_fx_allocate_memory_core(struct port_info *pp, int rid, int core_no)
{
	struct core_info	*core;
	int 				i;
	uchar				logdata[16];
	
	HFC_ENTRY("hfc_fx_allocate_memory_core");
	memset(logdata, 0, 16);
	
	/* allocate core_info */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate core_info for ioctl\n");
	if(pp->region_arg[rid] != NULL){
		if(pp->region_arg[rid]->core_arg[core_no] == NULL){
			pp->region_arg[rid]->core_arg[core_no] = (struct core_info *)hfc_fx_kmalloc(pp, sizeof(struct core_info), GFP_KERNEL);
			if (pp->region_arg[rid]->core_arg[core_no] == NULL){
				logdata[0] = 0x20; logdata[1] = rid; logdata[2] = core_no;
				goto attachmem_core_error_exit;
			}
			memset( pp->region_arg[rid]->core_arg[core_no], 0, sizeof(struct core_info) );
			pp->region_arg[rid]->core_arg[core_no]->pcore_no = pp->port_no + core_no;
			pp->region_arg[rid]->core_arg[core_no]->core_no = core_no;
			pp->region_arg[rid]->core_arg[core_no]->rp = pp->region_arg[rid];
			
			pp->region_arg[rid]->core_arg[core_no]->iov_map_cnt
				= (pp->dma_max / HFC_PAGE_SIZE/ 256) * HFC_IOVMAP_CNT_1M;
			
			/* Set structure character name */
			strcpy(pp->region_arg[rid]->core_arg[core_no]->name, "core_info");
//			HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate core_info logical=%lx\n",
//				(ulong)pp->region_arg[rid]->core_arg[core_no]);
		}
	}
	
	/* allocate trace area for core_info */
//	HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate trace area for core_info\n"); 
	if(pp->region_arg[rid] != NULL){
		if(pp->region_arg[rid]->core_arg[core_no] != NULL){
			core = pp->region_arg[rid]->core_arg[core_no];
			if(core->trc_ptr == NULL) {
				core->trc_max = pp->trc_max;
				core->trc_ptr = (struct hfctrace *)hfc_fx_kmalloc(pp, (sizeof(struct hfctrace) * core->trc_max),GFP_KERNEL);
				if (core->trc_ptr == NULL) {
					logdata[0] = 0x21; logdata[1] = rid; logdata[2] = core_no;
					goto attachmem_core_error_exit;
				}
				memset( core->trc_ptr, 0, (sizeof(struct hfctrace) * core->trc_max) );
//				HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate trace area for core_info logical=%lx\n",
//					(ulong)core->trc_ptr);
			}
		}
	}
	
	/* allocate iov_map */
//	HFC_DBGPRT( "  hfcldd : hfc_fx_allocate_memory - allocate iov_map \n");
	if(pp->region_arg[rid] != NULL){
		if(pp->region_arg[rid]->core_arg[core_no] != NULL){
			core = pp->region_arg[rid]->core_arg[core_no];
			if(core->iov_map == NULL) {
				core->iov_map = (uint *)hfc_fx_kmalloc(pp,(core->iov_map_cnt / 8),GFP_KERNEL);
				if (core->iov_map == NULL) {
					logdata[0] = 0x22; logdata[1] = rid; logdata[2] = core_no;
					goto attachmem_core_error_exit;
				}
				memset( core->iov_map, 0, (core->iov_map_cnt / 8) );
//				HFC_DBGPRT(" hfcldd : hfc_fx_allocate_memory - allocate iov_map logical=%lx\n",
//					(ulong)core->iov_map);
				
				/* initialize iov_map */
//				HFC_DBGPRT( "  hfcldd : hfc_fx_allocate_memory - initialize iov_map \n");
				for ( i = 0; i < pp->region_arg[rid]->core_arg[core_no]->iov_map_cnt; i++ ){
					pp->region_arg[rid]->core_arg[core_no]->iov_map[i/32] |= (0x80000000 >> (i%32));
				}
			}
		}
	}
	
	/* allocate icc_errlog for core_info */
	if(pp->region_arg[rid] != NULL){
		if(pp->region_arg[rid]->core_arg[core_no] != NULL){
			core = pp->region_arg[rid]->core_arg[core_no];
			if(core->icc_err == NULL) {
				core->icc_err = (struct icc_errlog *)hfc_fx_kmalloc(pp, sizeof(struct icc_errlog), GFP_KERNEL);
				if (core->icc_err == NULL) {
					logdata[0] = 0x23; logdata[1] = rid; logdata[2] = core_no;
					goto attachmem_core_error_exit;
				}
				memset( core->icc_err, 0, sizeof(struct icc_errlog) );
			}
		}
	}
	
	return (0);
	
attachmem_core_error_exit:
	hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
	hfc_fx_free_memory_core(pp, rid, core_no);

	return (1); /* fail */
}

int hfc_fx_allocate_dma(struct port_info *pp, int rid, int core_no)
{
	int 					an = 0;
	dma_addr_t				bus_addr_table[10];
	void					*vir_addr_table[10];
	int						size_table[10];
	struct pci_dev			*pdev = NULL;
	int 					seg_info_cnt, num;
	uint64_t				work_adr;
	uchar					logdata[16];
	uint					work_max;

	HFC_ENTRY("hfc_fx_allocate_dma");

	pdev = pp->pci_cfginf;
	
	/* check pci_device */
	if (pdev == NULL) goto alloc_error_exit;
	
	/* check region_arg[rid] */
	if (pp->region_arg[rid] == NULL) goto alloc_error_exit;
	
	/* check core_arg[core_no] */
	if (pp->region_arg[rid]->core_arg[core_no] == NULL) goto alloc_error_exit;
	
	/* allocate f/w init table (4096 Bytes) */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate fw_init_table \n", pp->dev_minor);
	size_table[an] = sizeof(struct fw_init_tbl_fx);
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->fw_init_p = (struct fw_init_tbl_fx *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->fw_init_p == NULL) { 	/* failed? */
		logdata[0] = 0x30; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->fw_init_p, 0, (int)size_table[an]);
	pp->region_arg[rid]->core_arg[core_no]->padr_init = bus_addr_table[an];
//	HFC_DBGPRT("  (init table) logical=%lx, physical=%lx, size = %d \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->fw_init_p,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->padr_init, size_table[an]);
	an++;
	
	
	/* allocate xob table (4096 * 4 Bytes) */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate xob \n", pp->dev_minor);
	size_table[an] = (sizeof(struct xob_fx)) * pp->xob_max;
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->xob = (struct xob_fx *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->xob == NULL) {	/* failed ? */
		logdata[0] = 0x31; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->xob, 0, (int)size_table[an]);
	pp->region_arg[rid]->core_arg[core_no]->fw_init_p->xob_num = pp->xob_max;
	pp->region_arg[rid]->core_arg[core_no]->phys_xob = bus_addr_table[an];
//	HFC_DBGPRT("  (init table) logical=%lx, physical=%lx, size = %d \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->xob,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_xob, size_table[an]);
	
	/* set each page top address to f/w init table */
	work_adr = (uint64_t)pp->region_arg[rid]->core_arg[core_no]->phys_xob;
	work_max = pp->xob_max/(HFC_PAGE_SIZE/sizeof(struct xob_fx));
	if (pp->xob_max & 0x1f) {
		work_max++;
	}
	for (num=0;num<work_max;num++) {
		hfc_fx_write_val( pp->region_arg[rid]->core_arg[core_no]->fw_init_p->fw_bus_addr[HFC_FX_INIT_TABLE_XOB_OFFSET + num], work_adr );
		work_adr += HFC_PAGE_SIZE;
	}
	an++;
	
	
	/* allocate for xrb table */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate xrb \n", pp->dev_minor);
	size_table[an] = (sizeof(struct xrb_fx)) * pp->xrb_max;
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->xrb = (struct xrb_fx *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->xrb == NULL) {
		logdata[0] = 0x32; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->xrb, 0, (int)size_table[an]);
	pp->region_arg[rid]->core_arg[core_no]->fw_init_p->xrb_num = pp->xrb_max;
	pp->region_arg[rid]->core_arg[core_no]->phys_xrb = bus_addr_table[an];
//	HFC_DBGPRT("  (xrb) logical=%lx, physical=%lx, size = %d \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->xrb,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_xrb, size_table[an]);
	
	/* set each page top address to f/w init table */
	work_adr = (uint64_t)pp->region_arg[rid]->core_arg[core_no]->phys_xrb;
	work_max = pp->xrb_max/(HFC_PAGE_SIZE/sizeof(struct xrb_fx));
	if (pp->xob_max & 0x07) {
		work_max++;
	}
	for (num=0;num<work_max;num++) {
		hfc_fx_write_val( pp->region_arg[rid]->core_arg[core_no]->fw_init_p->fw_bus_addr[HFC_FX_INIT_TABLE_XRB_OFFSET + num], work_adr );
		work_adr += HFC_PAGE_SIZE;
	}
	an++;
	
	
	/* allocate mailbox */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate mailbox \n", pp->dev_minor);
	size_table[an] = sizeof(struct mailbox_fx);
	if (pp->region_arg[rid]->core_arg[core_no] == NULL) goto alloc_error_exit;
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->mb = (struct mailbox_fx *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->mb == NULL) {
		logdata[0] = 0x37; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	pp->region_arg[rid]->core_arg[core_no]->phys_mb = bus_addr_table[an];
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->mb, 0, (int)size_table[an]);
//	HFC_DBGPRT("  (mailbox) logical=%lx, physical=%lx, size = %d \n",
//			   (ulong)pp->region_arg[rid]->core_arg[core_no]->mb,
//			   (ulong)pp->region_arg[rid]->core_arg[core_no]->phys_mb,
//			   size_table[an]);
	an++;
	
	
	/* allocate mailbox parameter */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate mailbox parameter \n", pp->dev_minor);
	size_table[an] = 4096 * 2;
	if (pp->region_arg[rid]->core_arg[core_no] == NULL) goto alloc_error_exit;
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->mb_parm = (uchar *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->mb_parm == NULL) {
		logdata[0] = 0x33; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	pp->region_arg[rid]->core_arg[core_no]->phys_mb_parm = bus_addr_table[an];
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->mb_parm, 0, 4096 * 2);
//	HFC_DBGPRT("  (init table) logical=%lx, physical=%lx, size = %d \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->mb_parm,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_mb_parm, size_table[an]);
	an++;
	
	
	/* allocate soft log area */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate soft log area \n", pp->dev_minor);
	for (num=0; num<pp->slog_max ;num++) {
		pp->region_arg[rid]->core_arg[core_no]->slog_addr[num] =
			(uchar *)hfc_fx_dma_alloc_coherent (
				pp,
				&pdev->dev,
				(uint)HFC_PAGE_SIZE,
				&pp->region_arg[rid]->core_arg[core_no]->phys_slog[num],
				GFP_ATOMIC);
		if (pp->region_arg[rid]->core_arg[core_no]->slog_addr[num] == NULL) {
			logdata[0] = 0x34; logdata[1] = rid; logdata[2] = core_no;
			goto alloc_error_exit;
		}
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->slog_addr[num], 0, HFC_PAGE_SIZE);
		hfc_fx_write_val ( pp->region_arg[rid]->core_arg[core_no]->fw_init_p->fw_bus_addr[HFC_FX_INIT_TABLE_SLOG_OFFSET + num],
			(uint64_t)pp->region_arg[rid]->core_arg[core_no]->phys_slog[num] );
//		HFC_DBGPRT("  (soft log area [%d]) logical=%lx, physical=%lx\n",
//			num,
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->slog_addr[num],
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_slog[num]);
	}
	pp->region_arg[rid]->core_arg[core_no]->slog = (uchar *)pp->region_arg[rid]->core_arg[core_no]->slog_addr[0];
//	HFC_DBGPRT("  (soft log area) logical=%lx, physical=%lx, size = %d, slog=%lx \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->slog,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_slog[0],
//		HFC_PAGE_SIZE,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->slog);
	
	
	/* allocate seg_info */
//	HFC_DBGPRT( "  hfcldd%d : hfc_fx_alloc_dma - allocate seg_info \n", pp->dev_minor);
	seg_info_cnt = (pp->dma_max / HFC_PAGE_SIZE);
	size_table[an] = HFC_ALIGNPG (seg_info_cnt * sizeof(struct seg_info_fx) );
	vir_addr_table[an] = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size_table[an], &bus_addr_table[an],GFP_ATOMIC);
	pp->region_arg[rid]->core_arg[core_no]->seg_info = (struct seg_info_fx *)vir_addr_table[an];
	if (pp->region_arg[rid]->core_arg[core_no]->seg_info == NULL) {
		logdata[0] = 0x35; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->seg_info, 0, (int)size_table[an]);
	pp->region_arg[rid]->core_arg[core_no]->seg_phys_addr = bus_addr_table[an];
//	HFC_DBGPRT("  (seg_info) logical=%lx, physical=%lx, size = %d seg_info_cnt=%d \n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->seg_info,
//		(ulong)pp->region_arg[rid]->core_arg[core_no]->seg_phys_addr,
//		size_table[an],
//		seg_info_cnt);
	an++;
	
	
	/* allocate Frmsndrcv Payload Area */
//	HFC_DBGPRT( "  hfcldd : hfc_fx_alloc_dma - allocate Frmsndrcv Payload Area \n");
	pp->region_arg[rid]->core_arg[core_no]->payload = 
			hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX, 
				&pp->region_arg[rid]->core_arg[core_no]->phys_payload,GFP_ATOMIC);
	if (pp->region_arg[rid]->core_arg[core_no]->payload == NULL) {
		logdata[0] = 0x36; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX);
//	HFC_DBGPRT("  (Frmsndrcv Payload Area) logical=%lx, physical=%lx \n",
//	(ulong)pp->region_arg[rid]->core_arg[core_no]->payload,
//	(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_payload);
	
	pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload = 
			hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)(HFC_PAGE_SIZE/2), 
				&pp->region_arg[rid]->core_arg[core_no]->phys_rcvfrm_payload,GFP_ATOMIC);
	if (pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload == NULL) {
		logdata[0] = 0x37; logdata[1] = rid; logdata[2] = core_no;
		goto alloc_error_exit;
	}
	memset((char *)pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload, 0, (uint)(HFC_PAGE_SIZE/2));
//	HFC_DBGPRT("  (Frmsndrcv Payload Area) logical=%lx, physical=%lx \n",
//	(ulong)pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload,
//	(ulong)pp->region_arg[rid]->core_arg[core_no]->phys_rcvfrm_payload);

	HFC_EXIT("hfc_fx_allocate_dma");

	return (0); /* Normal end */

alloc_error_exit:
	HFC_DBGPRT(" hfcldd : hfc_fx_alloc_dma - exit with error (alloc_error_exit) \n");
	hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
	hfc_fx_free_dma(pp, rid, core_no);
	HFC_EXIT("hfc_fx_allocate_dma (error) ");
	return (1);

}


int hfc_fx_free_memory(struct port_info *pp, int rid)
{
	int i,j;
	struct target_info_fx	*target;
	struct scatterlist		*sgl=NULL;
	struct hfc_pkt_fx		*hfcp, *hfcp_next;
	struct port_info		*wkpp;
	
	HFC_ENTRY("hfc_fx_free_memory");
	
	/* check pci_device */
	if (pp->pci_cfginf == NULL)
		return (0);
	
	/* check region_arg[rid] */
	if (pp->region_arg[rid] == NULL)
		return (0);

	/* Release hw_log area */
	if (pp->hw_log != NULL) {
		memset( pp->hw_log, 0, HFC_FX_HWLOG_SIZE );
		hfc_fx_kfree(pp, pp->hw_log);
		pp->hw_log = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release hw_log area logical=%lx\n",
			(ulong)pp->hw_log);
	}
	
	/* Release data buffer (DMA area) for ioctl */
	if(pp->ioctl_cmnd != NULL) {
		sgl = pp->ioctl_cmnd->sdb.table.sgl;
		for(i=0;i<HFC_SCATTERLIST_NUM;i++){
			if( sgl != NULL ){
				if( sgl->page_link != (unsigned long)0 ){
					sgl->page_link &= ~0x02;
					memset( (char *)sgl->page_link, 0, HFC_PAGE_SIZE );
					hfc_fx_dma_free_coherent(pp, &pp->pci_cfginf->dev, (size_t)HFC_PAGE_SIZE, (void *)sgl->page_link, (dma_addr_t)sgl->dma_address);
					sgl->page_link = (unsigned long)0;
					HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - Release data buffer (DMA area) for ioctl logical=%lx\n",
						(ulong)sgl->page_link);
				}
				sgl++;
			}
			else {
				break;
			}
		}
	}
	
	/* release vport_info for npiv */
	if(pp->vport_ptr != NULL) {
		memset( pp->vport_ptr, 0, (sizeof(struct vport_info)*(pp->max_vport_count+1)) );
		hfc_fx_kfree(pp, pp->vport_ptr);
		pp->vport_ptr = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release vport_info for npiv logical=%lx\n",
				(ulong)pp->vport_ptr);
		
	}
	
	/* release scatterlist for ioctl */
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->sdb.table.sgl != NULL){
			memset( pp->ioctl_cmnd->sdb.table.sgl, 0, sizeof(struct scatterlist)*HFC_SCATTERLIST_NUM );
			hfc_fx_kfree(pp, pp->ioctl_cmnd->sdb.table.sgl);
			pp->ioctl_cmnd->sdb.table.sgl = NULL;
			HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release scatterlist for ioctl logical=%lx\n",
				(ulong)pp->ioctl_cmnd->sdb.table.sgl);
		}
		
	}
	
	/* release request_queue for ioctl */
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->device != NULL){
			if(pp->ioctl_cmnd->device->request_queue != NULL){
				memset( pp->ioctl_cmnd->device->request_queue, 0, sizeof(struct request_queue) );
				hfc_fx_kfree(pp, pp->ioctl_cmnd->device->request_queue);
				pp->ioctl_cmnd->device->request_queue = NULL;
				HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release request_queue for ioctl logical=%lx\n",
					(ulong)pp->ioctl_cmnd->device->request_queue);
			}
		}
	}

	/* release dev_info_fx for ioctl */
	if(pp->ioctl_dev != NULL) {
		memset( pp->ioctl_dev, 0, sizeof(struct dev_info_fx) );
		hfc_fx_kfree(pp, pp->ioctl_dev);
		pp->ioctl_dev = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release dev_info_fx for ioctl logical=%lx\n",
			(ulong)pp->ioctl_dev);
	}
	
	/* release scsi_cmnd->cmnd area for ioctl */
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->cmnd != NULL){
			memset( pp->ioctl_cmnd->cmnd, 0, 16 );
			hfc_fx_kfree(pp, pp->ioctl_cmnd->cmnd);
			pp->ioctl_cmnd->cmnd = NULL;
			HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release scsi_cmnd->cmnd area for ioctl logical=%lx\n",
				(ulong)pp->ioctl_cmnd->cmnd);
		}
	}

	/* release scsi_device for ioctl */
	if(pp->ioctl_cmnd != NULL) {
		if(pp->ioctl_cmnd->device != NULL){
			memset( pp->ioctl_cmnd->device, 0, sizeof(struct scsi_device) );
			hfc_fx_kfree(pp, pp->ioctl_cmnd->device);
			pp->ioctl_cmnd->device = NULL;
			HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release scsi_device for ioctl logical=%lx\n",
				(ulong)pp->ioctl_cmnd->device);
		}
	}

	/* release scsi_cmnd for ioctl */
	if(pp->ioctl_cmnd != NULL){
		memset( pp->ioctl_cmnd, 0, sizeof(struct scsi_cmnd) );
		hfc_fx_kfree(pp, pp->ioctl_cmnd);
		pp->ioctl_cmnd = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release scsi_cmnd for ioctl logical=%lx\n",
			(ulong)pp->ioctl_cmnd);
	}

	/* release target_info */
	for (i=0;i<MAX_TARGET_PROBE;i++) {
		target = pp->target_arg[i];
		if (target != NULL) {
			if (!HFC_FX_MQ_VIRTUAL_PORT(pp)) {
				hfc_fx_free_dev(target);
			}
			for(j=0;j<MAX_CORE_PROBE_FX;j+=MAX_CORE_PROBE_FX/pp->core_num){
				if (target->dummy_cmnd[j] != NULL) {
					memset( target->dummy_cmnd[j], 0, sizeof(struct dummy_scsi_cmnd) );
					hfc_fx_kfree(pp, target->dummy_cmnd[j]);
					target->dummy_cmnd[j] = NULL;
					HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release target->dummy_cmnd logical=%lx\n",
						(ulong)target->dummy_cmnd[j]);
				}
			}
			if (target->reset_pkt != NULL) {
				memset( target->reset_pkt, 0, sizeof(struct hfc_pkt_fx)*MAX_CORE_PROBE_FX*3 );
				hfc_fx_kfree(pp, target->reset_pkt);
				target->reset_pkt = NULL;
				HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release target->reset_pkt logical=%lx\n",
					(ulong)target->reset_pkt);
			}
			memset( pp->target_arg[i], 0, sizeof(struct target_info_fx) );
			hfc_fx_kfree(pp, pp->target_arg[i]);
			pp->target_arg[i] = NULL;
			HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release target_info logical=%lx\n",
				(ulong)pp->target_arg[i]);
		}
	}
	
	/* release hfc_pm_pkt */
	if (pp->pm_pkt_pool != NULL) {
		memset( pp->pm_pkt_pool, 0, sizeof(struct hfc_pm_pkt_fx)*pp->pm_pkt_num );
		hfc_fx_kfree(pp, pp->pm_pkt_pool);
		pp->pm_pkt_pool = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release pm_pkt_pool logical=%lx\n",
			(ulong)pp->pm_pkt_pool);
	}
	
	/* release reserve hfc_pkt */
	if (pp->rsv_pkt_pool != NULL) {
		memset( pp->rsv_pkt_pool, 0, sizeof(struct hfc_pkt_fx)*pp->rsv_pkt_num );
		hfc_fx_kfree(pp, pp->rsv_pkt_pool);
		pp->rsv_pkt_pool = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release rsv_pkt_pool logical=%lx\n",
			(ulong)pp->rsv_pkt_pool);
	}
	
	/* release pkt_pool */
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		hfcp = pp->pkt_top;
		while (hfcp != NULL) {
			hfcp_next = hfcp->pkt_next;
			memset( hfcp, 0, sizeof(struct hfc_pkt_fx) );
			hfc_fx_kfree(pp, hfcp);
			hfcp = hfcp_next;
		}
	}
	
	/* release target_scan */
	if (pp->target_scan != NULL) {
		memset(pp->target_scan, 0, (sizeof(struct target_scan)*MAX_TARGET_PROBE) );
		hfc_fx_kfree(pp, pp->target_scan);
		pp->target_scan = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release target_scan logical=%lx\n",
			(ulong)pp->target_scan);
	}
	
	/* release trace area */
	if (pp->trc_ptr != NULL) {
		memset(pp->trc_ptr, 0, (sizeof(struct hfctrace) * pp->trc_max) );
		hfc_fx_kfree(pp, pp->trc_ptr);
		pp->trc_ptr = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release trace area logical=%lx\n",
			(ulong)pp->trc_ptr);
	}
	
	/* release region_info */
	if (!HFC_FX_EXT_VPORT_EXIST(pp->region_arg[rid])) {
		/* one vport exists in region */
		if (pp->region_arg[rid] != NULL) {
			memset(pp->region_arg[rid], 0, (sizeof(struct region_info)));
			hfc_fx_kfree(pp, pp->region_arg[rid]);
			pp->region_arg[rid] = NULL;
			HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release region_info logical=%lx\n",
				(ulong)pp->region_arg[rid]);
		}
	}
	else {
		if (pp->region_arg[rid] != NULL) {
			pp->region_arg[rid]->port_num--;
		}
	}
	
	/* release port_info */
	if (HFC_FX_MQ_VIRTUAL_PORT(pp)) {
		wkpp = pp->pport;
		memset(pp, 0, sizeof(struct port_info) );
		hfc_fx_kfree(wkpp, pp);
		pp = NULL;
		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory - release port_info=%lx\n",
			(ulong)pp);
	}
	
	HFC_EXIT("hfc_fx_free_memory");
	
	return(0);
}


int hfc_fx_free_memory_core(struct port_info *pp, int rid, int core_no)
{
	struct core_info	*core;
	
	HFC_ENTRY("hfc_fx_free_memory_core");
	
	if (pp->pci_cfginf == NULL)
		return (0);
	
	if (!pp->region_arg[rid])
		return(0);
	
	if (!pp->region_arg[rid]->core_arg[core_no])
		return(0);
	
	core = pp->region_arg[rid]->core_arg[core_no];
	
	/* release icc_errlog */
	if (core->icc_err != NULL) {
		memset(core->icc_err, 0, sizeof(struct icc_errlog) );
		hfc_fx_kfree(pp, core->icc_err);
		core->icc_err = NULL;
	}
	
	/* release iov_map */
	if (core->iov_map != NULL) {
		memset(core->iov_map, 0, (core->iov_map_cnt / 8) );
		hfc_fx_kfree(pp, core->iov_map);
		core->iov_map = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory_core - release iov_map logical=%lx\n",
//			(ulong)core->iov_map);
	}
	
	/* release trace area */
	if (core->trc_ptr != NULL) {
		memset(core->trc_ptr, 0, (sizeof(struct hfctrace) * core->trc_max) );
		hfc_fx_kfree(pp, core->trc_ptr);
		core->trc_ptr = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_memory_core - release trace area logical=%lx\n",
//			(ulong)core->trc_ptr);
	}
	
	/* release core_info */
	memset(core, 0, sizeof(struct core_info));
	hfc_fx_kfree(pp, core);
	pp->region_arg[rid]->core_arg[core_no] = NULL;
//	HFC_DBGPRT(" hfcldd : hfc_fx_free_memory_core - release core_info logical=%lx\n",
//		(ulong)pp->region_arg[rid]->core_arg[core_no]);
	
	HFC_EXIT("hfc_fx_free_memory_core");
	
	return(0);
}

int hfc_fx_free_dma(struct port_info *pp, int rid, int core_no)
{
	struct pci_dev		*pdev = NULL;
	int 				i;
	int 				seg_info_cnt;

	size_t size;

	pdev = pp->pci_cfginf;

	HFC_ENTRY("hfc_fx_free_dma");	
	
	/* check pci_device */
	if (pdev == NULL)
		return (0);
	
	/* check region_arg[rid] */
	if (pp->region_arg[rid] == NULL)
		return (0);
	
	/* check core_arg[core_no] */
	if (pp->region_arg[rid]->core_arg[core_no] == NULL)
		return (0);
	
	if ( pp->region_arg[rid]->core_arg[core_no]->fw_init_p != NULL ) {
		/* Release f/w init table (4096 Bytes) */
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->fw_init_p, 0, (int)sizeof(struct fw_init_tbl_fx));
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)sizeof(struct fw_init_tbl_fx),
			(void *)pp->region_arg[rid]->core_arg[core_no]->fw_init_p,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->padr_init );
		pp->region_arg[rid]->core_arg[core_no]->fw_init_p = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free fw_init_tbl logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->fw_init_p);
	}

	if (pp->region_arg[rid]->core_arg[core_no]->xob != NULL) {
		/* Release xob table (4096 * 4 Bytes) */
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->xob, 0, (int)(sizeof(struct xob_fx) * pp->xob_max));
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)(sizeof(struct xob_fx) * pp->xob_max),
			(void *)pp->region_arg[rid]->core_arg[core_no]->xob,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_xob );
		pp->region_arg[rid]->core_arg[core_no]->xob = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free xob_tbl logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->xob);
	}

	if (pp->region_arg[rid]->core_arg[core_no]->xrb != NULL) {
		/* Release xrb table */
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->xrb, 0, (int)(sizeof(struct xrb_fx) * pp->xrb_max));
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)(sizeof(struct xrb_fx) * pp->xrb_max),
			(void *)pp->region_arg[rid]->core_arg[core_no]->xrb,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_xrb );
		pp->region_arg[rid]->core_arg[core_no]->xrb = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free xrb tbl logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->xrb);
	}

	if (pp->region_arg[rid]->core_arg[core_no]->mb != NULL) {
		/* Release mailbox */
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->mb, 0, (int)sizeof(struct mailbox_fx));
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)sizeof(struct mailbox_fx),
			(void *)pp->region_arg[rid]->core_arg[core_no]->mb,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_mb);
		pp->region_arg[rid]->core_arg[core_no]->mb = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free fw_mailbox logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->mb);
	}

	if (pp->region_arg[rid]->core_arg[core_no]->mb_parm != NULL) {
		/* Release mailbox parameter */
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->mb_parm, 0, (int)(4096 * 2) );
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)(4096 * 2),
			(void *)pp->region_arg[rid]->core_arg[core_no]->mb_parm,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_mb_parm );
		pp->region_arg[rid]->core_arg[core_no]->mb_parm = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free mailbox parameter logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->mb_parm);
	}

	for (i=0; i<pp->slog_max; i++) {
		/* Release soft log area */
		if (pp->region_arg[rid]->core_arg[core_no]->slog_addr[i] != NULL) {
			memset((char *)pp->region_arg[rid]->core_arg[core_no]->slog_addr[i], 0, (int)HFC_PAGE_SIZE );
			hfc_fx_dma_free_coherent(
				pp,
				&pdev->dev,
				(size_t)HFC_PAGE_SIZE,
				(void *)pp->region_arg[rid]->core_arg[core_no]->slog_addr[i],
				(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_slog[i] );
			pp->region_arg[rid]->core_arg[core_no]->slog_addr[i] = NULL;
//			HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free soft log area logical=%lx\n",
//				(ulong)pp->region_arg[rid]->core_arg[core_no]->slog_addr[i]);
		}
	}

	if (pp->region_arg[rid]->core_arg[core_no]->seg_info != NULL) {
		/* Release seg_info */
		seg_info_cnt = (pp->dma_max / HFC_PAGE_SIZE);
		size =	HFC_ALIGNPG (seg_info_cnt * sizeof(struct seg_info_fx) );
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->seg_info, 0, (int)size);
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)size,
			(void *)pp->region_arg[rid]->core_arg[core_no]->seg_info,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->seg_phys_addr);
		pp->region_arg[rid]->core_arg[core_no]->seg_info = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free seg_info logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->seg_info);
	}
	
	if (pp->region_arg[rid]->core_arg[core_no]->payload != NULL) {
		/* Release Payload Area of Frmsndrcv */
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free Payload \n");
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->payload, 0, (uint)HFC_SEND_PAYLOADL_MAX+HFC_RECV_PAYLOADL_MAX);
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)HFC_PAGE_SIZE,
			(void *)pp->region_arg[rid]->core_arg[core_no]->payload,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_payload);
		pp->region_arg[rid]->core_arg[core_no]->payload = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free Payload logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->payload);
	}
	
	if (pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload != NULL) {
		/* Release Payload Area of Rcvfrm */
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free Payload of rcvfrm \n");
		memset((char *)pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload, 0, (int)(HFC_PAGE_SIZE/2));
		hfc_fx_dma_free_coherent(
			pp,
			&pdev->dev,
			(size_t)(HFC_PAGE_SIZE/2),
			(void *)pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload,
			(dma_addr_t)pp->region_arg[rid]->core_arg[core_no]->phys_rcvfrm_payload);
		pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload = NULL;
//		HFC_DBGPRT(" hfcldd : hfc_fx_free_dma - free Payload logical=%lx\n",
//			(ulong)pp->region_arg[rid]->core_arg[core_no]->rcvfrm_payload);
	}

	HFC_EXIT("hfc_fx_free_dma");

	return (0); /* Normal end */
}


void hfc_fx_detach(struct port_info *pp)
{
	int 	i;
	
	HFC_ENTRY("hfc_fx_detach");
	
	if (!HFC_FX_EXT_VPORT_EXIST(pp->region_arg[pp->rid])) {
		/* one vport exist in region */
		
		/* Release DMA memory area */
		for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			hfc_fx_free_dma(pp, pp->rid, i);
		}
		
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			hfc_manage_info.npubp->hfc_fx_remove_lgpath(pp);
		}
		
		if(HFC_FX_MMODE_CHECK_MLPF(pp)&&(pp->hg_cca_p != NULL)){
			hfc_fx_free_mlpf_cca(pp);
		}
		
		/* Release adapter memory area */
		for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			hfc_fx_free_memory_core(pp, pp->rid, i);
		}
	}
	
	hfc_fx_free_memory(pp, pp->rid);
	
	HFC_EXIT("hfc_fx_detach");
}

/*
 * Function: hfc_fx_determine_master_core
 *
 * Purpose:  Determine master core
 *
 * Arguments:
 *  pp       - Pointer to port_info
 *  rp       - Pointer to region_info
 *
 * Returns:
 *
 * Notes:
 */
void hfc_fx_determine_master_core(struct port_info *pp, struct region_info *rp)
{
	int core_no;
	struct core_info *core;
	uchar find_master_core = FALSE;

	pp->available_pcore = 0;
	for(core_no=0; core_no<MAX_CORE_PROBE_FX; core_no+=MAX_CORE_PROBE_FX/pp->core_num){		/* FCLNX-GPL-587 */
		core = pp->region_arg[ pp->rid ]->core_arg[core_no];
		if (core == NULL)
			continue;
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */
		
		set_bit( HFC_CS_HWINIT_COMP, (ulong *)&core->status);

		if (find_master_core == FALSE) {
			/* Most least number of available core is master_core_no */
			pp->master_core_no = (uchar)core_no;
			find_master_core   = TRUE;
		}
		pp->available_pcore |= (0x80 >> core->pcore_no);
	}

	return;
}

void hfc_fx_set_fw_init_tbl(struct port_info *pp)
{
	int		core_no, available_core = 0, i=0;
	struct core_info	*core=NULL;

	HFC_DBGPRT( "hfcldd : set_fw_init_tbl - HFC_ENTRY, rid=%d\n", pp->rid);
	HFC_ENTRY("hfc_fx_set_hw_init_tbl");

	for(core_no=0; core_no<MAX_CORE_PROBE_FX; core_no+=MAX_CORE_PROBE_FX/pp->core_num){
		core = pp->region_arg[pp->rid]->core_arg[core_no];
		if( core == NULL )
			continue;
		
		if(hfc_fx_check_cs_disable(pp, core)){
			hfc_fx_write_val( core->fw_init_p->flag, 0x00 );
			continue;	/* FCLNX-GPL-FX-438 */
		}
		
		i += 1;
		
		/* +00 INIT_TABLE_REV */
		hfc_fx_write_val( core->fw_init_p->init_tbl_rev, HFC_FWINIT_REV_20);
		/* +01 FLAG */
		if (i == 1) {
			/* FCLNX-GPL-FX-404 Start */
			if ( (!(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
				hfc_fx_write_val( core->fw_init_p->flag, 0xC0 );
			else hfc_fx_write_val( core->fw_init_p->flag, 0x80 );
			/* FCLNX-GPL-FX-404 End */
		}
		else hfc_fx_write_val( core->fw_init_p->flag, 0x00 );

		/* +02 CORE_NO */
		hfc_fx_write_val( core->fw_init_p->core_info.core_no, pp->region_arg[pp->rid]->core_arg[core_no]->pcore_no);
		if( pp->port_no == 0){
			hfc_fx_write_val( core->fw_init_p->core_info.master_core_no, pp->master_core_no);
		}
		else{
			hfc_fx_write_val( core->fw_init_p->core_info.master_core_no, pp->region_arg[pp->rid]->core_arg[pp->master_core_no]->pcore_no);
		}
		switch(pp->core_num){
		case 1:
			available_core |= 0x80 >> pp->region_arg[pp->rid]->core_arg[core_no]->pcore_no	;
			break;
		case 2:
			available_core |= 0x80 >> pp->region_arg[pp->rid]->core_arg[core_no]->pcore_no	;
			break;

//			if(pp->port_no==0) available_core = 0xa0;
//			else available_core = 0x50;
//			break;
		case 4:
			available_core |= 0x80 >> pp->region_arg[pp->rid]->core_arg[core_no]->pcore_no	;
			break;

//			available_core = 0xf0;
//			break;
		}
//		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->core_info.available_core, available_core);

		/* +04 - +07 Driver Support Information */
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->dd_support_info, HFC_DDSP_DEFAULT);

#if 0
		if ( pp->pkg.type == HFC_PKTYPE_FPP )
		{
			pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0x4B, 0x1);
		}
		else if( pp->pkg.type == HFC_PKTYPE_FIVE )
		{
			pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xAF, 0x1);
		}
		else /* FIVE-EX */
		{
			pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xCA, 0x1);
		}
#endif

#if 0
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->fw_rnid.n_port_name, pp->ww_name);
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->fw_rnid.node_name, pp->node_name);
		/* FCLNX-0299 */
#endif

		/* +08 - +0F Mailbox ADDR: pointer to Mailbox top */
		hfc_fx_write_val(
			pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->mb_addr,
			(uint64_t)pp->region_arg[pp->rid]->core_arg[core_no]->phys_mb );

		/* +10 - +13 XOB_num: XOB Queue HFC_ENTRY number */
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->xob_num,  pp->xob_max);
		/* +14 - +17 XRB_num: XRB Queue HFC_ENTRY number */
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->xrb_num,  pp->xrb_max);
		/* +18 - +1B SOFTLOG_num: Soft Log page number */
		hfc_fx_write_val(
			pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->slog_num,
			pp->slog_max*4 );

		/* +2A - +2F SPMA_MAC_Address */
		hfc_fx_write_val(
			pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->spma_mac_address, 0x000000000000 );

#if 0
		/* +200 - +23F XOB_Queue_ADDR */
		j=0;
		for (i=0;i<8;i++) {
			fw_bus_addr[i] = (void *)((uchar *)pp->region_arg[pp->rid]->core_arg[core_no]->phys_xob + j);
			j+=4096;
		}
		/* +240 - +33F XRB_Queue_ADDR */
		j=0;
		for (;i<40;i++) {
			fw_bus_addr[i] = (void *)((uchar *)pp->region_arg[pp->rid]->core_arg[core_no]->phys_xrb + j);
			j+=4096;
		}
		/* +340 - +37F resv */
		for (;i<48;i++);
		/* +380 - +57F */
		j=0;
		for (;i<112;i++) {
			fw_bus_addr[i] = (void *)((uchar *)pp->region_arg[pp->rid]->core_arg[core_no]->phys_slog + j);
			j+=4096;
		}
#endif

#if 0
		/* +2A-2B Soft Log HFC_ENTRY length */
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->slog_len, HFC_SLOG_LEN);


		/* +180 trace information  */
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_num, 0x01 );
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.inst,    0x04 );

		for (i=0;i<4;i++) {
			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].seg_no,        i );
			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].resv0,         0x00 );

			if ((i==0) && !(pp->fw_parm & HFC_FWP_SEGTRC_V))														/* FCWIN-0192 */
				hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].mode, 	 0x60 );	/* FCWIN-0192 */
			else
				hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].mode, 	 0xf8 );

			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].ctl,           0x00 );
			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].top_port_id,   0x00000000 );
			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].bottom_port_id,0x00ffffff );
			hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[i].resv1,         0x00000000 );
		}
		hfc_fx_write_val( pp->region_arg[pp->rid]->core_arg[core_no]->fw_init_p->trc_info.trc_seg[0].seg_no, 0x80 );	
	}

#endif

	}
	for(core_no=0; core_no<MAX_CORE_PROBE_FX; core_no+=MAX_CORE_PROBE_FX/pp->core_num){
		core = pp->region_arg[pp->rid]->core_arg[core_no];
		if( core == NULL )
			continue;
		
		if(hfc_fx_check_cs_disable(pp, core))
			continue;	/* FCLNX-GPL-FX-438 */
		
		hfc_fx_write_val( core->fw_init_p->core_info.available_core, available_core);
	}

	HFC_EXIT("hfc_fx_set_hw_init_tbl");
}

#define HFC_VPD_WAITCNT 			5000000
#define CHECKSUM_FPP_ADD_ADDR		0x55
#define CHECKSUM_FIVE_ADD_ADDR		0x63
#define CHECKSUM_FIVE_EX_ADD_ADDR	0x73 /* FIVE-EX */
#define CHECKSUM_FIVE_FX_ADD_ADDR	0x73 /* FIVE-FX */
#define VPDDATA_LEN                 0x80
#define VPD_ADDRESS_FLAG_MASK       0x80
#define HFC_CAPABILITIES_POINTER    0x34
#define HFC_CAPABILITIES_ADDR       0x40

int hfc_fx_get_vpd(struct port_info *pp, uchar *vpd_buf)
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
	
	HFC_ENTRY("hfc_fx_get_vpd");
//	HFC_DBGPRT("hfc_fx_get_vpd");

	if  ( (!(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
	{
		/* Read status register */
		value = (uint) hfc_fx_read_cnfg(pp, HFC_HOST_STAT_CMD, 4);
		HFC_4B_TO_4L(value_bak,value);

//		HFC_DBGPRT("hfc_fx_get_vpd: HFC_HOST_STAT_CMD: value=0x%04x, value_bak=0x%04x\n",value,value_bak);

		if (!(value & 0x00100000)) {	/* bit 4(CAPAbilities list) = 0? */
			/* VPD is not supported */
			err = 0x71;
			goto getvpd_error_exit;
		}
		
		/* Read CAPAbilities list(0x34) */
		value = (uint) hfc_fx_read_cnfg(pp, HFC_CAPABILITIES_POINTER, 4 );
		HFC_4B_TO_4L(value_bak,value);

//		HFC_DBGPRT("hfc_fx_get_vpd: HFC_CAPABILITIES_POINTER: value=0x%04x, value_bak=0x%04x\n",value,value_bak);

		if ((value & 0x000000FF) != HFC_CAPABILITIES_ADDR) {
			/* Abnormal end (data is not 0x40) */
			err = 0x72;
			goto getvpd_error_exit;
		}

		/* FIVE-FX */
		if( pp->pkg.type == HFC_PKTYPE_FIVE_FX)
		{
			/* Search Capabitilies ID (VPD) */
			value = pci_find_capability(pp->pci_cfginf, PCI_CAP_ID_VPD);
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

//		HFC_DBGPRT("hfc_fx_get_vpd: capabilities_list_addr = 0x%04x\n",value);

		vpd_ptr = (char *)vpd_buf;
		offset = 0;
		do {
			value = 0;
			adr = 0x0000 | offset;
			hfc_fx_write_cnfg(pp, (capabilities_list_addr + 0x2), 2, adr);

			i = 0;
			while (i != HFC_VPD_WAITCNT) {
				value = (uint) hfc_fx_read_cnfg(pp, capabilities_list_addr, 4);
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
			value = (uint) hfc_fx_read_cnfg(pp, (capabilities_list_addr + 0x4), 4);
			HFC_4B_TO_4L(value_bak,value);

			/* Store data in reverse order */
			*vpd_ptr++ = value         & 0xFF;
			*vpd_ptr++ = (value >> 8)  & 0xFF;
			*vpd_ptr++ = (value >> 16) & 0xFF;
			*vpd_ptr++ = (value >> 24) & 0xFF;

			offset += 4;
		} while (offset != VPDDATA_LEN);

		/* Caliculate checksum */
		if ( pp->pkg.type == HFC_PKTYPE_FIVE_FX )
		{
			chk_sum = CHECKSUM_FIVE_FX_ADD_ADDR;
		}
		vpd_ptr = (char *)vpd_buf;
		for (sum = 0, nwords = 0; nwords <= chk_sum; nwords++) {
			sum += *vpd_ptr++;
//			HFC_DBGPRT("%d ",sum);
		}

		/* Checksum = 0? */
		if (sum != 0) {
			err = 0x75;
			goto getvpd_error_exit;
		}
	}
	else /* This case is considered for Guest LPAR */
	{
		hfc_fx_mlpf_get_mmio_hg(pp, pp->vpd_buf, HFC_IOHGSPC_VPDAREA, HFC_IOHGSPC_VPDAREA_LEN);
	}

//	HFC_DBGPRT("hfcldd: hfc_fx_get_vpd: dump vpd data\n");
//	structdump( 0xf1, (uchar *)vpd_buf, VPDDATA_LEN);

	HFC_EXIT("hfc_fx_get_vpd");

	return(0);

getvpd_error_exit:
	/* Get driver Log */
	hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, err, NULL, 0) ;
	HFC_DBGPRT("hfcldd: hfc_fx_get_vpd: Read VPD error\n");

		/* FIVE-FX: Normal End */
		return (1);
}


int hfc_fx_query_pktype(struct port_info *pp)
{
	HFC_ENTRY("hfc_fx_query_pktype");
	
	HFC_DBGPRT( " memory map base addr = %lx\n", pp->mem_base_addr);

	/* Read package code from adr = 0x005	*/
	pp->pkg.code = (uchar) hfc_fx_read_reg_ext(pp, 0x005, 0x1);

	HFC_DBGPRT( " package code = %01x\n", pp->pkg.code);

	HFC_EXIT("hfc_fx_query_pktype");
	
	return 0;
}


int hfc_fx_release_adp(struct port_info *pp)
{
	uint				i,j,wait,lp;
	ulong				flags = 0;
	struct target_info_fx	*target;
	struct region_info		*rp;
	struct port_info		*wkpp;

	HFC_ENTRY("hfc_fx_release_adp");

	rp = pp->region_arg[pp->rid];
	/* If rp is not configured or opened, return (abend) */
	if ( rp == NULL )
		return 0;

	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);

	/* Closing process is in progress */
	set_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2);
	pp->initialize = 0;
	
	if(!test_bit(HFC_PS_ISOL,(ulong *)&pp->pport->status)){
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(rp->core_arg[i] != NULL){
					if(!hfc_fx_check_cs_disable(pp, rp->core_arg[i])){	/* FCLNX-GPL-FX-438 */
						/* Allow MBRSP_INT, mask other interrupt factors */
						HFC_DBGPRT( "hfc_fx_release_adp() - Allow MBRSP_INT, mask other interrupt factors\n");
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
						  (char)0x4, (int)HFC_MBRSP_INT, HFC_FX_CORE_OFFSET10);
						
						/* Clear interrupt factors other than MBRSP_INT */
						HFC_DBGPRT( "hfc_fx_release_adp() - Clear interrupt factors other than MBRSP_INT\n");
						hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
						  (char)0x4, (int)~HFC_MBRSP_INT, HFC_FX_CORE_OFFSET10);
					}
				}
			}
		}
		
		if (HFC_FX_PHYSICAL_PORT(pp) || HFC_FX_MQ_VIRTUAL_PORT(pp)) {
			/* Require OFFLINE_MB */
			HFC_DBGPRT( "hfc_fx_release_adp() - Require OFFLINE_MB\n");
			set_bit(HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp->status_detail2);
			start_fx_next_mailbox(pp, NULL);
			
			do {
				wait=0;		
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
					wait = hfc_manage_info.npubp->hfc_fx_wait_mp_ioend(pp);
				if (!wait) {
					wait = (test_bit(HFC_PD_NEED_OFFLINE_MB, (ulong *)&pp->status_detail2) ||
							test_bit(HFC_PD_WAIT_OFFLINE_MB, (ulong *)&pp->status_detail2) );
				}
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				msleep(1); /* sleep for a while */
				HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			} while (wait);
		}
		else if ((test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status)) &&
				(!test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->pport->status))) {	/* FCLNX-GPL-FX-248,272 */
			/* Require CANCEL_SCSI */
			HFC_DBGPRT( "hfc_fx_release_adp() - Require CANCEL_SCSI\n");
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++){
				target = hfc_fx_hash_target_info(pp, lp);
				if( target == NULL )
					continue;
				
				if (test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->pport->status)) {
					break;
				}
				else {
					set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);
					start_fx_next_mailbox(pp, NULL);
				}
				
				do {
					if (test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->pport->status)) {
						clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);
						clear_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);
						break;
					}
					wait = (test_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status) ||
							test_bit(HFC_TS_WAIT_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status) );
					HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
					msleep(1); /* sleep for a while */
					HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
				} while (wait);
			}/* FCLNX-GPL-FX-248 */
			
			/* Require LOGO to FC-SW */
			HFC_DBGPRT( "hfc_fx_release_adp() - Require LOGO to FC-SW\n");
			
			set_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);
			set_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1);
			start_fx_next_mailbox(pp, NULL);
			
			do {
				wait=0;
				/* FCLNX-GPL-FX-248 */
				if (test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->pport->status)) {
					wait=1;
					clear_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1);
					clear_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1);
					clear_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1);
					clear_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1);
				}	/* FCLNX-GPL-FX-248 */
				if (!wait) {
					wait = (test_bit(HFC_PD_NEED_LOGO_FCSW, (ulong *)&pp->status_detail1)  ||
							test_bit(HFC_PD_WAIT_LOGO_FCSW, (ulong *)&pp->status_detail1)  ||
							test_bit(HFC_PD_NEED_DEL_PORTID, (ulong *)&pp->status_detail1) ||
							test_bit(HFC_PD_WAIT_DEL_PORTID, (ulong *)&pp->status_detail1));
				}
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				msleep(1); /* sleep for a while */
				HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			} while (wait);
		}
	}
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(!hfc_fx_check_cs_disable(pp, rp->core_arg[i])){	/* FCLNX-GPL-FX-438 */
			
			if (HFC_FX_PHYSICAL_PORT(pp)) {
				/* Set interrupt mask OFF */
				HFC_DBGPRT( "hfc_fx_release_adp() - Set interrupt mask OFF\n");
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
				  (char)0x4, 0x0, HFC_FX_CORE_OFFSET10);
				  
				/* Clear all interrupt factors */
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
				  (char)0x4, (int)~0, HFC_FX_CORE_OFFSET10);
			}
			
			/* Cancel SCSI commands */
			for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++){
				target = hfc_fx_hash_target_info(pp, lp);
				if( target != NULL ){
					hfc_fx_cancel_scsi_cmd(
						pp, rp->core_arg[i], target, 0, NULL, SCS_ADAPTER_OFFLINE,
						HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
					target->status = HFC_NON_STATUS;
				}
			}
		}
	}

	/* Clear target_info */
	for (i=0;i<pp->max_target;i++) {
		target = pp->target_arg[i];
		if (target != NULL){
			hfc_fx_clear_target_info_fx( pp, target, FALSE );
		}
	}

	/* Stop timer (port_info, target_info) */
	hfc_fx_reset_all_timer(pp);
	HFC_DBGPRT("hfc_fx_release_adp: reset_all_timer\n");

	if ( test_bit(HFC_PS_ENABLE, (ulong *)&pp->status) ) {
		pp->status = 0;
		set_bit(HFC_PS_ENABLE, (ulong *)&pp->status);
	}
	else {
		pp->status = 0;
	}
	
	if (HFC_FX_VIRTUAL_PORT(pp) && HFC_FX_VPORT_ENABLE(pp)) {
		if (HFC_FX_EXT_VPORT_EXIST(pp->region_arg[pp->rid])) {
			/* two or more vports exists in region */
			HFC_DBGPRT( "hfc_fx_release_adp() - two or more vports exists in region\n");
			
			/* rid unregister */
			hfc_fx_rid_unregister(pp->pport, pp);
		}
		else {
			/* one vport exists in region */
			HFC_DBGPRT( "hfc_fx_release_adp() - one vport exist in region\n");
			
			/* region_arg update in physical port */
			HFC_DBGPRT( "hfc_fx_release_adp() - region_arg update in physical port\n");
			pp->pport->region_arg[pp->rid] = NULL;
			
			/* rid unregister */
			hfc_fx_rid_unregister(pp->pport, pp);
			
			/* region_arg update in virtual port*/
			for (i=1; i<=pp->pport->max_vport_count; i++) {
				wkpp = pp->pport->vport_ptr[i].vport_arg;
				if (wkpp == NULL)
					continue;
				
				for (j=0;j<MAX_REGION_PROBE;j++) {
					wkpp->region_arg[j] = pp->pport->region_arg[j];
				}
			}
		}
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	hfc_fx_detach(pp);
	
	HFC_EXIT("hfc_fx_release_adp");
	return (0);
}

static inline void hfc_fx_free_cpu_info(struct port_info *pp)
{
	int socket = 0;
	int socket_num = hfc_manage_info.socket_num;

	if(hfc_manage_info.socket_info == NULL)
	{
		HFC_DBGPRT("cpaldd : %s - socket_info is NULL \n", __FUNCTION__);
		return;
	}

	for(socket=0; socket < socket_num; socket++)
	{
		if(hfc_manage_info.socket_info[socket].cpu_into_list == NULL)
		{
			HFC_DBGPRT("hfcldd : %s - cpu_into_list is NULL \n", __FUNCTION__);
			continue;
		}
		hfc_fx_kfree(pp, hfc_manage_info.socket_info[socket].cpu_into_list);
		hfc_manage_info.socket_info[socket].cpu_into_list = NULL;
	}

	hfc_fx_kfree(pp, hfc_manage_info.socket_info);
	hfc_manage_info.socket_info = NULL;

	return;
}

int hfc_fx_release(struct Scsi_Host *host)
{
	ulong				flags = 0;
	struct port_info	*pp;
	struct port_info	*wkpp;
	struct region_info	*rp;
	int i, pci_fail=0;
	uint				read_reg = 0 ;

	HFC_ENTRY("hfc_fx_release");
	
	pp = (struct port_info *)host->hostdata;
	/* If pp is not configured or opened, return (abend) */
	if ( pp == NULL )
		return 0;

	rp = pp->region_arg[pp->rid];
	/* If rp is not configured or opened, return (abend) */
	if ( rp == NULL )
		return 0;
	
	/* Release adapter for virtual port */
	for (i=1; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		hfc_fx_release_adp(wkpp);
	}
	
	/* Release adapter for physical port */
	hfc_fx_release_adp(pp);
	
	read_reg = hfc_fx_read_reg_ext( pp,(uint)0, (char)0x4) ;
	if( read_reg == 0xffffffff )
	{
		pci_fail = -1;
	}
	
	/* Release interrupts */
	if( pp->msi_flag == HFC_INT_TYPE_MSI ||
	    pp->msi_flag == HFC_INT_TYPE_MSIX )
		hfc_fx_free_interrupts(pp, pp->msi_flag, HFC_FX_NVEC_PER_PORT, pci_fail);
	else if( pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI)
		hfc_fx_free_interrupts(pp, pp->msi_flag, HFC_FX_MSIX_MULTIQUEUE, pci_fail);
	else
		hfc_fx_free_interrupts(pp, pp->msi_flag, HFC_FX_NVEC_ONE, pci_fail);
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
		hfc_manage_info.npubp->hfc_fx_free_errcnt_info(pp);	/* FCLNX-GPL-FX-314 */
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

	pci_release_regions(pp->pci_cfginf);
	iounmap((void *)(pp->mem_base_addr));

	/* Update manage_info and port_info chain */
	spin_lock_irqsave(&hfc_manage_info.hfcmp_fx_lock, flags);
	
	if ( hfc_manage_info.mp_adap_cnt == 0) { /* Not any FPP/FIVE/FIVE-EX ports */
		if ( hfc_manage_info.port_cnt == 1) { 
			if(hfc_manage_info.hfcldd_mp_mod) {
				hfc_manage_info.npubp->hfc_free_mp_table(); /* Free FPP/FIVE/FIVE-EX/FIVE-FX common area. */
			}
		}
	}
	
	hfc_manage_info.port_cnt--;
	hfc_manage_info.port_info_arg[pp->dev_minor] = NULL;

	if (hfc_manage_info.pp == pp) {
		hfc_manage_info.pp = pp->next;
	}

	if (pp->prev != NULL)
		pp->prev->next = pp->next;
	if (pp->next != NULL)
		pp->next->prev = pp->prev;

	spin_unlock_irqrestore(&hfc_manage_info.hfcmp_fx_lock, flags);

	set_bit(HFC_CLOSED, (ulong *)&pp->open_status);

	if ( (hfc_manage_info.mp_adap_info == NULL) &&
		 (hfc_manage_info.pp == NULL)) {
		/* Free CPU info list and Socket_info *//* FCLNX-GPL-FX-420 */
		hfc_fx_free_cpu_info(pp);

#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )	/* FCLNX-GPL-FX-435 */
		unregister_chrdev(hfc_manage_info.major, "hfcldd");
#endif
		hfc_manage_info.instance = 0;
	}

	/* check alloc_cnt */
	HFC_DBGPRT("hfcldd%d: kmalloc_cnt_ap=%d\n",
			pp->instance, atomic_read(&hfc_manage_info.kmalloc_cnt_ap[pp->instance]));
	HFC_DBGPRT("hfcldd%d: dma_alloc_cnt_ap=%d\n",
			pp->instance, atomic_read(&hfc_manage_info.dma_alloc_cnt_ap[pp->instance]));

	HFC_EXIT("hfc_fx_release");	
	return (0);
}

int hfc_fx_start_adapter(struct port_info *pp)
{
	uint i;
	uchar	pkrev_format[17] = "ABCDEFGHJKLMNPQR",adap_id[16];					/* FCLNX-0274 */
	struct	hfc_vpd_five_fx	*vpdfx_ptr=NULL;
	unsigned long 			wk_buf;
	unsigned long long		wkp;
	uchar 					pkrev_data, buf[16], fail=0;
	int						dma_size;
	int 					err = 0;
	uchar 					addr[4];						/* FCLNX-GPL-319 */
	uint 					wk;								/* FCLNX-GPL-319 */
	uint					hyp_status;						/* FCLNX-GPL-393 */
	uint					hvm_support = 0;
	uint					hyp_core_status = 0;
	struct core_info		*core=NULL;
					 	
	HFC_ENTRY("hfc_fx_start_adapter");

//	/* Set fw_init_tbl */
//	HFC_DBGPRT( " set hw init table \n");
//	hfc_fx_set_fw_init_tbl(pp);

	/* Initialize adap_id */
	HFC_DBGPRT( " initialize adap_id \n");
																	/* @MLPF  */
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) )
		hfc_fx_mlpf_get_mmio_hg(pp, adap_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
	else {
		if (pp->pkg.code == 0x8f) {
			if( hfc_fx_read_flash(pp, 0x20018, 8, &adap_id[0]) ){ /* only FC-GW FCLNX-0405 */
				return(1); /* FCLNX-GPL-116 */
			}
			if( hfc_fx_read_flash(pp, 0x20028, 8, &adap_id[8]) ){ /* only FC-GW FCLNX-0405 */
				return(1); /* FCLNX-GPL-116 */
			}
		}
		else {
			if( hfc_fx_read_flash(pp, 0x54, 4, addr)){			/* FCLNX-GPL-319 */
				return (1);
			}
			HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
			if(hfc_fx_read_flash(pp, wk, 16, adap_id)){
				return(1);
			}
		}													/* FCLNX-GPL-319 */
	}																/* @MLPF  */
	memcpy(pp->adap_id, adap_id, sizeof(adap_id));


	/* Set sys_rev */
																/* @MLPF STR */
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) ) {
		uint wkint_sysrev1;
		uint wkint_sysrev2;

		wkint_sysrev1 = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_SYSREV0, 0x4 );
		HFC_4L_TO_4B(wkint_sysrev2,wkint_sysrev1);
		HFC_MEMCPY(buf,(uchar*)&wkint_sysrev2,4);
		
		HFC_4B_TO_4L(pp->sys_rev, (*(uint*)buf));
	}
	else {
		if( hfc_fx_read_flash(pp, 0, 4, buf) ){ /* FCLNX-GPL-116 */
			HFC_DBGPRT("hfcldd%d start_adapter_error_exit 3\n",pp->dev_minor);
			goto start_adapter_error_exit;
		}
		HFC_4B_TO_4L(pp->sys_rev, (*(uint*)buf));
	}
																/* @MLPF END */

	/* Set vpd information */
	err = hfc_fx_get_vpd( pp, pp->vpd_buf );
	if(err){
		HFC_DBGPRT("hfcldd%d start_adapter_error_exit 4\n",pp->dev_minor);
		set_bit( HFC_PS_ISOL, (ulong *)&pp->status );
		set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2);
		return(0);
	}
	if (HFC_FX_MMODE_CHECK_SHADOW(pp))
	{
		/* Shadow LPAR read VPD DATA from port_info and set it to MMIO-HG */
		hfc_fx_mlpf_set_mmio_hg(pp, pp->vpd_buf, HFC_IOHGSPC_VPDAREA, HFC_IOHGSPC_VPDAREA_LEN);
	}

	fail=0;
	if(pp->pkg.type == HFC_PKTYPE_FIVE_FX) /* FIVE-FX */
	{
		uchar parts_value[HFC_IOHGSPC_PARTSNUM_LEN];
		memset(parts_value, 0,HFC_IOHGSPC_PARTSNUM_LEN);
		/* Update vpd information */
		vpdfx_ptr = (struct hfc_vpd_five_fx *)pp->vpd_buf;
		strcpy(vpdfx_ptr->driver_ver, hfc_manage_info.package_ver);
		vpdfx_ptr->driver_len = (uchar) strlen(vpdfx_ptr->driver_ver);

		/* Read PK-REV (addr = 0x007) */
		pkrev_data = (uchar) hfc_fx_read_reg(pp, HFC_IOSPACE_PKREV, 0x1);
		vpdfx_ptr->ec_level = pkrev_format[pkrev_data];
		vpdfx_ptr->fw_ver = pp->sys_rev;
		
		/* We never need changing the format of PartsNumber(vpdfx_ptr->pn_value). */
		
		/* Get Model Name from VPD */ /* FCLNX-GPL-94 */
		strncpy(pp->model_name, vpdfx_ptr->v0_value, vpdfx_ptr->v0_len);
		
		if (HFC_FX_MMODE_CHECK_SHADOW(pp))
		{
			strncpy(parts_value, vpdfx_ptr->pn_value, HFC_IOHGSPC_PARTSNUM_LEN);
			hfc_fx_mlpf_set_mmio_hg(pp, parts_value, HFC_IOHGSPC_PARTSNUM0, HFC_IOHGSPC_PARTSNUM_LEN);
		}
		
		for (i=0 ; i < sizeof(pp->ecid)/4; i++ ) {
			wk = (uint)hfc_fx_read_reg_ext(pp, 0x8a0 + (i*4), 0x04);
			HFC_4L_TO_4B(pp->ecid[i], wk);
		}
		
		if(hfc_manage_info.instance == 0) {
//			HFC_DBGPRT(" mck_point = %d\n", pp->mck_point);
			if(pp->mck_point == HFC_BEFORE_POSTCHK) {        /* FCLNX-0533 */
//				HFC_DBGPRT("call hfc_fx_occurred_mck \n");
				hfc_fx_occurred_mck(pp,pp->mck_point);
			}       /* FCLNX-0533 */
		}
		                                                                            /* FCLNX-0370 */
		if( !HFC_FX_MMODE_CHECK_SHARED(pp) || 
		   ( HFC_FX_MMODE_CHECK_SHADOW(pp) && ! HFC_FX_MMODE_CHECK_REBOOT(pp) ) )
		{
			if ((hfc_fx_config_hw_set_five_fx(pp, HFC_FX_CONFIG_HW_CHECK_RETRY)) != 0)	/* FCLNX-GPL-FX-215 */
				fail=1;
		}                                                                           /* FCLNX-0370 */
		
		if ( HFC_FX_MMODE_CHECK_SHADOW(pp) && HFC_FX_MMODE_CHECK_REBOOT(pp) )             /* FCLNX-0386 */
		{
			
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					core = pp->region_arg[pp->rid]->core_arg[i];
					hyp_status = hfc_fx_read_hg_reg_core(pp, core->core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, HFC_FX_CORE_OFFSET40);	/* FCLNX-GPL-FX-387 */
					/* Read HYPER status before reset int_a_reg.     */
					if( hyp_status & ( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML | HFC_HG_HYPSTATUS_FMCK ) ){
						if(hyp_status & HFC_HG_HYPSTATUS_FCSTP)
							hfc_fx_mlpf_hwerr_int_detail(pp, core, hyp_status, HFC_HG_HYPINTDET_FCSTP);	/* FCLNX-GPL-FX-387 */
						else
							hfc_fx_mlpf_hwerr_int_detail(pp, core, hyp_status, HFC_HG_HYPINTDET_FMCK);	/* FCLNX-GPL-FX-387 */
					}
				}
			}
		}
		
		if ( HFC_FX_MMODE_CHECK_SHARED(pp) )                                           /* FCLNX-0384 */
		{
			hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);			/* FCLNX-GPL-393 */
			hvm_support = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
			
			if(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP)
				fail = 1;
			
			/* check core_status */
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					core = pp->region_arg[pp->rid]->core_arg[i];
					if( core != NULL ){
						switch( i ){
							case 0:
								hyp_core_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYP_STATUS0, 0x4);
								if( !(hyp_core_status & HFC_HG_HYPSTATUS_CORE_ENABLE) ){
									if(!(hyp_core_status & HFC_HG_HYPSTATUS_CORE_FDISABLE)
									|| (HFC_FX_MMODE_CHECK_SHADOW(pp) ))
										set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
								}else{
									set_bit(HFC_CS_CORE_ENABLE, (ulong *)&core->status);
								}	/* FCLNX-GPL-FX-438 */
								break;
							
							case 1:
								hyp_core_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYP_STATUS1, 0x4);
								if( !(hyp_core_status & HFC_HG_HYPSTATUS_CORE_ENABLE) ){
									if(!(hyp_core_status & HFC_HG_HYPSTATUS_CORE_FDISABLE)
									|| (HFC_FX_MMODE_CHECK_SHADOW(pp) ))
										set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
								}else{
									set_bit(HFC_CS_CORE_ENABLE, (ulong *)&core->status);
								}	/* FCLNX-GPL-FX-438 */
								break;
							
							case 2:
								hyp_core_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYP_STATUS2, 0x4);
								if( !(hyp_core_status & HFC_HG_HYPSTATUS_CORE_ENABLE) ){
									if(!(hyp_core_status & HFC_HG_HYPSTATUS_CORE_FDISABLE)
									|| (HFC_FX_MMODE_CHECK_SHADOW(pp) ))
										set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
								}else{
									set_bit(HFC_CS_CORE_ENABLE, (ulong *)&core->status);
								}	/* FCLNX-GPL-FX-438 */
								break;
							
							case 3:
								hyp_core_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYP_STATUS3, 0x4);
								if( !(hyp_core_status & HFC_HG_HYPSTATUS_CORE_ENABLE) ){
									if(!(hyp_core_status & HFC_HG_HYPSTATUS_CORE_FDISABLE)
									|| (HFC_FX_MMODE_CHECK_SHADOW(pp) ))
										set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
								}else{
									set_bit(HFC_CS_CORE_ENABLE, (ulong *)&core->status);
								}	/* FCLNX-GPL-FX-438 */
								break;
						}
					}
				}
			}
		}
		vpdfx_ptr->ww_name = pp->ww_name; /* FCLNX-0299 */
	}
	else {
		HFC_DBGPRT( "Findadapter() - Invalid Package Type.");
		fail=1;
	}

	if (!fail) {
		dma_size = sizeof(dma_addr_t);
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					wkp = pp->region_arg[pp->rid]->core_arg[i]->padr_init;
					if(dma_size == 8){
						wkp >>=32;
					}
					else{
						wkp = 0;
					}
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR0,
						0x4, wkp, HFC_FX_CORE_OFFSET80);
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR1,
						0x4, pp->region_arg[pp->rid]->core_arg[i]->padr_init, HFC_FX_CORE_OFFSET80);
					
					wk_buf = (uint) hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR0,
						0x04, HFC_FX_CORE_OFFSET80);
					HFC_DBGPRT( "hfcldd%d init addr = %lx",pp->dev_minor, wk_buf);
					wk_buf = (uint) hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_CA_INIT_ADDR1,
						0x04, HFC_FX_CORE_OFFSET80);
					HFC_DBGPRT( "** %lx \n",wk_buf);
					
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_FLAG,
						0x1, 0x00, HFC_FX_CORE_OFFSET80);
					
					if ( HFC_FX_MMODE_CHECK_SHADOW(pp) && HFC_FX_MMODE_CHECK_REBOOT(pp) )
					{
						set_bit( HFC_PD_NEED_SHADOW_UP, (ulong *)&pp->status_detail2 );
					}
				}
			}
		}
	}else{
		set_bit( HFC_PS_ISOL, (ulong *)&pp->status );
		set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2);
	}
	
	if (  ( !hfc_manage_info.hfcldd_mp_mod ) || /* FCLNX-GPL-349 */
		(( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) &&  ( pp->isol_force == HFC_NO_FRC_ISOL )) ){
	    
		if(!HFC_FX_MMODE_CHECK_SHADOW(pp) || !HFC_FX_MMODE_CHECK_REBOOT(pp))	/* FCLNX-GPL-FX-387 */
			set_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
	}
	
	if (fail) {
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_INTA_MSK,
									0x4, 0x00000000, HFC_FX_CORE_OFFSET10);
				}
			}
		}
		
		if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
		{
			hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD0, NULL, 0) ;/* FCLNX-GPL-161 */
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){	/* FCLNX-GPL-FX-376 */
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						hfc_fx_mlpf_change_state_core(pp, i, HFC_HG_HYPSTATUS_CORE_ENABLE, HFC_DISABLE_HYPER_STATE );
						if((i + MAX_CORE_PROBE_FX/pp->core_num) < MAX_CORE_PROBE_FX)
							hfc_fx_write_hg_reg_core(pp, i, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_CORE_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 *//* FCLNX-GPL-FX-405 */
						else
							hfc_fx_write_hg_reg_core(pp, i, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_PORT_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 *//* FCLNX-GPL-FX-405 */
					}
				}
			}	/* FCLNX-GPL-FX-376 */
		}
	}
	else {
//		set_bit(HFC_ATTACH, (ulong *)&pp->attach_status);
	}
	
	set_bit(HFC_ATTACH, (ulong *)&pp->attach_status);	/* FCLNX-GPL-FX-414 */
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_INTA_MSK,
							0x4, 0x00000000, HFC_FX_CORE_OFFSET10);
			}
		}
	}
	
	if ( ( HFC_FX_MMODE_CHECK_SHARED(pp) )&&(test_bit(HFC_ISOL, (ulong *)&pp->status ))){	/* FCLNX-GPL-FX-422 */
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_INTA_MSK,
									0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}																					/* FCLNX-GPL-489*/
	
	/* PCI error status reset */
	hfc_fx_clear_sticky_bit(pp);

	HFC_EXIT("hfc_fx_start_adapter");

    return (0); 

start_adapter_error_exit:

	return (1); 

}


int hfc_fx_core_start(struct port_info *pp, int immdt_cmd)
{
	unsigned long		flags = 0;
	uint				i=0;
	struct region_info	*rp=NULL;
	uint	mask;

	HFC_ENTRY("hfc_fx_core_start");
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if ( !test_bit(HFC_PS_ENABLE, (ulong *)&pp->status) ) { /* HBA initialization has not finished? */
		HFC_DBGPRT( "hfcldd : hfc_fx_core_start- HFC_PS_ENABLE=0.");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}

#if 0
//	if ( !test_bit(HFC_NEED_CORE_START, (ulong *)&pp->status1) ) { /* Need link initialization? */
//		HFC_DBGPRT( "hfcldd : hfc_fx_core_start - HFC_NEED_CORE_START=0.\n");
//		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
//		return TRUE;
//	}
#endif

	/* Start target detection */
	HFC_DBGPRT(" hfcldd : hfc_fx_core_start - start target detection adap_status = %lx\n",(ulong)pp->status); 

//	if( pp->region_arg[pp->rid] != NULL){
//		core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
//	}
	
//	if( core == NULL ){
//		HFC_DBGPRT( "hfcldd : hfc_fx_core_start - Core == NULL.\n");
//		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
//		return TRUE;
//	}

	rp = pp->region_arg[pp->rid];
	if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : hfc_fx_initialize - region_info==NULL.\n");
		clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}

	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKUP_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_MCK_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKINIT_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */

	set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);	/* FCLNX-GPL-FX-005 */
	hfc_fx_w_start( pp, NULL, HFC_FX_LINKINIT_TMR, pp->link_initialize_tmo);
	
	pp->initialize = 1;
	
	HFC_ALLCORELOCK(rp);
	
	/* Execute Core_Start for all core's and set Need flag */
	if ( hfc_fx_all_core_start(pp) ) {
		HFC_DBGPRT(" hfcldd : hfc_fx_core_start - issue core_start fail.\n");
		clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		pp->initialize = 0;
		HFC_ALLCOREUNLOCK(rp);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}
	
	if(hfc_manage_info.instance == 1) {
		if(pp->mck_point == HFC_BEFORE_LINKINITIALIZE) {	/* FCLNX-0533 */
			hfc_fx_occurred_mck(pp,pp->mck_point);
		}													/* FCLNX-0533 */
	}
	
	/* Start core_start mailbox for all the core */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				if(hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i]))
					continue;	/* FCLNX-GPL-FX-438 */
				
				/* clear free_xrb_area */
				if (HFC_FX_PHYSICAL_PORT(pp)) {
					hfc_fx_write_reg_ext(pp,
						(uint)0x300 + 0x80*pp->region_arg[pp->rid]->core_arg[i]->core_no + 0x2 ,( char )0x02, (ushort)0 );		
					HFC_DBGPRT(" hfcldd : hfc_fx_core_start - free_xrb_addr=0x%08x\n",
						(uint)0x300 + 0x80*pp->region_arg[pp->rid]->core_arg[i]->core_no + 0x2);
				}
				else {
					hfc_fx_write_reg_ext(pp,
						(uint)0x1000 + 0x80*pp->rid + 0x20*pp->region_arg[pp->rid]->core_arg[i]->core_no + 0x2 ,( char )0x02, (ushort)0 );
					HFC_DBGPRT(" hfcldd : hfc_fx_core_start - free_xrb_addr=0x%08x\n",
						(uint)0x1000 + 0x80*pp->rid + 0x20*pp->region_arg[pp->rid]->core_arg[i]->core_no + 0x2);
				}
				
				/* Enable interrupt */
				HFC_DBGPRT( " hfcldd : hfc_fx_core_start - enable interrupt\n");
				
				if (HFC_FX_MMODE_CHECK_SHARED(pp)){
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_INTA_MSK,
								0x4, ( int )hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);
				}
				else{
					mask = ( int )hfc_inta_mask[pp->pkg.type];
					HFC_DBGPRT("hfcldd%d mask = %08x\n",pp->dev_minor, mask);
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_INTA_MSK,
								0x4, ( int )hfc_inta_mask[pp->pkg.type], HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}

	HFC_DBGPRT(" hfcldd : hfc_fx_core_start - interruptible sleep on (mailbox event) \n");

	HFC_ALLCOREUNLOCK(rp);
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	if(!immdt_cmd){ /* FCLNX-0514 */
		hfc_fx_sleep_on(&pp->init_event, &pp->int_a_poll );								/* FCLNX-0269 */
	}
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	atomic_set(&pp->int_a_poll, 0);
	pp->initialize = 0;
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	
	HFC_EXIT("hfc_fx_core_start");
	
	return TRUE;
	
}


int hfc_fx_initialize(struct port_info *pp, int immdt_cmd)
{
	unsigned long		flags = 0;
	struct region_info	*rp=NULL;
	struct core_info	*core=NULL;

	HFC_ENTRY("hfc_fx_initialize");
	HFC_DBGPRT( "hfcldd : hfc_fx_initialize- start");
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if ( !test_bit(HFC_PS_ENABLE, (ulong *)&pp->status) ) { /* HBA initialization has not finished? */
		HFC_DBGPRT( "hfcldd : hfc_fx_initialize- HFC_PS_ENABLE=0.");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}

	if ( !test_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1) ) { /* Need link initialization? */
		HFC_DBGPRT( "hfcldd : hfc_fx_initialize - HFC_NEED_LINK_INIT=0.\n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}
	
	rp = pp->region_arg[pp->rid];
	if ( rp == NULL ) { /* region_info null */
		HFC_DBGPRT( "hfcldd : hfc_fx_initialize - region_info==NULL.\n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}
	
	core = rp->core_arg[ pp->master_core_no ];
	if( core == NULL ){
		HFC_DBGPRT( "hfcldd : hfc_fx_initialize - core_info==NULL.\n");
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}
	
	HFC_CORELOCK(core);
	
	/* Start target detection */
	HFC_DBGPRT(" hfcldd : hfc_fx_initialize - start target detection adap_status = %lx\n",(ulong)pp->status); 

//	clear_bit(HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1);
	pp->initialize = 1;
	atomic_set(&pp->int_a_poll, 0);

	if ( hfc_fx_issue_linkini(pp) ) {
		HFC_DBGPRT(" hfcldd : hfc_fx_initialize - issue link initialize fail.\n");
		HFC_COREUNLOCK(core);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return TRUE;
	}
	
	HFC_COREUNLOCK(core);
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	if(hfc_manage_info.instance == 1) {
		if(pp->mck_point == HFC_BEFORE_LINKINITIALIZE) {	/* FCLNX-0533 */
			hfc_fx_occurred_mck(pp,pp->mck_point);
		}													/* FCLNX-0533 */
	}

	HFC_DBGPRT(" hfcldd : hfc_fx_initialize - interruptible sleep on (mailbox event) \n");

	if(!immdt_cmd){ /* FCLNX-0514 */
		hfc_fx_sleep_on(&pp->init_event, &pp->int_a_poll );								/* FCLNX-0269 */
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		int count=300000;				/* 5min (max) */	/* FCLNX-GPL-466 */

		while (count) {
			if ( !HFC_SEMAPHORE_LOCK(pp->sem) ) {
				hfc_manage_info.npubp->hfc_fx_mp_scan_dev(pp);
				HFC_SEMAPHORE_UNLOCK(pp->sem) ;
				break;
			}

			msleep(1);
			count--;
		}

		if (!count) {
			HFC_DBGPRT("hfcldd%d : HFC_SEMAPHORE_LOCK is invalid.\n", pp->dev_minor);
		}													/* FCLNX-GPL-466 */
	}

	if(hfc_manage_info.instance == 1) {
		if(pp->mck_point == HFC_AFTER_LINKINITIALIZE) {		/* FCLNX-0533 */
			hfc_fx_occurred_mck(pp,pp->mck_point);
		}													/* FCLNX-0533 */
	}

	HFC_PORTLOCK_IRQSAVE(pp,flags);
	atomic_set(&pp->int_a_poll, 0);							/* FCLNX_0029 */
	pp->initialize = 0;		

	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	HFC_EXIT("hfc_fx_initialize");
	HFC_DBGPRT( "hfcldd : hfc_fx_initialize- end");
	
	return TRUE;
	
}


void hfc_fx_wwnverify_linkup (struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
		uint mb_resp_status, uint64_t ww_name )
{
	int i,j;
	int login_req = 0;
	uchar count;
	uchar hit;
	uint port_id=0;
				
	HFC_ENTRY("hfc_fx_wwnverify_linkup");

	if (mb_resp_status) {
		/* Link initialization has failed */
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		return;
	}

	for(i=0 ; i<MAX_TARGET_PROBE ; i++)	
	{
		target = hfc_fx_hash_target_valid(pp, i);
		if (target != NULL)
		{
			set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
			atomic_set(&pp->check_mbreq, 1);

			if ( (pp->connect_type == HFC_FX_SWITCH ) 
				|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {
//				set_bit(HFC_T_NEED_GIDPN, (ulong *)&target->status);
//				pp->next_gidpn = TRUE;
			}
			else {
				set_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
				if (HFC_FX_MQ_VALID(pp))
					hfc_fx_mq_change_target_info(pp, target);
				hfc_fx_enque_plogi_req(pp, target);	
			}
		}
	}

	set_bit(HFC_PD_LOGIN_DELAYI, (ulong *)&pp->status_detail2 );
	hfc_fx_watchdog_enter( pp, core, NULL, NULL, 0, HFC_FX_LOGIN_DELAY_TMR, 0, 1);
	hfc_fx_watchdog_enter( pp, core, NULL, NULL, 0, HFC_FX_LOGIN_DELAY_TMR, pp->login_wait, 0 );

	switch (pp->connect_type) {
		case HFC_FX_SWITCH :	/* FC-SW */
			HFC_DBGPRT("wwnverify_linkup() - Connect FC-SW.");
			set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);		/* request GPN_FT */
			atomic_set(&pp->check_mbreq, 1);
			
			break;

		case HFC_FX_AL :	/* FC-AL (without SW) */
		case HFC_FX_MULTI_ALPA :

			if (pp->scsi_id & 0x00ffff00) {
				HFC_DBGPRT("wwnverify_linkup() - Connect FC-AL (FC-SW).");
				set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);	/* request GPN_FT */
				atomic_set(&pp->check_mbreq, 1);
				break;
			}

			HFC_DBGPRT("wwnverify_linkup() - Connect FC-AL(Non SW).");
			/* Create the list of SCSI_ID (HFC_TF_WWN_VALID = 0 or 1) */
			login_req = FALSE;
			if ( (hfc_fx_read_val( core->fw_init_p->fw_iocinfo.configure_flag ) & HFC_FX_POSMAP_VALID )
			  && (!(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.configure_flag ) & HFC_FX_POSMAP_LISA)) ) {
				HFC_DBGPRT("wwnverify_linkup() - ***\n");
				/* Set position_map valid */
				count = (int) hfc_fx_read_val(core->fw_init_p->pos_map[0]);
				for (i=0;i<count; i++) {
					hit=0;
					if (pp->scsi_id != core->fw_init_p->pos_map[i+1]) {
						for (j=0;j<(int) pp->max_target;j++) {
							target = pp->target_arg[j];
							if((target != NULL) && (core->fw_init_p->pos_map[i+1] == target->scsi_id)){
								/* Target_info has already exsist */
								if(( ww_name == 0) || ( target->ww_name == ww_name )){
									hit=1;
									break; 
								}
							}
						}
					}
					if (!hit) {
						HFC_DBGPRT("wwnverify_linkup() - target did not hit \n");
						target = hfc_fx_add_target_info_fx( pp, core->fw_init_p->pos_map[i+1] );
						if(target != NULL){
						/* request LOGIN */
							HFC_DBGPRT("wwnverify_linkup() - target existed \n");
							set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
							atomic_set(&pp->check_mbreq, 1);
							login_req = TRUE;
						}
					}
					else if( !test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						atomic_set(&pp->check_mbreq, 1);
						login_req = TRUE;
					}
				}
			}
			else {
				HFC_DBGPRT("wwnverify_linkup() - @@@\n");
				/* Register all AL_PN number */
				for (i=0;i<127;i++) {
					hit=0;
					if (pp->scsi_id != posmap_lisa_fx[i+1]) {
						for (j=0;j<(int) pp->max_target;j++) {
							target = pp->target_arg[j];
							if((target != NULL) && (posmap_lisa_fx[i+1] == target->scsi_id)){
								/* Target_info has already exsist */
								if(( ww_name == 0) || ( target->ww_name == ww_name )){
									hit=1;
									break; 
								}
							}
						}
					}
					if (!hit) {
						HFC_DBGPRT("wwnverify_linkup() - target did not hit \n");
						target = hfc_fx_add_target_info_fx( pp, posmap_lisa_fx[i+1] );
						if(target != NULL){
						/* request LOGIN */
							HFC_DBGPRT("wwnverify_linkup() - target existed \n");
							set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
							atomic_set(&pp->check_mbreq, 1);
							login_req = TRUE;
						}
					}
					else if( !test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						atomic_set(&pp->check_mbreq, 1);
						login_req = TRUE;
					}
				}
			}
			if(login_req == FALSE){
				if(pp->initialize != 0){
					hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
				}
				HFC_DBGPRT(" pp->initialize = %d\n", pp->initialize);
			}
			break;

		case HFC_FX_PT2PT :							/* P2P */
			HFC_DBGPRT("wwnverify_linkup() - Connect P2P.");

			if (pp->target_arg[0] == NULL) { /* FCLNX-GPL-FX-237 */
				port_id = (uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0]) << 16) +
					(uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1]) << 8) +
					(uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2]));

				/* Create target_info_fx */
				target = hfc_fx_add_target_info_fx(pp, port_id);

				if (target != NULL) {	/* FCLNX-GPL-FX-446 >>> */
					set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
					atomic_set(&pp->check_mbreq, 1);
					hfc_fx_enque_plogi_req(pp,target);
				}						/* FCLNX-GPL-FX-446 <<< */
			}
			else {
				target = pp->target_arg[0];
			}

			break;

		case HFC_FX_F_PORT :							/* F_PORT */
			HFC_DBGPRT("wwnverify_linkup() - Connect F_PORT.");

			if (pp->target_arg[0] == NULL) { /* FCLNX-GPL-FX-237 */
				port_id = (uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0]) << 16) +
					(uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1]) << 8) +
					(uint)(hfc_fx_read_val( core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2]));

				/* Create target_info_fx */
				target = hfc_fx_add_target_info_fx(pp, port_id);
				if( target != NULL ){
					HFC_DBGPRT("wwnverify_linkup() - Connect F_PORT. Set WWNN/WWPN");
					target->ww_name = pp->flogi_ww_name;
					target->node_name = pp->flogi_node_name;
				}

				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
//				hfc_fx_enque_plogi_req(pp,target);
			}
			else {
				target = pp->target_arg[0];
				set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
			}

			break;
			
		default :
			HFC_DBGPRT( "wwnverify_linkup() - Invalid connect_type=%d.",pp->connect_type);
	}

	HFC_EXIT("hfc_fx_wwnverify_linkup");

}


void hfc_fx_wwnverify_linkup_timeout(struct port_info *pp, struct target_info_fx *target, uint mb_resp_status)
{
	int i;

	HFC_ENTRY("hfc_fx_wwnverify_linkup_timeout");

	if ( target != NULL ) {
		HFC_DBGPRT(  "wwnverify_linkup_timeout() - delete target (pseq#=%d).",target->pseq);

		/* Release specified target_info_fx */
		hfc_fx_clear_target_info_fx( pp, target, TRUE );
	}
	else {
		HFC_DBGPRT(  "wwnverify_linkup_timeout() - delete all target.");

		/* Release all target_info_fx */
		for (i=0;i<(int)pp->max_target;i++) {
			target = pp->target_arg[i];
			if(target != NULL) {
				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
		}
	}

	HFC_EXIT("hfc_fx_wwnverify_linkup_timeout");

}


void hfc_fx_wwnverify_gidft(struct port_info *pp, struct target_info_fx *target, struct core_info *core, uint mb_resp_status)
{
	uint	scsi_id,i,port_num=0;
	int		no_target=1;
	int 	detect_own_scsi_id;
	ushort	portnum_by_rcvlen=0;
	uchar	ctl=0;
	struct payload_fx	*pyload = NULL;
	struct mailbox_fx	*mbox = NULL ;

	HFC_ENTRY("hfc_fx_wwnverify_gidft");
	
	HFC_DBGPRT( "wwnverify_gidft() - start\n");

	/* Clear target_can */
	memset(pp->target_scan,0,(sizeof(struct target_scan)*MAX_TARGET_PROBE));
	
	pyload = core->payload;
	mbox = core->mb;

	if (mb_resp_status) {
		/* GID_FT has failed. Impossible to detect target */
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		HFC_DBGPRT( "wwnverify_gidft() - error end<mb_resp_status=0x%x>.",mb_resp_status);
		return;
	}
	
	portnum_by_rcvlen = (ushort)hfc_fx_read_val( mbox->mb_resp.type.frmsndrcv.recv_payload_length) ;
	portnum_by_rcvlen -= 0x10;
	portnum_by_rcvlen /= 4;

	/* Count device number */
	for(i=0 ; i < HFC_FX_MAX_PORTID ; i++){
		ctl = hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gid_ft.portid[i].ctl);
		port_num++;
		if ( ctl & 0x80 ) {
			/*---- Detect last Device on PortID Field of GID_FT Response -----*/
			break;
		}
		if (port_num >= portnum_by_rcvlen) {
			break;
		}
	}
	
	HFC_DBGPRT( "wwnverify_gidft() - port_num = %d\n",port_num);

	if (!port_num) {
		/* No target for this FC-SW */
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		HFC_DBGPRT( "fx_wwnverify_gidft() - non target");
		return;
	}

	if ( port_num > MAX_TARGET_PROBE ) port_num = MAX_TARGET_PROBE;

	HFC_DBGPRT(" port_num = %d\n",port_num);

	detect_own_scsi_id = 0;
	for (i=0;i<port_num;i++) {
		scsi_id = 0;
		scsi_id = (uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gid_ft.portid[i].port_id[0]) << 16 ) +
			(uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gid_ft.portid[i].port_id[1]) << 8 ) +
			(uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gid_ft.portid[i].port_id[2]) );
		
		if (pp->filter_target == HFC_FX_MB_LOGIN_FILTER_ON) { /* FCLNX-GPL-491 Filtering Login Target */
			if ((pp->scsi_id & 0x00ffff00) == (scsi_id & 0x00ffff00)) { /* domain,area */
				/* AccessGateway switch same physical port */
				continue; 
			}
		}
		
		HFC_DBGPRT("wwnverify_gidft port_id[%d] = %08x\n",i, scsi_id);
		
		if(pp->scsi_id != scsi_id) {
			pp->target_scan[i].flags  |= HFC_SCAN_SCV | HFC_SCAN_NEED;	
			pp->target_scan[i].scsi_id = scsi_id;
			set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
			atomic_set(&pp->check_mbreq, 1);
			no_target=0;
		}
		else{
			detect_own_scsi_id = 1;
		}

		if ( i == (MAX_TARGET_PROBE - 2) ){
			if ( detect_own_scsi_id == 0 )
				break;
		}

	}

	if(no_target){
		HFC_DBGPRT(" target is not found\n");
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}


	HFC_EXIT("hfc_fx_wwnverify_gidft");

}


void hfc_fx_wwnverify_gpnid(struct port_info *pp, struct target_info_fx *target, struct core_info *core, uint mb_resp_status)
{

	struct target_scan *tscan,*next_scan=NULL;
	int hit=0,i,j;
	int no_target=0;

	HFC_DBGPRT("hfc_fx_wwnverify_gpnid");

	HFC_DBGPRT("gpnid ** mb_resp_status = %x\n",mb_resp_status);

	for (i=0;i<MAX_TARGET_PROBE;i++) {
		tscan = &pp->target_scan[i];

		if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_WAIT))
							 == (HFC_SCAN_SCV | HFC_SCAN_WAIT) ) {

			HFC_DBGPRT("gpnid ** tscan->flags = %x\n", tscan->flags);
			
			if ( hfc_fx_read_val( core -> payload->send_payload.type.gxx.sub_type.gpn_id.port_id )
					== tscan->scsi_id) {
				tscan->flags &= ~(HFC_SCAN_NEED | HFC_SCAN_WAIT);
				tscan->flags |= HFC_SCAN_COMP;
				
				if (!mb_resp_status) {		

					tscan->wwpn = hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gpn_id.port_name ) ;
					HFC_DBGPRT("gpnid ** tscan->wwpn = %llx\n", (unsigned long long)tscan->wwpn);
				}
				else {
					/* Target is invalid */
					tscan->flags |= HFC_SCAN_FAIL;
					tscan->flags &= ~HFC_SCAN_SCV;
					HFC_DBGPRT("gpnid ** HFC_SCAN_FAIL\n");
				}
				hit++;
			}
		}

		if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_NEED))
				== (HFC_SCAN_SCV | HFC_SCAN_NEED)) {
			/* Store pointer of target_scan which will issue GPN_ID next */
			next_scan = tscan;
//			set_bit(HFC_PD_NEED_GPNID, (ulong *)&pp->status_detail1);
		}
	}

	HFC_DBGPRT(" hit = %d",hit);

	if ( !next_scan ) {	/* Find target to issue GPN_ID next */
		HFC_DBGPRT("next_scan finished \n");
		no_target=1;

		for (i=0;i<MAX_TARGET_PROBE;i++) {
			tscan = &pp->target_scan[i];

			if ( (tscan->flags & (HFC_SCAN_SCV | HFC_SCAN_COMP | HFC_SCAN_FAIL))
							  == (HFC_SCAN_SCV | HFC_SCAN_COMP)) {
				
				int new=TRUE;

				for (j=0;j<(int)(pp->max_target);j++) {
					target = pp->target_arg[j];

					if(target != NULL){
						/* If target has already exists, no need to make new target. */
						if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) 
							&& ( target->ww_name == tscan->wwpn ) ) {
						
							tscan->flags &= ~HFC_SCAN_SCV;
							new = FALSE;
							HFC_DBGPRT("target hit @@ \n");

							if (target->scsi_id != tscan->scsi_id)
							{
//								set_bit(HFC_T_NEED_GIDPN, (ulong *)&target->status);
//								pp -> next_gidpn = TRUE;
								no_target = 0;
							}
							else if(!test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags)){
								set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
								atomic_set(&pp->check_mbreq, 1);
								clear_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
								
								clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//								if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
//									clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
								hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);
								hfc_fx_enque_plogi_req(pp,target);
								no_target = 0; 
							}
							
							break;
						}

						/* If this target is in creating process, no need to create new target */
						if (  test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
							tscan->flags &= ~HFC_SCAN_SCV;
							new = FALSE;
							no_target = 0;
							break;
						}	
					}
				}

				if ( new == TRUE ) {/* Make new target if there is no target with the same WWN */
					
					HFC_DBGPRT("add target scsi_id=%d wwpn= %llx\n", tscan->scsi_id, (unsigned long long)tscan->wwpn);

					if ( (target = hfc_fx_add_target_info_fx(pp, tscan->scsi_id)) != NULL ) {
						HFC_DBGPRT("set HFC_NEED_PLOGI_T\n");
						set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
						atomic_set(&pp->check_mbreq, 1);
						hfc_fx_enque_plogi_req(pp,target);
						no_target = 0;
						
					} 
				}

			}
		}
	}

	HFC_DBGPRT("no_target = %d\n",no_target);

	if(no_target){
		HFC_DBGPRT(" verify_gpnid no_target; target is not found\n");
		if(pp->initialize != 0){
			HFC_DBGPRT("wake up detect forcefully\n");
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}

	HFC_DBGPRT("hfc_fx_wwnverify_gpnid");

}


void hfc_fx_wwnverify_gpnft(struct port_info *pp, struct target_info_fx *target, struct core_info *core, uint mb_resp_status)
{
	uint	scsi_id,i,j,port_num=0, div=0, rem=0, target_not_found=0, mb_code=0;	/* FCLNX-GPL-FX-139 */
	int		no_target=1, find_target=FALSE;
	int 	detect_own_scsi_id;
	ushort	portnum_by_rcvlen=0;
	uchar	ctl=0;
	unsigned long long wwpn=0;
	struct payload_fx	*pyload = NULL;
	struct mailbox_fx	*mbox = NULL ;
	struct target_info_fx	*tmp_target=NULL;							/* FCLNX-GPL-FX-139 */
	int		detect_tgt_scsi_id = 0; 									/* FCLNX-GPL-FX-154 */

	HFC_ENTRY("hfc_fx_wwnverify_gpnft");
	
	HFC_DBGPRT( "wwnverify_gpnft() - start\n");

	pyload = core->payload;
	mbox = core->mb;

	if (mb_resp_status) {
		/* GPN_FT has failed. Impossible to detect target */
		for (j=0;j<(int)(pp->max_target);j++) {	/* FCLNX-GPL-FX-087 */
			target = hfc_fx_hash_target_info(pp, j);
			if(target != NULL){
				set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);								/* FCLNX-GPL-fX-096 */
				clear_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);								/* FCLNX-GPL-fX-096 */
				clear_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);								/* FCLNX-GPL-FX-112 */
				clear_bit(HFC_TS_CANCEL_SCSI_TARGET,(ulong *)&target->status);						/* FCLNX-GPL-FX-112 */
				set_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);								/* FCLNX-GPL-fX-096 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
					set_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
				hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);	/* FCLNX-GPL-fX-096 */
				hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, FALSE);	/* FCLNX-GPL-fX-096 */
				div = target->target_id / 8;														/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
				rem = target->target_id % 8;														/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
				set_bit(7-rem, (ulong*)&pp->unfnd_tgtlist[div]);									/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
				target_not_found=1;																	/* FCLNX-GPL-fX-139 */
				tmp_target = target;
			}
		}	/* FCLNX-GPL-FX-087 */
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		if(target_not_found){																		/* FCLNX-GPL-fX-139 Start */
			memset((void *)core->logdata, 0, 16);
			memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code,4);
			HFC_4B_TO_4L(mb_code, mbox->mb_resp.mb_code);
			memcpy(&core->logdata[2],(char *)&core->payload->send_payload.data0[0],2);
			core->logdata[5] = mbox->mb_resp.esw ;
			core->logdata[6] = mbox->mb_resp.ssn ;
			core->logdata[7] = mb_resp_status ;
			memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
			core->logdata[12] = mbox->mb_resp.fsb ;
			memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);
			hfc_fx_errlog(pp, core, tmp_target, NULL, HFC_ERRLOG_TYPE_TGT_NOTFOUND, ERRID_HFCP_ERR6, 0x83, core->logdata, 16);
			for (j=0;j<32;j++) pp->unfnd_tgtlist[j]=0x00;
		}																							/* FCLNX-GPL-fX-139 End */
		HFC_DBGPRT( "wwnverify_gpnft() - error end<mb_resp_status=0x%x>.",mb_resp_status);
		return;
	}
	
	portnum_by_rcvlen = (ushort)hfc_fx_read_val( mbox->mb_resp.type.frmsndrcv.recv_payload_length) ;
	portnum_by_rcvlen -= 0x10;
	portnum_by_rcvlen /= 16;
	
	/* Count device number */
	for(i=0 ; i < HFC_FX_MAX_PORTID ; i++){
		ctl = hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].ctl);
		port_num++;
		if ( ctl & 0x80 ) {
			/*---- Detect last Device on PortID Field of GPN_FT Response -----*/
			break;
		}
		if (port_num >= portnum_by_rcvlen) {
			HFC_DBGPRT( "wwnverify_gpnft() - rcv_payload_len=%d.\n",portnum_by_rcvlen);
			break;
		}
	}
	
	/* Serch for target in fablic */
	for (j=0;j<(int)(pp->max_target);j++) {
		target = hfc_fx_hash_target_info(pp, j);	/* FCLNX-GPL-FX-087 */
		find_target = FALSE;
		if(target != NULL){
			clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);	/* FCLNX-GPL-FX-128 */
			for(i=0 ; i < HFC_FX_MAX_PORTID ; i++){
				wwpn = hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].port_name ) ;
				if(wwpn == target->ww_name){
					find_target = TRUE;
					break;
				}
			}
			if(find_target != TRUE){
				/* ABEND -> Was the device deleted?						*/
				/* Start Link Up waiting Timer between SW and device 	*/
				if (!test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status)) {
					set_bit(HFC_TS_NEED_CANCEL_SCSI_WAIT_DMA, (ulong *)&target->status);	/* FCLNX-GPL-FX-014 */
					atomic_set(&pp->check_mbreq, 1);
				
					clear_bit(HFC_TS_SCN_RESP, (ulong *)&target->status);					/* FCLNX-GPL-fX-096 */
					clear_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
					clear_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);					/* FCLNX-GPL-FX-112 */
					clear_bit(HFC_TS_CANCEL_SCSI_TARGET,(ulong *)&target->status);			/* FCLNX-GPL-FX-112 */
				
					set_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);
					hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, FALSE);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
					if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
						set_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
					div = target->target_id / 8;														/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
					rem = target->target_id % 8;														/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
					set_bit(7-rem, (ulong*)&pp->unfnd_tgtlist[div]);									/* FCLNX-GPL-fX-139 *//* FCLNX-GPL-FX-153 */
					target_not_found=1;																	/* FCLNX-GPL-fX-139 */
					tmp_target = target;																/* FCLNX-GPL-fX-139 */
				}
			}
		}
	}
	if(target_not_found){																	/* FCLNX-GPL-fX-139 Start */
		memset((void *)core->logdata, 0, 16);
		memcpy(&core->logdata[0],(char *)&mbox->mb_resp.mb_code,4);
		HFC_4B_TO_4L(mb_code, mbox->mb_resp.mb_code);
		memcpy(&core->logdata[2],(char *)&core->payload->send_payload.data0[0],2);
		core->logdata[5] = mbox->mb_resp.esw ;
		core->logdata[6] = mbox->mb_resp.ssn ;
		core->logdata[7] = mb_resp_status ;
		memcpy(&core->logdata[8],(char *)&mbox->mb_resp.sbc, 2) ;
		core->logdata[12] = mbox->mb_resp.fsb ;
		memcpy(&core->logdata[13],(char *)&mbox->mb_resp.err_code,3);
		hfc_fx_errlog(pp, core, tmp_target, NULL, HFC_ERRLOG_TYPE_TGT_NOTFOUND, ERRID_HFCP_ERR6, 0x83, core->logdata, 16);
		for (j=0;j<32;j++) pp->unfnd_tgtlist[j]=0x00;
	}																						/* FCLNX-GPL-fX-139 End */
	HFC_DBGPRT( "wwnverify_gpnft() - port_num = %d\n",port_num);

//	HFC_DBGPRT("hfc_fx_wwnverify_gpnft mailbox dump\n");
//	structdump( 0xec, (uchar *)mbox, sizeof(struct mailbox_fx) );
	
//	HFC_DBGPRT("hfc_fx_wwnverify_gpnft mailbox payload dump\n");
//	structdump( 0xee, (uchar *)core->payload, sizeof(struct payload_fx) );
	
	if (!port_num) {
		/* No target for this FC-SW */
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
		HFC_DBGPRT( "fx_wwnverify_gpnft() - non target");
		return;
	}

//	if ( port_num > MAX_TARGET_PROBE ) port_num = MAX_TARGET_PROBE;	/* FCLNX-GPL-FX-154*/

	detect_own_scsi_id = 0;
	for (i=0;i<port_num;i++) {
		scsi_id = 0;
		scsi_id = (uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].port_id[0]) << 16 ) +
			(uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].port_id[1]) << 8 ) +
			(uint)(hfc_fx_read_val( pyload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].port_id[2]) );
		
		wwpn = hfc_fx_read_val( core -> payload->receive_payload.type.gxx.sub_type.gpn_ft.portid[i].port_name ) ;
		
		if((scsi_id == 0)||(scsi_id == 0x00ffffff)||(wwpn == 0)||(wwpn == 0xffffffff)){
			/* scsi_id or wwpn is invalid. */
			HFC_DBGPRT("wwnverify_gpnft port_id or wwpn is invalid. port_id[%d] = %08x, wwpn = %llx\n", i, scsi_id, wwpn);
			continue;
		}
		
		if (pp->filter_target == HFC_FX_MB_LOGIN_FILTER_ON) { /* FCLNX-GPL-491 Filtering Login Target */
			if ((pp->scsi_id & 0x00ffff00) == (scsi_id & 0x00ffff00)) { /* domain,area */
				/* AccessGateway switch same physical port */
				continue; 
			}
		}
		
		HFC_DBGPRT("wwnverify_gpnft port_id[%d] = %08x\n",i, scsi_id);
		
		if(pp->scsi_id != scsi_id) {
			int new=TRUE;
			for (j=0;j<(int)(pp->max_target);j++) {
				target = pp->target_arg[j];
				if(target != NULL){
					if (target->ww_name == wwpn){
						new=FALSE;
						if(pp->ld_err_limit_s)	/* FCLNX-GPL-349 *//* FCLNX-GPL-598 */
						{ /* FCLNX-GPL-FX-472 */
							if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
								if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
									hfc_manage_info.npubp->hfc_fx_watched_errcount(pp, target, HFC_OCCURED_FAILURE, HFC_TGT_LDS_ERR);	/* FCLNX-0506 */
							}
							else{
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
								if (test_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags))
#else
								if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
#endif
									hfc_fx_watched_errcount_i(pp, target, HFC_TGT_LDS_ERR);	/* FCLNX-GPL-349 */
							}
						}																	/* FCLNX-GPL-327 *//* FCLNX-GPL-598 */
						if (!test_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status)) {	/* FCLNX-GPL-XXX */
							if (target->scsi_id != scsi_id){
								target->scsi_id = scsi_id;
								clear_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags);
								set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status );
								atomic_set(&pp->check_mbreq, 1);
								clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
							
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//								if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
//									clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
								if (HFC_FX_MQ_VALID(pp))	/* FCLNX-GPL-FX-270,275 */
									hfc_fx_mq_change_target_info(pp, target);
								HFC_DBGPRT("wwnverify_gpnft Target1 scsi_id changed . scsi_id[%d] = %08x\n",i, scsi_id);
								hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);
								no_target = 0;
							}														/* FCWIN-0166 */
							else if ( (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))
								 ||   (!test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags)) ){/* FCLNX-GPL-598 */
								HFC_DBGPRT("wwnverify_gpnft Target1 port_id[%d] = %08x\n",i, scsi_id);
								set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status );
								atomic_set(&pp->check_mbreq, 1);
								if (test_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status))/* FCLNX-GPL-598 */
									target->link_recovered = 1;
								clear_bit(HFC_TS_SCN_WLINKUP, (ulong *)&target->status);
								hfc_fx_watchdog_enter(pp, core, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
//								if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
//									clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
#endif
								no_target = 0;
							}
						}
					}
				}
			}
			if(new == TRUE) {/* Make new target if there is no target with the same WWN */
				detect_tgt_scsi_id++;	/* FCLNX-GPL-FX-154 */
				HFC_DBGPRT("add target scsi_id=%d wwpn= %llx\n", scsi_id, (unsigned long long)wwpn);
				if ( (target = hfc_fx_add_target_info_fx(pp, scsi_id)) != NULL ) {
					HFC_DBGPRT("set HFC_NEED_PLOGI_T\n");
					set_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status);
					atomic_set(&pp->check_mbreq, 1);
//					hfc_fx_enque_plogi_req(pp,target);
					no_target = 0;
				}
			}

		}
		else{
			detect_own_scsi_id = 1;
		}

		if (detect_tgt_scsi_id >= (pp->max_target)) { /* FCLNX-GPL-FX-154 */
				break;
		}

	}

	if(no_target){
		HFC_DBGPRT(" target is not found\n");
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}


	HFC_EXIT("hfc_fx_wwnverify_gidft");

}


void hfc_fx_wwnverify_plogi(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name)
{
	int i=0,tid=0,empty_hit=0;
	struct mailbox_fx	*mbox=NULL;
	int						hit          = 0;		/* FCLNX-GPL-FX-236 */
	struct target_info_fx	*work_target = NULL;	/* FCLNX-GPL-FX-236 */

	HFC_ENTRY("hfc_fx_wwnverify_plogi");

	HFC_DBGPRT( "wwnverify_plogi() login normal end \n");

	mbox = core->mb;

	switch (mb_resp_status) {
	case 0 :
		HFC_DBGPRT( "wwnverify_login() check login timing \n");

		/* Is this target in making process?*/
		if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
			empty_hit=0;
			
			/* If this target is not reserved with persistent binding, assign empty ID */
			HFC_DBGPRT( " allocate vacant id (without persistent binding) \n");
			
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				if ( !hfc_fx_hash_target_valid(pp, i) ) {	/* Not allocated */
					empty_hit=1;
					tid = i;
					break;
				}
			}
			
			if (empty_hit) {
				target->target_id      = tid;
				target->ww_name        = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwpn );
				target->node_name      = hfc_fx_read_val( mbox->mb_resp.type.plogi.target_wwnn );
				target->fc_class_mask  = (ushort) hfc_fx_read_val( mbox->mb_resp.type.plogi.class );
//				target->device_flags   = (ushort) hfc_fx_read_val(pp -> mb -> mb_resp.type.drvioctl1.login.parameter );
				target->max_frame_size = (ushort) hfc_fx_read_val( mbox->mb_resp.type.plogi.plogi_max_frame_size );
//				set_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags);
				pp->tid_map[tid]       = target->pseq;
				for (i=0;i<8;i++) {
					if (target->fc_class_mask & (0x80>>i))
						target->fc_class = 8-i;
				}
				
				set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				hfc_fx_enque_prli_req(pp,target);
				HFC_DBGPRT("    tid=%d ww_name=%llx, node_name=%llx, flag=%x, pseq=%d\n",
							target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name, target->flags, target->pseq);
			}
			else {
				/* PLOGI succeeded, but there is no target with specified target ID. */
				HFC_DBGPRT( "wwnverify_login() login succeeded, but the target id does not exist\n");
				HFC_DBGPRT("    tid=%d ww_name=%llx, node_name=%llx, flag=%x, pseq=%d\n",
							target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name, target->flags, target->pseq);

				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
		}
		else if ( test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
			set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
			atomic_set(&pp->check_mbreq, 1);
//			hfc_fx_enque_prli_req(pp,target);
		}
		
		break;


	case SCS_LOGIN_WWCHG :							/* WWN mismatch */
		HFC_DBGPRT("wwnverify_login() - login ww_name change.");

		/* LOGIN succeeded, but target->ww_name does not match with WNN in LOGIN parameter */
		hfc_fx_clear_target_info_fx( pp, target, TRUE );

		if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
			hfc_fx_wwnverify_linkup(pp, NULL, core, 0, ww_name);			/* Search target once more */
		}
		break;

	case SCS_NO_DEV_RESP :
	case SCS_CANCEL_RESP :
		set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);	/* FCLNX-GPL-FX-112 */
		atomic_set(&pp->check_mbreq, 1);							/* FCLNX-GPL-FX-112 */
		break;

	default :
		/* Remove target_info_fx due to abnormal end of LOGIN process*/
		HFC_DBGPRT( "wwnverify_login() - login fail.");
//		hfc_fx_clear_target_info_fx( pp, target, TRUE );			/* FCLNX-GPL-FX-446 */
		set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);	/* FCLNX-GPL-FX-112 */
		atomic_set(&pp->check_mbreq, 1);							/* FCLNX-GPL-FX-112 */
	}
	
	/* FCLNX-GPL-FX-236 start */
	hit = 0;
	for (i=0;i<MAX_TARGET_PROBE;i++) {
		work_target = pp->target_arg[i];
		if(work_target != NULL){
			if( (test_bit(HFC_TS_NEED_PRLI, (ulong *)&work_target->status ))
			||(test_bit(HFC_TS_WAIT_PRLI, (ulong *)&work_target->status ))
			||(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&work_target->status ))
			||(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&work_target->status )) ){
				hit = 1;	/* There remain targets needs LOGIN */
				break;
			}
		}
	}
	/* FCLNX-GPL-FX-236 end */
	
	if (!hit){	/* FCLNX-GPL-FX-236 */
		if (mb_resp_status != 0) {
			HFC_DBGPRT(  "set pp initialize = 0\n");
			if(pp->initialize != 0){
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			}
		}
	}	/* FCLNX-GPL-FX-236 */
	
	HFC_EXIT("hfc_fx_wwnverify_plogi");

}


void hfc_fx_wwnverify_prli(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name)
{
	int i=0,j,hit,tid=0,empty_hit=0;
	struct mailbox_fx	*mbox=NULL;
	
	HFC_ENTRY("hfc_fx_wwnverify_prli");

	HFC_DBGPRT( "fx_wwnverify_prli() login normal end \n");

	mbox = core->mb;

	switch (mb_resp_status) {
	case 0 :
		HFC_DBGPRT( "wwnverify_prli() check login timing \n");

		/* Is this target in making process?*/
		if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
			empty_hit=0;
			
			/* If this target is not reserved with persistent binding, assign empty ID */
			HFC_DBGPRT( " allocate vacant id (without persistent binding) \n");
			
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				if ( !hfc_fx_hash_target_valid(pp, i) ) {	/* Not allocated */
					empty_hit=1;
					tid = i;
					break;
				}
			}
			
			if (empty_hit) {
				target->device_flags   = (ushort)hfc_fx_read_val( core -> payload->receive_payload.type.prli.prli_param_resp) ;
				set_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags);
				if (HFC_FX_MQ_VALID(pp))
					hfc_fx_mq_change_target_info(pp, target);
				target->pp             = pp;
				target->dev            = NULL;
				target->group_id       = 0xff;
				target->attribute      = 0xff;
				target->path_id        = 0xff;
				target->lg_target      = NULL;
				
				HFC_DBGPRT(" empty_hit : tid=%d, ww_name = %llx, node_name=%llx flag=%x, preq=%d\n",
						target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name,
						target->flags, target->pseq);
				
				if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
					hfc_manage_info.npubp->hfc_fx_update_attribute(pp, target);
				}
			}
			else {
				/* LOGIN succeeded, but there is no target with specified target ID. */
				HFC_DBGPRT( "wwnverify_login() login succeeded, but the target id does not exist\n");
				HFC_DBGPRT("    tid=%d ww_name=%llx, node_name=%llx, flag=%x, pseq=%d\n",
							target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name, target->flags, target->pseq);
				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
		}
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&pp->pport->kthread_status ) ) {
			set_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
			atomic_set(&pp->pport->rport_event_wait, 1);
			wake_up_interruptible(&pp->pport->rport_event);
		}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
		break;


	case SCS_LOGIN_WWCHG :							/* WWN mismatch */
		HFC_DBGPRT("wwnverify_login() - login ww_name change.");

		/* LOGIN succeeded, but target->ww_name does not match with WNN in LOGIN parameter */
		hfc_fx_clear_target_info_fx( pp, target, TRUE );

		if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
			hfc_fx_wwnverify_linkup(pp, NULL, core, 0, ww_name);			/* Search target once more */
		}
		break;

	default :
		/* Remove target_info_fx due to abnormal end of LOGIN process*/
		HFC_DBGPRT( "wwnverify_login() - login fail.");
//		hfc_fx_clear_target_info_fx( pp, target, TRUE );			/* FCLNX-GPL-FX-446 */
		set_bit(HFC_TS_NEED_LOGO_TGT, (ulong *)&target->status);	/* FCLNX-GPL-FX-112 */
		atomic_set(&pp->check_mbreq, 1);							/* FCLNX-GPL-FX-112 */
	}

	hit = 0;
	for (j=0;j<MAX_TARGET_PROBE;j++) {
		target = pp->target_arg[j];
		if(target != NULL){
			if( (test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status ))
			||(test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status ))
			||(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status ))
			||(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status )) ){
				hit = 1;	/* There remain targets needs LOGIN */
				break;
			}
		}
	}

	if (!hit){
		HFC_DBGPRT(  "set pp initialize = 0\n");
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}
	
	
	HFC_EXIT("hfc_fx_wwnverify_prli");

}


void hfc_fx_wwnverify_receive_plogi(struct port_info *pp, struct target_info_fx *target, struct core_info *core, 
							uint mb_resp_status, uint64_t ww_name)
{
	int i=0,j,hit,tid=0,empty_hit=0;
	struct mailbox_fx	*mbox=NULL;

	HFC_ENTRY("hfc_fx_wwnverify_receive_plogi");

	HFC_DBGPRT( "wwnverify_receive_plogi() login normal end \n");

	mbox = core->mb;

	switch (mb_resp_status) {
	case 0 :
		HFC_DBGPRT( "wwnverify_receive_plogi() check login timing \n");

		/* Is this target in making process?*/
		if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
			empty_hit=0;
			
			/* If this target is not reserved with persistent binding, assign empty ID */
			HFC_DBGPRT( " allocate vacant id (without persistent binding) \n");
			
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				if ( !hfc_fx_hash_target_valid(pp, i) ) {	/* Not allocated */
					empty_hit=1;
					tid = i;
					break;
				}
			}
			
			if (empty_hit) {
				target->target_id      = tid;
				target->ww_name        = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.target_wwpn );
				target->node_name      = hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.target_wwnn );
				target->fc_class_mask  = (ushort) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.fc_class );
//				target->device_flags   = (ushort) hfc_fx_read_val(pp -> mb -> mb_resp.type.drvioctl1.login.parameter );
				target->max_frame_size = (ushort) hfc_fx_read_val( mbox->mb_intreq.type.rcvplogi.plogi_max_frame_size );
				set_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags);
				pp->tid_map[tid]       = target->pseq;
				for (i=0;i<8;i++) {
					if (target->fc_class_mask & (0x80>>i))
						target->fc_class = 8-i;
				}
				
				set_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status);
				atomic_set(&pp->check_mbreq, 1);
				clear_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status);
			}
			else {
				/* PLOGI succeeded, but there is no target with specified target ID. */
				HFC_DBGPRT( "wwnverify_receive_plogi() login succeeded, but the target id does not exist\n");
				HFC_DBGPRT("    tid=%d ww_name=%llx, node_name=%llx, flag=%x, pseq=%d\n",
							target->target_id, (unsigned long long)target->ww_name, (unsigned long long)target->node_name, target->flags, target->pseq);
				hfc_fx_clear_target_info_fx( pp, target, TRUE );
			}
		}
		
		break;


	case SCS_LOGIN_WWCHG :							/* WWN mismatch */
		HFC_DBGPRT("wwnverify_receive_plogi() - login ww_name change.");

		/* LOGIN succeeded, but target->ww_name does not match with WNN in LOGIN parameter */
		hfc_fx_clear_target_info_fx( pp, target, TRUE );

		if ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
			hfc_fx_wwnverify_linkup(pp, NULL, core, 0, ww_name);			/* Search target once more */
		}
		break;

	case SCS_NO_DEV_RESP :
	case SCS_CANCEL_RESP :
		break;

	default :
		/* Remove target_info_fx due to abnormal end of LOGIN process*/
		HFC_DBGPRT( "wwnverify_receive_plogi() - login fail.");
		hfc_fx_clear_target_info_fx( pp, target, TRUE );
	}

	hit = 0;
	for (j=0;j<MAX_TARGET_PROBE;j++) {
		target = pp->target_arg[j];
		if(target != NULL){
			if( (test_bit(HFC_TS_NEED_PRLI, (ulong *)&target->status ))
			||(test_bit(HFC_TS_WAIT_PRLI, (ulong *)&target->status ))
			||(test_bit(HFC_TS_NEED_PLOGI, (ulong *)&target->status )) /* FCLNX-GPL-FX-236 */
			||(test_bit(HFC_TS_WAIT_PLOGI, (ulong *)&target->status )) ){ /* FCLNX-GPL-FX-236 */
				hit = 1;	/* There remain targets needs LOGIN */
				break;
			}
		}
	}

	if (!hit){
		HFC_DBGPRT(  "set pp initialize = 0\n");
		if(pp->initialize != 0){
			hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
		}
	}
	
	HFC_EXIT("hfc_fx_wwnverify_receive_plogi");

}

void hfc_fx_change_portstat_linkup(struct port_info *pp, struct core_info *core)
{
	int i,lp;	/* FCLNX-GPL-FX-163 */
	struct port_info		*vpp;
	struct core_info		*wk_core=NULL;	/* FCLNX-GPL-FX-163 */
	struct target_info_fx	*target=NULL;	/* FCLNX-GPL-FX-163 */
	uchar					after_linkup = 0;
	
	HFC_ENTRY("hfc_fx_change_portstat_linkup");
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKINIT_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKUP_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_MCK_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */
	
	if(test_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1)){
		hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKUP );
		after_linkup = 1;
	}
	else{
		hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKINI );
	}
	
	if( (test_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1))
	||  (!test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status) )
	||  (pp->mck_linkup == HFC_LINKUP_MCK)){
		hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT1,0x15,core->logdata,16) ;	/* FCLNX-GPL-FX-005 */
	}
	
	clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
	clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);
	clear_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1 );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
#endif	/* FCLNX-GPL-FX-424 */
	set_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
	
	/* Shadow Driver sets Link Initialize Complete status */
	if ( HFC_FX_MMODE_CHECK_SHADOW(pp) ) {
		if ( hfc_fx_mlpf_config_check(pp, pp->region_arg[pp->rid]->core_arg[pp->master_core_no]) ){
			HFC_DBGPRT("hfcldd%d: hfc_fx_change_portstat_linkup - Unshared\n", pp->dev_minor);
		}
		if ( test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol) ) {	/* FCLNX-GPL-427 */
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			clear_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol);
		}
		else if(pp->mck_linkup){
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_CMD_MCKRECCMP|((uint)pp->mck_errno << 8));
		}else{
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_CMD_LINKINICMP);
		}
	}
	
	pp->mck_linkup = HFC_LINKUP_NOMCK;			/* FCLNX-595 */
	pp->mck_errno = 0;							/* FCLNX-GPL-FX-376 */
	
	if ((pp->connect_type == HFC_FX_AL)||(pp->connect_type == HFC_FX_MULTI_ALPA)) {	/* FCLNX-GPL-FX-163 */
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
		{
			target = hfc_fx_hash_target_info(pp, lp);
			if (target != NULL)
			{
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					if (wk_core->core_no == pp->master_core_no)
						continue;
					if(hfc_fx_check_cs_disable(pp, wk_core))
						continue;	/* FCLNX-GPL-FX-438 */
					
					hfc_fx_cancel_wxque(pp,wk_core,target,0,NULL,SCS_INTR_LINKDOWN,HFC_CSCSI_ERROR,HFC_FLASH_TARGET);
				}
			}
		}
	}	/* FCLNX-GPL-FX-163 */
	
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		if (HFC_FX_MQ_VALID(pp) && after_linkup) {
			HFC_DBGPRT( "hfc_fx_change_portstat_linkup - start for mq\n");
			for (i=1; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				if (!test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status))
					continue;
				
				hfc_fx_w_stop( vpp, NULL, HFC_FX_LINKINIT_TMR );
				hfc_fx_w_stop( vpp, NULL, HFC_FX_LINKUP_TMR );
				hfc_fx_w_stop( vpp, NULL, HFC_FX_WLINKUP_MCK_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
					hfc_fx_w_stop( vpp, NULL, HFC_FX_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */
				
				if ((pp->connect_type == HFC_FX_AL)||(pp->connect_type == HFC_FX_MULTI_ALPA)) {	/* FCLNX-GPL-FX-219 *//* FCLNX-GPL-FX-247,272 */
					HFC_DBGPRT( "hfc_fx_change_portstat_linkup - HFC_FX_AL or HFC_FX_MULTI_ALPA\n");
					clear_bit(HFC_PS_ONLINE, (ulong *)&vpp->status);
				}
				else {
					if(test_bit(HFC_PD_AFTER_LINKUP, (ulong *)&vpp->status_detail1)){
						hfc_fx_issue_change_state(vpp, HFC_FX_CHANGE_STATE_LINKUP );
					}
					else{
						hfc_fx_issue_change_state(vpp, HFC_FX_CHANGE_STATE_LINKINI );
					}
					set_bit(HFC_PS_ONLINE, (ulong *)&vpp->status);
				}	/* FCLNX-GPL-FX-219,272 */
				
				clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&vpp->status);
				clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&vpp->status);
				clear_bit(HFC_PD_AFTER_LINKUP, (ulong *)&vpp->status_detail1 );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
				if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
					clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
#endif	/* FCLNX-GPL-FX-424 */
				vpp->switch_exist = pp->switch_exist;
				vpp->connect_type = pp->connect_type;
				vpp->scsi_id = pp->scsi_id;
				
				vpp->mck_linkup = HFC_LINKUP_NOMCK;
				
			}
			HFC_DBGPRT( "hfc_fx_change_portstat_linkup - end for mq\n");
		}
		else if (HFC_FX_VPORT_EXIST(pp)) {
			for (i=1; i<=pp->max_vport_count; i++) {
				vpp = pp->vport_ptr[i].vport_arg;
				if (vpp == NULL)
					continue;
				if (!(test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status)))
					continue;
				
				if ((!HFC_FX_MIN_PORT_IN_REGION(vpp)) &&
					(!test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2))) {	/* FCLNX-GPL-FX-248,272 */
					vpp->host_alpa = pp->host_alpa;
					vpp->used_nmsrv = pp->used_nmsrv;
					vpp->connect_type = pp->connect_type;
					set_bit(HFC_PD_NEED_FLOGI, (ulong *)&vpp->status_detail1);
					set_bit(HFC_PS_CONNECTED, (ulong *)&vpp->status);
				}
				
				HFC_DBGPRT("hfc_fx_change_portstat_linkup - start next mailbox for vport\n");
				atomic_set(&vpp->check_mbreq, 1);
				start_fx_next_mailbox(vpp, NULL);
			}
		}
	}
	
	HFC_EXIT("hfc_fx_change_portstat_linkup");
}

void hfc_fx_change_portstat_linkdown(struct port_info *pp, struct core_info *core)
{
	int i;
	struct port_info		*vpp;
	uchar					after_linkup = 0;	/* FCLNX-GPL-FX-401 */
	uint					status = 0;			/* FCLNX-GPL-FX-407 */
	
	HFC_ENTRY("hfc_fx_change_portstat_linkdown");
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKINIT_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_LINKUP_TMR );
	hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_MCK_TMR );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		hfc_fx_w_stop( pp, NULL, HFC_FX_WLINKUP_CNT_TMR );
#endif	/* FCLNX-GPL-FX-424 */

	if(test_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1)){
		hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKDOWN );
		after_linkup = 1;
	}
	else{
		hfc_fx_issue_change_state(pp, HFC_FX_CHANGE_STATE_LINKINI_ERR );
	}
	
	clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status);
	clear_bit(HFC_PS_CONNECTED, (ulong *)&pp->status);
	clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);
	clear_bit(HFC_PD_AFTER_LINKUP, (ulong *)&pp->status_detail1 );
	HFC_DETAIL_CLEAR_LINKDOWN(pp);
	
	if (HFC_FX_MMODE_CHECK_SHADOW(pp)){
		if((after_linkup == 0)
		&&((pp->switch_exist != HFC_SWITCH_EXIST)||(pp->connect_type != HFC_FX_MULTI_ALPA))){	/* FCLNX-GPL-FX-401 *//* FCLNX-GPL-FX-407 */
			status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
			status &= ~HFC_HG_LPRSTATUS_UNSHARABLE;
			status |= HFC_HG_LPRSTATUS_LINKDOWN;
			if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
				status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );
		}	/* FCLNX-GPL-FX-401 *//* FCLNX-GPL-FX-407 */
		
		if ( test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol) ) {	/* FCLNX-GPL-427 */
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			clear_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol);
		}														/* FCLNX-GPL-393 */
		else if(pp->mck_linkup){
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_CMD_MCKRECCMP|((uint)pp->mck_errno << 8));
		}
	}
	pp->mck_linkup = HFC_LINKUP_NOMCK;			/* FCLNX-595 */
	pp->mck_errno = 0;							/* FCLNX-GPL-FX-376 */
	
	if (HFC_FX_PHYSICAL_PORT(pp)) {
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg;
			if (vpp == NULL)
				continue;
			if (!(test_bit(HFC_PS_ENABLE, (ulong *)&vpp->status)))
				continue;
			
			HFC_DBGPRT("hfc_fx_change_portstat_linkup - start next mailbox for vport\n");
			atomic_set(&vpp->check_mbreq, 1);
			start_fx_next_mailbox(vpp, NULL);
		}
	}
	
	if (HFC_FX_VIRTUAL_PORT(pp) && HFC_FX_VPORT_ENABLE(pp)) {
		hfc_fx_vport_set_state(pp->fc_vport, FC_VPORT_LINKDOWN);
	}
	
	HFC_EXIT("hfc_fx_change_portstat_linkdown");
}

void hfc_fx_wwnverify_scn(struct port_info *pp, struct target_info_fx *target, uint mb_resp_status)
{

	HFC_ENTRY("hfc_fx_wwnverify_scn");

	if (    (pp->connect_type == HFC_FX_SWITCH ) /* FC-SW ? */
		|| ((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00)) ) {/* FCWIN-0185 */
		if (!test_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2) ) {
			set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);		/* Reserve GID_FT */
			atomic_set(&pp->check_mbreq, 1);
		}
	}

	HFC_EXIT("hfc_fx_wwnverify_scn");
}


struct target_info_fx *hfc_fx_add_target_info_fx( struct port_info *pp, uint64_t scsi_id )
{
	struct port_info		*vpp;
	struct target_info_fx	*target, *target_wk;
	struct scsi_cmnd		*dummy_cmnd[MAX_CORE_PROBE_FX];
	struct hfc_pkt_fx		*hfcp=NULL;
	uint pseq;
	int  i;

	HFC_ENTRY("hfc_fx_add_target_info_fx");
	
	HFC_DBGPRT("hfc_fx_add_target_info_fx\n");

	if ( (pseq = hfc_fx_uniq_seq_num( pp )) == 0xFFFFFFFF )
		return (NULL);

	HFC_DBGPRT("hfc_fx_add_target_info_fx pseq = %d\n",pseq);
	
	if (pp->target_arg[pseq] == NULL) {
		pp->target_arg[pseq] = (struct target_info_fx *)hfc_fx_kmalloc(pp, sizeof(struct target_info_fx), GFP_ATOMIC);
		if(pp->target_arg[pseq] == NULL){
			uchar logdata[16];
			
			memset(logdata, 0x00, sizeof(logdata));
			logdata[0] = 0xc0;
			hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, (uchar*)logdata, 16) ;
			return(NULL);
		}
	}
	
	if (HFC_FX_MQ_VALID(pp)) {
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg;
			if (vpp == NULL)
				continue;
			
			if (vpp->target_arg[pseq] == NULL) {
				vpp->target_arg[pseq] = (struct target_info_fx *)hfc_fx_kmalloc(vpp, sizeof(struct target_info_fx), GFP_ATOMIC);
				if(vpp->target_arg[pseq] == NULL){
					uchar logdata[16];
					
					memset(logdata, 0x00, sizeof(logdata));
					logdata[0] = 0xc0;
					hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, (uchar*)logdata, 16) ;
					return(NULL);
				}
			}
		}
	}
	
	hfcp = (struct hfc_pkt_fx *)hfc_fx_kmalloc(pp, sizeof(struct hfc_pkt_fx)*MAX_CORE_PROBE_FX*3, GFP_ATOMIC);
	if (hfcp == NULL){
		uchar logdata[16];
		
		memset(logdata, 0x00, sizeof(logdata));
		logdata[0] = 0xc1;
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, (uchar*)logdata, 16) ;
	}
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		dummy_cmnd[i] = hfc_fx_get_new_cmnd(pp);  
	}
	
	target = pp->target_arg[pseq];
	memset( target, 0, sizeof(struct target_info_fx) );
	target->pseq      = (uchar) pseq;
	target->scsi_id   = (uint64_t) scsi_id;
	target->login_seq_retry_cnt = pp->login_seq_retry_cnt;	/* FCLNX-GPL-FX-446 >>> */
	pp->target_cnt++;
	
	HFC_DBGPRT( " target exist target = %lx\n", *((ulong *)&target));
	HFC_DBGPRT( "target  = %d, %d, %08x \n",(uint)target->flags, (uint)target->pseq, (uint)target->scsi_id);
	
	if (HFC_FX_MQ_VALID(pp)) {
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg;
			if (vpp == NULL)
				continue;
			
			target_wk = vpp->target_arg[pseq];
			memset( target_wk, 0, sizeof(struct target_info_fx) );
			target_wk->pseq      = (uchar) pseq;
			target_wk->scsi_id   = (uint64_t) scsi_id;
			vpp->target_cnt++;
			
			HFC_DBGPRT( " target exist target = %lx\n", *((ulong *)&target_wk));
			HFC_DBGPRT( "target  = %d, %d, %08x \n",(uint)target_wk->flags, (uint)target_wk->pseq, (uint)target_wk->scsi_id);
		}
	}
	
	if (hfcp != NULL) {
		memset( hfcp, 0, sizeof(struct hfc_pkt_fx)*MAX_CORE_PROBE_FX*3 );
		target->reset_pkt = hfcp;  
	}
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		target->dummy_cmnd[i] = dummy_cmnd[i];
	}
	
	HFC_EXIT("hfc_fx_add_target_info_fx");

	return (target);
}


void hfc_fx_clear_target_info_fx(
	struct port_info *pp, struct target_info_fx *target, int pmsg ){

	uint 	tid,pseq,i;
	struct	region_info	*rp=NULL;
	struct	core_info	*core=NULL;

	HFC_ENTRY("hfc_fx_clear_target_info_fx");

	rp = pp->region_arg[pp->rid];

	tid  = target->target_id ;
	pseq = target->pseq ;

	/* Set target-stop-waiting bit */
//	set_bit(HFC_WAIT_TARGET_STOP, (ulong *)&target->status);

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_fo_check_and_offline(target, pmsg);
	}

	/* Delete Target_info *//* FCLNX-GPL-FX-275 */
	hfc_fx_watchdog_enter( /* Cancel Timer */
		pp, NULL, target, NULL, 0, HFC_FX_DELAY_TMR, 0, TRUE);
	hfc_fx_watchdog_enter( /* Cancel Timer */
		pp, NULL, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, TRUE);

	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core = rp->core_arg[i];
		hfc_fx_deque_next_dstart(pp, rp, core, target );/* Release Next dstart waiting state */
	}
	
	/* FCLNX-GPL-FX-237 start */
	target->status = 0 ;
	clear_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags); /* temp */
	if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&pp->pport->kthread_status) ){
		set_bit( HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status );
		atomic_set(&pp->pport->rport_event_wait, 1);
		wake_up_interruptible(&pp->pport->rport_event);
	}
	if ( !test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags) ){
		hfc_fx_release_seq_num(pp, target->pseq);
		target->flags = 0;
		pp->target_cnt--;
	}
	/* FCLNX-GPL-FX-237 end */

	HFC_EXIT("hfc_fx_clear_target_info_fx");

	return;
}

/*
 * Function: hfc_fx_info
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
const char *hfc_fx_info (struct Scsi_Host *host)
{
	static char buffer[512];
	struct hfc_vpd	*vpd_info;
	struct port_info	*pp;
	uint	sys_rev=0;	/* FCLNX-GPL-112 *//* FCLNX-GPL-465 */

	pp = (struct port_info *)host->hostdata;

	buffer[0] = '\0';

	vpd_info = (struct hfc_vpd *)pp->vpd_buf;

	memset(&buffer[0], 0, sizeof(buffer));

	/* FCLNX-GPL-FX-218 */
	sprintf(&buffer[strlen(buffer)], "Hitachi FIVE-FX(16Gbps) based Fibre Channel to PCIe HBA: device %02x:%02x.%02x IRQ %d \n",
				pp->pci_cfginf->bus->number, 
				PCI_SLOT(pp->pci_cfginf->devfn),
				PCI_FUNC(pp->pci_cfginf->devfn),
				pp->pci_cfginf->irq);
	
	sys_rev = pp->sys_rev;
	
	sprintf(&buffer[strlen(buffer)], "      Firmware version %x, Driver version %s \n",
				sys_rev, hfc_manage_info.package_ver);	/* FCLNX-GPL-112 */

//	sprintf(&buffer[strlen(buffer)], "  (%016lx)\n",jiffies);									/* FCLNX-0266 *//* FCLNX-GPL-474 */

	if(pp->defparam)     sprintf(&buffer[strlen(buffer)], "       default parameter is enabled\n");

	if(pp->narrowmap == 1)		sprintf(&buffer[strlen(buffer)], "       narrow mode(lun) is enabled\n");	/* DPM */
	else if(pp->narrowmap == 2)	sprintf(&buffer[strlen(buffer)], "       narrow mode(wwpn) is enabled\n");	/* FC-GW */

	sprintf(&buffer[strlen(buffer)], "        hfcl%d-wwpn=0x%llx",pp->instance,(unsigned long long)pp->ww_name);	/* FCLNX-GPL-474 */
	sprintf(&buffer[strlen(buffer)], "  (%016lx)\n",jiffies);														/* FCLNX-GPL-474 */
	
	if ( ( pp->isol_force == HFC_PRT_FRC_ISOL ) || (pp->isol_force == HFC_SHARED_PRT_FRC_ISOL ) ){			/* FCLNX-GPL-393 */
			sprintf(&buffer[strlen(buffer)], "        adapter WWPN  0x%llx was forced to be isolated.\n",
			(unsigned long long)pp->ww_name);	
	}

	if ( pp->isol_force == HFC_CHKSTP_FRC_ISOL ){	/* FCLNX-GPL-147*/
			sprintf(&buffer[strlen(buffer)], "        adapter WWPN  0x%llx was forced to be isolated.(CHK-STP).\n",
			(unsigned long long)pp->ww_name);
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
hfc_fx_proc_info(char *buffer, char **start, off_t offset, int length, int hostno, int inout)
{
	struct Scsi_Host 	*host;
	struct port_info 	*pp;
	int					i; /* FCLNX-GPL-177 */

	host = NULL;
	/* Find the specified Host */
	for(i=0; i<MAX_ADAP_CNT; i++) /* FCLNX-GPL-177 */
	{
		pp = hfc_manage_info.port_info_arg[i];
		if(pp->hosts->host_no == hostno)
		{
			host = pp->hosts;
			return hfc_fx_proc_info_k26(host, buffer, start, offset, length, inout);
		}
	}
	
	return -ESRCH;

}	
			
/* FCLNX-GPL-564 start */
/*
 * Function:    hfc_fx_proc_info_com
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
int
hfc_fx_proc_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length, uint proc_type)
{
	struct hfc_vpd			*vpd_info  = NULL;
	struct hfc_vpd_five		*vpdf_info = NULL;
	struct hfc_vpd_five_ex	*vpdex_info= NULL;
	struct hfc_vpd_five_fx	*vpdfx_info= NULL;
	struct pci_dev			*slot_dev  = NULL; /* FCLNX-GPL-180 */
	struct port_info		*pp, *wkpp;
	struct target_info_fx	*tp;
	struct dev_info_fx		*dp=NULL;
	struct region_info		*rp = NULL, *wkrp;
	
	short				vendor_id, device_id, sub_system_id;
	int					i,j,hit;
	uchar				pre_conf=0;
	int					rtn;
	char				buf[256];
	char				*data = buffer;
	int					len, partial, pos = 0;
	struct target_info_fx	*target;
	uint				status;	/* FCLNX-GPL-393 */
	uint				ecid_wk;
#ifdef HFC_HVM_DEBUG
	int size;
	uchar *tmp_cca;
#endif /* HFC_HVM_DEBUG */
	
	pp = (struct port_info *)host->hostdata;
	if(HFC_FX_MMODE_CHECK_MLPF(pp) ){
		rp =  pp->region_arg[pp->rid];
	}else{
		rp =  pp->region_arg[0];
	}
	
	SPRINTF ("Hitachi FIVE-FX(16Gbps) based Fibre Channel to PCIe HBA\n");	/* FCLNX-GPL-FX-218 */
	
	if(pp->pkg.type == HFC_PKTYPE_FPP){
		vpd_info = (struct hfc_vpd *)pp->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpd_info->driver_ver, vpd_info->fw_ver);
	
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE){
		vpdf_info = (struct hfc_vpd_five *)pp->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpdf_info->driver_ver, hfc_fx_get_sysrev(rp->core_arg[pp->master_core_no]));
		/* FCLNX-GPL-112 */
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE_EX){/* FIVE-EX */
		vpdex_info = (struct hfc_vpd_five_ex *)pp->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpdex_info->driver_ver, hfc_fx_get_sysrev(rp->core_arg[pp->master_core_no]));
		/* FCLNX-GPL-112 */
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE_FX){/* FIVE-FX */
		vpdfx_info = (struct hfc_vpd_five_fx *)pp->vpd_buf;
		SPRINTF ("  Driver version %s  Firmware version %06x\n",vpdfx_info->driver_ver, hfc_fx_get_sysrev(rp->core_arg[pp->master_core_no]));
	}
	
	SPRINTF ("  Package_ID              = 0x%02x\n",pp->pkg.code);
	SPRINTF ("  Special file name       = hfcldd%d\n", pp->dev_minor);
	
	SPRINTF ("  Major_number            = %d\n", pp->dev_major);
	SPRINTF ("  Minor_number            = %d\n", pp->dev_minor);			
	SPRINTF ("  Instance_number         = %d\n",pp->instance);						
	SPRINTF ("  Host# = %d, Unique id   = %d \n", host->host_no, host->unique_id);
	SPRINTF ("  PCI memory space address= 0x%08llx (%d)\n", (unsigned long long)pp->mem_base_addr, (int)sizeof(dma_addr_t));
	
	SPRINTF("  adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_fx_read_cnfg(pp, 0x00, 0x2);
	device_id = (ushort) hfc_fx_read_cnfg(pp, 0x02, 0x2 );
	/* read config register (0x2e) *//* FCLNX-GPL-FX-048 */
	sub_system_id = (ushort)hfc_fx_read_cnfg(pp, HFC_HOST_SUB_SYSTEM_ID, 0x2);
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	SPRINTF ("   Sub_system ID          =  %x\n", sub_system_id);
	SPRINTF ("   Port name              =  %llx \n", (unsigned long long)pp->ww_name);
	SPRINTF ("   Node name              =  %llx \n", (unsigned long long)pp->node_name);
	SPRINTF ("   DID                    =  %06llx \n", (unsigned long long)pp->scsi_id);
	
	if ( pp->pkg.type == HFC_PKTYPE_FPP )
	{
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0x4B, 0x1);
	}
	else if( pp->pkg.type == HFC_PKTYPE_FIVE )
	{
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xAF, 0x1);
	}
	else /* FIVE-EX */
	{
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xCA, 0x1);
	}
	
	if( (pre_conf == 0x01) || (pre_conf == 0x03) ){
		SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)pp->org_ww_name);
		SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)pp->org_node_name);
		SPRINTF ("   Add port name          =  %llx \n", (unsigned long long)pp->add_ww_name);
		SPRINTF ("   Add node name          =  %llx \n", (unsigned long long)pp->add_node_name);
		SPRINTF ("   hfc_pxe_boot           =  %d \n", hfc_pxe_boot);			
	}
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) ) {
		SPRINTF ("   MLPF MODE              =  %x \n", pp->mlpf_mode);	
		if ( !(HFC_FX_MMODE_CHECK_DEDICATE(pp)) ) {
			SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)pp->org_ww_name);
			SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)pp->org_node_name);
		}
		SPRINTF ("   VFC port name          =  %llx \n", (unsigned long long)pp->vfc_ww_name);
		SPRINTF ("   VFC node name          =  %llx \n", (unsigned long long)pp->vfc_node_name);
		if ( !(HFC_FX_MMODE_CHECK_DEDICATE(pp)) ) {
			SPRINTF ("   RID                    =  %x \n", pp->rid);
		}
	}
	
	SPRINTF ("   adapter ID             =  ");
	for(i=0; i<16; i++) SPRINTF("%02x",pp->adap_id[i]);
		SPRINTF("\n");
		
	SPRINTF ("   port number            =  %d\n", pp->port_no);
//#ifdef _HFC_FX_PROTO	/* FCLNX-GPL-FX-164 */
	SPRINTF ("   Number of core         =  %d\n", pp->core_num);
//#endif				/* FCLNX-GPL-FX-164 */
	
	if(pp->pkg.type == HFC_PKTYPE_FPP){
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
		
		SPRINTF ("   model name             =  %s \n", pp->model_name);
		
	}else if(pp->pkg.type == HFC_PKTYPE_FIVE){
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
		
		SPRINTF ("   model name             =  %s \n", pp->model_name);
		SPRINTF ("   location               =  %02x:%02x.%02x\n", 							/* FCLNX-0404 */
												pp->pci_cfginf->bus->number,				/* FCLNX-0404 */
												PCI_SLOT(pp->pci_cfginf->devfn),			/* FCLNX-0404 */
												PCI_FUNC(pp->pci_cfginf->devfn));			/* FCLNX-0404 */
	}else if(pp->pkg.type == HFC_PKTYPE_FIVE_EX){ /* FIVE-EX */
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
		
		SPRINTF ("   model name             =  %s \n", pp->model_name);
		SPRINTF ("   location               =  %02x:%02x.%02x\n", 							/* FCLNX-0404 */
												pp->pci_cfginf->bus->number,				/* FCLNX-0404 */
												PCI_SLOT(pp->pci_cfginf->devfn),			/* FCLNX-0404 */
												PCI_FUNC(pp->pci_cfginf->devfn));			/* FCLNX-0404 */
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE_FX){/* FIVE-FX */
		char wkbuf[VPD_PN_LEN+13];  /* +13=NULL */
		
		SPRINTF ("   manufacturer ID        =  ");
		for(i=0; i<vpdfx_info->mn_len;i++)
				SPRINTF("%c",vpdfx_info->mn_value[i]);
		SPRINTF("\n");
		
		SPRINTF ("   parts number           =  ");
		memset(wkbuf,0,sizeof(wkbuf));									/* FCLNX-0337 *//* FCLNX-0368 */
		memcpy(wkbuf,vpdfx_info->pn_value,VPD_PN_LEN);					/* FCLNX-0337 *//* FCLNX-0368 */
		SPRINTF("%s\n",wkbuf);											/* FCLNX-0337 *//* FCLNX-0368 */
		
		SPRINTF ("   ec level               =  ");
		SPRINTF("%c",vpdfx_info->ec_level);
		SPRINTF("\n");
		
		SPRINTF ("   model name             =  %s \n", pp->model_name);
		SPRINTF ("   location               =  %02x:%02x.%02x\n", 							/* FCLNX-0404 */
												pp->pci_cfginf->bus->number,				/* FCLNX-0404 */
												PCI_SLOT(pp->pci_cfginf->devfn),			/* FCLNX-0404 */
												PCI_FUNC(pp->pci_cfginf->devfn));			/* FCLNX-0404 */
	}
	
	slot_dev = hfc_fx_get_slot_dev(pp); /* FCLNX-GPL-180 start */
	if(slot_dev != NULL)
	{
		SPRINTF ("   slot location          =  %02x:%02x.%02x\n",
										slot_dev->bus->number,
										PCI_SLOT(slot_dev->devfn),
										PCI_FUNC(slot_dev->devfn));
	} /* FCLNX-GPL-180 end */

	if( pp->raslog_install == 0 ){
		SPRINTF ("   Raslog version         =  raslog-%d.%d.%d-%d \n", pp->raslog_ver, pp->raslog_rev,
						pp->raslog_rver, pp->raslog_wver);
	}

	SPRINTF ("  Current Information \n");
	
	SPRINTF ("   Connection Type        = ");
	switch(pp->connect_type) {
		case HFC_FX_SWITCH	:	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
								SPRINTF (" Point to Point (fabric) [");
							}
							else{
								SPRINTF (" - [");
							}
							break;
		case HFC_FX_AL		:	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
								if (pp -> scsi_id & 0x00ffff00) {
									SPRINTF (" FC-AL (fabric) [");
								}
								else{
									SPRINTF (" FC-AL [");
								}
							}
							else{
								SPRINTF (" - [");
							}
							break;
		
		case HFC_FX_PT2PT	:	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
								SPRINTF (" Point to Point [");
							}
							else{
								SPRINTF (" - [");
							}
							break;
		case HFC_FX_F_PORT	:	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
								SPRINTF (" Point to Point (F-Port) [");
							}
							else{
								SPRINTF (" - [");
							}
							break;
		case HFC_FX_MULTI_ALPA : if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
								SPRINTF (" FC-AL (Multi ALPA) [");
							}
							else{
								SPRINTF (" - [");
							}
							break;
		default			:	SPRINTF (" - [");
		
	}
	
	switch(pp->topology){
		case 0				: SPRINTF (" Auto ]\n"); break;
		case HFC_FX_AL		: SPRINTF (" FC-AL ]\n"); break;
		case HFC_FX_PT2PT	: SPRINTF (" Point to Point ]\n"); break;
		case HFC_FX_F_PORT	: SPRINTF (" F-Port ]\n"); break;
		case HFC_FX_MULTI_ALPA: SPRINTF (" Multi ALPA ]\n"); break;
		default				: SPRINTF (" %d ]\n",pp->topology);
	}
	SPRINTF ("   Multiple Portid        =  %d\n", pp->multiple_portid);			/* FCLNX-GPL-FX-135 */
	
	SPRINTF ("   Link Speed             = ");
	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){
		SPRINTF (" %dGbps [", (pp->max_data_rate/100));
	}
	else{
		SPRINTF (" - [");
	}
	if(pp->linkspeed != 0){
		SPRINTF (" %dGbps ]\n", pp->linkspeed);
	}else{
		SPRINTF (" Auto ]\n");
	}
	
	
	SPRINTF ("   Max Transfer Size      =  %dMB\n", (pp->dma_max/(1024*1024)) );
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
		SPRINTF ("   Link Down Time         =  %dsec\n", pp->dev_loss_tmo);
	} else {
		SPRINTF ("   Link Down Time         =  %dsec\n", pp->linkup_tmo);
	}
#else
	SPRINTF ("   Link Down Time         =  %dsec\n", pp->linkup_tmo);
#endif
	SPRINTF ("   Reset Delay Time       =  %dsec\n", pp->scsi_reset_delay);
	SPRINTF ("   Machine Check Retry Count  =  %d\n", pp->max_mck_cnt);			/* FCLNX_011 */
	SPRINTF ("   Preferred AL-PA Number =  %02x\n", pp->pref_alpa);				/* FCLNX_011 */
	SPRINTF ("   Reset Timeout          =  %dsec\n", pp->target_reset_tmo);  	/* FCLNX_011 */
	SPRINTF ("   Abort timeout          =  %dsec\n", pp->abort_tmo);			/* FCLNX_011 */
	SPRINTF ("   Queue depth            =  %d\n", pp->queue_depth);
	SPRINTF ("   Enable Message         =  %d\n", pp->wmsg);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
		SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", pp->dev_loss_tmo);
	} else {
		SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", pp->mck_rcv_tmo);				/* FCLNX-0241*/
	}
#else
	SPRINTF ("   Link Down Time (MCK)   =  %dsec\n", pp->mck_rcv_tmo);				/* FCLNX-0241*/
#endif
	SPRINTF ("   Login delay            =  %dsec\n", pp->login_wait);				/* FCLNX-0243*/
	
	SPRINTF ("   Max target number      =  %d\n", pp->max_target);
	SPRINTF ("   Max xob number         =  %d\n", pp->xob_max);
	SPRINTF ("   Max xrb number         =  %d\n", pp->xrb_max);
	SPRINTF ("   Max soft log number    =  %d\n", pp->slog_max);
	SPRINTF ("   Max trace number       =  %d\n", pp->trc_max);
	SPRINTF ("   Max pkt number         =  %d\n", pp->pkt_num);
	SPRINTF ("   Max can queue          =  %d\n", pp->can_queue);
	SPRINTF ("   Max sg tablesize       =  %d\n", pp->sg_tblsize);
	SPRINTF ("   Max cmnd number        =  %d\n", pp->cmnd_num);
	SPRINTF ("   Minus timeout          =  %d\n", pp->minus_tout);
	SPRINTF ("   Scsi allowed           =  %d\n", pp->scsi_allowed);
	SPRINTF ("   Max cmd per lun        =  %d\n", pp->cmd_per_lun);
	SPRINTF ("   Max sectors            =  %d\n", pp->max_sectors);
	SPRINTF ("   Target reset flag      =  %d\n", pp->enable_tgtrst);
	SPRINTF ("   Mailbox Time-Out Retry =  %d\n", pp->to_reset_retry);		/* FCLNX-GPL-349 */
	SPRINTF ("   Login retry count      =  %d\n", pp->login_retry);					/* FCLNX-GPL-0343 */
	SPRINTF ("   Ioctl scsi cmd timeout =  %d\n", pp->ioctl_scsi_timeout);			/* FCLNX-GPL-0343 */
	SPRINTF ("   Els retry count        =  %d\n", pp->els_retry);					/* FCLNX-GPL-0343 */
	SPRINTF ("   Limit Log              =  %d\n", pp->limit_log);			/* FCLNX-GPL-491 */
	SPRINTF ("   Filtering Login Target =  %d\n", pp->filter_target);		/* FCLNX-GPL-491 */
	SPRINTF ("   Link Init Timer        =  %d\n", pp->link_initialize_tmo);
	SPRINTF ("   Abort Restrain         =  %d\n", pp->abort_t_restrain);		/* FCLNX-GPL-FX-112 */
	SPRINTF ("   Target Restrain        =  %d\n", pp->tgtrst_restrain);			/* FCLNX-GPL-FX-112 */
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) ) {
		SPRINTF ("   Disable statistics     =  %d\n", pp->hg_stats_disable);	/* FCLNX-GPL-494 */
	}
	if ( pp->ctl_change_qdepth ) {
		SPRINTF ("   Change qdepth param.   =  %d\n", pp->ctl_change_qdepth);	/* FCLNX-GPL-574 */
	}
	if ( pp->rport_lu_scan ) {
		SPRINTF ("   Lu scan parameter      =  %d\n", pp->rport_lu_scan);		/* FCLNX-GPL-575 */
	}
	if( pp->core_deg_mode ){
		SPRINTF ("   Core degradation mode  =  %d\n", pp->core_deg_mode);
	}
	
	if( pp->max_io ){
		SPRINTF ("   Max I/O per core       =  %d\n", pp->max_io);				/* FCLNX-GPL-FX-147 */
	}
	
	if( pp->cpu_map == HFC_VEC_CPU_MAP_ENABLE )	/* FCLNX-GPL-FX-420 */
	{
		SPRINTF ("   Cpu map                =  ENABLE\n");
	}											/* FCLNX-GPL-FX-420 */
	else{
		SPRINTF ("   Cpu map                =  DISABLE\n");
	}											/* FCLNX-GPL-FX-420 */
	
#if _HFC_DEBUG_DET_00
	SPRINTF ("   Seg info trace mode    =  %d\n", pp->fw_parm);
#endif

#ifdef HFC_STAR
	SPRINTF ("   Log File Select        =  %d\n", pp->log_file); /* FCLNX-GPL-547 */
#endif /* HFC_STAR */

	/*SPRINTF ("   Enable failover       = no \n");  */				/* FCLNX_011 */
	SPRINTF ("   Total number of the     \n");
	SPRINTF ("        target devices    =  %d \n", pp->target_cnt);

	SPRINTF ("   Current adapter status =  %08x (",pp->status);				/* FCLNX_011 */
	if(test_bit(HFC_PS_ENABLE, (ulong *)&pp->status)) SPRINTF ("enable ");		/* FCLNX_011 */
	if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)) SPRINTF ("online ");		/* FCLNX_011 */
	if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2)) SPRINTF ("check stop");/* FCLNX_011 */
	SPRINTF (")\n");														/* FCLNX_011 */

	SPRINTF ("   status detail1         =  %08x\n",pp->status_detail1);				/* FCLNX_011 */
	SPRINTF ("   status detail2         =  %08x\n",pp->status_detail2);				/* FCLNX_011 */
	
	/* FCLNX-GPL-147*/
	SPRINTF ("   FC Port Status         =  ");								/* FCLNX_0538 */
	switch(hfc_fx_get_adap_status(pp)){	/* FCLNX-GPL-428 */
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
	
	SPRINTF ("   Path Change Queue Count =  %08x   \n",pp->retry_hfcp_count);
	
	SPRINTF ("   Current interrupt type = ");								/* FCLNX-GPL-FX-272 */
	switch(pp->msi_flag){
		case HFC_INT_TYPE_INTX:
			SPRINTF ("Legacy Mode");
			break;
		case HFC_INT_TYPE_MSI:
			SPRINTF ("MSI Mode");
			break;
		case HFC_INT_TYPE_MSI_SHORTAGE:
			SPRINTF ("MSI Mode");											/* FCLNX-GPL-FX-160 */
			break;
		case HFC_INT_TYPE_MSIX:
			SPRINTF ("MSI-X Mode");
			break;
		case HFC_INT_TYPE_MSIX_SHORTAGE:
			SPRINTF ("MSI-X Mode\n");										/* FCLNX-GPL-FX-160 */
			break;
		case HFC_INT_TYPE_MSIX_MULTI:
//			SPRINTF ("MSI-X Multiqueue Mode");
			SPRINTF ("MSI-X Mode\n");										/* FCLNX-GPL-FX-203 */
			break;
		default:
			SPRINTF ("Invalid");
			break;
	}
	SPRINTF ("\n");

	SPRINTF ("  \n");
	SPRINTF ("  ECID Information \n");
	for(i=0; i<4; i++){
		SPRINTF ("   ");
		for(j=0; j<4; j++){
			HFC_4B_TO_4L(ecid_wk, pp->ecid[i*4+j]);
			SPRINTF("%08x", ecid_wk);
		}
		SPRINTF ("\n");
	}

	SPRINTF ("  \n");
	if( proc_type == HFC_PROC_INFO_TYPE ){
		SPRINTF ("  Device Information \n");
		i=0;
		
		for(j=0; j<(pp->max_target); j++){
		
			tp = pp->target_arg[j];
			while( tp != NULL){

				SPRINTF ("    target id [");
				if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
					SPRINTF("%d",tp->target_id);
				}
				else{ 
					SPRINTF("-");
				}
				SPRINTF ("] :");
				if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
					SPRINTF (" port name = %llx node name = %llx DID = %06llx ",
						(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
				}
				else{
					SPRINTF (" port name =    -    node name =   -     DID = %06llx",(unsigned long long)tp->scsi_id );                            
				}
				
				i++;
				
				SPRINTF (" (pseq = %02d flags=%08x status=%08x core_stat=%08x)\n", tp->pseq, tp->flags, tp->status, tp->tgt_core_stat.all);
				
				SPRINTF ("      LUN Information \n");
				for(i=0; i<MAX_DEV_CNT; i++) {
					dp = tp->dev;
					while( dp != NULL) {
						if( (test_bit(HFC_DEVINF_VALID,(ulong *)&dp->flags)) &&
							(dp->lun == i) ) {
							SPRINTF("      lun%03d: flags=%02x lustat=%08x core_stat=%08x\n",dp->lun, dp->flags, dp->lustat, dp->dev_core_stat.all);
						}
						dp = dp->next;
					}
				}
				tp = tp->next;
			}
		}

		SPRINTF ("\n");
	}
	
	SPRINTF ("  FC persistent binding information \n");
	SPRINTF ("   automap is ");
	if(pp->automap == 1){
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
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		data = hfc_manage_info.npubp->hfc_fx_proc_info_option(pp, data, offset, length, &pos);
	}
	else
	{
		SPRINTF ("   HFC-PCM                       = OFF\n");
		if (hfc_manage_info.hfcplus_enable)                /* FCLNX-0642 */
                        SPRINTF("   E-Option                      = ON\n");             /* FCLNX-0642 */
	}
	
	/* for Debug Mode */ /* FCLNX-GPL-236 */
	if( pp->debug_func != 0x00 )
	{
		SPRINTF("\n");
		SPRINTF("  Debug information \n");
		SPRINTF("   Debug mode           = 0x%x\n", pp->debug_func);
	}
	/* for Debug Mode(mem leak check) */
	if( pp->debug_func & HFC_DEBUG_MEM_LEAK )
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
	
	if( pp->issue_d3hot != 0 )	/* FCLNX-GPL-306 */
	{
		SPRINTF("\n");
		SPRINTF("   Issue D3 Hot          = ON\n");
	}								/* FCLNX-GPL-306 */
	
	/* FCLNX-GPL-393 */
	SPRINTF ("   Isolate setting of HBA port:\n");
	if( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
		SPRINTF ("    FW Support Bit                = On\n");
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support)){
			SPRINTF ("    HVM Support Bit               = On\n");
		}
		else{
			SPRINTF ("    HVM Support Bit               = OFF\n");		
		}
	}
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) ){
		
		status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
		SPRINTF ("    Current Hyper status          = %08x\n",status);
		status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
		SPRINTF ("    Current LPAR status           = %08x\n",status);
		status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
		SPRINTF ("    Current HVM SUPPORT bit       = %08x\n",status);
		status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		SPRINTF ("    Current DRV SUPPORT bit       = %08x\n",status);
		
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				status = (uint)hfc_fx_read_hg_reg_core(pp, i, (uint)HFC_IOHGSPC_HYP_STATUS0,
						(char)0x4, HFC_FX_CORE_OFFSET40);
				SPRINTF ("    Current Hyper status(core%d)   = %08x\n",i,status);
			}
		}
	}
	/* FCLNX-GPL-393 */
	
	if(((pp->ld_err_limit_s)||(pp->if_err_limit )||(pp->to_err_limit )||(pp->rt_err_enable)
	||(pp->total_abort_to)||(pp->total_tgtrst_to))
	 && (!(hfc_manage_info.hfcldd_mp_mod))){	/* FCLNX-GPL-349 *//* FCLNX-GPL-434 *//* FCLNX-GPL-FX-014 */
		SPRINTF ("   Isolate setting of HBA port:\n");
		if (pp->hba_isolation == HFC_ISOL_START){						/* FCLNX-GPL-393 */
			SPRINTF ("    HBA Isolation                = On\n");
		}
		else{
			SPRINTF ("    HBA Isolation                = Off\n");
		}																/* FCLNX-GPL-349 */
		if (pp->ld_err_limit_s) {
			SPRINTF ("    Linkdown Error(S)            Limit:%d  Count:%d\n", pp->ld_err_limit_s, pp->ld_err_count_s);
		}
		if (pp->if_err_limit ) {
			SPRINTF ("    Interface Error              Limit:%d  Count:%d\n", pp->if_err_limit, pp->if_err_count);
		}
		if (pp->to_err_limit ) {
			SPRINTF ("    TimeOut Error                Limit:%d  Count:%d\n", pp->to_err_limit, pp->to_err_count);
		}
		if (pp->rt_err_enable) {
			if(pp->c_err == HFC_ISOLATE_RT){
				SPRINTF ("    TimeOutReset Error           = Enable (Error occurred)\n");
			}
			else{
				SPRINTF ("    TimeOutReset Error           = Enable (No error)\n");
			}
		}
			
		if((pp->connect_type == HFC_FX_SWITCH)||((pp->connect_type == HFC_FX_AL) && (pp -> scsi_id & 0x00ffff00))) {
			for(i=0; i<(pp->max_target); i++){	
				target = hfc_fx_hash_target_valid(pp, i);
				if( target != NULL){
					if (pp->ld_err_limit_s ){
						SPRINTF ("   Isolate setting of Target port(Target: %d,  WWPN: %llx):\n", target->target_id, (unsigned long long)target->ww_name);
						SPRINTF ("    Linkdown Error(S)            Limit:%d  Count:%d\n", pp->ld_err_limit_s, target->tgt_ld_err_count_s);
					}
				}
			}
		}
		if (pp->total_abort_to ) {	/* FCLNX-GPL-FX-014 */
			SPRINTF ("    Total Abort Timer            = %d\n", pp->total_abort_to);
		}
		if (pp->total_tgtrst_to ) {
			SPRINTF ("    Total Target Reset Timer     = %d\n", pp->total_tgtrst_to);
		}	/* FCLNX-GPL-FX-014 */
	}																							/* FCLNX-GPL-349 */

#ifdef HFC_HVM_DEBUG
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) ) {
		if(pp->hg_cca_p != NULL){
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				SPRINTF ("-- HG CCA Statisitcs -- \n");
				SPRINTF ("   version          =  0x%x\n", pp->hg_cca_p[i].version);
				SPRINTF ("   valid            =  0x%x\n", pp->hg_cca_p[i].valid);
				SPRINTF ("   size             =  0x%x\n", pp->hg_cca_p[i].size);
				SPRINTF ("   uni_cnt          =  0x%x\n", pp->hg_cca_p[i].uni_cnt);
				SPRINTF ("   rid              =  0x%x\n", pp->hg_cca_p[i].rid);
				SPRINTF ("   cnum             =  0x%x\n", pp->hg_cca_p[i].cnum);
				SPRINTF ("   statistics_cnt   =  %lld\n", pp->hg_cca_p[i].statistics_cnt);
				SPRINTF ("   io_exec          =  %lld\n", pp->hg_cca_p[i].io_exec);
				SPRINTF ("   io_end           =  %lld\n", pp->hg_cca_p[i].io_end);
				SPRINTF ("   io_exec          =  %lld\n", pp->hg_cca_p[i].io_exec);
				SPRINTF ("   io_err           =  %lld\n", pp->hg_cca_p[i].io_err);
				SPRINTF ("   xob_full         =  %lld\n", pp->hg_cca_p[i].xob_full);
				SPRINTF ("   iov_full         =  %lld\n", pp->hg_cca_p[i].iov_full);
				SPRINTF ("   frame_full       =  %lld\n", pp->hg_cca_p[i].frame_full);
				SPRINTF ("   page_over        =  %lld\n", pp->hg_cca_p[i].page_over);
				SPRINTF ("   tx_frame         =  %lld\n", pp->hg_cca_p[i].tx_frame);
				SPRINTF ("   tx_word          =  %lld\n", pp->hg_cca_p[i].tx_word);
				SPRINTF ("   rx_frame         =  %lld\n", pp->hg_cca_p[i].rx_frame);
				SPRINTF ("   rx_word          =  %lld\n", pp->hg_cca_p[i].rx_word);
				
				
				SPRINTF ("-- HG CCA DUMP start -- \n");
				
				size = sizeof(struct hg_cca_fx);
				tmp_cca = NULL;
				tmp_cca = &pp->hg_cca_p[i];
			
				if(tmp_cca != NULL){
					for ( j = 0; j < ( size / 8 ); j+=8 ) {
						SPRINTF ("%02x%02x%02x%02x%02x%02x%02x%02x\n",
							tmp_cca[j],tmp_cca[j+1],tmp_cca[j+2],tmp_cca[j+4],
							tmp_cca[j+4],tmp_cca[j+5],tmp_cca[j+6],tmp_cca[j+7]);
					}
				}
			}
		
		SPRINTF ("-- HG CCA DUMP end -- \n");
		}
	}
#endif
	
	SPRINTF ("\n");
	SPRINTF ("  NPIV Information\n");
	if (HFC_FX_NPIV_ENABLE(pp)) {
		SPRINTF ("   NPIV mode                     = 0x%02x\n", pp->pport->npiv_mode);
		
		if (HFC_FX_PHYSICAL_PORT(pp) && HFC_FX_VPORT_EXIST(pp) && !HFC_FX_MQ_ENABLE(pp)) {
			SPRINTF ("\n");
			SPRINTF ("   Virtual ports list on this physical port:\n");
			for (i=1; i<=pp->max_vport_count; i++) {
				wkpp = pp->vport_ptr[i].vport_arg;
				if (wkpp == NULL)
					continue;
				
				wkrp = pp->region_arg[wkpp->rid];
					if (wkrp == NULL)
					continue;
				
				SPRINTF ("   vport_no=0x%02x(0x%02x:0x%02x), port name=%llx, node name=%llx\n",
					wkpp->vport_id, wkpp->sub_rid, wkpp->rid, (unsigned long long)wkpp->ww_name, (unsigned long long)wkpp->node_name );
			}
		}
		else if (HFC_FX_VIRTUAL_PORT(pp)) {
			SPRINTF ("\n");
			SPRINTF ("   Virtual port:\n");
			SPRINTF ("   vport_no=0x%02x(0x%02x:0x%02x), port name=%llx, node name=%llx\n",
				pp->vport_id, pp->sub_rid, pp->rid, (unsigned long long)pp->ww_name, (unsigned long long)pp->node_name );
		}
	}
	else {
		SPRINTF ("   NPIV mode                     = 0x%02x\n", pp->pport->npiv_mode);
	}
	
	if( HFC_FX_MQ_ENABLE(pp)) {		/* FCLNX-GPL-FX-223 */
		SPRINTF ("\n");
		SPRINTF ("  Multi Queue Information\n");
		SPRINTF ("   Multi Queue mode              = 0x%02x\n", pp->pport->mq_mode);
		if (HFC_FX_MQ_VALID(pp)) {
			SPRINTF ("   Multi Queue number            = 0x%02x\n", pp->pport->vport_num+1);
		}
		else {
			SPRINTF ("   Multi Queue number            = 0x01\n");
		}
		SPRINTF ("   Vector number                 = 0x%02x\n", pp->pport->vector_num);
		
		if (HFC_FX_MQ_VALID(pp)) {
			SPRINTF ("\n");
			SPRINTF ("   Multi Queue list on this physical port:\n");
			for (i=0; i<=pp->max_vport_count; i++) {
				wkpp = pp->vport_ptr[i].vport_arg;
				if (wkpp == NULL)
					continue;
				
				wkrp = pp->region_arg[wkpp->rid];
					if (wkrp == NULL)
					continue;
				
				SPRINTF ("   queue_no=0x%02x, irq=%d\n", wkpp->vport_id, wkpp->pport->intr_entries[i].vector);
			}
		}
	}								/* FCLNX-GPL-FX-223 */
	
	if (HFC_FX_PHYSICAL_PORT(pp)){	/* FCLNX-GPL-FX-201 */
		SPRINTF ("\n");
		SPRINTF ("  Server Information\n");
		SPRINTF ("   Socket number                 = %d\n", hfc_manage_info.socket_num);
		SPRINTF ("   Physical cpu number           = %d\n", hfc_manage_info.cpu_core_num);
		SPRINTF ("   Online cpu number             = %d\n", hfc_manage_info.online_cpu_num);
	}
	
	SPRINTF ("  --- \n");

    /*
     * Calculate start of next buffer, and return value.
     */

	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);

}
/* FCLNX-GPL-564 end */

int
hfc_fx_target_lu_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length)
{
	struct port_info		*pp;
	struct target_info_fx	*tp;
	struct dev_info_fx		*dp=NULL;
	struct region_info		*rp = NULL;
	
	int						i,j;
	int						rtn;
	char					buf[256];
	char					*data = buffer;
	int						len, partial, pos = 0;
	short					vendor_id, device_id, sub_system_id;
	uchar					pre_conf=0;
	int						proc_type = HFC_SYSFS_INFO_TYPE;	/* FCLNX-GPL-FX-483 */
	
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	if(HFC_FX_MMODE_CHECK_MLPF(pp) ){
		rp =  pp->region_arg[pp->rid];
	}else{
		rp =  pp->region_arg[0];
	}
	
	SPRINTF ("Hitachi FIVE-FX(16Gbps) based Fibre Channel to PCIe HBA\n");	/* FCLNX-GPL-FX-218 */
	
	SPRINTF ("  Special file name       = hfcldd%d\n", pp->dev_minor);
	
	SPRINTF ("  Major_number            = %d\n", pp->dev_major);
	SPRINTF ("  Minor_number            = %d\n", pp->dev_minor);			
	SPRINTF ("  Instance_number         = %d\n",pp->instance);						
	SPRINTF ("  Host# = %d, Unique id   = %d \n", host->host_no, host->unique_id);
	
	SPRINTF("  adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_fx_read_cnfg(pp, 0x00, 0x2);
	device_id = (ushort) hfc_fx_read_cnfg(pp, 0x02, 0x2 );
	/* read config register (0x2e) *//* FCLNX-GPL-FX-048 */
	sub_system_id = (ushort)hfc_fx_read_cnfg(pp, HFC_HOST_SUB_SYSTEM_ID, 0x2);
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	SPRINTF ("   Sub_system ID          =  %x\n", sub_system_id);
	SPRINTF ("   Port name              =  %llx \n", (unsigned long long)pp->ww_name);
	SPRINTF ("   Node name              =  %llx \n", (unsigned long long)pp->node_name);
	
	if( pp->pkg.type == HFC_PKTYPE_FIVE )
	{
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xAF, 0x1);
	}
	else /* FIVE-EX */
	{
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xCA, 0x1);
	}
	
	if( (pre_conf == 0x01) || (pre_conf == 0x03) ){
		SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)pp->org_ww_name);
		SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)pp->org_node_name);
		SPRINTF ("   Add port name          =  %llx \n", (unsigned long long)pp->add_ww_name);
		SPRINTF ("   Add node name          =  %llx \n", (unsigned long long)pp->add_node_name);
	}
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) ) {
		SPRINTF ("   MLPF MODE              =  %x \n", pp->mlpf_mode);	
		if ( !(HFC_FX_MMODE_CHECK_DEDICATE(pp)) ) {
			SPRINTF ("   Original port name     =  %llx \n", (unsigned long long)pp->org_ww_name);
			SPRINTF ("   Original node name     =  %llx \n", (unsigned long long)pp->org_node_name);
		}
		SPRINTF ("   VFC port name          =  %llx \n", (unsigned long long)pp->vfc_ww_name);
		SPRINTF ("   VFC node name          =  %llx \n", (unsigned long long)pp->vfc_node_name);
		if ( !(HFC_FX_MMODE_CHECK_DEDICATE(pp)) ) {
			SPRINTF ("   RID                    =  %x \n", pp->rid);
		}
	}
	
	SPRINTF ("   port number            =  %d\n", pp->port_no);
	
	SPRINTF ("  \n");
	SPRINTF ("  Device Information \n");
	i=0;
	
	for(j=0; j<(pp->max_target); j++){
	
		tp = pp->target_arg[j];
		while( tp != NULL){

			SPRINTF ("    target id [");
			if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
				SPRINTF("%d",tp->target_id);
			}
			else{ 
				SPRINTF("-");
			}
			SPRINTF ("] :");
			if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
				SPRINTF (" port name = %llx node name = %llx DID = %06llx ",
					(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
			}
			else{
				SPRINTF (" port name =    -    node name =   -     DID = %06llx",(unsigned long long)tp->scsi_id );                            
			}
			
			i++;
			
			SPRINTF (" (pseq = %02d flags=%08x status=%08x core_stat=%08x)\n", tp->pseq, tp->flags, tp->status, tp->tgt_core_stat.all);
			
			SPRINTF ("      LUN Information \n");
			for(i=0; i<MAX_DEV_CNT; i++) {
				dp = tp->dev;
				while( dp != NULL) {
					if( (test_bit(HFC_DEVINF_VALID,(ulong *)&dp->flags)) &&
						(dp->lun == i) ) {
						SPRINTF("      lun%03d: flags=%02x lustat=%08x core_stat=%08x\n",dp->lun, dp->flags, dp->lustat, dp->dev_core_stat.all);
					}
					dp = dp->next;
				}
			}

			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				data = hfc_manage_info.npubp->hfc_fx_proc_info_lun(pp, tp, data, offset, length, &pos);
			}	/* FCLNX-GPL-FX-483 */

			tp = tp->next;
		}
	}

	SPRINTF ("\n");
	
    /*
     * Calculate start of next buffer, and return value.
     */

	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);

}

int
hfc_fx_target_status_info_com(struct Scsi_Host *host, char *buffer, off_t offset, int length)
{
	struct port_info		*pp;
	struct target_info_fx	*tp;
	struct dev_info_fx		*dp=NULL;
	struct region_info		*rp = NULL;
	
	int						i, j;
	int						rtn;
	char					buf[256];
	char					*data = buffer;
	int						len, partial, pos = 0;
	short					vendor_id, device_id, sub_system_id;
	int 					selected_target_id = 0;
	int						proc_type = HFC_SYSFS_INFO_TYPE;	/* FCLNX-GPL-FX-483 */
	
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL){
		return 0;
	}
	if(HFC_FX_MMODE_CHECK_MLPF(pp) ){
		rp =  pp->region_arg[pp->rid];
	}else{
		rp =  pp->region_arg[0];
	}
	
	SPRINTF ("Hitachi FIVE-FX(16Gbps) based Fibre Channel to PCIe HBA\n");	/* FCLNX-GPL-FX-218 */
	
	SPRINTF ("  Special file name       = hfcldd%d\n", pp->dev_minor);
	SPRINTF ("  adapter information \n");
	
	/* read config register (0x00 - 0x03) */
	vendor_id = (ushort) hfc_fx_read_cnfg(pp, 0x00, 0x2);
	device_id = (ushort) hfc_fx_read_cnfg(pp, 0x02, 0x2 );
	/* read config register (0x2e) *//* FCLNX-GPL-FX-048 */
	sub_system_id = (ushort)hfc_fx_read_cnfg(pp, HFC_HOST_SUB_SYSTEM_ID, 0x2);
	
	SPRINTF ("   Vender ID              =  %x\n", vendor_id);
	SPRINTF ("   Device ID              =  %x\n", device_id);
	SPRINTF ("   Sub_system ID          =  %x\n", sub_system_id);	
	SPRINTF ("  \n");
	SPRINTF ("  Device Information \n");
	
	selected_target_id = atomic_read(&pp->selected_target_id);
    
    if (selected_target_id < 0){
    	return -EINVAL;
    }
    
    if (selected_target_id >= MAX_TARGET_PROBE){
    	return -EINVAL;
    }
	
	tp = hfc_fx_hash_target_valid(pp, selected_target_id);
	if( tp != NULL){

		SPRINTF ("    target id [");
		if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
			SPRINTF("%d",tp->target_id);
		}
		else{ 
			SPRINTF("-");
		}
		SPRINTF ("] :");
		if(test_bit(HFC_TF_WWN_VALID, (ulong *)&tp->flags)){
			SPRINTF (" port name = %llx node name = %llx DID = %06llx ",
				(unsigned long long)tp->ww_name, (unsigned long long)tp->node_name, (unsigned long long)tp->scsi_id );
		}
		else{
			SPRINTF (" port name =    -    node name =   -     DID = %06llx",(unsigned long long)tp->scsi_id );                            
		}
			
			
		SPRINTF (" (pseq = %02d flags=%08x status=%08x core_stat=%08x)\n", tp->pseq, tp->flags, tp->status, tp->tgt_core_stat.all);
		
		SPRINTF ("      LUN Information \n");
		j=0;
		for(i=0; i<MAX_DEV_CNT; i++) {
			dp = tp->dev;
			while( dp != NULL) {
				if( (test_bit(HFC_DEVINF_VALID,(ulong *)&dp->flags)) &&
					(dp->lun == i) ) {
						if( j < 0x30 ){
							SPRINTF("      lun%03d: flags=%02x lustat=%08x core_stat=%08x\n",dp->lun, dp->flags, dp->lustat, dp->dev_core_stat.all);
							j++;
						}
				}
				dp = dp->next;
			}
		}
		if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
			data = hfc_manage_info.npubp->hfc_fx_proc_info_lun(pp, tp, data, offset, length, &pos);
		}	/* FCLNX-GPL-FX-483 */
	}

	SPRINTF ("\n");
	
    /*
     * Calculate start of next buffer, and return value.
     */

	/* Calculate  return value. */
	rtn = pos > offset ? pos - offset : 0;

	return (rtn);
}


/* FCLNX-GPL-564 start */
/*
 * Function:    hfc_fx_proc_info_k26
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
 *    >=0       - Data length
 * Notes:
 */
int
hfc_fx_proc_info_k26(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
{
	if (!host){ /* This host is not found */
		return (-EINVAL);
	}
	if (inout) {
		return (length);
	}
	if (start) {
		*start = buffer;
	}
	
	return hfc_fx_proc_info_com(host, buffer, offset, length, HFC_PROC_INFO_TYPE);
}
/* FCLNX-GPL-564 end */


void hfc_fx_conf_setup(struct port_info *pp)
{

//	hfc_fx_set_topology(pp);			/* get topology					pp->topology */
//	hfc_fx_set_linkspeed(pp);			/* get linkspeed				pp->linkspeed */
	hfc_fx_set_max_transfer(pp);		/* get max tarnsfer length		pp->max_transfer, pp->dma_max */
//	pp->iov_map_cnt = ap->dma_max / HFC_PAGE_SIZE;

	hfc_fx_set_linkup_tmo(pp); 			/* get linkup timeout time		pp->linkup_tmo */
//	hfc_fx_set_linkup2_tmo(pp);			/* get linkup timeout time after MCK	pp->linkup2_tmo */
	hfc_fx_set_reset_delay(pp);			/* get reset delay				pp->scsi_reset_delay */
	hfc_fx_set_mck_count(pp);			/* get machine check count		pp->max_mck_cnt */

	hfc_fx_set_pref_alpa(pp);			/* get preferred alpa			pp->pref_alpa */
	pp->host_alpa = pp->pref_alpa;

	hfc_fx_set_target_timeout(pp); 		/* get target reset timeout 	pp->target_reset_tmo */
	hfc_fx_set_abort_timeout(pp);		/* get abort timeout			pp->abort_tmo*/

//	hfc_fx_set_seg_trace(pp);			/* get seg info trace mode		pp->fw_parm */
	hfc_fx_set_queue_depth(pp);			/* get queue_depth 				pp->queue_depth */
	hfc_fx_set_enable_target_reset(pp);	/* get enable target reset mode pp->enable_tgtrst */

	hfc_fx_set_max_target(pp);			/* max target number			pp->max_target */
	hfc_fx_set_xob_max(pp);				/* xob max						pp->xob_max */
	hfc_fx_set_xrb_max(pp);				/* xrb max						pp->xrb_max */
	hfc_fx_set_slog_max(pp);			/* softlog max					pp->slog_max */
	hfc_fx_set_trc_max(pp);				/* trace max					pp->trc_max */
	hfc_fx_set_pkt_num(pp);				/* hfc_pkt max					pp->pkt_num */
	hfc_fx_set_rsv_pkt_num(pp);			/* hfc_rsv_pkt max				pp->rsv_pkt_num */
	hfc_fx_set_pm_pkt_num(pp);			/* hfc_pm_pkt max				pp->pm_pkt_num */
	hfc_fx_set_can_queue(pp);			/* can queue depth				pp->can_queue */
	hfc_fx_set_sg_tblsize(pp);			/* sg table size				pp->sg_tablesize */
	hfc_fx_set_cmnd_num(pp);			/* cmnd num						pp->cmnd_num */
	hfc_fx_set_minus_tout(pp);			/* minus timeout				pp->minus_timeout */
	hfc_fx_set_scsi_allowed(pp);		/* Scsi_Cmnd->allowed 			pp->scsi_allowed */
//	hfc_fx_set_login_retry(pp);			/* mailbox login retry 			pp->login_retry */		/* FCLNX-GPL-0343 */
//	hfc_fx_set_els_retry(pp);			/* mailbox els retry 			pp->els_retry */		/* FCLNX-GPL-0343 */
	hfc_fx_set_ioctl_scsi_timeout(pp);	/* ioctl timeout period 		pp->ioctl_scsi_timeout */	/* FCLNX-GPL-0343 */
	hfc_fx_set_cmd_per_lun(pp);			/* cmd_per_lun					pp->cmd_per_lun */
	hfc_fx_set_max_sectors(pp);			/* max_sectors					pp->max_sectors */
	hfc_fx_set_lun_reset_delay(pp);		/* Delay after LUN Reset 		pp->tmt_delay			*/ /* FCLNX-0506 */	/* FCLNX-GPL-038 */
	hfc_fx_set_abort_t_restrain(pp);	/* Restrain of AbortT.S issue	pp->abort_t_restrain	*/ /* FCLNX-0506 */
//	hfc_fx_set_login_restrain(pp);		/* Restrain of Login isuue		pp->login_restrain		*/ /* FCLNX-0506 */
	hfc_fx_set_tgtrst_restrain(pp);		/* Restrain of Target Reset		pp->tgtrst_restrain		*/
	hfc_fx_set_mck_point(pp);  /* FCLNX-0533 */
	hfc_fx_set_pcie_sram_ce_count(pp);	/* Max PCIe IP Core SRAM ERR(CE) Count	pp->max_pcie_sram_ce_cnt	*/
	hfc_fx_set_core_ce_count(pp);		/* Max Core ERR(CE) Count				pp->max_core_ce_cnt			*/
	hfc_fx_set_msi_enable(pp);			/* INT type(INTx or MSI or MSI-X)		pp->msi_enable*/
	hfc_fx_set_inta_dummy_read(pp);		/* "1:Do" or "0:Not" dummy read(for MSI/MSI-X) pp->inta_dummy_read */
	hfc_fx_set_max_hwlog_cnt(pp);		/* Set the number of max HW log page count.(0 - 16) */
	hfc_fx_set_debug_func(pp);			/* Set Debug mode */
	hfc_fx_set_issue_d3hot(pp);			/* Set Issue D3 Hot */	/* FCLNX-GPL-306 */
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_fx_set_sysfs_control(pp);		/* Set rport control */
	hfc_fx_set_dev_loss_tmo(pp);		/* Set dev_loss_tmo  */	/* FCLNX-GPL-260 */
	hfc_fx_set_scan_finished_tmo(pp);	/* Set scan_finished time out */ /* FCLNX-GPL-565 *//* FCLNX-GPL-575 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	hfc_fx_set_hba_isolation(pp);		/* Set HBA isolation	 		pp->hba_isolation	*/	/* FCLNX-GPL-349 */
	if (!hfc_manage_info.hfcldd_mp_mod)				/* FCLNX-GPL-349 */
	{
		hfc_fx_set_ld_err_limit_s(pp);	/* Set Linkdown(s) Limit 		pp->ld_err_limit_s	*/
		hfc_fx_set_if_err_limit(pp);	/* Set interface Error Limit 	pp->if_err_limit	*/
		hfc_fx_set_to_err_limit(pp);	/* Set Time-Out Error Limit		pp->to_err_limit	*/
//		hfc_fx_set_to_reset_retry(pp);	/* Set Mailbox Time-Out Retry	pp->to_reset_retry	*/
		hfc_fx_set_rt_err_enable(pp);	/* Set Time-Out Reset Error		pp->rt_err_enable	*/
	}																						/* FCLNX-GPL-349 */

	hfc_fx_set_limit_log(pp);			/* Set limit log				pp->limit_log */		/* FCLNX-GPL-491 */
//	hfc_fx_set_filter_target(pp);		/* Set filter target			pp->filter_target */	/* FCLNX-GPL-491 */
	hfc_fx_set_hg_stats_disable(pp);	/* Set statistics for Virtage	pp->hg_stats_disable */	/* FCLNX-GPL-494 */
	hfc_fx_set_rport_lu_scan(pp);		/* Set rport lu scan control ap->rport_lu_scan	 	*/	/* FCLNX-GPL-575 */

	hfc_fx_set_ctl_change_qdepth(pp);	/* Set change_queue_depth control ap->ctl_change_qdepth *//* FCLNX-GPL-574 */
	
	hfc_fx_set_core_control(pp);		/* Set core control 			pp->core_control		*/
	hfc_fx_set_cc_cnt(pp);				/* Set cc_cnt		 			pp->cc_cnt				*/
	hfc_fx_set_cc_size(pp);				/* Set cc_size		 			pp->cc_size				*/
	hfc_fx_set_cc_core(pp);				/* Set cc_core		 			pp->cc_core				*/
	hfc_fx_set_link_reset(pp);			/* Set link reset				pp->link_reset			*/
	hfc_fx_set_vport_count(pp);			/* Set max vport count			pp->max_vport_count		*/
	hfc_fx_set_frame_count(pp);			/* Set max frame count			pp->max_frame_count		*/
	hfc_fx_set_mq_num(pp);				/* Set max mq count				pp->mq_num				*/
	hfc_fx_set_rdtsc(pp);				/* Set rdtsc					pp->pm_control			*/
	hfc_fx_set_intdisable(pp);			/* Set intdisable				pci memory 0x23e		*/
	hfc_fx_set_intenable(pp);			/* Set intenable				pci memory 0x23f		*/
	hfc_fx_set_total_tgtrst_to(pp);		/* Set target reset process total timer		pp->total_tgtrst_to	*//* FCLNX-GPL-FX-014 */
	hfc_fx_set_total_abort_to(pp);		/* Set abort process total timer			pp->total_abort_to	*//* FCLNX-GPL-FX-014 */
	if(!HFC_FX_MMODE_CHECK_SHADOW(pp)){
		if (HFC_FX_PHYSICAL_PORT(pp)) {
			hfc_fx_set_npiv_enable(pp);	/* Set NPIV mode 				pp->npiv_mode			*//* FCLNX-GPL-FX-137 */
			hfc_fx_set_mq_enable(pp);	/* Set MQ mode 					pp->mq_mode				*/
		}
	}else{
		pp->npiv_mode = HFC_NPIV_ENABLE;
	}
	hfc_fx_set_max_io(pp);				/* FCLNX-GPL-FX-147 */
	
	hfc_fx_set_login_seq_retry_cnt(pp); /* FCLNX-GPL-FX-446 */
	
	hfc_fx_set_log_file(pp);			/* Set a choice of log files */ /* FCLNX-GPL-547,563 */
	hfc_fx_set_max_lun(pp);				/* Set max lu number */ /* FCLNX-GPL-547,563 */

	hfc_fx_set_cpu_map(pp);
	
	pp->wmsg = hfc_message_enable;
	pp->errlog_max	= HFC_MAX_ERRLOG_CNT;

	pp->scsi_time_out = hfc_scsi_time_out;

	/* originally set by modules.conf. This part is still tentative */
	pp->hosts->max_id 		= 255; 		/* modules.conf ??? */
	pp->hosts->max_lun 		= pp->max_lun; /* the number of lun is more than 256 *//* FCLNX-GPL-343,547,563 */
	pp->hosts->max_channel 	= 1;
	pp->hosts->max_cmd_len 	= 16;	
	pp->hosts->this_id 		= 255;
	pp->hosts->can_queue 	= pp->can_queue;
	pp->hosts->n_io_port		= 0xff;
	pp->hosts->base		= (unsigned long)pp->mem_base_addr;
	pp->hosts->cmd_per_lun  = pp->cmd_per_lun;		/* FCLNX-283 */
	pp->hosts->sg_tablesize  = pp->sg_tblsize;		/* FCLNX-283 */

	pp->hosts->max_cmd_len = 255;

	HFC_EXIT("hfc_fx_conf_setup");

	return;	
}


int hfc_fx_search_adapter_number(struct port_info *pp)
{
	int i;
	uchar buf[64];
	uint wk;
	uchar pre_conf;
	uchar err_wwpn[16];		/* FCLNX-GPL-150 */
	uchar addr[4];																		/* FCLNX-GPL-319 */

	uint64_t pxe_add_ww_name;	/* FCLNX-XXX */
	uint64_t swap_add_ww_name;	/* FCLNX-GPL-180 */
	
	HFC_ENTRY("hfc_fx_search_adapter_number");
	
	HFC_DBGPRT("pkg code=%d , port = %d \n", pp->pkg.code, pp->pkg.port );

 	if(pp->pkg.type == HFC_PKTYPE_FPP){ /* FPP? */
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0x4B, 0x1);
 	}
 	else if(pp->pkg.type == HFC_PKTYPE_FIVE){ /* FIVE */
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xAF, 0x1);
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE_EX){ /* FIVE-EX */
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xCA, 0x1);
	}
	else{ /* FIVE-FX */
		pre_conf = (uchar) hfc_fx_read_cnfg (pp, 0xCA, 0x2);
	}

	HFC_DBGPRT("function number = %d\n",pp->port_no);
		
	if ( HFC_FX_MMODE_CHECK_BASIC(pp) )
	{
		if( hfc_fx_read_flash(pp, 0x54, 4, addr)){										/* FCLNX-GPL-319 */
			return (-1);
		}
		HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
		if(hfc_fx_read_flash(pp, wk+16*pp->port_no, 16, buf)){
			return(-1);
		}																				/* FCLNX-GPL-319 */
		/* Read WWPN and WWNN from flash. */
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
		pp->ww_name = wk;
		pp->ww_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
		pp->ww_name |= wk;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[8])));
		pp->node_name = wk;
		pp->node_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[12])));
		pp->node_name |= wk;
		
		HFC_DBGPRT("ww_name=%llx, node_name=%llx\n",(unsigned long long)pp->ww_name, (unsigned long long)pp->node_name);
		
		pp->org_ww_name = pp->ww_name;
		pp->org_node_name = pp->node_name;
		
		/*** Restore this HBA's slot_no ***/
		/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
		swap_add_ww_name = hfc_fx_restore_add_wwn(pp);
		
		/*** check add_ww_name ***/
		if(swap_add_ww_name != 0)
		{
			/* restore "add_ww_name" */
			pp->add_ww_name   = swap_add_ww_name;
			pp->add_node_name = swap_add_ww_name +1;
			pp->ww_name       = pp->add_ww_name;
			pp->node_name     = pp->add_node_name;
			
			/* Set pre_conf bit */ /* FCLNX-GPL-269 start */
		 	if(pp->pkg.type == HFC_PKTYPE_FPP)
		 	{	/* FPP */
				hfc_fx_write_cnfg(pp, 0x4B, 0x1, 0x01);
 			}
		 	else if(pp->pkg.type == HFC_PKTYPE_FIVE)
		 	{	/* FIVE */
				hfc_fx_write_cnfg(pp, 0xAF, 0x1, 0x01);
			}
			else
			{	/* FIVE-EX */
				hfc_fx_write_cnfg(pp, 0xCA, 0x1, 0x01);
			}
			/* FCLNX-GPL-269 end */
		}
		else /* FCLNX-GPL-180 end */
		{
			if( (pre_conf == 0x01) || (pre_conf == 0x03) ){

				if( hfc_fx_read_flash(pp, 0x20018 + 16*pp->port_no, 8, buf) ){
					return (-1); /* FCLNX-GPL-116 */
				}

				HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
				pp->add_ww_name = wk;
				pp->add_ww_name <<= 32;
				HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
				pp->add_ww_name |= wk;
				pp->add_node_name=pp->add_ww_name + 1;

				if ((pp->add_ww_name == 0) || (pp->add_ww_name == 0xffffffffffffffffLL) ||
					(pp->add_node_name == 0) || (pp->add_node_name == 0xffffffffffffffffLL)) {
					HFC_ERRPRT("hfcldd : 'Additional WWPN is invalid ' orgWWPN = 0x%llx, addWWPN = 0x%llx, addWWNN = 0x%llx\n", 
						(unsigned long long)pp->org_ww_name, (unsigned long long)pp->add_ww_name, (unsigned long long)pp->add_node_name);
					
					HFC_8L_TO_8B(err_wwpn[0], pp->add_ww_name);
					HFC_8L_TO_8B(err_wwpn[8], pp->add_node_name);
					
					hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, 0xD7, err_wwpn, 16);
					
					pp->add_ww_name   = pp->ww_name;
					pp->add_node_name = pp->node_name;
					
					return (MAX_ADAP_CNT + 1);
				}
				else {
					pp->ww_name   = pp->add_ww_name;				/* FCLNX-0299 */
					pp->node_name = pp->add_node_name;				/* FCLNX-0299 */
					
					/*** save "add_ww_name" ***/
					/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
					hfc_fx_backup_add_wwn(pp, pp->add_ww_name);
				}
			}
			else {
				if ((pp->org_ww_name == 0) || (pp->org_ww_name == 0xffffffffffffffffLL) ||
					(pp->org_node_name == 0) || (pp->org_node_name == 0xffffffffffffffffLL)) {
					HFC_ERRPRT("hfcldd : 'Original WWPN is invalid ' orgWWPN = 0x%llx, orgWWNN = 0x%llx\n", 
						(unsigned long long)pp->org_ww_name, (unsigned long long)pp->org_node_name);
					
					HFC_8L_TO_8B(err_wwpn[0], pp->org_ww_name);
					HFC_8L_TO_8B(err_wwpn[8], pp->org_node_name);
					
					hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, 0xD7, err_wwpn, 16);
					
					return (MAX_ADAP_CNT + 1);
				}
			}
		}
	}	
	else {															/* @MLPF */
		hfc_fx_mlpf_setup_wwn(pp);
		
		if ((pp->vfc_ww_name == 0) || (pp->vfc_ww_name == 0xfffffffffffffffLL) ||
			(pp->vfc_node_name == 0) || (pp->vfc_node_name == 0xffffffffffffffffLL))
		{
			HFC_DBGPRT("hfcldd @MLPF: WWN check error \n"); 
			
			HFC_ERRPRT("hfcldd : 'Additional VFC WWPN is invalid ' vfcWWPN = 0x%llx, vfcWWPN = 0x%llx\n", 
				(unsigned long long)pp->vfc_ww_name, (unsigned long long)pp->vfc_node_name);
			
			HFC_8L_TO_8B(err_wwpn[0], pp->vfc_ww_name);
			HFC_8L_TO_8B(err_wwpn[8], pp->vfc_node_name);
			
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERRF, 0xD7, err_wwpn, 16);
			
			return (MAX_ADAP_CNT + 1);
		}
	}																/* @MLPF */

	/* --------------------------------------------------------------------- FCLNX-XXX */
	if(hfc_pxe_boot != 0){

		if( hfc_fx_read_flash(pp, 0x20018 + 16*pp->port_no, 8, buf) ){ /* FCLNX-GPL-116 */
			return (-1);
		}

		HFC_4B_TO_4L(wk, (*(uint*)(&buf[0])));
		pxe_add_ww_name = wk;
		pxe_add_ww_name <<= 32;
		HFC_4B_TO_4L(wk, (*(uint*)(&buf[4])));
		pxe_add_ww_name |= wk;

		if ((pxe_add_ww_name == 0) || (pxe_add_ww_name == 0xffffffffffffffffLL)) { /* Error case */
			pp->ww_name = pp->org_ww_name;
			pp->node_name = pp->org_node_name;
		}

	}
	/* --------------------------------------------------------------------- FCLNX-XXX */


	for(i=0; i<MAX_ADAP_CNT; i++) {
		if(pp->ww_name == hfc_manage_info.adap_bind[i]){
			HFC_DBGPRT("adapter info is hit, i = %d, wwpn=%llx\n", i, (unsigned long long)pp->ww_name);
			return (i); /* hit */
		} 
	}

	HFC_EXIT("hfc_fx_search_adapter_number");
	return (-1); 
}


int hfc_fx_get_adapter_port_no(struct port_info *pp)
{
	int func_no;
	ulong pci_status;
	
	HFC_ENTRY("hfc_fx_get_adapter_port_no");
	
 	if(pp->pkg.type == HFC_PKTYPE_FPP) { /* FPP? */
		pp->port_no = 0; /* set port number to '0' */
 	}
 	else if(pp->pkg.type == HFC_PKTYPE_FIVE) { /* FIVE */
 		pci_status = (ulong) hfc_fx_read_cnfg (pp, 0x6C, 0x4);
 		HFC_DBGPRT("pci status = %x\n",(uint)pci_status);
		func_no = (pci_status & 0x00000007);
		pp->port_no = func_no; 
	}
	else { /* FIVE-EX */ 
		pp->port_no = PCI_FUNC(pp->pci_cfginf->devfn);
		 /* FIVE-EX */
	}
	
	HFC_EXIT("hfc_fx_set_config");
	return (pp->port_no); 
}


int hfc_fx_read_hfcbios(struct port_info *pp)
{
	uchar					buf[512];
	struct flash_param		*fp;
	int						i,j;	/* FCLNX-GPL-FX-377 */
	uint 					bios_ofs;
	uint					wwn_wk;
	ushort					lun_wk;
	uchar					wk;
	uchar					min,max,def;
	uint wkint_bios_parm1;	/* FCLNX-GPL-FX-377 */
	
	bios_ofs = 0xd8000 + (0x200 * pp->port_no);
	
	if ( HFC_FX_MMODE_CHECK_MLPF(pp) ) {
		for(i=0; i<128; i++) {
			wkint_bios_parm1 = (uint)hfc_fx_read_reg_hg_ext(pp,
				pp->lparmode.hg_map->iosp.reg[HFC_IOHGSPC_EFI_OP_TBL0]
				+4*i, 4);
			buf[4*i+3] =  wkint_bios_parm1        & 0xff;
			buf[4*i+2] = (wkint_bios_parm1 >>  8) & 0xff;
			buf[4*i+1] = (wkint_bios_parm1 >> 16) & 0xff;
			buf[4*i]   = (wkint_bios_parm1 >> 24) & 0xff;
		}
		structdump( 0xec, (uchar *)buf, 512 );
	}
	else {
		for(i=0; i<128; i++) {
			if (hfc_fx_read_flash(pp, (bios_ofs+4*i), 4, &buf[4*i])) {
				return(1);
			}
		}
	}
	
	fp = (struct flash_param *)&buf[0];
	
	/* Get Param for OS driver */
	pp->automap         = hfc_automap;
	pp->narrowmap       = hfc_narrowmap;
	pp->defparam        = 0;
	pp->core_deg_mode   = HFC_FX_CORE_DEG_ENABLE;
	
	if ((fp->param_for_os_driver != 0xFF) && (fp->param_for_os_driver != 0x00)) {
		if (fp->param_for_os_driver & 0x80) {
			/* Forced Persistent Binding into disable state */
			pp->automap = 1;
		}
		
		if (fp->param_for_os_driver & 0x40) {
			/* Forced default parameter into disable state */
			pp->defparam = 1;
		}
		
		if (fp->param_for_os_driver & 0x20) {
			/* Core degradation mode */
			pp->core_deg_mode = 1;
		}
	}
	
	/* Get LOGIN delay time */
//#if defined(HFC_X8664_SLES11SP3) || defined(HFC_X8664_SLES12)
//	pp->login_wait = HFC_DEF_LOGIN_WAIT;	/* 0s */
//#else
	pp->login_wait = HFC_FX_LOGIN_DELAY_TO;	/* 3s */
//#endif
	if ((fp->login_delay_time != 0xFF) && (fp->login_delay_time != 0x00)) {
		if (fp->login_delay_time & 0x80) {
			pp->login_wait = fp->login_delay_time & 0x7f;
		}
	}
	
	/* Get Link Speed */
	pp->linkspeed = 0;	/* Auto */
	if (fp->data_rate != 0xFF) {
		if((fp->data_rate == 0x00)		/* Auto   */
		|| (fp->data_rate == 0x02)		/* 2Gbps  */
		|| (fp->data_rate == 0x04)		/* 4Gbps  */
		|| (fp->data_rate == 0x08)		/* 8Gbps  */
		|| (fp->data_rate == 0x10))		/* 16Gbps */
		{
			pp->linkspeed = fp->data_rate;
		}
		else {
			hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD1, NULL, 0) ;
		}
	}
	
	/* Get Connection Type */
	pp->topology = 0;	/* Auto */
	if (fp->connection_type != 0xFF) {
		if((fp->connection_type == 0x00)	/* Auto */
		|| (fp->connection_type == 0x01)	/* Point to Point */
		|| (fp->connection_type == 0x03)	/* Arbitrated Loop */
		|| (fp->connection_type == 0xf0)	/* F_PORT */
		|| (fp->connection_type == 0xf3))	/* Arbitrated Loop(MultiAL-PA) */
		{
			 pp->topology = fp->connection_type;
		}
	}
	
	/* Get Conn_Type_Option */	/* FCLNX-GPL-FX-135*/
	if (fp->Conn_Type_Option & 0x80) {
		pp->multiple_portid = 1;
	}
	
	/* Get Spinup Delay */
	pp->spinup_delay = 0;			/* Disable */
	if (fp->flag != 0xFF) {
		if ((fp->flag & 0x20) == 0x20)
			 pp->spinup_delay = 1;	/* Enable */
	}
	
	/* Get Link Init Timer */
	pp->link_initialize_tmo = HFC_FX_LINKINT_TO;	/* 120s */
	if (fp->link_initialize_timer != 0) {
		pp->link_initialize_tmo = fp->link_initialize_timer;
	}
	
	/* Get MCK LinkUp Timer */
	pp->mck_rcv_tmo = HFC_FX_MCK_RCV_TO;	/* 15s */
	if (fp->MCKLinkup_Timer & 0x80) {
		wk = fp->MCKLinkup_Timer & 0x7f;
		if (hfc_chk_conf_val(0, 60, wk)) {
			pp->mck_rcv_tmo = wk;
		}
	}
	
	/* Get Driver Control FX */
	pp->isol_cmd    = HFC_FX_ISOL_CMD_OFF;
	pp->rft_id_skip = HFC_FX_RFT_ID_SKIP_DISABLE;
	pp->mailbox_force_retry = 0;
	pp->filter_target = HFC_FX_MB_LOGIN_FILTER_ON;
	
	if ((fp->DriverCtrlFX != 0xFF) && (fp->DriverCtrlFX != 0x00)) {
		if (fp->DriverCtrlFX & 0x80) {
			/* HBA_ISOL */
			pp->isol_cmd = HFC_FX_ISOL_CMD_ON;
		}
		
		if (fp->DriverCtrlFX & 0x40) {
			/* RFT_ID_skip */
			pp->rft_id_skip = HFC_FX_RFT_ID_SKIP_ENABLE;
		}
		
		if (fp->DriverCtrlFX & 0x20) {
			/* Mailbox Force Retry */
			pp->mailbox_force_retry = 1;
		}
		
		if (fp->DriverCtrlFX & 0x10) {
			/* LOGIN_Filter */
			pp->filter_target = HFC_FX_MB_LOGIN_FILTER_OFF;
		}
	}
	
	/* Get Virtual Fabric FLAG */
	pp->vf_enable       = HFC_FX_VF_DISABLE;
	pp->vf_mode_tagging = HFC_FX_VF_MODE_TAGGING_AUTO;
	
	if ((fp->VF_Flag != 0xFF) && (fp->VF_Flag != 0x00)) {
		if (fp->VF_Flag & 0x80) {
			/* VF_Enable */
			pp->vf_enable = HFC_FX_VF_ENABLE;
		}
		
		wk = (fp->VF_Flag >> 5) & 0x03;
		if ((wk == HFC_FX_VF_MODE_TAGGING_OFF)	||
			(wk == HFC_FX_VF_MODE_TAGGING_ON) 	||
			(wk == HFC_FX_VF_MODE_TAGGING_AUTO)) {
			/* VF Tagging Mode */
			pp->vf_mode_tagging = wk;
		}
	}
	
	/* Get Peer Password */
	memcpy(pp->peer_password, &fp->PearPassword[0], 40);
	
	/* Get Local Password */
	memcpy(pp->local_password, &fp->LocalPassword[0], 40);
	
	/* Get FC-SP_FLAG */
	pp->security_enable = HFC_FX_SECURITY_DISABLE;
	
	if ((fp->FCSP_Flag != 0xFF) && (fp->FCSP_Flag != 0x00)) {
		if (fp->FCSP_Flag & 0x80) {
			/* Security_Enable */
			pp->security_enable = HFC_FX_SECURITY_ENABLE;
		}
	}
	
	/* Get Mailbox Delay */
	for (i=0; i<25; i++) {
		pp->mb_timer[i].delay = 0;
		
		if (hfc_chk_conf_val(HFC_FX_MB_DELAY_MIN, HFC_FX_MB_DELAY_MAX, fp->mailbox_delay_time[i])) {
			pp->mb_timer[i].delay = fp->mailbox_delay_time[i];
		}
	}
	
	/* Get Mailbox Rsp Timer1 */
	pp->mb_timer[HFC_MBTIME_CORE_START].tout		= HFC_FX_DF1_MB_TO;
	pp->mb_timer[HFC_MBTIME_LINK_INI].tout			= HFC_FX_DF1_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp1)) {
		pp->mb_timer[HFC_MBTIME_CORE_START].tout	= fp->mailbox_rsp_timer_grp1;
		pp->mb_timer[HFC_MBTIME_LINK_INI].tout		= fp->mailbox_rsp_timer_grp1;
	}
	
	/* Get Mailbox Rsp Timer2 */
	pp->mb_timer[HFC_MBTIME_FLOGI].tout 	= HFC_FX_DF2_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp2)) {
		pp->mb_timer[HFC_MBTIME_FLOGI].tout	= fp->mailbox_rsp_timer_grp2;
	}
	
	/* Get Mailbox Rsp Timer3 */
	pp->mb_timer[HFC_MBTIME_PLOGI].tout 	= HFC_FX_DF3_MB_TO;
	pp->mb_timer[HFC_MBTIME_PDISC].tout 	= HFC_FX_DF3_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp3)) {
		pp->mb_timer[HFC_MBTIME_PLOGI].tout	= fp->mailbox_rsp_timer_grp3;
		pp->mb_timer[HFC_MBTIME_PDISC].tout	= fp->mailbox_rsp_timer_grp3;
	}
	
	/* Get Mailbox Rsp Timer4 */
	pp->mb_timer[HFC_MBTIME_CAN_SCSI].tout		= HFC_FX_DF4_MB_TO;
	pp->mb_timer[HFC_MBTIME_OFFLINE].tout		= HFC_FX_DF4_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp4)) {
		pp->mb_timer[HFC_MBTIME_CAN_SCSI].tout	= fp->mailbox_rsp_timer_grp4;
		pp->mb_timer[HFC_MBTIME_OFFLINE].tout	= fp->mailbox_rsp_timer_grp4;
	}
	
	/* Get Mailbox Rsp Timer5 */
	pp->mb_timer[HFC_MBTIME_PRLI].tout					= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_PRLO].tout					= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_SCR].tout					= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_LOGO].tout					= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_AUTH_RJT].tout				= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_AUTH_NEGO].tout				= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].tout		= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].tout			= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].tout		= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_EVFP_SYNC].tout				= HFC_FX_DF5_MB_TO;
	pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].tout			= HFC_FX_DF5_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp5)) {
		pp->mb_timer[HFC_MBTIME_PRLI].tout				= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_PRLO].tout				= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_SCR].tout				= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_LOGO].tout				= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_RJT].tout			= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_NEGO].tout			= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].tout	= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].tout		= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].tout	= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_SYNC].tout			= fp->mailbox_rsp_timer_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].tout		= fp->mailbox_rsp_timer_grp5;
	}
	
	/* Get Mailbox Rsp Timer6 */
	pp->mb_timer[HFC_MBTIME_GCS_ID].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_GID_PN].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_GPN_ID].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_GID_FT].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_RFT_ID].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_RFF_ID].tout		= HFC_FX_DF6_MB_TO;
	pp->mb_timer[HFC_MBTIME_GPN_FT].tout		= HFC_FX_DF6_MB_TO;
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp6)) {
		pp->mb_timer[HFC_MBTIME_GCS_ID].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_GID_PN].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_ID].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_GID_FT].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_RFT_ID].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_RFF_ID].tout	= fp->mailbox_rsp_timer_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_FT].tout	= fp->mailbox_rsp_timer_grp6;
	}
	
	/* Get Mailbox Rsp Timer7 */
	pp->mb_timer[HFC_MBTIME_SHADOW_UP].tout			= HFC_FX_DF7_MB_TO;	/* FCLNX-GPL-FX-387 */
	pp->mb_timer[HFC_MBTIME_ADD_PORTID].tout		= HFC_FX_DF7_MB_TO;
	pp->mb_timer[HFC_MBTIME_DEL_PORTID].tout		= HFC_FX_DF7_MB_TO;
	pp->mb_timer[HFC_MBTIME_MIHLOG].tout			= HFC_FX_DF7_MB_TO;
	pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout			= HFC_FX_DF7_MB_TO;
	
	pp->mb_timer[HFC_MBTIME_DIAG].tout				= HFC_FX_DF7_MB_TO;				/* FCLNX-GPL-FX-126 */
	
	if (hfc_chk_conf_val(HFC_FX_MB_TO_MIN, HFC_FX_MB_TO_MAX, fp->mailbox_rsp_timer_grp7)) {
		pp->mb_timer[HFC_MBTIME_SHADOW_UP].tout		= fp->mailbox_rsp_timer_grp7;	/* FCLNX-GPL-FX-387 */
		pp->mb_timer[HFC_MBTIME_ADD_PORTID].tout	= fp->mailbox_rsp_timer_grp7;
		pp->mb_timer[HFC_MBTIME_DEL_PORTID].tout	= fp->mailbox_rsp_timer_grp7;
		pp->mb_timer[HFC_MBTIME_MIHLOG].tout		= fp->mailbox_rsp_timer_grp7;
		pp->mb_timer[HFC_MBTIME_LOADCHTRC].tout		= fp->mailbox_rsp_timer_grp7;
		pp->mb_timer[HFC_MBTIME_DIAG].tout			= fp->mailbox_rsp_timer_grp7;	/* FCLNX-GPL-FX-126 */
	}
	
	/* Get Mailbox Retry Func Timer1 */
	wk = fp->mailbox_retry_count_grp1 & 0x7f;
	if (fp->mailbox_retry_count_grp1 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp1 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF1_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_CORE_START].retry	= fp->mailbox_retry_count_grp1;
		pp->mb_timer[HFC_MBTIME_LINK_INI].retry		= fp->mailbox_retry_count_grp1;
	}
	else {
		pp->mb_timer[HFC_MBTIME_CORE_START].retry	= def;
		pp->mb_timer[HFC_MBTIME_LINK_INI].retry		= def;
	}
	
	/* Get Mailbox Retry Func Timer2 */
	wk = fp->mailbox_retry_count_grp2 & 0x7f;
	if (fp->mailbox_retry_count_grp2 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp2 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF2_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_FLOGI].retry = fp->mailbox_retry_count_grp2;
	}
	else {
		pp->mb_timer[HFC_MBTIME_FLOGI].retry = def;
	}
	
	/* Get Mailbox Retry Func Timer3 */
	wk = fp->mailbox_retry_count_grp3 & 0x7f;
	if (fp->mailbox_retry_count_grp3 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp3 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF3_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_PLOGI].retry = fp->mailbox_retry_count_grp3;
		pp->mb_timer[HFC_MBTIME_PDISC].retry = fp->mailbox_retry_count_grp3;
	}
	else {
		pp->mb_timer[HFC_MBTIME_PLOGI].retry = def;
		pp->mb_timer[HFC_MBTIME_PDISC].retry = def;
	}
	
	/* Get Mailbox Retry Func Timer4 */
	wk = fp->mailbox_retry_count_grp4 & 0x7f;
	if (fp->mailbox_retry_count_grp4 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp4 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF4_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = fp->mailbox_retry_count_grp4;
		pp->mb_timer[HFC_MBTIME_OFFLINE].retry  = fp->mailbox_retry_count_grp4;
	}
	else {
		pp->mb_timer[HFC_MBTIME_CAN_SCSI].retry = def;
		pp->mb_timer[HFC_MBTIME_OFFLINE].retry  = def;
	}
	
	/* Get Mailbox Retry Func Timer5 */
	wk = fp->mailbox_retry_count_grp5 & 0x7f;
	if (fp->mailbox_retry_count_grp5 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp5 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF5_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_PRLI].retry				= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_PRLO].retry				= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_SCR].retry				= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_LOGO].retry				= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry			= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry		= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry	= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry		= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry	= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry		= fp->mailbox_retry_count_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry		= fp->mailbox_retry_count_grp5;
	}
	else {
		pp->mb_timer[HFC_MBTIME_PRLI].retry				= def;
		pp->mb_timer[HFC_MBTIME_PRLO].retry				= def;
		pp->mb_timer[HFC_MBTIME_SCR].retry				= def;
		pp->mb_timer[HFC_MBTIME_LOGO].retry				= def;
		pp->mb_timer[HFC_MBTIME_AUTH_RJT].retry			= def;
		pp->mb_timer[HFC_MBTIME_AUTH_NEGO].retry		= def;
		pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].retry	= def;
		pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].retry		= def;
		pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].retry	= def;
		pp->mb_timer[HFC_MBTIME_EVFP_SYNC].retry		= def;
		pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].retry		= def;
	}
	
	/* Get Mailbox Retry Func Timer6 */
	wk = fp->mailbox_retry_count_grp6 & 0x7f;
	if (fp->mailbox_retry_count_grp6 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp6 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF6_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_GCS_ID].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_GID_PN].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_ID].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_GID_FT].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_RFT_ID].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_RFF_ID].retry	= fp->mailbox_retry_count_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_FT].retry	= fp->mailbox_retry_count_grp6;
	}
	else {
		pp->mb_timer[HFC_MBTIME_GCS_ID].retry	= def;
		pp->mb_timer[HFC_MBTIME_GID_PN].retry	= def;
		pp->mb_timer[HFC_MBTIME_GPN_ID].retry	= def;
		pp->mb_timer[HFC_MBTIME_GID_FT].retry	= def;
		pp->mb_timer[HFC_MBTIME_RFT_ID].retry	= def;
		pp->mb_timer[HFC_MBTIME_RFF_ID].retry	= def;
		pp->mb_timer[HFC_MBTIME_GPN_FT].retry	= def;
	}
	
	/* Get Mailbox Retry Func Timer7 */
	wk = fp->mailbox_retry_count_grp7 & 0x7f;
	if (fp->mailbox_retry_count_grp7 & 0x80) {
		min = HFC_MBTIME_RETRY_MIN;
		max = HFC_MBTIME_RETRY_MAX;
		def = HFC_MBTIME_RETRY_DF | (fp->mailbox_retry_count_grp7 & 0x80);
	}
	else {
		min = HFC_FX_MB_RETRY_MIN;
		max = HFC_FX_MB_RETRY_MAX;
		def = HFC_FX_DF7_MB_RETRY;
	}
	if (hfc_chk_conf_val(min, max, wk)) {
		pp->mb_timer[HFC_MBTIME_SHADOW_UP].retry	= fp->mailbox_retry_count_grp7;	/* FCLNX-GPL-FX-387 */
		pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry	= fp->mailbox_retry_count_grp7;
		pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry	= fp->mailbox_retry_count_grp7;
		pp->mb_timer[HFC_MBTIME_MIHLOG].retry		= fp->mailbox_retry_count_grp7;
		pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry	= fp->mailbox_retry_count_grp7;
		pp->mb_timer[HFC_MBTIME_DIAG].retry			= fp->mailbox_retry_count_grp7;	/* FCLNX-GPL-FX-126 */
	}
	else {
		pp->mb_timer[HFC_MBTIME_SHADOW_UP].retry	= def;	/* FCLNX-GPL-FX-387 */
		pp->mb_timer[HFC_MBTIME_ADD_PORTID].retry	= def;
		pp->mb_timer[HFC_MBTIME_DEL_PORTID].retry	= def;
		pp->mb_timer[HFC_MBTIME_MIHLOG].retry		= def;
		pp->mb_timer[HFC_MBTIME_LOADCHTRC].retry	= def;
		pp->mb_timer[HFC_MBTIME_DIAG].retry			= def;	/* FCLNX-GPL-FX-126 */
	}
	
	/* Get Mailbox Retry Delay1 */
	pp->mb_timer[HFC_MBTIME_CORE_START].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_LINK_INI].intvl			= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp1)) {
		pp->mb_timer[HFC_MBTIME_CORE_START].intvl	= fp->mailbox_retry_delay_grp1;
		pp->mb_timer[HFC_MBTIME_LINK_INI].intvl		= fp->mailbox_retry_delay_grp1;
	}
	
	/* Get Mailbox Retry Delay2 */
	pp->mb_timer[HFC_MBTIME_FLOGI].intvl 		= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp2)) {
		pp->mb_timer[HFC_MBTIME_FLOGI].intvl	= fp->mailbox_retry_delay_grp2;
	}
	
	/* Get Mailbox Retry Delay3 */
	pp->mb_timer[HFC_MBTIME_PLOGI].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_PDISC].intvl		= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp3)) {
		pp->mb_timer[HFC_MBTIME_PLOGI].intvl	= fp->mailbox_retry_delay_grp3;
		pp->mb_timer[HFC_MBTIME_PDISC].intvl	= fp->mailbox_retry_delay_grp3;
	}
	
	/* Get Mailbox Retry Delay4 */
	pp->mb_timer[HFC_MBTIME_CAN_SCSI].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_OFFLINE].intvl		= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp4)) {
		pp->mb_timer[HFC_MBTIME_CAN_SCSI].intvl = fp->mailbox_retry_delay_grp4;
		pp->mb_timer[HFC_MBTIME_OFFLINE].intvl  = fp->mailbox_retry_delay_grp4;
	}
	
	/* Get Mailbox Retry Delay5 */
	pp->mb_timer[HFC_MBTIME_PRLI].intvl					= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_PRLO].intvl					= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_SCR].intvl					= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_LOGO].intvl					= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_AUTH_RJT].intvl				= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_AUTH_NEGO].intvl			= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].intvl			= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_EVFP_SYNC].intvl			= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].intvl			= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp5)) {
		pp->mb_timer[HFC_MBTIME_PRLI].intvl				= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_PRLO].intvl				= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_SCR].intvl				= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_LOGO].intvl				= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_RJT].intvl			= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_AUTH_NEGO].intvl		= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_CHALLENGE].intvl	= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_REPLY].intvl		= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_DHCHAP_SUCCESS].intvl	= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_SYNC].intvl		= fp->mailbox_retry_delay_grp5;
		pp->mb_timer[HFC_MBTIME_EVFP_COMMIT].intvl		= fp->mailbox_retry_delay_grp5;
	}
	
	/* Get Mailbox Retry Delay6 */
	pp->mb_timer[HFC_MBTIME_GCS_ID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_GID_PN].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_GPN_ID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_GID_FT].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_RFT_ID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_RFF_ID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_GPN_FT].intvl		= HFC_FX_MB_INTVL_MIN;
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp6)) {
		pp->mb_timer[HFC_MBTIME_GCS_ID].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_GID_PN].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_ID].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_GID_FT].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_RFT_ID].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_RFF_ID].intvl	= fp->mailbox_retry_delay_grp6;
		pp->mb_timer[HFC_MBTIME_GPN_FT].intvl	= fp->mailbox_retry_delay_grp6;
	}
	
	/* Get Mailbox Retry Delay7 */
	pp->mb_timer[HFC_MBTIME_SHADOW_UP].intvl		= HFC_FX_MB_INTVL_MIN;	/* FCLNX-GPL-FX-387 */
	pp->mb_timer[HFC_MBTIME_ADD_PORTID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_DEL_PORTID].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_MIHLOG].intvl			= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_LOADCHTRC].intvl		= HFC_FX_MB_INTVL_MIN;
	pp->mb_timer[HFC_MBTIME_DIAG].intvl				= HFC_FX_MB_INTVL_MIN;	/* FCLNX-GPL-FX-126 */
	
	if (hfc_chk_conf_val(HFC_FX_MB_INTVL_MIN, HFC_FX_MB_INTVL_MAX, fp->mailbox_retry_delay_grp7)) {
		pp->mb_timer[HFC_MBTIME_SHADOW_UP].intvl	= fp->mailbox_retry_delay_grp7;	/* FCLNX-GPL-FX-387 */
		pp->mb_timer[HFC_MBTIME_ADD_PORTID].intvl	= fp->mailbox_retry_delay_grp7;
		pp->mb_timer[HFC_MBTIME_DEL_PORTID].intvl	= fp->mailbox_retry_delay_grp7;
		pp->mb_timer[HFC_MBTIME_MIHLOG].intvl		= fp->mailbox_retry_delay_grp7;
		pp->mb_timer[HFC_MBTIME_LOADCHTRC].intvl	= fp->mailbox_retry_delay_grp7;
		pp->mb_timer[HFC_MBTIME_DIAG].intvl			= fp->mailbox_retry_delay_grp7;	/* FCLNX-GPL-FX-126 */
	}
	
	/* Seve Boot Priority */
	memset(pp->boot_priority,0,sizeof(pp->boot_priority));
	if (pp->narrowmap) {
		/* Check Byte0-3!=ALL'F' */
		if ((fp->Reserved_00 == 0xff) &&
			(fp->flag == 0xff) &&
			(fp->bvc_entry_num == 0xff) &&
			(fp->connection_type == 0xff)) {
			return(0);
		}
		
		/* Valid Boot Priority   */
		if ((fp->flag & 0x90) != 0x90) {
			return(0);
		}
		
		for (i=0; i<8; i++) {
			if ( HFC_FX_MMODE_CHECK_MLPF(pp) )
			{	/* FCLNX-GPL-FX-446 */
				for (j=0;j<16;j++) {
					buf[j] = hfc_fx_read_reg_hg_ext(pp,
								pp->lparmode.hg_map->iosp.reg[HFC_IOHGSPC_EFI_OP_TBL0]
								+(0x10*(i+1))+j, 1);
				}
				/* Get WWPN */
				pp->boot_priority[i].ww_name = (*(uint*)(&buf[12]));
				pp->boot_priority[i].ww_name <<= 32;
				pp->boot_priority[i].ww_name |= (*(uint*)(&buf[8]));
				
				/* Get LUN */
				pp->boot_priority[i].lun = buf[6];	/* FCLNX-GPL-FX-446 */
			}else{
				/* Get WWPN */
				HFC_4B_TO_4L(wwn_wk, fp->bvc_table[8+16*i]);
				pp->boot_priority[i].ww_name = wwn_wk;
				pp->boot_priority[i].ww_name <<= 32;
				HFC_4B_TO_4L(wwn_wk, fp->bvc_table[12+16*i]);
				pp->boot_priority[i].ww_name |= wwn_wk;
				
				/* Get LUN */
				HFC_2B_TO_2L(lun_wk, fp->bvc_table[6+16*i]);
				pp->boot_priority[i].lun = (uint)lun_wk;
			}
		}
	}
	
	HFC_DBGPRT("hfcldd%d : forced PB disable   = %d.\n",pp->dev_minor, pp->automap);
	HFC_DBGPRT("hfcldd%d : narrowmap           = %d.\n",pp->dev_minor, pp->narrowmap);
	HFC_DBGPRT("hfcldd%d : forced default parm = %d.\n",pp->dev_minor, pp->defparam);
	HFC_DBGPRT("hfcldd%d : core_deg_mode       = %d.\n",pp->dev_minor, pp->core_deg_mode);
	HFC_DBGPRT("hfcldd%d : login Delay time    = %d.\n",pp->dev_minor, pp->login_wait);
	HFC_DBGPRT("hfcldd%d : link speed          = %d.\n",pp->dev_minor, pp->linkspeed);
	HFC_DBGPRT("hfcldd%d : connection Type     = %d.\n",pp->dev_minor, pp->topology);
	HFC_DBGPRT("hfcldd%d : spinup delay        = %d.\n",pp->dev_minor, pp->spinup_delay);
	HFC_DBGPRT("hfcldd%d : Link Init Timer     = %d.\n",pp->dev_minor, pp->link_initialize_tmo);
	HFC_DBGPRT("hfcldd%d : MCK LinkUp Timer    = %d.\n",pp->dev_minor, pp->mck_rcv_tmo);
	HFC_DBGPRT("hfcldd%d : isol_cmd            = %d.\n",pp->dev_minor, pp->isol_cmd);
	HFC_DBGPRT("hfcldd%d : rft_id_skip         = %d.\n",pp->dev_minor, pp->rft_id_skip);
	HFC_DBGPRT("hfcldd%d : mailbox_force_retry = %d.\n",pp->dev_minor, pp->mailbox_force_retry);
	HFC_DBGPRT("hfcldd%d : login_filter        = %d.\n",pp->dev_minor, pp->filter_target);
	HFC_DBGPRT("hfcldd%d : vf_enable           = %d.\n",pp->dev_minor, pp->vf_enable);
	HFC_DBGPRT("hfcldd%d : vf_mode_tagging     = %d.\n",pp->dev_minor, pp->vf_mode_tagging);
	HFC_DBGPRT("hfcldd%d : security_enable     = %d.\n",pp->dev_minor, pp->security_enable);
	
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[0]    = %d.\n",pp->dev_minor, pp->mb_timer[0].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[1]    = %d.\n",pp->dev_minor, pp->mb_timer[1].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[2]    = %d.\n",pp->dev_minor, pp->mb_timer[2].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[3]    = %d.\n",pp->dev_minor, pp->mb_timer[3].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[4]    = %d.\n",pp->dev_minor, pp->mb_timer[4].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[5]    = %d.\n",pp->dev_minor, pp->mb_timer[5].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[6]    = %d.\n",pp->dev_minor, pp->mb_timer[6].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[7]    = %d.\n",pp->dev_minor, pp->mb_timer[7].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[8]    = %d.\n",pp->dev_minor, pp->mb_timer[8].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[9]    = %d.\n",pp->dev_minor, pp->mb_timer[9].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[10]   = %d.\n",pp->dev_minor, pp->mb_timer[10].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[11]   = %d.\n",pp->dev_minor, pp->mb_timer[11].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[12]   = %d.\n",pp->dev_minor, pp->mb_timer[12].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[13]   = %d.\n",pp->dev_minor, pp->mb_timer[13].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[14]   = %d.\n",pp->dev_minor, pp->mb_timer[14].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[15]   = %d.\n",pp->dev_minor, pp->mb_timer[15].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[16]   = %d.\n",pp->dev_minor, pp->mb_timer[16].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[17]   = %d.\n",pp->dev_minor, pp->mb_timer[17].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[18]   = %d.\n",pp->dev_minor, pp->mb_timer[18].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[19]   = %d.\n",pp->dev_minor, pp->mb_timer[19].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[20]   = %d.\n",pp->dev_minor, pp->mb_timer[20].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[21]   = %d.\n",pp->dev_minor, pp->mb_timer[21].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[22]   = %d.\n",pp->dev_minor, pp->mb_timer[22].delay);
	HFC_DBGPRT("hfcldd%d : Mailbox Delay[23]   = %d.\n",pp->dev_minor, pp->mb_timer[23].delay);
	
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[0]    = %d.\n",pp->dev_minor, pp->mb_timer[0].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[1]    = %d.\n",pp->dev_minor, pp->mb_timer[1].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[2]    = %d.\n",pp->dev_minor, pp->mb_timer[2].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[3]    = %d.\n",pp->dev_minor, pp->mb_timer[3].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[4]    = %d.\n",pp->dev_minor, pp->mb_timer[4].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[5]    = %d.\n",pp->dev_minor, pp->mb_timer[5].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[6]    = %d.\n",pp->dev_minor, pp->mb_timer[6].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[7]    = %d.\n",pp->dev_minor, pp->mb_timer[7].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[8]    = %d.\n",pp->dev_minor, pp->mb_timer[8].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[9]    = %d.\n",pp->dev_minor, pp->mb_timer[9].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[10]   = %d.\n",pp->dev_minor, pp->mb_timer[10].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[11]   = %d.\n",pp->dev_minor, pp->mb_timer[11].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[12]   = %d.\n",pp->dev_minor, pp->mb_timer[12].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[13]   = %d.\n",pp->dev_minor, pp->mb_timer[13].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[14]   = %d.\n",pp->dev_minor, pp->mb_timer[14].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[15]   = %d.\n",pp->dev_minor, pp->mb_timer[15].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[16]   = %d.\n",pp->dev_minor, pp->mb_timer[16].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[17]   = %d.\n",pp->dev_minor, pp->mb_timer[17].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[18]   = %d.\n",pp->dev_minor, pp->mb_timer[18].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[19]   = %d.\n",pp->dev_minor, pp->mb_timer[19].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[20]   = %d.\n",pp->dev_minor, pp->mb_timer[20].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[21]   = %d.\n",pp->dev_minor, pp->mb_timer[21].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[22]   = %d.\n",pp->dev_minor, pp->mb_timer[22].tout);
	HFC_DBGPRT("hfcldd%d : Mailbox Rsp Timer[23]   = %d.\n",pp->dev_minor, pp->mb_timer[23].tout);
	
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[0]    = %d.\n",pp->dev_minor, pp->mb_timer[0].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[1]    = %d.\n",pp->dev_minor, pp->mb_timer[1].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[2]    = %d.\n",pp->dev_minor, pp->mb_timer[2].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[3]    = %d.\n",pp->dev_minor, pp->mb_timer[3].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[4]    = %d.\n",pp->dev_minor, pp->mb_timer[4].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[5]    = %d.\n",pp->dev_minor, pp->mb_timer[5].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[6]    = %d.\n",pp->dev_minor, pp->mb_timer[6].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[7]    = %d.\n",pp->dev_minor, pp->mb_timer[7].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[8]    = %d.\n",pp->dev_minor, pp->mb_timer[8].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[9]    = %d.\n",pp->dev_minor, pp->mb_timer[9].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[10]   = %d.\n",pp->dev_minor, pp->mb_timer[10].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[11]   = %d.\n",pp->dev_minor, pp->mb_timer[11].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[12]   = %d.\n",pp->dev_minor, pp->mb_timer[12].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[13]   = %d.\n",pp->dev_minor, pp->mb_timer[13].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[14]   = %d.\n",pp->dev_minor, pp->mb_timer[14].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[15]   = %d.\n",pp->dev_minor, pp->mb_timer[15].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[16]   = %d.\n",pp->dev_minor, pp->mb_timer[16].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[17]   = %d.\n",pp->dev_minor, pp->mb_timer[17].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[18]   = %d.\n",pp->dev_minor, pp->mb_timer[18].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[19]   = %d.\n",pp->dev_minor, pp->mb_timer[19].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[20]   = %d.\n",pp->dev_minor, pp->mb_timer[20].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[21]   = %d.\n",pp->dev_minor, pp->mb_timer[21].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[22]   = %d.\n",pp->dev_minor, pp->mb_timer[22].retry);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Func Timer[23]   = %d.\n",pp->dev_minor, pp->mb_timer[23].retry);
	
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[0]    = %d.\n",pp->dev_minor, pp->mb_timer[0].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[1]    = %d.\n",pp->dev_minor, pp->mb_timer[1].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[2]    = %d.\n",pp->dev_minor, pp->mb_timer[2].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[3]    = %d.\n",pp->dev_minor, pp->mb_timer[3].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[4]    = %d.\n",pp->dev_minor, pp->mb_timer[4].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[5]    = %d.\n",pp->dev_minor, pp->mb_timer[5].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[6]    = %d.\n",pp->dev_minor, pp->mb_timer[6].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[7]    = %d.\n",pp->dev_minor, pp->mb_timer[7].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[8]    = %d.\n",pp->dev_minor, pp->mb_timer[8].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[9]    = %d.\n",pp->dev_minor, pp->mb_timer[9].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[10]   = %d.\n",pp->dev_minor, pp->mb_timer[10].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[11]   = %d.\n",pp->dev_minor, pp->mb_timer[11].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[12]   = %d.\n",pp->dev_minor, pp->mb_timer[12].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[13]   = %d.\n",pp->dev_minor, pp->mb_timer[13].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[14]   = %d.\n",pp->dev_minor, pp->mb_timer[14].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[15]   = %d.\n",pp->dev_minor, pp->mb_timer[15].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[16]   = %d.\n",pp->dev_minor, pp->mb_timer[16].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[17]   = %d.\n",pp->dev_minor, pp->mb_timer[17].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[18]   = %d.\n",pp->dev_minor, pp->mb_timer[18].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[19]   = %d.\n",pp->dev_minor, pp->mb_timer[19].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[20]   = %d.\n",pp->dev_minor, pp->mb_timer[20].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[21]   = %d.\n",pp->dev_minor, pp->mb_timer[21].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[22]   = %d.\n",pp->dev_minor, pp->mb_timer[22].intvl);
	HFC_DBGPRT("hfcldd%d : Mailbox Retry Delay Timer[23]   = %d.\n",pp->dev_minor, pp->mb_timer[23].intvl);
	
	return(0);
}


char hfc_fx_cnvc(char C)										  /* FCWIN-0081 */
{
	char c = C;
	if (c >= 'A' && c <= 'Z')
		return (c - 'A' + 'a');

	return c;
}



int hfc_fx_parse_string(char *string, char *keyword, uint64_t *value)/* FCWIN-0081 */
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
	while (hfc_fx_cnvc(*cptr) == hfc_fx_cnvc(*kptr)) {
		
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
		if ((*cptr == '0') && (hfc_fx_cnvc(*(cptr + 1)) == 'x')) {

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
					if ((hfc_fx_cnvc(*(cptr + index)) >= 'a') && (hfc_fx_cnvc(*(cptr + index)) <= 'f')) {
						*value = (16 * (*value)) + (hfc_fx_cnvc(*(cptr + index)) - 'a') + 10;
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



int hfc_fx_convert_string(char *string, uint64_t *value)
{
	char *cptr;
	int index;

	cptr = string;

		*value = 0;
		if ((*cptr == '0') && (hfc_fx_cnvc(*(cptr + 1)) == 'x')) {

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
					if ((hfc_fx_cnvc(*(cptr + index)) >= 'a') && (hfc_fx_cnvc(*(cptr + index)) <= 'f')) {
						*value = (16 * (*value)) + (hfc_fx_cnvc(*(cptr + index)) - 'a') + 10;
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

char *hfc_fx_delete_space(char *string){	/* FCLNX-GPL-311 */
	
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
char *hfc_fx_get_write_retries(void)
{
	return (write_retries);
}
#endif

void hfc_fx_set_pub_symbol_list(void)
{
#if 0
	struct pub_symbol_list		*pubp;
	
	HFC_ENTRY("hfc_set_pub_symbol_list");
	
	pubp  = hfc_get_pub_symbol_list();
	
	/* detect */
	pubp->hfc_fx_param_search				=	hfc_fx_param_search;
	pubp->hfc_fx_cnvc						=	hfc_fx_cnvc;
	pubp->hfc_fx_parse_string				=	hfc_fx_parse_string;
	pubp->hfc_fx_convert_string				=	hfc_fx_convert_string;
//	pubp->hfc_fx_get_write_retries			=	hfc_fx_get_write_retries;		/* FCLNX-GPL-0449 */
	pubp->hfc_fx_initialize					=	hfc_fx_initialize;				/* FCLNX-0526 */
	pubp->hfc_fx_chk_stop					=	hfc_fx_chk_stop;				/* FCLNX-0534 */
	pubp->hfc_fx_issue_forced_mck			=	hfc_fx_issue_forced_mck;		/* FCLNX-0534 */
	pubp->hfc_chk_conf_val					=	hfc_chk_conf_val;
	pubp->hfc_fx_config_hw_set_five_fx		=	hfc_fx_config_hw_set_five_fx;
	
	/* strategy */
	pubp->hfc_fx_strategy_pg				=	hfc_fx_strategy_pg;
	pubp->hfc_fx_eh_abort_pg				=	hfc_fx_eh_abort_pg;
	pubp->hfc_fx_eh_device_reset_pg			=	hfc_fx_eh_device_reset_pg;
	pubp->hfc_fx_eh_target_reset_pg			=	hfc_fx_eh_target_reset_pg;		/* FCLNX-GPL-0343 */
	pubp->hfc_fx_eh_bus_reset_pg			=	hfc_fx_eh_bus_reset_pg;
	pubp->hfc_fx_set_cmnd_res				=	hfc_fx_set_cmnd_res;
	pubp->hfc_fx_iodone						=	hfc_fx_iodone;
	pubp->hfc_fx_strategy_port				=	hfc_fx_strategy_port;
	pubp->hfc_fx_strategy_core				=	hfc_fx_strategy_core;
	pubp->hfc_fx_get_new_hfcp				=	hfc_fx_get_new_hfcp;
	pubp->hfc_fx_start						=	hfc_fx_start;					/* FCLNX-0429 */
	pubp->hfc_fx_cancel_scsi_cmd			=	hfc_fx_cancel_scsi_cmd;		/* FCLNX-0429 */
	pubp->hfc_fx_get_new_cmnd				=	hfc_fx_get_new_cmnd;
	pubp->hfc_fx_dummy_copy					=	hfc_fx_dummy_copy;
	
	/* timer_recovery*/
	pubp->hfc_fx_reset_port_info			=	hfc_fx_reset_port_info;
	pubp->hfc_fx_reset_start				= 	hfc_fx_reset_start;
	pubp->hfc_fx_errlog						=	hfc_fx_errlog;
	pubp->hfc_fx_watchdog					= 	hfc_fx_watchdog;
	pubp->hfc_fx_force_linkdown				=	hfc_fx_force_linkdown;
	pubp->hfc_fx_force_linkdown_recovery	=	hfc_fx_force_linkdown_recovery;

	/* top */
	pubp->hfc_fx_hash_target_valid			=	hfc_fx_hash_target_valid;
	pubp->hfc_fx_hash_target_info			=	hfc_fx_hash_target_info;
	pubp->hfc_fx_hash_target_info_wwn		=	hfc_fx_hash_target_info_wwn;
	pubp->hfc_fx_read_tbl					=	hfc_fx_read_tbl;
//	pubp->hfc_fx_issue_relogin				=	hfc_fx_issue_relogin;			/* FCLNX-0429 */
	pubp->hfc_fx_enque_plogi_req			=	hfc_fx_enque_plogi_req;		/* FCLNX-0429 */
	pubp->hfc_fx_enque_prli_req				=	hfc_fx_enque_prli_req;
	pubp->lock_mailbox						=	lock_mailbox;				/* FCLNX-0526 */
	pubp->unlock_mailbox					=	unlock_mailbox;				/* FCLNX-0526 */
	pubp->hfc_fx_write_reg_ext				=	hfc_fx_write_reg_ext;			/* FCLNX-0526 */
	pubp->hfc_fx_trace						=	hfc_fx_trace;
	pubp->hfc_fx_watchdog_enter				= 	hfc_fx_watchdog_enter;
	pubp->hfc_fx_search_dev_info			=	hfc_fx_search_dev_info;		/* FCLNX-GPL-449 */
	pubp->hfc_fx_kmalloc					=	hfc_fx_kmalloc;				/* FCLNX-GPL-204 */
	pubp->hfc_fx_kfree						=	hfc_fx_kfree;					/* FCLNX-GPL-204 */
	pubp->hfc_fx_mp_watchdog_enter			=	hfc_fx_mp_watchdog_enter;		/* FCLNX-GPL-471 */
	
	/* ioctl */
	pubp->hfc_fx_sciocmd					=	hfc_fx_sciocmd;
	pubp->hfc_fx_ioctl_iodone				=	hfc_fx_ioctl_iodone;
	pubp->structdump						=	structdump;

	/* mlpf */
	pubp->hfc_fx_read_reg_hg_ext			=	hfc_fx_read_reg_hg_ext;		/* FCLNX-GPL-451 */
	pubp->hfc_fx_write_reg_hg_ext			=	hfc_fx_write_reg_hg_ext;		/* FCLNX-GPL-451 */

	pubp->hfc_fx_reset_all_timer			=	hfc_fx_reset_all_timer;
	pubp->_hfc_fx_wake_up					=	_hfc_fx_wake_up;


	HFC_EXIT("hfc_fx_set_pub_symbol_list");
	
#endif
}



int hfc_fx_check_nonpub_symbol(void)
{
#if 0
	int	rtn = 0;
	
	HFC_ENTRY("hfc_fx_check_nonpub_symbol");
	
	/* detect */
	if (hfc_manage_info.npubp->hfc_fx_check_hop == NULL)						rtn++;	/* FCLNX-0429 */
	if (hfc_manage_info.npubp->hfc_fx_mp_scan_dev == NULL)						rtn++;	/* FCLNX-0521 */
	if (hfc_manage_info.npubp->hfc_fx_make_lgpath == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_remove_lgpath == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_host_rescan == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_wait_mp_ioend == NULL)					rtn++;	/* FCLNX-0459 */
	if (hfc_manage_info.npubp->hfc_fx_write_retries == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_update_attribute == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_proc_info_lun == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_proc_info_option == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_fo_check_and_offline == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_forced_offline_e == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_forced_offline_c == NULL)				rtn++;	/* FCLNX-0147 */
	if (hfc_manage_info.npubp->hfc_fx_mp_queue_depth == NULL)					rtn++;	/* FCLNX-0521 */
	if (hfc_manage_info.npubp->hfc_fx_isolconf_setup == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_allocate_errcnt_info == NULL)			rtn++;
	if (hfc_manage_info.npubp->hfc_fx_free_errcnt_info == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_set_retry_cnt == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_module_init == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_set_parm == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_host_alloc == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_add_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_scan_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_scsi_remove_host == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_check_mp_enable == NULL)					rtn++;

	/* strategy */
	if (hfc_manage_info.npubp->hfc_fx_mp_strategy == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_abort == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_device_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_target_reset == NULL)					rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_fx_mp_bus_reset == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_retry_strategy == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_ioerror_check == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_queue_check == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_queue_count == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_iodone == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_io_reset_complete == NULL)			rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_check_dev_reset_complete == NULL)		rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_check_bus_reset_complete == NULL)		rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_make_fcinfo == NULL)						rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_set_scsi_cmd_tmr == NULL)				rtn++;	/* FCLNX-GPL-0449 */
	
	/* top */
	if (hfc_manage_info.npubp->hfc_fx_convert_rptluns == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_check_luconfig == NULL)					rtn++;	/* FCLNX-611 */
	
	/* timer_recovery */
	if (hfc_manage_info.npubp->hfc_fx_errlog_mp == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_watched_errcount == NULL)				rtn++;
	if (hfc_manage_info.npubp->hfc_fx_clear_errinfo == NULL)					rtn++;	/* FCLNX-0147 */
	if (hfc_manage_info.npubp->hfc_fx_wdog_retry_path_tmr == NULL)			rtn++;
	
	/* ioctl */
	if (hfc_manage_info.npubp->hfc_fx_ioctl_mp == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_rd_param == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_wr_param == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lu_map == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_setpath == NULL)						rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_path_health == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lgtarget_map == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_mp_lgpath_info1 == NULL)					rtn++;
	if (hfc_manage_info.npubp->hfc_fx_read_isolparam == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_read_retry_cnt == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_get_isolparam == NULL)					rtn++;	/* FCLNX-0147 */

	/* diag */
	if (hfc_manage_info.npubp->read_dev_info_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->read_lg_target_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_dev_info_mp == NULL)					rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info1_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_lg_path_info2_mp == NULL)				rtn++;
	if (hfc_manage_info.npubp->read_failover_info_mp == NULL)				rtn++;

	if (hfc_manage_info.npubp->hfc_fx_mp_proc_info_pfb == NULL)				rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_strategy_pfb == NULL)					rtn++;	/* FCLNX-GPL-204 */
	if (hfc_manage_info.npubp->hfc_fx_mp_lun_reset == NULL)					rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_fx_mp_target_reset_pfb == NULL)				rtn++;	/* FCLNX-GPL-0449 */
	if (hfc_manage_info.npubp->hfc_fx_mp_bus_reset_pfb == NULL)				rtn++;	/* FCLNX-GPL-204 */
	
	/* version */
	if (hfc_manage_info.npubp->hfc_fx_get_pcm_rcsid == NULL)					rtn++;
	
	HFC_DBGPRT("hfc_fx_check_nonpub_symbol() -- null symbol = %d\n", rtn);
	HFC_EXIT("hfc_fx_check_nonpub_symbol");	
	if ( rtn != 0 ){
		HFC_DBGPRT("hfc_fx_check_nonpub_symbol() -- null symbol = %d\n", rtn);
	}
	else
	{
		HFC_INFPRT("Success at loading hfcldd_mp module.\n");
	}
	return (rtn);
#endif
	return 1;
}

/* Alocate a dev_info_fx for a lun *//* FCLNX-GPL-0343 *//* FCLNX-GPL-0369 */
int hfc_fx_slave_alloc(struct scsi_device *sdev)
{
	struct port_info	*pp = NULL;
	struct port_info	*vpp = NULL;
	struct dev_info_fx		*dev = NULL;
	struct target_info_fx	*target = NULL;
	struct region_info	*rp = NULL;
	struct hfc_pkt_fx	*hfcp;
	ulong				flags = 0;
	uchar				logdata[16];
	uchar				no_dev = 0;
	int					i, alloc_cnt = 0;
	
//	HFC_DBGPRT("hfc_fx_slave_alloc scsi (%d:%d:%d:%d)\n",
//		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
	{
		return hfc_manage_info.npubp->hfc_fx_mp_slave_alloc(sdev);
	}
	else {
		pp = (struct port_info *)sdev->host->hostdata;
		if( pp == NULL ) return(0);
		
		rp =  pp->region_arg[pp->rid];
		if (rp == NULL)  return(0);
		
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		target = hfc_fx_hash_target_info( pp, (uint)sdev->id );
		if( target != NULL){
			if ((sdev->hostdata =(struct dev_info_fx *)hfc_fx_get_dev_info_fx(target, sdev->lun)) == NULL) {
				no_dev = 1;
			}
		}
		if (pp->pport->pkt_num < pp->pport->can_queue) {
			alloc_cnt = pp->queue_depth;
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		if (no_dev == 0){
			return 0;
		}
		
		dev = (struct dev_info_fx *)hfc_fx_kmalloc(pp, sizeof(struct dev_info_fx), GFP_KERNEL);
		if(dev == NULL){
			memset(logdata, 0, 16);
			logdata[0] = 0xd0;
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			return 0;
		}
		memset( dev, 0, sizeof(struct dev_info_fx) );
		
		for (i=0;i<alloc_cnt;i++) {
			hfcp = (struct hfc_pkt_fx *)hfc_fx_kmalloc(pp, sizeof(struct hfc_pkt_fx), GFP_KERNEL);
			if (hfcp == NULL) {
				hfc_fx_kfree(pp, dev);
				memset(logdata, 0, 16);
				logdata[0] = 0xd1;
				HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				return 0;
			}
			memset( hfcp, 0, sizeof(struct hfc_pkt_fx) );
			
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			
			if (pp->pport->pkt_top == NULL) {
				pp->pport->pkt_top = hfcp;
				pp->pport->pkt_end = hfcp;
			}
			else {
				pp->pport->pkt_end->pkt_next = hfcp;
				hfcp->pkt_prev = pp->pport->pkt_end;
				pp->pport->pkt_end = hfcp;
			}
			hfcp->pkt_no = pp->pport->pkt_num;
			pp->pport->pkt_num++;
			if (pp->pport->pkt_num >= pp->pport->can_queue) {
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				break;
			}
			
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		}
		
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		target = hfc_fx_hash_target_info( pp, (uint)sdev->id );
		if( target != NULL){
			if ((struct dev_info_fx *)hfc_fx_get_dev_info_fx(target, sdev->lun) == NULL) {
				dev->target_id = sdev->id;
				dev->lun = sdev->lun;
				
				sdev->hostdata = dev;
				
				if (target->dev == NULL) {
					target->dev   = dev;
					if (HFC_FX_MQ_VALID(pp)) {
						for (i=1; i<=pp->max_vport_count; i++) {
							vpp = pp->vport_ptr[i].vport_arg;
							if (vpp == NULL)
								continue;
							if (vpp->target_arg[target->pseq] == NULL)
								continue;
							
							vpp->target_arg[target->pseq]->dev = target->dev;
						}
					}
					
					dev->prev = NULL;
					dev->next = NULL;
				}
				else {
					struct dev_info_fx *term_dev = target->dev;
				
				while (term_dev->next != NULL)
					term_dev = term_dev->next;
					
					term_dev->next = dev;
					dev->prev = term_dev;
					dev->next = NULL;
				}
				dev->target = target;
				
				hfc_fx_assign_core_no(pp, dev);
			}
			else{
				sdev->hostdata = (struct dev_info_fx *)hfc_fx_get_dev_info_fx(target, sdev->lun);
			}
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	}

	return 0;
}

/* Release a dev_info_fx when the lun does not exist *//* FCLNX-GPL-0343 */
void hfc_fx_slave_destroy(struct scsi_device *sdev)
{
	struct port_info	*pp = NULL;
	struct port_info	*vpp = NULL;
	struct dev_info_fx		*dev = NULL, *temp_dev = NULL;
	struct target_info_fx	*target = NULL;
	struct region_info	*rp = NULL;
	ulong				flags = 0;
	int					i;

//	HFC_DBGPRT("hfc_fx_slave_destroy scsi (%d:%d:%d:%d)\n",
//		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
	{
		hfc_manage_info.npubp->hfc_fx_mp_slave_destroy(sdev);
		return;
	}
	else {
		pp = (struct port_info *)sdev->host->hostdata;
		if( pp == NULL ) return;
		
		if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){
			rp =  pp->region_arg[0];
		}else{	/* FCLNX-GPL-FX-385 */
			rp =  pp->region_arg[pp->rid];
		}	/* FCLNX-GPL-FX-385 */
		
		if( rp == NULL ) return;

		dev = (struct dev_info_fx *)sdev->hostdata;
		if( dev != NULL ){
			if( !test_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags) ){
				HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
				target = hfc_fx_hash_target_valid( pp, (uint)sdev->id );	/* FCLNX-GPL-FX-258,272 */
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
							if (HFC_FX_MQ_VALID(pp)) {	/* FCLNX-GPL-FX-205 */
								for (i=1; i<=pp->max_vport_count; i++) {
									vpp = pp->vport_ptr[i].vport_arg;
									if (vpp == NULL)
										continue;
									if (vpp->target_arg[target->pseq] == NULL)
										continue;
									
									vpp->target_arg[target->pseq]->dev = target->dev;
								}
							}							/* FCLNX-GPL-FX-205 */
						}
						else{
							temp_dev = temp_dev->prev;
							if( temp_dev != NULL){
								temp_dev->next = dev->next;
							}
						}
					}
				}
				memset( dev, 0, sizeof(struct dev_info_fx) );
				hfc_fx_kfree(pp, sdev->hostdata);
				sdev->hostdata = NULL;
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			}
		}
	}
	return; 
}

void hfc_fx_lu_scan_start(struct port_info *pp)
{
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	int i;
	ulong 	flags = 0;
	struct	target_info_fx	*target=NULL;
	struct	region_info		*rp = NULL;
	
	HFC_ENTRY("hfc_fx_lu_scan_start");
	
	if ( test_bit(HFC_SYSFS_RPORT, (ulong *)&pp->sysfs_control) ) {
		if ( !hfc_manage_info.hfcldd_mp_mod ) {
			if (HFC_FX_PHYSICAL_PORT(pp)) {
				hfc_fx_start_rport(pp);
			}
			
			rp = pp->region_arg[pp->rid];
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			for (i=0;i<MAX_TARGET_PROBE;i++) {
				target = hfc_fx_hash_target_info(pp, i);
				if( target != NULL ){
					set_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
				}
			}
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			atomic_set(&pp->pport->rport_event_wait, 1);
			wake_up_interruptible(&pp->pport->rport_event);
		}
	}
	
	HFC_EXIT("hfc_fx_lu_scan_start");
#endif
#endif /* SYSFS_SUPPORT */
	
	return;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
int hfc_fx_scan_finished(struct Scsi_Host *shost, unsigned long time)
{
	struct port_info *pp=NULL;
	
	HFC_ENTRY("hfc_fx_scan_finished");
	
	if( shost == NULL )
	{	/* Incorrect pointer */
		return 1; 
	}
	
	pp = (struct port_info *)shost->hostdata;
	if( pp == NULL )
	{	/* Incorrect pointer */
		return 1; 
	}
	
	if( time >= pp->scan_finished_tmo * HZ )
	{	/* Time out */
		return 1; 
	}
	/* [Note]                                                             */
	/* This checking is secure from a wraparound of "(ulong)jiffies".     */
	/* Reason : We use "time" value. time = (ulong)jiffies - (ulong)start */
	/* Example: time = (ulong)0x07 - (ulong)0xfffffffe = (ulong)0x09      */
	
	
	if( atomic_read(&pp->pport->rport_event_wait) == 0 )
	{	/* Scan Finished. */
		return 1; 
	}
	
	HFC_EXIT("hfc_fx_scan_finished");
	
	return 0;
}

void hfc_fx_scan_start(struct Scsi_Host *shost)
{
	struct port_info    *pp    =NULL;
	
	HFC_ENTRY("hfc_fx_scan_start");
	
	if( shost == NULL )
	{	/* Incorrect pointer */
		return;
	}
	
	pp = (struct port_info *)shost->hostdata;
	if( pp == NULL )
	{	/* Incorrect pointer */
		return;
	}
	
	hfc_fx_lu_scan_start(pp);
	
	HFC_EXIT("hfc_fx_scan_start");
	
	return;
}
#endif
#endif /* SYSFS_SUPPORT */

int hfc_fx_slave_configure(struct scsi_device *sdev)
{
	/* FCLNX-GPL-255 start */
	struct port_info *pp=NULL;
	int queue_depth, qd;
	struct dev_info_fx	 	*dev=NULL;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	struct request_queue	*rq=NULL;	/* FCLNX-GPL-409 */
	struct fc_rport *rport=NULL;	/* FCLNX-GPL-FX-472 */
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		rport = starget_to_rport(sdev->sdev_target);
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
#endif

//	HFC_DBGPRT("hfc_fx_slave_configure scsi (%d:%d:%d:%d)\n",
//		sdev->host->host_no, sdev->channel, sdev->id, sdev->lun);

	dev = (struct dev_info_fx *)sdev->hostdata;
//	rport = dev->target->rport;
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		if (hfc_manage_info.npubp->hfc_fx_mp_queue_depth(sdev, &qd)) {				/* FCLNX-0521 */
			queue_depth = qd;
		}
		else {
			/* hfcldd_mp ON & HFC-PCM OFF */
			pp = (struct port_info *)sdev->host->hostdata;
			queue_depth = pp->queue_depth;
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
				if(rport != NULL)
					rport->dev_loss_tmo = pp->dev_loss_tmo;
#endif /* KERNEL_VERSION(2,6,16) */
#endif /* SYSFS_SUPPORT */
#endif
		}
	}
	else {
		/* hfcldd_mp OFF */
		pp = (struct port_info *)sdev->host->hostdata;
		queue_depth = pp->queue_depth;
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
			if(rport != NULL)
				rport->dev_loss_tmo = pp->dev_loss_tmo;
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

	if (!hfc_manage_info.hfcldd_mp_mod) {
		/* Validate the dev_info_fx since lun was recognized *//* FCLNX-GPL-0343 */
		if( pp != NULL ){
			if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){
				rp =  pp->region_arg[0];
			}else{	/* FCLNX-GPL-FX-385 */
				rp =  pp->region_arg[pp->rid];
			}	/* FCLNX-GPL-FX-385 */
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			dev = (struct dev_info_fx *)sdev->hostdata;
			set_bit(HFC_DEVINF_VALID, (ulong *)&dev->flags);
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		}
	}
	rq = sdev->request_queue;								/* FCLNX-GPL-409 */
	if( rq != NULL ){
		blk_queue_rq_timed_out( rq, NULL );
	}														/* FCLNX-GPL-409 */

//	HFC_EXIT("hfc_fx_slave_configure");

	return (0);
}

#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
int hfc_fx_change_queue_depth(struct scsi_device *sdev, int qdepth, int reason)	/* FCLNX-GPL-450 */
#else
int hfc_fx_change_queue_depth(struct scsi_device *sdev, int qdepth)	
#endif
{
	struct port_info	*pp=NULL;		/* FCLNX-GPL-574 */
	struct hfc_pkt_fx	*hfcp=NULL;
	struct region_info	*rp = NULL;
	int new_queue_depth, old_queue_depth;
	int i, alloc_cnt = 0;
	ulong flags = 0;
	uchar logdata[16];
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )
	{
#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
		return hfc_manage_info.npubp->hfc_fx_mp_change_queue_depth(sdev, qdepth, reason);
#else
		return hfc_manage_info.npubp->hfc_fx_mp_change_queue_depth(sdev, qdepth, 0);
#endif
	}
	
	pp = (struct port_info *)sdev->host->hostdata;
	
	if( pp != NULL ){
#if !defined(HFC_STAR) && (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0)) /* FCLNX-GPL-FX-496 */
		if( (pp->ctl_change_qdepth)&&(reason != SCSI_QDEPTH_DEFAULT) ){
			return -EOPNOTSUPP;
		}
#endif
	}									/* FCLNX-GPL-574 */
	else if( pp == NULL ){
		return sdev->queue_depth;
	}
	
	old_queue_depth = sdev->queue_depth;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
	scsi_adjust_queue_depth(sdev, scsi_get_tag_type(sdev), qdepth);
#else
	scsi_change_queue_depth(sdev, qdepth);
#endif /* FCLNX-GPL-FX-496 end */
	new_queue_depth = sdev->queue_depth;
	
	if (new_queue_depth > old_queue_depth) {
		if (pp->pport->pkt_num < pp->pport->can_queue) {
			alloc_cnt = new_queue_depth - old_queue_depth;
		}
	}
	
	rp =  pp->region_arg[pp->rid];
	if (rp == NULL)  return sdev->queue_depth;
	
	for (i=0;i<alloc_cnt;i++) {
		hfcp = (struct hfc_pkt_fx *)hfc_fx_kmalloc(pp, sizeof(struct hfc_pkt_fx), GFP_KERNEL);
		if (hfcp == NULL) {
			memset(logdata, 0, 16);
			logdata[0] = 0xd2;
			HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC5, logdata, 16) ;
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			break;
		}
		memset( hfcp, 0, sizeof(struct hfc_pkt_fx) );
		
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		
		if (pp->pport->pkt_top == NULL) {
			pp->pport->pkt_top = hfcp;
			pp->pport->pkt_end = hfcp;
		}
		else {
			pp->pport->pkt_end->pkt_next = hfcp;
			hfcp->pkt_prev = pp->pport->pkt_end;
			pp->pport->pkt_end = hfcp;
		}
		hfcp->pkt_no = pp->pport->pkt_num;
		pp->pport->pkt_num++;
		if (pp->pport->pkt_num >= pp->pport->can_queue) {
			HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
			break;
		}
		
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	}
	
	return sdev->queue_depth;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0) /* FCLNX-GPL-FX-496 start */
int hfc_fx_change_queue_type(struct scsi_device *sdev, int tag_type)			/* FCLNX-GPL-450*/
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

int
hfc_fx_biosparam(struct scsi_device *sdev, struct block_device *bdev, sector_t CAPAcity, int geom[])
{
	int heads, sectors, cylinders;

	heads = 64;
	sectors = 32;
	cylinders = (unsigned long)CAPAcity / (heads * sectors);

	if(cylinders > 1024) {
		heads = 255;
		sectors = 63;
		cylinders = (unsigned long)CAPAcity / (heads * sectors);
	}
 
	geom[0] = heads;
	geom[1] = sectors;
	geom[2] = cylinders;

	return 0;
}


/*
 * Function: hfc_fx_probe_one
 *
 * Purpose: 16 Gbps FC-HBA Driver initialization
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
hfc_fx_probe_one(struct pci_dev *pdev, const struct pci_device_id *id, char *p)
{
	struct Scsi_Host *host=NULL,*hsd_host=NULL,*pfb_host=NULL;
	struct port_info *pp = NULL;
	struct port_info *wk_pp = NULL;
	struct port_info *vpp = NULL;
	
	int			error = -ENODEV;
	int			reg_chrdev = 0, rtn;
	uint64_t	dma_mask;
	ushort		bind_err = FALSE;
	uint		wk_err_num ;
	ulong		flags = 0;
	uchar		vrt_host_alloc = 0;
	int			i;
	uchar 		logdata[16];
	uint		skip_rport = 0, status=0;	/* FCLNX-GPL-FX-407 */
	int			msi_enable;

	hfclddconf = p;
	
	HFC_DBGPRT("hfcldd[%d] found on PCI bus %i, dev %i\n",
		(int)id->driver_data, pdev->bus->number, PCI_SLOT(pdev->devfn));

	memset(logdata, 0, 16);
	
	rtn = pci_enable_device(pdev);
	if(rtn) {
		logdata[0] = (uchar)rtn;
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x9A, logdata, 16) ;/* FCLNX-GPL-161 */
		goto hfc_probe_enable_error;
	}
	
	HFC_DBGPRT(" hfcldd : pci device is enabled irq=%d\n",pdev->irq);

	error = -ENOMEM;

	HFC_DBGPRT(" hfcldd : befire scsi_host_alloc\n");

	/* FCLNX-GPL-565 start *//* FCLNX-GPL-575 start */
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		host = hfc_fx_scsi_host_alloc(hfc_manage_info.hfcldd_driver_template_mp, sizeof(struct port_info));
	} 
	else {
		if( hfc_rport_lu_scan != 1){
			host = hfc_scsi_host_alloc(hfc_manage_info.hfcldd_driver_template_mp, sizeof(struct port_info));
		}
		else if( hfc_rport_lu_scan == 1){
			host = hfc_fx_scsi_host_alloc(hfc_manage_info.hfcldd_driver_template, sizeof(struct port_info));
		}
	}
	/* FCLNX-GPL-565 end *//* FCLNX-GPL-575 end */
	
	if ( host == NULL ){
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x37, NULL, 0) ;/* FCLNX-GPL-161 */
		goto hfc_fx_host_alloc_error;
	}

	HFC_DBGPRT( "  hfcldd : hfc_fx_detect - port_info pointer was set\n"); 
	pp = (struct port_info *)host->hostdata;
	memset(pp, 0, sizeof(struct port_info));
	pp->hfclddconf = hfclddconf;
	pp->pport = pp;
	
	/* Set structure character name */
	strcpy(pp->name, "port_info");
	
	pp->raslog_install = raslog_install;
#ifndef _HFC_NO_RASLOG
	if( raslog_install == 0 ){
		pp->raslog_ver = hraslogopt.ver;
		pp->raslog_rev = hraslogopt.rev;
		pp->raslog_rver = hraslogopt.rver;
		pp->raslog_wver = hraslogopt.wver;
	}
#endif

	/* Set SRAM CE LOG ID */ /* FCLNX-GPL-116 */
	wk_err_num = 0xfffe1100;
	HFC_4L_TO_4B(pp->ce_log[0].err_num, wk_err_num);

	/* set port_info */

#ifdef __x86_64			
	/* set dma_mask */
	if ((rtn=pci_set_dma_mask(pdev, 0xffffffffffffffffULL))==0) {
		HFC_DBGPRT(" hfcldd : Using 64bit DMA\n");
	} else if ((rtn=pci_set_dma_mask(pdev, 0xffffffffUL))==0) {
		HFC_DBGPRT(" hfcldd : Using 32bit DMA\n");
	} else {
		logdata[0] = (uchar)rtn;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC3, logdata, 16) ;
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
		logdata[0] = (uchar)rtn;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x9B, logdata, 16) ;/* FCLNX-GPL-161 */
		goto hfc_probe_error;
	}
	
	pp->dma_mask = dma_mask;	/* FCLNX-0671 *//* FCLNX-GPL-204 */

	pp->manage_info = &hfc_manage_info;
	
	pp->hosts = host;
	pp->host_no = host->host_no;
	pp->pci_cfginf = pdev;
			
	if (hfc_shadow == 1)
		pp->mlpf_mode |= HFC_MMODE_SHADOW;

	if(hfc_fx_query_devid(pp)){ /* Get Device ID (FPP or FIVE or FIVE-EX ?) */
		/* ERR(Unknown Device) */
		goto hfc_probe_error;
	}
	
	rtn = hfc_fx_pci_conf(pp);
	
	if (rtn != 0) {
		if( rtn != (-ENODEV) )															/* FCLNX-0357 */
		{
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC4, NULL, 0) ;/* FCLNX-GPL-161 */
		}																				/* FCLNX-0357 */
		goto hfc_probe_error;
	}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	host->transportt = hfc_fc_attach_transport;
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	if(hfc_fx_query_pktype(pp)){
		goto hfc_fx_pktype_error;
	}

	hfc_fx_get_adapter_port_no(pp);		/* Read adapter port# */
	if( hfc_fx_read_hfcbios(pp) ){		/* Read hbabios data */
		goto hfc_fx_attach_error; /* FCLNX-GPL-116 */
	}

	rtn = hfc_fx_search_adapter_number(pp);

	if ( rtn == MAX_ADAP_CNT + 1) { /* Incorrect Instance number is specified */
		/* FCLNX-0376 */
		if ( !HFC_FX_MMODE_CHECK_BASIC(pp) ) {	/* FCLNX-GPL-FX-407 */
			status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
			status &= ~HFC_HG_LPRDETAIL_SPACE;
			status |= (HFC_HG_LPRSTATUS_ISVALID | HFC_HG_LPRDETAIL_SHADOW_DED);
			if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
				status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );
		}	/* FCLNX-GPL-FX-407 */
		for (i=0; i<MAX_ADAP_CNT; i++) {
			if(hfc_manage_info.port_info_arg[i] == NULL){
				break;
			}
		}
		if (i == MAX_ADAP_CNT)
			goto hfc_fx_attach_error;
		pp->instance=i;
		skip_rport = 1;
//		goto skip_rport_start;
	}
	else if ( (rtn == -1) || (( pp->defparam == 1 )&&(pp->automap == 1)) ) { /* instance number is not specified or Force Default Setting */ /* FCLNX-0634 */
		for (i=0; i<MAX_ADAP_CNT; i++) {
			if ((hfc_manage_info.adap_info_arg[i] == NULL) && (hfc_manage_info.port_info_arg[i] == NULL)) {
				if((( pp->defparam == 1 )&&( pp->automap == 1 ))
				/* FCLNX-0634 */
				||(hfc_manage_info.adap_bind[i] == -1))
					break;
			}
		}
		if (i == MAX_ADAP_CNT)
			goto hfc_fx_attach_error;
		pp->instance = i;
	}
	else if (hfc_manage_info.port_info_arg[rtn] != NULL){	/*The instance number is also attached.*/ /* FCLNX-0634 */
		bind_err = TRUE;
		hfc_fx_errlog(NULL,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT3,0x50,NULL,0) ;/* FCLNX-0634 *//* FCLNX-GPL-161 */
		for (i=0; i<MAX_ADAP_CNT; i++) {
			if(hfc_manage_info.port_info_arg[i] == NULL){
				break;
			}
		}
		if (i == MAX_ADAP_CNT)
			goto hfc_fx_attach_error;
		pp->instance=i;
	}
										/* FCLNX-0476 */
	else {	/* instance number is specified */
		pp->instance = rtn;
	}

	if(( pp->defparam ==1 )&&( pp->automap== 1 )){	/* FCLNX-0630 */
		host->unique_id = hfc_manage_info.instance; 	/* FCLNX-0476 */
		pp->unique_id	= hfc_manage_info.instance; 	/* FCLNX-0476 */
		pp->dev_minor	= hfc_manage_info.instance; 	/* FCLNX-0476 */
	}
	else{	/* FCLNX-0630 */ 
		host->unique_id = pp->instance;                 /* FCLNX-0453 */
		pp->unique_id   = pp->instance;                 /* FCLNX-0453 */
		pp->dev_minor   = pp->instance;                 /* FCLNX-0453 */
	}  	/* FCLNX-0630 */

	/* FCLNX-0429 */	/* FCLNX-0651 */
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		vrt_host_alloc = hfc_manage_info.npubp->hfc_fx_mp_scsi_host_alloc(hfc_manage_info.hfcldd_driver_template_platform_bus, 
															host, pp, &hsd_host, &pfb_host);	/* FCLNX-GPL-473 */
		if(vrt_host_alloc == HFC_VRTHOST_ALLOC_FAIL) {	/* FCLNX-GPL-473 */
			goto hfc_probe_error;
		}
	}												/* FCLNX-GPL-204 */

	HFC_DBGPRT("instance_num = %d pp->instance = %d\n",instance, pp->instance);
	hfc_fx_conf_setup(pp);

	if(HFC_FX_MMODE_CHECK_MLPF(pp)&&(pp->hg_stats_disable == HFC_ENABLE_HGSTATS)){	/* FCLNX-GPL-494 */
		if(hfc_fx_alloc_mlpf_cca(pp)){
			pp->hg_stats_disable = HFC_DISABLE_HGSTATS;
		}
		hfc_fx_mlpf_cca_setup(pp);	/* FCLNX-GPL-507 */
	}																			/* FCLNX-GPL-494 */
			

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_set_retry_cnt(pp); 					/* FCLNX-0534 */
	}	/* FCLNX-0651*/

//	if((hfc_manage_info.hfcplus_enable)&&(HFC_FX_MMODE_CHECK_BASIC(pp))){	/* FCLNX-0506 */
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_isolconf_setup(pp);  /* FCLNX-0506 */
		if(hfc_manage_info.npubp->hfc_fx_allocate_errcnt_info(pp, GFP_KERNEL)){	/* FCLNX-GPL-FX-314 */
			goto hfc_fx_attach_error;
		}
		if( HFC_FX_MMODE_CHECK_SHARED(pp) ){	/* FCLNX-GPL-393 */
			hfc_fx_mlpf_check_isol_psycalport(pp);								/* FCLNX-GPL-393 */
		}																	/* FCLNX-GPL-393 */
	}
	else {
		if( HFC_FX_MMODE_CHECK_SHARED(pp) ){	/* FCLNX-GPL-393 */
			hfc_fx_mlpf_check_isol_psycalport(pp);								/* FCLNX-GPL-393 */
			if(!( HFC_FX_MMODE_CHECK_SHADOW(pp) ))
				hfc_fx_mlpf_set_errorlimit(pp);
		}																	/* FCLNX-GPL-393 */
		pp->scsi_to_retry=HFC_SCSI_TO_RETRY;
	}

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_mp_set_parm(host, pp, hsd_host, pfb_host, dma_mask); /* FCLNX-0521 */
	}
	
	if (HFC_FX_MQ_ENABLE(pp)) {					/* FCLNX-GPL-FX-204 */
		pp->max_vport_count = pp->mq_num -1;
	}
	
	if( hfc_fx_attach(pp) ){
		HFC_ERRPRT("hfcldd :  Failed to allocate adapter resource\n");
		goto hfc_fx_attach_error;
	}
	
	hfc_fx_rid_register(pp, NULL);
	
	/* Initialize package type and fw_init table */
	HFC_DBGPRT(" hfcldd : hfcl_probe_one - initialize package type \n");

	if ( hfc_fx_start_adapter(pp) ) {
		goto hfc_fx_attach_error;
	}

	set_bit(HFC_PS_ENABLE, (ulong *)&pp->status);							/* hfcends */
	pp->open_status = 0;												/* hfcends */

	/* Register special file */
	HFC_DBGPRT(" hfcldd : hfc_fx_detect - set device special file\n");
	if (hfc_manage_info.instance == 0) {
#if defined(HFC_RHEL7) || defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7)
		if (misc_register(&hfc_miscdev)) {
			HFC_DBGPRT(" hfcldd : hfc_fx_probe_one - misc_register failed\n");
		} else {	/* FCLNX-GPL-FX-492 start */
			HFC_DBGPRT(" hfcldd : hfc_fx_probe_one - misc_register success\n");
			hfc_major = MISC_MAJOR;
		}			/* FCLNX-GPL-FX-492 end */
#else
		hfc_major = register_chrdev(0, "hfcldd", &hfc_fops);
		if (0 > hfc_major) {
			HFC_DBGPRT("%s(): register_chrdev rc=%d\n", 	__func__, hfc_major);
			reg_chrdev = 1;
		}
		HFC_DBGPRT(" hfcldd : hfc_fx_detect - major # = %d\n", hfc_major);
#endif
	}
	
	pp->dev_major = hfc_major;
	hfc_manage_info.major = hfc_major;
	HFC_DBGPRT(" hfcldd : hfc_fx_detect - major # = %d, minor # = %d\n", pp->dev_major, pp->dev_minor);
	
	/* Update manage_info and port_info chain */
	spin_lock_irqsave(&hfc_manage_info.hfcmp_fx_lock, flags);
	hfc_manage_info.port_cnt++;
	hfc_manage_info.port_info_arg[pp->dev_minor] = pp;
	
	if (hfc_manage_info.pp == NULL) {
		hfc_manage_info.pp = pp;
		pp->prev = NULL;
		pp->next = NULL;
	}
	else {
		wk_pp = hfc_manage_info.pp;
		while (wk_pp->next != NULL) {
			wk_pp = wk_pp->next;
		}
		wk_pp->next = pp;
		pp->prev = wk_pp;
		pp->next = NULL;
	}
	spin_unlock_irqrestore(&hfc_manage_info.hfcmp_fx_lock, flags);
	
	if ( (hfc_manage_info.instance == 0)||(hfc_manage_info.socket_num == 0)) {
		/* Calcurate the number of CPUs at this server */
		hfc_fx_calc_cpu_num(pp);
	}
	else{
		pp->socket_num = hfc_manage_info.socket_num;
	}

	HFC_DBGPRT(" hfcldd : hfcl_detect hfc_manage_info.instance = %d\n", hfc_manage_info.instance);
	hfc_manage_info.instance++;

	hfc_manage_info.hfcldd_driver_template->can_queue = pp->can_queue;
	hfc_manage_info.hfcldd_driver_template->sg_tablesize = pp->sg_tblsize;
	
	pci_set_drvdata(pdev,host);				/* FCLNX-0290 */
	HFC_DBGPRT(" hfcldd : hfc_fx_detect scsi-add-host\n");
	rtn = scsi_add_host(host, &pdev->dev);
	if(rtn) {
		logdata[0] = (uchar)rtn;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xC6, logdata, 16) ;/* FCLNX-GPL-161 */
		goto error_disable_adapter;
	}							/* FCLNX-0290 */
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		if ( hfc_manage_info.npubp->hfc_fx_mp_scsi_add_host(host, pp, hsd_host, pfb_host) ){
			goto error_disable_adapter;
		}
	}											/* FCLNX-GPL-204 */
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* Set fixed fc host attributes */
	hfc_fx_fc_host_init(host, pp);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	msi_enable = pp->msi_enable;
	if ( (HFC_FX_MQ_ENABLE(pp)) && (pp->topology != HFC_FX_AL) ) {	/* FCLNX-GPL-FX-246,272 */
		if (msi_enable == HFC_INT_TYPE_MSIX) {		/* FCLNX-GPL-FX-203 */
			msi_enable = HFC_INT_TYPE_MSIX_MULTI;	/* FCLNX-GPL-FX-203 */
			if (hfc_fx_mq_attach(pp)) {
				msi_enable = HFC_INT_TYPE_MSIX;
			}
		}
	}
	
	/* Set Interrrupts(INTx or MSI or MSI-X) */
	pp->msi_flag = hfc_fx_set_interrupts(pp, msi_enable);
	if(pp->msi_flag < 0){ /* err */
		logdata[0] = (uchar)pp->msi_flag;
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4F, logdata, 16) ;/* FCLNX-GPL-161 */
		goto hfc_fx_irq_error;
	}
	
	/* Set vector_num */
	if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
		pp->vector_num = HFC_FX_MSIX_MULTIQUEUE;
	}
	else if ((pp->msi_flag == HFC_INT_TYPE_MSI) || (pp->msi_flag == HFC_INT_TYPE_MSIX)) {
		pp->vector_num = HFC_FX_NVEC_PER_PORT;
	}
	else {
		pp->vector_num = 1;
	}
	
	/* multi queue valid check */
	if (HFC_FX_MQ_ENABLE(pp)) {
		/* multi queue enable */
		if (pp->msi_flag == HFC_INT_TYPE_MSIX_MULTI) {
			/* multi queue valid */
			pp->mq_mode |= HFC_MQ_VALID;
		}
		else {
			/* multi queue invalid */
			hfc_fx_mq_detach(pp);
		}
	}
	
	/* Don't get adapLOCK before calling this function */
	HFC_PORTLOCK_IRQSAVE(pp,flags);	/* FCLNX-GPL-FX-466 */
	rtn =  hfc_fx_skip_link_init(pp, bind_err);
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);	/* FCLNX-GPL-FX-466 */
	if( rtn == 1 )
	{
		goto skip_rport_start;
	}
	else if( rtn == 2 ){
		goto skip_link_init;
	}
	
	if( skip_rport == 1 ){
		goto skip_link_init;
	}
	
	/* Determine master core */
	hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);
	
	/* Set fw_init_tbl */
	hfc_fx_set_fw_init_tbl(pp);
	
	hfc_fx_reset_start(pp, HFC_RESET_ALL_INT);	/* FCLNX-GPL-FX-262,272 */
	
	HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - start hfc_fx_core_start \n");
	hfc_fx_core_start(pp, 0);
	HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - end hfc_fx_core_start \n");
	
	HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - start hfc_fx_initialize \n");
	hfc_fx_initialize(pp, 0);				/* FCLNX-0514 */
	HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - end hfc_fx_initialize \n");
	
	if (HFC_FX_MQ_VALID(pp)) {
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg; 
			if (vpp == NULL)
				continue;
			
			/* copy from physical port */
			vpp->scsi_id = pp->scsi_id;
			
			/* Determine master core */
			hfc_fx_determine_master_core(vpp, vpp->region_arg[vpp->rid]);
			
			/* Set fw_init_tbl */
			hfc_fx_set_fw_init_tbl(vpp);
			
			set_bit(HFC_PS_ENABLE, (ulong *)&vpp->status);
			set_bit(HFC_ATTACH, (ulong *)&vpp->attach_status);
			set_bit(HFC_PD_NEED_CORE_START, (ulong *)&vpp->status_detail1);
			
			HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - start hfc_fx_core_start \n");
			hfc_fx_core_start(vpp, 0);
			HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - end hfc_fx_core_start \n");
			
			HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - start hfc_fx_initialize \n");
			hfc_fx_initialize(vpp, 0);				/* FCLNX-0514 */
			HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - end hfc_fx_initialize \n");
		}
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	if(pp->hba_isolation == HFC_ISOL_START){
		if(!(hfc_fx_check_hba_isolation(pp))){ 										/* FCLNX-GPL-349 */
			pp->hba_isolation = HFC_ISOL_STOP;
		}
	}
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_make_lgpath();								/* FCLNX-GPL-204 */
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_scan_host\n");
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		if(vrt_host_alloc == HFC_VRTHOST_ALLOC_SUCCS)							/* FCNLNX-GPL-473 */
			hfc_manage_info.npubp->hfc_fx_mp_scsi_scan_host(host, hsd_host, pfb_host);			/* FCLNX-0429 */
	}
	
	scsi_scan_host(host);
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_mp_scsi_host_rescan();						/* FCLNX-GPL-204 */
	}

skip_link_init:

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	if ( !hfc_manage_info.hfcldd_mp_mod ) {
		if (!test_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status )) { /* FCLNX-GPL-FX-491 */
			hfc_fx_lu_scan_start(pp);
		}
	}
#endif
#endif /* SYSFS_SUPPORT */

skip_rport_start:

	if(HFC_FX_MMODE_CHECK_SHADOW(pp))	/* FCLNX-GPL-393 */
	{
		hfc_fx_mlpf_check_isol_support(pp);
		
		if( HFC_FX_MMODE_CHECK_REBOOT(pp) ){
			/* Determine master core */
			hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);
			
			/* Set fw_init_tbl */
			hfc_fx_set_fw_init_tbl(pp);
			
			HFC_ALLLOCK_IRQSAVE(pp,pp->region_arg[pp->rid],flags);
			
			if(test_bit( HFC_PD_NEED_SHADOW_UP, (ulong *)&pp->status_detail2 ))
				hfc_fx_all_shadow_up(pp);
			
			HFC_ALLUNLOCK_IRQRESTORE(pp,pp->region_arg[pp->rid],flags);
		}
	}								/* FCLNX-GPL-393 */
	
	if(HFC_FX_MMODE_CHECK_SHARED(pp)){	/* FCLNX-GPL-489 */
		hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPAR_LIVEMIG_SUPPORT, HFC_ENABLE_DRV_SUPPORT);
	}								/* FCLNX-GPL-489 */
	
	HFC_EXIT("hfc_fx_detect");
	
	return 0;

hfc_fx_irq_error:
error_disable_adapter:

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_fx_stop_rport(pp); /* FCLNX-GPL-306 */
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

	if(reg_chrdev != 0){
#if !(defined(HFC_RHEL7)|| defined(HFC_X8664_SLES12)|| defined(HFC_X8664_OEL7) )
		unregister_chrdev(hfc_major, "hfcldd");
#endif
	}

hfc_fx_attach_error:
//	if(hfc_manage_info.hfcplus_enable){	/* FCLNX-0506 */
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_free_errcnt_info(pp);		/* FCLNX-0506 *//* FCLNX-GPL-FX-314 */
	}
hfc_fx_pktype_error:
	pci_release_regions(pdev);
	iounmap((void *)(pp->mem_base_addr));

hfc_probe_error:
	if (hsd_host != NULL) {												/* FCLNX-0429 */
		hfc_manage_info.npubp->hfc_fx_mp_scsi_host_put(hsd_host);			/* FCLNX-0429 */
	}
	
	hfc_fx_scsi_host_put(host);
		
hfc_fx_host_alloc_error:
	pci_disable_device(pdev);

hfc_probe_enable_error:

	return error;
}

/*
 * Function: hfc_fx_remove_one
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
void 
hfc_fx_remove_one(struct pci_dev *pdev)
{
	struct port_info *pp;
	struct port_info *wkpp;
	ulong					flags = 0;
	struct region_info		*rp;
	
	struct Scsi_Host *host = pci_get_drvdata(pdev);

	int i;
	
	pp = (struct port_info *)host->hostdata;
	rp =  pp->region_arg[pp->rid];

	HFC_DBGPRT("hfcldd hfc_fx_remove_one\n");

	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if( hfc_fx_pcibus_chk(pp) != 0 )
	{	/* "PCI BUS ERR" has hpppen. */
		if( !test_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ){
			HFC_FX_ISSUE_CSTP_PCIERR(pp);
		}
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/* virtual port terminate */
	for (i=1; i<=pp->max_vport_count; i++) {
		wkpp = pp->vport_ptr[i].vport_arg;
		if (wkpp == NULL)
			continue;
		
		if (wkpp->fc_vport == NULL)
			continue;
		
		fc_vport_terminate(wkpp->fc_vport);
	}
	
	fc_remove_host(host);
#endif
#endif /* SYSFS_SUPPORT */

	scsi_remove_host(host);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	hfc_fx_stop_rport(pp);
#endif
#endif /* SYSFS_SUPPORT */

	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_mp_scsi_remove_host(host);			/* FCLNX-0429 */
	}
	
	hfc_fx_release(host);
	pci_disable_device(pdev);
	hfc_fx_scsi_host_put(host);
	pci_set_drvdata(pdev,NULL);

//	for(i=0; i< HFC_FX_MAX_TMR ; i++){
//		HFC_DBGPRT("pp->wtimer_cnt[%d]=%d\n",i,pp->wtimer_cnt[i]);
//	}
	HFC_DBGPRT("hfc_fx_remove_one() - end");
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)

void hfc_fx_get_starget_node_name(struct scsi_target *starget)
{
	struct Scsi_Host	*host;
	struct port_info	*pp;
	struct target_info_fx	*target;
	struct region_info	*rp = NULL;
	ulong flags = 0;

	host = dev_to_shost(starget->dev.parent);
	if( host == NULL ){
		return;
	}
	
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_starget_node_name");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_starget_node_name() - pp null\n");
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if (host != NULL) {
		target = hfc_fx_hash_target_info(pp, starget->id );
		
		if (target != NULL) {
			fc_starget_node_name(starget) = target->node_name;
		}
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_starget_port_name(struct scsi_target *starget)
{
	struct Scsi_Host	*host;
	struct port_info	*pp;
	struct target_info_fx	*target;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	
	host = dev_to_shost(starget->dev.parent);
	if( host == NULL ){
		return;
	}
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_starget_port_name");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_starget_port_name() - pp null\n");
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if (host != NULL) {
		target = hfc_fx_hash_target_info(pp, starget->id );
		
		if (target != NULL) {
			fc_starget_port_name(starget) = target->ww_name;
		}
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_starget_port_id(struct scsi_target *starget)
{
	struct Scsi_Host	*host;
	struct port_info	*pp;
	struct target_info_fx	*target;
	struct region_info	*rp = NULL;
	ulong flags = 0;

	host = dev_to_shost(starget->dev.parent);
	if( host == NULL ){
		return;
	}
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_starget_port_id");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_starget_port_id() - pp null\n");
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if (host != NULL) {
		target = hfc_fx_hash_target_info(pp, starget->id );
		
		if (target != NULL) {
			fc_starget_port_id(starget) = target->scsi_id;
		}
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_host_port_id(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	
	pp = (struct port_info *)host->hostdata;
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_host_port_id() - pp null\n");
		return;
	}
	
	HFC_ENTRY("hfc_fx_get_host_port_id");
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	fc_host_port_id(host) = pp->scsi_id;
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_host_port_type(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_host_port_type");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_host_port_type() - pp null\n");
		fc_host_port_type(host) = FC_PORTTYPE_UNKNOWN;
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if( pp->connect_type == HFC_FX_PT2PT ){			/* PtoP & NotSwitch */
		fc_host_port_type(host) = FC_PORTTYPE_PTP ;
		HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_PTP\n");
	}
	else if ( pp->connect_type == HFC_FX_SWITCH ){	/* PtoP & Switch */
		fc_host_port_type(host) = FC_PORTTYPE_NPORT ;
		HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_NPORT\n");
	}
	else if ( pp->connect_type == HFC_FX_AL ){
		if ( pp -> scsi_id & 0x00ffff00 ){			/* AL & Switch */
			fc_host_port_type(host) = FC_PORTTYPE_NLPORT ;
			HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_NLPORT\n");
		}
		else {										/* AL & NotSwitch */
			fc_host_port_type(host) = FC_PORTTYPE_LPORT ;
			HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_LPORT\n");
		}
	}
	else if ( pp->connect_type == HFC_FX_MULTI_ALPA ){	/* AL & Switch */
		fc_host_port_type(host) = FC_PORTTYPE_NLPORT ;
		HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_NLPORT\n");
	}
	else if ( pp->connect_type == HFC_FX_F_PORT ){		/* PtoP & Switch */
		fc_host_port_type(host) = FC_PORTTYPE_NPORT ;
		HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_NPORT\n");
	}
	else {
		fc_host_port_type(host) = FC_PORTTYPE_UNKNOWN ;
		HFC_DBGPRT("hfc_fx_get_host_port_type() - FC_PORTTYPE_UNKNOWN\n");
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_host_port_state(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_port_state");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_host_port_state() - pp null\n");
		fc_host_port_state(host) = FC_PORTSTATE_UNKNOWN;
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) {
		fc_host_port_state(host) = FC_PORTSTATE_ERROR ;
		HFC_DBGPRT("hfc_fx_get_host_port_state() - FC_PORTSTATE_ERROR\n");
	}
	else if( !test_bit(HFC_PS_ONLINE , (ulong *)&pp->status) ) {
		fc_host_port_state(host) = FC_PORTSTATE_LINKDOWN ;
		HFC_DBGPRT("hfc_fx_get_host_port_state() - FC_PORTSTATE_LINKDOWN_1\n");
	}
	else if( test_bit(HFC_PS_WAIT_LINKUP,(ulong *)&pp->status )||test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)) {	/* FCLNX-GPL-FX-005 */
		fc_host_port_state(host) = FC_PORTSTATE_LINKDOWN ;
		HFC_DBGPRT("hfc_fx_get_host_port_state() - FC_PORTSTATE_LINKDOWN_2\n");
	}
	else {
		fc_host_port_state(host) = FC_PORTSTATE_ONLINE ;
		HFC_DBGPRT("hfc_fx_get_host_port_state() - FC_PORTSTATE_ONLINE\n");
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}

void hfc_fx_get_host_speed(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_host_speed");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_host_speed() - pp null\n");
		fc_host_port_type(host) = FC_PORTSPEED_UNKNOWN;
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if( !test_bit(HFC_PS_ONLINE, (ulong *)&pp->status) ) {
		fc_host_speed(host) = FC_PORTSPEED_UNKNOWN ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - HFC_ONLINE\n");
	}
	else if( pp->max_data_rate == HFC_100MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_1GBIT ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_1GBIT\n");
	}
	else if( pp->max_data_rate == HFC_200MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_2GBIT ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_2GBIT\n");
	}
	else if( pp->max_data_rate == HFC_400MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_4GBIT ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_4GBIT\n");
	}
	else if( pp->max_data_rate == HFC_800MBS ) {
		fc_host_speed(host) = 0x10 ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_8GBIT\n");
	}
	else if( pp->max_data_rate == HFC_1000MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_10GBIT ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_10GBIT\n");
	}
	else if( pp->max_data_rate == HFC_1600MBS ) {
		fc_host_speed(host) = FC_PORTSPEED_16GBIT ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_16GBIT\n");
	}
	else {
		fc_host_speed(host) = FC_PORTSPEED_UNKNOWN ;
		HFC_DBGPRT("hfc_fx_get_host_speed() - FC_PORTSPEED_UNKNOWN\n");
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
}


void hfc_fx_get_host_fabric_name(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	ulong flags = 0;
	u64 node_name =0;
	
	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_host_fabric_name");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_host_fabric_name() - pp null\n");
		return;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	if( pp->switch_exist ){
		node_name = (u64)pp->fabric_node_name;
	}
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	fc_host_fabric_name(host) = node_name;
}

struct fc_host_statistics *hfc_fx_get_statistics(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	struct core_info	*core[MAX_CORE_PROBE_FX] = {0};
	uchar				storecnt[MAX_CORE_PROBE_FX] = {0};
	uchar 				storeflag[MAX_CORE_PROBE_FX] = {0};
	int					i,j;
	ulong				seconds;
	ulong				flags = 0;
	uint				port_exec;

	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_get_statistics");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_get_statistics() - pp null\n");
		return NULL;
	}
	
	rp = pp->region_arg[pp->rid];
	
	if (rp == NULL) {
		HFC_DBGPRT("hfc_fx_get_statistics() - rp null\n");
		return NULL;
	}
	
	if (test_bit( HFC_PS_ISOL, (ulong *)&pp->status)){
		HFC_DBGPRT("hfc_fx_get_statistics() - isol\n");
		return NULL;
	}
	
	if (test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)){
		HFC_DBGPRT("hfc_fx_get_statistics() - mck recovery\n");
		return NULL;
	}
	
	if (!test_bit(HFC_SYSFS_STATISTICS, (ulong *)&pp->sysfs_control) ) {
		HFC_DBGPRT("hfc_fx_get_statistics() - sysfs statistics disable\n");
		return NULL;
	}
	
	port_exec = 0 ;
	port_exec |= (uint)(0x00001000) ;
	port_exec |= (uint)( (pp->rid << 16) & 0x00ff0000) ;
	port_exec |= HFC_FRAMEA_PORTSTATISTICS ;
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core[i] = rp->core_arg[i];
		if(!hfc_fx_check_cs_disable(pp, core[i])){	/* FCLNX-GPL-FX-438 */
			storecnt[i] = core[i]->fw_init_p->portstatistics.fw_store_count;
			storeflag[i] = 1;
			hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
					(char)0x4, (int)port_exec, HFC_FX_CORE_OFFSET40);
		}
	}
	if(storeflag[0] == 0 && storeflag[1] == 0 && storeflag[2] == 0 && storeflag[3] == 0){
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return NULL;
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	for(j=0 ; j<5000 ; j++){
		if(storeflag[0] == 0 && storeflag[1] == 0 && storeflag[2] == 0 && storeflag[3] == 0){
			break;
		}
		msleep(1);

		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(storecnt[i] != core[i]->fw_init_p->portstatistics.fw_store_count){
				storeflag[i] = 0;
			}
		}
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	memset(&pp->port_statistics,0,sizeof(struct fc_host_statistics));
	
	seconds = get_seconds();
	if (seconds < pp->reset_stat_time)
		pp->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - ((uint64_t)1 + (uint64_t)pp->reset_stat_time));
	else
		pp->port_statistics.seconds_since_last_reset = (uint64_t)((uint64_t)seconds - (uint64_t)pp->reset_stat_time);
	
	pp->port_statistics.tx_frames
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.tx_frames );
	pp->port_statistics.tx_words
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.tx_words );
	pp->port_statistics.rx_frames
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.rx_frames );
	pp->port_statistics.rx_words
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.rx_words );
	pp->port_statistics.lip_count
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.lip_count );
	pp->port_statistics.nos_count
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.nos_count );
	pp->port_statistics.link_failure_count
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.link_failure_count );
	pp->port_statistics.loss_of_sync_count
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.loss_of_sync_count );
	pp->port_statistics.loss_of_signal_count
		=(uint64_t)hfc_fx_read_val( core[pp->master_core_no]->fw_init_p->portstatistics.loss_of_signal_count );
	
	for (i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num) {
		pp->port_statistics.error_frames
			+=(uint64_t)hfc_fx_read_val( core[i]->fw_init_p->portstatistics.error_frames );
		pp->port_statistics.invalid_crc_count
			+=(uint64_t)hfc_fx_read_val( core[i]->fw_init_p->portstatistics.invalid_crc_count );
	}
	
	/* not support */
//	fw_init_p->portstatistics.dumped_frames 
//	fw_init_p->portstatistics.primitive_seq_protocol_err_count
//	fw_init_p->portstatistics.invalid_tx_word_count
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	return &pp->port_statistics;
}

void hfc_fx_reset_statistics(struct Scsi_Host *host)
{
	struct port_info	*pp;
	struct region_info	*rp = NULL;
	struct core_info	*core[MAX_CORE_PROBE_FX] = {0};
	int					i;
	ulong				flags = 0;
	uint				port_exec;

	pp = (struct port_info *)host->hostdata;
	
	HFC_ENTRY("hfc_fx_reset_statistics");
	
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_reset_statistics() - pp null\n");
		return;
	}
	
	rp = pp->region_arg[pp->rid];
	
	if (rp == NULL) {
		HFC_DBGPRT("hfc_fx_reset_statistics() - rp null\n");
		return;
	}
	
	if (test_bit( HFC_PS_ISOL, (ulong *)&pp->status)){
		HFC_DBGPRT("hfc_fx_reset_statistics() - isol\n");
		return;
	}
	
	if (test_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)){
		HFC_DBGPRT("hfc_fx_reset_statistics() - mck recovery\n");
		return;
	}
	
	if (!test_bit(HFC_SYSFS_STATISTICS, (ulong *)&pp->sysfs_control) ) {
		HFC_DBGPRT("hfc_fx_reset_statistics() - sysfs statistics disable\n");
		return;
	}
	
	port_exec = 0 ;
	port_exec |= (uint)(0x00008000) ;
	port_exec |= (uint)( (pp->rid << 16) & 0x00ff0000) ;
	port_exec |= HFC_FRAMEA_PORTSTATISTICS ;
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core[i] = rp->core_arg[i];
		if(!hfc_fx_check_cs_disable(pp, core[i])){	/* FCLNX-GPL-FX-438 */
			hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_FRAMEA,
					(char)0x4, (int)port_exec, HFC_FX_CORE_OFFSET40);
		}
	}
	
	pp->reset_stat_time = get_seconds();
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	HFC_EXIT("hfc_fx_reset_statistics");
}

int hfc_fx_issue_lip(struct Scsi_Host *host)
{
	struct port_info		*pp;
	struct region_info		*rp = NULL;
	ulong					flags = 0;
	int						i;
	
	HFC_ENTRY("hfc_fx_issue_lip");
	
	pp = (struct port_info *)host->hostdata;
	if (pp == NULL) {
		HFC_DBGPRT("hfc_fx_issue_lip() - pp null\n");
		return -EIO;
	}
	
	rp = pp->region_arg[0];
	if (rp == NULL) {
		HFC_DBGPRT("hfc_fx_issue_lip() - rp null\n");
		return -EIO;
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	
	if (test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->pport->status_detail2 )) {
		/* link_reset is running */
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return -EIO;
	}
	
	if (pp->pport->issue_lip == HFC_SYSFS_ISSUE_LIP) {
		/* issue lip is running */
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		return 0;
	}
	
	set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->pport->status_detail2 );
	pp->pport->issue_lip = HFC_SYSFS_ISSUE_LIP;
	hfc_fx_abend(pp->pport, rp->core_arg[pp->master_core_no], HFC_ABEND_LINK_RESET);
	
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	for(i=0 ; i<((HFC_FX_MCKINT_TO+1)*1000) ; i++){
		/* wait link_reset */
		if (!test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->pport->status_detail2 )) {
			break;
		}
		
		msleep(1);
	}
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	pp->pport->issue_lip = 0;
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	HFC_EXIT("hfc_fx_issue_lip");
	
	return 0;
}

void hfc_fx_rport_add(struct port_info *pp, struct target_info_fx *target)
{
	struct fc_rport *rport;
	struct fc_rport_identifiers rport_ids;
	
//	HFC_ERRPRT("hfc_fx_rport_add() target_id = %d\n",target->target_id);
	HFC_ERRPRT("  \n");
	if (!target) { /* FCLNX-GPL-205 */
		HFC_DBGPRT("hfc_fx_rport_add() - target null\n");
		return;
	}
	
	if (target->rport != NULL) {
		HFC_DBGPRT("hfc_fx_rport_add() - rport not null\n");
		return;
	}
	
	rport_ids.node_name = target->node_name;
	rport_ids.port_name = target->ww_name;
	rport_ids.port_id = target->scsi_id;
	rport_ids.roles = FC_RPORT_ROLE_UNKNOWN;
	
	rport = fc_remote_port_add(pp->hosts, 0, &rport_ids);
	if( target ){
		clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
	}
	
	target->rport = rport;
	
	if (!rport) {
		HFC_DBGPRT("hfc_fx_rport_add() - fc_remote_port_add failed\n");
		return;
	}
	
#if !(defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7))
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		rport->dev_loss_tmo = pp->dev_loss_tmo;	/* FCLNX-GPL-260 */
#endif
	
	rport_ids.roles |= FC_RPORT_ROLE_FCP_TARGET;
	
	if (rport_ids.roles !=  FC_RPORT_ROLE_UNKNOWN) {
		fc_remote_port_rolechg(rport, rport_ids.roles);
	}
	
	*((struct target_info_fx **)rport->dd_data) = target;
}

void hfc_fx_rport_delete(struct target_info_fx *target)
{
//	HFC_ERRPRT("hfc_fx_rport_delete() target_id = %d\n",target->target_id);
	HFC_ERRPRT("  \n");
	if (!target) { /* FCLNX-GPL-205 */
		HFC_DBGPRT("hfc_fx_rport_delete() - target null\n");
		return;
	}
	
	if( target->rport != NULL ){
		target->dev_loss_tmo = target->rport->dev_loss_tmo;
		target->fast_io_fail_tmo = target->rport->fast_io_fail_tmo;
	}
	
	if ((target->rport != NULL) && 	!test_bit(HFC_PD_WAIT_CLOSE, (ulong *)&target->pp->status_detail2)) {
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
		if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
			if( test_bit( HFC_PS_ISOL, (ulong *)&target->pp->status) )
			{
				target->rport->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
			}
			else if( target->pp->link_reset_multi_mode == 1 ){
				target->rport->dev_loss_tmo = HFC_MIN_DEV_LOSS_TMO;
			}
		}
#endif
#ifdef HFC_UBUNTU
		set_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
	HFC_DBGPRT("hfc_fx_rport_add() set_dev_flag target_id = %d\n",target->target_id);
#endif
		fc_remote_port_delete(target->rport);
		target->rport = NULL;
		HFC_ERRPRT("  \n");
	}
}

void hfc_fx_fc_host_init(struct Scsi_Host *host, struct port_info *pp)
{
	fc_host_node_name(host)			= pp->node_name;
	fc_host_port_name(host)			= pp->ww_name;
	fc_host_supported_classes(host) = HFC_SUPPORT_CLASS;
	
	memset(fc_host_supported_fc4s(host), 0,sizeof(fc_host_supported_fc4s(host)));
	fc_host_supported_fc4s(host)[2] = 0x01;
	
	fc_host_supported_speeds(host) |= (	FC_PORTSPEED_1GBIT	|
										FC_PORTSPEED_2GBIT	|
										FC_PORTSPEED_4GBIT	|
										FC_PORTSPEED_8GBIT	|
										FC_PORTSPEED_10GBIT |
										FC_PORTSPEED_16GBIT);

	fc_host_maxframe_size(host) = HFC_PORT_MAX_FRAME;
	fc_host_max_npiv_vports(host) = pp->max_vport_count;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	fc_host_dev_loss_tmo(host) = pp->dev_loss_tmo;	/* FCLNX-GPL-564 */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37) */ /* FCLNX-GPL-564 */

	pp->reset_stat_time = get_seconds();
}
#endif // LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/*
 * Function: hfc_fx_reset_all_timer
 *
 * Purpose: Reset all timer (port, target)
 *
 * Arguments:
 *  pp	 - Pointer to port_info
 *
 * Returns: -
 *
 * Notes:
 */
void hfc_fx_reset_all_timer (struct port_info *pp)
{
	int						i;
	struct target_info_fx	*target;
	struct wtimer_fx		*w_timer;
	struct dev_info_fx		*dev;	/* FCLNX-GPL-353 */
	struct region_info		*rp = NULL;
	struct core_info		*core = NULL;

	rp = pp->region_arg[pp->rid];

	/* Stop timer (port_info) */
	for(i=0; i< HFC_FX_MAX_TMR ; i++){
		switch(i){
		case HFC_FX_LINKINIT_TMR :
			w_timer = &pp->link_init_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MB_RETRY_TMR :
			w_timer = &pp->mb_retry_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MB_DELAY_TMR :
			w_timer = &pp->mb_delay_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_CTLRST_DELAY_TMR :
			break;			
		case HFC_FX_REBOOT_DELAY_TMR :
			w_timer = &pp->reboot_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MCKINT_TMR :
			w_timer = &pp->mckint_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MLPF_FMCK_TMR :
			w_timer = &pp->fmck_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MLPF_FCSTP_TMR :
			w_timer = &pp->fcstp_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_LINKUP_TMR :
			w_timer = &pp->linkup_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_DIAG_DELAY_TMR :
			w_timer = &pp->reboot_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_LOGIN_DELAY_TMR :
			w_timer = &pp->lgdelay_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_LDLERR_TMR :
			w_timer = &pp->ldlerr_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_LDSERR_TMR :
			w_timer = &pp->ldserr_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_IFERR_TMR :
			w_timer = &pp->iferr_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_TOERR_TMR :
			w_timer = &pp->toerr_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_INT_CHECK_TMR :
			w_timer = &pp->int_chk_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_MLPF_ISOLEND_TMR :
			w_timer = &pp->isolend_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
				break;
		case HFC_FX_WLINKUP_MCK_TMR :
			w_timer = &pp->linkup_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		case HFC_FX_WLINKUP_CNT_TMR :	/* FCLNX-GPL-FX-424 */
			w_timer = &pp->ld_err_wdog;
			if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
				hfc_fx_watchdog_enter( pp, NULL, NULL, NULL, 0, i, 0, 1);
			break;
		default:
			break;
		}
	}

	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		core = rp->core_arg[i];
		w_timer = &core->core_mb_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, core, NULL, NULL, 0, HFC_FX_MB_RSP_TMR, 0, 1);
		w_timer = &core->mb_retry_intvl_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, core, NULL, NULL, 0, HFC_FX_MB_RETRY_DELAY_TMR, 0, 1);
		w_timer = &core->wexec_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, core, NULL, NULL, 0, HFC_FX_WEXEC_TMR, 0, 1);
	}

	for (i=0;i<pp->max_target;i++){
		if(pp->target_arg[i]==NULL)
			continue;
		target = pp -> target_arg[i];

		/* Cancel scn linkup timer */	
		w_timer = &target->scnlinkup_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
		hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_SCN_LINKUP_TMR, 0, 1);

		/* Cancel delay timer */
		w_timer = &target->delay_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
		hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_DELAY_TMR, 0, 1);

		/* Cancel isolate timer for Target*/
		w_timer = &target->tgt_ldlerr_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_TGT_LDLERR_TMR, 0, 1);

		w_timer = &target->tgt_ldserr_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_TGT_LDSERR_TMR, 0, 1);

		/* cancel restart tmer */
		w_timer = &target->restart_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
			hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_RESTART_TMR, 0, 1);

		/* FCLNX-GPL-FX Start */
		/* Cancel Target Reset process total timer */
		w_timer = &target->total_tgtrst_wdog;
		if( (w_timer != NULL) && (w_timer->timer_flag & HFC_TIMER_VALID) )
		hfc_fx_watchdog_enter( pp, NULL, target, NULL, 0, HFC_FX_TOTAL_TGTRST_TMR, 0, 1);
		/* FCLNX-GPL-FX End */
		
		dev = target->dev;
		if(dev != NULL) {
			/* stop LUN Reset Delay Timer */
//			hfc_manage_info.npubp->hfc_fx_all_clear_dev_info_fx(pp, dev);
			hfc_fx_all_clear_dev_info_fx(pp, dev);
		}																			/* FCLNX-GPL-038	*/
	}
}


/*
 * Function:    hfc_fx_set_interrupts
 *
 * Purpose:     This function sets interrupts.
 *
 * Arguments:
 *  pp         - pointer to port_info
 *  type       - Requested INT type
 *
 * Returns:
 *  type       - Enabled INT type
 *
 * INT type values:
 *                0: INTx
 *                1: MSI
 *                2: MSI-X
 *                3: MSI-X (Multi Queue)
 *                4: MSI (Vector Shortage)
 *                5; MSI-X (Vector Shortage)
 *               -1: Failed
 * Notes:
 */
int hfc_fx_set_interrupts(struct port_info *pp, int type)
{
	/* Values */
	struct pci_dev *pdev = pp->pci_cfginf;
	uint pos,dat_wk;
	int i, err;
	uchar logdata[16],chr_wk;

	HFC_DBGPRT("*hfcldd : hfc_fx_set_interrupts: required inttype=%d, pp=%p",type,pp);

	/*
	 * Set interruption type and require IRQ
	 */

	/****** INT type is MSI-X Multiqueue ************/
	if( type == HFC_INT_TYPE_MSIX_MULTI) {
			/* Search Capabitilies ID */
		pos = pci_find_capability(pdev, PCI_CAP_ID_MSIX);
		if(pos == 0){ /* This HBA don't support MSI-X.*/
				memset(logdata, 0, 16);
				logdata[0] = 0x01 ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				type = HFC_INT_TYPE_INTX;
		}else{
			/* MCW configuration (Enable RSS mode for this function) */
			chr_wk  = hfc_fx_read_reg_ext ( pp, 0x23c, 0x1 );
			chr_wk |= (0x80) >> PCI_FUNC(pp->pci_cfginf->devfn);
			hfc_fx_write_reg_ext( pp, 0x23c, 0x1, chr_wk );
			/* MSI-X Table configuration (vector number) */
			dat_wk = 0x0DC00060 | (0x00000400 * PCI_FUNC(pp->pci_cfginf->devfn) );
			hfc_fx_write_indarea(
				pp,
				pp->master_core_no, /* core_no */
				dat_wk,     /* offset */
				0x00001f00, /* data */
				0x20        /* rammask */
				);
			/* Enable MSI-X (Multi Queue) */
			err = hfc_fx_set_intr_entry(pp,type);
			if(err != 0){ /* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x02 ;
				logdata[1] = (uchar)err ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				if (err > 0) {
					pci_disable_msix(pdev);
					type = HFC_INT_TYPE_MSIX;
				}
				else {
					type = HFC_INT_TYPE_INTX;
				}
			}else{
				/* Request IRQ */
				for(i=0; i<HFC_FX_MSIX_MULTIQUEUE; i++){
					err = request_irq(pp->intr_entries[i].vector, hfc_fx_intr_share, 0,
									  pp->intr_entries[i].name, &pp->intr_entries[i]);
					if(err != 0){ /* There are some err. */
						memset(logdata, 0, 16);
						logdata[0] = 0x03 ;
						logdata[1] = (uchar)err ;
						hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
						hfc_fx_free_interrupts(pp, type, i, 0);
						/* Try INTx */
						type = HFC_INT_TYPE_INTX;
						break;
					}
				}
			}
			if ((type == HFC_INT_TYPE_INTX) || (type == HFC_INT_TYPE_MSIX)) {
				/* Disable MSI-X (Multi Queue) */
				chr_wk  = hfc_fx_read_reg_ext ( pp, 0x23c, 0x1 );
				chr_wk &= ~( (0x80) >> PCI_FUNC(pp->pci_cfginf->devfn) );
				hfc_fx_write_reg_ext( pp, 0x23c, 0x1, chr_wk );
				/* MSI-X Table configuration (vector number) */
				dat_wk = 0x0DC00060 & ~(0x00000400 * PCI_FUNC(pp->pci_cfginf->devfn) );
				hfc_fx_write_indarea(
					pp,
					pp->master_core_no, /* core_no */
					dat_wk,     /* offset */
					0x00000100 * pp->core_num, /* data */
					0x20        /* rammask */
					);
			}
		}
	}
	/****** INT type is MSI-X ***********************/
	if( type == HFC_INT_TYPE_MSIX ) {
		/* Search Capabitilies ID */
		pos = pci_find_capability(pdev, PCI_CAP_ID_MSIX);
		if(pos == 0){ /* This HBA don't support MSI-X.*/
				memset(logdata, 0, 16);
				logdata[0] = 0x04 ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				type = HFC_INT_TYPE_INTX;
		}else{/* Set msix_entry */
			/* Enable MSI-X */
			err = hfc_fx_set_intr_entry(pp,type);
			if(err > 0){
				type = HFC_INT_TYPE_MSIX_SHORTAGE;
			}else if(err < 0){ /* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x05 ;
				logdata[1] = (uchar)err ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				type = HFC_INT_TYPE_INTX;
			}else{
				/* Request IRQ */
				for(i=0; i<HFC_FX_NVEC_PER_PORT; i++){
					if( i < pp->core_num ){
						err = request_irq(pp->intr_entries[i].vector, hfc_fx_intr_xrb, 0,
										  pp->intr_entries[i].name, &pp->intr_entries[i]);
					}else{
						err = request_irq(pp->intr_entries[i].vector, hfc_fx_intr_share, 0,
										  pp->intr_entries[i].name, &pp->intr_entries[i]);
					}
					if(err != 0){ /* There are some err. */
						memset(logdata, 0, 16);
						logdata[0] = 0x06 ;
						logdata[1] = (uchar)err ;
						hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
						hfc_fx_free_interrupts(pp, type, i, 0);
						type = HFC_INT_TYPE_MSIX_SHORTAGE;
						break;
					}
				}
			}
		}
	}
	/****** INT type is MSI *************************/
	if( type == HFC_INT_TYPE_MSI) {
		HFC_DBGPRT("INT type is MSI, pdev->irq=%d\n",pdev->irq);
			/* Search Capabitilies ID */
//		pos = pci_find_capability(pdev, PCI_CAP_ID_MSI);
		pos = 1;
		if(pos == 0){ /* This HBA don't support MSI.*/
				memset(logdata, 0, 16);
				logdata[0] = 0x07 ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				type = HFC_INT_TYPE_INTX;
		}else{
			/* Enable MSI */
			err = hfc_fx_set_intr_entry(pp,type);
			if(err > 0){
				type = HFC_INT_TYPE_MSI_SHORTAGE;
			}else if(err < 0){ /*  There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x08 ;
				logdata[1] = (uchar)err ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				type = HFC_INT_TYPE_INTX;
			}else{
				/* Request IRQ */
				i=0;
				for(i=0; i<HFC_FX_NVEC_PER_PORT; i++) {
					if(i < pp->core_num){
						err = request_irq(pdev->irq + i, hfc_fx_intr_xrb, 0,
										  pp->intr_entries[i].name, &pp->intr_entries[i]);
					}else{
						err = request_irq(pdev->irq + i, hfc_fx_intr_share, 0,
										  pp->intr_entries[i].name, &pp->intr_entries[i]);
					}
					if(err != 0){ /* There are some err. */
						memset(logdata, 0, 16);
						logdata[0] = 0x09 ;
						logdata[1] = (uchar)err ;
						hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
						hfc_fx_free_interrupts(pp, type, i, 0);
						type = HFC_INT_TYPE_MSI_SHORTAGE;
						break;
					}
				}
			}
		}
	}
	/****** INT type is MSIX_SHORTAGE *************************/
	if( type == HFC_INT_TYPE_MSIX_SHORTAGE) {
		/* Set MSI Vector Shortage bit */
		chr_wk  = hfc_fx_read_reg ( pp, ( uint )HFC_IOSPACE_MSIXVSHORT,( char )0x1 );
		chr_wk |= 0x80;
		hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_MSIXVSHORT, ( char )0x1, ( char )chr_wk );
		err = hfc_fx_set_intr_entry(pp,type);
		if(err < 0){ /* There are some err. */
			memset(logdata, 0, 16);
			logdata[0] = 0x0A ;
			logdata[1] = (uchar)err ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
			type = HFC_INT_TYPE_INTX;
		}else{
			/* Request IRQ */
			err = request_irq(pp->intr_entries[0].vector, hfc_fx_intr_share, 0, pp->intr_entries[0].name, &pp->intr_entries[0]);
			if(err != 0){ /* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x0B ;
					logdata[1] = (uchar)err ;
					hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				hfc_fx_free_interrupts(pp, type, 0, 0);
				/* Try INTx */
					type = HFC_INT_TYPE_INTX;
			}
		}
	}
	/****** INT type is MSI_SHORTAGE *************************/
	if( type == HFC_INT_TYPE_MSI_SHORTAGE) {
		err = hfc_fx_set_intr_entry(pp,type);
		if(err < 0){ /* There are some err. */
				memset(logdata, 0, 16);
			logdata[0] = 0x0C ;
			logdata[1] = (uchar)err ;
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
			type = HFC_INT_TYPE_INTX;
		}else{
			/* Request IRQ*/
			err = request_irq(pdev->irq, hfc_fx_intr_share, 0, pp->intr_entries[0].name, &pp->intr_entries[0]);
			if(err != 0){ /* There are some err. */
				memset(logdata, 0, 16);
				logdata[0] = 0x0D ;
				logdata[1] = (uchar)err ;
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xB0, logdata, 16) ;
				hfc_fx_free_interrupts(pp, type, 0, 0);
				type = HFC_INT_TYPE_INTX;
			}
		}
	}
	/****** INT type is INTx ************************/
	if( type == HFC_INT_TYPE_INTX ) {
		hfc_fx_set_intr_entry(pp,type);
		err = request_irq(pdev->irq, hfc_fx_intr_share,
						  IRQF_SHARED, pp->intr_entries[0].name, &pp->intr_entries[0]);
		if(err != 0){ /* There are some err. */
			return -1;
		}
	}
	HFC_DBGPRT(" hfcldd : hfc_fx_set_interrupts: used inttype=%d, pp=%p",type,pp);

	return type;
}


static inline struct cpu_info* hfc_fx_get_min_mapped_cpu_info(struct port_info *pp, int entry_idx){
	ushort					socket_num;
	ushort					cpu_core_num;
	struct socket_info		*socket_info;
	struct socket_info		*min_mapped_socket_info;
	struct cpu_info			*min_mapped_cpu_info;
	int i, min_map_cnt;

	socket_num = pp->manage_info->socket_num;
	cpu_core_num = pp->manage_info->cpu_core_num;
	socket_info = pp->manage_info->socket_info;

	min_mapped_socket_info = NULL;
	min_map_cnt = INT_MAX;
	
	if( ( pp->core_num == 1 )||( socket_num == 1 ) ){
		for(i=0; i<socket_num; i++){
			if( socket_info[i].socket_no != HFC_SOCKET_NO_INVALID
			  && socket_info[i].map_count < min_map_cnt){
				min_map_cnt = socket_info[i].map_count;
				min_mapped_socket_info = &socket_info[i];
				break;
			}
		}
	}
	else{
		if( pp->core_num == 4 ){
			if( entry_idx < 2 ){
				for(i=0; i<socket_num; i++){
					if( socket_info[i].socket_no != HFC_SOCKET_NO_INVALID
			  		  && socket_info[i].map_count < min_map_cnt){
						min_map_cnt = socket_info[i].map_count;
						min_mapped_socket_info = &socket_info[i];
						break;
					}
				}
			}
			else{
				for(i=1; i<socket_num; i++){
					if( socket_info[i].socket_no != HFC_SOCKET_NO_INVALID
			  		  && socket_info[i].map_count < min_map_cnt){
						min_map_cnt = socket_info[i].map_count;
						min_mapped_socket_info = &socket_info[i];
						break;
					}
				}
			}
		}
		else if( pp->core_num == 2 ){
			if( entry_idx < 1 ){
				for(i=0; i<socket_num; i++){
					if( socket_info[i].socket_no != HFC_SOCKET_NO_INVALID
			  		  && socket_info[i].map_count < min_map_cnt){
						min_map_cnt = socket_info[i].map_count;
						min_mapped_socket_info = &socket_info[i];
						break;
					}
				}
			}
			else{
				for(i=1; i<socket_num; i++){
					if( socket_info[i].socket_no != HFC_SOCKET_NO_INVALID
			  		  && socket_info[i].map_count < min_map_cnt){
						min_map_cnt = socket_info[i].map_count;
						min_mapped_socket_info = &socket_info[i];
						break;
					}
				}
			}
		}
	}

	min_mapped_cpu_info = NULL;
	min_map_cnt = INT_MAX;
	for(i=0; i<min_mapped_socket_info->cpu_info_list_num; i++){
		if( min_mapped_socket_info->cpu_into_list[i].cpu_no != HFC_FX_CPU_NO_INVALID
				&& min_mapped_socket_info->cpu_into_list[i].map_count < min_map_cnt){
			min_map_cnt = min_mapped_socket_info->cpu_into_list[i].map_count;
			min_mapped_cpu_info = &(min_mapped_socket_info->cpu_into_list[i]);
		}
	}

	if( min_mapped_socket_info != NULL ){
		min_mapped_socket_info->map_count++;
	}
	if( min_mapped_cpu_info != NULL ){
		min_mapped_cpu_info->map_count++;
	}

	return min_mapped_cpu_info;
}


int hfc_fx_set_intr_entry(struct port_info *pp,int type){
	struct pci_dev *pdev = pp->pci_cfginf;
	int i,j,err,nvec,entry_idx;
	struct cpu_info *min_mapped_cpu_info;
	
	/* Clear intr_entries[] array */
	for(i=0; i<HFC_FX_MSIX_NVEC ; i++){
		pp->intr_entries[i].entry_num = 0;
		pp->intr_entries[i].vector    = 0;
		pp->intr_entries[i].mode      = 0;
		pp->intr_entries[i].core      = 0;
		pp->intr_entries[i].shr_info  = 0;
		pp->intr_entries[i].pp        = NULL;
		pp->intr_entries[i].handle    = NULL;
		cpumask_setall(&pp->intr_entries[i].affinity); /* FCLNX-GPL-FX-420 */
		pp->intr_entries[i].cpu_no = 0;
		pp->intr_entries[i].socket_no = 0;
		strcpy(pp->intr_entries[i].name, "");
	}

	/* Set necessary vector number to nvec */
	if( type == HFC_INT_TYPE_MSI ||
	    type == HFC_INT_TYPE_MSIX )
		nvec = HFC_FX_NVEC_PER_PORT;
	else if( type == HFC_INT_TYPE_MSIX_MULTI)
		nvec = HFC_FX_MSIX_MULTIQUEUE;
	else
		nvec = HFC_FX_NVEC_ONE;

	/* Initialize entry number and vector of msix entry array */
	for(i=0; i<HFC_FX_MSIX_NVEC; i++){
		if(i < nvec)
			pp->entries[i].entry  = i;
		else
			pp->entries[i].entry  = 0;
		pp->entries[i].vector = 0;
	}

	/* Enable MSI or MSIX function */
	err = 0;
	if( type == HFC_INT_TYPE_MSIX ||
		type == HFC_INT_TYPE_MSIX_SHORTAGE ||
		type == HFC_INT_TYPE_MSIX_MULTI)
		err = pci_enable_msix(pdev, pp->entries, nvec);
	else if( type == HFC_INT_TYPE_MSI)
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0) /* FCLNX-GPL-FX-496 start */
		err = pci_enable_msi_block(pdev, pp->core_num * 2);
#else
		err = pci_enable_msi_exact(pdev, (int)pp->core_num * 2);
#endif /* FCLNX-GPL-FX-496 end */
	else if( type == HFC_INT_TYPE_MSI_SHORTAGE )
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
		err = pci_enable_msi_block(pdev, nvec); /* FCLNX-GPL-FX-496 start */
#else
		err = pci_enable_msi_exact(pdev, (int)nvec);
#endif /* FCLNX-GPL-FX-496 end */

	if(err){
		HFC_DBGPRT("hfcldd%d: %s, %d: some error occured. errno=%d\n",pp->dev_minor,__func__,__LINE__,err);
		return err;
	}
	
	/* Disable cpu_map set mode *//* FCLNX-GPL-FX-420 */
	if( type != HFC_INT_TYPE_MSIX ){
		pp->cpu_map = HFC_VEC_CPU_MAP_DISABLE;
	}

	/* INT type is MSI or MSI-X */
	if( type == HFC_INT_TYPE_MSI || type == HFC_INT_TYPE_MSIX ){
		j = 0;
		for(i=0;i<nvec;i++){
			pp->intr_entries[i].entry_num	= i ;
			if( type == HFC_INT_TYPE_MSI ){
				pp->intr_entries[i].vector	= pdev->irq + i ;
			}else{ /* MSI-X */
				pp->intr_entries[i].vector	= pp->entries[i].vector ;
			}
			pp->intr_entries[i].mode		= type;
			pp->intr_entries[i].pp			= pp;
			if( i < pp->core_num ){
				pp->intr_entries[i].core	= j;
				pp->intr_entries[i].handle	= (irqreturn_t*)hfc_fx_intr_xrb ;
				sprintf(pp->intr_entries[i].name, "hfc_fx_intr_xrb:%d:%d", pp->instance, j);
			}else if( i == pp->core_num ){
				if( i == 0)
					pp->intr_entries[i].core= 0x00;
				else
					pp->intr_entries[i].core	= 0x80;
				
				pp->intr_entries[i].handle	= (irqreturn_t*)hfc_fx_intr_share ;
				sprintf(pp->intr_entries[i].name, "hfc_fx_intr_share:%d", pp->instance);
			}else{
				pp->intr_entries[i].core	= 0x80;
				pp->intr_entries[i].handle	= NULL;
			}
			j += (MAX_CORE_PROBE_FX/pp->core_num) ;
		}
		if( pp->core_num == 4 ){
			pp->intr_entries[0].shr_info	= 0x20000000;
			pp->intr_entries[1].shr_info	= 0x00200000;
			pp->intr_entries[2].shr_info	= 0x00002000;
			pp->intr_entries[3].shr_info	= 0x00000020;
			pp->intr_entries[4].shr_info	= 0xC5C5C5C5;
		}else if( pp->core_num == 2 ){
			pp->intr_entries[0].shr_info	= 0x20000000;
			pp->intr_entries[1].shr_info	= 0x00002000;
			pp->intr_entries[2].shr_info	= 0xC500C500;
		}else{ /* pp->core_num == 1 */
			pp->intr_entries[0].shr_info	= 0x20000000;
			pp->intr_entries[1].shr_info	= 0xC5000000;
		}
		
		if(pp->cpu_map == HFC_VEC_CPU_MAP_ENABLE){
			/* Vector - CPU mapping */
			for(entry_idx=0; entry_idx<(nvec-1); entry_idx++){
				min_mapped_cpu_info = hfc_fx_get_min_mapped_cpu_info(pp, entry_idx);
				if( min_mapped_cpu_info != NULL ){
					cpumask_clear(&pp->intr_entries[entry_idx].affinity);
					cpumask_set_cpu(min_mapped_cpu_info->cpu_no, &pp->intr_entries[entry_idx].affinity);
					pp->intr_entries[entry_idx].cpu_no = min_mapped_cpu_info->cpu_no;
					pp->intr_entries[entry_idx].socket_no = min_mapped_cpu_info->socket_info->socket_no;
					HFC_DBGPRT("hfcldd%d entry_idx=%d cpu_no = %04x socket_no %04x\n", pp->dev_minor,
						entry_idx, pp->intr_entries[entry_idx].cpu_no, pp->intr_entries[entry_idx].socket_no);
				}
			}
		}
	}

	/* INT type is MSI-X(Vector Share) or MSI-X(Multi Queue) or MSI(Vector Shortage) or INTx */
	if( type == HFC_INT_TYPE_MSIX_SHORTAGE ||
		type == HFC_INT_TYPE_MSIX_MULTI ||
		type == HFC_INT_TYPE_MSI_SHORTAGE ||
		type == HFC_INT_TYPE_INTX ){
		for(i=0;i<nvec;i++){
			pp->intr_entries[i].entry_num		= i ;
			if( type == HFC_INT_TYPE_MSIX_SHORTAGE || type == HFC_INT_TYPE_MSIX_MULTI ){
				/* INT type is MSI-X(Vector Share) or MSI-X(Multi Queue) */
				pp->intr_entries[i].vector		= pp->entries[i].vector ;
			}else{
				/* INT type is MSI(Vector Shortage) or INTx */
				pp->intr_entries[i].vector		= pdev->irq ;
			}
			pp->intr_entries[i].mode			= type;
			/* Set core */
			if(pp->core_num == 1){
				pp->intr_entries[i].core		= 0x00 ;
			}else{
				pp->intr_entries[i].core		= 0x80 ;
			}
			/* Set shr_info */
			if(pp->core_num == 1){
				pp->intr_entries[i].shr_info	= 0xE5000000;
			}else if(pp->core_num == 2){
				pp->intr_entries[i].shr_info	= 0xE500E500;
			}else{/* pp->core_num == 4 */
				pp->intr_entries[i].shr_info	= 0xE5E5E5E5;
			}
			pp->intr_entries[i].pp				= pp;
			pp->intr_entries[i].handle			= (irqreturn_t*)hfc_fx_intr_share ;
			if( HFC_INT_TYPE_MSIX_MULTI ){
				sprintf(pp->intr_entries[i].name, "hfc_fx_intr_share:%d:%d", pp->instance, i);
			}
			else{
				sprintf(pp->intr_entries[i].name, "hfc_fx_intr_share:%d", pp->instance);
			}
		}
	}
	return 0;
}


/*
 * Function:    hfc_fx_free_interrupts
 *
 * Purpose:     This Function releases interrupts.
 *
 * Arguments:
 *  pp         - pointer to port_info
 *  type
 *
 * INT type values:
 *                0: INTx
 *                1: MSI
 *                2: MSI-X
 *                3: MSI-X (Multi Queue)
 *                4: MSI (Vector Shortage)
 *                5; MSI-X (Vector Share)
 *               -1: Failed
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_fx_free_interrupts(struct port_info *pp, int type, int nvec, int pci_fail)
{
	/* Values */
	struct pci_dev *pdev = pp->pci_cfginf;
	int i;
	uint dat_wk;
	uchar chr_wk;

	switch(type){
	/****** INT type is INTx ***********************/
	case	HFC_INT_TYPE_INTX:
		if(pdev->irq)
		free_irq(pdev->irq, &pp->intr_entries[0]);
		break;

	/****** INT type is MSI or MSI Vector Shortage */
	case HFC_INT_TYPE_MSI:
	case HFC_INT_TYPE_MSI_SHORTAGE :
		for(i=0; i<nvec; i++)
			free_irq(pdev->irq + i, &pp->intr_entries[i]);
		if(pci_fail == 0)	
		pci_disable_msi(pdev);
		break;

	/****** INT type is MSI-X *********************/
	case	HFC_INT_TYPE_MSIX:
		for(i=0; i < nvec; i++)
			free_irq(pp->intr_entries[i].vector, &pp->intr_entries[i]);
		if(pci_fail == 0)
			pci_disable_msix(pdev);
		break;

	/****** INT type is MSI-X Vector Shortage ********/
	case	HFC_INT_TYPE_MSIX_SHORTAGE:
		if(nvec != 0)
			free_irq(pp->intr_entries[0].vector, &pp->intr_entries[0]);
		if(pci_fail == 0) pci_disable_msix(pdev);
			chr_wk  = hfc_fx_read_reg ( pp, ( uint )HFC_IOSPACE_MSIXVSHORT,( char )0x1 );
			chr_wk &= ~(0x80);
			hfc_fx_write_reg( pp, ( uint )HFC_IOSPACE_MSIXVSHORT, ( char )0x1, ( char )chr_wk );
		break;

	/****** INT type is MSI-X Multiqueue **********/
	case	HFC_INT_TYPE_MSIX_MULTI:
		for(i=0; i < nvec; i++)
			free_irq(pp->intr_entries[i].vector, &pp->intr_entries[i]);
		if(pci_fail == 0) pci_disable_msix(pdev);
		/* MCW configuration (Disable RSS mode for this function) */
		chr_wk  = hfc_fx_read_reg_ext ( pp, 0x23c, 0x1 );
		chr_wk &= ~( (0x80) >> PCI_FUNC(pp->pci_cfginf->devfn) );
		hfc_fx_write_reg_ext( pp, 0x23c, 0x1, chr_wk );
		/* MSI-X Table configuration (vector number) */
		dat_wk = 0x0DC00060 & ~(0x00000400 * PCI_FUNC(pp->pci_cfginf->devfn) );
		hfc_fx_write_indarea(
			pp,
			pp->master_core_no, /* core_no */
			dat_wk,     /* offset */
			0x00000100 * pp->core_num, /* data */
			0x20        /* rammask */
			);
		break;
	}

	return;
}

/*
 * Function:    hfc_fx_clear_status
 *
 * Purpose:
 *
 * Arguments:
 *  pp         - pointer to port_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
void hfc_fx_clear_status(struct port_info *pp)
{
	uint i=0;
	/* Stock these data for analyzing. */
	/* We can read-out these, if we used "dddu" or some "tools". */
	pp->hw_init_status0 = hfc_fx_read_reg(pp,HFC_IOSPACE_STATUS0, 0x4 );	/* Read-out:STATUS 12byte */
	pp->hw_init_status1 = hfc_fx_read_reg(pp,HFC_IOSPACE_STATUS1, 0x4 );
	pp->hw_init_detail0 = hfc_fx_read_reg(pp,HFC_IOSPACE_ERRDETAIL0, 0x4 );
	pp->hw_init_pcierr  = hfc_fx_read_reg_ext(pp, 0x13b0, 0x4 );			/* Read-out:PCI Err Status */

	hfc_fx_write_reg_ext(pp, 0x83f, 0x1, 0x0f);	/* Issue IPRES */
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
				 (char)0x4, (int)0xffffffff, HFC_FX_CORE_OFFSET10);
			}
		}
	}
	

	return;
}

/*
 * Function:    hfc_fx_set_mcw
 *
 * Purpose:
 *
 * Arguments:
 *  pp         - pointer to port_info
 *
 * Returns:    - None
 *
 * Notes:       
 */
#if 0
void hfc_fx_set_mcw(struct port_info *pp)
{
	return;
}
#endif

/* FCLNX-GPL-FX-199 */
#define HFC_POST_RETRY_SUCCESS			0x00000000
#define HFC_POST_RETRY_CONTINUE			0x10000000
#define HFC_POST_RETRY_FAILED			0x20000000

#define HFC_POST_ERROR_MASK				0x0000001f
#define HFC_POST_STATUS_EXGMCK			0x00000001
#define HFC_POST_STATUS_BOOTRUN			0x00000002
#define HFC_POST_STATUS_FSTOP 			0x00000004
#define HFC_POST_STATUS_OFFLINE			0x00000008
#define HFC_POST_RESULT_INVALID			0x00000010

#define HFC_POST_ISOLATE_NONE			0x01
#define HFC_POST_ISOLATE_FAILED_CORE	0x02
#define HFC_POST_ISOLATE_ALL_CORE		0x03


/* FCLNX-GPL-FX-199 */
/*
 * Function:    hfc_fx_check_failed_core
 *
 * Purpose:     HW Initialization Flow
 *              - Recongnazing process of Failure Core
 *
 * Arguments:
 *  pp           - pointer to port_info
 *  rest_retry_cnt - rest retry count of POST loop
 *
 *
 * Returns:    - None
 *
 * Notes:       0  :Normal end
 *              -1 :Error end
 */
int hfc_fx_check_failed_core(struct port_info *pp, int rest_retry_cnt)
{
	int			rtn		= 0;
	int			rtn2	= 0;
	uchar		core_no	= 0;
	uchar		mode	= 0;
	ushort		result=0;
	uint		err_id=0;
	uint		check_stop_exe = 0, i=0, status=0;
	
	uchar		chk_stp_core[MAX_CORE_PROBE_FX] = {0};
	uint		c_statusH[MAX_CORE_PROBE_FX] = {0};
	uint		c_edetail[MAX_CORE_PROBE_FX] = {0};

	struct core_info *core = NULL;
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){		/* FCLNX-GPL-587 */
		core = pp->region_arg[ pp->rid ]->core_arg[i];
		if (core == NULL){
			continue;
		}
		
		status = (uint) hfc_fx_read_reg_core(pp, i, HFC_IOSPACE_CORE0_STATUS0,
						0x04, HFC_FX_CORE_OFFSET100);					/* FCLNX-GPL-587 */
		
		HFC_DBGPRT("hfcldd%d hfc_fx_check_failed_core core#=%d status = %08x\n",
			pp->dev_minor, core->core_no, status);
		
		if (status & HFCFX_MMIO_STTH_HCHKSTOP) { // Core HCHKSTOP
			if(hfc_fx_check_cs_disable(pp, core))
				continue;	/* FCLNX-GPL-FX-438 */
			
			// Core FORCE-CHKSTOP command & Logout
			err_id = (uint)0x96;
			rtn2 = hfc_fx_core_chk_stop(pp, core, err_id);
			if( rtn2 == 0 ){
				check_stop_exe = 1;
			}
		}
	}
	
	if( ( rest_retry_cnt > 0 ) && ( check_stop_exe == 1 ) ){
		return (HFC_POST_RESULT_INVALID | HFC_POST_RETRY_CONTINUE);
	}
	
	for (HFC_LOOP_CORE(core_no, pp)) {
		core = HFC_GET_CORE_ADDR(pp, core_no);
		if (core == NULL)                   { continue; }
		if(hfc_fx_check_cs_disable(pp, core))  { continue; }	/* FCLNX-GPL-FX-438 */
		
		c_statusH[core_no] = HFCFX_MMIO_R4(pp, CORE(STATUSH(core_no)));
		c_edetail[core_no] = HFCFX_MMIO_R4(pp, CORE(EDETAIL(core_no)));
	}

	if (pp->core_deg_mode == HFC_FX_CORE_DEG_ENABLE) {
		if (rest_retry_cnt > 1)
			mode = HFC_POST_ISOLATE_NONE;
		else if (rest_retry_cnt == 1)
			mode = HFC_POST_ISOLATE_FAILED_CORE;
		else
			mode = HFC_POST_ISOLATE_ALL_CORE;
	}
	else {
		if (rest_retry_cnt > 0)
			mode = HFC_POST_ISOLATE_NONE;
		else
			mode = HFC_POST_ISOLATE_ALL_CORE;
	}

	for (HFC_LOOP_CORE(core_no, pp)) {
		core = HFC_GET_CORE_ADDR(pp, core_no);
		if (core == NULL)                   { continue; }
		if(hfc_fx_check_cs_disable(pp, core))  { continue; }	/* FCLNX-GPL-FX-438 */
		
		/* Check EXGMCK */
		if ((c_edetail[core_no] & HFCFX_MMIO_EDTL_HMCKST0) &&
			(c_edetail[core_no] & HFCFX_MMIO_EDTL_HBROADMCK))
		{
			rtn = HFC_POST_STATUS_EXGMCK;
		}
		else if ((c_edetail[core_no] & HFCFX_MMIO_EDTL_HMCKST0) &&
			!(c_edetail[core_no] & HFCFX_MMIO_EDTL_HBROADMCK))
		{
			rtn = HFC_POST_STATUS_EXGMCK;
			core->post_err_cnt++;
			chk_stp_core[core_no] = 1;
		}
		/* Check BOOTRUN */
		else if (c_statusH[core_no] & HFCFX_MMIO_STTH_BOOTRUN) {
			rtn = HFC_POST_STATUS_BOOTRUN;
			core->post_err_cnt++;
			chk_stp_core[core_no] = 1;
		}
		/* Check F-STOP */
		else if (c_statusH[core_no] & HFCFX_MMIO_STTH_FNCSTOP) {
			rtn = HFC_POST_STATUS_FSTOP;
			core->post_err_cnt++;
			chk_stp_core[core_no] = 1;
		}
		/* Check OFFLINE */
		else if (c_statusH[core_no] & HFCFX_MMIO_STTH_COREOFFLINE) {
			rtn = HFC_POST_STATUS_OFFLINE;
			core->post_err_cnt++;
			chk_stp_core[core_no] = 1;
		}
	}

	/* Check POST result if status is valid */		
	if (rtn == 0) {
		for (HFC_LOOP_CORE(core_no, pp)) {
			core = HFC_GET_CORE_ADDR(pp, core_no);
			if (core == NULL)                   { continue; }
			if(hfc_fx_check_cs_disable(pp, core))  { continue; }	/* FCLNX-GPL-FX-438 */
			
			/* This core is available */
			result = HFCFX_MMIO_R2(pp, CCA_DT(POST_RSLT(core_no)));
			if (result == 0x80ff) {
				continue;
			}
		
			rtn = HFC_POST_RESULT_INVALID;
			
			core->post_err_cnt++;

			chk_stp_core[core_no] = 1;
		}
	}

	/* Isolate cores */
	if (rtn != 0) {
		for (HFC_LOOP_CORE(core_no, pp)) {
			core = HFC_GET_CORE_ADDR(pp, core_no);
			if (core == NULL)                   { continue; }
			if(hfc_fx_check_cs_disable(pp, core)){   continue; }	/* FCLNX-GPL-FX-438 */
			
			if ((chk_stp_core[core_no] && (mode == HFC_POST_ISOLATE_FAILED_CORE)) ||
				(mode == HFC_POST_ISOLATE_ALL_CORE))
			{
				/* Core FORCE-CHKSTOP & Logout */
				err_id = (uint)0x96;
				rtn2 = hfc_fx_core_chk_stop(pp, core, err_id);
			}
		}

		if ((rest_retry_cnt == 0) || (test_bit(HFC_PS_ISOL, (ulong *)&pp->status) ))
			return (rtn | HFC_POST_RETRY_FAILED);
		else
			return (rtn | HFC_POST_RETRY_CONTINUE);
	}
	return HFC_POST_RETRY_SUCCESS;
}

int hfc_fx_core_chk_stop(struct port_info *pp, struct core_info *core, uint err_id)
{
	uint	lp=0, i, logdata=0, logdata1;
	uchar	rid, rtn=1,available_core=0;	/* FCLNX-GPL-FX-405 */
	
	struct target_info_fx	*target=NULL;
	struct port_info		*vpp;
	struct region_info		*rp;
	struct core_info		*wk_core;	/* FCLNX-GPL-FX-405 */
	
	if (HFC_FX_VIRTUAL_PORT(pp))
		return rtn;
	
	if( core != NULL){
		HFC_DBGPRT("hfcldd hfc_fx_core_chk_stop core#=%d\n",core->core_no);
	}
	
	// Core FORCE-CHKSTOP command
	if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){
		hfc_fx_write_reg_ext(pp,
			(pp->pkg.map->iosp.reg[HFC_IOSPACE_CORE0_CMD1] + (core->core_no * 0x100)), 
			0x01, 0x01);
	}
	
	rid = pp->rid;
	
	for (i=0; i<=pp->max_vport_count; i++) {
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		rp = pp->region_arg[vpp->rid];
		if (rp == NULL)
			continue;
		
		if (rid != vpp->rid) {
			HFC_ALLCORELOCK(rp);
		}
		
		if (rp->core_arg[core->core_no] != NULL) {
			set_bit(HFC_CS_CHK_STOP, (ulong *)&rp->core_arg[core->core_no]->status);
			rtn = 0;
		}
		
		hfc_fx_determine_master_core(vpp, rp);	/* FCLNX-GPL-FX-191 */
		
		/* Cancel SCSI Command to the check stop core */
		for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
		{
			target = hfc_fx_hash_target_info(vpp, lp);
			if( target != NULL )
			{
				hfc_fx_cancel_scsi_cmd(vpp, rp->core_arg[core->core_no], target, 0, NULL,
					SCS_LINKUP_TO, HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
				target->status = HFC_NON_STATUS;
			}
		}
		
		if (rid != vpp->rid) {
			HFC_ALLCOREUNLOCK(rp);
		}
	}
	
	if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		HFC_DBGPRT("hfcldd%d : Shadow driver enable status off by HW error.\n",pp->dev_minor);
		
		/* Set check stop to core_status */
		/* Core FORCE-CHKSTOP command	FCLNX-GPL-FX-385 *//* FCLNX-GPL-FX-405 */
		set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
				continue;
			
			if(hfc_fx_check_cs_disable(pp, wk_core))
				continue;	/* FCLNX-GPL-FX-438 */
			available_core++;
		}
		if(available_core)
			hfc_fx_write_hg_reg_core(pp, core->core_no, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_CORE_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 */
		else
			hfc_fx_write_hg_reg_core(pp, core->core_no, HFC_IOHGSPC_CMD_REG0, 4, HFC_FX_MLPF_PORT_CSTPEND, HFC_FX_CORE_OFFSET40);     /* FCLNX-0388 */
		/* FCLNX-GPL-FX-405 */
	}
	else if( HFC_FX_MMODE_CHECK_SHARED(pp) && !(HFC_FX_MMODE_CHECK_SHADOW(pp) ) ) {
		HFC_DBGPRT("hfcldd%d : shared Guest driver enable status off by HW error.\n",pp->dev_minor);
		
		/* Set check stop to core_status */
		hfc_fx_write_hg_reg_core(pp, core->core_no, (uint)HFC_IOHGSPC_CMD_REG0,
			  (char)0x4, ( int )HFC_MLPF_FFCSTP, HFC_FX_CORE_OFFSET40);
	}
	
	/* Set Isolate core into CCA */						/* FCLNX-GPL-FX-079 */
	hfc_fx_reset_start(pp, HFC_SET_ISOLATE_CORE);		/* FCLNX-GPL-FX-079 */
	
	// core CHK-STOP log
	memset((void *)core->logdata, 0, 16);
	
	core->logdata[0] = (uchar)core->core_no;
	core->logdata[1] = (uchar)core->pcore_no;
	
	logdata = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[core->core_no], 0x4 ) ;
	
	HFC_DBGPRT("hfcldd hfc_fx_core_chk_stop core_info status = %08x core status = %08x\n",
		core->status, logdata);
	
	HFC_4B_TO_4L(logdata1, logdata);
	memcpy(&core->logdata[8],(char *)&logdata1, 4) ;
	
	
	logdata = (uint)hfc_fx_read_reg_ext(pp, (uint)hfc_status_of_core[core->core_no]+0x8, 0x4 ) ;
	
	HFC_DBGPRT("hfcldd hfc_fx_core_chk_stop core_info status = %08x core status2 = %08x\n",
		core->status, logdata);
		
	HFC_4B_TO_4L(logdata1, logdata);
	memcpy(&core->logdata[12],(char *)&logdata1, 4) ;
	
	if( err_id == 0x96 ){		/* FCLNX-GPL-FX-189 */
		hfc_fx_save_hwlog_five_fx(pp, core, 0x35, HFC_ERRLOG_TYPE_IMLLOG);	/* FCLNX-GPL-FX-239,272 */

		/* Driver log */
		hfc_fx_errlog(pp, core, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, 
			ERRID_HFCP_ERRF, (uint)err_id, core->logdata, 16) ;
	}
	else{
		hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_NONE,
			ERRID_HFCP_EVNT3, (uint)err_id, core->logdata, 16) ;
	}							/* FCLNX-GPL-FX-189 */

	return rtn;
}

int hfc_fx_config_hw_set_five_fx(struct port_info *pp, uint retry_maxcnt)
{
	uchar		stat_chkerrflg;
	uchar		post_chkerrflg;
	uchar		retry_finish = 0;
	uint		retry_cnt;
	uchar		dmp_data[32];
	uint		errlog_data[4];

	uchar		err_code = 0;
	uint 		p_statusH = 0, p_statusL = 0;		/* FCLNX-GPL-FX-197 */
	uint		i=0, wk4, j=0, rss=0;
	int 		rtn=0, rtn2=0;
	uint		err_id=0;
	struct core_info	*core=NULL;

	HFC_ENTRY("hfc_fx_config_hw_set_five_fx");
	
	/* Initialize */
	memset(dmp_data, 0, sizeof(dmp_data));
	memset(errlog_data, 0, sizeof(errlog_data));

	/* Set 0x00 to CMD_RES (0x30) */
	hfc_fx_write_reg(pp, HFC_IOSPACE_CMDRES, 0x1, 0x00);

	mdelay(1000); /* Wait 1000ms */

	/* FCLNX-GPL-227 start */
/*	if( pp->debug_func & HFC_DEBUG_LINK_WIDTH_CHK ) */ /* FCLNX-GPL-246 */
/*	{ */
		hfc_fx_pcie_link_width_chk(pp);
/*	} */
	/* FCLNX-GPL-227 end */

	if (( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() )&&
		 ( pp->isol_force == HFC_CHKSTP_FRC_ISOL )){ /* - FCLNX-546 - *//* FCLNX-GPL-147 */
		/* Stop optical transmission */
		if(!(HFC_FX_MMODE_CHECK_SHARED(pp))){
			hfc_fx_write_reg(pp,(uint)HFC_IOSPACE_CMDFCIF,(char)0x4,(char)0x80808080);
		}
		else{
			hfc_fx_mlpf_set_fcif(pp, 0x80808080);	/* FCLNX-GPL-399 */
		}
		/* Turn LED (Yellow and Green) off */
		if(!(HFC_FX_MMODE_CHECK_SHARED(pp))){
			hfc_fx_write_reg(
				pp, ( uint )HFC_IOSPACE_CMDLED,( char )0x4,HFC_WAKE_UP_FAILURE_FIVE);
		}
		else{
			hfc_fx_mlpf_set_led(pp, HFC_WAKE_UP_FAILURE_FIVE);	/* FCLNX-GPL-399 */
		}
		return 0;
	} /* FCLNX-GPL-147 */


	for (retry_cnt = 0; retry_cnt < retry_maxcnt ; retry_cnt++) {	/* FCLNX-GPL-FX-197 */
		
		HFC_DBGPRT("hfcldd hfc_fx_config_hw_set_five_fx retry_cnt = %d\n",retry_cnt);

		stat_chkerrflg = 0;
		post_chkerrflg = 0;
		err_code = 0;

		/* Read status information (8 bytes) */
		p_statusH = HFCFX_MMIO_R4(pp, PORT(STATUSH));	/* FCLNX-GPL-FX-197 */
		p_statusL = HFCFX_MMIO_R4(pp, PORT(STATUSL));	/* FCLNX-GPL-FX-197 */

		HFC_DBGPRT( "hfc_fx_read_reg HFC_IOSPACE_STATUS[%x] = 0x%08x.",
					HFC_IOSPACE_STATUS0, p_statusH);

		/* FCLNX-GPL-FX-197 */
		if ((p_statusL & HFCFX_MMIO_STTL_BOOTERR) ||
		    (p_statusL & HFCFX_MMIO_STTL_FMEMBOOTEE)) 
		{
			stat_chkerrflg = 1;	
			err_code = 0x20;
			retry_finish = 1;
			
			for (HFC_LOOP_CORE(i, pp)) {
				if ((core = HFC_GET_CORE_ADDR(pp, i)) == NULL)
					continue;
				if(hfc_fx_check_cs_disable(pp, core))
					continue;	/* FCLNX-GPL-FX-438 */
				
				// Core FORCE-CHKSTOP command & Logout
				err_id = (uint)0x96;
				rtn2 = hfc_fx_core_chk_stop(pp, core, err_id);
			}
		} 			/* FCLNX-GPL-FX-197 */
		else if (p_statusH & HFCFX_MMIO_STTH_HCHKSTOP) { // ALL Core CHK-STOP
			stat_chkerrflg = 1;	err_code = 0x21;
			retry_finish = 1;
		}			/* FCLNX-GPL-FX-197 */

		if (retry_finish == 0) {
//			HFC_DBGPRT("hfcldd hfc_fx_config_hw_set_five_fx retry_finish = 0\n");
			//HW initializing set flow << Failure core recognizing process >>
			rtn = hfc_fx_check_failed_core(pp, (retry_maxcnt - retry_cnt - 1));
			if (rtn == HFC_POST_RETRY_SUCCESS) {
				/* No error occcurred without check stopped cores. */
				break;
			}
			
			if (rtn & HFC_POST_RESULT_INVALID)
				post_chkerrflg = 1;
			else
				stat_chkerrflg = 1;

			err_code = (rtn & HFC_POST_ERROR_MASK);

			if (rtn & HFC_POST_RETRY_FAILED)
				retry_finish   = 1;
		}	/* FCLNX-GPL-FX-197 */
		
		/* Retry count over at the port */
		if ( retry_finish == 1 ) {		/* FCLNX-GPL-FX-197 */
			
			/* Set LED to POST up Falure */
			hfc_fx_write_reg(pp, HFC_IOSPACE_CMDLED, 0x1, HFC_FX_WAKE_UP_FAILURE_FIVE);

			/* Collect IML FAIL log information  */
			/* Status information is Error? */
			if (stat_chkerrflg == 1) {
				HFC_DBGPRT("config_hw_set_five_fx() - stat_chkerrflg=1, errcode = 0x%x.",err_code );

				memset(errlog_data, 0, sizeof(errlog_data));
				/* Collect log information */
				/* Collect log information (16 Bytes from STATUS) */
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS0, 0x4);
				HFC_4L_TO_4B(errlog_data[0],wk4);
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS1, 0x4);
				HFC_4L_TO_4B(errlog_data[1],wk4);
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_ERRDETAIL0, 0x4);
				HFC_4L_TO_4B(errlog_data[2],wk4);
				errlog_data[3] = err_code;

				/* H/W log */
//				hfc_fx_logout(pp, 0x35, HFC_ERRLOG_TYPE_IMLLOG); 
				pp->imllog_logout = 1;		/* FCLNX-GPL-FX-141 */
				hfc_fx_save_hwlog_five_fx(pp, NULL, 0x35, HFC_ERRLOG_TYPE_IMLLOG);

				/* Driver log */
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x35, (uchar *)errlog_data, 16) ;
			}
			/* Does the error occure at POST? */
			else if (post_chkerrflg == 1) {
				HFC_DBGPRT("config_hw_set_five_fx() - post_chkerrflg=1, errcode = 0x%x.", err_code );

				memset(errlog_data, 0, sizeof(errlog_data));
				
				/* Collect POST information */
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS0, 0x4);
				HFC_4L_TO_4B(errlog_data[0],wk4);
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS1, 0x4);
				HFC_4L_TO_4B(errlog_data[1],wk4);
				wk4 = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_ERRDETAIL0, 0x4);
				HFC_4L_TO_4B(errlog_data[2],wk4);
				errlog_data[3] = err_code;

				/* H/W log */
//				hfc_fx_logout(pp, 0x36, HFC_ERRLOG_TYPE_IMLLOG); 
				pp->imllog_logout = 1;		/* FCLNX-GPL-FX-141 */
				hfc_fx_save_hwlog_five_fx(pp, NULL, 0x36, HFC_ERRLOG_TYPE_IMLLOG);

				/* Driver log */
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, 0x36, (uchar *)errlog_data, 16) ;
			}

			/* Set 0x00 to CMD_RES register (0x30) */
			hfc_fx_write_reg(pp, HFC_IOSPACE_CMDRES, 0x1, 0x00);

			/* Error return (abnormal end) */
			return(-1);
		}

		if (pp->debug_func & HFC_DEBUG_POST_LOGOUT)
		{	
			/* Debug Mode */ /* Get Log "ErrNo:0xf1" */
			memset(dmp_data, 0, sizeof(dmp_data));
			memset(errlog_data, 0, sizeof(errlog_data));

			errlog_data[0] = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS0, 0x4);
			errlog_data[1] = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS1, 0x4);
			errlog_data[2] = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_ERRDETAIL0, 0x4);
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
//			hfc_fx_logout(pp, (uint)0xf1, HFC_ERRLOG_TYPE_IMLLOG);
			hfc_fx_save_hwlog_five_fx(pp, NULL, 0xf1, HFC_ERRLOG_TYPE_IMLLOG);
			
			/* Driver log */
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_EVNT3, 0xf1, dmp_data, 16) ;
		}
		/* FCLNX-GPL-236 end */
		
		/* Multi Queue */
		if( HFC_FX_MQ_VALID(pp) ){
			for (j=0x4000 ; j< 0x8000 ; j += 0x10) {
				HFC_DBGPRT("hfcldd%d : before core_start, addr#%08x status:%08x %08x %08x %08x\n", pp->dev_minor, i,
								(uint)hfc_fx_read_reg_ext(pp, (uint)j, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)j+0x4, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)j+0x8, 0x4 ),
								(uint)hfc_fx_read_reg_ext(pp, (uint)j+0xc, 0x4 )
								);
			}
		}
		
		/* Forced MCK to port */
		hfc_fx_write_reg(pp, HFC_IOSPACE_CMDCTL, 0x1, 0x08);
			
		/* Multi Queue *//*FCLNX-GPL-FX-208 *//*FCLNX-GPL-FX-212 */
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					for (rss=0 ; rss< HFC_FX_MSIX_NVEC ; rss++) {
						hfc_fx_write_reg_rss_core(pp, i, rss, HFC_IOSPACE_RSS_INTA_RST,
							0x4, 0xffffffff, HFC_FX_CORE_OFFSET10, HFC_FX_CORE_OFFSET40);
					}
				}
			}
		}
		
		/* Reset all interrupt factor */  
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_RST,
					  (char)0x4, 0xffffffff, HFC_FX_CORE_OFFSET10);
				}
			}
		}
		
		if (HFC_FX_MMODE_CHECK_SHADOW(pp)){				/* FCLNX-GPL-317 */
			hfc_fx_write_reg_ext(pp, ( uint )0x10a0,( char )0x4, 0x00000000 );
		}												/* FCLNX-GPL-317 */
	
		/* Set ERPTYP internal loop state */
		hfc_fx_write_reg(pp, HFC_IOSPACE_PTYP0, 0x1, 0x04);
		
		/* Recovery Reset (CTLRES) */
		hfc_fx_write_reg(pp, HFC_IOSPACE_CMDRES, 0x1, 0x02);	/* FCLNX-GPL-FX-094 */

		/* 1ms wait */
		mdelay(1);

		/* set reboot to all core */
		if( HFC_FX_MMODE_CHECK_MLPF(pp) )
		{	
			/* Clear Communication area of WS. */
			hfc_fx_reset_start(pp, HFC_WSCA_CLEAR);
			/* We reboot the same as MCK recovery. */
			hfc_fx_reset_start(pp, HFC_REBOOT);
		}
		else
		{	/* Basic */
			hfc_fx_write_reg(pp, HFC_IOSPACE_CMDBOOT, 0x1, 0x20);
		}

		/* 200 msec wait *//* FCLNX-GPL-FX-113 */
		for (i=0;i<200;i++) {
			mdelay(1); // 1ms
		}
		
		/* F-Start and Tracer-Start Command *//* FCLNX-GPL-FX-094 */
		hfc_fx_write_reg(pp,( uint )HFC_IOSPACE_CMDCTL,( char )0x1,(char)0xa0);
		
		/* 1 sec wait *//* FCLNX-GPL-FX-113 */
		for (i=0;i<1000;i++) {
			mdelay(1); // 1ms
		}

		pp->post_retry_cnt++;
	}

	/* write 0x00 to PCIRESEXE as initialization complete flag */
	hfc_fx_write_reg(pp, HFC_IOSPACE_CMDRES, 0x1, 0x00);

	/* Release Main PF Flag */
	hfc_fx_write_reg(pp, HFC_IOSPACE_HPMFLG, 0x1, 0x80);
	
	/* PCI error status clear */
	hfc_fx_pci_err_status_clear_five_fx(pp);			/* FCLNX-GPL-FX-145 */
	
	/* Reset UTL Register */
	hfc_fx_reset_start(pp, HFC_UTL_REG_CLEAR);			/* FCLNX-GPL-FX-145 */
	
	/* PCI error status reset */
	/* hfc_fx_clear_sticky_bit(pp); */
	
	/* Disable REQID#0 */ /* FCLNX-GPL-294 start */
	/* Deleted for FIVE-FX
	wk_char = hfc_fx_read_reg_ext(pp,  0x222, 0x1);
	wk_char |= 0x80;
	hfc_fx_write_reg_ext(pp, 0x222, 0x1, wk_char);
	*/
	/* FCLNX-GPL-294 end */
	
	/* Read status information (4 bytes) */
	p_statusH = (uint) hfc_fx_read_reg(pp, HFC_IOSPACE_STATUS0, 0x4 );

	HFC_DBGPRT( "hfcldd : hfc_fx_config_hw_set_five_fx - end HFC_IOSPACE_STATUS= 0x%08x.",
			p_statusH);

	HFC_EXIT("hfc_fx_config_hw_set_five_fx");

    return(0);
}

void hfc_fx_pci_err_status_clear_five_fx(struct port_info *pp)
{
	/* Clear Status Register */
	hfc_fx_write_cnfg(pp, 0x06, 0x2, 0xffff);

	/* Clear Device Status Register */
	hfc_fx_write_cnfg(pp, 0x76, 0x1, 0xff);

	/* Clear AER Uncorrectable Error Status */
	hfc_fx_write_cnfg(pp, 0x104, 0x4, 0xffffffff);

	/* Clear AER Correctable Error Status */
	hfc_fx_write_cnfg(pp, 0x110, 0x4, 0xffffffff);
	
	/* Release PCIe Dump Core Freezing Status */
	hfc_fx_write_ramid(pp, HFCFX_RADR(DMP(OFF(0x804))), TYPE_2, 0x00, 0x00400000);

}


int hfc_fx_query_devid(struct port_info *pp)
{
	uint	vender_id;
	uint	device_id;
	uint	sub_system_id;

	memset(&pp->pkg, 0, sizeof(pp->pkg));

	/* Get "VenderID" */
	vender_id = hfc_fx_read_cnfg(pp, HFC_HOST_VENDER_ID, 0x2);
	pp->pkg.vender_id = (ushort)vender_id;
	/* Get "DeviceID" */
	device_id = hfc_fx_read_cnfg(pp, HFC_HOST_DEVICE_ID, 0x2);
	pp->pkg.device_id = (ushort)device_id;
	/* Get "SubSystemID" */
	sub_system_id = hfc_fx_read_cnfg(pp, HFC_HOST_SUB_SYSTEM_ID, 0x2);
	pp->pkg.sub_system_id = (ushort)sub_system_id;

	switch (pp->pkg.device_id) {
		case HFC_PCI_DEVICE_ID_3070 :
			/* FIVE-FX */
			pp->pkg.type = HFC_PKTYPE_FIVE_FX;
			pp->pkg.map  = (struct pkg_map *) &hfc_fx_pkg_map[0];
			switch(pp->pkg.sub_system_id){
			case HFC_HOST_SUB_SYSTEM_ID_FX1P:
				pp->pkg.port = 1;
				break;
			case HFC_HOST_SUB_SYSTEM_ID_FX2P:
				pp->pkg.port = 2;
				break;
			case HFC_HOST_SUB_SYSTEM_ID_FX4P:
				pp->pkg.port = 4;
				break;
			}
			pp->core_num = MAX_CORE_PROBE_FX/pp->pkg.port;
			break;

		default :
			/* This device_id is unkown.  */
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xCE, NULL, 0) ;/* FCLNX-GPL-161 */
			return(-1);
	}
	return(0);
}

/* FCLNX-GPL-112 */
/*
 * Function:  hfc_fx_get_sysrev
 *
 * Purpose:
 *
 * Arguments:
 *  core        - pointer to a core_info structure
 *
 * Returns:
 *  Current FW revision (uint)
 *
 * Notes:
 *  This
 */
uint hfc_fx_get_sysrev(struct core_info *core)
{
	uint   sysrev;
	struct port_info *pp;

	/* Get current FW sysrev info from mp_adap_info */

	if ( core == NULL ){
		sysrev = 0xeeeeeeee; return(sysrev);
	}

	sysrev = core->rp->pport->sys_rev;
	pp = core->rp->pport;
	if ( core->fw_init_p != NULL) {
		if(hfc_fx_read_val(core->fw_init_p->fls_hdr.sys_rev)){
		/* Get current FW sysrev info from FW_INIT_TBL when FW online-update function is supported */
		sysrev = hfc_fx_read_val(core->fw_init_p->fls_hdr.sys_rev);
		}
	}

	return (sysrev);

}

/* FCLNX-GPL-180 */ /* FCLNX-GPL-201 */
/*
 * Function:  hfc_fx_get_slot_dev
 *
 * Purpose:   Identify pointer to this slot's pci_dev structure
 *
 * Arguments:
 *  pp        - pointer to an port_info structure
 *
 * Returns:   
 *            - error case  : NULL
 *            - succeed case: pointer to the pci_dev
 *
 * Notes:
 *
 */
struct pci_dev *hfc_fx_get_slot_dev(struct port_info *pp)
{
	struct pci_bus *bus			= pp->pci_cfginf->bus;
	struct pci_bus *parent_bus	= NULL;
	struct pci_dev *slot_dev	= NULL;

	if(bus == NULL)
	{	/* abnormal case */
		return slot_dev;
	}

	if(pp->pkg.type == HFC_PKTYPE_FPP)
	{
		/* This BUS's parent is the "slot bus" */
		slot_dev = bus->self;
	}
	else if(pp->pkg.type == HFC_PKTYPE_FIVE)
	{
		/* check the package code */
		switch(pp->pkg.code)
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
 * Function:  hfc_fx_restore_add_wwn
 *
 * Purpose:
 *
 * Arguments:
 *  pp        - pointer to an port_info structure
 *
 * Returns:   
 *            - Succeed : add_ww_name
 *            - Failed  : 0
 *
 * Notes:
 *
 */
uint64_t hfc_fx_restore_add_wwn(struct port_info *pp)
{
	struct   pci_dev         *slot_dev;
	struct   slot_ww_name    *slot_wwn;
	int      domain_no; /* 4byte */
	uchar    bus_no;    /* 1byte */
	uchar    devfn;     /* 1byte */
	uint64_t add_ww_name = 0x00;
	int i;
	
	/*** Get the "BUS:DEV.FUNC" of this slot  ***/
	slot_dev = hfc_fx_get_slot_dev(pp);
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
		if(slot_wwn[i].port_no != pp->port_no)
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
 * Function:  hfc_fx_backup_add_wwn
 *
 * Purpose:
 *
 * Arguments:
 *  pp        - pointer to an port_info structure
 *
 * Returns:   
 *            - Succeed :  0
 *            - Failed  : -1
 *
 * Notes:
 *
 */
int hfc_fx_backup_add_wwn(struct port_info *pp, uint64_t add_ww_name)
{
	struct  pci_dev         *slot_dev;
	struct  slot_ww_name    *slot_wwn;
	int     domain_no; /* 4byte */
	uchar   bus_no;    /* 1byte */
	uchar   devfn;     /* 1byte */
	int     err_code = -1;
	int     i;
	
	/*** Get "BUS:DEV.FUNC" of this slot  ***/
	slot_dev = hfc_fx_get_slot_dev(pp);
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
		slot_wwn[i].port_no   = pp->port_no;
		slot_wwn[i].ww_name   = add_ww_name;
		err_code = 0; /* No err */
		break;
	}
	
	return err_code;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
void hfc_fx_kthread_stop(struct port_info *pp)
{
	atomic_set(&pp->rport_event_wait, 1); /* FCLNX-GPL-565 */
	kthread_stop(pp->worker_thread); /* FCLNX-GPL-259, 276 */
	return;
}

int hfc_fx_do_rport(void *p)
{
	struct port_info	*pp = p;
	struct port_info	*wkpp;
	struct region_info	*rp = NULL;
	struct region_info	*wkrp = NULL;
	ulong				flags = 0;
	int					i=0,rc=0;
	int					need;
	
	HFC_ENTRY("hfc_fx_do_rport");
//	HFC_ERRPRT("hfc_fx_do_rport\n");
	
	while (!kthread_should_stop()) {
		rc = wait_event_interruptible(pp->rport_event, atomic_read(&pp->rport_event_wait)!=0 );
	
		if((kthread_should_stop()) || (rc))
			break;
		
		/* physical port */
		if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){
			rp = pp->region_arg[0];
		}else{	/* FCLNX-GPL-FX-385 */
			rp = pp->region_arg[pp->rid];
		}	/* FCLNX-GPL-FX-385 */
		
		hfc_fx_set_rport_status(pp,rp);
		
		/* virtual port */
		for (i=1; i<=pp->max_vport_count; i++) {
			wkpp = pp->vport_ptr[i].vport_arg;
			if ((wkpp == NULL) || (wkpp == pp))
				continue;
			
			if (HFC_FX_MQ_VIRTUAL_PORT(wkpp))
				continue;
			
			wkrp = pp->region_arg[wkpp->rid];
			if (wkrp == NULL)
				continue;
			
			hfc_fx_set_rport_status(wkpp,wkrp);
		}
		
		need = 0;
		/* try again rport status check for physical port */
		if (hfc_fx_scan_rport_status(pp,rp)) {
			need = 1;
		}
		
		/* try again rport status check for virtual port */
		for (i=1; i<=pp->max_vport_count; i++) {
			wkpp = pp->vport_ptr[i].vport_arg;
			if (wkpp == NULL)
				continue;
			
			if (HFC_FX_MQ_VIRTUAL_PORT(wkpp))
				continue;
			
			wkrp = pp->region_arg[wkpp->rid];
			if (wkrp == NULL)
				continue;
			
			if (hfc_fx_scan_rport_status(wkpp,wkrp)) {
				need = 1;
				break;
			}
		}
		
		if (need) {
			atomic_set(&pp->rport_event_wait, 1);
		}
		else {
			atomic_set(&pp->rport_event_wait, 0);
		}
	}
	
	if(!HFC_FX_MMODE_CHECK_MLPF(pp) ){
		rp = pp->region_arg[0];
	}else{	/* FCLNX-GPL-FX-385 */
		rp = pp->region_arg[pp->rid];
	}	/* FCLNX-GPL-FX-385 */
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	clear_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status );
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	HFC_EXIT("hfc_fx_do_rport");
	
	return 0;
}

void hfc_fx_set_rport_status(struct port_info *pp, struct region_info *rp)
{
	struct target_info_fx	*target;
	ulong				flags = 0;
	int					i=0;
	uchar				add, del;
	
	for (i=0;i<MAX_TARGET_PROBE;i++) {
		HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
		target = hfc_fx_hash_target_valid(pp, i);
		/* delete rport */
		del = FALSE;
		if(target){
			if ( test_bit(HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status ) ) {
				del = TRUE;
				HFC_DBGPRT("hfc_fx_set_rport_status - delete: instance=%d, target_id=%d, rid=%d\n",
					pp->instance, target->target_id, pp->rid);
			}
			clear_bit( HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status );
		}
		
		/* add rport */
		add = FALSE;
		if(target){
			if ( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){
				if ( test_bit(HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status ) ) {
					add = TRUE;
					HFC_DBGPRT("hfc_fx_set_rport_status - add: instance=%d, target_id=%d, rid=%d\n",
						pp->instance, target->target_id, pp->rid);
				}
				clear_bit( HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status );
			}
		}
		HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
		
		if (del == TRUE) {
			hfc_fx_rport_delete(target);
		}
		if (add == TRUE) {
			hfc_fx_rport_add(pp, target);
		}
	}
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
	if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ) /* FCLNX-GPL-FX-472 */
		pp->link_reset_multi_mode = 0;
#endif
}

int hfc_fx_scan_rport_status(struct port_info *pp, struct region_info *rp)
{
	struct target_info_fx	*target;
	ulong				flags = 0;
	int					i=0;
	
	/* try again rport status check */
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	for (i=0;i<MAX_TARGET_PROBE;i++) {
		target = hfc_fx_hash_target_valid(pp, i);
		
		if(target){
			if ( test_bit(HFC_NEED_RPORT_DEL, (ulong *)&target->rport_status ) ) {
				HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
				return (1);
			}
		}
		
		if(target){
			if ( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){
				if ( test_bit(HFC_NEED_RPORT_ADD, (ulong *)&target->rport_status ) ) {
					HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
					return (1);
				}
			}
		}
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	return (0);
}

#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
void hfc_fx_dev_loss_tmo_callbk(struct Scsi_Host *host, struct fc_rport *rport)
{
	struct port_info		*pp = NULL;		/* Pointer to an port_info		*/
	struct target_info_fx	*target = *(struct target_info_fx **)rport->dd_data; /* Pointer to a target_info_fx	*/
	ulong					flags = 0;
	struct region_info		*rp=NULL;

	if (hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) /* FCLNX-GPL-FX-472 */
		return;
	pp = (struct port_info *)host->hostdata;
	if (!pp){ /* This port_info is not found */
		return ;
	}
	
	rp =  pp->region_arg[pp->rid];
	
	HFC_ALLLOCK_IRQSAVE(pp,rp,flags);
	if( target ){
		clear_bit(HFC_TF_WAITING_DEV_LOSS_TMO, (ulong *)&target->flags);
	}
	HFC_ALLUNLOCK_IRQRESTORE(pp,rp,flags);
	
	return;
}
#endif

#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

#if 0
/* FCLNX-GPL-204 STR */
/*
 * Function:    hfc_fx_proc_info_pfb
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
int hfc_fx_proc_info_pfb(struct Scsi_Host *host, char *buffer, char **start, off_t offset, int length, int inout)
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_proc_info_pfb(host, buffer, start, offset, length, inout) );
}


/*
 * Function:    hfc_fx_strategy
 *
 * Purpose:     Initiate SCSI command 
 *
 * Arguments:   
 *  cmnd		Pointer to Scsi_Cmnd
 *  iodone		Pointer to done() (Return SCSI response)
 *
 * Returns:     
 *  			0 :  Completed SCSI initiation successfully 
 *  			EIO : port_info or dev_info_fx does not exist or invalid.
 * 				(Only for ioctl)
 *
 * Notes:       Caller should be in process level or interruption level.
 */
int hfc_fx_strategy_pfb(struct scsi_cmnd *cmnd, void (*iodone)(struct scsi_cmnd *))
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_strategy_pfb(cmnd,iodone) );
}

int hfc_fx_eh_abort_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_abort_pfb(cmnd) );
}

int hfc_fx_eh_device_reset_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_device_reset_pfb(cmnd) );
}

int hfc_fx_eh_target_reset_pfb(struct scsi_cmnd *cmnd)					/* FCLNX-GPL-0449 */
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_target_reset_pfb(cmnd) );
}

int hfc_fx_eh_bus_reset_pfb(struct scsi_cmnd *cmnd)
{
	return ( hfc_manage_info.npubp->hfc_fx_mp_bus_reset_pfb(cmnd) );
}

/* FCLNX-GPL-204 END */
#endif

/* FCLNX-GPL-227 */ /* FCLNX-GPL-246 */
/*
 * Function:  hfc_fx_pcie_link_width_chk
 *
 * Purpose:   Check pcie_link_width for FIVE-EX
 *
 * Arguments:
 *  pp        - pointer to an port_info structure
 *
 * Returns:   
 *            - Succeed :  0
 *            - Failed  : -1
 *
 * Notes:     pp is not NULL
 *
 */
int
hfc_fx_pcie_link_width_chk(struct port_info *pp)
{
	const uchar pcie_X1      = 0x01;
	int         rc           = 0;
	ushort      cfg_data     = 0x0000;
	uchar       negotiated_link_width = 0x00;

	/* Get negotiated_link_width (cfgadr 7F-7Eh ,bit9-4) */
	cfg_data   = (ushort)hfc_fx_read_cnfg(pp, 0x7e, 0x2);
	cfg_data >>= 4;
	cfg_data  &= 0x003f;
	negotiated_link_width = (uchar)cfg_data;
	
	/* for Debug */
	HFC_DBGPRT("hfcldd%d: negotiated_link_width = %d\n", pp->dev_minor, negotiated_link_width);

	/* Data check */
	if( negotiated_link_width == pcie_X1 )
	{	/* Err Case (LinkWidth = x1) */
		/* FCLNX-GPL-246 start */
		if (pp->pkg.lsi_rev == 0x02)
		{	/* FIVE-EX Pass2.1 */
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xD9, NULL, 0);
		}
		else
		{	/* FIVE-EX Pass2.2 (Pass1.0, Pass2.0, and others) */ /* FCLNX-GPL-263 */
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR2, 0xDA, NULL, 0);
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
 * Function:    hfc_fx_suspend_one
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
hfc_fx_suspend_one(struct pci_dev *pdev, pm_message_t msg)
{
	struct Scsi_Host *host = NULL;
	struct port_info *pp   = NULL;
	ulong flags=0;
	int pci_fail=0;
	
	
	HFC_DBGPRT( "hfcldd : hfc_fx_suspend_one - Start.\n");

	/*** NULL pointer check ***/
	if( pdev == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_suspend_one - pdev is NULL.\n");
		goto hfc_fx_suspend_err;
	}
	
	host = pci_get_drvdata(pdev);
	if(host == NULL)
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_suspend_one - host is NULL.\n");
		goto hfc_fx_suspend_err;
	}
	pp = (struct port_info *)host->hostdata;
	if(pp == NULL)
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_suspend_one - pp is NULL.\n");
		goto hfc_fx_suspend_err;
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	/*** Check the staus of this port. ***/
	if( test_bit(HFC_SUSPEND, (ulong *)&pp->status) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - This is adapter is SUSPEND.\n",pp->dev_minor);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		goto hfc_fx_suspend_err;
	}
	
	set_bit(HFC_SUSPEND, (ulong *)&pp->status);
	pp->pm_event = msg.event;
	
	pci_fail = hfc_fx_pcibus_chk(pp);

	/*** Do suspend ***/
	switch( msg.event )
	{
		case PM_EVENT_ON:
			/* NOP */
			HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - PM_EVENT_ON.\n",pp->dev_minor);
			break;

		case PM_EVENT_FREEZE:
		case PM_EVENT_SUSPEND:
			HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - PM_EVENT_FREEZE or PM_EVENT_SUSPEND.\n",pp->dev_minor);
			/* Bring down the device and stop kthread. */
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			hfc_fx_suspend_driver(pp, host);
			HFC_PORTLOCK_IRQSAVE(pp,flags);
			
			/* suspend PCI device */
			if((!test_bit(HFC_PS_ISOL, (ulong *)&pp->status)))
			{	/* FCLNX-GPL-428 */
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				if( hfc_fx_suspend_pci(pp, pdev, pci_fail) )
				{
					HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - Failed hfc_fx_suspend_pci.\n",pp->dev_minor);
					goto hfc_fx_suspend_err;
				}
				HFC_PORTLOCK_IRQSAVE(pp,flags);
			}
			
			/* Change power state */
			if( msg.event == PM_EVENT_SUSPEND)
			{	/* for only PM_EVENT_SUSPEND */
				HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - PM_EVENT_SUSPEND.\n",pp->dev_minor);
				if(( pp->issue_d3hot)&&(pci_fail == 0)){
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
					/* Set power state. This function call "msleep".*/
					if( pci_set_power_state(pdev, PCI_D3hot) )
					{
						goto hfc_fx_suspend_err;
					}
					HFC_PORTLOCK_IRQSAVE(pp,flags);
				}
			}
			break;

		default:
			HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - Default.\n",pp->dev_minor);
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			goto hfc_fx_suspend_err;
	}
	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - End.\n",pp->dev_minor);
	/* No Err */
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	return 0;
	
hfc_fx_suspend_err:
	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_one - hfc_fx_suspend_err.\n",pp->dev_minor);
	return -EIO;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_resume_one
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
hfc_fx_resume_one(struct pci_dev *pdev)
{
	struct Scsi_Host *host = NULL;
	struct port_info *pp   = NULL;
	ulong flags=0;

	HFC_DBGPRT( "hfcldd : hfc_fx_resume_one - Start.\n");

	if( pdev == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_resume_one - pdev is NULL.\n");
		goto hfc_fx_resume_err;
	}

	host = pci_get_drvdata(pdev);
	if( host == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_resume_one - host is NULL.\n");
		goto hfc_fx_resume_err;
	}
	
	pp = (struct port_info *)host->hostdata;
	if( pp == NULL )
	{
		HFC_DBGPRT( "hfcldd : hfc_fx_resume_one - pp is NULL.\n");
		goto hfc_fx_resume_err;
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if( !test_bit(HFC_SUSPEND, (ulong *)&pp->status) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - This adapter is not SUSPEND.\n",pp->dev_minor);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		goto hfc_fx_resume_err;
	}
	clear_bit(HFC_SUSPEND, (ulong *)&pp->status);
	
	switch( pp->pm_event )
	{
		case PM_EVENT_ON:
			/* NOP */
			HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - PM_EVENT_ON.\n",pp->dev_minor);
			break;

		case PM_EVENT_FREEZE:
		case PM_EVENT_SUSPEND:
			HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - PM_EVENT_FREEZE,PM_EVENT_SUSPEND.\n",pp->dev_minor);
			/* Change power state */
			if( pp->pm_event == PM_EVENT_SUSPEND)
			{	/* for only PM_EVENT_SUSPEND */
				HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - PM_EVENT_SUSPEND.\n",pp->dev_minor);
				if( pp->issue_d3hot){
					HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - D3_HOT ON.\n",pp->dev_minor);
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
					if( pci_set_power_state(pdev, PCI_D0) )
					{
						goto hfc_fx_resume_err;
					}
				}
				else{
					HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
				}
			}
			else{
				HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			}
			
			/* resume PCI device */
			if( hfc_fx_resume_pci(pp, pdev) )
			{
				HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - Failed hfc_fx_resume_pci.\n",pp->dev_minor);
				goto hfc_fx_resume_err;
			}
			
			/* Start the device and start kthread. */
			hfc_fx_resume_driver(pp, host);
			HFC_PORTLOCK_IRQSAVE(pp,flags);
			
			break;

		default:
			HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - default.\n",pp->dev_minor);
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
			goto hfc_fx_resume_err;
	}
	
	/* No Err */
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - End.\n",pp->dev_minor);
	return 0;
	
hfc_fx_resume_err:
	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_one - hfc_fx_resume_err.\n",pp->dev_minor);
	return -EIO;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_suspend_driver
 *
 * Purpose:     Bring down the device and stop kthread
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *  host		The pointer for scsi_host information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "pp" and "host" are not NULL pointer.
 */
void
hfc_fx_suspend_driver(struct port_info *pp, struct Scsi_Host *host)
{
	ulong flags = 0;
	int wait=0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_driver - Start.\n",pp->dev_minor);
	
	/*** Lock ***/
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	/*** Closing process is in progress ***/
	set_bit(HFC_PD_WAIT_CLOSE, (ulong *)&pp->status_detail2);
	pp->initialize = 0;
	clear_bit(HFC_ATTACH, (ulong *)&pp->attach_status);		/* FCLNX-0459 */

	do {													/* FCLNX-0459 */
		wait=0;
		
		if (hfc_manage_info.hfcldd_mp_mod) {				/* FCLNX-GPL-204 */
			wait = hfc_manage_info.npubp->hfc_fx_wait_mp_ioend(pp);
		}
		
		if (!wait) {
			HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_driver - called hfc_fx_releasse.\n",pp->dev_minor);
			wait = hfc_fx_release_adp(pp);
		}
		
		/* sleep for a while */
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		msleep(1);
		hfc_fx_intr(0, (void *)pp); /* Check interrupts by Drivers self Because, Not Interrupt for Kernel in suspend process*/
		HFC_PORTLOCK_IRQSAVE(pp,flags);
	} while (wait);											/* FCLNX-0459 */
	
	/*** Unlock ***/
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/*** stop kthread for rport ***/
	hfc_fx_stop_rport(pp);
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */	
	/*** put Scsi_Host ***/
	hfc_fx_scsi_host_put(host);

	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_driver - End.\n",pp->dev_minor);
	return;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_suspend_pci
 *
 * Purpose:     suspend PCI device
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *  pdev		The pointer for pci device
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       "pp" and "pdev" are not NULL pointer.
 */
int
hfc_fx_suspend_pci(struct port_info *pp, struct pci_dev *pdev, int pci_fail)
{
	ulong flags = 0;

	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_pci - Start.\n",pp->dev_minor);

	/* Free IRQ. This function call "spin_lock_irqsave()". */
	hfc_fx_free_interrupts(pp, pp->msi_flag, pci_fail);
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	pp->is_busmaster = pdev->is_busmaster;
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	/* Save state of pdev. This function call "spin_lock_irqsave()". */
	if(pci_fail == 0){
		if( pci_save_state(pdev) )
		{
			HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_pci - Failed pci_save_state.\n",pp->dev_minor);
			return -EIO;
		}
	}

	/* Disable pdev. This function call "spin_lock_irqsave()". */
	if(pci_fail == 0){
		pci_disable_device(pdev);
	}
	
	HFC_DBGPRT( "hfcldd%d : hfc_fx_suspend_pci - End.\n",pp->dev_minor);
	/* No Err */
	return 0;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_resume_driver
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *  host		The pointer for scsi_host information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "pp" and "host" are not NULL pointer.
 */
void
hfc_fx_resume_driver(struct port_info *pp, struct Scsi_Host *host)
{
	ulong flags = 0;
	struct mp_adap_info *mpap=NULL;
	struct core_info	*core=NULL;
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	mpap=pp->mp_adap_info;
	
	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - start.\n",pp->dev_minor);
	
	/*** get Scsi_Host ***/
	if ( scsi_host_get(host) == NULL )
	{	/* Err case */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - Not get scsi_host.\n",pp->dev_minor);
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return;
	}
		
	/*** Reset xob,xrb ***/
	hfc_fx_reset_port_info(pp);
	
	/*** Set MCW ***/
	if ( (!(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - set mcw.\n",pp->dev_minor);
		hfc_fx_set_hw_mcw_cfg(pp);
		hfc_fx_set_hw_mcw_pci(pp);
	}
	else{	/*** Set only LSI rev ***/ /* FCLNX-GPL-579 */
		if (pp->pkg.type == HFC_PKTYPE_FIVE_EX) {
			pp->pkg.lsi_rev = (uchar)hfc_fx_read_reg(pp, HFC_IOSPACE_LSIREV, 0x01);
		}
	}
	
	/*** Sets RID (for only shared mode) ***/
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - set rid.\n",pp->dev_minor);
		pp->rid = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4 );
	}

	/*** Restart HW/FW ***/
	if(mpap->pp == pp)
	{	/* This Function is the port0 on this core. */
		/* This core needs HW setup. */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - Clear mpap->status.\n",pp->dev_minor);
		HFC_ADAP_LOCK(mpap,HFC_MP_ADAP_BUSY);
		mpap->status = 0;
		mpap->mck_err_cnt = 0;
		HFC_ADAP_UNLOCK(mpap,HFC_MP_ADAP_BUSY);
	}
	
	/*** clear errcnt_info ***/
//	if(hfc_manage_info.hfcplus_enable){
	if ( hfc_manage_info.hfcldd_mp_mod ){	/* FCLNX-GPL-349 */
		hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);
	}
	else{
		hfc_fx_clear_errinfo_i(pp);
	}
	
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	
	if ( hfc_fx_start_adapter(pp) )
	{	/* Err case */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - Failed Start adapter.\n",pp->dev_minor);
		return;
	}
	
#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
	/*** start kthread for rport ***/
	if ( !hfc_manage_info.hfcldd_mp_mod ) {		/*FCLNX-GPL-315*/
		hfc_fx_start_rport(pp);
	}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */
	
	/*** Link Initialize(FRAME_A) ***/
	/* check */
	if( hfc_fx_skip_link_init(pp, FALSE) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - skip linkinitialize.\n",pp->dev_minor);
		return;
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	/* polling start */
	pp->int_check = TRUE;
	hfc_fx_w_start( pp, core, HFC_FX_INT_CHECK_TMR, 1 );
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	/* issue Link Initialize */
	hfc_fx_initialize(pp, 0);
	
	/* polling stop */
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	if(pp->hba_isolation == HFC_ISOL_START){
		if(hfc_fx_check_hba_isolation(pp) == HFC_ISOL_STOP){ 										/* FCLNX-GPL-349 */
			pp->hba_isolation = HFC_ISOL_STOP;
		}
	}
	pp->int_check = FALSE; /* Stop HFC_FX_INT_CHECK_TMR */
	hfc_fx_w_stop( pp, core, HFC_FX_INT_CHECK_TMR );
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);

	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_driver - End.\n",pp->dev_minor);
	return;
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_resume_pci
 *
 * Purpose:     resume PCI device
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *  pdev		The pointer for pci device
 *
 * Returns:     
 *  			   0 : No Err
 *  			-EIO : Err case
 *
 * Notes:       "pp" and "pdev" are not NULL pointer.
 */
int
hfc_fx_resume_pci(struct port_info *pp, struct pci_dev *pdev)
{
	ulong flags=0;
	int msi_flag=0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_pci - Start.\n",pp->dev_minor);
	
	pci_restore_state(pdev); /* FCLNX-GPL-564 */
	
	/* Enable pdev. This function call "msleep()". */
	if( pci_enable_device(pdev) )
	{
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_pci - Failed pci_enable_device.\n",pp->dev_minor);
		return -EIO;
	}
	
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	
	if (pp->is_busmaster)
	{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_pci - Called pci_set_master.\n",pp->dev_minor);
		pci_set_master(pdev);
	}
	else{
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	}
	
	/* Set Interrrupts(INTx or MSI or MSI-X) */
	msi_flag = hfc_fx_set_interrupts(pp, pp->msi_enable);
	HFC_PORTLOCK_IRQSAVE(pp,flags);
	pp->msi_flag = msi_flag;
	
	if(pp->msi_flag < 0)
	{	/* err case */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_pci - Failed to set MSI for Kernel.\n",pp->dev_minor);
		hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0x4F, (uchar *)&pp->msi_flag, 4) ;/* FCLNX-GPL-161 */
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		return -EIO;
	}
	
	/* No Err */
	HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	HFC_DBGPRT( "hfcldd%d : hfc_fx_resume_pci - End.\n",pp->dev_minor);
	return 0;
	
}
#endif	/* FCLNX-GPL-429 */

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_set_hw_mcw_cfg()
 *
 * Purpose:     Set MCW (cfg space)
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "pp" is not NULL pointer.
 *
 */
void 
hfc_fx_set_hw_mcw_cfg(struct port_info *pp)
{
	ushort pci_reg;
	
	pci_reg = (ushort) hfc_fx_read_cnfg (pp, HFC_HOST_STAT_CMD, 0x2);
	hfc_fx_write_cnfg(pp, HFC_HOST_STAT_CMD, 0x2, (uint64_t)(pci_reg|HFC_HSC_MSS|HFC_HSC_MB));
	
	if ( (ushort) hfc_fx_read_cnfg (pp, HFC_HOST_VENDER_ID, 0x2) == 0x1054 )	/* FIVE FCWIN-0217 */
	{
		if (pp->pkg.type == HFC_PKTYPE_FIVE)
		{
			uchar lsi_rev = (uchar)hfc_fx_read_cnfg(pp, 0x4a, 0x01);
			lsi_rev &= 0x07;
			if ( lsi_rev == 0x02 )
			{
				ushort      wkc;
			
				// Set registers for FIVE Pass2				/* @@ */
				
				// It is set by Max Outstanding Split Transaction one
				wkc = (ushort)hfc_fx_read_cnfg(pp, 0x6a, 0x02);
				wkc &= 0xff8f;
				hfc_fx_write_cnfg(pp, 0x6a, 0x02, wkc);
			
				// Clear Base Address Register (Upper 32-bit) with 0
				hfc_fx_write_cnfg(pp, 0x14, 0x04, 0x00000000);
				
				// Conventional PCI (not PCI-X)? 
				wkc = (ushort)hfc_fx_read_cnfg(pp, 0x94, 0x02);
				wkc &= 0x000f;
				if ( wkc == 0x0000 )
				{
					// Set Disable REQ64# Assertion
					wkc = (ushort)hfc_fx_read_cnfg(pp, 0x90, 0x02);
					wkc |= 0x0001;
					hfc_fx_write_cnfg(pp, 0x90, 0x02, wkc);
				}
			}
		}
	}
}

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_set_hw_mcw_pci()
 *
 * Purpose:     Set MCW (pci space)
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "pp" is not NULL pointer.
 *
 */
void
hfc_fx_set_hw_mcw_pci(struct port_info *pp)
{
	ushort pci_reg;
	
	pci_reg = (ushort) hfc_fx_read_cnfg (pp, HFC_HOST_STAT_CMD, 0x2);
	hfc_fx_write_cnfg(pp, HFC_HOST_STAT_CMD, 0x2, (uint64_t)(pci_reg|HFC_HSC_MSS|HFC_HSC_MB));

	if ( (ushort) hfc_fx_read_cnfg (pp, HFC_HOST_VENDER_ID, 0x2) == 0x1054 )	/* FIVE FCWIN-0217 */
	{
		if(pp->pkg.type == HFC_PKTYPE_FPP)
		{
			/* NOP */
		}
		else if(pp->pkg.type == HFC_PKTYPE_FIVE)
		{
			ushort      wkc;
			
			uchar lsi_rev = (uchar)hfc_fx_read_cnfg(pp, 0x4a, 0x01);
			lsi_rev &= 0x07;
			if ( lsi_rev == 0x02 )
			{
			
				// Set registers for FIVE Pass2				/* @@ */

				// Set MCW HFMToCKDSBL
				wkc = (uchar)hfc_fx_read_reg_ext(pp, 0x217, 0x01);
				wkc |= 0x40;
				hfc_fx_write_reg_ext(pp, 0x217, 0x01, wkc);
			
				// Set MCW HMCWBUREQSEL
				wkc = (uchar)hfc_fx_read_reg_ext(pp, 0x229, 0x01);
				wkc |= 0x08;
				hfc_fx_write_reg_ext(pp, 0x229, 0x01, wkc);		/* @@ */
			}
			// Set MCW HMCWBG(0-1)								/* FCLNX-0367 */
			wkc = (uchar)hfc_fx_read_reg_ext(pp, 0x228, 0x01);
			wkc |= 0x30;
			hfc_fx_write_reg_ext(pp, 0x228, 0x01, wkc);			/* FCLNX-0367 */
		}
		else  /* FIVE-FX */
		{
			/* Set LSI revision */ /* FCLNX-GPL-283 */
			pp->pkg.lsi_rev = (uchar)hfc_fx_read_reg(pp, HFC_IOSPACE_LSIREV, 0x01);

			/* Clear any status registers before H/W init.  */
			hfc_fx_clear_status(pp);
			/* Set MCW */
			// hfc_fx_set_mcw(pp); /* Not needed for FIVE-FX */
		}
	}
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_start_rport()
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "host" and "host->hostdata" are not NULL pointer.
 *              Caller must unlock port_lock
 */
void
hfc_fx_start_rport( struct port_info *pp )
{
	ulong flags = 0;
	struct task_struct	*tmp_thread;
	
	HFC_ENTRY("hfc_fx_start_rport");
	
	/* Startup the kernel thread for this host adapter. */
	if ( test_bit(HFC_SYSFS_RPORT, (ulong *)&pp->sysfs_control) ) {
		if (!test_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status )) {
			tmp_thread = kthread_run(hfc_fx_do_rport, pp, "hfc_fx_worker_%d", pp->instance);
			if (IS_ERR(tmp_thread)) {
				HFC_PORTLOCK_IRQSAVE(pp,flags);
				clear_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status );
				hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xBF, NULL, 0) ;
			}
			else {
				HFC_PORTLOCK_IRQSAVE(pp,flags);
				set_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status );
			}
			pp->worker_thread = tmp_thread;
			HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
		}
	}
	else {
		HFC_PORTLOCK_IRQSAVE(pp,flags);
		clear_bit( HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status );
		HFC_PORTUNLOCK_IRQRESTORE(pp,flags);
	}
	
	HFC_EXIT("hfc_fx_start_rport");
	
	return;
}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_skip_link_init()
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *
 * Returns:     
 *  			TRUE  : Skip "Link initialize".
 *  			FALSE : Don't skip "Link initialize".
 *
 * Notes:       "pp" and "pp->mp_adap_info" are not NULL pointer.
 */
int
hfc_fx_skip_link_init(struct port_info *pp, ushort bind_err)
{
	uint				 i=0;
	
	HFC_DBGPRT( "hfcldd%d : hfc_fx_skip_link_init - Start.\n",pp->dev_minor);
	
	if((hfc_pxe_boot != 0)|| (bind_err == TRUE) ){          /* FCLNX-0634 */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_skip_link_init - Bind Error.\n",pp->dev_minor);
		return 1;
	}
	
	if (( pp->isol_force == HFC_PRT_FRC_ISOL )||(pp->isol_cmd == HFC_FX_ISOL_CMD_ON)||(pp->isol_force == HFC_CHKSTP_FRC_ISOL )){		/* Force port isolated state *//* - FCLNX-546 - */
		HFC_DBGPRT( "hfcldd%d : hfc_fx_skip_link_init - Force port isolated state.\n",pp->dev_minor);
		clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		hfc_fx_force_linkdown(pp->pport, TRUE);	/* FCLNX-GPL-FX-043 */
		return 1;
	}
	else if (pp->isol_force == HFC_SHARED_PRT_FRC_ISOL ){
		HFC_DBGPRT( "hfcldd%d : hfc_fx_skip_link_init - Physical port is isolated.\n",pp->dev_minor);
		
		set_bit( HFC_PS_ISOL, (ulong *)&pp->pport->status );
		set_bit( HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->pport->status_detail2 );
		hfc_fx_w_stop( pp, NULL, HFC_FX_LINKINIT_TMR );	/* FCLNX-GPL-FX-005 */
		clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );
		clear_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );
		clear_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  (char)0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				}
			}
		}
			
		return 2;	/* FCLNX-GPL-521 */
	}
	
	if( !test_bit(HFC_ATTACH, (ulong *)&pp->attach_status ) ){					/* FCLNX-0228 */
		HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - skip hfc_fx_initialize \n");
		return 1;
	} else {
		if ( HFC_FX_MMODE_CHECK_SHADOW(pp) && HFC_FX_MMODE_CHECK_REBOOT(pp) )
		{	/* Skip link initlialization and open int_a_mask */
			HFC_DBGPRT( " ** hfcldd : hfc_fx_detect - skip hfc_fx_initialize by mlpf reboot\n"); 
			
			for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						if(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])){
							hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
							  (char)0x4, ( int )hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);
						}	/* FCLNX-GPL-FX-438 */
					}
				}
			}
			return 1;
		}
	}
	HFC_DBGPRT( "hfcldd%d : hfc_fx_skip_link_init - End.\n",pp->dev_minor);
	
	/* Don't skip "Link initialize". */

	return 0;
}

#ifdef SYSFS_SUPPORT
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
/* FCLNX-GPL-306 */
/*
 * Function:    hfc_fx_stop_rport()
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp			The pointer for adapter port information.
 *
 * Returns:     
 *  			None
 *
 * Notes:       "host" and "host->hostdata" are not NULL pointer.
 *              Caller must unlock port_lock
 */
void
hfc_fx_stop_rport(struct port_info *pp)
{
	
	if ( test_bit(HFC_KTHREAD_RUN, (ulong *)&pp->kthread_status ) ) {
		hfc_fx_kthread_stop(pp);
	}

}
#endif
#endif /* SYSFS_SUPPORT */ /* FCLNX-GPL-191 */

int hfc_fx_check_hba_isolation(struct port_info *pp)				/* FCLNX-GPL-349 */
{
	uint logdata[8];	/* FCLNX-GPL-FX-014 */
	
	if( (HFC_FX_MMODE_CHECK_SHARED(pp)) && (!HFC_FX_MMODE_CHECK_SHADOW(pp) ) ){
		if(test_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&pp->fw_support)){
			return(1);
		}
		else if((pp->ld_err_limit_s)||(pp->if_err_limit)||(pp->to_err_limit)||(pp->rt_err_enable)
		||(pp->total_abort_to)||(pp->total_tgtrst_to)) {	/* FCLNX-GPL-FX-014 */
			logdata[0] = pp->ld_err_limit_s;
			logdata[1] = pp->if_err_limit;
			logdata[2] = pp->to_err_limit;
			logdata[3] = pp->rt_err_enable;
			logdata[4] = pp->total_abort_to;	/* FCLNX-GPL-FX-014 */
			logdata[5] = pp->total_tgtrst_to;	/* FCLNX-GPL-FX-014 */
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT3, 0xD6, (uchar *)&logdata[0], 16);
		}
		return(0);
	}
	return(1);
}																/* FCLNX-GPL-349 */

/* FCLNX-GPL-428 */	
/*
 * Function:    hfc_fx_get_adap_status
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp        - Pointer to port_info 
 *
 * Returns:     adapter status
 *
 * Notes:       
 */
int	hfc_fx_get_adap_status(struct port_info *pp){
	int    rtn =0;
	uint   hyp_status;
	
	/* FCLNX-GPL-147 */
	if(!HFC_FX_MMODE_CHECK_SHARED(pp)){
		if(!test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){
			if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){     //FCLNX-0488
				if(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)||test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
					rtn = HFC_WAITLINKUP;
				}
				else{ 	/* FCLNX-0488 */
					rtn = HFC_LINKUP;		   /* FCLNX-0488 */
				}
			}
			else{
				rtn = HFC_LINKDOWN;
			}
		}
		else if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_CHKSTP_E;	 				/* FCLNX-0488 */
		}
		else if(test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_ISOLATE_C; 				/* FCLNX-0488 */
		}
		else if(test_bit(HFC_PD_ISOLATE_PORT_E, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_ISOLATE_E;			/* FCLNX-0488 */
		}
		else if(test_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
		}
		else if(test_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
		}
		else if(test_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)(ulong *)&pp->status_detail2)){
			rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
		}
		else{
			rtn = HFC_UNKNOWN_STATUS;			/* FCLNX-0488 */
		}
	}
	else{	/* FCLNX-GPL-428 */
		if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support)){
	
			hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
	
			if(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_CSTP){
				rtn = HFC_CHKSTP_E;
			}
			else if((hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)||
			(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)||
			(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL)){
				if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_HYPSTATUS_ISOLCMD, HFC_CHECK_HYPER_STATE )){
					rtn = HFC_ISOLATE_C;
				}
				else{
					rtn = HFC_ISOLATE_E;
				}
			}
			else{
				if(!test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){
					if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){     //FCLNX-0488
						if(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)||test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
							rtn = HFC_WAITLINKUP;
						}
						else{ 	/* FCLNX-0488 */
							rtn = HFC_LINKUP;		   /* FCLNX-0488 */
						}
					}
					else{
						rtn = HFC_LINKDOWN;
					}
				}
				else if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_CHKSTP_E;	 				/* FCLNX-0488 */
				}
				else if(test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_ISOLATE_C; 				/* FCLNX-0488 */
				}
				else if(test_bit(HFC_PD_ISOLATE_PORT_E, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_ISOLATE_E;			/* FCLNX-0488 */
				}
				else if(test_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
				}
				else if(test_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
				}
				else if(test_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)(ulong *)&pp->status_detail2)){
					rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
				}
				else{
					rtn = HFC_UNKNOWN_STATUS;			/* FCLNX-0488 */
				}
			}
		}
		else{
			if(!test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){
				if(test_bit(HFC_PS_ONLINE, (ulong *)&pp->status)){     //FCLNX-0488
					if(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)||test_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status)){
						rtn = HFC_WAITLINKUP;
					}
					else{ 	/* FCLNX-0488 */
						rtn = HFC_LINKUP;		   /* FCLNX-0488 */
					}
				}
				else{
					rtn = HFC_LINKDOWN;
				}
			}
			else if(test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_CHKSTP_E;	 				/* FCLNX-0488 */
			}
			else if(test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_ISOLATE_C; 				/* FCLNX-0488 */
			}
			else if(test_bit(HFC_PD_ISOLATE_PORT_E, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_ISOLATE_E;			/* FCLNX-0488 */
			}
			else if(test_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_SFPFAIL;				/* FCLNX-0488 */
			}
			else if(test_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_SFPNOTSUPPORT;		/* FCLNX-0488 */
			}
			else if(test_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)(ulong *)&pp->status_detail2)){
				rtn = HFC_SFPDOWN;				/* FCLNX-0488 */
			}
			else{
				rtn = HFC_UNKNOWN_STATUS;			/* FCLNX-0488 */
			}
		}
	}
	return(rtn);
}
	/* FCLNX-GPL-428 */

int hfc_fx_param_search(char *search_str, int *value)
{
	return 0;
}

void hfc_fx_assign_core_no(struct port_info *pp, struct dev_info_fx *dev)
{
	struct region_info	*rp = NULL;
	uchar core_no;
	uchar online_core_num = 0;
	uchar i;
	
	rp = pp->region_arg[pp->rid];
	
	if ((pp->core_control != HFC_FX_CORECTL_AT_ASSGN_LU) &&
		(pp->core_control != HFC_FX_CORECTL_SEQUENTIAL)) {
		return;
	}
	
	if (pp->core_control == HFC_FX_CORECTL_SEQUENTIAL) {
		dev->curr_core = pp->master_core_no;
		return;
	}
	
	/* 4-port adapter */
	if( pp->core_num == 1 ){
		dev->curr_core = pp->master_core_no;
		pp->curr_core = pp->master_core_no;
		return;
	}
	
	/* Count online_core_num */
	for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(!hfc_fx_check_cs_disable(pp, rp->core_arg[i])){	/* FCLNX-GPL-FX-438 */
			online_core_num++;
		}
	}

	if(online_core_num == 1){
		dev->curr_core = pp->master_core_no;
		pp->curr_core = pp->master_core_no;
		return;
	}
	
	core_no = pp->curr_core;
	if(pp->core_num == 2) {
		core_no = (core_no == 0 ? 2 : 0);
	}
	else { /* 4core/port */
		/* search next online core */
		for (i=0; i< MAX_CORE_PROBE_FX; i++) {
			if(!hfc_fx_check_cs_disable(pp, rp->core_arg[i])){	/* FCLNX-GPL-FX-438 */
				break;
			}
		}
	}
	
	dev->curr_core = core_no;
	pp->curr_core++;
	if (pp->curr_core >= MAX_CORE_PROBE_FX) { 
		pp->curr_core = 0;
	}
}

int hfc_fx_mq_attach(struct port_info *pp)
{
	struct port_info	*vpp, *wkpp;
	int					i,j,k;
	uint				init_addr;
	
	HFC_ENTRY("hfc_fx_mq_attach");
	
	for (i=1; i<=pp->max_vport_count; i++)
	{
		vpp = (struct port_info *)hfc_fx_kmalloc(pp, sizeof(struct port_info), GFP_KERNEL);
		if (vpp == NULL) {
			goto attachmem_error_exit;
		}
		memset( vpp, 0, sizeof(struct port_info) );
		
		/* Set structure character name */
		strcpy(vpp->name, "port_info");
		
		vpp->fc_vport = pp->fc_vport;
		vpp->raslog_install = pp->raslog_install;
		
		vpp->manage_info = pp->manage_info;
		
		vpp->hosts = pp->hosts;
		vpp->host_no = pp->host_no;
		vpp->pci_cfginf = pp->pci_cfginf;
		
		/* don't call hfc_fx_query_devid() */
		vpp->pkg.vender_id		= pp->pkg.vender_id;
		vpp->pkg.device_id		= pp->pkg.device_id;
		vpp->pkg.sub_system_id	= pp->pkg.sub_system_id;
		vpp->pkg.type			= pp->pkg.type;
		vpp->pkg.map			= pp->pkg.map;
		vpp->pkg.port			= pp->pkg.port;
		vpp->core_num			= pp->core_num;
		
		/* don't call hfc_fx_pci_conf() */
		vpp->mem_base_addr	= pp->mem_base_addr;
		
		/* don't call hfc_fx_query_pktype() */
		vpp->pkg.code		= pp->pkg.code;
		vpp->pkg.core_no	= pp->pkg.core_no;
		vpp->pkg.one_core	= pp->pkg.one_core;
		vpp->pkg.lsi_rev	= pp->pkg.lsi_rev;
		
		vpp->pport = pp;
		hfc_fx_rid_register(pp, vpp);
		
		/* don't call hfc_fx_read_hfcbios() */
		vpp->port_no		= pp->port_no;
		vpp->automap		= pp->automap;
		vpp->narrowmap		= pp->narrowmap;
		vpp->defparam		= pp->defparam;
		vpp->linkspeed		= pp->linkspeed;
		vpp->topology		= pp->topology;
		vpp->isol_cmd		= pp->isol_cmd;
		vpp->spinup_delay	= pp->spinup_delay;
		for (j=0; j<8; j++) {
			vpp->boot_priority[j].ww_name = pp->boot_priority[j].ww_name;
			vpp->boot_priority[j].lun = pp->boot_priority[j].lun;
		}
		
		vpp->instance = pp->instance;
		
		/* don't call hfc_fx_search_adapter_number() */
		HFC_MEMCPY((uchar*)&vpp->ww_name, (uchar*)&pp->ww_name, 8);
		HFC_MEMCPY((uchar*)&vpp->node_name, (uchar*)&pp->node_name, 8);
		vpp->org_ww_name = pp->org_ww_name;
		vpp->org_node_name = pp->org_node_name;
		vpp->add_ww_name = pp->add_ww_name;
		vpp->add_node_name = pp->add_node_name;
		
		vpp->unique_id		= pp->unique_id;
		vpp->dev_minor		= pp->dev_minor;
		vpp->dev_minor		= (pp->dev_minor | 0x00000100);
		vpp->dev_major		= pp->dev_major;
		HFC_MEMCPY((uchar*)&vpp->ecid[0], (uchar*)&pp->ecid[0], 64);
		
		hfc_fx_conf_setup(vpp);
		hfc_fx_param_copy(pp, vpp);
		
		memcpy(vpp->adap_id, pp->adap_id, 16);
		vpp->sys_rev = pp->sys_rev;
		memcpy(vpp->vpd_buf, pp->vpd_buf, 512);
		memcpy(vpp->model_name, pp->model_name, 16);
		
		if (hfc_fx_attach(vpp)) {
			HFC_DBGPRT("hfc_fx_mq_attach - hfc_fx_attach fail rid=%d", i);
			pp->vport_ptr[i].vport_arg = NULL;
			pp->vport_num--;
			goto attachmem_error_exit;
		}
		
		/* region_arg update */
		pp->region_arg[vpp->rid] = vpp->region_arg[vpp->rid];
		for (j=1; j<=pp->max_vport_count; j++) {
			wkpp = pp->vport_ptr[j].vport_arg;
			if (wkpp == NULL) {
				continue;
			}
			
			for (k=0;k<MAX_REGION_PROBE;k++) {
				wkpp->region_arg[k] = pp->region_arg[k];
			}
		}
		
		for(j=0;j<MAX_CORE_PROBE_FX;j+=MAX_CORE_PROBE_FX/vpp->core_num){
			if (vpp->region_arg[vpp->rid] != NULL) {
				vpp->region_arg[vpp->rid]->core_arg[j]->status =
					pp->region_arg[pp->rid]->core_arg[j]->status;
			}
		}
		
		/* Set INIT_ADDR in CCA */
		init_addr  = 0x1000;
		init_addr += 0x80 * vpp->rid;
		
		for(j=0;j<MAX_CORE_PROBE_FX;j+=MAX_CORE_PROBE_FX/vpp->core_num){
			if (vpp->region_arg[vpp->rid] != NULL) {
				hfc_fx_write_reg_ext(vpp, (uint)init_addr + j*0x20 + 0x10, (char)0x04,
					((vpp->region_arg[vpp->rid]->core_arg[j]->padr_init) >> 32));
				hfc_fx_write_reg_ext(vpp, (uint)init_addr + j*0x20 + 0x14, (char)0x04,
					(vpp->region_arg[vpp->rid]->core_arg[j]->padr_init));
			}
		}
	}
	
	HFC_EXIT("hfc_fx_mq_attach");
	
	return (0);
	
attachmem_error_exit:
	hfc_fx_mq_detach(pp);
	return (1);
}

void hfc_fx_mq_detach(struct port_info *pp)
{
	struct port_info	*vpp;
	int i;
	
	for (i=1; i<=pp->max_vport_count; i++)
	{
		pp->region_arg[i] = NULL;
		
		vpp = pp->vport_ptr[i].vport_arg;
		if (vpp == NULL)
			continue;
		
		hfc_fx_rid_unregister(pp, vpp);
		hfc_fx_detach(vpp);
	}
}

void hfc_fx_mq_change_target_info(struct port_info *pp, struct target_info_fx *target)
{
	struct port_info *vpp;
	int i;
	
	HFC_ENTRY("hfc_fx_mq_change_target_info");
	
	if (HFC_FX_MQ_VALID(pp) && HFC_FX_PHYSICAL_PORT(pp)) {	/* FCLNX-GPL-FX-270,275 */
		for (i=1; i<=pp->max_vport_count; i++) {
			vpp = pp->vport_ptr[i].vport_arg;
			if (vpp == NULL)
				continue;
			if (vpp->target_arg[target->pseq] == NULL)
				continue;
			
			if (test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags)) {
				set_bit(HFC_TF_WWN_VALID, (ulong *)&vpp->target_arg[target->pseq]->flags);
				vpp->target_arg[target->pseq]->ww_name = target->ww_name;
				vpp->target_arg[target->pseq]->node_name = target->node_name;
				vpp->target_arg[target->pseq]->prli_parm = target->prli_parm;
				vpp->target_arg[target->pseq]->mfsize = target->mfsize;
				vpp->target_arg[target->pseq]->target_id = target->target_id;
				vpp->target_arg[target->pseq]->pp = vpp;
			}
			else {
				clear_bit(HFC_TF_WWN_VALID, (ulong *)&vpp->target_arg[target->pseq]->flags);
			}
			
			if (test_bit(HFC_TF_DEVFLG_VALID, (ulong *)&target->flags)) {
				set_bit(HFC_TF_DEVFLG_VALID, (ulong *)&vpp->target_arg[target->pseq]->flags);
			}
			else {
				clear_bit(HFC_TF_DEVFLG_VALID, (ulong *)&vpp->target_arg[target->pseq]->flags);
			}
			
			vpp->tid_map[target->target_id] = target->pseq;
		}
	}
	
	HFC_EXIT("hfc_fx_mq_change_target_info");
}

/* FCLNX-GPL-FX-206 */
void hfc_fx_mq_copy_iocinfo(struct port_info *pp)
{
	struct core_info	*pcore;
	struct core_info	*vcore;
	int i;
	
	HFC_ENTRY("hfc_fx_mq_copy_iocinfo");
	
	if (HFC_FX_PHYSICAL_PORT(pp))
		return;
	if (!HFC_FX_MQ_VIRTUAL_PORT(pp))
		return;
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((vcore = pp->pport->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		if ((pcore = pp->pport->region_arg[0]->core_arg[vcore->core_no]) == NULL)
			continue;
		
		memcpy ((uchar*)&vcore->fw_init_p->fw_iocinfo,
				(uchar*)&pcore->fw_init_p->fw_iocinfo,
				64);
	}
	HFC_EXIT("hfc_fx_mq_copy_iocinfo");
}


/* FCLNX-GPL-FX-420 */
static inline int hfc_fx_search_next_socket(int from, uchar* socket_bitmap, int len){
	int i;
	int rtn = -1;
	for(i=from; i<len; i++){
		if(socket_bitmap[i] == 1){
			rtn = i;
			break;
		}
	}
	return rtn;
}

static inline int hfc_fx_count_cpu(int socket_no, uchar *cpu_core_bitmap, int len){
	struct cpuinfo_x86 	*cpuinfo=NULL;
	uint32_t			cpu;
	int					cpu_core_num=0;

	memset( cpu_core_bitmap, 0, len );
	for_each_present_cpu(cpu) {
		cpuinfo = &cpu_data(cpu);
		if( cpuinfo->phys_proc_id == socket_no
		 && cpuinfo->cpu_core_id < len
		 && cpu_core_bitmap[cpuinfo->cpu_core_id] == 0 ){
			cpu_core_bitmap[cpuinfo->cpu_core_id] = 1;
			cpu_core_num++;
		}
	}
	return cpu_core_num;
}

static inline int hfc_fx_search_next_cpu(int from_cpu_core_id, uchar *cpu_core_bitmap, int len){
	int i;
	int rtn = -1;

	for(i=from_cpu_core_id; i<len; i++){
		if(cpu_core_bitmap[i] == 1) {
			rtn = i;
			break;
		}
	}
	return rtn;
}

static inline struct cpuinfo_x86* hfc_fx_cpu_get_cpuinfo_x86(int phys_proc_id, int cpu_core_id){
	struct cpuinfo_x86 	*cpuinfo=NULL, *cpuinfo_wk;
	uint32_t			cpu;

	for_each_present_cpu(cpu) {
		cpuinfo_wk = &cpu_data(cpu);
		if((cpuinfo_wk->phys_proc_id == phys_proc_id)
			&& (cpuinfo_wk->cpu_core_id == cpu_core_id)) {
			cpuinfo = cpuinfo_wk;
			break;
		}
	}
	return cpuinfo;
}/* FCLNX-GPL-FX-420 */


/* FCLNX-GPL-FX-201 *//* FCLNX-GPL-FX-214 */
void hfc_fx_calc_cpu_num(struct port_info *pp)
{
	uint				logical_cpu=0, online_cpu=0, l=0, socket_no,cpu_core_id, cpu_core_num_per_socket, i, j;
	uint32_t			cpu;
	struct cpuinfo_x86 	*cpuinfo=NULL;
	uchar				*socket_bitmap=NULL;
	ushort				socket_num=0;
	
	uchar				*cpu_core_bitmap=NULL;
	ushort				cpu_core_num=0;
	
	if (!HFC_FX_PHYSICAL_PORT(pp))
		return;
	
	for_each_present_cpu(cpu) {
		if (cpu_online(cpu))
			online_cpu++;
		logical_cpu++;
	}
	if( logical_cpu == 0 ){
		return;
	}
	
	if( logical_cpu > HFC_MAX_CPU_NUM )
		logical_cpu = HFC_MAX_CPU_NUM;
	
	socket_bitmap = (uchar *)hfc_fx_kmalloc(pp, HFC_MAX_CPU_NUM, GFP_KERNEL);	/* FCLNX-GPL-FX-254,272 */
	if (socket_bitmap == NULL){
		return;
	}
	
	cpu_core_bitmap = (uchar *)hfc_fx_kmalloc(pp, HFC_MAX_CPU_NUM, GFP_KERNEL);	/* FCLNX-GPL-FX-254,272 */
	if (cpu_core_bitmap == NULL){
		if (socket_bitmap != NULL) {
			memset( socket_bitmap, 0, HFC_MAX_CPU_NUM );	/* FCLNX-GPL-FX-254,272 */
			hfc_fx_kfree(pp, socket_bitmap);
			socket_bitmap = NULL;
		}
		return;
	}
	
	memset( socket_bitmap, 0, HFC_MAX_CPU_NUM );	/* FCLNX-GPL-FX-254,272 */
	memset( cpu_core_bitmap, 0, HFC_MAX_CPU_NUM );	/* FCLNX-GPL-FX-254,272 */

	for(l=0; l < logical_cpu ; l++){
		cpuinfo = &cpu_data( l );
		
		/* Calcurate the number of the sockets *//* FCLNX-GPL-FX-254,272 */
		if( cpuinfo->phys_proc_id >= HFC_MAX_CPU_NUM )
			continue;

		if( socket_bitmap[cpuinfo->phys_proc_id] == 0 ){
			socket_bitmap[cpuinfo->phys_proc_id] = 1;
			socket_num++;
		}
	}
	
	if( socket_num == 0 ){	/* FCLNX-GPL-FX-254,272 */
		/* Release socket_bitmap area */
		if (socket_bitmap != NULL) {
			memset( socket_bitmap, 0, HFC_MAX_CPU_NUM );
			hfc_fx_kfree(pp, socket_bitmap);
			socket_bitmap = NULL;
		}
	
		/* Release cpu_core_bitmap area */
		if (cpu_core_bitmap != NULL) {
			memset( cpu_core_bitmap, 0, HFC_MAX_CPU_NUM );
			hfc_fx_kfree(pp, cpu_core_bitmap);
			cpu_core_bitmap = NULL;
		}
		return;
	}	/* FCLNX-GPL-FX-254,272 */
	
	/* allocate socket_info */
	pp->manage_info->socket_info = (struct socket_info*)hfc_fx_kmalloc(pp,sizeof(struct socket_info)*socket_num, GFP_KERNEL);
	if (pp->manage_info->socket_info == NULL){
		/* Release socket_bitmap area */
		if (socket_bitmap != NULL) {
			memset( socket_bitmap, 0, HFC_MAX_CPU_NUM );
			hfc_fx_kfree(pp, socket_bitmap);
			socket_bitmap = NULL;
		}

		/* Release cpu_core_bitmap area */
		if (cpu_core_bitmap != NULL) {
			memset( cpu_core_bitmap, 0, HFC_MAX_CPU_NUM );
			hfc_fx_kfree(pp, cpu_core_bitmap);
			cpu_core_bitmap = NULL;
		}
		return;
	}
	memset(pp->manage_info->socket_info, 0, sizeof(struct socket_info)*socket_num);

	socket_no = 0;
	for(i = 0; i < socket_num; i++){
		/* initialize socket_info member  */
		pp->manage_info->socket_info[i].socket_no = HFC_SOCKET_NO_INVALID;
		pp->manage_info->socket_info[i].cpu_info_list_num = 0;
		pp->manage_info->socket_info[i].cpu_into_list = NULL;
		pp->manage_info->socket_info[i].map_count = 0;
		cpumask_clear(&pp->manage_info->socket_info[i].cpumask);

		/* search online socket no */
		if((socket_no = hfc_fx_search_next_socket(socket_no, socket_bitmap, HFC_MAX_CPU_NUM))==-1) continue;

		/* set socket number */
		pp->manage_info->socket_info[i].socket_no = socket_no;

		/* count cpu core number of this socket */
		memset( cpu_core_bitmap, 0, HFC_MAX_CPU_NUM );
		cpu_core_num_per_socket = hfc_fx_count_cpu(socket_no,cpu_core_bitmap,HFC_MAX_CPU_NUM);
		if(cpu_core_num_per_socket==0) continue;
		cpu_core_num += cpu_core_num_per_socket;

		/* allocate cpu_info */
		pp->manage_info->socket_info[i].cpu_into_list = (struct cpu_info*)hfc_fx_kmalloc(pp,sizeof(struct cpu_info)*cpu_core_num_per_socket,GFP_KERNEL);
		if(pp->manage_info->socket_info[i].cpu_into_list==NULL){
			/* failed to allocate */
			continue;
		}

		/* set cpu_info_list_num */
		pp->manage_info->socket_info[i].cpu_info_list_num = cpu_core_num_per_socket;

		cpu_core_id = 0;
		for(j=0; j<cpu_core_num_per_socket; j++){
			/* initialize cpu_info member */
			pp->manage_info->socket_info[i].cpu_into_list[j].map_count = 0;
			pp->manage_info->socket_info[i].cpu_into_list[j].cpu_no = HFC_FX_CPU_NO_INVALID;
			pp->manage_info->socket_info[i].cpu_into_list[j].socket_info = &(pp->manage_info->socket_info[i]);

			/* search cpu no */
			if(((cpu_core_id = hfc_fx_search_next_cpu(cpu_core_id, cpu_core_bitmap, HFC_MAX_CPU_NUM))!=-1)
				&& ((cpuinfo = hfc_fx_cpu_get_cpuinfo_x86(socket_no, cpu_core_id))!=NULL)){
				pp->manage_info->socket_info[i].cpu_into_list[j].cpu_no = cpuinfo->cpu_index;
				cpumask_set_cpu(cpuinfo->cpu_index,&pp->manage_info->socket_info[i].cpumask);
				cpu_core_id++;  /* increment cpucore search header */
			}
		}

		socket_no++; /* increment socket search header */
	}

	pp->manage_info->socket_num = socket_num;
	pp->manage_info->cpu_core_num = cpu_core_num;
	pp->manage_info->logical_cpu_num = logical_cpu;
	pp->manage_info->online_cpu_num = online_cpu;
	pp->socket_num = socket_num;
	
	/* Release socket_bitmap area */
	if (socket_bitmap != NULL) {
		memset( socket_bitmap, 0, HFC_MAX_CPU_NUM );	/* FCLNX-GPL-FX-254,272 */
		hfc_fx_kfree(pp, socket_bitmap);
		socket_bitmap = NULL;
	}
	
	/* Release cpu_core_bitmap area */
	if (cpu_core_bitmap != NULL) {
		memset( cpu_core_bitmap, 0, HFC_MAX_CPU_NUM );	/* FCLNX-GPL-FX-254,272 */
		hfc_fx_kfree(pp, cpu_core_bitmap);
		cpu_core_bitmap = NULL;
	}
	
	return;
}

