/** @file isf_info_manager.cpp
 *  @brief Implementation of class InfoManager.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2016 Samsung Electronics Co., Ltd.
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
 * Modifications by Samsung Electronics Co., Ltd.
 * 1. Add new signals
 *    a. m_signal_set_keyboard_ise and m_signal_get_keyboard_ise
 *    b. m_signal_focus_in and m_signal_focus_out
 *    c. m_signal_expand_candidate, m_signal_contract_candidate and m_signal_set_candidate_ui
 *    d. m_signal_get_ise_list, m_signal_get_keyboard_ise_list, m_signal_update_ise_geometry and m_signal_get_ise_information
 *    e. m_signal_set_active_ise_by_uuid and m_signal_get_ise_info_by_uuid
 *    f. m_signal_accept_connection, m_signal_close_connection and m_signal_exit
 * 2. Add new interface APIs in PanelClient class
 *    a. get_helper_manager_id (), has_helper_manager_pending_event () and filter_helper_manager_event ()
 *    b. update_candidate_panel_event (), update_input_panel_event () and select_aux ()
 *    c. candidate_more_window_show () and candidate_more_window_hide ()
 *    d. update_displayed_candidate_number () and update_candidate_item_layout ()
 *    e. stop_helper (), send_longpress_event () and update_ise_list ()
 *    f. filter_event (), filter_exception_event () and get_server_id ()
 * 3. Donot use thread to receive message
 * 4. Monitor socket frontend for self-recovery function
 *
 * $Id: scim_panel_agent.cpp,v 1.8.2.1 2006/01/09 14:32:18 suzhe Exp $
 *
 */


#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_HELPER
#define Uses_SCIM_EVENT
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_UTILITY

#include <string.h>
#include <sys/types.h>
#include <sys/times.h>
#include <dlog.h>
#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"
#include "isf_panel_agent_manager.h"
#include "isf_debug.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_PANEL_EFL"

#define MIN_REPEAT_TIME     2.0


namespace scim
{

typedef Signal0<void>
InfoManagerSignalVoid;

typedef Signal1<void, int>
InfoManagerSignalInt;

typedef Signal1<void, int&>
InfoManagerSignalInt2;

typedef Signal1<void, const String&>
InfoManagerSignalString;

typedef Signal2<void, const String&, bool>
InfoManagerSignalStringBool;

typedef Signal2<void, String&, String&>
InfoManagerSignalString2;

typedef Signal2<void, int, const String&>
InfoManagerSignalIntString;

typedef Signal1<void, const PanelFactoryInfo&>
InfoManagerSignalFactoryInfo;

typedef Signal1<void, const std::vector <PanelFactoryInfo> &>
InfoManagerSignalFactoryInfoVector;

typedef Signal1<void, const LookupTable&>
InfoManagerSignalLookupTable;

typedef Signal1<void, const Property&>
InfoManagerSignalProperty;

typedef Signal1<void, const PropertyList&>
InfoManagerSignalPropertyList;

typedef Signal2<void, int, int>
InfoManagerSignalIntInt;

typedef Signal2<void, int&, int&>
InfoManagerSignalIntInt2;

typedef Signal3<void, int, int, int>
InfoManagerSignalIntIntInt;

typedef Signal3<void, const String &, const String &, const String &>
InfoManagerSignalString3;

typedef Signal4<void, int, int, int, int>
InfoManagerSignalIntIntIntInt;

typedef Signal2<void, int, const Property&>
InfoManagerSignalIntProperty;

typedef Signal2<void, int, const PropertyList&>
InfoManagerSignalIntPropertyList;

typedef Signal2<void, int, const HelperInfo&>
InfoManagerSignalIntHelperInfo;

typedef Signal3<void, const String&, const AttributeList&, int>
InfoManagerSignalAttributeStringInt;

typedef Signal2<void, const String&, const AttributeList&>
InfoManagerSignalAttributeString;

typedef Signal1<void, std::vector <String> &>
InfoManagerSignalStringVector;

typedef Signal1<bool, HELPER_ISE_INFO&>
InfoManagerSignalBoolHelperInfo;

typedef Signal1<bool, std::vector <String> &>
InfoManagerSignalBoolStringVector;

typedef Signal2<void, char*, std::vector <String> &>
InfoManagerSignalStrStringVector;

typedef Signal2<bool, const String&, ISE_INFO&>
InfoManagerSignalStringISEINFO;

typedef Signal2<bool, String, int&>
InfoManagerSignalStringInt;

typedef Signal1<void, const KeyEvent&>
InfoManagerSignalKeyEvent;

typedef Signal1<void, struct rectinfo&>
InfoManagerSignalRect;

typedef Signal6<bool, String, String&, String&, int&, int&, String&>
InfoManagerSignalBoolString4int2;

typedef Signal2<void, int, struct rectinfo&>
InfoManagerSignalIntRect;

typedef Signal2<bool, int, String>
InfoManagerSignalIntString2;

struct HelperClientStub {
    int id;
    int ref;

    HelperClientStub (int i = 0, int r = 0) : id (i), ref (r) { }
};

struct IMControlStub {
    std::vector<ISE_INFO> info;
    std::vector<int> count;
};

#define DEFAULT_CONTEXT_VALUE 0xfff

#if SCIM_USE_STL_EXT_HASH_MAP
typedef __gnu_cxx::hash_map <int, ClientInfo, __gnu_cxx::hash <int> >       ClientRepository;
typedef __gnu_cxx::hash_map <int, HelperInfo, __gnu_cxx::hash <int> >       HelperInfoRepository;
typedef __gnu_cxx::hash_map <uint32, String, __gnu_cxx::hash <unsigned int> > ClientContextUUIDRepository;
typedef __gnu_cxx::hash_map <String, HelperClientStub, scim_hash_string>    HelperClientIndex;
typedef __gnu_cxx::hash_map <String, std::vector < std::pair <uint32, String> >, scim_hash_string>    StartHelperICIndex;
#elif SCIM_USE_STL_HASH_MAP
typedef std::hash_map <int, ClientInfo, std::hash <int> >                   ClientRepository;
typedef std::hash_map <int, HelperInfo, std::hash <int> >                   HelperInfoRepository;
typedef std::hash_map <uint32, String, std::hash <unsigned int> >           ClientContextUUIDRepository;
typedef std::hash_map <String, HelperClientStub, scim_hash_string>          HelperClientIndex;
typedef std::hash_map <String, std::vector < std::pair <uint32, String> >, scim_hash_string>          StartHelperICIndex;
#else
typedef std::map <int, ClientInfo>                                          ClientRepository;
typedef std::map <int, HelperInfo>                                          HelperInfoRepository;
typedef std::map <uint32, String>                                           ClientContextUUIDRepository;
typedef std::map <String, HelperClientStub>                                 HelperClientIndex;
typedef std::map <String, std::vector < std::pair <uint32, String> > >                                StartHelperICIndex;
#endif

typedef std::map <String, uint32>              UUIDCountRepository;
typedef std::map <String, enum HelperState>    UUIDStateRepository;
typedef std::map <String, int>                 StringIntRepository;
typedef std::map <int, struct IMControlStub>   IMControlRepository;
typedef std::map <int, int>                    IntIntRepository;

static uint32
get_helper_ic (int client, uint32 context)
{
    return (uint32) (client & 0xFFFF) | ((context & 0x7FFF) << 16);
}

static void
get_imengine_client_context (uint32 helper_ic, int& client, uint32& context)
{
    client   = (int) (helper_ic & 0xFFFF);
    context  = ((helper_ic >> 16) & 0x7FFF);
}

//==================================== InfoManager ===========================
class InfoManager::InfoManagerImpl
{
    int                                 m_current_screen;

    String                              m_config_name;
    String                              m_display_name;

    int                                 m_current_socket_client;
    uint32                              m_current_client_context;
    String                              m_current_context_uuid;
    TOOLBAR_MODE_T                      m_current_toolbar_mode;
    uint32                              m_current_helper_option;
    String                              m_current_helper_uuid;
    String                              m_last_helper_uuid;
    String                              m_current_ise_name;
    int                                 m_pending_active_imcontrol_id;
    int                                 m_show_request_client_id;
    int                                 m_active_client_id;
    IntIntRepository                    m_panel_client_map;
    IntIntRepository                    m_imcontrol_map;
    bool                                m_should_shared_ise;
    bool                                m_ise_exiting;
    bool                                m_is_imengine_aux;
    bool                                m_is_imengine_candidate;

    int                                 m_last_socket_client;
    uint32                              m_last_client_context;
    String                              m_last_context_uuid;

    char*                               m_ise_context_buffer;
    size_t                              m_ise_context_length;

    ClientRepository                    m_client_repository;
    /*
    * Each Helper ISE has two socket connect between InfoManager and HelperAgent.
    * m_helper_info_repository records the active connection.
    * m_helper_active_info_repository records the passive connection.
    */
    HelperInfoRepository                m_helper_info_repository;
    HelperInfoRepository                m_helper_active_info_repository;
    HelperClientIndex                   m_helper_client_index;

    /* when helper register, notify imcontrol client */
    StringIntRepository                 m_ise_pending_repository;
    IMControlRepository                 m_imcontrol_repository;

    StartHelperICIndex                  m_start_helper_ic_index;

    /* Keyboard ISE */
    ClientContextUUIDRepository         m_client_context_uuids;

    /* Helper ISE */
    ClientContextUUIDRepository         m_client_context_helper;
    UUIDCountRepository                 m_helper_uuid_count;

    InfoManagerSignalVoid                m_signal_turn_on;
    InfoManagerSignalVoid                m_signal_turn_off;
    InfoManagerSignalVoid                m_signal_show_panel;
    InfoManagerSignalVoid                m_signal_hide_panel;
    InfoManagerSignalInt                 m_signal_update_screen;
    InfoManagerSignalIntIntInt           m_signal_update_spot_location;
    InfoManagerSignalFactoryInfo         m_signal_update_factory_info;
    InfoManagerSignalVoid                m_signal_start_default_ise;
    InfoManagerSignalVoid                m_signal_stop_default_ise;
    InfoManagerSignalIntInt              m_signal_update_input_context;
    InfoManagerSignalIntInt              m_signal_set_candidate_ui;
    InfoManagerSignalIntInt2             m_signal_get_candidate_ui;
    InfoManagerSignalIntInt              m_signal_set_candidate_position;
    InfoManagerSignalRect                m_signal_get_candidate_geometry;
    InfoManagerSignalRect                m_signal_get_input_panel_geometry;
    InfoManagerSignalString              m_signal_set_keyboard_ise;
    InfoManagerSignalString2             m_signal_get_keyboard_ise;
    InfoManagerSignalString              m_signal_show_help;
    InfoManagerSignalFactoryInfoVector   m_signal_show_factory_menu;
    InfoManagerSignalVoid                m_signal_show_preedit_string;
    InfoManagerSignalVoid                m_signal_show_aux_string;
    InfoManagerSignalVoid                m_signal_show_lookup_table;
    InfoManagerSignalVoid                m_signal_show_associate_table;
    InfoManagerSignalVoid                m_signal_hide_preedit_string;
    InfoManagerSignalVoid                m_signal_hide_aux_string;
    InfoManagerSignalVoid                m_signal_hide_lookup_table;
    InfoManagerSignalVoid                m_signal_hide_associate_table;
    InfoManagerSignalAttributeStringInt  m_signal_update_preedit_string;
    InfoManagerSignalInt                 m_signal_update_preedit_caret;
    InfoManagerSignalAttributeString     m_signal_update_aux_string;
    InfoManagerSignalLookupTable         m_signal_update_lookup_table;
    InfoManagerSignalLookupTable         m_signal_update_associate_table;
    InfoManagerSignalPropertyList        m_signal_register_properties;
    InfoManagerSignalProperty            m_signal_update_property;
    InfoManagerSignalIntPropertyList     m_signal_register_helper_properties;
    InfoManagerSignalIntProperty         m_signal_update_helper_property;
    InfoManagerSignalIntHelperInfo       m_signal_register_helper;
    InfoManagerSignalInt                 m_signal_remove_helper;
    InfoManagerSignalStringBool          m_signal_set_active_ise_by_uuid;
    InfoManagerSignalVoid                m_signal_focus_in;
    InfoManagerSignalVoid                m_signal_focus_out;
    InfoManagerSignalVoid                m_signal_expand_candidate;
    InfoManagerSignalVoid                m_signal_contract_candidate;
    InfoManagerSignalInt                 m_signal_select_candidate;
    InfoManagerSignalBoolStringVector    m_signal_get_ise_list;
    InfoManagerSignalBoolHelperInfo      m_signal_get_all_helper_ise_info;
    InfoManagerSignalStringBool          m_signal_set_has_option_helper_ise_info;
    InfoManagerSignalStringBool          m_signal_set_enable_helper_ise_info;
    InfoManagerSignalVoid                m_signal_show_helper_ise_list;
    InfoManagerSignalVoid                m_signal_show_helper_ise_selector;
    InfoManagerSignalStringInt           m_signal_is_helper_ise_enabled;
    InfoManagerSignalBoolString4int2     m_signal_get_ise_information;
    InfoManagerSignalBoolStringVector    m_signal_get_keyboard_ise_list;
    InfoManagerSignalIntIntIntInt        m_signal_update_ise_geometry;
    InfoManagerSignalStringVector        m_signal_get_language_list;
    InfoManagerSignalStringVector        m_signal_get_all_language;
    InfoManagerSignalStrStringVector     m_signal_get_ise_language;
    InfoManagerSignalStringISEINFO       m_signal_get_ise_info_by_uuid;
    InfoManagerSignalKeyEvent            m_signal_send_key_event;

    InfoManagerSignalInt                 m_signal_accept_connection;
    InfoManagerSignalInt                 m_signal_close_connection;
    InfoManagerSignalVoid                m_signal_exit;

    InfoManagerSignalVoid                m_signal_transaction_start;
    InfoManagerSignalVoid                m_signal_transaction_end;

    InfoManagerSignalVoid                m_signal_lock;
    InfoManagerSignalVoid                m_signal_unlock;

    InfoManagerSignalVoid                m_signal_show_ise;
    InfoManagerSignalVoid                m_signal_hide_ise;

    InfoManagerSignalVoid                m_signal_will_show_ack;
    InfoManagerSignalVoid                m_signal_will_hide_ack;

    InfoManagerSignalInt                 m_signal_set_keyboard_mode;

    InfoManagerSignalVoid                m_signal_candidate_will_hide_ack;
    InfoManagerSignalInt2                m_signal_get_ise_state;
    InfoManagerSignalString3             m_signal_run_helper;

    InfoManagerSignalIntRect             m_signal_get_recent_ise_geometry;

    InfoManagerSignalIntString2          m_signal_check_privilege_by_sockfd;

    PanelAgentManager                    m_panel_agent_manager;


public:
    InfoManagerImpl ()
        : m_current_screen (0),
          m_current_socket_client (-1), m_current_client_context (0),
          m_current_toolbar_mode (TOOLBAR_KEYBOARD_MODE),
          m_current_helper_option (0),
          m_pending_active_imcontrol_id (-1),
          m_show_request_client_id (-1),
          m_active_client_id (-1),
          m_should_shared_ise (false),
          m_ise_exiting (false), m_is_imengine_aux (false), m_is_imengine_candidate (false),
          m_last_socket_client (-1), m_last_client_context (0),
          m_ise_context_buffer (NULL), m_ise_context_length (0) {
        m_current_ise_name = String (_ ("English Keyboard"));
        m_imcontrol_repository.clear ();
        m_imcontrol_map.clear ();
        m_panel_client_map.clear ();
    }

    ~InfoManagerImpl () {
        delete_ise_context_buffer ();
    }

    void delete_ise_context_buffer (void) {
        if (m_ise_context_buffer != NULL) {
            delete[] m_ise_context_buffer;
            m_ise_context_buffer = NULL;
            m_ise_context_length = 0;
        }
    }

    bool initialize (InfoManager* info_manager, const ConfigPointer& config, const String& display, bool resident) {

        m_config_name = "socket";
        m_display_name = display;

        return m_panel_agent_manager.initialize (info_manager, config, display, resident);
    }

    bool valid (void) const {
        return m_panel_agent_manager.valid ();
    }

public:

    void stop (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::stop ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            String helper_uuid = get_current_helper_uuid ();
            hide_helper (helper_uuid);
            stop_helper (helper_uuid, -2, 0);
        }

        m_panel_agent_manager.stop ();
    }

    TOOLBAR_MODE_T get_current_toolbar_mode () const {
        return m_current_toolbar_mode;
    }

    String get_current_ise_name () const {
        return m_current_ise_name;
    }

    String get_current_helper_uuid () const {
        return m_current_helper_uuid;
    }

    uint32 get_current_helper_option () const {
        return m_current_helper_option;
    }

    void set_current_ise_name (String& name) {
        m_current_ise_name = name;
    }

    void set_current_toolbar_mode (TOOLBAR_MODE_T mode) {
        m_current_toolbar_mode = mode;
    }

    void set_current_helper_option (uint32 option) {
        m_current_helper_option = option;
    }

