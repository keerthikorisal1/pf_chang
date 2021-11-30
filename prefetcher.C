#include "prefetcher.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

/*defines number of rpt_table entries*/ 
static rpt_row_entries rpt_table[NUM_RPT_ENTRIES];

Prefetcher::Prefetcher(){
    int i;
    _ready = false;
    /*clear RPT table*/
    for(i = 0; i < NUM_RPT_ENTRIES; i++){
        rpt_table[i].pc = 0;
        rpt_table[i].stride = 0;
        rpt_table[i].prev_addr = 0;
    }
}

bool Prefetcher::hasRequest(u_int32_t cycle){ return _ready; }

Request Prefetcher::getRequest(u_int32_t cycle) { return _nextReq; }

void Prefetcher::completeRequest(u_int32_t cycle) {
    int rpt_row, current_stride;
    rpt_row_entries *current_rpt_row;
    /*check for remaining request, if not check if next request is in RPT, if not fetch next request+32*/
    if(_req_left == 0){
        _ready = false;
    }
    else{
        _req_left--;
        rpt_row = _nextReq.pc % NUM_RPT_ENTRIES;
        std::cout << "rpt_row: " << rpt_row << "\n";
        std::cout << "_nextReq.PC: " << _nextReq.pc << "\n";
        current_rpt_row = &rpt_table[rpt_row];
        std::cout << "current_rpt_row: " << current_rpt_row << "\n";
        std::cout << "current_rpt_row_PC: " << current_rpt_row->PC << "\n";
        std::cout << "_nextReq.PC: " << _nextReq.pc << "\n";
        /* check if PC exists, if yee, fetch next_req if not fetch _nextReq+32*/
        if(current_rpt_row->pc == _nextReq.pc){
            _nextReq.addr = _nextReq.addr + current_rpt_row->stride;
            std::cout << "current_rpt_row_stride: " << current_rpt_row->stride << "\n";
        }
        else{
            _nextReq.addr = _nextReq.addr + L2_BLOCK;
        }
    }
}

void Prefetcher::cpuRequest(Request req){
    int rpt_row, current_stride;
    rpt_row_entries *current_rpt_row;

    if(req.HitL1 && !_ready){
        rpt_row = req.pc % NUM_RPT_ENTRIES;
        current_rpt_row = &rpt_table[rpt_row];
        if(current_rpt_row->pc == req.pc){
            current_stride = current_rpt_row->stride;
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
        current_rpt_row = &rpt_table[rpt_row];
        if(current_rpt_row->pc == req.pc){
            if((current_stride = req.addr - (current_rpt_row->prev_addr)) == current_rpt_row->stride){
                _nextReq.addr = req.addr + current_stride;
            }
            else{
                current_rpt_row->stride = current_stride;
                _nextReq.addr = req.addr + L2_BLOCK;
            }
        }
        else{
            _nextReq.addr = req.addr + L2_BLOCK;
            current_rpt_row->stride = 0;
        }
        current_rpt_row->pc = req.pc;
        current_rpt_row->prev_addr = req.addr;
        _ready = true;
        _req_left = NUM_REQ_PER_MISS - 1;
    }
    
}
