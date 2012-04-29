/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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
#define WAIT_WM

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
#include <Ecore_X.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <X11/Xlib.h>
#include <malloc.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#if HAVE_VCONF
#include <heynoti.h>
#include <vconf.h>
#include <vconf-keys.h>
#endif
#include <privilege-control.h>
#include "isf_panel_utility.h"


using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define EFL_CANDIDATE_THEME1                            (SCIM_DATADIR "/isf_candidate_theme1.edj")
#define EFL_CANDIDATE_THEME2                            (SCIM_DATADIR "/isf_candidate_theme2.edj")

#define ISF_CONFIG_PANEL_LOOKUP_TABLE_VERTICAL          "/Panel/Gtk/LookupTableVertical"
#define ISF_CONFIG_PANEL_LOOKUP_TABLE_STYLE             "/Panel/Gtk/LookupTableStyle"
#define ISF_CONFIG_PANEL_LOOKUP_TABLE_MODE              "/Panel/Gtk/LookupTableMode"

#define ISF_CANDIDATE_TABLE                             0

#define ISF_PORT_BUTTON_NUMBER                          9
#define ISF_LAND_BUTTON_NUMBER                          18

#define ISF_EFL_AUX                                     1
#define ISF_EFL_CANDIDATE_0                             2
#define ISF_EFL_CANDIDATE_ITEMS                         3

#define ISE_DEFAULT_HEIGHT_PORTRAIT                     360
#define ISE_DEFAULT_HEIGHT_LANDSCAPE                    288


/////////////////////////////////////////////////////////////////////////////
// Declaration of external variables.
/////////////////////////////////////////////////////////////////////////////
extern MapStringVectorSizeT         _groups;
extern std::vector<String>          _uuids;
extern std::vector<String>          _names;
extern std::vector<String>          _module_names;
extern std::vector<String>          _langs;
extern std::vector<String>          _icons;
extern std::vector<uint32>          _options;
extern std::vector<TOOLBAR_MODE_T>  _modes;
extern std::vector<String>          _disabled_langs;

extern std::vector<String>          _load_ise_list;

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal data types.
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object *efl_create_window                    (const char *strWinName, const char *strEffect);
static void       efl_set_transient_for_app_window       (Evas_Object *win_obj);
static int        efl_get_angle_for_root_window          (Evas_Object *win_obj);

static void       ui_candidate_hide                      (bool bForce);
static void       ui_destroy_candidate_window            (void);
static void       ui_settle_candidate_window             (void);
static void       ui_create_more_window                  (void);
static void       ui_more_window_rotate                  (int angle);
static void       ui_more_window_update                  (void);

/* PanelAgent related functions */
static bool       initialize_panel_agent               (const String &config, const String &display, bool resident);

static void       slot_reload_config                   (void);
static void       slot_focus_in                        (void);
static void       slot_focus_out                       (void);
static void       slot_update_spot_location            (int x, int y, int top_y);
static void       slot_update_factory_info             (const PanelFactoryInfo &info);
static void       slot_show_aux_string                 (void);
static void       slot_show_candidate_table            (void);
static void       slot_hide_aux_string                 (void);
static void       slot_hide_candidate_table            (void);
static void       slot_update_aux_string               (const String &str, const AttributeList &attrs);
static void       slot_update_candidate_table          (const LookupTable &table);
static void       slot_set_active_ise_by_uuid          (const String &uuid, bool);
static bool       slot_get_ise_list                    (std::vector<String> &name);
static bool       slot_get_keyboard_ise_list           (std::vector<String> &name);
static void       slot_get_language_list               (std::vector<String> &name);
static void       slot_get_all_language                (std::vector<String> &lang);
static void       slot_get_ise_language                (char *, std::vector<String> &name);
static void       slot_set_isf_language                (const String &language);
static bool       slot_get_ise_info_by_uuid            (const String &uuid, ISE_INFO &info);
static void       slot_set_candidate_ui                (int style, int mode);
static void       slot_get_candidate_ui                (int &style, int &mode);
static void       slot_set_candidate_position          (int left, int top);
static void       slot_get_candidate_geometry          (struct rectinfo &info);
static void       slot_set_keyboard_ise                (int type, const String &ise);
static void       slot_get_keyboard_ise                (String &ise_name, String &ise_uuid);
static void       slot_start_default_ise               (void);
static void       slot_accept_connection               (int fd);
static void       slot_close_connection                (int fd);
static void       slot_exit                            (void);

static Eina_Bool  panel_agent_handler                  (void *data, Ecore_Fd_Handler *fd_handler);
static void       change_hw_and_sw_keyboard            (void);


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object       *_candidate_window                 = 0;
static Evas_Object       *_candidate_area_1                 = 0;
static Evas_Object       *_candidate_area_2                 = 0;
static Evas_Object       *_candidate_bg                     = 0;
static Evas_Object       *_candidate_0_scroll               = 0;
static Evas_Object       *_candidate_scroll                 = 0;
static Evas_Object       *_candidate_0_table                = 0;
static Evas_Object       *_candidate_table                  = 0;
static Evas_Object       *_candidate_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_candidate_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_more_btn                         = 0;
static Evas_Object       *_close_btn                        = 0;
static bool               _candidate_window_show            = false;

static Evas_Object       *_more_window                      = 0;
static Evas_Object       *_more_port_table                  = 0;
static Evas_Object       *_more_land_table                  = 0;
static Evas_Object       *_page_button_port                 = 0;
static Evas_Object       *_page_button_land                 = 0;
static Evas_Object       *_port_button [ISF_PORT_BUTTON_NUMBER];
static Evas_Object       *_land_button [ISF_LAND_BUTTON_NUMBER];
static bool               _more_window_enable               = false;
static int                _more_port_height                 = 329;
static int                _more_land_height                 = 270;
static int                _pages                            = 0;
static int                _page_no                          = 0;
static std::vector<String> _more_list;

static int                _candidate_width                  = 0;
static int                _candidate_height                 = 0;

static int                _candidate_port_width             = 464;
static int                _candidate_port_height_min        = 86;
static int                _candidate_port_height_min_2      = 150;
static int                _candidate_port_height_max        = 286;
static int                _candidate_port_height_max_2      = 350;
static int                _candidate_land_width             = 784;
static int                _candidate_land_height_min        = 86;
static int                _candidate_land_height_max        = 150;
static int                _candidate_land_height_max_2      = 214;
static int                _candidate_area_1_pos [2]         = {11, 13};
static int                _candidate_area_2_pos [2]         = {11, 13};
static int                _more_btn_pos [4]                 = {369, 11, 689, 11};
static int                _close_btn_pos [4]                = {362, 211, 682, 75};

static int                _candidate_port_number            = 3;
static int                _candidate_land_number            = 5;
static int                _h_padding                        = 4;
static int                _v_padding                        = 11;
static int                _item_min_width                   = 66;
static int                _item_min_height                  = 55;

static int                _candidate_scroll_0_width_min     = 350;
static int                _candidate_scroll_0_width_max     = 670;

static int                _candidate_scroll_width           = 453;
static int                _candidate_scroll_width_min       = 453;
static int                _candidate_scroll_width_max       = 663;
static int                _candidate_scroll_height_min      = 124;
static int                _candidate_scroll_height_max      = 190;

static Evas_Object       *_aux_area                         = 0;
static Evas_Object       *_aux_line                         = 0;
static Evas_Object       *_aux_table                        = 0;
static int                _aux_height                       = 0;
static int                _aux_port_width                   = 444;
static int                _aux_land_width                   = 764;
static std::vector<Evas_Object *> _aux_items;

static Evas_Object       *_tmp_aux_text                     = 0;
static Evas_Object       *_tmp_candidate_text               = 0;
static std::vector<Evas_Object *> _text_fonts;

static int                _spot_location_x                  = -1;
static int                _spot_location_y                  = -1;
static int                _spot_location_top_y              = -1;
static int                _candidate_left                   = -1;
static int                _candidate_top                    = -1;
static int                _candidate_angle                  = 0;

static int                _indicator_height                 = 24;
static int                _screen_width                     = 480;
static int                _screen_height                    = 800;
static float              _width_rate                       = 1.0;
static float              _height_rate                      = 1.0;
static int                _blank_width                      = 10;

static String             _candidate_name                   = String ("candidate");
static String             _candidate_edje_file              = String (EFL_CANDIDATE_THEME1);

static String             _candidate_font_name              = String ("SLP:style=medium");
static int                _candidate_font_max_size          = 30;
static int                _candidate_font_min_size          = 14;
static int                _aux_font_size                    = 30;
static int                _button_font_size                 = 22;
static int                _click_object                     = 0;
static int                _click_down_pos [2]               = {0, 0};
static int                _click_up_pos [2]                 = {0, 0};

static DEFAULT_ISE_T      _initial_ise;
static DEFAULT_ISE_T      _active_ise;
static ConfigPointer      _config;
static PanelAgent        *_panel_agent                      = 0;
static std::vector<Ecore_Fd_Handler *> _read_handler_list;

static ISF_CANDIDATE_STYLE_T _candidate_style               = PREDICTION_ENGINE_CANDIDATE_STYLE;
static ISF_CANDIDATE_MODE_T  _candidate_mode                = PORTRAIT_VERTICAL_CANDIDATE_MODE;

static clock_t            _clock_start;


/////////////////////////////////////////////////////////////////////////////
// Implementation of internal functions.
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print system time point for panel performance.
 *
 * @param strInfo The output information.
 */
static void check_time (const char *strInfo)
{
    gettime (_clock_start, strInfo);
    ISF_LOG ("%s ppid=%d pid=%d\n", strInfo, getppid (), getpid ());
}

/**
 * @brief Check the specific file.
 *
 * @param strFile The file path.
 *
 * @return true if the file is existed, otherwise false.
 */
static bool check_file (const char* strFile)
{
    struct stat st;

    /* Workaround so that "/" returns a true, otherwise we can't monitor "/" in ecore_file_monitor */
    if (stat (strFile, &st) < 0 && strcmp (strFile, "/"))
        return false;
    else
        return true;
}

/**
 * @brief Flush memory for elm.
 *
 * @return void
 */
static void flush_memory (void)
{
    elm_cache_all_flush ();
    malloc_trim (0);
}

/**
 * @brief Get ISE name according to uuid.
 *
 * @param uuid The ISE uuid.
 *
 * @return The ISE name
 */
static String get_ise_name (const String uuid)
{
    String ise_name;
    if (uuid.length () > 0) {
        for (unsigned int i = 0; i < _uuids.size (); i++) {
            if (strcmp (uuid.c_str (), _uuids[i].c_str ()) == 0) {
                ise_name = _names[i];
                break;
            }
        }
    }

    return ise_name;
}

/**
 * @brief Change keyboard ISE.
 *
 * @param uuid The keyboard ISE's uuid.
 * @param name The keyboard ISE's name.
 *
 * @return false if keyboard ISE change is failed, otherwise return true.
 */
static bool activate_keyboard_ise (const String &uuid, const String &name)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        _panel_agent->stop_helper (pre_uuid);
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        String pre_name = _panel_agent->get_current_ise_name ();
        if (!pre_name.compare (name))
            return false;
    }

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);
    _config->flush ();

    _panel_agent->change_factory (uuid);

    return true;
}

/**
 * @brief Change helper ISE.
 *
 * @param uuid The helper ISE's uuid.
 * @param changeDefault The flag for change default ISE.
 *
 * @return false if helper ISE change is failed, otherwise return true.
 */
