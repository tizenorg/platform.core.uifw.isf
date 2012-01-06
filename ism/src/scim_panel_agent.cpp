/** @file scim_panel.cpp
 *  @brief Implementation of class PanelAgent.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2005 James Su <suzhe@tsinghua.org.cn>
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
 * $Id: scim_panel_agent.cpp,v 1.8.2.1 2006/01/09 14:32:18 suzhe Exp $
 *
 */

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_PANEL_AGENT
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_SCIM_UTILITY

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"


scim::CommonLookupTable g_isf_candidate_table;


namespace scim {

typedef Signal0<void>
        PanelAgentSignalVoid;

typedef Signal1<void, int>
        PanelAgentSignalInt;

typedef Signal1<void, const String &>
        PanelAgentSignalString;

typedef Signal2<void, const String &, bool>
        PanelAgentSignalStringBool;

typedef Signal2<void, String &, String &>
        PanelAgentSignalString2;

typedef Signal2<void, int, const String &>
        PanelAgentSignalIntString;

typedef Signal1<void, const PanelFactoryInfo &>
        PanelAgentSignalFactoryInfo;

typedef Signal1<void, const std::vector <PanelFactoryInfo> &>
        PanelAgentSignalFactoryInfoVector;

typedef Signal1<void, const LookupTable &>
        PanelAgentSignalLookupTable;

typedef Signal1<void, const Property &>
        PanelAgentSignalProperty;

typedef Signal1<void, const PropertyList &>
        PanelAgentSignalPropertyList;

typedef Signal2<void, int, int>
        PanelAgentSignalIntInt;

typedef Signal2<void, int &, int &>
        PanelAgentSignalIntInt2;

typedef Signal3<void, int, int, int>
        PanelAgentSignalIntIntInt;

typedef Signal2<void, int, const Property &>
        PanelAgentSignalIntProperty;

typedef Signal2<void, int, const PropertyList &>
        PanelAgentSignalIntPropertyList;

typedef Signal2<void, int, const HelperInfo &>
        PanelAgentSignalIntHelperInfo;

typedef Signal2<void, const String &, const AttributeList &>
        PanelAgentSignalAttributeString;

typedef Signal1<void, std::vector <String> &>
        PanelAgentSignalStringVector;

typedef Signal1<bool, std::vector <String> &>
        PanelAgentSignalBoolStringVector;

typedef Signal2<void, char *, std::vector <String> &>
        PanelAgentSignalStrStringVector;

typedef Signal2<bool, const String &, ISE_INFO &>
        PanelAgentSignalStringISEINFO;

typedef Signal1<void, const KeyEvent &>
        PanelAgentSignalKeyEvent;

typedef Signal1<void, struct rectinfo &>
        PanelAgentSignalRect;

enum ClientType {
    UNKNOWN_CLIENT,
    FRONTEND_CLIENT,
    HELPER_CLIENT,
    HELPER_ACT_CLIENT,
    IMCONTROL_ACT_CLIENT,
    IMCONTROL_CLIENT
};

struct ClientInfo {
    uint32       key;
    ClientType   type;
};

struct HelperClientStub {
    int id;
    int ref;

    HelperClientStub (int i = 0, int r = 0) : id (i), ref (r) { }
};

struct IMControlStub {
    std::vector<ISE_INFO> info;
    std::vector<int> count;
};

static  int _id_count = -4;

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
get_imengine_client_context (uint32 helper_ic, int &client, uint32 &context)
{
    client   = (int) (helper_ic & 0xFFFF);
    context  = ((helper_ic >> 16) & 0x7FFF);
}

//==================================== PanelAgent ===========================
class PanelAgent::PanelAgentImpl
{
    bool                                m_should_exit;
    bool                                m_should_resident;

    int                                 m_current_screen;

    String                              m_config_name;
    String                              m_display_name;

    int                                 m_socket_timeout;
    String                              m_socket_address;
    SocketServer                        m_socket_server;

    Transaction                         m_send_trans;
    Transaction                         m_recv_trans;
    Transaction                         m_nest_trans;

    int                                 m_current_socket_client;
    uint32                              m_current_client_context;
    String                              m_current_context_uuid;
    TOOLBAR_MODE_T                      m_current_toolbar_mode;
    String                              m_current_factory_icon;
    String                              m_current_helper_uuid;
    String                              m_last_helper_uuid;
    String                              m_current_ise_name;
    uint32                              m_current_ise_style;
    int                                 m_current_active_imcontrol_id;
    int                                 m_pending_active_imcontrol_id;
    IntIntRepository                    m_imcontrol_map;
    DEFAULT_ISE_T                       m_default_ise;
    bool                                m_should_shared_ise;
    char *                              m_ise_settings;
    size_t                              m_ise_settings_len;
    bool                                m_ise_changing;
    bool                                m_ise_exiting;

    int                                 m_last_socket_client;
    uint32                              m_last_client_context;
    String                              m_last_context_uuid;

    ClientRepository                    m_client_repository;
    /*
    * Each Helper ISE has two socket connect between PanelAgent and HelperAgent.
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
    UUIDStateRepository                 m_helper_uuid_state;

    HelperManager                       m_helper_manager;

    PanelAgentSignalVoid                m_signal_reload_config;
    PanelAgentSignalVoid                m_signal_turn_on;
    PanelAgentSignalVoid                m_signal_turn_off;
    PanelAgentSignalVoid                m_signal_show_panel;
    PanelAgentSignalVoid                m_signal_hide_panel;
    PanelAgentSignalInt                 m_signal_update_screen;
    PanelAgentSignalIntIntInt           m_signal_update_spot_location;
    PanelAgentSignalFactoryInfo         m_signal_update_factory_info;
    PanelAgentSignalVoid                m_signal_start_default_ise;
    PanelAgentSignalIntInt              m_signal_set_candidate_ui;
    PanelAgentSignalIntInt2             m_signal_get_candidate_ui;
    PanelAgentSignalIntInt              m_signal_set_candidate_position;
    PanelAgentSignalRect                m_signal_get_candidate_rect;
    PanelAgentSignalIntString           m_signal_set_keyboard_ise;
    PanelAgentSignalString2             m_signal_get_keyboard_ise;
    PanelAgentSignalString              m_signal_show_help;
    PanelAgentSignalFactoryInfoVector   m_signal_show_factory_menu;
    PanelAgentSignalVoid                m_signal_show_preedit_string;
    PanelAgentSignalVoid                m_signal_show_aux_string;
    PanelAgentSignalVoid                m_signal_show_lookup_table;
    PanelAgentSignalVoid                m_signal_show_associate_table;
    PanelAgentSignalVoid                m_signal_hide_preedit_string;
    PanelAgentSignalVoid                m_signal_hide_aux_string;
    PanelAgentSignalVoid                m_signal_hide_lookup_table;
    PanelAgentSignalVoid                m_signal_hide_associate_table;
    PanelAgentSignalAttributeString     m_signal_update_preedit_string;
    PanelAgentSignalInt                 m_signal_update_preedit_caret;
    PanelAgentSignalAttributeString     m_signal_update_aux_string;
    PanelAgentSignalLookupTable         m_signal_update_lookup_table;
    PanelAgentSignalLookupTable         m_signal_update_associate_table;
    PanelAgentSignalPropertyList        m_signal_register_properties;
    PanelAgentSignalProperty            m_signal_update_property;
    PanelAgentSignalIntPropertyList     m_signal_register_helper_properties;
    PanelAgentSignalIntProperty         m_signal_update_helper_property;
    PanelAgentSignalIntHelperInfo       m_signal_register_helper;
    PanelAgentSignalInt                 m_signal_remove_helper;
    PanelAgentSignalStringBool          m_signal_set_active_ise_by_uuid;
    PanelAgentSignalString              m_signal_set_active_ise_by_name;
    PanelAgentSignalVoid                m_signal_focus_in;
    PanelAgentSignalVoid                m_signal_focus_out;
    PanelAgentSignalBoolStringVector    m_signal_get_ise_list;
    PanelAgentSignalBoolStringVector    m_signal_get_keyboard_ise_list;
    PanelAgentSignalInt                 m_signal_launch_helper_ise_list_selection;
    PanelAgentSignalStringVector        m_signal_get_language_list;
    PanelAgentSignalStringVector        m_signal_get_all_language;
    PanelAgentSignalStrStringVector     m_signal_get_ise_language;
    PanelAgentSignalString              m_signal_set_isf_language;
    PanelAgentSignalStringISEINFO       m_signal_get_ise_info_by_uuid;
    PanelAgentSignalStringISEINFO       m_signal_get_ise_info_by_name;
    PanelAgentSignalKeyEvent            m_signal_send_key_event;

    PanelAgentSignalInt                 m_signal_accept_connection;
    PanelAgentSignalInt                 m_signal_close_connection;
    PanelAgentSignalVoid                m_signal_exit;

    PanelAgentSignalVoid                m_signal_transaction_start;
    PanelAgentSignalVoid                m_signal_transaction_end;

    PanelAgentSignalVoid                m_signal_lock;
    PanelAgentSignalVoid                m_signal_unlock;

public:
    PanelAgentImpl ()
        : m_should_exit (false),
          m_should_resident (false),
          m_current_screen (0),
          m_socket_timeout (scim_get_default_socket_timeout ()),
          m_current_socket_client (-1), m_current_client_context (0),
          m_current_toolbar_mode (TOOLBAR_KEYBOARD_MODE),
          m_current_ise_style (0),
          m_current_active_imcontrol_id (-1), m_pending_active_imcontrol_id (-1),
          m_should_shared_ise (false),
          m_ise_settings (NULL), m_ise_settings_len (0),
          m_ise_changing (false), m_ise_exiting (false),
          m_last_socket_client (-1), m_last_client_context (0)
    {
        m_current_ise_name = String (_("English/Keyboard"));
        m_imcontrol_repository.clear ();
        m_imcontrol_map.clear ();
        m_socket_server.signal_connect_accept (slot (this, &PanelAgentImpl::socket_accept_callback));
        m_socket_server.signal_connect_receive (slot (this, &PanelAgentImpl::socket_receive_callback));
        m_socket_server.signal_connect_exception (slot (this, &PanelAgentImpl::socket_exception_callback));
    }

    bool initialize (const String &config, const String &display, bool resident)
    {
        m_config_name = config;
        m_display_name = display;
        m_should_resident = resident;

        m_socket_address = scim_get_default_panel_socket_address (display);

        m_socket_server.shutdown ();

        return m_socket_server.create (SocketAddress (m_socket_address));
    }

    bool valid (void) const
    {
        return m_socket_server.valid ();
    }

public:
    bool run (void)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::run ()\n";

        return m_socket_server.run ();
    }

    void stop (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::stop ()\n";

        lock ();
        m_should_exit = true;
        unlock ();

        SocketClient  client;

        if (client.connect (SocketAddress (m_socket_address))) {
            client.close ();
        }
    }

    int get_helper_list (std::vector <HelperInfo> & helpers) const
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::get_helper_list ()\n";

        helpers.clear ();

        m_helper_manager.get_helper_list ();
        unsigned int num = m_helper_manager.number_of_helpers ();
        HelperInfo info;

        SCIM_DEBUG_MAIN (2) << "Found " << num << " Helper objects\n";

        for (unsigned int i = 0; i < num; ++i) {
            if (m_helper_manager.get_helper_info (i, info) && info.uuid.length ()
                && (info.option & SCIM_HELPER_STAND_ALONE))
                helpers.push_back (info);

            SCIM_DEBUG_MAIN (3) << "Helper " << i << " : " << info.uuid << " : " << info.name << " : "
                                << ((info.option & SCIM_HELPER_STAND_ALONE) ? "SA " : "")
                                << ((info.option & SCIM_HELPER_AUTO_START) ? "AS " : "")
                                << ((info.option & SCIM_HELPER_AUTO_RESTART) ? "AR " : "") << "\n";
        }

        return (int)(helpers.size ());
    }

    TOOLBAR_MODE_T get_current_toolbar_mode () const
    {
        return m_current_toolbar_mode;
    }

    String get_current_ise_name () const
    {
        return m_current_ise_name;
    }

    String get_current_factory_icon () const
    {
        return m_current_factory_icon;
    }

    String get_current_helper_uuid () const
    {
        return m_current_helper_uuid;
    }

    String get_current_helper_name () const
    {
        std::vector<HelperInfo> helpers;

        get_helper_list (helpers);

        std::vector<HelperInfo>::iterator iter;

        for (iter = helpers.begin (); iter != helpers.end (); iter++) {
            if (iter->uuid == m_current_helper_uuid)
                return iter->name;
        }

        return String ("");
    }

    void set_current_ise_name (String &name)
    {
        m_current_ise_name = name;
    }

    void set_current_ise_style (uint32 &style)
    {
        m_current_ise_style = style;
    }

    void set_current_toolbar_mode (TOOLBAR_MODE_T mode)
    {
        m_current_toolbar_mode = mode;
    }

    void update_ise_name (String &name)
    {
        ClientRepository::iterator iter = m_client_repository.begin ();

        for (; iter != m_client_repository.end (); iter++)
        {
            if (IMCONTROL_CLIENT == iter->second.type
                && iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                trans.put_command (ISM_TRANS_CMD_ISE_CHANGED);
                trans.put_data (name);

                trans.write_to_socket (client_socket);
                break;
            }
        }
    }

    void update_ise_style (uint32 &style)
    {
        ClientRepository::iterator iter = m_client_repository.begin ();

        for (; iter != m_client_repository.end (); iter++)
        {
            if (IMCONTROL_CLIENT == iter->second.type &&
                iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                trans.put_command (ISM_TRANS_CMD_UPDATE_ISE_STYLE);
                trans.put_data (style);

                trans.write_to_socket (client_socket);
                break;
            }
        }
    }

    void set_current_factory_icon (String &icon)
    {
        m_current_factory_icon = icon;
    }

    bool move_preedit_caret (uint32 position)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::move_preedit_caret (" << position << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_MOVE_PREEDIT_CARET);
            m_send_trans.put_data ((uint32) position);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool request_help (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::request_help ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REQUEST_HELP);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool request_factory_menu (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::request_factory_menu ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool reset_keyboard_ise (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::reset_keyboard_ise ()\n";
        int    client = -1;
        uint32 context = 0;

        lock ();

        get_focused_context (client, context);
        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_PANEL_REQUEST_RESET_ISE);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool update_keyboard_ise_list (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_keyboard_ise_list ()\n";
        int    client = -1;
        uint32 context = 0;

        lock ();

        get_focused_context (client, context);
        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_PANEL_UPDATE_KEYBOARD_ISE);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool change_factory (const String  &uuid)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::change_factory (" << uuid << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY);
            m_send_trans.put_data (uuid);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool candidate_more_window_show (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool candidate_more_window_hide (void)
    {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool select_aux (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_aux (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_SELECT_AUX);
            m_send_trans.put_data ((uint32)item);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool select_candidate (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_candidate (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_SELECT_CANDIDATE);
            m_send_trans.put_data ((uint32)item);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_select_candidate (item);

        return client >= 0;
    }

    bool lookup_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::lookup_table_page_up ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_lookup_table_page_up ();

        return client >= 0;
    }

    bool lookup_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::lookup_table_page_down ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_lookup_table_page_down ();

        return client >= 0;
    }

    bool update_lookup_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_lookup_table_page_size (" << size << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE);
            m_send_trans.put_data (size);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_update_lookup_table_page_size (size);

        return client >= 0;
    }

    bool select_associate (uint32 item)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_associate (" << item << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_SELECT_ASSOCIATE);
            m_send_trans.put_data ((uint32)item);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_select_associate (item);

        return client >= 0;
    }

    bool associate_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::associate_table_page_up ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_associate_table_page_up ();

        return client >= 0;
    }

    bool associate_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::associate_table_page_down ()\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_associate_table_page_down ();

        return client >= 0;
    }

    bool update_associate_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::update_associate_table_page_size (" << size << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE);
            m_send_trans.put_data (size);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        helper_update_associate_table_page_size (size);

        return client >= 0;
    }

    bool trigger_property (const String  &property)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::trigger_property (" << property << ")\n";

        int client;
        uint32 context;

        lock ();

        get_focused_context (client, context);

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (SCIM_TRANS_CMD_TRIGGER_PROPERTY);
            m_send_trans.put_data (property);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0;
    }

    bool trigger_helper_property (int            client,
                                  const String  &property)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::trigger_helper_property (" << client << "," << property << ")\n";

        lock ();

        ClientInfo info = socket_get_client_info (client);

        if (client >= 0 && info.type == HELPER_CLIENT) {
            int fe_client;
            uint32 fe_context;
            String fe_uuid;

            fe_uuid = get_focused_context (fe_client, fe_context);

            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

            /* FIXME: We presume that client and context are both less than 65536.
             * Hopefully, it should be true in any UNIXs.
             * So it's ok to combine client and context into one uint32.*/
            m_send_trans.put_data (get_helper_ic (fe_client, fe_context));
            m_send_trans.put_data (fe_uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_TRIGGER_PROPERTY);
            m_send_trans.put_data (property);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        return client >= 0 && info.type == HELPER_CLIENT;
    }

