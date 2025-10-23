/* ISC License
 *
 * Copyright (c) 2025 Thiago Negri
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "agenda.h"
#include "date.h"
#include "scan.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int
_entry_compare(const void *a, const void *b)
{
	const struct agenda_entry *left = a;
	const struct agenda_entry *right = b;
	return date_compare(&left->date, &right->date);
}

static int
_file_read_alloc(const char *path, char **ret_buffer, size_t *ret_count,
                 int *reterr_errno)
{
	char *buffer = NULL;
	FILE *fd = NULL;
	size_t file_size = 0;
	int r = 0;

	fd = fopen(path, "rb");
	if (fd == NULL)
	{
		switch (errno)
		{
			case EACCES:
				r = AGENDA_EACCES;
				goto _done;

			case ENOENT:
				r = AGENDA_ENOENT;
				goto _done;

			default:
				if (reterr_errno != NULL)
					*reterr_errno = errno;
				r = AGENDA_EERRNO;
				goto _done;
		}
	}

	if (fseek(fd, 0, SEEK_END) != 0)
	{
		if (reterr_errno != NULL)
			*reterr_errno = errno;
		r = AGENDA_EERRNO;
		goto _done;
	}

	file_size = ftell(fd);

	buffer = malloc(file_size);
	if (buffer == NULL)
	{
		r = AGENDA_EOOM;
		goto _done;
	}

	if (fseek(fd, 0, SEEK_SET) != 0)
	{
		if (reterr_errno != NULL)
			*reterr_errno = errno;
		r = AGENDA_EERRNO;
		goto _done;
	}

	if (fread(buffer, 1, file_size, fd) != file_size)
	{
		if (reterr_errno != NULL)
			*reterr_errno = errno;
		r = AGENDA_EERRNO;
		goto _done;
	}

	r = AGENDA_OK;
_done:
	if (r == AGENDA_OK)
	{
		*ret_buffer = buffer;
		*ret_count = file_size;
	}
	else if (buffer != NULL)
		free(buffer);
	if (fd != NULL)
		fclose(fd);
	return r;
}

int
agenda_file_read_alloc(const char *path, struct agenda_file **ret_file,
                       int *reterr_errno)
{
	char *buffer = NULL;
	struct agenda_file *file = NULL;
	size_t i = 0;
	size_t mark = 0;
	size_t buffer_count = 0;
	size_t entry_count = 0;
	size_t entry_i = 0;
	int r = 0;

	r = _file_read_alloc(path, &buffer, &buffer_count, reterr_errno);
	if (r != AGENDA_OK)
		goto _done;

	for (i = 0; i < buffer_count; i++)
	{
		if (buffer[i] != '#')
			entry_count++;
		for (; i < buffer_count; i++)
			if (buffer[i] == '\n')
				break;
	}

	file = malloc(sizeof *file);
	if (file == NULL)
	{
		r = AGENDA_EOOM;
		goto _done;
	}
	file->last_run.day = 0;
	file->last_run.month = 0;
	file->last_run.year = 0;

	file->entry_count = entry_count;
	file->entry_array = malloc(sizeof *file->entry_array * entry_count);
	if (file->entry_array == NULL)
	{
		r = AGENDA_EOOM;
		goto _done;
	}
	for (i = 0; i < entry_count; i++)
	{
		file->entry_array[i].date.day = 0;
		file->entry_array[i].date.month = 0;
		file->entry_array[i].date.year = 0;
		file->entry_array[i].title = NULL;
		file->entry_array[i].tag_csv = NULL;
	}

	for (i = 0; i < buffer_count; i++)
	{
		if (buffer[i] == '#')
		{
			/* 29 = 19 for prefix + 10 for date */
			if (buffer_count - i >= 29 &&
			    strncmp(&buffer[i], "# ysarys: last_run ", 19) == 0)
			{
				r = scan_date(&buffer[i + 19], 10,
				              &file->last_run);
				if (r != SCAN_OK)
				{
					r = AGENDA_EINVALHEAD;
					goto _done;
				}
				for (i += 29; i < buffer_count; i++)
					if (buffer[i] == '\n')
						break;
			}
			else
			{
				r = AGENDA_EINVALHEAD;
				goto _done;
			}
		}
		/* 12 = 10 for date + 2 \t */
		else if (buffer_count - i < 12)
		{
			r = AGENDA_EINVALENTRY;
			goto _done;
		}
		else
		{
			r = scan_date(&buffer[i], 10,
			              &file->entry_array[entry_i].date);
			if (r != SCAN_OK)
			{
				r = AGENDA_EINVALENTRY;
				goto _done;
			}
			if (buffer[i + 10] != '\t')
			{
				r = AGENDA_EINVALENTRY;
				goto _done;
			}
			i += 11;
			mark = i;
			for (; i < buffer_count; i++)
				if (buffer[i] == '\t' || buffer[i] == '\n')
					break;
			r = str_slice_alloc(&buffer[mark], i - mark,
			                    &file->entry_array[entry_i].title);
			if (r != STR_OK)
			{
				r = AGENDA_EOOM;
				goto _done;
			}
			if (buffer[i] == '\t')
				i++;
			mark = i;
			for (; i < buffer_count; i++)
				if (buffer[i] == '\n')
					break;
			r = str_slice_alloc(
			    &buffer[mark], i - mark,
			    &file->entry_array[entry_i].tag_csv);
			if (r != STR_OK)
			{
				r = AGENDA_EOOM;
				goto _done;
			}
			entry_i++;
		}
		for (; i < buffer_count; i++)
			if (buffer[i] == '\n')
				break;
	}

	r = AGENDA_OK;
