#include "memory.h"
#include "lib.h"
#include "lib.h"
#include "UEFI.h"


unsigned long page_init(struct Page *page, unsigned long flags) {

    page->attribute |= flags;

    if (!page->reference_count || (page->attribute & PG_Shared)) {
        page->reference_count++;
        page->zone_struct->total_pages_link++;
    }
    return 1;
}

unsigned long page_clean(struct Page *page) {
    page->reference_count--;
    page->zone_struct->total_pages_link--;

    if (!page->reference_count) {               // reference_count == 0
        page->attribute &= PG_PTable_Mapped;
    }
    return 1;
}

unsigned long get_page_attribute(struct Page *page) {
    if (page == NULL) {
        color_printk(RED, BLACK, "get_page_attribute(): ERROR: page == NULL\n");
        return 0;
    }
    return page->attribute;
}

unsigned long set_page_attribute(struct Page *page, unsigned long flags) {
    if (page == NULL) {
        color_printk(RED, BLACK, "set_page_attribute(): ERROR: page == NULL\n");
        return 0;
    }
    page->attribute = flags;
    return 1;
}


void init_memory() {
    int i, j;
    unsigned long TotalMem = 0;
    struct EFI_E820_MEMORY_DESCRIPTOR * p = NULL;

    color_printk(BLUE, BLACK, "Display Physics Address MAP,Type(1:RAM,2:ROM or "
                              "Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,"
                              "Others:Undefine)\n");

    p = (struct EFI_E820_MEMORY_DESCRIPTOR *) boot_param->E820_info.E820_Entry;

    for (i = 0; i < boot_param->E820_info.E820_Entry_count; i++) {
//        color_printk(ORANGE, BLACK, "Address:%#018lx\tLength:%#018lx\tType:%#010x\n",
//                p->address, p->length, p->type);
        unsigned long tmp = 0;
        if (p->type == 1)
            TotalMem += p->length;

        globalMemoryDesc.e820[i].address = p->address;
        globalMemoryDesc.e820[i].length = p->length;
        globalMemoryDesc.e820[i].type = p->type;
        globalMemoryDesc.e820_length = i;
        p++;
        if (p->type > 4 || p->length == 0 || p->type < 1)
            break;
    }

    color_printk(ORANGE, BLACK, "OS Can Used Total RAM:%#018lx\n", TotalMem);

    TotalMem = 0;

    for (i = 0; i <= globalMemoryDesc.e820_length; i++) {
        unsigned long start, end;
        if (globalMemoryDesc.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(globalMemoryDesc.e820[i].address);
        end = ((globalMemoryDesc.e820[i].address + globalMemoryDesc.e820[i].length) >> PAGE_2M_SHIFT)
                << PAGE_2M_SHIFT;
        if (end <= start)
            continue;
        TotalMem += (end - start) >> PAGE_2M_SHIFT;
    }

    color_printk(ORANGE, BLACK, "OS Can Used Total 2M PAGEs:%#010x=%010d\n",
            TotalMem, TotalMem);

    TotalMem = globalMemoryDesc.e820[globalMemoryDesc.e820_length].address +
            globalMemoryDesc.e820[globalMemoryDesc.e820_length].length;

    //bits map construction init

    globalMemoryDesc.bits_map = (unsigned long *) ((globalMemoryDesc.end_brk +
            PAGE_4K_SIZE - 1) & PAGE_4K_MASK);

    globalMemoryDesc.bits_size = TotalMem >> PAGE_2M_SHIFT;

    globalMemoryDesc.bits_length = (((unsigned long) (TotalMem >> PAGE_2M_SHIFT)
            + sizeof(long) * 8 - 1) / 8) & (~(sizeof(long) - 1));

    memset(globalMemoryDesc.bits_map, 0xff, globalMemoryDesc.bits_length);
    //init bits map memory

    //pages construction init

    globalMemoryDesc.pages_struct = (struct Page *) (
            ((unsigned long) globalMemoryDesc.bits_map + globalMemoryDesc.bits_length + PAGE_4K_SIZE -
             1) & PAGE_4K_MASK);

    globalMemoryDesc.pages_size = TotalMem >> PAGE_2M_SHIFT;

    globalMemoryDesc.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) +
                    sizeof(long) - 1) & (~(sizeof(long) - 1));

    memset(globalMemoryDesc.pages_struct, 0x00, globalMemoryDesc.pages_length);    //init pages memory

    //zones construction init

    globalMemoryDesc.zones_struct = (struct Zone *) (
            ((unsigned long) globalMemoryDesc.pages_struct + globalMemoryDesc.pages_length +
             PAGE_4K_SIZE - 1) & PAGE_4K_MASK);

    globalMemoryDesc.zones_size = 0;

    globalMemoryDesc.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

    memset(globalMemoryDesc.zones_struct, 0x00, globalMemoryDesc.zones_length);    //init zones memory



    for (i = 0; i <= globalMemoryDesc.e820_length; i++) {
        unsigned long start, end;
        struct Zone *z;
        struct Page *p;
        unsigned long *b;

        if (globalMemoryDesc.e820[i].type != 1)
            continue;
        start = PAGE_2M_ALIGN(globalMemoryDesc.e820[i].address);
        end = ((globalMemoryDesc.e820[i].address + globalMemoryDesc.e820[i].length)
                >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        if (end <= start)
            continue;

        // zone init
        z = globalMemoryDesc.zones_struct + globalMemoryDesc.zones_size;
        globalMemoryDesc.zones_size++;

        z->zone_start_address = start;
        z->zone_end_address = end;
        z->zone_length = end - start;

        z->page_using_count = 0;
        z->page_free_count = (end - start) >> PAGE_2M_SHIFT;

        z->total_pages_link = 0;

        z->attribute = 0;
        z->GMD_struct = &globalMemoryDesc;

        z->pages_length = (end - start) >> PAGE_2M_SHIFT;
        z->pages_group = (struct Page *) (globalMemoryDesc.pages_struct +
                (start >> PAGE_2M_SHIFT));

        // page init
        p = z->pages_group;
        for (j = 0; j < z->pages_length; j++, p++) {
            p->zone_struct = z;
            p->PHY_address = start + PAGE_2M_SIZE * j;
            p->attribute = 0;
            p->reference_count = 0;
            p->age = 0;
            *(globalMemoryDesc.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^=
                    1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
        }
    }

    // init address 0 to page struct 0; because the globalMemoryDesc.e820[0].type != 1
    globalMemoryDesc.pages_struct->zone_struct = globalMemoryDesc.zones_struct;
    globalMemoryDesc.pages_struct->PHY_address = 0UL;

    set_page_attribute(globalMemoryDesc.pages_struct,
            PG_PTable_Mapped | PG_Kernel | PG_Kernel_Init);

    globalMemoryDesc.pages_struct->reference_count = 1;
    globalMemoryDesc.pages_struct->age = 0;

    globalMemoryDesc.zones_length = (globalMemoryDesc.zones_size *
            sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));

    color_printk(ORANGE, BLACK, "bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n",
                 globalMemoryDesc.bits_map, globalMemoryDesc.bits_size,
                 globalMemoryDesc.bits_length);

    color_printk(ORANGE, BLACK, "pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n",
                 globalMemoryDesc.pages_struct, globalMemoryDesc.pages_size,
                 globalMemoryDesc.pages_length);

    color_printk(ORANGE, BLACK, "zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n",
                 globalMemoryDesc.zones_struct, globalMemoryDesc.zones_size,
                 globalMemoryDesc.zones_length);

    ZONE_DMA_INDEX = 0;
    ZONE_NORMAL_INDEX = 0;
    ZONE_UNMAPPED_INDEX = 0;

    for (i = 0; i < globalMemoryDesc.zones_size; i++) {
        struct Zone *z = globalMemoryDesc.zones_struct + i;
        color_printk(ORANGE, BLACK,
                     "zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n",
                     z->zone_start_address, z->zone_end_address, z->zone_length, z->pages_group, z->pages_length);

        if (z->zone_start_address >= 0x100000000 && !ZONE_UNMAPPED_INDEX)
            ZONE_UNMAPPED_INDEX = i;
    }
    color_printk(ORANGE, BLACK, "ZONE_DMA_INDEX:%d\tZONE_NORMAL_INDEX:%d\tZONE_UNMAPED_INDEX:%d\n", ZONE_DMA_INDEX,
                 ZONE_NORMAL_INDEX, ZONE_UNMAPPED_INDEX);

    globalMemoryDesc.end_of_struct = (unsigned long) ((unsigned long) globalMemoryDesc.zones_struct +
            globalMemoryDesc.zones_length + sizeof(long) * 32) & (~(sizeof(long) - 1));
    // need a blank to separate globalMemoryDesc

    color_printk(ORANGE, BLACK,
                 "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",
                 globalMemoryDesc.start_code, globalMemoryDesc.end_code,
                 globalMemoryDesc.end_data, globalMemoryDesc.end_brk,
                 globalMemoryDesc.end_of_struct);


    i = Virt_To_Phy(globalMemoryDesc.end_of_struct) >> PAGE_2M_SHIFT;

    for (j = 1; j <= i; j++) {
        struct Page * tmp_page = globalMemoryDesc.pages_struct + j;
        page_init(tmp_page, PG_PTable_Mapped | PG_Kernel_Init | PG_Kernel);
        tmp_page->zone_struct->page_using_count++;
        tmp_page->zone_struct->page_free_count--;
        *(globalMemoryDesc.bits_map + ((tmp_page->PHY_address >> PAGE_2M_SHIFT)
                >> 6)) |= 1UL << (tmp_page->PHY_address >> PAGE_2M_SHIFT) % 64;
    }


    global_CR3 = Get_gdt();

    color_printk(INDIGO, BLACK, "Global_CR3\t:%#018lx\n", global_CR3);
    color_printk(INDIGO, BLACK, "*Global_CR3\t:%#018lx\n",
            *Phy_To_Virt(global_CR3) & (~0xff));
    color_printk(PURPLE, BLACK, "**Global_CR3\t:%#018lx\n",
            *Phy_To_Virt(*Phy_To_Virt(global_CR3) & (~0xff)) & (~0xff));

    color_printk(ORANGE, BLACK, "1.memory_management_struct.bits_map:%#018lx\t"
                                "zone_struct->page_using_count:%d\tzone_struct->"
                                "page_free_count:%d\n",
                 *globalMemoryDesc.bits_map,
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);

//    for (i = 0; i < 10; i++)
//        *(Phy_To_Virt(global_CR3) + i) = 0UL;

    flush_tlb();
}


