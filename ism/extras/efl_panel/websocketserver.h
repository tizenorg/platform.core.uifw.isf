/*
 * Copyright 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef _WEB_SOCKET_SERVER_H_
#define _WEB_SOCKET_SERVER_H_

#define Uses_SCIM_PANEL_AGENT

#include <queue>
#include <string>
#include <sstream>
#include <vector>
#include <Ecore.h>
#include <Ecore_IMF.h>

/* Wait for at most 1 second */
const struct timeval REPLY_TIMEOUT = {1, 0};

typedef enum {
    ISE_MESSAGE_TYPE_PLAIN,
    ISE_MESSAGE_TYPE_QUERY,
    ISE_MESSAGE_TYPE_REPLY,
} ISE_MESSAGE_TYPES;

const std::string ISE_MESSAGE_TYPE_STRINGS[] = {
    "plain", // ISE_MESSAGE_TYPE_PLAIN,
    "query", // ISE_MESSAGE_TYPE_QUERY
    "reply", // ISE_MESSAGE_TYPE_REPLY
};

typedef enum {
    ISE_MESSAGE_COMMAND_INIT,
    ISE_MESSAGE_COMMAND_EXIT,

    ISE_MESSAGE_COMMAND_FOCUS_IN,
    ISE_MESSAGE_COMMAND_FOCUS_OUT,
    ISE_MESSAGE_COMMAND_SHOW,
    ISE_MESSAGE_COMMAND_HIDE,
    ISE_MESSAGE_COMMAND_SET_ROTATION,
    ISE_MESSAGE_COMMAND_UPDATE_CURSOR_POSITION,
    ISE_MESSAGE_COMMAND_SET_LANGUAGE,
    ISE_MESSAGE_COMMAND_SET_IMDATA,
    ISE_MESSAGE_COMMAND_GET_IMDATA,
    ISE_MESSAGE_COMMAND_SET_RETURN_KEY_TYPE,
    ISE_MESSAGE_COMMAND_GET_RETURN_KEY_TYPE,
    ISE_MESSAGE_COMMAND_SET_RETURN_KEY_DISABLE,
    ISE_MESSAGE_COMMAND_GET_RETURN_KEY_DISABLE,
    ISE_MESSAGE_COMMAND_SET_LAYOUT,
    ISE_MESSAGE_COMMAND_GET_LAYOUT,
    ISE_MESSAGE_COMMAND_RESET_INPUT_CONTEXT,
    ISE_MESSAGE_COMMAND_PROCESS_KEY_EVENT,

    ISE_MESSAGE_COMMAND_LOG,
    ISE_MESSAGE_COMMAND_COMMIT_STRING,
    ISE_MESSAGE_COMMAND_UPDATE_PREEDIT_STRING,
    ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    ISE_MESSAGE_COMMAND_SEND_MOUSE_KEY,
    ISE_MESSAGE_COMMAND_SEND_MOUSE_MOVE,
    ISE_MESSAGE_COMMAND_SEND_WHEEL_MOVE,
    ISE_MESSAGE_COMMAND_SEND_AIR_INPUT,
    ISE_MESSAGE_COMMAND_SEND_AIR_SETTING,
    ISE_MESSAGE_COMMAND_FORWARD_KEY_EVENT,
    ISE_MESSAGE_COMMAND_SET_KEYBOARD_SIZES,
    ISE_MESSAGE_COMMAND_LOGIN,

    ISE_MESSAGE_COMMANDS_NUM,

    ISE_MESSAGE_COMMANDS_GET_SURROUNDING_TEXT,
} ISE_MESSAGE_COMMANDS;

const std::string ISE_MESSAGE_COMMAND_STRINGS[] = {
    "init", // ISE_MESSAGE_COMMAND_INIT,
    "exit", // ISE_MESSAGE_COMMAND_EXIT,

    "focus_in", // ISE_MESSAGE_COMMAND_FOCUS_IN,
    "focus_out", // ISE_MESSAGE_COMMAND_FOCUS_OUT,
    "show", // ISE_MESSAGE_COMMAND_SHOW,
    "hide", // ISE_MESSAGE_COMMAND_HIDE,
    "set_rotation", // ISE_MESSAGE_COMMAND_SET_ROTATION,
    "update_cursor_position", // ISE_MESSAGE_COMMAND_UPDATE_CURSOR_POSITION,
    "set_language", // ISE_MESSAGE_COMMAND_SET_LANGUAGE,
    "set_imdata", // ISE_MESSAGE_COMMAND_SET_IMDATA,
    "get_imdata", // ISE_MESSAGE_COMMAND_GET_IMDATA,
    "set_return_key_type", // ISE_MESSAGE_COMMAND_SET_RETURN_KEY_TYPE,
    "get_return_key_type", // ISE_MESSAGE_COMMAND_GET_RETURN_KEY_TYPE,
    "set_return_key_disable", // ISE_MESSAGE_COMMAND_SET_RETURN_KEY_DISABLE,
    "get_return_key_disable", // ISE_MESSAGE_COMMAND_GET_RETURN_KEY_DISABLE,
    "set_layout", // ISE_MESSAGE_COMMAND_SET_LAYOUT,
    "get_layout", // ISE_MESSAGE_COMMAND_GET_LAYOUT,
    "reset_input_context", // ISE_MESSAGE_COMMAND_RESET_INPUT_CONTEXT,
    "process_key_event", // ISE_MESSAGE_COMMAND_PROCESS_KEY_EVENT,

    "log", // ISE_MESSAGE_COMMAND_LOG,
    "commit_string", // ISE_MESSAGE_COMMAND_COMMIT_STRING,
    "update_preedit_string", // ISE_MESSAGE_COMMAND_UPDATE_PREEDIT_STRING,
    "send_key_event", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "send_mouse_key", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "send_mouse_move", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "send_wheel_move", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "send_air_input", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "send_air_setting", // ISE_MESSAGE_COMMAND_SEND_KEY_EVENT,
    "forward_key_event", // ISE_MESSAGE_COMMAND_FORWARD_KEY_EVENT,
    "set_keyboard_sizes", // ISE_MESSAGE_COMMAND_SET_KEYBOARD_SIZES,
    "login", // ISE_MESSAGE_COMMAND_LOGIN,
};

