#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <string>
#include <thread>
#include <chrono>
#include "headers/memory.hpp"
#include "headers/dma.hpp"
#include <cstring>


using namespace std;
using u8 = uint8_t;
using u16 = uint16_t;
using u64 = uint64_t;
using u32 = uint32_t;
using i16 = int16_t;
using i32 = int32_t;
using i8 = int8_t;
using i64 = int64_t;
#define pass (void)0

class Instruction {
public:
    u8 t(u32 op)         { return (op >> 16) & 0x1f; }
    u8 s(u32 op)         { return (op >> 21) & 0x1f; }
    u8 d(u32 op)         { return (op >> 11) & 0x1f; }
    u8 shift(u32 op)     { return (op >> 6) & 0x1f; }
    u8 subfunction(u32 op){ return op & 0x3f; }
    u16 imm(u32 op)      { return op & 0xffff; }
    i32 imm_se(u32 op)   { return (i32)(i16)(op & 0xffff); }
    u32 imm_jump(u32 op) { return op & 0x3ffffff; }
};

class CPU {
public:
    enum ExceptionCode {
        INT = 0,
        ADEL = 4,
        ADES = 5,
        SYS = 8,
        BRK = 9,
        RI = 10,
        OVF = 12
    };
    unordered_map<uint8_t, string> table1 = {
            {0x00, "SPECIAL"}, {0x01, "BcondZ"}, {0x02, "J"}, {0x03, "JAL"},
            {0x04, "BEQ"}, {0x05, "BNE"}, {0x06, "BLEZ"}, {0x07, "BGTZ"},
            {0x08, "ADDI"}, {0x09, "ADDIU"}, {0x0A, "SLTI"}, {0x0B, "SLTIU"},
            {0x0C, "ANDI"}, {0x0D, "ORI"}, {0x0E, "XORI"}, {0x0F, "LUI"},
            {0x10, "COP0"}, {0x11, "COP1"}, {0x12, "COP2"}, {0x13, "COP3"},
            {0x20, "LB"}, {0x21, "LH"}, {0x22, "LWL"}, {0x23, "LW"},
            {0x24, "LBU"}, {0x25, "LHU"}, {0x26, "LWR"}, {0x27, "N/A"},
            {0x28, "SB"}, {0x29, "SH"}, {0x2A, "SWL"}, {0x2B, "SW"},
            {0x2C, "N/A"}, {0x2D, "N/A"}, {0x2E, "SWR"}, {0x2F, "N/A"},
            {0x30, "LWC0"}, {0x31, "LWC1"}, {0x32, "LWC2"}, {0x33, "LWC3"},
            {0x34, "N/A"}, {0x35, "N/A"}, {0x36, "N/A"}, {0x37, "N/A"},
            {0x38, "SWC0"}, {0x39, "SWC1"}, {0x3A, "SWC2"}, {0x3B, "SWC3"},
            {0x3C, "N/A"}, {0x3D, "N/A"}, {0x3E, "N/A"}, {0x3F, "N/A"}
        };
    unordered_map<uint8_t, string> table2 = {
                {0x00, "SLL"}, {0x01, "N/A"}, {0x02, "SRL"}, {0x03, "SRA"},
                {0x04, "SLLV"}, {0x05, "N/A"}, {0x06, "SRLV"}, {0x07, "SRAV"},
                {0x08, "JR"}, {0x09, "JALR"}, {0x0A, "N/A"}, {0x0B, "N/A"},
                {0x0C, "SYSCALL"}, {0x0D, "BREAK"}, {0x0E, "N/A"}, {0x0F, "N/A"},
                {0x10, "MFHI"}, {0x11, "MTHI"}, {0x12, "MFLO"}, {0x13, "MTLO"},
                {0x14, "N/A"}, {0x15, "N/A"}, {0x16, "N/A"}, {0x17, "N/A"},
                {0x18, "MULT"}, {0x19, "MULTU"}, {0x1A, "DIV"}, {0x1B, "DIVU"},
                {0x1C, "N/A"}, {0x1D, "N/A"}, {0x1E, "N/A"}, {0x1F, "N/A"},
                {0x20, "ADD"}, {0x21, "ADDU"}, {0x22, "SUB"}, {0x23, "SUBU"},
                {0x24, "AND"}, {0x25, "OR"}, {0x26, "XOR"}, {0x27, "NOR"},
                {0x28, "N/A"}, {0x29, "N/A"}, {0x2A, "SLT"}, {0x2B, "SLTU"},
                {0x2C, "N/A"}, {0x2D, "N/A"}, {0x2E, "N/A"}, {0x2F, "N/A"},
                {0x30, "N/A"}, {0x31, "N/A"}, {0x32, "N/A"}, {0x33, "N/A"},
                {0x34, "N/A"}, {0x35, "N/A"}, {0x36, "N/A"}, {0x37, "N/A"},
                {0x38, "N/A"}, {0x39, "N/A"}, {0x3A, "N/A"}, {0x3B, "N/A"},
                {0x3C, "N/A"}, {0x3D, "N/A"}, {0x3E, "N/A"}, {0x3F, "N/A"}
            };
    u32 cop_map[64];
    u32 reg_map[32];
    bool delay_slot;  
    u32 PC;
    u32 branch ;
    u32 HI,LO;
    Memory mem;
    Instruction instr;
    ofstream logFile;

