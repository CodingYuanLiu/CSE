// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"
#include <pthread.h>
#include <map>
#include <list>

// Classes that inherit lock_release_user can override dorelease so that
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 6.
class lock_release_user {
public:
    virtual void dorelease(lock_protocol::lockid_t) = 0;

    virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
private:
    class lock_release_user *lu;

    int rlock_port;
    std::string hostname;

    //The revoke and retry handler set the message and handle in asynchronize way.
    enum Message {
        EMPTY,
        RETRY,
        REVOKE
    };
    
    //just like the hint said.
    enum LockState {
        NONE,
        ACQUIRING,
        FREE,
        RELEASING,
        LOCKED,
    };

    
    struct waiting_thread {

        pthread_cond_t cv;

        waiting_thread() {
            pthread_cond_init(&cv, NULL);
        }
    };


    struct cli_lockattr {
        LockState state;
        Message message;
        //The queue of the waiting threads. The head thread is responsible for acquiring the lock.
        std::list<waiting_thread *> threads;

        cli_lockattr() {
            state = NONE;
            message = EMPTY;
        }
    };


    pthread_mutex_t lock;

    std::map<lock_protocol::lockid_t, cli_lockattr *> lockManager;

    lock_protocol::status acquire_from_server(cli_lockattr *,
                                        lock_protocol::lockid_t,
                                        waiting_thread *thisThread);


public:
    std::string id;

    static int last_port;

    lock_client_cache(std::string xdst, class lock_release_user *l = 0);

    virtual ~lock_client_cache();

    lock_protocol::status acquire(lock_protocol::lockid_t);

    lock_protocol::status release(lock_protocol::lockid_t);

    rlock_protocol::status revoke_handler(lock_protocol::lockid_t,
                                          int &);

    rlock_protocol::status retry_handler(lock_protocol::lockid_t,
                                         int &);
};


#endif