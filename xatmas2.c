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

#define getch getchar
#define fclose(x) if (x) { fclose(x); x=NULL; }

typedef struct {                /* Define the structure ssstype */
    char symbol[9];
    unsigned int symadr;
    char ofst;
    char macf;
    char symtyp;
} ssstype;

#define MAX_BIN_SIZ 0x1ffffL
#define MAXSYMS 2000
#define MAXMACSYMS 200
#define MAXLOCLABS 50
#define MXLBS 10                /* maximum # of labels in present macro */
#define MAXMACLEV 10            /* maximum # ofnesting levels in macros */
#define NOTFOUND 0xeffff

#define MAX_TXT_SIZ 0xfffffL
enum { OPC, NN, AA, AAAA, AAX, AAAAX, AAY, AAAAY, BAAXB, BAABY, BRA, BAAAAB,
    ERROR
};
enum { UNUSED, SYMBOL, MACRO, MACRO_UNUSED, MACRO_LAB };

#define MAXDUMS 20
#define LSTBYTES 10

char opcdat[] =
    "BITSTYLDYCPYCPXORAANDEORADCSTALDACMPSBCASLROLLSRRORSTXLDXDECINC";

unsigned short data[21][15] = {
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
short cpfflg;
short lstflg = 0;
short include_flg;
short mismatch_flg;
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
short bytcnt = 0;
short prnlnum = 0;
char src_filename[300];

ssstype sss[MAXSYMS];

int ssscmpadr(const ssstype * dummy1, const ssstype * dummy2);

FILE *stream;
FILE *stream2;
FILE *stream3;
fpos_t pos2;

static const char opcdat1[] = "BRKPHPASLCLCPLPROLSECRTIPHALSRCLIRTSPLARORSEI"
        "DEYTXATYATXSTAYTAXCLVTSXINYDEXCLDINXNOPSED";
static const int dat1[] = { 0, 8, 10, 24, 40, 42, 56, 64, 72, 74, 88, 96, 104, 106,
    120, 136, 138, 152, 154, 168, 170, 184, 186, 200, 202, 216, 232, 234, 248
    };

static const char opcds[] = " BIT STY LDY CPY CPX ORA AND EOR ADC STA LDA CMP SBC "
    "ASL ROL LSR ROR STX LDX DEC INC BRK PHP ASL CLC PLP ROL SEC RTI PHA LSR "
    "CLI RTS PLA ROR SEI DEY TXA TYA TXS TAY TAX CLV TSX INY DEX CLD INX NOP "
    "SED BPL BMI BVC BVS BCC BCS BNE BEQ JMP JSR ";     // for checking labels

static const char assdirecs[] =
        " EQU EPZ ORG ASC DFB DFW ORG RMB INITRMB "
        " FILLMEM INCLUDE ";
static const char assdirecssolo[] = " OUT NOHEADER OBJFILE ";

static const char brastrdat[] = "BPLBMIBVCBVSBCCBCSBNEBEQ";
static const unsigned short bradat[] =
        { 0x10, 0x30, 0x50, 0x70, 0x90, 0xb0, 0xd0, 0xf0 };
static const int intab[] = { 0x40, 0, 0x20, 0x60, 0xA0, 0xC0, 0x80, 0xE0 };

// ----------------------------------------------------------------------------

void error(char *errstr) {
    int len;
    char partstr[100];

    printf("\n\nERROR --\n%s\nat ", errstr);
    if (pc)
        printf("address $%X, ", pc);
    printf("line %d\n\n\a", linnum);
    printf("%s", linbuf[0]);

    if (maclevel == 0) {
        len = txtptr - linbegptr;
        strcpy(partstr, linbuf[0]);
        partstr[len] = '\0';
        printf("%s", partstr);
    } else {
        printf("Error in macro:\n%s", linbuf[maclevel]);
    }
    exit(linnum);
}

// ----------------------------------------------------------------------------

int prec(char symbol) {
    if (symbol == '(')
        return 0;
    if (symbol == ')')
        return 0;
    if (symbol == '+' || symbol == '-')
        return 1;
    if (symbol == '*' || symbol == '/')
        return 2;
    return 0;
}

// ----------------------------------------------------------------------------

unsigned int symtoi3(char *strbeg) {
    int i, j;

    if (symnum == 0)
        return (NOTFOUND);
    i = 0;
    while (i < symnum) {
        j = 0;
        while (sss[i].symbol[j] == strbeg[j]) {
            if ((!strbeg[j] && !sss[i].symbol[j]))
                return (sss[i].symadr);
            j++;
        }
        i++;
    }
    return (NOTFOUND);
}

// ----------------------------------------------------------------------------

/*********** symtoi1()
 * converts symbol or number to integer (return value begins at txtptr.
 * When done txtptr is the address directly after the location of symbol.
 * If evaluation is not possible,the value returned is NOTFOUND
 */
unsigned int symtoi1() {
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
            error("Binary digit 0 or 1 expected.");
        ch = 0;
        chrcnt = 1;
        while (*txtptr == '0' || *txtptr == '1') {
            if (chrcnt++ > 8)
                error
                    ("Too many binary digits. Maximum number of digits is 8. e.g. %10010011");
            ch <<= 1;
            ch += *txtptr++ - '0';
        }
        return (ch);
    }
    if (ch == '$') {            /*  $:Hex */
        txtptr++;
        if (isxdigit(*txtptr) == 0)
            error("Hexidecimal digit expected");
        chrcnt = 1;
        while (isxdigit(*txtptr)) {
            if (chrcnt++ > 4)
                error
                    ("Too many hexdecimal digits. Maximum number of digits is 4 eg. $1234.");
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
            error("Ilegal space, tab, or end of line");
        if (!(isalpha(*txtptr)))
            error("Label must begin with a letter");

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
                if (*(txtptr + 2) == '+')       //These two lines take care
                    sssptr->ofst = sssptr->ofst + *(txtptr + 3) - '0';  //of hidden labels 
            }
            if (*txtptr == '-')
                sssptr->ofst = (char)(256 - (*(txtptr + 1) - '0'));

            symfg = 1;
            return (sssptr->symadr);
        } else {
            error("label undefined");
        }
    }
