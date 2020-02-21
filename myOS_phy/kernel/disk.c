#include "disk.h"
#include "interrupt.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"
#include "lib.h"


void disk_handler() {

}

hw_int_controller disk_int_controller {
    .enable = NULL,
    .disable = NULL,
    .install = NULL,
    .uninstall = NULL,
    .ack = NULL,
};


void disk_init() {
    
}

void disk_exit() {
    unregister_irq(0x2f);
}

