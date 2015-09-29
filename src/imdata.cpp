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

#include "ise.h"

#define IMDATA_STRING_MAX_LEN 32

#define IMDATA_ITEM_DELIMETER "&"
#define IMDATA_ITEM_MAX_NUM 16

#define IMDATA_KEY_LANGUAGE "lang"
#define IMDATA_KEY_ACTION   "action"
#define IMDATA_KEY_VALUE_DELIMETER "="

#define IMDATA_VALUE_DISABLE_EMOTICONS  "disable_emoticons"

static ISELanguageManager _language_manager;
extern int g_imdata_state;

/* A macro that frees an array returned by eina_str_split() function -
 * We need to free the first element and the pointer itself. */

static void safe_release_splitted_eina_str(char **string_array)
{
    if(string_array) {
        if(string_array[0]) {
            free(string_array[0]);
        }
        free(string_array);
    }
}


static void process_imdata_string_language(const char *language_string)
{
    bool found = FALSE;
    if(language_string) {
        for (scluint loop = 0;loop < _language_manager.get_languages_num() && !found;loop++) {
            LANGUAGE_INFO *language = _language_manager.get_language_info(loop);
            if (language) {
                if (language->locale_string.compare(language_string) == 0 && language->enabled) {
                    _language_manager.select_language(language->name.c_str());
                    found = TRUE;
                }
            }
        }
    }
}

void set_ise_imdata(const char * buf, size_t &len)
{
    if(buf) {
        /* Make sure we're dealing with a NULL-terminated string */
        char imdatastr[IMDATA_STRING_MAX_LEN + 1] = {0};
        strncpy(imdatastr, buf, (IMDATA_STRING_MAX_LEN > len) ? len : IMDATA_STRING_MAX_LEN);

        char **items = eina_str_split(imdatastr, IMDATA_ITEM_DELIMETER, IMDATA_ITEM_MAX_NUM);
        if(items) {
            int loop = 0;
            while(items[loop] && loop < IMDATA_ITEM_MAX_NUM) {
                char **keyvalue = eina_str_split(items[loop], IMDATA_KEY_VALUE_DELIMETER, 2);
                if(keyvalue[0]) {
                    LOGD ("key (%s), value (%s)", keyvalue[0], keyvalue[1]);
                    /* If the key string requests us to change the language */
                    if(strncmp(keyvalue[0], IMDATA_KEY_LANGUAGE, strlen(IMDATA_KEY_LANGUAGE)) == 0) {
                        process_imdata_string_language(keyvalue[1]);
                    } else if (strncmp (keyvalue[0], IMDATA_KEY_ACTION, strlen (IMDATA_KEY_ACTION)) == 0) {
                        if (keyvalue[1] != NULL) {
                            if (strncmp (keyvalue[1], IMDATA_VALUE_DISABLE_EMOTICONS, strlen (IMDATA_VALUE_DISABLE_EMOTICONS)) == 0) {  /* Hide Emoticon CM key */
                                g_imdata_state = g_imdata_state | IMDATA_ACTION_DISABLE_EMOTICONS;
                            }
                        }
                    }
                }
                safe_release_splitted_eina_str(keyvalue);
                loop++;
            }
            safe_release_splitted_eina_str(items);
        }
    }
}
