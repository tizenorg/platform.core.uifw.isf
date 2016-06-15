/** @file scim_helper.cpp
 *  @brief Implementation of class HelperAgent.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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
 * 1. Add new interface APIs for keyboard ISE
 *    a. expand_candidate (), contract_candidate () and set_candidate_style ()
 *    b. set_keyboard_ise_by_uuid () and reset_keyboard_ise ()
 *    c. get_surrounding_text () and delete_surrounding_text ()
 *    d. show_preedit_string (), hide_preedit_string (), update_preedit_string () and update_preedit_caret ()
 *    e. show_candidate_string (), hide_candidate_string () and update_candidate_string ()
 *
 * $Id: scim_helper.cpp,v 1.13 2005/05/24 12:22:51 suzhe Exp $
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT
#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "scim_private.h"
#include "scim.h"
#include <scim_panel_common.h>
#include "isf_query_utility.h"
#include <dlog.h>

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "SCIM_HELPER"


//FIXME: remove this definitions
#define SHIFT_MODE_OFF  0xffe1
#define SHIFT_MODE_ON   0xffe2
#define SHIFT_MODE_LOCK 0xffe6
#define SHIFT_MODE_ENABLE 0x9fe7
#define SHIFT_MODE_DISABLE 0x9fe8

namespace scim {

typedef Signal3<void, const HelperAgent *, int, const String &>
        HelperAgentSignalVoid;

typedef Signal4<void, const HelperAgent *, int, const String &, const String &>
        HelperAgentSignalString;

typedef Signal4<void, const HelperAgent *, int, const String &, const std::vector<String> &>
        HelperAgentSignalStringVector;

typedef Signal5<void, const HelperAgent *, int, const String &, const String &, const String &>
        HelperAgentSignalString2;

typedef Signal4<void, const HelperAgent *, int, const String &, int>
        HelperAgentSignalInt;

typedef Signal5<void, const HelperAgent *, int, const String &, int, int>
        HelperAgentSignalIntInt;

typedef Signal4<void, const HelperAgent *, int, const String &, const Transaction &>
        HelperAgentSignalTransaction;

typedef Signal4<void, const HelperAgent *, int, const String &, const rectinfo &>
        HelperAgentSignalRect;

typedef Signal2<void, const HelperAgent *, struct rectinfo &>
        HelperAgentSignalSize;

typedef Signal2<void, const HelperAgent *, uint32 &>
        HelperAgentSignalUintVoid;

typedef Signal3<void, const HelperAgent *, int, uint32 &>
        HelperAgentSignalIntUint;

typedef Signal3 <void, const HelperAgent *, char *, size_t &>
        HelperAgentSignalRawVoid;

typedef Signal3 <void, const HelperAgent *, char **, size_t &>
        HelperAgentSignalGetRawVoid;

typedef Signal4 <void, const HelperAgent *, int, char *, size_t &>
        HelperAgentSignalIntRawVoid;

typedef Signal3 <void, const HelperAgent *, int, char **>
        HelperAgentSignalIntGetStringVoid;

typedef Signal2<void, const HelperAgent *, const std::vector<uint32> &>
        HelperAgentSignalUintVector;

typedef Signal2<void, const HelperAgent *, LookupTable &>
        HelperAgentSignalLookupTable;

typedef Signal3<void, const HelperAgent *, KeyEvent &, uint32 &>
        HelperAgentSignalKeyEventUint;

typedef Signal5<void, const HelperAgent *, uint32 &, char *, size_t &, uint32 &>
        HelperAgentSignalUintCharSizeUint;

class HelperAgent::HelperAgentImpl
{
public:
    SocketClient socket;
    SocketClient socket_active;
    Transaction  recv;
    Transaction  send;
    uint32       magic;
    uint32       magic_active;
    int          timeout;
    uint32       focused_ic;

    HelperAgent* thiz;
    IMEngineInstancePointer si;
    ConfigPointer m_config;
    IMEngineModule engine_module;

    char* surrounding_text;
    char* selection_text;
    uint32 cursor_pos;
    bool need_update_surrounding_text;
    bool need_update_selection_text;
    uint32 layout;

    HelperAgentSignalVoid           signal_exit;
    HelperAgentSignalVoid           signal_attach_input_context;
    HelperAgentSignalVoid           signal_detach_input_context;
    HelperAgentSignalVoid           signal_reload_config;
    HelperAgentSignalInt            signal_update_screen;
    HelperAgentSignalIntInt         signal_update_spot_location;
    HelperAgentSignalInt            signal_update_cursor_position;
    HelperAgentSignalInt            signal_update_surrounding_text;
    HelperAgentSignalVoid           signal_update_selection;
    HelperAgentSignalString         signal_trigger_property;
    HelperAgentSignalTransaction    signal_process_imengine_event;
    HelperAgentSignalVoid           signal_focus_out;
    HelperAgentSignalVoid           signal_focus_in;
    HelperAgentSignalIntRawVoid     signal_ise_show;
    HelperAgentSignalVoid           signal_ise_hide;
    HelperAgentSignalVoid           signal_candidate_show;
    HelperAgentSignalVoid           signal_candidate_hide;
    HelperAgentSignalSize           signal_get_geometry;
    HelperAgentSignalUintVoid       signal_set_mode;
    HelperAgentSignalUintVoid       signal_set_language;
    HelperAgentSignalRawVoid        signal_set_imdata;
    HelperAgentSignalGetRawVoid     signal_get_imdata;
    HelperAgentSignalIntGetStringVoid   signal_get_language_locale;
    HelperAgentSignalUintVoid           signal_set_return_key_type;
    HelperAgentSignalUintVoid           signal_get_return_key_type;
    HelperAgentSignalUintVoid           signal_set_return_key_disable;
    HelperAgentSignalUintVoid           signal_get_return_key_disable;
    HelperAgentSignalUintVoid           signal_set_layout;
    HelperAgentSignalUintVoid           signal_get_layout;
    HelperAgentSignalUintVoid           signal_set_caps_mode;
    HelperAgentSignalVoid               signal_reset_input_context;
    HelperAgentSignalIntInt             signal_update_candidate_ui;
    HelperAgentSignalRect               signal_update_candidate_geometry;
    HelperAgentSignalString2            signal_update_keyboard_ise;
    HelperAgentSignalStringVector       signal_update_keyboard_ise_list;
    HelperAgentSignalVoid               signal_candidate_more_window_show;
    HelperAgentSignalVoid               signal_candidate_more_window_hide;
    HelperAgentSignalLookupTable        signal_update_lookup_table;
    HelperAgentSignalInt                signal_select_aux;
    HelperAgentSignalInt                signal_select_candidate;
    HelperAgentSignalVoid               signal_candidate_table_page_up;
    HelperAgentSignalVoid               signal_candidate_table_page_down;
    HelperAgentSignalInt                signal_update_candidate_table_page_size;
    HelperAgentSignalUintVector         signal_update_candidate_item_layout;
    HelperAgentSignalInt                signal_select_associate;
    HelperAgentSignalVoid               signal_associate_table_page_up;
    HelperAgentSignalVoid               signal_associate_table_page_down;
    HelperAgentSignalInt                signal_update_associate_table_page_size;
    HelperAgentSignalVoid               signal_reset_ise_context;
    HelperAgentSignalUintVoid           signal_turn_on_log;
    HelperAgentSignalInt                signal_update_displayed_candidate_number;
    HelperAgentSignalInt                signal_longpress_candidate;
    HelperAgentSignalKeyEventUint       signal_process_key_event;
    HelperAgentSignalUintVoid           signal_set_input_mode;
    HelperAgentSignalUintVoid           signal_set_input_hint;
    HelperAgentSignalUintVoid           signal_update_bidi_direction;
    HelperAgentSignalVoid               signal_show_option_window;
    HelperAgentSignalUintVoid           signal_check_option_window;
    HelperAgentSignalUintCharSizeUint   signal_process_input_device_event;

public:
    HelperAgentImpl (HelperAgent* thiz) : focused_ic ((uint32) -1), thiz (thiz),
        surrounding_text (NULL), selection_text (NULL), cursor_pos (0),
        need_update_surrounding_text (false), need_update_selection_text (false),
        layout (0) {
    }

    ~HelperAgentImpl () {
        if (!si.null ()) {
            si.reset ();
        }

        if (surrounding_text != NULL)
            free (surrounding_text);

        if (selection_text != NULL)
            free (selection_text);

        if (engine_module.valid ()) {
            engine_module.unload ();
        }
    }

    // Implementation of slot functions
    void
    slot_show_preedit_string (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->show_preedit_string (focused_ic, "");
    }

    void
    slot_show_aux_string (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->show_aux_string ();
    }

    void
    slot_show_lookup_table (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->show_candidate_string ();
    }

    void
    slot_hide_preedit_string (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->hide_preedit_string (focused_ic, "");
    }

    void
    slot_hide_aux_string (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->hide_aux_string ();
    }

    void
    slot_hide_lookup_table (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->hide_candidate_string ();
    }

    void
    slot_update_preedit_caret (IMEngineInstanceBase *si, int caret)
    {
        LOGD ("");
        thiz->update_preedit_caret (caret);
    }

    void
    slot_update_preedit_string (IMEngineInstanceBase *si,
                                const WideString & str,
                                const AttributeList & attrs,
                                int caret)
    {
        LOGD ("");
        thiz->update_preedit_string (-1, "", str, attrs, caret);
    }

    void
    slot_update_aux_string (IMEngineInstanceBase *si,
                            const WideString & str,
                            const AttributeList & attrs)
    {
        LOGD ("");
        thiz->update_aux_string (utf8_wcstombs(str), attrs);
    }

    void
    slot_commit_string (IMEngineInstanceBase *si,
                        const WideString & str)
    {
        LOGD ("");
        thiz->commit_string (-1, "", str);
    }

    void
    slot_forward_key_event (IMEngineInstanceBase *si,
                            const KeyEvent & key)
    {
        LOGD ("");
        thiz->forward_key_event (-1, "", key);
    }

    void
    slot_update_lookup_table (IMEngineInstanceBase *si,
                              const LookupTable & table)
    {
        LOGD ("");
        thiz->update_candidate_string (table);
    }

    void
    slot_register_properties (IMEngineInstanceBase *si,
                              const PropertyList & properties)
    {
        LOGD ("");
        thiz->register_properties (properties);
    }

    void
    slot_update_property (IMEngineInstanceBase *si,
                          const Property & property)
    {
        LOGD ("");
        thiz->update_property (property);
    }

    void
    slot_beep (IMEngineInstanceBase *si)
    {
        //FIXME
        LOGD ("");
    }

    void
    slot_start_helper (IMEngineInstanceBase *si,
                       const String &helper_uuid)
    {
        LOGW ("deprecated function");
    }

    void
    slot_stop_helper (IMEngineInstanceBase *si,
                      const String &helper_uuid)
    {
        LOGW ("deprecated function");
    }

    void
    slot_send_helper_event (IMEngineInstanceBase *si,
                            const String      &helper_uuid,
                            const Transaction &trans)
    {
        LOGD ("");
        signal_process_imengine_event (thiz, focused_ic, helper_uuid, trans);
    }

    bool
    slot_get_surrounding_text (IMEngineInstanceBase *si,
                               WideString            &text,
                               int                   &cursor,
                               int                    maxlen_before,
                               int                    maxlen_after)
    {
        LOGD ("");
        String _text;
        thiz->get_surrounding_text (maxlen_before, maxlen_after, _text, cursor);
        text = utf8_mbstowcs(_text);
        return true;
    }

    bool
    slot_delete_surrounding_text (IMEngineInstanceBase *si,
                                  int                   offset,
                                  int                   len)
    {
        LOGD ("");

        thiz->delete_surrounding_text (offset, len);
        return true;
    }

    bool
    slot_get_selection (IMEngineInstanceBase *si,
                        WideString            &text)
    {
        LOGD ("");
        String _text;
        thiz->get_selection_text (_text);
        text = utf8_mbstowcs (_text);
        return true;
    }

    bool
    slot_set_selection (IMEngineInstanceBase *si,
                        int              start,
                        int              end)
    {
        LOGD ("");
        thiz->set_selection (start, end);
        return true;
    }

    void
    slot_expand_candidate (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->expand_candidate ();
    }

    void
    slot_contract_candidate (IMEngineInstanceBase *si)
    {
        LOGD ("");
        thiz->contract_candidate ();
    }

    void
    slot_set_candidate_style (IMEngineInstanceBase *si, ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line, ISF_CANDIDATE_MODE_T mode)
    {
        LOGD ("");
        thiz->set_candidate_style (portrait_line, mode);
    }

    void
    slot_send_private_command (IMEngineInstanceBase *si,
                               const String &command)
    {
        LOGD ("");
        thiz->send_private_command (command);
    }

    void
    attach_instance ()
    {
        si->signal_connect_show_preedit_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_show_preedit_string));
        si->signal_connect_show_aux_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_show_aux_string));
        si->signal_connect_show_lookup_table (
            slot (this, &HelperAgent::HelperAgentImpl::slot_show_lookup_table));

        si->signal_connect_hide_preedit_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_hide_preedit_string));
        si->signal_connect_hide_aux_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_hide_aux_string));
        si->signal_connect_hide_lookup_table (
            slot (this, &HelperAgent::HelperAgentImpl::slot_hide_lookup_table));

        si->signal_connect_update_preedit_caret (
            slot (this, &HelperAgent::HelperAgentImpl::slot_update_preedit_caret));
        si->signal_connect_update_preedit_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_update_preedit_string));
        si->signal_connect_update_aux_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_update_aux_string));
        si->signal_connect_update_lookup_table (
            slot (this, &HelperAgent::HelperAgentImpl::slot_update_lookup_table));

        si->signal_connect_commit_string (
            slot (this, &HelperAgent::HelperAgentImpl::slot_commit_string));

        si->signal_connect_forward_key_event (
            slot (this, &HelperAgent::HelperAgentImpl::slot_forward_key_event));

        si->signal_connect_register_properties (
            slot (this, &HelperAgent::HelperAgentImpl::slot_register_properties));

        si->signal_connect_update_property (
            slot (this, &HelperAgent::HelperAgentImpl::slot_update_property));

        si->signal_connect_beep (
            slot (this, &HelperAgent::HelperAgentImpl::slot_beep));

        si->signal_connect_start_helper (
            slot (this, &HelperAgent::HelperAgentImpl::slot_start_helper));

        si->signal_connect_stop_helper (
            slot (this, &HelperAgent::HelperAgentImpl::slot_stop_helper));

        si->signal_connect_send_helper_event (
            slot (this, &HelperAgent::HelperAgentImpl::slot_send_helper_event));

        si->signal_connect_get_surrounding_text (
            slot (this, &HelperAgent::HelperAgentImpl::slot_get_surrounding_text));

        si->signal_connect_delete_surrounding_text (
            slot (this, &HelperAgent::HelperAgentImpl::slot_delete_surrounding_text));

        si->signal_connect_get_selection (
            slot (this, &HelperAgent::HelperAgentImpl::slot_get_selection));

        si->signal_connect_set_selection (
            slot (this, &HelperAgent::HelperAgentImpl::slot_set_selection));

        si->signal_connect_expand_candidate (
            slot (this, &HelperAgent::HelperAgentImpl::slot_expand_candidate));
        si->signal_connect_contract_candidate (
            slot (this, &HelperAgent::HelperAgentImpl::slot_contract_candidate));

        si->signal_connect_set_candidate_style (
            slot (this, &HelperAgent::HelperAgentImpl::slot_set_candidate_style));

        si->signal_connect_send_private_command (
            slot (this, &HelperAgent::HelperAgentImpl::slot_send_private_command));
    }
private:
    HelperAgentImpl () : magic (0), magic_active (0), timeout (-1), focused_ic ((uint32) -1) { }
};

HelperAgent::HelperAgent ()
    : m_impl (new HelperAgentImpl (this))
{
}

HelperAgent::~HelperAgent ()
{
    delete m_impl;
}

/**
 * @brief Open socket connection to the Panel.
 *
 * @param info The information of this Helper object.
 * @param display The display which this Helper object should run on.
 *
 * @return The connection socket id. -1 means failed to create
 *         the connection.
 */
