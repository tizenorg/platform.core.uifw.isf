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

#include <scl.h>
#include <stdio.h>
#include <algorithm>
#include <assert.h>
#include "utils.h"
#include "config.h"
#include "languages.h"
using namespace scl;
using std::string;
using std::vector;

extern CONFIG_VALUES g_config_values;

static vector<LANGUAGE_INFO> _language_vector;
static int _current_language = -1;
static string _default_resource_file;

// A class function that is used for algorithm std::find_if
class _language_info_finder {
public:
    _language_info_finder(const string &language_name): m_name(language_name) {}
    bool operator() (const LANGUAGE_INFO &info) {
        return info.name == m_name;
    }
private:
    string m_name;
};

sclboolean
ISELanguageManager::set_all_languages_enabled(sclboolean enabled)
{
    sclboolean ret = TRUE;

    vector<LANGUAGE_INFO>::iterator iter;
    for (iter = _language_vector.begin(); iter != _language_vector.end(); ++iter) {
            iter->enabled = enabled;
            iter->enabled_temporarily = FALSE;
    }

    return ret;
}

/* Each language-specific source files should put their callback information in the following vectors */
sclboolean
ISELanguageManager::add_language(LANGUAGE_INFO language)
{
    int language_id = -1;

    // check whether there is an language_info has the same name with "language" in the vector
    vector<LANGUAGE_INFO>::iterator it;
    it = std::find_if(_language_vector.begin(), _language_vector.end(), _language_info_finder(language.name));
    if (it != _language_vector.end()) {
        // if the assigned one has the priority "LANGUAGE_PRIORITY_SPECIALIZED" - which means high priority,
        // then the assigned one will replace with the original one
        if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
            *it = language;
            language_id = it - _language_vector.begin();
        }
    } else {
        _language_vector.push_back(language);
        language_id = _language_vector.size() -1;
    }

    if (_current_language == -1) {
        /* If there is no default language set currently, assume this to be a default language */
        _current_language = language_id;
    } else if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
        /* If this language has the SPECIALIZED priority, assume this to be a default language */
        _current_language = language_id;
    }
    if (language.resource_file.length() > 0) {
        if (_default_resource_file.empty()) {
            /* If we don't have default resource file, set this resource file information as default */
            _default_resource_file = language.resource_file;
        } else if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
            /* Or if this has SPECIALIZED priority, overwrite the existing default resource file information */
            _default_resource_file = language.resource_file;
        }
    }

    return TRUE;
}

int
 _find_language_info(const string &language_name, const vector<LANGUAGE_INFO>& vec_language_info) {
    vector<LANGUAGE_INFO>::const_iterator it;

    it = std::find_if(vec_language_info.begin(), vec_language_info.end(), _language_info_finder(language_name));
    if (it != vec_language_info.end()) {
        return it - vec_language_info.begin();
    }
    return -1;
}

// normal routine of select language
sclboolean
ISELanguageManager::do_select_language(int language_info_index) {
    sclboolean ret = FALSE;

    if (language_info_index < 0 || language_info_index >= (int)_language_vector.size()) {
        return FALSE;
    }
    LANGUAGE_INFO &language_info = _language_vector.at(language_info_index);
    // if not enabled and not temporary, return false
    if (!language_info.enabled && !language_info.enabled_temporarily) {
        return FALSE;
    }

    // run the callback function
    ILanguageCallback *callback = language_info.callback;
    if (callback) {
        ret = callback->on_language_selected(language_info.name.c_str(), language_info.selected_input_mode.c_str());
    }
    if (ret == FALSE) {
        return ret;
    }

    _current_language = language_info_index;
    if (!language_info.enabled_temporarily) {
        /* Save the selected language */
        g_config_values.selected_language = language_info.name;
        write_ise_config_values();
    }

    return ret;
}

// normal routine of select language
sclboolean
ISELanguageManager::do_unselect_language(int language_info_index) {
    sclboolean ret = FALSE;
    if (language_info_index < 0 || language_info_index >= (int)_language_vector.size()) {
        return FALSE;
    }

    LANGUAGE_INFO &language_info = _language_vector.at(language_info_index);
    if (language_info.callback) {
        ret = language_info.callback->on_language_unselected(language_info.name.c_str(), language_info.selected_input_mode.c_str());
    }

    return ret;
}

// interface function, call do_select_language actually
sclboolean
ISELanguageManager::select_language(const sclchar *language, sclboolean temporarily)
{
    sclboolean ret = FALSE;

    if (language == NULL) return FALSE;
    int pos = _find_language_info(language, _language_vector);

    // the assigned language could not be found in the language info vector
    if (pos < 0 || pos >= (int)_language_vector.size()) {
        return FALSE;
    }

    LANGUAGE_INFO &info = _language_vector.at(pos);
    info.enabled_temporarily = temporarily;

    ret = do_select_language(pos);

    if (ret == FALSE) {
        ret = select_next_language();
    }
    return ret;
}

sclboolean
ISELanguageManager::select_current_language()
{
    sclboolean ret = FALSE;
    ret = do_select_language(_current_language);

    return ret;
}

sclboolean
ISELanguageManager::select_next_language()
{
    sclboolean ret = FALSE;

    // do some work before change to next language
    // eg: commit preedit string...
    ret = do_unselect_language(_current_language);
    if (ret == FALSE) {
        return FALSE;
    }

    // get next position
    int next_pos = -1;
    if (_current_language == (int)_language_vector.size() -1){
        next_pos = 0;
    } else {
        next_pos = _current_language + 1;
    }
    assert(next_pos >= 0 && next_pos < (int)_language_vector.size());

    // select next language
    // the next one may be not enabled, so the loop below is to search afterwards
    // continually until find an enabled one
    // example: if current index is 5, the search order is:
    // 5, 6, 7, 8, max, 0, 1, 2, 3, 4
    sclboolean b_select_ok = FALSE;
    for (int i = next_pos; i < (int)_language_vector.size(); ++i) {
        b_select_ok = do_select_language(i);
        if (b_select_ok == TRUE) {
            break;
        }
    }

    if (b_select_ok == FALSE) {
        for (int i = 0; i < next_pos; ++i) {
            b_select_ok = do_select_language(i);
            if (b_select_ok == TRUE) {
                break;
            }
        }
    }
    ret = b_select_ok;

    return ret;
}

