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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vconf.h>

#ifndef WAYLAND
#include <Ecore_X.h>
#endif
#include <Ecore_IMF.h>
#include <Elementary.h>

#ifndef WAYLAND
#include <X11/XF86keysym.h>
#endif

#include "sclui.h"
#include "sclcore.h"
#include "ise.h"
#include "utils.h"
#include "option.h"
#include "languages.h"
#include "candidate-factory.h"
#define CANDIDATE_WINDOW_HEIGHT 84
using namespace scl;
#include <vector>
using namespace std;

CSCLUI *g_ui = NULL;

#include <sclcore.h>

#include "ise.h"

static CCoreEventCallback g_core_event_callback;
CSCLCore g_core(&g_core_event_callback);

extern CONFIG_VALUES g_config_values;
static sclboolean g_need_send_shift_event = FALSE;
#ifdef WAYLAND
int gLastIC = 0;
#endif

extern void set_ise_imdata(const char * buf, size_t &len);

KEYBOARD_STATE g_keyboard_state = {
    0,
    0,
    ISE_LAYOUT_STYLE_NORMAL,
    0,
    FALSE,
    TRUE,
    FALSE,
};

#define ISE_LAYOUT_NUMBERONLY_VARIATION_MAX 4
const sclchar *g_ise_numberonly_variation_name[ISE_LAYOUT_NUMBERONLY_VARIATION_MAX] = {
    "DEFAULT", "SIG", "DEC", "SIGDEC"
};

#define SIG_DEC_SIZE        2
static scluint              _click_count = 0;
static const char          *_sig_dec[SIG_DEC_SIZE] = {".", "-"};
static scluint              _sig_dec_event[SIG_DEC_SIZE] = {'.', '-'};
static Ecore_Timer         *_commit_timer = NULL;

Candidate *g_candidate = NULL;
class CandidateEventListener: public EventListener
{
    public:
        void on_event(const EventDesc &desc)
        {
            const MultiEventDesc &multidesc = dynamic_cast<const MultiEventDesc &>(desc);
            switch (multidesc.type) {
                case MultiEventDesc::CANDIDATE_ITEM_MOUSE_DOWN:
                    g_core.select_candidate(multidesc.index);
                    break;
                case MultiEventDesc::CANDIDATE_MORE_VIEW_SHOW:
                    // when more parts shows, click on the candidate will
                    // not affect the key click event
                    g_ui->disable_input_events(TRUE);
                    break;
                case MultiEventDesc::CANDIDATE_MORE_VIEW_HIDE:
                    g_ui->disable_input_events(FALSE);
                    break;
                default: break;
            }
        }
};
static CandidateEventListener g_candidate_event_listener;

static ISELanguageManager _language_manager;
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
    SCLEventReturnType on_event_notification(SCLUINotiType noti_type, SclNotiDesc *etc_info);
};

static CUIEventCallback callback;

sclboolean
check_ic_temporary(int ic)
{
    if ((ic & 0xFFFF) == 0) {
        return TRUE;
    }
    return FALSE;
}

void CCoreEventCallback::on_init()
{
    LOGD("CCoreEventCallback::init()\n");
    ise_create();
}

void CCoreEventCallback::on_run(int argc, char **argv)
{
    LOGD("CCoreEventCallback::on_run()\n");
    g_core.run();
}

void CCoreEventCallback::on_exit()
{
    ::ise_hide();
    ise_destroy();
}

void CCoreEventCallback::on_attach_input_context(sclint ic, const sclchar *ic_uuid)
{
    ise_attach_input_context(ic);
}

void CCoreEventCallback::on_detach_input_context(sclint ic, const sclchar *ic_uuid)
{
    ise_detach_input_context(ic);
}

void CCoreEventCallback::on_focus_in(sclint ic, const sclchar *ic_uuid)
{
    ise_focus_in(ic);
}

void CCoreEventCallback::on_focus_out(sclint ic, const sclchar *ic_uuid)
{
    ise_focus_out(ic);
}

