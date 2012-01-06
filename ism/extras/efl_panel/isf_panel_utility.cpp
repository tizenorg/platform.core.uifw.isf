/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_COMPOSE_KEY


#include <string.h>
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_panel_utility.h"
#include "isf_query_utility.h"


/////////////////////////////////////////////////////////////////////////////
// Declaration of global variables.
/////////////////////////////////////////////////////////////////////////////
MapStringVectorSizeT         _groups;
std::vector<String>          _uuids;
std::vector<String>          _names;
std::vector<String>          _module_names;
std::vector<String>          _langs;
std::vector<String>          _icons;
std::vector<uint32>          _options;
std::vector<TOOLBAR_MODE_T>  _modes;

std::vector<String>          _load_ise_list;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////
static std::vector<String>   _current_modules_list;

static MapStringVectorString _disabled_ise_map;
static std::vector<String>   _disabled_langs_bak;
std::vector<String>          _disabled_langs;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static bool add_keyboard_ise_module (const String ise_name, const ConfigPointer &config);
static bool add_helper_ise_module   (const String ise_name);


/**
 * @brief Get all ISEs support languages.
 *
 * @param all_langs The list to store all languages.
 */
void isf_get_all_languages (std::vector<String> &all_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        all_langs.push_back (lang_name);
    }
}

/**
 * @brief Get enabled languages.
 *
 * @param enabled_langs The list to store languages.
 */
void isf_get_enabled_languages (std::vector<String> &enabled_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ())
            enabled_langs.push_back (lang_name);
    }
}

/**
 * @brief Get enabled ISE names for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param ise_names The list to store ISE names .
 */
void isf_get_enabled_ise_names_in_languages (std::vector<String> lang_list, std::vector<String> &ise_names)
{
    String lang_name;
    String active_app ("Default");

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (_current_modules_list.size () > 0 &&
                        std::find (_current_modules_list.begin (), _current_modules_list.end (), _module_names[it->second[i]]) == _current_modules_list.end ())
                    continue;
                if (std::find (_disabled_ise_map[active_app].begin (), _disabled_ise_map[active_app].end (), _uuids[it->second[i]]) ==_disabled_ise_map[active_app].end ()) {
                    // Avoid to add the same ise
                    if (std::find (ise_names.begin (), ise_names.end (), _names[it->second[i]]) == ise_names.end ())
                        ise_names.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}

/**
 * @brief Get keyboard ISE
 *
 * @param ise_uuid The keyboard ISE uuid.
 * @param ise_name The keyboard ISE name.
 */
void isf_get_keyboard_ise (String &ise_uuid, String &ise_name, const ConfigPointer &config)
{
    String language = String ("~other");
    String uuid     = config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, String ("d75857a5-4148-4745-89e2-1da7ddaf7999"));
    if (ise_uuid.length () > 0)
        uuid = ise_uuid;
    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (uuid == _uuids[i]) {
            ise_name = _names[i];
            ise_uuid = uuid;
            return;
        }
    }
    ise_name = String ("");
    ise_uuid = String ("");
}

/**
 * @brief Get all keyboard ISE names for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param keyboard_names The list to store keyboard ISE names.
 */
void isf_get_keyboard_names_in_languages (std::vector<String> lang_list, std::vector<String> &keyboard_names)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (_modes[it->second[i]] != TOOLBAR_KEYBOARD_MODE)
                    continue;
                if (_current_modules_list.size () > 0 &&
                        std::find (_current_modules_list.begin (), _current_modules_list.end (), _module_names[it->second[i]]) == _current_modules_list.end ())
                    continue;
                if (std::find (keyboard_names.begin (), keyboard_names.end (), _names[it->second[i]]) == keyboard_names.end ())
                    keyboard_names.push_back (_names[it->second[i]]);
            }
        }
    }
}

/**
 * @brief Get all keyboard ISE uuids for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param keyboard_uuids The list to store keyboard ISE uuids.
 */
void isf_get_keyboard_uuids_in_languages (std::vector<String> lang_list, std::vector<String> &keyboard_uuids)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (_modes[it->second[i]] != TOOLBAR_KEYBOARD_MODE)
                    continue;
                if (_current_modules_list.size () > 0 &&
                        std::find (_current_modules_list.begin (), _current_modules_list.end (), _module_names[it->second[i]]) == _current_modules_list.end ())
                    continue;
                if (std::find (keyboard_uuids.begin (), keyboard_uuids.end (), _uuids[it->second[i]]) == keyboard_uuids.end ())
                    keyboard_uuids.push_back (_uuids[it->second[i]]);
            }
        }
    }
}

