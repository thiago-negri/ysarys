#include "sqlite_migrate.h"
#include <sqlite3.h>
#include <stdio.h>

void
sqlite_print_error(sqlite3 *db, const char *tag)
{
	int errcode = 0;
	const char *errmsg = NULL;

	errcode = sqlite3_extended_errcode(db);
	errmsg = sqlite3_errmsg(db);

	fprintf(stderr, "%s %d: %s\n", tag, errcode, errmsg);
}

int
main(int argc, const char *argv[])
{
	const char *db_name = NULL;
	sqlite3 *db = NULL;
	int r = 0;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: ysarys <db>\n");
		r = -1;
		goto _done;
	}

	db_name = argv[1];

	r = sqlite3_open(db_name, &db);
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
		r = -1;
		goto _done;
	}

	r = sqlite_migrate(db);
	if (r != SQLITE_MIGRATE_OK)
	{
		sqlite_print_error(db, "sqlite_migrate");
		r = -1;
		goto _done;
	}

	r = 0;
_done:
	if (db != NULL)
	{
		r = sqlite3_close(db);
		if (r != SQLITE_OK)
		{
			sqlite_print_error(db, "sqlite3_close");
			return -1;
		}
	}
	return r;
}