static bool activate_helper_ise (const String &uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        if (!pre_uuid.compare (uuid))
            return false;
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        /*_panel_agent->change_factory ("");*/
    }

    _panel_agent->start_helper (uuid);
    if (changeDefault) {
        _config->write (String (SCIM_CONFIG_DEFAULT_HELPER_ISE), uuid);
        _config->flush ();
    }

    return true;
}

/**
 * @brief Set active ISE by uuid.
 *
 * @param uuid The ISE's uuid.
 * @param fromISE It indicates whether the request is sent from helper ISE.
 *
 * @return false if ISE change is failed, otherwise return true.
 */
static bool set_active_ise_by_uuid (const String &ise_uuid, bool fromISE, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (ise_uuid.length () <= 0)
        return false;

    bool ise_changed = false;

    if (fromISE) {
        TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

        if (TOOLBAR_HELPER_MODE == mode) {
            String pre_uuid = _panel_agent->get_current_helper_uuid ();
            _panel_agent->hide_helper (pre_uuid);
        }
        _panel_agent->set_ise_changing (true);
    }

    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (!ise_uuid.compare (_uuids[i])) {
            if (TOOLBAR_KEYBOARD_MODE == _modes[i])
                ise_changed = activate_keyboard_ise (_uuids[i], _names[i]);
            else if (TOOLBAR_HELPER_MODE == _modes[i])
                ise_changed = activate_helper_ise (_uuids[i], changeDefault);

            if (ise_changed) {
                DEFAULT_ISE_T default_ise;
                default_ise.type = _modes[i];
                default_ise.uuid = _uuids[i];
                default_ise.name = _names[i];
                if (changeDefault)
                    _panel_agent->set_default_ise (default_ise);
                _panel_agent->set_current_toolbar_mode (default_ise.type);
                _panel_agent->set_current_ise_name (default_ise.name);
            }

            return true;
        }
    }

    return false;
}

/**
 * @brief Load ISF configuration and ISEs information.
 */
static void load_config (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String str;
    bool   shared_ise = false;

    /* Read configurations. */
    if (!_config.null ()) {
        _candidate_style    = (ISF_CANDIDATE_STYLE_T)_config->read (String (ISF_CONFIG_PANEL_LOOKUP_TABLE_STYLE), _candidate_style);
        _candidate_mode     = (ISF_CANDIDATE_MODE_T)_config->read (String (ISF_CONFIG_PANEL_LOOKUP_TABLE_MODE), _candidate_mode);
        shared_ise          = _config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), shared_ise);
        _panel_agent->set_should_shared_ise (shared_ise);
    }

    isf_load_ise_information (ALL_ISE, _config);
}

/**
 * @brief Reload config callback function for ISF panel.
 *
 * @param config The config pointer.
 */
static void config_reload_cb (const ConfigPointer &config)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* load_config (); */
}


//////////////////////////////////////////////////////////////////////
// Start of Candidate Functions
//////////////////////////////////////////////////////////////////////
/**
 * @brief This function will add one candidate window resize timer.
 *
 * @param new_width New width for candidate window.
 * @param new_height New height for candidate window.
 */
static void ui_candidate_window_resize (int new_width, int new_height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window)
        return;

    int x, y, width, height;
    evas_object_geometry_get (_candidate_window, &x, &y, &width, &height);

    if (width == new_width && height == new_height)
        return;

    evas_object_resize (_candidate_window, new_width, new_height);
    _candidate_width  = new_width;
    _candidate_height = new_height;
    _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

    ui_settle_candidate_window ();
}

/**
 * @brief This function will show/hide widgets of candidate window,
 *        and resize candidate window size according to aux_area/candidate_area.
 */
static void ui_candidate_window_adjust (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    int x, y, width, height;

    /* Get candidate window size */
    evas_object_geometry_get (_candidate_window, &x, &y, &width, &height);
    {
        if (evas_object_visible_get (_aux_area) && evas_object_visible_get (_candidate_area_1)) {
            evas_object_show (_aux_line);
            ui_candidate_window_resize (width, _candidate_port_height_min_2);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
            } else {
                evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            }
        } else if (evas_object_visible_get (_aux_area) && evas_object_visible_get (_candidate_area_2)) {
            evas_object_show (_aux_line);
            evas_object_move (_candidate_area_2, _candidate_area_2_pos[0], _candidate_area_2_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_max_2);
                evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_max_2);
                evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            }
        } else if (evas_object_visible_get (_aux_area)) {
            evas_object_hide (_aux_line);
            ui_candidate_window_resize (width, _aux_height);
        } else if (evas_object_visible_get (_candidate_area_1)) {
            evas_object_hide (_aux_line);
            ui_candidate_window_resize (width, _candidate_port_height_min);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3]);
            } else {
                evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
            }
        } else if (evas_object_visible_get (_candidate_area_2)) {
            evas_object_hide (_aux_line);
            evas_object_move (_candidate_area_2, _candidate_area_2_pos[0], _candidate_area_2_pos[1]);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_max);
                evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3]);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_max);
                evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
            }
        }
    }
}

/**
 * @brief Rotate candidate window.
 *
 * @param angle The angle of candidate window.
 */
static void ui_candidate_window_rotate (int angle)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    elm_win_rotation_set (_candidate_window, angle);
    ui_candidate_hide (true);
    if (angle == 90 || angle == 270) {
        _candidate_scroll_width = _candidate_scroll_width_max;
        ui_candidate_window_resize (_candidate_land_width, _candidate_land_height_min);
        evas_object_resize (_aux_area, _aux_land_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_max, _item_min_height);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_min);
    } else {
        _candidate_scroll_width = _candidate_scroll_width_min;
        ui_candidate_window_resize (_candidate_port_width, _candidate_port_height_min);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_min, _item_min_height);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_max);
    }

    /* Delete old candidate items and popup lines */
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_items [i]) {
            evas_object_del (_candidate_items [i]);
            _candidate_items [i] = NULL;
        }
    }

    ui_more_window_rotate (angle);
    ui_settle_candidate_window ();
    flush_memory ();
}

/**
 * @brief Update candidate text font size according to display width.
 *
 * @param obj A valid Evas_Object handle of text.
 * @param obj_width The text display width.
 * @param mbs The dislpayed string.
 */
static void ui_candidate_text_update_font_size (Evas_Object *obj, int obj_width, String mbs)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (obj == NULL || mbs.length () <= 0 || _text_fonts.size () <= 0)
        return;

    int x, y, width, height;
    edje_object_text_class_set (obj, "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_min_size);
    for (unsigned int i = 0; i < _text_fonts.size (); i++) {
        evas_object_text_text_set (_text_fonts [i], mbs.c_str ());
        evas_object_geometry_get (_text_fonts [i], &x, &y, &width, &height);
        if (width < obj_width) {
            edje_object_text_class_set (obj, "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_max_size - 2*i);
            break;
        }
    }
}

/**
 * @brief This function is used to judge whether candidate window should be hidden.
 *
 * @return true if candidate window should be hidden, otherwise return false.
 */
static bool ui_candidate_can_be_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (evas_object_visible_get (_aux_area) ||
        evas_object_visible_get (_candidate_area_1) ||
        evas_object_visible_get (_candidate_area_2))
        return false;
    else
        return true;
}

/**
 * @brief Show candidate window.
 */
static void ui_candidate_show (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    int angle = efl_get_angle_for_root_window (_candidate_window);
    if (_candidate_angle != angle) {
        _candidate_angle = angle;
        ui_candidate_window_rotate (angle);
    }

    if (!evas_object_visible_get (_candidate_window))
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_SHOW);

    _candidate_window_show = true;
    evas_object_show (_candidate_window);
    elm_win_raise (_candidate_window);
    efl_set_transient_for_app_window (_candidate_window);
}

/**
 * @brief Hide candidate window.
 */
static void ui_candidate_hide (bool bForce)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    if (bForce) {
        evas_object_hide (_aux_area);
        elm_scroller_region_show (_aux_area, 0, 0, 10, 10);
        evas_object_hide (_candidate_area_1);
        evas_object_hide (_candidate_area_2);
    }

    if (ui_candidate_can_be_hide () || bForce) {
        if (evas_object_visible_get (_candidate_window))
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_HIDE);

        _candidate_window_show = false;
        evas_object_hide (_candidate_window);
    }
}

/**
 * @brief Callback function for more button.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_candidate_window_more_button_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    evas_object_hide (_candidate_area_1);

    _panel_agent->candidate_more_window_show ();
    if (_more_window_enable) {
        ui_candidate_hide (false);
        ui_candidate_window_adjust ();
        if (_more_window == NULL)
            ui_create_more_window ();
        evas_object_show (_more_window);

        _page_no = 1;
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            _pages = _more_list.size () / ISF_LAND_BUTTON_NUMBER;
            if ((_more_list.size () % ISF_LAND_BUTTON_NUMBER) > 0)
                _pages += 1;
        } else {
            _pages = _more_list.size () / ISF_PORT_BUTTON_NUMBER;
            if ((_more_list.size () % ISF_PORT_BUTTON_NUMBER) > 0)
                _pages += 1;
        }

        ui_more_window_update ();
    } else {
        evas_object_hide (_more_btn);
        evas_object_show (_candidate_area_2);
        evas_object_show (_close_btn);
        elm_win_raise (_candidate_window);
        ui_candidate_window_adjust ();
    }
    ui_settle_candidate_window ();
    flush_memory ();
}

/**
 * @brief Callback function for close button.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_candidate_window_close_button_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (evas_object_visible_get (_candidate_area_2)) {
        evas_object_hide (_candidate_area_2);
        evas_object_hide (_close_btn);
        _panel_agent->candidate_more_window_hide ();
    }
    evas_object_show (_candidate_area_1);
    evas_object_show (_more_btn);
    elm_win_raise (_candidate_window);
    ui_candidate_window_adjust ();
    ui_settle_candidate_window ();
    elm_scroller_region_show (_candidate_area_2, 0, 0, _candidate_scroll_width, 100);
    flush_memory ();
}

/**
 * @brief Callback function for mouse button press.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_button_pressed_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    elm_win_raise (_candidate_window);
    _click_object = GPOINTER_TO_INT (data);

    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    _click_down_pos [0] = ev->canvas.x;
    _click_down_pos [1] = ev->canvas.y;
}

/**
 * @brief Callback function for mouse button release.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_button_released_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " index:" << GPOINTER_TO_INT (data) << "...\n";

    int index = GPOINTER_TO_INT (data);
    if (_click_object == ISF_EFL_AUX) {
        double ret = 0;
        const char *buf = edje_object_part_state_get (button, "aux", &ret);
        if (strcmp ("selected", buf)) {
            for (unsigned int i = 0; i < _aux_items.size (); i++) {
                buf = edje_object_part_state_get (_aux_items [i], "aux", &ret);
                if (!strcmp ("selected", buf))
                    edje_object_signal_emit (_aux_items [i], "aux,state,unselected", "aux");
            }
            edje_object_signal_emit (button, "aux,state,selected", "aux");
            _panel_agent->select_aux (index);
        }
    } else if (_click_object == ISF_EFL_CANDIDATE_0) {
        _panel_agent->select_candidate (index);
    } else if (_click_object == ISF_EFL_CANDIDATE_ITEMS) {
        ui_candidate_window_close_button_cb (NULL, NULL, _close_btn, NULL);
        _panel_agent->select_candidate (index);
    }
}

/**
 * @brief Callback function for mouse move.
 *
 * @param data Data to pass when it is called.
 * @param e The evas for current event.
 * @param button The evas object for current event.
 * @param event_info The information for current event.
 */
