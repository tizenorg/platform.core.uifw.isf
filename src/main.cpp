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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Elementary.h>
#include <scl.h>
#include "common.h"
#include "ise.h"

using namespace scim;
using namespace scl;

#include <Elementary.h>

#define ISEUUID "12aa3425-f88d-45f4-a509-cee8dfe904e3"
#define ISENAME "Tizen keyboard"

CISECommon *g_ise_common = NULL;
ConfigPointer _scim_config (0);

extern KEYBOARD_STATE g_keyboard_state;

/* A temporary function for setting imdata for supporting legacy language setting API */
extern void set_ise_imdata(const char * buf, size_t &len);

class CCoreEventCallback : public IISECommonEventCallback
{
    void init();
    void exit(sclint ic, const sclchar *ic_uuid);

    void attach_input_context(sclint ic, const sclchar *ic_uuid);
    void detach_input_context(sclint ic, const sclchar *ic_uuid);

    void focus_out (sclint ic, const sclchar *ic_uuid);
    void focus_in (sclint ic, const sclchar *ic_uuid);

    void ise_show(sclint ic, const sclint degree, Ise_Context context);
    void ise_hide(sclint ic, const sclchar *ic_uuid);

    void reset_input_context(sclint ic, const sclchar *uuid);

    void set_display_language(const sclchar *language);
    void set_accessibility_state(const sclboolean state);
    void set_rotation_degree(sclint degree);

    void set_caps_mode(sclu32 mode);
    void update_cursor_position(sclint ic, const sclchar *ic_uuid, sclint cursor_pos);

    void set_return_key_type (sclu32 type);
    void set_return_key_disable (sclu32 disabled);

    void set_imdata (sclchar *buf, sclu32 len);
    void get_language_locale (sclint ic, sclchar **locale);
};

void CCoreEventCallback::init()
{
    LOGD("CCoreEventCallback::init()\n");
    ise_create();
}

void CCoreEventCallback::exit(sclint ic, const sclchar *ic_uuid)
{
    ise_destroy();
    ::ise_hide();
}

void CCoreEventCallback::attach_input_context(sclint ic, const sclchar *ic_uuid)
{
    ise_attach_input_context(ic);
}

void CCoreEventCallback::detach_input_context(sclint ic, const sclchar *ic_uuid)
{
    ise_detach_input_context(ic);
}

void CCoreEventCallback::focus_in(sclint ic, const sclchar *ic_uuid)
{
    ise_focus_in(ic);
}

void CCoreEventCallback::focus_out(sclint ic, const sclchar *ic_uuid)
{
    ise_focus_out(ic);
}

void CCoreEventCallback::ise_show(sclint ic, const sclint degree, Ise_Context context)
{
    //g_ise_common->set_keyboard_ise_by_uuid(KEYBD_ISE_UUID);

    ise_reset_context(); // reset ISE

    /*if (context.language == ECORE_IMF_INPUT_PANEL_LANG_ALPHABET) {
        ise_explictly_set_language(PRIMARY_LATIN_LANGUAGE_INDEX);
    }*/

    ise_set_layout(context.layout);

    ise_set_return_key_type(context.return_key_type);
    ise_set_return_key_disable(context.return_key_disabled);

    ise_set_caps_mode(context.caps_mode);
    ise_update_cursor_position(context.cursor_pos);

    /* Do not follow the application's rotation angle if we are already in visible state,
       since in that case we will receive the angle through ROTATION_CHANGE_REQUEST message */
    if (!(g_keyboard_state.visible_state)) {
        ise_set_screen_rotation(degree);
    } else {
        LOGD("Skipping rotation angle , %d", degree);
    }

    ::ise_show(ic);
}

void CCoreEventCallback::ise_hide(sclint ic, const sclchar *ic_uuid)
{
    ::ise_hide();
}

void CCoreEventCallback::reset_input_context(sclint ic, const sclchar *uuid)
{
    ise_reset_input_context();
}

void CCoreEventCallback::set_display_language(const sclchar *language)
{
    setlocale(LC_ALL, language);
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    LOGD("Language : %s\n", (language ? language : "NULL"));
}

void CCoreEventCallback::set_accessibility_state(const sclboolean state)
{
    ise_set_accessibility_state(state);
}

void CCoreEventCallback::set_rotation_degree(sclint degree)
{
    ise_set_screen_rotation(degree);
}

void CCoreEventCallback::set_caps_mode(sclu32 mode)
{
    ise_set_caps_mode(mode);
}

void CCoreEventCallback::update_cursor_position(sclint ic, const sclchar *ic_uuid, sclint cursor_pos)
{
    ise_update_cursor_position(cursor_pos);
}

void CCoreEventCallback::set_return_key_type (sclu32 type)
{
    ise_set_return_key_type(type);
}

void CCoreEventCallback::set_return_key_disable (sclu32 disabled)
{
    ise_set_return_key_disable(disabled);
}

void CCoreEventCallback::set_imdata(sclchar *buf, sclu32 len)
{
    set_ise_imdata(buf, len);
}

void CCoreEventCallback::get_language_locale(sclint ic, sclchar **locale)
{
    ise_get_language_locale(locale);
}


static CCoreEventCallback g_core_event_callback;

extern "C"
{
    void scim_module_init (void) {
        if (g_ise_common == NULL) {
            g_ise_common = CISECommon::get_instance();
        }
        if  (g_ise_common) {
            g_ise_common->init(ISENAME, ISEUUID, "en_US");
            g_ise_common->set_core_event_callback(&g_core_event_callback);
        }
    }

    void scim_module_exit (void) {
        if (g_ise_common) {
            g_ise_common->exit();

            delete g_ise_common;
            g_ise_common = NULL;
        }
    }

    unsigned int scim_helper_module_number_of_helpers (void) {
        if (g_ise_common) {
            return g_ise_common->get_number_of_helpers();
        }
        return 0;
    }

    bool scim_helper_module_get_helper_info (unsigned int idx, HelperInfo &info) {
        if (g_ise_common) {
            return g_ise_common->get_helper_info(idx, info);
        }
        return false;
    }

    String scim_helper_module_get_helper_language (unsigned int idx) {
        if (g_ise_common) {
            return g_ise_common->get_helper_language(idx);
        }
        return String("");
    }

    void scim_helper_module_run_helper (const String &uuid, const ConfigPointer &config, const String &display) {
        _scim_config = config;
        if (g_ise_common) {
            g_ise_common->run(uuid.c_str(), config, display.c_str());
        }
    }
}
