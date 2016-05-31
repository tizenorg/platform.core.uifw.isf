/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_REMOTE_CLIENT
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE

#include <string.h>
#include <dlog.h>
#include "scim.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_REMOTE_CLIENT"

namespace scim
{

typedef Signal1<void, int> RemoteInputClientSignalVoid;

#if ENABLE_LAZY_LAUNCH
static bool check_panel (const String &display)
{
    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_panel_socket_address (display));

    if (!client.connect (address))
        return false;

    if (!scim_socket_open_connection (magic,
        String ("ConnectionTester"),
        String ("Panel"),
        client,
        1000)) {
        return false;
    }

    return true;
}

static bool
check_socket_frontend (void)
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    SocketAddress address;
    SocketClient client;

    uint32 magic;

    address.set_address (scim_get_default_socket_frontend_address ());

    if (!client.connect (address)) {
        std::cerr << "check_socket_frontend's connect () failed.\n";
        return false;
    }

    if (!scim_socket_open_connection (magic,
                                      String ("ConnectionTester"),
                                      String ("SocketFrontEnd"),
                                      client,
                                      500)) {
        std::cerr << "check_socket_frontend's scim_socket_open_connection () failed.\n";
        return false;
    }

    return true;
}

static int
launch_socket_frontend ()
{
    SCIM_DEBUG_FRONTEND(1) << __FUNCTION__ << "...\n";

    std::vector<String>     engine_list;
    std::vector<String>     helper_list;
    std::vector<String>     load_engine_list;

    std::vector<String>::iterator it;

    std::cerr << "Launching a ISF daemon with Socket FrontEnd...\n";
    //get modules list
    scim_get_imengine_module_list (engine_list);
    scim_get_helper_module_list (helper_list);

    for (it = engine_list.begin (); it != engine_list.end (); it++) {
        if (*it != "socket")
            load_engine_list.push_back (*it);
    }
    for (it = helper_list.begin (); it != helper_list.end (); it++)
        load_engine_list.push_back (*it);

    return scim_launch (true,
        "simple",
        (load_engine_list.size () > 0 ? scim_combine_string_list (load_engine_list, ',') : "none"),
        "socket",
        NULL);
}
#endif

class RemoteInputClient::RemoteInputClientImpl
{
    SocketClient                                m_socket_remoteinput2panel;
    SocketClient                                m_socket_panel2remoteinput;
    int                                         m_socket_timeout;
    uint32                                      m_socket_r2p_magic_key;
    uint32                                      m_socket_p2r_magic_key;
    Transaction                                 m_trans;
    Transaction                                 m_trans_recv;
    RemoteInputClientSignalVoid                 m_signal_show_ise;
    RemoteInputClientSignalVoid                 m_signal_hide_ise;

    String m_default_text;
    uint32 m_hint, m_cursor, m_layout, m_variation, m_autocapital_type;

public:
    RemoteInputClientImpl ()
          : m_socket_timeout (scim_get_default_socket_timeout ()),
            m_socket_r2p_magic_key (0),
            m_socket_p2r_magic_key (0),
            m_default_text (""),
            m_hint (0),
            m_cursor (0),
            m_layout (0),
            m_variation (0),
            m_autocapital_type (0) {
    }

    int open_connection (void) {
        const char *p = getenv ("DISPLAY");
        String display;
        if (p) display = String (p);

        SocketAddress addr (scim_get_default_panel_socket_address (display));

        if (m_socket_remoteinput2panel.is_connected ()) close_connection ();

        bool ret=false, ret2=false;
        int count = 0;

        /* Try three times. */
        while (1) {
            ret = m_socket_remoteinput2panel.connect (addr);
            ret2 = m_socket_panel2remoteinput.connect (addr);
            if (!ret) {
                scim_usleep (100000);
#if ENABLE_LAZY_LAUNCH
                if (!check_socket_frontend ())
                    launch_socket_frontend ();
                if (!check_panel (display))
                    scim_launch_panel (true, "socket", display, NULL);
#endif
                for (int i = 0; i < 200; ++i) {
                    if (m_socket_remoteinput2panel.connect (addr)) {
                        ret = true;
                        break;
                    }
                    scim_usleep (100000);
                }
            }

            if (ret && scim_socket_open_connection (m_socket_r2p_magic_key, String ("RemoteInput_Active"), String ("Panel"), m_socket_remoteinput2panel, m_socket_timeout)) {
                if (ret2 && scim_socket_open_connection (m_socket_p2r_magic_key, String ("RemoteInput_Passive"), String ("Panel"), m_socket_panel2remoteinput, m_socket_timeout))
                    break;
            }
            m_socket_remoteinput2panel.close ();
            m_socket_panel2remoteinput.close ();

            if (count++ >= 3) break;

            scim_usleep (100000);
        }

        return m_socket_remoteinput2panel.get_id ();
    }

    void close_connection (void) {
        m_socket_remoteinput2panel.close ();
        m_socket_panel2remoteinput.close ();
        m_socket_r2p_magic_key = 0;
        m_socket_p2r_magic_key = 0;
    }

    bool is_connected (void) const {
        return (m_socket_remoteinput2panel.is_connected () && m_socket_panel2remoteinput.is_connected ());
    }

