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

vector<LANGUAGE_INFO> ISELanguageManager::language_vector;
string ISELanguageManager::current_language;
string ISELanguageManager::temporary_language;
string ISELanguageManager::default_resource_file;

sclboolean
ISELanguageManager::set_all_languages_enabled(sclboolean enabled)
{
    sclboolean ret = TRUE;

    vector<LANGUAGE_INFO>::iterator iter;
    for (iter = language_vector.begin(); iter != language_vector.end(); ++iter) {
            iter->enabled = enabled;
            iter->enabled_temporarily = FALSE;
    }

    return ret;
}

/* Each language-specific source files should put their callback information in the following vectors */
sclboolean ISELanguageManager::add_language(LANGUAGE_INFO language)
{
    sclboolean ret = FALSE;

    if (language.name.length() > 0) {
        sclboolean found = FALSE;
        for (vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;advance(iter, 1)) {
                if (iter->name.length() > 0) {
                    if (iter->name.compare(language.name) == 0) {
                        found = TRUE;
                        /* If the new language has the SPECIALIZED priority, overwrite existing callback */
                        if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
                            *iter = language;
                        }
                    }
                }
        }
        if (!found) {
            language_vector.push_back(language);
        }

        if (current_language.empty()) {
            /* If there is no default language set currently, assume this to be a default language */
            current_language = language.name;
        } else if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
            /* If this language has the SPECIALIZED priority, assume this to be a default language */
            current_language = language.name;
        }
        if (language.resource_file.length() > 0) {
            if (default_resource_file.empty()) {
                /* If we don't have default resource file, set this resource file information as default */
                default_resource_file = language.resource_file;
            } else if (language.priority == LANGUAGE_PRIORITY_SPECIALIZED) {
                /* Or if this has SPECIALIZED priority, overwrite the existing default resource file information */
                default_resource_file = language.resource_file;
            }
        }

        ret = TRUE;
    }

    return ret;
}

class _language_info_finder {
public:
    _language_info_finder(const string &language_name): m_name(language_name) {}
    bool operator() (const LANGUAGE_INFO &info) {
        return info.name == m_name;
    }
private:
    string m_name;
};

int
 _find_language_info(const string &language_name, const vector<LANGUAGE_INFO>& vec_language_info) {
    vector<LANGUAGE_INFO>::const_iterator it;

    it = std::find_if(vec_language_info.begin(), vec_language_info.end(), _language_info_finder(language_name));
    if (it != vec_language_info.end()) {
        return it - vec_language_info.begin();
    }
    return -1;
}

sclboolean
ISELanguageManager::do_select_language(const string &language_name, sclboolean temporarily/* = FALSE*/) {

    int pos = _find_language_info(language_name, language_vector);

    // the assigned language could not be found in the language info vector
    if (pos < 0 || pos >= language_vector.size()) {
        return FALSE;
    }

    LANGUAGE_INFO &info = language_vector.at(pos);
    info.enabled_temporarily = temporarily;
    return do_select_language(language_vector.at(pos));
}

sclboolean
ISELanguageManager::do_select_language(LANGUAGE_INFO &language_info) {
    sclboolean ret = FALSE;

    // if not enabled and not temporary, return false
    if (!language_info.enabled && !language_info.enabled_temporarily) {
        return FALSE;
    }
    ILanguageCallback *callback = language_info.callback;
    if (callback) {
        ret = callback->on_language_selected(language_info.name.c_str(), language_info.selected_input_mode.c_str());
    }
    if (ret == FALSE) {
        return ret;
    }

    if (language_info.enabled_temporarily) {
        temporary_language = language_info.name;
    } else {
        current_language = language_info.name;
        /* Save the selected language */
        g_config_values.selected_language = current_language;
        write_ise_config_values();
    }

    return ret;
}

sclboolean ISELanguageManager::select_language(const sclchar *language, sclboolean temporarily)
{
    sclboolean ret = FALSE;

    if (language == NULL) return FALSE;

    string unselected_language = current_language;
    if (!(temporary_language.empty())) {
        unselected_language = temporary_language;
    }
    LANGUAGE_INFO *info = get_language_info(unselected_language.c_str());
    if (info) {
        if (info->callback) {
            info->callback->on_language_unselected(unselected_language.c_str(), info->selected_input_mode.c_str());
        }
    }
    temporary_language.clear();

    ret = do_select_language(language, temporarily);

    if (ret == FALSE) {
        ret = select_next_language();
    }
    return ret;
}

sclboolean ISELanguageManager::select_current_language()
{
    sclboolean ret = FALSE;

    if (temporary_language.empty()) {
        if (!(ret = select_language(current_language.c_str()))) {
            ret = select_next_language();
        }
    } else {
        ret = select_language(temporary_language.c_str(), TRUE);
    }

    return ret;
}

