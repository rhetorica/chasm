#ifndef TYPES_H
#define TYPES_H

    #include <bit>
    #include <stdint.h>
    
    #define octet uint_least8_t
    #define memt uint_fast16_t
    #define dwordt uint_fast32_t
    #define regt uint_fast64_t
    #define sregt int_fast64_t
    #define fregt double
    #define NOWHERE (regt)(0xffffffffffffffff)

    #define reg_to_mem16(v) (std::endian::native == std::endian::little ? __builtin_bswap16(v) : (v) )
    #define reg_to_mem32(v) (std::endian::native == std::endian::little ? __builtin_bswap32(v) : (v) )
    #define reg_to_mem64(v) (std::endian::native == std::endian::little ? __builtin_bswap64(v) : (v) )
    #define mem_to_reg16(v) (std::endian::native == std::endian::little ? __builtin_bswap16(v) : (v) )
    #define mem_to_reg32(v) (std::endian::native == std::endian::little ? __builtin_bswap32(v) : (v) )
    #define mem_to_reg64(v) (std::endian::native == std::endian::little ? __builtin_bswap64(v) : (v) )

#endif // TYPES_H