    void update_panel_event (int cmd, uint32 nType, uint32 nValue) {
        LOGD ("");
        int    focused_client;
        uint32 focused_context;
        get_focused_context (focused_client, focused_context);

        if (focused_client == -1 && m_active_client_id != -1) {
            focused_client  = m_panel_client_map[m_active_client_id];
            focused_context = 0;
        }

        SCIM_DEBUG_MAIN (1) << __func__ << " (" << nType << ", " << nValue << "), client=" << focused_client << "\n";
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.update_panel_event (focused_client, focused_context, cmd, nType, nValue);
        } else {
            std::cerr << __func__ << " focused client is not existed!!!" << "\n";
        }

        if (m_panel_client_map[m_show_request_client_id] != focused_client) {
            client_info = socket_get_client_info (m_panel_client_map[m_show_request_client_id]);

            if (client_info.type == FRONTEND_CLIENT) {
                m_panel_agent_manager.update_panel_event (m_panel_client_map[m_show_request_client_id], 0, cmd, nType, nValue);
                std::cerr << __func__ << " show request client=" << m_panel_client_map[m_show_request_client_id] << "\n";
            } else {
                std::cerr << __func__ << " show request client is not existed!!!" << "\n";
            }
        }
    }

    bool move_preedit_caret (uint32 position) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::move_preedit_caret (" << position << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.update_preedit_caret (client, context, position);
        }

        unlock ();
        return client >= 0;
    }
//useless
#if 0
    bool request_help (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::request_help ()\n";
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.request_help (client, context);
        }

        unlock ();
        return client >= 0;
    }

    bool request_factory_menu (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::request_factory_menu ()\n";
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.request_factory_menu (client, context);
        }

        unlock ();
        return client >= 0;
    }
#endif

    //ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE
    bool reset_keyboard_ise (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (1) << "InfoManager::reset_keyboard_ise ()\n";
        int    client = -1;
        uint32 context = 0;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.reset_keyboard_ise (client, context);
        }

        unlock ();
        return client >= 0;
    }

    bool update_keyboard_ise_list (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (1) << "InfoManager::update_keyboard_ise_list ()\n";
        int    client = -1;
        uint32 context = 0;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.update_keyboard_ise_list (client, context);
        }

        unlock ();
        return client >= 0;
    }

    bool change_factory (const String&  uuid) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::change_factory (" << uuid << ")\n";
        LOGD ("");
        int client;
        uint32 context;
#if 0
        if (scim_global_config_read (SCIM_GLOBAL_CONFIG_PRELOAD_KEYBOARD_ISE, false))
            m_helper_manager.preload_keyboard_ise (uuid);
