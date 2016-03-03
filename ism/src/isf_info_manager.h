/**
 * @file scim_panel_agent.h
 * @brief Defines scim::InfoManager and their related types.
 *
 * scim::InfoManager is a class used to write Panel daemons.
 * It acts like a Socket Server and handles all socket clients
 * issues.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
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
 * $Id: scim_panel_agent.h,v 1.2 2005/06/11 14:50:31 suzhe Exp $
 */

#ifndef __ISF_INFO_MANAGER_H
#define __ISF_INFO_MANAGER_H

#include <scim_panel_common.h>
#ifndef _OUT_
#define _OUT_
#endif

namespace scim
{

enum ClientType {
    UNKNOWN_CLIENT,
    FRONTEND_CLIENT,
    FRONTEND_ACT_CLIENT,
    HELPER_CLIENT,
    HELPER_ACT_CLIENT,
    IMCONTROL_ACT_CLIENT,
    IMCONTROL_CLIENT
};

struct ClientInfo {
    uint32       key;
    ClientType   type;
};

/**
 * @addtogroup Panel
 * @ingroup InputServiceFramework
 * The accessory classes to help develop Panel daemons and FrontEnds
 * which need to communicate with Panel daemons.
 * @{
 */

typedef Slot0<void>
InfoManagerSlotVoid;

typedef Slot1<void, int>
InfoManagerSlotInt;

typedef Slot1<void, int&>
InfoManagerSlotInt2;

typedef Slot1<void, const String&>
InfoManagerSlotString;

typedef Slot2<void, String&, String&>
InfoManagerSlotString2;

typedef Slot2<void, int, const String&>
InfoManagerSlotIntString;

typedef Slot1<void, const PanelFactoryInfo&>
InfoManagerSlotFactoryInfo;

typedef Slot1<void, const std::vector <PanelFactoryInfo> &>
InfoManagerSlotFactoryInfoVector;

typedef Slot1<void, const LookupTable&>
InfoManagerSlotLookupTable;

typedef Slot1<void, const Property&>
InfoManagerSlotProperty;

typedef Slot1<void, const PropertyList&>
InfoManagerSlotPropertyList;

typedef Slot2<void, int, int>
InfoManagerSlotIntInt;

typedef Slot2<void, int&, int&>
InfoManagerSlotIntInt2;

typedef Slot3<void, int, int, int>
InfoManagerSlotIntIntInt;

typedef Slot4<void, int, int, int, int>
InfoManagerSlotIntIntIntInt;

typedef Slot2<void, int, const Property&>
InfoManagerSlotIntProperty;

typedef Slot2<void, int, const PropertyList&>
InfoManagerSlotIntPropertyList;

typedef Slot2<void, int, const HelperInfo&>
InfoManagerSlotIntHelperInfo;

typedef Slot2<void, const String&, const AttributeList&>
InfoManagerSlotAttributeString;

typedef Slot3<void, const String&, const AttributeList&, int>
InfoManagerSlotAttributeStringInt;

typedef Slot1<void, std::vector<String> &>
InfoManagerSlotStringVector;

typedef Slot1<bool, HELPER_ISE_INFO&>
InfoManagerSlotBoolHelperInfo;

typedef Slot1<bool, std::vector<String> &>
InfoManagerSlotBoolStringVector;

typedef Slot2<void, char*, std::vector<String> &>
InfoManagerSlotStrStringVector;

typedef Slot2<bool, const String&, ISE_INFO&>
InfoManagerSlotStringISEINFO;

typedef Slot2<bool, String, int&>
InfoManagerSlotStringInt;

typedef Slot1<void, const KeyEvent&>
InfoManagerSlotKeyEvent;

typedef Slot1<void, struct rectinfo&>
InfoManagerSlotRect;

typedef Slot2<void, const String&, bool>
InfoManagerSlotStringBool;

typedef Slot6<bool, String, String&, String&, int&, int&, String&>
InfoManagerSlotBoolString4int2;

typedef Slot2<void, int, struct rectinfo&>
InfoManagerSlotIntRect;

typedef Slot2<bool, int, String>
InfoManagerSlotIntString2;

/**
 * @brief The class to implement all socket protocol in Panel.
 *
 * This class acts like a stand alone SocketServer.
 * It has its own dedicated main loop, and will be blocked when run () is called.
 * So run () must be called within a separated thread, in order to not block
 * the main loop of the Panel program itself.
 *
 * Before calling run (), the panel must hook the callback functions to the
 * corresponding signals.
 *
 * Note that, there are two special signals: lock(void) and unlock(void). These
 * two signals are used to provide a thread lock to InfoManager, so that InfoManager
 * can run correctly within a multi-threading Panel program.
 */
class EXAPI InfoManager
{
    class InfoManagerImpl;
    InfoManagerImpl* m_impl;

    InfoManager (const HelperAgent&);
    const InfoManager& operator = (const HelperAgent&);

public:
    InfoManager ();
    ~InfoManager ();

    /**
     * @brief Initialize this InfoManager.
     *
     * @param config The name of the config module to be used by Helpers.
     * @param display The name of display, on which the Panel should run.
     * @param resident If this is true then this InfoManager will keep running
     *                 even if there is no more client connected.
     *
     * @return true if the InfoManager is initialized correctly and ready to run.
     */
    bool initialize (InfoManager* info_manager, const ConfigPointer& config, const String& display, bool resident = false);

