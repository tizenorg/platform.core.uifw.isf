/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more fitable.
 * Copyright (c) 2012-2014 Intel Co., Ltd.
 *
 * Contact: Yan Wang <yan.wang@intel.com>
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
#include <Ecore_Wayland.h>
#include <Ecore_File.h>
#include <Elementary.h>
#include <malloc.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#if HAVE_VCONF
#include <vconf.h>
#include <vconf-keys.h>
#endif
#include <privilege-control.h>
#include "isf_wsm_utility.h"
#include <dlog.h>
#if HAVE_TTS
#include <tts.h>
#endif
#if HAVE_FEEDBACK
#include <feedback.h>
#endif

using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define EFL_CANDIDATE_THEME1                            (SCIM_DATADIR "/isf_candidate_theme1.edj")

#define ISF_CANDIDATE_TABLE                             0

#define ISF_EFL_AUX                                     1
#define ISF_EFL_CANDIDATE_0                             2
#define ISF_EFL_CANDIDATE_ITEMS                         3

#define ISE_DEFAULT_HEIGHT_PORTRAIT                     444
#define ISE_DEFAULT_HEIGHT_LANDSCAPE                    316

#define ISF_SYSTEM_APPSERVICE_READY_VCONF               "memory/appservice/status"
#define ISF_SYSTEM_APPSERVICE_READY_STATE               1
#define ISF_SYSTEM_WM_WAIT_COUNT                        200
#define ISF_SYSTEM_WAIT_DELAY                           100 * 1000
#define ISF_CANDIDATE_DESTROY_DELAY                     3

#define ISF_PREEDIT_BORDER                              16

#define LOG_TAG                                         "ISF_WSM_EFL"

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

extern EAPI CommonLookupTable       g_isf_candidate_table;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal data types.
/////////////////////////////////////////////////////////////////////////////
typedef enum _WINDOW_STATE {
    WINDOW_STATE_HIDE = 0,
    WINDOW_STATE_WILL_HIDE,
    WINDOW_STATE_WILL_SHOW,
    WINDOW_STATE_SHOW,
    WINDOW_STATE_ON,
} WINDOW_STATE;

typedef std::map <String, Ecore_File_Monitor *>  OSPEmRepository;


/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static Evas_Object *efl_create_window                  (const char *strWinName, const char *strEffect);
static void       efl_set_transient_for_app_window     (Ecore_X_Window window);
static int        efl_get_angle_for_app_window         (void);
static int        efl_get_angle_for_ise_window         (void);

static int        ui_candidate_get_valid_height        (void);
static void       ui_candidate_hide                    (bool bForce, bool bSetVirtualKbd = true, bool will_hide = false);
static void       ui_destroy_candidate_window          (void);
static void       ui_settle_candidate_window           (void);
static void       ui_candidate_show                    (bool bSetVirtualKbd = true);
static void       ui_create_candidate_window           (void);
static void       update_table                         (int table_type, const LookupTable &table);
static void       ui_candidate_window_close_button_cb  (void *data, Evas *e, Evas_Object *button, void *event_info);

static bool       check_wm_ready                       (void);
static bool       check_system_ready                   (void);
static void       launch_default_soft_keyboard         (keynode_t *key = NULL, void* data = NULL);

/* PanelAgent related functions */
static bool       initialize_panel_agent               (const String &config, const String &display, bool resident);

static void       slot_reload_config                   (void);
static void       slot_focus_in                        (void);
static void       slot_focus_out                       (void);
static void       slot_expand_candidate                (void);
static void       slot_contract_candidate              (void);
static void       slot_set_candidate_style             (int portrait_line, int mode);
static void       slot_update_input_context            (int type, int value);
static void       slot_update_ise_geometry             (int x, int y, int width, int height);
static void       slot_update_spot_location            (int x, int y, int top_y);
static void       slot_update_factory_info             (const PanelFactoryInfo &info);
static void       slot_show_preedit_string             (void);
static void       slot_show_aux_string                 (void);
static void       slot_show_candidate_table            (void);
static void       slot_hide_preedit_string             (void);
static void       slot_hide_aux_string                 (void);
static void       slot_hide_candidate_table            (void);
static void       slot_update_preedit_string           (const String &str, const AttributeList &attrs, int caret);
static void       slot_update_preedit_caret            (int caret);
static void       slot_update_aux_string               (const String &str, const AttributeList &attrs);
static void       slot_update_candidate_table          (const LookupTable &table);
static void       slot_select_candidate                (int index);
static void       slot_set_active_ise                  (const String &uuid, bool changeDefault);
static bool       slot_get_ise_list                    (std::vector<String> &list);
static bool       slot_get_ise_information             (String uuid, String &name, String &language, int &type, int &option, String &module_name);
static bool       slot_get_keyboard_ise_list           (std::vector<String> &name_list);
static void       slot_get_language_list               (std::vector<String> &name);
static void       slot_get_all_language                (std::vector<String> &lang);
static void       slot_get_ise_language                (char *name, std::vector<String> &list);
static bool       slot_get_ise_info                    (const String &uuid, ISE_INFO &info);
static void       slot_get_candidate_geometry          (struct rectinfo &info);
static void       slot_get_input_panel_geometry        (struct rectinfo &info);
static void       slot_set_keyboard_ise                (const String &uuid);
static void       slot_get_keyboard_ise                (String &ise_name, String &ise_uuid);
static void       slot_accept_connection               (int fd);
static void       slot_close_connection                (int fd);
static void       slot_exit                            (void);

static void       slot_register_helper_properties      (int id, const PropertyList &props);
static void       slot_show_ise                        (void);
static void       slot_hide_ise                        (void);

static void       slot_will_hide_ack                   (void);
static void       slot_candidate_will_hide_ack         (void);

static void       slot_get_ise_state                   (int &state);

static Eina_Bool  panel_agent_handler                  (void *data, Ecore_Fd_Handler *fd_handler);

static Eina_Bool  efl_create_control_window            (void);
static void       _launch_default_soft_keyboard        (void);

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
static bool               _candidate_show_requested         = false;

static int                _candidate_x                      = 0;
static int                _candidate_y                      = 0;
static int                _candidate_width                  = 0;
static int                _candidate_height                 = 0;
static int                _candidate_valid_height           = 0;

static bool               _candidate_area_1_visible         = false;
static bool               _candidate_area_2_visible         = false;
static bool               _aux_area_visible                 = false;

static ISF_CANDIDATE_MODE_T          _candidate_mode        = SOFT_CANDIDATE_WINDOW;
static ISF_CANDIDATE_PORTRAIT_LINE_T _candidate_port_line   = ONE_LINE_CANDIDATE;

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

static Evas_Object       *_preedit_window                   = 0;
static Evas_Object       *_preedit_text                     = 0;
static int                _preedit_width                    = 100;
static int                _preedit_height                   = 54;

static Evas_Object       *_aux_area                         = 0;
static Evas_Object       *_aux_line                         = 0;
static Evas_Object       *_aux_table                        = 0;
static int                _aux_height                       = 0;
static int                _aux_port_width                   = 444;
static int                _aux_land_width                   = 764;
static std::vector<Evas_Object *> _aux_items;
static std::vector<Evas_Object *> _aux_seperates;

static Evas_Object       *_tmp_preedit_text                 = 0;
static Evas_Object       *_tmp_aux_text                     = 0;
static Evas_Object       *_tmp_candidate_text               = 0;

static int                _spot_location_x                  = -1;
static int                _spot_location_y                  = -1;
static int                _spot_location_top_y              = -1;
static int                _candidate_angle                  = 0;
static int                _window_angle                     = -1;

static int                _ise_width                        = 0;
static int                _ise_height                       = 0;
static WINDOW_STATE       _ise_state                        = WINDOW_STATE_HIDE;
static WINDOW_STATE       _candidate_state                  = WINDOW_STATE_HIDE;

static int                _indicator_height                 = 0;//24;
static int                _screen_width                     = 720;
static int                _screen_height                    = 1280;
static float              _width_rate                       = 1.0;
static float              _height_rate                      = 1.0;
static int                _blank_width                      = 30;

static String             _candidate_name                   = String ("candidate");
static String             _candidate_edje_file              = String (EFL_CANDIDATE_THEME1);

static String             _candidate_font_name              = String ("Tizen");
static int                _candidate_font_size              = 38;
static int                _aux_font_size                    = 38;
static int                _click_object                     = 0;
static int                _click_down_pos [2]               = {0, 0};
static int                _click_up_pos [2]                 = {0, 0};
static bool               _is_click                         = true;
static bool               _appsvc_callback_regist           = false;
static String             _initial_ise_uuid                 = String ("");
static ConfigPointer      _config;
static PanelAgent        *_panel_agent                      = 0;
static std::vector<Ecore_Fd_Handler *> _read_handler_list;

static clock_t            _clock_start;

static Ecore_Timer       *_check_size_timer                 = NULL;
static Ecore_Timer       *_longpress_timer                  = NULL;
static Ecore_Timer       *_destroy_timer                    = NULL;
static Ecore_Timer       *_system_ready_timer               = NULL;
static Ecore_Timer       *_off_prepare_done_timer           = NULL;
static Ecore_Timer       *_candidate_hide_timer             = NULL;

static Ecore_X_Window     _ise_window                       = 0;
static Ecore_X_Window     _app_window                       = 0;
static Ecore_X_Window     _control_window                   = 0;

static Ecore_File_Monitor *_inh_helper_ise_em               = NULL;
static Ecore_File_Monitor *_inh_keyboard_ise_em             = NULL;
static Ecore_File_Monitor *_osp_helper_ise_em               = NULL;
static Ecore_File_Monitor *_osp_keyboard_ise_em             = NULL;
static OSPEmRepository     _osp_bin_em;
static OSPEmRepository     _osp_info_em;

static bool               hw_kbd_mode                       = false;

#if HAVE_TTS
static tts_h              _tts                              = NULL;
#endif

#if HAVE_FEEDBACK
static bool               feedback_initialized              = false;
#endif

static Ecore_Event_Handler *_candidate_show_handler         = NULL;

/* This structure stores the geometry information reported by ISE */
struct GeometryCache
{
    bool valid;                /* Whether this information is currently valid */
    int angle;                 /* For which angle this information is useful */
    struct rectinfo geometry;  /* Geometry information */
};
static struct GeometryCache _ise_reported_geometry          = {0};

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
 *        Returns the "expected" ISE geometry when kbd_state is ON, otherwise w/h set to 0
 *
 * @param info The data is used to store ISE position and size.
 * @param kbd_state The keyboard state.
 */
