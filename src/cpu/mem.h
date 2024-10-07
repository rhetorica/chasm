
#ifndef MEM_H
#define MEM_H

#include "cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>

extern regt MMT_POINTER;

extern regt MMT_PAGE_SIZE;
extern regt MMT_PAGE_MASK;
extern regt MMT_PAGE_BITS;
extern regt MMT_PAGE_COUNT;

extern regt PAGE_FAULT_ADDRESS;
extern regt PAGE_FAULT_POINTER;
extern regt PAGE_FAULT_ENTRY;

extern octet* sysmem;

// words of RAM:
#define MEM_COUNT 1024 * 1024

void init_memory();

void dump_memory(regt offset, regt length);
octet* mem(regt, memt);
regt mem_to_flat(regt offset);
regt flat_to_mem(regt offset);
void load_mmt(regt pointer);
octet* flat(regt offset);

static inline regt loadvar(octet width, regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- var-loading %d-bit value from 0x%016llx\n", width * 16, offset);
    #endif
    octet* o = mem(offset, MMT_READABLE_MASK);
    if(width > 1) {
        octet* o_end = mem(offset + width - 1, MMT_READABLE_MASK);
        if(width == 4) {
            if(o_end - o == 6) {
                // memory is actually contiguous; we can use pointer math and skip further lookups:
                return mem_to_reg64(*reinterpret_cast<regt*>(o));
                /* return ((regt)*(o    ) << 56)
                     | ((regt)*(o + 1) << 48)
                     | ((regt)*(o + 2) << 40)
                     | ((regt)*(o + 3) << 32)
                     | ((regt)*(o + 4) << 24)
                     | ((regt)*(o + 5) << 16)
                     | ((regt)*(o + 6) <<  8)
                     | ((regt)*(o + 7) <<  0); */
            } else {
                // memory is discontinguous; be safe and make additional mem() calls
                octet* o2 = mem(offset + 1, MMT_READABLE_MASK);
                octet* o3 = mem(offset + 2, MMT_READABLE_MASK);
                return ((regt)*(o        ) << 56)
                     | ((regt)*(o     + 1) << 48)
                     | ((regt)*(o2       ) << 40)
                     | ((regt)*(o2    + 1) << 32)
                     | ((regt)*(o3       ) << 24)
                     | ((regt)*(o3    + 1) << 16)
                     | ((regt)*(o_end    ) <<  8)
                     | ((regt)*(o_end + 1) <<  0);
            }
        } else {
            if(o_end - o == 2) {
                return mem_to_reg32(*reinterpret_cast<dwordt*>(o));
            } else {
                return    ((regt)*(o        ) << 24)
                        | ((regt)*(o     + 1) << 16)
                        | ((regt)*(o_end    ) <<  8)
                        | ((regt)*(o_end + 1) <<  0);
            }
        }
    } else {
        /*return ((regt)*(o)    << 8)
             | ((regt)*(o + 1)    );*/
        return mem_to_reg16(*reinterpret_cast<memt*>(o));
    }
}

