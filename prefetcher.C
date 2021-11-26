#include "prefetcher.h"
#include "mem-sim.h"
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <cmath>

using namespace std;

Prefetcher::Prefetcher()
{
  _policyBasePriorities.push_back(65);
  _policyBasePriorities.push_back(31);
  _policyBasePriorities.push_back(100);
  _policyBasePriorities.push_back(80);
}

bool Prefetcher::hasRequest(u_int32_t cycle)
{
  return !_reqQueue.empty();
}

Request Prefetcher::getRequest(u_int32_t cycle)
{
  return _reqQueue.back().first;
}

void Prefetcher::completeRequest(u_int32_t cycle)
{
  _recentRequests.push_back(_reqQueue.back());
  while(_recentRequests.size() > 1)
    {
      _recentRequests.erase(_recentRequests.begin());
    }
  _reqQueue.pop_back();
  if(_reqQueue.size() > 10)
    {
      _reqQueue.erase(_reqQueue.begin());
    }
  while(_reqQueue.size()>500)
  {
    _reqQueue.erase(_reqQueue.begin());
  }
}

vector<TReqPair> Prefetcher::GetOffsetRequests()
{
  vector<TReqPair> ret;
  Request tempReq;
  ReqPriority tempPriority;
  if (_offsets.size() > 2)
    {
	if (_offsets[_offsets.size()-1]==_offsets[_offsets.size()-2] &&
	    _offsets[_offsets.size()-2]==_offsets[_offsets.size()-3] &&
	    _offsets.back() != 0)
	  {
	    tempReq.addr=_arrivals.back().addr + _offsets.back();
	    tempPriority.priority=_policyBasePriorities[0];
	    tempPriority.policy=0;
	    ret.push_back(make_pair(tempReq, tempPriority));
	    tempReq.addr += _offsets.back();
	    tempPriority.priority=_policyBasePriorities[0];
	    tempPriority.policy=0;
	    ret.push_back(make_pair(tempReq, tempPriority));
	  }
	else
	  {
	    if (_offsets[_offsets.size()-1]==_offsets[_offsets.size()-2] &&
		_offsets.back() != 0)
	      {
		tempReq.addr=_arrivals.back().addr + _offsets.back();
		tempPriority.priority=_policyBasePriorities[0]+1;
		tempPriority.policy=0;
		ret.push_back(make_pair(tempReq, tempPriority));
	      }
	  }
      }
  return ret;
}

vector<TReqPair> Prefetcher::GetTemporalRequests()
{
  ReqPriority tempPriority;
  Request tempReq;
  u_int32_t max_count=0;
  u_int32_t address;
  for (map<u_int32_t,u_int32_t>::iterator it=_temporalMap.begin(); 
       it!=_temporalMap.end(); ++it)
    {
      if( it->second > max_count )
	{
	  max_count=it->second;
	  address=it->first;
	}
    }
  vector<TReqPair> ret;
  tempPriority.priority=_policyBasePriorities[1];
  tempPriority.policy=1;
  tempReq.addr=address;
  ret.push_back(make_pair(tempReq,tempPriority));
  return ret;
}

vector<TReqPair> Prefetcher::GetLookAheadRequests( Request req )
{
  vector<TReqPair> ret;
  Request tempReq;
  ReqPriority tempPriority;
  tempReq.addr = req.addr + 64;
  tempPriority.priority = _policyBasePriorities[2];
  tempPriority.policy=2;
  ret.push_back( make_pair(tempReq, tempPriority) );

  tempReq.addr += 16;
  tempPriority.priority = _policyBasePriorities[2]+1;
  tempPriority.policy=2;
  ret.push_back( make_pair(tempReq, tempPriority) );

  tempReq.addr += 16;
  tempPriority.priority = _policyBasePriorities[2]+1;
  tempPriority.policy=2;
  ret.push_back( make_pair(tempReq, tempPriority) );

  return ret;
}

