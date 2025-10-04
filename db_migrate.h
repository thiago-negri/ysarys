#ifndef DB_MIGRATE_H
#define DB_MIGRATE_H

#include <sqlite3.h>

#define DB_MIGRATE_ERROR (-1)
#define DB_MIGRATE_OK    (0)

/* ERROR | OK */
int db_migrate(sqlite3 *db);

#endif /* !DB_MIGRATE_H */
