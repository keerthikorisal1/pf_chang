#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"
#include <queue>
#include <utility>
#include <vector>
#include <map>

struct ReqPriority
{
	char priority;
	u_int32_t policy;
};

typedef std::pair<Request, ReqPriority> TReqPair;

class ReqComp
{
	public:
	bool operator() (const TReqPair& lhs, const TReqPair& rhs) const
	{
		return (lhs.second.priority > rhs.second.priority);
	}
};

class Prefetcher
{
  private:
        // total state saved: 2625+200+5.25+80+40+4+4 = 2958.25 bytes

        // 500 max elements * pair(4 bytes addr, [1 byte priority, 2 bit policy])
	std::vector<TReqPair> _reqQueue;		// (4+1.25)*500=2625

        // 50 max elements * 4 byte address counts
	std::map<u_int32_t,u_int32_t> _temporalMap;	// 4*50=200

        // 1 max element * pair(4 bytes addr, [1 byte priority, 2 bit policy])
	std::vector<TReqPair> _recentRequests;		// 5.25
	
	// 20 max elements * 4 byte address
        std::vector<Request> _arrivals;			// 4*20=80

        // 10 max elements * 4 byte address
	std::vector<u_int32_t> _periodicRequests;	// 4*10=40

        // 2 max elements * 2 byte offsets
	std::vector<short> _offsets;			// 2*2=4

        // 4 elements * 1 byte priority
	std::vector<char> _policyBasePriorities;	// 4

	std::vector<TReqPair> GetOffsetRequests();
	std::vector<TReqPair> GetTemporalRequests();
	std::vector<TReqPair> GetLookAheadRequests(Request req);
	std::vector<TReqPair> GetPeriodicRequests();
	std::vector<ReqPriority> GetPriorities();

	void insertRequest( TReqPair newReq );

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
