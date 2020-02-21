#ifndef _LIB_H
#define _LIB_H


#define NULL 0

// 复合语句表达式
#define CONTAINER_OF(ptr, type, member)     ({  \
    typeof(((type *)0) -> member) * p = (ptr);  \
	(type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));		\
})

#define sti() 		__asm__ __volatile__ ("sti	\n\t":::"memory")
#define cli()	 	__asm__ __volatile__ ("cli	\n\t":::"memory")
#define nop() 		__asm__ __volatile__ ("nop	\n\t")
#define io_mfence() 	__asm__ __volatile__ ("mfence	\n\t":::"memory")


// 双向链表
struct List {
    struct List *prev;
    struct List *next;
};

void list_init(struct List *list) {
    list->next = list;
    list->prev = list;
}

void list_add_to_next(struct List *entry, struct List *new1) {
    new1->next = entry->next;
    new1->prev = entry;
    entry->next = new1;
    new1->next->prev = new1;
}
void list_add_to_before(struct List *entry, struct List *new1) {
    new1->next = entry;
    new1->prev = entry->prev;
    entry->prev->next = new1;
    entry->prev = new1;
}
void list_del(struct List *l) {
    l->prev->next = l->next;
    l->next->prev = l->prev;
}
long list_isEmpty(struct List *l) {
    if (l->prev == l && l->next == l)
        return 1UL;
    return 0UL;
}
struct List* list_prev(struct List *l) {
    if (l->prev != NULL)
        return l->prev;
    return NULL;
}
struct List* list_next(struct List *l) {
    if (l->next != NULL)
        return l->next;
    return NULL;
}

void * memset(void *Address, unsigned char C, long Count) {
    int d0, d1;
    unsigned long tmp = C * 0x0101010101010101UL;
    __asm__ __volatile__    (    "cld	\n\t"
                                 "rep	\n\t"
                                 "stosq	\n\t"
                                 "testb	$4, %b3	\n\t"
                                 "je	1f	\n\t"
                                 "stosl	\n\t"
                                 "1:\ttestb	$2, %b3	\n\t"
                                 "je	2f\n\t"
                                 "stosw	\n\t"
                                 "2:\ttestb	$1, %b3	\n\t"
                                 "je	3f	\n\t"
                                 "stosb	\n\t"
                                 "3:	\n\t"
    :"=&c"(d0), "=&D"(d1)
    :"a"(tmp), "q"(Count), "0"(Count / 8), "1"(Address)
    :"memory"
    );
    return Address;
}

void * memcpy(void *from, void *to, long num) {
    int d0, d1, d2;
    __asm__ __volatile__ (
            "cld	\n\t"
            "rep	\n\t"
            "movsq	\n\t"
            "testb	$4, %b4	\n\t"
            "je	1f	\n\t"
            "movsl	\n\t"
            "1:\ttestb	$2, %b4	\n\t"
            "je	2f	\n\t"
            "movsw	\n\t"
            "2:\ttestb	$1, %b4	\n\t"
            "je	3f	\n\t"
            "movsb	\n\t"
            "3:	\n\t"
            :"=&c"(d0), "=&D"(d1), "=&S"(d2)
            :"0"(num / 8), "q"(num), "1"(to), "2"(from)
            :"memory"
            );
    return to;
}

char *strcpy(char *Dest, char *Src) {
    __asm__ __volatile__    (    "cld	\n\t"
                                 "1:	\n\t"
                                 "lodsb	\n\t"
                                 "stosb	\n\t"
                                 "testb	%%al,	%%al	\n\t"
                                 "jne	1b	\n\t"
    :
    :"S"(Src), "D"(Dest)
    :
    );
    return Dest;
}


int strlen(char *String) {
    register int __res;
    __asm__ __volatile__    (    "cld	\n\t"
                                 "repne	\n\t"
                                 "scasb	\n\t"
                                 "notl	%0	\n\t"
                                 "decl	%0	\n\t"
    :"=c"(__res)
    :"D"(String), "a"(0), "0"(0xffffffff)
    :
    );
    return __res;
}


unsigned char io_in8(unsigned short port) {
    unsigned char ret = 0;
    __asm__ __volatile__(    "inb	%%dx,	%0	\n\t"
                             "mfence			\n\t"
    :"=a"(ret)
    :"d"(port)
    :"memory");
    return ret;
}


unsigned int io_in32(unsigned short port) {
    unsigned int ret = 0;
    __asm__ __volatile__(
            "inl	%%dx,	%0	\n\t"
            "mfence			\n\t"
            :"=a"(ret)
            :"d"(port)
            :"memory");
    return ret;
}


void io_out8(unsigned short port, unsigned char value) {
    __asm__ __volatile__(
            "outb	%0,	%%dx	\n\t"
            "mfence			\n\t"
            :
            :"a"(value), "d"(port)
            :"memory");
}


void io_out32(unsigned short port, unsigned int value) {
    __asm__ __volatile__(
            "outl	%0,	%%dx	\n\t"
            "mfence			\n\t"
            :
            :"a"(value), "d"(port)
            :"memory");
}

#define port_insw(port, buffer, nr)	\
__asm__ __volatile__("cld; rep; insw; mfence;"::"d"(port),"D"(buffer),"c"(nr):"memory")

#define port_outsw(port, buffer, nr)	\
__asm__ __volatile__("cld; rep; outsw; mfence;"::"d"(port),"S"(buffer),"c"(nr):"memory")


void wrmsr(unsigned long addr, unsigned long value) {
    __asm__ __volatile__ (
            "wrmsr      \n\t"
            :
            :"d"(value >> 32), "a"(value & 0xffffffff), "c"(addr)
            :"memory"
            );
}


long verify_area(unsigned char *addr,
        unsigned long size) {
    if ((unsigned long) addr + size
            <= (unsigned long) 0x00007fffffffffff)
        return 1;
    return 0;
}

long copy_from_user(void *from, void *to, unsigned long size) {
    if (!verify_area(from, size)) {
        return 0;
    }

}


#endif
