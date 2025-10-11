#include "../lib/date.h"
#include "../lib/db_migrate.h"
#include "../lib/intdef.h"
#include "../lib/log.h"
#include "../lib/rule.h"
#include "../lib/scan.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

enum
{
	YSARYS_OK = 0,
	YSARYS_E
};

struct scheduler
{
	int id;
	const char *rule;
	const char *description;
	const char *tags_csv;
	int monetary_value;
};

void
sqlite_print_error(sqlite3 *db, const char *tag)
{
	int errcode = 0;
	const char *errmsg = NULL;

	errcode = sqlite3_extended_errcode(db);
	errmsg = sqlite3_errmsg(db);

	fprintf(stderr, "%s %d: %s\n", tag, errcode, errmsg);
}

void
usage(void)
{
	fprintf(stderr, "Usage: ysarys <db> [command]\n");
}

static int
agenda_list(sqlite3 *db)
{
	(void)db;
	/* TODO(tnegri): agenda_list */
	return YSARYS_E;
}

static int
agenda_archive(sqlite3 *db, int agenda_id)
{
	const char sql_archive[] =
	    "INSERT INTO agenda_archive (scheduler_id, scheduler_archive_id, "
	    "description, tags_csv, monetary_value, due_at, archived_at) "
	    "SELECT scheduler_id, scheduler_archive_id, description, tags_csv, "
	    "monetary_value, due_at, ? FROM agenda WHERE id = ?";
	time_t archived_at = 0;
	sqlite3_stmt *stmt = NULL;
	int r = 0;

	archived_at = time(NULL);
	if (archived_at == ((time_t)-1))
	{
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_prepare_v2(db, sql_archive, sizeof(sql_archive), &stmt,
	                       NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_archive.prepare");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_bind_int64(stmt, 1, archived_at);
	if (r == SQLITE_OK)
		r = sqlite3_bind_int(stmt, 2, agenda_id);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_archive.bind");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_step(stmt);
	if (r != SQLITE_DONE)
	{
		sqlite_print_error(db, "agenda_archive.step");
		r = YSARYS_E;
		goto _done;
	}

	r = YSARYS_OK;
_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

static int
agenda_delete(sqlite3 *db, int agenda_id)
{
	const char sql_delete[] = "DELETE FROM agenda WHERE id = ?";
	sqlite3_stmt *stmt = NULL;
	int r = 0;

	r = sqlite3_prepare_v2(db, sql_delete, sizeof(sql_delete), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_delete.prepare");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_bind_int(stmt, 1, agenda_id);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_delete.bind");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_step(stmt);
	if (r != SQLITE_DONE)
	{
		sqlite_print_error(db, "agenda_delete.step");
		r = YSARYS_E;
		goto _done;
	}

	r = YSARYS_OK;
_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

static int
agenda_rm(sqlite3 *db, int argc, const char *argv[])
{
	int agenda_id = 0;
	int r = 0;

	if (argc != 1)
	{
		r = YSARYS_E;
		goto _done;
	}

	r = scan_int((const unsigned char *)argv[0], strlen(argv[0]),
	             &agenda_id);
	if (r != SCAN_OK)
	{
		r = YSARYS_E;
		goto _done;
	}

	r = agenda_archive(db, agenda_id);
	if (r != YSARYS_OK)
		goto _done;

	r = agenda_delete(db, agenda_id);
	if (r != YSARYS_OK)
		goto _done;

	r = YSARYS_OK;
_done:
	return YSARYS_E;
}

static int
agenda_add(sqlite3 *db, int argc, const char *argv[])
{
	(void)db;
	(void)argc;
	(void)argv;
	/* TODO(tnegri): agenda_add */
	return YSARYS_E;
}

static int
scheduler_list(sqlite3 *db)
{
	(void)db;
	/* TODO(tnegri): scheduler_list */
	return YSARYS_E;
}

static int
scheduler_rm(sqlite3 *db, int argc, const char *argv[])
{
	(void)db;
	(void)argc;
	(void)argv;
	/* TODO(tnegri): scheduler_rm */
	return YSARYS_E;
}

static int
scheduler_add(sqlite3 *db, int argc, const char *argv[])
{
	(void)db;
	(void)argc;
	(void)argv;
	/* TODO(tnegri): scheduler_add */
	return YSARYS_E;
}

sqlite3_int64
select_last_run_time(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] =
	    "SELECT last_run_at_timestamp FROM scheduler_control WHERE id = 1";
	int r = 0;
	sqlite3_int64 last_run = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "select_last_run_time.prepare");
		last_run = 0;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			last_run = 0;
			goto _done;

		case SQLITE_ROW:
			last_run = sqlite3_column_int64(stmt, 0);
			goto _done;

		default:
			sqlite_print_error(db, "select_last_run_time.step");
			last_run = 0;
			goto _done;
	}

_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);

	return last_run;
}