    int  get_panel2remote_connection_number (void) const {
        if (m_socket_panel2remoteinput.is_connected ())
            return m_socket_panel2remoteinput.get_id ();
        return -1;
    }

    bool prepare (void) {
        if (!m_socket_remoteinput2panel.is_connected ()) return false;

        m_trans.clear ();
        m_trans.put_command (SCIM_TRANS_CMD_REQUEST);
        m_trans.put_data (m_socket_r2p_magic_key);

        return true;
    }

    bool has_pending_event (void) const {
        if (m_socket_panel2remoteinput.is_connected () && m_socket_panel2remoteinput.wait_for_data (0) > 0)
            return true;

        return false;
    }

    bool send_remote_input_message (const char* str) {
        int cmd;

        m_trans.put_command (ISM_TRANS_CMD_SEND_REMOTE_INPUT_MESSAGE);
        m_trans.put_data (str, strlen (str)+1);
        m_trans.write_to_socket (m_socket_remoteinput2panel);
        if (!m_trans.read_from_socket (m_socket_remoteinput2panel, m_socket_timeout)) {
            std::cerr << __func__ << " read_from_socket() may be timeout \n";
            return false;
        }

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            std::cerr << __func__ << " get_command() or get_data() may fail!!!\n";
            return false;
        }

        return true;
    }

    remote_control_callback_type recv_callback_message(void) {
        if (! m_socket_panel2remoteinput.is_connected () || ! m_trans_recv.read_from_socket(m_socket_panel2remoteinput, m_socket_timeout))
            return REMOTE_CONTROL_CALLBACK_ERROR;

        int cmd;

        if (! m_trans_recv.get_command (cmd) || cmd != SCIM_TRANS_CMD_REPLY) {
            LOGW ("wrong format of transaction\n");
            return REMOTE_CONTROL_CALLBACK_ERROR;
        }

        remote_control_callback_type type = REMOTE_CONTROL_CALLBACK_ERROR; 
        while (m_trans_recv.get_command (cmd)) {
            LOGD ("RemoteInput_Client::cmd = %d\n", cmd);
            switch (cmd) {
                case ISM_TRANS_CMD_RECV_REMOTE_FOCUS_IN:
                    type = REMOTE_CONTROL_CALLBACK_FOCUS_IN;
                    break;
                case ISM_TRANS_CMD_RECV_REMOTE_FOCUS_OUT:
                    type = REMOTE_CONTROL_CALLBACK_FOCUS_OUT;
                    break;
                case ISM_TRANS_CMD_RECV_REMOTE_ENTRY_METADATA:
                {
                    type = REMOTE_CONTROL_CALLBACK_ENTRY_METADATA;
                    
                    if (m_trans_recv.get_data (m_hint) && m_trans_recv.get_data (m_layout) && m_trans_recv.get_data (m_variation) && 
                        m_trans_recv.get_data (m_autocapital_type)) {
                    }
                    else
                        LOGW ("wrong format of transaction\n");
                    break;
                }
                case ISM_TRANS_CMD_RECV_REMOTE_DEFAULT_TEXT:
                {
                    type = REMOTE_CONTROL_CALLBACK_DEFAULT_TEXT;

                    if (m_trans_recv.get_data (m_default_text) && m_trans_recv.get_data (m_cursor)) {
                    }
                    else
                        LOGW ("wrong format of transaction\n");
                    break;
                }
                default:
                    break;
            }
        }
        return type;
    }

    void get_entry_metadata (int *hint, int *layout, int *variation, int *autocapital_type) {
        *hint = m_hint;
        *layout = m_layout;
        *variation = m_variation;
        *autocapital_type = m_autocapital_type;
    }

    void get_default_text (String &default_text, int *cursor) {
        default_text = m_default_text;
        *cursor = m_cursor;
    }
};

RemoteInputClient::RemoteInputClient ()
        : m_impl (new RemoteInputClientImpl ())
{
}

RemoteInputClient::~RemoteInputClient ()
{
    delete m_impl;
}

int
RemoteInputClient::open_connection (void)
{
    return m_impl->open_connection ();
}

void
RemoteInputClient::close_connection (void)
{
    m_impl->close_connection ();
}

bool RemoteInputClient::is_connected (void) const
{
    return m_impl->is_connected ();
}

int RemoteInputClient::get_panel2remote_connection_number (void) const
{
    return m_impl->get_panel2remote_connection_number ();
}

bool
RemoteInputClient::prepare (void)
{
    return m_impl->prepare ();
}

bool
RemoteInputClient::has_pending_event (void) const
{
    return m_impl->has_pending_event ();
}

bool
RemoteInputClient::send_remote_input_message (const char* str)
{
    return m_impl->send_remote_input_message (str);
}

remote_control_callback_type
RemoteInputClient::recv_callback_message (void)
{
    return m_impl->recv_callback_message ();
}

void
RemoteInputClient::get_entry_metadata (int *hint, int *layout, int *variation, int *autocapital_type)
{
    m_impl->get_entry_metadata (hint, layout, variation, autocapital_type);
}

void
RemoteInputClient::get_default_text (String &default_text, int *cursor)
{
    m_impl->get_default_text (default_text, cursor);   
}
};

/*
vi:ts=4:nowrap:ai:expandtab
*/