static struct rectinfo get_ise_geometry ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    struct rectinfo info = {0, 0, 0, 0};

    int win_w = _screen_width, win_h = _screen_height;
    int angle = (_window_angle == -1) ? efl_get_angle_for_app_window () : _window_angle;

    if (angle == 90 || angle == 270) {
        win_w = _screen_height;
        win_h = _screen_width;
    }

    /* If we have geometry reported by ISE, use the geometry information */
    if (_ise_reported_geometry.valid && _ise_reported_geometry.angle == angle) {
        info = _ise_reported_geometry.geometry;
    } else {
        /* READ ISE's SIZE HINT HERE */
        int pos_x, pos_y, width, height;
        
        // FIXME: Get the ISE's SIZE.
        pos_x = 0;
        pos_y = 0;
        width = 0;
        height = 0;

        info.pos_x = pos_x;
        info.pos_y = pos_y;
        info.width = width;
        info.height = height;
    }
    _ise_width  = info.width;
    _ise_height = info.height;

    return info;
}

/**
 * @brief Set keyboard geometry for autoscroll.
 *        This includes the ISE geometry together with candidate window
 *
 * @param kbd_state The keyboard state.
 */
static void set_keyboard_geometry_atom_info (Ecore_X_Window window, struct rectinfo ise_rect)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (hw_kbd_mode) {
        ise_rect.pos_x = 0;
        if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
            ise_rect.width  = _candidate_width;
            ise_rect.height = _candidate_height;
        }
        int angle = efl_get_angle_for_app_window ();
        if (angle == 90 || angle == 270)
            ise_rect.pos_y = _screen_width - ise_rect.height;
        else
            ise_rect.pos_y = _screen_height - ise_rect.height;
    } else {
        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
                _candidate_valid_height = ui_candidate_get_valid_height ();
                if ((_candidate_height - _candidate_valid_height) > _ise_height) {
                    _candidate_valid_height = _candidate_height;
                    ise_rect.pos_y  = ise_rect.pos_y + ise_rect.height - _candidate_height;
                    ise_rect.height = _candidate_height;
                } else {
                    ise_rect.pos_y  -= _candidate_valid_height;
                    ise_rect.height += _candidate_valid_height;
                }
            }
        }
    }

    // FIXME: Ecore_X dependency.
}

/**
 * @brief Get ISE index according to uuid.
 *
 * @param uuid The ISE uuid.
 *
 * @return The ISE index
 */
static int get_ise_index (const String uuid)
{
    int index = 0;
    if (uuid.length () > 0) {
        for (unsigned int i = 0; i < _uuids.size (); i++) {
            if (uuid == _uuids[i]) {
                index = i;
                break;
            }
        }
    }

    return index;
}

/**
 * @brief Get ISE module file path.
 *
 * @param module_name The ISE's module name.
 * @param type The ISE's type.
 *
 * @return ISE module file path if successfully, otherwise return empty string.
 */
static String get_module_file_path (const String &module_name, const String &type)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String strFile;
    if (module_name.substr (0, 1) == String ("/")) {
        strFile = module_name + String (".so");
        if (access (strFile.c_str (), R_OK) == 0)
            return strFile;
    }

    /* Check inhouse path */
    strFile = String (SCIM_MODULE_PATH) +
              String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
              String (SCIM_PATH_DELIM_STRING) + type +
              String (SCIM_PATH_DELIM_STRING) + module_name + String (".so");
    if (access (strFile.c_str (), R_OK) == 0)
        return strFile;

    const char *module_path_env = getenv ("SCIM_MODULE_PATH");
    if (module_path_env) {
        strFile = String (module_path_env) +
                  String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
                  String (SCIM_PATH_DELIM_STRING) + type +
                  String (SCIM_PATH_DELIM_STRING) + module_name + String (".so");
        if (access (strFile.c_str (), R_OK) == 0)
            return strFile;
    }

    return String ("");
}

/**
 * @brief Get module names for all 3rd party's IMEs.
 * @param ops_module_names The list to store module names for all 3rd party's IMEs.
 * @return module name list size
 */
static int osp_module_list_get (std::vector <String> &ops_module_names)
{
    const char *module_path_env = getenv ("SCIM_MODULE_PATH");
    if (module_path_env) {
        String path = String (module_path_env) +
                      String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
                      String (SCIM_PATH_DELIM_STRING) + String ("Helper");
        ops_module_names.clear ();

        DIR *dir = opendir (path.c_str ());
        if (dir) {
            struct dirent *file = readdir (dir);
            while (file) {
                struct stat filestat;
                String absfn = path + String (SCIM_PATH_DELIM_STRING) + file->d_name;
                stat (absfn.c_str (), &filestat);
                if (S_ISREG (filestat.st_mode)) {
                    String link_name = String (file->d_name);
                    ops_module_names.push_back (link_name.substr (0, link_name.find_last_of ('.')));
                }

                file = readdir (dir);
            }
            closedir (dir);
        }
    }

    std::sort (ops_module_names.begin (), ops_module_names.end ());
    ops_module_names.erase (std::unique (ops_module_names.begin (), ops_module_names.end ()), ops_module_names.end ());

    return ops_module_names.size ();
}

static String osp_module_name_get (const String &path)
{
    String pkg_id   = path.substr (path.find_last_of (SCIM_PATH_DELIM) + 1);
    String ise_name = String ("");
    String bin_path = path + String ("/bin");
    DIR *dir = opendir (bin_path.c_str ());
    if (dir) {
        struct dirent *file = readdir (dir);
        while (file) {
            struct stat filestat;
            String absfn = bin_path + String (SCIM_PATH_DELIM_STRING) + file->d_name;
            stat (absfn.c_str (), &filestat);
            if (S_ISREG (filestat.st_mode)) {
                String ext = absfn.substr (absfn.length () - 4);
                if (ext == String (".exe")) {
                    int begin = absfn.find_last_of (SCIM_PATH_DELIM) + 1;
                    ise_name = absfn.substr (begin, absfn.find_last_of ('.') - begin);
                    break;
                }
            }

            file = readdir (dir);
        }
        closedir (dir);
    } else {
        LOGD ("open (%s) is failed!!!", bin_path.c_str ());
    }
    String module_name = String ("");
    if (ise_name.length () > 0)
        module_name = pkg_id + String (".") + ise_name;

    LOGD ("module name = %s", module_name.c_str ());
    return module_name;
}

static void osp_engine_dir_monitor_cb (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
    String em_path       = String (ecore_file_monitor_path_get (em));
    String manifest_file = em_path.substr (0, em_path.find_last_of (SCIM_PATH_DELIM)) + String ("/info/manifest.xml");
    String ext           = String (path).substr (String (path).length () - 4);

    if (event == ECORE_FILE_EVENT_CLOSED) {
        if (String (path) == manifest_file || ext == String (".exe")) {
            LOGD ("path = %s, event = %d", path, event);
            String pkg_path = em_path.substr (0, em_path.find_last_of (SCIM_PATH_DELIM));
            String module_name = osp_module_name_get (pkg_path);
            const char *module_path_env = getenv ("SCIM_MODULE_PATH");
            if (module_path_env && module_name.length () > 0) {
                String module_path = String (module_path_env) +
                      String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
                      String (SCIM_PATH_DELIM_STRING) + String ("Helper") + String (SCIM_PATH_DELIM_STRING) + module_name + String (".so");
                isf_update_ise_module (String (module_path), _config);
                _panel_agent->update_ise_list (_uuids);
                _panel_agent->reload_config ();
#if 0
                String uuid  = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
                int    index = get_ise_index (uuid);
                if (_modes[index] == TOOLBAR_HELPER_MODE) {
                    String active_module = get_module_file_path (_module_names[index], String ("Helper"));
                    if (String (module_path) == active_module) {
                        /* Restart helper ISE */
                        _panel_agent->hide_helper (uuid);
                        _panel_agent->stop_helper (uuid);
                        _panel_agent->start_helper (uuid);
                    }
                }
#endif
            }
        }
    }
}

static void add_monitor_for_osp_module (const String &module_name)
{
    String rpath         = String ("/opt/apps/") + module_name.substr (0, module_name.find_first_of ('.'));
    String exe_path      = rpath + String ("/bin");
    String manifest_path = rpath + String ("/info");

    if (_osp_bin_em.find (module_name) == _osp_bin_em.end () || _osp_bin_em[module_name] == NULL) {
        _osp_bin_em[module_name] = ecore_file_monitor_add (exe_path.c_str (), osp_engine_dir_monitor_cb, NULL);
        LOGD ("add %s", module_name.c_str ());
    }
    if (_osp_info_em.find (module_name) == _osp_info_em.end () || _osp_info_em[module_name] == NULL) {
        _osp_info_em[module_name] = ecore_file_monitor_add (manifest_path.c_str (), osp_engine_dir_monitor_cb, NULL);
    }
}

/**
 * @brief : add monitor for engine file and info file update of 3rd party's IMEs
 */
static void add_monitor_for_all_osp_modules (void)
{
    std::vector<String> osp_module_list;
    osp_module_list_get (osp_module_list);

    if (osp_module_list.size () > 0) {
        for (unsigned int i = 0; i < osp_module_list.size (); i++)
            add_monitor_for_osp_module (osp_module_list[i]);
    }
}

/**
 * @brief Callback function for ISE file monitor.
 *
 * @param data Data to pass when it is called.
 * @param em The handle of Ecore_File_Monitor.
 * @param event The event type.
 * @param path The path for current event.
 */
