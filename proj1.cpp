//To make this project, we used Codex in Chat mode to help sort out the logic and certain functions

#include <iostream>
#include <fstream>
#include <cstdint>
#include <map>
#include <iomanip>
#include <cstdint>
#include <bitset>
#include <string>

using namespace std;

//array of register values
int32_t REG[32] = {0};

/*
Struct to represent all fields possible in R, I, and J type MIPS instructions.
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

        //this shifts the instruction binary to the right by a certain amount of bits 
        //the hex takes the last n bits and zeros everything else
        opcode = (instruction >> 26) & 0x3F;
        rs     = (instruction >> 21) & 0x1F;
        rt     = (instruction >> 16) & 0x1F;
        rd     = (instruction >> 11) & 0x1F;
        shamt  = (instruction >> 6)  & 0x1F;
        funct  = instruction & 0x3F;
        immediate = static_cast<int16_t>(instruction & 0xFFFF);
        address = instruction & 0x03FFFFFF;
    }
};

//function that returns the register name from a list of registers (stored in ascending order by memory) given the index
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
Function to print a human-readable MIPS instruction
based on opcode and funct fields.
*/
void printInstruction(uint32_t instruction, ofstream& out, uint32_t pcAddr) {
    MIPSInstruction inst(instruction);

    if (inst.opcode == 0) {  // R-type
        switch (inst.funct) {

            case 32: // add
                out << "add " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 33: // addu
                out << "addu " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 8: // jr
                out << "jr " << regName(inst.rs);
                break;

            case 0: // sll
                out << "sll " << regName(inst.rd) << ", "
                     << regName(inst.rt) << ", "
                     << (int)inst.shamt;
                break;

            case 42: // slt
                out << "slt " << regName(inst.rd) << ", "
                     << regName(inst.rs) << ", "
                     << regName(inst.rt);
                break;

            case 12: // syscall
                out << "syscall";
                break;

            default:
                out << "Unknown R-type instruction";
        }
    }

    else {
        switch (inst.opcode) {

            case 8: // addi
                out << "addi " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 9: // addiu
                out << "addiu " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 4: // beq
                out << "beq " << regName(inst.rs) << ", "
                     << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 5: // bne
                out << "bne " << regName(inst.rs) << ", "
                     << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 2: // j
                out << "j " << (((pcAddr + 4) & 0xF0000000) | (inst.address << 2));
                break;

            case 3: // jal
                out << "jal " << (((pcAddr + 4) & 0xF0000000) | (inst.address << 2));
                break;

            case 15: // lui
                out << "lui " << regName(inst.rt) << ", "
                     << inst.immediate;
                break;

            case 35: // lw
                out << "lw " << regName(inst.rt) << ", "
                     << inst.immediate << "("
                     << regName(inst.rs) << ")";
                break;

            case 43: // sw
                out << "sw " << regName(inst.rt) << ", "
                     << inst.immediate << "("
                     << regName(inst.rs) << ")";
                break;

            case 13: // ori
                out << "ori " << regName(inst.rt) << ", "
                     << regName(inst.rs) << ", "
                     << inst.immediate;
                break;

            case 28: // mul (SPECIAL2)
                if (inst.funct == 2) {
                    out << "mul " << regName(inst.rd) << ", "
                         << regName(inst.rs) << ", "
                         << regName(inst.rt);
                } else {
                    out << "Unknown mul variant";
                }
                break;

            default:
                out << "Unknown instruction";
        }
    }

    out << endl;
}

