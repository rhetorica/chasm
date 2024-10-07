#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <SDL3/SDL.h>

#include "../types.h"
#include "cpu.h"
#include "mem.hpp"
#include "../dev.h"
#include "mem.h"
#include "ops.h"
#include "../video.h"
#include "../main.h"

regt ins_count = 0;

regt* reg;

void dump_registers() {
    fprintf(stderr,"\n A=%016llx\t  B=%016llx\t C=%016llx\t     D=%016llx",   reg[csrA],  reg[csrB],   reg[csrC],  reg[csrD]);
    fprintf(stderr,"\n E=%016llx\t  F=%016llx\t G=%016llx\t     H=%016llx",   reg[csrE],  reg[csrF],   reg[csrG],  reg[csrH]);
    fprintf(stderr,"\nAd=%016llx\t Bd=%016llx\tCd=%016llx\t    Dd=%016llx",   reg[csrAd], reg[csrBd],  reg[csrCd], reg[csrDd]);
    fprintf(stderr,"\nEd=%016llx\t Fd=%016llx\tGd=%016llx\t    Hd=%016llx",   reg[csrEd], reg[csrFd],  reg[csrGd], reg[csrHd]);
    fprintf(stderr,"\nPC=%016llx\t RA=%016llx\tSB=%016llx\t    SP=%016llx",   reg[csrPC], reg[csrRA],  reg[csrSB], reg[csrSP]);
    fprintf(stderr,"\nDB=%016llx\t CB=%016llx\tPI=%016llx\tSTATUS=%016llx\n", reg[csrDB], reg[csrCB],  reg[csrPI], reg[csrSTATUS]);
}

void unimplemented(memt ins) {
    fprintf(stderr, "\n -- execution aborted on unimplemented instruction: %04x before PC=%llx\n", ins, reg[csrPC]);
    dump_registers();
    exit(1);
}


memt interrupts_pending = 0; // how many different interrupt numbers are currently pending?

