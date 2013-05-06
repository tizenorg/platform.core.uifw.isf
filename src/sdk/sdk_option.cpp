/*
 * Copyright 2012-2013 Samsung Electronics Co., Ltd.
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

#include "sdk.h"
#include "option.h"
#include "ise_lang_table.h"
using namespace scl;

static ISELanguageManager _language_manager;
/*
 * This class customizes option windows based on languages
 * So you should perform desired tasks in this class.
 */
class CSDKOption : public ILanguageOption
{
public :
    CSDKOption() {
        /* Provide language information only if we are builing SetupModule... */
#ifdef SETUP_MODULE
        sclint loop;
        for (loop = 0;loop < get_lang_table_size();loop++) {
            INPUT_MODE_INFO input_mode_QTY;
            input_mode_QTY.name = get_lang_table()[loop].inputmode_QTY;
            input_mode_QTY.display_name = get_lang_table()[loop].inputmode_QTY_name;

            LANGUAGE_INFO language;
            language.name = get_lang_table()[loop].language;
            language.display_name = get_lang_table()[loop].language_name;
            language.callback = NULL;
            language.input_modes.push_back(input_mode_QTY);
            language.priority = LANGAUGE_PRIORITY_DEFAULT;
            language.resource_file = MAIN_ENTRY_XML_PATH;
            language.is_latin_language = get_lang_table()[loop].is_latin_language;
            language.accepts_caps_mode = get_lang_table()[loop].accepts_caps_mode;

            /* These variable should be read from stored setting values */
            language.enabled = TRUE;
            language.selected_input_mode = input_mode_QTY.name;

            _language_manager.add_language(language);

            printf("Adding Language : %s\n", get_lang_table()[loop].language);
        }
#endif // SETUP_MODULE

        LanguageOptionManager::add_language_option(this);
    }
    void on_create_option_main_view(Evas_Object *genlist, Evas_Object *naviframe);
    void on_destroy_option_main_view();
};

static CSDKOption ise_option_instance;

void CSDKOption::on_create_option_main_view(Evas_Object *genlist, Evas_Object *naviframe)
{

}

void CSDKOption::on_destroy_option_main_view()
{

}
