#ifndef _TIMER_H
#define _TIMER_H

#include "lib.h"


struct Timer_list {
    struct List list;
    unsigned long expire_jiffies;
    void (* func) (void * data);
    void * data;
};

struct Timer_list timer_list_head;

void init_timer(struct Timer_list *timer,
        void (*func)(void *data), void *data, unsigned long expire_jiffies);

void add_timer(struct Timer_list * timer);

void del_timer(struct Timer_list * timer);




unsigned long volatile jiffies = 0;

void timer_init();
void do_timer();


#endif



