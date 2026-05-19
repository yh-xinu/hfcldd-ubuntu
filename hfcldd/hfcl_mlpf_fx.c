/*
 * hfcl_mlpf_fx.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */

char mlpf_fx_rcsid[] = "$Id: hfcl_mlpf_fx.c,v 1.1.2.12.2.3.2.1.2.16 2015/07/29 08:04:06 toyo Exp $";

#include "hfcldd.h"
#include "hfclddcom.h"
#include "hfcl_detect.h"
#include "hfcl_hand_timer_trace.h"
#include "hfcl_top.h"
#include "hfcl_tbol.h"
#include "hfcl_mlpf.h"
#include "hfcl_ioctl.h"
#include "hfcl_strategy.h"
#include "hfcl_timer_recovery.h"
#include "hfcldd_conf.h"
#include "hfcl_handler.h"											/* FCLNX-GPL-489 */

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

/* MMIO-HG space address map */										/* @MLPF STR */
#define HFC_IOHGSPC_PARTSNUM_LEN        0x10
#define HFC_IOHGSPC_SYSREV_LEN          0x4
#define HFC_IOHGSPC_ADAPID_LEN          0x10
#define HFC_IOHGSPC_VPDAREA_LEN         0x80
#define HFC_IOHGSPC_INDAREA_LIEN        0x80
#define HFC_IOHGSPC_EFI_OP_TBL_LEN      0x100
#define HFC_FX_IOHGSPC_EFI_OP_TBL_LEN   0x200
const struct hg_map hfc_fx_hg_map[1] = {
	/* MMIO-HG Ver=0200 */
	{
		{
	      /* MMIO-HG */     /* no : name		 			   size */
	      { 0x0000,         /* 0  :  HFC_IOHGSPC_ZERO			-   */
	        0x0000,         /* 1  :  HFC_IOHGSPC_LENGTH			4B  */
	        0x0004,         /* 2  :  HFC_IOHGSPC_VERSION		4B  */
	        0x0008,         /* 3  :  HFC_IOHGSPC_HYPSTATUS		4B  */
	        0x000c,         /* 4  :  HFC_IOHGSPC_CMNDREG		4B  */
	        0x0010,         /* 5  :  HFC_IOHGSPC_VFCWWPN0		4B  */
	        0x0014,         /* 6  :  HFC_IOHGSPC_VFCWWPN1		4B  */
	        0x0018,         /* 7  :  HFC_IOHGSPC_VFCWWNN0		4B  */
	        0x001c,         /* 8  :  HFC_IOHGSPC_VFCWWNN1		4B  */
	        0x0020,         /* 9  :  HFC_IOHGSPC_RID			4B  */
	        0x0024,         /* 10 :  HFC_IOHGSPC_MLPFDDVER		4B  */
	        0x0028,         /* 11 :  HFC_IOHGSPC_LPARSTATUS		4B  */
	        0x002C,         /* 12 :  HFC_IOHGSPC_HYPINTDETAIL	4B	*//* FCLNX-GPL-427 */
	        0x0000,         /* 13 :  */
	        0x0000,         /* 14 :  */
	        0x0000,         /* 15 :  */
	        0x0000,         /* 16 :  */
	        0x0000,         /* 17 :  */
	        0x0031,         /* 18 :  HFC_IOHGSPC_INDACTYPE		1B  */
	        0x0032,         /* 19 :  HFC_IOHGSPC_INDACSIZE		1B  */
	        0x0030,         /* 20 :  HFC_IOHGSPC_INDACC0		4B  */
	        0x0034,         /* 21 :  HFC_IOHGSPC_INDACC1		4B  */
	        0x0038,         /* 22 :  HFC_IOHGSPC_INDACC2		4B  */
	        0x003c,         /* 23 :  HFC_IOHGSPC_INDACC3		4B  */
	        0x0040,         /* 24 :  HFC_IOHGSPC_PARTSNUM0		4B  */
	        0x0044,         /* 25 :  HFC_IOHGSPC_PARTSNUM1		4B  */
	        0x0048,         /* 26 :  HFC_IOHGSPC_PARTSNUM2		4B  */
	        0x004c,         /* 27 :  HFC_IOHGSPC_PARTSNUM3		4B  */
	        0x0050,         /* 28 :  HFC_IOHGSPC_SYSREV0		4B  */
	        0x0054,         /* 29 :  HFC_IOHGSPC_SYSREV1		4B  */
	        0x0058,         /* 30 :  HFC_IOHGSPC_SYSREV2		4B  */
	        0x005c,         /* 31 :  HFC_IOHGSPC_SYSREV3		4B  */
	        0x0060,         /* 32 :  HFC_IOHGSPC_ADAPID0		4B  */
	        0x0064,         /* 33 :  HFC_IOHGSPC_ADAPID1		4B  */
	        0x0068,         /* 34 :  HFC_IOHGSPC_ADAPID2		4B  */
	        0x006c,         /* 35 :  HFC_IOHGSPC_ADAPID3		4B  */
	        0x0070,         /* 36 :  HFC_IOHGSPC_HVM_SUPPORT	4B  */ /* FCLNX-GPL-140 */
	        0x0074,         /* 37 :  HFC_IOHGSPC_DRV_SUPPORT	4B  */ /* FCLNX-GPL-140 */
	        0x0000,         /* 38 :  */
	        0x0000,         /* 39 :  */
	        0x0080,         /* 40 :  HFC_IOHGSPC_VPDAREA		128B */
	        0x0100,         /* 41 :  HFC_IOHGSPC_INDAREA		128B */
	        0x0180,         /* 42 :  HFC_IOHGSPC_LDS_LIMIT		2B	 */
	        0x0182,         /* 43 :  HFC_IOHGSPC_LDS_COUNT		2B	 */
	        0x0184,         /* 44 :  HFC_IOHGSPC_FCIF_LIMIT		2B	 */
	        0x0186,         /* 45 :  HFC_IOHGSPC_FCIF_COUNT		2B	 */
	        0x0188,         /* 46 :  HFC_IOHGSPC_TO_LIMIT		2B	 */
	        0x018a,         /* 47 :  HFC_IOHGSPC_TO_COUNT		2B	 */
	        0x018c,         /* 48 :  HFC_IOHGSPC_RST_LIMIT		2B	 */
	        0x018e,         /* 49 :  HFC_IOHGSPC_RST_COUNT		2B	 */
	        0x01f0,         /* 50 :  HFC_IOHGSPC_EFI_INFO0		4B  */
	        0x01f4,         /* 51 :  HFC_IOHGSPC_EFI_INFO1		4B  */
	        0x01f8,         /* 52 :  HFC_IOHGSPC_EFI_INFO2		4B  */
	        0x01fc,         /* 53 :  HFC_IOHGSPC_EFI_INFO3		4B  */
	        0x0200,         /* 54 :  HFC_IOHGSPC_EFI_OP_TBL0	4B  */
	        0x0204,         /* 55 :  HFC_IOHGSPC_EFI_OP_TBL1	4B  */
	        0x0190,         /* 56 :  HFC_IOHGSPC_HGCCA_ADDR0	4B  */
	        0x0194,         /* 57 :  HFC_IOHGSPC_HGCCA_ADDR1	4B  */
	        0x0198,         /* 58 :  HFC_IOHGSPC_HGCCA_FLAG		1B  */
	        0x0408,         /* 59 :  HFC_IOHGSPC_HYP_STATUS0	4B  */
	        0x040c,         /* 60 :  HFC_IOHGSPC_CMD_REG0		4B  */
	        0x042c,         /* 61 :  HFC_IOHGSPC_HYP_INTDETAIL0	4B  */
	        0x0448,         /* 62 :  HFC_IOHGSPC_HYP_STATUS1	4B  */
	        0x044c,         /* 63 :  HFC_IOHGSPC_CMD_REG1		4B  */
	        0x046c,         /* 64 :  HFC_IOHGSPC_HYP_INTDETAIL1	4B  */
	        0x0488,         /* 65 :  HFC_IOHGSPC_HYP_STATUS2	4B  */
	        0x048c,         /* 66 :  HFC_IOHGSPC_CMD_REG2		4B  */
	        0x04Ac,         /* 67 :  HFC_IOHGSPC_HYP_INTDETAIL2	4B  */
	        0x04C8,         /* 68 :  HFC_IOHGSPC_HYP_STATUS3	4B  */
	        0x04Cc,         /* 69 :  HFC_IOHGSPC_CMD_REG3		4B  */
	        0x04Ec,         /* 70 :  HFC_IOHGSPC_HYP_INTDETAIL3	4B  */
	        0x0000          /* 71 :  */
	        }
	    }
    }
};																	/* @MLPF END */

/*--------------------------------------------------------------------------*/
/* name : hfc_fx_version()                                                     */
/*--------------------------------------------------------------------------*/

/*
 * Function:    HFC_FX_MMODE_CHECK_BASIC
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_BASIC(
	struct port_info            *pp)
{
	if ( !(pp->mlpf_mode & HFC_MMODE_MLPF ) )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_FX_MMODE_CHECK_MLPF
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_MLPF(
	struct port_info            *pp)
{
	if (pp->mlpf_mode & HFC_MMODE_MLPF )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_FX_MMODE_CHECK_SHADOW
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_SHADOW(
	struct port_info            *pp)
{
	if (pp->mlpf_mode & HFC_MMODE_SHADOW )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_FX_MMODE_CHECK_REBOOT
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_REBOOT(
	struct port_info            *pp)
{
	if (pp->mlpf_mode & HFC_MMODE_REBOOT )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_FX_MMODE_CHECK_SHARED
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_SHARED(
	struct port_info            *pp)
{
	if ( (pp->mlpf_mode & HFC_MMODE_MLPF ) && ! (pp->mlpf_mode & HFC_MMODE_DEDICATE ) )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}


/*
 * Function:    HFC_FX_MMODE_CHECK_DEDICATE
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int HFC_FX_MMODE_CHECK_DEDICATE(
	struct port_info            *pp)
{
	if ( (pp->mlpf_mode & HFC_MMODE_MLPF ) && (pp->mlpf_mode & HFC_MMODE_DEDICATE ) )
		return HFC_FX_MMODE_CHECK_OK;
	else
		return HFC_FX_MMODE_CHECK_ERROR;
}

#define MIN_IOBASE_LEN          0x100


/*
 * Function:    hfc_fx_mlpf_setup_lparmode
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_mlpf_setup_lparmode(
	struct port_info            *pp)
{
	struct pci_dev *pdev	= NULL;
	uint                      flag4, len4;
	uint                      base4;
	unsigned long             hyper_status;
	uint                        rid_int;
	uint64_t                    mmio_hg;
	unsigned long             lpar_status,status;	/* FCLNX-GPL-393 */

	if(HFC_FX_MMODE_CHECK_SHADOW(pp)){
		HFC_DBGPRT("This LPAR is shadow LPAR\n");
	}
	else
		HFC_DBGPRT("This LPAR is Guest LPAR\n");
	
	/* PCI memory space mpaping */
	pdev = pp->pci_cfginf;
	
	base4 = pci_resource_start(pdev, 4);
	len4 = pci_resource_len(pdev, 4);
	flag4 = pci_resource_flags(pdev, 4);
	
	if ( base4 == 0 )
	{
		pp->mlpf_mode &= ~( HFC_MMODE_MLPF | HFC_MMODE_DEDICATE );
		HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: MMIO HG pointer NULL\n");
		if( HFC_FX_MMODE_CHECK_SHADOW(pp)){
			HFC_DBGPRT("Shadow LPAR\n");
			HFC_ERRPRT("hfcldd : HFC_ERR9 FC Adppter Driver error (ErrNo:0xA3) \n"); 	/* FCLNX-0357 */
			HFC_ERRPRT("hfcldd : Invalid MMIO-HG Address.\n"); 							/* FCLNX-0357 */
			return HFC_MLPF_DISABLE;                                             /* FCLNX-0*** */
		}
		else
		{
			HFC_DBGPRT("Guest LPAR\n");
			return HFC_MLPF_ENABLE;  // When the setting is not invalid, it is ended with enable
		}
	}
	else
	{
		mmio_hg = (ulong)ioremap(base4, len4);  /* FCLNX-0352 */
		
		if (!(flag4 & IORESOURCE_MEM)) {
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xA3, NULL, 0) ;/* FCLNX-0357 */ /* FCLNX-GPL-161 */
			HFC_DBGPRT("  scsi(%ld): region #0 not a PIO resource, aborting\n",
				pp->host_no);
			HFC_DBGPRT("hfcldd :  Failed to pci_resource_flags \n");			 
			return HFC_MLPF_DISABLE;
		}
		
		if (len4 < MIN_IOBASE_LEN) {
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xA3, NULL, 0) ;/* FCLNX-0357 */ /* FCLNX-GPL-161 */
			HFC_DBGPRT("hfcldd : Invalid MMIO-HG Length.\n"); 							/* FCLNX-0357 */
			HFC_DBGPRT("  scsi(%d): Invalid PCI I/O region size, aborting\n",
					(uint)pp->host_no);
			HFC_DBGPRT("hfcldd :  Failed to pci_resource_flags \n");			 
			return HFC_MLPF_DISABLE;
		}
	
		HFC_DBGPRT("mmio_hg NOT NULL\n");
		pp->hg_mem_base_addr = mmio_hg;
		HFC_DBGPRT("set pp->hg_mem_base_addr mmio_hg\n");
		
		pp->mlpf_mode |= HFC_MMODE_MLPF;
