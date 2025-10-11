#ifndef DB_MIGRATE_H
#define DB_MIGRATE_H

#include <sqlite3.h>

enum
{
  DB_MIGRATE_OK = 0,
  DB_MIGRATE_E
};

/* ERROR | OK */
int db_migrate(sqlite3 *db);

#endif /* !DB_MIGRATE_H */
