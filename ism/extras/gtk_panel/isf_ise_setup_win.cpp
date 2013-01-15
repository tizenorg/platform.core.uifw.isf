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

#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_PANEL_AGENT


#include <cstring>
#include <cstdio>
#include <gdk/gdk.h>
#include <sys/time.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_setup_module.h"
#include "isf_setup_utility.h"
#include "isf_ise_setup_win.h"


static gboolean       _is_module_running = false;
static GtkWidget     *_main_window       = 0;
static GtkWidget     *_work_area         = 0;
static GtkWidget     *_current_widget    = 0;
static SetupModule   *_current_module    = 0;
static ConfigPointer  _config;

extern PanelAgent    *_panel_agent;
extern int            _screen_width;
extern float          _width_rate;
extern float          _height_rate;

SetupUI::SetupUI (const ConfigPointer & config)
{
    _config = config;

    create_main_ui ();
}

SetupUI::~SetupUI ()
{
    if (_current_widget) {
        gtk_widget_destroy (_current_widget);
        _current_widget = 0;
    }
    if (_work_area) {
        gtk_widget_destroy (_work_area);
        _work_area = 0;
    }
    if (_main_window) {
        gtk_widget_destroy (_main_window);
        _main_window = 0;
    }
}

bool SetupUI::add_module (SetupModule *module)
{
    if (_is_module_running) {
        std::cerr << "One ISE Setup is running...\n";
        return false;
    } else if (!module || !module->valid ()) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (_main_window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_INFO,
                                                    GTK_BUTTONS_OK,
                                                    _("No setup for this ISE!\n"));
        gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return false;
    }

    _is_module_running = true;

    GtkWidget *module_widget = module->create_ui ();
    String module_label      = module->get_name ();
    String module_category   = module->get_category ();

    if (!module_widget || !module_label.length () || !module_category.length ())
        return false;

    if (!_config.null ()) {
        module->load_config (_config);
    }

    _current_widget = module_widget;
    _current_module = module;

    gtk_box_pack_start (GTK_BOX (_work_area), _current_widget, TRUE, TRUE, 0);
    gtk_widget_show_all (_work_area);
    run ();

    return true;
}

void SetupUI::run (void)
{
    if (_main_window) {
        gtk_widget_show_all (_main_window);
        gtk_window_present (GTK_WINDOW (_main_window));
    }
    gtk_main ();

    SCIM_DEBUG_MAIN(1) << "exit SetupUI::run ()\n";
}

void SetupUI::create_main_ui (void)
{
#if HAVE_GCONF
    if (!_main_window) {
        _main_window = gtk_main_window_new (GTK_WIN_STYLE_DEFAULT);
        gtk_main_window_set_title_style (GTK_MAIN_WINDOW (_main_window), GTK_WIN_TITLE_STYLE_TEXT_ICON);

        GtkWidget *form = gtk_form_new (TRUE);
        gtk_form_set_title (GTK_FORM (form), "ISE Option");
        gtk_widget_show (form);

        GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (form), scroll);
        gtk_widget_show (scroll);

        _work_area = gtk_vbox_new (false, 0);
        gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scroll), _work_area);

#ifdef USING_ISF_SWITCH_BUTTON
        GtkWidget *softkey = GTK_WIDGET (gtk_form_get_softkey_bar (GTK_FORM (form)));

        gtk_softkey_bar_use_ise_switch (GTK_SOFTKEY_BAR(softkey), false);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY1,
                                     "Save", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (softkey_ok_clicked_cb),
                                     (gpointer) this);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY4,
                                     "Exit", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (softkey_exit_clicked_cb),
                                     (gpointer) this);
#else
        gtk_form_add_softkey (GTK_FORM (form), "Save", NULL, SOFTKEY_CALLBACK, (void *)softkey_ok_clicked_cb, NULL);
        gtk_form_add_softkey (GTK_FORM (form), "Exit", NULL, SOFTKEY_CALLBACK, (void *)softkey_exit_clicked_cb, NULL);

#endif

        gtk_main_window_add_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));
        gtk_main_window_set_current_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));

        g_signal_connect (G_OBJECT (_main_window), "delete_event", G_CALLBACK (hide), this);
    }
#endif
}


void SetupUI::save_and_exit (gpointer user_data)
{
    _current_module->save_config (_config);
    _config->flush ();

    struct timeval start, end;
    int    timeuse = 0;

    gettimeofday (&start, NULL);

    _config->reload ();
    _panel_agent->reload_config ();
    gettimeofday (&end, NULL);
    timeuse = 1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    printf (mzc_red "ISF config reload time : %d usec" mzc_normal ".\n", timeuse);

    hide (user_data);
}

void SetupUI::exit_without_save (gpointer user_data)
{
    hide (user_data);
}

void SetupUI::softkey_ok_clicked_cb (GtkWidget * softkey, gpointer user_data)
{
    save_and_exit (user_data);
}

void SetupUI::softkey_exit_clicked_cb (GtkWidget * softkey, gpointer user_data)
{
    exit_without_save (user_data);
}


void SetupUI::hide (gpointer user_data)
{
    _current_module->unload ();
    _is_module_running = false;
    if (_current_widget) {
        gtk_widget_destroy (_current_widget);
        _current_widget = 0;
    }
    gtk_widget_hide (_main_window);
    gtk_main_quit ();
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
