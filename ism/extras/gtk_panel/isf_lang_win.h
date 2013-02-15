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

#if !defined (__ISF_LANG_WIN_H)
#define __ISF_LANG_WIN_H

#include <gtk/gtk.h>

using namespace scim;

class ISFLangWin
{
    ISFLangWin (const ISFLangWin &);
    ISFLangWin & operator = (const ISFLangWin &);

public:
    ISFLangWin ();
    ~ISFLangWin ();

    void show_window (void);
private:

    GtkWidget *create_isf_lang_list_view (void);
    static void on_lang_enable_box_clicked (GtkCellRendererToggle *cell,
                                            gchar *arg1, gpointer data);
    static void isf_lang_save_callback (GtkWidget *softkey,
                                        gpointer   user_data);
    static void isf_lang_back_callback (GtkWidget *softkey,
                                        gpointer   user_data);

};

#endif // __ISF_LANG_WIN_H

/*
vi:ts=4:ai:nowrap:expandtab
*/

