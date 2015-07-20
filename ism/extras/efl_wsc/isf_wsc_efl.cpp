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
#include <list>
#include <glib.h>
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Wayland.h>
#include <Ecore_File.h>
#include <malloc.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#if HAVE_VCONF
#include <vconf.h>
#include <vconf-keys.h>
#endif
#include <privilege-control.h>
#include <dlog.h>

#include <linux/input.h>
#include <xkbcommon/xkbcommon.h>
#include <sys/mman.h>
#include "input-method-client-protocol.h"
#include "text-client-protocol.h"
#include "isf_wsc_context.h"
#include "isf_wsc_control_ui.h"
#include "scim_stl_map.h"

using namespace scim;

#if SCIM_USE_STL_EXT_HASH_MAP
    typedef __gnu_cxx::hash_map <uint32_t, uint32_t, __gnu_cxx::hash <uint32_t> >   KeycodeRepository;
#elif SCIM_USE_STL_HASH_MAP
    typedef std::hash_map <uint32_t, ClientInfo, std::hash <uint32_t> >             KeycodeRepository;
#else
    typedef std::map <uint32_t, uint32_t>                                           KeycodeRepository;
#endif

static KeycodeRepository _keysym2keycode;

/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_WSC_EFL"

/* This structure stores the wayland input method information */
#define MOD_SHIFT_MASK      0x01
#define MOD_ALT_MASK        0x02
#define MOD_CONTROL_MASK    0x04

static struct weescim _wsc                                  = {0};

/////////////////////////////////////////////////////////////////////////////
// Implementation of Wayland Input Method functions.
/////////////////////////////////////////////////////////////////////////////
static void
_wsc_im_ctx_surrounding_text(void *data, struct wl_input_method_context *im_ctx, const char *text, uint32_t cursor, uint32_t anchor)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    if (wsc->surrounding_text)
        free (wsc->surrounding_text);

    wsc->surrounding_text = strdup (text);
    wsc->surrounding_cursor = cursor;

    if (wsc->cursor_pos != cursor) {
        wsc->cursor_pos = cursor;
        caps_mode_check (wsc->wsc_ctx, EINA_FALSE, EINA_TRUE);
    }

    LOGD ("text : '%s', cursor : %d", text, cursor);
}

static void
_wsc_im_ctx_reset(void *data, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    isf_wsc_context_reset(wsc->wsc_ctx);
}

static void
_wsc_im_ctx_content_type(void *data, struct wl_input_method_context *im_ctx, uint32_t hint, uint32_t purpose)
{
    struct weescim *wsc = (weescim*)data;

    LOGD ("im_context = %p hint = %d purpose = %d", im_ctx, hint, purpose);

    if (!wsc || !wsc->context_changed)
        return;

    wsc->content_hint = hint;
    wsc->content_purpose = purpose;

    isf_wsc_context_input_panel_layout_set (wsc->wsc_ctx,
                                            wsc_context_input_panel_layout_get (wsc));

    isf_wsc_context_autocapital_type_set (wsc->wsc_ctx, wsc_context_autocapital_type_get(wsc));

    isf_wsc_context_input_panel_language_set (wsc->wsc_ctx, wsc_context_input_panel_language_get(wsc));

    caps_mode_check (wsc->wsc_ctx, EINA_TRUE, EINA_TRUE);

    wsc->context_changed = EINA_FALSE;

    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static void
_wsc_im_ctx_invoke_action(void *data, struct wl_input_method_context *im_ctx, uint32_t button, uint32_t index)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    LOGD ("invoke action. button : %d", button);

    if (button != BTN_LEFT)
        return;

    wsc_context_send_preedit_string (wsc);
}

static void
_wsc_im_ctx_commit_state(void *data, struct wl_input_method_context *im_ctx, uint32_t serial)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    wsc->serial = serial;

    if (wsc->surrounding_text)
        LOGD ("Surrounding text updated: %s", wsc->surrounding_text);

    if (wsc->language)
        wl_input_method_context_language (im_ctx, wsc->serial, wsc->language);

    wl_input_method_context_text_direction (im_ctx, wsc->serial, wsc->text_direction);
}

static void
_wsc_im_ctx_preferred_language(void *data, struct wl_input_method_context *im_ctx, const char *language)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    if (language && wsc->language && !strcmp (language, wsc->language))
        return;

    if (wsc->language) {
        free (wsc->language);
        wsc->language = NULL;
    }

    if (language) {
        wsc->language = strdup (language);
        LOGD ("Language changed, new: '%s'", language);
    }
}

