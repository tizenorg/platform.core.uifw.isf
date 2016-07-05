/*
 * Copyright (c) 2012 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
#include <Ecore_IMF.h>
#include <Elementary.h>

#include "sclui.h"
#include "sclcore.h"
#include "sclutils.h"
#include "ise.h"
#include "utils.h"
#include "option.h"
#include "languages.h"
#include "candidate-factory.h"
#include "ise-emoticon-mode.h"
#define CANDIDATE_WINDOW_HEIGHT 84
using namespace scl;
#include <vector>
using namespace std;

CSCLUI *g_ui = NULL;

#include <sclcore.h>

#include "ise.h"

static CCoreEventCallback g_core_event_callback;
CSCLCore g_core(&g_core_event_callback);
int g_imdata_state = 0;

extern emoticon_group_t current_emoticon_group;
extern std::vector <int> emoticon_list_recent;

extern CONFIG_VALUES g_config_values;
static sclboolean g_need_send_shift_event = FALSE;

extern void set_ise_imdata(const char * buf, size_t &len);
static void init_recent_used_punctuation();
static void update_recent_used_punctuation(const char *key_value);
static sclboolean g_punctuation_popup_opened = FALSE;
static sclboolean g_popup_opened = FALSE;
static vector<string> g_recent_used_punctuation;
static const int MAX_DEFAULT_PUNCTUATION = 6;
static string g_default_punctuation[MAX_DEFAULT_PUNCTUATION] = {"-", "@", "'", "!", "?", ","};
static string g_current_punctuation[MAX_DEFAULT_PUNCTUATION-1] = {"RCENT1", "RCENT2", "RCENT3", "RCENT4", "RCENT5"};
static vector<string> g_softcandidate_string;
static bool g_softcandidate_show = false;

#define SOFT_CANDIDATE_DELETE_TIME (1.0/100)
static Ecore_Timer *g_softcandidate_hide_timer = NULL;

static SCLKeyModifier g_prev_modifier;

KEYBOARD_STATE g_keyboard_state = {
    0,
    0,
    ISE_LAYOUT_STYLE_NORMAL,
    0,
    FALSE,
    TRUE,
    FALSE,
    ""
};

#define ISE_LAYOUT_NUMBERONLY_VARIATION_MAX 4
/*static const sclchar *_ise_numberonly_variation_name[ISE_LAYOUT_NUMBERONLY_VARIATION_MAX] = {
    "DEFAULT", "SIG", "DEC", "SIGDEC"
};*/

#define SIG_DEC_SIZE        2
static scluint              _click_count = 0;
static const char          *_sig_dec[SIG_DEC_SIZE] = {".", "-"};
static scluint              _sig_dec_event[SIG_DEC_SIZE] = {'.', '-'};
static Ecore_Timer         *_commit_timer = NULL;
static Candidate           *g_candidate = NULL;

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

#define CM_KEY_LIST_SIZE        2
#define USER_KEYSTRING_OPTION   "OPTION"
#define USER_KEYSTRING_EMOTICON "EMOTICON_LAYOUT"

static sclboolean           _cm_popup_opened = FALSE;
static const char          *_cm_key_list[CM_KEY_LIST_SIZE] = {USER_KEYSTRING_OPTION, USER_KEYSTRING_EMOTICON};
static scluint              _current_cm_key_id = 0;

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

static void _reset_shift_state(void)
{
    if (g_ui) {
        /* Reset all shift state variables */
        SCLShiftState old_shift_state = g_ui->get_shift_state();
        SCLShiftState new_shift_state = SCL_SHIFT_STATE_OFF;
        if (old_shift_state != new_shift_state) {
            g_need_send_shift_event = true;
            g_ui->set_shift_state(new_shift_state);
        }
        LOGD("Shift state changed from (%d) to (%d)\n", (int)old_shift_state, (int)new_shift_state);
    }
}

static void ise_set_cm_private_key(scluint cm_key_id)
{
    if (cm_key_id >= CM_KEY_LIST_SIZE || g_ui == NULL) {
        LOGE("cm_key_id=%d\n", cm_key_id);
        return;
    }

    if (strcmp(_cm_key_list[cm_key_id], USER_KEYSTRING_EMOTICON) == 0) {
        sclchar* imagelabel[SCL_BUTTON_STATE_MAX] = {
            const_cast<sclchar*>("emoticon/icon_emotion_nor_55x55.png"),
            const_cast<sclchar*>("emoticon/icon_emotion_press_55x55.png"),
            const_cast<sclchar*>("emoticon/icon_emotion_dim_55x55.png")};
        g_ui->set_private_key("CM_KEY", const_cast<sclchar*>(""), imagelabel, NULL, 0, const_cast<sclchar*>(USER_KEYSTRING_EMOTICON), TRUE);
    } else if (strcmp(_cm_key_list[cm_key_id], USER_KEYSTRING_OPTION) == 0) {
        sclchar* imagelabel[SCL_BUTTON_STATE_MAX] = {
            const_cast<sclchar*>("setting icon/B09_icon_setting_nor_54x54.png"),
            const_cast<sclchar*>("setting icon/B09_icon_setting_press_54x54.png"),
            const_cast<sclchar*>("setting icon/B09_icon_setting_dim_54x54.png")};
        g_ui->set_private_key("CM_KEY", const_cast<sclchar*>(""), imagelabel, NULL, 0, const_cast<sclchar*>(USER_KEYSTRING_OPTION), TRUE);
    }
}

