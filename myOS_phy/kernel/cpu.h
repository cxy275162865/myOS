#ifndef _CPU_H
#define _CPU_H

#define NR_CPUS  8

void get_cpuid(unsigned int mop, unsigned int sop, unsigned int *a,
               unsigned int *b, unsigned int *c, unsigned int *d);


void init_cpu();

#endif
