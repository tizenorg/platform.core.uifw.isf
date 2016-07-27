/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2015 Samsung Electronics Co., Ltd.
 *
 * Contact: Jihoon Kim <jihoon48.kim@samsung.com>, Haifeng Deng <haifeng.deng@samsung.com>
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

#ifndef __ISF_MESSAGE_QUEUE_H
#define __ISF_MESSAGE_QUEUE_H

#define Uses_SCIM_TRANSACTION
#define Uses_SCIM_TRANS_COMMANDS
#define Uses_SCIM_HELPER
#define Uses_SCIM_SOCKET
#define Uses_SCIM_EVENT
#define Uses_SCIM_BACKEND
#define Uses_SCIM_IMENGINE_MODULE

#include <string.h>
#include <unistd.h>

#include "scim_private.h"
#include "scim.h"
#include <scim_panel_common.h>
#include "isf_query_utility.h"
#include <dlog.h>
#include "isf_debug.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_MESSAGE_QUEUE"

namespace scim {

class MessageItem
{
public:
    MessageItem() : m_command(0) {}
    virtual ~MessageItem() {}

    int& get_command_ref() { return m_command; }
protected:
    int m_command;
};

class MessageItemHelper : public MessageItem
{
public:
    MessageItemHelper() : m_ic(0) {}
    virtual ~MessageItemHelper() {}

    uint32& get_ic_ref() { return m_ic; }
    String& get_ic_uuid_ref() { return m_ic_uuid; }
protected:
    uint32 m_ic;
    String m_ic_uuid;
};

/* SCIM_TRANS_CMD_EXIT */
class MessageItemExit : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_RELOAD_CONFIG */
class MessageItemReloadConfig : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_UPDATE_SCREEN */
class MessageItemUpdateScreen : public MessageItemHelper
{
public:
    MessageItemUpdateScreen() : m_screen(0) {}
    virtual ~MessageItemUpdateScreen() {}

    uint32& get_screen_ref() { return m_screen; }
protected:
    uint32 m_screen;
};

/* SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION */
class MessageItemUpdateSpotLocation : public MessageItemHelper
{
public:
    MessageItemUpdateSpotLocation() : m_x(0), m_y(0) {}
    virtual ~MessageItemUpdateSpotLocation() {}

    uint32& get_x_ref() { return m_x; }
    uint32& get_y_ref() { return m_y; }

protected:
    uint32 m_x;
    uint32 m_y;
};

/* ISM_TRANS_CMD_UPDATE_CURSOR_POSITION */
class MessageItemUpdateCursorPosition : public MessageItemHelper
{
public:
    MessageItemUpdateCursorPosition() : m_cursor_pos(0) {}
    virtual ~MessageItemUpdateCursorPosition() {}

    uint32& get_cursor_pos_ref() { return m_cursor_pos; }
protected:
    uint32 m_cursor_pos;
};

/* ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT */
class MessageItemUpdateSurroundingText : public MessageItemHelper
{
public:
    MessageItemUpdateSurroundingText() : m_cursor(0) {}
    virtual ~MessageItemUpdateSurroundingText() {}

    String& get_text_ref() { return m_text; }
    uint32& get_cursor_ref() { return m_cursor; }
protected:
    String m_text;
    uint32 m_cursor;
};

/* ISM_TRANS_CMD_UPDATE_SELECTION */
class MessageItemUpdateSelection : public MessageItemHelper
{
public:
    MessageItemUpdateSelection() {}
    virtual ~MessageItemUpdateSelection() {}

    String& get_text_ref() { return m_text; }
protected:
    String m_text;

};

/* SCIM_TRANS_CMD_TRIGGER_PROPERTY */
class MessageItemTriggerProperty : public MessageItemHelper
{
public:
    MessageItemTriggerProperty() {}
    virtual ~MessageItemTriggerProperty() {}

    String& get_property_ref() { return m_property; }
protected:
    String m_property;

};

/* SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT */
class MessageItemHelperProcessImengineEvent : public MessageItemHelper
{
public:
    MessageItemHelperProcessImengineEvent() {}
    virtual ~MessageItemHelperProcessImengineEvent() {}

