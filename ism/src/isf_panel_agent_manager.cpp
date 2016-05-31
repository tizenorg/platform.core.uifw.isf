/** @file isf_panel_agent_manager.cpp
 *  @brief Implementation of class PanelAgentManager.
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
#define Uses_SCIM_PANEL_AGENT_MODULE

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
#define LOG_TAG             "ISF_PANEL_AGENT_MANAGER"

//FIXME
//remove this
#define WAYLAND_MODULE_CLIENT_ID (0)


namespace scim
{


//==================================== PanelAgent ===========================
class PanelAgentManager::PanelAgentManagerImpl
{
public:
    std::vector<PanelAgentModule*> m_panel_agent_modules;
    std::map <int, PanelAgentPointer> m_client_repository;

    PanelAgentPointer m_socket;
    PanelAgentPointer m_wayland;
public:
    PanelAgentManagerImpl () {
    }

    ~PanelAgentManagerImpl () {
        m_socket.reset(0);
        m_wayland.reset(0);
        std::vector<PanelAgentModule*>::iterator iter = m_panel_agent_modules.begin ();

        for (; iter != m_panel_agent_modules.end (); iter++) {
            (*iter)->unload();
            delete *iter;
        }

        m_panel_agent_modules.clear ();
    }

    PanelAgentPointer get_panel_agent_by_id (int id) {
        if (id == WAYLAND_MODULE_CLIENT_ID)
            return m_wayland;
        else
            return m_socket;
    }
};

PanelAgentManager::PanelAgentManager ()
    :m_impl (new PanelAgentManagerImpl ())
{
}

PanelAgentManager::~PanelAgentManager ()
{
    delete m_impl;
}

bool PanelAgentManager::initialize (InfoManager* info_manager, const ConfigPointer& config, const String& display, bool resident)
{
    std::vector <String> mod_list;
    scim_get_panel_agent_module_list (mod_list);

    for (std::vector<String>::iterator it = mod_list.begin (); it != mod_list.end (); ++it) {
        LOGD ("Trying to load panelagent module %s", it->c_str ());
        PanelAgentModule* _panel_agent_module = new PanelAgentModule;

        if (!_panel_agent_module) {
            LOGD ("");
            continue;
        }

        _panel_agent_module->load (*it, config);

        if (!_panel_agent_module->valid ()) {
            LOGD ("");
            delete _panel_agent_module;
            continue;
        }

        PanelAgentPointer _panelagent = _panel_agent_module->get_instance ();

        if (_panelagent.null ()) {
            LOGD ("");
            delete _panel_agent_module;
            continue;
        }

        _panelagent->initialize (info_manager, display, resident);
        LOGD ("PanelAgent Module %s loaded", _panel_agent_module->get_module_name ().c_str ());
        m_impl->m_panel_agent_modules.push_back (_panel_agent_module);

        if (_panel_agent_module->get_module_name () == "wayland")
            m_impl->m_wayland = _panelagent;
        else
            m_impl->m_socket = _panelagent;
    }

    return true;
}

bool PanelAgentManager::valid (void) const
{
    return true;
}

void PanelAgentManager::stop (void)
{
    SCIM_DEBUG_MAIN (1) << "PanelAgent::stop ()\n";
}

void PanelAgentManager::update_panel_event (int id,  uint32 context_id, int cmd, uint32 nType, uint32 nValue)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_panel_event (id, context_id, cmd, nType, nValue);
}

void PanelAgentManager::reset_keyboard_ise (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->reset_keyboard_ise (id, context_id);
}

void PanelAgentManager::update_keyboard_ise_list (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_keyboard_ise_list (id, context_id);
}

void PanelAgentManager::change_factory (int id, uint32 context_id, const String&  uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->change_factory (id, context_id, uuid);
}

void PanelAgentManager::helper_candidate_show (int id, uint32 context_id, const String&  uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_candidate_show (id, context_id, uuid);
}

void PanelAgentManager::helper_candidate_hide (int id, uint32 context_id, const String&  uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_candidate_hide (id, context_id, uuid);
}

void PanelAgentManager::candidate_more_window_show (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->candidate_more_window_show (id, context_id);
}

void PanelAgentManager::candidate_more_window_hide (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->candidate_more_window_hide (id, context_id);
}

void PanelAgentManager::update_helper_lookup_table (int id, uint32 context_id, const String&  uuid, const LookupTable& table)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_helper_lookup_table (id, context_id, uuid, table);
}

void PanelAgentManager::select_aux (int id, uint32 context_id, uint32 item)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->select_aux (id, context_id, item);
}

void PanelAgentManager::select_candidate (int id, uint32 context_id, uint32 item)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->select_candidate (id, context_id, item);
}

void PanelAgentManager::lookup_table_page_up (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->lookup_table_page_up (id, context_id);
}

void PanelAgentManager::lookup_table_page_down (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->lookup_table_page_down (id, context_id);
}

void PanelAgentManager::update_lookup_table_page_size (int id, uint32 context_id, uint32 size)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_lookup_table_page_size (id, context_id, size);
}

void PanelAgentManager::update_candidate_item_layout (int id, uint32 context_id, const std::vector<uint32>& row_items)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_candidate_item_layout (id, context_id, row_items);
}

void PanelAgentManager::select_associate (int id, uint32 context_id, uint32 item)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->select_associate (id, context_id, item);
}

void PanelAgentManager::associate_table_page_up (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->associate_table_page_up (id, context_id);
}

void PanelAgentManager::associate_table_page_down (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->associate_table_page_down (id, context_id);
}

void PanelAgentManager::update_associate_table_page_size (int id, uint32 context_id, uint32 size)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_associate_table_page_size (id, context_id, size);
}

void PanelAgentManager::update_displayed_candidate_number (int id, uint32 context_id, uint32 size)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_displayed_candidate_number (id, context_id, size);
}

void PanelAgentManager::send_longpress_event (int id, uint32 context_id, uint32 index)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->send_longpress_event (id, context_id, index);
}

void PanelAgentManager::trigger_property (int id, uint32 context_id, const String&  property)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->trigger_property (id, context_id, property);
}

void PanelAgentManager::socket_start_helper (int id, uint32 context_id, const String& ic_uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_start_helper (id, context_id, ic_uuid);
}


void PanelAgentManager::exit (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->exit (id, context_id);
}

void PanelAgentManager::focus_out_helper (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->focus_out_helper (id, context_id, uuid);
}

void PanelAgentManager::focus_in_helper (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->focus_in_helper (id, context_id, uuid);
}

void PanelAgentManager::show_helper (int id, uint32 context_id, const String& uuid, char* data, size_t& len)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->show_helper (id, context_id, uuid, data, len);
}

void PanelAgentManager::hide_helper (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->hide_helper (id, context_id, uuid);
}

void PanelAgentManager::set_helper_mode (int id, uint32 context_id, const String& uuid, uint32& mode)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_mode (id, context_id, uuid, mode);
}

void PanelAgentManager::set_helper_language (int id, uint32 context_id, const String& uuid, uint32& language)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_language (id, context_id, uuid, language);
}

void PanelAgentManager::set_helper_imdata (int id, uint32 context_id, const String& uuid, const char* imdata, size_t& len)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_imdata (id, context_id, uuid, imdata, len);
}

void PanelAgentManager::set_helper_return_key_type (int id, uint32 context_id, const String& uuid, uint32 type)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_return_key_type (id, context_id, uuid, type);
}

void PanelAgentManager::get_helper_return_key_type (int id, uint32 context_id, const String& uuid, _OUT_ uint32& type)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->get_helper_return_key_type (id, context_id, uuid, type);
}

void PanelAgentManager::set_helper_return_key_disable (int id, uint32 context_id, const String& uuid, uint32 disabled)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_return_key_disable (id, context_id, uuid, disabled);
}

void PanelAgentManager::get_helper_return_key_disable (int id, uint32 context_id, const String& uuid, _OUT_ uint32& disabled)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->get_helper_return_key_disable (id, context_id, uuid, disabled);
}

void PanelAgentManager::set_helper_layout (int id, uint32 context_id, const String& uuid, uint32& layout)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_layout (id, context_id, uuid, layout);
}

void PanelAgentManager::set_helper_input_mode (int id, uint32 context_id, const String& uuid, uint32& mode)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_input_mode (id, context_id, uuid, mode);
}

void PanelAgentManager::set_helper_input_hint (int id, uint32 context_id, const String& uuid, uint32& hint)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_input_hint (id, context_id, uuid, hint);
}

void PanelAgentManager::set_helper_bidi_direction (int id, uint32 context_id, const String& uuid, uint32& direction)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_bidi_direction (id, context_id, uuid, direction);
}

void PanelAgentManager::set_helper_caps_mode (int id, uint32 context_id, const String& uuid, uint32& mode)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->set_helper_caps_mode (id, context_id, uuid, mode);
}

void PanelAgentManager::show_helper_option_window (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->show_helper_option_window (id, context_id, uuid);
}

bool PanelAgentManager::process_key_event (int id, uint32 context_id, const String& uuid, KeyEvent& key, uint32 serial)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        return _p->process_key_event (id, context_id, uuid, key, serial);
    return false;
}

bool PanelAgentManager::get_helper_geometry (int id, uint32 context_id, String& uuid, _OUT_ struct rectinfo& info)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        return _p->get_helper_geometry (id, context_id, uuid, info);
    return false;
}

void PanelAgentManager::get_helper_imdata (int id, uint32 context_id, String& uuid, _OUT_ char** imdata, _OUT_ size_t& len)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->get_helper_imdata (id, context_id, uuid, imdata, len);
}

void PanelAgentManager::get_helper_layout (int id, uint32 context_id, String& uuid, uint32& layout)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->get_helper_layout (id, context_id, uuid, layout);
}

void PanelAgentManager::get_ise_language_locale (int id, uint32 context_id, String& uuid, _OUT_ char** data, _OUT_ size_t& len)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->get_ise_language_locale (id, context_id, uuid, data, len);
}

void PanelAgentManager::check_option_window (int id, uint32 context_id, String& uuid, _OUT_ uint32& avail)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->check_option_window (id, context_id, uuid, avail);
}

void PanelAgentManager::reset_ise_option (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->reset_ise_option (id, context_id);
}

void PanelAgentManager::reset_helper_context (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->reset_helper_context (id, context_id, uuid);
}

void PanelAgentManager::reload_config (int id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->reload_config (id);
}

void PanelAgentManager::socket_update_surrounding_text (int id, uint32 context_id, const String& uuid, String& text, uint32 cursor)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_update_surrounding_text (id, context_id, uuid, text, cursor);
}

void PanelAgentManager::socket_remoteinput_focus_in (int id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_remoteinput_focus_in (id);
}

void PanelAgentManager::socket_remoteinput_focus_out (int id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_remoteinput_focus_out (id);
}

void PanelAgentManager::socket_remoteinput_entry_metadata (int id, uint32 hint, uint32 layout, int variation, uint32 autocapital_type)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_remoteinput_entry_metadata (id, hint, layout, variation, autocapital_type);
}

void PanelAgentManager::socket_remoteinput_default_text (int id, String& text, uint32 cursor)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_remoteinput_default_text (id, text, cursor);
}

void PanelAgentManager::socket_update_selection (int id, uint32 context_id, String& uuid, String text)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_update_selection (id, context_id, uuid, text);
}

void PanelAgentManager::socket_get_keyboard_ise_list (int id, uint32 context_id, const String& uuid, std::vector<String>& list)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_get_keyboard_ise_list (id, context_id, uuid, list);
}

void PanelAgentManager::socket_get_candidate_ui (int id, uint32 context_id, const String& uuid,  int style,  int mode)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_get_candidate_ui (id, context_id, uuid, style, mode);
}

void PanelAgentManager::socket_get_candidate_geometry (int id, uint32 context_id, const String& uuid, struct rectinfo& info)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_get_candidate_geometry (id, context_id, uuid, info);
}

void PanelAgentManager::socket_get_keyboard_ise (int id, uint32 context_id, const String& uuid, String& ise_name, String& ise_uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_get_keyboard_ise (id, context_id, uuid, ise_name, ise_uuid);
}

void PanelAgentManager::helper_detach_input_context (int id, uint32 context_id, const String& uuid)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_detach_input_context (id, context_id, uuid);
}

void PanelAgentManager::helper_process_imengine_event (int id, uint32 context_id, const String& uuid, const Transaction& nest_transaction)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_process_imengine_event (id, context_id, uuid, nest_transaction);
}

void PanelAgentManager::process_helper_event (int id, uint32 context_id, String target_uuid, String active_uuid, Transaction& nest_trans)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->process_helper_event (id, context_id, target_uuid, active_uuid, nest_trans);
}

bool PanelAgentManager::process_input_device_event(int id, uint32 context_id, const String& uuid, uint32 type, const char *data, size_t len, _OUT_ uint32& result)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id(id);

    if (!_p.null())
        return _p->process_input_device_event(id, context_id, uuid, type, data, len, result);
    return false;
}

void PanelAgentManager::socket_helper_key_event (int id, uint32 context_id, int cmd , KeyEvent& key)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_helper_key_event (id, context_id, cmd, key);
}

void PanelAgentManager::socket_helper_get_surrounding_text (int id, uint32 context_id, uint32 maxlen_before, uint32 maxlen_after)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_helper_get_surrounding_text (id, context_id, maxlen_before, maxlen_after);
}

void PanelAgentManager::socket_helper_delete_surrounding_text (int id, uint32 context_id, uint32 offset, uint32 len)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_helper_delete_surrounding_text (id, context_id, offset, len);
}

void PanelAgentManager::socket_helper_get_selection (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_helper_get_selection (id, context_id);
}

void PanelAgentManager::socket_helper_set_selection (int id, uint32 context_id, uint32 start, uint32 end)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->socket_helper_set_selection (id, context_id, start, end);
}

void PanelAgentManager::update_ise_input_context (int id, uint32 context_id, uint32 type, uint32 value)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_ise_input_context (id, context_id, type, value);
}

void PanelAgentManager::send_private_command (int id, uint32 context_id, String command)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->send_private_command (id, context_id, command);
}

void PanelAgentManager::helper_all_update_spot_location (int id, uint32 context_id, String uuid, int x, int y)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_all_update_spot_location (id, context_id, uuid, x, y);
}

void PanelAgentManager::helper_all_update_cursor_position (int id, uint32 context_id, String uuid, int cursor_pos)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_all_update_cursor_position (id, context_id, uuid, cursor_pos);
}

void PanelAgentManager::helper_all_update_screen (int id, uint32 context_id, String uuid, int screen)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_all_update_screen (id, context_id, uuid, screen);
}

void PanelAgentManager::commit_string (int id, uint32 context_id, const WideString& wstr)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->commit_string (id, context_id, wstr);
}

void PanelAgentManager::show_preedit_string (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->show_preedit_string (id, context_id);
}

void PanelAgentManager::hide_preedit_string (int id, uint32 context_id)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->hide_preedit_string (id, context_id);
}

void PanelAgentManager::update_preedit_string (int id, uint32 context_id, WideString wstr, AttributeList& attrs, uint32 caret)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_preedit_string (id, context_id, wstr, attrs, caret);
}

void PanelAgentManager::update_preedit_caret (int id, uint32 context_id, uint32 caret)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->update_preedit_caret (id, context_id, caret);
}

void PanelAgentManager::helper_attach_input_context_and_update_screen (int id, std::vector < std::pair <uint32, String> >& helper_ic_index,
        uint32 current_screen)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->helper_attach_input_context_and_update_screen (id, helper_ic_index, current_screen);
}

void PanelAgentManager::hide_helper_ise (int id, uint32 context)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->hide_helper_ise (id, context);
}

void PanelAgentManager::process_key_event_done(int id, uint32 context_id, KeyEvent &key, uint32 ret, uint32 serial)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->process_key_event_done (id, context_id, key, ret, serial);
}

void PanelAgentManager::send_key_event (int id, uint32 context_id, const KeyEvent &key)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->send_key_event (id, context_id, key);
}

void PanelAgentManager::forward_key_event (int id, uint32 context_id, const KeyEvent &key)
{
    PanelAgentPointer _p = m_impl->get_panel_agent_by_id (id);

    if (!_p.null ())
        _p->forward_key_event (id, context_id, key);
}


} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/
