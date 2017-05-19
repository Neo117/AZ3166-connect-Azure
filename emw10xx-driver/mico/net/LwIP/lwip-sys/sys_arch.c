/* MiCO Team
 * Copyright (c) 2017 MXCHIP Information Tech. Co.,Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/* mbed includes */
#include "mbed_error.h"
#include "mbed_interface.h"
#include "us_ticker_api.h"

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"

#ifdef TCP_RX_PBUF_COPY
/* Message queue constants. */
#define archMESG_QUEUE_LENGTH     ( (unsigned long) 120 )
#else
#define archMESG_QUEUE_LENGTH     ( (unsigned long) 6 )
#endif
#define archPOST_BLOCK_TIME_MS    ( ( unsigned long ) 10000 )

/* The timeout code seems to be unused */
#if LWIP_SYS_ARCH_TIMEOUTS
struct timeout_list
{
    struct sys_timeouts timeouts;
    xTaskHandle pid;
};
static struct timeout_list timeout_list[SYS_THREAD_MAX];
static uint16_t next_thread = 0;
#endif /* if LWIP_SYS_ARCH_TIMEOUTS */

/* This is the number of threads that can be started with sys_thread_new() */
#define SYS_THREAD_MAX                 (4)

mico_mutex_t lwip_sys_mutex;

u32_t sys_now(void)
{
    return (u32_t)mico_rtos_get_time();
}

/*-----------------------------------------------------------------------------------
 * Creates an empty mailbox.
 */
err_t sys_mbox_new( sys_mbox_t *mbox, /*@unused@*/int size )
{
    /*@-noeffect@*/
    (void) size; /* unused parameter */
    /*@+noeffect@*/
    OSStatus result = mico_rtos_init_queue(mbox, NULL, (unsigned long) sizeof(void *), archMESG_QUEUE_LENGTH);

    return result == kNoErr ? ERR_OK : ERR_VAL;
}

/*-----------------------------------------------------------------------------------
 * Deallocates a mailbox. If there are messages still present in the
 * mailbox when the mailbox is deallocated, it is an indication of a
 * programming error in lwIP and the developer should be notified.
 */
void sys_mbox_free( sys_mbox_t *mbox )
{
    if ( mico_rtos_is_queue_empty( mbox ) != true )
    {
        /* Line for breakpoint.  Should never break here! */
#ifdef __GNUC__
        __asm volatile ( "NOP" );
#elif defined( __IAR_SYSTEMS_ICC__ )
        asm( "NOP" );
#endif
    }

    mico_rtos_deinit_queue( mbox );
}

/*-----------------------------------------------------------------------------------
 * Posts the "msg" to the mailbox.
 */
void sys_mbox_post( sys_mbox_t *mbox, void *msg )
{
    OSStatus result = mico_rtos_push_to_queue(mbox, &msg, archPOST_BLOCK_TIME_MS);

    LWIP_ASSERT("Error posting to LwIP mailbox", result == kNoErr );
}

/*-----------------------------------------------------------------------------------
 * Blocks the thread until a message arrives in the mailbox, but does
 * not block the thread longer than "timeout" milliseconds (similar to
 * the sys_arch_sem_wait() function). The "msg" argument is a result
 * parameter that is set by the function (i.e., by doing "*msg =
 * ptr"). The "msg" parameter maybe NULL to indicate that the message
 * should be dropped.
 *
 * The return values are the same as for the sys_arch_sem_wait() function:
 * Number of milliseconds spent waiting or SYS_ARCH_TIMEOUT if there was a
 * timeout.
 *
 * Note that a function with a similar name, sys_mbox_fetch(), is
 * implemented by lwIP.
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, /*@null@*/ /*@out@*/ void **msg, u32_t timeout)
{
    void *dummyptr;
    void ** tmp_ptr;
    mico_time_t start_time, end_time, elapsed_time;

    start_time = mico_rtos_get_time( );

    if ( msg == NULL )
    {
        tmp_ptr = &dummyptr;
    }
    else
    {
        tmp_ptr = msg;
    }

    if ( timeout != 0 )
    {   
        if ( kNoErr == mico_rtos_pop_from_queue( mbox, tmp_ptr, timeout ) )
        {
            end_time = mico_rtos_get_time( );
            elapsed_time = end_time - start_time;
            if ( elapsed_time == 0 )
            {
                elapsed_time = 1;
            }
            return ( elapsed_time );
        }
        else /* timed out blocking for message */
        {
            if ( msg != NULL )
            {
                *msg = NULL;
            }
            return SYS_ARCH_TIMEOUT;
        }
    }
    else /* block forever for a message. */
    {
        mico_rtos_pop_from_queue( mbox, &(*tmp_ptr), MICO_WAIT_FOREVER);
        end_time = mico_rtos_get_time( );
        elapsed_time = end_time - start_time;
        if ( elapsed_time == 0 )
        {
            elapsed_time = 1;
        }
        return ( elapsed_time ); /* return time blocked TBD test */
    }
}

