/*
 * xatmas2 - Atmas 2 compatible Cross-Assembler
 *
 * Copyright (C) 2004-2011 by Richard Jackman <richard.miles.jackman@gmail.com>
 * Copyright (C) 2018 by Ivo van Poorten <ivopvp@gmail.com>
 * See LICENSE for details
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <err.h>
#include <stdarg.h>

#define fclose(x) if (x) { fclose(x); x=NULL; }

typedef struct {                /* Define the structure ssstype */
    char symbol[9];
    unsigned int symadr;
    char ofst;
    char macf;
    char symtyp;
} ssstype;

#define MAX_BIN_SIZ     0x1ffffL
#define MAXSYMS         2000
#define MAXMACSYMS      200
#define MAXLOCLABS      50
#define MXLBS           10          // maximum # of labels in present macro
#define MAXMACLEV       10          // maximum # ofnesting levels in macros
#define NOTFOUND        0xeffff
#define MAX_TXT_SIZ     0xfffffL
#define MAXDUMS         20
#define LSTBYTES        10

enum { OPC, NN, AA, AAAA, AAX, AAAAX, AAY, AAAAY, BAAXB, BAABY, BRA, BAAAAB,
    ERROR
};
enum { UNUSED, SYMBOL, MACRO, MACRO_UNUSED, MACRO_LAB };

static const char opcdat[] =
    "BITSTYLDYCPYCPXORAANDEORADCSTALDACMPSBCASLROLLSRRORSTXLDXDECINC";

