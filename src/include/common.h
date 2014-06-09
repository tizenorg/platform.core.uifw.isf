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

#ifndef __COMMON_H__
#define __COMMON_H__

#define Uses_SCIM_UTILITY
#define Uses_SCIM_OBJECT
#define Uses_SCIM_POINTER
#define Uses_SCIM_EVENT
#define Uses_SCIM_HELPER
#define Uses_SCIM_CONFIG_BASE

#include <scim.h>
#include <Evas.h>
#include <scl.h>
#include <ise_context.h>
#include "candidate.h"

//SCL_BEGIN_DECLS

struct IISECommonEventCallback {
    virtual void init () {}
    virtual void exit (sclint ic, const sclchar *ic_uuid) {}
    virtual void attach_input_context(sclint ic, const sclchar *ic_uuid) {}
    virtual void detach_input_context(sclint ic, const sclchar *ic_uuid) {}
    virtual void reload_config (sclint ic, const sclchar *ic_uuid) {}
    virtual void update_spot_location (sclint ic, const sclchar *ic_uuid, sclint x, sclint y) {}
    virtual void update_cursor_position (sclint ic, const sclchar *ic_uuid, sclint cursor_pos) {}
    virtual void update_surrounding_text (sclint ic, const sclchar *text, sclint cursor) {}
    virtual void focus_out (sclint ic, const sclchar *ic_uuid) {}
    virtual void focus_in (sclint ic, const sclchar *ic_uuid) {}
    virtual void ise_show (sclint ic, const int degree, Ise_Context context) {}
    virtual void ise_hide (sclint ic, const sclchar *ic_uuid) {}
    virtual void get_geometry (sclu32 *pos_x, sclu32 *pos_y, sclu32 *width, sclu32 *height) {}
    virtual void set_mode (sclu32 mode) {}
    virtual void set_language (sclu32 language) {}
    virtual void set_imdata (sclchar *buf, sclu32 len) {}
    virtual void get_imdata (sclchar **buf, sclu32 *len) {}
    virtual void get_language_locale (sclint ic, sclchar **locale) {}
    virtual void set_return_key_type (sclu32 type) {}
    virtual void get_return_key_type (sclu32 *type) {}
    virtual void set_return_key_disable (sclu32 disabled) {}
    virtual void get_return_key_disable (sclu32 *disabled) {}
    virtual void set_layout (sclu32 layout) {}
    virtual void get_layout (sclu32 *layout) {}
    virtual void set_caps_mode (sclu32 mode) {}
    virtual void reset_input_context (sclint ic, const sclchar *uuid) {}
    virtual void update_candidate_geometry (sclint ic, const sclchar *uuid, sclu32 pos_x, sclu32 pos_y, sclu32 width, sclu32 height) {}
    virtual void update_keyboard_ise (sclint ic, const sclchar *uuid, const sclchar *ise_name, const sclchar *ise_uuid) {}
    virtual void candidate_more_window_show (sclint ic, const sclchar *uuid) {}
    virtual void candidate_more_window_hide (sclint ic, const sclchar *uuid) {}
    virtual void select_aux (sclint ic, const sclchar *uuid, sclint index) {}
    virtual void select_candidate (sclint ic, const sclchar *uuid, sclint index) {}
    virtual void candidate_table_page_up (sclint ic, const sclchar *uuid) {}
    virtual void candidate_table_page_down (sclint ic, const sclchar *uuid) {}
    virtual void update_lookup_table (scim::LookupTable& table) {}
    virtual void update_candidate_table_page_size (sclint ic, const sclchar *uuid, sclint page_size) {}
    virtual void select_associate (sclint ic, const sclchar *uuid, sclint index) {}
    virtual void associate_table_page_up (sclint ic, const sclchar *uuid) {}
    virtual void associate_table_page_down (sclint ic, const sclchar *uuid) {}
    virtual void update_associate_table_page_size (sclint ic, const sclchar *uuid, sclint page_size) {}
    virtual void turn_on_log (sclu32 on) {}

    virtual void set_display_language(const sclchar *language) {}
    virtual void set_theme_name(const sclchar *name) {}
    virtual void set_rotation_degree(sclint degree) {}
    virtual void set_accessibility_state(sclboolean state) {}
    virtual sclboolean process_key_event(const char *key) {return FALSE;}
};

/**
 * @brief The base class to work as a soft-based keyboard
 *
 * This class defines all functions for working as a soft-based keyboard
 * In side of ISE developer, they can use it by their requirements.
 */
class CISECommon
{
private:
    static CISECommon* m_instance; /* For singleton */
    CISECommon();

public:
    ~CISECommon();

    static CISECommon* get_instance();

    /**
     * @brief This API returns the pointer to current helper agent object
     */
    scim::HelperAgent* get_helper_agent();

