/*
 * Copyright 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Ecore_X.h>
#include <Elementary.h>

#include <dlog.h>

#include <vconf.h>
#include <vconf-keys.h>

#include "common.h"
#include "ise.h"

using namespace scl;

CISECommon* CISECommon::m_instance = NULL; /* For singleton */

extern KEYBOARD_STATE g_keyboard_state;

/* Internal signal handler function */
void signal_handler(int sig);

/* Internal input handler function */
Eina_Bool input_handler (void *data, Ecore_Fd_Handler *fd_handler);

static int get_app_window_degree(Evas_Object *keypad_win)
{
    int  ret = 0;
    Atom type_return;
    int  format_return;
    unsigned long    nitems_return;
    unsigned long    bytes_after_return;
    unsigned char   *data_window = NULL;
    unsigned char   *data_angle = NULL;

    int retVal = 0;
    Ecore_X_Window app_window = 0;

    Ecore_X_Window win = elm_win_xwindow_get(static_cast<Evas_Object*>(keypad_win));
    ret = XGetWindowProperty((Display *)ecore_x_display_get (),
        ecore_x_window_root_get(win),
        ecore_x_atom_get("_ISF_ACTIVE_WINDOW"),
        0, G_MAXLONG, False, XA_WINDOW, &type_return,
        &format_return, &nitems_return, &bytes_after_return,
        &data_window);

    if (ret == Success) {
        if ((type_return == XA_WINDOW) && (format_return == 32) && (data_window)) {
            app_window = *(Window *)data_window;

            ret = XGetWindowProperty((Display *)ecore_x_display_get(), app_window,
                ecore_x_atom_get("_E_ILLUME_ROTATE_WINDOW_ANGLE"),
                0, G_MAXLONG, False, XA_CARDINAL, &type_return,
                &format_return, &nitems_return, &bytes_after_return,
                &data_angle);

            if (ret == Success) {
                if (data_angle) {
                    if (type_return == XA_CARDINAL) {
                        retVal = *(unsigned int*)data_angle;
                        LOGD("current rotation angle is %p %d\n", app_window, retVal);
                    }
                    XFree(data_angle);
                }
            }
        }
        if (data_window)
            XFree(data_window);
    }


    return retVal;
}

const char * extract_themename_from_theme_file_path(const char *filepath) {
    static char themename[_POSIX_PATH_MAX] = {0};
    memset(themename, 0x00, sizeof(themename));

    if (filepath) {
        /* There could be more than 1 theme filepath, separated by : */
        char pathstr[_POSIX_PATH_MAX] = {0};
        strncpy(pathstr, filepath, _POSIX_PATH_MAX - 1);
        for(int loop = 0;loop < _POSIX_PATH_MAX;loop++) {
            if (pathstr[loop] == ':') {
                /* FIXME : Let's consider the 1st theme filepath only for now */
                pathstr[loop] = '\0';
            }
        }

        if (pathstr[0]) {
            const char *filename = ecore_file_file_get(pathstr);
            if (filename) {
                char *stripname = ecore_file_strip_ext(filename);
                if (stripname) {
                    strncpy(themename, stripname, _POSIX_PATH_MAX - 1);
                    free(stripname);
                }
            }
        }
    }

    return themename;
}

void language_changed_cb(keynode_t *key, void* data)
{
    char clang[_POSIX_PATH_MAX] = {0};
    char *vconf_str = vconf_get_str(VCONFKEY_LANGSET);
    if (vconf_str) {
        snprintf(clang, sizeof(clang), "%s",vconf_str);
        free(vconf_str);
    }
    LOGD("current language is %s\n",clang);

    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_display_language(clang);
        }
    }
}

void theme_changed_cb(keynode_t *key, void* data)
{
    char clang[256] = {0};
    char *vconf_str = vconf_get_str(VCONFKEY_SETAPPL_WIDGET_THEME_STR);
    if (vconf_str) {
        snprintf(clang, sizeof(clang), "%s",extract_themename_from_theme_file_path(vconf_str));
        free(vconf_str);
    }
    LOGD("current theme is %s\n",clang);

    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_theme_name(clang);
        }
    }
}

