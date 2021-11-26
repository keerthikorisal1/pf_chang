#include "prefetcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <climits>
int count=0;
Prefetcher::Prefetcher():
    nextReqAddr(),ready()
{
}

u_int32_t Prefetcher::blockStartAddr(u_int32_t addr,int size)
{
    return addr/size*size;
}
// should return true if a request is ready for this cycle

bool Prefetcher::hasRequest(u_int32_t cycle)
{
    return ready;
}

// request a desired address be brought in
/*
 * NOTES: u_int32_t cycle will always be the same as it was in hasRequest
 */

Request Prefetcher::getRequest(u_int32_t cycle)
{
    Request nextReq={};
    nextReq.addr=nextReqAddr;
    return nextReq;
}

// this function is called whenever the last prefetcher request was successfully sent to the L2
//only doesn't get sent if the L2 queue is full!
void Prefetcher::completeRequest(u_int32_t cycle)
{
    ready = false;
}

/*
 * This function is called whenever the CPU references memory.
 * Note that only the addr, pc, load, fromCPU, issuedAt, and HitL1 should be considered valid data
 */
void Prefetcher::cpuRequest(Request req)
{

    if(ready || !req.fromCPU)
	return;

    u_int32_t reqAddrBlock= blockStartAddr(req.addr,L1_STEP_VALUE);
    u_int32_t lastReqAddrBlock=blockStartAddr(this->nextReqAddr,L1_STEP_VALUE);
    int distance=lastReqAddrBlock-reqAddrBlock;
    //If the CPU didn't hit in L1 we have mispredicted
    if(!req.HitL1)
    {

	nextReqAddr = reqAddrBlock + L2_STEP_VALUE;
	ready = true;
	++count;
    }
    else if(distance>0 && ((distance/L2_STEP_VALUE)<MAX_L2_BLOCK_DIST))
    { //let the overflow deal with walkign off the end of memory
	nextReqAddr=lastReqAddrBlock+L2_STEP_VALUE;
	ready = true;
    }



}