    /**
     * @brief This API creates an SCLCore object that communicates with SCIM server
     * @param[in] name the name of this input method
     * @param[in] uuid the uuid to identify this input method
     * @param[in] language the locale string that this input methods supports
     */
    void init(const sclchar *name, const sclchar *uuid, const sclchar *language);

    /**
     * @brief This API deletes SCLCore object and releases all resources
     */
    void exit();

    /**
     * @brief This API runs the main loop
     * @param[in] uuid The uuid string acquired from SCIM's run_helper interface
     * @param[in] display The display string acquired from SCIM's run_helper interface
     */
    void run(const sclchar *uuid, const scim::ConfigPointer &config, const sclchar *display);
    //void run(const sclchar *uuid, const sclchar *display);

    /**
     * @brief This API returns the number of helpers in current ISE
     */
    scluint get_number_of_helpers(void);

    /**
     * @brief This API returns the helper information
     * @param[in] idx The index number of helper acquired from SCIM's get_helper_info interface
     * @param[out] info The HelperInfo structure acquired from SCIM's get_helper_info interface
     */
    sclboolean get_helper_info(scluint idx, scim::HelperInfo &info);

    /**
     * @brief This API returns the language locale string supported by current ISE
     * @param[in] idx The index number of helper acquired from SCIM's get_helper_language interface
     */
    scim::String get_helper_language(scluint idx);

    /**
     * @brief This API registers a callback interface, to handle the core events generated by SCIM
     * @param[in] callback a pointer to IISECommonEventCallback interface implementation class
     */
    void set_core_event_callback(IISECommonEventCallback *callback);

    /**
     * @brief This API returns the current callback interface.
     * @param[in] callback a pointer to IISECommonEventCallback interface implementation class
     */
    IISECommonEventCallback* get_core_event_callback();

    /**
     * @brief This API returns the pointer to main keyboard window
     */
    Evas_Object* get_main_window();

    /**
     * @brief This API updates the keyboard window's geometry information
     */
    void set_keyboard_size_hints(SclSize portrait, SclSize landscape);

    /**
     * @brief This API returns the uuid of keyboard ise that was previously set.
     */
    scim::String get_keyboard_ise_uuid();

    /**
     * @brief Request SCIM to reload all configuration.
     */
    void reload_config();

    /**
     * @brief Send an Event to IMEngine
     *
     * @param[in] ic The handle of the IMEngineInstance to receive the event.
     *            -1 means the currently focused IMEngineInstance.
     * @param[in] ic_uuid The UUID of the IMEngineInstance. NULL means don't match.
     * @param[in] command The command to be sent.
     * @param[in] value The value corresponding to the command.
     */
    void send_imengine_event(sclint ic, const sclchar *ic_uuid, const sclint command, const sclu32 value);

    /**
     * @brief Send a reset keyboard Event to IMEngine
     */
    void reset_keyboard_ise();

    /**
     * @brief Send a KeyEvent to SCIM
     *
     * @param[in] ic The handle of the IMEngineInstance to receive the event.
     *            -1 means the currently focused IMEngineInstance.
     * @param[in] ic_uuid The UUID of the IMEngineInstance. NULL means don't match.
     * @param[in] keycode The KeyEvent to be sent.
     * @param[in] keymask The modifier key mask
     */
    void send_key_event(sclint ic, const sclchar *ic_uuid, sclu32 keycode, sclu16 keymask);

    /**
     * @brief Forward a KeyEvent to client application directly.
     *
     * @param[in] ic The handle of the client Input Context to receive the event.
     *            -1 means the currently focused Input Context.
     * @param[in] ic_uuid The UUID of the IMEngineInstance. NULL means don't match.
     * @param[in] keycode The KeyEvent to be sent.
     * @param[in] keymask The modifier key mask
     */
    void forward_key_event(sclint ic, const sclchar *ic_uuid, sclu32 keycode, sclu16 keymask);

    /**
     * @brief Commit a WideString to client application directly.
     *
     * @param[in] ic The handle of the client Input Context to receive the WideString.
     *            -1 means the currently focused Input Context.
     * @param[in] ic_uuid The UUID of the IMEngine used by the Input Context.
     *            NULL means don't match.
     * @param[in] str The UTF-8 string to be committed.
     */
    //void commit_string (int ic, const sclchar *ic_uuid, const WideString &wstr) const;
    void commit_string(sclint ic, const sclchar *ic_uuid, const sclchar *str);

    void select_candidate(int index);
    /**
     * @brief Request to show preedit string.
     *
     * @param[in] ic The handle of the client Input Context to receive the request.
     *            -1 means the currently focused Input Context.
     * @param[in] ic_uuid The UUID of the IMEngine used by the Input Context.
     *            NULL means don't match.
     */
    void show_preedit_string(sclint ic, const sclchar *ic_uuid);