void accessibility_changed_cb(keynode_t *key, void* data)
{
    int vconf_value = 0;
    if(vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &vconf_value) == 0) {
        LOGD("accessbility state : %d\n", vconf_value);

        CISECommon *impl = CISECommon::get_instance();
        if (impl) {
            IISECommonEventCallback *callback = impl->get_core_event_callback();
            if (callback) {
                callback->set_accessibility_state(vconf_value);
            }
        }
    }
}

static Eina_Bool _client_message_cb (void *data, int type, void *event)
{
    Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *)event;
    int angle;

    IISECommonEventCallback *callback = NULL;
    CISECommon *common = CISECommon::get_instance();
    Evas_Object *main_window = NULL;
    if (common) {
        callback = common->get_core_event_callback();
        main_window = common->get_main_window();
    }

#ifndef APPLY_WINDOW_MANAGER_CHANGE
#else
    if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE) {
        LOGD("ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE , %d %d\n", ev->data.l[0], gFHiddenState);
        angle = ev->data.l[0];
        ise_set_screen_direction(angle);
        if (!gFHiddenState) {
            ise_show(gLastIC);
        }
    } else if (ev->message_type == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) {
        LOGD("ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE , %d\n", ev->data.l[0]);
        elm_win_keyboard_mode_set(main_window, (Elm_Win_Keyboard_Mode)(ev->data.l[0]));
        gFHiddenState = !(ev->data.l[0]);
    }
#endif

    if (ev->message_type == ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST) {
        if (ev->win == elm_win_xwindow_get(main_window)) {
            int angle = ev->data.l[1];
            LOGD("_ECORE_X_ATOM_E_WINDOW_ROTATION_REQUEST, %d\n", angle);
            if (callback) {
                callback->set_rotation_degree(angle);
            }
            Ecore_X_Window control_window = 0;
            Ecore_X_Atom atom = ecore_x_atom_get ("_ISF_CONTROL_WINDOW");
            Ecore_X_Window root = ecore_x_window_root_first_get ();
            if (ecore_x_window_prop_xid_get(root, atom, ECORE_X_ATOM_WINDOW, &control_window, 1) == 1) {
                ecore_x_client_message32_send(control_window, ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_REQUEST,
                    ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
                    ev->data.l[0], ev->data.l[1], ev->data.l[2], ev->data.l[3], ev->data.l[4]);
            }
        }
    }

    return ECORE_CALLBACK_RENEW;
}

CISECommon::CISECommon()
{
    m_main_window = NULL;
    m_event_callback = NULL;
}

CISECommon::~CISECommon()
{
}

CISECommon*
CISECommon::get_instance()
{
    if (!m_instance) {
        m_instance = new CISECommon();
    }
    return (CISECommon*)m_instance;
}

void CISECommon::init(const sclchar *name, const sclchar *uuid, const sclchar *language)
{
    m_helper_info.uuid = scim::String(uuid);
    m_helper_info.name = scim::String(name);
    m_helper_info.option = scim::SCIM_HELPER_STAND_ALONE | scim::SCIM_HELPER_NEED_SCREEN_INFO |
        scim::SCIM_HELPER_NEED_SPOT_LOCATION_INFO | scim::SCIM_HELPER_AUTO_RESTART;

    m_supported_language = scim::String(language);
}

void CISECommon::exit()
{
}

