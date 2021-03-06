#ifndef _APIC_H
#define _APIC_H

#include "ptrace.h"
#include "linkage.h"
#include "interrupt.h"

struct IO_APIC_map {
    unsigned int phy_addr;
    unsigned char * virtual_idx_addr;
    unsigned int * virtual_data_addr;
    unsigned int * virtual_EOI_addr;
} ioapic_map;


unsigned long ioapic_rte_read(unsigned char idx);
void ioapic_rte_write(unsigned char idx, unsigned long value);

void IOAPIC_pageTable_remap();

void IOAPIC_init();
void APIC_IOAPIC_init();
void LOCAL_APIC_init();

void do_IRQ(struct PT_regs *regs, unsigned long nr);




struct IO_APIC_RET_entry {
    unsigned int 	vector	:8,	//0~7
            deliver_mode	:3,	//8~10
            dest_mode	:1,	//11
            deliver_status	:1,	//12
            polarity:1,	//13
            irr	:1,	//14
            trigger	:1,	//15
            mask	:1,	//16
            reserved:15;	//17~31
    union{
        struct {
            unsigned int reserved1	:24,	//32~55
                    phy_dest	:4,	//56~59
                    reserved2	:4;	//60~63
        } physical;

        struct {
            unsigned int reserved1	:24,	//32~55
                    logical_dest	:8;	    //56~63
        } logical;
    } destination;
}__attribute__((packed));

// delivery mode
#define	APIC_ICR_IOAPIC_Fixed 		 0	//LAPIC	IOAPIC 	ICR
#define	IOAPIC_ICR_Lowest_Priority 	 1	//	IOAPIC 	ICR
#define	APIC_ICR_IOAPIC_SMI		 2	//LAPIC	IOAPIC 	ICR

#define	APIC_ICR_IOAPIC_NMI		 4	//LAPIC	IOAPIC 	ICR
#define	APIC_ICR_IOAPIC_INIT		 5	//LAPIC	IOAPIC 	ICR
#define	ICR_Start_up			 6	//		ICR
#define	IOAPIC_ExtINT			 7	//	IOAPIC

// timer mode
#define APIC_LVT_Timer_One_Shot		0
#define APIC_LVT_Timer_Periodic		1
#define APIC_LVT_Timer_TSC_Deadline	2

// mask
#define APIC_ICR_IOAPIC_Masked		1
#define APIC_ICR_IOAPIC_UN_Masked	0

// trigger mode
#define APIC_ICR_IOAPIC_Edge		0
#define APIC_ICR_IOAPIC_Level		1

// delivery status
#define APIC_ICR_IOAPIC_Idle	    	0
#define APIC_ICR_IOAPIC_Send_Pending	1

// destination shorthand
#define ICR_No_Shorthand	    	0
#define ICR_Self        			1
#define ICR_ALL_INCLUDE_Self		2
#define ICR_ALL_EXCLUDE_Self		3

// destination mode
#define ICR_IOAPIC_DELV_PHYSICAL	0
#define ICR_IOAPIC_DELV_LOGIC		1

//level
#define ICR_LEVEL_DE_ASSERT		0
#define ICR_LEVLE_ASSERT		1

//remote irr
#define APIC_IOAPIC_IRR_RESET		0
#define APIC_IOAPIC_IRR_ACCEPT		1

//pin polarity
#define APIC_IOAPIC_POLARITY_HIGH	0
#define APIC_IOAPIC_POLARITY_LOW	1

void IOAPIC_enable(unsigned long irq);
void IOAPIC_disable(unsigned long irq);
unsigned long IOAPIC_install(unsigned long irq, void *arg);
void IOAPIC_uninstall(unsigned long irq);
void IOAPIC_level_ack(unsigned long irq);
void IOAPIC_edge_ack(unsigned long irq);

#endif