    Transaction& get_transaction_ref() { return m_trans; }
protected:
    Transaction m_trans;
};

/* SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT */
class MessageItemHelperAttachInputContext : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT */
class MessageItemHelperDetachInputContext : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_FOCUS_OUT */
class MessageItemFocusOut : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_FOCUS_IN */
class MessageItemFocusIn : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_SHOW_ISE_PANEL */
class MessageItemShowISEPanel : public MessageItemHelper
{
public:
    MessageItemShowISEPanel() : m_data(NULL), m_len(0) {}
    virtual ~MessageItemShowISEPanel() { if (m_data) delete[] m_data; m_data = NULL; }

    char** get_data_ptr() { return &m_data; }
    size_t& get_len_ref() { return m_len; }
protected:
    char* m_data;
    size_t m_len;
};

/* ISM_TRANS_CMD_HIDE_ISE_PANEL */
class MessageItemHideISEPanel : public MessageItemHelper
{
};

// ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY
class MessageItemGetActiveISEGeometry : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_SET_ISE_MODE */
class MessageItemSetISEMode : public MessageItemHelper
{
public:
    MessageItemSetISEMode() : m_mode(0) {}
    virtual ~MessageItemSetISEMode() {}

    uint32& get_mode_ref() { return m_mode; }
protected:
    uint32 m_mode;
};

/* ISM_TRANS_CMD_SET_ISE_LANGUAGE */
class MessageItemSetISELanguage : public MessageItemHelper
{
public:
    MessageItemSetISELanguage() : m_language(0) {}
    virtual ~MessageItemSetISELanguage() {}

    uint32& get_language_ref() { return m_language; }
protected:
    uint32 m_language;
};

/* ISM_TRANS_CMD_SET_ISE_IMDATA */
class MessageItemSetISEImData : public MessageItemHelper
{
public:
    MessageItemSetISEImData() : m_imdata(NULL), m_len(0) {}
    virtual ~MessageItemSetISEImData() { if (m_imdata) delete[] m_imdata; m_imdata = NULL; }

    char** get_imdata_ptr() { return &m_imdata; }
    size_t& get_len_ref() { return m_len; }
protected:
    char* m_imdata;
    size_t m_len;
};

/* ISM_TRANS_CMD_GET_ISE_IMDATA */
class MessageItemGetISEImdata : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE */
class MessageItemGetISELanguageLocale : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_SET_RETURN_KEY_TYPE */
class MessageItemSetReturnKeyType : public MessageItemHelper
{
public:
    MessageItemSetReturnKeyType() : m_type(0) {}
    virtual ~MessageItemSetReturnKeyType() {}

    uint32& get_type_ref() { return m_type; }
protected:
    uint32 m_type;
};

/* ISM_TRANS_CMD_GET_RETURN_KEY_TYPE */
class MessageItemGetReturnKeyType : public MessageItemHelper
{
public:
    MessageItemGetReturnKeyType() : m_type(0) {}
    virtual ~MessageItemGetReturnKeyType() {}

    uint32& get_type_ref() { return m_type; }
protected:
    uint32 m_type;
};

/* ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE */
class MessageItemSetReturnKeyDisable : public MessageItemHelper
{
public:
    MessageItemSetReturnKeyDisable() : m_disabled(0) {}
    virtual ~MessageItemSetReturnKeyDisable() {}

    uint32& get_disabled_ref() { return m_disabled; }
protected:
    uint32 m_disabled;
};

/* ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE */
class MessageItemGetReturnKeyDisable : public MessageItemHelper
{
public:
    MessageItemGetReturnKeyDisable() : m_disabled(0) {}
    virtual ~MessageItemGetReturnKeyDisable() {}

    uint32& get_disabled_ref() { return m_disabled; }
protected:
    uint32 m_disabled;
};

/* SCIM_TRANS_CMD_PROCESS_KEY_EVENT */
class MessageItemProcessKeyEvent : public MessageItemHelper
{
public:
    MessageItemProcessKeyEvent() : m_serial(0) {}
    virtual ~MessageItemProcessKeyEvent() {}

