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

#include <sclui.h>
#include "ise.h"
#include "sdk.h"
#include "option.h"
#include "sclcore.h"

#include "ise_lang_table.h"

using namespace scl;

#define MVK_Shift_L 0xffe1
#define MVK_Caps_Lock 0xffe5
#define MVK_Shift_Off 0xffe1
#define MVK_Shift_On 0xffe2
#define MVK_Shift_Lock 0xffe6
#define MVK_Shift_Enable 0x9fe7
#define MVK_Shift_Disable 0x9fe8

static ISELanguageManager _language_manager;

/*
 * This callback class will receive all response events from SCL
 * So you should perform desired tasks in this class.
 */

const sclchar *PLACEHOLDER_MAIN_KEYBOARD_NAME = "abc";

class CSDKISE : public ILanguageCallback
{
public :
    CSDKISE() {
        /* Our CSDKISE instance will handle the following input modes... */
        sclint loop;

        for (loop = 0;loop < get_lang_table_size();loop++) {
            INPUT_MODE_INFO input_mode_QTY;
            input_mode_QTY.name = get_lang_table()[loop].inputmode_QTY;
            input_mode_QTY.display_name = get_lang_table()[loop].inputmode_QTY_name;
            LANGUAGE_INFO language;
            language.name = get_lang_table()[loop].language;
            language.display_name = get_lang_table()[loop].language_name;
            if (get_lang_table()[loop].locale_string) {
                language.locale_string = get_lang_table()[loop].locale_string;
            }
            language.callback = this;
            language.input_modes.push_back(input_mode_QTY);
            language.priority = LANGAUGE_PRIORITY_DEFAULT;
            language.resource_file = MAIN_ENTRY_XML_PATH;
            language.is_latin_language = get_lang_table()[loop].is_latin_language;
            language.accepts_caps_mode = get_lang_table()[loop].accepts_caps_mode;

            /* These variable should be read from stored setting values */
            language.enabled = FALSE;
            language.selected_input_mode = input_mode_QTY.name;

            _language_manager.add_language(language);
        }
    }
    SCLEventReturnType on_event_key_clicked(SclUIEventDesc event_desc);
    SCLEventReturnType on_event_drag_state_changed(SclUIEventDesc event_desc);

    sclboolean on_language_selected(const sclchar *language, const sclchar *input_mode);
    sclboolean on_language_unselected(const sclchar *language, const sclchar *input_mode);

    sclboolean flush_imengine(const sclchar *language);
    private:
        SCLEventReturnType process_key_type_char(const SclUIEventDesc&);
};

static CSDKISE ise_instance;

extern CSCLUI *g_ui;
extern CSCLCore g_core;

extern CONFIG_VALUES g_config_values;
extern KEYBOARD_STATE g_keyboard_state;

static void set_caps_mode(sclint mode) {
    if (g_ui->get_shift_state() != SCL_SHIFT_STATE_LOCK) {
        g_ui->set_shift_state(mode ? SCL_SHIFT_STATE_ON : SCL_SHIFT_STATE_OFF);
    }
}
SCLEventReturnType
CSDKISE::process_key_type_char(const SclUIEventDesc& event_desc)
{
    // patch for Chinese symbol ^
    // Chinese engine will regards ^ as ……
    // this patch is to avoid this
    if (0 == strcmp(_language_manager.get_current_language(), "Chinese")) {
        if (event_desc.key_event == '^') {
            ise_forward_key_event(event_desc.key_event);
            return SCL_EVENT_DONE;
        }
    }
    // patch for Chinese symbol ^ end
    /* If longkey symbol was pressed, let's flush the preedit buffer */
    if (event_desc.key_modifier == KEY_MODIFIER_LONGKEY) {
        flush_imengine(_language_manager.get_current_language());
    }


    return SCL_EVENT_PASS_ON;
}