    bool start_helper (const String  &uuid, int client, uint32 context)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::start_helper (" << uuid << ")\n";
        if (uuid.length () <= 0)
            return false;

        lock ();

        if (m_current_toolbar_mode != TOOLBAR_HELPER_MODE || m_current_helper_uuid.compare (uuid) != 0)
        {
            SCIM_DEBUG_MAIN(1) << uuid.c_str () <<  ".....enter run_helper ..............\n";
            m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
        }
        m_current_helper_uuid = uuid;
#ifdef ONE_HELPER_ISE_PROCESS
        if (client == -2)
            get_focused_context (client, context);

        SCIM_DEBUG_MAIN(1) << "[start helper] client : " << client << " context : " << context << "\n";
        uint32 ctx = get_helper_ic (client, context);

        /*HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        if (it == m_helper_client_index.end ())*/
        if (m_helper_uuid_count.find (uuid) == m_helper_uuid_count.end ())
        {
            m_client_context_helper[ctx] = uuid;
            m_current_helper_uuid        = uuid;
            m_helper_uuid_count[uuid]    = 1;
            m_helper_uuid_state[uuid]    = HELPER_HIDED;

            m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
            SCIM_DEBUG_MAIN(1) << "Start HelperISE " << uuid << " ...\n";
        }
        else
        {
            ClientContextUUIDRepository::iterator it2 = m_client_context_helper.find (ctx);
            if (it2 == m_client_context_helper.end ())
            {
                m_client_context_helper[ctx] = uuid;
                m_current_helper_uuid        = uuid;
                m_helper_uuid_count[uuid]    = m_helper_uuid_count[uuid] + 1;
            }

            if (m_current_active_imcontrol_id != -1
                && m_ise_settings != NULL && m_ise_changing)
            {
                show_helper (uuid, m_ise_settings, m_ise_settings_len);
                m_ise_changing = false;
            }

            SCIM_DEBUG_MAIN(1) << "Increment usage count of HelperISE " << uuid << " to "
                        << m_helper_uuid_count[uuid] << "\n";
        }
#endif
        unlock ();

        return true;
    }

    bool stop_helper (const String &helper_uuid, int client, uint32 context)
    {
        String uuid = helper_uuid;
        SCIM_DEBUG_MAIN(1) << "PanelAgent::stop_helper (" << uuid << ")\n";
        if (uuid.length () <= 0)
            return false;

        lock ();

        uint32 ctx = get_helper_ic (client, context);
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        if (it != m_helper_client_index.end ())
        {
            Socket client_socket (it->second.id);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);

            m_ise_exiting = true;
            m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
            m_send_trans.write_to_socket (client_socket);
            SCIM_DEBUG_MAIN(1) << "Stop HelperISE " << uuid << " ...\n";
        }
#ifdef ONE_HELPER_ISE_PROCESS
        if (client == -2)
            get_focused_context (client, context);

        SCIM_DEBUG_MAIN(1) << "[stop helper] client : " << client << " context : " << context << "\n";
        uint32 ctx = get_helper_ic (client, context);

        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
        /*if (it != m_helper_client_index.end ())*/
        if (m_helper_uuid_count.find (uuid) != m_helper_uuid_count.end ())
        {
            m_client_context_helper.erase (ctx);

            uint32 count = m_helper_uuid_count[uuid];
            if (1 == count)
            {
                m_helper_uuid_count.erase (uuid);

                if (it != m_helper_client_index.end ())
                {
                    Socket client_socket (it->second.id);
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (ctx);
                    m_send_trans.put_data (uuid);
                    m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
                    m_send_trans.write_to_socket (client_socket);
                    SCIM_DEBUG_MAIN(1) << "Stop HelperISE " << uuid << " ...\n";
                }
            }
            else
            {
                m_helper_uuid_count[uuid] = count - 1;
                SCIM_DEBUG_MAIN(1) << "Decrement usage count of HelperISE " << uuid
                        << " to " << m_helper_uuid_count[uuid] << "\n";
            }
        }
