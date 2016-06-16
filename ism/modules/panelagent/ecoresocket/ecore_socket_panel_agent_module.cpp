/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *
 * Contact: Li Zhang <li2012.zhang@samsung.com>
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
#include <sys/times.h>
#include <dlog.h>
#include <unistd.h>
#include <Ecore.h>
#include "scim_private.h"
#include "scim.h"
#include "scim_stl_map.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_ECORE_SOCKET_MODULE"

#define MIN_REPEAT_TIME     2.0

#define IMEMANAGER_PRIVILEGE "http://tizen.org/privilege/imemanager"

#define SCIM_CONFIG_PANEL_SOCKET_CONFIG_READONLY    "/Panel/Socket/ConfigReadOnly"

namespace scim
{

struct HelperClientStub {
    int id;
    int ref;

    HelperClientStub(int i = 0, int r = 0) : id(i), ref(r) { }
};

struct IMControlStub {
    std::vector<ISE_INFO> info;
    std::vector<int> count;
};

#define DEFAULT_CONTEXT_VALUE 0xfff

#define scim_module_init ecoresocket_LTX_scim_module_init
#define scim_module_exit ecoresocket_LTX_scim_module_exit
#define scim_panel_agent_module_init ecoresocket_LTX_scim_panel_agent_module_init
#define scim_panel_agent_module_get_instance ecoresocket_LTX_scim_panel_agent_module_get_instance


static ConfigPointer        _config;


//==================================== PanelAgent ===========================
class EcoreSocketPanelAgent: public PanelAgentBase
{
    bool                                m_should_exit;

    int                                 m_socket_timeout;
    String                              m_socket_address;
    SocketServer                        m_socket_server;

    Transaction                         m_send_trans;
    Transaction                         m_recv_trans;
    Transaction                         m_nest_trans;

    bool                                m_should_shared_ise;
    bool                                m_ise_exiting;
    bool                                m_config_readonly;

    std::vector<Ecore_Fd_Handler*>     _read_handler_list;

    InfoManager* m_info_manager;

public:
    EcoreSocketPanelAgent()
        : PanelAgentBase ("ecore_socket"),
          m_should_exit(false),
          m_socket_timeout(scim_get_default_socket_timeout()),
          m_should_shared_ise(false),
          m_ise_exiting(false),
          m_config_readonly (false) {
        m_socket_server.signal_connect_accept(slot(this, &EcoreSocketPanelAgent::socket_accept_callback));
        m_socket_server.signal_connect_receive(slot(this, &EcoreSocketPanelAgent::socket_receive_callback));
        m_socket_server.signal_connect_exception(slot(this, &EcoreSocketPanelAgent::socket_exception_callback));
    }

    ~EcoreSocketPanelAgent() {
        for (unsigned int ii = 0; ii < _read_handler_list.size(); ++ii) {
            ecore_main_fd_handler_del(_read_handler_list[ii]);
        }
        _read_handler_list.clear();
    }

    bool initialize(InfoManager* info_manager, const String& display, bool resident) {
        LOGI ("");
        m_info_manager = info_manager;
        m_socket_address = scim_get_default_panel_socket_address(display);

        if (!_config.null ())
            m_config_readonly = _config->read (String (SCIM_CONFIG_PANEL_SOCKET_CONFIG_READONLY), false);
        else {
            m_config_readonly = false;
            LOGW("config is not ready");
        }

        m_socket_server.shutdown();

        if (m_socket_server.create(SocketAddress(m_socket_address))) {
            Ecore_Fd_Handler* panel_agent_read_handler = NULL;
            panel_agent_read_handler = ecore_main_fd_handler_add(get_server_id(), ECORE_FD_READ, panel_agent_handler, this, NULL, NULL);
            _read_handler_list.push_back(panel_agent_read_handler);
            return true;
        }
        LOGE("create server failed\n");
        return false;
    }

    bool valid(void) const {
        return m_socket_server.valid();
    }

    void stop(void) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::stop ()\n";
        LOGI ("");
        lock();
        m_should_exit = true;
        unlock();
        SocketClient  client;
        if (client.connect(SocketAddress(m_socket_address))) {
            client.close();
        }
        _config.reset();
    }
private:
    void update_panel_event(int client, uint32 context_id, int cmd, uint32 nType, uint32 nValue) {
        LOGI ("client id:%d\n", client);
        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_command(cmd);
        m_send_trans.put_data(nType);
        m_send_trans.put_data(nValue);
        m_send_trans.write_to_socket(client_socket);
    }

    void move_preedit_caret(int client, uint32 context_id, uint32 position) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::move_preedit_caret (" << position << ")\n";
        LOGI ("client id:%d\n", client);
        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_MOVE_PREEDIT_CARET);
        m_send_trans.put_data((uint32) position);
        m_send_trans.write_to_socket(client_socket);
    }

//useless
#if 0
    void request_help(int client_id, uint32 context_id) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::request_help ()\n";
        LOGI ("client id:%d", client_id);

        Socket client_socket(client_id);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_PANEL_REQUEST_HELP);
        m_send_trans.write_to_socket(client_socket);

    }

    void request_factory_menu(int client_id, uint32 context_id) {
        LOGI ("client id:%d", client_id);
        Socket client_socket(client_id);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_PANEL_REQUEST_FACTORY_MENU);
        m_send_trans.write_to_socket(client_socket);
    }
