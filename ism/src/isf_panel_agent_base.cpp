/** @file isf_panel_agent_base.cpp
 *  @brief Implementation of class PanelAgentBase.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2016 Samsung Electronics Co., Ltd.
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
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_UTILITY

#include <string.h>
#include <sys/types.h>
#include <sys/times.h>
#include <dlog.h>
#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_debug.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_PANEL_AGENT_BASE"


namespace scim
{


PanelAgentBase::PanelAgentBase (const String& name)
    :m_name (name)
{
}

PanelAgentBase::~PanelAgentBase ()
{
}

bool PanelAgentBase::initialize (InfoManager* info_manager, const String& display, bool resident)
{
    LOGW ("not implemented for %s", m_name.c_str ());
    return false;
}

bool PanelAgentBase::valid (void) const
{
    LOGW ("not implemented for %s", m_name.c_str ());
    return false;
}

void PanelAgentBase::stop (void)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}


void PanelAgentBase::update_panel_event (int client,  uint32 context, int cmd, uint32 nType, uint32 nValue)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::reset_keyboard_ise (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_keyboard_ise_list (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::change_factory (int client, uint32 context, const String&  uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::helper_candidate_show (int client, uint32 context, const String&  uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::helper_candidate_hide (int client, uint32 context, const String&  uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

/* if the uuid is empty, it must send to panelclient otherwise HelperAgent
 */