static void ui_mouse_moved_cb (void *data, Evas *e, Evas_Object *button, void *event_info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    _click_up_pos [0] = ev->canvas.x;
    _click_up_pos [1] = ev->canvas.y;

    if (abs (_click_up_pos [0] - _click_down_pos [0]) >= 10 ||
        abs (_click_up_pos [1] - _click_down_pos [1]) >= 10) {
        _click_object = 0;
    }
}

/**
 * @brief Rotate more window.
 *
 * @param angle The angle of more window.
 */
static void ui_more_window_rotate (int angle)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window || !_more_window)
        return;

    elm_win_rotation_set (_more_window, angle);
    if (angle == 90 || angle == 270) {
        evas_object_resize (_more_window, _screen_height, _more_land_height);
        evas_object_hide (_more_port_table);
        evas_object_show (_more_land_table);
        if (angle == 90)
            evas_object_move (_more_window, _screen_width - _more_land_height, 0);
        else
            evas_object_move (_more_window, 0, 0);
    } else {
        evas_object_resize (_more_window, _screen_width, _more_port_height);
        evas_object_hide (_more_land_table);
        evas_object_show (_more_port_table);
        if (angle == 180)
            evas_object_move (_more_window, 0, 0);
        else
            evas_object_move (_more_window, 0, _screen_height - _more_port_height);
    }
}

/**
 * @brief Update more window.
 */
static void ui_more_window_update (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window || !_more_window)
        return;

    unsigned int offset, index, i;
    char buffer [20] = {0};

    snprintf (buffer, sizeof (buffer), "%d/%d", _page_no, _pages);

    if (_candidate_angle == 90 || _candidate_angle == 270) {
        offset = (_page_no - 1) * ISF_LAND_BUTTON_NUMBER;
        for (i = 0; i < ISF_LAND_BUTTON_NUMBER; i++) {
            index = offset + i;
            if (index < _more_list.size ()) {
                edje_object_part_text_set (_land_button [i], "button_text", _more_list [index].c_str ());
                ui_candidate_text_update_font_size (_land_button [i], (128 - 10)*_height_rate, _more_list [index]);
                evas_object_show (_land_button [i]);
            } else {
                edje_object_part_text_set (_land_button [i], "button_text", "");
                evas_object_hide (_land_button [i]);
            }
        }
        edje_object_part_text_set (_page_button_land, "page_text", buffer);
    } else {
        offset = (_page_no - 1) * ISF_PORT_BUTTON_NUMBER;
        for (i = 0; i < ISF_PORT_BUTTON_NUMBER; i++) {
            index = offset + i;
            if (index < _more_list.size ()) {
                edje_object_part_text_set (_port_button [i], "button_text", _more_list [index].c_str ());
                ui_candidate_text_update_font_size (_port_button [i], (153 - 10)*_width_rate, _more_list [index]);
                evas_object_show (_port_button [i]);
            } else {
                edje_object_part_text_set (_port_button [i], "button_text", "");
                evas_object_hide (_port_button [i]);
            }
        }
        edje_object_part_text_set (_page_button_port, "page_text", buffer);
    }
}

/**
 * @brief Callback function for close button of more window.
 *
 * @param data A pointer to data to pass in when the signal is emitted.
 * @param obj A valid Evas_Object handle for emitted signal.
 * @param emission The signal's name.
 * @param source The signal's source.
 */
static void ui_more_window_close_button_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (evas_object_visible_get (_more_window)) {
        evas_object_hide (_more_window);
        _panel_agent->candidate_more_window_hide ();
    }

    elm_win_raise (_candidate_window);
    slot_show_candidate_table ();
    flush_memory ();
}

/**
 * @brief Callback function for page button of more window.
 *
 * @param data A pointer to data to pass in when the signal is emitted.
 * @param obj A valid Evas_Object handle for emitted signal.
 * @param emission The signal's name.
 * @param source The signal's source.
 */
static void ui_more_window_page_button_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_pages <= 1)
        return;

    int  action = GPOINTER_TO_INT (data);
    if (action == 1)        /* Show prev page */
        _page_no = (_page_no == 1) ? _pages : (_page_no - 1);
    else                    /* Show next page */
        _page_no = (_page_no == _pages) ? 1 : (_page_no + 1);

    ui_more_window_update ();
    flush_memory ();
}

/**
 * @brief Callback function for candidate item of more window.
 *
 * @param data A pointer to data to pass in when the signal is emitted.
 * @param obj A valid Evas_Object handle for emitted signal.
 * @param emission The signal's name.
 * @param source The signal's source.
 */
static void ui_more_window_candidate_item_click_cb (void *data, Evas_Object *obj, const char *emission, const char *source)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int offset;
    int index = GPOINTER_TO_INT (data);
    if (index < SCIM_LOOKUP_TABLE_MAX_PAGESIZE) {
        if (_candidate_angle == 90 || _candidate_angle == 270)
            offset = (_page_no - 1) * ISF_LAND_BUTTON_NUMBER;
        else
            offset = (_page_no - 1) * ISF_PORT_BUTTON_NUMBER;

        ui_more_window_close_button_cb (NULL, NULL, NULL, NULL);
        _panel_agent->select_candidate (offset + index);
    }
}

/**
 * @brief Create more window for candidate.
 */
