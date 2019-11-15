// extent client interface.

#ifndef extent_client_cache_h
#define extent_client_cache_h

#include <string>
#include "extent_protocol.h"
#include "extent_server.h"

class extent_client_cache {
 private:
  rpcc *cl;
  
  //因为现在file和attr分开cache了所以下面的数据结构就没用了。
  /*
  struct file_cache{
    //If the file is newly created (and so that it's attr has no time information), set it to true.
    bool cached_attr;

    //Inode information
    extent_protocol::attr a;

    //file content
    std::string data;
    file_cache(){
      cached_attr=false;
    }
  };
  struct attr_cache{
    extent_protocol::attr a;
  };
  */
  struct file_content{
    char* content;
    size_t size;
    file_content(size_t s){
      size = s;
    }
  };


  std::map<extent_protocol::extentid_t, std::string> file_cache;
  std::map<extent_protocol::extentid_t, extent_protocol::attr> attr_cache;

 public:

  extent_client_cache();
  
  //inode number to file content
  

  bool cached_content(extent_protocol::extentid_t id);
  bool cached_attr(extent_protocol::extentid_t id);

  extent_protocol::status updateattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr a);
  
  extent_protocol::status updatefile(extent_protocol::extentid_t eid,
                                  std::string content);


  extent_protocol::attr getattr(extent_protocol::extentid_t eid);
  std::string getfile(extent_protocol::extentid_t eid);
  
  extent_protocol::status remove(extent_protocol::extentid_t eid);

  extent_protocol::status clear_flush(extent_protocol::extentid_t eid);
};

#endif 

