/* hfcl_hand_timer_trace_fx.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcl_hand_timer_trace_fx.h,v 1.1.2.10.2.2.2.3.2.1 2015/03/05 02:19:40 toyo Exp $
 */

/*--- TRACE ID ---------------------*/

#define HFC_FX_TRC_HANDLER				0x91	/* HAND_TRC Format1 */
#define HFC_FX_TRC_MBRESP          		0x92	/* HAND_TRC Format2 */
#define HFC_FX_TRC_MBINT          		0x93	/* HAND_TRC Format2 */
#define HFC_FX_TRC_XRBRSP         		0x94	/* HAND_TRC Format3 */
#define HFC_FX_TRC_SCSI_CHK				0x95	/* HAND_TRC Format4 */
#define HFC_FX_TRC_MGM_CHK				0x96	/* HAND_TRC Format4 */
#define HFC_FX_TRC_LINK_CHK				0x97	/* HAND_TRC Format4 */
#define HFC_FX_TRC_CHKSTP				0x98	/* HAND_TRC Format6 */
#define HFC_FX_TRC_DEQ_WE				0x99	/* HAND_TRC Format4 */
#define HFC_FX_TRC_LINKRSP				0x9a	/* HAND_TRC Format2 */
#define HFC_FX_TRC_LGINRSP				0x9b	/* HAND_TRC Format2 */
#define HFC_FX_TRC_ABEND				0x9c	/* HAND_TRC Format6 */
#define HFC_FX_TRC_PDISCRSP				0x9d	/* HAND_TRC Format2 */
#define HFC_FX_TRC_MCKREC				0x9e	/* HAND_TRC Format6 */
#define HFC_FX_TRC_GIDFTRSP				0x9f	/* HAND_TRC Format2 */
#define HFC_FX_TRC_MIHLGRSP				0xa1	/* HAND_TRC Format2 */
#define HFC_FX_TRC_HWERR				0xa2	/* HAND_TRC Format6 */
#define HFC_FX_TRC_WDOG					0xa5	/* HAND_TRC Format5 */
#define HFC_FX_TRC_GPNIDRSP				0xa6	/* HAND_TRC Format2 */
#define HFC_FX_TRC_LINKDOWN_INT			0xa7	/* HAND_TRC Format2 */
#define HFC_FX_TRC_LINKUP_INT			0xa8	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PLOGI_INT			0xa9	/* HAND_TRC Format2 */
#define HFC_FX_TRC_LOGO_INT				0xaa	/* HAND_TRC Format2 */
#define HFC_FX_TRC_SCN_INT				0xab	/* HAND_TRC Format2 */
#define HFC_FX_TRC_RSCN_INT				0xac	/* HAND_TRC Format2 */
#define HFC_FX_TRC_GIDPNRSP				0xad	/* HAND_TRC Format2 */
#define HFC_FX_TRC_DIAGRSP				0xae    /* HAND_TRC Format2 *//* FCLNX-GPL-FX-126 */
#define HFC_FX_TRC_FORCE_ISOL			0xb0	/* HAND_TRC Format7 */
#define HFC_FX_TRC_FORCE_ISOL_REC		0xb1	/* HAND_TRC Format7 */
#define HFC_FX_TRC_FORCE_ISOL_REC_P		0xb2	/* HAND_TRC Format7 */

#define HFC_FX_TRC_ADD_PORTIDRSP		0xb3	/* HAND_TRC Format2 */
#define HFC_FX_TRC_DEL_PORTIDRSP		0xb4	/* HAND_TRC Format2 */
#define HFC_FX_TRC_LDCH_TRCLOGRSP		0xb5	/* HAND_TRC Format2 */
#define HFC_FX_TRC_CSCSIRSP				0xb6	/* HAND_TRC Format2 */
#define HFC_FX_TRC_CORESTARTRSP			0xb7	/* HAND_TRC Format2 */
#define HFC_FX_TRC_FLOGIRSP				0xb8	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PLOGIRSP				0xb9	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PRLIRSP				0xba	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PRLORSP				0xbb	/* HAND_TRC Format2 */
#define HFC_FX_TRC_SCRRSP				0xbc	/* HAND_TRC Format2 */
#define HFC_FX_TRC_GCSIDRSP				0xbd	/* HAND_TRC Format2 */
#define HFC_FX_TRC_RFTIDRSP				0xbe	/* HAND_TRC Format2 */
#define HFC_FX_TRC_RFFIDRSP				0xbf	/* HAND_TRC Format2 */

#define HFC_FX_TRC_MLPF_INT				0xC0	/* HAND_TRC Format8 */
#define HFC_FX_TRC_MLPF_HWERR_INT		0xC1	/* HAND_TRC Format8 */
#define HFC_FX_TRC_MLPF_FORCE_ISOL		0xC2	/* HAND_TRC Format8 */
#define HFC_FX_TRC_MLPF_RECV_ISOL		0xC3	/* HAND_TRC Format8 */
#define HFC_FX_TRC_MLPF_HWERR_INT_DET	0xC4	/* HAND_TRC Format8 */
#define HFC_FX_TRC_MLPF_MIGRATION		0xC5	/* HAND_TRC Format8 */

#define HFC_FX_TRC_LOGORSP				0xc6	/* HAND_TRC Format2 */
#define HFC_FX_TRC_FLOGI_INT			0xc7	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PDISC_INT			0xc8	/* HAND_TRC Format2 */
#define HFC_FX_TRC_RFRAME_INT			0xc9	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PRLI_INT				0xca	/* HAND_TRC Format2 */
#define HFC_FX_TRC_PRLO_INT				0xcb	/* HAND_TRC Format2 */
#define HFC_FX_TRC_GPNFTRSP				0xcc	/* HAND_TRC Format2 */
#define HFC_FX_TRC_SHADOWUPRSP			0xcd	/* HAND_TRC Format2 */
#define HFC_FX_TRC_OFFLINEMBRSP			0xce	/* HAND_TRC Format2 */
#define HFC_FX_TRC_INTL_START           0xcf    /* HAND_TRC Format2 */

#define HFC_FX_TRC_CHECK_ERRCOUNT		0xD2	/* HAND_TRC Format7 */

/*--- hfc_reset_start() parm(type)  ----*/
#define HFC_CTLRST              0x01
#define HFC_REBOOT              0x02
//#define HFC_F_START             0x03
#define HFC_FW_START            0x04
#define HFC_INI_RESET           0x05
#define HFC_SET_INIADR          0x06
#define HFC_SET_WS80            0x07
#define HFC_SET_WS40            0x08
#define HFC_WSCA_CLEAR          0x09
#define HFC_SET_WS04            0x0a
#define HFC_GR_CLEAR            0x0b    /* FCLNX-GPL-220 */
#define HFC_TSEQ	            0x0c
#define HFC_SET_ISOLATE_CORE	0x0d	/* FCLNX-GPL-FX-079 */
#define HFC_UTL_REG_CLEAR		0x0e	/* FCLNX-GPL-FX-079 */
#define HFC_RECEIVE_CTL_FLAG_CLEAR	0x0f	/* FCLNX-GPL-FX-079 */

#define HFC_SET_MLPF_MODE       0x10    /* FCLNX-0379 */

#define HFC_RESET_ALL_INT		0x11	/* FCLNX-GPL-FX-272 */

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
#define TYPE_TRC4               0xeb
#define TYPE_TRC5               0xea
#define TYPE_TRC6               0xe9
#define TYPE_TRC7               0xe8
#define TYPE_TRC8               0xe7
#define TYPE_TRC9               0xe6
#define TYPE_TRCA               0xe5
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
#define TYPE_IERR               0x09
#define TYPE_CORSV              0x0a
/* #define TYPE_ROTATE             0x10 */
#define TYPE_ERRID				0x11	/* FIVE-EX */
#define TYPE_RSV                0x7f    /* FCWIN-0220 END*/
/*------------------*/
#define REG_RAMMSK              0x02f8
#define REG_RAMADR              0x02fc	
#define REG_INDAREA             0x0500

#define REG_INDFLASH            0x0500
#define REG_INDAREA_FIVE        0x0600  /* FCWIN-234 */

/*==============================================================================
 *    PCI Config
 *============================================================================*/
#define HFCFX_PCI(REG)			HFCFX_PCI_##REG
#define HFCFX_PCI_OFF						/*  Disappearance HFCX_MMIO()     */
/*==== PCI COnfig Header =====================================================*/
#define HFCFX_PCI_HEAD				0x0000	/* Top Address of Config Header   */
#define HFCFX_PCI_VENDERID			0x0000	/* Vender ID                      */
#define HFCFX_PCI_DEVICEID			0x0002	/* Device ID                      */
#define HFCFX_PCI_COMMAND			0x0004	/* Command                        */
#define HFCFX_PCI_STATUS			0x0006	/* Status                         */
#define HFCFX_PCI_REVISION			0x0008	/* Revision ID                    */
#define HFCFX_PCI_CLASSCODE			0x0009	/* Class Code                     */
#define HFCFX_PCI_CHASELINE			0x000C	/* Chase Line Size                */
#define HFCFX_PCI_LATENCY			0x000D	/* Latency Timer                  */
#define HFCFX_PCI_TYPE				0x000E	/* Header Type                    */
#define HFCFX_PCI_BIST				0x000F	/* BIST                           */
#define HFCFX_PCI_BAR0				0x0010	/* Base Address#0                 */
#define HFCFX_PCI_BAR0L				0x0010	/* Base Address#0 (Lower)         */
#define HFCFX_PCI_BAR0H				0x0014	/* Base Address#0 (Upper)         */
#define HFCFX_PCI_BAR1				0x0018	/* Base Address#1                 */
#define HFCFX_PCI_BAR1L				0x0018	/* Base Address#1 (Lower)         */
#define HFCFX_PCI_BAR1H				0x001C	/* Base Address#1 (Upper)         */
#define HFCFX_PCI_BAR2				0x0020	/* Base Address#2                 */
#define HFCFX_PCI_BAR2L				0x0020	/* Base Address#2 (Lower)         */
#define HFCFX_PCI_BAR2H				0x0024	/* Base Address#2 (Upper)         */
#define HFCFX_PCI_CARDBUS			0x0028	/* Cardbus CIS Pointer            */
#define HFCFX_PCI_SUBVENDER			0x002C	/* Subsystem Vender ID            */
#define HFCFX_PCI_SUBSYSTEM			0x002E	/* Subsystem ID                   */
#define HFCFX_PCI_EEPROM			0x0030	/* Expantion ROM Address          */
#define HFCFX_PCI_CAPBILITY			0x0034	/* Capabilities Pointer           */
#define HFCFX_PCI_INTLINE			0x003C	/* Interrupt Line                 */
#define HFCFX_PCI_INTPIN			0x003D	/* Interrupt PIN                  */
#define HFCFX_PCI_MINGNT			0x003E	/* Max GNT                        */
#define HFCFX_PCI_MAXLAT			0x003F	/* Min LAT                        */
#define HFCFX_PCI_CAP(C)			HFCFX_PCI_CAP_##C
/*==== PM Capability =========================================================*/
#define HFCFX_PCI_CAP_PM			0x0040	/* Top Address of PM Capability   */
/*==== MSI Capability ========================================================*/
#define HFCFX_PCI_CAP_MSI			0x0048	/* Top Address of MSI Capability  */
/*==== MSIX Capability =======================================================*/
#define HFCFX_PCI_CAP_MSIX			0x0060	/* Top Address of MSIX Capability */
/*==== PCIe Capability =======================================================*/
#define HFCFX_PCI_CAP_PCIE			0x006C	/* Top Address of PCIe Capability */
/*==== PM Capability =========================================================*/
#define HFCFX_PCI_CAP_VPD			0x00B0	/* Top Address of VPD Capability  */
/*==== User Defined  Capability ==============================================*/
#define HFCFX_PCI_CAP_USER			0x00B8	/* Top Address of USER Capability */
#define HFCFX_PCI_USER(R)			(HFCFX_PCI_CAP_USER + HFCFX_PCI_USER_##R)
#define HFCFX_PCI_USER_NAKIDINT		0x0000	/* NAKEID_INT_0                   */
#define HFCFX_PCI_USER_INTRST		0x0004	/* INT_0_RST                      */
#define HFCFX_PCI_USER_STATUSH		0x0008	/* STATUS(Upper 32bit)            */
#define HFCFX_PCI_USER_STATUSL		0x000C	/* STATUS(Lower 32bit)            */
#define HFCFX_PCI_USER_RAMMSK		0x0010	/* RAMMSK                         */
#define HFCFX_PCI_USER_PRECONF		0x0011	/* Preconf                        */
#define HFCFX_PCI_USER_IDFLGEN		0x0013	/* IDFLGEN                        */
#define HFCFX_PCI_USER_RAMADDR		0x0014	/* RAMADDR                        */
#define HFCFX_PCI_USER_RAMI			0x0018	/* RAM Indirect Space             */
#define HFCFX_PCI_USER_CMDRES		0x0020	/* CMD_RES                        */
#define HFCFX_PCI_USER_CMDCTL		0x0021	/* CMD_CTL                        */
#define HFCFX_PCI_USER_CMDBOOT		0x0022	/* CMD_BOOT                       */
#define HFCFX_PCI_USER_CMDFMEM		0x0023	/* CMD_FMEM                       */
#define HFCFX_PCI_USER_CMDLED		0x0024	/* CMD_LED                        */
#define HFCFX_PCI_USER_CMDFCIF		0x0026	/* CMD_FCIF                       */
#define HFCFX_PCI_USER_CCADT		0x0028	/* CCA Direct Spase(Upper16byte)  */
#define HFCFX_PCI_USER_FRAME		0x0038	/* FRAME(Upper16byte)             */
/*==== PCIe AER  Capability ==================================================*/
#define HFCFX_PCI_CAP_AER			0x0100	/* Top Address of VPD Capability  */
/*==== PCIe PB  Capability ===================================================*/
#define HFCFX_PCI_CAP_PB			0x01D4	/* Top Address of PB Capability   */
/*==== PCIe SR-IOV  Capability ===============================================*/
#define HFCFX_PCI_CAP_SRI			0x01E4	/* Top Address of SRI Capability  */
/*==== PCIe ARI  Capability ==================================================*/
#define HFCFX_PCI_CAP_ARI			0x0224	/* Top Address of ARI Capability  */
/*==== PCIe SEC-EXT  Capability ==============================================*/
#define HFCFX_PCI_CAP_SEC			0x0254	/* Top Address of SEC Capability  */
/*==============================================================================
 *     Access Macros (PCI Config)
 *============================================================================*/
#define HFCFX_PCI_R4(P,R)		\
	((uint) hfc_fx_read_cnfg((P), HFCFX_PCI(R), 0x04)) 
#define HFCFX_PCI_R2(P,R)		\
	((ushort)hfc_fx_read_cnfg((P), HFCFX_PCI(R), 0x02))
#define HFCFX_PCI_R1(P,R)		\
	((uchar) hfc_fx_read_cnfg((P), HFCFX_PCI(R), 0x01))
#define HFCFX_PCI_W4(P,R,D)	\
	hfc_fx_write_cnfg((P), HFCFX_PCI(R), 0x04, (uint)D) 
#define HFCFX_PCI_W2(P,R,D)	\
	hfc_fx_write_cnfg((P), HFCFX_PCI(R), 0x02, (uint)D)
#define HFCFX_PCI_W1(P,R,D)	\
	hfc_fx_write_cnfg((P), HFCFX_PCI(R), 0x01, (uint)D)
//<<<<<FCWIN_FX-018

/*==============================================================================
 *    MMIO BAR0
 *============================================================================*/