int
select_scheduler(sqlite3 *db, sqlite3_stmt **stmt)
{
	const char sql[] = "SELECT id, rule, description, tags_csv, "
	                   "monetary_value FROM scheduler";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "select_scheduler.prepare");
		if (*stmt != NULL)
			sqlite3_finalize(*stmt);

		r = YSARYS_E;
		goto _done;
	}

	r = YSARYS_OK;
_done:
	return r;
}

void
next(struct date *date)
{
	date->week_day = (date->week_day + 1) % 7;

	if (date->day < date_month_last_day(date->year, date->month))
	{
		date->day += 1;
		return;
	}
	date->day = 1;

	if (date->month < MONTH_DECEMBER)
	{
		date->month = date->month + 1;
		return;
	}
	date->month = MONTH_JANUARY;

	date->year += 1;
}

int
compare(struct date *a, struct date *b)
{
	if (a->year < b->year)
		return -1;
	if (a->year > b->year)
		return 1;
	if (a->month < b->month)
		return -1;
	if (a->month > b->month)
		return 1;
	if (a->day < b->day)
		return -1;
	if (a->day > b->day)
		return 1;
	return 0;
}

static int
agenda_exists(sqlite3 *db, sqlite_int64 scheduler_id, sqlite_int64 due_at,
              int *ret_exists)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT 1 FROM agenda WHERE scheduler_id = ? AND "
	                   "due_at = ? LIMIT 1";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_exists.prepare");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_bind_int64(stmt, 1, scheduler_id);
	if (r == SQLITE_OK)
		r = sqlite3_bind_int64(stmt, 2, due_at);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_exists.bind");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			*ret_exists = 0;
			r = YSARYS_OK;
			goto _done;

		case SQLITE_ROW:
			*ret_exists = 1;
			r = YSARYS_OK;
			goto _done;

		default:
			sqlite_print_error(db, "agenda_exists.step");
			r = YSARYS_E;
			goto _done;
	}

_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

static int
agenda_insert(sqlite3 *db, sqlite_int64 scheduler_id,
              const unsigned char *description, usize description_count,
              const unsigned char *tags_csv, usize tags_csv_count,
              sqlite_int64 monetary_value, sqlite_int64 due_at)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "INSERT INTO agenda (scheduler_id, description, "
	                   "tags_csv, monetary_value, due_at)"
	                   " VALUES (?, ?, ?, ?, ?)";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_insert.prepare");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_bind_int64(stmt, 1, scheduler_id);
	if (r == SQLITE_OK)
		r = sqlite3_bind_blob(stmt, 2, description, description_count,
		                      SQLITE_STATIC);
	if (r == SQLITE_OK)
		r = sqlite3_bind_blob(stmt, 3, tags_csv, tags_csv_count,
		                      SQLITE_STATIC);
	if (r == SQLITE_OK)
		r = sqlite3_bind_int64(stmt, 4, monetary_value);
	if (r == SQLITE_OK)
		r = sqlite3_bind_int64(stmt, 5, due_at);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_insert.bind");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = YSARYS_OK;
			goto _done;

		default:
			sqlite_print_error(db, "agenda_insert.step");
			r = YSARYS_E;
			goto _done;
	}

_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

int
update_last_run(sqlite3 *db, struct date *date)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "REPLACE INTO scheduler_control (id, "
	                   "last_run_at_timestamp) VALUES (1, ?)";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "update_last_run.prepare");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_bind_int64(stmt, 1, date_to_time(date));
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "update_last_run.bind");
		r = YSARYS_E;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = YSARYS_OK;
			goto _done;

		default:
			sqlite_print_error(db, "update_last_run.step");
			r = YSARYS_E;
			goto _done;
	}

	r = YSARYS_OK;
_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

