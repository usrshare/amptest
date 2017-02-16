// vim: cin:sts=4:sw=4
#ifndef UI_STRING_H
#define UI_STRING_H
#include "ui.h"

extern char* text_layout;

int iterate_utf8_codepoint(const char* cur, unsigned int* u_val);
int find_utf8char_in_utf8layout(unsigned int c_cp, const char* layout, int* o_x, int* o_y);

int skinTextLength(const char* text);
int skinTextLengthC(const char* text);

int load_text_layout(const char* filename);
#endif
