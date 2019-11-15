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
extent_client_cache::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret = extent_protocol::OK;
  /*
  assert(cache.find(id) == cache.end());

  file_cache* file = new file_cache();
  file->cached_attr = true;
  file->a.type = type;
  file->a.size = 0;
  cache[id] = file;*/
  return ret;
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
  
  // The file attr is not in the cache
  char *in_cache = (char*)malloc(content.size());
  memcpy(in_cache,content.c_str(),content.size());
  file_content *cache_content = new file_content(content.size());
  cache_content->content = in_cache;
  file_cache[eid] = cache_content;

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


  file_content* in_cache = file_cache[eid];
  //deep copy
  
  char* rett_data = (char *)malloc(in_cache->size);
  memcpy(rett_data,in_cache->content,in_cache->size);
  
  std::string rett;
  rett.assign(rett_data);

  //debug
  //rett[0] = '6';
  /*
  if(rett == file_cache[eid]){
    printf("rett:%s, file_cache[eid]:%s\n",rett.data(),file_cache[eid].data());
    printf("rett addr:%lx,%lx, file_cache[eid] addr:%lx,%lx\n",(unsigned long)rett.c_str(),(unsigned long)rett.data(),(unsigned long)file_cache[eid].c_str(),(unsigned long)file_cache[eid].data());
    assert(0);
  }
  */
  //assert(rett != file_cache[eid]);
  //end debug

  return rett;
}


extent_protocol::status
extent_client_cache::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;

  attr_cache.erase(eid);
  free(file_cache[eid]->content);
  free(file_cache[eid]);
  file_cache.erase(eid);  
  return ret;
}


