
#include <stdio.h>
#include <malloc.h>

#include "../types.h"
#include "cpu.h"
#include "ops.h"
#include "../dev.h"
#include "mem.h"
#include "mem.hpp"


regt MMT_POINTER;

regt MMT_PAGE_SIZE = 512;
regt MMT_PAGE_MASK = 511;
regt MMT_PAGE_BITS = 9;
regt MMT_PAGE_COUNT = 0;

regt PAGE_FAULT_ADDRESS;
regt PAGE_FAULT_POINTER;
regt PAGE_FAULT_ENTRY;

octet* sysmem;

void init_memory() {
    sysmem = (octet*)malloc_aligned(MEM_COUNT * sizeof(memt));
}

void dump_memory(regt offset, regt length) {
    fprintf(stderr, "\n -- Flat Memory at %016llx - %016llx: \n", offset, offset + length);
    regt i = offset;
    while(i < (offset + length)) {
        if(i % 16 == 0 || i == offset)
            fprintf(stderr, "%016llx | ", i);
        octet* p = flat(i);
        fprintf(stderr, "%02x%02x ", *p, *(p + 1));
        if(i % 16 == 15)
            fprintf(stderr, "\n");
        
        i += 1;
    }
    fprintf(stderr, "\n");
}

#define PAGE_TABLE_ENTRY_SIZE 4

octet* mem(regt offset, memt reason) {
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        regt page_number = offset >> MMT_PAGE_BITS;
        regt PTCBASE = MMT_POINTER;
        octet* o = flat(PTCBASE);
        regt page_table_entry_count = mem_to_reg64(*reinterpret_cast<regt*>(o));

        if(page_number >= page_table_entry_count) {
            PageFault* p = new PageFault();
            p->offset = offset;
            p->pointer = MMT_POINTER;
            p->entry = (regt)-1;
            throw *p;
            return NULL;
        } else {
            regt PTBASE = page_number * (PAGE_TABLE_ENTRY_SIZE + 1) + MMT_POINTER;
            octet* o = flat(PTBASE);
            regt page_table_entry = mem_to_reg64(*reinterpret_cast<regt*>(o));

            if(!(page_table_entry & MMT_VALID_MASK) || !(page_table_entry & MMT_ALLOCATED_MASK) || ((page_table_entry & reason) != reason)) {
                PageFault* p = new PageFault();
                p->offset = offset;
                p->pointer = MMT_POINTER;
                p->entry = page_table_entry;
                throw *p;
                return NULL;
            } else {
                offset = (page_table_entry & ~MMT_PAGE_MASK) + (offset & MMT_PAGE_MASK);
            }

            return flat(offset);
        }
    } else {
        return flat(offset);
    }
}

regt mem_to_flat(regt offset) {
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        regt page_number = offset >> MMT_PAGE_BITS;
        regt PTCBASE = MMT_POINTER;
        octet* o = flat(PTCBASE);
        regt page_table_entry_count = mem_to_reg64(*reinterpret_cast<regt*>(o));

        if(page_number >= page_table_entry_count) {
            return -1;
        } else {
            regt PTBASE = page_number * (PAGE_TABLE_ENTRY_SIZE + 1) + MMT_POINTER;
            octet* o = flat(PTBASE);
            regt page_table_entry = mem_to_reg64(*reinterpret_cast<regt*>(o));

            if(!(page_table_entry & MMT_VALID_MASK) || !(page_table_entry & MMT_ALLOCATED_MASK) /*|| ((page_table_entry & reason) != reason) */) {
                return -1;
            } else {
                offset = (page_table_entry & ~MMT_PAGE_MASK) + (offset & MMT_PAGE_MASK);
            }

            return offset;
        }
    } else {
        return offset;
    }
}

regt flat_to_mem(regt offset) {
    fprintf(stderr, "Unimplemented: flat-to-mem() reverse lookup\n");
    unimplemented(0000);
    return -1;
}

void load_mmt(regt pointer) {
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        MMT_POINTER = (regt)(flat(pointer) - flat(0));
        if(MMT_POINTER >= MEM_COUNT) {
            fprintf(stderr, "\n -- invalid memory map address %016llx: must be an address in real memory\n", MMT_POINTER);
            call_interrupt(0x04, true);
        }
    } else {
        MMT_POINTER = pointer;
        regt tempcount = *flat(pointer);
        MMT_PAGE_BITS = 8 + (tempcount >> 61);
        MMT_PAGE_SIZE = 1 << MMT_PAGE_BITS; // 256 .. 32768
        MMT_PAGE_MASK = MMT_PAGE_SIZE - 1;
        tempcount &= 0x1fffffffffffffff; // mask off top 3 bits
    }
}

