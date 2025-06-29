#include <iostream>
#include <cstdint>
#include "headers/dma.hpp"
#include "headers/memory.hpp"
Memory mem;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

void DMA::set_control(u32 val){
    DMA::control = val;
};
void DMA::run_channel(u32 channel){
    if (channel == 6){
        u32 base = DMA::channels[channel].base;
        u32 block_control = DMA::channels[channel].block_control;
        u32 addr = base & 0x001f'fffc;
        u32 words = block_control & 0xffff;
        for (int i = 0; i < words; i++){
            if (i == words - 1) {
                mem.write_u32(addr, 0xFFFFFF); 
            } else {
                u32 the_value = (addr - 4) & 0x001FFFFC;
                mem.write_u32(addr, the_value);
            }
            addr -= 4;
        }
    }else if (channel == 2){
        if (((control >> 9) & 0x3) != 0b10) {
            printf("DMA channel Sync not in linked-list mode.\n");
            exit(0);
        }

        u32 addr = DMA::channels[channel].base & 0x001FFFFC;
        while (true) {
            u32 header = mem.read_u32(addr);
            u32 cot = (header >> 24) & 0xFF;
            u32 naddr = header & 0x001FFFFC;
            for (u32 i = 0; i < cot; i++) {
                u32 command = mem.read_u32(addr + 4 + i * 4);
                // TODO send to GPU
                printf("GPU 0x%08X\n", command);
            }

            if (naddr == 0xFFFFFF) break;
            addr = naddr;
        }
    }else{
        printf("not handle yet \n");
        exit(0);
    }
};