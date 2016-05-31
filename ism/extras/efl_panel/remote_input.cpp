/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Inhong Han <inhong1.han@samsung.com>
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

#include "remote_input.h"
#include <iostream>
#include <math.h>

using namespace scim;

extern std::vector<String>          _uuids;
extern std::vector<String>          _names;
extern std::vector<String>          _module_names;
extern std::vector<String>          _langs;
extern std::vector<String>          _icons;
extern std::vector<uint32>          _options;
extern std::vector<TOOLBAR_MODE_T>  _modes;

static InfoManager*      _info_manager;

char* surrounding_text = 0;
int cursor_pos = 0;

static WebSocketServer *g_web_socket_server = NULL;
Remote_Input* Remote_Input::m_instance = NULL;

static Motion_Input motion_input;

Remote_Input::Remote_Input()
{
    if (m_instance != NULL) {
        LOGD("WARNING : m_instance is NOT NULL");
    }
    m_instance = this;
    fd_uinput_keyboard = 0;
    fd_uinput_mouse = 0;
}

Remote_Input::~Remote_Input()
{
    if (m_instance == this) {
        m_instance = NULL;
    }
}

Remote_Input*
Remote_Input::get_instance()
{
    return m_instance;
}

void Remote_Input::init (InfoManager* info_manager)
{
    LOGD("Remote Input init");
    /* Create web socket server for remote input */
    g_web_socket_server = new WebSocketServer();
    if (g_web_socket_server) {
        g_web_socket_server->init();
    }

    /* Create uinput device for remote input */
    if (!init_uinput_keyboard_device())
    {
        LOGD ("Fail to create uinput device for keyboard");
    }
    if (!init_uinput_mouse_device())
    {
        LOGD ("Fail to create uinput device for mouse");
    }

    if (info_manager != NULL) {
        _info_manager = info_manager;
    } else {
        LOGW("Error panel_agent is NULL !!");
    }

}

void Remote_Input::exit()
{
    /* Delete web socket server */
    if (g_web_socket_server) delete g_web_socket_server;

    /* Close uinput device */
    if(ioctl(fd_uinput_keyboard, UI_DEV_DESTROY) < 0)
    {
        LOGD ( "error destroy\n");
    }
    close(fd_uinput_keyboard);

    if(ioctl(fd_uinput_mouse, UI_DEV_DESTROY) < 0)
    {
        LOGD ("error destroy\n");
    }
    close(fd_uinput_mouse);

}

bool Remote_Input::init_uinput_keyboard_device() {
    //For initialize uinput device for keyboard
     struct uinput_user_dev device_key;
     int uinput_keys[] = {KEY_POWER, KEY_F6, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_MINUS, KEY_0, KEY_REDO, KEY_F10, KEY_F9, KEY_F8, KEY_F7, KEY_F5, KEY_F12, KEY_F11, KEY_LEFTMETA, KEY_HOMEPAGE, KEY_BOOKMARKS, KEY_MENU, KEY_F18, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ENTER, KEY_BACK, KEY_EXIT, KEY_ESC, KEY_BACKSPACE, KEY_POWER, KEY_PHONE};
     memset(&device_key, 0, sizeof device_key);

     fd_uinput_keyboard = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
     if (fd_uinput_keyboard < 0)
     {
         LOGD ("Fail to open fd uinput_keyboard!\n");
         return false;
     }

     const char *dev_name = "Remote-Input(Keyboard)";
     strncpy(device_key.name, dev_name, 1 + strlen(dev_name));
     device_key.id.bustype = BUS_USB;
     device_key.id.vendor = 1;
     device_key.id.product = 1;
     device_key.id.version = 1;

     if (write(fd_uinput_keyboard, &device_key, sizeof(device_key)) != sizeof(device_key))
     {
         LOGD("Fail to setup uinput structure on fd\n");
         return false;
     }
     if (ioctl(fd_uinput_keyboard, UI_SET_EVBIT, EV_KEY) < 0)
     {
         LOGD("Fail to enable EV_KEY event type\n");
         return false;
     }
     if (ioctl(fd_uinput_keyboard, UI_SET_EVBIT, EV_SYN) < 0)
     {
         LOGD("Fail to enable EV_SYN event type\n");
         return false;
     }

     for (unsigned int i  = 0; i < sizeof(uinput_keys)/sizeof(uinput_keys[0]); i ++)
     {
         if (ioctl(fd_uinput_keyboard, UI_SET_KEYBIT, uinput_keys[i]) < 0)
         {
             LOGD("Fail to register uinput event key : %d\n", uinput_keys[i]);
             return false;
         }
     }
     if (ioctl(fd_uinput_keyboard, UI_DEV_CREATE) < 0)
     {
         LOGD("Fail to create keyboard uinput device\n");
         return false;
     }

    return true;
}

