#ifndef PLIST_H
#define PLIST_H
#include <stdint.h>

struct pl_item {
	int loaded;
	char filePath[256];
	char title[256];
	int length;
};

struct playlist {
	int count;
	int arraysize;
	struct pl_item* i;
};

typedef struct playlist plist_t;

int plist_init_empty(plist_t* pl);
int plist_init_sz(plist_t* pl, unsigned int size);

int plist_add(plist_t* pl, struct pl_item* i);

int plist_count(plist_t* pl);

struct pl_item plist_get(plist_t* pl, unsigned int index);

int plist_del_m(plist_t* pl, unsigned int index, unsigned int count);

int plist_del(plist_t* pl, unsigned int index);

#endif