/**
 * @brief Get enabled helper ISE names for the specific languages.
 *
 * @param lang_list The specific languages list.
 * @param helper_names The list to store helper ISE names.
 */
void isf_get_helper_names_in_languages (std::vector<String> lang_list, std::vector<String> &helper_names)
{
    String lang_name;
    String active_app ("Default");

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {

                if (_modes[it->second[i]]!= TOOLBAR_HELPER_MODE)
                    continue;
                if (_current_modules_list.size () > 0 &&
                        std::find (_current_modules_list.begin (), _current_modules_list.end (), _module_names[it->second[i]]) == _current_modules_list.end ())
                    continue;
                if (std::find (_disabled_ise_map[active_app].begin (), _disabled_ise_map[active_app].end (), _uuids[it->second[i]])
                        ==_disabled_ise_map[active_app].end ()) {
                    // Avoid to add the same ISE
                    if (std::find (helper_names.begin (), helper_names.end (), _names[it->second[i]]) == helper_names.end ())
                        helper_names.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}

/**
 * @brief Save all ISEs information into cache file.
 *
 * @return void
 */
void isf_save_ise_information (void)
{
    if (_module_names.size () <= 0)
        return;

    std::vector<ISEINFO> info_list;
    for (size_t i = 0; i < _module_names.size (); ++i) {
        if (_module_names[i] == ENGLISH_KEYBOARD_MODULE)
            continue;
        ISEINFO info;
        info.name     = _names[i];
        info.uuid     = _uuids[i];
        info.module   = _module_names[i];
        info.language = _langs[i];
        info.icon     = _icons[i];
        info.mode     = _modes[i];
        info.option   = _options[i];
        info.locales  = "";
        info_list.push_back (info);
    }

    String user_file_name = String (USER_ENGINE_FILE_NAME);
    isf_write_ise_info_list (user_file_name.c_str (), info_list);
}

/**
 * @brief Load all ISEs to initialize.
 *
 * @param type The loading ISE type.
 * @param config The config pointer for loading keyboard ISE.
 * @param uuids The ISE uuid list.
 * @param names The ISE name list.
 * @param module_names The ISE module name list.
 * @param langs The ISE language list.
 * @param icons The ISE icon list.
 * @param modes The ISE type list.
 * @param options The ISE option list.
 * @param ise_list The already loaded ISE list.
 */
void isf_get_factory_list (LOAD_ISE_TYPE  type,
                           const ConfigPointer &config,
                           std::vector<String> &uuids,
                           std::vector<String> &names,
                           std::vector<String> &module_names,
                           std::vector<String> &langs,
                           std::vector<String> &icons,
                           std::vector<TOOLBAR_MODE_T> &modes,
                           std::vector<uint32> &options,
                           const std::vector<String> &ise_list)
{
    uuids.clear ();
    names.clear ();
    module_names.clear ();
    langs.clear ();
    icons.clear ();
    modes.clear ();
    options.clear ();
    _groups.clear ();

    if (type != HELPER_ONLY) {
        /* Add "English/Keyboard" factory first. */
        IMEngineFactoryPointer factory = new ComposeKeyFactory ();
        uuids.push_back (factory->get_uuid ());
        names.push_back (utf8_wcstombs (factory->get_name ()));
        module_names.push_back (ENGLISH_KEYBOARD_MODULE);
        langs.push_back (isf_get_normalized_language (factory->get_language ()));
        icons.push_back (factory->get_icon_file ());
        modes.push_back (TOOLBAR_KEYBOARD_MODE);
        options.push_back (0);
        factory.reset ();
    }

    String user_file_name = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (user_file_name.c_str (), "r");
    if (engine_list_file == NULL) {
        std::cerr <<  user_file_name << " doesn't exist.\n";
        return;
    }

    char buf[MAXLINE];
    while (fgets (buf, MAXLINE, engine_list_file) != NULL) {
        ISEINFO info;
        isf_get_ise_info_from_string (buf, info);
        if (info.mode == TOOLBAR_HELPER_MODE || type != HELPER_ONLY) {
            names.push_back (info.name);
            uuids.push_back (info.uuid);
            module_names.push_back (info.module);
            langs.push_back (info.language);
            icons.push_back (info.icon);
            modes.push_back (info.mode);
            options.push_back (info.option);
        }
    }

    fclose (engine_list_file);
}

/**
 * @brief Load all ISEs information and ISF default languages.
 *
 * @param type The load ISE type.
 * @param config The config pointer for loading keyboard ISE.
 */
void isf_load_ise_information (LOAD_ISE_TYPE  type, const ConfigPointer &config)
{
    /* Load ISE engine info */
    isf_get_factory_list (type, config, _uuids, _names, _module_names, _langs, _icons, _modes, _options, _load_ise_list);

    _current_modules_list.clear ();
    scim_get_helper_module_list (_current_modules_list);
    /* Check keyboard ISEs */
    if (type != HELPER_ONLY) {
        _current_modules_list.push_back (ENGLISH_KEYBOARD_MODULE);
        std::vector<String> imengine_list;
        scim_get_imengine_module_list (imengine_list);
        for (size_t i = 0; i < imengine_list.size (); ++i)
            _current_modules_list.push_back (imengine_list [i]);
    }

    /* Update _groups */
    std::vector<String> ise_langs;
    for (size_t i = 0; i < _uuids.size (); ++i) {
        scim_split_string_list (ise_langs, _langs[i]);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                _groups[ise_langs[j]].push_back (i);
        }
        ise_langs.clear ();
    }

    /* Get ISF default language */
    std::vector<String> isf_default_langs;
    isf_default_langs = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), isf_default_langs);

    /* No default ISF language */
    if (isf_default_langs.size () == 0) {
        String sys_input_lang, lang_info ("English");
        sys_input_lang = config->read (String (SCIM_CONFIG_SYSTEM_INPUT_LANGUAGE), sys_input_lang);
        MapStringVectorSizeT::iterator g = _groups.find (sys_input_lang);
        if (g != _groups.end ())
            lang_info = sys_input_lang;
        else
            std::cerr << "System input language is not included in the ISF languages.\n";

        for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
            String lang_name = scim_get_language_name_english (it->first);
            if (strcmp (lang_info.c_str (), lang_name.c_str ())) {
                _disabled_langs.push_back (lang_name);
                _disabled_langs_bak.push_back (lang_name);
            }
        }
    } else {
        std::vector<String> disabled;
        disabled = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), disabled);
        for (size_t i = 0; i < disabled.size (); i++) {
            _disabled_langs.push_back (disabled[i]);
            _disabled_langs_bak.push_back (disabled[i]);
        }
    }
}

