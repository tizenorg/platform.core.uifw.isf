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

#ifndef _LANGUAGES_H_
#define _LANGUAGES_H_

#include <string>
#include <vector>
#include <scl.h>

enum LANGUAGE_PRIORITY {
    LANGAUGE_PRIORITY_NONE,
    LANGAUGE_PRIORITY_DEFAULT,
    LANGUAGE_PRIORITY_SPECIALIZED,
};

struct ILanguageCallback : public scl::ISCLUIEventCallback {
    /* Default callback functions inherited from scl::ISCLUIEventCallback */
    /* FIXME scl::SCLEventReturnType */
    virtual SCLEventReturnType on_event_key_clicked(scl::SclUIEventDesc ui_event_desc) { return SCL_EVENT_PASS_ON; }
    virtual SCLEventReturnType on_event_drag_state_changed(scl::SclUIEventDesc ui_event_desc) { return SCL_EVENT_PASS_ON; }
    virtual SCLEventReturnType on_event_notification(SCLUINotiType noti_type, sclint etc_info) { return SCL_EVENT_PASS_ON; }

    /* Additional callback function, which is called when this language is selected */
    virtual sclboolean on_language_selected(const sclchar *language, const sclchar *input_mode) { return FALSE; }
    /* Additional callback function, which is called when this language is unselected */
    virtual sclboolean on_language_unselected(const sclchar *language, const sclchar *input_mode) { return FALSE; }
};

struct INPUT_MODE_INFO {
    /* The name of input mode, described in the name attribute of mode field in input_mode_configure.xml */
    std::string name;
    /* UTF-8 String that is used when displaying the name of this input mode, in option window */
    std::string display_name;
};
typedef std::vector<INPUT_MODE_INFO> INPUT_MODE_VECTOR;

typedef struct _LANGUAGE_INFO{
    _LANGUAGE_INFO() {
        enabled = false;
        enabled_temporarily = false;
        callback = NULL;
        priority = LANGAUGE_PRIORITY_NONE;
        is_latin_language = false;
        accepts_caps_mode = false;
    }
    /* Indicates whether this language was enabled in option window's language selection list */
    sclboolean enabled;
    /* Indicates whether this language was enabled temporarily */
    sclboolean enabled_temporarily;

    /* The name of this language, in English. This is used when calling the function select_language() */
    std::string name;
    /* UTF-8 String to display the name of this language, in the language itself */
    std::string display_name;
    /* The name of currently selected input mode, since we allow only 1 input mode selected per language */
    std::string selected_input_mode;

    /* Indicated what input modes are available in this language */
    INPUT_MODE_VECTOR input_modes;
    /* A function pointer to ILanguageCallback, that handles all kinds of events */
    ILanguageCallback *callback;

    /* Indicates whether this language information should overwrite all existing information on the same language */
    LANGUAGE_PRIORITY priority;

    /* If this language needs to load custom xml resource file, provide the resource file path here */
    std::string resource_file;

    /* If this language is set as latin language, the language itself will be shown when URL or EMAIL layouts are requested */
    sclboolean is_latin_language;
    /* If this language accepts caps mode, try to handle AutoCapitalization option */
    sclboolean accepts_caps_mode;
} LANGUAGE_INFO;


class ISELanguageManager {
public:
    sclboolean add_language(LANGUAGE_INFO language);

    sclboolean select_language(const sclchar *name, sclboolean temporarily = FALSE);
    sclboolean select_current_language();
    sclboolean select_next_language();
    sclboolean select_previous_language();

    sclboolean set_language_enabled(const sclchar *name, sclboolean enabled);
    sclboolean set_language_enabled_temporarily(const sclchar *name, sclboolean enabled_temporarily);
    /* if languages num is 0, enable default language, othewise enable languages assigned */
    sclboolean set_enabled_languages(const std::vector<std::string> &languages);
    sclboolean set_all_languages_enabled(sclboolean enabled);

    const sclchar* get_current_language();
    scluint get_languages_num();
    scluint get_enabled_languages_num();
    LANGUAGE_INFO* get_language_info(const sclchar *name);
    LANGUAGE_INFO* get_language_info(int index);

    LANGUAGE_INFO* get_current_language_info();
    const sclchar* get_resource_file_path();
private:
    /* enable languages, if languages num is 0, return false */
    sclboolean enable_languages(const std::vector<std::string> &languages);
    /* enable default language, regards the 1st language in the language_vector is the default language*/
    sclboolean enable_default_language();
    sclboolean do_select_language(int language_info_idx);
    sclboolean do_unselect_language(int language_info_idx);
};

#endif
