#include "headers/dma_channels.hpp"

u32 DMAChannel::control() const {
    u32 r = 0;

    r |= ((u32)(direction) << 0);
    r |= ((u32)(step) << 1);
    r |= ((u32)(chop) << 8);
    r |= ((u32)(sync) << 9);
    r |= ((u32)(chop_dma_sz) << 16);
    r |= ((u32)(chop_cpu_sz) << 20);
    r |= ((u32)(enable) << 24);
    r |= ((u32)(trigger) << 28);
    r |= ((u32)(dummy) << 29);

    return r;
}

void DMAChannel::set_control(u32 val) {
    direction = (val & 1) ? Direction::FromRam : Direction::ToRam;
    step = ((val >> 1) & 1) ? Step::Decrement : Step::Increment;
    chop = ((val >> 8) & 1) != 0;

    u32 sync_val = (val >> 9) & 3;
    switch (sync_val) {
        case 0: sync = Sync::Manual; break;
        case 1: sync = Sync::Request; break;
        case 2: sync = Sync::LinkedList; break;
        default: throw std::runtime_error("Unknown DMA sync mode");
    }

    chop_dma_sz = (val >> 16) & 7;
    chop_cpu_sz = (val >> 20) & 7;
    enable = ((val >> 24) & 1) != 0;
    trigger = ((val >> 28) & 1) != 0;
    dummy = (val >> 29) & 3;  
}
