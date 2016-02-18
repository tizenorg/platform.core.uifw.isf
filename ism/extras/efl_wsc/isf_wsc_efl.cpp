/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more fitable.
 * Copyright (c) 2012-2014 Intel Co., Ltd.
 *
 * Contact: Yan Wang <yan.wang@intel.com>
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

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_COMPOSE_KEY
#define Uses_SCIM_IMENGINE_MODULE

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <Ecore.h>
#include <Ecore_Wayland.h>
#include <malloc.h>
#include "scim_private.h"
#include "scim.h"
#include <dlog.h>

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include <input-method-client-protocol.h>
#include <text-client-protocol.h>
#include "isf_wsc_context.h"
#include "isf_wsc_control_ui.h"

using namespace scim;

/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_WSC_EFL"

static struct weescim _wsc                                  = {0};


/////////////////////////////////////////////////////////////////////////////
// Implementation of Wayland Input Method functions.
/////////////////////////////////////////////////////////////////////////////
static void
_wsc_im_ctx_surrounding_text(void *data, struct wl_input_method_context *im_ctx, const char *text, uint32_t cursor, uint32_t anchor)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    if (wsc_ctx->surrounding_text)
        free (wsc_ctx->surrounding_text);

    wsc_ctx->surrounding_text = strdup (text ? text : "");
    wsc_ctx->surrounding_cursor = cursor;

    isf_wsc_context_cursor_position_set(wsc_ctx, cursor);

    LOGD ("text : '%s', cursor : %d\n", text, cursor);
}

static void
_wsc_im_ctx_reset(void *data, struct wl_input_method_context *im_ctx)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;
    isf_wsc_context_reset(wsc_ctx);
}

static void
_wsc_im_ctx_content_type(void *data, struct wl_input_method_context *im_ctx, uint32_t hint, uint32_t purpose)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    LOGD ("im_context = %p hint = %d purpose = %d\n", im_ctx, hint, purpose);

    if (!wsc_ctx->context_changed) return;

    wsc_ctx->content_hint = hint;
    wsc_ctx->content_purpose = purpose;

    isf_wsc_context_input_panel_layout_set (wsc_ctx,
                                            wsc_context_input_panel_layout_get (wsc_ctx));

    isf_wsc_context_autocapital_type_set (wsc_ctx, wsc_context_autocapital_type_get(wsc_ctx));

    isf_wsc_context_input_panel_language_set (wsc_ctx, wsc_context_input_panel_language_get(wsc_ctx));

    caps_mode_check (wsc_ctx, EINA_TRUE, EINA_TRUE);

    wsc_ctx->context_changed = EINA_FALSE;
}

static void
_wsc_im_ctx_invoke_action(void *data, struct wl_input_method_context *im_ctx, uint32_t button, uint32_t index)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    LOGD ("invoke action. button : %d\n", button);

    if (button != BTN_LEFT)
        return;

    wsc_context_send_preedit_string (wsc_ctx);
}

static void
_wsc_im_ctx_commit_state(void *data, struct wl_input_method_context *im_ctx, uint32_t serial)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    wsc_ctx->serial = serial;

    if (wsc_ctx->surrounding_text)
        LOGD ("Surrounding text updated: '%s'\n", wsc_ctx->surrounding_text);

    if (wsc_ctx->language)
        wl_input_method_context_language (im_ctx, wsc_ctx->serial, wsc_ctx->language);

    wl_input_method_context_text_direction (im_ctx, wsc_ctx->serial, wsc_ctx->text_direction);
}

static void
_wsc_im_ctx_preferred_language(void *data, struct wl_input_method_context *im_ctx, const char *language)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    if (!wsc_ctx) return;

    if (language && wsc_ctx->language && !strcmp (language, wsc_ctx->language))
        return;

    if (wsc_ctx->language) {
        free (wsc_ctx->language);
        wsc_ctx->language = NULL;
    }

    if (language) {
        wsc_ctx->language = strdup (language);
        LOGD ("Language changed, new: '%s'\n", language);
    }
}

