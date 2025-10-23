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
#include "../lib/date.h"
#include "../lib/dir.h"
#include "../lib/log.h"
#include "../lib/rule_lua.h"
#include <errno.h>
#include <stdio.h>

int
main(int argc, char *argv[])
{
	struct agenda_file *agenda = NULL;
	struct agenda_array *array = NULL;
	struct rule *rule = NULL;
	struct weekdate date = WEEKDATE_ZERO;
	struct weekdate today = WEEKDATE_ZERO;
	struct weekdate max_date = WEEKDATE_ZERO;
	const char *lua_error_str = NULL;
	dir_handle *rules_dir = NULL;
	struct file_entry rule_file = FILE_ENTRY_ZERO;
	time_t now = 0;
	size_t i = 0;
	int r = 0;
	int errno_ = 0;

	r = agenda_array_alloc(1, &array);
	if (r == AGENDA_OK)
		r = agenda_file_read_alloc(argv[1], &agenda, &errno_);
	switch (r)
	{
		case AGENDA_OK:
			break;

		case AGENDA_EOOM:
			log_error("Out of memory.");
			return -1;

		case AGENDA_EACCES:
			log_error("Permission error trying to read: %s.",
			          argv[1]);
			return -1;

		case AGENDA_ENOENT:
			log_error("File not found: %s.", argv[1]);
			return -1;

		case AGENDA_EERRNO:
			errno = errno_;
			log_error("IO error trying to read: %s.", argv[1]);
			perror("start");
			return -1;

		case AGENDA_EINVALHEAD:
			log_error("Invalid file format. Invalid header.");
			return -1;

		case AGENDA_EINVALENTRY:
			log_error("Invalid file format. Invalid entry.");
			return -1;
	}

	if (time(&now) == (time_t)-1)
	{
		log_error("Can't query current time.");
		return -1;
	}
	weekdate_from_time(now, &today);
	weekdate_add_days(&today, 60, &max_date);

	if (agenda->last_run.day == 0)
		weekdate_from_time(now, &date);
	else
	{
		weekdate_from_date(&agenda->last_run, &date);
		weekdate_next(&date);
	}

	rule_alloc(&rule);

	r = dir_first_alloc("rules.d", &rules_dir, &rule_file, &errno_);
	switch (r)
	{
		case DIR_OK_ROW:
		case DIR_OK_DONE:
			break;

		case DIR_EOOM:
			log_error("Out of memory.");
			return -1;

		case DIR_EACCES:
			log_error("Permission error trying to list directory: "
			          "rules.d.");
			return -1;

		case DIR_ENOENT:
			log_error("Directory not found: rules.d.");
			return -1;

		case DIR_EERRNO:
			errno = errno_;
			log_error("IO error trying to read: rules.d.");
			perror("dir_first_alloc");
			return -1;
	}

	while (r != DIR_OK_DONE)
	{
		if (rule_file.type == FILE_TYPE_DIRECTORY)
			continue;

		log_debug("Adding rule %s.", rule_file.name);
		r = rule_add_file(rule, rule_file.name, &lua_error_str);
		switch (r)
		{
			case RULE_OK:
				break;

			case RULE_EOOM:
				log_error("Out of memory.");
				return -1;

			case RULE_ELUA:
				log_error("Lua error: %s - %s.", rule_file.name,
				          lua_error_str);
				return -1;
		}

		r = dir_next(rules_dir, &rule_file, &errno_);
		switch (r)
		{
			case DIR_OK_ROW:
			case DIR_OK_DONE:
				break;

			case DIR_EOOM:
				log_error("Out of memory.");
				return -1;

			case DIR_EACCES:
				log_error("Permission error trying to list "
				          "directory: rules.d.");
				return -1;

			case DIR_ENOENT:
				log_error("Directory not found: rules.d.");
				return -1;

			case DIR_EERRNO:
				errno = errno_;
				log_error("IO error trying to read: rules.d.");
				perror("dir_next");
				return -1;
		}
	}

	if (rule->rule_count < 1)
	{
		log_error("No rule found.");
		return -1;
	}

	for (; date_compare((struct date *)&date, (struct date *)&max_date) < 0;
	     weekdate_next(&date))
	{
		r = rule_run(rule, &date, array, NULL, NULL);
		if (r != RULE_OK)
		{
			log_error("Rule error.");
			return -1;
		}
	}

	agenda_entry_sort(array->array, array->count);

	for (i = 0; i < array->count; i++)
	{
		date_fprintf(stdout, &array->array[i].date);
		fprintf(stdout, "\t%s\n", array->array[i].title);
	}

	return 0;
}
