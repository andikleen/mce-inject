struct mce;

extern int do_dump;

void init_cpu_info(void);
void init_inject(void);
void submit_mce(struct mce *m);
void init_mce(struct mce *m);