#endif
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.change_factory (client, context, uuid);
        }

        unlock ();
        return client >= 0;
    }

    bool helper_candidate_show (void) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        int    client;
        uint32 context;
        get_focused_context (client, context);

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                uint32 ctx = get_helper_ic (client, context);
                m_panel_agent_manager.helper_candidate_show (it->second.id, ctx, m_current_helper_uuid);
                return true;
            }
        }

        return false;
    }

    bool helper_candidate_hide (void) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        int    client;
        uint32 context;
        get_focused_context (client, context);

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                uint32 ctx = get_helper_ic (client, context);
                m_panel_agent_manager.helper_candidate_hide (it->second.id, ctx, m_current_helper_uuid);
                return true;
            }
        }

        return false;
    }

    bool candidate_more_window_show (void) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        int    client;
        uint32 context;
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.candidate_more_window_show (client, context);
                return true;
            }
        } else {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ()) {
                    uint32 ctx = get_helper_ic (client, context);
                    m_panel_agent_manager.candidate_more_window_show (it->second.id, ctx);
                    return true;
                }
            }
        }

        return false;
    }

    bool candidate_more_window_hide (void) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        int    client;
        uint32 context;
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.candidate_more_window_hide (client, context);
                return true;
            }
        } else {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ()) {
                    uint32 ctx = get_helper_ic (client, context);
                    m_panel_agent_manager.candidate_more_window_hide (it->second.id, ctx);
                    return true;
                }
            }
        }

        return false;
    }

    bool update_helper_lookup_table (const LookupTable& table) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        int    client;
        uint32 context;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.update_helper_lookup_table (it->second.id, ctx, m_current_helper_uuid, table);
                return true;
            }
        }

        return false;
    }

    bool select_aux (uint32 item) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::select_aux (" << item << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_aux) {
            if (client >= 0) {
                m_panel_agent_manager.select_aux (client, context, item);
            }
        } else {
            helper_select_aux (item);
        }

        unlock ();
        return client >= 0;
    }

    bool select_candidate (uint32 item) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::select_candidate (" << item << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.select_candidate (client, context, item);
            }
        } else {
            helper_select_candidate (item);
        }

        unlock ();
        return client >= 0;
    }

    bool lookup_table_page_up (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::lookup_table_page_up ()\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.lookup_table_page_up (client, context);
            }
        } else {
            helper_lookup_table_page_up ();
        }

        unlock ();
        return client >= 0;
    }

    bool lookup_table_page_down (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::lookup_table_page_down ()\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.lookup_table_page_down (client, context);
            }
        } else {
            helper_lookup_table_page_down ();
        }

        unlock ();
        return client >= 0;
    }

    bool update_lookup_table_page_size (uint32 size) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::update_lookup_table_page_size (" << size << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.update_lookup_table_page_size (client, context, size);
            }
        } else {
            helper_update_lookup_table_page_size (size);
        }

        unlock ();
        return client >= 0;
    }

    bool update_candidate_item_layout (const std::vector<uint32>& row_items) {
        SCIM_DEBUG_MAIN (1) << __func__ << " (" << row_items.size () << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.update_candidate_item_layout (client, context, row_items);
            }
        } else {
            helper_update_candidate_item_layout (row_items);
        }

        unlock ();
        return client >= 0;
    }

    bool select_associate (uint32 item) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::select_associate (" << item << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.select_associate (client, context, item);
        }

        unlock ();
        helper_select_associate (item);
        return client >= 0;
    }

    bool associate_table_page_up (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::associate_table_page_up ()\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.associate_table_page_up (client, context);
        }

        unlock ();
        helper_associate_table_page_up ();
        return client >= 0;
    }

    bool associate_table_page_down (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::associate_table_page_down ()\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.associate_table_page_down (client, context);
        }

        unlock ();
        helper_associate_table_page_down ();
        return client >= 0;
    }

    bool update_associate_table_page_size (uint32 size) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::update_associate_table_page_size (" << size << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.update_associate_table_page_size (client, context, size);
        }

        unlock ();
        helper_update_associate_table_page_size (size);
        return client >= 0;
    }

    bool update_displayed_candidate_number (uint32 size) {
        SCIM_DEBUG_MAIN (1) << __func__ << " (" << size << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.update_displayed_candidate_number (client, context, size);
            }
        } else {
            helper_update_displayed_candidate_number (size);
        }

        unlock ();
        return client >= 0;
    }

    void send_longpress_event (int type, int index) {
        SCIM_DEBUG_MAIN (1) << __func__ << " (" << type << ", " << index << ")\n";
        LOGD ("");
        int    client;
        uint32 context;
        get_focused_context (client, context);

        if (m_is_imengine_candidate) {
            if (client >= 0) {
                m_panel_agent_manager.send_longpress_event (client, context, index);
            }
        } else {
            helper_longpress_candidate (index);
        }
    }

    bool trigger_property (const String&  property) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::trigger_property (" << property << ")\n";
        LOGD ("");
        int client;
        uint32 context;
        lock ();
        get_focused_context (client, context);

        if (client >= 0) {
            m_panel_agent_manager.trigger_property (client, context, property);
        }

        unlock ();
        return client >= 0;
    }

    bool start_helper (const String&  uuid, int client, uint32 context) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::start_helper (" << uuid << ")\n";
        LOGD ("start ISE(%s)", uuid.c_str ());

        if (uuid.length () <= 0)
            return false;

        lock ();
        /*if (m_current_toolbar_mode != TOOLBAR_HELPER_MODE || m_current_helper_uuid.compare (uuid) != 0)*/ {
            SCIM_DEBUG_MAIN (1) << "Run_helper\n";
            m_signal_run_helper (uuid, m_config_name, m_display_name);
        }
        m_current_helper_uuid = uuid;
        unlock ();
        return true;
    }

    bool stop_helper (const String& uuid, int client, uint32 context) {
        char buf[256] = {0};
        snprintf (buf, sizeof (buf), "time:%ld  pid:%d  %s  %s  prepare to stop ISE(%s)\n",
                  time (0), getpid (), __FILE__, __func__, uuid.c_str ());
        LOGD ("prepare to stop ISE(%s)", uuid.c_str ());
        SCIM_DEBUG_MAIN (1) << "InfoManager::stop_helper (" << uuid << ")\n";

        if (uuid.length () <= 0)
            return false;

        lock ();
        uint32 ctx = get_helper_ic (client, context);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            m_ise_exiting = true;
            m_current_helper_uuid = String ("");
            m_panel_agent_manager.exit (it->second.id, ctx);
            SCIM_DEBUG_MAIN (1) << "Stop helper\n";
            ISF_SAVE_LOG ("send SCIM_TRANS_CMD_EXIT message to %s\n", uuid.c_str ());
        }

        unlock ();
        return true;
    }

    void focus_out_helper (const String& uuid, int client, uint32 context) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.focus_out_helper (it->second.id, ctx, uuid);
        }
    }

    void focus_in_helper (const String& uuid, int client, uint32 context) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.focus_in_helper (it->second.id, ctx, uuid);
        }
    }

    bool show_helper (const String& uuid, char* data, size_t& len, uint32 ctx) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            Socket client_socket (it->second.id);
            m_panel_agent_manager.show_helper (it->second.id, ctx, uuid, data, len);
            LOGD ("Send ISM_TRANS_CMD_SHOW_ISE_PANEL message");
            return true;
        }

        LOGW ("Can't find %s", m_current_helper_uuid.c_str ());
        return false;
    }

    void hide_helper (const String& uuid, uint32 ctx = 0) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);
        LOGD ("");

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;

            if (ctx == 0) {
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
            }

            m_panel_agent_manager.hide_helper (it->second.id, ctx, uuid);
            LOGD ("Send ISM_TRANS_CMD_HIDE_ISE_PANEL message");
        }
    }

    bool set_helper_mode (const String& uuid, uint32& mode) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_mode (it->second.id, ctx, uuid, mode);
            return true;
        }

        return false;
    }

    bool set_helper_language (const String& uuid, uint32& language) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_language (it->second.id, ctx, uuid, language);
            return true;
        }

        return false;
    }

    bool set_helper_imdata (const String& uuid, const char* imdata, size_t& len) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_imdata (it->second.id, ctx, uuid, imdata, len);
            return true;
        }

        return false;
    }

    bool set_helper_return_key_type (const String& uuid, int type) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_return_key_type (it->second.id, ctx, uuid, type);
            return true;
        }

        return false;
    }

    bool get_helper_return_key_type (const String& uuid, uint32& type) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.get_helper_return_key_type (it->second.id, ctx, uuid, type);
        }

        return false;
    }

    bool set_helper_return_key_disable (const String& uuid, bool disabled) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client     = -1;
            uint32 context = 0;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_return_key_disable (it->second.id, ctx, uuid, disabled);
            return true;
        }

        return false;
    }

    bool get_helper_return_key_disable (const String& uuid, uint32& disabled) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.get_helper_return_key_disable (it->second.id, ctx, uuid, disabled);
        }

        return false;
    }

    bool set_helper_layout (const String& uuid, uint32& layout) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_layout (it->second.id, ctx, uuid, layout);
            return true;
        }

        return false;
    }

    bool set_helper_input_mode (const String& uuid, uint32& mode) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_input_mode (it->second.id, ctx, uuid, mode);
            return true;
        }

        return false;
    }

    bool set_helper_input_hint (const String& uuid, uint32& hint) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_input_hint (it->second.id, ctx, uuid, hint);
            return true;
        }

        return false;
    }

    bool set_helper_bidi_direction (const String& uuid, uint32& direction) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_bidi_direction (it->second.id, ctx, uuid, direction);
            return true;
        }

        return false;
    }

    bool set_helper_caps_mode (const String& uuid, uint32& mode) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.set_helper_caps_mode (it->second.id, ctx, uuid, mode);
            return true;
        }

        return false;
    }

    bool show_helper_option_window (const String& uuid) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            LOGD ("Send ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW message");
            m_panel_agent_manager.show_helper_option_window (it->second.id, ctx, uuid);
            return true;
        }

        return false;
    }
    //ISM_TRANS_CMD_SHOW_ISF_CONTROL
    void show_isf_panel (int client_id) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::show_isf_panel ()\n";
        m_signal_show_panel ();
    }
    //ISM_TRANS_CMD_HIDE_ISF_CONTROL
    void hide_isf_panel (int client_id) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::hide_isf_panel ()\n";
        m_signal_hide_panel ();
    }

    //ISM_TRANS_CMD_SHOW_ISE_PANEL
    void show_ise_panel (int client_id, uint32 client, uint32 context, char*   data, size_t  len) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::show_ise_panel ()\n";
        LOGD ("");
        String initial_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));
        String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
        LOGD ("prepare to show ISE %d [%s] [%s]", client_id, initial_uuid.c_str (), default_uuid.c_str ());
        bool ret = false;
        m_show_request_client_id = client_id;
        m_active_client_id = client_id;
        SCIM_DEBUG_MAIN (4) << __func__ << " (client:" << client << " context:" << context << ")\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode) {
            uint32 ctx = get_helper_ic (client, context);
            ret = show_helper (m_current_helper_uuid, data, len, ctx);
        }

        /* Save ISE context for ISE panel re-showing */
        if (data && len > 0) {
            delete_ise_context_buffer ();
            m_ise_context_buffer = new char [len];

            if (m_ise_context_buffer) {
                m_ise_context_length = len;
                memcpy (m_ise_context_buffer, data, m_ise_context_length);
            }
        }

        if (ret) {
            m_signal_show_ise ();
        } else {
            m_signal_start_default_ise ();
        }
    }

    //ISM_TRANS_CMD_HIDE_ISE_PANEL
    void hide_ise_panel (int client_id, uint32 client, uint32 context) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::hide_ise_panel ()\n";
        LOGD ("prepare to hide ISE, %d %d", client_id, m_show_request_client_id);
        SCIM_DEBUG_MAIN (4) << __func__ << " (client:" << client << " context:" << context << ")\n";

        if (m_panel_client_map[client_id] == m_current_socket_client || client_id == m_show_request_client_id) {
//            && (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)) {
            int    focused_client;
            uint32 focused_context;
            get_focused_context (focused_client, focused_context);

            if (focused_client == -1 && m_active_client_id != -1) {
                focused_client  = m_panel_client_map[m_active_client_id];
                focused_context = 0;
            }

            m_signal_hide_ise ();
        }

        /* Release ISE context buffer */
        delete_ise_context_buffer ();
    }

    void hide_helper_ise (void) {
#ifdef WAYLAND
        int    focused_client;
        uint32 focused_context;
        get_focused_context (focused_client, focused_context);
        m_panel_agent_manager.hide_helper_ise (focused_client, focused_context);
#else
        m_signal_hide_ise ();
#endif
    }

    void set_default_ise (const DEFAULT_ISE_T& ise) {
        LOGD ("");
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), ise.uuid);
        scim_global_config_flush ();
    }

    void set_should_shared_ise (const bool should_shared_ise) {
        LOGD ("");
        m_should_shared_ise = should_shared_ise;
    }
    //SCIM_TRANS_CMD_PROCESS_KEY_EVENT
    bool process_key_event (KeyEvent& key, _OUT_ uint32& result) {
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            return m_panel_agent_manager.process_key_event (it->second.id, ctx, m_current_helper_uuid, key, result);
        }

        return false;
    }

    //ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT
    bool process_input_device_event(int client_id, uint32 type, const char *data, size_t len, _OUT_ uint32& result) {
        SCIM_DEBUG_MAIN(4) << "InfoManager::process_input_device_event ()\n";
        HelperClientIndex::iterator it = m_helper_client_index.find(m_current_helper_uuid);

        if (it != m_helper_client_index.end()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context(client, context);
            ctx = get_helper_ic(client, context);
            return m_panel_agent_manager.process_input_device_event(it->second.id, ctx, m_current_helper_uuid, type, data, len, result);
        }

        return false;
    }

    bool get_helper_geometry (String& uuid, struct rectinfo& info) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            return m_panel_agent_manager.get_helper_geometry (it->second.id, ctx, uuid, info);
        }

        return false;
    }

    bool get_helper_imdata (String& uuid, char** imdata, size_t& len) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.get_helper_imdata (it->second.id, ctx, uuid, imdata, len);
            return true;
        }

        return false;
    }

    bool get_helper_layout (String& uuid, uint32& layout) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.get_helper_layout (it->second.id, ctx, uuid, layout);
            return true;
        }

        return false;
    }
    //ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY
    void get_input_panel_geometry (int client_id, _OUT_ struct rectinfo& info) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        m_signal_get_input_panel_geometry (info);
    }
    //ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
    void get_candidate_window_geometry (int client_id, _OUT_ struct rectinfo& info) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        m_signal_get_candidate_geometry (info);
    }
    //ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE
    void get_ise_language_locale (int client_id, _OUT_ char** data, _OUT_ size_t& len) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        Transaction trans;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                m_panel_agent_manager.get_ise_language_locale (it->second.id, ctx, m_current_helper_uuid, data, len);
            }
        }
    }

    void get_current_ise_geometry (rectinfo& rect) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << __func__ << " \n";
        bool           ret  = false;

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_geometry (m_current_helper_uuid, rect);

        if (!ret) {
            rect.pos_x  = 0;
            rect.pos_y  = 0;
            rect.width  = 0;
            rect.height = 0;
        }
    }

    void set_ise_mode (int client_id, uint32 mode) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_mode ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_mode (m_current_helper_uuid, mode);
    }
    //ISM_TRANS_CMD_SET_LAYOUT
    void set_ise_layout (int client_id, uint32 layout) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_layout ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_layout (m_current_helper_uuid, layout);
    }
    //ISM_TRANS_CMD_SET_INPUT_MODE
    void set_ise_input_mode (int client_id, uint32 input_mode) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_input_mode ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_input_mode (m_current_helper_uuid, input_mode);
    }
    //ISM_TRANS_CMD_SET_INPUT_HINT
    void set_ise_input_hint (int client_id, uint32 input_hint) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_input_hint ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_input_hint (m_current_helper_uuid, input_hint);
    }
    //ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION
    void update_ise_bidi_direction (int client_id, uint32 bidi_direction) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::update_ise_bidi_direction ()\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_bidi_direction (m_current_helper_uuid, bidi_direction);
    }
    //ISM_TRANS_CMD_SET_ISE_LANGUAGE
    void set_ise_language (int client_id, uint32 language) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_language ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_language (m_current_helper_uuid, language);
    }
    //ISM_TRANS_CMD_SET_ISE_IMDATA
    void set_ise_imdata (int client_id, const char*   imdata, size_t  len) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_imdata ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_imdata (m_current_helper_uuid, imdata, len);
    }
    //ISM_TRANS_CMD_GET_ISE_IMDATA
    bool get_ise_imdata (int client_id, _OUT_ char** imdata, _OUT_ size_t& len) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_ise_imdata ()\n";
        bool    ret    = false;
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_imdata (m_current_helper_uuid, imdata, len);

        return ret;
    }
    //ISM_TRANS_CMD_GET_LAYOUT
    bool get_ise_layout (int client_id, _OUT_ uint32& layout) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_ise_layout ()\n";
        bool   ret = false;
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_layout (m_current_helper_uuid, layout);

        return ret;
    }
    //ISM_TRANS_CMD_GET_ISE_STATE
    void get_ise_state (int client_id, _OUT_ int& state) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        m_signal_get_ise_state (state);
    }
    //ISM_TRANS_CMD_GET_ACTIVE_ISE
    void get_active_ise (int client_id, _OUT_ String& default_uuid) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
    }
    //ISM_TRANS_CMD_GET_ISE_LIST
    void get_ise_list (int client_id, _OUT_ std::vector<String>& strlist) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_ise_list ()\n";
        m_signal_get_ise_list (strlist);
    }
    //ISM_TRANS_CMD_GET_ALL_HELPER_ISE_INFO
    void get_all_helper_ise_info (int client_id, _OUT_ HELPER_ISE_INFO& info) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_all_helper_ise_info ()\n";
        LOGD ("");
        m_signal_get_all_helper_ise_info (info);

        //1 Check if the current IME's option (setting) is available or not.
        for (uint32 i = 0; i < info.appid.size (); i++) {
            if (m_current_helper_uuid.compare (info.appid [i]) == 0) {  // Find the current IME
                // If the current IME's "has_option" is unknown (-1), get it through ISM_TRANS_CMD_CHECK_OPTION_WINDOW command.
                // And it's saved to ime_info DB. Then next time this will be skipped.
                if (info.has_option [i] >= 2) {
                    HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                    if (it != m_helper_client_index.end ()) {
                        int    client;
                        uint32 context;
                        get_focused_context (client, context);
                        uint32 ctx = get_helper_ic (client, context);
                        uint32 avail = static_cast<uint32> (-1);
                        m_panel_agent_manager.check_option_window (it->second.id, ctx, m_current_helper_uuid, avail);

                        if (avail < 2) {
                            info.has_option [i] = avail;
                            // Update "has_option" column of ime_info DB and global variable.
                            m_signal_set_has_option_helper_ise_info (info.appid [i], static_cast<bool> (avail));
                        }
                    }
                }
            }
        }
    }
    //ISM_TRANS_CMD_SET_ENABLE_HELPER_ISE_INFO
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void set_enable_helper_ise_info (int client_id, String appid, uint32 is_enabled) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_enable_helper_ise_info ()\n";
        m_signal_set_enable_helper_ise_info (appid, static_cast<bool> (is_enabled));
    }
    //ISM_TRANS_CMD_SHOW_HELPER_ISE_LIST
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void show_helper_ise_list (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::show_helper_ise_list ()\n";
        LOGD ("");
        m_signal_show_helper_ise_list ();
    }
    //ISM_TRANS_CMD_SHOW_HELPER_ISE_SELECTOR
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void show_helper_ise_selector (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::show_helper_ise_selector ()\n";
        LOGD ("");
        m_signal_show_helper_ise_selector ();
    }
    //ISM_TRANS_CMD_IS_HELPER_ISE_ENABLED
    //reply
    void is_helper_ise_enabled (int client_id, String strAppid, _OUT_ uint32& nEnabled) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        int _nEnabled = 0;
        m_signal_is_helper_ise_enabled (strAppid, _nEnabled);
        nEnabled = (uint32)_nEnabled;
    }
    //ISM_TRANS_CMD_GET_ISE_INFORMATION
    void get_ise_information (int client_id, String strUuid, _OUT_ String& strName, _OUT_ String& strLanguage,
                              _OUT_ int& nType, _OUT_ int& nOption, _OUT_ String& strModuleName) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        m_signal_get_ise_information (strUuid, strName, strLanguage, nType, nOption, strModuleName);
    }
    /*
     * useless
     */
    void get_language_list (int client_id, _OUT_ std::vector<String>& strlist) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_language_list ()\n";
        m_signal_get_language_list (strlist);
    }

    void get_all_language (int client_id, _OUT_ std::vector<String>& strlist) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_all_language ()\n";
        m_signal_get_all_language (strlist);
    }
    /*
     * useless
     */
    void get_ise_language (int client_id, char* buf, size_t len, _OUT_ std::vector<String>& strlist) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::get_ise_language ()\n";
        m_signal_get_ise_language (buf, strlist);
    }
    //ISM_TRANS_CMD_RESET_ISE_OPTION
    //reply SCIM_TRANS_CMD_OK
    bool reset_ise_option (int client_id) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::resect_ise_option ()\n";
        LOGD ("");
        int    client = -1;
        uint32 context;
        lock ();
        ClientContextUUIDRepository::iterator it = m_client_context_uuids.begin ();

        if (it != m_client_context_uuids.end ()) {
            get_imengine_client_context (it->first, client, context);
        }

        if (client >= 0) {
            m_panel_agent_manager.reset_ise_option (client, context);
        }

        unlock ();
        return client >= 0;
    }

    bool find_active_ise_by_uuid (String uuid) {
        HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();

        for (; iter != m_helper_info_repository.end (); iter++) {
            if (!uuid.compare (iter->second.uuid))
                return true;
        }

        return false;
    }
    //ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID
    //reply
    bool set_active_ise_by_uuid (int client_id, char*  buf, size_t  len) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_active_ise_by_uuid ()\n";
        LOGD ("");

        if (buf == NULL) {
            return false;
        }

        String uuid (buf);
        ISE_INFO info;

        if (!m_signal_get_ise_info_by_uuid (uuid, info)) {
            return false;
        }

        if (info.type == TOOLBAR_KEYBOARD_MODE) {
            m_signal_set_active_ise_by_uuid (uuid, 1);
            return true;
        } else {
            m_signal_set_active_ise_by_uuid (uuid, 1);
        }

        sync ();

        if (find_active_ise_by_uuid (uuid)) {
            return true;
        } else {
            m_ise_pending_repository[uuid] = client_id;
            return false;
        }
    }
    //ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void set_initial_ise_by_uuid (int client_id, char*  buf, size_t  len) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_initial_ise_by_uuid ()\n";
        LOGD ("");

        if (buf == NULL) {
            return;
        }

        String uuid (buf);
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (uuid));
        scim_global_config_flush ();
    }
    //ISM_TRANS_CMD_SET_RETURN_KEY_TYPE
    void set_ise_return_key_type (int client_id, uint32 type) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_return_key_type (m_current_helper_uuid, type);
    }
    //ISM_TRANS_CMD_GET_RETURN_KEY_TYPE
    //reply
    bool get_ise_return_key_type (int client_id, _OUT_ uint32& type) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        bool   ret  = false;
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            ret = get_helper_return_key_type (m_current_helper_uuid, type);

        return ret;
    }
    //ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE
    void set_ise_return_key_disable (int client_id, uint32 disabled) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_return_key_disable (m_current_helper_uuid, disabled);
    }
    //ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE
    void get_ise_return_key_disable (int client_id, _OUT_ uint32& disabled) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            get_helper_return_key_disable (m_current_helper_uuid, disabled);
    }

    void reset_helper_context (const String& uuid) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int client;
            uint32 context;
            uint32 ctx;
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.reset_helper_context (it->second.id, ctx, uuid);
        }
    }
    /*
     * useless
     */
    void reset_ise_context (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::reset_ise_context ()\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            reset_helper_context (m_current_helper_uuid);
    }
    //ISM_TRANS_CMD_SET_CAPS_MODE
    void set_ise_caps_mode (int client_id, uint32 mode) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_ise_caps_mode ()\n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            set_helper_caps_mode (m_current_helper_uuid, mode);
    }

    //SCIM_TRANS_CMD_RELOAD_CONFIG
    void reload_config (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::reload_config ()\n";
        LOGD ("");
        lock ();

        for (ClientRepository::iterator it = m_client_repository.begin (); it != m_client_repository.end (); ++it) {
            if (it->second.type == FRONTEND_CLIENT || it->second.type == HELPER_CLIENT) {
                m_panel_agent_manager.reload_config (it->first);
            }
        }

        unlock ();
    }

    bool exit (void) {
        SCIM_DEBUG_MAIN (1) << "InfoManager::exit ()\n";
        lock ();

        for (ClientRepository::iterator it = m_client_repository.begin (); it != m_client_repository.end (); ++it) {
            m_panel_agent_manager.exit (it->first, 0);
        }

        unlock ();
        stop ();
        return true;
    }

    void update_ise_list (std::vector<String>& strList) {
        /* request PanelClient to update keyboard ise list */
        update_keyboard_ise_list ();
    }
    //ISM_TRANS_CMD_SEND_WILL_SHOW_ACK
    void will_show_ack (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::will_show_ack ()\n";
        LOGD ("");
        m_signal_will_show_ack ();
    }
    //ISM_TRANS_CMD_SEND_WILL_HIDE_ACK
    void will_hide_ack (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::will_hide_ack ()\n";
        LOGD ("");
        m_signal_will_hide_ack ();
    }
    //ISM_TRANS_CMD_RESET_DEFAULT_ISE
    void reset_default_ise (int client_id) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        String initial_ise = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));

        if (initial_ise.length () > 0)
            m_signal_set_active_ise_by_uuid (initial_ise, 1);
        else
            std::cerr << "Read SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID is failed!!!\n";
    }
    //ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE
    void set_keyboard_mode (int client_id, uint32 mode) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::set_keyboard_mode ()\n";
        m_signal_set_keyboard_mode (mode);
    }
    //ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK
    void candidate_will_hide_ack (int client_id) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "\n";
        LOGD ("");
        m_signal_candidate_will_hide_ack ();
    }

    //ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION
    void get_active_helper_option (int client_id, _OUT_ uint32& option) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        option = get_current_helper_option ();
    }
    //ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW
    void show_ise_option_window (int client_id) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::show_ise_panel ()\n";
        LOGD ("");
        String initial_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_INITIAL_ISE_UUID), String (""));
        String default_uuid = scim_global_config_read (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), String (""));
        LOGD ("prepare to show ISE option window %d [%s] [%s]", client_id, initial_uuid.c_str (), default_uuid.c_str ());

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode) {
            show_helper_option_window (m_current_helper_uuid);
        }
    }

    //ISM_TRANS_CMD_EXPAND_CANDIDATE
    void expand_candidate () {
        LOGD ("");
        m_signal_expand_candidate ();
    }

    //ISM_TRANS_CMD_CONTRACT_CANDIDATE
    void contract_candidate () {
        LOGD ("");
        m_signal_contract_candidate ();
    }
    //ISM_TRANS_CMD_GET_RECENT_ISE_GEOMETRY
    void get_recent_ise_geometry (int client_id, uint32 angle, _OUT_ struct rectinfo& info) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        m_signal_get_recent_ise_geometry (angle, info);
    }

    bool check_privilege_by_sockfd (int client_id, const String& privilege) {
        return m_signal_check_privilege_by_sockfd (client_id, privilege);
    }

    Connection signal_connect_turn_on (InfoManagerSlotVoid*                slot) {
        return m_signal_turn_on.connect (slot);
    }

    Connection signal_connect_turn_off (InfoManagerSlotVoid*                slot) {
        return m_signal_turn_off.connect (slot);
    }

    Connection signal_connect_show_panel (InfoManagerSlotVoid*                slot) {
        return m_signal_show_panel.connect (slot);
    }

    Connection signal_connect_hide_panel (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_panel.connect (slot);
    }

    Connection signal_connect_update_screen (InfoManagerSlotInt*                 slot) {
        return m_signal_update_screen.connect (slot);
    }

    Connection signal_connect_update_spot_location (InfoManagerSlotIntIntInt*           slot) {
        return m_signal_update_spot_location.connect (slot);
    }

    Connection signal_connect_update_factory_info (InfoManagerSlotFactoryInfo*         slot) {
        return m_signal_update_factory_info.connect (slot);
    }

    Connection signal_connect_start_default_ise (InfoManagerSlotVoid*                slot) {
        return m_signal_start_default_ise.connect (slot);
    }

    Connection signal_connect_stop_default_ise (InfoManagerSlotVoid*                slot) {
        return m_signal_stop_default_ise.connect (slot);
    }

    Connection signal_connect_set_candidate_ui (InfoManagerSlotIntInt*              slot) {
        return m_signal_set_candidate_ui.connect (slot);
    }

    Connection signal_connect_get_candidate_ui (InfoManagerSlotIntInt2*             slot) {
        return m_signal_get_candidate_ui.connect (slot);
    }

    Connection signal_connect_set_candidate_position (InfoManagerSlotIntInt*              slot) {
        return m_signal_set_candidate_position.connect (slot);
    }

    Connection signal_connect_get_candidate_geometry (InfoManagerSlotRect*                slot) {
        return m_signal_get_candidate_geometry.connect (slot);
    }

    Connection signal_connect_get_input_panel_geometry (InfoManagerSlotRect*                slot) {
        return m_signal_get_input_panel_geometry.connect (slot);
    }

    Connection signal_connect_set_keyboard_ise (InfoManagerSlotString*              slot) {
        return m_signal_set_keyboard_ise.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise (InfoManagerSlotString2*             slot) {
        return m_signal_get_keyboard_ise.connect (slot);
    }

    Connection signal_connect_show_help (InfoManagerSlotString*              slot) {
        return m_signal_show_help.connect (slot);
    }

    Connection signal_connect_show_factory_menu (InfoManagerSlotFactoryInfoVector*   slot) {
        return m_signal_show_factory_menu.connect (slot);
    }

    Connection signal_connect_show_preedit_string (InfoManagerSlotVoid*                slot) {
        return m_signal_show_preedit_string.connect (slot);
    }

    Connection signal_connect_show_aux_string (InfoManagerSlotVoid*                slot) {
        return m_signal_show_aux_string.connect (slot);
    }

    Connection signal_connect_show_lookup_table (InfoManagerSlotVoid*                slot) {
        return m_signal_show_lookup_table.connect (slot);
    }

    Connection signal_connect_show_associate_table (InfoManagerSlotVoid*                slot) {
        return m_signal_show_associate_table.connect (slot);
    }

    Connection signal_connect_hide_preedit_string (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_preedit_string.connect (slot);
    }

    Connection signal_connect_hide_aux_string (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_aux_string.connect (slot);
    }

    Connection signal_connect_hide_lookup_table (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_lookup_table.connect (slot);
    }

    Connection signal_connect_hide_associate_table (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_associate_table.connect (slot);
    }

    Connection signal_connect_update_preedit_string (InfoManagerSlotAttributeStringInt*  slot) {
        return m_signal_update_preedit_string.connect (slot);
    }

    Connection signal_connect_update_preedit_caret (InfoManagerSlotInt*                 slot) {
        return m_signal_update_preedit_caret.connect (slot);
    }

    Connection signal_connect_update_aux_string (InfoManagerSlotAttributeString*     slot) {
        return m_signal_update_aux_string.connect (slot);
    }

    Connection signal_connect_update_lookup_table (InfoManagerSlotLookupTable*         slot) {
        return m_signal_update_lookup_table.connect (slot);
    }

    Connection signal_connect_update_associate_table (InfoManagerSlotLookupTable*         slot) {
        return m_signal_update_associate_table.connect (slot);
    }

    Connection signal_connect_register_properties (InfoManagerSlotPropertyList*        slot) {
        return m_signal_register_properties.connect (slot);
    }

    Connection signal_connect_update_property (InfoManagerSlotProperty*            slot) {
        return m_signal_update_property.connect (slot);
    }

    Connection signal_connect_register_helper_properties (InfoManagerSlotIntPropertyList*     slot) {
        return m_signal_register_helper_properties.connect (slot);
    }

    Connection signal_connect_update_helper_property (InfoManagerSlotIntProperty*         slot) {
        return m_signal_update_helper_property.connect (slot);
    }

    Connection signal_connect_register_helper (InfoManagerSlotIntHelperInfo*       slot) {
        return m_signal_register_helper.connect (slot);
    }

    Connection signal_connect_remove_helper (InfoManagerSlotInt*                 slot) {
        return m_signal_remove_helper.connect (slot);
    }

    Connection signal_connect_set_active_ise_by_uuid (InfoManagerSlotStringBool*          slot) {
        return m_signal_set_active_ise_by_uuid.connect (slot);
    }

    Connection signal_connect_focus_in (InfoManagerSlotVoid*                   slot) {
        return m_signal_focus_in.connect (slot);
    }

    Connection signal_connect_focus_out (InfoManagerSlotVoid*                   slot) {
        return m_signal_focus_out.connect (slot);
    }

    Connection signal_connect_expand_candidate (InfoManagerSlotVoid*                   slot) {
        return m_signal_expand_candidate.connect (slot);
    }

    Connection signal_connect_contract_candidate (InfoManagerSlotVoid*                   slot) {
        return m_signal_contract_candidate.connect (slot);
    }

    Connection signal_connect_select_candidate (InfoManagerSlotInt*                    slot) {
        return m_signal_select_candidate.connect (slot);
    }

    Connection signal_connect_get_ise_list (InfoManagerSlotBoolStringVector*       slot) {
        return m_signal_get_ise_list.connect (slot);
    }

    Connection signal_connect_get_all_helper_ise_info (InfoManagerSlotBoolHelperInfo*       slot) {
        return m_signal_get_all_helper_ise_info.connect (slot);
    }

    Connection signal_connect_set_has_option_helper_ise_info (InfoManagerSlotStringBool*          slot) {
        return m_signal_set_has_option_helper_ise_info.connect (slot);
    }

    Connection signal_connect_set_enable_helper_ise_info (InfoManagerSlotStringBool*          slot) {
        return m_signal_set_enable_helper_ise_info.connect (slot);
    }

    Connection signal_connect_show_helper_ise_list (InfoManagerSlotVoid*                slot) {
        return m_signal_show_helper_ise_list.connect (slot);
    }

    Connection signal_connect_show_helper_ise_selector (InfoManagerSlotVoid*                slot) {
        return m_signal_show_helper_ise_selector.connect (slot);
    }

    Connection signal_connect_is_helper_ise_enabled (InfoManagerSlotStringInt*                slot) {
        return m_signal_is_helper_ise_enabled.connect (slot);
    }

    Connection signal_connect_get_ise_information (InfoManagerSlotBoolString4int2*        slot) {
        return m_signal_get_ise_information.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise_list (InfoManagerSlotBoolStringVector*       slot) {
        return m_signal_get_keyboard_ise_list.connect (slot);
    }

    Connection signal_connect_update_ise_geometry (InfoManagerSlotIntIntIntInt*           slot) {
        return m_signal_update_ise_geometry.connect (slot);
    }

    Connection signal_connect_get_language_list (InfoManagerSlotStringVector*           slot) {
        return m_signal_get_language_list.connect (slot);
    }

    Connection signal_connect_get_all_language (InfoManagerSlotStringVector*           slot) {
        return m_signal_get_all_language.connect (slot);
    }

    Connection signal_connect_get_ise_language (InfoManagerSlotStrStringVector*        slot) {
        return m_signal_get_ise_language.connect (slot);
    }

    Connection signal_connect_get_ise_info_by_uuid (InfoManagerSlotStringISEINFO*          slot) {
        return m_signal_get_ise_info_by_uuid.connect (slot);
    }

    Connection signal_connect_send_key_event (InfoManagerSlotKeyEvent*               slot) {
        return m_signal_send_key_event.connect (slot);
    }

    Connection signal_connect_accept_connection (InfoManagerSlotInt*                    slot) {
        return m_signal_accept_connection.connect (slot);
    }

    Connection signal_connect_close_connection (InfoManagerSlotInt*                    slot) {
        return m_signal_close_connection.connect (slot);
    }

    Connection signal_connect_exit (InfoManagerSlotVoid*                   slot) {
        return m_signal_exit.connect (slot);
    }

    Connection signal_connect_transaction_start (InfoManagerSlotVoid*                   slot) {
        return m_signal_transaction_start.connect (slot);
    }

    Connection signal_connect_transaction_end (InfoManagerSlotVoid*                   slot) {
        return m_signal_transaction_end.connect (slot);
    }

    Connection signal_connect_lock (InfoManagerSlotVoid*                   slot) {
        return m_signal_lock.connect (slot);
    }

    Connection signal_connect_unlock (InfoManagerSlotVoid*                   slot) {
        return m_signal_unlock.connect (slot);
    }

    Connection signal_connect_update_input_context (InfoManagerSlotIntInt*                 slot) {
        return m_signal_update_input_context.connect (slot);
    }

    Connection signal_connect_show_ise (InfoManagerSlotVoid*                slot) {
        return m_signal_show_ise.connect (slot);
    }

    Connection signal_connect_hide_ise (InfoManagerSlotVoid*                slot) {
        return m_signal_hide_ise.connect (slot);
    }

    Connection signal_connect_will_show_ack (InfoManagerSlotVoid*                slot) {
        return m_signal_will_show_ack.connect (slot);
    }

    Connection signal_connect_will_hide_ack (InfoManagerSlotVoid*                slot) {
        return m_signal_will_hide_ack.connect (slot);
    }

    Connection signal_connect_set_keyboard_mode (InfoManagerSlotInt*                slot) {
        return m_signal_set_keyboard_mode.connect (slot);
    }

    Connection signal_connect_candidate_will_hide_ack (InfoManagerSlotVoid*                slot) {
        return m_signal_candidate_will_hide_ack.connect (slot);
    }

    Connection signal_connect_get_ise_state (InfoManagerSlotInt2*                slot) {
        return m_signal_get_ise_state.connect (slot);
    }

    Connection signal_connect_run_helper (InfoManagerSlotString3*                slot)
    {
        return m_signal_run_helper.connect (slot);
    }

    Connection signal_connect_get_recent_ise_geometry (InfoManagerSlotIntRect*                slot) {
        return m_signal_get_recent_ise_geometry.connect (slot);
    }

    Connection signal_connect_check_privilege_by_sockfd  (InfoManagerSlotIntString2* slot)
    {
        return m_signal_check_privilege_by_sockfd.connect (slot);
    }

    //ISM_TRANS_CMD_REGISTER_PANEL_CLIENT
    void register_panel_client (uint32 client_id, uint32 id) {
        LOGD ("");
        m_panel_client_map [client_id] = (int)id;
    }
    //SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT
    void register_input_context (uint32 client_id, uint32 context, String  uuid) {
        LOGD ("");
        uint32 ctx = get_helper_ic (m_panel_client_map[client_id], context);
        m_client_context_uuids [ctx] = uuid;
    }
    //SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT
    void remove_input_context (uint32 client_id, uint32 context) {
        LOGD ("");
        uint32 ctx = get_helper_ic (m_panel_client_map[client_id], context);
        m_client_context_uuids.erase (ctx);

        if (ctx == get_helper_ic (m_current_socket_client, m_current_client_context)) {
            lock ();
            m_current_socket_client  = m_last_socket_client;
            m_current_client_context = m_last_client_context;
            m_current_context_uuid   = m_last_context_uuid;
            m_last_socket_client     = -1;
            m_last_client_context    = 0;
            m_last_context_uuid      = String ("");

            if (m_current_socket_client == -1) {
                unlock ();
                socket_update_control_panel ();
            } else {
                unlock ();
            }
        } else if (ctx == get_helper_ic (m_last_socket_client, m_last_client_context)) {
            lock ();
            m_last_socket_client  = -1;
            m_last_client_context = 0;
            m_last_context_uuid   = String ("");
            unlock ();
        }

        if (m_client_context_uuids.size () == 0)
            m_signal_stop_default_ise ();
    }

    //SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT
    void socket_reset_input_context (int client_id, uint32 context) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_reset_input_context \n";
        LOGD ("");

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
            socket_reset_helper_input_context (m_current_helper_uuid, m_panel_client_map[client_id], context);
    }

    //SCIM_TRANS_CMD_FOCUS_IN
    void focus_in (int client_id, uint32 context,  String  uuid) {
        LOGD ("");
        m_signal_focus_in ();
        focus_in_helper (m_current_helper_uuid, m_panel_client_map[client_id], context);
        SCIM_DEBUG_MAIN (2) << "PanelAgent::focus_in (" << client_id << "," << context << "," << uuid << ")\n";
        m_active_client_id = client_id;
        lock ();

        if (m_current_socket_client >= 0) {
            m_last_socket_client  = m_current_socket_client;
            m_last_client_context = m_current_client_context;
            m_last_context_uuid   = m_current_context_uuid;
        }

        m_current_socket_client  = m_panel_client_map[client_id];
        m_current_client_context = context;
        m_current_context_uuid   = uuid;
        unlock ();
    }

    //SCIM_TRANS_CMD_FOCUS_OUT
    void focus_out (int client_id, uint32 context) {
        LOGD ("");
        m_signal_focus_out ();
        lock ();
        focus_out_helper (m_current_helper_uuid, m_panel_client_map[client_id], context);

        if (m_current_socket_client >= 0) {
            m_last_socket_client  = m_current_socket_client;
            m_last_client_context = m_current_client_context;
            m_last_context_uuid   = m_current_context_uuid;
        }

        m_current_socket_client  = -1;
        m_current_client_context = 0;
        m_current_context_uuid   = String ("");
        unlock ();
    }

    //ISM_TRANS_CMD_TURN_ON_LOG
    void socket_turn_on_log (uint32 isOn) {
        LOGD ("");

        if (isOn) {
            DebugOutput::enable_debug (SCIM_DEBUG_AllMask);
            DebugOutput::set_verbose_level (7);
            SCIM_DEBUG_MAIN (4) << __func__ << " on\n";
        } else {
            SCIM_DEBUG_MAIN (4) << __func__ << " off\n";
            DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
            DebugOutput::set_verbose_level (0);
        }

        int     focused_client;
        uint32  focused_context;
        get_focused_context (focused_client, focused_context);

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                //uint32 ctx = get_helper_ic (focused_client, focused_context);
                //FIXME
                //m_panel_agent_manager.socket_turn_on_log (it->second.id, ctx, m_current_helper_uuid, isOn);
            }
        }

        if (focused_client == -1) {
            std::cerr << __func__ << " get_focused_context is failed!!!\n";
            return;
        }

        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            //FIXME
            //m_panel_agent_manager.socket_turn_on_log (focused_client, focused_context, isOn);
        }
    }


    void add_client (int client_id, uint32 key, ClientType type) {
        LOGD ("id:%d, key:%d, type:%d", client_id, key, type);
        ClientInfo info;
        info.key = key;
        info.type = type;
        SCIM_DEBUG_MAIN (4) << "Add client to repository. Type=" << type << " key=" << key << "\n";
        lock ();
        m_client_repository [client_id] = info;

        if (info.type == IMCONTROL_ACT_CLIENT) {
            m_pending_active_imcontrol_id = client_id;
        } else if (info.type == IMCONTROL_CLIENT) {
            m_imcontrol_map [m_pending_active_imcontrol_id] = client_id;
            m_pending_active_imcontrol_id = -1;
        }

        LOGD ("%d clients connecting", m_client_repository.size());

        unlock ();
    }

    void del_client (int client_id) {
        LOGD ("id:%d", client_id);
        lock ();
        m_signal_close_connection (client_id);
        ClientInfo client_info = socket_get_client_info (client_id);
        m_client_repository.erase (client_id);
#ifdef PANEL_SERVER_AUTO_EXIT
        /* Exit panel if there is no connected client anymore. */
        if (client_info.type != UNKNOWN_CLIENT && m_client_repository.size () == 0 && !m_should_resident) {
            SCIM_DEBUG_MAIN (4) << "Exit Socket Server Thread.\n";
            server->shutdown ();
            m_signal_exit.emit ();
        }
#endif
        unlock ();

        if (client_info.type == FRONTEND_CLIENT) {
            SCIM_DEBUG_MAIN (4) << "It's a FrontEnd client.\n";

            /* The focused client is closed. */
            if (m_current_socket_client == client_id) {
                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT)
                    hide_helper (m_current_helper_uuid);

                lock ();
                m_current_socket_client = -1;
                m_current_client_context = 0;
                m_current_context_uuid = String ("");
                unlock ();
                socket_transaction_start ();
                socket_turn_off ();
                socket_transaction_end ();
            }

            if (m_last_socket_client == client_id) {
                lock ();
                m_last_socket_client = -1;
                m_last_client_context = 0;
                m_last_context_uuid = String ("");
                unlock ();
            }

            /* Erase all associated Client Context UUIDs. */
            std::vector <uint32> ctx_list;
            ClientContextUUIDRepository::iterator it = m_client_context_uuids.begin ();

            for (; it != m_client_context_uuids.end (); ++it) {
                if ((it->first & 0xFFFF) == (client_id & 0xFFFF))
                    ctx_list.push_back (it->first);
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                m_client_context_uuids.erase (ctx_list [i]);

            /* Erase all helperise info associated with the client */
            ctx_list.clear ();
            it = m_client_context_helper.begin ();

            for (; it != m_client_context_helper.end (); ++it) {
                if ((it->first & 0xFFFF) == (client_id & 0xFFFF)) {
                    ctx_list.push_back (it->first);
                    /* similar to stop_helper except that it will not call get_focused_context() */
                    String uuid = it->second;

                    if (m_helper_uuid_count.find (uuid) != m_helper_uuid_count.end ()) {
                        uint32 count = m_helper_uuid_count[uuid];

                        if (1 == count) {
                            m_helper_uuid_count.erase (uuid);
                            HelperClientIndex::iterator pise = m_helper_client_index.find (uuid);

                            if (pise != m_helper_client_index.end ()) {
                                stop_helper (uuid, pise->second.id, it->first);
                            }

                            SCIM_DEBUG_MAIN (1) << "Stop HelperISE " << uuid << " ...\n";
                        } else {
                            m_helper_uuid_count[uuid] = count - 1;
                            focus_out_helper (uuid, (it->first & 0xFFFF), ((it->first >> 16) & 0x7FFF));
                            SCIM_DEBUG_MAIN (1) << "Decrement usage count of HelperISE " << uuid
                                                << " to " << m_helper_uuid_count[uuid] << "\n";
                        }
                    }
                }
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                m_client_context_helper.erase (ctx_list [i]);

            HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();

            for (; iter != m_helper_info_repository.end (); iter++) {
                if (!m_current_helper_uuid.compare (iter->second.uuid))
                    if (! (iter->second.option & ISM_ISE_HIDE_IN_CONTROL_PANEL))
                        socket_update_control_panel ();
            }
        } else if (client_info.type == FRONTEND_ACT_CLIENT) {
            SCIM_DEBUG_MAIN (4) << "It's a FRONTEND_ACT_CLIENT client.\n";
            IntIntRepository::iterator iter2 = m_panel_client_map.find (client_id);

            if (iter2 != m_panel_client_map.end ())
                m_panel_client_map.erase (iter2);
        } else if (client_info.type == HELPER_CLIENT) {
            SCIM_DEBUG_MAIN (4) << "It's a Helper client.\n";
            lock ();
            HelperInfoRepository::iterator hiit = m_helper_info_repository.find (client_id);

            if (hiit != m_helper_info_repository.end ()) {
                bool restart = false;
                String uuid = hiit->second.uuid;
                HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

                if ((hiit->second.option & SCIM_HELPER_AUTO_RESTART) &&
                    (it != m_helper_client_index.end () && it->second.ref > 0))
                    restart = true;

                m_helper_client_index.erase (uuid);
                m_helper_info_repository.erase (hiit);

                if (restart && !m_ise_exiting) {
                    struct tms     tiks_buf;
                    static clock_t start_tiks = times (&tiks_buf);
                    static double  clock_tiks = (double)sysconf (_SC_CLK_TCK);
                    clock_t curr_tiks = times (&tiks_buf);
                    double  secs = (double) (curr_tiks - start_tiks) / clock_tiks;
                    //LOGE ("time second:%f\n", secs);
                    static String  restart_uuid;

                    if (restart_uuid != uuid || secs > MIN_REPEAT_TIME) {
                        m_signal_run_helper (uuid, m_config_name, m_display_name);
                        restart_uuid = uuid;
                        LOGE ("Auto restart soft ISE:%s", uuid.c_str ());
                    } else {
                        reset_default_ise (0);
                        ISF_SAVE_LOG ("Auto restart is abnormal, reset default ISE\n");
                    }

                    start_tiks = curr_tiks;
                }
            }

            m_ise_exiting = false;
            unlock ();
            socket_transaction_start ();
            m_signal_remove_helper (client_id);
            socket_transaction_end ();
        } else if (client_info.type == HELPER_ACT_CLIENT) {
            SCIM_DEBUG_MAIN (4) << "It's a Helper passive client.\n";
            lock ();
            HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client_id);

            if (hiit != m_helper_active_info_repository.end ())
                m_helper_active_info_repository.erase (hiit);

            unlock ();
        } else if (client_info.type == IMCONTROL_ACT_CLIENT) {
            SCIM_DEBUG_MAIN (4) << "It's a IMCONTROL_ACT_CLIENT client.\n";
            IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);

            if (iter != m_imcontrol_repository.end ()) {
                int size = iter->second.info.size ();
                int i = 0;

                while (i < size) {
                    stop_helper ((iter->second.info)[i].uuid, (iter->second.count)[i], DEFAULT_CONTEXT_VALUE);

                    if ((iter->second.info)[i].option & ISM_ISE_HIDE_IN_CONTROL_PANEL)
                        m_current_helper_uuid = m_last_helper_uuid;

                    i++;
                }

                m_imcontrol_repository.erase (iter);
            }

            IntIntRepository::iterator iter2 = m_imcontrol_map.find (client_id);

            if (iter2 != m_imcontrol_map.end ())
                m_imcontrol_map.erase (iter2);
        } else if (client_info.type == CONFIG_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a CONFIG_CLIENT client.\n";
        }
        LOGI ("clients: %d, panel clients: %d, imcontrols size: %d, helper infos: %d, \
helper active infos: %d, helper client indexs: %d, ises pending: %d, \
imcontrols: %d, start helper ic indexs: %d, client context uuids: %d, \
client context helpers: %d, helpers uuid count: %d",
               m_client_repository.size(),
               m_panel_client_map.size(),
               m_imcontrol_map.size(),
               m_helper_info_repository.size(),
               m_helper_active_info_repository.size(),
               m_helper_client_index.size(),
               m_ise_pending_repository.size(),
               m_imcontrol_repository.size(),
               m_start_helper_ic_index.size(),
               m_client_context_uuids.size(),
               m_client_context_helper.size(),
               m_helper_uuid_count.size());
    }

    const ClientInfo& socket_get_client_info (int client) {
        static ClientInfo null_client = { 0, UNKNOWN_CLIENT };
        ClientRepository::iterator it = m_client_repository.find (client);

        if (it != m_client_repository.end ())
            return it->second;

        return null_client;
    }

    //SCIM_TRANS_CMD_PANEL_TURN_ON
    void socket_turn_on (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_turn_on ()\n";
        LOGD ("");
        m_signal_turn_on ();
    }
    //SCIM_TRANS_CMD_PANEL_TURN_OFF
    void socket_turn_off (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_turn_off ()\n";
        LOGD ("");
        m_signal_turn_off ();
    }
    //SCIM_TRANS_CMD_UPDATE_SCREEN
    void socket_update_screen (int client_id, uint32 num) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_screen ()\n";
        LOGD ("");

        if (((int) num) != m_current_screen) {
            SCIM_DEBUG_MAIN (4) << "New Screen number = " << num << "\n";
            m_signal_update_screen ((int) num);
            helper_all_update_screen ((int) num);
            m_current_screen = (num);
        }
    }
    //SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
    void socket_update_spot_location (uint32 x, uint32 y, uint32 top_y) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_spot_location ()\n";
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "New Spot location x=" << x << " y=" << y << "\n";
        m_signal_update_spot_location ((int)x, (int)y, (int)top_y);
        helper_all_update_spot_location ((int)x, (int)y);
    }
    //ISM_TRANS_CMD_UPDATE_CURSOR_POSITION
    void socket_update_cursor_position (uint32 cursor_pos) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_cursor_position ()\n";
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "New cursor position pos=" << cursor_pos << "\n";
        helper_all_update_cursor_position ((int)cursor_pos);
    }
    //ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT
    void socket_update_surrounding_text (String text, uint32 cursor) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            lock ();
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_update_surrounding_text (it->second.id, ctx, m_current_helper_uuid, text, cursor);
            unlock ();
        }
    }
    //ISM_TRANS_CMD_UPDATE_SELECTION
    void socket_update_selection (String text) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << "...\n";
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            uint32 ctx;
            lock ();
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_update_selection (it->second.id, ctx, m_current_helper_uuid, text);
            unlock ();
        }
    }
    //SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO
    void socket_update_factory_info (PanelFactoryInfo& info) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_factory_info ()\n";
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "New Factory info uuid=" << info.uuid << " name=" << info.name << "\n";
        info.lang = scim_get_normalized_language (info.lang);
        m_signal_update_factory_info (info);
    }
    //SCIM_TRANS_CMD_PANEL_SHOW_HELP
    void socket_show_help (String help) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_help ()\n";
        LOGD ("");
        m_signal_show_help (help);
    }
    //SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU
    void socket_show_factory_menu (std::vector <PanelFactoryInfo>& vec) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_factory_menu ()\n";
        LOGD ("");

        if (vec.size ())
            m_signal_show_factory_menu (vec);
    }


    //SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
    void socket_show_preedit_string (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_preedit_string ()\n";
        m_signal_show_preedit_string ();
    }
    //SCIM_TRANS_CMD_SHOW_AUX_STRING
    void socket_show_aux_string (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_aux_string ()\n";
        m_signal_show_aux_string ();
    }
    //SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE
    void socket_show_lookup_table (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_lookup_table ()\n";
        m_signal_show_lookup_table ();
    }
    //ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE
    void socket_show_associate_table (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_show_associate_table ()\n";
        m_signal_show_associate_table ();
    }
    //SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
    void socket_hide_preedit_string (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_hide_preedit_string ()\n";
        m_signal_hide_preedit_string ();
    }
    //SCIM_TRANS_CMD_HIDE_AUX_STRING
    void socket_hide_aux_string (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_hide_aux_string ()\n";
        m_signal_hide_aux_string ();
    }
    //SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE
    void socket_hide_lookup_table (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_hide_lookup_table ()\n";
        m_signal_hide_lookup_table ();
    }
    //ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE
    void socket_hide_associate_table (void) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_hide_associate_table ()\n";
        m_signal_hide_associate_table ();
    }
    //SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
    void socket_update_preedit_string (String& str, const AttributeList& attrs, uint32 caret) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_preedit_string ()\n";
        LOGD ("");
        m_signal_update_preedit_string (str, attrs, (int) caret);
    }
    //SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
    void socket_update_preedit_caret (uint32 caret) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_preedit_caret ()\n";
        LOGD ("");
        m_signal_update_preedit_caret ((int) caret);
    }
    //SCIM_TRANS_CMD_UPDATE_AUX_STRING
    void socket_update_aux_string (String& str, const AttributeList& attrs) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_aux_string ()\n";
        LOGD ("");
        m_signal_update_aux_string (str, attrs);
        m_is_imengine_aux = true;
    }
    //SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE
    void socket_update_lookup_table (const LookupTable& table) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_lookup_table ()\n";
        LOGD ("");
        //FIXME:
        //g_isf_candidate_table = _isf_candidate_table;
        m_signal_update_lookup_table (table);
        m_is_imengine_candidate = true;
    }
    //ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE
    void socket_update_associate_table (const LookupTable& table) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_associate_table ()\n";
        LOGD ("");
        m_signal_update_associate_table (table);
    }

    void socket_update_control_panel (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_control_panel ()\n";
        String name, uuid;
        m_signal_get_keyboard_ise (name, uuid);
        PanelFactoryInfo info;

        if (name.length () > 0)
            info = PanelFactoryInfo (uuid, name, String (""), String (""));
        else
            info = PanelFactoryInfo (String (""), String (_ ("English Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));

        m_signal_update_factory_info (info);
    }
    //SCIM_TRANS_CMD_REGISTER_PROPERTIES
    void socket_register_properties (const PropertyList& properties) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_register_properties ()\n";
        LOGD ("");
        m_signal_register_properties (properties);
    }
    //SCIM_TRANS_CMD_UPDATE_PROPERTY
    void socket_update_property (const Property& property) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_update_property ()\n";
        LOGD ("");
        m_signal_update_property (property);
    }
    //ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST
    void socket_get_keyboard_ise_list (String& uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_get_keyboard_ise_list ()\n";
        LOGD ("");
        std::vector<String> list;
        list.clear ();
        m_signal_get_keyboard_ise_list (list);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            get_focused_context (client, context);
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_get_keyboard_ise_list (it->second.id, ctx, uuid, list);
        }
    }
    //ISM_TRANS_CMD_SET_CANDIDATE_UI
    void socket_set_candidate_ui (uint32 portrait_line, uint32 mode) {
        SCIM_DEBUG_MAIN (4) << __func__ << " \n";
        LOGD ("");
        m_signal_set_candidate_ui (portrait_line, mode);
    }
    //ISM_TRANS_CMD_GET_CANDIDATE_UI
    void socket_get_candidate_ui (String uuid) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_get_candidate_ui ()\n";
        int style = 0, mode = 0;
        m_signal_get_candidate_ui (style, mode);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            get_focused_context (client, context);
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_get_candidate_ui (it->second.id, ctx, uuid, style, mode);
        }
    }
    //ISM_TRANS_CMD_SET_CANDIDATE_POSITION
    void socket_set_candidate_position (uint32 left, uint32 top) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_set_candidate_position ()\n";
        m_signal_set_candidate_position (left, top);
    }
    //ISM_TRANS_CMD_HIDE_CANDIDATE
    void socket_hide_candidate (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_hide_candidate ()\n";
        LOGD ("");
        m_signal_hide_preedit_string ();
        m_signal_hide_aux_string ();
        m_signal_hide_lookup_table ();
        m_signal_hide_associate_table ();
    }
    //ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
    void socket_get_candidate_geometry (String& uuid) {
        LOGD ("");
        SCIM_DEBUG_MAIN (4) << __func__ << " \n";
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            struct rectinfo info = {0, 0, 0, 0};
            m_signal_get_candidate_geometry (info);
            int    client;
            uint32 context;
            get_focused_context (client, context);
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_get_candidate_geometry (it->second.id, ctx, uuid, info);
        }
    }
    //ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID
    void socket_set_keyboard_ise (String uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_set_keyboard_ise ()\n";
        LOGD ("");
        m_signal_set_keyboard_ise (uuid);
    }
    //ISM_TRANS_CMD_SELECT_CANDIDATE
    void socket_helper_select_candidate (uint32 index) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_set_keyboard_ise ()\n";
        LOGD ("");
        m_signal_select_candidate (index);
    }
    //ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY
    void socket_helper_update_ise_geometry (int client, uint32 x, uint32 y, uint32 width, uint32 height) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";
        LOGD ("");
        HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);

        if (it != m_helper_active_info_repository.end ()) {
            if (it->second.uuid == m_current_helper_uuid)
                m_signal_update_ise_geometry (x, y, width, height);
        }
    }
    //ISM_TRANS_CMD_GET_KEYBOARD_ISE
    void socket_get_keyboard_ise (String uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_get_keyboard_ise ()\n";
        LOGD ("");
        String ise_name, ise_uuid;
        int    client  = -1;
        uint32 context = 0;
        uint32 ctx     = 0;
        get_focused_context (client, context);
        ctx = get_helper_ic (client, context);

        if (m_client_context_uuids.find (ctx) != m_client_context_uuids.end ())
            ise_uuid = m_client_context_uuids[ctx];

        m_signal_get_keyboard_ise (ise_name, ise_uuid);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);
            m_panel_agent_manager.socket_get_keyboard_ise (it->second.id, ctx, uuid, ise_name, ise_uuid);
        }
    }

    //SCIM_TRANS_CMD_START_HELPER
    void socket_start_helper (int client_id, uint32 context, String uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_start_helper ()\n";
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        lock ();
        uint32 ic = get_helper_ic (m_panel_client_map[client_id], context);
        String ic_uuid;
        /* Get the context uuid from the client context registration table. */
        {
            ClientContextUUIDRepository::iterator it = m_client_context_uuids.find (get_helper_ic (m_panel_client_map[client_id], context));

            if (it != m_client_context_uuids.end ())
                ic_uuid = it->second;
        }

        if (m_current_socket_client == m_panel_client_map[client_id] && m_current_client_context == context) {
            if (!ic_uuid.length ()) ic_uuid = m_current_context_uuid;
        } else if (m_last_socket_client == m_panel_client_map[client_id] && m_last_client_context == context) {
            if (!ic_uuid.length ()) ic_uuid = m_last_context_uuid;
        }

        if (it == m_helper_client_index.end ()) {
            SCIM_DEBUG_MAIN (5) << "Run this Helper.\n";
            m_start_helper_ic_index [uuid].push_back (std::make_pair (ic, ic_uuid));
            m_signal_run_helper (uuid, m_config_name, m_display_name);
        } else {
            SCIM_DEBUG_MAIN (5) << "Increase the Reference count.\n";
            m_panel_agent_manager.socket_start_helper (it->second.id, ic, ic_uuid);
            ++ it->second.ref;
        }

        unlock ();
    }
    //SCIM_TRANS_CMD_STOP_HELPER
    void socket_stop_helper (int client_id, uint32 context, String uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_stop_helper ()\n";
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        lock ();
        uint32 ic = get_helper_ic (m_panel_client_map[client_id], context);

        if (it != m_helper_client_index.end ()) {
            SCIM_DEBUG_MAIN (5) << "Decrase the Reference count.\n";
            -- it->second.ref;
            String ic_uuid;
            /* Get the context uuid from the client context registration table. */
            {
                ClientContextUUIDRepository::iterator it = m_client_context_uuids.find (get_helper_ic (m_panel_client_map[client_id], context));

                if (it != m_client_context_uuids.end ())
                    ic_uuid = it->second;
            }

            if (m_current_socket_client == m_panel_client_map[client_id] && m_current_client_context == context) {
                if (!ic_uuid.length ()) ic_uuid = m_current_context_uuid;
            } else if (m_last_socket_client == m_panel_client_map[client_id] && m_last_client_context == context) {
                if (!ic_uuid.length ()) ic_uuid = m_last_context_uuid;
            }

            m_panel_agent_manager.helper_detach_input_context (it->second.id, ic, ic_uuid);

            if (it->second.ref <= 0)
                m_panel_agent_manager.exit (it->second.id, ic);
        }

        unlock ();
    }
    //SCIM_TRANS_CMD_SEND_HELPER_EVENT
    void socket_send_helper_event (int client_id, uint32 context, String uuid, const Transaction& _nest_trans) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_send_helper_event ()\n";
        LOGD ("");
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {
            String ic_uuid;
            /* Get the context uuid from the client context registration table. */
            {
                ClientContextUUIDRepository::iterator it = m_client_context_uuids.find (get_helper_ic (m_panel_client_map[client_id], context));

                if (it != m_client_context_uuids.end ())
                    ic_uuid = it->second;
            }

            if (m_current_socket_client == m_panel_client_map[client_id] && m_current_client_context == context) {
                if (!ic_uuid.length ()) ic_uuid = m_current_context_uuid;
            } else if (m_last_socket_client == m_panel_client_map[client_id] && m_last_client_context == context) {
                if (!ic_uuid.length ()) ic_uuid = m_last_context_uuid;
            }

            m_panel_agent_manager.helper_process_imengine_event (it->second.id, get_helper_ic (m_panel_client_map[client_id], context), ic_uuid, _nest_trans);
        }
    }

    //SCIM_TRANS_CMD_REGISTER_PROPERTIES
    void socket_helper_register_properties (int client, PropertyList& properties) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_register_properties (" << client << ")\n";
        LOGD ("");
        m_signal_register_helper_properties (client, properties);

