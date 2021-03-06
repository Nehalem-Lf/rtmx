#define NUM_EVT_CNTS 6
#define NUM_EVTS 66

unsigned int perf_evt_reg[NUM_EVTS]={
0x00,
0x01,
0x02,
0x03,
0x04,
0x05,
0x08,
0x09,
0x0A,
0x0B,
0x10,
0x12,
0x13,
0x14,
0x15,
0x16,
0x17,
0x18,
0x19,
0x1A,
0x1B,
0x1C,
0x1D,
0x40,
0x41,
0x42,
0x43,
0x46,
0x47,
0x48,
0x4C,
0x4D,
0x50,
0x51,
0x52,
0x53,
0x56,
0x57,
0x58,
0x60,
0x61,
0x62,
0x63,
0x64,
0x65,
0x66,
0x67,
0x68,
0x69,
0x6A,
0x6C,
0x6D,
0x6E,
0x70,
0x71,
0x72,
0x73,
0x74,
0x75,
0x76,
0x78,
0x79,
0x7A,
0x7C,
0x7D,
0x7E
};

char perf_evt_name[NUM_EVTS][100]={
"SW_INCR",
"L1I_CACHE_REFILL",
"L1I_TLB_REFILL",
"L1D_CACHE_REFILL",
"L1D_CACHE",
"L1D_TLB_REFILL",
"INST_RETIRED",
"EXC_TAKEN",
"EXC_RETURN",
"CID_WRITE_RETIRED",
"BR_MIS_PRED",
"BR_PRED",
"MEM_ACCESS",
"L1I_CACHE",
"L1D_CACHE_WB",
"L2D_CACHE",
"L2D_CACHE_REFILL",
"L2D_CACHE_WB",
"BUS_ACCESS",
"MEMORY_ERROR",
"INST_SPEC",
"TTBR_WRITE_RETIRED",
"BUS_CYCLES",
"L1D_CACHE_LD",
"L1D_CACHE_ST",
"L1D_CACHE_REFILL_LD",
"L1D_CACHE_REFILL_ST",
"L1D_CACHE_WB_VICTIM",
"L1D_CACHE_WB_CLEAN",
"L1D_CACHE_INVAL",
"L1D_TLB_REFILL_LD",
"L1D_TLB_REFILL_ST",
"L2D_CACHE_LD",
"L2D_CACHE_ST",
"L2D_CACHE_REFILL_LD",
"L2D_CACHE_REFILL_ST",
"L2D_CACHE_WB_VICTIM",
"L2D_CACHE_WB_CLEAN",
"L2D_CACHE_INVAL",
"BUS_ACCESS_LD",
"BUS_ACCESS_ST",
"BUS_ACCESS_SHARED",
"BUS_ACCESS_NOT_SHARED",
"BUS_ACCESS_NORMAL",
"BUS_ACCESS_PERIPH",
"MEM_ACCESS_LD",
"MEM_ACCESS_ST",
"UNALIGNED_LD_SPEC",
"UNALIGNED_ST_SPEC",
"UNALIGNED_LDST_SPEC",
"LDREX_SPEC",
"STREX_PASS_SPEC",
"STREX_FAIL_SPEC",
"LD_SPEC",
"ST_SPEC",
"LDST_SPEC",
"DP_SPEC",
"ASE_SPEC",
"VFP_SPEC",
"PC_WRITE_SPEC",
"BR_IMMED_SPEC",
"BR_RETURN_SPEC",
"BR_INDIRECT_SPEC",
"ISB_SPEC",
"DSB_SPEC",
"DMB_SPEC"
};


