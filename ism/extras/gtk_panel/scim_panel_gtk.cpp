/** @file scim_panel_gtk.cpp
 */
/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim_panel_gtk.cpp,v 1.118.2.15 2007/04/11 11:30:31 suzhe Exp $
 */

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/times.h>
#include <signal.h>
#include <glib.h>
#include <string.h>
#include <gdk/gdk.h>
#undef GDK_WINDOWING_X11
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <list>

#define Uses_C_STDIO
#define Uses_C_STDLIB
#define Uses_SCIM_LOOKUP_TABLE
#define Uses_SCIM_SOCKET
#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_DEBUG
#define Uses_SCIM_HELPER
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_PANEL_AGENT

#define Uses_SCIM_MODULE
#define Uses_SCIM_COMPOSE_KEY
#define Uses_SCIM_CONFIG_BASE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HOTKEY
#define Uses_SCIM_FILTER_MANAGER
#define Uses_SCIM_UTILITY

#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#include "scimstringview.h"

#if HAVE_GCONF
#include <gtk/gtkimscreenresolution.h>
#include <gconf/gconf-client.h>
#include <global-gconf.h>
#include <phonestatus.h>
#include "limo-event-delivery.h"
#endif

#include "scim_setup_module.h"
#include "isf_setup_utility.h"
#include "isf_help_win.h"
#include "isf_setup_win.h"

using namespace scim;

#include "icons/up.xpm"
#include "icons/down.xpm"
#include "icons/left.xpm"
#include "icons/right.xpm"


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#define MAX_FONT_STR_SIZE                               200
#define FONT_SIZE_OF_CANDIDATE_WIDGET                   change_screen_resolution_P_H_RATE(11)

#define SCIM_CONFIG_PANEL_GTK_FONT                      "/Panel/Gtk/Font"
#define SCIM_CONFIG_PANEL_GTK_COLOR_NORMAL_BG           "/Panel/Gtk/Color/NormalBackground"
#define SCIM_CONFIG_PANEL_GTK_COLOR_ACTIVE_BG           "/Panel/Gtk/Color/ActiveBackground"
#define SCIM_CONFIG_PANEL_GTK_COLOR_NORMAL_TEXT         "/Panel/Gtk/Color/NormalText"
#define SCIM_CONFIG_PANEL_GTK_COLOR_ACTIVE_TEXT         "/Panel/Gtk/Color/ActiveText"
#define SCIM_CONFIG_PANEL_GTK_COLOR_HIGHLIGHT_TEXT      "/Panel/Gtk/Color/HighlightText"
#define SCIM_CONFIG_PANEL_GTK_COLOR_SEPARATOR           "/Panel/Gtk/Color/Separator"
#define SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_VERTICAL     "/Panel/Gtk/LookupTableVertical"
#define SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_STYLE        "/Panel/Gtk/LookupTableStyle"
#define SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_MODE         "/Panel/Gtk/LookupTableMode"
#define SCIM_CONFIG_PANEL_GTK_DEFAULT_STICKED           "/Panel/Gtk/DefaultSticked"

#define LOOKUP_ICON_SIZE                                12

#define BLANK_SIZE                                      0
#define VERTICAL_SHOW_LINE                              7

#define CANDIDATE_TABLE                                 0
#define ASSOCIATE_TABLE                                 1

#define PORTRAIT_DEGREE                                 0
#define LANDSCAPE_DEGREE                                90

/////////////////////////////////////////////////////////////////////////////
// Declaration of external variables.
/////////////////////////////////////////////////////////////////////////////
extern MapStringVectorSizeT         _groups;
extern std::vector < String >       _uuids;
extern std::vector < String >       _names;
extern std::vector < String >       _module_names;
extern std::vector < String >       _langs;
extern std::vector < String >       _icons;
extern std::vector < uint32 >       _options;
extern std::vector <TOOLBAR_MODE_T> _modes;
extern MapStringString              _keyboard_ise_help_map;
extern MapStringVectorString        _disabled_ise_map;
extern std::vector < String >       _disabled_langs;
extern MapStringVectorString        _disabled_ise_map_bak;
extern std::vector < String >       _disabled_langs_bak;
extern std::vector < String >       _isf_app_list;
extern gboolean                     _ise_list_changed;
extern gboolean                     _setup_enable_changed;

extern CommonLookupTable            g_isf_candidate_table;

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal data types.
/////////////////////////////////////////////////////////////////////////////
enum {
    IME_SETTING_KEYPAD_TYPE_3X4 = 0,
    IME_SETTING_KEYPAD_TYPE_QTY,
    IME_SETTING_KEYPAD_TYPE_NUM
};

typedef enum {
    IME_BIT_0 = 0,
    IME_BIT_1,
    IME_BIT_2,
    IME_BIT_3,
    IME_BIT_4,
    IME_BIT_5,
    IME_BIT_6,
    IME_BIT_7,
    IME_BITL_MAX
} ImeBitPosition_E;

typedef struct _ImeSettingPredictionEngineOptionInfo_S {
    gint keypad_type;                       /* IME_SETTING_KEYPAD_TYPE_3X4, IME_SETTING_KEYPAD_TYPE_QTY */

    gint word_cmpletion_point;              /* 0(Off) - 6 */
    gboolean spell_correction;              /* On - Off */
    gboolean next_word_prediction;          /* On - Off */
    gboolean auto_substitution;             /* On - Off */
    gboolean multitap_word_completion;      /* On - Off */
    gboolean regional_input;                /* On - Off */

    gint word_cmpletion_point_prev;         /* 0(Off) - 6 */
    gboolean spell_correction_prev;         /* On - Off */
    gboolean next_word_prediction_prev;     /* On - Off */
    gboolean auto_substitution_prev;        /* On - Off */
    gboolean multitap_word_completion_prev; /* On - Off */
    gboolean regional_input_prev;           /* On - Off */
} ImeSettingPredictionEngineOptionInfo_S;

enum {
    ACTIVE_ISE_LIST_ENABLE = 0,
    ACTIVE_ISE_NAME,
    ACTIVE_ISE_UUID,
    ACTIVE_ISE_TYPE,
    ACTIVE_ISE_ICON,
    ACTIVE_ISE_OPTION,
    ACTIVE_ISE_NUM_COLUMS
};

typedef enum
{
    SCIM_CANDIDATE_STYLE = 0,
    PREDICTION_ENGINE_CANDIDATE_STYLE,
} ISF_CANDIDATE_STYLE_T;

typedef enum
{
    PORTRAIT_HORIZONTAL_CANDIDATE_MODE = 0,
    PORTRAIT_VERTICAL_CANDIDATE_MODE,
    LANDSCAPE_HORIZONTAL_CANDIDATE_MODE,
    LANDSCAPE_VERTICAL_CANDIDATE_MODE,
    PORTRAIT_MORE_CANDIDATE_MODE,
    LANDSCAPE_MORE_CANDIDATE_MODE,
} ISF_CANDIDATE_MODE_T;

#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <String, std::vector <size_t>, scim_hash_string>    MapStringVectorSizeT;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <String, std::vector <size_t>, scim_hash_string>          MapStringVectorSizeT;
#else
typedef std::map <String, std::vector <size_t> >                                MapStringVectorSizeT;
#endif

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal functions.
/////////////////////////////////////////////////////////////////////////////
static void       update_active_ise_list_store         (bool set_default_ise);
static void       ui_config_reload_callback            (const ConfigPointer &config);
static void       ui_load_config                       (void);
static void       ui_initialize                        (void);
static void       ui_settle_input_window               (bool            relative = false,
                                                        bool            force    = false);
static int        ui_screen_width                      (void);
static int        ui_screen_height                     (void);

#if GDK_MULTIHEAD_SAFE
static void       ui_switch_screen                     (GdkScreen      *screen);
#endif

static GdkPixbuf* ui_scale_pixbuf                      (GdkPixbuf      *pixbuf,
                                                        int             width,
                                                        int             height);
static GtkWidget* ui_create_icon                       (const String   &iconfile,
                                                        const char    **xpm = NULL,
                                                        int             width = -1,
                                                        int             height = -1,
                                                        bool            force_create = false);
static GtkWidget* ui_create_up_icon                    (void);
static GtkWidget* ui_create_down_icon                  (void);
static GtkWidget* ui_create_left_icon                  (void);
static GtkWidget* ui_create_right_icon                 (void);

// callback functions
static void       ui_preedit_area_move_cursor_cb       (ScimStringView *view,
                                                        guint           position);
static gboolean   ui_lookup_table_vertical_click_cb    (GtkWidget      *item,
                                                        GdkEventButton *event,
                                                        gpointer        user_data);
static void       ui_lookup_table_horizontal_click_cb  (GtkWidget      *item,
                                                        guint           position,
                                                        gpointer        user_data);
static void       ui_lookup_table_up_button_click_cb   (GtkButton      *button,
                                                        gpointer        user_data);
static void       ui_lookup_table_down_button_click_cb (GtkButton      *button,
                                                        gpointer        user_data);
static gboolean   ui_associate_table_vertical_click_cb (GtkWidget      *item,
                                                        GdkEventButton *event,
                                                        gpointer        user_data);
static void       ui_associate_table_horizontal_click_cb  (GtkWidget      *item,
                                                           guint           position,
                                                           gpointer        user_data);
static void       ui_associate_table_up_button_click_cb   (GtkButton      *button,
                                                           gpointer        user_data);
static void       ui_associate_table_down_button_click_cb (GtkButton      *button,
                                                           gpointer        user_data);
static gboolean   ui_input_window_motion_cb            (GtkWidget      *window,
                                                        GdkEventMotion *event,
                                                        gpointer        user_data);
static gboolean   ui_input_window_click_cb             (GtkWidget      *window,
                                                        GdkEventButton *event,
                                                        gpointer        user_data);

static bool       ui_can_hide_input_window             (void);
static PangoAttrList * create_pango_attrlist           (const String    &str,
                                                        const AttributeList &attrs);

// PanelAgent related functions
static bool       initialize_panel_agent               (const String &config, const String &display, bool resident);
static bool       run_panel_agent                      (void);
static gpointer   panel_agent_thread_func              (gpointer data);

static void       slot_transaction_start               (void);
static void       slot_transaction_end                 (void);
static void       slot_reload_config                   (void);
static void       slot_turn_on                         (void);
static void       slot_turn_off                        (void);
static void       slot_focus_in                        (void);
static void       slot_focus_out                       (void);
static void       slot_show_panel                      (void);
static void       slot_hide_panel                      (void);
static void       slot_update_screen                   (int screen);
static void       slot_update_spot_location            (int x, int y, int top_y);
static void       slot_update_factory_info             (const PanelFactoryInfo &info);
static void       slot_show_preedit_string             (void);
static void       slot_show_aux_string                 (void);
static void       slot_show_lookup_table               (void);
static void       slot_show_associate_table            (void);
static void       slot_hide_preedit_string             (void);
static void       slot_hide_aux_string                 (void);
static void       slot_hide_lookup_table               (void);
static void       slot_hide_associate_table            (void);
static void       slot_update_preedit_string           (const String &str, const AttributeList &attrs);
static void       slot_update_preedit_caret            (int caret);
static void       slot_update_aux_string               (const String &str, const AttributeList &attrs);
static void       slot_update_candidate_table          (const LookupTable &table);
static void       slot_update_associate_table          (const LookupTable &table);
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
static void       slot_set_keyboard_ise                (const String &ise_uuid);
static void       slot_get_keyboard_ise                (String &ise_name, String &ise_uuid);
static void       slot_start_default_ise               ();
static void       slot_send_key_event                  (const KeyEvent &key);
static void       slot_lock                            (void);
static void       slot_unlock                          (void);

static gboolean   candidate_show_timeout_cb            (gpointer data);
static gboolean   check_exit_timeout_cb                (gpointer data);

/////////////////////////////////////////////////////////////////////////////
// Declaration of internal variables.
/////////////////////////////////////////////////////////////////////////////
#if GDK_MULTIHEAD_SAFE
static GdkScreen         *_current_screen                   = 0;
#endif

static GtkWidget         *_input_window                     = 0;
static GtkWidget         *_preedit_area                     = 0;
static GtkWidget         *_aux_area                         = 0;

static GtkWidget         *_lookup_table_window              = 0;
static GtkWidget         *_lookup_table_up_button           = 0;
static GtkWidget         *_lookup_table_down_button         = 0;
static GtkWidget         *_lookup_table_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];

static GtkWidget         *_associate_table_window           = 0;
static GtkWidget         *_associate_table_up_button        = 0;
static GtkWidget         *_associate_table_down_button      = 0;
static GtkWidget         *_associate_table_items [SCIM_LOOKUP_TABLE_MAX_PAGESIZE];

static GtkWidget         *_toolbar_window                   = 0;

static PangoFontDescription *_default_font_desc             = 0;

static gboolean           _input_window_draging             = FALSE;
static gint               _input_window_drag_x              = 0;
static gint               _input_window_drag_y              = 0;
static gint               _input_window_x                   = 0;
static gint               _input_window_y                   = 0;

static bool               _lookup_table_vertical            = false;
static bool               _window_sticked                   = false;

static int                _spot_location_x                  = -1;
static int                _spot_location_y                  = -1;
static int                _spot_location_top_y              = -1;

static int                _lookup_table_index [SCIM_LOOKUP_TABLE_MAX_PAGESIZE+1];
static int                _associate_table_index [SCIM_LOOKUP_TABLE_MAX_PAGESIZE+1];
static int                _lookup_table_index_pos [SCIM_LOOKUP_TABLE_MAX_PAGESIZE+1];
static int                _associate_table_index_pos [SCIM_LOOKUP_TABLE_MAX_PAGESIZE+1];

static GdkColor                      _normal_bg;
static GdkColor                      _normal_text;
static GdkColor                      _active_bg;
static GdkColor                      _active_text;
static GdkColor                     *_theme_color           = NULL;

static ConfigPointer                 _config;

static guint                         _check_exit_timeout    = 0;
static bool                          _should_exit           = false;

static bool                          _panel_is_on           = true;

static GThread                      *_panel_agent_thread    = 0;
PanelAgent                          *_panel_agent           = 0;

static std::vector<HelperInfo>       _helper_list;
static std::vector<String>           _load_ise_list;

static DEFAULT_ISE_T                 _initial_ise;

static int                           _lookup_icon_size      = 0;

int                                  _screen_width          = 0;
static int                           _screen_height         = 0;
float                                _width_rate            = 0;
float                                _height_rate           = 0;
static int                           _softkeybar_height     = 0;
static int                           _panel_width           = 0;
static int                           _panel_height          = 0;
static int                           _setup_button_width    = 0;
static int                           _setup_button_height   = 0;
static int                           _help_icon_width       = 0;
static int                           _help_icon_height      = 0;


static bool                          _candidate_is_showed   = false;
static ISF_CANDIDATE_STYLE_T         _candidate_style       = PREDICTION_ENGINE_CANDIDATE_STYLE;
static ISF_CANDIDATE_MODE_T          _candidate_mode        = PORTRAIT_HORIZONTAL_CANDIDATE_MODE;
static GtkWidget                    *_candidate_scroll      = 0;
static GtkWidget                    *_associate_scroll      = 0;
static GtkWidget                    *_candidate_separator   = 0;
static GtkWidget                    *_associate_separator   = 0;
static GtkWidget                    *_middle_separator      = 0;
static int                           _candidate_left        = -1;
static int                           _candidate_top         = -1;
static bool                          _show_scroll           = false;
static int                           _candidate_motion      = 0;
static gdouble                       _scroll_start          = -10000;
static String                        _split_string1         = String ("");
static String                        _split_string2         = String ("");
static String                        _space_string          = String ("");

static guint                         _candidate_timer       = 0;

#if HAVE_GCONF
static const gchar                  *_ime_setting_ed_id[]   = { "PhoneStatus.Application.Ise.NWordCompPoint",
                                                                "PhoneStatus.Application.Ise.QWordCompPoint",
                                                                "PhoneStatus.Application.Ise.KeyboardEngineOption" };
static const gint                    _ime_setting_ed_number = sizeof (_ime_setting_ed_id) / sizeof (gchar*);
static unsigned int                  _ed_subscription_id[_ime_setting_ed_number] = {0};
#endif

static GdkBitmap                    *_panel_window_mask     = 0;
static GdkPixbuf                    *_panel_bg_pixbuf       = 0;

static ISFSetupWin                  *_isf_setup_win         = 0;
static ISFHelpWin                   *_isf_help_win          = 0;
static GtkListStore                 *_active_ise_list_store = 0;

static clock_t                       _clock_start;

G_LOCK_DEFINE_STATIC                (_global_resource_lock);
G_LOCK_DEFINE_STATIC                (_panel_agent_lock);


/////////////////////////////////////////////////////////////////////////////
// Implementation of internal functions.
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print system time point for panel performance.
 *
 * @param str_info The output information.
 */
static void check_time (const char *str_info)
{
    gettime (_clock_start, str_info);
}

#if HAVE_GCONF
/**
 * @brief Set default language by given language ID.
 *
 * @param id The given language ID.
 */
static void set_default_language_by_id (int id)
{
    String sys_input_language = String (SCIM_CONFIG_SYSTEM_INPUT_LANGUAGE);
#if HAVE_GCONF
    switch (id) {
    case SETTINGS_LANGUAGE_ENGLISH:
        _config->write (sys_input_language, String ("English"));
        break;
    case SETTINGS_LANGUAGE_GERMAN:
        _config->write (sys_input_language, String ("German"));
        break;
    case SETTINGS_LANGUAGE_FRENCH:
        _config->write (sys_input_language, String ("French"));
        break;
    case SETTINGS_LANGUAGE_ITALIAN:
        _config->write (sys_input_language, String ("Italian"));
        break;
    case SETTINGS_LANGUAGE_DUTCH:
        _config->write (sys_input_language, String ("Dutch"));
        break;
    case SETTINGS_LANGUAGE_SPANISH:
        _config->write (sys_input_language, String ("Spanish"));
        break;
    case SETTINGS_LANGUAGE_GREEK:
        _config->write (sys_input_language, String ("Greek"));
        break;
    case SETTINGS_LANGUAGE_PORTUGUESE:
        _config->write (sys_input_language, String ("Portuguese"));
        break;
    case SETTINGS_LANGUAGE_TURKISH:
        _config->write (sys_input_language, String ("Turkish"));
        break;
    default:
        _config->write (sys_input_language, String ("English"));
        break;
    }
#endif

    _config->flush ();
    _config->reload ();
    //_panel_agent->reload_config ();
}

/**
 * @brief Callback function for input language change.
 *
 * @param client The GConf client handler.
 * @param cnxn_id An ID.
 * @param entry The handler of GConf entry.
 * @param user_data The data to pass to this callback.
 */
static void input_lang_key_changed_callback (GConfClient *client,
                                             guint        cnxn_id,
                                             GConfEntry  *entry,
                                             gpointer     user_data)
{
    GConfValue *value = gconf_entry_get_value (entry);
    if (value->type == GCONF_VALUE_INT) {
        int id = gconf_value_get_int (value);
        set_default_language_by_id (id);

        String sys_input_lang;
        sys_input_lang = _config->read (String (SCIM_CONFIG_SYSTEM_INPUT_LANGUAGE), sys_input_lang);

        String lang_info ("English");
        for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
            String lang_name;
            lang_name = scim_get_language_name (it->first);
            if (!strcmp (sys_input_lang.c_str (), lang_name.c_str ()))
                lang_info = sys_input_lang;
        }

        _disabled_langs.clear ();
        _disabled_langs_bak.clear ();

        for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
            String lang_name;
            lang_name = scim_get_language_name (it->first);
            if (strcmp (lang_info.c_str (), lang_name.c_str ())) {
                _disabled_langs.push_back (lang_name);
                _disabled_langs_bak.push_back (lang_name);
            }
        }

        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), lang_info);
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), _disabled_langs);

        for (size_t i = 0; i < _isf_app_list.size (); i++) {
            String active_app = _isf_app_list[i];

            _disabled_ise_map[active_app].clear ();
            _disabled_ise_map_bak[active_app].clear ();
        }

        for (size_t i = 0; i < _isf_app_list.size (); i++) {
            String app = _isf_app_list[i];
            scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_IMENGINE_FACTORIES
                                              "/") + _isf_app_list[i], _disabled_ise_map[app]);
        }

        scim_global_config_flush ();

        update_active_ise_list_store (true);
    }
}
#endif

/**
 * @brief This function is used to judge whether ISE should be hidden in control panel.
 *
 * @param uuid The ISE uuid.
 *
 * @return true if ISE should be hidden, otherwise return false.
 */
static bool hide_ise_in_control_panel (String uuid)
{
    if (uuid.length () <= 0)
        return true;

    for (unsigned int i = 0; i < _helper_list.size (); i++) {
        if (uuid == _helper_list[i].uuid) {
            if (_helper_list[i].option & ISM_ISE_HIDE_IN_CONTROL_PANEL)
                return true;
            else
                return false;
        }
    }

    return false;
}

/**
 * @brief Update active ISE list in control panel.
 *
 * @param set_default_ise The variable decides whether default ISE is set when update.
 */
static void update_active_ise_list_store (bool set_default_ise)
{
    String ise_name = _panel_agent->get_current_ise_name ();

    bool find_current_ise = false;

    gtk_list_store_clear (_active_ise_list_store);
    GtkTreeIter         iter;
    std::vector<String> f_show;
    String active_app = _isf_app_list [0];
    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        String lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ()) {
            for (size_t i = 0; i < it->second.size (); i++) {
                if (std::find (_disabled_ise_map[active_app].begin (), _disabled_ise_map[active_app].end (), _uuids[it->second[i]])
                        ==_disabled_ise_map[active_app].end ()) {
                    if (hide_ise_in_control_panel (_uuids[it->second[i]]))
                        continue;

                    // Protect from add the same ISE more than once in case of multiple langs
                    if (std::find (f_show.begin (), f_show.end (), _uuids[it->second[i]]) == f_show.end ()) {
                        gtk_list_store_append (_active_ise_list_store, &iter);
                        gtk_list_store_set (_active_ise_list_store, &iter,
                                            ACTIVE_ISE_NAME, _names[it->second[i]].c_str (),
                                            ACTIVE_ISE_UUID, _uuids[it->second[i]].c_str (),
                                            ACTIVE_ISE_TYPE, (gint)_modes[it->second[i]],
                                            ACTIVE_ISE_ICON, _icons[it->second[i]].c_str (),
                                            ACTIVE_ISE_OPTION, (gint)_options[it->second[i]], -1);
                        f_show.push_back (_uuids[it->second[i]]);

                        if (!strcmp (_names[it->second[i]].c_str (), ise_name.c_str ())) {
                            gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                                ACTIVE_ISE_LIST_ENABLE, TRUE, -1);
                            find_current_ise = true;
                        } else {
                            gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                                ACTIVE_ISE_LIST_ENABLE, FALSE, -1);
                        }
                    }
                }
            }
        }
    }

    if (!find_current_ise && set_default_ise) {
        std::cerr << " not find current ISE : " << ise_name << "\n";
        _panel_agent->set_default_ise (_initial_ise);
    }
}