#endif

    void reset_keyboard_ise(int client, uint32 context_id) {
        LOGI ("client id:%d\n", client);
        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context_id);
        m_send_trans.put_command(ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_keyboard_ise_list(int client, uint32 context) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);
        m_send_trans.put_command(ISM_TRANS_CMD_PANEL_UPDATE_KEYBOARD_ISE);
        m_send_trans.write_to_socket(client_socket);
    }

    void change_factory(int client, uint32 context, const String&  uuid) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::change_factory (" << uuid << ")\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);
        m_send_trans.put_command(SCIM_TRANS_CMD_PANEL_CHANGE_FACTORY);
        m_send_trans.put_data(uuid);
        m_send_trans.write_to_socket(client_socket);
    }

    void helper_candidate_show(int client, uint32 context, const String&  uuid) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";
        LOGI ("client id:%d\n", client);


        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_CANDIDATE_SHOW);
        m_send_trans.write_to_socket(client_socket);
    }

    void helper_candidate_hide(int client, uint32 context, const String&  uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_CANDIDATE_HIDE);
        m_send_trans.write_to_socket(client_socket);
    }

    void candidate_more_window_show(int client, uint32 context) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";
        LOGI ("client id:%d\n", client);


        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());
        m_send_trans.put_command(ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_SHOW);
        m_send_trans.write_to_socket(client_socket);
    }

    void candidate_more_window_hide(int client, uint32 context) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());
        m_send_trans.put_command(ISM_TRANS_CMD_CANDIDATE_MORE_WINDOW_HIDE);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_helper_lookup_table(int client, uint32 context, const String& uuid, const LookupTable& table) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_LOOKUP_TABLE);
        m_send_trans.put_data(table);
        m_send_trans.write_to_socket(client_socket);
    }

    void select_aux(int client, uint32 contextid, uint32 item) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::select_aux (" << item << ")\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) contextid);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_SELECT_AUX);
        m_send_trans.put_data((uint32)item);
        m_send_trans.write_to_socket(client_socket);
    }

    void select_candidate(int client, uint32 context, uint32 item) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(SCIM_TRANS_CMD_SELECT_CANDIDATE);
        m_send_trans.put_data((uint32)item);
        m_send_trans.write_to_socket(client_socket);
    }

    void lookup_table_page_up(int client, uint32 context) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::lookup_table_page_up ()\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_UP);
        m_send_trans.write_to_socket(client_socket);
    }

    void lookup_table_page_down(int client, uint32 context) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(SCIM_TRANS_CMD_LOOKUP_TABLE_PAGE_DOWN);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_lookup_table_page_size(int client, uint32 context, uint32 size) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE_PAGE_SIZE);
        m_send_trans.put_data(size);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_candidate_item_layout(int client, uint32 context, const std::vector<uint32>& row_items) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_CANDIDATE_ITEM_LAYOUT);
        m_send_trans.put_data(row_items);
        m_send_trans.write_to_socket(client_socket);
    }

    void select_associate(int client, uint32 context, uint32 item) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_SELECT_ASSOCIATE);
        m_send_trans.put_data((uint32)item);
        m_send_trans.write_to_socket(client_socket);
    }

    void associate_table_page_up(int client, uint32 context) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_UP);
        m_send_trans.write_to_socket(client_socket);
    }

    void associate_table_page_down(int client, uint32 context) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_ASSOCIATE_TABLE_PAGE_DOWN);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_associate_table_page_size(int client, uint32 context, uint32 size) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE_PAGE_SIZE);
        m_send_trans.put_data(size);
        m_send_trans.write_to_socket(client_socket);
    }

    void update_displayed_candidate_number(int client, uint32 context, uint32 size) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_DISPLAYED_CANDIDATE);
        m_send_trans.put_data(size);
        m_send_trans.write_to_socket(client_socket);
    }

    void send_longpress_event(int client, uint32 context, int index) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(ISM_TRANS_CMD_LONGPRESS_CANDIDATE);
        m_send_trans.put_data(index);
        m_send_trans.write_to_socket(client_socket);
    }

    void trigger_property(int client, uint32 context, const String&  property) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT)
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());

        m_send_trans.put_command(SCIM_TRANS_CMD_TRIGGER_PROPERTY);
        m_send_trans.put_data(property);
        m_send_trans.write_to_socket(client_socket);
    }

    void focus_out_helper(int client, uint32 context, const String& uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_FOCUS_OUT);
        m_send_trans.write_to_socket(client_socket);
    }

    void focus_in_helper(int client, uint32 context, const String& uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_FOCUS_IN);
        m_send_trans.write_to_socket(client_socket);
    }

    void show_helper(int client, uint32 context, const String& uuid, char* data, size_t& len) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SHOW_ISE_PANEL);
        m_send_trans.put_data(data, len);
        m_send_trans.write_to_socket(client_socket);
    }

    void hide_helper(int client, uint32 context, const String& uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_HIDE_ISE_PANEL);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_mode(int client, uint32 context, const String& uuid, uint32& mode) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_ISE_MODE);
        m_send_trans.put_data(mode);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_language(int client, uint32 context, const String& uuid, uint32& language) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_ISE_LANGUAGE);
        m_send_trans.put_data(language);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_imdata(int client, uint32 context, const String& uuid, const char* imdata, size_t& len) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_ISE_IMDATA);
        m_send_trans.put_data(imdata, len);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_return_key_type(int client, uint32 context, const String& uuid, uint32 type) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_RETURN_KEY_TYPE);
        m_send_trans.put_data(type);
        m_send_trans.write_to_socket(client_socket);
    }

    void get_helper_return_key_type(int client, uint32 context, const String& uuid, _OUT_ uint32& type) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        Transaction trans;
        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_RETURN_KEY_TYPE);

        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(type)) {
            SCIM_DEBUG_MAIN(1) << __func__ << " success\n";

        } else {
            LOGW ("read failed\n");
        }
    }

    void set_helper_return_key_disable(int client, uint32 context, const String& uuid, uint32 disabled) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE);
        m_send_trans.put_data(disabled);
        m_send_trans.write_to_socket(client_socket);
    }

    void get_helper_return_key_disable(int client, uint32 context, const String& uuid, _OUT_ uint32& disabled) {

        Socket client_socket(client);
        LOGI ("client id:%d", client);

        Transaction trans;

        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE);

        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(disabled)) {
            SCIM_DEBUG_MAIN(1) << __func__ << " success\n";

        } else {
            LOGW ("read failed");
        }
    }

    void set_helper_layout(int client, uint32 context, const String& uuid, uint32& layout) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_LAYOUT);
        m_send_trans.put_data(layout);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_input_mode(int client, uint32 context, const String& uuid, uint32& mode) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_INPUT_MODE);
        m_send_trans.put_data(mode);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_input_hint(int client, uint32 context, const String& uuid, uint32& hint) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_INPUT_HINT);
        m_send_trans.put_data(hint);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_bidi_direction(int client, uint32 context, const String& uuid, uint32& direction) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION);
        m_send_trans.put_data(direction);
        m_send_trans.write_to_socket(client_socket);
    }

    void set_helper_caps_mode(int client, uint32 context, const String& uuid, uint32& mode) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SET_CAPS_MODE);
        m_send_trans.put_data(mode);
        m_send_trans.write_to_socket(client_socket);
    }

    void show_helper_option_window(int client, uint32 context, const String& uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW);
        m_send_trans.write_to_socket(client_socket);
    }

    bool process_key_event(int client, uint32 context, const String& uuid, KeyEvent& key, _OUT_ uint32& result) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        Transaction trans;

        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(SCIM_TRANS_CMD_PROCESS_KEY_EVENT);
        trans.put_data(key);
        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(result)) {
            SCIM_DEBUG_MAIN(1) << __func__ << " success\n";
            return true;
        } else {
            LOGW ("read failed\n");
        }

        return false;
    }

    bool get_helper_geometry(int client, uint32 context, String& uuid, _OUT_ struct rectinfo& info) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        Transaction trans;
        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY);

        if (trans.write_to_socket(client_socket)) {
            int cmd;

            trans.clear();

            if (trans.read_from_socket(client_socket)
                && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
                && trans.get_data(info.pos_x)
                && trans.get_data(info.pos_y)
                && trans.get_data(info.width)
                && trans.get_data(info.height)) {
                SCIM_DEBUG_MAIN(1) << __func__ << " is successful\n";
                return true;
            } else
                LOGW ("read failed\n");
        } else
            LOGW ("write failed\n");

        return false;
    }

    void get_helper_imdata(int client, uint32 context, String& uuid, char** imdata, size_t& len) {
        LOGI ("client id:%d\n", client);
        Socket client_socket(client);

        Transaction trans;
        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_ISE_IMDATA);

        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY) {
            trans.get_data(imdata, len);
            LOGI ("length of imdata is %d", len);
        } else {
            LOGW ("read imdata failed\n");
        }
    }

    void get_helper_layout(int client, uint32 context, String& uuid, uint32& layout) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        Transaction trans;
        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_LAYOUT);

        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(layout)) {
            SCIM_DEBUG_MAIN(1) << "get_helper_layout success\n";
        } else
            LOGW ("failed\n");
    }

    void get_ise_language_locale(int client, uint32 context, String& uuid, char** data,  size_t& len) {
        SCIM_DEBUG_MAIN(4) << __func__ << "\n";
        LOGI ("client id:%d\n", client);

        Transaction trans;

        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE);

        int cmd;
        Socket client_socket(client);

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(data, len)) {
        } else {
            LOGW ("failed\n");
        }
    }

    void check_option_window(int client, uint32 context, String& uuid, _OUT_ uint32& avail) {
        LOGI ("client id:%d\n", client);

        int cmd;
        Socket client_socket(client);

        Transaction trans;

        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_CHECK_OPTION_WINDOW);
        trans.write_to_socket(client_socket);

        if (!trans.read_from_socket(client_socket, m_socket_timeout) ||
            !trans.get_command(cmd) || cmd != SCIM_TRANS_CMD_REPLY ||
            !trans.get_data(avail)) {
            LOGW ("ISM_TRANS_CMD_CHECK_OPTION_WINDOW failed\n");
        }
    }

    void reset_ise_option(int client, uint32 context) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data((uint32) context);
        m_send_trans.put_command(ISM_TRANS_CMD_RESET_ISE_OPTION);
        m_send_trans.write_to_socket(client_socket);
    }

    void reset_helper_context(int client, uint32 context, const String& uuid) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_RESET_ISE_CONTEXT);
        m_send_trans.write_to_socket(client_socket);
    }

    void reload_config(int client) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::reload_config ()\n";
        LOGI ("client id:%d\n", client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command(SCIM_TRANS_CMD_RELOAD_CONFIG);

        Socket client_socket(client);
        m_send_trans.write_to_socket(client_socket);
    }

    void exit(int client, uint32 context) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::exit ()\n";
        LOGI ("client id:%d\n", client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);

        if (m_info_manager->socket_get_client_info(client).type == HELPER_CLIENT) {
            m_send_trans.put_data(context);
            m_send_trans.put_data(m_info_manager->get_current_helper_uuid());
        }

        m_send_trans.put_command(SCIM_TRANS_CMD_EXIT);

        Socket client_socket(client);
        m_send_trans.write_to_socket(client_socket);
    }

    bool process_input_device_event(int client, uint32 context, const String& uuid, uint32 type, const char *data, size_t len, _OUT_ uint32& result) {
        LOGI("client id:%d\n", client);

        Socket client_socket(client);

        Transaction trans;

        trans.clear();
        trans.put_command(SCIM_TRANS_CMD_REPLY);
        trans.put_data(context);
        trans.put_data(uuid);
        trans.put_command(ISM_TRANS_CMD_PROCESS_INPUT_DEVICE_EVENT);
        trans.put_data(type);
        trans.put_data(data, len);
        int cmd;

        if (trans.write_to_socket(client_socket)
            && trans.read_from_socket(client_socket)
            && trans.get_command(cmd) && cmd == SCIM_TRANS_CMD_REPLY
            && trans.get_data(result)) {
            SCIM_DEBUG_MAIN(1) << __func__ << " success\n";
            return true;
        }
        else {
            LOGW("read failed\n");
        }

        return false;
    }

    void socket_update_surrounding_text(int client, uint32 context, const String& uuid, String& text, uint32 cursor) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << "...\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT);
        m_send_trans.put_data(text);
        m_send_trans.put_data(cursor);
        m_send_trans.write_to_socket(client_socket);
    }

    void socket_update_selection(int client, uint32 context, String& uuid, String text) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_SELECTION);
        m_send_trans.put_data(text);
        m_send_trans.write_to_socket(client_socket);
    }

    void socket_get_keyboard_ise_list(int client, uint32 context, const String& uuid, std::vector<String>& list) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE_LIST);
        m_send_trans.put_data(list.size());

        for (unsigned int i = 0; i < list.size(); i++)
            m_send_trans.put_data(list[i]);

        m_send_trans.write_to_socket(socket_client);
    }

    void socket_get_candidate_ui(int client, uint32 context, const String& uuid,  int style,  int mode) {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_get_candidate_ui ()\n";

        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_CANDIDATE_UI);
        m_send_trans.put_data(style);
        m_send_trans.put_data(mode);
        m_send_trans.write_to_socket(socket_client);
    }

    void socket_get_candidate_geometry(int client, uint32 context, const String& uuid, struct rectinfo& info) {
        SCIM_DEBUG_MAIN(4) << __func__ << " \n";

        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_CANDIDATE_GEOMETRY);
        m_send_trans.put_data(info.pos_x);
        m_send_trans.put_data(info.pos_y);
        m_send_trans.put_data(info.width);
        m_send_trans.put_data(info.height);
        m_send_trans.write_to_socket(socket_client);
    }

    void socket_get_keyboard_ise(int client, uint32 context, const String& uuid, String& ise_name, String& ise_uuid) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_KEYBOARD_ISE);
        m_send_trans.put_data(ise_name);
        m_send_trans.put_data(ise_uuid);
        m_send_trans.write_to_socket(socket_client);
    }

    void socket_start_helper(int client, uint32 context, const String& ic_uuid) {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_start_helper ()\n";

        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(ic_uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);
        m_send_trans.write_to_socket(client_socket);
    }

    void helper_detach_input_context(int client, uint32 context, const String& ic_uuid) {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_stop_helper ()\n";

        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_data(ic_uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_HELPER_DETACH_INPUT_CONTEXT);
        m_send_trans.write_to_socket(client_socket);
    }

    void helper_process_imengine_event(int client, uint32 context, const String& ic_uuid, const Transaction& _nest_trans) {
        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_send_helper_event ()\n";
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);

        lock();

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data(context);
        m_send_trans.put_data(ic_uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_HELPER_PROCESS_IMENGINE_EVENT);
        m_send_trans.put_data(_nest_trans);
        m_send_trans.write_to_socket(client_socket);

        unlock();
    }

    void process_helper_event(int client, uint32 context, String target_uuid, String active_uuid, Transaction& nest_trans) {
        LOGI ("client id:%d\n", client);

        SCIM_DEBUG_MAIN(4) << "PanelAgent::socket_helper_send_imengine_event (" << client << ")\n";
        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_command(SCIM_TRANS_CMD_PROCESS_HELPER_EVENT);
        m_send_trans.put_data(target_uuid);
        m_send_trans.put_data(active_uuid);
        m_send_trans.put_data(nest_trans);
        m_send_trans.write_to_socket(socket_client);
        unlock();

    }

    void socket_helper_key_event(int client, uint32 context, int cmd, KeyEvent& key) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context);
        m_send_trans.put_command(cmd);
        m_send_trans.put_data(key);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void commit_string(int client, uint32 target_context, const WideString& wstr) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(target_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_COMMIT_STRING);
        m_send_trans.put_data(wstr);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void socket_helper_get_surrounding_text(int client, uint32 context_id, uint32 maxlen_before, uint32 maxlen_after, const int fd) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_GET_SURROUNDING_TEXT);
        m_send_trans.put_data(maxlen_before);
        m_send_trans.put_data(maxlen_after);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void socket_helper_delete_surrounding_text(int client, uint32 context_id, uint32 offset, uint32 len) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT);
        m_send_trans.put_data(offset);
        m_send_trans.put_data(len);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void socket_helper_get_selection(int client, uint32 context_id, const int fd) {
        SCIM_DEBUG_MAIN(4) << __FUNCTION__ << " (" << client << ")\n";
        LOGI ("client id:%d\n", client);


        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_GET_SELECTION);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void socket_helper_set_selection(int client, uint32 context_id, uint32 start, uint32 end) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_command(SCIM_TRANS_CMD_SET_SELECTION);
        m_send_trans.put_data(start);
        m_send_trans.put_data(end);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void show_preedit_string(int client, uint32  target_context) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(target_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_SHOW_PREEDIT_STRING);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void hide_preedit_string(int client, uint32  target_context) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(target_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_HIDE_PREEDIT_STRING);
        m_send_trans.write_to_socket(socket_client);
        unlock();
    }

    void update_preedit_string(int client, uint32  target_context, WideString wstr, AttributeList& attrs, uint32 caret) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(target_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING);
        m_send_trans.put_data(wstr);
        m_send_trans.put_data(attrs);
        m_send_trans.put_data(caret);
        m_send_trans.write_to_socket(socket_client);
        unlock();

    }

    void update_preedit_caret(int client, uint32 focused_context, uint32 caret) {

        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(focused_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET);
        m_send_trans.put_data(caret);
        m_send_trans.write_to_socket(socket_client);
        unlock();

    }

    void helper_attach_input_context_and_update_screen(int client, std::vector < std::pair <uint32, String> >& helper_ic_index, uint32 current_screen) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_command(SCIM_TRANS_CMD_OK);
        m_send_trans.put_command(SCIM_TRANS_CMD_HELPER_ATTACH_INPUT_CONTEXT);

        for (size_t i = 0; i < helper_ic_index.size(); ++i) {
            m_send_trans.put_data(helper_ic_index[i].first);
            m_send_trans.put_data(helper_ic_index[i].second);
        }

        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_SCREEN);
        m_send_trans.put_data((uint32)current_screen);
        m_send_trans.write_to_socket(socket_client);
    }

    void update_ise_input_context(int client, uint32 focused_context, uint32 type, uint32 value) {
        LOGI ("client id:%d\n", client);

        Socket client_socket(client);
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(focused_context);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT);
        m_send_trans.put_data(type);
        m_send_trans.put_data(value);
        m_send_trans.write_to_socket(client_socket);

    }

    void send_private_command(int client, uint32 focused_context, String command) {
        LOGI ("client id:%d\n", client);

        Socket socket_client(client);
        lock();
        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(focused_context);
        m_send_trans.put_command(SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND);
        m_send_trans.put_data(command);
        m_send_trans.write_to_socket(socket_client);

    }

    void helper_all_update_spot_location(int client, uint32 context_id, String uuid, int x, int y) {
        LOGI ("client id:%d\n", client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data(context_id);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION);
        m_send_trans.put_data((uint32) x);
        m_send_trans.put_data((uint32) y);

        Socket client_socket(client);
        m_send_trans.write_to_socket(client_socket);

    }

    void helper_all_update_cursor_position(int client, uint32 context_id, String uuid, int cursor_pos) {
        LOGI ("client id:%d\n", client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);
        m_send_trans.put_data(context_id);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(ISM_TRANS_CMD_UPDATE_CURSOR_POSITION);
        m_send_trans.put_data((uint32) cursor_pos);

        Socket client_socket(client);
        m_send_trans.write_to_socket(client_socket);

    }

    void helper_all_update_screen(int client, uint32 context_id, String uuid, int screen) {
        LOGI ("client id:%d\n", client);

        m_send_trans.clear();
        m_send_trans.put_command(SCIM_TRANS_CMD_REPLY);

        /* FIXME: We presume that client and context are both less than 65536.
         * Hopefully, it should be true in any UNIXs.
         * So it's ok to combine client and context into one uint32. */
        m_send_trans.put_data(context_id);
        m_send_trans.put_data(uuid);
        m_send_trans.put_command(SCIM_TRANS_CMD_UPDATE_SCREEN);
        m_send_trans.put_data((uint32) screen);

        Socket client_socket(client);
        m_send_trans.write_to_socket(client_socket);

    }