int
scheduler_populate(sqlite3 *db, struct date *today, int populate_from_today)
{
	struct date check_start_date = DATE_ZERO;
	struct date *check_start = NULL;
	struct date check_end = DATE_ZERO;
	struct date current = DATE_ZERO;
	sqlite3_stmt *stmt_scheduler = NULL;
	sqlite3_int64 last_run = 0;
	sqlite3_int64 scheduler_id = 0;
	const unsigned char *scheduler_rule = NULL;
	int scheduler_rule_count = 0;
	const unsigned char *scheduler_description = NULL;
	int scheduler_description_count;
	const unsigned char *scheduler_tags_csv = NULL;
	int scheduler_tags_csv_count = 0;
	sqlite3_int64 scheduler_monetary_value = 0;
	struct rule *rule = NULL;
	time_t due_at = 0;
	int exists = 0;
	int r = 0;

	check_start = today;

	last_run = select_last_run_time(db);
	if (last_run != 0)
	{
		date_from_time(last_run, &check_start_date);
		if (!populate_from_today ||
		    date_compare(&check_start_date, today) < 0)
			check_start = &check_start_date;
	}

	date_add_days(today, 60, &check_end);

	r = select_scheduler(db, &stmt_scheduler);
	if (r != YSARYS_OK)
		goto _done;

	while ((r = sqlite3_step(stmt_scheduler)) == SQLITE_ROW)
	{
		scheduler_id = sqlite3_column_int64(stmt_scheduler, 0);
		scheduler_rule = sqlite3_column_text(stmt_scheduler, 1);
		scheduler_rule_count = sqlite3_column_bytes(stmt_scheduler, 1);
		scheduler_description = sqlite3_column_text(stmt_scheduler, 2);
		scheduler_description_count =
		    sqlite3_column_bytes(stmt_scheduler, 2);
		scheduler_tags_csv = sqlite3_column_text(stmt_scheduler, 3);
		scheduler_tags_csv_count =
		    sqlite3_column_bytes(stmt_scheduler, 3);
		scheduler_monetary_value =
		    sqlite3_column_int64(stmt_scheduler, 4);

		r = rule_compile(scheduler_rule, scheduler_rule_count, &rule);
		if (r != RULE_OK)
		{
			log_error(
			    "Failed to compile rule '%s'. Return code: %d",
			    scheduler_rule, r);
			r = YSARYS_E;
			goto _done;
		}

		for (current = *check_start; compare(&current, &check_end) <= 0;
		     next(&current))
		{
			if (rule_matches(rule, &current))
			{
				due_at = date_to_time(&current);

				r = agenda_exists(db, scheduler_id, due_at,
				                  &exists);
				if (r != YSARYS_OK)
					goto _done;

				/*
				printf("%d-%d-%d <%d> %lld: (%s) '%s' [%s]
				$%lld\n", current.year, current.month,
				current.day, exists, scheduler_id,
				scheduler_rule, scheduler_description,
				scheduler_tags_csv, scheduler_monetary_value);
				       */

				if (!exists)
				{
					r = agenda_insert(
					    db, scheduler_id,
					    scheduler_description,
					    scheduler_description_count,
					    scheduler_tags_csv,
					    scheduler_tags_csv_count,
					    scheduler_monetary_value, due_at);
					if (r != YSARYS_OK)
						goto _done;
				}
			}
		}

		rule_free(rule);
	}

	r = update_last_run(db, &check_end);
	if (r != YSARYS_OK)
		goto _done;

	r = YSARYS_OK;
_done:
	if (stmt_scheduler != NULL)
		sqlite3_finalize(stmt_scheduler);
	return r;
}

