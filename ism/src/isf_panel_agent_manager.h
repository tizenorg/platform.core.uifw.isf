/**
 * @file isf_panel_agent_manager.h
 * @brief Defines scim::PanelAgentManager and their related types.
 *
 * scim::PanelAgentManager is a class used to manage PanelAgent object.
 */

/* ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable. */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2004-2005 James Su <suzhe@tsinghua.org.cn>
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
 */

#ifndef __ISF_PANEL_AGENT_MANAGER_H
#define __ISF_PANEL_AGENT_MANAGER_H

#include <scim_panel_common.h>

#ifndef _OUT_
#define _OUT_
#endif

namespace scim
{


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
 * two signals are used to provide a thread lock to PanelAgent, so that PanelAgent
 * can run correctly within a multi-threading Panel program.
 */
class EXAPI PanelAgentManager
{
    class PanelAgentManagerImpl;
    PanelAgentManagerImpl* m_impl;

    PanelAgentManager (const PanelAgentManager&);
    const PanelAgentManager& operator = (const PanelAgentManager&);

public:
    PanelAgentManager ();
    ~PanelAgentManager ();

    /**
     * @brief Initialize this PanelAgent.
     *
     * @param config The name of the config module to be used by Helpers.
     * @param display The name of display, on which the Panel should run.
     * @param resident If this is true then this PanelAgent will keep running
     *                 even if there is no more client connected.
     *
     * @return true if the PanelAgent is initialized correctly and ready to run.
     */
    bool initialize (InfoManager* info_manager,const ConfigPointer& config ,const String& display, bool resident = false);

    /**
     * @brief Check if this PanelAgent is initialized correctly and ready to run.
     *
     * @return true if this PanelAgent is ready to run.
     */
    bool valid (void) const;

    /**
     * @brief Stop this PanelAgent.
     */
    void stop (void);

public:

    /**
     * @brief Notice helper ISE to focus out.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_out_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Notice helper ISE to focus in.
     *
     * @param uuid The helper ISE uuid.
     */
    void focus_in_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Notice helper ISE to show window.
     *
     * @param uuid The helper ISE uuid.
     */
    void show_helper (int client, uint32 context, const String& uuid, char* data, size_t& len);

    /**
     * @brief Notice helper ISE to hide window.
     *
     * @param uuid The helper ISE uuid.
     */
    void hide_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Reset keyboard ISE.
     *
     * @return true if this operation is successful, otherwise return false.
     */
    void reset_keyboard_ise (int client, uint32 context);

    /**
     * @brief Change the factory used by the focused IMEngineInstance object.
     *
     * @param uuid The uuid of the new factory.
     * @return true if the command was sent correctly.
     */
    void change_factory (int client, uint32 context, const String&  uuid);

    /**
     * @brief Notice Helper ISE that candidate more window is showed.
     * @return true if the command was sent correctly.
     */
    void candidate_more_window_show (int client, uint32 context);

    /**
     * @brief Notice Helper ISE that candidate more window is hidden.
     * @return true if the command was sent correctly.
     */
    void candidate_more_window_hide (int client, uint32 context);

    /**
     * @brief Notice Helper ISE that show candidate.
     * @return true if the command was sent correctly.
     */
    void helper_candidate_show (int client, uint32 context, const String&  uuid);

    /**
     * @brief Notice Helper ISE that hide candidate.
     * @return true if the command was sent correctly.
     */
    void helper_candidate_hide (int client, uint32 context, const String&  uuid);

