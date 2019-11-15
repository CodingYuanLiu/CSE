// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"
#include "extent_client_cache.h"

class extent_client {
 private:
  rpcc *cl;
  
  int rextent_port;
  std::string hostname;
  std::string id;
  
  extent_client_cache *cacheManager;
 public:
  extent_client(std::string dst);

  //rextent_protocol::status flush(extent_protocol::extentid_t ,int &);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  rextent_protocol::status flush(extent_protocol::extentid_t eid, int &);
};

#endif 

