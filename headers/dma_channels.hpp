#pragma once
#include <cstdint>
#include <stdexcept>
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
enum class Direction {
    ToRam = 0,
    FromRam = 1
};

enum class Step {
    Increment = 0,
    Decrement = 1
};

enum class Sync {
    Manual = 0 ,
    Request = 1,
    LinkedList = 2
};

class DMAChannel {
public:
    u32 base = 0;
    u32 block_control = 0;
    bool enable = false;
    Direction direction = Direction::ToRam;
    Step step = Step::Increment;
    Sync sync = Sync::Manual;
    bool trigger = false;
    bool chop = false;
    u8 chop_dma_sz = 0;
    u8 chop_cpu_sz = 0;
    u8 dummy = 0;

    DMAChannel() = default;

    u32 control() const;
    void set_control(u32 val);
};
