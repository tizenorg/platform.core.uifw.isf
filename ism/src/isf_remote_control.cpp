#define Uses_SCIM_TRANSACTION
#define Uses_ISF_REMOTE_CLIENT
#define Uses_STL_STRING

#include <Ecore.h>
#include <dlog.h>

#include "scim.h"
#include "isf_remote_control.h"

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG             "ISF_REMOTE_CONTROL"

using namespace scim;

static bool focus_flag, event_check_flag;

enum remote_control_command_option{
    REMOTE_CONTROL_COMMAND_KEY = 0,
    REMOTE_CONTROL_COMMAND_COMMIT,
    REMOTE_CONTROL_COMMAND_PREEDIT,
};

struct _remote_control_client {
    RemoteInputClient remote_client;
    Ecore_Fd_Handler *remote_fd_handler = NULL;
    int remote_client_id;
    remote_control_focus_in_cb focus_in_cb;
    void* focus_in_cb_user_data;
    remote_control_focus_out_cb focus_out_cb;
    void* focus_out_cb_user_data;
    remote_control_entry_metadata_cb metadata_cb;
    void* metadata_cb_user_data;
    remote_control_default_text_cb default_text_cb;
    void* default_text_cb_user_data;
};

static Eina_Bool
remote_handler(void *data, Ecore_Fd_Handler *fd_handler)
{
    remote_control_client *client = static_cast<remote_control_client*>(data);

    if (client->remote_client.has_pending_event()) {
        switch (client->remote_client.recv_callback_message()) {
            case REMOTE_CONTROL_CALLBACK_FOCUS_IN:
            {
                focus_flag = true;
                event_check_flag = false;
                client->focus_in_cb (client->focus_in_cb_user_data);
                break;
            }
            case REMOTE_CONTROL_CALLBACK_FOCUS_OUT:
            {
                focus_flag = false;
                client->focus_out_cb (client->focus_out_cb_user_data);
                break;
            }
            case REMOTE_CONTROL_CALLBACK_ENTRY_METADATA:
            {
                if (focus_flag) {
                    remote_control_entry_metadata_s *data = new remote_control_entry_metadata_s;
                    int hint, layout, variation, autocapital_type;

                    client->remote_client.get_entry_metadata (&hint, &layout, &variation, &autocapital_type);
                    data->hint = static_cast<Ecore_IMF_Input_Hints> (hint);
                    data->layout = static_cast<Ecore_IMF_Input_Panel_Layout> (layout);
                    data->variation = variation;
                    data->autocapital_type = static_cast<Ecore_IMF_Autocapital_Type> (autocapital_type);

                    client->metadata_cb (client->metadata_cb_user_data, data);
                    delete data;
                }
                break;
            }
            case REMOTE_CONTROL_CALLBACK_DEFAULT_TEXT:
            {
                if (focus_flag && !event_check_flag) {
                    String default_text;
                    int cursor;

                    client->remote_client.get_default_text (default_text, &cursor);
                    remote_control_update_preedit_string(client, default_text.c_str ()), NULL, 0);
                    client->default_text_cb (client->default_text_cb_user_data, strdup (default_text.c_str ()), cursor);
                }
            }
            case REMOTE_CONTROL_CALLBACK_ERROR:
                break;
            default:
                break;
        }
    }
    return ECORE_CALLBACK_RENEW;
}

EXAPI remote_control_client * remote_control_connect(void)
{
    remote_control_client *client = new remote_control_client;
    focus_flag = false;
    event_check_flag = false;

    if (client) {
        RemoteInputClient *remote_client = new RemoteInputClient;
        client->remote_client = *remote_client;
        if (!client->remote_client.open_connection())
            goto cleanup;

        client->remote_client_id = client->remote_client.get_panel2remote_connection_number();

        if (client->remote_client_id >= 0) {
            client->remote_fd_handler = ecore_main_fd_handler_add(client->remote_client_id, ECORE_FD_READ, remote_handler, client, NULL, NULL);

            if (client->remote_fd_handler == NULL)
                goto cleanup;
        } else
            goto cleanup;

    } else
        goto cleanup;

    return client;
cleanup:
    if (client)
        delete client;

    return NULL;
}

EXAPI int remote_control_disconnect(remote_control_client **client)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    if ((*client)->remote_fd_handler)
        ecore_main_fd_handler_del((*client)->remote_fd_handler);

    (*client)->remote_client.close_connection();
    delete *client;
    *client = NULL;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_focus_in_callback_set(remote_control_client *client, remote_control_focus_in_cb func, void *user_data)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->focus_in_cb = func;
    client->focus_in_cb_user_data = user_data;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_focus_in_callback_unset(remote_control_client *client)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->focus_in_cb = NULL;
    client->focus_in_cb_user_data = NULL;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_focus_out_callback_set(remote_control_client *client, remote_control_focus_out_cb func , void *user_data)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->focus_out_cb = func;
    client->focus_out_cb_user_data = user_data;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_focus_out_callback_unset(remote_control_client *client)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->focus_out_cb = NULL;
    client->focus_out_cb_user_data = NULL;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_entry_metadata_callback_set(remote_control_client *client, remote_control_entry_metadata_cb func, void *user_data)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->metadata_cb = func;
    client->metadata_cb_user_data = user_data;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_entry_metadata_callback_unset(remote_control_client *client)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->metadata_cb = NULL;
    client->metadata_cb_user_data = NULL;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_default_text_callback_set(remote_control_client *client, remote_control_default_text_cb func, void *user_data)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->default_text_cb = func;
    client->default_text_cb_user_data = user_data;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_default_text_callback_unset(remote_control_client *client)
{
    if (client == NULL)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    client->default_text_cb = NULL;
    client->default_text_cb_user_data = NULL;

    return REMOTE_CONTROL_ERROR_NONE;
}

EXAPI int remote_control_send_key_event(remote_control_client *client, remote_control_key_type_e key)
{
    if (client == NULL || key < REMOTE_CONTROL_KEY_ENTER || key > REMOTE_CONTROL_KEY_PAGE_DOWN)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    char key_str[10] = {};
    snprintf(key_str, sizeof(key_str), "%d", key);
    String command = String ("|plain|send_key_event|") + String (key_str) + String ("|");

    client->remote_client.prepare();
    if (focus_flag && client->remote_client.send_remote_input_message(command.c_str ())) {
        event_check_flag = true;
        return REMOTE_CONTROL_ERROR_NONE;
    }
    return REMOTE_CONTROL_INVALID_OPERATION;
}

EXAPI int remote_control_send_commit_string(remote_control_client *client, const char *text)
{
    if (client == NULL || !text || strlen(text) == 0)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    String command = String ("|plain|commit_string|") + String (text) + String ("|");

    client->remote_client.prepare();
    if (focus_flag && client->remote_client.send_remote_input_message(command.c_str ())) {
        event_check_flag = true;
        return REMOTE_CONTROL_ERROR_NONE;
    }
    return REMOTE_CONTROL_INVALID_OPERATION;
}

EXAPI int remote_control_update_preedit_string(remote_control_client *client, const char *text, Eina_List *attrs, int cursor_pos)
{
    if (client == NULL || !text || strlen(text) == 0 || cursor_pos < 0)
        return REMOTE_CONTROL_INVALID_PARAMETER;

    String command = String ("|plain|update_preedit_string|") + String (text) + String ("|");

    client->remote_client.prepare();
    if (focus_flag && client->remote_client.send_remote_input_message(command.c_str ())) {
        event_check_flag = true;
        return REMOTE_CONTROL_ERROR_NONE;
    }
    return REMOTE_CONTROL_INVALID_OPERATION;
}