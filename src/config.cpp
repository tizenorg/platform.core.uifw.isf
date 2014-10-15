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

#define Uses_SCIM_CONFIG_BASE

#include <scim.h>
#include <scl.h>

#include "config.h"
#include "languages.h"

using namespace scim;
using namespace scl;

extern ConfigPointer _scim_config;

CONFIG_VALUES::CONFIG_VALUES() {
    keypad_mode = KEYPAD_MODE_QTY; // keypad_mode
    prediction_on = FALSE; // prediction_on
};

CONFIG_VALUES g_config_values;

void read_ise_config_values() {
    if (_scim_config) {
        _scim_config->reload();
        g_config_values.keypad_mode =
            (KEYPAD_MODE)(_scim_config->read(String(ISE_CONFIG_KEYPAD_MODE), g_config_values.keypad_mode));
        g_config_values.prediction_on =
            _scim_config->read (String (ISE_CONFIG_PREDICTION_ON), g_config_values.prediction_on);
        g_config_values.enabled_languages =
            _scim_config->read(String(ISE_CONFIG_ENABLED_LANGUAGES), g_config_values.enabled_languages);
        g_config_values.selected_language =
            _scim_config->read(String(ISE_CONFIG_SELECTED_LANGUAGE), g_config_values.selected_language);
    }
}

void write_ise_config_values() {
    if (_scim_config) {
        _scim_config->write(String(ISE_CONFIG_KEYPAD_MODE), g_config_values.keypad_mode);
        _scim_config->write(String(ISE_CONFIG_PREDICTION_ON), g_config_values.prediction_on);
        _scim_config->write(String(ISE_CONFIG_ENABLED_LANGUAGES), g_config_values.enabled_languages);
        _scim_config->write(String(ISE_CONFIG_SELECTED_LANGUAGE), g_config_values.selected_language);

        _scim_config->flush ();
    }
}

void erase_ise_config_values() {
    if (_scim_config) {
        _scim_config->erase(String(ISE_CONFIG_KEYPAD_MODE));
        _scim_config->erase(String(ISE_CONFIG_PREDICTION_ON));
        _scim_config->erase(String(ISE_CONFIG_ENABLED_LANGUAGES));
        _scim_config->erase(String(ISE_CONFIG_SELECTED_LANGUAGE));

        _scim_config->flush ();
    }
}
