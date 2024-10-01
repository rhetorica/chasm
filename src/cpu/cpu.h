#ifndef CPU_H
#define CPU_H

    #include <stddef.h>

    extern int verbosity;
    
    #define REG_COUNT 24
    
    // where the firmware is loaded and execution starts:
    #define START_ADDRESS 0x0000020000000000
    #define INTERRUPT_SIZE 0x10
    #define INTERRUPT_TABLE_START 0
    #define STOP_GUARD (START_ADDRESS + 0x0110)

    #define csrA 0x00
    #define csrB 0x01
    #define csrC 0x02
    #define csrD 0x03
    #define csrE 0x04
    #define csrF 0x05
    #define csrG 0x06
    #define csrH 0x07
    #define csrPC 0x08
    #define csrRA 0x09
    #define csrSB 0x0a
    #define csrSP 0x0b
    #define csrDB 0x0c
    #define csrCB 0x0d
    #define csrPI 0x0e
    #define csrSTATUS 0x0f
    #define csrAd 0x10
    #define csrBd 0x11
    #define csrCd 0x12
    #define csrDd 0x13
    #define csrEd 0x14
    #define csrFd 0x15
    #define csrGd 0x16
    #define csrHd 0x17

    #define FLAG_C 0x00
    #define FLAG_L 0x01
    #define FLAG_E 0x02
    #define FLAG_G 0x03
    #define FLAG_Z 0x04
    #define FLAG_S 0x06
    #define FLAG_R0 0x20
    #define FLAG_R1 0x21
    #define FLAG_R2 0x22
    #define FLAG_M 0x23
    #define FLAG_IP 0x24
    #define FLAG_H 0x26
    #define FLAG_J 0x25
    #define FLAG_OFFSET_CI 0x28
    #define FLAG_RM0 0x30
    #define FLAG_RM1 0x31
    #define FLAG_RM2 0x32
    
    #define FLAG_MASK_C 0x00000001
    #define FLAG_MASK_L 0x00000002
    #define FLAG_MASK_E 0x00000004
    #define FLAG_MASK_G 0x00000008
    #define FLAG_MASK_Z 0x00000010
    #define FLAG_MASK_S 0x00000020
    #define FLAG_MASK_6 0x00000040
    #define FLAG_MASK_7 0x00000080
    #define FLAG_MASK_RING 0x0000000700000000
    #define FLAG_MASK_R0 0x0000000100000000
    #define FLAG_MASK_R1 0x0000000200000000
    #define FLAG_MASK_R2 0x0000000400000000
    #define FLAG_MASK_M 0x0000000800000000
    #define FLAG_MASK_IP 0x0000001000000000
    #define FLAG_MASK_H 0x0000004000000000
    #define FLAG_MASK_J 0x0000002000000000
    #define FLAG_MASK_CI 0x0000ff0000000000
    #define FLAG_MASK_RM0 0x0001000000000000
    #define FLAG_MASK_RM1 0x0002000000000000
    #define FLAG_MASK_RM2 0x0004000000000000
    
    #define DATA_TYPE_UNSIGNED 0
    #define DATA_TYPE_SIGNED 1
    #define DATA_TYPE_BYTES 2
    #define DATA_TYPE_FLOAT 3
    #define DATA_TYPE_BITWISE 4
    #define DATA_TYPE_CONS 5
    #define DATA_TYPE_STRING 6
    
    #define INT_ENABLED 0x8000
    #define INT_MMT_ENABLE 0x2000
    #define INT_PENDING 0x1000
    #define INT_DEVICE 0x0100

    extern regt* reg;
    extern octet* sysmem;
    extern octet* rom_image;
    extern size_t rom_size;

    octet* flat(regt);
    octet* mem(regt, memt);
    void call_interrupt(memt interrupt_number, int from_hardware);
    void interrupt_return();
    void unimplemented(memt);
    void dump_registers();
    void create_default_interrupts();

    #define MMT_ALLOCATED_MASK 0x80
    #define MMT_EXECUTABLE_MASK 0x40
    #define MMT_READABLE_MASK 0x20
    #define MMT_WRITABLE_MASK 0x10
    #define MMT_SHARED_WITH_ME_MASK 0x08
    #define MMT_SHARED_WITH_OTHER_MASK 0x04
    #define MMT_DIRTY_MASK 0x02
    #define MMT_VALID_MASK 0x01

#endif // CPU_H
