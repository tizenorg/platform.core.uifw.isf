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

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT


#include <string.h>
#include "scim.h"

#define IMCONTROLDBG(str...)
#define IMCONTROLERR(str...) printf(str)

namespace scim
{

typedef Signal1<void, int> IMControlClientSignalVoid;

class IMControlClient::IMControlClientImpl
{
    SocketClient                                m_socket_imclient2panel;
    SocketClient                                m_socket_panel2imclient;
    int                                         m_socket_timeout;
    uint32                                      m_socket_i2p_magic_key;
    uint32                                      m_socket_p2i_magic_key;
    Transaction                                 m_trans;

    IMControlClientSignalVoid                   m_signal_show_ise;
    IMControlClientSignalVoid                   m_signal_hide_ise;

public:
    IMControlClientImpl ()
          : m_socket_timeout (scim_get_default_socket_timeout ()),
            m_socket_i2p_magic_key (0),
            m_socket_p2i_magic_key (0) {
    }

    int open_connection () {
        String config = "";
        String display = String(getenv ("DISPLAY"));

        SocketAddress addr (scim_get_default_panel_socket_address (display));

        if (m_socket_imclient2panel.is_connected ()) close_connection ();

        bool ret=false, ret2=false;
        int count = 0;

        /* Try three times. */
        while (1) {
            ret = m_socket_imclient2panel.connect (addr);
            ret2 = m_socket_panel2imclient.connect (addr);
            if (!ret) {
                scim_usleep (100000);
                scim_launch_panel (true, config, display, NULL);
                for (int i = 0; i < 200; ++i) {
                    if (m_socket_imclient2panel.connect (addr)) {
                        ret = true;
                        break;
                    }
                    scim_usleep (100000);
                }
            }

            if (ret && scim_socket_open_connection (m_socket_i2p_magic_key, String ("IMControl_Active"), String ("Panel"), m_socket_imclient2panel, m_socket_timeout)) {
                if (ret2 && scim_socket_open_connection (m_socket_p2i_magic_key, String ("IMControl_Passive"), String ("Panel"), m_socket_panel2imclient, m_socket_timeout))
                    break;
            }
            m_socket_imclient2panel.close ();
            m_socket_panel2imclient.close ();

            if (count++ >= 3) break;

            scim_usleep (100000);
        }

        return m_socket_imclient2panel.get_id ();
    }

    void close_connection () {
        m_socket_imclient2panel.close ();
        m_socket_panel2imclient.close ();
        m_socket_i2p_magic_key = 0;
        m_socket_p2i_magic_key = 0;
    }

    bool is_connected () const {
        return (m_socket_imclient2panel.is_connected () && m_socket_panel2imclient.is_connected ());
    }

    int  get_panel2imclient_connection_number  () const {
        return m_socket_panel2imclient.get_id ();
    }

    bool prepare () {
        if (!m_socket_imclient2panel.is_connected ()) return false;

        m_trans.clear ();
        m_trans.put_command (SCIM_TRANS_CMD_REQUEST);
        m_trans.put_data (m_socket_i2p_magic_key);

        return true;
    }

    bool send () {
        if (!m_socket_imclient2panel.is_connected ()) return false;
        if (m_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
            return m_trans.write_to_socket (m_socket_imclient2panel, 0x4d494353);
        return false;
    }


    void show_ise (void *data, int length) {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISE_PANEL);
        m_trans.put_data ((const char *)data, (size_t)length);
    }

    void hide_ise () {
        m_trans.put_command (ISM_TRANS_CMD_HIDE_ISE_PANEL);
    }

    void show_control_panel () {
        m_trans.put_command (ISM_TRANS_CMD_SHOW_ISF_CONTROL);
    }

    void hide_control_panel () {
        m_trans.put_command (ISM_TRANS_CMD_HIDE_ISF_CONTROL);
    }

    void set_mode (int mode) {
        m_trans.put_command (ISM_TRANS_CMD_SET_ISE_MODE);
        m_trans.put_data (mode);
    }

    void set_imdata (const char* data, int len) {
        m_trans.put_command (ISM_TRANS_CMD_SET_ISE_IMDATA);
        m_trans.put_data (data, len);
    }
    void get_imdata (char* data, int* len) {
        int cmd;
        size_t datalen = 0;
        char* data_temp = NULL;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_IMDATA);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (&data_temp, datalen)) {
            memcpy (data, data_temp, datalen);
            *len = datalen;
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
        delete [] data_temp;
    }