//if (pass == 2) error("Label undefined"); //probably don't need this
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
// interpret '*'  *******


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
        if (!strchr("+-*/()", infix[i]))        //Numbers are added to the postfix string
            postfix[j++] = infix[i];

        else                    //If an operator or a bracket is encountered...
        {
            if (tos == 0)       //If the stack is empty, the operator is added to it

                stack[tos++] = infix[i];

            else                //stack not empty
            {                   //Operators are pushed or popped based on the order of precedence
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
                                error("Missing opening parenthisis");
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
                if (*ptr == '(')
                    error("Missing closing parenthisis");
                printf("infix: %s   -Character '%c' in %s  operand is %s",
                       infix, *ptr, postfix, operand);
                error("Illegal character in postfix expression");
                break;
            }
        }
        ptr++;
    }

    if (strstr(opcode, "),Y"))
        txtptr--;
    result = intstack[--tos];

    if (tos) {
        printf("\n\noperand: %s   infix: %s   postfix: %s", operand, infix,
               postfix);
        error("Illegal syntax. Can't resolve expression");
    }

    if (notfound)
        result = NOTFOUND;
    return (result);
}

// ----------------------------------------------------------------------------

unsigned int expresstoi_p2() {
    unsigned int val;

    find_symad_flg = 0;
    val = expresstoi();
    find_symad_flg++;
    return (val);
}

// ----------------------------------------------------------------------------

char fptsmb(int i) {
    char *lnbfptr;
    int cnt;
    char len;

    if ((sss[i].symadr == sss[i - 1].symadr) && (i > 0)) {      /* double entry */
        if (sss[i - 1].symbol[0])
            strcpy(sss[i].symbol, sss[i - 1].symbol);
        if (sss[i - 1].symtyp)
            sss[i].symtyp = sss[i - 1].symtyp;
        secsiz -= lensv;
        fsetpos(stream, &pos2);
    }
    lnbfptr = linbuf[0];
    cnt = 0;
    if (sss[i].macf == MACRO)
        return (0);
    while (sss[i].symbol[cnt])
        *lnbfptr++ = sss[i].symbol[cnt++];
    if (sss[i].symtyp) {
        *lnbfptr = sss[i].symtyp;
        cnt++;
    }
    len = cnt + 4;
    fgetpos(stream, &pos2);
    fputc((char)sss[i].symadr, stream);
    fputc((char)((sss[i].symadr) >> 8), stream);
    fputc(sss[i].ofst, stream);
    fputc(len, stream);
    lnbfptr = linbuf[0];
    while (cnt) {
        fputc(*lnbfptr++, stream);
        cnt--;
    }
    return (len);
}

