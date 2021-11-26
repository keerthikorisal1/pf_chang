#ifndef PREFETCHER_H
#define PREFETCHER_H

#include <sys/types.h>
#include "mem-sim.h"
#include <iostream>
#include <queue>
#include <map>
#include <set>
#include <list>
#include <assert.h>
#include <limits.h>
using namespace std;

#define BUFFER_SIZE 96
#define INDEX_SIZE 32

class GlobalHistory
{
private:
	// Typedef types for clarity
	typedef u_int64_t						Address;
	typedef int DiffAddr;
	typedef int Index;

	// IPAir is a pair in Index Table
	typedef pair<
		pair<DiffAddr, DiffAddr>, Index>	IPair;

	// HPair is a pair in Global History Buffer table
	typedef pair<Address, DiffAddr>			HPair; 
	typedef list<HPair>::iterator			HistoryIt;
	
	// Required to compute program counter offsets
	Address									_PC1;	// Last
	Address									_PC2;	// One before last

	// All saved in an iteratable loops
	list<HPair>								_historyTable;
	list<IPair>								_indexTable;

	// Lower bound for buffer
	Index									_sizeLimit;

public:

	// Auxilary internal indexes for width and depth settings
	static short index1, index2;

private:

	// Find pairs of PCs and remove them
	Index ListFindAndRemove(pair<DiffAddr, DiffAddr>& thePair)
	{
		list<IPair>::iterator myIter = _indexTable.begin();

		for (; myIter != _indexTable.end(); ++myIter)
		{
			if (myIter->first == thePair)
			{
				Index idx = myIter->second;
				_indexTable.erase(myIter);

				return idx;
			}
		}

		return -1;
	}

	// Add new pairs of PCs mapped to an index in buffer
	void ListAddLimited(IPair toPush)
	{
		_indexTable.push_back(toPush);

		if (_indexTable.size() > INDEX_SIZE)
			_indexTable.pop_front();
	}

public:
	GlobalHistory() : _PC1(ULONG_MAX), _PC2(ULONG_MAX), _sizeLimit(0) {}

	// Auxilary print function
	void PrintStacks()
	{
		cout << " ======================== START (index) ===========================\n";
		int loc = 0;
		for (list<IPair>::iterator i = _indexTable.begin(); i != _indexTable.end(); ++i)
		{
			cout << "Diff " << loc++ << ": (" << 
				i->first.first << ", " << i->first.second << ") to index: " << i->second << endl;
		}
		cout << " ---------(history) ------ " << endl;
		loc = 0;
		for (HistoryIt it = _historyTable.begin(); it != _historyTable.end(); ++it)
		{
			cout << "Address " << loc++ << ": " << it->first << " with index: " << it->second << endl;
		}
		cout << "\n\n\n";
	}

	void AddMiss(Address PC, Address address, queue<u_int32_t>& prefetch, bool saveFetches)
	{
		// If first time, we cannot have a diff
		if (_PC1 == ULONG_MAX)
		{
			_PC1 = PC;
			return;
		}

		if (_PC2 == ULONG_MAX)
		{
			_PC2 = _PC1;
			_PC1 = PC;
			return;
		}

		// Compute current diff
		DiffAddr diffPC1 = DiffAddr(PC) - DiffAddr(_PC1);
		DiffAddr diffPC2 = DiffAddr(_PC1) - DiffAddr(_PC2);
		pair<DiffAddr, DiffAddr> DiffPair = pair<DiffAddr, DiffAddr>(diffPC1, diffPC2);

		// Forward for next time
		_PC2 = _PC1;
		_PC1 = PC;

		Index stepBack = INT_MAX;
		Index index;

		// If difference does exist in IndexTable
		if ((index = ListFindAndRemove(DiffPair)) >= 0/* && saveFetches*/)
		{
			// If does exist
			short limitDepth = index1;						// Depth limitation (how many backtracks)
			short limitWidth = index2;						// Width limitation (how many lookaheads)

			if (index < _sizeLimit)
			{
				goto HERE;
			}

			int limitation = _historyTable.size();			// limitation of width search
			stepBack = limitation - index;

			HPair hPair(address, stepBack);
			_historyTable.push_back(hPair);
			ListAddLimited(IPair(DiffPair,  _historyTable.size() - 1));

			if (_historyTable.size() > BUFFER_SIZE)
			{
				_historyTable.pop_front();
				++_sizeLimit;
			}

			DiffAddr diffIt = stepBack;

			while(index >= 0 && diffIt >= 1 && limitDepth-- > 0)
			{
				DiffAddr myDiff = 0;

				HistoryIt historyHit = _historyTable.begin();
				std::advance(historyHit, index - _sizeLimit);

				HPair refPair = *historyHit; 
				HPair orgPair = refPair;

				for (int i=1; i<=limitWidth && index+i <= limitation; ++i)
				{
					std::advance(historyHit, 1);
					HPair hPair = *historyHit;
					myDiff += (DiffAddr(hPair.first) - DiffAddr(refPair.first));

					prefetch.push(Address(DiffAddr(address) + myDiff));
					refPair = hPair;
				}

				limitation = index;
				diffIt = orgPair.second;
				index -= diffIt;
			} 

			return;
		}
		
HERE:
		HPair hPair(address, stepBack);
		_historyTable.push_back(hPair);
		ListAddLimited(IPair(DiffPair,  _historyTable.size() - 1));

		if (_historyTable.size() > BUFFER_SIZE)
		{
			_historyTable.pop_front();
			++_sizeLimit;
		}
		return;
	}
};


class Prefetcher {
  private:
	GlobalHistory _globalHistoryLoads;
	GlobalHistory _globalHistoryStores;

	
	queue<u_int32_t> _fetchQueue;

	long _cFetch;
	long _cReqs;

	long _address_load_diff;
	long _address_store_diff;

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
