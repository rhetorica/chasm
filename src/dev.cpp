
/*
    0x 0000 0000 0000 1000 - start of DMT (256 x 16 words = 8192 bytes) - see 8. DEVICES TABLE
	0x 0000 0000 0000 1fff - end of devices table
*/
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <malloc.h>

#include "types.h"
#include "cpu/cpu.h"
#include "cpu/mem.hpp"
#include "cpu/mem.h"
#include "dev.h"
#include "main.h"
#include "video.h"

octet** mem_bank;
regt* mem_size;

const char* rom_path = NULL;
octet* rom_image;
size_t rom_size;

void create_device(
        memt number,
        memt flags, memt mfgr_id, memt driver_id, memt rev, memt event,
        memt power, memt temp,
        regt dma_size,
        int auto_enable_DCI) {
    regt DMBASE = (number * DEVICE_MAP_ENTRY_SIZE + DEVICE_MAP_START) * sizeof(memt);
    sysmem[DMBASE] = flags >> 8;
    sysmem[DMBASE + 1] = flags & 0xff;
    sysmem[DMBASE + 2] = mfgr_id >> 8;
    sysmem[DMBASE + 3] = mfgr_id & 0xff;
    sysmem[DMBASE + 4] = driver_id >> 8;
    sysmem[DMBASE + 5] = driver_id & 0xff;
    sysmem[DMBASE + 6] = rev;
    sysmem[DMBASE + 7] = event;
    sysmem[DMBASE + 12] = power >> 8;
    sysmem[DMBASE + 13] = power & 0xff;
    sysmem[DMBASE + 14] = temp >> 8;
    sysmem[DMBASE + 15] = temp & 0xff;
    *(reinterpret_cast<regt*>(&sysmem[DMBASE + 16])) = reg_to_mem64(dma_size);
    sysmem[DMBASE + 24] = 0;
    sysmem[DMBASE + 25] = 0;
    sysmem[DMBASE + 26] = 0;
    sysmem[DMBASE + 27] = 0;
    sysmem[DMBASE + 28] = 0;
    sysmem[DMBASE + 29] = 0;
    sysmem[DMBASE + 30] = 0;
    sysmem[DMBASE + 31] = 0;
    mem_size[number] = dma_size;

    if(auto_enable_DCI) {
        regt ISBASE = (INTERRUPT_TABLE_START + INTERRUPT_SIZE * (event + 1)) * sizeof(memt);
        sysmem[ISBASE] = (INT_DEVICE | INT_ENABLED) >> 8;
        sysmem[ISBASE + 1] = number;
        sysmem[ISBASE + 2] = 0xff; // enable mask in all rings
        sysmem[ISBASE + 3] = 0x00;
    }

    //if(verbosity & 2) {
    if(1) {
        fprintf(stderr, " -- created device #%u\n", number);
        if(flags & DF_IN_MEMORY) {
            regt dma_start = (regt)number << 40;
            fprintf(stderr, "    mapped to %llx - %llx\n", dma_start, (dma_start + dma_size - 1));
        }
    }
}