//		pp->hg_map  = (struct hg_map *) &hfc_fx_hg_map[0]; /* FCLNX-0326 *//* FCLNX-0359 */

		pp->lparmode.hg_map = (struct hg_map *) &hfc_fx_hg_map[0];         /* FCLNX-0374 */
		
		if(HFC_FX_MMODE_CHECK_SHADOW(pp))
		{
			if ( hfc_fx_mlpf_check_state_port(pp, HFC_HG_HYPSTATUS_REBOOT, HFC_CHECK_HYPER_STATE) )
			{
				HFC_DBGPRT("Shadow LPAR is rebooted\n");
				pp->mlpf_mode |= HFC_MMODE_REBOOT;
				hfc_fx_mlpf_change_state_port(pp, HFC_HG_HYPSTATUS_REBOOT, HFC_DISABLE_HYPER_STATE);	/* FCLNX-GPL-411 */
			}
		}
		/* Refer MMIO-HG HyperStatus */
		HFC_DBGPRT("Refer MMIO-HG HyperStatus\n");
		hyper_status = (unsigned long) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4 );

		if (hyper_status & HFC_HG_HYPSTATUS_MODE)
		{
			/* dedicate mode */
			if(HFC_FX_MMODE_CHECK_SHADOW(pp))
			{
				HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: This Shadow LPAR is dedicate %x\n", pp->mlpf_mode);
				HFC_ERRPRT("hfcldd : HFC_ERR9 FC Adppter Driver error (ErrNo:0xA2) \n"); 	/* FCLNX-0357 */
				HFC_ERRPRT("hfcldd : Invalid Hyper Status Mode.\n"); 						/* FCLNX-0357 */
				hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRSTATUS_ISVALID, HFC_ENABLE_LPAR_STATE);
				hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRDETAIL_SPACE, HFC_DISABLE_LPAR_STATE);
				hfc_fx_mlpf_change_state_port(pp, HFC_HG_LPRDETAIL_WWN_INVALID, HFC_ENABLE_LPAR_STATE);
				return HFC_MLPF_DISABLE;
			}
			
			pp->mlpf_mode |= HFC_MMODE_DEDICATE;
			
			/* Set rid at Dedicated Mode */
			HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: Dedicated LPAR set RID %x\n", pp->mlpf_mode);
			rid_int = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4 );
			pp->rid = 0;
			pp->hvm_rid = rid_int;
			HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: read RID from MMIOHG%x\n", pp->hvm_rid);
			
			HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: This LPAR is dedicate %x\n", pp->mlpf_mode);
		} else 
		{
			/* shared mode */
			if (hyper_status & HFC_HG_HYPSTATUS_FDISABLE)
			{
				HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: This LPAR is force disable %x\n", pp->mlpf_mode);
				return HFC_MLPF_DISABLE;
			}
			HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: This LPAR is shared %x\n", pp->mlpf_mode);
		}
	}
	
	pp->hg_cca_p = NULL;		/* FCLNX-GPL-494 */
	
	if(HFC_FX_MMODE_CHECK_SHADOW(pp))
	{
		uchar                   adpp_id[16];
		uchar                   sys_buf[4];
		uint					wk;						/* FCLNX-GPL-319 */
		uchar					addr[4];				/* FCLNX-GPL-319 */

		
		HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: Shadow LPAR set adpp_id, sysrev %x\n", pp->mlpf_mode);
		
		// Sets adpp_id and sys_buf to mmio-hg, but parts_number is set after.
		if( hfc_fx_read_flash(pp, 0x54, 4, addr)){			/* FCLNX-GPL-319 */
			return(HFC_MLPF_DISABLE);
		}
		HFC_4B_TO_4L(wk, (*(uint*)(&addr[0])));
		if(hfc_fx_read_flash(pp, wk, 16, adpp_id)){
			return(HFC_MLPF_DISABLE);
		}												/* FCLNX-GPL-319 */
		hfc_fx_mlpf_set_mmio_hg(pp, adpp_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
		
		if( hfc_fx_read_flash(pp, 0, 4, sys_buf) ){
			return(HFC_MLPF_DISABLE);
		}
		hfc_fx_mlpf_set_mmio_hg(pp, sys_buf, HFC_IOHGSPC_SYSREV0, HFC_IOHGSPC_SYSREV_LEN);
	}
	
	//Sets RID.
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) )
	{
		HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: Shared LPAR set RID %x\n", pp->mlpf_mode);
		rid_int = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4 );
		if(HFC_FX_MMODE_CHECK_SHADOW(pp)){
			pp->rid = 0x00;
		}
		else{
			pp->rid = rid_int;
		}
		pp->hvm_rid = rid_int;
		HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: read RID from MMIOHG%x\n", pp->hvm_rid);
		
		/* FCLNX-GPL-393 */
		lpar_status = (unsigned long) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4 );
		
		status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 4);
		if( !(status & HFC_HG_FIVE_FX_SUPPORT) ){
			return(HFC_MLPF_DISABLE);
		}
		
		if(!(HFC_FX_MMODE_CHECK_SHADOW(pp) )){
//			status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 4);

			if (lpar_status & HFC_HG_LPRSTATUS_ISOLSUPPRT)
				set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&pp->fw_support);
		}
		if(status & HFC_HG_LPAR_ISOLATION_SUPPORT)
			set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support);
		/* FCLNX-GPL-393 */
	}
	
	/* Set Support bits for MMIO-HG Version 2 */ /* FCLNX-GPL-140 */ /* FCLNX-GPL-306 */ /* FCLNX-GPL-393 *//* FCLNX-GPL-481 *//* FCLNX-GPL-494 */
	hfc_fx_write_hg_reg(pp,  HFC_IOHGSPC_DRV_SUPPORT, 4, 
		(
			HFC_HG_FIVE_EX_SUPPORT |
			HFC_HG_FIVE_FX_SUPPORT |
		    HFC_HG_MULTI_ALPA_SUPPORT |
		    HFC_HG_GUEST_FWUPDATE_SUPPORT |
		    HFC_HG_LPAR_ISOLATION_SUPPORT |
		    HFC_HG_LPAR_STATISTICS_SUPPORT|
		    HFC_HG_SUPPORT_COREDEDICATE
		) );/* FCLNX-GPL-FX-438 */
	HFC_DBGPRT("hfc_fx_mlpf_setup_lparmode: Exit HFC_MLPF_ENABLE %x\n", pp->mlpf_mode);

	return HFC_MLPF_ENABLE;
}


/*
 * Function:    hfc_fx_mlpf_setup_wwn
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_fx_mlpf_setup_wwn(
	struct port_info            *pp)
{
	uint                        wk;
	uchar                       adpp_id[16];


	wk = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_VFCWWPN0, 0x4 );
	pp->vfc_ww_name = wk;
	pp->vfc_ww_name <<= 32;
	HFC_DBGPRT("get vfc_wwpn form MMIOHG %llx\n", (unsigned long long)pp->vfc_ww_name);
	
	wk = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_VFCWWPN1, 0x4 );
	pp->vfc_ww_name |= wk;

	wk = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_VFCWWNN0, 0x4 );
	pp->vfc_node_name = wk;
	pp->vfc_node_name <<= 32;
	HFC_DBGPRT("get vfc_wwnn form MMIOHG %llx\n", (unsigned long long)pp->vfc_node_name);

	wk = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_VFCWWNN1, 0x4 );
	pp->vfc_node_name |= wk;

	pp->ww_name = pp->vfc_ww_name;
	pp->node_name = pp->vfc_node_name;
	HFC_DBGPRT("set adpp_info : wwpn %llx\n", (unsigned long long)pp->ww_name);
	HFC_DBGPRT("set adpp_info : wwnn %llx\n", (unsigned long long)pp->node_name);
	
	hfc_fx_mlpf_get_mmio_hg(pp, adpp_id, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);
	
	HFC_4B_TO_4L(wk, (*(uint*)(&adpp_id[0])));
	pp->org_ww_name = wk;
	pp->org_ww_name <<= 32;
	HFC_4B_TO_4L(wk, (*(uint*)(&adpp_id[4])));
	pp->org_ww_name |= wk;
	HFC_4B_TO_4L(wk, (*(uint*)(&adpp_id[8])));
	pp->org_node_name = wk;
	pp->org_node_name <<= 32;
	HFC_4B_TO_4L(wk, (*(uint*)(&adpp_id[12])));
	pp->org_node_name |= wk;
	
	pp->org_ww_name += pp->port_no * 2;
	pp->org_node_name += pp->port_no * 2;

	return HFC_MLPF_VFC_VALID;
}


/*
 * Function:    hfc_fx_mlpf_set_mmio_hg
 *
 * Purpose:     This routine sets hfcldd_data from mp_adpp_info or adpp-info to MMIOHG.
 *              This routine is considered for Shadow LPAR.
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_set_mmio_hg(
	struct port_info            *pp,
	uchar                       *buf,
	uint                        regno,
	uint                        length)
{
	uchar                       *ptr;
	uint                        value;							/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uint                        i;
	uint                        offset;

	offset = pp->lparmode.hg_map->iosp.reg[regno];                  /* FCLNX-0330 *//* FCLNX-0374 */
	HFC_DBGPRT("hfc_fx_mlpf_set_mmio_hg: offset %x\n", offset);
	HFC_DBGPRT("hfc_fx_mlpf_set_mmio_hg: length %x\n", length);
	
	ptr = (uchar *)buf;
	
	for ( i = 0 ; i < (length/4) ; i++)                         /* FCLNX-0331 */
	{
		value = 0;
		value += 0xff000000 & ((uint)ptr[(i*4)] << 24);      /* FCLNX-0333 */
		value += 0x00ff0000 & ((uint)ptr[((i*4)+1)] << 16 );
		value += 0x0000ff00 & ((uint)ptr[((i*4)+2)] << 8 );
		value += 0x000000ff & (uint)ptr[((i*4)+3)];
		
		HFC_DBGPRT("hfc_fx_mlpf_set_mmio_hg: value[%d] : %x\n", i, value);
		hfc_fx_write_reg_hg_ext(pp, offset + (i*4), 4, value);
	}	

	return;
}


/*
 * Function:    hfc_fx_mlpf_set_mmio_hg
 *
 * Purpose:     This routine gets VPDDATA from MMIOHG, and sets it mp_adpp_info.
 *              This routine is considered for Guest LPAR.
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  buf        -
 *  regno      -
 *  length     -
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_get_mmio_hg(
	struct port_info            *pp,
	uchar                       *buf,
	uint                        regno,
	uint                        length)
{
	uchar                       *ptr;
	uint                        value;
	uint                        i;
	uint                        offset;

	offset = pp->lparmode.hg_map->iosp.reg[regno];                  /* FCLNX-0374 */
	ptr = (uchar *)buf;
	
	for ( i = 0 ; i < (length / 4) ; i ++)          /* FCLNX-0332 */
	{
		value = 0;
		value = (uint)hfc_fx_read_reg_hg_ext(pp, offset + (i*4), 4);
		
		/* store data with reverse order */
		*ptr++ = (uchar)((value >> 24) & 0xFF);
		*ptr++ = (uchar)((value >> 16) & 0xFF);
		*ptr++ = (uchar)((value >> 8) & 0xFF);
		*ptr++ = (uchar)(value & 0xFF);
	}	

	return;
}


/*
 * Function:    hfc_fx_mlpf_config_check
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_mlpf_config_check(
	struct port_info            *pp,
	struct core_info			*core)
{
	uchar                       wk1=0,rtn=0;
	uchar                       wk2=0;
	uchar                       wk4=0;
	uint						status=0;	/* FCLNX-GPL-FX-407 */

	// This case is considered for Guest LPAR.
	// Guest LPAR doesn't have to check status about Link Initialize resoponse.
	// So, This routine is finished for Guest LPAR at success .
	if (!HFC_FX_MMODE_CHECK_SHADOW(pp))
		return 0;
	
	wk1 = hfc_fx_read_val( core->fw_init_p->fw_iocinfo.configure_flag);
	wk2 = hfc_fx_read_val( core->fw_init_p->fw_iocinfo.connect_type);
	/* FIVE-FX */
	wk4 = hfc_fx_read_val(core->fw_init_p->fw_iocinfo.alpa_count);
	status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);	/* FCLNX-GPL-FX-407 */
	if(pp->multiple_portid == 1)
		status |= HFC_HG_LPRSTATUS_MPID_ENABLE;	/* FCLNX-GPL-FX-407 */
	if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
		status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
	
	if ( !(wk1 & HFC_FX_FABRIC_VALID) ) 
	{
		if (( wk2 == HFC_FX_MULTI_ALPA) || ( wk2 == HFC_FX_F_PORT))	/* Multi ALPA */
		{
			status &= ~(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE);	/* FCLNX-GPL-FX-407 */

			if ( wk2 == HFC_FX_MULTI_ALPA)
				status |= ((0x00003000) | wk4);	/* FCLNX-GPL-FX-407 */
			else
				status |= 0x00001000;	/* FCLNX-GPL-FX-407 */
			
			rtn = HFC_MLPF_CONFIG_CHECK_OK;	/* FCLNX-GPL-FX-407 */
		}
		else 
		{
			status &= ~((uint)(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE));	/* FCLNX-GPL-FX-407 */
			status |= (HFC_HG_LPRSTATUS_UNSHARABLE | ((wk2 << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
			
			rtn = HFC_MLPF_CONFIG_CHECK_ERROR;	/* FCLNX-GPL-FX-407 */
		}
	}
	else if ( wk2 == HFC_FX_AL) 
	{
		status &= ~(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE);	/* FCLNX-GPL-FX-407 */
		status |= (HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | ((wk2 << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
		
		rtn = HFC_MLPF_CONFIG_CHECK_ERROR;	/* FCLNX-GPL-FX-407 */
	}
	else						/* Fabric exist & PtoP *//* FCLNX-GPL-FX-382 Start */
	{
		if ( pp->flogi_rsp_param & FLOGI_PARAM_NPIV )
		{	/* NPIV valid */
			status &= ~(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE);	/* FCLNX-GPL-FX-407 */
			status |= (HFC_HG_LPRDETAIL_FCSW | HFC_HG_LPRDETAIL_NPIV | ((wk2 << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
			
			rtn = HFC_MLPF_CONFIG_CHECK_OK;	/* FCLNX-GPL-FX-407 */
		}
		else
		{	/* NPIV invalid */
			status &= ~((uint)(HFC_HG_LPRSTATUS_LINKDOWN | HFC_HG_LPRDETAIL_SPACE | HFC_HG_LPRALPACNT_SPACE));	/* FCLNX-GPL-FX-407 */
			status |= (HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_FCSW | ((wk2 << 12) & 0x00003000));	/* FCLNX-GPL-FX-407 */
			
			rtn = HFC_MLPF_CONFIG_CHECK_ERROR;	/* FCLNX-GPL-FX-407 */
		}
	} /* FCLNX-GPL-FX-382 End */
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );	/* FCLNX-GPL-FX-407 */
	return(rtn);	/* FCLNX-GPL-FX-407 */
}


/*
 * Function:    hfc_fx_mlpf_check_state_port
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
int hfc_fx_mlpf_check_state_port(
	struct port_info            *pp,
	uint                        status,
	uint                        type)
{
	uint                        wk4 = 0;
	uint                        check;

	HFC_DBGPRT("hfc_fx_mlpf_check_state_port Enter\n");
	if ( type == HFC_CHECK_HYPER_STATE )
	{
		wk4 = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
	} else if ( type == HFC_CHECK_LPAR_STATE )
	{
		wk4 = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
	} else if ( type == HFC_CHECK_HVM_SUPPORT )	/* FCLNX-GPL-489 */
	{
		wk4 = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 0x4);
	}											/* FCLNX-GPL-489 */

	
	check = wk4 & status;
	
	if ( check == status )
	{
		HFC_DBGPRT("hfc_fx_mlpf_check_state_port End return 1\n");
		return 1;
	}
	else
	{
		HFC_DBGPRT("hfc_fx_mlpf_check_state_port End return 0\n");
		return 0;
	}

	return 0;
}


int hfc_fx_mlpf_check_state_core(
	struct port_info            *pp,
	uint                        core_no,
	uint                        status,
	uint                        type)
{
	uint                        wk4 = 0;
	uint                        check;

	HFC_DBGPRT("hfc_fx_mlpf_check_state_core Enter\n");
	if( type == HFC_CHECK_HYPER_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg_core(pp, core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, HFC_FX_CORE_OFFSET40);
	}
		
	check = wk4 & status;
	
	if ( check == status )
	{
		HFC_DBGPRT("hfc_fx_mlpf_check_state_core End return 1\n");
		return 1;
	}
	else
	{
		HFC_DBGPRT("hfc_fx_mlpf_check_state_core End return 0\n");
		return 0;
	}

	return 0;
}


/* FCLNX-GPL-393 */
/*
 * Function:    hfc_fx_mlpf_check_isol_support
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_check_isol_support(
	struct port_info            *pp)
{
	uint                        status,hyp_status;

	status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HVM_SUPPORT, 4);
	
	hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);

	if(status & HFC_HG_LPAR_ISOLATION_SUPPORT){
		set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support);
	}

	return;
}



/*
 * Function:    hfc_fx_mlpf_check_isol_psycalport
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       This function must be called before calling hfc_fx_start_adppter.
 */
void hfc_fx_mlpf_check_isol_psycalport(
	struct port_info            *pp)
{
	uint					hyp_status;
	uchar					logdata[16];

	pp->isol_force = HFC_NO_FRC_ISOL;

	if(!test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support)) return;

	hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
	
	if((hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOL)||
	(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_WAIT_ISOLRCV)||
	(hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_ISOL))
	{	/* FCLNX-GPL-427 */
		pp->isol_force = HFC_SHARED_PRT_FRC_ISOL;

		memset(logdata, 0, 16);
		pp->c_err = 0x00;
		logdata[0]=pp->c_err;
		
		if( hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){	/* FCLNX-GPL-427 */
			set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );			
			
			logdata[1]=pp->status_detail2;
			
			if( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
				hfc_fx_errlog(
					pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0x8E, logdata, 16);
			}
			else{
				hfc_fx_errlog(
					pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0xD4, logdata, 16);
			}

			HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
				pp->pci_cfginf->bus->number,
				PCI_SLOT(pp->pci_cfginf->devfn),
				PCI_FUNC(pp->pci_cfginf->devfn));
		}
		else{
			set_bit(HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->status_detail2);
			
			logdata[1]=pp->status_detail2;
			
			if( HFC_FX_MMODE_CHECK_SHADOW(pp) ){
				hfc_fx_errlog(
					pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0x8F, logdata, 16);
			}
			else{
				hfc_fx_errlog(
					pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT2, 0xD5, logdata, 16);
			}
			
			HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
				pp->pci_cfginf->bus->number,
				PCI_SLOT(pp->pci_cfginf->devfn),
				PCI_FUNC(pp->pci_cfginf->devfn));
		}
	}

	return;
}
/* FCLNX-GPL-393 */


/*
 * Function:    hfc_fx_mlpf_change_state_port
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  status     - 
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_change_state_port(
	struct port_info            *pp,
	uint                        status,
	uint                        type)
{
	uint                        wk4;

	HFC_DBGPRT("hfc_fx_mlpf_change_state_port - type=0x%08x, status=0x%08x\n", type, status);
	
	if( type == HFC_ENABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
		wk4 |= status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
		wk4 &= ~status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4, wk4 );
	} else if ( type == HFC_ENABLE_LPAR_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
		wk4 |= status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_LPAR_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
		wk4 &= ~status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, wk4 );
	} else if( type == HFC_ENABLE_DRV_SUPPORT )	/* FCLNX-GPL-489 */
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		wk4 |= status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_DRV_SUPPORT, 0x4, wk4 );
	} else if ( type == HFC_DISABLE_DRV_SUPPORT )
	{
		wk4 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_DRV_SUPPORT, 0x4);
		wk4 &= ~status;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_DRV_SUPPORT, 0x4, wk4 );
	}											/* FCLNX-GPL-489 */

	return;
}


