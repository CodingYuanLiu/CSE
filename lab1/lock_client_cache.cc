// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include "tprintf.h"


int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst,
                                     class lock_release_user *_lu)
        : lock_client(xdst), lu(_lu) {
    srand(time(NULL) ^ last_port);
    rlock_port = ((rand() % 32000) | (0x1 << 10));
    const char *hname;
    // VERIFY(gethostname(hname, 100) == 0);
    hname = "127.0.0.1";
    std::ostringstream host;
    host << hname << ":" << rlock_port;
    id = host.str();
    last_port = rlock_port;
    rpcs *rlsrpc = new rpcs(rlock_port);
    rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
    rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);
    pthread_mutex_init(&lock, NULL);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid) {

    pthread_mutex_lock(&lock);
    if (lockManager.find(lid) == lockManager.end()) lockManager[lid] = new cli_lockattr();
    cli_lockattr *lockEntry = lockManager[lid];
    waiting_thread *thisThread = new waiting_thread();
    if (lockEntry->threads.empty()) {
        LockState s = lockEntry->state;
        lockEntry->threads.push_back(thisThread);
        if (s == FREE) {
            lockEntry->state = LOCKED;
            pthread_mutex_unlock(&lock);
            return lock_protocol::OK;
        } else if (s == NONE) {
            //Acquire the lock from the server.
            return acquire_from_server(lockEntry, lid, thisThread);
        } else {
            //Sleep and wait for the former operation done and then get the lock.
            pthread_cond_wait(&thisThread->cv, &lock);
            return acquire_from_server(lockEntry, lid, thisThread);
        }
    } else {
        lockEntry->threads.push_back(thisThread);
        pthread_cond_wait(&thisThread->cv, &lock);
        switch (lockEntry->state) {
            case FREE:
		        lockEntry->state = LOCKED;
                pthread_mutex_unlock(&lock);
                return lock_protocol::OK;
            case LOCKED:
                pthread_mutex_unlock(&lock);
                return lock_protocol::OK;
            case NONE:
                return acquire_from_server(lockEntry, lid, thisThread);
            default:
                assert(0);
        }
    }
}


lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid) {

    pthread_mutex_lock(&lock);
    int r;
    int ret = rlock_protocol::OK;
    cli_lockattr *lockEntry = lockManager[lid];
    bool fromRevoked = false;
    //If the client received revoke rpc
    if (lockEntry->message == REVOKE) {
        fromRevoked = true;
        lockEntry->state = RELEASING;

        pthread_mutex_unlock(&lock);
        ret = cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&lock);
        lockEntry->message = EMPTY;
        lockEntry->state = NONE;
    } else lockEntry->state = FREE;

    delete lockEntry->threads.front();
    lockEntry->threads.pop_front();
    //Other threads coming to acquire the lock when doing above operations.
    if (lockEntry->threads.size() >= 1) {
        if (!fromRevoked) 
            lockEntry->state = LOCKED;
        pthread_cond_signal(&lockEntry->threads.front()->cv);
    }
    pthread_mutex_unlock(&lock);
    return ret;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid,
                                  int &) {
    int r;
    int ret = rlock_protocol::OK;
    pthread_mutex_lock(&lock);
    cli_lockattr *lockEntry = lockManager[lid];
    if (lockEntry->state == FREE) {
        lockEntry->state = RELEASING;
        pthread_mutex_unlock(&lock);
        ret = cl->call(lock_protocol::release, lid, id, r);
        pthread_mutex_lock(&lock);
        lockEntry->state = NONE;
        //If other threads come to acquire the lock in state releasing
        if (lockEntry->threads.size() >= 1) {
            pthread_cond_signal(&lockEntry->threads.front()->cv);
        }
    } else { lockEntry->message = REVOKE; }
    pthread_mutex_unlock(&lock);
    return ret;
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid,
                                 int &) {
    int ret = rlock_protocol::OK;
    pthread_mutex_lock(&lock);
    cli_lockattr *lockEntry = lockManager[lid];
    lockEntry->message = RETRY;
    //Wake up the front thread to acquire the lock.
    pthread_cond_signal(&lockEntry->threads.front()->cv);
    pthread_mutex_unlock(&lock);
    return ret;
}


lock_protocol::status lock_client_cache::
acquire_from_server(lock_client_cache::cli_lockattr *lockEntry,
              lock_protocol::lockid_t lid,
              waiting_thread *thisThread) {
    int r;
    lockEntry->state = ACQUIRING;
    while (lockEntry->state == ACQUIRING) {
        pthread_mutex_unlock(&lock);
        int ret = cl->call(lock_protocol::acquire, lid, id, r);
        pthread_mutex_lock(&lock);
        if (ret == lock_protocol::OK) {
            lockEntry->state = LOCKED;
            pthread_mutex_unlock(&lock);
            return lock_protocol::OK;
        } else {
            //Cannot get the lock: wait for another retry and acquire the lock again.
            if (lockEntry->message == EMPTY) {
                pthread_cond_wait(&thisThread->cv, &lock);
                lockEntry->message = EMPTY;
            } else{
                lockEntry->message = EMPTY;
            } 
        }
    }
    assert(0);
}

lock_client_cache::~lock_client_cache() {
    pthread_mutex_lock(&lock);
    std::map<lock_protocol::lockid_t, cli_lockattr *>::iterator iter;
    for (iter = lockManager.begin(); iter != lockManager.end(); iter++)
        delete iter->second;
    pthread_mutex_unlock(&lock);
}