#endif
        unlock ();

        return true;
    }

    void focus_out_helper (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_OUT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void focus_in_helper (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_FOCUS_IN);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void show_helper (const String &uuid, char *data, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            m_helper_uuid_state[uuid] = HELPER_SHOWED;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SHOW_ISE);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);
        }
        return;
    }

    void hide_helper (const String &uuid)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            m_helper_uuid_state[uuid] = HELPER_HIDED;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_HIDE_ISE);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    bool set_helper_mode (const String &uuid, uint32 &mode)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_MODE);
            m_send_trans.put_data (mode);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_language (const String &uuid, uint32 &language)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_LANGUAGE);
            m_send_trans.put_data (language);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_imdata (const String &uuid, char *imdata, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_IMDATA);
            m_send_trans.put_data (imdata, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_private_key (const String &uuid,
                                 uint32 layout_idx,
                                 uint32 key_idx,
                                 char *buf, size_t len1,
                                 char *value, size_t len2,
                                 bool is_image)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            if (is_image)
                m_send_trans.put_command (ISM_TRANS_CMD_SET_PRIVATE_KEY_BY_IMG);
            else
                m_send_trans.put_command (ISM_TRANS_CMD_SET_PRIVATE_KEY);
            m_send_trans.put_data (layout_idx);
            m_send_trans.put_data (key_idx);
            m_send_trans.put_data (buf, len1);
            m_send_trans.put_data (value, len2);
            m_send_trans.write_to_socket (client_socket);

            return true;
        }

        return false;
    }

    bool set_helper_disable_key (const String &uuid,
                                 uint32 layout_idx,
                                 uint32 key_idx,
                                 bool disabled)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_DISABLE_KEY);
            m_send_trans.put_data (layout_idx);
            m_send_trans.put_data (key_idx);
            m_send_trans.put_data (disabled);
            m_send_trans.write_to_socket (client_socket);

            return true;
        }

        return false;
    }

    bool set_helper_layout (const String &uuid, uint32 &layout)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_LAYOUT);
            m_send_trans.put_data (layout);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_caps_mode (const String &uuid, uint32 &mode)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_CAPS_MODE);
            m_send_trans.put_data (mode);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_im_embedded_editor_im_button_set_label (const String &uuid, char *data, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_IM_BUTTON_SET_LABEL);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_im_embedded_editor_set_preset_text (const String &uuid, char *data, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_PRESET_TEXT);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_im_embedded_editor_set_text (const String &uuid, char *data, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_TEXT);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    bool set_helper_im_embedded_editor_set_max_length (const String &uuid, uint32 &length)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_MAX_LENGTH);
            m_send_trans.put_data (length);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_im_embedded_editor_set_button_senstivity(const String &uuid, uint32 &senstivity)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_BUTTON_SENSTIVITY);
            m_send_trans.put_data (senstivity);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_im_embedded_editor_set_progress_bar(const String &uuid, uint32 &timeout,uint32 &is_showing)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_PROGRESS_BAR);
            m_send_trans.put_data (timeout);
            m_send_trans.put_data (is_showing);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    bool set_helper_im_indicator_count_label (const String &uuid, char *data, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_IM_INDICATOR_SET_COUNT_LABEL);
            m_send_trans.put_data (data, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    void show_isf_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::show_isf_panel ()\n";
        Transaction trans;
        Socket client_socket (client_id);

        m_signal_show_panel ();
    }

    void hide_isf_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::hide_isf_panel ()\n";
        Transaction trans;
        Socket client_socket (client_id);

        m_signal_hide_panel ();
    }

    void show_ise_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::show_ise_panel ()\n";
        char   *data = NULL;
        size_t  len;

        m_current_active_imcontrol_id = client_id;

        if (m_recv_trans.get_data (&data, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                show_helper (m_current_helper_uuid, data, len);
        }

        if (data != NULL)
        {
            if (m_ise_settings != NULL)
                delete [] m_ise_settings;
            m_ise_settings = data;
            m_ise_settings_len = len;
        }
    }

    void hide_ise_panel (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::hide_ise_panel ()\n";
        TOOLBAR_MODE_T mode;

        mode = m_current_toolbar_mode;

        if (client_id == m_current_active_imcontrol_id &&
            TOOLBAR_HELPER_MODE == mode)
        {
            hide_helper (m_current_helper_uuid);
        }
    }

    void set_default_ise (const DEFAULT_ISE_T &ise)
    {
        m_default_ise.type = ise.type;
        m_default_ise.uuid = ise.uuid;
        m_default_ise.name = ise.name;

        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_TYPE), (int)m_default_ise.type);
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_UUID), m_default_ise.uuid);
        scim_global_config_write (String (SCIM_GLOBAL_CONFIG_DEFAULT_ISE_NAME), m_default_ise.name);
        scim_global_config_flush ();
    }

    void set_should_shared_ise (const bool should_shared_ise)
    {
        m_should_shared_ise = should_shared_ise;
    }

    bool get_helper_size (String &uuid, struct rectinfo &info)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {
            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_ACTIVE_ISE_SIZE);

            if (trans.write_to_socket (client_socket)) {
                int cmd;

                trans.clear ();
                if (trans.read_from_socket (client_socket)
                    && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                    && trans.get_data (info.pos_x)
                    && trans.get_data (info.pos_y)
                    && trans.get_data (info.width)
                    && trans.get_data (info.height)) {
                    SCIM_DEBUG_MAIN (1) << "get_helper_size success\n";
                    return true;
                } else {
                    std::cerr << "get_helper_size failed\n";
                    return false;
                }
            }
        }
        return false;
    }

    bool get_helper_imdata (String &uuid, char **imdata, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {

            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_ISE_IMDATA);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (imdata, len))
            {
                SCIM_DEBUG_MAIN (1) << "get_helper_imdata success\n";
                return true;
            }
            else
            {
                std::cerr << "get_helper_imdata failed\n";
            }
        }
        return false;
    }

    bool get_helper_im_embedded_editor_text (String &uuid, char **buf, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {

            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_GET_TEXT);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (buf, len))
            {
                SCIM_DEBUG_MAIN (1) << "get_helper_im_embedded_editor_text success\n";
                return true;
            }
            else
            {
                std::cerr << "get_helper_im_embedded_editor_text failed\n";
            }
        }
        return false;
    }

    bool get_helper_layout (String &uuid, uint32 &layout)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ()) {

            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_LAYOUT);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (layout))
            {
                SCIM_DEBUG_MAIN (1) << "get_helper_layout success\n";
                return true;
            }
            else
            {
                std::cerr << "get_helper_layout failed\n";
            }
        }
        return false;
    }

    bool get_helper_layout_list (String &uuid, std::vector<uint32> &list)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

        if (it != m_helper_client_index.end ()) {

            int    client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;
            Transaction trans;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_data (ctx);
            trans.put_data (uuid);
            trans.put_command (ISM_TRANS_CMD_GET_LAYOUT_LIST);

            int cmd;
            if (trans.write_to_socket (client_socket)
                && trans.read_from_socket (client_socket, 500)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data (list))
            {
                SCIM_DEBUG_MAIN (1) << "get_helper_layout_list success\n";
                return true;
            }
            else
            {
                std::cerr << "get_helper_layout_list failed\n";
            }
        }
        return false;
    }

    void get_ise_size (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_size ()\n";
        struct rectinfo info;
        bool ret = false;

        TOOLBAR_MODE_T mode;

        mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
            ret = get_helper_size (m_current_helper_uuid, info);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);

        if (ret)
        {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (info.pos_x);
            trans.put_data (info.pos_y);
            trans.put_data (info.width);
            trans.put_data (info.height);
        }
        else
        {
            std::cerr << "get_ise_size failed\n";
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    void get_current_ise_rect (rectinfo &ise_rect)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_current_ise_rect ()\n";
        TOOLBAR_MODE_T mode = m_current_toolbar_mode;
        bool           ret  = false;

        if (TOOLBAR_HELPER_MODE == mode)
            ret = get_helper_size (m_current_helper_uuid, ise_rect);

        if (!ret)
        {
            ise_rect.pos_x  = 0;
            ise_rect.pos_y  = 0;
            ise_rect.width  = 0;
            ise_rect.height = 0;
        }
    }

    void set_ise_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_mode ()\n";
        uint32 mode;

        if (m_recv_trans.get_data (mode))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_mode (m_current_helper_uuid, mode);
        }
    }

    void set_ise_layout (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_layout ()\n";
        uint32 layout;

        if (m_recv_trans.get_data (layout))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_layout (m_current_helper_uuid, layout);
        }
    }

    void set_ise_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_language ()\n";
        uint32 language;

        if (m_recv_trans.get_data (language))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_language (m_current_helper_uuid, language);
        }
    }

    void set_isf_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_isf_language ()\n";
        char   *buf = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&buf, len))
        {
            String lang (buf);
            m_signal_set_isf_language (lang);
        }

        if (NULL != buf)
            delete[] buf;
    }

    void set_ise_imdata (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_imdata ()\n";
        char   *imdata = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&imdata, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_imdata (m_current_helper_uuid, imdata, len);
        }

        if (NULL != imdata)
            delete [] imdata;
    }

    void set_ise_im_embedded_editor_im_button_set_label (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_im_button_set_label ()\n";
        char   *data = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&data, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_im_button_set_label (m_current_helper_uuid, data, len);
        }

        if (NULL != data)
            delete [] data;
    }

    void set_ise_im_embedded_editor_set_preset_text (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_set_preset_text ()\n";
        char   *data = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&data, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_set_preset_text (m_current_helper_uuid, data, len);
        }

        if (NULL != data)
            delete [] data;
    }

    void set_ise_im_embedded_editor_set_text (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_set_text ()\n";
        char   *data = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&data, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_set_text (m_current_helper_uuid, data, len);
        }

        if (NULL != data)
            delete [] data;
    }

    void set_ise_im_embedded_editor_set_max_length (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_set_max_length ()\n";
        uint32 length;

        if (m_recv_trans.get_data (length))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_set_max_length (m_current_helper_uuid, length);
        }
    }

    void set_ise_im_embedded_editor_set_button_senstivity (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_set_button_senstivity ()\n";
        uint32 senstivity;
        if (m_recv_trans.get_data (senstivity))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_set_button_senstivity (m_current_helper_uuid, senstivity);
        }
    }

    void set_ise_im_embedded_editor_set_progress_bar (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_embedded_editor_set_progress_bar ()\n";
        uint32 timeout,is_showing;
        if (m_recv_trans.get_data (timeout) && m_recv_trans.get_data (is_showing))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_embedded_editor_set_progress_bar (m_current_helper_uuid, timeout,is_showing);
        }
    }

    void get_ise_im_embedded_editor_text (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_im_embedded_editor_text ()\n";
        char   *buf = NULL;
        size_t  len;
        bool    ret    = false;

        TOOLBAR_MODE_T mode;

        mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
        {
            ret = get_helper_im_embedded_editor_text (m_current_helper_uuid, &buf, len);
        }

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret)
        {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (buf, len);
        }
        else
            trans.put_command (SCIM_TRANS_CMD_FAIL);

        trans.write_to_socket (client_socket);

        if (NULL != buf)
            delete [] buf;
    }

    void set_ise_im_indicator_count_label (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_im_indicator_count_label ()\n";
        char   *data = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&data, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_im_indicator_count_label (m_current_helper_uuid, data, len);
        }

        if (NULL != data)
            delete [] data;
    }

    void get_ise_imdata (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_imdata ()\n";
        char   *imdata = NULL;
        size_t  len;
        bool    ret    = false;

        TOOLBAR_MODE_T mode;

        mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
        {
            ret = get_helper_imdata (m_current_helper_uuid, &imdata, len);
        }

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret)
        {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (imdata, len);
        }
        else
            trans.put_command (SCIM_TRANS_CMD_FAIL);

        trans.write_to_socket (client_socket);

        if (NULL != imdata)
            delete [] imdata;
    }

    void get_ise_layout (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_layout ()\n";
        uint32 layout;
        bool   ret = false;

        TOOLBAR_MODE_T mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
            ret = get_helper_layout (m_current_helper_uuid, layout);

        Transaction trans;
        Socket client_socket (client_id);

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (ret) {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.put_data (layout);
        } else {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
        }

        trans.write_to_socket (client_socket);
    }

    bool get_ise_layout_list (std::vector<uint32> &list)
    {
        bool ret = false;

        TOOLBAR_MODE_T mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
            ret = get_helper_layout_list (m_current_helper_uuid, list);

        return ret;
    }

    void get_active_ise_name (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_active_ise_name ()\n";
        Transaction trans;
        Socket client_socket (client_id);
        char *name = const_cast<char *> (m_current_ise_name.c_str ());
        size_t len = strlen (name) + 1;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        trans.put_data (name, len);
        trans.write_to_socket (client_socket);
    }

    void get_ise_list (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_list ()\n";
        std::vector<String> strlist;
        m_signal_get_ise_list (strlist);

        Transaction trans;
        Socket client_socket (client_id);
        char *buf = NULL;
        size_t len;
        uint32 num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++)
        {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_language_list (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_language_list ()\n";
        std::vector<String> strlist;

        m_signal_get_language_list (strlist);

        Transaction trans;
        Socket client_socket (client_id);
        char *buf = NULL;
        size_t len;
        uint32 num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++)
        {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_all_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_all_language ()\n";
        std::vector<String> strlist;

        m_signal_get_all_language (strlist);

        Transaction trans;
        Socket  client_socket (client_id);
        char   *buf = NULL;
        size_t  len;
        uint32  num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++)
        {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    void get_ise_language (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::get_ise_language ()\n";
        std::vector<String> strlist;
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);

        if (!(m_recv_trans.get_data (&buf, len)))
        {
            trans.clear ();
            trans.put_command (SCIM_TRANS_CMD_REPLY);
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        m_signal_get_ise_language (buf, strlist);

        if (buf)
        {
            delete [] buf;
            buf = NULL;
        }

        uint32 num;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);

        num = strlist.size ();
        trans.put_data (num);
        for (unsigned int i = 0; i < num; i++)
        {
            buf = const_cast<char *>(strlist[i].c_str ());
            len = strlen (buf) + 1;
            trans.put_data (buf, len);
        }

        trans.write_to_socket (client_socket);
    }

    bool reset_ise_option (int client_id)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::resect_ise_option ()\n";

        int    client = -1;
        uint32 context;

        lock ();

        ClientContextUUIDRepository::iterator it = m_client_context_uuids.begin ();
        if (it != m_client_context_uuids.end ()) {
            get_imengine_client_context (it->first, client, context);
        }

        if (client >= 0) {
            Socket client_socket (client);
            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data ((uint32) context);
            m_send_trans.put_command (ISM_TRANS_CMD_RESET_ISE_OPTION);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        Transaction trans;
        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_OK);
        Socket client_socket (client_id);
        trans.write_to_socket (client_socket);

        return client >= 0;
    }

    bool set_helper_char_count (const String &uuid, char *buf, size_t &len)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client     = -1;
            uint32 context = 0;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_INDICATOR_CHAR_COUNT);
            m_send_trans.put_data (buf, len);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }
        return false;
    }

    void set_ise_char_count (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_char_count ()\n";
        char   *buf = NULL;
        size_t  len;

        if (m_recv_trans.get_data (&buf, len))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_char_count (m_current_helper_uuid, buf, len);
        }

        if (NULL != buf)
            delete [] buf;
    }

    bool find_active_ise_by_uuid (String uuid)
    {
        HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();
        for (; iter != m_helper_info_repository.end (); iter++)
        {
            if (!uuid.compare (iter->second.uuid))
                return true;
        }

        return false;
    }

    void set_active_ise_by_uuid (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_active_ise_by_uuid ()\n";
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);
        m_current_active_imcontrol_id = client_id;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (!(m_recv_trans.get_data (&buf, len)))
        {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        String uuid (buf);
        ISE_INFO info;

        if (!m_signal_get_ise_info_by_uuid (uuid, info))
        {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        if (info.type == TOOLBAR_KEYBOARD_MODE)
        {
            m_signal_set_active_ise_by_uuid (uuid, 1);
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }
        else if (info.option & ISM_ISE_HIDE_IN_CONTROL_PANEL)
        {
            int count = _id_count--;
            if (info.type == TOOLBAR_HELPER_MODE)
            {
                m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                if (uuid != m_current_helper_uuid)
                    m_last_helper_uuid = m_current_helper_uuid;
                start_helper (uuid, count, DEFAULT_CONTEXT_VALUE);
                IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);
                if (iter == m_imcontrol_repository.end ())
                {
                    struct IMControlStub stub;
                    stub.count.clear ();
                    stub.info.clear ();
                    stub.info.push_back (info);
                    stub.count.push_back (count);
                    m_imcontrol_repository[client_id] = stub;
                }
                else
                {
                    iter->second.info.push_back (info);
                    iter->second.count.push_back (count);
                }
            }
        }
        else
            m_signal_set_active_ise_by_uuid (uuid, 1);

        if (find_active_ise_by_uuid (uuid))
        {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
        }
        else
            m_ise_pending_repository[uuid] = client_id;

        if (NULL != buf)
            delete[] buf;
    }

    bool find_active_ise_by_name (String name)
    {
        HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();
        for (; iter != m_helper_info_repository.end (); iter++)
        {
            if (!name.compare (iter->second.name))
                return true;
        }

        return false;
    }

    void set_active_ise_by_name (int client_id)
    {
        char   *buf = NULL;
        size_t  len;
        Transaction trans;
        Socket client_socket (client_id);
        m_current_active_imcontrol_id = client_id;

        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        if (!(m_recv_trans.get_data (&buf, len)))
        {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        String name (buf);
        ISE_INFO info;

        if (!m_signal_get_ise_info_by_name (name, info))
        {
            trans.put_command (SCIM_TRANS_CMD_FAIL);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }

        if (info.type == TOOLBAR_KEYBOARD_MODE)
        {
            m_signal_set_keyboard_ise (ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_NAME, name);
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
            if (NULL != buf)
                delete[] buf;
            return;
        }
        else if (info.option & ISM_ISE_HIDE_IN_CONTROL_PANEL)
        {
            int count = _id_count--;
            if (info.type == TOOLBAR_HELPER_MODE)
            {
                m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                if (info.uuid != m_current_helper_uuid)
                    m_last_helper_uuid = m_current_helper_uuid;
                start_helper (info.uuid, count, DEFAULT_CONTEXT_VALUE);
                IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);
                if (iter == m_imcontrol_repository.end ())
                {
                    struct IMControlStub stub;
                    stub.count.clear ();
                    stub.info.clear ();
                    stub.info.push_back (info);
                    stub.count.push_back (count);
                    m_imcontrol_repository[client_id] = stub;
                }
                else
                {
                    iter->second.info.push_back (info);
                    iter->second.count.push_back (count);
                }
            }
        }
        else
            m_signal_set_active_ise_by_name (name);

        if (find_active_ise_by_name (name))
        {
            trans.put_command (SCIM_TRANS_CMD_OK);
            trans.write_to_socket (client_socket);
        }
        else
            m_ise_pending_repository[name] = client_id;

        if (NULL != buf)
            delete[] buf;
    }

    void update_isf_control_status (const bool showed)
    {
        for (ClientRepository::iterator iter = m_client_repository.begin ();
             iter != m_client_repository.end (); ++iter)
        {
            if (IMCONTROL_CLIENT == iter->second.type
                && iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                if (showed)
                    trans.put_command (ISM_TRANS_CMD_ISF_CONTROL_SHOWED);
                else
                    trans.put_command (ISM_TRANS_CMD_ISF_CONTROL_HIDED);
                trans.write_to_socket (client_socket);
                break;
            }
        }
        return;
    }

    void set_ise_private_key (int client_id, bool is_image)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_private_key ()\n";
        uint32 layout_idx, key_idx;
        char  *label = NULL, *value = NULL;
        size_t len1, len2;

        if (m_recv_trans.get_data (layout_idx)
            && m_recv_trans.get_data (key_idx)
            && m_recv_trans.get_data (&label, len1)
            && m_recv_trans.get_data (&value, len2))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_private_key (m_current_helper_uuid,
                                        layout_idx,
                                        key_idx,
                                        label,
                                        len1,
                                        value,
                                        len2,
                                        is_image);
        }

        if (NULL != label)
            delete[] label;
        if (NULL != value)
            delete[] value;
    }

    void set_ise_disable_key (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_disable_key ()\n";
        uint32 layout_idx, key_idx, disabled;

        if (m_recv_trans.get_data (layout_idx)
            && m_recv_trans.get_data (key_idx)
            && m_recv_trans.get_data (disabled))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_disable_key (m_current_helper_uuid,
                                        layout_idx,
                                        key_idx,
                                        disabled);
        }
    }

    int get_active_ise_list (std::vector<String> &strlist)
    {
        strlist.clear ();
        m_helper_manager.get_active_ise_list (strlist);
        return (int)(strlist.size ());
    }

    void reset_helper_context (const String &uuid)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_RESET_ISE_CONTEXT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void reset_ise_context (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::reset_ise_context ()\n";
        TOOLBAR_MODE_T mode;

        mode = m_current_toolbar_mode;

        if (TOOLBAR_HELPER_MODE == mode)
        {
            reset_helper_context (m_current_helper_uuid);
        }
    }

    bool set_helper_screen_direction (const String &uuid, uint32 &direction)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            int client;
            uint32 context;
            Socket client_socket (it->second.id);
            uint32 ctx;

            get_focused_context (client, context);
            ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (ISM_TRANS_CMD_SET_ISE_SCREEN_DIRECTION);
            m_send_trans.put_data (direction);
            m_send_trans.write_to_socket (client_socket);
            return true;
        }

        return false;
    }

    void set_ise_screen_direction (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_screen_direction ()\n";
        uint32 direction;

        if (m_recv_trans.get_data (direction))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_screen_direction (m_current_helper_uuid, direction);
        }
    }

    void set_ise_caps_mode (int client_id)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::set_ise_caps_mode ()\n";
        uint32 mode;

        if (m_recv_trans.get_data (mode))
        {
            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                set_helper_caps_mode (m_current_helper_uuid, mode);
        }
    }

    int send_display_name (String &name)
    {
        return m_helper_manager.send_display_name (name);
    }

    bool reload_config (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::reload_config ()\n";

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command (SCIM_TRANS_CMD_RELOAD_CONFIG);

        for (ClientRepository::iterator it = m_client_repository.begin ();
             it != m_client_repository.end (); ++it) {

            if (it->second.type == IMCONTROL_ACT_CLIENT
                || it->second.type == IMCONTROL_CLIENT
                || it->second.type == HELPER_ACT_CLIENT)
                continue;

            Socket client_socket (it->first);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();
        return true;
    }

    bool exit (void)
    {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::exit ()\n";

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);

        for (ClientRepository::iterator it = m_client_repository.begin ();
             it != m_client_repository.end (); ++it) {
            Socket client_socket (it->first);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();

        stop ();

        return true;
    }

    bool filter_event (int fd)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::filter_event ()\n";

        return m_socket_server.filter_event (fd);
    }

    bool filter_exception_event (int fd)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::filter_exception_event ()\n";

        return m_socket_server.filter_exception_event (fd);
    }

    int get_server_id ()
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::get_server_id ()\n";

        return m_socket_server.get_id ();
    }

    void set_ise_changing (bool changing)
    {
        SCIM_DEBUG_MAIN (1) << "PanelAgent::set_ise_changing ()\n";
        m_ise_changing = changing;
    }

    void update_ise_list (std::vector<String> &strList)
    {
        /* send ise list to frontend */
        String dst_str = scim_combine_string_list (strList);
        m_helper_manager.send_ise_list (dst_str);

        /* request PanelClient to update keyboard ise list */
        update_keyboard_ise_list ();
    }

    Connection signal_connect_reload_config              (PanelAgentSlotVoid                *slot)
    {
        return m_signal_reload_config.connect (slot);
    }

    Connection signal_connect_turn_on                    (PanelAgentSlotVoid                *slot)
    {
        return m_signal_turn_on.connect (slot);
    }

    Connection signal_connect_turn_off                   (PanelAgentSlotVoid                *slot)
    {
        return m_signal_turn_off.connect (slot);
    }

    Connection signal_connect_show_panel                 (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_panel.connect (slot);
    }

    Connection signal_connect_hide_panel                 (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_panel.connect (slot);
    }

    Connection signal_connect_update_screen              (PanelAgentSlotInt                 *slot)
    {
        return m_signal_update_screen.connect (slot);
    }

    Connection signal_connect_update_spot_location       (PanelAgentSlotIntIntInt           *slot)
    {
        return m_signal_update_spot_location.connect (slot);
    }

    Connection signal_connect_update_factory_info        (PanelAgentSlotFactoryInfo         *slot)
    {
        return m_signal_update_factory_info.connect (slot);
    }

    Connection signal_connect_start_default_ise          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_start_default_ise.connect (slot);
    }

    Connection signal_connect_set_candidate_ui           (PanelAgentSlotIntInt              *slot)
    {
        return m_signal_set_candidate_ui.connect (slot);
    }

    Connection signal_connect_get_candidate_ui           (PanelAgentSlotIntInt2             *slot)
    {
        return m_signal_get_candidate_ui.connect (slot);
    }

    Connection signal_connect_set_candidate_position     (PanelAgentSlotIntInt              *slot)
    {
        return m_signal_set_candidate_position.connect (slot);
    }

    Connection signal_connect_get_candidate_rect         (PanelAgentSlotRect                *slot)
    {
        return m_signal_get_candidate_rect.connect (slot);
    }

    Connection signal_connect_set_keyboard_ise           (PanelAgentSlotIntString           *slot)
    {
        return m_signal_set_keyboard_ise.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise           (PanelAgentSlotString2             *slot)
    {
        return m_signal_get_keyboard_ise.connect (slot);
    }

    Connection signal_connect_show_help                  (PanelAgentSlotString              *slot)
    {
        return m_signal_show_help.connect (slot);
    }

    Connection signal_connect_show_factory_menu          (PanelAgentSlotFactoryInfoVector   *slot)
    {
        return m_signal_show_factory_menu.connect (slot);
    }

    Connection signal_connect_show_preedit_string        (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_preedit_string.connect (slot);
    }

    Connection signal_connect_show_aux_string            (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_aux_string.connect (slot);
    }

    Connection signal_connect_show_lookup_table          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_lookup_table.connect (slot);
    }

    Connection signal_connect_show_associate_table       (PanelAgentSlotVoid                *slot)
    {
        return m_signal_show_associate_table.connect (slot);
    }

    Connection signal_connect_hide_preedit_string        (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_preedit_string.connect (slot);
    }

    Connection signal_connect_hide_aux_string            (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_aux_string.connect (slot);
    }

    Connection signal_connect_hide_lookup_table          (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_lookup_table.connect (slot);
    }

    Connection signal_connect_hide_associate_table       (PanelAgentSlotVoid                *slot)
    {
        return m_signal_hide_associate_table.connect (slot);
    }

    Connection signal_connect_update_preedit_string      (PanelAgentSlotAttributeString     *slot)
    {
        return m_signal_update_preedit_string.connect (slot);
    }

    Connection signal_connect_update_preedit_caret       (PanelAgentSlotInt                 *slot)
    {
        return m_signal_update_preedit_caret.connect (slot);
    }

    Connection signal_connect_update_aux_string          (PanelAgentSlotAttributeString     *slot)
    {
        return m_signal_update_aux_string.connect (slot);
    }

    Connection signal_connect_update_lookup_table        (PanelAgentSlotLookupTable         *slot)
    {
        return m_signal_update_lookup_table.connect (slot);
    }

    Connection signal_connect_update_associate_table     (PanelAgentSlotLookupTable         *slot)
    {
        return m_signal_update_associate_table.connect (slot);
    }

    Connection signal_connect_register_properties        (PanelAgentSlotPropertyList        *slot)
    {
        return m_signal_register_properties.connect (slot);
    }

    Connection signal_connect_update_property            (PanelAgentSlotProperty            *slot)
    {
        return m_signal_update_property.connect (slot);
    }

    Connection signal_connect_register_helper_properties (PanelAgentSlotIntPropertyList     *slot)
    {
        return m_signal_register_helper_properties.connect (slot);
    }

    Connection signal_connect_update_helper_property     (PanelAgentSlotIntProperty         *slot)
    {
        return m_signal_update_helper_property.connect (slot);
    }

    Connection signal_connect_register_helper            (PanelAgentSlotIntHelperInfo       *slot)
    {
        return m_signal_register_helper.connect (slot);
    }

    Connection signal_connect_remove_helper              (PanelAgentSlotInt                 *slot)
    {
        return m_signal_remove_helper.connect (slot);
    }

    Connection signal_connect_set_active_ise_by_uuid     (PanelAgentSlotStringBool          *slot)
    {
        return m_signal_set_active_ise_by_uuid.connect (slot);
    }

    Connection signal_connect_set_active_ise_by_name     (PanelAgentSlotString                 *slot)
    {
        return m_signal_set_active_ise_by_name.connect (slot);
    }

    Connection signal_connect_focus_in                   (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_focus_in.connect (slot);
    }

    Connection signal_connect_focus_out                  (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_focus_out.connect (slot);
    }

    Connection signal_connect_get_ise_list               (PanelAgentSlotBoolStringVector       *slot)
    {
        return m_signal_get_ise_list.connect (slot);
    }

    Connection signal_connect_get_keyboard_ise_list      (PanelAgentSlotBoolStringVector       *slot)
    {
        return m_signal_get_keyboard_ise_list.connect (slot);
    }
    Connection signal_connect_launch_helper_ise_list_selection (PanelAgentSlotInt              *slot)
    {
        return m_signal_launch_helper_ise_list_selection.connect (slot);
    }

    Connection signal_connect_get_language_list          (PanelAgentSlotStringVector           *slot)
    {
        return m_signal_get_language_list.connect (slot);
    }

    Connection signal_connect_get_all_language           (PanelAgentSlotStringVector           *slot)
    {
        return m_signal_get_all_language.connect (slot);
    }

    Connection signal_connect_get_ise_language           (PanelAgentSlotStrStringVector        *slot)
    {
        return m_signal_get_ise_language.connect (slot);
    }

    Connection signal_connect_set_isf_language           (PanelAgentSlotString                 *slot)
    {
        return m_signal_set_isf_language.connect (slot);
    }

    Connection signal_connect_get_ise_info_by_uuid       (PanelAgentSlotStringISEINFO          *slot)
    {
        return m_signal_get_ise_info_by_uuid.connect (slot);
    }

    Connection signal_connect_get_ise_info_by_name       (PanelAgentSlotStringISEINFO          *slot)
    {
        return m_signal_get_ise_info_by_name.connect (slot);
    }

    Connection signal_connect_send_key_event             (PanelAgentSlotKeyEvent               *slot)
    {
        return m_signal_send_key_event.connect (slot);
    }

    Connection signal_connect_accept_connection          (PanelAgentSlotInt                    *slot)
    {
        return m_signal_accept_connection.connect (slot);
    }

    Connection signal_connect_close_connection           (PanelAgentSlotInt                    *slot)
    {
        return m_signal_close_connection.connect (slot);
    }

    Connection signal_connect_exit                       (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_exit.connect (slot);
    }

    Connection signal_connect_transaction_start          (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_transaction_start.connect (slot);
    }

    Connection signal_connect_transaction_end            (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_transaction_end.connect (slot);
    }

    Connection signal_connect_lock                       (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_lock.connect (slot);
    }

    Connection signal_connect_unlock                     (PanelAgentSlotVoid                   *slot)
    {
        return m_signal_unlock.connect (slot);
    }