_done:
	if (r == AGENDA_OK)
		*ret_file = file;
	else if (file != NULL)
	{
		if (file->entry_array != NULL)
		{
			for (i = 0; i < entry_count; i++)
			{
				if (file->entry_array[i].title != NULL)
					str_free(file->entry_array[i].title);
				if (file->entry_array[i].tag_csv != NULL)
					str_free(file->entry_array[i].tag_csv);
			}
			free(file->entry_array);
		}
		free(file);
	}
	if (buffer != NULL)
		free(buffer);
	return r;
}

int
agenda_file_write(const char *path, struct agenda_file *file, int *reterr_errno)
{
	FILE *fd = NULL;
	size_t i = 0;
	int r = 0;

	fd = fopen(path, "wb");
	if (fd == NULL)
	{
		switch (errno)
		{
			case EACCES:
				r = AGENDA_EACCES;
				goto _done;

			case ENOENT:
				r = AGENDA_ENOENT;
				goto _done;

			default:
				if (reterr_errno != NULL)
					*reterr_errno = errno;
				r = AGENDA_EERRNO;
				goto _done;
		}
	}

	fprintf(fd, "# ysarys: last_run ");
	date_fprintf(fd, &file->last_run);
	fprintf(fd, "\n");

	for (i = 0; i < file->entry_count; i++)
	{
		date_fprintf(fd, &file->entry_array[i].date);
		fprintf(fd, "\t%s\t%s\n", file->entry_array[i].title,
		        file->entry_array[i].tag_csv);
	}

	r = AGENDA_OK;
_done:
	if (fd != NULL)
		fclose(fd);
	return r;
}

int
agenda_entry_sort(struct agenda_entry *array, size_t count)
{
	qsort(array, count, sizeof *array, _entry_compare);
	return AGENDA_OK;
}

void
agenda_file_free(struct agenda_file *file)
{
	size_t i = 0;

	if (file->entry_array != NULL)
	{
		for (i = 0; i < file->entry_count; i++)
		{
			if (file->entry_array[i].title != NULL)
				free(file->entry_array[i].title);
			file->entry_array[i].title = NULL;

			if (file->entry_array[i].tag_csv != NULL)
				free(file->entry_array[i].tag_csv);
			file->entry_array[i].tag_csv = NULL;
		}
		free(file->entry_array);
		file->entry_array = NULL;
	}
	file->entry_count = 0;
	free(file);
}

int
agenda_array_alloc(size_t capacity, struct agenda_array **ret_array)
{
	int r = 0;
	struct agenda_array *array = NULL;

	array = malloc(sizeof *array);
	if (array == NULL)
	{
		r = AGENDA_EOOM;
		goto _done;
	}

	array->count = 0;
	array->capacity = capacity;
	array->array = malloc(sizeof *array->array * capacity);
	if (array->array == NULL)
	{
		r = AGENDA_EOOM;
		goto _done;
	}

	r = AGENDA_OK;
	*ret_array = array;
_done:
	if (r != AGENDA_OK)
	{
		if (array != NULL)
		{
			if (array->array != NULL)
				free(array->array);
			free(array);
		}
	}
	return r;
}

int
agenda_array_push_alloc(struct agenda_array *array, struct agenda_entry *value)
{
	int r = 0;
	struct agenda_entry *new_array = NULL;
	char *new_title = NULL;
	char *new_tag_csv = NULL;
	size_t new_capacity = 0;

	if (array->count >= array->capacity)
	{
		new_capacity = (array->capacity + 1) * 1.25f;
		new_array =
		    realloc(array->array, sizeof *array->array * new_capacity);
		if (new_array == NULL)
		{
			r = AGENDA_EOOM;
			goto _done;
		}

		array->capacity = new_capacity;
		array->array = new_array;
	}

	array->array[array->count].date = value->date;
	array->array[array->count].title = value->title;
	array->array[array->count].tag_csv = value->tag_csv;
	array->count += 1;

	r = AGENDA_OK;
_done:
	if (r != AGENDA_OK)
	{
		if (new_title != NULL)
			free(new_title);
		if (new_tag_csv != NULL)
			free(new_tag_csv);
	}
	return r;
}

void
agenda_array_free(struct agenda_array *array)
{
	size_t i = 0;

	for (i = 0; i < array->count; i++)
	{
		str_free(array->array[i].title);
		str_free(array->array[i].tag_csv);
	}

	free(array->array);
	free(array);
}