    KeyEvent& get_key_ref() { return m_key; }
    uint32& get_serial_ref() { return m_serial; }
protected:
    KeyEvent m_key;
    uint32 m_serial;
};

/* ISM_TRANS_CMD_SET_LAYOUT */
class MessageItemSetLayout : public MessageItemHelper
{
public:
    MessageItemSetLayout() : m_layout(0) {}
    virtual ~MessageItemSetLayout() {}

    uint32& get_layout_ref() { return m_layout; }
protected:
    uint32 m_layout;
};

/* ISM_TRANS_CMD_GET_LAYOUT */
class MessageItemGetLayout : public MessageItemHelper
{
public:
    MessageItemGetLayout() : m_layout(0) {}
    virtual ~MessageItemGetLayout() {}

    uint32& get_layout_ref() { return m_layout; }
protected:
    uint32 m_layout;
};

/* ISM_TRANS_CMD_SET_INPUT_MODE */
class MessageItemSetInputMode : public MessageItemHelper
{
public:
    MessageItemSetInputMode() : m_input_mode(0) {}
    virtual ~MessageItemSetInputMode() {}

    uint32& get_input_mode_ref() { return m_input_mode; }
protected:
    uint32 m_input_mode;
};

/* ISM_TRANS_CMD_SET_CAPS_MODE */
class MessageItemSetCapsMode : public MessageItemHelper
{
public:
    MessageItemSetCapsMode() : m_mode(0) {}
    virtual ~MessageItemSetCapsMode() {}

    uint32& get_mode_ref() { return m_mode; }
protected:
    uint32 m_mode;
};

/* SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT */
class MessageItemPanelResetInputContext : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_UPDATE_CANDIDATE_UI */
class MessageItemUpdateCandidateUI : public MessageItemHelper
{
public:
    MessageItemUpdateCandidateUI() : m_style(0), m_mode(0) {}
    virtual ~MessageItemUpdateCandidateUI() {}

    uint32& get_style_ref() { return m_style; }
    uint32& get_mode_ref() { return m_mode; }
protected:
    uint32 m_style;
    uint32 m_mode;
};

/* ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY */
class MessageItemUpdateCandidateGeometry : public MessageItemHelper
{
public:
    MessageItemUpdateCandidateGeometry() {}
    virtual ~MessageItemUpdateCandidateGeometry() {}

    struct rectinfo& get_rectinfo_ref() { return m_info; }
protected:
    struct rectinfo m_info;
};

/* ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE */
class MessageItemUpdateKeyboardISE : public MessageItemHelper
{
public:
    MessageItemUpdateKeyboardISE() {}
    virtual ~MessageItemUpdateKeyboardISE() {}

    String& get_name_ref() { return m_name; }
    String& get_uuid_ref() { return m_uuid; }
protected:
    String m_name;
    String m_uuid;
};

/* ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST */
class MessageItemUpdateKeyboardISEList : public MessageItemHelper
{
public:
    MessageItemUpdateKeyboardISEList() {}
    virtual ~MessageItemUpdateKeyboardISEList() {}

    std::vector<String>& get_list_ref() { return m_list; }
protected:
    std::vector<String> m_list;
};

/* ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW */
class MessageItemCandidateMoreWindowShow : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE */
class MessageItemCandidateMoreWindowHide : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_SELECT_AUX */
class MessageItemSelectAux : public MessageItemHelper
{
public:
    MessageItemSelectAux() : m_item(0) {}
    virtual ~MessageItemSelectAux() {}

    uint32& get_item_ref() { return m_item; }
protected:
    uint32 m_item;
};

/* SCIM_TRANS_CMD_SELECT_CANDIDATE */
class MessageItemSelectCandidate : public MessageItemHelper
{
public:
    MessageItemSelectCandidate() : m_item(0) {}
    virtual ~MessageItemSelectCandidate() {}

    uint32& get_item_ref() { return m_item; }
protected:
    uint32 m_item;
};

/* SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP */
class MessageItemLookupTablePageUp : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN */
class MessageItemLookupTablePageDown : public MessageItemHelper
{
};

/* SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE: */
class MessageItemUpdateLookupTablePageSize : public MessageItemHelper
{
public:
    MessageItemUpdateLookupTablePageSize() : m_size(0) {}
    virtual ~MessageItemUpdateLookupTablePageSize() {}

