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

long
select_count_scheduler(sqlite3 *db)
{
  sqlite3_stmt *stmt = NULL;
  const char sql[] = "SELECT COUNT(1) FROM scheduler";
  int r = 0;
  int count = 0;

  r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
  if (r != SQLITE_OK)
  {
    sqlite_print_error(db, "select_count_scheduler.prepare");
    count = 0;
    goto _done;
  }

  r = sqlite3_step(stmt);
  switch (r)
  {
    case SQLITE_DONE:
      count = 0;
      goto _done;

    case SQLITE_ROW:
      count = sqlite3_column_int(stmt, 0);
      goto _done;

    default:
      sqlite_print_error(db, "select_count_scheduler.step");
      count = 0;
      goto _done;
  }

_done:
  if (stmt != NULL)
    sqlite3_finalize(stmt);

  return count;
}

int
scheduler_populate(sqlite3 *db, struct date *today)
{
  struct date check_start_data = DATE_ZERO;
  struct date *check_start = NULL;
  struct date check_end = DATE_ZERO;
  int rules_count = 0;
  sqlite3_int64 last_run = 0;

  last_run = select_last_run_time(db);
  if (last_run != 0)
  {
    date_from_time(last_run, &check_start_data);
    check_start = &check_start_data;
  }
  else
    check_start = today;

  date_add_days(today, 60, &check_end);

  rules_count = select_count_scheduler(db);

  /* TODO: Continue... */
  return 0;
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

  r = agenda_list_due(db, &today, &near_future_ref_date, &future_ref_date);
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