/*
	number: number < 64
	zone_select: zone select from DMA, mapped in  page table, unmapped in page table
	page_flags: struct Page flags
*/
struct Page * alloc_pages(int zone_select, int number, unsigned long page_flags) {
	unsigned long page = 0;
    unsigned long attribute = 0;

	int zone_start = 0;
	int zone_end = 0;

	if (number >= 64 || number <= 0) {
        color_printk(RED, BLACK, "alloc_pages(): ERROR: @number invalid\n");
        return NULL;
	}

    switch (zone_select) {
        case ZONE_DMA:
            zone_start = 0;
            zone_end = ZONE_DMA_INDEX;
            attribute = PG_PTable_Mapped;
            break;

        case ZONE_NORMAL:
            zone_start = ZONE_DMA_INDEX;
            zone_end = ZONE_NORMAL_INDEX;
            attribute = PG_PTable_Mapped;
            break;

        case ZONE_UNMAPED:
            zone_start = ZONE_UNMAPPED_INDEX;
            zone_end = globalMemoryDesc.zones_size - 1;
            attribute = 0;
            break;

        default:
            color_printk(RED, BLACK, "alloc_pages error zone_select index\n");
            return NULL;
    }

    int i;
    for (i = zone_start; i <= zone_end; i++) {
        struct Zone *z;
        unsigned long j;
        unsigned long start, end;
        unsigned long tmp;

        if ((globalMemoryDesc.zones_struct + i)->page_free_count < number)
            continue;

        z = globalMemoryDesc.zones_struct + i;
        start = z->zone_start_address >> PAGE_2M_SHIFT;
        end = z->zone_end_address >> PAGE_2M_SHIFT;

        tmp = 64 - start % 64;
        for (j = start; j < end; j += j % 64 ? tmp : 64) {
            unsigned long *p = globalMemoryDesc.bits_map + (j >> 6);
            unsigned long shift = j % 64;
            unsigned long k = 0;
            unsigned long num = (1UL << number) - 1;

            for (k = shift; k < 64; k++) {
                if (!((k ? ((*p >> k) | (*(p + 1) << (64 - k))) : *p) & num)) {
                    unsigned long l;
                    page = j + k - shift;
                    for (l = 0; l < number; l++) {
                        struct Page *pagePtr = globalMemoryDesc.pages_struct + page + l;
                        *(globalMemoryDesc.bits_map + ((pagePtr->PHY_address >>
                                    PAGE_2M_SHIFT) >> 6)) |= 1UL << (pagePtr->PHY_address
                                    >> PAGE_2M_SHIFT) % 64;
                        z->page_free_count--;
                        z->page_using_count++;
                        pagePtr->attribute = attribute;
                    }
                    return (struct Page *)(globalMemoryDesc.pages_struct + page);
                }
            }
        }
    }
    color_printk(RED, BLACK, "alloc_pages() ERROR: NO page can alloc\n");
	return NULL;
}

