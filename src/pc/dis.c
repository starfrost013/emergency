
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dis.h"
#include <emu.h>

static char buf[128];
#define IPOS (buf + 17)
#define EPOS (buf + 127)

static const char *byte_reg[] = {"AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};
static const char *word_reg[] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};
static const char *seg_reg[] = {"ES", "CS", "SS", "DS"};
static const char *index_reg[] = {"BX+SI", "BX+DI", "BP+SI", "BP+DI",
                                  "SI",    "DI",    "BP",    "BX"};
static const char *table_dx[] = {"ROL", "ROR", "RCL", "RCR", "SHL", "SHR", "SHL", "SAR"};
static const char *table_f6[] = {"TEST", "ILL",  "NOT", "NEG",
                                 "MUL",  "IMUL", "DIV", "IDIV"};
static const char *table_fe[] = {"INC", "DEC", "ILL", "ILL", "ILL", "ILL", "ILL", "ILL"};
static const char *table_ff[] = {"INC", "DEC", "CALL", "CALL",
                                 "JMP", "JMP", "PUSH", "ILL"};
static const char *table_8x[] = {"ADD", "OR", "ADC", "SBB", "AND", "SUB", "XOR", "CMP"};

#define BREG byte_reg[(ModRM & 0x38) >> 3]
#define WREG word_reg[(ModRM & 0x38) >> 3]
#define SREG seg_reg[(ModRM & 0x18) >> 3]
#define IXREG index_reg[ModRM & 0x07]

static void disasm_fillbytes(const uint8_t *ip, int num)
{
    for(int i = 0; i < num; i++)
        sprintf(buf + 2 * i, "%02X", ip[i]);
    memset(buf + 2 * num, ' ', IPOS - buf - 2 * num);
    memset(IPOS, 0, EPOS - IPOS);
}

static const char *seg_names[] = {"ES:", "CS:", "SS:", "DS:", ""};

static char *disasm_get_mem(unsigned ModRM, const uint8_t *ip, const char *rg[],
                     const char *cast, int seg_over)
{
    static char buffer[100];
    unsigned num;
    char ch;
    switch(ModRM & 0xc0)
    {
    case 0x00:
        if((ModRM & 0x07) != 6)
            sprintf(buffer, "%s%s[%s]", cast, seg_names[seg_over], IXREG);
        else
            sprintf(buffer, "%s%s[%02X%02X]", cast, seg_names[seg_over], ip[2], ip[1]);
        break;
    case 0x40:
        if((num = ip[1]) > 127)
        {
            ch = '-';
            num = 256 - num;
        }
        else
            ch = '+';
        sprintf(buffer, "%s%s[%s%c%02X]", cast, seg_names[seg_over], IXREG, ch, num);
        break;
    case 0x80:
        if((num = (ip[2] * 256 + ip[1])) > 0x7fff)
        {
            ch = '-';
            num = 0x10000 - num;
        }
        else
            ch = '+';
        sprintf(buffer, "%s%s[%s%c%04X]", cast, seg_names[seg_over], IXREG, ch, num);
        break;
    case 0xc0:
        strcpy(buffer, rg[ModRM & 7]);
        break;
    }
    return buffer;
}

static int disasm_get_mem_len(unsigned ModRM)
{
    switch(ModRM & 0xc0)
    {
    case 0x00:
        return ((ModRM & 0x07) == 6) ? 2 : 0;
    case 0x40:
        return 1;
    case 0x80:
        return 2;
    default:
        return 0;
    }
}

static const char *disasm_decode_pushpopseg(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 1);
    sprintf(IPOS, "%-7s %s", ins, seg_reg[(*ip & 0x38) >> 3]);
    return buf;
}

static const char *disasm_decode_wordregx(const uint8_t *ip, const char *ins, const char *pre)
{
    disasm_fillbytes(ip, 1);
    sprintf(IPOS, "%-7s %s%s", ins, pre, word_reg[*ip & 7]);
    return buf;
}

static const char *disasm_decode_wordreg(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 1);
    sprintf(IPOS, "%-7s %s", ins, word_reg[*ip & 7]);
    return buf;
}