#if 0 //why? remove if useless, infinite loop
        /* Check whether application is already focus_in */
        if (m_current_socket_client != -1) {
            SCIM_DEBUG_MAIN (2) << "Application is already focus_in!\n";
            focus_in_helper (m_current_helper_uuid, m_current_socket_client, m_current_client_context);
            reset_keyboard_ise ();
        }

        /* Check whether ISE panel is already shown */
        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode && m_ise_context_length > 0) {
            SCIM_DEBUG_MAIN (2) << "Re-show ISE panel!\n";
            int    focused_client;
            uint32 focused_context;
            get_focused_context (focused_client, focused_context);

            if (focused_client == -1 && m_active_client_id != -1) {
                focused_client  = m_panel_client_map[m_active_client_id];
                focused_context = 0;
            }

            uint32 ctx = get_helper_ic (focused_client, focused_context);
            bool ret = show_helper (m_current_helper_uuid, m_ise_context_buffer, m_ise_context_length, ctx);

            if (ret)
                m_signal_show_ise ();
        }
#endif
    }
    //SCIM_TRANS_CMD_UPDATE_PROPERTY
    void socket_helper_update_property (int client, Property& property) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_update_property (" << client << ")\n";
        LOGD ("");
        m_signal_update_helper_property (client, property);
    }
    //SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT
    void socket_helper_send_imengine_event (int client, uint32 target_ic, String target_uuid , Transaction& nest_trans) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_send_imengine_event (" << client << ")\n";
        LOGD ("");
        HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client);

        if (hiit != m_helper_active_info_repository.end ()) {
            int     target_client;
            uint32  target_context;
            get_imengine_client_context (target_ic, target_client, target_context);
            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;
            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            ClientInfo  client_info = socket_get_client_info (target_client);
            SCIM_DEBUG_MAIN (5) << "Target UUID = " << target_uuid << "  Focused UUId = " << focused_uuid << "\nTarget Client = " << target_client << "\n";

            if (client_info.type == FRONTEND_CLIENT) {
                m_panel_agent_manager.process_helper_event (target_client, target_context, target_uuid, hiit->second.uuid, nest_trans);
            }
        }
    }

    void socket_helper_key_event_op (int client, int cmd, uint32 target_ic, String& target_uuid, KeyEvent& key) {
        if (!key.empty ()) {
            int     target_client;
            uint32  target_context;
            get_imengine_client_context (target_ic, target_client, target_context);
            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;
            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client == -1) {
                /* FIXUP: monitor 'Invalid Window' error */
                LOGW ("focused target client is NULL");
            } else if (target_client == focused_client && target_context == focused_context && target_uuid == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);

                if (client_info.type == FRONTEND_CLIENT) {
                    m_panel_agent_manager.socket_helper_key_event (target_client, target_context, cmd, key);
                }
            }
        }
    }
    //SCIM_TRANS_CMD_PROCESS_KEY_EVENT
    //SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT
    void socket_helper_send_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_send_key_event (" << client << ")\n";
        ISF_PROF_DEBUG ("first message")
        LOGD ("");
        socket_helper_key_event_op (client, SCIM_TRANS_CMD_PROCESS_KEY_EVENT, target_ic, target_uuid, key);
    }
    //SCIM_TRANS_CMD_FORWARD_KEY_EVENT
    void socket_helper_forward_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_forward_key_event (" << client << ")\n";
        LOGD ("");
        socket_helper_key_event_op (client, SCIM_TRANS_CMD_FORWARD_KEY_EVENT, target_ic, target_uuid, key);
    }

    //SCIM_TRANS_CMD_COMMIT_STRING
    void socket_helper_commit_string (int client, uint32 target_ic, String target_uuid, WideString wstr) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_commit_string (" << client << ")\n";
        LOGD ("");

        if (wstr.length ()) {
            int     target_client;
            uint32  target_context;
            get_imengine_client_context (target_ic, target_client, target_context);
            int     focused_client;
            uint32  focused_context;
            String  focused_uuid;
            focused_uuid = get_focused_context (focused_client, focused_context);

            if (target_ic == (uint32) (-1)) {
                target_client  = focused_client;
                target_context = focused_context;
            }

            if (target_uuid.length () == 0)
                target_uuid = focused_uuid;

            if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid) {
                ClientInfo client_info = socket_get_client_info (target_client);

                if (client_info.type == FRONTEND_CLIENT) {
                    m_panel_agent_manager.commit_string (target_client, target_context, wstr);
                } else {
                    std::cerr << "target client is not existed!!!" << "\n";
                }
            }
        }
    }
    //SCIM_TRANS_CMD_GET_SURROUNDING_TEXT
    void socket_helper_get_surrounding_text (int client, String uuid, uint32 maxlen_before, uint32 maxlen_after, const int fd) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.socket_helper_get_surrounding_text (focused_client, focused_context, maxlen_before, maxlen_after, fd);
        }
    }
    //SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT
    void socket_helper_delete_surrounding_text (int client, uint32 offset, uint32 len) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.socket_helper_delete_surrounding_text (focused_client, focused_context, offset, len);
        }
    }
    //SCIM_TRANS_CMD_GET_SELECTION
    void socket_helper_get_selection (int client, String uuid, const int fd) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.socket_helper_get_selection (focused_client, focused_context, fd);
        }
    }
    //SCIM_TRANS_CMD_SET_SELECTION
    void socket_helper_set_selection (int client, uint32 start, uint32 end) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.socket_helper_set_selection (focused_client, focused_context, start, end);
        }
    }

    //SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
    void socket_helper_show_preedit_string (int client, uint32 target_ic, String target_uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_show_preedit_string (" << client << ")\n";
        LOGD ("");
        int     target_client;
        uint32  target_context;
        get_imengine_client_context (target_ic, target_client, target_context);
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);

        if (target_ic == (uint32) (-1)) {
            target_client  = focused_client;
            target_context = focused_context;
        }

        if (target_uuid.length () == 0)
            target_uuid = focused_uuid;

        if (target_client  == focused_client &&
            target_context == focused_context &&
            target_uuid    == focused_uuid) {
            ClientInfo client_info = socket_get_client_info (target_client);

            if (client_info.type == FRONTEND_CLIENT) {
                m_panel_agent_manager.show_preedit_string (target_client, target_context);
            }
        }
    }
    //SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
    void socket_helper_hide_preedit_string (int client, uint32 target_ic, String target_uuid) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_hide_preedit_string (" << client << ")\n";
        LOGD ("");
        int     target_client;
        uint32  target_context;
        get_imengine_client_context (target_ic, target_client, target_context);
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);

        if (target_ic == (uint32) (-1)) {
            target_client  = focused_client;
            target_context = focused_context;
        }

        if (target_uuid.length () == 0)
            target_uuid = focused_uuid;

        if (target_client  == focused_client &&
            target_context == focused_context &&
            target_uuid    == focused_uuid) {
            ClientInfo client_info = socket_get_client_info (target_client);

            if (client_info.type == FRONTEND_CLIENT) {
                m_panel_agent_manager.hide_preedit_string (target_client, target_context);
            }
        }
    }
    //SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
    void socket_helper_update_preedit_string (int client, uint32 target_ic, String target_uuid, WideString wstr, AttributeList& attrs, uint32 caret) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_update_preedit_string (" << client << ")\n";
        LOGD ("");
        int     target_client;
        uint32  target_context;
        get_imengine_client_context (target_ic, target_client, target_context);
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid;
        focused_uuid = get_focused_context (focused_client, focused_context);

        if (target_ic == (uint32) (-1)) {
            target_client  = focused_client;
            target_context = focused_context;
        }

        if (target_uuid.length () == 0)
            target_uuid = focused_uuid;

        if (target_client  == focused_client &&
            target_context == focused_context &&
            target_uuid    == focused_uuid) {
            ClientInfo client_info = socket_get_client_info (target_client);

            if (client_info.type == FRONTEND_CLIENT) {
                m_panel_agent_manager.update_preedit_string (target_client, target_context, wstr, attrs, caret);
            }
        }
    }
    //SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
    void socket_helper_update_preedit_caret (int client, uint32 caret) {
        SCIM_DEBUG_MAIN (4) << __func__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid;
        focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.update_preedit_caret (focused_client, focused_context, caret);
        }
    }

    //SCIM_TRANS_CMD_PANEL_REGISTER_HELPER
    void socket_helper_register_helper (int client, HelperInfo& info) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_register_helper (" << client << ")\n";
        bool result = false;
        lock ();
        LOGD ("");

        if (info.uuid.length ()) {
            SCIM_DEBUG_MAIN (4) << "New Helper uuid=" << info.uuid << " name=" << info.name << "\n";
            HelperClientIndex::iterator it = m_helper_client_index.find (info.uuid);

            if (it == m_helper_client_index.end ()) {
                m_helper_info_repository [client] = info;
                m_helper_client_index [info.uuid] = HelperClientStub (client, 1);
                StartHelperICIndex::iterator icit = m_start_helper_ic_index.find (info.uuid);

                if (icit != m_start_helper_ic_index.end ()) {
                    m_panel_agent_manager.helper_attach_input_context_and_update_screen (client, icit->second, m_current_screen);
                    m_start_helper_ic_index.erase (icit);
                }

                result = true;
            }
        }

        unlock ();

        if (result)
            m_signal_register_helper (client, info);
    }

    //SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER
    void socket_helper_register_helper_passive (int client, HelperInfo& info) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_register_helper_passive (" << client << ")\n";
        LOGD ("");
        lock ();

        if (info.uuid.length ()) {
            SCIM_DEBUG_MAIN (4) << "New Helper Passive uuid=" << info.uuid << " name=" << info.name << "\n";
            HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);

            if (it == m_helper_active_info_repository.end ()) {
                m_helper_active_info_repository[client] =  info;
            }

            StringIntRepository::iterator iter = m_ise_pending_repository.find (info.uuid);

            if (iter != m_ise_pending_repository.end ()) {
                m_ise_pending_repository.erase (iter);
            }

            iter = m_ise_pending_repository.find (info.name);

            if (iter != m_ise_pending_repository.end ()) {
                m_ise_pending_repository.erase (iter);
            }
        }

        unlock ();
    }
    //ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT
    void socket_helper_update_input_context (int client, uint32 type, uint32 value) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::socket_helper_update_input_context (" << client << ")\n";
        LOGD ("");
        m_signal_update_input_context ((int)type, (int)value);
        int    focused_client;
        uint32 focused_context;
        get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.update_ise_input_context (focused_client, focused_context, type, value);
        } else {
            std::cerr << "focused client is not existed!!!" << "\n";
        }
    }
    //SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND
    void socket_helper_send_private_command (int client, String command) {
        SCIM_DEBUG_MAIN (4) << __FUNCTION__ << " (" << client << ")\n";
        LOGD ("");
        int     focused_client;
        uint32  focused_context;
        String  focused_uuid = get_focused_context (focused_client, focused_context);
        ClientInfo client_info = socket_get_client_info (focused_client);

        if (client_info.type == FRONTEND_CLIENT) {
            m_panel_agent_manager.send_private_command (focused_client, focused_context, command);
        }
    }
    //ISM_TRANS_CMD_UPDATE_ISE_EXIT
    //void UPDATE_ISE_EXIT (int client) {
    //    LOGD ("");
    //    HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client);

    //    if (hiit != m_helper_active_info_repository.end ()) {
    //        String l_uuid = hiit->second.uuid;
    //        HelperClientIndex::iterator it = m_helper_client_index.find (l_uuid);

    //        if (it != m_helper_client_index.end ()) {
    //            del_client (it->second.id);
    //        }
    //    }

    //    del_client (client);
    //}

    void socket_reset_helper_input_context (const String& uuid, int client, uint32 context) {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            uint32 ctx = get_helper_ic (client, context);
            m_panel_agent_manager.reset_helper_context (it->second.id, ctx, uuid);
        }
    }

    bool helper_select_aux (uint32 item) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_select_aux \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.select_aux (it->second.id, ctx, item);
                return true;
            }
        }

        return false;
    }

    bool helper_select_candidate (uint32 item) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_select_candidate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.select_candidate (it->second.id, ctx, item);
                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_up (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_lookup_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.lookup_table_page_up (it->second.id, ctx);
                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_down (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_lookup_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.lookup_table_page_down (it->second.id, ctx);
                return true;
            }
        }

        return false;
    }

    bool helper_update_lookup_table_page_size (uint32 size) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_update_lookup_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.update_lookup_table_page_size (it->second.id, ctx, size);
                return true;
            }
        }

        return false;
    }

    bool helper_update_candidate_item_layout (const std::vector<uint32>& row_items) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.update_candidate_item_layout (it->second.id, ctx, row_items);
                return true;
            }
        }

        return false;
    }

    bool helper_select_associate (uint32 item) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_select_associate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.select_associate (it->second.id, ctx, item);
                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_up (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_associate_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.associate_table_page_up (it->second.id, ctx);
                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_down (void) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_associate_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.associate_table_page_down (it->second.id, ctx);
                return true;
            }
        }

        return false;
    }

    bool helper_update_associate_table_page_size (uint32         size) {
        SCIM_DEBUG_MAIN (4) << "InfoManager::helper_update_associate_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.update_associate_table_page_size (it->second.id, ctx, size);
                return true;
            }
        }

        return false;
    }

    bool helper_update_displayed_candidate_number (uint32 size) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.update_displayed_candidate_number (it->second.id, ctx, size);
                return true;
            }
        }

        return false;
    }

    bool helper_longpress_candidate (uint32 index) {
        SCIM_DEBUG_MAIN (4) << __func__ << "\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode || m_current_helper_option & ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT) {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ()) {
                int    client;
                uint32 context;
                uint32 ctx;
                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);
                m_panel_agent_manager.send_longpress_event (it->second.id, ctx, index);
                return true;
            }
        }

        std::cerr << __func__ << " is failed!!!\n";
        return false;
    }

    void helper_all_update_spot_location (int x, int y) {
        SCIM_DEBUG_MAIN (5) << "InfoManager::helper_all_update_spot_location (" << x << "," << y << ")\n";
        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();
        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SPOT_LOCATION_INFO) {
                m_panel_agent_manager.helper_all_update_spot_location (hiit->first, get_helper_ic (client, context), uuid, x, y);
            }
        }
    }

    void helper_all_update_cursor_position (int cursor_pos) {
        SCIM_DEBUG_MAIN (5) << "InfoManager::helper_all_update_cursor_position (" << cursor_pos << ")\n";
        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();
        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            m_panel_agent_manager.helper_all_update_cursor_position (hiit->first, get_helper_ic (client, context), uuid, cursor_pos);
        }
    }

    void helper_all_update_screen (int screen) {
        SCIM_DEBUG_MAIN (5) << "InfoManager::helper_all_update_screen (" << screen << ")\n";
        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();
        int    client;
        uint32 context;
        String uuid;
        uuid = get_focused_context (client, context);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SCREEN_INFO) {
                m_panel_agent_manager.helper_all_update_screen (hiit->first, get_helper_ic (client, context), uuid, screen);
            }
        }
    }

    const String& get_focused_context (int& client, uint32& context, bool force_last_context = false) const {
        if (m_current_socket_client >= 0) {
            client  = m_current_socket_client;
            context = m_current_client_context;
            return m_current_context_uuid;
        } else {
            client  = m_last_socket_client;
            context = m_last_client_context;
            return m_last_context_uuid;
        }
    }

