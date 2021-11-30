#include "prefetcher.h"
#include <stdio.h>

static char tags[STATE_SIZE];
/*static char rpt_check[NUM_RPT_ENTRIES/8 +1]; */
static rpt_row_entries rpt_table[NUM_RPT_ENTRIES];

Prefetcher::Prefetcher(){
    int i;
    _ready = false;
    memset(tags, 0, STATE_SIZE);
    for(i = 0; i < NUM_RPT_ENTRIES; i++){
        rpt_table[i].pc = 0;
        rpt_table[i].stride = 0;
        rpt_table[i].last_mem_access = 0;
    }
}

bool Prefetcher::hasRequest(u_int32_t cycle){ return _ready; }

Request Prefetcher::getRequest(u_int32_t cycle) { return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) {}

void Prefetcher::cpuRequest(Request req){
    int rpt_row, current_stride;
    rpt_row_entries *current_row;

    if(req.HitL1 && !_ready){
        rpt_row = req.pc % NUM_RPT_ENTRIES;
        current_row = &rpt_table[rpt_row];
        if(current_row->pc == req.pc){
            current_stride = current_row->stride;
            _nextReq.addr = req.addr + current_stride;
        }
        else{
            _nextReq.addr = req.addr + L2_BLOCK;
        }
        _ready = true;
        _req_left = NUM_REQ_PER_MISS - 1;
    }
    else if (!req.HitL1) {
        rpt_row = req.pc % NUM_RPT_ENTRIES;
        current_row = &rpt_table[rpt_row];
        if(current_row->pc == req.pc){
            if((current_stride = req.addr - (current_row->last_mem)) == current_row->stride){
                _nextReq.addr = req.addr + current_stride;
            }
            else{
                current_row->stride = current_stride;
                _nextReq.addr = req.addr + L2_BLOCK;
            }
        }
        else{
            _nextReq.addr = req.addr + L2_BLOCK;
            current_row.stride = 0;
        }
        current_row->pc = req.pc;
        current_row->last_mem_access = req.addr;
        _ready = true;
        _req_left = NUM_REQ_PER_MISS - 1;
    }
    
}
