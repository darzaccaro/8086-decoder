#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#include <Windows.h>
#include <string>
#include <format>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#define ASSERT(condition) assert(condition);
#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))
#define ABSOLUTE_VALUE(n) (((n) > 0) ? n : -n);
#include "Types.h"
#include "Win32Helpers.h"
#include "File.h"

#define OP_MOVE_REGISTER_OR_MEMORY_TO_OR_FROM_REGISTER(byte1) (((byte1) & 0b11111100) == 0b10001000) 
#define OP_MOVE_IMMEDIATE_TO_REGISTER_OR_MEMORY(byte1)        (((byte1) & 0b11111110) == 0b11000110) 
#define OP_MOVE_IMMEDIATE_TO_REGISTER(byte1)                  (((byte1) & 0b11110000) == 0b10110000) 
#define OP_MOVE_MEMORY_TO_ACCUMULATOR(byte1)                  (((byte1) & 0b11111110) == 0b10100000) 
#define OP_MOVE_ACCUMULATOR_TO_MEMORY(byte1)                  (((byte1) & 0b11111110) == 0b10100010) 
#define OP_ADD_RM_AND_REG_TO_EITHER(byte1)                    (((byte1) & 0b11111100) == 0b00000000)
#define OP_ADD_SUB_CMP_IMMEDIATE_TO_RM(byte1)                 (((byte1) & 0b11111100) == 0b10000000)
#define OP_ADD_IMMEDIATE_TO_ACCUMULATOR(byte1)                (((byte1) & 0b11111110) == 0b00000100)
#define OP_SUB_RM_AND_REG_TO_EITHER(byte1)                    (((byte1) & 0b11111100) == 0b00101000)
#define OP_SUB_IMMEDIATE_FROM_ACCUMULATOR(byte1)              (((byte1) & 0b11111110) == 0b00101100)
#define OP_CMP_RM_TO_REG(byte1)                               (((byte1) & 0b11111100) == 0b00111000)
#define OP_CMP_IMMEDIATE_TO_ACCUMULATOR(byte1)                (((byte1) & 0b11111110) == 0b00111100)
// jump if not equal / zero
#define OP_JNZ(byte1)    ((byte1) == 0b01110101)
// jump on equal / zero
#define OP_JE(byte1)     ((byte1) == 0b01110100)
// jump jump if <
#define OP_JL(byte1)     ((byte1) == 0b01111100)
// jump if <=
#define OP_JLE(byte1)    ((byte1) == 0b01111110)
// jump if below
#define OP_JB(byte1)     ((byte1) == 0b01110010)
// jump if below or equal
#define OP_JBE(byte1)    ((byte1) == 0b01110110)
// jump on parity / parity even
#define OP_JP(byte1)     ((byte1) == 0b01111010)
// jump on overflow
#define OP_JO(byte1)     ((byte1) == 0b01110000)
// jump on sign
#define OP_JS(byte1)     ((byte1) == 0b01111000)
// jump if not <
#define OP_JNL(byte1)    ((byte1) == 0b01111101)
// jump if >
#define OP_JG(byte1)     ((byte1) == 0b01111111)
// jump not below
#define OP_JNB(byte1)    ((byte1) == 0b01110011)
// jump if above
#define OP_JA(byte1)     ((byte1) == 0b01110111)
// jump not par
#define OP_JNP(byte1)    ((byte1) == 0b01111011)
// jump not overflow
#define OP_JNO(byte1)    ((byte1) == 0b01110001)
// jump not sign
#define OP_JNS(byte1)    ((byte1) == 0b01111001)
#define OP_LOOP(byte1)   ((byte1) == 0b11100010)
// while zero/equal
#define OP_LOOPZ(byte1)  ((byte1) == 0b11100001)
#define OP_LOOPNZ(byte1) ((byte1) == 0b11100000)
// jump if cx register is zero
#define OP_JCXZ(byte1)   ((byte1) == 0b11100011)

// TODO etc...
// if no displacement and r/m = 110, then it's actually a 16 bit displacment (lol)
#define MOD_MEMORY_MODE_NO_DISPLACEMENT     0b00000000
#define MOD_MEMORY_MODE_8_BIT_DISPLACEMENT  0b01000000
#define MOD_MEMORY_MODE_16_BIT_DISPLACEMENT 0b10000000
#define MOD_REGISTER_MODE_NO_DISPLACEMENT   0b11000000
#define MOD_MASK 0b11000000
#define RM_MASK  0b00000111

