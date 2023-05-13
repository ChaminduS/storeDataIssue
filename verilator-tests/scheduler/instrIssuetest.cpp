#include <stdio.h>
#include <stdlib.h>
#include "VinstrIssue.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define ISSUE_DEPTH 8
#define TIMEOUT 10
#define BRANCH_MASK_WIDTH 4

void tick(int tickcount, VinstrIssue *tb, VerilatedVcdC* tfp){
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
	VinstrIssue *tb = new VinstrIssue;
	
	Verilated::traceEverOn(true);
	VerilatedVcdC* tfp = new VerilatedVcdC;
	tb->trace(tfp, 99);
	tfp->open("instrIssue_trace.vcd");
	
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	printf("\n\
	\n\
	***TEST 1***\
	Make sure the module does not release instructions without allocating any entries\n\
	");
	tb->reset = 0;
	for(int i = 0; i < 100; i++){
		if (tb -> release_ready)
		{
			printf("Test failed!");
			return 0;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 2***\n\
	All ready allocated instructions should fire without any external wakeups\n\
	");

	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i <= ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		while(!tb->allocate_ready)
			tick(++tickcount, tb, tfp);

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		tb -> allocate_instruction = 0x00208033 + (i*0x00108080);
		tb -> allocate_branchMask = 0;
		tb -> allocate_rs1_ready = 1;
		tb -> allocate_rs1_prfAddr = 3*i + 1;
		tb -> allocate_rs2_ready = 1;
		tb -> allocate_rs2_prfAddr = 3*i + 2;
		tb -> allocate_prfDest = 3*i;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++)
	{
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	// reading issued instructions
	// ISSUE_DEPTH instructions was allocated prior to start reading
	for(int i = 0; i <= ISSUE_DEPTH; i++) {
		// wait for released_ready
		tb -> release_fired = 0;
		while(!tb -> release_ready)
			tick(++tickcount, tb, tfp);

		tb -> release_fired = 1;
		if(tb -> release_instruction != (0x00208033 + (i*0x00108080)) ||
  		tb -> release_branchMask != 0 ||
  		tb -> release_rs1prfAddr != 3*i + 1 ||
			tb -> release_rs2prfAddr != 3*i + 2 ||
  		tb -> release_prfDest != 3*i ||
  		tb -> release_robAddr != (i&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", (0x00208033 + (i*0x00108080)), 0, 3*i + 1, 3*i + 2, 3*i, (i&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
			}
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 0;
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 3***\n\
	All instructions instructions allocated with sources desasserted, they should not fire without wakeups\n\
	");


	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted allocate_ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> allocate_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		tb -> allocate_instruction = 0x02208033 + (i*0x00108080);
		tb -> allocate_branchMask = 0;
		tb -> allocate_rs1_ready = 0;
		tb -> allocate_rs1_prfAddr = 3*i + 1;
		tb -> allocate_rs2_ready = 0;
		tb -> allocate_rs2_prfAddr = 3*i + 2;
		tb -> allocate_prfDest = 3*i;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++) {
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		if(tb-> release_ready) {
			printf("Test failed: Module asserted release.ready when no instruction should be ready\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	// we'll be releasing them in the reverse order (i.e. waking them up in the reverse order)
	for(int i = 1; i < ISSUE_DEPTH + 1; i++) {
		tb-> release_fired = 0;
		// waking up only one src at a time
		tb -> wakeUpExt_0_valid = 1;
		tb -> wakeUpExt_0_prfAddr = (8 - i)*3 + 1;
		tb -> wakeUpExt_1_valid = 0;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_0_valid = 0;

		// no instruction should fire
		for (int i = 0; i < TIMEOUT; i++) {
			if(tb-> release_ready) {
				printf("Test failed: Module asserted release.ready when no instruction should be ready\n\n\n");
				return -1;
			}
			tick(++tickcount, tb, tfp);
		}

		tb -> wakeUpExt_1_valid = 1;
		tb -> wakeUpExt_1_prfAddr = (8 - i)*3 + 2;
		tb -> wakeUpExt_0_valid = 0;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_1_valid = 0;

		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> release_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		tb -> release_fired = 1;
		if(tb -> release_instruction != (0x02208033 + ((8-i)*0x00108080)) ||
  		tb -> release_branchMask != 0 ||
  		tb -> release_rs1prfAddr != 3*(8-i) + 1 ||
			tb -> release_rs2prfAddr != 3*(8-i) + 2 ||
  		tb -> release_prfDest != 3*(8-i) ||
  		tb -> release_robAddr != ((8-i)&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", (0x00208033 + ((8-i)*0x00108080)), 0, 3*(8-i) + 1, 3*(8-i) + 2, 3*(8-i), ((8-i)&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 5***\n\
	No ready sources should be ready in the next cycle, if the dependent instruction is single cycle at exec and is issued\n\
	");


	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted allocate_ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> allocate_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		tb -> allocate_instruction = (0x00208033 + (i*0x00108080));
		tb -> allocate_branchMask = 0;
		tb -> allocate_rs1_ready = 0;
		tb -> allocate_rs1_prfAddr = i ? 1 : 4;
		tb -> allocate_rs2_ready = 0;
		tb -> allocate_rs2_prfAddr = i ? 1 : 5;
		tb -> allocate_prfDest = i ? 20 : 1;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++) {
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		if(tb-> release_ready) {
			printf("Test failed: Module asserted release.ready when no instruction should be ready\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	tb -> wakeUpExt_1_valid = 1;
	tb -> wakeUpExt_1_prfAddr = 4;
	tb -> wakeUpExt_0_valid = 1;
	tb -> wakeUpExt_0_prfAddr = 5;
	tick(++tickcount, tb, tfp);
	tb -> wakeUpExt_1_valid = 0;
	tb -> wakeUpExt_0_valid = 0;

	for(int i = 0; i < ISSUE_DEPTH; i++) {
		tb-> release_fired = 0;

		

		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> release_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		tb -> release_fired = 1;
		if(tb -> release_instruction != (0x00208033 + (i*0x00108080)) ||
  		tb -> release_branchMask != 0 ||
  		tb -> release_rs1prfAddr != (i ? 1 : 4) ||
			tb -> release_rs2prfAddr != (i ? 1 : 5) ||
  		tb -> release_prfDest != (i ? 20 : 1)||
  		tb -> release_robAddr != (i&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
				", (0x00208033 + (i*0x00108080)) , 0, i ? 1 : 4, i ? 1 : 5, i ? 20 : 1, (i&7));
				printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 6***\n\
	Branches should be issued in order of program\n\
	");

	tb -> release_fired = 0;
	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted allocate_ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> allocate_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		// 1st instruction allocated is a branch and everyother instruction allocated is a branch
		tb -> allocate_instruction = (i&1) ? 0x00208033 : 0x00521a63;
		tb -> allocate_branchMask = (1 << ((i+1)/2)) - 1;
		tb -> allocate_rs1_ready = 0;
		tb -> allocate_rs1_prfAddr = i*3 + 1;
		tb -> allocate_rs2_ready = 0;
		tb -> allocate_rs2_prfAddr = i*3 + 2;
		tb -> allocate_prfDest = i*3;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++) {
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		if(tb-> release_ready) {
			printf("Test failed: Module asserted release.ready when no instruction should be ready\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	// releasing the non-branch instructions
	for (int i = 0; i < 4; i++) {
		tb-> release_fired = 0;

		tb -> wakeUpExt_1_valid = 1;
		tb -> wakeUpExt_1_prfAddr = (2*i+1)*3 + 1;
		tb -> wakeUpExt_0_valid = 1;
		tb -> wakeUpExt_0_prfAddr = (2*i+1)*3 + 2;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_1_valid = 0;
		tb -> wakeUpExt_0_valid = 0;

		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> release_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		tb -> release_fired = 1;
		if(tb -> release_instruction != 0x00208033 ||
  		tb -> release_branchMask != (1<<(i+1))-1 ||
  		tb -> release_rs1prfAddr != (2*i+1)*3 + 1 ||
			tb -> release_rs2prfAddr != (2*i+1)*3 + 2 ||
  		tb -> release_prfDest != (2*i+1)*3 ||
  		tb -> release_robAddr != ((2*i + 1)&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", 0x00208033, (1<<(i+1))-1, (2*i+1)*3 + 1, (2*i+1)*3 + 2, (2*i+1)*3, ((2*i + 1)&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 0;

	// waking up the remaining branch instructions
	// instructions are woken up in the order of newest to oldest
	// because the remaining instructions are branches they should not fire
	// branches are issued in-order, no matter how the instructions are woken up
	for(int i = 0; i < 4; i++) {
		for (int i = 0; i <= TIMEOUT; i++) {
			if(tb-> release_ready) {
				printf("Test failed: Branches should be issued in-order, hence no instruction should be ready\n\n\n\n");
				return -1;
			}
			tick(++tickcount, tb, tfp);
		}

		tb -> wakeUpExt_1_valid = 1;
		tb -> wakeUpExt_1_prfAddr = (6 - 2*i)*3 + 1;
		tb -> wakeUpExt_0_valid = 1;
		tb -> wakeUpExt_0_prfAddr = (6 - 2*i)*3 + 2;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_1_valid = 0;
		tb -> wakeUpExt_0_valid = 0;
	}

	// releasing the branch instructions
	for (int i = 0; i < 4; i++) {
		tb-> release_fired = 0;
		
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> release_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		tb -> release_fired = 1;
		if(tb -> release_instruction != 0x00521a63 ||
  		tb -> release_branchMask != (1<<(i))-1 ||
  		tb -> release_rs1prfAddr != (2*i)*3 + 1 ||
			tb -> release_rs2prfAddr != (2*i)*3 + 2 ||
  		tb -> release_robAddr != ((2*i)&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", 0x00521a63, (1<<(i+1))-1, (2*i)*3 + 1, (2*i)*3 + 2, (2*i)*3, ((2*i)&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 0;
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 7***\n\
	Branch miss/hit functionality\n\
	");


	tb -> release_fired = 0;
	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted allocate_ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> allocate_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		// 1st instruction allocated is a branch and everyother instruction allocated is a branch
		tb -> allocate_instruction = (i&1) ? 0x00208033 : 0x00521a63;
		tb -> allocate_branchMask = (1 << ((i+1)/2)) - 1;
		tb -> allocate_rs1_ready = 0;
		tb -> allocate_rs1_prfAddr = i*3 + 1;
		tb -> allocate_rs2_ready = 0;
		tb -> allocate_rs2_prfAddr = i*3 + 2;
		tb -> allocate_prfDest = i*3;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// we are going to simulate the failure of the second allocated instruction
	// we need to get the second issued instruction out of the queue to mimic the pipeline
	// branches are issued in-order, no matter how the instructions are woken up
	for(int i = 0; i < 2; i++) {
		for (int i = 0; i <= TIMEOUT; i++) {
			if(tb-> release_ready) {
				printf("Test failed: Branches should be issued in-order, hence no instruction should be ready\n\n\n\n");
				return -1;
			}
			tick(++tickcount, tb, tfp);
		}

		tb -> wakeUpExt_1_valid = 1;
		tb -> wakeUpExt_1_prfAddr = (2 - 2*i)*3 + 1;
		tb -> wakeUpExt_0_valid = 1;
		tb -> wakeUpExt_0_prfAddr = (2 - 2*i)*3 + 2;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_1_valid = 0;
		tb -> wakeUpExt_0_valid = 0;
	}

	// releasing the 2 branch instructions
	for (int i = 0; i < 2; i++) {
		tb-> release_fired = 0;
		
		for (int i = 0; i <= TIMEOUT; i++) {
			if(i == TIMEOUT) {
				printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
				return -1;
			}
			if(tb-> release_ready)
				break;
			tick(++tickcount, tb, tfp);
		}

		tb -> release_fired = 1;
		if(tb -> release_instruction != 0x00521a63 ||
  		tb -> release_branchMask != (1<<(i))-1 ||
  		tb -> release_rs1prfAddr != (2*i)*3 + 1 ||
			tb -> release_rs2prfAddr != (2*i)*3 + 2 ||
  		tb -> release_robAddr != ((2*i)&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", 0x00521a63, (1<<(i+1))-1, (2*i)*3 + 1, (2*i)*3 + 2, (2*i)*3, ((2*i)&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
		}
		tick(++tickcount, tb, tfp);
	}

	tb -> branchOps_valid = 1;
	tb -> branchOps_branchMask = 0b0001;
	tb -> branchOps_passed = 1;
	tick(++tickcount, tb, tfp);
	tb -> branchOps_passed = 0;
	tb -> branchOps_branchMask = 0b0010;
	tick(++tickcount, tb, tfp);
	tb -> branchOps_valid = 0;

	// Only one instruction should remain in queue
	// waking up that instruction
	tb -> wakeUpExt_1_valid = 1;
	tb -> wakeUpExt_1_prfAddr = 4;
	tb -> wakeUpExt_0_valid = 1;
	tb -> wakeUpExt_0_prfAddr = 5;
	tick(++tickcount, tb, tfp);
	tb -> wakeUpExt_1_valid = 0;
	tb -> wakeUpExt_0_valid = 0;
	tick(++tickcount, tb, tfp);

	// waiting for ready
	for (int i = 0; i <= TIMEOUT; i++) {
		if(i == TIMEOUT) {
			printf("Test failed: Module did not asserted release.ready before timeout\n\n\n");
			return -1;
		}
		if(tb-> release_ready)
			break;
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 1;
	if(tb -> release_instruction != 0x00208033 ||
		tb -> release_branchMask != 0 ||
		tb -> release_rs1prfAddr != 4 ||
		tb -> release_rs2prfAddr != 5 ||
		tb -> release_prfDest != 3 ||
		tb -> release_robAddr != 1) {
			printf("Test faild: Instructions was not issued correctly\n\
			");
			printf("Expected:\n\
				instruction	:%#010x\n\
				branchMask	:%#03x\n\
				rs1prfAddr	:%#04x\n\
				rs2prfAddr	:%#04x\n\
				prfDest	:%#04x\n\
				robAddr	:%#03x\n\
				", 0x00208033, 0, 4, 5, 3, 1);
				printf("Recieved:\n\
				instruction	:%#010x\n\
				branchMask	:%#03x\n\
				rs1prfAddr	:%#04x\n\
				rs2prfAddr	:%#04x\n\
				prfDest	:%#04x\n\
				robAddr	:%#03x\n\
				", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
			return -1;
	}
	tick(++tickcount, tb, tfp);

	// the queue should be empty, i.e. we should be able fill the queue with new (ISSUE_WIDTH+1) instructions
	tb->release_fired = 0;
	for(int i = 0; i <= ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		for (int j = 0; j <= TIMEOUT; j++) {
			if(j == TIMEOUT) {
				printf("Test failed: Module did not asserted allocate.ready before at i = %d timeout. Branch miss did not fully empty queue\n\n\n", i);
				return -1;
			}
			if(tb-> allocate_ready)
				break;
		tick(++tickcount, tb, tfp);
	}

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		tb -> allocate_instruction = 0x00208033 + (i*0x00108080);
		tb -> allocate_branchMask = 0;
		tb -> allocate_rs1_ready = 1;
		tb -> allocate_rs1_prfAddr = 3*i + 1;
		tb -> allocate_rs2_ready = 1;
		tb -> allocate_rs2_prfAddr = 3*i + 2;
		tb -> allocate_prfDest = 3*i;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++)
	{
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	// reading issued instructions
	// ISSUE_DEPTH instructions was allocated prior to start reading
	for(int i = 0; i <= ISSUE_DEPTH; i++) {
		// wait for released_ready
		tb -> release_fired = 0;
		while(!tb -> release_ready)
			tick(++tickcount, tb, tfp);

		tb -> release_fired = 1;
		if(tb -> release_instruction != (0x00208033 + (i*0x00108080)) ||
  		tb -> release_branchMask != 0 ||
  		tb -> release_rs1prfAddr != 3*i + 1 ||
			tb -> release_rs2prfAddr != 3*i + 2 ||
  		tb -> release_prfDest != 3*i ||
  		tb -> release_robAddr != (i&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", (0x00208033 + (i*0x00108080)), 0, 3*i + 1, 3*i + 2, 3*i, (i&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
			}
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 0;
	printf("Test passed!\n\
	\n\
	\n\
	***TEST 8***\n\
	All instructions instructions allocated with sources desasserted, they should not fire without wakeups\n\
	");


	// setting reset
	tb->reset = 1;
	for(int i = 0; i < 20; i++){
		tick(++tickcount, tb, tfp);
	}
	tb->reset = 0;
	tb->release_fired = 0;
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// waiting for module to be ready
		tb -> allocate_fired = 0;
		while(!tb->allocate_ready)
			tick(++tickcount, tb, tfp);

		// allocating instruction in the issue queue
		tb -> allocate_fired = 1;
		tb -> allocate_instruction = 0x04013283;
		tb -> allocate_branchMask = 0;
		tb -> allocate_rs1_ready = 0;
		tb -> allocate_rs1_prfAddr = i + 2;
		tb -> allocate_rs2_ready = 0;
		tb -> allocate_rs2_prfAddr = 1;
		tb -> allocate_prfDest = 30;
		tb -> allocate_robAddr = (i&7);

		tick(++tickcount, tb, tfp);
	}
	tb -> allocate_fired = 0;

	// allocate_ready should not be asserted until atleast one allocated instruction is released
	for (int i = 0; i < 100; i++)
	{
		if(tb-> allocate_ready) {
			printf("Test failed: Module asserted allocated.ready when the module is full\n\n\n");
			return -1;
		}
		tick(++tickcount, tb, tfp);
	}
	printf("Module does not assert \"allocate.ready\" as expected when queue is full\n\
	");

	// waking up the instructions
	for(int i = 0; i < 4; i++) {
		for (int i = 0; i <= TIMEOUT; i++) {
			if(tb-> release_ready) {
				printf("Test failed: Branches should be issued in-order, hence no instruction should be ready\n\n\n\n");
				return -1;
			}
			tick(++tickcount, tb, tfp);
		}

		tb -> wakeUpExt_1_valid = 1;
		tb -> wakeUpExt_1_prfAddr = 2 + (3-i)*2;
		tb -> wakeUpExt_0_valid = 1;
		tb -> wakeUpExt_0_prfAddr = 3 + (3-i)*2;
		tick(++tickcount, tb, tfp);
		tb -> wakeUpExt_1_valid = 0;
		tb -> wakeUpExt_0_valid = 0;
	}

	// reading issued instructions
	// ISSUE_DEPTH instructions was allocated prior to start reading
	for(int i = 0; i < ISSUE_DEPTH; i++) {
		// wait for released_ready
		tb -> release_fired = 0;
		while(!tb -> release_ready)
			tick(++tickcount, tb, tfp);

		tb -> release_fired = 1;
		if(tb -> release_instruction != 0x04013283 ||
  		tb -> release_branchMask != 0 ||
  		tb -> release_rs1prfAddr != i + 2 ||
			tb -> release_rs2prfAddr != 1 ||
  		tb -> release_prfDest != 30 ||
  		tb -> release_robAddr != (i&7)) {
				printf("Test faild: Instructions was not issued correctly at i = %d\n\
				", i);
				printf("Expected:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", 0x04013283, 0, i + 2, 1, 30, (i&7));
					printf("Recieved:\n\
					instruction	:%#010x\n\
					branchMask	:%#03x\n\
					rs1prfAddr	:%#04x\n\
					rs2prfAddr	:%#04x\n\
					prfDest	:%#04x\n\
					robAddr	:%#03x\n\
					", tb -> release_instruction, tb -> release_branchMask, tb -> release_rs1prfAddr, tb -> release_rs2prfAddr, tb -> release_prfDest, tb -> release_robAddr);
				return -1;
			}
		tick(++tickcount, tb, tfp);
	}
	tb -> release_fired = 0;
	printf("Test passed!\n\
	\n\
	\n\
	***ALL TESTS PASSED***\n\
	");
}

