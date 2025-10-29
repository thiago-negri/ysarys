#include "str.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

size_t
cstr_len(const char *cstr)
{
	const char *c = NULL;

	for (c = cstr; *c != '\0' && c - cstr < CSTR_LEN_MAX; c++)
		;

	return c - cstr;
}

int
str_alloc(const char *cstr, struct str **ret_dst)
{
	int r = 0;
	struct str *dst = NULL;
	size_t count = 0;

	count = cstr_len(cstr);

	dst = malloc(sizeof(struct str));
	if (dst == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	dst->count = count;
	dst->array = malloc(count + 1);
	if (dst->array == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	memcpy(dst->array, cstr, count);
	dst->array[count] = '\0';

	r = STR_OK;
	*ret_dst = dst;
_done:
	if (r != STR_OK && dst != NULL)
	{
		if (dst->array != NULL)
			free(dst->array);
		free(dst);
	}
	return r;
}

int
str_dup_alloc(struct str *src, struct str **ret_dst)
{
	int r = 0;
	struct str *dst = NULL;
	size_t count = 0;

	count = src->count;

	dst = malloc(sizeof(struct str));
	if (dst == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	dst->count = count;
	dst->array = malloc(count + 1);
	if (dst->array == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	memcpy(dst->array, src->array, count);
	dst->array[count] = '\0';

	r = STR_OK;
	*ret_dst = dst;
_done:
	if (r != STR_OK && dst != NULL)
	{
		if (dst->array != NULL)
			free(dst->array);
		free(dst);
	}
	return r;
}

int
str_slice_alloc(const char *cstr, size_t count, struct str **ret_dst)
{
	int r = 0;
	struct str *dst = NULL;

	assert(count < CSTR_LEN_MAX);

	dst = malloc(sizeof(struct str));
	if (dst == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	dst->count = count;
	dst->array = malloc(count + 1);
	if (dst->array == NULL)
	{
		r = STR_EOOM;
		goto _done;
	}

	memcpy(dst->array, cstr, count);
	dst->array[count] = '\0';

	r = STR_OK;
	*ret_dst = dst;
_done:
	if (r != STR_OK && dst != NULL)
	{
		if (dst->array != NULL)
			free(dst->array);
		free(dst);
	}
	return r;
}

void
str_free(struct str *str)
{
	free(str->array);
	free(str);
}

void
str_print(FILE *fd, struct str *str)
{
	char *cur = NULL;
	char *end = NULL;

	cur = &str->array[0];
	end = &str->array[str->count];
	for (; cur != end; cur++)
		fputc(*cur, fd);
}
