// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

lock_server_cache::lock_server_cache()
{
  pthread_mutex_init(&lock, NULL);
}

int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id,
                               int &)
{
  int r;//RPC NULL receiver
  lock_protocol::status ret = lock_protocol::OK;
  pthread_mutex_lock(&lock);
  //The first time that a client meets the server
  if (client_callers.find(id) == client_callers.end())
  {
    handle h(id);
    rpcc *cl = h.safebind();
    if (!cl)
    {
      printf("error: Fail to safebind client %s\n", id.c_str());
      exit(0);
    }
    client_callers[id] = cl;
  }
  //The lock is aquired for the first time.
  if (lock_ownership.find(lid) == lock_ownership.end())
  {
    srv_lockattr *newLock = new srv_lockattr;
    newLock->owner = id;
    newLock->state = LOCKED;
    assert(newLock->waiting_clients.empty());
    lock_ownership[lid] = newLock;
    pthread_mutex_unlock(&lock);
    return ret;
  }

  srv_lockattr *lockattr = lock_ownership[lid];
  //If the client already got the lock, return an Error
  if(lockattr->owner.compare(id) == 0){
    printf("error: double acquire of client %s\n",id.c_str());
    assert(0);
  }
  assert(client_callers.find(id) != client_callers.end());
  /*
      Try to revoke the lock_owner's lock. If the owner's lock is not free, 
      fail to get the lock and add the client to the pending list.
    */
  if(lockattr->state == LOCKED)
  {
    lockattr->state = REVOKING;
    lockattr->waiting_clients.push(id);
    //printf("Revoke client %s in locked\n",lockattr->owner.c_str());
    pthread_mutex_unlock(&lock);
    client_callers[lockattr->owner]->call(rlock_protocol::revoke,lid,r);
    //No matter this revoke successfully 
    return lock_protocol::RETRY;
  }
  else if(lockattr->state == REVOKING)
  {
    lockattr->waiting_clients.push(id);
    ret = lock_protocol::RETRY;
  }
  else if(lockattr->state == RETRYING){
    //Another client try to get the lock but not the most qualified one.
    if(id != lockattr->waiting_clients.front())
    {
      lockattr->waiting_clients.push(id);
      ret = lock_protocol::RETRY;
    }
    //The most qualified client retry for the lock: satisfy it!
    else
    {
      lockattr->owner = lockattr->waiting_clients.front();
      lockattr->waiting_clients.pop();
      lockattr->state = LOCKED;
      //printf("The client %s get the lock in retry\n",id.c_str());
      //TODO: if other waiting clients exist, change the server state to revoking and revoke 
      //the new owner of the lock
      //Alert!!!: The new owner must get the lock first, and then get then try to release 
      //the lock!(The logic should be carefully implemented in revoke handler)
      if(!lockattr->waiting_clients.empty()){
        lockattr->state = REVOKING;
        pthread_mutex_unlock(&lock);
        //printf("Revoke client %s in retrying\n",lockattr->owner.c_str());
        client_callers[lockattr->owner]->call(rlock_protocol::revoke,lid,r);
        return lock_protocol::OK;  
      }
    }
  }

  pthread_mutex_unlock(&lock);
  return ret;
}

int lock_server_cache::release(lock_protocol::lockid_t lid, std::string id,
                               int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  //The client revoke handler is responsible to call the server to release the lock.
  /*
    TODO: the status of the server from revoking->retrying
    call the head of the waiting client to retry.
  */
  pthread_mutex_lock(&lock);
  //TODO: may be blocked here!
  srv_lockattr* lockattr = lock_ownership[lid];
  assert(lockattr->owner == id);
  assert(lockattr->state == REVOKING);
  lockattr->state = RETRYING;
  lockattr->owner.clear();
  int re;
  //printf("client %s released the lock successfully\n",id.c_str());
  pthread_mutex_unlock(&lock);
  //printf("rpc: call client %s to retry\n",lockattr->waiting_clients.front().c_str());
  client_callers[lockattr->waiting_clients.front()]->call(rlock_protocol::retry,lid,re);
  
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{ 
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}