static void ise_file_monitor_cb (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " path=" << path << "\n";
    LOGD ("path = %s, event = %d", path, event);

    String directory = String (path);
    directory = directory.substr (0, 4);
    if (event == ECORE_FILE_EVENT_DELETED_FILE || event == ECORE_FILE_EVENT_CLOSED ||
        (event == ECORE_FILE_EVENT_CREATED_FILE && directory == String ("/opt"))) {
        String file_path = String (path);
        String file_ext  = file_path.substr (file_path.length () - 3);
        if (file_ext != String (".so")) {
            LOGD ("%s is not valid so file!!!", path);
            return;
        }

        int begin = String (path).find_last_of (SCIM_PATH_DELIM) + 1;
        String module_name = String (path).substr (begin, String (path).find_last_of ('.') - begin);

        if (event == ECORE_FILE_EVENT_DELETED_FILE) {
            /* Update ISE list */
            std::vector<String> list;
            slot_get_ise_list (list);

            /* delete osp monitor */
            OSPEmRepository::iterator iter = _osp_bin_em.find (module_name);
            if (iter != _osp_bin_em.end ()) {
                ecore_file_monitor_del (iter->second);
                _osp_bin_em.erase (iter);
                LOGD ("delete %s", module_name.c_str ());
            }
            iter = _osp_info_em.find (module_name);
            if (iter != _osp_info_em.end ()) {
                ecore_file_monitor_del (iter->second);
                _osp_info_em.erase (iter);
            }
        } else if (event == ECORE_FILE_EVENT_CLOSED) {
            isf_update_ise_module (String (path), _config);
            _panel_agent->update_ise_list (_uuids);
            _panel_agent->reload_config ();
#if 0
            String uuid    = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
            int    index   = get_ise_index (uuid);
            String strFile = String (ecore_file_monitor_path_get (em)) +
                             String (SCIM_PATH_DELIM_STRING) + _module_names[index] + String (".so");

            if (String (path) == strFile && _modes[index] == TOOLBAR_HELPER_MODE) {
                /* Restart helper ISE */
                _panel_agent->hide_helper (uuid);
                _panel_agent->stop_helper (uuid);
                _panel_agent->start_helper (uuid);
            }

            /* add osp monitor */
            if (event == ECORE_FILE_EVENT_CREATED_FILE && directory == String ("/opt")) {
                add_monitor_for_osp_module (module_name);
                LOGD ("add %s", module_name.c_str ());
            }
#endif
        }
    }
}

/**
 * @brief Delete keyboard ISE file monitor.
 */
static void delete_ise_directory_em (void) {
    if (_inh_keyboard_ise_em) {
        ecore_file_monitor_del (_inh_keyboard_ise_em);
        _inh_keyboard_ise_em = NULL;
    }
    if (_inh_helper_ise_em) {
        ecore_file_monitor_del (_inh_helper_ise_em);
        _inh_helper_ise_em = NULL;
    }

    if (_osp_keyboard_ise_em) {
        ecore_file_monitor_del (_osp_keyboard_ise_em);
        _osp_keyboard_ise_em = NULL;
    }
    if (_osp_helper_ise_em) {
        ecore_file_monitor_del (_osp_helper_ise_em);
        _osp_helper_ise_em = NULL;
    }

    OSPEmRepository::iterator iter;
    for (iter = _osp_bin_em.begin (); iter != _osp_bin_em.end (); iter++) {
        if (iter->second) {
            ecore_file_monitor_del (iter->second);
            iter->second = NULL;
        }
    }
    for (iter = _osp_info_em.begin (); iter != _osp_info_em.end (); iter++) {
        if (iter->second) {
            ecore_file_monitor_del (iter->second);
            iter->second = NULL;
        }
    }
    _osp_bin_em.clear ();
    _osp_info_em.clear ();
}

/**
 * @brief Add inhouse ISEs and OSP ISEs directory monitor.
 */
static void add_ise_directory_em (void) {
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    // inhouse IMEngine path
    String path = String (SCIM_MODULE_PATH) +
                  String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
                  String (SCIM_PATH_DELIM_STRING) + String ("IMEngine");
    if (_inh_keyboard_ise_em == NULL)
        _inh_keyboard_ise_em = ecore_file_monitor_add (path.c_str (), ise_file_monitor_cb, NULL);

    // inhouse Helper path
    path = String (SCIM_MODULE_PATH) +
           String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
           String (SCIM_PATH_DELIM_STRING) + String ("Helper");
    if (_inh_helper_ise_em == NULL)
        _inh_helper_ise_em = ecore_file_monitor_add (path.c_str (), ise_file_monitor_cb, NULL);

    const char *module_path_env = getenv ("SCIM_MODULE_PATH");
    if (module_path_env) {
        // OSP IMEngine path
        path = String (module_path_env) +
               String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
               String (SCIM_PATH_DELIM_STRING) + String ("IMEngine");
        if (access (path.c_str (), R_OK) == 0) {
            if (_osp_keyboard_ise_em == NULL) {
                _osp_keyboard_ise_em = ecore_file_monitor_add (path.c_str (), ise_file_monitor_cb, NULL);
                LOGD ("ecore_file_monitor_add path=%s", path.c_str ());
            }
        } else {
            LOGD ("access path=%s is failed!!!", path.c_str ());
        }

        // OSP Helper path
        path = String (module_path_env) +
               String (SCIM_PATH_DELIM_STRING) + String (SCIM_BINARY_VERSION) +
               String (SCIM_PATH_DELIM_STRING) + String ("Helper");
        if (access (path.c_str (), R_OK) == 0) {
            if (_osp_helper_ise_em == NULL) {
                _osp_helper_ise_em = ecore_file_monitor_add (path.c_str (), ise_file_monitor_cb, NULL);
                LOGD ("ecore_file_monitor_add path=%s", path.c_str ());
            }
        } else {
            LOGD ("access path=%s is failed!!!", path.c_str ());
        }
    } else {
        LOGD ("getenv (\"SCIM_MODULE_PATH\") is failed!!!");
    }

    add_monitor_for_all_osp_modules ();
}

/**
 * @brief Set keyboard ISE.
 *
 * @param uuid The keyboard ISE's uuid.
 * @param module_name The keyboard ISE's module name.
 *
 * @return false if keyboard ISE change is failed, otherwise return true.
 */
static bool set_keyboard_ise (const String &uuid, const String &module_name)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        uint32 kbd_option = 0;
        String kbd_uuid, kbd_name;
        isf_get_keyboard_ise (_config, kbd_uuid, kbd_name, kbd_option);
        if (kbd_uuid == uuid)
            return false;
    }

    _panel_agent->change_factory (uuid);

    String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);

    return true;
}

/**
 * @brief Set helper ISE.
 *
 * @param uuid The helper ISE's uuid.
 * @param module_name The helper ISE's module name.
 *
 * @return false if helper ISE change is failed, otherwise return true.
 */
static bool set_helper_ise (const String &uuid, const String &module_name)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();
    String pre_uuid = _panel_agent->get_current_helper_uuid ();
    if (pre_uuid == uuid)
        return false;

    if (TOOLBAR_HELPER_MODE == mode) {
        _panel_agent->hide_helper (pre_uuid);
        _panel_agent->stop_helper (pre_uuid);
        char buf[256] = {0};
        snprintf (buf, sizeof (buf), "time:%ld  pid:%d  %s  %s  stop helper(%s)\n",
            time (0), getpid (), __FILE__, __func__, pre_uuid.c_str ());
        isf_save_log (buf);
    }

    /* Set ComposeKey as keyboard ISE */
    uint32 kbd_option = 0;
    String kbd_uuid, kbd_name;
    isf_get_keyboard_ise (_config, kbd_uuid, kbd_name, kbd_option);
    if (kbd_uuid != String (SCIM_COMPOSE_KEY_FACTORY_UUID)) {
        kbd_uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
        _panel_agent->change_factory (kbd_uuid);

        String language = String ("~other");/*scim_get_locale_language (scim_get_current_locale ());*/
        _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, kbd_uuid);
    }
    char buf[256] = {0};
    snprintf (buf, sizeof (buf), "time:%ld  pid:%d  %s  %s  Start helper(%s)\n",
        time (0), getpid (), __FILE__, __func__, uuid.c_str ());
    isf_save_log (buf);

    _panel_agent->start_helper (uuid);
    _config->write (String (SCIM_CONFIG_DEFAULT_HELPER_ISE), uuid);

    return true;
}

/**
 * @brief Set active ISE.
 *
 * @param uuid The ISE's uuid.
 *
 * @return false if ISE change is failed, otherwise return true.
 */