static void ui_create_more_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Object *button = NULL;

    _more_window = efl_create_window ("more_window", "ISF Popup");
    evas_object_resize (_more_window, _screen_width, _more_port_height);
    evas_object_move (_more_window, 0, _screen_height - _more_port_height);

    Evas *evas = evas_object_evas_get (_more_window);

    /* Create port table */
    _more_port_table = elm_table_add (_more_window);
    evas_object_size_hint_weight_set (_more_port_table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show (_more_port_table);

    Evas_Object *bg = edje_object_add (evas);
    edje_object_file_set (bg, _candidate_edje_file.c_str (), "candidate_bg");
    evas_object_size_hint_min_set (bg, _screen_width, _more_port_height);
    elm_table_pack (_more_port_table, bg, 0, 0, _screen_width, _more_port_height);
    edje_object_signal_emit (bg, "candidate_bg,state,port_more_2", "candidate_bg");
    evas_object_show (bg);

    for (int i = 0; i < ISF_PORT_BUTTON_NUMBER; i++) {
        button = edje_object_add (evas);
        edje_object_file_set (button, _candidate_edje_file.c_str (), "candidate_button");
        evas_object_size_hint_min_set (button, 153*_width_rate, 71*_height_rate);
        elm_table_pack (_more_port_table, button, (6 + (i%3)*158)*_width_rate, (15 + (i/3)*79)*_height_rate, 153*_width_rate, 71*_height_rate);
        edje_object_signal_callback_add (button, "button,action,clicked", "",
                                         ui_more_window_candidate_item_click_cb, GINT_TO_POINTER (i));
        evas_object_show (button);
        _port_button [i] = button;
    }

    /* Create port page button */
    _page_button_port = edje_object_add (evas);
    edje_object_file_set (_page_button_port, _candidate_edje_file.c_str (), "page_button_port");
    evas_object_size_hint_min_set (_page_button_port, 232*_width_rate, 71*_height_rate);
    edje_object_text_class_set (_page_button_port, "page_text_class", "1/1", _candidate_font_max_size);
    elm_table_pack (_more_port_table, _page_button_port, 6*_width_rate, 252*_height_rate, 233*_width_rate, 71*_height_rate);
    edje_object_signal_callback_add (_page_button_port, "prev,action,clicked", "", ui_more_window_page_button_cb, GINT_TO_POINTER (1));
    edje_object_signal_callback_add (_page_button_port, "next,action,clicked", "", ui_more_window_page_button_cb, GINT_TO_POINTER (2));
    evas_object_show (_page_button_port);

    /* Create port close button */
    button = edje_object_add (evas);
    edje_object_file_set (button, _candidate_edje_file.c_str (), "close_button");
    evas_object_size_hint_min_set (button, 114*_width_rate, 71*_height_rate);
    edje_object_text_class_set (button, "button_text_class", "Close", _candidate_font_max_size);
    elm_table_pack (_more_port_table, button, 361*_width_rate, 252*_height_rate, 114*_width_rate, 71*_height_rate);
    edje_object_signal_callback_add (button, "button,action,clicked", "", ui_more_window_close_button_cb, 0);
    evas_object_show (button);

    /* Create land table */
    _more_land_table = elm_table_add (_more_window);
    evas_object_size_hint_weight_set (_more_land_table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    bg = edje_object_add (evas);
    edje_object_file_set (bg, _candidate_edje_file.c_str (), "candidate_bg");
    evas_object_size_hint_min_set (bg, _screen_height, _more_land_height);
    elm_table_pack (_more_land_table, bg, 0, 0, _screen_height, _more_land_height);
    edje_object_signal_emit (bg, "candidate_bg,state,land_more_2", "candidate_bg");
    evas_object_show (bg);

    for (int i = 0; i < ISF_LAND_BUTTON_NUMBER; i++) {
        button = edje_object_add (evas);
        edje_object_file_set (button, _candidate_edje_file.c_str (), "candidate_button");
        evas_object_size_hint_min_set (button, 128*_height_rate, 56*_width_rate);
        elm_table_pack (_more_land_table, button, (7 + (i%6)*132)*_height_rate, (16 + (i/6)*65)*_width_rate, 128*_height_rate, 56*_width_rate);
        edje_object_signal_callback_add (button, "button,action,clicked", "",
                                         ui_more_window_candidate_item_click_cb, GINT_TO_POINTER (i));
        evas_object_show (button);
        _land_button [i] = button;
    }

    /* Create land page button */
    _page_button_land = edje_object_add (evas);
    edje_object_file_set (_page_button_land, _candidate_edje_file.c_str (), "page_button_land");
    evas_object_size_hint_min_set (_page_button_land, 260*_height_rate, 56*_width_rate);
    edje_object_text_class_set (_page_button_land, "page_text_class", "1/1", _candidate_font_max_size);
    elm_table_pack (_more_land_table, _page_button_land, 7*_height_rate, 211*_width_rate, 260*_height_rate, 56*_width_rate);
    edje_object_signal_callback_add (_page_button_land, "prev,action,clicked", "", ui_more_window_page_button_cb, GINT_TO_POINTER (1));
    edje_object_signal_callback_add (_page_button_land, "next,action,clicked", "", ui_more_window_page_button_cb, GINT_TO_POINTER (2));
    evas_object_show (_page_button_land);

    /* Create land close button */
    button = edje_object_add (evas);
    edje_object_file_set (button, _candidate_edje_file.c_str (), "close_button");
    evas_object_size_hint_min_set (button, 128*_height_rate, 56*_width_rate);
    edje_object_text_class_set (button, "button_text_class", "Close", _candidate_font_max_size);
    elm_table_pack (_more_land_table, button, 667*_height_rate, 211*_width_rate, 128*_height_rate, 56*_width_rate);
    edje_object_signal_callback_add (button, "button,action,clicked", "", ui_more_window_close_button_cb, 0);
    evas_object_show (button);

    if (_candidate_angle != 0)
        ui_more_window_rotate (_candidate_angle);

    flush_memory ();
}

/**
 * @brief Create prediction engine style candidate window.
 */
static void ui_create_prediction_engine_candidate_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Object *tmp_text = NULL;

    if (_candidate_edje_file == String (EFL_CANDIDATE_THEME2)) {
        _candidate_port_width        = 480 * _width_rate;
        _candidate_port_height_min   = 102 * _height_rate;
        _candidate_port_height_min_2 = 157 * _height_rate;
        _candidate_port_height_max   = 310 * _height_rate;
        _candidate_port_height_max_2 = 365 * _height_rate;
        _candidate_land_width        = 800 * _height_rate;
        _candidate_land_height_min   = 102 * _width_rate;
        _candidate_land_height_max   = 170 * _width_rate;
        _candidate_land_height_max_2 = 230 * _width_rate;

        _candidate_scroll_0_width_min = 350 * _width_rate;
        _candidate_scroll_0_width_max = 670 * _height_rate;
        _candidate_scroll_width_min  = 446 * _width_rate;
        _candidate_scroll_width_max  = 676 * _height_rate;
        _candidate_scroll_height_min = 130 * _height_rate;
        _candidate_scroll_height_max = 198 * _width_rate;

        _candidate_area_1_pos [0]    = 20 * _width_rate;
        _candidate_area_1_pos [1]    = 20 * _height_rate;
        _candidate_area_2_pos [0]    = 20 * _width_rate;
        _candidate_area_2_pos [1]    = 20 * _height_rate;
        _more_btn_pos [0]            = 376 * _width_rate;
        _more_btn_pos [1]            = 20 * _height_rate;
        _more_btn_pos [2]            = 696 * _height_rate;
        _more_btn_pos [3]            = 20 * _width_rate;
        _close_btn_pos [0]           = 376 * _width_rate;
        _close_btn_pos [1]           = 228 * _height_rate;
        _close_btn_pos [2]           = 696 * _height_rate;
        _close_btn_pos [3]           = 88 * _width_rate;

        _aux_height                  = 68 * _height_rate;
        _aux_port_width              = 460 * _width_rate;
        _aux_land_width              = 780 * _height_rate;

        _h_padding = 5 * _width_rate;
        _v_padding = 6 * _height_rate;
        _item_min_height = 62 * _height_rate;
    } else {
        _candidate_port_width        = 464 * _width_rate;
        _candidate_port_height_min   = 86 * _height_rate;
        _candidate_port_height_min_2 = 150 * _height_rate;
        _candidate_port_height_max   = 286 * _height_rate;
        _candidate_port_height_max_2 = 350 * _height_rate;
        _candidate_land_width        = 784 * _height_rate;
        _candidate_land_height_min   = 86 * _width_rate;
        _candidate_land_height_max   = 150 * _width_rate;
        _candidate_land_height_max_2 = 214 * _width_rate;

        _candidate_scroll_0_width_min = 350 * _width_rate;
        _candidate_scroll_0_width_max = 670 * _height_rate;
        _candidate_scroll_width_min  = 448 * _width_rate;
        _candidate_scroll_width_max  = 676 * _height_rate;
        _candidate_scroll_height_min = 124 * _height_rate;
        _candidate_scroll_height_max = 190 * _width_rate;

        _candidate_area_1_pos [0]    = 11 * _width_rate;
        _candidate_area_1_pos [1]    = 13 * _height_rate;
        _candidate_area_2_pos [0]    = 11 * _width_rate;
        _candidate_area_2_pos [1]    = 13 * _height_rate;
        _more_btn_pos [0]            = 366 * _width_rate;
        _more_btn_pos [1]            = 13 * _height_rate;
        _more_btn_pos [2]            = 689 * _height_rate;
        _more_btn_pos [3]            = 13 * _width_rate;
        _close_btn_pos [0]           = 366 * _width_rate;
        _close_btn_pos [1]           = 214 * _height_rate;
        _close_btn_pos [2]           = 689 * _height_rate;
        _close_btn_pos [3]           = 79 * _width_rate;

        _aux_height                  = _candidate_port_height_min * 4 / 5;
        _aux_port_width              = 444 * _width_rate;
        _aux_land_width              = 764 * _height_rate;

        _h_padding = 4 * _width_rate;
        _v_padding = 11 * _height_rate;
        _item_min_height = 55 * _height_rate;
    }

    _candidate_name = String ("candidate");

    /* Create candidate window */
    if (_candidate_window == NULL) {
        _candidate_window = efl_create_window ("candidate", "Prediction Window");
        evas_object_resize (_candidate_window, _candidate_port_width, _candidate_port_height_min);
        _candidate_width  = _candidate_port_width;
        _candidate_height = _candidate_port_height_min;

        /* Add background */
        _candidate_bg = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_candidate_bg, _candidate_edje_file.c_str (), "candidate_bg");
        evas_object_size_hint_weight_set(_candidate_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (_candidate_window, _candidate_bg);
        evas_object_show (_candidate_bg);

        /* Create _candidate_0 scroller */
        _candidate_0_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_0_scroll, EINA_TRUE, EINA_FALSE);
        elm_scroller_policy_set (_candidate_0_scroll, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_candidate_0_scroll, _candidate_scroll_0_width_min, _item_min_height);
        evas_object_move (_candidate_0_scroll, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
        _candidate_0_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_0_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_0_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_0_table, _h_padding, 0);
        elm_object_content_set (_candidate_0_scroll, _candidate_0_table);
        evas_object_show (_candidate_0_table);
        _candidate_area_1 = _candidate_0_scroll;

        /* Create more button */
        _more_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), _candidate_name.c_str ());
        edje_object_text_class_set (_more_btn, "candidate_text_class", _candidate_font_name.c_str (), _button_font_size);
        edje_object_part_text_set (_more_btn, "candidate", "More");
        evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
        evas_object_resize (_more_btn, 84 * _width_rate, _item_min_height);
        evas_object_event_callback_add (_more_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_more_button_cb, NULL);

        /* Create vertical scroller */
        _candidate_scroll_width = _candidate_scroll_width_min;
        _candidate_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_scroll, 0, 1);
        elm_scroller_policy_set (_candidate_scroll, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
        evas_object_resize (_candidate_scroll, _candidate_scroll_width, _candidate_scroll_height_max);
        elm_scroller_page_size_set (_candidate_scroll, 0, _item_min_height+_v_padding);
        evas_object_move (_candidate_scroll, _candidate_area_2_pos[0], _candidate_area_2_pos[1]);
        _candidate_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_table, _h_padding, _v_padding);
        elm_object_content_set (_candidate_scroll, _candidate_table);
        evas_object_show (_candidate_table);
        _candidate_area_2 = _candidate_scroll;

        /* Create close button */
        _close_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), _candidate_name.c_str ());
        edje_object_text_class_set (_close_btn, "candidate_text_class", _candidate_font_name.c_str (), _button_font_size);
        edje_object_part_text_set (_close_btn, "candidate", "Close");
        evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
        evas_object_resize (_close_btn, 84 * _width_rate, _item_min_height);
        evas_object_event_callback_add (_close_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_close_button_cb, NULL);

        _tmp_candidate_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_candidate_text, _candidate_font_name.c_str (), _candidate_font_max_size);

        _text_fonts.clear ();
        for (int font_size = _candidate_font_max_size; font_size > _candidate_font_min_size; font_size-=2) {
            tmp_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
            evas_object_text_font_set (tmp_text, _candidate_font_name.c_str (), font_size);
            _text_fonts.push_back (tmp_text);
        }

        /* Create aux */
        _aux_area = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_aux_area, 1, 0);
        elm_scroller_policy_set (_aux_area, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_move (_aux_area, _blank_width, 0);

        _aux_table = elm_table_add (_candidate_window);
        elm_object_content_set (_aux_area, _aux_table);
        elm_table_padding_set (_aux_table, _blank_width, 0);
        evas_object_size_hint_weight_set (_aux_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_aux_table, 0.0, 0.0);
        evas_object_show (_aux_table);

        _aux_line = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_aux_line, _candidate_edje_file.c_str (), "popup_line");
        evas_object_resize (_aux_line, _candidate_port_width, 2);
        evas_object_move (_aux_line, 0, 62 * _height_rate);

        _tmp_aux_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_aux_text, _candidate_font_name.c_str (), _aux_font_size);
    }

    flush_memory ();
}

/**
 * @brief Create candidate window.
 *
 * @param style The candidate window style.
 */
static void ui_create_candidate_window (ISF_CANDIDATE_STYLE_T style)
{
    check_time ("\nEnter ui_create_candidate_window");
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_destroy_candidate_window ();

    _candidate_angle = 0;

    ui_create_prediction_engine_candidate_window ();

    check_time ("Exit ui_create_candidate_window");
}

/**
 * @brief Destroy candidate window.
 *
 * @return void
 */
static void ui_destroy_candidate_window (void)
{
    check_time ("Enter ui_destroy_candidate_window");
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Delete candidate items, popup lines and seperator items */
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_del (_candidate_0 [i]);
            _candidate_0 [i] = NULL;
        }

        if (_candidate_items [i]) {
            evas_object_del (_candidate_items [i]);
            _candidate_items [i] = NULL;
        }
    }

    _aux_items.clear ();
    /* Delete candidate window */
    if (_candidate_window) {
        if (evas_object_visible_get (_candidate_window))
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_HIDE);

        evas_object_del (_candidate_window);
        _candidate_window = NULL;
        _aux_area         = NULL;
        _candidate_area_1 = NULL;
        _candidate_area_2 = NULL;
    }
    _candidate_window_show = false;

    /* Delete more window */
    if (_more_window) {
        evas_object_del (_more_window);
        _more_window = NULL;
    }

    flush_memory ();

    check_time ("Exit ui_destroy_candidate_window");
}

/**
 * @brief Settle candidate window position.
 */
static void ui_settle_candidate_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window) {
        return;
    } else if (!_candidate_window_show) {
        ui_candidate_hide (false);
        return;
    }

    int spot_x, spot_y;
    int candidate_width, candidate_height;
    int x, y, width, height, x2, y2, height2;
    bool reverse = false;

    /* Get candidate window position */
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);
    /* Get exact candidate window size */
    evas_object_geometry_get (_candidate_window, &x2, &y2, &width, &height);
    candidate_width  = width;
    candidate_height = height;

    if (_candidate_left >= 0 || _candidate_top >= 0) {
        spot_x = _candidate_left;
        spot_y = _candidate_top;
    } else {
        spot_x = _spot_location_x;
        spot_y = _spot_location_y;

        rectinfo ise_rect;
        _panel_agent->get_current_ise_geometry (ise_rect);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >= (uint32)_screen_width)
                ise_rect.height = ISE_DEFAULT_HEIGHT_LANDSCAPE * _width_rate;
        } else {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >=(uint32) _screen_height)
                ise_rect.height = ISE_DEFAULT_HEIGHT_PORTRAIT * _height_rate;
        }

        /* Get aux+candidate window height */
        if (evas_object_visible_get (_aux_area))
            height2 = _candidate_port_height_min_2;
        else
            height2 = _candidate_port_height_min;

        int nOffset = _candidate_port_height_min / 3;
        if (_candidate_angle == 270) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_x = _screen_width - _spot_location_top_y;
                if (spot_x > _screen_width - (_indicator_height+candidate_height))
                    spot_x = _screen_width - (_indicator_height+candidate_height);
            } else {
                spot_x = _screen_width - _spot_location_y - candidate_height;
            }
        } else if (_candidate_angle == 90) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_x = _spot_location_top_y - candidate_height;
                spot_x = spot_x < _indicator_height ? _indicator_height : spot_x;
            } else {
                spot_x = spot_y;
            }
        } else if (_candidate_angle == 180) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_y = _screen_height - _spot_location_top_y;
                if (spot_y > _screen_height - (_indicator_height+candidate_height))
                    spot_y = _screen_height - (_indicator_height+candidate_height);
            } else {
                spot_y = _screen_height - _spot_location_y - candidate_height;
            }
        } else {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_y = _spot_location_top_y - candidate_height;
                spot_y = spot_y < _indicator_height ? _indicator_height : spot_y;
            } else {
                spot_y = spot_y;
            }
        }
    }

    if (_candidate_angle == 90 || _candidate_angle == 270) {
        spot_y = (_screen_height - width) / 2;
    } else {
        spot_x = (_screen_width - width) / 2;
    }

    if (spot_x != x || spot_y != y) {
        evas_object_move (_candidate_window, spot_x, spot_y);
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);
    }
}