void CISECommon::run(const sclchar *uuid, const scim::ConfigPointer &config, const sclchar *display)
{
    char *argv[4];
    int argc = 3;

    argv [0] = const_cast<char *> (m_helper_info.name.c_str());
    argv [1] = (char *)"--display";
    argv [2] = const_cast<char *> (display);
    argv [3] = 0;

    m_config = config;

    elm_init(argc, argv);

    m_main_window = elm_win_add(NULL, "Tizen Keyboard", ELM_WIN_UTILITY);

    //elm_win_alpha_set(m_main_window, EINA_TRUE);
    elm_win_borderless_set(m_main_window, EINA_TRUE);
    elm_win_keyboard_win_set(m_main_window, EINA_TRUE);
    elm_win_autodel_set(m_main_window, EINA_TRUE);
    elm_win_title_set(m_main_window, "Tizen Keyboard");

    unsigned int set = 1;
    ecore_x_window_prop_card32_set(elm_win_xwindow_get(m_main_window),
        ECORE_X_ATOM_E_WINDOW_ROTATION_SUPPORTED,
        &set, 1);

#ifdef FULL_SCREEN_TEST
    elm_win_fullscreen_set(m_main_window, EINA_TRUE);
#endif

    /*Evas_Object *box = elm_box_add(m_main_window);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_win_resize_object_add(m_main_window, box);*/

    if (m_event_callback) {
        m_event_callback->init();
    }

    vconf_notify_key_changed(VCONFKEY_LANGSET, language_changed_cb, NULL);
    vconf_notify_key_changed(VCONFKEY_SETAPPL_WIDGET_THEME_STR, theme_changed_cb, NULL);
    vconf_notify_key_changed(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, accessibility_changed_cb, NULL);

    /* Should we call these callback functions here? */
    language_changed_cb(NULL, NULL);
    theme_changed_cb(NULL, NULL);
    accessibility_changed_cb(NULL, NULL);

    register_slot_functions();

    m_helper_agent.open_connection (m_helper_info, display);
    int fd = m_helper_agent.get_connection_number();

    if (fd >= 0) {
        Ecore_X_Window xwindow = elm_win_xwindow_get(m_main_window);
        char xid[255];
        snprintf(xid, 255, "%d", xwindow);
        scim::Property prop (xid, "XID", "", "");
        scim::PropertyList props;
        props.push_back (prop);
        m_helper_agent.register_properties (props);

        ecore_main_fd_handler_add(fd, ECORE_FD_READ, input_handler, NULL, NULL, NULL);
    }

    Ecore_Event_Handler *XClientMsgHandler = ecore_event_handler_add (ECORE_X_EVENT_CLIENT_MESSAGE, _client_message_cb, m_main_window);

    signal (SIGQUIT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGINT,  signal_handler);
    signal (SIGHUP,  signal_handler);

    elm_run();

    vconf_ignore_key_changed (VCONFKEY_LANGSET, language_changed_cb);
    vconf_ignore_key_changed (VCONFKEY_SETAPPL_WIDGET_THEME_STR, theme_changed_cb);
    vconf_ignore_key_changed (VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, accessibility_changed_cb);

    ecore_event_handler_del (XClientMsgHandler);

    /* Would the below code needed? */
    /*
    if (m_event_callback) {
        m_event_callback->exit();
    }
    */

    elm_shutdown();

}

scim::HelperAgent* CISECommon::get_helper_agent()
{
    return &m_helper_agent;
}

scluint CISECommon::get_number_of_helpers(void)
{
    return 1;
}

sclboolean CISECommon::get_helper_info(scluint idx, scim::HelperInfo &info)
{
    info = m_helper_info;
    return TRUE;
}

scim::String CISECommon::get_helper_language(scluint idx)
{
    /* FIXME : We should return appropriate language locale string */
    std::vector<scim::String> langlist;
    langlist.push_back(m_supported_language);
    return scim::scim_combine_string_list(langlist);
}

void CISECommon::set_core_event_callback(IISECommonEventCallback *callback)
{
    m_event_callback = callback;
}

Evas_Object* CISECommon::get_main_window()
{
    return m_main_window;
}

void CISECommon::set_keyboard_size_hints(SclSize portrait, SclSize landscape)
{
    /* Temporary code, this should be automatically calculated when changing input mode */
    ecore_x_e_window_rotation_geometry_set(elm_win_xwindow_get(m_main_window),   0, 0, 0, portrait.width, portrait.height);
    ecore_x_e_window_rotation_geometry_set(elm_win_xwindow_get(m_main_window),  90, 0, 0, landscape.height, landscape.width);
    ecore_x_e_window_rotation_geometry_set(elm_win_xwindow_get(m_main_window), 180, 0, 0, portrait.width, portrait.height);
    ecore_x_e_window_rotation_geometry_set(elm_win_xwindow_get(m_main_window), 270, 0, 0, landscape.height, landscape.width);
}

scim::String CISECommon::get_keyboard_ise_uuid()
{
    return m_uuid_keyboard_ise;
}

IISECommonEventCallback* CISECommon::get_core_event_callback()
{
    return m_event_callback;
}