void PanelAgentBase::candidate_more_window_show (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
/* if the uuid is empty, it must send to panelclient otherwise HelperAgent
 */
void PanelAgentBase::candidate_more_window_hide (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_helper_lookup_table (int client, uint32 context, const String&  uuid, const LookupTable& table)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//select_aux
//helper_select_aux
//SCIM_TRANS_CMD_SELECT_AUX
//this function called by two places, will send message to help(with uuid) or app
void PanelAgentBase::select_aux (int client, uint32 context, uint32 item)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//SCIM_TRANS_CMD_SELECT_CANDIDATE
//this function called by two places, will send message to help(with uuid) or app
void PanelAgentBase::select_candidate (int client, uint32 context, uint32 item)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::lookup_table_page_up (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::lookup_table_page_down (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_lookup_table_page_size (int client, uint32 context, uint32 size)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_candidate_item_layout (int client, uint32 context, const std::vector<uint32>& row_items)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::select_associate (int client, uint32 context, uint32 item)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::associate_table_page_up (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::associate_table_page_down (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_associate_table_page_size (int client, uint32 context, uint32 size)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::update_displayed_candidate_number (int client, uint32 context, uint32 size)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::send_longpress_event (int client, uint32 context, uint32 index)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::trigger_property (int client, uint32 context, const String&  property)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_start_helper (int client, uint32 context, const String& ic_uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}


void PanelAgentBase::exit (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::focus_out_helper (int client, uint32 context, const String& uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::focus_in_helper (int client, uint32 context, const String& uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::show_helper (int client, uint32 context, const String& uuid, char* data, size_t& len)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::hide_helper (int client, uint32 context, const String& uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_mode (int client, uint32 context, const String& uuid, uint32& mode)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_language (int client, uint32 context, const String& uuid, uint32& language)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_imdata (int client, uint32 context, const String& uuid, const char* imdata, size_t& len)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_return_key_type (int client, uint32 context, const String& uuid, uint32 type)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::get_helper_return_key_type (int client, uint32 context, const String& uuid, _OUT_ uint32& type)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_return_key_disable (int client, uint32 context, const String& uuid, uint32 disabled)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::get_helper_return_key_disable (int client, uint32 context, const String& uuid, _OUT_ uint32& disabled)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_layout (int client, uint32 context, const String& uuid, uint32& layout)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_input_mode (int client, uint32 context, const String& uuid, uint32& mode)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_input_hint (int client, uint32 context, const String& uuid, uint32& hint)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_bidi_direction (int client, uint32 context, const String& uuid, uint32& direction)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::set_helper_caps_mode (int client, uint32 context, const String& uuid, uint32& mode)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::show_helper_option_window (int client, uint32 context, const String& uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

bool PanelAgentBase::process_key_event (int client, uint32 context, const String& uuid, KeyEvent& key, uint32 serial)
{
    LOGW ("not implemented for %s", m_name.c_str ());
    return false;
}

bool PanelAgentBase::get_helper_geometry (int client, uint32 context, String& uuid, _OUT_ struct rectinfo& info)
{
    LOGW ("not implemented for %s", m_name.c_str ());
    return false;
}

void PanelAgentBase::get_helper_imdata (int client, uint32 context, String& uuid, _OUT_ char** imdata, _OUT_ size_t& len)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::get_helper_layout (int client, uint32 context, String& uuid, uint32& layout)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}


void PanelAgentBase::get_ise_language_locale (int client, uint32 context, String& uuid, _OUT_ char** data, _OUT_ size_t& len)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::check_option_window (int client, uint32 context, String& uuid, _OUT_ uint32& avail)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::reset_ise_option (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::reset_helper_context (int client, uint32 context, const String& uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::reload_config (int client)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_update_surrounding_text (int client, uint32 context, const String& uuid, String& text, uint32 cursor)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_update_selection (int client, uint32 context, String& uuid, String text)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_get_keyboard_ise_list (int client, uint32 context, const String& uuid, std::vector<String>& list)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_get_candidate_ui (int client, uint32 context, const String& uuid,  int style,  int mode)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_get_candidate_geometry (int client, uint32 context, const String& uuid, struct rectinfo& info)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
void PanelAgentBase::socket_get_keyboard_ise (int client, uint32 context, const String& uuid, String& ise_name, String& ise_uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::helper_detach_input_context (int client, uint32 context, const String& ic_uuid)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::helper_process_imengine_event (int client, uint32 context, const String& ic_uuid, const Transaction& nest_transaction)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::process_helper_event (int client, uint32 context, String target_uuid, String active_uuid, Transaction& nest_trans)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::socket_helper_key_event (int client, uint32 context, int cmd , KeyEvent& key)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//SCIM_TRANS_CMD_GET_SURROUNDING_TEXT
//socket_helper_get_surrounding_text
void PanelAgentBase::socket_helper_get_surrounding_text (int client, uint32 context, uint32 maxlen_before, uint32 maxlen_after)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT
//socket_helper_delete_surrounding_text
void PanelAgentBase::socket_helper_delete_surrounding_text (int client, uint32 context, uint32 offset, uint32 len)
{
    LOGD ("not implemented ");
}
//SCIM_TRANS_CMD_GET_SELECTION
void PanelAgentBase::socket_helper_get_selection (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//SCIM_TRANS_CMD_SET_SELECTION
void PanelAgentBase::socket_helper_set_selection (int client, uint32 context, uint32 start, uint32 end)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//socket_helper_update_input_context
//ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT
void PanelAgentBase::update_ise_input_context (int client, uint32 context, uint32 type, uint32 value)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//socket_helper_send_private_command
//SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND
void PanelAgentBase::send_private_command (int client, uint32 context, const String& command)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
void PanelAgentBase::helper_all_update_spot_location (int client, uint32 context, String uuid, int x, int y)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//ISM_TRANS_CMD_UPDATE_CURSOR_POSITION
void PanelAgentBase::helper_all_update_cursor_position (int client, uint32 context, String uuid, int cursor_pos)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//SCIM_TRANS_CMD_UPDATE_SCREEN
void PanelAgentBase::helper_all_update_screen (int client, uint32 context, String uuid, int screen)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

//socket_helper_commit_string
//SCIM_TRANS_CMD_COMMIT_STRING
void PanelAgentBase::commit_string (int client, uint32 context,const WideString& wstr)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//socket_helper_show_preedit_string
//SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
void PanelAgentBase::show_preedit_string (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//socket_helper_hide_preedit_string
//SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
void PanelAgentBase::hide_preedit_string (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//socket_helper_update_preedit_string
//SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
void PanelAgentBase::update_preedit_string (int client, uint32 context, WideString wstr, AttributeList& attrs, uint32 caret)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//socket_helper_update_preedit_caret
//SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
void PanelAgentBase::update_preedit_caret (int client, uint32 context, uint32 caret)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}
//socket_helper_register_helper
//SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT
//SCIM_TRANS_CMD_UPDATE_SCREEN
void PanelAgentBase::helper_attach_input_context_and_update_screen (int client, std::vector < std::pair <uint32, String> >& helper_ic_index,
        uint32 current_screen)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

void PanelAgentBase::hide_helper_ise (int client, uint32 context)
{
    LOGW ("not implemented for %s", m_name.c_str ());
}

bool PanelAgentBase::process_input_device_event(int client, uint32 context, const String& uuid, uint32 type, const char *data, size_t len, _OUT_ uint32& result)
{
    LOGW("not implemented for %s", m_name.c_str());
    return false;
}

void PanelAgentBase::process_key_event_done(int client, uint32 context, KeyEvent &key, uint32 ret, uint32 serial)
{
    LOGW("not implemented for %s", m_name.c_str());
}

void PanelAgentBase::set_autocapital_type(int client, uint32 context, String uuid, int mode)
{
    LOGW("not implemented for %s", m_name.c_str());
}


} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/
