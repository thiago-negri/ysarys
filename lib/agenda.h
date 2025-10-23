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

#ifndef AGENDA_H
#define AGENDA_H

#include "date.h"

struct agenda_entry
{
	struct date date;
	char *title;
	char *tag_csv;
};

#define AGENDA_ENTRY_ZERO { DATE_ZERO, NULL, NULL }

struct agenda_file
{
	struct date last_run;
	size_t entry_count;
	struct agenda_entry *entry_array;
};

struct agenda_array
{
	size_t count;
	size_t capacity;
	struct agenda_entry *array;
};

enum
{
	AGENDA_OK = 0,
	AGENDA_EOOM,       /* Out of memory */
	AGENDA_EACCES,     /* Permission denied */
	AGENDA_ENOENT,     /* No such file */
	AGENDA_EERRNO,     /* IO error, check reterr_errno */
	AGENDA_EINVALHEAD, /* Invalid file format, invalid header */
	AGENDA_EINVALENTRY /* Invalid file format, invalid entry */
};

/* TODO(tnegri): Add a `struct agenda_file_err` that has file number, etc. */
int agenda_file_read_alloc(const char *path, struct agenda_file **ret_file,
                           int *reterr_errno);

int agenda_file_write(const char *path, struct agenda_file *file,
                      int *reterr_errno);

int agenda_entry_sort(struct agenda_entry *array, size_t count);

void agenda_file_free(struct agenda_file *file);

int agenda_array_alloc(size_t capacity, struct agenda_array **ret_array);

int agenda_array_push_alloc(struct agenda_array *array,
                            struct agenda_entry *value);

void agenda_array_free(struct agenda_array *array);

#endif /* !AGENDA_H */
