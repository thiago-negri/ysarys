#include "../lib/agenda.h"
#include <stdio.h>

/*
 * Testing the new file format for the agenda.
 */
int
main(void)
{
	struct agenda_file *agenda = NULL;
	int r = 0;

	printf("read\n");
	r = agenda_file_read_alloc("agenda.txt", &agenda, NULL);
	if (r != AGENDA_OK)
	{
		perror("read");
		return -1;
	}

	printf("sort\n");
	agenda_entry_sort(agenda->entry_array, agenda->entry_count);

	printf("write\n");
	r = agenda_file_write("agenda_out.txt", agenda, NULL);
	if (r != AGENDA_OK)
	{
		perror("write");
		return -1;
	}

	printf("free\n");
	agenda_file_free(agenda);

	return 0;
}