static inline regt loadvarx(octet width, regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- var-loading %d-bit (X) from 0x%016llx\n", width * 16, offset);
    #endif
    octet* o = mem(offset, MMT_EXECUTABLE_MASK);
    if(width > 1) {
        octet* o_end = mem(offset + width - 1, MMT_EXECUTABLE_MASK);
        if(width == 4) {
            if(o_end - o == 6) {
                // memory is actually contiguous; we can use pointer math and skip further lookups:
                return mem_to_reg64(*reinterpret_cast<regt*>(o));
                /* return ((regt)*(o    ) << 56)
                     | ((regt)*(o + 1) << 48)
                     | ((regt)*(o + 2) << 40)
                     | ((regt)*(o + 3) << 32)
                     | ((regt)*(o + 4) << 24)
                     | ((regt)*(o + 5) << 16)
                     | ((regt)*(o + 6) <<  8)
                     | ((regt)*(o + 7) <<  0); */
            } else {
                // memory is discontinguous; be safe and make additional mem() calls
                octet* o2 = mem(offset + 1, MMT_EXECUTABLE_MASK);
                octet* o3 = mem(offset + 2, MMT_EXECUTABLE_MASK);
                return ((regt)*(o        ) << 56)
                     | ((regt)*(o     + 1) << 48)
                     | ((regt)*(o2       ) << 40)
                     | ((regt)*(o2    + 1) << 32)
                     | ((regt)*(o3       ) << 24)
                     | ((regt)*(o3    + 1) << 16)
                     | ((regt)*(o_end    ) <<  8)
                     | ((regt)*(o_end + 1) <<  0);
            }
        } else {
            if(o_end - o == 2) {
                return mem_to_reg32(*reinterpret_cast<dwordt*>(o));
            } else {
                return    ((regt)*(o        ) << 24)
                        | ((regt)*(o     + 1) << 16)
                        | ((regt)*(o_end    ) <<  8)
                        | ((regt)*(o_end + 1) <<  0);
            }
        }
    } else {
        /*return ((regt)*(o)    << 8)
             | ((regt)*(o + 1)    );*/
        return mem_to_reg16(*reinterpret_cast<memt*>(o));
    }
}

static inline void storevar(octet width, regt value, regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- var-storing %d-bit value %llx from 0x%016llx\n", width * 16, value, offset);
    #endif
    octet* o = mem(offset, MMT_WRITABLE_MASK);
    if(width > 1) {
        octet* o_end = mem((offset + width - 1), MMT_WRITABLE_MASK);
        if(width == 4) {
            if(o_end - o == 6) {
                // memory is actually contiguous; we can use pointer math and skip further lookups:
                *(reinterpret_cast<regt*>(o)) = reg_to_mem64(value);
                /*
                *o       = (value >> 56) & 0xff;
                *(o + 1) = (value >> 48) & 0xff;
                *(o + 2) = (value >> 40) & 0xff;
                *(o + 3) = (value >> 32) & 0xff;
                *(o + 4) = (value >> 24) & 0xff;
                *(o + 5) = (value >> 16) & 0xff;
                *(o + 6) = (value >>  8) & 0xff;
                *(o + 7) = (value      ) & 0xff;
                */
            } else {
                // memory is discontinguous; be safe and make additional mem() calls
                octet* o2 = mem(offset + 1, MMT_WRITABLE_MASK);
                octet* o3 = mem(offset + 2, MMT_WRITABLE_MASK);
                * o          = (value >> 56) & 0xff;
                *(o + 1)     = (value >> 48) & 0xff;
                * o2         = (value >> 40) & 0xff;
                *(o2 + 1)    = (value >> 32) & 0xff;
                * o3         = (value >> 24) & 0xff;
                *(o3 + 1)    = (value >> 16) & 0xff;
                *o_end       = (value >>  8) & 0xff;
                *(o_end + 1) = (value      ) & 0xff;
            }
        } else {
            if(o_end - o == 2) {
                *(reinterpret_cast<dwordt*>(o)) = reg_to_mem32(value);
            } else {
                * o          = (value >> 24) & 0xff;
                *(o + 1)     = (value >> 16) & 0xff;
                * o_end      = (value >>  8) & 0xff;
                *(o_end + 1) = (value      ) & 0xff;
            }
        }
    } else {
        *(reinterpret_cast<memt*>(o)) = reg_to_mem16(value);
        /*
        * o      = (value >> 8) & 0xff;
        *(o + 1) = (value     ) & 0xff;
        */
    }
}

