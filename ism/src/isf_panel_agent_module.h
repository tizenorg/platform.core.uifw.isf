/** @file scim_config_module.h
 *  @brief Define scim::ConfigModule class for manipulating the config modules.
 *
 *  Class scim::ConfigModule is a wrapper of class scim::Module,
 *  which is for manipulating the configuration modules.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 Li Zhang <li2012.zhang@samsung.com>
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
 * $Id: scim_panel_client_module.h,v 0.01 2016/01/26 15:38:00 lizhang $
 */

#ifndef __ISF_PANEL_AGENT_MODULE_H
#define __ISF_PANEL_AGENT_MODULE_H

namespace scim
{
/**
 * @addtogroup Config
 * @ingroup InputServiceFramework
 * @{
 */

/**
 * @brief The prototype of initialization function in config modules.
 *
 * There must be a function called "scim_config_module_init"
 * which complies with this prototype.
 * This function name can have a prefix like simple_LTX_,
 * in which "simple" is the module's name.
 */
typedef void (*PanelAgentModuleInitFunc) (const ConfigPointer& config);

/**
 * @brief Create a factory instance for an engine,
 *
 * There must be a function called "scim_imengine_module_create_factory"
 * which complies with this prototype.
 * This function name can have a prefix like table_LTX_,
 * in which "table" is the module's name.
 *
 * @param engine - the index of the engine for which a factory object will be created.
 * @return the pointer of the factory object.
 */
typedef PanelAgentPointer (*PanelAgentModuleGetInstanceFunc) ();


/**
 * @brief The class to manipulate the PanelClient modules.
 *
 * This is a wrapper of scim::Module class, which is specially
 * for manipulating the PanelClient modules.
 */
class EXAPI PanelAgentModule
{
    Module      m_module;
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
     * @param name - the module's name, eg. "simple".
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
     * @param name - the module's name, eg. "simple".
     * @return true if success.
     */
    bool load (const String& name, const ConfigPointer& config);

    /**
     * @brief Check if a module is loaded and initialized successfully.
     * @return true if a module is already loaded and initialized successfully.
     */
    bool valid () const;
    /**
     * @brief Get a name of currently IMEngine modules.
     */
    String get_module_name () const;


    /**
     * @brief Create an object for an IMEngine factory.
     *
     * @param engine - the index of this IMEngine factory,
     *                 must be less than the result of number_of_factories method
     *                 and greater than or equal to zero.
     * @return A smart pointer to the factory object, NULL if failed.
     */
    PanelAgentPointer get_instance () const;

};

/**
 * @brief Get a name list of currently available configuration modules.
 * @param mod_list - the result list will be stored here.
 * @return the number of the modules, equal to mod_list.size ().
 */
EXAPI int scim_get_panel_agent_module_list (std::vector <String>& mod_list);

/** @} */

} // namespace scim

#endif //__SCIM_CONFIG_MODULE_H

/*
vi:ts=4:ai:nowrap:expandtab
*/
