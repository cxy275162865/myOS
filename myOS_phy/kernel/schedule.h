#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "task.h"

struct Scheduler {
    long running_tsk_cnt;               // ready queue length
    long CPU_exec_tsk_jiffies;          // nr time slice
    struct Task_struct task_queue;      // ready queue,
                                        // in ascending order by vrun_time
};

struct Scheduler task_scheduler;

void init_schedule();

void schedule();

#endif