/*-----------------------------------------------------------------------------------
 * Creates and returns a new semaphore. The "count" argument specifies
 * the initial state of the semaphore. TBD finish and test
 */
err_t sys_sem_new( /*@out@*/ sys_sem_t *sem, u8_t count)
{
    OSStatus result = mico_rtos_init_semaphore(sem, 1);
    if ( result != kNoErr )
    {
        return (err_t) ERR_MEM;
    }
    mico_rtos_set_semaphore(sem);

    if ( count == (u8_t) 0 ) /* Means it can't be taken */
    {
        if ( kNoErr != mico_rtos_get_semaphore(sem, 1 ) )
        {
            return ERR_VAL;
        }
    }

    return (err_t) ERR_OK;
}

/*-----------------------------------------------------------------------------------
 * Blocks the thread while waiting for the semaphore to be
 * signaled. If the "timeout" argument is non-zero, the thread should
 * only be blocked for the specified time (measured in
 * milliseconds).
 *
 * If the timeout argument is non-zero, the return value is the number of
 * milliseconds spent waiting for the semaphore to be signaled. If the
 * semaphore wasn't signaled within the specified time, the return value is
 * SYS_ARCH_TIMEOUT. If the thread didn't have to wait for the semaphore
 * (i.e., it was already signaled), the function may return zero.
 *
 * Notice that lwIP implements a function with a similar name,
 * sys_sem_wait(), that uses the sys_arch_sem_wait() function.
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    mico_time_t start_time, end_time, elapsed_time;

    start_time = mico_rtos_get_time( );

    if ( timeout != 0 )
    {
        if (kNoErr == mico_rtos_get_semaphore(sem, timeout))
        {
            end_time = mico_rtos_get_time( );
            elapsed_time = end_time - start_time;
            if ( elapsed_time == 0 )
            {
                elapsed_time = 1;
            }
            return ( elapsed_time ); /* return time blocked TBD test */
        }
        else
        {
            return SYS_ARCH_TIMEOUT;
        }
    }
    else /* must block without a timeout */
    {
        mico_rtos_get_semaphore(sem, MICO_WAIT_FOREVER);
        end_time = mico_rtos_get_time( );
        elapsed_time = end_time - start_time;
        if ( elapsed_time == 0 )
        {
            elapsed_time = 1;
        }

        return ( elapsed_time ); /* return time blocked */

    }
}

/*-----------------------------------------------------------------------------------
 * Signals a semaphore
 */
void sys_sem_signal( sys_sem_t *sem )
{
    OSStatus result = mico_rtos_set_semaphore(sem);
    /*@-noeffect@*/
    (void) result;  /* unused in release build */
    /*@+noeffect@*/

    LWIP_ASSERT( "FreeRTOS failed to set semaphore for LwIP", result == kNoErr );
}

/*-----------------------------------------------------------------------------------
 * Deallocates a semaphore
 */
void sys_sem_free( sys_sem_t *sem )
{
    mico_rtos_deinit_semaphore( sem );
}

/*-----------------------------------------------------------------------------------
 * Initialize sys arch
 */
void sys_init( void )
{
#if LWIP_SYS_ARCH_TIMEOUTS
    int i;
    /* Initialize the the per-thread sys_timeouts structures
     * make sure there are no valid pids in the list
     */
    for(i = 0; i < SYS_THREAD_MAX; i++)
    {
        timeout_list[i].pid = 0;
    }

    /* keep track of how many threads have been created */
    next_thread = 0;
#endif /* if LWIP_SYS_ARCH_TIMEOUTS */

	mico_rtos_init_mutex(&lwip_sys_mutex);
    if (lwip_sys_mutex == NULL)
        error("sys_init error\n");
}

#if LWIP_SYS_ARCH_TIMEOUTS
/*-----------------------------------------------------------------------------------
 * Returns a pointer to the per-thread sys_timeouts structure. In lwIP,
 * each thread has a list of timeouts which is represented as a linked
 * list of sys_timeout structures. The sys_timeouts structure holds a
 * pointer to a linked list of timeouts. This function is called by
 * the lwIP timeout scheduler and must not return a NULL value.
 *
 * In a single threaded sys_arch implementation, this function will
 * simply return a pointer to a global sys_timeouts variable stored in
 * the sys_arch module.
 */

