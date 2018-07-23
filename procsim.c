#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "procsim.h"



proc_inst_t* p_inst;
scheduling_t* scheduler;
processor_t* process;
areg_t* aregsFile;
preg_t* pregsFile;
int32_t* rat;
rob_t* ROB1;
uint64_t counter = 1;
int stalled = 0;
uint64_t* availFU;
bool done = false;
/**
 * Subroutine for initializing the processor. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @k0 Number of k0 FUs
 * @k1 Number of k1 FUs
 * @k2 Number of k2 FUs
 * @f Number of instructions to fetch
 * @ROB Number of ROB Entries
 * @PREG Number of registers in the PRF
 */
void setup_proc(uint64_t k0, uint64_t k1, uint64_t k2, uint64_t f, uint64_t rob, uint64_t preg) 
{
	p_inst = (proc_inst_t*) malloc( sizeof(proc_inst_t));

	 ROB1 = (rob_t*) calloc(1, sizeof(rob_t));
	 ROB1->size = rob;
	 ROB1->entries = (robentry_t*) calloc(rob, sizeof(robentry_t));

	 process = (processor_t*) calloc(1, sizeof(processor_t));
	 process->k0 = k0;
	 process->k1 = k1;
	 process->k2 = k2;
	 process->f = f;
	 availFU = (uint64_t*) calloc(3, sizeof(uint64_t));
	 availFU[0] = k0;
	 availFU[1] = k1;
	 availFU[2] = k2;

	 aregsFile = (areg_t*) calloc(1, sizeof(areg_t));
	 aregsFile->number = 32;
	 aregsFile->aregs = (registers_t*) calloc(32, sizeof(registers_t));

	 pregsFile = (preg_t*) calloc(1, sizeof(preg_t));
	 pregsFile->number = preg;
	 pregsFile->pregs = (registers_t*) calloc(preg, sizeof(registers_t));

	 rat = (int32_t*) calloc(32, sizeof(int));
	 for(int x = 0; x<32;x++){
	 	rat[x] = -1;
	 }

	 

	 scheduler = (scheduling_t*) calloc(1, sizeof(scheduling_t));
	 scheduler->size = 2 * (k0+k1+k2);
	
	 scheduler->schedrow = (schedrow_t*) calloc(2*(k0+k1+k2), sizeof(schedrow_t));

}