void call_interrupt(memt interrupt_number, int from_hardware) {
    regt IBASE = (INTERRUPT_TABLE_START + INTERRUPT_SIZE * interrupt_number) * sizeof(memt);
    
    memt ring = ((reg[csrSTATUS] & FLAG_MASK_RING) >> FLAG_R0);
    memt ring_mask = 1 << ((reg[csrSTATUS] & FLAG_MASK_RING) >> FLAG_R0);

    if(!from_hardware) {
        memt call_mask = sysmem[IBASE + 3];
        if(!(call_mask & ring_mask) && (ring > 0)) {
            fprintf(stderr, " -- blocked interrupt %02x at %016llx from unprivileged task\n", interrupt_number, reg[csrPC] - 1);    
            return;
        }
    }

    int syscall_mode = reg[csrSTATUS] & FLAG_MASK_RM2;
    memt real_interrupt_number = interrupt_number;
    if(interrupt_number == 0) {
        fprintf(stderr, " -- interrupt 0: halt at %016llx\n", reg[csrPC] - 1);
        reg[csrSTATUS] |= FLAG_MASK_H;
    } else if(interrupt_number == 4) {
        fprintf(stderr, " -- interrupt 4: invalid MMT address at %016llx\n", reg[csrPC] - 1);
        reg[csrSTATUS] |= FLAG_MASK_H;
    } else {
        if(interrupt_number == 255) { // answer pending interrupts
            if(interrupts_pending == 0) {
                reg[csrSTATUS] &= ~FLAG_MASK_IP;
                return;
            }
            memt iscan = 1;
            while(iscan < 255) {
                regt ISBASE = INTERRUPT_TABLE_START + INTERRUPT_SIZE * iscan;
                memt interrupt_flags = sysmem[ISBASE];
                if((interrupt_flags & INT_PENDING)) {
                    if(interrupt_flags & INT_ENABLED) {
                        interrupt_number = iscan;
                        break;
                    } else {
                        // remove ghost interrupt:
                        --interrupts_pending;
                        sysmem[ISBASE] &= ~INT_PENDING;
                    }
                }
                ++iscan;
            }
            --interrupts_pending;
            if(interrupt_number == 255) {
                // oops, maybe a ghost interrupt brought us here (pending but disabled)
                interrupts_pending = 0;
                return;
            }
            if(interrupts_pending == 0)
                reg[csrSTATUS] &= ~FLAG_MASK_IP;
        }
        memt interrupt_flags = sysmem[IBASE] << 8;
        if(interrupt_flags & INT_ENABLED) {
            memt device_number = sysmem[IBASE + 1];
            memt interrupt_mask = sysmem[IBASE + 2];
            regt interrupt_address = mem_to_reg64(*reinterpret_cast<regt*>(&sysmem[IBASE + 8]));
            /*    ((regt)sysmem[IBASE +  8] << 56)
              | ((regt)sysmem[IBASE +  9] << 48)
              | ((regt)sysmem[IBASE + 10] << 40)
              | ((regt)sysmem[IBASE + 11] << 32)
              | ((regt)sysmem[IBASE + 12] << 24)
              | ((regt)sysmem[IBASE + 13] << 16)
              | ((regt)sysmem[IBASE + 14] <<  8)
              | ((regt)sysmem[IBASE + 15] <<  0); */
            regt interrupt_mmtp = mem_to_reg64(*reinterpret_cast<regt*>(&sysmem[IBASE + 24]));
            /*    ((regt)sysmem[IBASE + 24] << 56)
              | ((regt)sysmem[IBASE + 25] << 48)
              | ((regt)sysmem[IBASE + 26] << 40)
              | ((regt)sysmem[IBASE + 27] << 32)
              | ((regt)sysmem[IBASE + 28] << 24)
              | ((regt)sysmem[IBASE + 29] << 16)
              | ((regt)sysmem[IBASE + 30] <<  8)
              | ((regt)sysmem[IBASE + 31] <<  0);*/

            // we only answer interrupts if they're unmasked (or we called 255 explicitly)
            // also we must not be inside an interrupt handler currently (in which case FLAG_MASK_CI would be set)
            if((interrupt_mask & ring_mask || real_interrupt_number == 255) && !(reg[csrSTATUS] & FLAG_MASK_CI)) {
                if(interrupt_flags & INT_DEVICE) {
                    // Type F (Device Control) Interrupt
                    call_device(device_number, reg);
                    // reg[csrPC] -= 1; // why though

                } else {
                    sysmem[IBASE] &= ~INT_PENDING;
                    reg[csrSTATUS] = (reg[csrSTATUS] & ~FLAG_MASK_CI) | ((regt)interrupt_number << FLAG_OFFSET_CI);
                    push64(reg[csrPC]);
                    push_all_special();
                    if(!syscall_mode)
                        push_all_data();

                    reg[csrRA] = reg[csrPC] - 1;
                    if(interrupt_number == 0x02) { // page fault
                        reg[csrH] = PAGE_FAULT_ADDRESS;
                        reg[csrG] = PAGE_FAULT_ENTRY;
                        reg[csrF] = PAGE_FAULT_POINTER;
                    }

                    int interrupt_uses_mmt = interrupt_flags & INT_MMT_ENABLE;
                    if(reg[csrSTATUS] & FLAG_MASK_M) {
                        if(interrupt_uses_mmt) {
                            if(MMT_POINTER == interrupt_mmtp) {
                                // Type E (Same-Map) Interrupt
                                push64(reg[csrPC]);
                                push_all_special();
                                if(!syscall_mode)
                                    push_all_data();
                                reg[csrRA] = mem_to_flat(reg[csrSP]);
                                reg[csrSTATUS] &= ~FLAG_MASK_RM0;
                                reg[csrSTATUS] |= FLAG_MASK_RM1;
                                reg[csrPC] = interrupt_address;
                            } else {
                                // Type D (Alternate-Map) Interrupt
                                push64(reg[csrPC]);
                                push_all_special();
                                if(!syscall_mode)
                                    push_all_data();
                                push64(MMT_POINTER);
                                reg[csrRA] = mem_to_flat(reg[csrSP]);
                                MMT_POINTER = interrupt_mmtp;
                                reg[csrSTATUS] |= FLAG_MASK_RM0;
                                reg[csrSTATUS] &= ~FLAG_MASK_RM1;
                                reg[csrPC] = interrupt_address;
                            }
                        } else {
                            // Type C (Map-Escape) Interrupt
                            push64(reg[csrPC]);
                            push_all_special();
                            if(!syscall_mode)
                                push_all_data();
                            reg[csrRA] = reg[csrSP];
                            reg[csrSTATUS] &= ~FLAG_MASK_M;
                            reg[csrPC] = interrupt_address;
                        }
                    } else if(interrupt_uses_mmt) {
                        // Type B (Interrupt Adds Mapping) Interrupt
                        push64(reg[csrPC]);
                        push_all_special();
                        if(!syscall_mode)
                            push_all_data();
                        reg[csrRA] = reg[csrSP];
                        MMT_POINTER = interrupt_mmtp;
                        reg[csrSTATUS] |= FLAG_MASK_M;
                        reg[csrSTATUS] &= ~(FLAG_MASK_RM0 | FLAG_MASK_RM1);
                        reg[csrPC] = interrupt_address;

                    } else {
                        // Type A (Mapless) Interrupt
                        push64(reg[csrPC]);
                        push_all_special();
                        if(!syscall_mode)
                            push_all_data();
                        reg[csrPC] = interrupt_address;
                        reg[csrRA] = reg[csrSP];
                    }
                }
            } else {
                // interrupt is masked
                if(!(interrupt_flags & INT_PENDING))
                    ++interrupts_pending;
                sysmem[IBASE] |= INT_PENDING;
            }
        } else {
            // fully disabled interrupts may never become pending,
            // and should be removed from the list if discovered in the pending state
            if(verbosity)
                printf(" -- ignored call to disabled interrupt 0x%02x\n", interrupt_number);
            if(interrupt_flags & INT_PENDING)
                --interrupts_pending;
            sysmem[IBASE] &= ~INT_PENDING;
        }
    }
}

