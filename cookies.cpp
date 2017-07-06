// Info source:
// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

#include <cstdint>
#include <cstdio>

#include <SFML/Graphics.hpp>

using u8 = uint8_t;
using u16 = uint16_t;

struct Chip8_CPU
{
    u8 V[0x0010];
    u16 I;
    u8 DT;
    u8 ST;
    u16 PC;
    u16 SP;
};

int main(int argc, const char ** argv)
{
    if (argc < 2) {
        printf("Error: Program path not specified\n");
        printf("Usage: %s program-path\n", argv[0]);
        return 1;
    }

    // Init CPU and memory
    Chip8_CPU cpu;
    cpu.PC = 0x00000200;
    cpu.DT = 0x0000;
    cpu.ST = 0x0000;
    cpu.SP = 0x00000EA0;
    u8 memory[0x1000];
    u8 * display = memory + 0xF00;

    // Read program from file
    FILE * program = fopen(argv[1], "rb");
    if (program == NULL) {
        printf("Error: File %s doesn't exist\n", argv[1]);
        return 1;
    }
    fseek(program, 0, SEEK_END);
    long program_size = ftell(program);
    rewind(program);
    fread(memory + cpu.PC, 1, program_size, program);
    fclose(program);

    // Open window
    sf::RenderWindow window(sf::VideoMode(640, 320), "Cookies");

    u8 randomst = 1;
    u16 opcode;
    sf::Event event;

    sf::RectangleShape rectangle(sf::Vector2f(10, 10));
    rectangle.setFillColor(sf::Color(255, 255, 255));
    while (window.isOpen()) {
        opcode = memory[cpu.PC] << 8 + memory[cpu.PC + 1];
        cpu.PC += 0x0002;

        printf("%.4X\n", opcode);

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
//                printf("Unimplemented: CLEAR DISPLAY\n");
                break;
            /* 00EE - RET
            Return from a subroutine.
            The interpreter sets the program counter to the address at the top
            of the stack, then subtracts 1 from the stack pointer. */
            case 0x00EE:
                cpu.PC = (memory[cpu.SP] << 8) + memory[cpu.SP + 1];
                cpu.SP -= 2;
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
            cpu.PC = opcode & 0x0FFF;
            break;
        /* 2nnn - CALL addr
        Call subroutine at nnn.
        The interpreter increments the stack pointer, then puts the current PC
        on the top of the stack. The PC is then set to nnn. */
        case 0x2000:
            cpu.SP += 2;
            memory[cpu.SP] = cpu.PC >> 8;
            memory[cpu.SP + 1] = cpu.PC;
            cpu.PC = opcode & 0x0FFF;
            break;
        /* 3xkk - SE Vx, byte
        Skip next instruction if Vx = kk.
        The interpreter compares register Vx to kk, and if they are equal,
        increments the program counter by 2. */
        case 0x3000:
            if (cpu.V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
                cpu.PC += 2;
            break;
        /* 4xkk - SNE Vx, byte
        Skip next instruction if Vx != kk.
        The interpreter compares register Vx to kk, and if they are not equal,
        increments the program counter by 2. */
        case 0x4000:
            if (cpu.V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
                cpu.PC += 2;
            break;
        /* 6xkk - LD Vx, byte
        Set Vx = kk.
        The interpreter puts the value kk into register Vx. */
        case 0x6000:
            cpu.V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;
        /* 7xkk - ADD Vx, byte
        Set Vx = Vx + kk.
        Adds the value kk to the value of register Vx, then stores the
        result in Vx. */
        case 0x7000:
            cpu.V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
            break;
        /* 8___ - ALU or some shit */
        case 0x8000:
            switch (opcode & 0x000F) {
            /* 8xy0 - LD Vx, Vy
            Set Vx = Vy.
            Stores the value of register Vy in register Vx. */
            case 0x0000:
                cpu.V[(opcode & 0x0F00) >> 8] = cpu.V[(opcode & 0x00F0) >> 8];
                break;
            /* 8xy2 - AND Vx, Vy
            Set Vx = Vx AND Vy.
            Performs a bitwise AND on the values of Vx and Vy, then stores
            the result in Vx. A bitwise AND compares the corrseponding bits
            from two values, and if both bits are 1, then the same bit in
            the result is also 1. Otherwise, it is 0. */
            case 0x0002:
                cpu.V[(opcode & 0x0F00) >> 8] &= cpu.V[(opcode & 0x00F0) >> 8];
                break;
            /* 8xy4 - ADD Vx, Vy
            Set Vx = Vx + Vy, set VF = carry.
            The values of Vx and Vy are added together. If the result is
            greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
            Only the lowest 8 bits of the result are kept, and
            stored in Vx. */
            case 0x0004: {
                u8 x = cpu.V[(opcode & 0x0F00) >> 8];
                u8 y = cpu.V[(opcode & 0x00F0) >> 8];
                u8 r = x + y;
                cpu.V[0xF] = r < x || r < y ? 0x0001 : 0x0000;
                cpu.V[(opcode & 0x0F00) >> 8] = r;
                break;
            }
            /* 8xy5 - SUB Vx, Vy
            Set Vx = Vx - Vy, set VF = NOT borrow.
            If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted
            from Vx, and the results stored in Vx. */
            case 0x0005: {
                u8 x = cpu.V[(opcode & 0x0F00) >> 8];
                u8 y = cpu.V[(opcode & 0x00F0) >> 8];
                cpu.V[0xF] = x > y ? 0x0001 : 0x0000;
                cpu.V[(opcode & 0x0F00) >> 8] = x - y;
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
            cpu.I = opcode & 0x0FFF;
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
            cpu.V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF & randomst;
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
            u8 n = opcode & 0x000F;
            u16 i = cpu.I;
            u8 offset = cpu.V[(opcode & 0x0F00) >> 8] +
                64 * cpu.V[(opcode & 0x00F0) >> 8];

            u8 bitshift;
            while (n > 0) {
                bitshift = 8;
                while (bitshift > 0) {
                    bitshift--;

                    if (memory[i] & (1 << bitshift)) {
                        // Determine the address of the effected byte on the screen
                        u8 * address = display + offset / 8;
                        // Determine the effected bit in the byte
                        u8 effected_bit = offset % 8;
                        // Check to see if the screen's bit is set and set VF appropriately
                        if (*address & (1 << effected_bit))
                            cpu.V[0xF] = 1;
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
                cpu.V[(opcode & 0x0F00) >> 8] = cpu.DT;
                break;
            /* Fx15 - LD DT, Vx
            Set delay timer = Vx.
            DT is set equal to the value of Vx. */
            case 0x0015:
                cpu.DT = cpu.V[(opcode & 0x0F00) >> 8];
                break;
            /* Fx18 - LD ST, Vx
            Set sound timer = Vx.
            ST is set equal to the value of Vx. */
            case 0x0018:
                cpu.ST = cpu.V[(opcode & 0x0F00) >> 8];
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
                u8 x = cpu.V[(opcode & 0x0F00) >> 8];
                memory[cpu.I] = x / 100;
                memory[cpu.I + 1] = x / 10 % 10;
                memory[cpu.I + 2] = x % 10;
                break;
            }
            /* Fx65 - LD Vx, [I]
            Read registers V0 through Vx from memory starting at location I.
            The interpreter reads values from memory starting at location I into
            registers V0 through Vx. */
            case 0x0065: {
                u8 x = cpu.V[(opcode & 0x0F00) >> 8];
                for (int i = 0; i <= x; i++)
                    cpu.V[i] = memory[cpu.I + i];
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
                u8 displaybyte = display[z];

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
