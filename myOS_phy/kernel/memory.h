#ifndef _MEMORY_H
#define _MEMORY_H

#include "printk.h"
#include "lib.h"

#define PTRS_PER_PAGE	512

#define SIZEOF_LONG_ALIGN(size) ((size + sizeof(long) - 1) & ~(sizeof(long)-1))
#define SIZEOF_INT_ALIGN(size)  ((size + sizeof(int) - 1) & ~(sizeof(int)-1))

#define PAGE_OFFSET	((unsigned long) 0xffff800000000000)

#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr)	(globalMemoryDesc.pages_struct + \
    (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(globalMemoryDesc.pages_struct + \
    ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))


// page table attribute
#define PAGE_XD		    (unsigned long)0x1000000000000000
#define	PAGE_PAT	    (unsigned long)0x1000
#define	PAGE_Global 	(unsigned long)0x0100
#define	PAGE_PS		    (unsigned long)0x0080
#define	PAGE_Dirty	    (unsigned long)0x0040
#define	PAGE_Accessed	(unsigned long)0x0020
#define PAGE_PCD	    (unsigned long)0x0010
#define PAGE_PWT	    (unsigned long)0x0008
#define	PAGE_U_S	    (unsigned long)0x0004
#define	PAGE_R_W	    (unsigned long)0x0002
#define	PAGE_Present	(unsigned long)0x0001

#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)
#define PAGE_USER_GDT		(PAGE_U_S | PAGE_R_W | PAGE_Present)
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)


typedef struct {
    unsigned long pml4t;
} pml4t_t;
#define	mk_mpl4t(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr, mpl4tval)	(*(mpl4tptr) = (mpl4tval))

typedef struct {
    unsigned long pdpt;
} pdpt_t;
#define mk_pdpt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr, pdptval)	(*(pdptptr) = (pdptval))

typedef struct {
    unsigned long pdt;
} pdt_t;
#define mk_pdt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr, pdtval)		(*(pdtptr) = (pdtval))

typedef struct {
    unsigned long pt;
} pt_t;
#define mk_pt(addr, attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr, ptval)		(*(ptptr) = (ptval))



unsigned long * global_CR3 = NULL;

struct E820 {
	unsigned long address;
	unsigned long length;
	unsigned int	type;
}__attribute__((packed));


struct Global_Memory_Descriptor {
	struct E820 	e820[32];
	unsigned long 	e820_length;

	unsigned long * bits_map;
	unsigned long 	bits_size;
	unsigned long   bits_length;

	struct Page *	pages_struct;
	unsigned long	pages_size;
	unsigned long 	pages_length;

	struct Zone * 	zones_struct;
	unsigned long	zones_size;
	unsigned long 	zones_length;

	unsigned long 	start_code , end_code , end_data , end_brk;

	unsigned long	end_of_struct;	
};

//alloc_pages zone_select
#define ZONE_DMA	    (1 << 0)
#define ZONE_NORMAL	    (1 << 1)
#define ZONE_UNMAPED	(1 << 2)

//struct page attribute (alloc_pages flags)
#define PG_PTable_Mapped	(1 << 0)
#define PG_Kernel_Init	    (1 << 1)
#define PG_Device	        (1 << 2)
#define PG_Kernel	        (1 << 3)
#define PG_Shared   		(1 << 4)

struct Page {
	struct Zone *	zone_struct;
	unsigned long	PHY_address;
	unsigned long	attribute;

	unsigned long	reference_count;
	
	unsigned long	age;
};


// each zone index
int ZONE_DMA_INDEX	= 0;
int ZONE_NORMAL_INDEX	= 0;	//low 1GB RAM ,was mapped in pagetable
int ZONE_UNMAPPED_INDEX	= 0;	//above 1GB RAM,unmapped in pagetable


struct Zone {
	struct Page * 	pages_group;
	unsigned long	pages_length;
	
	unsigned long	zone_start_address;
	unsigned long	zone_end_address;
	unsigned long	zone_length;
	unsigned long	attribute;

	struct Global_Memory_Descriptor * GMD_struct;

	unsigned long	page_using_count;
	unsigned long	page_free_count;

	unsigned long	total_pages_link;
};


extern struct Global_Memory_Descriptor globalMemoryDesc;


#define flush_tlb()						\
do {								\
	unsigned long	tmpreg;					\
	__asm__ __volatile__ 	(				\
				"movq	%%cr3,	%0	\n\t"	\
				"movq	%0,	%%cr3	\n\t"	\
				:"=r"(tmpreg)			\
				:				\
				:"memory"			\
				);				\
} while(0)


unsigned long *Get_gdt() {
    unsigned long *tmp;
    __asm__ __volatile__    (
    "movq	%%cr3,	%0	\n\t"
    :"=r"(tmp)
    :
    :"memory"
    );
    return tmp;
}



struct Slab {
    struct List list;
    struct Page * page;

    unsigned long using_cnt;
    unsigned long free_cnt;

    void * vaddr;   // linear addr of current page

    unsigned long color_len;
    unsigned long color_cnt;
    unsigned long *color_map;
};


struct SlabCache {
    unsigned long size;
    unsigned long total_using;
    unsigned long total_free;
    struct Slab * cache_pool;
    struct Slab * cache_DMA_pool;
    void * (* constructor) (void *vaddr, unsigned long arg);
    void *(*destructor) (void *vaddr, unsigned long arg);
};


unsigned long page_init(struct Page *page, unsigned long flags);
unsigned long page_clean(struct Page *page);

unsigned long get_page_attribute(struct Page *page);
unsigned long set_page_attribute(struct Page *page, unsigned long flags);

void init_memory();

struct Page* alloc_pages(int zone_select, int number, unsigned long page_flags);
void free_pages(struct Page *page, int num);


struct SlabCache * slab_create(unsigned long size,
                               void * (* constructor)(void *vaddr, unsigned long arg),
                               void * (* destructor)(void *vaddr, unsigned long arg),
                               unsigned long arg);
unsigned long slab_destroy(struct SlabCache * slab_cache);
void * slab_malloc(struct SlabCache * slab_cache, unsigned long arg);
unsigned long slab_free(struct SlabCache * slab_cache,
        void * addr, unsigned long arg);


struct SlabCache kmalloc_cache_size[16] = {
        {32     , 0, 0, NULL, NULL, NULL, NULL},
        {64     , 0, 0, NULL, NULL, NULL, NULL},
        {128    , 0, 0, NULL, NULL, NULL, NULL},
        {256    , 0, 0, NULL, NULL, NULL, NULL},
        {512    , 0, 0, NULL, NULL, NULL, NULL},
        {1024   , 0, 0, NULL, NULL, NULL, NULL},
        {2048   , 0, 0, NULL, NULL, NULL, NULL},
        {4096   , 0, 0, NULL, NULL, NULL, NULL},
        {8192   , 0, 0, NULL, NULL, NULL, NULL},
        {16384  , 0, 0, NULL, NULL, NULL, NULL},
        {32768  , 0, 0, NULL, NULL, NULL, NULL},
        {65536  , 0, 0, NULL, NULL, NULL, NULL},
        {131072 , 0, 0, NULL, NULL, NULL, NULL},
        {262144 , 0, 0, NULL, NULL, NULL, NULL},
        {524288 , 0, 0, NULL, NULL, NULL, NULL},
        {1048576, 0, 0, NULL, NULL, NULL, NULL},
};
unsigned long slab_init();

void * kmalloc(unsigned long size, unsigned long gfp_flags);
unsigned long kfree(void *addr);


struct Slab * kmalloc_create(unsigned long size);

void pageTable_init();

#endif
