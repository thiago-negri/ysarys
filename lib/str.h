#ifndef STR_H
#define STR_H

#include <stdio.h>

struct str
{
	size_t count;
	char *array;
};

enum
{
	STR_OK,
	STR_EOOM
};

#define CSTR_LEN_MAX 8192

size_t cstr_len(const char *cstr);

int str_alloc(const char *cstr, struct str **ret_dst);

int str_dup_alloc(struct str *src, struct str **ret_dst);

int str_slice_alloc(const char *cstr, size_t count, struct str **ret_dst);

void str_free(struct str *str);

void str_print(FILE *fd, struct str *str);

#endif /* !STR_H */