private:
    bool socket_check_client_connection (const Socket &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_check_client_connection (" << client.get_id () << ")\n";

        unsigned char buf [sizeof(uint32)];

        int nbytes = client.read_with_timeout (buf, sizeof(uint32), m_socket_timeout);

        if (nbytes == sizeof (uint32))
            return true;

        if (nbytes < 0) {
            SCIM_DEBUG_MAIN (4) << "Error occurred when reading socket: " << client.get_error_message () << ".\n";
        } else {
            SCIM_DEBUG_MAIN (4) << "Timeout when reading socket.\n";
        }

        return false;
    }

    void socket_accept_callback                 (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (2) << "PanelAgent::socket_accept_callback (" << client.get_id () << ")\n";

        lock ();
        if (m_should_exit) {
            SCIM_DEBUG_MAIN (3) << "Exit Socket Server Thread.\n";
            server->shutdown ();
        } else
            m_signal_accept_connection (client.get_id ());
        unlock ();
    }

    void socket_receive_callback                (SocketServer   *server,
                                                 const Socket   &client)
    {
        int     client_id = client.get_id ();
        int     cmd     = 0;
        uint32  key     = 0;
        uint32  context = 0;
        String  uuid;
        bool    current = false;
        bool    last    = false;

        ClientInfo client_info;

        SCIM_DEBUG_MAIN (1) << "PanelAgent::socket_receive_callback (" << client_id << ")\n";

        /* If the connection is closed then close this client. */
        if (!socket_check_client_connection (client)) {
            socket_close_connection (server, client);
            return;
        }

        client_info = socket_get_client_info (client_id);

        /* If it's a new client, then request to open the connection first. */
        if (client_info.type == UNKNOWN_CLIENT) {
            socket_open_connection (server, client);
            return;
        }

        /* If can not read the transaction,
         * or the transaction is not started with SCIM_TRANS_CMD_REQUEST,
         * or the key is mismatch,
         * just return. */
        if (!m_recv_trans.read_from_socket (client, m_socket_timeout) ||
            !m_recv_trans.get_command (cmd) || cmd != SCIM_TRANS_CMD_REQUEST ||
            !m_recv_trans.get_data (key)    || key != (uint32) client_info.key)
            return;

        if (client_info.type == FRONTEND_CLIENT) {
            if (m_recv_trans.get_data (context)) {
                SCIM_DEBUG_MAIN (1) << "PanelAgent::FrontEnd Client, context = " << context << "\n";
                socket_transaction_start();
                while (m_recv_trans.get_command (cmd)) {
                    SCIM_DEBUG_MAIN (3) << "PanelAgent::cmd = " << cmd << "\n";

                    if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT) {
                        if (m_recv_trans.get_data (uuid)) {
                            SCIM_DEBUG_MAIN (2) << "PanelAgent::register_input_context (" << client_id << "," << "," << context << "," << uuid << ")\n";
                            uint32 ctx = get_helper_ic (client_id, context);
                            m_client_context_uuids [ctx] = uuid;
                        }
                        continue;
                    }

                    if (cmd == ISM_TRANS_CMD_PANEL_START_DEFAULT_ISE) {
                        if ((m_default_ise.type == TOOLBAR_HELPER_MODE) && (m_default_ise.uuid.length () > 0))
                        {
                            m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                            start_helper (m_default_ise.uuid, client_id, context);
                        }
                        else if (m_default_ise.type == TOOLBAR_KEYBOARD_MODE)
                        {
                            m_current_toolbar_mode = TOOLBAR_KEYBOARD_MODE;
                        }
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT) {
                        uint32 ctx = get_helper_ic (client_id, context);
                        m_client_context_uuids.erase (ctx);
#ifdef ONE_HELPER_ISE_PROCESS
                        if (m_client_context_helper.find (ctx) != m_client_context_helper.end ())
                            stop_helper (m_client_context_helper[ctx], client_id, context);
#endif
                        if (ctx == get_helper_ic (m_current_socket_client, m_current_client_context))
                        {
                            lock ();
                            m_current_socket_client  = m_last_socket_client;
                            m_current_client_context = m_last_client_context;
                            m_current_context_uuid   = m_last_context_uuid;
                            m_last_socket_client     = -1;
                            m_last_client_context    = 0;
                            m_last_context_uuid      = String ("");
                            if (m_current_socket_client == -1)
                            {
                                unlock ();
                                socket_update_control_panel ();
                            }
                            else
                                unlock ();
                        }
                        else if (ctx == get_helper_ic (m_last_socket_client, m_last_client_context))
                        {
                            lock ();
                            m_last_socket_client  = -1;
                            m_last_client_context = 0;
                            m_last_context_uuid   = String ("");
                            unlock ();
                        }
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT) {
                        socket_reset_input_context (client_id, context);
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_FOCUS_IN) {
                        get_helper_ic (client_id, context);
                        m_signal_focus_in ();
                        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                            focus_in_helper (m_current_helper_uuid, client_id, context);
#ifdef ONE_HELPER_ISE_PROCESS
                        uint32 ctx = get_helper_ic (client_id, context);
                        ClientContextUUIDRepository::iterator it = m_client_context_helper.find (ctx);
                        if (it != m_client_context_helper.end ())
                        {
                            if (m_should_shared_ise)
                            {
                                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                                {
                                    if (m_current_helper_uuid != it->second)
                                    {
                                        stop_helper (it->second, client_id, context);
                                        start_helper (m_current_helper_uuid, client_id, context);
                                    }
                                    focus_in_helper (m_current_helper_uuid, client_id, context);
                                }
                                else if (TOOLBAR_KEYBOARD_MODE == m_current_toolbar_mode)
                                    stop_helper (it->second, client_id, context);
                            }
                            else
                            {
                                /* focus in the helper if the context is associated with some helper */
                                m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                                m_current_helper_uuid  = it->second;
                                focus_in_helper (m_current_helper_uuid, client_id, context);
                            }
                        }
                        else
                        {
                            if (m_should_shared_ise)
                            {
                                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                                {
                                    start_helper (m_current_helper_uuid, client_id, context);
                                    focus_in_helper (m_current_helper_uuid, client_id, context);
                                }
                            }
                            else
                            {
                                /* come here if the context is associated with some imengine */
                                m_current_toolbar_mode = TOOLBAR_KEYBOARD_MODE;
                            }
                        }
#endif
                        if (m_recv_trans.get_data (uuid)) {
                            SCIM_DEBUG_MAIN (2) << "PanelAgent::focus_in (" << client_id << "," << "," << context << "," << uuid << ")\n";
                            lock ();
                            if (m_current_socket_client >= 0) {
                                m_last_socket_client  = m_current_socket_client;
                                m_last_client_context = m_current_client_context;
                                m_last_context_uuid   = m_current_context_uuid;
                            }
                            m_current_socket_client  = client_id;
                            m_current_client_context = context;
                            m_current_context_uuid   = uuid;
                            unlock ();
                        }
                        continue;
                    }

                    if (cmd == ISM_TRANS_CMD_TURN_ON_LOG) {
                        socket_turn_on_log ();
                        continue;
                    }

                    current = last = false;
                    uuid.clear ();

                    /* Get the context uuid from the client context registration table. */
                    {
                        ClientContextUUIDRepository::iterator it = m_client_context_uuids.find (get_helper_ic (client_id, context));
                        if (it != m_client_context_uuids.end ())
                            uuid = it->second;
                    }

                    if (m_current_socket_client == client_id && m_current_client_context == context) {
                        current = true;
                        if (!uuid.length ()) uuid = m_current_context_uuid;
                    } else if (m_last_socket_client == client_id && m_last_client_context == context) {
                        last = true;
                        if (!uuid.length ()) uuid = m_last_context_uuid;
                    }

                    /* Skip to the next command and continue, if it's not current or last focused. */
                    if (!uuid.length ()) {
                        SCIM_DEBUG_MAIN (3) << "PanelAgent:: Couldn't find context uuid.\n";
                        while (m_recv_trans.get_data_type () != SCIM_TRANS_DATA_COMMAND && m_recv_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
                            m_recv_trans.skip_data ();
                        continue;
                    }

                    if (cmd == SCIM_TRANS_CMD_START_HELPER) {
                        socket_start_helper (client_id, context, uuid);
                        continue;
                    }
                    else if (cmd == SCIM_TRANS_CMD_SEND_HELPER_EVENT) {
                        socket_send_helper_event (client_id, context, uuid);
                        continue;
                    }
                    else if (cmd == SCIM_TRANS_CMD_STOP_HELPER) {
                        socket_stop_helper (client_id, context, uuid);
                        continue;
                    }

                    /* If it's not focused, just continue. */
                    if ((!current && !last) || (last && m_current_socket_client >= 0)) {
                        SCIM_DEBUG_MAIN (3) << "PanelAgent::Not current focused.\n";
                        while (m_recv_trans.get_data_type () != SCIM_TRANS_DATA_COMMAND && m_recv_trans.get_data_type () != SCIM_TRANS_DATA_UNKNOWN)
                            m_recv_trans.skip_data ();
                        continue;
                    }

                    /* Client must focus in before do any other things. */
                    if (cmd == SCIM_TRANS_CMD_PANEL_TURN_ON)
                        socket_turn_on ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_TURN_OFF)
                        socket_turn_off ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_SCREEN)
                        socket_update_screen ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION)
                        socket_update_spot_location ();
                    else if (cmd == ISM_TRANS_CMD_UPDATE_CURSOR_POSITION)
                        socket_update_cursor_position ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO)
                        socket_update_factory_info ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING)
                        socket_show_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING)
                        socket_show_aux_string ();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE)
                        socket_show_lookup_table ();
                    else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE)
                        socket_show_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING)
                        socket_hide_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING)
                        socket_hide_aux_string ();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE)
                        socket_hide_lookup_table ();
                    else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE)
                        socket_hide_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING)
                        socket_update_preedit_string ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET)
                        socket_update_preedit_caret ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING)
                        socket_update_aux_string ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE)
                        socket_update_lookup_table ();
                    else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE)
                        socket_update_associate_table ();
                    else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES)
                        socket_register_properties ();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY)
                        socket_update_property ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_HELP)
                        socket_show_help ();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU)
                        socket_show_factory_menu ();
                    else if (cmd == SCIM_TRANS_CMD_FOCUS_OUT) {
                        m_signal_focus_out ();
                        lock ();
                        TOOLBAR_MODE_T mode = m_current_toolbar_mode;

                        if (TOOLBAR_HELPER_MODE == mode)
                            focus_out_helper (m_current_helper_uuid, client_id, context);

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
                }
                socket_transaction_end ();
            }
        } else if (client_info.type == HELPER_CLIENT) {
            socket_transaction_start ();
            while (m_recv_trans.get_command (cmd)) {
                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_HELPER) {
                    socket_helper_register_helper (client_id);
                }
            }
            socket_transaction_end ();
        }else if (client_info.type == HELPER_ACT_CLIENT) {
            socket_transaction_start ();
            while (m_recv_trans.get_command (cmd)) {
                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER) {
                    socket_helper_register_helper_passive (client_id);
                } else if (cmd == SCIM_TRANS_CMD_COMMIT_STRING) {
                    socket_helper_commit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING) {
                    socket_helper_show_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING) {
                    socket_show_aux_string ();
                } else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE) {
                    socket_show_lookup_table ();
                } else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE) {
                    socket_show_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING) {
                    socket_helper_hide_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING) {
                    socket_hide_aux_string ();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE) {
                    socket_hide_lookup_table ();
                } else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE) {
                    socket_hide_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING) {
                    socket_helper_update_preedit_string (client_id);
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING) {
                    socket_update_aux_string ();
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE) {
                    socket_update_lookup_table ();
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE) {
                    socket_update_associate_table ();
                } else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT ||
                           cmd == SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT) {
                    socket_helper_send_key_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_FORWARD_KEY_EVENT) {
                    socket_helper_forward_key_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT) {
                    socket_helper_send_imengine_event (client_id);
                } else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES) {
                    socket_helper_register_properties (client_id);
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY) {
                    socket_helper_update_property (client_id);
                } else if (cmd == SCIM_TRANS_CMD_RELOAD_CONFIG) {
                    reload_config ();
                    m_signal_reload_config ();
                } else if (cmd == ISM_TRANS_CMD_ISE_PANEL_HIDED) {
                    socket_helper_update_state_hided (client_id);
                } else if (cmd == ISM_TRANS_CMD_ISE_PANEL_SHOWED) {
                    socket_helper_update_state_showed (client_id);
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT) {
                    socket_helper_update_input_context (client_id);
                } else if (cmd == ISM_TRANS_CMD_ISE_RESULT_TO_IMCONTROL) {
                    socket_helper_commit_ise_result_to_imcontrol (client_id);
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST) {
                    socket_get_keyboard_ise_list ();
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_UI) {
                    socket_set_candidate_ui ();
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_UI) {
                    socket_get_candidate_ui ();
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_POSITION) {
                    socket_set_candidate_position ();
                } else if (cmd == ISM_TRANS_CMD_HIDE_CANDIDATE) {
                    socket_hide_candidate ();
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_RECT) {
                    socket_get_candidate_rect ();
                } else if (cmd == ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_NAME) {
                    socket_set_keyboard_ise (ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_NAME);
                } else if (cmd == ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID) {
                    socket_set_keyboard_ise (ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID);
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE) {
                    socket_get_keyboard_ise ();
                } else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_STRING) {
                    socket_helper_commit_im_embedded_editor_string (client_id);
                } else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_CHANGED) {
                    socket_helper_im_embedded_editor_changed (client_id);
                } else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_PREEDIT_CHANGED) {
                    socket_helper_im_embedded_editor_preedit_changed (client_id);
                } else if(cmd == ISM_TRANS_CMD_LAUNCH_HELPER_ISE_LIST_SELECTION){
                    socket_helper_launch_helper_ise_list_selection();
                }
            }
            socket_transaction_end ();
        }
        else if (client_info.type == IMCONTROL_ACT_CLIENT)
        {
            socket_transaction_start ();

            while (m_recv_trans.get_command (cmd))
            {
                if (cmd == ISM_TRANS_CMD_SHOW_ISF_CONTROL)
                    show_isf_panel (client_id);
                else if (cmd == ISM_TRANS_CMD_HIDE_ISF_CONTROL)
                    hide_isf_panel (client_id);
                else if (cmd == ISM_TRANS_CMD_SHOW_ISE_PANEL)
                    show_ise_panel (client_id);
                else if (cmd == ISM_TRANS_CMD_HIDE_ISE_PANEL)
                    hide_ise_panel (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE_SIZE)
                    get_ise_size (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ISE_MODE)
                    set_ise_mode (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ISE_LANGUAGE)
                    set_ise_language (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ISE_IMDATA)
                    set_ise_imdata (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ISE_IMDATA)
                    get_ise_imdata (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE_NAME)
                    get_active_ise_name (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_NAME)
                    set_active_ise_by_name (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID)
                    set_active_ise_by_uuid (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_PRIVATE_KEY)
                    set_ise_private_key (client_id, false);
                else if (cmd == ISM_TRANS_CMD_SET_PRIVATE_KEY_BY_IMG)
                    set_ise_private_key (client_id, true);
                else if (cmd == ISM_TRANS_CMD_SET_DISABLE_KEY)
                    set_ise_disable_key (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_LAYOUT)
                    get_ise_layout (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_LAYOUT)
                    set_ise_layout (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_CAPS_MODE)
                    set_ise_caps_mode (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ISE_LIST)
                    get_ise_list (client_id);
                else if (cmd == ISM_TRANS_CMD_RESET_ISE_OPTION)
                    reset_ise_option (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_LANGUAGE_LIST)
                    get_language_list (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ALL_LANGUAGE_LIST)
                    get_all_language (client_id);
                else if (cmd == ISM_TRANS_CMD_GET_ISE_LANGUAGE)
                    get_ise_language (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ISF_LANGUAGE)
                    set_isf_language (client_id);
                else if (cmd == ISM_TRANS_CMD_RESET_ISE_CONTEXT)
                    reset_ise_context (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_ISE_SCREEN_DIRECTION)
                    set_ise_screen_direction (client_id);
                else if (cmd == ISM_TRANS_CMD_SET_INDICATOR_CHAR_COUNT)
                    set_ise_char_count (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_IM_BUTTON_SET_LABEL)
                    set_ise_im_embedded_editor_im_button_set_label (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_PRESET_TEXT)
                    set_ise_im_embedded_editor_set_preset_text (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_TEXT)
                    set_ise_im_embedded_editor_set_text (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_SET_MAX_LENGTH)
                    set_ise_im_embedded_editor_set_max_length (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_BUTTON_SENSTIVITY)
                    set_ise_im_embedded_editor_set_button_senstivity (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_PROGRESS_BAR)
                    set_ise_im_embedded_editor_set_progress_bar (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_GET_TEXT)
                    get_ise_im_embedded_editor_text (client_id);
                else if (cmd == ISM_TRANS_CMD_IM_INDICATOR_SET_COUNT_LABEL)
                    set_ise_im_indicator_count_label (client_id);
            }

            socket_transaction_end ();
        }
    }

    void socket_exception_callback              (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (2) << "PanelAgent::socket_exception_callback (" << client.get_id () << ")\n";

        socket_close_connection (server, client);
    }

    bool socket_open_connection                 (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_open_connection (" << client.get_id () << ")\n";

        uint32 key;
        String type = scim_socket_accept_connection (key,
                                                     String ("Panel"),
                                                     String ("FrontEnd,Helper,Helper_Active,IMControl_Active,IMControl_Passive"),
                                                     client,
                                                     m_socket_timeout);

        if (type.length ()) {
            ClientInfo info;
            info.key = key;
            info.type = ((type == "FrontEnd") ? FRONTEND_CLIENT :
                        ((type == "IMControl_Active") ? IMCONTROL_ACT_CLIENT :
                        ((type == "Helper_Active") ? HELPER_ACT_CLIENT :
                        ((type == "IMControl_Passive") ? IMCONTROL_CLIENT : HELPER_CLIENT))));

            SCIM_DEBUG_MAIN (4) << "Add client to repository. Type=" << type << " key=" << key << "\n";
            lock ();
            m_client_repository [client.get_id ()] = info;

            if (info.type == IMCONTROL_ACT_CLIENT)
            {
                m_pending_active_imcontrol_id = client.get_id ();
            }
            else if (info.type == IMCONTROL_CLIENT)
            {
                m_imcontrol_map [m_pending_active_imcontrol_id] = client.get_id();
                m_pending_active_imcontrol_id = -1;
            }

            const_cast<Socket &>(client).set_nonblock_mode ();

            unlock ();
            return true;
        }

        SCIM_DEBUG_MAIN (4) << "Close client connection " << client.get_id () << "\n";
        server->close_connection (client);
        return false;
    }

    void socket_close_connection                (SocketServer   *server,
                                                 const Socket   &client)
    {
        SCIM_DEBUG_MAIN (3) << "PanelAgent::socket_close_connection (" << client.get_id () << ")\n";

        lock ();

        m_signal_close_connection (client.get_id ());

        ClientInfo client_info = socket_get_client_info (client.get_id ());

        m_client_repository.erase (client.get_id ());

        server->close_connection (client);

        /* Exit panel if there is no connected client anymore. */
        if (client_info.type != UNKNOWN_CLIENT && m_client_repository.size () == 0 && !m_should_resident) {
            SCIM_DEBUG_MAIN (4) << "Exit Socket Server Thread.\n";
            server->shutdown ();
            m_signal_exit.emit ();
        }

        unlock ();

        if (client_info.type == FRONTEND_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a FrontEnd client.\n";
            /* The focused client is closed. */
            if (m_current_socket_client == client.get_id ()) {
                lock ();
                m_current_socket_client = -1;
                m_current_client_context = 0;
                m_current_context_uuid = String ("");
                unlock ();

                socket_transaction_start ();
                socket_turn_off ();
                socket_transaction_end ();
            }

            if (m_last_socket_client == client.get_id ()) {
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
                if ((it->first & 0xFFFF) == (client.get_id () & 0xFFFF))
                    ctx_list.push_back (it->first);
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                m_client_context_uuids.erase (ctx_list [i]);

            int client_id = client.get_id ();

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
                            if (pise != m_helper_client_index.end ())
                            {
                                m_send_trans.clear ();
                                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                                m_send_trans.put_data (it->first);
                                m_send_trans.put_data (uuid);
                                m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
                                m_send_trans.write_to_socket (pise->second.id);
                            }
                            SCIM_DEBUG_MAIN(1) << "Stop HelperISE " << uuid << " ...\n";
                        }
                        else
                        {
                            m_helper_uuid_count[uuid] = count - 1;
                            focus_out_helper (uuid, (it->first & 0xFFFF), ((it->first >> 16) & 0x7FFF));
                            SCIM_DEBUG_MAIN(1) << "Decrement usage count of HelperISE " << uuid
                                    << " to " << m_helper_uuid_count[uuid] << "\n";
                        }
                    }
                }
            }

            for (size_t i = 0; i < ctx_list.size (); ++i)
                 m_client_context_helper.erase (ctx_list [i]);

            HelperInfoRepository::iterator iter = m_helper_info_repository.begin ();
            for (; iter != m_helper_info_repository.end (); iter++)
            {
                if (!m_current_helper_uuid.compare (iter->second.uuid))
                    if (!(iter->second.option & ISM_ISE_HIDE_IN_CONTROL_PANEL))
                        socket_update_control_panel ();
            }
        } else if (client_info.type == HELPER_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a Helper client.\n";

            lock ();

            HelperInfoRepository::iterator hiit = m_helper_info_repository.find (client.get_id ());

            if (hiit != m_helper_info_repository.end ()) {
                bool restart = false;
                String uuid = hiit->second.uuid;

                HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
                if ((hiit->second.option & SCIM_HELPER_AUTO_RESTART) &&
                    (it != m_helper_client_index.end () && it->second.ref > 0))
                    restart = true;

                m_helper_manager.stop_helper (hiit->second.name);

                m_helper_client_index.erase (uuid);
                m_helper_info_repository.erase (hiit);

                if (restart && !m_ise_exiting)
                    m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
            }

            m_ise_exiting = false;
            unlock ();

            socket_transaction_start ();
            m_signal_remove_helper (client.get_id ());
            socket_transaction_end ();
        } else if (client_info.type == HELPER_ACT_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a Helper passive client.\n";

            lock ();

            HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client.get_id ());
            if (hiit != m_helper_active_info_repository.end ())
                m_helper_active_info_repository.erase (hiit);

            unlock ();
        } else if (client_info.type == IMCONTROL_ACT_CLIENT) {
            SCIM_DEBUG_MAIN(4) << "It's a IMCONTROL_ACT_CLIENT client.\n";
            int client_id = client.get_id ();

            if (client_id == m_current_active_imcontrol_id
                && TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                hide_helper (m_current_helper_uuid);

            IMControlRepository::iterator iter = m_imcontrol_repository.find (client_id);
            if (iter != m_imcontrol_repository.end ())
            {
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
        }
    }

    const ClientInfo & socket_get_client_info   (int client)
    {
        static ClientInfo null_client = { 0, UNKNOWN_CLIENT };

        ClientRepository::iterator it = m_client_repository.find (client);

        if (it != m_client_repository.end ())
            return it->second;

        return null_client;
    }

private:
    void socket_turn_on                         (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_turn_on ()\n";

        m_signal_turn_on ();
    }

    void socket_turn_off                        (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_turn_off ()\n";

        m_signal_turn_off ();
    }

    void socket_update_screen                   (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_screen ()\n";

        uint32 num;
        if (m_recv_trans.get_data (num) && ((int) num) != m_current_screen) {
            SCIM_DEBUG_MAIN(4) << "New Screen number = " << num << "\n";
            m_signal_update_screen ((int) num);
            helper_all_update_screen ((int) num);
            m_current_screen = (num);
        }
    }

    void socket_update_spot_location            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_spot_location ()\n";

        uint32 x, y, top_y;
        if (m_recv_trans.get_data (x) && m_recv_trans.get_data (y) && m_recv_trans.get_data (top_y)) {
            SCIM_DEBUG_MAIN(4) << "New Spot location x=" << x << " y=" << y << "\n";
            m_signal_update_spot_location ((int)x, (int)y, (int)top_y);
            helper_all_update_spot_location ((int)x, (int)y);
        }
    }

    void socket_update_cursor_position          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_cursor_position ()\n";

        uint32 cursor_pos;
        if (m_recv_trans.get_data (cursor_pos)) {
            SCIM_DEBUG_MAIN(4) << "New cursor position pos=" << cursor_pos << "\n";
            helper_all_update_cursor_position ((int)cursor_pos);
        }
    }

    void socket_update_factory_info             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_factory_info ()\n";

        PanelFactoryInfo info;
        if (m_recv_trans.get_data (info.uuid) && m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.lang) && m_recv_trans.get_data (info.icon)) {
            SCIM_DEBUG_MAIN(4) << "New Factory info uuid=" << info.uuid << " name=" << info.name << "\n";
            info.lang = scim_get_normalized_language (info.lang);
            m_signal_update_factory_info (info);
        }
    }

    void socket_show_help                       (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_help ()\n";

        String help;
        if (m_recv_trans.get_data (help))
            m_signal_show_help (help);
    }

    void socket_show_factory_menu               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_factory_menu ()\n";

        PanelFactoryInfo info;
        std::vector <PanelFactoryInfo> vec;

        while (m_recv_trans.get_data (info.uuid) && m_recv_trans.get_data (info.name) &&
               m_recv_trans.get_data (info.lang) && m_recv_trans.get_data (info.icon)) {
            info.lang = scim_get_normalized_language (info.lang);
            vec.push_back (info);
        }

        if (vec.size ())
            m_signal_show_factory_menu (vec);
    }

    void socket_turn_on_log                      (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_turn_on_log ()\n";

        uint32 isOn;
        if (m_recv_trans.get_data (isOn)) {
            if (isOn) {
                DebugOutput::enable_debug (SCIM_DEBUG_AllMask);
                DebugOutput::set_verbose_level (7);
            } else {
                DebugOutput::disable_debug (SCIM_DEBUG_AllMask);
                DebugOutput::set_verbose_level (0);
            }

            int     focused_client;
            uint32  focused_context;

            get_focused_context (focused_client, focused_context);

            if (focused_client == -1 || focused_context == 0)
                return;

            if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
            {
                HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

                if (it != m_helper_client_index.end ())
                {
                    Socket client_socket (it->second.id);
                    uint32 ctx;

                    ctx = get_helper_ic (focused_client, focused_context);

                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (ctx);
                    m_send_trans.put_data (m_current_helper_uuid);
                    m_send_trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
                    m_send_trans.put_data (isOn);
                    m_send_trans.write_to_socket (client_socket);
                }
            }

            ClientInfo client_info = socket_get_client_info (focused_client);
            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (focused_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (focused_context);
                m_send_trans.put_command (ISM_TRANS_CMD_TURN_ON_LOG);
                m_send_trans.put_data (isOn);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_show_preedit_string             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_preedit_string ()\n";
        m_signal_show_preedit_string ();
    }

    void socket_show_aux_string                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_aux_string ()\n";
        m_signal_show_aux_string ();
    }

    void socket_show_lookup_table               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_lookup_table ()\n";
        m_signal_show_lookup_table ();
    }

    void socket_show_associate_table            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_show_associate_table ()\n";
        m_signal_show_associate_table ();
    }

    void socket_hide_preedit_string             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_preedit_string ()\n";
        m_signal_hide_preedit_string ();
    }

    void socket_hide_aux_string                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_aux_string ()\n";
        m_signal_hide_aux_string ();
    }

    void socket_hide_lookup_table               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_lookup_table ()\n";
        m_signal_hide_lookup_table ();
    }

    void socket_hide_associate_table            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_associate_table ()\n";
        m_signal_hide_associate_table ();
    }

    void socket_update_preedit_string           (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_preedit_string ()\n";

        String str;
        AttributeList attrs;
        if (m_recv_trans.get_data (str) && m_recv_trans.get_data (attrs))
            m_signal_update_preedit_string (str, attrs);
    }

    void socket_update_preedit_caret            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_preedit_caret ()\n";

        uint32 caret;
        if (m_recv_trans.get_data (caret))
            m_signal_update_preedit_caret ((int) caret);
    }

    void socket_update_aux_string               (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_aux_string ()\n";

        String str;
        AttributeList attrs;
        if (m_recv_trans.get_data (str) && m_recv_trans.get_data (attrs))
            m_signal_update_aux_string (str, attrs);
    }

    void socket_update_lookup_table             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_lookup_table ()\n";

        lock ();
        if (m_recv_trans.get_data (g_isf_candidate_table))
        {
            unlock ();
            m_signal_update_lookup_table (g_isf_candidate_table);
        }
        else
        {
            unlock ();
        }
    }

    void socket_update_associate_table          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_associate_table ()\n";

        CommonLookupTable table;
        if (m_recv_trans.get_data (table))
            m_signal_update_associate_table (table);
    }

    void socket_update_control_panel            (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_control_panel ()\n";
        /* Check default ISE for no context app */
#ifdef ONE_HELPER_ISE_PROCESS
        uint32 ctx = get_helper_ic (-1, 0);
        ClientContextUUIDRepository::iterator it = m_client_context_helper.find (ctx);
        if (it != m_client_context_helper.end ())
        {
            if (m_should_shared_ise)
            {
                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                {
                    if (m_current_helper_uuid != it->second)
                    {
                        stop_helper (it->second, -1, 0);
                        start_helper (m_current_helper_uuid, -1, 0);
                    }
                }
                else if (TOOLBAR_KEYBOARD_MODE == m_current_toolbar_mode)
                    stop_helper (it->second, -1, 0);
            }
            else
            {
                m_current_toolbar_mode = TOOLBAR_HELPER_MODE;
                m_current_helper_uuid  = it->second;
            }
        }
        else
        {
            if (m_should_shared_ise)
            {
                if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
                    start_helper (m_current_helper_uuid, -1, 0);
            }
            else
            {
                m_current_toolbar_mode = TOOLBAR_KEYBOARD_MODE;
            }
        }

#endif
        String name, uuid;
        m_signal_get_keyboard_ise (name, uuid);

        PanelFactoryInfo info;
        if (name.length () > 0)
            info = PanelFactoryInfo (uuid, name, String (""), String (""));
        else
            info = PanelFactoryInfo (String (""), String (_("English/Keyboard")), String ("C"), String (SCIM_KEYBOARD_ICON_FILE));
        m_signal_update_factory_info (info);
    }

    void socket_register_properties             (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_register_properties ()\n";

        PropertyList properties;

        if (m_recv_trans.get_data (properties))
            m_signal_register_properties (properties);
    }

    void socket_update_property                 (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_update_property ()\n";

        Property property;
        if (m_recv_trans.get_data (property))
            m_signal_update_property (property);
    }

    void socket_get_keyboard_ise_list           (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_keyboard_ise_list ()\n";

        std::vector<String> list;
        list.clear ();
        m_signal_get_keyboard_ise_list (list);

        String uuid;
        if (m_recv_trans.get_data (uuid))
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST);
                m_send_trans.put_data (list.size ());
                for (unsigned int i = 0; i < list.size (); i++)
                    m_send_trans.put_data (list[i]);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_candidate_ui                (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_candidate_ui ()\n";

        uint32 style, mode;
        if (m_recv_trans.get_data (style) && m_recv_trans.get_data (mode))
        {
            m_signal_set_candidate_ui (style, mode);
        }
    }

    void socket_get_candidate_ui                (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_candidate_ui ()\n";

        int style = 0, mode = 0;
        m_signal_get_candidate_ui (style, mode);

        String uuid;
        if (m_recv_trans.get_data (uuid))
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_UI);
                m_send_trans.put_data (style);
                m_send_trans.put_data (mode);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_candidate_position          (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_candidate_position ()\n";

        uint32 left, top;
        if (m_recv_trans.get_data (left) && m_recv_trans.get_data (top))
        {
            m_signal_set_candidate_position (left, top);
        }
    }

    void socket_hide_candidate                  (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_hide_candidate ()\n";

        m_signal_hide_preedit_string ();
        m_signal_hide_aux_string ();
        m_signal_hide_lookup_table ();
        m_signal_hide_associate_table ();
    }

    void socket_get_candidate_rect              (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_candidate_rect ()\n";

        struct rectinfo info;
        info.pos_x  = 0;
        info.pos_y  = 0;
        info.width  = 0;
        info.height = 0;
        m_signal_get_candidate_rect (info);

        String uuid;
        if (m_recv_trans.get_data (uuid))
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CANDIDATE_RECT);
                m_send_trans.put_data (info.pos_x);
                m_send_trans.put_data (info.pos_y);
                m_send_trans.put_data (info.width);
                m_send_trans.put_data (info.height);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_set_keyboard_ise                (int type)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_set_keyboard_ise ()\n";

        String ise;
        if (m_recv_trans.get_data (ise))
            m_signal_set_keyboard_ise (type, ise);
    }

    void socket_helper_launch_helper_ise_list_selection (void)
    {
        uint32 withUI;
        if (m_recv_trans.get_data (withUI))
            m_signal_launch_helper_ise_list_selection (withUI);
    }

    void socket_get_keyboard_ise                (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_keyboard_ise ()\n";

        String ise_name, ise_uuid;
        int    client  = -1;
        uint32 context = 0;
        uint32 ctx     = 0;

        get_focused_context (client, context);
        ctx = get_helper_ic (client, context);

        if (m_client_context_uuids.find (ctx) != m_client_context_uuids.end ())
            ise_uuid = m_client_context_uuids[ctx];
        m_signal_get_keyboard_ise (ise_name, ise_uuid);

        String uuid;
        if (m_recv_trans.get_data (uuid))
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                get_focused_context (client, context);
                uint32 ctx = get_helper_ic (client, context);
                Socket socket_client (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE);
                m_send_trans.put_data (ise_name);
                m_send_trans.put_data (ise_uuid);
                m_send_trans.write_to_socket (socket_client);
            }
        }
    }

    void socket_start_helper                    (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_start_helper ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && uuid.length ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

            lock ();

            uint32 ic = get_helper_ic (client, context);

            SCIM_DEBUG_MAIN(5) << "Helper UUID =" << uuid << "  IC UUID =" << ic_uuid <<"\n";

            if (it == m_helper_client_index.end ()) {
                SCIM_DEBUG_MAIN(5) << "Run this Helper.\n";
                m_start_helper_ic_index [uuid].push_back (std::make_pair (ic, ic_uuid));
                m_helper_manager.run_helper (uuid, m_config_name, m_display_name);
            } else {
                SCIM_DEBUG_MAIN(5) << "Increase the Reference count.\n";
                Socket client_socket (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ic);
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);
                m_send_trans.write_to_socket (client_socket);
                ++ it->second.ref;
            }

            unlock ();
        }
    }

    void socket_stop_helper                     (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_stop_helper ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && uuid.length ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);

            lock ();

            uint32 ic = get_helper_ic (client, context);

            SCIM_DEBUG_MAIN(5) << "Helper UUID =" << uuid << "  IC UUID =" << ic_uuid <<"\n";

            if (it != m_helper_client_index.end ()) {
                SCIM_DEBUG_MAIN(5) << "Decrase the Reference count.\n";
                -- it->second.ref;
                Socket client_socket (it->second.id);
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ic);
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT);
                if (it->second.ref <= 0)
                    m_send_trans.put_command (SCIM_TRANS_CMD_EXIT);
                m_send_trans.write_to_socket (client_socket);
            }

            unlock ();
        }
    }

    void socket_send_helper_event               (int client, uint32 context, const String &ic_uuid)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_send_helper_event ()\n";

        String uuid;
        if (m_recv_trans.get_data (uuid) && m_recv_trans.get_data (m_nest_trans) &&
            uuid.length () && m_nest_trans.valid ()) {
            HelperClientIndex::iterator it = m_helper_client_index.find (uuid);
            if (it != m_helper_client_index.end ()) {
                Socket client_socket (it->second.id);

                lock ();

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                /* FIXME: We presume that client and context are both less than 65536.
                 * Hopefully, it should be true in any UNIXs.
                 * So it's ok to combine client and context into one uint32. */
                m_send_trans.put_data (get_helper_ic (client, context));
                m_send_trans.put_data (ic_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT);
                m_send_trans.put_data (m_nest_trans);
                m_send_trans.write_to_socket (client_socket);

                unlock ();
            }
        }
    }

    void socket_helper_register_properties      (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_properties (" << client << ")\n";

        PropertyList properties;
        if (m_recv_trans.get_data (properties))
            m_signal_register_helper_properties (client, properties);
    }

    void socket_helper_update_property          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_property (" << client << ")\n";

        Property property;
        if (m_recv_trans.get_data (property))
            m_signal_update_helper_property (client, property);
    }

    void socket_helper_send_imengine_event      (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_send_imengine_event (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        HelperInfoRepository::iterator hiit = m_helper_active_info_repository.find (client);

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (m_nest_trans) &&
            m_nest_trans.valid ()                &&
            hiit != m_helper_active_info_repository.end ()) {

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

            SCIM_DEBUG_MAIN(5) << "Target UUID = " << target_uuid << "  Focused UUId = " << focused_uuid << "\nTarget Client = " << target_client << "\n";

            if (client_info.type == FRONTEND_CLIENT) {
                Socket socket_client (target_client);
                lock ();
                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (target_context);
                m_send_trans.put_command (SCIM_TRANS_CMD_PROCESS_HELPER_EVENT);
                m_send_trans.put_data (target_uuid);
                m_send_trans.put_data (hiit->second.uuid);
                m_send_trans.put_data (m_nest_trans);
                m_send_trans.write_to_socket (socket_client);
                unlock ();
            }
        }
    }

    void socket_helper_key_event_op (int client, int cmd)
    {
        uint32 target_ic;
        String target_uuid;
        KeyEvent key;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (key)          &&
            !key.empty ()) {

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

            if (target_client == -1)
            {
                /* FIXUP: monitor 'Invalid Window' error */
                std::cerr << "focused target client is NULL" << "\n";
            }
            else if (target_client  == focused_client &&
                target_context == focused_context &&
                target_uuid    == focused_uuid)
            {
                ClientInfo client_info = socket_get_client_info (target_client);
                if (client_info.type == FRONTEND_CLIENT) {
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (cmd);
                    m_send_trans.put_data (key);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_send_key_event (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_send_key_event (" << client << ")\n";
        ISF_PROF_DEBUG("first message")

        socket_helper_key_event_op (client, SCIM_TRANS_CMD_PROCESS_KEY_EVENT);
    }

    void socket_helper_forward_key_event        (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_forward_key_event (" << client << ")\n";

        socket_helper_key_event_op (client, SCIM_TRANS_CMD_FORWARD_KEY_EVENT);
    }

    void socket_helper_commit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_commit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;
        WideString wstr;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (wstr)         &&
            wstr.length ()) {

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
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_COMMIT_STRING);
                    m_send_trans.put_data (wstr);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_show_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_show_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        if (m_recv_trans.get_data (target_ic) && m_recv_trans.get_data (target_uuid)) {
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
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_hide_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_hide_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;

        if (m_recv_trans.get_data (target_ic) && m_recv_trans.get_data (target_uuid)) {
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
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }

    void socket_helper_update_preedit_string            (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_preedit_string (" << client << ")\n";

        uint32 target_ic;
        String target_uuid;
        WideString wstr;
        AttributeList attrs;

        if (m_recv_trans.get_data (target_ic)    &&
            m_recv_trans.get_data (target_uuid)  &&
            m_recv_trans.get_data (wstr) && wstr.length () &&
            m_recv_trans.get_data (attrs)) {

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
                    Socket socket_client (target_client);
                    lock ();
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                    m_send_trans.put_data (target_context);
                    m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
                    m_send_trans.put_data (wstr);
                    m_send_trans.put_data (attrs);
                    m_send_trans.write_to_socket (socket_client);
                    unlock ();
                }
            }
        }
    }


    void socket_helper_register_helper          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_helper (" << client << ")\n";

        HelperInfo info;

        bool result = false;

        lock ();

        Socket socket_client (client);
        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        if (m_recv_trans.get_data (info.uuid) &&
            m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.icon) &&
            m_recv_trans.get_data (info.description) &&
            m_recv_trans.get_data (info.option) &&
            info.uuid.length () &&
            info.name.length ()) {

            SCIM_DEBUG_MAIN(4) << "New Helper uuid=" << info.uuid << " name=" << info.name << "\n";

            HelperClientIndex::iterator it = m_helper_client_index.find (info.uuid);

            if (it == m_helper_client_index.end ()) {
                m_helper_info_repository [client] = info;
                m_helper_client_index [info.uuid] = HelperClientStub (client, 1);
                m_send_trans.put_command (SCIM_TRANS_CMD_OK);

                StartHelperICIndex::iterator icit = m_start_helper_ic_index.find (info.uuid);

                if (icit != m_start_helper_ic_index.end ()) {
                    m_send_trans.put_command (SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);
                    for (size_t i = 0; i < icit->second.size (); ++i) {
                        m_send_trans.put_data (icit->second [i].first);
                        m_send_trans.put_data (icit->second [i].second);
                    }
                    m_start_helper_ic_index.erase (icit);
                }

                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SCREEN);
                m_send_trans.put_data ((uint32)m_current_screen);

                result = true;
            } else {
                m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
            }
        }

        m_send_trans.write_to_socket (socket_client);

        unlock ();

        if (result)
            m_signal_register_helper (client, info);
    }

    void socket_helper_register_helper_passive          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_register_helper_passive (" << client << ")\n";

        HelperInfo info;

        lock ();

        if (m_recv_trans.get_data (info.uuid) &&
            m_recv_trans.get_data (info.name) &&
            m_recv_trans.get_data (info.icon) &&
            m_recv_trans.get_data (info.description) &&
            m_recv_trans.get_data (info.option) &&
            info.uuid.length () &&
            info.name.length ()) {

            SCIM_DEBUG_MAIN(4) << "New Helper Passive uuid=" << info.uuid << " name=" << info.name << "\n";

            HelperInfoRepository::iterator it = m_helper_active_info_repository.find (client);
            if (it == m_helper_active_info_repository.end ()) {
                m_helper_active_info_repository[client] =  info;
            }

            StringIntRepository::iterator iter = m_ise_pending_repository.find (info.uuid);
            if (iter != m_ise_pending_repository.end ())
            {
                Transaction trans;
                Socket client_socket (iter->second);
                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REPLY);
                trans.put_command (SCIM_TRANS_CMD_OK);
                trans.write_to_socket (client_socket);
                m_ise_pending_repository.erase (iter);
            }

            iter = m_ise_pending_repository.find (info.name);
            if (iter != m_ise_pending_repository.end ())
            {
                Transaction trans;
                Socket client_socket (iter->second);
                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REPLY);
                trans.put_command (SCIM_TRANS_CMD_OK);
                trans.write_to_socket (client_socket);
                m_ise_pending_repository.erase (iter);
            }

            if (m_current_active_imcontrol_id != -1 &&
                m_ise_settings != NULL && m_ise_changing)
            {
                show_helper (info.uuid, m_ise_settings, m_ise_settings_len);
                m_ise_changing = false;
            }
        }

        unlock ();
    }

    void socket_helper_update_state_hided          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_state_hided (" << client << ")\n";

        ClientRepository::iterator iter = m_client_repository.begin ();

        for (; iter != m_client_repository.end (); iter++)
        {
            if (IMCONTROL_CLIENT == iter->second.type
                && iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                trans.put_command (ISM_TRANS_CMD_ISE_PANEL_HIDED);

                trans.write_to_socket (client_socket);
                break;
            }
        }
    }

    void socket_helper_update_state_showed          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_state_showed (" << client << ")\n";

        ClientRepository::iterator iter = m_client_repository.begin ();

        for (; iter != m_client_repository.end (); iter++)
        {
            if (IMCONTROL_CLIENT == iter->second.type &&
                iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                trans.put_command (ISM_TRANS_CMD_ISE_PANEL_SHOWED);

                trans.write_to_socket (client_socket);
                break;
            }
        }
    }

    void socket_helper_update_input_context          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_update_input_context (" << client << ")\n";

        uint32 type;
        uint32 value;

        if (m_recv_trans.get_data (type) && m_recv_trans.get_data (value))
        {
            ClientRepository::iterator iter = m_client_repository.begin ();

            for (; iter != m_client_repository.end (); iter++)
            {
                if (IMCONTROL_CLIENT == iter->second.type &&
                    iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
                {
                    Socket client_socket (iter->first);
                    Transaction trans;

                    trans.clear ();
                    trans.put_command (SCIM_TRANS_CMD_REQUEST);
                    trans.put_command (ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT);
                    trans.put_data (type);
                    trans.put_data (value);

                    trans.write_to_socket (client_socket);
                    break;
                }
            }
        }
    }

    void socket_helper_commit_ise_result_to_imcontrol          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_commit_ise_result_to_imcontrol (" << client << ")\n";

        char * buf = NULL;
        size_t len;

        if (m_recv_trans.get_data (&buf, len))
        {
            ClientRepository::iterator iter = m_client_repository.begin ();

            for (; iter != m_client_repository.end (); iter++)
            {
                if (IMCONTROL_CLIENT == iter->second.type &&
                    iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
                {
                    Socket client_socket (iter->first);
                    Transaction trans;

                    trans.clear ();
                    trans.put_command (SCIM_TRANS_CMD_REQUEST);
                    trans.put_command (ISM_TRANS_CMD_ISE_RESULT_TO_IMCONTROL);
                    trans.put_data (buf, len);
                    trans.write_to_socket (client_socket);
                    break;
                }
            }

            if (buf)
                delete [] buf;
        }
    }

    void socket_reset_helper_input_context (const String &uuid, int client, uint32 context)
    {
        HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

        if (it != m_helper_client_index.end ())
        {
            Socket client_socket (it->second.id);
            uint32 ctx = get_helper_ic (client, context);

            m_send_trans.clear ();
            m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
            m_send_trans.put_data (ctx);
            m_send_trans.put_data (uuid);
            m_send_trans.put_command (SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT);
            m_send_trans.write_to_socket (client_socket);
        }
    }

    void socket_reset_input_context (int client, uint32 context)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_reset_input_context \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
            socket_reset_helper_input_context (m_current_helper_uuid, client, context);
    }

    void socket_helper_commit_im_embedded_editor_string          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_commit_ise_result_to_imcontrol (" << client << ")\n";

        char * buf = NULL;
        size_t len;

        if (m_recv_trans.get_data (&buf, len))
        {
            ClientRepository::iterator iter = m_client_repository.begin ();

            for (; iter != m_client_repository.end (); iter++)
            {
                if (IMCONTROL_CLIENT == iter->second.type
                    && iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
                {
                    Socket client_socket (iter->first);
                    Transaction trans;

                    trans.clear ();
                    trans.put_command (SCIM_TRANS_CMD_REQUEST);
                    trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_STRING);
                    trans.put_data (buf, len);
                    trans.write_to_socket (client_socket);
                    break;
                }
            }

            if (buf)
                delete [] buf;
        }
    }

    void socket_helper_im_embedded_editor_changed          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_im_embedded_editor_changed (" << client << ")\n";

        ClientRepository::iterator iter = m_client_repository.begin ();

        for (; iter != m_client_repository.end (); iter++)
        {
            if (IMCONTROL_CLIENT == iter->second.type
                && iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
            {
                Socket client_socket (iter->first);
                Transaction trans;

                trans.clear ();
                trans.put_command (SCIM_TRANS_CMD_REQUEST);
                trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_CHANGED);

                trans.write_to_socket (client_socket);
                break;
            }
        }
    }

     void socket_helper_im_embedded_editor_preedit_changed          (int client)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_im_embedded_editor_preedit_changed (" << client << ")\n";

        char * buf = NULL;
        size_t len;

        if (m_recv_trans.get_data (&buf, len))
        {
            ClientRepository::iterator iter = m_client_repository.begin ();

            for (; iter != m_client_repository.end (); iter++)
            {
                if (IMCONTROL_CLIENT == iter->second.type &&
                    iter->first == m_imcontrol_map[m_current_active_imcontrol_id])
                {
                    Socket client_socket (iter->first);
                    Transaction trans;

                    trans.clear ();
                    trans.put_command (SCIM_TRANS_CMD_REQUEST);
                    trans.put_command (ISM_TRANS_CMD_IM_EMBEDDED_EDITOR_PREEDIT_CHANGED);
                    trans.put_data (buf, len);
                    trans.write_to_socket (client_socket);
                    break;
                }
            }

            if (buf)
                delete [] buf;
        }
    }


    bool helper_select_candidate (uint32         item)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_select_candidate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_SELECT_CANDIDATE);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_lookup_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_lookup_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_lookup_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_lookup_table_page_size (uint32 size)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_update_lookup_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_select_associate (uint32 item)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_select_associate \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_SELECT_ASSOCIATE);
                m_send_trans.put_data ((uint32)item);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_up (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_associate_table_page_up \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_associate_table_page_down (void)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_associate_table_page_down \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    bool helper_update_associate_table_page_size (uint32         size)
    {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::helper_update_associate_table_page_size \n";

        if (TOOLBAR_HELPER_MODE == m_current_toolbar_mode)
        {
            HelperClientIndex::iterator it = m_helper_client_index.find (m_current_helper_uuid);

            if (it != m_helper_client_index.end ())
            {
                int    client;
                uint32 context;
                Socket client_socket (it->second.id);
                uint32 ctx;

                get_focused_context (client, context);
                ctx = get_helper_ic (client, context);

                m_send_trans.clear ();
                m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                m_send_trans.put_data (ctx);
                m_send_trans.put_data (m_current_helper_uuid);
                m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE);
                m_send_trans.put_data (size);
                m_send_trans.write_to_socket (client_socket);

                return true;
            }
        }

        return false;
    }

    void helper_all_update_spot_location (int x, int y)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_spot_location (" << x << "," << y << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION);
        m_send_trans.put_data ((uint32) x);
        m_send_trans.put_data ((uint32) y);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SPOT_LOCATION_INFO) {
                Socket client_socket (hiit->first);
                m_send_trans.write_to_socket (client_socket);
            }
        }

        unlock ();
    }

    void helper_all_update_cursor_position      (int cursor_pos)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_cursor_position (" << cursor_pos << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid = get_focused_context (client, context);

        lock ();

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (ISM_TRANS_CMD_UPDATE_CURSOR_POSITION);
        m_send_trans.put_data ((uint32) cursor_pos);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            Socket client_socket (hiit->first);
            m_send_trans.write_to_socket (client_socket);
        }

        unlock ();
    }

    void helper_all_update_screen               (int screen)
    {
        SCIM_DEBUG_MAIN (5) << "PanelAgent::helper_all_update_screen (" << screen << ")\n";

        HelperInfoRepository::iterator hiit = m_helper_info_repository.begin ();

        int    client;
        uint32 context;
        String uuid;

        lock ();

        uuid = get_focused_context (client, context);

        m_send_trans.clear ();
        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data (get_helper_ic (client, context));
        m_send_trans.put_data (uuid);
        m_send_trans.put_command (SCIM_TRANS_CMD_UPDATE_SCREEN);
        m_send_trans.put_data ((uint32) screen);

        for (; hiit != m_helper_info_repository.end (); ++ hiit) {
            if (hiit->second.option & SCIM_HELPER_NEED_SCREEN_INFO) {
                Socket client_socket (hiit->first);
                m_send_trans.write_to_socket (client_socket);
            }
        }

        unlock ();
    }

    const String & get_focused_context (int &client, uint32 &context, bool force_last_context = false) const
    {
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
    void socket_transaction_start (void)
    {
        m_signal_transaction_start ();
    }

    void socket_transaction_end (void)
    {
        m_signal_transaction_end ();
    }

    void lock (void)
    {
        m_signal_lock ();
    }
    void unlock (void)
    {
        m_signal_unlock ();
    }
};