#define HFCFX_MMIO(REG)			HFCFX_MMIO_##REG
#define HFCFX_MMIO_OFF						/*  Disappearance HFCX_MMIO()     */
//FCWIN_FX-018>>>>>
/*==== Install Infomation Registers ==========================================*/
#define HFCFX_MMIO_TOP				0x0000	/* TOP Address of MMIO            */
#define HFCFX_MMIO_LSICODE			0x0001	/* LSI CODE                       */
#define HFCFX_MMIO_FPGAREV			0x0002	/* FPGA REV                       */
#define HFCFX_MMIO_LSIREV			0x0003	/* LSI REV                        */
#define HFCFX_MMIO_PKCODE			0x0005	/* PK CODE                        */
#define HFCFX_MMIO_PKREV			0x0007	/* PK REV                         */
//<<<<<FCWIN_FX-018

/*==== Status and Control Registers ==========================================*/
/*---- Common(Card)/Port/Core Register Block Area ----------------------------*/
#define HFCFX_MMIO_PCTLA			0x0000	/* Port Block Control Reg Area    */
#define HFCFX_MMIO_KCTLA			0x0800  /* Common Block Control Reg Area  */
#define HFCFX_MMIO_CCTLA			0x0C00	/* Core Block Control Reg Area    */
/*---- Offset Address for Block ----------------------------------------------*/
#define HFCFX_MMIO_OF_STATUSH		0x0010	/* STATUS (Upper DWord)           */
#define HFCFX_MMIO_OF_STATUSL		0x0014	/* STATUS (Lower DWord)           */
#define HFCFX_MMIO_OF_EDETAIL		0x0018  /* Error Detail(Rsv,#0,#1,#2)     */
#define HFCFX_MMIO_OF_HWINFO		0x001C  /* H/W Info(#0,#1,#2,Rsv)         */
#define HFCFX_MMIO_OF_HWINFO3		0x001C	/* H/W Info#3 (Core Block Only)   */
#define HFCFX_MMIO_OF_HTYP			0x001D	/* HTYP#3     (Core Block Only)   */
#define HFCFX_MMIO_OF_OTYP			0x001E	/* OTYP       (Core Block Only)   */
#define HFCFX_MMIO_OF_LAPC			0x001F	/* LAPC       (Core Block Only)   */
#define HFCFX_MMIO_OF_FSYND			0x0024	/* FSYND      (Core Block Only)   */	//FCWIN_FX-018
#define HFCFX_MMIO_OF_FAR			0x0025	/* FAR        (Core Block Only)   */	//FCWIN_FX-018
#define HFCFX_MMIO_OF_CMDRES		0x0030  /* CMD_RES    CARD:PORT:CORE      */
#define HFCFX_MMIO_OF_CMDCTL		0x0031  /* CMD_CTL    CARD:PORT:CORE      */
#define HFCFX_MMIO_OF_CMDBOOT		0x0032  /* CMD_BOOT   CARD:PORT:CORE      */
#define HFCFX_MMIO_OF_CMDBCAST      0x0033  /* CMD_BCAST  CARD:****:****      */
#define HFCFX_MMIO_OF_CMDFMEM0		0x0034  /* CMD_FMEM0  CARD:****:****      */
#define HFCFX_MMIO_OF_CMDFMEM1		0x0035  /* CMD_FMEM1  CARD:****:****      */
#define HFCFX_MMIO_OF_CMDSCAN		0x0036  /* CMD_SCAN   CARD:****:****      */
#define HFCFX_MMIO_OF_CMDMODE		0x0037  /* CMD_MODE   CARD:****:****      */
#define HFCFX_MMIO_OF_CMDLED		0x0038  /* CMD_LED    CARD:PORT:****      */
#define HFCFX_MMIO_OF_CMDRES1		0x003A  /* CMD_RES1   CARD:****:****      */
#define HFCFX_MMIO_OF_CMDRES2		0x003B  /* CMD_RES2   CARD:PORT:****      */
#define HFCFX_MMIO_OF_CMDOPT		0x003C  /* CMD_OPT    CARD:PORT:****      */
#define HFCFX_MMIO_OF_CMDQOS		0x003D  /* CMD_QOS    ****:PORT:****      */
#define HFCFX_MMIO_OF_CMDIPCTL      0x003F  /* CMD_IPCTL  CARD:****:****      */
#define HFCFX_MMIO_OF_TRCCA			0x00F0	/* TRACE_CA   CARD:****:CORE      */	//FCWIN_FX-018
#define HFCFX_MMIO_OF_RUTRCCA		0x00F2	/* RUTRC_CA   CARD:****:****      */	//FCWIN_FX-018
/*---- Core# Shift Address (HFCFX_MMIO_"Register Name"(Core#)) ---------------*/ 
#define HFCFX_MMIO_STSCSS		0x0100	/* Core# Shift Address(STATUS)		  */
#define HFCFX_MMIO_CS_STATUSH(C)	(HFCFX_MMIO_OF_STATUSH + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_STATUSL(C)	(HFCFX_MMIO_OF_STATUSL + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_EDETAIL(C)	(HFCFX_MMIO_OF_EDETAIL + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_HWINFO3(C)	(HFCFX_MMIO_OF_HWINFO3 + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_HTYP(C)		(HFCFX_MMIO_OF_HTYP    + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_OTYP(C)		(HFCFX_MMIO_OF_OTYP    + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_LAPC(C)		(HFCFX_MMIO_OF_LAPC    + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_FSYND(C)		(HFCFX_MMIO_OF_FSYND   + (C)*HFCFX_MMIO_STSCSS)	//FCWIN_FX-018
#define HFCFX_MMIO_CS_FAR(C)		(HFCFX_MMIO_OF_FAR     + (C)*HFCFX_MMIO_STSCSS)	//FCWIN_FX-018
#define HFCFX_MMIO_CS_CMDRES(C)		(HFCFX_MMIO_OF_CMDRES  + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_CMDCTL(C)		(HFCFX_MMIO_OF_CMDCTL  + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_CMDBOOT(C)	(HFCFX_MMIO_OF_CMDBOOT + (C)*HFCFX_MMIO_STSCSS)
#define HFCFX_MMIO_CS_TRACECA(C)	(HFCFX_MMIO_OF_TRACECA + (C)*HFCFX_MMIO_STSCSS)	//FCWIN_FX-018
/*---- Block Area Shift Address ----------------------------------------------*/
#define HFCFX_MMIO_PORT(R)		(HFCFX_MMIO_PCTLA + HFCFX_MMIO_OF_##R)
#define HFCFX_MMIO_CMMN(R)		(HFCFX_MMIO_KCTLA + HFCFX_MMIO_OF_##R)
#define HFCFX_MMIO_CORE(R)		(HFCFX_MMIO_CCTLA + HFCFX_MMIO_CS_##R)
/*---- Bit Define for STATUS (Upper DWord)------------------------------------*/
#define HFCFX_MMIO_STTH(BIT)		HFCFX_MMIO_STTH_##BIT
#define HFCFX_MMIO_STTH_INSTAL		0x80000000
#define HFCFX_MMIO_STTH_NOINIT		0x40000000
#define HFCFX_MMIO_STTH_GENRES		0x08000000
#define HFCFX_MMIO_STTH_INIRES		0x04000000
#define HFCFX_MMIO_STTH_CTLRES		0x02000000
#define HFCFX_MMIO_STTH_CHKRES		0x01000000
#define HFCFX_MMIO_STTH_PCIERR		0x00800000
#define HFCFX_MMIO_STTH_EXGMCK		0x00400000
#define HFCFX_MMIO_STTH_BOOTRUN		0x00200000
#define HFCFX_MMIO_STTH_FNCSTOP		0x00100000
#define HFCFX_MMIO_STTH_TRCSTOP		0x00080000
#define HFCFX_MMIO_STTH_ADRCOIN		0x00040000
#define HFCFX_MMIO_STTH_CHKDSBL		0x00020000
#define HFCFX_MMIO_STTH_FMEMBSY		0x00010000
#define HFCFX_MMIO_STTH_COREOFFLINE	0x00008000
#define HFCFX_MMIO_STTH_PCIADRCIN	0x00004000
#define HFCFX_MMIO_STTH_ADRCINFSTP	0x00002000
#define HFCFX_MMIO_STTH_BOOTRUNOTR	0x00001000
#define HFCFX_MMIO_STTH_MPCHK		0x00000800
#define HFCFX_MMIO_STTH_ZKTRSTOP	0x00000400
#define HFCFX_MMIO_STTH_HCHKSTOP	0x00000100
#define HFCFX_MMIO_STTH_FATALUCE	0x00000080
#define HFCFX_MMIO_STTH_NONFATALUCE	0x00000040
#define HFCFX_MMIO_STTH_COMMONMCK	0x00000020
#define HFCFX_MMIO_STTH_CE			0x00000010
#define HFCFX_MMIO_STTH_INT0ASERT	0x00000008
#define HFCFX_MMIO_STTH_INT1ASERT	0x00000004
#define HFCFX_MMIO_STTH_INT2ASERT	0x00000002
#define HFCFX_MMIO_STTH_INT3ASERT	0x00000001
/*---- Bit Define for STATUS (Lower DWord)------------------------------------*/
#define HFCFX_MMIO_STTL(BIT)		HFCFX_MMIO_STTL_##BIT
#define HFCFX_MMIO_STTL_PLLNOINIT	0x80000000
#define HFCFX_MMIO_STTL_PCIFMWRBSY	0x40000000
#define HFCFX_MMIO_STTL_DL_LINKUP	0x20000000
#define HFCFX_MMIO_STTL_TGTCONFLICT	0x10000000
#define HFCFX_MMIO_STTL_BOOTERR		0x00800000
#define HFCFX_MMIO_STTL_FMEMBOOTEE	0x00400000
#define HFCFX_MMIO_STTL_FMEMPCIOE	0x00200000
#define HFCFX_MMIO_STTL_FMEMPCIEE	0x00100000
#define HFCFX_MMIO_STTL_TXDSBL		0x00008000
#define HFCFX_MMIO_STTL_LSGNL		0x00000800
#define HFCFX_MMIO_STTL_TXFAULT		0x00000080
#define HFCFX_MMIO_STTL_LSYNC		0x00000008
/*---- Bit Define for ErrorDetail --------------------------------------------*/
#define HFCFX_MMIO_EDTL(BIT)		HFCFX_MMIO_EDTL_##BIT
#define HFCFX_MMIO_EDTL_STARUN		0x01000000
#define HFCFX_MMIO_EDTL_HMCKST0		0x00800000
#define HFCFX_MMIO_EDTL_HMCKST1		0x00400000
#define HFCFX_MMIO_EDTL_HPUMCKST	0x00200000
#define HFCFX_MMIO_EDTL_HZUMCKST	0x00100000
#define HFCFX_MMIO_EDTL_HEUMCKST	0x00080000
#define HFCFX_MMIO_EDTL_HBUMCKST	0x00040000
#define HFCFX_MMIO_EDTL_HTUMCST		0x00020000
#define HFCFX_MMIO_EDTL_HRUMCKST	0x00010000
#define HFCFX_MMIO_EDTL_HMUMCKST	0x00008000
#define HFCFX_MMIO_EDTL_HLUMCKST	0x00004000
#define HFCFX_MMIO_EDTL_HIUMCKST	0x00002000
#define HFCFX_MMIO_EDTL_HBROADMCK	0x00000800
#define HFCFX_MMIO_EDTL_XHTO3CK		0x00000400
#define HFCFX_MMIO_EDTL_XHMPCK		0x00000200
#define HFCFX_MMIO_EDTL_USERMCK		0x00000080
#define HFCFX_MMIO_EDTL_PCIEIPERR	0x00000040
#define HFCFX_MMIO_EDTL_PCIEECCUCE	0x00000020
#define HFCFX_MMIO_EDTL_PCIEECCCE	0x00000010
/*---- Bit Define for HWINFO -------------------------------------------------*/
#define HFCFX_MMIO_HINF(BIT)		HFCFX_MMIO_HINF_##BIT
#define HFCFX_MMIO_HINF_PKTYP0		0x80000000
#define HFCFX_MMIO_HINF_PKTYP1		0x40000000
#define HFCFX_MMIO_HINF_PKTYP2		0x20000000
#define HFCFX_MMIO_HINF_PKTYP3		0x10000000
#define HFCFX_MMIO_HINF_PKREV0		0x08000000
#define HFCFX_MMIO_HINF_PKREV1		0x04000000
#define HFCFX_MMIO_HINF_PKREV2		0x02000000
#define HFCFX_MMIO_HINF_PKREV3		0x01000000
#define HFCFX_MMIO_HINF_FLASHINSTL	0x00800000
#define HFCFX_MMIO_HINF_FLASHTYPE0	0x00400000
#define HFCFX_MMIO_HINF_FLASHTYPE1	0x00200000
#define HFCFX_MMIO_HINF_RSS			0x00100000
#define HFCFX_MMIO_HINF_IPORT0		0x00080000
#define HFCFX_MMIO_HINF_IPORT1		0x00040000
#define HFCFX_MMIO_HINF_RSD			0x00020000
#define HFCFX_MMIO_HINF_MULTIPF		0x00010000
#define HFCFX_MMIO_HINF_FCSPEED0	0x00004000
#define HFCFX_MMIO_HINF_FCSPEED1	0x00002000
#define HFCFX_MMIO_HINF_OMTYP0		0x00000800
#define HFCFX_MMIO_HINF_OMTYP1		0x00000400
#define HFCFX_MMIO_HINF3(BIT)		HFCFX_MMIO_HINF3_##BIT
#define HFCFX_MMIO_HINF3_CORE		0x03000000	/*CORE# (CORE -- HWINFO3)     */
#define HFCFX_MMIO_HTYP(BIT)		HFCFX_MMIO_HTYP_##BIT
#define HFCFX_MMIO_HTYP_IOS			0x00400000	/*IOS   (CORE -- HTYP)        */
#define HFCFX_MMIO_HTYP_FC4TYP		0x00300000	/*FC4TYP(CORE -- HTYP)        */
#define HFCFX_MMIO_HTYP_CU			0x00080000	/*CU    (CORE -- HTYP)        */
#define HFCFX_MMIO_OTYP(BIT)		HFCFX_MMIO_OTYP_##BIT
#define HFCFX_MMIO_OTYP_MPTYP		0x0000E000	/*MPTYP (CORE -- OTYP)        */
#define HFCFX_MMIO_OTYP_PAG4K		0x00001000	/*PAG4K (CORE -- OTYP)        */
#define HFCFX_MMIO_LAPC(BIT)		HFCFX_MMIO_LAPC_##BIT
#define HFCFX_MMIO_LAPC_LAPC		0x0000007F	/*LAPC	(CORE -- LAPC)        */
/*---- Bit Define for CMD_RES (Byte Access)-----------------------------------*/
#define HFCFX_MMIO_CRES(BIT)		HFCFX_MMIO_CRES_##BIT
#define HFCFX_MMIO_CRES_HRESTIME	0xC0
#define HFCFX_MMIO_CRES_HPCIRESEXE	0x20
#define HFCFX_MMIO_CRES_HPONRES		0x10
#define HFCFX_MMIO_CRES_HGENRES		0x08
#define HFCFX_MMIO_CRES_HINIRES		0x04
#define HFCFX_MMIO_CRES_HCTLRES		0x02
#define HFCFX_MMIO_CRES_HCHKRES		0x01
#define HFCFX_MMIO_CRES_CLEAR		0x00
#define HFCFX_MMIO_CRES_WAIT(W)		\
	((((W) & HFCFX_MMIO_CRES_HRESTIME) == HFCFX_MMIO_CRES_HRESTIME) \
	? (1000 * 1000) \
	: ((32 ^ (((W) & HFCFX_MMIO_CRES_HRESTIME) >> 6)) / \
	  (((((W) & HFCFX_MMIO_CRES_HRESTIME) >> 6) + 2) / 2)))
