#include "prefetcher.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

static char tags[STATE_SIZE];
static rpt_row_entries rpt_table[NUM_RPT_ENTRIES];

Prefetcher::Prefetcher(){
    int i;
    _ready = false;
    memset(tags, 0, STATE_SIZE);
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
    rpt_row_entries *current_rtp_row;

    if(_req_left == 0){
        _ready = false;
    }
    else{
        _req_left--;
        rpt_row = _nextReq.pc % NUM_RPT_ENTRIES;
        current_rpt_row = &rpt_table[rpt_row];
        if(current_rpt_row->pc == _nextReq.pc){
            std::cout << "called cpmplete req && in RPT"
            _nextReq.addr = _nextReq.addr + current_rpt_row->stride;
        }
        else{
            std::cout << "called cpmplete req && not in RPT"
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
            /*std::cout << "current PC: %s :" << req.pc << "\n";
            printStruct(current_row);*/
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
                /*std::cout << "current stride: %s\n" << current_stride << "\n";
                printStruct(current_row);*/
                _nextReq.addr = req.addr + current_stride;
            }
            else{
                current_rpt_row->stride = current_stride;
                /*std::cout << "current stride: %s\n" << current_stride << "\n";
                printStruct(*current_row);*/
                _nextReq.addr = req.addr + L2_BLOCK;
            }
        }
        else{
            _nextReq.addr = req.addr + L2_BLOCK;
            current_rpt_row->stride = 0;
        }
        current_rpt_row->pc = req.pc;
        current_rpt_row->prev_addr = req.addr;
        /*std::cout << "last_addr: %s\n" << req.addr << "\n";
        std::cout << "current PC: %s\n" << req.pc << "\n";
        printStruct(current_row);*/
        _ready = true;
        _req_left = NUM_REQ_PER_MISS - 1;
    }
    
}

void Prefetcher::printStruct(rpt_row_entries *current_row){
    std::cout << "PC: \n" << current_row->pc << "\n";
    std::cout << "Prev_Add: \n" << current_row->prev_addr_access << "\n";
    std::cout << "Current Stride: \n" << current_row->stride << "\n";
}