static const char *disasm_decode_jump(const uint8_t *ip, const char *ins, uint16_t reg_ip)
{
    reg_ip += 3 + ip[2] * 256 + ip[1];
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s %04X", ins, reg_ip);
    return buf;
}

static const char *disasm_decode_jump8(const uint8_t *ip, const char *ins, uint16_t reg_ip)
{
    if(ip[1] < 0x80)
        reg_ip += 2 + ip[1];
    else
        reg_ip += 2 + ip[1] + 0xFF00;
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s %04X", ins, reg_ip);
    return buf;
}

static const char *disasm_decode_far(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 5);
    sprintf(IPOS, "%-7s %04X:%04X", ins, ip[4] * 256U + ip[3], ip[2] * 256U + ip[1]);
    return buf;
}

static const char *disasm_decode_far_ind(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s FAR %s", ins, disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over));
    return buf;
}

static const char *disasm_decode_memal(const uint8_t *ip, const char *ins, int seg_over)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s %s[%02X%02X],AL", ins, seg_names[seg_over], ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_memax(const uint8_t *ip, const char *ins, int seg_over)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s %s[%02X%02X],AX", ins, seg_names[seg_over], ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_almem(const uint8_t *ip, const char *ins, int seg_over)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s AL,%s[%02X%02X]", ins, seg_names[seg_over], ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_axmem(const uint8_t *ip, const char *ins, int seg_over)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s AX,%s[%02X%02X]", ins, seg_names[seg_over], ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_rd8(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s %s,%02X", ins, byte_reg[*ip & 0x7], ip[1]);
    return buf;
}

static const char *disasm_decode_ald8(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s AL,%02X", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_d8al(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s %02X,AL", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_axd8(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s AX,%02X", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_d8ax(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s %02X,AX", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_enter(const uint8_t *ip)
{
    disasm_fillbytes(ip, 4);
    sprintf(IPOS, "ENTER   %02X%02X,%02X", ip[2], ip[1], ip[3]);
    return buf;
}

static const char *disasm_decode_databyte(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 1);
    sprintf(IPOS, "%-7s %02X", ins, ip[0]);
    return buf;
}

static const char *disasm_decode_adjust(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    if(ip[1] == 10)
        sprintf(IPOS, "%-7s", ins);
    else
        sprintf(IPOS, "%-7s %02X", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_d8(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 2);
    sprintf(IPOS, "%-7s %02X", ins, ip[1]);
    return buf;
}

static const char *disasm_decode_d16(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s %02X%02X", ins, ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_axd16(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s AX,%02X%02X", ins, ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_rd16(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 3);
    sprintf(IPOS, "%-7s %s,%02X%02X", ins, word_reg[*ip & 0x7], ip[2], ip[1]);
    return buf;
}

static const char *disasm_decode_br8(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, disasm_get_mem(ModRM, ip + 1, byte_reg, "", seg_over),
            BREG);
    return buf;
}

static const char *disasm_decode_r8b(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, BREG,
            disasm_get_mem(ModRM, ip + 1, byte_reg, "", seg_over));
    return buf;
}

static const char *disasm_decode_bd8(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    int ln = disasm_get_mem_len(ModRM);
    disasm_fillbytes(ip, 3 + ln);
    sprintf(IPOS, "%-7s %s,%02X", ins,
            disasm_get_mem(ModRM, ip + 1, byte_reg, "BYTE PTR ", seg_over), ip[ln + 2]);
    return buf;
}

static const char *disasm_decode_b(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s", ins,
            disasm_get_mem(ModRM, ip + 1, byte_reg, "BYTE PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_ws(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over),
            SREG);
    return buf;
}

static const char *disasm_decode_sw(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, SREG,
            disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over));
    return buf;
}

static const char *disasm_decode_w(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s", ins,
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_wr16(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over),
            WREG);
    return buf;
}

static const char *disasm_decode_r16w(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s", ins, WREG,
            disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over));
    return buf;
}

static const char *disasm_decode_wd16(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    int ln = disasm_get_mem_len(ModRM);
    disasm_fillbytes(ip, 4 + ln);
    sprintf(IPOS, "%-7s %s,%02X%02X", ins,
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over), ip[ln + 3],
            ip[ln + 2]);
    return buf;
}