/* FCLNX-GPL-FX-376 */
void hfc_fx_mlpf_change_state_core(
	struct port_info            *pp,
	uint                        core_no,
	uint                        status,
	uint                        type)
{
	uint                        wk4;

	HFC_DBGPRT("hfc_fx_mlpf_change_state_core - core=0x%02x,type=0x%08x, status=0x%08x\n", core_no, type, status);
	
	if( type == HFC_ENABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg_core(pp, core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, HFC_FX_CORE_OFFSET40);
		wk4 |= status;
		hfc_fx_write_hg_reg_core(pp, core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, wk4, HFC_FX_CORE_OFFSET40 );
	} else if ( type == HFC_DISABLE_HYPER_STATE )
	{
		wk4 = (uint)hfc_fx_read_hg_reg_core(pp, core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, HFC_FX_CORE_OFFSET40);
		wk4 &= ~status;
		hfc_fx_write_hg_reg_core(pp, core_no, HFC_IOHGSPC_HYP_STATUS0, 0x4, wk4, HFC_FX_CORE_OFFSET40 );
	}

	return;
}
/* FCLNX-GPL-FX-376 */


/*
 * Function:    hfc_fx_read_reg_hg_ext
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  offset     - 
 *  reg_size   - 
 *
 * Returns:     
 *
 * Notes:       
 */
uint64_t hfc_fx_read_reg_hg_ext(
	struct port_info            *pp,
	uint                        offset,
	char                        reg_size)
{
	uint   data32=0,rtn=0;

	uchar *ptr;
	ushort data16=0,rtn16=0;	/* FCLNX-0659 */

	
	if ( offset % reg_size ) {
		return (0);
	}

	ptr = (uchar *)(pp->hg_mem_base_addr + (ulong)offset);

	switch (reg_size) {
		case 1: 
				rtn = readb( (uchar  *) ptr );
				break;

		case 2: 
				data16 = readw( (ushort *) ptr );
				rtn16 = data16;	 					/* FCLNX-0659 *//* Endian Convert Error */
				HFC_2B_TO_2L(rtn16, data16);		/* FCLNX-0659 */
				rtn = (uint)rtn16;					/* FCLNX-0659 */
				break;

		case 4: 
				data32 = readl( (uint   *) ptr );
				rtn = data32;
				HFC_4B_TO_4L(rtn, data32);
				break;

		default: 
				break;
	}

	
//	HFC_DBGPRT("hfc_fx_write_reg_hg_ext end offset=0x%llx, data=0x%llx\n",offset,data);

	return rtn;										/* FCLNX-GPL-FX-120 */
}


/*
 * Function:    hfc_fx_write_reg_hg_ext
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  offset     - 
 *  reg_size   - 
 *  data       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_write_reg_hg_ext(
	struct port_info            *pp,
	uint                        offset,
	char                        reg_size,
	uint64_t                    data)
{
	uchar   *ptr;
	ushort  data16_1,data16_2=0;
	uint    data32_1,data32_2=0;

	if ( offset % reg_size ) {
		return;
	}
	
	ptr = (uchar *)( pp->hg_mem_base_addr + (ulong)offset);
	
	switch (reg_size) {
		case 1: 
				writeb( (unsigned)data, (uchar  *) ptr ); 
				break;
		case 2: 
				data16_1 = (ushort) data;
				data16_2 = data16_1;
				HFC_2B_TO_2L(data16_2, data16_1);
				writew( (unsigned)data16_2, (uchar  *) ptr );
				break;
		case 4: 
				data32_1 = (uint) data;
				data32_2 = data32_1;
				HFC_4B_TO_4L(data32_2, data32_1);

				writel( (unsigned)data32_2, (uchar  *) ptr );
				break;

		default: 
				break;
	}
	
	HFC_DBGPRT("hfc_fx_write_reg_hg_ext end \n");

	return;
}


/*
 * Function:    hfc_fx_mlpf_pci_error
 *
 * Purpose:     It hpppens as follows, the following functions are called with 
 *              fc_abend() for Guest LPAR, and it comes off
 *              case HFC_ABEND_SERR:
 *              case HFC_ABEND_PERR:
 *              case HFC_ABEND_SPERR:
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_pci_error(
	struct port_info            *pp,
	uchar                       type)
{
	uint                    err_no=0 ;
	uchar                   data[16];
	
	memset(data, 0, 16);
	
	if( type == HFC_ABEND_SERR )
		err_no = 0x00000032 ;
	
	if( type == HFC_ABEND_PERR )
		err_no = 0x00000033 ;
	
	if( type == HFC_ABEND_SPERR )
		err_no = 0x00000034 ;
	
	hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERRD,err_no,data,16) ;

	return;
}


/*
 * Function:    hfc_fx_mlpf_intr
 *
 * Purpose:     The following functions are called at the time of sharing after int_a bit[0 to 3] is checked
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  int_a_reg  - 
 *
 * Returns:     
 *
 * Notes:       
 */
uchar hfc_fx_mlpf_intr(
	struct port_info            *pp,
	struct core_info			*core,
	uint						int_vector)
{	/* FCLNX-GPL-FX-386*/
	uint                        hyp_status=0;
	uint						hyp_int_core_detail[4], hyp_int_detail=0,i=0;	/* FCLNX-GPL-FX-386 *//* FCLNX-GPL-FX-405 */
	struct core_info			*wk_core=NULL;	/* FCLNX-GPL-FX-405 */
	
	HFC_DBGPRT("hfcldd : hfcl_intr - HFC_MLPF_HWERR bit 23 occured\n");
	
	/* Check Core_status */
	/* Read Hyper_int_detail factors */
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-405 */
		hyp_int_core_detail[i] = hfc_fx_read_hg_reg_core(pp, i, (uint)HFC_IOHGSPC_HYP_INTDETAIL0,
							(char)0x4, HFC_FX_CORE_OFFSET40);
		HFC_DBGPRT("hfcldd : hfcl_intr - HYPER INT CORE DETAIL[%d]=0x%08x\n", i, hyp_int_core_detail[i]);
	}	/* FCLNX-GPL-FX-405 */
	
	/* Read Hyper_core_status */
	/* read HYPER status of the core before reset int_a_reg.     */
	hyp_int_detail = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPINTDETAIL, 0x4);	/* FCLNX-GPL-FX-386 */
	HFC_DBGPRT("hfcldd : hfcl_intr - HYPER INT DETAIL=0x%08x\n", hyp_int_detail);
	
	/* FCLNX-GPL-FX-386 Start */
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HYPINTDETAIL, 0x4, hyp_int_detail);
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if (pp->region_arg[pp->rid]->core_arg[i] == NULL)
			continue;
		
		hfc_fx_write_hg_reg_core(pp, i, HFC_IOHGSPC_HYP_INTDETAIL0, 
			0x4, hyp_int_core_detail[i], HFC_FX_CORE_OFFSET40);
	}
	/* FCLNX-GPL-FX-386 End */
	
	/* reset PCI space for HW_MLPF_HWERR			 */
	hfc_fx_write_reg( pp, HFC_IOSPACE_INT_VECTOR_RST, (char)0x4, (int_vector & 0x01010101) );	/* FCLNX-GPL-FX-386 */
	
	/* FCLNX-GPL-427 */
	hyp_status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
			
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_INT, 0x20, pp, pp->region_arg[pp->rid], wk_core, NULL, NULL, hyp_int_core_detail[i], hyp_status, 0);	/* FCLNX-GPL-393 */	/* FCLNX-GPL-FX-414 */
			
			if(hyp_int_core_detail[i] & HFC_HG_HYPINTDET_CSTP_END)
				hfc_fx_mlpf_hwerr_int_detail(pp, wk_core, hyp_status, (hyp_int_core_detail[i] & HFC_HG_HYPINTDET_CSTP_END));	/* FCLNX-GPL-FX-386,414 */
	}
	
	if(test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) || (hyp_int_detail & ~HFC_HG_HYPINTDET_CSTP_END) )
		hfc_fx_mlpf_hwerr_int_detail(pp, core, hyp_status, (hyp_int_detail & ~HFC_HG_HYPINTDET_CSTP_END));	/* FCLNX-GPL-FX-386,414 */
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_INT, 0x10, pp, NULL, NULL, NULL, NULL, hyp_int_detail, hyp_status, 0);	/* FCLNX-GPL-393 */
	
	return(1);
}


