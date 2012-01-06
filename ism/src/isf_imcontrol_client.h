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

#ifndef __ISF_IMCONTROL_CLIENT_H
#define __ISF_IMCONTROL_CLIENT_H

namespace scim
{

typedef Slot1<void, int> IMControlClientSlotVoid;


class IMControlClient
{
    class IMControlClientImpl;
    IMControlClientImpl *m_impl;

    IMControlClient (const IMControlClient &);
    const IMControlClient & operator = (const IMControlClient &);

public:
    IMControlClient ();
    ~IMControlClient ();

    int  open_connection        ();
    void close_connection       ();
    bool is_connected           () const;
    int  get_panel2imclient_connection_number  () const;
    bool prepare                ();
    bool send                   ();

    void show_ise (void *data, int length);
    void hide_ise ();
    void show_control_panel ();
    void hide_control_panel ();
    void set_mode (int mode);
    void set_lang (int lang);

    void set_imdata (const char* data, int len);
    void get_imdata (char* data, int* len);
    void get_window_rect (int* x, int* y, int* width, int* height);
    void set_private_key (int layout_index, int key_index, const char *label, const char *value);
    void set_private_key_by_image (int layout_index, int key_index, const char *img_path, const char *value);
    void set_disable_key (int layout_index, int key_index, int disabled);
    void set_layout (int layout);
    void get_layout (int* layout);
    void set_ise_language (int language);
    void reset ();
    void set_screen_orientation (int orientation);
    void get_active_isename (char* name);
    void set_active_ise_by_name (const char* name);
    void set_active_ise_by_uuid (const char* uuid);
    void get_iselist (int* count, char*** iselist);
    void reset_ise_option (void);
    void set_caps_mode (int mode);
};

}

#endif /* __ISF_IMCONTROL_CLIENT_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