void free_pages(struct Page *page, int num) {

    if (page == NULL) {
        color_printk(RED, BLACK, "free_pages() ERROR: @page invalid\n");
        return;
    }
    if (num <= 0 || num >= 64) {
        color_printk(RED, BLACK, "free_pages() ERROR: @num invalid\n");
        return;
    }
    for (int i = 0; i < num; i++, page++) {
        *(globalMemoryDesc.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &=
                ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);
        page->zone_struct->page_using_count--;
        page->zone_struct->page_free_count++;
        page->attribute = 0;
    }
}



struct SlabCache * slab_create(unsigned long size,
        void * (* constructor)(void *vaddr, unsigned long arg),
        void * (* destructor)(void *vaddr, unsigned long arg),
        unsigned long arg) {

    struct SlabCache * slab_cache = NULL;
    slab_cache = (struct SlabCache *) kmalloc(sizeof(struct SlabCache), 0);
    if (slab_cache == NULL) {
        color_printk(RED, BLACK, "ERROR: slab_create(): kmalloc(): NULL\n");
        return NULL;
    }
    memset(slab_cache, 0, sizeof(struct SlabCache));

    slab_cache->size = SIZEOF_LONG_ALIGN(size);
    slab_cache->total_free = 0;
    slab_cache->total_using = 0;
    slab_cache->cache_pool = (struct Slab *) kmalloc(sizeof(struct Slab), 0);
    if (slab_cache->cache_pool == NULL) {
        color_printk(RED, BLACK, "slab_create(): slab_cache->slab_pool == NULL\n");
        kfree(slab_cache);
        return NULL;
    }
    memset(slab_cache->cache_pool, 0, sizeof(struct Slab));
    slab_cache->cache_DMA_pool = NULL;
    slab_cache->constructor = constructor;
    slab_cache->destructor = destructor;
    list_init(&slab_cache->cache_pool->list);

    slab_cache->cache_pool->page = alloc_pages(ZONE_NORMAL, 1, 0);
    if (slab_cache->cache_pool->page == NULL) {
        color_printk(RED, BLACK, "slab_create(): slab_cache->cache_pool->page == NULL\n");
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }

    page_init(slab_cache->cache_pool->page, PG_Kernel);

    slab_cache->cache_pool->using_cnt = 0;
    slab_cache->cache_pool->free_cnt = PAGE_2M_SIZE / slab_cache->size;
    slab_cache->total_free = slab_cache->cache_pool->free_cnt;
    slab_cache->cache_pool->color_cnt = slab_cache->cache_pool->free_cnt;
    slab_cache->cache_pool->color_len = ((slab_cache->cache_pool->color_cnt +
            sizeof(long) * 8 - 1) >> 6) << 3;
    slab_cache->cache_pool->vaddr = Phy_To_Virt(slab_cache->cache_pool->page->PHY_address);
    slab_cache->cache_pool->color_map = (unsigned long *)
            kmalloc(slab_cache->cache_pool->color_len, 0);
    if (slab_cache->cache_pool->color_map == NULL) {
        color_printk(RED, BLACK, "slab_create(): slab_cache->cache_pool->color_map == NULL\n");
        free_pages(slab_cache->cache_pool->page, 1);
        kfree(slab_cache->cache_pool);
        kfree(slab_cache);
        return NULL;
    }
    memset(slab_cache->cache_pool->color_map, 0, slab_cache->cache_pool->color_len);

    return slab_cache;
}

