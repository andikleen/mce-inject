
typedef unsigned long long u64;

#define YYSTYPE u64


extern void yyerror(const char *fmt, ...);
extern int yylineno;
extern int yylex(void);
extern int yyparse(void);
extern char *filename;

enum mceflags { 
	MCE_NOBROADCAST = (1 << 0),
	MCE_HOLD = (1 << 1),
};

extern enum mceflags mce_flags;

enum mce_context {
	MCEC_PROCESS,
	MCEC_IRQ,
};