/*
 * Function:    hfc_fx_mlpf_hwerr_int_detail
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  hyp_status - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_hwerr_int_detail(
	struct port_info     *pp,
	struct core_info     *core,
	unsigned int         hyp_status,
	unsigned int         hyp_int_detail)
{
	int skip = 0, fw_start = 0, cnt = 0;
	uint hyp_condition = 0, wk=0, addr=0, i=0, available_core=0, lp=0;	/* FCLNX-GPL-FX-406 */
	uint *hypint_priority = NULL;
	struct core_info     *wk_core;
	struct target_info_fx	*target=NULL;
	
	uint hypint_priority_normal[16] = { /* HyperCondition(Normal) and HypIntDetail(RecvIsol=0) */
		HFC_HG_HYPINTDET_FMCK, HFC_HG_HYPINTDET_FCSTP, HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END,
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), HFC_HG_HYPINTDET_F_ISOL_END,
		HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END, HFC_HG_HYPINTDET_LINKEND, 
		0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_normal_isolrcv[16] = { /* HyperCondition(Normal) and HypIntDetail(RecvIsol=1) */
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END,
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		HFC_HG_HYPINTDET_LINKEND, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_mck[16] = { /* HyperCondition(MCK) */
		HFC_HG_HYPINTDET_MCK_END, (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_LINKEND, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	uint hypint_priority_isol[16] = { /* HyperCondition(ISOL/WaitISOL/WaitRcvISOL) */
		HFC_HG_HYPINTDET_MCK, HFC_HG_HYPINTDET_MCK_END, 
		(HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD), 
		HFC_HG_HYPINTDET_F_ISOL_END, HFC_HG_HYPINTDET_RCV_ISOL, HFC_HG_HYPINTDET_RCV_ISOL_END,
		HFC_HG_HYPINTDET_LINKEND, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};
	
	if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
	{
		// check Force CSTP INT
		if( hyp_int_detail & HFC_HG_HYPINTDET_FCSTP)
		{
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x01, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
			
			hfc_fx_mlpf_change_state_core(
				pp, core->core_no,
				( HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML ),
				HFC_DISABLE_HYPER_STATE );
			hfc_fx_w_stop(pp, core, HFC_FX_MLPF_FCSTP_TMR) ;
			clear_bit(HFC_PD_MLPF_WAIT_FCSTP, (ulong *)&pp->status_detail2 );
			
			if((  hyp_status &  HFC_HG_HYPSTATUS_FCSTP ) || ( hyp_status & HFC_HG_HYPSTATUS_FCSTP_IML ))
				hfc_fx_chk_stop(pp);
			
			return;
		}
		// check FMCK INT
		else if( hyp_int_detail &  HFC_HG_HYPINTDET_FMCK )
		{
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x05, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
			
			if((test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) ||
			(test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) ||
			(test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2))){

				hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
				0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
				return;
			}
			
			if(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL){
				pp->c_err = HFC_ISOLATE_SHADOW;
				set_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol);
			}
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLCMD ){
				set_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol);
			}
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLERR ){
				set_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol);
			}
			
			hfc_fx_w_stop(pp, core, HFC_FX_MLPF_FMCK_TMR) ;
			clear_bit(HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 );
			
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-077,406 */
				if(pp->region_arg[pp->rid] != NULL){
					if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
						addr = 0x326 + 0x80*i;
						wk = hfc_fx_read_reg_ext(pp, ( uint )addr, ( char )0x1 );
						HFC_DBGPRT(" hfcldd%d : hfc_fx_mlpf_hwerr_int_detail - start addr=%08x wk = %x \n", pp->dev_minor, addr, wk);
						hfc_fx_write_reg_ext(pp, ( uint )addr, (char)0x1, 0x00);	/* FCLNX-GPL-FX-405 */
						if(wk  == HFC_ABEND_LINK_RESET){
							break;
						}
					}
				}
			}																			/* FCLNX-GPL-FX-077 */
			if(wk == HFC_ABEND_LINK_RESET){
				set_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );	/* FCLNX-GPL-FX-423 */
				hfc_fx_issue_forced_mck(pp, core, HFC_ABEND_LINK_RESET);
			}else{
				hfc_fx_issue_forced_mck(pp, core, HFC_ABEND_T3);
			}	/* FCLNX-GPL-FX-406 */
			
			return;
		}
		// check Force Isolate INT	/* FCLNX-GPL-393 */
		else if ( hyp_int_detail & ( HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD ) )
		{
			
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x04, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
			
			if((hyp_int_detail &  HFC_HG_HYPINTDET_F_ISOLERR) && 
			  ((test_bit(HFC_PS_ISOL, (ulong *)&pp->status))||(test_bit(HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2)))){	/* FCLNX-GPL-433 */
				hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,
				0xb1,(uchar*)&hyp_status,4);	/* FCLNX-GPL-423 */
				return;
			}
			
			if((test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol))||(test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol))){
				/* Force Isolate Interrupt is not executed in running force isolate process. */
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x06, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
				return;
			}
			
			if( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL ){
				hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
			}
			
			pp->c_err = HFC_ISOLATE_SHADOW;		/* FCLNX-GPL-426 */
			if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLCMD ){
				set_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol);
			}
			else if( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOLERR ){
				set_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol);
			}
			
			if((!test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status)) &&
			(!test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) &&
			(!test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol))){	/* FCLNX-GPL-432 */
				hfc_fx_mlpf_isol_start_slpar(pp, hyp_status);	/* FCLNX-GPL-426 */
			}
			
			return;
		}							/* FCLNX-GPL-393 */
		//check Recovery Isolate port	/* FCLNX-GPL-393 */
		else if ( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL )
		{
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x03, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
			
			if(test_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol)){
				/* Recovry Isolate Interrupt is not executed in running recovery isolate process. */
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x07, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
				return;
			}
			
			if(!test_bit(HFC_PS_ISOL, (ulong *)&pp->status)){
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x08, pp, NULL, NULL,
				NULL, NULL, hyp_status, hyp_int_detail, pp->wait_isol);
				hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOL_END);
				return;
			}
			
			pp->c_err = HFC_ISOLATE_SHADOW;
			set_bit(HFC_WAIT_ISOL_REC, (ulong *)&pp->wait_isol);
			
			if((!test_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status)) &&
			(!test_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status)) &&
			(!test_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol)) &&
			(!test_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol))){
				hfc_fx_mlpf_isol_recovery_start_slpar(pp, hyp_status);	/* FCLNX-GPL-426 */
			}
			return;
		}else{	/* FCLNX-GPL-FX-376 */
			if (hfc_fx_pcibus_chk(pp) != 0)	{
				/* "PCI BUS ERR" has hpppen. */
				HFC_FX_ISSUE_CSTP_PCIERR(pp);
			}
			else if ( test_bit(HFC_PS_MCK_RECOVERY,	(ulong *)&pp->status) && test_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2) ) {
				clear_bit(HFC_PD_FLASH_UPDATE_PROCESS,	(ulong *)&pp->status_detail2);
				hfc_fx_mck_recovery_five_fx(pp, HFC_ABEND_MCK_RESUME); /* resume MCK recovery */
			}
		}	/* FCLNX-GPL-FX-376 */
	}
	
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) && !( HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
	{
		// Check HyperIntDetail bit
		if ( hyp_int_detail & (~HFC_HG_HYPINTDET_GUEST_SUPPORT_FX) )	/* unsupported interrupt */
		{
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x30, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			if (pp->debug_func & HFC_DEBUG_HYP_INT_CHK) {
				hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xb1,(uchar*)&hyp_int_detail,4);	/* FCLNX-GPL-423 */
			}
			hyp_int_detail &= HFC_HG_HYPINTDET_GUEST_SUPPORT_FX; /* exclude unsupport interrupt */
		}
		else if ( hyp_int_detail == 0 ) {
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x31, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			if (pp->debug_func & HFC_DEBUG_HYP_INT_CHK) {
				hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4,0xb1,(uchar*)&hyp_int_detail,4);	/* FCLNX-GPL-423 */
			}
		}
		
		// CSTP END
		if ( hyp_int_detail & HFC_HG_HYPINTDET_CSTP_END )
		{
			/* FCLNX-GPL-FX-414 Start*/
			HFC_DBGPRT("hwerr_int_detail1 - core->core_no=%d\n", core->core_no);
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x10, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			if( core != NULL){
				HFC_DBGPRT( "hwerr_int_detail2 - core->core_no=%d\n", core->core_no);
				set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
				HFC_DBGPRT( "hwerr_int_detail2 - core->core_no=%d, core->status=%08x\n", core->core_no, core->status);
			}
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
			
				if(hfc_fx_check_cs_disable(pp, wk_core))
					continue;	/* FCLNX-GPL-FX-438 */
				available_core++;
			}
			if(available_core){
				HFC_DBGPRT( "hwerr_int_detail3 - core->core_no=%d\n", core->core_no);
				/* FCLNX-GPL-FX-468 Start */
				hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);	/* FCLNX-GPL-FX-191 */
				
				/* Cancel SCSI Command to the check stop core */
				for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
				{
					target = hfc_fx_hash_target_info(pp, lp);
					if( target != NULL )
					{
						for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
							if ((wk_core = pp->region_arg[pp->rid]->core_arg[i]) == NULL){
								hfc_fx_cancel_scsi_cmd(pp, wk_core, target, 0, NULL,
									SCS_LINKUP_TO, HFC_CSCSI_ERROR, TRUE, TRUE, HFC_FLASH_TARGET);
								target->status = HFC_NON_STATUS;
							}
						}
					}
				}
				/* FCLNX-GPL-FX-468 End */
				memset((void *)core->logdata, 0, 16);
				
				core->logdata[0] = (uchar)core->core_no;
				core->logdata[1] = (uchar)core->pcore_no;
				
				hfc_fx_errlog(pp,core,NULL,NULL,HFC_ERRLOG_TYPE_NONE,
					ERRID_HFCP_EVNT3, 0x2f, core->logdata, 16) ;
			}else{
				HFC_DBGPRT( "hwerr_int_detail4 - core->core_no=%d\n", core->core_no);
				if((!test_bit(HFC_PS_ISOL, (ulong *)&pp->status ))
				|| (!test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2 )))
					hfc_fx_mlpf_cstpend_int(pp);	/* FCLNX-GPL-FX-461 */
			}
			/* FCLNX-GPL-FX-414 End*/
			return;
		}
		
		// FCLNX-GPL-489 Live Migration
		if ( hyp_int_detail & (HFC_HG_HYPINTDET_MIG_END | HFC_HG_HYPINTDET_MIG_RCV) )
		{
			if ( hyp_int_detail & HFC_HG_HYPINTDET_MIG_END) {
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x21, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_fx_mlpf_migration_end(pp, core);
			}
			if ( hyp_int_detail & HFC_HG_HYPINTDET_MIG_RCV) {
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x22, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_fx_mlpf_migration_recovery(pp, core, hyp_status);
			}
			return;
		}
		
		/* HyperCondition from HyperStatus */
		hyp_condition = hfc_fx_mlpf_check_hypcondition(hyp_status);
		
		/* The interrupt processing priority level is decided by HyperCondition */
		switch ( hyp_condition )
		{
		case HFC_HYPCONDITION_MCK :
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x11, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_mck;
			break;
		case HFC_HYPCONDITION_WAIT_ISOL : 
		case HFC_HYPCONDITION_WAIT_ISOLRCV :
		case HFC_HYPCONDITION_ISOL:
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x12, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_isol;
			break;
		case HFC_HYPCONDITION_NORMAL :
			if ( !(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END) ) {
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x13, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				hypint_priority = hypint_priority_normal;
			} else {
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x14, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				hypint_priority = hypint_priority_normal_isolrcv;
			}
			break;
		case HFC_HYPCONDITION_CSTP:
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x15, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_isol;
			break;
		default:
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0xa1, pp, NULL, NULL, 
				NULL, NULL, hyp_status, hyp_int_detail, 0);
			hypint_priority = hypint_priority_isol;
			break;
		}
		
		for (cnt = 0; (cnt < 16) && (hypint_priority[cnt] != 0) ; cnt++)
		{
			if ( !(hyp_int_detail & hypint_priority[cnt]) ) {
				continue;
			}
			
			switch ( hypint_priority[cnt] )
			{
			case (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD) : // Isolate Start
				if ( !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) {
					hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x16, pp, NULL, NULL, 
						NULL, NULL, hyp_status, hyp_int_detail, 0);
					hfc_fx_mlpf_isol_start_glpar(pp, hyp_status);
				}
				break;
			case HFC_HG_HYPINTDET_F_ISOL_END : // Isolate End
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x17, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				hfc_fx_mlpf_isol_end_glpar(pp, hyp_status);
				break;
			case HFC_HG_HYPINTDET_RCV_ISOL : // Recovery Isolate Start
				skip = 0;
				if ( hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END ) {
					skip = 1;
				}
				if ( hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END ) {
					skip = 1;
				}
				if ( (hyp_int_detail & (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD)) &&
				    !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) 
				{
					skip = 1;
				}
				if (!skip) {
					hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x18, pp, NULL, NULL, 
						NULL, NULL, hyp_status, hyp_int_detail, 0);
					hfc_fx_mlpf_isol_recovery_start_glpar(pp, hyp_status);
				}
				break;
			case HFC_HG_HYPINTDET_RCV_ISOL_END : // Recovery Isolate End
				skip = 0;
				fw_start = 1;
				if ( (hyp_int_detail & (HFC_HG_HYPINTDET_F_ISOLERR | HFC_HG_HYPINTDET_F_ISOLCMD)) &&
				    !(hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) ) 
				{
					skip = 1;
				}
				if ( (hypint_priority == hypint_priority_isol) &&       /* HyperStatus(Isol) */
				     (hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) )
				{
					skip = 1;
				}
				if (!skip) {
					hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x19, pp, NULL, NULL, 
						NULL, NULL, hyp_status, hyp_int_detail, 0);
					if (hypint_priority == hypint_priority_normal_isolrcv) {
						if (hyp_int_detail & HFC_HG_HYPINTDET_MCK_END) {
							fw_start = 0;
						}
					}
					hfc_fx_mlpf_isol_recovery_end_glpar(pp, core, hyp_status, fw_start);
				}
				break;
			case HFC_HG_HYPINTDET_MCK : // MCK rec Start
				skip = 0;
				if ( hypint_priority == hypint_priority_mck )
				{
					if ( (hyp_int_detail & HFC_HG_HYPINTDET_F_ISOL_END) &&
					     !(hyp_int_detail & HFC_HG_HYPINTDET_RCV_ISOL_END) )
					{
							clear_bit ( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 );
							skip =1;
					}
				}
				if (!skip) {
					hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x1a, pp, NULL, NULL, 
						NULL, NULL, hyp_status, hyp_int_detail, 0);
					if((!test_bit(HFC_PS_ISOL, (ulong *)&pp->status ))
					|| (!test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2 )))
						hfc_fx_mlpf_mck_recovery_glpar(pp);	
				}
				
				break;
			case HFC_HG_HYPINTDET_MCK_END : // MCK rec End
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x1d, pp, NULL, NULL, 
					NULL, NULL, hyp_status, 0, 0);
				if((!test_bit(HFC_PS_ISOL, (ulong *)&pp->status ))
				|| (!test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2 )))
					hfc_fx_mlpf_mckend_int_glpar(pp);	/* FCLNX-GPL-FX-461 */
				break;
			case HFC_HG_HYPINTDET_FMCK : // FMCK INT
				if ( (hyp_int_detail & ~HFC_HG_HYPINTDET_FCSTP ) == HFC_HG_HYPINTDET_FMCK ) { /* only FMCK bit (exclude F-CSTP) */
					hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x1e, pp, NULL, NULL, 
						NULL, NULL, hyp_status, hyp_int_detail, 0);
					clear_bit(HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 );
					clear_bit ( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 );
					set_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status );		/* FCLNX-GPL-0320 */
				}
				break;
			case HFC_HG_HYPINTDET_FCSTP : // Force CSTP INT
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x1f, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				break;
			case HFC_HG_HYPINTDET_LINKEND : // Link Ini Complete
				if((!test_bit(HFC_PS_ISOL, (ulong *)&pp->status ))
				|| (!test_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2 )))
					hfc_fx_mlpf_linkend_int_glpar(pp);	/* FCLNX-GPL-FX-461 */
				break;
			default : 
				hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x20, pp, NULL, NULL, 
					NULL, NULL, hyp_status, hyp_int_detail, 0);
				break;
			} /* switch */
		} /* for */
	}
	
	HFC_DBGPRT(" hfcldd : hfc_fx_hwerr_int_detail - end\n");

	return;
}


/*
 * Function:    hfc_fx_mlpf_mck_recovery_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_mck_recovery_glpar(
	struct port_info            *pp)
{
	struct target_info_fx			*target;
	uint                        	lp, i, addr=0;	/* FCLNX-GPL-FX-077 */
	struct dev_info_fx				*dev=NULL;	/* FCLNX-GPL-0343 */
	struct core_info				*core=NULL;
	uchar							wk=0;	/* FCLNX-GPL-FX-077 */
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_mck_recovery_glpar\n", pp->dev_minor);

/* FCLNX-GPL-427 */
	if( test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) )
	{
		clear_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status );
	}
	clear_bit(HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 );
	clear_bit ( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 );
	hfc_fx_mlpf_change_state_port(pp, HFC_HG_HYPSTATUS_MCK, HFC_DISABLE_HYPER_STATE );
	set_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
	clear_bit(HFC_PS_ISOL, (ulong *)&pp->status );
/* FCLNX-GPL-427 */

	HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY); 
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)						
	{
		target = hfc_fx_hash_target_info(pp, lp);
		if (target != NULL)
		{
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				hfc_fx_notify_tout(pp, core, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */	/* FCLNX-GPL-FX-406 */
				hfc_fx_cancel_scsi_cmd(
					pp, core, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,		/* FCLNX-0429 */
					TRUE, TRUE, HFC_FLASH_TARGET);
					
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
				  (char)0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				
			}
			target->status = HFC_NON_STATUS ; 
			
			dev = target->dev;										/* FCLNX-GPL-0343 */
			while( dev != NULL ){
				dev->lustat = 0x00;
				dev = dev->next;
			}														/* FCLNX-GPL-0343 */
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_forced_offline_e(target, TRUE);
			}
		}
	}
	
	if((test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 )) && (pp->link_reset == HFC_FX_LINK_RESET_MULTI) ){
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-077 */
			if(pp->region_arg[pp->rid] != NULL){
				if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
					if(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])){	/* FCLNX-GPL-FX-438 */
						addr = 0x326 + 0x80*i;
						wk = hfc_fx_read_reg_ext(pp, ( uint )addr, ( char )0x1 );
						if(wk  == HFC_ABEND_LINK_RESET){
							clear_bit( HFC_PS_ONLINE, (ulong *)&pp->status );
							break;
						}
					}
				}
			}
		}																			/* FCLNX-GPL-FX-077 */
	}

	pp->mck_err_cnt++ ;

	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				if(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])){	/* FCLNX-GPL-FX-438 */
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  ( char )0x4, HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}
	hfc_fx_reset_all_timer(pp);	/* FCLNX-GPL-FX-190 */

	return;
}


/*
 * Function:    hfc_fx_mlpf_mckend_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_mckend_int(
	struct port_info            *pp)
{
	struct core_info	*core=NULL;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_mckend_int\n", pp->dev_minor);
	
	set_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );	/* FCLNX-GPL-FX-005 */
	hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_MCK_TMR );
	hfc_fx_w_start( pp, core, HFC_FX_WLINKUP_MCK_TMR, pp->linkup_tmo );

}


