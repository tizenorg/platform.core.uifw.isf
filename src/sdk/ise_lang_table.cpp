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

#include <dlog.h>
#undef LOG_TAG
#define LOG_TAG "ISE_DEFAULT"
#include <sclcommon.h> // scl structures need
#include <libxml/parser.h>
#include <vector>
#include <string>
#include <assert.h>
#include <memory.h>
#include <string.h>

#include "ise_lang_table.h"

using namespace scl;

#ifdef _TV
#define LANG_TABLE_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/tv/ise_lang_table.xml"
#else
#define LANG_TABLE_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/sdk/ise_lang_table.xml"
#endif

typedef struct Keyboard_UUID {
    std::string name;
    std::string uuid;
}Keyboard_UUID;

static std::vector<Keyboard_UUID> vec_keyboard_uuid;

class Ise_Lang {
    public:
        Ise_Lang();
        ~Ise_Lang();
        SDK_ISE_LANG_TABLE m_table[MAX_LANG_TABLE_SIZE];
        int m_size;
    private:
        void parsing_languages();
        void parsing_lang_table(const xmlNodePtr);
        void parsing_keyboard_uuid_table(const xmlNodePtr);
};
Ise_Lang::~Ise_Lang() {
    for (int i = 0; i < m_size; ++i) {
        free(m_table[i].language);
        free(m_table[i].language_name);
        free(m_table[i].locale_string);
        free(m_table[i].inputmode_QTY);
        free(m_table[i].inputmode_QTY_name);
        free(m_table[i].main_keyboard_name);
        free(m_table[i].keyboard_ise_uuid);
        free(m_table[i].country_code_URL);
    }
}
static int
get_prop_int(
        const xmlNodePtr cur_node,
        const char* prop) {
    int val = 0;
    if (cur_node && prop) {
        xmlChar* key = xmlGetProp(
                cur_node,
                (const xmlChar*)prop);
        if (key) {
            val = atoi((const char*)key);
            xmlFree(key);
        }
    }
    return val;
}

static std::string
get_prop_str(
        const xmlNodePtr cur_node,
        const char* prop) {
    std::string str;
    if (cur_node && prop) {
        xmlChar* key = xmlGetProp(
                cur_node,
                (const xmlChar*)prop);
        if (key) {
            str = std::string((const char*)key);
            xmlFree(key);
        }
    }
    return str;
}

static bool
get_prop_bool(
        const xmlNodePtr cur_node,
        const char* prop) {
    bool bret = false;

    if (cur_node && prop) {
        xmlChar* key = xmlGetProp(
                cur_node,
                (const xmlChar*)prop);
        if (key) {
            if (0 == strcmp("true", (const char*)key)) {
                bret = true;
            }
            xmlFree(key);
        }
    }
    return bret;
}

Ise_Lang::Ise_Lang() {
    memset(m_table, 0x00, sizeof(SDK_ISE_LANG_TABLE) * MAX_LANG_TABLE_SIZE);
    parsing_languages();
}

void
Ise_Lang::parsing_languages() {
    xmlDocPtr doc;
    xmlNodePtr cur_node;

    doc = xmlReadFile(LANG_TABLE_XML_PATH, NULL, 0);
    if (doc == NULL) {
        LOGE("Could not load file.\n");
        return;
    }

    cur_node = xmlDocGetRootElement(doc);
    if (cur_node == NULL) {
        LOGE("empty document.\n");
        xmlFreeDoc(doc);
        return;
    }
    if (0 != xmlStrcmp(cur_node->name, (const xmlChar*)"languages"))
    {
        LOGE("root name %s error!\n", cur_node->name);
        xmlFreeDoc(doc);
        return;
    }

    cur_node = cur_node->xmlChildrenNode;

    while (cur_node != NULL) {
        if (0 == xmlStrcmp(cur_node->name, (const xmlChar *)"text")) {
            cur_node = cur_node->next;
            continue;
        } else if (0 == xmlStrcmp(cur_node->name, (const xmlChar *)"keyboard_uuid_table")) {
            parsing_keyboard_uuid_table(cur_node);
        } else if (0 == xmlStrcmp(cur_node->name, (const xmlChar *)"language_table")) {
            parsing_lang_table(cur_node);
        }
        cur_node = cur_node->next;
    }
}

