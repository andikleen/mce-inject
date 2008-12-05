CFLAGS := -g -Wall
LDFLAGS += -lpthread

OBJ := mce.tab.o lex.yy.o inject.o util.o
GENSRC := mce.tab.c mce.tab.h lex.yy.c
SRC := inject.c util.c
CLEAN := ${OBJ} ${GENSRC} inject .gdb_history .depend
DISTCLEAN := .depend .gdb_history

.PHONY: clean depend

inject: ${OBJ}

lex.yy.c: mce.lex mce.tab.h
	flex mce.lex
	
mce.tab.c mce.tab.h: mce.y
	bison -d mce.y

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