/*
memt* flat(regt offset) {
    // look for a device mapped at this address:
    if(offset < MEM_COUNT) {
        /*if(verbosity & 2)
            fprintf(stderr, " -- sysmem offset %llx\n", offset); * /
        return &sysmem[offset];
    } else for(memt dmi = 0; dmi <= 255; ++dmi) {
        regt DMBASE = dmi * DEVICE_MAP_ENTRY_SIZE + DEVICE_MAP_START;
        memt dflags = sysmem[DMBASE];
        if((dflags & 0xc800) == 0xc800) { // DMA is enabled and valid
            regt device_start_address =
                (((regt)sysmem[DMBASE + 12]) << 48)
                | (((regt)sysmem[DMBASE + 13]) << 32)
                | (((regt)sysmem[DMBASE + 14]) << 16)
                | (((regt)sysmem[DMBASE + 15]) << 0);
            regt device_length =
                (((regt)sysmem[DMBASE + 10]) << 16)
                | (((regt)sysmem[DMBASE + 11]) << 0);
            device_length &= 0x00ffffff;
            device_start_address <<= 9;
            device_length <<= 9;
            if(offset >= device_start_address && offset < (device_length + device_start_address)) {
                return &mem_bank[dmi][offset - device_start_address];
            } /* else {
                fprintf(stderr, " -- address %llx is not within %llx - %llx\n", offset, device_start_address, device_length + device_start_address);
            } * /
        }
    }

    if(offset >= MEM_COUNT) {
        fprintf(stderr, " -- attempt to access unassigned location in flat memory: 0x%016llx >= 0x%016llx\n", offset, (regt)MEM_COUNT);
        dump_registers();
        octet* buffer = (octet*)malloc(sizeof(octet) * 32);
        for(memt bi = 0; bi < 32; ++bi)
            buffer[bi] = 0;
        fgets(buffer, 31, stdin);
        return NULL;
    } else {
        return &sysmem[offset];
    }
}*/

octet* flat(regt offset) {
    memt bank = (offset >> 40) & 0xff;
    regt position = offset & 0xffffffffff;
    /*if(verbosity)
        fprintf(stderr, "bank %02x position %llx\n", bank, position);*/
    if(position >= mem_size[bank]) {
        fprintf(stderr, " -- attempt to access unassigned location in flat memory: 0x%016llx >= 0x%016llx\n", offset, (regt)mem_size[bank] + (offset & 0x0000ff0000000000));
        dump_registers();
        return NULL;
    }
    return &(mem_bank[bank][position * 2]);
}