    void get_window_rect (int* x, int* y, int* width, int* height) {
        int cmd;
        uint32 x_temp = 0;
        uint32 y_temp = 0;
        uint32 w_temp = 0;
        uint32 h_temp = 0;

        m_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_SIZE);
        m_trans.write_to_socket (m_socket_imclient2panel);

        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
            m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
            m_trans.get_data (x_temp) &&
            m_trans.get_data (y_temp) &&
            m_trans.get_data (w_temp) &&
            m_trans.get_data (h_temp)) {
            *x = x_temp;
            *y = y_temp;
            *width = w_temp;
            *height = h_temp;
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
    }

    void set_private_key (int layout_index, int key_index, const char *label, const char *value) {
        m_trans.put_command (ISM_TRANS_CMD_SET_PRIVATE_KEY);
        m_trans.put_data (layout_index);
        m_trans.put_data (key_index);
        m_trans.put_data (label, strlen(label)+1);
        m_trans.put_data (value, strlen(value)+1);

    }
    void set_private_key_by_image (int layout_index, int key_index, const char *img_path, const char *value) {
        m_trans.put_command (ISM_TRANS_CMD_SET_PRIVATE_KEY_BY_IMG);
        m_trans.put_data (layout_index);
        m_trans.put_data (key_index);
        m_trans.put_data (img_path, strlen(img_path)+1);
        m_trans.put_data (value, strlen(value)+1);
    }

    void set_disable_key (int layout_index, int key_index, int disabled) {
        m_trans.put_command (ISM_TRANS_CMD_SET_DISABLE_KEY);
        m_trans.put_data (layout_index);
        m_trans.put_data (key_index);
        m_trans.put_data (disabled);
    }

    void set_layout (int layout) {
        m_trans.put_command (ISM_TRANS_CMD_SET_LAYOUT);
        m_trans.put_data (layout);
    }

    void get_layout (int* layout) {
        int cmd;
        uint32 layout_temp;

        m_trans.put_command (ISM_TRANS_CMD_GET_LAYOUT);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (layout_temp)) {
            *layout = layout_temp;
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
    }

    void set_ise_language (int language) {
        m_trans.put_command (ISM_TRANS_CMD_SET_ISE_LANGUAGE);
        m_trans.put_data (language);
    }

    void reset () {
        m_trans.put_command (ISM_TRANS_CMD_RESET_ISE_CONTEXT);
    }

    void set_screen_orientation (int orientation) {
        m_trans.put_command (ISM_TRANS_CMD_SET_ISE_SCREEN_DIRECTION);
        m_trans.put_data (orientation);

    }

    void get_active_isename (char* name) {
        int cmd;
        size_t datalen = 0;
        char* name_temp = NULL;

        m_trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_NAME);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (&name_temp, datalen) ) {
            strncpy (name, name_temp, strlen (name_temp));
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
        if (name_temp)
            delete [] name_temp;
    }

    void set_active_ise_by_name (const char* name) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_NAME);
        m_trans.put_data (name, strlen(name)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
    }

    void set_active_ise_by_uuid (const char* uuid) {
        int cmd;
        m_trans.put_command (ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID);
        m_trans.put_data (uuid, strlen(uuid)+1);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
        } else {
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }
    }

    void get_iselist (int* count, char*** iselist) {
        int cmd;
        uint32 count_temp = 0;
        char **buf = NULL;
        size_t len;
        char * buf_temp = NULL;

        m_trans.put_command (ISM_TRANS_CMD_GET_ISE_LIST);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK &&
                m_trans.get_data (count_temp) ) {
            *count = count_temp;
        } else {
            *count = 0;
            IMCONTROLERR ("%s:: get_command() or get_data() may fail!!!\n", __FUNCTION__);
        }

        if (count_temp > 0) {
            buf = (char**)malloc(count_temp * sizeof (char*));
            if (buf) {
                memset (buf, 0, count_temp*sizeof(char*));
                for (uint32 i = 0; i < count_temp; i++) {
                    if (m_trans.get_data (&buf_temp, len))
                        buf[i] = buf_temp;
                }
            }
        }
        *iselist = buf;
    }

    void reset_ise_option (void) {
        int cmd;

        m_trans.put_command (ISM_TRANS_CMD_RESET_ISE_OPTION);
        m_trans.write_to_socket (m_socket_imclient2panel);
        if (!m_trans.read_from_socket (m_socket_imclient2panel, m_socket_timeout))
            IMCONTROLERR ("%s:: read_from_socket() may be timeout \n", __FUNCTION__);

        if (m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_REPLY &&
                m_trans.get_command (cmd) && cmd == SCIM_TRANS_CMD_OK) {
            ;
        } else {
            IMCONTROLERR ("%s:: get_command() is failed!!!\n", __FUNCTION__);
        }
    }

    void set_caps_mode (int mode) {
        m_trans.put_command (ISM_TRANS_CMD_SET_CAPS_MODE);
        m_trans.put_data (mode);
    }
};

