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

#define Uses_SCIM_EVENT
#define Uses_SCIM_CONFIG_PATH

#include <gdk/gdk.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_lang_win.h"
#include "isf_setup_utility.h"


enum {
    LANG_LIST_ENABLE = 0,
    LANG_LIST_NAME,
    LANG_LIST_NUM_COLUMS
};

extern MapStringVectorSizeT    _groups;
extern MapStringVectorString   _disabled_ise_map;
extern std::vector <String>    _disabled_langs;
extern MapStringVectorString   _disabled_ise_map_bak;
extern std::vector <String>    _disabled_langs_bak;

extern gboolean                _lang_selected_changed;

extern int                     _screen_width;
extern float                   _width_rate;
extern float                   _height_rate;

static GtkWidget              *_main_window     = 0;
static GtkListStore           *_lang_list_store = 0;
static unsigned int            _lang_num        = 0;


static gboolean lang_list_set_enable_func (GtkTreeModel *model,
                                           GtkTreePath  *path,
                                           GtkTreeIter  *iter,
                                           gpointer      data)
{
    gchar *lang = 0;
    gtk_tree_model_get (model, iter, LANG_LIST_NAME, &lang, -1);

    if (std::find (_disabled_langs.begin (), _disabled_langs.end (), String (lang)) ==_disabled_langs.end ())
        gtk_list_store_set (_lang_list_store, iter, LANG_LIST_ENABLE, true, -1);
    else
        gtk_list_store_set (_lang_list_store, iter, LANG_LIST_ENABLE, false, -1);
    if (lang) {
        g_free (lang);
        lang = 0;
    }

    return (gboolean)false;
}

ISFLangWin::ISFLangWin ()
{
#if HAVE_GCONF
    if (!_main_window) {
        _main_window = gtk_main_window_new (GTK_WIN_STYLE_DEFAULT);
        gtk_main_window_set_title_style (GTK_MAIN_WINDOW (_main_window), GTK_WIN_TITLE_STYLE_TEXT_ICON);

        GtkWidget *form = gtk_form_new (TRUE);
        gtk_form_set_title (GTK_FORM (form), "ISF Language");

        GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (form), scroll);
        gtk_widget_show (scroll);

        GtkWidget *isf_lang = create_isf_lang_list_view ();

        gtk_container_add (GTK_CONTAINER (scroll), isf_lang);
        gtk_widget_show (isf_lang);

#ifdef USING_ISF_SWITCH_BUTTON
        GtkWidget *softkey = GTK_WIDGET (gtk_form_get_softkey_bar (GTK_FORM (form)));
        gtk_softkey_bar_use_ise_switch (GTK_SOFTKEY_BAR (softkey), false);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY1,
                                     "Save", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (isf_lang_save_callback),
                                     (gpointer) NULL);

        gtk_softkey_bar_set_softkey (GTK_SOFTKEY_BAR (softkey), SOFTKEY4,
                                     "Exit", NULL,
                                     (SoftkeyActionType) SOFTKEY_CALLBACK,
                                     (gpointer) (isf_lang_back_callback),
                                     (gpointer) NULL);
#else
        gtk_form_add_softkey (GTK_FORM (form), "Save", NULL, SOFTKEY_CALLBACK, (void *)isf_lang_save_callback, NULL);
        gtk_form_add_softkey (GTK_FORM (form), "Exit", NULL, SOFTKEY_CALLBACK, (void *)isf_lang_back_callback, NULL);
#endif

        gtk_main_window_add_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));
        gtk_main_window_set_current_form (GTK_MAIN_WINDOW (_main_window), GTK_FORM (form));

        gtk_widget_show_all (form);
    }
#endif
}

ISFLangWin::~ISFLangWin ()
{
    if (_main_window) {
        gtk_widget_destroy (_main_window);
        _main_window = 0;
    }
    if (_lang_list_store) {
        delete[] _lang_list_store;
        _lang_list_store = 0;
    }
}

void ISFLangWin::show_window (void)
{
    gtk_tree_model_foreach (GTK_TREE_MODEL (_lang_list_store),
                            lang_list_set_enable_func,
                            NULL);

    gtk_widget_show_all (_main_window);
    gtk_window_present (GTK_WINDOW (_main_window));
    gtk_main ();
}


