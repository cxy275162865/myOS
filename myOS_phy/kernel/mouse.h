#ifndef _MOUSE_H
#define _MOUSE_H

#define KBCMD_SEND_TO_MOUSE     0xd4
#define MOUSE_ENABLE            0xf4
#define KBCMD_EN_MOUSE_INTFACE  0xa8


extern struct Keyboard_input_buffer * p_mouse;

struct Mouse_packet {

    unsigned char byte0;
    // 7:Y overflow, 6:X overflow, 5:Y sign bit, 4:X sign bit,
    // 3:Always, 2:Middle Btn, 1:Right Btn, 0:Left Btn

    char byte1;
    char byte2;
};

struct Mouse_packet mousePacket;


void analyse_mouse_code();

void mouse_init();
void mouse_exit();


#endif

