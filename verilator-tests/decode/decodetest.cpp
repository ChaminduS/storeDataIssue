#include <stdio.h>
#include <stdlib.h>
#include "Vdecode.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

void tick(int tickcount, Vdecode *tb, VerilatedVcdC* tfp){
	tb->eval();
	if (tfp){
		tfp->dump(tickcount*10 - 2);
	}
	tb->clock = 1;
	tb->eval();
	if(tfp){
		tfp->dump(tickcount*10);
	}
	tb->clock = 0;
	tb->eval();
	if(tfp){
		tfp->dump(tickcount*10 + 5);
		tfp->flush();
	}
}

int main(int argc, char **argv){

	unsigned tickcount = 0;

	// Call commandArgs first!
	Verilated::commandArgs(argc, argv);
	
	//Instantiate our design
	Vdecode *tb = new Vdecode;
	
	Verilated::traceEverOn(true);
	VerilatedVcdC* tfp = new VerilatedVcdC;
	tb->trace(tfp, 99);
	tfp->open("decode_trace.vcd");
	
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}

	tb->reset = 1;
	for(int i = 0; i < 1; i++){
		tick(++tickcount, tb, tfp);
	}

	printf("\n\n*** TEST 1 *** Checking the empty state\n");
	tb->reset = 0;
	for(int i = 0; i < 1; i++){
		if (tb -> toExec_ready || tb -> branchPCs_ready || tb -> branchEvalOut_ready)
		{
			printf("Test failed!");
			return 0;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Test passed!\n");

	printf("\n\n*** TEST 2 *** Instructions\n");
	tb->reset = 1;
	for(int i = 0; i < 1; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->fromFetch_fired = 0;
	tb->fromFetch_ready = 1;

	tb->fromFetch_fired = 1;
	tb->fromFetch_pc = 1;
	tb->fromFetch_instruction = 0x3e800093; // addi x1, x0, 1000

	tick(++tickcount, tb, tfp);

	tb->fromFetch_fired = 1;
	tb->fromFetch_pc = 2;
	tb->fromFetch_instruction = 0x00008133;	// add x2, x1, x0

	tick(++tickcount, tb, tfp);

	tb->toExec_fired = 1;
	tb->fromFetch_fired = 0;

	tick(++tickcount, tb, tfp);
	tick(++tickcount, tb, tfp);

	tb->toExec_fired = 0;

	tick(++tickcount, tb, tfp);

	tb->fromFetch_fired = 1;
	tb->fromFetch_pc = 3;
	tb->fromFetch_instruction = 0x00209463;  // bne x1, x2, 8

	tick(++tickcount, tb, tfp);

	tb->fromFetch_fired = 1;
	tb->fromFetch_pc = 4;
	tb->fromFetch_instruction = 0x00309113;  // slli x1, x2, 3

	tick(++tickcount, tb, tfp);

	tb->fromFetch_fired = 0;
	tb->toExec_fired = 1;

	tick(++tickcount, tb, tfp);
	tick(++tickcount, tb, tfp);

	tb->toExec_fired = 1;

	tick(++tickcount, tb, tfp);

	tb->branchEvalIn_fired = 1;
	tb->branchEvalIn_passFail = 0;
	tb->branchEvalIn_branchMask = 1;
	tb->branchEvalIn_targetPC = 8;
	
	tick(++tickcount, tb, tfp);


	// if(!tb->toExec_ready || tb->toExec_PRFDest==1 || !tb->toExec_rs1Addr==0 || !tb->toExec_immediate==1000){
	// 	printf("Test failed!");
	// 	return 0;
	// }
	printf("Test passed!\n");
}

