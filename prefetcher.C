#include "prefetcher.h"
#include "mem-sim.h"
#include <stdio.h>

Prefetcher::Prefetcher() {
  for (int i = 0; i < N; ++i) {
    _readys[i] = false;
  }

  for (int i = 0; i < STORE; ++i) {
    last_addrs[i] = 0;
  }

  _strider_ready = false;
  step = -1;
  prediction = 0;
  fifo = 0;
  // _ready = false;
}

bool Prefetcher::hasRequest(u_int32_t cycle) {
  return hasChunkRequest() | hasStrideRequest();
}

Request Prefetcher::getRequest(u_int32_t cycle) {
  if (hasChunkRequest()) {
    Request r = _nextReq;
    for (int i = 0; i < N; ++i) {
      r.addr += 16;
      if(_readys[i]) {
        break;
      }
    }
  	return r;
  }
  return _striderReq;
  
  // _nextReq.addr += 16;
  // return _nextReq;
}

void Prefetcher::completeRequest(u_int32_t cycle) { 
  for (int i = 0; i < N; ++i) {
    if(_readys[i]) {
      _readys[i] = false;
      return;
    }
  }
  _strider_ready = false;
}

void Prefetcher::cpuRequest(Request req) {
  last_addrs[fifo] = req.addr;

  printf("%u\t%i\n", req.addr, req.HitL1);
  bool ready = !req.HitL1;
  for (int i = 0; i < N; ++i) {
    ready &= !_readys[i];
  }

  if(ready) {
    _nextReq.addr = req.addr;
    for (int i = 0; i < N; ++i) {
      _readys[i] = true;
    }
    // _ready = true;
  } else {
    findPattern();
    if (prediction) {
      _strider_ready = true;
      _striderReq.addr = last_addrs[getIdx(step)] + prediction;
      // printf("\tMaking prediction to %u of step size %d\n", (u_int32_t)(last_addrs[getIdx(step)] + prediction), prediction);
    }
  }
  fifo = (fifo+1)%STORE;
}

void Prefetcher::findPattern() {
  long long diff1 = 0;
  long long diff2 = 0;
  step = -1;
  prediction = 0;
  for (int i = 0; i < (STORE/3)-1; ++i) {
    diff1 = last_addrs[getIdx(i)] - last_addrs[getIdx(2*i + 1)];
    diff2 = last_addrs[getIdx(2*i + 1)] - last_addrs[getIdx(3*i + 2)];
    if(diff1 == diff2 && 
      diff1 != 0) {
      prediction = diff1;
      step = i;
    }
  }

  // if(step != -1) printf("\t\t%i\n", step);
}

short Prefetcher::getIdx(short addr) {
  return (fifo + STORE - addr)%STORE;
}

bool Prefetcher::hasChunkRequest() {
  bool all = false;
  for (int i = 0; i < N; ++i) {
    all |= _readys[i];
  }
  return all;
}

bool Prefetcher::hasStrideRequest() {
  return _strider_ready;
}
