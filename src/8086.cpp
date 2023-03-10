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
#include "Types.h"
#include "Win32Helpers.h"
#include "File.h"

#define OP_MOVE_REGISTER_OR_MEMORY_TO_OR_FROM_REGISTER(byte1) (((byte1) & 0b11111100) == 0b10001000) 
#define OP_MOVE_IMMEDIATE_TO_REGISTER_OR_MEMORY(byte1)        (((byte1) & 0b11111110) == 0b11000110) 
#define OP_MOVE_IMMEDIATE_TO_REGISTER(byte1)                  (((byte1) & 0b11110000) == 0b10110000) 

// if no displacement and r/m = 110, then it's actually a 16 bit displacment (lol)
#define MOD_MEMORY_MODE_NO_DISPLACEMENT     0b00000000
#define MOD_MEMORY_MODE_8_BIT_DISPLACEMENT  0b01000000
#define MOD_MEMORY_MODE_16_BIT_DISPLACEMENT 0b10000000
#define MOD_REGISTER_MODE_NO_DISPLACEMENT   0b11000000
#define MOD_MASK 0b11000000

void decode(cstring inputName);
std::string get_register_string(u8 byte, bool is16Bit);

int main() {
    using std::string;
    cstring inputs[] = {
        //"listing_0037_single_register_mov",
        //"listing_0038_many_register_mov",
        "listing_0039_more_movs", // TODO immediate to register
        //"listing_0040_challenge_movs",
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
            switch (byte2 & MOD_MASK) {
            case MOD_MEMORY_MODE_NO_DISPLACEMENT:
            {
                // r/m mask
                switch (byte2 & 0b00000111) {
                case 0b00000000: rm = "[bx + si]"; break;
                case 0b00000001: rm = "[bx + di]"; break;
                case 0b00000010: rm = "[bp + si]"; break;
                case 0b00000011: rm = "[bp + di]"; break;
                case 0b00000100: rm = "si"; break;
                case 0b00000101: rm = "di"; break;
                case 0b00000110:
                {
                    u16 displacement = *((u16*)ptr);
                    ptr += 2;
                    rm = std::format("[{}]", displacement);
                } break;
                case 0b00000111: rm = "bx"; break;
                }
            } break;
            case MOD_MEMORY_MODE_8_BIT_DISPLACEMENT:
            {
                u8 displacement = *ptr;
                ptr++;
                // r/m mask
                switch (byte2 & 0b00000111) {
                case 0b00000000: rm = std::string("[bx + si"); break;
                case 0b00000001: rm = std::string("[bx + di"); break;
                case 0b00000010: rm = std::string("[bp + si"); break;
                case 0b00000011: rm = std::string("[bp + di"); break;
                case 0b00000100: rm = std::string("[si"); break;
                case 0b00000101: rm = std::string("[di"); break;
                case 0b00000110: rm = std::string("[bp"); break;
                case 0b00000111: rm = std::string("[bx"); break;
                }
                if (displacement != 0) {
                    rm += std::format(" + {}]", displacement);
                } else {
                    rm += ']';
                }
            } break;
            case MOD_MEMORY_MODE_16_BIT_DISPLACEMENT:
            {
                u16 displacement = *((u16*)ptr);
                ptr += 2;
                // r/m mask
                switch (byte2 & 0b00000111) {
                case 0b00000000: rm = std::string("[bx + si"); break;
                case 0b00000001: rm = std::string("[bx + di"); break;
                case 0b00000010: rm = std::string("[bp + si"); break;
                case 0b00000011: rm = std::string("[bp + di"); break;
                case 0b00000100: rm = std::string("[si"); break;
                case 0b00000101: rm = std::string("[di"); break;
                case 0b00000110: rm = std::string("[bp"); break;
                case 0b00000111: rm = std::string("[bx"); break;
                }
                if (displacement != 0) {
                    rm += std::format(" + {}]", displacement);
                } else {
                    rm += ']';
                }
            } break;
            case MOD_REGISTER_MODE_NO_DISPLACEMENT:
            {
                // r/m mask
                rm = get_register_string(byte2, (byte1 & 0b00000001) > 0);
            } break;
            }
            // reg mask
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
#if 0
        } else if (OP_MOVE_IMMEDIATE_TO_REGISTER_OR_MEMORY(byte1)) {
            //           w mask
            if ((byte1 & 0b00000001) > 0) {
            } else {
                    
            }
#endif
        } else if (OP_MOVE_IMMEDIATE_TO_REGISTER(byte1)) {
            std::string reg, rm, source, dest;
            //OutputDebugString("unimplemented OPCODE_MASK_IMMEDIATE_TO_REGISTER\n");
            //ASSERT(false);
            //           w mask
            if ((byte1 & 0b00001000) > 0) {
                // instruction is 3 bytes (2 bytes of data)
                reg = get_register_string(byte1, true);
                dest = reg;
                i16 immediate = *((u16*)ptr);
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