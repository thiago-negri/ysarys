APP := ysarys
CFLAGS := $(shell cat compile_flags.txt)
LDFLAGS := -lsqlite3

KERNEL := $(shell uname)
ifeq ($(KERNEL),Linux)
	DIR := dir_linux.o
else
	DIR := dir_windows.o
endif

.PHONY:	default
default:	$(APP)

.PHONY:	clean
clean:
	rm -f *.o $(APP)

$(APP): $(DIR) sqlite_migrate.o ysarys.o

dir_linux.o: dir_linux.c dir.h
dir_windows.o: dir_windows.c dir.h
sqlite_migrate.o: sqlite_migrate.c sqlite_migrate.h dir.h
ysarys.o: ysarys.c sqlite_migrate.h