unsigned long slab_destroy(struct SlabCache * slab_cache) {

    struct Slab * slab_p = slab_cache->cache_pool;
    struct Slab * slab_tmp = NULL;
    if (slab_cache->total_using != 0) {
        color_printk(RED, BLACK, "slab_cache->total_using != 0\n");
        return 0;
    }

    while (!list_isEmpty(&slab_p->list)) {
        slab_tmp = slab_p;
        slab_p = CONTAINER_OF(list_next(&slab_p->list), struct Slab, list);
        list_del(&slab_tmp->list);
        kfree(slab_tmp->color_map);

        page_clean(slab_tmp->page);
        free_pages(slab_tmp->page, 1);
        kfree(slab_tmp);
    }
    kfree(slab_p->color_map);
    page_clean(slab_p->page);
    free_pages(slab_p->page, 1);
    kfree(slab_p);

    kfree(slab_cache);
    return 1;
}



void * slab_malloc(struct SlabCache * slab_cache, unsigned long arg) {
    struct Slab * slab_p = slab_cache->cache_pool;
    struct Slab * slab_tmp = NULL;

    if (slab_cache->total_free == 0) {
        // need to make slab_cache bigger
        slab_tmp = (struct Slab *) kmalloc(sizeof(struct Slab), 0);
        if (slab_tmp == NULL) {
            color_printk(RED, BLACK, "slab_malloc(): slab_tmp == NULL\n");
            return NULL;
        }
        memset(slab_tmp, 0, sizeof(struct Slab));

        list_init(&slab_tmp->list);
        slab_tmp->page = alloc_pages(ZONE_NORMAL, 1, 0);
        if (slab_tmp->page == NULL) {
            color_printk(RED, BLACK, "slab_malloc(): slab_tmp->page == NULL\n");
            kfree(slab_tmp);
            return NULL;
        }
        page_init(slab_tmp->page, PG_Kernel);

        // --------------------------------
        slab_tmp->using_cnt = PAGE_2M_SIZE / slab_cache->size;
        slab_tmp->free_cnt = slab_tmp->using_cnt;
        slab_tmp->vaddr = Phy_To_Virt(slab_tmp->page->PHY_address);

        slab_tmp->color_cnt = slab_tmp->free_cnt;
        slab_tmp->color_len = ((slab_tmp->color_cnt + sizeof(long) * 8 - 1)
                >> 6) << 3;
        slab_tmp->color_map = (unsigned long *) kmalloc(slab_tmp->color_len, 0);
        if (slab_tmp->color_map == NULL) {
            color_printk(RED, BLACK, "slab_malloc(): slab_tmp->color_map == NULL\n");
            free_pages(slab_tmp->page, 1);
            kfree(slab_tmp);
            return NULL;
        }
        memset(slab_tmp->color_map, 0, slab_tmp->color_len);

        list_add_to_next(&slab_cache->cache_pool->list, &slab_tmp->list);

        slab_cache->total_free += slab_tmp->color_cnt;

        for (int i = 0; i < slab_tmp->color_cnt; i++) {
            if (*(slab_tmp->color_map + (i >> 6)) & (1UL << (i % 64)) == 0) {
                *(slab_tmp->color_map + (i >> 6)) |= 1UL << (i % 64);
                slab_tmp->using_cnt++;
                slab_tmp->free_cnt--;

                if (slab_cache->constructor != NULL) {
                    return slab_cache->constructor((char *)slab_tmp->vaddr +
                            slab_cache->size * i, arg);
                }
                return (void *) ((char *)slab_tmp->vaddr + slab_cache->size * i);
            }
        }

    } else {

        do {
            if (slab_p->free_cnt == 0) {
                slab_p = CONTAINER_OF(list_next(&slab_p->list), struct Slab, list);
                continue;
            }

            for (int i = 0; i < slab_p->color_cnt; i++) {

                if (*(slab_p->color_map + (i >> 6)) == 0xffffffffffffffff) {
                    i += 63;
                    continue;
                }

                if (*(slab_p->color_map + (i >> 6)) & (1UL << (i % 64)) == 0) {
                    *(slab_p->color_map + (i >> 6)) |= 1UL << (i % 64);

                    slab_p->using_cnt++;
                    slab_p->free_cnt--;

                    slab_cache->total_using++;
                    slab_cache->total_free--;
                    if (slab_cache->constructor != NULL) {
                        return slab_cache->constructor((char *)slab_p->vaddr +
                            slab_cache->size * i, arg);
                    }
                    return (void *) ((char *)slab_p->vaddr + slab_cache->size * i);
                }
            }

        } while (slab_p != slab_cache->cache_pool);
    }

    color_printk(RED, BLACK, "slab_malloc() ERROR: cannot alloc\n");

    if (slab_tmp != NULL) {
        list_del(&slab_tmp->list);
        kfree(slab_tmp->color_map);
        page_clean(slab_tmp->page);
        free_pages(slab_tmp->page, 1);
        kfree(slab_tmp);
    }
    return NULL;
}