static const char *disasm_decode_wd8(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    int ln = disasm_get_mem_len(ModRM);
    disasm_fillbytes(ip, 3 + ln);
    sprintf(IPOS, "%-7s %s,%02X", ins,
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over), ip[ln + 2]);
    return buf;
}

static const char *disasm_decode_imul_b(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    uint8_t d1 = ip[2 + disasm_get_mem_len(ModRM)];
    disasm_fillbytes(ip, 3 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s,%c%02X", ins, WREG,
            disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over), d1 > 0x7F ? '-' : '+',
            d1 > 0x7F ? 0x100U - d1 : d1);
    return buf;
}

static const char *disasm_decode_imul_w(const uint8_t *ip, const char *ins, int seg_over)
{
    unsigned ModRM = ip[1];
    uint8_t d1 = ip[2 + disasm_get_mem_len(ModRM)];
    uint8_t d2 = ip[3 + disasm_get_mem_len(ModRM)];
    disasm_fillbytes(ip, 4 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,%s,%02X%02X", ins, WREG,
            disasm_get_mem(ModRM, ip + 1, word_reg, "", seg_over), d2, d1);
    return buf;
}

static const char *disasm_decode_bbitd8(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    int ln = disasm_get_mem_len(ModRM);
    disasm_fillbytes(ip, 3 + ln);
    sprintf(IPOS, "%-7s %s,%2x", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, byte_reg, "BYTE PTR ", seg_over), ip[ln + 2]);
    return buf;
}

static const char *disasm_decode_wbitd8(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    int ln = disasm_get_mem_len(ModRM);
    disasm_fillbytes(ip, 3 + ln);
    sprintf(IPOS, "%-7s %s,%02x", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over), ip[ln + 2]);
    return buf;
}