void interrupt_return() {
    // memt interrupt_number = (reg[csrSTATUS] & FLAG_MASK_CI) >> FLAG_OFFSET_CI;
    // regt IBASE = INTERRUPT_TABLE_START + INTERRUPT_SIZE * interrupt_number;
    // memt interrupt_flags = sysmem[IBASE];
    // int interrupt_uses_mmt = interrupt_flags & INT_MMT_ENABLE;

    int syscall_mode = reg[csrSTATUS] & FLAG_MASK_RM2;

    if(reg[csrSTATUS] & FLAG_MASK_M) {
        if((reg[csrSTATUS] & (FLAG_MASK_RM0 | FLAG_MASK_RM1)) == FLAG_MASK_RM0) {
            // Type D (Alternate-Map)
            reg[csrSTATUS] &= ~(FLAG_MASK_RM0 | FLAG_MASK_M);
            reg[csrSP] = reg[csrRA];
            MMT_POINTER = pop64();
            reg[csrSTATUS] |= FLAG_MASK_M;
            if(!syscall_mode)
                pop_all_data();
            pop_all_special();
            reg[csrPC] = pop64();
        } else if((reg[csrSTATUS] & (FLAG_MASK_RM0 | FLAG_MASK_RM1)) == FLAG_MASK_RM1) {
            // Type E (Same-Map)
            reg[csrSTATUS] &= ~(FLAG_MASK_RM1 | FLAG_MASK_M);
            reg[csrSP] = reg[csrRA];
            if(!syscall_mode)
                pop_all_data();
            pop_all_special();
            reg[csrSTATUS] |= FLAG_MASK_M;
            reg[csrPC] = pop64();
        } else {
            // Type C (Map-Add)
            MMT_POINTER = 0;
            reg[csrSTATUS] &= ~FLAG_MASK_M;
            reg[csrSP] = reg[csrRA];
            if(!syscall_mode)
                pop_all_data();
            pop_all_special();
            reg[csrPC] = pop64();
        }
    } else {
        if(!MMT_POINTER) {
            // Type A (Mapless) Return
            reg[csrSP] = reg[csrRA];
            if(!syscall_mode)
                pop_all_data();
            pop_all_special();
            reg[csrPC] = pop64();
        } else {
            // Type B (Map-Escape)
            reg[csrSTATUS] |= FLAG_MASK_M;
            reg[csrSP] = reg[csrRA];
            if(!syscall_mode)
                pop_all_data();
            pop_all_special();
            reg[csrPC] = pop64();
        }
    }
}

