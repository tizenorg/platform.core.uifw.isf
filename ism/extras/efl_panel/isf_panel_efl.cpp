/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd. All rights reserved.
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

#define ISF_CONFIG_PANEL_FIXED_FOR_HARDWARE_KBD         "/Panel/Hardware/Fixed"

#define ISF_CANDIDATE_TABLE                             0

#define ISF_EFL_AUX                                     1
#define ISF_EFL_CANDIDATE_0                             2
#define ISF_EFL_CANDIDATE_ITEMS                         3

#define ISE_DEFAULT_HEIGHT_PORTRAIT                     444
#define ISE_DEFAULT_HEIGHT_LANDSCAPE                    316

#define ISF_READY_FILE                                  "/tmp/hibernation/isf_ready"


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

extern std::vector<String>          _load_ise_list;

extern CommonLookupTable            g_isf_candidate_table;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal data types.
/////////////////////////////////////////////////////////////////////////////
typedef enum _VIRTUAL_KEYBOARD_STATE {
    KEYBOARD_STATE_UNKNOWN = 0,
    KEYBOARD_STATE_OFF,
    KEYBOARD_STATE_ON,
} VIRTUAL_KEYBOARD_STATE;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object *efl_create_window                  (const char *strWinName, const char *strEffect);
static void       efl_set_transient_for_app_window     (Evas_Object *win_obj);
static int        efl_get_angle_for_root_window        (Evas_Object *win_obj);

static int        ui_candidate_get_valid_height        (void);
static void       ui_candidate_hide                    (bool bForce);
static void       ui_destroy_candidate_window          (void);
static void       ui_settle_candidate_window           (void);
static void       ui_candidate_show                    (void);
static void       update_table                         (int table_type, const LookupTable &table);

/* PanelAgent related functions */
static bool       initialize_panel_agent               (const String &config, const String &display, bool resident);

static void       slot_reload_config                   (void);
static void       slot_focus_in                        (void);
static void       slot_focus_out                       (void);
static void       slot_expand_candidate                (void);
static void       slot_contract_candidate              (void);
static void       slot_set_candidate_style             (int display_line, int reserved);
static void       slot_update_input_context            (int type, int value);
static void       slot_update_ise_geometry             (int x, int y, int width, int height);
static void       slot_update_spot_location            (int x, int y, int top_y);
static void       slot_update_factory_info             (const PanelFactoryInfo &info);
static void       slot_show_aux_string                 (void);
static void       slot_show_candidate_table            (void);
static void       slot_hide_aux_string                 (void);
static void       slot_hide_candidate_table            (void);
static void       slot_update_aux_string               (const String &str, const AttributeList &attrs);
static void       slot_update_candidate_table          (const LookupTable &table);
static void       slot_set_active_ise                  (const String &uuid, bool changeDefault);
static bool       slot_get_ise_list                    (std::vector<String> &name);
static bool       slot_get_keyboard_ise_list           (std::vector<String> &name_list);
static void       slot_get_language_list               (std::vector<String> &name);
static void       slot_get_all_language                (std::vector<String> &lang);
static void       slot_get_ise_language                (char *name, std::vector<String> &list);
static bool       slot_get_ise_info                    (const String &uuid, ISE_INFO &info);
static void       slot_get_candidate_geometry          (struct rectinfo &info);
static void       slot_get_input_panel_geometry        (struct rectinfo &info);
static void       slot_set_keyboard_ise                (const String &uuid);
static void       slot_get_keyboard_ise                (String &ise_name, String &ise_uuid);
static void       slot_start_default_ise               (void);
static void       slot_accept_connection               (int fd);
static void       slot_close_connection                (int fd);
static void       slot_exit                            (void);

static Eina_Bool  panel_agent_handler                  (void *data, Ecore_Fd_Handler *fd_handler);


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object       *_candidate_window                 = 0;
static Evas_Object       *_candidate_area_1                 = 0;
static Evas_Object       *_candidate_area_2                 = 0;
static Evas_Object       *_candidate_bg                     = 0;
static Evas_Object       *_candidate_0_scroll               = 0;
static Evas_Object       *_candidate_scroll                 = 0;
static Evas_Object       *_scroller_bg                      = 0;
static Evas_Object       *_candidate_0_table                = 0;
static Evas_Object       *_candidate_table                  = 0;
static Evas_Object       *_candidate_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_candidate_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_seperate_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_seperate_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_line_0 [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_line_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];
static Evas_Object       *_more_btn                         = 0;
static Evas_Object       *_close_btn                        = 0;
static bool               _candidate_window_show            = false;

static int                _candidate_x                      = 0;
static int                _candidate_y                      = 0;
static int                _candidate_width                  = 0;
static int                _candidate_height                 = 0;
static int                _candidate_valid_height           = 0;

static int                _candidate_port_line              = 1;
static int                _candidate_port_width             = 480;
static int                _candidate_port_height_min        = 76;
static int                _candidate_port_height_min_2      = 150;
static int                _candidate_port_height_max        = 286;
static int                _candidate_port_height_max_2      = 350;
static int                _candidate_land_width             = 784;
static int                _candidate_land_height_min        = 84;
static int                _candidate_land_height_min_2      = 168;
static int                _candidate_land_height_max        = 150;
static int                _candidate_land_height_max_2      = 214;
static int                _candidate_area_1_pos [2]         = {0, 2};
static int                _more_btn_pos [4]                 = {369, 11, 689, 11};
static int                _close_btn_pos [4]                = {362, 211, 682, 75};

static int                _v_padding                        = 2;
static int                _item_min_width                   = 99;
static int                _item_min_height                  = 82;

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

static int                _spot_location_x                  = -1;
static int                _spot_location_y                  = -1;
static int                _spot_location_top_y              = -1;
static int                _candidate_angle                  = 0;

static int                _ise_width                        = -1;
static int                _ise_height                       = -1;
static bool               _ise_show                         = false;

static int                _indicator_height                 = 0;//24;
static int                _screen_width                     = 720;
static int                _screen_height                    = 1280;
static float              _width_rate                       = 1.0;
static float              _height_rate                      = 1.0;
static int                _blank_width                      = 30;

static String             _candidate_name                   = String ("candidate");
static String             _candidate_edje_file              = String (EFL_CANDIDATE_THEME1);

static String             _candidate_font_name              = String ("SLP");
static int                _candidate_font_size              = 38;
static int                _aux_font_size                    = 38;
static int                _click_object                     = 0;
static int                _click_down_pos [2]               = {0, 0};
static int                _click_up_pos [2]                 = {0, 0};
static bool               _is_click                         = true;

static DEFAULT_ISE_T      _initial_ise;
static ConfigPointer      _config;
static PanelAgent        *_panel_agent                      = 0;
static std::vector<Ecore_Fd_Handler *> _read_handler_list;

static clock_t            _clock_start;

static Ecore_Timer       *_check_size_timer                 = NULL;
static Ecore_Timer       *_longpress_timer                  = NULL;
static Ecore_Timer       *_destroy_timer                    = NULL;


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
 * @brief Get ISE geometry information.
 *
 * @param info The data is used to store ISE position and size.
 * @param kbd_state The keyboard state.
 */
static void get_ise_geometry (RECT_INFO &info, VIRTUAL_KEYBOARD_STATE kbd_state)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_ise_width == -1 && _ise_height == -1) {
        _panel_agent->get_current_ise_geometry (info);
    } else {
        info.width  = _ise_width;
        info.height = _ise_height;
    }

    int win_w = _screen_width, win_h = _screen_height;
    int angle = efl_get_angle_for_root_window (_candidate_window);
    if (angle == 90 || angle == 270) {
        win_w = _screen_height;
        win_h = _screen_width;
    }

    if (win_w != (int)info.width)
        _panel_agent->get_current_ise_geometry (info);

    if ((int)info.width > win_w) {
        win_w = _screen_height;
        win_h = _screen_width;
    }

    info.pos_x = (int)info.width > win_w ? 0 : (win_w - info.width) / 2;
    if (kbd_state == KEYBOARD_STATE_OFF)
        info.pos_y = win_h;
    else
        info.pos_y = win_h - info.height;
}

/**
 * @brief Set keyboard geometry for autoscroll.
 *
 * @param kbd_state The keyboard state.
 */