    /**
     * @brief Check if this InfoManager is initialized correctly and ready to run.
     *
     * @return true if this InfoManager is ready to run.
     */
    bool valid (void) const;

    /**
     * @brief Stop this InfoManager.
     */
    void stop (void);

public:

    const ClientInfo& socket_get_client_info (int client) const;

    /**
     * @brief Get the list of all helpers.
     *
     * Panel program should provide a menu which contains
     * all stand alone but not auto start Helpers, so that users can activate
     * the Helpers by clicking in the menu.
     *
     * All auto start Helpers should be started by Panel after running InfoManager
     * by calling InfoManager::start_helper().
     *
     * @param helpers A list contains information of all Helpers.
     */
    int get_helper_list (std::vector <HelperInfo>& helpers) const;

    /**
     * @brief Get the list of active ISEs.
     *
     * @param strlist A list contains information of active ISEs.
     *
     * @return the list size.
     */
    int get_active_ise_list (std::vector<String>& strlist);

    /**
     * @brief Get the helper manager connection id.
     *
     * @return the connection id
     */
    int get_helper_manager_id (void);

    /**
     * @brief Check if there are any events available to be processed.
     *
     * If it returns true then HelperManager object should call
     * HelperManager::filter_event () to process them.
     *
     * @return true if there are any events available.
     */
    bool has_helper_manager_pending_event (void);

    /**
     * @brief Filter the events received from helper manager.
     *
     * Corresponding signal will be emitted in this method.
     *
     * @return true if the command was sent correctly, otherwise return false.
     */
    bool filter_helper_manager_event (void);

    /**
     * @brief Send display name to FrontEnd.
     *
     * @param name The display name.
     *
     * @return zero if this operation is successful, otherwise return -1.
     */
    int send_display_name (String& name);


    /**
     * @brief Get current ISE type.
     *
     * @return the current ISE type.
     */
    TOOLBAR_MODE_T get_current_toolbar_mode (void) const;

    /**
     * @brief Get current helper ISE uuid.
     *
     * @return the current helper ISE uuid.
     */
    String get_current_helper_uuid (void) const;

    /**
     * @brief Get current helper ISE name.
     *
     * @return the current helper ISE name.
     */
    String get_current_helper_name (void) const;

    /**
     * @brief Get current ISE name.
     *
     * @return the current ISE name.
     */
    String get_current_ise_name (void) const;

    /**
     * @brief Set current ISE name.
     *
     * @param name The current ISE name.
     */
    void set_current_ise_name (String& name);

    /**
     * @brief Set current ISE type.
     *
     * @param mode The current ISE type.
     */
    void set_current_toolbar_mode (TOOLBAR_MODE_T mode);

    /**
     * @brief Set current helper ISE option.
     *
     * @param mode The current helper ISE option.
     */
    void set_current_helper_option (uint32 option);
    /**
     * @brief Get current ISE size and position.
     *
     * @param rect It contains ISE size and position.
     */
    void get_current_ise_geometry (rectinfo& rect);

    /**
     * @brief Send candidate panel event to IM Control.
     *
     * @param nType  The candidate panel event type.
     * @param nValue The candidate panel event value.
     */
    void update_candidate_panel_event (uint32 nType, uint32 nValue);

    /**
     * @brief Send input panel event to IM Control.
     *
     * @param nType  The input panel event type.
     * @param nValue The input panel event value.
     */
    void update_input_panel_event (uint32 nType, uint32 nValue);

    /**
     * @brief Notice helper ISE to focus out.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_out_helper (const String& uuid);

    /**
     * @brief Notice helper ISE to focus in.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_in_helper (const String& uuid);

    /**
     * @brief Notice helper ISE to show window.
     *
     * @param uuid The helper ISE uuid.
     */
    bool show_helper (const String& uuid);

    /**
     * @brief Notice helper ISE to hide window.
     *
     * @param uuid The helper ISE uuid.
     */
    void hide_helper (const String& uuid);


    /**
     * @brief Set default ISE.
     *
     * @param ise The variable contains the information of default ISE.
     */
    void set_default_ise (const DEFAULT_ISE_T& ise);

    /**
     * @brief Set whether shared ISE is for all applications.
     *
     * @param should_shared_ise The indicator for shared ISE.
     */
    void set_should_shared_ise (const bool should_shared_ise);



public:
    /**
     * @brief Let the focused IMEngineInstance object move the preedit caret.
     *
     * @param position The new preedit caret position.
     * @return true if the command was sent correctly.
     */
    bool move_preedit_caret (uint32         position);

#if 0
    /**
     * @brief Request help information from the focused IMEngineInstance object.
     * @return true if the command was sent correctly.
     */
    bool request_help (void);

    /**
     * @brief Request factory menu from the focused FrontEnd.
     * @return true if the command was sent correctly.
     */
    bool request_factory_menu (void);
#endif

    /**
     * @brief Change the factory used by the focused IMEngineInstance object.
     *
     * @param uuid The uuid of the new factory.
     * @return true if the command was sent correctly.
     */
    bool change_factory (const String&  uuid);

    /**
     * @brief Notice Helper ISE that candidate more window is showed.
     * @return true if the command was sent correctly.
     */
    bool candidate_more_window_show (void);

    /**
     * @brief Notice Helper ISE that candidate more window is hidden.
     * @return true if the command was sent correctly.
     */
    bool candidate_more_window_hide (void);

