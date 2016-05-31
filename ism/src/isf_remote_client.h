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

#ifndef __ISF_REMOTE_CLIENT_H
#define __ISF_REMOTE_CLIENT_H

namespace scim
{

typedef enum {
    REMOTE_CONTROL_CALLBACK_ERROR,
    REMOTE_CONTROL_CALLBACK_FOCUS_IN,
    REMOTE_CONTROL_CALLBACK_FOCUS_OUT,
    REMOTE_CONTROL_CALLBACK_ENTRY_METADATA,
    REMOTE_CONTROL_CALLBACK_DEFAULT_TEXT,
} remote_control_callback_type;

class EXAPI RemoteInputClient
{
    class RemoteInputClientImpl;
    RemoteInputClientImpl *m_impl;

public:
    RemoteInputClient ();
    ~RemoteInputClient ();

    int  open_connection        (void);
    void close_connection       (void);
    bool is_connected           (void) const;
    int  get_panel2remote_connection_number (void) const;
    bool prepare                (void);
    bool has_pending_event      (void) const;
    bool send_remote_input_message (const char* str);    
    remote_control_callback_type recv_callback_message (void);
    void get_entry_metadata (int *hint, int *layout, int *variation, int *autocapital_type);
    void get_default_text (String &default_text, int *cursor);
};
}

#endif /* __ISF_REMOTE_CLIENT_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/