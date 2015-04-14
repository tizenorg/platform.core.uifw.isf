/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_COMPOSE_KEY
#define Uses_SCIM_PANEL_AGENT

#include <string.h>
#include "scim.h"
#include "isf_control.h"


using namespace scim;


EAPI int isf_control_set_active_ise_by_uuid (const char *uuid)
{
    if (uuid == NULL)
        return -1;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.set_active_ise_by_uuid (uuid);
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_get_active_ise (char **uuid)
{
    if (uuid == NULL)
        return -1;

    String strUuid;
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_active_ise (strUuid);
    imcontrol_client.close_connection ();

    *uuid = strUuid.length () ? strdup (strUuid.c_str ()) : strdup ("");

    return strUuid.length ();
}

EAPI int isf_control_get_ise_list (char ***uuid_list)
{
    if (uuid_list == NULL)
        return -1;

    int count;
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_ise_list (&count, uuid_list);
    imcontrol_client.close_connection ();
    return count;
}

EAPI int isf_control_get_ise_info (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option)
{
    return isf_control_get_ise_info_and_module_name (uuid, name, language, type, option, NULL);
}

EAPI int isf_control_get_ise_info_and_module_name (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option, char **module_name)
{
    if (uuid == NULL)
        return -1;

    int nType   = 0;
    int nOption = 0;
    String strName, strLanguage, strModuleName;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_ise_info (uuid, strName, strLanguage, nType, nOption, strModuleName);
    imcontrol_client.close_connection ();

    if (name != NULL)
        *name = strName.length () ? strdup (strName.c_str ()) : strdup ("");

    if (language != NULL)
        *language = strLanguage.length () ? strdup (strLanguage.c_str ()) : strdup ("");

    if (type != NULL)
        *type = (ISE_TYPE_T)nType;

    if (option != NULL)
        *option = nOption;

    if (module_name != NULL)
        *module_name = strModuleName.length () ? strdup (strModuleName.c_str ()) : strdup ("");

    return 0;
}

EAPI int isf_control_set_active_ise_to_default (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.set_active_ise_to_default ();
    imcontrol_client.send ();
    imcontrol_client.close_connection ();

    return 0;
}

EAPI int isf_control_reset_ise_option (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.reset_ise_option ();
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_set_initial_ise_by_uuid (const char *uuid)
{
    if (uuid == NULL)
        return -1;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.set_initial_ise_by_uuid (uuid);
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_get_initial_ise (char **uuid)
{
    if (uuid == NULL)
        return -1;

    String strUuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (SCIM_COMPOSE_KEY_FACTORY_UUID));

    *uuid = strUuid.length () ? strdup (strUuid.c_str ()) : strdup ("");

    return strUuid.length ();
}

EAPI int isf_control_show_ise_selector (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.show_ise_selector ();
    imcontrol_client.send ();
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_get_ise_count (ISE_TYPE_T type)
{
    char **iselist = NULL;
    int all_ise_count, ise_count = 0;
    ISE_TYPE_T isetype;

    all_ise_count = isf_control_get_ise_list (&iselist);
    if (all_ise_count < 0) {
        if (iselist)
            free (iselist);
        return -1;
    }

    for (int i = 0; i < all_ise_count; i++) {
        if (iselist[i]) {
            isf_control_get_ise_info (iselist[i], NULL, NULL, &isetype, NULL);
            if (isetype == type) {
                ise_count++;
            }

            free (iselist[i]);
        }
    }

    if (iselist)
        free (iselist);

    return ise_count;
}

EAPI int isf_control_show_ise_option_window (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.show_ise_option_window ();
    imcontrol_client.send ();
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_get_all_ime_info (ime_info_s **info)
{
    int count = -1, i = 0;
    HELPER_ISE_INFO helper_info;
    ime_info_s *ime_info = NULL;

    if (info == NULL)
        return count;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_all_helper_ise_info (helper_info);
    imcontrol_client.close_connection ();

    count = static_cast<int>(helper_info.label.size());
    if (count > 0) {
        ime_info = (ime_info_s *)calloc (count, sizeof (ime_info_s));
        if (ime_info) {
            for (i = 0; i < count; i++) {
                snprintf(ime_info[i].appid, sizeof (ime_info[i].appid), "%s", helper_info.appid[i].c_str());
                snprintf(ime_info[i].label, sizeof (ime_info[i].label), "%s", helper_info.label[i].c_str());
                ime_info[i].is_enabled = static_cast<bool>(helper_info.is_enabled[i]);
                ime_info[i].is_preinstalled = static_cast<bool>(helper_info.is_preinstalled[i]);
                ime_info[i].has_option = static_cast<int>(helper_info.has_option[i]);
            }
            *info = ime_info;
        }
        else {
            count = -1;
        }
    }

    return count;
}

EAPI int isf_control_get_active_ime (char **appid)
{
    if (appid == NULL)
        return -1;

    String strUuid;
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_active_ise (strUuid);
    imcontrol_client.close_connection ();

    *appid = strUuid.length () ? strdup (strUuid.c_str ()) : NULL;

    if (*appid == NULL)
        return -1;
    else
        return strUuid.length ();
}

EAPI int isf_control_set_active_ime (const char *appid)
{
    return isf_control_set_active_ise_by_uuid(appid);
}

EAPI int isf_control_set_enable_ime (const char *appid, bool is_enabled)
{
    if (!appid)
        return -1;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.set_enable_helper_ise_info (appid, is_enabled);
    imcontrol_client.close_connection ();

    return 0;
}

EAPI int isf_control_show_ime_list (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.show_helper_ise_list ();
    imcontrol_client.send ();
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_show_ime_selector (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.show_helper_ise_selector ();
    imcontrol_client.send ();
    imcontrol_client.close_connection ();
    return 0;
}

EAPI int isf_control_is_ime_enabled (const char *appid, bool *enabled)
{
    if (!appid || !enabled)
        return -1;

    int nEnabled = -1;

    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.is_helper_ise_enabled (appid, nEnabled);
    imcontrol_client.close_connection ();

    if (nEnabled < 0)
        return -1;
    else
        *enabled = static_cast<bool>(nEnabled);

    return 0;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
