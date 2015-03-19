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
struct weescim;

#define MOD_SHIFT_MASK      0x01
#define MOD_ALT_MASK        0x02
#define MOD_CONTROL_MASK    0x04

typedef void (*keyboard_input_key_handler_t)(struct weescim *wsc,
                                             uint32_t serial,
                                             uint32_t time, uint32_t key, uint32_t unicode,
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

    Eina_Bool context_changed;
    Eina_Bool hw_kbd;

    WSCContextISF *wsc_ctx;
};
static struct weescim _wsc                                  = {0};

static char *
insert_text(const char *text, uint32_t offset, const char *insert)
{
    int tlen = strlen(text), ilen = strlen(insert);
    char *new_text = (char*)malloc(tlen + ilen + 1);

    memcpy(new_text, text, offset);
    memcpy(new_text + offset, insert, ilen);
    memcpy(new_text + offset + ilen, text + offset, tlen - offset);
    new_text[tlen + ilen] = '\0';

    return new_text;
}

static void
wsc_commit_preedit(weescim *ctx)
{
    char *surrounding_text;

    if (!ctx->preedit_str ||
        strlen(ctx->preedit_str) == 0)
        return;

    wl_input_method_context_cursor_position(ctx->im_ctx,
                                            0, 0);

    wl_input_method_context_commit_string(ctx->im_ctx,
                                          ctx->serial,
                                          ctx->preedit_str);

    if (ctx->surrounding_text) {
        surrounding_text = insert_text(ctx->surrounding_text,
                                       ctx->surrounding_cursor,
                                       ctx->preedit_str);
        free(ctx->surrounding_text);
        ctx->surrounding_text = surrounding_text;
        ctx->surrounding_cursor += strlen(ctx->preedit_str);
    } else {
        ctx->surrounding_text = strdup(ctx->preedit_str);
        ctx->surrounding_cursor = strlen(ctx->preedit_str);
    }

    free(ctx->preedit_str);
    ctx->preedit_str = strdup("");
}

static void
wsc_send_preedit(weescim *ctx, int32_t cursor)
{
    uint32_t index = strlen(ctx->preedit_str);

    if (ctx->preedit_style)
        wl_input_method_context_preedit_styling(ctx->im_ctx,
                                                0,
                                                strlen(ctx->preedit_str),
                                                ctx->preedit_style);
    if (cursor > 0)
        index = cursor;

    wl_input_method_context_preedit_cursor(ctx->im_ctx, index);
    wl_input_method_context_preedit_string(ctx->im_ctx,
                                           ctx->serial,
                                           ctx->preedit_str,
                                           ctx->preedit_str);
}

bool wsc_context_surrounding_get(weescim *ctx, char **text, int *cursor_pos)
{
    if (!ctx)
        return false;

    *text = strdup (ctx->surrounding_text);
    *cursor_pos = ctx->surrounding_cursor;

    return true;
}

Ecore_IMF_Input_Panel_Layout wsc_context_input_panel_layout_get(weescim *ctx)
{
    Ecore_IMF_Input_Panel_Layout layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;

    switch (ctx->content_purpose) {
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DIGITS:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_NUMBER:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DATE:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_TIME:
        case WL_TEXT_INPUT_CONTENT_PURPOSE_DATETIME:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PHONE:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_URL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_EMAIL:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
            break;
        case WL_TEXT_INPUT_CONTENT_PURPOSE_PASSWORD:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
            break;
        default:
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
            break;
    }

    return layout;
}

bool wsc_context_input_panel_caps_lock_mode_get(weescim *ctx)
{
    if (ctx->content_hint & WL_TEXT_INPUT_CONTENT_HINT_UPPERCASE)
        return true;

    return false;
}

void wsc_context_delete_surrounding (weescim *ctx, int offset, int len)
{
    if (!ctx)
        return;

    wl_input_method_context_delete_surrounding_text(ctx->im_ctx, offset, len);
}