PanelAgent::PanelAgent ()
    : m_impl (new PanelAgentImpl ())
{
}

PanelAgent::~PanelAgent ()
{
    delete m_impl;
}

bool
PanelAgent::initialize (const String &config, const String &display, bool resident)
{
    return m_impl->initialize (config, display, resident);
}

bool
PanelAgent::valid (void) const
{
    return m_impl->valid ();
}

bool
PanelAgent::run (void)
{
    return m_impl->run ();
}

void
PanelAgent::stop (void)
{
    if (m_impl != NULL)
        m_impl->stop ();
}

int
PanelAgent::get_helper_list (std::vector <HelperInfo> & helpers) const
{
    return m_impl->get_helper_list (helpers);
}

void PanelAgent::hide_helper (const String &uuid)
{
    m_impl->hide_helper (uuid);
}
TOOLBAR_MODE_T
PanelAgent::get_current_toolbar_mode () const
{
    return m_impl->get_current_toolbar_mode ();
}

void
PanelAgent::get_current_ise_rect (rectinfo &ise_rect)
{
    m_impl->get_current_ise_rect (ise_rect);
}

String
PanelAgent::get_current_helper_uuid () const
{
    return m_impl->get_current_helper_uuid ();
}

String
PanelAgent::get_current_helper_name () const
{
    return m_impl->get_current_helper_name ();
}