bool Remote_Input::init_uinput_mouse_device() {
    //For initialize uinput device for mouse

    struct uinput_user_dev device_mouse;
    int uinput_btns[] = {BTN_LEFT, BTN_RIGHT, BTN_MIDDLE};
    int uinput_rel_axes[] = {REL_X, REL_Y, REL_WHEEL};
    memset(&device_mouse, 0, sizeof device_mouse);

    //open the device node
    fd_uinput_mouse = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd_uinput_mouse < 0)
    {
        LOGD ("Fail to open fd uinput_mouse!\n");
        return false;
    }

    const char *dev_name = "Remote-Input(Mouse)";
    strncpy(device_mouse.name, dev_name, 1 + strlen(dev_name));
    device_mouse.id.bustype = BUS_USB;
    device_mouse.id.vendor = 1;
    device_mouse.id.product = 1;
    device_mouse.id.version = 1;

    if (write(fd_uinput_mouse, &device_mouse, sizeof(device_mouse)) != sizeof(device_mouse))
    {
        LOGD("Fail to setup uinput structure on fd\n");
        return false;
    }

    if (ioctl(fd_uinput_mouse, UI_SET_EVBIT, EV_KEY) < 0)
    {
        LOGD("Fail to enable EV_KEY event type\n");
        return false;
    }
    for (unsigned int i  = 0; i < sizeof(uinput_btns)/sizeof(uinput_btns[0]); i++)
    {
        if (ioctl(fd_uinput_mouse, UI_SET_KEYBIT, uinput_btns[i]) < 0)
        {
            LOGD("Fail to register uinput event key: %d\n", uinput_btns[i]);
            return false;
        }
    }

    if (ioctl(fd_uinput_mouse, UI_SET_EVBIT, EV_REL) < 0)
    {
        LOGD("Fail to enable EV_REL event type\n");
        return false;
    }
    for (unsigned int i  = 0; i < sizeof(uinput_rel_axes)/sizeof(uinput_rel_axes[0]); i++)
    {
        if (ioctl(fd_uinput_mouse, UI_SET_RELBIT, uinput_rel_axes[i]) < 0)
        {
            LOGD("Fail to register uinput event key: %d\n", uinput_rel_axes[i]);
            return false;
        }
    }
    if (ioctl(fd_uinput_mouse, UI_SET_EVBIT, EV_SYN) < 0)
    {
        LOGD("Fail to enable EV_SYN event type\n");
        return false;
    }
    if (ioctl(fd_uinput_mouse,UI_DEV_CREATE) < 0)
    {
        LOGD("Fail to create keyboard uinput device\n");
        return false;
    }

    return true;
}
void Remote_Input::send_uinput_event(UINPUT_DEVICE device, __u16 type, __u16 code, __s32 value)
{
    struct input_event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;

    if (device == UINPUT_KEYBOARD ){
        if (write(fd_uinput_keyboard, &event, sizeof(event)) != sizeof(event)) {
            LOGD ("Error to send uinput event");
        }
    }
    else if (device == UINPUT_MOUSE ){
        if (write(fd_uinput_mouse, &event, sizeof(event)) != sizeof(event)) {
            LOGD ("Error to send uinput event");
        }
    }
    else{
        LOGD ("Fail to send uinput event because of device name!");
    }

}

void Remote_Input::panel_send_uinput_event(UINPUT_DEVICE device, __u16 type, __u16 code, __s32 value)
{
    send_uinput_event(device, type, code, value);
}