static inline regt loadv64(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 64-bit value from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_READABLE_MASK);
    octet* o_end = mem(offset + 3, MMT_READABLE_MASK);
    if(o_end - o == 6) {
        // memory is actually contiguous; we can use pointer math and skip further lookups:
        return mem_to_reg64(*reinterpret_cast<regt*>(o));
        /* return ((regt)*(o    ) << 56)
                | ((regt)*(o + 1) << 48)
                | ((regt)*(o + 2) << 40)
                | ((regt)*(o + 3) << 32)
                | ((regt)*(o + 4) << 24)
                | ((regt)*(o + 5) << 16)
                | ((regt)*(o + 6) <<  8)
                | ((regt)*(o + 7) <<  0); */
    } else {
        // memory is discontinguous; be safe and make additional mem() calls
        octet* o2 = mem(offset + 1, MMT_READABLE_MASK);
        octet* o3 = mem(offset + 2, MMT_READABLE_MASK);
        return ((regt)*(o        ) << 56)
                | ((regt)*(o     + 1) << 48)
                | ((regt)*(o2       ) << 40)
                | ((regt)*(o2    + 1) << 32)
                | ((regt)*(o3       ) << 24)
                | ((regt)*(o3    + 1) << 16)
                | ((regt)*(o_end    ) <<  8)
                | ((regt)*(o_end + 1) <<  0);
    }
}

static inline regt loadv32(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 32-bit value from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_READABLE_MASK);
    octet* o_end = mem(offset + 1, MMT_READABLE_MASK);
    if(o_end - o == 2) {
        return mem_to_reg32(*reinterpret_cast<dwordt*>(o));
    } else {
        return    ((regt)*(o        ) << 24)
                | ((regt)*(o     + 1) << 16)
                | ((regt)*(o_end    ) <<  8)
                | ((regt)*(o_end + 1) <<  0);
    }
}

static inline regt loadv16(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 16-bit value from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_READABLE_MASK);
    return mem_to_reg16(*reinterpret_cast<memt*>(o));
}

static inline regt loadv64x(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 64-bit (X) from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_EXECUTABLE_MASK);
    octet* o_end = mem(offset + 3, MMT_EXECUTABLE_MASK);
    if(o_end - o == 6) {
        // memory is actually contiguous; we can use pointer math and skip further lookups:
        return mem_to_reg64(*reinterpret_cast<regt*>(o));
        /* return ((regt)*(o    ) << 56)
                | ((regt)*(o + 1) << 48)
                | ((regt)*(o + 2) << 40)
                | ((regt)*(o + 3) << 32)
                | ((regt)*(o + 4) << 24)
                | ((regt)*(o + 5) << 16)
                | ((regt)*(o + 6) <<  8)
                | ((regt)*(o + 7) <<  0); */
    } else {
        // memory is discontinguous; be safe and make additional mem() calls
        octet* o2 = mem(offset + 1, MMT_EXECUTABLE_MASK);
        octet* o3 = mem(offset + 2, MMT_EXECUTABLE_MASK);
        return ((regt)*(o        ) << 56)
                | ((regt)*(o     + 1) << 48)
                | ((regt)*(o2       ) << 40)
                | ((regt)*(o2    + 1) << 32)
                | ((regt)*(o3       ) << 24)
                | ((regt)*(o3    + 1) << 16)
                | ((regt)*(o_end    ) <<  8)
                | ((regt)*(o_end + 1) <<  0);
    }
}

static inline regt loadv32x(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 32-bit (X) from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_EXECUTABLE_MASK);
    octet* o_end = mem(offset + 1, MMT_EXECUTABLE_MASK);
    if(o_end - o == 2) {
        return mem_to_reg32(*reinterpret_cast<dwordt*>(o));
    } else {
        return    ((regt)*(o        ) << 24)
                | ((regt)*(o     + 1) << 16)
                | ((regt)*(o_end    ) <<  8)
                | ((regt)*(o_end + 1) <<  0);
    }
}

static inline regt loadv16x(regt offset) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- loading 16-bit (X) from 0x%016llx\n", offset);
    #endif
    octet* o = mem(offset, MMT_EXECUTABLE_MASK);
    return mem_to_reg16(*reinterpret_cast<memt*>(o));
}

static inline void storev16(regt offset, regt value) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- storing 16-bit value %llx at 0x%016llx\n", value, offset);
    #endif
    octet* o = mem(offset, MMT_WRITABLE_MASK);
    *(reinterpret_cast<memt*>(o)) = reg_to_mem16(value);
    /** o      = (value >> 8) & 0xff;
    *(o + 1) = (value     ) & 0xff; */
}