unsigned long slab_free(struct SlabCache * slab_cache,
                        void * addr, unsigned long arg) {
    struct Slab * slab_p = slab_cache->cache_pool;
    int idx = -1;

    do {
        if (slab_p->vaddr <= addr && slab_p->vaddr + PAGE_2M_SIZE > addr) {
            // hit !
            idx = (addr - slab_p->vaddr) / slab_cache->size;

            *(slab_p->color_map + (idx >> 6)) |= 1UL << (idx % 64);
            slab_p->using_cnt--;
            slab_p->free_cnt++;

            slab_cache->total_using--;
            slab_cache->total_free++;

            if (slab_cache->destructor != NULL) {
                slab_cache->destructor((char *) slab_p->vaddr +
                                       slab_cache->size * idx, arg);
            }

            if (slab_p->using_cnt == 0 &&
                slab_cache->total_free >= slab_p->color_cnt * 3 / 2) {

                list_del(&slab_p->list);
                slab_cache->total_free -= slab_p->color_cnt;

                kfree(slab_p->color_map);
                page_clean(slab_p->page);
                free_pages(slab_p->page, 1);
                kfree(slab_p);
            }
            return 1;
        } else {
            slab_p = CONTAINER_OF(list_next(&slab_p->list), struct Slab, list);
        }

    } while (slab_p != slab_cache->cache_pool);

    color_printk(RED, BLACK, "slab_free(): ERROR: @addr NOT in slab\n");
    return 0;
}