/*---- Bit Define for CMD_CTL (Byte Access)-----------------------------------*/
#define HFCFX_MMIO_CCTL(BIT)		HFCFX_MMIO_CCTL_##BIT
#define HFCFX_MMIO_CCTL_STRTFNC		0x80
#define HFCFX_MMIO_CCTL_STOPFNC		0x40
#define HFCFX_MMIO_CCTL_STRTTRC		0x20
#define HFCFX_MMIO_CCTL_STOPTRC		0x10
#define HFCFX_MMIO_CCTL_FORCEMCK	0x08
#define HFCFX_MMIO_CCTL_CLEARTRC	0x04
#define HFCFX_MMIO_CCTL_ULPREQ		0x02
#define HFCFX_MMIO_CCTL_FORCECHKSTP	0x01
/*---- Bit Define for CMD_BOOT (Byte Access)----------------------------------*/
#define HFCFX_MMIO_CBOOT(BIT)		HFCFX_MMIO_CBOOT_##BIT
#define HFCFX_MMIO_CBOOT_PART1		0x20
#define HFCFX_MMIO_CBOOT_PART3		0x08
#define HFCFX_MMIO_CBOOT_RSTCHKSTP	0x01
#define HFCFX_MMIO_CBOOT_WAIT		1*1000*1000
/*---- Bit Define for CMD_FMEM (Byte Access)----------------------------------*/
#define HFCFX_MMIO_CFMEM(BIT)		HFCFX_MMIO_CFMEM_##BIT
#define HFCFX_MMIO_CFMEM_CERASE		0x80
#define HFCFX_MMIO_CFMEM_SERASE		0x40
#define HFCFX_MMIO_CFMEM_RESET		0x20
#define HFCFX_MMIO_CFMEM_SPROTECT	0x10
#define HFCFX_MMIO_CFMEM_SUNPROTECT	0x08
#define HFCFX_MMIO_CFMEM_BOOTEECLR	0x04
#define HFCFX_MMIO_CFMEM_PCIOECLR	0x02
#define HFCFX_MMIO_CFMEM_PCIEECLR	0x01
/*---- Bit Define for CMD_LED (Byte Access)-----------------------------------*/
#define HFCFX_MMIO_CLED(BIT)		HFCFX_MMIO_CLED_##BIT
#define HFCFX_MMIO_CLED_GMODE		0x70
#define HFCFX_MMIO_CLED_YMODE		0x07
/*---- Bit Define for CMD_RES2 (Byte Access)----------------------------------*/
#define HFCFX_MMIO_CRES2(BIT)		HFCFX_MMIO_CRES2_##BIT
#define HFCFX_MMIO_CRES2_BW2XT		0x20
#define HFCFX_MMIO_CRES2_BW2XR		0x10
#define HFCFX_MMIO_CRES2_FEC		0x08
#define HFCFX_MMIO_CRES2_AET		0x04
#define HFCFX_MMIO_CRES2_AEC		0x02
/*---- Bit Define for CMD_OPT (Byte Access)-----------------------------------*/
#define HFCFX_MMIO_COPT(BIT)		HFCFX_MMIO_COPT_##BIT
#define HFCFX_MMIO_COPT_DISABLELINK	0x80
#define HFCFX_MMIO_COPT_STORETSEQ	0x08
/*---- Bit Define for CMD_QOS (Byte Access)-----------------------------------*/
#define HFCFX_MMIO_CQOS(BIT)		HFCFX_MMIO_CQOS_##BIT
#define HFCFX_MMIO_CQOS_START		0x80
#define HFCFX_MMIO_CQOS_STOP		0x40
#define HFCFX_MMIO_CQOS_CLR			0x20
/*---- Bit Define for CMD_IPRES (Byte Access)---------------------------------*/
#define HFCFX_MMIO_CIPCTL_HSSRTUNE	0x80
#define HFCFX_MMIO_CIPCTL_FUCECLR	0x08
#define HFCFX_MMIO_CIPCTL_NUCECLR	0x04
#define HFCFX_MMIO_CIPCTL_CECLR		0x02
#define HFCFX_MMIO_CIPCTL_SRAMCECLR	0x01
/*==== Interrupt Registers ===================================================*/
/*---- Offset Address of Core#0 Registers ------------------------------------*/
#define HFCFX_MMIO_INT0				0x00A0	/* INT_0                          */
#define HFCFX_MMIO_MPINT0			0x00A4	/* MPINT_0                        */
#define HFCFX_MMIO_INTMSK0			0x00A8  /* INT_0_MASK                     */
#define HFCFX_MMIO_INTRST0			0x00AC  /* INT_0_RST                      */
#define HFCFX_MMIO_INT_VECTOR		0x00E0	/* INT_VECTOR                     */
#define HFCFX_MMIO_INT_VECTORRST	0x00E4  /* INT_VECTOR_RST                 */
#define HFCFX_MMIO_INT_SETTING		0x00E8  /* INT_SETTING                    */
#define HFCFX_MMIO_NAKED_INT0		0x00F0  /* NAKED_INT_0                    */
#define HFCFX_MMIO_STS0				0x0C10	/* STATUS_0						  */
#define HFCFX_MMIO_STS1				0x0C14	/* STATUS_1						  */
#define HFCFX_MMIO_STS2				0x0C18	/* STATUS_2(ErrorDetail)		  */

//#define HFCFX_MMIO_FRAME_A0			0x0500	/* Frame-A						  */
/*---- Core# Shift Address ---------------------------------------------------*/ 
#define HFCFX_MMIO_INTCSI		0x0010	/* Core# Shift Address(INT)           */
#define HFCFX_MMIO_INTCSN		0x0004  /* Core# Shift Address(NAKED_INT)     */
#define HFCFX_MMIO_INTCSV		0x0001	/* Core# Shift Address(INT_VECTOR)    */

#define HFCFX_MMIO_INT(C)		(HFCFX_MMIO_INT0           + (C)*HFCFX_MMIO_INTCSI)
#define HFCFX_MMIO_MPINT(C)		(HFCFX_MMIO_MPINT0         + (C)*HFCFX_MMIO_INTCSI)
#define HFCFX_MMIO_INTMSK(C)	(HFCFX_MMIO_INTMSK0        + (C)*HFCFX_MMIO_INTCSI)
#define HFCFX_MMIO_INTRST(C)	(HFCFX_MMIO_INTRST0        + (C)*HFCFX_MMIO_INTCSI)
#define HFCFX_MMIO_INTVEC(C)	(HFCFX_MMIO_INT_VECTOR     + (C)*HFCFX_MMIO_INTCSV)
#define HFCFX_MMIO_INTVECRST(C)	(HFCFX_MMIO_INT_VECTORRST  + (C)*HFCFX_MMIO_INTCSV)
#define HFCFX_MMIO_NKINT(C)		(HFCFX_MMIO_NAKED_INT0     + (C)*HFCFX_MMIO_INTCSN)

/*---- Bit Define for INT_N --------------------------------------------------*/
#define HFCFX_MMIO_INTN(BIT)		HFCFX_MMIO_INTN_##BIT
#define HFCFX_MMIO_INTN_EXGMCK		0x80000000
#define HFCFX_MMIO_INTN_PCIUCEF		0x40000000
#define HFCFX_MMIO_INTN_PCIUCEN		0x20000000
#define HFCFX_MMIO_INTN_PCICE		0x10000000
#define HFCFX_MMIO_INTN_IPSRAMCE	0x08000000
#define HFCFX_MMIO_INTN_MBINT	    0x00008000
#define HFCFX_MMIO_INTN_MBRSP	    0x00004000
#define HFCFX_MMIO_INTN_XRB		    0x00002000
#define HFCFX_MMIO_INTN_HYPER	    0x00000100
#define HFCFX_MMIO_INTN_HWERR		(HFCFX_MMIO_INTN_EXGMCK |\
									 HFCFX_MMIO_INTN_PCIUCEF|\
									 HFCFX_MMIO_INTN_PCIUCEN)
#define HFCFX_MMIO_INTN_FWINT		(HFCFX_MMIO_INTN_MBINT|\
									 HFCFX_MMIO_INTN_MBRSP|\
									 HFCFX_MMIO_INTN_XRB)	                                 
#define HFCFX_MMIO_INTN_MSKCLS		(uint)0
#define HFCFX_MMIO_INTN_ALLRST		(uint)-1								
/*---- Bit Define for INT_VECTOR ---------------------------------------------*/
#define HFCFX_MMIO_INTV(BIT)		HFCFX_MMIO_INTV_##BIT
#define HFCFX_MMIO_INTV_32_MBINT	0x80000000
#define HFCFX_MMIO_INTV_32_MBRSP	0x40000000
#define HFCFX_MMIO_INTV_32_XRB		0x20000000
#define HFCFX_MMIO_INTV_32_HWINT	0x04000000
#define HFCFX_MMIO_INTV_32_HYPER	0x01000000
#define HFCFX_MMIO_INTV_MBINT(C)	(HFCFX_MMIO_INTV_32_MBINT>>((C)*8))
#define HFCFX_MMIO_INTV_MBRSP(C)	(HFCFX_MMIO_INTV_32_MBRSP>>((C)*8))
#define HFCFX_MMIO_INTV_XRB(C)		(HFCFX_MMIO_INTV_32_XRB>>((C)*8))
#define HFCFX_MMIO_INTV_HWINT(C)	(HFCFX_MMIO_INTV_32_HWINT>>((C)*8))
#define HFCFX_MMIO_INTV_HYPER(C)	(HFCFX_MMIO_INTV_32_HYPER>>((C)*8))
#define HFCFX_MMIO_INTV_ANY_MBINT	(HFCFX_MMIO_INTV_MBINT(0) |\
	                                 HFCFX_MMIO_INTV_MBINT(1) |\
									 HFCFX_MMIO_INTV_MBINT(2) |\
									 HFCFX_MMIO_INTV_MBINT(3))
#define HFCFX_MMIO_INTV_ANY_MBRSP	(HFCFX_MMIO_INTV_MBRSP(0) |\
	                                 HFCFX_MMIO_INTV_MBRSP(1) |\
									 HFCFX_MMIO_INTV_MBRSP(2) |\
									 HFCFX_MMIO_INTV_MBRSP(3))
#define HFCFX_MMIO_INTV_ANY_XRB		(HFCFX_MMIO_INTV_XRB(0) |\
	                                 HFCFX_MMIO_INTV_XRB(1) |\
									 HFCFX_MMIO_INTV_XRB(2) |\
									 HFCFX_MMIO_INTV_XRB(3))
#define HFCFX_MMIO_INTV_ANY_HWINT	(HFCFX_MMIO_INTV_HWINT(0) |\
	                                 HFCFX_MMIO_INTV_HWINT(1) |\
									 HFCFX_MMIO_INTV_HWINT(2) |\
									 HFCFX_MMIO_INTV_HWINT(3))
#define HFCFX_MMIO_INTV_ANY_HYPER	(HFCFX_MMIO_INTV_HYPER(0) |\
	                                 HFCFX_MMIO_INTV_HYPER(1) |\
									 HFCFX_MMIO_INTV_HYPER(2) |\
									 HFCFX_MMIO_INTV_HYPER(3))
