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

#ifndef __ISE_LANG_TABLE_H
#define __ISE_LANG_TABLE_H
#define MAX_LANG_TABLE_SIZE 128

typedef struct SDK_ISE_LANG_TABLE {
    SDK_ISE_LANG_TABLE() {
        language = NULL;
        language_name = NULL;
        locale_string = NULL;
        inputmode_QTY = NULL;
        inputmode_QTY_name = NULL;
        keyboard_ise_uuid = NULL;
        main_keyboard_name = NULL;
        country_code_URL = NULL;
        language_code = 0;
        language_command = 0;
        flush_code = 0;
        flush_command = 0;
        is_latin_language = false;
        accepts_caps_mode = false;
    }
    /* This is the string ID of this language, used in ISE side. For example, ISE developer can use this string to select this language, by passing "Korean" to ISELanguageManager::select_language() function */
    char *language;
    /* The translated UTF8 string of this language in its own language. */
    char *language_name;
    /* A locale string such as 'en_US' to let the application know what language the keyboard currently selected */
    char *locale_string;

    /* The QWERTY input mode of this language. This is used for passing as parameter of SCLUI::set_input_mode(), so has to be same with the name in input_mode_configure.xml file's mode name */
    char *inputmode_QTY;
    /* The translate UTF8 string that will be displayed in the option window's language selection. */
    char *inputmode_QTY_name;

    /* The keyboard ISE's uuid, since each language might have to use different IMEngines. ise-engine-hangul for korean, ise-engine-sunpinyin for chinese,... */
    char *keyboard_ise_uuid;

    /* Click "?123" button, ISE will show SYM keyboard, and the "?123" button changes the label to main_keyboard_name,
     * then click on the button will back to main_keyboard
     * the main_keyboard_name may be "abc" in English, "¿" in Chinese and so on
     */
    char *main_keyboard_name;

    /* URL country code */
    char *country_code_URL;

    /* The language parameter of this keyboard ise's language change command */
    int language_code;
    /* The language change command that this keyboard ise uses */
    int language_command;
    /* The flush parameter of this keyboard ise's flush command */
    int flush_code;
    /* The flush change command that this keyboard ise uses */
    int flush_command;

    /* If this language is set as latin language, the language itself will be shown when URL or EMAIL layouts are requested */
    bool is_latin_language;
    /* If this language accepts caps mode, try to handle AutoCapitalization option */
    bool accepts_caps_mode;
}SDK_ISE_LANG_TABLE;


SDK_ISE_LANG_TABLE* get_lang_table();
int get_lang_table_size();
#endif