IMControlClient::IMControlClient ()
        : m_impl (new IMControlClientImpl ())
{
}

IMControlClient::~IMControlClient ()
{
    delete m_impl;
}

int
IMControlClient::open_connection ()
{
    return m_impl->open_connection ();
}

void
IMControlClient::close_connection ()
{
    m_impl->close_connection ();
}

bool IMControlClient::is_connected () const
{
    return m_impl->is_connected ();
}

int IMControlClient::get_panel2imclient_connection_number () const
{
    return m_impl->get_panel2imclient_connection_number ();
}

bool
IMControlClient::prepare ()
{
    return m_impl->prepare ();
}

bool
IMControlClient::send ()
{
    return m_impl->send ();
}

void IMControlClient::show_ise (void *data, int length)
{
    m_impl->show_ise (data,length);
}

void IMControlClient::hide_ise ()
{
    m_impl->hide_ise ();
}

void IMControlClient::show_control_panel ()
{
    m_impl->show_control_panel ();
}

void IMControlClient::hide_control_panel ()
{
    m_impl->hide_control_panel ();
}

void IMControlClient::set_mode (int mode)
{
    m_impl->set_mode (mode);
}

void IMControlClient::set_imdata (const char* data, int len)
{
    m_impl->set_imdata (data, len);
}

void IMControlClient::get_imdata (char* data, int* len)
{
    m_impl->get_imdata (data, len);
}

void IMControlClient::get_window_rect (int* x, int* y, int* width, int* height)
{
    m_impl->get_window_rect (x, y, width, height);
}

void IMControlClient::set_private_key (int layout_index, int key_index, const char *label, const char *value)
{
    m_impl->set_private_key (layout_index, key_index, label, value);
}

void IMControlClient::set_private_key_by_image (int layout_index, int key_index, const char *img_path, const char *value)
{
    m_impl->set_private_key_by_image (layout_index, key_index, img_path, value);
}

void IMControlClient::set_disable_key (int layout_index, int key_index, int disabled)
{
    m_impl->set_disable_key (layout_index, key_index, disabled);
}

void IMControlClient::set_layout (int layout)
{
    m_impl->set_layout (layout);
}

void IMControlClient::get_layout (int* layout)
{
    m_impl->get_layout (layout);
}

void IMControlClient::set_ise_language (int language)
{
    m_impl->set_ise_language (language);
}

void IMControlClient::reset ()
{
    m_impl->reset ();
}

void IMControlClient::set_screen_orientation (int orientation)
{
    m_impl->set_screen_orientation (orientation);
}

void IMControlClient::get_active_isename (char* name)
{
    m_impl->get_active_isename (name);
}

void IMControlClient::set_active_ise_by_name (const char* name)
{
    m_impl->set_active_ise_by_name (name);
}

void IMControlClient::set_active_ise_by_uuid (const char* uuid)
{
    m_impl->set_active_ise_by_uuid (uuid);
}

void IMControlClient::get_iselist (int* count, char*** iselist)
{
    m_impl->get_iselist (count, iselist);
}

void IMControlClient::reset_ise_option (void)
{
    m_impl->reset_ise_option ();
}

void IMControlClient::set_caps_mode (int mode)
{
    m_impl->set_caps_mode (mode);
}

};

/*
vi:ts=4:nowrap:ai:expandtab
*/
