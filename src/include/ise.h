/*
 * Copyright (c) 2012 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _ISE_H_
#define _ISE_H_

#include <sclcore.h>
#include <string>

#include <Elementary.h>

#include "languages.h"

#define ISE_VERSION "1.1.0-1"
#define LOCALEDIR "/usr/share/locale"

#define PRIMARY_LATIN_LANGUAGE "English"

#ifdef _TV
#define MAIN_ENTRY_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/tv/main_entry.xml"
#else
#define MAIN_ENTRY_XML_PATH_480X800    "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry_480x800.xml"
#define MAIN_ENTRY_XML_PATH_540X960    "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry_540x960.xml"
#define MAIN_ENTRY_XML_PATH_720X1280   "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry.xml"
#define MAIN_ENTRY_XML_PATH_1440X2560  "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry_1440x2560.xml"
#define MAIN_ENTRY_XML_PATH MAIN_ENTRY_XML_PATH_720X1280
#endif

#define DEFAULT_KEYBOARD_ISE_UUID "org.tizen.ise-engine-default"

//#define INPUT_MODE_NATIVE MAX_INPUT_MODE /* Native mode. It will distinguish to the current user language */

//#define ISE_RELEASE_AUTOCOMMIT_BLOCK_INTERVAL 1300

#include <dlog.h>
#undef LOG_TAG
#define LOG_TAG "ISE_DEFAULT"

enum ISE_LAYOUT{
    ISE_LAYOUT_STYLE_NORMAL = 0,
    ISE_LAYOUT_STYLE_NUMBER,
    ISE_LAYOUT_STYLE_EMAIL,
    ISE_LAYOUT_STYLE_URL,
    ISE_LAYOUT_STYLE_PHONENUMBER,
    ISE_LAYOUT_STYLE_IP,
    ISE_LAYOUT_STYLE_MONTH,
    ISE_LAYOUT_STYLE_NUMBERONLY,
    ISE_LAYOUT_STYLE_INVALID,
    ISE_LAYOUT_STYLE_HEX,
    ISE_LAYOUT_STYLE_TERMINAL,
    ISE_LAYOUT_STYLE_PASSWORD,
    ISE_LAYOUT_STYLE_DATETIME,
    ISE_LAYOUT_STYLE_EMOTICON,

    ISE_LAYOUT_STYLE_NUMBERONLY_SIG,
    ISE_LAYOUT_STYLE_NUMBERONLY_DEC,
    ISE_LAYOUT_STYLE_NUMBERONLY_SIGDEC,

    ISE_LAYOUT_STYLE_PASSWD_3X4,

    ISE_LAYOUT_STYLE_MAX
};

typedef struct {
    const sclchar *input_mode;
    const sclchar *sublayout_name;
    const sclboolean force_latin;
} ISE_DEFAULT_VALUES;