static scluint ise_get_cm_key_id(const sclchar *key_value)
{
    for (int i = 0; i < CM_KEY_LIST_SIZE; ++i) {
        if (0 == strcmp (key_value, _cm_key_list[i]))
            return i;
    }
    return 0;
}

static bool ise_is_emoticons_disabled(void)
{
    bool ret = true;
    sclu32 current_layout = g_keyboard_state.layout;
    LOGD("layout=%d\n", current_layout);

    if ((current_layout == ISE_LAYOUT_STYLE_NORMAL) ||
        (current_layout == ISE_LAYOUT_STYLE_NUMBER) ||
        (current_layout == ISE_LAYOUT_STYLE_EMOTICON))
        ret = false;

    if (g_imdata_state & IMDATA_ACTION_DISABLE_EMOTICONS)
        ret = true;

    return ret;
}

static Eina_Bool softcandidate_hide_timer_callback(void *data)
{
    if (g_candidate) {
        g_candidate->hide();
        g_softcandidate_show = false;
    }
    return ECORE_CALLBACK_CANCEL;
}

static void delete_softcandidate_hide_timer(void)
{
    if (g_softcandidate_hide_timer) {
        ecore_timer_del(g_softcandidate_hide_timer);
        g_softcandidate_hide_timer = NULL;
    }
}

static void add_softcandidate_hide_timer(void)
{
    delete_softcandidate_hide_timer();
    g_softcandidate_hide_timer = ecore_timer_add(SOFT_CANDIDATE_DELETE_TIME, softcandidate_hide_timer_callback, NULL);
}

static void create_softcandidate(void)
{
    if (!g_candidate) {
        g_candidate = CandidateFactory::make_candidate(CANDIDATE_MULTILINE, g_core.get_main_window());
        if (g_candidate) {
            g_candidate->add_event_listener(&g_candidate_event_listener);
        }
    }
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
    LOGD("Enter\n");
    ise_focus_in(ic);
}

void CCoreEventCallback::on_focus_out(sclint ic, const sclchar *ic_uuid)
{
    LOGD("Enter\n");
    ise_focus_out(ic);
    g_imdata_state = 0;
}

void CCoreEventCallback::on_ise_show(sclint ic, const sclint degree, Ise_Context &context)
{
    LOGD("Enter\n");
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
        LOGD("Skipping rotation angle , %d\n", degree);
    }

    ::ise_show(ic);
}

void CCoreEventCallback::on_ise_hide(sclint ic, const sclchar *ic_uuid)
{
    LOGD("Enter\n");
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
    LOGD("state=%d\n", state);
    ise_set_accessibility_state(state);
}

void CCoreEventCallback::on_set_rotation_degree(sclint degree)
{
    ise_set_screen_rotation(degree);

    LOGD("degree=%d\n", degree);
    if (is_emoticon_show()) {
        ise_destroy_emoticon_window();
    }
    if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_EMOTICON) {
        ise_show_emoticon_window(current_emoticon_group, degree, false, g_core.get_main_window());
    } else if (g_ui) {
        const sclchar *input_mode = g_ui->get_input_mode();
        if (input_mode) {
            if (!(strcmp(input_mode, "EMOTICON_LAYOUT")))
                ise_show_emoticon_window(current_emoticon_group, degree, false, g_core.get_main_window());
        }
    }
}

void CCoreEventCallback::on_set_layout(sclu32 layout)
{
    LOGD("layout=%d\n", layout);
    /* Check if the layoutIdx is in the valid range */
    if (layout < ISE_LAYOUT_STYLE_MAX) {
        if (g_keyboard_state.layout != layout) {
            g_keyboard_state.need_reset = TRUE;
        }
        g_keyboard_state.layout = layout;
    }
    if (g_keyboard_state.visible_state)
        ise_show(g_keyboard_state.ic);
}

void CCoreEventCallback::on_set_caps_mode(sclu32 mode)
{
    ise_set_caps_mode(mode);
}

void CCoreEventCallback::on_update_cursor_position(sclint ic, const sclchar *ic_uuid, sclint cursor_pos)
{
    ise_update_cursor_position(cursor_pos);
}

void CCoreEventCallback::on_update_surrounding_text(sclint ic, const sclchar *text, sclint cursor)
{
    LOGD("surrounding text:%s, cursor=%d\n", text, cursor);
    g_core.delete_surrounding_text(-cursor, strlen(text));
}