/**
 * @brief Callback function for setup button of control panel.
 *
 * @param button The GtkButton handler.
 * @param user_data Data to pass when it is called.
 */
static void on_isf_setup_button_clicked (GtkButton * button,
                                         gpointer user_data)
{
    gboolean has_lookup_popwin = false;
    gboolean has_input_popwin  = false;

    slot_hide_panel ();
    // Check whether candidate window is showed
    if (_lookup_table_window && GTK_WIDGET_VISIBLE (_lookup_table_window)) {
        has_lookup_popwin = true;
        gtk_widget_hide (_lookup_table_window);
    }
    if (_input_window && GTK_WIDGET_VISIBLE (_input_window)) {
        has_input_popwin = true;
        gtk_widget_hide (_input_window);
    }

    if (!_isf_setup_win)
        _isf_setup_win = new ISFSetupWin (_config);
    if (_isf_setup_win)
        _isf_setup_win->show_window ();

    if (_ise_list_changed ||_setup_enable_changed) {
        update_active_ise_list_store (true);
        _ise_list_changed     = false;
        _setup_enable_changed = false;
    }

    if (has_input_popwin) {
        gtk_widget_show (_input_window);
        gdk_window_raise (_input_window->window);
        ui_settle_input_window (false, true);
    }
    if (has_lookup_popwin)
        gtk_widget_show (_lookup_table_window);
    slot_show_panel ();
}

/**
 * @brief Callback function for help button of control panel.
 *
 * @param button The GtkButton handler.
 * @param user_data Data to pass when it is called.
 */
static void on_isf_help_button_clicked (GtkButton * button,
                                        gpointer user_data)
{
    gboolean has_lookup_popwin = false;
    gboolean has_input_popwin  = false;

    slot_hide_panel ();
    // Check whether candidate window is showed
    if (_lookup_table_window && GTK_WIDGET_VISIBLE (_lookup_table_window)) {
        has_lookup_popwin = true;
        gtk_widget_hide (_lookup_table_window);
    }
    if (_input_window && GTK_WIDGET_VISIBLE (_input_window)) {
        has_input_popwin = true;
        gtk_widget_hide (_input_window);
    }

    if (!_isf_help_win)
        _isf_help_win = new ISFHelpWin ();
    if (_isf_help_win)
        _isf_help_win->show_help ("ISF Help", "ISF (Input Service Framework)\n © 2008 SAMSUNG");

    if (has_input_popwin) {
        gtk_widget_show (_input_window);
        gdk_window_raise (_input_window->window);
        ui_settle_input_window (false, true);
    }
    if (has_lookup_popwin)
        gtk_widget_show (_lookup_table_window);
    slot_show_panel ();
}

/**
 * @brief Callback function for press event of ISE help button.
 *
 * @param button The GtkButton handler.
 * @param user_data Data to pass when it is called.
 */
static void on_isf_help_button_pressed (GtkButton * button,
                                        gpointer user_data)
{
    GtkWidget *isf_help_bt_img = ui_create_icon (ISF_ICON_HELP_T_FILE, NULL, _help_icon_width, _help_icon_height, TRUE);
    gtk_button_set_image (GTK_BUTTON (button), isf_help_bt_img);
}

/**
 * @brief Callback function for release event of ISE help button.
 *
 * @param button The GtkButton handler.
 * @param user_data Data to pass when it is called.
 */
static void on_isf_help_button_released (GtkButton * button,
                                         gpointer user_data)
{
    GtkWidget *isf_help_bt_img = ui_create_icon (ISF_ICON_HELP_FILE, NULL, _help_icon_width, _help_icon_height, TRUE);
    gtk_button_set_image (GTK_BUTTON (button), isf_help_bt_img);
}

/**
 * @brief Change keyboard ISE.
 *
 * @param uuid The keyboard ISE's uuid.
 * @param uuid The keyboard ISE's name.
 * @param uuid The keyboard ISE's icon.
 * @param uuid The keyboard ISE's option.
 *
 * @return false if keyboard ISE change is failed, otherwise return true.
 */
static bool ui_active_ise_box_keyboardise_activate_cb (String &uuid, String &name, String &icon, uint32 &option)
{
    SCIM_DEBUG_MAIN (1) << "ui_active_ise_box_keyboardise_activate_cb\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        _panel_agent->stop_helper (pre_uuid);
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        String pre_name = _panel_agent->get_current_ise_name ();
        if (!pre_name.compare (name))
            return false;
    }

    mode = TOOLBAR_KEYBOARD_MODE;
    _panel_agent->set_current_toolbar_mode (mode);
    _panel_agent->set_current_ise_name (name);

    uint32 style = (option & ISM_ISE_FULL_STYLE) != 0;
    _panel_agent->set_current_ise_style (style);

    String language = String ("~other");//scim_get_locale_language (scim_get_current_locale ());
    _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, uuid);
    _config->flush ();

    _panel_agent->change_factory (uuid);

    return true;
}

/**
 * @brief Change helper ISE.
 *
 * @param uuid The helper ISE's uuid.
 * @param uuid The helper ISE's name.
 * @param uuid The helper ISE's icon.
 * @param uuid The helper ISE's option.
 *
 * @return false if helper ISE change is failed, otherwise return true.
 */
static bool ui_active_ise_box_helper_activate_cb (String &uuid, String &name, String &icon, uint32 &option)
{
    SCIM_DEBUG_MAIN (1) << "ui_active_ise_box_helper_activate_cb\n";

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode) {
        String pre_uuid = _panel_agent->get_current_helper_uuid ();
        if (!pre_uuid.compare (uuid))
            return false;
        _panel_agent->stop_helper (pre_uuid);
    } else if (TOOLBAR_KEYBOARD_MODE == mode) {
        //_panel_agent->change_factory ("");
    }

    _panel_agent->set_current_toolbar_mode (TOOLBAR_HELPER_MODE);
    _panel_agent->set_current_ise_name (name);
    _panel_agent->set_current_factory_icon (icon);

    uint32 style = (option & ISM_ISE_FULL_STYLE) != 0;
    _panel_agent->set_current_ise_style (style);

    _panel_agent->start_helper (uuid);
    PanelFactoryInfo not_an_factoryinfo (uuid, name, "", icon);
    slot_update_factory_info (not_an_factoryinfo);

    return true;
}

/**
 * @brief Callback function for click event of ISE enable button.
 *
 * @param cell The GtkCellRendererToggle handler.
 * @param arg1 The string for GtkTreePath.
 * @param data Data pointer to pass when it is called.
 */
static void on_active_ise_enable_box_clicked (GtkCellRendererToggle * cell,
                                              gchar * arg1, gpointer data)
{
    bool         ise_changed = false;
    GtkTreePath *path        = 0;
    GtkTreePath *path_valid  = 0;
    GtkTreeIter  iter, iter_valid;

    std::vector < String >::iterator it;

    path_valid = gtk_tree_path_new_from_string (arg1);
    if (gtk_tree_model_get_iter (GTK_TREE_MODEL (_active_ise_list_store), &iter_valid, path_valid)) {
        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_active_ise_list_store), &iter)) {
            do {
                path = gtk_tree_model_get_path (GTK_TREE_MODEL (_active_ise_list_store), &iter);
                if (gtk_tree_path_compare (path, path_valid) == 0) {
                    gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                        ACTIVE_ISE_LIST_ENABLE, TRUE, -1);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                        ACTIVE_ISE_LIST_ENABLE, FALSE, -1);
                }
                if (path) {
                    gtk_tree_path_free (path);
                    path = 0;
                }
            } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_active_ise_list_store), &iter));
        }

        gchar *g_name = 0;
        gchar *g_uuid = 0;
        gchar *g_icon = 0;
        gint   g_mode;
        gint   g_option;

        gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store),
                            &iter_valid,
                            ACTIVE_ISE_NAME,&g_name,
                            ACTIVE_ISE_UUID,&g_uuid,
                            ACTIVE_ISE_ICON,&g_icon,
                            ACTIVE_ISE_TYPE,&g_mode,
                            ACTIVE_ISE_OPTION,&g_option,
                            -1);

        TOOLBAR_MODE_T mode = (TOOLBAR_MODE_T) g_mode;
        String name (g_name);
        String uuid (g_uuid);
        String icon (g_icon);
        uint32 option = (uint32) g_option;

        if (g_name) {
            g_free (g_name);
            g_name = 0;
        }
        if (g_uuid) {
            g_free (g_uuid);
            g_uuid = 0;
        }
        if (g_icon) {
            g_free (g_icon);
            g_icon = 0;
        }

        if (TOOLBAR_KEYBOARD_MODE == mode)
            ise_changed = ui_active_ise_box_keyboardise_activate_cb (uuid, name, icon, option);
        else if (TOOLBAR_HELPER_MODE == mode)
            ise_changed = ui_active_ise_box_helper_activate_cb (uuid, name, icon, option);

        if (ise_changed) {
            _panel_agent->update_ise_name (name);
            uint32 style = (option & ISM_ISE_FULL_STYLE) != 0;
            _panel_agent->update_ise_style (style);

            DEFAULT_ISE_T default_ise;
            default_ise.type = mode;
            default_ise.uuid = uuid;
            default_ise.name = name;
            _panel_agent->set_default_ise (default_ise);
            //slot_hide_panel ();
        }
    }

    if (path_valid) {
        gtk_tree_path_free (path_valid);
        path_valid = 0;
    }
}

/**
 * @brief Create active ISE list view.
 *
 * @return The GtkWidget handler of active ise list view.
 */
static GtkWidget *create_active_ise_list_view (void)
{
    GtkWidget         *view = NULL;

    GtkCellRenderer   *renderer = NULL;
    GtkTreeViewColumn *column = NULL;

    view = gtk_tree_view_new ();
    g_object_set (view, "enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_NONE, NULL);
    gtk_widget_show (view);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), false);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), false);

    // Invoke ISE in mutually-exclusive
    gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
                                 GTK_SELECTION_SINGLE);

    // Enable column
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Enable"));

    renderer = gtk_cell_renderer_toggle_new ();
    g_object_set (renderer, "indicator-size", (gint)(26*(_width_rate < _height_rate ? _width_rate : _height_rate)), NULL);
    g_object_set (renderer, "xpad", 0, NULL);
    gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), true);
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_cell_renderer_set_fixed_size (renderer, -1, (gint)(31*_height_rate));
    gtk_tree_view_column_set_attributes (column, renderer, "active", ACTIVE_ISE_LIST_ENABLE, NULL);

    g_signal_connect (G_OBJECT (renderer), "toggled",
                      G_CALLBACK (on_active_ise_enable_box_clicked),
                      (gpointer) view);

    gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

    // Name column
    column = gtk_tree_view_column_new ();
    //gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
    //gtk_tree_view_column_set_fixed_width (column, 232);

    gtk_tree_view_column_set_title (column, _("Name"));

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (renderer, "foreground", "black", NULL);
    gtk_tree_view_column_pack_start (column, renderer, TRUE);
    gtk_tree_view_column_set_attributes (column, renderer, "text", ACTIVE_ISE_NAME, NULL);
    gtk_cell_renderer_set_fixed_size (renderer, -1, (gint)(31*_height_rate));
    g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
    g_object_set (renderer, "xpad", (gint)(4*_width_rate), NULL);
    //gtk_cell_renderer_set_cell_font_size (renderer, GTK_CELL_RENDERER_NORMAL, 18);

    gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

    return view;
}

/**
 * @brief Create and fill active ISE list store.
 */
static void create_and_fill_active_ise_list_store (void)
{
    // Create model.
    _active_ise_list_store = gtk_list_store_new (ACTIVE_ISE_NUM_COLUMS, G_TYPE_BOOLEAN,
                                                 G_TYPE_STRING, G_TYPE_STRING,
                                                 G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT);

    update_active_ise_list_store (false);
}

/**
 * @brief Normalize language string.
 *
 * @param src_str The source string for language.
 *
 * @return The normalized language string.
 */
static String get_langs (String src_str)
{
    std::vector<String> str_list, dst_list;
    scim_split_string_list (str_list, src_str);

    for (size_t i = 0; i < str_list.size (); i++)
        dst_list.push_back (scim_get_normalized_language (str_list[i]));

    String dst_str =  scim_combine_string_list (dst_list);

    return dst_str;
}

/**
 * @brief Load all ISEs to initialize.
 *
 * @param config The config pointer for loading keyboard ISE.
 * @param uuids The ISE uuid list.
 * @param names The ISE name list.
 * @param module_names The ISE module name list.
 * @param langs The ISE language list.
 * @param icons The ISE icon list.
 * @param modes The ISE type list.
 * @param options The ISE option list.
 */
static void get_factory_list (const ConfigPointer & config,
                              std::vector < String > &uuids,
                              std::vector < String > &names,
                              std::vector < String > &module_names,
                              std::vector < String > &langs,
                              std::vector < String > &icons,
                              std::vector <TOOLBAR_MODE_T> &modes,
                              std::vector <uint32> &options)
{
    std::vector < String > imengine_list;
    std::vector < String > helper_list;
    std::vector<String>::iterator it;

    IMEngineFactoryPointer factory;
    IMEngineModule         ime_module;

    HelperModule helper_module;
    HelperInfo   helper_info;

    scim_get_imengine_module_list (imengine_list);
    scim_get_helper_module_list   (helper_list);

    uuids.clear ();
    names.clear ();
    module_names.clear ();
    langs.clear ();
    icons.clear ();
    modes.clear ();

    // Add "English/Keyboard" factory first.
    factory = new ComposeKeyFactory ();
    uuids.push_back (factory->get_uuid ());
    names.push_back (utf8_wcstombs (factory->get_name ()));
    String module_name = String ("English/Keyboard");
    module_names.push_back (module_name);
    langs.push_back (get_langs (factory->get_language ()));
    _keyboard_ise_help_map[utf8_wcstombs (factory->get_name ())] = utf8_wcstombs (factory->get_help ());
    icons.push_back (factory->get_icon_file ());
    modes.push_back (TOOLBAR_KEYBOARD_MODE);
    options.push_back (0);

    for (size_t i = 0; i < imengine_list.size (); ++i) {
        if (_load_ise_list.size () > 0) {
            if (std::find (_load_ise_list.begin (), _load_ise_list.end (), imengine_list [i]) == _load_ise_list.end ())
                continue;
        }

        if (imengine_list[i] != "socket") {
            ime_module.load (imengine_list[i], config);

            if (ime_module.valid ()) {
                for (size_t j = 0; j < ime_module.number_of_factories (); ++j) {
                    try {
                        factory = ime_module.create_factory (j);
                    } catch (...) {
                        factory.reset ();
                    }

                    if (!factory.null ()) {
                        if (std::find (uuids.begin (), uuids.end (), factory->get_uuid ()) == uuids.end ()) {
                            uuids.push_back (factory->get_uuid ());
                            names.push_back (utf8_wcstombs (factory->get_name ()));
                            module_name = imengine_list[i];
                            module_names.push_back (module_name);
                            _keyboard_ise_help_map[module_name] = utf8_wcstombs (factory->get_help ());
                            langs.push_back (get_langs (factory->get_language ()));
                            icons.push_back (factory->get_icon_file ());
                            modes.push_back (TOOLBAR_KEYBOARD_MODE);
                            options.push_back (0);
                        }
                        factory.reset ();
                    }
                }
                ime_module.unload ();
            }
        }
    }

    for (size_t i = 0; i < helper_list.size (); ++i) {
        if (_load_ise_list.size () > 0) {
            if (std::find (_load_ise_list.begin (), _load_ise_list.end (), helper_list [i]) == _load_ise_list.end ())
                continue;
        }

        helper_module.load (helper_list[i]);
        if (helper_module.valid ()) {
            for (size_t j = 0; j < helper_module.number_of_helpers (); ++j) {
                helper_module.get_helper_info (j, helper_info);
                uuids.push_back (helper_info.uuid);
                names.push_back (helper_info.name);
                langs.push_back (get_langs (helper_module.get_helper_lang (j)));
                module_names.push_back (helper_list[i]);
                icons.push_back (helper_info.icon);
                modes.push_back (TOOLBAR_HELPER_MODE);
                options.push_back (helper_info.option);
            }
            helper_module.unload ();
        }
    }
}

/**
 * @brief Load all disabled languages.
 *
 * @param disabled This variable is used to store disabled language.
 */
static void  load_disabled_lang_list (std::vector<String> &disabled)
{
    disabled = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), disabled);
}

/**
 * @brief Load ISF configuration and ISEs information.
 */
static void isf_load_config (void)
{
    // Load app list
    _isf_app_list.push_back ("Default");

    // Load ISE engine info
    get_factory_list (_config, _uuids, _names, _module_names, _langs, _icons, _modes, _options);

    std::vector<String> ise_langs;

    for (size_t i = 0; i < _uuids.size (); ++i) {
        scim_split_string_list (ise_langs, _langs[i]);
        for (size_t j = 0; j < ise_langs.size (); j++) {
            if (std::find (_groups[ise_langs[j]].begin (), _groups[ise_langs[j]].end (), i) == _groups[ise_langs[j]].end ())
                _groups[ise_langs[j]].push_back (i);
        }
        ise_langs.clear ();
    }

    std::vector<String> isf_default_langs;
    isf_default_langs = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), isf_default_langs);

    // No default ISF lang
    if (isf_default_langs.size () == 0) {
        String sys_input_lang, lang_info ("English");
        sys_input_lang = _config->read (String (SCIM_CONFIG_SYSTEM_INPUT_LANGUAGE), sys_input_lang);
        MapStringVectorSizeT::iterator g = _groups.find (sys_input_lang);
        if (g != _groups.end ())
            lang_info = sys_input_lang;
        else
            std::cerr << "System input language is not included in the ISF languages.\n";

        for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
            String lang_name = scim_get_language_name (it->first);
            if (strcmp (lang_info.c_str (), lang_name.c_str ())) {
                _disabled_langs.push_back (lang_name);
                _disabled_langs_bak.push_back (lang_name);
            }
        }
    } else {
        std::vector<String> disabled;

        load_disabled_lang_list (disabled);
        for (size_t i = 0; i < disabled.size (); i++) {
            _disabled_langs.push_back (disabled[i]);
            _disabled_langs_bak.push_back (disabled[i]);
        }
    }
}

/**
 * @brief Create control panel window.
 */
static void create_panel_window (void)
{
    check_time ("\nEnter create_panel_window");

    GtkWidget *isf_setup_bt = NULL;
    GtkWidget *isf_help_bt = NULL;
    GtkWidget *scroll = NULL;
    GtkWidget *view = NULL;

    gint       softkeybar_height = _softkeybar_height;

    if (_toolbar_window) {
        gtk_widget_destroy (_toolbar_window);
        _toolbar_window = 0;
    }

    _toolbar_window = gtk_window_new (GTK_WINDOW_POPUP);

#ifdef USING_ISF_SWITCH_BUTTON
    GtkSettings *settings = gtk_widget_get_settings (GTK_WIDGET (_toolbar_window));
    g_object_get (settings, "gtk-softkeybar-height", &softkeybar_height, NULL);
#endif

    gint panel_x = (_screen_width - _panel_width) / 2;
    gint panel_y = _screen_height - softkeybar_height - _panel_height;
    gtk_window_move (GTK_WINDOW (_toolbar_window), panel_x, panel_y);
    gtk_widget_set_size_request (GTK_WIDGET (_toolbar_window), _panel_width, _panel_height);
    gtk_window_set_policy (GTK_WINDOW (_toolbar_window), TRUE, TRUE, FALSE);
    //gtk_window_set_accept_focus (GTK_WINDOW (_toolbar_window), FALSE);
    gtk_window_set_resizable (GTK_WINDOW (_toolbar_window), FALSE);

    GtkWidget *fixed = gtk_fixed_new ();
    gtk_container_add (GTK_CONTAINER (_toolbar_window), fixed);
    gtk_widget_show (fixed);

    GtkWidget *panel_bg = ui_create_icon (ISF_PANEL_BG_FILE, NULL, _panel_width, _panel_height, TRUE);
    _panel_bg_pixbuf    = gtk_image_get_pixbuf (GTK_IMAGE (panel_bg));
    gtk_fixed_put (GTK_FIXED (fixed), panel_bg, 0, 0);
    gtk_widget_show (panel_bg);

    isf_setup_bt = gtk_button_new_with_label ("Setup");

    GtkWidget *newlabel = gtk_label_new ("Setup");
    gtk_label_set_markup (GTK_LABEL (newlabel), "<span foreground='black'>Setup</span>");

    if (newlabel) {
        GtkWidget *old = gtk_bin_get_child (GTK_BIN (isf_setup_bt));
        if (old)
            gtk_container_remove (GTK_CONTAINER (isf_setup_bt), old);
        gtk_container_add (GTK_CONTAINER (isf_setup_bt), newlabel);
        gtk_widget_show (newlabel);
    }
    gtk_widget_set_size_request (isf_setup_bt, _setup_button_width, _setup_button_height);

    gint setup_button_x = (int)(_panel_width - (75+3+31+11)*_width_rate);
    gint setup_button_y = (int)(9 * _height_rate);

    gtk_fixed_put (GTK_FIXED (fixed), isf_setup_bt, setup_button_x, setup_button_y);
    gtk_widget_show (isf_setup_bt);

    g_signal_connect ((gpointer) isf_setup_bt, "clicked",
                      G_CALLBACK (on_isf_setup_button_clicked), NULL);

    GtkWidget *isf_help_bt_img = ui_create_icon (ISF_ICON_HELP_FILE, NULL, _help_icon_width, _help_icon_height, TRUE);
    isf_help_bt = gtk_button_new ();
    gtk_widget_set_size_request (isf_help_bt, _help_icon_width, _help_icon_height);
    gtk_button_set_image (GTK_BUTTON (isf_help_bt), isf_help_bt_img);
    gtk_widget_show (isf_help_bt_img);

    gint help_icon_x = setup_button_x + _setup_button_width + 3*_width_rate;
    gint help_icon_y = setup_button_y;
    gtk_fixed_put (GTK_FIXED (fixed), isf_help_bt, help_icon_x, help_icon_y);
    gtk_widget_show (isf_help_bt);
    g_signal_connect ((gpointer) isf_help_bt, "clicked",
                      G_CALLBACK (on_isf_help_button_clicked), NULL);
    g_signal_connect ((gpointer) isf_help_bt, "pressed",
                      G_CALLBACK (on_isf_help_button_pressed), NULL);
    g_signal_connect ((gpointer) isf_help_bt, "released",
                      G_CALLBACK (on_isf_help_button_released), NULL);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gint scroll_width  = (gint)(_panel_width - 16*_width_rate - 16*_width_rate);
    gint scroll_height = (gint)(_panel_height - 43*_height_rate);
    gtk_widget_set_size_request (scroll, scroll_width, scroll_height);

    gint scroll_x = (gint)(16 * _width_rate);
    gint scroll_y = (gint)(43 * _height_rate);
    gtk_fixed_put (GTK_FIXED (fixed), scroll, scroll_x, scroll_y);
    gtk_widget_show (scroll);

    view = create_active_ise_list_view ();
    if (view && view->style) {
        gtk_widget_modify_bg (view, GTK_STATE_NORMAL, &(view->style->white));

        gtk_tree_view_set_model (GTK_TREE_VIEW  (view),
                                 GTK_TREE_MODEL (_active_ise_list_store));

        gtk_container_add (GTK_CONTAINER (scroll), view);
        gtk_widget_show (view);
    }
    check_time ("Exit create_panel_window");
}