int
HelperAgent::open_connection (const HelperInfo &info,
                              const String     &display)
{
    if (m_impl->socket.is_connected ())
        close_connection ();

    SocketAddress address (scim_get_default_panel_socket_address (display));
    int timeout = m_impl->timeout = scim_get_default_socket_timeout ();
    uint32 magic;

    if (!address.valid ())
        return -1;

    int i = 0;
    std::cerr << " Connecting to PanelAgent server.";
    ISF_LOG (" Connecting to PanelAgent server.\n");
    while (!m_impl->socket.connect (address)) {
        std::cerr << ".";
        scim_usleep (100000);
        if (++i == 200) {
            std::cerr << "m_impl->socket.connect () is failed!!!\n";
            ISF_LOG ("m_impl->socket.connect () is failed!!!\n");
            return -1;
        }
    }
    std::cerr << " Connected :" << i << "\n";
    ISF_LOG ("  Connected :%d\n", i);
    LOGD ("Connection to PanelAgent succeeded, %d\n", i);

    /* Let's retry 10 times when failed */
    int open_connection_retries = 0;
    while (!scim_socket_open_connection (magic,
                                      String ("Helper"),
                                      String ("Panel"),
                                      m_impl->socket,
                                      timeout)) {
        if (++open_connection_retries > 10) {
            m_impl->socket.close ();
            std::cerr << "scim_socket_open_connection () is failed!!!\n";
            ISF_LOG ("scim_socket_open_connection () is failed!!!\n");
            ISF_SAVE_LOG ("scim_socket_open_connection failed, %d\n", timeout);

            return -1;
        }

        /* Retry after re-connecting the socket */
        if (m_impl->socket.is_connected ())
            close_connection ();

        /* This time, just retry atmost 2 seconds */
        i = 0;
        while (!m_impl->socket.connect (address) && ++i < 10) {
            scim_usleep (200000);
        }

    }

    ISF_LOG ("scim_socket_open_connection () is successful.\n");
    LOGD ("scim_socket_open_connection successful\n");

    bool match = false;
    std::vector<ImeInfoDB> ime_info_db;
    isf_db_select_all_ime_info (ime_info_db);
    for (i = 0; i < (int)ime_info_db.size (); i++) {
        if (ime_info_db[i].appid.compare (info.uuid) == 0) {
            match = true;
            break;
        }
    }

    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (magic);
    m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_REGISTER_HELPER);
    m_impl->send.put_data (info.uuid);
    m_impl->send.put_data (match? ime_info_db[i].label : info.name);
    m_impl->send.put_data (match? ime_info_db[i].iconpath : info.icon);
    m_impl->send.put_data (info.description);
    m_impl->send.put_data (match? ime_info_db[i].options : info.option);

    if (!m_impl->send.write_to_socket (m_impl->socket, magic)) {
        m_impl->socket.close ();
        return -1;
    }

    int cmd;
    if (m_impl->recv.read_from_socket (m_impl->socket, timeout) &&
        m_impl->recv.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
        m_impl->recv.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        m_impl->magic = magic;

        while (m_impl->recv.get_command (cmd)) {
            switch (cmd) {
                case SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT:
                {
                    uint32 ic;
                    String ic_uuid;
                    while (m_impl->recv.get_data (ic) && m_impl->recv.get_data (ic_uuid))
                        m_impl->signal_attach_input_context (this, ic, ic_uuid);
                    break;
                }
                case SCIM_TRANS_CMD_UPDATE_SCREEN:
                {
                    uint32 screen;
                    if (m_impl->recv.get_data (screen))
                        m_impl->signal_update_screen (this, (uint32) -1, String (""), (int) screen);
                    break;
                }
                default:
                    break;
            }
        }
    } else {
        //FIXME: Attaching input context is needed for desktop environment
        LOGW ("Attach input context and update screen failed");
    }

    ISF_SAVE_LOG ("Trying connect() with Helper_Active\n");

    /* connect to the panel agent as the active helper client */
    if (!m_impl->socket_active.connect (address)) return -1;
    if (!scim_socket_open_connection (magic,
                                      String ("Helper_Active"),
                                      String ("Panel"),
                                      m_impl->socket_active,
                                      timeout)) {
        m_impl->socket_active.close ();
        ISF_SAVE_LOG ("Helper_Active scim_socket_open_connection() failed %d\n", timeout);
        return -1;
    }

    m_impl->magic_active = magic;

    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (magic);
    m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER);
    m_impl->send.put_data (info.uuid);
    m_impl->send.put_data (match? ime_info_db[i].label : info.name);
    m_impl->send.put_data (match? ime_info_db[i].iconpath : info.icon);
    m_impl->send.put_data (info.description);
    m_impl->send.put_data (match? ime_info_db[i].options : info.option);

    if (!m_impl->send.write_to_socket (m_impl->socket_active, magic)) {
        ISF_SAVE_LOG ("Helper_Active write_to_socket() failed\n");
        m_impl->socket_active.close ();
        return -1;
    }
    m_impl->m_config = ConfigBase::get (false, "socket");

    return m_impl->socket.get_id ();
}

