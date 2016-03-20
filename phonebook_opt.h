#ifndef _PHONEBOOK_H
#define _PHONEBOOK_H

#define MAX_LAST_NAME_SIZE 16

/* TODO: After modifying the original version, uncomment the following
 * line to set OPT properly */
#define OPT 1

typedef struct __DETAILED_INFO {
    char firstName[16];
    char email[16];
    char phone[10];
    char cell[10];
    char addr1[16];
    char addr2[16];
    char city[16];
    char state[2];
    char zip[5];
} DetailInfo;

typedef struct __PHONE_BOOK_ENTRY {
    char lastName[MAX_LAST_NAME_SIZE];
    unsigned int hash;
    struct DetailInfo *pinfo;
    struct __PHONE_BOOK_ENTRY *pNext;
} entry;

typedef struct __HASH_ENTRY {
	entry *pHead;
	entry *pTail;
} HashEntry;

static inline unsigned int djb2_hash (unsigned char *str) {
	unsigned int hash = 5381;
	int c;
	while ( (c = *str++) )
		hash = ((hash << 5 ) + hash ) + c; /* hash * 33 + c */
	return hash;
}

entry *findName(char lastname[], entry *pHead);
entry *append(char lastName[], entry *e);

#endif