/**
 * @brief Create SCIM style candidate window.
 *
 * @param vertical An indicator for vertical window or horizontal window.
 */
static void create_scim_candidate_window (bool vertical)
{
    GtkWidget *input_window_vbox = NULL;
    GtkWidget *input_window_hbox = NULL;
    gint       input_window_width;
    gint       thickness = 1;

    input_window_width = gdk_screen_width ();

    _lookup_icon_size = input_window_width / 24;
    if (_lookup_icon_size < LOOKUP_ICON_SIZE)
        _lookup_icon_size = LOOKUP_ICON_SIZE;

    // Create input window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *frame = NULL;

        _input_window = gtk_window_new (GTK_WINDOW_POPUP);

        if (_candidate_mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE) {
#if HAVE_GCONF
            gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
#endif
            gtk_widget_set_size_request (GTK_WIDGET (_input_window), gdk_screen_height (), -1);
        } else if (_candidate_mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
#if HAVE_GCONF
            gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
#endif
        }

        //gtk_window_set_keep_above (GTK_WINDOW (_input_window), TRUE);
        gtk_container_set_border_width (GTK_CONTAINER (_input_window), 5);
        //gtk_widget_modify_bg (_input_window, GTK_STATE_NORMAL, &(_active_bg));
        gtk_widget_modify_bg (_input_window, GTK_STATE_NORMAL, &_normal_bg);
        gtk_window_set_policy (GTK_WINDOW (_input_window), TRUE, TRUE, FALSE);
        gtk_window_set_accept_focus (GTK_WINDOW (_input_window), FALSE);
        gtk_window_set_resizable (GTK_WINDOW (_input_window), FALSE);
        gtk_widget_add_events (_input_window, GDK_BUTTON_PRESS_MASK);
        gtk_widget_add_events (_input_window, GDK_BUTTON_RELEASE_MASK);
        gtk_widget_add_events (_input_window, GDK_POINTER_MOTION_MASK);
        g_signal_connect (G_OBJECT (_input_window), "button-press-event",
                          G_CALLBACK (ui_input_window_click_cb),
                          GINT_TO_POINTER (0));
        g_signal_connect (G_OBJECT (_input_window), "button-release-event",
                          G_CALLBACK (ui_input_window_click_cb),
                          GINT_TO_POINTER (1));

        frame = gtk_frame_new (0);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
        gtk_container_add (GTK_CONTAINER (_input_window), frame);
        gtk_widget_show (frame);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (frame), hbox);
        gtk_widget_show (hbox);

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
        gtk_widget_show (vbox);
        input_window_vbox = vbox;

        // Create preedit area
        _preedit_area = scim_string_view_new ();
        //if (_default_font_desc)
        //    gtk_widget_modify_font (_preedit_area, _default_font_desc);
        gtk_widget_modify_base (_preedit_area, GTK_STATE_NORMAL, &_normal_bg);
        gtk_widget_modify_base (_preedit_area, GTK_STATE_ACTIVE, &_active_bg);
        gtk_widget_modify_text (_preedit_area, GTK_STATE_NORMAL, &_normal_text);
        gtk_widget_modify_text (_preedit_area, GTK_STATE_ACTIVE, &_active_text);
        scim_string_view_set_width_chars (SCIM_STRING_VIEW (_preedit_area), 24);
        scim_string_view_set_forward_event (SCIM_STRING_VIEW (_preedit_area), TRUE);
        scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_preedit_area), TRUE);
        scim_string_view_set_has_frame (SCIM_STRING_VIEW (_preedit_area), FALSE);
        g_signal_connect (G_OBJECT (_preedit_area), "move_cursor",
                          G_CALLBACK (ui_preedit_area_move_cursor_cb),
                          0);
        gtk_box_pack_start (GTK_BOX (vbox), _preedit_area, TRUE, TRUE, 0);

        // Create aux area
        _aux_area = scim_string_view_new ();
        //if (_default_font_desc)
        //    gtk_widget_modify_font (_aux_area, _default_font_desc);
        gtk_widget_modify_base (_aux_area, GTK_STATE_NORMAL, &_normal_bg);
        gtk_widget_modify_base (_aux_area, GTK_STATE_ACTIVE, &_active_bg);
        gtk_widget_modify_text (_aux_area, GTK_STATE_NORMAL, &_normal_text);
        gtk_widget_modify_text (_aux_area, GTK_STATE_ACTIVE, &_active_text);
        scim_string_view_set_width_chars (SCIM_STRING_VIEW (_aux_area), 24);
        scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_aux_area), FALSE);
        scim_string_view_set_forward_event (SCIM_STRING_VIEW (_aux_area), TRUE);
        scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_aux_area), TRUE);
        scim_string_view_set_has_frame (SCIM_STRING_VIEW (_aux_area), FALSE);
        gtk_box_pack_start (GTK_BOX (vbox), _aux_area, TRUE, TRUE, 0);
    }

    // Create lookup table window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *lookup_table_parent = NULL;
        GtkWidget *image = NULL;
        GtkWidget *separator = NULL;

        _lookup_table_window = gtk_vbox_new (FALSE, 0);
        if (vertical) {
            input_window_hbox = gtk_hbox_new (FALSE, 0);
            gtk_box_pack_start (GTK_BOX (input_window_vbox), input_window_hbox, TRUE, TRUE, 0);
            gtk_widget_show (input_window_hbox);
            gtk_box_pack_start (GTK_BOX (input_window_hbox), _lookup_table_window, TRUE, TRUE, 0);
        } else {
            gtk_box_pack_start (GTK_BOX (input_window_vbox), _lookup_table_window, TRUE, TRUE, 0);
        }
        lookup_table_parent = _lookup_table_window;
        _candidate_separator = gtk_hseparator_new ();
        GtkStyle *style = gtk_widget_get_style (_candidate_separator);
        if (style != NULL) {
            style->xthickness = thickness;
            style->ythickness = thickness;
            gtk_widget_set_style (_candidate_separator, style);
        }
        gtk_box_pack_start (GTK_BOX (lookup_table_parent), _candidate_separator, FALSE, FALSE, 0);
        gtk_widget_show (_candidate_separator);

        // Vertical lookup table
        if (vertical) {
            vbox = gtk_vbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (lookup_table_parent), vbox);
            gtk_widget_show (vbox);

            // New table items
            for (int i=0; i<SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
                _lookup_table_items [i] = scim_string_view_new ();
                //if (_default_font_desc)
                //    gtk_widget_modify_font (_lookup_table_items [i], _default_font_desc);
                gtk_widget_modify_base (_lookup_table_items [i], GTK_STATE_NORMAL, &_normal_bg);
                gtk_widget_modify_base (_lookup_table_items [i], GTK_STATE_ACTIVE, &_active_bg);
                gtk_widget_modify_text (_lookup_table_items [i], GTK_STATE_NORMAL, &_normal_text);
                gtk_widget_modify_text (_lookup_table_items [i], GTK_STATE_ACTIVE, &_active_text);
                scim_string_view_set_width_chars (SCIM_STRING_VIEW (_lookup_table_items [i]), 80);
                scim_string_view_set_has_frame (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                scim_string_view_set_forward_event (SCIM_STRING_VIEW (_lookup_table_items [i]), TRUE);
                scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_lookup_table_items [i]), TRUE);
                scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                g_signal_connect (G_OBJECT (_lookup_table_items [i]), "button-press-event",
                                  G_CALLBACK (ui_lookup_table_vertical_click_cb),
                                  GINT_TO_POINTER (i));
                gtk_box_pack_start (GTK_BOX (vbox), _lookup_table_items [i], FALSE, FALSE, 0);
            }

            hbox = gtk_hbox_new (FALSE, 0);
            gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
            gtk_widget_show (hbox);

            // New down button
            image = ui_create_down_icon ();
            _lookup_table_down_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_lookup_table_down_button), image);
            gtk_widget_show (image);
            gtk_box_pack_end (GTK_BOX (hbox), _lookup_table_down_button, TRUE, TRUE, 0);
            gtk_widget_show (_lookup_table_down_button);
            g_signal_connect (G_OBJECT (_lookup_table_down_button), "clicked",
                              G_CALLBACK (ui_lookup_table_down_button_click_cb),
                              image);

            // New up button
            image = ui_create_up_icon ();
            _lookup_table_up_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_lookup_table_up_button), image);
            gtk_widget_show (image);
            gtk_box_pack_end (GTK_BOX (hbox), _lookup_table_up_button, TRUE, TRUE, 0);
            gtk_widget_show (_lookup_table_up_button);
            g_signal_connect (G_OBJECT (_lookup_table_up_button), "clicked",
                              G_CALLBACK (ui_lookup_table_up_button_click_cb),
                              image);

            separator = gtk_hseparator_new ();
            GtkStyle *style = gtk_widget_get_style (separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (separator, style);
            }
            //gtk_box_pack_end (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
            gtk_widget_show (separator);
        } else {
            gtk_widget_set_size_request (GTK_WIDGET (_input_window), input_window_width, -1);
            hbox = gtk_hbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (lookup_table_parent), hbox);
            gtk_widget_show (hbox);

            _lookup_table_items [0] = scim_string_view_new ();
            //if (_default_font_desc)
            //    gtk_widget_modify_font (_lookup_table_items [0], _default_font_desc);
            gtk_widget_modify_base (_lookup_table_items [0], GTK_STATE_NORMAL, &_normal_bg);
            gtk_widget_modify_base (_lookup_table_items [0], GTK_STATE_ACTIVE, &_active_bg);
            gtk_widget_modify_text (_lookup_table_items [0], GTK_STATE_NORMAL, &_normal_text);
            gtk_widget_modify_text (_lookup_table_items [0], GTK_STATE_ACTIVE, &_active_text);
            scim_string_view_set_forward_event (SCIM_STRING_VIEW (_lookup_table_items [0]), TRUE);
            scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_lookup_table_items [0]), TRUE);
            scim_string_view_set_has_frame (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            g_signal_connect (G_OBJECT (_lookup_table_items [0]), "move_cursor",
                              G_CALLBACK (ui_lookup_table_horizontal_click_cb),
                              0);
            gtk_box_pack_start (GTK_BOX (hbox), _lookup_table_items [0], TRUE, TRUE, 0);
            gtk_widget_show (_lookup_table_items [0]);

            separator = gtk_vseparator_new ();
            GtkStyle *style = gtk_widget_get_style (separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (separator, style);
            }
            //gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);
            gtk_widget_show (separator);

            // New left button
            image = ui_create_left_icon ();
            _lookup_table_up_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_lookup_table_up_button), image);
            gtk_widget_show (image);
            gtk_box_pack_start (GTK_BOX (hbox), _lookup_table_up_button, FALSE, FALSE, 0);
            gtk_widget_show (_lookup_table_up_button);
            g_signal_connect (G_OBJECT (_lookup_table_up_button), "clicked",
                              G_CALLBACK (ui_lookup_table_up_button_click_cb),
                              image);

            // New right button
            image = ui_create_right_icon ();
            _lookup_table_down_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_lookup_table_down_button), image);
            gtk_widget_show (image);
            gtk_box_pack_start (GTK_BOX (hbox), _lookup_table_down_button, FALSE, FALSE, 0);
            gtk_widget_show (_lookup_table_down_button);
            g_signal_connect (G_OBJECT (_lookup_table_down_button), "clicked",
                              G_CALLBACK (ui_lookup_table_down_button_click_cb),
                              image);
        }

        gtk_button_set_relief (GTK_BUTTON (_lookup_table_up_button), GTK_RELIEF_NONE);
        gtk_widget_modify_bg (_lookup_table_up_button, GTK_STATE_ACTIVE, &_normal_bg);
        gtk_widget_modify_bg (_lookup_table_up_button, GTK_STATE_INSENSITIVE, &_normal_bg);
        gtk_widget_modify_bg (_lookup_table_up_button, GTK_STATE_PRELIGHT, &_normal_bg);

        gtk_button_set_relief (GTK_BUTTON (_lookup_table_down_button), GTK_RELIEF_NONE);
        gtk_widget_modify_bg (_lookup_table_down_button, GTK_STATE_ACTIVE, &_normal_bg);
        gtk_widget_modify_bg (_lookup_table_down_button, GTK_STATE_INSENSITIVE, &_normal_bg);
        gtk_widget_modify_bg (_lookup_table_down_button, GTK_STATE_PRELIGHT, &_normal_bg);
    }

    // Create associate table window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *image = NULL;
        GtkWidget *separator = NULL;
        GtkWidget *associate_table_parent = NULL;

        _associate_table_window = gtk_vbox_new (FALSE, 0);
        if (vertical) {
            _middle_separator = gtk_vseparator_new ();
            GtkStyle *style = gtk_widget_get_style (_middle_separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (_middle_separator, style);
            }
            gtk_box_pack_start (GTK_BOX (input_window_hbox), _middle_separator, FALSE, FALSE, 0);
            gtk_widget_show (_middle_separator);

            gtk_box_pack_start (GTK_BOX (input_window_hbox), _associate_table_window, TRUE, TRUE, 0);
        } else {
            gtk_box_pack_start (GTK_BOX (input_window_vbox), _associate_table_window, TRUE, TRUE, 0);
        }
        associate_table_parent = _associate_table_window;
        _associate_separator = gtk_hseparator_new ();
        GtkStyle *style = gtk_widget_get_style (_associate_separator);
        if (style != NULL) {
            style->xthickness = thickness;
            style->ythickness = thickness;
            gtk_widget_set_style (_associate_separator, style);
        }
        gtk_box_pack_start (GTK_BOX (associate_table_parent), _associate_separator, FALSE, FALSE, 0);
        gtk_widget_show (_associate_separator);

        if (vertical) {
            vbox = gtk_vbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (associate_table_parent), vbox);
            gtk_widget_show (vbox);

            for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
                _associate_table_items [i] = scim_string_view_new ();
                //if (_default_font_desc)
                //    gtk_widget_modify_font (_associate_table_items [i], _default_font_desc);
                gtk_widget_modify_base (_associate_table_items [i], GTK_STATE_NORMAL, &_normal_bg);
                gtk_widget_modify_base (_associate_table_items [i], GTK_STATE_ACTIVE, &_active_bg);
                gtk_widget_modify_text (_associate_table_items [i], GTK_STATE_NORMAL, &_normal_text);
                gtk_widget_modify_text (_associate_table_items [i], GTK_STATE_ACTIVE, &_active_text);
                scim_string_view_set_width_chars (SCIM_STRING_VIEW (_associate_table_items [i]), 80);
                scim_string_view_set_has_frame (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                scim_string_view_set_forward_event (SCIM_STRING_VIEW (_associate_table_items [i]), TRUE);
                scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_associate_table_items [i]), TRUE);
                scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                g_signal_connect (G_OBJECT (_associate_table_items [i]), "button-press-event",
                                  G_CALLBACK (ui_associate_table_vertical_click_cb),
                                  GINT_TO_POINTER (i));
                gtk_box_pack_start (GTK_BOX (vbox), _associate_table_items [i], FALSE, FALSE, 0);
            }

            hbox = gtk_hbox_new (FALSE, 0);
            gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
            gtk_widget_show (hbox);

            // New down button
            image = ui_create_down_icon ();
            _associate_table_down_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_associate_table_down_button), image);
            gtk_widget_show (image);
            gtk_box_pack_end (GTK_BOX (hbox), _associate_table_down_button, TRUE, TRUE, 0);
            gtk_widget_show (_associate_table_down_button);
            g_signal_connect (G_OBJECT (_associate_table_down_button), "clicked",
                              G_CALLBACK (ui_associate_table_down_button_click_cb),
                              image);

            // New up button
            image = ui_create_up_icon ();
            _associate_table_up_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_associate_table_up_button), image);
            gtk_widget_show (image);
            gtk_box_pack_end (GTK_BOX (hbox), _associate_table_up_button, TRUE, TRUE, 0);
            gtk_widget_show (_associate_table_up_button);
            g_signal_connect (G_OBJECT (_associate_table_up_button), "clicked",
                              G_CALLBACK (ui_associate_table_up_button_click_cb),
                              image);

            separator = gtk_hseparator_new ();
            GtkStyle *style = gtk_widget_get_style (separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (separator, style);
            }
            //gtk_box_pack_end (GTK_BOX (vbox), separator, FALSE, FALSE, 0);
            gtk_widget_show (separator);
        } else {
            hbox = gtk_hbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (associate_table_parent), hbox);
            gtk_widget_show (hbox);

            _associate_table_items [0] = scim_string_view_new ();
            //if (_default_font_desc)
            //    gtk_widget_modify_font (_associate_table_items [0], _default_font_desc);
            gtk_widget_modify_base (_associate_table_items [0], GTK_STATE_NORMAL, &_normal_bg);
            gtk_widget_modify_base (_associate_table_items [0], GTK_STATE_ACTIVE, &_active_bg);
            gtk_widget_modify_text (_associate_table_items [0], GTK_STATE_NORMAL, &_normal_text);
            gtk_widget_modify_text (_associate_table_items [0], GTK_STATE_ACTIVE, &_active_text);
            scim_string_view_set_forward_event (SCIM_STRING_VIEW (_associate_table_items [0]), TRUE);
            scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_associate_table_items [0]), TRUE);
            scim_string_view_set_has_frame (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            g_signal_connect (G_OBJECT (_associate_table_items [0]), "move_cursor",
                              G_CALLBACK (ui_associate_table_horizontal_click_cb),
                              0);
            gtk_box_pack_start (GTK_BOX (hbox), _associate_table_items [0], TRUE, TRUE, 0);
            gtk_widget_show (_associate_table_items [0]);

            separator = gtk_vseparator_new ();
            GtkStyle *style = gtk_widget_get_style (separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (separator, style);
            }
            //gtk_box_pack_start (GTK_BOX (hbox), separator, FALSE, FALSE, 0);
            gtk_widget_show (separator);

            // New right button
            image = ui_create_left_icon ();
            _associate_table_up_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_associate_table_up_button), image);
            gtk_widget_show (image);
            gtk_box_pack_start (GTK_BOX (hbox), _associate_table_up_button, FALSE, FALSE, 0);
            gtk_widget_show (_associate_table_up_button);
            g_signal_connect (G_OBJECT (_associate_table_up_button), "clicked",
                              G_CALLBACK (ui_associate_table_up_button_click_cb),
                              image);

            // New left button
            image = ui_create_right_icon ();
            _associate_table_down_button = gtk_button_new ();
            gtk_container_add (GTK_CONTAINER (_associate_table_down_button), image);
            gtk_widget_show (image);
            gtk_box_pack_start (GTK_BOX (hbox), _associate_table_down_button, FALSE, FALSE, 0);
            gtk_widget_show (_associate_table_down_button);
            g_signal_connect (G_OBJECT (_associate_table_down_button), "clicked",
                              G_CALLBACK (ui_associate_table_down_button_click_cb),
                              image);
        }

        gtk_button_set_relief (GTK_BUTTON (_associate_table_up_button), GTK_RELIEF_NONE);
        gtk_widget_modify_bg (_associate_table_up_button, GTK_STATE_ACTIVE, &_normal_bg);
        gtk_widget_modify_bg (_associate_table_up_button, GTK_STATE_INSENSITIVE, &_normal_bg);
        gtk_widget_modify_bg (_associate_table_up_button, GTK_STATE_PRELIGHT, &_normal_bg);

        gtk_button_set_relief (GTK_BUTTON (_associate_table_down_button), GTK_RELIEF_NONE);
        gtk_widget_modify_bg (_associate_table_down_button, GTK_STATE_ACTIVE, &_normal_bg);
        gtk_widget_modify_bg (_associate_table_down_button, GTK_STATE_INSENSITIVE, &_normal_bg);
        gtk_widget_modify_bg (_associate_table_down_button, GTK_STATE_PRELIGHT, &_normal_bg);
    }
}

/**
 * @brief Callback function for press event of candidate scroller.
 *
 * @param widget The handler of candidate scroller.
 * @param event The GdkEvent pointer for this event.
 * @param user_data Data pointer to pass when it is called.
 */
static gboolean candidate_scroll_button_press_cb (GtkWidget *widget,
                                                  GdkEvent  *event,
                                                  gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "[candidate_scroll_button_press_cb] event type=" << event->type << "\n";

    if (_scroll_start < -9999) {
        GtkWidget *hscrollbar = gtk_scrolled_window_get_hscrollbar (GTK_SCROLLED_WINDOW (widget));
        _scroll_start = gtk_range_get_value (GTK_RANGE (hscrollbar));
    }
    _show_scroll = true;

    return FALSE;
}

