#include "ise.h"

#define IMDATA_STRING_MAX_LEN 32

#define IMDATA_ITEM_DELIMETER "&"
#define IMDATA_ITEM_MAX_NUM 16

#define IMDATA_KEY_LANGUAGE "lang"
#define IMDATA_KEY_VALUE_DELIMETER "="

static ISELanguageManager _language_manager;

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
        for (scluint loop = 0;loop < _language_manager.get_languages_num();loop++) {
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
                    /* If the key string requests us to change the language */
                    if(strncmp(keyvalue[0], IMDATA_KEY_LANGUAGE, strlen(IMDATA_KEY_LANGUAGE)) == 0) {
                        process_imdata_string_language(keyvalue[1]);
                    }
                }
                safe_release_splitted_eina_str(keyvalue);
                loop++;
            }
            safe_release_splitted_eina_str(items);
        }
    }
}
