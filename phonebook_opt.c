#include <stdlib.h>
#include <string.h>

#include "phonebook_opt.h"

#define HASH_SIZE    (1UL << 10)
HashEntry HashTable[HASH_SIZE];

unsigned int djb2_hash ( unsigned char *str ) {
	unsigned int hash = 5381;
	int c;
	while ( (c = *str++) ) {
		hash = ((hash << 5 ) + hash ) + c; /* hash * 33 + c */
	}
	return hash;
}

/* FILL YOUR OWN IMPLEMENTATION HERE! */
entry *findName(char lastname[], entry *pHead)
{
	unsigned int index = (djb2_hash((unsigned char*) lastname)) % HASH_SIZE;
	entry* iter = HashTable[index].pHead;
	while (iter) {
		if ( strcasecmp(lastname, iter->lastName) == 0) {
			return iter;
		}
		else
			iter = iter->pNext;
	}
	return NULL;
}

entry *append(char lastName[], entry *e)
{
	unsigned int index = (djb2_hash((unsigned char*) lastName)) % HASH_SIZE;
	e = HashTable[index].pTail;
	if (e) { /* hash collision */
		e->pNext = (entry *) malloc (sizeof(entry));
		e = e->pNext;
	} else { /* no hash collision */
		HashTable[index].pHead = (entry *) malloc (sizeof(entry));
		e = HashTable[index].pHead;
		HashTable[index].pTail = HashTable[index].pHead;
	}
	strcpy(e->lastName, lastName);
	e->pNext = NULL;
	return e;
}
