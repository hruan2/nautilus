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
 * http://xstack.sandia.gov/hobbes
 *
 * Copyright (c) 2015, Kyle C. Hale <khale@cs.iit.edu>
 * Copyright (c) 2015, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kyle C. Hale <khale@cs.iit.edu>
 *          Conghao Liu <cliu115@hawk.iit.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */
#include <nautilus/nautilus.h>
#include <nautilus/coreboot.h>
#include <nautilus/cb_utils.h>
#include <nautilus/nautilus.h>
#include <nautilus/naut_types.h>
#include <nautilus/paging.h>
#include <nautilus/mm.h>
#include <nautilus/elf.h>

extern addr_t _bssEnd;
extern uint64_t nk_tsc_coreboot_start;

void coreboot_parse(ulong_t cbd)
{
    if (cbd & LB_ENTRY_ALIGN) {
        panic("ERROR: Unaligned coreboot header\n");
    }

    struct lb_header *tables_header = (struct lb_header *)cbd;
    ulong_t header_size = tables_header->header_bytes;
    ulong_t tables_size = tables_header->table_bytes;
    ulong_t tables_end = cbd + header_size + tables_size;

    struct lb_record *record = (struct lb_record *)(cbd + header_size);

    while ((ulong_t) record < tables_end) {
        switch (record->tag) {
            case LB_TAG_MEMORY: {
                DEBUG_PRINT("Coreboot memory map detected\n");
                break;
            }

            case LB_TAG_FRAMEBUFFER: {
                struct lb_framebuffer *fb = (struct lb_framebuffer *)record;
                DEBUG_PRINT("fb phys addr: %p, fb x res: %u, fb y res: %u\n",
                            (void *)fb->physical_address,
                            fb->x_resolution,
                            fb->y_resolution);
                break;
            }

            case LB_TAG_MAINBOARD: {
                struct lb_mainboard *mb = (struct lb_mainboard *)record;
                DEBUG_PRINT("vendor index %u, part number index %u\n",
                            mb->vendor_idx, mb->part_number_idx);
                break;
            }

            case LB_TAG_ACPI_RSDP: {
                struct lb_acpi_rsdp *rsdp = (struct lb_acpi_rsdp *)record;
                DEBUG_PRINT("ACPI: rsdp=0x%x\n", rsdp->rsdp_pointer);
                break;
            }

            case LB_TAG_TIMESTAMPS: {
                struct lb_cbmem_ref *ts_ref = (struct lb_cbmem_ref *)record;
                struct timestamp_table *ts = (struct timestamp_table *)(uintptr_t)ts_ref->cbmem_addr;
                if (ts && ts->num_entries > 0) {
                    nk_tsc_coreboot_start = ts->base_time + ts->entries[0].entry_stamp;
                }
                break;
            }

            default:
                DEBUG_PRINT("Unhandled record type (0x%x) of size %u\n",
                            record->tag, record->size);
                break;
        }

        record = (ulong_t)record + record->size;
    }
}
