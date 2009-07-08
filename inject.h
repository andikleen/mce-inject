struct mce;

extern int do_dump;
extern int no_random;

void init_cpu_info(void);
void init_inject(void);
void clean_inject(void);
void submit_mce(struct mce *m);
void init_mce(struct mce *m);