    uint32& get_size_ref() { return m_size; }
protected:
    uint32 m_size;
};

/* ISM_TRANS_CMD_CANDIDATE_SHOW */
class MessageItemCandidateShow : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_CANDIDATE_HIDE */
class MessageItemCandidateHide : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE */
class MessageItemUpdateLookupTable : public MessageItemHelper
{
public:
    MessageItemUpdateLookupTable() {}
    virtual ~MessageItemUpdateLookupTable() {}

    CommonLookupTable& get_candidate_table_ref() { return m_helper_candidate_table; }
protected:
    CommonLookupTable m_helper_candidate_table;
};

/* ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT */
class MessageItemUpdateCandidateItemLayout : public MessageItemHelper
{
public:
    MessageItemUpdateCandidateItemLayout() {}
    virtual ~MessageItemUpdateCandidateItemLayout() {}

    std::vector<uint32>& get_row_items_ref() { return m_row_items; }
protected:
    std::vector<uint32> m_row_items;
};

/* ISM_TRANS_CMD_SELECT_ASSOCIATE: */
class MessageItemSelectAssociate : public MessageItemHelper
{
public:
    MessageItemSelectAssociate() : m_item(0) {}
    virtual ~MessageItemSelectAssociate() {}

    uint32& get_item_ref() { return m_item; }
protected:
    uint32 m_item;
};

/* ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP */
class MessageItemAssociateTablePageUp : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN */
class MessageItemAssociateTablePageDown : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE */
class MessageItemUpdateAssociateTablePageSize : public MessageItemHelper
{
public:
    MessageItemUpdateAssociateTablePageSize() : m_size(0) {}
    virtual ~MessageItemUpdateAssociateTablePageSize() {}

    uint32& get_size_ref() { return m_size; }
protected:
    uint32 m_size;
};

/* ISM_TRANS_CMD_RESET_ISE_CONTEXT */
class MessageItemResetISEContext : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_TURN_ON_LOG */
class MessageItemTurnOnLog : public MessageItemHelper
{
public:
    MessageItemTurnOnLog() : m_state(0) {}
    virtual ~MessageItemTurnOnLog() {}

    uint32& get_state_ref() { return m_state; }
protected:
    uint32 m_state;
};

/* ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE */
class MessageItemUpdateDisplayedCandidate : public MessageItemHelper
{
public:
    MessageItemUpdateDisplayedCandidate() : m_size(0) {}
    virtual ~MessageItemUpdateDisplayedCandidate() {}

    uint32& get_size_ref() { return m_size; }
protected:
    uint32 m_size;
};

/* ISM_TRANS_CMD_LONGPRESS_CANDIDATE */
class MessageItemLongpressCandidate : public MessageItemHelper
{
public:
    MessageItemLongpressCandidate() : m_index(0) {}
    virtual ~MessageItemLongpressCandidate() {}

    uint32& get_index_ref() { return m_index; }
protected:
    uint32 m_index;
};

/* ISM_TRANS_CMD_SET_INPUT_HINT */
class MessageItemSetInputHint : public MessageItemHelper
{
public:
    MessageItemSetInputHint() : m_input_hint(0) {}
    virtual ~MessageItemSetInputHint() {}

    uint32& get_input_hint_ref() { return m_input_hint; }
protected:
    uint32 m_input_hint;
};

/* ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION */
class MessageItemUpdateBidiDirection : public MessageItemHelper
{
public:
    MessageItemUpdateBidiDirection() : m_bidi_direction(0) {}
    virtual ~MessageItemUpdateBidiDirection() {}

    uint32& get_bidi_direction() { return m_bidi_direction; }
protected:
    uint32 m_bidi_direction;
};

/* ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW */
class MessageItemShowISEOptionWindow : public MessageItemHelper
{
};

/* ISM_TRANS_CMD_CHECK_OPTION_WINDOW */
class MessageItemCheckOptionWindow : public MessageItemHelper
{
public:
    MessageItemCheckOptionWindow() : m_avail(0) {}
    virtual ~MessageItemCheckOptionWindow() {}