static bool set_active_ise (const String &uuid)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    char buf[256] = {0};
    snprintf (buf, sizeof (buf), "time:%ld  pid:%d  %s  %s  set ISE(%s)\n",
        time (0), getpid (), __FILE__, __func__, uuid.c_str ());
    isf_save_log (buf);

    if (uuid.length () <= 0)
        return false;

    bool ise_changed = false;

    for (unsigned int i = 0; i < _uuids.size (); i++) {
        if (!uuid.compare (_uuids[i])) {
            if (TOOLBAR_KEYBOARD_MODE == _modes[i])
                ise_changed = set_keyboard_ise (_uuids[i], _module_names[i]);
            else if (TOOLBAR_HELPER_MODE == _modes[i])
                ise_changed = set_helper_ise (_uuids[i], _module_names[i]);

            if (ise_changed) {
                _panel_agent->set_current_toolbar_mode (_modes[i]);
                _panel_agent->set_current_ise_name (_names[i]);

                _ise_width  = 0;
                _ise_height = 0;
                _ise_state  = WINDOW_STATE_HIDE;
                _candidate_mode      = SOFT_CANDIDATE_WINDOW;
                _candidate_port_line = ONE_LINE_CANDIDATE;
                if (_candidate_window)
                    ui_create_candidate_window ();

                scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _uuids[i]);
                scim_global_config_flush ();

                _config->flush ();
                _config->reload ();
                _panel_agent->reload_config ();
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

static Eina_Bool system_ready_timeout_cb (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    LOGW ("Launching IME because of appservice ready timeout\n");

    _launch_default_soft_keyboard ();

    return ECORE_CALLBACK_CANCEL;
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
    int angle = 0;

    if (_candidate_window) {
        if (_candidate_state == WINDOW_STATE_SHOW)
            angle = _candidate_angle;
        else
            angle = efl_get_angle_for_app_window ();

        if (_aux_area_visible && _candidate_area_1_visible) {
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

    int height;

    LOGD ("%s (w: %d, h: %d)\n", __func__, new_width, new_height);
    evas_object_resize (_aux_line, new_width, 2);
    _candidate_width  = new_width;
    _candidate_height = new_height;
    if (_candidate_state == WINDOW_STATE_SHOW)
        _panel_agent->update_candidate_panel_event ((uint32)ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, 0);

    if (_candidate_state == WINDOW_STATE_SHOW && _candidate_mode == FIXED_CANDIDATE_WINDOW) {
        height = ui_candidate_get_valid_height ();
        if ((_ise_width == 0 && _ise_height == 0) ||
            (_ise_height > 0 && _candidate_valid_height != height) ||
            (_ise_height > 0 && (_candidate_height - height) > _ise_height)) {
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
        }
    }

    /* Get height for portrait and landscape */
    int port_width  = _candidate_port_width;
    int port_height = _candidate_port_height_min;
    int land_width  = _candidate_land_width;
    int land_height = _candidate_land_height_min;
    if (_candidate_angle == 90 || _candidate_angle == 270) {
        land_height = new_height;
        if (land_height == _candidate_land_height_min_2) {
            port_height = _candidate_port_height_min_2;
        } else if (land_height == _candidate_land_height_max) {
            port_height = _candidate_port_height_max;
        } else if (land_height == _candidate_land_height_max_2) {
            port_height = _candidate_port_height_max_2;
        }
    } else {
        port_height = new_height;
        if (port_height == _candidate_port_height_min_2) {
            land_height = _candidate_land_height_min_2;
        } else if (port_height == _candidate_port_height_max) {
            land_height = _candidate_land_height_max;
        } else if (port_height == _candidate_port_height_max_2) {
            land_height = _candidate_land_height_max_2;
        }
    }

    // FIXME: ECORE_X dependency.
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
    // FIXME:
    x = y = 0;
    width = _candidate_width;
    height = _candidate_height;

    if (_aux_area_visible && _candidate_area_2_visible) {
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
    } else if (_aux_area_visible && _candidate_area_1_visible) {
        evas_object_show (_aux_line);
        evas_object_move (_candidate_area_1, _candidate_area_1_pos[0], _candidate_area_1_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            ui_candidate_window_resize (width, _candidate_land_height_min_2);
            evas_object_move (_more_btn, _more_btn_pos[2], _more_btn_pos[3] + _candidate_port_height_min_2 - _candidate_port_height_min);
        } else {
            ui_candidate_window_resize (width, _candidate_port_height_min_2);
            evas_object_move (_more_btn, _more_btn_pos[0], _more_btn_pos[1] + _candidate_port_height_min_2 - _candidate_port_height_min);
        }
    } else if (_aux_area_visible) {
        evas_object_hide (_aux_line);
        ui_candidate_window_resize (width, _aux_height + 2);
    } else if (_candidate_area_2_visible) {
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

    ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);

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

    evas_object_hide (_candidate_area_2);
    _candidate_area_2_visible = false;
    ui_candidate_window_adjust ();
    if (_candidate_area_1_visible) {
        update_table (ISF_CANDIDATE_TABLE, g_isf_candidate_table);
    }
    flush_memory ();

    LOGD ("elm_win_rotation_with_resize_set (window : %p, angle : %d)\n", _candidate_window, angle);
    elm_win_rotation_with_resize_set (_candidate_window, angle);
    if (_preedit_window)
        elm_win_rotation_with_resize_set (_preedit_window, angle);
}

/**
 * @brief This function is used to judge whether candidate window should be hidden.
 *
 * @return true if candidate window should be hidden, otherwise return false.
 */
static bool ui_candidate_can_be_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_aux_area_visible || _candidate_area_1_visible || _candidate_area_2_visible)
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
 * @brief Callback function for off_prepare_done.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool off_prepare_done_timeout (void *data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #8 Let the Window Manager to actually hide keyboard window */
    // WILL_HIDE_REQUEST_DONE Ack to WM
    // FIXME:

    _off_prepare_done_timer = NULL;

    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete candidate hide timer.
 *
 * @return void
 */
static void delete_candidate_hide_timer (void)
{
    LOGD ("deleting candidate_hide_timer");
    if (_candidate_hide_timer) {
        ecore_timer_del (_candidate_hide_timer);
        _candidate_hide_timer = NULL;
    }
}

static void candidate_window_hide (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "\n";

    delete_candidate_hide_timer ();
    _candidate_state = WINDOW_STATE_HIDE;

    LOGD ("evas_object_hide (_candidate_window, %p)\n", elm_win_xwindow_get (_candidate_window));

    if (_candidate_window) {
        /* There are cases that when there are rapid ISE_HIDE and ISE_SHOW requests,
           candidate window should be displayed but STATE_OFF for the first ISE_HIDE
           calls this function, so when the candidate window is shown by the following
           STATE_ON message, a blank area is displayed in candidate window -
           so we let the _cnadidate_area_1 as the default area that would be displayed */
        //evas_object_hide (_candidate_area_1);
        //evas_object_hide (_more_btn);
        _candidate_area_1_visible = false;

        evas_object_hide (_candidate_window);
        SCIM_DEBUG_MAIN (3) << "    Hide candidate window\n";
    }
}

/**
 * @brief Callback function for candidate hide timer
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool candidate_hide_timer (void *data)
{
    LOGD ("calling candidate_window_hide()");
    candidate_window_hide ();

    return ECORE_CALLBACK_CANCEL;
}

/**
 * @brief Delete candidate show handler.
 *
 * @return void
 */
static void delete_candidate_show_handler (void)
{
    if (_candidate_show_handler) {
        ecore_event_handler_del (_candidate_show_handler);
        _candidate_show_handler = NULL;
    }
}


/**
 * @brief Show candidate window.
 *
 * @param bSetVirtualKbd The flag for set_keyboard_geometry_atom_info () calling.
 */
static void ui_candidate_show (bool bSetVirtualKbd)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);

    delete_candidate_hide_timer ();

    if (!_candidate_window) return;

   /* FIXME : SHOULD UNIFY THE METHOD FOR CHECKING THE HW KEYBOARD EXISTENCE */
   /* If the ISE is not visible currently, wait for the ISE to be opened and then show our candidate window */
   _candidate_show_requested = true;
   if( (hw_kbd_detect == 0 && _ise_state != WINDOW_STATE_SHOW)) {
        LOGD ("setting _show_can didate_requested to TRUE");
        return;
    }

    ui_candidate_window_rotate (_candidate_angle);

    /* If the candidate window was about to hide, turn it back to SHOW state now */
    if (_candidate_state == WINDOW_STATE_WILL_HIDE) {
        _candidate_state = WINDOW_STATE_SHOW;
    }

    /* Change to WILL_SHOW state only when we are not currently in SHOW state */
    if (_candidate_state != WINDOW_STATE_SHOW) {
        _candidate_state = WINDOW_STATE_WILL_SHOW;
    }

    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (bSetVirtualKbd) {
            set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
        }
    }

    ui_candidate_delete_check_size_timer ();
    _check_size_timer = ecore_timer_add (0.02, ui_candidate_check_size_timeout, NULL);

    SCIM_DEBUG_MAIN (3) << "    Show candidate window\n";
    if (_ise_state == WINDOW_STATE_SHOW) {
        edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "more_button");
        edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "close_button");
    } else {
        edje_object_file_set (_more_btn, _candidate_edje_file.c_str (), "close_button");
        edje_object_file_set (_close_btn, _candidate_edje_file.c_str (), "more_button");
    }

    /* If we are in hardward keyboard mode, this candidate window is now considered to be a input panel */
    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (hw_kbd_mode) {
            LOGD ("sending ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW");
            _panel_agent->update_input_panel_event ((uint32)ECORE_IMF_INPUT_PANEL_STATE_EVENT, (uint32)ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW);
        }
    }

    evas_object_show (_candidate_window);
}

/**
 * @brief Hide candidate window.
 *
 * @param bForce The flag to hide candidate window by force.
 * @param bSetVirtualKbd The flag for set_keyboard_geometry_atom_info () calling.
 */
static void ui_candidate_hide (bool bForce, bool bSetVirtualKbd, bool will_hide)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " bForce:" << bForce << " bSetVirtualKbd:" << bSetVirtualKbd << " will_hide:" << will_hide << "...\n";
    
    if (!_candidate_window)
        return;

    if (bForce) {
        if (_candidate_area_2 && _candidate_area_2_visible) {
            evas_object_hide (_candidate_area_2);
            _candidate_area_2_visible = false;
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
            ui_candidate_window_adjust ();
        }
    }

    if (bForce || ui_candidate_can_be_hide ()) {
        if (will_hide) {
            LOGD ("candidate_state = WILL_HIDE");
            _candidate_state = WINDOW_STATE_WILL_HIDE;

            delete_candidate_hide_timer ();
            _candidate_hide_timer = ecore_timer_add (2.0, candidate_hide_timer, NULL);
        }

        if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
            /* FIXME : should check if bSetVirtualKbd flag is really needed in this case */
            if (_ise_state == WINDOW_STATE_SHOW) {
                set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
            } else {
                if (bSetVirtualKbd) {
                    set_keyboard_geometry_atom_info (_app_window, get_ise_geometry ());
                }
            }
        }

        if (!will_hide) {
            /* If we are not in will_hide state, hide the candidate window immediately */
            candidate_window_hide ();
            evas_object_hide (_preedit_window);
        }
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

    if (_candidate_angle == 180 || _candidate_angle == 270) {
        Ecore_Evas *ee = ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, 0, 0, 0, 0);
    }

    evas_object_show (_candidate_area_2);
    _candidate_area_2_visible = true;
    evas_object_show (_scroller_bg);
    evas_object_hide (_more_btn);
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

    if (_candidate_area_2 == NULL || !_candidate_area_2_visible)
        return;

    evas_object_hide (_candidate_area_2);
    _candidate_area_2_visible = false;
    evas_object_hide (_scroller_bg);
    evas_object_hide (_close_btn);
    _panel_agent->candidate_more_window_hide ();

    evas_object_show (_candidate_area_1);
    _candidate_area_1_visible = true;
    evas_object_show (_more_btn);

    elm_scroller_region_show (_candidate_area_2, 0, 0, _candidate_scroll_width, 100);
    if (_candidate_angle == 180 || _candidate_angle == 270) {
        Ecore_Evas *ee= ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window));
        ecore_evas_move_resize (ee, 0, 0, 0, 0);
    }

    ui_candidate_window_adjust ();
    ui_settle_candidate_window ();
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

#if HAVE_FEEDBACK
        if (feedback_initialized) {
            int feedback_result = 0;

            feedback_result = feedback_play_type (FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_SIP);

            if (FEEDBACK_ERROR_NONE == feedback_result)
                LOGD ("Sound play successful");
            else
                LOGW ("Cannot play feedback sound : %d", feedback_result);

            feedback_result = feedback_play_type (FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_SIP);

            if (FEEDBACK_ERROR_NONE == feedback_result)
                LOGD ("Vibration play successful");
            else
                LOGW ("Cannot play feedback vibration : %d", feedback_result);
        }
#endif

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
 * @brief Open TTS device.
 *
 * @return false if open is failed, otherwise return true.
 */
