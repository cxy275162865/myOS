#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"
#include "ptrace.h"
#include "cpu.h"
#include "APIC.h"


void IOAPIC_enable(unsigned long irq) {
    unsigned long value = 0;
    value = ioapic_rte_read((irq - 32) * 2 + 0x10);
    value = value & (~0x10000UL);
    ioapic_rte_write((irq - 32) * 2 + 0x10, value);
}

void IOAPIC_disable(unsigned long irq) {
    unsigned long value = 0;
    value = ioapic_rte_read((irq - 32) * 2 + 0x10);
    value = value | 0x10000UL;
    ioapic_rte_write((irq - 32) * 2 + 0x10, value);
}

unsigned long IOAPIC_install(unsigned long irq, void *arg) {
    struct IO_APIC_RET_entry *entry = (struct IO_APIC_RET_entry *) arg;
    ioapic_rte_write((irq - 32) * 2 + 0x10, *(unsigned long *) entry);
    return 1;
}

void IOAPIC_uninstall(unsigned long irq) {
    ioapic_rte_write((irq - 32) * 2 + 0x10, 0x10000UL);
}

void IOAPIC_level_ack(unsigned long irq) {
    __asm__ __volatile__(    "movq	$0x00,	%%rdx	\n\t"
                             "movq	$0x00,	%%rax	\n\t"
                             "movq 	$0x80b,	%%rcx	\n\t"
                             "wrmsr	\n\t"
    :: :"memory");

    *ioapic_map.virtual_EOI_addr = irq;
}

void IOAPIC_edge_ack(unsigned long irq) {
    __asm__ __volatile__(    "movq	$0x00,	%%rdx	\n\t"
                             "movq	$0x00,	%%rax	\n\t"
                             "movq 	$0x80b,	%%rcx	\n\t"
                             "wrmsr	\n\t"
    :: :"memory");
}

unsigned long ioapic_rte_read(unsigned char idx) {
    unsigned long ret = 0;

    *ioapic_map.virtual_idx_addr = idx + 1;
    io_mfence();
    ret = *ioapic_map.virtual_data_addr;
    ret <<= 32;
    io_mfence();

    *ioapic_map.virtual_idx_addr = idx;
    io_mfence();
    ret |= *ioapic_map.virtual_data_addr;
    io_mfence();
    return ret;
}
void ioapic_rte_write(unsigned char idx, unsigned long value) {
    *ioapic_map.virtual_idx_addr = idx;
    io_mfence();
    *ioapic_map.virtual_data_addr = value & 0xffffffff;
    value >>= 32;
    io_mfence();

    *ioapic_map.virtual_idx_addr = idx + 1;
    io_mfence();
    *ioapic_map.virtual_data_addr = value & 0xffffffff;
    io_mfence();
}


void APIC_IOAPIC_init1() {
    for (int i = 32; i < 56; i++) {
        set_intr_gate(i, 2, interrupt[i - 32]);
    }

////     mask 8259A
//    io_out8(0x21, 0xff);
//    io_out8(0xa1, 0xff);
//
//    LOCAL_APIC_init();


    // 8259A-master	ICW1-4
    io_out8(0x20, 0x11);
    io_out8(0x21, 0x20);
    io_out8(0x21, 0x04);
    io_out8(0x21, 0x01);
    // 8259A-slave	ICW1-4
    io_out8(0xa0, 0x11);
    io_out8(0xa1, 0x28);
    io_out8(0xa1, 0x02);
    io_out8(0xa1, 0x01);
    // 8259A-M/S	OCW1
    io_out8(0x21, 0xfd);
    io_out8(0xa1, 0xef);

    memset(interrupt_desc, 0, sizeof(irq_desc_T) * NR_IRQS);

    sti();
}


void APIC_IOAPIC_init() {

    // get ready to access the RF of IO APIC
    IOAPIC_pageTable_remap();

    // init the intr vector table
    for (int i = 32; i < 56; i++) {
        set_intr_gate(i, 0, interrupt[i - 32]);
    }

    // mask 8259A
    io_out8(0x21, 0xff);
    io_out8(0xa1, 0xff);

    // set IMCR to make CPU only handle the intr from APIC
    io_out8(0x22, 0x70);
    io_out8(0x23, 0x01);

    // init local APIC
    LOCAL_APIC_init();

    // init IO APIC
    IOAPIC_init();

    // get RCBA address
    io_out32(0xcf8, 0x8000f8f0);
    unsigned int x = io_in32(0xcfc);
    x &= 0xffffc000;

    // get OIC address
    unsigned int *p;
    if (x > 0xfec00000 && x < 0xfee00000) {
        p = (unsigned int *) Phy_To_Virt(x + 0x31feUL);
    }

    memset(interrupt_desc, 0, sizeof(irq_desc_T) * NR_IRQS);

    // enable IO APIC
    x = (*p & 0xffffff00) | 0x100;
    io_mfence();
    *p = x;
    io_mfence();

    color_printk(BLACK, WHITE, "----\n");
    sti();

}