/**
 * @brief Callback function for motion event of candidate scroller.
 *
 * @param widget The handler of candidate scroller.
 * @param event The GdkEvent pointer for this event.
 * @param user_data Data pointer to pass when it is called.
 */
static gboolean candidate_scroll_motion_cb (GtkWidget *widget,
                                            GdkEvent  *event,
                                            gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "[candidate_scroll_motion_cb] event type=" << event->type << "\n";

    _candidate_motion++;
    if (_candidate_motion > 1)
        _show_scroll = true;

    return FALSE;
}

/**
 * @brief Callback function for flick event of candidate scroller.
 *
 * @param widget The handler of candidate scroller.
 * @param event The GdkEvent pointer for this event.
 * @param user_data Data pointer to pass when it is called.
 */
static gboolean candidate_scroll_flick_cb (GtkWidget *widget,
                                           GdkEvent  *event,
                                           gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "[candidate_scroll_flick_cb] event type=" << event->type << "\n";

    int *index_pos = NULL;
    if (GPOINTER_TO_INT (user_data) == 1) {
        scim_string_view_set_highlight (SCIM_STRING_VIEW (_lookup_table_items [0]), -1, -1);
        index_pos = _lookup_table_index_pos;
    } else if (GPOINTER_TO_INT (user_data) == 2) {
        scim_string_view_set_highlight (SCIM_STRING_VIEW (_associate_table_items [0]), -1, -1);
        index_pos = _associate_table_index_pos;
    }

    if (event->type != GDK_BUTTON_RELEASE && index_pos != NULL) {
        GtkWidget *hscrollbar = gtk_scrolled_window_get_hscrollbar (GTK_SCROLLED_WINDOW (widget));
        gdouble pos = gtk_range_get_value (GTK_RANGE (hscrollbar));
        int new_pos = 0;
        for (int i = 1; (int)pos > 0; i++) {
            if (index_pos[i] <= 0 || ((pos-index_pos[i] > -0.01) && (pos-index_pos[i] < 0.01)))
                break;
            if (pos < index_pos[i]) {
                if (_scroll_start < pos)
                    new_pos = index_pos[i];
                else
                    new_pos = index_pos[i-1];
                gtk_range_set_value (GTK_RANGE (hscrollbar), new_pos);
                break;
            }
        }
    }

    _candidate_motion = 0;
    _scroll_start     = -10000;
    _show_scroll      = false;

    return FALSE;
}

/**
 * @brief Create prediction engine style candidate window.
 *
 * @param vertical An indicator for vertical window or horizontal window.
 */
static void create_prediction_engine_candidate_window (bool vertical)
{
    GtkWidget *input_window_vbox = NULL;
    GtkWidget *input_window_hbox = NULL;
    gint       input_window_width;
    gint       thickness    = 1;
    gint       border_width = 2;
    gint       new_height   = ((69 * gdk_screen_height ()) / 800) - (2 * border_width);

    input_window_width = gdk_screen_width ();

    _lookup_icon_size = input_window_width / 24;
    if (_lookup_icon_size < LOOKUP_ICON_SIZE)
        _lookup_icon_size = LOOKUP_ICON_SIZE;

    GtkWidget *default_label = gtk_label_new ("default label");
    GtkStyle  *label_style  = gtk_widget_get_style (default_label);
#ifdef USING_ISF_SWITCH_BUTTON
    GdkColor   base_normal  = label_style->base[GTK_STATE_NORMAL];
    GdkColor   base_active  = label_style->base[GTK_STATE_ACTIVE];
    GdkColor   text_normal  = label_style->text[GTK_STATE_NORMAL];
    GdkColor   text_active  = label_style->text[GTK_STATE_ACTIVE];
#else
    GdkColor   base_normal  = label_style->bg[GTK_STATE_NORMAL];
    GdkColor   base_active  = label_style->bg[GTK_STATE_ACTIVE];
    GdkColor   text_normal  = label_style->fg[GTK_STATE_NORMAL];
    GdkColor   text_active  = label_style->fg[GTK_STATE_ACTIVE];
#endif

    GdkColor text_insensitive;
    gdk_color_parse ("#333333", &text_insensitive);
    //String strColor = _config->read (String (SCIM_CONFIG_PANEL_GTK_COLOR_SEPARATOR), String ("#333333"));
    //gdk_color_parse (strColor.c_str (), &text_insensitive);
    //_config->write (String (SCIM_CONFIG_PANEL_GTK_COLOR_SEPARATOR), strColor);

    gdk_color_parse ("#2B2B2B", &base_normal);

    // Create input window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *frame = NULL;
        GtkWidget *event_box = NULL;

        _input_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_type_hint (GTK_WINDOW (_input_window), GDK_WINDOW_TYPE_HINT_UTILITY);

        if (!vertical)
            gtk_widget_set_size_request (GTK_WIDGET (_input_window), input_window_width, -1);
        if (_candidate_mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE) {
#if HAVE_GCONF
            gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
#endif
            gtk_widget_set_size_request (GTK_WIDGET (_input_window), gdk_screen_height (), -1);
        } else if (_candidate_mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
#if HAVE_GCONF
            gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
#endif
        }

        //gtk_window_set_type_hint (GTK_WINDOW (_input_window), GDK_WINDOW_TYPE_HINT_NOTIFICATION);
        //gtk_window_set_keep_above (GTK_WINDOW (_input_window), TRUE);
        gtk_container_set_border_width (GTK_CONTAINER (_input_window), border_width);
        //gtk_widget_modify_bg (_input_window, GTK_STATE_NORMAL, &(text_insensitive));
        gtk_window_set_policy (GTK_WINDOW (_input_window), TRUE, TRUE, FALSE);
        gtk_window_set_accept_focus (GTK_WINDOW (_input_window), FALSE);
        gtk_window_set_resizable (GTK_WINDOW (_input_window), FALSE);
        gtk_widget_add_events (_input_window, GDK_BUTTON_PRESS_MASK);
        gtk_widget_add_events (_input_window, GDK_BUTTON_RELEASE_MASK);
        gtk_widget_add_events (_input_window, GDK_POINTER_MOTION_MASK);
        g_signal_connect (G_OBJECT (_input_window), "button-press-event",
                          G_CALLBACK (ui_input_window_click_cb),
                          GINT_TO_POINTER (0));
        g_signal_connect (G_OBJECT (_input_window), "button-release-event",
                          G_CALLBACK (ui_input_window_click_cb),
                          GINT_TO_POINTER (1));

        event_box = gtk_event_box_new ();
        gtk_container_add (GTK_CONTAINER (_input_window), event_box);
        gtk_widget_show (event_box);

        frame = gtk_frame_new (0);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);
        gtk_container_add (GTK_CONTAINER (event_box), frame);
        gtk_widget_show (frame);

        hbox = gtk_hbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (frame), hbox);
        gtk_widget_show (hbox);

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
        gtk_widget_show (vbox);
        input_window_vbox = vbox;

        // Create preedit area
        _preedit_area = scim_string_view_new ();
        gtk_widget_modify_base (_preedit_area, GTK_STATE_NORMAL, &(base_normal));
        gtk_widget_modify_base (_preedit_area, GTK_STATE_ACTIVE, &(base_active));
        gtk_widget_modify_text (_preedit_area, GTK_STATE_NORMAL, &(text_normal));
        gtk_widget_modify_text (_preedit_area, GTK_STATE_ACTIVE, &(text_active));
        scim_string_view_set_width_chars (SCIM_STRING_VIEW (_preedit_area), 24);
        scim_string_view_set_forward_event (SCIM_STRING_VIEW (_preedit_area), TRUE);
        scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_preedit_area), TRUE);
        scim_string_view_set_has_frame (SCIM_STRING_VIEW (_preedit_area), FALSE);
        g_signal_connect (G_OBJECT (_preedit_area), "move_cursor",
                          G_CALLBACK (ui_preedit_area_move_cursor_cb),
                          0);
        gtk_box_pack_start (GTK_BOX (vbox), _preedit_area, TRUE, TRUE, 0);

        // Create aux area
        _aux_area = scim_string_view_new ();
        if (_default_font_desc)
            gtk_widget_modify_font (_aux_area, _default_font_desc);
        gtk_widget_modify_base (_aux_area, GTK_STATE_NORMAL, &(base_normal));
        gtk_widget_modify_base (_aux_area, GTK_STATE_ACTIVE, &(base_active));
        gtk_widget_modify_text (_aux_area, GTK_STATE_NORMAL, &(text_normal));
        gtk_widget_modify_text (_aux_area, GTK_STATE_ACTIVE, &(text_active));
        scim_string_view_set_width_chars (SCIM_STRING_VIEW (_aux_area), 24);
        scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_aux_area), FALSE);
        scim_string_view_set_forward_event (SCIM_STRING_VIEW (_aux_area), TRUE);
        scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_aux_area), TRUE);
        scim_string_view_set_has_frame (SCIM_STRING_VIEW (_aux_area), FALSE);
        gtk_box_pack_start (GTK_BOX (vbox), _aux_area, TRUE, TRUE, 0);
    }

    // Create lookup table window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *lookup_table_parent = NULL;

        {
            _lookup_table_window = gtk_vbox_new (FALSE, 0);
            if (vertical) {
                input_window_hbox = gtk_hbox_new (FALSE, 0);
                gtk_box_pack_start (GTK_BOX (input_window_vbox), input_window_hbox, TRUE, TRUE, 0);
                gtk_widget_show (input_window_hbox);
                gtk_box_pack_start (GTK_BOX (input_window_hbox), _lookup_table_window, TRUE, TRUE, 0);
            } else {
                gtk_box_pack_start (GTK_BOX (input_window_vbox), _lookup_table_window, TRUE, TRUE, 0);
            }
            lookup_table_parent = _lookup_table_window;
            _candidate_separator = gtk_hseparator_new ();
            GtkStyle *style = gtk_widget_get_style (_candidate_separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (_candidate_separator, style);
            }
            gtk_box_pack_start (GTK_BOX (lookup_table_parent), _candidate_separator, FALSE, FALSE, 0);
            gtk_widget_show (_candidate_separator);
        }

        // Vertical lookup table
        if (vertical) {
            vbox = gtk_vbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (lookup_table_parent), vbox);
            gtk_widget_show (vbox);

            _candidate_scroll = gtk_scrolled_window_new (NULL, NULL);
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_candidate_scroll),
                                            GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
            gtk_widget_set_size_request (_candidate_scroll, -1, gdk_screen_height () / 3);
            gtk_box_pack_start (GTK_BOX (vbox), _candidate_scroll, TRUE, TRUE, 0);
            gtk_widget_show (_candidate_scroll);

            hbox = gtk_hbox_new (FALSE, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (_candidate_scroll), hbox);
            gtk_widget_show (hbox);

            GtkWidget *ilabel = gtk_label_new (" ");
            gtk_widget_set_size_request (ilabel, BLANK_SIZE, -1);
            gtk_box_pack_start (GTK_BOX (hbox), ilabel, FALSE, FALSE, 0);
            gtk_widget_show (ilabel);

            GtkWidget *vbox_candidate = gtk_vbox_new (FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), vbox_candidate, TRUE, TRUE, 0);
            gtk_widget_show (vbox_candidate);

            // New table items
            for (int i=0; i<SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
                _lookup_table_items [i] = scim_string_view_new ();
                if (_default_font_desc)
                    gtk_widget_modify_font (_lookup_table_items [i], _default_font_desc);
                gtk_widget_modify_base (_lookup_table_items [i], GTK_STATE_NORMAL, &(base_normal));
                gtk_widget_modify_base (_lookup_table_items [i], GTK_STATE_ACTIVE, &(base_active));
                gtk_widget_modify_text (_lookup_table_items [i], GTK_STATE_NORMAL, &(text_normal));
                gtk_widget_modify_text (_lookup_table_items [i], GTK_STATE_ACTIVE, &(text_active));
                scim_string_view_set_width_chars (SCIM_STRING_VIEW (_lookup_table_items [i]), 80);
                scim_string_view_set_has_frame (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                scim_string_view_set_forward_event (SCIM_STRING_VIEW (_lookup_table_items [i]), TRUE);
                scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_lookup_table_items [i]), TRUE);
                scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_lookup_table_items [i]), FALSE);
                g_signal_connect (G_OBJECT (_lookup_table_items [i]), "button-release-event",
                                  G_CALLBACK (ui_lookup_table_vertical_click_cb),
                                  GINT_TO_POINTER (i));
                gtk_box_pack_start (GTK_BOX (vbox_candidate), _lookup_table_items [i], FALSE, FALSE, 0);
            }
        } else {
            hbox = gtk_hbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (lookup_table_parent), hbox);
            gtk_widget_show (hbox);

            _candidate_scroll = gtk_scrolled_window_new (NULL, NULL);
#if HAVE_GCONF
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_candidate_scroll),
                                            GTK_POLICY_AUTOHIDE, GTK_POLICY_NEVER);
#else
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_candidate_scroll),
                                            GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
#endif
            gtk_box_pack_start (GTK_BOX (hbox), _candidate_scroll, TRUE, TRUE, 0);
            gtk_widget_show (_candidate_scroll);
            g_signal_connect (G_OBJECT (_candidate_scroll), "button-press-event",
                              G_CALLBACK (candidate_scroll_button_press_cb),
                              GINT_TO_POINTER (1));
            g_signal_connect (G_OBJECT (_candidate_scroll), "button-release-event",
                              G_CALLBACK (candidate_scroll_flick_cb),
                              GINT_TO_POINTER (1));
            g_signal_connect (G_OBJECT (_candidate_scroll), "motion-notify-event",
                              G_CALLBACK (candidate_scroll_motion_cb),
                              GINT_TO_POINTER (1));
            g_signal_connect (G_OBJECT (_candidate_scroll), "flick-event",
                              G_CALLBACK (candidate_scroll_flick_cb),
                              GINT_TO_POINTER (1));

            vbox = gtk_vbox_new (FALSE, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (_candidate_scroll), vbox);
            gtk_widget_show (vbox);

            _lookup_table_items [0] = scim_string_view_new ();
            if (_default_font_desc)
                gtk_widget_modify_font (_lookup_table_items [0], _default_font_desc);
            gtk_widget_modify_base (_lookup_table_items [0], GTK_STATE_NORMAL, &(base_normal));
            gtk_widget_modify_base (_lookup_table_items [0], GTK_STATE_ACTIVE, &(base_active));
            gtk_widget_modify_text (_lookup_table_items [0], GTK_STATE_NORMAL, &(text_normal));
            gtk_widget_modify_text (_lookup_table_items [0], GTK_STATE_ACTIVE, &(text_active));
            gtk_widget_modify_text (_lookup_table_items [0], GTK_STATE_INSENSITIVE, &(text_insensitive));
            scim_string_view_set_forward_event (SCIM_STRING_VIEW (_lookup_table_items [0]), TRUE);
            scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_lookup_table_items [0]), TRUE);
            scim_string_view_set_press_move_cursor (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            scim_string_view_set_separator_index (SCIM_STRING_VIEW (_lookup_table_items [0]), _lookup_table_index);
            scim_string_view_set_has_frame (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_lookup_table_items [0]), FALSE);
            g_signal_connect (G_OBJECT (_lookup_table_items [0]), "move_cursor",
                              G_CALLBACK (ui_lookup_table_horizontal_click_cb),
                              0);

            gtk_box_pack_start (GTK_BOX (vbox), _lookup_table_items [0], FALSE, FALSE, 0);
            gtk_widget_show (_lookup_table_items [0]);

            // Resize candidate height
            GtkRequisition scroll_size, lookup_size;
            gtk_widget_size_request (_candidate_scroll, &scroll_size);
            if (scroll_size.height >= new_height) {
                gtk_widget_set_size_request (GTK_WIDGET (_candidate_scroll), -1, scroll_size.height);
            } else {
                gtk_widget_size_request (_lookup_table_items [0], &lookup_size);
                gtk_widget_set_size_request (GTK_WIDGET (_candidate_scroll), -1, new_height);
                gtk_widget_set_size_request (GTK_WIDGET (_lookup_table_items [0]), -1,
                                             lookup_size.height + (new_height - scroll_size.height));
            }
        }
    }

    //Create associate table window
    {
        GtkWidget *vbox = NULL;
        GtkWidget *hbox = NULL;
        GtkWidget *associate_table_parent = NULL;

        _associate_table_window = gtk_vbox_new (FALSE, 0);
        if (vertical) {
            _middle_separator = gtk_vseparator_new ();
            GtkStyle *style = gtk_widget_get_style (_middle_separator);
            if (style != NULL) {
                style->xthickness = thickness;
                style->ythickness = thickness;
                gtk_widget_set_style (_middle_separator, style);
            }
            //gtk_box_pack_start (GTK_BOX (input_window_hbox), _middle_separator, FALSE, FALSE, 0);
            gtk_widget_show (_middle_separator);

            gtk_box_pack_start (GTK_BOX (input_window_hbox), _associate_table_window, TRUE, TRUE, 0);
        } else {
            gtk_box_pack_start (GTK_BOX (input_window_vbox), _associate_table_window, TRUE, TRUE, 0);
        }
        associate_table_parent = _associate_table_window;
        _associate_separator = gtk_hseparator_new ();
        GtkStyle *style = gtk_widget_get_style (_associate_separator);
        if (style != NULL) {
            style->xthickness = thickness;
            style->ythickness = thickness;
            gtk_widget_set_style (_associate_separator, style);
        }
        gtk_box_pack_start (GTK_BOX (associate_table_parent), _associate_separator, FALSE, FALSE, 0);
        gtk_widget_show (_associate_separator);

        if (vertical) {
            vbox = gtk_vbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (associate_table_parent), vbox);
            gtk_widget_show (vbox);

            _associate_scroll = gtk_scrolled_window_new (NULL, NULL);
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_associate_scroll),
                                            GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
            gtk_widget_set_size_request (_associate_scroll, -1, gdk_screen_height () / 3);
            gtk_box_pack_start (GTK_BOX (vbox), _associate_scroll, TRUE, TRUE, 0);
            gtk_widget_show (_associate_scroll);

            hbox = gtk_hbox_new (FALSE, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (_associate_scroll), hbox);
            gtk_widget_show (hbox);

            GtkWidget *ilabel = gtk_label_new (" ");
            gtk_widget_set_size_request (ilabel, BLANK_SIZE, -1);
            gtk_box_pack_start (GTK_BOX (hbox), ilabel, FALSE, FALSE, 0);
            gtk_widget_show (ilabel);

            GtkWidget *vbox_associate = gtk_vbox_new (FALSE, 0);
            gtk_box_pack_start (GTK_BOX (hbox), vbox_associate, TRUE, TRUE, 0);
            gtk_widget_show (vbox_associate);

            // New table items
            for (int i=0; i<SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
                _associate_table_items [i] = scim_string_view_new ();
                if (_default_font_desc)
                    gtk_widget_modify_font (_associate_table_items [i], _default_font_desc);
                gtk_widget_modify_base (_associate_table_items [i], GTK_STATE_NORMAL, &(base_normal));
                gtk_widget_modify_base (_associate_table_items [i], GTK_STATE_ACTIVE, &(base_active));
                gtk_widget_modify_text (_associate_table_items [i], GTK_STATE_NORMAL, &(text_normal));
                gtk_widget_modify_text (_associate_table_items [i], GTK_STATE_ACTIVE, &(text_active));
                scim_string_view_set_width_chars (SCIM_STRING_VIEW (_associate_table_items [i]), 80);
                scim_string_view_set_has_frame (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                scim_string_view_set_forward_event (SCIM_STRING_VIEW (_associate_table_items [i]), TRUE);
                scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_associate_table_items [i]), TRUE);
                scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_associate_table_items [i]), FALSE);
                g_signal_connect (G_OBJECT (_associate_table_items [i]), "button-release-event",
                                  G_CALLBACK (ui_associate_table_vertical_click_cb),
                                  GINT_TO_POINTER (i));
                gtk_box_pack_start (GTK_BOX (vbox_associate), _associate_table_items [i], FALSE, FALSE, 0);
            }
        } else {
            hbox = gtk_hbox_new (FALSE, 0);
            gtk_container_add (GTK_CONTAINER (associate_table_parent), hbox);
            gtk_widget_show (hbox);

            _associate_scroll = gtk_scrolled_window_new (NULL, NULL);
#if HAVE_GCONF
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_associate_scroll),
                                            GTK_POLICY_AUTOHIDE, GTK_POLICY_NEVER);
#else
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (_associate_scroll),
                                            GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
