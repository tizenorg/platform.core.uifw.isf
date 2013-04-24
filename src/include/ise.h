/*
 * Copyright 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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

#ifndef _ISE_H_
#define _ISE_H_

#include <scl.h>
#include <Ecore.h>
#include <Evas.h>
#include <Ecore_Evas.h>
#include <Ecore_IMF.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif  /*  */

#include "languages.h"

#define ISE_VERSION "0.8.0-1"
#define LOCALEDIR "/usr/share/locale"

#define PRIMARY_LATIN_LANGUAGE "English"
#define MAIN_ENTRY_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry.xml"

#define DEFAULT_KEYBOARD_ISE_UUID "d75857a5-4148-4745-89e2-1da7ddaf7999"

//#define INPUT_MODE_NATIVE	MAX_INPUT_MODE /* Native mode. It will distinguish to the current user language */

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

    ISE_LAYOUT_STYLE_MAX
};

typedef struct {
    const sclchar *input_mode;
    const sclchar *sublayout_name;
    const sclboolean force_latin;
} ISE_DEFAULT_VALUES;

const ISE_DEFAULT_VALUES g_ise_default_values[ISE_LAYOUT_STYLE_MAX] = {
    {"",			"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_NORMAL */
    {"SYM_QTY_1",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_NUMBER */
    {"",			"EMAIL",		TRUE },		/* ISE_LAYOUT_STYLE_EMAIL */
    {"",			"URL",			TRUE },		/* ISE_LAYOUT_STYLE_URL */
    {"PHONE_3X4",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_PHONENUMBER */
    {"SYM_QTY_1",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_IP */
    {"MONTH_3X4",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_MONTH */
    {"NUMONLY_3X4",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_NUMBERONLY */
    {"",			"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_INVALID */
    {"SYM_QTY_1",	"DEFAULT",		FALSE },	/* ISE_LAYOUT_STYLE_HEX */
    {"",			"DEFAULT",		TRUE },	/* ISE_LAYOUT_STYLE_TERMINAL */
    {"",			"DEFAULT",		TRUE },	/* ISE_LAYOUT_STYLE_PASSWORD */
};

#define ISE_RETURN_KEY_LABEL_DONE dgettext("sys_string", "IDS_COM_BODY_DONE")
#define ISE_RETURN_KEY_LABEL_GO gettext("IDS_IME_OPT_GO_ABB")
#define ISE_RETURN_KEY_LABEL_JOIN gettext("IDS_IME_OPT_JOIN_M_SIGN_IN")
#define ISE_RETURN_KEY_LABEL_LOGIN gettext("IDS_IME_OPT_LOG_IN_ABB")
#define ISE_RETURN_KEY_LABEL_NEXT dgettext("sys_string", "IDS_COM_SK_NEXT")
#define ISE_RETURN_KEY_LABEL_SEARCH dgettext("sys_string", "IDS_COM_BODY_SEARCH")
#define ISE_RETURN_KEY_LABEL_SEND dgettext("sys_string", "IDS_COM_SK_SEND")
#define ISE_RETURN_KEY_LABEL_SIGNIN dgettext("sys_string", "IDS_COM_HEADER_SIGN_IN")

typedef struct {
    int ic;
    sclu32 layout;
    sclboolean caps_mode;
    sclboolean need_reset;
} KEYBOARD_STATE;

void ise_send_string(const sclchar *key_value);
void ise_update_preedit_string(const sclchar *str);
void ise_send_event(sclulong key_event, sclulong key_mask);
void ise_forward_key_event(sclulong key_event);

void ise_show(int ic);
void ise_hide();
void ise_create();
void ise_destroy();
void ise_reset_context();
void ise_reset_input_context();
void ise_set_layout(sclu32 layout);
void ise_set_screen_rotation(int degree);
void ise_set_caps_mode(unsigned int mode);
void ise_update_cursor_position(int position);
void ise_set_return_key_type(unsigned int type);
void ise_set_return_key_disable(unsigned int disabled);

#endif