/**
 * Subroutine that simulates the processor.
 *   The processor should fetch instructions as appropriate, until all instructions have executed
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void run_proc(proc_stats_t* p_stats)
{
	p_stats->cycle_count=0;
	p_stats->retired_instruction = 0;
    p_stats->instructions_fired = 0;
	uint64_t z = 1;
	while (done == false || ROB1->occupancy != 0){
		p_stats->cycle_count=p_stats->cycle_count+1;
		//printf("------------------------------------------  Cycle %llu 		 ---------------------------------\n",z);
		state_update(p_stats);
		
		execute(p_stats);

		schedule(p_stats);
		

		dispatch(p_stats);
		//printf("---------------------------------------------------------------------------------------------------\n\n");
	//qsort(scheduler->schedrow, scheduler->size, sizeof(schedrow_t), compareSched);
		z++;
	}

}

/**
 * Subroutine for cleaning up any outstanding instructions and calculating overall statistics
 * such as average IPC, average fire rate etc.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_proc(proc_stats_t *p_stats) 
{

	p_stats->avg_inst_retired = 1.0 * p_stats->retired_instruction / p_stats->cycle_count;
	p_stats->avg_inst_fired = 1.0 * p_stats->instructions_fired / p_stats->cycle_count;
}



void dispatch(proc_stats_t* p_stats)
{
	uint64_t freepregs = 0;
	uint64_t tofetch = process->f;
	uint64_t pregdestt;
	int robplaced = 0;
	int schedplaced =0;
	bool instructionsleft = true;



	for(int x =0; x < pregsFile->number;x++){
		if(pregsFile->pregs[x].occupied == 0){
			freepregs++;
		}

	}

/*
	if(totalnotbusy < tofetch){
		tofetch = totalnotbusy;
		if(p_stats->cycle_count == 18606){
			for(int k = 0; k < pregsFile->number;k++){
				printf("Preg#:%d,Occupied Preg:%llu\n",k, pregsFile->pregs[k].occupied);
			}
			for(int j = 0; j < pregsFile->number;j++){
				printf("RAT#:%d,PregID:%d\n",j, rat[j]);
			}
		}

	}
*/
	if((scheduler->size - scheduler->occupancy) < tofetch){
		tofetch = (scheduler->size - scheduler->occupancy);
	}
	
	if(ROB1->size - ROB1->occupancy < tofetch){
		tofetch = (ROB1->size - ROB1->occupancy);
		
	}
	//printf("ROBsize: %llu ROB:%llu FETCH:%llu\n", ROB1->size,ROB1->occupancy, tofetch);
	for(uint64_t y= 0; y < tofetch && freepregs > 0;y++){
		pregdestt = 0;
		schedplaced = 0;
		robplaced = 0;
		instructionsleft = read_instruction(p_inst);

		if(instructionsleft == false){
			break;
		}

		for(int z =0; z < pregsFile->number; z++){
			if(pregsFile->pregs[z].occupied == 0){
				pregdestt = z;
				break;
			}

		}

		//printf("DU: Inst Num %llu \n",counter);
		for(int x =0; x < scheduler->size;x++){
			if(scheduler->schedrow[x].inuse == 0 && schedplaced != 1){
				schedplaced = 1;
				scheduler->schedrow[x].inuse = 1;
				scheduler->schedrow[x].fired = 0;
				scheduler->schedrow[x].FU = (uint64_t) abs(p_inst->op_code);
				scheduler->schedrow[x].tag = counter;
				scheduler->schedrow[x].executed = 0;

				if(p_inst->src_reg[0] == -1){
					scheduler->schedrow[x].pregindexSRC1 = -1;
				}
				else{
					scheduler->schedrow[x].pregindexSRC1 = rat[p_inst->src_reg[0]];
				}

				if(p_inst->src_reg[1] == -1){
					scheduler->schedrow[x].pregindexSRC2 = -1;
				}
				else{
					scheduler->schedrow[x].pregindexSRC2 = rat[p_inst->src_reg[1]];
				}
				
				if(p_inst->dest_reg == -1){
					scheduler->schedrow[x].pregindexdest = -1;
				}
				else{
					pregsFile->pregs[pregdestt].busy = 1;
					freepregs = freepregs-1;
					pregsFile->pregs[pregdestt].occupied = 1;
					scheduler->schedrow[x].pregindexdest = pregdestt;
				}
			}
		}

		scheduler->occupancy = scheduler->occupancy + 1;

		for(int x =0; x < ROB1->size;x++) {
			if(ROB1->entries[x].inuse == 0 && robplaced != 1){
				robplaced = 1;
				ROB1->entries[x].inuse = 1;
				ROB1->entries[x].FU = (uint64_t) abs(p_inst->op_code);
				ROB1->entries[x].tag = counter;
				ROB1->entries[x].ready = 0;
				ROB1->entries[x].aregno = p_inst->dest_reg;
				if(p_inst->dest_reg == -1){
					ROB1->entries[x].pregno = -1;
					ROB1->entries[x].prevpregno = -1;
				}
				else{
					ROB1->entries[x].prevpregno = rat[p_inst->dest_reg];
					ROB1->entries[x].pregno = pregdestt;
				}
				
				
			}
		}
		ROB1->occupancy = ROB1->occupancy + 1;
		//TODO NEED TO ADD if prevPREGNO == -1 then do shit in ROB
		if(p_inst->dest_reg != -1){
			rat[p_inst->dest_reg] = pregdestt;
		}
		
		
		counter++;
	}
	if(instructionsleft == false){
		done = true;
	}


}


void schedule(proc_stats_t* p_stats){
	qsort(scheduler->schedrow, scheduler->size, sizeof(schedrow_t), compareSched);

	
	for(int x =0; x < scheduler->size;x++){
			if(scheduler->schedrow[x].inuse == 1 && scheduler->schedrow[x].fired == 0){
				if(scheduler->schedrow[x].tag == 945){
						//printf("cycle:%lu\n", p_stats->cycle_count);

				}
				if(availFU[scheduler->schedrow[x].FU] != 0){
						//printf("Wants:%llu\n", scheduler->schedrow[x].FU);
						//printf("cycle:%lu , FU0 left:%llu,FU1 left:%llu,FU2 left:%llu\n", p_stats->cycle_count,availFU[0],availFU[1],availFU[2]);

					if(scheduler->schedrow[x].pregindexSRC1 == -1 && scheduler->schedrow[x].pregindexSRC2 == -1){
						//printf("SCHED Inst Num: %llu \n",scheduler->schedrow[x].tag);
						availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] - 1;
						scheduler->schedrow[x].fired = 1;
						p_stats->instructions_fired = p_stats->instructions_fired +1;
					} else if (scheduler->schedrow[x].pregindexSRC1 == -1 && (scheduler->schedrow[x].pregindexSRC2 == scheduler->schedrow[x].pregindexdest || pregsFile->pregs[scheduler->schedrow[x].pregindexSRC2].busy == 0)){
						availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] - 1;
						scheduler->schedrow[x].fired = 1;
						p_stats->instructions_fired = p_stats->instructions_fired +1;
						//printf("SCHED Inst Num: %llu \n",scheduler->schedrow[x].tag);

					} else if (scheduler->schedrow[x].pregindexSRC2 == -1 && (scheduler->schedrow[x].pregindexSRC1 == scheduler->schedrow[x].pregindexdest || pregsFile->pregs[scheduler->schedrow[x].pregindexSRC1].busy == 0)){
						availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] - 1;
						scheduler->schedrow[x].fired = 1;
						p_stats->instructions_fired = p_stats->instructions_fired +1;
						//printf("SCHED Inst Num: %llu \n",scheduler->schedrow[x].tag);

					} else if((scheduler->schedrow[x].pregindexSRC1 == scheduler->schedrow[x].pregindexdest || pregsFile->pregs[scheduler->schedrow[x].pregindexSRC1].busy == 0) && (scheduler->schedrow[x].pregindexSRC2 == scheduler->schedrow[x].pregindexdest || pregsFile->pregs[scheduler->schedrow[x].pregindexSRC2].busy == 0)){
						availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] - 1;
						scheduler->schedrow[x].fired = 1;
						//printf("SCHED Inst Num: %llu \n",scheduler->schedrow[x].tag);
						p_stats->instructions_fired = p_stats->instructions_fired +1;

					}

				}
			}
	}	
				
	//printf("after: %llu\n ,%llu\n,%llu\n", availFU[0],availFU[1],availFU[2]);

}