// ----------------------------------------------------------------------------

void sectyp(char line_type) {
    if (!labflg && (line_type == ';') && strchr("!&'", section_type))
        line_type = section_type;


    if ((line_type != section_type) || labflg)  // if new sec type or a label
    {
        section_type = line_type;
        if (labflg)             //if there is a label....
        {
            if (pass == 1)
                /**/ {
                if (line_type == ';')   //with normal code (;)..
                    sss[symnum - 1].symtyp = 0; // label is enough, so no entry
                else
                    sss[symnum - 1].symtyp = line_type; // else make entry
                }
            if ((pass == 2) && (line_type == '&'))
                /**/ {
                sss[symnum - 1].symtyp = line_type;
                }


        } else /* no label */ if ((pass == 2) && (pc > 0xff)) {
            if (symnumext >= MAXSYMS)
                error("Too many symbols");
            sss[symnumext].symtyp = line_type;
            sss[symnumext++].symadr = pc;
        }
    }
}


// ----------------------------------------------------------------------------

int ssscmpadr(const ssstype * a1, const ssstype * a2) {
    return (a1->symadr - a2->symadr);
}

// ----------------------------------------------------------------------------

void list_label(int i) {
    printf("%s\t", sss[i].symbol);
    if (sss[i].macf == MACRO)
        printf("MACRO\n");
    else
        printf(" = %X\n", sss[i].symadr);
}

// ----------------------------------------------------------------------------

void list_label_f(int i) {
    fprintf(stream2, "%s\t", sss[i].symbol);
    if (sss[i].macf == MACRO)
        fprintf(stream2, "MACRO\n");
    else
        fprintf(stream2, " = %X\n", sss[i].symadr);
}

// ----------------------------------------------------------------------------

void operand_to_filename(char filename[]) {

    int cnt = 0;
    char *linptr;

    switch (*txtptr++) {
    case '\n':
    case '\t':
        error("Filename missing");
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
            error("Filename missing");
        break;
    }
    operand[cnt] = 0;
    strcpy(filename, operand);
}

// ----------------------------------------------------------------------------

