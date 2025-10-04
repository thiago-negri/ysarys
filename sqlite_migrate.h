#ifndef SQLITE_MIGRATE_H
#define SQLITE_MIGRATE_H

#include <sqlite3.h>

#define SQLITE_MIGRATE_ERROR        (-1)
#define SQLITE_MIGRATE_OK           (0)

/* ERROR | OK */
int sqlite_migrate(sqlite3 *db);

#endif /* !SQLITE_MIGRATE_H */