private:
    void socket_transaction_start (void) {
        m_signal_transaction_start ();
    }

    void socket_transaction_end (void) {
        m_signal_transaction_end ();
    }

    void lock (void) {
        m_signal_lock ();
    }
    void unlock (void) {
        m_signal_unlock ();
    }
};

InfoManager::InfoManager ()
    : m_impl (new InfoManagerImpl ())
{
}

InfoManager::~InfoManager ()
{
    delete m_impl;
}

bool
InfoManager::initialize (InfoManager* info_manager, const ConfigPointer& config, const String& display, bool resident)
{
    return m_impl->initialize (info_manager, config, display, resident);
}

bool
InfoManager::valid (void) const
{
    return m_impl->valid ();
}

void
InfoManager::stop (void)
{
    if (m_impl != NULL)
        m_impl->stop ();
}

const ClientInfo&
InfoManager::socket_get_client_info (int client) const
{
    return m_impl->socket_get_client_info (client);
}

void InfoManager::hide_helper (const String& uuid)
{
    m_impl->hide_helper (uuid);
}
TOOLBAR_MODE_T
InfoManager::get_current_toolbar_mode () const
{
    return m_impl->get_current_toolbar_mode ();
}

void
InfoManager::get_current_ise_geometry (rectinfo& rect)
{
    m_impl->get_current_ise_geometry (rect);
}

