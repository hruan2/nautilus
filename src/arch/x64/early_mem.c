/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Hobbes and V3VEE Projects with funding from the
 * United States National  Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. You can find out more at:
 * http://www.v3vee.org  and
 * http://xtack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <kh@u.northwestern.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Author: Kyle C. Hale <kh@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/mm.h>
#include <nautilus/macros.h>
#include <nautilus/coreboot.h>

extern char *mem_region_types[10];

#ifndef NAUT_CONFIG_DEBUG_BOOTMEM
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define BMM_DEBUG(fmt, args...) DEBUG_PRINT("BOOTMEM: " fmt, ##args)
#define BMM_PRINT(fmt, args...) printk("BOOTMEM: " fmt, ##args)
#define BMM_WARN(fmt, args...)  WARN_PRINT("BOOTMEM: " fmt, ##args)


// this doesn't seem to be used...
void
arch_reserve_boot_regions (unsigned long mbd)
{
#ifdef NAUT_CONFIG_REAL_MODE_INTERFACE
    INFO_PRINT("Reserving Long->Real Interface Segment (%p, size %lu)\n",
		NAUT_CONFIG_REAL_MODE_INTERFACE_SEGMENT*16UL, 65536UL);
    mm_boot_reserve_mem((addr_t)(NAUT_CONFIG_REAL_MODE_INTERFACE_SEGMENT*16UL),
			(ulong_t)65536);
#endif
}

void
arch_detect_mem_map (mmap_info_t *mm_info,
                     mem_map_entry_t *memory_map,
                     unsigned long cbd)
{
    struct lb_header *tables_header = (struct lb_header *) cbd;
    uint32_t header_size = tables_header->header_bytes;
    uint32_t tables_size = tables_header->table_bytes;
    void *tables_end = cbd + header_size + tables_size;
    struct lb_record *cur_record = (struct lb_record *) (cbd + header_size);

    // look for coreboot memory entry
    while (cur_record->tag != LB_TAG_MEMORY) {
        if ((void *) cur_record > tables_end) {
            panic("ERROR: memory entry could not be found\n");
        }

        cur_record = (struct lb_record *) ((unsigned long) cur_record + cur_record->size);
    }

    // if we ever reach here, it's over
    if (cur_record->tag != LB_TAG_MEMORY) {
        panic("ERROR: no memory tag found\n");
    }

    struct lb_memory *memory_record = (struct lb_memory *) cur_record;
    struct lb_memory_range *memory_range = memory_record->map;

    // do some math to determine how many memory range entries there are in
    // the memory map
    unsigned long total_entries = (memory_record->size - (2 * sizeof(uint32_t)))
                        / sizeof(struct lb_memory_range);

    if (total_entries > MAX_MMAP_ENTRIES) {
        panic("ERROR: too many memory map entries!\n");
    }

    struct lb_memory_range cur_entry = { 0 };
    ulong_t start = 0;
    ulong_t end = 0;
    int mem_type = 0;
    for (unsigned long entry = 0; entry < total_entries; ++entry) {
        start = round_up(memory_range->start, PAGE_SIZE_4KB);
        end = round_down(memory_range->start + memory_range->size, PAGE_SIZE_4KB);

        memory_map[entry].addr = start;
        memory_map[entry].len = end - start;

        // convert memory type appropriately to use with mem_region_types
        switch (memory_map[entry].type) {
            case LB_MEM_TABLE:
                mem_type = LB_MEM_TABLE_EQUIV;
                break;

            case LB_MEM_TAG:
                mem_type = LB_MEM_TAG_EQUIV;
                break;

            case LB_MEM_SOFT_RESERVED:
                mem_type = LB_MEM_SOFT_RESERVED_EQUIV;
                break;

            default:
                mem_type = memory_map[entry].type;
        }

        memory_map[entry].type = mem_type;

        BMM_PRINT("Memory map[%u] - [%p - %p] <%s>\n",
                entry,
                start,
                end,
                mem_region_types[memory_map[entry].type]);


        if (memory_range->type == LB_MEM_RAM) {
            mm_info->usable_ram += memory_range->size;
        }

        if (end > (mm_info->last_pfn << PAGE_SHIFT)) {
            mm_info->last_pfn = end >> PAGE_SHIFT;
        }

        memory_range = (struct lb_memory_range *) ((uint64_t) memory_range
                                            + sizeof(struct lb_memory_range));
        mm_info->total_mem += end - start;
        ++mm_info->num_regions;
    }
}