void
Ise_Lang::parsing_keyboard_uuid_table(const xmlNodePtr p_node) {
    assert(p_node != NULL);
    if (0 != xmlStrcmp(p_node->name, (const xmlChar *)"keyboard_uuid_table")) {
        LOGD("parsing-keyboard_uuid_table error.\n");
        return;
    }
    xmlNodePtr cur_node = p_node->xmlChildrenNode;

    while (cur_node != NULL) {
        if (0 == xmlStrcmp(cur_node->name, (const xmlChar *)"rec")) {
            Keyboard_UUID rec;
            rec.name = get_prop_str(cur_node, "name");
            rec.uuid = get_prop_str(cur_node, "uuid");
            if (!rec.name.empty() && !rec.uuid.empty()) {
                vec_keyboard_uuid.push_back(rec);
            }
        }
        cur_node = cur_node->next;
    }
}
static inline std::string
find_uuid(const std::vector<Keyboard_UUID>& vec_rec, const std::string& name) {
    std::vector<Keyboard_UUID>::const_iterator it;
    for (it = vec_rec.begin(); it != vec_rec.end(); ++it) {
        if (it->name == name) {
            return it->uuid;
        }
    }

    return std::string();
}

static inline char*
get_str(std::string str) {
    int len = str.length();
    if (len == 0) return NULL;

    char* p = new char[len + 1];
    if (p == NULL) return NULL;

    strncpy(p, str.c_str(), len);
    p[len] = 0;

    return p;
}

void
Ise_Lang::parsing_lang_table(const xmlNodePtr p_node) {
    assert(p_node != NULL);

    if (0 != xmlStrcmp(p_node->name, (const xmlChar *)"language_table")) {
        LOGD("parsing language_table error.\n");
        return;
    }
    xmlNodePtr cur_node = p_node->xmlChildrenNode;

    m_size = 0;
    while (cur_node != NULL) {
        if (0 == xmlStrcmp(cur_node->name, (const xmlChar *)"rec")) {
            xmlChar* language = xmlGetProp(cur_node, (const xmlChar*)"language");
            m_table[m_size].language = (sclchar*)language;
            xmlChar* language_name = xmlGetProp(cur_node, (const xmlChar*)"language_name");
            m_table[m_size].language_name = (sclchar*)language_name;
            xmlChar* locale_string = xmlGetProp(cur_node, (const xmlChar*)"locale_string");
            m_table[m_size].locale_string = (sclchar*)locale_string;
            xmlChar* inputmode_QTY = xmlGetProp(cur_node, (const xmlChar*)"inputmode_QTY");
            m_table[m_size].inputmode_QTY = (sclchar*)inputmode_QTY;

            xmlChar* inputmode_QTY_name = xmlGetProp(cur_node, (const xmlChar*)"inputmode_QTY_name");
            m_table[m_size].inputmode_QTY_name = (sclchar*)inputmode_QTY_name;

            xmlChar* main_keyboard_name = xmlGetProp(cur_node, (const xmlChar*)"main_keyboard_name");
            if (main_keyboard_name) {
                m_table[m_size].main_keyboard_name = (sclchar*)main_keyboard_name;
            } else {
                m_table[m_size].main_keyboard_name = (sclchar *)strdup("abc");
            }

            std::string uuid = find_uuid(vec_keyboard_uuid, get_prop_str(cur_node, "keyboard_ise_uuid"));
            m_table[m_size].keyboard_ise_uuid = get_str(uuid);
            xmlChar* country_code_URL = xmlGetProp(cur_node, (const xmlChar*)"country_code_URL");
            m_table[m_size].country_code_URL = (sclchar*)country_code_URL;

            m_table[m_size].language_code = get_prop_int(cur_node, "language_code");
            m_table[m_size].language_command = get_prop_int(cur_node, "language_command");
            m_table[m_size].flush_code = get_prop_int(cur_node, "flush_code");
            m_table[m_size].flush_command = get_prop_int(cur_node, "flush_command");
            m_table[m_size].is_latin_language = get_prop_bool(cur_node, "is_latin_language");
            m_table[m_size].accepts_caps_mode = get_prop_bool(cur_node, "accepts_caps_mode");

            m_size++;
        }
        cur_node = cur_node->next;
    }
}

static Ise_Lang ise_lang;

SDK_ISE_LANG_TABLE* get_lang_table() {
    return ise_lang.m_table;
}

int get_lang_table_size() {
    return ise_lang.m_size;
}