    CPU() {
        logFile.open("cpu_log.txt");
        std::memset(cop_map, 0, sizeof(cop_map));
        std::memset(reg_map, 0, sizeof(reg_map));
        cop_map[12] = 0x10000000;
        reg_map[0] = 0;
        delay_slot = false;
        PC = 0xBFC00000;
        branch = 0;
        HI = 0, LO = 0;

    }

    ~CPU() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    u32 regs(u8 index) { return reg_map[index]; }

    void set_regs(u8 index, u32 val) {
        reg_map[index] = val;
        reg_map[0] = 0;
    }

    void raise_exception(u8 code) {
        int mode = cop_map[12] & 0x3f;
        cop_map[12] &= ~0x3f;
        cop_map[12] |= (mode << 2) & 0x3f;
        cop_map[14] = PC;
        cop_map[13] = code << 2;
        if (delay_slot){
            cop_map[14] = PC - 4;
            cop_map[13] |= 1 << 31;
        };
        if ((cop_map[12] & (1 << 22)) == 0){
            PC = 0x80000080;
        } else {
            PC = 0xbfc00180;
        };
    }

    u32 fetch() {
        u32 opcode = mem.read_u32(PC);
        return opcode;
    }

    // Special Instructions
    void op_sll(u32 op) {
        u8 shamt = instr.shift(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(t) << shamt);
    }

    void op_srl(u32 op) {
        u8 shamt = instr.shift(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(t) >> shamt);
    }

    void op_sra(u32 op) {
        u8 shamt = instr.shift(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, ((i32)regs(t) >> shamt));
    }