    /**
     * @brief Notice Helper ISE that show candidate.
     * @return true if the command was sent correctly.
     */
    bool helper_candidate_show (void);

    /**
     * @brief Notice Helper ISE that hide candidate.
     * @return true if the command was sent correctly.
     */
    bool helper_candidate_hide (void);

    /**
     * @brief Update helper lookup table.
     * @return true if the command was sent correctly.
     */
    bool update_helper_lookup_table (const LookupTable& table);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a aux in current aux string.
     *
     * @param item The index of the selected aux.
     * @return true if the command was sent correctly.
     */
    bool select_aux (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a candidate in current lookup table.
     *
     * @param item The index of the selected candidate.
     * @return true if the command was sent correctly.
     */
    bool select_candidate (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to previous page.
     * @return true if the command was sent correctly.
     */
    bool lookup_table_page_up (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to next page.
     * @return true if the command was sent correctly.
     */
    bool lookup_table_page_down (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the LookupTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    bool update_lookup_table_page_size (uint32         size);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update candidate items layout.
     *
     * @param row_items The items of each row.
     * @return true if the command was sent correctly.
     */
    bool update_candidate_item_layout (const std::vector<uint32>& row_items);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a associate in current associate table.
     *
     * @param item The index of the selected associate.
     * @return true if the command was sent correctly.
     */
    bool select_associate (uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to previous page.
     * @return true if the command was sent correctly.
     */
    bool associate_table_page_up (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to next page.
     * @return true if the command was sent correctly.
     */
    bool associate_table_page_down (void);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the AssociateTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    bool update_associate_table_page_size (uint32       size);

    /**
     * @brief Inform helper ISE to update displayed candidate number.
     *
     * @param size The displayed candidate number.
     * @return true if the command was sent correctly.
     */
    bool update_displayed_candidate_number (uint32      size);

    /**
     * @brief Trigger a property of the focused IMEngineInstance object.
     *
     * @param property The property key to be triggered.
     * @return true if the command was sent correctly.
     */
    bool trigger_property (const String&  property);

    /**
     * @brief Start a stand alone helper.
     *
     * @param uuid The uuid of the Helper object to be started.
     * @return true if the command was sent correctly.
     */
    bool start_helper (const String&  uuid);

    /**
     * @brief Stop a stand alone helper.
     *
     * @param uuid The uuid of the Helper object to be stopped.
     * @return true if the command was sent correctly.
     */
    bool stop_helper (const String&  uuid);

    /**
     * @brief Let all FrontEnds and Helpers reload configuration.
     * @return true if the command was sent correctly.
     */
    //void reload_config                  (void);

    /**
     * @brief Let all FrontEnds, Helpers and this Panel exit.
     * @return true if the command was sent correctly.
     */
    bool exit (void);


    /**
     * @brief Send candidate longpress event to ISE.
     *
     * @param type The candidate object type.
     * @param index The candidate object index.
     *
     * @return none.
     */
    void send_longpress_event (int type, int index);




    /**
     * @brief Request to update ISE list.
     *
     * @return none.
     */
    void update_ise_list (std::vector<String>& strList);
/////////////////////////////////Message function begin/////////////////////////////////////////

//ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE
    /**
     * @brief Reset keyboard ISE.
     *
     * @return true if this operation is successful, otherwise return false.
     */
    bool reset_keyboard_ise (void);

    //ISM_TRANS_CMD_SHOW_ISF_CONTROL
    void show_isf_panel (int client_id);

    //ISM_TRANS_CMD_HIDE_ISF_CONTROL
    void hide_isf_panel (int client_id);

    //ISM_TRANS_CMD_SHOW_ISE_PANEL
    void show_ise_panel (int client_id, uint32 client, uint32 context, char*   data, size_t  len);

    //ISM_TRANS_CMD_HIDE_ISE_PANEL
    void hide_ise_panel (int client_id, uint32 client, uint32 context);

    //SCIM_TRANS_CMD_PROCESS_KEY_EVENT
    bool process_key_event (KeyEvent& key, _OUT_ uint32& result);

    //ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY
    void get_input_panel_geometry (int client_id, _OUT_ struct rectinfo& info);

    //ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
    void get_candidate_window_geometry (int client_id, _OUT_ struct rectinfo& info);


    //ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE
    void get_ise_language_locale (int client_id, _OUT_ char* data, _OUT_ size_t& len);

    //ISM_TRANS_CMD_SET_LAYOUT
    void set_ise_layout (int client_id, uint32 layout);

    //ISM_TRANS_CMD_SET_INPUT_MODE
    void set_ise_input_mode (int client_id, uint32 input_mode);

    //ISM_TRANS_CMD_SET_INPUT_HINT
    void set_ise_input_hint (int client_id, uint32 input_hint);

    //ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION
    void update_ise_bidi_direction (int client_id, uint32 bidi_direction);


    //ISM_TRANS_CMD_SET_ISE_LANGUAGE
    void set_ise_language (int client_id, uint32 language);

    //ISM_TRANS_CMD_SET_ISE_IMDATA
    void set_ise_imdata (int client_id, const char*   imdata, size_t  len);

    //ISM_TRANS_CMD_GET_ISE_IMDATA
    bool get_ise_imdata (int client_id, _OUT_ char** imdata, _OUT_ size_t& len);

    //ISM_TRANS_CMD_GET_LAYOUT
    bool get_ise_layout (int client_id, _OUT_ uint32& layout);

    //ISM_TRANS_CMD_GET_ISE_STATE
    void get_ise_state (int client_id, _OUT_ int& state);

    //ISM_TRANS_CMD_GET_ACTIVE_ISE
    void get_active_ise (int client_id, _OUT_ String& default_uuid);

    //ISM_TRANS_CMD_GET_ISE_LIST
    void get_ise_list (int client_id, _OUT_ std::vector<String>& strlist);


    //ISM_TRANS_CMD_GET_ALL_HELPER_ISE_INFO
    void get_all_helper_ise_info (int client_id, _OUT_ HELPER_ISE_INFO& info);

    //ISM_TRANS_CMD_SET_ENABLE_HELPER_ISE_INFO
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void set_enable_helper_ise_info (int client_id, String appid, uint32 is_enabled);

    //ISM_TRANS_CMD_SHOW_HELPER_ISE_LIST
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void show_helper_ise_list (int client_id);


    //ISM_TRANS_CMD_SHOW_HELPER_ISE_SELECTOR
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void show_helper_ise_selector (int client_id);

    //ISM_TRANS_CMD_IS_HELPER_ISE_ENABLED
    //reply
    void is_helper_ise_enabled (int client_id, String strAppid, _OUT_ uint32& nEnabled);

    //ISM_TRANS_CMD_GET_ISE_INFORMATION
    void get_ise_information (int client_id, String strUuid, _OUT_ String& strName, _OUT_ String& strLanguage,
                              _OUT_ int& nType, _OUT_ int& nOption, _OUT_ String& strModuleName);

    //ISM_TRANS_CMD_RESET_ISE_OPTION
    //reply SCIM_TRANS_CMD_OK
    bool reset_ise_option (int client_id);

    //ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID
    //reply
    bool set_active_ise_by_uuid (int client_id, char*  buf, size_t  len);

    //ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID
    //reply SCIM_TRANS_CMD_FAIL or SCIM_TRANS_CMD_OK
    void set_initial_ise_by_uuid (int client_id, char*  buf, size_t  len);

    //ISM_TRANS_CMD_SET_RETURN_KEY_TYPE
    void set_ise_return_key_type (int client_id, uint32 type);


    //ISM_TRANS_CMD_GET_RETURN_KEY_TYPE
    //reply
    bool get_ise_return_key_type (int client_id, _OUT_ uint32& type);


    //ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE
    void set_ise_return_key_disable (int client_id, uint32 disabled);


    //ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE
    void get_ise_return_key_disable (int client_id, _OUT_ uint32& disabled);


    //ISM_TRANS_CMD_SET_CAPS_MODE
    void set_ise_caps_mode (int client_id, uint32 mode);


    //SCIM_TRANS_CMD_RELOAD_CONFIG
    void reload_config (void);


    //ISM_TRANS_CMD_SEND_WILL_SHOW_ACK
    void will_show_ack (int client_id);


    //ISM_TRANS_CMD_SEND_WILL_HIDE_ACK
    void will_hide_ack (int client_id);


    //ISM_TRANS_CMD_RESET_DEFAULT_ISE
    void reset_default_ise (int client_id);

    //ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE
    void set_keyboard_mode (int client_id, uint32 mode);

    //ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK
    void candidate_will_hide_ack (int client_id);

    //ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION
    void get_active_helper_option (int client_id, _OUT_ uint32& option);

    //ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW
    void show_ise_option_window (int client_id);

    //ISM_TRANS_CMD_EXPAND_CANDIDATE
    void expand_candidate ();

    //ISM_TRANS_CMD_CONTRACT_CANDIDATE
    void contract_candidate ();

    //ISM_TRANS_CMD_GET_RECENT_ISE_GEOMETRY
    void get_recent_ise_geometry (int client_id, uint32 angle, _OUT_ struct rectinfo& info);

    //ISM_TRANS_CMD_REGISTER_PANEL_CLIENT
    void register_panel_client (uint32 client_id, uint32 id);

    //SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT
    void register_input_context (uint32 client_id, uint32 context, String  uuid);

    //SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT
    void remove_input_context (uint32 client_id, uint32 context);

    //SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT
    void socket_reset_input_context (int client_id, uint32 context);

    //SCIM_TRANS_CMD_FOCUS_IN
    void focus_in (int client_id, uint32 context,  String  uuid);

    //SCIM_TRANS_CMD_FOCUS_OUT
    void focus_out (int client_id, uint32 context);

    //ISM_TRANS_CMD_TURN_ON_LOG
    void socket_turn_on_log (uint32 isOn);


    //SCIM_TRANS_CMD_PANEL_TURN_ON
    void socket_turn_on (void);


    //SCIM_TRANS_CMD_PANEL_TURN_OFF
    void socket_turn_off (void);

    //SCIM_TRANS_CMD_UPDATE_SCREEN
    void socket_update_screen (int client_id, uint32 num);


    //SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION
    void socket_update_spot_location (uint32 x, uint32 y, uint32 top_y);

    //ISM_TRANS_CMD_UPDATE_CURSOR_POSITION
    void socket_update_cursor_position (uint32 cursor_pos);

    //ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT
    void socket_update_surrounding_text (String text, uint32 cursor);

    //ISM_TRANS_CMD_UPDATE_SELECTION
    void socket_update_selection (String text);

    //SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO
    void socket_update_factory_info (PanelFactoryInfo& info);

    //SCIM_TRANS_CMD_PANEL_SHOW_HELP
    void socket_show_help (String help);

    //SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU
    void socket_show_factory_menu (std::vector <PanelFactoryInfo>& vec);

    //SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
    void socket_show_preedit_string (void);

    //SCIM_TRANS_CMD_SHOW_AUX_STRING
    void socket_show_aux_string (void);

    //SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE
    void socket_show_lookup_table (void);

    //ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE
    void socket_show_associate_table (void);

    //SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
    void socket_hide_preedit_string (void);

    //SCIM_TRANS_CMD_HIDE_AUX_STRING
    void socket_hide_aux_string (void);

    //SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE
    void socket_hide_lookup_table (void);

    //ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE
    void socket_hide_associate_table (void);

    //SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
    void socket_update_preedit_string (String& str, const AttributeList& attrs, uint32 caret);


    //SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
    void socket_update_preedit_caret (uint32 caret);

    //SCIM_TRANS_CMD_UPDATE_AUX_STRING
    void socket_update_aux_string (String& str, const AttributeList& attrs);

    //SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE
    void socket_update_lookup_table (const LookupTable& table);

    //ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE
    void socket_update_associate_table (const LookupTable& table);

    //SCIM_TRANS_CMD_REGISTER_PROPERTIES
    void socket_register_properties (const PropertyList& properties);

    //SCIM_TRANS_CMD_UPDATE_PROPERTY
    void socket_update_property (const Property& property);

    //ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST
    void socket_get_keyboard_ise_list (String& uuid);

    //ISM_TRANS_CMD_SET_CANDIDATE_UI
    void socket_set_candidate_ui (uint32 portrait_line, uint32 mode);

    //ISM_TRANS_CMD_GET_CANDIDATE_UI
    void socket_get_candidate_ui (String uuid);

    //ISM_TRANS_CMD_SET_CANDIDATE_POSITION
    void socket_set_candidate_position (uint32 left, uint32 top);

    //ISM_TRANS_CMD_HIDE_CANDIDATE
    void socket_hide_candidate (void);

    //ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY
    void socket_get_candidate_geometry (String& uuid);

    //ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID
    void socket_set_keyboard_ise (String uuid);

    //ISM_TRANS_CMD_SELECT_CANDIDATE
    void socket_helper_select_candidate (uint32 index);

    //ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY
    void socket_helper_update_ise_geometry (int client, uint32 x, uint32 y, uint32 width, uint32 height);

    //ISM_TRANS_CMD_GET_KEYBOARD_ISE
    void socket_get_keyboard_ise (String uuid);

    //SCIM_TRANS_CMD_START_HELPER
    void socket_start_helper (int client_id, uint32 context, String uuid);


    //SCIM_TRANS_CMD_STOP_HELPER
    void socket_stop_helper (int client_id, uint32 context, String uuid);

    //SCIM_TRANS_CMD_SEND_HELPER_EVENT
    void socket_send_helper_event (int client_id, uint32 context, String uuid, const Transaction& _nest_trans);


    //SCIM_TRANS_CMD_REGISTER_PROPERTIES
    void socket_helper_register_properties (int client, PropertyList& properties);

    //SCIM_TRANS_CMD_UPDATE_PROPERTY
    void socket_helper_update_property (int client, Property& property);

    //SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT
    void socket_helper_send_imengine_event (int client, uint32 target_ic, String target_uuid , Transaction& nest_trans);

    //SCIM_TRANS_CMD_PROCESS_KEY_EVENT
    //SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT
    void socket_helper_send_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key);

    //SCIM_TRANS_CMD_FORWARD_KEY_EVENT
    void socket_helper_forward_key_event (int client, uint32 target_ic, String target_uuid, KeyEvent key);

    //SCIM_TRANS_CMD_COMMIT_STRING
    void socket_helper_commit_string (int client, uint32 target_ic, String target_uuid, WideString wstr);

    //SCIM_TRANS_CMD_GET_SURROUNDING_TEXT
    void socket_helper_get_surrounding_text (int client, String uuid, uint32 maxlen_before, uint32 maxlen_after);

    //SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT
    void socket_helper_delete_surrounding_text (int client, uint32 offset, uint32 len);

    //SCIM_TRANS_CMD_GET_SELECTION
    void socket_helper_get_selection (int client, String uuid);

    //SCIM_TRANS_CMD_SET_SELECTION
    void socket_helper_set_selection (int client, uint32 start, uint32 end);


    //SCIM_TRANS_CMD_SHOW_PREEDIT_STRING
    void socket_helper_show_preedit_string (int client, uint32 target_ic, String target_uuid);

    //SCIM_TRANS_CMD_HIDE_PREEDIT_STRING
    void socket_helper_hide_preedit_string (int client, uint32 target_ic, String target_uuid);

    //SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING
    void socket_helper_update_preedit_string (int client, uint32 target_ic, String target_uuid, WideString wstr, AttributeList& attrs, uint32 caret);

    //SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET
    void socket_helper_update_preedit_caret (int client, uint32 caret);

    //SCIM_TRANS_CMD_PANEL_REGISTER_HELPER
    void socket_helper_register_helper (int client, HelperInfo& info);

    //SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER
    void socket_helper_register_helper_passive (int client, HelperInfo& info);

    //ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT
    void socket_helper_update_input_context (int client, uint32 type, uint32 value);

    //SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND
    void socket_helper_send_private_command (int client, String command);

    //ISM_TRANS_CMD_UPDATE_ISE_EXIT
    //void UPDATE_ISE_EXIT (int client);
    bool check_privilege_by_sockfd (int client_id, const String& privilege);

    void add_client (int client_id, uint32 key, ClientType type);


    void del_client (int client_id);

//////////////////////////////////Message function end/////////////////////////////////////////


public:
    /**
     * @brief Signal: Reload configuration.
     *
     * When a Helper object send a RELOAD_CONFIG event to this Panel,
     * this signal will be emitted. Panel should reload all configuration here.
     */
    Connection signal_connect_reload_config (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Turn on.
     *
     * slot prototype: void turn_on (void);
     */
    Connection signal_connect_turn_on (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Turn off.
     *
     * slot prototype: void turn_off (void);
     */
    Connection signal_connect_turn_off (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Show panel.
     *
     * slot prototype: void show_panel (void);
     */
    Connection signal_connect_show_panel (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide panel.
     *
     * slot prototype: void hide_panel (void);
     */
    Connection signal_connect_hide_panel (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Update screen.
     *
     * slot prototype: void update_screen (int screen);
     */
    Connection signal_connect_update_screen (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Update spot location.
     *
     * slot prototype: void update_spot_location (int x, int y, int top_y);
     */
    Connection signal_connect_update_spot_location (InfoManagerSlotIntIntInt*           slot);

    /**
     * @brief Signal: Update factory information.
     *
     * slot prototype: void update_factory_info (const PanelFactoryInfo &info);
     */
    Connection signal_connect_update_factory_info (InfoManagerSlotFactoryInfo*         slot);

    /**
     * @brief Signal: start default ise.
     *
     * slot prototype: void start_default_ise (void);
     */
    Connection signal_connect_start_default_ise (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: stop default ise.
     *
     * slot prototype: void stop_default_ise (void);
     */
    Connection signal_connect_stop_default_ise (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Get the list of keyboard ise name.
     *
     * slot prototype: bool get_keyboard_ise_list (std::vector<String> &);
     */
    Connection signal_connect_get_keyboard_ise_list (InfoManagerSlotBoolStringVector*    slot);

    /**
     * @brief Signal: Set candidate ui.
     *
     * slot prototype: void set_candidate_ui (int style, int mode);
     */
    Connection signal_connect_set_candidate_ui (InfoManagerSlotIntInt*              slot);

    /**
     * @brief Signal: Get candidate ui.
     *
     * slot prototype: void get_candidate_ui (int &style, int &mode);
     */
    Connection signal_connect_get_candidate_ui (InfoManagerSlotIntInt2*             slot);

    /**
     * @brief Signal: Get candidate window geometry information.
     *
     * slot prototype: void get_candidate_geometry (rectinfo &info);
     */
    Connection signal_connect_get_candidate_geometry (InfoManagerSlotRect*                slot);

    /**
     * @brief Signal: Get input panel geometry information.
     *
     * slot prototype: void get_input_panel_geometry (rectinfo &info);
     */
    Connection signal_connect_get_input_panel_geometry (InfoManagerSlotRect*                slot);

    /**
     * @brief Signal: Set candidate position.
     *
     * slot prototype: void set_candidate_position (int left, int top);
     */
    Connection signal_connect_set_candidate_position (InfoManagerSlotIntInt*              slot);

    /**
     * @brief Signal: Set keyboard ise.
     *
     * slot prototype: void set_keyboard_ise (const String &uuid);
     */
    Connection signal_connect_set_keyboard_ise (InfoManagerSlotString*              slot);

    /**
     * @brief Signal: Get keyboard ise.
     *
     * slot prototype: void get_keyboard_ise (String &ise_name);
     */
    Connection signal_connect_get_keyboard_ise (InfoManagerSlotString2*             slot);
    /**
     * @brief Signal: Update ise geometry.
     *
     * slot prototype: void update_ise_geometry (int x, int y, int width, int height);
     */
    Connection signal_connect_update_ise_geometry (InfoManagerSlotIntIntIntInt*        slot);

    /**
     * @brief Signal: Show help.
     *
     * slot prototype: void show_help (const String &help);
     */
    Connection signal_connect_show_help (InfoManagerSlotString*              slot);

    /**
     * @brief Signal: Show factory menu.
     *
     * slot prototype: void show_factory_menu (const std::vector <PanelFactoryInfo> &menu);
     */
    Connection signal_connect_show_factory_menu (InfoManagerSlotFactoryInfoVector*   slot);

    /**
     * @brief Signal: Show preedit string.
     *
     * slot prototype: void show_preedit_string (void):
     */
    Connection signal_connect_show_preedit_string (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Show aux string.
     *
     * slot prototype: void show_aux_string (void):
     */
    Connection signal_connect_show_aux_string (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Show lookup table.
     *
     * slot prototype: void show_lookup_table (void):
     */
    Connection signal_connect_show_lookup_table (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Show associate table.
     *
     * slot prototype: void show_associate_table (void):
     */
    Connection signal_connect_show_associate_table (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide preedit string.
     *
     * slot prototype: void hide_preedit_string (void);
     */
    Connection signal_connect_hide_preedit_string (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide aux string.
     *
     * slot prototype: void hide_aux_string (void);
     */
    Connection signal_connect_hide_aux_string (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide lookup table.
     *
     * slot prototype: void hide_lookup_table (void);
     */
    Connection signal_connect_hide_lookup_table (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide associate table.
     *
     * slot prototype: void hide_associate_table (void);
     */
    Connection signal_connect_hide_associate_table (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Update preedit string.
     *
     * slot prototype: void update_preedit_string (const String &str, const AttributeList &attrs, int caret);
     * - str   -- The UTF-8 encoded string to be displayed in preedit area.
     * - attrs -- The attributes of the string.
     */
    Connection signal_connect_update_preedit_string (InfoManagerSlotAttributeStringInt*  slot);

    /**
     * @brief Signal: Update preedit caret.
     *
     * slot prototype: void update_preedit_caret (int caret);
     */
    Connection signal_connect_update_preedit_caret (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Update aux string.
     *
     * slot prototype: void update_aux_string (const String &str, const AttributeList &attrs);
     * - str   -- The UTF-8 encoded string to be displayed in aux area.
     * - attrs -- The attributes of the string.
     */
    Connection signal_connect_update_aux_string (InfoManagerSlotAttributeString*     slot);

    /**
     * @brief Signal: Update lookup table.
     *
     * slot prototype: void update_lookup_table (const LookupTable &table);
     * - table -- The new LookupTable object.
     */
    Connection signal_connect_update_lookup_table (InfoManagerSlotLookupTable*         slot);

    /**
     * @brief Signal: Update associate table.
     *
     * slot prototype: void update_associate_table (const LookupTable &table);
     * - table -- The new LookupTable object.
     */
    Connection signal_connect_update_associate_table (InfoManagerSlotLookupTable*         slot);

    /**
     * @brief Signal: Register properties.
     *
     * Register properties of the focused instance.
     *
     * slot prototype: void register_properties (const PropertyList &props);
     * - props -- The properties to be registered.
     */
    Connection signal_connect_register_properties (InfoManagerSlotPropertyList*        slot);

    /**
     * @brief Signal: Update property.
     *
     * Update a property of the focused instance.
     *
     * slot prototype: void update_property (const Property &prop);
     * - prop -- The property to be updated.
     */
    Connection signal_connect_update_property (InfoManagerSlotProperty*            slot);

    /**
     * @brief Signal: Register properties of a helper.
     *
     * slot prototype: void register_helper_properties (int id, const PropertyList &props);
     * - id    -- The client id of the helper object.
     * - props -- The properties to be registered.
     */
    Connection signal_connect_register_helper_properties (InfoManagerSlotIntPropertyList*     slot);

    /**
     * @brief Signal: Update helper property.
     *
     * slot prototype: void update_helper_property (int id, const Property &prop);
     * - id   -- The client id of the helper object.
     * - prop -- The property to be updated.
     */
    Connection signal_connect_update_helper_property (InfoManagerSlotIntProperty*         slot);

    /**
     * @brief Signal: Register a helper object.
     *
     * A newly started helper object will send this event to Panel.
     *
     * slot prototype: register_helper (int id, const HelperInfo &helper);
     * - id     -- The client id of the helper object.
     * - helper -- The information of the helper object.
     */
    Connection signal_connect_register_helper (InfoManagerSlotIntHelperInfo*       slot);

    /**
     * @brief Signal: Remove a helper object.
     *
     * If a running helper close its connection to Panel, then this signal will be triggered to
     * tell Panel to remove all data associated to this helper.
     *
     * slot prototype: void remove_helper (int id);
     * - id -- The client id of the helper object to be removed.
     */
    Connection signal_connect_remove_helper (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Start an ise with the speficied uuid
     *
     * slot prototype: void set_active_ise_by_uuid (const String& uuid);
     * - uuid -- the uuid of the ise object
     */
    Connection signal_connect_set_active_ise_by_uuid (InfoManagerSlotStringBool*          slot);

    /**
     * @brief Signal: Focus in panel.
     *
     * slot prototype: void focus_in (void);
     */
    Connection signal_connect_focus_in (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Focus out panel.
     *
     * slot prototype: void focus_out (void);
     */
    Connection signal_connect_focus_out (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Expand candidate panel.
     *
     * slot prototype: void expand_candidate (void);
     */
    Connection signal_connect_expand_candidate (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Contract candidate panel.
     *
     * slot prototype: void contract_candidate (void);
     */
    Connection signal_connect_contract_candidate (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Select candidate string index.
     *
     * slot prototype: void select_candidate (int index);
     */
    Connection signal_connect_select_candidate (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Get the list of ise name.
     *
     * slot prototype: bool get_ise_list (std::vector<String> &);
     */
    Connection signal_connect_get_ise_list (InfoManagerSlotBoolStringVector*    slot);

    /**
     * @brief Signal: Get the info of all helper ise.
     *
     * slot prototype: bool get_all_helper_ise_info (HELPER_ISE_INFO &);
     */
    Connection signal_connect_get_all_helper_ise_info (InfoManagerSlotBoolHelperInfo*     slot);

    /**
     * @brief Signal: Update "has_option" column of ime_info DB by Application ID
     *
     * slot prototype: void set_enable_helper_ise_info (const String &, bool );
     */
    Connection signal_connect_set_has_option_helper_ise_info (InfoManagerSlotStringBool*     slot);

    /**
     * @brief Signal: Update "is_enable" column of ime_info DB by Application ID
     *
     * slot prototype: void set_enable_helper_ise_info (const String &, bool );
     */
    Connection signal_connect_set_enable_helper_ise_info (InfoManagerSlotStringBool*     slot);

    /**
     * @brief Signal: Launch inputmethod-setting-list application
     *
     * slot prototype: void show_helper_ise_list (void);
     */
    Connection signal_connect_show_helper_ise_list (InfoManagerSlotVoid*       slot);

    /**
     * @brief Signal: Launch inputmethod-setting-selector application
     *
     * slot prototype: void show_helper_ise_selector (void);
     */
    Connection signal_connect_show_helper_ise_selector (InfoManagerSlotVoid*   slot);

    /**
     * @brief Signal: Checks if the specific IME is enabled or disabled in the system keyboard setting
     *
     * slot prototype: bool is_helper_ise_enabled (const String, int &);
     */
    Connection signal_connect_is_helper_ise_enabled (InfoManagerSlotStringInt*   slot);

    /**
     * @brief Signal: Get the ISE information according to UUID.
     *
     * slot prototype: bool get_ise_information (String, String &, String &, int &, int &);
     */
    Connection signal_connect_get_ise_information (InfoManagerSlotBoolString4int2*     slot);

    /**
     * @brief Signal: Get the list of selected language name.
     *
     * slot prototype: void get_language_list (std::vector<String> &);
     */
    Connection signal_connect_get_language_list (InfoManagerSlotStringVector*        slot);

    /**
     * @brief Signal: Get the all languages name.
     *
     * slot prototype: void get_all_language (std::vector<String> &);
     */
    Connection signal_connect_get_all_language (InfoManagerSlotStringVector*        slot);
    /**
     * @brief Signal: Get the language list of a specified ise.
     *
     * slot prototype: void get_ise_language (char *, std::vector<String> &);
     */
    Connection signal_connect_get_ise_language (InfoManagerSlotStrStringVector*     slot);

    /**
     * @brief Signal: Get the ise information by uuid.
     *
     * slot prototype: bool get_ise_info_by_uuid (const String &, ISE_INFO &);
     */
    Connection signal_connect_get_ise_info_by_uuid (InfoManagerSlotStringISEINFO*       slot);

    /**
     * @brief Signal: send key event in panel.
     *
     * slot prototype: void send_key_event (const KeyEvent &);
     */
    Connection signal_connect_send_key_event (InfoManagerSlotKeyEvent*            slot);

    /**
     * @brief Signal: A transaction is started.
     *
     * This signal infers that the Panel should disable update before this transaction finishes.
     *
     * slot prototype: void signal_connect_transaction_start (void);
     */
    Connection signal_connect_transaction_start (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Accept connection for this InfoManager.
     *
     * slot prototype: void accept_connection (int fd);
     * - fd -- the file description for connection
     */
    Connection signal_connect_accept_connection (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Close connection for this InfoManager.
     *
     * slot prototype: void close_connection (int fd);
     * - fd -- the file description for connection
     */
    Connection signal_connect_close_connection (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Exit application for this InfoManager.
     *
     * slot prototype: void app_exit (void);
     */
    Connection signal_connect_exit (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: A transaction is finished.
     *
     * This signal will get emitted when a transaction is finished. This implys to re-enable
     * panel GUI update
     *
     * slot prototype: void signal_connect_transaction_end (void);
     */
    Connection signal_connect_transaction_end (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Lock the exclusive lock for this InfoManager.
     *
     * The panel program should provide a thread lock and hook a slot into this signal to lock it.
     * InfoManager will use this lock to ensure the data integrity.
     *
     * slot prototype: void lock (void);
     */
    Connection signal_connect_lock (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Unlock the exclusive lock for this InfoManager.
     *
     * slot prototype: void unlock (void);
     */
    Connection signal_connect_unlock (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: The input context of ISE is changed.
     *
     * slot prototype: void update_input_context (int type, int value);
     */
    Connection signal_connect_update_input_context (InfoManagerSlotIntInt*              slot);

    /**
     * @brief Signal: Show ISE.
     *
     * slot prototype: void show_ise (void);
     */
    Connection signal_connect_show_ise (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Hide ISE.
     *
     * slot prototype: void hide_ise (void);
     */
    Connection signal_connect_hide_ise (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_SHOW event
     *
     * slot prototype: void will_show_ack (void);
     */
    Connection signal_connect_will_show_ack (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_HIDE event
     *
     * slot prototype: void will_hide_ack (void);
     */
    Connection signal_connect_will_hide_ack (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Set hardware mode
     *
     * slot prototype: void set_keyboard_mode (void);
     */
    Connection signal_connect_set_keyboard_mode (InfoManagerSlotInt*                 slot);

    /**
     * @brief Signal: Notifies the client finished handling WILL_HIDE event for candidate
     *
     * slot prototype: void candidate_will_hide_ack (void);
     */
    Connection signal_connect_candidate_will_hide_ack (InfoManagerSlotVoid*                slot);

    /**
     * @brief Signal: Get ISE state.
     *
     * slot prototype: void get_ise_state (int &state);
     */
    Connection signal_connect_get_ise_state (InfoManagerSlotInt2*                slot);

    /**
     * @brief Signal: Get the recent input panel geometry information.
     *
     * slot prototype: void get_recent_ise_geometry (int angle, rectinfo &info);
     */
    Connection signal_connect_get_recent_ise_geometry (InfoManagerSlotIntRect*             slot);

    Connection signal_connect_check_privilege_by_sockfd  (InfoManagerSlotIntString2* slot);
};

/**  @} */

} /* namespace scim */

#endif /* __SCIM_PANEL_AGENT_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