void LOCAL_APIC_init() {
    unsigned int x, y;
    unsigned int a, b, c, d;

    // check whether support APIC & x2APIC
    get_cpuid(1, 0, &a, &b, &c, &d);

    color_printk(WHITE, BLACK,
         "CPUID\t01,eax:%#010x,ebx:%#010x,ecx:%#010x,edx:%#010x\n", a, b, c, d);

    if ((1<<9) & d)
        color_printk(WHITE,BLACK,"support APIC&xAPIC\t");
    else
        color_printk(WHITE,BLACK,"NO support APIC&xAPIC\t");

    if ((1<<21) & c)
        color_printk(WHITE,BLACK,"support x2APIC\n");
    else
        color_printk(WHITE,BLACK,"NO support x2APIC\n");

    // enable xAPIC & x2APIC
    __asm__ __volatile__ (
            "movq   $0x1b,  %%rcx   \n\t"
            "rdmsr      \n\t"
            "bts    $10,    %%rax   \n\t"
            "bts    $11,    %%rax   \n\t"
            "wrmsr      \n\t"

            "movq   $0x1b,  %%rcx   \n\t"
            "rdmsr      \n\t"
            : "=a"(x), "=d"(y)
            :
            : "memory"
    );
    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\t", x, y);
    if (x & 0xc00) {
        color_printk(WHITE, BLACK, "xAPIC & x2APIC enabled\n");
    }

//     enable SVR[8] [12]
    __asm__ __volatile__ (
            "movq 	$0x80f,	%%rcx	\n\t"
            "rdmsr	\n\t"
            "bts	$8,	    %%rax	\n\t"
            "bts	$12,    %%rax\n\t"
            "wrmsr	\n\t"
            "movq 	$0x80f,	%%rcx	\n\t"
            "rdmsr	\n\t"
            : "=a"(x), "=d"(y)
            :
            : "memory"
    );
    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\t", x, y);
    if (x & 0x100) {
        color_printk(WHITE, BLACK, "SVR[8] enabled\t");
    }
    if (x & 0x1000) {
        color_printk(WHITE, BLACK, "SVR[12] enabled\n");
    }

    // get local APIC ID
    __asm__ __volatile__(
            "movq $0x802,	%%rcx	\n\t"
            "rdmsr	\n\t"
            :"=a"(x),"=d"(y)
            :
            :"memory"
    );
    color_printk(WHITE, BLACK, "eax:%#010x,edx:%#010x\tx2APIC ID:%#010x\n", x, y, x);

    // get local APIC version
    __asm__ __volatile__(
            "movq $0x803,	%%rcx	\n\t"
            "rdmsr	\n\t"
            :"=a"(x),"=d"(y)
            :
            :"memory"
    );
    color_printk(WHITE, BLACK, "local APIC Version:%#010x,Max LVT Entry:%#010x, "
                               "SVR(Suppress EOI Broadcast):%#04x\t",
                 x & 0xff, (x >> 16 & 0xff) + 1, x >> 24 & 0x1);
    if ((x & 0xff) < 0x10) {
        color_printk(WHITE, BLACK, "82489DX discrete APIC\n");
    } else if (((x & 0xff) >= 0x10) && ((x & 0xff) <= 0x15)) {
        color_printk(WHITE, BLACK, "Integrated APIC\n");
    }

//     mask all LVT
    __asm__ __volatile__(
            "movq 	$0x82f,	%%rcx	\n\t"	// CMCI
            "wrmsr	\n\t"
            "movq 	$0x832,	%%rcx	\n\t"	// Timer
            "wrmsr	\n\t"
            "movq 	$0x833,	%%rcx	\n\t"	// Thermal Monitor
            "wrmsr	\n\t"
            "movq 	$0x834,	%%rcx	\n\t"	// Performance Counter
            "wrmsr	\n\t"
            "movq 	$0x835,	%%rcx	\n\t"	// LINT0
            "wrmsr	\n\t"
            "movq 	$0x836,	%%rcx	\n\t"	// LINT1
            "wrmsr	\n\t"
            "movq 	$0x837,	%%rcx	\n\t"	// Error
            "wrmsr	\n\t"
            :
            :"a"(0x10000), "d"(0x00)
            :"memory"
    );
    color_printk(GREEN, BLACK, "Mask ALL LVT\n");

    // TPR
    __asm__ __volatile__(
            "movq 	$0x808,	%%rcx	\n\t"
            "rdmsr	\n\t"
            :"=a"(x), "=d"(y)
            :
            :"memory"
    );
    color_printk(GREEN, BLACK, "Set LVT TPR:%#010x\t", x);

    // PPR
    __asm__ __volatile__(
            "movq 	$0x80a,	%%rcx	\n\t"
            "rdmsr	\n\t"
            :"=a"(x), "=d"(y)
            :
            :"memory"
    );
    color_printk(GREEN, BLACK, "Set LVT PPR:%#010x\n", x);

}


