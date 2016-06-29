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

#ifndef ISE_EMOTICON_MODE_H_
#define ISE_EMOTICON_MODE_H_

#ifdef SUPPORTS_EMOTICONS

#ifdef _WEARABLE
#define EMOTICON_GROUP_RECENTLY_USED_NUM 9
#define EMOTICON_GROUP_1_NUM 9
#define EMOTICON_GROUP_2_NUM 9
#define EMOTICON_GROUP_3_NUM 9
#define EMOTICON_GROUP_4_NUM 0
#define EMOTICON_GROUP_5_NUM 0
#else
#define EMOTICON_GROUP_RECENTLY_USED_NUM 18
#define EMOTICON_GROUP_1_NUM 77
#define EMOTICON_GROUP_2_NUM 157
#define EMOTICON_GROUP_3_NUM 178
#define EMOTICON_GROUP_4_NUM 114
#define EMOTICON_GROUP_5_NUM 207
#endif

enum emoticon_group_t
{
    EMOTICON_GROUP_DESTROY = -1,
    EMOTICON_GROUP_RECENTLY_USED,
    EMOTICON_GROUP_1,
    EMOTICON_GROUP_2,
    EMOTICON_GROUP_3,
    EMOTICON_GROUP_4,
    EMOTICON_GROUP_5,
    MAX_EMOTICON_GROUP,
};

void ise_show_emoticon_window(emoticon_group_t emoticon_group, const int screen_degree, const bool is_candidate_on, void *main_window);
void ise_change_emoticon_mode(emoticon_group_t emoticon_group);
void ise_destroy_emoticon_window(void);
void ise_init_emoticon_list(void);
bool is_emoticon_show(void);
emoticon_group_t ise_get_emoticon_group_id(const char *group_name);
const char *ise_get_emoticon_group_name(int id);
void ise_set_private_key_for_emoticon_mode(const emoticon_group_t emoticon_group);
#endif /* SUPPORTS_EMOTICONS */
#endif /* ISE_EMOTICON_MODE_H_ */
