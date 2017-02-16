// vim: cin:sts=4:sw=4
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ui_string.h"
#include "ui.h"

char* default_text_layout = 
"Aa\tBb\tCc\tDd\tEe\tFf\tGg\tHh\tIi\tJj\tKk\tLl\tMm\tNn\tOo\tPp\tQq\tRr\tSs\tTt\tUu\tVv\tWw\tXx\tYy\tZz\t\"\t@\n"
"0\t1\t2\t3\t4\t5\t6\t7\t8\t9\t…\t.\t:\t(\t)\t-\t'\t!\t_\t+\t\\\t/\t[\t]\t^\t&\t%\t.\t=\t$\t#\n"
"Åå\tÖö\tÄä\t?\t*\t ";
char* allocd_text_layout = NULL;
char* text_layout = NULL;

int iterate_utf8_codepoint(const char* cur, unsigned int* u_val) {

    if ((!cur) || (!u_val)) {
	return -1;
    }
    const unsigned char* c = (const unsigned char*) cur;

    if (c[0] == 0)   { *u_val = 0; return 0; }
    if (c[0] < 0x80) { *u_val = c[0]; return 1; } //0x00 - 0x7F, 1 byte
    if (c[0] < 0xC0) { *u_val = '?'; return 1; } //0x80 - 0xBF, continuation byte
    if (c[0] < 0xE0) { *u_val = ((c[0] & 0x1f) << 6) + (c[1] & 0x3F); return 2; }
    if (c[0] < 0xF0) { *u_val = ((c[0] & 0x0f) << 12) + ((c[1] & 0x3F) << 6) + (c[2] & 0x3F); return 3; }
    /* if (c[0] < 0xF8) { */
    *u_val = ((c[0] & 0x07) << 18) + ((c[1] & 0x3F) << 12) + ((c[2] & 0x3F) << 6) + (c[3] & 0x3F); return 4;
    /* } */
}

int find_utf8char_in_utf8layout(unsigned int c_cp, const char* layout, int* o_x, int* o_y) {

    int x=0, y=0;
    unsigned int l_cp = 0;
    const char* cur_l = layout;
    int r = 1;
    do {
	r = iterate_utf8_codepoint(cur_l, &l_cp);

	if (l_cp == '\t') x++;
	if (l_cp == '\n') {x = 0; y++;}

	if (l_cp == c_cp) {*o_x = x; *o_y = y; return 0;}

	cur_l += r;
    } while (r > 0);


    *o_x = 3; *o_y = 2; //location of the ? character
    return -1;
}

int skinTextLengthC(const char* text) { //length in codepoints

    const char* cur = text;
    int r = 0;
    unsigned int c_cp = 0;

    int dx=0;

    do {
	r = iterate_utf8_codepoint(cur,&c_cp);
	if (r == 0) return dx;
	dx++;
	cur += r;
    } while (1);
}

int skinTextLength(const char* text) {
    return skinTextLengthC(text) * 5;
}

int load_text_layout(const char* filename) {
    FILE* textlayout = NULL;
    if ( (textlayout = fopen(filename,"rb")) ) {

	fseek(textlayout,0,SEEK_END);
	long fsz = ftell(textlayout);
	fseek(textlayout,0,SEEK_SET);

	allocd_text_layout = realloc(allocd_text_layout,fsz);
	fread(allocd_text_layout,fsz,1,textlayout);
	fclose(textlayout);
	text_layout = allocd_text_layout;

    } else text_layout = default_text_layout;
}
