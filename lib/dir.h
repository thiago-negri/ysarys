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

#ifndef DIR_H
#define DIR_H

enum
{
	FILE_TYPE_DIRECTORY,
	FILE_TYPE_FILE
};

enum
{
	DIR_OK_ROW = 0,
	DIR_OK_DONE,
	DIR_EOOM,
	DIR_EACCES,
	DIR_ENOENT,
	DIR_EERRNO
};

struct file_entry
{
	int type;
	char *name;
};

#define FILE_ENTRY_ZERO { 0, NULL }

typedef void dir_handle;

int dir_first_alloc(const char *path, dir_handle **ret_handle,
                    struct file_entry *ret_entry, int *reterr_errno);

int dir_next(dir_handle *handle, struct file_entry *ret_entry,
             int *reterr_errno);

void dir_close(dir_handle *handle);

#endif /* !DIR_H */
