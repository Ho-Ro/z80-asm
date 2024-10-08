/* INSTRUCTION TABLE FOR LEXICAL ANALYSIS (DURING ASSEMBLING) */

#include "instr_token"
#include "z80-cpu.h"

/* Be sure table is in alphabetical order in its first attribute !! */
/* we need this feature for binary search in convert() in asm.c */

const struct seznam_type instruction[ N_INSTRUCTIONS ] = {
    { "ADC", I_ADC },   { "ADD", I_ADD },   { "ALIGN", I_ALIGN }, { "AND", I_AND },   { "BIT", I_BIT },   { "CALL", I_CALL },
    { "CCF", I_CCF },   { "COND", I_COND }, { "CP", I_CP },       { "CPD", I_CPD },   { "CPDR", I_CPDR }, { "CPI", I_CPI },
    { "CPIR", I_CPIR }, { "CPL", I_CPL },   { "DAA", I_DAA },     { "DEC", I_DEC },   { "DEFB", I_DEFB }, { "DEFL", I_DEFL },
    { "DEFM", I_DEFM }, { "DEFS", I_DEFS }, { "DEFW", I_DEFW },   { "DI", I_DI },     { "DJNZ", I_DJNZ }, { "EI", I_EI },
    { "END", I_END },   { "ENDC", I_ENDC }, { "EQU", I_EQU },     { "EX", I_EX },     { "EXX", I_EXX },   { "HALT", I_HALT },
    { "IM", I_IM },     { "IN", I_IN },     { "INC", I_INC },     { "IND", I_IND },   { "INDR", I_INDR }, { "INI", I_INI },
    { "INIR", I_INIR }, { "JP", I_JP },     { "JR", I_JR },       { "LD", I_LD },     { "LDD", I_LDD },   { "LDDR", I_LDDR },
    { "LDI", I_LDI },   { "LDIR", I_LDIR }, { "NEG", I_NEG },     { "NOP", I_NOP },   { "OR", I_OR },     { "ORG", I_ORG },
    { "OTDR", I_OTDR }, { "OTIR", I_OTIR }, { "OUT", I_OUT },     { "OUTD", I_OUTD }, { "OUTI", I_OUTI }, { "POP", I_POP },
    { "PUSH", I_PUSH }, { "RES", I_RES },   { "RET", I_RET },     { "RETI", I_RETI }, { "RETN", I_RETN }, { "RL", I_RL },
    { "RLA", I_RLA },   { "RLC", I_RLC },   { "RLCA", I_RLCA },   { "RLD", I_RLD },   { "RR", I_RR },     { "RRA", I_RRA },
    { "RRC", I_RRC },   { "RRCA", I_RRCA }, { "RRD", I_RRD },     { "RST", I_RST },   { "SBC", I_SBC },   { "SCF", I_SCF },
    { "SET", I_SET },   { "SLA", I_SLA },   { "SLL", I_SLL },     { "SRA", I_SRA },   { "SRL", I_SRL },   { "SUB", I_SUB },
    { "XOR", I_XOR } };
