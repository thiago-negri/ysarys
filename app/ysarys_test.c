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
