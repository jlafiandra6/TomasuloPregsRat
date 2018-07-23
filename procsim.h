#ifndef PROCSIM_H
#define PROCSIM_H

#define DEFAULT_K0 3
#define DEFAULT_K1 2
#define DEFAULT_K2 1
#define DEFAULT_ROB 12
#define DEFAULT_F 4
#define DEFAULT_PREG 32



typedef struct processor{
	//USE
	uint64_t k0;
	uint64_t k1;
	uint64_t k2;
	uint64_t f;
} processor_t;

typedef struct robentry{
	uint64_t ready;
	int32_t pregno;
	int32_t prevpregno;
	int32_t aregno;
	uint64_t FU;
	uint64_t tag;
	uint64_t inuse;
} robentry_t;

typedef struct ROBs{
	////USE
	robentry_t* entries;
	uint64_t size;
	uint64_t occupancy;
} rob_t;


typedef struct registers{
	uint64_t busy;
	uint64_t occupied;

} registers_t;

typedef struct pregFile{
	//USE
	uint64_t number;
	registers_t* pregs;
} preg_t;

typedef struct aregFile{
	//USE
	uint64_t number;
	registers_t* aregs;
} areg_t;



typedef struct schedulerow{
	int32_t pregindexdest;
	int32_t pregindexSRC1;
	int32_t pregindexSRC2;
	uint64_t tag;
	uint64_t FU;
	uint64_t inuse;
	uint64_t fired;
	uint64_t executed;


} schedrow_t;

typedef struct schedulering{
	uint64_t size;
	uint64_t occupancy;
	schedrow_t* schedrow;
} scheduling_t;



typedef struct _proc_inst_t
{
    uint32_t instruction_address;
    int32_t op_code;
    int32_t src_reg[2];
    int32_t dest_reg;
    
    // You may introduce other fields as needed
    
} proc_inst_t;

typedef struct _proc_stats_t
{
    float avg_inst_retired;
    float avg_inst_fired;
    unsigned long retired_instruction;
    unsigned long instructions_fired;
    unsigned long cycle_count;
    
} proc_stats_t;

bool read_instruction(proc_inst_t* p_inst);

void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg);
void run_proc(proc_stats_t* p_stats);
void complete_proc(proc_stats_t* p_stats);
void dispatch(proc_stats_t* p_stats);
void schedule(proc_stats_t* p_stats);
void execute(proc_stats_t* p_stats);
void state_update(proc_stats_t* p_stats);
int compareSched(const void * a, const void * b);
int compareROB(const void * a, const void * b);

#endif /* PROCSIM_H */