SCLEventReturnType CSDKISE::on_event_key_clicked(SclUIEventDesc event_desc)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;

    switch (event_desc.key_type) {
        case KEY_TYPE_CHAR:
            ret = process_key_type_char(event_desc);
            break;
        case KEY_TYPE_STRING:
            flush_imengine(_language_manager.get_current_language());
            break;
        case KEY_TYPE_MODECHANGE:
            {
                flush_imengine(_language_manager.get_current_language());
            }
            break;
        case KEY_TYPE_CONTROL:
        /* Do not flush when the entered key is one of : BACKSPACE / SHIFT / CAPSLOCK / SHIFTLOCK */
        if (event_desc.key_event != MVK_BackSpace &&
            event_desc.key_event != MVK_Shift_L &&
            event_desc.key_event != MVK_Caps_Lock &&
            event_desc.key_event != MVK_Shift_Lock &&
            event_desc.key_event != MVK_space &&
            event_desc.key_event != MVK_Return) {
            flush_imengine(_language_manager.get_current_language());
        }
        if (event_desc.key_event == MVK_space) {
            if (_language_manager.get_enabled_languages_num() > 1) {
                if (event_desc.key_modifier == KEY_MODIFIER_DIRECTION_LEFT) {
                    /* If flick event upon space key was detected, perform a language change and don't proceed anymore */
                    _language_manager.select_previous_language();
                    ret = SCL_EVENT_DONE;
                }
                if (event_desc.key_modifier == KEY_MODIFIER_DIRECTION_RIGHT) {
                    /* If flick event upon space key was detected, perform a language change and don't proceed anymore */
                    _language_manager.select_next_language();
                    ret = SCL_EVENT_DONE;
                }
                LANGUAGE_INFO *info = _language_manager.get_language_info(_language_manager.get_current_language());
                if (info) {
                    if (info->accepts_caps_mode) {
                        // FIXME this if condition means the AC is off
                        if (g_keyboard_state.layout != ISE_LAYOUT_STYLE_NORMAL) {
                            g_ui->set_autocapital_shift_state(TRUE);
                            g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                        }

                        // normal layout means the AC is on
                        else {
                            g_ui->set_autocapital_shift_state(FALSE);
                            ise_send_event(MVK_Shift_Enable, KEY_MASK_NULL);
                            set_caps_mode(g_keyboard_state.caps_mode);
                        }
                    } else {
                        g_ui->set_autocapital_shift_state(TRUE);
                        ise_send_event(MVK_Shift_Disable, KEY_MASK_NULL);
                        g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                    }
                }
            } else {
                if (event_desc.key_modifier == KEY_MODIFIER_DIRECTION_LEFT ||
                    event_desc.key_modifier == KEY_MODIFIER_DIRECTION_LEFT_RETURN) {
                        /* If flick event upon space key was detected, perform a cursor movement and don't proceed anymore */
                        ise_send_event(MVK_Left, 0);
                        ret = SCL_EVENT_DONE;
                }
                if (event_desc.key_modifier == KEY_MODIFIER_DIRECTION_RIGHT ||
                    event_desc.key_modifier == KEY_MODIFIER_DIRECTION_RIGHT_RETURN) {
                        /* If flick event upon space key was detected, perform a cursor movement and don't proceed anymore */
                        ise_send_event(MVK_Right, 0);
                        ret = SCL_EVENT_DONE;
                }
            }
        }
        break;
    default:
        break;
    }

    return ret;
}

SCLEventReturnType CSDKISE::on_event_drag_state_changed(SclUIEventDesc event_desc)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;

    if (event_desc.event_type == EVENT_TYPE_LONGPRESS) {

        const char *www_string = "www.";
        if (event_desc.key_value) {
            if (strncmp(event_desc.key_value, www_string, strlen(www_string)) == 0) {
                /* Let's not do anything when the "www." button is longpressed,
                   instead of displaying ".ac.uk" or "co.uk" popup window */
                ret = SCL_EVENT_DONE;
            }
        }
    }

    return ret;
}

