// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include "extent_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  cacheManager = new extent_client_cache();
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
  /*
  int last_port = 777;
  srand(time(NULL) ^ last_port);
  rextent_port = ((rand() % 32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rextent_port;
  id = host.str();
  last_port = rextent_port;
  rpcs *rlsrpc = new rpcs(rextent_port);
  rlsrpc->reg(rextent_protocol::flush, this, &extent_client::flush);
  */
}

// a demo to show how to use RPC
extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab2 part1 code goes here
  ret = cl->call(extent_protocol::create,type,id);
  //cacheManager->create(type,id);  

  return ret;
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;

  
  if(cacheManager->cached_content(eid) == false){
    ret = cl->call(extent_protocol::get,eid,buf);  
    cacheManager->updatefile(eid,buf);
  } else{
    buf = cacheManager->getfile(eid);
  }

  extent_protocol::attr attr;
  if(cacheManager->cached_attr(eid) == false){
    ret = cl->call(extent_protocol::getattr, eid,attr);
    printf("debug get: get attr from server and get type: %d\n",attr.type);
  }
  else{
    attr = cacheManager->getattr(eid);
  }
  attr.atime = time(0);
  cacheManager->updateattr(eid,attr);
  
  printf("debug get:The file content of %lld is %s, attr type:%d ( file type:%d)\n",eid,buf.c_str(),attr.type, extent_protocol::T_FILE);
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;

  if(cacheManager->cached_attr(eid) == false){
    ret = cl->call(extent_protocol::getattr, eid, attr);
    printf("debug getattr: get %llu's attr from server and get type: %d\n",eid,attr.type);
    cacheManager->updateattr(eid,attr);
  }
  else{
    attr = cacheManager->getattr(eid);
  }
  return ret;
}

//In lab4, expect for initiating the root dir and symlink, 'put' is a cache operation.
extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  
  extent_protocol::attr attr;
  //Because every 'put' operation must behind a corresponding 'get' operation, the content must reside in the cache.
  cacheManager->updatefile(eid,buf);
  
  if(cacheManager->cached_attr(eid) == false){
    ret = cl->call(extent_protocol::getattr,eid,attr);
    
    printf("debug put: get attr from server and get type :%d\n",attr.type);
    
  } 
  else{
    attr = cacheManager->getattr(eid);
  }
  attr.mtime = time(0);
  attr.ctime = time(0);
  cacheManager->updateattr(eid,attr);
  
  
  printf("debug put:The file content of %llu is %s, size %lu,attr.type:%d\n",eid,buf.c_str(),buf.size(),attr.type);
  
  //debug
  std::string imm;
  imm = cacheManager->getfile(eid);
  printf("debug put:get out the buf immediately, buf:%s, buf size:%lu\n",imm.c_str(),imm.size());
  //debug end
  return ret;
}


extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  int r;
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab2 part1 code goes here
  ret = cl->call(extent_protocol::remove,eid,r);
  cacheManager->remove(eid);

  return ret;
}

/*
rextent_protocol::status 
extent_client::flush(extent_protocol::extentid_t eid,int &){
  rextent_protocol::status ret = rextent_protocol::OK;
  std::string buf = cacheManager->getfile(eid);
  int r;
  cl->call(extent_protocol::put,eid,buf,r);
  cacheManager->clear_flush(eid);
  return ret;
}
*/