#endif
            gtk_box_pack_start (GTK_BOX (hbox), _associate_scroll, TRUE, TRUE, 0);
            gtk_widget_show (_associate_scroll);
            g_signal_connect (G_OBJECT (_associate_scroll), "button-press-event",
                              G_CALLBACK (candidate_scroll_button_press_cb),
                              GINT_TO_POINTER (2));
            g_signal_connect (G_OBJECT (_associate_scroll), "button-release-event",
                              G_CALLBACK (candidate_scroll_flick_cb),
                              GINT_TO_POINTER (2));
            g_signal_connect (G_OBJECT (_associate_scroll), "motion-notify-event",
                              G_CALLBACK (candidate_scroll_motion_cb),
                              GINT_TO_POINTER (2));
            g_signal_connect (G_OBJECT (_associate_scroll), "flick-event",
                              G_CALLBACK (candidate_scroll_flick_cb),
                              GINT_TO_POINTER (2));

            vbox = gtk_vbox_new (FALSE, 0);
            gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (_associate_scroll), vbox);
            gtk_widget_show (vbox);

            _associate_table_items [0] = scim_string_view_new ();
            if (_default_font_desc)
                gtk_widget_modify_font (_associate_table_items [0], _default_font_desc);
            gtk_widget_modify_base (_associate_table_items [0], GTK_STATE_NORMAL, &(base_normal));
            gtk_widget_modify_base (_associate_table_items [0], GTK_STATE_ACTIVE, &(base_active));
            gtk_widget_modify_text (_associate_table_items [0], GTK_STATE_NORMAL, &(text_normal));
            gtk_widget_modify_text (_associate_table_items [0], GTK_STATE_ACTIVE, &(text_active));
            gtk_widget_modify_text (_associate_table_items [0], GTK_STATE_INSENSITIVE, &(text_insensitive));
            scim_string_view_set_forward_event (SCIM_STRING_VIEW (_associate_table_items [0]), TRUE);
            scim_string_view_set_auto_resize (SCIM_STRING_VIEW (_associate_table_items [0]), TRUE);
            scim_string_view_set_press_move_cursor (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            scim_string_view_set_separator_index (SCIM_STRING_VIEW (_associate_table_items [0]), _associate_table_index);
            scim_string_view_set_has_frame (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            scim_string_view_set_draw_cursor (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            scim_string_view_set_auto_move_cursor (SCIM_STRING_VIEW (_associate_table_items [0]), FALSE);
            g_signal_connect (G_OBJECT (_associate_table_items [0]), "move_cursor",
                              G_CALLBACK (ui_associate_table_horizontal_click_cb),
                              0);

            gtk_box_pack_start (GTK_BOX (vbox), _associate_table_items [0], FALSE, FALSE, 0);
            gtk_widget_show (_associate_table_items [0]);

            // Resize associate height
            GtkRequisition scroll_size, lookup_size;
            gtk_widget_size_request (_associate_scroll, &scroll_size);
            if (scroll_size.height >= new_height) {
                gtk_widget_set_size_request (GTK_WIDGET (_associate_scroll), -1, scroll_size.height);
            } else {
                gtk_widget_size_request (_associate_table_items [0], &lookup_size);
                gtk_widget_set_size_request (GTK_WIDGET (_associate_scroll), -1, new_height);
                gtk_widget_set_size_request (GTK_WIDGET (_associate_table_items [0]), -1,
                                             lookup_size.height + (new_height - scroll_size.height));
            }
        }
    }

    // Load background and divider
    {
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (ISF_CANDIDATE_BG_FILE, NULL);
        if (pixbuf != NULL && _input_window != NULL) {
            gtk_widget_set_app_paintable (_input_window, TRUE);
            gtk_widget_realize (_input_window);
            GdkPixmap *pixmap = gdk_pixmap_new (_input_window->window, gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf), -1);
            if (pixmap != NULL) {
                gdk_draw_pixbuf (pixmap,
                                 _input_window->style->fg_gc[GTK_STATE_NORMAL],
                                 pixbuf,
                                 0, 0, 0, 0,
                                 gdk_pixbuf_get_width (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 GDK_RGB_DITHER_NORMAL, 0, 0);
                gdk_window_set_back_pixmap (_input_window->window, pixmap, FALSE);
            }
            g_object_unref (pixbuf);
        }

        pixbuf = gdk_pixbuf_new_from_file (ISF_CANDIDATE_DIVIDER_FILE, NULL);
        if (pixbuf != NULL && _input_window != NULL) {
            GdkPixmap *pixmap = gdk_pixmap_new (_input_window->window, gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height (pixbuf), -1);
            gdk_pixbuf_render_pixmap_and_mask (pixbuf, &pixmap, NULL, 0);
            if (pixmap != NULL) {
                scim_string_view_set_divider (SCIM_STRING_VIEW (_lookup_table_items [0]), pixmap);
                scim_string_view_set_divider (SCIM_STRING_VIEW (_associate_table_items [0]), pixmap);
            }
            g_object_unref (pixbuf);
        }
    }

    // Get split string according to screen width
    String str;
    GtkRequisition size;
    size.width = 0;
    _split_string1 = String ("");
    _split_string2 = String ("");
    do {
        _split_string1 = _split_string1 + String (" ");
        _split_string2 = _split_string2 + String (" ");
        str = _split_string2 + _split_string1;
        scim_string_view_set_text (SCIM_STRING_VIEW (_lookup_table_items [0]), str.c_str ());
        gtk_widget_size_request (_lookup_table_items [0], &size);
    } while (size.width < (15*_width_rate));

    // Get space string
    _space_string = _split_string1;
}

/**
 * @brief Create candidate window.
 *
 * @param style The candidate window style.
 * @param vertical An indicator for vertical window or horizontal window.
 */
static void create_candidate_window (ISF_CANDIDATE_STYLE_T style, bool vertical)
{
    check_time ("\nEnter create_candidate_window");
    SCIM_DEBUG_MAIN (1) << "Create candidate window...\n";

    if (_associate_table_window) {
        gtk_widget_destroy (_associate_table_window);
        _associate_table_window = 0;
    }
    if (_lookup_table_window) {
        gtk_widget_destroy (_lookup_table_window);
        _lookup_table_window = 0;
    }
    if (_input_window) {
        gtk_widget_destroy (_input_window);
        _input_window = 0;
    }

    if (style == PREDICTION_ENGINE_CANDIDATE_STYLE)
        create_prediction_engine_candidate_window (vertical);
    else
        create_scim_candidate_window (vertical);
/*
    if (_candidate_mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE) {
        gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
        gtk_widget_set_size_request (GTK_WIDGET (_input_window), gdk_screen_height (), -1);
    } else if (_candidate_mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
        gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
    }
*/
    check_time ("Exit create_candidate_window");
}

/**
 * @brief Update separator for candidate window.
 */
static void update_separator_status (void)
{
    if (_lookup_table_vertical) {
        if (GTK_WIDGET_VISIBLE (_aux_area)) {
            gtk_widget_show (_candidate_separator);
            gtk_widget_show (_associate_separator);
        } else {
            gtk_widget_hide (_candidate_separator);
            gtk_widget_hide (_associate_separator);
        }

        if (GTK_IS_WIDGET (_middle_separator)) {
            if (GTK_WIDGET_VISIBLE (_lookup_table_window) && GTK_WIDGET_VISIBLE (_associate_table_window))
                gtk_widget_show (_middle_separator);
            else
                gtk_widget_hide (_middle_separator);
        }
    } else {
        if (GTK_WIDGET_VISIBLE (_aux_area))
            gtk_widget_show (_candidate_separator);
        else
            gtk_widget_hide (_candidate_separator);

        if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
            if (GTK_WIDGET_VISIBLE (_aux_area) && !GTK_WIDGET_VISIBLE (_lookup_table_window))
                gtk_widget_show (_associate_separator);
            else
                gtk_widget_hide (_associate_separator);
        } else {
            if (GTK_WIDGET_VISIBLE (_aux_area) || GTK_WIDGET_VISIBLE (_lookup_table_window))
                gtk_widget_show (_associate_separator);
            else
                gtk_widget_hide (_associate_separator);
        }

        if (GTK_IS_WIDGET (_middle_separator)) {
            if (GTK_WIDGET_VISIBLE (_middle_separator))
                gtk_widget_hide (_middle_separator);
        }
    }
}

/**
 * @brief Set active ISE by uuid.
 *
 * @param uuid The ISE's uuid.
 *
 * @return false if ISE change is failed, otherwise return true.
 */
static bool set_active_ise_by_uuid (const String &ise_uuid, bool changeDefault)
{
    if (ise_uuid.length () <= 0)
        return false;

    GtkTreePath *path = 0;
    gchar       *uuid = 0;
    gchar       *arg1 = 0;
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_active_ise_list_store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store), &iter, ACTIVE_ISE_UUID, &uuid, -1);
            if (uuid != NULL && !strcmp (uuid, ise_uuid.c_str ())) {
                path = gtk_tree_model_get_path (GTK_TREE_MODEL (_active_ise_list_store), &iter);
                arg1 = gtk_tree_path_to_string (path);
                on_active_ise_enable_box_clicked (NULL, arg1, NULL);
                if (path) {
                    gtk_tree_path_free (path);
                    path = 0;
                }
                if (arg1) {
                    g_free (arg1);
                    arg1 = 0;
                }
                if (uuid) {
                    g_free (uuid);
                    uuid = 0;
                }
                return true;
            }
            if (uuid) {
                g_free (uuid);
                uuid = 0;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_active_ise_list_store), &iter));
    }

    return false;
}

/**
 * @brief Reload config callback function for ISF panel.
 *
 * @param config The config pointer.
 */
static void ui_config_reload_callback (const ConfigPointer &config)
{
    _config = config;
    ui_initialize ();
}

/**
 * @brief Load ISF configuration and ISEs information.
 */
static void ui_load_config (void)
{
    String str;
    bool   shared_ise = false;
    gchar  font_description_string[MAX_FONT_STR_SIZE];

    // Read configurations.
    gdk_color_parse ("gray92",     &_normal_bg);
    gdk_color_parse ("black",      &_normal_text);
    gdk_color_parse ("light blue", &_active_bg);
    gdk_color_parse ("black",      &_active_text);

    if (_default_font_desc) {
        pango_font_description_free (_default_font_desc);
        _default_font_desc = 0;
    }

    if (!_config.null ()) {
        str = _config->read (String (SCIM_CONFIG_PANEL_GTK_FONT), String ("default"));

        if (str != String ("default")) {
#if HAVE_GCONF
            snprintf (font_description_string, sizeof (font_description_string), "Vodafone Rg Bold %d", FONT_SIZE_OF_CANDIDATE_WIDGET);
#endif
        } else {
            snprintf (font_description_string, sizeof (font_description_string), "default");
        }
        _default_font_desc = pango_font_description_from_string (font_description_string);

        str = _config->read (String (SCIM_CONFIG_PANEL_GTK_COLOR_NORMAL_BG), String ("gray92"));
        gdk_color_parse (str.c_str (), &_normal_bg);

        str = _config->read (String (SCIM_CONFIG_PANEL_GTK_COLOR_NORMAL_TEXT), String ("black"));
        gdk_color_parse (str.c_str (), &_normal_text);

        str = _config->read (String (SCIM_CONFIG_PANEL_GTK_COLOR_ACTIVE_BG), String ("light blue"));
        gdk_color_parse (str.c_str (), &_active_bg);

        str = _config->read (String (SCIM_CONFIG_PANEL_GTK_COLOR_ACTIVE_TEXT), String ("black"));
        gdk_color_parse (str.c_str (), &_active_text);

        _window_sticked        = _config->read (String (SCIM_CONFIG_PANEL_GTK_DEFAULT_STICKED), _window_sticked);
        _candidate_style       = (ISF_CANDIDATE_STYLE_T)_config->read (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_STYLE), _candidate_style);
        _candidate_mode        = (ISF_CANDIDATE_MODE_T)_config->read (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_MODE), _candidate_mode);
        _lookup_table_vertical = _config->read (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_VERTICAL), _lookup_table_vertical);
        shared_ise             = _config->read (String (SCIM_CONFIG_FRONTEND_SHARED_INPUT_METHOD), shared_ise);
        _panel_agent->set_should_shared_ise (shared_ise);
    }
}

#ifdef GDK_WINDOWING_X11
/**
 * @brief GDK X event filter function.
 *
 * @param gdk_xevent The handler of GDK X event.
 * @param event The GdkEvent handler.
 * @param data The data pointer to pass when it is called.
 *
 * @return GDK_FILTER_CONTINUE.
 */
static GdkFilterReturn ui_event_filter (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    g_return_val_if_fail (gdk_xevent, GDK_FILTER_CONTINUE);

    XEvent *xev = (XEvent*)gdk_xevent;

    if (xev->type == PropertyNotify) {
        if (xev->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WORKAREA") ||
            xev->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_CURRENT_DESKTOP")) {
        }
    }

    return GDK_FILTER_CONTINUE;
}
#endif

/**
 * @brief UI initialize function.
 */
static void ui_initialize (void)
{
    SCIM_DEBUG_MAIN (1) << "Initialize UI...\n";

    ui_load_config ();

#if GDK_MULTIHEAD_SAFE
    // Initialize the Display and Screen.
    _current_screen  = gdk_screen_get_default ();
#endif

    isf_load_config ();
    create_and_fill_active_ise_list_store ();

#ifdef GDK_WINDOWING_X11
    // Add an event filter function to observe X root window's properties.
    GdkWindow *root_window = gdk_get_default_root_window ();
#if GDK_MULTIHEAD_SAFE
    if (_current_screen)
        root_window = gdk_screen_get_root_window (_current_screen);
#endif
    gdk_window_set_events (root_window, (GdkEventMask)GDK_PROPERTY_NOTIFY);
    gdk_window_add_filter (root_window, ui_event_filter, NULL);
#endif
}

/**
 * @brief Settle candidate window position.
 *
 * @param relative It indicates whether candidate position is relative.
 * @param force It indicates whether candidate position is forced to update.
 */
static void ui_settle_input_window (bool relative, bool force)
{
    SCIM_DEBUG_MAIN (2) << " Settle input window...\n";
    if (!_input_window)
        return;

    if (_window_sticked) {
        if (force)
            gtk_window_move (GTK_WINDOW (_input_window), _input_window_x, _input_window_y);
        return;
    }

    GtkRequisition ws;
    gint spot_x, spot_y;
    gint screen_width, screen_height;

    if (_candidate_mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE ||
        _candidate_mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
        screen_width  = ui_screen_height ();
        screen_height = ui_screen_width ();
    } else {
        screen_width  = ui_screen_width ();
        screen_height = ui_screen_height ();
    }

    gtk_widget_size_request (_input_window, &ws);

    if (_candidate_left >= 0 || _candidate_top >= 0) {
        spot_x = _candidate_left;
        spot_y = _candidate_top;
        if (spot_x + ws.width > screen_width)
            spot_x = screen_width - ws.width;
        if (spot_y + ws.height > screen_height)
            spot_y = screen_height - ws.height;
    } else {
        rectinfo ise_rect;
        _panel_agent->get_current_ise_geometry (ise_rect);
        //std::cout << " ise x : " << ise_rect.pos_x << " ise y : " << ise_rect.pos_y;
        //std::cout << " ise width : " << ise_rect.width << " ise height : " << ise_rect.height << "\n";

        if (!relative) {
            spot_x = _spot_location_x;
            spot_y = _spot_location_y;
        } else {
            spot_x = _input_window_x;
            spot_y = _input_window_y;
        }

        if (ise_rect.pos_y > 0 && spot_y + ws.height > (int)ise_rect.pos_y
            && spot_y < (int)(ise_rect.pos_y + ise_rect.height / 2))
            spot_y = _spot_location_top_y - ws.height;

        if (spot_x + ws.width > screen_width)
            spot_x = screen_width - ws.width;
        if (spot_y + ws.height > screen_height)
            spot_y = _spot_location_top_y - ws.height;
    }

    if (spot_x < 0) spot_x = 0;
    if (spot_y < 0) spot_y = 0;

    gtk_window_get_position (GTK_WINDOW (_input_window), &_input_window_x, &_input_window_y);

    if (spot_x != _input_window_x || spot_y != _input_window_y || force) {
        if (!_lookup_table_vertical)
            spot_x = 0;

        gtk_window_move (GTK_WINDOW (_input_window), spot_x, spot_y);

        _input_window_x = spot_x;
        _input_window_y = spot_y;
        //std::cout << " candidate x : " << spot_x << " candidate y : " << spot_y << "\n";
    }
}

/**
 * @brief Get screen width.
 *
 * @return screen width.
 */
static int ui_screen_width (void)
{
#if GDK_MULTIHEAD_SAFE
    if (_current_screen)
        return gdk_screen_get_width (_current_screen);
#endif
    return gdk_screen_width ();
}

/**
 * @brief Get screen height.
 *
 * @return screen height.
 */
static int ui_screen_height (void)
{
#if GDK_MULTIHEAD_SAFE
    if (_current_screen)
        return gdk_screen_get_height (_current_screen);
#endif
    return gdk_screen_height ();
}

#if GDK_MULTIHEAD_SAFE
/**
 * @brief Switch screen.
 *
 * @param screen The GdkScreen pointer.
 */
static void ui_switch_screen (GdkScreen *screen)
{
    if (screen) {
        if (_input_window) {
            gtk_window_set_screen (GTK_WINDOW (_input_window), screen);

            _input_window_x = ui_screen_width ();
            _input_window_y = ui_screen_height ();

            gtk_window_move (GTK_WINDOW (_input_window), _input_window_x, _input_window_y);
        }

        if (_toolbar_window) {
            gtk_window_set_screen (GTK_WINDOW (_toolbar_window), screen);
        }

#ifdef GDK_WINDOWING_X11
        GdkWindow *root_window = gdk_get_default_root_window ();
        if (_current_screen)
            root_window = gdk_screen_get_root_window (_current_screen);
        gdk_window_set_events (root_window, (GdkEventMask)GDK_PROPERTY_NOTIFY);
        gdk_window_add_filter (root_window, ui_event_filter, NULL);
#endif

        ui_settle_input_window ();
    }
}
#endif

/**
 * @brief Scale pixbuf.
 *
 * @param pixbuf The GdkPixbuf pointer.
 * @param width The dest width.
 * @param height The dest height.
 *
 * @return the pointer of dest GdkPixbuf.
 */
static GdkPixbuf * ui_scale_pixbuf (GdkPixbuf *pixbuf,
                                    int        width,
                                    int        height)
{
    if (pixbuf) {
        if (gdk_pixbuf_get_width (pixbuf) != width ||
            gdk_pixbuf_get_height (pixbuf) != height) {
            GdkPixbuf *dest = gdk_pixbuf_scale_simple (pixbuf, width, height, GDK_INTERP_BILINEAR);
            g_object_unref (pixbuf);
            pixbuf = dest;
        }
    }
    return pixbuf;
}

/**
 * @brief Create icon.
 *
 * @param iconfile The icon file path.
 * @param xpm The GdkPixbuf pointer of icon.
 * @param width The dest icon width.
 * @param height The dest icon height.
 * @param force_create It indicates whether create one icon when error occurs.
 *
 * @return the GtkWidget pointer of dest icon.
 */
static GtkWidget * ui_create_icon (const String  &iconfile,
                                   const char   **xpm,
                                   int            width,
                                   int            height,
                                   bool           force_create)
{
    String path = iconfile;
    GdkPixbuf *pixbuf = 0;

    if (path.length ()) {
        // Not a absolute path, prepend SCIM_ICONDIR
        if (path [0] != SCIM_PATH_DELIM)
            path = String (SCIM_ICONDIR) + String (SCIM_PATH_DELIM_STRING) + path;

        pixbuf = gdk_pixbuf_new_from_file (path.c_str (), 0);
    }

    if (!pixbuf && xpm) {
        pixbuf = gdk_pixbuf_new_from_xpm_data (xpm);
    }

    if (!pixbuf && force_create) {
        if (width <= 0 || height <= 0)
            return 0;

        pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, true, 8, width, height);

        if (!pixbuf)
            return 0;

        gdk_pixbuf_fill (pixbuf, 0);
    }

    if (pixbuf) {
        if (width <= 0) width = gdk_pixbuf_get_width (pixbuf);
        if (height <= 0) height = gdk_pixbuf_get_height (pixbuf);

        pixbuf = ui_scale_pixbuf (pixbuf, width, height);

        GtkWidget *icon = gtk_image_new_from_pixbuf (pixbuf);
        gtk_widget_show (icon);

        gdk_pixbuf_unref (pixbuf);

        return icon;
    }
    return 0;
}

/**
 * @brief Create up icon.
 *
 * @return the GtkWidget pointer of up icon.
 */
static GtkWidget * ui_create_up_icon (void)
{
    return ui_create_icon (SCIM_UP_ICON_FILE,
                           (const char **) up_xpm,
                           _lookup_icon_size,
                           _lookup_icon_size);
}

/**
 * @brief Create left icon.
 *
 * @return the GtkWidget pointer of left icon.
 */
static GtkWidget * ui_create_left_icon (void)
{
    return ui_create_icon (SCIM_LEFT_ICON_FILE,
                           (const char **) left_xpm,
                           _lookup_icon_size,
                           _lookup_icon_size);
}

/**
 * @brief Create right icon.
 *
 * @return the GtkWidget pointer of right icon.
 */
static GtkWidget * ui_create_right_icon (void)
{
    return ui_create_icon (SCIM_RIGHT_ICON_FILE,
                           (const char **) right_xpm,
                           _lookup_icon_size,
                           _lookup_icon_size);
}

/**
 * @brief Create down icon.
 *
 * @return the GtkWidget pointer of down icon.
 */
static GtkWidget * ui_create_down_icon (void)
{
    return ui_create_icon (SCIM_DOWN_ICON_FILE,
                           (const char **) down_xpm,
                           _lookup_icon_size,
                           _lookup_icon_size);
}

// Implementation of callback functions
/**
 * @brief Callback function for cursor move event of preedit area.
 *
 * @param view The ScimStringView pointer.
 * @param position The caret position.
 */
static void ui_preedit_area_move_cursor_cb (ScimStringView *view,
                                            guint           position)
{
    SCIM_DEBUG_MAIN (3) << "  ui_preedit_area_move_cursor_cb...\n";

    _panel_agent->move_preedit_caret (position);
}

/**
 * @brief Callback function for click event of vertical candidate table.
 *
 * @param item The GtkWidget handler.
 * @param event The GdkEventButton handler for this event.
 * @param user_data Data pointer to pass when it is called.
 *
 * @return true.
 */
static gboolean ui_lookup_table_vertical_click_cb (GtkWidget      *item,
                                                   GdkEventButton *event,
                                                   gpointer        user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_lookup_table_vertical_click_cb...\n";

    _panel_agent->select_candidate ((uint32)GPOINTER_TO_INT (user_data));

    return (gboolean)TRUE;
}