String
PanelAgent::get_current_factory_icon () const
{
    return m_impl->get_current_factory_icon ();
}

String
PanelAgent::get_current_ise_name () const
{
    return m_impl->get_current_ise_name ();
}

void
PanelAgent::set_current_toolbar_mode (TOOLBAR_MODE_T mode)
{
    m_impl->set_current_toolbar_mode (mode);
}

void
PanelAgent::set_current_ise_name (String &name)
{
    m_impl->set_current_ise_name (name);
}

void
PanelAgent::set_current_ise_style (uint32 &style)
{
    m_impl->set_current_ise_style (style);
}

void
PanelAgent::update_ise_name (String &name)
{
    m_impl->update_ise_name (name);
}

void
PanelAgent::update_ise_style (uint32 &style)
{
    m_impl->update_ise_style (style);
}

void
PanelAgent::set_current_factory_icon (String &icon)
{
    m_impl->set_current_factory_icon (icon);
}

bool
PanelAgent::move_preedit_caret             (uint32         position)
{
    return m_impl->move_preedit_caret (position);
}

bool
PanelAgent::request_help                   (void)
{
    return m_impl->request_help ();
}

bool
PanelAgent::request_factory_menu           (void)
{
    return m_impl->request_factory_menu ();
}

bool
PanelAgent::change_factory                 (const String  &uuid)
{
    return m_impl->change_factory (uuid);
}