/*---- Bit Define for MPINT_N-------------------------------------------------*/
#define HFCFX_MMIO_MPINT_MBINT		HFCFX_MMIO_MPINT_BT_MASK(MBINT)
#define HFCFX_MMIO_MPINT_MBRSP		HFCFX_MMIO_MPINT_BT_MASK(MBRSP)
#define HFCFX_MMIO_MPINT_XRB		HFCFX_MMIO_MPINT_BT_MASK(XRB)
/*---- RID Get Macro from MMIO MPINT -----------------------------------------*/
#define HFCFX_MMIO_MPINT_BT_MBINT	1
#define HFCFX_MMIO_MPINT_BT_MBRSP	2
#define HFCFX_MMIO_MPINT_BT_XRB		3
#define HFCFX_MMIO_MPINT_BT_MASK(R)	(0xFF000000 >> (HFCFX_MMIO_MPINT_BT_##R * 8))
#define HFCFX_MMIO_GETRID(P, C, R)	(uchar)(\
	((HFCFX_MMIO_R4(P, MPINT(C)) & HFCFX_MMIO_MPINT_BT_MASK(R))\
	>> (3 - (HFCFX_MMIO_MPINT_BT_##R)) * 8))

/*---- Bit Define for INT_SETTING --------------------------------------------*/
#define HFCFX_MMIO_INTS_MSIXSHORT	0x80000000

/*==== PTYP Registers ========================================================*/
#define HFCFX_MMIO_PTYP0			0x0110	/* PTYP0                          */
/*---- Bit Define for PTYP0 --------------------------------------------------*/
#define HFCFX_MMIO_PTYP(BIT)		HFCFX_MMIO_PTYP_PTYP##BIT
#define HFCFX_MMIO_PTYP_PTYP00		0x80
#define HFCFX_MMIO_PTYP_PTYP01		0x40
#define HFCFX_MMIO_PTYP_PTYP02		0x20
#define HFCFX_MMIO_PTYP_PTYP03		0x10
#define HFCFX_MMIO_PTYP_PTYP04		0x08
#define HFCFX_MMIO_PTYP_PTYP05		0x04
#define HFCFX_MMIO_PTYP_PTYP06		0x02
#define HFCFX_MMIO_PTYP_PTYP07		0x01
/*==== TSEQ Register =========================================================*/
#define HFCFX_MMIO_TSEQ				0x0144	/* TSEQ                           */
/*---- Bit Define for TSEQ ---------------------------------------------------*/
#define HFCFX_MMIO_TSEQ_NOS			0x04
#define HFCFX_MMIO_TSEQ_OLS			0x05
#define HFCFX_MMIO_TSEQ_LPR			0x07
/*==== RAM ===================================================================*/
#define HFCFX_MMIO_TGTCORE			0x02F0						//FCWIN_FX-018
#define HFCFX_MMIO_RAMMSK			0x02F8
#define HFCFX_MMIO_IDFLGEN			0x02FB
#define HFCFX_MMIO_RAMADR			0x02FC
/*---- Bit Define for RAMMSK -------------------------------------------------*/
#define HFCFX_MMIO_RAMMSK_TYP0MSK	0x80
#define HFCFX_MMIO_RAMMSK_TYP1MSK	0x40
#define HFCFX_MMIO_RAMMSK_TYP2MSK	0x20
#define HFCFX_MMIO_RAMMSK_TYP3MSK	0x10
#define HFCFX_MMIO_RAMMSK_CFGRAMEN	0x01
/*---- Bit Define for IDFLGEN ------------------------------------------------*/
#define HFCFX_MMIO_IDFLGEN_IDFLGEN	0x08
/*---- Bit Define for RAMADR -------------------------------------------------*/
#define HFCFX_MMIO_RAMADR_FLG		0x80000000
#define HFCFX_MMIO_RAMADR_MODE		0x70000000
//FCWIN_FX-018>>>>>
#define HFCFX_MMIO_RAMADR_MODEG		0x00000000
#define HFCFX_MMIO_RAMADR_MODELI	0x10000000
#define HFCFX_MMIO_RAMADR_MODELD	0x20000000
#define HFCFX_MMIO_RAMADR_ADR		0x0FFFFFFC
#define HFCFX_MMIO_RAMADR_ECWS		0x00000000
#define HFCFX_MMIO_RAMADR_ECGRS		0x00C00000
#define HFCFX_MMIO_RAMADR_ECLS		0x04000000
#define HFCFX_MMIO_RAMADR_MKCCR		0x04800000
#define HFCFX_MMIO_RAMADR_ZCTR		0x06000000
#define HFCFX_MMIO_RAMADR_ZKTR		0x06400000
#define HFCFX_MMIO_RAMADR_BCBCR		0x08000000
#define HFCFX_MMIO_RAMADR_FLASH		0x0C000000
#define HFCFX_MMIO_RAMADR_DUMP		0x0D000000
#define HFCFX_MMIO_RAMADR_UTL		0x0D800000
#define HFCFX_MMIO_RAMADR_IDREG		0x0E400000
//<<<<<FCWIN_FX-018
/*==== CCA ===================================================================*/
/*---- CCA Direct/Extend Area ------------------------------------------------*/
#define HFCFX_MMIO_DTCCA			0x0300	/* CCA Direct Area                */
#define HFCFX_MMIO_EXCCA			0x1000  /* CCA Extend Area                */
/*---- Offset Address for Block ----------------------------------------------*/
//FCWIN_FX-018>>>>>
#define HFCFX_MMIO_CCA_OF_TOP			0x0000	/*Top Address of CCA          */
#define HFCFX_MMIO_CCA_OF_POST_RSLT		0x0000	/*POST Result                 */
#define HFCFX_MMIO_CCA_OF_FREE_XRB		0x0002	/*Free XRB                    */
#define HFCFX_MMIO_CCA_OF_MPCK_CODE		0x0004	/*MPCK Code                   */
#define HFCFX_MMIO_CCA_OF_INIT_ADDR		0x0010	/*INIT Addr                   */
#define HFCFX_MMIO_CCA_OF_MEMI_ADDR		0x0018	/*MEM_INFO Addr               */
#define HFCFX_MMIO_CCA_OF_SEL_INFO		0x0020	/*SEL Infomation              */
#define HFCFX_MMIO_CCA_OF_OPMODE		0x0024	/*OPMODE                      */
#define HFCFX_MMIO_CCA_OF_MLPF_MODE		0x0025	/*MLPF Mode                   */
#define HFCFX_MMIO_CCA_OF_FMCK_CODE		0x0026	/*ForceMCK Code               */
#define HFCFX_MMIO_CCA_OF_UPDATE_ST		0x0027	/*Online Update State         */
#define HFCFX_MMIO_CCA_OF_FWSUPPORT		0x0028	/*FW Support Informaion       */
#define HFCFX_MMIO_CCA_OF_FMCK_INFO		0x002C	/*ForceMCK Add Info           */
#define HFCFX_MMIO_CCA_OF_TRACEMODE		0x0050	/*Trace Mode                  */
#define HFCFX_MMIO_CCA_OF_DIAGINFO		0x0052	/*POST/DIAG Info              */
#define HFCFX_MMIO_CCA_OF_ISOLCORE		0x005C	/*Isolate Core(bitmap)        */
#define HFCFX_MMIO_CCA_OF_DIAGAREA		0x0070	/*DIAG AREA(Use for Tool)     */
//<<<<<FCWIN_FX-018
/*---- Core# Shift Address (HFCFX_MMIO_"Register Name"(Core#)) ---------------*/ 
#define HFCFX_MMIO_CCA_DTS			0x0080	/* CCA(Direct) Core# Shift Address*/
#define HFCFX_MMIO_CCA_EXR			0x0080	/* CCA(Extend) RID Shift Address  */
#define HFCFX_MMIO_CCA_EXC			0x0020	/* CCA(Extend) Core# Shift Address*/
#define HFCFX_MMIO_CCADT_TOP(C)			(HFCFX_MMIO_CCA_OF_TOP       + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_POST_RSLT(C)	(HFCFX_MMIO_CCA_OF_POST_RSLT + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_FREE_XRB(C)	(HFCFX_MMIO_CCA_OF_FREE_XRB  + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_MPCK_CODE(C)	(HFCFX_MMIO_CCA_OF_MPCK_CODE + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_INIT_ADDR(C)	(HFCFX_MMIO_CCA_OF_INIT_ADDR + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_MEMI_ADDR(C)	(HFCFX_MMIO_CCA_OF_MEMI_ADDR + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_SEL_INFO(C)	(HFCFX_MMIO_CCA_OF_SEL_INFO  + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_OPMODE(C)		(HFCFX_MMIO_CCA_OF_OPMODE    + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_MLPF_MODE(C)	(HFCFX_MMIO_CCA_OF_MLPF_MODE + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_FMCK_CODE(C)	(HFCFX_MMIO_CCA_OF_FMCK_CODE + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_UPDATE_ST(C)	(HFCFX_MMIO_CCA_OF_UPDATE_ST + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_FWSUPPORT(C)	(HFCFX_MMIO_CCA_OF_FWSUPPORT + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_FMCK_INFO(C)	(HFCFX_MMIO_CCA_OF_FMCK_INFO + (C)*HFCFX_MMIO_CCA_DTS)
//FCWIN_FX-018>>>>>
#define HFCFX_MMIO_CCADT_TRACEMODE(C)	(HFCFX_MMIO_CCA_OF_TRACEMODE + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_DIAGINFO(C)	(HFCFX_MMIO_CCA_OF_DIAGINFO  + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_ISOLCORE(C)	(HFCFX_MMIO_CCA_OF_ISOLCORE  + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCADT_DIAGAREA(C)	(HFCFX_MMIO_CCA_OF_DIAGAREA  + (C)*HFCFX_MMIO_CCA_DTS)
//<<<<FCWIN_FX-018
#define HFCFX_MMIO_CCADT_OFF(O,C)		((O) + (C)*HFCFX_MMIO_CCA_DTS)
#define HFCFX_MMIO_CCAEX_TOP(R,C)		(HFCFX_MMIO_CCA_OF_TOP       + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_POST_RSLT(R,C)	(HFCFX_MMIO_CCA_OF_POST_RSLT + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_FREE_XRB(R,C)	(HFCFX_MMIO_CCA_OF_FREE_XRB  + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_MPCK_CODE(R,C)	(HFCFX_MMIO_CCA_OF_MPCK_CODE + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_INIT_ADDR(R,C)	(HFCFX_MMIO_CCA_OF_INIT_ADDR + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_MEMI_ADDR(R,C)	(HFCFX_MMIO_CCA_OF_MEMI_ADDR + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
#define HFCFX_MMIO_CCAEX_OFF(O,R,C)		((O) + (R)*HFCFX_MMIO_CCA_EXR + (C)*HFCFX_MMIO_CCA_EXC)
/*---- Area Shift Address ----------------------------------------------------*/
#define HFCFX_MMIO_CCA_DT(R)		(HFCFX_MMIO_DTCCA + HFCFX_MMIO_CCADT_##R)
#define HFCFX_MMIO_CCA_EX(R)		(HFCFX_MMIO_EXCCA + HFCFX_MMIO_CCAEX_##R)

/*==== FRAME Registers =======================================================*/
#define HFCFX_MMIO_FRAME0			0x0500	/* FRAME_0                        */
#define HFCFX_MMIO_FRAMECS			0x0040	/* Core# Shift Address(FRAME_#)   */
#define HFCFX_MMIO_FRAME(C)		(HFCFX_MMIO_FRAME0 + (C)*HFCFX_MMIO_FRAMECS)

/*==== RAM INDIRECT AREA Registers ===========================================*/
#define HFCFX_MMIO_RAMAREA			0x0600

//FCWIN_FX-018>>>>>
/*==== Flash FAR =============================================================*/
#define HFCFX_MMIO_BOOTFAR			0x0840	/* BOOTFAR                        */
/*==== ECID ==================================================================*/
#define HFCFX_MMIO_ECID				0x08D0	/* ECID                           *//* FCLNX-GPL-FX-141 */

/*==============================================================================
 *    MMIO BAR0(Direct SCAN-MAP AREA 0x4000 - 0x7FF)
 *         Offset
 *       + N x 0x1000    ---- Core Shift
 *       + 0x4000        ---- Area Shift
 *============================================================================*/
/*==== Area and Shift Address ================================================*/
#define HFCFX_MMIO_SCAN_TOP			0x4000	/* Start Address of SCAN AREA     */
#define HFCFX_MMIO_SCAN_CS			0x1000	/* CORE# Shift Address            */
#define HFCFX_MMIO_SCAN_PS			0x1000	/* PORT# Shift Address            */
/*==== Offset Address ========================================================*/
/*---- Offset Address for Block -------- MMIO Compatible Area[0x0000 - 0x02FF]*/
#define HFCFX_MMIO_OF_ELV			0x002C	/* ELV                            */
#define HFCFX_MMIO_OF_GR0			0x0040	/* GR#0                           */
#define HFCFX_MMIO_OF_GR1			0x0044	/* GR#1                           */
#define HFCFX_MMIO_OF_GR2			0x0048	/* GR#2                           */
#define HFCFX_MMIO_OF_GR3			0x004C	/* GR#3                           */
#define HFCFX_MMIO_OF_GR4			0x0050	/* GR#4                           */
#define HFCFX_MMIO_OF_GR5			0x0054	/* GR#5                           */
#define HFCFX_MMIO_OF_GR6			0x0058	/* GR#6                           */
#define HFCFX_MMIO_OF_GR7			0x005C	/* GR#7                           */
#define HFCFX_MMIO_OF_GR8			0x0060	/* GR#8                           */
#define HFCFX_MMIO_OF_GR9			0x0064	/* GR#9                           */
#define HFCFX_MMIO_OF_GRA			0x0068	/* GR#A                           */
#define HFCFX_MMIO_OF_GRB			0x006C	/* GR#B                           */
#define HFCFX_MMIO_OF_GRC			0x0070	/* GR#C                           */
#define HFCFX_MMIO_OF_GRD			0x0074	/* GR#D                           */
#define HFCFX_MMIO_OF_GRE			0x0078	/* GR#E                           */
#define HFCFX_MMIO_OF_GRF			0x007C	/* GR#F                           */
#define HFCFX_MMIO_OF_LBR0			0x0090	/* LBR0                           */
#define HFCFX_MMIO_OF_LBR1			0x0094	/* LBR1                           */
#define HFCFX_MMIO_OF_LBR2			0x0098	/* LBR2                           */
#define HFCFX_MMIO_OF_LBR3			0x009C	/* LBR3                           */
#define HFCFX_MMIO_OF_TOD			0x00FC	/* TOD                            */
#define HFCFX_MMIO_OF_FRIPOP		0x01C0	/* FRiP/FRoP                      */		
/*---- Offset Address for Block --------- Common Checker Area[0x0300 - 0x044F]*/
#define HFCFX_MMIO_OF_CKCMNTU		0x0300  /* Common TU Checker Area         */
#define HFCFX_MMIO_OF_CKCMNPU		0x0370	/* Common PU Checher Area         */
#define HFCFX_MMIO_OF_CKCMNZU		0x0410	/* Common ZU Checker Area         */
/*---- Offset Address for Block ----------- Port Checker Area[0x0450 - 0x049F]*/
#define HFCFX_MMIO_OF_CKPORT		0x0450  /* PORT Checker Area              */
/*---- Offset Address for Block --------------- ZU Block Area[0x04A0 - 0x04DF]*/
/*---- Offset Address for Block --------------- PU Block Area[0x04E0 - 0x04FF]*/
/*---- Offset Address for Block --------------- BU Block Area[0x0500 - 0x05FF]*/
#define HFCFX_MMIO_OF_BSTRA			0x0500	/* BSTRA                          */
#define HFCFX_MMIO_OF_FTBBSY		0x0510	/* FTBBSY                         */
#define HFCFX_MMIO_OF_STBBSY		0x0520	/* STBBSY                         */
#define HFCFX_MMIO_OF_RID			0x0560	/* RID                            */
/*---- Offset Address for Block ---- RAM Indirect Access Area[0x0600 - 0x07FF]*/
/*---- Offset Address for Block -------- MMIO Compatible Area[0x0800 - 0x08FF]*/
/*---- Offset Address for Block --------------- MU Block Area[0x0900 - 0x096F]*/
/*---- Offset Address for Block --------------- EU Block Area[0x0970 - 0x0AFF]*/
#define HFCFX_MMIO_OF_RTBR			0x0990	/* RTBR                           */
#define HFCFX_MMIO_OF_ULP			0x0AB4	/* ULP                            */
/*---- Offset Address for Block --------------- TU Block Area[0x0B00 - 0x0BFF]*/
#define HFCFX_MMIO_OF_RLRA			0x0B00	/* RLRA                           */
#define HFCFX_MMIO_OF_TLRA			0x0B04	/* TLRA                           */
#define HFCFX_MMIO_OF_TLRBSY		0x0B90	/* TLRBSY                         */ 
/*---- Offset Address for Block -------- MMIO Compatible Area[0x0C00 - 0x0CFF]*/
/*---- Offset Address for Block --------------- RU Block Area[0x0D00 - 0x0DFF]*/
#define HFCFX_MMIO_OF_OTHBR			0x0D00	/* OTHER_BR                       */
#define HFCFX_MMIO_OF_RLRBSY		0x0D80	/* RLRBSY                         */
#define HFCFX_MMIO_OF_HK0RLRBSYTMP0	0x0DD0	/* HK0RLRBSYTMP #  0- #127        */
#define HFCFX_MMIO_OF_HK0RLRBSYTMP1	0x0DF0	/* HK0RLRBSYTMP #128- #255        */
/*---- Offset Address for Block --------------- LU Block Area[0x0E00 - 0x0EFF]*/
/*---- Offset Address for Block ----------- Core Checker Area[0x0F00 - 0x0FFF]*/
#define HFCFX_MMIO_OF_CKCORE		0x0F00	/* CORE Checker Area              */
#define HFCFX_MMIO_OF_HK0RLRBSYTMP2	0x0FC0	/* HK0RLRBSYTMP #256- #383        */
#define HFCFX_MMIO_OF_HK0RLRBSYTMP3	0x0FD0	/* HK0RLRBSYTMP #384- #511        */
/*==== CORE Shift Address ====================================================*/
/*---- Register Shift Address ---------- MMIO Compatible Area[0x0000 - 0x02FF]*/
#define HFCFX_MMIO_SCAN_STATUSH(C)	(HFCFX_MMIO_OF_STATUSH + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_STATUSL(C)	(HFCFX_MMIO_OF_STATUSL + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_FSYND(C)	(HFCFX_MMIO_OF_FSYND   + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_ELV(C)		(HFCFX_MMIO_OF_ELV     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR0(C)		(HFCFX_MMIO_OF_GR0     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR1(C)		(HFCFX_MMIO_OF_GR1     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR2(C)		(HFCFX_MMIO_OF_GR2     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR3(C)		(HFCFX_MMIO_OF_GR3     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR4(C)		(HFCFX_MMIO_OF_GR4     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR5(C)		(HFCFX_MMIO_OF_GR5     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR6(C)		(HFCFX_MMIO_OF_GR6     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR7(C)		(HFCFX_MMIO_OF_GR7     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR8(C)		(HFCFX_MMIO_OF_GR8     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GR9(C)		(HFCFX_MMIO_OF_GR9     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRA(C)		(HFCFX_MMIO_OF_GRA     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRB(C)		(HFCFX_MMIO_OF_GRB     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRC(C)		(HFCFX_MMIO_OF_GRC     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRD(C)		(HFCFX_MMIO_OF_GRD     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRE(C)		(HFCFX_MMIO_OF_GRE     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_GRF(C)		(HFCFX_MMIO_OF_GRF     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_LBR0(C)		(HFCFX_MMIO_OF_LBR0    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_LBR1(C)		(HFCFX_MMIO_OF_LBR1    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_LBR2(C)		(HFCFX_MMIO_OF_LBR2    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_LBR3(C)		(HFCFX_MMIO_OF_LBR3    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_TRCCA(C)	(HFCFX_MMIO_OF_TRCCA   + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_TOD(C)		(HFCFX_MMIO_OF_TOD     + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_INT(C)		(HFCFX_MMIO_INT0       + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_FRIPFROP(C)	(HFCFX_MMIO_OF_FRIPOP  + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_PORT(R)		(HFCFX_MMIO_PCTLA + HFCFX_MMIO_SCAN_##R)
/*---- Register Shift Address ----------- Common Checker Area[0x0300 - 0x044F]*/
#define HFCFX_MMIO_SCAN_CKCMNTU		HFCFX_MMIO_OF_CKCMNTU
#define HFCFX_MMIO_SCAN_CKCMNPU		HFCFX_MMIO_OF_CKCMNPU
#define HFCFX_MMIO_SCAN_CKCMNZU		HFCFX_MMIO_OF_CKCMNZU
/*---- Register Shift Address ------------- Port Checker Area[0x0450 - 0x049F]*/
#define HFCFX_MMIO_SCAN_CKPORT(P)	(HFCFX_MMIO_OF_CKPORT  + (P)*HFCFX_MMIO_SCAN_PS)
/*---- Register Shift Address ----------------- ZU Block Area[0x04A0 - 0x04DF]*/
/*---- Register Shift Address ----------------- PU Block Area[0x04E0 - 0x04FF]*/
/*---- Register Shift Address ----------------- BU Block Area[0x0500 - 0x05FF]*/
#define HFCFX_MMIO_SCAN_BSTRA(C)	(HFCFX_MMIO_OF_BSTRA   + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_FTBBSY(C)	(HFCFX_MMIO_OF_FTBBSY  + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_STBBSY(C)	(HFCFX_MMIO_OF_STBBSY  + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_RID(C)		(HFCFX_MMIO_OF_RID     + (C)*HFCFX_MMIO_SCAN_CS)
/*---- Register Shift Address ------ RAM Indirect Access Area[0x0600 - 0x07FF]*/
/*---- Register Shift Address ---------- MMIO Compatible Area[0x0800 - 0x08FF]*/
#define HFCFX_MMIO_SCAN_CMMN(R)		(HFCFX_MMIO_KCTLA      + HFCFX_MMIO_SCAN_##R)
/*---- Register Shift Address ----------------- MU Block Area[0x0900 - 0x096F]*/
/*---- Register Shift Address ----------------- EU Block Area[0x0970 - 0x0AFF]*/
#define HFCFX_MMIO_SCAN_RTBR(C)		(HFCFX_MMIO_OF_RTBR    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_ULP(C)		(HFCFX_MMIO_OF_ULP     + (C)*HFCFX_MMIO_SCAN_CS)
/*---- Register Shift Address ----------------- TU Block Area[0x0B00 - 0x0BFF]*/
#define HFCFX_MMIO_SCAN_RLRA(C)		(HFCFX_MMIO_OF_RLRA    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_TLRA(C)		(HFCFX_MMIO_OF_TLRA    + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_TLRBSY(C)	(HFCFX_MMIO_OF_TLRBSY  + (C)*HFCFX_MMIO_SCAN_CS)
/*---- Register Shift Address ---------- MMIO Compatible Area[0x0C00 - 0x0CFF]*/
#define HFCFX_MMIO_SCAN_CORE(R)		(HFCFX_MMIO_CCTLA      + HFCFX_MMIO_SCAN_##R)
/*---- Register Shift Address ----------------- RU Block Area[0x0D00 - 0x0DFF]*/
#define HFCFX_MMIO_SCAN_OTHERBR(C)	(HFCFX_MMIO_OF_OTHBR   + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_RLRBSY(C)   (HFCFX_MMIO_OF_RLRBSY  + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_RLRBSYTMP0	HFCFX_MMIO_OF_HK0RLRBSYTMP0
#define HFCFX_MMIO_SCAN_RLRBSYTMP1	HFCFX_MMIO_OF_HK0RLRBSYTMP1
/*---- Register Shift Address ----------------- LU Block Area[0x0E00 - 0x0EFF]*/
/*---- Register Shift Address ------------- Core Checker Area[0x0F00 - 0x0FFF]*/
#define HFCFX_MMIO_SCAN_CKCORE(C)	(HFCFX_MMIO_OF_CKCORE  + (C)*HFCFX_MMIO_SCAN_CS)
#define HFCFX_MMIO_SCAN_RLRBSYTMP2	HFCFX_MMIO_OF_HK0RLRBSYTMP2
#define HFCFX_MMIO_SCAN_RLRBSYTMP3	HFCFX_MMIO_OF_HK0RLRBSYTMP3
/*---- Register Shift Address ---- SCAN AREA Shift ---------------------------*/
#define HFCFX_MMIO_SCAN(R)			(HFCFX_MMIO_SCAN_TOP + HFCFX_MMIO_SCAN_##R)

/* FCLNX-GPL-FX-141 */
/*---- Bit Define for ELV ----------------------------------------------------*/
#define HFCFX_MMIO_ELV_LEVEL2C		0x80
#define HFCFX_MMIO_ELV_LEVEL1C		0x40
#define HFCFX_MMIO_ELV_LEVEL2B		0x20
#define HFCFX_MMIO_ELV_LEVEL1B		0x10
#define HFCFX_MMIO_ELV_LEVEL3		0x08
#define HFCFX_MMIO_ELV_LEVEL2A		0x04
#define HFCFX_MMIO_ELV_LEVEL1A		0x02
#define HFCFX_MMIO_ELV_LEVEL0		0x01
/* FCLNX-GPL-FX-141 */

/*==============================================================================
 *     Access Macros (MMIO BAR0)
 *============================================================================*/
#define HFCFX_MMIO_R4(P,R)		\
	((uint) hfc_fx_read_reg_ext((P), HFCFX_MMIO(R), 0x04)) 
#define HFCFX_MMIO_R2(P,R)		\
	((ushort)hfc_fx_read_reg_ext((P), HFCFX_MMIO(R), 0x02))
#define HFCFX_MMIO_R1(P,R)		\
	((uchar) hfc_fx_read_reg_ext((P), HFCFX_MMIO(R), 0x01))
#define HFCFX_MMIO_W4(P,R,D)	\
	hfc_fx_write_reg_ext((P), HFCFX_MMIO(R), 0x04, (uint)D) 
#define HFCFX_MMIO_W2(P,R,D)	\
	hfc_fx_write_reg_ext((P), HFCFX_MMIO(R), 0x02, (ushort)D)
#define HFCFX_MMIO_W1(P,R,D)	\
	hfc_fx_write_reg_ext((P), HFCFX_MMIO(R), 0x01, (uchar)D)
//FCWIN_FX-018>>>>>
/*==============================================================================
 *    RAM Indirect Access
 *============================================================================*/
/*==== WS ====================================================================*/
/*---- Base Address ----------------------------------------------------------*/
#define HFCFX_RAMI_WS_UI		0x00000000	/* ULP Info Area                  */
#define HFCFX_RAMI_WS_LW		0x00002000	/* LEVEL WK Area                  */
#define HFCFX_RAMI_WS_CM		0x00003000	/* Common Info Area               */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_WS_US		0x00000100	/* ULP Shift                      */
#define HFCFX_RAMI_WS_LS		0x00000100	/* LEVEL Shift                    */
/*---- Offset Address of ULP Info --------------------------------------------*/
#define HFCFX_RAMI_WS_OF_ULPI	0x00000000	/* WS ULP#  Info Top              */
#define HFCFX_RAMI_WS_OF_CCAI	0x00000000	/* WS ULP#  CCA Info              */
#define HFCFX_RAMI_WS_OF_RIDI	0x00000080	/* WS ULP#  RID Info              */
#define HFCFX_RAMI_WS_OF_RID1	0x00000080	/* WS ULP#  RID Info #1           */
#define HFCFX_RAMI_WS_OF_RID2	0x000000D8	/* WS ULP#  RID Info #2           */
/*---- Offset Address of LEVEL_WK --------------------------------------------*/
#define HFCFX_RAMI_WS_OF_LWKT	0x00000000	/* WS LEVEL WK Area Top           */
#define HFCFX_RAMI_WS_OF_LWKI	0x00000090	/* WS LEVEL WK Info               */
/*---- Offset Address of Common Info -----------------------------------------*/
#define HFCFX_RAMI_WS_OF_CMNI	0x00000000	/* WS Common Info                 */
#define HFCFX_RAMI_WS_OF_CMN1	0x00000000	/* WS Common Info #1              */
#define HFCFX_RAMI_WS_OF_CMN2	0x00000060	/* WS Common Info #2              */
#define HFCFX_RAMI_WS_OF_CMN3	0x000001D0	/* WS Common Info #3              */
#define HFCFX_RAMI_WS_OF_FTRCCA	0x00000100	/* Current Address of Frame Trace */
#define HFCFX_RAMI_WS_OF_ETRCCA	0x00000108	/* Current Address of Event Trace */
#define HFCFX_RAMI_WS_OF_OFF
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_WS_ULPI(U)	(HFCFX_RAMI_WS_OF_ULPI   + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_CCAI(U)	(HFCFX_RAMI_WS_OF_CCAI   + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_RIDI(U)	(HFCFX_RAMI_WS_OF_RIDI   + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_RID1(U)	(HFCFX_RAMI_WS_OF_RID1   + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_RID2(U)	(HFCFX_RAMI_WS_OF_RID2   + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_LWKT(L)	(HFCFX_RAMI_WS_OF_LWKT   + HFCFX_RAMI_WS_LW + (L)*HFCFX_RAMI_WS_LS)
#define HFCFX_RAMI_WS_LWKI(L)	(HFCFX_RAMI_WS_OF_LWKI   + HFCFX_RAMI_WS_LW + (L)*HFCFX_RAMI_WS_LS)
#define HFCFX_RAMI_WS_CMNI		(HFCFX_RAMI_WS_OF_CMNI   + HFCFX_RAMI_WS_CM)
#define HFCFX_RAMI_WS_CMN1		(HFCFX_RAMI_WS_OF_CMN1   + HFCFX_RAMI_WS_CM)
#define HFCFX_RAMI_WS_CMN2		(HFCFX_RAMI_WS_OF_CMN2   + HFCFX_RAMI_WS_CM)
#define HFCFX_RAMI_WS_CMN3		(HFCFX_RAMI_WS_OF_CMN3   + HFCFX_RAMI_WS_CM)
#define HFCFX_RAMI_WS_ULP(R, U)	(HFCFX_RAMI_WS_OF_##R + HFCFX_RAMI_WS_UI + (U)*HFCFX_RAMI_WS_US)
#define HFCFX_RAMI_WS_LEV(R, L)	(HFCFX_RAMI_WS_OF_##R + HFCFX_RAMI_WS_LW + (L)*HFCFX_RAMI_WS_LS)
#define HFCFX_RAMI_WS_CMN(R)	(HFCFX_RAMI_WS_OF_##R + HFCFX_RAMI_WS_CM)
/*---- WS Address Interface --------------------------------------------------*/
#define HFCFX_RAMI_WS_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_WS(R)		(HFCFX_RAMI_WS_##R)
#define HFCFX_RADR_WS(R)		(HFCFX_MMIO_RAMADR_ECWS + HFCFX_RAMI_WS(R))
/*==== GRS ===================================================================*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_GRS_OF_TOP	0x00000000	/* Top Address of GRS             */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_GRS_TOP		(HFCFX_RAMI_GRS_OF_TOP)
/*---- GRS Address Interface -------------------------------------------------*/
#define HFCFX_RAMI_GRS_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_GRS(R)		(HFCFX_RAMI_GRS_##R)
#define HFCFX_RADR_GRS(R)		(HFCFX_MMIO_RAMADR_ECGRS + HFCFX_RAMI_GRS(R))
/*==== LS ====================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_LS_FRM_US	0x00000100	/* ULP Shift for FRAME TRACE(IOV) */
#define HFCFX_RAMI_LS_LPR_US	0x00000200	/* ULP Shift for LPAR TRACE       */
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_LS_OF_FRMT	0x00026000	/* Top Address of FRAME Trace(IOV)*/
#define HFCFX_RAMI_LS_OF_CSCSID	0x00029000	/* Canncel SCSI Info(Driver)      */
#define HFCFX_RAMI_LS_OF_CSCSII	0x00029400	/* Canncel SCSI Info(Interrupt)   */
#define HFCFX_RAMI_LS_OF_LPRT	0x0009C000	/* Top Address of LPAR Trace      */
#define HFCFX_RAMI_LS_OF_OFF
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_LS_FRMT(U)	(HFCFX_RAMI_LS_OF_FRMT + (U)*HFCFX_RAMI_LS_FRM_US)
#define HFCFX_RAMI_LS_CSCSID	(HFCFX_RAMI_LS_OF_CSCSID)
#define HFCFX_RAMI_LS_CSCSII	(HFCFX_RAMI_LS_OF_CSCSII)
#define HFCFX_RAMI_LS_LPRT(U)	(HFCFX_RAMI_LS_OF_LPRT + (U)*HFCFX_RAMI_LS_LPR_US)
#define HFCFX_RAMI_LS_ULPF(R, U)	(HFCFX_RAMI_LS_OF_##R + (U)*HFCFX_RAMI_LS_FRM_US)
#define HFCFX_RAMI_LS_ULPL(R, U)	(HFCFX_RAMI_LS_OF_##R + (U)*HFCFX_RAMI_LS_LPR_US)
/*---- LS Address Interface --------------------------------------------------*/
#define HFCFX_RAMI_LS_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_LS(R)		(HFCFX_RAMI_LS_##R)
#define HFCFX_RADR_LS(R)		(HFCFX_MMIO_RAMADR_ECLS + HFCFX_RAMI_LS(R))
/*==== CCR ===================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_CCR_CS		0x00000400	/* CORE Shift Address             */
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_CCR_OS_TOP				/* Top Address of CCR             */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_CCR_TOP(C)	(HFCFX_RAMI_CCR_OS_TOP + (C)*HFCFX_RAMI_CCR_CS)
/*---- CCR Address Interface -------------------------------------------------*/
#define HFCFX_RAMI_CCR_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_CCR(R)		(HFCFX_RAMI_CCR_##R)
#define HFCFX_RADR_CCR(R)		(HFCFX_MMIO_RAMADR_MKCCR + HFCFX_RAMI_CCR(R))
/*==== ZCTR ==================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_ZCTR_OF_TOP	0x00000000	/* Top Address of ZCTR            */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_ZCTR_TOP		(HFCFX_RAMI_ZCTR_OF_TOP)
/*---- ZCTR Address Interface ------------------------------------------------*/
#define HFCFX_RAMI_ZCTR_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_ZCTR(R)		(HFCFX_RAMI_ZCTR_##R)
#define HFCFX_RADR_ZCTR(R)		(HFCFX_MMIO_RAMADR_ZCTR + HFCFX_RAMI_ZCTR(R))
/*==== ZKTR ==================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_ZKTR_OF_TOP	0x00000000	/* Top Address of ZCTR            */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_ZKTR_TOP		(HFCFX_RAMI_ZKTR_OF_TOP)
/*---- ZKTR Address Interface ------------------------------------------------*/
#define HFCFX_RAMI_ZKTR_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_ZKTR(R)		(HFCFX_RAMI_ZKTR_##R)
#define HFCFX_RADR_ZKTR(R)		(HFCFX_MMIO_RAMADR_ZKTR + HFCFX_RAMI_ZKTR(R))
/*==== BCR ===================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_BCR_OF_TOP	0x00000000	/* Top Address of BCR             */
#define HFCFX_RAMI_BCR_OF_LSA	0x00000004  /* Logging Start Address          */ 
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_BCR_TOP		(HFCFX_RAMI_BCR_OF_TOP)
#define HFCFX_RAMI_BCR_LSA		(HFCFX_RAMI_BCR_OF_LSA)
/*---- BCR Address Interface -------------------------------------------------*/
#define HFCFX_RAMI_BCR_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_BCR(R)		(HFCFX_RAMI_BCR_##R)
#define HFCFX_RADR_BCR(R)		(HFCFX_MMIO_RAMADR_BCBCR + HFCFX_RAMI_BCR(R))
/*==== FLASH =================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_FLS_OF_TOP	0x00000000	/* Top Address of FLUSH           */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_FLS_TOP		(HFCFX_RAMI_FLS_OF_TOP)
/*---- FLASH Address Interface -----------------------------------------------*/
#define HFCFX_RAMI_FLS_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_FLS(R)		(HFCFX_RAMI_FLS_##R)
#define HFCFX_RADR_FLS(R)		(HFCFX_MMIO_RAMADR_FLASH + HFCFX_RAMI_FLS(R))
/*==== DUMP ==================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_DMP_OF_TOP	0x00000000	/* Top Address of DUMP            */
#define HFCFX_RAMI_DMP_OF_AREA1	0x00000000	/* Log Area1 Address of DUMP      */
#define HFCFX_RAMI_DMP_OF_AREA2	0x00000100	/* Log Area1 Address of DUMP      */
#define HFCFX_RAMI_DMP_OF_AREA3	0x00000140	/* Log Area1 Address of DUMP      */
#define HFCFX_RAMI_DMP_OF_AREA4	0x00000200	/* Log Area1 Address of DUMP      */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_DMP_TOP		(HFCFX_RAMI_DMP_OF_TOP)
#define HFCFX_RAMI_DMP_AREA1	(HFCFX_RAMI_DMP_OF_AREA1)
#define HFCFX_RAMI_DMP_AREA2	(HFCFX_RAMI_DMP_OF_AREA2)
#define HFCFX_RAMI_DMP_AREA3	(HFCFX_RAMI_DMP_OF_AREA3)
#define HFCFX_RAMI_DMP_AREA4	(HFCFX_RAMI_DMP_OF_AREA4)
/*---- DUMP Address Interface ------------------------------------------------*/
#define HFCFX_RAMI_DMP_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_DMP(R)		(HFCFX_RAMI_DMP_##R)
#define HFCFX_RADR_DMP(R)		(HFCFX_MMIO_RAMADR_DUMP + HFCFX_RAMI_DMP(R))

/* FCLNX-GPL-FX-145 */
/*==== UTL ===================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
/*---- Offset Address --------------------------------------------------------*/
#define HFCFX_RAMI_UTL_OF_BSTAT	0x000000BC	/* EndPoint/Upstearm Bridge Status*/
#define HFCFX_RAMI_UTL_OF_PSTAT	0x000000A4	/* PCIe Port Status               */
#define HFCFX_RAMI_UTL_OF_INTEN	0x000000AC	/* PCIe Port Interrupt Enable     */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_UTL_BSTATUS	(HFCFX_RAMI_UTL_OF_BSTAT)
#define HFCFX_RAMI_UTL_PSTATUS	(HFCFX_RAMI_UTL_OF_PSTAT)
#define HFCFX_RAMI_UTL_INTEN	(HFCFX_RAMI_UTL_OF_INTEN)
/*---- DUMP Address Interface ------------------------------------------------*/
#define HFCFX_RAMI_UTL_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_UTL(R)		(HFCFX_RAMI_UTL_##R)
#define HFCFX_RADR_UTL(R)		(HFCFX_MMIO_RAMADR_UTL + HFCFX_RAMI_UTL(R))
/* FCLNX-GPL-FX-145 */

/*==== Trace Area Address ======================================================
 *    RAMADDR = HFCFX_RADR(<TraceType>(<MacroType>[(MacroParameter)]))
 *    ex.(FW Event trace, Entry# = EntryNum)
 *      CR = HFCFX_RAMI_TRACE_CR(FWEVTR, Current)
 *      WN = HFCFX_RAMI_TRACE_WN(FWEVTR, EntryNum)
 *      ram_addr = HFCFX_RADR(FWEVTR(LA(CR, EW)))
 *
 *      Trace Type
 *        FRMTR		: FRAME Trace		(1 Word)
 *        FWEVTR	: FW Event Trace	(8 Word)
 *        FWFRTR	: FW Frame Trace	(4 Word)
 *        FWBRTR	: FW Branch Trace	(1 Word)
 *        PCIPTR	: PCIe Packet Trace	(4 Word)
 *        RUTR		: RU Trace			(4 Word)
 *      Macro Type
 *        SA        : Start D-Word Pointer of Trace Area(H/W Interface Specific)
 *        EA        : End D-Word Pointer of Trace Area(H/W Interface Specific)
 *        AS        : D-Word Size of Trace Area
 *        CS        : Step Count of Current Pointer
 *        EW		: Event Entry D-Word Size
 *        MK        : Offset D-Word Address Mask of Trace Area(Use for Rap around)
 *        CR(TY,CA) : D-Word Size of Current D-Word Pointer
 *        WN(TY,EN) : D-Word Number of Trace Entry
 *        LA(CR,WN) : Logging Byte Address (Start Pointer Shift)
 *        OFF		: Decupcel Macro(Use for Address Direct Set)
 *      Macro Parameter
 *       (CA)       : Current Trace Address
 *       (EN)       : Logging Trace Entry Number  
 *       (CR)       : Current Trace D-Word Pointer
 *       (WN)       : Logging Trace D-Word Number
 *============================================================================*/
/*==== Trace Common Deifne ===================================================*/
#define HFCFX_RAMI_TRACE_WS		0x00000004
#define HFCFX_RAMI_TRACE_CR(TY, CR)	((CR)*(HFCFX_RAMI_##TY##_CS))
#define HFCFX_RAMI_TRACE_WN(TY, EN)	(((EN) - 1) * HFCFX_RAMI_##TY##_EW)
#define HFCFX_RAMI_TRACE_LA(TY, CR, WN)		\
	((((((CR) - HFCFX_RAMI_##TY##_SA) + (WN)) & HFCFX_RAMI_##TY##_MK) \
	+ HFCFX_RAMI_##TY##_SA) \
	* HFCFX_RAMI_TRACE_WS)
/*==== FRAME Trace =============================LS[DW:0x00009800 - 0x00009FFF]*/
#define HFCFX_RAMI_FRMTR_SA	0x00009800
#define HFCFX_RAMI_FRMTR_EA	0x00009FFF
#define HFCFX_RAMI_FRMTR_AS	0x00000800
#define HFCFX_RAMI_FRMTR_CS	0x00000001
#define HFCFX_RAMI_FRMTR_EW	0x00000001
#define HFCFX_RAMI_FRMTR_MK	(HFCFX_RAMI_FRMTR_AS - 1)
#define HFCFX_RAMI_FRMTR_WN(EN)		(HFCFX_RAMI_TRACE_WN(FRMTR, EN))
#define HFCFX_RAMI_FRMTR_CR(CA)		(HFCFX_RAMI_TRACE_CR(FRMTR, CA))
#define HFCFX_RAMI_FRMTR_LA(CR, WN)	(HFCFX_RAMI_TRACE_LA(FRMTR, CR, WN))
#define HFCFX_RAMI_FRMTR_OFF
#define HFCFX_RAMI_FRMTR(R)		(HFCFX_RAMI_FRMTR_##R)
#define HFCFX_RADR_FRMTR(R)		(HFCFX_MMIO_RAMADR_ECLS + HFCFX_RAMI_FRMTR(R))
/*==== FW Event Trace ==========================LS[DW:0x00024000 - 0x00025FFF]*/
#define HFCFX_RAMI_FWEVTR_SA	0x00024000
#define HFCFX_RAMI_FWEVTR_EA	0x00025FFF
#define HFCFX_RAMI_FWEVTR_AS	0x00002000
#define HFCFX_RAMI_FWEVTR_CS	0x00000001
#define HFCFX_RAMI_FWEVTR_EW	0x00000008
#define HFCFX_RAMI_FWEVTR_MK	(HFCFX_RAMI_FWEVTR_AS - 1)
#define HFCFX_RAMI_FWEVTR_WN(EN)		(HFCFX_RAMI_TRACE_WN(FWEVTR, EN))
#define HFCFX_RAMI_FWEVTR_CR(CA)		(HFCFX_RAMI_TRACE_CR(FWEVTR, CA))
#define HFCFX_RAMI_FWEVTR_LA(CR, WN)	(HFCFX_RAMI_TRACE_LA(FWEVTR, CR, WN))
#define HFCFX_RAMI_FWEVTR_OFF
#define HFCFX_RAMI_FWEVTR(R)	(HFCFX_RAMI_FWEVTR_##R)
#define HFCFX_RADR_FWEVTR(R)	(HFCFX_MMIO_RAMADR_ECLS + HFCFX_RAMI_FWEVTR(R))
/*==== FW Frame Trace ==========================LS[DW:0x00026000 - 0x00026FFF]*/
#define HFCFX_RAMI_FWFRTR_SA	0x00026000
#define HFCFX_RAMI_FWFRTR_EA	0x00026FFF
#define HFCFX_RAMI_FWFRTR_AS	0x00001000
#define HFCFX_RAMI_FWFRTR_CS	0x00000001
#define HFCFX_RAMI_FWFRTR_EW	0x00000004
#define HFCFX_RAMI_FWFRTR_MK	(HFCFX_RAMI_FWFRTR_AS - 1)
#define HFCFX_RAMI_FWFRTR_WN(EN)		(HFCFX_RAMI_TRACE_WN(FWFRTR, EN))
#define HFCFX_RAMI_FWFRTR_CR(CA)		(HFCFX_RAMI_TRACE_CR(FWFRTR, CA))
#define HFCFX_RAMI_FWFRTR_LA(CR, WN)	(HFCFX_RAMI_TRACE_LA(FWFRTR, CR, WN))
#define HFCFX_RAMI_FWFRTR_OFF
#define HFCFX_RAMI_FWFRTR(R)	(HFCFX_RAMI_FWFRTR_##R)
#define HFCFX_RADR_FWFRTR(R)	(HFCFX_MMIO_RAMADR_ECLS + HFCFX_RAMI_FWFRTR(R))
/*==== FW Branch Trace =======================ZCTR[DW:0x00000000 - 0x000007FF]*/
#define HFCFX_RAMI_FWBRTR_SA	0x00000000
#define HFCFX_RAMI_FWBRTR_EA	0x000007FF
#define HFCFX_RAMI_FWBRTR_AS	0x00000800
#define HFCFX_RAMI_FWBRTR_CS	0x00000002
#define HFCFX_RAMI_FWBRTR_EW	0x00000001
#define HFCFX_RAMI_FWBRTR_MK	(HFCFX_RAMI_FWBRTR_AS - 1)
#define HFCFX_RAMI_FWBRTR_WN(EN)		(HFCFX_RAMI_TRACE_WN(FWBRTR, EN))
#define HFCFX_RAMI_FWBRTR_CR(CA)		(HFCFX_RAMI_TRACE_CR(FWBRTR, CA))
#define HFCFX_RAMI_FWBRTR_LA(CR, WN)	(HFCFX_RAMI_TRACE_LA(FWBRTR, CR, WN))
#define HFCFX_RAMI_FWBRTR_OFF
#define HFCFX_RAMI_FWBRTR(R)	(HFCFX_RAMI_FWBRTR_##R)
#define HFCFX_RADR_FWBRTR(R)	(HFCFX_MMIO_RAMADR_ZCTR + HFCFX_RAMI_FWBRTR(R))
/*==== PCIe Packet Trace =====================ZKTR[DW:0x00000000 - 0x000007FF]*/
#define HFCFX_RAMI_PCIPTR_SA	0x00000000
#define HFCFX_RAMI_PCIPTR_EA	0x000007FF
#define HFCFX_RAMI_PCIPTR_AS	0x00000800
#define HFCFX_RAMI_PCIPTR_CS	0x00000004
#define HFCFX_RAMI_PCIPTR_EW	0x00000004
#define HFCFX_RAMI_PCIPTR_MK	(HFCFX_RAMI_PCIPTR_AS - 1)
#define HFCFX_RAMI_PCIPTR_WN(EN)		(HFCFX_RAMI_TRACE_WN(PCIPTR, EN))
#define HFCFX_RAMI_PCIPTR_CR(CA)		(HFCFX_RAMI_TRACE_CR(PCIPTR, CA))
#define HFCFX_RAMI_PCIPTR_LA(CR, WN)	(HFCFX_RAMI_TRACE_LA(PCIPTR, CR, WN))
#define HFCFX_RAMI_PCIPTR_OFF
#define HFCFX_RAMI_PCIPTR(R)	(HFCFX_RAMI_PCIPTR_##R)
#define HFCFX_RADR_PCIPTR(R)	(HFCFX_MMIO_RAMADR_ZKTR + HFCFX_RAMI_PCIPTR(R))
/*==== RU Trace ==============================ZKTR[DW:0x00000800 - 0x00000FFF]*/
#define HFCFX_RAMI_RUTR_SA		0x00000800
#define HFCFX_RAMI_RUTR_EA		0x00000FFF
#define HFCFX_RAMI_RUTR_AS		0x00000800
#define HFCFX_RAMI_RUTR_CS		0x00000004
#define HFCFX_RAMI_RUTR_EW		0x00000004
#define HFCFX_RAMI_RUTR_MK		(HFCFX_RAMI_PCIPTR_AS - 1)
#define HFCFX_RAMI_RUTR_WN(EN)			(HFCFX_RAMI_TRACE_WN(RUTR, EN))
#define HFCFX_RAMI_RUTR_CR(CA)			(HFCFX_RAMI_TRACE_CR(RUTR, CA))
#define HFCFX_RAMI_RUTR_LA(CR, WN)		(HFCFX_RAMI_TRACE_LA(RUTR, CR, WN))
#define HFCFX_RAMI_RUTR_OFF
#define HFCFX_RAMI_RUTR(R)		(HFCFX_RAMI_RUTR_##R)
#define HFCFX_RADR_RUTR(R)		(HFCFX_MMIO_RAMADR_ZKTR + HFCFX_RAMI_RUTR(R))
/*==== RAMADDR SET Interface =================================================*/
#define HFCFX_RAMI_OFF
#define HFCFX_RADR_OFF
#define HFCFX_RAMI(R)			(HFCFX_RAMI_##R) 
#define HFCFX_RADR(R)	\
	((uint)((HFCFX_RADR_##R & HFCFX_MMIO_RAMADR_ADR) | HFCFX_MMIO_RAMADR_MODELI)) 

/*==============================================================================
 *    RAM Indirect Access (Indirect SCAN-MAP)
 *         Offset
 *       + N x 0x2000    ---- Core Shift
 *============================================================================*/
/*==== IDREG =================================================================*/
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_ID_CS		0x00002000	/* CORE Shift                     */
#define HFCFX_RAMI_ID_FR_US		0x00000004	/* EU FR(FRiP/FRoP) ULP Shift     */
/*---- Offset Address of  EU -------------------------------------------------*/
#define HFCFX_RAMI_ID_OS_FR		0x00001000	/* EU ULP# FR(FRiP/FRoP)          */
/*---- Shift Address ---------------------------------------------------------*/
#define HFCFX_RAMI_ID_FR(C,U)	(HFCFX_RAMI_ID_OS_FR + (C)*HFCFX_RAMI_ID_CS + (U)*HFCFX_RAMI_ID_FR_US)
/*---- IDREG Address Interface -----------------------------------------------*/
#define HFCFX_RAMI_ID_OFF					/* Decupcel (Direct Address Set)  */
#define HFCFX_RAMI_ID(R)		(HFCFX_RAMI_ID_##R)
#define HFCFX_RADR_ID(R)		(HFCFX_MMIO_RAMADR_IDREG + HFCFX_RAMI_ID(R))
//<<<<<FCWIN_FX-018

/*==============================================================================
 *    MMIO BAR1(MSIX Table)
 *============================================================================*/
/*==============================================================================
 *    Access Macros (MMIO BAR1 - MSIX Table)
 *============================================================================*/
/*==============================================================================
 *    MMIO BAR2(MMIO-HG)
 *============================================================================*/
#define HFCFX_MMHG(REG)			HFCFX_MMHG_##REG
#define HFCFX_MMHG_OFF						/*  Disappearance HFCX_MMHG()     */
/*==== Hyper INT Registers ===================================================*/
/*---- Port/Core Register Block Area -----------------------------------------*/
#define HFCFX_MMHG_PORTA			0x0000	/* Port Block Reg Area            */
#define HFCFX_MMIO_COREA			0x0400	/* Core Block Reg Area            */
/*---- Port Block Registers---------------------------------------------------*/
#define HFCFX_MMHG_OF_HYPSTS		0x0008
#define HFCFX_MMHG_OF_HYPCMD		0x000C
#define HFCFX_MMHG_OF_HYPINT		0x002C
/*---- Core Block Registers---------------------------------------------------*/
#define HFCFX_MMHG_COREBASE		0x0400
#define HFCFX_MMHG_CORESHIFT	0x0040
#define HFCFX_MMHG_CS_HYPSTS(C)	(HFCFX_MMHG_OF_HYPSTS + (C)*HFCFX_MMHG_CORESHIFT)	
#define HFCFX_MMHG_CS_HYPCMD(C)	(HFCFX_MMHG_OF_HYPCMD + (C)*HFCFX_MMHG_CORESHIFT)
#define HFCFX_MMHG_CS_HYPINT(C)	(HFCFX_MMHG_OF_HYPINT + (C)*HFCFX_MMHG_CORESHIFT) 
/*---- Block Area Shift Address ----------------------------------------------*/
#define HFCFX_MMHG_PORT(R)		(HFCFX_MMHG_OF_##R)
#define HFCFX_MMHG_CORE(R)		(HFCFX_MMHG_COREBASE + HFCFX_MMHG_CS_##R)
/*---- Bit Define for HyperIntDetail -----------------------------------------*/
#define HFCFX_MMHG_HYPINT(BIT)			HFCFX_MMHG_HYPINT_##BIT
#define HFCFX_MMHG_HYPINT_FMCK			0x80000000
#define HFCFX_MMHG_HYPINT_FCSTP			0x40000000
#define HFCFX_MMHG_HYPINT_MCK			0x20000000
#define HFCFX_MMHG_HYPINT_MCKEND		0x10000000
#define HFCFX_MMHG_HYPINT_CSTPEND		0x08000000
#define HFCFX_MMHG_HYPINT_LINKEND		0x04000000
#define HFCFX_MMHG_HYPINT_FISOLERR		0x00800000
#define HFCFX_MMHG_HYPINT_FISOLCMD		0x00400000
#define HFCFX_MMHG_HYPINT_FISOLEND		0x00200000
#define HFCFX_MMHG_HYPINT_RCVISOL		0x00100000
#define HFCFX_MMHG_HYPINT_RCVISOLEND	0x00080000
#define HFCFX_MMHG_HYPINT_MIGEND		0x00040000
#define HFCFX_MMHG_HYPINT_MIGRCV		0x00020000
#define HFCFX_MMHG_HYPINT_PFACTOR		(HFCFX_MMHG_HYPINT_MCKEND\
										|HFCFX_MMHG_HYPINT_LINKEND\
										|HFCFX_MMHG_HYPINT_FISOLERR\
										|HFCFX_MMHG_HYPINT_FISOLCMD\
										|HFCFX_MMHG_HYPINT_FISOLEND\
										|HFCFX_MMHG_HYPINT_RCVISOL\
										|HFCFX_MMHG_HYPINT_RCVISOLEND\
										|HFCFX_MMHG_HYPINT_MIGEND\
										|HFCFX_MMHG_HYPINT_MIGRCV)
#define HFCFX_MMHG_HYPINT_CFACTOR		(HFCFX_MMHG_HYPINT_FMCK\
										|HFCFX_MMHG_HYPINT_FCSTP\
										|HFCFX_MMHG_HYPINT_MCK\
										|HFCFX_MMHG_HYPINT_CSTPEND)
/*==============================================================================
 *    Access Macros (MMIO BAR2 - MMIO-HG)
 *============================================================================*/
#define HFCFX_MMHG_R4(P,R)		\
	((uint) hfc_fx_read_hg_reg_ext((P), HFCFX_MMHG(R), 0x04)) 
#define HFCFX_MMHG_R2(P,R)		\
	((ushort)hfc_fx_read_hg_reg_ext((P), HFCFX_MMHG(R), 0x02))
#define HFCFX_MMHG_R1(P,R)		\
	((uchar) hfc_fx_read_hg_reg_ext((P), HFCFX_MMHG(R), 0x01))
#define HFCFX_MMHG_W4(P,R,D)	\
	hfc_fx_write_hg_reg_ext((P), HFCFX_MMHG(R), 0x04, (uint)D) 
#define HFCFX_MMHG_W2(P,R,D)	\
	hfc_fx_write_hg_reg_ext((P), HFCFX_MMHG(R), 0x02, (ushort)D)
#define HFCFX_MMHG_W1(P,R,D)	\
	hfc_fx_write_hg_reg_ext((P), HFCFX_MMHG(R), 0x01, (uchar)D)
/*====================================*//*====================================*/

/*-----------------------------------------------------------------*/
/*                         trace format                            */
/* The member name of each structure uses the following prefixes   */
/*                                                                 */
/* a_   : Value acquired from member of adap_info                  */
/* t_   : Value acquired from member of target_info_fx                */
/* s_   : Value acquired from member of Srb                        */
/* h_   : Value acquired from member of hfc_pkt                    */
/*                                                                 */
/*-----------------------------------------------------------------*/
#if 0
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
#endif

typedef struct mem_type_fx {
	uchar	type ;
	uint	reg_adr ;
//#define FX_ETRC 0x24000
#define FX_ETRC 0xa400
//#define FX_FTRC 0x26000
#define FX_FTRC 0xa800
	uint	size ;
	uint	func_core;
	/* 0xPLLL                  Pcore#   Valid Flag
                               pcore_no 4cores  2cores  1core 
                                        FF      FF      FFFF
                                        0*      01      0123 */
#define FXLOG_AC 0x0ff0ffff /* 0x0f     0xf0    0xff    0xff */
#define FXLOG_C0 0x00808080 /* 0x00     0x80    0x80    0x80 */
#define FXLOG_C1 0x01900820 /* 0x01     0x90    0x08    0x20 */
#define FXLOG_C2 0x02a0a008 /* 0x02     0xa0    0xa0    0x08 */
#define FXLOG_C3 0x03b00a02 /* 0x03     0xb0    0x0a    0x02 *//* FCLNX-GPL-FX-098 */
}Type_mem_fx ;

#define FXLOG_CORE(C) \
	(((((C) & HFC_INTE_CORE_SHR) >> 7) * FXLOG_AC)	\
	|(((uint)((C) & HFC_INTE_CORE_MSK)) << 24)	\
	|(((uint)(((C) & HFC_INTE_CORE_MSK) | 0x08)) << 20)	\
	|(((uint)(((C) & 0x02) | 0x08)) << (12 - (((C) & 0x01) * 4)))	\
	|(0x00000080 >> (((C) & HFC_INTE_CORE_MSK) * 2)))
#define FXLOG_FUNC(I, N)	\
	((((0x00800000 << ((N) & 0x03)) & 0x00FF0000)	\
	 |((0x00008000 >> (((N) & 0x03) * 4)) & 0x0000FF00)	\
	 | (0x00000080 >> (((N) & 0x03) * 2)))	\
	&((0x00FF0000) >> (((I)/2) * 8)))
#define FXLOG_PCORE(F)	((uchar)(((F) & 0xFF000000) >> 24))
/*==== Log Data type =========================================================*/
#define	HLOG_ERRID		0x01			/* Error ID                           */
#define	HLOG_RSV		0x02			/* Reserved Location                  */
#define	HLOG_UICORE		0x03			/* Un Installed  Core                 */
#define	HLOG_TIMESTAMP	0x04			/* Time Stamp                         */
/*---- PCI Config Access -----------------------------------------------------*/
#define	HLOG_CFG		0x10			/* PCI Config Access                  */
/*---- PCI Memory Direct Access ----------------------------------------------*/
#define	HLOG_PCI		0x20			/* PCI Memory  Direct Read            */
#define	HLOG_CHK		0x21			/*   + Checker Area Select            */
/*---- RAM Indirect Access ---------------------------------------------------*/
#define	HLOG_WS			0x30			/* WS   Area Read                     */
#define	HLOG_WSULP		0x31			/*   + "ULP" Shift                    */
#define	HLOG_WSLEVEL	0x32			/*   + "LEVEL" Shift                  */
#define	HLOG_WSCMN		0x33			/*   + "COMMON" Area                  */
#define	HLOG_GRS		0x40			/* GRS  Area Read                     */
#define	HLOG_LS			0x50			/* LS   Area Read                     */
#define	HLOG_LSULPF		0x51			/*   + "ULP"(for Frame) Shift Macro   */
#define	HLOG_LSULPL		0x52			/*   + "ULP"(for LPAR)  Shift Macro   */
#define	HLOG_CCR		0x60			/* CCR  Area Read                     */
#define	HLOG_ZCTR		0x70			/* ZCTR Area Read                     */
#define	HLOG_ZKTR		0x80			/* ZKTR Area Read                     */
#define	HLOG_BCR		0x90			/* BCR  Area Read                     */
#define	HLOG_FLASH		0xA0			/* FLASH Area Read                    */
#define	HLOG_DUMP		0xB0			/* DUMP AREA Read                     */
#define	HLOG_IDREG		0xC0			/* IDREG Area Read(Indirect SCAN Area)*/
/*---- Trace Access ----------------------------------------------------------*/
#define HLOG_FRMTR		0x53			/* FRAME Trace(LS Area)               */
#define	HLOG_FWEVTR		0x54			/* FW Event Trace(LS Area)            */
#define	HLOG_FWFRTR		0x55			/* FW Frame Trace(LS Area)            */
#define	HLOG_FWBRTR		0x71			/* FW Branch Trace(ZCTR Area)         */
#define	HLOG_PCIPTR		0x81			/* PCIe Packet Trace(ZKTR Area)       */
#define	HLOG_RUTR		0x82			/* RU Trace(ZKTR Area)                */
/*---- END MARK --------------------------------------------------------------*/
#define	HLOG_ENDMARK	0x00			/* END of Format                      */

/*==== Log Data Format Set Macro =============================================*/
#define HFCFX_HL_ERRID				{HLOG_ERRID,	0x00000000,				0x00000004,	FXLOG_AC}
#define HFCFX_HL_RESERVE(S)			{HLOG_RSV,		0x00000000,				(uint)(S),	FXLOG_AC}
#define HFCFX_HL_TIMESTAMP(S)		{HLOG_TIMESTAMP,0x00000000,				(uint)(S),	FXLOG_AC}
/*---- PCI Config Access -----------------------------------------------------*/
#define HFCFX_HL_CFG(R, S)			{HLOG_CFG,		HFCFX_PCI(R),			(uint)(S),	FXLOG_AC}
/*---- PCI Memory Direct Access ----------------------------------------------*/
#define HFCFX_HL_PCI(C, R, S)		{HLOG_PCI,		HFCFX_MMIO(R),			(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_CHK(R, S)			{HLOG_CHK,		HFCFX_MMIO(R),			(uint)(S),	FXLOG_AC}
/*---- RAM Indirect Access ---------------------------------------------------*/
#define HFCFX_HL_WS(C, R, S) 		{HLOG_WS,		HFCFX_RAMI(WS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_WSULP(C, R, S)		{HLOG_WSULP,	HFCFX_RAMI(WS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_WSLEV(C, R, S)		{HLOG_WSLEVEL,	HFCFX_RAMI(WS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_WSCMN(C, R, S)		{HLOG_WSCMN,	HFCFX_RAMI(WS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_GRS(C, R, S)		{HLOG_GRS,		HFCFX_RAMI(GRS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_LS(C, R, S)		{HLOG_LS,		HFCFX_RAMI(LS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_LSULPF(C, R, S)	{HLOG_LSULPF,	HFCFX_RAMI(LS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_LSULPL(C, R, S)	{HLOG_LSULPL,	HFCFX_RAMI(LS(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_CCR(R, S)			{HLOG_CCR,		HFCFX_RAMI(CCR(R)),		(uint)(S),	FXLOG_AC}
#define HFCFX_HL_ZCTR(C, R, S)		{HLOG_ZCTR,		HFCFX_RAMI(ZCTR(R)),	(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_ZKTR(R, S)			{HLOG_ZCTR,		HFCFX_RAMI(ZKTR(R)),	(uint)(S),	FXLOG_AC}
#define HFCFX_HL_BCR(C, R, S)		{HLOG_BCR,		HFCFX_RAMI(BCR(R)),		(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_FLASH(R, S)		{HLOG_FLASH,	HFCFX_RAMI(FLS(R)),		(uint)(S),	FXLOG_AC}
#define HFCFX_HL_DUMP(R, S)			{HLOG_DUMP,		HFCFX_RAMI(DMP(R)),		(uint)(S),	FXLOG_AC}	
#define HFCFX_HL_IDREG(C, R, S)		{HLOG_IDREG,	HFCFX_RAMI(ID(R)),		(uint)(S),	FXLOG_CORE(C)}
/*---- Trace Access ----------------------------------------------------------*/
#define HFCFX_HL_FRMT(C, EN, S)		{HLOG_FRMTR,	((uint)HFCFX_RAMI_TRACE_WN(FRMTR,	EN)),	(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_FWEVT(C, EN, S)	{HLOG_FWEVTR,	((uint)HFCFX_RAMI_TRACE_WN(FWEVTR,	EN)),	(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_FWFRT(C, EN, S)	{HLOG_FWFRTR,	((uint)HFCFX_RAMI_TRACE_WN(FWFRTR,	EN)),	(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_FWBRT(C, EN, S)	{HLOG_FWBRTR,	((uint)HFCFX_RAMI_TRACE_WN(FWBRTR,	EN)),	(uint)(S),	FXLOG_CORE(C)}
#define HFCFX_HL_PCIPT(EN, S)		{HLOG_PCIPTR,	((uint)HFCFX_RAMI_TRACE_WN(PCIPTR,	EN)),	(uint)(S),	FXLOG_AC}
#define HFCFX_HL_RUT(EN, S)			{HLOG_RUTR,		((uint)HFCFX_RAMI_TRACE_WN(RUTR,	EN)),	(uint)(S),	FXLOG_AC}
#define HFCFX_HL_ENDMARK			{HLOG_ENDMARK,	(uint)0,									(uint)0,	(uint)0}
/*---- Trace Access ----------------------------------------------------------*/
/*---- Systemlog Area CORE Infomation ----------------------------------------*/
#define HFCFX_HL_SYS_COREINFO(C)	\
	HFCFX_HL_PCI((C),	SCAN(GR0((C) & HFC_INTE_CORE_MSK)),	0x0040),\
	HFCFX_HL_PCI((C),	SCAN(RTBR((C) & HFC_INTE_CORE_MSK)),0x0040),\
	HFCFX_HL_FWBRT((C),	2017,								0x0080),\
	HFCFX_HL_WSLEV((C),	OF_LWKI,							0x0070),\
	HFCFX_HL_WS((C),	RID1(0),							0x001C),\
	HFCFX_HL_WS((C),	RID2(0),							0x0004),\
	HFCFX_HL_WSULP((C),	OF_RID1,							0x001C),\
	HFCFX_HL_WSULP((C),	OF_RID2,							0x0004),\
	HFCFX_HL_WSCMN((C),	OF_CMN1,							0x0010),\
	HFCFX_HL_WSCMN((C),	OF_CMN2,							0x0020),\
	HFCFX_HL_WSCMN((C),	OF_CMN3,							0x0010),\
	HFCFX_HL_FWEVT((C),	1019,								0x00C0),\
	HFCFX_HL_FWFRT((C),	1018,								0x0070)
/*---- SCAN Core Resouce ----------------------------------------------------*/
#define HFCFX_HL_SCAN_RSC(C)	\
	HFCFX_HL_ERRID,\
	HFCFX_HL_PCI(0x80,		PTYP0,				0x0004),\
	HFCFX_HL_PCI((C),		SCAN(ULP(C)),		0x0004),\
	HFCFX_HL_PCI((C),		SCAN(CORE(TOD(C))),	0x0004),\
	HFCFX_HL_PCI((C),		SCAN(CKCORE(C)),	0x00F0),\
	HFCFX_HL_GRS((C),		TOP,				0x0200),\
	HFCFX_HL_PCI((C),		SCAN(LBR0(C)),		0x0010),\
	HFCFX_HL_PCI((C),		SCAN(OTHERBR(C)),	0x0020),\
	HFCFX_HL_PCI((C),		SCAN(FTBBSY(C)),	0x0008),\
	HFCFX_HL_PCI((C),		SCAN(STBBSY(C)),	0x0008),\
	HFCFX_HL_PCI((C),		SCAN(RLRA(C)),		0x0004),\
	HFCFX_HL_PCI((C),		SCAN(TLRA(C)),		0x0004),\
	HFCFX_HL_PCI((C),		SCAN(BSTRA(C)),		0x0004),\
	HFCFX_HL_PCI((C),		SCAN(RID(C)),		0x0004),\
	HFCFX_HL_RESERVE(0x0030),\
	HFCFX_HL_PCI((C),		SCAN(FRIPFROP(C)),	0x0004),\
	HFCFX_HL_IDREG((C),		FR((C),1),			0x007C)
/*----  FW Bramch Trace -----------------------------------------------------*/
#define HFCFX_HL_FW_BRANCH_TRC(C)	\
	HFCFX_HL_ERRID,\
	HFCFX_HL_FWBRT((C),	1539,	0x03FC),\
	HFCFX_HL_ERRID,\
	HFCFX_HL_FWBRT((C),	1794,	0x03FC)
/*---- BCR Info -------------------------------------------------------------*/
#define HFCFX_HL_BCR_INFO(C)	\
	HFCFX_HL_ERRID,\
	HFCFX_HL_BCR((C),	LSA,	0x03FC)
/*---- FW WS Info and Trace -------------------------------------------------*/
#define HFCFX_HL_FW_INFO(C)	\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		RIDI(0x00),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x01),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x02),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x03),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x04),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x05),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x06),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x07),								0x0070),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_RIDI(0x07) + 0x0070),	0x0010),\
												HFCFX_HL_WS((C),		RIDI(0x08),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x09),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0A),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0B),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0C),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0D),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0E),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x0F),								0x0060),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_RIDI(0x0F) + 0x0060),	0x0020),\
												HFCFX_HL_WS((C),		RIDI(0x10),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x11),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x12),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x13),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x14),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x15),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x16),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x17),								0x0050),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_RIDI(0x17) + 0x0050),	0x0030),\
												HFCFX_HL_WS((C),		RIDI(0x18),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x19),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1A),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1B),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1C),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1D),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1E),								0x0080),\
												HFCFX_HL_WS((C),		RIDI(0x1F),								0x0040),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_RIDI(0x1F) + 0x0040),	0x0040),\
												HFCFX_HL_WS((C),		LWKT(0x00),								0x03B0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_LWKT(0x03) + 0x00B0),	0x03F0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_LWKT(0x07) + 0x00A0),	0x0060),\
												HFCFX_HL_WS((C),		CMNI,									0x0390),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_WS((C),		OFF(HFCFX_RAMI_WS_CMNI + 0x0390),		0x0070),\
												HFCFX_HL_FRMT((C),		1921,									0x0200),\
												HFCFX_HL_LS((C),		CSCSID,									0x0180),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_LS((C),		OFF(HFCFX_RAMI_LS_CSCSID + 0x0180),		0x0280),\
												HFCFX_HL_LS((C),		CSCSII,									0x0170),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_LS((C),		OFF(HFCFX_RAMI_LS_CSCSII + 0x0170),		0x0290),\
												HFCFX_HL_FWEVT((C),		897,									0x0160),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x001C),	HFCFX_HL_FWEVT((C),		908,									0x03E0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x001C),	HFCFX_HL_FWEVT((C),		939,									0x03E0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x001C),	HFCFX_HL_FWEVT((C),		970,									0x03E0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x001C),	HFCFX_HL_FWEVT((C),		1001,									0x0300),\
												HFCFX_HL_FWFRT((C),		786,									0x00E0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_FWFRT((C),		801,									0x03F0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_FWFRT((C),		864,									0x0330),\
												HFCFX_HL_LS((C),		LPRT(0),								0x00C0),\
	HFCFX_HL_ERRID,	HFCFX_HL_RESERVE(0x000C),	HFCFX_HL_LS((C),		OFF(HFCFX_RAMI_LS_LPRT(0) + 0x00C0),	0x0140),\
												HFCFX_HL_LSULPL((C),	OF_LPRT,								0x0200),\
												HFCFX_HL_RESERVE(0x00B0)
/*==== HW ErrorID ============================================================*/
#define HFCFX_HL_ERRID_MCK		0xFFFE0000	/* MCK                            */
#define HFCFX_HL_ERRID_CHKSTOP	0xFFFE0800	/* Check Stop                     */
#define HFCFX_HL_ERRID_IMLFAIL	0xFFFE1000	/* IML FAIL                       */
#define HFCFX_HL_ERRID_SRAMCE	0xFFFE1100	/* SRAM CE                        */
/*==== Log Size ==============================================================*/
#define HFCFX_HL_SIZE_DATA		0x00000004	/* Byte Size of Word Data(D Word) */
#define HFCFX_HL_SIZE_UNITB		0x00000400	/* Byte Size of Log Unit(1024byte)*/
#define HFCFX_HL_SIZE_UNITW		(HFCFX_HL_SIZE_UNITB / HFCFX_HL_SIZE_DATA)
#define HFCFX_HL_SIZE_USYS		0x00000004	/* Num of Log UNIT for System Log */
#define HFCFX_HL_SIZE_UEXT		0x0000005D	/* Num of Log UNIT for Extend Log */
#define HFCFX_HL_SIZE_SYS		(HFCFX_HL_SIZE_UNITB * HFCFX_HL_SIZE_USYS)
#define HFCFX_HL_SIZE_EXT		(HFCFX_HL_SIZE_UNITB * HFCFX_HL_SIZE_UEXT)
#define HFCFX_HL_SIZE_MCK		HFCFX_HL_SIZE_EXT
#define HFCFX_HL_SIZE_CHKSTOP	HFCFX_HL_SIZE_EXT
#define HFCFX_HL_SIZE_IMLFAIL	0x00008000
#define HFCFX_HL_SIZE_SRAMCE	0x00001000
#define HFCFX_HL_SIZE_CKCMNTU	0x0070
#define HFCFX_HL_SIZE_CKCMNPU	0x00A0
#define HFCFX_HL_SIZE_CKCMNZU	0x0020
#define HFCFX_HL_SIZE_CKPORT	0x0050
#define HFCFX_HL_SIZE_CKCORE	0x00C0
/*==== MCK CODE ==============================================================*/
#define HFCFX_HL_MCKCD_CMNPU	0xA0A00000	/* Common-PU MCK                  */
#define HFCFX_HL_MCKCD_CMN		0xA0A00001	/* Common(Other) MCK              */
#define HFCFX_HL_MCKCD_PORT_B	0xB0B00000	/* PORT MCK(Base Define)          */
#define HFCFX_HL_MCKCD_PORT(P)	(HFCFX_HL_MCKCD_PORT_B | (uint)(P))
#define HFCFX_HL_MCKCD_CORE_B	0xC0C00000	/* CORE MCK(Base Define)          */
#define HFCFX_HL_MCKCD_CORE(C)	(HFCFX_HL_MCKCD_CORE_B | (uint)(C))
/*==== DATA VALUE ============================================================*/
#define HFCFX_HL_DATA_UNDEF		0xFFFFFFFF	/* Undefined Area                 */
#define HFCFX_HL_DATA_RAMIE1	0xFEFEFEFE	/* Indirect Access Error (Case 1) */
#define HFCFX_HL_DATA_RAMIE2	0xFDFDFDFD	/* Indirect Access Error (Case 2) */
#define HFCFX_HL_DATA_CORES		0xFCFCFCFC	
#define HFCFX_HL_DATA_RESEVED	0xEEEEEEEE	/* Reserved Location(on LogFormat)*/
/*==== HW Log Buffer =========================================================*/
typedef struct hw_log_deteil {
	uint	ErrorID;
	uint	Data[(HFCFX_HL_SIZE_UNITW - 1)];
}	HWLOG_DETAIL;

#define HFCFX_HL_IPCCE_GETE(L, E)	 \
	if (((L)->IPC[0].IpcCur) < HFCFX_HL_IPCCE_MAXENT) {	\
	((E) = (IPCCE_ENT *)&((L)->IPC[(((L)->IPC[0].IpcCur) / HFCFX_HL_IPCCE_ENTRY)].entry[(((L)->IPC[0].IpcCur) % HFCFX_HL_IPCCE_ENTRY)]));	\
	} else {	\
	((E) = NULL);	\
	}\
	(L)->IPC[0].IpcCur++;	\
	(L)->IPC[1].IpcCur++;	\
	(L)->IPC[2].IpcCur++;
#define HFCFX_HL_CORECE_GETE(L, C, E)	 \
	if ((L)->CORECE.CoreCur[((C) & HFC_INTE_CORE_MSK)] < HFCFX_HL_CORECE_MAXENT) {\
	((E) = (CORECE_ENT *)&((L)->CORECE.Core[((C) & HFC_INTE_CORE_MSK)].entry[((L)->CORECE.CoreCur[((C) & HFC_INTE_CORE_MSK)])]));	\
	} else {	\
	((E) = NULL);	\
	}	\
	(L)->CORECE.CoreCur[((C) & HFC_INTE_CORE_MSK)]++;

/*==== HW Log Data Get Loop ==================================================*/
#define HFC_LOOP_HLOG(F, G, S)	\
	(G)=0; \
	(G)<((Type_mem_fx *)(F))->size; \
	get+=4,set++


struct err_fx_trc1 {
	uchar				id ;				/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				seq_no ;			/* +02 */
	uint				int_a_status ;		/* +04 */
	uint				a_status_d1 ;		/* +08 */
	uint				a_status_d2 ;		/* +0c */
	uchar				a_status;			/* +10 */
	uchar				resv1[8];			/* +11-18 */
	uchar				a_scsi_id[3];		/* +19-1b */
	uint				rid;				/* +1c-1f */
	uchar				resv2[88];			/* +20-77 */
//	uint64_t			current_time;		/* +78-7f */
};

struct err_fx_trc2 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				seq_no ;			/* +02 */
	uchar				c_core_no ;			/* +04 */
	uchar				mb_retry_cnt ;		/* +05 */
	uchar				mb_status;			/* +06 */
	uchar				a_status ;			/* +07 */
	uint				a_status_d1 ;		/* +08 */
	uint				a_status_d2 ;		/* +0c */
	uint				passthrough_rsp;	/* +10 */
	uchar				t_flag;				/* +14 */
	uchar				t_status[3];		/* +15 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	uchar				t_id;				/* +1c */
	uchar				t_scsi_id[3];		/* +1d */
	uint64_t			a_next_tstart;		/* +20 */
	uint64_t			t_ww_name;			/* +28 */
	uint64_t			t_node_name;		/* +30 */
	uchar				mb_resp[64];		/* +38-77 */
//	uint64_t			current_time;		/* +78-7f */
};

struct err_fx_trc3 {
	uchar				id;					/* +00 */
	uchar				sub_id ;			/* +01 */
	ushort				seq_no ;			/* +02 */
	uchar				c_core_no ;			/* +04 */
	uchar				resv1[1];			/* +05 */
	ushort				xrb_cnt ;			/* +06 */
	uint				a_status_d1 ;		/* +08 */
	uint				a_status_d2 ;		/* +0c */
	uchar				a_status;			/* +10 */
	uchar				c_xob_exec_cnt;		/* +11 */
	uchar				c_xob_w_exec_cnt;	/* +12 */
	uchar				c_next_dstart_cnt;	/* +13 */
	uint				c_iov_no;			/* +14 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	ushort				c_drv_next_xob;		/* +1c */
	ushort				c_drv_next_xrb;		/* +1e */
	uchar				xrb[64] ;			/* +20-5f */
	uint64_t			c_scsi_exec_cnt;	/* +60 */
	uint64_t			c_scsi_end_cnt;		/* +68 */
	uint64_t			hfcp;				/* +70 */
//	uint64_t			current_time;		/* +78-7f */
};

struct err_fx_trc4 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				cmnd0;				/* +05 */
	ushort				h_adap_status;		/* +06 */
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
	ushort				c_drv_next_xob;		/* +24 */
	ushort				c_drv_next_xrb;		/* +26 */
	uint				retries;			/* +28 */
	uint				allowed;			/* +2c */
	uchar				cmnd[16];			/* +30 */
	uint				resid;				/* +40 */
	uint				serial_number;		/* +44 */
	uint				result;				/* +48 */
	uint				h_cmd_flags;		/* +4c */
	uchar				resv1[3];			/* +50 */
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

struct err_fx_trc5 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	ushort				timer_id;			/* +06 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uchar				a_status;			/* +10 */
	uchar				resv1[1];			/* +11 */
	ushort				d_lun;				/* +12 */
	uint				wdog_timeout;		/* +14 */
	uchar				r_rid;				/* +18 */
	uchar				a_scsi_id[3];		/* +19 */
	uchar				t_id;				/* +1c */
	uchar				t_scsi_id[3];		/* +1d */
	uchar				resv2[1];			/* +20 */
	uchar				timeout;			/* +21 */
	uchar				d_flags;			/* +22 */
	uchar				d_status;			/* +23 */
	uchar				t_flag;				/* +24 */
	uchar				t_status[3];		/* +25 */
	uchar				resv3[8];			/* +28 */
	ushort				ct_wx_que_cnt;		/* +30 */
	ushort				ct_we_que_cnt;		/* +32 */
	uchar				c_status;			/* +34 */
	uchar				resv4[1];			/* +35 */
	uchar				t_pseq;				/* +36 */
	uchar				use_seg;			/* +37 */
	uint				retries;			/* +38 */
	uint				allowed;			/* +3c */
	uchar				cmnd[16];			/* +40 */
	uint				resid;				/* +50 */
	uint				serial_number;		/* +54 */
	uint				result;				/* +58 */
	uint				h_cmd_flags;		/* +5c */
	uint				h_adap_status;		/* +60 */
	uint				h_iov_no;			/* +64 */
	uint				h_iov_cnt;			/* +68 */
	uchar				h_cmd_xob;			/* +6c */
	uchar				h_rid;				/* +6d */
	uchar				resv5[2];			/* +6e */
	uint				int_a_reg;			/* +70 */
	uchar				resv6[4];			/* +73 */
//	uint64_t			current_time;		/* +78-7f */
};