/*
 * Function:    hfc_fx_mlpf_mckend_int_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_mckend_int_glpar(
	struct port_info            *pp)
{
	struct core_info	*core=NULL;
	uint				i=0;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_mckend_int_glpar\n", pp->dev_minor);

	hfc_fx_mlpf_errlog_glpar(pp);
	
	hfc_fx_reset_port_info(pp);
	
	if ( hfc_fx_mlpf_check_normal_hypsts(pp) ) { /* FCLNX-GPL-427 HyperStatus Normal, execute F/W start */

		/* INITTBL ADR(0x310)  */
		/* ALPA(0x319)         */
		hfc_fx_reset_start(pp, HFC_SET_INIADR);
		
		pp->mck_linkup = HFC_LINKUP_MCK;			/* FCLNX-GPL-393 */

		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
		
		/* FCLNX-GPL-FX-414 Start*/
		/* Determine master core */
		hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);
		
		/* Set fw_init_tbl */
		hfc_fx_set_fw_init_tbl(pp);					/* Issue Core_Start to all the core's */
		/* FCLNX-GPL-FX-414 End*/
		
		/* Issue Core_Start to all the core's */
		set_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		atomic_set(&pp->check_mbreq, 1);
		
		/* Start core_start mailbox for all the core */
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if(pp->region_arg[pp->rid] != NULL){
				if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				if(hfc_fx_check_cs_disable(pp, core))
					continue;	/* FCLNX-GPL-FX-438 */

				if( hfc_fx_mlpf_check_state_core(pp, i,  HFC_HG_HYPSTATUS_CSTPEND, HFC_CHECK_HYPER_STATE )
				|| !hfc_fx_mlpf_check_state_core(pp, i,  HFC_HG_HYPSTATUS_ENABLE, HFC_CHECK_HYPER_STATE) ){
					set_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
					continue;
				}
				
				/* Enable interrupt */
				HFC_DBGPRT( " hfcldd : hfc_fx_core_start - enable interrupt\n");
				
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
				  (char)0x4, ( int )hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);	/* FCLNX-GPL-FX-422 */
			}
		}
		
		if( pp->region_arg[pp->rid] != NULL){
			core = pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
		}
	
		if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
			if(pp->mck_on_sleep) {
				hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);
			}
		} 

		/* Link Up Timer Set */
		//@MLPF
		set_bit( HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status );	/* FCLNX-GPL-FX-005 */
	
		hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_MCK_TMR );
		hfc_fx_w_start( pp, core, HFC_FX_WLINKUP_MCK_TMR, pp->linkup_tmo );
	}
	else { /* FCLNX-GPL-427 HyperStatus NOT Normal */
		pp->mck_linkup = HFC_LINKUP_MCK;			/* FCLNX-GPL-393 */
		
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
	}
	
	for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
		core = pp->region_arg[pp->rid]->core_arg[i];
		if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
		{
			hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
			break;
		}
	}
	
	if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) )
	{
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
	}

	return;
}

/* FCLNX-GPL-FX-376 Start */
/*
 * Function:    hfc_fx_mlpf_linkend_int_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_linkend_int_glpar(
	struct port_info            *pp)
{
	/* FCLNX-GPL-FX-386 Start */
	struct core_info	*core=pp->region_arg[pp->rid]->core_arg[pp->master_core_no];
	uchar domain=0;
	/* FCLNX-GPL-FX-386 End */
	
		HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar\n", pp->dev_minor);

	
	if(!test_bit ( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 ))
		return;	/* FCLNX-GPL-FX-426 */
	
	clear_bit ( HFC_PD_MLPF_WAIT_LINKEND, (ulong *)&pp->status_detail2 );
	
	if(test_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status)
	|| !test_bit(HFC_PS_CONNECTED, (ulong *)&pp->status ))	/* FCLNX-GPL-FX-436 */
		return;
	
	if ( hfc_fx_mlpf_check_normal_hypsts(pp) ) { /* FCLNX-GPL-427 HyperStatus Normal, execute F/W start *//* FCLNX-GPL-FX-386 Start */
		if((!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_LINKDOWN , HFC_CHECK_LPAR_STATE) )
		&&(!hfc_fx_mlpf_check_state_port(pp,HFC_HG_LPRSTATUS_UNSHARABLE , HFC_CHECK_LPAR_STATE) )){	/* FCLNX-GPL-FX-401 */
			if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_LPRDETAIL_MULTIALPA, HFC_CHECK_LPAR_STATE)){
				HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar - Complete Link Initialize for Mulit ALPA.\n",pp->dev_minor);
				hfc_fx_copy_master_to_slave( pp, core );
				hfc_fx_change_portstat_linkup(pp, core);
				hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
			}
			else if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_LPRDETAIL_FPORT, HFC_CHECK_LPAR_STATE)){
				HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar - Complete Link Initialize for F_PORT.\n",pp->dev_minor);
				pp->scsi_id = 0;
				domain = (uchar)pp->rid + 0x80;
				pp->scsi_id |= (uint64_t)(domain << 16);
				
				core->fw_init_p->fw_iocinfo.configure_flag = ( HFC_FL_PID_VALID | HFC_FL_P2P_PID_VALID );	/* FCLNX-GPL-398 */
				
				/* Set p2p_tgt_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[0] = 0x01;
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[1] = 0;
				core->fw_init_p->fw_iocinfo.p2p_tgt_port_id[2] = 0;
				
				/* Set self_port_id of Init_Table */
				core->fw_init_p->fw_iocinfo.self_port_id[0] = domain;
				core->fw_init_p->fw_iocinfo.self_port_id[1] = 0;
				core->fw_init_p->fw_iocinfo.self_port_id[2] = 0;
				
				hfc_fx_copy_master_to_slave( pp, core );
				hfc_fx_change_portstat_linkup(pp, core);
				hfc_fx_wwnverify_linkup(pp, NULL, core, 0, 0);
			}
			else if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_LPRDETAIL_FCSW | HFC_HG_LPRDETAIL_NPIV | HFC_HG_LPRDETAIL_FCSWPTOP, HFC_CHECK_LPAR_STATE)){
				HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar - Complete Link Initialize for FCSW.\n",pp->dev_minor);
			/* Issue Core_Start to all the core's */
				set_bit( HFC_PD_NEED_FLOGI, (ulong *)&pp->status_detail1 );
				atomic_set(&pp->check_mbreq, 1);
			}else{
				HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_linkend_int_glpar - The vaalue of LPAR Status is undefined.\n",pp->dev_minor);
				hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-18 */
				hfc_fx_change_portstat_linkdown(pp, core);
				if(pp->initialize != 0){
					hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
				}
			}
		}else{
			hfc_fx_copy_master_to_slave( pp, core );	/* FCLNX-GPL-FX-18 */
			hfc_fx_change_portstat_linkdown(pp, core);
			if(pp->initialize != 0){
				hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
			}
		}/* FCLNX-GPL-FX-386 End */
	}
	
	return;
}
/* FCLNX-GPL-FX-376 End */


/*
 * Function:    hfc_fx_mlpf_isol_start_slpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_isol_start_slpar(
	struct port_info     *pp,
	unsigned int         hyp_status)
{
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_start_slpar\n", pp->dev_minor);
	
	if(hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){
		set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );
	}
	else{
		set_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->status_detail2 );
	}

	pp->c_err = 0x00;
	
	hfc_fx_force_linkdown(pp, FALSE);	/* FCLNX-GPL-FX-043 */
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_END);     //@MLPF
	clear_bit(HFC_WAIT_ISOL_ERR, (ulong *)&pp->wait_isol);
	clear_bit(HFC_WAIT_ISOL_CMD, (ulong *)&pp->wait_isol);
			
	hfc_fx_mlpf_change_state_port(pp, HFC_HG_HYPSTATUS_ENABLE, HFC_DISABLE_HYPER_STATE );   /* FCLNX-0393 */
	hfc_fx_mlpf_indacc(pp, NULL, 0, 0, HFC_ERRLOG_TYPE_MCK);	/* FCLNX-GPL-FX-391 */

	return;
}
 
 

/*
 * Function:    hfc_fx_mlpf_isol_start_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_isol_start_glpar(
	struct port_info     *pp,
	unsigned int         hyp_status)
{
	uint i=0;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_start_glpar\n", pp->dev_minor);
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if((pp->region_arg[pp->rid]->core_arg[i] != NULL)
			&&(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i]))){	/* FCLNX-GPL-FX-438 */
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
				  ( char )0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
			}
		}
	}
	
	if (hyp_status & HFC_HG_HYPSTATUS_ISOLCMD){
		set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );
	}
	else {
		set_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->status_detail2 );
	}
	hfc_fx_reset_all_timer(pp);
	set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
	
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_MLPF_ISOLEND_TMR, 0, TRUE);
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_MLPF_ISOLEND_TMR, 0, FALSE);

}

/*
 * Function:    hfc_fx_mlpf_isol_end_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_isol_end_glpar(
	struct port_info            *pp,
	unsigned int		hyp_status)
{
	// Guest LPAR only
	uint                        lp, i;
	struct target_info_fx		*target;
	uchar						logdata[16];
	struct core_info			*core=NULL;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_end_glpar\n", pp->dev_minor);
	
	hfc_fx_mlpf_isol_start_glpar(pp, hyp_status); /* FCLNX-GPL-427 */
	
	hfc_fx_watchdog_enter(pp, NULL, NULL, NULL, 0, HFC_FX_MLPF_ISOLEND_TMR, 0, TRUE);
	
	memset(logdata, 0, 16);
	logdata[0]=pp->c_err;
	logdata[1]=pp->status_detail2;
	
	if (test_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 )){
		hfc_fx_errlog(
				pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, 0xD4, logdata, 16);

		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by user command \n",
				pp->pci_cfginf->bus->number,
				PCI_SLOT(pp->pci_cfginf->devfn),
				PCI_FUNC(pp->pci_cfginf->devfn));
	}
	else{
		hfc_fx_errlog(
				pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
				ERRID_HFCP_EVNT2, 0xD5, logdata, 16);
		
		HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is isolated by error \n",
				pp->pci_cfginf->bus->number,
				PCI_SLOT(pp->pci_cfginf->devfn),
				PCI_FUNC(pp->pci_cfginf->devfn));
	}
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = hfc_fx_hash_target_valid(pp, lp);		/* FCLNX-703 *//* FCLNX-GPL-433 */
		if (target != NULL)
		{
			if ( test_bit(HFC_TF_WWN_VALID, (ulong *)&target->flags) ){	/* FCLNX-703 */
				for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
					if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;
					hfc_fx_notify_tout(pp, core, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-573 output SCSI time-out log. */	/* FCLNX-GPL-596 */
					hfc_fx_cancel_scsi_cmd(
						pp, core, target, 0, NULL, SCS_MCK, HFC_CSCSI_ERROR,
							TRUE, TRUE, HFC_FLASH_TARGET);
				}
				target->status = HFC_NON_STATUS ;
			}
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				if (test_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 )){
					hfc_manage_info.npubp->hfc_fx_forced_offline_c(target, TRUE);
				}
				hfc_manage_info.npubp->hfc_fx_forced_offline_e(target, TRUE);	/* FCLNX-704 */
			}
		}														/* FCLNX-GPL-433 */
	}
	hfc_fx_wwnverify_linkup_timeout(pp, NULL, 0);		/* FCLNX-GPL-433 */
	
	HFC_FX_MAILBOX_UNLOCK( pp, HFC_MAILBOX_BUSY );
	
	pp->status = 0;          /* FCLNX-GPL-427 *//* FCLNX-GPL-572 */
	set_bit(HFC_PS_ENABLE,  (ulong *)&pp->status);	/* FCLNX-GPL-572 */
	set_bit(HFC_PS_ISOL, (ulong *)&pp->status);	/* FCLNX-GPL-572 */
	
	if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
		if(pp->mck_on_sleep) {
			hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);	
		}
	}
	if(pp->initialize == 1) {                             /* FCLNX-GPL-431 */
		hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll); /* FCLNX-GPL-431 */
	}

	return;
}


/*
 * Function:    hfc_fx_mlpf_isol_recovery_start_slpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_isol_recovery_start_slpar(
	struct port_info	*pp,
	unsigned int		hyp_status)
{
	uint				status=0;	/* FCLNX-GPL-FX-407 */
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_recovery_start_slpar\n", pp->dev_minor);
	
	pp->c_err = 0x00;

	if(test_bit(HFC_PS_ISOL, (ulong *)&pp->status) ) { /* FCLNX-556 */
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_HWERR_INT_DET, 0x02, pp, NULL, NULL, 
			NULL, NULL, hyp_status, 0, 0);
		hfc_fx_force_linkdown_recovery(pp);
	}
	
	status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);	/* FCLNX-GPL-FX-407 */
	status &= ~(HFC_HG_LPRSTATUS_UNSHARABLE | HFC_HG_LPRDETAIL_SPACE);	/* FCLNX-GPL-FX-407 */
	status |= HFC_HG_LPRSTATUS_LINKDOWN;	/* FCLNX-GPL-FX-407 */
	if(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support))
		status |= HFC_HG_LPRSTATUS_ISOLSUPPRT;	/* FCLNX-GPL-FX-428 */
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4, status );	/* FCLNX-GPL-FX-407 */

	return;
}

/*
 * Function:    hfc_fx_mlpf_isol_recovery_start_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_isol_recovery_start_glpar(
	struct port_info	*pp,
	unsigned int		hyp_status)
{
	uint	i=0;
	struct core_info	*core=NULL;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_recovery_start_glpar\n", pp->dev_minor);
	
//	if(hyp_status & HFC_HG_HYPSTATUS_MCK){
		hfc_fx_reset_port_info(pp);
//	}
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		
		clear_bit(HFC_CS_CHK_STOP, (ulong *)&core->status);
	}
	
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);			/* FCLNX-0488 */
	}														/* FCLNX-GPL-331 */
	else{
		hfc_fx_clear_errinfo_i(pp);								/* FCLNX-GPL-349 */
	}

	set_bit ( HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2);

	return;
}

/*
 * Function:    hfc_fx_mlpf_isol_recovery_end_glpar
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
 
#define HFC_ISSUE_CORE_START	1
#define HFC_SKIP_CORE_START		2
 
void hfc_fx_mlpf_isol_recovery_end_glpar(
	struct port_info            *pp,
	struct core_info			*core,
	unsigned int		hyp_status,
	int					fw_start)
{
	
	uchar					fwstart_log = HFC_ISSUE_CORE_START;
	unsigned long long		wkp;		/* FCLNX-GPL-490 */
	int						dma_size,i=0;	/* FCLNX-GPL-490 */
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_isol_recovery_end_glpar\n", pp->dev_minor);
	
/* FCLNX-GPL-427 */
	hfc_fx_mlpf_isol_recovery_start_glpar(pp, hyp_status);

	clear_bit ( HFC_PD_ISOLATE_RECOVERY, (ulong *)&pp->status_detail2);
	HFC_DETAIL_CLEAR_ISOLREC(pp);
	pp->c_err = 0x00;
		
	if(!test_bit ( HFC_PS_ISOL, (ulong *)&pp->status)){
		hfc_fx_hand2_trace(HFC_FX_TRC_FORCE_ISOL_REC_P, 0x06, pp, NULL, NULL, 
			NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */
		fwstart_log = HFC_SKIP_CORE_START;
		goto hfc_fx_skip_isol_recovery;
	}
	
	hfc_fx_hand2_trace(HFC_FX_TRC_FORCE_ISOL_REC_P, 0x05, pp, NULL, NULL, 
		NULL, NULL, 0, 0, 0);	/* FCLNX-GPL-374 *//* FCLNX-GPL-376 */

	
	clear_bit ( HFC_PS_ISOL, (ulong *)&pp->status);

/* FCLNX-GPL-427 */

	if ( fw_start && hfc_fx_mlpf_check_normal_hypsts(pp) ) { /* FCLNX-GPL-427 HyperStatus Normal, execute F/W start */

		/* FCLNX-GPL-490 */
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
					
					hfc_fx_write_reg_core(pp, i, HFC_IOSPACE_CA_FLAG,
						0x1, 0x00, HFC_FX_CORE_OFFSET80);
				}
			}
		}
		
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
//		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&pp->mp_adpp_info->lock); /* FCLNX-GPL-177 */

		/* FCLNX-GPL-490 Start */
		/* Determine master core */
		hfc_fx_determine_master_core(pp, pp->region_arg[pp->rid]);
		
		/* Set fw_init_tbl */
		hfc_fx_set_fw_init_tbl(pp);					/* Issue Core_Start to all the core's */
		
		HFC_DETAIL_CLEAR_ISOLREC(pp);
		pp->c_err = 0x00;
		
		set_bit( HFC_PD_NEED_CORE_START, (ulong *)&pp->status_detail1 );
		atomic_set(&pp->check_mbreq, 1);
		
		clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
		set_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);	/* FCLNX-GPL-FX-005 */
		
		/* FCLNX-GPL-490 End */
		
		/* Interruption mask setting */
		for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
			if(pp->region_arg[pp->rid] != NULL){
				if((pp->region_arg[pp->rid]->core_arg[i] != NULL)
					&&(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i]))){	/* FCLNX-GPL-FX-438 */
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  ( char )0x4, hfc_inta_mask_mlpf[pp->pkg.type], HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}
	else { /* FCLNX-GPL-427 HyperStatus NOT Normal */
		clear_bit( HFC_PS_MCK_RECOVERY, (ulong *)&pp->status );
		clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
//		test_and_clear_bit( HFC_HMCK_RECOVRTY, (ulong *)&pp->mp_adpp_info->lock); /* FCLNX-GPL-177 */
		fwstart_log = HFC_SKIP_CORE_START;
	}

	if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
	{
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
	}
	if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) )
	{
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
	}

