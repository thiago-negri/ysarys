#ifdef WIN32

#include "dir.h"
#include <windows.h>

struct dir_handle_windows
{
	HANDLE handle;
	char *filename;
	size_t len;
};

dir_handle *
dir_first(const char *path, const char **ret_filename)
{
	HANDLE handle = INVALID_HANDLE_VALUE;
	char *filename = NULL;
	size_t len = 0;
	struct dir_handle_windows *win_handle = NULL;
	WIN32_FIND_DATAA ffd;
	DWORD error;

	filename = malloc(MAX_PATH);
	len = strlen(path);
	memcpy(filename, path, len);
	strcpy(&filename[len++], "\\*");

	handle = FindFirstFileA(filename, &ffd);
	if (handle == INVALID_HANDLE_VALUE)
	{
		free(filename);
		*ret_filename = NULL;
		return NULL;
	}

	while (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0 ||
	       ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY))
	{
		if (FindNextFileA(handle, &ffd) == 0)
		{
			error = GetLastError();
			if (error != ERROR_NO_MORE_FILES)
			{
				fprintf(stderr, "FindNextFileA: %d\n", error);
			}
			free(filename);
			FindClose(handle);
			*ret_filename = NULL;
			return NULL;
		}
	}

	strcpy(&filename[len], ffd.cFileName);

	win_handle = malloc(sizeof *win_handle);
	win_handle->handle = handle;
	win_handle->filename = filename;
	win_handle->len = len;

	*ret_filename = filename;
	return win_handle;
}

enum dir_rc
dir_next(dir_handle *opaque_handle, const char **ret_filename)
{
	struct dir_handle_windows *win_handle = NULL;
	char *filename = NULL;
	size_t len = 0;
	HANDLE handle;
	WIN32_FIND_DATAA ffd;
	DWORD error;

	win_handle = opaque_handle;
	handle = win_handle->handle;
	filename = win_handle->filename;
	len = win_handle->len;

	while (FindNextFileA(handle, &ffd) != 0)
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
		{
			strcpy(&filename[len], ffd.cFileName);
			*ret_filename = filename;
			return DIR_OK;
		}
	}

	error = GetLastError();
	if (error != ERROR_NO_MORE_FILES)
	{
		fprintf(stderr, "FindNextFileA: %d\n", error);
		return DIR_ERROR;
	}

	*ret_filename = NULL;
	return DIR_OK;
}

void
dir_close(dir_handle *opaque_handle)
{
	struct dir_handle_windows *win_handle = opaque_handle;
	FindClose(win_handle->handle);
	free(win_handle->filename);
	free(win_handle);
}

#endif /* WIN32 */
