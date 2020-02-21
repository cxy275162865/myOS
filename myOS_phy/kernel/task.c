#include "task.h"
#include "memory.h"
#include "cpu.h"
#include "ptrace.h"
#include "gate.h"
#include "unistd.h"


extern void ret_system_call(void);
extern void system_call(void);


unsigned long init(unsigned long arg) {
    color_printk(WHITE, BLACK, "init task is running, arg = %#018lx\n", arg);
    struct PT_regs *regs;
    current->thread->rip = (unsigned long) ret_system_call;
    current->thread->rsp = (unsigned long) current + STACK_SIZE - sizeof(struct PT_regs);

    regs = (struct PT_regs *) ((unsigned long) current + STACK_SIZE - sizeof(struct PT_regs));

    __asm__ __volatile__ (
            "movq   %1, %%rsp   \n\t"
            "pushq  %2          \n\t"
            "jmp    do_execve   \n\t"
            :
            :"D"(regs), "m"(current->thread->rsp), "m"(current->thread->rip)
            :"memory"
    );
    return 1;
}



// system call starts HERE!!!!!!

void user_level_func() {
    int errno = 0;
    color_printk(WHITE, BLACK, "NOW: in user_level_function\n");

    char str[] = "\nThis is the first SYSTEM CALL: sys_printf\n\n\n";

    __asm__ __volatile__ (
            "pushq      %%r10   \n\t"
            "pushq      %%r11   \n\t"

            "leaq   sysexit_ret_addr(%%rip),    %%r10       \n\t"
            "movq   %%rsp,  %%r11   \n\t"
            "sysenter           \n\t"

            "sysexit_ret_addr:  \n\t"
            "xchgq      %%rdx,  %%r10   \n\t"
            "xchgq      %%rcx,  %%r11   \n\t"
            "popq       %%r11   \n\t"
            "popq       %%r10   \n\t"
            : "=a"(errno)
            : "0"(__NR_putstring), "D"(str)
                // 0: system_call NR, could be rax, rbx or whatever.
                // remember to modify the system_call_function.
            : "memory"
    );
    color_printk(WHITE, BLACK, "back to user_level_func, errno = %d\n", errno);

    while (1) ;
}

//void cxy() {
//    color_printk(BLUE, BLACK, "cxyyyyy");
//    while (1) ;
//}


unsigned long do_execve(struct PT_regs *regs) {

    regs->r10 = (unsigned long) 0x8000000;   // rdx is related to the inst sysexit: entry of user_level_func
    regs->r11 = (unsigned long) 0x8200000;   // rcx is related to the inst sysexit: stack of user_level_func
//    regs->r10 = (unsigned long) user_level_func;
//    regs->r11 = (unsigned long) current->thread->rsp;
    regs->rax = 1;
    regs->ds = 0;
    regs->es = 0;


    unsigned long addr = 0x8000000;
    unsigned long * tmp;
    unsigned long * virtual = NULL;
    struct Page * p = NULL;
    global_CR3 = Get_gdt();
    tmp = Phy_To_Virt((unsigned long *)((unsigned long)global_CR3 &
            (~ 0xfffUL)) + ((addr >> PAGE_GDT_SHIFT) & 0x1ff));

//    virtual = kmalloc(PAGE_4K_SIZE, 0);
//    set_mpl4t(tmp, mk_mpl4t(Virt_To_Phy(virtual), PAGE_USER_GDT));

    set_mpl4t(tmp, 0x102007);

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~ 0xfffUL)) +
            ((addr >> PAGE_1G_SHIFT) & 0x1ff));

    if (*tmp == 0) {
        virtual = kmalloc(PAGE_4K_SIZE, 0);
        set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_USER_Dir));
    }

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~ 0xfffUL)) +
            ((addr >> PAGE_2M_SHIFT) & 0x1ff));
    p = alloc_pages(ZONE_NORMAL, 1, PG_PTable_Mapped);
    set_pdt(tmp, mk_pdt(p->PHY_address, PAGE_USER_Page));
    color_printk(INDIGO, BLACK, "p->phy_addr=%#018lx\n", p->PHY_address);

    flush_tlb();

    if (!(current->flags & PF_KTHREAD))
        current->addr_limit = 0xffff800000000000;

    memcpy(&user_level_func, (void *) 0x8000000, 1024);

    addr = 0x8000000;
    global_CR3 = Get_gdt();
    tmp = Phy_To_Virt((unsigned long *)((unsigned long)global_CR3 &
                (~ 0xfffUL)) + ((addr >> PAGE_GDT_SHIFT) & 0x1ff));
    color_printk(YELLOW, BLACK, "0: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~ 0xfffUL)) +
              ((addr >> PAGE_1G_SHIFT) & 0x1ff));
    color_printk(YELLOW, BLACK, "1: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    tmp = Phy_To_Virt((unsigned long *)(*tmp & (~ 0xfffUL)) +
              ((addr >> PAGE_2M_SHIFT) & 0x1ff));
    color_printk(YELLOW, BLACK, "2: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);
    color_printk(WHITE, BLACK, "do_execve is done\n");
    return 0;
}

unsigned long do_exit(unsigned long code) {
    color_printk(RED, BLACK, "exit task is running,arg:%#018lx\n", code);
    while (1);
}