    /**
     * @brief Request to show aux string.
     */
    void show_aux_string(void);

    /**
     * @brief Request to show candidate string.
     */
    void show_candidate_string(void);

    /**
     * @brief Request to show associate string.
     */
    void show_associate_string(void);

    /**
     * @brief Request to hide preedit string.
     *
     * @param[in] ic The handle of the client Input Context to receive the request.
     *            -1 means the currently focused Input Context.
     * @param[in] ic_uuid The UUID of the IMEngine used by the Input Context.
     *            NULL means don't match.
     */
    void hide_preedit_string(sclint ic, const sclchar *ic_uuid);

    /**
     * @brief Request to hide aux string.
     */
    void hide_aux_string(void);

    /**
     * @brief Request to hide candidate string.
     */
    void hide_candidate_string(void);

    /**
     * @brief Request to hide associate string.
     */
    void hide_associate_string(void);

    /**
     * @brief Update a new WideString for preedit.
     *
     * @param[in] ic The handle of the client Input Context to receive the WideString.
     *            -1 means the currently focused Input Context.
     * @param[in] ic_uuid The UUID of the IMEngine used by the Input Context.
     *            NULL means don't match.
     * @param[in] str The UTF-8 string to be updated.
     * @param[in] attrs The attribute list for preedit string.
     */
    /*void update_preedit_string (int ic, const sclchar *ic_uuid, const sclchar *str, const AttributeList &attrs) const;*/
    void update_preedit_string(sclint ic, const sclchar *ic_uuid, const sclchar *str);

    /**
     * @brief Update a new string for aux.
     *
     * @param[in] str The string to be updated.
     * @param[in] attrs The attribute list for aux string.
     */
    //void update_aux_string (const sclchar *str, const AttributeList &attrs) const;
    void update_aux_string(const sclchar *str);

    /**
     * @brief Request to update candidate.
     *
     * @param[in] table The lookup table for candidate.
     */
    //void update_candidate_string (const LookupTable &table) const;

    /**
     * @brief Request to update associate.
     *
     * @param[in] table The lookup table for associate.
     */
    //void update_associate_string (const LookupTable &table) const;

    /**
     * @ brief When the input context of ISE is changed,
     *         ISE can call this function to notify application
     *
     * @param[in] type  type of event.
     * @param[in] value value of event.
     */
    void update_input_context(sclu32 type, sclu32 value);

    ///**
    // * @ brief Request to get surrounding text.
    // *
    // * @param[in] uuid The helper ISE UUID.
    // * @param[in] maxlen_before The max length of before.
    // * @param[in] maxlen_after The max length of after.
    // */
    //void get_surrounding_text     (const String                &uuid,
    //                               int                          maxlen_before,
    //                               int                          maxlen_after) const;

    ///**
    // * @ brief Request to delete surrounding text.
    // *
    // * @param[in] offset The offset for cursor position.
    // * @param[in] len The length for delete text.
    // */
    //void delete_surrounding_text  (int                          offset,
    //                               int                          len) const;

    /**
     * @ brief Set candidate position in screen.
     *
     * @param[in] left The x position in screen.
     * @param[in] top The y position in screen.
     */
    void set_candidate_position(sclint left, sclint top);

    /**
     * @ brief Request to hide candidate window.
     */
    void candidate_hide(void);

    /**
     * @ brief Request to get candidate window size and position.
     *
     * @param[in] uuid The helper ISE UUID.
     */
    //void get_candidate_window_geometry (const String           &uuid) const;

    /**
     * @ brief Set current keyboard ISE.
     *
     * @param[in] uuid The keyboard ISE UUID.
     */
    void set_keyboard_ise_by_uuid(const sclchar *uuid);

    /**
     * @ brief Request to get current keyboard ISE information.
     *
     * @param[in] uuid The helper ISE UUID.
     */
    void get_keyboard_ise(const sclchar *uuid);

    /**
     * @ brief Request to get uuid list of all keyboard ISEs.
     *
     * @param[in] uuid The helper ISE UUID.
     */
    //void get_keyboard_ise_list    (const String                &uuid) const;

#ifdef WAYLAND
     /**
     * @brief This API returns the pointer to main keyboard object
     */
    Evas_Object* get_main_obj();
#endif

private:
    virtual void register_slot_functions();

private:
    scim::HelperInfo m_helper_info;
    scim::HelperAgent m_helper_agent;

    IISECommonEventCallback *m_event_callback;

    scim::ConfigPointer m_config;
    scim::String m_uuid_keyboard_ise;

    scim::String m_supported_language;

    Evas_Object *m_main_window;
    Evas_Object *m_main_obj;
};

#endif //__COMMON_H__
