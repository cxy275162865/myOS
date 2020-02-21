#include "HPET.h"
#include "printk.h"
#include "memory.h"
#include "lib.h"
#include "APIC.h"
#include "time.h"
#include "timer.h"
#include "softirq.h"
#include "task.h"
#include "schedule.h"

hw_int_controller HPET_int_controller = {
        .enable = IOAPIC_enable,
        .disable = IOAPIC_disable,
        .install = IOAPIC_install,
        .uninstall = IOAPIC_uninstall,
        .ack = IOAPIC_edge_ack,
};


void HPET_handler(unsigned long nr, unsigned long param,
        struct PT_regs * regs) {

    jiffies++;

    if (CONTAINER_OF(list_next(&timer_list_head.list), struct Timer_list, list)
            ->expire_jiffies >= jiffies) {
        set_softirq_status(TIMER_SIRQ);
    }

    switch (current->priority) {
        case 0:
        case 1:
            task_scheduler.CPU_exec_tsk_jiffies--;
            current->vrun_time++;
        case 2:
        default:
            task_scheduler.CPU_exec_tsk_jiffies -= 2;
            current->vrun_time += 2;
    }

    if (task_scheduler.CPU_exec_tsk_jiffies <= 0) {
        current->flags |= NEED_SCHEDULE;
    }

}

extern struct Time time;

void HPET_init() {
    unsigned char *HPET_addr = (unsigned char *) Phy_To_Virt(0xfed00000);

    // get RCBA addr
    io_out32(0xcf8,0x8000f8f0);
    unsigned int x = io_in32(0xcfc);
    x = x & 0xffffc000;

    //get HPTC address
    unsigned int * p;
    if (x > 0xfec00000 && x < 0xfee00000) {
        p = (unsigned int *) Phy_To_Virt(x + 0x3404UL);
    }

    //enable HPET
    *p = 0x80;
    io_mfence();

    struct IO_APIC_RET_entry entry;
    entry.vector = 34;
    entry.deliver_mode = APIC_ICR_IOAPIC_Fixed ;
    entry.dest_mode = ICR_IOAPIC_DELV_PHYSICAL;
    entry.deliver_status = APIC_ICR_IOAPIC_Idle;
    entry.polarity = APIC_IOAPIC_POLARITY_HIGH;
    entry.irr = APIC_IOAPIC_IRR_RESET;
    entry.trigger = APIC_ICR_IOAPIC_Edge;
    entry.mask = APIC_ICR_IOAPIC_Masked;
    entry.reserved = 0;
    entry.destination.physical.reserved1 = 0;
    entry.destination.physical.phy_dest = 0;
    entry.destination.physical.reserved2 = 0;

    register_irq(0x22, &entry, HPET_handler, NULL,
                 &HPET_int_controller, "HPET");

    *(unsigned long *)(HPET_addr + 0x10) = 3;
    io_mfence();

    // edge triggered & periodic
    *(unsigned long *) (HPET_addr + 0x100) = 0x004c;
    io_mfence();

    // 1S
    *(unsigned long *) (HPET_addr + 0x108) = 14318179;
    io_mfence();

    // init MAIN_CNT & get CMOS time
    get_cmos_time(&time);
    *(unsigned long *) (HPET_addr + 0xf0) = 0;
    io_mfence();

    color_printk(RED, BLACK, "%#010x, %#010x, %#010x, %#010x, %#010x, %#010x\n", time.year,
                 time.mon, time.day, time.hour, time.min, time.sec);
}



