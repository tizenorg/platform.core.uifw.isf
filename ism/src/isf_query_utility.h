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

#ifndef __ISF_QUERY_UTILITY_H
#define __ISF_QUERY_UTILITY_H

/* For multi-user support */
#include <tzplatform_config.h>

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define USER_ENGINE_LIST_PATH           "/home/app/.scim"

typedef struct {
    String name;
    String uuid;
    String module;
    String language;
    String icon;
    TOOLBAR_MODE_T mode;
    uint32 option;
    String locales;
} ISEINFO;

typedef struct {
    String appid;
    String label;
    String languages;
    String iconpath;
    String pkgid;
    String pkgtype;
    String module_name;
    String module_path;
    TOOLBAR_MODE_T mode;
    uint32 options;
} ImeInfoDB;

EAPI int isf_db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info);
EAPI int isf_db_update_label_ime_info(const char *appid, const char *label);

#endif /* __ISF_QUERY_UTILITY_H */

/*
vi:ts=4:ai:nowrap:expandtab
*/
