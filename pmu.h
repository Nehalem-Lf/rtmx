static inline void init_perf_batch(unsigned int evt0, unsigned int evt1, unsigned int evt2, unsigned int evt3, unsigned int evt4, unsigned int evt5)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	//PMCNTENSET enable event counters and cycle counter
	__asm volatile ("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f) );
	//PMSELR select the event counter
	//PMXEVTYPER set performance event
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(0) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt0) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(1) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt1) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(2) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt2) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(3) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt3) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(4) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt4) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(5) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt5) );
#else
#error Unsupported architecture/compiler!
#endif
}

static inline void init_perf_start()
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	//PMCNTENSET enable event counters and cycle counter
	__asm volatile ("mcr p15, 0, %0, c9, c12, 1\t\n" :: "r"(0x8000000f) );
#else
#error Unsupported architecture/compiler!
#endif
}

static inline void init_perf(unsigned int id, unsigned int evt)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	//PMSELR select the event counter
	//PMXEVTYPER set performance event
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(id) );
	__asm volatile("mcr p15, 0, %0, c9, c13, 1\t\n" ::"r"(evt) );
#else
#error Unsupported architecture/compiler!
#endif
}


static inline unsigned int get_cyclecnt(void)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	unsigned int r = 0;
	__asm volatile("mrc p15, 0, %0, c9, c13, 0\t\n" : "=r"(r) );
	return r;
#else
#error Unsupported architecture/compiler!
#endif
}


static inline void get_evt_batch(unsigned int *evt0, unsigned int *evt1, unsigned int *evt2, unsigned int *evt3, unsigned int *evt4, unsigned int *evt5)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	unsigned int r;
	//PMSELR select the event counter
	//PMXEVCNTR get the counter value
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(0) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt0) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(1) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt1) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(2) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt2) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(3) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt3) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(4) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt4) );
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(5) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt5) );
#else
#error Unsupported architecture/compiler!
#endif
}

static inline void get_evt(unsigned int id, unsigned int *evt)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	//PMSELR select the event counter
	//PMXEVCNTR get the counter value
	__asm volatile("mcr p15, 0, %0, c9, c12, 5\t\n" ::"r"(id) );
	__asm volatile("mrc p15, 0, %0, c9, c13, 2\t\n" : "=r"(*evt) );
#else
#error Unsupported architecture/compiler!
#endif
}

static inline void rst_instrcnt(void)
{
#if defined(__GNUC__) && defined(__ARM_ARCH_7A__)
	unsigned int r = 0;
	__asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(r) );
	r = r | 0x4;
	__asm volatile("mcr p15, 0, %0, c9, c13, 0" ::"r"(r) );
#else
#error Unsupported architecture/compiler!
#endif
}
