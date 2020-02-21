#include "schedule.h"
#include "task.h"
#include "lib.h"
#include "printk.h"
#include "timer.h"


// select from the ready queue
struct Task_struct * get_next_task() {

    if (list_isEmpty(&task_scheduler.task_queue.list)) {
        return &init_task_union.task;
    }

    struct Task_struct * ret = CONTAINER_OF(
            list_next(&task_scheduler.task_queue.list), struct Task_struct, list);

    list_del(&ret->list);
    task_scheduler.running_tsk_cnt--;
    return ret;
}


void insert_task_queue(struct Task_struct * tsk) {
    if (tsk == &init_task_union.task)
        return;

    struct Task_struct * tmp = CONTAINER_OF(
            list_next(&task_scheduler.task_queue.list), struct Task_struct, list);

    if (list_isEmpty(&task_scheduler.task_queue.list)) {

    } else {

        // insert at the pos in ascending order
        while (tmp->vrun_time < tsk->vrun_time) {
            tmp = CONTAINER_OF(
                    list_next(&tmp->list), struct Task_struct, list);
        }
    }

    list_add_to_before(&tmp->list, &tsk->list);
    task_scheduler.running_tsk_cnt++;
}



void schedule() {
    current->flags &= ~NEED_SCHEDULE;
    struct Task_struct *tsk = get_next_task();

    color_printk(RED, BLACK, "#schedule:%d#\n", jiffies);


    if (current->vrun_time >= tsk->vrun_time) {
        // time slice passed
        if (current->state == TASK_RUNNING)
            insert_task_queue(current);

        if (!task_scheduler.CPU_exec_tsk_jiffies) {
            switch (tsk->priority) {
                case 0:
                case 1:
                    task_scheduler.CPU_exec_tsk_jiffies = 4 /
                              task_scheduler.running_tsk_cnt;
                    break;
                case 2:
                default:
                    task_scheduler.CPU_exec_tsk_jiffies = 4 /
                              task_scheduler.running_tsk_cnt * 3;
            }
        }
        switch_to(current, tsk);

    } else {

        // current->vrun_time is still the minimum
        insert_task_queue(tsk);

        if (!task_scheduler.CPU_exec_tsk_jiffies) {
            switch (tsk->priority) {
                case 0:
                case 1:
                    task_scheduler.CPU_exec_tsk_jiffies = 4 /
                            task_scheduler.running_tsk_cnt;
                    break;
                case 2:
                default:
                    task_scheduler.CPU_exec_tsk_jiffies = 4 /
                            task_scheduler.running_tsk_cnt * 3;
            }
        }
    }
}




void init_schedule() {
    memset(&task_scheduler, 0x00, sizeof(struct Scheduler));

    list_init(&task_scheduler.task_queue.list);
    task_scheduler.task_queue.vrun_time = 0x7fffffffffffffff;

    task_scheduler.running_tsk_cnt = 1;     // kernel process(idle process)
    task_scheduler.CPU_exec_tsk_jiffies = 4;
}