void state_update(proc_stats_t* p_stats){
	qsort(ROB1->entries, ROB1->size, sizeof(robentry_t), compareROB);
	for(int y = 0 ; y < ROB1->size;y++){
		if(ROB1->entries[y].inuse == 1 && ROB1->entries[y].ready == 1){
			p_stats->retired_instruction=p_stats->retired_instruction+1;
			ROB1->entries[y].inuse = 0;
			ROB1->occupancy = ROB1->occupancy - 1;

			//printf("RU Inst: %llu \n", ROB1->entries[y].tag);
			if(ROB1->entries[y].prevpregno!= -1){
				pregsFile->pregs[ROB1->entries[y].prevpregno].occupied = 0;
			}


			for(int x =0; x < scheduler->size;x++){
				if(scheduler->schedrow[x].tag == ROB1->entries[y].tag && scheduler->schedrow[x].inuse == 1){
					scheduler->schedrow[x].inuse = 0;
					scheduler->schedrow[x].fired = 0;
					scheduler->occupancy = scheduler->occupancy - 1;
				} 		



			}
		} else if(ROB1->entries[y].inuse == 1 &&  ROB1->entries[y].ready == 0){
			break;
		} 	

	}

			

}





void execute(proc_stats_t* p_stats){

	for(int x =0; x < scheduler->size;x++){
			if(scheduler->schedrow[x].inuse == 1 && scheduler->schedrow[x].fired == 1 && scheduler->schedrow[x].pregindexdest == -1 && scheduler->schedrow[x].executed == 0){
				availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] + 1;
				//printf("EX : Type: %llu Inst_num: %llu\n",scheduler->schedrow[x].FU,scheduler->schedrow[x].tag);
				scheduler->schedrow[x].executed = 1;
				for(int y =0; y < ROB1->size;y++){
					if(ROB1->entries[y].inuse == 1 && ROB1->entries[y].tag == scheduler->schedrow[x].tag ){
						ROB1->entries[y].ready = 1;
					}
				}
			} else if(scheduler->schedrow[x].inuse == 1 && scheduler->schedrow[x].fired == 1 && scheduler->schedrow[x].executed == 0){
				scheduler->schedrow[x].executed = 1;

				//printf("EX : Type: %llu Inst_num: %llu\n",scheduler->schedrow[x].FU,scheduler->schedrow[x].tag);
				availFU[scheduler->schedrow[x].FU] = availFU[scheduler->schedrow[x].FU] + 1;
				pregsFile->pregs[scheduler->schedrow[x].pregindexdest].busy = 0;	

				for(int y =0; y < ROB1->size;y++){
					if(ROB1->entries[y].inuse == 1 && ROB1->entries[y].tag == scheduler->schedrow[x].tag ){
						ROB1->entries[y].ready = 1;
					}
				}

			}

	}	


}

int compareROB(const void * a, const void * b) {
	if(((robentry_t*)a)->tag > ((robentry_t*)b)->tag){
		return 1;
	} else{
		return -1;
	}
}

int compareSched(const void * a, const void * b) {
	if( ((schedrow_t*)a)->FU <((schedrow_t*)b)->FU) {
		return -1;
	} else if( ((schedrow_t*)a)->FU > ((schedrow_t*)b)->FU) {
		return 1;
	} else{
		if( ((schedrow_t*)a)->tag > ((schedrow_t*)b)->tag){
			return 1;
		}else{
			return -1;
		}
	}
}


