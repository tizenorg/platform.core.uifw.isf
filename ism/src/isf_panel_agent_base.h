/**
 * @file scim_panel_agent_base.h
 * @brief Defines scim::PanelAgentBase and their related types.
 *
 * scim::PanelAgentBase is a class used to write Panel daemons.
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

#ifndef __ISF_PANEL_AGENT_BASE_H
#define __ISF_PANEL_AGENT_BASE_H

#include <scim_panel_common.h>

#ifndef _OUT_
#define _OUT_
#endif

namespace scim
{

/**
 * @addtogroup Panel
 * @ingroup InputServiceFramework
 * The accessory classes to help develop Panel daemons and FrontEnds
 * which need to communicate with Panel daemons.
 * @{
 */

typedef struct _ISE_INFO {
    String uuid;
    String name;
    String lang;
    String icon;
    uint32 option;
    TOOLBAR_MODE_T type;
} ISE_INFO;

typedef struct _HELPER_ISE_INFO {
    std::vector<String> appid;
    std::vector<String> label;
    std::vector<uint32> is_enabled;
    std::vector<uint32> is_preinstalled;
    std::vector<uint32> has_option;
} HELPER_ISE_INFO;


typedef struct DefaultIse {
    TOOLBAR_MODE_T type;
    String         uuid;
    String         name;
    DefaultIse () : type (TOOLBAR_KEYBOARD_MODE), uuid (""), name ("") { }
} DEFAULT_ISE_T;


class PanelAgentBase;
class InfoManager;
/**
 * @typedef typedef Pointer <PanelAgentBase> PanelAgentPointer;
 *
 * A smart pointer for scim::PanelAgentBase and its derived classes.
 */
typedef Pointer <PanelAgentBase>  PanelAgentPointer;

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
class EXAPI PanelAgentBase : public ReferencedObject
{
    class PanelAgentBaseImpl;
    PanelAgentBaseImpl* m_impl;

    PanelAgentBase (const PanelAgentBase&);
    const PanelAgentBase& operator = (const PanelAgentBase&);
    String m_name;
public:
    PanelAgentBase (const String& name);
    virtual ~PanelAgentBase ();

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
    virtual bool initialize (InfoManager* info_manager, const String& display, bool resident = false);

    /**
     * @brief Check if this PanelAgent is initialized correctly and ready to run.
     *
     * @return true if this PanelAgent is ready to run.
     */
    virtual bool valid (void) const;

    /**
     * @brief Stop this PanelAgent.
     */
    virtual void stop (void);

public:

    /**
     * @brief Notice helper ISE to focus out.
     *
     * @param uuid The helper ISE uuid.
     */
    virtual void focus_out_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Notice helper ISE to focus in.
     *
     * @param uuid The helper ISE uuid.
     */
    virtual void focus_in_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Notice helper ISE to show window.
     *
     * @param uuid The helper ISE uuid.
     */
    virtual void show_helper (int client, uint32 context, const String& uuid, char* data, size_t& len);

    /**
     * @brief Notice helper ISE to hide window.
     *
     * @param uuid The helper ISE uuid.
     */
    virtual void hide_helper (int client, uint32 context, const String& uuid);

    /**
     * @brief Reset keyboard ISE.
     *
     * @return true if this operation is successful, otherwise return false.
     */
    virtual void reset_keyboard_ise (int client, uint32 context);

    /**
     * @brief Change the factory used by the focused IMEngineInstance object.
     *
     * @param uuid The uuid of the new factory.
     * @return true if the command was sent correctly.
     */
    virtual void change_factory (int client, uint32 context, const String&  uuid);

    /**
     * @brief Notice Helper ISE that candidate more window is showed.
     * @return true if the command was sent correctly.
     */
    virtual void candidate_more_window_show (int client, uint32 context);

    /**
     * @brief Notice Helper ISE that candidate more window is hidden.
     * @return true if the command was sent correctly.
     */
    virtual void candidate_more_window_hide (int client, uint32 context);

    /**
     * @brief Notice Helper ISE that show candidate.
     * @return true if the command was sent correctly.
     */
    virtual void helper_candidate_show (int client, uint32 context, const String&  uuid);