void CISECommon::reload_config()
{
    m_helper_agent.reload_config();
}

void CISECommon::send_imengine_event(sclint ic, const sclchar *ic_uuid, const sclint command, const sclu32 value)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    scim::Transaction trans;
    trans.put_command(command);
    trans.put_data (value);
    m_helper_agent.send_imengine_event(ic, uuid, trans);
}

void CISECommon::reset_keyboard_ise() {
    m_helper_agent.reset_keyboard_ise();
}

void CISECommon::send_key_event(sclint ic, const sclchar *ic_uuid, sclu32 keycode, sclu16 keymask)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    scim::KeyEvent event;
    event.code = keycode;
    event.mask = keymask;
    m_helper_agent.send_key_event(ic, uuid, event);
}

void CISECommon::forward_key_event(sclint ic, const sclchar *ic_uuid, sclu32 keycode, sclu16 keymask)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    scim::KeyEvent event;
    event.code = keycode;
    event.mask = keymask;
    m_helper_agent.forward_key_event(ic, uuid, event);
}

void CISECommon::commit_string(sclint ic, const sclchar *ic_uuid, const sclchar *str)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    m_helper_agent.commit_string(ic, uuid, scim::utf8_mbstowcs(str));
}

void CISECommon::show_preedit_string(sclint ic, const sclchar *ic_uuid)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    m_helper_agent.show_preedit_string(ic, uuid);
}

void CISECommon::show_aux_string(void)
{
    m_helper_agent.show_aux_string();
}

void CISECommon::show_candidate_string(void)
{
    m_helper_agent.show_candidate_string();
}

void CISECommon::show_associate_string(void)
{
    m_helper_agent.show_associate_string();
}

void CISECommon::hide_preedit_string(sclint ic, const sclchar *ic_uuid)
{
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    m_helper_agent.hide_preedit_string(ic, uuid);
}

void CISECommon::hide_aux_string(void)
{
    m_helper_agent.hide_aux_string();
}

void CISECommon::hide_candidate_string(void)
{
    m_helper_agent.hide_candidate_string();
}

void CISECommon::hide_associate_string(void)
{
    m_helper_agent.hide_associate_string();
}

void CISECommon::update_preedit_string(sclint ic, const sclchar *ic_uuid, const sclchar *str)
{
    scim::AttributeList list;
    scim::String uuid;
    if (ic_uuid) {
        uuid = scim::String(ic_uuid);
    }
    m_helper_agent.update_preedit_string(ic, uuid, scim::utf8_mbstowcs(str), list);

    if (str && strlen(str) > 0) {
        show_preedit_string(ic, ic_uuid);
    } else {
        hide_preedit_string(ic, ic_uuid);
    }
}

void CISECommon::update_aux_string(const sclchar *str)
{
    scim::AttributeList list;
    m_helper_agent.update_aux_string(scim::String(str), list);
}

void CISECommon::update_input_context(sclu32 type, sclu32 value)
{
    m_helper_agent.update_input_context(type, value);
}

void CISECommon::set_candidate_position(sclint left, sclint top)
{
    m_helper_agent.set_candidate_position(left, top);
}

void CISECommon::candidate_hide(void)
{
    m_helper_agent.candidate_hide();
}

void CISECommon::set_keyboard_ise_by_uuid(const sclchar *uuid)
{
    if (uuid) {
        m_uuid_keyboard_ise = scim::String(uuid);
        if (g_keyboard_state.ic == g_keyboard_state.focused_ic) {
            LOGD("Calling helper_agent.set_keyboard_ise_by_uuid() : %s", uuid);
            m_helper_agent.set_keyboard_ise_by_uuid(m_uuid_keyboard_ise);
        }
    }
}

void CISECommon::get_keyboard_ise(const sclchar *uuid)
{
    m_helper_agent.get_keyboard_ise(scim::String(uuid));
}

/* Slot functions for calling appropriate callback functions */
void slot_exit (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->ise_hide(ic, ic_uuid.c_str());
            callback->exit(ic, ic_uuid.c_str());
        }
        scim::HelperAgent *helper_agent = impl->get_helper_agent();
        if (helper_agent) {
            helper_agent->update_ise_exit();
            helper_agent->close_connection();
        }
    }
    elm_exit ();
}