/**
 * @brief Load one keyboard ISE module.
 *
 * @param module_name The keboard ISE module name.
 * @param config The config pointer for loading keyboard ISE.
 *
 * @return true if load module is successful, otherwise return false.
 */
static bool add_keyboard_ise_module (const String module_name, const ConfigPointer &config)
{
    if (module_name.length () <= 0 || module_name == "socket")
        return false;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;

    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        std::cerr << "failed to open " << filename << "\n";
        return false;
    }

    ime_module.load (module_name, config);
    if (ime_module.valid ()) {
        for (size_t j = 0; j < ime_module.number_of_factories (); ++j) {
            try {
                factory = ime_module.create_factory (j);
            } catch (...) {
                factory.reset ();
            }

            if (!factory.null ()) {
                if (std::find (_uuids.begin (), _uuids.end (), factory->get_uuid ()) == _uuids.end ()) {
                    String uuid = factory->get_uuid ();
                    String name = utf8_wcstombs (factory->get_name ());
                    String language = isf_get_normalized_language (factory->get_language ());
                    String icon = factory->get_icon_file ();
                    char mode[12];
                    char option[12];

                    _uuids.push_back (uuid);
                    _names.push_back (name);
                    _module_names.push_back (module_name);
                    _langs.push_back (language);
                    _icons.push_back (icon);
                    _modes.push_back (TOOLBAR_KEYBOARD_MODE);
                    _options.push_back (0);

                    snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_KEYBOARD_MODE);
                    snprintf (option, sizeof (option), "%d", 0);

                    String line = isf_combine_ise_info_string (name, uuid, module_name, language, 
                                                               icon, String (mode), String (option), factory->get_locales ());
                    if (fputs (line.c_str (), engine_list_file) < 0) {
                        std::cerr << "write to ise cache file failed:" << line << "\n";
                        break;
                    }
                }
                factory.reset ();
            }
        }
        ime_module.unload ();
    }

    fclose (engine_list_file);

    return true;
}

/**
 * @brief Load one helper ISE module.
 *
 * @param module_name The helper ISE module name.
 *
 * @return true if load module is successful, otherwise return false.
 */
