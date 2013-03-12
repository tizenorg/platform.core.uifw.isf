/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
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

#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_PANEL_AGENT


#include <cstring>
#include <cstdio>
#include <gdk/gdk.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_setup_win.h"
#include "isf_setup_utility.h"
#include "scim_setup_module.h"
#include "isf_ise_setup_win.h"
#include "isf_help_win.h"
#include "isf_lang_win.h"

#define LIST_ICON_SIZE             20

enum {
    FACTORY_LIST_ENABLE = 0,
    FACTORY_LIST_ICON,
    FACTORY_LIST_NAME,
    FACTORY_LIST_MODULE_NAME,
    FACTORY_LIST_UUID,
    FACTORY_LIST_TYPE,
    FACTORY_LIST_OPTION_PIX,
    FACTORY_LIST_HELP_PIX,
    FACTORY_LIST_NUM_COLUMNS
};

static SetupUI                *_setup_ui               = 0;
static ISFHelpWin             *_ise_help_win           = 0;
static ISFLangWin             *_isf_lang_win           = 0;
static GtkListStore           *_app_list_store         = 0;
static GtkListStore           *_factory_list_store     = 0;
static GtkTreePath            *_app_combo_default_path = 0;
static GtkWidget              *_combo_app              = 0;
static GtkWidget              *_main_window            = 0;
static int                     _display_selected_items = 0;
static ConfigPointer           _config;

extern std::vector<String>    _isf_app_list;
extern MapStringVectorSizeT   _groups;
extern std::vector < String > _uuids;
extern std::vector < String > _names;
extern std::vector < String > _module_names;
extern std::vector < String > _langs;
extern std::vector < String > _icons;
extern std::vector <TOOLBAR_MODE_T> _modes;
extern MapStringString        _keyboard_ise_help_map;
extern MapStringVectorString  _disabled_ise_map;
extern std::vector <String>   _disabled_langs;
extern MapStringVectorString  _disabled_ise_map_bak;
extern std::vector <String>   _disabled_langs_bak;
extern gboolean               _lang_selected_changed;
extern gboolean               _ise_list_changed;
extern gboolean               _setup_enable_changed;
extern int                    _screen_width;
extern float                  _width_rate;
extern float                  _height_rate;


ISFSetupWin::ISFSetupWin (const ConfigPointer & config)
{
    _config = config;
#if HAVE_GCONF
    if (!_main_window) {
        _main_window = gtk_main_window_new (GTK_WIN_STYLE_DEFAULT);
        gtk_main_window_set_title_style (GTK_MAIN_WINDOW (_main_window), GTK_WIN_TITLE_STYLE_TEXT_ICON);

        GtkWidget *form = gtk_form_new (TRUE);
        gtk_form_set_title (GTK_FORM (form), "ISF Setup");

        GtkWidget *isf_setup = create_isf_setup_widget ();

        gtk_container_add (GTK_CONTAINER (form), isf_setup);

#ifdef USING_ISF_SWITCH_BUTTON
        GtkWidget *softkey = GTK_WIDGET (gtk_form_get_softkey_bar (GTK_FORM (form)));
        gtk_softkey_bar_use_ise_switch (GTK_SOFTKEY_BAR (softkey), false);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY1,
                                     "Save", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (isf_setup_save_callback),
                                     (gpointer) NULL);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY4,
                                     "Exit", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (isf_setup_back_callback),
                                     (gpointer) NULL);
#else
        gtk_form_add_softkey (GTK_FORM (form), "Save", NULL, SOFTKEY_CALLBACK, (void *)isf_setup_save_callback, NULL);
        gtk_form_add_softkey (GTK_FORM (form), "Exit", NULL, SOFTKEY_CALLBACK, (void *)isf_setup_back_callback, NULL);
#endif

        gtk_main_window_add_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));
        gtk_main_window_set_current_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));

        gtk_widget_show_all (form);
    }
#endif
}

ISFSetupWin::~ISFSetupWin ()
{
    if (_main_window) {
        gtk_widget_destroy (_main_window);
        _main_window = 0;
    }
    if (_setup_ui) {
        delete _setup_ui;
        _setup_ui = 0;
    }
    if (_ise_help_win) {
        delete _ise_help_win;
        _ise_help_win = 0;
    }
    if (_isf_lang_win) {
        delete _isf_lang_win;
        _isf_lang_win = 0;
    }
}

