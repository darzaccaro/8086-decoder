#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#include <Windows.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#define ASSERT(condition) assert(condition);
#include "Types.h"
#include "Win32Helpers.h"
#include "File.h"

// byte 1 bitmasks
#define OPCODE_MASK 0b11111100
#define D_MASK      0b00000010
#define W_MASK      0b00000001
// byte 2 bitmasks
#define MOD_MASK    0b11000000
#define REG_MASK    0b00111000
#define RM_MASK     0b00000111

#define OPCODE_MASK_MOVE 0b10001000

#define W_MASK_1_BYTE_REGISTER 0b00000000
#define W_MASK_2_BYTE_REGISTER 0b00000001

#define D_MASK_REG_IS_SOURCE 	  0b00000000
#define D_MASK_REG_IS_DESTINATION 0b00000010

// if no displacement and r/m = 110, then it's actually a 16 bit displacment (lol)
#define MOD_MASK_MEMORY_MODE_NO_DISPLACEMENT     0b00000000
#define MOD_MASK_MEMORY_MODE_8_BIT_DISPLACEMENT  0b01000000
#define MOD_MASK_MEMORY_MODE_16_BIT_DISPLACEMENT 0b10000000
#define MOD_MASK_REGISTER_MODE_NO_DISPLACEMENT   0b11000000

int main(int argc, char* argv[]) {
    using std::string;
    auto listing_0037_input     = new File("test-data/computer_enhance/perfaware/part1/listing_0037_single_register_mov");
    auto listing_0037_reference = new File("test-data/computer_enhance/perfaware/part1/listing_0037_single_register_mov.asm");
    auto listing_0038_input     = new File("test-data/computer_enhance/perfaware/part1/listing_0038_many_register_mov");
    auto listing_0038_reference = new File("test-data/computer_enhance/perfaware/part1/listing_0038_many_register_mov.asm");
    auto listing_0039_input     = new File("test-data/computer_enhance/perfaware/part1/listing_0039_more_movs");
    auto listing_0039_reference = new File("test-data/computer_enhance/perfaware/part1/listing_0039_more_movs.asm");
    auto listing_0040_input     = new File("test-data/computer_enhance/perfaware/part1/listing_0040_challenge_movs");
    auto listing_0040_reference = new File("test-data/computer_enhance/perfaware/part1/listing_0040_challenge_movs.asm");

    {
        auto input = listing_0037_input;
        string output;
        u8* ptr = input->data;
        while (ptr < input->data + input->size) {
            u8 byte1 = *ptr;
            ptr++;
            switch (byte1 & OPCODE_MASK) {
            case OPCODE_MASK_MOVE:
            {
                u8 byte2 = *ptr;
                ptr++;
                switch (byte2 & MOD_MASK) {
                case MOD_MASK_MEMORY_MODE_NO_DISPLACEMENT:
                {
                    switch (byte2 & RM_MASK) {
                    case 0b00000000:
                    {

                    } break;
                    }
                } break;
                }
            } break;
            default:
                OutputDebugString("encountered unimplemented opcode");
                ASSERT(false);
            }

        }

    }
}