static bool add_helper_ise_module (const String module_name)
{
    if (module_name.length () <= 0)
        return false;

    HelperModule helper_module;
    HelperInfo   helper_info;

    String filename = String (USER_ENGINE_FILE_NAME);
    FILE *engine_list_file = fopen (filename.c_str (), "a");
    if (engine_list_file == NULL) {
        std::cerr << "failed to open " << filename << "\n";
        return false;
    }

    helper_module.load (module_name);
    if (helper_module.valid ()) {
        for (size_t j = 0; j < helper_module.number_of_helpers (); ++j) {
            helper_module.get_helper_info (j, helper_info);
            _uuids.push_back (helper_info.uuid);
            _names.push_back (helper_info.name);
            _langs.push_back (isf_get_normalized_language (helper_module.get_helper_lang (j)));
            _module_names.push_back (module_name);
            _icons.push_back (helper_info.icon);
            _modes.push_back (TOOLBAR_HELPER_MODE);
            _options.push_back (helper_info.option);

            char mode[12];
            char option[12];
            snprintf (mode, sizeof (mode), "%d", (int)TOOLBAR_HELPER_MODE);
            snprintf (option, sizeof (option), "%d", helper_info.option);

            String line = isf_combine_ise_info_string (helper_info.name, helper_info.uuid, module_name, isf_get_normalized_language (helper_module.get_helper_lang (j)),
                                                       helper_info.icon, String (mode), String (option), String (""));
            if (fputs (line.c_str (), engine_list_file) < 0) {
                 std::cerr << "write to ise cache file failed:" << line << "\n";
                 break;
            }
        }
        helper_module.unload ();
    }

    fclose (engine_list_file);

    return true;
}

/**
 * @brief Update ISEs information for ISE is added or removed.
 *
 * @param type The load ISE type.
 * @param config The config pointer for loading keyboard ISE.
 *
 * @return true if ISEs list is changed, otherwise return false.
 */
bool isf_update_ise_list (LOAD_ISE_TYPE type, const ConfigPointer &config)
{
    bool ret = false;

    std::vector<String> helper_list;
    scim_get_helper_module_list (helper_list);

    _current_modules_list.clear ();

    /* Check keyboard ISEs */
    if (type != HELPER_ONLY) {
        _current_modules_list.push_back (ENGLISH_KEYBOARD_MODULE);
        std::vector<String> imengine_list;
        scim_get_imengine_module_list (imengine_list);
        for (size_t i = 0; i < imengine_list.size (); ++i) {
            _current_modules_list.push_back (imengine_list [i]);
            if (std::find (_module_names.begin (), _module_names.end (), imengine_list [i]) == _module_names.end ()) {
                if (add_keyboard_ise_module (imengine_list [i], config))
                    ret = true;
            }
        }
    }

    /* Check helper ISEs */
    for (size_t i = 0; i < helper_list.size (); ++i) {
        _current_modules_list.push_back (helper_list [i]);
        if (std::find (_module_names.begin (), _module_names.end (), helper_list [i]) == _module_names.end ()) {
            if (add_helper_ise_module (helper_list [i]))
                ret = true;
        }
    }

    /* Update _groups */
    if (ret) {
        std::vector<String> ise_langs;
        for (size_t i = 0; i < _uuids.size (); ++i) {
            scim_split_string_list (ise_langs, _langs[i]);
            for (size_t j = 0; j < ise_langs.size (); j++) {
                if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                    _groups[ise_langs[j]].push_back (i);
            }
            ise_langs.clear ();
        }
    }

    /* When load ise list is empty, all ISEs can be loaded. */
    _load_ise_list.clear ();
    return ret;
}

/**
 * @brief Set language as specific language.
 *
 * @param language The specific language.
 */
void isf_set_language (String language)
{
    if (language.length () <= 0)
        return;

    std::vector<String> langlist, all_langs;
    if (language == String ("Automatic")) {
        _disabled_langs.clear ();
        _disabled_langs_bak.clear ();
    } else {
        scim_split_string_list (langlist, language);
        isf_get_all_languages (all_langs);

        _disabled_langs.clear ();
        _disabled_langs_bak.clear ();

        for (unsigned int i = 0; i < all_langs.size (); i++) {
            if (std::find (langlist.begin (), langlist.end (), all_langs[i]) == langlist.end ()) {
                _disabled_langs.push_back (all_langs[i]);
                _disabled_langs_bak.push_back (all_langs[i]);
            }
        }
    }
    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), _disabled_langs);

    std::vector<String> enable_langs;
    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        String lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ())
            enable_langs.push_back (lang_name);
    }
    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), enable_langs);
    scim_global_config_flush ();
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