static bool ui_open_tts (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

#if HAVE_TTS
    int r = tts_create (&_tts);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_create FAILED : result(%d)\n", r);
        _tts = NULL;
        return false;
    }

    r = tts_set_mode (_tts, TTS_MODE_SCREEN_READER);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_set_mode FAILED : result(%d)\n", r);
    }

    tts_state_e current_state;
    r = tts_get_state (_tts, &current_state);
    if (TTS_ERROR_NONE != r) {
        LOGW ("tts_get_state FAILED : result(%d)\n", r);
    }

    if (TTS_STATE_CREATED == current_state)  {
        r = tts_prepare (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_prepare FAILED : ret(%d)\n", r);
        }
    }
    return true;
#else
    return false;
#endif
}

/**
 * @brief Close TTS device.
 */
static void ui_close_tts (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

#if HAVE_TTS
    if (_tts) {
        int r = tts_unprepare (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_unprepare FAILED : result(%d)\n", r);
        }

        r = tts_destroy (_tts);
        if (TTS_ERROR_NONE != r) {
            LOGW ("tts_destroy FAILED : result(%d)\n", r);
        }
    }
#endif
}

/**
 * @brief Play string by TTS.
 *
 * @param str The string for playing.
 */
static void ui_play_tts (const char* str)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " str=" << str << "\n";

#if HAVE_TTS
    if (_tts == NULL) {
        if (!ui_open_tts ())
            return;
    }

    if (str) {
        int r;
        int utt_id = 0;
        tts_state_e current_state;

        r = tts_get_state (_tts, &current_state);
        if (TTS_ERROR_NONE != r) {
            LOGW ("Fail to get state from TTS : ret(%d)\n", r);
        }

        if (TTS_STATE_PLAYING == current_state)  {
            r = tts_stop (_tts);
            if (TTS_ERROR_NONE != r) {
                LOGW ("Fail to stop TTS : ret(%d)\n", r);
            }
        }
        /* FIXME: Should support for all languages */
        r = tts_add_text (_tts, str, "en_US", TTS_VOICE_TYPE_FEMALE, TTS_SPEED_NORMAL, &utt_id);
        if (TTS_ERROR_NONE == r) {
            r = tts_play (_tts);
            if (TTS_ERROR_NONE != r) {
                LOGW ("Fail to play TTS : ret(%d)\n", r);
            }
        }
    }
#endif
}

/**
 * @brief Mouse over (find focus object and play text by TTS) when screen reader is enabled.
 *
 * @param mouse_x Mouse X position.
 * @param mouse_y Mouse Y position.
 */
static void ui_mouse_over (int mouse_x, int mouse_y)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL || _candidate_state != WINDOW_STATE_SHOW)
        return;

    int x, y, width, height;
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_geometry_get (_candidate_0 [i], &x, &y, &width, &height);
            if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
                /* FIXME: Should consider emoji case */
                String mbs = utf8_wcstombs (g_isf_candidate_table.get_candidate_in_current_page (i));
                SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " play candidate string: " << mbs << "\n";
                ui_play_tts (mbs.c_str ());
                return;
            }
        }
    }

    String strTts = String ("");
    if (_candidate_area_2_visible) {
        evas_object_geometry_get (_close_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height)
            strTts = String ("close button");
    } else {
        evas_object_geometry_get (_more_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height)
            strTts = String ("more button");
    }
    if (strTts.length () > 0)
        ui_play_tts (strTts.c_str ());
}

/**
 * @brief Mouse click (find focus object and do click event) when screen reader is enabled.
 *
 * @param mouse_x Mouse X position.
 * @param mouse_y Mouse Y position.
 */
static void ui_mouse_click (int mouse_x, int mouse_y)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL || _candidate_state != WINDOW_STATE_SHOW)
        return;

    int x, y, width, height;
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
        if (_candidate_0 [i]) {
            evas_object_geometry_get (_candidate_0 [i], &x, &y, &width, &height);
            if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
                Evas_Event_Mouse_Down event_info;
                event_info.canvas.x = mouse_x;
                event_info.canvas.y = mouse_y;
                ui_mouse_button_pressed_cb (GINT_TO_POINTER ((i << 8) + ISF_EFL_CANDIDATE_0), NULL, NULL, &event_info);
                ui_mouse_button_released_cb (GINT_TO_POINTER (i), NULL, NULL, &event_info);
                return;
            }
        }
    }

    if (_candidate_area_2_visible) {
        evas_object_geometry_get (_close_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
            ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
        }
    } else {
        evas_object_geometry_get (_more_btn, &x, &y, &width, &height);
        if (mouse_x >= x && mouse_x <= x + width && mouse_y >= y && mouse_y <= y + height) {
            ui_candidate_window_more_button_cb (NULL, NULL, NULL, NULL);
        }
    }
}

/**
 * @brief Create preedit window.
 */
static void ui_create_preedit_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    _preedit_width  = 100;
    _preedit_height = _preedit_height * _height_rate;
    if (_preedit_window == NULL) {
        _preedit_window = efl_create_window ("preedit", "Preedit Window");
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);

        int preedit_font_size = (int)(32 * _width_rate);

        _preedit_text = edje_object_add (evas_object_evas_get (_preedit_window));
        edje_object_file_set (_preedit_text, _candidate_edje_file.c_str (), "preedit_text");
        edje_object_text_class_set (_preedit_text, "preedit_text_class", _candidate_font_name.c_str (), preedit_font_size);
        evas_object_size_hint_fill_set (_preedit_text, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_size_hint_weight_set (_preedit_text, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        elm_win_resize_object_add (_preedit_window, _preedit_text);
        evas_object_show (_preedit_text);

        _tmp_preedit_text = evas_object_text_add (evas_object_evas_get (_preedit_window));
        evas_object_text_font_set (_tmp_preedit_text, _candidate_font_name.c_str (), preedit_font_size);
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
    _candidate_port_height_max   = 444 * _height_rate + _candidate_port_height_min;
    _candidate_port_height_max_2 = 84 * _height_rate + _candidate_port_height_max;
    _candidate_land_width        = _screen_height;
    _candidate_land_height_min   = 84 * _width_rate;
    _candidate_land_height_min_2 = 168 * _width_rate;
    _candidate_land_height_max   = 316 * _width_rate + _candidate_land_height_min;
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
        if (_candidate_angle == 90 || _candidate_angle == 270) {
            _candidate_width  = _candidate_land_width;
            _candidate_height = _candidate_land_height_min;
        } else {
            _candidate_width  = _candidate_port_width;
            _candidate_height = _candidate_port_height_min;
        }

        // FIXME: Ecore_X dependency.

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
        evas_object_move (_aux_area, _candidate_area_1_pos[0], _candidate_area_1_pos[1]);

        _aux_table = elm_table_add (_candidate_window);
        elm_object_content_set (_aux_area, _aux_table);
        elm_table_padding_set (_aux_table, 0, 0);
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

    unsigned int set = 1;
    // FIXME:

    int angle = efl_get_angle_for_app_window ();
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
    _aux_seperates.clear ();
    /* Delete candidate window */
    if (_candidate_window) {
        LOGD ("calling ui_candidate_hide (true)");
        ui_candidate_hide (true);

        evas_object_del (_candidate_window);
        _candidate_window = NULL;
        _aux_area         = NULL;
        _candidate_area_1 = NULL;
        _candidate_area_2 = NULL;
        _scroller_bg      = NULL;
    }

    if (_preedit_window) {
        evas_object_del (_preedit_text);
        _preedit_text = NULL;

        evas_object_hide (_preedit_window);
        evas_object_del (_preedit_window);
        _preedit_window = NULL;
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

    /* If both ISE and candidate window are going to be hidden,
       let's just not move our candidate window */
    if (_ise_state == WINDOW_STATE_WILL_HIDE && _candidate_state == WINDOW_STATE_WILL_HIDE)
        return;

    int spot_x, spot_y;
    int x, y, width, height;
    int pos_x = 0, pos_y = 0, ise_width = 0, ise_height = 0;
    bool get_geometry_result = false;
    bool reverse = false;

    /* Get candidate window position */
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);

    //FIXME: Ecore_X dependency.

    if ((_ise_state != WINDOW_STATE_SHOW && _ise_state != WINDOW_STATE_WILL_HIDE) ||
            get_geometry_result == false) {
        ise_height = 0;
        ise_width = 0;
    }

    int height2 = ui_candidate_get_valid_height ();

    if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
        if (_candidate_angle == 90) {
            spot_x = _screen_width - ise_height - height2;
            spot_y = 0;
        } else if (_candidate_angle == 270) {
            spot_x = ise_height - (_candidate_height - height2);
            spot_y = 0;
        } else if (_candidate_angle == 180) {
            spot_x = 0;
            spot_y = ise_height - (_candidate_height - height2);
        } else {
            spot_x = 0;
            spot_y = _screen_height - ise_height - height2;
        }
    } else {
        spot_x = _spot_location_x;
        spot_y = _spot_location_y;

        rectinfo ise_rect = {0, 0, ise_width, ise_height};
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
        LOGD ("Moving candidate window to : %d %d", spot_x, spot_y);
        if (_preedit_window) {
            if (_candidate_angle == 90) {
                spot_x -= _preedit_height;
                spot_y = _screen_height - _preedit_width;
            } else if (_candidate_angle == 270) {
                spot_x += height2;
            } else if (_candidate_angle == 180) {
                spot_x = _screen_width - _preedit_width;
                spot_y += height2;
            } else {
                spot_y -= _preedit_height;
            }
            evas_object_move (_preedit_window, spot_x, spot_y);
        }
    }
}

//////////////////////////////////////////////////////////////////////
// End of Candidate Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Set transient for app window.
 *
 * @param window The Ecore_X_Window handler of app window.
 */
static void efl_set_transient_for_app_window (Ecore_X_Window window)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Set a transient window for window stack */
    //FIXME:
}

/**
 * @brief Get angle for app window.
 *
 * @param win_obj The Evas_Object handler of application window.
 *
 * @return The angle of app window.
 */
static int efl_get_angle_for_app_window ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret;
    int count;
    int angle = 0;
    unsigned char *prop_data = NULL;

    //FIXME:

    return angle;
}

/**
 * @brief Get angle for ise window.
 *
 * @param win_obj The Evas_Object handler of ise window.
 *
 * @return The angle of ise window.
 */
static int efl_get_angle_for_ise_window ()
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret;
    int count;
    int angle = 0;
    unsigned char *prop_data = NULL;

    //FIXME:

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
    //FIXME:
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
    elm_win_prop_focus_skip_set (win, EINA_TRUE);
    efl_set_showing_effect_for_app_window (win, strEffect);

    //const char *szProfile[] = {"mobile", ""};
    //elm_win_profiles_set (win, szProfile, 1);

    return win;
}

/**
 * @brief Create elementary control window.
 *
 * @return EINA_TRUE if successful, otherwise return EINA_FALSE
 */
