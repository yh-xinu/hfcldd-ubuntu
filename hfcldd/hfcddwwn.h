/*
 * hfcddwwn.h
 *
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 *
 */

/*
 * $Id: hfcddwwn.h,v 1.4.8.6.12.1.14.2.2.2.6.1.2.1.2.2 2015/02/04 08:32:45 toyo Exp $ 
 */
 
#ifndef _H_HFCDDWWN
#define _H_HFCDDWWN

 
/* ioctl command */ 
/*************************************************************************/
/* Attention !!                                                          */
/* Originally, it is necessary to define ioctl command with hfclddioc.h. */
/*************************************************************************/
#define HFC_IOC_MAGIC   'F'
#define IOCTL_HFC_WWN_INFO  _IOWR(HFC_IOC_MAGIC,20,struct hfc_ioctl_wwn) 



#define MAX_PARTITION   1       /* Max lpar count                           */
#define MAX_PRIORITY    8       /* Max boot Priority                        */

struct hfc_ioctl_wwn {
	uchar   minor       HFC_VAL_ATTR_C;
    uchar   rsv         HFC_VAL_ATTR_C;
 
    uchar       version HFC_VAL_ATTR_C;       /* this sturucture version    */
#define VERSION_0       0       /*   Version 0                              */
#define VERSION_1       1       /*   Version 1				FCLNX-GPL-XX */
    uchar       command HFC_VAL_ATTR_C;       /*   							*/
#define GET_WWN_INFO    1       /* Get wwn & boot infomation                */
#define PUT_WWN_INFO    2       /* Put wwn infomation                       */
#define PUT_BOOT_INFO   3       /* Put boot infomation                      */
#define PUT_WWNBT_INFO  4       /* Put wwn & boot infomation                */
#define CLEAR_WWN_INFO  10      /* Clear wwn infomation                     */
#define CLEAR_BOOT_INFO 11      /* Clear boot infomation                    */
#define GET_ALL_INFO	20		/* Get all boot informaition FCLNX-GPL-XX	*/
#define PUT_ALL_INFO    21      /* Put all boot informaition FCLNX-GPL-XX       */
#define PUT_ALL_INFO_AND_AWWN    	22      /* Put all boot info and Put addWWNN FCLNX-GPL-XX       */
#define PUT_BOOT_INFO_FROM_ALLDATA	83      /* Put boot info from all boot infon FCLNX-GPL-XX       */
#define PUT_WWNBT_INFO_FROM_ALLDATA	84		/* Put boot and addWNN from all boot info FCLNX-GPL-XX	*/
    ushort      resp_code HFC_VAL_ATTR;
#define NORMAL_END      0x0000
#define IOCTL_WWN_BUSY  0x0001
#define IOCTL_WWN_ERR   0xFF00
    uchar       bios_enable HFC_VAL_ATTR_C;
#define BIOS_ENABLE        0x80 /* HBA-BIOS Enable                          */
#define SELECT_BOOT_ENABLE 0x10	/* HBA-BIOS Disable                         */
    uchar     resv0         HFC_VAL_ATTR_C;

    uint64_t    orgWWPN HFC_VAL_ATTR;       /* World Wide Port Name(original)           */
    uint64_t    orgWWNN HFC_VAL_ATTR;       /* World Wide Node Name(original)           */

#define MODEL_NAME_LENGTH 16					/* FCLNX-0329 */
    uchar       model_name[MODEL_NAME_LENGTH] HFC_VAL_ATTR_C;/* HBA model_name           */
    uint        fw_version  HFC_VAL_ATTR;    /* adapter F/W version                      */
    uchar       location[3] HFC_VAL_ATTR_C;  /* Bus,Dev,Func                             */
    uchar       rsrv1 HFC_VAL_ATTR_C;
    struct {
        uint64_t    addWWPN HFC_VAL_ATTR;                   /* World Wide Port Name(additional) */
        uint64_t    addWWNN HFC_VAL_ATTR;                   /* World Wide Node Name(additional) */
        uint64_t    targetWWN[MAX_PRIORITY] HFC_VAL_ATTR;   /* Boot device wwn                  */
        ushort      targetLUN[MAX_PRIORITY] HFC_VAL_ATTR;   /* boot device lun                  */
//        uchar       rsrv2[32];
    } add_info[MAX_PARTITION];
    uint64_t	boot_info_p HFC_VAL_ATTR;					/* boot information  FCLNX-GPL-XX */
    uchar       rsrv3[44]   HFC_VAL_ATTR_C;

};

#define HFC_MAX_BIOS_ENTRY	4		/* Maximum entry number of BIOS table			*/

struct hfc_bios_info {														/* FCLNX-GPL-XX */
	uchar	byte0;					/* + 00 reserved ( should be 0x00 )						*/
	uchar	FLAG;					/* + 01 FLAG											*/
#define HBA_BIOS_ENABLE		0x80		/* bit 0 : HBA BIOS is enabled 						*/
#define BCV_VALID		0x10		/* bit 3 : Select Boot is enabled. Use BCV table	*/
	uchar	BCV_entry;				/* + 02 Last valid entry number of BCV table 			*/
	uchar	connection_type;		/* + 03 Connection Type									*/
#define AUTO				0x00		/* 0x00 : Auto 										*/
#define P2P					0x01		/* 0x01 : Point to Point 							*/
#define FCAL				0x03		/* 0x03 : Arbitrated Loop 							*/

	uchar	data_rate;				/* + 04 Data rate										*/
#define DATA_AUTO			0x00		/* 0x00 : Auto Negotiation							*/
#define DATA_1G				0x01		/* 0x01 : 1G bps									*/
#define DATA_2G				0x02		/* 0x02 : 2G bps									*/
#define DATA_4G				0x04		/* 0x04 : 4G bps									*/
#define DATA_8G				0x08		/* 0x08 : 8G bps									*/
	uchar	bios_param;				/* + 05 Parameters for OS driver						*/
#define PERSISTENT_BINDINGS		0x80	/* bit 0 : Persistent Binding is enabled			*/
#define FORCE_DEFAULT_PARAMETER	0x40	/* bit 1 : Force Default Parameter is enabled 		*/
	uchar 	LOGIN_delay_time;		/* + 06 LOGIN Delay Time								*/
#define LOGIN_DELAY_VALID	0x80		/* bit 0 : Valid bit								*/
										/* bit 1 - 7 : Login Delay Time (0-60sec)			*/
	uchar	byte7;					/* + 07 reserved ( should be 0x00 )						*/
	
	uchar   byte8[8];			/* + 08 reserved    ( byte  8-  15 )	*/
	
#define BCV_TABLE_ENTRY			8
	struct  tgt_entry {			/* target entry */
		uchar		byte0[6];		/* + 00 - 05 : reserved		*/
		ushort		lun;			/* + 06 - 07 : target lun#	*/
		uint64_t	wwpn;			/* + 08 - 15 : wwpn			*/
	} BCV_Table[BCV_TABLE_ENTRY];

	uchar expansion_control[32];		/* Expansion Control Data Table */
	uchar byte176[80];			/* reserved (64bytes) */
};



struct cr_PartsNumber {							/* FCLNX-0329 */
	uchar	pk_code;							/* package code */
	uchar	pk_port;							/* port num */
	ushort	wk;
	char	header[4];							/* Parts Number char0 */
	char	pk[4];								/* Parts Number char6 */
	char	hwinf[4];							/* Parts Number char9,10 */
	char	model_name[MODEL_NAME_LENGTH];		/* modele name */
};


#endif /* _H_HFCDDWWN */