static void
_wsc_im_ctx_return_key_type(void *data, struct wl_input_method_context *im_ctx, uint32_t return_key_type)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD ("im_context = %p return key type = %d\n", im_ctx, return_key_type);
    if (!wsc_ctx) return;

    if (wsc_ctx->return_key_type != return_key_type) {
        wsc_ctx->return_key_type = return_key_type;
        isf_wsc_context_input_panel_return_key_type_set (wsc_ctx, (Ecore_IMF_Input_Panel_Return_Key_Type)wsc_ctx->return_key_type);
    }
}

static void
_wsc_im_ctx_return_key_disabled(void *data, struct wl_input_method_context *im_ctx, uint32_t disabled)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    Eina_Bool return_key_disabled = !!disabled;

    LOGD ("im_context = %p return key disabled = %d\n", im_ctx, return_key_disabled);
    if (!wsc_ctx) return;

    if (wsc_ctx->return_key_disabled != return_key_disabled) {
        wsc_ctx->return_key_disabled = return_key_disabled;
        isf_wsc_context_input_panel_return_key_disabled_set (wsc_ctx, wsc_ctx->return_key_disabled);
    }
}

static void
_wsc_im_ctx_input_panel_data(void *data, struct wl_input_method_context *im_ctx, const char *input_panel_data, uint32_t input_panel_data_length)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    LOGD ("im_context = %p input panel data = %s len = %d\n", im_ctx, input_panel_data, input_panel_data_length);
    if (!wsc_ctx) return;

    isf_wsc_context_input_panel_imdata_set (wsc_ctx, (void *)input_panel_data, input_panel_data_length);
}

static void
_wsc_im_ctx_bidi_direction(void *data, struct wl_input_method_context *im_ctx, uint32_t bidi_direction)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;

    LOGD ("im_context = %p bidi_direction = %d\n", im_ctx, bidi_direction);
    if (!wsc_ctx) return;

    if (wsc_ctx->bidi_direction != bidi_direction) {
        wsc_ctx->bidi_direction = bidi_direction;

        isf_wsc_context_bidi_direction_set (wsc_ctx, (Ecore_IMF_BiDi_Direction)wsc_ctx->bidi_direction);
    }
}

static const struct wl_input_method_context_listener wsc_im_context_listener = {
     _wsc_im_ctx_surrounding_text,
     _wsc_im_ctx_reset,
     _wsc_im_ctx_content_type,
     _wsc_im_ctx_invoke_action,
     _wsc_im_ctx_commit_state,
     _wsc_im_ctx_preferred_language,
     _wsc_im_ctx_return_key_type,
     _wsc_im_ctx_return_key_disabled,
     _wsc_im_ctx_input_panel_data,
     _wsc_im_ctx_bidi_direction
};

static void
_init_keysym2keycode(WSCContextISF *wsc_ctx)
{
    uint32_t i = 0;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;

    if (!wsc_ctx || !wsc_ctx->state)
        return;

    for (i = 0; i < 256; i++) {
        code = i + 8;
        num_syms = xkb_key_get_syms(wsc_ctx->state, code, &syms);

        if (num_syms == 1)
            wsc_ctx->_keysym2keycode[syms[0]] = i;
    }
}

static void
_fini_keysym2keycode(WSCContextISF *wsc_ctx)
{
    wsc_ctx->_keysym2keycode.clear();
}

static void
_wsc_im_keyboard_keymap(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t format,
        int32_t fd,
        uint32_t size)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    char *map_str;

    if (!wsc_ctx) return;

    _fini_keysym2keycode(wsc_ctx);

    if (wsc_ctx->state) {
        xkb_state_unref(wsc_ctx->state);
        wsc_ctx->state = NULL;
    }

    if (wsc_ctx->keymap) {
        xkb_map_unref(wsc_ctx->keymap);
        wsc_ctx->keymap = NULL;
    }

    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
        close(fd);
        return;
    }

    map_str = (char*)mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_str == MAP_FAILED) {
        close(fd);
        return;
    }

    wsc_ctx->keymap =
        xkb_map_new_from_string(wsc_ctx->xkb_context,
                map_str,
                XKB_KEYMAP_FORMAT_TEXT_V1,
                (xkb_keymap_compile_flags)0);

    munmap(map_str, size);
    close(fd);

    if (!wsc_ctx->keymap) {
        LOGW ("failed to compile keymap\n");
        return;
    }

    wsc_ctx->state = xkb_state_new(wsc_ctx->keymap);
    if (!wsc_ctx->state) {
        LOGW ("failed to create XKB state\n");
        xkb_map_unref(wsc_ctx->keymap);
        return;
    }

    wsc_ctx->control_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Control");
    wsc_ctx->alt_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Mod1");
    wsc_ctx->shift_mask =
        1 << xkb_map_mod_get_index(wsc_ctx->keymap, "Shift");

    LOGW ("create _keysym2keycode\n");
    _init_keysym2keycode(wsc_ctx);
}

