/** @file scim_helper_module.cpp
 *  @brief Implementation of class HelperModule.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * Modifications by Samsung Electronics Co., Ltd.
 * 1. Add new interface APIs
 *    a. get_helper_lang () and set_path_info ()
 *    b. set_arg_info ()
 *
 * $Id: scim_helper_module.cpp,v 1.5 2005/01/05 13:41:10 suzhe Exp $
 *
 */

#define Uses_SCIM_HELPER
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_CONFIG_MODULE
#define Uses_C_STDLIB

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scim_private.h"
#include "scim.h"
#include <scim_panel_common.h>
#include "isf_query_utility.h"

namespace scim {

HelperModule::HelperModule (const String &name)
    : appid (""),
      m_number_of_helpers (0),
      m_get_helper_info (0),
      m_get_helper_lang (0),
      m_run_helper (0),
      m_set_arg_info (0),
      m_set_path_info (0)
{
    if (name.length ()) load (name);
}

bool
HelperModule::load (const String &name)
{
    try {
        if (!m_module.load (name, "Helper"))
            return false;

        appid = name;


        m_run_helper =
            (HelperModuleRunHelperFunc) m_module.symbol ("scim_helper_module_run_helper");

        if (!m_run_helper) {
            m_module.unload ();
            appid = "";
            m_number_of_helpers = 0;
            m_get_helper_info = 0;
            m_get_helper_lang = 0;
            m_run_helper = 0;
            m_set_arg_info = 0;
            m_set_path_info = 0;
            return false;
        }
    } catch (...) {
        m_module.unload ();
        appid = "";
        m_number_of_helpers = 0;
        m_get_helper_info = 0;
        m_get_helper_lang = 0;
        m_run_helper = 0;
        m_set_arg_info = 0;
        m_set_path_info = 0;
        return false;
    }

    return true;
}

bool
HelperModule::unload ()
{
    return m_module.unload ();
}

bool
HelperModule::valid () const
{
    return (m_module.valid () &&
            appid.length () > 0 &&
            m_run_helper);
}

unsigned int
HelperModule::number_of_helpers () const
{
    return static_cast<unsigned int>(1);
}

bool
HelperModule::get_helper_info (unsigned int idx, HelperInfo &info) const
{
    if (m_module.valid () && m_run_helper && appid.length () > 0) {
        std::vector<ImeInfoDB> ime_info;
        std::vector<ImeInfoDB>::iterator iter;

        isf_db_select_all_ime_info (ime_info);
        for (iter = ime_info.begin (); iter != ime_info.end (); iter++) {
            if (iter->appid.compare(appid) == 0) {
                info.uuid = iter->appid;
                info.name = iter->label;
                info.icon = iter->iconpath;
                info.description = "";
                info.option = iter->options;
                return true;
            }
        }
    }
    return false;
}

String
HelperModule::get_helper_lang (unsigned int idx) const
{
    return String("other");
}

void
HelperModule::run_helper (const String &uuid, const ConfigPointer &config, const String &display) const
{
    m_run_helper (uuid, config, display);
}

void
HelperModule::set_arg_info (int argc, char *argv []) const
{
    return;
}

EAPI int scim_get_helper_module_list (std::vector <String> &mod_list)
{
    return scim_get_module_list (mod_list, "Helper");
}

} // namespace scim

/*
vi:ts=4:nowrap:ai:expandtab
*/

