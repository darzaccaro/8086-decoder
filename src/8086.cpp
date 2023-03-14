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
            } else if ((byte2 & 0b00111000) == 0b00010100) {
                op = "sub";
            } else if ((byte2 & 0b00111000) == 0b00011100) {
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
                output += std::format("{} ax, {}", op, data);
            } else {
                i8 data = *(i8*)ptr;
                ptr++;
                output += std::format("{} al, {}", op, data);
            }
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