private:

    static void send_fail_reply (int client_id)
    {
        Socket client_socket (client_id);
        Transaction trans;
        trans.clear ();
        trans.put_command (SCIM_TRANS_CMD_REPLY);
        trans.put_command (SCIM_TRANS_CMD_FAIL);
        trans.write_to_socket (client_socket);
    }

    bool filter_event(int fd) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::filter_event ()\n";

        return m_socket_server.filter_event(fd);
    }

    bool filter_exception_event(int fd) {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::filter_exception_event ()\n";

        return m_socket_server.filter_exception_event(fd);
    }

    int get_server_id() {
        SCIM_DEBUG_MAIN(1) << "PanelAgent::get_server_id ()\n";

        return m_socket_server.get_id();
    }

    bool socket_check_client_connection(const Socket& client) {
        SCIM_DEBUG_MAIN(3) << "PanelAgent::socket_check_client_connection (" << client.get_id() << ")\n";

        unsigned char buf [sizeof(uint32)];

        int nbytes = client.read_with_timeout(buf, sizeof(uint32), m_socket_timeout);

        if (nbytes == sizeof(uint32))
            return true;

        if (nbytes < 0) {
            LOGW ("Error occurred when reading socket: %s\n", client.get_error_message().c_str());
        } else {
            LOGW ("Timeout when reading socket\n");
        }

        return false;
    }

    /**
     * @brief Callback function for ecore fd handler.
     *
     * @param data The data to pass to this callback.
     * @param fd_handler The ecore fd handler.
     *
     * @return ECORE_CALLBACK_RENEW
     */
    static Eina_Bool panel_agent_handler(void* data, Ecore_Fd_Handler* fd_handler) {
        if (fd_handler == NULL || data == NULL)
            return ECORE_CALLBACK_RENEW;

        EcoreSocketPanelAgent* _agent = (EcoreSocketPanelAgent*)data;

        int fd = ecore_main_fd_handler_fd_get(fd_handler);

        for (unsigned int i = 0; i < _agent->_read_handler_list.size(); i++) {
            if (fd_handler == _agent->_read_handler_list [i]) {
                if (!_agent->filter_event(fd)) {
                    std::cerr << "_panel_agent->filter_event () is failed!!!\n";
                    ecore_main_fd_handler_del(fd_handler);

                    ISF_SAVE_LOG("_panel_agent->filter_event (fd=%d) is failed!!!\n", fd);
                }

                return ECORE_CALLBACK_RENEW;
            }
        }

        std::cerr << "panel_agent_handler () has received exception event!!!\n";
        _agent->filter_exception_event(fd);
        ecore_main_fd_handler_del(fd_handler);

        ISF_SAVE_LOG("Received exception event (fd=%d)!!!\n", fd);
        return ECORE_CALLBACK_RENEW;
    }

    void socket_accept_callback(SocketServer*   server,
                                const Socket&   client) {
        SCIM_DEBUG_MAIN(2) << "PanelAgent::socket_accept_callback (" << client.get_id() << ")\n";
        LOGI ("");
        lock();

        if (m_should_exit) {
            SCIM_DEBUG_MAIN(3) << "Exit Socket Server Thread.\n";
            server->shutdown();
        } else {
            //m_signal_accept_connection (client.get_id ());
            Ecore_Fd_Handler* panel_agent_read_handler = ecore_main_fd_handler_add(client.get_id(), ECORE_FD_READ, panel_agent_handler, this, NULL, NULL);
            _read_handler_list.push_back(panel_agent_read_handler);
        }

        unlock();
    }

    void socket_receive_callback(SocketServer*   server,
                                 const Socket&   client) {
        int     client_id = client.get_id();
        int     cmd     = 0;
        uint32  key     = 0;
        uint32  context = 0;
        String  uuid;

        ClientInfo client_info;

        SCIM_DEBUG_MAIN(1) << "PanelAgent::socket_receive_callback (" << client_id << ")\n";

        /* If the connection is closed then close this client. */
        if (!socket_check_client_connection(client)) {
            LOGW ("check client connection failed\n");
            socket_close_connection(server, client);
            return;
        }

        client_info = m_info_manager->socket_get_client_info(client_id);

        /* If it's a new client, then request to open the connection first. */
        if (client_info.type == UNKNOWN_CLIENT) {
            socket_open_connection(server, client);
            return;
        }

        /* If can not read the transaction,
         * or the transaction is not started with SCIM_TRANS_CMD_REQUEST,
         * or the key is mismatch,
         * just return. */
        if (!m_recv_trans.read_from_socket(client, m_socket_timeout) ||
            !m_recv_trans.get_command(cmd) || cmd != SCIM_TRANS_CMD_REQUEST ||
            !m_recv_trans.get_data(key)    || key != (uint32) client_info.key) {
            LOGW ("cmd:%d key:%d client info key: %d\n", cmd, key, client_info.key);
            return;
        }

        if (client_info.type == FRONTEND_ACT_CLIENT) {
            if (m_recv_trans.get_data(context)) {
                SCIM_DEBUG_MAIN(1) << "PanelAgent::FrontEnd Client, context = " << context << "\n";
                socket_transaction_start();

                while (m_recv_trans.get_command(cmd)) {
                    LOGI ("PanelAgent::cmd = %d\n", cmd);

                    if (cmd == ISM_TRANS_CMD_REGISTER_PANEL_CLIENT) {
                        uint32 id = 0;

                        if (m_recv_trans.get_data(id)) {
                            m_info_manager->register_panel_client(client_id, id);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    }

                    else if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_INPUT_CONTEXT) {
                        if (m_recv_trans.get_data(uuid)) {
                            m_info_manager->register_input_context(client_id, context, uuid);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    }

                    else if (cmd == SCIM_TRANS_CMD_PANEL_REMOVE_INPUT_CONTEXT) {
                        m_info_manager->remove_input_context(client_id, context);
                        continue;
                    }

                    else if (cmd == SCIM_TRANS_CMD_PANEL_RESET_INPUT_CONTEXT) {
                        m_info_manager->socket_reset_input_context(client_id, context);
                        continue;
                    }

                    else if (cmd == SCIM_TRANS_CMD_FOCUS_IN) {
                        SCIM_DEBUG_MAIN(4) << "    SCIM_TRANS_CMD_FOCUS_IN (" << "client:" << client_id << " context:" << context << ")\n";

                        if (m_recv_trans.get_data(uuid)) {
                            m_info_manager->focus_in(client_id, context, uuid);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    }

                    else if (cmd == ISM_TRANS_CMD_TURN_ON_LOG) {
                        uint32 isOn;

                        if (m_recv_trans.get_data(isOn)) {
                            m_info_manager->socket_turn_on_log(isOn);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    }

                    else if (cmd == ISM_TRANS_CMD_SHOW_ISF_CONTROL) {
                        m_info_manager->show_isf_panel(client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_HIDE_ISF_CONTROL) {
                        m_info_manager->hide_isf_panel(client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SHOW_ISE_PANEL) {
                        uint32 client;
                        uint32 context;
                        char*   data = NULL;
                        size_t  len;
                        bool ret = false;

                        if (m_recv_trans.get_data(client) && m_recv_trans.get_data(context) && m_recv_trans.get_data(&data, len)) {
                            ret = true;
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(ret ? SCIM_TRANS_CMD_OK : SCIM_TRANS_CMD_FAIL);
                        trans.put_data(ret);
                        trans.write_to_socket(client_socket);

                        if (data != NULL)
                            delete[] data;

                        if (ret)
                            m_info_manager->show_ise_panel(client_id, client, context, data, len);

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_HIDE_ISE_PANEL) {
                        uint32 client;
                        uint32 context;

                        if (m_recv_trans.get_data(client) && m_recv_trans.get_data(context)) {
                            m_info_manager->hide_ise_panel(client_id, client, context);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE_GEOMETRY) {
                        struct rectinfo info;
                        m_info_manager->get_input_panel_geometry(client_id, info);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(info.pos_x);
                        trans.put_data(info.pos_y);
                        trans.put_data(info.width);
                        trans.put_data(info.height);
                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY) {
                        struct rectinfo info;
                        m_info_manager->get_candidate_window_geometry(client_id, info);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(info.pos_x);
                        trans.put_data(info.pos_y);
                        trans.put_data(info.width);
                        trans.put_data(info.height);
                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_LANGUAGE_LOCALE) {
                        size_t  len;
                        char*   data = NULL;
                        m_info_manager->get_ise_language_locale(client_id, &data, len);
                        Transaction trans;
                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (data != NULL && len > 0) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(data, len);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        Socket client_socket(client_id);
                        trans.write_to_socket(client_socket);

                        if (NULL != data)
                            delete [] data;

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_ISE_LANGUAGE) {
                        uint32 language;

                        if (m_recv_trans.get_data(language)) {
                            m_info_manager->set_ise_language(client_id, language);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_ISE_IMDATA) {
                        char*   imdata = NULL;
                        size_t  len;

                        if (m_recv_trans.get_data(&imdata, len)) {
                            m_info_manager->set_ise_imdata(client_id, imdata, len);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        if (NULL != imdata)
                            delete [] imdata;

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_IMDATA) {
                        char*   imdata = NULL;
                        size_t  len = 0;
                        m_info_manager->get_ise_imdata(client_id, &imdata, len);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (len) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(imdata, len);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);

                        if (NULL != imdata)
                            delete [] imdata;

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_RETURN_KEY_TYPE) {
                        uint32 type;

                        if (m_recv_trans.get_data(type)) {
                            m_info_manager->set_ise_return_key_type(client_id, type);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_RETURN_KEY_TYPE) {
                        uint32 type;
                        bool ret = m_info_manager->get_ise_return_key_type(client_id, type);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (ret) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(type);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_RETURN_KEY_DISABLE) {
                        uint32 disabled;

                        if (m_recv_trans.get_data(disabled)) {
                            m_info_manager->set_ise_return_key_disable(client_id, disabled);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_RETURN_KEY_DISABLE) {
                        uint32 disabled;
                        bool ret = true;
                        m_info_manager->get_ise_return_key_disable(client_id, disabled);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (ret) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(disabled);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_LAYOUT) {
                        uint32 layout;
                        bool ret = m_info_manager->get_ise_layout(client_id, layout);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (ret) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(layout);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_LAYOUT) {
                        uint32 layout;

                        if (m_recv_trans.get_data(layout)) {
                            m_info_manager->set_ise_layout(client_id, layout);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_CAPS_MODE) {
                        uint32 mode;

                        if (m_recv_trans.get_data(mode)) {
                            m_info_manager->set_ise_caps_mode(client_id, mode);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_WILL_SHOW_ACK) {
                        m_info_manager->will_show_ack(client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_WILL_HIDE_ACK) {
                        m_info_manager->will_hide_ack(client_id);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_HARDWARE_KEYBOARD_MODE) {
                        uint32 mode;

                        if (m_recv_trans.get_data(mode)) {
                            m_info_manager->set_keyboard_mode(client_id, mode);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SEND_CANDIDATE_WILL_HIDE_ACK) {
                        m_info_manager->candidate_will_hide_ack(client_id);
                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT) {
                        uint32 result = 0;
                        bool   ret      = false;

                        KeyEvent key;

                        if (m_recv_trans.get_data(key)) {
                            ret      = true;
                            m_info_manager->process_key_event(key, result);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (ret) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(result);
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ACTIVE_HELPER_OPTION) {
                        uint32 option;
                        m_info_manager->get_active_helper_option(client_id, option);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(option);
                        SCIM_DEBUG_MAIN(4) << __func__ << " option " << option << "\n";
                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_GET_ISE_STATE) {
                        int state = 0;
                        m_info_manager->get_ise_state(client_id, state);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(state);
                        trans.write_to_socket(client_socket);
                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_INPUT_MODE) {
                        uint32 input_mode;

                        if (m_recv_trans.get_data(input_mode)) {
                            m_info_manager->set_ise_input_mode(client_id, input_mode);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SET_INPUT_HINT) {
                        uint32 input_hint;

                        if (m_recv_trans.get_data(input_hint)) {
                            m_info_manager->set_ise_input_hint(client_id, input_hint);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_BIDI_DIRECTION) {
                        uint32 bidi_direction;

                        if (m_recv_trans.get_data(bidi_direction)) {
                            m_info_manager->update_ise_bidi_direction(client_id, bidi_direction);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW) {
                        m_info_manager->show_ise_option_window(client_id);
                        continue;
                    }

                    //FIXME
                    //skip data

                    else if (cmd == SCIM_TRANS_CMD_START_HELPER) {
                        String uuid;

                        if (m_recv_trans.get_data(uuid) && uuid.length()) {
                            m_info_manager->socket_start_helper(client_id, context, uuid);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_SEND_HELPER_EVENT) {
                        String uuid;

                        if (m_recv_trans.get_data(uuid) && m_recv_trans.get_data(m_nest_trans) &&
                            uuid.length() && m_nest_trans.valid()) {
                            m_info_manager->socket_send_helper_event(client_id, context, uuid, m_nest_trans);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    } else if (cmd == SCIM_TRANS_CMD_STOP_HELPER) {
                        String uuid;

                        if (m_recv_trans.get_data(uuid) && uuid.length()) {
                            m_info_manager->socket_stop_helper(client_id, context, uuid);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        continue;
                    }

                    //FIXME
                    //skip data

                    /* Client must focus in before do any other things. */
                    else if (cmd == SCIM_TRANS_CMD_PANEL_TURN_ON)
                        m_info_manager->socket_turn_on();
                    else if (cmd == SCIM_TRANS_CMD_PANEL_TURN_OFF)
                        m_info_manager->socket_turn_off();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_SCREEN) {
                        uint32 num;

                        if (m_recv_trans.get_data(num))
                            m_info_manager->socket_update_screen(client_id, num);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_SPOT_LOCATION) {
                        uint32 x, y, top_y;

                        if (m_recv_trans.get_data(x) && m_recv_trans.get_data(y) && m_recv_trans.get_data(top_y)) {
                            m_info_manager->socket_update_spot_location(x, y, top_y);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_CURSOR_POSITION) {
                        uint32 cursor_pos;

                        if (m_recv_trans.get_data(cursor_pos)) {
                            m_info_manager->socket_update_cursor_position(cursor_pos);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_SURROUNDING_TEXT) {
                        String text;
                        uint32 cursor;

                        if (m_recv_trans.get_data(text) && m_recv_trans.get_data(cursor)) {
                            m_info_manager->socket_update_surrounding_text(text, cursor);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }
                    } else if (cmd == ISM_TRANS_CMD_UPDATE_SELECTION) {
                        String text;

                        if (m_recv_trans.get_data(text)) {
                            m_info_manager->socket_update_selection(text);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }
                    } else if (cmd == ISM_TRANS_CMD_EXPAND_CANDIDATE)
                        m_info_manager->expand_candidate();
                    else if (cmd == ISM_TRANS_CMD_CONTRACT_CANDIDATE)
                        m_info_manager->contract_candidate();
                    else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_UI) {
                        uint32 portrait_line, mode;

                        if (m_recv_trans.get_data(portrait_line) && m_recv_trans.get_data(mode))
                            m_info_manager->socket_set_candidate_ui(portrait_line, mode);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_PANEL_UPDATE_FACTORY_INFO) {
                        PanelFactoryInfo info;

                        if (m_recv_trans.get_data(info.uuid) && m_recv_trans.get_data(info.name) &&
                            m_recv_trans.get_data(info.lang) && m_recv_trans.get_data(info.icon)) {
                            m_info_manager->socket_update_factory_info(info);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }
                    } else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING)
                        m_info_manager->socket_show_preedit_string();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING)
                        m_info_manager->socket_show_aux_string();
                    else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE)
                        m_info_manager->socket_show_lookup_table();
                    else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE)
                        m_info_manager->socket_show_associate_table();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING)
                        m_info_manager->socket_hide_preedit_string();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING)
                        m_info_manager->socket_hide_aux_string();
                    else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE)
                        m_info_manager->socket_hide_lookup_table();
                    else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE)
                        m_info_manager->socket_hide_associate_table();
                    else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING) {
                        String str;
                        AttributeList attrs;
                        uint32 caret;

                        if (m_recv_trans.get_data(str) && m_recv_trans.get_data(attrs) && m_recv_trans.get_data(caret))
                            m_info_manager->socket_update_preedit_string(str, attrs, caret);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET) {
                        uint32 caret;

                        if (m_recv_trans.get_data(caret))
                            m_info_manager->socket_update_preedit_caret(caret);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING) {
                        String str;
                        AttributeList attrs;

                        if (m_recv_trans.get_data(str) && m_recv_trans.get_data(attrs))
                            m_info_manager->socket_update_aux_string(str, attrs);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE) {
                        CommonLookupTable _isf_candidate_table;

                        if (m_recv_trans.get_data(_isf_candidate_table))
                            m_info_manager->socket_update_lookup_table(_isf_candidate_table);
                        else
                            LOGW ("wrong format of transaction\n");

                    } else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE) {
                        CommonLookupTable table;

                        if (m_recv_trans.get_data(table))
                            m_info_manager->socket_update_associate_table(table);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES) {
                        PropertyList properties;

                        if (m_recv_trans.get_data(properties))
                            m_info_manager->socket_register_properties(properties);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY) {
                        Property property;

                        if (m_recv_trans.get_data(property))
                            m_info_manager->socket_update_property(property);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_HELP) {
                        String help;

                        if (m_recv_trans.get_data(help))
                            m_info_manager->socket_show_help(help);
                        else
                            LOGW ("wrong format of transaction\n");
                    } else if (cmd == SCIM_TRANS_CMD_PANEL_SHOW_FACTORY_MENU) {
                        PanelFactoryInfo info;
                        std::vector <PanelFactoryInfo> vec;

                        while (m_recv_trans.get_data(info.uuid) && m_recv_trans.get_data(info.name) &&
                               m_recv_trans.get_data(info.lang) && m_recv_trans.get_data(info.icon)) {
                            info.lang = scim_get_normalized_language(info.lang);
                            vec.push_back(info);
                        }

                        m_info_manager->socket_show_factory_menu(vec);
                    } else if (cmd == SCIM_TRANS_CMD_FOCUS_OUT) {
                        m_info_manager->focus_out(client_id, context);
                    } else {
                        LOGW ("unknow cmd: %d\n", cmd);
                    }
                }

                socket_transaction_end();
            }
        } else if (client_info.type == FRONTEND_CLIENT) {
            if (m_recv_trans.get_data(context)) {
                SCIM_DEBUG_MAIN(1) << "client_info.type == FRONTEND_CLIENT\n";
                socket_transaction_start();

                while (m_recv_trans.get_command(cmd)) {
                    LOGI ("PanelAgent::cmd = %d\n", cmd);

                    if (cmd == ISM_TRANS_CMD_GET_PANEL_CLIENT_ID) {
                        Socket client_socket(client_id);

                        Transaction trans;
                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(client_id);
                        trans.write_to_socket(client_socket);
                        continue;
                    } else {
                        LOGW ("unknow cmd: %d\n", cmd);
                    }
                }

                socket_transaction_end();
            }
        } else if (client_info.type == HELPER_CLIENT) {
            socket_transaction_start();

            while (m_recv_trans.get_command(cmd)) {
                LOGI ("PanelAgent::cmd = %d\n", cmd);

                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_HELPER) {
                    HelperInfo info;

                    if (m_recv_trans.get_data(info.uuid) &&
                        m_recv_trans.get_data(info.name) &&
                        m_recv_trans.get_data(info.icon) &&
                        m_recv_trans.get_data(info.description) &&
                        m_recv_trans.get_data(info.option) &&
                        info.uuid.length()) {
                        m_info_manager->socket_helper_register_helper(client_id, info);
                    }
                } else {
                    LOGW ("unknow cmd: %d\n", cmd);
                }
            }

            socket_transaction_end();
        } else if (client_info.type == HELPER_ACT_CLIENT) {
            socket_transaction_start();

            while (m_recv_trans.get_command(cmd)) {
                LOGI ("PanelAgent::cmd = %d\n", cmd);

                if (cmd == SCIM_TRANS_CMD_PANEL_REGISTER_ACTIVE_HELPER) {
                    HelperInfo info;

                    if (m_recv_trans.get_data(info.uuid) &&
                        m_recv_trans.get_data(info.name) &&
                        m_recv_trans.get_data(info.icon) &&
                        m_recv_trans.get_data(info.description) &&
                        m_recv_trans.get_data(info.option) &&
                        info.uuid.length()) {
                        m_info_manager->socket_helper_register_helper_passive(client_id, info);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_COMMIT_STRING) {
                    uint32 target_ic;
                    String target_uuid;
                    WideString wstr;

                    if (m_recv_trans.get_data(target_ic)    &&
                        m_recv_trans.get_data(target_uuid)  &&
                        m_recv_trans.get_data(wstr)         &&
                        wstr.length()) {
                        m_info_manager->socket_helper_commit_string(client_id, target_ic, target_uuid, wstr);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_SHOW_PREEDIT_STRING) {
                    uint32 target_ic;
                    String target_uuid;

                    if (m_recv_trans.get_data(target_ic) && m_recv_trans.get_data(target_uuid)) {
                        m_info_manager->socket_helper_show_preedit_string(client_id, target_ic, target_uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_SHOW_AUX_STRING) {
                    m_info_manager->socket_show_aux_string();
                } else if (cmd == SCIM_TRANS_CMD_SHOW_LOOKUP_TABLE) {
                    m_info_manager->socket_show_lookup_table();
                } else if (cmd == ISM_TRANS_CMD_SHOW_ASSOCIATE_TABLE) {
                    m_info_manager->socket_show_associate_table();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_PREEDIT_STRING) {
                    uint32 target_ic;
                    String target_uuid;

                    if (m_recv_trans.get_data(target_ic) && m_recv_trans.get_data(target_uuid)) {
                        m_info_manager->socket_helper_hide_preedit_string(client_id, target_ic, target_uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_HIDE_AUX_STRING) {
                    m_info_manager->socket_hide_aux_string();
                } else if (cmd == SCIM_TRANS_CMD_HIDE_LOOKUP_TABLE) {
                    m_info_manager->socket_hide_lookup_table();
                } else if (cmd == ISM_TRANS_CMD_HIDE_ASSOCIATE_TABLE) {
                    m_info_manager->socket_hide_associate_table();
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_STRING) {
                    uint32 target_ic;
                    String target_uuid;
                    WideString wstr;
                    AttributeList attrs;
                    uint32 caret;

                    if (m_recv_trans.get_data(target_ic)    &&
                        m_recv_trans.get_data(target_uuid)  &&
                        m_recv_trans.get_data(wstr) &&
                        m_recv_trans.get_data(attrs) &&
                        m_recv_trans.get_data(caret)) {
                        m_info_manager->socket_helper_update_preedit_string(client_id, target_ic, target_uuid, wstr, attrs, caret);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PREEDIT_CARET) {
                    uint32 caret;

                    if (m_recv_trans.get_data(caret)) {
                        m_info_manager->socket_helper_update_preedit_caret(client_id, caret);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_AUX_STRING) {
                    String str;
                    AttributeList attrs;

                    if (m_recv_trans.get_data(str) && m_recv_trans.get_data(attrs))
                        m_info_manager->socket_update_aux_string(str, attrs);
                    else
                        LOGW ("wrong format of transaction\n");

                } else if (cmd == SCIM_TRANS_CMD_UPDATE_LOOKUP_TABLE) {
                    CommonLookupTable _isf_candidate_table;

                    if (m_recv_trans.get_data(_isf_candidate_table)) {
                        m_info_manager->socket_update_lookup_table(_isf_candidate_table);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ASSOCIATE_TABLE) {
                    CommonLookupTable _isf_candidate_table;

                    if (m_recv_trans.get_data(_isf_candidate_table)) {
                        m_info_manager->socket_update_associate_table(_isf_candidate_table);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_PROCESS_KEY_EVENT ||
                           cmd == SCIM_TRANS_CMD_PANEL_SEND_KEY_EVENT) {
                    uint32 target_ic;
                    String target_uuid;
                    KeyEvent key;

                    if (m_recv_trans.get_data(target_ic)    &&
                        m_recv_trans.get_data(target_uuid)  &&
                        m_recv_trans.get_data(key)          &&
                        !key.empty()) {
                        m_info_manager->socket_helper_send_key_event(client_id, target_ic, target_uuid, key);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_FORWARD_KEY_EVENT) {
                    uint32 target_ic;
                    String target_uuid;
                    KeyEvent key;

                    if (m_recv_trans.get_data(target_ic)    &&
                        m_recv_trans.get_data(target_uuid)  &&
                        m_recv_trans.get_data(key)          &&
                        !key.empty()) {
                        m_info_manager->socket_helper_forward_key_event(client_id, target_ic, target_uuid, key);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_PANEL_SEND_IMENGINE_EVENT) {
                    uint32 target_ic;
                    String target_uuid;

                    if (m_recv_trans.get_data(target_ic)    &&
                        m_recv_trans.get_data(target_uuid)  &&
                        m_recv_trans.get_data(m_nest_trans) &&
                        m_nest_trans.valid()) {
                        m_info_manager->socket_helper_send_imengine_event(client_id, target_ic, target_uuid, m_nest_trans);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_REGISTER_PROPERTIES) {
                    PropertyList properties;

                    if (m_recv_trans.get_data(properties))
                        m_info_manager->socket_helper_register_properties(client_id, properties);
                    else
                        LOGW ("wrong format of transaction\n");
                } else if (cmd == SCIM_TRANS_CMD_UPDATE_PROPERTY) {
                    Property property;

                    if (m_recv_trans.get_data(property))
                        m_info_manager->socket_helper_update_property(client_id, property);
                    else
                        LOGW ("wrong format of transaction\n");
#if 0 //only receives reload message by socketconfig
                } else if (cmd == SCIM_TRANS_CMD_RELOAD_CONFIG) {
                    m_info_manager->reload_config();
#endif
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_INPUT_CONTEXT) {
                    uint32 type;
                    uint32 value;

                    if (m_recv_trans.get_data(type) && m_recv_trans.get_data(value)) {
                        m_info_manager->socket_helper_update_input_context(client_id, type, value);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE_LIST) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        m_info_manager->socket_get_keyboard_ise_list(uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_UI) {
                    uint32 portrait_line, mode;

                    if (m_recv_trans.get_data(portrait_line) && m_recv_trans.get_data(mode))
                        m_info_manager->socket_set_candidate_ui(portrait_line, mode);
                    else
                        LOGW ("wrong format of transaction\n");
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_UI) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        m_info_manager->socket_get_candidate_ui(uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_SET_CANDIDATE_POSITION) {
                    uint32 left, top;

                    if (m_recv_trans.get_data(left) && m_recv_trans.get_data(top))
                        m_info_manager->socket_set_candidate_position(left, top);
                    else
                        LOGW ("wrong format of transaction\n");
                } else if (cmd == ISM_TRANS_CMD_HIDE_CANDIDATE) {
                    m_info_manager->socket_hide_candidate();
                } else if (cmd == ISM_TRANS_CMD_GET_CANDIDATE_GEOMETRY) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        m_info_manager->socket_get_candidate_geometry(uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_PANEL_RESET_KEYBOARD_ISE) {
                    m_info_manager->reset_keyboard_ise();
                } else if (cmd == ISM_TRANS_CMD_SET_KEYBOARD_ISE_BY_UUID) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        m_info_manager->socket_set_keyboard_ise(uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_KEYBOARD_ISE) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        m_info_manager->socket_get_keyboard_ise(uuid);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_GEOMETRY) {
                    uint32 x, y, width, height;

                    if (m_recv_trans.get_data(x) && m_recv_trans.get_data(y) &&
                        m_recv_trans.get_data(width) && m_recv_trans.get_data(height)) {
                        m_info_manager->socket_helper_update_ise_geometry(client_id, x, y, width, height);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == ISM_TRANS_CMD_EXPAND_CANDIDATE) {
                    m_info_manager->expand_candidate();
                } else if (cmd == ISM_TRANS_CMD_CONTRACT_CANDIDATE) {
                    m_info_manager->contract_candidate();
                } else if (cmd == ISM_TRANS_CMD_SELECT_CANDIDATE) {
                    uint32 index;

                    if (m_recv_trans.get_data(index))
                        m_info_manager->socket_helper_select_candidate(index);
                    else
                        LOGW ("wrong format of transaction\n");
                } else if (cmd == SCIM_TRANS_CMD_GET_SURROUNDING_TEXT) {
                    String uuid;
                    uint32 maxlen_before;
                    uint32 maxlen_after;

                    if (m_recv_trans.get_data(uuid) &&
                        m_recv_trans.get_data(maxlen_before) &&
                        m_recv_trans.get_data(maxlen_after)) {
                        int fd;
                        client.read_fd (&fd);
                        if (fd == -1) {
                            LOGW ("wrong format of transaction\n");
                        } else {
                            m_info_manager->socket_helper_get_surrounding_text(client_id, uuid, maxlen_before, maxlen_after, fd);
                            close (fd);
                        }
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_DELETE_SURROUNDING_TEXT) {
                    uint32 offset;
                    uint32 len;

                    if (m_recv_trans.get_data(offset) && m_recv_trans.get_data(len)) {
                        m_info_manager->socket_helper_delete_surrounding_text(client_id, offset, len);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_GET_SELECTION) {
                    String uuid;

                    if (m_recv_trans.get_data(uuid)) {
                        int fd;
                        client.read_fd (&fd);
                        if (fd == -1) {
                            LOGW ("wrong format of transaction\n");
                        } else {
                            m_info_manager->socket_helper_get_selection(client_id, uuid, fd);
                            close (fd);
                        }
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_SET_SELECTION) {
                    uint32 start;
                    uint32 end;

                    if (m_recv_trans.get_data(start) && m_recv_trans.get_data(end)) {
                        m_info_manager->socket_helper_set_selection(client_id, start, end);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                } else if (cmd == SCIM_TRANS_CMD_SEND_PRIVATE_COMMAND) {
                    String command;

                    if (m_recv_trans.get_data(command)) {
                        m_info_manager->socket_helper_send_private_command(client_id, command);
                    } else {
                        LOGW ("wrong format of transaction\n");
                    }
                //FIXME: useless
                //} else if (cmd == ISM_TRANS_CMD_UPDATE_ISE_EXIT) {
                //    m_info_manager->UPDATE_ISE_EXIT(client_id);
                } else {
                    LOGW ("unknow cmd: %d\n", cmd);
                }
            }

            socket_transaction_end();
        } else if (client_info.type == IMCONTROL_ACT_CLIENT) {
            socket_transaction_start();

            while (m_recv_trans.get_command(cmd)) {
                LOGI ("PanelAgent::cmd = %d\n", cmd);

                if (cmd == ISM_TRANS_CMD_GET_ACTIVE_ISE) {

                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        String default_uuid;
                        m_info_manager->get_active_ise(client_id, default_uuid);
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(default_uuid);
                        trans.write_to_socket(client_socket);
                    }
                    else {
                        LOGW ("Access denied to get active ise\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SET_ACTIVE_ISE_BY_UUID) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        char*   buf = NULL;
                        size_t  len;
                        bool ret = false;

                        if (m_recv_trans.get_data(&buf, len)) {
                            ret = m_info_manager->set_active_ise_by_uuid(client_id, buf, len);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(ret ? SCIM_TRANS_CMD_OK : SCIM_TRANS_CMD_FAIL);
                        trans.write_to_socket(client_socket);

                        if (NULL != buf)
                            delete[] buf;
                    }
                    else {
                        LOGW ("Access denied to set active ise\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SET_INITIAL_ISE_BY_UUID) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        char*   buf = NULL;
                        size_t  len;
                        bool ret = true;

                        //FIXME
                        //ret need be checked
                        if (m_recv_trans.get_data(&buf, len)) {
                            m_info_manager->set_initial_ise_by_uuid(client_id, buf, len);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(ret ? SCIM_TRANS_CMD_OK : SCIM_TRANS_CMD_FAIL);
                        trans.write_to_socket(client_socket);

                        if (NULL != buf)
                            delete[] buf;
                    }
                    else {
                        LOGW ("Access denied to set initial ise\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_ISE_LIST) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        std::vector<String> strlist;
                        m_info_manager->get_ise_list(client_id, strlist);
                        Transaction trans;
                        Socket client_socket(client_id);
                        char* buf = NULL;
                        size_t len;
                        uint32 num;

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);

                        num = strlist.size();
                        trans.put_data(num);

                        for (unsigned int i = 0; i < num; i++) {
                            buf = const_cast<char*>(strlist[i].c_str());
                            len = strlen(buf) + 1;
                            trans.put_data(buf, len);
                        }

                        trans.write_to_socket(client_socket);
                    }
                    else {
                        LOGW ("Access denied to get ise list\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_ALL_HELPER_ISE_INFO) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        HELPER_ISE_INFO info;
                        m_info_manager->get_all_helper_ise_info(client_id, info);

                        do {
                            Transaction trans;
                            Socket client_socket(client_id);

                            trans.clear();
                            trans.put_command(SCIM_TRANS_CMD_REPLY);
                            trans.put_command(SCIM_TRANS_CMD_OK);

                            if (info.appid.size() > 0) {
                                trans.put_data(info.appid);
                                trans.put_data(info.label);
                                trans.put_data(info.is_enabled);
                                trans.put_data(info.is_preinstalled);
                                trans.put_data(info.has_option);
                            }

                            trans.write_to_socket(client_socket);
                        } while (0);
                    }
                    else {
                        LOGW ("Access denied to get all helper ise info\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SET_ENABLE_HELPER_ISE_INFO) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        String appid;
                        uint32 is_enabled;
                        bool ret = false;

                        if (m_recv_trans.get_data(appid) && appid.length() != 0 && m_recv_trans.get_data(is_enabled)) {
                            m_info_manager->set_enable_helper_ise_info(client_id, appid, is_enabled);
                            ret = true;
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(ret ? SCIM_TRANS_CMD_OK : SCIM_TRANS_CMD_FAIL);
                        trans.write_to_socket(client_socket);
                    }
                    else {
                        LOGW ("Access denied to set enable helper ise info\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_ISE_INFORMATION) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        String strUuid, strName, strLanguage, strModuleName;
                        int nType   = 0;
                        int nOption = 0;

                        if (m_recv_trans.get_data(strUuid)) {
                            m_info_manager->get_ise_information(client_id, strUuid, strName, strLanguage, nType, nOption, strModuleName);
                        } else {
                            LOGW ("wrong format of transaction\n");
                        }

                        Transaction trans;
                        Socket client_socket(client_id);
                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(strName);
                        trans.put_data(strLanguage);
                        trans.put_data(nType);
                        trans.put_data(nOption);
                        trans.put_data(strModuleName);
                        trans.write_to_socket(client_socket);
                    }
                    else {
                        LOGW ("Access denied to get ise information\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_RESET_ISE_OPTION) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.write_to_socket(client_socket);
                        m_info_manager->show_helper_ise_selector(client_id);
                        m_info_manager->reset_ise_option(client_id);
                    }
                    else {
                        LOGW ("Access denied to reset ise option\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_RESET_DEFAULT_ISE) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        m_info_manager->reset_default_ise(client_id);
                    }
                    else {
                        LOGW ("Access denied to reset default ise\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SHOW_ISF_CONTROL) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        m_info_manager->show_isf_panel(client_id);
                    }
                    else {
                        LOGW ("Access denied to show isf control\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SHOW_ISE_OPTION_WINDOW) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        m_info_manager->show_ise_option_window(client_id);
                    }
                    else {
                        LOGW ("Access denied to show ise option window\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SHOW_HELPER_ISE_LIST) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.write_to_socket(client_socket);

                        m_info_manager->show_helper_ise_list(client_id);
                    }
                    else {
                        LOGW ("Access denied to show helper ise list\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_SHOW_HELPER_ISE_SELECTOR) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        Transaction trans;
                        Socket client_socket(client_id);

                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);
                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.write_to_socket(client_socket);
                        m_info_manager->show_helper_ise_selector(client_id);
                    }
                    else {
                        LOGW ("Access denied to show helper ise selector\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_IS_HELPER_ISE_ENABLED) {
                    if (m_info_manager->check_privilege_by_sockfd(client_id, IMEMANAGER_PRIVILEGE)) {
                        String strAppid;
                        uint32 nEnabled = 0;
                        bool ret = true;
                        //FIXME
                        //ret need be checked
                        m_info_manager->is_helper_ise_enabled(client_id, strAppid, nEnabled);
                        Transaction trans;
                        Socket client_socket(client_id);
                        trans.clear();
                        trans.put_command(SCIM_TRANS_CMD_REPLY);

                        if (ret) {
                            trans.put_command(SCIM_TRANS_CMD_OK);
                            trans.put_data(static_cast<uint32>(nEnabled));
                        } else {
                            trans.put_command(SCIM_TRANS_CMD_FAIL);
                        }

                        trans.write_to_socket(client_socket);
                    }
                    else {
                        LOGW ("Access denied to check helper ise enabled\n");
                        send_fail_reply (client_id);
                    }
                } else if (cmd == ISM_TRANS_CMD_GET_RECENT_ISE_GEOMETRY) {
                    uint32 angle;

                    Transaction trans;
                    Socket client_socket(client_id);

                    trans.clear();
                    trans.put_command(SCIM_TRANS_CMD_REPLY);

                    if (m_recv_trans.get_data(angle)) {
                        struct rectinfo info = {0, 0, 0, 0};
                        m_info_manager->get_recent_ise_geometry(client_id, angle, info);

                        trans.put_command(SCIM_TRANS_CMD_OK);
                        trans.put_data(info.pos_x);
                        trans.put_data(info.pos_y);
                        trans.put_data(info.width);
                        trans.put_data(info.height);
                    } else {
                        trans.put_command(SCIM_TRANS_CMD_FAIL);
                    }

                    trans.write_to_socket(client_socket);
                } else if (cmd == ISM_TRANS_CMD_HIDE_ISE_PANEL) {
                    Transaction trans;
                    Socket client_socket(client_id);

                    trans.clear();
                    trans.put_command(SCIM_TRANS_CMD_REPLY);
                    trans.put_command(SCIM_TRANS_CMD_OK);
                    trans.write_to_socket(client_socket);
                    m_info_manager->hide_helper_ise ();
                } else {
                    LOGW ("unknow cmd: %d\n", cmd);
                }
            }

            socket_transaction_end();
        } else if (client_info.type == CONFIG_CLIENT) {
            socket_transaction_start ();
            while (m_recv_trans.get_command (cmd)) {
                LOGI ("PanelAgent::cmd = %d\n", cmd);

                if (cmd == SCIM_TRANS_CMD_FLUSH_CONFIG) {
                    if (m_config_readonly) {
                        LOGW ("sorry config readonly");
                        continue;
                    }
                    if (_config.null ()) {
                        LOGW ("config is not ready");
                        continue;
                    }
                    SCIM_DEBUG_FRONTEND (2) << " socket_flush_config.\n";

                    _config->flush ();

                } else if (cmd == SCIM_TRANS_CMD_ERASE_CONFIG) {

                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_erase_config.\n";

                    if (m_recv_trans.get_data (key)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        _config->erase (key);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }
                } else if (cmd == SCIM_TRANS_CMD_RELOAD_CONFIG) {

                    SCIM_DEBUG_FRONTEND (2) << " socket_reload_config.\n";
                    Socket client_socket (client_id);
                    m_send_trans.clear ();
                    m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                    if (_config.null ()) {
                        LOGW ("config is not ready");
                        m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                    } else {
                        static timeval last_timestamp = {0, 0};

                        timeval timestamp;

                        gettimeofday (&timestamp, 0);

                        if (timestamp.tv_sec >= last_timestamp.tv_sec)
                            _config->reload ();

                        gettimeofday (&last_timestamp, 0);
                        m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                    }

                    m_send_trans.write_to_socket (client_socket);

                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_STRING) {
                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_string.\n";

                    if (m_recv_trans.get_data (key)) {
                        String value;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                        if (!_config.null () && _config->read (key, &value)) {
                            m_send_trans.put_data (value);
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }
                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_STRING) {
                    String key;
                    String value;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_string.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (value)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
                        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        _config->write (key, value);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }
                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_INT) {
                    if (_config.null ()) {
                        LOGW ("config not ready");
                        break;
                    }

                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_int.\n";

                    if (m_recv_trans.get_data (key)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

                        int value;
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);
                        if (_config->read (key, &value)) {
                            m_send_trans.put_data ((uint32) value);
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_INT) {

                    String key;
                    uint32 value;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_int.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (value)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
                        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        _config->write (key, (int) value);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_BOOL) {

                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_bool.\n";

                    if (m_recv_trans.get_data (key)) {
                        bool value;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                        if (!_config.null () && _config->read (key, &value)) {
                            m_send_trans.put_data ((uint32) value);
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_BOOL) {

                    String key;
                    uint32 value;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_bool.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (value)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
                        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        _config->write (key, (bool) value);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_DOUBLE) {
                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_double.\n";

                    if (m_recv_trans.get_data (key)) {
                        double value;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                        if (!_config.null () && _config->read (key, &value)) {
                            char buf [80];
                            snprintf (buf, 79, "%lE", value);
                            m_send_trans.put_data (String (buf));
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_DOUBLE) {

                    String key;
                    String str;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_double.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (str)) {
                        double value;
                        sscanf (str.c_str (), "%lE", &value);

                        SCIM_DEBUG_FRONTEND (3) << "  Key   (" << key << ").\n";
                        SCIM_DEBUG_FRONTEND (3) << "  Value (" << value << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        _config->write (key, value);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_VECTOR_STRING) {

                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_vector_string.\n";

                    if (m_recv_trans.get_data (key)) {
                        std::vector <String> vec;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                        if (!_config.null () && _config->read (key, &vec)) {
                            m_send_trans.put_data (vec);
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_VECTOR_STRING) {

                    String key;
                    std::vector<String> vec;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_vector_string.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (vec)) {

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        _config->write (key, vec);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_GET_CONFIG_VECTOR_INT) {

                    String key;

                    SCIM_DEBUG_FRONTEND (2) << " socket_get_config_vector_int.\n";

                    if (m_recv_trans.get_data (key)) {
                        std::vector <int> vec;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";
                        Socket client_socket (client_id);
                        m_send_trans.clear ();
                        m_send_trans.put_command (SCIM_TRANS_CMD_REPLY);

                        if (!_config.null () && _config->read (key, &vec)) {
                            std::vector <uint32> reply;

                            for (uint32 i=0; i<vec.size (); ++i)
                                reply.push_back ((uint32) vec[i]);

                            m_send_trans.put_data (reply);
                            m_send_trans.put_command (SCIM_TRANS_CMD_OK);
                        } else {
                            m_send_trans.put_command (SCIM_TRANS_CMD_FAIL);
                            LOGW ("read config key = %s faided\n", key.c_str());
                        }
                        m_send_trans.write_to_socket (client_socket);

                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                } else if (cmd == SCIM_TRANS_CMD_SET_CONFIG_VECTOR_INT) {

                    String key;
                    std::vector<uint32> vec;

                    SCIM_DEBUG_FRONTEND (2) << " socket_set_config_vector_int.\n";

                    if (m_recv_trans.get_data (key) &&
                        m_recv_trans.get_data (vec)) {
                        if (m_config_readonly) {
                            LOGW ("sorry config readonly");
                            continue;
                        }
                        if (_config.null ()) {
                            LOGW ("config is not ready");
                            continue;
                        }
                        std::vector<int> req;

                        SCIM_DEBUG_FRONTEND (3) << "  Key (" << key << ").\n";

                        for (uint32 i=0; i<vec.size (); ++i)
                            req.push_back ((int) vec[i]);

                        _config->write (key, req);
                    } else {
                        LOGW ("wrong format of transaction\n");
                        break;
                    }

                }
            }

            socket_transaction_end ();
        }
    }

    void socket_exception_callback(SocketServer*   server,
                                   const Socket&   client) {
        SCIM_DEBUG_MAIN(2) << "PanelAgent::socket_exception_callback (" << client.get_id() << ")\n";
        LOGI ("client id:%d\n", client.get_id());
        socket_close_connection(server, client);
    }

    bool socket_open_connection(SocketServer*   server,
                                const Socket&   client) {
        SCIM_DEBUG_MAIN(3) << "PanelAgent::socket_open_connection (" << client.get_id() << ")\n";
        LOGI ("client id:%d\n", client.get_id());
        uint32 key;
        String type = scim_socket_accept_connection(key,
                      String("Panel"),
                      String("FrontEnd,FrontEnd_Active,Helper,Helper_Active,IMControl_Active,IMControl_Passive,SocketConfig"),
                      client,
                      m_socket_timeout);

        SCIM_DEBUG_MAIN (3) << "type = " << type << "\n";
        if (type.length()) {
            ClientType _type = ((type == "FrontEnd") ? FRONTEND_CLIENT :
                               ((type == "FrontEnd_Active") ? FRONTEND_ACT_CLIENT :
                               ((type == "Helper") ? HELPER_CLIENT :
                               ((type == "Helper_Active") ? HELPER_ACT_CLIENT :
                               ((type == "IMControl_Active") ? IMCONTROL_ACT_CLIENT :
                               ((type == "IMControl_Passive") ? IMCONTROL_CLIENT : CONFIG_CLIENT))))));
            lock();
            m_info_manager->add_client(client.get_id(), key, _type);
            unlock();
            return true;
        }
        LOGW ("open_connection failed\n");

        SCIM_DEBUG_MAIN(4) << "Close client connection " << client.get_id() << "\n";
        server->close_connection(client);
        return false;
    }

    void socket_close_connection(SocketServer*   server,
                                 const Socket&   client) {
        SCIM_DEBUG_MAIN(3) << "PanelAgent::socket_close_connection (" << client.get_id() << ")\n";
        LOGI ("client id:%d\n", client.get_id());
        int i = 0;
        std::vector<Ecore_Fd_Handler *>::iterator IterPos;

        for (IterPos = _read_handler_list.begin (); IterPos != _read_handler_list.end (); ++IterPos,++i) {
            if (ecore_main_fd_handler_fd_get (_read_handler_list[i]) == client.get_id()) {
                ecore_main_fd_handler_del (_read_handler_list[i]);
                _read_handler_list.erase (IterPos);
                break;
            }
        }
        m_info_manager->del_client(client.get_id());
    }

    void socket_transaction_start(void) {
        //m_signal_transaction_start ();
    }

    void socket_transaction_end(void) {
        //m_signal_transaction_end ();
    }

    void lock(void) {
        //m_signal_lock ();
    }
    void unlock(void) {
        //m_signal_unlock ();
    }
};

} /* namespace scim */

/***************************************************/
/*** Beginning of panel agent interface for ISF ***/
/***************************************************/
static scim::PanelAgentPointer instance;

extern "C" {

    EXAPI void scim_module_init(void)
    {
    }

    EXAPI void scim_module_exit(void)
    {
        instance.reset();
    }

    EXAPI void scim_panel_agent_module_init(const scim::ConfigPointer& config)
    {
        LOGI ("");
        scim::_config = config;
    }

    EXAPI scim::PanelAgentPointer scim_panel_agent_module_get_instance()
    {
        scim::PanelAgentBase* _instance = NULL;
        if (instance.null()) {
            try {
                _instance = new scim::EcoreSocketPanelAgent();
            } catch (...) {
                delete _instance;
                _instance = NULL;
            }
            if(_instance)
                instance = _instance;
        }
        return instance;
    }
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