static Eina_Bool efl_create_control_window (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #1 Creating and registering control window */
    // FIXME:
    return EINA_TRUE;
}

/**
 * @brief Get screen resolution.
 *
 * @param width The screen width.
 * @param height The screen height.
 */
static void efl_get_screen_resolution (int &width, int &height)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    width = 720;
    height = 1280;
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
    _panel_agent->signal_connect_show_preedit_string        (slot (slot_show_preedit_string));
    _panel_agent->signal_connect_show_aux_string            (slot (slot_show_aux_string));
    _panel_agent->signal_connect_show_lookup_table          (slot (slot_show_candidate_table));
    _panel_agent->signal_connect_hide_preedit_string        (slot (slot_hide_preedit_string));
    _panel_agent->signal_connect_hide_aux_string            (slot (slot_hide_aux_string));
    _panel_agent->signal_connect_hide_lookup_table          (slot (slot_hide_candidate_table));
    _panel_agent->signal_connect_update_preedit_string      (slot (slot_update_preedit_string));
    _panel_agent->signal_connect_update_preedit_caret       (slot (slot_update_preedit_caret));
    _panel_agent->signal_connect_update_aux_string          (slot (slot_update_aux_string));
    _panel_agent->signal_connect_update_lookup_table        (slot (slot_update_candidate_table));
    _panel_agent->signal_connect_select_candidate           (slot (slot_select_candidate));
    _panel_agent->signal_connect_get_candidate_geometry     (slot (slot_get_candidate_geometry));
    _panel_agent->signal_connect_get_input_panel_geometry   (slot (slot_get_input_panel_geometry));
    _panel_agent->signal_connect_set_active_ise_by_uuid     (slot (slot_set_active_ise));
    _panel_agent->signal_connect_get_ise_list               (slot (slot_get_ise_list));
    _panel_agent->signal_connect_get_ise_information        (slot (slot_get_ise_information));
    _panel_agent->signal_connect_get_keyboard_ise_list      (slot (slot_get_keyboard_ise_list));
    _panel_agent->signal_connect_get_language_list          (slot (slot_get_language_list));
    _panel_agent->signal_connect_get_all_language           (slot (slot_get_all_language));
    _panel_agent->signal_connect_get_ise_language           (slot (slot_get_ise_language));
    _panel_agent->signal_connect_get_ise_info_by_uuid       (slot (slot_get_ise_info));
    _panel_agent->signal_connect_set_keyboard_ise           (slot (slot_set_keyboard_ise));
    _panel_agent->signal_connect_get_keyboard_ise           (slot (slot_get_keyboard_ise));
    _panel_agent->signal_connect_accept_connection          (slot (slot_accept_connection));
    _panel_agent->signal_connect_close_connection           (slot (slot_close_connection));
    _panel_agent->signal_connect_exit                       (slot (slot_exit));

    _panel_agent->signal_connect_register_helper_properties (slot (slot_register_helper_properties));
    _panel_agent->signal_connect_show_ise                   (slot (slot_show_ise));
    _panel_agent->signal_connect_hide_ise                   (slot (slot_hide_ise));

    _panel_agent->signal_connect_will_hide_ack              (slot (slot_will_hide_ack));

    _panel_agent->signal_connect_candidate_will_hide_ack    (slot (slot_candidate_will_hide_ack));
    _panel_agent->signal_connect_get_ise_state              (slot (slot_get_ise_state));

    std::vector<String> load_ise_list;
    _panel_agent->get_active_ise_list (load_ise_list);

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

    ui_candidate_delete_destroy_timer ();
}

/**
 * @brief Focus out slot function for PanelAgent.
 */
static void slot_focus_out (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    ui_candidate_delete_destroy_timer ();
    _destroy_timer = ecore_timer_add (ISF_CANDIDATE_DESTROY_DELAY, ui_candidate_destroy_timeout, NULL);
}

/**
 * @brief Expand candidate slot function for PanelAgent.
 */
static void slot_expand_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    if (_candidate_area_2 && !_candidate_area_2_visible)
        ui_candidate_window_more_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Contract candidate slot function for PanelAgent.
 */
static void slot_contract_candidate (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW)
        return;

    ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
}

/**
 * @brief Set candidate style slot function for PanelAgent.
 *
 * @param portrait_line The displayed line number for portrait.
 * @param mode The candidate mode.
 */
static void slot_set_candidate_style (int portrait_line, int mode)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " display_line:" << portrait_line << " mode:" << mode << "\n";
    if ((portrait_line != _candidate_port_line) || (mode != _candidate_mode)) {
        _candidate_mode      = (ISF_CANDIDATE_MODE_T)mode;
        _candidate_port_line = (ISF_CANDIDATE_PORTRAIT_LINE_T)portrait_line;

        if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
            if (_candidate_window)
                ui_destroy_candidate_window ();

            return;
        }

        if (_candidate_window)
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

    String old_ise = _panel_agent->get_current_ise_name ();
    if (old_ise != ise_name) {
        int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
        if (hw_kbd_detect && _candidate_window) {
            ui_destroy_candidate_window ();
        }
    }

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode)
        ise_name = _names[get_ise_index (_panel_agent->get_current_helper_uuid ())];

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

    LOGD ("x : %d , y : %d , width : %d , height : %d", x, y, width, height);

    int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (hw_kbd_detect)
        return;

    int old_height = _ise_height;

    _ise_width  = width;
    _ise_height = height;

    if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
        if (old_height != height) {
            ui_settle_candidate_window ();
        }
    }

    if (old_height != height && _ise_state == WINDOW_STATE_SHOW) {
        _ise_reported_geometry.valid = true;
        _ise_reported_geometry.angle = efl_get_angle_for_ise_window ();
        _ise_reported_geometry.geometry.pos_x = x;
        _ise_reported_geometry.geometry.pos_y = y;
        _ise_reported_geometry.geometry.width = width;
        _ise_reported_geometry.geometry.height = height;
        set_keyboard_geometry_atom_info (_app_window, _ise_reported_geometry.geometry);
        _panel_agent->update_input_panel_event (ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, 0);
    }
}

/**
 * @brief Show preedit slot function for PanelAgent.
 */
static void slot_show_preedit_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_preedit_window == NULL) {
        ui_create_preedit_window ();
        int angle = efl_get_angle_for_app_window ();
        elm_win_rotation_with_resize_set (_preedit_window, angle);

        /* Move preedit window according to candidate window position */
        if (_candidate_window) {
            /* Get candidate window position */
            int x, y, width, height;
            ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &width, &height);

            int height2 = ui_candidate_get_valid_height ();
            if (angle == 90) {
                x -= _preedit_height;
                y = _screen_height - _preedit_width;
            } else if (_candidate_angle == 270) {
                x += height2;
            } else if (_candidate_angle == 180) {
                x = _screen_width - _preedit_width;
                y += height2;
            } else {
                y -= _preedit_height;
            }
            evas_object_move (_preedit_window, x, y);
        }
    }

    if (evas_object_visible_get (_preedit_window))
        return;

    slot_show_candidate_table ();
    evas_object_show (_preedit_window);
}

/**
 * @brief Show aux slot function for PanelAgent.
 */
static void slot_show_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (_aux_area == NULL || _aux_area_visible)
        return;

    evas_object_show (_aux_area);
    _aux_area_visible = true;
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_show ()");
    ui_candidate_show ();
    ui_settle_candidate_window ();
}

/**
 * @brief Show candidate table slot function for PanelAgent.
 */
static void slot_show_candidate_table (void)
{
    int feedback_result = 0;

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        _panel_agent->helper_candidate_show ();
        return;
    }

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (_candidate_state == WINDOW_STATE_SHOW &&
        (_candidate_area_1_visible || _candidate_area_2_visible)) {
        efl_set_transient_for_app_window (elm_win_xwindow_get (_candidate_window));
        return;
    }

    evas_object_show (_candidate_area_1);
    _candidate_area_1_visible = true;
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_show ()");
    ui_candidate_show ();
    ui_settle_candidate_window ();

#if HAVE_FEEDBACK
    feedback_result = feedback_initialize ();

    if (FEEDBACK_ERROR_NONE == feedback_result) {
        LOGD ("Feedback initialize successful");
        feedback_initialized = true;
    } else {
        LOGW ("Feedback initialize fail : %d",feedback_result);
        feedback_initialized = false;
    }
#endif
}

/**
 * @brief Hide preedit slot function for PanelAgent.
 */
static void slot_hide_preedit_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_preedit_window || !evas_object_visible_get (_preedit_window))
        return;

    evas_object_hide (_preedit_window);
}

/**
 * @brief Hide aux slot function for PanelAgent.
 */
static void slot_hide_aux_string (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (!_aux_area || !_aux_area_visible)
        return;

    evas_object_hide (_aux_area);
    _aux_area_visible = false;
    elm_scroller_region_show (_aux_area, 0, 0, 10, 10);
    ui_candidate_window_adjust ();

    LOGD ("calling ui_candidate_hide (false)");
    ui_candidate_hide (false);
    ui_settle_candidate_window ();

    if (ui_candidate_can_be_hide ()) {
        _candidate_show_requested = false;
        LOGD ("setting _show_can didate_requested to FALSE");
    }
}

/**
 * @brief Hide candidate table slot function for PanelAgent.
 */
static void slot_hide_candidate_table (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int feedback_result = 0;

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        _panel_agent->helper_candidate_hide ();
        return;
    }

    if (!_candidate_area_1 || _candidate_state == WINDOW_STATE_WILL_HIDE)
        return;

    if (_candidate_area_1_visible || _candidate_area_2_visible) {
        bool bForce = false;
        if (_candidate_area_1_visible) {
            if (_aux_area_visible) {
                evas_object_hide (_candidate_area_1);
                _candidate_area_1_visible = false;
                evas_object_hide (_more_btn);
            } else {
                /* Let's not actually hide the _candidate_area_1 object, for the case that
                   even if the application replies CANDIDATE_WILL_HIDE_ACK a little late,
                   it is better to display the previous candidates instead of blank screen */
                _candidate_area_1_visible = false;
                bForce = true;
            }
        }
        if (_candidate_area_2_visible) {
            evas_object_hide (_candidate_area_2);
            _candidate_area_2_visible = false;
            evas_object_hide (_scroller_bg);
            evas_object_hide (_close_btn);
            _panel_agent->candidate_more_window_hide ();
        }
        ui_candidate_window_adjust ();

        LOGD ("calling ui_candidate_hide (%d, true, true)", bForce);
        ui_candidate_hide (bForce, true, true);
        ui_settle_candidate_window ();
    }

