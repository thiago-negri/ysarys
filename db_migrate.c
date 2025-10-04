#include "db_migrate.h"
#include "db_migrations.h"
#include "log.h"
#include <sqlite3.h>
#include <string.h>
#include <time.h>

#define DB_MIGRATE_EXIST        (0)
#define DB_MIGRATE_DOESNT_EXIST (1)

static int
db_migrate_order(const char *migration, const char *applied_migration)
{
	/* Any migration must be considered "less than" when there's no more applied migrations. */
	if (applied_migration == NULL)
		return -1;

	return strcmp(migration, applied_migration);
}

/* ERROR | EXIST | DOESNT_EXIST */
static int
db_migrate_table_exists(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT 1 FROM sqlite_schema WHERE type='table' AND name='z_migrate'";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = DB_MIGRATE_DOESNT_EXIST;
			goto _done;

		case SQLITE_ROW:
			r = DB_MIGRATE_EXIST;
			goto _done;

		default:
			r = DB_MIGRATE_ERROR;
			goto _done;
	}

_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);

	return r;
}

/* ERROR | OK */
static int
db_migrate_table_create(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "CREATE TABLE z_migrate(filename TEXT,applied_at INT)";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = DB_MIGRATE_OK;
			goto _done;

		default:
			r = DB_MIGRATE_ERROR;
			goto _done;
	}

_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);

	return r;
}

/* ERROR | OK */
static int
db_migrate_prepare_read(sqlite3 *db, sqlite3_stmt **ret_stmt)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT filename FROM z_migrate ORDER BY filename";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		if (stmt != NULL)
		{
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
		goto _done;
	}

	r = DB_MIGRATE_OK;
_done:
	*ret_stmt = stmt;
	return r;
}

/* ERROR | OK. OK+*ret_filename==NULL means cursor done */
static int
db_migrate_step(sqlite3_stmt *stmt, const char **ret_filename)
{
	int r = 0;

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			*ret_filename = NULL;
			return DB_MIGRATE_OK;

		case SQLITE_ROW:
			*ret_filename = (const char *)sqlite3_column_text(stmt, 0);
			return DB_MIGRATE_OK;

		default:
			return DB_MIGRATE_ERROR;
	}
}

/* ERROR | OK */
static int
db_migrate_apply(sqlite3 *db, const char *sql)
{
	sqlite3_stmt *stmt = NULL;
	const char *current_sql = NULL;
	const char *next_sql = NULL;
	size_t sql_size = 0;
	int r = 0;

	/* SQLite documentation states there is a small performance benefit for including the nul-terminator.
	 * See: https://www3.sqlite.org/c3ref/prepare.html */
	sql_size = strlen(sql) + 1;

	current_sql = sql;
	while (current_sql != NULL && current_sql[0] != '\0')
	{
		r = sqlite3_prepare_v2(db, current_sql, sql_size, &stmt, &next_sql);
		if (r != SQLITE_OK)
		{
			r = DB_MIGRATE_ERROR;
			goto _done;
		}
		log_debug("DB_MIGRATE: %.*s\n", next_sql - current_sql, current_sql);
		sql_size -= next_sql - current_sql;
		current_sql = next_sql;

		r = sqlite3_step(stmt);
		if (r != SQLITE_DONE)
		{
			r = DB_MIGRATE_ERROR;
			goto _done;
		}

		sqlite3_finalize(stmt);
		stmt = NULL;
	}

	r = DB_MIGRATE_OK;
_done:
	if (stmt != NULL)
		sqlite3_finalize(stmt);

	return r;
}

/* ERROR | OK */
static int
db_migrate_prepare_insert(sqlite3 *db, sqlite3_stmt **ret_stmt)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "INSERT INTO z_migrate(filename,applied_at)VALUES(?,?)";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		if (stmt != NULL)
		{
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
		goto _done;
	}

	r = DB_MIGRATE_OK;
_done:
	*ret_stmt = stmt;
	return r;
}

/* ERROR | OK */
static int
db_migrate_insert(sqlite3_stmt *stmt, const char *migration_name)
{
	time_t now = 0;
	int r = 0;

	r = sqlite3_bind_text(stmt, 1, migration_name, strlen(migration_name), SQLITE_STATIC);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	now = time(NULL);
	if (now == ((time_t)-1))
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_bind_int64(stmt, 2, now);
	if (r != SQLITE_OK)
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_step(stmt);
	if (r != SQLITE_DONE)
	{
		r = DB_MIGRATE_ERROR;
		goto _done;
	}

	r = DB_MIGRATE_OK;
_done:
	return r;
}

/* ERROR | OK */
int
db_migrate(sqlite3 *db)
{
	sqlite3_stmt *stmt_read = NULL;
	sqlite3_stmt *stmt_insert = NULL;
	const char *applied_migration = NULL;
	struct db_migration *migration = NULL;
	int ord = 0;
	int r = 0;

	r = db_migrate_table_exists(db);
	switch (r)
	{
		case DB_MIGRATE_ERROR:
			goto _done;

		case DB_MIGRATE_DOESNT_EXIST:
			r = db_migrate_table_create(db);
			if (r != DB_MIGRATE_OK)
				goto _done;

			break;
	}

	r = db_migrate_prepare_read(db, &stmt_read);
	if (r != DB_MIGRATE_OK)
		goto _done;

	r = db_migrate_step(stmt_read, &applied_migration);
	if (r != DB_MIGRATE_OK)
		goto _done;

	for (migration = db_migrations; migration->name != NULL; migration++)
	{
		/* Advance z_migrate cursor until we are exactly at this file or past it.
		 * Accounts for SQLite database having extra migrations that we don't have in file system. */
		ord = db_migrate_order(migration->name, applied_migration);
		while (ord > 0)
		{
			r = db_migrate_step(stmt_read, &applied_migration);
			if (r != DB_MIGRATE_OK)
			{
				r = DB_MIGRATE_ERROR;
				goto _done;
			}
			ord = db_migrate_order(migration->name, applied_migration);
		}

		/* If current migration name is less than the current one in z_migrate, it means it has not been
		 * applied yet, so apply it. */
		if (ord < 0)
		{
			r = db_migrate_apply(db, migration->sql);
			if (r != DB_MIGRATE_OK)
				goto _done;

			if (stmt_insert == NULL)
			{
				r = db_migrate_prepare_insert(db, &stmt_insert);
				if (r != DB_MIGRATE_OK)
					goto _done;
			}
			else
			{
				r = sqlite3_reset(stmt_insert);
				if (r != SQLITE_OK)
				{
					r = DB_MIGRATE_ERROR;
					goto _done;
				}
			}

			r = db_migrate_insert(stmt_insert, migration->name);
			if (r != DB_MIGRATE_OK)
				goto _done;
		}
	}

	r = DB_MIGRATE_OK;
_done:
	if (stmt_read != NULL)
		sqlite3_finalize(stmt_read);

	if (stmt_insert != NULL)
		sqlite3_finalize(stmt_insert);

	return r;
}