sclboolean ISELanguageManager::select_next_language()
{
    sclboolean ret = FALSE;

    if (!(temporary_language.empty())) {
        int pos = _find_language_info(temporary_language, language_vector);
        if (pos >= 0) {
            language_vector.at(pos).enabled_temporarily = FALSE;
        }
        temporary_language.clear();
        return select_current_language();
    } else {
        LANGUAGE_INFO *info = get_language_info(current_language.c_str());
        if (info) {
            if (info->callback) {
                info->callback->on_language_unselected(current_language.c_str(), info->selected_input_mode.c_str());
            }
        }

        int next_pos = -1;
        int pos = _find_language_info(current_language, language_vector);
        if (pos < 0) {
            next_pos = 0;
        } else if (pos >= language_vector.size() -1){
            next_pos = 0;
        } else {
            next_pos = pos + 1;
        }
        assert(next_pos >= 0 && next_pos < language_vector.size());

        sclboolean b_select_ok = FALSE;
        for (int i = next_pos; i < language_vector.size(); ++i) {
            b_select_ok = do_select_language(language_vector.at(i));
            if (b_select_ok == TRUE) {
                break;
            }
        }

        if (b_select_ok == FALSE) {
            for (int i = 0; i < next_pos; ++i) {
                b_select_ok = do_select_language(language_vector.at(i));
                if (b_select_ok == TRUE) {
                    break;
                }
            }
        }
        ret = b_select_ok;
    }

    return ret;
}

sclboolean ISELanguageManager::select_previous_language()
{
    sclboolean ret = FALSE;

    if (!(temporary_language.empty())) {
        temporary_language.clear();
        return select_current_language();
    } else {
        LANGUAGE_INFO *info = get_language_info(current_language.c_str());
        if (info) {
            if (info->callback) {
                info->callback->on_language_unselected(current_language.c_str(), info->selected_input_mode.c_str());
            }
        }

        int pre_pos = -1;
        int pos = _find_language_info(current_language, language_vector);
        if (pos < 0) {
            pre_pos = 0;
        }
        else if (pos == 0) {
            pre_pos = language_vector.size() -1;
        } else {
            pre_pos = pos -1;
        }

        assert(pre_pos >= 0 && pre_pos < language_vector.size());

        sclboolean b_select_ok = FALSE;
        for (int i = pre_pos; i >= 0; --i) {
            b_select_ok = do_select_language(language_vector.at(i));
            if (b_select_ok == TRUE) {
                break;
            }
        }
        if (b_select_ok == FALSE) {
            for (int i = language_vector.size() -1; i > pre_pos; --i) {
                b_select_ok = do_select_language(language_vector.at(i));
                if (b_select_ok == TRUE) {
                    break;
                }
            }
        }
        ret = b_select_ok;
    }

    return ret;
}

sclboolean ISELanguageManager::set_language_enabled(const sclchar *name, sclboolean enabled)
{
    sclboolean ret = FALSE;

    if (name) {
        for (vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;advance(iter, 1)) {
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
        for (vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;advance(iter, 1)) {
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

sclboolean ISELanguageManager::enable_languages(const vector<string> &vec_language_id)
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

/* FIXME A temporaty way for enable default language */
sclboolean ISELanguageManager::enable_default_language() {
    if (language_vector.size()) {
        LANGUAGE_INFO &default_language = language_vector.at(0);
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
    return current_language.c_str();
}

const sclchar* ISELanguageManager::get_resource_file_path()
{
    return default_resource_file.c_str();
}

scluint ISELanguageManager::get_languages_num()
{
    return language_vector.size();
}

scluint ISELanguageManager::get_enabled_languages_num()
{
    scluint ret = 0;

    for (vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
        iter != language_vector.end();advance(iter, 1)) {
            if (iter->enabled || iter->enabled_temporarily) {
                ret++;
            }
    }

    return ret;
}

LANGUAGE_INFO* ISELanguageManager::get_language_info(const sclchar *language)
{
    LANGUAGE_INFO *ret = NULL;

    for (vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
        iter != language_vector.end() && !ret;advance(iter, 1)) {
            if (iter->name.length() > 0) {
                if (iter->name.compare(current_language) == 0) {
                    ret = &(*iter);
                }
            }
    }

    return ret;
}
LANGUAGE_INFO* ISELanguageManager::get_language_info(int index)
{
    LANGUAGE_INFO *ret = NULL;

    if (CHECK_ARRAY_INDEX(index, language_vector.size())) {
        ret = &(language_vector.at(index));
    }

    return ret;
}