//////////////////////////////////////////////////////////////////////
// End of Candidate Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Get screen width and height.
 *
 * @param width The screen width.
 * @param height The screen height.
 */
static void efl_get_screen_size (int &width, int &height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Display *d = (Display *)ecore_x_display_get ();
    if (d) {
        int screen_num = DefaultScreen (d);
        width  = DisplayWidth (d, screen_num);
        height = DisplayHeight (d, screen_num);
    } else {
        std::cerr << "ecore_x_display_get () is failed!!!\n";
    }
}

/**
 * @brief Disable focus for app window.
 *
 * @param win_obj The Evas_Object handler of app window.
 */
static void efl_disable_focus_for_app_window (Evas_Object *win_obj)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Eina_Bool accepts_focus;
    Ecore_X_Window_State_Hint initial_state;
    Ecore_X_Pixmap icon_pixmap;
    Ecore_X_Pixmap icon_mask;
    Ecore_X_Window icon_window;
    Ecore_X_Window window_group;
    Eina_Bool is_urgent;

    ecore_x_icccm_hints_get (elm_win_xwindow_get (win_obj),
                             &accepts_focus, &initial_state, &icon_pixmap, &icon_mask, &icon_window, &window_group, &is_urgent);
    ecore_x_icccm_hints_set (elm_win_xwindow_get (win_obj),
                             0, initial_state, icon_pixmap, icon_mask, icon_window, window_group, is_urgent);
}

/**
 * @brief Set transient for app window.
 *
 * @param win_obj The Evas_Object handler of app window.
 */
static void efl_set_transient_for_app_window (Evas_Object *win_obj)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Set a transient window for window stack */
    /* Gets the current XID of the active window into the root window property */
    int  ret = 0;
    Atom type_return;
    int  format_return;
    unsigned long    nitems_return;
    unsigned long    bytes_after_return;
    unsigned char   *data = NULL;
    Ecore_X_Window   xAppWindow;
    Ecore_X_Window   xKeypadWin = elm_win_xwindow_get (win_obj);

    ret = XGetWindowProperty ((Display *)ecore_x_display_get (), ecore_x_window_root_get (xKeypadWin),
                              ecore_x_atom_get ("_ISF_ACTIVE_WINDOW"),
                              0, G_MAXLONG, False, ((Atom) 33), &type_return,
                              &format_return, &nitems_return, &bytes_after_return,
                              &data);

    if (ret == Success) {
        if ((type_return == ((Atom) 33)) && (format_return == 32) && (data)) {
            xAppWindow = *(Window *)data;
            ecore_x_icccm_transient_for_set (xKeypadWin, xAppWindow);
            if (data)
                XFree (data);
        }
    } else {
        std::cerr << "XGetWindowProperty () is failed!!!\n";
    }
}

/**
 * @brief Get angle for root window.
 *
 * @param win_obj The Evas_Object handler of application window.
 *
 * @return The angle of root window.
 */
static int efl_get_angle_for_root_window (Evas_Object *win_obj)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret;
    int count;
    int angle = 0;
    unsigned char *prop_data = NULL;
    Ecore_X_Window root_win = ecore_x_window_root_get (elm_win_xwindow_get (win_obj));

    ret = ecore_x_window_prop_property_get (root_win, ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
    if (ret && prop_data) {
        memcpy (&angle, prop_data, sizeof (int));
    } else {
        std::cerr << "ecore_x_window_prop_property_get () is failed!!!\n";
    }
    if (prop_data)
        XFree (prop_data);

    return angle;
}

/**
 * @brief Set showing effect for application window.
 *
 * @param win The Evas_Object handler of application window.
 * @param strEffect The pointer of effect string.
 */
static void efl_set_showing_effect_for_app_window (Evas_Object *win, const char* strEffect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    ecore_x_icccm_name_class_set (elm_win_xwindow_get (static_cast<Evas_Object*>(win)), strEffect, "ISF");
}

/**
 * @brief Create elementary window.
 *
 * @param strWinName The window name.
 * @param strEffect The window effect string.
 *
 * @return The window pointer
 */
static Evas_Object *efl_create_window (const char *strWinName, const char *strEffect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Evas_Object *win = elm_win_add (NULL, strWinName, ELM_WIN_UTILITY);

    /* set window properties */
    elm_win_autodel_set (win, EINA_TRUE);
    elm_object_focus_allow_set (win, EINA_FALSE);
    elm_win_borderless_set (win, EINA_TRUE);
    elm_win_alpha_set (win, EINA_TRUE);
    efl_disable_focus_for_app_window (win);
    efl_set_showing_effect_for_app_window (win, strEffect);

    return win;
}


//////////////////////////////////////////////////////////////////////
// Start of PanelAgent Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Initialize panel agent.
 *
 * @param config The config string for PanelAgent.
 * @param display The current display.
 * @param resident The variable indicates whether panel will be resident.
 *
 * @return true if initialize is successful, otherwise return false.
 */
static bool initialize_panel_agent (const String &config, const String &display, bool resident)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _panel_agent = new PanelAgent ();

    if (!_panel_agent || !_panel_agent->initialize (config, display, resident))
        return false;

    _panel_agent->signal_connect_reload_config              (slot (slot_reload_config));
    _panel_agent->signal_connect_focus_in                   (slot (slot_focus_in));
    _panel_agent->signal_connect_focus_out                  (slot (slot_focus_out));
    _panel_agent->signal_connect_update_factory_info        (slot (slot_update_factory_info));
    _panel_agent->signal_connect_update_spot_location       (slot (slot_update_spot_location));
    _panel_agent->signal_connect_show_aux_string            (slot (slot_show_aux_string));
    _panel_agent->signal_connect_show_lookup_table          (slot (slot_show_candidate_table));
    _panel_agent->signal_connect_hide_aux_string            (slot (slot_hide_aux_string));
    _panel_agent->signal_connect_hide_lookup_table          (slot (slot_hide_candidate_table));
    _panel_agent->signal_connect_update_aux_string          (slot (slot_update_aux_string));
    _panel_agent->signal_connect_update_lookup_table        (slot (slot_update_candidate_table));
    _panel_agent->signal_connect_set_candidate_ui           (slot (slot_set_candidate_ui));
    _panel_agent->signal_connect_get_candidate_ui           (slot (slot_get_candidate_ui));
    _panel_agent->signal_connect_set_candidate_position     (slot (slot_set_candidate_position));
    _panel_agent->signal_connect_get_candidate_geometry     (slot (slot_get_candidate_geometry));
    _panel_agent->signal_connect_set_active_ise_by_uuid     (slot (slot_set_active_ise_by_uuid));
    _panel_agent->signal_connect_get_ise_list               (slot (slot_get_ise_list));
    _panel_agent->signal_connect_get_keyboard_ise_list      (slot (slot_get_keyboard_ise_list));
    _panel_agent->signal_connect_get_language_list          (slot (slot_get_language_list));
    _panel_agent->signal_connect_get_all_language           (slot (slot_get_all_language));
    _panel_agent->signal_connect_get_ise_language           (slot (slot_get_ise_language));
    _panel_agent->signal_connect_set_isf_language           (slot (slot_set_isf_language));
    _panel_agent->signal_connect_get_ise_info_by_uuid       (slot (slot_get_ise_info_by_uuid));
    _panel_agent->signal_connect_set_keyboard_ise           (slot (slot_set_keyboard_ise));
    _panel_agent->signal_connect_get_keyboard_ise           (slot (slot_get_keyboard_ise));
    _panel_agent->signal_connect_start_default_ise          (slot (slot_start_default_ise));
    _panel_agent->signal_connect_accept_connection          (slot (slot_accept_connection));
    _panel_agent->signal_connect_close_connection           (slot (slot_close_connection));
    _panel_agent->signal_connect_exit                       (slot (slot_exit));
    _panel_agent->get_active_ise_list (_load_ise_list);

    return true;
}

/**
 * @brief Reload config slot function for PanelAgent.
 */
static void slot_reload_config (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_config.null ())
        _config->reload ();
}

/**
 * @brief Focus in slot function for PanelAgent.
 */
static void slot_focus_in (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window) {
        ui_create_candidate_window (_candidate_style);
    }
}

/**
 * @brief Focus out slot function for PanelAgent.
 */
static void slot_focus_out (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    ui_destroy_candidate_window ();
}

/**
 * @brief Update keyboard ISE information slot function for PanelAgent.
 *
 * @param info The information of current Keyboard ISE.
 */
static void slot_update_factory_info (const PanelFactoryInfo &info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String ise_name = info.name;
    String ise_icon = info.icon;

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode)
        ise_name = get_ise_name (_panel_agent->get_current_helper_uuid ());

    if (ise_name.length () > 0)
        _panel_agent->set_current_ise_name (ise_name);
}

/**
 * @brief Update cursor position slot function for PanelAgent.
 *
 * @param x The x position of current cursor.
 * @param y The bottom y position of current cursor.
 * @param top_y The top y position of current cursor.
 */
static void slot_update_spot_location (int x, int y, int top_y)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (x >= 0 && x < _screen_height && y >= 0 && y < _screen_height) {
        _spot_location_x = x;
        _spot_location_y = y;
        _spot_location_top_y = top_y;

        ui_settle_candidate_window ();
    }
}

/**
 * @brief Show aux slot function for PanelAgent.
 */
static void slot_show_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_aux_area == NULL || evas_object_visible_get (_aux_area) ||
        _candidate_angle == 90 || _candidate_angle == 270)
        return;

    evas_object_show (_aux_area);
    ui_candidate_window_adjust ();

    ui_candidate_show ();
    ui_settle_candidate_window ();
}

/**
 * @brief Show candidate table slot function for PanelAgent.
 */
static void slot_show_candidate_table (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_area_1 ||
        evas_object_visible_get (_candidate_area_1) ||
        evas_object_visible_get (_candidate_area_2))
        return;

    evas_object_show (_candidate_area_1);
    evas_object_show (_more_btn);
    evas_object_hide (_close_btn);
    ui_candidate_window_adjust ();

    ui_candidate_show ();
    ui_settle_candidate_window ();
}

/**
 * @brief Hide aux slot function for PanelAgent.
 */
static void slot_hide_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_aux_area || !evas_object_visible_get (_aux_area))
        return;

    evas_object_hide (_aux_area);
    elm_scroller_region_show (_aux_area, 0, 0, 10, 10);
    ui_candidate_window_adjust ();

    ui_candidate_hide (false);
    ui_settle_candidate_window ();
}

/**
 * @brief Hide candidate table slot function for PanelAgent.
 */
static void slot_hide_candidate_table (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_area_1)
        return;

    if (_more_window && evas_object_visible_get (_more_window))
        evas_object_hide (_more_window);

    if (evas_object_visible_get (_candidate_area_1) || evas_object_visible_get (_candidate_area_2)) {
        if (evas_object_visible_get (_candidate_area_1)) {
            evas_object_hide (_candidate_area_1);
            evas_object_hide (_more_btn);
        }
        if (evas_object_visible_get (_candidate_area_2)) {
            evas_object_hide (_candidate_area_2);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
        }
        ui_candidate_window_adjust ();

        ui_candidate_hide (false);
        ui_settle_candidate_window ();
    }
}