hfc_fx_skip_isol_recovery:
/* FCLNX-GPL-427 */
	hfc_fx_errlog(
		pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE,
		ERRID_HFCP_EVNT2, 0xD3, &fwstart_log, 1);
				
	HFC_ERRPRT("hfcldd : Device %02x:%02x.%02x is recovered \n",
		pp->pci_cfginf->bus->number,
		PCI_SLOT(pp->pci_cfginf->devfn),
		PCI_FUNC(pp->pci_cfginf->devfn));
/* FCLNX-GPL-427 */

	if( ( fwstart_log == HFC_SKIP_CORE_START)&&(pp->initialize != 0) ){							/* FCLNX-GPL-521 */
		hfc_fx_wake_up(&pp->init_event,&pp->int_a_poll);
	}													/* FCLNX-GPL-521 */

	return;
}

/*
 * Function:    hfc_fx_mlpf_check_hypcondition
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   hyper status
 *
 * Returns:     HyperCondition
 *
 * Notes:       
 */
uint hfc_fx_mlpf_check_hypcondition(unsigned int hyp_status)
{
	HFC_DBGPRT("hfcldd : hfc_fx_mlpf_check_hypcondition\n");
	
	if((!(hyp_status & (HFC_HG_HYPSTATUS_ENABLE)))
	&& (!(hyp_status & (HFC_HG_HYPSTATUS_ISOL)))){
		return HFC_HYPCONDITION_CSTP;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_WAIT_ISOL) {
		return HFC_HYPCONDITION_WAIT_ISOL;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_WAIT_RCV_ISOL) {
		return HFC_HYPCONDITION_WAIT_ISOLRCV;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_ISOL) {
		return HFC_HYPCONDITION_ISOL;
	}
	else if (hyp_status & HFC_HG_HYPSTATUS_MCK) {
		return HFC_HYPCONDITION_MCK;
	}
	else {
		return HFC_HYPCONDITION_NORMAL;
	}

	return 0;
}

/*
 * Function:    hfc_fx_mlpf_check_normal_hypsts
 *
 * Purpose:     FCLNX-GPL-427
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     HyperStatus  1:normal 0:not normal 
 *
 * Notes:       
 */
int hfc_fx_mlpf_check_normal_hypsts(
	struct port_info    *pp)
{
	unsigned int hyp_status;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_check_normal_hypsts\n", pp->dev_minor);

	if ( !(HFC_FX_MMODE_CHECK_SHARED(pp)) || HFC_FX_MMODE_CHECK_SHADOW(pp) ){
//	     !(test_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support)) ) {
		return 1;
	}

	hyp_status = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);
	if (hfc_fx_mlpf_check_hypcondition(hyp_status) == HFC_HYPCONDITION_NORMAL) {
		return 1;
	} else {
		return 0;
	}

	return 1;
}


/*
 * Function:    hfc_fx_mlpf_issue_ffcstp
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_issue_ffcstp(
	struct port_info            *pp,
	uchar                       type)
{
	uint                        int_a_reg, i=0;
	struct core_info			*core=NULL;
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		int_a_reg = (uint)hfc_fx_read_reg_core(pp, i, (uint)HFC_IOSPACE_INTA,
						(char)0x4, HFC_FX_CORE_OFFSET10);
	
		if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
		{
			if( ( int_a_reg & HFC_MLPF_HWERR ) &&
				( hfc_fx_mlpf_check_state_core(pp, i, HFC_HG_HYPSTATUS_FCSTP | HFC_HG_HYPSTATUS_FCSTP_IML, HFC_CHECK_HYPER_STATE ) ) )
				return;
		}
		else
		{//Guest LPAR nothing is done and return is done
			return;
		}
	}
	
	if ( !test_bit (HFC_PS_ISOL, (ulong *)&pp->status ) )
	{
		set_bit( HFC_PD_MLPF_WAIT_FCSTP, (ulong *)&pp->status_detail2 );
		
		hfc_fx_w_stop(pp, core, HFC_FX_MLPF_FCSTP_TMR) ;
		hfc_fx_w_start(pp, core, HFC_FX_MLPF_FCSTP_TMR, HFC_FX_MLPF_FCSTP_STO) ;
		
		for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
			if(pp->region_arg[pp->rid] != NULL){
				if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
					continue;
				if(hfc_fx_check_cs_disable(pp, core))
					continue;	/* FCLNX-GPL-FX-438 */
				
				/* Enable interrupt */
				HFC_DBGPRT( " hfcldd : hfc_fx_mlpf_isuue_ffcstp - enable interrupt\n");
				
				hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
				  (char)0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
			}
		}
		
		if( type == HFC_ABEND_FCSTP )
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_FFCSTP);
		else if ( type == HFC_ABEND_FCSTP_IML )
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_FFCSTP_IML);
	}

	return;
}


/*
 * Function:    hfc_fx_mlpf_issue_ffmck
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  type       - 
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_issue_ffmck(
	struct port_info            *pp,
	struct core_info			*core,
	uchar                       type)
{
	uint                        int_a_reg;
	uint                        wk_reg=0, addr=0, core_no=0,mb_code=0, i=0;	/* FCLNX-GPL-FX-077 */
	
	HFC_DBGPRT("hfc_fx_mlpf_issue_ffmck - start\n");
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
		if ((pp->region_arg[pp->rid]->core_arg[i]) == NULL)
			continue;
		int_a_reg = hfc_fx_read_reg_core(pp, i, (uint)HFC_IOSPACE_INTA,
							(char)0x4, HFC_FX_CORE_OFFSET10);
		
		if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
		{
			if( ( int_a_reg & HFC_HWERR_MCK ) ||
				( ( int_a_reg & HFC_MLPF_HWERR ) &&
				( hfc_fx_mlpf_check_state_core(pp, i, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) ) )
				return;
		}
		else
		{   //Guest LPAR
			if( ( (int_a_reg & HFC_MLPF_HWERR ) &&
				( hfc_fx_mlpf_check_state_core(pp, i, HFC_HG_HYPSTATUS_ENABLE | HFC_HG_HYPSTATUS_MCK, HFC_CHECK_HYPER_STATE ) ) ) ||
				( ( int_a_reg & HFC_MLPF_HWERR ) &&
				( hfc_fx_mlpf_check_state_core(pp, i, HFC_HG_HYPSTATUS_FMCK, HFC_CHECK_HYPER_STATE ) ) ) )
				return;
		}
	}
	
	if( !test_bit( HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 ) &&
		!test_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status ) &&
		!test_bit( HFC_PS_ISOL, (ulong *)&pp->status ) )
	{
		set_bit( HFC_PD_MLPF_WAIT_FMCK, (ulong *)&pp->status_detail2 );

		
		if( HFC_FX_MMODE_CHECK_SHADOW(pp) )
		{   // shadow LPAR only start FMCK TMR
			hfc_fx_w_stop(pp, core, HFC_FX_MLPF_FMCK_TMR) ;
			hfc_fx_w_start(pp, core, HFC_FX_MLPF_FMCK_TMR, HFC_MLPF_FMCK_STO ) ;
		}
		else
		{   // Guest LPAR set HFC_PS_WAIT_MCKINT
			set_bit( HFC_PS_WAIT_MCKINT, (ulong *)&pp->status );
		}

		if (core != NULL) {	/* FCLNX-GPL-FX-077 */
			core_no = core->core_no;
			HFC_4B_TO_4L(mb_code, core->mb->mb_init.mb_code);
			wk_reg = (uint)(mb_code >> 16);
				
			addr = 0x32c+0x80*core_no;
			hfc_fx_write_reg_ext(pp->pport,( uint )addr,( char )0x2, (ushort)wk_reg);
		}else{
			core_no = pp->master_core_no;
		}	/* FCLNX-GPL-FX-077 */
		
		wk_reg = 0;
		wk_reg |= type & 0xff;
		
		addr = 0x326+0x80*core_no;	/* FCLNX-GPL-FX-077 */
		hfc_fx_write_reg_ext(pp,( uint )addr,( char )0x1,(uchar)wk_reg);	/* FCLNX-GPL-FX-077 */
		
		wk_reg = 0;                                                             /* FCLNX-0389 */
		wk_reg |= type & 0xff;
		wk_reg <<= 8;	/* FCLNX-GPL-FX-383 */
		wk_reg |= HFC_MLPF_FFMCK;
		hfc_fx_write_hg_reg_core(pp,core_no, (uint)HFC_IOHGSPC_CMD_REG0,
			(char)0x4, ( int )wk_reg, HFC_FX_CORE_OFFSET40);
	}

	return;
}


/* FCLNX-GPL-393 */

/*
 * Function:    hfc_fx_mlpf_issue_fisolate
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *  proc       - 0:Isolate by error, 1:Isolate by command
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_fx_mlpf_issue_fisolate(
	struct port_info            *pp,
	uchar                       proc)
{
	uint hyp_status;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_issue_fisolate\n", pp->dev_minor);

	hyp_status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);

	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_FORCE_ISOL, 0x00, pp, NULL, NULL, 
		NULL, NULL, hyp_status, 0, 0);
	
	if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has hpppen. */
		HFC_FX_ISSUE_CSTP_PCIERR(pp);		/* FCLNX-GPL-400 */
		return EINVAL;
	}
	
	if (test_bit(HFC_PS_ISOL, (ulong *)&pp->status)) {	/* FCLNX-GPL-414 */
		if (test_bit(HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2)
		|| test_bit(HFC_PD_ISOLATE_CHKSTP_C, (ulong *)&pp->status_detail2)) {
		hfc_fx_hand2_trace(HFC_FX_TRC_FORCE_ISOL, 0x05, pp, NULL, NULL, 
			NULL, NULL, 0, 0, 0);
			return EINVAL;
		}
	}/* FCLNX-GPL-414 */
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_FORCE_ISOL, 0x01, pp, NULL, NULL, 
		NULL, NULL, hyp_status, 0, 0);
		
	if(proc == HFC_ISSUE_ISOLREQ_ERR){
		hfc_fx_mlpf_set_errorlimit(pp);    /* FCLNX-GPL-431 */
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_ERR);
		//hfc_fx_mlpf_set_errorlimit(pp);  /* FCLNX-GPL-431 */
	}
	else if(proc == HFC_ISSUE_ISOLREQ_CMD){
		hfc_fx_mlpf_set_errorlimit(pp);   /* FCLNX-GPL-431 */
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_F_ISOLATE_CMD);
		pp->isol_cmd_mck_cnt = (uchar)pp->mck_err_cnt; /* FCWIN-GPL-393 */
		set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );
		//hfc_fx_mlpf_set_errorlimit(pp); /* FCLNX-GPL-431 */
	}

	return 0;
}

/*
 * Function:    hfc_fx_mlpf_issue_recov_isolate
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_fx_mlpf_issue_recov_isolate(
	struct port_info            *pp)
{
	uint hyp_status;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_issue_recov_isolate\n", pp->dev_minor);

	hyp_status = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HYPSTATUS, 0x4);

	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_RECV_ISOL, 0x00, pp, NULL, NULL, 
		NULL, NULL, hyp_status, 0, 0);

	if( hfc_fx_pcibus_chk(pp) != 0 ) /* FCLNX-GPL-209 */
	{	/* "PCI BUS ERR" has hpppen. */
		return EINVAL;
	}
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_RECV_ISOL, 0x01, pp, NULL, NULL, 
		NULL, NULL, hyp_status, 0, 0);
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RECOV_ISOLATE);

	return 0;
}

/* FCLNX-GPL-393 */


/*
 * Function:    hfc_fx_mlpf_cstpend_int
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_cstpend_int(
	struct port_info            *pp)
{
	// Guest LPAR only
	uint                        lp, i;
	struct target_info_fx		*target;
	struct core_info			*core;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_cstpend_int\n", pp->dev_minor);
	
	hfc_fx_mlpf_errlog_glpar(pp);
	
	for(lp=0 ; lp<MAX_TARGET_PROBE ; lp++)
	{
		target = hfc_fx_hash_target_info(pp, lp);
		if( target != NULL )
		{
			for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {
				if(pp->region_arg[pp->rid] != NULL){
					if ((core = pp->region_arg[pp->rid]->core_arg[i]) == NULL)
						continue;

					hfc_fx_notify_tout(pp, core, target, 0, HFC_FLASH_TARGET);	/* FCLNX-GPL-596 */	/* FCLNX-GPL-FX-406 */
					hfc_fx_cancel_scsi_cmd(pp, core, target,0, NULL, SCS_MCK, HFC_CSCSI_ERROR,	/* FCLNX-0429 */
						TRUE, TRUE, HFC_FLASH_TARGET);
				}
			}
			target->status = HFC_NON_STATUS;
			
			if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
				hfc_manage_info.npubp->hfc_fx_forced_offline_e(target, TRUE);
			}
		}
	}
	
	hfc_fx_reset_all_timer(pp);	/* FCLNX-GPL-FX-190 */
	
	clear_bit(HFC_PS_MCK_RECOVERY, (ulong *)&pp->status);
	clear_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 );
	clear_bit(HFC_PS_WAIT_MCKINT, (ulong *)&pp->status);	/* FCLNX-GPL-FX-460 */
	
	if( test_bit( HFC_PD_NEED_LINK_INI, (ulong *)&pp->status_detail1)){
		if(pp->mck_on_sleep) {
			hfc_fx_wake_up(&pp->mck_event, &pp->mck_event_wait);	
		}
	} 

	set_bit(HFC_PS_ISOL, (ulong *)&pp->status ) ;	/* FCLNX-GPL-419 */
	set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2 );
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				if(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])){	/* FCLNX-GPL-FX-438 */
					hfc_fx_write_reg_core(pp, i, (uint)HFC_IOSPACE_INTA_MSK,
					  ( char )0x4, ( int )HFC_MLPF_HWERR, HFC_FX_CORE_OFFSET10);
				}
			}
		}
	}																					/* FCLNX-GPL-481*//* FCLNX-GPL-FX-422 */
	
	clear_bit(HFC_PS_ONLINE, (ulong *)&pp->status );
	
	if(pp->initialize == 1) {                             /* FCLNX-GPL-431 */
		hfc_wake_up(&pp->init_event,&pp->int_a_poll); /* FCLNX-GPL-431 */
	}
	
	for(i=0; i<MAX_CORE_PROBE_FX; i+=MAX_CORE_PROBE_FX/pp->core_num){
		core = pp->region_arg[pp->rid]->core_arg[i];
		if( test_bit(HFC_MB_PROL, (ulong *)&core->mb_status) )
		{
			hfc_wake_up(&pp->mb_event, &pp->mb_event_wait);
			break;
		}
	}
	if( test_bit(HFC_PS_DIAG, (ulong *)&pp->status) )
	{
		hfc_fx_wake_up(&pp->mb_event, &pp->mb_event_wait);
	}

	return;
}