void Remote_Input::panel_send_uinput_event_for_key(UINPUT_DEVICE device, __u16 code)
{
    panel_send_uinput_event (device, EV_KEY, code, 1);
    panel_send_uinput_event (device, EV_SYN, SYN_REPORT, 0);
    panel_send_uinput_event (device, EV_KEY, code, 0);
    panel_send_uinput_event (device, EV_SYN, SYN_REPORT, 0);

}

void Remote_Input::panel_send_uinput_event_for_touch_mouse(UINPUT_DEVICE device, __s32 value_x, __s32 value_y)
{
    struct timespec sleeptime = {0, 50};//speed (low value: fast, high value: slow)
    __s32 x, y, delta;

    //mouse accelerate
    delta = sqrt((value_x*value_x)+(value_y*value_y));

    if (delta > 10){
        x = (value_x*7);
        y = (value_y*7);
    }
    else if (delta > 7){
        x = (value_x*5);
        y = (value_y*5);
    }
    else if (delta > 3){
        x = (value_x*3);
        y = (value_y*3);
    }
    else {
        x = (value_x*2);
        y = (value_y*2);
    }
    //LOGD("touch Move : x: %d,  y: %d, delta :%d", x, y, delta);
    panel_send_uinput_event (device, EV_REL, REL_X, x);
    panel_send_uinput_event (device, EV_REL, REL_Y, y);
    panel_send_uinput_event (device, EV_SYN, SYN_REPORT, 0);
    nanosleep(&sleeptime, NULL);

}

void Remote_Input::panel_send_uinput_event_for_wheel(UINPUT_DEVICE device, __s32 value_y)
{
    struct timespec sleeptime = {0, 800000};
    __s32 y;

    y = value_y / 4;

    //LOGD("wheel value : %d, %d", value_y, y);
    panel_send_uinput_event (device, EV_REL, REL_WHEEL, y);
    panel_send_uinput_event (device, EV_SYN, SYN_REPORT, 0);
    nanosleep(&sleeptime, NULL);
}

void Remote_Input::panel_send_uinput_event_for_air_mouse(UINPUT_DEVICE device, double data[])
{
    struct timespec sleeptime = {0, 100};//speed (low value: fast, high value: slow)
    Point3Df AccData = Point3Df(data[0], data[1], data[2]);
    Point3Df GyrData = Point3Df(data[3], data[4], data[5]);
    Point2D Cursor_delta = Point2D(0,0);

    LOGD("1) Acc x: %f, y: %f, z: %f, Gyro x: %f, y:%f, z:%f", AccData.x, AccData.y, AccData.z, GyrData.x, GyrData.y, GyrData.z);
    motion_input.filter_raw_sensor_data(&AccData, &GyrData);
    LOGD("2) Acc x: %f, y: %f, z: %f, Gyro x: %f, y:%f, z:%f", AccData.x, AccData.y, AccData.z, GyrData.x, GyrData.y, GyrData.z);

    motion_input.calculate_cursor_delta_value(AccData, GyrData, &Cursor_delta);
    LOGD("3) Cursor_delta.x : %d, y: %d", Cursor_delta.x, Cursor_delta.y);

    panel_send_uinput_event (device, EV_REL, REL_X, Cursor_delta.x);
    panel_send_uinput_event (device, EV_REL, REL_Y, Cursor_delta.y);
    panel_send_uinput_event (device, EV_SYN, SYN_REPORT, 0);
    nanosleep (&sleeptime, NULL);
}

void Remote_Input::reset_setting_value_for_air_mouse(double data[])
{
    LOGD("ISE_AIR_SETTING acc :%f, gain: %f, smooth: %f, speed: %f, filter: %f ", data[0], data[1],data[2],data[3],data[4]);

    motion_input.Set_cursor_movement_acceleration(data[0]);
    motion_input.Set_cursor_movement_gain(data[1]);
    motion_input.Set_cursor_movement_smoothness((int)data[2]);
    motion_input.Set_cursor_movement_speed(data[3]);
    motion_input.Set_filter_number(data[4]);
}