unsigned long slab_init() {

    struct Page * page = NULL;

    unsigned long end_of_struct_old = globalMemoryDesc.end_of_struct;
    unsigned long i, j;

    for (i = 0; i < 16; i++) {
        kmalloc_cache_size[i].cache_pool = (struct Slab *)globalMemoryDesc.end_of_struct;
        globalMemoryDesc.end_of_struct = globalMemoryDesc.end_of_struct +
                sizeof(struct Slab) + sizeof(long) * 10;

        list_init(&kmalloc_cache_size[i].cache_pool->list);

        kmalloc_cache_size[i].cache_pool->using_cnt = 0;
        kmalloc_cache_size[i].cache_pool->free_cnt =
                PAGE_2M_SIZE / kmalloc_cache_size[i].size;
        kmalloc_cache_size[i].cache_pool->color_len =
                ((PAGE_2M_SIZE / kmalloc_cache_size[i].size
                + sizeof(long) * 8 - 1) >> 6) << 3;
        kmalloc_cache_size[i].cache_pool->color_cnt =
                kmalloc_cache_size[i].cache_pool->free_cnt;
        kmalloc_cache_size[i].cache_pool->color_map = (unsigned long *)
                globalMemoryDesc.end_of_struct;

        globalMemoryDesc.end_of_struct = (unsigned long) (
                globalMemoryDesc.end_of_struct +
                kmalloc_cache_size[i].cache_pool->color_len + sizeof(long) * 10) &
                                         (~(sizeof(long) - 1));
        memset(kmalloc_cache_size[i].cache_pool->color_map, 0xff,
               kmalloc_cache_size[i].cache_pool->color_len);

        for (j = 0; j < kmalloc_cache_size[i].cache_pool->color_cnt; j++) {
            *(kmalloc_cache_size[i].cache_pool->color_map + (j >> 6)) ^= 1UL << j % 64;
        }
        kmalloc_cache_size[i].total_using = 0;
        kmalloc_cache_size[i].total_free = kmalloc_cache_size[i].cache_pool->color_cnt;
    }

    i = Virt_To_Phy(globalMemoryDesc.end_of_struct) >> PAGE_2M_SHIFT;
    for (j = PAGE_2M_ALIGN(Virt_To_Phy(end_of_struct_old)) >> PAGE_2M_SHIFT;
                j <= i; j++) {
        page = globalMemoryDesc.pages_struct + j;
        *(globalMemoryDesc.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |=
                1UL << ((page->PHY_address >> PAGE_2M_SHIFT) % 64);
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;
        page_init(page, PG_PTable_Mapped | PG_Kernel | PG_Kernel_Init);
        color_printk(INDIGO, BLACK, "-----%d----\n", page->zone_struct->page_using_count);
    }
    color_printk(ORANGE, BLACK, "2.memory_management_struct.bits_map:%#018lx\tzone_struct"
                                "->page_using_count:%d\tzone_struct->page_free_count:"
                                "%d\n",
                 *globalMemoryDesc.bits_map,
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);

    unsigned long * virtual = NULL;
    for (i = 0; i < 16; i++) {
        virtual = (unsigned long *)((globalMemoryDesc.end_of_struct + PAGE_2M_SIZE * i
                + PAGE_2M_SIZE - 1) & PAGE_2M_MASK);
        page = Virt_To_2M_Page(virtual);

        *(globalMemoryDesc.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |=
                1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
        page->zone_struct->page_using_count++;
        page->zone_struct->page_free_count--;

        page_init(page, PG_PTable_Mapped | PG_Kernel_Init | PG_Kernel);

        kmalloc_cache_size[i].cache_pool->page = page;
        kmalloc_cache_size[i].cache_pool->vaddr = virtual;
//        color_printk(ORANGE, BLACK, "-----.memory_management_struct.bits_map:%#018lx\tzone_struct"
//                                   "->page_using_count:%d\tzone_struct->page_free_count:"
//                                   "%d\n",
//                    *globalMemoryDesc.bits_map,
//                    page->zone_struct->page_using_count,
//                    page->zone_struct->page_free_count);
    }

    color_printk(ORANGE, BLACK,
                 "3.memory_management_struct.bits_map:%#018lx\tzone_struct->page_using_count:%d\tzone_struct->page_free_count:%d\n",
                 *globalMemoryDesc.bits_map,
                 globalMemoryDesc.zones_struct->page_using_count,
                 globalMemoryDesc.zones_struct->page_free_count);

    color_printk(ORANGE, BLACK,
                 "start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",
                 globalMemoryDesc.start_code, globalMemoryDesc.end_code,
                 globalMemoryDesc.end_data, globalMemoryDesc.end_brk,
                 globalMemoryDesc.end_of_struct);

    return 1;
}



void * kmalloc(unsigned long size, unsigned long gfp_flags) {

    struct Slab *slab = NULL;
    int i, j;

    if (size > 1048576) {
        color_printk(RED, BLACK, "kmalloc() ERROR: @size: %08d is too large\n", size);
        return NULL;
    }

    for (i = 0; i < 16; i++)
        if (kmalloc_cache_size[i].size >= size)
            break;
    slab = kmalloc_cache_size[i].cache_pool;

    if (kmalloc_cache_size[i].total_free != 0) {
        do {
            if (slab->free_cnt == 0)
                slab = CONTAINER_OF(list_next(&slab->list), struct Slab, list);
            else
                break;

        } while (slab != kmalloc_cache_size[i].cache_pool);

    } else {
        slab = kmalloc_create(kmalloc_cache_size[i].size);
        if (slab == NULL) {
            color_printk(RED, BLACK, "kmalloc(): kmalloc_create() return NULL\n");
            return NULL;
        }
        kmalloc_cache_size[i].total_free += slab->color_cnt;
        color_printk(BLUE, BLACK, "kmalloc(): kmalloc_create(): size=%#010x\n",
                     kmalloc_cache_size[i].size);
        list_add_to_before(&kmalloc_cache_size[i].cache_pool->list, &slab->list);
    }

    for (j = 0; j < slab->color_cnt; j++) {
        if (*(slab->color_map + (j >> 6)) == 0xffffffffffffffffUL) {
            j += 63;
            continue;
        }
        if ((*(slab->color_map + (j >> 6)) & (1UL << (j % 64))) == 0) {
            *(slab->color_map + (j >> 6)) |= 1UL << (j % 64);
            slab->using_cnt++;
            slab->free_cnt--;

            kmalloc_cache_size[i].total_free--;
            kmalloc_cache_size[i].total_using++;
            return (void *) ((char *)slab->vaddr + kmalloc_cache_size[i].size * j);
        }
    }

    color_printk(RED, BLACK, "NO mem can alloc!\n");
    return NULL;
}

unsigned long kfree(void *addr) {
    void *page_base_addr = (void *) ((unsigned long)addr & PAGE_2M_MASK);
    struct Slab * slab = NULL;

    int idx;
    int i;
    for (i = 0; i < 16; i++) {
        slab = kmalloc_cache_size[i].cache_pool;
        do {
            if (slab->vaddr != page_base_addr) {
                slab = CONTAINER_OF(list_next(&slab->list), struct Slab, list);

            } else {
                // hit !
                idx = (addr - slab->vaddr) / kmalloc_cache_size[i].size;

                *(slab->color_map + (idx >> 6)) ^= 1UL << idx % 64;
                slab->free_cnt++;
                slab->using_cnt--;

                kmalloc_cache_size[i].total_using--;
                kmalloc_cache_size[i].total_free++;

                if (slab->using_cnt == 0 &&
                    kmalloc_cache_size[i].total_free >= slab->color_cnt * 3 / 2 &&
                    slab != kmalloc_cache_size[i].cache_pool) {
                    // reduce the space of current SlabCache

                    switch (kmalloc_cache_size[i].size) {
                        case 32:
                        case 64:
                        case 128:
                        case 256:
                        case 512:
                            // slab, color map in the page
                            list_del(&slab->list);
                            kmalloc_cache_size[i].total_free -= slab->color_cnt;
                            page_clean(slab->page);
                            free_pages(slab->page, 1);
                            break;

                        default:
                            // slab, color map not in the page
                            list_del(&slab->list);
                            kmalloc_cache_size[i].total_free -= slab->color_cnt;

                            kfree(slab->color_map);

                            page_clean(slab->page);
                            free_pages(slab->page, 1);
                            kfree(slab);
                            break;
                    }
                }
                return 1;
            }
        } while (slab != kmalloc_cache_size[i].cache_pool);
    }

    color_printk(RED, BLACK, "kfree(): ERROR: @addr invalid\n");
    return 0;
}


struct Slab * kmalloc_create(unsigned long size) {
    struct Slab * slab = NULL;
    struct Page * page = NULL;
    unsigned long * vaddr = NULL;
    long structSize = 0;

    page = alloc_pages(ZONE_NORMAL, 1, 0);
    if (page == NULL) {
        color_printk(RED, BLACK, "kmalloc_create(): page == NULL\n");
        return NULL;
    }
    page_init(page, PG_Kernel);

    int i;
    switch (size) {
        case 32:
        case 64:
        case 128:
        case 256:
        case 512:
            // put slab ang colorMap at the end of the page just applied

            vaddr = Phy_To_Virt(page->PHY_address);
            structSize = sizeof(struct Slab) + PAGE_2M_SIZE / size / 8;  // ???

            slab = (struct Slab *) ((unsigned char *)vaddr + PAGE_2M_SIZE - structSize);
            slab->color_map = (unsigned long *) ((unsigned char *)slab
                    + sizeof(struct Slab));
            slab->free_cnt = (PAGE_2M_SIZE - (PAGE_2M_SIZE / size / 8) -
                    sizeof(struct Slab)) / size;
            slab->using_cnt = 0;
            slab->color_cnt = slab->free_cnt;
            slab->color_len = ((slab->free_cnt + sizeof(unsigned long) * 8
                                - 1) >> 6) << 3;
            memset(slab->color_map, 0xff, slab->color_len);
            for (i = 0; i < slab->color_cnt; i++) {
                *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;
            }
            break;

        case 1024:
        case 2048:
        case 4096:
        case 8192:
        case 16384:
        case 32768:
        case 65536:
        case 131072:		//128KB
        case 262144:
        case 524288:
        case 1048576:		//1MB
            // slab and color map not in the page just applied
            // color map is very short

            slab = (struct Slab *) kmalloc(sizeof(struct Slab), 0);
            slab->using_cnt = 0;
            slab->free_cnt = PAGE_2M_SIZE / size;
            slab->color_cnt = slab->free_cnt;
            slab->color_len = ((slab->free_cnt + sizeof(unsigned long) * 8
                                - 1) >> 6) << 3;
            slab->color_map = (unsigned long *) kmalloc(slab->color_len, 0);
            if (slab->color_cnt == NULL) {
                color_printk(RED, BLACK, "kmalloc_create(): ERROR: colormap==NULL\n");
                return NULL;
            }
            memset(slab->color_map, 0xff, slab->color_len);
            for (i = 0; i < slab->color_cnt; i++) {
                *(slab->color_map + (i >> 6)) ^= 1UL << i % 64;
            }
            slab->vaddr = Phy_To_Virt(page->PHY_address);
            slab->page = page;
            list_init(&slab->list);
            break;

        default:
            color_printk(RED, BLACK, "kmalloc(): ERROR: wrong size:%08d\n", size);
            free_pages(page, 1);
            return NULL;
    }

    return slab;
}


void pageTable_init() {
    global_CR3 = Get_gdt();
    unsigned long * tmp;

    tmp = (unsigned long *) (((unsigned long)Phy_To_Virt(
            (unsigned long) global_CR3 & (~0xfffUL))));
    color_printk(YELLOW, BLACK, "0: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    tmp = (unsigned long *) (((unsigned long)Phy_To_Virt(
            (unsigned long) global_CR3 & (~0xfffUL))) + 8 * 256);
    color_printk(YELLOW, BLACK, "1: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    tmp = Phy_To_Virt(*tmp & (~0xfffUL));
    color_printk(YELLOW, BLACK, "2: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    tmp = Phy_To_Virt(*tmp & (~0xfffUL));
    color_printk(YELLOW, BLACK, "3: %#018lx, %#018lx\t\t\n", (unsigned long) tmp, *tmp);

    unsigned long i, j;
    for (i = 0; i < globalMemoryDesc.zones_size; i++) {
        struct Zone * z = globalMemoryDesc.zones_struct + i;
        struct Page * p = z->pages_group;

        if (ZONE_UNMAPPED_INDEX && i == ZONE_UNMAPPED_INDEX)
            break;

        for (j = 0; j < z->pages_length; j++, p++) {
            tmp = (unsigned long *) (((unsigned long)Phy_To_Virt((unsigned long)global_CR3
                    & (~0xfffUL))) + (((unsigned long)Phy_To_Virt(p->PHY_address) >>
                            PAGE_GDT_SHIFT) & 0x1ff) * 8);
            if (p->PHY_address >= 0xa0000000 && p->PHY_address <= 0xc0000000) {
                color_printk(RED, WHITE, "!!!!!");
            }
            if (*tmp == 0) {
                // new a entry
                // will not execute
                color_printk(RED, BLACK,
                        "pg tbl init ERROR: *tmp=%018lx", *tmp);
                while (1);
            }

            // *tmp =   0x102003
            //	        .fill	255, 8, 0
            //	        .quad	0x102003
            //	        .fill	255, 8, 0
            tmp = (unsigned long *) ((unsigned long)Phy_To_Virt(*tmp & (~ 0xfffUL))
                    + (((unsigned long)Phy_To_Virt(p->PHY_address)
                    >> PAGE_1G_SHIFT) & 0x1ff) * 8);

            // *tmp =   0x103003
            //          .fill 511, 8, 0
            if (*tmp == 0) {
                unsigned long *virtual = kmalloc(PAGE_4K_SIZE, 0);
                set_pdpt(tmp, mk_pdpt(Virt_To_Phy(virtual), PAGE_USER_Dir));
            }

            tmp = (unsigned long *) ((unsigned long) Phy_To_Virt(*tmp & (~0xfffUL)) +
                 (((unsigned long) Phy_To_Virt(p->PHY_address) >>
                   PAGE_2M_SHIFT) & 0x1ff) * 8);
            set_pdt(tmp, mk_pdt(p->PHY_address, PAGE_USER_Page));


//            if (j % 50 == 0) {
//                color_printk(GREEN, BLACK,
//                        "@:%#018lx,%#018lx\t\n", (unsigned long) tmp, *tmp);
//            }
        }
    }

    flush_tlb();
}





