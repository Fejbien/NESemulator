#include <iostream>
#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

class NESemulator
{
private:
    bool _loggingEnabled = true;
    std::string _filePath;

    ushort _programCounter;
    std::uint8_t _A;
    std::uint8_t _X;
    std::uint8_t _Y;

    std::uint8_t _addressBus;

    std::uint8_t _header[0x10];
    std::uint8_t _ram[0x800];
    std::uint8_t _rom[0x8000];

    bool _flagCarry;
    bool _flagZero;
    bool _flagInterruptDisable;
    bool _flagDecimal;
    bool _flagOverflow;
    bool _flagNegative;

    std::uint8_t _stackPointer = 0xFF;

    bool _cpuHalted = false;
    int _cycleCount = 0;

    std::uint8_t readMemory(ushort address)
    {
        if (address < 0x800)
        {
            return _ram[address % 0x800];
        }
        else if (address >= 0x8000)
        {
            return _rom[address - 0x8000];
        }
        else
        {
            std::cerr << "Invalid memory read at address: " << std::hex << address << std::dec << std::endl;
            return 0;
        }
    }

    void writeMemory(ushort address, std::uint8_t value)
    {
        _ram[address % 0x800] = value;
    }

    void reset()
    {
        _programCounter = 0;
        _A = 0;
        _X = 0;
        _Y = 0;
        _flagCarry = false;
        _flagZero = false;
        _flagInterruptDisable = true; // True on purpose
        _flagDecimal = false;
        _flagOverflow = false;
        _flagNegative = false;

        _stackPointer = 0xFD; // Stack Pointer starts at 0xFD on reset

        std::ifstream romFile(_filePath, std::ios::binary | std::ios::ate);
        if (!romFile)
        {
            std::cerr << "Failed to open ROM file: " << _filePath << std::endl;
            return;
        }

        std::streamsize size = romFile.tellg();
        romFile.seekg(0, std::ios::beg);

        // storage as uint8_t
        std::vector<std::uint8_t> headeredRom(size);

        // cast only at the read boundary
        if (!romFile.read(reinterpret_cast<char *>(headeredRom.data()), size))
        {
            std::cerr << "Failed to read ROM file: " << _filePath << std::endl;
            return;
        }
        romFile.close();

        // Copy header (first 0x10 bytes)
        std::copy(headeredRom.begin(),
                  headeredRom.begin() + 0x10,
                  _header);

        // Copy ROM (next 0x8000 bytes after header)
        std::copy(headeredRom.begin() + 0x10,
                  headeredRom.begin() + 0x10 + 0x8000,
                  _rom);

        // Reset vector (little-endian: low at 0xFFFC, high at 0xFFFD)
        std::uint8_t PCL = readMemory(0xFFFC);
        std::uint8_t PCH = readMemory(0xFFFD);
        _programCounter = static_cast<std::uint16_t>((PCH << 8) | PCL);
    }