void copy_block(regt src, regt dst, regt size) {
    if(verbosity)
        printf("BCOPY: block-copying %u words from %llx to %llx\n", size, src, dst);
    
    regt segmask;
    regt blocksize;
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        blocksize = 0x000001000000000000;
        segmask = ~0x000000ffffffffffff;
    } else {
        regt pagesize = *flat(MMT_POINTER) >> 13; // top 3 bits only
        blocksize = (1 << (pagesize + 8));
        segmask = ~(blocksize - 1);
    }
    segmask = segmask & 0x0000ffffffffffff;

    if(((src + size) & segmask) == ((dst + size) & segmask)
    && (src & segmask) == (dst & segmask)
    && ((src + size) & segmask) == (src & segmask)
    && ((dst + size) & segmask) == (dst & segmask)) {
        // direct copy is possible
        octet* src_start = mem(src, MMT_READABLE_MASK);
        octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
        memcpy(dst_start, src_start, size * sizeof(memt));
    } else {
        // get messy
        regt src_offset = src & ~segmask;
        regt dst_offset = dst & ~segmask;
        if(src_offset == dst_offset) { // alignment within pages
            octet* src_start = mem(src, MMT_READABLE_MASK);
            octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
            memt preblock_size = blocksize - src_offset;
            memcpy(dst_start, src_start, preblock_size * sizeof(memt));
            size -= preblock_size;
            src += preblock_size;
            dst += preblock_size;
            while(size >= blocksize) {
                src_start = mem(src, MMT_READABLE_MASK);
                dst_start = mem(dst, MMT_WRITABLE_MASK);
                memcpy(dst_start, src_start, blocksize * sizeof(memt));
                src += blocksize;
                dst += blocksize;
                size -= blocksize;
            }
            if(size > 0) {
                src_start = mem(src, MMT_READABLE_MASK);
                dst_start = mem(dst, MMT_WRITABLE_MASK);
                memcpy(dst_start, src_start, size * sizeof(memt));
            }
        } else { // pages are misaligned AND we're copying across page boundaries
            /*
                Copying occurs in four phases, repeating the middle two phases until the last page:

                                |--------| |--------|
                          _____ |        | |        | 
                preblock |_____ |::::::::| |        | 
            misalignment |      |::::::::| |        | _____
                         |_____ |::::::::| |::::::::| _____| preblock
                          _____ |--------| |--------|      |
               remainder |      |::::::::| |::::::::| _____| misalignment
                         |_____ |::::::::| |::::::::| _____
            misalignment |      |::::::::| |::::::::|      |
                         |_____ |::::::::| |::::::::| _____| remainder 
                          _____ |--------| |--------|      |
               remainder |      |::::::::| |::::::::| _____| misalignment
                         |_____ |::::::::| |::::::::| _____
            misalignment |      |::::::::| |::::::::|      |
                         |_____ |::::::::| |::::::::| _____| remainder 
                          _____ |--------| |--------| _____
                    tail |      |::::::::| |::::::::|      |
                         '----- |''''''''| |::::::::| _____| misalignment
                                |        | |::::::::|      |
                                |        | |''''''''| -----' tail
                                |--------| |--------|
            
                An 'offset' is a start address minus the page start; i.e., the offset within the page.
                The preblock size is the page size minus the smaller offset: either the destination or source offset.
                The misalignment size is the amount of data remaining in the first page that wasn't covered by the preblock.
                The remainder size is the rest of a page.
                The tail size is the minimum offset, but computed with end addresses rather than start addresses.

                If the destination offset is first, then the inner loop is:
                    - copy misalignment
                    - get next destination page
                    - copy remainder
                    - get next source page

                If the source offset is first then:
                    - copy misalignment
                    - get next source page
                    - copy remainder
                    - get next destination page
                
            */
            octet* src_start = mem(src, MMT_READABLE_MASK);
            octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
            regt src_preblock_size = blocksize - src_offset;
            regt dst_preblock_size = blocksize - dst_offset;
            regt min_preblock_size = (src_preblock_size < dst_preblock_size ? src_preblock_size : dst_preblock_size);
            
            memcpy(dst_start, src_start, min_preblock_size * sizeof(memt));
            size -= min_preblock_size;
            src += min_preblock_size;
            dst += min_preblock_size;

            regt min_offset = (src_offset < dst_offset ? src_offset : dst_offset);
            regt remainder = min_preblock_size + min_offset;
            regt misalignment = blocksize - remainder;

            if(src_offset < dst_offset) {
                dst_start = mem(dst, MMT_WRITABLE_MASK);
                src_start += min_preblock_size * sizeof(memt);
            } else {
                src_start = mem(src, MMT_READABLE_MASK);
                dst_start += min_preblock_size * sizeof(memt);
            }

            while(size >= blocksize) {
                memcpy(dst_start, src_start, misalignment * sizeof(memt));
                size -= misalignment;
                src += misalignment;
                dst += misalignment;

                if(src_offset < dst_offset) {
                    src_start = mem(src, MMT_READABLE_MASK);
                    dst_start += misalignment * sizeof(memt);
                } else {
                    dst_start = mem(dst, MMT_WRITABLE_MASK);
                    src_start += misalignment * sizeof(memt);
                }

                memcpy(dst_start, src_start, remainder * sizeof(memt));
                size -= remainder;
                src += remainder;
                dst += remainder;

                if(src_offset < dst_offset) {
                    src_start = mem(src, MMT_READABLE_MASK);
                    dst_start += remainder * sizeof(memt);
                } else {
                    dst_start = mem(dst, MMT_WRITABLE_MASK);
                    src_start += remainder * sizeof(memt);
                }
            }

            if(size > 0) {
                // copy tail:
                memcpy(dst_start, src_start, size * sizeof(memt));
            }
        }
    }
}

// fill memory at start with repeats of value, allowing for partial repeats
inline void memset64(octet* start, regt value, regt count) {
    regt i = 0;
    regt be_value = reg_to_mem64(value);
    while(count >= 8) {
        *reinterpret_cast<regt*>(start + i) = be_value;
        i += 8;
        count -= 8;
    }
    if(count)
        memcpy(start + i, &be_value, count);
}

