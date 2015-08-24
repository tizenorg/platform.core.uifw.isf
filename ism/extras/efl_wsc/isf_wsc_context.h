/*
 * Copyright © 2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ISF_WSC_CONTEXT_H_
#define __ISF_WSC_CONTEXT_H_

#include <Ecore.h>
#include <Ecore_IMF.h>
#include <dlog.h>

struct weescim;

const double DOUBLE_SPACE_INTERVAL = 1.0;
const double HIDE_TIMER_INTERVAL = 0.05;
const double WILL_SHOW_TIMER_INTERVAL = 5.0;

typedef struct _WSCContextISF      WSCContextISF;
typedef struct _WSCContextISFImpl  WSCContextISFImpl;

typedef void (*keyboard_input_key_handler_t)(struct weescim *wsc,
                                             uint32_t serial,
                                             uint32_t time, uint32_t key, uint32_t unicode,
                                             char *keyname,
                                             enum wl_keyboard_key_state state);

struct weescim
{
    struct wl_input_method *im;
    struct wl_input_method_context *im_ctx;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;

    struct xkb_context *xkb_context;

    uint32_t modifiers;

    struct xkb_keymap *keymap;
    struct xkb_state *state;
    xkb_mod_mask_t control_mask;
    xkb_mod_mask_t alt_mask;
    xkb_mod_mask_t shift_mask;

    keyboard_input_key_handler_t key_handler;

    char *surrounding_text;
    char *preedit_str;
    char *language;

    uint32_t serial;
    uint32_t text_direction;
    uint32_t preedit_style;
    uint32_t content_hint;
    uint32_t content_purpose;
    uint32_t surrounding_cursor;
    uint32_t return_key_type;
    int      cursor_pos;

    Eina_Bool context_changed;
    Eina_Bool hw_kbd;
    Eina_Bool return_key_disabled;

    WSCContextISF *wsc_ctx;
};

struct _WSCContextISF {
    weescim *ctx;

    WSCContextISFImpl *impl;

    int id; /* Input Context id*/
    _WSCContextISF *next;
};

EAPI void get_language(char **language);
EAPI int get_panel_client_id ();
EAPI Eina_Bool caps_mode_check (WSCContextISF *ctx, Eina_Bool force, Eina_Bool noti);

EAPI WSCContextISF *get_focused_ic ();

EAPI void context_scim_imdata_get (WSCContextISF *ctx, void* data, int* length);
EAPI void imengine_layout_set (WSCContextISF *ctx, Ecore_IMF_Input_Panel_Layout layout);

EAPI void isf_wsc_context_add (WSCContextISF *ctx);
EAPI void isf_wsc_context_del (WSCContextISF *ctx);
EAPI void isf_wsc_context_focus_in (WSCContextISF *ctx);
EAPI void isf_wsc_context_focus_out (WSCContextISF *ctx);
EAPI void isf_wsc_context_reset (WSCContextISF *ctx);
EAPI void isf_wsc_context_preedit_string_get (WSCContextISF *ctx, char** str, int *cursor_pos);
EAPI void isf_wsc_context_prediction_allow_set (WSCContextISF* ctx, Eina_Bool prediction);
EAPI Eina_Bool isf_wsc_context_prediction_allow_get (WSCContextISF* ctx);
EAPI void isf_wsc_context_autocapital_type_set (WSCContextISF* ctx, Ecore_IMF_Autocapital_Type autocapital_type);
EAPI void isf_wsc_context_filter_key_event (struct weescim *wsc,
                                             uint32_t serial,
                                             uint32_t timestamp, uint32_t key, uint32_t unicode,
                                             char *keyname,
                                             enum wl_keyboard_key_state state);

EAPI WSCContextISF* isf_wsc_context_new      (void);
EAPI void           isf_wsc_context_shutdown (void);

bool wsc_context_surrounding_get (weescim *ctx, char **text, int *cursor_pos);
Ecore_IMF_Input_Panel_Layout wsc_context_input_panel_layout_get(weescim *ctx);
bool wsc_context_input_panel_caps_lock_mode_get(weescim *ctx);
void wsc_context_delete_surrounding (weescim *ctx, int offset, int len);
Ecore_IMF_Autocapital_Type wsc_context_autocapital_type_get (weescim *ctx);
Ecore_IMF_Input_Panel_Lang wsc_context_input_panel_language_get (weescim *ctx);
bool wsc_context_input_panel_password_mode_get (weescim *ctx);
Ecore_IMF_Input_Hints wsc_context_input_hint_get (weescim *ctx);
Eina_Bool wsc_context_prediction_allow_get (weescim *ctx);
void wsc_context_commit_preedit_string(weescim *ctx);
void wsc_context_commit_string(weescim *ctx, const char *str);
void wsc_context_send_preedit_string(weescim *ctx);
void wsc_context_send_key(weescim *ctx, uint32_t keysym, uint32_t modifiers, uint32_t time, bool press);

#endif /* __ISF_WSC_CONTEXT_H_ */
