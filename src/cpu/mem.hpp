#ifndef MEM_HPP
#define MEM_HPP
    #include "cpu.h"
    #include <exception>
    struct PageFault : public std::exception {
        regt offset;
        regt pointer;
        regt entry;

        const char * what () const throw () {
            return "CHASM Page Fault";
        }
    };

#endif // MEM_HPP