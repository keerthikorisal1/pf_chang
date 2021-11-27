#include "prefetcher.h"
#include <stdio.h>
#include <stdlib.h>

Prefetcher::Prefetcher(){
    rpt_entries = 0;
    oldest_rpt = 0;

    u_int16_t stream_buff[STREAM_COUNT] = {0,0,0,0,0,0,0,0};
    for(u_int16_t i = 0; i < STREAM_COUNT; i++){
        streamReset(i);
    }
    live_streams = 0;
    for(u_int16_t i = 0; i < MAX_REQUESTS; i++){
        requestQueue[i] = 0;
    }
    num_req = 0;
}

bool Prefetcher::hasRequest(u_int32_t cycle){
    if (num_req > 0) return true;
    else return false;
}

Request Prefetcher::getRequest(u_int32_t cycle){
    num_req--;
    _nextReq.addr = requestQueue[0];

    for(u_int16_t i = 0; i < MAX_REQUESTS; i++){
        if(i < num_req){
            requestQueue[i] = requestQueue[i+1];
        }
        else{
            requestQueue[i] = 0;
        }
    }
    return _nextReq;
}

void Prefetcher::completeRequest(u_int32_t cycle){}

void Prefetcher::cpuRequest(Request req){
    // lifetime calc for SLH table for locatily stream buffer
    for(u_int16_t i = 0; i < STREAM_COUNT; i++){
        if(stream_buff[i] == 1){
            SLH_TABLE[i].lifetime -= 1;
        }
    }
    // rm lifetime 0
    for(u_int16_t i = 0; i < STREAM_COUNT; i++){
        if(stream_buf[i] == 1 && SLH_TABLE[i].lifetime == 0){
            streamRest(i);
        }
    }
    // stream init + index
    u_int16_t streamIndex = inStream(req.addr);

    //RPT inti + set tag index
    int RPT_index = -1;
    for(int i = 0; i < NUM_RPT_ENTRIES; i++){
        if(rpt[i].pc == req.pc){
            RPT_index = i;
            break;
        }
    }
    if(RPT_index != 1){
        u_int32_t temp_stride = rpt[RPT_index].stride;
        rpt[RPT_index].stride = req.addr - rpt[RPT_index].prev_addr;
        rpt[RPT_index].prev_addr = req.addr;
        if(temp_stride == rpt[RPT_index].stride){
            rpt[RPT_index].strate++;
            if(rpt[RPT_index].state > 2){
                if(rpt[RPT_index].state >= REQUEST_CUTOFF && req.HitL1 == true){
                    // if request is part of existing stream buffer
                    if(req.HitL1 && streamIndex == STREAM_COUNT){
                        if(live_streams < STREAM_COUNT){
                            for(u_int16_t i = 0; i < STREAM_COUNT; i++){
                                if(stream_buff[i] == 0){
                                    SLH_TABLE[i].pc = req.pc;
                                    SLH_TABLE[i].addr = req.addr;
                                    SLH_TABLE[i].length = 1;
                                    SLH_TABLE[i].lifetime = 16;
                                    stream_buff[i] = 1;
                                    live_streams++;
                                    addRequest(SLH_TABLE[i].addr + L1_BLOCK);
                                    break;
                                }
                            }
                        }
                        else{
                            u_int16_t min_lifetime = SLH_TABLE[0].lifetime;
                            u_int16_t LRU_index = 0;
                            for(u_int16_t i = 0; i < STREAM_COUNT; i++){
                                if(SLH_TABLE[i].lifetime < min_lifetime){
                                    min_lifetime = SLH_TABLE[i].lifetime;
                                    LRU_index = i;
                                }
                                else if(SLH_TABLE[i].lifetime == min_lifetime){
                                    if(SLH_TABLE[i].length < SLH_TABLE[LRU_index].length){
                                        LRU_INDEX = i;
                                    }
                                }
                            }
                            streamRest(LRU_index);
                            SLH_TABLE[LRU_index].pc = req.pc;
                            SLH_TABLE[LRU_index].addr = req.addr;
                            SLH_TABLE[LRU_index].length = 1;
                            SLH_TABLE[LRU_index].lifetime = 16;
                            stream_buff[LRU_index] = 1;
                            live_streams++;
                            addRequest(SLH_TABLE[LRU_index].addr + L1_BLOCK);
                        }
                    }
                    // if request is part of existing stream buffer
                    else(req.HitL1 && streamIndex != STREAM_COUNT){
                        SLH_TABLE[streamIndex].pc = req.pc;
                        SLH_TABLE[streamIndex].addr = req.addr;
                        SLH_TABLE[streamIndex].length += 1;
                        SLH_TABLE[streamIndex].lifetime += 1;
                        addRequest(SLH_TABLE[streamIndex].addr + L1_BLOCK);
                    }
                }
                else{
                    if(rpt[RPT_index].stride <= 32){
                        for (int n = 1; n <= rpt[RPT_index].state && n <= REQUEST_CUTOFF; n++){
                            u_int32_t temp_addr = req.addr + 32 * n;
                            addRequest(temp_addr);
                        }
                    }
                    else{
                        for(int n = 1; n <= rpt[RPT_index].state && n <= REQUEST_CUTOFF; n++){
                            u_int32_t temp_addr = req.addr + rpt[index].stride * n;
                            addRequest(temp_addr);  
                        }
                    }
                }
            }
        }
        else{
            for (int i = 0; i < DEFAULT_STREAMSIZE; ++i){
                addRequest(req.addr + 32 * (i+1));
            }
        }
    }
    else{
        rpt[oldest_rpt].pc = req.pc;
        rpt[oldest_rpt].prev_addr = req.addr;
        rpt[oldest_rpt].state = 0;
        rpt[oldest_rpt].stride = 4;
        oldest_rpt = (oldest_rpt + 1) % NUM_RPT_ENTRIES;
        for (int i = 0; i < DEFAULT_STREAMSIZE; ++i){
            addRequest(req.addr + 32 * (i+1));
        }
    }
}

void Prefetcher::streamReset(u_int16_t index){
	SLH_TABLE[index].pc = 0;
	SLH_TABLE[index].addr = 0;
	SLH_TABLE[index].length = 0;
	SLH_TABLE[index].lifetime = 0;
	stream_buff[index] = 0;
	live_streams--;
}

u_int16_t Prefetcher::inStream(u_int32_t addr){
	for(u_int16_t i = 0; i < STREAM_COUNT; i++){
		if(stream_buff[i] == 1){
			if(abs(SLH_TABLE[i].addr - addr) <= L2_BLOCK){
				return i;
			}
		}
	}
	// 0-7 is valid, thus 8 is not
	return STREAM_COUNT;
}

void Prefetcher::addReqest(u_int32_t addr){
    if(num_req != MAX_REQUESTS){
        requestQueue[num_req] = addr;
        num_req++;
    }
}
