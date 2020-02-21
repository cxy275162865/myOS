CFLAGS := -mcmodel=large -fno-builtin -m64
PIC := APIC

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

system:	head.o entry.o main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o time.o HPET.o timer.o softirq.o schedule.o syscalls.o sys.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o main.o printk.o trap.o memory.o interrupt.o PIC.o task.o cpu.o keyboard.o mouse.o time.o HPET.o timer.o softirq.o schedule.o syscalls.o sys.o -T Kernel.lds

head.o:	head.S
	gcc -E  head.S > head.s
	as --64 -o head.o head.s

entry.o: entry.S
	gcc -E  entry.S > entry.s
	as --64 -o entry.o entry.s

main.o:	main.c
	gcc $(CFLAGS) -c main.c -D$(PIC)
	
printk.o: printk.c
	gcc $(CFLAGS) -c printk.c

trap.o: trap.c
	gcc $(CFLAGS) -c trap.c

memory.o: memory.c
	gcc $(CFLAGS) -c memory.c

interrupt.o: interrupt.c
	gcc $(CFLAGS) -c interrupt.c



ifeq ($(PIC), APIC)
PIC.o: APIC.c
	gcc $(CFLAGS) -c APIC.c -o PIC.o
else
PIC.o: 8259A.c
	gcc $(CFLAGS) -c 8259A.c -o PIC.o
endif



task.o: task.c
	gcc $(CFLAGS) -c task.c

cpu.o: cpu.c
	gcc $(CFLAGS) -c cpu.c

keyboard.o: keyboard.c
	gcc $(CFLAGS) -c keyboard.c

mouse.o: mouse.c
	gcc $(CFLAGS) -c mouse.c

time.o: time.c
	gcc $(CFLAGS) -c time.c

HPET.o: HPET.c
	gcc $(CFLAGS) -c HPET.c

softirq.o: softirq.c
	gcc $(CFLAGS) -c softirq.c

timer.o: timer.c
	gcc $(CFLAGS) -c timer.c

schedule.o: schedule.c
	gcc $(CFLAGS) -c schedule.c

syscalls.o: syscalls.c
	gcc $(CFLAGS) -c syscalls.c

sys.o: sys.c
	gcc $(CFLAGS) -c sys.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin


