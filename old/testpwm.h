#ifndef _KTHREAD_H
#define _KTHREAD_H
#include <linux/config.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tqueue.h>
#include <linux/wait.h>

#include <asm/unistd.h>
#include <asm/semaphore.h>

typedef struct kthread_struct
{
        struct task_struct *thread;
        struct tq_struct tq;
        void (*function) (struct kthread_struct *kthread);
        struct semaphore startstop_sem;

        wait_queue_head_t queue;
        int terminate;
        void *arg;
} kthread_t;

void start_kthread(void (*func)(kthread_t *), kthread_t *kthread);

void stop_kthread(kthread_t *kthread);

void init_kthread(kthread_t *kthread, char *name);

void exit_kthread(kthread_t *kthread);

#endif