/*
 * Function:    hfc_fx_mlpf_errlog_slpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_errlog_slpar(
	struct port_info        *pp,
	struct core_info        *core,
	uchar                   ssn,
	uchar                   son,
	uchar                   log_type)/* FCLNX-GPL-FX-391 */
{
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_errlog_slpar\n", pp->dev_minor);
	
	if ( ! hfc_fx_mlpf_check_state_port(pp, HFC_HG_HYPSTATUS_MCKLOG, HFC_CHECK_HYPER_STATE ) )
	{
		hfc_fx_mlpf_change_state_port(pp, HFC_HG_HYPSTATUS_MCKLOG,HFC_ENABLE_HYPER_STATE );
		
		hfc_fx_mlpf_indacc(pp, core, ssn, son, log_type);	/* FCLNX-GPL-FX-391 */
		
		hfc_fx_mlpf_change_state_port(pp, HFC_HG_HYPSTATUS_MCKLOG,HFC_DISABLE_HYPER_STATE );
	}

}


/*
 * Function:    hfc_fx_mlpf_errlog_glpar
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_errlog_glpar(
	struct port_info        *pp)
{
	struct hfc_err_rec		*err_rec;	/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	uchar                   data[16], wk=0, link_reset = 0, i = 0, available_core=0;	/* FCLNX-GPL-FX-077,461 */
	int                     err_no=0;
	int                     read_reg = 0, addr = 0;	/* FCLNX_GPL-472 *//* FCLNX-GPL-FX-077 */
//	struct core_info		*core = NULL;

	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_errlog_glpar\n", pp->dev_minor);
	
	for (i=0 ; i< MAX_CORE_PROBE_FX ; i += (MAX_CORE_PROBE_FX/pp->core_num)) {	/* FCLNX-GPL-FX-077 */
		if(pp->region_arg[pp->rid] != NULL){
			if(pp->region_arg[pp->rid]->core_arg[i] != NULL){
				if(!hfc_fx_check_cs_disable(pp, pp->region_arg[pp->rid]->core_arg[i])){	/* FCLNX-GPL-FX-438 */
					addr = 0x326 + 0x80*i;
					wk = hfc_fx_read_reg_ext(pp, ( uint )addr, ( char )0x1 );
					available_core++;	/* FCLNX-GPL-FX-461 */
					if(wk  == HFC_ABEND_LINK_RESET){
						link_reset = 1;
					}
				}
			}
		}
	}																			/* FCLNX-GPL-FX-077 */
	
	if((!link_reset)&&(available_core)){	/* FCLNX-GPL-FX-077,461 */
		hfc_fx_mlpf_indacc(pp, NULL, 0, 0, HFC_ERRLOG_TYPE_MCK);	/* FCLNX-GPL-FX-391 */
	
//		memset(&err_rec, 0, sizeof(struct hfc_fx_err_rec));			/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
//		HFC_MEMCPY( (uchar *)&err_rec, pp->mlpf_drv_log, 0x40);		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		err_rec = (struct hfc_err_rec *) pp->mlpf_drv_log;		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	
		HFC_4L_TO_4B(err_no, err_rec->log_area[0]);					/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		HFC_MEMCPY( data, &err_rec->log_area[48], 16);				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		
		HFC_DBGPRT("hfc_fx_mlpf_errlog_glpar - err_no=%d\n", err_no);
	
		read_reg = hfc_fx_read_reg_ext( pp,(uint)0, (char)0x4) ;		/* FCLNX_GPL-472 */

		if( ( err_no == 0x00000035 ) ||
			( err_no == 0x00000036 ) ||
			( err_no == 0x00000071 ) ) /* FCLNX-GPL-423 */
		{
			hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRF, err_no, data,16) ;
		}
		else if ( ( err_no == 0x00000077 ) ||
			( err_no == 0x00000078 ) )
		{
			hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_IMLLOG, ERRID_HFCP_ERRC, err_no, data,16) ;
		}
		else if ( err_no == 0x00000031 ) /* FCLNX-GPL-423 */
		{
			hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1, err_no,data,16) ;
		}
		else if( err_no == 0x0000002b )	/* FCLNX-GPL-423 */
		{
			hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR2, err_no, data,16) ;
		}
		else if( err_no == 0x00000000 )	/* FCLNX-GPL-472 */
		{
			if( read_reg == 0xffffffff )
			{
				hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_ERR1,
														0x31,data,16) ;
			}
		}								/* FCLNX-GPL-472 */
		else if( err_no == 0x0000002e )
		{
			hfc_fx_errlog(
				pp, NULL ,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT2,
				err_no,pp->logdata,16) ;
		}	/* FCLNX-GPL-FX-406 */
		else{
			hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_MCK,ERRID_HFCP_ERR4, err_no,data,16) ;
		}
	}else if(!available_core){	/* FCLNX-GPL-FX-461 */
		hfc_fx_mlpf_indacc(pp, NULL, 0, 0, HFC_ERRLOG_TYPE_MCK);	/* FCLNX-GPL-FX-391 */
		
		err_rec = (struct hfc_err_rec *) pp->mlpf_drv_log;		/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
	
		err_no = 0x00000031;										/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		HFC_MEMCPY( data, &err_rec->log_area[48], 16);				/* FCLNX_GPL-0151 *//* FCLNX_GPL-147 */
		
		HFC_DBGPRT("hfc_fx_mlpf_errlog_glpar - err_no=%d\n", err_no);
	
		read_reg = hfc_fx_read_reg_ext( pp,(uint)0, (char)0x4) ;		/* FCLNX_GPL-472 */
		
		hfc_fx_errlog(
				pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_CHKSTP,ERRID_HFCP_ERR1, err_no,data,16) ;
		
	}else if( test_bit( HFC_PD_LINK_RESET, (ulong *)&pp->status_detail2 ) ){
		hfc_fx_errlog(
			pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT2,
			0x0000002e, pp->logdata,16) ;	/* FCLNX-GPL-391 */
	}
	
}


/*
 * Function:    hfc_fx_mlpf_indacc
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
uint hfc_fx_mlpf_indacc(
	struct port_info        *pp,
	struct core_info        *core,
	uchar                   ssn,
	uchar                   son,
	uchar                   log_type)/* FCLNX-GPL-FX-391 */
{
	uint                    flag;
	uchar                   *log_area;
	uint					reg_addr=0;		/* FCLNX-GPL-262 */
	uint					type=0;			/* FCLNX-GPL-262 */
	uint					size=0;			/* FCLNX-GPL-FX-391 */
	uchar					log_detail=0, log_size=0;	/* FCLNG-GPL-FX-391 */
	
	if(HFC_FX_MMODE_CHECK_SHADOW(pp) && (( log_type == HFC_ERRLOG_TYPE_MBINT )||(log_type == HFC_ERRLOG_TYPE_LINKINCLOG))){	/* FCLNX-GPL-FX-391 Start */
		log_area = (uchar *)(core->slog_addr[ssn]+( HFC_SLOG_LEN * son)) ;
		size = HFC_SLOG_LEN*2;
		log_detail = HFC_MLPF_INDACC_LINKDOWN;
		log_size = size/1024;
	}else{
		log_area = (uchar *)pp->hw_log;
		size = HFC_FX_HWLOG_SIZE;
		if(HFC_FX_MMODE_CHECK_SHADOW(pp)){ 
			log_size = size/1024;
			if( log_type == HFC_ERRLOG_TYPE_MCK ){
				log_detail = HFC_MLPF_INDACC_MCKLOG;
			}else if(log_type == HFC_ERRLOG_TYPE_CHKSTP){
				log_detail = HFC_MLPF_INDACC_CSTPLOG;
			}else{
				log_detail = HFC_MLPF_INDACC_IMLFAIL;
			}
		}
	}	/* FCLNX-GPL-FX-391 End */
	HFC_DBGPRT("hfc_fx_mlpf_indacc - log_detail=%08x, size=%08x\n",log_detail,log_size);
	
	flag = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_INDACC0, 4);
	if ( flag & HFC_MLPF_INDACC_FLG )
	{
		return HFC_MLPF_INDACC_DISABLE;
	}
	
	if(HFC_FX_MMODE_CHECK_SHADOW(pp)){
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_INDACTYPE, 1, log_detail);	/* FCLNX-GPL-FX-391 */
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_INDACSIZE, 1, log_size);	/* FCLNX-GPL-FX-391 */
	}
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_SET_INDACC);
	
	/* It is repeated Read as for the right or wrong of INDACC use flag */
	flag = (uint) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_INDACC0, 0x4);
	if ( !(flag & HFC_MLPF_INDACC_FLG) )
	{
		return HFC_MLPF_INDACC_USED;
	}
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_INDACC3, 4, 0x100);
	type = hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_INDAREA, 4);
	if ( HFC_FX_MMODE_CHECK_SHARED(pp) && ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
		memset(pp->hw_log, 0, HFC_FX_HWLOG_SIZE);

	for ( reg_addr=0; reg_addr<size; reg_addr+=0x80)		/* FCLNX-GPL-262 */
	{
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_INDACC3, 4, reg_addr);
		
		if ( HFC_FX_MMODE_CHECK_SHADOW (pp) )
		{
			if ( reg_addr == 0 )
				hfc_fx_mlpf_set_mmio_hg(pp, pp->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
			else if ( reg_addr >= 0x400 )
				hfc_fx_mlpf_set_mmio_hg(pp, (uchar *)&log_area[(reg_addr-0x400)], HFC_IOHGSPC_INDAREA, 0x80);
		}
		else if ( HFC_FX_MMODE_CHECK_SHARED(pp) && ( !HFC_FX_MMODE_CHECK_SHADOW(pp) ) )
		{
			if( type != 0x00000000 )
			{
				if ( reg_addr == 0 )
				{
					memset(pp->mlpf_drv_log, 0, 0x40);
					hfc_fx_mlpf_get_mmio_hg(pp, pp->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
				}

				hfc_fx_mlpf_get_mmio_hg(pp, (uchar *)&log_area[reg_addr], HFC_IOHGSPC_INDAREA, 0x80);
			}
			else {
				if ( reg_addr == 0 )
				{
					memset(pp->mlpf_drv_log, 0, 0x40);
					hfc_fx_mlpf_get_mmio_hg(pp, pp->mlpf_drv_log, HFC_IOHGSPC_INDAREA, 0x40 );
				}
				else if ( reg_addr >= 0x400 )
				{
					hfc_fx_mlpf_get_mmio_hg(pp, (uchar *)&log_area[(reg_addr-0x400)], HFC_IOHGSPC_INDAREA, 0x80);
				}
			}
		}
	}																/* FCLNX-GPL-262 */
	
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_CMNDREG, 4, HFC_MLPF_RESET_INDACC);
	
	return HFC_MLPF_INDACC_SUCCESS;
}


/* FCLNX-GPL-393 */
/*
 * Function:    hfc_fx_mlpf_set_errorlimit
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_set_errorlimit(
	struct port_info            *pp)
{
	// Guest LPAR only
	int	i;
	short	max_ld_err_count_s=0, min_ld_remain_count_s=0;
	struct	target_info_fx		*target;
	
	/*  Write Error Limit for MMIO-HG */
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LDS_LIMIT,  2, pp->ld_err_limit_s);
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_FCIF_LIMIT, 2, pp->if_err_limit);
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_TO_LIMIT,   2, pp->to_err_limit);
	hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_RST_LIMIT,  2, pp->rt_err_enable);
	
	/*  Write Error Count for MMIO-HG */
	if (!(hfc_manage_info.hfcldd_mp_mod)) {
		max_ld_err_count_s = pp->ld_err_count_s;
		for(i=0; i<(pp->max_target); i++){
			target = pp->target_arg[i];
			if( target != NULL){
				if(max_ld_err_count_s < target->tgt_ld_err_count_s){
					max_ld_err_count_s = target->tgt_ld_err_count_s;
				}
			}
		}
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LDS_COUNT,  2, max_ld_err_count_s);
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_FCIF_COUNT, 2, pp->if_err_count);
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_TO_COUNT,   2, pp->to_err_count);
	}else{
		max_ld_err_count_s =	pp->ld_err_limit_s;
		if (pp->lds_errcnt_info != NULL ) {
			min_ld_remain_count_s = pp->lds_errcnt_info->remain_count;
		}
		for(i=0; i<(pp->max_target); i++){
			target = hfc_fx_hash_target_valid(pp, i);
			if( target != NULL){
				if (target->tgt_lds_errcnt_info != NULL ) {
					if(target->tgt_lds_errcnt_info->remain_count < min_ld_remain_count_s){
						min_ld_remain_count_s = target->tgt_lds_errcnt_info->remain_count;
					}
				}
			}
		}
		max_ld_err_count_s -= min_ld_remain_count_s;
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_LDS_COUNT,  2, max_ld_err_count_s);
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_FCIF_COUNT, 2, ((pp->if_errcnt_info) ? (pp->if_err_limit - pp->if_errcnt_info->remain_count) : 0 ));
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_TO_COUNT, 2, ((pp->to_errcnt_info) ? (pp->to_err_limit - pp->to_errcnt_info->remain_count) : 0));
	}


	if(pp->c_err == HFC_ISOLATE_RT){
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_RST_COUNT,  2, 1);
	}
	else{
		hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_RST_COUNT,  2, 0);
	}

	return;
}


/* FCLNX-GPL-399 */
/*
 * Function:    hfc_fx_mlpf_set_led
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_set_led(
	struct port_info			 *pp,
	uint64_t						data)
{	
	int i;
	
	for(i = 0; i < 4; i++){
		hfc_fx_write_reg_ext(pp, (pp->pkg.map->iosp.reg[(uint)HFC_IOSPACE_CMDLED]+i), (char)0x1, (((uint)(data) >> (8*(3-i))) & 0x000000ff) );
	}

}

/* FCLNX-GPL-399 */
/*
 * Function:    hfc_fx_mlpf_set_fcif
 *
 * Purpose:     
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_set_fcif(
	struct port_info			 *pp,
	uint64_t						data)
{	
	int i;

	for(i = 0; i < 4; i++){
		hfc_fx_write_reg_ext(pp, (pp->pkg.map->iosp.reg[(uint)HFC_IOSPACE_CMDFCIF]+i), (char)0x1, (((uint)(data) >> (8*(3-i))) & 0x000000ff) );
	}

}

/* FCLNX-GPL-489 */
/*
 * Function:    hfc_fx_mlpf_migration_end
 *
 * Purpose:     Set Hardware Status after LPAR Migration
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_migration_end(
	struct port_info    *pp,
	struct core_info	*core)
{
	uint logdata[4];
//	uchar wkc = 0;
	uchar buf[16];
	uint wkint1 = 0;
//	uint wkint2 = 0;
	
	if(pp == NULL)return;
	
	memset(logdata, 0x00, sizeof(logdata));
	memset(buf, 0x00, sizeof(buf));
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x00, pp, NULL, NULL, 
		NULL, NULL, 0, 0, 0);

	/* Set adpp_id */
	hfc_fx_mlpf_get_mmio_hg(pp, buf, HFC_IOHGSPC_ADAPID0, HFC_IOHGSPC_ADAPID_LEN);

	/* Set WWPN */
	hfc_fx_search_adapter_number(pp);

	/* Set HBA F/W, HVM F/W support bit */
	if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_LPAR_ISOLATION_SUPPORT, HFC_CHECK_HVM_SUPPORT )) /* FCLNX-GPL-551 */
		set_bit(HFC_SUPPORT_HVM_ISOL, (ulong *)&pp->fw_support);
	if(hfc_fx_mlpf_check_state_port(pp, HFC_HG_LPRSTATUS_ISOLSUPPRT, HFC_CHECK_LPAR_STATE )) /* FCLNX-GPL-551 */
		set_bit(HFC_SUPPORT_FW_ISOL, (ulong *)&pp->fw_support);
	
	if(!( HFC_FX_MMODE_CHECK_SHADOW(pp) ))
		hfc_fx_mlpf_set_errorlimit(pp);
	
	pp->pkg.lsi_rev = (uchar)hfc_fx_read_reg(pp, HFC_IOSPACE_LSIREV, 0x01);

	/* Set vpd support bit */
	hfc_fx_copy_iocinfo(pp, core); 
	
	/* counter clear */
	core->core_ce_cnt = 0;
	pp->pcie_sram_ce_cnt = 0;
	if ( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ){
		hfc_manage_info.npubp->hfc_fx_clear_errinfo(pp);
	}
	else{
		hfc_fx_clear_errinfo_i(pp);
	}
	
	/* check for conflicting data*/
	/* check RID */
	wkint1 = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_RID, 0x4 );
	if (pp->rid != wkint1) {
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x01, pp, NULL, NULL, 
			NULL, NULL, pp->rid, wkint1, 0);
		
		pp->rid = wkint1; /* FCLNX-GPL-549 */
	}