static inline void storev32(regt offset, regt value) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- storing 32-bit value %llx at 0x%016llx\n", value, offset);
    #endif
    octet* o = mem(offset, MMT_WRITABLE_MASK);
    octet* o_end = mem(offset + 1, MMT_WRITABLE_MASK);
    if(o_end - o == 2) {
        *(reinterpret_cast<dwordt*>(o)) = reg_to_mem32(value);
    } else {
        * o          = (value >> 24) & 0xff;
        *(o + 1)     = (value >> 16) & 0xff;
        * o_end      = (value >>  8) & 0xff;
        *(o_end + 1) = (value      ) & 0xff;
    }
}

static inline void storev64(regt offset, regt value) {
    #ifdef VERBOSITY_MODE
        if(verbosity > 2) printf(" -- storing 64-bit value %llx at 0x%016llx\n", value, offset);
    #endif
    octet* o = mem(offset, MMT_WRITABLE_MASK);
    octet* o_end = mem((offset + 3), MMT_WRITABLE_MASK);
    if(o_end - o == 6) {
        // memory is actually contiguous; we can use pointer math and skip further lookups:
        
        *(reinterpret_cast<regt*>(o)) = reg_to_mem64(value);
        /*
        *o       = (value >> 56) & 0xff;
        *(o + 1) = (value >> 48) & 0xff;
        *(o + 2) = (value >> 40) & 0xff;
        *(o + 3) = (value >> 32) & 0xff;
        *(o + 4) = (value >> 24) & 0xff;
        *(o + 5) = (value >> 16) & 0xff;
        *(o + 6) = (value >>  8) & 0xff;
        *(o + 7) = (value      ) & 0xff; */
    } else {
        // memory is discontinguous; be safe and make additional mem() calls
        octet* o2 = mem(offset + 1, MMT_WRITABLE_MASK);
        octet* o3 = mem(offset + 2, MMT_WRITABLE_MASK);
        * o          = (value >> 56) & 0xff;
        *(o + 1)     = (value >> 48) & 0xff;
        * o2         = (value >> 40) & 0xff;
        *(o2 + 1)    = (value >> 32) & 0xff;
        * o3         = (value >> 24) & 0xff;
        *(o3 + 1)    = (value >> 16) & 0xff;
        *o_end       = (value >>  8) & 0xff;
        *(o_end + 1) = (value      ) & 0xff;
    }
}

static inline void getv128(unsigned short regn, regt offset) {
    reg[regn & ~0x10] = loadv64(offset);
    reg[regn | 0x10] = loadv64(offset + 4);
}

static inline void storev128(unsigned short regn, regt offset) {
    storev64(reg[regn], offset);
    storev64(reg[regn | 0x10], offset + 4);
}

static inline void push128(regt ar_value, regt dr_value) {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to push while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        storev64(reg[csrSP] - 4, ar_value);
        storev64(reg[csrSP] - 8, dr_value);
        reg[csrSP] -= 8;
    }
}

static inline void pop128(memt regnum) {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to pop while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        reg[regnum] = loadv64(reg[csrSP]);
        reg[regnum | 0x10] = loadv64(reg[csrSP] + 4);
        reg[csrSP] += 8;
    }
}

static inline void push64(regt value) {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to push while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        storev64(reg[csrSP] - 4, value);
        reg[csrSP] -= 4;
    }
}

static inline regt pop64() {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to pop while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        regt temp = loadv64(reg[csrSP]);
        reg[csrSP] += 4;
        return temp;
    }
}

static inline void push32(regt value) {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to push while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        storev32(reg[csrSP] - 2, value);
        reg[csrSP] -= 2;
    }
}

static inline regt pop32() {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to pop while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        regt temp = loadv32(reg[csrSP]);
        reg[csrSP] += 2;
        return temp;
    }
}

static inline void push16(regt value) {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to push while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        storev16(reg[csrSP] - 1, value);
        reg[csrSP] -= 1;
    }
}

