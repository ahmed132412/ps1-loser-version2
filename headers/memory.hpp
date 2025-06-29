#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>

using u8 = uint8_t;
using u32 = uint32_t;
using u16 = uint16_t;

class Memory {
public:
    Memory();
    ~Memory();

    u8 read(u32 address);
    void write(u32 address, u8 value);

    u32 read_u32(u32 address);
    u16 read_u16(u32 address);
    void write_u16(u32 address,u16 value);
    void write_u32(u32 address, u32 value,u32 p=1);

    void load_bios(const char* filename);

private:
    u8* Main_Ram;
    u8* ROM_Ram;
    u8* scrat;
    u8* io;
    u8* Expansion_2;
    u8* Expansion_3;
    u8* bios;
    u8* Cache_control;
};

#endif
