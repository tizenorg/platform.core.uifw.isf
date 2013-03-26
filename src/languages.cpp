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
#include "utils.h"
#include "config.h"
#include "languages.h"
using namespace scl;

extern CONFIG_VALUES g_config_values;

std::vector<LANGUAGE_INFO> ISELanguageManager::language_vector;
std::string ISELanguageManager::current_language;
std::string ISELanguageManager::temporary_language;
std::string ISELanguageManager::default_resource_file;

sclboolean
ISELanguageManager::set_all_languages_enabled(sclboolean enabled)
{
    sclboolean ret = TRUE;

    std::vector<LANGUAGE_INFO>::iterator iter;
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
        for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;std::advance(iter, 1)) {
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

sclboolean ISELanguageManager::select_language(const sclchar *language, sclboolean temporarily)
{
    sclboolean ret = FALSE;

    std::string unselected_language = current_language;
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

    if (language) {
        for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;std::advance(iter, 1)) {
                if (iter->name.length() > 0) {
                    if (iter->name.compare(language) == 0) {
                        /* If this language is not enabled and also not a temporary selection, select next language */
                        if (!(iter->enabled) && !(iter->enabled_temporarily) && !temporarily) {
                            ret = select_next_language();
                        } else {
                            ILanguageCallback *callback = iter->callback;
                            if (callback) {
                                ret = callback->on_language_selected(iter->name.c_str(), iter->selected_input_mode.c_str());
                            }
                            if (ret) {
                                if (temporarily || iter->enabled_temporarily) {
                                    temporary_language = language;
                                } else {
                                    current_language = language;
                                }
                            }
                        }
                    }
                }
        }
    }

    if (ret) {
        /* Save the selected language */
        g_config_values.selected_language = current_language;
        write_ise_config_values();
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
        temporary_language.clear();
        return select_current_language();
    } else {
        LANGUAGE_INFO *info = get_language_info(current_language.c_str());
        if (info) {
            if (info->callback) {
                info->callback->on_language_unselected(current_language.c_str(), info->selected_input_mode.c_str());
            }
        }

        /* Look for the next language, after we find the current language */
        sclboolean found_current = FALSE;
        for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;std::advance(iter, 1)) {
                if (found_current) {
                    if ((iter->enabled || iter->enabled_temporarily) && iter->name.length() > 0) {
                        ILanguageCallback *callback = iter->callback;
                        if (iter->enabled) {
                            current_language = iter->name;
                        } else {
                            temporary_language = iter->name;
                        }
                        if (callback) {
                            ret = callback->on_language_selected(iter->name.c_str(), iter->selected_input_mode.c_str());
                        }
                    }
                } else {
                    if (iter->name.length() > 0) {
                        if (iter->name.compare(current_language) == 0) {
                            found_current = TRUE;
                        }
                    }
                }
        }

        /* If we could not find a appropriate next language, restart from the beginning one more time.. */
        if (!ret) {
            for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
                iter != language_vector.end() && !ret;std::advance(iter, 1)) {
                    if ((iter->enabled || iter->enabled_temporarily) && iter->name.length() > 0) {
                        ILanguageCallback *callback = iter->callback;
                        if (iter->enabled) {
                            current_language = iter->name;
                        } else {
                            temporary_language = iter->name;
                        }
                        if (callback) {
                            ret = callback->on_language_selected(iter->name.c_str(), iter->selected_input_mode.c_str());
                        }
                    }
            }
        }

        /* If no language can be selected, just select the previous one */
        if (!ret) {
            if (info) {
                if (info->callback) {
                    info->callback->on_language_selected(current_language.c_str(), info->selected_input_mode.c_str());
                    current_language = info->name;
                }
            }
        }

        /* Save the selected language */
        g_config_values.selected_language = current_language;
        write_ise_config_values();
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

        /* Look for the next language, after we find the current language */
        sclboolean found_current = FALSE;
        for (std::vector<LANGUAGE_INFO>::reverse_iterator riter = language_vector.rbegin();
            riter != language_vector.rend() && !ret;std::advance(riter, 1)) {
                if (found_current) {
                    if ((riter->enabled || riter->enabled_temporarily) && riter->name.length() > 0) {
                        ILanguageCallback *callback = riter->callback;
                        if (riter->enabled) {
                            current_language = riter->name;
                        } else {
                            temporary_language = riter->name;
                        }
                        if (callback) {
                            ret = callback->on_language_selected(riter->name.c_str(), riter->selected_input_mode.c_str());
                        }
                    }
                } else {
                    if (riter->name.length() > 0) {
                        if (riter->name.compare(current_language) == 0) {
                            found_current = TRUE;
                        }
                    }
                }
        }

        /* If we could not find a appropriate next language, restart from the beginning one more time.. */
        if (!ret) {
            for (std::vector<LANGUAGE_INFO>::reverse_iterator riter = language_vector.rbegin();
                riter != language_vector.rend() && !ret;std::advance(riter, 1)) {
                    if ((riter->enabled || riter->enabled_temporarily) && riter->name.length() > 0) {
                        ILanguageCallback *callback = riter->callback;
                        if (riter->enabled) {
                            current_language = riter->name;
                        } else {
                            temporary_language = riter->name;
                        }
                        if (callback) {
                            ret = callback->on_language_selected(riter->name.c_str(), riter->selected_input_mode.c_str());
                        }
                    }
            }
        }

        /* If no language can be selected, just select the previous one */
        if (!ret) {
            if (info) {
                if (info->callback) {
                    info->callback->on_language_selected(current_language.c_str(), info->selected_input_mode.c_str());
                    current_language = info->name;
                }
            }
        }

        /* Save the selected language */
        g_config_values.selected_language = current_language;
        write_ise_config_values();
    }

    return ret;
}

sclboolean ISELanguageManager::set_language_enabled(const sclchar *name, sclboolean enabled)
{
    sclboolean ret = FALSE;

    if (name) {
        for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;std::advance(iter, 1)) {
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
        for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
            iter != language_vector.end() && !ret;std::advance(iter, 1)) {
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

sclboolean ISELanguageManager::enable_languages(const std::vector<std::string> &vec_language_id)
{
    sclboolean ret = FALSE;

    if (vec_language_id.size() != 0) {
        ret = set_all_languages_enabled(FALSE);

        if (ret == FALSE) return ret;

        std::vector<std::string>::const_iterator citer;
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

sclboolean ISELanguageManager::set_enabled_languages(const std::vector<std::string> &vec_language_id) {
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

    for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
        iter != language_vector.end();std::advance(iter, 1)) {
            if (iter->enabled || iter->enabled_temporarily) {
                ret++;
            }
    }

    return ret;
}

LANGUAGE_INFO* ISELanguageManager::get_language_info(const sclchar *language)
{
    LANGUAGE_INFO *ret = NULL;

    for (std::vector<LANGUAGE_INFO>::iterator iter = language_vector.begin();
        iter != language_vector.end() && !ret;std::advance(iter, 1)) {
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