/**
 * @brief Callback function for click event of horizontal candidate table.
 *
 * @param item The GtkWidget handler of candidate string view.
 * @param position The click position.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_lookup_table_horizontal_click_cb (GtkWidget *item,
                                                 guint      position,
                                                 gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_lookup_table_horizontal_click_cb...\n";
    uint32 type = (uint32)GPOINTER_TO_INT (user_data);

    int *index = _lookup_table_index;
    int  pos   = (int) position - 1;
    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE && index [i] >= 0; ++i) {
        if (pos >= index [i] && pos < index [i+1]) {
            if (type == 0) { // button press event
                if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    //gtk_widget_style_get (_input_window, "theme-color", &_theme_color, NULL);
                    if (_theme_color != NULL) {
                        gtk_widget_modify_base (item, GTK_STATE_ACTIVE, _theme_color);
                        gdk_color_free (_theme_color);
                        _theme_color = NULL;
                    }
                }
                scim_string_view_set_highlight (SCIM_STRING_VIEW (item), index [i], index [i+1]);
            } else {         // button release event
                _panel_agent->select_candidate ((uint32) i);
                scim_string_view_set_highlight (SCIM_STRING_VIEW (item), -1, - 1);
            }

            return;
        }
    }
    scim_string_view_set_highlight (SCIM_STRING_VIEW (item), -1, - 1);
}

/**
 * @brief Callback function for up button of candidate table.
 *
 * @param button The GtkButton handler.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_lookup_table_up_button_click_cb (GtkButton *button,
                                                gpointer user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_lookup_table_up_button_click_cb...\n";

    _panel_agent->lookup_table_page_up ();
}

/**
 * @brief Callback function for down button of candidate table.
 *
 * @param button The GtkButton handler.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_lookup_table_down_button_click_cb (GtkButton *button,
                                                  gpointer user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_lookup_table_down_button_click_cb...\n";

    _panel_agent->lookup_table_page_down ();
}

/**
 * @brief Callback function for click event of vertical associate table.
 *
 * @param item The GtkWidget handler.
 * @param event The GdkEventButton handler for this event.
 * @param user_data Data pointer to pass when it is called.
 *
 * @return true.
 */
static gboolean ui_associate_table_vertical_click_cb (GtkWidget      *item,
                                                      GdkEventButton *event,
                                                      gpointer        user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_associate_table_vertical_click_cb...\n";

    _panel_agent->select_associate ((uint32)GPOINTER_TO_INT (user_data));

    return (gboolean)TRUE;
}

/**
 * @brief Callback function for click event of horizontal associate table.
 *
 * @param item The GtkWidget handler of candidate string view.
 * @param position The click position.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_associate_table_horizontal_click_cb (GtkWidget *item,
                                                    guint      position,
                                                    gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_associate_table_horizontal_click_cb...\n";
    uint32 type = (uint32)GPOINTER_TO_INT (user_data);

    int *index = _associate_table_index;
    int  pos   = (int) position - 1;

    for (int i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE && index [i] >= 0; ++i) {
        if (pos >= index [i] && pos < index [i+1]) {
            if (type == 0) { // button press event
                if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    //gtk_widget_style_get (_input_window, "theme-color", &_theme_color, NULL);
                    if (_theme_color != NULL) {
                        gtk_widget_modify_base (item, GTK_STATE_ACTIVE, _theme_color);
                        gdk_color_free (_theme_color);
                        _theme_color = NULL;
                    }
                }
                scim_string_view_set_highlight (SCIM_STRING_VIEW (item), index [i], index [i+1]);
            } else {         // button release event
                _panel_agent->select_associate ((uint32) i);
                scim_string_view_set_highlight (SCIM_STRING_VIEW (item), -1, - 1);
            }

            return;
        }
    }
    scim_string_view_set_highlight (SCIM_STRING_VIEW (item), -1, - 1);
}

/**
 * @brief Callback function for up button of associate table.
 *
 * @param button The GtkButton handler.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_associate_table_up_button_click_cb (GtkButton *button,
                                                   gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_associate_table_up_button_click_cb...\n";

    _panel_agent->associate_table_page_up ();
}

/**
 * @brief Callback function for down button of associate table.
 *
 * @param button The GtkButton handler.
 * @param user_data Data pointer to pass when it is called.
 */
static void ui_associate_table_down_button_click_cb (GtkButton *button,
                                                     gpointer   user_data)
{
    SCIM_DEBUG_MAIN (3) << "  ui_associate_table_down_button_click_cb...\n";

    _panel_agent->associate_table_page_down ();
}

/**
 * @brief Callback function for motion event of candidate window.
 *
 * @param window The GtkWidget handler.
 * @param event The GdkEventMotion handler.
 * @param user_data Data pointer to pass when it is called.
 *
 * @return FALSE.
 */
static gboolean ui_input_window_motion_cb (GtkWidget      *window,
                                           GdkEventMotion *event,
                                           gpointer        user_data)
{
    return FALSE;
}

/**
 * @brief Callback function for click event of candidate window.
 *
 * @param window The GtkWidget handler.
 * @param event The GdkEventMotion handler.
 * @param user_data Data pointer to pass when it is called.
 *
 * @return TRUE if this operation is successful, otherwise return FALSE.
 */
static gboolean ui_input_window_click_cb (GtkWidget *window,
                                          GdkEventButton *event,
                                          gpointer user_data)
{
    int click_type = GPOINTER_TO_INT (user_data);
    static gulong motion_handler;

    if (click_type == 0) {
        if (_input_window_draging)
            return (gboolean)FALSE;

        // Connection pointer motion handler to this window.
        motion_handler = g_signal_connect (G_OBJECT (window), "motion-notify-event",
                                           G_CALLBACK (ui_input_window_motion_cb),
                                           NULL);

        _input_window_draging = TRUE;
        _input_window_drag_x = (gint) event->x_root;
        _input_window_drag_y = (gint) event->y_root;
#ifdef ENABLE_CHANGE_CURSOR
        cursor = gdk_cursor_new (GDK_TOP_LEFT_ARROW);

        // Grab the cursor to prevent losing events.
        gdk_pointer_grab (window->window, TRUE,
                          (GdkEventMask) (GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK),
                          NULL, cursor, event->time);
        gdk_cursor_unref (cursor);
#endif
        return (gboolean)TRUE;
    } else if (click_type == 1) {
        if (!_input_window_draging)
            return (gboolean)FALSE;

        g_signal_handler_disconnect (G_OBJECT (window), motion_handler);
        gdk_pointer_ungrab (event->time);
        _input_window_draging = FALSE;

        gtk_window_get_position (GTK_WINDOW (window), &_input_window_x, &_input_window_y);

        return (gboolean)TRUE;
    }

    return (gboolean)FALSE;
}

/**
 * @brief This function is used to judge whether candidate window should be hidden.
 *
 * @return true if candidate window should be hidden, otherwise return false.
 */
static bool ui_can_hide_input_window (void)
{
    if (!_panel_is_on)
        return true;

    if (GTK_WIDGET_VISIBLE (_preedit_area) ||
        GTK_WIDGET_VISIBLE (_aux_area) ||
        GTK_WIDGET_VISIBLE (_associate_table_window) ||
        GTK_WIDGET_VISIBLE (_lookup_table_window))
        return false;
    return true;
}

/**
 * @brief Create pango attribute list.
 *
 * @param mbs The source string.
 * @param attrs The attribute list.
 *
 * @return the pointer of PangoAttrList.
 */
static PangoAttrList * create_pango_attrlist (const String        &mbs,
                                              const AttributeList &attrs)
{
    PangoAttrList  *attrlist = pango_attr_list_new ();
    PangoAttribute *attr = NULL;

    guint start_index, end_index;
    guint wlen = g_utf8_strlen (mbs.c_str (), mbs.length ());

    for (int i=0; i < (int) attrs.size (); ++i) {
        start_index = attrs[i].get_start ();
        end_index = attrs[i].get_end ();

        if (end_index <= wlen && start_index < end_index) {
            start_index = g_utf8_offset_to_pointer (mbs.c_str (), attrs[i].get_start ()) - mbs.c_str ();
            end_index = g_utf8_offset_to_pointer (mbs.c_str (), attrs[i].get_end ()) - mbs.c_str ();

            if (attrs[i].get_type () == SCIM_ATTR_DECORATE) {
                if (attrs[i].get_value () == SCIM_ATTR_DECORATE_UNDERLINE) {
                    attr = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
                    if (attr != NULL) {
                        attr->start_index = start_index;
                        attr->end_index = end_index;
                        pango_attr_list_insert (attrlist, attr);
                    }
                } else if (attrs[i].get_value () == SCIM_ATTR_DECORATE_REVERSE) {
                    attr = pango_attr_foreground_new (_normal_bg.red, _normal_bg.green, _normal_bg.blue);
                    if (attr != NULL) {
                        attr->start_index = start_index;
                        attr->end_index = end_index;
                        pango_attr_list_insert (attrlist, attr);
                    }

                    attr = pango_attr_background_new (_normal_text.red, _normal_text.green, _normal_text.blue);
                    if (attr != NULL) {
                        attr->start_index = start_index;
                        attr->end_index = end_index;
                        pango_attr_list_insert (attrlist, attr);
                    }
                } else if (attrs[i].get_value () == SCIM_ATTR_DECORATE_HIGHLIGHT) {
                    attr = pango_attr_foreground_new (_active_text.red, _active_text.green, _active_text.blue);
                    if (attr != NULL) {
                        attr->start_index = start_index;
                        attr->end_index = end_index;
                        pango_attr_list_insert (attrlist, attr);
                    }

                    attr = pango_attr_background_new (_active_bg.red, _active_bg.green, _active_bg.blue);
                    if (attr != NULL) {
                        attr->start_index = start_index;
                        attr->end_index = end_index;
                        pango_attr_list_insert (attrlist, attr);
                    }
                }
            } else if (attrs[i].get_type () == SCIM_ATTR_FOREGROUND) {
                unsigned int color = attrs[i].get_value ();

                attr = pango_attr_foreground_new (SCIM_RGB_COLOR_RED(color) * 256, SCIM_RGB_COLOR_GREEN(color) * 256, SCIM_RGB_COLOR_BLUE(color) * 256);
                if (attr != NULL) {
                    attr->start_index = start_index;
                    attr->end_index = end_index;
                    pango_attr_list_insert (attrlist, attr);
                }
            } else if (attrs[i].get_type () == SCIM_ATTR_BACKGROUND) {
                unsigned int color = attrs[i].get_value ();

                attr = pango_attr_background_new (SCIM_RGB_COLOR_RED(color) * 256, SCIM_RGB_COLOR_GREEN(color) * 256, SCIM_RGB_COLOR_BLUE(color) * 256);
                if (attr != NULL) {
                    attr->start_index = start_index;
                    attr->end_index = end_index;
                    pango_attr_list_insert (attrlist, attr);
                }
            }
        }
    }
    return attrlist;
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
    _panel_agent = new PanelAgent ();

    if (!_panel_agent || !_panel_agent->initialize (config, display, resident))
        return false;

    _panel_agent->signal_connect_transaction_start          (slot (slot_transaction_start));
    _panel_agent->signal_connect_transaction_end            (slot (slot_transaction_end));
    _panel_agent->signal_connect_reload_config              (slot (slot_reload_config));
    _panel_agent->signal_connect_turn_on                    (slot (slot_turn_on));
    _panel_agent->signal_connect_turn_off                   (slot (slot_turn_off));
    _panel_agent->signal_connect_focus_in                   (slot (slot_focus_in));
    _panel_agent->signal_connect_focus_out                  (slot (slot_focus_out));
    _panel_agent->signal_connect_show_panel                 (slot (slot_show_panel));
    _panel_agent->signal_connect_hide_panel                 (slot (slot_hide_panel));
    _panel_agent->signal_connect_update_screen              (slot (slot_update_screen));
    _panel_agent->signal_connect_update_spot_location       (slot (slot_update_spot_location));
    _panel_agent->signal_connect_update_factory_info        (slot (slot_update_factory_info));
    _panel_agent->signal_connect_start_default_ise          (slot (slot_start_default_ise));
    _panel_agent->signal_connect_set_candidate_ui           (slot (slot_set_candidate_ui));
    _panel_agent->signal_connect_get_candidate_ui           (slot (slot_get_candidate_ui));
    _panel_agent->signal_connect_set_candidate_position     (slot (slot_set_candidate_position));
    _panel_agent->signal_connect_get_candidate_geometry     (slot (slot_get_candidate_geometry));
    _panel_agent->signal_connect_set_keyboard_ise           (slot (slot_set_keyboard_ise));
    _panel_agent->signal_connect_get_keyboard_ise           (slot (slot_get_keyboard_ise));
    _panel_agent->signal_connect_show_preedit_string        (slot (slot_show_preedit_string));
    _panel_agent->signal_connect_show_aux_string            (slot (slot_show_aux_string));
    _panel_agent->signal_connect_show_lookup_table          (slot (slot_show_lookup_table));
    _panel_agent->signal_connect_show_associate_table       (slot (slot_show_associate_table));
    _panel_agent->signal_connect_hide_preedit_string        (slot (slot_hide_preedit_string));
    _panel_agent->signal_connect_hide_aux_string            (slot (slot_hide_aux_string));
    _panel_agent->signal_connect_hide_lookup_table          (slot (slot_hide_lookup_table));
    _panel_agent->signal_connect_hide_associate_table       (slot (slot_hide_associate_table));
    _panel_agent->signal_connect_update_preedit_string      (slot (slot_update_preedit_string));
    _panel_agent->signal_connect_update_preedit_caret       (slot (slot_update_preedit_caret));
    _panel_agent->signal_connect_update_aux_string          (slot (slot_update_aux_string));
    _panel_agent->signal_connect_update_lookup_table        (slot (slot_update_candidate_table));
    _panel_agent->signal_connect_update_associate_table     (slot (slot_update_associate_table));
    _panel_agent->signal_connect_set_active_ise_by_uuid     (slot (slot_set_active_ise_by_uuid));
    _panel_agent->signal_connect_get_ise_list               (slot (slot_get_ise_list));
    _panel_agent->signal_connect_get_keyboard_ise_list      (slot (slot_get_keyboard_ise_list));
    _panel_agent->signal_connect_get_language_list          (slot (slot_get_language_list));
    _panel_agent->signal_connect_get_all_language           (slot (slot_get_all_language));
    _panel_agent->signal_connect_get_ise_language           (slot (slot_get_ise_language));
    _panel_agent->signal_connect_set_isf_language           (slot (slot_set_isf_language));
    _panel_agent->signal_connect_get_ise_info_by_uuid       (slot (slot_get_ise_info_by_uuid));
    _panel_agent->signal_connect_send_key_event             (slot (slot_send_key_event));
    _panel_agent->signal_connect_lock                       (slot (slot_lock));
    _panel_agent->signal_connect_unlock                     (slot (slot_unlock));

    _panel_agent->get_helper_list (_helper_list);
    _panel_agent->get_active_ise_list (_load_ise_list);

    return true;
}

/**
 * @brief Run PanelAgent thread.
 *
 * @return ture if run PanelAgent thread is successful, otherwise return false.
 */
static bool run_panel_agent (void)
{
    SCIM_DEBUG_MAIN(1) << "run_panel_agent ()\n";

    _panel_agent_thread = NULL;

    if (_panel_agent && _panel_agent->valid ())
        _panel_agent_thread = g_thread_create (panel_agent_thread_func, NULL, TRUE, NULL);

    return (_panel_agent_thread != NULL);
}

/**
 * @brief PanelAgent thread function.
 *
 * @param data Data to pass when it is called.
 *
 * @return NULL.
 */
static gpointer panel_agent_thread_func (gpointer data)
{
    SCIM_DEBUG_MAIN(1) << "panel_agent_thread_func ()\n";

    if (!_panel_agent->run ())
        std::cerr << "Failed to run Panel.\n";

    G_LOCK (_global_resource_lock);
    _should_exit = true;
    G_UNLOCK (_global_resource_lock);

    g_thread_exit (NULL);
    return ((gpointer) NULL);
}

/**
 * @brief Start transaction slot function for PanelAgent.
 */
static void slot_transaction_start (void)
{
    gdk_threads_enter ();
}

/**
 * @brief End transaction slot function for PanelAgent.
 */
static void slot_transaction_end (void)
{
    gdk_threads_leave ();
}

/**
 * @brief Reload config slot function for PanelAgent.
 */
static void slot_reload_config (void)
{
    if (!_config.null ()) _config->reload ();
}

/**
 * @brief Turn on slot function for PanelAgent.
 */
static void slot_turn_on (void)
{
    _panel_is_on = true;

    if (_input_window) {
        gtk_widget_hide (_input_window);
        gtk_widget_hide (_lookup_table_window);
        gtk_widget_hide (_associate_table_window);
        gtk_widget_hide (_preedit_area);
        gtk_widget_hide (_aux_area);
    }
}

/**
 * @brief Turn off slot function for PanelAgent.
 */
static void slot_turn_off (void)
{
    _panel_is_on = false;

    if (_input_window) {
        gtk_widget_hide (_input_window);
        gtk_widget_hide (_lookup_table_window);
        gtk_widget_hide (_associate_table_window);
        gtk_widget_hide (_preedit_area);
        gtk_widget_hide (_aux_area);
    }
}

/**
 * @brief Focus in slot function for PanelAgent.
 */
static void slot_focus_in (void)
{
    if (!GTK_IS_WIDGET (_input_window))
        create_candidate_window (_candidate_style, _lookup_table_vertical);
}

/**
 * @brief Focus out slot function for PanelAgent.
 */
static void slot_focus_out (void)
{
}

/**
 * @brief Show control panel slot function for PanelAgent.
 */
static void slot_show_panel (void)
{
    if (!_toolbar_window)
        create_panel_window ();

    gtk_widget_show (_toolbar_window);
    gtk_window_present (GTK_WINDOW (_toolbar_window));
    gdk_window_raise (_toolbar_window->window);

    GdkDrawable * pixmap = (GdkDrawable *) gdk_pixmap_new (_toolbar_window->window,
                                                           _panel_width, _panel_height, -1);

    gdk_pixbuf_render_pixmap_and_mask_for_colormap (_panel_bg_pixbuf,
                                                    gdk_drawable_get_colormap (pixmap),
                                                    &pixmap,
                                                    &_panel_window_mask,
                                                    128);
    gtk_widget_shape_combine_mask (_toolbar_window, _panel_window_mask, 0, 0);

    _panel_agent->update_isf_control_status (true);
}

/**
 * @brief Hide control panel slot function for PanelAgent.
 */
static void slot_hide_panel (void)
{
    if (_toolbar_window) {
        gtk_widget_hide (_toolbar_window);
    }

    _panel_agent->update_isf_control_status (false);
}

/**
 * @brief Update screen slot function for PanelAgent.
 *
 * @param num The screen number.
 */
static void slot_update_screen (int num)
{
#if GDK_MULTIHEAD_SAFE
    if (gdk_display_get_n_screens (gdk_display_get_default ()) > num) {

        GdkScreen *screen = gdk_display_get_screen (gdk_display_get_default (), num);

        if (screen) {
#ifdef GDK_WINDOWING_X11
            GdkWindow *root_window = gdk_get_default_root_window ();
            if (_current_screen)
                root_window = gdk_screen_get_root_window (_current_screen);
            gdk_window_remove_filter (root_window, ui_event_filter, NULL);
#endif

            _current_screen = screen;
            ui_switch_screen (screen);
        }
    }
#endif
}

/**
 * @brief Update keyboard ISE information slot function for PanelAgent.
 *
 * @param info The information of current Keyboard ISE.
 */
static void slot_update_factory_info (const PanelFactoryInfo &info)
{
    String ise_name = info.name;
    String ise_icon = info.icon;

    TOOLBAR_MODE_T mode = _panel_agent->get_current_toolbar_mode ();

    if (TOOLBAR_HELPER_MODE == mode)
        ise_name = _panel_agent->get_current_helper_name ();

    if (ise_name.length () > 0) {
        _panel_agent->set_current_ise_name (ise_name);

        gchar *name = 0, *icon_file = 0;
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_active_ise_list_store), &iter)) {
            do {
                gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store), &iter, ACTIVE_ISE_NAME, &name, -1);

                if (name == (void *)0)
                    continue;

                if (!strcmp (name, ise_name.c_str ())) {
                    gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store), &iter, ACTIVE_ISE_ICON, &icon_file, -1);
                    ise_icon = String (icon_file);
                    gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                        ACTIVE_ISE_LIST_ENABLE, TRUE, -1);
                } else {
                    gtk_list_store_set (GTK_LIST_STORE (_active_ise_list_store), &iter,
                                        ACTIVE_ISE_LIST_ENABLE, FALSE, -1);
                }
                if (name) {
                    g_free (name);
                    name = 0;
                }
                if (icon_file) {
                    g_free (icon_file);
                    icon_file = 0;
                }
            } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_active_ise_list_store), &iter));
        }
    }
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
    if (x >= 0 && x < ui_screen_width () && y >= 0 && y < ui_screen_height ()) {
        _spot_location_x = x;
        _spot_location_y = y;
        _spot_location_top_y = top_y;

        ui_settle_input_window ();
    }
}

/**
 * @brief Show preedit slot function for PanelAgent.
 */
static void slot_show_preedit_string (void)
{
    if (!_preedit_area)
        return;

    gtk_widget_show (_preedit_area);

    if (_panel_is_on && !GTK_WIDGET_VISIBLE (_input_window)) {
        gtk_widget_show (_input_window);
        gdk_window_raise (_input_window->window);
    }

    ui_settle_input_window (false, true);
    update_separator_status ();
}

/**
 * @brief Show aux slot function for PanelAgent.
 */
static void slot_show_aux_string (void)
{
    if (!_aux_area)
        return;

    gtk_widget_show (_aux_area);

    if (_panel_is_on && !GTK_WIDGET_VISIBLE (_input_window)) {
        gtk_widget_show (_input_window);
        gdk_window_raise (_input_window->window);
    }

    ui_settle_input_window (false, true);
    update_separator_status ();
}

/**
 * @brief Show candidate table slot function for PanelAgent.
 */
static void slot_show_lookup_table (void)
{
    if (!_lookup_table_window)
        return;

    gtk_widget_show (_lookup_table_window);
    _candidate_is_showed = true;

    if (_panel_is_on) {
        if (!GTK_WIDGET_VISIBLE (_input_window)) {
            gtk_widget_show (_input_window);
            gdk_window_raise (_input_window->window);// Move the window up in case it is not at the top
            gtk_window_present (GTK_WINDOW (_input_window));
        }
        //ui_settle_input_window (false, true);
    }

    update_separator_status ();
}

/**
 * @brief Show associate table slot function for PanelAgent.
 */
static void slot_show_associate_table (void)
{
    if (!_associate_table_window)
        return;

    gtk_widget_show (_associate_table_window);

    if (_panel_is_on) {
        if (!GTK_WIDGET_VISIBLE (_input_window)) {
            gtk_widget_show (_input_window);
            gdk_window_raise (_input_window->window);
        }
        ui_settle_input_window (false, true);
    }

    update_separator_status ();
}

/**
 * @brief Hide preedit slot function for PanelAgent.
 */