static void set_keyboard_geometry_atom_info (VIRTUAL_KEYBOARD_STATE kbd_state)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (kbd_state == KEYBOARD_STATE_UNKNOWN)
        return;

    Ecore_X_Window* zone_lists;
    if (ecore_x_window_prop_window_list_get (ecore_x_window_root_first_get (),
            ECORE_X_ATOM_E_ILLUME_ZONE_LIST, &zone_lists) > 0) {

        struct rectinfo info = {0, 0, 0, 0};
        if (_ise_width == 0 && _ise_height == 0) {
            info.pos_x = 0;
            if (_candidate_window && evas_object_visible_get (_candidate_window)) {
                info.width  = _candidate_width;
                info.height = _candidate_height;
            }
            int angle = efl_get_angle_for_root_window (_candidate_window);
            if (angle == 90 || angle == 270)
                info.pos_y = _screen_width - info.height;
            else
                info.pos_y = _screen_height - info.height;
        } else {
            get_ise_geometry (info, kbd_state);
            if (_ise_width == -1 && _ise_height == -1) {
                ; // Floating style
            } else {
                if (_candidate_window && evas_object_visible_get (_candidate_window)) {
                    _candidate_valid_height = ui_candidate_get_valid_height ();
                    if ((_candidate_height - _candidate_valid_height) > _ise_height) {
                        _candidate_valid_height = _candidate_height;
                        info.pos_y  = info.pos_y + info.height - _candidate_height;
                        info.height = _candidate_height;
                    } else {
                        info.pos_y  -= _candidate_valid_height;
                        info.height += _candidate_valid_height;
                    }
                }
            }
        }
        if (kbd_state == KEYBOARD_STATE_ON) {
            ecore_x_e_virtual_keyboard_state_set (zone_lists[0], ECORE_X_VIRTUAL_KEYBOARD_STATE_ON);
            ecore_x_e_illume_keyboard_geometry_set (zone_lists[0], info.pos_x, info.pos_y, info.width, info.height);
        } else {
            ecore_x_e_virtual_keyboard_state_set (zone_lists[0], ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF);
            ecore_x_e_illume_keyboard_geometry_set (zone_lists[0], info.pos_x, info.pos_y, 0, 0);
        }
    }
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
 * @brief Set keyboard ISE.
 *
 * @param uuid The keyboard ISE's uuid.
 * @param name The keyboard ISE's name.
 *
 * @return false if keyboard ISE change is failed, otherwise return true.
 */
static bool set_keyboard_ise (const String &uuid, const String &name)
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

    slot_set_keyboard_ise (uuid);

    return true;
}

/**
 * @brief Set helper ISE.
 *
 * @param uuid The helper ISE's uuid.
 * @param changeDefault The flag for change default ISE.
 *
 * @return false if helper ISE change is failed, otherwise return true.
 */
static bool set_helper_ise (const String &uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        if (!pre_uuid.compare (uuid))
            return false;
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
    }

    String ise_uuid = SCIM_COMPOSE_KEY_FACTORY_UUID;
    slot_set_keyboard_ise (ise_uuid);

    _ise_width  = -1;
    _ise_height = -1;
    _ise_show   = false;
    _candidate_port_line = 1;
    _panel_agent->start_helper (uuid);
    if (changeDefault) {
        _config->write (String (SCIM_CONFIG_DEFAULT_HELPER_ISE), uuid);
        _config->flush ();
    }

    return true;
}

/**
 * @brief Set active ISE.
 *
 * @param uuid The ISE's uuid.
 * @param changeDefault It indicates whether call _panel_agent->set_default_ise ().
 *
 * @return false if ISE change is failed, otherwise return true.
 */
static bool set_active_ise (const String &uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (uuid.length () <= 0)
        return false;

    bool ise_changed = false;

    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (!uuid.compare (_uuids[i])) {
            if (TOOLBAR_KEYBOARD_MODE == _modes[i])
                ise_changed = set_keyboard_ise (_uuids[i], _names[i]);
            else if (TOOLBAR_HELPER_MODE == _modes[i])
                ise_changed = set_helper_ise (_uuids[i], changeDefault);

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

    /* Read configurations. */
    if (!_config.null ()) {
        bool shared_ise = _config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), false);
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
 * @brief Get candidate window valid height for autoscroll.
 *
 * @return The valid height.
 */
static int ui_candidate_get_valid_height (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "\n";

    int height = 0;
    if (_candidate_window) {
        int angle = efl_get_angle_for_root_window (_candidate_window);

        if (evas_object_visible_get (_aux_area) && evas_object_visible_get (_candidate_area_1)) {
            if (angle == 90 || angle == 270)
                height = _candidate_land_height_min_2;
            else
                height = _candidate_port_height_min_2;
        } else {
            if (angle == 90 || angle == 270)
                height = _candidate_land_height_min;
            else
                height = _candidate_port_height_min;
        }
    }
    return height;
}

/**
 * @brief Resize candidate window size.
 *
 * @param new_width  New width for candidate window.
 * @param new_height New height for candidate window.
 */
static void ui_candidate_window_resize (int new_width, int new_height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " width:" << new_width << " height:" << new_height << "\n";

    if (!_candidate_window)
        return;

    int x, y, width, height;
    evas_object_geometry_get (_candidate_window, &x, &y, &width, &height);
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " old width:" << width << " old height:" << height << "\n";

    if (width == new_width && height == new_height)
        return;

    evas_object_resize (_candidate_window, new_width, new_height);
    evas_object_resize (_aux_line, new_width, 2);
    _candidate_width  = new_width;
    _candidate_height = new_height;
    if (evas_object_visible_get (_candidate_window))
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

    if (evas_object_visible_get (_candidate_window)) {
        height = ui_candidate_get_valid_height ();
        if ((_ise_width == 0 && _ise_height == 0) ||
            (_ise_height > 0 && _candidate_valid_height != height) ||
            (_ise_height > 0 && (_candidate_height - height) > _ise_height)) {
            set_keyboard_geometry_atom_info (KEYBOARD_STATE_ON);
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
        }
    }
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
        if (evas_object_visible_get (_aux_area) && evas_object_visible_get (_candidate_area_2)) {
            evas_object_show (_aux_line);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_max_2);
                evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
                evas_object_move (_candidate_area_2, 0, _candidate_land_height_min_2);
                evas_object_move (_scroller_bg, 0, _candidate_land_height_min_2);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_max_2);
                evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
                evas_object_move (_candidate_area_2, 0, _candidate_port_height_min_2);
                evas_object_move (_scroller_bg, 0, _candidate_port_height_min_2);
            }
        } else if (evas_object_visible_get (_aux_area) && evas_object_visible_get (_candidate_area_1)) {
            evas_object_show (_aux_line);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_min_2);
                evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_min_2);
                evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
            }
        } else if (evas_object_visible_get (_aux_area)) {
            evas_object_hide (_aux_line);
            ui_candidate_window_resize (width, _aux_height + 2);
        } else if (evas_object_visible_get (_candidate_area_2)) {
            evas_object_hide (_aux_line);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_max);
                evas_object_move (_close_btn, _close_btn_pos[2], _close_btn_pos[3]);
                evas_object_move (_candidate_area_2, 0, _candidate_land_height_min);
                evas_object_move (_scroller_bg, 0, _candidate_land_height_min);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_max);
                evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
                evas_object_move (_candidate_area_2, 0, _candidate_port_height_min);
                evas_object_move (_scroller_bg, 0, _candidate_port_height_min);
            }
        } else {
            evas_object_hide (_aux_line);
            evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
            if (_candidate_angle == 90 || _candidate_angle == 270) {
                ui_candidate_window_resize (width, _candidate_land_height_min);
                evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3]);
            } else {
                ui_candidate_window_resize (width, _candidate_port_height_min);
                evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
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

    ui_candidate_hide (true);
    elm_win_rotation_set (_candidate_window, angle);
    if (angle == 90 || angle == 270) {
        _candidate_scroll_width = _candidate_scroll_width_max;
        ui_candidate_window_resize (_candidate_land_width, _candidate_land_height_min);
        evas_object_resize (_aux_area, _aux_land_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_max, _item_min_height);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_min);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_min + 6);
    } else {
        _candidate_scroll_width = _candidate_scroll_width_min;
        ui_candidate_window_resize (_candidate_port_width, _candidate_port_height_min);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_resize (_candidate_area_1, _candidate_scroll_0_width_min, (_item_min_height+2)*_candidate_port_line-2);
        evas_object_resize (_candidate_area_2, _candidate_scroll_width, _candidate_scroll_height_max);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
    }

    ui_settle_candidate_window ();
    ui_candidate_window_adjust ();
    if (evas_object_visible_get (_candidate_area_1)) {
        update_table (ISF_CANDIDATE_TABLE, g_isf_candidate_table);
        ui_candidate_show ();
    }
    flush_memory ();
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
 * @brief Delete check candidate window size timer.
 *
 * @return void
 */
static void ui_candidate_delete_check_size_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_check_size_timer != NULL) {
        ecore_timer_del (_check_size_timer);
        _check_size_timer = NULL;
    }
}

/**
 * @brief Callback function for check candidate window size timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_check_size_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_delete_check_size_timer ();
    ui_candidate_window_resize (_candidate_width, _candidate_height);
    ui_settle_candidate_window ();
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete longpress timer.
 *
 * @return void
 */
static void ui_candidate_delete_longpress_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_longpress_timer != NULL) {
        ecore_timer_del (_longpress_timer);
        _longpress_timer = NULL;
    }
}

/**
 * @brief Callback function for candidate longpress timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_longpress_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int index = (int)GPOINTER_TO_INT(data);
    ui_candidate_delete_longpress_timer ();
    _is_click = false;
    _panel_agent->send_longpress_event (_click_object, index);
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete destroy timer.
 *
 * @return void
 */
