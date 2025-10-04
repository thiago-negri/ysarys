APP := ysarys
CFLAGS := $(shell cat compile_flags.txt)
LDFLAGS := -lsqlite3

.PHONY:	default
default:	$(APP)

.PHONY:	clean
clean:
	rm -f *.o $(APP)

$(APP):	log.o db_migrate.o ysarys.o

db_migrate.o:	db_migrate.h db_migrations.h log.h
ysarys.o:	db_migrate.h