/**
 * @brief Close the socket connection to Panel.
 */
void
HelperAgent::close_connection ()
{
    m_impl->socket.close ();
    m_impl->socket_active.close ();
    m_impl->send.clear ();
    m_impl->recv.clear ();
    m_impl->magic        = 0;
    m_impl->magic_active = 0;
    m_impl->timeout      = 0;
}

/**
 * @brief Get the connection id previously returned by open_connection().
 *
 * @return the connection id
 */
int
HelperAgent::get_connection_number () const
{
    if (m_impl->socket.is_connected ())
        return m_impl->socket.get_id ();
    return -1;
}

/**
 * @brief Check whether this HelperAgent has been connected to a Panel.
 *
 * Return true when it is connected to panel, otherwise return false.
 */
bool
HelperAgent::is_connected () const
{
    return m_impl->socket.is_connected ();
}

/**
 * @brief Check if there are any events available to be processed.
 *
 * If it returns true then Helper object should call
 * HelperAgent::filter_event() to process them.
 *
 * @return true if there are any events available.
 */
bool
HelperAgent::has_pending_event () const
{
    if (m_impl->socket.is_connected () && m_impl->socket.wait_for_data (0) > 0)
        return true;

    return false;
}

/**
 * @brief Process the pending events.
 *
 * This function will emit the corresponding signals according
 * to the events.
 *
 * @return false if the connection is broken, otherwise return true.
 */
