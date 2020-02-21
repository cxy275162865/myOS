#include "printk.h"
#include "errno.h"

unsigned long no_system_call(void) {
    color_printk(RED, BLACK, "no_system_call is calling\n");
    return -ENOSYS;
}


unsigned long sys_putstring(char *str) {
    color_printk(GREEN, BLACK, "sys_putstring: ");
    color_printk(INDIGO, BLACK, str);
    return 0;
}