    void handleOpcode(std::uint8_t opcode)
    {
        switch (opcode)
        {
        case 0x02: // HLT
        {
            _cpuHalted = true;
        }
        break;
        case 0xA0: // LDY Immediate
        {
            _Y = readMemory(_programCounter);
            _flagZero = (_Y == 0);
            _flagNegative = (_Y > 127);

            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xA2: // LDX Immediate
        {
            _X = readMemory(_programCounter);
            _flagZero = (_X == 0);
            _flagNegative = (_X > 127);

            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xA5: // LDA Zero Page
        {
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;
            _A = readMemory(zp);
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;
            _cycleCount += 3;
        }
        break;
        case 0xA9: // LDA Immediate
        {
            _A = readMemory(_programCounter);
            _flagZero = (_A == 0);
            _flagNegative = (_A > 127);

            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xAD: // LDA Absolute
        {
            std::uint8_t low = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t high = readMemory(_programCounter);
            _programCounter++;
            ushort addr = static_cast<ushort>((high << 8) | low);
            _A = readMemory(addr);
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;
            _cycleCount += 4;
        }
        break;
        case 0x85: // STA Zero Page
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            writeMemory(temp, _A);
            _cycleCount += 3;
        }
        break;
        case 0x8D: // STA Absolute
        {
            std::uint8_t tempLow = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t tempHigh = readMemory(_programCounter);
            _programCounter++;
            writeMemory((tempHigh << 8) | tempLow, _A);
            _cycleCount += 4;
        }
        break;
        case 0x86: // STX Zero Page
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            writeMemory(temp, _X);
            _cycleCount += 3;
        }
        break;
        case 0x8E: // STX Absolute
        {
            std::uint8_t tempLow = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t tempHigh = readMemory(_programCounter);
            _programCounter++;
            writeMemory((tempHigh << 8) | tempLow, _X);
            _cycleCount += 4;
        }
        break;
        case 0x84: // STY Zero Page
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            writeMemory(temp, _Y);
            _cycleCount += 3;
        }
        break;
        case 0x8C: // STY Absolute
        {
            std::uint8_t tempLow = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t tempHigh = readMemory(_programCounter);
            _programCounter++;
            writeMemory((tempHigh << 8) | tempLow, _Y);
            _cycleCount += 4;
        }
        break;
        case 0x10: // BPL Branch on Plus
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2; // base cycles
            if (!_flagNegative)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0x30: // BMI Branch on Minus
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (_flagNegative)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        // case 0x50: // BVC Branch on Overflow Clear
        // {
        // }
        // break;
        // case 0x70: // BVS Branch on Overflow Set
        // {
        // }
        // break;
        // case 0x90: // BCC Branch on Carry Clear
        // {
        // }
        // break;
        // case 0xB0: // BCS Branch on Carry Set
        // {
        // }
        // break;
        case 0xD0: // BNE Branch on Not Equal
        {
            std::uint8_t offset = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (!_flagZero)
            {
                // Sign-extend 8-bit offset
                int8_t rel = static_cast<int8_t>(offset);
                ushort oldPC = _programCounter;
                _programCounter = static_cast<ushort>(_programCounter + rel);
                _cycleCount += 1;
                if ((oldPC & 0xFF00) != (_programCounter & 0xFF00))
                    _cycleCount += 1;
            }
        }
        break;
        case 0xF0: // BEQ Branch on Equal
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (_flagZero)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0x48: // PHA Push Accumulator
        {
            pushStack(_A);
            _cycleCount += 3;
        }
        break;
        case 0x68: // PLA Pull Accumulator
        {
            _A = pullStack();
            _flagZero = (_A == 0);
            _flagNegative = (_A > 127);
            _cycleCount += 4;
        }
        break;
        case 0x20: // JSR Jump to Subroutine
        {
            std::uint8_t low = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t high = readMemory(_programCounter);
            _programCounter++;
            ushort target = static_cast<ushort>((high << 8) | low);

            ushort returnAddr = static_cast<ushort>(_programCounter - 1);
            pushStack(static_cast<std::uint8_t>(returnAddr >> 8));   // high
            pushStack(static_cast<std::uint8_t>(returnAddr & 0xFF)); // low

            _programCounter = target;
            _cycleCount += 6;
        }
        break;
        case 0x60: // RTS Return from Subroutine
        {
            std::uint8_t tempLow = pullStack();
            std::uint8_t tempHigh = pullStack();
            _programCounter = (ushort)((tempHigh << 8) | tempLow);
            _programCounter++;
            _cycleCount += 6;
        }
        break;
        case 0xEA: // NOP No Operation
            _cycleCount += 2;
            break;
        case 0xE8: // INX Increment X
        {
            _X++;
            _flagZero = (_X == 0);
            _flagNegative = (_X > 127);
            _cycleCount += 2;
        }
        break;
        case 0xC8: // INY Increment Y
        {
            _Y++;
            _flagZero = (_Y == 0);
            _flagNegative = (_Y > 127);
            _cycleCount += 2;
        }
        break;
        case 0xCA: // DEX Decrement X
        {
            _X--;
            _flagZero = (_X == 0);
            _flagNegative = (_X > 127);
            _cycleCount += 2;
        }
        break;
        case 0x88: // DEY Decrement Y
        {
            _Y--;
            _flagZero = (_Y == 0);
            _flagNegative = (_Y > 127);
            _cycleCount += 2;
        }
        break;
        case 0xAA: // TAX Transfer Accumulator to X
        {
            _X = _A;
            _flagZero = (_X == 0);
            _flagNegative = (_X > 127);
            _cycleCount += 2;
        }
        break;
        case 0x8A: // TXA Transfer X to Accumulator
        {
            _A = _X;
            _flagZero = (_A == 0);
            _flagNegative = (_A > 127);
            _cycleCount += 2;
        }
        break;
        case 0xA8: // TAY Transfer Accumulator to Y
        {
            _Y = _A;
            _flagZero = (_Y == 0);
            _flagNegative = (_Y > 127);
            _cycleCount += 2;
        }
        break;
        case 0x98: // TYA Transfer Y to Accumulator
        {
            _A = _Y;
            _flagZero = (_A == 0);
            _flagNegative = (_A > 127);
            _cycleCount += 2;
        }
        break;
        case 0x9A: // TXS Transfer X to Stack Pointer
        {
            _stackPointer = _X;
            _cycleCount += 2;
        }
        break;
        case 0xBA: // TSX Transfer Stack Pointer to X
        {
            _X = _stackPointer;
            _flagZero = (_X == 0);
            _flagNegative = (_X > 127);
            _cycleCount += 2;
        }
        break;
        case 0x38: // SEC Set Carry Flag
            _flagCarry = true;
            _cycleCount += 2;
            break;
        case 0x18: // CLC Clear Carry Flag
            _flagCarry = false;
            _cycleCount += 2;
            break;
        case 0xB8: // CLV Clear Overflow Flag
            _flagOverflow = false;
            _cycleCount += 2;
            break;
        case 0x78: // SEI Set Interrupt Disable
            _flagInterruptDisable = true;
            _cycleCount += 2;
            break;
        case 0x58: // CLI Clear Interrupt Disable
            _flagInterruptDisable = false;
            _cycleCount += 2;
            break;
        case 0xF8: // SED Set Decimal Flag
            _flagDecimal = true;
            _cycleCount += 2;
            break;
        case 0xD8: // CLD Clear Decimal Flag
            _flagDecimal = false;
            _cycleCount += 2;
            break;
        case 0x08: // PHP Push Processor Status
        {
            std::uint8_t status =
                (_flagNegative << 7) |
                (_flagOverflow << 6) |
                (1 << 5) |
                (1 << 4) |
                (_flagDecimal << 3) |
                (_flagInterruptDisable << 2) |
                (_flagZero << 1) |
                _flagCarry;

            pushStack(status);
            _cycleCount += 3;
        }
        break;
        case 0x28: // PLP Pull Processor Status
        {
            std::uint8_t status = pullStack();
            _flagNegative = (status & 0x80) != 0;
            _flagOverflow = (status & 0x40) != 0;
            // Bit 5 ignored
            // Bit 4 (Break) ignored internally
            _flagDecimal = (status & 0x08) != 0;
            _flagInterruptDisable = (status & 0x04) != 0;
            _flagZero = (status & 0x02) != 0;
            _flagCarry = (status & 0x01) != 0;
            _cycleCount += 4;
        }
        break;
        case 0x0A: // ASL Accumulator
        {
            _flagCarry = (_A > 127);
            _A <<= 1;
            _flagZero = (_A == 0);
            _flagNegative = (_A > 127);
            _cycleCount += 2;
        }
        break;
        case 0x06: // ASL Zero Page
        {
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t v = readMemory(zp);
            _flagCarry = (v & 0x80) != 0;
            v <<= 1;
            writeMemory(zp, v);
            _flagZero = (v == 0);
            _flagNegative = (v & 0x80) != 0;
            _cycleCount += 5;
        }
        break;
        case 0x0E: // ASL Absolute
        {
            std::uint8_t low = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t high = readMemory(_programCounter);
            _programCounter++;
            ushort addr = static_cast<ushort>((high << 8) | low);
            std::uint8_t v = readMemory(addr);
            _flagCarry = (v & 0x80) != 0;
            v <<= 1;
            writeMemory(addr, v);
            _flagZero = (v == 0);
            _flagNegative = (v & 0x80) != 0;
            _cycleCount += 6;
        }
        break;
        case 0x26: // ROL Zero Page
        {
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t v = readMemory(zp);
            bool oldCarry = _flagCarry;
            _flagCarry = (v & 0x80) != 0;
            v = static_cast<std::uint8_t>((v << 1) | (oldCarry ? 1 : 0));
            writeMemory(zp, v);
            _flagZero = (v == 0);
            _flagNegative = (v & 0x80) != 0;
            _cycleCount += 5;
        }
        break;
        case 0x2E: // ROL Absolute
        {
            readOperandsAbsoluteAddress();
            opROL(_addressBus, readMemory(_addressBus));
            _cycleCount += 6;
        }
        break;
        case 0x4A: // LSR Accumulator
        {
            _flagCarry = (_A & 0x01) != 0;
            _A >>= 1;
            _flagZero = (_A == 0);
            _flagNegative = false;
            _cycleCount += 2;
        }
        break;
        case 0x46: // LSR Zero Page
        {
            std::uint8_t zpAddr = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t value = readMemory(zpAddr);
            _flagCarry = (value & 1) != 0;
            value >>= 1;
            writeMemory(zpAddr, value);
            _flagZero = (value == 0);
            _flagNegative = false; // LSR always clears negative
            _cycleCount += 5;
        }
        break;
        case 0x4E: // LSR Absolute
        {
            readOperandsAbsoluteAddress();
            std::uint8_t value = readMemory(_addressBus);
            _flagCarry = (value & 1) != 0;
            value >>= 1;
            writeMemory(_addressBus, value);
            _cycleCount += 6;
        }
        break;
        case 0x6A: // ROR Accumulator
        {
            bool oldCarry = _flagCarry;
            _flagCarry = (_A & 0x01) != 0;
            _A >>= 1;
            if (oldCarry)
                _A |= 0x80;
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;
            _cycleCount += 2;
        }
        break;
        case 0x66: // ROR Zero Page
        {
            std::uint8_t zpAddr = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t value = readMemory(zpAddr);
            bool futureFlagCarry = (value & 1) != 0;
            value >>= 1;
            if (_flagCarry)
            {
                value |= 0x80;
            }
            writeMemory(zpAddr, value);
            _flagCarry = futureFlagCarry;
            _flagZero = (value == 0);
            _flagNegative = (value > 127);
            _cycleCount += 5;
        }
        break;
        case 0x6E: // ROR Absolute
        {
            readOperandsAbsoluteAddress();
            std::uint8_t value = readMemory(_addressBus);
            bool futureFlagCarry = (value & 1) != 0;
            value >>= 1;
            if (_flagCarry)
            {
                value |= 0x80;
            }
            writeMemory(_addressBus, value);
            _flagCarry = futureFlagCarry;
            _flagZero = (value == 0);
            _flagNegative = (value > 127);
            _cycleCount += 6;
        }
        break;
        case 0x09: // ORA Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opORA(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0x05: // ORA Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opORA(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0x0D: // ORA Absolute
        {
            readOperandsAbsoluteAddress();
            opORA(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0x29: // AND Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opAND(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0x25: // AND Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opAND(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0x2D: // AND Absolute
        {
            readOperandsAbsoluteAddress();
            opAND(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0x49: // EOR Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opEOR(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0x45: // EOR Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opEOR(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0x4D: // EOR Absolute
        {
            readOperandsAbsoluteAddress();
            opEOR(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0xE6: // INC Zero Page
        {
            readOperandsAbsoluteAddress();
            opINC(_addressBus, readMemory(_addressBus));
            _cycleCount += 5;
        }
        break;
        case 0xEE: // INC Absolute
        {
            readOperandsAbsoluteAddress();
            opINC(_addressBus, readMemory(_addressBus));
            _cycleCount += 6;
        }
        break;
        case 0xC6: // DEC Zero Page
        {
            readOperandsAbsoluteAddress();
            opDEC(_addressBus, readMemory(_addressBus));
            _cycleCount += 5;
        }
        break;
        case 0xCE: // DEC Absolute
        {
            readOperandsAbsoluteAddress();
            opDEC(_addressBus, readMemory(_addressBus));
            _cycleCount += 6;
        }
        break;
        case 0x69: // ADC Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opADC(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0x65: // ADC Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opADC(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0x6D: // ADC Absolute
        {
            readOperandsAbsoluteAddress();
            opADC(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0xE9: // SBC Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opSBC(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xE5: // SBC Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opSBC(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0xED: // SBC Absolute
        {
            readOperandsAbsoluteAddress();
            opSBC(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0xC9: // CMP Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            opCMP(value);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xC5: // CMP Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opCMP(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0xCD: // CMP Absolute
        {
            readOperandsAbsoluteAddress();
            opCMP(readMemory(_addressBus));
            _cycleCount += 4;
        }
        break;
        case 0xE0: // CPX Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            _flagCarry = (_X >= value);
            _flagZero = (_X == value);
            _flagNegative = (_X - value > 127);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xE4: // CPX Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            _flagCarry = (_X >= value);
            _flagZero = (_X == value);
            _flagNegative = (_X - value > 127);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0xEC: // CPX Absolute
        {
            readOperandsAbsoluteAddress();
            std::uint8_t value = readMemory(_addressBus);
            _flagCarry = (_X >= value);
            _flagZero = (_X == value);
            _flagNegative = (_X - value > 127);
            _cycleCount += 4;
        }
        break;
        case 0xC0: // CPY Immediate
        {
            std::uint8_t value = readMemory(_programCounter);
            _flagCarry = (_Y >= value);
            _flagZero = (_Y == value);
            _flagNegative = (_Y - value > 127);
            _programCounter++;
            _cycleCount += 2;
        }
        break;
        case 0xC4: // CPY Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            _flagCarry = (_Y >= value);
            _flagZero = (_Y == value);
            _flagNegative = (_Y - value > 127);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0xCC: // CPY Absolute
        {
            readOperandsAbsoluteAddress();
            std::uint8_t value = readMemory(_addressBus);
            _flagCarry = (_Y >= value);
            _flagZero = (_Y == value);
            _flagNegative = (_Y - value > 127);
            _cycleCount += 4;
        }
        break;
        case 0x24: // BIT Zero Page
        {
            std::uint8_t address = readMemory(_programCounter);
            std::uint8_t value = readMemory(address);
            opBIT(value);
            _programCounter++;
            _cycleCount += 3;
        }
        break;
        case 0x2C: // BIT Absolute
        {
            readOperandsAbsoluteAddress();
            std::uint8_t value = readMemory(_addressBus);
            opBIT(value);
            _cycleCount += 4;
        }
        break;
        case 0x00: // BRK Force Interrupt
        {
            _programCounter++;

            std::uint16_t returnPC = _programCounter;

            pushStack(static_cast<std::uint8_t>(returnPC >> 8));
            pushStack(static_cast<std::uint8_t>(returnPC & 0xFF));

            std::uint8_t status =
                (_flagNegative << 7) |
                (_flagOverflow << 6) |
                (1 << 5) | // unused, always 1
                (1 << 4) | // Break flag set
                (_flagDecimal << 3) |
                (_flagInterruptDisable << 2) |
                (_flagZero << 1) |
                _flagCarry;

            pushStack(status);

            // Set Interrupt Disable
            _flagInterruptDisable = true;

            std::uint8_t pcl = readMemory(0xFFFE);
            std::uint8_t pch = readMemory(0xFFFF);
            _programCounter = static_cast<ushort>((pch << 8) | pcl);

            _cycleCount += 7;
        }
        break;
        case 0x40: // RTI Return from Interrupt
        {
            std::uint8_t status = pullStack();
            _flagNegative = (status & 0x80) != 0;
            _flagOverflow = (status & 0x40) != 0;
            // Bit 5 is ignored
            // Bit 4 is ignored
            _flagDecimal = (status & 0x08) != 0;
            _flagInterruptDisable = (status & 0x04) != 0;
            _flagZero = (status & 0x02) != 0;
            _flagCarry = (status & 0x01) != 0;

            std::uint8_t tempLow = pullStack();
            std::uint8_t tempHigh = pullStack();
            _programCounter = (ushort)((tempHigh << 8) | tempLow);
            _cycleCount += 6;
        }
        break;
        case 0x4C: // JMP Absolute
        {
            std::uint8_t tempLow = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t tempHigh = readMemory(_programCounter);
            _programCounter++;
            _programCounter = (ushort)((tempHigh << 8) | tempLow);
            _cycleCount += 3;
        }
        break;
        case 0x70: // BVS Branch on Overflow Set
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (_flagOverflow)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0xB0: // BCS Branch on Carry Set
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (_flagCarry)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0x90: // BCC Branch on Carry Clear
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (!_flagCarry)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0x50: // BVC Branch on Overflow Clear
        {
            std::uint8_t temp = readMemory(_programCounter);
            _programCounter++;
            _cycleCount += 2;
            if (!_flagOverflow)
            {
                int signedVal = temp;
                if (temp > 127)
                {
                    signedVal = temp - 256;
                }
                _programCounter = (ushort)(_programCounter + signedVal);
                _cycleCount += 3; // Branch taken
            }
            else
            {
                _cycleCount += 2; // Branch not taken
            }
        }
        break;
        case 0xB9: // LDA Absolute,Y
        {
            std::uint8_t low = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t high = readMemory(_programCounter);
            _programCounter++;

            ushort baseAddr = static_cast<ushort>((high << 8) | low);
            ushort addr = static_cast<ushort>(baseAddr + _Y);

            _A = readMemory(addr);
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;

            _cycleCount += 4;
            if ((baseAddr & 0xFF00) != (addr & 0xFF00))
                _cycleCount += 1; // page boundary penalty
        }
        break;
        case 0xBD: // LDA Absolute,X
        {
            std::uint8_t low = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t high = readMemory(_programCounter);
            _programCounter++;

            ushort baseAddr = static_cast<ushort>((high << 8) | low);
            ushort addr = static_cast<ushort>(baseAddr + _X);

            _A = readMemory(addr);
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;

            _cycleCount += 4;
            if ((baseAddr & 0xFF00) != (addr & 0xFF00))
                _cycleCount += 1; // page boundary penalty
        }
        break;
        case 0x6C: // JMP Indirect
        {
            std::uint8_t ptrLow = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t ptrHigh = readMemory(_programCounter);
            _programCounter++;
            std::uint16_t ptr = (ptrHigh << 8) | ptrLow;

            std::uint8_t targetLow = readMemory(ptr);
            std::uint8_t targetHigh;
            if ((ptr & 0x00FF) == 0x00FF)
            {
                targetHigh = readMemory(ptr & 0xFF00);
            }
            else
            {
                targetHigh = readMemory(ptr + 1);
            }
            _programCounter = (ushort)((targetHigh << 8) | targetLow);
            _cycleCount += 5;
        }
        break;
        case 0x95: // STA Zero Page,X
        {
            std::uint8_t zpAddr = readMemory(_programCounter);
            _programCounter++;
            writeMemory((zpAddr + _X) & 0x00FF, _A);
            _cycleCount += 4;
        }
        break;
        case 0xb5: // LDA Zero Page,X
        {
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t addr = static_cast<std::uint8_t>(zp + _X);
            _A = readMemory(addr);
            _flagZero = (_A == 0);
            _flagNegative = (_A & 0x80) != 0;
            _cycleCount += 4;
        }
        break;
        case 0x75: // ADC Zero Page,X
        {
            std::uint8_t zpAddr = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t value = readMemory((zpAddr + _X) & 0x00FF);
            opADC(value);
            _cycleCount += 4;
        }
        break;
        case 0x91: // STA Zero Page,Y
        {
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;
            std::uint8_t ptrLow = readMemory(zp);
            std::uint8_t ptrHigh = readMemory(static_cast<std::uint8_t>(zp + 1));
            ushort base = static_cast<ushort>((ptrHigh << 8) | ptrLow);
            ushort effective = static_cast<ushort>(base + _Y);
            writeMemory(effective, _A);
            _cycleCount += 6;
        }
        break;
        case 0x81: // STA (Indirect,X)
        {
            // Operand is zero-page base pointer
            std::uint8_t zp = readMemory(_programCounter);
            _programCounter++;

            // Indexed zero-page pointer (wraps in zero page)
            std::uint8_t ptrLow = readMemory(static_cast<std::uint8_t>(zp + _X));
            std::uint8_t ptrHigh = readMemory(static_cast<std::uint8_t>(zp + _X + 1));

            ushort effective = static_cast<ushort>((ptrHigh << 8) | ptrLow);
            writeMemory(effective, _A);

            _cycleCount += 6;
        }
        break;
        default:
            std::cerr << "Unknown opcode: " << std::hex << static_cast<int>(opcode) << std::dec << std::endl;
            _cpuHalted = true; // Halt on unknown opcode for safety
            break;
        }
    }

    void traceLog(std::uint8_t opcode)
    {
        if (!_loggingEnabled)
            return;
        std::cout << "PC: " << std::hex << _programCounter
                  << " Opcode: " << static_cast<int>(opcode)
                  << " A: " << static_cast<int>(_A)
                  << " X: " << static_cast<int>(_X)
                  << " Y: " << static_cast<int>(_Y)
                  << " SP: " << static_cast<int>(_stackPointer)
                  << " Flags: "
                  << (_flagNegative ? 'N' : 'n')
                  << (_flagOverflow ? 'V' : 'v')
                  << (_flagDecimal ? 'D' : 'd')
                  << (_flagInterruptDisable ? 'I' : 'i')
                  << (_flagZero ? 'Z' : 'z')
                  << (_flagCarry ? 'C' : 'c')
                  << std::dec
                  << " Cycles: " << _cycleCount
                  << std::endl;
    }

    void readOperandsAbsoluteAddress()
    {
        std::uint8_t low = readMemory(_programCounter);
        _programCounter++;
        std::uint8_t high = readMemory(_programCounter);
        _programCounter++;
        _addressBus = static_cast<ushort>((high << 8) | low);
    }

    void readOperandsIndirectAddressedXIndexed()
    {
        std::uint8_t zpAddr = readMemory(_programCounter);
        _programCounter++;
        std::uint8_t effectiveAddrLow = readMemory((ushort)((zpAddr + _X) & 0x00FF));
        std::uint8_t effectiveAddrHigh = readMemory((ushort)((zpAddr + _X + 1) & 0x00FF));
        _addressBus = (ushort)((effectiveAddrHigh << 8) | effectiveAddrLow);
    }

    void opASL(ushort address, std::uint8_t input)
    {
        _flagCarry = (input > 127);
        input <<= 1;
        writeMemory(address, input);
        _flagNegative = (input > 127);
        _flagZero = input == 0;
    }

    void opROL(ushort address, std::uint8_t input)
    {
        bool futureFlagCarry = input > 127;
        input <<= 1;
        if (_flagCarry)
        {
            input |= 1;
        }

        writeMemory(address, input);
        _flagCarry = futureFlagCarry;
        _flagNegative = input > 127;
        _flagZero = input == 0;
    }

    void opINC(ushort address, std::uint8_t input)
    {
        input++;
        writeMemory(address, input);
        _flagNegative = (input > 127);
        _flagZero = (input == 0);
    }

    void opDEC(ushort address, std::uint8_t input)
    {
        input--;
        writeMemory(address, input);
        _flagNegative = (input > 127);
        _flagZero = (input == 0);
    }

    void opORA(std::uint8_t input)
    {
        _A |= input;
        _flagNegative = (_A > 127);
        _flagZero = (_A == 0);
    }

    void opAND(std::uint8_t input)
    {
        _A &= input;
        _flagNegative = (_A > 127);
        _flagZero = (_A == 0);
    }

    void opEOR(std::uint8_t input)
    {
        _A ^= input;
        _flagNegative = (_A > 127);
        _flagZero = (_A == 0);
    }

    void opADC(std::uint8_t input)
    {
        int sum = _A + input + (_flagCarry ? 1 : 0);
        _flagCarry = (sum > 0xFF);
        _flagOverflow = (~(_A ^ input) & (_A ^ sum) & 0x80) != 0;
        _A = static_cast<std::uint8_t>(sum);
        _flagNegative = (_A > 127);
        _flagZero = (_A == 0);
    }

    void opSBC(std::uint8_t input)
    {
        uint16_t intermediate = (uint16_t)_A + ((uint16_t)(~input) & 0xFF) + (_flagCarry ? 1 : 0);
        uint8_t result = (uint8_t)(intermediate & 0xFF);

        _flagCarry = (intermediate & 0x100) != 0;
        _flagOverflow = (((_A ^ result) & (_A ^ input) & 0x80) != 0);

        _A = result;
        _flagNegative = (_A & 0x80) != 0;
        _flagZero = (_A == 0);
    }

    void opCMP(std::uint8_t input)
    {
        std::uint8_t diff = static_cast<std::uint8_t>(_A - input);
        _flagCarry = (_A >= input);
        _flagZero = (_A == input);
        _flagNegative = (diff & 0x80) != 0;
    }

    void opBIT(std::uint8_t input)
    {
        _flagZero = (_A & input) == 0;
        _flagNegative = (input & 0x80) != 0;
        _flagOverflow = (input & 0x40) != 0;
    }

    void pushStack(std::uint8_t value)
    {
        writeMemory((ushort)(0x0100 + _stackPointer), value);
        _stackPointer--;
    }

    std::uint8_t pullStack()
    {
        _stackPointer++;
        return readMemory((ushort)(0x0100 + _stackPointer));
    }

    void emulateCPU()
    {
        std::uint8_t opcode = readMemory(_programCounter);
        _programCounter++;
        handleOpcode(opcode);
    }

public:
    NESemulator(std::string filePath) : _filePath(filePath), _programCounter(0), _A(0), _X(0), _Y(0)
    {
        std::fill(std::begin(_ram), std::end(_ram), 0);
        std::fill(std::begin(_rom), std::end(_rom), 0);
    }

    void init()
    {
        reset();
    }

    void run()
    {
        std::cout << "Starting Emulator..." << std::endl;

        int i = 0;
        while (!_cpuHalted && i < 1000)
        {
            traceLog(readMemory(_programCounter));
            emulateCPU();
            i++;
        }
    }
};

int main()
{
    NESemulator emulator("5_Instructions1.nes");
    emulator.init();
    emulator.run();

    return 0;
}