#ifdef _TV
const ISE_DEFAULT_VALUES g_ise_default_values[ISE_LAYOUT_STYLE_MAX] = {
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NORMAL */
    {"SYM_QTY_1",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NUMBER */
    {"",                "EMAIL",        TRUE },     /* ISE_LAYOUT_STYLE_EMAIL */
    {"",                "URL",          TRUE },     /* ISE_LAYOUT_STYLE_URL */
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_PHONENUMBER */
    {"NUMONLY_QTY",     "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_IP */
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_MONTH */
    {"NUMONLY_QTY",     "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NUMBERONLY */
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_INVALID */
    {"SYM_QTY_1",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_HEX */
    {"",                "DEFAULT",      TRUE },     /* ISE_LAYOUT_STYLE_TERMINAL */
    {"",                "DEFAULT",      TRUE },     /* ISE_LAYOUT_STYLE_PASSWORD */
    {"NUMONLY_QTY",     "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_DATETIME */
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_EMOTICON */

    {"NUMONLY_QTY",     "DEFAULT",   FALSE },       /* ISE_LAYOUT_STYLE_NUMBERONLY_SIG */
    {"NUMONLY_QTY",     "DEFAULT",   FALSE },       /* ISE_LAYOUT_STYLE_NUMBERONLY_DEC */
    {"NUMONLY_QTY",     "DEFAULT",   FALSE },       /* ISE_LAYOUT_STYLE_NUMBERONLY_SIGDEC */

    {"NUMONLY_QTY",     "DEFAULT",   FALSE },       /* ISE_LAYOUT_STYLE_PASSWD_3X4 */
};
#else
const ISE_DEFAULT_VALUES g_ise_default_values[ISE_LAYOUT_STYLE_MAX] = {
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NORMAL */
    {"SYM_QTY_1",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NUMBER */
    {"",                "EMAIL",        TRUE },     /* ISE_LAYOUT_STYLE_EMAIL */
    {"",                "URL",          TRUE },     /* ISE_LAYOUT_STYLE_URL */
    {"PHONE_3X4",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_PHONENUMBER */
    {"NUMONLY_3X4_DEC", "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_IP */
    {"MONTH_3X4",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_MONTH */
    {"NUMONLY_3X4",     "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_NUMBERONLY */
    {"",                "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_INVALID */
    {"SYM_QTY_1",       "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_HEX */
    {"",                "DEFAULT",      TRUE },     /* ISE_LAYOUT_STYLE_TERMINAL */
    {"",                "DEFAULT",      TRUE },     /* ISE_LAYOUT_STYLE_PASSWORD */
    {"DATETIME_3X4",    "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_DATETIME */
    {"EMOTICON_LAYOUT", "DEFAULT",      FALSE },    /* ISE_LAYOUT_STYLE_EMOTICON */

    {"NUMONLY_3X4_SIG",    "DEFAULT",   FALSE },    /* ISE_LAYOUT_STYLE_NUMBERONLY_SIG */
    {"NUMONLY_3X4_DEC",    "DEFAULT",   FALSE },    /* ISE_LAYOUT_STYLE_NUMBERONLY_DEC */
    {"NUMONLY_3X4_SIGDEC", "DEFAULT",   FALSE },    /* ISE_LAYOUT_STYLE_NUMBERONLY_SIGDEC */

    {"PASSWD_3X4",         "DEFAULT",   FALSE },    /* ISE_LAYOUT_STYLE_PASSWD_3X4 */
};
#endif

#define ISE_RETURN_KEY_LABEL_DONE   gettext("IDS_IME_SK_DONE_ABB")
#define ISE_RETURN_KEY_LABEL_GO     gettext("IDS_IME_BUTTON_GO_M_KEYBOARD")
#define ISE_RETURN_KEY_LABEL_JOIN   gettext("IDS_IME_BUTTON_JOIN_M_KEYBOARD")
#define ISE_RETURN_KEY_LABEL_LOGIN  gettext("IDS_IME_BUTTON_LOG_IN_M_KEYBOARD")
#define ISE_RETURN_KEY_LABEL_NEXT   gettext("IDS_IME_OPT_NEXT_ABB")
#define ISE_RETURN_KEY_LABEL_SEARCH gettext("IDS_IME_BUTTON_SEARCH_M_KEYBOARD")
#define ISE_RETURN_KEY_LABEL_SEND   gettext("IDS_IME_BUTTON_SEND_M_KEYBOARD")
#define ISE_RETURN_KEY_LABEL_SIGNIN gettext("IDS_IME_BUTTON_SIGN_IN_M_KEYBOARD")


#define IMDATA_ACTION_DISABLE_EMOTICONS 0x0040


typedef struct {
    int ic;
    int focused_ic;
    sclu32 layout;
    sclu32 layout_variation;
    sclboolean caps_mode;
    sclboolean need_reset;
    sclboolean visible_state;
} KEYBOARD_STATE;

using namespace scl;

class CCoreEventCallback : public ISCLCoreEventCallback
{
    void on_init();
    void on_run(int argc, char **argv);
    void on_exit();

    void on_attach_input_context(sclint ic, const sclchar *ic_uuid);
    void on_detach_input_context(sclint ic, const sclchar *ic_uuid);

    void on_focus_out(sclint ic, const sclchar *ic_uuid);
    void on_focus_in(sclint ic, const sclchar *ic_uuid);

    void on_ise_show(sclint ic, const sclint degree, Ise_Context &context);
    void on_ise_hide(sclint ic, const sclchar *ic_uuid);

    void on_reset_input_context(sclint ic, const sclchar *uuid);

    void on_set_display_language(const sclchar *language);
    void on_set_accessibility_state(const sclboolean state);
    void on_set_rotation_degree(sclint degree);

    void on_set_layout(sclu32 layout);
    void on_set_caps_mode(sclu32 mode);
    void on_update_cursor_position(sclint ic, const sclchar *ic_uuid, sclint cursor_pos);
    void on_update_surrounding_text(sclint ic, const sclchar *text, sclint cursor);

    void on_set_return_key_type(sclu32 type);
    void on_set_return_key_disable(sclu32 disabled);

    void on_set_imdata(sclchar *buf, sclu32 len);
    void on_get_language_locale(sclint ic, sclchar **locale);
    void on_update_lookup_table(SclCandidateTable &table);

    void on_create_option_window(sclwindow window, SCLOptionWindowType type);
    void on_destroy_option_window(sclwindow window);
    void on_check_option_window_availability(sclboolean *ret);

    void on_process_key_event(scim::KeyEvent &key, sclu32 *ret);// only for TV profile to handle remote control button
};

void ise_send_string(const sclchar *key_value);
void ise_update_preedit_string(const sclchar *str);
void ise_send_event(sclulong key_event, sclulong key_mask);
void ise_forward_key_event(sclulong key_event);

void ise_focus_in(int ic);
void ise_focus_out(int ic);
void ise_attach_input_context(int ic);
void ise_detach_input_context(int ic);
void ise_show(int ic);
void ise_hide();
void ise_create();
void ise_destroy();
void ise_reset_context();
void ise_reset_input_context();
void ise_set_layout(sclu32 layout, sclu32 layout_variation);
void ise_set_screen_rotation(int degree);
void ise_set_accessibility_state(bool state);
void ise_set_caps_mode(unsigned int mode);
void ise_update_cursor_position(int position);
void ise_set_return_key_type(unsigned int type);
void ise_set_return_key_disable(unsigned int disabled);
void ise_get_language_locale(char **locale);
void ise_update_table(const std::vector<std::string> &vec_str);
void ise_process_key_event(scim::KeyEvent &key, sclu32 &ret);

#endif