    void op_sllv(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(t) << (regs(s) & 0x1F));
    }

    void op_srlv(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(t) >> (regs(s) & 0x1F));
    }

    void op_srav(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, ((i32)regs(t) >> (regs(s) & 0x1F)));
    }

    void op_jr(u32 op) {
        u8 s = instr.s(op);
        branch = regs(s);
        delay_slot = true;
    }

    void op_jalr(u32 op) {
        u8 s = instr.s(op);
        u8 d = instr.d(op);
        branch = regs(s);
        set_regs(d,PC+8);
        delay_slot = true;
    }

    void op_syscall(u32 op) {
        raise_exception(SYS);
    }

    void op_break(u32 op) {
        raise_exception(BRK);
    }

    void op_mfhi(u32 op) {
        u8 d = instr.d(op);
        set_regs(d, HI);
    }

    void op_mthi(u32 op) {
        u8 s = instr.s(op);
        HI = regs(s);
    }

    void op_mflo(u32 op) {
        u8 d = instr.d(op);
        set_regs(d, LO);
    }

    void op_mtlo(u32 op) {
        u8 s = instr.s(op);
        LO = regs(s);
    }

    void op_mult(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i64 a = (i64)(i32)regs(s);
        i64 b = (i64)(i32)regs(t);
        i64 v = a * b;
        HI = (v >> 32);
        LO = (v & 0xffffffff);
    }

    void op_multu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u64 a = (u64)regs(s);
        u64 b = (u64)regs(t);
        u64 v = a * b;
        HI = (v >> 32);
        LO = (v  & 0xffffffff);
    }

    void op_div(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i32 n = (i32)regs(s);
        i32 d = (i32)regs(t);
        if (d == 0) {
            HI = n;
            LO = (n >= 0) ? 0xffffffff : 1;
        } else if (n == 0x80000000 && d == -1) {
            HI = 0;
            LO = 0x80000000;
        } else {
            HI = (n % d);
            LO = (n / d);
        }
    }

    void op_divu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u32 n = regs(s);
        u32 d = regs(t);
        if (d == 0) {
            HI = n;
            LO = 0xffffffff;
        } else {
            HI = n % d;
            LO = n / d;
        }
    }

    void op_add(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        i32 rs_val = (i32)regs(s);
        i32 rt_val = (i32)regs(t);
        i32 res = rs_val + rt_val;
        
        if (((rs_val > 0) && (rt_val > 0) && (res <= 0)) ||
            ((rs_val < 0) && (rt_val < 0) && (res >= 0))) {
            raise_exception(OVF);
        } else {
            set_regs(d, res);
        }
    }

    void op_addu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) + regs(t));
    }

    void op_sub(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        i32 r = ((i32)regs(s) - (i32)regs(t));
        int o = __builtin_ssub_overflow(regs(s),regs(t),&r);
        if (!o) set_regs(d,r);
    }

    void op_subu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) - regs(t));
    }

    void op_and(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) & regs(t));
    }

    void op_or(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) | regs(t));
    }

    void op_xor(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) ^ regs(t));
    }

    void op_nor(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, ~(regs(s) | regs(t)));
    }

    void op_slt(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, (i32)regs(s) < (i32)regs(t));
    }

    void op_sltu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(d, regs(s) < regs(t));
    }
    void op_addi(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        set_regs(t, (u32)((i32)regs(s) + imm_se));
    }

    void op_addiu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        set_regs(t, regs(s) + (u32)imm_se);
    }

    void op_slti(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        set_regs(t, ((i32)regs(s) < (i32)imm_se) ? 1 : 0);
    }

    void op_sltiu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        set_regs(t, (regs(s) < (u32)imm_se) ? 1 : 0);
    }

    void op_andi(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u16 imm = instr.imm(op);
        set_regs(t, regs(s) & imm);
    }

    void op_ori(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u16 imm = instr.imm(op);
        set_regs(t, regs(s) | imm);
    }

    void op_xori(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u16 imm = instr.imm(op);
        set_regs(t, regs(s) ^ imm);
    }

    void op_lui(u32 op) {
        u8 t = instr.t(op);
        u16 imm = instr.imm(op);
        set_regs(t, imm << 16);
    }

    void op_lb(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        set_regs(t, (i8)mem.read(addr));
    }

    void op_lh(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        set_regs(t, (i16)mem.read_u16(addr));
    }

    void op_lwl(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i32 imm = (u32)(i32)(instr.imm_se(op));
        u32 addr = regs(s) + imm;
        u32 addr_2 = addr & 0xfffffffc;
        u32 cur_mem = mem.read_u32(addr_2);
        u32 orig = regs(t);
        u32 mem_;
        u32 x = addr & 3;
        if (x == 0) mem_ = (cur_mem << 24) | (orig & 0x00FFFFFF);
        else if (x == 1) mem_ = (cur_mem << 16) | (orig & 0x0000FFFF);
        else if (x == 2) mem_ = (cur_mem << 8)  | (orig & 0x000000FF);
        else           mem_ = cur_mem;
        set_regs(t, mem_);
    }

    void op_lw(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        if (addr % 4 == 0) {
            set_regs(t, mem.read_u32(addr));
        } else {
            raise_exception(ADEL);
        }
    }

    void op_lbu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        set_regs(t, mem.read(addr));
    }

    void op_lhu(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        set_regs(t, mem.read_u16(addr));
    }

    void op_lwr(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        u16 imm = (u32)(i32)(instr.imm_se(op));
        u32 addr = (regs(s) + imm);
        u32 addr_2 = addr& 0xfffffffc;
        u32 cur_mem = mem.read_u32(addr_2);
        u32 orig = regs(t);
        u32 mem_;
        u32 x = addr & 3;
        if (x == 0) mem_ = cur_mem;
        else if (x == 1) mem_ = (cur_mem >> 8)  | (orig & 0xFF000000);
        else if (x == 2) mem_ = (cur_mem >> 16) | (orig & 0xFFFF0000);
        else           mem_ = (cur_mem >> 24) | (orig & 0xFFFFFF00);
        set_regs(t, mem_);
    }

    void op_sb(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        if (cop_map[12] & 0x00010000) return;
        u32 addr = regs(s) + imm_se;
        mem.write(addr, (u8)regs(t));
    }

    void op_sh(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        if (cop_map[12] & 0x00010000) return;
        u32 addr = regs(s) + imm_se;
        mem.write_u16(addr, (u16)regs(t));
    }

    void op_swl(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + (u32)imm_se;
        u32 v = regs(t);
        u32 addr_2 = addr & 0xfffffffc;
        u32 cur_mem = mem.read_u32(addr_2);
        u32 mem_;
        u32 x = addr & 3;
        if (x == 0)      mem_ = (cur_mem & 0x00FFFFFF) | (v >> 24);
        else if (x == 1) mem_ = (cur_mem & 0x0000FFFF) | (v >> 16);
        else if (x == 2) mem_ = (cur_mem & 0x000000FF) | (v >> 8);
        else             mem_ = v;
        mem.write_u32(addr_2, mem_);
    }

    void op_sw(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + imm_se;
        if (cop_map[12] & 0x00010000) return;
        mem.write_u32(addr, regs(t));
    }

    void op_swr(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        u32 addr = regs(s) + (u32)imm_se;
        u32 v = regs(t);
        u32 addr_2 = addr & 0xfffffffc;
        u32 cur_mem = mem.read_u32(addr_2);
        u32 mem_;
        u32 x = addr & 3;
        if (x == 0) mem_ = v;
        else if (x == 1) mem_ = (cur_mem & 0xFF000000) | (v << 8);
        else if (x == 2) mem_ = (cur_mem & 0xFFFF0000) | (v << 16);
        else           mem_ = (cur_mem & 0xFFFFFF00) | (v << 24);
        mem.write_u32(addr_2, mem_);
    }

    void op_j(u32 op) {
        branch = (PC & 0xF0000000) | (instr.imm_jump(op) << 2);
        delay_slot = true;
    }

    void op_jal(u32 op) {
        set_regs(31, PC);
        branch = (PC & 0xF0000000) | (instr.imm_jump(op) << 2);
        delay_slot = true;
    }

    void op_beq(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        if (regs(s) == regs(t)) {
            branch = PC + ((i32)imm_se << 2);
            delay_slot = true;
        }
    }

    void op_bne(u32 op) {
        u8 s = instr.s(op);
        u8 t = instr.t(op);
        i16 imm_se = instr.imm_se(op);
        if (regs(s) != regs(t)) {
            branch = PC + ((i32)imm_se << 2);
            delay_slot = true;
        }
    }

    void op_blez(u32 op) {
        u8 s = instr.s(op);
        i16 imm_se = instr.imm_se(op);
        if ((i32)regs(s) <= 0) {
            branch = PC + ((i32)imm_se << 2);
            delay_slot = true;
        }
    }

    void op_bgtz(u32 op) {
        u8 s = instr.s(op);
        i16 imm_se = instr.imm_se(op);
        if ((i32)regs(s) > 0) {
            branch = PC + ((i32)imm_se << 2);
            delay_slot = true;
        }
    }

    void op_bcondz(u32 op) {
        u8 s = instr.s(op);
        i16 imm_se = instr.imm_se(op);
        switch ((op & 0x001f0000) >> 16) {
            case 0x00000000 >> 16: // BLTZ
                if ((i32)regs(s) < 0) {
                    branch = PC + ((i32)imm_se << 2);
                    delay_slot = true;
                }
                break;
            case 0x00010000 >> 16: // BGEZ
                if ((i32)regs(s) >= 0) {
                    branch = PC + ((i32)imm_se << 2);
                    delay_slot = true;
                }
                break;
            case 0x00100000 >> 16: // BLTZAL
                if ((i32)regs(s) < 0) {
                    set_regs(31, PC);
                    branch = PC + ((i32)imm_se << 2);
                    delay_slot = true;
                }
                break;
            case 0x00110000 >> 16: // BGEZAL
                if ((i32)regs(s) >= 0) {
                    set_regs(31, PC);
                    branch = PC + ((i32)imm_se << 2);
                    delay_slot = true;
                }
                break;
            default:
                raise_exception(RI);
        }
    }

    // COP0 Instructions
    void op_mfc0(u32 op) {
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        set_regs(t, cop_map[d]);
    }

    void op_mtc0(u32 op) {
        u8 t = instr.t(op);
        u8 d = instr.d(op);
        cop_map[d] = regs(t);
    }

    void op_rfe(u32 op) {
        u32 mode = cop_map[12] & 0x3f;
        cop_map[12] &= 0xfffffff0;
        cop_map[12] |= mode >> 2;
    }

    void exe(u32 op) {
        u8 opcode = (op >> 26) & 0x3F;
        if (opcode == 0x00) {
            u8 func = op & 0x3F;
            logFile << table2[func] << "  ";
            logFile << "PC:" << hex << PC-4 << "   ";
            logFile << "opcode: " << hex << op << " func: " << hex << (int)func << endl;

            switch (func) {
                case 0x00: op_sll(op); break;
                case 0x02: op_srl(op); break;
                case 0x03: op_sra(op); break;
                case 0x04: op_sllv(op); break;
                case 0x06: op_srlv(op); break;
                case 0x07: op_srav(op); break;
                case 0x08: op_jr(op); break;
                case 0x09: op_jalr(op); break;
                case 0x0C: op_syscall(op); break;
                case 0x0D: op_break(op); break;
                case 0x10: op_mfhi(op); break;
                case 0x11: op_mthi(op); break;
                case 0x12: op_mflo(op); break;
                case 0x13: op_mtlo(op); break;
                case 0x18: op_mult(op); break;
                case 0x19: op_multu(op); break;
                case 0x1A: op_div(op); break;
                case 0x1B: op_divu(op); break;
                case 0x20: op_add(op); break;
                case 0x21: op_addu(op); break;
                case 0x22: op_sub(op); break;
                case 0x23: op_subu(op); break;
                case 0x24: op_and(op); break;
                case 0x25: op_or(op); break;
                case 0x26: op_xor(op); break;
                case 0x27: op_nor(op); break;
                case 0x2A: op_slt(op); break;
                case 0x2B: op_sltu(op); break;
                default: raise_exception(RI);
            }
            return;
        }

        logFile << table1[opcode] << "  ";
        logFile << "PC:" << hex << PC-4 << "   ";
        logFile << "opcode: " << hex << op << endl;

        switch (opcode) {
            case 0x01: op_bcondz(op); break;
            case 0x02: op_j(op); break;
            case 0x03: op_jal(op); break;
            case 0x04: op_beq(op); break;
            case 0x05: op_bne(op); break;
            case 0x06: op_blez(op); break;
            case 0x07: op_bgtz(op); break;
            case 0x08: op_addi(op); break;
            case 0x09: op_addiu(op); break;
            case 0x0A: op_slti(op); break;
            case 0x0B: op_sltiu(op); break;
            case 0x0C: op_andi(op); break;
            case 0x0D: op_ori(op); break;
            case 0x0E: op_xori(op); break;
            case 0x0F: op_lui(op); break;
            case 0x10: {
                switch((op & 0x03e00000) >> 21) {
                    case 0x00000000 >> 21: op_mfc0(op); break;
                    case 0x00800000 >> 21: op_mtc0(op); break;
                    case 0x02000000 >> 21: op_rfe(op); break;
                    default: raise_exception(RI);
                }
                break;
            }
            case 0x20: op_lb(op); break;
            case 0x21: op_lh(op); break;
            case 0x22: op_lwl(op); break;
            case 0x23: op_lw(op); break;
            case 0x24: op_lbu(op); break;
            case 0x25: op_lhu(op); break;
            case 0x26: op_lwr(op); break;
            case 0x28: op_sb(op); break;
            case 0x29: op_sh(op); break;
            case 0x2A: op_swl(op); break;
            case 0x2B: op_sw(op); break;
            case 0x2E: op_swr(op); break;
            case 0x30: case 0x31: case 0x33: case 0x38: case 0x39: case 0x3B:
                raise_exception(RI); break;
            case 0x32: case 0x3A: // GTE
                logFile << "GTE not handled yet"; exit(0);
            default:
                raise_exception(RI);
        }
    }

    void step() {
        u32 opcode = fetch();
        PC += 4;
        exe(opcode);
        if (delay_slot) {
            u32 opcode = fetch();
            PC += 4;
            exe(opcode);
            delay_slot = false;
            PC = branch;
        }
    }

    void Run() {
        mem.load_bios("SCPH1001.BIN");
        while(true) {
            step();
        }
    }
};

int main() {
    CPU cpu;
    cpu.Run();
    return 0;
}