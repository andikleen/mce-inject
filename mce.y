/* Grammar for machine check injection. Follows the format printed out 
   by the kernel on panics with some extensions.  See SPEC. */
%{
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "parser.h"
#include "mce.h"
#include "inject.h"

static struct mce m;

enum mceflags mce_flags;

static void init(void);

%} 

%token STATUS RIP TSC ADDR MISC CPU BANK MCGSTATUS NOBROADCAST
%token CORRECTED UNCORRECTED FATAL MCE
%token NUMBER
%token SYMBOL
%token MACHINE CHECK EXCEPTION

%token RIPV EIPV MCIP 
%token VAL OVER UC EN PCC

%%

input: /* empty */ 
     | input mce_start mce { submit_mce(&m); } ;

mce_start: CPU NUMBER 	   { init(); m.cpu = $2; }
     | CPU NUMBER NUMBER   { init(); m.cpu = $2; m.bank = $3; }
     | MCE		   { init(); } 
     | CPU NUMBER ':'
       MACHINE CHECK EXCEPTION ':' NUMBER BANK NUMBER ':' 
       NUMBER 		   { init();
			     m.cpu = $2; m.mcgstatus = $6; 
			     m.bank = $8; m.status = $10; }
     ;

mce:  mce_term
     | mce mce_term
     ;

mce_term:   STATUS status_list  { m.status = $2; }
     | STATUS NUMBER	   { m.status = $2; }
     | MCGSTATUS mcgstatus_list { m.mcgstatus = $2; }
     | MCGSTATUS NUMBER	   { m.mcgstatus = $2; }
     | BANK NUMBER 	   { m.bank = $2; }
    
     | TSC NUMBER	   { m.tsc = $2; }
     | RIP NUMBER 	   { m.ip = $2; } 
     | RIP NUMBER ':' NUMBER { m.ip = $4; m.cs = $2; }
     | RIP NUMBER ':' '<' NUMBER '>' '{' SYMBOL '}' 
			   { m.ip = $5; m.cs = $2; } 
     | ADDR NUMBER	   { m.addr = $2; m.status |= MCI_STATUS_ADDRV; }
     | MISC NUMBER	   { m.misc = $2; m.status |= MCI_STATUS_MISCV; } 
     | NOBROADCAST	   { mce_flags |= MCE_NOBROADCAST; } 
     ; 

mcgstatus_list:  /* empty */
     | mcgstatus_list mcgstatus { $$ = $1 | $2; } 
     ;

mcgstatus : RIPV | EIPV | MCIP | NUMBER ; 

status_list: 	 /* empty */ { $$ = 0; } 
     | status_list status { $$ = $1 | $2; } 

status: UC | EN | VAL | OVER | PCC | NUMBER | CORRECTED | UNCORRECTED | 
     FATAL
     ; 

%% 

static void init(void)
{
	init_mce(&m);
	mce_flags = 0;
}

void yyerror(char const *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	fprintf(stderr, "%s:%d: ", filename, yylineno);
	vfprintf(stderr, msg, ap);
	fputc('\n', stderr);
	va_end(ap);
	exit(1);
}