#if HAVE_FEEDBACK
    feedback_result = feedback_deinitialize ();

    if (FEEDBACK_ERROR_NONE == feedback_result)
        LOGD ("Feedback deinitialize successful");
    else
        LOGW ("Feedback deinitialize fail : %d", feedback_result);

    feedback_initialized = false;
#endif

    if (ui_candidate_can_be_hide ()) {
        _candidate_show_requested = false;
        LOGD ("setting _show_can didate_requested to FALSE");
    }
}

/**
 * @brief Update preedit slot function for PanelAgent.
 *
 * @param str The new preedit string.
 * @param attrs The attribute list of new preedit string.
 */
static void slot_update_preedit_string (const String &str, const AttributeList &attrs, int caret)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " string=" << str << "\n";

    if (str.length () <= 0)
        return;

    if (_preedit_window == NULL || !evas_object_visible_get (_preedit_window)) {
        slot_show_preedit_string ();
    }

    int x, y, width, height, candidate_width;
    evas_object_text_text_set (_tmp_preedit_text, str.c_str ());
    evas_object_geometry_get (_tmp_preedit_text, &x, &y, &width, &height);
    ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_candidate_window)), &x, &y, &candidate_width, &height);
    _preedit_width = (width + ISF_PREEDIT_BORDER * 2) < candidate_width ? (width + ISF_PREEDIT_BORDER * 2) : candidate_width;

    /* Resize preedit window and avoid text blink */
    int old_width, old_height;
    evas_object_geometry_get (_preedit_window, &x, &y, &old_width, &old_height);
    if (old_width < _preedit_width) {
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);
        edje_object_part_text_set (_preedit_text, "preedit", str.c_str ());
    } else {
        edje_object_part_text_set (_preedit_text, "preedit", str.c_str ());
        evas_object_resize (_preedit_window, _preedit_width, _preedit_height);
    }

    /* Move preedit window */
    if (_candidate_angle == 90 || _candidate_angle == 180) {
        ecore_evas_geometry_get (ecore_evas_ecore_evas_get (evas_object_evas_get (_preedit_window)), &x, &y, &width, &height);
        if (_candidate_angle == 90) {
            y = _screen_height - _preedit_width;
        } else if (_candidate_angle == 180) {
            x = _screen_width - _preedit_width;
        }
        evas_object_move (_preedit_window, x, y);
    }
}

/**
 * @brief Update caret slot function for PanelAgent.
 *
 * @param caret The caret position.
 */
static void slot_update_preedit_caret (int caret)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " caret=" << caret << "\n";
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
    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (!_aux_area || (str.length () <= 0))
        return;

    if (!_aux_area_visible) {
        LOGD ("calling ui_candidate_show ()");
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
    if (_aux_seperates.size () > 0) {
        for (i = 0; i < _aux_seperates.size (); i++)
            evas_object_del (_aux_seperates [i]);
        _aux_seperates.clear ();
    }

    int   seperate_width  = 4;
    int   seperate_height = 52 * _height_rate;
    Evas *evas = evas_object_evas_get (_candidate_window);
    for (i = 0; i < aux_list.size (); i++) {
        if (i > 0) {
            Evas_Object *seperate_item = edje_object_add (evas);
            edje_object_file_set (seperate_item, _candidate_edje_file.c_str (), "seperate_line");
            evas_object_size_hint_min_set (seperate_item, seperate_width, seperate_height);
            elm_table_pack (_aux_table, seperate_item, 2 * i - 1, 0, 1, 1);
            evas_object_show (seperate_item);
            _aux_seperates.push_back (seperate_item);
        }

        count++;
        aux_edje = edje_object_add (evas);
        edje_object_file_set (aux_edje, _candidate_edje_file.c_str (), "aux");
        edje_object_text_class_set (aux_edje, "aux_text_class", _candidate_font_name.c_str (), _aux_font_size);
        edje_object_part_text_set (aux_edje, "aux", aux_list [i].c_str ());
        elm_table_pack (_aux_table, aux_edje, 2 * i, 0, 1, 1);
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
    int i, x, y, width, height, item_0_width = 0;

    int nLast      = 0;
    std::vector<uint32> row_items;

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
    int cursor_pos      = table.get_cursor_pos ();
    int cursor_line     = 0;

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

                /* Check whether this item is the last one */
                if (i == item_num - 1) {
                    if (_candidate_angle == 90 || _candidate_angle == 270)
                        scroll_0_width = _candidate_land_width;
                    else
                        scroll_0_width = _candidate_port_width;
                }

                /* Add first item */
                if (i == 0) {
                    item_0_width = item_0_width > scroll_0_width ? scroll_0_width : item_0_width;
                    evas_object_size_hint_min_set (_candidate_0 [i], item_0_width, _item_min_height);
                    elm_table_pack (_candidate_0_table, _candidate_0 [i], 0, 0, item_0_width, _item_min_height);
                    total_width += item_0_width;
                    continue;
                } else {
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

                        row_items.push_back (i - nLast);
                        nLast = i;
                        continue;
                    } else {
                        row_items.push_back (i - nLast);
                        nLast = i;
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

                row_items.push_back (i - nLast);
                nLast = i;
                if (cursor_pos >= i)
                    cursor_line++;
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

    row_items.push_back (item_num - nLast);     /* Add the number of last row */
    _panel_agent->update_candidate_item_layout (row_items);
    _panel_agent->update_displayed_candidate_number (item_num - more_item_count);
    if (more_item_count == 0) {
        ui_candidate_window_close_button_cb (NULL, NULL, NULL, NULL);
        evas_object_hide (_more_btn);
        evas_object_hide (_close_btn);
    } else if (!_candidate_area_2_visible) {
        evas_object_show (_more_btn);
        evas_object_hide (_close_btn);
    } else {
        evas_object_hide (_more_btn);
        evas_object_show (_close_btn);
    }

    int w, h;
    elm_scroller_region_get (_candidate_area_2, &x, &y, &w, &h);

    int line_h   = _item_min_height + _v_padding;
    int cursor_y = cursor_line * line_h;
    if (cursor_y < y) {
        elm_scroller_region_show (_candidate_area_2, 0, cursor_y, w, h);
    } else if (cursor_y >= y + h) {
        elm_scroller_region_show (_candidate_area_2, 0, cursor_y + line_h - h, w, h);
    }

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

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW){
        _panel_agent->update_helper_lookup_table (table);
        return ;
    }

    if (_candidate_window == NULL)
        ui_create_candidate_window ();

    if (!_candidate_window || table.get_current_page_size () < 0)
        return;

    update_table (ISF_CANDIDATE_TABLE, table);
}

/**
 * @brief Send selected candidate index.
 *
 * @param selected candidate string index number.
 */
static void slot_select_candidate (int index)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";
    _panel_agent->select_candidate (index);
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

    if (_candidate_mode == SOFT_CANDIDATE_WINDOW) {
        info.pos_x  = x;
        info.pos_y  = y;
        info.width  = width;
        info.height = height;
        return;
    }

    if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
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

    LOGD ("%d %d %d %d", info.pos_x, info.pos_y, info.width, info.height);
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
    int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    if (hw_kbd_detect) {
        info.pos_x = 0;
        info.width = 0;
        info.height = 0;
        if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
            info.width  = _candidate_width;
            info.height = _candidate_height;
        }
        int angle = efl_get_angle_for_app_window ();
        if (angle == 90 || angle == 270)
            info.pos_y = _screen_width - info.height;
        else
            info.pos_y = _screen_height - info.height;
    } else {
        info = get_ise_geometry ();
        if (_ise_state != WINDOW_STATE_SHOW) {
            info.width = 0;
            info.height = 0;
        } else {
            if (_candidate_mode == FIXED_CANDIDATE_WINDOW) {
                if (_candidate_window && _candidate_state == WINDOW_STATE_SHOW) {
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
    }

    LOGD ("%d %d %d %d", info.pos_x, info.pos_y, info.width, info.height);
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

    set_active_ise (uuid);
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
    bool ret = isf_update_ise_list (ALL_ISE, _config);

    list.clear ();
    list = _uuids;

    _panel_agent->update_ise_list (list);

    if (ret && _initial_ise_uuid.length () > 0) {
        String active_uuid   = _initial_ise_uuid;
        String default_uuid  = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
        int    hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
        if (std::find (_uuids.begin (), _uuids.end (), default_uuid) == _uuids.end ()) {
            if (hw_kbd_detect && _modes[get_ise_index (_initial_ise_uuid)] != TOOLBAR_KEYBOARD_MODE) {
                active_uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
            }
            set_active_ise (active_uuid);
        } else if (!hw_kbd_detect) {    // Check whether keyboard engine is installed
            String IMENGINE_KEY  = String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + String ("~other");
            String keyboard_uuid = _config->read (IMENGINE_KEY, String (""));
            if (std::find (_uuids.begin (), _uuids.end (), keyboard_uuid) == _uuids.end ()) {
                active_uuid = String (SCIM_COMPOSE_KEY_FACTORY_UUID);
                _panel_agent->change_factory (active_uuid);
                _config->write (IMENGINE_KEY, active_uuid);
                _config->flush ();
            }
        }
    }

    add_ise_directory_em ();
    return ret;
}

/**
 * @brief Get the ISE's information.
 *
 * @param uuid The ISE's uuid.
 * @param name The ISE's name.
 * @param language The ISE's language.
 * @param type The ISE's type.
 * @param option The ISE's option.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_ise_information (String uuid, String &name, String &language, int &type, int &option, String &module_name)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (uuid.length () > 0) {
        for (unsigned int i = 0; i < _uuids.size (); i++) {
            if (uuid == _uuids[i]) {
                name     = _names[i];
                language = _langs[i];
                type     = _modes[i];
                option   = _options[i];
                module_name = "";
                return true;
            }
        }
    }

    std::cerr << __func__ << " is failed!!!\n";
    return false;
}

/**
 * @brief Get keyboard ISEs list slot function for PanelAgent.
 *
 * @param name_list The list is used to store keyboard ISEs.
 *
 * @return true if this operation is successful, otherwise return false.
 */
static bool slot_get_keyboard_ise_list (std::vector<String> &name_list)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* update ise list */
    bool ret = isf_update_ise_list (ALL_ISE, _config);

    std::vector<String> lang_list, uuid_list;
    isf_get_all_languages (lang_list);
    isf_get_keyboard_ises_in_languages (lang_list, uuid_list, name_list, false);

    if (ret)
        _panel_agent->update_ise_list (uuid_list);
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
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " uuid = " << uuid << "\n";

    if (uuid.length () <= 0 || std::find (_uuids.begin (), _uuids.end (), uuid) == _uuids.end ())
        return;

    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    if (_modes[get_ise_index (default_uuid)] == TOOLBAR_KEYBOARD_MODE)
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
    uint32 ise_option = 0;
    isf_get_keyboard_ise (_config, ise_uuid, ise_name, ise_option);

    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " uuid = " << ise_uuid << "\n";
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

static void slot_register_helper_properties (int id, const PropertyList &props)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #2 Receiving X window ID from ISE */
    /* FIXME : We should add an API to set window id of ISE */
}

static void slot_show_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* If the candidate was already in SHOW state, respect the current angle */
    if (_candidate_state != WINDOW_STATE_SHOW) {
        /* FIXME : Need to check if candidate_angle and window_angle should be left as separated */
        _candidate_angle = efl_get_angle_for_app_window ();
        _window_angle = efl_get_angle_for_app_window ();
    }

    efl_set_transient_for_app_window (_ise_window);

    /* If our ISE was already in SHOW state, skip state transition to WILL_SHOW */
    if (_ise_state != WINDOW_STATE_SHOW) {
        _ise_state = WINDOW_STATE_WILL_SHOW;
    }
}

