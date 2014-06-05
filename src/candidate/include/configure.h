#ifndef __CONFIG_H__
#define __CONFIG_H__
#include <string>

enum Item_Status {
    NORMAL = 0,
    PRESSED,
    MAX_ITEM_STATUS
};

struct Candidate_Item_Config{
    Candidate_Item_Config () {
        /* This part will be moved to the XML later*/
        width = 200;
        height = 84;
        text_color[0] = 100;
        text_color[1] = 100;
        text_color[2] = 100;
        text_color[3] = 255;
        text_font = std::string("arial");
        text_size = 32;
    }
    int width;
    int height;
    int text_color[4];
    string text_font;
    int text_size;
    string item_background[MAX_ITEM_STATUS];
};

struct Candidate_Config {
    Candidate_Config() {
        /* This part will be moved to the XML later*/
        width = 780;
        height = 84;
    }
    int width;
    int height;
    string backgrond;
    int max_candidate;
};

static Candidate_Item_Config item_config;
static Candidate_Config candidate_config;
#endif
