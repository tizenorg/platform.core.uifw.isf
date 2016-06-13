/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2013 Intel Corporation
 * Copyright (c) 2013-2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Li Zhang <li2012.zhang@samsung.com>
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

#ifndef __ISF_WSC_CONTEXT_H_
#define __ISF_WSC_CONTEXT_H_

#include <Ecore.h>
#include <Ecore_IMF.h>
#include <dlog.h>
#include "scim_stl_map.h"

#if SCIM_USE_STL_EXT_HASH_MAP
    typedef __gnu_cxx::hash_map <uint32_t, uint32_t, __gnu_cxx::hash <uint32_t> >   KeycodeRepository;
#elif SCIM_USE_STL_HASH_MAP
    typedef std::hash_map <uint32_t, ClientInfo, std::hash <uint32_t> >             KeycodeRepository;
#else
    typedef std::map <uint32_t, uint32_t>                                           KeycodeRepository;
#endif

struct weescim;

const double DOUBLE_SPACE_INTERVAL = 1.0;
const double HIDE_TIMER_INTERVAL = 0.05;
const double WILL_SHOW_TIMER_INTERVAL = 5.0;

typedef struct _WSCContextISF      WSCContextISF;
typedef struct _WSCContextISFImpl  WSCContextISFImpl;

typedef void (*keyboard_input_key_handler_t)(WSCContextISF *wsc_ctx,
                                             uint32_t serial,
                                             uint32_t time, uint32_t keycode, uint32_t symcode,
                                             char *keyname,
                                             enum wl_keyboard_key_state state);

struct weescim
{
    struct wl_input_method *im;

    WSCContextISF *wsc_ctx;
};

struct _WSCContextISF {
    weescim *ctx;

    struct wl_keyboard *keyboard;
    struct wl_input_method_context *im_ctx;

    struct xkb_context *xkb_context;

    uint32_t modifiers;

    struct xkb_keymap *keymap;
    struct xkb_state *state;
    xkb_mod_mask_t control_mask;
    xkb_mod_mask_t alt_mask;
    xkb_mod_mask_t shift_mask;

    KeycodeRepository _keysym2keycode;

    keyboard_input_key_handler_t key_handler;

    char *surrounding_text;
    char *preedit_str;
    char *language;

    uint32_t serial;
    uint32_t content_hint;
    uint32_t content_purpose;
    uint32_t surrounding_cursor;
    uint32_t return_key_type;
    uint32_t bidi_direction;

    Eina_Bool context_changed;
    Eina_Bool return_key_disabled;

    WSCContextISFImpl *impl;

    int id; /* Input Context id*/
    _WSCContextISF *next;
};

void get_language(char **language);
int get_panel_client_id ();
Eina_Bool caps_mode_check (WSCContextISF *wsc_ctx, Eina_Bool force, Eina_Bool noti);

WSCContextISF *get_focused_ic ();

void context_scim_imdata_get (WSCContextISF *wsc_ctx, void* data, int* length);

void isf_wsc_context_add (WSCContextISF *wsc_ctx);
void isf_wsc_context_del (WSCContextISF *wsc_ctx);
void isf_wsc_context_focus_in (WSCContextISF *wsc_ctx);
void isf_wsc_context_focus_out (WSCContextISF *wsc_ctx);
void isf_wsc_context_preedit_string_get (WSCContextISF *wsc_ctx, char** str, int *cursor_pos);
void isf_wsc_context_autocapital_type_set (WSCContextISF* wsc_ctx, Ecore_IMF_Autocapital_Type autocapital_type);
void isf_wsc_context_bidi_direction_set (WSCContextISF* wsc_ctx, Ecore_IMF_BiDi_Direction direction);
void isf_wsc_context_filter_key_event (WSCContextISF* wsc_ctx,
                                       uint32_t serial,
                                       uint32_t timestamp, uint32_t key, uint32_t unicode,
                                       char *keyname,
                                       enum wl_keyboard_key_state state);

bool wsc_context_surrounding_get (WSCContextISF *wsc_ctx, char **text, int *cursor_pos);
Ecore_IMF_Input_Panel_Layout wsc_context_input_panel_layout_get(WSCContextISF *wsc_ctx);
int wsc_context_input_panel_layout_variation_get (WSCContextISF *wsc_ctx);
bool wsc_context_input_panel_caps_lock_mode_get(WSCContextISF *wsc_ctx);
void wsc_context_delete_surrounding (WSCContextISF *wsc_ctx, int offset, int len);
Ecore_IMF_Autocapital_Type wsc_context_autocapital_type_get (WSCContextISF *wsc_ctx);
Ecore_IMF_Input_Panel_Lang wsc_context_input_panel_language_get (WSCContextISF *wsc_ctx);
bool wsc_context_input_panel_password_mode_get (WSCContextISF *wsc_ctx);
Ecore_IMF_Input_Hints wsc_context_input_hint_get (WSCContextISF *wsc_ctx);
Eina_Bool wsc_context_prediction_allow_get (WSCContextISF *wsc_ctx);
void wsc_context_commit_preedit_string(WSCContextISF *wsc_ctx);
void wsc_context_commit_string(WSCContextISF *wsc_ctx, const char *str);
void wsc_context_send_preedit_string(WSCContextISF *wsc_ctx);
void wsc_context_send_key(WSCContextISF *wsc_ctx, uint32_t keysym, uint32_t modifiers, uint32_t time, bool press);

#endif /* __ISF_WSC_CONTEXT_H_ */
