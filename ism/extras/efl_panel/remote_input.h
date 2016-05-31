/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>
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

#ifndef __REMOTE_INPUT_H__
#define __REMOTE_INPUT_H__

#define Uses_SCIM_PANEL_AGENT


#include <dlog.h>

#include <stdio.h>
#include <stdlib.h>

//for socket
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/input.h>
#include <linux/uinput.h>

#include "scim.h"
#include "motion_input.h"
#include "websocketserver.h"

using namespace scim;

enum UINPUT_DEVICE{
    UINPUT_KEYBOARD = 0,
    UINPUT_MOUSE
};

class Remote_Input
{
public :

private:
    static Remote_Input*    m_instance;
    int fd_uinput_keyboard;
    int fd_uinput_mouse;

public:
    Remote_Input();
    ~Remote_Input();

    static Remote_Input* get_instance();

    void init(InfoManager* info_manager);

    void exit();

    bool init_uinput_keyboard_device();

    bool init_uinput_mouse_device();

    void send_uinput_event(UINPUT_DEVICE device, __u16 type, __u16 code, __s32 value);

    void panel_send_uinput_event(UINPUT_DEVICE device, __u16 type, __u16 code, __s32 value);

    void panel_send_uinput_event_for_key(UINPUT_DEVICE device, __u16 code);

    void panel_send_uinput_event_for_air_mouse(UINPUT_DEVICE device, double data[]);

    void panel_send_uinput_event_for_wheel(UINPUT_DEVICE device, __s32 value_y);

    void panel_send_uinput_event_for_touch_mouse(UINPUT_DEVICE device, __s32 value_x, __s32 value_y);

    void reset_setting_value_for_air_mouse(double data[]);

    void handle_websocket_message(ISE_MESSAGE &message);

    void handle_recv_panel_message(int mode, const char* text, int cursor);
};

#endif /* __REMOTE_INPUT_H__ */