    uint32& get_avail_ref() { return m_avail; }
protected:
    uint32 m_avail;
};

/* ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT */
class MessageItemProcessInputDeviceEvent : public MessageItemHelper
{
public:
    MessageItemProcessInputDeviceEvent() : m_type(0), m_data(NULL), m_len(0) {}
    virtual ~MessageItemProcessInputDeviceEvent() {}

    uint32& get_type_ref() { return m_type; }
    char** get_data_ptr() { return &m_data; }
    size_t& get_len_ref() { return m_len; }
protected:
    uint32 m_type;
    char* m_data;
    size_t m_len;
};

/* SCIM_TRANS_CMD_SET_AUTOCAPITAL_TYPE */
class MessageItemSetAutocapitalType : public MessageItemHelper
{
public:
    MessageItemSetAutocapitalType() : m_auto_capital_type(0) {}
    virtual ~MessageItemSetAutocapitalType() {}

    uint32& get_auto_capital_type_ref() { return m_auto_capital_type; }
protected:
    uint32 m_auto_capital_type;
};

template <typename T>
inline T*
alloc_message() /* We could use memory pool in the future for performance enhancement */
{
    return new T;
}

template <typename T>
inline void
dealloc_message(T* ptr) /* We could use memory pool in the future for performance enhancement */
{
    if (ptr) delete ptr;
}

class MessageQueue
{
public:
    MessageQueue() {}
    virtual ~MessageQueue() { destroy(); }

    void create()
    {

    }
    void destroy()
    {
        for (MESSAGE_LIST::iterator iter = m_list_messages.begin();
            iter != m_list_messages.end(); advance(iter, 1)) {
            /* Here we are using MessageItem type template deallocator, should be cautious when using memory pool */
            dealloc_message<MessageItem>(*iter);
        }
        m_list_messages.clear();
    }

    bool has_pending_message() {
        return (m_list_messages.size() > 0);
    }

    MessageItem* get_pending_message()
    {
        MessageItem* ret = NULL;
        if (!m_list_messages.empty()) {
            ret = m_list_messages.front();
        }
        return ret;
    }

    MessageItem* get_pending_message_by_cmd(int cmd)
    {
        MessageItem* ret = NULL;
        for (MESSAGE_LIST::iterator iter = m_list_messages.begin();
            !ret && iter != m_list_messages.end(); advance(iter, 1)) {
            if ((*iter)->get_command_ref() == cmd) {
                ret = (*iter);
            }
        }
        return ret;
    }

    void remove_message(MessageItem *message)
    {
        if (message) {
            for (MESSAGE_LIST::iterator iter = m_list_messages.begin();
                iter != m_list_messages.end(); advance(iter, 1)) {
                if ((*iter) == message) {
                    iter = m_list_messages.erase(iter);
                }
            }
            /* Here we are using MessageItem type template deallocator, should be cautious when using memory pool */
            dealloc_message<MessageItem>(message);
        }
        return;
    }