struct err_fx_trc6 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				resv1[2];			/* +05 */
	uchar				a_status;			/* +07 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uint				int_a_status;		/* +10 */
	uint				detail_reg;			/* +14 */
	uint64_t			status_reg;			/* +18 */
	uint				link_dead_cnt ;		/* +20 */
	uint				pci_err_cnt ;		/* +24 */
	uint				mck_err_cnt ;		/* +28 */
	uchar				a_scsi_id[4];		/* +2c */
	uchar				resv2[72] ;			/* +30-77 */
//	uint64_t			current_time;		/* +78-7f */
};

struct err_fx_trc7 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				resv1[2];			/* +05 */
	uchar				a_status;			/* +07 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uint				etc1;				/* +10 */
	uint				etc2;				/* +14 */
	uint				etc3;				/* +18 */
	uchar				isol_detail;		/* +1c */
	uchar				c_err;				/* +1d */
	uchar				resv2[2];			/* +1e */
	uint				link_dead_cnt ;		/* +20 */
	uint				pci_err_cnt ;		/* +24 */
	uint				mck_err_cnt ;		/* +28 */
	uchar				a_scsi_id[4];		/* +2c */
	uchar				resv3[72] ;			/* +30-77 */
//	uint64_t			current_time;
};

struct err_fx_trc8 {
	uchar				id;					/* +00 */
	uchar				sub_id;				/* +01 */
	ushort				seq_no;				/* +02 */
	uchar				c_core_no;			/* +04 */
	uchar				resv1[2];			/* +05 */
	uchar				a_status;			/* +07 */
	uint				a_status_d1;		/* +08 */
	uint				a_status_d2;		/* +0c */
	uint				hyp_status;			/* +10 */
	uint				etc2;				/* +14 */
	uint				etc3;				/* +18 */
	uchar				resv2[4];			/* +1c */
	uint				link_dead_cnt ;		/* +20 */
	uint				pci_err_cnt ;		/* +24 */
	uint				mck_err_cnt ;		/* +28 */
	uchar				a_scsi_id[4];		/* +2c */
	uchar				resv3[72] ;			/* +30-77 */
//	uint64_t			current_time;
};