static void ui_candidate_delete_destroy_timer (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_destroy_timer != NULL) {
        ecore_timer_del (_destroy_timer);
        _destroy_timer = NULL;
    }
}

/**
 * @brief Callback function for destroy timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool ui_candidate_destroy_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_delete_destroy_timer ();
    ui_destroy_candidate_window ();
    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Show candidate window.
 */
static void ui_candidate_show (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (!_candidate_window || (hw_kbd_detect == 0 && !_ise_show))
        return;

    int angle = efl_get_angle_for_root_window (_candidate_window);
    if (_candidate_angle != angle) {
        _candidate_angle = angle;
        ui_candidate_window_rotate (angle);
    }

    if (!evas_object_visible_get (_candidate_window)) {
        evas_object_show (_candidate_window);
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_SHOW);

        if (!(_ise_width == -1 && _ise_height == -1)) {
            set_keyboard_geometry_atom_info (KEYBOARD_STATE_ON);
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
        }

        ui_candidate_delete_check_size_timer ();
        _check_size_timer = ecore_timer_add (0.02, ui_candidate_check_size_timeout, NULL);
    }

    SCIM_DEBUG_MAIN (3) << "    Show candidate window\n";
    _candidate_window_show = true;
    evas_object_show (_candidate_window);
    efl_set_transient_for_app_window (_candidate_window);
}

/**
 * @brief Hide candidate window.
 *
 * @param bForce The flag to hide candidate window by force.
 */
static void ui_candidate_hide (bool bForce)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (!_candidate_window)
        return;

    if (bForce) {
        if (_candidate_area_2 && evas_object_visible_get (_candidate_area_2)) {
            evas_object_hide (_candidate_area_2);
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
            ui_candidate_window_adjust ();
        }
    }

    if (bForce || ui_candidate_can_be_hide ()) {
        if (evas_object_visible_get (_candidate_window)) {
            evas_object_hide (_candidate_window);
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, (uint32)ECORE_IMF_CANDIDATE_PANEL_HIDE);

            if (!(_ise_width == -1 && _ise_height == -1)) {
                _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
                if (_ise_show)
                    set_keyboard_geometry_atom_info (KEYBOARD_STATE_ON);
                else
                    set_keyboard_geometry_atom_info (KEYBOARD_STATE_OFF);
            }
        }

        SCIM_DEBUG_MAIN (3) << "    Hide candidate window\n";
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

    _panel_agent->candidate_more_window_show ();

    evas_object_hide (_more_btn);
    evas_object_show (_candidate_area_2);
    evas_object_show (_scroller_bg);
    evas_object_show (_close_btn);
    ui_candidate_window_adjust ();

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

    if (_candidate_area_2 == NULL || !evas_object_visible_get (_candidate_area_2))
        return;

    evas_object_hide (_candidate_area_2);
    evas_object_hide (_scroller_bg);
    evas_object_hide (_close_btn);
    _panel_agent->candidate_more_window_hide ();

    evas_object_show (_candidate_area_1);
    evas_object_show (_more_btn);
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

    _click_object = GPOINTER_TO_INT (data) & 0xFF;
    _is_click     = true;

    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    _click_down_pos [0] = ev->canvas.x;
    _click_down_pos [1] = ev->canvas.y;

    if (_click_object == ISF_EFL_CANDIDATE_0 || _click_object == ISF_EFL_CANDIDATE_ITEMS) {
        int index = GPOINTER_TO_INT (data) >> 8;
        ui_candidate_delete_longpress_timer ();
        _longpress_timer = ecore_timer_add (1.0, ui_candidate_longpress_timeout, (void *)index);
    }
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

    ui_candidate_delete_longpress_timer ();

    int index = GPOINTER_TO_INT (data);
    if (_click_object == ISF_EFL_AUX && _is_click) {
/*      double ret = 0;
        const char *buf = edje_object_part_state_get (button, "aux", &ret);
        if (strcmp ("selected", buf)) {
            for (unsigned int i = 0; i < _aux_items.size (); i++) {
                buf = edje_object_part_state_get (_aux_items [i], "aux", &ret);
                if (!strcmp ("selected", buf))
                    edje_object_signal_emit (_aux_items [i], "aux,state,unselected", "aux");
            }
            edje_object_signal_emit (button, "aux,state,selected", "aux");
            _panel_agent->select_aux (index);
        }*/
        int r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3;
        edje_object_color_class_get (_aux_items [index], "text_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3);
        // Normal item is clicked
        if (!(r == 62 && g == 207 && b == 255)) {
            for (unsigned int i = 0; i < _aux_items.size (); i++) {
                edje_object_color_class_set (_aux_items [i], "text_color", 249, 249, 249, 255, r2, g2, b2, a2, r3, g3, b3, a3);
            }
            edje_object_color_class_set (_aux_items [index], "text_color", 62, 207, 255, 255, r2, g2, b2, a2, r3, g3, b3, a3);
            _panel_agent->select_aux (index);
        }
    } else if (_click_object == ISF_EFL_CANDIDATE_0 && _is_click) {
        ui_candidate_window_close_button_cb (NULL, NULL, _close_btn, NULL);
        _panel_agent->select_candidate (index);
    } else if (_click_object == ISF_EFL_CANDIDATE_ITEMS && _is_click) {
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

    if (abs (_click_up_pos [0] - _click_down_pos [0]) >= (int)(15 * _height_rate) ||
        abs (_click_up_pos [1] - _click_down_pos [1]) >= (int)(15 * _height_rate)) {
        _is_click = false;
        ui_candidate_delete_longpress_timer ();
    }
}

/**
 * @brief Create native style candidate window.
 */
static void ui_create_native_candidate_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _candidate_port_width        = _screen_width;
    _candidate_port_height_min   = 84 * _height_rate * _candidate_port_line;
    _candidate_port_height_min_2 = 84 * _height_rate + _candidate_port_height_min;
    _candidate_port_height_max   = 426 * _height_rate + _candidate_port_height_min;
    _candidate_port_height_max_2 = 84 * _height_rate + _candidate_port_height_max;
    _candidate_land_width        = _screen_height;
    _candidate_land_height_min   = 84 * _width_rate;
    _candidate_land_height_min_2 = 168 * _width_rate;
    _candidate_land_height_max   = 342 * _width_rate;
    _candidate_land_height_max_2 = 426 * _width_rate;

    _candidate_scroll_0_width_min= _screen_width;
    _candidate_scroll_0_width_max= _screen_height;
    _candidate_scroll_width_min  = _screen_width;
    _candidate_scroll_width_max  = _screen_height;
    _candidate_scroll_height_min = 252 * _width_rate;
    _candidate_scroll_height_max = 420 * _height_rate;

    _candidate_area_1_pos [0]    = 0 * _width_rate;
    _candidate_area_1_pos [1]    = 2 * _height_rate;
    _more_btn_pos [0]            = 628 * _width_rate;
    _more_btn_pos [1]            = 12 * _height_rate;
    _more_btn_pos [2]            = 1188 * _height_rate;
    _more_btn_pos [3]            = 12 * _width_rate;
    _close_btn_pos [0]           = 628 * _width_rate;
    _close_btn_pos [1]           = 12 * _height_rate;
    _close_btn_pos [2]           = 1188 * _height_rate;
    _close_btn_pos [3]           = 12 * _width_rate;

    _aux_height                  = 84 * _height_rate - 2;
    _aux_port_width              = _screen_width;
    _aux_land_width              = _screen_height;

    _item_min_height             = 84 * _height_rate - 2;

    /* Create candidate window */
    if (_candidate_window == NULL) {
        _candidate_window = efl_create_window ("candidate", "Prediction Window");
        evas_object_resize (_candidate_window, _candidate_port_width, _candidate_port_height_min);
        _candidate_width  = _candidate_port_width;
        _candidate_height = _candidate_port_height_min;

        /* Add background */
        _candidate_bg = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_candidate_bg, _candidate_edje_file.c_str (), "candidate_bg");
        evas_object_size_hint_weight_set (_candidate_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (_candidate_window, _candidate_bg);
        evas_object_show (_candidate_bg);

        /* Create _candidate_0 scroller */
        _candidate_0_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_0_scroll, EINA_TRUE, EINA_FALSE);
        elm_scroller_policy_set (_candidate_0_scroll, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_candidate_0_scroll, _candidate_scroll_0_width_min, (_item_min_height+2)*_candidate_port_line-2);
        evas_object_move (_candidate_0_scroll, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);
        _candidate_0_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_0_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_0_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_0_table, 0, 0);
        elm_object_content_set (_candidate_0_scroll, _candidate_0_table);
        evas_object_show (_candidate_0_table);
        _candidate_area_1 = _candidate_0_scroll;

        /* Create more button */
        _more_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        if (_ise_width == 0 && _ise_height == 0)
            edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "close_button");
        else
            edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "more_button");
        evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1]);
        evas_object_resize (_more_btn, 80 * _width_rate, 64 * _height_rate);
        evas_object_event_callback_add (_more_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_more_button_cb, NULL);

        /* Add scroller background */
        _candidate_scroll_width = _candidate_scroll_width_min;
        _scroller_bg = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_scroller_bg, _candidate_edje_file.c_str (), "scroller_bg");
        evas_object_size_hint_weight_set (_scroller_bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
        evas_object_move (_scroller_bg, 0, _candidate_port_height_min);

        /* Create vertical scroller */
        _candidate_scroll = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_candidate_scroll, 0, 1);
        elm_scroller_policy_set (_candidate_scroll, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
        evas_object_resize (_candidate_scroll, _candidate_scroll_width, _candidate_scroll_height_max);
        evas_object_resize (_scroller_bg, _candidate_scroll_width, _candidate_scroll_height_max + 6);
        elm_scroller_page_size_set (_candidate_scroll, 0, _item_min_height+_v_padding);
        evas_object_move (_candidate_scroll, 0, _candidate_port_height_min);
        _candidate_table = elm_table_add (_candidate_window);
        evas_object_size_hint_weight_set (_candidate_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_candidate_table, 0.0, 0.0);
        elm_table_padding_set (_candidate_table, 0, 0);
        elm_object_content_set (_candidate_scroll, _candidate_table);
        evas_object_show (_candidate_table);
        _candidate_area_2 = _candidate_scroll;

        /* Create close button */
        _close_btn = edje_object_add (evas_object_evas_get (_candidate_window));
        if (_ise_width == 0 && _ise_height == 0)
            edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "more_button");
        else
            edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "close_button");
        evas_object_move (_close_btn, _close_btn_pos[0], _close_btn_pos[1]);
        evas_object_resize (_close_btn, 80 * _width_rate, 64 * _height_rate);
        evas_object_event_callback_add (_close_btn, EVAS_CALLBACK_MOUSE_UP, ui_candidate_window_close_button_cb, NULL);

        _tmp_candidate_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_candidate_text, _candidate_font_name.c_str (), _candidate_font_size);

        /* Create aux */
        _aux_area = elm_scroller_add (_candidate_window);
        elm_scroller_bounce_set (_aux_area, 1, 0);
        elm_scroller_policy_set (_aux_area, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        evas_object_resize (_aux_area, _aux_port_width, _aux_height);
        evas_object_move (_aux_area, 0, 2);

        _aux_table = elm_table_add (_candidate_window);
        elm_object_content_set (_aux_area, _aux_table);
        elm_table_padding_set (_aux_table, 4, 0);
        evas_object_size_hint_weight_set (_aux_table, 0.0, 0.0);
        evas_object_size_hint_align_set (_aux_table, 0.0, 0.0);
        evas_object_show (_aux_table);

        _aux_line = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (_aux_line, _candidate_edje_file.c_str (), "popup_line");
        evas_object_resize (_aux_line, _candidate_port_width, 2);
        evas_object_move (_aux_line, 0, _aux_height + 2);

        _tmp_aux_text = evas_object_text_add (evas_object_evas_get (_candidate_window));
        evas_object_text_font_set (_tmp_aux_text, _candidate_font_name.c_str (), _aux_font_size);
    }

    flush_memory ();
}

