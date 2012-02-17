/** @file scim_helper.cpp
 *  @brief Implementation of class HelperAgent.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
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
 * $Id: scim_helper.cpp,v 1.13 2005/05/24 12:22:51 suzhe Exp $
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT

#include <string.h>

#include "scim_private.h"
#include "scim.h"

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

typedef Signal4 <void, const HelperAgent *, int, char *, size_t &>
        HelperAgentSignalIntRawVoid;

typedef Signal3 <void, const HelperAgent *, char **, size_t &>
        HelperAgentSignalGetIMDataVoid;

typedef Signal5 <void, const HelperAgent *, uint32 &, uint32 &, char *, char *>
        HelperAgentSignalSetPrivateKeyVoid;

typedef Signal4<void, const HelperAgent *, uint32 &, uint32 &, uint32 &>
        HelperAgentSignalSetDisableKeyVoid;

typedef Signal2<void, const HelperAgent *, std::vector<uint32> &>
        HelperAgentSignalUintVector;

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

    HelperAgentSignalVoid           signal_exit;
    HelperAgentSignalVoid           signal_attach_input_context;
    HelperAgentSignalVoid           signal_detach_input_context;
    HelperAgentSignalVoid           signal_reload_config;
    HelperAgentSignalInt            signal_update_screen;
    HelperAgentSignalIntInt         signal_update_spot_location;
    HelperAgentSignalInt            signal_update_cursor_position;
    HelperAgentSignalInt            signal_update_surrounding_text;
    HelperAgentSignalString         signal_trigger_property;
    HelperAgentSignalTransaction    signal_process_imengine_event;
    HelperAgentSignalVoid           signal_focus_out;
    HelperAgentSignalVoid           signal_focus_in;
    HelperAgentSignalIntRawVoid     signal_ise_show;
    HelperAgentSignalVoid           signal_ise_hide;
    HelperAgentSignalSize           signal_get_size;
    HelperAgentSignalUintVoid       signal_set_mode;
    HelperAgentSignalUintVoid       signal_set_language;
    HelperAgentSignalRawVoid        signal_set_imdata;
    HelperAgentSignalGetIMDataVoid  signal_get_imdata;
    HelperAgentSignalSetPrivateKeyVoid  signal_set_private_key_by_label;
    HelperAgentSignalSetPrivateKeyVoid  signal_set_private_key_by_image;
    HelperAgentSignalSetDisableKeyVoid  signal_set_disable_key;
    HelperAgentSignalUintVoid           signal_set_layout;
    HelperAgentSignalUintVoid           signal_get_layout;
    HelperAgentSignalUintVoid           signal_set_caps_mode;
    HelperAgentSignalUintVector         signal_get_layout_list;
    HelperAgentSignalVoid               signal_reset_input_context;
    HelperAgentSignalIntInt             signal_update_candidate_ui;
    HelperAgentSignalRect               signal_update_candidate_rect;
    HelperAgentSignalString2            signal_update_keyboard_ise;
    HelperAgentSignalStringVector       signal_update_keyboard_ise_list;
    HelperAgentSignalVoid               signal_candidate_more_window_show;
    HelperAgentSignalVoid               signal_candidate_more_window_hide;
    HelperAgentSignalInt                signal_select_aux;
    HelperAgentSignalInt                signal_select_candidate;
    HelperAgentSignalVoid               signal_candidate_table_page_up;
    HelperAgentSignalVoid               signal_candidate_table_page_down;
    HelperAgentSignalInt                signal_update_candidate_table_page_size;
    HelperAgentSignalInt                signal_select_associate;
    HelperAgentSignalVoid               signal_associate_table_page_up;
    HelperAgentSignalVoid               signal_associate_table_page_down;
    HelperAgentSignalInt                signal_update_associate_table_page_size;
    HelperAgentSignalVoid               signal_reset_ise_context;
    HelperAgentSignalUintVoid           signal_set_screen_direction;
    HelperAgentSignalUintVoid           signal_turn_on_log;

public:
    HelperAgentImpl () : magic (0), magic_active (0), timeout (-1) { }
};

HelperAgent::HelperAgent ()
    : m_impl (new HelperAgentImpl ())
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
    int timeout = scim_get_default_socket_timeout ();
    uint32 magic;

    if (!address.valid ())
        return -1;
    bool ret;
    int  i;
    ret = m_impl->socket.connect (address);
    if (ret == false) {
        scim_usleep (100000);
        std::cerr << " Re-connecting to PanelAgent server.";
        ISF_SYSLOG (" Re-connecting to PanelAgent server.\n");
        for (i = 0; i < 200; ++i) {
            if (m_impl->socket.connect (address)) {
                ret = true;
                break;
            }
            std::cerr << ".";
            scim_usleep (100000);
        }
        std::cerr << " Connected :" << i << "\n";
        ISF_SYSLOG ("  Connected :%d\n", i);
    }

    if (ret == false)
    {
        std::cerr << "m_impl->socket.connect () is failed!!!\n";
        ISF_SYSLOG ("m_impl->socket.connect () is failed!!!\n");
        return -1;
    }

    if (!scim_socket_open_connection (magic,
                                      String ("Helper"),
                                      String ("Panel"),
                                      m_impl->socket,
                                      timeout)) {
        m_impl->socket.close ();
        std::cerr << "scim_socket_open_connection () is failed!!!\n";
        ISF_SYSLOG ("scim_socket_open_connection () is failed!!!\n");
        return -1;
    }

    ISF_SYSLOG ("scim_socket_open_connection () is successful.\n");
    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (magic);
    m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_REGISTER_HELPER);
    m_impl->send.put_data (info.uuid);
    m_impl->send.put_data (info.name);
    m_impl->send.put_data (info.icon);
    m_impl->send.put_data (info.description);
    m_impl->send.put_data (info.option);

    if (!m_impl->send.write_to_socket (m_impl->socket, magic)) {
        m_impl->socket.close ();
        return -1;
    }

    int cmd;
    if (m_impl->recv.read_from_socket (m_impl->socket, timeout) &&
        m_impl->recv.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
        m_impl->recv.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        m_impl->magic = magic;
        m_impl->timeout = timeout;

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
    }

    /* connect to the panel agent as the active helper client */
    if (!m_impl->socket_active.connect (address)) return -1;
    if (!scim_socket_open_connection (magic,
                                      String ("Helper_Active"),
                                      String ("Panel"),
                                      m_impl->socket_active,
                                      timeout)) {
        m_impl->socket_active.close ();
        return -1;
    }

    m_impl->socket_active.set_nonblock_mode ();

    m_impl->magic_active = magic;

    m_impl->send.clear ();
    m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
    m_impl->send.put_data (magic);
    m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER);
    m_impl->send.put_data (info.uuid);
    m_impl->send.put_data (info.name);
    m_impl->send.put_data (info.icon);
    m_impl->send.put_data (info.description);
    m_impl->send.put_data (info.option);

    if (!m_impl->send.write_to_socket (m_impl->socket_active, magic)) {
        m_impl->socket_active.close ();
        return -1;
    }

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

    if (!m_impl->recv.get_command (cmd) || cmd != SCIM_TRANS_CMD_REPLY)
        return true;

    /* If there are ic and ic_uuid then read them. */
    if (!(m_impl->recv.get_data_type () == SCIM_TRANS_DATA_COMMAND) &&
        !(m_impl->recv.get_data (ic) && m_impl->recv.get_data (ic_uuid)))
        return true;

    while (m_impl->recv.get_command (cmd)) {
        switch (cmd) {
            case SCIM_TRANS_CMD_EXIT:
                m_impl->signal_exit (this, ic, ic_uuid);
                break;
            case SCIM_TRANS_CMD_RELOAD_CONFIG:
                m_impl->signal_reload_config (this, ic, ic_uuid);
                break;
            case SCIM_TRANS_CMD_UPDATE_SCREEN:
            {
                uint32 screen;
                if (m_impl->recv.get_data (screen))
                    m_impl->signal_update_screen (this, ic, ic_uuid, (int) screen);
                break;
            }
            case SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION:
            {
                uint32 x, y;
                if (m_impl->recv.get_data (x) && m_impl->recv.get_data (y))
                    m_impl->signal_update_spot_location (this, ic, ic_uuid, (int) x, (int) y);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CURSOR_POSITION:
            {
                uint32 cursor_pos;
                if (m_impl->recv.get_data (cursor_pos))
                    m_impl->signal_update_cursor_position (this, ic, ic_uuid, (int) cursor_pos);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT:
            {
                String text;
                uint32 cursor;
                if (m_impl->recv.get_data (text) && m_impl->recv.get_data (cursor))
                    m_impl->signal_update_surrounding_text (this, ic, text, (int) cursor);
                break;
            }
            case SCIM_TRANS_CMD_TRIGGER_PROPERTY:
            {
                String property;
                if (m_impl->recv.get_data (property))
                    m_impl->signal_trigger_property (this, ic, ic_uuid, property);
                break;
            }
            case SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT:
            {
                Transaction trans;
                if (m_impl->recv.get_data (trans))
                    m_impl->signal_process_imengine_event (this, ic, ic_uuid, trans);
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
                break;
            }
            case SCIM_TRANS_CMD_FOCUS_IN:
            {
                m_impl->signal_focus_in (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_SHOW_ISE:
            {
                char   *data = NULL;
                size_t  len;
                if (m_impl->recv.get_data (&data, len))
                    m_impl->signal_ise_show (this, ic, data, len);
                if (data)
                    delete [] data;
                break;
            }
            case ISM_TRANS_CMD_HIDE_ISE:
            {
                m_impl->signal_ise_hide (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_GET_ACTIVE_ISE_SIZE:
            {
                struct rectinfo info = {0, 0, 0, 0};
                m_impl->signal_get_size (this, info);
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
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_LANGUAGE:
            {
                uint32 language;
                if (m_impl->recv.get_data (language))
                    m_impl->signal_set_language (this, language);
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_IMDATA:
            {
                char   *imdata = NULL;
                size_t  len;
                if (m_impl->recv.get_data (&imdata, len))
                    m_impl->signal_set_imdata (this, imdata, len);

                if (NULL != imdata)
                    delete[] imdata;
                break;
            }
            case ISM_TRANS_CMD_GET_ISE_IMDATA:
            {
                char   *buf = NULL;
                size_t  len = 0;

                m_impl->signal_get_imdata (this, &buf, len);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (buf, len);
                m_impl->send.write_to_socket (m_impl->socket);
                if (NULL != buf)
                    delete[] buf;
                break;
            }
            case ISM_TRANS_CMD_SET_PRIVATE_KEY:
            {
                uint32  layout_idx, key_idx;
                char   *label = NULL;
                size_t  len1;
                char   *value = NULL;
                size_t  len2;

                if (m_impl->recv.get_data (layout_idx)
                    && m_impl->recv.get_data (key_idx)
                    && m_impl->recv.get_data (&label, len1)
                    && m_impl->recv.get_data (&value, len2))
                {
                    m_impl->signal_set_private_key_by_label (this,
                                                             layout_idx,
                                                             key_idx,
                                                             label,
                                                             value);
                }
                if (NULL != label)
                    delete[] label;
                if (NULL != value)
                    delete[] value;
                break;
            }
            case ISM_TRANS_CMD_SET_PRIVATE_KEY_BY_IMG:
            {
                uint32  layout_idx, key_idx;
                char   *image = NULL;
                size_t  len1;
                char   *value = NULL;
                size_t  len2;

                if (m_impl->recv.get_data (layout_idx)
                    && m_impl->recv.get_data (key_idx)
                    && m_impl->recv.get_data (&image, len1)
                    && m_impl->recv.get_data (&value, len2))
                {
                    m_impl->signal_set_private_key_by_image (this,
                                                             layout_idx,
                                                             key_idx,
                                                             image,
                                                             value);
                }
                if (NULL != image)
                    delete[] image;
                if (NULL != value)
                    delete[] value;
                break;
            }
            case ISM_TRANS_CMD_SET_DISABLE_KEY:
            {
                uint32  layout_idx, key_idx, disabled;

                if (m_impl->recv.get_data (layout_idx)
                    && m_impl->recv.get_data (key_idx)
                    && m_impl->recv.get_data (disabled))
                {
                    m_impl->signal_set_disable_key (this,
                                                    layout_idx,
                                                    key_idx,
                                                    disabled);
                }
                break;
            }
            case ISM_TRANS_CMD_SET_LAYOUT:
            {
                uint32 layout;

                if (m_impl->recv.get_data (layout))
                    m_impl->signal_set_layout (this, layout);
                break;
            }
            case ISM_TRANS_CMD_GET_LAYOUT_LIST:
            {
                std::vector<uint32> list;
                list.clear();

                m_impl->signal_get_layout_list (this, list);
                m_impl->send.clear ();
                m_impl->send.put_command (SCIM_TRANS_CMD_REPLY);
                m_impl->send.put_data (list);
                m_impl->send.write_to_socket (m_impl->socket);
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
            case ISM_TRANS_CMD_SET_CAPS_MODE:
            {
                uint32 mode;

                if (m_impl->recv.get_data (mode))
                    m_impl->signal_set_caps_mode (this, mode);
                break;
            }
            case SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT:
            {
                m_impl->signal_reset_input_context (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CANDIDATE_UI:
            {
                uint32 style, mode;
                if (m_impl->recv.get_data (style) && m_impl->recv.get_data (mode))
                    m_impl->signal_update_candidate_ui (this, ic, ic_uuid, style, mode);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_CANDIDATE_RECT:
            {
                struct rectinfo info = {0, 0, 0, 0};
                if (m_impl->recv.get_data (info.pos_x)
                    && m_impl->recv.get_data (info.pos_y)
                    && m_impl->recv.get_data (info.width)
                    && m_impl->recv.get_data (info.height))
                    m_impl->signal_update_candidate_rect (this, ic, ic_uuid, info);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE:
            {
                String name, uuid;
                if (m_impl->recv.get_data (name) && m_impl->recv.get_data (uuid))
                    m_impl->signal_update_keyboard_ise (this, ic, ic_uuid, name, uuid);
                break;
            }
            case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST:
            {
                uint32 num;
                String ise;
                std::vector<String> list;
                if (m_impl->recv.get_data (num))
                {
                    for (unsigned int i = 0; i < num; i++)
                    {
                        if (m_impl->recv.get_data (ise))
                            list.push_back (ise);
                        else
                        {
                            list.clear ();
                            break;
                        }
                    }
                    m_impl->signal_update_keyboard_ise_list (this, ic, ic_uuid, list);
                }
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW:
            {
                m_impl->signal_candidate_more_window_show (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE:
            {
                m_impl->signal_candidate_more_window_hide (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_SELECT_AUX:
            {
                uint32 item;
                if (m_impl->recv.get_data (item))
                    m_impl->signal_select_aux (this, ic, ic_uuid, item);
                break;
            }
            case SCIM_TRANS_CMD_SELECT_CANDIDATE:
            {
                uint32 item;
                if (m_impl->recv.get_data (item))
                    m_impl->signal_select_candidate (this, ic, ic_uuid, item);
                break;
            }
            case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP:
            {
                m_impl->signal_candidate_table_page_up (this, ic, ic_uuid);
                break;
            }
            case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN:
            {
                m_impl->signal_candidate_table_page_down (this, ic, ic_uuid);
                break;
            }
            case SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE:
            {
                uint32 size;
                if (m_impl->recv.get_data (size))
                    m_impl->signal_update_candidate_table_page_size (this, ic, ic_uuid, size);
                break;
            }
            case ISM_TRANS_CMD_SELECT_ASSOCIATE:
            {
                uint32 item;
                if (m_impl->recv.get_data (item))
                    m_impl->signal_select_associate (this, ic, ic_uuid, item);
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
                break;
            }
            case ISM_TRANS_CMD_RESET_ISE_CONTEXT:
            {
                m_impl->signal_reset_ise_context (this, ic, ic_uuid);
                break;
            }
            case ISM_TRANS_CMD_SET_ISE_SCREEN_DIRECTION:
            {
                uint32 direction;
                if (m_impl->recv.get_data (direction))
                    m_impl->signal_set_screen_direction (this, direction);
                break;
            }
            case ISM_TRANS_CMD_TURN_ON_LOG:
            {
                uint32 isOn;
                if (m_impl->recv.get_data (isOn))
                    m_impl->signal_turn_on_log (this, isOn);
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
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_RELOAD_CONFIG);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
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
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT);
        m_impl->send.put_data ((uint32)ic);
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
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_FORWARD_KEY_EVENT);
        m_impl->send.put_data ((uint32)ic);
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
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (wstr);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Commit a buffer to client application by IMControl.
 *
 * @param buf The result context.
 * @param len The length of buffer.
 */
void
HelperAgent::commit_ise_result_to_imcontrol (char *buf, size_t len) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_ISE_RESULT_TO_IMCONTROL);
        m_impl->send.put_data (buf, len);
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
 * @param wstr The WideString to be updated.
 * @param attrs The attribute list for preedit string.
 */
void
HelperAgent::update_preedit_string (int                  ic,
                                    const String        &ic_uuid,
                                    const WideString    &str,
                                    const AttributeList &attrs) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
        m_impl->send.put_data ((uint32)ic);
        m_impl->send.put_data (ic_uuid);
        m_impl->send.put_data (str);
        m_impl->send.put_data (attrs);
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
 * @ brief Request to get surrounding text.
 *
 * @param uuid The helper ISE UUID.
 * @param maxlen_before The max length of before.
 * @param maxlen_after The max length of after.
 */
void
HelperAgent::get_surrounding_text (const String &uuid, int maxlen_before, int maxlen_after) const
{
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
}

/**
 * @ brief Request to delete surrounding text.
 *
 * @param offset The offset for cursor position.
 * @param len The length for delete text.
 */
void
HelperAgent::delete_surrounding_text (int offset, int len) const
{
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
 * @brief Request to get uuid list of all keyboard ISEs.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_keyboard_ise_list (const String &uuid) const
{
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
 * @brief Set new candidate UI.
 *
 * @param style style of new candidate UI.
 * @param mode mode of new candidate UI.
 */
void
HelperAgent::set_candidate_ui (const ISF_CANDIDATE_STYLE_T style,
                               const ISF_CANDIDATE_MODE_T  mode) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SET_CANDIDATE_UI);
        m_impl->send.put_data (style);
        m_impl->send.put_data (mode);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get current candidate style and mode.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_candidate_ui (const String &uuid) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_CANDIDATE_UI);
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
 * @brief Request to hide candidate window.
 */
void
HelperAgent::candidate_hide (void) const
{
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
HelperAgent::get_candidate_window_rect (const String &uuid) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_CANDIDATE_RECT);
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
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Request to get current keyboard ISE information.
 *
 * @param uuid The helper ISE UUID.
 */
void
HelperAgent::get_keyboard_ise (const String &uuid) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_GET_KEYBOARD_ISE);
        m_impl->send.put_data (uuid);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
    }
}

/**
 * @brief Launch another helper ISE.
 *
 * @param loop If loop is true, this function will launch helper ISE by loop.
 *             If loop is false, this function will popup helper ISE list window.
 */
void
HelperAgent::launch_helper_ise (const bool loop) const
{
    if (m_impl->socket_active.is_connected ()) {
        m_impl->send.clear ();
        m_impl->send.put_command (SCIM_TRANS_CMD_REQUEST);
        m_impl->send.put_data (m_impl->magic_active);
        m_impl->send.put_command (ISM_TRANS_CMD_LAUNCH_HELPER_ISE_LIST_SELECTION);
        if (loop)
            m_impl->send.put_data (0);
        else
            m_impl->send.put_data (1);
        m_impl->send.write_to_socket (m_impl->socket_active, m_impl->magic_active);
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
 * @brief Connect a slot to Helper get rect signal.
 *
 * This signal is used to get Helper ISE window size and position.
 *
 * The prototype of the slot is:
 * void get_rect (const HelperAgent *agent, struct rectinfo &info);
 */
Connection
HelperAgent::signal_connect_get_size (HelperAgentSlotSize *slot)
{
    return m_impl->signal_get_size.connect (slot);
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
HelperAgent::signal_connect_get_imdata (HelperAgentSlotGetIMDataVoid *slot)
{
    return m_impl->signal_get_imdata.connect (slot);
}

/**
 * @brief Connect a slot to Helper set label private key signal.
 *
 * This signal is used to send label private key to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_private_key_by_label (const HelperAgent *agent, uint32 &layout_idx,
 *                                uint32 & key_idx, char *label, char *value);
 */
Connection
HelperAgent::signal_connect_set_private_key_by_label (HelperAgentSlotSetPrivateKeyVoid *slot)
{
    return m_impl->signal_set_private_key_by_label.connect (slot);
}

/**
 * @brief Connect a slot to Helper set image private key signal.
 *
 * This signal is used to send image private key to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_private_key_by_image (const HelperAgent *agent, uint32 &layout_idx,
 *                                uint32 & key_idx, char *img_path, char *value);
 */
Connection
HelperAgent::signal_connect_set_private_key_by_image (HelperAgentSlotSetPrivateKeyVoid *slot)
{
    return m_impl->signal_set_private_key_by_image.connect (slot);
}

/**
 * @brief Connect a slot to Helper set disable key signal.
 *
 * This signal is used to send disable key to Helper ISE.
 *
 * The prototype of the slot is:
 * void set_disable_key (const HelperAgent *agent, uint32 &layout_idx, uint32 &key_idx, uint32 &disabled);
 */
Connection
HelperAgent::signal_connect_set_disable_key (HelperAgentSlotSetDisableKeyVoid *slot)
{
    return m_impl->signal_set_disable_key.connect (slot);
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
 * @brief Connect a slot to Helper to get layout list signal.
 *
 * This signal is used to get the availabe Helper ISE layout list.
 *
 * The prototype of the slot is:
 * void get_layout_list (const HelperAgent *agent, std::vector<uint32> &list);
 */
Connection
HelperAgent::signal_connect_get_layout_list (HelperAgentSlotUintVector *slot)
{
    return m_impl->signal_get_layout_list.connect (slot);
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
 * @brief Connect a slot to Helper update candidate ui signal.
 *
 * This signal is used to get candidate ui style and mode.
 *
 * The prototype of the slot is:
 * void update_candidate_ui (const HelperAgent *agent, int ic, const String &uuid, int style, int mode);
 */
Connection
HelperAgent::signal_connect_update_candidate_ui (HelperAgentSlotIntInt *slot)
{
    return m_impl->signal_update_candidate_ui.connect (slot);
}

/**
 * @brief Connect a slot to Helper update candidate rect signal.
 *
 * This signal is used to get candidate window size and position.
 *
 * The prototype of the slot is:
 * void update_candidate_rect (const HelperAgent *agent, int ic, const String &uuid, const rectinfo &info);
 */
Connection
HelperAgent::signal_connect_update_candidate_rect (HelperAgentSlotRect *slot)
{
    return m_impl->signal_update_candidate_rect.connect (slot);
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
 * @brief Connect a slot to Helper reset ISE context signal.
 *
 * This signal is used to reset Helper ISE.
 *
 * The prototype of the slot is:
 * void reset_ise_context (const HelperAgent *agent, int ic, const String &uuid);
 */
Connection
HelperAgent::signal_connect_reset_ise_context (HelperAgentSlotVoid *slot)
{
    return m_impl->signal_reset_ise_context.connect (slot);
}

/**
 * @brief Connect a slot to Helper set screen direction signal.
 *
 * This signal is used to rotate and resize Helper when screen direction is changed.
 *
 * The prototype of the slot is:
 * void set_screen_direction (const HelperAgent *agent, uint32 &mode);
 */
Connection
HelperAgent::signal_connect_set_screen_direction (HelperAgentSlotUintVoid *slot)
{
    return m_impl->signal_set_screen_direction.connect (slot);
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

} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/

