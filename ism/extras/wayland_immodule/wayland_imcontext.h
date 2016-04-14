/*
 * Copyright © 2012, 2013 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __WAYLAND_IM_CONTEXT_H_
#define __WAYLAND_IM_CONTEXT_H_

#include <Ecore_IMF.h>
#include <text-client-protocol.h>

typedef struct _WaylandIMContext WaylandIMContext;

// TIZEN_ONLY(20150708): Support back key
EAPI void register_key_handler                  ();
EAPI void unregister_key_handler                ();

EAPI void register_ecore_event_handler          ();
EAPI void unregister_ecore_event_handler        ();
//

EAPI void wayland_im_context_add                (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_del                (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_reset              (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_focus_in           (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_focus_out          (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_preedit_string_get (Ecore_IMF_Context    *ctx,
                                                 char                **str,
                                                 int                  *cursor_pos);
EAPI void wayland_im_context_preedit_string_with_attributes_get(Ecore_IMF_Context  *ctx,
                                                                char              **str,
                                                                Eina_List         **attr,
                                                                int                *cursor_pos);

EAPI void wayland_im_context_cursor_position_set(Ecore_IMF_Context    *ctx,
                                                 int                   cursor_pos);
EAPI void wayland_im_context_use_preedit_set    (Ecore_IMF_Context    *ctx,
                                                 Eina_Bool             use_preedit);
EAPI void wayland_im_context_client_window_set  (Ecore_IMF_Context    *ctx,
                                                 void                 *window);
EAPI void wayland_im_context_client_canvas_set  (Ecore_IMF_Context    *ctx,
                                                 void                 *canvas);
EAPI void wayland_im_context_show               (Ecore_IMF_Context    *ctx);
EAPI void wayland_im_context_hide               (Ecore_IMF_Context    *ctx);
EAPI Eina_Bool wayland_im_context_filter_event  (Ecore_IMF_Context    *ctx,
                                                 Ecore_IMF_Event_Type  type,
                                                 Ecore_IMF_Event      *event);
EAPI void wayland_im_context_cursor_location_set(Ecore_IMF_Context    *ctx,
                                                 int                   x,
                                                 int                   y,
                                                 int                   width,
                                                 int                   height);

EAPI void wayland_im_context_autocapital_type_set(Ecore_IMF_Context *ctx,
                                                  Ecore_IMF_Autocapital_Type autocapital_type);

EAPI void wayland_im_context_input_panel_layout_set(Ecore_IMF_Context *ctx,
                                                    Ecore_IMF_Input_Panel_Layout layout);

EAPI void wayland_im_context_input_mode_set(Ecore_IMF_Context *ctx,
                                            Ecore_IMF_Input_Mode input_mode);

EAPI void wayland_im_context_input_hint_set(Ecore_IMF_Context *ctx,
                                            Ecore_IMF_Input_Hints input_hints);

EAPI void wayland_im_context_input_panel_language_set(Ecore_IMF_Context *ctx,
                                                      Ecore_IMF_Input_Panel_Lang lang);

// TIZEN_ONLY(20150708): Support back key
EAPI Ecore_IMF_Input_Panel_State
wayland_im_context_input_panel_state_get   (Ecore_IMF_Context *ctx);

EAPI void
wayland_im_context_input_panel_return_key_type_set(Ecore_IMF_Context *ctx,
                                                   Ecore_IMF_Input_Panel_Return_Key_Type return_key_type);

EAPI void
wayland_im_context_input_panel_return_key_disabled_set(Ecore_IMF_Context *ctx,
                                                       Eina_Bool disabled);
//

EAPI void
wayland_im_context_input_panel_language_locale_get(Ecore_IMF_Context *ctx,
                                                   char **locale);

EAPI void
wayland_im_context_prediction_allow_set(Ecore_IMF_Context *ctx,
                                        Eina_Bool prediction);

// TIZEN_ONLY(20151221): Support input panel geometry
EAPI void
wayland_im_context_input_panel_geometry_get(Ecore_IMF_Context *ctx,
                                            int *x, int *y, int *w, int *h);

EAPI void
wayland_im_context_input_panel_imdata_set(Ecore_IMF_Context *ctx, const void *data, int length);

EAPI void
wayland_im_context_input_panel_imdata_get(Ecore_IMF_Context *ctx, void *data, int *length);
//
// TIZEN_ONLY(20160218): Support BiDi direction
EAPI void
wayland_im_context_bidi_direction_set(Ecore_IMF_Context *ctx, Ecore_IMF_BiDi_Direction bidi_direction);
//

WaylandIMContext *wayland_im_context_new        (struct wl_text_input_manager *text_input_manager);

extern int _ecore_imf_wayland_log_dom;

#endif

/* vim:ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0
 */
