#include "mouse.h"
#include "keyboard.h"
#include "lib.h"
#include "interrupt.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"

struct Keyboard_input_buffer * p_mouse = NULL;
static int mouse_cnt = 0;


void mouse_handler(unsigned long nr, unsigned long param,
        struct PT_regs * regs) {

    unsigned char c;
    c = io_in8(PORT_KB_DATA);

    if (p_mouse->p_head == p_mouse->buf + KB_BUF_SIZE) {
        p_mouse->p_head = p_mouse->buf;
    }

    *(p_mouse->p_head) = c;
    p_mouse->cnt++;
    p_mouse->p_head++;
}


unsigned char get_mouse_code() {
    unsigned char ret = 0;

    if (p_mouse->cnt == 0) {
        while (!p_mouse->cnt) {
            nop();
            color_printk(GREEN, BLACK, "nop");
        }
    }

    if (p_mouse->p_tail == p_mouse->buf + KB_BUF_SIZE)
        p_mouse->p_tail = p_mouse->buf;

    ret = *(p_mouse->p_tail);
    p_mouse->cnt--;
    p_mouse->p_tail++;

    return ret;
}

void analyse_mouse_code() {
    unsigned char x = get_mouse_code();
    color_printk(GREEN, BLACK, "analyse mouse: %02x\n", x);

    switch (mouse_cnt) {
        case 0:
            // the byte from controller when init,
            // pass
            mouse_cnt++;
            break;
        case 1:
            mousePacket.byte0 = x;
            mouse_cnt++;
            break;
        case 2:
            mousePacket.byte1 = x;
            mouse_cnt++;
        case 3:
            mousePacket.byte2 = x;
            mouse_cnt = 1;
            color_printk(YELLOW, BLACK, "(Mouse: %02x, X: %3d, Y: %3d)\n",
                         mousePacket.byte0, mousePacket.byte1, mousePacket.byte2);
            break;

        default:
            break;
    }
}

hw_int_controller mouse_int_controller = {
        .enable = IOAPIC_enable,
        .disable = IOAPIC_disable,
        .install = IOAPIC_install,
        .uninstall = IOAPIC_uninstall,
        .ack = IOAPIC_edge_ack,
};

void mouse_init() {
    p_mouse = (struct Keyboard_input_buffer *) kmalloc(
            sizeof(struct Keyboard_input_buffer), 0);
    p_mouse->p_head = p_mouse->buf;
    p_mouse->p_tail = p_mouse->buf;
    p_mouse->cnt = 0;
    memset(p_mouse->buf, 0, KB_BUF_SIZE);

    struct IO_APIC_RET_entry entry;
    entry.vector = 0x2c;
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

    mouse_cnt = 0;

    register_irq(0x2c, &entry, mouse_handler, (unsigned long)p_mouse,
            &mouse_int_controller, "ps/2 mouse");

    wait_KB_write();
    io_out8(PORT_KB_CMD, KBCMD_EN_MOUSE_INTFACE);

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            nop();
        }
    }

    // the kb cannot work with the code below
    wait_KB_write();
    io_out8(PORT_KB_CMD, KBCMD_SEND_TO_MOUSE);
    wait_KB_write();
    io_out8(PORT_KB_DATA, MOUSE_ENABLE);

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            nop();
        }
    }
    wait_KB_write();
    io_out8(PORT_KB_CMD, KBCMD_WR_CMD);
    wait_KB_write();
    io_out8(PORT_KB_DATA, KB_INIT_MODE);

}

void mouse_exit() {
    unregister_irq(0x2c);
    kfree((unsigned long *) p_mouse);
}
