/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
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

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT
#define Uses_SCIM_PANEL_CLIENT
#define Uses_SCIM_PANEL_AGENT

#include <stdio.h>
#include <Ecore.h>
#include <errno.h>
#include "isf_wsc_control.h"
#include "scim.h"


using namespace scim;


//#define IMFCONTROLDBG(str...) printf(str)
#define IMFCONTROLDBG(str...)
#define IMFCONTROLERR(str...) printf(str)

int get_panel_client_id ();

extern InfoManager* g_info_manager;


int _isf_wsc_context_input_panel_show (int client_id, int context, void* data, int length, bool& input_panel_show)
{
    g_info_manager->show_ise_panel (get_panel_client_id (), client_id, context, (char*)data, length);
    input_panel_show = true;
    return 0;
}

int _isf_wsc_context_input_panel_hide (int client_id, int context)
{
    g_info_manager->hide_ise_panel (get_panel_client_id (), client_id, context);
    return 0;
}

int _isf_wsc_context_control_panel_show (int context)
{
    g_info_manager->show_isf_panel (get_panel_client_id ());
    return 0;
}

int _isf_wsc_context_control_panel_hide (int context)
{
    g_info_manager->hide_isf_panel (get_panel_client_id ());
    return 0;
}

int _isf_wsc_context_input_panel_language_set (int context, Ecore_IMF_Input_Panel_Lang lang)
{
    g_info_manager->set_ise_language (get_panel_client_id (), lang);
    return 0;
}

int _isf_wsc_context_input_panel_language_locale_get (int context, char** locale)
{
    size_t datalen = 0;
    char*  data = NULL;
    g_info_manager->get_ise_language_locale (get_panel_client_id (), data, datalen);

    if (locale)
        *locale = strndup (data, datalen);

    if (data)
        delete [] data;

    return 0;
}

int _isf_wsc_context_input_panel_imdata_set (int context, const void* data, int len)
{
    g_info_manager->set_ise_imdata (get_panel_client_id (), (const char*)data, (size_t)len);
    return 0;
}

int _isf_wsc_context_input_panel_imdata_get (int context, void* data, int* len)
{
    g_info_manager->get_ise_imdata (get_panel_client_id (), (char**)&data, (size_t&)*len);
    return 0;
}

int _isf_wsc_context_input_panel_geometry_get (int context, int* x, int* y, int* w, int* h)
{
    struct rectinfo info;
    g_info_manager->get_input_panel_geometry (get_panel_client_id (), info);
    *x = info.pos_x;
    *y = info.pos_y;
    *w = info.width;
    *h = info.height;
    return 0;
}

int _isf_wsc_context_input_panel_return_key_type_set (int context, Ecore_IMF_Input_Panel_Return_Key_Type type)
{
    g_info_manager->set_ise_return_key_type (get_panel_client_id (), (int)type);
    return 0;
}

int _isf_wsc_context_input_panel_return_key_type_get (int context, Ecore_IMF_Input_Panel_Return_Key_Type& type)
{
    uint32 temp = 0;
    g_info_manager->get_ise_return_key_type (get_panel_client_id (), temp);
    type = (Ecore_IMF_Input_Panel_Return_Key_Type)temp;
    return 0;
}

int _isf_wsc_context_input_panel_return_key_disabled_set (int context, Eina_Bool disabled)
{
    g_info_manager->set_ise_return_key_disable (get_panel_client_id (), (int)disabled);
    return 0;
}

int _isf_wsc_context_input_panel_return_key_disabled_get (int context, Eina_Bool& disabled)
{
    uint32 temp = 0;
    g_info_manager->get_ise_return_key_disable (get_panel_client_id (), temp);
    disabled = (Eina_Bool)temp;
    return 0;
}

int _isf_wsc_context_input_panel_layout_set (int context, Ecore_IMF_Input_Panel_Layout layout)
{
    g_info_manager->set_ise_layout (get_panel_client_id (), layout);
    return 0;
}

int _isf_wsc_context_input_panel_layout_get (int context, Ecore_IMF_Input_Panel_Layout* layout)
{
    uint32 layout_temp;
    g_info_manager->get_ise_layout (get_panel_client_id (), layout_temp);
    *layout = (Ecore_IMF_Input_Panel_Layout)layout_temp;
    return 0;
}

int _isf_wsc_context_input_panel_state_get (int context, Ecore_IMF_Input_Panel_State& state)
{
    int temp = 1; // Hide state
    g_info_manager->get_ise_state (get_panel_client_id (), temp);
    state = (Ecore_IMF_Input_Panel_State)temp;
    return 0;
}

int _isf_wsc_context_input_panel_caps_mode_set (int context, unsigned int mode)
{
    g_info_manager->set_ise_caps_mode (get_panel_client_id (), mode);
    return 0;
}

int _isf_wsc_context_candidate_window_geometry_get (int context, int* x, int* y, int* w, int* h)
{
    struct rectinfo info;
    g_info_manager->get_candidate_window_geometry (get_panel_client_id (), info);
    *x = info.pos_x;
    *y = info.pos_y;
    *w = info.width;
    *h = info.height;
    return 0;
}

int _isf_wsc_context_input_panel_send_will_show_ack (int context)
{
    g_info_manager->will_show_ack (get_panel_client_id ());
    return 0;
}

int _isf_wsc_context_input_panel_send_will_hide_ack (int context)
{
    g_info_manager->will_hide_ack (get_panel_client_id ());
    return 0;
}

int _isf_wsc_context_set_keyboard_mode (int context, TOOLBAR_MODE_T mode)
{
    g_info_manager->set_keyboard_mode (get_panel_client_id (), mode);
    return 0;
}

int _isf_wsc_context_input_panel_send_candidate_will_hide_ack (int context)
{
    g_info_manager->candidate_will_hide_ack (get_panel_client_id ());
    return 0;
}

/*
vi:ts=4:expandtab:nowrap
*/
