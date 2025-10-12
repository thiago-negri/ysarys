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

#include "dir.h"
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct dir_handle_linux
{
	DIR *dir;
	char *filepath;
	size_t len;
};

int
dir_first_alloc(const char *path, dir_handle **ret_handle,
                struct file_entry *ret_entry, int *reterr_errno)
{
	DIR *dir = NULL;
	char *filepath = NULL;
	struct dir_handle_linux *handle = NULL;
	struct dirent *inode = NULL;
	struct stat sb = { 0 };
	size_t len = 0;
	int r = 0;

	dir = opendir(path);
	if (dir == NULL)
	{
		switch (errno)
		{
			case EACCES:
				r = DIR_EACCES;
				goto _done;

			case ENOENT:
				r = DIR_ENOENT;
				goto _done;

			default:
				if (reterr_errno != NULL)
					*reterr_errno = errno;
				r = DIR_EERRNO;
				goto _done;
		}
	}

	len = strlen(path);
	filepath = malloc(PATH_MAX);
	if (filepath == NULL)
	{
		r = DIR_EOOM;
		goto _done;
	}
	memcpy(filepath, path, len);
	filepath[len++] = '/';
	filepath[len] = 0;

	handle = malloc(sizeof *handle);
	if (handle == NULL)
	{
		r = DIR_EOOM;
		goto _done;
	}
	handle->dir = dir;
	handle->filepath = filepath;
	handle->len = len;

	errno = 0;
	inode = readdir(dir);
	if (inode == NULL && errno != 0)
	{
		if (reterr_errno != NULL)
			*reterr_errno = errno;
		r = DIR_EERRNO;
		goto _done;
	}

	while (inode != NULL)
	{
		if (strcmp(inode->d_name, ".") != 0 &&
		    strcmp(inode->d_name, "..") != 0)
		{
			strcpy(&filepath[len], inode->d_name);

			if (stat(filepath, &sb) != 0)
			{
				if (reterr_errno != NULL)
					*reterr_errno = errno;
				r = DIR_EERRNO;
				goto _done;
			}

			if (S_ISREG(sb.st_mode))
			{
				ret_entry->type = FILE_TYPE_FILE;
				ret_entry->name = filepath;
				*ret_handle = handle;
				r = DIR_OK_ROW;
				goto _done;
			}

			if (S_ISDIR(sb.st_mode))
			{
				ret_entry->type = FILE_TYPE_DIRECTORY;
				ret_entry->name = filepath;
				*ret_handle = handle;
				r = DIR_OK_ROW;
				goto _done;
			}
		}

		errno = 0;
		inode = readdir(dir);
		if (inode == NULL && errno != 0)
		{
			if (reterr_errno != NULL)
				*reterr_errno = errno;
			r = DIR_EERRNO;
			goto _done;
		}
	}

	/* If we are out of the loop, it means we're done with the directory.
	 * So directory is either empty or only contains inodes we don't care
	 * about. In such case we don't even return the handle back. */
	r = DIR_OK_DONE;
_done:
	if (r != DIR_OK_ROW)
	{
		if (handle != NULL)
			free(handle);
		if (filepath != NULL)
			free(filepath);
		if (dir != NULL)
			closedir(dir);
		*ret_handle = NULL;
	}
	return r;
}

int
dir_next(dir_handle *opaque_handle, struct file_entry *ret_entry,
         int *reterr_errno)
{
	struct dir_handle_linux *handle = NULL;
	DIR *dir = NULL;
	char *filepath = NULL;
	struct stat sb = { 0 };
	struct dirent *f = NULL;
	size_t len = 0;
	int r = 0;

	handle = opaque_handle;
	dir = handle->dir;
	filepath = handle->filepath;
	len = handle->len;

	errno = 0;
	f = readdir(dir);
	if (f == NULL && errno != 0)
	{
		if (reterr_errno != NULL)
			*reterr_errno = errno;
		r = DIR_EERRNO;
		goto _done;
	}

	while (f != NULL)
	{
		if (strcmp(f->d_name, ".") != 0 && strcmp(f->d_name, "..") != 0)
		{
			strcpy(&filepath[len], f->d_name);

			if (stat(filepath, &sb) != 0)
			{
				if (reterr_errno != NULL)
					*reterr_errno = errno;
				r = DIR_EERRNO;
				goto _done;
			}

			if (S_ISREG(sb.st_mode))
			{
				ret_entry->type = FILE_TYPE_FILE;
				ret_entry->name = filepath;
				r = DIR_OK_ROW;
				goto _done;
			}

			if (S_ISDIR(sb.st_mode))
			{
				ret_entry->type = FILE_TYPE_DIRECTORY;
				ret_entry->name = filepath;
				r = DIR_OK_ROW;
				goto _done;
			}
		}

		errno = 0;
		f = readdir(dir);
		if (f == NULL && errno != 0)
		{
			if (reterr_errno != NULL)
				*reterr_errno = errno;
			r = DIR_EERRNO;
			goto _done;
		}
	}

_done:
	return r;
}

void
dir_close(dir_handle *opaque_handle)
{
	struct dir_handle_linux *handle = NULL;

	handle = opaque_handle;

	if (handle->dir != NULL)
	{
		closedir(handle->dir);
		handle->dir = NULL;
	}

	if (handle->filepath != NULL)
	{
		free(handle->filepath);
		handle->filepath = NULL;
	}

	free(handle);
}
