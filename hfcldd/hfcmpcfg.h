/*
 * hfcmpcfg.h
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcmpcfg.h,v 1.2.8.6.2.2.22.2.2.3.6.1.2.1.2.1 2015/02/04 08:32:56 toyo Exp $
 */


#define	ENT_NAME_SIZE		64
#define KEY_ENT_SIZE		4108
//#define CFG_ENT_SIZE		100012
#define CFG_ENT_SIZE		200000		/* FCLNX-GPL-463 */


typedef struct	common_entry {
	char	name[ENT_NAME_SIZE];
	int		next;
	char	value[28];
} Common_entry;


typedef struct	key_entry {
	char	name[ENT_NAME_SIZE];
	int		next;
	char	value[KEY_ENT_SIZE];
} Key_entry;

typedef struct	config_entry {
	char	name[ENT_NAME_SIZE];
	int		next;
	char	value[CFG_ENT_SIZE];
} Config_entry;


typedef struct config_table {
	Common_entry	description;
	Common_entry	author;
	Key_entry		key;
	Key_entry		key_hop;			/* FCLNX-0429 */
	Config_entry	config;
	Common_entry	end;
} Config_table;


typedef struct type_table {
	off_t	offset;
	long	size;
	char 	*data;
} Type_tbl;


#define	DESCRIPTION_NAME		"<Hitachi_Fibre_Channel_Host_Bus_Adapter_Configuration_Module>"
#define	AUTHOR_NAME				"<Hitachi_Ltd>"
#define	KEY_NAME				"<hfcldd_key>"
#define	KEY_NAME_HOP			"<hfcldd_key_hop>"														/* FCLNX-0429 */
#define	CONFIG_NAME				"<hfcldd_conf>"
#define	CONFIG_END_NAME			"<CONFIG_END>"

#define DESCRIPTION_NAME_SIZE	sizeof(DESCRIPTION_NAME)
#define AUTHOR_NAME_SIZE		sizeof(AUTHOR_NAME)
#define KEY_NAME_SIZE			sizeof(KEY_NAME)
#define KEY_NAME_HOP_SIZE		sizeof(KEY_NAME_HOP)													/* FCLNX-0429 */
#define CONFIG_NAME_SIZE    	sizeof(CONFIG_NAME)
#define	CONFIG_END_NAME_SIZE	sizeof(CONFIG_END_NAME)

#define	BUF_SIZE				(sizeof(Config_table) )
#define KEY_OFFSET				(sizeof(Common_entry) + sizeof(Common_entry) )
#define KEY_OFFSET_HOP			(sizeof(Common_entry) + sizeof(Common_entry) + sizeof(Key_entry) )		/* FCLNX-0429 */
#define CONFIG_OFFSET			(sizeof(Common_entry) + sizeof(Common_entry) + sizeof(Key_entry) + sizeof(Key_entry) )
#define	ALIGNMENT				4

#define OPT_PRINT		0
#define OPT_WRITE		1
#define OPT_WRITE_KEY	2
#define OPT_WRITE_HOP	3
#define OPT_VER			9