static void slot_hide_preedit_string (void)
{
    SCIM_DEBUG_MAIN (1) << "slot_hide_preedit_string\n";
    if (!_preedit_area)
        return;

    gtk_widget_hide (_preedit_area);
    scim_string_view_set_text (SCIM_STRING_VIEW (_preedit_area), "");

    if (ui_can_hide_input_window ())
        gtk_widget_hide (_input_window);
}

/**
 * @brief Hide aux slot function for PanelAgent.
 */
static void slot_hide_aux_string (void)
{
    if (!_aux_area)
        return;

    gtk_widget_hide (_aux_area);
    scim_string_view_set_text (SCIM_STRING_VIEW (_aux_area), "");

    if (ui_can_hide_input_window ())
        gtk_widget_hide (_input_window);

    update_separator_status ();
}

/**
 * @brief Hide candidate table slot function for PanelAgent.
 */
static void slot_hide_lookup_table (void)
{
    if (!_lookup_table_window)
        return;

    slot_lock ();
    g_isf_candidate_table.clear ();
    if (_candidate_timer) {
        g_source_remove (_candidate_timer);
        _candidate_timer = 0;
    }
    slot_unlock ();

    gtk_widget_hide (_lookup_table_window);
    _candidate_is_showed = false;

    if (ui_can_hide_input_window ())
        gtk_widget_hide (_input_window);

    update_separator_status ();
}

/**
 * @brief Hide associate table slot function for PanelAgent.
 */
static void slot_hide_associate_table (void)
{
    if (!_associate_table_window)
        return;

    gtk_widget_hide (_associate_table_window);

    if (ui_can_hide_input_window ())
        gtk_widget_hide (_input_window);

    update_separator_status ();
}

/**
 * @brief Update preedit slot function for PanelAgent.
 *
 * @param str The new preedit string.
 * @param attrs The attribute list of new preedit string.
 */
static void slot_update_preedit_string (const String &str, const AttributeList &attrs)
{
    if (!GTK_IS_WIDGET (_preedit_area))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    PangoAttrList  *attrlist = create_pango_attrlist (str, attrs);

    scim_string_view_set_attributes (SCIM_STRING_VIEW (_preedit_area), attrlist);
    scim_string_view_set_text (SCIM_STRING_VIEW (_preedit_area), str.c_str ());

    pango_attr_list_unref (attrlist);

    ui_settle_input_window (false);
}

/**
 * @brief Update caret slot function for PanelAgent.
 *
 * @param caret The caret position.
 */
static void slot_update_preedit_caret (int caret)
{
    if (!GTK_IS_WIDGET (_preedit_area))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    scim_string_view_set_position (SCIM_STRING_VIEW (_preedit_area), caret);
}

/**
 * @brief Update aux slot function for PanelAgent.
 *
 * @param str The new aux string.
 * @param attrs The attribute list of new aux string.
 */
static void slot_update_aux_string (const String &str, const AttributeList &attrs)
{
    if (!GTK_IS_WIDGET (_aux_area))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    PangoAttrList  *attrlist = create_pango_attrlist (str, attrs);

    String strAux = String ("  ") + str;
    scim_string_view_set_attributes (SCIM_STRING_VIEW (_aux_area), attrlist);
    scim_string_view_set_text (SCIM_STRING_VIEW (_aux_area), strAux.c_str ());

    pango_attr_list_unref (attrlist);

    ui_settle_input_window (false);
}

/**
 * @brief Update candidate/associate table.
 *
 * @param table_type The table type.
 * @param table The lookup table for candidate or associate.
 * @param table_items The table items for candidate or associate.
 * @param table_index The index for candidate string or associate string.
 * @param table_index_pos The index position for candidate or associate.
 * @param scroll The candidate scroller or associate scroller.
 */
static void update_table (const int table_type, const LookupTable &table,
                          GtkWidget *table_items[], int table_index[], int table_index_pos[],
                          GtkWidget *scroll)
{
    int i;
    int item_num = table.get_current_page_size ();

    String          mbs, tmp_mbs;
    WideString      wcs;
    WideString      label;
    GtkRequisition  size = {0, 0};
    AttributeList   attrs;
    PangoAttrList  *attrlist = NULL;
    int             max_width = 0;

    if (_lookup_table_vertical) {
        int show_line = VERTICAL_SHOW_LINE;

        if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE)
            gtk_range_set_value (GTK_RANGE ((GTK_SCROLLED_WINDOW (scroll))->vscrollbar), 0);
        for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++ i) {
            if (i < item_num) {
                mbs = String ();

                wcs = table.get_candidate_in_current_page (i);

                if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    label = utf8_mbstowcs ("");
                } else {
                    label = table.get_candidate_label (i);

                    if (label.length ()) {
                        label += utf8_mbstowcs (".");
                    }
                }

                mbs = utf8_wcstombs (label+wcs);

                scim_string_view_set_text (SCIM_STRING_VIEW (table_items [i]), mbs.c_str ());

                gtk_widget_size_request (table_items [i], &size);
                max_width = max_width > size.width ? max_width : size.width;

                if ((size.height * (i + 1)) > (gdk_screen_height () / 3))
                    show_line = show_line < i + 1 ? show_line : i + 1;

                if (_candidate_style != PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    if ((i + 1) == show_line)
                        item_num = i + 1;
                }

                // Update attributes
                attrs = table.get_attributes_in_current_page (i);

                if (attrs.size ()) {
                    for (AttributeList::iterator ait = attrs.begin (); ait != attrs.end (); ++ait)
                        ait->set_start (ait->get_start () + label.length ());

                    attrlist = create_pango_attrlist (mbs, attrs);
                    scim_string_view_set_attributes (SCIM_STRING_VIEW (table_items [i]), attrlist);
                    pango_attr_list_unref (attrlist);
                } else {
                    scim_string_view_set_attributes (SCIM_STRING_VIEW (table_items [i]), 0);
                }

                gtk_widget_show (table_items [i]);
            } else {
                gtk_widget_hide (table_items [i]);
            }
        }
        if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
            if (item_num < show_line)
                gtk_widget_set_size_request (scroll, max_width + size.height*2/3 + BLANK_SIZE, item_num*size.height);
            else
                gtk_widget_set_size_request (scroll, max_width + size.height*2/3 + BLANK_SIZE, show_line*size.height);
        }
    } else {
        table_index [0]     = 0;
        table_index_pos [0] = 0;
        if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE)
            gtk_range_set_value (GTK_RANGE ((GTK_SCROLLED_WINDOW (scroll))->hscrollbar), 0);
        for (i = 0; i < SCIM_LOOKUP_TABLE_MAX_PAGESIZE; ++i) {
            if (i < item_num) {
                // Update attributes
                AttributeList item_attrs = table.get_attributes_in_current_page (i);
                size_t attr_start, attr_end;

                if (_candidate_style == PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    if (i > 0)
                        label = utf8_mbstowcs (_split_string1);
                    else
                        label = utf8_mbstowcs (_space_string);
                } else {
                    label = table.get_candidate_label (i);

                    if (label.length ()) {
                        label += utf8_mbstowcs (".");
                    } else {
                        if (i > 0)
                            label = utf8_mbstowcs ("  ");
                    }
                }
                wcs += label;

                attr_start = wcs.length ();

                wcs += table.get_candidate_in_current_page (i);

                attr_end = wcs.length ();

                wcs = wcs + utf8_mbstowcs (_split_string2);

                table_index [i+1] = wcs.length ();

                tmp_mbs = mbs;
                mbs     = utf8_wcstombs (wcs);

                scim_string_view_set_text (SCIM_STRING_VIEW (table_items [0]), mbs.c_str ());

                gtk_widget_size_request (table_items [0], &size);
                table_index_pos [i+1] = size.width;
                if (_candidate_style != PREDICTION_ENGINE_CANDIDATE_STYLE) {
                    if (i > 0  && size.width > (ui_screen_width () - 2*_lookup_icon_size - 2) && !table.is_page_size_fixed ()) {
                        item_num = i;
                        mbs      = tmp_mbs;
                        scim_string_view_set_text (SCIM_STRING_VIEW (table_items [0]), mbs.c_str ());
                    }
                }

                if (item_attrs.size ()) {
                    for (AttributeList::iterator ait = item_attrs.begin (); ait != item_attrs.end (); ++ait) {
                        ait->set_start (ait->get_start () + attr_start);
                        if (ait->get_end () + attr_start > attr_end)
                            ait->set_length (attr_end - ait->get_start ());
                    }

                    attrs.insert (attrs.end (), item_attrs.begin (), item_attrs.end ());
                }

            } else {
                table_index [i+1]     = -1;
                table_index_pos [i+1] = -1;
            }
        }

        if (attrs.size ()) {
            attrlist = create_pango_attrlist (mbs, attrs);
            scim_string_view_set_attributes (SCIM_STRING_VIEW (table_items [0]), attrlist);
            pango_attr_list_unref (attrlist);
        } else {
            scim_string_view_set_attributes (SCIM_STRING_VIEW (table_items [0]), 0);
        }
    }

    int nCandidateCount = table.number_of_candidates ();
    if (table_type == CANDIDATE_TABLE) {
        if (_candidate_style == SCIM_CANDIDATE_STYLE) {
            if (table.get_current_page_start ())
                gtk_widget_set_sensitive (_lookup_table_up_button, TRUE);
            else
                gtk_widget_set_sensitive (_lookup_table_up_button, FALSE);

            if (table.get_current_page_start () + item_num < nCandidateCount)
                gtk_widget_set_sensitive (_lookup_table_down_button, TRUE);
            else
                gtk_widget_set_sensitive (_lookup_table_down_button, FALSE);

            if (item_num < table.get_current_page_size ())
                _panel_agent->update_lookup_table_page_size (item_num);
        }

        if (SCIM_STRING_VIEW (_lookup_table_items [0])->highlight_start != -1 ||
                SCIM_STRING_VIEW (_lookup_table_items [0])->highlight_end != -1) {
            scim_string_view_set_highlight (SCIM_STRING_VIEW (_lookup_table_items [0]), -1, -1);
        }
    } else if (table_type == ASSOCIATE_TABLE) {
        if (_candidate_style == SCIM_CANDIDATE_STYLE) {
            if (table.get_current_page_start ())
                gtk_widget_set_sensitive (_associate_table_up_button, TRUE);
            else
                gtk_widget_set_sensitive (_associate_table_up_button, FALSE);

            if (table.get_current_page_start () + item_num < nCandidateCount)
                gtk_widget_set_sensitive (_associate_table_down_button, TRUE);
            else
                gtk_widget_set_sensitive (_associate_table_down_button, FALSE);

            if (item_num < table.get_current_page_size ())
                _panel_agent->update_associate_table_page_size (item_num);
        }
        if (SCIM_STRING_VIEW (_associate_table_items [0])->highlight_start != -1 ||
            SCIM_STRING_VIEW (_associate_table_items [0])->highlight_end != -1) {
            scim_string_view_set_highlight (SCIM_STRING_VIEW (_associate_table_items [0]), -1, -1);
        }
    }

    //ui_settle_input_window ();
}

/**
 * @brief Update candidate table slot function for PanelAgent.
 *
 * @param table The lookup table for candidate.
 */
static void slot_update_candidate_table (const LookupTable &table)
{
    if (!GTK_IS_WIDGET (_lookup_table_window))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    //update_table (CANDIDATE_TABLE, table, _lookup_table_items,
    //              _lookup_table_index, _lookup_table_index_pos, _candidate_scroll);
    slot_lock ();
    if (_candidate_timer) {
        g_source_remove (_candidate_timer);
        _candidate_timer = 0;
    }
    _candidate_timer = g_timeout_add (200, candidate_show_timeout_cb, GINT_TO_POINTER (CANDIDATE_TABLE));
    slot_unlock ();
}

/**
 * @brief Update associate table slot function for PanelAgent.
 *
 * @param table The lookup table for associate.
 */
static void slot_update_associate_table (const LookupTable &table)
{
    if (!GTK_IS_WIDGET (_associate_table_window))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    update_table (ASSOCIATE_TABLE, table, _associate_table_items,
                  _associate_table_index, _associate_table_index_pos, _associate_scroll);
}

/**
 * @brief Set active ISE slot function for PanelAgent.
 *
 * @param ise_uuid The active ISE's uuid.
 */
static void slot_set_active_ise_by_uuid (const String &ise_uuid, bool changeDefault)
{
    set_active_ise_by_uuid (ise_uuid, changeDefault);
    return;
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
    bool ret = true;

    std::vector<String> selected_lang;
    get_selected_languages (selected_lang);
    get_interested_iselist_in_languages (selected_lang, list);

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
    gchar       *uuid     = 0;
    gint         ise_type = 0;
    GtkTreeIter  iter;

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_active_ise_list_store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store), &iter,
                                ACTIVE_ISE_UUID, &uuid,
                                ACTIVE_ISE_TYPE, &ise_type, -1);
            if (ise_type == TOOLBAR_KEYBOARD_MODE)
                list.push_back (uuid);
            if (uuid) {
                g_free (uuid);
                uuid = 0;
            }
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_active_ise_list_store), &iter));
    }

    return true;
}

/**
 * @brief Get enable languages list slot function for PanelAgent.
 *
 * @param list The list is used to store languages.
 */
static void slot_get_language_list (std::vector<String> &list)
{
    String lang_name;
    MapStringVectorSizeT::iterator iter = _groups.begin ();

    for (; iter != _groups.end (); iter++) {
        lang_name = scim_get_language_name (iter->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) ==_disabled_langs.end ())
            list.push_back (lang_name);
    }

    return;
}

/**
 * @brief Get all languages list slot function for PanelAgent.
 *
 * @param lang The list is used to store languages.
 */
static void slot_get_all_language (std::vector<String> &lang)
{
    get_all_languages (lang);
}

/**
 * @brief Get specific ISE language list slot function for PanelAgent.
 *
 * @param name The ISE name.
 * @param list The list is used to store ISE languages.
 */
static void slot_get_ise_language (char *name, std::vector<String> &list)
{
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

    return;
}

/**
 * @brief Set ISF language slot function for PanelAgent.
 *
 * @param language The ISF language string.
 */
static void slot_set_isf_language (const String &language)
{
    SCIM_DEBUG_MAIN (1) << "<PanelGtk> enter slot_set_isf_language:" << language << "\n";

    if (language.length () <= 0)
        return;

    std::vector<String> langlist, all_langs;
    scim_split_string_list (langlist, language);
    get_all_languages (all_langs);

    _disabled_langs.clear ();
    _disabled_langs_bak.clear ();
    for (unsigned int i = 0; i < all_langs.size (); i++) {
        if (std::find (langlist.begin (), langlist.end (), all_langs[i]) == langlist.end ()) {
            _disabled_langs.push_back (all_langs[i]);
            _disabled_langs_bak.push_back (all_langs[i]);
        }
    }
    update_active_ise_list_store (true);

    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DISABLED_LANGS), _disabled_langs);

    std::vector<String> enable_langs;
    for (MapStringVectorSizeT::iterator it = _groups.begin (); it != _groups.end (); ++it) {
        String lang_name = scim_get_language_name (it->first);
        if (std::find (_disabled_langs.begin (), _disabled_langs.end (), lang_name) == _disabled_langs.end ())
            enable_langs.push_back (lang_name);
    }
    scim_global_config_write (String (SCIM_GLOBAL_CONFIG_ISF_DEFAULT_LANGUAGES), enable_langs);
    scim_global_config_flush ();

    return;
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
 * @brief Set candidate style slot function for PanelAgent.
 *
 * @param style The new candidate style.
 * @param mode The new candidate mode.
 */
static void slot_set_candidate_ui (int style, int mode)
{
    if (GTK_IS_WIDGET (_input_window) && style == _candidate_style && mode == _candidate_mode)
        return;

    //int  degree   = 0;
    bool vertical = false;
    if (mode == PORTRAIT_VERTICAL_CANDIDATE_MODE || mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
        vertical = true;
    }
    /*if (GTK_IS_WIDGET (_input_window))
    {
        gtk_window_get_rotate (GTK_WINDOW (_input_window), &degree);
    }*/

    if (GTK_IS_WIDGET (_input_window) && _candidate_style == style && _lookup_table_vertical == vertical) {
        if (mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE || mode == LANDSCAPE_VERTICAL_CANDIDATE_MODE) {
            //if (degree == PORTRAIT_DEGREE)
            {
#if HAVE_GCONF
                gtk_window_set_rotate (GTK_WINDOW (_input_window), LANDSCAPE_DEGREE);
#endif
                if (mode == LANDSCAPE_HORIZONTAL_CANDIDATE_MODE)
                    gtk_widget_set_size_request (GTK_WIDGET (_input_window), gdk_screen_height (), -1);
            }
        } else {
            //if (degree == LANDSCAPE_DEGREE)
            {
#if HAVE_GCONF
                gtk_window_set_rotate (GTK_WINDOW (_input_window), PORTRAIT_DEGREE);
#endif
                if (mode == PORTRAIT_HORIZONTAL_CANDIDATE_MODE)
                    gtk_widget_set_size_request (GTK_WIDGET (_input_window), gdk_screen_width (), -1);
            }
        }
        _candidate_mode = (ISF_CANDIDATE_MODE_T)mode;
        _config->write (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_MODE), mode);
        _config->flush ();
    } else {
        _candidate_style       = (ISF_CANDIDATE_STYLE_T)style;
        _candidate_mode        = (ISF_CANDIDATE_MODE_T)mode;
        _lookup_table_vertical = vertical;

        _config->write (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_STYLE), style);
        _config->write (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_MODE), mode);
        _config->write (String (SCIM_CONFIG_PANEL_GTK_LOOKUP_TABLE_VERTICAL), vertical);
        _config->flush ();

        //_panel_agent->reset_keyboard_ise ();
        slot_hide_preedit_string ();
        slot_hide_aux_string ();
        slot_hide_lookup_table ();
        slot_hide_associate_table ();
        create_candidate_window ((ISF_CANDIDATE_STYLE_T)style, vertical);
    }
}

/**
 * @brief Get candidate style slot function for PanelAgent.
 *
 * @param style The current candidate style.
 * @param mode The current candidate mode.
 */
static void slot_get_candidate_ui (int &style, int &mode)
{
    style = _candidate_style;
    mode  = _candidate_mode;
}

/**
 * @brief Set candidate position slot function for PanelAgent.
 *
 * @param left The new candidate left position.
 * @param top The new candidate top position.
 */
static void slot_set_candidate_position (int left, int top)
{
    SCIM_DEBUG_MAIN (3) << "[slot_set_candidate_position] left=" << left << " top=" << top << "\n";
    _candidate_left = left;
    _candidate_top  = top;

    ui_settle_input_window ();

    // Set a transient window for window stack
    // Gets the current XID of the active window into the root window property
    if (GTK_IS_WINDOW (_input_window)) {
        Atom type_return;
        gulong nitems_return;
        gulong bytes_after_return;
        gint format_return;
        guchar *data = NULL;
        Window xParentWindow;
        GdkDisplay *display = gdk_drawable_get_display (_input_window->window);
        gdk_error_trap_push ();
        if (XGetWindowProperty (gdk_x11_get_default_xdisplay (), gdk_x11_get_default_root_xwindow (),
                                gdk_x11_get_xatom_by_name_for_display (display, "_ISF_ACTIVE_WINDOW"),
                                0, G_MAXLONG, False, ((Atom) 33), &type_return,
                                &format_return, &nitems_return, &bytes_after_return,
                                &data) == Success) {
            if ((type_return == ((Atom) 33)) && (format_return == 32) && (data)) {
                xParentWindow = *(Window *)data;

                //printf ("#Mcf-Now ISE get the active window XID : %d\n", xParentWindow);

                if (xParentWindow == 0) {
                    if (!GTK_WIDGET_VISIBLE (GTK_WINDOW (_input_window))) {
                        gtk_window_set_type_hint (GTK_WINDOW (_input_window), GDK_WINDOW_TYPE_HINT_NOTIFICATION);
                    }
                } else {
                    if (!GTK_WIDGET_VISIBLE (GTK_WINDOW (_input_window))) {
                        gtk_window_set_type_hint (GTK_WINDOW (_input_window), GDK_WINDOW_TYPE_HINT_UTILITY);
                    }
                    XSetTransientForHint (GDK_WINDOW_XDISPLAY (_input_window->window),
                                          GDK_WINDOW_XID (_input_window->window),
                                          xParentWindow);
                }
                gdk_window_raise (_input_window->window);
                if (data)
                    XFree (data);
            }
        }
        if (gdk_error_trap_pop ()) {
            /* FIXUP: monitor 'Invalid Window' error -mbqu */
            std::cout << "Oops!!!!!! X error in slot_set_candidate_position()!!!" << "\n";
        }
    }
}

/**
 * @brief Get candidate window geometry slot function for PanelAgent.
 *
 * @param info The data is used to store candidate position and size.
 */
static void slot_get_candidate_geometry (struct rectinfo &info)
{
    if (!GTK_IS_WIDGET (_input_window))
        create_candidate_window (_candidate_style, _lookup_table_vertical);

    gint x      = 0;
    gint y      = 0;
    gint width  = 0;
    gint height = 0;
    if (GTK_IS_WIDGET (_input_window)) {
        gtk_window_get_position (GTK_WINDOW (_input_window), &x, &y);
        if (GTK_WIDGET_VISIBLE (_input_window)) {
            gtk_window_get_size (GTK_WINDOW (_input_window), &width, &height);
        } else {
            gtk_widget_hide (_preedit_area);
            gtk_widget_hide (_aux_area);
            gtk_widget_show (_lookup_table_window);
            gtk_widget_hide (_associate_table_window);
            gtk_widget_hide (_candidate_separator);
            gtk_widget_hide (_associate_separator);
            gtk_widget_hide (_middle_separator);
            gtk_window_get_size (GTK_WINDOW (_input_window), &width, &height);
            gtk_widget_hide (_lookup_table_window);
        }
    }
    info.pos_x  = x;
    info.pos_y  = y;
    info.width  = width;
    info.height = height;
    //std::cout << "[slot_get_candidate_rect] x=" << x << " y=" << y << " width=" << width << " height=" << height << "\n";
}

/**
 * @brief Set keyboard ISE slot function for PanelAgent.
 *
 * @param ise_uuid The variable is ISE uuid.
 */