String
InfoManager::get_current_helper_uuid () const
{
    return m_impl->get_current_helper_uuid ();
}

String
InfoManager::get_current_ise_name () const
{
    return m_impl->get_current_ise_name ();
}

void
InfoManager::set_current_toolbar_mode (TOOLBAR_MODE_T mode)
{
    m_impl->set_current_toolbar_mode (mode);
}

void
InfoManager::set_current_ise_name (String& name)
{
    m_impl->set_current_ise_name (name);
}

void
InfoManager::set_current_helper_option (uint32 option)
{
    m_impl->set_current_helper_option (option);
}

void
InfoManager::update_candidate_panel_event (uint32 nType, uint32 nValue)
{
    m_impl->update_panel_event (ISM_TRANS_CMD_UPDATE_ISF_CANDIDATE_PANEL, nType, nValue);
}

void
InfoManager::update_input_panel_event (uint32 nType, uint32 nValue)
{
    m_impl->update_panel_event (ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT, nType, nValue);
}

bool
InfoManager::move_preedit_caret (uint32         position)
{
    return m_impl->move_preedit_caret (position);
}

//useless
#if 0
bool
InfoManager::request_help (void)
{
    return m_impl->request_help ();
}

bool
InfoManager::request_factory_menu (void)
{
    return m_impl->request_factory_menu ();
}
#endif

bool
InfoManager::change_factory (const String&  uuid)
{
    return m_impl->change_factory (uuid);
}

bool
InfoManager::helper_candidate_show (void)
{
    return m_impl->helper_candidate_show ();
}

bool
InfoManager::helper_candidate_hide (void)
{
    return m_impl->helper_candidate_hide ();
}

bool
InfoManager::candidate_more_window_show (void)
{
    return m_impl->candidate_more_window_show ();
}

bool
InfoManager::candidate_more_window_hide (void)
{
    return m_impl->candidate_more_window_hide ();
}

bool
InfoManager::update_helper_lookup_table (const LookupTable& table)
{
    return m_impl->update_helper_lookup_table (table);
}

bool
InfoManager::select_aux (uint32         item)
{
    return m_impl->select_aux (item);
}

bool
InfoManager::select_candidate (uint32         item)
{
    return m_impl->select_candidate (item);
}

bool
InfoManager::lookup_table_page_up (void)
{
    return m_impl->lookup_table_page_up ();
}

