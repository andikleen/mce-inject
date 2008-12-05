struct mce;

extern int do_dump;

void submit_mce(struct mce *m);
void init_mce(struct mce *m);