/**
 * @brief Update aux slot function for PanelAgent.
 *
 * @param str The new aux string.
 * @param attrs The attribute list of new aux string.
 */
static void slot_update_aux_string (const String &str, const AttributeList &attrs)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_aux_area || (str.length () <= 0))
        return;

    int x, y, width, height, item_width = 0;
    unsigned int window_width = 0, count = 0, i;

    Evas_Object *aux_edje = NULL;

    /* Get selected aux index */
    int    aux_index = -1, aux_start = 0, aux_end = 0;
    String strAux    = str;
    const char *buf  = str.c_str ();
    if (buf[0] >= '0' && buf[0] <= '9') {
        aux_index = buf[0] - 48;
        strAux.erase (0, 1);
    }

    std::vector<String> aux_list;
    scim_split_string_list (aux_list, strAux, '|');

    if (_aux_items.size () > 0) {
        for (i = 0; i < _aux_items.size (); i++)
            evas_object_del (_aux_items [i]);
        _aux_items.clear ();
    }

    for (i = 0; i < aux_list.size (); i++) {
        count++;
        if (i >= _aux_items.size ()) {
            aux_edje = edje_object_add (evas_object_evas_get (_candidate_window));
            edje_object_file_set (aux_edje, _candidate_edje_file.c_str (), "aux");
            edje_object_text_class_set (aux_edje, "aux_text_class", _candidate_font_name.c_str (), _aux_font_size);
            elm_table_pack (_aux_table, aux_edje, i, 0, 1, 1);
            evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER (ISF_EFL_AUX));
            evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
            evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_AUX));
            evas_object_show (aux_edje);
            _aux_items.push_back (aux_edje);
        } else {
            aux_edje = _aux_items [i];
        }
        edje_object_part_text_set (aux_edje, "aux", aux_list [i].c_str ());
        if (i == (unsigned int)aux_index)
            edje_object_signal_emit (aux_edje, "aux,state,selected", "aux");
        else
            edje_object_signal_emit (aux_edje, "aux,state,unselected", "aux");

        evas_object_text_text_set (_tmp_aux_text, aux_list [i].c_str ());
        evas_object_geometry_get (_tmp_aux_text, &x, &y, &width, &height);
        item_width = width + 2*_blank_width;
        item_width = item_width > _item_min_width ? item_width : _item_min_width;
        evas_object_size_hint_min_set (aux_edje, item_width, _aux_height - 2);
        if (aux_index == (int)i || (aux_index == -1 && i == 0)) {
            aux_start = window_width;
            aux_end   = window_width + item_width;
        }
        window_width = window_width + item_width + _blank_width;
    }

    /* Remove redundant aux edje */
    if (count < _aux_items.size ()) {
        for (i = count; i < _aux_items.size (); i++)
            evas_object_del (_aux_items [i]);
        _aux_items.erase (_aux_items.begin () + count, _aux_items.end ());
    }

    int w, h;
    elm_scroller_region_get (_aux_area, &x, &y, &w, &h);
    item_width = aux_end - aux_start;
    if (item_width > 0) {
        if (item_width >= w)
            elm_scroller_region_show (_aux_area, aux_end - w, y, w, h);
        else if (aux_end > x + w)
            elm_scroller_region_show (_aux_area, aux_end - w, y, w, h);
        else if (aux_start < x)
            elm_scroller_region_show (_aux_area, aux_start, y, w, h);
    }
    flush_memory ();
}

/**
 * @brief Update candidate/associate table.
 *
 * @param table_type The table type.
 * @param table The lookup table for candidate or associate.
 * @param table_items The table items for candidate or associate.
 * @param seperator_items The seperator items for candidate or associate.
 */
static void update_table (const int table_type, const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String     mbs;
    WideString wcs;

    int i, x, y, width, height, item_width, item_number, item_0_width;
    int item_num = table.get_current_page_size ();

    {
        _more_list.clear ();

        if (_candidate_angle == 90 || _candidate_angle == 270)
            item_number = _candidate_land_number;
        else
            item_number = _candidate_port_number;

        item_width = (_candidate_scroll_width - 6*_width_rate - (item_number-1)*_h_padding) / item_number;

        Evas *evas = evas_object_evas_get (_candidate_window);
        for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++ i) {
            if (i < item_num) {
                wcs = table.get_candidate_in_current_page (i);
                mbs = utf8_wcstombs (wcs);
                _more_list.push_back (mbs);

                if (_candidate_0 [i]) {
                    evas_object_del (_candidate_0 [i]);
                    _candidate_0 [i] = NULL;
                }

                if (!_candidate_0 [i]) {
                    _candidate_0 [i] = edje_object_add (evas);
                    edje_object_file_set (_candidate_0 [i], _candidate_edje_file.c_str (), _candidate_name.c_str ());
                    edje_object_text_class_set (_candidate_0 [i], "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_max_size);
                    elm_table_pack (_candidate_0_table, _candidate_0 [i], i, 0, 1, 1);
                    evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_0));
                    evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                    evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_0));
                    evas_object_show (_candidate_0 [i]);
                }
                edje_object_part_text_set (_candidate_0 [i], "candidate", mbs.c_str ());
                /* Resize _candidate_0 [i] display width */
                evas_object_text_text_set (_tmp_candidate_text, mbs.c_str ());
                evas_object_geometry_get (_tmp_candidate_text, &x, &y, &width, &height);
                item_0_width = width + 3.5*_blank_width;
                item_0_width = item_0_width > _item_min_width ? item_0_width : _item_min_width;
                evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);

                if (!_candidate_items [i]) {
                    _candidate_items [i] = edje_object_add (evas);
                    edje_object_file_set (_candidate_items [i], _candidate_edje_file.c_str (), _candidate_name.c_str ());
                    edje_object_text_class_set (_candidate_items [i], "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_max_size);
                    evas_object_size_hint_min_set (_candidate_items [i], item_width, _item_min_height);
                    evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_ITEMS));
                    evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                    evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_ITEMS));
                    elm_table_pack (_candidate_table, _candidate_items [i], i%item_number, i/item_number, 1, 1);
                }
                evas_object_show (_candidate_items [i]);
                edje_object_part_text_set (_candidate_items [i], "candidate", mbs.c_str ());
                ui_candidate_text_update_font_size (_candidate_items [i], item_width - 22*_width_rate, mbs);
            } else {
                if (_candidate_0 [i]) {
                    evas_object_del (_candidate_0 [i]);
                    _candidate_0 [i] = NULL;
                }
                if (_candidate_items [i]) {
                    if (i < item_number) {  /* Only hide for first line */
                        evas_object_hide (_candidate_items [i]);
                    } else {
                        evas_object_del (_candidate_items [i]);
                        _candidate_items [i] = NULL;
                    }
                }
            }
        }
        elm_scroller_region_show (_candidate_area_1, 0, 0, _candidate_scroll_width, _item_min_height);
        elm_scroller_region_show (_candidate_area_2, 0, 0, _candidate_scroll_width, item_num * 66 * _height_rate - _v_padding);
        flush_memory ();

        /* Update more window */
        if (_more_window && evas_object_visible_get (_more_window)) {
            _page_no = 1;
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                _pages = _more_list.size () / ISF_LAND_BUTTON_NUMBER;
                if ((_more_list.size () % ISF_LAND_BUTTON_NUMBER) > 0)
                    _pages += 1;
            } else {
                _pages = _more_list.size () / ISF_PORT_BUTTON_NUMBER;
                if ((_more_list.size () % ISF_PORT_BUTTON_NUMBER) > 0)
                    _pages += 1;
            }

            ui_more_window_update ();
        }
    }
}

/**
 * @brief Update candidate table slot function for PanelAgent.
 *
 * @param table The lookup table for candidate.
 */
static void slot_update_candidate_table (const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window || table.get_current_page_size () <= 0)
        return;

    if (!evas_object_visible_get (_candidate_area_1) &&
        !evas_object_visible_get (_candidate_area_2) &&
        !evas_object_visible_get (_more_window)) {
        ui_candidate_show ();
        slot_show_candidate_table ();
    }

    update_table (ISF_CANDIDATE_TABLE, table);
}

/**
 * @brief Set candidate style slot function for PanelAgent.
 *
 * @param style The new candidate style.
 * @param mode The new candidate mode.
 */
static void slot_set_candidate_ui (int style, int mode)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
}

/**
 * @brief Get candidate style slot function for PanelAgent.
 *
 * @param style The current candidate style.
 * @param mode The current candidate mode.
 */
static void slot_get_candidate_ui (int &style, int &mode)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    style = _candidate_style;
    mode  = _candidate_mode;

    if (_more_window_enable == true && _candidate_mode == PORTRAIT_VERTICAL_CANDIDATE_MODE)
        mode = PORTRAIT_MORE_CANDIDATE_MODE;
    else if (_more_window_enable == true && _candidate_mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE)
        mode = LANDSCAPE_MORE_CANDIDATE_MODE;
}

/**
 * @brief Set candidate position slot function for PanelAgent.
 *
 * @param left The new candidate left position.
 * @param top The new candidate top position.
 */
static void slot_set_candidate_position (int left, int top)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _candidate_left = left;
    _candidate_top  = top;

    ui_settle_candidate_window ();
}

/**
 * @brief Get candidate geometry slot function for PanelAgent.
 *
 * @param info The data is used to store candidate position and size.
 */
static void slot_get_candidate_geometry (struct rectinfo &info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int x      = 0;
    int y      = 0;
    int width  = 0;//_candidate_port_width;
    int height = 0;//_candidate_port_height_min;
    if (_candidate_window) {
        /* Get candidate window position */
        ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);
        /* Get exact candidate window size */
        int x2, y2;
        evas_object_geometry_get (_candidate_window, &x2, &y2, &width, &height);
    }
    info.pos_x  = x;
    info.pos_y  = y;
    info.width  = _candidate_width;
    info.height = _candidate_height;
}

/**
 * @brief Set active ISE slot function for PanelAgent.
 *
 * @param ise_uuid The active ISE's uuid.
 */
static void slot_set_active_ise_by_uuid (const String &ise_uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    set_active_ise_by_uuid (ise_uuid, 0, changeDefault);
}

/**
 * @brief Get all ISEs list slot function for PanelAgent.
 *
 * @param list The list is used to store all ISEs.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_list (std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* update ise list */
    isf_load_ise_information (ALL_ISE, _config);
    bool ret = isf_update_ise_list (ALL_ISE, _config);

    std::vector<String> selected_lang;
    isf_get_enabled_languages (selected_lang);
    isf_get_enabled_ise_names_in_languages (selected_lang, list);

    _panel_agent->update_ise_list (list);
    return ret;
}

/**
 * @brief Get keyboard ISEs list slot function for PanelAgent.
 *
 * @param list The list is used to store keyboard ISEs.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_keyboard_ise_list (std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* update ise list */
    isf_load_ise_information (ALL_ISE, _config);
    bool ret = isf_update_ise_list (ALL_ISE, _config);

    std::vector<String> selected_lang;
    isf_get_all_languages (selected_lang);
    isf_get_keyboard_uuids_in_languages (selected_lang, list);

    if (ret)
        _panel_agent->update_ise_list (list);
    return ret;
}

/**
 * @brief Get enable languages list slot function for PanelAgent.
 *
 * @param list The list is used to store languages.
 */
static void slot_get_language_list (std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String lang_name;
    MapStringVectorSizeT::iterator iter = _groups.begin ();

    for (; iter != _groups.end (); iter++) {
        lang_name = scim_get_language_name (iter->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) ==_disabled_langs.end ())
            list.push_back (lang_name);
    }
}