void decode(cstring inputName);
std::string get_register_string(u8 byte, bool is16Bit);
template<typename T>
void append_displacement_string(std::string& str, T displacement);
u8 get_rm_with_displacement_string(std::string& str, u8* ptr, u8 byte1, u8 byte2);

int main() {
    using std::string;
    cstring inputs[] = {
        //"listing_0037_single_register_mov",
        //"listing_0038_many_register_mov",
        //"listing_0039_more_movs",
        //"listing_0040_challenge_movs",
        "listing_0041_add_sub_cmp_jnz",
        //"listing_0042_completionist_decode",
    };
    for (int i = 0; i < ARRAY_COUNT(inputs); i++) {
        decode(inputs[i]);
    }
    return 0;
}

void decode(cstring inputName) {
    auto input = new File(std::format("test-data/computer_enhance/perfaware/part1/{}", inputName).c_str());
    // TODO diff the output against the reference
    // auto reference = new File(std::format("test-data/computer_enhance/perfaware/part1/{}.asm", inputName).c_str());
    std::string output = std::format(";{} decoded\n\nbits 16\n\n", inputName);
    u8* ptr = input->data;
    while (ptr < input->data + input->size) {
        u8 byte1 = *ptr;
        ptr++;
        if (OP_MOVE_REGISTER_OR_MEMORY_TO_OR_FROM_REGISTER(byte1)) {
            std::string reg, rm, source, dest;
            u8 byte2 = *ptr;
            ptr++;
            ptr += get_rm_with_displacement_string(rm, ptr, byte1, byte2);
            //                                                    reg mask
            reg = get_register_string(byte2 >> 3, (byte1 & 0b00000001) > 0);
            // check d bit to determine which registers are source and destination
            if ((byte1 & 0b00000010) > 0) {
                source = rm;
                dest = reg;
            } else {
                source = reg;
                dest = rm;
            }
            output += std::format("mov {}, {}\n", dest, source);
        } else if (OP_MOVE_IMMEDIATE_TO_REGISTER_OR_MEMORY(byte1)) {
            std::string dest, immediate;
            u8 byte2 = *ptr;
            ptr++;
            ASSERT((byte2 & 0b00111000) == 0);
            ptr += get_rm_with_displacement_string(dest, ptr, byte1, byte2);
            
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
                i16 data = *(i16*)ptr;
                ptr += 2;
                immediate = std::format("word {}", data);
            } else {
                i8 data = *(i8*)ptr;
                ptr++;
                immediate = std::format("byte {}", data);;
            }
            output += std::format("mov {}, {}\n", dest, immediate);
        } else if (OP_MOVE_IMMEDIATE_TO_REGISTER(byte1)) {
            std::string reg, source, dest;
            //           w mask
            if ((byte1 & 0b00001000) > 0) {
                // instruction is 3 bytes (2 bytes of data)
                reg = get_register_string(byte1, true);
                dest = reg;
                i16 immediate = *((i16*)ptr);
                ptr += 2;
                source = std::format("{}", immediate);
            } else {
                // instruction is 2 bytes (1 byte of data)
                reg = get_register_string(byte1, false);
                dest = reg;
                i8 immediate = *((i8*)ptr);
                ptr++;
                source = std::format("{}", immediate);
            }
            output += std::format("mov {}, {}\n", dest, source);
        } else if (OP_MOVE_MEMORY_TO_ACCUMULATOR(byte1)) {
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
                u16 addr = *(u16*)ptr;
                ptr += 2;
                output += std::format("mov ax, [{}]\n", addr);
            } else {
                u8 addr = *ptr;
                ptr += 1;
                output += std::format("mov al, [{}]\n", addr);
            }
        } else if (OP_MOVE_ACCUMULATOR_TO_MEMORY(byte1)) {
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
                u16 addr = *(u16*)ptr;
                ptr += 2;
                output += std::format("mov [{}], ax\n", addr);
            } else {
                u8 addr = *ptr;
                ptr += 1;
                output += std::format("mov [{}], al\n", addr);
            }
        } else if (OP_ADD_RM_AND_REG_TO_EITHER(byte1) || OP_SUB_RM_AND_REG_TO_EITHER(byte1) || OP_CMP_RM_TO_REG(byte1)) {
            std::string op, reg, rm, source, dest;
            if (OP_ADD_RM_AND_REG_TO_EITHER(byte1)) {
                op = "add";
            } else if (OP_SUB_RM_AND_REG_TO_EITHER(byte1)) {
                op = "sub";
            } else {
                op = "cmp";
            }
            u8 byte2 = *ptr;
            ptr++;
            ptr += get_rm_with_displacement_string(rm, ptr, byte1, byte2);
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
                reg = get_register_string(byte2 >> 3, true);
            } else {
                reg = get_register_string(byte2 >> 3, false);
            }
            if ((byte1 & 0b00000010) > 0) {
                source = rm;
                dest = reg;
            } else {
                source = reg;
                dest = rm;
            }
            output += std::format("{} {}, {}\n", op, dest, source);
        } else if (OP_ADD_SUB_CMP_IMMEDIATE_TO_RM(byte1)) {
            std::string op, rm, immediate, type;
            u8 byte2 = *ptr;
            ptr++;
            if ((byte2 & 0b00111000) == 0b00000000) {
                op = "add";
            } else if ((byte2 & 0b00111000) == 0b00101000) {
                op = "sub";
            } else if ((byte2 & 0b00111000) == 0b00111000) {
                op = "cmp";
            }
            ptr += get_rm_with_displacement_string(rm, ptr, byte1, byte2);
            //           s mask
            if ((byte1 & 0b00000010) > 0) {
                // TODO @implementation sign extension of 8 bits into 16 bits if w = 1
                if (((byte1 & 0b00000001) > 0)) {
                    u8 low = *ptr;
                    ptr++;
                    u8 high = ((low & 0b10000000) > 0) ? 0b11111111 : 0b00000000;
                    i16 quad = 0;
                    quad |= high;
                    quad <<= 8;
                    quad |= low;
                    immediate = std::format("{}", quad);
                    type = "word";
                } else {
                    ASSERT(false);
                }
            } else {
                //            w mask
                if (((byte1 & 0b00000001) > 0)) {
                    i16 data = *((i16*)ptr);
                    ptr += 2;
                    type = "word";
                    immediate = std::format("{}", data);
                } else {
                    i8 data = *((i8*)ptr);
                    ptr++;
                    type = "byte";
                    immediate = std::format("{}", data);
                }
            }
            output += std::format("{} {} {}, {}\n", op, type, rm, immediate);
        } else if (OP_ADD_IMMEDIATE_TO_ACCUMULATOR(byte1) || OP_SUB_IMMEDIATE_FROM_ACCUMULATOR(byte1) || OP_CMP_IMMEDIATE_TO_ACCUMULATOR(byte1)) {
            std::string op;
            if (OP_ADD_IMMEDIATE_TO_ACCUMULATOR(byte1)) {
                op = "add";
            } else if (OP_SUB_IMMEDIATE_FROM_ACCUMULATOR(byte1)) {
                op = "sub";
            } else {
                op = "cmp";
            }
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
                i16 data = *(i16*)ptr;
                ptr += 2;
                output += std::format("{} ax, {}\n", op, data);
            } else {
                i8 data = *(i8*)ptr;
                ptr++;
                output += std::format("{} al, {}\n", op, data);
            }
        } else if (OP_JNZ(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jnz {}\n", byte2);
        } else if (OP_JE(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("je {}\n", byte2);
        } else if (OP_JL(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jl {}\n", byte2);
        } else if (OP_JLE(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jle {}\n", byte2);
        } else if (OP_JB(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jb {}\n", byte2);
        } else if (OP_JBE(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jbe {}\n", byte2);
        } else if (OP_JP(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jp {}\n", byte2);
        } else if (OP_JO(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jo {}\n", byte2);
        } else if (OP_JS(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("js {}\n", byte2);
        } else if (OP_JNL(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jnl {}\n", byte2);
        } else if (OP_JG(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jg {}\n", byte2);
        } else if (OP_JNB(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jnb {}\n", byte2);
        } else if (OP_JA(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("ja {}\n", byte2);
        } else if (OP_JNP(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jnp {}\n", byte2);
        } else if (OP_JNO(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jno {}\n", byte2);
        } else if (OP_JNS(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jns {}\n", byte2);
        } else if (OP_LOOP(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("loop {}\n", byte2);
        } else if (OP_LOOPZ(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("loopz {}\n", byte2);
        } else if (OP_LOOPNZ(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("loopnz {}\n", byte2);
        } else if (OP_JCXZ(byte1)) {
            i8 byte2 = *(i8*)ptr; // offset to jump to
            ptr++;
            output += std::format("jcxz {}\n", byte2);
        } else {
            OutputDebugString("encountered unimplemented opcode\n");
            ASSERT(false);
        }
    }
    File::write(std::format("test-data/output/{}-output.asm", inputName).c_str(), output);
}

std::string get_register_string(u8 byte, bool is16Bit) {
    ASSERT((byte & 0b00000111) <= 0b00000111);
    switch (byte & 0b00000111) {
    case 0b00000000: return (is16Bit) ? "ax" : "al";
    case 0b00000001: return (is16Bit) ? "cx" : "cl";
    case 0b00000010: return (is16Bit) ? "dx" : "dl";
    case 0b00000011: return (is16Bit) ? "bx" : "bl";
    case 0b00000100: return (is16Bit) ? "sp" : "ah";
    case 0b00000101: return (is16Bit) ? "bp" : "ch";
    case 0b00000110: return (is16Bit) ? "si" : "dh";
    case 0b00000111: return (is16Bit) ? "di" : "bh";
    default: ASSERT(false); return "";
    }
}

[[nodiscard]] 
u8 get_rm_with_displacement_string(std::string& str, u8* ptr, u8 byte1, u8 byte2) {
    u8 ptr_increment = 0;
    switch (byte2 & MOD_MASK) {
    case MOD_MEMORY_MODE_NO_DISPLACEMENT:
    {
        switch (byte2 & RM_MASK) {
        case 0b00000000: str = "[bx + si]"; break;
        case 0b00000001: str = "[bx + di]"; break;
        case 0b00000010: str = "[bp + si]"; break;
        case 0b00000011: str = "[bp + di]"; break;
        case 0b00000100: str = "si"; break;
        case 0b00000101: str = "di"; break;
        case 0b00000110:
        {
            i16 displacement = *((i16*)ptr);
            ptr_increment = 2;
            str = std::format("[{}]", displacement);
        } break;
        case 0b00000111: str = "bx"; break;
        }
    } break;
    case MOD_MEMORY_MODE_8_BIT_DISPLACEMENT:
    {
        i8 displacement = *((i8*)ptr);
        ptr_increment = 1;
        switch (byte2 & RM_MASK) {
        case 0b00000000: str = std::string("[bx + si"); break;
        case 0b00000001: str = std::string("[bx + di"); break;
        case 0b00000010: str = std::string("[bp + si"); break;
        case 0b00000011: str = std::string("[bp + di"); break;
        case 0b00000100: str = std::string("[si"); break;
        case 0b00000101: str = std::string("[di"); break;
        case 0b00000110: str = std::string("[bp"); break;
        case 0b00000111: str = std::string("[bx"); break;
        }
        append_displacement_string<i8>(str, displacement);
    } break;
    case MOD_MEMORY_MODE_16_BIT_DISPLACEMENT:
    {
        i16 displacement = *((i16*)ptr);
        ptr_increment = 2;
        switch (byte2 & RM_MASK) {
        case 0b00000000: str = std::string("[bx + si"); break;
        case 0b00000001: str = std::string("[bx + di"); break;
        case 0b00000010: str = std::string("[bp + si"); break;
        case 0b00000011: str = std::string("[bp + di"); break;
        case 0b00000100: str = std::string("[si"); break;
        case 0b00000101: str = std::string("[di"); break;
        case 0b00000110: str = std::string("[bp"); break;
        case 0b00000111: str = std::string("[bx"); break;
        }
        append_displacement_string<i16>(str, displacement);
    } break;
    case MOD_REGISTER_MODE_NO_DISPLACEMENT:
    {
        str = get_register_string(byte2, (byte1 & 0b00000001) > 0);
    } break;
    }
    return ptr_increment;
}

template<typename T>
void append_displacement_string(std::string& str, T displacement) { // u8 or u16
    static_assert(std::is_same<std::decay_t<decltype(displacement)>, i8>::value || std::is_same<std::decay_t<decltype(displacement)>, i16>::value, "displacement must be of type i8 or i16");
    if (displacement > 0) {
        str += std::format(" + {}]", displacement);
    } else if (displacement < 0) {
        str += std::format(" - {}]", -displacement);
    } else {
        str += ']';
    }
}