bool
PanelAgent::candidate_more_window_show     (void)
{
    return m_impl->candidate_more_window_show ();
}

bool
PanelAgent::candidate_more_window_hide     (void)
{
    return m_impl->candidate_more_window_hide ();
}

bool
PanelAgent::select_aux                     (uint32         item)
{
    return m_impl->select_aux (item);
}

bool
PanelAgent::select_candidate               (uint32         item)
{
    return m_impl->select_candidate (item);
}

bool
PanelAgent::lookup_table_page_up           (void)
{
    return m_impl->lookup_table_page_up ();
}

bool
PanelAgent::lookup_table_page_down         (void)
{
    return m_impl->lookup_table_page_down ();
}

bool
PanelAgent::update_lookup_table_page_size  (uint32         size)
{
    return m_impl->update_lookup_table_page_size (size);
}

bool
PanelAgent::select_associate               (uint32         item)
{
    return m_impl->select_associate (item);
}

bool
PanelAgent::associate_table_page_up        (void)
{
    return m_impl->associate_table_page_up ();
}

bool
PanelAgent::associate_table_page_down      (void)
{
    return m_impl->associate_table_page_down ();
}

bool
PanelAgent::update_associate_table_page_size (uint32         size)
{
    return m_impl->update_associate_table_page_size (size);
}