/**
 * @brief Get all languages list slot function for PanelAgent.
 *
 * @param lang The list is used to store languages.
 */
static void slot_get_all_language (std::vector<String> &lang)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    isf_get_all_languages (lang);
}

/**
 * @brief Get specific ISE language list slot function for PanelAgent.
 *
 * @param name The ISE name.
 * @param list The list is used to store ISE languages.
 */
static void slot_get_ise_language (char *name, std::vector<String> &list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (name == NULL)
        return;

    unsigned int num = _names.size ();
    std::vector<String> list_tmp;
    list_tmp.clear ();
    for (unsigned int i = 0; i < num; i++) {
        if (!strcmp (_names[i].c_str (), name)) {
            scim_split_string_list (list_tmp, _langs[i], ',');
            for (i = 0; i < list_tmp.size (); i++)
                list.push_back (scim_get_language_name (list_tmp[i]));
            return;
        }
    }
}

/**
 * @brief Set ISF language slot function for PanelAgent.
 *
 * @param language The ISF language string.
 */
static void slot_set_isf_language (const String &language)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (language.length () <= 0)
        return;

    isf_set_language (language);
}

/**
 * @brief Get ISE information slot function for PanelAgent.
 *
 * @param uuid The ISE uuid.
 * @param info The variable is used to store ISE information.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_info_by_uuid (const String &uuid, ISE_INFO &info)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (!uuid.compare (_uuids[i])) {
            info.uuid   = _uuids[i];
            info.name   = _names[i];
            info.icon   = _icons[i];
            info.lang   = _langs[i];
            info.option = _options[i];
            info.type   = _modes[i];
            return true;
        }
    }

    return false;
}

/**
 * @brief Set keyboard ISE slot function for PanelAgent.
 *
 * @param type The variable should be ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID.
 * @param ise The variable is ISE uuid.
 */
static void slot_set_keyboard_ise (int type, const String &ise)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (ise.length () <= 0)
        return;

    String ise_uuid = ise;
    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, ise_uuid);
    _config->flush ();

    _panel_agent->change_factory (ise_uuid);
}

/**
 * @brief Get current keyboard ISE name and uuid slot function for PanelAgent.
 *
 * @param ise_name The variable is used to store ISE name.
 * @param ise_uuid The variable is used to store ISE uuid.
 */
static void slot_get_keyboard_ise (String &ise_name, String &ise_uuid)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    String uuid     = _config->read (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, String (""));
    if (ise_uuid.length () > 0)
        uuid = ise_uuid;
    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (uuid == _uuids[i]) {
            ise_name = _names[i];
            ise_uuid = uuid;
            return;
        }
    }
    ise_name = String ("");
    ise_uuid = String ("");
}

/**
 * @brief Start default ISE slot function for PanelAgent.
 */
static void slot_start_default_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    DEFAULT_ISE_T default_ise;

    default_ise.type = (TOOLBAR_MODE_T)scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_TYPE), (int)_initial_ise.type);
    default_ise.uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise.uuid);
    default_ise.name = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_NAME), _initial_ise.name);

    if (!set_active_ise_by_uuid (default_ise.uuid, 0, 1)) {
        if (default_ise.uuid != _initial_ise.uuid)
            set_active_ise_by_uuid (_initial_ise.uuid, 0, 1);

        _active_ise = _initial_ise;
    } else {
        _active_ise = default_ise;
    }

    return;
}

/**
 * @brief Accept connection slot function for PanelAgent.
 *
 * @param fd The file descriptor to connect.
 */
static void slot_accept_connection (int fd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_Fd_Handler *panel_agent_read_handler = ecore_main_fd_handler_add (fd, ECORE_FD_READ, panel_agent_handler, NULL, NULL, NULL);
    _read_handler_list.push_back (panel_agent_read_handler);
}

/**
 * @brief Close connection slot function for PanelAgent.
 *
 * @param fd The file descriptor to connect.
 */
static void slot_close_connection (int fd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    int i = 0;
    std::vector<Ecore_Fd_Handler *>::iterator IterPos;

    for (IterPos = _read_handler_list.begin (); IterPos != _read_handler_list.end (); ++IterPos,++i) {
        if (ecore_main_fd_handler_fd_get (_read_handler_list[i]) == fd) {
            ecore_main_fd_handler_del (_read_handler_list[i]);
            _read_handler_list.erase (IterPos);
            break;
        }
    }
}

/**
 * @brief Exit panel process slot function for PanelAgent.
 */
static void slot_exit (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    elm_exit ();
}

//////////////////////////////////////////////////////////////////////
// End of PanelAgent-Functions
//////////////////////////////////////////////////////////////////////


/**
 * @brief Callback function for ecore fd handler.
 *
 * @param data The data to pass to this callback.
 * @param fd_handler The ecore fd handler.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool panel_agent_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    if (fd_handler == NULL)
        return ECORE_CALLBACK_RENEW;

    int fd = ecore_main_fd_handler_fd_get (fd_handler);
    for (unsigned int i = 0; i < _read_handler_list.size (); i++) {
        if (fd_handler == _read_handler_list [i]) {
            if (!_panel_agent->filter_event (fd)) {
                std::cerr << "_panel_agent->filter_event () is failed!!!\n";
                ecore_main_fd_handler_del (fd_handler);
            }
            return ECORE_CALLBACK_RENEW;
        }
    }
    std::cerr << "panel_agent_handler () has received exception event!!!\n";
    _panel_agent->filter_exception_event (fd);
    ecore_main_fd_handler_del (fd_handler);
    return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Callback function for abnormal signal.
 *
 * @param sig The signal.
 */
static void signalhandler (int sig)
{
    SCIM_DEBUG_MAIN (1) << __FUNCTION__ << "...\n";

    if (_panel_agent != NULL)
        _panel_agent->stop ();
    elm_exit ();
}

#if HAVE_VCONF
/**
 * @brief Callback function for setting theme change.
 *
 * @param key The key node.
 * @param data The data to pass to this callback.
 */
static void setting_theme_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    char *theme_str = vconf_get_str (VCONFKEY_THEME);
    if (!theme_str) return;

    if (String (theme_str) == "tizen-black" || String (theme_str) == "tizen-black-hd")
        _candidate_edje_file = String (EFL_CANDIDATE_THEME1);
    else if (String (theme_str) == "tizen" || String (theme_str) == "tizen-hd")
        _candidate_edje_file = String (EFL_CANDIDATE_THEME2);

    free (theme_str);
}

/**
 * @brief Update keyboard ISE name when display language is changed.
 *
 * @param module_name The keyboard ISE module name.
 * @param index The index of _module_names.
 *
 * @return true if suceessful, otherwise return false.
 */
static bool update_keyboard_ise_locale (const String module_name, int index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (module_name.length () <= 0 || module_name == "socket")
        return false;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;
    ime_module.load (module_name, _config);

    if (ime_module.valid ()) {
        for (size_t j = 0; j < ime_module.number_of_factories (); ++j) {
            try {
                factory = ime_module.create_factory (j);
            } catch (...) {
                factory.reset ();
            }
            if (!factory.null ()) {
                _names[index+j] = utf8_wcstombs (factory->get_name ());
                factory.reset ();
            }
        }
        ime_module.unload ();
    } else {
        std::cerr << module_name << " can not be loaded!!!\n";
    }

    return true;
}

/**
 * @brief Update helper ISE name when display language is changed.
 *
 * @param module_name The helper ISE module name.
 * @param index The index of _module_names.
 *
 * @return true if suceessful, otherwise return false.
 */
static bool update_helper_ise_locale (const String module_name, int index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (module_name.length () <= 0)
        return false;

    HelperModule helper_module;
    HelperInfo   helper_info;
    helper_module.load (module_name);
    if (helper_module.valid ()) {
        for (size_t j = 0; j < helper_module.number_of_helpers (); ++j) {
            helper_module.get_helper_info (j, helper_info);
            _names[index+j] = helper_info.name;
        }
        helper_module.unload ();
    } else {
        std::cerr << module_name << " can not be loaded!!!\n";
    }

    return true;
}

/**
 * @brief Set language and locale.
 *
 * @return void
 */
static void set_language_and_locale (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    char *lang_str = vconf_get_str (VCONFKEY_LANGSET);

    if (lang_str) {
        setenv ("LANG", lang_str, 1);
        setlocale (LC_MESSAGES, lang_str);
        free(lang_str);
    } else {
        setenv ("LANG", "en_US.utf8", 1);
        setlocale (LC_MESSAGES, "en_US.utf8");
    }
}

/**
 * @brief Callback function for display language change.
 *
 * @param key The key node.
 * @param data The data to pass to this callback.
 *
 * @return void
 */
static void display_language_changed_cb (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    set_language_and_locale ();

    /* update the active ise and  all the ises names in new locale */
    std::vector<String> module_list;
    for (unsigned int i = 0; i < _module_names.size (); i++) {
        if (std::find (module_list.begin (), module_list.end (), _module_names[i]) != module_list.end ())
            continue;
        module_list.push_back (_module_names[i]);
        if (_module_names[i] == String (ENGLISH_KEYBOARD_MODULE)) {
            IMEngineFactoryPointer factory;
            factory = new ComposeKeyFactory ();
            _names[i] = utf8_wcstombs (factory->get_name ());
            factory.reset ();
        } else if (_modes[i] == TOOLBAR_KEYBOARD_MODE) {
            update_keyboard_ise_locale (_module_names[i], i);
        } else if (_modes[i] == TOOLBAR_HELPER_MODE) {
            update_helper_ise_locale (_module_names[i], i);
        }
    }
    isf_save_ise_information ();

    String name = get_ise_name (_panel_agent->get_current_helper_uuid ());
    _panel_agent->set_current_ise_name (name);
}

/**
 * @brief Callback function for hibernation enter.
 *
 * @param data Data to pass when it is called.
 *
 * @return void
 */
static void hibernation_enter_cb (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Remove callback function for input language and display language */
    vconf_ignore_key_changed (VCONFKEY_LANGSET, display_language_changed_cb);
    vconf_ignore_key_changed (VCONFKEY_THEME, setting_theme_changed_cb);
}

/**
 * @brief Callback function for hibernation leave.
 *
 * @param data Data to pass when it is called.
 *
 * @return void
 */
static void hibernation_leave_cb (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (data == NULL) {
        if (!_config.null ())
            _config->reload ();
    }
    /* Add callback function for input language and display language */
    vconf_notify_key_changed (VCONFKEY_LANGSET, display_language_changed_cb, NULL);
    vconf_notify_key_changed (VCONFKEY_THEME, setting_theme_changed_cb, NULL);
    setting_theme_changed_cb (NULL, NULL);

    scim_global_config_update ();
    set_language_and_locale ();

    try {
        /* update ise list */
        std::vector<String> list;
        slot_get_ise_list (list);

        /* Start default ISE */
        slot_start_default_ise ();
        change_hw_and_sw_keyboard ();
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
    }
}
#endif

/**
 * @brief Callback function for X event client message.
 *
 * @param data Data to pass when it is called.
 * @param type The event type.
 * @param event The information for current message.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool x_event_client_message_cb (void *data, int type, void *event)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message *)event;
    if (ev->message_type == ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE) {
        int angle = ev->data.l[0];
        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " : ANGLE (" << angle << ")\n";
        ui_candidate_hide (true);
        if (_candidate_window && _candidate_angle != angle) {
            _candidate_angle = angle;
            ui_candidate_window_rotate (angle);
        }
    }

    return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Change hw and sw keyboard.
 *
 * @return void
 */
