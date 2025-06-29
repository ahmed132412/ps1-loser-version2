#ifndef DMA_HPP
#define DMA_HPP

#include <cstdint>
#include "dma_channels.hpp"
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;

class DMA {
public:
    // DMA Registers (based on PS1 memory map)
    /*
        1F80108x - DMA0: MDECin
        1F80109x - DMA1: MDECout
        1F8010Ax - DMA2: GPU (lists + image data)
        1F8010Bx - DMA3: CDROM
        1F8010Cx - DMA4: SPU
        1F8010Dx - DMA5: PIO (Expansion Port)
        1F8010Ex - DMA6: OTC (reverse clear OT)
        1F8010F0 - DPCR: DMA Control Register
        1F8010F4 - DICR: DMA Interrupt Register
    */
    u32 control = 0;
    DMAChannel channels[7];
    void set_control(u32 val);
    void run_channel(u32 channel);
};

#endif // DMA_HPP
