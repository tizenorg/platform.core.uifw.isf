/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

#if !defined (__ISF_SETUP_WIN_H)
#define __ISF_SETUP_WIN_H

#include <gtk/gtk.h>

using namespace scim;


#define ISE_SETUP_ICON_FILE        (SCIM_ICONDIR "/ISF_icon_option.png")
#define ISE_HELP_ICON_FILE         (SCIM_ICONDIR "/ISF_icon_help.png")
#define ISE_SETUP_ICON_T_FILE      (SCIM_ICONDIR "/ISF_icon_option_t.png")
#define ISE_HELP_ICON_T_FILE       (SCIM_ICONDIR "/ISF_icon_help_t.png")


class ISFSetupWin
{
    ISFSetupWin (const ISFSetupWin &);
    ISFSetupWin & operator = (const ISFSetupWin &);

public:
    ISFSetupWin (const ConfigPointer & config);
    ~ISFSetupWin ();

    void show_window (void);
private:
    GtkWidget *create_factory_list_view (void);
    GtkWidget *create_isf_setup_widget (void);
    void create_and_fill_factory_list_store (void);
    void update_factory_list_store (void);
    void on_isf_help_back_callback (GtkWidget * softkey, gpointer user_data);
    void create_ise_option_main (const char *str);
    void create_ise_help_main (gint type, char *name, char *title);

    static gboolean button_press_cb (GtkWidget *widget, GdkEventButton *event, gpointer data);
    static int  get_app_active_ise_number (String app_name);
    static void isf_setup_save_callback (GtkWidget *softkey, gpointer user_data);
    static void isf_setup_back_callback (GtkWidget *softkey, gpointer user_data);
    static void on_factory_enable_box_clicked (GtkCellRendererToggle *cell, gchar *arg1, gpointer data);
    static void app_selection_changed_callback (GtkComboBox *combo, gpointer data);
    static void on_language_button_clicked_callback (GtkButton *button, gpointer user_data);
};

#endif //  __ISF_SETUP_WIN_H

/*
vi:ts=4:ai:nowrap:expandtab
*/