void slot_attach_input_context (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->attach_input_context(ic, ic_uuid.c_str());
        }
    }
}

void slot_detach_input_context (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->detach_input_context(ic, ic_uuid.c_str());
        }
    }
}

void slot_reload_config (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->reload_config(ic, ic_uuid.c_str());
        }
    }
}

void slot_update_screen (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid, int screen_number) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            //callback->update_screen(ic, ic_uuid.c_str(), screen_number);
        }
    }
}

void slot_update_spot_location (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid, int x, int y) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_spot_location(ic, ic_uuid.c_str(), x, y);
        }
    }
}

void slot_update_cursor_position (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid, int cursor_pos) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_cursor_position(ic, ic_uuid.c_str(), cursor_pos);
        }
    }
}

void slot_update_surrounding_text (const scim::HelperAgent *agent, int ic, const scim::String &text, int cursor) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_surrounding_text(ic, text.c_str(), cursor);
        }
    }
}

void slot_trigger_property (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid, const scim::String &property) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            //callback->trigger_property(ic, ic_uuid.c_str(), property.c_str());
        }
    }
}

//void slot_process_imengine_event (const HelperAgent *agent, int ic,
//                                  const String &ic_uuid, const Transaction transaction) {
//    CISECommon *impl = CISECommon::get_instance();
//    if (impl) {
//        IISECommonEventCallback *callback = impl->get_core_event_callback();
//        if (callback) {
//            callback->
//        }
//    }
//}

void slot_focus_out (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->focus_out(ic, ic_uuid.c_str());
        }
    }
}

void slot_focus_in (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->focus_in(ic, ic_uuid.c_str());
        }
    }
}

void slot_ise_show (const scim::HelperAgent *agent, int ic, char *buf, size_t &len) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        /* Make sure the appropriate keyboard ise was selected -> is this really necessary? */
        //impl->set_keyboard_ise_by_uuid(impl->get_keyboard_ise_uuid().c_str());

        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            /* Check if effect is enabled */
            Ise_Context ise_context;
            memset(&ise_context, 0x00, sizeof(ise_context));

            if (len >= sizeof(Ise_Context)) {
                memcpy(&ise_context, buf, sizeof(ise_context));
            } else {
                LOGD("\n-=-= WARNING - buf %p len %d size %d \n", buf, len, sizeof(ise_context));
            }
            callback->ise_show(ic, get_app_window_degree(impl->get_main_window()), ise_context);
        }
    }
}

void slot_ise_hide (const scim::HelperAgent *agent, int ic, const scim::String &ic_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->ise_hide(ic, ic_uuid.c_str());
        }
    }
}

void slot_get_geometry (const scim::HelperAgent *agent, struct scim::rectinfo &info) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_geometry(&(info.pos_x), &(info.pos_y), &(info.width), &(info.height));
        }
    }
}

void slot_set_mode (const scim::HelperAgent *agent, scim::uint32 &mode) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_mode(mode);
        }
    }
}

void slot_set_language (const scim::HelperAgent *agent, scim::uint32 &language) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_language(language);
        }
    }
}

void slot_set_imdata (const scim::HelperAgent *agent, char *buf, size_t &len) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_imdata(buf, len);
        }
    }
}

void slot_get_imdata (const scim::HelperAgent *, char **buf, size_t &len) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_imdata(buf, &len);
        }
    }
}

void slot_get_language_locale (const scim::HelperAgent *, int ic, char **locale) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_language_locale(ic, locale);
        }
    }
}

void slot_set_return_key_type (const scim::HelperAgent *agent, scim::uint32 &type) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_return_key_type(type);
        }
    }
}

void slot_get_return_key_type (const scim::HelperAgent *agent, scim::uint32 &type) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_return_key_type(&type);
        }
    }
}

void slot_set_return_key_disable (const scim::HelperAgent *agent, scim::uint32 &disabled) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_return_key_disable(disabled);
        }
    }
}

void slot_get_return_key_disable (const scim::HelperAgent *agent, scim::uint32 &disabled) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_return_key_disable(&disabled);
        }
    }
}

