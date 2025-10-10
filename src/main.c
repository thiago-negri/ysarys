#include "date.h"
#include "db_migrate.h"
#include "intdef.h"
#include <sqlite3.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define YSARYS_OK    (0)
#define YSARYS_ERROR (-1)

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

sqlite3_int64
select_last_run_time(sqlite3 *db)
{
  sqlite3_stmt *stmt = NULL;
  const char sql[] = "SELECT last_run_at_timestamp FROM scheduler_control WHERE id = 1";
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
  const char sql[] = "SELECT id, rule, description, tags_csv, monetary_value FROM scheduler";
  int r = 0;

  r = sqlite3_prepare_v2(db, sql, sizeof(sql), stmt, NULL);
  if (r != SQLITE_OK)
  {
    sqlite_print_error(db, "select_scheduler.prepare");
    if (*stmt != NULL)
      sqlite3_finalize(*stmt);

    r = YSARYS_ERROR;
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

  if (date->day < month_last_day(date->year, date->month))
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

int
scheduler_populate(sqlite3 *db, struct date *today)
{
  struct date check_start_data = DATE_ZERO;
  struct date *check_start = NULL;
  struct date check_end = DATE_ZERO;
  struct date current = DATE_ZERO;
  sqlite3_stmt *stmt_scheduler = NULL;
  sqlite3_int64 last_run = 0;
  sqlite3_int64 scheduler_id = 0;
  const unsigned char *scheduler_rule = NULL;
  const unsigned char *scheduler_description = NULL;
  const unsigned char *scheduler_tags_csv = NULL;
  sqlite3_int64 scheduler_monetary_value = 0;
  int r = 0;

  last_run = select_last_run_time(db);
  if (last_run != 0)
  {
    date_from_time(last_run, &check_start_data);
    check_start = &check_start_data;
  }
  else
    check_start = today;

  date_add_days(today, 60, &check_end);

  r = select_scheduler(db, &stmt_scheduler);
  if (r != YSARYS_OK)
    goto _done;

  while ((r = sqlite3_step(stmt_scheduler)) == SQLITE_ROW)
  {
    scheduler_id = sqlite3_column_int64(stmt_scheduler, 0);
    scheduler_rule = sqlite3_column_text(stmt_scheduler, 1);
    scheduler_description = sqlite3_column_text(stmt_scheduler, 2);
    scheduler_tags_csv = sqlite3_column_text(stmt_scheduler, 3);
    scheduler_monetary_value = sqlite3_column_int64(stmt_scheduler, 4);

    printf("Scheduler %lld: (%s) '%s' [%s] $%lld\n", scheduler_id, scheduler_rule, scheduler_description,
           scheduler_tags_csv, scheduler_monetary_value);

    for (current = *check_start; compare(&current, &check_end) <= 0; next(&current))
    {
      if (matches(scheduler_rule, &current))
      {
        if (!exists(db, scheduler_id, &current))
        {
          agenda_insert(db, scheduler..., &current);
        }
      }
    }
  }

  update_last_run_time(db, &check_end);

  /* TODO: Continue... */
  r = YSARYS_OK;
_done:
  if (stmt_scheduler != NULL)
    sqlite3_finalize(stmt_scheduler);
  return r;
}

int
run(sqlite3 *db)
{
  time_t now = 0;
  struct date today = DATE_ZERO;
  struct date near_future_ref_date = DATE_ZERO;
  struct date future_ref_date = DATE_ZERO;
  int r = 0;

  now = time(NULL);
  if (now == ((time_t)-1))
  {
    r = YSARYS_ERROR;
    goto _done;
  }

  date_from_time(now, &today);
  date_add_days(&today, 7, &near_future_ref_date);
  date_add_days(&today, 15, &future_ref_date);

  r = scheduler_populate(db, &today);
  if (r != YSARYS_OK)
    goto _done;

  /*
  r = agenda_list_due(db, &today, &near_future_ref_date, &future_ref_date);
  if (r != YSARYS_OK)
    goto _done;
    */

  r = YSARYS_OK;
_done:
  return r;
}

int
main(int argc, const char *argv[])
{
  const char *db_filename = NULL;
  const char *command = NULL;
  sqlite3 *db = NULL;
  int r = 0;
  int argi = 0;

  if (argc < 2)
  {
    fprintf(stderr, "Usage: ysarys <db> [command]\n");
    r = YSARYS_ERROR;
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
    r = YSARYS_ERROR;
    goto _done;
  }

  r = db_migrate(db);
  if (r != DB_MIGRATE_OK)
  {
    sqlite_print_error(db, "db_migrate");
    r = YSARYS_ERROR;
    goto _done;
  }

  if (strcmp("run", command) == 0)
    r = run(db);

  r = YSARYS_OK;
_done:
  if (db != NULL && sqlite3_close(db) != SQLITE_OK)
  {
    sqlite_print_error(db, "sqlite3_close");
    r = YSARYS_ERROR;
  }
  return r;
}
