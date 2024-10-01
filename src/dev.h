#ifndef DEV_H
#define DEV_H
    #define DEVICE_MAP_START 0x1000
    #define DEVICE_MAP_ENTRY_SIZE 0x0010
    #define DEVICE_MAX 0xff

    #define DF_PRESENT 0x8000
    #define DF_READY 0x4000
    #define DF_UNPLUG_SAFE 0x2000
    #define DF_WORKING 0x1000
    #define DF_IN_MEMORY 0x0800
    // #define unused 0x0400
    #define DF_CAN_WRITE_ARBITRARY 0x0200
    #define DF_CAN_READ_ARBITRARY 0x0100
    #define DF_CAN_TRIGGER 0x0080
    #define DF_FILESYSTEM 0x0040
    #define DF_CAN_WRITE_REGISTERS 0x0020
    #define DF_CAN_READ_REGISTERS 0x0010
    #define DF_CAN_READ_DMA 0x0008
    #define DF_CAN_WRITE_DMA 0x0004
    #define DF_CAN_READ_SERIAL 0x0002
    #define DF_CAN_WRITE_SERIAL 0x0001

    #define MFGR_HOMEBREW 0x0000
    #define MFGR_PHAROS 0x0001
    #define MFGR_OPAQUE 0x0002
    #define MFGR_NANITE_SYSTEMS 0x0003
    #define MFGR_YUTANI 0x0004
    #define MFGR_CHIYODA 0x0005
    #define MFGR_NANOCOM 0x0006
    #define MFGR_BRIDGED 0xfffe
    #define MFGR_GENERIC 0xffff

    #define D_CPU 0x0001
    #define D_RAM 0x0002
    #define D_CLOCK 0x0005
    #define D_RNG 0x0010
    #define D_ROM 0x0102
    #define D_POWER 0x0103
    #define D_CHARM 0x0104
    #define D_TELETYPE 0x0110
    #define D_PRINTER_TEXT 0x0111
    #define D_PRINTER_PS 0x0112
    #define D_KEYBOARD 0x011a
    #define D_POINTER 0x0120
    #define D_PEN 0x0121
    #define D_TABLET 0x0122
    #define D_DRIVE 0x0200
    #define D_DRAKIRA 0x075a
    #define D_VIDEO_FB 0x1000
    #define D_CAMERA 0x2000
    #define D_BEEP 0x5001
    #define D_AUDIO_OUT 0x5005
    #define D_AUDIO_IN 0x5006
    #define D_MODEM 0x9000
    #define D_ETHERNET 0x9001

    #define D_TEMP_DEFAULT 0x0ba5

    void call_device(memt, regt*);
    void create_device_table();
    extern octet** mem_bank;
    extern regt* mem_size;

    extern const char* rom_path;
    extern octet* rom_image;
    extern size_t rom_size; // in words

#endif // DEV_H