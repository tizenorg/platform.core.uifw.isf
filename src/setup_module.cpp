/*
 * Copyright (c) 2012 - 2014 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#define Uses_SCIM_UTILITY
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_LOOKUP_TABLE
#define Uses_SCIM_CONFIG_BASE
#define Uses_SCIM_CONFIG_PATH

#include <Elementary.h>
#include <Ecore_IMF.h>
#ifndef WAYLAND
#include <Ecore_X.h>
#endif
#include "option.h"

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <scim.h>
#include <string.h>

using namespace scim;

static ISELanguageManager _language_manager;

static Evas_Object* create_ise_setup_eo (Evas_Object *parent, Evas_Object *naviframe);
static void load_config (const ConfigPointer &config);
static void save_config (const ConfigPointer &config);
static void reset_config (const ConfigPointer &config);
static bool query_changed ();

ConfigPointer _scim_config (0);

extern CONFIG_VALUES g_config_values;
extern Evas_Object* create_option_main_view(Evas_Object *parent, Evas_Object *naviframe);

extern "C" {
    void scim_module_init (void)
    {
    }

    void scim_module_exit (void)
    {
    }

    Evas_Object* scim_setup_module_create_ui (Evas_Object *parent, Evas_Object *layout)
    {
        setlocale(LC_ALL, "");
        bindtextdomain(PACKAGE, LOCALEDIR);
        textdomain(PACKAGE);
        return create_ise_setup_eo (parent, layout);
    }

    String scim_setup_module_get_category (void)
    {
        return String ("Helper");
    }

    String scim_setup_module_get_name (void)
    {
        return String ("Tizen Keyboard");
    }

    String scim_setup_module_get_description (void)
    {
        return String (("Setup Module of Tizen Keyboard."));
    }

    void scim_setup_module_load_config (const ConfigPointer &config)
    {
        load_config (config);
    }

    void scim_setup_module_save_config (const ConfigPointer &config)
    {
        save_config (config);
    }

    bool scim_setup_module_query_changed ()
    {
        return query_changed ();
    }

    bool scim_setup_module_key_proceeding (int key_type)
    {
        return false;
    }

    bool scim_setup_module_option_reset(const ConfigPointer &config)
    {
        reset_config (config);
        return true;
    }
}

static void load_config (const ConfigPointer &config)
{
    if (!config.null ()) {
        _scim_config = config;

        read_ise_config_values();

        _language_manager.set_enabled_languages(g_config_values.enabled_languages);

        _language_manager.select_language(g_config_values.selected_language.c_str());
    }
}

static void save_config (const ConfigPointer &config)
{
    write_options();

    if (!config.null ()) {
        _scim_config = config;
        write_ise_config_values();
    }
}

static void reset_config (const ConfigPointer &config)
{
    if (!config.null ()) {
        _scim_config = config;
        erase_ise_config_values();
    }
}

static bool query_changed ()
{
    load_config(_scim_config);
    read_options();
    return EINA_TRUE;
}

void
navi_back_cb(void *data, Evas_Object *obj, void *event_info)
{
    close_option_window();
}

static void transition_finished (void *data, Evas_Object *obj, void *event_info)
{
    /* Delete this smart callback */
    evas_object_smart_callback_del (obj, "transition,finished", transition_finished);

    /* Now register an callback to back button for removing all created objects */
    Elm_Object_Item *top_item = elm_naviframe_top_item_get(obj);
    Evas_Object *back_btn = elm_object_item_part_content_get (top_item, "prev_btn");
    evas_object_smart_callback_add (back_btn, "clicked", navi_back_cb, NULL);
}

static Evas_Object * create_ise_setup_eo (Evas_Object *parent, Evas_Object *naviframe)
{
    Evas_Object *ret = NULL;
    if(parent && naviframe) {
        ret = create_option_main_view(parent, naviframe);
        /* Currently, we have to wait until the transition gets finished,
             to access the back button of the second page.
           Better to modify to access the pushed item object, sometime later */
        evas_object_smart_callback_add (naviframe, "transition,finished", transition_finished, NULL);
    }
    return ret;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
