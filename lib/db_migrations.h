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

#ifndef DB_MIGRATIONS_H
#define DB_MIGRATIONS_H

#include <stdlib.h>

struct db_migration
{
	const char *name;
	const char *sql;
};

struct db_migration db_migrations[] = {
	{ "20241129013908_init.sql",
	  /* Only active rules */
	  "CREATE TABLE scheduler("
	  "id INTEGER PRIMARY KEY,"
	  "rule TEXT,"
	  "description TEXT,"
	  "tags_csv TEXT,"
	  "monetary_value INT,"
	  "created_at INT"
	  ");"
	  /* Deleted rules */
	  "CREATE TABLE scheduler_archive("
	  "id INTEGER PRIMARY KEY,"
	  "rule TEXT,"
	  "description TEXT,"
	  "tags_csv TEXT,"
	  "monetary_value INT,"
	  "created_at INT,"
	  "archived_at INT"
	  ");"
	  /* To do entries */
	  "CREATE TABLE agenda("
	  "id INTEGER PRIMARY KEY,"
	  "scheduler_id INT,"
	  "scheduler_archive_id INT,"
	  "description TEXT,"
	  "tags_csv TEXT,"
	  "monetary_value INT,"
	  "due_at INT,"
	  "FOREIGN KEY(scheduler_id)REFERENCES scheduler(id),"
	  "FOREIGN KEY(scheduler_archive_id)REFERENCES scheduler_archive(id)"
	  ");"
	  /* Completed entries */
	  "CREATE TABLE agenda_archive("
	  "id INTEGER PRIMARY KEY,"
	  "scheduler_id INT,"
	  "scheduler_archive_id INT,"
	  "description TEXT,"
	  "tags_csv TEXT,"
	  "monetary_value INT,"
	  "due_at INT,"
	  "archived_at INT,"
	  "FOREIGN KEY(scheduler_id)REFERENCES scheduler(id),"
	  "FOREIGN KEY(scheduler_archive_id)REFERENCES scheduler_archive(id)"
	  ");"
	  /* Controls when the scheduler ran */
	  "CREATE TABLE scheduler_control("
	  "id INTEGER PRIMARY KEY,"
	  "last_run_at_timestamp INT"
	  ");" },

	{ NULL, NULL }
};

#endif /* !DB_MIGRATIONS_H */