/**
 * @brief Create candidate window.
 *
 * @return void
 */
static void ui_create_candidate_window (void)
{
    check_time ("\nEnter ui_create_candidate_window");
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_destroy_candidate_window ();

    _candidate_x     = 0;
    _candidate_y     = 0;
    _candidate_angle = 0;

    ui_create_native_candidate_window ();

    int angle = efl_get_angle_for_root_window (_candidate_window);
    if (_candidate_angle != angle) {
        _candidate_angle = angle;
        ui_candidate_window_rotate (angle);
    } else {
        ui_settle_candidate_window ();
    }

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

        if (_seperate_0 [i]) {
            evas_object_del (_seperate_0 [i]);
            _seperate_0 [i] = NULL;
        }
        if (_seperate_items [i]) {
            evas_object_del (_seperate_items [i]);
            _seperate_items [i] = NULL;
        }

        if (_line_0 [i]) {
            evas_object_del (_line_0 [i]);
            _line_0 [i] = NULL;
        }
        if (_line_items [i]) {
            evas_object_del (_line_items [i]);
            _line_items [i] = NULL;
        }
    }

    _aux_items.clear ();
    /* Delete candidate window */
    if (_candidate_window) {
        ui_candidate_hide (true);

        evas_object_del (_candidate_window);
        _candidate_window = NULL;
        _aux_area         = NULL;
        _candidate_area_1 = NULL;
        _candidate_area_2 = NULL;
        _scroller_bg      = NULL;
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

    if (!_candidate_window)
        return;

    int spot_x, spot_y;
    int x, y, width, height;
    bool reverse = false;

    /* Get candidate window position */
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);

    int height2 = ui_candidate_get_valid_height ();

    if (_ise_height >= 0) {
        if (_candidate_angle == 90) {
            spot_x = _screen_width - _ise_height - height2;
            spot_y = 0;
        } else if (_candidate_angle == 270) {
            spot_x = _ise_height - (_candidate_height - height2);
            spot_y = 0;
        } else if (_candidate_angle == 180) {
            spot_x = 0;
            spot_y = _ise_height - (_candidate_height - height2);
        } else {
            spot_x = 0;
            spot_y = _screen_height - _ise_height - height2;
        }
    } else {
        spot_x = _spot_location_x;
        spot_y = _spot_location_y;

        rectinfo ise_rect;
        _panel_agent->get_current_ise_geometry (ise_rect);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >= (uint32)_screen_width)
                ise_rect.height = ISE_DEFAULT_HEIGHT_LANDSCAPE * _width_rate;
        } else {
            if (ise_rect.height <= (uint32)0 || ise_rect.height >= (uint32) _screen_height)
                ise_rect.height = ISE_DEFAULT_HEIGHT_PORTRAIT * _height_rate;
        }

        int nOffset = _candidate_port_height_min / 3;
        if (_candidate_angle == 270) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_x = _screen_width - _spot_location_top_y - (_candidate_height - height2);
            } else {
                spot_x = _screen_width - _spot_location_y - _candidate_height;
            }
        } else if (_candidate_angle == 90) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_width - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_x = _spot_location_top_y - height2;
            } else {
                spot_x = spot_y;
            }
        } else if (_candidate_angle == 180) {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_y = _screen_height - _spot_location_top_y - (_candidate_height - height2);
            } else {
                spot_y = _screen_height - _spot_location_y - _candidate_height;
            }
        } else {
            if (ise_rect.height > 0 && spot_y + height2 > _screen_height - (int)ise_rect.height + nOffset) {
                reverse = true;
                spot_y = _spot_location_top_y - height2;
            } else {
                spot_y = spot_y;
            }
        }
    }

    if (_candidate_angle == 90) {
        spot_y = (_screen_height - _candidate_width) / 2;
        spot_x = spot_x < _indicator_height ? _indicator_height : spot_x;
        if (spot_x > _screen_width - _candidate_height)
            spot_x = _screen_width - _candidate_height;
    } else if (_candidate_angle == 270) {
        spot_y = (_screen_height - _candidate_width) / 2;
        spot_x = spot_x < 0 ? 0 : spot_x;
        if (spot_x > _screen_width - (_indicator_height+_candidate_height))
            spot_x = _screen_width - (_indicator_height+_candidate_height);
    } else if (_candidate_angle == 180) {
        spot_x = (_screen_width - _candidate_width) / 2;
        spot_y = spot_y < 0 ? 0 : spot_y;
        if (spot_y > _screen_height - (_indicator_height+_candidate_height))
            spot_y = _screen_height - (_indicator_height+_candidate_height);
    } else {
        spot_x = (_screen_width - _candidate_width) / 2;
        spot_y = spot_y < _indicator_height ? _indicator_height : spot_y;
        if (spot_y > _screen_height - _candidate_height)
            spot_y = _screen_height - _candidate_height;
    }

    if (spot_x != x || spot_y != y) {
        _candidate_x = spot_x;
        _candidate_y = spot_y;
        evas_object_move (_candidate_window, spot_x, spot_y);
        if (evas_object_visible_get (_candidate_window))
            _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);
    }

    if (!_candidate_window_show)
        ui_candidate_hide (false);
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
    Ecore_X_Window root_win  = ecore_x_window_root_first_get ();
    if (win_obj)
        root_win = ecore_x_window_root_get (elm_win_xwindow_get (win_obj));

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
    elm_win_title_set (win, strWinName);

    /* set window properties */
    elm_win_autodel_set (win, EINA_TRUE);
    elm_object_focus_allow_set (win, EINA_FALSE);
    elm_win_borderless_set (win, EINA_TRUE);
    elm_win_alpha_set (win, EINA_TRUE);
    efl_disable_focus_for_app_window (win);
    efl_set_showing_effect_for_app_window (win, strEffect);

    const char *szProfile[] = {"mobile", ""};
    elm_win_profiles_set (win, szProfile, 1);

    return win;
}

