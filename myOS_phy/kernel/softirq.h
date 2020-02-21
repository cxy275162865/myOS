#ifndef _SOFTIRQ_H
#define _SOFTIRQ_H

#define TIMER_SIRQ  (1 << 0)

unsigned long softirq_status = 0;

struct SoftIrq {
    void (*action)(void *data);
    void * data;
};

struct SoftIrq softirq_vector[64] = {0, };


void set_softirq_status(unsigned long status);
unsigned long get_softirq_status();

void register_softirq(int nr, void (*action)(void *data), void *data);
void unregister_softirq(int nr);

void softirq_init();


#endif



