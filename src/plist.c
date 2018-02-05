#include "plist.h"
#include <malloc.h>
#include <string.h>

int plist_resize(plist_t* pl, int size) {

	int oldsz = pl->arraysize;
	pl->i = realloc(pl->i, size * sizeof(struct pl_item));
	if (pl->i == NULL) return 1;
	memset( pl->i + oldsz, 0, sizeof(struct pl_item) * (size - oldsz) );
	pl->arraysize = size;
	return 0;
}

int plist_init_empty(plist_t* pl) {

	pl->count = 0; pl->arraysize = 0;
	return plist_resize(pl, 8);
}

int plist_add(plist_t* pl, struct pl_item* i) {

	if (pl->count == pl->arraysize) {
	plist_resize(pl, pl->arraysize * 2);
	}

	memcpy(pl->i + (pl->count), i, sizeof(struct pl_item));
	pl->count++;
	return 0;
}

int plist_clear(plist_t* pl) {
	return plist_init_empty(pl);
}

int plist_del_m(plist_t* pl, unsigned int index, unsigned int count) {

	if (index >= pl->count) return 1;
	if (index + count > pl->count) count = pl->count - index;

	memcpy(pl->i + index, pl->i + index + count, pl->arraysize - (index + count) );
	pl->count -= count;

	if (pl->count < (pl->arraysize / 2)) plist_resize(pl,pl->arraysize / 2);
	return 0;
}

int plist_del(plist_t* pl, unsigned int index) { return plist_del_m(pl,index,1); }

int plist_count(plist_t* pl) { return pl->count; }

struct pl_item plist_get(plist_t* pl, unsigned int index) { return pl->i[index]; }