/**
 * @brief Get default zone geometry.
 *
 * @param x The zone x position.
 * @param y The zone y position.
 * @param w The zone width.
 * @param h The zone height.
 *
 * @return EINA_TRUE if successful, otherwise return EINA_FALSE
 */
static Eina_Bool efl_get_default_zone_geometry_info (Ecore_X_Window root, uint *x, uint *y, uint *w, uint *h)
{
    Ecore_X_Atom    zone_geometry_atom;
    Ecore_X_Window *zone_lists;
    int             num_zone_lists;
    int             num_ret;
    Eina_Bool       ret;

    zone_geometry_atom = ecore_x_atom_get ("_E_ILLUME_ZONE_GEOMETRY");
    if (!zone_geometry_atom) {
        /* Error... */
        return EINA_FALSE;
    }

    uint geom[4];
    num_zone_lists = ecore_x_window_prop_window_list_get (root, ECORE_X_ATOM_E_ILLUME_ZONE_LIST, &zone_lists);
    if (num_zone_lists > 0) {
        num_ret = ecore_x_window_prop_card32_get (zone_lists[0], zone_geometry_atom, geom, 4);
        if (num_ret == 4) {
            if (x) *x = geom[0];
            if (y) *y = geom[1];
            if (w) *w = geom[2];
            if (h) *h = geom[3];
            ret = EINA_TRUE;
        } else {
            ret = EINA_FALSE;
        }
    } else {
        /* if there is no zone available */
        ret = EINA_FALSE;
    }

    if (zone_lists) {
        /* We must free zone_lists */
        free (zone_lists);
    }

    return ret;
}

/**
 * @brief Get screen resolution.
 *
 * @param width The screen width.
 * @param height The screen height.
 */
static void efl_get_screen_resolution (int &width, int &height)
{
    static Evas_Coord scr_w = 0, scr_h = 0;

    if (scr_w == 0 || scr_h == 0) {
        uint w, h;
        if (efl_get_default_zone_geometry_info (ecore_x_window_root_first_get (), NULL, NULL, &w, &h)) {
            scr_w = w;
            scr_h = h;
        } else {
            ecore_x_window_size_get (ecore_x_window_root_first_get (), &scr_w, &scr_h);
        }
    }

    width  = scr_w;
    height = scr_h;
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
    _panel_agent->signal_connect_expand_candidate           (slot (slot_expand_candidate));
    _panel_agent->signal_connect_contract_candidate         (slot (slot_contract_candidate));
    _panel_agent->signal_connect_set_candidate_ui           (slot (slot_set_candidate_style));
    _panel_agent->signal_connect_update_factory_info        (slot (slot_update_factory_info));
    _panel_agent->signal_connect_update_spot_location       (slot (slot_update_spot_location));
    _panel_agent->signal_connect_update_input_context       (slot (slot_update_input_context));
    _panel_agent->signal_connect_update_ise_geometry        (slot (slot_update_ise_geometry));
    _panel_agent->signal_connect_show_aux_string            (slot (slot_show_aux_string));
    _panel_agent->signal_connect_show_lookup_table          (slot (slot_show_candidate_table));
    _panel_agent->signal_connect_hide_aux_string            (slot (slot_hide_aux_string));
    _panel_agent->signal_connect_hide_lookup_table          (slot (slot_hide_candidate_table));
    _panel_agent->signal_connect_update_aux_string          (slot (slot_update_aux_string));
    _panel_agent->signal_connect_update_lookup_table        (slot (slot_update_candidate_table));
    _panel_agent->signal_connect_get_candidate_geometry     (slot (slot_get_candidate_geometry));
    _panel_agent->signal_connect_get_input_panel_geometry   (slot (slot_get_input_panel_geometry));
    _panel_agent->signal_connect_set_active_ise_by_uuid     (slot (slot_set_active_ise));
    _panel_agent->signal_connect_get_ise_list               (slot (slot_get_ise_list));
    _panel_agent->signal_connect_get_keyboard_ise_list      (slot (slot_get_keyboard_ise_list));
    _panel_agent->signal_connect_get_language_list          (slot (slot_get_language_list));
    _panel_agent->signal_connect_get_all_language           (slot (slot_get_all_language));
    _panel_agent->signal_connect_get_ise_language           (slot (slot_get_ise_language));
    _panel_agent->signal_connect_get_ise_info_by_uuid       (slot (slot_get_ise_info));
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

    //if (!_config.null ())
    //    _config->reload ();
}

/**
 * @brief Focus in slot function for PanelAgent.
 */
static void slot_focus_in (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_delete_destroy_timer ();
    if (!_candidate_window) {
        ui_create_candidate_window ();
    }
}

/**
 * @brief Focus out slot function for PanelAgent.
 */
static void slot_focus_out (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    ui_candidate_delete_destroy_timer ();
    _destroy_timer = ecore_timer_add (1.0, ui_candidate_destroy_timeout, NULL);
}

/**
 * @brief Expand candidate slot function for PanelAgent.
 */
static void slot_expand_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_area_2 && !evas_object_visible_get (_candidate_area_2))
        ui_candidate_window_more_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Contract candidate slot function for PanelAgent.
 */
static void slot_contract_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Set candidate style slot function for PanelAgent.
 *
 * @param display_line The displayed line number for portrait mode.
 * @param reserved The reserved parameter.
 */
static void slot_set_candidate_style (int display_line, int reserved)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " display_line:" << display_line << "\n";
    if (display_line > 0 && display_line < 5) {
        int nOld = _candidate_port_line;
        _candidate_port_line = display_line;
        if (nOld != display_line && _candidate_window)
            ui_create_candidate_window ();
    }
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
 * @brief The input context of ISE is changed.
 *
 * @param type  The event type.
 * @param value The event value.
 */
static void slot_update_input_context (int type, int value)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (type == ECORE_IMF_INPUT_PANEL_STATE_EVENT) {
        if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
            _ise_show = false;
            ui_candidate_hide (true);
            set_keyboard_geometry_atom_info (KEYBOARD_STATE_OFF);
        } else if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
            _ise_show = true;
            if (evas_object_visible_get (_candidate_area_1))
                ui_candidate_show ();
            set_keyboard_geometry_atom_info (KEYBOARD_STATE_ON);
        }
    }
}

/**
 * @brief Update ise geometry.
 *
 * @param x      The x position in screen.
 * @param y      The y position in screen.
 * @param width  The ISE window width.
 * @param height The ISE window height.
 */
static void slot_update_ise_geometry (int x, int y, int width, int height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << x << " y:" << y << " width:" << width << " height:" << height << "...\n";

    int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (hw_kbd_detect)
        return;

    int old_height = _ise_height;

    _ise_width  = width;
    _ise_height = height;

    int angle = efl_get_angle_for_root_window (_candidate_window);
    if (_candidate_angle != angle) {
        _candidate_angle = angle;
        ui_candidate_window_rotate (angle);
    } else if (old_height != height) {
        ui_settle_candidate_window ();
    }

    if (old_height != height && _ise_show)
        set_keyboard_geometry_atom_info (KEYBOARD_STATE_ON);
}

/**
 * @brief Show aux slot function for PanelAgent.
 */
static void slot_show_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_aux_area == NULL || evas_object_visible_get (_aux_area))
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

    if (evas_object_visible_get (_candidate_area_1) || evas_object_visible_get (_candidate_area_2)) {
        if (evas_object_visible_get (_candidate_area_1)) {
            evas_object_hide (_candidate_area_1);
            evas_object_hide (_more_btn);
        }
        if (evas_object_visible_get (_candidate_area_2)) {
            evas_object_hide (_candidate_area_2);
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
        }
        ui_candidate_window_adjust ();

        ui_candidate_hide (false);
        ui_settle_candidate_window ();
    }
}

/**
 * @brief Set hightlight text color and background color for edje object.
 *
 * @param item The edje object pointer.
 * @param nForeGround The text color.
 * @param nBackGround The background color.
 * @param bSetBack The flag for background color.
 */
