#include "timer.h"
#include "softirq.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"

void funca(void * data) {
    color_printk(BLUE, WHITE, "this is a 5s timer task!!\n");
}

void timer_init() {
    jiffies = 0;
    init_timer(&timer_list_head, NULL, NULL, -1UL);

    register_softirq(0, &do_timer, NULL);

    struct Timer_list * tmp = (struct Timer_list *)
            kmalloc(sizeof(struct Timer_list), 0);
    init_timer(tmp, &funca, NULL, 5);
    add_timer(tmp);

}


void init_timer(struct Timer_list *timer,
        void (*func)(void *data),
        void *data, unsigned long expire_jiffies) {

    list_init(&timer->list);
    timer->func = func;
    timer->data = data;
    timer->expire_jiffies = jiffies + expire_jiffies;
}

void add_timer(struct Timer_list * timer) {

    struct Timer_list * tmp = CONTAINER_OF(list_next(&timer_list_head.list),
            struct Timer_list, list);

    if (list_isEmpty(&timer_list_head.list)) {

    } else {
        while (tmp->expire_jiffies < timer->expire_jiffies)
            tmp = CONTAINER_OF(list_next(&tmp->list),
                               struct Timer_list, list);
    }
    list_add_to_next(&tmp->list, &timer->list);
}

void del_timer(struct Timer_list * timer) {
    list_del(&timer->list);
}



void do_timer(void * data) {
    struct Timer_list * tmp = CONTAINER_OF(list_next(&timer_list_head.list),
            struct Timer_list, list);

    while (!list_isEmpty(&timer_list_head.list)
            && tmp->expire_jiffies <= jiffies) {
        del_timer(tmp);
        tmp->func(tmp->data);
        tmp = CONTAINER_OF(list_next(&tmp->list),
                           struct Timer_list, list);
    }
    color_printk(INDIGO, BLACK, "%lds ", jiffies);

}



