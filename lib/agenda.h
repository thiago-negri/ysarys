#ifndef AGENDA_H
#define AGENDA_H

#include "date.h"

struct agenda_entry
{
	struct date date;
	char *title;
	char *tag_csv;
};

struct agenda_file
{
	struct date last_run;
	size_t entry_count;
	struct agenda_entry *entry_array;
};

enum
{
	AGENDA_OK = 0,
	AGENDA_EOOM,
	AGENDA_EACCES,
	AGENDA_ENOENT,
	AGENDA_EERRNO,
	AGENDA_EINVALHEAD,
	AGENDA_EINVALENTRY
};

/* TODO(tnegri): Add a `struct agenda_file_err` that has file number, etc. */
int agenda_file_read_alloc(const char *path, struct agenda_file **ret_file,
                           int *reterr_errno);

int agenda_file_write(const char *path, struct agenda_file *file,
                      int *reterr_errno);

int agenda_entry_sort(struct agenda_entry *array, size_t count);

void agenda_file_free(struct agenda_file *file);

#endif /* !AGENDA_H */