//function that simulates all the instructions by executing their operation based on the function code
void simulate(uint32_t PC, uint32_t dataAddr, uint32_t numData, map<uint32_t,int32_t>& MEM, ofstream& out, uint32_t initSP)
{
    uint32_t cycle = 1;
    bool running = true;

    while (running) {
        uint32_t currentPC = PC;
        uint32_t instruction = (uint32_t)MEM[PC];
        MIPSInstruction inst(instruction);

        out << "Cycle " << cycle << ":" << endl;

        out << "Fetched: "
             << currentPC << ":" << "\t"
             << bitset<32>(instruction) << "\t";
        printInstruction(instruction, out, PC);

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
                    if(REG[2]==10){
                        running = false;
                    }
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
                        PC = currentPC + 4 + (static_cast<int32_t>(inst.immediate) << 2);
                    break;

                case 5: // bne
                    if (REG[inst.rs] != REG[inst.rt])
                        PC = currentPC + 4 + (static_cast<int32_t>(inst.immediate << 2));
                    break;

                case 2: // j
                    PC = ((currentPC+4) & 0xF0000000) | (inst.address << 2);
                    break;

                case 3: // jal
                    REG[31] = PC;
                    PC = ((currentPC+4) & 0xF0000000) | (inst.address << 2);
                    break;

                case 15: // lui
                    REG[inst.rt] = inst.immediate << 16;
                    break;

                case 35: // lw
                    REG[inst.rt] = MEM[REG[inst.rs] + inst.immediate];
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

        out << "Registers:" << endl;

        for (int i = 0; i < 32; i+=4) {
            out << left << setw(16) << (regName(i) + ":")
                 << right << setw(16) << REG[i] << "\t"
                 << left << setw(16) << (regName(i+1) + ":")
                 << right << setw(16) << REG[i+1] << "\t"
                 << left << setw(16) << (regName(i+2) + ":")
                 << right << setw(16) << REG[i+2] << "\t"
                 << left << setw(16) << (regName(i+3) + ":")
                 << right << setw(16) << REG[i+3]
                 << endl;
        }

        // ================= DATA SECTION =================

        out << "Data Section:" << endl;
        for (uint32_t addr = dataAddr;
             addr < dataAddr + numData*4;
             addr += 4)
        {
            out << addr << " : "
                 << MEM[addr] << endl;
        }

        // ================= STACK SECTION =================

        out << "Stack Section:" << endl;

        for (uint32_t addr = REG[29]; addr < initSP; addr +=4) {
            out << addr << ":";
            if (MEM.find(addr) != MEM.end()) {
                out << MEM[addr];
            }
            else{
                out << 0;
            }
            out << endl;
        }

        out << "--------------------------------------------------"
             << endl;

        cycle++;
    }
}

/*
Main function:
- Reads filename from command line
- Opens file in binary mode
- creates/overwrites output file based on name
- Reads 32-bit unsigned integers
- Reverses endianness
- Stores values in MEM map
- prints each instruction to output file
- prints each cycle of simulation to output file
*/
int main(int argc, char* argv[]) {

    if (argc < 3) {
        cerr << "Usage: ./simulate <binaryfile> <output_filename>" << endl;
        return 1;
    }

    ifstream file(argv[1], ios::binary);
    if (!file) {
        cerr << "Error opening file." << endl;
        return 1;
    }

    ofstream outFile(argv[2]);
    if (!outFile){
        cerr << "Error opening output file." << endl;
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
outFile << "initialPC: " << PC << endl;
outFile << "dataStartAddress: " << dataAddr << endl;
outFile << "initialStackPointer: " << SP << endl;
outFile << "Number of data items: " << numData << endl;

// Print DATA values
for (uint32_t addr = dataAddr; addr < dataAddr + numData * 4; addr += 4) {

    uint32_t value = static_cast<uint32_t>(MEM[addr]);

    outFile << left
         << setw(12) << (to_string(addr) + ":")              // Address column
         << setw(35) << bitset<32>(value) // Binary column
         << value                         // Decimal column
         << endl;
}

// Print INSTRUCTIONS
for (uint32_t addr = PC; addr < PC + (MEM.size() - numData) * 4; addr += 4) {

    if (addr >= dataAddr && addr < dataAddr + numData * 4)
        continue;

    uint32_t instruction = static_cast<uint32_t>(MEM[addr]);

    outFile << left
         << setw(12) << (to_string(addr) + ":")                       // Address column
         << setw(35) << bitset<32>(instruction);   // Binary column

    printInstruction(instruction, outFile, PC);                 // Text column
}

REG[29] = SP;    //hardcoded $sp value
uint32_t initialSP = SP; //hardcoded $sp value to keep track of the beginning
simulate(PC, dataAddr, numData, MEM, outFile, initialSP);
}