void CCoreEventCallback::on_ise_show(sclint ic, const sclint degree, Ise_Context context)
{
    //g_ise_common->set_keyboard_ise_by_uuid(KEYBD_ISE_UUID);

    ise_reset_context(); // reset ISE

    /*if (context.language == ECORE_IMF_INPUT_PANEL_LANG_ALPHABET) {
        ise_explictly_set_language(PRIMARY_LATIN_LANGUAGE_INDEX);
    }*/

    ise_set_layout(context.layout, context.layout_variation);

    ise_set_return_key_type(context.return_key_type);
    ise_set_return_key_disable(context.return_key_disabled);

    ise_set_caps_mode(context.caps_mode);
    ise_update_cursor_position(context.cursor_pos);

    /* Do not follow the application's rotation angle if we are already in visible state,
       since in that case we will receive the angle through ROTATION_CHANGE_REQUEST message */
    if (!(g_keyboard_state.visible_state)) {
        ise_set_screen_rotation(degree);
    } else {
        LOGD("Skipping rotation angle , %d", degree);
    }

    ::ise_show(ic);
}

void CCoreEventCallback::on_ise_hide(sclint ic, const sclchar *ic_uuid)
{
    ::ise_hide();
}

void CCoreEventCallback::on_reset_input_context(sclint ic, const sclchar *uuid)
{
    ise_reset_input_context();
}

void CCoreEventCallback::on_set_display_language(const sclchar *language)
{
    setlocale(LC_ALL, language);
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    LOGD("Language : %s\n", (language ? language : "NULL"));
}

void CCoreEventCallback::on_set_accessibility_state(const sclboolean state)
{
    ise_set_accessibility_state(state);
}

void CCoreEventCallback::on_set_rotation_degree(sclint degree)
{
    ise_set_screen_rotation(degree);
}

void CCoreEventCallback::on_set_caps_mode(sclu32 mode)
{
    ise_set_caps_mode(mode);
}

void CCoreEventCallback::on_update_cursor_position(sclint ic, const sclchar *ic_uuid, sclint cursor_pos)
{
    ise_update_cursor_position(cursor_pos);
}

void CCoreEventCallback::on_set_return_key_type (sclu32 type)
{
    ise_set_return_key_type(type);
}

void CCoreEventCallback::on_set_return_key_disable (sclu32 disabled)
{
    ise_set_return_key_disable(disabled);
}

void CCoreEventCallback::on_set_imdata(sclchar *buf, sclu32 len)
{
    set_ise_imdata(buf, len);
}

void CCoreEventCallback::on_get_language_locale(sclint ic, sclchar **locale)
{
    ise_get_language_locale(locale);
}

void CCoreEventCallback::on_update_lookup_table(SclCandidateTable &table)
{
    vector<string> vec_str;
    //printf("candidate num: %d, current_page_size: %d\n",
    //    table.number_of_candidates(), table.get_current_page_size());
    for (int i = table.current_page_start; i < table.current_page_start + table.page_size; ++i)
    {
        if (i < (int)table.candidate_labels.size()) {
            vec_str.push_back(table.candidate_labels[i]);
        }
    }
    ise_update_table(vec_str);
}

void CCoreEventCallback::on_create_option_window(sclwindow window, SCLOptionWindowType type)
{
    if (window) {
        option_window_created(NATIVE_WINDOW_CAST(window), type);
    }
}

void CCoreEventCallback::on_destroy_option_window(sclwindow window)
{
    option_window_destroyed(NATIVE_WINDOW_CAST(window));
}

/**
 * Send the given string to input framework
 */
void
ise_send_string(const sclchar *key_value)
{
    int ic = -1;
    if (!check_ic_temporary(g_keyboard_state.ic)) {
        ic = g_keyboard_state.ic;
    }
    g_core.hide_preedit_string(ic, "");
    g_core.commit_string(ic, "", key_value);
    LOGD("ic : %x, %s", ic, key_value);
}

/**
* Send the preedit string to input framework
*/
void
ise_update_preedit_string(const sclchar *str)
{
    int ic = -1;
    if (!check_ic_temporary(g_keyboard_state.ic)) {
        ic = g_keyboard_state.ic;
    }
    g_core.update_preedit_string(ic, "", str);
    LOGD("ic : %x, %s", ic, str);
}

