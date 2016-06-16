/** @file isf_panel_agent_module.cpp
 *  @brief Implementation of class PanelAgentModule.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * Contact: Li Zhang <li2012.zhang@samsung.com>
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
 * $Id: isf_panel_agent_module.cpp,v 1.00 2016/03/03 15:38:00 lizhang $
 */

#define Uses_SCIM_PANEL_AGENT_MODULE
#define Uses_SCIM_MODULE
#include "scim_private.h"
#include "scim.h"
#include <dlog.h>
#include "isf_debug.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_PANEL_AGENT_MODULE"


namespace scim
{

PanelAgentModule::PanelAgentModule ()
    : m_panel_agent_init (0),
      m_panel_agent_get_instance (0)
{
}

PanelAgentModule::PanelAgentModule (const String& name, const ConfigPointer& config)
    : m_panel_agent_init (0),
      m_panel_agent_get_instance (0)
{
    load (name, config);
}

bool
PanelAgentModule::load (const String& name, const ConfigPointer& config)
{
    m_module_name = name;

    try {
        if (!m_module.load (name, "PanelAgent"))
            return false;

        m_panel_agent_init = (PanelAgentModuleInitFunc) m_module.symbol ("scim_panel_agent_module_init");
        m_panel_agent_get_instance = (PanelAgentModuleGetInstanceFunc) m_module.symbol ("scim_panel_agent_module_get_instance");

        if (!m_panel_agent_init || !m_panel_agent_get_instance) {
            m_module.unload ();
            m_panel_agent_init = 0;
            m_panel_agent_get_instance = 0;
            return false;
        }

        m_panel_agent_init (config);
    } catch (...) {
        m_module.unload ();
        m_panel_agent_init = 0;
        m_panel_agent_get_instance = 0;
        return false;
    }

    return true;
}

bool
PanelAgentModule::unload ()
{
    return m_module.unload ();
}

bool
PanelAgentModule::valid () const
{
    return (m_module.valid () && m_panel_agent_init && m_panel_agent_get_instance);
}

String
PanelAgentModule::get_module_name () const
{
    return m_module_name;
}


PanelAgentPointer
PanelAgentModule::get_instance () const
{
    if (valid ())
        return m_panel_agent_get_instance ();

    return PanelAgentPointer (0);
}


EXAPI int scim_get_panel_agent_module_list (std::vector <String>& mod_list)
{
    return scim_get_module_list (mod_list, "PanelAgent");
}


} // namespace scim

/*
vi:ts=4:nowrap:ai:expandtab
*/
