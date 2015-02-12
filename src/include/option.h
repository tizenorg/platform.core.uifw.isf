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

#include "ise.h"
#include "config.h"

#ifndef __OPTION_H__
#define __OPTION_H__

#define LANGUAGE            dgettext(PACKAGE, "IDS_IME_HEADER_INPUT_LANGUAGES")
#define PREDICTION          dgettext(PACKAGE, "IDS_IME_BODY_PREDICTIVE_TEXT")
#define KEYPAD              dgettext(PACKAGE, "IDS_IME_BODY_KEYBOARD_TYPE")
#define KEYPAD_3X4          dgettext(PACKAGE, "IDS_IME_OPT_3_X_4_KEYBOARD")
#define KEYPAD_QTY          dgettext(PACKAGE, "IDS_IME_OPT_QWERTY_KEYBOARD_ABB")
#define OPTIONS             dgettext(PACKAGE, "IDS_IME_BODY_KEYBOARD_SETTINGS")

#define SELECT_LANGUAGES    dgettext(PACKAGE, "IDS_IME_MBODY_SELECT_INPUT_LANGUAGES")
#define SMART_FUNCTIONS     dgettext(PACKAGE, "IDS_IME_HEADER_SMART_INPUT_FUNCTIONS_ABB")
#define AUTO_CAPITALISE     dgettext(PACKAGE, "IDS_IME_MBODY_AUTO_CAPITALISE")
#define CAPITALISE_DESC     dgettext(PACKAGE, "IDS_IME_SBODY_CAPITALISE_THE_FIRST_LETTER_OF_EACH_SENTENCE_AUTOMATICALLY")
#define AUTO_PUNCTUATE      dgettext(PACKAGE, "IDS_IME_MBODY_AUTO_PUNCTUATE")
#define PUNCTUATE_DESC      dgettext(PACKAGE, "IDS_IME_BODY_AUTOMATICALLY_INSERT_A_FULL_STOP_BY_TAPPING_THE_SPACE_BAR_TWICE")
#define KEY_FEEDBACK        dgettext(PACKAGE, "IDS_IME_HEADER_KEY_TAP_FEEDBACK_ABB")
#define SOUND               dgettext(PACKAGE, "IDS_IME_MBODY_SOUND")
#define VIBRATION           dgettext(PACKAGE, "IDS_IME_MBODY_VIBRATION")
#define CHARACTER_PREVIEW   dgettext(PACKAGE, "IDS_IME_MBODY_CHARACTER_PREVIEW")
#define PREVIEW_DESC        dgettext(PACKAGE, "IDS_IME_BODY_SHOW_A_BIG_CHARACTER_BUBBLE_WHEN_A_KEY_ON_A_QWERTY_KEYBOARD_IS_TAPPED")
#define MORE_SETTINGS       dgettext(PACKAGE, "IDS_IME_HEADER_MORE_SETTINGS_ABB")
#define RESET               dgettext(PACKAGE, "IDS_IME_MBODY_RESET")

#define MSG_NONE_SELECTED       dgettext(PACKAGE, "IDS_IME_BODY_YOU_MUST_SELECT_AT_LEAST_ONE_LANGUAGE_IN_KEYBOARD_SETTINGS")
#define SUPPORTED_MAX_LANGUAGES dgettext(PACKAGE,"IDS_IME_POP_MAXIMUM_NUMBER_OF_SUPPORTED_LANGUAGES_HPD_REACHED")

#define RESET_SETTINGS_POPUP_TITLE_TEXT "IDS_IME_OPT_ATTENTION"
#define RESET_SETTINGS_POPUP_TEXT       "Keyboard settings will be reset."
#define POPUP_OK_BTN                    "IDS_ST_SK_OK"
#define POPUP_CANCEL_BTN                "IDS_ST_SK_CANCEL"

#ifndef VCONFKEY_AUTOCAPITAL_ALLOW_BOOL
  #define VCONFKEY_AUTOCAPITAL_ALLOW_BOOL "file/private/isf/autocapital_allow"
#endif
#ifndef VCONFKEY_AUTOPERIOD_ALLOW_BOOL
  #define VCONFKEY_AUTOPERIOD_ALLOW_BOOL  "file/private/isf/autoperiod_allow"
#endif

#define ITEM_DATA_STRING_LEN 256
struct ITEMDATA
{
    char main_text[ITEM_DATA_STRING_LEN];
    char sub_text[ITEM_DATA_STRING_LEN];
    int mode;
    ITEMDATA()
    {
        memset(main_text, 0, sizeof(char)*ITEM_DATA_STRING_LEN);
        memset(sub_text, 0, sizeof(char)*ITEM_DATA_STRING_LEN);
        mode = 0;
    }
};

class ILanguageOption {
public:
    virtual void on_create_option_main_view(Evas_Object *genlist, Evas_Object *naviframe) {}
    virtual void on_destroy_option_main_view() {}
};

class LanguageOptionManager {
public:
    static void add_language_option(ILanguageOption *language_option);
    static scluint get_language_options_num();
    static ILanguageOption* get_language_option_info(unsigned int index);
private:
    static std::vector<ILanguageOption*> language_option_vector;
};

/**
 * Shows the options window when options key is pressed in keypad.
    @param[in] parent           Parent elementary widget
    @param[in] degree           Rotation angle
    @return                     Nothing.
 *
 **/
void
option_window_created(Evas_Object *window, SCLOptionWindowType type);

/**
 * Closes option window
    @return                     Nothing.
 *
 **/
void
option_window_destroyed(Evas_Object *window);

void
read_options(Evas_Object *naviframe);

void
write_options();

#endif