static void set_highlight_color (Evas_Object *item, uint32 nForeGround, uint32 nBackGround, bool bSetBack)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3;
    if (edje_object_color_class_get (item, "text_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3)) {
        r = SCIM_RGB_COLOR_RED(nForeGround);
        g = SCIM_RGB_COLOR_GREEN(nForeGround);
        b = SCIM_RGB_COLOR_BLUE(nForeGround);
        edje_object_color_class_set (item, "text_color", r, g, b, a, r2, g2, b2, a2, r3, g3, b3, a3);
    }
    if (bSetBack && edje_object_color_class_get (item, "rect_color", &r, &g, &b, &a, &r2, &g2, &b2, &a2, &r3, &g3, &b3, &a3)) {
        r = SCIM_RGB_COLOR_RED(nBackGround);
        g = SCIM_RGB_COLOR_GREEN(nBackGround);
        b = SCIM_RGB_COLOR_BLUE(nBackGround);
        edje_object_color_class_set (item, "rect_color", r, g, b, 255, r2, g2, b2, a2, r3, g3, b3, a3);
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

    if (!evas_object_visible_get (_aux_area)) {
        ui_candidate_show ();
        slot_show_aux_string ();
    }

    int x, y, width, height, item_width = 0;
    unsigned int window_width = 0, count = 0, i;

    Evas_Object *aux_edje = NULL;

    /* Get highlight item index */
    int    aux_index = -1, aux_start = 0, aux_end = 0;
    String strAux      = str;
    bool   bSetBack    = false;
    uint32 nForeGround = SCIM_RGB_COLOR(62, 207, 255);
    uint32 nBackGround = SCIM_RGB_COLOR(0, 0, 0);
    for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
        if (aux_index == -1 && ait->get_type () == SCIM_ATTR_DECORATE) {
            aux_index = ait->get_value ();
        } else if (ait->get_type () == SCIM_ATTR_FOREGROUND) {
            nForeGround = ait->get_value ();
        } else if (ait->get_type () == SCIM_ATTR_BACKGROUND) {
            nBackGround = ait->get_value ();
            bSetBack = true;
        }
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
        aux_edje = edje_object_add (evas_object_evas_get (_candidate_window));
        edje_object_file_set (aux_edje, _candidate_edje_file.c_str (), "aux");
        edje_object_text_class_set (aux_edje, "aux_text_class", _candidate_font_name.c_str (), _aux_font_size);
        edje_object_part_text_set (aux_edje, "aux", aux_list [i].c_str ());
        elm_table_pack (_aux_table, aux_edje, i, 0, 1, 1);
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_AUX));
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
        evas_object_event_callback_add (aux_edje, EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_AUX));
        evas_object_show (aux_edje);
        _aux_items.push_back (aux_edje);
/*      if (i == (unsigned int)aux_index)
            edje_object_signal_emit (aux_edje, "aux,state,selected", "aux");
        else
            edje_object_signal_emit (aux_edje, "aux,state,unselected", "aux");
*/
        evas_object_text_text_set (_tmp_aux_text, aux_list [i].c_str ());
        evas_object_geometry_get (_tmp_aux_text, &x, &y, &width, &height);
        item_width = width + 2*_blank_width;
        item_width = item_width > _item_min_width ? item_width : _item_min_width;
        evas_object_size_hint_min_set (aux_edje, item_width, _aux_height);
        if (aux_index == (int)i || (aux_index == -1 && i == 0)) {
            aux_start = window_width;
            aux_end   = window_width + item_width;
        }
        window_width = window_width + item_width + 4;
    }

    // Set highlight item
    for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
        if (ait->get_type () == SCIM_ATTR_DECORATE) {
            unsigned int index = ait->get_value ();
            if (index < _aux_items.size ())
                set_highlight_color (_aux_items [index], nForeGround, nBackGround, bSetBack);
        }
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
 */
static void update_table (int table_type, const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " (" << table.get_current_page_size () << ")\n";

    int item_num = table.get_current_page_size ();
    if (item_num < 0)
        return;

    String     mbs;
    WideString wcs;

    AttributeList attrs;
    int i, x, y, width, height, item_0_width;

    int seperate_width  = 4;
    int seperate_height = 52 * _height_rate;
    int line_width      = _candidate_scroll_width;
    int line_height     = _v_padding;
    int total_width     = 0;
    int current_width   = 0;
    int line_0          = 0;
    int line_count      = 0;
    int more_item_count = 0;
    int scroll_0_width  = _candidate_scroll_0_width_min;

    if (_candidate_angle == 90 || _candidate_angle == 270)
        scroll_0_width = 1176 * _height_rate;
    else
        scroll_0_width = 618 * _width_rate;

    Evas *evas = evas_object_evas_get (_candidate_window);
    for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_del (_candidate_0 [i]);
            _candidate_0 [i] = NULL;
        }
        if (_seperate_0 [i]) {
            evas_object_del (_seperate_0 [i]);
            _seperate_0 [i] = NULL;
        }
        if (_candidate_items [i]) {
            evas_object_del (_candidate_items [i]);
            _candidate_items [i] = NULL;
        }
        if (_seperate_items [i]) {
            evas_object_del (_seperate_items [i]);
            _seperate_items [i] = NULL;
        }
        if (_line_items [i]) {
            evas_object_del (_line_items [i]);
            _line_items [i] = NULL;
        }

        if (i < item_num) {
            bool   bHighLight  = false;
            bool   bSetBack    = false;
            uint32 nForeGround = SCIM_RGB_COLOR(249, 249, 249);
            uint32 nBackGround = SCIM_RGB_COLOR(0, 0, 0);
            attrs = table.get_attributes_in_current_page (i);
            for (AttributeList::const_iterator ait = attrs.begin (); ait != attrs.end (); ++ait) {
                if (ait->get_type () == SCIM_ATTR_DECORATE && ait->get_value () == SCIM_ATTR_DECORATE_HIGHLIGHT) {
                    bHighLight  = true;
                    nForeGround = SCIM_RGB_COLOR(62, 207, 255);
                } else if (ait->get_type () == SCIM_ATTR_FOREGROUND) {
                    bHighLight  = true;
                    nForeGround = ait->get_value ();
                } else if (ait->get_type () == SCIM_ATTR_BACKGROUND) {
                    bSetBack    = true;
                    nBackGround = ait->get_value ();
                }
            }

            wcs = table.get_candidate_in_current_page (i);
            mbs = utf8_wcstombs (wcs);

            if (!_candidate_0 [i] && total_width <= scroll_0_width) {
                _candidate_0 [i] = edje_object_add (evas);
                edje_object_file_set (_candidate_0 [i], _candidate_edje_file.c_str (), _candidate_name.c_str ());
                edje_object_text_class_set (_candidate_0 [i], "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_size);
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_CANDIDATE_0));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                evas_object_event_callback_add (_candidate_0 [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_0));
                evas_object_show (_candidate_0 [i]);

                edje_object_part_text_set (_candidate_0 [i], "candidate", mbs.c_str ());
                /* Resize _candidate_0 [i] display width */
                evas_object_text_text_set (_tmp_candidate_text, mbs.c_str ());
                evas_object_geometry_get (_tmp_candidate_text, &x, &y, &width, &height);
                item_0_width = width + 2*_blank_width;
                item_0_width = item_0_width > _item_min_width ? item_0_width : _item_min_width;
                evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                if (bHighLight || bSetBack) {
                    set_highlight_color (_candidate_0 [i], nForeGround, nBackGround, bSetBack);
                }

                // Add first item
                if (i == 0) {
                    if (item_num == 1) {
                        if (_candidate_angle == 90 || _candidate_angle == 270)
                            scroll_0_width = _candidate_land_width;
                        else
                            scroll_0_width = _candidate_port_width;
                    }
                    item_0_width = item_0_width > scroll_0_width ? scroll_0_width : item_0_width;
                    evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                    elm_table_pack (_candidate_0_table, _candidate_0 [i], 0, 0, item_0_width, _item_min_height);
                    total_width += item_0_width;
                    continue;
                } else {
                    if (i == item_num - 1) {
                        if (_candidate_angle == 90 || _candidate_angle == 270)
                            scroll_0_width = _candidate_land_width;
                        else
                            scroll_0_width = _candidate_port_width;
                    }

                    total_width += (item_0_width + seperate_width);
                    if (total_width <= scroll_0_width) {
                        _seperate_0 [i] = edje_object_add (evas);
                        edje_object_file_set (_seperate_0 [i], _candidate_edje_file.c_str (), "seperate_line");
                        evas_object_size_hint_min_set (_seperate_0 [i], seperate_width, seperate_height);
                        elm_table_pack (_candidate_0_table, _seperate_0 [i],
                                        total_width - item_0_width - seperate_width,
                                        line_0*(_item_min_height+line_height) + (_item_min_height - seperate_height)/2,
                                        seperate_width, seperate_height);
                        evas_object_show (_seperate_0 [i]);

                        elm_table_pack (_candidate_0_table, _candidate_0 [i], total_width - item_0_width, line_0*(_item_min_height+line_height), item_0_width, _item_min_height);
                        continue;
                    } else if ((_candidate_angle == 0 || _candidate_angle == 180) &&
                               (_candidate_port_line > 1 && (line_0 + 1) < _candidate_port_line)) {
                        line_0++;
                        scroll_0_width = _candidate_scroll_0_width_min;
                        item_0_width = item_0_width > scroll_0_width ? scroll_0_width : item_0_width;
                        evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                        elm_table_pack (_candidate_0_table, _candidate_0 [i], 0, line_0*(_item_min_height+line_height), item_0_width, _item_min_height);
                        total_width = item_0_width;
                        continue;
                    }
                }
            }

            if (!_candidate_items [i]) {
                _candidate_items [i] = edje_object_add (evas);
                edje_object_file_set (_candidate_items [i], _candidate_edje_file.c_str (), _candidate_name.c_str ());
                edje_object_text_class_set (_candidate_items [i], "candidate_text_class", _candidate_font_name.c_str (), _candidate_font_size);
                evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_DOWN, ui_mouse_button_pressed_cb, GINT_TO_POINTER ((i << 8) + ISF_EFL_CANDIDATE_ITEMS));
                evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_UP, ui_mouse_button_released_cb, GINT_TO_POINTER (i));
                evas_object_event_callback_add (_candidate_items [i], EVAS_CALLBACK_MOUSE_MOVE, ui_mouse_moved_cb, GINT_TO_POINTER (ISF_EFL_CANDIDATE_ITEMS));
                evas_object_show (_candidate_items [i]);
            }
            if (bHighLight || bSetBack) {
                set_highlight_color (_candidate_items [i], nForeGround, nBackGround, bSetBack);
            }
            edje_object_part_text_set (_candidate_items [i], "candidate", mbs.c_str ());
            /* Resize _candidate_items [i] display width */
            evas_object_text_text_set (_tmp_candidate_text, mbs.c_str ());
            evas_object_geometry_get (_tmp_candidate_text, &x, &y, &width, &height);
            item_0_width = width + 2*_blank_width;
            item_0_width = item_0_width > _item_min_width ? item_0_width : _item_min_width;
            evas_object_size_hint_min_set (_candidate_items [i], item_0_width, _item_min_height);
            if (current_width > 0 && current_width + item_0_width > _candidate_scroll_width) {
                current_width = 0;
                line_count++;
            }
            if (current_width == 0 && !_line_items [i]) {
                _line_items [i] = edje_object_add (evas);
                edje_object_file_set (_line_items [i], _candidate_edje_file.c_str (), "popup_line");
                evas_object_size_hint_min_set (_line_items [i], line_width, line_height);
                x = 0;
                y = line_count*(_item_min_height+line_height);
                elm_table_pack (_candidate_table, _line_items [i], x, y, line_width, line_height);
                evas_object_show (_line_items [i]);
            }
            if (current_width != 0 && !_seperate_items [i]) {
                _seperate_items [i] = edje_object_add (evas);
                edje_object_file_set (_seperate_items [i], _candidate_edje_file.c_str (), "seperate_line");
                evas_object_size_hint_min_set (_seperate_items [i], seperate_width, seperate_height);
                x = current_width;
                y = line_count*(_item_min_height+line_height) + line_height + (_item_min_height - seperate_height)/2;
                elm_table_pack (_candidate_table, _seperate_items [i], x, y, seperate_width, seperate_height);
                evas_object_show (_seperate_items [i]);
                current_width += seperate_width;
            }
            x = current_width;
            y = line_count*(_item_min_height+line_height) + line_height;
            elm_table_pack (_candidate_table, _candidate_items [i], x, y, item_0_width, _item_min_height);
            current_width += item_0_width;
            more_item_count++;
        }
    }

    for (i = 1; i < _candidate_port_line; i++) {
        if ((_candidate_angle == 0 || _candidate_angle == 180)) {
            if (_line_0 [i] == NULL) {
                _line_0 [i] = edje_object_add (evas);
                edje_object_file_set (_line_0 [i], _candidate_edje_file.c_str (), "popup_line");
                evas_object_size_hint_min_set (_line_0 [i], line_width, line_height);
                x = 0;
                y = i * (_item_min_height + line_height) - line_height;
                elm_table_pack (_candidate_0_table, _line_0 [i], x, y, line_width, line_height);
                evas_object_show (_line_0 [i]);
            }

            // Create blank line
            if (line_0 + 1 < _candidate_port_line && i > line_0) {
                int nIndex = item_num + i;
                nIndex = nIndex < SCIM_LOOKUP_TABLE_MAX_PAGESIZE ? nIndex : SCIM_LOOKUP_TABLE_MAX_PAGESIZE - 1;
                _seperate_0 [nIndex] = edje_object_add (evas);
                edje_object_file_set (_seperate_0 [nIndex], _candidate_edje_file.c_str (), "seperate_line");
                evas_object_size_hint_min_set (_seperate_0 [nIndex], seperate_width, _item_min_height);
                elm_table_pack (_candidate_0_table, _seperate_0 [nIndex],
                                0, i*(_item_min_height+line_height), seperate_width, _item_min_height);
            }
        } else if (_line_0 [i]) {
            evas_object_del (_line_0 [i]);
            _line_0 [i] = NULL;
        }
    }

    _panel_agent->update_displayed_candidate_number (item_num - more_item_count);
    if (more_item_count == 0) {
        ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
        evas_object_hide (_more_btn);
    } else if (!evas_object_visible_get (_candidate_area_2)) {
        evas_object_show (_more_btn);
        evas_object_hide (_close_btn);
    } else {
        evas_object_hide (_more_btn);
        evas_object_show (_close_btn);
    }
    elm_scroller_region_show (_candidate_area_1, 0, 0, _candidate_scroll_width, _item_min_height);
    elm_scroller_region_show (_candidate_area_2, 0, 0, _candidate_scroll_width, _item_min_height);
    flush_memory ();
}