static void slot_set_keyboard_ise (const String &ise_uuid)
{
    if (ise_uuid.length () <= 0)
        return;

    bool   ret  = false;
    gchar *name = 0;
    gchar *uuid = 0;
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_active_ise_list_store), &iter)) {
        do {
            gtk_tree_model_get (GTK_TREE_MODEL (_active_ise_list_store), &iter,
                                ACTIVE_ISE_NAME, &name,
                                ACTIVE_ISE_UUID, &uuid, -1);
            if (uuid != NULL && !strcmp (ise_uuid.c_str (), uuid)) {
                String language = String ("~other");//scim_get_locale_language (scim_get_current_locale ());
                _config->write (String (SCIM_CONFIG_DEFAULT_IMENGINE_FACTORY) + String ("/") + language, String (uuid));
                _config->flush ();
                _panel_agent->change_factory (uuid);
                ret = true;
            }
            if (name) {
                g_free (name);
                name = 0;
            }
            if (uuid) {
                g_free (uuid);
                uuid = 0;
            }
            if (ret)
                return;
        } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (_active_ise_list_store), &iter));
    }
}

/**
 * @brief Get current keyboard ISE name and uuid slot function for PanelAgent.
 *
 * @param ise_name The variable is used to store ISE name.
 * @param ise_uuid The variable is used to store ISE uuid.
 */
static void slot_get_keyboard_ise (String &ise_name, String &ise_uuid)
{
    String language = String ("~other");//scim_get_locale_language (scim_get_current_locale ());
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
static void slot_start_default_ise ()
{
    DEFAULT_ISE_T default_ise;

    default_ise.type = (TOOLBAR_MODE_T)scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_TYPE), (int)_initial_ise.type);
    default_ise.uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), _initial_ise.uuid);
    default_ise.name = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_NAME), _initial_ise.name);

    if (!set_active_ise_by_uuid (default_ise.uuid, 1)) {
        if (default_ise.uuid != _initial_ise.uuid)
            set_active_ise_by_uuid (_initial_ise.uuid, 1);
    }

    return;
}

/**
 * @brief Send key event slot function for PanelAgent.
 *
 * @param key The key event should be sent.
 */
static void slot_send_key_event (const KeyEvent &key)
{
    //std::cout << "[slot_send_key_event] code : " << key.code << " mask : " << key.mask << "\n";
}

/**
 * @brief Lock slot function for PanelAgent.
 */
static void slot_lock (void)
{
    G_LOCK (_panel_agent_lock);
}

/**
 * @brief Unlock slot function for PanelAgent.
 */
static void slot_unlock (void)
{
    G_UNLOCK (_panel_agent_lock);
}
//////////////////////////////////////////////////////////////////////
// End of PanelAgent-Functions
//////////////////////////////////////////////////////////////////////

/**
 * @brief Timeout function for candidate show timer.
 *
 * @param data Data pointer to pass when it is called.
 *
 * @return FALSE.
 */
static gboolean candidate_show_timeout_cb (gpointer data)
{
    SCIM_DEBUG_MAIN (1) << "candidate_show_timeout_cb......\n";

    slot_lock ();
    if (g_isf_candidate_table.get_current_page_size () && _candidate_timer) {
        update_table (CANDIDATE_TABLE, g_isf_candidate_table, _lookup_table_items,
                      _lookup_table_index, _lookup_table_index_pos, _candidate_scroll);

        if (!_candidate_is_showed)
            slot_show_lookup_table ();
    }

    _candidate_timer = 0;
    slot_unlock ();

    return FALSE;
}

/**
 * @brief Timeout function for check exit timer.
 *
 * @param data Data pointer to pass when it is called.
 *
 * return TRUE.
 */
static gboolean check_exit_timeout_cb (gpointer data)
{
    G_LOCK (_global_resource_lock);
    if (_should_exit) {
        gdk_threads_enter ();
        gtk_main_quit ();
        gdk_threads_leave ();
    }
    G_UNLOCK (_global_resource_lock);

    return (gboolean)TRUE;
}

/**
 * @brief Callback function for abnormal signal.
 *
 * @param sig The signal.
 */
static void signalhandler (int sig)
{
    SCIM_DEBUG_MAIN (1) << "In signal handler...\n";

    if (_panel_agent != NULL)
        _panel_agent->stop ();
}

#if HAVE_GCONF
/**
 * @brief Create prediction engine option information.
 *
 * @return the pointer of ImeSettingPredictionEngineOptionInfo_S.
 */
static ImeSettingPredictionEngineOptionInfo_S* prediction_engine_option_info_create (void)
{
    ImeSettingPredictionEngineOptionInfo_S *prediction_engine_option_info_s;

    prediction_engine_option_info_s = g_slice_new0 (ImeSettingPredictionEngineOptionInfo_S);

    prediction_engine_option_info_s->keypad_type                   = IME_SETTING_KEYPAD_TYPE_3X4;
    prediction_engine_option_info_s->word_cmpletion_point          = 0;
    prediction_engine_option_info_s->spell_correction              = FALSE;
    prediction_engine_option_info_s->next_word_prediction          = FALSE;
    prediction_engine_option_info_s->auto_substitution             = FALSE;
    prediction_engine_option_info_s->multitap_word_completion      = FALSE;
    prediction_engine_option_info_s->regional_input                = FALSE;
    prediction_engine_option_info_s->word_cmpletion_point_prev     = 0;
    prediction_engine_option_info_s->spell_correction_prev         = FALSE;
    prediction_engine_option_info_s->next_word_prediction_prev     = FALSE;
    prediction_engine_option_info_s->auto_substitution_prev        = FALSE;
    prediction_engine_option_info_s->multitap_word_completion_prev = FALSE;
    prediction_engine_option_info_s->regional_input_prev           = FALSE;

    return prediction_engine_option_info_s;
}

/**
 * @brief Get bit mask for prediction engine option.
 *
 * @param value The source value.
 * @param index The index for mask.
 *
 * @return TRUE or FALSE.
 */
static gboolean prediction_engine_option_get_noti_bit_mask (int value, int index)
{
    int mask;

    mask = 1 << index;
    if ((value & mask) ? TRUE : FALSE)
        SCIM_DEBUG_MAIN (1) << "true.....\n";
    else
        SCIM_DEBUG_MAIN (1) << "false....\n";
    return (value & mask) ? TRUE : FALSE;
}

/**
 * @brief Set prediction engine option information.
 *
 * @param prediction_engine_option_info_s The ImeSettingPredictionEngineOptionInfo_S pointer.
 * @param keypad_type The keypad layout is IME_SETTING_KEYPAD_TYPE_3X4 or IME_SETTING_KEYPAD_TYPE_QTY.
 */
static void prediction_engine_option_info_set (ImeSettingPredictionEngineOptionInfo_S *prediction_engine_option_info_s, gint keypad_type)
{
    GConfValue* value = NULL;
    int  error_code = 0;
    gint gconfvalue = 0;

    g_return_if_fail ((keypad_type >= 0) && (keypad_type < 2));

    prediction_engine_option_info_s->keypad_type = keypad_type;

    value = gconf_value_new (GCONF_VALUE_INT);

    if (keypad_type == IME_SETTING_KEYPAD_TYPE_3X4) {
        // WORD_COMPLETION_POINT
        if (!(phonestatus_get ((char *)PS_KEY_APPL_ISE_NWORDCOMPPOINT, value, &error_code)))
            g_printf ("\n>>>>>>[%s][%d]Error  in get as result is no true,error =%d \n", __FUNCTION__, __LINE__, error_code);
        gconfvalue = gconf_value_get_int (value);

        prediction_engine_option_info_s->word_cmpletion_point_prev = prediction_engine_option_info_s->word_cmpletion_point = gconfvalue;
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_3x4_WORD_COMPLETION_POINT), prediction_engine_option_info_s->word_cmpletion_point);

        // OPTIONS
        if (!(phonestatus_get ((char *)PS_KEY_APPL_ISE_PREDICTION_ENGINE_OPTION, value, &error_code)))
            g_printf ("\n>>>>>>[%s][%d]Error  in get as result is no true,error =%d \n", __FUNCTION__, __LINE__, error_code);
        gconfvalue = gconf_value_get_int (value);

        SCIM_DEBUG_MAIN (1) << "spell_correction_prev:";
        prediction_engine_option_info_s->spell_correction_prev = prediction_engine_option_info_s->spell_correction = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_0);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_3x4_SPELL_CORRECTION), prediction_engine_option_info_s->spell_correction);

        SCIM_DEBUG_MAIN (1) << "next_word_prediction_prev:";
        prediction_engine_option_info_s->next_word_prediction_prev = prediction_engine_option_info_s->next_word_prediction = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_1);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_3x4_NEXT_WORD_PREDIECTION), prediction_engine_option_info_s->next_word_prediction);

        SCIM_DEBUG_MAIN (1) << "auto_substitution_prev:";
        prediction_engine_option_info_s->auto_substitution_prev = prediction_engine_option_info_s->auto_substitution = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_2);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_3x4_AUTO_SUBSTITUTION), prediction_engine_option_info_s->auto_substitution);

        SCIM_DEBUG_MAIN (1) << "multitap_word_completion_prev:";
        prediction_engine_option_info_s->multitap_word_completion_prev = prediction_engine_option_info_s->multitap_word_completion = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_3);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_3x4_MULTITAP_WORD_COMPLETION), prediction_engine_option_info_s->multitap_word_completion);

        //prediction_engine_option_info_s->regional_input = 0; // unused !!
    } else if (keypad_type == IME_SETTING_KEYPAD_TYPE_QTY) {
        // WORD_COMPLETION_POINT
        if (!(phonestatus_get ((char *)PS_KEY_APPL_ISE_QWORDCOMPPOINT, value, &error_code)))
            g_printf ("\n>>>>>>[%s][%d]Error  in get as result is no true,error =%d \n", __FUNCTION__, __LINE__, error_code);
        gconfvalue = gconf_value_get_int (value);

        prediction_engine_option_info_s->word_cmpletion_point_prev = prediction_engine_option_info_s->word_cmpletion_point = gconfvalue;
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_QWERTY_WORD_COMPLETION_POINT), prediction_engine_option_info_s->word_cmpletion_point);

        // OPTIONS
        if (!(phonestatus_get ((char *)PS_KEY_APPL_ISE_PREDICTION_ENGINE_OPTION, value, &error_code)))
            g_printf ("\n>>>>>>[%s][%d]Error  in get as result is no true,error =%d \n", __FUNCTION__, __LINE__, error_code);
        gconfvalue = gconf_value_get_int (value);

        SCIM_DEBUG_MAIN (1) << "spell_correction_prev:";
        prediction_engine_option_info_s->spell_correction_prev = prediction_engine_option_info_s->spell_correction = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_4);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_QWERTY_SPELL_CORRECTION), prediction_engine_option_info_s->spell_correction);

        SCIM_DEBUG_MAIN (1) << "next_word_prediction_prev:";
        prediction_engine_option_info_s->next_word_prediction_prev = prediction_engine_option_info_s->next_word_prediction = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_5);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_QWERTY_NEXT_WORD_PREDIECTION), prediction_engine_option_info_s->next_word_prediction);

        SCIM_DEBUG_MAIN (1) << "auto_substitution_prev:";
        prediction_engine_option_info_s->auto_substitution_prev = prediction_engine_option_info_s->auto_substitution = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_6);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_QWERTY_AUTO_SUBSTITUTION), prediction_engine_option_info_s->auto_substitution);

        SCIM_DEBUG_MAIN (1) << "regional_input_prev:";
        prediction_engine_option_info_s->regional_input_prev = prediction_engine_option_info_s->regional_input = prediction_engine_option_get_noti_bit_mask (gconfvalue, IME_BIT_7);
        _config->write (String (SCIM_PREDICTION_ENGINE_CONFIG_QWERTY_REGIONAL_INPUT), prediction_engine_option_info_s->regional_input);

        //prediction_engine_option_info_s->multitap_word_completion = 0; // unused !!
    }

    gconf_value_free (value);
}

/**
 * @brief Callback function for prediction engine option setting.
 *
 * @param t_src_id The source id of system event.
 * @param p_noti_id The handler of notice id.
 * @param pg_parameters The GArray pointer.
 * @param arg Data pointer to pass when it is called.
 *
 * @return FALSE.
 */
static bool prediction_engine_option_setting_ed_handler (EvtSysEventSourceId_t  t_src_id,
                                           const char            *p_noti_id,
                                           const GArray          *pg_parameters,
                                           void                  *arg)
{
    if (strcmp (p_noti_id, _ime_setting_ed_id[0]) != 0 &&
        strcmp (p_noti_id, _ime_setting_ed_id[1]) != 0 &&
        strcmp (p_noti_id, _ime_setting_ed_id[2]) != 0)
        return FALSE;

    ImeSettingPredictionEngineOptionInfo_S  *prediction_engine_option_info_s;
    prediction_engine_option_info_s = prediction_engine_option_info_create ();
    for (gint i = IME_SETTING_KEYPAD_TYPE_3X4; i < IME_SETTING_KEYPAD_TYPE_NUM; i++) {
        prediction_engine_option_info_set (prediction_engine_option_info_s, i);
    }

    _config->flush ();
    _config->reload ();

    g_slice_free (ImeSettingPredictionEngineOptionInfo_S, prediction_engine_option_info_s);

    return FALSE;
}

/**
 * @brief Subscrible prediction engine option setting.
 *
 * @return event handle if successfully, otherwise return -1.
 */
static int prediction_engine_option_setting_ed_subscribe (void)
{
    int i;
    int ret;
    int ierror;
    int event_handle;

    ierror = EvtSysLibraryOpen (&event_handle);
    if (ierror) {
        std::cerr << "Error EvtSysLibraryOpen ()!! Return Value :" << ierror << "\n";
        return -1;
    }

    EvtSysEventSubscription_t *subscriptions = g_new (EvtSysEventSubscription_t, _ime_setting_ed_number);

    for (i = 0; i < _ime_setting_ed_number; i++) {
        subscriptions[i].noti_id           = _ime_setting_ed_id[i];
        subscriptions[i].filter_expression = NULL;
        subscriptions[i].callback          = prediction_engine_option_setting_ed_handler;
        subscriptions[i].priv_data         = NULL;
    }

    if (ret = EvtSysEventMultiSubscribe (event_handle, subscriptions, _ime_setting_ed_number)) {
        std::cerr << "Error EvtSysEventMultiSubscribe ()!! Return Value :" << ret << "\n";
        event_handle = -1;
    } else {
        for (i = 0; i < _ime_setting_ed_number; i++)
            _ed_subscription_id[i] = subscriptions[i].subscription_id;
    }
    if (subscriptions)
        g_free (subscriptions);

    return event_handle;
}
#endif

int main (int argc, char *argv [])
{
    struct tms    tiks_buf;
    _clock_start = times (&tiks_buf);

    int           i;
    size_t      j;
    int           ret                   = 0;
#if HAVE_GCONF
    int           g_ed_event_handle     = -1;
    GConfClient  *client                = 0;
    GError       *err                   = 0;
    int           lang_id;
#endif

    bool          daemon                = false;
    bool          should_resident       = true;

    int           new_argc              = 0;
    char        **new_argv              = new char * [40];
    String        config_name ("socket");
    ConfigModule *config_module = 0;
    String        display_name;

    check_time ("\nStarting ISF Panel ...... ");

    DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
    DebugOutput::enable_debug (SCIM_DEBUG_MainMask);

    // Parse command options
    i = 0;
    while (i<argc) {
        if (++i >= argc) break;

        if (String ("-c") == argv [i] ||
            String ("--config") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            config_name = argv [i];
            continue;
        }

        if (String ("-h") == argv [i] ||
            String ("--help") == argv [i]) {
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

        if (String ("-d") == argv [i] ||
            String ("--daemon") == argv [i]) {
            daemon = true;
            continue;
        }

        if (String ("-ns") == argv [i] ||
            String ("--no-stay") == argv [i]) {
            should_resident = false;
            continue;
        }

        if (String ("-v") == argv [i] ||
            String ("--verbose") == argv [i]) {
            if (++i >= argc) {
                std::cerr << "no argument for option " << argv [i-1] << "\n";
                ret = -1;
                goto cleanup;
            }
            DebugOutput::set_verbose_level (atoi (argv [i]));
            continue;
        }

        if (String ("-o") == argv [i] ||
            String ("--output") == argv [i]) {
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
    } // End of command line parsing.

    new_argv [new_argc ++] = argv [0];

    // Store the rest argvs into new_argv.
    for (++i; i < argc && new_argc < 37; ++i) {
        new_argv [new_argc ++] = argv [i];
    }

    // Make up DISPLAY env.
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
        // Load config module
        config_module = new ConfigModule (config_name);

        if (!config_module || !config_module->valid ()) {
            std::cerr << "Can not load " << config_name << " Config module.\n";
            ret = -1;
            goto cleanup;
        }
    } else {
        _config = new DummyConfig ();
    }

    // Init threads
    g_thread_init (NULL);
    gdk_threads_init ();
    check_time ("gdk_thread_init");

    signal (SIGQUIT, signalhandler);
    signal (SIGTERM, signalhandler);
    signal (SIGINT,  signalhandler);
    //signal (SIGHUP,  signalhandler);

    gtk_init (&new_argc, &new_argv);
    check_time ("gtk_init");

    // Get current display.
    {
#if GDK_MULTIHEAD_SAFE
        const char *p = gdk_display_get_name (gdk_display_get_default ());
#else
        const char *p = getenv ("DISPLAY");
#endif
        if (p) display_name = String (p);
    }

    try {
        if (!initialize_panel_agent (config_name, display_name, should_resident)) {
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

    // Create config instance
    if (_config.null () && config_module && config_module->valid())
        _config = config_module->create_config ();
    if (_config.null ()) {
        std::cerr << "Failed to create Config instance from " << config_name << " Config module.\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("create config instance");

#if HAVE_GCONF
    //if (gconf_init (argc, argv, &err))
    {
        client  = gconf_client_get_default ();
        if (client && client->engine) {
            lang_id = gconf_client_get_int (client, SETTINGS_INPUTLANGUAGETYPE, &err);
            if (err) {
                std::cerr << "gconf_client_get_int : " << err->message << "\n";
                g_error_free (err);
                err = NULL;
            } else {
                set_default_language_by_id (lang_id);
                gconf_client_add_dir (client, "/Application/Settings", GCONF_CLIENT_PRELOAD_NONE, NULL);
                gconf_client_notify_add (client, SETTINGS_INPUTLANGUAGETYPE,
                                         input_lang_key_changed_callback,
                                         NULL, NULL, &err);
                check_time ("gconf_client_notify_add");
            }
        }
    }
    if (err) {
        std::cerr << "gconf error : " << err->message << "\n";
        g_error_free (err);
        err = NULL;
    }

    g_ed_event_handle = prediction_engine_option_setting_ed_subscribe ();
#endif

    _screen_width        = gdk_screen_get_width (gdk_screen_get_default ());
    _screen_height       = gdk_screen_get_height (gdk_screen_get_default ());
    _width_rate          = (float)(_screen_width / BASE_SCREEN_WIDTH);
    _height_rate         = (float)(_screen_height / BASE_SCREEN_HEIGHT);
    _softkeybar_height   = (int)(BASE_SOFTKEYBAR_HEIGHT * _height_rate);
    _panel_width         = (int)(BASE_PANEL_WIDTH * _width_rate);
    _panel_height        = (int)(BASE_PANEL_HEIGHT * _height_rate);
    _setup_button_width  = (int)(BASE_SETUP_BUTTON_WIDTH * _width_rate);
    _setup_button_height = (int)(BASE_SETUP_BUTTON_HEIGHT * _height_rate);
    _help_icon_width     = (int)(BASE_HELP_ICON_WIDTH * _width_rate);
    _help_icon_height    = (int)(BASE_HELP_ICON_HEIGHT * _height_rate);

    ui_initialize ();
    check_time ("ui_initialize");

    try {
        _panel_agent->send_display_name (display_name);

        _initial_ise.type = (TOOLBAR_MODE_T)scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_TYPE), (int)TOOLBAR_HELPER_MODE);
        _initial_ise.uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String ("ff110940-b8f0-4062-9ff6-a84f4f3575c0"));
        _initial_ise.name = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_NAME), String ("Input Pad"));

        for (j = 0; j < _uuids.size (); ++j) {
            if (!_uuids[j].compare(_initial_ise.uuid))
                break;
        }

        if (j == _uuids.size ())
        {
            for (j = 0; j < _uuids.size (); ++j) {
                if (_modes[j] == TOOLBAR_HELPER_MODE) {
                    _initial_ise.type = _modes[j];
                    _initial_ise.uuid = _uuids[j];
                    _initial_ise.name = _names[j];
                }
            }
        }

        slot_start_default_ise ();
    } catch (scim::Exception & e) {
        std::cerr << e.what() << "\n";
        ret = -1;
        goto cleanup;
    }

    if (daemon) {
        check_time ("ISF Panel run as daemon");
        scim_daemon ();
    }

    // Connect the configuration reload signal.
    _config->signal_connect_reload (slot (ui_config_reload_callback));

    if (!run_panel_agent ()) {
        std::cerr << "Failed to run Socket Server!\n";
        ret = -1;
        goto cleanup;
    }
    check_time ("run_panel_agent");

    _check_exit_timeout = gtk_timeout_add (500, check_exit_timeout_cb, NULL);

    _ise_list_changed     = false;
    _setup_enable_changed = false;
    gdk_threads_enter ();
    check_time ("ISF Panel launch time");
    gtk_main ();
    gdk_threads_leave ();

    // Exiting...
    g_thread_join (_panel_agent_thread);

    // Destroy candidate window
    if (GTK_IS_WIDGET (_lookup_table_window))
        gtk_widget_destroy (_lookup_table_window);
    if (GTK_IS_WINDOW (_input_window))
        gtk_widget_destroy (_input_window);

    _config->flush ();
    ret = 0;

cleanup:
    if (!_config.null ())
        _config.reset ();
    if (config_module)
        delete config_module;
#if HAVE_GCONF
    if (client)
        g_object_unref (client);

    if (EvtSysEventHandleExist (g_ed_event_handle)) {
        EvtSysEventMultiUnsubscribe (g_ed_event_handle, _ed_subscription_id, _ime_setting_ed_number);
        if (g_ed_event_handle != -1)
            EvtSysLibraryClose (g_ed_event_handle);
    }
#endif
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
