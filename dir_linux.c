#ifdef __linux

#include "dir.h"
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct dir_handle_linux
{
	DIR *dir;
	char *filepath;
	size_t len;
};

dir_handle *
dir_first(const char *path, const char **ret_filename)
{
	DIR *dir = NULL;
	size_t len = 0;
	char *filepath = NULL;
	struct dir_handle_linux *handle = NULL;
	struct dirent *f = NULL;
	struct stat sb;

	dir = opendir(path);
	if (dir == NULL)
	{
		return NULL;
	}

	len = strlen(path);
	filepath = malloc(PATH_MAX);
	memcpy(filepath, path, len);
	filepath[len++] = '/';
	filepath[len] = 0;

	handle = malloc(sizeof *handle);
	handle->dir = dir;
	handle->filepath = filepath;
	handle->len = len;

	errno = 0;
	f = readdir(dir);
	if (f == NULL && errno != 0)
	{
		perror("dir_first.readdir");
	}

	while (f != NULL)
	{
		if (strcmp(f->d_name, ".") != 0 && strcmp(f->d_name, "..") != 0)
		{
			strcpy(&filepath[len], f->d_name);

			if (stat(filepath, &sb) != 0)
			{
				perror("dir_first.stat");
				break;
			}

			switch (sb.st_mode & __S_IFMT)
			{
				case __S_IFREG:
					*ret_filename = filepath;
					return handle;

				default:
					// We don't care.
					break;
			}
		}

		errno = 0;
		f = readdir(dir);
		if (f == NULL && errno != 0)
		{
			perror("dir_first.readdir");
		}
	}

	closedir(dir);
	free(handle);
	free(filepath);
	return NULL;
}

/* ERROR | OK */
enum dir_rc
dir_next(dir_handle *opaque_handle, const char **ret_filename)
{
	struct dir_handle_linux *handle = NULL;
	DIR *dir = NULL;
	char *filepath = NULL;
	size_t len = 0;
	struct dirent *f = NULL;
	struct stat sb;

	handle = opaque_handle;
	dir = handle->dir;
	filepath = handle->filepath;
	len = handle->len;

	errno = 0;
	f = readdir(dir);
	if (f == NULL && errno != 0)
	{
		perror("dir_next.readdir");
		return DIR_ERROR;
	}

	while (f != NULL)
	{
		if (strcmp(f->d_name, ".") != 0 && strcmp(f->d_name, "..") != 0)
		{
			strcpy(&filepath[len], f->d_name);

			if (stat(filepath, &sb) != 0)
			{
				perror("dir_next.stat");
				return DIR_ERROR;
			}

			switch (sb.st_mode & __S_IFMT)
			{
				case __S_IFREG:
					*ret_filename = filepath;
					return DIR_OK;

				default:
					// We don't care.
					break;
			}
		}

		errno = 0;
		f = readdir(dir);
		if (f == NULL && errno != 0)
		{
			perror("dir_next.readdir");
			return DIR_ERROR;
		}
	}

	*ret_filename = NULL;
	return DIR_OK;
}

void
dir_close(dir_handle *opaque_handle)
{
	struct dir_handle_linux *handle = opaque_handle;
	closedir(handle->dir);
	free(handle->filepath);
	free(handle);
}

#endif /* __linux */