void wsc_context_commit_string(weescim *ctx, const char *str)
{
    if (ctx->preedit_str) {
        free(ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = strdup (str);
    wsc_commit_preedit(ctx);
}

void wsc_context_commit_preedit_string(weescim *ctx)
{
    char *preedit_str = NULL;
    int cursor_pos = 0;

    isf_wsc_context_preedit_string_get(ctx->wsc_ctx, &preedit_str, &cursor_pos);
    if (ctx->preedit_str) {
        free(ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = preedit_str;
    wsc_commit_preedit(ctx);
}

void wsc_context_send_preedit_string(weescim *ctx)
{
    char *preedit_str = NULL;
    int cursor_pos = 0;

    isf_wsc_context_preedit_string_get(ctx->wsc_ctx, &preedit_str, &cursor_pos);
    if (ctx->preedit_str) {
        free(ctx->preedit_str);
        ctx->preedit_str = NULL;
    }

    ctx->preedit_str = preedit_str;
    wsc_send_preedit(ctx, cursor_pos);
}

void wsc_context_send_key(weescim *ctx, uint32_t keysym, uint32_t modifiers, uint32_t time, bool press)
{
    uint32_t keycode;
    KeycodeRepository::iterator it = _keysym2keycode.find(keysym);

    if(it == _keysym2keycode.end())
        return;

    keycode = it->second;
    ctx->modifiers = modifiers;

    if (press) {
        if (ctx->modifiers) {
            wl_input_method_context_modifiers(ctx->im_ctx, ctx->serial,
                                              ctx->modifiers, 0, 0, 0);
        }

        wl_input_method_context_key(ctx->im_ctx, ctx->serial, time,
                                    keycode, WL_KEYBOARD_KEY_STATE_PRESSED);
    }
    else {
        wl_input_method_context_key(ctx->im_ctx, ctx->serial, time,
                                    keycode, WL_KEYBOARD_KEY_STATE_RELEASED);

        if (ctx->modifiers) {
            wl_input_method_context_modifiers(ctx->im_ctx, ctx->serial,
                                              0, 0, 0, 0);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of Wayland Input Method functions.
/////////////////////////////////////////////////////////////////////////////
static void
_wsc_im_ctx_surrounding_text(void *data, struct wl_input_method_context *im_ctx, const char *text, uint32_t cursor, uint32_t anchor)
{
    struct weescim *wsc = (weescim*)data;

    free (wsc->surrounding_text);
    wsc->surrounding_text = strdup (text);
    wsc->surrounding_cursor = cursor;
}

static void
_wsc_im_ctx_reset(void *data, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;

    isf_wsc_context_reset(wsc->wsc_ctx);
}

static void
_wsc_im_ctx_content_type(void *data, struct wl_input_method_context *im_ctx, uint32_t hint, uint32_t purpose)
{
    struct weescim *wsc = (weescim*)data;

    ISF_LOG ("im_context = %p hint = %d purpose = %d", im_ctx, hint, purpose);

    if (!wsc->context_changed)
        return;

    wsc->content_hint = hint;
    wsc->content_purpose = purpose;

    isf_wsc_context_input_panel_layout_set (wsc->wsc_ctx,
                                            wsc_context_input_panel_layout_get (wsc));

    caps_mode_check(wsc->wsc_ctx, EINA_TRUE, EINA_TRUE);

    //FIXME: process hint like layout.

    wsc->context_changed = EINA_FALSE;
}

static void
_wsc_im_ctx_invoke_action(void *data, struct wl_input_method_context *im_ctx, uint32_t button, uint32_t index)
{
    struct weescim *wsc = (weescim*)data;

    if (button != BTN_LEFT)
        return;

    wsc_context_send_preedit_string (wsc);
}

static void
_wsc_im_ctx_commit_state(void *data, struct wl_input_method_context *im_ctx, uint32_t serial)
{
    struct weescim *wsc = (weescim*)data;

    wsc->serial = serial;

    if (wsc->surrounding_text)
        ISF_LOG ("Surrounding text updated: %s", wsc->surrounding_text);

    if(wsc->language)
        wl_input_method_context_language (im_ctx, wsc->serial, wsc->language);

    wl_input_method_context_text_direction (im_ctx, wsc->serial, wsc->text_direction);
}

static void
_wsc_im_ctx_preferred_language(void *data, struct wl_input_method_context *im_ctx, const char *language)
{
    struct weescim *wsc = (weescim*)data;

    if (language && wsc->language && !strcmp (language, wsc->language))
        return;

    if (wsc->language) {
        free (wsc->language);
        wsc->language = NULL;
    }

    if (language) {
        wsc->language = strdup (language);
        ISF_LOG ("Language changed, new: '%s'", language);
    }
}

static const struct wl_input_method_context_listener wsc_im_context_listener = {
     _wsc_im_ctx_surrounding_text,
     _wsc_im_ctx_reset,
     _wsc_im_ctx_content_type,
     _wsc_im_ctx_invoke_action,
     _wsc_im_ctx_commit_state,
     _wsc_im_ctx_preferred_language,
};

static void
_init_keysym2keycode(struct weescim *wsc)
{
    uint32_t i = 0;
    uint32_t code;
    uint32_t num_syms;
    const xkb_keysym_t *syms;

    if (!wsc->state)
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
        fprintf(stderr, "failed to compile keymap\n");
        return;
    }

    wsc->state = xkb_state_new(wsc->keymap);
    if (!wsc->state) {
        fprintf(stderr, "failed to create XKB state\n");
        xkb_map_unref(wsc->keymap);
        return;
    }

    wsc->control_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Control");
    wsc->alt_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Mod1");
    wsc->shift_mask =
        1 << xkb_map_mod_get_index(wsc->keymap, "Shift");

    fprintf(stderr, "create _keysym2keycode\n");
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

    if (!wsc->state)
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

    if (!wsc->state)
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

    if (wsc->im_ctx)
        wl_input_method_context_destroy (wsc->im_ctx);

    if (wsc->preedit_str)
        free (wsc->preedit_str);

    wsc->preedit_str = strdup ("");
    wsc->content_hint = WL_TEXT_INPUT_CONTENT_HINT_NONE;
    wsc->content_purpose = WL_TEXT_INPUT_CONTENT_PURPOSE_NORMAL;

    free (wsc->language);
    wsc->language = NULL;

    free (wsc->surrounding_text);
    wsc->surrounding_text = NULL;

    wsc->im_ctx = im_ctx;
    wl_input_method_context_add_listener (im_ctx, &wsc_im_context_listener, wsc);

    wsc->keyboard = wl_input_method_context_grab_keyboard(im_ctx);
    wl_keyboard_add_listener(wsc->keyboard, &wsc_im_keyboard_listener, wsc);

    wsc->context_changed = EINA_TRUE;
    isf_wsc_context_focus_in (wsc->wsc_ctx);
    isf_wsc_context_input_panel_show (wsc->wsc_ctx);
}

static void
_wsc_im_deactivate(void *data, struct wl_input_method *input_method, struct wl_input_method_context *im_ctx)
{
    struct weescim *wsc = (weescim*)data;

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

static void
_wsc_setup(struct weescim *wsc)
{
    Eina_Inlist *globals;
    struct wl_registry *registry;
    Ecore_Wl_Global *global;

    wsc->xkb_context = xkb_context_new((xkb_context_flags)0);
    if (wsc->xkb_context == NULL) {
        fprintf(stderr, "Failed to create XKB context\n");
        return;
    }

    wsc->key_handler = _wsc_im_key_handler;

    wsc->wsc_ctx = isf_wsc_context_new ();
    wsc->wsc_ctx->ctx = wsc;

    get_language(&wsc->language);

    if (!(registry = ecore_wl_registry_get()))
        return;

    if (!(globals = ecore_wl_globals_get()))
        return;

    EINA_INLIST_FOREACH(globals, global) {
        if (strcmp (global->interface, "wl_input_method") == 0)
            wsc->im = (wl_input_method*)wl_registry_bind (registry, global->id, &wl_input_method_interface, 1);
        else if (strcmp (global->interface, "wl_seat") == 0)
            wsc->seat = (wl_seat*)wl_registry_bind (registry, global->id, &wl_seat_interface, 1);
    }

    /* Input method listener */
    ISF_LOG ("Adding wl_input_method listener");
    wl_input_method_add_listener (wsc->im, &wsc_im_listener, wsc);

    wl_seat_add_listener(wsc->seat, &wsc_seat_listener, wsc);

    isf_wsc_context_add (wsc->wsc_ctx);
}

static void
_wsc_free(struct weescim *wsc)
{
    _fini_keysym2keycode(wsc);

    if (wsc->state) {
        xkb_state_unref(wsc->state);
        wsc->state = NULL;
    }

    if (wsc->keymap) {
        xkb_map_unref(wsc->keymap);
        wsc->keymap = NULL;
    }

    if (wsc->im_ctx)
        wl_input_method_context_destroy (wsc->im_ctx);

    isf_wsc_context_del(wsc->wsc_ctx);
    isf_wsc_context_shutdown ();

    free (wsc->preedit_str);
    free (wsc->surrounding_text);
}

int main (int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
    sleep(1);

    _wsc_setup(&_wsc);

    ecore_main_loop_begin();

    _wsc_free(&_wsc);

    return 0;
}

/*
vi:ts=4:nowrap:expandtab
*/