void fill_block(regt value, regt dst, regt size) {
    if(verbosity)
        printf("BFILL: filling %u words at %llx with %llx\n", size, value, dst);
    
    regt segmask;
    regt blocksize;
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        blocksize = 0x000001000000000000;
        segmask = ~0x000000ffffffffffff;
    } else {
        regt pagesize = *flat(MMT_POINTER) >> 13; // top 3 bits only
        blocksize = (1 << (pagesize + 8));
        segmask = ~(blocksize - 1);
    }
    segmask = segmask & 0x0000ffffffffffff;

    if(((dst + size) & segmask) == (dst & segmask)) {
        // direct fill is possible
        octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
        memset64(dst_start, value, size * sizeof(memt));
    } else {
        // get messy
        regt dst_offset = dst & ~segmask;
        
        octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
        memt preblock_size = blocksize - dst_offset;
        memset64(dst_start, value, preblock_size * sizeof(memt));
        size -= preblock_size;
        dst += preblock_size;
        while(size >= blocksize) {
            dst_start = mem(dst, MMT_WRITABLE_MASK);
            memset64(dst_start, value, blocksize * sizeof(memt));
            dst += blocksize;
            size -= blocksize;
        }
        if(size > 0) {
            dst_start = mem(dst, MMT_WRITABLE_MASK);
            memset64(dst_start, value, size * sizeof(memt));
        }
    }
}

octet* clone_block(regt src, regt size) {
    if(verbosity)
        printf("BCLONE: creating array from %u words at %llx\n", size, src);
    
    regt segmask;
    regt blocksize;
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        blocksize = 0x000001000000000000;
        segmask = ~0x000000ffffffffffff;
    } else {
        regt pagesize = *flat(MMT_POINTER) >> 13; // top 3 bits only
        blocksize = (1 << (pagesize + 8));
        segmask = ~(blocksize - 1);
    }
    segmask = segmask & 0x0000ffffffffffff;

    octet* dst_0 = (octet*)malloc(size * sizeof(memt));
    octet* dst = dst_0;

    if(((src + size) & segmask) == (src & segmask)) {
        // source is entirely within one segment
        octet* src_start = mem(src, MMT_READABLE_MASK);
        octet* dst_start = dst;
        memcpy(dst_start, src_start, size * sizeof(memt));
    } else {
        // get messy
        regt src_offset = src & ~segmask;

        octet* src_start = mem(src, MMT_READABLE_MASK);
        octet* dst_start = dst;
        memt preblock_size = blocksize - src_offset;
        memcpy(dst_start, src_start, preblock_size * sizeof(memt));
        size -= preblock_size;
        src += preblock_size;
        dst += preblock_size;
        while(size >= blocksize) {
            src_start = mem(src, MMT_READABLE_MASK);
            dst_start = dst;
            memcpy(dst_start, src_start, blocksize * sizeof(memt));
            src += blocksize;
            dst += blocksize;
            size -= blocksize;
        }
        if(size > 0) {
            src_start = mem(src, MMT_READABLE_MASK);
            dst_start = dst;
            memcpy(dst_start, src_start, size * sizeof(memt));
        }
    }

    return dst_0;
}

void store_block(octet* src, regt dst, regt size) {
    if(verbosity)
        printf("BCOPY: block-copying %u words from %llx to %llx\n", size, src, dst);
    
    regt segmask;
    regt blocksize;
    if(reg[csrSTATUS] & FLAG_MASK_M) {
        blocksize = 0x000001000000000000;
        segmask = ~0x000000ffffffffffff;
    } else {
        regt pagesize = *flat(MMT_POINTER) >> 13; // top 3 bits only
        blocksize = (1 << (pagesize + 8));
        segmask = ~(blocksize - 1);
    }

    if(((dst + size) & segmask) == (dst & segmask)) {
        // direct copy is possible
        octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
        memcpy(dst_start, src, size * sizeof(memt));
    } else {
        // get messy
        regt dst_offset = dst & ~segmask;
        
        octet* src_start = src;
        octet* dst_start = mem(dst, MMT_WRITABLE_MASK);
        memt preblock_size = blocksize - dst_offset;
        memcpy(dst_start, src_start, preblock_size * sizeof(memt));
        size -= preblock_size;
        src += preblock_size;
        dst += preblock_size;
        while(size >= blocksize) {
            src_start = src;
            dst_start = mem(dst, MMT_WRITABLE_MASK);
            memcpy(dst_start, src_start, blocksize * sizeof(memt));
            src += blocksize;
            dst += blocksize;
            size -= blocksize;
        }
        if(size > 0) {
            src_start = src;
            dst_start = mem(dst, MMT_WRITABLE_MASK);
            memcpy(dst_start, src_start, size * sizeof(memt));
        }
    }
}
