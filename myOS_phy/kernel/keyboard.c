#include "keyboard.h"
#include "lib.h"
#include "interrupt.h"
#include "APIC.h"
#include "memory.h"
#include "printk.h"


struct Keyboard_input_buffer *p_kb = NULL;
static int shift_l, shift_r, ctrl_l, ctrl_r, alt_l, alt_r;

hw_int_controller keyboard_int_controller = {
        .enable = IOAPIC_enable,
        .disable = IOAPIC_disable,
        .install = IOAPIC_install,
        .uninstall = IOAPIC_uninstall,
        .ack = IOAPIC_edge_ack,
};


void keyboard_handler(unsigned long nr,
        unsigned long param, struct PT_regs * regs) {

    unsigned char c;
    c = io_in8(PORT_KB_DATA);       // 0x60

//    color_printk(WHITE, BLACK, "(K: %02x) ", c);

    if (p_kb->p_head == p_kb->buf + KB_BUF_SIZE) {
        p_kb->p_head = p_kb->buf;
    }

    *(p_kb->p_head) = c;
    p_kb->p_head++;
    p_kb->cnt++;

}

void keyboard_init() {

    p_kb = (struct Keyboard_input_buffer *)
            kmalloc(sizeof(struct Keyboard_input_buffer), 0);
    p_kb->p_head = p_kb->buf;
    p_kb->p_tail = p_kb->buf;
    p_kb->cnt = 0;
    memset(p_kb->buf, 0, KB_BUF_SIZE);

    struct IO_APIC_RET_entry entry;
    entry.vector = 0x21;
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

    wait_KB_write();
    io_out8(PORT_KB_CMD, KBCMD_WR_CMD);
    wait_KB_write();
    io_out8(PORT_KB_DATA, KB_INIT_MODE);

    for (int i = 0; i<1000; i++)
        for(int j = 0; j<1000; j++)
                nop();

    shift_l = 0;
    shift_r = 0;
    alt_l = 0;
    alt_r = 0;
    ctrl_l = 0;
    ctrl_r = 0;


    register_irq(0x21, &entry, keyboard_handler, (unsigned long) p_kb,
            &keyboard_int_controller, "ps/2 keyboard");

}

void keyboard_exit() {
    unregister_irq(0x21);
    kfree((unsigned long *) p_kb);
}



void analyse_keycode() {
    unsigned char x = 0;
    x = get_scancode();
//    color_printk(WHITE, BLACK, "%02x)", x);

    int key = 0;
    int make = 0;           // press or break

    if (x == 0xE1) {                   // pause break
        key = PAUSE_BREAK;
        for (int i = 0; i < 6; i++) {
            if (get_scancode() != pausebreak_scode[i]) {
                key = 0;
                break;
            }
        }

    } else if (x == 0xE0) {            // print screen
        x = get_scancode();

        switch (x) {
            case 0x2A: // press printScreen
                if (get_scancode() == 0xE0) {
                    if (get_scancode() == 0x37) {
                        key = PRINT_SCREEN;
                        make = 1;
                    }
                }
                break;

            case 0xB7: // UNpress printScreen
                if (get_scancode() == 0xE0) {
                    if (get_scancode() == 0xAA) {
                        key = PRINT_SCREEN;
                        make = 0;
                    }
                }
                break;

            case 0x1d: // press right ctrl
                ctrl_r = 1;
                key = OTHER_KEY;
                break;

            case 0x9d: // UNpress right ctrl
                ctrl_r = 0;
                key = OTHER_KEY;
                break;

            case 0x38: // press right alt
                alt_r = 1;
                key = OTHER_KEY;
                break;

            case 0xb8: // UNpress right alt
                alt_r = 0;
                key = OTHER_KEY;
                break;

            default:

                // not completed
                key = OTHER_KEY;
                break;
        }
    }

    if (key == 0) {
        unsigned int * keyRow = NULL;
        int column = 0;

        make = (x & FLAG_BREAK) ? 0 : 1;
        keyRow = &keycode_map_normal[(x & 0x7F) * MAP_COLS];

        if (shift_l || shift_r)
            column = 1;

        key = keyRow[column];

        switch (x & 0x7F) {
            case 0x2a:	// SHIFT_L:
                shift_l = make;
                key = 0;
                break;

            case 0x36:	// SHIFT_R:
                shift_r = make;
                key = 0;
                break;

            case 0x1d:	// CTRL_L:
                ctrl_l = make;
                key = 0;
                break;

            case 0x38:	// ALT_L:
                alt_l = make;
                key = 0;
                break;

            default:
                if (!make)
                    key = 0;
        }

        if (key)
            color_printk(INDIGO, BLACK, "%c", key);
    }

}


unsigned char get_scancode() {

    unsigned char ret;
    if (p_kb->cnt == 0) {
        while (!p_kb->cnt) {
            nop();
        }
    }

    if (p_kb->p_tail == p_kb->buf + KB_BUF_SIZE)
        p_kb->p_tail = p_kb->buf;

    ret = *p_kb->p_tail;
    p_kb->cnt--;
    p_kb->p_tail++;

    return ret;
}



