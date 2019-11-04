// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/syscall.h>

#define gettid() syscall(__NR_gettid)

lock_server::lock_server():
  nacquire (0)
{
  pthread_mutex_init(&lock,NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("Thread %ld acquire lock %016llx\n",gettid(),lid);
	// Your lab2 part2 code goes here
  pthread_mutex_lock(&lock);  


  pthread_mutex_t* client_lock;

  //The first client that acquires for lock(lid)
  if(client_locks.find(lid) == client_locks.end()){
    client_lock = new pthread_mutex_t;
    pthread_mutex_init(client_lock,NULL);
    client_locks[lid] = client_lock;
  } 
  else{
    client_lock = client_locks[lid];
  }
  
  pthread_mutex_unlock(&lock);
  
  pthread_mutex_lock(client_lock);
  printf("Thread %ld get lock %016llx\n",gettid(),lid);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  
	// Your lab2 part2 code goes here
  pthread_mutex_lock(&lock);

  
  assert(client_locks.find(lid) != client_locks.end());
  printf("Thread %ld release lock %016llx\n",gettid(),lid);
  pthread_mutex_unlock(client_locks[lid]);
  pthread_mutex_unlock(&lock);
  r = ret;
  return ret;
}
