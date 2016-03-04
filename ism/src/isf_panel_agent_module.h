/** @file isf_panel_agent_module.h
 *  @brief Define scim::PanelAgentModule class for manipulating the PanelAgent modules.
 *
 *  Class scim::PanelAgentModule is a wrapper of class scim::Module,
 *  which is for manipulating the PanelAgent modules.
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
 * $Id: isf_panel_agent_module.h,v 1.00 2016/03/03 15:38:00 lizhang $
 */

#ifndef __ISF_PANEL_AGENT_MODULE_H
#define __ISF_PANEL_AGENT_MODULE_H

namespace scim
{
/**
 * @addtogroup PanelAgent
 * @ingroup InputServiceFramework
 * @{
 */

/**
 * @brief The prototype of initialization function in PanelAgent modules.
 *
 * There must be a function called "scim_panel_agent_module_init"
 * which complies with this prototype.
 * This function name can have a prefix like wayland_LTX_,
 * in which "wayland" is the module's name.
 *
 * @param config The Config object should be used to read/write configurations.
 */
typedef void (*PanelAgentModuleInitFunc) (const ConfigPointer& config);

/**
 * @brief Get a PanelAgentModule instance,
 *
 * There must be a function called "scim_panel_agent_module_get_instance"
 * which complies with this prototype.
 * This function name can have a prefix like wayland_LTX_,
 * in which "wayland" is the module's name.
 *
 * @return the pointer of the PanelAgent object.
 */
typedef PanelAgentPointer (*PanelAgentModuleGetInstanceFunc) ();


/**
 * @brief The class to manipulate the PanelAgent modules.
 *
 * This is a wrapper of scim::Module class, which is specially
 * for manipulating the PanelAgent modules.
 */
class EXAPI PanelAgentModule
{
    Module m_module;
    String m_module_name;

    PanelAgentModuleInitFunc m_panel_agent_init;
    PanelAgentModuleGetInstanceFunc m_panel_agent_get_instance;

    PanelAgentModule (const PanelAgentModule&);
    PanelAgentModule& operator= (const PanelAgentModule&);

public:
    /**
     * @brief Default constructor.
     */
    PanelAgentModule ();

    /**
     * @brief Constructor.
     *
     * @param name The module name to be loaded.
     * @param config The Config object should be used to read/write configurations.
     */
    PanelAgentModule (const String& name, const ConfigPointer& config);

    /**
     * @brief Load a module by its name.
     *
     * Load a module into memory.
     * If another module has been loaded into this object,
     * then the old module will be unloaded first.
     * If the old module is resident, false will be returned,
     * and the old module will be untouched.
     *
     * @param name - the module's name, eg. "wayland".
     * @param config The Config object should be used to read/write configurations.
     * @return true if success.
     */
    bool load (const String& name, const ConfigPointer& config);

    /**
     * @brief Check if a module is loaded and initialized successfully.
     *
     * @return true if a module is already loaded and initialized successfully.
     */
    bool valid () const;

    /**
     * @brief Get a name of currently PanelAgent modules.
     *
     * @return the module name.
     */
    String get_module_name () const;

    /**
     * @brief Get the pointer of the PanelAgent object.
     *
     * @return A smart pointer to the PanelAgent object, NULL if failed.
     */
    PanelAgentPointer get_instance () const;

};

/**
 * @brief Get a name list of currently available PanelAgent modules.
 * @param mod_list - the result list will be stored here.
 * @return the number of the modules, equal to mod_list.size ().
 */
EXAPI int scim_get_panel_agent_module_list (std::vector <String>& mod_list);

/** @} */

} // namespace scim

#endif //__ISF_PANEL_AGENT_MODULE_H

/*
vi:ts=4:ai:nowrap:expandtab
*/