struct sys_timeouts *
sys_arch_timeouts(void)
{
    int i;
    sys_thread_t pid;
    struct timeout_list *tl;

    pid = mico_rtos_get_current_thread_handle( );

    for(i = 0; i < next_thread; i++)
    {
        tl = &timeout_list[i];
        if(tl->pid == pid)
        {
            return &(tl->timeouts);
        }
    }

    /* Error */
    return NULL;
}
#endif /* if LWIP_SYS_ARCH_TIMEOUTS */

/*-----------------------------------------------------------------------------------
 * Starts a new thread with priority "prio" that will begin its execution in the
 * function "thread()". The "arg" argument will be passed as an argument to the
 * thread() function. The id of the new thread is returned. Both the id and
 * the priority are system dependent.
 */
/*@null@*/ sys_thread_t sys_thread_new( const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio )
{
    sys_thread_t created_task;
    OSStatus result;
    /* The first time this is called we are creating the lwIP handler. */
    result = mico_rtos_create_thread(&created_task, prio, name, (mico_thread_function_t)thread, stacksize, (mico_thread_arg_t)arg);
#if LWIP_SYS_ARCH_TIMEOUTS
    /* For each task created, store the task handle (pid) in the timers array.
     * This scheme doesn't allow for threads to be deleted
     */
    timeout_list[next_thread++].pid = created_task;
#endif /* if LWIP_SYS_ARCH_TIMEOUTS */

    if ( result == kNoErr )
    {
        return created_task;
    }
    else
    {
        return NULL;
    }
}

/*-----------------------------------------------------------------------------------
 * This optional function does a "fast" critical region protection and returns
 * the previous protection level. This function is only called during very short
 * critical regions. An embedded system which supports ISR-based drivers might
 * want to implement this function by disabling interrupts. Task-based systems
 * might want to implement this by using a mutex or disabling tasking. This
 * function should support recursive calls from the same task or interrupt. In
 * other words, sys_arch_protect() could be called while already protected. In
 * that case the return value indicates that it is already protected.
 *
 * sys_arch_protect() is only required if your port is supporting an operating
 * system.
 */
sys_prot_t sys_arch_protect( void )
{
    if (mico_rtos_lock_mutex(&lwip_sys_mutex) != kNoErr)
        error("sys_arch_protect error\n");
    return (sys_prot_t) 1;
}

/*-----------------------------------------------------------------------------------
 * This optional function does a "fast" set of critical region protection to the
 * value specified by pval. See the documentation for sys_arch_protect() for
 * more information. This function is only required if your port is supporting
 * an operating system.
 */
void sys_arch_unprotect( sys_prot_t pval )
{
    /*@-noeffect@*/
    (void) pval;  /* Parameter is unused */
    /*@+noeffect@*/
    if (mico_rtos_unlock_mutex(&lwip_sys_mutex) != kNoErr)
        error("sys_arch_unprotect error\n");
}

/*-----------------------------------------------------------------------------------
 * Similar to sys_arch_mbox_fetch, but if message is not ready immediately, we'll
 * return with SYS_MBOX_EMPTY.  On success, 0 is returned.
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *dummy_ptr;
    void ** tmp_ptr = msg;

    if ( msg == NULL )
    {
        tmp_ptr = &dummy_ptr;
    }

    if ( kNoErr == mico_rtos_pop_from_queue( mbox, tmp_ptr, 0 ) )
    {
        return ERR_OK;
    }
    else
    {
        return SYS_MBOX_EMPTY;
    }
}


/*-----------------------------------------------------------------------------------
 * Try to post the "msg" to the mailbox.
 */
err_t sys_mbox_trypost( sys_mbox_t *mbox, void *msg )
{
    err_t result;

    if ( mico_rtos_push_to_queue( mbox, &msg, 0 ) == kNoErr )
    {
        result = ERR_OK;
    }
    else
    {
        /* could not post, queue must be full */
        result = ERR_MEM;

#if SYS_STATS
        lwip_stats.sys.mbox.err++;
#endif /* SYS_STATS */

    }

    return result;
}

int sys_mbox_valid( sys_mbox_t *mbox )
{
    return (int)( *mbox != 0 );
}

int sys_sem_valid( sys_sem_t *sem )
{
    return (int)( *sem != 0 );
}

void sys_mbox_set_invalid( sys_mbox_t *mbox )
{
    ( *(int*) mbox ) = 0;
}

void sys_sem_set_invalid( sys_sem_t *sem )
{
    ( *(int*) sem ) = 0;
}