void ISFSetupWin::show_window (void)
{
    GtkTreeIter it;
    gtk_tree_model_get_iter (GTK_TREE_MODEL (_app_list_store), &it,
                             _app_combo_default_path);
    gtk_combo_box_set_active_iter ((GtkComboBox *) _combo_app, &it);

    update_factory_list_store ();

    gtk_widget_show_all (_main_window);
    gtk_window_present (GTK_WINDOW (_main_window));
    gtk_main ();
}


void ISFSetupWin::app_selection_changed_callback (GtkComboBox * combo, gpointer data)
{
    ISFSetupWin *obj = (ISFSetupWin *)data;
    obj->update_factory_list_store ();
}


void ISFSetupWin::on_language_button_clicked_callback (GtkButton * button,
                                                       gpointer user_data)
{
    ISFSetupWin *obj = (ISFSetupWin *)user_data;

    if (!_isf_lang_win)
        _isf_lang_win = new ISFLangWin ();
    if (_isf_lang_win)
        _isf_lang_win->show_window ();

    if (_lang_selected_changed) {
        _ise_list_changed = true;
        obj->update_factory_list_store ();
    }
}

GtkWidget *ISFSetupWin::create_isf_setup_widget (void)
{
    GtkWidget *language_button = NULL, *hbox = NULL, *vbox = NULL, *scroll = NULL;

    GtkTreeIter      iter, it;
    GtkCellRenderer *cell = NULL;

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);
    gtk_widget_set_size_request (hbox, _screen_width, (int)(40*_height_rate));
    gtk_container_set_border_width (GTK_CONTAINER (hbox),(int)( 5*_height_rate));

    // Create application list combox
    _combo_app = gtk_combo_box_new ();
    cell = gtk_cell_renderer_text_new ();
    _app_list_store = gtk_list_store_new (1, G_TYPE_STRING);

    for (unsigned int i = 0; i < _isf_app_list.size (); i++) {
        gtk_list_store_prepend (_app_list_store, &iter);
        gtk_list_store_set (_app_list_store, &iter, 0, _isf_app_list[_isf_app_list.size () - i - 1].c_str (), -1);
    }

    _app_combo_default_path = gtk_tree_model_get_path (GTK_TREE_MODEL (_app_list_store), &iter);

    gtk_cell_layout_pack_start ((GtkCellLayout *) _combo_app, cell, TRUE);
    gtk_cell_layout_set_attributes ((GtkCellLayout *) _combo_app, cell, "text", 0, NULL);
    gtk_combo_box_set_model ((GtkComboBox *) _combo_app, (GtkTreeModel *) _app_list_store);
    gtk_tree_model_get_iter (GTK_TREE_MODEL (_app_list_store), &it, _app_combo_default_path);
    gtk_combo_box_set_active_iter ((GtkComboBox *) _combo_app, &it);
//    gtk_widget_show (_combo_app);  //hide the app list combox temp
    g_signal_connect ((gpointer) _combo_app, "changed",
                      G_CALLBACK (app_selection_changed_callback), (gpointer)this);

    // Create language button
    language_button = gtk_button_new_with_label ("Language");
    GtkStyle *style1 = gtk_rc_get_style (language_button);
    GtkWidget *newlabel = gtk_label_new ("Language");
    if (newlabel) {
        GtkWidget * old = gtk_bin_get_child (GTK_BIN (language_button));
        if (old)
            gtk_container_remove (GTK_CONTAINER (language_button), old);
        gtk_container_add (GTK_CONTAINER (language_button), newlabel);
    }
    if(style1)
    {
        gtk_widget_modify_font (language_button, style1->font_desc);
    }
    gtk_widget_show (language_button);
    gtk_box_pack_start (GTK_BOX (hbox), language_button, true, true, 0);
    g_signal_connect ((gpointer) language_button, "clicked",
                      G_CALLBACK (on_language_button_clicked_callback), (gpointer)this);

    // Create ISE list view
    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add (GTK_CONTAINER (vbox), scroll);
    gtk_widget_show (scroll);

    GtkWidget *view = create_factory_list_view ();
    create_and_fill_factory_list_store ();
    gtk_tree_view_set_model (GTK_TREE_VIEW  (view), GTK_TREE_MODEL (_factory_list_store));
    gtk_container_add (GTK_CONTAINER (scroll), view);
    gtk_widget_show (view);

    return vbox;
}

