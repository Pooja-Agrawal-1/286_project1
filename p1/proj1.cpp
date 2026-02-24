#include <iostream>
#include <fstream>
#include <cstdint>
#include <map>
#include <iomanip>
#include <cstdint>
#include <bitset>

using namespace std;

//array of register values
int32_t REG[32] = {0};

/*
Create a struct to represent all fields possible in R, I, and J type MIPS instructions.
The constructor takes a 32-bit integer and extracts all fields.
*/
struct MIPSInstruction {
    uint32_t raw;

    // Common
    uint8_t opcode;

    // R-type
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint8_t shamt;
    uint8_t funct;

    // I-type
    int16_t immediate;

    // J-type
    uint32_t address;

    MIPSInstruction(uint32_t instruction) {
        raw = instruction;

        opcode = (instruction >> 26) & 0x3F;
        rs     = (instruction >> 21) & 0x1F;
        rt     = (instruction >> 16) & 0x1F;
        rd     = (instruction >> 11) & 0x1F;
        shamt  = (instruction >> 6)  & 0x1F;
        funct  = instruction & 0x3F;
        immediate = instruction & 0xFFFF;
        address = instruction & 0x03FFFFFF;
    }
};

string regName(uint8_t reg) {
    static const string names[32] = {
        "$zero", "$at",
        "$v0", "$v1",
        "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3",
        "$t4", "$t5", "$t6", "$t7",
        "$s0", "$s1", "$s2", "$s3",
        "$s4", "$s5", "$s6", "$s7",
        "$t8", "$t9",
        "$k0", "$k1",
        "$gp", "$sp", "$fp", "$ra"
    };

    return names[reg];
}

/*
Create a function to print a human-readable MIPS instruction
based on opcode and funct fields.
*/
void printInstruction(uint32_t instruction) {
    MIPSInstruction inst(instruction);

    if (inst.opcode == 0) {  // R-type
        switch (inst.funct) {

            case 32: // add
                cout << "add " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 33: // addu
                cout << "addu " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 8: // jr
                cout << "jr " << regName(inst.rs);
                break;

            case 0: // sll
                cout << "sll " << regName(inst.rd) << ", "
                     << regName(inst.rt) << ", "
                     << (int)inst.shamt;
                break;

            case 42: // slt
                cout << "slt " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 12: // syscall
                cout << "syscall";
                break;

            default:
                cout << "Unknown R-type instruction";
        }
    }

    else {
        switch (inst.opcode) {

            case 8: // addi
                cout << "addi " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 9: // addiu
                cout << "addiu " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 4: // beq
                cout << "beq " << regName(inst.rs) << ", "
                     << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 5: // bne
                cout << "bne " << regName(inst.rs) << ", "
                     << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 2: // j
                cout << "j " << inst.address;
                break;

            case 3: // jal
                cout << "jal " << inst.address;
                break;

            case 15: // lui
                cout << "lui " << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 35: // lw
                cout << "lw " << regName(inst.rt) << ", "
                     << inst.immediate << "("
                     << regName(inst.rs) << ")";
                break;

            case 43: // sw
                cout << "sw " << regName(inst.rt) << ", "
                     << inst.immediate << "("
                     << regName(inst.rs) << ")";
                break;

            case 13: // ori
                cout << "ori " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 28: // mul (SPECIAL2)
                if (inst.funct == 2) {
                    cout << "mul " << regName(inst.rd) << ", "
                         << regName(inst.rs) << ", "
                         << regName(inst.rt);
                } else {
                    cout << "Unknown mul variant";
                }
                break;

            default:
                cout << "Unknown instruction";
        }
    }

    cout << endl;
}