#if 0
	/* check core no */
	wkc = (uchar)hfc_fx_read_reg(pp, HFC_IOSPACE_CHNO, 0x1);
	if (pp->pkg.core_no != wkc) {
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x02, pp, NULL, NULL, 
			NULL, NULL, pp->pkg.core_no, wkc, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000002;
		HFC_4L_TO_4B(logdata[0], wkint1);
		logdata[1] = pp->pkg.core_no;
		logdata[2] = wkc;
	}

	/* check xob no */
	wkint1 = pp->fw_init_p->xob_inp; /* xob no */
	HFC_4B_TO_4L(wkint2, wkint1);
	wkint1 = ((wkint2 & 0x00ff0000)>>16) * HFC_XOB_PER_PAGE ;
	wkint1 += (wkint2 & 0x0000ffff) ;
	
	if (pp->xob_no != wkint1) {
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x03, pp, NULL, NULL, pp->xob_no, wkint2, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000003;
		HFC_4L_TO_4B(logdata[0], wkint1);
		wkint2 = pp->xob_no;
		HFC_4L_TO_4B(logdata[1], wkint2);
		logdata[2] = pp->fw_init_p->xob_inp;
	}
	
	/* check xrb no */
	wkint1 = pp->fw_init_p->xrb_outp; /* xrb no */
	HFC_4B_TO_4L(wkint2, wkint1);
	wkint1 = ((wkint2 & 0x00ff0000)>>16) * HFC_XRB_PER_PAGE ;
	wkint1 += (wkint2 & 0x0000ffff) ;

	if (pp->xrb_no != wkint1) {
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x04, pp, NULL, NULL, pp->xrb_no, wkint2, 0);
		
		memset(logdata, 0x00, sizeof(logdata));
		wkint1 = 0x00000004;
		HFC_4L_TO_4B(logdata[0], wkint1);
		wkint2 = pp->xrb_no;
		HFC_4L_TO_4B(logdata[1], wkint2);
		logdata[2] = pp->fw_init_p->xrb_outp;
	}
#endif

	HFC_4L_TO_4B(logdata[3], wkint1); /* FCLNX-GPL-504 */
	
	HFC_INFPRT("hfcldd%d : Adppter was changed into the other one by Live Migration.(HBA WWPN=%llx result=0x%08x%08x%08x%08x)\n", pp->dev_minor, (unsigned long long)pp->org_ww_name, logdata[0],logdata[1],logdata[2],logdata[3]); /* FCLNX-GPL-520 */
	
#ifdef HFC_HVM_DEBUG /* FCLNX-GPL-520 */	
	/* Errlog 0xAF*/
	hfc_fx_errlog(pp,NULL,NULL,NULL,HFC_ERRLOG_TYPE_NONE,ERRID_HFCP_EVNT4, 0xAF, (uchar *)&logdata, sizeof(logdata)) ;
#endif /* FCLNX-GPL-520 */
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x10, pp, NULL, NULL, 
		NULL, NULL, wkint1, 0, 0); /* FCLNX-GPL-504 */

}

/* FCLNX-GPL-489 */
/*
 * Function:    hfc_fx_mlpf_migration_recovery
 *
 * Purpose:     Recovery process after LPAR Migration
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */
void hfc_fx_mlpf_migration_recovery(
	struct port_info    *pp,
	struct core_info	*core,
	uint				hyp_status)
{
	uint lpar_sts = (uint)hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_LPARSTATUS, 0x4);
	uint sfp_err = (lpar_sts & 0x00000f00);
	uint hyp_condition = hfc_fx_mlpf_check_hypcondition(hyp_status);
	
	hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x20, pp, NULL, NULL, 
		NULL, NULL, hyp_status, lpar_sts, sfp_err);
	
	if ( hyp_condition == HFC_HYPCONDITION_NORMAL )
	{
		/* CHK-STOP or Isolate -> Normal(LinkUp or LinkDown) */
		if ( !sfp_err && test_bit(HFC_PS_ISOL, (ulong *)&pp->status) )
		{
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x21, pp, NULL, NULL, 
				NULL, NULL, pp->status_detail2, 0, 0);
			set_bit(HFC_ATTACH, (ulong *)&pp->attach_status);
			set_bit( HFC_PS_ISOL, (ulong *)&pp->status );	/* To recovery isolation */
			hfc_fx_mlpf_isol_recovery_end_glpar(pp, core, hyp_status, 1);
		}
		/* LinkDown -> LinkDown */
		else if (lpar_sts & HFC_HG_LPRSTATUS_LINKDOWN) {
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x22, pp, NULL, NULL, 
				NULL, NULL, pp->status_detail2, 0, 0);
			
			switch (sfp_err) {
			case HFC_HG_LPRSTATUS_SFP_FAIL:
				set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
				set_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2);
				break;
			case HFC_HG_LPRSTATUS_SFP_NOTSUPT:
				set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
				set_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->status_detail2);
				break;
			case HFC_HG_LPRSTATUS_SFP_DOWN:
				set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
				set_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->status_detail2);
				break;
			default:
				clear_bit(HFC_PS_ISOL, (ulong *)&pp->status);
				clear_bit(HFC_PD_ISOLATE_SFPFAIL, (ulong *)&pp->status_detail2);
				clear_bit(HFC_PD_ISOLATE_SFPNOTSUPPORT, (ulong *)&pp->status_detail2);
				clear_bit(HFC_PD_ISOLATE_SFPDOWN, (ulong *)&pp->status_detail2);
				break;
			}
			hfc_fx_linkdown_intreq(pp, core, 1);
		}
		else { /* LinkDown -> LinkUp or LinkUp -> LinkUp */
			hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x23, pp, NULL, NULL, 
				NULL, NULL, pp->status_detail2, 0, 0);
			
			hfc_fx_change_portstat_linkup(pp, core);
			clear_bit(HFC_PS_WAIT_LINKUP, (ulong *)&pp->status);
			clear_bit(HFC_PS_WAIT_INITIALIZE, (ulong *)&pp->status);	/* FCLNX-GPL-FX-005 */
			clear_bit(HFC_PS_ISOL, (ulong *)&pp->status);
#if defined(HFC_X8664_SLES11SP3) || defined(HFC_RHEL7) || defined(HFC_X8664_SLES12) || defined(HFC_X8664_OEL6) || defined(HFC_X8664_OEL7)
			if ( !( hfc_manage_info.hfcldd_mp_mod && hfc_manage_info.npubp->hfc_fx_check_mp_enable() ) ){ /* FCLNX-GPL-FX-472 */
				clear_bit(HFC_PD_WAIT_ISOL_LINKUP_CNT, (ulong *)&pp->status_detail1);
				hfc_fx_w_stop( pp, core, HFC_FX_WLINKUP_CNT_TMR );
			}
#endif	/* FCLNX-GPL-FX-424 */
			HFC_DETAIL_CLEAR_ISOLREC(pp);
			hfc_fx_w_stop( pp, core, HFC_FX_LINKUP_TMR );
			
			/* Search Target. Issue GID_FT. */
			set_bit(HFC_PD_NEED_GPNFT, (ulong *)&pp->status_detail2);
			atomic_set(&pp->check_mbreq, 1);
//			start_fx_next_mailbox( pp, NULL ); 
		}
	}
	else if ( hyp_condition == HFC_HYPCONDITION_CSTP ) /* -> CHK-STOP */
	{
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x24, pp, NULL, NULL, 
			NULL, NULL, pp->status_detail2, 0, 0);
			
		set_bit(HFC_PD_ISOLATE_CHKSTP, (ulong *)&pp->status_detail2);
		set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
		
		/* CHK-STOP *?CHK-STOP Nop */
		/* LinkDown *?CHK-STOP or Isolation is disallowed by Hyper. */
	}
	else if ( hyp_condition == HFC_HYPCONDITION_ISOL ) /* -> Isolation */
	{
		hfc_fx_hand2_trace(HFC_FX_TRC_MLPF_MIGRATION, 0x26, pp, NULL, NULL, 
			NULL, NULL, pp->status_detail2, 0, 0);

		if( test_bit(HFC_PS_ISOL, (ulong *)&pp->status) ){
			set_bit(HFC_ATTACH, (ulong *)&pp->attach_status);
		}
		set_bit(HFC_PS_ISOL, (ulong *)&pp->status);
		if (hyp_status & HFC_HG_HYPSTATUS_ISOLCMD) {
			set_bit( HFC_PD_ISOLATE_PORT_C, (ulong *)&pp->status_detail2 );
		} else {
			set_bit( HFC_PD_ISOLATE_PORT_E, (ulong *)&pp->status_detail2 );
		}
	}
	else{
		if (pp->debug_func & HFC_DEBUG_HYP_INT_CHK) {
			hfc_fx_errlog(pp, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_EVNT4, 0xb1, NULL, 0) ;
		}
	}

}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_fx_alloc_mlpf_cca
 *
 * Purpose:     Allcate uncached memory area shared hypervisor.
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */

int hfc_fx_alloc_mlpf_cca(
	struct port_info    *pp)
{
	dma_addr_t			bus_addr;
	void				*vir_addr;
	int					size;
	struct pci_dev		*pdev = NULL;
	
	HFC_DBGPRT( "  hfcldd : hfc_fx_alloc_mlpf_cca - Start \n");
	
	pdev = pp->pci_cfginf;
	size = sizeof(struct hg_cca_fx);	/* FCLNX-GPL-FX-433 */

	vir_addr = hfc_fx_dma_alloc_coherent(pp, &pdev->dev, (uint)size*MAX_CORE_PROBE_FX, &bus_addr,GFP_ATOMIC);	/* FCLNX-GPL-FX-433 */

	pp->hg_cca_p = (struct hg_cca_fx *)vir_addr;	/* FCLNX-GPL-FX-433 */
	if (pp->hg_cca_p == NULL) { 	/* failed? */
 		HFC_DBGPRT(" hfcldd : hfc_fx_alloc_mlpf_cca  - exit with error (alloc_error_exit) \n");
		hfc_fx_errlog(NULL, NULL, NULL, NULL, HFC_ERRLOG_TYPE_NONE, ERRID_HFCP_ERR9, 0xD0, NULL, 0) ;
		hfc_fx_free_mlpf_cca(pp);
		HFC_EXIT("hfc_fx_alloc_mlpf_cca  (error) ");
		return(1);
	}                                                                                         

	HFC_DBGPRT("  (hg_cca) logical=%lx, physical=%lx, size = %d \n",
			(ulong)pp->hg_cca_p, (ulong)bus_addr, size);
			
	memset((char *)pp->hg_cca_p, 0, (int)size);
	pp->padr_hg_cca = bus_addr;

	return (0);

}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_fx_free_mlpf_cca
 *
 * Purpose:     free uncached memory area shared hypervisor.
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */

void hfc_fx_free_mlpf_cca(
	struct port_info    *pp)
{
	struct pci_dev		*pdev = NULL;
	
	pdev = pp->pci_cfginf;

	if ( pp->hg_cca_p != NULL ) {
		/* Release mlpf cca (128 Bytes) */
		HFC_DBGPRT(" hfcldd : hfc_fx_free_mlpf_cca - free mlpf cca \n");
		memset((char *)pp->hg_cca_p, 0, (int)sizeof(struct hg_cca_fx)*MAX_CORE_PROBE_FX);	/* FCLNX-GPL-FX-433 */
		
		hfc_fx_dma_free_coherent(pp, &pdev->dev, (size_t)sizeof(struct hg_cca_fx)*MAX_CORE_PROBE_FX,(void *)pp->hg_cca_p, (dma_addr_t)pp->padr_hg_cca);	/* FCLNX-GPL-FX-433 */
		
		pp->hg_cca_p = NULL;
	}

	return;
}

/* FCLNX-GPL-494 */
/*
 * Function:    hfc_fx_mlpf_cca_setup
 *
 * Purpose:     Setup mlpf_cca
 *
 * Arguments:   
 *  pp         - pointer to adpp_info
 *
 * Returns:     
 *
 * Notes:       
 */

#define HFC_HG_CCA_PHYADR_VALID		0x80

void hfc_fx_mlpf_cca_setup(
	struct port_info    *pp)
{
	uchar flag = 0, i=0;	/* FCLNX-GPL-FX-433 */
	unsigned long long		wkp;
	int						dma_size;
	
	HFC_DBGPRT("hfcldd%d : hfc_fx_mlpf_cca_setup\n", pp->dev_minor);
	
	if ( (HFC_FX_MMODE_CHECK_BASIC(pp)) || (pp->hg_cca_p == NULL) ) {
		return;
	}
	
	for(i=0;i<MAX_CORE_PROBE_FX;i+=MAX_CORE_PROBE_FX/pp->core_num){	/* FCLNX-GPL-FX-433 */
		pp->hg_cca_p[i].version = 0x02;
		
		pp->hg_cca_p[i].valid |= HFC_FWSTATISTICS_VALID;
		
		pp->hg_cca_p[i].size = sizeof(struct hg_cca_fx);
		
		pp->hg_cca_p[i].uni_cnt = MAX_CORE_PROBE_FX;
		
		pp->hg_cca_p[i].rid = pp->rid;
			
		pp->hg_cca_p[i].cnum = i;
		
		flag = (uchar) hfc_fx_read_hg_reg(pp, HFC_IOHGSPC_HGCCA_FLAG, 0x1);
		
		if (!(flag & HFC_HG_CCA_PHYADR_VALID)) { /* Not updating after setting pyscal address. */
			dma_size = sizeof(dma_addr_t);
			wkp = pp->padr_hg_cca;									/* FCLNX-GPL-524 */
			if(dma_size == 8){
				wkp >>=32;
			}
			else{
				wkp = 0;
			}
			
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HGCCA_FLAG,  1, 0x00); /* Clear flag set by EFI driver. */
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HGCCA_ADDR1, 4, pp->padr_hg_cca );
			hfc_fx_write_hg_reg(pp, HFC_IOHGSPC_HGCCA_ADDR0, 4, wkp);
			/* Set flag after setting psycal address. */
			hfc_fx_write_hg_reg(pp,  HFC_IOHGSPC_HGCCA_FLAG,  1, HFC_HG_CCA_PHYADR_VALID);
		}
	}
}

