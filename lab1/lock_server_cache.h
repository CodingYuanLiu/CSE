#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>


#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"
#include <queue>


class lock_server_cache {
 private:
  int nacquire;
  enum xxstatus { 
    //One client got it
    LOCKED,
    /*
      A client is acquiring the lock and the server is trying to revoke the lock.
      The revoking operation of clients is asynchronized.
    */
    REVOKING,
    /*
      The pre-owner of the lock have revoked and released the lock successfully, 
      so that the server is responsible for calling a new owner (at the front of 
      queue) to retry.
    */
    RETRYING
  };
  struct srv_lockattr{
    std::string owner;
    int state;
    std::queue<std::string> waiting_clients;
  };

  //The ownership of each lock, id == "" for no one owns the lock(but may never happens)
  std::map<lock_protocol::lockid_t,srv_lockattr*> lock_ownership;
  //The callers to call the revoke and retry function of each client
  std::map<std::string,rpcc*> client_callers;

  pthread_mutex_t lock;
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