void Remote_Input::handle_websocket_message(ISE_MESSAGE &message)
{
    LOGD("Received message : %s, %s, %s", message.type.c_str(), message.command.c_str() , message.values.at(0).c_str());

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_KEY_EVENT]) == 0) {
        if (message.values.size() == 1) {
            int e = atoi(message.values.at(0).c_str());
            LOGD("send_key_event key num : %d", e);
            switch (e) {
                case 0: //enter
                    LOGD ("enter");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Select));
                    break;
                case 1: //space
                    LOGD ("space");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_KP_Space));
                    break;
                case 2: //backspace
                    LOGD ("backspace");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_BackSpace));
                    break;
                case 3: //esc
                    LOGD ("esc");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Escape));
                    break;
                case 4: //up
                    LOGD ("up");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Up));
                    break;
                case 5: //down
                    LOGD ("down");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Down));
                    break;
                case 6: //left
                    LOGD ("left");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Left));
                    break;
                case 7: //right
                    LOGD ("right");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Right));
                    break;
                case 8: //page_up
                    LOGD ("page_up");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Page_Up));
                    break;
                case 9: //page_down
                    LOGD ("page_down");
                    _info_manager->remoteinput_forward_key_event(KeyEvent(SCIM_KEY_Page_Down));
                    break;
                case 10001://Menu
                    LOGD ("menu");
                    //panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_PHONE);
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_LEFTMETA); //for tv product binary
                    break;

                case 10002://Home
                    LOGD ("home");
                    //panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_MENU);
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F5); //for tv product binary
                    break;

                case 10003://Back
                    LOGD ("back");
                    //panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_BACK); //for TDC, 3.0 binary
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_ESC); //for tv product binary
                    break;

                case 124://TV_KEY_POWER
                    LOGD ("TV_KEY_POWER");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_POWER);
                    break;

                case 235://TV_KEY_SWITCHMODE
                    LOGD ("TV_KEY_SWITCHMODE");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F6);
                    break;

                case 179://TV_KEY_MENU
                    LOGD ("TV_KEY_SHORT_MENU");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_MENU);
                    break;

                case 111://TV_KEY_UP
                    LOGD ("TV_KEY_UP");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_UP);
                    break;

                case 69://TV_KEY_INFO
                    LOGD ("TV_KEY_INFO");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F18);
                    break;

                case 113://TV_KEY_LEFT
                    LOGD ("TV_KEY_LEFT");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_LEFT);
                    break;

                case 36://TV_KEY_SELECT
                    LOGD ("TV_KEY_SELECT");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_ENTER);
                    break;

                case 114://TV_KEY_RIGHT
                    LOGD ("TV_KEY_RIGHT");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_RIGHT);
                    break;

                case 166://TV_KEY_BACK
                    LOGD ("TV_KEY_BACK");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_BACK);
                    break;

                case 116://TV_KEY_DOWN
                    LOGD ("TV_KEY_DOWN");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_DOWN);
                    break;

                case 182://TV_KEY_EXIT
                    LOGD ("TV_KEY_EXIT");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_EXIT);
                    break;

                case 123://TV_KEY_VOL_UP
                    LOGD ("TV_KEY_VOL_UP");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F10);
                    break;

                case 121://TV_KEY_MUTE
                    LOGD ("TV_KEY_MUTE");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F8);
                    break;

                case 112://TV_KEY_CHAN_UP
                    LOGD ("TV_KEY_CHAN_UP");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F12);
                    break;

                case 122://TV_KEY_VOL_DOWN
                    LOGD ("TV_KEY_VOL_DOWN");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F9);
                    break;

                case 68://TV_KEY_CHAN_LIST
                    LOGD ("TV_KEY_CHAN_LIST");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F7);
                    break;

                case 117://TV_KEY_CHAN_DOWN
                    LOGD ("TV_KEY_CHAN_DOWN");
                    panel_send_uinput_event_for_key(UINPUT_KEYBOARD, KEY_F11);
                    break;

                default:
                    LOGD ("unknow key=%d", e);
                    break;
            }
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_LOG]) == 0) {
        if (message.values.size() == 1) {
            LOGD("Web_page LOG: %s", message.values.at(0).c_str());
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_COMMIT_STRING]) == 0) {
        if (message.values.size() == 1) {
            scim::AttributeList attrs;
            attrs.push_back(scim::Attribute(0, scim::utf8_mbstowcs((char*)message.values.at(0).c_str()).length(), scim::SCIM_ATTR_DECORATE, scim::SCIM_ATTR_DECORATE_UNDERLINE));

            LOGD( "commit_str:|%s|", message.values.at(0).c_str());
            _info_manager->remoteinput_commit_string(scim::utf8_mbstowcs((char*)message.values.at(0).c_str()));
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_UPDATE_PREEDIT_STRING]) == 0) {
        if (message.values.size() == 1) {
            scim::AttributeList attrs;
            attrs.push_back(scim::Attribute(0, scim::utf8_mbstowcs((char*)message.values.at(0).c_str()).length(), scim::SCIM_ATTR_DECORATE, scim::SCIM_ATTR_DECORATE_UNDERLINE));

            LOGD ("preedit:|%s|", message.values.at(0).c_str());
            _info_manager->remoteinput_update_preedit_string(scim::utf8_mbstowcs((char*)message.values.at(0).c_str()), attrs, -1);
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_MOUSE_KEY]) == 0) {
        if (message.values.size() == 1) {

            int e = atoi(message.values.at(0).c_str());
            if (e == 555) {
                panel_send_uinput_event_for_key(UINPUT_MOUSE, BTN_LEFT);
            }
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_MOUSE_MOVE]) == 0) {
        if (message.values.size() == 1) {

            std::size_t tmp_offset = message.values.at(0).find(',');
            int x = atoi(message.values.at(0).substr(0, tmp_offset).c_str());
            int y = atoi(message.values.at(0).substr(tmp_offset+1).c_str());
            panel_send_uinput_event_for_touch_mouse(UINPUT_MOUSE, (__s32)x, (__s32)y);
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_WHEEL_MOVE]) == 0) {
        if (message.values.size() == 1) {

            std::size_t tmp_offset = message.values.at(0).find(',');
            //int x = atoi(message.values.at(0).substr(0, tmp_offset).c_str());
            int y = atoi(message.values.at(0).substr(tmp_offset+1).c_str());
            panel_send_uinput_event_for_wheel(UINPUT_MOUSE, (__s32)y);
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_AIR_SETTING]) == 0) {
        if (message.values.size() == 1) {

            double set_data[5] = {0};
            char *pch;
            char *ptr[2];
            char *string = (char *)message.values.at(0).c_str();
            int count=0;

            pch = strtok_r (string, ",", &ptr[0]);
            while (pch !=NULL){
                set_data[count++]=atof(pch);
                pch = strtok_r (NULL, ",", &ptr[1]);
            }

            LOGD( "2. SETTING data : %f, %f, %f, %f, %f" ,set_data[0],set_data[1],set_data[2],set_data[3],set_data[4] );
            reset_setting_value_for_air_mouse(set_data);
        }
    }

    if (message.command.compare(ISE_MESSAGE_COMMAND_STRINGS[ISE_MESSAGE_COMMAND_SEND_AIR_INPUT]) == 0) {

        if (message.values.size() == 1) {

            double dataBuf[6] = {0};
            char *pch;
            char *ptr[2];
            char *string = (char *)message.values.at(0).c_str();
            int count=0;

            pch = strtok_r (string, ",", &ptr[0]);
            while (pch !=NULL){
                dataBuf[count++]=atof(pch);
                pch = strtok_r (NULL, ",", &ptr[1]);
            }
            //LOGD( "2. CV Air Input data : %f, %f, %f, %f, %f, %f" ,dataBuf[0],dataBuf[1],dataBuf[2],dataBuf[3],dataBuf[4],dataBuf[5] );
            panel_send_uinput_event_for_air_mouse(UINPUT_MOUSE, dataBuf);
        }
    }
}

void Remote_Input::handle_recv_panel_message(int mode, const char* text, int cursor)
{
    switch (mode) {
        case 1:
            break;
        case 2:
            break;
        case 3:
        {
            if (cursor_pos != cursor || strcmp (surrounding_text, text) != 0) {
                cursor_pos = cursor;
                surrounding_text = strdup(text);
                g_web_socket_server->get_surrounding_text(cursor, text);
            }
            break;
        }
        default:
            break;
    }
}