void call_device(memt number, regt* reg) {
    if(verbosity)
        printf(" -- call to device %u\n", number);
    
    regt DMBASE = (number * DEVICE_MAP_ENTRY_SIZE + DEVICE_MAP_START) * 2;
    memt device_type = (sysmem[DMBASE + 4] << 8) + (sysmem[DMBASE + 5]);
    switch(device_type) {
        case D_CLOCK:
            if(reg[csrH]) {
                if(verbosity)
                    printf(" -- ignored attempt to set system clock\n");
            } else {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                reg[csrH] = (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
            }
        break;
        case D_RNG: {
            reg[csrH] = ((regt)rand() << 0)
                      ^ ((regt)rand() << 15)
                      ^ ((regt)rand() << 30)
                      ^ ((regt)rand() << 45)
                      ^ ((regt)rand() << 60);
        } break;
        case D_VIDEO_FB: {
            video_call(number, reg);
        } break;
    }
}

void create_device_table() {
    srand(time(NULL));
    mem_bank = (octet**)calloc((DEVICE_MAX + 1), sizeof(octet*));
    mem_size = (regt*)calloc((DEVICE_MAX + 1), sizeof(regt));

    int dnum = 0;

    mem_bank[dnum] = sysmem;
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_READ_DMA | DF_CAN_WRITE_DMA | DF_IN_MEMORY,
        MFGR_OPAQUE, D_RAM, 0x00, 0x00,
        D_TEMP_DEFAULT, 0x0000,
        MEM_COUNT, false
    );

    // 1. CPU0:
    mem_bank[dnum] = (octet*)reg;
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_WRITE_ARBITRARY | DF_CAN_READ_ARBITRARY |
        DF_CAN_TRIGGER | DF_CAN_WRITE_REGISTERS | DF_CAN_READ_REGISTERS | DF_CAN_READ_DMA | DF_CAN_WRITE_DMA,
        MFGR_PHAROS, D_CPU, 0x01, 0xc0,
        D_TEMP_DEFAULT, 0x000a,
        (sizeof(regt) * REG_COUNT) / sizeof(memt), true
    );
    
    // 2. ROM: - start address is dnum << 40
    if(START_ADDRESS != (regt)dnum << 40) {
        fprintf(stderr, " -- assertion failed: start address (%llx) is not in ROM (%llx)\n", START_ADDRESS, (regt)dnum << 40);
        exit(5);
    }
    mem_bank[dnum] = rom_image;
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_IN_MEMORY,
        MFGR_OPAQUE, D_ROM, 0x01, 0x10,
        D_TEMP_DEFAULT, 0x000a,
        rom_size, true
    );

    // 3. Lithrai Real-Time Clock:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_WORKING | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_OPAQUE, D_CLOCK, 0x01, 0xfc,
        D_TEMP_DEFAULT, 0x0005,
        0, true
    );

    // 4. Ellinill HWRNG:
    create_device(dnum++,
        DF_PRESENT | DF_READY |DF_WORKING | DF_CAN_READ_SERIAL,
        MFGR_NANOCOM, D_RNG, 0x01, 0xac,
        D_TEMP_DEFAULT, 0x01,
        0, true
    );

    // 5. Vaul power control:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_TRIGGER | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_OPAQUE, D_POWER, 0x01, 0xae,
        D_TEMP_DEFAULT, 0x0001,
        0, true
    );

    // 6. Atharti video display (active):
    if(video_enabled) {
        mem_bank[dnum] = (octet*)calloc((VIDEO_MEM_SIZE), sizeof(octet));
        
        mem_bank[dnum][0] = 0x3c;
        mem_bank[dnum][1] = 0x00;
        *(reinterpret_cast<memt*>(&mem_bank[dnum][2])) = reg_to_mem16(SCREEN_WIDTH);
        *(reinterpret_cast<memt*>(&mem_bank[dnum][4])) = reg_to_mem16(SCREEN_HEIGHT);
        mem_bank[dnum][6] = 0x00;
        mem_bank[dnum][7] = 0x05;
        *(reinterpret_cast<regt*>(&mem_bank[dnum][8])) = reg_to_mem64(VIDEO_MEM_DEFAULT_FRAME_START);

        VIDEO_MEM_OFFSET = (regt)dnum << 40;

        create_device(dnum++,
            DF_PRESENT | DF_READY | DF_IN_MEMORY | DF_CAN_TRIGGER | DF_CAN_READ_DMA | DF_CAN_READ_SERIAL,
            MFGR_PHAROS, D_VIDEO_FB, 0x01, 0xd0,
            D_TEMP_DEFAULT, 0x0000,
            (VIDEO_MEM_SIZE) / 2, true
        );
    }

    // 7. Kharaidon keyboard:
    create_device(dnum++, 
        DF_PRESENT | DF_READY | DF_UNPLUG_SAFE | DF_CAN_TRIGGER | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_PHAROS, D_KEYBOARD, 0x01, 0xd8,
        D_TEMP_DEFAULT, 0x0005,
        0, true
    );

    // 8. Hoeth drive controller:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_TRIGGER | DF_FILESYSTEM | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_PHAROS, D_DRIVE, 0x01, 0xf0,
        D_TEMP_DEFAULT, 0x01f4,
        0, true
    );

    // 9. Chroesh beeper:
    create_device(dnum++,
        DF_PRESENT | DF_READY,
        MFGR_OPAQUE, D_BEEP, 0x01, 0xfd,
        D_TEMP_DEFAULT, 0x0001,
        0, true
    );

    // 10. Loec mouse:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_UNPLUG_SAFE | DF_CAN_TRIGGER | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_PHAROS, D_POINTER, 0x01, 0xe0,
        D_TEMP_DEFAULT, 0x0002,
        0, true
    );

    // 11. Drakira:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_TRIGGER | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL | DF_CAN_READ_DMA | DF_CAN_WRITE_DMA,
        MFGR_NANOCOM, D_DRAKIRA, 0x20, 0xec,
        D_TEMP_DEFAULT, 0x100,
        0, true // TODO: raise mem size to 400 * 512 when implemented
    );

    // 12. Lileath network:
    create_device(dnum++,
        DF_PRESENT | DF_READY | DF_CAN_TRIGGER | DF_CAN_READ_SERIAL | DF_CAN_WRITE_SERIAL,
        MFGR_PHAROS, D_ETHERNET, 0x01, 0xe2,
        D_TEMP_DEFAULT, 0x01,
        0, true
    );

    // TODO: CHARM

    printf(" -- initialized %i devices\n", dnum);
}