sclboolean CSDKISE::on_language_selected(const sclchar *language, const sclchar *input_mode)
{
    sclboolean ret = FALSE;

    /* We have to change these url postfixes with corresponding language-specific ones */
    const char *url_postfixes[] = {".co.us", ".or.us", ".ac.us", ".go.us"};
    const char *replace_target = "us";

    if (language) {
        sclint loop;
        for (loop = 0;loop < get_lang_table_size();loop++) {
            if (strcmp(language, get_lang_table()[loop].language) == 0) {
                if (g_ui) {
                    if (get_lang_table()[loop].keyboard_ise_uuid) {
                        g_core.set_keyboard_ise_by_uuid(get_lang_table()[loop].keyboard_ise_uuid);
                        g_core.send_imengine_event(-1, get_lang_table()[loop].keyboard_ise_uuid,
                            get_lang_table()[loop].language_command, get_lang_table()[loop].language_code);
                    }

                    /* This is to update the screen only for once, not everytime we request a UI update  */
                    g_ui->set_update_pending(TRUE);
                    g_ui->set_input_mode(input_mode);

                    SclSize size_portrait = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_PORTRAIT);
                    SclSize size_landscape = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_LANDSCAPE);
                    g_core.set_keyboard_size_hints(size_portrait, size_landscape);

                    /* Check if we need to turn on the shift key */
                    LANGUAGE_INFO *info = _language_manager.get_language_info(language);
                    if (info) {
                        if (info->accepts_caps_mode) {
                            if (g_keyboard_state.caps_mode) {
                                g_ui->set_shift_state(SCL_SHIFT_STATE_ON);
                                ise_send_event(MVK_Shift_On, KEY_MASK_NULL);
                                g_keyboard_state.caps_mode = TRUE;
                            } else {
                                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                                ise_send_event(MVK_Shift_Off, KEY_MASK_NULL);
                                g_keyboard_state.caps_mode = FALSE;
                            }
                            if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_NORMAL) {
                                // not allow the SCL auto capital shift state
                                g_ui->set_autocapital_shift_state(FALSE);
                            }
                        } else {
                            g_ui->set_autocapital_shift_state(TRUE);
                        }
                    } else {
                        g_ui->set_autocapital_shift_state(TRUE);
                    }
                    /* And set the url postfixes */
                    for (size_t inner_loop = 0; inner_loop < (sizeof(url_postfixes) / sizeof (const char *)); inner_loop++) {
                        std::string postfix = url_postfixes[inner_loop];
                        std::string::size_type offset = postfix.find(replace_target, 0);

                        if(offset != postfix.npos) {
                            postfix.replace(offset, strlen(replace_target), get_lang_table()[loop].country_code_URL);
                        }

                        g_ui->set_string_substitution(url_postfixes[inner_loop], postfix.c_str());
                    }

                    /* Replace LANGUAGE_STRING with localized language name */
                    g_ui->set_string_substitution(LANGUAGE_STRING, get_lang_table()[loop].language_name);
                    /* Change main_keyboard_name with localized language name */
                    if (get_lang_table()[loop].main_keyboard_name) {
                        g_ui->set_string_substitution(PLACEHOLDER_MAIN_KEYBOARD_NAME, get_lang_table()[loop].main_keyboard_name);
                    }

                    /* Now we update the whole screen */
                    g_ui->set_update_pending(FALSE);
                }

                ret = TRUE;
            }
        }
    }

    if (ret) {
        g_core.update_input_context(ECORE_IMF_INPUT_PANEL_LANGUAGE_EVENT, 0);
    }

    return ret;
}

sclboolean CSDKISE::on_language_unselected(const sclchar *language, const sclchar *input_mode)
{
    sclboolean ret = FALSE;

    if (language) {
        sclint loop;
        for (loop = 0;loop < get_lang_table_size();loop++) {
            if (strcmp(language, get_lang_table()[loop].language) == 0) {
                flush_imengine(language);
                ret = TRUE;
            }
        }
    }

    return ret;
}

static inline int get_lang_id(const sclchar* language) {
    if (language == NULL) return -1;

    for (int loop = 0;loop < get_lang_table_size();loop++) {
        if (strcmp(language, get_lang_table()[loop].language) == 0) {
            if (get_lang_table()[loop].keyboard_ise_uuid) {
                return loop;
            }
        }
    }
    return -1;
}

sclboolean CSDKISE::flush_imengine(const sclchar *language)
{
    bool bRet = false;

    int lang_id = get_lang_id(language);
    if (lang_id != -1) {
        if (get_lang_table()[lang_id].flush_code != 0) {
            g_core.send_imengine_event(-1, get_lang_table()[lang_id].keyboard_ise_uuid,
                    get_lang_table()[lang_id].flush_command, get_lang_table()[lang_id].flush_code);
            bRet = true;
        } else {
            g_core.reset_keyboard_ise();
            bRet = true;
        }

    }

    return bRet;
}