bool
HelperAgent::filter_event ()
{
    if (!m_impl->socket.is_connected () || !m_impl->recv.read_from_socket (m_impl->socket, m_impl->timeout))
        return false;

    int cmd;

    uint32 ic = (uint32) -1;
    String ic_uuid;

    if (!m_impl->recv.get_command (cmd) || cmd != SCIM_TRANS_CMD_REPLY) {
        LOGW ("wrong format of transaction\n");
        return true;
    }

    /* If there are ic and ic_uuid then read them. */
    if (!(m_impl->recv.get_data_type () == SCIM_TRANS_DATA_COMMAND) &&
        !(m_impl->recv.get_data (ic) && m_impl->recv.get_data (ic_uuid))) {
        LOGW ("wrong format of transaction\n");
        return true;
    }

    while (m_impl->recv.get_command (cmd)) {
        LOGD ("HelperAgent::cmd = %d\n", cmd);
        switch (cmd) {
            case SCIM_TRANS_CMD_EXIT:
                ISF_SAVE_LOG ("Helper ISE received SCIM_TRANS_CMD_EXIT message\n");
                m_impl->signal_exit (this, ic, ic_uuid);
                break;
            case SCIM_TRANS_CMD_RELOAD_CONFIG:
                m_impl->signal_reload_config (this, ic, ic_uuid);
                if (!m_impl->m_config.null())
                    m_impl->m_config->ConfigBase::reload();
                break;
            case SCIM_TRANS_CMD_UPDATE_SCREEN:
            {
                uint32 screen;
                if (m_impl->recv.get_data (screen))
                    m_impl->signal_update_screen (this, ic, ic_uuid, (int) screen);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION:
            {
                uint32 x, y;
                if (m_impl->recv.get_data (x) && m_impl->recv.get_data (y))
                    m_impl->signal_update_spot_location (this, ic, ic_uuid, (int) x, (int) y);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CURSOR_POSITION:
            {
                uint32 cursor_pos;
                if (m_impl->recv.get_data (cursor_pos)) {
                    m_impl->cursor_pos = cursor_pos;
                    LOGD ("update cursor position %d", cursor_pos);
                    m_impl->signal_update_cursor_position (this, ic, ic_uuid, (int) cursor_pos);
                        if (!m_impl->si.null ()) m_impl->si->update_cursor_position(cursor_pos);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT:
            {
                String text;
                uint32 cursor;
                if (m_impl->recv.get_data (text) && m_impl->recv.get_data (cursor)) {
                    if (m_impl->surrounding_text != NULL)
                        free (m_impl->surrounding_text);
                    m_impl->surrounding_text = strdup (text.c_str ());
                    m_impl->cursor_pos = cursor;
                    LOGD ("surrounding text: %s, %d", m_impl->surrounding_text, cursor);
                    if (m_impl->need_update_surrounding_text) {
                        m_impl->need_update_surrounding_text = false;
                        m_impl->signal_update_surrounding_text (this, ic, text, (int) cursor);
                    }
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_SELECTION:
            {
                String text;
                if (m_impl->recv.get_data (text)) {
                    if (m_impl->selection_text != NULL)
                        free (m_impl->selection_text);

                    m_impl->selection_text = strdup (text.c_str ());
                    LOGD ("selection text: %s", m_impl->selection_text);

                    if (m_impl->need_update_selection_text) {
                        m_impl->need_update_selection_text = false;
                        m_impl->signal_update_selection (this, ic, text);
                    }
                } else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_TRIGGER_PROPERTY:
            {
                String property;
                if (m_impl->recv.get_data (property)) {
                    m_impl->signal_trigger_property (this, ic, ic_uuid, property);
                    if (!m_impl->si.null ()) m_impl->si->trigger_property(property);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT:
            {
                Transaction trans;
                if (m_impl->recv.get_data (trans))
                    m_impl->signal_process_imengine_event (this, ic, ic_uuid, trans);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT:
                m_impl->signal_attach_input_context (this, ic, ic_uuid);
                break;
            case SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT:
                m_impl->signal_detach_input_context (this, ic, ic_uuid);
                break;
            case SCIM_TRANS_CMD_FOCUS_OUT:
            {
                m_impl->signal_focus_out (this, ic, ic_uuid);
                m_impl->focused_ic = (uint32) -1;
                if (!m_impl->si.null ()) m_impl->si->focus_out();
                break;
            }
            case SCIM_TRANS_CMD_FOCUS_IN:
            {
                m_impl->signal_focus_in (this, ic, ic_uuid);
                m_impl->focused_ic = ic;
                if (!m_impl->si.null ()) m_impl->si->focus_in();
                break;
            }
            case ISM_TRANS_CMD_SHOW_ISE_PANEL:
            {
                LOGD ("Helper ISE received ISM_TRANS_CMD_SHOW_ISE_PANEL message\n");

                char   *data = NULL;
                size_t  len;
                if (m_impl->recv.get_data (&data, len))
                    m_impl->signal_ise_show (this, ic, data, len);
                else
                    LOGW ("wrong format of transaction\n");
                if (data)
                    delete [] data;
                break;
            }
            case ISM_TRANS_CMD_HIDE_ISE_PANEL:
            {
                LOGD ("Helper ISE received ISM_TRANS_CMD_HIDE_ISE_PANEL message\n");
                m_impl->signal_ise_hide (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY:
            {
                struct rectinfo info = {0, 0, 0, 0};
                m_impl->signal_get_geometry (this, info);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (info.pos_x);
                m_impl->send.put_data (info.pos_y);
                m_impl->send.put_data (info.width);
                m_impl->send.put_data (info.height);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_MODE:
            {
                uint32 mode;
                if (m_impl->recv.get_data (mode))
                    m_impl->signal_set_mode (this, mode);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_LANGUAGE:
            {
                uint32 language;
                if (m_impl->recv.get_data (language))
                    m_impl->signal_set_language (this, language);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_IMDATA:
            {
                char   *imdata = NULL;
                size_t  len;
                if (m_impl->recv.get_data (&imdata, len)) {
                    m_impl->signal_set_imdata (this, imdata, len);
                    if (!m_impl->si.null ()) m_impl->si->set_imdata(imdata, len);
                }
                else
                    LOGW ("wrong format of transaction\n");

                if (NULL != imdata)
                    delete[] imdata;
                break;
            }
            case ISM_TRANS_CMD_GET_ISE_IMDATA:
            {
                char   *buf = NULL;
                size_t  len = 0;

                m_impl->signal_get_imdata (this, &buf, len);
                LOGD ("send ise imdata len = %d", len);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (buf, len);
                m_impl->send.write_to_socket (m_impl->socket);
                if (NULL != buf)
                    delete[] buf;
                break;
            }
            case ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE:
            {
                char *buf = NULL;
                m_impl->signal_get_language_locale (this, ic, &buf);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                if (buf != NULL)
                    m_impl->send.put_data (buf, strlen (buf));
                m_impl->send.write_to_socket (m_impl->socket);
                if (NULL != buf)
                    delete[] buf;
                break;
            }
            case ISM_TRANS_CMD_SET_RETURN_KEY_TYPE:
            {
                uint32 type = 0;
                if (m_impl->recv.get_data (type)) {
                    m_impl->signal_set_return_key_type (this, type);
                }
                break;
            }
            case ISM_TRANS_CMD_GET_RETURN_KEY_TYPE:
            {
                uint32 type = 0;
                m_impl->signal_get_return_key_type (this, type);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (type);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE:
            {
                uint32 disabled = 0;
                if (m_impl->recv.get_data (disabled)) {
                    m_impl->signal_set_return_key_disable (this, disabled);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE:
            {
                uint32 disabled = 0;
                m_impl->signal_get_return_key_type (this, disabled);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (disabled);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case SCIM_TRANS_CMD_PROCESS_KEY_EVENT:
            {
                KeyEvent key;
                uint32 ret = 0;
                if (m_impl->recv.get_data (key))
                    m_impl->signal_process_key_event(this, key, ret);
                else
                    LOGW ("wrong format of transaction\n");
                if (ret == 0)
                    if (!m_impl->si.null ())
                    {
                        ret = m_impl->si->process_key_event (key);
                        LOGD("imengine(%s) process key %d return %d", m_impl->si->get_factory_uuid().c_str(), key.code, ret);
                    }
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (ret);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case ISM_TRANS_CMD_SET_LAYOUT:
            {
                uint32 layout;

                if (m_impl->recv.get_data (layout)) {
                    m_impl->layout = layout;
                    m_impl->signal_set_layout (this, layout);
                    if (!m_impl->si.null ()) m_impl->si->set_layout(layout);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_GET_LAYOUT:
            {
                uint32 layout = 0;

                m_impl->signal_get_layout (this, layout);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (layout);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case ISM_TRANS_CMD_SET_INPUT_MODE:
            {
                uint32 input_mode;

                if (m_impl->recv.get_data (input_mode))
                    m_impl->signal_set_input_mode (this, input_mode);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SET_CAPS_MODE:
            {
                uint32 mode;

                if (m_impl->recv.get_data (mode))
                    m_impl->signal_set_caps_mode (this, mode);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT:
            {
                m_impl->signal_reset_input_context (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->reset();
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CANDIDATE_UI:
            {
                uint32 style, mode;
                if (m_impl->recv.get_data (style) && m_impl->recv.get_data (mode))
                    m_impl->signal_update_candidate_ui (this, ic, ic_uuid, style, mode);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY:
            {
                struct rectinfo info = {0, 0, 0, 0};
                if (m_impl->recv.get_data (info.pos_x)
                    && m_impl->recv.get_data (info.pos_y)
                    && m_impl->recv.get_data (info.width)
                    && m_impl->recv.get_data (info.height))
                    m_impl->signal_update_candidate_geometry (this, ic, ic_uuid, info);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE:
            {
                String name, uuid;
                if (m_impl->recv.get_data (name) && m_impl->recv.get_data (uuid))
                    m_impl->signal_update_keyboard_ise (this, ic, ic_uuid, name, uuid);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST:
            {
                uint32 num;
                String ise;
                std::vector<String> list;
                if (m_impl->recv.get_data (num)) {
                    for (unsigned int i = 0; i < num; i++) {
                        if (m_impl->recv.get_data (ise)) {
                            list.push_back (ise);
                        } else {
                            list.clear ();
                            break;
                        }
                    }
                    m_impl->signal_update_keyboard_ise_list (this, ic, ic_uuid, list);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW:
            {
                m_impl->signal_candidate_more_window_show (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->candidate_more_window_show();
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE:
            {
                m_impl->signal_candidate_more_window_hide (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->candidate_more_window_hide();
                break;
            }
            case ISM_TRANS_CMD_SELECT_AUX:
            {
                uint32 item;
                if (m_impl->recv.get_data (item)) {
                    m_impl->signal_select_aux (this, ic, ic_uuid, item);
                    if (!m_impl->si.null ()) m_impl->si->select_aux(item);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case SCIM_TRANS_CMD_SELECT_CANDIDATE: //FIXME:remove if useless
            {
                uint32 item;
                if (m_impl->recv.get_data (item))
                    m_impl->signal_select_candidate (this, ic, ic_uuid, item);
                else
                    LOGW ("wrong format of transaction\n");
                if (!m_impl->si.null ()) m_impl->si->select_candidate(item);
                break;
            }
            case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP: //FIXME:remove if useless
            {
                m_impl->signal_candidate_table_page_up (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->lookup_table_page_up();
                break;
            }
            case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN: //FIXME:remove if useless
            {
                m_impl->signal_candidate_table_page_down (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->lookup_table_page_down();
                break;
            }
            case SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE:
            {
                uint32 size;
                if (m_impl->recv.get_data (size)) {
                    m_impl->signal_update_candidate_table_page_size (this, ic, ic_uuid, size);
                    if (!m_impl->si.null ()) m_impl->si->update_lookup_table_page_size(size);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_SHOW: //FIXME:remove if useless
            {
                m_impl->signal_candidate_show (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_HIDE: //FIXME:remove if useless
            {
                m_impl->signal_candidate_hide (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE: //FIXME:remove if useless
            {
                CommonLookupTable helper_candidate_table;
                if (m_impl->recv.get_data (helper_candidate_table))
                    m_impl->signal_update_lookup_table (this, helper_candidate_table);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT:
            {
                std::vector<uint32> row_items;
                if (m_impl->recv.get_data (row_items)) {
                    m_impl->signal_update_candidate_item_layout (this, row_items);
                    if (!m_impl->si.null ()) m_impl->si->update_candidate_item_layout(row_items);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SELECT_ASSOCIATE:
            {
                uint32 item;
                if (m_impl->recv.get_data (item))
                    m_impl->signal_select_associate (this, ic, ic_uuid, item);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP:
            {
                m_impl->signal_associate_table_page_up (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN:
            {
                m_impl->signal_associate_table_page_down (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE:
            {
                uint32 size;
                if (m_impl->recv.get_data (size))
                    m_impl->signal_update_associate_table_page_size (this, ic, ic_uuid, size);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_RESET_ISE_CONTEXT:
            {
                m_impl->signal_reset_ise_context (this, ic, ic_uuid);
                if (!m_impl->si.null ()) m_impl->si->reset();
                break;
            }
            case ISM_TRANS_CMD_TURN_ON_LOG:
            {
                uint32 isOn;
                if (m_impl->recv.get_data (isOn))
                    m_impl->signal_turn_on_log (this, isOn);
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE:
            {
                uint32 size;
                if (m_impl->recv.get_data (size)) {
                    m_impl->signal_update_displayed_candidate_number (this, ic, ic_uuid, size);
                    if (!m_impl->si.null ()) m_impl->si->update_displayed_candidate_number(size);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_LONGPRESS_CANDIDATE:
            {
                uint32 index;
                if (m_impl->recv.get_data (index)) {
                    m_impl->signal_longpress_candidate (this, ic, ic_uuid, index);
                    if (!m_impl->si.null ()) m_impl->si->longpress_candidate(index);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SET_INPUT_HINT:
            {
                uint32 input_hint;

                if (m_impl->recv.get_data (input_hint)) {
                    m_impl->signal_set_input_hint (this, input_hint);
                    if (!m_impl->si.null ()) m_impl->si->set_input_hint(input_hint);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION:
            {
                uint32 bidi_direction;

                if (m_impl->recv.get_data (bidi_direction)) {
                    m_impl->signal_update_bidi_direction (this, bidi_direction);
                    if (!m_impl->si.null ()) m_impl->si->update_bidi_direction(bidi_direction);
                }
                else
                    LOGW ("wrong format of transaction\n");
                break;
            }
            case ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW:
            {
                m_impl->signal_show_option_window (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_CHECK_OPTION_WINDOW:
            {
                uint32 avail = 0;
                m_impl->signal_check_option_window (this, avail);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (avail);
                m_impl->send.write_to_socket (m_impl->socket);
                break;
            }
            case ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT:
            {
                uint32 ret = 0;
                uint32 type;
                char *data = NULL;
                size_t len;
                if (m_impl->recv.get_data(type) &&
                    m_impl->recv.get_data(&data, len)) {
                    m_impl->signal_process_input_device_event(this, type, data, len, ret);
                }
                else
                    LOGW("wrong format of transaction\n");
                m_impl->send.clear();
                m_impl->send.put_command(SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data(ret);
                m_impl->send.write_to_socket(m_impl->socket);
                break;
            }
            default:
                break;
        }
    }
    return true;
}

/**
 * @brief Request SCIM to reload all configuration.
 *
 * This function should only by used by Setup Helper to request
 * scim's reloading the configuration.
 */
void
HelperAgent::reload_config () const
{
    LOGD ("");
#if 0 //reload config message only send by socketconfig client
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_RELOAD_CONFIG);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
    if (!m_impl->m_config.null())
        m_impl->m_config->reload();
}

/**
 * @brief Register some properties into Panel.
 *
 * This function send the request to Panel to register a list
 * of Properties.
 *
 * @param properties The list of Properties to be registered into Panel.
 *
 * @sa scim::Property.
 */
void
HelperAgent::register_properties (const PropertyList &properties) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_REGISTER_PROPERTIES);
        m_impl->send.put_data (properties);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Update a registered property.
 *
 * @param property The property to be updated.
 */
void
HelperAgent::update_property (const Property &property) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_PROPERTY);
        m_impl->send.put_data (property);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Send a set of events to an IMEngineInstance.
 *
 * All events should be put into a Transaction.
 * And the events can only be received by one IMEngineInstance object.
 *
 * @param ic The handle of the Input Context to receive the events.
 * @param ic_uuid The UUID of the Input Context.
 * @param trans The Transaction object holds the events.
 */
void
HelperAgent::send_imengine_event (int                ic,
                                  const String      &ic_uuid,
                                  const Transaction &trans) const
{
    LOGD ("");
//remove if not necessary
#if 0
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (trans);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
    if (!m_impl->si.null ()) m_impl->si->process_helper_event (ic_uuid, trans);
}

/**
 * @brief Send a KeyEvent to an IMEngineInstance.
 *
 * @param ic The handle of the IMEngineInstance to receive the event.
 *        -1 means the currently focused IMEngineInstance.
 * @param ic_uuid The UUID of the IMEngineInstance. Empty means don't match.
 * @param key The KeyEvent to be sent.
 */
void
HelperAgent::send_key_event (int            ic,
                             const String   &ic_uuid,
                             const KeyEvent &key) const
{
    LOGD ("");

    //FIXME: remove shift_mode_off, shift_mode_on, shift_mode_lock from ISE side
    if (key.code == SHIFT_MODE_OFF ||
        key.code == SHIFT_MODE_ON ||
        key.code == SHIFT_MODE_LOCK ||
        key.code == SHIFT_MODE_ENABLE ||
        key.code == SHIFT_MODE_DISABLE) {
        LOGW("FIXME ignore shift codes");
        return;
    }

    bool ret = false;
    if (!m_impl->si.null ()) {
        ret = m_impl->si->process_key_event (key);
        LOGD ("imengine(%s) process key %d return %d", m_impl->si->get_factory_uuid().c_str(), key.code, ret);
    }
    if (ret == false && m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT);
        if (ic == -1) {
            m_impl->send.put_data (m_impl->focused_ic);
        } else {
            m_impl->send.put_data ((uint32)ic);
        }
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (key);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Forward a KeyEvent to client application directly.
 *
 * @param ic The handle of the client Input Context to receive the event.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 * @param key The KeyEvent to be forwarded.
 */
void
HelperAgent::forward_key_event (int            ic,
                                const String   &ic_uuid,
                                const KeyEvent &key) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_FORWARD_KEY_EVENT);
        if (ic == -1) {
            m_impl->send.put_data (m_impl->focused_ic);
        } else {
            m_impl->send.put_data ((uint32)ic);
        }
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (key);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Commit a WideString to client application directly.
 *
 * @param ic The handle of the client Input Context to receive the WideString.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 * @param wstr The WideString to be committed.
 */
void
HelperAgent::commit_string (int               ic,
                            const String     &ic_uuid,
                            const WideString &wstr) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
        if (ic == -1) {
            m_impl->send.put_data (m_impl->focused_ic);
        } else {
            m_impl->send.put_data ((uint32)ic);
        }
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (wstr);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

void
HelperAgent::commit_string (int               ic,
                            const String     &ic_uuid,
                            const  char      *buf,
                            int               buflen) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
        if (ic == -1) {
            m_impl->send.put_data (m_impl->focused_ic);
        } else {
            m_impl->send.put_data ((uint32)ic);
        }
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_dataw (buf, buflen);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to show preedit string.
 *
 * @param ic The handle of the client Input Context to receive the request.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 */
void
HelperAgent::show_preedit_string (int               ic,
                                  const String     &ic_uuid) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to show aux string.
 */
void
HelperAgent::show_aux_string (void) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_SHOW_AUX_STRING);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to show candidate string.
 */
void
HelperAgent::show_candidate_string (void) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to show associate string.
 */
void
HelperAgent::show_associate_string (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to hide preedit string.
 *
 * @param ic The handle of the client Input Context to receive the request.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 */
void
HelperAgent::hide_preedit_string (int               ic,
                                  const String     &ic_uuid) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to hide aux string.
 */
void
HelperAgent::hide_aux_string (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_HIDE_AUX_STRING);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to hide candidate string.
 */
void
HelperAgent::hide_candidate_string (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to hide associate string.
 */
void
HelperAgent::hide_associate_string (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Update a new WideString for preedit.
 *
 * @param ic The handle of the client Input Context to receive the WideString.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 * @param str The WideString to be updated.
 * @param attrs The attribute list for preedit string.
 */
void
HelperAgent::update_preedit_string (int                  ic,
                                    const String        &ic_uuid,
                                    const WideString    &str,
                                    const AttributeList &attrs) const
{
    LOGD ("");
    update_preedit_string (ic, ic_uuid, str, attrs, -1);
}

void
HelperAgent::update_preedit_string (int                  ic,
                                    const String        &ic_uuid,
                                    const char         *buf,
                                    int                 buflen,
                                    const AttributeList &attrs) const
{
    LOGD ("");
    update_preedit_string (ic, ic_uuid, buf, buflen, attrs, -1);
}

/**
 * @brief Update a new WideString for preedit.
 *
 * @param ic The handle of the client Input Context to receive the WideString.
 *        -1 means the currently focused Input Context.
 * @param ic_uuid The UUID of the IMEngine used by the Input Context.
 *        Empty means don't match.
 * @param str The WideString to be updated.
 * @param attrs The attribute list for preedit string.
 * @param caret The caret position in preedit string.
 */
void
HelperAgent::update_preedit_string (int                  ic,
                                    const String        &ic_uuid,
                                    const WideString    &str,
                                    const AttributeList &attrs,
                                    int            caret) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (str);
        m_impl->send.put_data (attrs);
        m_impl->send.put_data (caret);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

void
HelperAgent::update_preedit_string (int                 ic,
                                    const String       &ic_uuid,
                                    const char         *buf,
                                    int                 buflen,
                                    const AttributeList &attrs,
                                    int            caret) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_dataw (buf, buflen);
        m_impl->send.put_data (attrs);
        m_impl->send.put_data (caret);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Update the preedit caret position in the preedit string.
 *
 * @param caret - the new position of the preedit caret.
 */
void
HelperAgent::update_preedit_caret (int caret) const
{
    LOGD ("");

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET);
        m_impl->send.put_data ((uint32)caret);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Update a new string for aux.
 *
 * @param str The string to be updated.
 * @param attrs The attribute list for aux string.
 */
void
HelperAgent::update_aux_string (const String        &str,
                                const AttributeList &attrs) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_AUX_STRING);
        m_impl->send.put_data (str);
        m_impl->send.put_data (attrs);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to update candidate.
 *
 * @param table The lookup table for candidate.
 */
void
HelperAgent::update_candidate_string (const LookupTable &table) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE);
        m_impl->send.put_data (table);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to update associate.
 *
 * @param table The lookup table for associate.
 */
void
HelperAgent::update_associate_string (const LookupTable &table) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE);
        m_impl->send.put_data (table);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief When the input context of ISE is changed,
 *         ISE can call this function to notify application
 *
 * @param type  type of event.
 * @param value value of event.
 */
void
HelperAgent::update_input_context (uint32 type, uint32 value) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT);
        m_impl->send.put_data (type);
        m_impl->send.put_data (value);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get surrounding text asynchronously.
 *
 * @param uuid The helper ISE UUID.
 * @param maxlen_before The max length of before.
 * @param maxlen_after The max length of after.
 */
void
HelperAgent::get_surrounding_text (const String &uuid, int maxlen_before, int maxlen_after) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_GET_SURROUNDING_TEXT);
        m_impl->send.put_data (uuid);
        m_impl->send.put_data (maxlen_before);
        m_impl->send.put_data (maxlen_after);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
    m_impl->need_update_surrounding_text = true;
}

/**
 * @brief Request to get surrounding text synchronously.
 *
 * @param uuid The helper ISE UUID.
 * @param maxlen_before The max length of before.
 * @param maxlen_after The max length of after.
 * @param text The surrounding text.
 * @param cursor The cursor position.
 */
void
HelperAgent::get_surrounding_text (int maxlen_before, int maxlen_after, String &text, int &cursor)
{
    LOGD ("");

    if (!m_impl->socket_active.is_connected ())
        return;

    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (m_impl->magic_active);
    m_impl->send.put_command (SCIM_TRANS_CMD_GET_SURROUNDING_TEXT);
    m_impl->send.put_data ("");
    m_impl->send.put_data (maxlen_before);
    m_impl->send.put_data (maxlen_after);
    m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);

#ifdef WAYLAND
    int filedes[2];
    if (pipe2(filedes,O_CLOEXEC|O_NONBLOCK) ==-1 ) {
        LOGD ("create pipe failed");
        return;
    }
    LOGD("%d,%d",filedes[0],filedes[1]);

    m_impl->socket_active.write_fd (filedes[1]);
    close (filedes[1]);

    for (int i = 0; i < 3; i++) {
        int fds[3];
        fds[0] = m_impl->socket_active.get_id();
        fds[1] = filedes[0];
        fds[2] = 0;
        if (!scim_wait_for_data (fds)) {
            LOGE ("");
            break;
        }

        if (fds[1]) {
            char buff[512];
            int len = read (fds[1], buff, sizeof(buff) - 1);
            if (len <= 0)
                break;
            else {
                buff[len] = '\0';
                text.append (buff);
            }
        }
        if (fds[0])
            filter_event ();
    }
    close (filedes[0]);
    cursor = m_impl->cursor_pos;
#else
    if (m_impl->surrounding_text) {
        free (m_impl->surrounding_text);
        m_impl->surrounding_text = NULL;
    }

    for (int i = 0; i < 3; i++) {
        if (filter_event () && m_impl->surrounding_text) {
            text = m_impl->surrounding_text;
            cursor = m_impl->cursor_pos;
            break;
        }
    }

    if (m_impl->surrounding_text) {
        free (m_impl->surrounding_text);
        m_impl->surrounding_text = NULL;
    }
#endif
}

/**
 * @brief Request to delete surrounding text.
 *
 * @param offset The offset for cursor position.
 * @param len The length for delete text.
 */
void
HelperAgent::delete_surrounding_text (int offset, int len) const
{
    LOGD ("offset = %d, len = %d", offset, len);

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT);
        m_impl->send.put_data (offset);
        m_impl->send.put_data (len);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get selection text asynchronously.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_selection (const String &uuid) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_GET_SELECTION);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }

    m_impl->need_update_selection_text = true;
}

/**
 * @brief Request to get selection text synchronously.
 *
 * @param text The selection text.
 */
void
HelperAgent::get_selection_text (String &text)
{
    LOGD ("");

    if (!m_impl->socket_active.is_connected ())
        return;

    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (m_impl->magic_active);
    m_impl->send.put_command (SCIM_TRANS_CMD_GET_SELECTION);
    m_impl->send.put_data ("");
    m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
#ifdef WAYLAND
    int filedes[2];
    if (pipe2 (filedes,O_CLOEXEC|O_NONBLOCK) == -1 ) {
        LOGD ("create pipe failed");
        return;
    }
    LOGD("%d,%d", filedes[0], filedes[1]);

    m_impl->socket_active.write_fd (filedes[1]);
    close (filedes[1]);

    for (int i = 0; i < 3; i++) {
        int fds[3];
        fds[0] = m_impl->socket_active.get_id();
        fds[1] = filedes[0];
        fds[2] = 0;
        if (!scim_wait_for_data (fds)) {
            LOGE ("");
            break;
        }

        if (fds[1]) {
            char buff[512];
            int len = read (fds[1], buff, sizeof(buff) - 1);
            if (len <= 0)
                break;
            else {
                buff[len] = '\0';
                text.append (buff);
            }
        }
        if (fds[0])
            filter_event ();
    }
    close (filedes[0]);
#else
    if (m_impl->selection_text) {
        free (m_impl->selection_text);
        m_impl->selection_text = NULL;
    }

    for (int i = 0; i < 3; i++) {
        if (filter_event () && m_impl->selection_text) {
            text = m_impl->selection_text;
            break;
        }
    }
    if (m_impl->selection_text) {
        free (m_impl->selection_text);
        m_impl->selection_text = NULL;
    }
#endif
}

/**
 * @brief Request to select text.
 *
 * @param start The start position in text.
 * @param end The end position in text.
 */
void
HelperAgent::set_selection (int start, int end) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_SET_SELECTION);
        m_impl->send.put_data (start);
        m_impl->send.put_data (end);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Send a private command to an application.
 *
 * @param command The private command sent from IME.
 */
void
HelperAgent::send_private_command (const String &command) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND);
        m_impl->send.put_data (command);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get uuid list of all keyboard ISEs.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_keyboard_ise_list (const String &uuid) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Set candidate position in screen.
 *
 * @param left The x position in screen.
 * @param top The y position in screen.
 */
void
HelperAgent::set_candidate_position (int left, int top) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SET_CANDIDATE_POSITION);
        m_impl->send.put_data (left);
        m_impl->send.put_data (top);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Set candidate style.
 *
 * @param portrait_line - the displayed line number for portrait mode.
 * @param mode          - candidate window mode.
 */
void
HelperAgent::set_candidate_style (ISF_CANDIDATE_PORTRAIT_LINE_T portrait_line,
                                  ISF_CANDIDATE_MODE_T          mode) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SET_CANDIDATE_UI);
        m_impl->send.put_data (portrait_line);
        m_impl->send.put_data (mode);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to hide candidate window.
 */
void
HelperAgent::candidate_hide (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_HIDE_CANDIDATE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get candidate window size and position.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_candidate_window_geometry (const String &uuid) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Set current keyboard ISE.
 *
 * @param uuid The keyboard ISE UUID.
 */
void
HelperAgent::set_keyboard_ise_by_uuid (const String &uuid) const
{
    LOGD ("");
    ImeInfoDB imeInfo;
    IMEngineFactoryPointer factory;
    IMEngineModule *engine_module = NULL;
    static int instance_count = 1;

    if ((!m_impl->si.null ()) && m_impl->si->get_factory_uuid () == uuid) {
        LOGD ("Already in UUID: %s\n", uuid.c_str());
        return;
    }

    if (!m_impl->si.null()) {
        m_impl->si->focus_out();
        m_impl->si.reset();
    }

    if (m_impl->m_config.null ()) {
        LOGW ("config is not working");
        return;
    }

    if (isf_db_select_ime_info_by_appid(uuid.c_str (), &imeInfo) < 1) {
        LOGW ("ime_info row is not available for %s", uuid.c_str ());
        return;
    }

    engine_module = &m_impl->engine_module;

    if (engine_module->valid() && imeInfo.module_name != engine_module->get_module_name()) {
        LOGD ("imengine module %s unloaded", engine_module->get_module_name().c_str());
        engine_module->unload();
    }

    if (!engine_module->valid()) {

        if (engine_module->load (imeInfo.module_name, m_impl->m_config) == false) {
            LOGW ("load module %s failed", imeInfo.module_name.c_str());
            return;
        }
        LOGD ("imengine module %s loaded", imeInfo.module_name.c_str());
    }

    for (size_t j = 0; j < engine_module->number_of_factories (); ++j) {

        try {
            factory = engine_module->create_factory (j);
            if (factory.null () == false && factory->get_uuid () == uuid)
                break;
        } catch (...) {
            factory.reset ();
            return;
        }
    }

    if (factory.null()) {
        LOGW ("imengine uuid %s is not found", uuid.c_str());
        return;
    }

    m_impl->si = factory->create_instance ("UTF-8", instance_count++);
    if (m_impl->si.null ()) {
        LOGE ("create_instance %s failed",uuid.c_str ());
        return;
    }

    m_impl->attach_instance ();
    LOGD ("Require UUID: %s Current UUID: %s", uuid.c_str (), m_impl->si->get_factory_uuid ().c_str ());
    m_impl->si->set_layout (m_impl->layout);
    if(m_impl->focused_ic != (uint32)-1)
        m_impl->si->focus_in ();
}

/**
 * @brief Request to get current keyboard ISE information.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_keyboard_ise (const String &uuid) const
{
    LOGD ("");
    //FIXME: maybe useless
#if 0
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_KEYBOARD_ISE);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
}

/**
 * @brief Update ISE window geometry.
 *
 * @param x      The x position in screen.
 * @param y      The y position in screen.
 * @param width  The ISE window width.
 * @param height The ISE window height.
 */
void
HelperAgent::update_geometry (int x, int y, int width, int height) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY);
        m_impl->send.put_data (x);
        m_impl->send.put_data (y);
        m_impl->send.put_data (width);
        m_impl->send.put_data (height);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to expand candidate window.
 */
void
HelperAgent::expand_candidate (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_EXPAND_CANDIDATE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to contract candidate window.
 */
void
HelperAgent::contract_candidate (void) const
{
    LOGD ("");
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_CONTRACT_CANDIDATE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Send selected candidate string index number.
 */
void
HelperAgent::select_candidate (int index) const
{
    LOGD ("");
    if (!m_impl->si.null ())
        m_impl->si->select_candidate (index);
    //FIXME: maybe useless
#if 0

    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SELECT_CANDIDATE);
        m_impl->send.put_data (index);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
}

/**
 * @brief Update our ISE is exiting.
 */
void
HelperAgent::update_ise_exit (void) const
{
    LOGD ("");
    //FIXME: maybe useless
#if 0
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_UPDATE_ISE_EXIT);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
}

/**
 * @brief Request to reset keyboard ISE.
 */
void
HelperAgent::reset_keyboard_ise (void) const
{
    LOGD ("");
//FIXME: maybe useless
#if 0
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
#endif
    if (!m_impl->si.null ()) {
        m_impl->si->reset ();
    }
}

/**
 * @brief Connect a slot to Helper exit signal.
 *
 * This signal is used to let the Helper exit.
 *
 * The prototype of the slot is:
 *
 * void exit (const HelperAgent *agent, int ic, const String &ic_uuid);
 *
 * Parameters:
 * - agent    The pointer to the HelperAgent object which emits this signal.
 * - ic       An opaque handle of the currently focused input context.
 * - ic_uuid  The UUID of the IMEngineInstance associated with the focused input context.
 */
Connection
HelperAgent::signal_connect_exit (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_exit.connect (slot);
}

/**
 * @brief Connect a slot to Helper attach input context signal.
 *
 * This signal is used to attach an input context to this helper.
 *
 * When an input context requst to start this helper, then this
 * signal will be emitted as soon as the helper is started.
 *
 * When an input context want to start an already started helper,
 * this signal will also be emitted.
 *
 * Helper can send some events back to the IMEngineInstance in this
 * signal-slot, to inform that it has been started sccessfully.
 *
 * The prototype of the slot is:
 *
 * void attach_input_context (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_attach_input_context (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_attach_input_context.connect (slot);
}

/**
 * @brief Connect a slot to Helper detach input context signal.
 *
 * This signal is used to detach an input context from this helper.
 *
 * When an input context requst to stop this helper, then this
 * signal will be emitted.
 *
 * Helper shouldn't send any event back to the IMEngineInstance, because
 * the IMEngineInstance attached to the ic should have been destroyed.
 *
 * The prototype of the slot is:
 *
 * void detach_input_context (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_detach_input_context (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_detach_input_context.connect (slot);
}

/**
 * @brief Connect a slot to Helper reload config signal.
 *
 * This signal is used to let the Helper reload configuration.
 *
 * The prototype of the slot is:
 *
 * void reload_config (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_reload_config (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_reload_config.connect (slot);
}

/**
 * @brief Connect a slot to Helper update screen signal.
 *
 * This signal is used to let the Helper move its GUI to another screen.
 * It can only be emitted when SCIM_HELPER_NEED_SCREEN_INFO is set in HelperInfo.option.
 *
 * The prototype of the slot is:
 *
 * void update_screen (const HelperAgent *agent, int ic, const String &ic_uuid, int screen_number);
 */
Connection
HelperAgent::signal_connect_update_screen (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_screen.connect (slot);
}

/**
 * @brief Connect a slot to Helper update spot location signal.
 *
 * This signal is used to let the Helper move its GUI according to the current spot location.
 * It can only be emitted when SCIM_HELPER_NEED_SPOT_LOCATION_INFO is set in HelperInfo.option.
 *
 * The prototype of the slot is:
 * void update_spot_location (const HelperAgent *agent, int ic, const String &ic_uuid, int x, int y);
 */
Connection
HelperAgent::signal_connect_update_spot_location (HelperAgentSlotIntInt *slot)
{
    return m_impl->signal_update_spot_location.connect (slot);
}

/**
 * @brief Connect a slot to Helper update cursor position signal.
 *
 * This signal is used to let the Helper get the cursor position information.
 *
 * The prototype of the slot is:
 * void update_cursor_position (const HelperAgent *agent, int ic, const String &ic_uuid, int cursor_pos);
 */
Connection
HelperAgent::signal_connect_update_cursor_position (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_cursor_position.connect (slot);
}

/**
 * @brief Connect a slot to Helper update surrounding text signal.
 *
 * This signal is used to let the Helper get the surrounding text.
 *
 * The prototype of the slot is:
 * void update_surrounding_text (const HelperAgent *agent, int ic, const String &text, int cursor);
 */
Connection
HelperAgent::signal_connect_update_surrounding_text (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_surrounding_text.connect (slot);
}

/**
 * @brief Connect a slot to Helper update selection signal.
 *
 * This signal is used to let the Helper get the selection.
 *
 * The prototype of the slot is:
 * void update_selection (const HelperAgent *agent, int ic, const String &text);
 */
Connection
HelperAgent::signal_connect_update_selection (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_update_selection.connect (slot);
}

/**
 * @brief Connect a slot to Helper trigger property signal.
 *
 * This signal is used to trigger a property registered by this Helper.
 * A property will be triggered when user clicks on it.
 *
 * The prototype of the slot is:
 * void trigger_property (const HelperAgent *agent, int ic, const String &ic_uuid, const String &property);
 */
Connection
HelperAgent::signal_connect_trigger_property (HelperAgentSlotString *slot)
{
    return m_impl->signal_trigger_property.connect (slot);
}

/**
 * @brief Connect a slot to Helper process imengine event signal.
 *
 * This signal is used to deliver the events sent from IMEngine to Helper.
 *
 * The prototype of the slot is:
 * void process_imengine_event (const HelperAgent *agent, int ic, const String &ic_uuid, const Transaction &transaction);
 */
Connection
HelperAgent::signal_connect_process_imengine_event (HelperAgentSlotTransaction *slot)
{
    return m_impl->signal_process_imengine_event.connect (slot);
}

/**
 * @brief Connect a slot to Helper focus out signal.
 *
 * This signal is used to do something when input context is focus out.
 *
 * The prototype of the slot is:
 * void focus_out (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_focus_out (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_focus_out.connect (slot);
}

/**
 * @brief Connect a slot to Helper focus in signal.
 *
 * This signal is used to do something when input context is focus in.
 *
 * The prototype of the slot is:
 * void focus_in (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_focus_in (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_focus_in.connect (slot);
}

/**
 * @brief Connect a slot to Helper show signal.
 *
 * This signal is used to show Helper ISE window.
 *
 * The prototype of the slot is:
 * void ise_show (const HelperAgent *agent, int ic, char *buf, size_t &len);
 */
Connection
HelperAgent::signal_connect_ise_show (HelperAgentSlotIntRawVoid *slot)
{
    return m_impl->signal_ise_show.connect (slot);
}

/**
 * @brief Connect a slot to Helper hide signal.
 *
 * This signal is used to hide Helper ISE window.
 *
 * The prototype of the slot is:
 * void ise_hide (const HelperAgent *agent, int ic, const String &ic_uuid);
 */
Connection
HelperAgent::signal_connect_ise_hide (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_ise_hide.connect (slot);
}

/**
 * @brief Connect a slot to Helper get ISE window geometry signal.
 *
 * This signal is used to get Helper ISE window size and position.
 *
 * The prototype of the slot is:
 * void get_geometry (const HelperAgent *agent, struct rectinfo &info);
 */
Connection
HelperAgent::signal_connect_get_geometry (HelperAgentSlotSize *slot)
{
    return m_impl->signal_get_geometry.connect (slot);
}

/**
 * @brief Connect a slot to Helper set mode signal.
 *
 * This signal is used to set Helper ISE mode.
 *
 * The prototype of the slot is:
 * void set_mode (const HelperAgent *agent, uint32 &mode);
 */
Connection
HelperAgent::signal_connect_set_mode (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_mode.connect (slot);
}

/**
 * @brief Connect a slot to Helper set language signal.
 *
 * This signal is used to set Helper ISE language.
 *
 * The prototype of the slot is:
 * void set_language (const HelperAgent *agent, uint32 &language);
 */
Connection
HelperAgent::signal_connect_set_language (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_language.connect (slot);
}

/**
 * @brief Connect a slot to Helper set im data signal.
 *
 * This signal is used to send im data to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_imdata (const HelperAgent *agent, char *buf, size_t &len);
 */
Connection
HelperAgent::signal_connect_set_imdata (HelperAgentSlotRawVoid *slot)
{
    return m_impl->signal_set_imdata.connect (slot);
}

/**
 * @brief Connect a slot to Helper get im data signal.
 *
 * This signal is used to get im data from Helper ISE.
 *
 * The prototype of the slot is:
 * void get_imdata (const HelperAgent *, char **buf, size_t &len);
 */
Connection
HelperAgent::signal_connect_get_imdata (HelperAgentSlotGetRawVoid *slot)
{
    return m_impl->signal_get_imdata.connect (slot);
}

/**
 * @brief Connect a slot to Helper get language locale signal.
 *
 * This signal is used to get language locale from Helper ISE.
 *
 * The prototype of the slot is:
 * void get_language_locale (const HelperAgent *, int ic, char **locale);
 */
Connection
HelperAgent::signal_connect_get_language_locale (HelperAgentSlotIntGetStringVoid *slot)
{
    return m_impl->signal_get_language_locale.connect (slot);
}

/**
 * @brief Connect a slot to Helper set return key type signal.
 *
 * This signal is used to send return key type to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_return_key_type (const HelperAgent *agent, uint32 &type);
 */
Connection
HelperAgent::signal_connect_set_return_key_type (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_return_key_type.connect (slot);
}

/**
 * @brief Connect a slot to Helper get return key type signal.
 *
 * This signal is used to get return key type from Helper ISE.
 *
 * The prototype of the slot is:
 * void get_return_key_type (const HelperAgent *agent, uint32 &type);
 */
Connection
HelperAgent::signal_connect_get_return_key_type (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_get_return_key_type.connect (slot);
}

/**
 * @brief Connect a slot to Helper set return key disable signal.
 *
 * This signal is used to send return key disable to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_return_key_disable (const HelperAgent *agent, uint32 &disabled);
 */
Connection
HelperAgent::signal_connect_set_return_key_disable (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_return_key_disable.connect (slot);
}

/**
 * @brief Connect a slot to Helper process key event signal.
 *
 * This signal is used to send keyboard key event to Helper ISE.
 *
 * The prototype of the slot is:
 * void process_key_event (const HelperAgent *agent, KeyEvent &key, uint32 &ret);
 */
Connection
HelperAgent::signal_connect_process_key_event (HelperAgentSlotKeyEventUint *slot)
{
    return m_impl->signal_process_key_event.connect (slot);
}

/**
 * @brief Connect a slot to Helper get return key disable signal.
 *
 * This signal is used to get return key disable from Helper ISE.
 *
 * The prototype of the slot is:
 * void get_return_key_disable (const HelperAgent *agent, uint32 &disabled);
 */
Connection
HelperAgent::signal_connect_get_return_key_disable (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_get_return_key_disable.connect (slot);
}

/**
 * @brief Connect a slot to Helper set layout signal.
 *
 * This signal is used to set Helper ISE layout.
 *
 * The prototype of the slot is:
 * void set_layout (const HelperAgent *agent, uint32 &layout);
 */
Connection
HelperAgent::signal_connect_set_layout (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_layout.connect (slot);
}

/**
 * @brief Connect a slot to Helper get layout signal.
 *
 * This signal is used to get Helper ISE layout.
 *
 * The prototype of the slot is:
 * void get_layout (const HelperAgent *agent, uint32 &layout);
 */
Connection
HelperAgent::signal_connect_get_layout (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_get_layout.connect (slot);
}

/**
 * @brief Connect a slot to Helper set input mode signal.
 *
 * This signal is used to set Helper ISE input mode.
 *
 * The prototype of the slot is:
 * void set_input_mode (const HelperAgent *agent, uint32 &input_mode);
 */
Connection
HelperAgent::signal_connect_set_input_mode (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_input_mode.connect (slot);
}

/**
 * @brief Connect a slot to Helper set input hint signal.
 *
 * This signal is used to set Helper ISE input hint.
 *
 * The prototype of the slot is:
 * void set_input_hint (const HelperAgent *agent, uint32 &input_hint);
 */
Connection
HelperAgent::signal_connect_set_input_hint (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_input_hint.connect (slot);
}

/**
 * @brief Connect a slot to Helper set BiDi direction signal.
 *
 * This signal is used to set Helper ISE BiDi direction.
 *
 * The prototype of the slot is:
 * void update_bidi_direction (const HelperAgent *agent, uint32 &bidi_direction);
 */
Connection
HelperAgent::signal_connect_update_bidi_direction (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_update_bidi_direction.connect (slot);
}

/**
 * @brief Connect a slot to Helper set shift mode signal.
 *
 * This signal is used to set Helper shift mode.
 *
 * The prototype of the slot is:
 * void set_caps_mode (const HelperAgent *agent, uint32 &mode);
 */
Connection
HelperAgent::signal_connect_set_caps_mode (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_caps_mode.connect (slot);
}

/**
 * @brief Connect a slot to Helper reset input context signal.
 *
 * This signal is used to reset Helper ISE input context.
 *
 * The prototype of the slot is:
 * void reset_input_context (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_reset_input_context (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_reset_input_context.connect (slot);
}

/**
 * @brief Connect a slot to Helper update candidate window geometry signal.
 *
 * This signal is used to get candidate window size and position.
 *
 * The prototype of the slot is:
 * void update_candidate_geometry (const HelperAgent *agent, int ic, const String &uuid, const rectinfo &info);
 */
Connection
HelperAgent::signal_connect_update_candidate_geometry (HelperAgentSlotRect *slot)
{
    return m_impl->signal_update_candidate_geometry.connect (slot);
}

/**
 * @brief Connect a slot to Helper update keyboard ISE signal.
 *
 * This signal is used to get current keyboard ISE name and uuid.
 *
 * The prototype of the slot is:
 * void update_keyboard_ise (const HelperAgent *agent, int ic, const String &uuid,
 *                           const String &ise_name, const String &ise_uuid);
 */
Connection
HelperAgent::signal_connect_update_keyboard_ise (HelperAgentSlotString2 *slot)
{
    return m_impl->signal_update_keyboard_ise.connect (slot);
}

/**
 * @brief Connect a slot to Helper update keyboard ISE list signal.
 *
 * This signal is used to get uuid list of all keyboard ISEs.
 *
 * The prototype of the slot is:
 * void update_keyboard_ise_list (const HelperAgent *agent, int ic, const String &uuid,
 *                                const std::vector<String> &ise_list);
 */
Connection
HelperAgent::signal_connect_update_keyboard_ise_list (HelperAgentSlotStringVector *slot)
{
    return m_impl->signal_update_keyboard_ise_list.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate more window show signal.
 *
 * This signal is used to do someting when candidate more window is showed.
 *
 * The prototype of the slot is:
 * void candidate_more_window_show (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_more_window_show (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_more_window_show.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate more window hide signal.
 *
 * This signal is used to do someting when candidate more window is hidden.
 *
 * The prototype of the slot is:
 * void candidate_more_window_hide (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_more_window_hide (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_more_window_hide.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate show signal.
 *
 * This signal is used to do candidate show.
 *
 * The prototype of the slot is:
 * void candidate_show (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_show (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_show.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate hide signal.
 *
 * This signal is used to do candidate hide.
 *
 * The prototype of the slot is:
 * void candidate_hide (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_hide (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_hide.connect (slot);
}

/**
 * @brief Connect a slot to Helper update lookup table signal.
 *
 * This signal is used to do someting when update lookup table.
 *
 * The prototype of the slot is:
 * void update_lookup_table (const HelperAgent *agent, int ic, const String &uuid ,LookupTable &table);
 */
Connection
HelperAgent::signal_connect_update_lookup_table (HelperAgentSlotLookupTable *slot)
{
    return m_impl->signal_update_lookup_table.connect (slot);
}

/**
 * @brief Connect a slot to Helper select aux signal.
 *
 * This signal is used to do something when aux is selected.
 *
 * The prototype of the slot is:
 * void select_aux (const HelperAgent *agent, int ic, const String &uuid, int index);
 */
Connection
HelperAgent::signal_connect_select_aux (HelperAgentSlotInt *slot)
{
    return m_impl->signal_select_aux.connect (slot);
}

/**
 * @brief Connect a slot to Helper select candidate signal.
 *
 * This signal is used to do something when candidate is selected.
 *
 * The prototype of the slot is:
 * void select_candidate (const HelperAgent *agent, int ic, const String &uuid, int index);
 */
Connection
HelperAgent::signal_connect_select_candidate (HelperAgentSlotInt *slot)
{
    return m_impl->signal_select_candidate.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate table page up signal.
 *
 * This signal is used to do something when candidate table is paged up.
 *
 * The prototype of the slot is:
 * void candidate_table_page_up (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_table_page_up (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_table_page_up.connect (slot);
}

/**
 * @brief Connect a slot to Helper candidate table page down signal.
 *
 * This signal is used to do something when candidate table is paged down.
 *
 * The prototype of the slot is:
 * void candidate_table_page_down (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_candidate_table_page_down (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_candidate_table_page_down.connect (slot);
}

/**
 * @brief Connect a slot to Helper update candidate table page size signal.
 *
 * This signal is used to do something when candidate table page size is changed.
 *
 * The prototype of the slot is:
 * void update_candidate_table_page_size (const HelperAgent *, int ic, const String &uuid, int page_size);
 */
Connection
HelperAgent::signal_connect_update_candidate_table_page_size (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_candidate_table_page_size.connect (slot);
}

/**
 * @brief Connect a slot to Helper update candidate item layout signal.
 *
 * The prototype of the slot is:
 * void update_candidate_item_layout (const HelperAgent *, const std::vector<uint32> &row_items);
 */
Connection
HelperAgent::signal_connect_update_candidate_item_layout (HelperAgentSlotUintVector *slot)
{
    return m_impl->signal_update_candidate_item_layout.connect (slot);
}

/**
 * @brief Connect a slot to Helper select associate signal.
 *
 * This signal is used to do something when associate is selected.
 *
 * The prototype of the slot is:
 * void select_associate (const HelperAgent *agent, int ic, const String &uuid, int index);
 */
Connection
HelperAgent::signal_connect_select_associate (HelperAgentSlotInt *slot)
{
    return m_impl->signal_select_associate.connect (slot);
}

/**
 * @brief Connect a slot to Helper associate table page up signal.
 *
 * This signal is used to do something when associate table is paged up.
 *
 * The prototype of the slot is:
 * void associate_table_page_up (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_associate_table_page_up (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_associate_table_page_up.connect (slot);
}

/**
 * @brief Connect a slot to Helper associate table page down signal.
 *
 * This signal is used to do something when associate table is paged down.
 *
 * The prototype of the slot is:
 * void associate_table_page_down (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_associate_table_page_down (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_associate_table_page_down.connect (slot);
}

/**
 * @brief Connect a slot to Helper update associate table page size signal.
 *
 * This signal is used to do something when associate table page size is changed.
 *
 * The prototype of the slot is:
 * void update_associate_table_page_size (const HelperAgent *, int ic, const String &uuid, int page_size);
 */
Connection
HelperAgent::signal_connect_update_associate_table_page_size (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_associate_table_page_size.connect (slot);
}

/**
 * @brief Connect a slot to Helper turn on log signal.
 *
 * This signal is used to turn on Helper ISE debug information.
 *
 * The prototype of the slot is:
 * void turn_on_log (const HelperAgent *agent, uint32 &on);
 */
Connection
HelperAgent::signal_connect_turn_on_log (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_turn_on_log.connect (slot);
}

/**
 * @brief Connect a slot to Helper update displayed candidate number signal.
 *
 * This signal is used to inform helper ISE displayed candidate number.
 *
 * The prototype of the slot is:
 * void update_displayed_candidate_number (const HelperAgent *, int ic, const String &uuid, int number);
 */
Connection
HelperAgent::signal_connect_update_displayed_candidate_number (HelperAgentSlotInt *slot)
{
    return m_impl->signal_update_displayed_candidate_number.connect (slot);
}

/**
 * @brief Connect a slot to Helper longpress candidate signal.
 *
 * This signal is used to do something when candidate is longpress.
 *
 * The prototype of the slot is:
 * void longpress_candidate (const HelperAgent *agent, int ic, const String &uuid, int index);
 */
Connection
HelperAgent::signal_connect_longpress_candidate (HelperAgentSlotInt *slot)
{
    return m_impl->signal_longpress_candidate.connect (slot);
}

/**
 * @brief Connect a slot to Helper show option window.
 *
 * This signal is used to do request the ISE to show option window.
 *
 * The prototype of the slot is:
 * void show_option_window (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_show_option_window (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_show_option_window.connect (slot);
}

/**
 * @brief Connect a slot to Helper check if the option is available.
 *
 * This signal is used to check if the option (setting) is available from Helper ISE.
 *
 * The prototype of the slot is:
 * void check_option_window (const HelperAgent *agent, uint32 &avail);
 */
Connection
HelperAgent::signal_connect_check_option_window (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_check_option_window.connect (slot);
}

} /* namespace scim */

/**
 * @brief Connect a slot to Helper process unconventional input device event signal.
 *
 * This signal is used to send unconventional input device event to Helper ISE.
 *
 * The prototype of the slot is:
 * void process_input_device_event (const HelperAgent *, uint32 &type, char *data, size_t &size, uint32 &ret);
 */
Connection
HelperAgent::signal_connect_process_input_device_event (HelperAgentSlotUintCharSizeUint *slot)
{
    return m_impl->signal_process_input_device_event.connect (slot);
}

/*
vi:ts=4:nowrap:ai:expandtab
*/

