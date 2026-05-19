/*
 * hfcldd_conf.c
 * Copyright (C) 2007, 2015, Hitachi, Ltd.
 * Author(s): Yoshihiro Toyohara <yoshihiro.toyohara.qs@hitachi.com>
 */
/*
 * $Id: hfcldd_conf.c,v 1.2.8.9.2.3.22.2.2.2.6.1.2.1.2.1 2015/02/04 08:32:56 toyo Exp $
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "hfcmpcfg.h"
#include "hfcldd_conf.h"
#include "hfcldd.h"
#include "hfcl_modulever.h"

static char *config_string = NULL ;
static char *key_string = NULL ;
static char *key_string_hop = NULL ;					/* FCLNX-0429 */

static Config_table conf_tbl =
{
	{
		DESCRIPTION_NAME, sizeof(Common_entry), ""
	},
	{
		AUTHOR_NAME, sizeof(Common_entry), ""
	},
	{
		KEY_NAME, sizeof(Key_entry), ""
	},
	{
		KEY_NAME_HOP, sizeof(Key_entry), ""				/* FCLNX-0429 */
	},
	{
		CONFIG_NAME, sizeof(Config_entry), ""
	},
	{
		CONFIG_END_NAME, 0, ""
	}
};

static struct nonpub_symbol_list hfc_nonpub_symbol_list;
static struct pub_symbol_list hfc_pub_symbol_list;


/*
 * Function:    hfcldd_cnf
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
char *hfcldd_cnf(void)
{
	return (config_string);
}
EXPORT_SYMBOL(hfcldd_cnf);


/*
 * Function:    hfcldd_key
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
char *hfcldd_key(void)
{
	return (key_string);
}
EXPORT_SYMBOL(hfcldd_key);


/*
 * Function:    hfcldd_key_hop
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
char *hfcldd_key_hop(void)					/* FCLNX-0429 */
{
	return (key_string_hop);
}
EXPORT_SYMBOL(hfcldd_key_hop);


/*
 * Function:    hfc_get_nonpub_symbol_list
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
struct nonpub_symbol_list *hfc_get_nonpub_symbol_list(void)
{
	struct nonpub_symbol_list *npubp;
	npubp = &hfc_nonpub_symbol_list;
	return (npubp);
}
EXPORT_SYMBOL(hfc_get_nonpub_symbol_list);


/*
 * Function:    hfc_get_pub_symbol_list
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
struct pub_symbol_list *hfc_get_pub_symbol_list(void)
{
	struct pub_symbol_list *pubp;
	pubp = &hfc_pub_symbol_list;
	return (pubp);
}
EXPORT_SYMBOL(hfc_get_pub_symbol_list);


/*
 * Function:    hfcldd_conf_init
 *
 * Purpose:     
 *
 * Arguments:   
 *
 * Returns:     
 *
 * Notes:       
 */
static int hfcldd_conf_init(void)
{
	Common_entry	*cmnp;
	Key_entry		*kygp;
	Key_entry		*kygp_hop;					/* FCLNX-0429 */
	Config_entry	*cfgp;
	int err = -EPERM;
	
	memset( &hfc_nonpub_symbol_list, 0, sizeof(hfc_nonpub_symbol_list) );
	memset( &hfc_pub_symbol_list, 0, sizeof(hfc_pub_symbol_list) );
	
	/* description name check */
	cmnp = (Common_entry *)&conf_tbl;

	/* offset check */
	if (cmnp->next == 0)
		return err;

	/* author name check */
	cmnp = (Common_entry *)(((char *)cmnp) + cmnp->next);
	if (memcmp(cmnp->name, AUTHOR_NAME, AUTHOR_NAME_SIZE))
		return err;
			
	/* offset check */
	if (cmnp->next == 0)
		return err;

	/* key name check */
	kygp = (Key_entry *)(((char *)cmnp) + cmnp->next);
	if (memcmp(kygp->name, KEY_NAME, KEY_NAME_SIZE))
		return err;

	/* offset check */
	if (kygp->next == 0)
		return err;

	/* key_hop name check */			/* FCLNX-0429 */
	kygp_hop = (Key_entry *)(((char *)kygp) + kygp->next);
	if (memcmp(kygp_hop->name, KEY_NAME_HOP, KEY_NAME_HOP_SIZE))
		return err;

	/* offset check */					/* FCLNX-0429 */
	if (kygp_hop->next == 0)
		return err;

	/* config name check */
	cfgp = (Config_entry *)(((char *)kygp_hop) + kygp_hop->next);	/* FCLNX-0429 */
	if (memcmp(cfgp->name, CONFIG_NAME, CONFIG_NAME_SIZE))
		return err;

	/* offset check */
	if (cfgp->next == 0)
		return err;

	/* end name check */
	cmnp = (Common_entry *)(((char *)cfgp) + cfgp->next);
	if (memcmp(cmnp->name, CONFIG_END_NAME, CONFIG_END_NAME_SIZE))
		return err;

	/* offset check */
	if (cmnp->next != 0)
		return err;

	key_string = kygp->value;
	if (kygp->value[0] == '\0')
		memset(kygp->value, '\0', KEY_ENT_SIZE);

	key_string_hop = kygp_hop->value;								/* FCLNX-0429 */
	if (kygp_hop->value[0] == '\0')
		memset(kygp_hop->value, '\0', KEY_ENT_SIZE);

	config_string = cfgp->value;
	if (cfgp->value[0] == '\0')
		memset(cfgp->value, '\0', CFG_ENT_SIZE);
	
	return 0;
}

static void hfcldd_conf_cleanup (void)
{}

module_init(hfcldd_conf_init);
module_exit(hfcldd_conf_cleanup);

MODULE_LICENSE("GPL");

MODULE_DESCRIPTION("Hitachi Fibre Channel Host Bus Adapter Configuration Module");
MODULE_AUTHOR("Hitachi, Ltd.");
MODULE_VERSION(HFC_MODULEVER);
