/* Copyright (c) 2008 by Intel Corp.
   Scanner for the machine check grammar.

   mce-inject is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; version
   2.

   mce-inject is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should find a copy of v2 of the GNU General Public License somewhere
   on your Linux system; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

   Authors:
        Andi Kleen
	Ying Huang
*/
%{
#define _GNU_SOURCE 1
#include <stdlib.h>
#include <string.h>

#include "mce.h"
#include "parser.h"
#include "mce.tab.h"
#include "util.h"
#include "inject.h"

int yylineno;

static int lookup_symbol(const char *);

#define YY_NO_INPUT 1
%}

%option nounput

%%

#.*\n			/* comment */;
\n			++yylineno;
0x[0-9a-fA-F]+ 		|
0[0-7]+			|
[0-9]+			yylval = strtoull(yytext, NULL, 0); return NUMBER;
[:{}<>]			return yytext[0];
[_a-zA-Z][_a-zA-Z0-9]*	return lookup_symbol(yytext);
[ \t]+			/* white space */;
.			yyerror("Unrecognized character '%s'", yytext);

%%

/* Keyword handling */

static struct key {
	const char *name;
	int tok;
	u64 val;
} keys[] = {
#define KEY(x) { #x, x }
#define KEYVAL(x,v) { #x, x, v }
	KEY(MCE),
	KEY(STATUS),
	KEY(RIP),
	KEY(TSC),
	KEY(TIME),
	KEY(SOCKETID),
	KEY(APICID),
	KEY(MCGCAP),
	KEY(ADDR),
	KEY(MISC),
	KEY(CPU),
	KEY(BANK),
	KEY(MCGSTATUS),
	KEY(PROCESSOR),
	KEY(NOBROADCAST),
	KEY(IRQBROADCAST),
	KEY(NMIBROADCAST),
	KEY(HOLD),
	KEY(IN_IRQ),
	KEY(IN_PROC),
	KEY(POLL),
	KEY(EXCP),
	KEYVAL(CORRECTED, MCI_STATUS_VAL|MCI_STATUS_EN), 	// checkme
	KEYVAL(UNCORRECTED, MCI_STATUS_VAL|MCI_STATUS_UC|MCI_STATUS_EN),
	KEYVAL(FATAL, MCI_STATUS_VAL|MCI_STATUS_UC|MCI_STATUS_EN
	       |MCI_STATUS_PCC),
	KEY(MACHINE),
	KEY(CHECK),
	KEY(EXCEPTION),
	KEYVAL(RIPV, MCG_STATUS_RIPV),
	KEYVAL(EIPV, MCG_STATUS_EIPV),
	KEYVAL(MCIP, MCG_STATUS_MCIP),
	KEYVAL(VAL, MCI_STATUS_VAL),
	KEYVAL(OVER, MCI_STATUS_OVER),
	KEYVAL(UC, MCI_STATUS_UC),
	KEYVAL(EN, MCI_STATUS_EN),
	KEYVAL(PCC, MCI_STATUS_PCC),
	KEYVAL(S, MCI_STATUS_S),
	KEYVAL(AR, MCI_STATUS_AR),
	KEYVAL(UCNA, 0),
	KEYVAL(SRAO, MCI_STATUS_S),
	KEYVAL(SRAR, MCI_STATUS_S|MCI_STATUS_AR),
};

static int cmp_key(const void *av, const void *bv)
{
	const struct key *a = av;
	const struct key *b = bv;
	return strcasecmp(a->name, b->name);
}

static int lookup_symbol(const char *name)
{
	struct key *k;
	struct key key;
	key.name = name;
	k = bsearch(&key, keys, ARRAY_SIZE(keys), sizeof(struct key), cmp_key);
	if (k != NULL) {
		yylval = k->val;
		return k->tok;
	}
	return SYMBOL;
}

static void init_lex(void)
{
	qsort(keys, ARRAY_SIZE(keys), sizeof(struct key), cmp_key);
}

int do_dump;
int no_random;
static char **argv;
char *filename = "<stdin>";

int yywrap(void)
{
	if (*argv == NULL)
		return 1;
	filename = *argv;
	yyin = fopen(filename, "r");
	if (!yyin)
		err(filename);
	argv++;
	return 0;
}

int main(int ac, char **av)
{
	int rc = 0;

	init_lex();
	argv = ++av;
	if (*argv && !strcmp(*argv, "--dump")) {
		do_dump = 1;
		argv++;
	}
	if (*argv && !strcmp(*argv, "--no-random")) {
		no_random = 1;
		argv++;
	}
	init_cpu_info();
	init_inject();
	if (*argv)
		yywrap();
	rc = yyparse();
	clean_inject();

	return rc;
}
