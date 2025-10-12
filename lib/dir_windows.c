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

/*
 *
 * THIS FILE NEEDS WORK.
 *
 * It's a copy-paste from another project, but I've changed the signature of
 * dir_* functions and I didn't update the Windows implementation, yet.
 *
 */

#include "dir.h"
#include <windows.h>

struct dir_handle_windows
{
	HANDLE handle;
	char *filename;
	size_t len;
};

dir_handle *
dir_first(const char *path, struct file_entry *out)
{
	WIN32_FIND_DATAA ffd;
	HANDLE handle;

	char *filename = malloc(MAX_PATH);
	size_t len = strlen(path);
	memcpy(filename, path, len);
	strcpy(&filename[len++], "\\*"); // We could do "\\*.lua" here

	handle = FindFirstFileA(filename, &ffd);
	if (INVALID_HANDLE_VALUE == handle)
	{
		TODO("INVALID_HANDLE_VALUE");
		free(filename);
		return NULL;
	}

	while (strcmp(ffd.cFileName, ".") == 0 ||
	       strcmp(ffd.cFileName, "..") == 0)
	{
		if (FindNextFileA(handle, &ffd) == 0)
		{
			DWORD error = GetLastError();
			if (error != ERROR_NO_MORE_FILES)
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
				             "dir_first.FindNextFileA: %d.",
				             error);
			}
			free(filename);
			FindClose(handle);
			return NULL;
		}
	}

	strcpy(&filename[len], ffd.cFileName);
	out->name = filename;

	if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		out->type = DIR_DIRECTORY;
	}
	else
	{
		out->type = DIR_FILE;
	}

	struct dir_handle_windows *win_handle = malloc(sizeof *win_handle);
	win_handle->handle = handle;
	win_handle->filename = filename;
	win_handle->len = len;

	return win_handle;
}

enum dir_rc
dir_next(dir_handle *opaque_handle, struct file_entry *out)
{
	struct dir_handle_windows *win_handle = opaque_handle;
	HANDLE handle = win_handle->handle;
	char *filename = win_handle->filename;
	size_t len = win_handle->len;

	WIN32_FIND_DATAA ffd;
	if (FindNextFileA(handle, &ffd) != 0)
	{
		strcpy(&filename[len], ffd.cFileName);
		out->name = filename;

		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			out->type = DIR_DIRECTORY;
		}
		else
		{
			out->type = DIR_FILE;
		}

		return DIR_OK;
	}

	DWORD error = GetLastError();
	if (error != ERROR_NO_MORE_FILES)
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
		             "dir_next.FindNextFileA: %d.", error);
		return DIR_ERROR;
	}

	return DIR_NO_MORE_FILES;
}

void
dir_close(dir_handle *opaque_handle)
{
	struct dir_handle_windows *win_handle = opaque_handle;
	FindClose(win_handle->handle);
	free(win_handle->filename);
	free(win_handle);
}
