#include "sqlite_migrate.h"
#include "dir.h"
#include <sqlite3.h>
#include <string.h>

#define SQLITE_MIGRATE_EXIST        (0)
#define SQLITE_MIGRATE_DOESNT_EXIST (1)

/* ERROR | EXIST | DOESNT_EXIST */
static int
sqlite_migrate_table_exists(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT 1 FROM sqlite_schema WHERE type='table' AND name='z_migrate'";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = SQLITE_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = SQLITE_MIGRATE_DOESNT_EXIST;
			goto _done;

		case SQLITE_ROW:
			r = SQLITE_MIGRATE_EXIST;
			goto _done;

		default:
			r = SQLITE_MIGRATE_ERROR;
			goto _done;
	}

_done:
	if (stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}
	return r;
}

/* ERROR | OK */
static int
sqlite_migrate_table_create(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "CREATE TABLE z_migrate(filename TEXT,applied_at INT)";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		r = SQLITE_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			r = SQLITE_MIGRATE_OK;
			goto _done;

		default:
			r = SQLITE_MIGRATE_ERROR;
			goto _done;
	}

_done:
	if (stmt != NULL)
	{
		sqlite3_finalize(stmt);
	}
	return r;
}

/* NULL: error, non-NULL: ok */
static sqlite3_stmt *
sqlite_migrate_prepare_read(sqlite3 *db)
{
	sqlite3_stmt *stmt = NULL;
	const char sql[] = "SELECT filename FROM z_migrate ORDER BY filename";
	int r = 0;

	r = sqlite3_prepare_v2(db, sql, sizeof(sql), &stmt, NULL);
	if (r != SQLITE_OK)
	{
		if (stmt != NULL)
		{
			sqlite3_finalize(stmt);
			stmt = NULL;
		}
	}

	return stmt;
}

/* ERROR | OK. OK+*ret_filename==NULL means cursor done */
static int
sqlite_migrate_step(sqlite3_stmt *stmt, const char **ret_filename, int *ret_size)
{
	int r = 0;

	r = sqlite3_step(stmt);
	switch (r)
	{
		case SQLITE_DONE:
			*ret_filename = NULL;
			*ret_size = 0;
			return SQLITE_MIGRATE_OK;

		case SQLITE_ROW:
			*ret_filename = (const char *)sqlite3_column_text(stmt, 0);
			*ret_size = sqlite3_column_bytes(stmt, 0);
			return SQLITE_MIGRATE_OK;

		default:
			return SQLITE_MIGRATE_ERROR;
	}
}

/* ERROR | OK */
int
sqlite_migrate(sqlite3 *db)
{
	sqlite3_stmt *stmt_read = NULL;
	sqlite3_stmt *stmt_insert = NULL;
	const char *filename = NULL;
	const char *migration_filename = NULL;
	dir_handle *dir_handle = NULL;
	int filename_size = 0;
	int ord = 0;
	int r = 0;

	r = sqlite_migrate_table_exists(db);
	switch (r)
	{
		case SQLITE_MIGRATE_ERROR:
			r = SQLITE_MIGRATE_ERROR;
			goto _done;

		case SQLITE_MIGRATE_DOESNT_EXIST:
			r = sqlite_migrate_table_create(db);
			if (r != SQLITE_MIGRATE_OK)
			{
				r = SQLITE_MIGRATE_ERROR;
				goto _done;
			}
			break;
	}

	stmt_read = sqlite_migrate_prepare_read(db);
	if (stmt_read == NULL)
	{
		r = SQLITE_MIGRATE_ERROR;
		goto _done;
	}

	r = sqlite_migrate_step(stmt_read, &filename, &filename_size);
	if (r != SQLITE_OK)
	{
		r = SQLITE_MIGRATE_ERROR;
		goto _done;
	}

	dir_handle = dir_first("migrations/", &migration_filename);
	if (dir_handle == NULL)
	{
		r = SQLITE_MIGRATE_ERROR;
		goto _done;
	}

	while (migration_filename != NULL)
	{
		/* Advance z_migrate cursor until we are exactly at this file or past it.
		 * Accounts for SQLite database having extra migrations that we don't have in file system. */
		ord = strcmp(migration_filename, filename);
		while (ord > 0)
		{
			r = sqlite_migrate_step(stmt_read, &filename, &filename_size);
			if (r != SQLITE_MIGRATE_OK)
			{
				r = SQLITE_MIGRATE_ERROR;
				goto _done;
			}
			if (filename == NULL)
			{
				r = SQLITE_MIGRATE_OK;
				goto _done;
			}
			ord = strcmp(migration_filename, filename);
		}

		/* If current filename is less than the current one in z_migrate, it means it has not been
		 * applied yet, so apply it. */
		if (ord < 0)
		{
			r = sqlite_migrate_apply(db, migration_filename);
			if (r != SQLITE_MIGRATE_OK)
			{
				r = SQLITE_MIGRATE_ERROR;
				goto _done;
			}

			if (stmt_insert == NULL)
			{
				stmt_insert = sqlite_migrate_prepare_insert(db);
				if (stmt_insert == NULL)
				{
					r = SQLITE_MIGRATE_ERROR;
					goto _done;
				}
			}
			else
			{
				r = sqlite3_reset(stmt_insert);
				if (r != SQLITE_OK)
				{
					r = SQLITE_MIGRATE_ERROR;
					goto _done;
				}
			}

			r = sqlite_migrate_insert(stmt_insert, migration_filename);
			if (r != SQLITE_OK)
			{
				r = SQLITE_MIGRATE_ERROR;
				goto _done;
			}
		}

		r = dir_next(dir_handle, &migration_filename);
	}

_done:
	if (stmt_read != NULL)
	{
		sqlite3_finalize(stmt_read);
	}
	if (stmt_insert != NULL)
	{
		sqlite3_finalize(stmt_insert);
	}
	return r;
}