static void change_hw_and_sw_keyboard (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    unsigned int val = 0;

    _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (ecore_x_window_prop_card32_get (ecore_x_window_root_first_get (), ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST), &val, 1)) {
        if (val != 0) {
            /* Currently active the hw ise directly */
            String uuid, name;
            isf_get_keyboard_ise (uuid, name, _config);
            set_active_ise_by_uuid (uuid, 0, 1);

            _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 1);
        } else {
            String previous_helper = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, _initial_ise.uuid);
            set_active_ise_by_uuid (previous_helper, 0, 1);
        }
    }
    _config->reload ();
}

/**
 * @brief Callback function for ECORE_X_EVENT_WINDOW_PROPERTY.
 *
 * @param data Data to pass when it is called.
 * @param ev_type The event type.
 * @param ev The information for current message.
 *
 * @return ECORE_CALLBACK_PASS_ON
 */
static Eina_Bool x_event_property_change_cb (void *data, int ev_type, void *ev)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Event_Window_Property *event = (Ecore_X_Event_Window_Property *)ev;

    Ecore_X_Window rootwin = ecore_x_window_root_first_get ();
    if (event->win == rootwin && event->atom == ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST)) {
        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
        change_hw_and_sw_keyboard ();
    }

    return ECORE_CALLBACK_PASS_ON;
}

int main (int argc, char *argv [])
{
    struct tms    tiks_buf;
    _clock_start = times (&tiks_buf);

    int           i, ret1, hib_fd;
#ifdef WAIT_WM
    int           try_count       = 0;
#endif
    size_t        j;
    int           ret             = 0;

    bool          daemon          = false;
    bool          should_resident = true;

    int           new_argc        = 0;
    char        **new_argv        = new char * [40];
    ConfigModule *config_module   = NULL;
    String        config_name     = String ("socket");
    String        display_name    = String ();
    char         *clang           = NULL;

    Ecore_Fd_Handler *panel_agent_read_handler = NULL;
    Ecore_Event_Handler *xclient_msg_handler = NULL;
    Ecore_Event_Handler *prop_change_handler = NULL;

    control_privilege ();

    check_time ("\nStarting ISF Panel EFL...... ");

    DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
    DebugOutput::enable_debug (SCIM_DEBUG_MainMask);

    /* Parse command options */
    i = 0;
    while (i < argc) {
        if (++i >= argc)
            break;

        if (String ("-c") == argv [i] || String ("--config") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            config_name = argv [i];
            continue;
        }

        if (String ("-h") == argv [i] || String ("--help") == argv [i]) {
            std::cout << "Usage: " << argv [0] << " [option]...\n\n"
                 << "The options are: \n"
                 << "  --display DISPLAY    Run on display DISPLAY.\n"
                 << "  -c, --config NAME    Uses specified Config module.\n"
                 << "  -d, --daemon         Run " << argv [0] << " as a daemon.\n"
                 << "  -ns, --no-stay       Quit if no connected client.\n"
#if ENABLE_DEBUG
                 << "  -v, --verbose LEVEL  Enable debug info, to specific LEVEL.\n"
                 << "  -o, --output FILE    Output debug information into FILE.\n"
#endif
                 << "  -h, --help           Show this help message.\n";
            delete []new_argv;
            return 0;
        }

        if (String ("-d") == argv [i] || String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("-ns") == argv [i] || String ("--no-stay") == argv [i]) {
            should_resident = false;
            continue;
        }

        if (String ("-v") == argv [i] || String ("--verbose") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            DebugOutput::set_verbose_level (atoi (argv [i]));
            continue;
        }

        if (String ("-o") == argv [i] || String ("--output") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            DebugOutput::set_output (argv [i]);
            continue;
        }

        if (String ("--display") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "No argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            display_name = argv [i];
            continue;
        }

        if (String ("--") == argv [i])
            break;

        std::cerr << "Invalid command line option: " << argv [i] << "\n";
        delete []new_argv;
        return 0;
    } /* End of command line parsing. */

    new_argv [new_argc ++] = argv [0];

    /* Store the rest argvs into new_argv. */
    for (++i; i < argc && new_argc < 37; ++i) {
        new_argv [new_argc ++] = argv [i];
    }

    /* Make up DISPLAY env. */
    if (display_name.length ()) {
        new_argv [new_argc ++] = const_cast <char*> ("--display");
        new_argv [new_argc ++] = const_cast <char*> (display_name.c_str ());

        setenv ("DISPLAY", display_name.c_str (), 1);
    }

    new_argv [new_argc] = 0;

    if (!config_name.length ()) {
        std::cerr << "No Config module is available!\n";
        ret = -1;
        goto cleanup;
    }

    if (config_name != "dummy") {
        /* Load config module */
        config_module = new ConfigModule (config_name);

        if (!config_module || !config_module->valid ()) {
            std::cerr << "Can not load " << config_name << " Config module.\n";
            ret = -1;
            goto cleanup;
        }
    } else {
        _config = new DummyConfig ();
    }

    signal (SIGQUIT, signalhandler);
    signal (SIGTERM, signalhandler);
    signal (SIGINT,  signalhandler);

    setenv ("ELM_FPS", "6000", 1);
    setenv ("ELM_ENGINE", "software_x11", 1); /* to avoid the inheritance of ELM_ENGINE */
    set_language_and_locale ();

#ifdef WAIT_WM
    while (1) {
        if (check_file ("/tmp/.wm_ready"))
            break;

        if (try_count == 100) {
            std::cerr << "[ISF-PANEL-EFL] Timeout. cannot check the state of window manager....\n";
            break;
        }

        try_count++;
        usleep (50000);
    }
#endif

    elm_init (argc, argv);
    check_time ("elm_init");

#if HAVE_VCONF
    hib_fd = heynoti_init ();

    /* register hibernation callback */
    heynoti_subscribe (hib_fd, "HIBERNATION_ENTER", hibernation_enter_cb, NULL);
    heynoti_subscribe (hib_fd, "HIBERNATION_LEAVE", hibernation_leave_cb, NULL);
#endif

    /* Get current display. */
    {
        const char *p = getenv ("DISPLAY");
        if (p)
            display_name = String (p);
    }

    char buf [256];
    snprintf (buf, sizeof (buf), "config_name=%s display_name=%s", config_name.c_str (), display_name.c_str ());
    check_time (buf);
    try {
        if (!initialize_panel_agent (config_name, display_name, should_resident)) {
            check_time ("Failed to initialize Panel Agent!");
            std::cerr << "Failed to initialize Panel Agent!\n";
            ret = -1;
            goto cleanup;
        }
    } catch (scim::Exception & e) {
        std::cerr << e.what() << "\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("initialize_panel_agent");

    /* Create config instance */
    if (_config.null () && config_module && config_module->valid ())
        _config = config_module->create_config ();
    if (_config.null ()) {
        std::cerr << "Failed to create Config instance from " << config_name << " Config module.\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("create config instance");

    /* Initialize global variables and pointers for candidate items and etc. */
    for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; i++) {
        _candidate_0 [i]     = NULL;
        _candidate_items [i] = NULL;
    }

    efl_get_screen_size (_screen_width, _screen_height);

    _width_rate       = (float)(_screen_width / 480.0);
    _height_rate      = (float)(_screen_height / 800.0);
    _blank_width      = (int)(_blank_width * _width_rate);
    _item_min_width   = (int)(_item_min_width * _width_rate);
    _item_min_height  = (int)(_item_min_height * _height_rate);
    _aux_font_size    = (int)(_aux_font_size * _width_rate);

    _button_font_size        = (int)(_button_font_size * _width_rate);
    _candidate_font_max_size = (int)(_candidate_font_max_size * _width_rate);
    _candidate_font_min_size = (int)(_candidate_font_min_size * _width_rate);

    _more_port_height = (int)(_more_port_height * _height_rate);
    _more_land_height = (int)(_more_land_height * _width_rate);

    /* Load ISF configuration */
    load_config ();
    check_time ("load_config");

    try {
        _panel_agent->send_display_name (display_name);
    } catch (scim::Exception & e) {
        std::cerr << e.what() << "\n";
        ret = -1;
        goto cleanup;
    }

    _initial_ise.type = (TOOLBAR_MODE_T)scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_TYPE), (int)TOOLBAR_HELPER_MODE);
    _initial_ise.uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String ("ff110940-b8f0-4062-9ff6-a84f4f3575c0"));
    _initial_ise.name = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_NAME), String ("Input Pad"));

    for (j = 0; j < _uuids.size (); ++j) {
        if (!_uuids[j].compare (_initial_ise.uuid))
            break;
    }

    if (j == _uuids.size ()) {
        for (j = 0; j < _uuids.size (); ++j) {
            if (_modes[j] == TOOLBAR_HELPER_MODE) {
                _initial_ise.type = _modes[j];
                _initial_ise.uuid = _uuids[j];
                _initial_ise.name = _names[j];
            }
        }
    }

    if (daemon) {
        check_time ("ISF Panel EFL run as daemon");
        scim_daemon ();
    }

    /* Connect the configuration reload signal. */
    _config->signal_connect_reload (slot (config_reload_cb));

    panel_agent_read_handler = ecore_main_fd_handler_add (_panel_agent->get_server_id (), ECORE_FD_READ, panel_agent_handler, NULL, NULL, NULL);
    _read_handler_list.push_back (panel_agent_read_handler);
    check_time ("run_panel_agent");

#if HAVE_VCONF
    ret1 = heynoti_attach_handler (hib_fd);
    if (ret1 == -1)
        std::cerr << "heynoti_attach_handler () is failed!!!\n";

    clang = vconf_get_str (VCONFKEY_ISF_INPUT_LANG_STR);
    if (clang == NULL) {
        std::cerr << "\nGet vconf key value failed, set active language to Automatic...\n";
        isf_set_language ("Automatic");
    } else {
        isf_set_language (String (clang));
    }

    if (!ecore_file_exists ("/opt/etc/.hib_capturing")) {
        /* in case of normal booting */
        hibernation_leave_cb ((void *)1);
    }

    /* set hibernation ready state flag */
    vconf_set_int ("memory/hibernation/isf_ready", 1);
#endif

    xclient_msg_handler = ecore_event_handler_add (ECORE_X_EVENT_CLIENT_MESSAGE, x_event_client_message_cb, NULL);
    ecore_x_event_mask_set (ecore_x_window_root_first_get (), ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
    prop_change_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_PROPERTY, x_event_property_change_cb, NULL);

    /* Set elementary scale */
    {
        Evas_Coord win_w = 0, win_h = 0;
        ecore_x_window_size_get (ecore_x_window_root_first_get (), &win_w, &win_h);
        if (win_w) {
            elm_config_scale_set (win_w / 720.0f);
        }
    }

    check_time ("EFL Panel launch time");

    elm_run ();
    elm_shutdown ();

    _config->flush ();
    ret = 0;

    ecore_event_handler_del (xclient_msg_handler);
    ecore_event_handler_del (prop_change_handler);
#if HAVE_VCONF
    hibernation_enter_cb (NULL);
    heynoti_close (hib_fd);
#endif

cleanup:
    if (!_config.null ())
        _config.reset ();
    if (config_module)
        delete config_module;
    if (_panel_agent)
        delete _panel_agent;

    delete []new_argv;

    if (ret == 0) {
        std::cerr << "Successfully exited.\n";
        return 0;
    } else {
        std::cerr << "Abnormally exited.\n";
        return -1;
    }
}

/*
vi:ts=4:nowrap:expandtab
*/