static void
_wsc_im_keyboard_key(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t time,
        uint32_t key,
        uint32_t state_w)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    char keyname[64] = {0};
    enum wl_keyboard_key_state state = (wl_keyboard_key_state)state_w;

    if (!wsc_ctx || !wsc_ctx->state)
        return;

    code = key + 8;
    num_syms = xkb_key_get_syms(wsc_ctx->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
    {
        sym = syms[0];
        xkb_keysym_get_name(sym, keyname, 64);
    }

    if (wsc_ctx->key_handler)
        (*wsc_ctx->key_handler)(wsc_ctx, serial, time, code, sym, keyname,
                state);
}

static void
_wsc_im_keyboard_modifiers(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group)
{
    WSCContextISF *wsc_ctx = (WSCContextISF*)data;
    struct wl_input_method_context *context = wsc_ctx->im_ctx;
    xkb_mod_mask_t mask;

    if (!wsc_ctx || !wsc_ctx->state)
        return;

    xkb_state_update_mask(wsc_ctx->state, mods_depressed,
            mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods(wsc_ctx->state,
            (xkb_state_component)(XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

    wsc_ctx->modifiers = 0;
    if (mask & wsc_ctx->control_mask)
        wsc_ctx->modifiers |= SCIM_KEY_ControlMask;
    if (mask & wsc_ctx->alt_mask)
        wsc_ctx->modifiers |= SCIM_KEY_AltMask;
    if (mask & wsc_ctx->shift_mask)
        wsc_ctx->modifiers |= SCIM_KEY_ShiftMask;

    wl_input_method_context_modifiers(context, serial,
            mods_depressed, mods_depressed,
            mods_latched, group);
}

static const struct wl_keyboard_listener wsc_im_keyboard_listener = {
    _wsc_im_keyboard_keymap,
    NULL, /* enter */
    NULL, /* leave */
    _wsc_im_keyboard_key,
    _wsc_im_keyboard_modifiers
};

static void
_wsc_im_activate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx, uint32_t text_input_id)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    WSCContextISF *wsc_ctx = new WSCContextISF;
    if (!wsc_ctx) {
        return;
    }
    wsc_ctx->xkb_context = xkb_context_new((xkb_context_flags)0);
    if (wsc_ctx->xkb_context == NULL) {
        LOGW ("Failed to create XKB context\n");
        delete wsc_ctx;
        return;
    }
    wsc_ctx->id = text_input_id;
    wsc->wsc_ctx = wsc_ctx;
    wsc_ctx->ctx = wsc;
    wsc_ctx->state = NULL;
    wsc_ctx->keymap = NULL;
    wsc_ctx->surrounding_text = NULL;
    wsc_ctx->key_handler = isf_wsc_context_filter_key_event;

    get_language(&wsc_ctx->language);

    wsc_ctx->preedit_str = strdup ("");
    wsc_ctx->content_hint = WL_TEXT_INPUT_CONTENT_HINT_NONE;
    wsc_ctx->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;

    wsc_ctx->im_ctx = im_ctx;
    wl_input_method_context_add_listener (im_ctx, &wsc_im_context_listener, wsc_ctx);

    wsc_ctx->keyboard = wl_input_method_context_grab_keyboard(im_ctx);
    if (wsc_ctx->keyboard)
        wl_keyboard_add_listener(wsc_ctx->keyboard, &wsc_im_keyboard_listener, wsc_ctx);

    if (wsc_ctx->language)
        wl_input_method_context_language (im_ctx, wsc_ctx->serial, wsc_ctx->language);

    wl_input_method_context_text_direction (im_ctx, wsc_ctx->serial, wsc_ctx->text_direction);

    isf_wsc_context_add (wsc_ctx);

    wsc_ctx->context_changed = EINA_TRUE;

    isf_wsc_context_focus_in (wsc_ctx);
}

static void
_wsc_im_deactivate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;
    WSCContextISF *wsc_ctx = wsc->wsc_ctx;

    isf_wsc_context_input_panel_hide (wsc_ctx);
    isf_wsc_context_focus_out (wsc_ctx);

    if (wsc_ctx->keyboard) {
        wl_keyboard_destroy (wsc_ctx->keyboard);
        wsc_ctx->keyboard = NULL;
    }

    _fini_keysym2keycode (wsc_ctx);

    if (wsc_ctx->state) {
        xkb_state_unref (wsc_ctx->state);
        wsc_ctx->state = NULL;
    }

    if (wsc_ctx->keymap) {
        xkb_map_unref (wsc_ctx->keymap);
        wsc_ctx->keymap = NULL;
    }

    if (wsc_ctx->im_ctx) {
        wl_input_method_context_destroy (wsc_ctx->im_ctx);
        wsc_ctx->im_ctx = NULL;
    }

    if (wsc_ctx->preedit_str) {
        free (wsc_ctx->preedit_str);
        wsc_ctx->preedit_str = NULL;
    }

    if (wsc_ctx->surrounding_text) {
        free (wsc_ctx->surrounding_text);
        wsc_ctx->surrounding_text = NULL;
    }

    if (wsc_ctx->language) {
        free (wsc_ctx->language);
        wsc_ctx->language = NULL;
    }

    isf_wsc_context_del (wsc_ctx);
    delete wsc_ctx;
    wsc->wsc_ctx = NULL;
}