typedef struct {
    std::string type;
    std::string command;
    std::vector<std::string> values;
} ISE_MESSAGE;

typedef struct {
    int type_value;
    std::string type_string;
} ISE_TYPE_VALUE_STRING;

const ISE_TYPE_VALUE_STRING ISE_RETURN_KEY_TYPES[] = {
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT, "default"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE, "done"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO, "go"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_JOIN, "join"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_LOGIN, "login"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT, "next"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH, "search"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEND, "send"},
    {ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SIGNIN, "signin"},
};

const ISE_TYPE_VALUE_STRING ISE_LAYOUT_TYPES[] = {
    {ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL, "normal"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBER, "number"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL, "email"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_URL, "url"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER, "phonenumber"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_IP, "ip"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_MONTH, "month"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY, "numberonly"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD, "password"},
    {ECORE_IMF_INPUT_PANEL_LAYOUT_DATETIME, "datetime"},
};

const ISE_TYPE_VALUE_STRING ISE_LANGUAGE_TYPES[] = {
    {ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC, "automatic"},
    {ECORE_IMF_INPUT_PANEL_LANG_ALPHABET, "alphabet"},
};

/* FIXME : Should consider the case if the boolean value does not match with EINA_TRUE or EINA_FALSE */
const ISE_TYPE_VALUE_STRING ISE_TRUEFALSE_TYPES[] = {
    {EINA_FALSE, "false"},
    {EINA_TRUE, "true"},
};


class CStringTokenizer
{
public:
    static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            if (!item.empty()) {
                elems.push_back(item);
            }
        }
        if (elems.size() < 3) {
            elems.push_back("");
        }
        return elems;
    }

    static std::vector<std::string> split(const std::string &s, char delim) {
        std::vector<std::string> elems;
        split(s, delim, elems);
        return elems;
    }
};

class CISEMessageSerializer
{
protected:
    /* FIXME : Temporary solution for distinguish commands and values */
    static const char MESSAGE_DELIMETER = '|';

public:
    static std::string serialize(ISE_MESSAGE message) {
        std::string ret;
        ret += message.type;
        ret += MESSAGE_DELIMETER;
        ret += message.command;
        for (unsigned int loop = 0;loop < message.values.size();loop++) {
            ret += MESSAGE_DELIMETER;
            ret += message.values.at(loop);
        }
        return ret;
    }

    static ISE_MESSAGE deserialize(std::string message) {
        ISE_MESSAGE ret;
        std::vector<std::string> vec = CStringTokenizer::split(message, MESSAGE_DELIMETER);
        if (vec.size() > 1) {
            ret.type = vec.at(0);
            vec.erase(vec.begin());
            ret.command = vec.at(0);
            vec.erase(vec.begin());
            ret.values = vec;
        }
        return ret;
    }
};

class WebSocketServer {
public:
    WebSocketServer();
    virtual ~WebSocketServer();

    bool init();
    bool exit();

    void signal(int sig);

    void on_init();
    void on_exit();

    void on_focus_in(int ic);
    void on_focus_out(int ic);

    void get_surrounding_text(int cursor, const char *text);

    void on_show(int ic);
    void on_hide(int ic);

    void on_set_rotation(int degree);

    void on_update_cursor_position(int ic, int cursor_pos);

    void on_set_language(unsigned int language);

    void on_set_imdata(char *buf, unsigned int len);
    void on_get_imdata(char **buf, unsigned int *len);

    void on_set_return_key_type(unsigned int type);
    void on_get_return_key_type(unsigned int *type);

    void on_set_return_key_disable(unsigned int disabled);
    void on_get_return_key_disable(unsigned int *disabled);

    void on_set_layout(unsigned int layout);
    void on_get_layout(unsigned int *layout);

    void on_reset_input_context(int ic);

    void on_process_key_event(unsigned int code, unsigned int mask, unsigned int layout, unsigned int *ret);

    std::queue<ISE_MESSAGE>& get_send_message_queue();
    std::queue<ISE_MESSAGE>& get_recv_message_queue();
    Ecore_Pipe* get_recv_message_pipe();

    void wait_for_reply_message();

    void process_recved_messages();
    bool process_recved_messages_until_reply_found(std::string command, std::vector<std::string> &values);
    void handle_recved_message(ISE_MESSAGE &message);

    static WebSocketServer* get_current_instance();
protected:
    static WebSocketServer *m_current_instance;
    
    std::queue<ISE_MESSAGE> m_send_message_queue;
    std::queue<ISE_MESSAGE> m_recv_message_queue;

    Ecore_Pipe *m_recv_message_pipe;
};

#endif // _WEB_HELPER_AGENT_WEBSOCKET_H_
