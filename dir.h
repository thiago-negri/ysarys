#ifndef DIR_H
#define DIR_H

enum dir_rc
{
	DIR_OK,
	DIR_ERROR,
};

typedef void dir_handle;

dir_handle *dir_first(const char *path, const char **ret_filename);
enum dir_rc dir_next(dir_handle *handle, const char **ret_filename);
void dir_close(dir_handle *handle);

#endif // !DIR_H
