MRUBY_DIR:=../mruby
CC=gcc
INCLUDES=-I$(MRUBY_DIR)/include -I$(MRUBY_DIR)/src
CFLAGS=$(shell pkg-config sqlite3 --cflags) -Wall -Wextra -Wno-unused -fPIC -g -O2 $(INCLUDES)
SONAME=libsqlite3ext_mruby.so.1
LDFLAGS=-shared -Wl,-soname,$(SONAME)
LIBS=$(MRUBY_DIR)/lib/libmruby.a $(shell pkg-config sqlite3 --libs) -lm
all: libsqlite3ext_mruby.so.1.0.0
$(MRUBY_DIR)/lib/libmruby.a:
	$(MAKE) CFLAGS=-fPIC -C $(MRUBY_DIR)
libsqlite3ext_mruby.so.1.0.0: sqlite3ext_mruby.o $(MRUBY_DIR)/lib/libmruby.a
	$(CC) $(LDFLAGS) -o $@ $< $(LIBS)
sqlite3ext_mruby.o: sqlite3ext_mruby.c
clean:
	$(MAKE) -C test/lib clean
	rm -f *.o *.so.*
test: libsqlite3ext_mruby.so.1.0.0
	$(MAKE) -C test/lib
	LD_LIBRARY_PATH=. testrb test
eg:
	$(MAKE) -C eg
.PHONY: clean test eg