/**
* Send the given event to input framework
*/
void
ise_send_event(sclulong key_event, sclulong key_mask)
{
    int ic = -1;
    if (!check_ic_temporary(g_keyboard_state.ic)) {
        ic = g_keyboard_state.ic;
    }
    g_core.send_key_event(ic, "", key_event, KEY_MASK_NULL);
    g_core.send_key_event(ic, "", key_event, KEY_MASK_RELEASE);
    LOGD("ic : %x, %x", ic, key_event);
}

/**
* Forward the given event to input framework
*/
void
ise_forward_key_event(sclulong key_event)
{
    int ic = -1;
    if (!check_ic_temporary(g_keyboard_state.ic)) {
        ic = g_keyboard_state.ic;
    }
    g_core.forward_key_event(ic, "", key_event, KEY_MASK_NULL);
    g_core.forward_key_event(ic, "", key_event, KEY_MASK_RELEASE);
    LOGD("ic : %x, %x", ic, key_event);
}

static void set_caps_mode(sclint mode) {
    if (g_ui) {
        if (g_ui->get_shift_state() != SCL_SHIFT_STATE_LOCK) {
            g_ui->set_shift_state(mode ? SCL_SHIFT_STATE_ON : SCL_SHIFT_STATE_OFF);
        }
    }
}

/**
 * @brief Delete commit timer.
 *
 * @return void
 */
static void delete_commit_timer (void)
{
    if (_commit_timer != NULL) {
        ecore_timer_del (_commit_timer);
        _commit_timer = NULL;
    }
}

/**
 * @brief Callback function for commit timer.
 *
 * @param data Data to pass when it is called.
 *
 * @return ECORE_CALLBACK_CANCEL
 */
