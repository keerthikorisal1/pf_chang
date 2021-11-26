#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"

struct Request;

#define N 13
#define STORE 100

class Prefetcher {
	private:
	bool _readys[N];
	bool _strider_ready;
	Request _nextReq;

	long long last_addrs[STORE];
	short fifo;
	long long prediction;
	short step;
	Request _striderReq;

  public:
	Prefetcher();
	// should return true if a request is ready for this cycle
	bool hasRequest(u_int32_t cycle);
	bool hasChunkRequest();
	bool hasStrideRequest();

	// request a desired address be brought in
	Request getRequest(u_int32_t cycle);

	// this function is called whenever the last prefetcher request was successfully sent to the L2
	void completeRequest(u_int32_t cycle);

	// go through the last STORE addresses and determine if there are any linear strides
	void findPattern();
	// will get the index of last_addrs relative to fifo (addr is interpreted like fifo = 0, i.e. addr = 5 -> (fifo - 5)%STORE)
	short getIdx(short addr);

	/*
	 * This function is called whenever the CPU references memory.
	 * Note that only the addr, pc, load, issuedAt, and HitL1 should be considered valid data
	 */
	void cpuRequest(Request req); 
};

#endif
