#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"
#include <set>
class Prefetcher {
  private:
	static const u_int32_t  L1_STEP_VALUE=16;
	static const u_int32_t  L2_STEP_VALUE=32;
	static const int MAX_L2_BLOCK_DIST=23; //number of L2 blocks ahead of current address to fetch
	bool ready;
	u_int32_t nextReqAddr;

	u_int32_t blockStartAddr(u_int32_t addr, int size);
  public:
	Prefetcher();

	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req); 
};

#endif
