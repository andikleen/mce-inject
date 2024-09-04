/* Taken from the v2.6.26 Linux 2.6.26-rc8 kernel source + some changes */
#ifndef _ASM_X86_MCE_H
#define _ASM_X86_MCE_H

#include <asm/ioctls.h>
#include <asm/types.h>

/*
 * Machine Check support for x86
 */

#define MCG_CTL_P	 (1ULL<<8)   /* MCG_CAP register available */

#define MCG_STATUS_RIPV  (1ULL<<0)   /* restart ip valid */
#define MCG_STATUS_EIPV  (1ULL<<1)   /* ip points to correct instruction */
#define MCG_STATUS_MCIP  (1ULL<<2)   /* machine check in progress */
#define MCG_STATUS_LMCES  (1ULL<<3)   /* local machine check exception signaled */
#define MCG_STATUS_SEAM_NR (1ULL<<12) /* SEAM NON-ROOT */

#define MCI_STATUS_VAL   (1ULL<<63)  /* valid error */
#define MCI_STATUS_OVER  (1ULL<<62)  /* previous errors lost */
#define MCI_STATUS_UC    (1ULL<<61)  /* uncorrected error */
#define MCI_STATUS_EN    (1ULL<<60)  /* error enabled */
#define MCI_STATUS_MISCV (1ULL<<59)  /* misc error reg. valid */
#define MCI_STATUS_ADDRV (1ULL<<58)  /* addr reg. valid */
#define MCI_STATUS_PCC   (1ULL<<57)  /* processor context corrupt */
#define MCI_STATUS_S	 (1ULL<<56)  /* Signaled machine check */
#define MCI_STATUS_AR	 (1ULL<<55)  /* Action required */

/* MISC register defines */
#define MCM_ADDR_SEGOFF  0	/* segment offset */
#define MCM_ADDR_LINEAR  1	/* linear address */
#define MCM_ADDR_PHYS	 2	/* physical address */
#define MCM_ADDR_MEM	 3	/* memory address */
#define MCM_ADDR_GENERIC 7	/* generic */

#define MCJ_CTX_MASK		3
#define MCJ_CTX(flags)		((flags) & MCJ_CTX_MASK)
#define MCJ_CTX_RANDOM		0    /* inject context: random */
#define MCJ_CTX_PROCESS		1    /* inject context: process */
#define MCJ_CTX_IRQ		2    /* inject context: IRQ */
#define MCJ_NMI_BROADCAST	4    /* do NMI broadcasting */
#define MCJ_EXCEPTION		8    /* raise as exception */
#define MCJ_IRQ_BRAODCAST	0x10 /* do IRQ broadcasting */

#define MCJ_CTX_SET(flags, ctx)				\
	do {						\
		(flags) &= ~MCJ_CTX_MASK;		\
		(flags) |= ((ctx) & MCJ_CTX_MASK);	\
	} while (0)

/* Fields are zero when not available */
struct mce {
	__u64 status;
	__u64 misc;
	__u64 addr;
	__u64 mcgstatus;
	__u64 ip;
	__u64 tsc;	/* cpu time stamp counter */
	__u64 time;	/* wall time_t when error was detected */
	__u8  cpuvendor;	/* cpu vendor as encoded in system.h */
	__u8  inject_flags;
	__u16 pad2;
	__u32 cpuid;	/* CPUID 1 EAX */
	__u8  cs;	/* code segment */
	__u8  bank;	/* machine check bank */
	__u8  cpu;	/* cpu that raised the error */
	__u8  finished;   /* entry is valid */
	__u32 extcpu;	/* extended CPU number */
	__u32 socketid;	/* CPU socket ID */
	__u32 apicid;	/* CPU initial apic ID */
	__u64 mcgcap;	/* MCGCAP MSR: machine check capabilities of CPU */
};

/*
 * This structure contains all data related to the MCE log.  Also
 * carries a signature to make it easier to find from external
 * debugging tools.  Each entry is only valid when its finished flag
 * is set.
 */

#define MCE_LOG_LEN 32

struct mce_log {
	char signature[12]; /* "MACHINECHECK" */
	unsigned len;	    /* = MCE_LOG_LEN */
	unsigned next;
	unsigned flags;
	unsigned recordlen;	/* length of struct mce */
	struct mce entry[MCE_LOG_LEN];
};

#define MCE_OVERFLOW 0		/* bit 0 in flags means overflow */

#define MCE_LOG_SIGNATURE	"MACHINECHECK"

#define MCE_GET_RECORD_LEN   _IOR('M', 1, int)
#define MCE_GET_LOG_LEN      _IOR('M', 2, int)
#define MCE_GETCLEAR_FLAGS   _IOR('M', 3, int)

/* Software defined banks */
#define MCE_EXTENDED_BANK	128
#define MCE_THERMAL_BANK	MCE_EXTENDED_BANK + 0

#define K8_MCE_THRESHOLD_BASE      (MCE_EXTENDED_BANK + 1)      /* MCE_AMD */
#define K8_MCE_THRESHOLD_BANK_0    (MCE_THRESHOLD_BASE + 0 * 9)
#define K8_MCE_THRESHOLD_BANK_1    (MCE_THRESHOLD_BASE + 1 * 9)
#define K8_MCE_THRESHOLD_BANK_2    (MCE_THRESHOLD_BASE + 2 * 9)
#define K8_MCE_THRESHOLD_BANK_3    (MCE_THRESHOLD_BASE + 3 * 9)
#define K8_MCE_THRESHOLD_BANK_4    (MCE_THRESHOLD_BASE + 4 * 9)
#define K8_MCE_THRESHOLD_BANK_5    (MCE_THRESHOLD_BASE + 5 * 9)
#define K8_MCE_THRESHOLD_DRAM_ECC  (MCE_THRESHOLD_BANK_4 + 0)

#endif
