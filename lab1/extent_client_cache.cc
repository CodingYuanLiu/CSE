// RPC stubs for clients to talk to extent_server

#include "extent_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_client_cache::extent_client_cache()
{

}

extent_protocol::status
extent_client_cache::updateattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  
  // The file attr is not in the cache
  printf("update %llu's attr, it's type is %d\n",eid,attr.type);
  extent_protocol::attr cachedattr;
  cachedattr.type = attr.type;
  cachedattr.size = attr.size;
  cachedattr.atime = attr.atime;
  cachedattr.ctime = attr.ctime;
  cachedattr.mtime = attr.mtime;
  if(attr_cache.find(eid) == attr_cache.end()){
    attr_cache.insert(std::pair<extent_protocol::extentid_t,extent_protocol::attr>(eid,attr));
  }
  else{
    attr_cache[eid] = cachedattr;
  }

  //debug
  printf("Get the attr out immediately,it's id and type are:%llu,%d\n",eid,attr_cache[eid].type);
  //end debug

  return ret;
}

extent_protocol::status
extent_client_cache::updatefile(extent_protocol::extentid_t eid, 
		       std::string content)
{
  extent_protocol::status ret = extent_protocol::OK;

  file_cache[eid] = content;

  return ret;
}

bool extent_client_cache::cached_content(extent_protocol::extentid_t eid){
  if(file_cache.find(eid) == file_cache.end()){
    return false;
  }
  return true;
}

bool extent_client_cache::cached_attr(extent_protocol::extentid_t eid){
  if(attr_cache.find(eid) == attr_cache.end()){
    return false;
  }
  return true;
}

extent_protocol::attr
extent_client_cache::getattr(extent_protocol::extentid_t eid)
{
  extent_protocol::attr attr;
  assert(attr_cache.find(eid) != attr_cache.end());
  attr = attr_cache[eid];
  return attr;
}

std::string
extent_client_cache::getfile(extent_protocol::extentid_t eid)
{
  assert(file_cache.find(eid) != file_cache.end());

  std::string rett;
  rett = file_cache[eid];

  return rett;
}


extent_protocol::status
extent_client_cache::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;

  attr_cache.erase(eid);
  file_cache.erase(eid);  
  return ret;
}


extent_protocol::status
extent_client_cache::clear_flush(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  attr_cache.erase(eid);
  file_cache.erase(eid);
  return ret;
}