    bool read_from_transaction(scim::Transaction &transaction)
    {
        int cmd;

        uint32 ic = (uint32)-1;
        String ic_uuid;

        if (!transaction.get_command(cmd) || cmd != SCIM_TRANS_CMD_REPLY) {
            LOGW("wrong format of transaction\n");
            return true;
        }

        /* If there are ic and ic_uuid then read them. */
        if (!(transaction.get_data_type() == SCIM_TRANS_DATA_COMMAND) &&
            !(transaction.get_data(ic) && transaction.get_data(ic_uuid))) {
            LOGW("wrong format of transaction\n");
            return true;
        }

        while (transaction.get_command(cmd)) {
            switch (cmd) {
                case SCIM_TRANS_CMD_EXIT:
                {
                    MessageItemExit *message = alloc_message<MessageItemExit>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_RELOAD_CONFIG:
                {
                    MessageItemReloadConfig *message = alloc_message<MessageItemReloadConfig>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_UPDATE_SCREEN:
                {
                    MessageItemUpdateScreen *message = alloc_message<MessageItemUpdateScreen>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_screen_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateScreen>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION:
                {
                    MessageItemUpdateSpotLocation *message = alloc_message<MessageItemUpdateSpotLocation>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_x_ref()) && transaction.get_data(message->get_y_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateSpotLocation>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_CURSOR_POSITION:
                {
                    MessageItemUpdateCursorPosition *message = alloc_message<MessageItemUpdateCursorPosition>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_cursor_pos_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateCursorPosition>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT:
                {
                    MessageItemUpdateSurroundingText *message = alloc_message<MessageItemUpdateSurroundingText>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_text_ref()) &&
                            transaction.get_data(message->get_cursor_ref())) {
                            message->get_ic_ref() = ic;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateSurroundingText>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_SELECTION:
                {
                    MessageItemUpdateSelection *message = alloc_message<MessageItemUpdateSelection>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_text_ref())) {
                            message->get_ic_ref() = ic;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateSelection>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_TRIGGER_PROPERTY:
                {
                    MessageItemTriggerProperty *message = alloc_message<MessageItemTriggerProperty>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_property_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemTriggerProperty>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT:
                {
                    MessageItemHelperProcessImengineEvent *message = alloc_message<MessageItemHelperProcessImengineEvent>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_transaction_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemHelperProcessImengineEvent>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT:
                {
                    MessageItemHelperAttachInputContext *message = alloc_message<MessageItemHelperAttachInputContext>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT:
                {
                    MessageItemHelperDetachInputContext *message = alloc_message<MessageItemHelperDetachInputContext>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_FOCUS_OUT:
                {
                    MessageItemFocusOut *message = alloc_message<MessageItemFocusOut>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_FOCUS_IN:
                {
                    MessageItemFocusIn *message = alloc_message<MessageItemFocusIn>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SHOW_ISE_PANEL:
                {
                    MessageItemShowISEPanel *message = alloc_message<MessageItemShowISEPanel>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_data_ptr(), message->get_len_ref())) {
                            message->get_ic_ref() = ic;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemShowISEPanel>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_HIDE_ISE_PANEL:
                {
                    MessageItemHideISEPanel *message = alloc_message<MessageItemHideISEPanel>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY:
                {
                    MessageItemGetActiveISEGeometry *message = alloc_message<MessageItemGetActiveISEGeometry>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_ISE_MODE:
                {
                    MessageItemSetISEMode *message = alloc_message<MessageItemSetISEMode>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_mode_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetISEMode>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_ISE_LANGUAGE:
                {
                    MessageItemSetISELanguage *message = alloc_message<MessageItemSetISELanguage>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_language_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetISELanguage>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_ISE_IMDATA:
                {
                    MessageItemSetISEImData *message = alloc_message<MessageItemSetISEImData>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_imdata_ptr(), message->get_len_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetISEImData>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_ISE_IMDATA:
                {
                    MessageItemGetISEImdata *message = alloc_message<MessageItemGetISEImdata>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE:
                {
                    MessageItemGetISELanguageLocale *message = alloc_message<MessageItemGetISELanguageLocale>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_RETURN_KEY_TYPE:
                {
                    MessageItemSetReturnKeyType *message = alloc_message<MessageItemSetReturnKeyType>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_type_ref())) {
                            m_list_messages.push_back(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_RETURN_KEY_TYPE:
                {
                    MessageItemGetReturnKeyType *message = alloc_message<MessageItemGetReturnKeyType>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE:
                {
                    MessageItemSetReturnKeyDisable *message = alloc_message<MessageItemSetReturnKeyDisable>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_disabled_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetReturnKeyDisable>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE:
                {
                    MessageItemGetReturnKeyDisable *message = alloc_message<MessageItemGetReturnKeyDisable>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_PROCESS_KEY_EVENT:
                {
                    MessageItemProcessKeyEvent *message = alloc_message<MessageItemProcessKeyEvent>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_key_ref()) &&
                            transaction.get_data(message->get_serial_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemProcessKeyEvent>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_LAYOUT:
                {
                    MessageItemSetLayout *message = alloc_message<MessageItemSetLayout>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_layout_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetLayout>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_GET_LAYOUT:
                {
                    MessageItemGetLayout *message = alloc_message<MessageItemGetLayout>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_INPUT_MODE:
                {
                    MessageItemSetInputMode *message = alloc_message<MessageItemSetInputMode>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_input_mode_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetInputMode>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_CAPS_MODE:
                {
                    MessageItemSetCapsMode *message = alloc_message<MessageItemSetCapsMode>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_mode_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetCapsMode>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT:
                {
                    MessageItemPanelResetInputContext *message = alloc_message<MessageItemPanelResetInputContext>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_CANDIDATE_UI:
                {
                    MessageItemUpdateCandidateUI *message = alloc_message<MessageItemUpdateCandidateUI>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_style_ref()) &&
                            transaction.get_data(message->get_mode_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateCandidateUI>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY:
                {
                    MessageItemUpdateCandidateGeometry *message = alloc_message<MessageItemUpdateCandidateGeometry>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_rectinfo_ref().pos_x)
                            && transaction.get_data(message->get_rectinfo_ref().pos_y)
                            && transaction.get_data(message->get_rectinfo_ref().width)
                            && transaction.get_data(message->get_rectinfo_ref().height)) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateCandidateGeometry>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE:
                {
                    MessageItemUpdateKeyboardISE *message = alloc_message<MessageItemUpdateKeyboardISE>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        String name, uuid;
                        if (transaction.get_data(message->get_name_ref()) &&
                            transaction.get_data(message->get_uuid_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateKeyboardISE>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST:
                {
                    MessageItemUpdateKeyboardISEList *message = alloc_message<MessageItemUpdateKeyboardISEList>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        uint32 num;
                        String ise;
                        if (transaction.get_data(num)) {
                            for (unsigned int i = 0; i < num; i++) {
                                if (transaction.get_data(ise)) {
                                    message->get_list_ref().push_back (ise);
                                } else {
                                    message->get_list_ref().clear ();
                                    break;
                                }
                            }
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateKeyboardISEList>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW:
                {
                    MessageItemCandidateMoreWindowShow *message = alloc_message<MessageItemCandidateMoreWindowShow>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE:
                {
                    MessageItemCandidateMoreWindowHide *message = alloc_message<MessageItemCandidateMoreWindowHide>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SELECT_AUX:
                {
                    MessageItemSelectAux *message = alloc_message<MessageItemSelectAux>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_item_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSelectAux>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_SELECT_CANDIDATE: //FIXME:remove if useless
                {
                    MessageItemSelectCandidate *message = alloc_message<MessageItemSelectCandidate>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_item_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSelectCandidate>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP: //FIXME:remove if useless
                {
                    MessageItemLookupTablePageUp *message = alloc_message<MessageItemLookupTablePageUp>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN: //FIXME:remove if useless
                {
                    MessageItemLookupTablePageDown *message = alloc_message<MessageItemLookupTablePageDown>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE:
                {
                    MessageItemUpdateLookupTablePageSize *message = alloc_message<MessageItemUpdateLookupTablePageSize>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_size_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateLookupTablePageSize>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_CANDIDATE_SHOW: //FIXME:remove if useless
                {
                    MessageItemCandidateShow *message = alloc_message<MessageItemCandidateShow>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_CANDIDATE_HIDE: //FIXME:remove if useless
                {
                    MessageItemCandidateHide *message = alloc_message<MessageItemCandidateHide>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE: //FIXME:remove if useless
                {
                    MessageItemUpdateLookupTable *message = alloc_message<MessageItemUpdateLookupTable>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_candidate_table_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateLookupTable>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT:
                {
                    MessageItemUpdateCandidateItemLayout *message = alloc_message<MessageItemUpdateCandidateItemLayout>();
                    if (transaction.get_data(message->get_row_items_ref())) {
                        m_list_messages.push_back(message);
                    } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateCandidateItemLayout>(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_SELECT_ASSOCIATE:
                {
                    MessageItemSelectAssociate *message = alloc_message<MessageItemSelectAssociate>();
                    if (transaction.get_data(message->get_item_ref())) {
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSelectAssociate>(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP:
                {
                    MessageItemAssociateTablePageUp *message = alloc_message<MessageItemAssociateTablePageUp>();
                    message->get_ic_ref() = ic;
                    message->get_ic_uuid_ref() = ic_uuid;
                    m_list_messages.push_back(message);
                    break;
                }
                case ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN:
                {
                    MessageItemAssociateTablePageDown *message = alloc_message<MessageItemAssociateTablePageDown>();
                    message->get_ic_ref() = ic;
                    message->get_ic_uuid_ref() = ic_uuid;
                    m_list_messages.push_back(message);
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE:
                {
                    MessageItemUpdateAssociateTablePageSize *message = alloc_message<MessageItemUpdateAssociateTablePageSize>();
                    if (transaction.get_data(message->get_size_ref())) {
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    } else {
                        LOGW("wrong format of transaction\n");
                        dealloc_message<MessageItemUpdateAssociateTablePageSize>(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_RESET_ISE_CONTEXT:
                {
                    MessageItemResetISEContext *message = alloc_message<MessageItemResetISEContext>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_TURN_ON_LOG:
                {
                    MessageItemTurnOnLog *message = alloc_message<MessageItemTurnOnLog>();
                    if (transaction.get_data(message->get_state_ref())) {
                        m_list_messages.push_back(message);
                    } else {
                        LOGW("wrong format of transaction\n");
                        dealloc_message<MessageItemTurnOnLog>(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE:
                {
                    MessageItemUpdateDisplayedCandidate *message = alloc_message<MessageItemUpdateDisplayedCandidate>();
                    if (transaction.get_data(message->get_size_ref())) {
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    } else {
                        LOGW("wrong format of transaction\n");
                        dealloc_message<MessageItemUpdateDisplayedCandidate>(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_LONGPRESS_CANDIDATE:
                {
                    MessageItemLongpressCandidate *message = alloc_message<MessageItemLongpressCandidate>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_index_ref())) {
                            message->get_ic_ref() = ic;
                            message->get_ic_uuid_ref() = ic_uuid;
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemLongpressCandidate>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SET_INPUT_HINT:
                {
                    MessageItemSetInputHint *message = alloc_message<MessageItemSetInputHint>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_input_hint_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetInputHint>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION:
                {
                    MessageItemUpdateBidiDirection *message = alloc_message<MessageItemUpdateBidiDirection>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_bidi_direction())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemUpdateBidiDirection>(message);
                        }
                    }
                    break;
                }
                case ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW:
                {
                    MessageItemShowISEOptionWindow *message = alloc_message<MessageItemShowISEOptionWindow>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        message->get_ic_ref() = ic;
                        message->get_ic_uuid_ref() = ic_uuid;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_CHECK_OPTION_WINDOW:
                {
                    MessageItemCheckOptionWindow *message = alloc_message<MessageItemCheckOptionWindow>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        m_list_messages.push_back(message);
                    }
                    break;
                }
                case ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT:
                {
                    MessageItemProcessInputDeviceEvent *message = alloc_message<MessageItemProcessInputDeviceEvent>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_type_ref()) &&
                            transaction.get_data(message->get_data_ptr(), message->get_len_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemProcessInputDeviceEvent>(message);
                        }
                    }
                    break;
                }
                case SCIM_TRANS_CMD_SET_AUTOCAPITAL_TYPE:
                {
                    MessageItemSetAutocapitalType *message = alloc_message<MessageItemSetAutocapitalType>();
                    if (message) {
                        message->get_command_ref() = cmd;
                        if (transaction.get_data(message->get_auto_capital_type_ref())) {
                            m_list_messages.push_back(message);
                        } else {
                            LOGW("wrong format of transaction\n");
                            dealloc_message<MessageItemSetAutocapitalType>(message);
                        }
                    }
                    break;
                }
            }
        }

        return true;
    }

protected:
    typedef std::list<MessageItem*> MESSAGE_LIST;
    MESSAGE_LIST m_list_messages;
};

} /* namespace scim */

#endif  /* __ISF_MESSAGE_H */

/*
vi:ts=4:expandtab:nowrap
*/