/*
	0x 0000 0000 0000 0010 - start of interrupts table (254 x 16 words = 8128 bytes) - see 5. INTERRUPTS TABLE
	0x 0000 0000 0000 0fef - end of interrupts table (end of int 254)
*/

void create_interrupt(regt index, memt interrupt_flags, memt device_number, memt ring_mask, regt address, regt mmt) {
    regt IBASE = index * INTERRUPT_SIZE * sizeof(memt);
    sysmem[IBASE +  0] = interrupt_flags;
    sysmem[IBASE +  1] = device_number;
    sysmem[IBASE +  2] = ring_mask;
    sysmem[IBASE +  3] = 0x00;
    sysmem[IBASE +  4] = 0xca;
    sysmem[IBASE +  5] = 0x11;
    sysmem[IBASE +  6] = 0xab;
    sysmem[IBASE +  7] = 0x1e;
    *(reinterpret_cast<regt*>(&sysmem[IBASE + 8])) = reg_to_mem64(address);
    // 64 bits unused
    *(reinterpret_cast<regt*>(&sysmem[IBASE + 24])) = reg_to_mem64(mmt);
}

void create_default_interrupts() {
    for(regt i = 1; i < 255; ++i) {
        // enabled? global? MMT change? MMT change? <3 unused> device interrupt? <8 bit device address>
        memt interrupt_flags = 0x0000;
        memt device_number = 0;
        memt ring_mask = 0;
        regt destination = NOWHERE;
        regt mmt = 0;
        if(i == 0) {
            interrupt_flags |= INT_ENABLED;
            destination = START_ADDRESS;
        }
        create_interrupt(i, interrupt_flags, device_number, ring_mask, destination, mmt);

        /*sysmem[i * 8] = 0b0000000000000000; 
        sysmem[i * 8 + 1] = 0x0000; // unused
        sysmem[i * 8 + 2] = 0x0000; // unused
        sysmem[i * 8 + 3] = 0x0000; // unused
        sysmem[i * 8 + 4] = 0x0000; // address
        sysmem[i * 8 + 5] = 0x0000; // address
        sysmem[i * 8 + 6] = 0x0000; // address
        sysmem[i * 8 + 7] = 0x0000; // address */
    }
}

// IMPORTANT: When modifying reg[csrPC] after loading an immediate, make sure it happens at the end of the opcode
// implementation, or page faults may not resume at the right address!

regt last_block = -1;
regt last_PC = -1;
memt* last_pointer = NULL;

#define INS_ADDRESS_MASK 0x0000ffffffffff00

octet* ins_p;

clock_t current_time = 0;
regt last_ins = 0;

