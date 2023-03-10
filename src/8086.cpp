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
    auto reference = new File(std::format("test-data/computer_enhance/perfaware/part1/{}.asm", inputName).c_str());
    std::string output = std::format(";{} decoded\n\nbits 16\n\n", inputName);
    u8* ptr = input->data;
    while (ptr < input->data + input->size) {
        u8 byte1 = *ptr;
        ptr++;
        if (OP_MOVE_REGISTER_OR_MEMORY_TO_OR_FROM_REGISTER(byte1)) {
            u8 byte2 = *ptr;
            ptr++;
            std::string reg, rm, source, dest;
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
                    u8 byte3 = *ptr;
                    ptr++;
                    u8 byte4 = *ptr;
                    ptr++;
                    rm = std::format("[{}]", byte3 + byte4);
                } break;
                case 0b00000111: rm = "bx"; break;
                }
            } break;
            case MOD_MEMORY_MODE_8_BIT_DISPLACEMENT:
            {
                u8 byte3 = *ptr;
                ptr++;
                // r/m mask
                switch (byte2 & 0b00000111) {
                case 0b00000000: rm = std::format("[bx + si + {}]", byte3); break;
                case 0b00000001: rm = std::format("[bx + di + {}]", byte3); break;
                case 0b00000010: rm = std::format("[bp + si + {}]", byte3); break;
                case 0b00000011: rm = std::format("[bp + di + {}]", byte3); break;
                case 0b00000100: rm = std::format("[si + {}]", byte3); break;
                case 0b00000101: rm = std::format("[di + {}]", byte3); break;
                case 0b00000110: rm = std::format("[bp + {}]", byte3); break;
                case 0b00000111: rm = std::format("[bx + {}]", byte3); break;
                } break;
            } break;
            case MOD_MEMORY_MODE_16_BIT_DISPLACEMENT:
            {
                u8 byte3 = *ptr;
                ptr++;
                u8 byte4 = *ptr;
                ptr++;
                // r/m mask
                switch (byte2 & 0b00000111) {
                case 0b00000000: rm = std::format("[bx + si + {}]", byte3 + byte4); break;
                case 0b00000001: rm = std::format("[bx + di + {}]", byte3 + byte4); break;
                case 0b00000010: rm = std::format("[bp + si + {}]", byte3 + byte4); break;
                case 0b00000011: rm = std::format("[bp + di + {}]", byte3 + byte4); break;
                case 0b00000100: rm = std::format("[si + {}]", byte3 + byte4); break;
                case 0b00000101: rm = std::format("[di + {}]", byte3 + byte4); break;
                case 0b00000110: rm = std::format("[bp + {}]", byte3 + byte4); break;
                case 0b00000111: rm = std::format("[bx + {}]", byte3 + byte4); break;
                } break;
            } break;
            case MOD_REGISTER_MODE_NO_DISPLACEMENT:
            {
                // r/m mask
                switch (byte2 & 0b00000111) {
                //                      check w bit to determine if register is 8 or 16 bits
                case 0b00000000: rm = ((byte1 & 0b00000001) > 0) ? "ax" : "al"; break;
                case 0b00000001: rm = ((byte1 & 0b00000001) > 0) ? "cx" : "cl"; break;
                case 0b00000010: rm = ((byte1 & 0b00000001) > 0) ? "dx" : "dl"; break;
                case 0b00000011: rm = ((byte1 & 0b00000001) > 0) ? "bx" : "bl"; break;
                case 0b00000100: rm = ((byte1 & 0b00000001) > 0) ? "sp" : "ah"; break;
                case 0b00000101: rm = ((byte1 & 0b00000001) > 0) ? "bp" : "ch"; break;
                case 0b00000110: rm = ((byte1 & 0b00000001) > 0) ? "si" : "dh"; break;
                case 0b00000111: rm = ((byte1 & 0b00000001) > 0) ? "di" : "bh"; break;
                }
            } break;
            }
            // reg mask
            switch (byte2 & 0b00111000) {
            //                       check w bit to determine if register is 8 or 16 bits
            case 0b00000000: reg = ((byte1 & 0b00000001) > 0) ? "ax" : "al"; break;
            case 0b00001000: reg = ((byte1 & 0b00000001) > 0) ? "cx" : "cl"; break;
            case 0b00010000: reg = ((byte1 & 0b00000001) > 0) ? "dx" : "dl"; break;
            case 0b00011000: reg = ((byte1 & 0b00000001) > 0) ? "bx" : "bl"; break;
            case 0b00100000: reg = ((byte1 & 0b00000001) > 0) ? "sp" : "ah"; break;
            case 0b00101000: reg = ((byte1 & 0b00000001) > 0) ? "bp" : "ch"; break;
            case 0b00110000: reg = ((byte1 & 0b00000001) > 0) ? "si" : "dh"; break;
            case 0b00111000: reg = ((byte1 & 0b00000001) > 0) ? "di" : "bh"; break;
            }
            
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
            OutputDebugString("unimplemented OPCODE_MASK_IMMEDIATE_TO_REGISTER_OR_MEMORY\n");
            ASSERT(false);
        } else if (OP_MOVE_IMMEDIATE_TO_REGISTER(byte1)) {
            OutputDebugString("unimplemented OPCODE_MASK_IMMEDIATE_TO_REGISTER\n");
            ASSERT(false);
        } else {
            OutputDebugString("encountered unimplemented opcode\n");
            ASSERT(false);
        }
    }
    File::write(std::format("test-data/output/{}-output.asm", inputName).c_str(), output);
}