// map the phy addr 0xfec00000 in the page table
void IOAPIC_pageTable_remap() {

    unsigned char * ioapic_addr = (unsigned char *)Phy_To_Virt(0xfec00000);

    ioapic_map.phy_addr = 0xfec00000;
    ioapic_map.virtual_idx_addr = ioapic_addr;
    ioapic_map.virtual_data_addr = (unsigned int *) (ioapic_addr + 0x10);
    ioapic_map.virtual_EOI_addr = (unsigned int *) (ioapic_addr + 0x40);

    global_CR3 = Get_gdt();

    unsigned long * tmp = Phy_To_Virt(global_CR3 +
            (((unsigned long)ioapic_addr >> PAGE_GDT_SHIFT) & 0x1ff));
    if (*tmp == 0) {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_KERNEL_GDT));
    }
    color_printk(YELLOW, BLACK,
            "1:%#018lx\t%#018lx\n", (unsigned long) tmp, (unsigned long) *tmp);

    tmp = Phy_To_Virt((unsigned long *) (*tmp & (~0xfffUL)) +
            (((unsigned long) ioapic_addr >> PAGE_1G_SHIFT) & 0x1ff));
    if (*tmp == 0) {
        unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_KERNEL_Dir));
    }

    color_printk(YELLOW, BLACK, "2:%#018lx\t%#018lx\n",
            (unsigned long) tmp, (unsigned long) *tmp);

    tmp = Phy_To_Virt((unsigned long *) (*tmp & (~0xfffUL)) +
            (((unsigned long) ioapic_addr >> PAGE_2M_SHIFT) & 0x1ff));
    set_pdt(tmp,mk_pdt(ioapic_map.phy_addr,
            PAGE_KERNEL_Page | PAGE_PWT | PAGE_PCD));

    color_printk(BLUE, BLACK, "3:%#018lx\t%#018lx\n",
            (unsigned long) tmp, (unsigned long) *tmp);
    color_printk(BLUE, BLACK, "ioapic_map.physical_address:%#010x\t\t\n",
            ioapic_map.phy_addr);
    color_printk(BLUE, BLACK, "ioapic_map.virtual_address:%#018lx\t\t\n",
                 (unsigned long) ioapic_map.virtual_idx_addr);

    flush_tlb();

}

void IOAPIC_init() {
    // IO APIC ID
    *ioapic_map.virtual_idx_addr = 0x00;
    io_mfence();
    *ioapic_map.virtual_data_addr = 0x0f000000;
    io_mfence();
    color_printk(GREEN, BLACK, "Get IOAPIC ID REG:%#010x,ID:%#010x\n",
            *ioapic_map.virtual_data_addr,
            *ioapic_map.virtual_data_addr >> 24 & 0xf);
    io_mfence();

    // IO APIC version
    *ioapic_map.virtual_idx_addr = 0x01;
    io_mfence();
    color_printk(GREEN, BLACK, "Get IOAPIC Version REG:%#010x,MAX redirection entries:%#08d\n",
                 *ioapic_map.virtual_data_addr,
                 ((*ioapic_map.virtual_data_addr >> 16) & 0xff) + 1);

    // RTE
    for (int i = 0x10; i < 0x40; i += 2) {
        ioapic_rte_write(i, 0x10020 + ((i - 0x10) >> 1));
    }

    // enable the keyboard interrupt
    ioapic_rte_write(0x12, 0x21);

}


void do_IRQ(struct PT_regs *regs, unsigned long nr) {
//    if (nr != 0x21)
//        color_printk(RED, BLACK, "(IRQ: %#04x)\t", nr);

    irq_desc_T *p = &interrupt_desc[nr - 32];

    if (p->handler != NULL) {
        p->handler(nr, p->param, regs);
        if (p->controller->ack != NULL) {
            p->controller->ack(nr);
        }
    } else {
        color_printk(RED, BLACK, "ERR: p->handler == NULL\n");
    }

    // EOI register: MSR 0x80b
    __asm__ __volatile__(
            "movq	$0x00,	%%rdx	\n\t"
            "movq	$0x00,	%%rax	\n\t"
            "movq 	$0x80b,	%%rcx	\n\t"
            "wrmsr	\n\t"
    :::"memory");

    // io_out8(0x20, 0x20);   // EOI
}


