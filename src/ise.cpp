/*
 * Copyright 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <Ecore_X.h>
#include <Ecore_IMF.h>
#include <Elementary.h>

#include <X11/XF86keysym.h>

#include <vconf.h>
#include <vconf-keys.h>

#include "scl.h"
#include "ise.h"
#include "utils.h"
#include "option.h"
#include "common.h"
#include "languages.h"

using namespace scl;
CSCLUI *gSCLUI = NULL;
extern CISECommon *g_ise_common;
extern CONFIG_VALUES g_config_values;

KEYBOARD_STATE g_keyboard_state = {
    -1,
    ISE_LAYOUT_STYLE_NORMAL,
    FALSE,
    TRUE,
};

#define MVK_Shift_L 0xffe1
#define MVK_Caps_Lock 0xffe5
#define MVK_Shift_Off 0xffe1
#define MVK_Shift_On 0xffe2
#define MVK_Shift_Lock 0xffe6
#define MVK_Shift_Enable 0x9fe7
#define MVK_Shift_Disable 0x9fe8

#define USER_KEYSTRING_OPTION "OPTION"

/*
 * This callback class will receive all response events from SCL
 * So you should perform desired tasks in this class.
 */
class CUIEventCallback : public ISCLUIEventCallback
{
public :
    SCLEventReturnType on_event_key_clicked(SclUIEventDesc event_desc);
    SCLEventReturnType on_event_drag_state_changed(SclUIEventDesc event_desc);
    SCLEventReturnType on_event_notification(SCLUINotiType notiType, sclint etcInfo);
};

static CUIEventCallback callback;

/**
 * Send the given string to input framework
 */
void
ise_send_string(const sclchar *key_value)
{
    g_ise_common->hide_preedit_string(-1, "");
    g_ise_common->commit_string(-1, "", key_value);
}

/**
* Send the preedit string to input framework
*/
void
ise_update_preedit_string(const sclchar *str)
{
    g_ise_common->update_preedit_string(-1, "", str);
}

/**
* Send the given event to input framework
*/
void
ise_send_event(sclulong key_event, sclulong key_mask)
{
    g_ise_common->send_key_event(-1, "", key_event, scim::SCIM_KEY_NullMask);
    g_ise_common->send_key_event(-1, "", key_event, scim::SCIM_KEY_ReleaseMask);
}

/**
* Forward the given event to input framework
*/
void
ise_forward_key_event(sclulong key_event)
{
    g_ise_common->forward_key_event(-1, "", key_event, scim::SCIM_KEY_NullMask);
    g_ise_common->forward_key_event(-1, "", key_event, scim::SCIM_KEY_ReleaseMask);
}

static sclboolean
on_input_mode_changed(const sclchar *key_value, sclulong key_event, sclint key_type)
{
    sclboolean ret = FALSE;

    if (gSCLUI) {
        if (key_value) {
            if (strcmp(key_value, "CUR_LANG") == 0) {
                ret = ISELanguageManager::select_current_language();
            }
            if (strcmp(key_value, "NEXT_LANG") == 0) {
                ret = ISELanguageManager::select_next_language();
            }
        }
        LANGUAGE_INFO *info = ISELanguageManager::get_language_info(ISELanguageManager::get_current_language());
        if (info) {
            if (info->accepts_caps_mode) {
                ise_send_event(MVK_Shift_Enable, scim::SCIM_KEY_NullMask);
                gSCLUI->set_caps_mode(g_keyboard_state.caps_mode);
            } else {
                ise_send_event(MVK_Shift_Disable, scim::SCIM_KEY_NullMask);
                gSCLUI->set_caps_mode(0);
            }
        }
    }

    return ret;
}