static const unsigned short data[21][15] = {
    /* BIT */
    {0, 0, 0x24, 0x2C, 0, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0x48, 0x4A},
    /* STY */
    {0, 0, 0x84, 0x8C, 0x94, 0, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* LDY */
    {0, 0xA0, 0xA4, 0xAC, 0xB4, 0xBC, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0xD0, 0x4A},
    /* CPY */
    {0, 0xC0, 0xC4, 0xCC, 0, 0, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* CPX */
    {0, 0xE0, 0xE4, 0xEC, 0, 0, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* ORA */
    {0, 9, 5, 0xD, 0x15, 0x1D, 0, 0x19, 1, 0x11, 0xD6, 0x67, 0x46, 0, 0},
    /* AND */
    {0, 0x29, 0x25, 0x2D, 0x35, 0x3D, 0, 0x39, 0x21, 0x31, 0xD6, 0x67, 0x46, 0, 0},
    /* EOR */
    {0, 0x49, 0x45, 0x4D, 0x55, 0x5D, 0, 0x59, 0x41, 0x51, 0xD6, 0x67, 0x46, 0, 0},
    /* ADC */
    {0, 0x69, 0x65, 0x6D, 0x75, 0x7D, 0, 0x79, 0x61, 0x71, 0xD6, 0x67, 0x46, 0xBF, 0x4A},
    /* STA */
    {0, 0, 0x85, 0x8D, 0x95, 0x9D, 0, 0x99, 0x81, 0x91, 0xD6, 0x67, 0x46, 0, 0},
    /* LDA */
    {0, 0xA9, 0xA5, 0xAD, 0xB5, 0xBD, 0, 0xB9, 0xA1, 0xB1, 0xD6, 0x67, 0x46, 0, 0},
    /* CMP */
    {0, 0xC9, 0xC5, 0xCD, 0xD5, 0xDD, 0, 0xD9, 0xC1, 0xD1, 0xD6, 0x67, 0x46, 0, 0},
    /* SBC */
    {0, 0xE9, 0xE5, 0xED, 0xF5, 0xFD, 0, 0xF9, 0xE1, 0xF1, 0xD6, 0x67, 0x46, 0x9E, 0x49},
    /* ASL */
    {0xA, 0, 6, 0xE, 0x16, 0x1E, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* ROL */
    {0x2A, 0, 0x26, 0x2E, 0x36, 0x3E, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0x37, 0x4A},
    /* LSR */
    {0x4A, 0, 0x46, 0x4E, 0x56, 0x5E, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0xF3, 0x49},
    /* ROR */
    {0x6A, 0, 0x66, 0x6E, 0x76, 0x7E, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0x8D, 0x49},
    /* STX */
    {0, 0, 0x86, 0x8E, 0, 0, 0x96, 0, 0, 0, 0xD6, 0x67, 0x46, 0x15, 0x4A},
    /* LDX */
    {0, 0xA2, 0xA6, 0xAE, 0, 0, 0xB6, 0xBE, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* DEC */
    {0, 0, 0xC6, 0xCE, 0xD6, 0xDE, 0, 0, 0, 0, 0xD6, 0x67, 0x46, 0, 0},
    /* INC */
    {0, 0, 0xE6, 0xEE, 0xF6, 0xFE, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

unsigned int sgndtb[100], *sgndtbptr1, *sgndtbptr2, pcsv;

char double_byte_flg = 1;
char ch;
char section_type;
char *txtptr, *txtptr_sav;      /* pointer for all addresses in this buffer */
char opcode[40], operand[300];
char *xlbfptr;

char *linbegptr;                /* begin of present line */
char outflgs[40];
char linbuf[MAXMACLEV][80];

int txtsiz, include_txtsiz;

int symnum, symval, linnum, linnum_sav, mismatch_linnum, symnumext;
unsigned int pc, pc_sav, mismatch_pc;
int presymnum;
int secsiz;

short linlen;                   /* length of present line in text buffer */
int cnt;
short pass;
short labflg;
short orgflg;
short lstflg;
short include_flg;
short data_flg;
short lensv;
int bin;
short find_symad_flg;
short maclevel;
short symfg;
int donum;
int loopnum;
int minusnum;
int plusnum;
int skipnum;
int contnum;
int endifnum;
char tempstr[5];
char prnlnbufbeg[120];
char prnlnbuf2[10][40];
char cmpflbytln[9][40];
short mismatchln_flg[10];
short bytcnt;
short prnlnum;

char xlbuf[MAX_BIN_SIZ];
char prnlnbuf[120];

char *src_filename, *lstfilename, *xexfilename;
char include_filename[300];

ssstype sss[MAXSYMS];

FILE *stream;       // src file, later xex file
FILE *stream2;      // lis file
FILE *stream3;      // included files
fpos_t pos2;

static const char opcdat1[] = "BRKPHPASLCLCPLPROLSECRTIPHALSRCLIRTSPLARORSEI"
        "DEYTXATYATXSTAYTAXCLVTSXINYDEXCLDINXNOPSED";

static const int dat1[] = { 0, 8, 10, 24, 40, 42, 56, 64, 72, 74, 88, 96, 104,
    106, 120, 136, 138, 152, 154, 168, 170, 184, 186, 200, 202, 216, 232, 234,
    248
};

static const char opcds[] =
    " BIT STY LDY CPY CPX ORA AND EOR ADC STA LDA CMP SBC"
    " ASL ROL LSR ROR STX LDX DEC INC BRK PHP ASL CLC PLP ROL SEC RTI PHA LSR"
    " CLI RTS PLA ROR SEI DEY TXA TYA TXS TAY TAX CLV TSX INY DEX CLD INX NOP"
    " SED BPL BMI BVC BVS BCC BCS BNE BEQ JMP JSR ";    // for checking labels

static const char assdirecs[] =
        " EQU EPZ ORG ASC DFB DFW ORG RMB INITRMB FILLMEM INCLUDE ";

static const char assdirecssolo[] = " OUT NOHEADER ";

static const char brastrdat[] = "BPLBMIBVCBVSBCCBCSBNEBEQ";
static const unsigned short bradat[] =
        { 0x10, 0x30, 0x50, 0x70, 0x90, 0xb0, 0xd0, 0xf0 };
static const int intab[] = { 0x40, 0, 0x20, 0x60, 0xA0, 0xC0, 0x80, 0xE0 };

// ----------------------------------------------------------------------------

static int quiet;

static void info_and_fatal(const int s, char *f, ...) {
    va_list ap;
    if (s) {
        if (linnum) fprintf(stderr, "xatmas2: error at line %d\n", linnum);
        if (pc) fprintf(stderr, "xatmas2: address: $%04X\n", pc);
        if (maclevel) fprintf(stderr, "xatmas2: error in macro\n");
    } else if (quiet) {
        return;
    }
    va_start(ap,f);
    fprintf(stderr, "xatmas2: %s: ", s ? "fatal" : "info");
    vfprintf(stderr, f, ap);
    va_end(ap);
    if (s) exit(s);
}

#define info(...)    info_and_fatal(0, __VA_ARGS__)
#define fatal(...)   info_and_fatal(1, __VA_ARGS__)

// ----------------------------------------------------------------------------

static int prec(char symbol) {
    if (symbol == '+' || symbol == '-') return 1;
    if (symbol == '*' || symbol == '/') return 2;
    return 0;
}

// ----------------------------------------------------------------------------

static unsigned int symtoi3(char *strbeg) {
    if (symnum == 0) return (NOTFOUND);
    for (int i=0; i<symnum; i++) {
        for (int j=0; sss[i].symbol[j] == strbeg[j]; j++) {
            if ((!strbeg[j] && !sss[i].symbol[j])) return (sss[i].symadr);
        }
    }
    return (NOTFOUND);
}

// ----------------------------------------------------------------------------

/*********** symtoi1()
 * converts symbol or number to integer (return value begins at txtptr.
 * When done txtptr is the address directly after the location of symbol.
 * If evaluation is not possible,the value returned is NOTFOUND
 */

static unsigned int symtoi1() {
    char strbeg[40];
    char *ptr;
    char ch;
    unsigned int i, chrcnt;
    ssstype *sssptr;

    ptr = strbeg;
    ch = *txtptr;
    symfg = 0;

    if (ch == '*') {            /* *:program counter) */
        txtptr++;
        return (pc);
    }
    if (isdigit(ch)) {          /*  :Decimal */
        while (isdigit(*txtptr))
            *ptr++ = *txtptr++;
        *ptr = '\0';
        return (atoi(strbeg));
    }
    if (ch == '%') {            /*  %:Binary */
        txtptr++;
        if (!(*txtptr == '0' || *txtptr == '1'))
            fatal("Binary digit 0 or 1 expected.\n");
        ch = 0;
        chrcnt = 1;
        while (*txtptr == '0' || *txtptr == '1') {
            if (chrcnt++ > 8)
                fatal("too many binary digits. maximum number of digits is "
                      "8. e.g. %10010011\n");
            ch <<= 1;
            ch += *txtptr++ - '0';
        }
        return (ch);
    }
    if (ch == '$') {            /*  $:Hex */
        txtptr++;
        if (isxdigit(*txtptr) == 0)
            fatal("hexidecimal digit expected\n");
        chrcnt = 1;
        while (isxdigit(*txtptr)) {
            if (chrcnt++ > 4)
                fatal("too many hexdecimal digits. maximum number of digits "
                      "is 4 eg. $1234.\n");
            *ptr++ = *txtptr++;
        }
        *ptr = '\0';
        sscanf(strbeg, "%x", &i);
        return (i);
    }
    if (*txtptr == '\'') {      /*  ':ASCII character */
        txtptr++;
        ch = *txtptr++;
        if (*txtptr == '\'')
            txtptr++;
        return (ch);
    }

    switch (ch) {
    case '+':
        strcpy(strbeg, "PLUS");
        break;
    case '-':
        strcpy(strbeg, "MINUS");
        break;
    default:                   //label
        if (isspace(*txtptr))
            fatal("illegal space, tab, or end of line\n");
        if (!(isalpha(*txtptr)))
            fatal("label must begin with a letter\n");

        while (isalnum(*txtptr))
            *ptr++ = (char)toupper(*txtptr++);
        *ptr = '\0';            //label
    }

//printf ("opcode: %s\tstrbeg: %s\n",operand,strbeg);

    if (pass == 1) {
        if (find_symad_flg)
            return (symtoi3(strbeg));
    } else {                    /* pass == 2 */

        if ((strcmp(strbeg, "DO") == 0))
            sprintf(strbeg, "DO%4.4i", donum);
        if ((strcmp(strbeg, "LOOP") == 0))
            sprintf(strbeg, "LOOP%4.4i", loopnum);
        if ((strcmp(strbeg, "MINUS") == 0))
            sprintf(strbeg, "MINU%4.4i", minusnum);
        if ((strcmp(strbeg, "PLUS") == 0))
            sprintf(strbeg, "PLUS%4.4i", plusnum);
        if ((strcmp(strbeg, "SKIP") == 0))
            sprintf(strbeg, "SKIP%4.4i", skipnum);
        if ((strcmp(strbeg, "CONT") == 0))
            sprintf(strbeg, "CONT%4.4i", contnum);
        if ((strcmp(strbeg, "ENDIF") == 0))
            sprintf(strbeg, "ENDI%4.4i", endifnum);

        if ((sssptr =
             bsearch(strbeg, sss, symnum, sizeof(ssstype), (int (*)(const void *, const void *)) strcmp)) != 0) {
            if (maclevel)
                sssptr->macf = MACRO_LAB;
            else
                sssptr->macf = SYMBOL;

            if (*txtptr == '+') {
                if ((*(txtptr + 1) - '0') > sssptr->ofst)
                    sssptr->ofst = *(txtptr + 1) - '0';
               //The next two lines take care of hidden labels 
                if (*(txtptr + 2) == '+')
                    sssptr->ofst = sssptr->ofst + *(txtptr + 3) - '0';
            }
            if (*txtptr == '-')
                sssptr->ofst = (char)(256 - (*(txtptr + 1) - '0'));

            symfg = 1;
            return (sssptr->symadr);
        } else {
            fatal("label undefined\n");
        }
    }
    return (NOTFOUND);
}

// ----------------------------------------------------------------------------

static unsigned int expresstoi() {
    int i;
    int result = 0;

    char notfound = 0;
    char *tptrsv, *ptr;
    char infix[50], postfix[50];
    int symval_list[30];
    int symval_cnt = 0;

    int j = 0, k = 0, length;
    char temp;
    char stack[30];
    int intstack[30];
    int tos = 0;
    int a, b;

    tptrsv = txtptr;
    ptr = infix;

// tokenise ************

    if (*operand == '(')
        *ptr++ = '(';           //because txtptr++ if beginning bracket


    if (isspace(*(txtptr + 1)))
        if ((*txtptr == '-') || (*txtptr == '+'))
            return (symtoi1()); // "- " is DO, "+ " is CONT


    do {
        switch (*txtptr) {
        case '+':
        case '-':
            if ((tptrsv == txtptr) || (*(txtptr - 1) == '(')) {
                symval_list[symval_cnt] = 0;
                *ptr++ = symval_cnt++ + 'A';
            }
        case '/':
        case '*':
        case ')':
        case '(':
            *ptr++ = *txtptr++;
            break;
        default:

            if ((symval_list[symval_cnt] = symtoi1()) == NOTFOUND)
                notfound = 1;
            *ptr++ = symval_cnt++ + 'A';
        }
    } while (!(isspace(*txtptr) || (*txtptr == ',') || (*txtptr == ':')));

    if (*txtptr == ',')         //if ",X)"
        if (*(txtptr + 1) == 'X' || *(txtptr + 1) == 'x')
            if (*(txtptr + 2) == ')')
                if (isspace(*(txtptr + 3)))
                    *ptr++ = ')';       // add ')'

    *ptr = '\0';
    //printf ("%s\n",infix);

    ptr = strchr(infix, '*');
    while (ptr) {
        if (!isalpha(*(ptr + 1)) && (*(ptr + 1) != '(')) {
            symval_list[symval_cnt] = pc;
            *ptr = symval_cnt++ + 'A';
        }

        ptr = strchr(++ptr, '*');
    }

//convert to postfix  *********

    i = j = k = length = 0;

    length = strlen(infix);
    for (i = 0; i < length; i++) {
        if (!strchr("+-*/()", infix[i])) //Numbers are added to postfix string
            postfix[j++] = infix[i];

        else               //If an operator or a bracket is encountered...
        {
            if (tos == 0) //If the stack is empty, the operator is added to it

                stack[tos++] = infix[i];

            else                //stack not empty
            { //Operators are pushed or popped based on the order of precedence
                if (infix[i] != ')' && infix[i] != '(') {
                    if (prec(infix[i]) <= prec(stack[tos - 1]))
                        postfix[j++] = stack[--tos];
                    stack[tos++] = infix[i];
                } else {
                    if (infix[i] == '(')
                        stack[tos++] = infix[i];

                    if (infix[i] == ')') {
                        temp = stack[--tos];
                        while (temp != '(') {
                            postfix[j++] = temp;
                            if (tos == 0)
                                fatal("missing opening parenthesis\n");
                            temp = stack[--tos];
                        }
                    }
                }

            }

        }
        postfix[j] = '\0';

    }
    while (tos != 0) {
        postfix[j++] = stack[--tos];
    }
    postfix[j] = '\0';

// convert postfix to int ***************

    tos = 0;
    ptr = postfix;
    while (*ptr) {
        if (isalpha(*ptr))
            intstack[tos++] = symval_list[*ptr - 'A'];
        else {
            b = intstack[--tos];
            a = intstack[--tos];

            switch (*ptr) {
            case '+':
                intstack[tos++] = a + b;
                break;
            case '-':
                intstack[tos++] = a - b;
                break;
            case '*':
                intstack[tos++] = a * b;
                break;
            case '/':
                intstack[tos++] = a / b;
                break;
            default:
                if (*ptr == '(') fatal("missing closing parenthisis\n");
                fatal("infix: %s - character '%c' in %s operand "
                        "is %s. illegal character in postfix expression\n",
                        infix, *ptr, postfix, operand);
                break;
            }
        }
        ptr++;
    }

    if (strstr(opcode, "),Y"))
        txtptr--;
    result = intstack[--tos];

    if (tos) {
        fatal("operand: %s   infix: %s   postfix: %s  "
              "illegal syntax. can't resolve expression\n",
              operand, infix, postfix);
    }

    if( *txtptr == ':' )
    {
        if ( *(txtptr + 1) == 'L' )
        {
            txtptr += 2;
            result = result & 0xFF;
        }
        else if ( *(txtptr + 1) == 'H' )
        {
            txtptr += 2;
            result = result >> 8;
        }
    }

    if (notfound)
        result = NOTFOUND;

    return (result);
}

// ----------------------------------------------------------------------------

static unsigned int expresstoi_p2() {
    unsigned int val;

    find_symad_flg = 0;
    val = expresstoi();
    find_symad_flg++;
    return (val);
}

// ----------------------------------------------------------------------------

static void sectyp(char line_type) {
    if (!labflg && (line_type == ';') && strchr("!&'", section_type))
        line_type = section_type;

    if ((line_type != section_type) || labflg)  // if new sec type or a label
    {
        section_type = line_type;
        if (labflg)             //if there is a label....
        {
            if (pass == 1) {
                if (line_type == ';')   //with normal code (;)..
                    sss[symnum - 1].symtyp = 0; // label is enough, so no entry
                else
                    sss[symnum - 1].symtyp = line_type; // else make entry
            }
            if ((pass == 2) && (line_type == '&')) {
                sss[symnum - 1].symtyp = line_type;
            }
        } else /* no label */ if ((pass == 2) && (pc > 0xff)) {
            if (symnumext >= MAXSYMS)
                fatal("too many symbols\n");
            sss[symnumext].symtyp = line_type;
            sss[symnumext++].symadr = pc;
        }
    }
}

// ----------------------------------------------------------------------------

static int ssscmpadr(const ssstype * a1, const ssstype * a2) {
    return (a1->symadr - a2->symadr);
}

// ----------------------------------------------------------------------------

static void list_label(int i) {
    fprintf(stderr, "%s\t", sss[i].symbol);
    if (sss[i].macf == MACRO) fprintf(stderr, "MACRO\n");
    else                      fprintf(stderr, " = %X\n", sss[i].symadr);
}

// ----------------------------------------------------------------------------

static void list_label_f(int i) {
    fprintf(stream2, "%s\t", sss[i].symbol);
    if (sss[i].macf == MACRO)
        fprintf(stream2, "MACRO\n");
    else
        fprintf(stream2, " = %X\n", sss[i].symadr);
}

// ----------------------------------------------------------------------------

static void operand_to_filename(char filename[]) {
    int cnt = 0;
    char *linptr;

    switch (*txtptr++) {
    case '\n':
    case '\t':
        fatal("filename missing\n");
        break;
    case ' ':
        linptr = txtptr;
        if (!strchr(" \t\n", *txtptr)) {        /* there is an operand */
            if (*linptr == '"') {
                for (;;) {      /* create operand[] */
                    operand[cnt++] = *linptr++;
                    if (strchr("\"\t\n", *linptr)) {
                        operand[cnt++] = *linptr++;
                        break;
                    }

                }
            } else {
                while (!(strchr(" \t\n", *linptr))) {   /* create operand[] */
                    operand[cnt++] = *linptr++;
                }
            }
        } else
            fatal("filename missing\n");
        break;
    }
    operand[cnt] = 0;
    strcpy(filename, operand);
}

// ----------------------------------------------------------------------------

static void find_operand(void) {
    int cnt = 0;
    char *linptr;

    switch (*txtptr++) {
    case '\n':
        break;
    case ' ':
    case '\t':
        linptr = txtptr;
        if (!strchr(" \t\n", *txtptr)) {        /* there is an operand */
            if (*txtptr == '(')
                txtptr++;
            while (!(strchr(" \t\n", *linptr))) {       /* create operand[] */

                operand[cnt++] = *linptr++;
            }
        }
    }
    operand[cnt] = 0;
}

// ----------------------------------------------------------------------------

static void toxlbuf(char ch) {
    char tempstr[10];

    if (!orgflg) fatal("ORG value must precede this line\n");

    *xlbfptr = ch;

    xlbfptr++;
    pc++;

    if (pass == 2) {
        bytcnt++;
        if ((strchr(outflgs, 'L') || strchr(outflgs, 'l'))
            && !(strchr(outflgs, 'M') || strchr(outflgs, 'm'))) {

            sprintf(tempstr, "%02X ", ch & 255);
            if (bytcnt <= LSTBYTES)
                strcat(prnlnbufbeg, tempstr);
            else {
                if (bytcnt % LSTBYTES == 1) {
                    prnlnum++;
                    sprintf(prnlnbuf2[prnlnum - 1], "%04X: ", pc - 1);
                }
                strcat(prnlnbuf2[prnlnum - 1], tempstr);
            }
        }
    }
}

// ----------------------------------------------------------------------------

/********* chkopc    returns the position (0, 1, 2,.. ) of opc in string
						returns -1 if not found */

static int chkopc(const char opcstring[]) {
    int cnt = 0;
    const char *ptr;

    if (*(opcode + 3))
        return (-1);
    ptr = opcstring;
    while (*ptr) {
        if (!memcmp(opcode, ptr, 3))
            return (cnt);
        ptr += 3;
        cnt++;
    }
    return (-1);
}

// ----------------------------------------------------------------------------

static void sort_symbols(void) {
    int i;
    qsort(sss, symnum, sizeof(ssstype), (int (*)(const void *, const void *)) strcmp);

    for (i = symnum - 1; i > 1; i--) {
        if (strcmp(sss[i].symbol, sss[i - 1].symbol) == 0) {
            fprintf(stderr, "\nSame label twice -\n");
            list_label(i - 1);
            list_label(i);
            exit(1);
        }
    }
}

// ----------------------------------------------------------------------------

#define NEXT do { argc--;argv++; } while(0)

int main(int argc, char *argv[]) {
    int braoffset;

    char *includebuf;
    char *txtendptr;            /* pointer to end of text Buffer (=10) */
    char *linptr;
    char *ptr;                  /* temporary pointer */
    int pos;
    char ch;
    int offset;
    unsigned int operval;
    int operno;
    int i, intval;
    char label[40];
    char labelstr[40];
    char opcstr[40];
    char *tpsav;
    int temp;

    char *operptr, macdumtab[MAXDUMS][10], dumbuf[80];
    int dumnum = 0, macroflg;

    char macsymbol[MAXMACSYMS][10];
    char *macptrtab[MAXMACSYMS];
    short macnum = 0;

    char *linbegsav[MAXMACLEV];
    short linlensav[MAXMACLEV];
    char *macargsptr[MAXMACLEV];
    char *lnbfptr;
    short loclabnum;            /* # of labels in loclabtb */
    char loclabtb[MAXLOCLABS][5];
    int loclabtbpos;
    int labexttab[MAXLOCLABS];  /* 0001 to 9999 */
    int lclb[MAXMACLEV][MXLBS]; /* table of label nos. in present macro */
    unsigned short lclbnum[MAXMACLEV];
    char lab[5];
    char *labptr;

    short lo_fg;
    short objfile_flag = 0;
    short rmbflg = 0;
    int rmbadr = 0;
    char *txtbuf;

    txtbuf = (char *)malloc(MAX_TXT_SIZ);
    if (!txtbuf) fatal("Can't allocate memory for text buffer.\n");

    NEXT;
    while (argc--) {
        if (!strcmp(*argv, "-o")) {
            NEXT;
            if (argc >= 0) xexfilename = *argv;
            else      fatal("-o needs an argument\n");
        } else if (!strcmp(*argv, "-q")) {
            quiet = 1;
        } else if (!strcmp(*argv, "-v")) {
            src_filename = NULL;
            break;
        } else {
            src_filename = *argv;
        }
        argv++;
    }

    if (!src_filename) {
        info("\n\n\txatmas2 v2.7\n"
             "\tCopyright (c) 2004-2011 by Richard Jackman\n"
             "\tCopyright (c) 2018 by Ivo van Poorten\n\n");
        fatal("usage: xatmas2 [-q] [-v] [-o output.xex] <file.src>\n");
    }

    if (!((stream = fopen(src_filename, "r"))))
        fatal("can't open %s\n", src_filename);

    txtsiz = fread(txtbuf, 1, MAX_TXT_SIZ, stream);
    fclose(stream);

    if (txtsiz == MAX_TXT_SIZ) fatal("file is too long\n");

    info("source size: %i bytes\n", txtsiz);

    lstfilename = strdup(src_filename);
    lstfilename = realloc(lstfilename, strlen(lstfilename) + 4);
    strcat(lstfilename, ".lis");

    if (!xexfilename) {
        xexfilename = strdup(src_filename);
        xexfilename = realloc(xexfilename, strlen(xexfilename) + 4);
        strcat(xexfilename, ".xex");
    }

/********************** Now work on buffer *********************/

    txtendptr = txtbuf + txtsiz;

    symnum = 0;
    *outflgs = 0;
    find_symad_flg = 1;
    maclevel = 0;

    for (i = 0; i < MAXMACLEV; i++)
        lclbnum[i] = 0;


    for (ptr = txtbuf; ptr < txtendptr; ptr++)
        if (*ptr == '\r') {
            if (*(ptr + 1) == '\n')
                *ptr = ' ';
            else
                *ptr = '\n';
        }
    // ATMAS adds $FF bytes at the end of files:
    while (txtendptr > txtbuf && *(txtendptr - 1) == '\377')
        txtendptr--;
    *txtendptr++ = '\n';
    *txtendptr++ = '\n';
    *txtendptr++ = '\0';

/********************** Main loop *******************/

    for (pass = 1; pass <= 2; pass++)
    {
        sgndtbptr1 = sgndtbptr2 = sgndtb;
        info("pass %d\n", pass);
        linptr = txtptr = txtbuf;
        xlbfptr = xlbuf;
        orgflg = 0;
        loclabnum = 0;
        macroflg = 0;

        donum = 0;
        loopnum = 0;
        minusnum = 0;
        plusnum = 1;
        skipnum = 1;
        contnum = 1;
        endifnum = 1;

        symnumext = symnum;
        include_flg = 0;
        if (pass == 1) {
            sss[symnum].symtyp = '#';
            sss[symnum++].symadr = 0xd40f;
        }

        for (linnum = 0; !((*txtptr == 0) && (include_flg == 0));
             txtptr = linbegptr + linlen) {
            if ((*txtptr == 0) && (include_flg)) {
                linnum = linnum_sav;
                txtptr = txtptr_sav;
                include_flg = 0;
            }


            if (strchr(outflgs, 'L') || strchr(outflgs, 'l')) {
                if (*prnlnbufbeg || *prnlnbuf) {
                    if (bytcnt < LSTBYTES) {
                        if (!(strchr(outflgs, 'M') || strchr(outflgs, 'm'))) {
                            for (i = LSTBYTES - bytcnt; i > 0; i--)
                                strcat(prnlnbufbeg, "   ");
                        } else
                            strcat(prnlnbufbeg, "\t");
                    }
                    strcat(prnlnbufbeg, prnlnbuf);
                    fprintf(stream2, "%s", prnlnbufbeg);

                    if (bytcnt > LSTBYTES) {
                        for (i = 1; i <= prnlnum; i++) {
                            fprintf(stream2, "%s\n", prnlnbuf2[i - 1]);
                        }
                    }
                    *prnlnbuf = '\0';
                    *prnlnbufbeg = '\0';
                    *prnlnbuf2[0] = '\0';
                    bytcnt = 0;
                    prnlnum = 0;
                    data_flg = 0;
                    for (i = 0; i < 10; i++)
                        mismatchln_flg[i] = 0;
                }
            }

            if (!maclevel)
                linnum++;       //no change of line number if macro
            *label = 0;
            labflg = 0;
            *operand = '\0';
            linbegptr = linptr = txtptr;        //set line begin to txtptr
            lnbfptr = linbuf[maclevel];
            linlen = (short)(strchr(linbegptr, '\n') + 1L - linbegptr);

            if (!maclevel && pass == 1) { /*Test line for illegal characters */
                for (cnt = 1; cnt <= linlen; cnt++) {
                    if (*linptr < 0) {
                        fatal("illegal character at line %i:\n", linnum);
                    }
                    linptr++;
                }
                linptr = linbegptr;
            }

            while (*linptr != '\n') {
                /* Determine and place local label in macro */
                *lnbfptr = 0;
                if ((*linptr == '@') && maclevel) {
                    ptr = linptr - 1;
                    cnt = 0;
                    while (isalnum(*ptr)) { /* find beginning of local label */
                        ptr--;
                        cnt++;
                    }
                    if (strchr(", \n+-", *ptr)) { /*Characters which can 
                                                    precede a local label! */
                        if (cnt > 4)
                            fatal("local labels cannot be longer than "
                                  "5 characters\n");
                        ptr++;
                        labptr = lab;
                        while (cnt) {
                            cnt--;
                            *labptr++ = *ptr++;
                        }
                        *labptr = 0;

                        for (loclabtbpos = 0; loclabtbpos < loclabnum;
                             loclabtbpos++)
                            if (strcmp(loclabtb[loclabtbpos], lab) == 0)
                                break;  /*local label found */
                        if (loclabtbpos == loclabnum) { // label into loclabtb
                            strcpy(loclabtb[loclabnum], lab);
                            labexttab[loclabnum++] = 0;
                        }
                        cnt = 0;
                        while (cnt < lclbnum[maclevel]) {
                            if (lclb[maclevel][cnt] == loclabtbpos)
                                break;
                            cnt++;
                        }
                        if (cnt == lclbnum[maclevel]) {
                            lclb[maclevel][cnt] = loclabtbpos;
                            labexttab[loclabtbpos]++;
                            lclbnum[maclevel]++;
                        }
                        /* Add four digits to label */
                        sprintf(lnbfptr, "%04d", labexttab[loclabtbpos]);
                        lnbfptr += 4;
                        linptr++;
                    }
                }

                /********** Dummy in macro ***********/

                if ((*linptr & 128) && !macroflg) { /* inverted characters */
                    ptr = macargsptr[maclevel];

                    cnt = *linptr & 127;
                    while (cnt) {
                        while (*ptr != ',')
                            ptr++;
                        ptr++;
                        cnt--;
                    }
                    if (strncmp(lnbfptr - 4, "ASC ", 4) == 0) {
                        ch = *ptr;
                        while (*ptr != '\n') {
                            *lnbfptr++ = *ptr++;
                            if (*ptr == ch) {
                                *lnbfptr++ = *ptr++;
                                break;
                            }
                        }
                    } else
                        while (strchr(",\t\n ", *ptr) == 0)
                            *lnbfptr++ = *ptr++;
                    while (*linptr & 128)
                        linptr++;
                } else /* *linptr < 128 */ if (*linptr != '\n')
                    *lnbfptr++ = *linptr++;
            }


            if ((!macroflg) && maclevel)
                txtptr = linbuf[maclevel];

            *lnbfptr++ = '\n';
            *lnbfptr = '\0';

/**********  Test first character of line ***********/

            switch (*txtptr) {
            case '\?':
            case '\n':          /* empty line   */
            case ';':
            case '*':
                continue;       /* comment              */
            case ' ':           /* white space  */
            case '\t':
                break;
            case '#':
                do {
                    txtptr = strchr(txtptr, '\n');
                    linnum++;
                    txtptr++;
                } while (*txtptr != '#');
                linbegptr = txtptr;
                linlen = 1;
                continue;

            case '-':
                strcpy(label, "MINUS");
                txtptr++;
                break;
            case '+':
                strcpy(label, "PLUS");
                txtptr++;
                break;
            default:           /* label                        */
            {
                if (!isalpha(*txtptr)) {
                    fprintf(stderr, "%c is not valid", *txtptr);
                    fatal("label must begin with alphabetic character\n");
                }
                pos = 0;
                while (isalnum(*txtptr))
                    label[pos++] = (char)toupper(*txtptr++);
                if (pos > 8)
                    fatal("label is too long\n");
                if (*txtptr == '@')
                    label[pos++] = *txtptr++;
                if (!isspace(*txtptr))
                    fatal("label must consist only of alphabetic and numeric "
                         "characters\n");
                label[pos] = 0;

                sprintf(labelstr, " %s ", label);
                if (strstr(opcds, labelstr))
                    fatal("you can't have an opcode at the beginning "
                           "of a line\n");
                if (strstr(assdirecs, labelstr))
                    fatal("you can't have an assembler directive the "
                           "beginning of a line\n");
                if (strstr(assdirecssolo, labelstr))
                    fatal("you can't have an assembler directive the "
                          "beginning of a line\n");
            }
            }

            /*      ********* find opcode *************************** */
            while ((*txtptr == ' ') || (*txtptr == '\t'))
                txtptr++;
            tpsav = txtptr;
            if (*txtptr == '\n')
                continue;
            if (isalpha(*txtptr) == 0)
                fatal("alphabetic character expected\n");
            ptr = opcode;

            while (isalnum(*txtptr))
                *ptr++ = (char)toupper(*txtptr++);      //opcode is upper case

            double_byte_flg = 0;

            if (*txtptr == '<') {
                double_byte_flg = 1;
                txtptr++;
            }

            if (!isspace(*txtptr))
                fatal("illegal character '%c' in symbol. space expected\n",
                                                                    *txtptr);

            *ptr = 0;           //opcode finalised

            if (!(*txtptr == ' ' || *txtptr == '\t') || *(txtptr + 1) == ' '
                                  || *(txtptr + 1) == '\n'
                                  || *(txtptr + 1) == '\t') {
                //Check operand for dirictives
                sprintf(opcstr, " %s ", opcode);
                if (strstr(assdirecs, opcstr))
                    fatal("directive must be followed by a single space "
                          "or tab and an operand\n");
            }

            *prnlnbuf = 0;
            *prnlnbufbeg = 0;
            if ((pass == 2) && *outflgs) {
                if (strcmp(opcode, "MACRO") && strcmp(opcode, "MEND")
                                        && !macroflg) {   /* Not in macro */
                    if ((maclevel == 0) || !(strchr(outflgs, 'M') || strchr(outflgs, 'm')))     //M = macros not expanded
                    {

                        if (!(strstr("EQU EPZ ORG", opcode)
                              && (strlen(opcode) == 3))) {
                            if (pc != pc_sav)
                                sprintf(prnlnbufbeg, "%04X: ", pc); // PC ADDR
                            else
                                sprintf(prnlnbufbeg, "    : ");
                            pc_sav = pc;
                        } else
                            sprintf(prnlnbufbeg, "      ");
                        if (*linbuf[maclevel] == '\t') {
                            strcat(prnlnbuf, "\t");
                            for (i = 0; i < maclevel; i++)
                                strcat(prnlnbuf, "| "); /* INDENT */
                            strcat(prnlnbuf, linbuf[maclevel] + 1);
                        } else {
                            for (i = 0; i < maclevel; i++)
                                strcat(prnlnbuf, "| "); /* INDENT */
                            /*add instruction line */
                            strcat(prnlnbuf, linbuf[maclevel]);
                        }
                    }
                }
            }

/* LABEL AT LINE BEGIN */

            if (*label) {
                if (pass == 1) {
                    if (symnum >= MAXSYMS - 1)
                        fatal("too many labels\n");
                    if (macnum >= MAXMACSYMS - 1)
                        fatal("too many labels\n");
                }

/* EPZ EQU  LABEL AT LINE BEGIN*/
                if ((strcmp(opcode, "EQU") == 0)
                    || (strcmp(opcode, "EPZ") == 0)) {
                    txtptr++;
                    i = expresstoi();
                    if (pass == 1) {
                        if (i != NOTFOUND) {
                            strcpy(sss[symnum].symbol, label);
                            sss[symnum++].symadr = i;
                            sss[symnum - 1].symtyp = '#';
                        }
                    }
                    else if( i != NOTFOUND && symtoi3(label) == NOTFOUND )
                    {
                        // Add symbol in second pass
                        strcpy(sss[symnum].symbol, label);
                        sss[symnum++].symadr = i;
                        sss[symnum - 1].symtyp = '#';
                        sort_symbols();
                    }
                    continue;
                }

/* RMB  LABEL AT LINE BEGIN */ //(Init ving(?) memory bytes)
                if (strcmp(opcode, "RMB") == 0) {
                    if (!rmbflg)
                        fatal("INITRMB value must precede this line\n");

                    if (pass == 1) {
                        strcpy(sss[symnum].symbol, label);
                        sss[symnum++].symadr = rmbadr;
                        sss[symnum - 1].symtyp = '#';
                    }
                    txtptr++;
                    rmbadr += expresstoi_p2();

                    continue;
                }

/* MACRO  LABEL AT LINE BEGIN*/
                if (strcmp(opcode, "MACRO") == 0)       // MovAl   MACRO A,N
                {
                    if (macroflg)
                        fatal("MEND missing\n");
                    macroflg++;
                    if (pass == 1) {
                        find_operand(); // A,N
                        operptr = operand;
                        dumnum = 0;
                        do {
                            pos = 0;
                            while ((*operptr != ',') && (*operptr != 0))
                                macdumtab[dumnum][pos++] = *operptr++;
                            macdumtab[dumnum][pos] = 0;
                            dumnum++;
                        } while (*operptr++);

                        strcpy(macsymbol[macnum], label);
                        if (*(txtptr - 1) == '\n')
                            txtptr--;
                        macptrtab[macnum++] = strchr(txtptr, '\n') + 1;
                    }
                    continue;
                }

/*  LABEL AT LINE BEGIN IN CODE */
                else {
                    labflg = 1;
                    if (strcmp(label, "DO") == 0)
                        sprintf(label, "DO%4.4i", ++donum);
                    if (strcmp(label, "LOOP") == 0)
                        sprintf(label, "LOOP%4.4i", ++loopnum);
                    if (strcmp(label, "MINUS") == 0)
                        sprintf(label, "MINU%4.4i", ++minusnum);
                    if (strcmp(label, "PLUS") == 0)
                        sprintf(label, "PLUS%4.4i", plusnum++);
                    if (strcmp(label, "SKIP") == 0)
                        sprintf(label, "SKIP%4.4i", skipnum++);
                    if (strcmp(label, "CONT") == 0)
                        sprintf(label, "CONT%4.4i", contnum++);
                    if (strcmp(label, "ENDIF") == 0)
                        sprintf(label, "ENDI%4.4i", endifnum++);
                }
                if ((pass == 1) && !(macroflg)) {
                    strcpy(sss[symnum].symbol, label);
                    sss[symnum++].symadr = pc;
                }
            }

            /*END LABEL AT LINE BEGIN */
            /* MEND */
            if (strcmp(opcode, "MEND") == 0) {
                if (macroflg)
                    macroflg--;
                else {
                    if (!maclevel)
                        fatal("MEND without MACRO\n");
                    lclbnum[maclevel] = 0;
                    linbegptr = linbegsav[--maclevel];
                    linlen = linlensav[maclevel];
                }
                continue;
            }

/* MACROFLG */
            if (macroflg) {
                if (pass == 1) {
                    find_operand();
                    operptr = operand;
                    if (*operand) {
                        if (*operand == '(')
                            operptr++;
                        operptr--;
                        txtptr--;
                        do {
                            operptr++;
                            txtptr++;

                            pos = 0;
                            while (isalnum(*operptr)) {
                                dumbuf[pos++] = *operptr++; // dubuf == operand
                                txtptr++;
                            }
                            dumbuf[pos] = 0;
                            for (cnt = 0; cnt < dumnum; cnt++) {
                                if (strcmp(macdumtab[cnt], dumbuf) == 0) {
                                    while (pos)
                                        *(txtptr - pos--) = (char)cnt | 128;
                                    break;      /* out of for loop */
                                }
                            }
                        } while (*operptr);
                    }
                }
                continue;
            }

/* INCLUDE */
            if (strcmp(opcode, "INCLUDE") == 0) {
                if (include_flg)
                    fatal("can't have 'INCLUDE' in an included file\n");
                include_flg = 1;
                operand_to_filename(include_filename);

                info("including %s\n", include_filename);

                if ((stream3 = fopen(include_filename, "r")) == NULL)
                    fatal("can't open INCLUDE file\n");
                includebuf = txtendptr;
                include_txtsiz =
                    fread(includebuf, 1, MAX_TXT_SIZ - txtsiz, stream3);
                fclose(stream3);

                txtsiz += include_txtsiz;
                if (txtsiz > MAX_TXT_SIZ - txtsiz)
                    fatal("INCLUDE File is too long\n");

                fprintf(stderr, ": %i bytes\n", include_txtsiz);

                txtendptr = includebuf + include_txtsiz;

                *txtendptr++ = '\n';
                *txtendptr++ = '\0';
                txtptr_sav = linbegptr + linlen;
                linnum_sav = linnum;

                linbegptr = includebuf;
                linlen = 0;
                linnum = 0;

                continue;
            }

/* NOHEADER */
            if ((strcmp(opcode, "NOHEADER") == 0)) {
                objfile_flag = 1;
                continue;
            }

/* OUT */
            if ((strcmp(opcode, "OUT") == 0)) {
                if (pass == 2) {
                    find_operand();
                    strcpy(outflgs, operand);

                    if (*outflgs && !lstflg) {
                        lstflg++;
                        if ((stream2 = fopen(lstfilename, "w")) == 0) {
                            fatal("can't open %s\n", lstfilename);
                        }
                    }
                }
                continue;
            }

/* INITRMB */
            if (strcmp(opcode, "INITRMB") == 0) {
                txtptr++;
                rmbadr = expresstoi_p2();
                rmbflg = 1;
                continue;
            }

/* ORG */
            if (strcmp(opcode, "ORG") == 0) {
                txtptr++;
                {
                    if (!orgflg) {      /* first ORG */
                        if (*txtptr == '*')
                            fatal("ORG value is undefined\n");
                        orgflg = 1;
                    } else {
                        info("segment end: $%04X\n", pc - 1);  /*write NDAD */
                        *sgndtbptr1++ = pc - 1;
                    }


                    pc = expresstoi();

                    if (!objfile_flag) {
                        pcsv = pc;
                        toxlbuf(255);
                        toxlbuf(255);
                        toxlbuf((char)pcsv);
                        toxlbuf((char)(pcsv / 256));
                        toxlbuf((char)*sgndtbptr2);
                        toxlbuf((char)((*sgndtbptr2++) / 256));
                        pc = pcsv;
                    }
                    info("segment start: $%04X\n", pc);
                }
                continue;
            }

/* DFB */
            if (strcmp(opcode, "DFB") == 0) {
                data_flg = 1;
                if (*(txtptr + 1) == '%')
                    sectyp('%');
                else
                    sectyp('#');
                do {
                    txtptr++;
                    toxlbuf((char)expresstoi_p2());
                } while (*txtptr == ',');
                continue;
            }

/* DFW */
            if (strcmp(opcode, "DFW") == 0) {
                data_flg = 1;
                sectyp('=');
                do {
                    txtptr++;
                    symval = expresstoi_p2();
                    toxlbuf((char)symval);
                    toxlbuf((char)(symval >> 8));
                } while (*txtptr == ',');
                continue;
            }

/* FILLMEM */
            if (strcmp(opcode, "FILLMEM") == 0) {
                data_flg = 1;
                if (*(txtptr + 1) == '\'')
                    sectyp('"');
                else
                    sectyp('#');
                txtptr++;
                cnt = expresstoi_p2();
                if (*txtptr != ',')
                    fatal("comma followed by byte value expected\n");
                txtptr++;
                ch = (char)expresstoi_p2();
                for (i = 0; i < cnt; i++)
                    toxlbuf(ch);
                continue;
            }

/* ASC */
            if (strcmp(opcode, "ASC") == 0) {
                data_flg = 1;
                do {
                    txtptr++;
                    switch (*txtptr++) {
                    case '%':  /* screen code  */
                        sectyp('$');
                        while ((*txtptr != '%') && (*txtptr != '\n')) {
                            offset = *txtptr >> 5;
                            ch = *txtptr++ & 0x1f;
                            ch |= *(intab + offset);
                            toxlbuf(ch);
                        }
                        break;
                    case '$':  /* screen code, end inverse  */
                        sectyp('$');
                        while ((*txtptr != '$') && (*txtptr != '\n')) {
                            offset = *txtptr >> 5;
                            ch = *txtptr++ & 0x1f;
                            ch |= *(intab + offset);
                            toxlbuf((char)(ch | 128));
                        }
                        break;
                    case '/':  /* inverse ASC */
                        sectyp('"');
                        while ((*txtptr != '/') && (*txtptr != '\n'))
                            toxlbuf((char)(128 | *txtptr++));
                        break;
                    case '\\': /* ASC, end inverse  */
                        sectyp('\\');
                        for (;;) {
                            ch = *txtptr++;
                            if ((*txtptr == '\\') || (*txtptr == '\n'))
                                break;
                            toxlbuf(ch);
                        }
                        toxlbuf((char)(128 | ch));
                        break;
                    default:   /* normal ASC: can use any other non alnum
                                  character */
                        sectyp('"');
                        if (!isalnum(*(txtptr - 1))) {
                            ch = *(txtptr - 1);
                            while ((*txtptr != ch) && (*txtptr != '\n'))
                                toxlbuf(*txtptr++);
                        } else
                            fatal("invalid string marking\n");
                    }
                } while (*(++txtptr) == ',');
                continue;
            }

/* Find operand */
            find_operand();

/* OPCODE IS A MACRO */
            for (i = 0; i < macnum; i++)
                if (strcmp(macsymbol[i], opcode) == 0)
                    break;
            if (i < macnum) {
                linbegsav[maclevel] = linbegptr;
                linlensav[maclevel] = linlen;
                maclevel++;
                macargsptr[maclevel] = txtptr;
                linbegptr = macptrtab[i];
                if ( *(txtptr - 1) == '(' )
                    macargsptr[maclevel]--;
                linlen = 0;
                continue;
            }

/* OPC without operand */
            cnt = 0;
            if (!(*operand)) {
                if ((cnt = chkopc(opcdat1)) >= 0) {     /* rts, sec, plp, ...... */
                    sectyp(';');
                    toxlbuf((char)*(dat1 + cnt));
                    continue;
                } else {
                    if (strlen(opcode) == 3)
                        if (strstr(opcds, opcode))
                            fatal("opcode must be followed by a single space "
                                  "(not tab) and an operand\n");
                    strcpy(operand, opcode);
                    strcpy(opcode, "JSR");
                    txtptr = tpsav;
                }
            }


/* JMP */
            if (strcmp(opcode, "JMP") == 0) {
                sectyp(';');
                if (*operand == '(') {
                    if (operand[strlen(operand) - 1] != ')')
                        fatal("illegal syntax or addressing\n");
                    toxlbuf(108);
                } else
                    toxlbuf(76);

                operval = expresstoi_p2();


                toxlbuf((char)operval);
                toxlbuf((char)(operval >> 8));
                continue;
            }

/* BRA */
            if ((cnt = chkopc(brastrdat)) >= 0) {
                sectyp(';');
                toxlbuf((char)bradat[cnt]);
                braoffset = temp = expresstoi_p2() - (pc + 1);
                if (pass == 2) {
                    if (temp < -128 || temp > 127)
                        fatal("branch out of range\n");
                }
                toxlbuf((char)braoffset);
                continue;
            }

/* Subroutine */
            if (strcmp(opcode, "JSR") != 0) {
                if ((cnt = chkopc(opcdat)) < 0) {
                    /* opcode not found, must be subroutine */
                    if ((strcmp(opcode, "MACRO") == 0) ||
                        (strcmp(opcode, "EPZ") == 0) ||
                        (strcmp(opcode, "EQU") == 0))
                        fatal("label missing at beginning of line\n");
                        // since otherwise these would have been taken care of

                    fatal("subroutine can't have an operand\n");
                }

            }

/* JSR */
            if (!strcmp(opcode, "JSR")) {
                if (strchr(";'!(", section_type) == 0)
                    sectyp(';');        //version 1.01
                symval = expresstoi_p2();
                toxlbuf(0x20);
                toxlbuf((char)symval);
                toxlbuf((char)(symval >> 8));
                continue;
            }

/* opc 'c */
            if (operand[0] == '\'') {
                sectyp('\'');
                if (data[cnt][NN] == 0)
                    fatal("direct addressing (') cannot be used with "
                          "this opcode\n");
                txtptr++;
                ch = *txtptr++;
                if (*txtptr == '\'')
                    txtptr++;
                toxlbuf((char)data[cnt][NN]);
                toxlbuf(ch);
                continue;

            }

/* opc #nn */
            if (operand[0] == '#') {
                if (data[cnt][NN] == 0)
                    fatal("direct addressing (#) cannot be used with "
                          "this opcode\n");

                txtptr++;
                intval = expresstoi_p2();

                if (operand[1] == '%')  /* e.g. LDA #%11000000 */
                    sectyp('!');
                else {
                    lo_fg = 1;
                    if (*txtptr == ':') {       /* e.g. LDA #OKTXT:H */
                        txtptr++;
                        if ((*txtptr == 'H') || (*txtptr == 'h')) {
                            intval >>= 8;
                            lo_fg = 0;  /* ok */
                        }
                    }
                    if ((symfg || (intval == NOTFOUND)) && lo_fg) {
                        if (intval > 255)
                            sectyp('&');
                        else    /* zero page */
                            sectyp('(');
                    }
                }
                if (strchr("!&@(", section_type) == 0)
                    sectyp(';');
                toxlbuf((char)data[cnt][NN]);
                toxlbuf((char)intval);
                continue;
            }

/***********  Commands in data block  ***************/

            operno = ERROR;

            sectyp(';');

            symval = expresstoi();

            if ((symval < 256) && (double_byte_flg == 0)) {
                // txtptr -> first byte after expression
                if ((*(txtptr - 1) == ')') && (*operand == '('))
                    txtptr--;   //needed for expresstoi(new)

                switch (*txtptr) {
                case ' ':
                case '\t':
                case '\n':
                    operno = AA;
                    break;      //OPC without operand
                case ',':
                    switch (*(++txtptr)) {
                    case 'Y':  //   ,Y
                    case 'y':
                        if (*(opcode + 2) == 'Y')
                            fatal("can't have ,Y with this opcode\n");
                        if (*(opcode + 2) == 'X')       //  OPX aa,Y
                            operno = AAY;
                        else
                            operno = AAAAY;     //  OPA $0059,Y
                        break;
                    case 'X':  //   ,X
                    case 'x':
                        if (*(opcode + 2) == 'X')
                          fatal("Can't have X after comma with this opcode\n");
                        switch (*(++txtptr)) {
                        case ')':      //  OPC (aa,X)
                            if (*operand != '(')
                                fatal("opening bracket missing\n");
                            operno = BAAXB;
                            break;
                        default:
                            operno = AAX;
                            break;      // OPC aa,X
                        }
                        break;
                    default:
                        fatal("must have X or Y after comma\n");
                        break;
                    }
                    break;
                case ')':
                    if (*operand != '(')
                        fatal("opening bracket missing\n");
                    switch (*(++txtptr)) {
                    case ',':
                        switch (*(++txtptr)) {
                        case 'Y':
                        case 'y':
                            if ((*(opcode + 2) == 'Y')
                                || (*(opcode + 2) == 'X'))
                                fatal("can't use indirect addressing with "
                                      "this opcode\n");

                            operno = BAABY;
                            break;      // OPA (AA),Y
                        default:
                            fatal("must have Y after comma for indirect "
                                  "addressing\n");
                            break;
                        }
                        break;
                    default:
                        fatal("comma missing after bracket\n");
                        break;
                    }
                    break;
                default:
                    fatal("symbol contains illegal character '%c'\n", *txtptr);
                    break;
                }

            } else {              /* symval >= 256 */

                if (*operand == '(')
                    fatal("zero page address expected -- operand not "
                          "defined -- label may be unknown\n");

                switch (*txtptr) {
                case ' ':
                case '\t':
                case '\n':
                case 13:
                    operno = AAAA;
                    break;
                case ',':
                    switch (*(++txtptr)) {
                    case 'Y':
                    case 'y':
                        if (*(opcode + 2) == 'Y')
                            fatal("can't have ,Y with this opcode\n");
                        operno = AAAAY;
                        break;
                    case 'X':
                    case 'x':
                        if (*(opcode + 2) == 'X')
                            fatal("can't have ,X with this opcode\n");
                        operno = AAAAX;
                        break;
                    default:
                        fatal("must have X or Y after ','\n");
                        break;
                    }
                    break;
                default:
                    fatal("illegal operand\n");
                    break;
                }
            }

            if (operno == ERROR)
                fatal("illegal addressing mode\n");
            if (pass == 2)
                if (*xlbfptr != (char)data[cnt][operno]) {
                    fatal("opcode mismatch %X <> %X. "
                          "this zero-page label must be defined "
                          "before the current line.\n",
                          0xff & (*xlbfptr), 0xff & data[cnt][operno]);
                }
            if (data[cnt][operno] == 0)
                fatal("this addressing is not possible with this opcode\n");
            toxlbuf((char)data[cnt][operno]);   // opcode command
            toxlbuf((char)symval);      // address low byte

            switch (operno)     // now address high byte (if applicable)
            {
            case AAAA:
            case AAAAX:
            case AAAAY:
                toxlbuf((char)(symval >> 8));
                break;
            }
        }                       // end toxlbuf
        info("segment end: $%04X\n", pc - 1);

        if (pass == 1) {
            info("checking symbols...\n");

            for (i = 0; i < macnum; i++) {
                if (symnum >= MAXSYMS - 1)
                    fatal("too many labels\n");
                if (macptrtab[i])
                    sss[symnum].macf = MACRO;
                else
                    sss[symnum].macf = MACRO_UNUSED;
                strcpy(sss[symnum++].symbol, macsymbol[i]);

            }
            sort_symbols();
        }

        *sgndtbptr1 = pc - 1;

    } // END PASSES END PASSES END PASSES

    qsort(sss, symnumext, sizeof(ssstype), (int (*)(const void *, const void *)) ssscmpadr);

    /* Symbol table */
    if (strchr(outflgs, 'N') || strchr(outflgs, 'n')) {
        fprintf(stream2, "\n\n**** Symbol table *****\n\n");
        for (i = 0; i < symnumext; i++) {
            if (sss[i].symbol)
                list_label_f(i);
        }
    }

    if (lstflg)
        fclose(stream2);

    symnum = 0;
    for (i = 0; i < symnumext; i++) {
        if (sss[i].macf == MACRO)
            continue;
        if (((sss[i].macf == UNUSED) || (sss[i].macf == MACRO_UNUSED))
            && sss[i].symbol[0])
            info("unused symbol: %s\n", sss[i].symbol);
        if ((sss[i].symadr == sss[symnum - 1].symadr) && (symnum > 0)) {
            /* double entry */
            if (sss[symnum - 1].symbol[0] && sss[i].symbol[0]) {
                /* 2 labels */
                if ((sss[symnum - 1].macf == MACRO_LAB)
                    || (sss[i].macf == MACRO_LAB)) {
                    if (sss[symnum - 1].macf == MACRO_LAB)
                        strcpy(sss[symnum - 1].symbol, sss[i].symbol);
                    continue;
                } else
                    info("%s and %s both = $%X\n", sss[i].symbol,
                           sss[symnum - 1].symbol, sss[i].symadr);
            }
            if (!(sss[symnum - 1].symbol[0]))
                strcpy(sss[symnum - 1].symbol, sss[i].symbol);
            if (sss[i].symtyp)
                sss[symnum - 1].symtyp = sss[i].symtyp;
            continue;
        }

        memmove(sss[symnum++].symbol, sss[i].symbol, sizeof(ssstype));
    }

    fclose(stream);

    if (!((stream = fopen(xexfilename, "wb"))))
        fatal("can't open %s\n", xexfilename);

    info("saving binary file %s\n", xexfilename);
    txtsiz = fwrite(xlbuf, 1, xlbfptr - xlbuf, stream);
    if (txtsiz != xlbfptr - xlbuf)
        fatal("can't save %s\n", xexfilename);

    info("file size: %d bytes\n", txtsiz);
    fclose(stream);
}
