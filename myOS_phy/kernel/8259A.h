#ifndef _8259A_H
#define _8259A_H

#include "linkage.h"
#include "ptrace.h"



void init_8259A();

void do_IRQ(struct PT_regs *regs, unsigned long nr);


#endif

