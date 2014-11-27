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
};

CONFIG_VALUES g_config_values;

void read_ise_config_values() {
    g_core.config_reload();
    sclint integer_value;
    std::string string_value;

    if (g_core.config_read_int(ISE_CONFIG_KEYPAD_MODE, integer_value)) {
        g_config_values.keypad_mode = static_cast<KEYPAD_MODE>(integer_value);
    }
    if (g_core.config_read_int(ISE_CONFIG_PREDICTION_ON, integer_value)) {
        g_config_values.prediction_on = integer_value;
    }
    if (g_core.config_read_string(ISE_CONFIG_ENABLED_LANGUAGES, string_value)) {
        std::stringstream ss(string_value);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);
        g_config_values.enabled_languages = vstrings;
    }
    if (g_core.config_read_string(ISE_CONFIG_SELECTED_LANGUAGE, string_value)) {
        g_config_values.selected_language = string_value;
    }
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
    g_core.config_reload();
}

void erase_ise_config_values() {
    g_core.config_erase(ISE_CONFIG_KEYPAD_MODE);
    g_core.config_erase(ISE_CONFIG_PREDICTION_ON);
    g_core.config_erase(ISE_CONFIG_ENABLED_LANGUAGES);
    g_core.config_erase(ISE_CONFIG_SELECTED_LANGUAGE);
    g_core.config_reload();
}