static void slot_hide_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Only if we are not already in HIDE state */
    if (_ise_state != WINDOW_STATE_HIDE) {
        _ise_state = WINDOW_STATE_WILL_HIDE;
    }
    _window_angle = -1;

    if (_candidate_window) {
        int hw_kbd_detect = _config->read (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
        if (hw_kbd_detect)
            ui_candidate_hide (true, true, true);
    }
}

static void slot_will_hide_ack (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* WMSYNC, #8 Let the Window Manager to actually hide keyboard window */
    // WILL_HIDE_REQUEST_DONE Ack to WM
    // FIXME:
}

static void slot_candidate_will_hide_ack (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    // FIXME:
}

static void slot_get_ise_state (int &state)
{
    if (_ise_state == WINDOW_STATE_SHOW ||
        (hw_kbd_mode && _candidate_state == WINDOW_STATE_SHOW)) {
        state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;
    } else {
        /* Currently we don't have WILL_HIDE / HIDE state distinction in Ecore_IMF */
        switch (_ise_state) {
            case WINDOW_STATE_SHOW :
                state = ECORE_IMF_INPUT_PANEL_STATE_SHOW;
                break;
            case WINDOW_STATE_WILL_SHOW :
                state = ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;
                break;
            case WINDOW_STATE_WILL_HIDE :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
                break;
            case WINDOW_STATE_HIDE :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
                break;
            default :
                state = ECORE_IMF_INPUT_PANEL_STATE_HIDE;
        };
    }
    LOGD ("state = %d", state);
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << " state = " << state << "\n";
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
 * @return true if successful, otherwise return false.
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
 * @return true if successful, otherwise return false.
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

    /* Update all ISE names according to display language */
    std::vector<String> module_list;
    for (unsigned int i = 0; i < _module_names.size (); i++) {
        if (std::find (module_list.begin (), module_list.end (), _module_names[i]) != module_list.end ())
            continue;
        module_list.push_back (_module_names[i]);
        if (_module_names[i] == String (COMPOSE_KEY_MODULE)) {
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

    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
    String default_name = _names[get_ise_index (default_uuid)];
    _panel_agent->set_current_ise_name (default_name);
    _config->reload ();
}
#endif

/**
 * @brief Start default ISE.
 *
 * @return void
 */
static void start_default_ise (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise_uuid);
    if (!set_active_ise (default_uuid)) {
        std::cerr << __FUNCTION__ << " Failed to launch default ISE(" << default_uuid << ")\n";
        LOGW ("Failed to launch default ISE (%s)\n", default_uuid.c_str ());

        if (default_uuid != _initial_ise_uuid) {
            std::cerr << __FUNCTION__ << " Launch initial ISE(" << _initial_ise_uuid << ")\n";
            if (!set_active_ise (_initial_ise_uuid)) {
                LOGW ("Failed to launch initial ISE (%s)\n", _initial_ise_uuid.c_str ());
            } else {
                LOGD ("Succeed to launch initial ISE (%s)\n", _initial_ise_uuid.c_str ());
            }
        }
    } else {
        LOGD ("Succeed to launch default ISE (%s)\n", default_uuid.c_str ());
    }
}

/**
 * @brief Check hardware keyboard.
 *
 * @return void
 */
static void check_hardware_keyboard (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_off_prepare_done_timer) {
        ecore_timer_del (_off_prepare_done_timer);
        _off_prepare_done_timer = NULL;
    }

    unsigned int val = 0;

    _config->write (ISF_CONFIG_HARDWARE_KEYBOARD_DETECT, 0);
    
    //FIXME:
    uint32 option = 0;
    String uuid, name;
    String helper_uuid  = _config->read (SCIM_CONFIG_DEFAULT_HELPER_ISE, String (""));
    String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));

    /* When switching back to S/W keyboard mode, let's hide candidate window first */
    LOGD ("calling ui_candidate_hide (true, true, true)");
    ui_candidate_hide (true, true, true);
    uuid = helper_uuid.length () > 0 ? helper_uuid : _initial_ise_uuid;
    hw_kbd_mode = false;
    _panel_agent->reset_keyboard_ise ();

    set_active_ise (uuid);
}

/**
 * @brief : Checks whether the window manager is launched or not
 * @return true if window manager launched, else false
 */
static bool check_wm_ready (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    return true;
}

/**
 * @brief : Checks whether the system service is ready or not
 * @return true if all system service are ready, else false
 */
static bool check_system_ready (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    int ret = 0;
    int val = 0;
    ret = vconf_get_int (ISF_SYSTEM_APPSERVICE_READY_VCONF, &val);

    LOGD ("ret : %d, val : %d\n", ret, val);

    if (ret == 0 && val >= ISF_SYSTEM_APPSERVICE_READY_STATE) {
        LOGD ("Appservice was ready\n");
        return true;
    } else {
        /* Register a call back function for checking system ready */
        if (!_appsvc_callback_regist) {
            if (vconf_notify_key_changed (ISF_SYSTEM_APPSERVICE_READY_VCONF, launch_default_soft_keyboard, NULL)) {
                _appsvc_callback_regist = true;
            }

            if (!_system_ready_timer) {
                _system_ready_timer = ecore_timer_add (5.0, system_ready_timeout_cb, NULL);
            }
        }

        return false;
    }
}

static void _launch_default_soft_keyboard (void)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    if (_appsvc_callback_regist)
        vconf_ignore_key_changed (ISF_SYSTEM_APPSERVICE_READY_VCONF, launch_default_soft_keyboard);

    if (_system_ready_timer) {
        ecore_timer_del (_system_ready_timer);
        _system_ready_timer = NULL;
    }

    /* Start default ISE */
    start_default_ise ();
    check_hardware_keyboard ();
}

/**
 * @brief : Launches default soft keyboard for performance enhancement (It's not mandatory)
 */
static void launch_default_soft_keyboard (keynode_t *key, void* data)
{
    SCIM_DEBUG_MAIN (3) << __FUNCTION__ << "...\n";

    /* Soft keyboard will be started when all system service are ready */
    if (check_system_ready ()) {
        _launch_default_soft_keyboard ();
    }
}

int main (int argc, char *argv [])
{
    struct tms    tiks_buf;
    _clock_start = times (&tiks_buf);

    int           i;
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

    check_time ("\nStarting ISF WSM EFL...... ");
    LOGD ("Starting ISF WSM EFL...... \n");

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

    try {
        _panel_agent->send_display_name (display_name);
    } catch (scim::Exception & e) {
        std::cerr << e.what() << "\n";
        ret = -1;
        goto cleanup;
    }

    if (daemon) {
        check_time ("ISF WSM EFL run as daemon");
        scim_daemon ();
    }

    /* Connect the configuration reload signal. */
    _config->signal_connect_reload (slot (config_reload_cb));

    if (!check_wm_ready ()) {
        std::cerr << "[ISF-PANEL-EFL] WM ready timeout\n";
        LOGW ("Window Manager ready timeout\n");
    } else {
        LOGD ("Window Manager is in ready state\n");
    }

    elm_init (argc, argv);
    check_time ("elm_init");

    //elm_policy_set (ELM_POLICY_THROTTLE, ELM_POLICY_THROTTLE_NEVER);

    if (!efl_create_control_window ()) {
        LOGW ("Failed to create control window\n");
        goto cleanup;
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

    helper_manager_handler   = ecore_main_fd_handler_add (_panel_agent->get_helper_manager_id (), ECORE_FD_READ, helper_manager_input_handler, NULL, NULL, NULL);
    panel_agent_read_handler = ecore_main_fd_handler_add (_panel_agent->get_server_id (), ECORE_FD_READ, panel_agent_handler, NULL, NULL, NULL);
    _read_handler_list.push_back (panel_agent_read_handler);
    check_time ("run_panel_agent");

    set_language_and_locale ();

#if HAVE_VCONF
    /* Add callback function for input language and display language */
    vconf_notify_key_changed (VCONFKEY_LANGSET, display_language_changed_cb, NULL);
#endif

    try {
        /* Update ISE list */
        std::vector<String> list;
        slot_get_ise_list (list);

        /* Load initial ISE information */
        _initial_ise_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (SCIM_COMPOSE_KEY_FACTORY_UUID));

        /* Launches default soft keyboard when all conditions are satisfied */
        launch_default_soft_keyboard ();
    } catch (scim::Exception & e) {
        std::cerr << e.what () << "\n";
    }

    // FIXME:

    /* Set elementary scale */
    if (_screen_width)
        elm_config_scale_set (_screen_width / 720.0f);

    signal (SIGQUIT, signalhandler);
    signal (SIGTERM, signalhandler);
    signal (SIGINT,  signalhandler);
    signal (SIGHUP,  signalhandler);
    signal (SIGABRT, signalhandler);

    check_time ("EFL WSM launch time");

    elm_run ();

    _config->flush ();
    ret = 0;

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
    delete_ise_directory_em ();
    ui_close_tts ();

    if (!_config.null ())
        _config.reset ();
    if (config_module)
        delete config_module;
    if (_panel_agent) {
        try {
            _panel_agent->stop ();
        } catch (scim::Exception & e) {
            std::cerr << "Exception is thrown from _panel_agent->stop (), error is " << e.what () << "\n";
        }
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
