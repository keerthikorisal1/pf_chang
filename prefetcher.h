#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"
#include <cstdlib>
#include <cstring>

#define L2_BLOCK 32
#define L1_BLOCK 16

#define MAX_REQUESTS 40
#define STREAM_COUNT 8

#define NUM_RPT_ENTRIES 128 
#define STATE_SIZE 4096
#define NUM_REQ_PERMISS 3


struct RPT {
  u_int32_t pc;
  u_int32_t last_mem_access;
  u_int32_t stride;
};

class Prefetcher {
  private:
      bool _ready;
      Request _nextReq;
      int _req_left;
  
  public:
      Prefetcher();

      bool hasRequest(u_int32_t cycle);
      Request getRequest(u_int32_t cycle);
      void completeRequest(u_int32_t cycle);
      void cpuRequest(Request req);

};

#endif
