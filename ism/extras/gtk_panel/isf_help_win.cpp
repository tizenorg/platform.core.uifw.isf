/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
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

#include <gtk/gtk.h>
#include "isf_help_win.h"

static GtkWidget *_main_window  = 0;
static GtkWidget *_content_box  = 0;
static GtkWidget *_help_label   = 0;
static GtkWidget *_form         = 0;


extern int        _screen_width;
extern float      _width_rate;
extern float      _height_rate;

ISFHelpWin::ISFHelpWin ()
{
#if HAVE_GCONF
    if (!_main_window) {
        _main_window = gtk_main_window_new (GTK_WIN_STYLE_DEFAULT);
        gtk_main_window_set_title_style (GTK_MAIN_WINDOW (_main_window), GTK_WIN_TITLE_STYLE_TEXT_ICON);

        _form = gtk_form_new (TRUE);
        gtk_form_set_title (GTK_FORM (_form), "Help");
        GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (_form), scroll);

        _content_box = gtk_vbox_new (false, 0);
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), _content_box);

#ifdef USING_ISF_SWITCH_BUTTON
        GtkWidget *softkey = GTK_WIDGET (gtk_form_get_softkey_bar (GTK_FORM (_form)));
        gtk_softkey_bar_use_ise_switch (GTK_SOFTKEY_BAR (softkey), false);
        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY4,
                                     "Exit", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (on_isf_help_back_callback),
                                     (gpointer) this);
#else
        gtk_form_add_softkey(GTK_FORM(_form), "Exit", NULL, SOFTKEY_CALLBACK, (void *)on_isf_help_back_callback, NULL);

#endif

        gtk_main_window_add_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (_form));
        gtk_main_window_set_current_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (_form));
        gtk_widget_show_all (_form);
    }
#endif
}


ISFHelpWin::~ISFHelpWin ()
{
    if (_main_window) {
        gtk_widget_destroy (_main_window);
        _main_window = 0;
    }
    if (_content_box) {
        gtk_widget_destroy (_content_box);
        _content_box = 0;
    }
    if (_help_label) {
        gtk_widget_destroy (_help_label);
        _help_label = 0;
    }
    if (_form) {
        gtk_widget_destroy (_form);
        _form = 0;
    }
}

void ISFHelpWin::show_help (const char *title, const char *help_info)
{
    if (!_help_label) {
        _help_label = gtk_label_new (help_info);
        gtk_box_pack_start (GTK_BOX (_content_box), _help_label, true, true, 0);
#if HAVE_GCONF
        gtk_form_set_title (GTK_FORM (_form), (gchar *)title);
#endif
        gtk_widget_show_all (_form);
        gtk_widget_show_all (_main_window);
        gtk_window_present (GTK_WINDOW (_main_window));
        gtk_main ();
    } else {
        gtk_window_present (GTK_WINDOW (_main_window));
    }
}

void ISFHelpWin::on_isf_help_back_callback (GtkWidget *softkey,
                                            gpointer   user_data)
{
    gtk_widget_hide (_main_window);
    if (_help_label) {
        gtk_widget_destroy (_help_label);
        _help_label = 0;
    }
    gtk_main_quit ();
}


/*
vi:ts=4:nowrap:ai:expandtab
*/
