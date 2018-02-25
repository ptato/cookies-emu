// http://devernay.free.fr/hacks/chip8/C8TECH10.HTM

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "SDL.h"

//     1 2 3 C       1 2 3 4
//     4 5 6 D  ==>  Q W E R
//     7 8 9 E  ==>  A S D F
//     A 0 B F       Z X C V
// TODO: I don't know what the fuck I did here
static const SDL_Keycode keys[16] =
{
        SDLK_x,
        SDLK_1, SDLK_2, SDLK_3,
        SDLK_q, SDLK_w, SDLK_e,
        SDLK_a, SDLK_s, SDLK_d,
        SDLK_z, SDLK_c,
        SDLK_4, SDLK_r, SDLK_f, SDLK_v,
};

static const unsigned char Sprites[80]
{
        // 0
        0b11110000,
        0b10010000,
        0b10010000,
        0b10010000,
        0b11110000,

        // 1
        0b00100000,
        0b01100000,
        0b00100000,
        0b00100000,
        0b01110000,

        // 2
        0b11110000,
        0b00010000,
        0b11110000,
        0b10000000,
        0b11110000,

        // 3
        0b11110000,
        0b00010000,
        0b11110000,
        0b00010000,
        0b11110000,

        // 4
        0b10010000,
        0b10010000,
        0b11110000,
        0b00010000,
        0b00010000,

        // 5
        0b11110000,
        0b10000000,
        0b11110000,
        0b00010000,
        0b11110000,

        // 6
        0b11110000,
        0b10000000,
        0b11110000,
        0b10010000,
        0b11110000,

        // 7
        0b11110000,
        0b00010000,
        0b00100000,
        0b01000000,
        0b01000000,

        // 8
        0b11110000,
        0b10010000,
        0b11110000,
        0b10010000,
        0b11110000,

        // 9
        0b11110000,
        0b10010000,
        0b11110000,
        0b00010000,
        0b11110000,

        // A
        0b11110000,
        0b10010000,
        0b11110000,
        0b10010000,
        0b10010000,

        // B
        0b11100000,
        0b10010000,
        0b11100000,
        0b10010000,
        0b11100000,

        // C
        0b11110000,
        0b10000000,
        0b10000000,
        0b10000000,
        0b11110000,

        // D
        0b11100000,
        0b10010000,
        0b10010000,
        0b10010000,
        0b11100000,

        // E
        0b11110000,
        0b10000000,
        0b11110000,
        0b10000000,
        0b11110000,

        // F
        0b11110000,
        0b10000000,
        0b11110000,
        0b10000000,
        0b10000000
};

struct chip8_state
{
    unsigned char V[0x0010];
    uint16_t      I;
    unsigned char DT;
    unsigned char ST;
    uint16_t      PC;
    uint16_t      SP;
    unsigned char M[0x1000];
    unsigned char *DISP;

    unsigned char RandomST;
};

static void Chip8InitialState(chip8_state *Out)
{
    chip8_state State = { 0 };
    *Out = State;

    Out->PC = 0x200;
    Out->SP = 0xEA0;
    Out->DISP = Out->M + 0xF00;
    memcpy(Out->M, Sprites, sizeof(Sprites));

    Out->RandomST = 1;
}

static bool ReadProgram(const char *Filename, chip8_state *C8)
{
    FILE *Program = fopen(Filename, "rb");
    if (Program == NULL)
    {
        printf("Error: File %s doesn't exist\n", Filename);
        return false;
    }

    fseek(Program, 0, SEEK_END);
    long ProgramSize = ftell(Program);
    rewind(Program);

    fread(C8->M + C8->PC, 1, (size_t)ProgramSize, Program);

    fclose(Program);
    return true;
}