    /**
     * @brief Notice Helper ISE that hide candidate.
     * @return true if the command was sent correctly.
     */
    virtual void helper_candidate_hide (int client, uint32 context, const String&  uuid);

    /**
     * @brief Update helper lookup table.
     * @return true if the command was sent correctly.
     */
    virtual void update_helper_lookup_table (int client, uint32 context, const String&  uuid, const LookupTable& table);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a aux in current aux string.
     *
     * @param item The index of the selected aux.
     * @return true if the command was sent correctly.
     */
    virtual void select_aux (int client, uint32 context, uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a candidate in current lookup table.
     *
     * @param item The index of the selected candidate.
     * @return true if the command was sent correctly.
     */
    virtual void select_candidate (int client, uint32 context, uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to previous page.
     * @return true if the command was sent correctly.
     */
    virtual void lookup_table_page_up (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the LookupTable to next page.
     * @return true if the command was sent correctly.
     */
    virtual void lookup_table_page_down (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the LookupTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    virtual void update_lookup_table_page_size (int client, uint32 context, uint32         size);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update candidate items layout.
     *
     * @param row_items The items of each row.
     * @return true if the command was sent correctly.
     */
    virtual void update_candidate_item_layout (int client, uint32 context, const std::vector<uint32>& row_items);

    /**
     * @brief Let the focused IMEngineInstance object
     *        select a associate in current associate table.
     *
     * @param item The index of the selected associate.
     * @return true if the command was sent correctly.
     */
    virtual void select_associate (int client, uint32 context, uint32         item);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to previous page.
     * @return true if the command was sent correctly.
     */
    virtual void associate_table_page_up (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        flip the AssociateTable to next page.
     * @return true if the command was sent correctly.
     */
    virtual void associate_table_page_down (int client, uint32 context);

    /**
     * @brief Let the focused IMEngineInstance object
     *        update the page size of the AssociateTable.
     *
     * @param size The new page size.
     * @return true if the command was sent correctly.
     */
    virtual void update_associate_table_page_size (int client, uint32 context, uint32       size);

    /**
     * @brief Inform helper ISE to update displayed candidate number.
     *
     * @param size The displayed candidate number.
     * @return true if the command was sent correctly.
     */
    virtual void update_displayed_candidate_number (int client, uint32 context, uint32      size);

    /**
     * @brief Trigger a property of the focused IMEngineInstance object.
     *
     * @param property The property key to be triggered.
     * @return true if the command was sent correctly.
     */
    virtual void trigger_property (int client, uint32 context, const String&  property);

    /**
     * @brief Let all FrontEnds and Helpers reload configuration.
     * @return true if the command was sent correctly.
     */
    virtual void reload_config (int client);

    /**
     * @brief Let all FrontEnds, Helpers and this Panel exit.
     * @return true if the command was sent correctly.
     */
    virtual void exit (int client, uint32 context);

    /**
     * @brief Send candidate longpress event to ISE.
     *
     * @param index The candidate object index.
     *
     * @return none.
     */
    virtual void send_longpress_event (int client, uint32 context, uint32 index);

    /**
     * @brief update_panel_event.
     *
     * @param
     *
     * @return none.
     */
    virtual void update_panel_event (int client,  uint32 context, int cmd, uint32 nType, uint32 nValue);

    /**
     * @brief update_keyboard_ise_list.
     *
     * @param
     *
     * @return none.
     */
    virtual void update_keyboard_ise_list (int client, uint32 context);

    /**
     * @brief set_helper_mode.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_mode (int client, uint32 context, const String& uuid, uint32& mode);

    /**
     * @brief set_helper_language.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_language (int client, uint32 context, const String& uuid, uint32& language);

    /**
     * @brief set_helper_imdata.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_imdata (int client, uint32 context, const String& uuid, const char* imdata, size_t& len);

    /**
     * @brief set_helper_return_key_type.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_return_key_type (int client, uint32 context, const String& uuid, uint32 type);

    /**
     * @brief get_helper_return_key_type.
     *
     * @param
     *
     * @return none.
     */
    virtual void get_helper_return_key_type (int client, uint32 context, const String& uuid, _OUT_ uint32& type);

    /**
     * @brief set_helper_return_key_disable.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_return_key_disable (int client, uint32 context, const String& uuid, uint32 disabled);

    /**
     * @brief get_helper_return_key_disable.
     *
     * @param
     *
     * @return none.
     */
    virtual void get_helper_return_key_disable (int client, uint32 context, const String& uuid, _OUT_ uint32& disabled);

    /**
     * @brief set_helper_layout.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_layout (int client, uint32 context, const String& uuid, uint32& layout);

    /**
     * @brief set_helper_input_mode.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_input_mode (int client, uint32 context, const String& uuid, uint32& mode);

    /**
     * @brief set_helper_input_hint.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_input_hint (int client, uint32 context, const String& uuid, uint32& hint);

    /**
     * @brief set_helper_bidi_direction.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_bidi_direction (int client, uint32 context, const String& uuid, uint32& direction);

    /**
     * @brief set_helper_caps_mode.
     *
     * @param
     *
     * @return none.
     */
    virtual void set_helper_caps_mode (int client, uint32 context, const String& uuid, uint32& mode);

    /**
     * @brief show_helper_option_window.
     *
     * @param
     *
     * @return none.
     */
    virtual void show_helper_option_window (int client, uint32 context, const String& uuid);

    /**
     * @brief process_key_event.
     *
     * @param
     *
     * @return none.
     */
    virtual bool process_key_event (int client, uint32 context, const String& uuid, KeyEvent& key, uint32 serial);

    /**
     * @brief get_helper_geometry.
     *
     * @param
     *
     * @return none.
     */
    virtual bool get_helper_geometry (int client, uint32 context, String& uuid, _OUT_ struct rectinfo& info);

    /**
     * @brief get_helper_imdata.
     *
     * @param
     *
     * @return none.
     */
    virtual void get_helper_imdata (int client, uint32 context, String& uuid, _OUT_ char** imdata, _OUT_ size_t& len);

    /**
     * @brief get_helper_layout.
     *
     * @param
     *
     * @return none.
     */
    virtual void get_helper_layout (int client, uint32 context, String& uuid, uint32& layout);

    /**
     * @brief get_ise_language_locale.
     *
     * @param
     *
     * @return none.
     */
    virtual void get_ise_language_locale (int client, uint32 context, String& uuid, _OUT_ char** data, _OUT_ size_t& len);

    /**
     * @brief check_option_window.
     *
     * @param
     *
     * @return none.
     */
    virtual void check_option_window (int client, uint32 context, String& uuid, _OUT_ uint32& avail);

    /**
     * @brief reset_ise_option.
     *
     * @param
     *
     * @return none.
     */
    virtual void reset_ise_option (int client, uint32 context);

    /**
     * @brief reset_helper_context.
     *
     * @param
     *
     * @return none.
     */
    virtual void reset_helper_context (int client, uint32 context, const String& uuid);

    /**
     * @brief socket_update_surrounding_text.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_update_surrounding_text (int client, uint32 context,const String& uuid, String& text, uint32 cursor);

    /**
     * @brief socket_remoteinput_focus_in.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_remoteinput_focus_in (int client);

    /**
     * @brief socket_remoteinput_focus_out.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_remoteinput_focus_out (int client);

    /**
     * @brief socket_remoteinput_entry_metadata.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_remoteinput_entry_metadata (int client, uint32 hint, uint32 layout, int variation, uint32 autocapital_type);

    /**
     * @brief socket_remoteinput_default_text.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_remoteinput_default_text (int client, String& text, uint32 cursor);

    /**
     * @brief socket_update_selection.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_update_selection (int client, uint32 context, String& uuid, String text);

    /**
     * @brief socket_get_keyboard_ise_list.
     *
     * @param
     *
     * @return none.
     */
    virtual  void socket_get_keyboard_ise_list (int client, uint32 context, const String& uuid, std::vector<String>& list);

    /**
     * @brief socket_get_candidate_ui.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_get_candidate_ui (int client, uint32 context, const String& uuid,  int style,  int mode);

    /**
     * @brief socket_get_candidate_geometry.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_get_candidate_geometry (int client, uint32 context, const String& uuid, struct rectinfo& info);

    /**
     * @brief socket_get_keyboard_ise.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_get_keyboard_ise (int client, uint32 context, const String& uuid, String& ise_name, String& ise_uuid);

    /**
     * @brief socket_start_helper.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_start_helper (int client, uint32 context, const String& ic_uuid);

    /**
     * @brief helper_detach_input_context.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_detach_input_context (int client, uint32 context, const String& ic_uuid);

    /**
     * @brief helper_process_imengine_event.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_process_imengine_event (int client, uint32 context, const String& ic_uuid, const Transaction& nest_transaction);

    /**
     * @brief process_helper_event.
     *
     * @param
     *
     * @return none.
     */
    virtual void process_helper_event (int client, uint32 context,  String target_uuid, String active_uuid, Transaction& nest_trans);

    /**
     * @brief socket_helper_key_event.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_helper_key_event (int client, uint32 context, int cmd , KeyEvent& key);

    /**
     * @brief socket_helper_get_surrounding_text.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_helper_get_surrounding_text (int client, uint32 context, uint32 maxlen_before, uint32 maxlen_after);

    /**
     * @brief socket_helper_delete_surrounding_text.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_helper_delete_surrounding_text (int client, uint32 context, uint32 offset, uint32 len);

    /**
     * @brief socket_helper_get_selection.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_helper_get_selection (int client, uint32 context);

    /**
     * @brief socket_helper_set_selection.
     *
     * @param
     *
     * @return none.
     */
    virtual void socket_helper_set_selection (int client, uint32 context, uint32 start, uint32 end);

    /**
     * @brief update_ise_input_context.
     *
     * @param
     *
     * @return none.
     */
    virtual void update_ise_input_context (int client, uint32 context, uint32 type, uint32 value);

    /**
     * @brief send_private_command.
     *
     * @param
     *
     * @return none.
     */
    virtual void send_private_command (int client, uint32 context, const String& command);

    /**
     * @brief helper_all_update_spot_location.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_all_update_spot_location (int client, uint32 context, String uuid, int x, int y);

    /**
     * @brief helper_all_update_cursor_position.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_all_update_cursor_position (int client, uint32 context, String uuid, int cursor_pos);

    /**
     * @brief helper_all_update_screen.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_all_update_screen (int client, uint32 context, String uuid, int screen);

    /**
     * @brief commit_string.
     *
     * @param
     *
     * @return none.
     */
    virtual void commit_string (int client, uint32 context,const WideString& wstr);

    /**
     * @brief show_preedit_string.
     *
     * @param
     *
     * @return none.
     */
    virtual void show_preedit_string (int client, uint32 context);

    /**
     * @brief hide_preedit_string.
     *
     * @param
     *
     * @return none.
     */
    virtual void hide_preedit_string (int client, uint32 context);

    /**
     * @brief update_preedit_string.
     *
     * @param
     *
     * @return none.
     */
    virtual void update_preedit_string (int client, uint32  context, WideString wstr, AttributeList& attrs, uint32 caret);

    /**
     * @brief update_preedit_caret.
     *
     * @param
     *
     * @return none.
     */
    virtual void update_preedit_caret (int client, uint32 context, uint32 caret);

    /**
     * @brief attach_input_context and update_screen.
     *
     * @param
     *
     * @return none.
     */
    virtual void helper_attach_input_context_and_update_screen (int client, std::vector < std::pair <uint32, String> >& helper_ic_index, uint32 current_screen);

    /**
     * @brief hide_helper_ise.
     *
     * @param
     *
     * @return none.
     */
    virtual void hide_helper_ise (int client, uint32 context);

    /**
    * @brief process_input_device_event.
    *
    * @param
    *
    * @return none.
    */
    virtual bool process_input_device_event(int client, uint32 context, const String& uuid, uint32 type, const char *data, size_t len, _OUT_ uint32& result);

    /**
    * @brief process_key_event_done.
    *
    * @param
    *
    * @return none.
    */
    virtual void process_key_event_done(int client, uint32 context, KeyEvent &key, uint32 ret, uint32 serial);

    virtual void send_key_event (int client, uint32 context,const KeyEvent &key);

    virtual void forward_key_event (int client, uint32 context,const KeyEvent &key);
};

/**  @} */

} /* namespace scim */

#endif /* __ISF_PANEL_AGENT_BASE_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
