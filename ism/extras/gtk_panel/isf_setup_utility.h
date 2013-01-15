/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
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

#if !defined  (__ISF_SETUP_UTILITY_H)
#define __ISF_SETUP_UTILITY_H


#include "scim.h"
#include "scim_stl_map.h"
#include <glib.h>
#include <gtk/gtk.h>
//#include <gtk/gtkimcontrol.h> /* for include USING_ISF_SWITCH_BUTTON - by jyjeon */

using namespace scim;

#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <String, std::vector <size_t>, scim_hash_string>        MapStringVectorSizeT;
typedef __gnu_cxx::hash_map <String, KeyEventList, scim_hash_string>                MapStringKeyEventList;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <String, std::vector <size_t>, scim_hash_string>              MapStringVectorSizeT;
typedef std::hash_map <String, KeyEventList, scim_hash_string>                      MapStringKeyEventList;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#else
typedef std::map <String, std::vector <size_t> >                                    MapStringVectorSizeT;
typedef std::map <String, KeyEventList>                                             MapStringKeyEventList;
typedef std::map <String, std::vector <String> >                                    MapStringVectorString;
typedef std::map <String ,String>                                                   MapStringString;
#endif


#define SCIM_UP_ICON_FILE                          (SCIM_ICONDIR "/up.png")
#define SCIM_DOWN_ICON_FILE                        (SCIM_ICONDIR "/down.png")
#define SCIM_LEFT_ICON_FILE                        (SCIM_ICONDIR "/left.png")
#define SCIM_RIGHT_ICON_FILE                       (SCIM_ICONDIR "/right.png")

#define ISF_PANEL_BG_FILE                          (SCIM_ICONDIR "/ISF_panel_bg.png")
#define ISF_ICON_HELP_FILE                         (SCIM_ICONDIR "/ISF_icon_31_help.png")
#define ISF_ICON_HELP_T_FILE                       (SCIM_ICONDIR "/ISF_icon_31_help_t.png")
#define ISF_CANDIDATE_BG_FILE                      (SCIM_ICONDIR "/ISF_candidate_bg.png")
#define ISF_CANDIDATE_DIVIDER_FILE                 (SCIM_ICONDIR "/ISF_candidate_divider.png")

#define BASE_SCREEN_WIDTH                          240.0
#define BASE_SCREEN_HEIGHT                         400.0
#define BASE_SOFTKEYBAR_HEIGHT                     48.0
#define BASE_PANEL_WIDTH                           230.0
#define BASE_PANEL_HEIGHT                          198.0

/*setup button*/
#define BASE_SETUP_BUTTON_WIDTH                    75.0
#define BASE_SETUP_BUTTON_HEIGHT                   31.0

/*help button*/
#define BASE_HELP_ICON_WIDTH                       31.0
#define BASE_HELP_ICON_HEIGHT                      31.0

//#define USING_ISF_SWITCH_BUTTON


void get_all_languages (std::vector<String> &all_langs);
void get_selected_languages (std::vector<String> &selected_langs);
void get_all_iselist_in_languages (std::vector<String> lang_list, std::vector<String> &all_iselist);
void get_interested_iselist_in_languages (std::vector<String> lang_list, std::vector<String> &interested);

#endif // __ISF_SETUP_UTILITY_H

/*
vi:ts=4:ai:nowrap:expandtab
*/
