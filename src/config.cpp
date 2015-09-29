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

#include <string>
#include <vector>
#include <sstream>
#include <iterator>

#include <sclcore.h>

#include "config.h"
#include "languages.h"

using namespace scl;

extern CSCLCore g_core;

CONFIG_VALUES::CONFIG_VALUES() {
    keypad_mode = KEYPAD_MODE_QTY; // keypad_mode
    prediction_on = FALSE; // prediction_on
    auto_capitalise = TRUE;
    auto_punctuate = TRUE;
    sound_on = TRUE;
    vibration_on = TRUE;
    preview_on = TRUE;
};

CONFIG_VALUES g_config_values;

void read_ise_config_values() {
    g_core.config_reload();
    sclint integer_value = 0;
    std::string string_value;

    integer_value = KEYPAD_MODE_QTY;
    g_core.config_read_int(ISE_CONFIG_KEYPAD_MODE, integer_value);
    g_config_values.keypad_mode = static_cast<KEYPAD_MODE>(integer_value);

    integer_value = 0;
    g_core.config_read_int(ISE_CONFIG_PREDICTION_ON, integer_value);
    g_config_values.prediction_on = integer_value;

    string_value = "";
    g_core.config_read_string(ISE_CONFIG_ENABLED_LANGUAGES, string_value);
    if (string_value.length () > 0) {
        std::stringstream ss(string_value);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
        g_config_values.enabled_languages = vstrings;
    }
    string_value = "";
    g_core.config_read_string(ISE_CONFIG_SELECTED_LANGUAGE, string_value);
    if (string_value.length () > 0) {
        g_config_values.selected_language = string_value;
    }

    integer_value = 1;
    g_core.config_read_int(ISE_CONFIG_AUTO_CAPITALISE, integer_value);
    g_config_values.auto_capitalise = integer_value;

    integer_value = 1;
    g_core.config_read_int(ISE_CONFIG_AUTO_PUNCTUATE, integer_value);
    g_config_values.auto_punctuate = integer_value;

    integer_value = 1;
    g_core.config_read_int(ISE_CONFIG_SOUND_ON, integer_value);
    g_config_values.sound_on = integer_value;

    integer_value = 1;
    g_core.config_read_int(ISE_CONFIG_VIBRATION_ON, integer_value);
    g_config_values.vibration_on = integer_value;

    integer_value = 1;
    g_core.config_read_int(ISE_CONFIG_PREVIEW_ON, integer_value);
    g_config_values.preview_on = integer_value;
#ifdef _TV
    g_config_values.enabled_languages.push_back ("English");
    g_config_values.enabled_languages.push_back ("Chinese");
    g_config_values.enabled_languages.push_back ("Korean");
    g_config_values.enabled_languages.push_back ("Japanese");
    g_config_values.enabled_languages.push_back ("Hongkong");
    g_config_values.selected_language = std::string ("English");
#endif
}

void write_ise_config_values() {
    std::string string_value;
    g_core.config_write_int(ISE_CONFIG_KEYPAD_MODE, g_config_values.keypad_mode);
    g_core.config_write_int(ISE_CONFIG_PREDICTION_ON, g_config_values.prediction_on);
    for(std::vector<std::string>::iterator it = g_config_values.enabled_languages.begin();
        it != g_config_values.enabled_languages.end();std::advance(it, 1)) {
            string_value += *it;
            string_value += " ";
    }
    g_core.config_write_string(ISE_CONFIG_ENABLED_LANGUAGES, string_value);
    g_core.config_write_string(ISE_CONFIG_SELECTED_LANGUAGE, g_config_values.selected_language);
    g_core.config_write_int(ISE_CONFIG_AUTO_CAPITALISE, g_config_values.auto_capitalise);
    g_core.config_write_int(ISE_CONFIG_AUTO_PUNCTUATE, g_config_values.auto_punctuate);
    g_core.config_write_int(ISE_CONFIG_SOUND_ON, g_config_values.sound_on);
    g_core.config_write_int(ISE_CONFIG_VIBRATION_ON, g_config_values.vibration_on);
    g_core.config_write_int(ISE_CONFIG_PREVIEW_ON, g_config_values.preview_on);
    g_core.config_flush();
    g_core.config_reload();
}

void erase_ise_config_values() {
    g_core.config_erase(ISE_CONFIG_KEYPAD_MODE);
    g_core.config_erase(ISE_CONFIG_PREDICTION_ON);
    g_core.config_erase(ISE_CONFIG_ENABLED_LANGUAGES);
    g_core.config_erase(ISE_CONFIG_SELECTED_LANGUAGE);
    g_core.config_erase(ISE_CONFIG_AUTO_CAPITALISE);
    g_core.config_erase(ISE_CONFIG_AUTO_PUNCTUATE);
    g_core.config_erase(ISE_CONFIG_SOUND_ON);
    g_core.config_erase(ISE_CONFIG_VIBRATION_ON);
    g_core.config_erase(ISE_CONFIG_PREVIEW_ON);
    g_core.config_reload();
}

void reset_ise_config_values () {
    g_config_values.keypad_mode = KEYPAD_MODE_QTY;
    g_config_values.prediction_on = FALSE;
    g_config_values.selected_language = "English";
    g_config_values.enabled_languages.clear ();
    g_config_values.enabled_languages.push_back ("English");
    g_config_values.auto_capitalise = TRUE;
    g_config_values.auto_punctuate = TRUE;
    g_config_values.sound_on = TRUE;
    g_config_values.vibration_on = TRUE;
    g_config_values.preview_on = TRUE;

    write_ise_config_values ();
}