static void
_wsc_im_show_input_panel(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;

    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static void
_wsc_im_hide_input_panel(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc || !wsc->wsc_ctx) return;

    isf_wsc_context_input_panel_hide (wsc->wsc_ctx);
}

static const struct wl_input_method_listener wsc_im_listener = {
    _wsc_im_activate,
    _wsc_im_deactivate,
    _wsc_im_show_input_panel,
    _wsc_im_hide_input_panel
};

static void
_wsc_seat_handle_capabilities(void *data, struct wl_seat *seat,
                              uint32_t caps)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD)) {

    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {

    }
}

static const struct wl_seat_listener wsc_seat_listener = {
    _wsc_seat_handle_capabilities,
};

static bool
_wsc_setup(struct weescim *wsc)
{
    Eina_Inlist *globals;
    struct wl_registry *registry;
    Ecore_Wl_Global *global;

    if (!wsc) return false;

    if (!(registry = ecore_wl_registry_get()))
        return false;

    if (!(globals = ecore_wl_globals_get()))
        return false;

    EINA_INLIST_FOREACH(globals, global) {
        if (strcmp (global->interface, "wl_input_method") == 0)
            wsc->im = (wl_input_method*)wl_registry_bind (registry, global->id, &wl_input_method_interface, 1);
        else if (strcmp (global->interface, "wl_seat") == 0)
            wsc->seat = (wl_seat*)wl_registry_bind (registry, global->id, &wl_seat_interface, 1);
    }

    if (wsc->im == NULL) {
        LOGW ("Failed because wl_input_method is null\n");
        return false;
    }

    /* Input method listener */
    LOGD ("Adding wl_input_method listener\n");

    if (wsc->im)
        wl_input_method_add_listener (wsc->im, &wsc_im_listener, wsc);
    else {
        LOGW ("Couldn't get wayland input method interface\n");
        return false;
    }

    if (wsc->seat)
        wl_seat_add_listener(wsc->seat, &wsc_seat_listener, wsc);
    else {
        LOGW ("Couldn't get wayland seat interface\n");
        return false;
    }

    return true;
}

int main (int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
    sleep(1);

    isf_wsc_context_init ();

    if (!_wsc_setup (&_wsc)) {
        return 0;
    }

    ecore_main_loop_begin();

    isf_wsc_context_shutdown ();

    return 0;
}

/*
vi:ts=4:nowrap:expandtab
*/
