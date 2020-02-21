#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "interrupt.h"
#include "task.h"
#include "UEFI.h"

#if APIC
    #include "APIC.h"
#else
    #include "8259A.h"
#endif

#include "keyboard.h"
#include "mouse.h"
#include "time.h"
#include "timer.h"
#include "HPET.h"
#include "softirq.h"


struct Global_Memory_Descriptor globalMemoryDesc = {{0}, 0};
struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_param =
        (struct KERNEL_BOOT_PARAMETER_INFORMATION *) 0xffff800000060000;

void start_kernel() {

    memset((void *) &_bss, 0, (unsigned long) &_ebss - (unsigned long) &_bss);

    pos.XResolution = boot_param->graphics_info.horizontalResolution;
    pos.YResolution = boot_param->graphics_info.verticalResolution;
    pos.XPosition = 0;
    pos.YPosition = 0;
    pos.XCharSize = 8;
    pos.YCharSize = 16;
    pos.FB_addr = (int *) Phy_To_Virt(0x4a00000);
    pos.FB_length = (pos.XResolution * pos.YResolution * 4 +
                     PAGE_4K_SIZE - 1) & PAGE_4K_MASK;
    memset((void *) pos.FB_addr, 0, boot_param->graphics_info.frameBufferSize);

    load_TR(10);
    set_tss64(_stack_start, _stack_start,
              _stack_start, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00,
              0xffff800000007c00, 0xffff800000007c00);
    sys_vector_init();

    init_cpu();

    globalMemoryDesc.start_code = (unsigned long) &_text;
    globalMemoryDesc.end_code   = (unsigned long) &_etext;
    globalMemoryDesc.end_data   = (unsigned long) &_edata;
    globalMemoryDesc.end_brk    = (unsigned long) &_end;


    color_printk(GREEN, BLACK, "memory init\t");
    init_memory();
    color_printk(GREEN, BLACK, "slab init\t");
    slab_init();
    color_printk(GREEN, BLACK, "frame buffer init\t");
    frame_buffer_init();
    color_printk(GREEN, BLACK, "page table init \n");
    pageTable_init();

    color_printk(GREEN, BLACK, "task initial\n");
    task_init();


#if APIC
    color_printk(GREEN, BLACK, "APIC init \n");
    APIC_IOAPIC_init();
#else
    color_printk(GREEN, BLACK, "8259A init \n");
    init_8259A();
#endif

//    color_printk(GREEN, BLACK, "softirq init\t");
//    softirq_init();
//
//    color_printk(GREEN, BLACK, "timer & clock init \n");
//    timer_init();
//    HPET_init();


    color_printk(GREEN, BLACK, "keyboard init\n");
    keyboard_init();
//    color_printk(GREEN, BLACK, "mouse init \n");
//    mouse_init();


//    color_printk(GREEN, BLACK, "task initial\n");
//    task_init();
    while (1) {
        if (p_kb->cnt)
            analyse_keycode();
//        if (p_mouse->cnt)
//            analyse_mouse_code();
    }
}











void page_test() {
    struct Page *page = NULL;
    struct Slab * slab = NULL;

    page = alloc_pages(ZONE_NORMAL, 63, 0);
    page = alloc_pages(ZONE_NORMAL, 63, 0);

    color_printk(ORANGE, BLACK,
                 "4.globalMemoryDesc.bits_map:%#018lx\tglobalMemoryDesc"
                 ".bits_map+1:%#018lx\tglobalMemoryDesc.bits_"
                 "map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct"
                 "->page_free_count:%d\n",
                 *globalMemoryDesc.bits_map, *(globalMemoryDesc.bits_map + 1),
                 *(globalMemoryDesc.bits_map + 2),
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);

    for (int i = 80; i <= 85; i++) {
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
        i++;
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
    }
    for (int i = 140; i <= 145; i++) {
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
        i++;
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
    }
    free_pages(page, 1);
    color_printk(ORANGE, BLACK,
                 "5.globalMemoryDesc.bits_map:%#018lx\tglobalMemoryDesc"
                 ".bits_map+1:%#018lx\tglobalMemoryDesc.bits_"
                 "map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct"
                 "->page_free_count:%d\n",
                 *globalMemoryDesc.bits_map, *(globalMemoryDesc.bits_map + 1),
                 *(globalMemoryDesc.bits_map + 2),
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);

    for (int i = 75; i <= 85; i++) {
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\t", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
        i++;
        color_printk(INDIGO, BLACK, "page%03d attribute:%#018lx address:%#018lx\n", i,
                     (globalMemoryDesc.pages_struct + i)->attribute,
                     (globalMemoryDesc.pages_struct + i)->PHY_address);
    }
    page = alloc_pages(ZONE_UNMAPED, 63, 0);
    color_printk(ORANGE, BLACK,
                 "6.globalMemoryDesc.bits_map:%#018lx\tglobalMemoryDesc"
                 ".bits_map+1:%#018lx\tglobalMemoryDesc.bits_"
                 "map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct"
                 "->page_free_count:%d\n",
                 *globalMemoryDesc.bits_map, *(globalMemoryDesc.bits_map + 1),
                 *(globalMemoryDesc.bits_map + 2),
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);
    free_pages(page, 1);

    color_printk(ORANGE, BLACK,
                 "7.globalMemoryDesc.bits_map:%#018lx\tglobalMemoryDesc"
                 ".bits_map+1:%#018lx\tglobalMemoryDesc.bits_"
                 "map+2:%#018lx\tzone_struct->page_using_count:%d\tzone_struct"
                 "->page_free_count:%d\n",
                 *globalMemoryDesc.bits_map, *(globalMemoryDesc.bits_map + 1),
                 *(globalMemoryDesc.bits_map + 2),
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);
}


void paint() {

    int *addr = (int *) 0xffff800003000000;
    int i;
    for (i = 0; i < 1920 * 32; i++) {
        *((char *) addr + 0) = (char) 0x00;
        *((char *) addr + 1) = (char) 0x00;
        *((char *) addr + 2) = (char) 0xff;
        *((char *) addr + 3) = (char) 0x00;
        addr += 1;
    }
    for (i = 0; i < 1920 * 32; i++) {
        *((char *) addr + 0) = (char) 0x00;
        *((char *) addr + 1) = (char) 0xff;
        *((char *) addr + 2) = (char) 0x00;
        *((char *) addr + 3) = (char) 0x00;
        addr += 1;
    }
    for (i = 0; i < 1920 * 32; i++) {
        *((char *) addr + 0) = (char) 0xff;
        *((char *) addr + 1) = (char) 0x00;
        *((char *) addr + 2) = (char) 0x00;
        *((char *) addr + 3) = (char) 0x00;
        addr += 1;
    }
    for (i = 0; i < 1920 * 32; i++) {
        *((char *) addr + 0) = (char) 0xff;
        *((char *) addr + 1) = (char) 0xff;
        *((char *) addr + 2) = (char) 0xff;
        *((char *) addr + 3) = (char) 0x00;
        addr += 1;
    }
}