/**
 * @brief Update candidate table slot function for PanelAgent.
 *
 * @param table The lookup table for candidate.
 */
static void slot_update_candidate_table (const LookupTable &table)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_candidate_window || table.get_current_page_size () < 0)
        return;

    update_table (ISF_CANDIDATE_TABLE, table);
}

/**
 * @brief Get candidate geometry slot function for PanelAgent.
 *
 * @param info The data is used to store candidate position and size.
 */
static void slot_get_candidate_geometry (struct rectinfo &info)
{
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
    if (_candidate_window && evas_object_visible_get (_candidate_window)) {
        /* Get candidate window position */
        /*ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);*/
        /* Get exact candidate window size */
        /*int x2, y2;
        evas_object_geometry_get (_candidate_window, &x2, &y2, &width, &height);*/

        x      = _candidate_x;
        y      = _candidate_y;
        width  = _candidate_width;
        height = _candidate_height;
    }
    info.pos_x  = x;
    info.pos_y  = y;
    info.width  = width;
    info.height = height;

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << info.pos_x << " y:" << info.pos_y
                        << " width:" << info.width << " height:" << info.height << "\n";
}

/**
 * @brief Get input panel geometry slot function for PanelAgent.
 *
 * @param info The data is used to store input panel position and size.
 */
static void slot_get_input_panel_geometry (struct rectinfo &info)
{
    VIRTUAL_KEYBOARD_STATE kbd_state = _ise_show ? KEYBOARD_STATE_ON : KEYBOARD_STATE_OFF;

    if (_ise_width == 0 && _ise_height == 0) {
        info.pos_x = 0;
        if (_candidate_window && evas_object_visible_get (_candidate_window)) {
            info.width  = _candidate_width;
            info.height = _candidate_height;
        }
        int angle = efl_get_angle_for_root_window (_candidate_window);
        if (angle == 90 || angle == 270)
            info.pos_y = _screen_width - info.height;
        else
            info.pos_y = _screen_height - info.height;
    } else {
        get_ise_geometry (info, kbd_state);
        if (_ise_width == -1 && _ise_height == -1) {
            ; // Floating style
        } else {
            if (_candidate_window && evas_object_visible_get (_candidate_window)) {
                int height = ui_candidate_get_valid_height ();
                if ((_candidate_height - height) > _ise_height) {
                    info.pos_y  = info.pos_y + info.height - _candidate_height;
                    info.height = _candidate_height;
                } else {
                    info.pos_y  -= height;
                    info.height += height;
                }
            }
        }
    }

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " x:" << info.pos_x << " y:" << info.pos_y
                        << " width:" << info.width << " height:" << info.height << "\n";
}

/**
 * @brief Set active ISE slot function for PanelAgent.
 *
 * @param uuid The active ISE's uuid.
 * @param changeDefault The flag for changeing default ISE.
 */