GtkWidget *ISFLangWin::create_isf_lang_list_view (void)
{
    GtkWidget         *view = NULL;

    GtkCellRenderer   *renderer = NULL;
    GtkTreeViewColumn *column = NULL;

    view = gtk_tree_view_new ();
    g_object_set (view, "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_NONE, NULL);
    gtk_widget_show (view);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), false);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), false);

    // Enable column
    column = gtk_tree_view_column_new ();
    renderer = gtk_cell_renderer_toggle_new ();
    gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), false);
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_cell_renderer_set_fixed_size (renderer, (int)(40*_width_rate), (int)(40*_height_rate));
    g_object_set (renderer, "radio", 0, NULL);
    g_object_set (renderer, "indicator-size", (gint)(26*(_width_rate < _height_rate ? _width_rate : _height_rate)), NULL);
    gtk_tree_view_column_set_attributes (column, renderer, "active", LANG_LIST_ENABLE, NULL);

    g_signal_connect (G_OBJECT (renderer), "toggled",
                      G_CALLBACK (on_lang_enable_box_clicked),
                      (gpointer) view);

    gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

    // Name column
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Name"));
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", LANG_LIST_NAME, NULL);
    gtk_cell_renderer_set_fixed_size (renderer, (int) (_screen_width - 40*_width_rate), (int)(40*_height_rate));
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    g_object_set (renderer, "xpad", 4, NULL);

    //gtk_cell_renderer_set_cell_font_size (renderer, GTK_CELL_RENDERER_NORMAL, 22);
    gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);
    // Create model.
    _lang_list_store = gtk_list_store_new (LANG_LIST_NUM_COLUMS, G_TYPE_BOOLEAN, G_TYPE_STRING);

    // Fill the model
    GtkTreeIter iter;
    String      lang_name;

    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        _lang_num++;
        lang_name = scim_get_language_name (it->first);
        gtk_list_store_append (_lang_list_store, &iter);
        gtk_list_store_set (_lang_list_store, &iter, LANG_LIST_NAME, lang_name.c_str (), -1);
    }

    gtk_tree_model_foreach (GTK_TREE_MODEL (_lang_list_store), lang_list_set_enable_func, NULL);

    gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (_lang_list_store));

    return view;
}

void ISFLangWin::on_lang_enable_box_clicked (GtkCellRendererToggle *cell,
                                             gchar *arg1, gpointer data)
{
    GtkTreeIter  iter;
    GtkTreePath *path   = 0;
    gboolean     enable = FALSE;

    std::vector<String>::iterator it;

    _lang_selected_changed = true;

    path = gtk_tree_path_new_from_string (arg1);
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_lang_list_store), &iter, path)) {
        gtk_tree_model_get (GTK_TREE_MODEL (_lang_list_store), &iter,
                            LANG_LIST_ENABLE, &enable, -1);

        gtk_list_store_set (GTK_LIST_STORE (_lang_list_store), &iter,
                            LANG_LIST_ENABLE, !enable, -1);
    }

    if (path) {
        gtk_tree_path_free (path);
        path = 0;
    }

    gchar *lang = 0;

    gtk_tree_model_get (GTK_TREE_MODEL (_lang_list_store), &iter,
                        LANG_LIST_NAME, &lang, -1);
    String lang_str (lang);
    if (lang) {
        g_free (lang);
        lang = 0;
    }

    it = std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_str);

    if (it != _disabled_langs.end ())
        _disabled_langs.erase (it);
    else
        _disabled_langs.push_back (lang_str);
}

void ISFLangWin::isf_lang_save_callback (GtkWidget *softkey,
                                         gpointer   user_data)
{
    if (_disabled_langs.size ()) {
        if (_disabled_langs.size () == _lang_num) {
            GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (_main_window),
                                                        GTK_DIALOG_MODAL,
                                                        GTK_MESSAGE_INFO,
                                                        GTK_BUTTONS_OK,
                                                        _("At least select one language!\n"));
            gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
            return;
        }
        _disabled_langs_bak.clear ();
        for (size_t j = 0; j < _disabled_langs.size (); j++) {
            _disabled_langs_bak.push_back (_disabled_langs[j]);
        }
    }

    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), _disabled_langs);

    std::vector<String> enable_langs;
    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        String lang_name;
        lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ()) {
            enable_langs.push_back (lang_name);
        }
    }
    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), enable_langs);
    scim_global_config_flush ();

    gtk_widget_hide (_main_window);
    gtk_main_quit ();
}

void ISFLangWin::isf_lang_back_callback (GtkWidget *softkey,
                                         gpointer   user_data)
{
    _lang_selected_changed = false;
    _disabled_langs.clear ();
    if (_disabled_langs_bak.size ()) {
        for (size_t j = 0; j < _disabled_langs_bak.size (); j++) {
            _disabled_langs.push_back (_disabled_langs_bak[j]);
        }
    }

    gtk_widget_hide (_main_window);
    gtk_main_quit ();
}


/*
vi:ts=4:nowrap:ai:expandtab
*/