bool
InfoManager::lookup_table_page_down (void)
{
    return m_impl->lookup_table_page_down ();
}

bool
InfoManager::update_lookup_table_page_size (uint32         size)
{
    return m_impl->update_lookup_table_page_size (size);
}

bool
InfoManager::update_candidate_item_layout (const std::vector<uint32>& row_items)
{
    return m_impl->update_candidate_item_layout (row_items);
}

bool
InfoManager::select_associate (uint32         item)
{
    return m_impl->select_associate (item);
}

bool
InfoManager::associate_table_page_up (void)
{
    return m_impl->associate_table_page_up ();
}

bool
InfoManager::associate_table_page_down (void)
{
    return m_impl->associate_table_page_down ();
}

bool
InfoManager::update_associate_table_page_size (uint32         size)
{
    return m_impl->update_associate_table_page_size (size);
}

bool
InfoManager::update_displayed_candidate_number (uint32        size)
{
    return m_impl->update_displayed_candidate_number (size);
}

void
InfoManager::send_longpress_event (int type, int index)
{
    m_impl->send_longpress_event (type, index);
}

bool
InfoManager::trigger_property (const String&  property)
{
    return m_impl->trigger_property (property);
}

bool
InfoManager::start_helper (const String&  uuid)
{
    return m_impl->start_helper (uuid, -2, 0);
}

bool
InfoManager::stop_helper (const String&  uuid)
{
    return m_impl->stop_helper (uuid, -2, 0);
}

void
InfoManager::set_default_ise (const DEFAULT_ISE_T&  ise)
{
    m_impl->set_default_ise (ise);
}

void
InfoManager::set_should_shared_ise (const bool should_shared_ise)
{
    m_impl->set_should_shared_ise (should_shared_ise);
}

//void
//InfoManager::reload_config                  (void)
//{
//    m_impl->reload_config ();
//}

bool
InfoManager::exit (void)
{
    return m_impl->exit ();
}
void
InfoManager::update_ise_list (std::vector<String>& strList)
{
    m_impl->update_ise_list (strList);
}

/////////////////////////////////Message function begin/////////////////////////////////////////

//ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE
bool InfoManager:: reset_keyboard_ise (void)
{
    return m_impl->reset_keyboard_ise ();
}

//ISM_TRANS_CMD_SHOW_ISF_CONTROL
void InfoManager::show_isf_panel (int client_id)
{
    m_impl->show_isf_panel (client_id);
}

//ISM_TRANS_CMD_HIDE_ISF_CONTROL
void InfoManager::hide_isf_panel (int client_id)
{
    m_impl->hide_isf_panel (client_id);
}

//ISM_TRANS_CMD_SHOW_ISE_PANEL
void InfoManager::show_ise_panel (int client_id, uint32 client, uint32 context, char*   data, size_t  len)
{
    m_impl->show_ise_panel (client_id, client, context, data, len);
}

//ISM_TRANS_CMD_HIDE_ISE_PANEL
void InfoManager::hide_ise_panel (int client_id, uint32 client, uint32 context)
{
    m_impl->hide_ise_panel (client_id, client, context);
}

//ISM_TRANS_CMD_HIDE_ISE_PANEL from ISF control
void InfoManager::hide_helper_ise (void)
{
    m_impl->hide_helper_ise ();
}

//SCIM_TRANS_CMD_PROCESS_KEY_EVENT
bool InfoManager::process_key_event (KeyEvent& key, _OUT_ uint32& result)
{
    return m_impl->process_key_event (key, result);
}

//ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY
void InfoManager::get_input_panel_geometry (int client_id, _OUT_ struct rectinfo& info)
{
    m_impl->get_input_panel_geometry (client_id, info);
}

//ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
void InfoManager::get_candidate_window_geometry (int client_id, _OUT_ struct rectinfo& info)
{
    m_impl->get_candidate_window_geometry (client_id, info);
}


//ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE
void InfoManager::get_ise_language_locale (int client_id, _OUT_ char** data, _OUT_ size_t& len)
{
    m_impl->get_ise_language_locale (client_id, data, len);
}

//ISM_TRANS_CMD_SET_LAYOUT
void InfoManager::set_ise_layout (int client_id, uint32 layout)
{
    m_impl->set_ise_layout (client_id, layout);
}

//ISM_TRANS_CMD_SET_INPUT_MODE
void InfoManager::set_ise_input_mode (int client_id, uint32 input_mode)
{
    m_impl->set_ise_input_mode (client_id, input_mode);
}

//ISM_TRANS_CMD_SET_INPUT_HINT
void InfoManager::set_ise_input_hint (int client_id, uint32 input_hint)
{
    m_impl->set_ise_input_hint (client_id, input_hint);
}

//ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION
void InfoManager::update_ise_bidi_direction (int client_id, uint32 bidi_direction)
{
    m_impl->update_ise_bidi_direction (client_id, bidi_direction);
}


//ISM_TRANS_CMD_SET_ISE_LANGUAGE
void InfoManager::set_ise_language (int client_id, uint32 language)
{
    m_impl->set_ise_language (client_id, language);
}

//ISM_TRANS_CMD_SET_ISE_IMDATA
void InfoManager::set_ise_imdata (int client_id, const char*   imdata, size_t  len)
{
    m_impl->set_ise_imdata (client_id, imdata, len);
}

//ISM_TRANS_CMD_GET_ISE_IMDATA
bool InfoManager:: get_ise_imdata (int client_id, _OUT_ char** imdata, _OUT_ size_t& len)
{
    return m_impl->get_ise_imdata (client_id, imdata, len);
}

//ISM_TRANS_CMD_GET_LAYOUT
bool InfoManager:: get_ise_layout (int client_id, _OUT_ uint32& layout)
{
    return m_impl->get_ise_layout (client_id, layout);
}

//ISM_TRANS_CMD_GET_ISE_STATE
void InfoManager::get_ise_state (int client_id, _OUT_ int& state)
{
    m_impl->get_ise_state (client_id, state);
}

//ISM_TRANS_CMD_GET_ACTIVE_ISE
void InfoManager::get_active_ise (int client_id, _OUT_ String& default_uuid)
{
    m_impl->get_active_ise (client_id, default_uuid);
}

//ISM_TRANS_CMD_GET_ISE_LIST
void InfoManager::get_ise_list (int client_id, _OUT_ std::vector<String>& strlist)
{
    m_impl->get_ise_list (client_id, strlist);
}


//ISM_TRANS_CMD_GET_ALL_HELPER_ISE_INFO
void InfoManager::get_all_helper_ise_info (int client_id, _OUT_ HELPER_ISE_INFO& info)
{
    m_impl->get_all_helper_ise_info (client_id, info);
}

//ISM_TRANS_CMD_SET_ENABLE_HELPER_ISE_INFO
//reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
void InfoManager::set_enable_helper_ise_info (int client_id, String appid, uint32 is_enabled)
{
    m_impl->set_enable_helper_ise_info (client_id, appid, is_enabled);
}

//ISM_TRANS_CMD_SHOW_HELPER_ISE_LIST
//reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
void InfoManager::show_helper_ise_list (int client_id)
{
    m_impl->show_helper_ise_list (client_id);
}


//ISM_TRANS_CMD_SHOW_HELPER_ISE_SELECTOR
//reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
void InfoManager::show_helper_ise_selector (int client_id)
{
    m_impl->show_helper_ise_selector (client_id);
}

//ISM_TRANS_CMD_IS_HELPER_ISE_ENABLED
//reply
void InfoManager::is_helper_ise_enabled (int client_id, String strAppid, _OUT_ uint32& nEnabled)
{
    m_impl->is_helper_ise_enabled (client_id, strAppid, nEnabled);
}

//ISM_TRANS_CMD_GET_ISE_INFORMATION
void InfoManager::get_ise_information (int client_id, String strUuid, _OUT_ String& strName, _OUT_ String& strLanguage,
                                       _OUT_ int& nType,   _OUT_ int& nOption, _OUT_ String& strModuleName)
{
    m_impl->get_ise_information (client_id, strUuid, strName, strLanguage, nType, nOption, strModuleName);
}

//ISM_TRANS_CMD_RESET_ISE_OPTION
//reply SCIM_TRANS_CMD_OK
bool InfoManager:: reset_ise_option (int client_id)
{
    return m_impl->reset_ise_option (client_id);
}

//ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID
//reply
bool InfoManager:: set_active_ise_by_uuid (int client_id, char*  buf, size_t  len)
{
    return m_impl->set_active_ise_by_uuid (client_id, buf, len);
}

//ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID
//reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
void InfoManager::set_initial_ise_by_uuid (int client_id, char*  buf, size_t  len)
{
    m_impl->set_initial_ise_by_uuid (client_id, buf, len);
}

//ISM_TRANS_CMD_SET_RETURN_KEY_TYPE
void InfoManager::set_ise_return_key_type (int client_id, uint32 type)
{
    m_impl->set_ise_return_key_type (client_id, type);
}

//ISM_TRANS_CMD_GET_RETURN_KEY_TYPE
//reply
bool InfoManager:: get_ise_return_key_type (int client_id, _OUT_ uint32& type)
{
    return m_impl->get_ise_return_key_type (client_id, type);
}


//ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE
void InfoManager::set_ise_return_key_disable (int client_id, uint32 disabled)
{
    m_impl->set_ise_return_key_disable (client_id, disabled);
}

//ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE
void InfoManager::get_ise_return_key_disable (int client_id, _OUT_ uint32& disabled)
{
    m_impl->get_ise_return_key_disable (client_id, disabled);
}

//ISM_TRANS_CMD_SET_CAPS_MODE
void InfoManager::set_ise_caps_mode (int client_id, uint32 mode)
{
    m_impl->set_ise_caps_mode (client_id, mode);
}

//SCIM_TRANS_CMD_RELOAD_CONFIG
void InfoManager::reload_config (void)
{
    m_impl->reload_config ();
}

//ISM_TRANS_CMD_SEND_WILL_SHOW_ACK
void InfoManager::will_show_ack (int client_id)
{
    m_impl->will_show_ack (client_id);
}

//ISM_TRANS_CMD_SEND_WILL_HIDE_ACK
void InfoManager::will_hide_ack (int client_id)
{
    m_impl->will_hide_ack (client_id);
}

//ISM_TRANS_CMD_RESET_DEFAULT_ISE
void InfoManager::reset_default_ise (int client_id)
{
    m_impl->reset_default_ise (client_id);
}

//ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE
void InfoManager::set_keyboard_mode (int client_id, uint32 mode)
{
    m_impl->set_keyboard_mode (client_id, mode);
}

//ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK
void InfoManager::candidate_will_hide_ack (int client_id)
{
    m_impl->candidate_will_hide_ack (client_id);
}

//ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION
void InfoManager::get_active_helper_option (int client_id, _OUT_ uint32& option)
{
    m_impl->get_active_helper_option (client_id, option);
}

//ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW
void InfoManager::show_ise_option_window (int client_id)
{
    m_impl->show_ise_option_window (client_id);
}

//ISM_TRANS_CMD_EXPAND_CANDIDATE
void InfoManager::expand_candidate ()
{
    m_impl->expand_candidate ();
}

//ISM_TRANS_CMD_CONTRACT_CANDIDATE
void InfoManager::contract_candidate ()
{
    m_impl->contract_candidate ();
}

//ISM_TRANS_CMD_GET_RECENT_ISE_GEOMETRY
void InfoManager::get_recent_ise_geometry (int client_id, uint32 angle, _OUT_ struct rectinfo& info)
{
    m_impl->get_recent_ise_geometry (client_id, angle, info);
}

//ISM_TRANS_CMD_REGISTER_PANEL_CLIENT
void InfoManager::register_panel_client (uint32 client_id, uint32 id)
{
    m_impl->register_panel_client (client_id, id);
}

//SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT
void InfoManager::register_input_context (uint32 client_id, uint32 context, String  uuid)
{
    m_impl->register_input_context (client_id, context, uuid);
}

//SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT
void InfoManager::remove_input_context (uint32 client_id, uint32 context)
{
    m_impl->remove_input_context (client_id, context);
}

//SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT
void InfoManager::socket_reset_input_context (int client_id, uint32 context)
{
    m_impl->socket_reset_input_context (client_id, context);
}

//SCIM_TRANS_CMD_FOCUS_IN
void InfoManager::focus_in (int client_id, uint32 context,  String  uuid)
{
    m_impl->focus_in (client_id, context, uuid);
}

//SCIM_TRANS_CMD_FOCUS_OUT
void InfoManager::focus_out (int client_id, uint32 context)
{
    m_impl->focus_out (client_id, context);
}

//ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT
bool InfoManager::process_input_device_event(int client, uint32 type, const char *data, size_t len, _OUT_ uint32& result)
{
    return m_impl->process_input_device_event(client, type, data, len, result);
}

//ISM_TRANS_CMD_TURN_ON_LOG
void InfoManager::socket_turn_on_log (uint32 isOn)
{
    m_impl->socket_turn_on_log (isOn);
}

//SCIM_TRANS_CMD_PANEL_TURN_ON
void InfoManager::socket_turn_on (void)
{
    m_impl->socket_turn_on ();
}

//SCIM_TRANS_CMD_PANEL_TURN_OFF
void InfoManager::socket_turn_off (void)
{
    m_impl->socket_turn_off ();
}

//SCIM_TRANS_CMD_UPDATE_SCREEN
void InfoManager::socket_update_screen (int client_id, uint32 num)
{
    m_impl->socket_update_screen (client_id, num);
}

//SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
void InfoManager::socket_update_spot_location (uint32 x, uint32 y, uint32 top_y)
{
    m_impl->socket_update_spot_location (x, y, top_y);
}

//ISM_TRANS_CMD_UPDATE_CURSOR_POSITION
void InfoManager::socket_update_cursor_position (uint32 cursor_pos)
{
    m_impl->socket_update_cursor_position (cursor_pos);
}

//ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT
void InfoManager::socket_update_surrounding_text (String text, uint32 cursor)
{
    m_impl->socket_update_surrounding_text (text, cursor);
}

//ISM_TRANS_CMD_UPDATE_SELECTION
void InfoManager::socket_update_selection (String text)
{
    m_impl->socket_update_selection (text);
}

//FIXME: useless anymore
//SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO
void InfoManager::socket_update_factory_info (PanelFactoryInfo& info)
{
    m_impl->socket_update_factory_info (info);
}

//SCIM_TRANS_CMD_PANEL_SHOW_HELP
void InfoManager::socket_show_help (String help)
{
    m_impl->socket_show_help (help);
}

//SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU
void InfoManager::socket_show_factory_menu (std::vector <PanelFactoryInfo>& vec)
{
    m_impl->socket_show_factory_menu (vec);
}

//SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
void InfoManager::socket_show_preedit_string (void)
{
    m_impl->socket_show_preedit_string ();
}

//SCIM_TRANS_CMD_SHOW_AUX_STRING
void InfoManager::socket_show_aux_string (void)
{
    m_impl->socket_show_aux_string ();
}

//SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE
void InfoManager::socket_show_lookup_table (void)
{
    m_impl->socket_show_lookup_table ();
}

//ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE
void InfoManager::socket_show_associate_table (void)
{
    m_impl->socket_show_associate_table ();
}

//SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
void InfoManager::socket_hide_preedit_string (void)
{
    m_impl->socket_hide_preedit_string ();
}

//SCIM_TRANS_CMD_HIDE_AUX_STRING
void InfoManager::socket_hide_aux_string (void)
{
    m_impl->socket_hide_aux_string ();
}

//SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE
void InfoManager::socket_hide_lookup_table (void)
{
    m_impl->socket_hide_lookup_table ();
}

//ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE
void InfoManager::socket_hide_associate_table (void)
{
    m_impl->socket_hide_associate_table ();
}

//SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
void InfoManager::socket_update_preedit_string (String& str, const AttributeList& attrs, uint32 caret)
{
    m_impl->socket_update_preedit_string (str, attrs, caret);
}

//SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
void InfoManager::socket_update_preedit_caret (uint32 caret)
{
    m_impl->socket_update_preedit_caret (caret);
}

//SCIM_TRANS_CMD_UPDATE_AUX_STRING
void InfoManager::socket_update_aux_string (String& str, const AttributeList& attrs)
{
    m_impl->socket_update_aux_string (str, attrs);
}

//SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE
void InfoManager::socket_update_lookup_table (const LookupTable& table)
{
    m_impl->socket_update_lookup_table (table);
}

//ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE
void InfoManager::socket_update_associate_table (const LookupTable& table)
{
    m_impl->socket_update_associate_table (table);
}

//SCIM_TRANS_CMD_REGISTER_PROPERTIES
void InfoManager::socket_register_properties (const PropertyList& properties)
{
    m_impl->socket_register_properties (properties);
}

//SCIM_TRANS_CMD_UPDATE_PROPERTY
void InfoManager::socket_update_property (const Property& property)
{
    m_impl->socket_update_property (property);
}

//ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST
void InfoManager::socket_get_keyboard_ise_list (String& uuid)
{
    m_impl->socket_get_keyboard_ise_list (uuid);
}

//ISM_TRANS_CMD_SET_CANDIDATE_UI
void InfoManager::socket_set_candidate_ui (uint32 portrait_line, uint32 mode)
{
    m_impl->socket_set_candidate_ui (portrait_line, mode);
}