static void slot_set_active_ise (const String &uuid, bool changeDefault)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " (" << uuid << ")\n";

    set_active_ise (uuid, changeDefault);
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

    std::vector<String> langs;
    isf_get_all_languages (langs);
    isf_get_all_ise_names_in_languages (langs, list);

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
static bool slot_get_keyboard_ise_list (std::vector<String> &name_list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* update ise list */
    isf_load_ise_information (ALL_ISE, _config);
    bool ret = isf_update_ise_list (ALL_ISE, _config);

    std::vector<String> lang_list, uuid_list;
    isf_get_all_languages (lang_list);
    isf_get_keyboard_ises_in_languages (lang_list, uuid_list, name_list, false);

    if (ret)
        _panel_agent->update_ise_list (name_list);
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
 * @brief Get ISE information slot function for PanelAgent.
 *
 * @param uuid The ISE uuid.
 * @param info The variable is used to store ISE information.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_info (const String &uuid, ISE_INFO &info)
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
 * @param uuid The variable is ISE uuid.
 */
static void slot_set_keyboard_ise (const String &uuid)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (uuid.length () <= 0)
        return;

    uint32 ise_option = 0;
    String ise_uuid, ise_name;
    isf_get_keyboard_ise (_config, ise_uuid, ise_name, ise_option);
    if (ise_uuid == uuid)
        return;

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);
    _config->flush ();
    _config->reload ();

    _panel_agent->change_factory (uuid);
    _panel_agent->reload_config ();
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

    uint32 ise_option = 0;
    isf_get_keyboard_ise (_config, ise_uuid, ise_name, ise_option);
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

    if (!set_active_ise (default_ise.uuid, 1)) {
        if (default_ise.uuid != _initial_ise.uuid)
            set_active_ise (_initial_ise.uuid, 1);
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
 * @brief Handler function for HelperManager input.
 *
 * @param data The data to pass to this callback.
 * @param fd_handler The Ecore Fd handler.
 *
 * @return ECORE_CALLBACK_RENEW
 */
static Eina_Bool helper_manager_input_handler (void *data, Ecore_Fd_Handler *fd_handler)
{
    if (_panel_agent->has_helper_manager_pending_event ()) {
        if (!_panel_agent->filter_helper_manager_event ()) {
            std::cerr << "_panel_agent->filter_helper_manager_event () is failed!!!\n";
            elm_exit ();
        }
    } else {
        std::cerr << "_panel_agent->has_helper_manager_pending_event () is failed!!!\n";
        elm_exit ();
    }

    return ECORE_CALLBACK_RENEW;
}

/**
 * @brief Callback function for abnormal signal.
 *
 * @param sig The signal.
 */
static void signalhandler (int sig)
{
    SCIM_DEBUG_MAIN (1) << __FUNCTION__ << " Signal=" << sig << "\n";

    elm_exit ();
}

#if HAVE_VCONF
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
        free (lang_str);
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

    DEFAULT_ISE_T default_ise;
    default_ise.type = (TOOLBAR_MODE_T)scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_TYPE), (int)_initial_ise.type);
    default_ise.uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise.uuid);
    default_ise.name = get_ise_name (default_ise.uuid);

    _panel_agent->set_current_ise_name (default_ise.name);
    _panel_agent->set_default_ise (default_ise);
    _config->reload ();
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
 * @brief Check hardware keyboard.
 *
 * @return void
 */
static void check_hardware_keyboard (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    unsigned int val = 0;
    bool fixed = _config->read (String (ISF_CONFIG_PANEL_FIXED_FOR_HARDWARE_KBD), true);

    _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (ecore_x_window_prop_card32_get (ecore_x_window_root_first_get (), ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST), &val, 1)) {
        if (val != 0) {
            /* Currently active the hw ise directly */
            uint32 option = 0;
            String uuid, name;
            isf_get_keyboard_ise (_config, uuid, name, option);
            if (option & SCIM_IME_NOT_SUPPORT_HARDWARE_KEYBOARD) {
                uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
                std::cerr << __FUNCTION__ << ": Keyboard ISE (" << name << ") can not support hardware keyboard!!!\n";
            }
            set_active_ise (uuid, 1);
            if (fixed) {
                _ise_width  = 0;
                _ise_height = 0;
                if (_candidate_window && _more_btn && _close_btn) {
                    edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "close_button");
                    edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "more_button");
                }
            } else {
                _ise_width  = -1;
                _ise_height = -1;
            }
            _ise_show = false;
            _candidate_port_line = 1;

            _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 1);
        } else {
            String previous_helper = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, _initial_ise.uuid);
            set_active_ise (previous_helper, 1);
            if (_candidate_window && _more_btn && _close_btn) {
                edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "more_button");
                edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "close_button");
            }
        }
    }
    _config->write (ISF_CONFIG_PANEL_FIXED_FOR_HARDWARE_KBD, fixed);
    _config->flush ();
    _config->reload ();
    _panel_agent->reload_config ();
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
static Eina_Bool x_event_window_property_cb (void *data, int ev_type, void *ev)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    Ecore_X_Event_Window_Property *event = (Ecore_X_Event_Window_Property *)ev;

    Ecore_X_Window rootwin = ecore_x_window_root_first_get ();
    if (event->win == rootwin && event->atom == ecore_x_atom_get (PROP_X_EXT_KEYBOARD_EXIST)) {
        SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
        check_hardware_keyboard ();
    }

    return ECORE_CALLBACK_PASS_ON;
}

int main (int argc, char *argv [])
{
    struct tms    tiks_buf;
    _clock_start = times (&tiks_buf);

    int           i, fd;
#ifdef WAIT_WM
    int           try_count       = 0;
#endif
    int           ret             = 0;

    bool          daemon          = false;
    bool          should_resident = true;

    int           new_argc        = 0;
    char        **new_argv        = new char * [40];
    ConfigModule *config_module   = NULL;
    String        config_name     = String ("socket");
    String        display_name    = String ();

    Ecore_Fd_Handler *panel_agent_read_handler = NULL;
    Ecore_Fd_Handler *helper_manager_handler   = NULL;
    Ecore_Event_Handler *xclient_message_handler  = NULL;
    Ecore_Event_Handler *xwindow_property_handler = NULL;

    set_app_privilege ("isf", NULL, NULL);

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

    setenv ("ELM_FPS", "6000", 1);
    setenv ("ELM_ENGINE", "software_x11", 1); /* to avoid the inheritance of ELM_ENGINE */
    set_language_and_locale ();

#ifdef WAIT_WM
    while (1) {
        if (check_file ("/tmp/.wm_ready"))
            break;

        if (try_count == 6000) {
            std::cerr << "[ISF-PANEL-EFL] Timeout. cannot check the state of window manager....\n";
            break;
        }

        try_count++;
        usleep (50000);
    }
#endif

    elm_init (argc, argv);
    check_time ("elm_init");

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
        _seperate_0 [i]      = NULL;
        _seperate_items [i]  = NULL;
        _line_0 [i]          = NULL;
        _line_items [i]      = NULL;
    }

    efl_get_screen_resolution (_screen_width, _screen_height);

    _width_rate       = (float)(_screen_width / 720.0);
    _height_rate      = (float)(_screen_height / 1280.0);
    _blank_width      = (int)(_blank_width * _width_rate);
    _item_min_width   = (int)(_item_min_width * _width_rate);
    _item_min_height  = (int)(_item_min_height * _height_rate);
    _candidate_width  = (int)(_candidate_port_width * _width_rate);
    _candidate_height = (int)(_candidate_port_height_min * _height_rate);
    _indicator_height = (int)(_indicator_height * _height_rate);

    _aux_font_size       = (int)(_aux_font_size * _width_rate);
    _candidate_font_size = (int)(_candidate_font_size * _width_rate);

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
    /* Add callback function for input language and display language */
    vconf_notify_key_changed (VCONFKEY_LANGSET, display_language_changed_cb, NULL);

    scim_global_config_update ();
    set_language_and_locale ();

    try {
        /* update ise list */
        std::vector<String> list;
        slot_get_ise_list (list);

        /* Start default ISE */
        slot_start_default_ise ();
        check_hardware_keyboard ();
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
    }
#endif

    /* create hibernation ready file */
    FILE *rfd;
    rfd = fopen (ISF_READY_FILE, "w+");
    if (rfd)
        fclose (rfd);

    xclient_message_handler = ecore_event_handler_add (ECORE_X_EVENT_CLIENT_MESSAGE, x_event_client_message_cb, NULL);
    ecore_x_event_mask_set (ecore_x_window_root_first_get (), ECORE_X_EVENT_MASK_WINDOW_PROPERTY);
    xwindow_property_handler = ecore_event_handler_add (ECORE_X_EVENT_WINDOW_PROPERTY, x_event_window_property_cb, NULL);
    fd = _panel_agent->get_helper_manager_id ();
    if (fd >= 0)
        helper_manager_handler = ecore_main_fd_handler_add (fd, ECORE_FD_READ, helper_manager_input_handler, NULL, NULL, NULL);

    /* Set elementary scale */
    if (_screen_width)
        elm_config_scale_set (_screen_width / 720.0f);

    signal (SIGQUIT, signalhandler);
    signal (SIGTERM, signalhandler);
    signal (SIGINT,  signalhandler);
    signal (SIGHUP,  signalhandler);

    check_time ("EFL Panel launch time");

    elm_run ();

    _config->flush ();
    ret = 0;

    ecore_event_handler_del (xclient_message_handler);
    ecore_event_handler_del (xwindow_property_handler);
    if (helper_manager_handler)
        ecore_main_fd_handler_del (helper_manager_handler);
    for (unsigned int ii = 0; ii < _read_handler_list.size (); ++ii) {
        ecore_main_fd_handler_del (_read_handler_list[ii]);
    }
    _read_handler_list.clear ();

#if HAVE_VCONF
    /* Remove callback function for input language and display language */
    vconf_ignore_key_changed (VCONFKEY_LANGSET, display_language_changed_cb);
#endif

cleanup:
    ui_candidate_delete_check_size_timer ();
    ui_candidate_delete_longpress_timer ();
    ui_candidate_delete_destroy_timer ();

    if (!_config.null ())
        _config.reset ();
    if (config_module)
        delete config_module;
    if (_panel_agent) {
        _panel_agent->stop ();
        delete _panel_agent;
    }

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