// Show all ISEs in the view
void ISFSetupWin::create_and_fill_factory_list_store (void)
{
    _factory_list_store = gtk_list_store_new (FACTORY_LIST_NUM_COLUMNS, G_TYPE_BOOLEAN,
                                              GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING,
                                              G_TYPE_STRING, G_TYPE_INT, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF);
    update_factory_list_store ();
}

void ISFSetupWin::update_factory_list_store (void)
{
    GtkTreeIter  iter;

    GdkPixbuf   *icon = NULL, *icon2 = NULL;
    GtkWidget   *pix = NULL, *pix2 = NULL;

    _display_selected_items = 0;
    pix   = gtk_image_new_from_file (ISE_SETUP_ICON_FILE);
    icon  = gtk_image_get_pixbuf (GTK_IMAGE (pix));
    pix2  = gtk_image_new_from_file (ISE_HELP_ICON_FILE);
    icon2 = gtk_image_get_pixbuf (GTK_IMAGE (pix2));

    String _app_name = gtk_combo_box_get_active_text ((GtkComboBox *) _combo_app);

    gtk_list_store_clear (_factory_list_store);
    std::vector<String>  f_show;
    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        String lang_name;
        lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                // protect from add the same ise more than once in case of multiple langs.-->f_show list
                if (std::find (f_show.begin (), f_show.end (), _uuids[it->second[i]]) == f_show.end ()) {
                    gtk_list_store_append (_factory_list_store, &iter);
                    gtk_list_store_set (_factory_list_store, &iter,
                                        FACTORY_LIST_NAME,
                                        _names[it->second[i]].c_str (),
                                        FACTORY_LIST_MODULE_NAME,
                                        _module_names[it->second[i]].c_str (),
                                        FACTORY_LIST_UUID,
                                        _uuids[it->second[i]].c_str (),
                                        FACTORY_LIST_TYPE, (gint)_modes[it->second[i]],
                                        FACTORY_LIST_OPTION_PIX, icon,
                                        FACTORY_LIST_HELP_PIX,icon2,
                                        -1);
                    f_show.push_back (_uuids[it->second[i]]);


                    if (std::find (_disabled_ise_map[_app_name].begin (), _disabled_ise_map[_app_name].end (), _uuids[it->second[i]])
                        ==_disabled_ise_map[_app_name].end ()) {
                        gtk_list_store_set (_factory_list_store, &iter, FACTORY_LIST_ENABLE, true, -1);
                        _display_selected_items++;
                    } else {
                        gtk_list_store_set (_factory_list_store, &iter, FACTORY_LIST_ENABLE, false, -1);
                    }
                }
            }
        }
    }

    if (icon) {
        gdk_pixbuf_unref (icon);
        icon = 0;
    }
    if (icon2) {
        gdk_pixbuf_unref (icon2);
        icon2 = 0;
    }
}

GtkWidget * ISFSetupWin::create_factory_list_view (void)
{
    GtkCellRenderer*     renderer = NULL;
    GtkTreeViewColumn*   column   = NULL;

    GtkWidget *tv = gtk_tree_view_new ();
    g_object_set (tv, "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_NONE, NULL);
    column = gtk_tree_view_column_new ();

    //toggle renderer
    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "activatable", TRUE, NULL);
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
    gtk_cell_renderer_set_fixed_size (renderer, _screen_width/8, (gint)(40*_height_rate));
    g_object_set (renderer, "xalign", 0.5, NULL);
    g_object_set (renderer, "radio", 0, NULL);
    g_object_set (renderer, "indicator-size", (gint)(26*(_width_rate < _height_rate ? _width_rate : _height_rate)), NULL);
    gtk_tree_view_column_set_attributes (column, renderer, "active", FACTORY_LIST_ENABLE, NULL);
    g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (on_factory_enable_box_clicked), (gpointer) NULL);

    // text renderer
    renderer =  gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", FACTORY_LIST_NAME, NULL);
    gtk_cell_renderer_set_fixed_size (renderer, (gint)(_screen_width*5/8), (gint)(40*_height_rate));
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    g_object_set (renderer, "xalign", 0.0, NULL);
#if HAVE_GCONF
    gtk_cell_renderer_set_cell_flag (tv, renderer, GTK_CELL_RENDERER_MAINCELL);
#endif
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    // pixbuf render
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_pixbuf_new ();
#if HAVE_GCONF
    renderer->cell_type = GTK_CELL_RENDERER_ICONCELL;