SCLEventReturnType CUIEventCallback::on_event_notification(SCLUINotiType noti_type, sclint etc_info)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;

    if (noti_type == SCL_UINOTITYPE_GESTURE_FLICK) {
        if (etc_info == DRAG_DOWN) {
            ise_hide();
            ret = SCL_EVENT_DONE;
        }
    }
    if (noti_type == SCL_UINOTITYPE_SHIFT_STATE_CHANGE) {
        LANGUAGE_INFO *info = ISELanguageManager::get_language_info(ISELanguageManager::get_current_language());
        if (info) {
            if (info->accepts_caps_mode) {
                if (etc_info == SCL_SHIFT_STATE_OFF) {
                    ise_send_event(MVK_Shift_Off, scim::SCIM_KEY_NullMask);
                }
                else if (etc_info == SCL_SHIFT_STATE_ON) {
                    ise_send_event(MVK_Shift_On, scim::SCIM_KEY_NullMask);
                }
                else if (etc_info == SCL_SHIFT_STATE_LOCK) {
                    ise_send_event(MVK_Shift_Lock, scim::SCIM_KEY_NullMask);
                }
                ret = SCL_EVENT_PASS_ON;
            }
        }
    }

    return ret;
}

SCLEventReturnType CUIEventCallback::on_event_drag_state_changed(SclUIEventDesc event_desc)
{
    return SCL_EVENT_PASS_ON;
}

SCLEventReturnType CUIEventCallback::on_event_key_clicked(SclUIEventDesc event_desc)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;

    if (gSCLUI) {
        switch (event_desc.key_type) {
        case KEY_TYPE_STRING:
            if (event_desc.key_modifier != KEY_MODIFIER_MULTITAP_START &&
                event_desc.key_modifier != KEY_MODIFIER_MULTITAP_REPEAT) {
                    if (event_desc.key_event) {
                        ise_forward_key_event(event_desc.key_event);
                    } else {
                        ise_send_string(event_desc.key_value);
                    }
            }
            break;
        case KEY_TYPE_CHAR: {
                sclboolean need_forward = FALSE;
                // FIXME : Should decide when to forward key events
                const sclchar *input_mode = gSCLUI->get_input_mode();
                if (input_mode) {
                    if (strcmp(input_mode, "SYM_QTY_1") == 0 ||
                            strcmp(input_mode, "SYM_QTY_2") == 0 ||
                            strcmp(input_mode, "PHONE_3X4") == 0 ||
                            strcmp(input_mode, "IPv6_3X4_123") == 0 ||
                            strcmp(input_mode, "IPv6_3X4_ABC") == 0 ||
                            strcmp(input_mode, "NUMONLY_3X4") == 0) {
                        need_forward = TRUE;
                    }
                }
                if (event_desc.key_event) {
                    if (need_forward) {
                        ise_forward_key_event(event_desc.key_event);
                    } else {
                        ise_send_event(event_desc.key_event, scim::SCIM_KEY_NullMask);
                    }
                }
                break;
            }
        case KEY_TYPE_CONTROL: {
                if (event_desc.key_event) {
                    ise_send_event(event_desc.key_event, scim::SCIM_KEY_NullMask);
               }
               break;
           }
        case KEY_TYPE_MODECHANGE:
            if (on_input_mode_changed(event_desc.key_value, event_desc.key_event, event_desc.key_type)) {
                ret = SCL_EVENT_DONE;
            }
            break;
        case KEY_TYPE_USER:
            if (strcmp(event_desc.key_value, USER_KEYSTRING_OPTION) == 0) {
                open_option_window(NULL, ROTATION_TO_DEGREE(gSCLUI->get_rotation()));
                ret = SCL_EVENT_DONE;
            }
            break;
        default:
            break;
        }
    }

    return ret;
}

void
ise_set_layout(sclu32 layout)
{
    /* Check if the layoutIdx is in the valid range */
    if (layout < ISE_LAYOUT_STYLE_MAX) {
        if (g_keyboard_state.layout != layout) {
            g_keyboard_state.need_reset = TRUE;
        }
        g_keyboard_state.layout = layout;

        if (layout == ISE_LAYOUT_STYLE_PHONENUMBER ||
                layout == ISE_LAYOUT_STYLE_IP ||
                layout == ISE_LAYOUT_STYLE_MONTH ||
                layout == ISE_LAYOUT_STYLE_NUMBERONLY) {
            if (g_ise_common) {
                g_ise_common->set_keyboard_ise_by_uuid(DEFAULT_KEYBOARD_ISE_UUID);
            }
        }
    }
}

void
ise_reset_context()
{
}

void
ise_reset_input_context()
{
}

