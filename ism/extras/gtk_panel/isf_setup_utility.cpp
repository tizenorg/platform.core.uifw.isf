/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
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

#define Uses_SCIM_PANEL_AGENT


#include "isf_setup_utility.h"

MapStringVectorSizeT         _groups;
std::vector < String >       _uuids;
std::vector < String >       _names;
std::vector < String >       _module_names;
std::vector < String >       _langs;
std::vector < String >       _icons;
std::vector < uint32 >       _options;
std::vector <TOOLBAR_MODE_T> _modes;
MapStringString              _keyboard_ise_help_map;
MapStringVectorString        _disabled_ise_map;
std::vector < String >       _disabled_langs;
MapStringVectorString        _disabled_ise_map_bak;
std::vector < String >       _disabled_langs_bak;
std::vector < String >       _isf_app_list;
gboolean                     _ise_list_changed      = false;
gboolean                     _setup_enable_changed  = false;
gboolean                     _lang_selected_changed = false;


void get_all_languages (std::vector<String> &all_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        all_langs.push_back (lang_name);
    }
}

void get_selected_languages (std::vector<String> &selected_langs)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ())
            selected_langs.push_back (lang_name);
    }
}

void get_all_iselist_in_languages (std::vector<String> lang_list, std::vector<String> &all_iselist)
{
    String lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);

        if (std::find (lang_list.begin (), lang_list.end (), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (std::find (all_iselist.begin (), all_iselist.end (), _names[it->second[i]]) == all_iselist.end ())
                    all_iselist.push_back (_names[it->second[i]]);
            }
        }
    }
}

void get_interested_iselist_in_languages (std::vector<String> lang_list, std::vector<String> &interested)
{
    String lang_name;
    String active_app = _isf_app_list [0];

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name_english (it->first);
        if (std::find(lang_list.begin(), lang_list.end(), lang_name) != lang_list.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (std::find (_disabled_ise_map[active_app].begin (), _disabled_ise_map[active_app].end (), _uuids[it->second[i]])
                        ==_disabled_ise_map[active_app].end ()) {
                    // Protect from add the same ise more than once in case of multiple langs.-->f_show list
                    if (std::find (interested.begin (), interested.end (), _names[it->second[i]]) == interested.end ())
                        interested.push_back (_names[it->second[i]]);
                }
            }
        }
    }
}


/*
vi:ts=4:nowrap:ai:expandtab
*/