static int
agenda_list_due(sqlite3 *db, struct date *today, struct date *near_future,
                struct date *future)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT id, scheduler_id, scheduler_archive_id, "
	                   "description, tags_csv, monetary_value, due_at"
	                   " FROM agenda ORDER BY due_at DESC, id DESC";
	sqlite_int64 agenda_id = 0;
	const unsigned char *agenda_description = NULL;
	sqlite_int64 agenda_due_at = 0;
	struct date date = DATE_ZERO;
	int print_details = 0;
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		sqlite_print_error(db, "agenda_list_due.prepate");
		r = YSARYS_E;
		goto _done;
	}

	while ((r = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		agenda_id = sqlite3_column_int64(stmt, 0);
		agenda_description = sqlite3_column_text(stmt, 3);
		agenda_due_at = sqlite3_column_int64(stmt, 6);

		print_details = 0;

		date_from_time(agenda_due_at, &date);
		if (date_compare(today, &date) >= 0)
		{
			fprintf(stdout, "\x001b[31mDue      -- ");
			print_details = 1;
		}
		else if (date_compare(near_future, &date) >= 0)
		{
			fprintf(stdout, "\x001b[33mSoon     -- ");
			print_details = 1;
		}
		else if (date_compare(future, &date) >= 0)
		{
			fprintf(stdout, "\x001b[32mUpcoming -- ");
			print_details = 1;
		}

		if (print_details)
		{
			date_fprintf(stdout, &date);
			fprintf(stdout, "  %d   %s\x001b[0m\n", (int)agenda_id,
			        agenda_description);
		}
	}

	if (r != SQLITE_DONE)
	{
		sqlite_print_error(db, "agenda_list_due.step");
		r = YSARYS_E;
		goto _done;
	}

	r = YSARYS_OK;
_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);
	return r;
}

int
run(sqlite3 *db, int populate_from_today)
{
	time_t now = 0;
	struct date today = DATE_ZERO;
	struct date near_future_ref_date = DATE_ZERO;
	struct date future_ref_date = DATE_ZERO;
	int r = 0;

	now = time(NULL);
	if (now == ((time_t)-1))
	{
		r = YSARYS_E;
		goto _done;
	}

	date_from_time(now, &today);
	date_add_days(&today, 7, &near_future_ref_date);
	date_add_days(&today, 15, &future_ref_date);

	r = scheduler_populate(db, &today, populate_from_today);
	if (r != YSARYS_OK)
		goto _done;

	r = agenda_list_due(db, &today, &near_future_ref_date,
	                    &future_ref_date);
	if (r != YSARYS_OK)
		goto _done;

	r = YSARYS_OK;
_done:
	return r;
}

int
main(int argc, const char *argv[])
{
	const char *db_filename = NULL;
	const char *command = NULL;
	const char *sub_command = NULL;
	sqlite3 *db = NULL;
	int r = 0;
	int argi = 0;

	if (argc < 2)
	{
		usage();
		r = YSARYS_E;
		goto _done;
	}

	argi++; /* Skip executable name. */
	db_filename = argv[argi++];
	command = argi < argc ? argv[argi++] : "run";

	r = sqlite3_open(db_filename, &db);
	if (r != SQLITE_OK)
	{
		if (db != NULL)
		{
			sqlite_print_error(db, "sqlite3_open");
		}
		else
		{
			fprintf(stderr, "sqlite3_open: %d\n", r);
		}
		r = YSARYS_E;
		goto _done;
	}

	r = db_migrate(db);
	if (r != DB_MIGRATE_OK)
	{
		sqlite_print_error(db, "db_migrate");
		r = YSARYS_E;
		goto _done;
	}

	if (strcmp("run", command) == 0)
		r = run(db, 0);
	else if (strcmp("recheck", command) == 0)
		r = run(db, 1);
	else if (strcmp("scheduler", command) == 0)
	{
		sub_command = argi < argc ? argv[argi++] : "list";
		if (strcmp("list", sub_command) == 0)
			r = scheduler_list(db);
		else if (strcmp("rm", sub_command) == 0)
			r = scheduler_rm(db, argc - argi, &argv[argi]);
		else if (strcmp("add", sub_command) == 0)
		{
			r = scheduler_add(db, argc - argi, &argv[argi]);
			if (r != YSARYS_OK)
				goto _done;
			r = run(db, 1);
		}
	}
	else if (strcmp("agenda", command) == 0)
	{
		sub_command = argi < argc ? argv[argi++] : "list";
		if (strcmp("list", sub_command) == 0)
			r = agenda_list(db);
		else if (strcmp("rm", sub_command) == 0)
			r = agenda_rm(db, argc - argi, &argv[argi]);
		else if (strcmp("add", sub_command) == 0)
		{
			r = agenda_add(db, argc - argi, &argv[argi]);
			if (r != YSARYS_OK)
				goto _done;
			r = run(db, 1);
		}
	}
	else
	{
		usage();
		r = YSARYS_E;
		goto _done;
	}

_done:
	if (db != NULL && sqlite3_close(db) != SQLITE_OK)
	{
		sqlite_print_error(db, "sqlite3_close");
		r = YSARYS_E;
	}
	return r;
}