    /**
     * @brief Update helper lookup table.
     * @return true if the command was sent correctly.
     */
    void update_helper_lookup_table (int client, uint32 context, const String&  uuid, const LookupTable& table);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a aux in current aux string.
     *
     * @param item The index of the selected aux.
     * @return true if the command was sent correctly.
     */
    void select_aux (int client, uint32 context, uint32 item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a candidate in current lookup table.
     *
     * @param item The index of the selected candidate.
     * @return true if the command was sent correctly.
     */
    void select_candidate (int client, uint32 context, uint32 item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to previous page.
     * @return true if the command was sent correctly.
     */
    void lookup_table_page_up (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to next page.
     * @return true if the command was sent correctly.
     */
    void lookup_table_page_down (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the LookupTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    void update_lookup_table_page_size (int client, uint32 context, uint32 size);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update candidate items layout.
     *
     * @param row_items The items of each row.
     * @return true if the command was sent correctly.
     */
    void update_candidate_item_layout (int client, uint32 context, const std::vector<uint32>& row_items);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a associate in current associate table.
     *
     * @param item The index of the selected associate.
     * @return true if the command was sent correctly.
     */
    void select_associate (int client, uint32 context, uint32 item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to previous page.
     * @return true if the command was sent correctly.
     */
    void associate_table_page_up (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to next page.
     * @return true if the command was sent correctly.
     */
    void associate_table_page_down (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the AssociateTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    void update_associate_table_page_size (int client, uint32 context, uint32 size);

    /**
     * @brief Inform helper ISE to update displayed candidate number.
     *
     * @param size The displayed candidate number.
     * @return true if the command was sent correctly.
     */
    void update_displayed_candidate_number (int client, uint32 context, uint32 size);

    /**
     * @brief Trigger a property of the focused IMEngineInstance object.
     *
     * @param property The property key to be triggered.
     * @return true if the command was sent correctly.
     */
    void trigger_property (int client, uint32 context, const String&  property);



    /**
     * @brief Let all FrontEnds and Helpers reload configuration.
     * @return true if the command was sent correctly.
     */
    void reload_config (int client);

    /**
     * @brief Let all FrontEnds, Helpers and this Panel exit.
     * @return true if the command was sent correctly.
     */
    void exit (int client, uint32 context);

    /**
     * @brief Send candidate longpress event to ISE.
     *
     * @param type The candidate object type.
     * @param index The candidate object index.
     *
     * @return none.
     */
    void send_longpress_event (int client, uint32 context, uint32 index);
    void update_panel_event (int id,  uint32 contextid, int cmd, uint32 nType, uint32 nValue);
    void update_keyboard_ise_list (int id, uint32 contextid);
    void set_helper_mode (int client, uint32 context, const String& uuid, uint32& mode);
    void set_helper_language (int client, uint32 context, const String& uuid, uint32& language);
    void set_helper_imdata (int client, uint32 context, const String& uuid, const char* imdata, size_t& len);
    void set_helper_return_key_type (int client, uint32 context, const String& uuid, uint32 type);
    void get_helper_return_key_type (int client, uint32 context, const String& uuid, _OUT_ uint32& type);
    void set_helper_return_key_disable (int client, uint32 context, const String& uuid, uint32 disabled);
    void get_helper_return_key_disable (int client, uint32 context, const String& uuid, _OUT_ uint32& disabled);
    void set_helper_layout (int client, uint32 context, const String& uuid, uint32& layout);
    void set_helper_input_mode (int client, uint32 context, const String& uuid, uint32& mode);
    void set_helper_input_hint (int client, uint32 context, const String& uuid, uint32& hint);
    void set_helper_bidi_direction (int client, uint32 context, const String& uuid, uint32& direction);
    void set_helper_caps_mode (int client, uint32 context, const String& uuid, uint32& mode);
    void show_helper_option_window (int client, uint32 context, const String& uuid);
    bool process_key_event (int client, uint32 context, const String& uuid, KeyEvent& key, uint32 serial);
    bool get_helper_geometry (int client, uint32 context, String& uuid, _OUT_ struct rectinfo& info);
    void get_helper_imdata (int client, uint32 context, String& uuid, _OUT_ char** imdata, _OUT_ size_t& len);
    void get_helper_layout (int client, uint32 context, String& uuid, uint32& layout);
    void get_ise_language_locale (int client, uint32 context, String& uuid, _OUT_ char** data, _OUT_ size_t& len);
    void check_option_window (int client, uint32 context, String& uuid, _OUT_ uint32& avail);
    void reset_ise_option (int client_id, uint32 context);
    void reset_helper_context (int client_id, uint32 context, const String& uuid);
    void socket_update_surrounding_text (int client, uint32 context, const String& uuid, String& text, uint32 cursor);
    void socket_update_selection (int client, uint32 context, String& uuid, String text);
    void socket_get_keyboard_ise_list (int client, uint32 context, const String& uuid, std::vector<String>& list);
    void socket_get_candidate_ui (int client, uint32 context, const String& uuid,  int style,  int mode);
    void socket_get_candidate_geometry (int client, uint32 context, const String& uuid, struct rectinfo& info);
    void socket_get_keyboard_ise (int client, uint32 context, const String& uuid, String& ise_name, String& ise_uuid);
    void socket_start_helper (int client, uint32 context, const String& ic_uuid);
    void helper_detach_input_context (int client, uint32 context, const String& ic_uuid);
    void helper_process_imengine_event (int client, uint32 context, const String& ic_uuid, const Transaction& nest_transaction);
    void process_helper_event (int client, uint32 context, String target_uuid, String active_uuid, Transaction& nest_trans);
    void socket_helper_key_event (int client, uint32 context, int cmd , KeyEvent& key);
    void socket_helper_get_surrounding_text (int client, uint32 context_id, uint32 maxlen_before, uint32 maxlen_after, const int fd);
    void socket_helper_delete_surrounding_text (int client, uint32 context_id, uint32 offset, uint32 len);
    void socket_helper_get_selection (int client, uint32 context_id, const int fd);
    void socket_helper_set_selection (int client, uint32 context_id, uint32 start, uint32 end);
    void update_ise_input_context (int    focused_client, uint32 focused_context, uint32 type, uint32 value);
    void send_private_command (int    focused_client, uint32 focused_context, String command);
    void helper_all_update_spot_location (int client_id, uint32 context_id, String uuid, int x, int y);
    void helper_all_update_cursor_position (int client_id, uint32 context_id, String uuid, int cursor_pos);
    void helper_all_update_screen (int client_id, uint32 context_id, String uuid, int screen);
    void commit_string (int target_client, uint32  target_context,const WideString& wstr);
    void show_preedit_string (int target_client, uint32  target_context);
    void hide_preedit_string (int target_client, uint32  target_context);
    void update_preedit_string (int target_client, uint32  target_context, WideString wstr, AttributeList& attrs, uint32 caret);
    void update_preedit_caret (int focused_client, uint32 focused_context, uint32 caret);
    void helper_attach_input_context_and_update_screen (int client, std::vector < std::pair <uint32, String> >& helper_ic_index, uint32 current_screen);
    void hide_helper_ise (int id, uint32 context);
    bool process_input_device_event(int client, uint32 context, const String& uuid, uint32 type, const char *data, size_t len, _OUT_ uint32& result);
    void process_key_event_done(int client, uint32 context, KeyEvent &key, uint32 ret, uint32 serial);
};

/**  @} */

} /* namespace scim */

#endif /* __ISF_PANEL_AGENT_MANAGER_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
