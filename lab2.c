#include <dos.h> 

#define BUFF_WIDTH 80
#define CENTER_OFFSET 12
#define LEFT_OFFSET 20
#define REG_SCREEN_SIZE 9

struct VIDEO
{
    unsigned char symb;
    unsigned char attr;
};

int attribute = 0x5e;

void print(int offset, int value);
void getRegisterValue(void);
void init(void);

void interrupt(*oldHandle08)(void);
void interrupt(*oldHandle09)(void);
void interrupt(*oldHandle0A)(void);
void interrupt(*oldHandle0B)(void);
void interrupt(*oldHandle0C)(void);
void interrupt(*oldHandle0D)(void);
void interrupt(*oldHandle0E)(void);
void interrupt(*oldHandle0F)(void);

void interrupt(*oldHandle70)(void);
void interrupt(*oldHandle71)(void);
void interrupt(*oldHandle72)(void);
void interrupt(*oldHandle73)(void);
void interrupt(*oldHandle74)(void);
void interrupt(*oldHandle75)(void);
void interrupt(*oldHandle76)(void);
void interrupt(*oldHandle77)(void);

void interrupt newHandleC8() { getRegisterValue(); oldHandle08(); }
void interrupt newHandleC9() { attribute++; getRegisterValue(); oldHandle09(); }
void interrupt newHandleCA() { getRegisterValue(); oldHandle0A(); }
void interrupt newHandleCB() { getRegisterValue(); oldHandle0B(); }
void interrupt newHandleCC() { getRegisterValue(); oldHandle0C(); }
void interrupt newHandleCD() { getRegisterValue(); oldHandle0D(); }
void interrupt newHandleCE() { getRegisterValue(); oldHandle0E(); }
void interrupt newHandleCF() { getRegisterValue(); oldHandle0F(); }

void interrupt newHandleD0() { getRegisterValue(); oldHandle70(); }
void interrupt newHandleD1() { getRegisterValue(); oldHandle71(); }
void interrupt newHandleD2() { getRegisterValue(); oldHandle72(); }
void interrupt newHandleD3() { getRegisterValue(); oldHandle73(); }
void interrupt newHandleD4() { getRegisterValue(); oldHandle74(); }
void interrupt newHandleD5() { getRegisterValue(); oldHandle75(); }
void interrupt newHandleD6() { getRegisterValue(); oldHandle76(); }
void interrupt newHandleD7() { getRegisterValue(); oldHandle77(); }

void print(int offset, int value)
{
    char temp;
    int i;
    struct VIDEO far *screen;
    
    screen = (struct VIDEO far *)MK_FP(0xB800, 0);
    screen += CENTER_OFFSET * BUFF_WIDTH + offset;

    for (i = 7; i >= 0; i--)
    {
        temp = value % 2;
        value = value / 2;
        screen->symb = temp + '0';
        screen->attr = attribute;
        screen++;
    }
}

void getRegisterValue(void)
{
    print(0 + LEFT_OFFSET, inp(0x21));
    
    outp(0x20, 0x0B);
    print(REG_SCREEN_SIZE + LEFT_OFFSET, inp(0x20));
    
    outp(0x20, 0x0A);
    print(REG_SCREEN_SIZE * 2 + LEFT_OFFSET, inp(0x20));
    
    print(BUFF_WIDTH + LEFT_OFFSET, inp(0xA1));
    
    outp(0xA0, 0x0B);
    print(BUFF_WIDTH + REG_SCREEN_SIZE + LEFT_OFFSET, inp(0xA0));
    
    outp(0xA0, 0x0A);
    print(BUFF_WIDTH + REG_SCREEN_SIZE * 2 + LEFT_OFFSET, inp(0xA0));
}

void init(void)
{
    oldHandle08 = getvect(0x08);
    oldHandle09 = getvect(0x09);
    oldHandle0A = getvect(0x0A);
    oldHandle0B = getvect(0x0B);
    oldHandle0C = getvect(0x0C);
    oldHandle0D = getvect(0x0D);
    oldHandle0E = getvect(0x0E);
    oldHandle0F = getvect(0x0F);

    oldHandle70 = getvect(0x70);
    oldHandle71 = getvect(0x71);
    oldHandle72 = getvect(0x72);
    oldHandle73 = getvect(0x73);
    oldHandle74 = getvect(0x74);
    oldHandle75 = getvect(0x75);
    oldHandle76 = getvect(0x76);
    oldHandle77 = getvect(0x77);

    setvect(0xC8, newHandleC8);
    setvect(0xC9, newHandleC9);
    setvect(0xCA, newHandleCA);
    setvect(0xCB, newHandleCB);
    setvect(0xCC, newHandleCC);
    setvect(0xCD, newHandleCD);
    setvect(0xCE, newHandleCE);
    setvect(0xCF, newHandleCF);

    setvect(0xD0, newHandleD0);
    setvect(0xD1, newHandleD1);
    setvect(0xD2, newHandleD2);
    setvect(0xD3, newHandleD3);
    setvect(0xD4, newHandleD4);
    setvect(0xD5, newHandleD5);
    setvect(0xD6, newHandleD6);
    setvect(0xD7, newHandleD7);

    _disable();

    outp(0x20, 0x11);
    outp(0x21, 0xC8);
    outp(0x21, 0x04);
    outp(0x21, 0x01);

    outp(0xA0, 0x11);
    outp(0xA1, 0xD0);
    outp(0xA1, 0x02);
    outp(0xA1, 0x01);

    _enable();
}

int main()
{
    unsigned far *fp;

    init();

    FP_SEG(fp) = _psp;
    FP_OFF(fp) = 0x2c;
    _dos_freemem(*fp);

    _dos_keep(0, (_DS - _CS) + (_SP / 16) + 1);
    return 0;
}