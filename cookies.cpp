// Info source:
// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

#include <cstdint>
#include <cstdio>

#include <SFML/Graphics.hpp>

using u8 = uint8_t;
using u16 = uint16_t;

struct Chip8_State
{
    u8 V[0x0010];
    u16 I;
    u8 DT;
    u8 ST;
    u16 PC;
    u16 SP;
    u8 mem[0x1000];
    u8 * display;

    Chip8_State() : DT(0), ST(0), PC(0x200), SP(0xEA0) {
        this->display = this->mem + 0xF00;
        for (int i = 0; i < 256; i++)
            this->display[i] = 0;
    }
};

inline bool ReadProgram(const char * program_path, Chip8_State * c8)
{
    FILE * program = fopen(program_path, "rb");
    if (program == NULL) {
        printf("Error: File %s doesn't exist\n", program_path);
        return false;
    }

    fseek(program, 0, SEEK_END);
    long program_size = ftell(program);
    rewind(program);

    fread(c8->mem + c8->PC, 1, (size_t) program_size, program);

    fclose(program);
    return true;
}

int main(int argc, const char ** argv)
{
    if (argc < 2) {
        printf("Error: Program path not specified\n");
        printf("Usage: %s program-path\n", argv[0]);
        return 1;
    }

    // Init CPU and memory
    Chip8_State c8;

    // Read program from file
    if (!ReadProgram(argv[1], &c8))
        return 1;

    u16 xDD = c8.mem[c8.PC] << 8 | c8.mem[c8.PC + 1];
    printf("%.4X\n", xDD);

    // Open window
    sf::RenderWindow window(sf::VideoMode(640, 320), "Cookies");

    u8 randomst = 1;
    u16 opcode;
    sf::Event event;

    sf::RectangleShape rectangle(sf::Vector2f(10, 10));
    rectangle.setFillColor(sf::Color(255, 255, 255));
    while (window.isOpen()) {
        opcode = c8.mem[c8.PC] << 8 | c8.mem[c8.PC + 1];
        c8.PC += 0x0002;

//        printf("%.4X\n", opcode);

        // @Todo: timers...

        // Process events
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        switch (opcode & 0xF000) {
        /* 0___ - ??? */
        case 0x0000:
            switch (opcode) {
            /* 00E0 - CLS
            Clear the display. */
            case 0x00E0:
                for (int i = 0; i < 256; i++)
                    c8.display[i] = 0;
                break;
            /* 00EE - RET
            Return from a subroutine.
            The interpreter sets the program counter to the address at the top
            of the stack, then subtracts 1 from the stack pointer. */
            case 0x00EE:
                c8.PC = (c8.mem[c8.SP] << 8) + c8.mem[c8.SP + 1];
                c8.SP -= 2;
                break;
            /* 0nnn - SYS addr
            Jump to a machine code routine at nnn.
            This instruction is only used on the old computers on which Chip-8
            was originally implemented. It is ignored by modern interpreters. */
            default:
//                printf("Found SYS opcode: %.4X\n", opcode);
                break;
            }
            break;
        /* 1nnn - JP addr
        Jump to location nnn.
        The interpreter sets the program counter to nnn. */
        case 0x1000:
            c8.PC = (u16) (opcode & 0x0FFF);
            break;
        /* 2nnn - CALL addr
        Call subroutine at nnn.
        The interpreter increments the stack pointer, then puts the current PC
        on the top of the stack. The PC is then set to nnn. */
        case 0x2000:
            c8.SP += 2;
            c8.mem[c8.SP] = (u8) (c8.PC >> 8);
            c8.mem[c8.SP + 1] = (u8) c8.PC;
            c8.PC = (u16) (opcode & 0x0FFF);
            break;
        /* 3xkk - SE Vx, byte
        Skip next instruction if Vx = kk.
        The interpreter compares register Vx to kk, and if they are equal,
        increments the program counter by 2. */
        case 0x3000:
            if (c8.V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                c8.PC += 2;
            break;
        /* 4xkk - SNE Vx, byte
        Skip next instruction if Vx != kk.
        The interpreter compares register Vx to kk, and if they are not equal,
        increments the program counter by 2. */
        case 0x4000:
            if (c8.V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                c8.PC += 2;
            break;
        /* 6xkk - LD Vx, byte
        Set Vx = kk.
        The interpreter puts the value kk into register Vx. */
        case 0x6000:
            c8.V[(opcode & 0x0F00) >> 8] = (u8) (opcode & 0x00FF);
            break;
        /* 7xkk - ADD Vx, byte
        Set Vx = Vx + kk.
        Adds the value kk to the value of register Vx, then stores the
        result in Vx. */
        case 0x7000:
            c8.V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;
        /* 8___ - ALU or some shit */
        case 0x8000:
            switch (opcode & 0x000F) {
            /* 8xy0 - LD Vx, Vy
            Set Vx = Vy.
            Stores the value of register Vy in register Vx. */
            case 0x0000:
                c8.V[(opcode & 0x0F00) >> 8] = c8.V[(opcode & 0x00F0) >> 8];
                break;
            /* 8xy2 - AND Vx, Vy
            Set Vx = Vx AND Vy.
            Performs a bitwise AND on the values of Vx and Vy, then stores
            the result in Vx. A bitwise AND compares the corrseponding bits
            from two values, and if both bits are 1, then the same bit in
            the result is also 1. Otherwise, it is 0. */
            case 0x0002:
                c8.V[(opcode & 0x0F00) >> 8] &= c8.V[(opcode & 0x00F0) >> 8];
                break;
            /* 8xy4 - ADD Vx, Vy
            Set Vx = Vx + Vy, set VF = carry.
            The values of Vx and Vy are added together. If the result is
            greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
            Only the lowest 8 bits of the result are kept, and
            stored in Vx. */
            case 0x0004: {
                u8 x = c8.V[(opcode & 0x0F00) >> 8];
                u8 y = c8.V[(opcode & 0x00F0) >> 8];
                u8 r = x + y;
                c8.V[0xF] = (u8) (r < x || r < y ? 0x0001 : 0x0000);
                c8.V[(opcode & 0x0F00) >> 8] = r;
                break;
            }
            /* 8xy5 - SUB Vx, Vy
            Set Vx = Vx - Vy, set VF = NOT borrow.
            If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted
            from Vx, and the results stored in Vx. */
            case 0x0005: {
                u8 x = c8.V[(opcode & 0x0F00) >> 8];
                u8 y = c8.V[(opcode & 0x00F0) >> 8];
                c8.V[0xF] = (u8) (x > y ? 0x0001 : 0x0000);
                c8.V[(opcode & 0x0F00) >> 8] = x - y;
                break;
            }
            default:
                printf("Error: unimplemented ALU op: %.4X\n", opcode);
                break;
            }
            break;
        /* Annn - LD I, addr
        Set I = nnn.
        The value of register I is set to nnn. */
        case 0xA000:
            c8.I = (u16) (opcode & 0x0FFF);
            break;
        /* Cxkk - RND Vx, byte
        Set Vx = random byte AND kk.
        The interpreter generates a random number from 0 to 255, which is
        then ANDed with the value kk. The results are stored in Vx. See
        instruction 8xy2 for more information on AND. */
        case 0xC000:
            // Xorshift
            randomst ^= (randomst << 7);
            randomst ^= (randomst >> 5);
            randomst ^= (randomst << 3);
            c8.V[(opcode & 0x0F00) >> 8] = (u8) (opcode & 0x00FF & randomst);
            break;
        /* Dxyn - DRW Vx, Vy, nibble
        Display n-byte sprite starting at memory location I at (Vx, Vy),
        set VF = collision.
        The interpreter reads n bytes from memory, starting at the address
        stored in I. These bytes are then displayed as sprites on screen at
        coordinates (Vx, Vy). Sprites are XORed onto the existing screen. If
        this causes any pixels to be erased, VF is set to 1, otherwise it is
        set to 0. If the sprite is positioned so part of it is outside the
        coordinates of the display, it wraps around to the opposite side of
        the screen. See instruction 8xy3 for more information on XOR, and
        section 2.4, Display, for more information on the Chip-8 screen and
        sprites. */
        case 0xD000: {
            // http://www.emulator101.com/chip-8-sprites.html
            // i'm not sure if it's well implemented and i'm 100% sure it can be optimized
            u8 n = (u8) (opcode & 0x000F);
            u16 i = c8.I;
            u8 offset = (u8) (c8.V[(opcode & 0x0F00) >> 8] + 64 * c8.V[(opcode & 0x00F0) >> 8]);

            u8 bitshift;
            while (n > 0) {
                bitshift = 8;
                while (bitshift > 0) {
                    bitshift--;

                    if (c8.mem[i] & (1 << bitshift)) {
                        // Determine the address of the effected byte on the screen
                        u8 * address = c8.display + offset / 8;
                        // Determine the effected bit in the byte
                        u8 effected_bit = (u8) (offset % 8);
                        // Check to see if the screen's bit is set and set VF appropriately
                        if (*address & (1 << effected_bit))
                            c8.V[0xF] = 1;
                        // XOR the source bit and screen bit
                        // Write the effected bit to the screen
                        *address ^= 1 << effected_bit;
                    }
                    offset++;
                }
                offset += 56;
                i++;
                n--;
            }
            break;
        }
        // E___ - Keyboard
        case 0xE000:
            switch (opcode & 0x00FF) {
            /* ExA1 - SKNP Vx
            Skip next instruction if key with the value of Vx is not pressed
            Checks the keyboard, and if the key corresponding to the value
            of Vx is currently in the up position, PC is increased by 2. */
            case 0x00A1:
                // @Todo: PONG
                break;
            default:
                printf("Error: unimplemented keyboard op: %.4X\n", opcode);
                break;
            }
            break;
        // F___ - Different functions
        case 0xF000:
            switch (opcode & 0x00FF) {
            /* Fx07 - LD Vx, DT
            Set Vx = delay timer value.
            The value of DT is placed into Vx. */
            case 0x0007:
                c8.V[(opcode & 0x0F00) >> 8] = c8.DT;
                break;
            /* Fx15 - LD DT, Vx
            Set delay timer = Vx.
            DT is set equal to the value of Vx. */
            case 0x0015:
                c8.DT = c8.V[(opcode & 0x0F00) >> 8];
                break;
            /* Fx18 - LD ST, Vx
            Set sound timer = Vx.
            ST is set equal to the value of Vx. */
            case 0x0018:
                c8.ST = c8.V[(opcode & 0x0F00) >> 8];
                break;
            /* Fx29 - LD F, Vx
            Set I = location of sprite for digit Vx.
            The value of I is set to the location for the hexadecimal sprite
            corresponding to the value of Vx. See section 2.4, Display, for more
            information on the Chip-8 hexadecimal font. */
            case 0x0029:
                // @Todo: PONG
                break;
            /* Fx33 - LD B, Vx
            Store BCD representation of Vx in memory locations I, I+1, and I+2.
            The interpreter takes the decimal value of Vx, and places the
            hundreds digit in memory at location in I, the tens digit at
            location I+1, and the ones digit at location I+2. */
            case 0x0033: {
                u8 x = c8.V[(opcode & 0x0F00) >> 8];
                c8.mem[c8.I] = (u8) (x / 100);
                c8.mem[c8.I + 1] = (u8) (x / 10 % 10);
                c8.mem[c8.I + 2] = (u8) (x % 10);
                break;
            }
            /* Fx65 - LD Vx, [I]
            Read registers V0 through Vx from memory starting at location I.
            The interpreter reads values from memory starting at location I into
            registers V0 through Vx. */
            case 0x0065: {
                u8 x = c8.V[(opcode & 0x0F00) >> 8];
                for (int i = 0; i <= x; i++)
                    c8.V[i] = c8.mem[c8.I + i];
                break;
            }
            default:
                printf("Error: unimplemented FXXX function: %.4X\n", opcode);
                break;
            }
            break;
        default:
            printf("Error: unimplemented instruction: %.4X\n", opcode);
            break;
        }

        // Update screen
        int z = 0;
        for (int j = 0; j < 320; j += 10) {
            for (int i = 0; i < 640; i += 10) {
                u8 displaybyte = c8.display[z];

                if (displaybyte & 0b10000000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b01000000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00100000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00010000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00001000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00000100) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00000010) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                i += 10;
                if (displaybyte & 0b00000001) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }

                z++;
            }
        }
        window.display();

        // @Todo: timers, display...
    }

    // Cleanup?
    return 0;
}