//ISM_TRANS_CMD_GET_CANDIDATE_UI
void InfoManager::socket_get_candidate_ui (String uuid)
{
    m_impl->socket_get_candidate_ui (uuid);
}

//ISM_TRANS_CMD_SET_CANDIDATE_POSITION
void InfoManager::socket_set_candidate_position (uint32 left, uint32 top)
{
    m_impl->socket_set_candidate_position (left, top);
}

//ISM_TRANS_CMD_HIDE_CANDIDATE
void InfoManager::socket_hide_candidate (void)
{
    m_impl->socket_hide_candidate ();
}

//ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
void InfoManager::socket_get_candidate_geometry (String& uuid)
{
    m_impl->socket_get_candidate_geometry (uuid);
}

//ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID
void InfoManager::socket_set_keyboard_ise (String uuid)
{
    m_impl->socket_set_keyboard_ise (uuid);
}

//ISM_TRANS_CMD_SELECT_CANDIDATE
void InfoManager::socket_helper_select_candidate (uint32 index)
{
    m_impl->socket_helper_select_candidate (index);
}

//ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY
void InfoManager::socket_helper_update_ise_geometry (int client, uint32 x, uint32 y, uint32 width, uint32 height)
{
    m_impl->socket_helper_update_ise_geometry (client, x, y, width, height);
}

//ISM_TRANS_CMD_GET_KEYBOARD_ISE
void InfoManager::socket_get_keyboard_ise (String uuid)
{
    m_impl->socket_get_keyboard_ise (uuid);
}

//SCIM_TRANS_CMD_START_HELPER
void InfoManager::socket_start_helper (int client_id, uint32 context, String uuid)
{
    m_impl->socket_start_helper (client_id, context, uuid);
}

//SCIM_TRANS_CMD_STOP_HELPER
void InfoManager::socket_stop_helper (int client_id, uint32 context, String uuid)
{
    m_impl->socket_stop_helper (client_id, context, uuid);
}

//SCIM_TRANS_CMD_SEND_HELPER_EVENT
void InfoManager::socket_send_helper_event (int client_id, uint32 context, String uuid, const Transaction& _nest_trans)
{
    m_impl->socket_send_helper_event (client_id, context, uuid, _nest_trans);
}

//SCIM_TRANS_CMD_REGISTER_PROPERTIES
void InfoManager::socket_helper_register_properties (int client, PropertyList& properties)
{
    m_impl->socket_helper_register_properties (client, properties);
}

//SCIM_TRANS_CMD_UPDATE_PROPERTY
void InfoManager::socket_helper_update_property (int client, Property& property)
{
    m_impl->socket_helper_update_property (client, property);
}

//SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT
void InfoManager::socket_helper_send_imengine_event (int client, uint32 target_ic, String target_uuid , Transaction& nest_trans)
{
    m_impl->socket_helper_send_imengine_event (client, target_ic, target_uuid, nest_trans);
}

//SCIM_TRANS_CMD_PROCESS_KEY_EVENT
//SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT
void InfoManager::socket_helper_send_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key)
{
    m_impl->socket_helper_send_key_event (client, target_ic, target_uuid, key);
}

//SCIM_TRANS_CMD_FORWARD_KEY_EVENT
void InfoManager::socket_helper_forward_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key)
{
    m_impl->socket_helper_forward_key_event (client, target_ic, target_uuid, key);
}

//SCIM_TRANS_CMD_COMMIT_STRING
void InfoManager::socket_helper_commit_string (int client, uint32 target_ic, String target_uuid, WideString wstr)
{
    m_impl->socket_helper_commit_string (client, target_ic, target_uuid, wstr);
}

//SCIM_TRANS_CMD_GET_SURROUNDING_TEXT
void InfoManager::socket_helper_get_surrounding_text (int client, String uuid, uint32 maxlen_before, uint32 maxlen_after, const int fd)
{
    m_impl->socket_helper_get_surrounding_text (client, uuid, maxlen_before, maxlen_after, fd);
}

//SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT
void InfoManager::socket_helper_delete_surrounding_text (int client, uint32 offset, uint32 len)
{
    m_impl->socket_helper_delete_surrounding_text (client, offset, len);
}

//SCIM_TRANS_CMD_GET_SELECTION
void InfoManager::socket_helper_get_selection (int client, String uuid, const int fd)
{
    m_impl->socket_helper_get_selection (client, uuid, fd);
}

//SCIM_TRANS_CMD_SET_SELECTION
void InfoManager::socket_helper_set_selection (int client, uint32 start, uint32 end)
{
    m_impl->socket_helper_set_selection (client, start, end);
}

//SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
void InfoManager::socket_helper_show_preedit_string (int client, uint32 target_ic, String target_uuid)
{
    m_impl->socket_helper_show_preedit_string (client, target_ic, target_uuid);
}

//SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
void InfoManager::socket_helper_hide_preedit_string (int client, uint32 target_ic, String target_uuid)
{
    m_impl->socket_helper_hide_preedit_string (client, target_ic, target_uuid);
}

//SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
void InfoManager::socket_helper_update_preedit_string (int client, uint32 target_ic, String target_uuid, WideString wstr,
        AttributeList& attrs, uint32 caret)
{
    m_impl->socket_helper_update_preedit_string (client, target_ic, target_uuid, wstr, attrs, caret);
}

//SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
void InfoManager::socket_helper_update_preedit_caret (int client, uint32 caret)
{
    m_impl->socket_helper_update_preedit_caret (client, caret);
}

//SCIM_TRANS_CMD_PANEL_REGISTER_HELPER
void InfoManager::socket_helper_register_helper (int client, HelperInfo& info)
{
    m_impl->socket_helper_register_helper (client, info);
}

//SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER
void InfoManager::socket_helper_register_helper_passive (int client, HelperInfo& info)
{
    m_impl->socket_helper_register_helper_passive (client, info);
}

//ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT
void InfoManager::socket_helper_update_input_context (int client, uint32 type, uint32 value)
{
    m_impl->socket_helper_update_input_context (client, type, value);
}

//SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND
void InfoManager::socket_helper_send_private_command (int client, String command)
{
    m_impl->socket_helper_send_private_command (client, command);
}

//ISM_TRANS_CMD_UPDATE_ISE_EXIT
//void InfoManager::UPDATE_ISE_EXIT (int client)
//{
//    m_impl->UPDATE_ISE_EXIT (client);
//}

bool InfoManager::check_privilege_by_sockfd (int client_id, const String& privilege) {
    return m_impl->check_privilege_by_sockfd (client_id, privilege);
}

void InfoManager::add_client (int client_id, uint32 key, ClientType type)
{
    m_impl->add_client (client_id, key, type);
}

void InfoManager::del_client (int client_id)
{
    m_impl->del_client (client_id);
}

//////////////////////////////////Message function end/////////////////////////////////////////

Connection
InfoManager::signal_connect_turn_on (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_turn_on (slot);
}

Connection
InfoManager::signal_connect_turn_off (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_turn_off (slot);
}

Connection
InfoManager::signal_connect_show_panel (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_panel (slot);
}

Connection
InfoManager::signal_connect_hide_panel (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_panel (slot);
}

Connection
InfoManager::signal_connect_update_screen (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_update_screen (slot);
}

Connection
InfoManager::signal_connect_update_spot_location (InfoManagerSlotIntIntInt*           slot)
{
    return m_impl->signal_connect_update_spot_location (slot);
}

Connection
InfoManager::signal_connect_update_factory_info (InfoManagerSlotFactoryInfo*         slot)
{
    return m_impl->signal_connect_update_factory_info (slot);
}

Connection
InfoManager::signal_connect_start_default_ise (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_start_default_ise (slot);
}

Connection
InfoManager::signal_connect_stop_default_ise (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_stop_default_ise (slot);
}

Connection
InfoManager::signal_connect_set_candidate_ui (InfoManagerSlotIntInt*              slot)
{
    return m_impl->signal_connect_set_candidate_ui (slot);
}

Connection
InfoManager::signal_connect_get_candidate_ui (InfoManagerSlotIntInt2*             slot)
{
    return m_impl->signal_connect_get_candidate_ui (slot);
}

Connection
InfoManager::signal_connect_set_candidate_position (InfoManagerSlotIntInt*              slot)
{
    return m_impl->signal_connect_set_candidate_position (slot);
}

Connection
InfoManager::signal_connect_get_candidate_geometry (InfoManagerSlotRect*                slot)
{
    return m_impl->signal_connect_get_candidate_geometry (slot);
}

Connection
InfoManager::signal_connect_get_input_panel_geometry (InfoManagerSlotRect*                slot)
{
    return m_impl->signal_connect_get_input_panel_geometry (slot);
}

Connection
InfoManager::signal_connect_set_keyboard_ise (InfoManagerSlotString*              slot)
{
    return m_impl->signal_connect_set_keyboard_ise (slot);
}

Connection
InfoManager::signal_connect_get_keyboard_ise (InfoManagerSlotString2*             slot)
{
    return m_impl->signal_connect_get_keyboard_ise (slot);
}

Connection
InfoManager::signal_connect_show_help (InfoManagerSlotString*              slot)
{
    return m_impl->signal_connect_show_help (slot);
}

Connection
InfoManager::signal_connect_show_factory_menu (InfoManagerSlotFactoryInfoVector*   slot)
{
    return m_impl->signal_connect_show_factory_menu (slot);
}

Connection
InfoManager::signal_connect_show_preedit_string (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_preedit_string (slot);
}

Connection
InfoManager::signal_connect_show_aux_string (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_aux_string (slot);
}

Connection
InfoManager::signal_connect_show_lookup_table (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_lookup_table (slot);
}

Connection
InfoManager::signal_connect_show_associate_table (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_associate_table (slot);
}

Connection
InfoManager::signal_connect_hide_preedit_string (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_preedit_string (slot);
}

Connection
InfoManager::signal_connect_hide_aux_string (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_aux_string (slot);
}

Connection
InfoManager::signal_connect_hide_lookup_table (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_lookup_table (slot);
}

Connection
InfoManager::signal_connect_hide_associate_table (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_associate_table (slot);
}

Connection
InfoManager::signal_connect_update_preedit_string (InfoManagerSlotAttributeStringInt*  slot)
{
    return m_impl->signal_connect_update_preedit_string (slot);
}

Connection
InfoManager::signal_connect_update_preedit_caret (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_update_preedit_caret (slot);
}

Connection
InfoManager::signal_connect_update_aux_string (InfoManagerSlotAttributeString*     slot)
{
    return m_impl->signal_connect_update_aux_string (slot);
}

Connection
InfoManager::signal_connect_update_lookup_table (InfoManagerSlotLookupTable*         slot)
{
    return m_impl->signal_connect_update_lookup_table (slot);
}

Connection
InfoManager::signal_connect_update_associate_table (InfoManagerSlotLookupTable*         slot)
{
    return m_impl->signal_connect_update_associate_table (slot);
}

Connection
InfoManager::signal_connect_register_properties (InfoManagerSlotPropertyList*        slot)
{
    return m_impl->signal_connect_register_properties (slot);
}

Connection
InfoManager::signal_connect_update_property (InfoManagerSlotProperty*            slot)
{
    return m_impl->signal_connect_update_property (slot);
}

Connection
InfoManager::signal_connect_register_helper_properties (InfoManagerSlotIntPropertyList*     slot)
{
    return m_impl->signal_connect_register_helper_properties (slot);
}

Connection
InfoManager::signal_connect_update_helper_property (InfoManagerSlotIntProperty*         slot)
{
    return m_impl->signal_connect_update_helper_property (slot);
}

Connection
InfoManager::signal_connect_register_helper (InfoManagerSlotIntHelperInfo*       slot)
{
    return m_impl->signal_connect_register_helper (slot);
}

Connection
InfoManager::signal_connect_remove_helper (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_remove_helper (slot);
}

Connection
InfoManager::signal_connect_set_active_ise_by_uuid (InfoManagerSlotStringBool*          slot)
{
    return m_impl->signal_connect_set_active_ise_by_uuid (slot);
}

Connection
InfoManager::signal_connect_focus_in (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_focus_in (slot);
}

Connection
InfoManager::signal_connect_focus_out (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_focus_out (slot);
}

Connection
InfoManager::signal_connect_expand_candidate (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_expand_candidate (slot);
}

Connection
InfoManager::signal_connect_contract_candidate (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_contract_candidate (slot);
}

Connection
InfoManager::signal_connect_select_candidate (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_select_candidate (slot);
}

Connection
InfoManager::signal_connect_get_ise_list (InfoManagerSlotBoolStringVector*    slot)
{
    return m_impl->signal_connect_get_ise_list (slot);
}

Connection
InfoManager::signal_connect_get_all_helper_ise_info (InfoManagerSlotBoolHelperInfo*      slot)
{
    return m_impl->signal_connect_get_all_helper_ise_info (slot);
}

Connection
InfoManager::signal_connect_set_has_option_helper_ise_info (InfoManagerSlotStringBool*      slot)
{
    return m_impl->signal_connect_set_has_option_helper_ise_info (slot);
}

Connection
InfoManager::signal_connect_set_enable_helper_ise_info (InfoManagerSlotStringBool*     slot)
{
    return m_impl->signal_connect_set_enable_helper_ise_info (slot);
}

Connection
InfoManager::signal_connect_show_helper_ise_list (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_helper_ise_list (slot);
}

Connection
InfoManager::signal_connect_show_helper_ise_selector (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_helper_ise_selector (slot);
}

Connection
InfoManager::signal_connect_is_helper_ise_enabled (InfoManagerSlotStringInt*           slot)
{
    return m_impl->signal_connect_is_helper_ise_enabled (slot);
}

Connection
InfoManager::signal_connect_get_ise_information (InfoManagerSlotBoolString4int2*     slot)
{
    return m_impl->signal_connect_get_ise_information (slot);
}

Connection
InfoManager::signal_connect_get_keyboard_ise_list (InfoManagerSlotBoolStringVector*    slot)
{
    return m_impl->signal_connect_get_keyboard_ise_list (slot);
}

Connection
InfoManager::signal_connect_update_ise_geometry (InfoManagerSlotIntIntIntInt*        slot)
{
    return m_impl->signal_connect_update_ise_geometry (slot);
}

Connection
InfoManager::signal_connect_get_language_list (InfoManagerSlotStringVector*        slot)
{
    return m_impl->signal_connect_get_language_list (slot);
}

Connection
InfoManager::signal_connect_get_all_language (InfoManagerSlotStringVector*        slot)
{
    return m_impl->signal_connect_get_all_language (slot);
}

Connection
InfoManager::signal_connect_get_ise_language (InfoManagerSlotStrStringVector*     slot)
{
    return m_impl->signal_connect_get_ise_language (slot);
}

Connection
InfoManager::signal_connect_get_ise_info_by_uuid (InfoManagerSlotStringISEINFO*       slot)
{
    return m_impl->signal_connect_get_ise_info_by_uuid (slot);
}

Connection
InfoManager::signal_connect_send_key_event (InfoManagerSlotKeyEvent*            slot)
{
    return m_impl->signal_connect_send_key_event (slot);
}

Connection
InfoManager::signal_connect_accept_connection (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_accept_connection (slot);
}

Connection
InfoManager::signal_connect_close_connection (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_close_connection (slot);
}

Connection
InfoManager::signal_connect_exit (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_exit (slot);
}

Connection
InfoManager::signal_connect_transaction_start (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_transaction_start (slot);
}

Connection
InfoManager::signal_connect_transaction_end (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_transaction_end (slot);
}

Connection
InfoManager::signal_connect_lock (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_lock (slot);
}

Connection
InfoManager::signal_connect_unlock (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_unlock (slot);
}

Connection
InfoManager::signal_connect_update_input_context (InfoManagerSlotIntInt*              slot)
{
    return m_impl->signal_connect_update_input_context (slot);
}

Connection
InfoManager::signal_connect_show_ise (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_show_ise (slot);
}

Connection
InfoManager::signal_connect_hide_ise (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_hide_ise (slot);
}

Connection
InfoManager::signal_connect_will_show_ack (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_will_show_ack (slot);
}

Connection
InfoManager::signal_connect_will_hide_ack (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_will_hide_ack (slot);
}

Connection
InfoManager::signal_connect_set_keyboard_mode (InfoManagerSlotInt*                 slot)
{
    return m_impl->signal_connect_set_keyboard_mode (slot);
}

Connection
InfoManager::signal_connect_candidate_will_hide_ack (InfoManagerSlotVoid*                slot)
{
    return m_impl->signal_connect_candidate_will_hide_ack (slot);
}

Connection
InfoManager::signal_connect_get_ise_state (InfoManagerSlotInt2*                slot)
{
    return m_impl->signal_connect_get_ise_state (slot);
}

Connection
InfoManager::signal_connect_run_helper (InfoManagerSlotString3*                slot)
{
    return m_impl->signal_connect_run_helper (slot);
}

Connection
InfoManager::signal_connect_get_recent_ise_geometry (InfoManagerSlotIntRect*             slot)
{
    return m_impl->signal_connect_get_recent_ise_geometry (slot);
}

Connection
InfoManager::signal_connect_check_privilege_by_sockfd  (InfoManagerSlotIntString2* slot)
{
    return m_impl->signal_connect_check_privilege_by_sockfd (slot);
}


} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/
