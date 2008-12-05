#define ARRAY_SIZE(x) (sizeof(x)/sizeof(*(x)))
void err(const char *msg);

#define NEW(x) ((x) = xalloc(sizeof(*(x))))
void *xalloc(size_t sz);

#ifdef __GNUC__
#define barrier() asm volatile("" ::: "memory")
#else
#define barrier() do {} while(0)
#endif

