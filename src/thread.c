/*
 * Copyright 2017-2025 Matt "MateoConLechuga" Waltz
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "clean.h"
#include "strings.h"
#include "log.h"
#include "memory.h"
#include "thread.h"

#include <string.h>
#include <stdlib.h>

struct thread
{
    bool (*func)(void*);
    void *args;
    thrd_t thrd;
    uint32_t id;
};

static struct
{
    struct thread thread[THREAD_MAX];
    unsigned int id[THREAD_MAX];
    atomic_bool error;
    atomic_size_t head;
    atomic_size_t tail;
    atomic_size_t count;
    atomic_size_t max_count;
} thread_pool;

static void thread_pool_push(unsigned int id)
{
    thread_pool.id[thread_pool.head] = id;
    thread_pool.head++;
    thread_pool.count++;
    if (thread_pool.head >= THREAD_MAX)
    {
        thread_pool.head = 0;
    }
}

static struct thread *thread_pool_pop(void)
{
    int next;
    unsigned int id;
    struct thread *thread;

    while (thread_pool.head == thread_pool.tail)
    {
        thrd_yield();
    }

    next = thread_pool.tail + 1;
    if (next >= THREAD_MAX)
    {
        next = 0;
    }

    id = thread_pool.id[thread_pool.tail];
    thread_pool.tail = next;
    thread_pool.count--;

    thread = &thread_pool.thread[id];
    thread->id = id;

    return thread;
}

bool thread_pool_wait(void)
{
    while (thread_pool.count != thread_pool.max_count)
    {
        thrd_yield();
    }

    return !thread_pool.error;
}

static int thread_func(void *t)
{
    struct thread *thread = t;

    LOG_DEBUG("Enter thread %u\n", thread->id);

    if (!thread->func(thread->args))
    {
        thread_pool.error = true;
    }

    LOG_DEBUG("Exit thread %u\n", thread->id);

    thread_pool_push(thread->id);

    return 0;
}

bool thread_start(bool (*func)(void*), void *args)
{
    struct thread *thread = thread_pool_pop();

    thread->func = func;
    thread->args = args;

    if (thrd_create(&thread->thrd, thread_func, thread) != thrd_success)
    {
        LOG_ERROR("Could not start thread.");
        thread_pool_push(thread->id);
        thread_pool.error = true;
        return false;
    }

    return true;
}

void thread_pool_init(unsigned int max_count)
{
    for (unsigned int i = 0; i < max_count; ++i)
    {
        thread_pool_push(i);
    }
    thread_pool.max_count = max_count;
    thread_pool.error = false;
}