bool
PanelAgent::trigger_property               (const String  &property)
{
    return m_impl->trigger_property (property);
}

bool
PanelAgent::trigger_helper_property        (int            client,
                                            const String  &property)
{
    return m_impl->trigger_helper_property (client, property);
}

bool
PanelAgent::start_helper                   (const String  &uuid)
{
    return m_impl->start_helper (uuid, -2, 0);
}

bool
PanelAgent::stop_helper                    (const String  &uuid)
{
    return m_impl->stop_helper (uuid, -2, 0);
}

void
PanelAgent::set_default_ise                (const DEFAULT_ISE_T  &ise)
{
    m_impl->set_default_ise (ise);
}

void
PanelAgent::set_should_shared_ise          (const bool should_shared_ise)
{
    m_impl->set_should_shared_ise (should_shared_ise);
}

bool
PanelAgent::reset_keyboard_ise             () const
{
    return m_impl->reset_keyboard_ise ();
}

int
PanelAgent::get_active_ise_list            (std::vector<String> &strlist)
{
    return m_impl->get_active_ise_list (strlist);
}

void
PanelAgent::update_isf_control_status      (const bool showed)
{
   m_impl->update_isf_control_status (showed);
}

int
PanelAgent::send_display_name              (String &name)
{
    return m_impl->send_display_name (name);
}

bool
PanelAgent::reload_config                  (void)
{
    return m_impl->reload_config ();
}

bool
PanelAgent::exit                           (void)
{
    return m_impl->exit ();
}

bool
PanelAgent::filter_event (int fd)
{
    return m_impl->filter_event (fd);
}

bool
PanelAgent::filter_exception_event (int fd)
{
    return m_impl->filter_exception_event (fd);
}

int
PanelAgent::get_server_id (void)
{
    return m_impl->get_server_id ();
}

void
PanelAgent::update_ise_list (std::vector<String> &strList)
{
    m_impl->update_ise_list (strList);
}

void
PanelAgent::set_ise_changing (bool changing)
{
    m_impl->set_ise_changing (changing);
}

Connection
PanelAgent::signal_connect_reload_config              (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_reload_config (slot);
}

Connection
PanelAgent::signal_connect_turn_on                    (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_turn_on (slot);
}

Connection
PanelAgent::signal_connect_turn_off                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_turn_off (slot);
}

Connection
PanelAgent::signal_connect_show_panel                 (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_panel (slot);
}

Connection
PanelAgent::signal_connect_hide_panel                 (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_panel (slot);
}

Connection
PanelAgent::signal_connect_update_screen              (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_update_screen (slot);
}

Connection
PanelAgent::signal_connect_update_spot_location       (PanelAgentSlotIntIntInt           *slot)
{
    return m_impl->signal_connect_update_spot_location (slot);
}

Connection
PanelAgent::signal_connect_update_factory_info        (PanelAgentSlotFactoryInfo         *slot)
{
    return m_impl->signal_connect_update_factory_info (slot);
}

Connection
PanelAgent::signal_connect_start_default_ise          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_start_default_ise (slot);
}

Connection
PanelAgent::signal_connect_set_candidate_ui           (PanelAgentSlotIntInt              *slot)
{
    return m_impl->signal_connect_set_candidate_ui (slot);
}

Connection
PanelAgent::signal_connect_get_candidate_ui           (PanelAgentSlotIntInt2             *slot)
{
    return m_impl->signal_connect_get_candidate_ui (slot);
}

Connection
PanelAgent::signal_connect_set_candidate_position     (PanelAgentSlotIntInt              *slot)
{
    return m_impl->signal_connect_set_candidate_position (slot);
}

Connection
PanelAgent::signal_connect_get_candidate_rect         (PanelAgentSlotRect                *slot)
{
    return m_impl->signal_connect_get_candidate_rect (slot);
}

Connection
PanelAgent::signal_connect_set_keyboard_ise           (PanelAgentSlotIntString           *slot)
{
    return m_impl->signal_connect_set_keyboard_ise (slot);
}

Connection
PanelAgent::signal_connect_get_keyboard_ise           (PanelAgentSlotString2             *slot)
{
    return m_impl->signal_connect_get_keyboard_ise (slot);
}

Connection
PanelAgent::signal_connect_show_help                  (PanelAgentSlotString              *slot)
{
    return m_impl->signal_connect_show_help (slot);
}

Connection
PanelAgent::signal_connect_show_factory_menu          (PanelAgentSlotFactoryInfoVector   *slot)
{
    return m_impl->signal_connect_show_factory_menu (slot);
}

Connection
PanelAgent::signal_connect_show_preedit_string        (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_show_aux_string            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_aux_string (slot);
}

Connection
PanelAgent::signal_connect_show_lookup_table          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_show_associate_table       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_show_associate_table (slot);
}

Connection
PanelAgent::signal_connect_hide_preedit_string        (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_hide_aux_string            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_aux_string (slot);
}

Connection
PanelAgent::signal_connect_hide_lookup_table          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_hide_associate_table       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_hide_associate_table (slot);
}

Connection
PanelAgent::signal_connect_update_preedit_string      (PanelAgentSlotAttributeString     *slot)
{
    return m_impl->signal_connect_update_preedit_string (slot);
}

Connection
PanelAgent::signal_connect_update_preedit_caret       (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_update_preedit_caret (slot);
}

Connection
PanelAgent::signal_connect_update_aux_string          (PanelAgentSlotAttributeString     *slot)
{
    return m_impl->signal_connect_update_aux_string (slot);
}

Connection
PanelAgent::signal_connect_update_lookup_table        (PanelAgentSlotLookupTable         *slot)
{
    return m_impl->signal_connect_update_lookup_table (slot);
}

Connection
PanelAgent::signal_connect_update_associate_table     (PanelAgentSlotLookupTable         *slot)
{
    return m_impl->signal_connect_update_associate_table (slot);
}

Connection
PanelAgent::signal_connect_register_properties        (PanelAgentSlotPropertyList        *slot)
{
    return m_impl->signal_connect_register_properties (slot);
}

Connection
PanelAgent::signal_connect_update_property            (PanelAgentSlotProperty            *slot)
{
    return m_impl->signal_connect_update_property (slot);
}

Connection
PanelAgent::signal_connect_register_helper_properties (PanelAgentSlotIntPropertyList     *slot)
{
    return m_impl->signal_connect_register_helper_properties (slot);
}

Connection
PanelAgent::signal_connect_update_helper_property     (PanelAgentSlotIntProperty         *slot)
{
    return m_impl->signal_connect_update_helper_property (slot);
}

Connection
PanelAgent::signal_connect_register_helper            (PanelAgentSlotIntHelperInfo       *slot)
{
    return m_impl->signal_connect_register_helper (slot);
}

Connection
PanelAgent::signal_connect_remove_helper              (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_remove_helper (slot);
}

Connection
PanelAgent::signal_connect_set_active_ise_by_uuid     (PanelAgentSlotStringBool              *slot)
{
    return m_impl->signal_connect_set_active_ise_by_uuid (slot);
}

Connection
PanelAgent::signal_connect_set_active_ise_by_name     (PanelAgentSlotString              *slot)
{
    return m_impl->signal_connect_set_active_ise_by_name (slot);
}

Connection
PanelAgent::signal_connect_focus_in                   (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_focus_in (slot);
}

Connection
PanelAgent::signal_connect_focus_out                  (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_focus_out (slot);
}

Connection
PanelAgent::signal_connect_get_ise_list               (PanelAgentSlotBoolStringVector    *slot)
{
    return m_impl->signal_connect_get_ise_list (slot);
}

Connection
PanelAgent::signal_connect_get_keyboard_ise_list      (PanelAgentSlotBoolStringVector    *slot)
{
    return m_impl->signal_connect_get_keyboard_ise_list (slot);
}

Connection
PanelAgent::signal_connect_launch_helper_ise_list_selection(PanelAgentSlotInt * slot)
{
    return m_impl->signal_connect_launch_helper_ise_list_selection (slot);
}

Connection
PanelAgent::signal_connect_get_language_list          (PanelAgentSlotStringVector        *slot)
{
    return m_impl->signal_connect_get_language_list (slot);
}

Connection
PanelAgent::signal_connect_get_all_language           (PanelAgentSlotStringVector        *slot)
{
    return m_impl->signal_connect_get_all_language (slot);
}

Connection
PanelAgent::signal_connect_get_ise_language           (PanelAgentSlotStrStringVector     *slot)
{
    return m_impl->signal_connect_get_ise_language (slot);
}

Connection
PanelAgent::signal_connect_set_isf_language           (PanelAgentSlotString              *slot)
{
    return m_impl->signal_connect_set_isf_language (slot);
}

Connection
PanelAgent::signal_connect_get_ise_info_by_uuid       (PanelAgentSlotStringISEINFO       *slot)
{
    return m_impl->signal_connect_get_ise_info_by_uuid (slot);
}

Connection
PanelAgent::signal_connect_get_ise_info_by_name       (PanelAgentSlotStringISEINFO       *slot)
{
    return m_impl->signal_connect_get_ise_info_by_name (slot);
}

Connection
PanelAgent::signal_connect_send_key_event             (PanelAgentSlotKeyEvent            *slot)
{
    return m_impl->signal_connect_send_key_event (slot);
}

Connection
PanelAgent::signal_connect_accept_connection          (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_accept_connection (slot);
}

Connection
PanelAgent::signal_connect_close_connection           (PanelAgentSlotInt                 *slot)
{
    return m_impl->signal_connect_close_connection (slot);
}

Connection
PanelAgent::signal_connect_exit                       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_exit (slot);
}

Connection
PanelAgent::signal_connect_transaction_start          (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_transaction_start (slot);
}

Connection
PanelAgent::signal_connect_transaction_end            (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_transaction_end (slot);
}

Connection
PanelAgent::signal_connect_lock                       (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_lock (slot);
}

Connection
PanelAgent::signal_connect_unlock                     (PanelAgentSlotVoid                *slot)
{
    return m_impl->signal_connect_unlock (slot);
}

} /* namespace scim */

/*
vi:ts=4:nowrap:ai:expandtab
*/