sclboolean
ISELanguageManager::select_previous_language()
{
    sclboolean ret = FALSE;

    // do some work before change to previous language
    // eg: commit predit string...
    ret = do_unselect_language(_current_language);
    if (ret == FALSE) {
        return FALSE;
    }

    // get previous position
    int pre_pos = -1;
    assert(_current_language >= 0 && _current_language < (int)_language_vector.size());
    if (_current_language == 0) {
        pre_pos = _language_vector.size() -1;
    } else {
        pre_pos = _current_language -1;
    }
    assert(pre_pos >= 0 && pre_pos < (int)_language_vector.size());

    // select previous language
    // the previous one may be not enabled, so the loop below is to search forwards
    // continually until find an enabled one
    // example: if current index is 5, the search order is:
    // 5, 4, 3, 2, 1, 0, max, max-1, ...4
    sclboolean b_select_ok = FALSE;
    for (int i = pre_pos; i >= 0; --i) {
        b_select_ok = do_select_language(i);
        if (b_select_ok == TRUE) {
            break;
        }
    }
    if (b_select_ok == FALSE) {
        for (int i = _language_vector.size() -1; i > pre_pos; --i) {
            b_select_ok = do_select_language(i);
            if (b_select_ok == TRUE) {
                break;
            }
        }
    }
    ret = b_select_ok;

    return ret;
}

sclboolean ISELanguageManager::set_language_enabled(const sclchar *name, sclboolean enabled)
{
    sclboolean ret = FALSE;

    if (name) {
        for (vector<LANGUAGE_INFO>::iterator iter = _language_vector.begin();
            iter != _language_vector.end() && !ret;advance(iter, 1)) {
                if (iter->name.length() > 0) {
                    if (iter->name.compare(name) == 0) {
                        iter->enabled = enabled;
                        ret = TRUE;
                    }
                }
        }
    }

    return ret;
}

sclboolean ISELanguageManager::set_language_enabled_temporarily(const sclchar *name, sclboolean enabled_temporarily)
{
    sclboolean ret = FALSE;

    if (name) {
        for (vector<LANGUAGE_INFO>::iterator iter = _language_vector.begin();
            iter != _language_vector.end() && !ret;advance(iter, 1)) {
                if (iter->name.length() > 0) {
                    if (iter->name.compare(name) == 0) {
                        iter->enabled_temporarily = enabled_temporarily;
                        ret = TRUE;
                    }
                }
        }
    }

    return ret;
}

sclboolean
ISELanguageManager::enable_languages(const vector<string> &vec_language_id)
{
    sclboolean ret = FALSE;

    if (vec_language_id.size() != 0) {
        ret = set_all_languages_enabled(FALSE);

        if (ret == FALSE) return ret;

        vector<string>::const_iterator citer;
        for (citer = vec_language_id.begin(); citer != vec_language_id.end(); ++citer) {
            ret = set_language_enabled(citer->c_str(), TRUE);
            if (ret == FALSE) return FALSE;
        }
    }

    return ret;
}

/* FIXME A temporary way for enable default language */
sclboolean ISELanguageManager::enable_default_language() {
    if (_language_vector.size()) {
        LANGUAGE_INFO &default_language = _language_vector.at(0);
        default_language.enabled = TRUE;
        default_language.enabled_temporarily = FALSE;
        return TRUE;
    }

    return FALSE;
}

sclboolean ISELanguageManager::set_enabled_languages(const vector<string> &vec_language_id) {
    sclboolean ret = FALSE;

    if (vec_language_id.size() == 0 || FALSE == enable_languages(vec_language_id)) {
        ret = enable_default_language();
    }

    return ret;
}

const sclchar* ISELanguageManager::get_current_language()
{
    if (_current_language >= 0 && _current_language < (int)_language_vector.size()) {
        return _language_vector.at(_current_language).name.c_str();
    }
    return NULL;
}

const sclchar* ISELanguageManager::get_resource_file_path()
{
    return _default_resource_file.c_str();
}

scluint ISELanguageManager::get_languages_num()
{
    return _language_vector.size();
}

scluint ISELanguageManager::get_enabled_languages_num()
{
    scluint ret = 0;

    for (vector<LANGUAGE_INFO>::iterator iter = _language_vector.begin();
        iter != _language_vector.end();advance(iter, 1)) {
            if (iter->enabled || iter->enabled_temporarily) {
                ret++;
            }
    }

    return ret;
}

LANGUAGE_INFO* ISELanguageManager::get_language_info(const sclchar *language_name)
{
    vector<LANGUAGE_INFO>::iterator it;

    it = std::find_if(_language_vector.begin(), _language_vector.end(), _language_info_finder(language_name));

    if (it != _language_vector.end()) {
        return &(*it);
    }
    return NULL;
}

LANGUAGE_INFO* ISELanguageManager::get_language_info(int index)
{
    LANGUAGE_INFO *ret = NULL;

    if (index >= 0 && index < (int)_language_vector.size()) {
        ret = &(_language_vector.at(index));
    }

    return ret;
}

LANGUAGE_INFO* ISELanguageManager::get_current_language_info()
{
    return get_language_info(_current_language);
}
