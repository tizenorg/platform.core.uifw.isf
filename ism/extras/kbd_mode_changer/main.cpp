/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Wonkeun Oh <wonkeun.oh@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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

#include "scim_private.h"
#include "scim_visibility.h"
#if HAVE_VCONF
#include <vconf.h>
#endif
#include "main.h"

static bool app_create (void *user_data)
{
    LOGI ("app create\n");
    return true;
}

static void app_control (app_control_h app_control, void *user_data)
{
    LOGI ("%s\n", __func__);

    LOGI ("isf_extra_hwkbd_module start\n");

    /* Toggle input mode */
#if HAVE_VCONF
    int val = 1;
    if (vconf_get_bool (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, &val) != 0)
        LOGW ("Failed to get vconf key\n");

    if (vconf_set_bool (VCONFKEY_ISF_HW_KEYBOARD_INPUT_DETECTED, !val) != 0)
        LOGW ("Failed to set vconf key\n");
    else
        LOGI ("Succeeded to set vconf key\n");
#endif

    ui_app_exit ();
}

static void app_terminate (void *user_data)
{
    LOGI ("app terminated\n");
}

EXAPI int main (int argc, char *argv [])
{
    ui_app_lifecycle_callback_s event_callback = {0,};

    event_callback.create = app_create;
    event_callback.terminate = app_terminate;
    event_callback.app_control = app_control;

    LOGI ("start org.tizen.isf-kbd-mode-changer\n");

    return ui_app_main (argc, argv, &event_callback, NULL);
}
