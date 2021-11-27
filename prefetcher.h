#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"

#define L2_BLOCK 32
#define L1_BLOCK 16

#define MAX_REQUESTS 40
#define STREAM_COUNT 8

#define NUM_RPT_ENTRIES 128 
#define REQUEST_CUTOFF 20
#define DEFAULT_STREAMSIZE 2

struct SLH {
	u_int32_t pc;	     //PC of stream
	u_int32_t addr;      //Last Address Accessed
	u_int16_t length;    //Length of stream
	u_int32_t lifetime;  //Lifetime of stream
};

struct RPT {
  u_int32_t pc;
  u_int32_t prev_addr;
  u_int32_t stride;
  int32_t state;
};

class Prefetcher {
  private:
      bool _ready;
      Request requests[MAX_REQUESTS];
      SLH SLH_TABLE[STREAM_COUNT];

      u_int16_t live_streams;
      u_int16_t stream_buff[STREAM_COUNT];
      u_int32_t requestQueue[MAX_REQUESTS];
      u_int16_t num_req;

      void streamReset(u_int16_t index);
      u_int16_t inStream(u_int32_t addr);
      void addRequest(u_int32_t addr);

      int num_rpt;
      int oldest_rpt;
      int current_pending_request;
      RPT rpt[NUM_RPT_ENTRIES];
  
  public:
      Prefetcher();

      bool hasRequest(u_int32_t cycle);
      Request getRequest(u_int32_t cycle);
      void completeRequest(u_int32_t cycle);
      void cpuRequest(Request req);
      bool ifAlreadyInQueue(u_int32_t addr); 

};

#endif