void ise_show(int ic)
{
    sclboolean reset_inputmode = FALSE;
    if (gSCLUI && g_ise_common) {

        read_ise_config_values();

        ISELanguageManager::set_enabled_languages(g_config_values.enabled_languages);

        /* Reset input mode if the input context value has changed */
        if (ic != g_keyboard_state.ic) {
            reset_inputmode = TRUE;
        }
        /* Reset input mode if the current language is not the selected language */
        if (g_config_values.selected_language.compare(
                    ISELanguageManager::get_current_language()) != 0) {
            reset_inputmode = TRUE;
        }
        /* No matter what, just reset the inputmode if it needs to */
        if (g_keyboard_state.need_reset) {
            reset_inputmode = TRUE;
        }

        /* If the current layout requires latin language and current our language is not latin, enable the primary latin */
        sclboolean force_primary_latin = FALSE;
        LANGUAGE_INFO *info = ISELanguageManager::get_language_info(g_config_values.selected_language.c_str());
        if (info) {
            if (g_ise_default_values[g_keyboard_state.layout].force_latin && !(info->is_latin_language)) {
                force_primary_latin = TRUE;
            }
        }
        if (force_primary_latin) {
            ISELanguageManager::set_language_enabled_temporarily(PRIMARY_LATIN_LANGUAGE, TRUE);
        }

        if (reset_inputmode) {
            int loop;

            ise_reset_context();

            /* Turn the shift state off if we need to reset our input mode, only when auto-capitaluzation is not set  */
            if (!(g_keyboard_state.caps_mode)) {
                gSCLUI->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
            if (g_keyboard_state.layout < ISE_LAYOUT_STYLE_MAX) {
                /* If this layout requires specific input mode, set it */
                if (strlen(g_ise_default_values[g_keyboard_state.layout].input_mode) > 0) {
                    gSCLUI->set_input_mode(g_ise_default_values[g_keyboard_state.layout].input_mode);

                    SclSize size_portrait = gSCLUI->get_input_mode_size(gSCLUI->get_input_mode(), DISPLAYMODE_PORTRAIT);
                    SclSize size_landscape = gSCLUI->get_input_mode_size(gSCLUI->get_input_mode(), DISPLAYMODE_LANDSCAPE);
                    g_ise_common->set_keyboard_size_hints(size_portrait, size_landscape);
                } else {
                    if (force_primary_latin) {
                        ISELanguageManager::select_language(PRIMARY_LATIN_LANGUAGE, TRUE);
                    } else {
                        if (!(ISELanguageManager::select_language(g_config_values.selected_language.c_str()))) {
                            ISELanguageManager::select_language(PRIMARY_LATIN_LANGUAGE);
                        }
                    }
                }
                gSCLUI->set_cur_sublayout(g_ise_default_values[g_keyboard_state.layout].sublayout_name);
            }
        }

        gSCLUI->show();
        gSCLUI->disable_input_events(FALSE);
    }

    g_keyboard_state.ic = ic;
    g_keyboard_state.need_reset = FALSE;
}

/**
 * Sets screen rotation
 */
void
ise_set_screen_rotation(int degree)
{
    if (gSCLUI) {
        gSCLUI->set_rotation(DEGREE_TO_SCLROTATION(degree));
    }
}

void
ise_hide()
{
    if (gSCLUI && g_ise_common) {
        gSCLUI->disable_input_events(TRUE);
        gSCLUI->hide();
    }
}

void
ise_create()
{
    if (!gSCLUI) {
        gSCLUI = new CSCLUI;
    }

    /* Set scl_parser_type
     * default type is text xml
     * use command: export sclres_type="sclres_binary" to enable use binary resource
     * please make sure there is sclresource.bin in resource folder
     * Or you can use `xml2binary $resource_dir` to generate the sclresource.bin
     * xml2binary is in the libscl-ui-devel package
     */
    SCLParserType scl_parser_type = SCL_PARSER_TYPE_XML;
    char* sclres_type = getenv("sclres_type");
    if (sclres_type != NULL && 0 == strcmp("sclres_binary", sclres_type)) {
        scl_parser_type = SCL_PARSER_TYPE_BINARY_XML;
    } else {
        scl_parser_type = SCL_PARSER_TYPE_XML;
    }


    if (gSCLUI && g_ise_common) {
        if (g_ise_common->get_main_window()) {
            sclboolean succeeded = FALSE;

            const sclchar *resource_file_path = ISELanguageManager::get_resource_file_path();

            if (resource_file_path) {
                if (strlen(resource_file_path) > 0) {
                    succeeded = gSCLUI->init((sclwindow)g_ise_common->get_main_window(), scl_parser_type, resource_file_path);
                }
            }
            if (!succeeded) {
                gSCLUI->init((sclwindow)g_ise_common->get_main_window(), scl_parser_type, MAIN_ENTRY_XML_PATH);
            }
            gSCLUI->set_longkey_duration(elm_config_longpress_timeout_get() * 1000);

            /* Default ISE callback */
            gSCLUI->set_ui_event_callback(&callback);

            /* Accumulated customized ISE callbacks, depending on the input modes */
            for (scluint loop = 0;loop < ISELanguageManager::get_languages_num();loop++) {
                LANGUAGE_INFO *language = ISELanguageManager::get_language_info(loop);
                if (language) {
                    for (scluint inner_loop = 0;inner_loop < language->input_modes.size();inner_loop++) {
                        INPUT_MODE_INFO &info = language->input_modes.at(inner_loop);
                        printf("Registering callback for input mode %s : %p\n", info.name.c_str(), language->callback);
                        gSCLUI->set_ui_event_callback(language->callback, info.name.c_str());
                    }
                }
            }

            read_ise_config_values();
            ISELanguageManager::set_enabled_languages(g_config_values.enabled_languages);

            ISELanguageManager::select_language(g_config_values.selected_language.c_str());
        }

        SclSize size_portrait = gSCLUI->get_input_mode_size(gSCLUI->get_input_mode(), DISPLAYMODE_PORTRAIT);
        SclSize size_landscape = gSCLUI->get_input_mode_size(gSCLUI->get_input_mode(), DISPLAYMODE_LANDSCAPE);
        g_ise_common->set_keyboard_size_hints(size_portrait, size_landscape);
    }
}

void
ise_destroy()
{
    if (gSCLUI) {
        delete gSCLUI;
        gSCLUI = NULL;
    }
}

void
ise_set_caps_mode(unsigned int mode)
{
    if (mode) {
        g_keyboard_state.caps_mode = TRUE;
    } else {
        g_keyboard_state.caps_mode = FALSE;
    }
    LANGUAGE_INFO *info = ISELanguageManager::get_language_info(ISELanguageManager::get_current_language());
    if (info) {
        if (info->accepts_caps_mode) {
            ise_send_event(MVK_Shift_Enable, scim::SCIM_KEY_NullMask);
            gSCLUI->set_caps_mode(g_keyboard_state.caps_mode);
        } else {
            ise_send_event(MVK_Shift_Disable, scim::SCIM_KEY_NullMask);
            gSCLUI->set_caps_mode(0);
        }
    }
}

void
ise_update_cursor_position(int position)
{
    if (gSCLUI) {
        if (position > 0) {
            gSCLUI->set_string_substitution("www.", ".com");
        } else {
            gSCLUI->unset_string_substitution("www.");
        }
    }
}

void ise_set_return_key_type(unsigned int type)
{
    const int BUF_LEN = 64;
    char buf[BUF_LEN];

    switch (type)
    {
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_DONE);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_GO);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_JOIN:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_JOIN);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_LOGIN:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_LOGIN);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_NEXT:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_NEXT);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_SEARCH);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEND:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_SEND);
        break;
    case ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SIGNIN:
        snprintf(buf, BUF_LEN, ISE_RETURN_KEY_LABEL_SIGNIN);
        break;
    }

    if (type == ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT) {
        gSCLUI->unset_private_key("Enter");
    } else {
        static sclchar *imagelabel[SCL_BUTTON_STATE_MAX] = {
            " ", " ", " "
        };
        gSCLUI->set_private_key("Enter", buf, imagelabel, NULL, 0, "Enter", TRUE);
    }
}

void ise_set_return_key_disable(unsigned int disabled)
{
    gSCLUI->enable_button("Enter", !disabled);
}