void find_operand(void) {
    int cnt = 0;
    char *linptr;

    switch (*txtptr++) {
    case '\n':
    case '\t':
        break;
    case ' ':
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

void toxlbuf(char ch) {
    char tempstr[10];

    if (!orgflg)
        error("ORG value must precede this line");

    *xlbfptr = ch;

    xlbfptr++;
    pc++;

    if (pass == 2) {
        bytcnt++;
        if (cpfflg) {
            bin = fgetc(stream);
            if (bin != (ch & 255)) {
                if (mismatch_flg == 0) {
                    mismatch_flg = 1;
                    mismatch_linnum = linnum;
                    mismatch_pc = pc;
                }
                if (mismatch_flg == 1) {
                    if ((bytcnt == 1) && (data_flg == 0)) {
                        mismatch_flg = 2;
                        mismatch_linnum = linnum;
                        mismatch_pc = pc;
                    }
                }
            }

        }
        if ((strchr(outflgs, 'L') || strchr(outflgs, 'l'))
            && !(strchr(outflgs, 'M') || strchr(outflgs, 'm'))) {

            sprintf(tempstr, "%02X ", ch & 255);
            if (bytcnt <= LSTBYTES)
                strcat(prnlnbufbeg, tempstr);
            else {
                if (bytcnt % LSTBYTES == 1) {
                    prnlnum++;
                    sprintf(prnlnbuf2[prnlnum - 1], "%4X: ", pc - 1);
                }
                strcat(prnlnbuf2[prnlnum - 1], tempstr);
            }
            if (cpfflg) {
                if (bytcnt % LSTBYTES == 1) {
                    if (bin != (ch & 255))
                        sprintf(cmpflbytln[prnlnum], "******");
                    else
                        sprintf(cmpflbytln[prnlnum], "<><><>");
                }
                if (bin != (ch & 255)) {
                    mismatchln_flg[prnlnum] = 1;
                    sprintf(tempstr, "%02X)", bin);
                } else
                    sprintf(tempstr, "%02X ", bin);
                strcat(cmpflbytln[prnlnum], tempstr);
            }
        }
    }
}

// ----------------------------------------------------------------------------

/********* chkopc    returns the position (0, 1, 2,.. ) of opc in string
						returns -1 if not found */
int chkopc(const char opcstring[]) {
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

int main(int argc, char *argv[]) {
    int braoffset;

    char *includebuf;
    char *txtendptr;            /* pointer to end of text Buffer (=10) */
    char xlbuf[MAX_BIN_SIZ];
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
    int dumnum, macroflg;

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

    char prnlnbuf[120];
    short twiceflg = 0;

    char include_filename[300];

    char loafilename[300];
    char lstfilename[300];
    char filnmtmp[300];
    short lo_fg;
    short objfile_flag = 0;
    short rmbflg = 0;
    int rmbadr;
    char *txtbuf;

    txtbuf = (char *)malloc(MAX_TXT_SIZ); /* malloc   begin of buffer file is loaded into */
    if (txtbuf == NULL) {
        printf("Can't allocate memory for text buffer.\a\n");
        exit(1);
    }

    puts("xatmas2 v2.6.2\n"
         "Copyright (c) 2004-2011 by Richard Jackman\n"
         "Copyright (c) 2018 by Ivo van Poorten\n");

    if (argc > 1)
        strcpy(src_filename, argv[1]);
    else {
        fprintf(stderr, "%s: usage: codegen <file.src>\n", argv[0]);
        exit(1);
    }

    if ((stream = fopen(src_filename, "r")) == NULL) {
        fprintf(stderr, "%s: Can't open %s\n", argv[0], src_filename);
        exit(1);
    }

    txtsiz = fread(txtbuf, 1, MAX_TXT_SIZ, stream);     // read SRC file into buffer
    fclose(stream);

    if (txtsiz == MAX_TXT_SIZ) {
        puts("File is too long");
        exit(1);
    }
    printf("%i bytes\n", txtsiz);


    strcpy(lstfilename, src_filename);
    strcpy(strchr(lstfilename, '.'), ".LIS");

    strcpy(loafilename, "AUTORUN.SYS");

/********************** Now work on buffer *********************/

    txtendptr = txtbuf + txtsiz;

    symnum = 0;
    *outflgs = 0;
    cpfflg = 0;
    mismatch_flg = 0;
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
    *txtendptr++ = '\n';
    *txtendptr++ = '\n';
    *txtendptr++ = '\0';

/********************** Main loop *******************/

    for (pass = 1; pass <= 2; pass++)
    {
        sgndtbptr1 = sgndtbptr2 = sgndtb;
        printf("\n  pass %d\n", pass);
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
                    if (cpfflg)
                        if (mismatchln_flg[0])
                            fprintf(stream2, "%s\n", cmpflbytln[0]);

                    if (bytcnt > LSTBYTES) {
                        for (i = 1; i <= prnlnum; i++) {
                            fprintf(stream2, "%s\n", prnlnbuf2[i - 1]);
                            if (cpfflg)
                                if (mismatchln_flg[i])
                                    fprintf(stream2, "%s\n", cmpflbytln[i]);
                        }
                    }
                    //fprintf (stream2,"%s\n",linbuf[maclevel]); ////
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
            linlen = (short)(strchr(linbegptr, '\n') + 1L - linbegptr); //find linlen

            if (!maclevel && pass == 1) {       /*Test line for illegal characters */
                for (cnt = 1; cnt <= linlen; cnt++) {
                    if (*linptr < 0) {
                        linbegptr[linlen] = 0;
                        printf("\a\nIllegal character at line %i:\n", linnum);
                        puts(linbegptr);
                        exit(1);
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
                    while (isalnum(*ptr)) {     /* find beginning of local label */
                        ptr--;
                        cnt++;
                    }
                    if (strchr(", \n+-", *ptr)) {       /*Characters which can precede a local label!!!!!!! */
                        if (cnt > 4)
                            error
                                ("Local labels cannot be longer than 5 characters");
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
                        if (loclabtbpos == loclabnum) { /* label into loclabtb */
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
                        sprintf(lnbfptr, "%04d", labexttab[loclabtbpos]);       /* Add four digits to label */
                        lnbfptr += 4;
                        linptr++;
                    }
                }

                                                        /********** Dummy in macro ***********/


                if ((*linptr & 128) && !macroflg) {     /* inverted characters */
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
                } else /*           *linptr < 128           */ if (*linptr !=
                                                                   '\n')
                    *lnbfptr++ = *linptr++;
            }


            if ((!macroflg) && maclevel)
                txtptr = linbuf[maclevel];

            *lnbfptr++ = '\n';
            *lnbfptr = '\0';


/**********  Test first character of line ***********/

            switch (*txtptr) {
            case '\?':
                ptr = ptr;      /*dummy statement for 'run to cursor' at this line */
            case '\n':         /* empty line   */
            case ';':
            case '*':
                continue;       /* comment              */
            case ' ':          /* white space  */
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
                    printf("%c is not valid", *txtptr);
                    error("label must begin with alphabetic character");
                }
                pos = 0;
                while (isalnum(*txtptr))
                    label[pos++] = (char)toupper(*txtptr++);
                if (pos > 8)
                    error("Label is too long");
                if (*txtptr == '@')
                    label[pos++] = *txtptr++;
                if (!isspace(*txtptr))
                    error
                        ("Label must consist only of alphabetic and numeric characters");
                label[pos] = 0;

                sprintf(labelstr, " %s ", label);
                if (strstr(opcds, labelstr))
                    error
                        ("You can't have an opcode at the beginning of a line");
                if (strstr(assdirecs, labelstr))
                    error
                        ("You can't have an assembler directive the beginning of a line");
                if (strstr(assdirecssolo, labelstr))
                    error
                        ("You can't have an assembler directive the beginning of a line");
            }
            }

            /*      ********* find opcode *************************** */

            while ((*txtptr == ' ') || (*txtptr == '\t'))
                txtptr++;
            tpsav = txtptr;
            if (*txtptr == '\n')
                continue;
            if (isalpha(*txtptr) == 0)
                error("Alphabetic character expected");
            ptr = opcode;

            while (isalnum(*txtptr))
                *ptr++ = (char)toupper(*txtptr++);      //opcode is upper case

            double_byte_flg = 0;

            if (*txtptr == '<') {
                double_byte_flg = 1;
                txtptr++;
            }

            if (!isspace(*txtptr)) {
                printf("Illegal character '%c' in symbol", *txtptr);
                error("Space expected");
            }

            *ptr = 0;           //opcode finalised




            if (!(*txtptr == ' ') || *(txtptr + 1) == ' ' || *(txtptr + 1) == '\n' || *(txtptr + 1) == '\t')    //Check operand for dirictives
            {
                sprintf(opcstr, " %s ", opcode);
                if (strstr(assdirecs, opcstr))
                    error
                        ("Directive must be followed by a single space (not tab) and an operand");
            }

            *prnlnbuf = 0;
            *prnlnbufbeg = 0;
            if ((pass == 2) && *outflgs) {
                if (strcmp(opcode, "MACRO") && strcmp(opcode, "MEND") && !macroflg) {   /* Not in macro */
                    if ((maclevel == 0) || !(strchr(outflgs, 'M') || strchr(outflgs, 'm')))     //M = macros not expanded
                    {

                        if (!(strstr("EQU EPZ ORG", opcode)
                              && (strlen(opcode) == 3))) {
                            if (pc != pc_sav)
                                sprintf(prnlnbufbeg, "%4X: ", pc);      /* PC ADDRESS */
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
                            strcat(prnlnbuf, linbuf[maclevel]); /*add instruction line */
                        }
                    }
                }
            }

/* LABEL AT LINE BEGIN  LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN*/

            if (*label) {
                if (pass == 1) {
                    if (symnum >= MAXSYMS - 1)
                        error("Too many labels");
                    if (macnum >= MAXMACSYMS - 1)
                        error("Too many labels");
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
                    continue;
                }



                /* RMB  LABEL AT LINE BEGIN *///(Init ving memory bytes)

                if (strcmp(opcode, "RMB") == 0) {
                    if (!rmbflg)
                        error("INITRMB value must precede this line");

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
                        error("MEND missing");
                    macroflg++;
                    if (pass == 1) {
                        find_operand(); // A,N
                        operptr = operand;
                        dumnum = 0;
                        do {
                            pos = 0;
                            while ((*operptr != ',') && (*operptr != 0))        // macdumtab[0]="A"
                                macdumtab[dumnum][pos++] = *operptr++;  // macdumtab[1]="B"
                            macdumtab[dumnum][pos] = 0;
                            dumnum++;
                        } while (*operptr++);

                        strcpy(macsymbol[macnum], label);       // macsymbol[macnum]    MovAl
                        if (*(txtptr - 1) == '\n')
                            txtptr--;
                        macptrtab[macnum++] = strchr(txtptr, '\n') + 1; // macptrtab[macnum++]     pointer
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


            /*END LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN LABEL AT LINE BEGIN */
            /* MEND */
            if (strcmp(opcode, "MEND") == 0) {
                if (macroflg)
                    macroflg--;
                else {
                    if (!maclevel)
                        error("MEND without MACRO");
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
                        operptr--;
                        txtptr--;
                        do {
                            operptr++;
                            txtptr++;
                            if (*operand == '(')
                                operptr++;

                            pos = 0;
                            while (isalnum(*operptr)) {
                                dumbuf[pos++] = *operptr++;     // dubuf == operand
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
                    error("Can't have 'INCLUDE' in an included file.");
                include_flg = 1;
                operand_to_filename(include_filename);

                printf("\nIncluding %s", include_filename);

                if ((stream3 = fopen(include_filename, "r")) == NULL)
                    error("Can't open INCLUDE file");
                includebuf = txtendptr;
                include_txtsiz =
                    fread(includebuf, 1, MAX_TXT_SIZ - txtsiz, stream3);
                fclose(stream3);

                txtsiz += include_txtsiz;
                if (txtsiz > MAX_TXT_SIZ - txtsiz)
                    error("INCLUDE File is too long");

                printf(": %i bytes\n", include_txtsiz);

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

            if ((strcmp(opcode, "NOHEADER") == 0)
                || (strcmp(opcode, "OBJFILE") == 0)) {
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
                            printf("Can't open %s\n", filnmtmp);
                            error("Can't open LST file");
                        }
                    }
                }
                continue;
            }

            /* INITRMB *///(Init reserving memory bytes)

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
                            error("ORG value is undefined");
                        orgflg = 1;
                    } else {
                        printf("to %4X\n", pc - 1);     /*write NDAD */
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
                    printf("Segment %4X ", pc);

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
                    error("Comma followed by byte value expected");
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
                    default:   /* normal ASC: kann use any other non alnum character */
                        sectyp('"');
                        if (!isalnum(*(txtptr - 1))) {
                            ch = *(txtptr - 1);
                            while ((*txtptr != ch) && (*txtptr != '\n'))
                                toxlbuf(*txtptr++);
                        } else
                            error("invalid string marking");
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
                            error
                                ("Opcode must be followed by a single space (not tab) and an operand");
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
                        error("Illegal syntax or addressing");
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
                    if (temp < -128 || temp > 127) {
                        printf
                            ("\nAttempted to branch a distance of %i. (Range -128 to 127 is permitted.)",
                             temp);
                        error("Branch out of range");
                    }
                }
                toxlbuf((char)braoffset);
                continue;
            }

/* Subroutine */

            if (strcmp(opcode, "JSR") != 0) {
                if ((cnt = chkopc(opcdat)) < 0) {       /* opcode not found, must be subroutine */
                    if ((strcmp(opcode, "MACRO") == 0) ||
                        (strcmp(opcode, "EPZ") == 0) ||
                        (strcmp(opcode, "EQU") == 0))
                        error("Label missing at beginning of line");    // since otherwise these would have been taken care of
//                              strcpy(operand,opcode);
//                              strcpy(opcode,"JSR");
                    error("A subroutine can't have an operand");
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
                    error
                        ("direct addressing (') cannot be used with this opcode");
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
                    error
                        ("direct addressing (#) cannot be used with this opcode");

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

            if ((symval < 256) && (double_byte_flg == 0))       // txtptr -> first byte after expression
            {

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
                            error("Can't have ,Y with this opcode");
                        if (*(opcode + 2) == 'X')       //  OPX aa,Y
                            operno = AAY;
                        else
                            operno = AAAAY;     //  OPA $0059,Y
                        break;
                    case 'X':  //   ,X
                    case 'x':
                        if (*(opcode + 2) == 'X')
                            error("Can't have X after comma with this opcode");
                        switch (*(++txtptr)) {
                        case ')':      //  OPC (aa,X)
                            if (*operand != '(')
                                error("Forward bracket missing");
                            operno = BAAXB;
                            break;
                        default:
                            operno = AAX;
                            break;      // OPC aa,X
                        }
                        break;
                    default:
                        error("Must have X or Y after comma -- ");
                        break;
                    }
                    break;
                case ')':
                    if (*operand != '(')
                        error("Forward bracket missing");
                    switch (*(++txtptr)) {
                    case ',':
                        switch (*(++txtptr)) {
                        case 'Y':
                        case 'y':
                            if ((*(opcode + 2) == 'Y')
                                || (*(opcode + 2) == 'X'))
                                error
                                    ("Can't use indirect addressing with this opcode");

                            operno = BAABY;
                            break;      // OPA (AA),Y
                        default:
                            error
                                ("Must have Y after comma for indirect addressing");
                            break;
                        }
                        break;
                    default:
                        error("Comma missing after bracket");
                        break;
                    }
                    break;
                default:
                    printf("Unexpected character: '%c' -- ", *txtptr);
                    error("Symbol contains illegal character");
                    break;
                }

            }


            else {              /* symval >= 256 */

                if (*operand == '(')
                    error
                        ("Zero page address expected -- Operand not defined -- Label may be unknown");


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
                            error("Can't have ,Y with this opcode");
                        operno = AAAAY;
                        break;
                    case 'X':
                    case 'x':
                        if (*(opcode + 2) == 'X')
                            error("Can't have ,X with this opcode");
                        operno = AAAAX;
                        break;
                    default:
                        error("Must have X or Y after ','");
                        break;
                    }
                    break;
                default:
                    error("Illegal operand");
                    break;
                }
            }



            if (operno == ERROR)
                error("Illegal addressing");
            if (pass == 2)
                if (*xlbfptr != (char)data[cnt][operno]) {
                    printf("\n\nOpcode mismatch %X <> %X", 0xff & (*xlbfptr),
                           0xff & data[cnt][operno]);
                    error
                        ("This zero-page label must be defined before present line.");
                }
            if (data[cnt][operno] == 0)
                error("This addressing is not possible with this opcode");
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
        }                       // end toxlbuf  end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf end toxlbuf
        printf("to %4X\n", pc - 1);

        if (pass == 1) {
            puts("Checking symbols ...");

            for (i = 0; i < macnum; i++) {
                if (symnum >= MAXSYMS - 1)
                    err(1, "Too many labels");
                if (macptrtab[i])
                    sss[symnum].macf = MACRO;
                else
                    sss[symnum].macf = MACRO_UNUSED;
                strcpy(sss[symnum++].symbol, macsymbol[i]);

            }
            qsort(sss, symnum, sizeof(ssstype), (int (*)(const void *, const void *)) strcmp);


            for (i = symnum - 1; i > 1; i--) {
                if (strcmp(sss[i].symbol, sss[i - 1].symbol) == 0) {
                    printf("\nSame label twice -\n");
                    list_label(i - 1);
                    list_label(i);
                    twiceflg = 1;
                }
            }
        }

        if (twiceflg) {
            exit(1);
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

    puts("");
    symnum = 0;
    for (i = 0; i < symnumext; i++) {
        if (sss[i].macf == MACRO)
            continue;
        if (((sss[i].macf == UNUSED) || (sss[i].macf == MACRO_UNUSED))
            && sss[i].symbol[0])
            printf("Unused symbol: %s\n", sss[i].symbol);
        if ((sss[i].symadr == sss[symnum - 1].symadr) && (symnum > 0)) {        /* double entry */
            if (sss[symnum - 1].symbol[0] && sss[i].symbol[0]) {        /* 2 labels */
                if ((sss[symnum - 1].macf == MACRO_LAB)
                    || (sss[i].macf == MACRO_LAB)) {
                    if (sss[symnum - 1].macf == MACRO_LAB)
                        strcpy(sss[symnum - 1].symbol, sss[i].symbol);
                    continue;
                } else
                    printf("%s and %s both = $%X\n", sss[i].symbol,
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

/* LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA LOA */
    if ((stream = fopen(loafilename, "wb")) == NULL) {
        fprintf(stderr, "Can't open %s\n", loafilename);
        exit(1);
    }

    printf("\nSaving binary file %s\n", loafilename);
    txtsiz = fwrite(xlbuf, 1, xlbfptr - xlbuf, stream);
    if (txtsiz != xlbfptr - xlbuf) {
        fprintf(stderr, "Can't save %s\n", loafilename);
        exit(1);
    }
    printf("%d bytes\n", txtsiz);
    fclose(stream);
}