static void
_wsc_im_ctx_return_key_type(void *data, struct wl_input_method_context *im_ctx, uint32_t return_key_type)
{
    struct weescim *wsc = (weescim*)data;

    LOGD ("im_context = %p return key type = %d", im_ctx, return_key_type);

    wsc->return_key_type = return_key_type;

    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static const struct wl_input_method_context_listener wsc_im_context_listener = {
     _wsc_im_ctx_surrounding_text,
     _wsc_im_ctx_reset,
     _wsc_im_ctx_content_type,
     _wsc_im_ctx_invoke_action,
     _wsc_im_ctx_commit_state,
     _wsc_im_ctx_preferred_language,
     _wsc_im_ctx_return_key_type,
};

static void
_init_keysym2keycode(struct weescim *wsc)
{
    uint32_t i = 0;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;

    if (!wsc || !wsc->state)
        return;

    for (i = 0; i < 256; i++) {
        code = i + 8;
        num_syms = xkb_key_get_syms(wsc->state, code, &syms);

        if (num_syms == 1)
            _keysym2keycode[syms[0]] = i;
    }
}

static void
_fini_keysym2keycode(struct weescim *wsc)
{
    _keysym2keycode.clear();
}

static void
_wsc_im_keyboard_keymap(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t format,
        int32_t fd,
        uint32_t size)
{
    struct weescim *wsc = (weescim*)data;
    char *map_str;

    if (!wsc) return;

    _fini_keysym2keycode(wsc);

    if (wsc->state) {
        xkb_state_unref(wsc->state);
        wsc->state = NULL;
    }

    if (wsc->keymap) {
        xkb_map_unref(wsc->keymap);
        wsc->keymap = NULL;
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

    wsc->keymap =
        xkb_map_new_from_string(wsc->xkb_context,
                map_str,
                XKB_KEYMAP_FORMAT_TEXT_V1,
                (xkb_keymap_compile_flags)0);

    munmap(map_str, size);
    close(fd);

    if (!wsc->keymap) {
        LOGW ("failed to compile keymap\n");
        return;
    }

    wsc->state = xkb_state_new(wsc->keymap);
    if (!wsc->state) {
        LOGW ("failed to create XKB state\n");
        xkb_map_unref(wsc->keymap);
        return;
    }

    wsc->control_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Control");
    wsc->alt_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Mod1");
    wsc->shift_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Shift");

    LOGW ("create _keysym2keycode\n");
    _init_keysym2keycode(wsc);
}

static void
_wsc_im_keyboard_key(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t time,
        uint32_t key,
        uint32_t state_w)
{
    struct weescim *wsc = (weescim*)data;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;
    xkb_keysym_t sym;
    enum wl_keyboard_key_state state = (wl_keyboard_key_state)state_w;

    if (!wsc || !wsc->state)
        return;

    code = key + 8;
    num_syms = xkb_key_get_syms(wsc->state, code, &syms);

    sym = XKB_KEY_NoSymbol;
    if (num_syms == 1)
        sym = syms[0];

    if (wsc->key_handler)
        (*wsc->key_handler)(wsc, serial, time, key, sym,
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
    struct weescim *wsc = (weescim*)data;
    struct wl_input_method_context *context = wsc->im_ctx;
    xkb_mod_mask_t mask;

    if (!wsc || !wsc->state)
        return;

    xkb_state_update_mask(wsc->state, mods_depressed,
            mods_latched, mods_locked, 0, 0, group);
    mask = xkb_state_serialize_mods(wsc->state,
            (xkb_state_component)(XKB_STATE_DEPRESSED | XKB_STATE_LATCHED));

    wsc->modifiers = 0;
    if (mask & wsc->control_mask)
        wsc->modifiers |= MOD_CONTROL_MASK;
    if (mask & wsc->alt_mask)
        wsc->modifiers |= MOD_ALT_MASK;
    if (mask & wsc->shift_mask)
        wsc->modifiers |= MOD_SHIFT_MASK;

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
_wsc_im_activate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    if (wsc->im_ctx)
        wl_input_method_context_destroy (wsc->im_ctx);

    if (wsc->preedit_str)
        free (wsc->preedit_str);

    wsc->preedit_str = strdup ("");
    wsc->content_hint = WL_TEXT_INPUT_CONTENT_HINT_NONE;
    wsc->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;
    wsc->cursor_pos = -1;

    if (wsc->language) {
        free (wsc->language);
        wsc->language = NULL;
    }

    if (wsc->surrounding_text) {
        free (wsc->surrounding_text);
        wsc->surrounding_text = NULL;
    }

    wsc->im_ctx = im_ctx;
    wl_input_method_context_add_listener (im_ctx, &wsc_im_context_listener, wsc);

    wsc->keyboard = wl_input_method_context_grab_keyboard(im_ctx);
    wl_keyboard_add_listener(wsc->keyboard, &wsc_im_keyboard_listener, wsc);

    if (wsc->language)
        wl_input_method_context_language (im_ctx, wsc->serial, wsc->language);

    wl_input_method_context_text_direction (im_ctx, wsc->serial, wsc->text_direction);

    wsc->context_changed = EINA_TRUE;
    isf_wsc_context_focus_in (wsc->wsc_ctx);
    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static void
_wsc_im_deactivate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    isf_wsc_context_input_panel_hide (wsc->wsc_ctx);
    isf_wsc_context_focus_out (wsc->wsc_ctx);

    if (wsc->im_ctx) {
        wl_input_method_context_destroy(wsc->im_ctx);
        wsc->im_ctx = NULL;
    }
}

static const struct wl_input_method_listener wsc_im_listener = {
    _wsc_im_activate,
    _wsc_im_deactivate
};

static void
_wsc_seat_handle_capabilities(void *data, struct wl_seat *seat,
                              uint32_t caps)
{
    struct weescim *wsc = (weescim*)data;
    if (!wsc) return;

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wsc->hw_kbd = true;
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wsc->hw_kbd = false;
    }
}

static const struct wl_seat_listener wsc_seat_listener = {
    _wsc_seat_handle_capabilities,
};

static void
_wsc_im_key_handler(struct weescim *wsc,
                    uint32_t serial, uint32_t time, uint32_t key, uint32_t sym,
                    enum wl_keyboard_key_state state)
{
}

static bool
_wsc_setup(struct weescim *wsc)
{
    Eina_Inlist *globals;
    struct wl_registry *registry;
    Ecore_Wl_Global *global;

    if (!wsc) return false;

    wsc->xkb_context = xkb_context_new((xkb_context_flags)0);
    if (wsc->xkb_context == NULL) {
        LOGW ("Failed to create XKB context\n");
        return false;
    }

    wsc->key_handler = _wsc_im_key_handler;

    wsc->wsc_ctx = isf_wsc_context_new ();
    if (!wsc->wsc_ctx) return false;

    wsc->wsc_ctx->ctx = wsc;

    get_language(&wsc->language);

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
    LOGD ("Adding wl_input_method listener");

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

    isf_wsc_context_add (wsc->wsc_ctx);

    return true;
}

static void
_wsc_free (struct weescim *wsc)
{
    if (!wsc) return;

    _fini_keysym2keycode (wsc);

    if (wsc->state) {
        xkb_state_unref (wsc->state);
        wsc->state = NULL;
    }

    if (wsc->keymap) {
        xkb_map_unref (wsc->keymap);
        wsc->keymap = NULL;
    }

    if (wsc->im_ctx) {
        wl_input_method_context_destroy (wsc->im_ctx);
        wsc->im_ctx = NULL;
    }

    if (wsc->wsc_ctx) {
        isf_wsc_context_del (wsc->wsc_ctx);
        wsc->wsc_ctx = NULL;
    }

    if (wsc->preedit_str) {
        free (wsc->preedit_str);
        wsc->preedit_str = NULL;
    }

    if (wsc->surrounding_text) {
        free (wsc->surrounding_text);
        wsc->surrounding_text = NULL;
    }

    if (wsc->language) {
        free (wsc->language);
        wsc->language = NULL;
    }

    isf_wsc_context_shutdown ();
}

int main (int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
    sleep(1);

    if (!_wsc_setup (&_wsc)) {
        _wsc_free (&_wsc);
        return 0;
    }

    ecore_main_loop_begin();

    _wsc_free (&_wsc);

    return 0;
}

/*
vi:ts=4:nowrap:expandtab
*/
