#ifndef _ISF_REMOTE_CONTROL_H_
#define _ISF_REMOTE_CONTROL_H_

#include <Ecore_IMF.h>
#include <scim_visibility.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    REMOTE_CONTROL_ERROR_NONE = 0,
    REMOTE_CONTROL_INVALID_PARAMETER,
    REMOTE_CONTROL_INVALID_OPERATION,
} remote_control_error_e;

typedef enum {
    REMOTE_CONTROL_KEY_ENTER = 0,
    REMOTE_CONTROL_KEY_SPACE,
    REMOTE_CONTROL_KEY_BACKSPACE,
    REMOTE_CONTROL_KEY_ESC,
    REMOTE_CONTROL_KEY_UP,
    REMOTE_CONTROL_KEY_DOWN,
    REMOTE_CONTROL_KEY_LEFT,
    REMOTE_CONTROL_KEY_RIGHT,
    REMOTE_CONTROL_KEY_PAGE_UP,
    REMOTE_CONTROL_KEY_PAGE_DOWN,
} remote_control_key_type_e;

typedef struct {
    Ecore_IMF_Input_Hints hint;
    Ecore_IMF_Input_Panel_Layout layout;
    int variation;
    Ecore_IMF_Autocapital_Type autocapital_type;
} remote_control_entry_metadata_s;

typedef struct _remote_control_client remote_control_client;

EXAPI remote_control_client * remote_control_connect(void);

EXAPI int remote_control_disconnect(remote_control_client **client);

typedef void (*remote_control_focus_in_cb)(void *user_data);

EXAPI int remote_control_focus_in_callback_set(remote_control_client *client, remote_control_focus_in_cb func, void *user_data);

EXAPI int remote_control_focus_in_callback_unset(remote_control_client *client);

typedef void (*remote_control_focus_out_cb)(void *user_data);

EXAPI int remote_control_focus_out_callback_set(remote_control_client *client, remote_control_focus_out_cb func , void *user_data);

EXAPI int remote_control_focus_out_callback_unset(remote_control_client *client);

typedef void (*remote_control_entry_metadata_cb)(void *user_data, remote_control_entry_metadata_s *data);

EXAPI int remote_control_entry_metadata_callback_set(remote_control_client *client, remote_control_entry_metadata_cb func, void *user_data);

EXAPI int remote_control_entry_metadata_callback_unset(remote_control_client *client);

typedef void (*remote_control_default_text_cb)(void *user_data, char *default_text, int cursor_pos);

EXAPI int remote_control_default_text_callback_set(remote_control_client *client, remote_control_default_text_cb func, void *user_data);

EXAPI int remote_control_default_text_callback_unset(remote_control_client *client);

EXAPI int remote_control_send_key_event(remote_control_client *client, remote_control_key_type_e key);

EXAPI int remote_control_send_commit_string(remote_control_client *client, const char *text);

EXAPI int remote_control_update_preedit_string(remote_control_client *client, const char *text, Eina_List *attrs, int cursor_pos);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif