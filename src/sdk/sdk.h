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

#ifndef _SDK_H_
#define _SDK_H_

#define MVK_BackSpace 0xff08
#define MVK_Left 0xff51
#define MVK_Right 0xff53
#define MVK_space 0x020
#define MVK_Shift_L 0xffe1
#define MVK_Caps_Lock 0xffe5
#define MVK_Shift_Lock 0xffe6
#define MVK_Return 0xff0d

#define LANGUAGE_STRING "_LANGUAGE_"

#ifdef _TV
#define MAIN_ENTRY_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/tv/main_entry.xml"
#elif _WEARABLE
#define MAIN_ENTRY_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/wearable/main_entry.xml"
#else
#ifndef MAIN_ENTRY_XML_PATH
#define MAIN_ENTRY_XML_PATH "/usr/share/isf/ise/ise-default/720x1280/default/sdk/main_entry.xml"
#endif
#endif

#endif
