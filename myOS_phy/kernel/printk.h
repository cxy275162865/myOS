#ifndef _PRINTK_H
#define _PRINTK_H

#include <stdarg.h>
#include "font.h"
#include "linkage.h"
#include "UEFI.h"

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED	    0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫


extern unsigned char ASCII[256][16];
extern struct KERNEL_BOOT_PARAMETER_INFORMATION *boot_param;

char buf[4096] = {0, };

struct position {
    int XResolution;
    int YResolution;

    int XPosition;
    int YPosition;

    int XCharSize;
    int YCharSize;

    unsigned int *FB_addr;
    unsigned long FB_length;
} pos;

void putchar(unsigned int *fb, int Xsize, int x, int y, unsigned int FRcolor, unsigned int BKcolor, unsigned char font);

int skip_atoi(const char **s);

#define do_div(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })

char *number(char *str, long num, int base, int size, int precision, int type);

int vsprintf(char *buf, const char *fmt, va_list args);

int color_printk(unsigned int FRcolor, unsigned int BKcolor, const char *fmt, ...);

void frame_buffer_init();

void __stack_chk_fail();

#endif