std::vector<TReqPair> Prefetcher::GetPeriodicRequests()
{
  vector<TReqPair> ret;
  Request tempReq;
  ReqPriority tempPriority;
  bool redundant=0;
  for(unsigned int i=0; i<_periodicRequests.size(); ++i)
    {
      if(_periodicRequests[i]==_arrivals[_arrivals.size()-1].addr)
	{
	  redundant=1;
	  break;
	}
    }
  if(!redundant)
    {
      _periodicRequests.clear();
      if(_arrivals.size() > 3)
	{
	  for(unsigned int i=_arrivals.size()-3; i>floor(_arrivals.size()/2.0); --i)
	    {
	      if(_arrivals[i].addr == _arrivals[_arrivals.size()-1].addr &&
		 _arrivals[i].addr == _arrivals[2*i+1-_arrivals.size()].addr)
		{
		  for(unsigned int j=i+2; j<_arrivals.size()-1; j++)
		    {
		      tempReq.addr = _arrivals[j].addr;
		      tempPriority.priority = _policyBasePriorities[3]+_arrivals.size()-2-j;
		      tempPriority.policy=3;
		      ret.push_back( make_pair(tempReq, tempPriority) );
		      _periodicRequests.push_back(_arrivals[j].addr);
		    }
		  break;
		}
	    }
	}
    }
  return ret;
}


void Prefetcher::insertRequest( TReqPair newReq )
{
  if( _reqQueue.empty( ) )
    {
      _reqQueue.push_back(newReq);
      return;
    }
  bool found=0;
  for( size_t j=0; j<_reqQueue.size( ); ++j )
    {
      if( _reqQueue[j].first.addr==newReq.first.addr )
	{
	  _reqQueue[j].second.priority=floor((_reqQueue[j].second.priority+newReq.second.priority)/2.0);
	  _reqQueue[j].second.policy=newReq.second.policy;
	  sort(_reqQueue.begin(), _reqQueue.end(), ReqComp());
	  found=1;
	  break;
	}
    }
  if (!found)
    {
      for (vector<TReqPair>::iterator it = _reqQueue.end()-1;;--it)
	{
	  if (newReq.second.priority < it->second.priority)
	    {
	      ++it;
	      _reqQueue.insert(it, newReq);
	      break;
	    }
	  if (_reqQueue.begin() == it)
	    {
	      _reqQueue.insert(it, newReq);
	      break;
	    }
	}
    }
}

void Prefetcher::cpuRequest(Request req)
{
  if(req.HitL1)
    {
      for(unsigned int i=0; i<_recentRequests.size(); ++i)
	{
	  if(req.addr==_recentRequests[i].first.addr)
	    {
	      if(_policyBasePriorities[_recentRequests[i].second.policy]>2)
		{
		  _policyBasePriorities[_recentRequests[i].second.policy]-=2;
		}
	    }
	}
    }
  if (!_arrivals.empty())
    {
	if(_offsets.size() > 2 )
	  {
	    _offsets.erase(_offsets.begin());
	  }
	_offsets.push_back((req.addr & 0x1EE) - (_arrivals.back().addr & 0x1EE));
    }
  if (_arrivals.size( ) > 20 )
    {
      _arrivals.erase(_arrivals.begin());
    }
  if (_temporalMap.size() > 50 )
    {
      _temporalMap.clear();
    }
  _arrivals.push_back(req);

  if(!req.HitL1)
    {
      ++_temporalMap.insert(make_pair(req.addr,0)).first->second;
    }

  vector<TReqPair> requests = GetLookAheadRequests(req);	
  for( size_t i=0; i<requests.size( ); ++i )
    {
      insertRequest( requests[i] );
    }
  requests = GetOffsetRequests();
  for( size_t i=0; i<requests.size( ); ++i )
    {
      insertRequest( requests[i] );
    }
	
  if( _temporalMap.size()%50 == 0 )
    {
      requests = GetTemporalRequests();
      for( size_t i=0; i<requests.size( ); ++i )
	{
	  insertRequest( requests[i] );
	}
    }
  requests = GetPeriodicRequests();
  for( size_t i=0; i<requests.size( ); ++i )
    {
      insertRequest( requests[i] );
    }
}