void CCoreEventCallback::on_set_return_key_type(sclu32 type)
{
    LOGD("Enter\n");
    ise_set_return_key_type(type);

    if (g_keyboard_state.visible_state)
        ise_show(g_keyboard_state.ic);
}

void CCoreEventCallback::on_set_return_key_disable(sclu32 disabled)
{
    ise_set_return_key_disable(disabled);
}

void CCoreEventCallback::on_set_imdata(sclchar *buf, sclu32 len)
{
    LOGD("Enter\n");
    g_imdata_state = 0;
    size_t _len = len;
    set_ise_imdata(buf, _len);
}

void CCoreEventCallback::on_get_language_locale(sclint ic, sclchar **locale)
{
    ise_get_language_locale(locale);
}

void CCoreEventCallback::on_update_lookup_table(SclCandidateTable &table)
{
    g_softcandidate_string = table.candidate;
    ise_update_table(g_softcandidate_string);
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

void CCoreEventCallback::on_check_option_window_availability(sclboolean *ret)
{
    if (ret)
        *ret = true;   // Tizen keyboard (ise-default) has the option window, but 3rd party IME may not. This interface is for it.
}

/**
* This is only for the TV profile to handle remote control button
* It will be called in the TV profile by ISF logic.
*/
void CCoreEventCallback::on_process_key_event(scim::KeyEvent &key, sclu32 *ret)
{
    if (g_keyboard_state.visible_state)
        ise_process_key_event(key, *ret);
    else
        ret = FALSE;
}

void CCoreEventCallback::on_candidate_show(sclint ic, const sclchar *ic_uuid)
{
    delete_softcandidate_hide_timer();

    create_softcandidate();

    if (g_candidate)
        g_candidate->show();

    g_softcandidate_show = true;
}

void CCoreEventCallback::on_candidate_hide(sclint ic, const sclchar *ic_uuid)
{
    add_softcandidate_hide_timer();
}

void CCoreEventCallback::on_process_input_device_event(sclu32 &type, sclchar *data, size_t &len, sclu32 *ret)
{
    LOGD("type:%d, len:%d\n", type, len);
    if (ret == NULL) {
        LOGE("ret is NULL\n");
        return;
    }
    *ret = 0;
#ifdef _WEARABLE
    if (type == (sclu32)(ECORE_EVENT_DETENT_ROTATE)) {
        void *event_data = static_cast<void*>(data);
        Ecore_Event_Detent_Rotate *rotary_device_event = static_cast<Ecore_Event_Detent_Rotate*>(event_data);
        if (rotary_device_event) {
            sclu32 new_layout = g_keyboard_state.layout;
            if (rotary_device_event->direction == ECORE_DETENT_DIRECTION_CLOCKWISE) {
                LOGD("CLOCKWISE\n");
                switch (g_keyboard_state.layout) {
                    case ISE_LAYOUT_STYLE_NORMAL:
                    case ISE_LAYOUT_STYLE_EMAIL:
                    case ISE_LAYOUT_STYLE_URL:
                    case ISE_LAYOUT_STYLE_PASSWORD:
                        new_layout = ISE_LAYOUT_STYLE_NUMBER;
                        break;
                    case ISE_LAYOUT_STYLE_NUMBER:
                        new_layout = ISE_LAYOUT_STYLE_HEX;
                        break;
                    case ISE_LAYOUT_STYLE_HEX:
                        new_layout = ISE_LAYOUT_STYLE_EMOTICON;
                        break;
                    case ISE_LAYOUT_STYLE_EMOTICON:
                        new_layout = ISE_LAYOUT_STYLE_TERMINAL;
                        break;
                    case ISE_LAYOUT_STYLE_TERMINAL:
                        new_layout = ISE_LAYOUT_STYLE_NORMAL;
                        break;
                    default:
                        ;
                }
            } else if (rotary_device_event->direction == ECORE_DETENT_DIRECTION_COUNTER_CLOCKWISE) {
                LOGD("COUNTER_CLOCKWISE\n");
                switch (g_keyboard_state.layout) {
                    case ISE_LAYOUT_STYLE_NORMAL:
                    case ISE_LAYOUT_STYLE_EMAIL:
                    case ISE_LAYOUT_STYLE_URL:
                    case ISE_LAYOUT_STYLE_PASSWORD:
                        new_layout = ISE_LAYOUT_STYLE_TERMINAL;
                        break;
                    case ISE_LAYOUT_STYLE_NUMBER:
                        new_layout = ISE_LAYOUT_STYLE_NORMAL;
                        break;
                    case ISE_LAYOUT_STYLE_HEX:
                        new_layout = ISE_LAYOUT_STYLE_NUMBER;
                        break;
                    case ISE_LAYOUT_STYLE_EMOTICON:
                        new_layout = ISE_LAYOUT_STYLE_HEX;
                        break;
                    case ISE_LAYOUT_STYLE_TERMINAL:
                        new_layout = ISE_LAYOUT_STYLE_EMOTICON;
                        break;
                    default:
                        ;
                }
            }
            if (new_layout != g_keyboard_state.layout) {
                on_set_layout(new_layout);
                *ret = 1;
            }
        }
    }
#endif
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
    LOGD("ic : %x, %s\n", ic, key_value);
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
    LOGD("ic : %x, %s\n", ic, str);
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
    LOGD("ic : %x, %x\n", ic, key_event);
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
    LOGD("ic : %x, %x\n", ic, key_event);
}

static void set_caps_mode(sclboolean mode) {
    if (g_ui) {
        if (g_ui->get_shift_state() != SCL_SHIFT_STATE_LOCK) {
            g_ui->set_shift_state(mode ? SCL_SHIFT_STATE_ON : SCL_SHIFT_STATE_OFF);
            g_ui->set_autocapital_shift_state(!mode);
        }
    }
}

/**
 * @brief Delete commit timer.
 *
 * @return void
 */
static void delete_commit_timer(void)
{
    if (_commit_timer != NULL) {
        ecore_timer_del(_commit_timer);
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
static Eina_Bool commit_timeout(void *data)
{
    if (_commit_timer != NULL) {
        g_core.hide_preedit_string(-1, "");
        ise_forward_key_event(_sig_dec_event[(_click_count-1)%SIG_DEC_SIZE]);
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

        const sclchar * cur_lang = _language_manager.get_current_language();
        if (cur_lang) {
            LANGUAGE_INFO *info = _language_manager.get_language_info(cur_lang);
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
        if (is_emoticon_show()) {
            ise_destroy_emoticon_window();
        }
        if (key_value) {
            if (!strcmp(key_value, USER_KEYSTRING_EMOTICON)) {
                ise_init_emoticon_list();
#ifdef _WEARABLE
                    current_emoticon_group = EMOTICON_GROUP_1;
#else
                if (emoticon_list_recent.size() == 0)
                    current_emoticon_group = EMOTICON_GROUP_1;
                else
                    current_emoticon_group = EMOTICON_GROUP_RECENTLY_USED;
#endif
                SCLRotation rotation = g_ui->get_rotation();
                ise_show_emoticon_window(current_emoticon_group, ROTATION_TO_DEGREE(rotation), false, g_core.get_main_window());
            }
        }
    }

    return ret;
}


SCLEventReturnType CUIEventCallback::on_event_notification(SCLUINotiType noti_type, SclNotiDesc *etc_info)
{
    SCLEventReturnType ret = SCL_EVENT_PASS_ON;
    LOGD("noti type: %d, g_need_send_shift_event: %d\n", noti_type, g_need_send_shift_event);

    if (noti_type == SCL_UINOTITYPE_SHIFT_STATE_CHANGE) {
        if (g_need_send_shift_event) {
            const sclchar * cur_lang = _language_manager.get_current_language();
            if (cur_lang) {
                LANGUAGE_INFO *info = _language_manager.get_language_info(cur_lang);
                SclNotiShiftStateChangeDesc *desc = static_cast<SclNotiShiftStateChangeDesc*>(etc_info);
                if (info && desc) {
                    if (info->accepts_caps_mode) {
                        LOGD("shift state: %d\n", desc->shift_state);
                        if (desc->shift_state == SCL_SHIFT_STATE_OFF) {
                            ise_send_event(MVK_Shift_Off, KEY_MASK_NULL);
                        } else if (desc->shift_state == SCL_SHIFT_STATE_ON) {
                            ise_send_event(MVK_Shift_On, KEY_MASK_NULL);
                        } else if (desc->shift_state == SCL_SHIFT_STATE_LOCK) {
                            ise_send_event(MVK_Shift_Lock, KEY_MASK_NULL);
                        }
                        ret = SCL_EVENT_PASS_ON;
                    }
                }
            }
            g_need_send_shift_event = FALSE;
        }
    } else if (noti_type == SCL_UINOTITYPE_POPUP_OPENING) {
        vector<string>::reverse_iterator iter = g_recent_used_punctuation.rbegin();
        int punc_pos = 0;
        for (; iter != g_recent_used_punctuation.rend(); ++iter)
        {
            g_ui->set_string_substitution(g_current_punctuation[punc_pos].c_str(), iter->c_str());
            punc_pos++;
        }
        SclNotiPopupOpeningDesc *openingDesc = (SclNotiPopupOpeningDesc *)etc_info;
        if (0 == strcmp(openingDesc->input_mode, "CM_POPUP")) {
            if (ise_is_emoticons_disabled())
                g_ui->enable_button("EMOTICON_KEY", false);
            else
                g_ui->enable_button("EMOTICON_KEY", true);
        }
    } else if (noti_type == SCL_UINOTITYPE_POPUP_OPENED) {
        g_popup_opened = TRUE;
        SclNotiPopupOpenedDesc *openedDesc = (SclNotiPopupOpenedDesc *)etc_info;
        if (0 == strcmp(openedDesc->input_mode, "PUNCTUATION_POPUP")) {
            g_punctuation_popup_opened = TRUE;
        } else if (0 == strcmp(openedDesc->input_mode, "CM_POPUP")) {
            _cm_popup_opened = TRUE;
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

    if (event_desc.key_modifier == KEY_MODIFIER_MULTITAP_START) {
        if (!g_keyboard_state.multitap_value.empty()) {
            ise_send_string(g_keyboard_state.multitap_value.c_str());
        }
        ise_update_preedit_string(event_desc.key_value);
        g_keyboard_state.multitap_value = event_desc.key_value;
    } else if (event_desc.key_modifier == KEY_MODIFIER_MULTITAP_REPEAT) {
        ise_update_preedit_string(event_desc.key_value);
        g_keyboard_state.multitap_value = event_desc.key_value;
    } else if (g_prev_modifier == KEY_MODIFIER_MULTITAP_START ||
            g_prev_modifier == KEY_MODIFIER_MULTITAP_REPEAT) {
        ise_send_string(g_keyboard_state.multitap_value.c_str());
        ise_update_preedit_string("");
        g_keyboard_state.multitap_value = "";
    }
    g_prev_modifier = event_desc.key_modifier;

    if (g_ui) {
        switch (event_desc.key_type) {
        case KEY_TYPE_STRING: {
            if (event_desc.key_modifier != KEY_MODIFIER_MULTITAP_START &&
                event_desc.key_modifier != KEY_MODIFIER_MULTITAP_REPEAT) {
                    if (event_desc.key_event) {
                        ise_forward_key_event(event_desc.key_event);
                    } else {
                        ise_send_string(event_desc.key_value);
                    }
            }
            if (!g_popup_opened) {
                const sclchar *input_mode = g_ui->get_input_mode();
                if (input_mode && ((0 == strcmp(input_mode, "SYM_QTY_1")) || (0 == strcmp(input_mode, "SYM_QTY_2")))) {
                    update_recent_used_punctuation(event_desc.key_value);
                }
            } else if (g_punctuation_popup_opened) {
                update_recent_used_punctuation(event_desc.key_value);
                g_punctuation_popup_opened = FALSE;
                g_popup_opened = FALSE;
            }
            break;
        }
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
                    strcmp(event_desc.key_value, ".") == 0) {
                    g_core.update_preedit_string(-1, "", _sig_dec[_click_count%SIG_DEC_SIZE]);
                    g_core.show_preedit_string(-1, "");
                    delete_commit_timer();
                    _commit_timer = ecore_timer_add(1.0, commit_timeout, NULL);
                    _click_count++;
                } else if (event_desc.key_event) {
                    commit_timeout(NULL);
                    if (need_forward) {
                        ise_forward_key_event(event_desc.key_event);
                    } else {
                        ise_send_event(event_desc.key_event, KEY_MASK_NULL);
                    }
                }
                if (input_mode) {
                    if ((strcmp(input_mode, "SYM_QTY_1") == 0) || (0 == strcmp(input_mode, "SYM_QTY_2"))) {
                        update_recent_used_punctuation(event_desc.key_value);
                    }
                }
                break;
            }
        case KEY_TYPE_CONTROL: {
                commit_timeout(NULL);
                if (event_desc.key_event) {
                    const char *long_shift = "LongShift";
                    const char *caps_lock = "CapsLock";
                    const char *delete_all = "DeleteAll";
                    const char *hide_panel = "Hide";
                    if (strncmp(event_desc.key_value, long_shift, strlen(long_shift)) == 0) {
                        LOGD("shift key is longpress\n");
                        g_ui->set_shift_state(SCL_SHIFT_STATE_ON);
                        g_need_send_shift_event = TRUE;
                        //ise_send_event (MVK_Shift_Lock, KEY_MASK_NULL);
                    } else if (strncmp(event_desc.key_value, caps_lock, strlen(caps_lock)) == 0) {
                        if (g_ui->get_shift_state() != SCL_SHIFT_STATE_LOCK) {
                            g_ui->set_shift_state(SCL_SHIFT_STATE_LOCK);
                            ise_send_event(MVK_Shift_Lock, KEY_MASK_NULL);
                        } else {
                            g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                            ise_send_event(MVK_Shift_Off, KEY_MASK_NULL);
                        }
                        //g_need_send_shift_event = TRUE;
                    } else if (strncmp(event_desc.key_value, delete_all, strlen(delete_all)) == 0) {
                        g_core.get_surrounding_text(NULL, -1, -1);
                    } else if (strncmp(event_desc.key_value, hide_panel, strlen(hide_panel)) == 0) {
                        ::ise_hide();
                    } else {
                        ise_send_event(event_desc.key_event, KEY_MASK_NULL);
                        if (event_desc.key_event == MVK_Shift_L) {
                            g_need_send_shift_event = TRUE;
                        }
                    }
                }
                break;
           }
        case KEY_TYPE_MODECHANGE:
            if (strcmp(event_desc.key_value, USER_KEYSTRING_OPTION) == 0) {
                if (!option_window_is_available (OPTION_WINDOW_TYPE_NORMAL))
                    g_core.create_option_window();
                ret = SCL_EVENT_DONE;
            } else if (on_input_mode_changed(event_desc.key_value, event_desc.key_event, event_desc.key_type)) {
                ret = SCL_EVENT_DONE;
            }
            if (_cm_popup_opened) {
                if (strcmp(event_desc.key_value, USER_KEYSTRING_EMOTICON) == 0) {
                    scluint id = ise_get_cm_key_id(event_desc.key_value);
                    if (id != _current_cm_key_id) {
                        _current_cm_key_id = id;
                        ise_set_cm_private_key(_current_cm_key_id);
                    }
                }
                _cm_popup_opened = FALSE;
            }
            break;
        case KEY_TYPE_USER:
            if (strcmp(event_desc.key_value, USER_KEYSTRING_OPTION) == 0) {
                //open_option_window(NULL, ROTATION_TO_DEGREE(g_ui->get_rotation()));
                if (!option_window_is_available (OPTION_WINDOW_TYPE_NORMAL))
                    g_core.create_option_window();
                ret = SCL_EVENT_DONE;
            } else {
                const sclchar *input_mode = g_ui->get_input_mode();
                if ((NULL != input_mode) && (!strcmp(input_mode, "EMOTICON_LAYOUT")))
                {
                    if (is_emoticon_show())
                    {
                        ise_destroy_emoticon_window();
                    }
#ifdef _WEARABLE
                    emoticon_group_t group_id = EMOTICON_GROUP_1;
                    if (current_emoticon_group < EMOTICON_GROUP_3)
                        group_id = (emoticon_group_t)(current_emoticon_group + 1);

                    const int BUF_LEN = 16;
                    char buf[BUF_LEN] = {0};
                    snprintf(buf, BUF_LEN, "%d/3", group_id);
                    static sclchar *imagelabel[SCL_BUTTON_STATE_MAX] = {
                           const_cast<sclchar*>(" "), const_cast<sclchar*>(" "), const_cast<sclchar*>(" ")};
                    g_ui->set_private_key("EMOTICON_GROUP_ID", buf, imagelabel, NULL, 0, const_cast<sclchar*>("EMOTICON_GROUP_NEXT"), TRUE);
#else
                    emoticon_group_t group_id = ise_get_emoticon_group_id(event_desc.key_value);
#endif
                    if ((group_id >= 0) && (group_id < MAX_EMOTICON_GROUP))
                    {
                        SCLRotation rotation = g_ui->get_rotation();
                        ise_show_emoticon_window(group_id, ROTATION_TO_DEGREE(rotation), false, g_core.get_main_window());
                    }
                }
            }
            if (_cm_popup_opened) {
                if (strcmp(event_desc.key_value, USER_KEYSTRING_OPTION) == 0) {
                    scluint id = ise_get_cm_key_id(USER_KEYSTRING_OPTION);
                    if (id != _current_cm_key_id) {
                        _current_cm_key_id = id;
                        ise_set_cm_private_key(_current_cm_key_id);
                    }
                }
                _cm_popup_opened = FALSE;
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
        LOGD("layout:%d, variation:%d\n", g_keyboard_state.layout, g_keyboard_state.layout_variation);
    }
}

void
ise_reset_context()
{
    g_keyboard_state.multitap_value = "";
    g_prev_modifier = KEY_MODIFIER_NONE;
}

void
ise_reset_input_context()
{
    g_keyboard_state.multitap_value = "";
    g_prev_modifier = KEY_MODIFIER_NONE;
}

void
ise_focus_in(int ic)
{
    LOGD("ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x\n", ic, check_ic_temporary(ic),
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
    LOGD("attaching, ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x\n", ic, check_ic_temporary(ic),
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

        LOGD("ic : %x , %x , g_ic : %x , %x, g_focused_ic : %x , %x\n", ic, check_ic_temporary(ic),
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
        const sclchar * cur_lang = _language_manager.get_current_language();
        if (cur_lang) {
            if (g_config_values.selected_language.compare(cur_lang) != 0) {
                reset_inputmode = TRUE;
            }
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

                LOGD("new layout index : %d\n", layout_index);
                const sclchar *old_input_mode = g_ui->get_input_mode();
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
                if (is_emoticon_show()) {
                    ise_destroy_emoticon_window();
                }

                if (g_keyboard_state.layout == ISE_LAYOUT_STYLE_EMOTICON) {
                    ise_init_emoticon_list();
#ifdef _WEARABLE
                    current_emoticon_group = EMOTICON_GROUP_1;
#else
                    if (emoticon_list_recent.size() == 0)
                        current_emoticon_group = EMOTICON_GROUP_1;
                    else
                        current_emoticon_group = EMOTICON_GROUP_RECENTLY_USED;
#endif
                    SCLRotation rotation = g_ui->get_rotation();
                    ise_show_emoticon_window(current_emoticon_group, ROTATION_TO_DEGREE(rotation), false, g_core.get_main_window());
                }
                // Check whether inout panel geometry is changed.
                const sclchar *new_input_mode = g_ui->get_input_mode();
                if (g_keyboard_state.visible_state && old_input_mode && new_input_mode && strcmp(old_input_mode, new_input_mode)) {
                    SclSize new_portrait = g_ui->get_input_mode_size(new_input_mode, DISPLAYMODE_PORTRAIT);
                    SclSize new_landscape = g_ui->get_input_mode_size(new_input_mode, DISPLAYMODE_LANDSCAPE);
                    LOGD("old input mode:%s, new input mode:%s, portrait(%d, %d), landscape(%d, %d)\n",
                        old_input_mode, new_input_mode, new_portrait.width, new_portrait.height, new_landscape.width, new_landscape.height);

                    sclint width = 0;
                    sclint height = 0;
                    g_ui->get_screen_resolution(&width, &height);

                    SclSize old_size = {0, 0};
                    SCLRotation rotation = g_ui->get_rotation();
                    if (rotation == ROTATION_90_CW || rotation == ROTATION_90_CCW) {
                        old_size = g_ui->get_input_mode_size(old_input_mode, DISPLAYMODE_LANDSCAPE);
                        if (old_size.width != new_landscape.width || old_size.height != new_landscape.height)
                            g_core.update_geometry(0, 0, new_landscape.width, new_landscape.height);
                    } else {
                        old_size = g_ui->get_input_mode_size(old_input_mode, DISPLAYMODE_PORTRAIT);
                        if (old_size.width != new_portrait.width || old_size.height != new_portrait.height)
                            g_core.update_geometry(0, height - new_portrait.height, new_portrait.width, new_portrait.height);
                    }
                }
            }
        }

        if (info) {
            if (info->accepts_caps_mode) {
                // FIXME this if condition means the AC is off
                if (g_keyboard_state.layout != ISE_LAYOUT_STYLE_NORMAL) {
                    g_ui->set_autocapital_shift_state(TRUE);
                    g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
                } else {
                    ise_send_event(MVK_Shift_Enable, KEY_MASK_NULL);
                    // Auto Capital is supported only in normal layout
                    if (g_keyboard_state.caps_mode) {
                        g_ui->set_autocapital_shift_state(FALSE);
                    }
                }
            } else {
                g_ui->set_autocapital_shift_state(TRUE);
                ise_send_event(MVK_Shift_Disable, KEY_MASK_NULL);
                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
        } else {
            g_ui->set_autocapital_shift_state(TRUE);
        }

        // Update CM key button
        if (strcmp(_cm_key_list[_current_cm_key_id], USER_KEYSTRING_EMOTICON) == 0) {
            if (ise_is_emoticons_disabled())
                ise_set_cm_private_key(ise_get_cm_key_id(USER_KEYSTRING_OPTION));
            else
                ise_set_cm_private_key(_current_cm_key_id);
        }

        g_ui->show();
        g_ui->disable_input_events(FALSE);
    }

    g_keyboard_state.visible_state = TRUE;
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
        if (g_softcandidate_show) {
            g_candidate->update(g_softcandidate_string);
        }
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
    delete_commit_timer();

    if (g_ui) {
        g_ui->disable_input_events(TRUE);
        g_ui->hide();
    }

    g_keyboard_state.visible_state = FALSE;

    if (g_candidate) {
        g_candidate->hide();
    }

    _reset_shift_state();
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

            const sclchar *entry_path = MAIN_ENTRY_XML_PATH;
            int nwidth  = 0;
            int nheight = 0;
            CSCLUtils *utils = CSCLUtils::get_instance();
            if (utils) {
                utils->get_screen_resolution(&nwidth, &nheight);
            }
#ifdef _MOBILE
            if ((480 == nwidth) && (800 == nheight)) {
                entry_path = MAIN_ENTRY_XML_PATH_480X800;
            } else if ((540 == nwidth) && (960 == nheight)) {
                entry_path = MAIN_ENTRY_XML_PATH_540X960;
            } else if ((1440 == nwidth) && (2560 == nheight)) {
                entry_path = MAIN_ENTRY_XML_PATH_1440X2560;
            }
#endif
            _language_manager.set_resource_file_path(entry_path);
            const sclchar *resource_file_path = _language_manager.get_resource_file_path();

            if (resource_file_path) {
                if (strlen(resource_file_path) > 0) {
                    succeeded = g_ui->init(g_core.get_main_window(), scl_parser_type, resource_file_path);
                }
            }
            if (!succeeded) {
                g_ui->init(g_core.get_main_window(), scl_parser_type, MAIN_ENTRY_XML_PATH);
            }

            g_core.enable_soft_candidate(true);

            g_ui->set_longkey_duration(elm_config_longpress_timeout_get() * 1000);

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
            vconf_set_bool(VCONFKEY_AUTOCAPITAL_ALLOW_BOOL, g_config_values.auto_capitalise);
            vconf_set_bool(VCONFKEY_AUTOPERIOD_ALLOW_BOOL, g_config_values.auto_punctuate);
            g_ui->enable_sound(g_config_values.sound_on);
            g_ui->enable_vibration(g_config_values.vibration_on);
            g_ui->enable_magnifier(g_config_values.preview_on);
        }

        SclSize size_portrait = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_PORTRAIT);
        SclSize size_landscape = g_ui->get_input_mode_size(g_ui->get_input_mode(), DISPLAYMODE_LANDSCAPE);
        g_core.set_keyboard_size_hints(size_portrait, size_landscape);
    }
    init_recent_used_punctuation();
}

void
ise_destroy()
{
    if (g_ui) {
        LOGD("calling g_ui->fini()\n");
        g_ui->fini();
        LOGD("deleting g_ui\n");
        delete g_ui;
        g_ui = NULL;
    }

    if (g_candidate) {
        delete g_candidate;
        g_candidate = NULL;
    }

    /* This is necessary. If this is not called, 3rd party IME might have auto period input regardless its settings */
    vconf_set_bool(VCONFKEY_AUTOPERIOD_ALLOW_BOOL, false);
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
    const sclchar * cur_lang = _language_manager.get_current_language();
    if (cur_lang) {
        LANGUAGE_INFO *info = _language_manager.get_language_info(cur_lang);
        if (info) {
            if (info->accepts_caps_mode) {
                set_caps_mode(g_keyboard_state.caps_mode);
            } else {
                g_ui->set_shift_state(SCL_SHIFT_STATE_OFF);
            }
        }
    }
}

void
ise_update_cursor_position(int position)
{
    if (g_ui) {
#ifndef _TV
        if (position > 0) {
            g_ui->set_string_substitution("www.", ".com");
        } else {
            g_ui->unset_string_substitution("www.");
        }
#endif
    }
}

void ise_set_return_key_type(unsigned int type)
{
    const int BUF_LEN = 64;
    char buf[BUF_LEN] = {0};

    LOGD("return key type : %d\n", type);
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
        LOGD("return key lable : %s\n", buf);
    }
}

void ise_set_return_key_disable(unsigned int disabled)
{
    LOGD("enable : %d\n", !disabled);
    g_ui->enable_button("Enter", !disabled);
}

void ise_get_language_locale(char **locale)
{
    LANGUAGE_INFO *info = _language_manager.get_current_language_info();
    if (info) {
        if (!(info->locale_string.empty())) {
            *locale = strdup(info->locale_string.c_str());
        }
    }
}

void ise_update_table(const vector<string> &vec_str)
{
    create_softcandidate();

    if (g_candidate) {
        g_candidate->update(vec_str);
    }
}

void ise_process_key_event(scim::KeyEvent& key, sclu32 &ret)
{
    ret = 0;
    if (g_ui) {
        ret = g_ui->process_key_event(key.get_key_string().c_str());
    }
}

static void init_recent_used_punctuation()
{
    if (g_recent_used_punctuation.empty())
    {
        g_recent_used_punctuation.push_back("#");
        g_recent_used_punctuation.push_back("$");
        g_recent_used_punctuation.push_back("%");
        g_recent_used_punctuation.push_back("^");
        g_recent_used_punctuation.push_back("&");
    }
}

static void update_recent_used_punctuation(const char * key_value)
{
    if (NULL == key_value)
    {
        return;
    }
    for (int i = 0; i < 10; ++i)
    {
        char buf[5] = {0};
        snprintf(buf, sizeof(buf), "%d", i);
        if (strcmp(key_value, buf) == 0)
        {
            return;
        }
    }
    string strKey = string(key_value);
    for (int i = 0; i < MAX_DEFAULT_PUNCTUATION; ++i)
    {
        if (0 == strKey.compare(g_default_punctuation[i].c_str()))
        {
            return;
        }
    }
    vector<string>::iterator iter = g_recent_used_punctuation.begin();
    for (; iter != g_recent_used_punctuation.end(); ++iter)
    {
        if (0 == strKey.compare(iter->c_str()))
        {
            break;
        }
    }
    if (iter != g_recent_used_punctuation.end())
    {
        g_recent_used_punctuation.erase(iter);
    }
    g_recent_used_punctuation.push_back(strKey);
    if (g_recent_used_punctuation.size() > MAX_DEFAULT_PUNCTUATION-1)
    {
        g_recent_used_punctuation.erase(g_recent_used_punctuation.begin());
    }
}
