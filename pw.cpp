#include <stdlib.h>
#include <pthread.h>
#include "pw.h"

namespace PW
{

struct Thread;
struct PWCtx;
static void* PWMain(void* arg);

struct Thread
{
    enum {
        THREAD_RUN,
        THREAD_WAIT, 
    };
    pthread_t id;
    pthread_cond_t state_cond;
    pthread_mutex_t state_mutex;
    int state;
    PWBase* worker;
    Thread* next;
    PWCtx* ctx;
};

struct PWCtx
{
    int count;
    int running_count;
    int finish_count;
    Thread* thread_list;
    pthread_mutex_t list_mutex;
    pthread_mutex_t wait_mutex;
    pthread_cond_t wait_cond;
};

PWManager::PWManager()
{
    _ctx = (PWCtx*)calloc(1, sizeof(PWCtx));
    _ctx->count = 0;
    _ctx->running_count = 0;
    _ctx->finish_count = 0;
    _ctx->thread_list = NULL;
    pthread_mutex_init(&_ctx->list_mutex, NULL);
    pthread_mutex_init(&_ctx->wait_mutex, NULL);
    pthread_cond_init(&_ctx->wait_cond, NULL);
}

PWManager::~PWManager()
{
    Thread* thread = _ctx->thread_list;
    Thread* thread_next = NULL;
    while (thread) {
       thread_next = thread->next;
       free(thread);
       thread = thread_next;
    }
    free(_ctx);
}

PWManager* PWManager::Instance()
{
    static PWManager instance;
    return &instance;
}

int PWManager::Init(int count)
{
    _ctx->count = count;
    _ctx->finish_count = 0;
    _ctx->running_count = 0;
}

int PWManager::Run(PWBase* worker)
{
    Thread* thread;

    if (_ctx->running_count >= _ctx->count) return PW_ERR_LIMIT;
    ++_ctx->running_count;

    pthread_mutex_lock(&_ctx->list_mutex);
    if (_ctx->thread_list == NULL) 
    {
        if (PW_OK != Create()) {
            pthread_mutex_unlock(&_ctx->list_mutex);
            --_ctx->running_count;
            return PW_ERR_SYS;
        }
    } 
    thread = _ctx->thread_list;
    _ctx->thread_list = _ctx->thread_list->next;
    pthread_mutex_unlock(&_ctx->list_mutex);

    pthread_mutex_lock(&thread->state_mutex);
    thread->worker = worker;
    thread->state = Thread::THREAD_RUN;
    pthread_cond_signal(&thread->state_cond);
    pthread_mutex_unlock(&thread->state_mutex);

    return PW_OK;
}

int PWManager::Wait()
{
    while (1)
    {
        pthread_mutex_lock(&_ctx->wait_mutex);
        if (_ctx->finish_count >= _ctx->count) break;
        pthread_cond_wait(&_ctx->wait_cond, &_ctx->wait_mutex);
        pthread_mutex_unlock(&_ctx->wait_mutex);
    }
    pthread_mutex_unlock(&_ctx->wait_mutex);
}

int PWManager::Create()
{
    Thread* thread = (Thread*)calloc(1, sizeof(Thread));

    thread->state = Thread::THREAD_WAIT;
    thread->worker = NULL;
    thread->ctx = _ctx;
    pthread_mutex_init(&thread->state_mutex, NULL);
    pthread_cond_init(&thread->state_cond, NULL);
    
    thread->next = _ctx->thread_list;
    _ctx->thread_list = thread;

    int ret = pthread_create(&thread->id, NULL, PWMain, (void*)thread);
    if (0 != ret)
    {
        return PW_ERR_SYS;
    }

    return PW_OK;
}

static void* PWMain(void* arg)
{
    Thread* thread = (Thread*)arg;
    PWCtx* ctx = thread->ctx;

    while(1) 
    {
        for (;;) 
        {
            pthread_mutex_lock(&thread->state_mutex);
            if (thread->state == Thread::THREAD_RUN) break;
            pthread_cond_wait(&thread->state_cond, &thread->state_mutex);
            pthread_mutex_unlock(&thread->state_mutex);
        }
        pthread_mutex_unlock(&thread->state_mutex);

        if (thread->worker)
        {
            thread->worker->Process();

            pthread_mutex_lock(&ctx->wait_mutex);
            if (++ctx->finish_count >= ctx->count) 
            {
                pthread_cond_signal(&ctx->wait_cond);
            }
            pthread_mutex_unlock(&ctx->wait_mutex);
        }

        thread->worker = NULL;
        thread->state = Thread::THREAD_WAIT;

        pthread_mutex_lock(&ctx->list_mutex);
        thread->next = ctx->thread_list;
        ctx->thread_list = thread;
        pthread_mutex_unlock(&ctx->list_mutex);
    }

    return NULL;
}

}