#endif
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", FACTORY_LIST_HELP_PIX, NULL);
    gtk_cell_renderer_set_fixed_size (renderer, _screen_width/11, _screen_width/10);
    g_object_set (renderer, "xalign", 0.5, NULL);
    g_object_set (renderer, "yalign", 0.5, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    // pixbuf render
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_pixbuf_new ();
#if HAVE_GCONF
    renderer->cell_type = GTK_CELL_RENDERER_ICONCELL;
#endif
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", FACTORY_LIST_OPTION_PIX, NULL);
    gtk_cell_renderer_set_fixed_size (renderer, _screen_width/11, _screen_width/10);
    g_object_set (renderer, "yalign", 0.5, NULL);
    g_object_set (renderer, "xalign", 0.5, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

#if HAVE_GCONF
    gtk_tree_view_set_special_row_selection_indicator (GTK_TREE_VIEW (tv), TRUE);
#endif
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), false);
    g_signal_connect (GTK_TREE_VIEW (tv), "button_press_event", G_CALLBACK (button_press_cb), this);
    gtk_widget_show_all (tv);

    return tv;
}

gboolean ISFSetupWin::button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    ISFSetupWin  *obj   = (ISFSetupWin *)data;
    GtkTreePath  *path  = NULL;
    GtkTreeModel *model = GTK_TREE_MODEL (_factory_list_store);
    GtkTreeIter   iter;
    gchar        *module_name = NULL, *title = NULL;
    int           type;
    int           cell_x, cell_y;

    GtkTreeViewColumn *column = NULL;
    if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                        (gint)event->x, (gint)event->y,
                                        &path, &column, &cell_x, &cell_y))
        return (gboolean)FALSE;

    if (path != NULL) {
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);
        gtk_tree_model_get (model, &iter, FACTORY_LIST_MODULE_NAME,
                            &module_name, FACTORY_LIST_NAME, &title, FACTORY_LIST_TYPE, &type, -1);

        GtkTreeView *tree_view = NULL;
        tree_view = GTK_TREE_VIEW (widget);
        if (event->window == gtk_tree_view_get_bin_window (tree_view)) {
            if (event->type == GDK_BUTTON_PRESS) {
                if (column == gtk_tree_view_get_column (tree_view, 1)) {
                    obj->create_ise_help_main (type, module_name, title);
                    return (gboolean)TRUE;
                } else if (column == gtk_tree_view_get_column (tree_view, 2)) {
                    obj->create_ise_option_main (module_name);
                    return (gboolean)TRUE;
                }
            }
        }
    }
    return (gboolean)FALSE;
}

void ISFSetupWin::create_ise_option_main (const char *str)
{
    if (!_setup_ui)
        _setup_ui = new SetupUI (_config);
    if (!_setup_ui)
        return;

    String setup_module_name;
    setup_module_name = utf8_wcstombs (utf8_mbstowcs (str)) + String ("-imengine-setup");

    SetupModule *setup_module = new SetupModule (setup_module_name);

    if (setup_module) {
        _setup_ui->add_module (setup_module);
        delete setup_module;
    }
}


void ISFSetupWin::create_ise_help_main (gint type, char *name, char *title)
{
    if (!_ise_help_win)
        _ise_help_win = new ISFHelpWin ();
    if (!_ise_help_win)
        return;

    TOOLBAR_MODE_T mode = TOOLBAR_MODE_T (type);
    // for mutually exclusive ISEs
    if (TOOLBAR_HELPER_MODE == mode) {
        String help_filename, help_line;

        if (TOOLBAR_HELPER_MODE == mode) {
            help_filename = String (SCIM_MODULE_PATH) + String (SCIM_PATH_DELIM_STRING)
                            + String (SCIM_BINARY_VERSION) + String (SCIM_PATH_DELIM_STRING) + String ("Helper")
                            + String (SCIM_PATH_DELIM_STRING) + String (name)
                            + String (".help");
        }

        std::ifstream helpfile (help_filename.c_str ());
        if (!helpfile) {
            std::cerr << "Can't open help file : " << help_filename << "!!!!!!!!!! \n";

            help_line = String (_("Input Service Framework ")) +
                        String (_("\n(C) 2008 SAMSUNG")) +
                        String (_("\n\n Help file is needed!!"));
        } else {
            /* FIXME -- 256 char per line for now -- */
            char str[256];
            while (helpfile.getline (str, sizeof (str))) {
                help_line = help_line + String (str) + String("\n");
            }
            helpfile.close ();
        }
        _ise_help_win->show_help (title, (char *)help_line.c_str ());
    } else {
        // FIXME
        _ise_help_win->show_help (title, (char *)_keyboard_ise_help_map[String(name)].c_str ());
    }
}