unsigned long do_fork(struct PT_regs *regs, unsigned long clone_flags,
                      unsigned long stack_start, unsigned long stack_size) {
    struct Task_struct *tsk = NULL;
    struct Thread_struct *thd = NULL;
    struct Page *p = NULL;

    color_printk(WHITE, BLACK, "alloc_pages, bitmap:%#018lx\n", *globalMemoryDesc.bits_map);
    p = alloc_pages(ZONE_NORMAL, 1, PG_PTable_Mapped | PG_Kernel);
    color_printk(WHITE, BLACK, "alloc_pages, bitmap:%#018lx\n", *globalMemoryDesc.bits_map);

    tsk = (struct Task_struct *) Phy_To_Virt(p->PHY_address);
    memset(tsk, 0, sizeof(*tsk));
    color_printk(WHITE, BLACK, "struct Task_struct address:%#018lx\n", (unsigned long) tsk);

    *tsk = *current;

    list_init(&tsk->list);
    list_add_to_before(&init_task_union.task.list, &tsk->list);
    tsk->pid++;
    tsk->state = TASK_UNINTERRUPTIBLE;

    thd = (struct Thread_struct *) (tsk + 1);
    memset(thd, 0x00, sizeof(struct Thread_struct));
    tsk->thread = thd;

    memcpy(regs, (void *) ((unsigned long) tsk + STACK_SIZE - sizeof(struct PT_regs)),
            sizeof(struct PT_regs));

    thd->rsp0 = (unsigned long) tsk + STACK_SIZE;       // 栈底, 倒着的
    thd->rip = regs->rip;
    thd->rsp = (unsigned long) tsk + STACK_SIZE - sizeof(struct PT_regs);   // tos
    thd->fs = KERNEL_DS;
    thd->gs = KERNEL_DS;

    if (!(tsk->flags & PF_KTHREAD)) {
        thd->rip = regs->rip = (unsigned long) ret_system_call;
    }

    tsk->state = TASK_RUNNING;

    return 1;
}



// 传参用rdi
extern void kernel_thread_func(void);
//void kernel_thread_func(void) {
__asm__ (
    "kernel_thread_func:	\n\t"
    "	popq	%r15	\n\t"
    "	popq	%r14	\n\t"
    "	popq	%r13	\n\t"
    "	popq	%r12	\n\t"
    "	popq	%r11	\n\t"
    "	popq	%r10	\n\t"
    "	popq	%r9	    \n\t"
    "	popq	%r8	    \n\t"
    "	popq	%rbx	\n\t"
    "	popq	%rcx	\n\t"
    "	popq	%rdx	\n\t"
    "	popq	%rsi	\n\t"
    "	popq	%rdi	\n\t"
    "	popq	%rbp	\n\t"
    "	popq	%rax	\n\t"
    "	movq	%rax,	%ds	\n\t"
    "	popq	%rax		\n\t"
    "	movq	%rax,	%es	\n\t"
    "	popq	%rax		\n\t"
    "	addq	$0x38,	%rsp	\n\t"

    "	movq	%rdx,	%rdi	\n\t"
    "	callq	*%rbx		    \n\t"
    "	movq	%rax,	%rdi	\n\t"       // rax is the @return of callq *%rbx above
    "	callq	do_exit		    \n\t"
);



// specify the PT_reg

int kernel_thread(unsigned long (*fn)(unsigned long), unsigned long arg,
        unsigned long flags) {

    struct PT_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.rbx = (unsigned long) fn;
    regs.rdx = (unsigned long) arg;

    regs.ds = KERNEL_DS;
    regs.es = KERNEL_DS;
    regs.cs = KERNEL_CS;
    regs.ss = KERNEL_DS;
    regs.rflags = 1 << 9;
    regs.rip = (unsigned long) kernel_thread_func;
//    regs.rip = (unsigned long) cxy;

    return do_fork(&regs, flags, 0, 0);
}


void __switch_to(struct Task_struct *prev, struct Task_struct *next) {
    init_tss[0].rsp0 = next->thread->rsp0;

    set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2,
              init_tss[0].ist1, init_tss[0].ist2,
              init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5,
              init_tss[0].ist6, init_tss[0].ist7);

    __asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));
    __asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

    __asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));
    __asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

    color_printk(WHITE, BLACK, "prev->thread->rsp0:%#018lx\n", prev->thread->rsp0);
    color_printk(WHITE, BLACK, "next->thread->rsp0:%#018lx\n", next->thread->rsp0);

}


void task_init() {

    init_mm.pgd = (pml4t_t *) global_CR3;
    init_mm.start_code = globalMemoryDesc.start_code;
    init_mm.end_code = globalMemoryDesc.end_code;
    init_mm.start_data = (unsigned long) &_data;
    init_mm.end_data = globalMemoryDesc.end_data;
    init_mm.start_rodata = (unsigned long) &_rodata;
    init_mm.end_rodata = (unsigned long) &_erodata;
    init_mm.start_brk = 0;
    init_mm.end_brk = globalMemoryDesc.end_brk;
    init_mm.start_stack = _stack_start;

    // CS after calling inst: sysenter
    wrmsr(0x174, KERNEL_CS);
    // RSP after calling inst: sysenter
    wrmsr(0x175, current->thread->rsp0);
    // RIP after calling inst: sysenter
    wrmsr(0x176, (unsigned long) system_call);  // head.S

    set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2,
              init_tss[0].ist1, init_tss[0].ist2,
              init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5,
              init_tss[0].ist6, init_tss[0].ist7);

    init_tss[0].rsp0 = init_thread.rsp0;

    list_init(&init_task_union.task.list);

    kernel_thread(init, 88, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

    init_task_union.task.state = TASK_RUNNING;

    struct Task_struct *p = CONTAINER_OF(list_next(&current->list),
            struct Task_struct, list);

    color_printk(WHITE, BLACK, "SWITCH!\n");
    switch_to(current, p);

}