static inline regt pop16() {
    if(reg[csrSP] == 0 || reg[csrSP] == 0xffffffffffffffff) {
        fprintf(stderr, " -- attempt to pop while SP is 0x%016llx!\n", reg[csrSP]);
        exit(1);
    } else {
        regt temp = loadv16(reg[csrSP]);
        reg[csrSP] += 1;
        return temp;
    }
}

static inline void push_all_data() {
    push64(reg[csrHd]);
    push64(reg[csrGd]);
    push64(reg[csrFd]);
    push64(reg[csrEd]);
    push64(reg[csrDd]);
    push64(reg[csrCd]);
    push64(reg[csrBd]);
    push64(reg[csrAd]);
    push64(reg[csrH]);
    push64(reg[csrG]);
    push64(reg[csrF]);
    push64(reg[csrE]);
    push64(reg[csrD]);
    push64(reg[csrC]);
    push64(reg[csrB]);
    push64(reg[csrA]);
}

static inline void push_all_special() {
    regt sp_temp = reg[csrSP];
    // push64(reg[csrSTATUS]);
    push64(reg[csrPI]);
    push64(reg[csrCB]);
    push64(reg[csrDB]);
    push64(sp_temp);
    push64(reg[csrSB]);
    push64(reg[csrRA]);
    // push64(reg[csrPC]);
}

static inline void pop_all_data() {
    reg[csrA] = pop64(); //loadv64(reg[csrSP]);
    reg[csrB] = pop64(); //loadv64(reg[csrSP] + 4);
    reg[csrC] = pop64(); //loadv64(reg[csrSP] + 8);
    reg[csrD] = pop64(); //loadv64(reg[csrSP] + 12);
    reg[csrE] = pop64(); //loadv64(reg[csrSP] + 16);
    reg[csrF] = pop64(); //loadv64(reg[csrSP] + 20);
    reg[csrG] = pop64(); //loadv64(reg[csrSP] + 24);
    reg[csrH] = pop64(); //loadv64(reg[csrSP] + 28);
    reg[csrAd] = pop64();
    reg[csrBd] = pop64();
    reg[csrCd] = pop64();
    reg[csrDd] = pop64();
    reg[csrEd] = pop64();
    reg[csrFd] = pop64();
    reg[csrGd] = pop64();
    reg[csrHd] = pop64();
    // reg[csrSP] += 64;
}

static inline void pop_all_special() {
    // reg[csrPC] = pop64(); //loadv64(reg[csrSP]);
    reg[csrRA] = pop64(); //loadv64(reg[csrSP] + 4);
    reg[csrSB] = pop64(); //loadv64(reg[csrSP] + 8);
    regt sp_temp = pop64(); //loadv64(reg[csrSP] + 12);
    reg[csrDB] = pop64(); //loadv64(reg[csrSP] + 16);
    reg[csrCB] = pop64(); //loadv64(reg[csrSP] + 20);
    reg[csrPI] = pop64(); //loadv64(reg[csrSP] + 24);
    // reg[csrSTATUS] = pop64(); //loadv64(reg[csrSP] + 28);

    reg[csrSP] = sp_temp;
    //reg[csrSP] += 32;
}

void copy_block(regt src, regt dst, regt size);
void fill_block(regt value, regt dst, regt size);
octet* clone_block(regt src, regt size);
void store_block(octet* src, regt dst, regt size);

// pinched from UAE via Hatari via Previous
/* Allocate aligned memory: */
static inline uint8_t* malloc_aligned(size_t size) {
#if defined(HAVE_POSIX_MEMALIGN)
	void* result = NULL;
	posix_memalign(&result, 0x10000, size);
	return (uint8_t*)result;
#elif defined(HAVE_ALIGNED_ALLOC)
	return (uint8_t*)aligned_alloc(0x10000, size);
#elif defined(HAVE__ALIGNED_MALLOC)
	return (uint8_t*)_aligned_malloc(0x10000, size);
#else
	return (uint8_t*)malloc(size);
#endif
}

#endif // MEM_H