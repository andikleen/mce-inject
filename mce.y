/* Copyright (c) 2008 by Intel Corp.
   Grammar for machine check injection. Follows the format printed out
   by the kernel on panics with some extensions.  See manpage for rough
   spec.

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

%token STATUS RIP TSC ADDR MISC CPU BANK MCGSTATUS NOBROADCAST HOLD
%token IN_IRQ IN_PROC PROCESSOR TIME SOCKETID APICID MCGCAP
%token POLL EXCP
%token CORRECTED UNCORRECTED FATAL MCE
%token NUMBER
%token SYMBOL
%token MACHINE CHECK EXCEPTION

%token RIPV EIPV MCIP
%token VAL OVER UC EN PCC S AR UCNA SRAO SRAR

%%

input: /* empty */
     | input mce_start mce { submit_mce(&m); } ;

mce_start: CPU NUMBER 	   { init(); m.cpu = m.extcpu = $2; }
     | CPU NUMBER NUMBER   { init(); m.cpu = m.extcpu = $2; m.bank = $3; }
     | MCE		   { init(); }
     | CPU NUMBER ':'
       MACHINE CHECK EXCEPTION ':' NUMBER BANK NUMBER ':'
       NUMBER 		   { init();
			     m.cpu = $2; m.mcgstatus = $8;
			     m.bank = $10; m.status = $12; }
     ;

mce:  mce_term
     | mce mce_term
     ;

mce_term:   STATUS status_list  { m.status = $2; }
     | MCGSTATUS mcgstatus_list { m.mcgstatus = $2; }
     | BANK NUMBER 	   { m.bank = $2; }
    
     | TSC NUMBER	   { m.tsc = $2; }
     | TIME NUMBER	   { m.time = $2; }
     | SOCKETID NUMBER	   { m.socketid = $2; }
     | APICID NUMBER	   { m.apicid = $2; }
     | MCGCAP NUMBER	   { m.mcgcap = $2; }
     | RIP NUMBER 	   { m.ip = $2; } 
     | RIP NUMBER ':' NUMBER { m.ip = $4; m.cs = $2; }
     | RIP NUMBER ':' '<' NUMBER '>' '{' SYMBOL '}' 
			   { m.ip = $5; m.cs = $2; } 
     | ADDR NUMBER	   { m.addr = $2; m.status |= MCI_STATUS_ADDRV; }
     | MISC NUMBER	   { m.misc = $2; m.status |= MCI_STATUS_MISCV; } 
     | PROCESSOR NUMBER ':' NUMBER { m.cpuvendor = $2; m.cpuid = $4; }
     | NOBROADCAST	   { mce_flags |= MCE_NOBROADCAST; } 
     | HOLD		   { mce_flags |= MCE_HOLD; }
     | IN_IRQ		   { MCJ_CTX_SET(m.inject_flags, MCJ_CTX_IRQ); }
     | IN_PROC		   { MCJ_CTX_SET(m.inject_flags, MCJ_CTX_PROCESS); }
     | POLL		   { mce_flags |= MCE_RAISE_MODE;
			     m.inject_flags &= ~MCJ_EXCEPTION; }
     | EXCP		   { mce_flags |= MCE_RAISE_MODE;
			     m.inject_flags |= MCJ_EXCEPTION; }
     ;

mcgstatus_list:  /* empty */ { $$ = 0; }
     | mcgstatus_list mcgstatus { $$ = $1 | $2; } 
     ;

mcgstatus : RIPV | EIPV | MCIP | NUMBER ; 

status_list: 	 /* empty */ { $$ = 0; } 
     | status_list status { $$ = $1 | $2; } 

status: UC | EN | VAL | OVER | PCC | NUMBER | CORRECTED | UNCORRECTED | 
     FATAL | S | AR | UCNA | SRAO | SRAR
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