static Eina_Bool commit_timeout (void *data)
{
    if (_commit_timer != NULL) {
        g_core.hide_preedit_string (-1, "");
        ise_forward_key_event (_sig_dec_event[(_click_count-1)%SIG_DEC_SIZE]);
        _click_count = 0;
    }
    _commit_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static sclboolean
on_input_mode_changed(const sclchar *key_value, sclulong key_event, sclint key_type)
{
    sclboolean ret = FALSE;

    if (g_ui) {
        if (key_value) {
            if (strcmp(key_value, "CUR_LANG") == 0) {
                ret = _language_manager.select_current_language();
            }
            if (strcmp(key_value, "NEXT_LANG") == 0) {
                ret = _language_manager.select_next_language();
            }
        }
        LANGUAGE_INFO *info = _language_manager.get_language_info(_language_manager.get_current_language());
        if (info) {
            if (info->accepts_caps_mode) {
                ise_send_event(MVK_Shift_Enable, KEY_MASK_NULL);
                set_caps_mode(g_keyboard_state.caps_mode);
            } else {
                ise_send_event(MVK_Shift_Disable, KEY_MASK_NULL);
                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
        }
    }

    return ret;
}


SCLEventReturnType CUIEventCallback::on_event_notification(SCLUINotiType noti_type, SclNotiDesc *etc_info)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;

    if (noti_type == SCL_UINOTITYPE_SHIFT_STATE_CHANGE) {
        if (g_need_send_shift_event) {
            LANGUAGE_INFO *info = _language_manager.get_language_info(_language_manager.get_current_language());
            SclNotiShiftStateChangeDesc *desc = static_cast<SclNotiShiftStateChangeDesc*>(etc_info);
            if (info && desc) {
                if (info->accepts_caps_mode) {
                    if (desc->shift_state == SCL_SHIFT_STATE_OFF) {
                        ise_send_event(MVK_Shift_Off, KEY_MASK_NULL);
                    }
                    else if (desc->shift_state == SCL_SHIFT_STATE_ON) {
                        ise_send_event(MVK_Shift_On, KEY_MASK_NULL);
                    }
                    else if (desc->shift_state == SCL_SHIFT_STATE_LOCK) {
                        ise_send_event(MVK_Shift_Lock, KEY_MASK_NULL);
                    }
                    ret = SCL_EVENT_PASS_ON;
                }
            }
            g_need_send_shift_event = FALSE;
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

    if (g_ui) {
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
                const sclchar *input_mode = g_ui->get_input_mode();
                if (input_mode) {
                    if (strcmp(input_mode, "SYM_QTY_1") == 0 ||
                            strcmp(input_mode, "SYM_QTY_2") == 0 ||
                            strcmp(input_mode, "PHONE_3X4") == 0 ||
                            strcmp(input_mode, "IPv6_3X4_123") == 0 ||
                            strcmp(input_mode, "IPv6_3X4_ABC") == 0 ||
                            strcmp(input_mode, "NUMONLY_3X4") == 0 ||
                            strcmp(input_mode, "NUMONLY_3X4_SIG") == 0 ||
                            strcmp(input_mode, "NUMONLY_3X4_DEC") == 0 ||
                            strcmp(input_mode, "NUMONLY_3X4_SIGDEC") == 0 ||
                            strcmp(input_mode, "DATETIME_3X4") == 0) {
                        need_forward = TRUE;
                    }
                }
                if (input_mode && strcmp (input_mode, "NUMONLY_3X4_SIGDEC") == 0 &&
                    strcmp (event_desc.key_value, ".") == 0) {
                    g_core.update_preedit_string (-1, "", _sig_dec[_click_count%SIG_DEC_SIZE]);
                    g_core.show_preedit_string (-1, "");
                    delete_commit_timer ();
                    _commit_timer = ecore_timer_add (1.0, commit_timeout, NULL);
                    _click_count++;
                } else if (event_desc.key_event) {
                    commit_timeout (NULL);
                    if (need_forward) {
                        ise_forward_key_event(event_desc.key_event);
                    } else {
                        ise_send_event(event_desc.key_event, KEY_MASK_NULL);
                    }
                }
                break;
            }
        case KEY_TYPE_CONTROL: {
                commit_timeout (NULL);
                if (event_desc.key_event) {
                    ise_send_event(event_desc.key_event, KEY_MASK_NULL);
                    if (event_desc.key_event == MVK_Shift_L) {
                        g_need_send_shift_event = TRUE;
                    }
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
                //open_option_window(NULL, ROTATION_TO_DEGREE(g_ui->get_rotation()));
                g_core.create_option_window();
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
ise_set_layout(sclu32 layout, sclu32 layout_variation)
{
    /* Check if the layoutIdx is in the valid range */
    if (layout < ISE_LAYOUT_STYLE_MAX) {
        if (g_keyboard_state.layout != layout ||
            g_keyboard_state.layout_variation != layout_variation) {
            g_keyboard_state.need_reset = TRUE;
        }
        g_keyboard_state.layout = layout;
        g_keyboard_state.layout_variation = layout_variation;
        LOGD ("layout:%d, variation:%d", g_keyboard_state.layout, g_keyboard_state.layout_variation);
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

void
ise_focus_in(int ic)
{
    LOGD("ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x", ic, check_ic_temporary(ic),
            g_keyboard_state.ic, check_ic_temporary(g_keyboard_state.ic),
            g_keyboard_state.focused_ic, check_ic_temporary(g_keyboard_state.focused_ic));
    if (check_ic_temporary(g_keyboard_state.ic) && !check_ic_temporary(ic)) {
        g_keyboard_state.ic = ic;
    }
    g_keyboard_state.focused_ic = ic;
    if (ic == g_keyboard_state.ic) {
        if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_PHONENUMBER ||
                g_keyboard_state.layout == ISE_LAYOUT_STYLE_IP ||
                g_keyboard_state.layout == ISE_LAYOUT_STYLE_MONTH ||
                g_keyboard_state.layout == ISE_LAYOUT_STYLE_NUMBERONLY) {
            g_core.set_keyboard_ise_by_uuid(DEFAULT_KEYBOARD_ISE_UUID);
        }
    }
}

void
ise_focus_out(int ic)
{
    g_keyboard_state.focused_ic = 0;
}

void
ise_attach_input_context(int ic)
{
    LOGD("attaching, ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x", ic, check_ic_temporary(ic),
            g_keyboard_state.ic, check_ic_temporary(g_keyboard_state.ic),
            g_keyboard_state.focused_ic, check_ic_temporary(g_keyboard_state.focused_ic));
    ise_focus_in(ic);
}

void
ise_detach_input_context(int ic)
{
    ise_focus_out(ic);
}

void
ise_show(int ic)
{
    sclboolean reset_inputmode = FALSE;
    if (g_ui) {

        read_ise_config_values();

        _language_manager.set_enabled_languages(g_config_values.enabled_languages);

        LOGD("ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x", ic, check_ic_temporary(ic),
                g_keyboard_state.ic, check_ic_temporary(g_keyboard_state.ic),
                g_keyboard_state.focused_ic, check_ic_temporary(g_keyboard_state.focused_ic));

        if (check_ic_temporary(ic) && !check_ic_temporary(g_keyboard_state.focused_ic)) {
            ic = g_keyboard_state.focused_ic;
        }
        if (!check_ic_temporary(ic) && check_ic_temporary(g_keyboard_state.focused_ic)) {
            g_keyboard_state.focused_ic = ic;
        }
        if (ic == g_keyboard_state.focused_ic) {
            if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_PHONENUMBER ||
                    g_keyboard_state.layout == ISE_LAYOUT_STYLE_IP ||
                    g_keyboard_state.layout == ISE_LAYOUT_STYLE_MONTH ||
                    g_keyboard_state.layout == ISE_LAYOUT_STYLE_NUMBERONLY) {
                g_core.set_keyboard_ise_by_uuid(DEFAULT_KEYBOARD_ISE_UUID);
            }
        }

        /* Reset input mode if the input context value has changed */
        if (ic != g_keyboard_state.ic) {
            reset_inputmode = TRUE;
        }
        g_keyboard_state.ic = ic;
        /* Reset input mode if the current language is not the selected language */
        if (g_config_values.selected_language.compare(_language_manager.get_current_language()) != 0) {
            reset_inputmode = TRUE;
        }
        /* No matter what, just reset the inputmode if it needs to */
        if (g_keyboard_state.need_reset) {
            reset_inputmode = TRUE;
        }
        g_keyboard_state.need_reset = FALSE;

        /* If the current layout requires latin language and current our language is not latin, enable the primary latin */
        sclboolean force_primary_latin = FALSE;
        LANGUAGE_INFO *info = _language_manager.get_language_info(g_config_values.selected_language.c_str());
        if (info) {
            if (g_ise_default_values[g_keyboard_state.layout].force_latin && !(info->is_latin_language)) {
                force_primary_latin = TRUE;
            }
        }
        if (force_primary_latin) {
            _language_manager.set_language_enabled_temporarily(PRIMARY_LATIN_LANGUAGE, TRUE);
        }

        if (reset_inputmode) {
            ise_reset_context();

            /* Turn the shift state off if we need to reset our input mode, only when auto-capitalization is not set  */
            if (!(g_keyboard_state.caps_mode)) {
                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
            if (g_keyboard_state.layout < ISE_LAYOUT_STYLE_MAX) {
                sclu32 layout_index = g_keyboard_state.layout;
                if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_NUMBERONLY &&
                    g_keyboard_state.layout_variation > 0 &&
                    g_keyboard_state.layout_variation < ISE_LAYOUT_NUMBERONLY_VARIATION_MAX) {
                    layout_index = ISE_LAYOUT_STYLE_NUMBERONLY_SIG + g_keyboard_state.layout_variation - 1;
                } else if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_PASSWORD && g_keyboard_state.layout_variation > 0) {
                    layout_index = ISE_LAYOUT_STYLE_PASSWD_3X4;
                }

                /* If this layout requires specific input mode, set it */
                if (strlen(g_ise_default_values[layout_index].input_mode) > 0) {
                    g_ui->set_input_mode(g_ise_default_values[layout_index].input_mode);

                    SclSize size_portrait = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_PORTRAIT);
                    SclSize size_landscape = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_LANDSCAPE);
                    g_core.set_keyboard_size_hints(size_portrait, size_landscape);
                } else {
                    if (force_primary_latin) {
                        _language_manager.select_language(PRIMARY_LATIN_LANGUAGE, TRUE);
                    } else {
                        if (!(_language_manager.select_language(g_config_values.selected_language.c_str()))) {
                            _language_manager.select_language(PRIMARY_LATIN_LANGUAGE);
                        }
                    }
                }
                g_ui->set_cur_sublayout(g_ise_default_values[layout_index].sublayout_name);
            }
        }

        if (info) {
            if (info->accepts_caps_mode) {
                // FIXME this if condition means the AC is off
                if (g_keyboard_state.layout != ISE_LAYOUT_STYLE_NORMAL) {
                    g_ui->set_autocapital_shift_state(TRUE);
                    g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                }
                // normal layout means the AC is on
                else {
                    ise_send_event(MVK_Shift_Enable, KEY_MASK_NULL);
                    g_ui->set_autocapital_shift_state(FALSE);
                }
            } else {
                g_ui->set_autocapital_shift_state(TRUE);
                ise_send_event(MVK_Shift_Disable, KEY_MASK_NULL);
                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
        } else {
            g_ui->set_autocapital_shift_state(TRUE);
        }

        g_ui->show();
        g_ui->disable_input_events(FALSE);
    }

    g_candidate->show();
    g_keyboard_state.visible_state = TRUE;
#ifdef WAYLAND
    gLastIC = ic;
#endif
}

/**
 * Sets screen rotation
 */
void
ise_set_screen_rotation(int degree)
{
    if (g_ui) {
        g_ui->set_rotation(DEGREE_TO_SCLROTATION(degree));
    }
    if (g_candidate) {
        g_candidate->rotate(degree);
    }
}

void
ise_set_accessibility_state(bool state)
{
    if (g_ui) {
        g_ui->enable_tts(state);
    }
}

void
ise_hide()
{
    _click_count = 0;
    delete_commit_timer ();
    if (g_ui) {
        g_ui->disable_input_events(TRUE);
        g_ui->hide();
    }
    g_keyboard_state.visible_state = FALSE;
    if (g_candidate) {
        g_candidate->hide();
    }
}

void
ise_create()
{
    if (!g_ui) {
        g_ui = new CSCLUI;
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


    if (g_ui) {
        if (g_core.get_main_window()) {
            sclboolean succeeded = FALSE;

            const sclchar *resource_file_path = _language_manager.get_resource_file_path();

            if (resource_file_path) {
                if (strlen(resource_file_path) > 0) {
                    succeeded = g_ui->init(g_core.get_main_window(), scl_parser_type, resource_file_path);
                }
            }
            if (!succeeded) {
                g_ui->init(g_core.get_main_window(), scl_parser_type, MAIN_ENTRY_XML_PATH);
            }
            // FIXME whether to use global var, need to check
            if (g_candidate == NULL) {
                g_candidate = CandidateFactory::make_candidate(CANDIDATE_MULTILINE, g_core.get_main_window());
            }
            g_candidate->add_event_listener(&g_candidate_event_listener);
#ifndef WAYLAND
            g_ui->set_longkey_duration(elm_config_longpress_timeout_get() * 1000);
#endif

            /* Default ISE callback */
            g_ui->set_ui_event_callback(&callback);

            /* Accumulated customized ISE callbacks, depending on the input modes */
            for (scluint loop = 0;loop < _language_manager.get_languages_num();loop++) {
                LANGUAGE_INFO *language = _language_manager.get_language_info(loop);
                if (language) {
                    for (scluint inner_loop = 0;inner_loop < language->input_modes.size();inner_loop++) {
                        INPUT_MODE_INFO &info = language->input_modes.at(inner_loop);
                        LOGD("Registering callback for input mode %s : %p\n", info.name.c_str(), language->callback);
                        g_ui->set_ui_event_callback(language->callback, info.name.c_str());
                    }
                }
            }

            read_ise_config_values();
            _language_manager.set_enabled_languages(g_config_values.enabled_languages);
            _language_manager.select_language(g_config_values.selected_language.c_str());
            vconf_set_bool (VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, g_config_values.auto_capitalise);
            vconf_set_bool (VCONFKEY_AUTOPERIOD_ALLOW_BOOL, g_config_values.auto_punctuate);
            g_ui->enable_sound (g_config_values.sound_on);
            g_ui->enable_vibration (g_config_values.vibration_on);
            g_ui->enable_magnifier (g_config_values.preview_on);
        }

        SclSize size_portrait = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_PORTRAIT);
        SclSize size_landscape = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_LANDSCAPE);
        g_core.set_keyboard_size_hints(size_portrait, size_landscape);
    }
}

void
ise_destroy()
{
    if (g_ui) {
        LOGD("calling g_ui->fini()");
        g_ui->fini();
        LOGD("deleting g_ui");
        delete g_ui;
        g_ui = NULL;
    }
    if (g_candidate) {
        delete g_candidate;
        g_candidate = NULL;
    }
}

// when it is the time to auto_cap, the
// ise_set_caps_mode is called.
// -------------------------------------------------------
// For example: [How are you. Fine.], the
// auto-capital process is as below:
// Note: "["<--this is the beginning,
// "|"<--this is the cursor position
// 1) call ise_set_caps_mode, auto_cap = on
//    input: "H",
//    result: [H|
// 2) call ise_set_caps_mode, auto_cap = off
//    input: "o"
//    result: [Ho|
// 3) input: "w are you. "
//    result: [How are you. |
// 4) call ise_set_caps_mode, auto_cap = on
//    input: "F"
//    result: [How are you. F
// 5) input: "ine."
//    result: [How are you. Fine.|
// --------------------------------------------------------
// If we want to change the auto_cap, eg,
// if we want to input [How Are you.]
// Note the "Are" is not use auto-capital rule.
// we should use:
//    ise_send_event(MVK_Shift_On, SclCoreKeyMask_Null);
// when we are want to input "A"
// following input still has the auto_cap rule.
void
ise_set_caps_mode(unsigned int mode)
{
    if (mode) {
        g_keyboard_state.caps_mode = TRUE;
    } else {
        g_keyboard_state.caps_mode = FALSE;
    }
    LANGUAGE_INFO *info = _language_manager.get_language_info(_language_manager.get_current_language());
    if (info) {
        if (info->accepts_caps_mode) {
            set_caps_mode(g_keyboard_state.caps_mode);
        } else {
            g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
        }
    }
}

void
ise_update_cursor_position(int position)
{
    if (g_ui) {
        if (position > 0) {
            g_ui->set_string_substitution("www.", ".com");
        } else {
            g_ui->unset_string_substitution("www.");
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
    default:
        break;
    }

    if (type == ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT) {
        g_ui->unset_private_key("Enter");
    } else {
        static sclchar *imagelabel[SCL_BUTTON_STATE_MAX] = {
           const_cast<sclchar*>(" "), const_cast<sclchar*>(" "), const_cast<sclchar*>(" ")
        };

        g_ui->set_private_key("Enter", buf, imagelabel, NULL, 0, const_cast<sclchar*>("Enter"), TRUE);
    }
}

void ise_set_return_key_disable(unsigned int disabled)
{
    g_ui->enable_button("Enter", !disabled);
}

void ise_get_language_locale(char **locale)
{
    LANGUAGE_INFO *info = _language_manager.get_current_language_info();
    if(info) {
        if(!(info->locale_string.empty())) {
            *locale = strdup(info->locale_string.c_str());
        }
    }
}

void ise_update_table(const vector<string> &vec_str)
{
    if (g_candidate) {
        g_candidate->update(vec_str);
    }
}

sclboolean ise_process_key_event(const char *key)
{
    if (g_ui) {
        return g_ui->process_key_event(key);
    }
    return FALSE;
}