void slot_set_layout (const scim::HelperAgent *agent, scim::uint32 &layout) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_layout(layout);
        }
    }
}

void slot_get_layout (const scim::HelperAgent *agent, scim::uint32 &layout) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->get_layout(&layout);
        }
    }
}

void slot_set_caps_mode (const scim::HelperAgent *agent, scim::uint32 &mode) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->set_caps_mode(mode);
        }
    }
}

void slot_reset_input_context (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->reset_input_context(ic, uuid.c_str());
        }
    }
}

void slot_update_candidate_geometry (const scim::HelperAgent *agent, int ic, const scim::String &uuid, const scim::rectinfo &info) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_candidate_geometry(ic, uuid.c_str(), info.pos_x, info.pos_y, info.width, info.height);
        }
    }
}
void slot_update_keyboard_ise (const scim::HelperAgent *agent, int ic, const scim::String &uuid,
                               const scim::String &ise_name, const scim::String &ise_uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_keyboard_ise(ic, uuid.c_str(), ise_name.c_str(), ise_uuid.c_str());
        }
    }
}

//void slot_update_keyboard_ise_list (const HelperAgent *agent, int ic, const String &uuid,
//                                    const std::vector<String> &ise_list) {
//    CISECommon *impl = CISECommon::get_instance();
//    if (impl) {
//        IISECommonEventCallback *callback = impl->get_core_event_callback();
//        if (callback) {
//            callback->get_layout(&layout);
//        }
//    }
//}

void slot_candidate_more_window_show (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->candidate_more_window_show(ic, uuid.c_str());
        }
    }
}

void slot_candidate_more_window_hide (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->candidate_more_window_hide(ic, uuid.c_str());
        }
    }
}

void slot_select_aux (const scim::HelperAgent *agent, int ic, const scim::String &uuid, int index) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->select_aux(ic, uuid.c_str(), index);
        }
    }
}

void slot_select_candidate (const scim::HelperAgent *agent, int ic, const scim::String &uuid, int index) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->select_candidate(ic, uuid.c_str(), index);
        }
    }
}

void slot_candidate_table_page_up (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->candidate_table_page_up(ic, uuid.c_str());
        }
    }
}

void slot_candidate_table_page_down (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->candidate_table_page_down(ic, uuid.c_str());
        }
    }
}

void slot_update_candidate_table_page_size (const scim::HelperAgent *, int ic, const scim::String &uuid, int page_size) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_candidate_table_page_size(ic, uuid.c_str(), page_size);
        }
    }
}

void slot_select_associate (const scim::HelperAgent *agent, int ic, const scim::String &uuid, int index) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->select_associate(ic, uuid.c_str(), index);
        }
    }
}

void slot_associate_table_page_up (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->associate_table_page_up(ic, uuid.c_str());
        }
    }
}

void slot_associate_table_page_down (const scim::HelperAgent *agent, int ic, const scim::String &uuid) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->associate_table_page_down(ic, uuid.c_str());
        }
    }
}

void slot_update_associate_table_page_size (const scim::HelperAgent *, int ic, const scim::String &uuid, int page_size) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->update_associate_table_page_size(ic, uuid.c_str(), page_size);
        }
    }
}

void slot_turn_on_log (const scim::HelperAgent *agent, scim::uint32 &on) {
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        IISECommonEventCallback *callback = impl->get_core_event_callback();
        if (callback) {
            callback->turn_on_log(on);
        }
    }
}

/* Internal signal handler function */
void signal_handler(int sig) {
    elm_exit();
}

/* Internal input handler function */
Eina_Bool input_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    CISECommon *impl = CISECommon::get_instance();
    if (impl) {
        scim::HelperAgent *agent = impl->get_helper_agent();
        if (agent) {
            if (agent->has_pending_event()) {
                if (!(agent->filter_event())) {
                    LOGD("helper_agent.filter_event() failed!!!\n");
                    elm_exit();
                }
            } else {
                LOGD("helper_agent.has_pending_event() failed!!!\n");
                elm_exit();
            }
        }
    }

    return ECORE_CALLBACK_RENEW;
}

