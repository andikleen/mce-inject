prefix := /usr
manprefix := ${prefix}/share
CFLAGS := -Os -g -Wall
LDFLAGS += -lpthread

OBJ := mce.tab.o lex.yy.o mce-inject.o util.o
GENSRC := mce.tab.c mce.tab.h lex.yy.c
SRC := mce-inject.c util.c
CLEAN := ${OBJ} ${GENSRC} inject mce-inject .depend
DISTCLEAN := .depend .gdb_history

.PHONY: clean depend install

mce-inject: ${OBJ}

lex.yy.c: mce.lex mce.tab.h
	flex mce.lex
	
mce.tab.c mce.tab.h: mce.y
	bison -d mce.y

install:
	install -m 755 mce-inject $(prefix)/sbin/mce-inject
	install -m 644 mce-inject.8 $(manprefix)/man/man8/mce-inject.8

clean:
	rm -f ${CLEAN}

distclean: clean
	rm -f ${DISTCLEAN} *~

depend: .depend

.depend: ${SRC} ${GENSRC}
	${CC} -MM -DDEPS_RUN -I. ${SRC} ${GENSRC} > .depend.X && \
		mv .depend.X .depend

Makefile: .depend

include .depend