void simulate(uint32_t PC,
              uint32_t dataAddr,
              uint32_t numData,
              map<uint32_t,int32_t>& MEM)
{
    uint32_t cycle = 1;
    bool running = true;

    while (running) {

        uint32_t currentPC = PC;
        uint32_t instruction = (uint32_t)MEM[PC];
        MIPSInstruction inst(instruction);

        cout << "Cycle " << cycle << ":" << endl;

        cout << "Fetched: "
             << currentPC << " : "
             << bitset<32>(instruction) << " : ";
        printInstruction(instruction);

        PC += 4;  // default next instruction

        // ================= EXECUTION =================

        if (inst.opcode == 0) {
            switch (inst.funct) {

                case 32: // add
                case 33: // addu
                    REG[inst.rd] = REG[inst.rs] + REG[inst.rt];
                    break;

                case 42: // slt
                    REG[inst.rd] = (REG[inst.rs] < REG[inst.rt]);
                    break;

                case 0: // sll
                    REG[inst.rd] = REG[inst.rt] << inst.shamt;
                    break;

                case 8: // jr
                    PC = REG[inst.rs];
                    break;

                case 12: // syscall
                    running = false;
                    break;
            }
        }

        else {
            switch (inst.opcode) {

                case 8:  // addi
                case 9:  // addiu
                    REG[inst.rt] = REG[inst.rs] + inst.immediate;
                    break;

                case 4: // beq
                    if (REG[inst.rs] == REG[inst.rt])
                        PC = currentPC + 4 + (inst.immediate << 2);
                    break;

                case 5: // bne
                    if (REG[inst.rs] != REG[inst.rt])
                        PC = currentPC + 4 + (inst.immediate << 2);
                    break;

                case 2: // j
                    PC = (currentPC & 0xF0000000) | (inst.address << 2);
                    break;

                case 3: // jal
                    REG[31] = PC;
                    PC = (currentPC & 0xF0000000) | (inst.address << 2);
                    break;

                case 15: // lui
                    REG[inst.rt] = inst.immediate << 16;
                    break;

                case 35: // lw
                    REG[inst.rt] =
                        MEM[REG[inst.rs] + inst.immediate];
                    break;

                case 43: // sw
                    MEM[REG[inst.rs] + inst.immediate] =
                        REG[inst.rt];
                    break;

                case 13: // ori
                    REG[inst.rt] =
                        REG[inst.rs] | (uint16_t)inst.immediate;
                    break;
            }
        }

        REG[0] = 0;  // $zero always 0

        // ================= PRINT REGISTERS =================

        cout << "Registers:" << endl;

        for (int i = 0; i < 16; i++) {
            cout << setw(10) << regName(i)
                 << ": " << setw(10) << REG[i]
                 << "    "
                 << setw(10) << regName(i+16)
                 << ": " << setw(10) << REG[i+16]
                 << endl;
        }

        // ================= DATA SECTION =================

        cout << "Data Section:" << endl;
        for (uint32_t addr = dataAddr;
             addr < dataAddr + numData*4;
             addr += 4)
        {
            cout << addr << " : "
                 << MEM[addr] << endl;
        }

        // ================= STACK SECTION =================

        cout << "Stack Section:" << endl;

        for (auto &entry : MEM) {
            if (entry.first >= REG[29]) {
                cout << entry.first
                     << " : "
                     << entry.second << endl;
            }
        }

        cout << "--------------------------------------------------"
             << endl;

        cycle++;
    }
}

/*
Main function:
- Reads filename from command line
- Opens file in binary mode
- Reads 32-bit unsigned integers
- Reverses endianness
- Stores values in MEM map
- prints each instruction
- prints each cycle of simulation
*/
int main(int argc, char* argv[]) {

    if (argc < 2) {
        cerr << "Usage: ./proj1 <binaryfile>" << endl;
        return 1;
    }

    ifstream file(argv[1], ios::binary);
    if (!file) {
        cerr << "Error opening file." << endl;
        return 1;
    }

    uint32_t value;

    // First 4 numbers
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    uint32_t PC = __builtin_bswap32(value);

    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    uint32_t dataAddr = __builtin_bswap32(value);

    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    uint32_t SP = __builtin_bswap32(value);

    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    uint32_t numData = __builtin_bswap32(value);

    map<uint32_t, int32_t> MEM;

    uint32_t address;

    // Store data section
    address = dataAddr;
    for (uint32_t i = 0; i < numData; i++) {
        file.read(reinterpret_cast<char*>(&value), sizeof(value));
        value = __builtin_bswap32(value);  // reverse endianness
        MEM[address] = static_cast<int32_t>(value);
        address += 4;
    }

    // Store instructions
    address = PC;
    while (file.read(reinterpret_cast<char*>(&value), sizeof(value))) {
        value = __builtin_bswap32(value);  // reverse endianness
        MEM[address] = static_cast<int32_t>(value);
        address += 4;
    }

    file.close();

// Print stored header values
cout << "initialPC: " << PC << endl;
cout << "dataStartAddress: " << dataAddr << endl;
cout << "initialStackPointer: " << SP << endl;
cout << "Number of data items: " << numData << endl;

// Print DATA values
for (uint32_t addr = dataAddr; addr < dataAddr + numData * 4; addr += 4) {

    uint32_t value = static_cast<uint32_t>(MEM[addr]);

    cout << left
         << setw(12) << addr              // Address column
         << setw(35) << bitset<32>(value) // Binary column
         << value                         // Decimal column
         << endl;
}

// Print INSTRUCTIONS
for (uint32_t addr = PC; addr < PC + (MEM.size() - numData) * 4; addr += 4) {

    if (addr >= dataAddr && addr < dataAddr + numData * 4)
        continue;

    uint32_t instruction = static_cast<uint32_t>(MEM[addr]);

    cout << left
         << setw(12) << addr                       // Address column
         << setw(35) << bitset<32>(instruction);   // Binary column

    printInstruction(instruction);                 // Text column
}

REG[29] = SP;    //hardcoded $sp value
simulate(PC, dataAddr, numData, MEM);
}