void CISECommon::register_slot_functions()
{
    m_helper_agent.signal_connect_exit (scim::slot (slot_exit));
    m_helper_agent.signal_connect_attach_input_context (scim::slot (slot_attach_input_context));
    m_helper_agent.signal_connect_detach_input_context (scim::slot (slot_detach_input_context));
    m_helper_agent.signal_connect_reload_config (scim::slot (slot_reload_config));
    m_helper_agent.signal_connect_update_screen (scim::slot (slot_update_screen));
    m_helper_agent.signal_connect_update_spot_location (scim::slot (slot_update_spot_location));
    m_helper_agent.signal_connect_update_cursor_position (scim::slot (slot_update_cursor_position));
    m_helper_agent.signal_connect_update_surrounding_text (scim::slot (slot_update_surrounding_text));
    m_helper_agent.signal_connect_trigger_property (scim::slot (slot_trigger_property));
    //m_helper_agent.signal_connect_process_imengine_event (slot (slot_process_imengine_event));
    m_helper_agent.signal_connect_focus_out (scim::slot (slot_focus_out));
    m_helper_agent.signal_connect_focus_in (scim::slot (slot_focus_in));
    m_helper_agent.signal_connect_ise_show (scim::slot (slot_ise_show));
    m_helper_agent.signal_connect_ise_hide (scim::slot (slot_ise_hide));
    m_helper_agent.signal_connect_get_geometry (scim::slot (slot_get_geometry));
    m_helper_agent.signal_connect_set_mode (scim::slot (slot_set_mode));
    m_helper_agent.signal_connect_set_language (scim::slot (slot_set_language));
    m_helper_agent.signal_connect_set_imdata (scim::slot (slot_set_imdata));
    m_helper_agent.signal_connect_get_imdata (scim::slot (slot_get_imdata));
    m_helper_agent.signal_connect_get_language_locale (scim::slot (slot_get_language_locale));
    m_helper_agent.signal_connect_set_return_key_type (scim::slot (slot_set_return_key_type));
    m_helper_agent.signal_connect_get_return_key_type (scim::slot (slot_get_return_key_type));
    m_helper_agent.signal_connect_set_return_key_disable (scim::slot (slot_set_return_key_disable));
    m_helper_agent.signal_connect_get_return_key_disable (scim::slot (slot_get_return_key_disable));
    m_helper_agent.signal_connect_get_layout (scim::slot (slot_get_layout));
    m_helper_agent.signal_connect_set_layout (scim::slot (slot_set_layout));
    m_helper_agent.signal_connect_set_caps_mode (scim::slot (slot_set_caps_mode));
    m_helper_agent.signal_connect_reset_input_context (scim::slot (slot_reset_input_context));
    m_helper_agent.signal_connect_update_candidate_geometry (scim::slot (slot_update_candidate_geometry));
    m_helper_agent.signal_connect_update_keyboard_ise (scim::slot (slot_update_keyboard_ise));
    //m_helper_agent.signal_connect_update_keyboard_ise_list (slot (slot_update_keyboard_ise_list));
    m_helper_agent.signal_connect_candidate_more_window_show (scim::slot (slot_candidate_more_window_show));
    m_helper_agent.signal_connect_candidate_more_window_hide (scim::slot (slot_candidate_more_window_hide));
    m_helper_agent.signal_connect_select_aux (scim::slot (slot_select_aux));
    m_helper_agent.signal_connect_select_candidate (scim::slot (slot_select_candidate));
    m_helper_agent.signal_connect_candidate_table_page_up (scim::slot (slot_candidate_table_page_up));
    m_helper_agent.signal_connect_candidate_table_page_down (scim::slot (slot_candidate_table_page_down));
    m_helper_agent.signal_connect_update_candidate_table_page_size (scim::slot (slot_update_candidate_table_page_size));
    m_helper_agent.signal_connect_select_associate (scim::slot (slot_select_associate));
    m_helper_agent.signal_connect_associate_table_page_up (scim::slot (slot_associate_table_page_up));
    m_helper_agent.signal_connect_associate_table_page_down (scim::slot (slot_associate_table_page_down));
    m_helper_agent.signal_connect_update_associate_table_page_size (scim::slot (slot_update_associate_table_page_size));
    m_helper_agent.signal_connect_turn_on_log (scim::slot (slot_turn_on_log));
}
