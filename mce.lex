/* Scanner for the machine check grammar */
%{ 
#define _GNU_SOURCE 1
#include <stdlib.h>
#include <string.h>

#include "mce.h"
#include "parser.h"
#include "mce.tab.h"
#include "util.h"

int yylineno;

static int lookup_symbol(const char *);
%}

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
	KEY(ADDR), 
	KEY(MISC), 
	KEY(CPU), 
	KEY(BANK), 
	KEY(MCGSTATUS), 
	KEY(NOBROADCAST), 
	KEYVAL(CORRECTED, MCI_STATUS_VAL), 	// checkme
	KEYVAL(UNCORRECTED, MCI_STATUS_UC|MCI_STATUS_EN), 
	KEYVAL(FATAL, MCI_STATUS_UC|MCI_STATUS_EN|MCI_STATUS_PCC), 
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
		printf("got %s val %lx\n", k->name, k->val);
		return k->tok;
	}
	printf("SYMBOL\n");
	return SYMBOL;
}

static void init_lex(void)
{
	qsort(keys, ARRAY_SIZE(keys), sizeof(struct key), cmp_key);
}

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
	init_lex();
	argv = ++av;	
	if (*argv)
		yywrap();
	return yyparse();
}