int main(int argc, const char ** argv)
{
    if (argc < 2)
    {
        printf("Error: Program path not specified\n");
        printf("Usage: %s program-path\n", argv[0]);
        return 1;
    }

    chip8_state Chip8;
    Chip8InitialState(&Chip8);

    if (!ReadProgram(argv[1], &c8))
        return 1;

    SDL_Window *Window = SDL_CreateWindow("Cookies", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 320, 0);


    uint32_t LastCounter = SDL_GetTicks();
    uint32_t ClockPeriod = 17;


    sf::RectangleShape rectangle(sf::Vector2f(10, 10));
    rectangle.setFillColor(sf::Color(255, 255, 255));

    while (window.isOpen()) {
        uint16_t OPCode = C8.M[C8.PC] << 8 | C8.M[C8.PC + 1];
        C8.PC += 0x0002;

        sf::Event event;
        while (window.pollEvent(event))
        {
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
                break;
            }
            break;
        /* 1nnn - JP addr
        Jump to location nnn.
        The interpreter sets the program counter to nnn. */
        case 0x1000:
            c8.PC = (uint16_t) (opcode & 0x0FFF);
            break;
        /* 2nnn - CALL addr
        Call subroutine at nnn.
        The interpreter increments the stack pointer, then puts the current PC
        on the top of the stack. The PC is then set to nnn. */
        case 0x2000:
            c8.SP += 2;
            c8.mem[c8.SP] = (unsigned char) (c8.PC >> 8);
            c8.mem[c8.SP + 1] = (unsigned char) c8.PC;
            c8.PC = (uint16_t) (opcode & 0x0FFF);
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
        /* 5xy0 - SE Vx, Vy
        Skip next instruction if Vx = Vy.
        The interpreter compares register Vx to register Vy, and if they are
        equal, increments the program counter by 2. */
        case 0x5000:
            if (c8.V[(opcode & 0x0F00) >> 8] == c8.V[(opcode & 0x0FF0) >> 4])
                c8.PC += 2;
            break;
        /* 6xkk - LD Vx, byte
        Set Vx = kk.
        The interpreter puts the value kk into register Vx. */
        case 0x6000:
            c8.V[(opcode & 0x0F00) >> 8] = (unsigned char) (opcode & 0x00FF);
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
                c8.V[(opcode & 0x0F00) >> 8] = c8.V[(opcode & 0x00F0) >> 4];
                break;
            /* 8xy1 - OR Vx, Vy
            Set Vx = Vx OR Vy.
            Performs a bitwise OR on the values of Vx and Vy, then stores the
            result in Vx. A bitwise OR compares the corrseponding bits from
            two values, and if either bit is 1, then the same bit in the result
            is also 1. Otherwise, it is 0. */
            case 0x0001:
                c8.V[(opcode & 0x0F00) >> 8] |= c8.V[(opcode & 0x00F0) >> 4];
                break;
            /* 8xy2 - AND Vx, Vy
            Set Vx = Vx AND Vy.
            Performs a bitwise AND on the values of Vx and Vy, then stores
            the result in Vx. A bitwise AND compares the corrseponding bits
            from two values, and if both bits are 1, then the same bit in
            the result is also 1. Otherwise, it is 0. */
            case 0x0002:
                c8.V[(opcode & 0x0F00) >> 8] &= c8.V[(opcode & 0x00F0) >> 4];
                break;
            /* 8xy3 - XOR Vx, Vy
            Set Vx = Vx XOR Vy.
            Performs a bitwise exclusive OR on the values of Vx and Vy, then
            stores the result in Vx. An exclusive OR compares the corrseponding
            bits from two values, and if the bits are not both the same, then
            the corresponding bit in the result is set to 1. Otherwise, it is 0. */
            case 0x0003:
                c8.V[(opcode & 0x0F00) >> 8] ^= c8.V[(opcode & 0x00F0) >> 4];
                break;
            /* 8xy4 - ADD Vx, Vy
            Set Vx = Vx + Vy, set VF = carry.
            The values of Vx and Vy are added together. If the result is
            greater than 8 bits (i.e., > 255,) VF is set to 1, otherwise 0.
            Only the lowest 8 bits of the result are kept, and
            stored in Vx. */
            case 0x0004: {
                unsigned char x = c8.V[(opcode & 0x0F00) >> 8];
                unsigned char y = c8.V[(opcode & 0x00F0) >> 4];
                unsigned char r = x + y;
                c8.V[0xF] = (unsigned char) (r < x || r < y ? 0x0001 : 0x0000);
                c8.V[(opcode & 0x0F00) >> 8] = r;
                break;
            }
            /* 8xy5 - SUB Vx, Vy
            Set Vx = Vx - Vy, set VF = NOT borrow.
            If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted
            from Vx, and the results stored in Vx. */
            case 0x0005: {
                unsigned char x = c8.V[(opcode & 0x0F00) >> 8];
                unsigned char y = c8.V[(opcode & 0x00F0) >> 4];
                c8.V[0xF] = (unsigned char) (x > y ? 0x0001 : 0x0000);
                c8.V[(opcode & 0x0F00) >> 8] = x - y;
                break;
            }
            /* 8xy6 - SHR Vx {, Vy}
            Set Vx = Vx SHR 1.
            If the least-significant bit of Vx is 1, then VF is set to 1,
            otherwise 0. Then Vx is divided by 2. */
            case 0x0006:
                if (c8.V[(opcode & 0x0F00) >> 8] & 0x01)
                    c8.V[0xF] = 1;
                c8.V[(opcode & 0x0F00) >> 8] >>= 1;
                break;
            /* 8xy7 - SUBN Vx, Vy
            Set Vx = Vy - Vx, set VF = NOT borrow.
            If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is
            subtracted from Vy, and the results stored in Vx. */
            case 0x0007: {
                unsigned char x = c8.V[(opcode & 0x0F00) >> 8];
                unsigned char y = c8.V[(opcode & 0x00F0) >> 4];
                c8.V[0xF] = (unsigned char) (y > x ? 0x0001 : 0x0000);
                c8.V[(opcode & 0x0F00) >> 8] = y - x;
                break;
            }
            /* 8xyE - SHL Vx {, Vy}
            Set Vx = Vx SHL 1.
            If the most-significant bit of Vx is 1, then VF is set to 1,
            otherwise to 0. Then Vx is multiplied by 2. */
            case 0x000E:
                if (c8.V[(opcode & 0x0F00) >> 8] & 0x80)
                    c8.V[0xF] = 1;
                c8.V[(opcode & 0x0F00) >> 8] <<= 1;
                break;
            default:
                break;
            }
            break;
        /* 9xy0 - SNE Vx, Vy
        Skip next instruction if Vx != Vy.
        The values of Vx and Vy are compared, and if they are not
        equal, the program counter is increased by 2. */
        case 0x9000:
            if (c8.V[(opcode & 0x0F00) >> 8] != c8.V[(opcode & 0x0FF0) >> 4])
                c8.PC += 2;
            break;
        /* Annn - LD I, addr
        Set I = nnn.
        The value of register I is set to nnn. */
        case 0xA000:
            c8.I = (uint16_t) (opcode & 0x0FFF);
            break;
        /* Bnnn - JP V0, addr
        Jump to location nnn + V0.
        The program counter is set to nnn plus the value of V0. */
        case 0xB000:
            c8.PC = (uint16_t) ((opcode & 0x0FFF) + c8.V[0]);
            break;
        /* Cxkk - RND Vx, byte
        Set Vx = random byte AND kk.
        The interpreter generates a random number from 0 to 255, which is
        then ANDed with the value kk. The results are stored in Vx. See
        instruction 8xy2 for more information on AND. */
        case 0xC000:
        {
            // Xorshift
            C8.RandomST ^= (C8.RandomST << 7);
            C8.RandomST ^= (C8.RandomST >> 5);
            mst ^= (C8.RandomST << 3);
            c8.V[(opcode & 0x0F00) >> 8] = (unsigned char) (opcode & 0x00FF & C8.RandomST);
        } break;
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
            unsigned char n = (unsigned char) (opcode & 0x000F);
            unsigned char x = c8.V[(opcode & 0x0F00) >> 8];
            unsigned char y = c8.V[(opcode & 0x00F0) >> 4];
            unsigned char display_byte = (unsigned char) ((y * 64 + x) >> 3);
            unsigned char offset = (unsigned char) (x % 8);
            c8.V[0xF] = 0;
            for (int i = 0; i < n; i++) {
                if (offset != 0) {
                    if (c8.display[display_byte] & (c8.mem[c8.I + i] >> offset))
                        c8.V[0xF] = 1;
                    c8.display[display_byte] ^= (c8.mem[c8.I + i] >> offset);
                    if (c8.display[display_byte + 1] & (c8.mem[c8.I + i] << (8 - offset)))
                        c8.V[0xF] = 1;
                    c8.display[display_byte + 1] ^= (c8.mem[c8.I + i] << (8 - offset));
                } else {
                    if (c8.display[display_byte] & c8.mem[c8.I + i])
                        c8.V[0xF] = 1;
                    c8.display[display_byte] ^= c8.mem[c8.I + i];
                }
                display_byte += 8;
            }
            break;
        }
        // E___ - Keyboard
        case 0xE000:
            switch (opcode & 0x00FF) {
            /* Ex9E - SKP Vx
            Skip next instruction if key with the value of Vx is pressed.
            Checks the keyboard, and if the key corresponding to the value
            of Vx is currently in the down position, PC is increased by 2. */
            case 0x009E:
                if (sf::Keyboard::isKeyPressed(keys[(opcode & 0x0F00) >> 8]))
                    c8.PC += 2;
                break;
            /* ExA1 - SKNP Vx
            Skip next instruction if key with the value of Vx is not pressed
            Checks the keyboard, and if the key corresponding to the value
            of Vx is currently in the up position, PC is increased by 2. */
            case 0x00A1:
                if (!sf::Keyboard::isKeyPressed(keys[(opcode & 0x0F00) >> 8]))
                    c8.PC += 2;
                break;
            default:
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
            /* Fx0A - LD Vx, K
            Wait for a key press, store the value of the key in Vx.
            All execution stops until a key is pressed, then the
            value of that key is stored in Vx. */
            case 0x000A: {
                bool pressed = false;
                while (!pressed) {
                    window.pollEvent(event);
                    switch (event.type) {
                        case sf::Event::Closed:
                            window.close();
                            break;
                        case sf::Event::KeyPressed:
                            for (int i = 0; i < 16; i++) {
                                if (event.key.code == keys[i]) {
                                    pressed = true;
                                    c8.V[(opcode & 0x0F00) >> 8] = (unsigned char) i;
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
                break;
            }
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
            /* Fx1E - ADD I, Vx
            Set I = I + Vx.
            The values of I and Vx are added, and the results are stored in I. */
            case 0x001E:
                c8.I += c8.V[(opcode & 0x0F00) >> 8];
                break;
            /* Fx29 - LD F, Vx
            Set I = location of sprite for digit Vx.
            The value of I is set to the location for the hexadecimal sprite
            corresponding to the value of Vx. See section 2.4, Display, for more
            information on the Chip-8 hexadecimal font. */
            case 0x0029:
                c8.I = (uint16_t) (c8.V[(opcode & 0x0F00) >> 8] * 5);
                break;
            /* Fx33 - LD B, Vx
            Store BCD representation of Vx in memory locations I, I+1, and I+2.
            The interpreter takes the decimal value of Vx, and places the
            hundreds digit in memory at location in I, the tens digit at
            location I+1, and the ones digit at location I+2. */
            case 0x0033: {
                unsigned char x = c8.V[(opcode & 0x0F00) >> 8];
                c8.mem[c8.I] = (unsigned char) (x / 100);
                c8.mem[c8.I + 1] = (unsigned char) (x / 10 % 10);
                c8.mem[c8.I + 2] = (unsigned char) (x % 10);
                break;
            }
            /* Fx55 - LD [I], Vx
            Store registers V0 through Vx in memory starting at location I.
            The interpreter copies the values of registers V0 through Vx into
            memory, starting at the address in I. */
            case 0x0055: {
                unsigned char x = (unsigned char) ((opcode & 0x0F00) >> 8);
                for (int i = 0; i <= x; i++)
                    c8.mem[c8.I + i] = c8.V[i];
                break;
            }
            /* Fx65 - LD Vx, [I]
            Read registers V0 through Vx from memory starting at location I.
            The interpreter reads values from memory starting at location I into
            registers V0 through Vx. */
            case 0x0065: {
                unsigned char x = (unsigned char) ((opcode & 0x0F00) >> 8);
                for (int i = 0; i <= x; i++)
                    c8.V[i] = c8.mem[c8.I + i];
                break;
            }
            default:
                break;
            }
            break;
        default:
            printf("Unknown instruction %.4X\n", opcode);
            break;
        }

        // Update screen
        window.clear(sf::Color(0, 0, 0));
        int z = 0;
        for (int j = 0; j < 320; j += 10) {
            for (int i = 0; i < 640; i += 80) {
                unsigned char displaybyte = c8.display[z];

                if (displaybyte & 0b10000000) {
                    rectangle.setPosition(i, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b01000000) {
                    rectangle.setPosition(i + 10, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00100000) {
                    rectangle.setPosition(i + 20, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00010000) {
                    rectangle.setPosition(i + 30, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00001000) {
                    rectangle.setPosition(i + 40, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00000100) {
                    rectangle.setPosition(i + 50, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00000010) {
                    rectangle.setPosition(i + 60, j);
                    window.draw(rectangle);
                }
                if (displaybyte & 0b00000001) {
                    rectangle.setPosition(i + 70, j);
                    window.draw(rectangle);
                }

                z++;
            }
        }
        window.display();

        uint32_t EndCounter = SDL_GetTicks();
        ClockPeriod = ClockPeriod - (EndCounter - LastCounter);
        while (ClockPeriod <= 0)
        {
            ClockPeriod += 17;
            if (C8.DT > 0) C8.DT--;
            if (C8.ST > 0) C8.ST--;
        }

        LastCounter = EndCounter;
    }

    return 0;
}