static const char *disasm_decode_bbit1(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,1", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, byte_reg, "BYTE PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_wbit1(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,1", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_bbitcl(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,CL", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, byte_reg, "BYTE PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_wbitcl(const uint8_t *ip, int seg_over)
{
    unsigned ModRM = ip[1];
    disasm_fillbytes(ip, 2 + disasm_get_mem_len(ModRM));
    sprintf(IPOS, "%-7s %s,CL", table_dx[(ModRM & 0x38) >> 3],
            disasm_get_mem(ModRM, ip + 1, word_reg, "WORD PTR ", seg_over));
    return buf;
}

static const char *disasm_decode_f6(const uint8_t *ip, int seg_over)
{
    int m = (ip[1] & 0x38) >> 3;
    if(m != 0)
        return disasm_decode_b(ip, table_f6[m], seg_over);
    else
        return disasm_decode_bd8(ip, table_f6[m], seg_over);
}

static const char *disasm_decode_f7(const uint8_t *ip, int seg_over)
{
    int m = (ip[1] & 0x38) >> 3;
    if(m != 0)
        return disasm_decode_w(ip, table_f6[m], seg_over);
    else
        return disasm_decode_wd16(ip, table_f6[m], seg_over);
}

static const char *disasm_decode_ff(const uint8_t *ip, int seg_over)
{
    int m = (ip[1] & 0x38) >> 3;
    if(m == 3 || m == 5)
        return disasm_decode_far_ind(ip, table_ff[m], seg_over);
    else
        return disasm_decode_w(ip, table_ff[m], seg_over);
}

static const char *disasm_show_io(const uint8_t *ip, const char *ins, const char *regs)
{
    disasm_fillbytes(ip, 1);
    strcpy(IPOS, ins);
    strcpy(IPOS + 8, regs);
    return buf;
}

static const char *disasm_show(const uint8_t *ip, const char *ins)
{
    disasm_fillbytes(ip, 1);
    strcpy(IPOS, ins);
    return buf;
}

static const char *disasm_show_str(const uint8_t *ip, const char *ins, int segment_override)
{
    disasm_fillbytes(ip, 1);
    int ln = strlen(seg_names[segment_override]);
    strcpy(IPOS, seg_names[segment_override]);
    strcpy(IPOS + ln, ins);
    return buf;
}

static const char *disasm_show_seg(const uint8_t *ip, uint8_t reg_ip, int seg_over)
{
    const char hx[] = "0123456789ABCDEF";
    // Call recursive
    disa(ip + 1, reg_ip + 1, seg_over);

    // Patch bytes!
    memmove(buf + 2, buf, IPOS - buf - 2);
    buf[0] = hx[*ip >> 4];
    buf[1] = hx[*ip & 0xF];

    // Patch ins if does not already has segment name:
    const char *sn = seg_names[seg_over];
    if(strstr(IPOS, sn) == 0)
    {
        unsigned s = strlen(sn) + 1;
        memmove(IPOS + s, IPOS, EPOS - IPOS - s);
        memcpy(IPOS, sn, s);
        IPOS[s - 1] = ' ';
    }
    return buf;
}

static const char *disasm_show_rep(const uint8_t *ip, const char *ins, uint16_t reg_ip,
                            int seg_over)
{
    // Call recursive (only if not REP*)
    if(ip[1] == 0xF2 || ip[1] == 0xF3)
        return disasm_show(ip, ins);

    disa(ip + 1, reg_ip + 1, seg_over);

    // Patch bytes!
    memmove(buf + 2, buf, IPOS - buf - 2);
    buf[0] = 'F';
    buf[1] = '0' + (*ip & 0x0F);

    // Patch ins!
    unsigned s = strlen(ins) + 1;
    memmove(IPOS + s, IPOS, EPOS - IPOS - s);
    memcpy(IPOS, ins, s);
    IPOS[s - 1] = ' ';
    return buf;
}

static const char *disasm_show_int(const uint8_t num)
{
    memset(buf, '?', 2);
    memset(buf + 2, ' ', IPOS - buf - 2);
    memset(IPOS, 0, EPOS - IPOS);
    sprintf(IPOS, "IRET    (EMU %02X)", num);
    return buf;
}

enum segments
{
    ES = 0,
    CS,
    SS,
    DS,
    NoSeg
};

// Show ins disassembly
const char *disa(const uint8_t *ip, uint16_t reg_ip, int segment_override)
{
    if(cpuGetCS() == 0 && (ip - memory) < 0x100)
        return disasm_show_int(ip - memory);
    switch(*ip)
    {
    case 0x00: return disasm_decode_br8(ip, "ADD", segment_override);
    case 0x01: return disasm_decode_wr16(ip, "ADD", segment_override);
    case 0x02: return disasm_decode_r8b(ip, "ADD", segment_override);
    case 0x03: return disasm_decode_r16w(ip, "ADD", segment_override);
    case 0x04: return disasm_decode_ald8(ip, "ADD");
    case 0x05: return disasm_decode_axd16(ip, "ADD");
    case 0x06: return disasm_decode_pushpopseg(ip, "PUSH");
    case 0x07: return disasm_decode_pushpopseg(ip, "POP");
    case 0x08: return disasm_decode_br8(ip, "OR", segment_override);
    case 0x09: return disasm_decode_wr16(ip, "OR", segment_override);
    case 0x0a: return disasm_decode_r8b(ip, "OR", segment_override);
    case 0x0b: return disasm_decode_r16w(ip, "OR", segment_override);
    case 0x0c: return disasm_decode_ald8(ip, "OR");
    case 0x0d: return disasm_decode_axd16(ip, "OR");
    case 0x0e: return disasm_decode_pushpopseg(ip, "PUSH");
    case 0x0f: return disasm_decode_databyte(ip, "DB");
    case 0x10: return disasm_decode_br8(ip, "ADC", segment_override);
    case 0x11: return disasm_decode_wr16(ip, "ADC", segment_override);
    case 0x12: return disasm_decode_r8b(ip, "ADC", segment_override);
    case 0x13: return disasm_decode_r16w(ip, "ADC", segment_override);
    case 0x14: return disasm_decode_ald8(ip, "ADC");
    case 0x15: return disasm_decode_axd16(ip, "ADC");
    case 0x16: return disasm_decode_pushpopseg(ip, "PUSH");
    case 0x17: return disasm_decode_pushpopseg(ip, "POP");
    case 0x18: return disasm_decode_br8(ip, "SBB", segment_override);
    case 0x19: return disasm_decode_wr16(ip, "SBB", segment_override);
    case 0x1a: return disasm_decode_r8b(ip, "SBB", segment_override);
    case 0x1b: return disasm_decode_r16w(ip, "SBB", segment_override);
    case 0x1c: return disasm_decode_ald8(ip, "SBB");
    case 0x1d: return disasm_decode_axd16(ip, "SBB");
    case 0x1e: return disasm_decode_pushpopseg(ip, "PUSH");
    case 0x1f: return disasm_decode_pushpopseg(ip, "POP");
    case 0x20: return disasm_decode_br8(ip, "AND", segment_override);
    case 0x21: return disasm_decode_wr16(ip, "AND", segment_override);
    case 0x22: return disasm_decode_r8b(ip, "AND", segment_override);
    case 0x23: return disasm_decode_r16w(ip, "AND", segment_override);
    case 0x24: return disasm_decode_ald8(ip, "AND");
    case 0x25: return disasm_decode_axd16(ip, "AND");
    case 0x26: return disasm_show_seg(ip, reg_ip, ES);
    case 0x27: return disasm_show(ip, "DAA");
    case 0x28: return disasm_decode_br8(ip, "SUB", segment_override);
    case 0x29: return disasm_decode_wr16(ip, "SUB", segment_override);
    case 0x2a: return disasm_decode_r8b(ip, "SUB", segment_override);
    case 0x2b: return disasm_decode_r16w(ip, "SUB", segment_override);
    case 0x2c: return disasm_decode_ald8(ip, "SUB");
    case 0x2d: return disasm_decode_axd16(ip, "SUB");
    case 0x2e: return disasm_show_seg(ip, reg_ip, CS);
    case 0x2f: return disasm_show(ip, "DAS");
    case 0x30: return disasm_decode_br8(ip, "XOR", segment_override);
    case 0x31: return disasm_decode_wr16(ip, "XOR", segment_override);
    case 0x32: return disasm_decode_r8b(ip, "XOR", segment_override);
    case 0x33: return disasm_decode_r16w(ip, "XOR", segment_override);
    case 0x34: return disasm_decode_ald8(ip, "XOR");
    case 0x35: return disasm_decode_axd16(ip, "XOR");
    case 0x36: return disasm_show_seg(ip, reg_ip, SS);
    case 0x37: return disasm_show(ip, "AAA");
    case 0x38: return disasm_decode_br8(ip, "CMP", segment_override);
    case 0x39: return disasm_decode_wr16(ip, "CMP", segment_override);
    case 0x3a: return disasm_decode_r8b(ip, "CMP", segment_override);
    case 0x3b: return disasm_decode_r16w(ip, "CMP", segment_override);
    case 0x3c: return disasm_decode_ald8(ip, "CMP");
    case 0x3d: return disasm_decode_axd16(ip, "CMP");
    case 0x3e: return disasm_show_seg(ip, reg_ip, DS);
    case 0x3f: return disasm_show(ip, "AAS");
    case 0x40: return disasm_decode_wordreg(ip, "INC");
    case 0x41: return disasm_decode_wordreg(ip, "INC");
    case 0x42: return disasm_decode_wordreg(ip, "INC");
    case 0x43: return disasm_decode_wordreg(ip, "INC");
    case 0x44: return disasm_decode_wordreg(ip, "INC");
    case 0x45: return disasm_decode_wordreg(ip, "INC");
    case 0x46: return disasm_decode_wordreg(ip, "INC");
    case 0x47: return disasm_decode_wordreg(ip, "INC");
    case 0x48: return disasm_decode_wordreg(ip, "DEC");
    case 0x49: return disasm_decode_wordreg(ip, "DEC");
    case 0x4a: return disasm_decode_wordreg(ip, "DEC");
    case 0x4b: return disasm_decode_wordreg(ip, "DEC");
    case 0x4c: return disasm_decode_wordreg(ip, "DEC");
    case 0x4d: return disasm_decode_wordreg(ip, "DEC");
    case 0x4e: return disasm_decode_wordreg(ip, "DEC");
    case 0x4f: return disasm_decode_wordreg(ip, "DEC");
    case 0x50: return disasm_decode_wordreg(ip, "PUSH");
    case 0x51: return disasm_decode_wordreg(ip, "PUSH");
    case 0x52: return disasm_decode_wordreg(ip, "PUSH");
    case 0x53: return disasm_decode_wordreg(ip, "PUSH");
    case 0x54: return disasm_decode_wordreg(ip, "PUSH");
    case 0x55: return disasm_decode_wordreg(ip, "PUSH");
    case 0x56: return disasm_decode_wordreg(ip, "PUSH");
    case 0x57: return disasm_decode_wordreg(ip, "PUSH");
    case 0x58: return disasm_decode_wordreg(ip, "POP");
    case 0x59: return disasm_decode_wordreg(ip, "POP");
    case 0x5a: return disasm_decode_wordreg(ip, "POP");
    case 0x5b: return disasm_decode_wordreg(ip, "POP");
    case 0x5c: return disasm_decode_wordreg(ip, "POP");
    case 0x5d: return disasm_decode_wordreg(ip, "POP");
    case 0x5e: return disasm_decode_wordreg(ip, "POP");
    case 0x5f: return disasm_decode_wordreg(ip, "POP");
    case 0x60: return disasm_show(ip, "PUSHA");
    case 0x61: return disasm_show(ip, "POPA");
    case 0x62: return disasm_decode_w(ip, "BOUND", segment_override);
    case 0x63: return disasm_decode_databyte(ip, "DB");
    case 0x64: return disasm_decode_databyte(ip, "DB");
    case 0x65: return disasm_decode_databyte(ip, "DB");
    case 0x66: return disasm_decode_databyte(ip, "DB");
    case 0x67: return disasm_decode_databyte(ip, "DB");
    case 0x68: return disasm_decode_d16(ip, "PUSH");
    case 0x69: return disasm_decode_imul_w(ip, "IMUL", segment_override);
    case 0x6a: return disasm_decode_d8(ip, "PUSH");
    case 0x6b: return disasm_decode_imul_b(ip, "IMUL", segment_override);
    case 0x6c: return disasm_show(ip, "INSB");
    case 0x6d: return disasm_show(ip, "INSW");
    case 0x6e: return disasm_show_str(ip, "OUTSB", segment_override);
    case 0x6f: return disasm_show_str(ip, "OUTSW", segment_override);
    case 0x70: return disasm_decode_jump8(ip, "JO", reg_ip);
    case 0x71: return disasm_decode_jump8(ip, "JNO", reg_ip);
    case 0x72: return disasm_decode_jump8(ip, "JB", reg_ip);
    case 0x73: return disasm_decode_jump8(ip, "JAE", reg_ip);
    case 0x74: return disasm_decode_jump8(ip, "JZ", reg_ip);
    case 0x75: return disasm_decode_jump8(ip, "JNZ", reg_ip);
    case 0x76: return disasm_decode_jump8(ip, "JBE", reg_ip);
    case 0x77: return disasm_decode_jump8(ip, "JA", reg_ip);
    case 0x78: return disasm_decode_jump8(ip, "JS", reg_ip);
    case 0x79: return disasm_decode_jump8(ip, "JNS", reg_ip);
    case 0x7a: return disasm_decode_jump8(ip, "JP", reg_ip);
    case 0x7b: return disasm_decode_jump8(ip, "JNP", reg_ip);
    case 0x7c: return disasm_decode_jump8(ip, "JL", reg_ip);
    case 0x7d: return disasm_decode_jump8(ip, "JGE", reg_ip);
    case 0x7e: return disasm_decode_jump8(ip, "JLE", reg_ip);
    case 0x7f: return disasm_decode_jump8(ip, "JG", reg_ip);
    case 0x80: return disasm_decode_bd8(ip, table_8x[(ip[1] & 0x38) >> 3], segment_override);
    case 0x81: return disasm_decode_wd16(ip, table_8x[(ip[1] & 0x38) >> 3], segment_override);
    case 0x82: return disasm_decode_bd8(ip, table_8x[(ip[1] & 0x38) >> 3], segment_override);
    case 0x83: return disasm_decode_wd8(ip, table_8x[(ip[1] & 0x38) >> 3], segment_override);
    case 0x84: return disasm_decode_br8(ip, "TEST", segment_override);
    case 0x85: return disasm_decode_wr16(ip, "TEST", segment_override);
    case 0x86: return disasm_decode_br8(ip, "XCHG", segment_override);
    case 0x87: return disasm_decode_wr16(ip, "XCHG", segment_override);
    case 0x88: return disasm_decode_br8(ip, "MOV", segment_override);
    case 0x89: return disasm_decode_wr16(ip, "MOV", segment_override);
    case 0x8a: return disasm_decode_r8b(ip, "MOV", segment_override);
    case 0x8b: return disasm_decode_r16w(ip, "MOV", segment_override);
    case 0x8c: return disasm_decode_ws(ip, "MOV", segment_override);
    case 0x8d: return disasm_decode_r16w(ip, "LEA", segment_override);
    case 0x8e: return disasm_decode_sw(ip, "MOV", segment_override);
    case 0x8f: return disasm_decode_w(ip, "POP", segment_override);
    case 0x90: return disasm_show(ip, "NOP");
    case 0x91: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x92: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x93: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x94: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x95: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x96: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x97: return disasm_decode_wordregx(ip, "XCHG", "AX,");
    case 0x98: return disasm_show(ip, "CBW");
    case 0x99: return disasm_show(ip, "CWD");
    case 0x9a: return disasm_decode_far(ip, "CALL");
    case 0x9b: return disasm_show(ip, "WAIT");
    case 0x9c: return disasm_show(ip, "PUSHF");
    case 0x9d: return disasm_show(ip, "POPF");
    case 0x9e: return disasm_show(ip, "SAHF");
    case 0x9f: return disasm_show(ip, "LAHF");
    case 0xa0: return disasm_decode_almem(ip, "MOV", segment_override);
    case 0xa1: return disasm_decode_axmem(ip, "MOV", segment_override);
    case 0xa2: return disasm_decode_memal(ip, "MOV", segment_override);
    case 0xa3: return disasm_decode_memax(ip, "MOV", segment_override);
    case 0xa4: return disasm_show_str(ip, "MOVSB", segment_override);
    case 0xa5: return disasm_show_str(ip, "MOVSW", segment_override);
    case 0xa6: return disasm_show_str(ip, "CMPSB", segment_override);
    case 0xa7: return disasm_show_str(ip, "CMPSW", segment_override);
    case 0xa8: return disasm_decode_ald8(ip, "TEST");
    case 0xa9: return disasm_decode_axd16(ip, "TEST");
    case 0xaa: return disasm_show(ip, "STOSB");
    case 0xab: return disasm_show(ip, "STOSW");
    case 0xac: return disasm_show_str(ip, "LODSB", segment_override);
    case 0xad: return disasm_show_str(ip, "LODSW", segment_override);
    case 0xae: return disasm_show(ip, "SCASB");
    case 0xaf: return disasm_show(ip, "SCASW");
    case 0xb0: return disasm_decode_rd8(ip, "MOV");
    case 0xb1: return disasm_decode_rd8(ip, "MOV");
    case 0xb2: return disasm_decode_rd8(ip, "MOV");
    case 0xb3: return disasm_decode_rd8(ip, "MOV");
    case 0xb4: return disasm_decode_rd8(ip, "MOV");
    case 0xb5: return disasm_decode_rd8(ip, "MOV");
    case 0xb6: return disasm_decode_rd8(ip, "MOV");
    case 0xb7: return disasm_decode_rd8(ip, "MOV");
    case 0xb8: return disasm_decode_rd16(ip, "MOV");
    case 0xb9: return disasm_decode_rd16(ip, "MOV");
    case 0xba: return disasm_decode_rd16(ip, "MOV");
    case 0xbb: return disasm_decode_rd16(ip, "MOV");
    case 0xbc: return disasm_decode_rd16(ip, "MOV");
    case 0xbd: return disasm_decode_rd16(ip, "MOV");
    case 0xbe: return disasm_decode_rd16(ip, "MOV");
    case 0xbf: return disasm_decode_rd16(ip, "MOV");
    case 0xc0: return disasm_decode_bbitd8(ip, segment_override);
    case 0xc1: return disasm_decode_wbitd8(ip, segment_override);
    case 0xc2: return disasm_decode_d16(ip, "RET");
    case 0xc3: return disasm_show(ip, "RET");
    case 0xc4: return disasm_decode_r16w(ip, "LES", segment_override);
    case 0xc5: return disasm_decode_r16w(ip, "LDS", segment_override);
    case 0xc6: return disasm_decode_bd8(ip, "MOV", segment_override);
    case 0xc7: return disasm_decode_wd16(ip, "MOV", segment_override);
    case 0xc8: return disasm_decode_enter(ip);
    case 0xc9: return disasm_show(ip, "LEAVE");
    case 0xca: return disasm_decode_d16(ip, "RETF");
    case 0xcb: return disasm_show(ip, "RETF");
    case 0xcc: return disasm_show(ip, "INT 3");
    case 0xcd: return disasm_decode_d8(ip, "INT");
    case 0xce: return disasm_show(ip, "INTO");
    case 0xcf: return disasm_show(ip, "IRET");
    case 0xd0: return disasm_decode_bbit1(ip, segment_override);
    case 0xd1: return disasm_decode_wbit1(ip, segment_override);
    case 0xd2: return disasm_decode_bbitcl(ip, segment_override);
    case 0xd3: return disasm_decode_wbitcl(ip, segment_override);
    case 0xd4: return disasm_decode_adjust(ip, "AAM");
    case 0xd5: return disasm_decode_adjust(ip, "AAD");
    case 0xd6: return disasm_show(ip, "SALC");
    case 0xd7: return disasm_show(ip, "XLAT");
    case 0xd8: return disasm_show(ip, "ESC");
    case 0xd9: return disasm_show(ip, "ESC");
    case 0xda: return disasm_show(ip, "ESC");
    case 0xdb: return disasm_show(ip, "ESC");
    case 0xdc: return disasm_show(ip, "ESC");
    case 0xdd: return disasm_show(ip, "ESC");
    case 0xde: return disasm_show(ip, "ESC");
    case 0xdf: return disasm_show(ip, "ESC");
    case 0xe0: return disasm_decode_jump8(ip, "LOOPNE", reg_ip);
    case 0xe1: return disasm_decode_jump8(ip, "LOOPE", reg_ip);
    case 0xe2: return disasm_decode_jump8(ip, "LOOP", reg_ip);
    case 0xe3: return disasm_decode_jump8(ip, "JCXZ", reg_ip);
    case 0xe4: return disasm_decode_ald8(ip, "IN");
    case 0xe5: return disasm_decode_axd8(ip, "IN");
    case 0xe6: return disasm_decode_d8al(ip, "OUT");
    case 0xe7: return disasm_decode_d8ax(ip, "OUT");
    case 0xe8: return disasm_decode_jump(ip, "CALL", reg_ip);
    case 0xe9: return disasm_decode_jump(ip, "JMP", reg_ip);
    case 0xea: return disasm_decode_far(ip, "JMP");
    case 0xeb: return disasm_decode_jump8(ip, "JMP", reg_ip);
    case 0xec: return disasm_show_io(ip, "IN", "AL,DX");
    case 0xed: return disasm_show_io(ip, "IN", "AX,DX");
    case 0xee: return disasm_show_io(ip, "OUT", "DX,AL");
    case 0xef: return disasm_show_io(ip, "OUT", "DX,AX");
    case 0xf0: return disasm_show(ip, "LOCK");
    case 0xf1: return disasm_decode_databyte(ip, "DB");
    case 0xf2: return disasm_show_rep(ip, "REPNZ", reg_ip, segment_override);
    case 0xf3: return disasm_show_rep(ip, "REPZ", reg_ip, segment_override);
    case 0xf4: return disasm_show(ip, "HLT");
    case 0xf5: return disasm_show(ip, "CMC");
    case 0xf6: return disasm_decode_f6(ip, segment_override);
    case 0xf7: return disasm_decode_f7(ip, segment_override);
    case 0xf8: return disasm_show(ip, "CLC");
    case 0xf9: return disasm_show(ip, "STC");
    case 0xfa: return disasm_show(ip, "CLI");
    case 0xfb: return disasm_show(ip, "STI");
    case 0xfc: return disasm_show(ip, "CLD");
    case 0xfd: return disasm_show(ip, "STD");
    case 0xfe: return disasm_decode_b(ip, table_fe[(ip[1] & 0x38) >> 3], segment_override);
    case 0xff: return disasm_decode_ff(ip, segment_override);
    }
}
