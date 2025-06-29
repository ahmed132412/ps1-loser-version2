#include "headers/memory.hpp"
#include "headers/dma.hpp"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>

DMA dma;

/*
  KUSEG     KSEG0     KSEG1
  00000000h 80000000h A0000000h  2048K  Main RAM (first 64K reserved for BIOS)
  1F000000h 9F000000h BF000000h  8192K  Expansion Region 1 (ROM/RAM)
  1F800000h 9F800000h    --      1K     Scratchpad (D-Cache used as Fast RAM)
  1F801000h 9F801000h BF801000h  4K     I/O Ports
  1F802000h 9F802000h BF802000h  8K     Expansion Region 2 (I/O Ports)
  1FA00000h 9FA00000h BFA00000h  2048K  Expansion Region 3 (SRAM BIOS region for DTL cards)
  1FC00000h 9FC00000h BFC00000h  512K   BIOS ROM (Kernel) (4096K max)
        FFFE0000h (in KSEG2)     0.5K   Internal CPU control registers (Cache Control)
*/
Memory::Memory() {
    Main_Ram = new u8[2 * 1024 * 1024];        // 2MB
    ROM_Ram = new u8[8 * 1024 * 1024];         // 8MB
    scrat = new u8[1024];                       // 1KB
    io = new u8[4 * 1024];                      // 4KB
    Expansion_2 = new u8[8 * 1024];             // 8KB
    Expansion_3 = new u8[2 * 1024 * 1024];      // 2MB
    bios = new u8[512 * 1024];                  // 512KB
    Cache_control = new u8[512];         // 512 byte 0.5kb
    memset(Main_Ram, 0, 2 * 1024 * 1024);
    memset(ROM_Ram, 0, 8 * 1024 * 1024);
    memset(scrat, 0, 1024);
    memset(io, 0, 4 * 1024);
    memset(Expansion_2, 0, 8 * 1024);
    memset(Expansion_3, 0, 2 * 1024 * 1024);
    memset(bios, 0, 512 * 1024);
    memset(Cache_control, 0, 512);
}

Memory::~Memory() {
    delete[] Main_Ram;
    delete[] ROM_Ram;
    delete[] scrat;
    delete[] io;
    delete[] Expansion_2;
    delete[] Expansion_3;
    delete[] bios;
    delete[] Cache_control;
}

u8 Memory::read(u32 address) {
    //printf("address %x \n",address);
    if (address >= 0xfffe0000 && address < 0xfffe0000 + 512){
        return Cache_control[address];
    }else{
        address &= 0x1FFFFFFF; // حتى تظل جوه النواة الاولى 
        //std::cout << "address :" << std::hex << address << std::endl;
        if (address <= 0x00200000) {
            return Main_Ram[address];
        } else if (address >= 0x1F000000 && address < 0x1F800000) {
            return ROM_Ram[address - 0x1F000000];
        } else if (address >= 0x1F800000 && address < 0x1F801000) {
            return scrat[address - 0x1F800000];
        } else if (address >= 0x1F801000 && address < 0x1F802000) {
            return io[address - 0x1F801000];
        } else if (address >= 0x1F802000 && address < 0x1F804000) {
            return Expansion_2[address - 0x1F802000];
        } else if (address >= 0x1FA00000 && address < 0x1FC00000) {
            return Expansion_3[address - 0x1FA00000];
        } else if (address >= 0x1FC00000 && address < 0x1FC80000) {
            return bios[address - 0x1FC00000];
        } else {
            printf("error address not mapped\n %x",address);
            exit(0);
        }
    }
}

void Memory::write(u32 address, u8 value) {
    if (address >= 0xfffe0000 && address < 0xfffe0000 + 512){
        Cache_control[address - 0xfffe0000] = value;
    }
    else{
        address &= 0x1FFFFFFF; // حتى تظل جوه النواة الاولى 
        //std::cout << "address :" << std::hex << address << std::endl;
        if (address < 0x00200000) {
            Main_Ram[address] = value;
        } else if (address >= 0x1F000000 && address < 0x1F800000) {
            ROM_Ram[address - 0x1F000000] = value;
        } else if (address >= 0x1F800000 && address < 0x1F801000) {
            scrat[address - 0x1F800000] = value;
        } else if (address >= 0x1F801000 && address < 0x1F802000) {
            io[address - 0x1F801000] = value;
        } else if (address >= 0x1F802000 && address < 0x1F804000) {
            Expansion_2[address - 0x1F802000] = value;
        } else if (address >= 0x1FA00000 && address < 0x1FC00000) {
            Expansion_3[address - 0x1FA00000] = value;
        } else if (address >= 0x1FC00000 && address < 0x1FC80000) {
            bios[address - 0x1FC00000] = value;
        } else {
            printf("error address not mapped %x \n",address);
            exit(0);
        }
    }
}

u32 Memory::read_u32(u32 address) {
    if (address == 0x1F8010F0){  
        return dma.control;
    }
    if (address >= 0x1F801080 && address <= 0x1F8010EF) {
        u32 offset = (address - 0x1F801080);
        int channel_id = offset / 0x10;
        int reg = offset % 0x10;
        printf("hello \n");

        switch (reg) {
            case 0x0: return dma.channels[channel_id].base;
            case 0x4: return dma.channels[channel_id].block_control;
            case 0x8: return dma.channels[channel_id].control();
            default:
                printf("Unhandled DMA channel register read: %08x\n", address);
                exit(1);
        }
    }
    u8 byte1 = read(address);
    u8 byte2 = read(address + 1);
    u8 byte3 = read(address + 2);
    u8 byte4 = read(address + 3);
    u32 value = (byte4 << 24) | (byte3 << 16) | (byte2 << 8) | byte1;
    return value;
};


u16 Memory::read_u16(u32 address) {
    u8 byte1 = read(address);
    u8 byte2 = read(address + 1);
    u16 value = (byte2 << 8) | byte1;
    return value;
}

void Memory::write_u16(u32 address,u16 value) {
    write(address + 1, value >> 8);
    write(address, value & 0xff);
}
void Memory::write_u32(u32 address, u32 value,u32 pc) {
    //printf("write %x %x \n",address,value);
    if (address == 0x1F80'10F0){
        dma.set_control(value);
        return;
    }
    if (address >= 0x1F801080 && address <= 0x1F8010EF) {
        u32 offset = (address - 0x1F801080);
        int channel_id = offset / 0x10;
        int reg = offset % 0x10;
        switch (reg) {
            case 0x0:
                dma.channels[channel_id].base = value;
                return;
            case 0x4:
                dma.channels[channel_id].block_control = value;
                return;
            case 0x8:
                dma.channels[channel_id].set_control(value);
                if (dma.channels[channel_id].enable && dma.channels[channel_id].trigger){
                    dma.run_channel(channel_id);
                };
                return;
            default:
                printf("Unhandled: %08x \n", address);
                exit(1);
        }
    }

    write(address + 3, value >> 24);
    write(address + 2, (value >> 16) & 0xFF);
    write(address + 1, (value >> 8) & 0xFF);
    write(address + 0, value & 0xFF);
    
}

void Memory::load_bios(const char* filename) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("wrong!");
        exit(1);
    }
    size_t read_bytes = fread(bios, 1, 512 * 1024, f);
    if (read_bytes != 512 * 1024) {
        fprintf(stderr, "BIOS file wrong\n");
        exit(1);
    }
    fclose(f);
}