// Check before back
int ISFSetupWin::get_app_active_ise_number (String app_name)
{
    if (app_name.length () <= 0)
        return 0;

    int         nCount = 0;
    String      lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (std::find (_disabled_ise_map_bak[app_name].begin (), _disabled_ise_map_bak[app_name].end (), _uuids[it->second[i]])
                        ==_disabled_ise_map_bak[app_name].end ()) {
                    nCount++;
                }
            }
        }
    }
    return nCount;
}

void ISFSetupWin::isf_setup_save_callback (GtkWidget *softkey, gpointer user_data)
{
    if (_display_selected_items == 0) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (_main_window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_INFO,
                                                    GTK_BUTTONS_OK,
                                                    _("At least select one ISE!\n"));
        gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return;
    }

    for (size_t i = 0; i < _isf_app_list.size (); i++) {
        String app = _isf_app_list[i];
        _disabled_ise_map_bak[app].clear ();
        if (_disabled_ise_map[app].size ()) {
            for (size_t j = 0; j < _disabled_ise_map[app].size (); j++) {
                _disabled_ise_map_bak[app].push_back (_disabled_ise_map[app][j]);
            }
        }
    }

    if (_setup_enable_changed) {
        for (size_t i = 0; i < _isf_app_list.size (); i++) {
            String app = _isf_app_list[i];
            scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_IMENGINE_FACTORIES
                                              "/") + _isf_app_list[i], _disabled_ise_map[app]);
        }
        scim_global_config_flush ();
    }
    gtk_widget_hide (_main_window);
    gtk_main_quit ();
}

void ISFSetupWin::isf_setup_back_callback (GtkWidget *softkey, gpointer user_data)
{
    if (get_app_active_ise_number (_isf_app_list[0]) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (_main_window),
                                                    GTK_DIALOG_MODAL,
                                                    GTK_MESSAGE_INFO,
                                                    GTK_BUTTONS_OK,
                                                   _("Please select ISE, then save.\n"));
        gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return;
    }
    _setup_enable_changed = false;
    for (size_t i = 0; i < _isf_app_list.size (); i++) {
        String app = _isf_app_list[i];
        _disabled_ise_map[app].clear ();
        if (_disabled_ise_map_bak[app].size ()) {
            for (size_t j = 0; j < _disabled_ise_map_bak[app].size (); j++) {
                _disabled_ise_map[app].push_back (_disabled_ise_map_bak[app][j]);
            }
        }
    }

    gtk_widget_hide (_main_window);
    gtk_main_quit ();
}

void ISFSetupWin::on_factory_enable_box_clicked (GtkCellRendererToggle *cell, gchar *arg1, gpointer data)
{
    GtkTreePath *path = 0;
    GtkTreeIter  iter;
    gboolean     enable;

    std::vector < String >::iterator it;

    _setup_enable_changed = true;

    path = gtk_tree_path_new_from_string (arg1);
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_factory_list_store), &iter, path)) {
        gtk_tree_model_get (GTK_TREE_MODEL (_factory_list_store), &iter,
                            FACTORY_LIST_ENABLE, &enable, -1);
        gtk_list_store_set (GTK_LIST_STORE (_factory_list_store), &iter,
                            FACTORY_LIST_ENABLE, !enable, -1);
        if (enable)
            _display_selected_items--;
        else
            _display_selected_items++;
    }
    if (path) {
        gtk_tree_path_free (path);
        path = 0;
    }

    gchar  *cuuid = 0;
    String  uuid;
    gtk_tree_model_get (GTK_TREE_MODEL (_factory_list_store),
                        &iter, FACTORY_LIST_UUID,
                        &cuuid, -1);
    uuid = (String)cuuid;
    if (cuuid) {
        g_free (cuuid);
        cuuid = 0;
    }
    String _app_name = gtk_combo_box_get_active_text ((GtkComboBox *) _combo_app);
    it = std::find (_disabled_ise_map[_app_name].begin (), _disabled_ise_map[_app_name].end (), uuid);

    if (it != _disabled_ise_map[_app_name].end ())
        _disabled_ise_map[_app_name].erase (it);
    else
        _disabled_ise_map[_app_name].push_back (uuid);
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