static __forceinline void execute() {
    ins_count += 1;
    if(video_enabled) {
        if((ins_count & 0x3ffffff) == 0) {
            clock_t new_time = clock();
            char time_string[128];
            sprintf(time_string, "CHASM - %02llu MHz",
                (regt)(((double)0x3ffffff / ((double)(new_time - current_time) / CLOCKS_PER_SEC)) / 1000000)
            );
            int r = SDL_SetWindowTitle(ChasmWindow, time_string);
            last_ins = ins_count;
            /*if(r)
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't change title: %s", SDL_GetError());*/
            
            current_time = new_time;
        }
    }

    if((reg[csrPC] & INS_ADDRESS_MASK) == last_block) {
        ins_p += (reg[csrPC] - last_PC) * sizeof(memt);
    } else {
        ins_p = mem(reg[csrPC], MMT_EXECUTABLE_MASK);
    }
    last_block = reg[csrPC] & INS_ADDRESS_MASK;
    last_PC = reg[csrPC];

    // memt ins = (((memt)*ins_p) << 8) | (memt)*(ins_p + 1);
    memt ins;
    ins = mem_to_reg16(*reinterpret_cast<memt*>(ins_p));
    /*if((last_PC >> 8) == (reg[csrPC] >> 8)) {
        ins = *(++last_pointer);
    } else {
        last_pointer = mem(reg[csrPC], MMT_EXECUTABLE_MASK);
        ins = *last_pointer;
    }
    last_PC = reg[csrPC]; */
    reg[csrPC] += 1;
    /*
    memt major_nibble = (ins >> 12) & 0b1111;
    memt minor_nibble = (ins >> 8) & 0b1111;
    memt regR = ins & 0b1111;
    memt regS = (ins >> 4) & 0b1111;
    */
    //if(ins && 0)
    
    struct opt* op = &opspace[ins];
    /*octet* pt = (octet*)op; // don't try this at home
    if constexpr (std::endian::native == std::endian::little) {
        ins_traits.r0 = pt[11];
        ins_traits.r1 = pt[10];
        ins_traits.r2 = pt[13];
        ins_traits.b = pt[12];
        ins_traits.vp = pt[15] & 0x02;
        ins_traits.ap = pt[15] & 0x01;
        ins_traits.skip = (pt[14] >> 4) & 0xf;
        ins_traits.w = pt[14] & 0xf;
    } else {
        ins_traits.r0 = pt[10];
        ins_traits.r1 = pt[11];
        ins_traits.r2 = pt[12];
        ins_traits.b = pt[13];
        ins_traits.vp = pt[14] & 0x02;
        ins_traits.ap = pt[14] & 0x01;
        ins_traits.skip = (pt[15] >> 4) & 0xf;
        ins_traits.w = pt[15] & 0xf;
    }*/
    
    #ifdef VERBOSITY_MODE
    if(verbosity >= 3) {
        fprintf(stderr, "\t@%016llx = 0x%016llx %04x %02x %02x %02x %02x %02x %02x %02x\n", reg[csrPC] - 1, op->impl, op->op, op->r0, op->r1, op->r2, op->b, op->w, op->skip, op->attribs);
        /*fprintf(stderr, "\tunpacked = %02x %02x %02x %02x %02x %02x %02x %02x",
         ins_traits.r0, ins_traits.r1, ins_traits.r2, ins_traits.b,
         ins_traits.vp, ins_traits.ap, ins_traits.skip, ins_traits.w
        );*/
        /*fprintf(stderr, "\t%016llx\t%04x = 0x%02x 0x%02x :: (%c, %c, %c, 0x%02x)\n", reg[csrPC] - 1, ins,
        ins_traits.skip, ins_traits.w, ins_traits.r0 + 'a', ins_traits.r1 + 'a', ins_traits.r2 + 'a', ins_traits.b);*/
    }
    #endif
    
    (*(op->impl))(op);
    reg[csrPC] += op->skip;
    /* last_pointer += op->skip;
    last_PC += op->skip; */
}

void init_cpu() {
    reg = (regt*)calloc(REG_COUNT, sizeof(regt));

    build_opspace();

    reg[csrSTATUS] = reg[csrPC] = 0;

    (reinterpret_cast<memt*>(sysmem))[0] = reg_to_mem16(0x600f);

    ins_p = sysmem;
    
    storev64(1, START_ADDRESS); // imm
    /* dump_memory(0x0, 0x32);
    exit(0); */

    clock_t t = clock();
    
    while(!(reg[csrSTATUS] & FLAG_MASK_H)) {
        /* reg[csrPCS] = (regt)(mem(reg[csrPC]) - sysmem);
        if(reg[csrPCS] > MEM_COUNT) // device-mapped or ROM
            reg[csrPCS] = NOWHERE; */
        
        try {
            execute(); // automatically increments PC
        } catch(PageFault& e) {
            PAGE_FAULT_ADDRESS = e.offset;
            PAGE_FAULT_POINTER = e.pointer;
            PAGE_FAULT_ENTRY = e.entry;
            delete &e;
            call_interrupt(0x02, true);
        }
        
        // reg[csrSTATUS] &= ~FLAG_MASK_J;
        /*
        if(reg[csrPC] == STOP_GUARD) {
            printf(" -- hit stop guard at 0x%llx\n", (regt)STOP_GUARD);
            reg[csrSTATUS] |= FLAG_MASK_H;
        }*/
    }

    double final_time = ((double)(clock() - t) / (double)CLOCKS_PER_SEC);

    printf(" -- end of simulation after %f sec (%llu instructions, %f ops/sec)\n", final_time, ins_count, (float)ins_count / final_time);
    
    // dump_memory(0x2fe0, 0x100);
    dump_registers();
}
