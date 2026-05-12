#include <stdio.h>
#include <dos.h>
#include <conio.h>

#define DATA_PORT 0x60
#define STATUS_PORT 0x64
#define PIC_MASK_PORT 0x21

void wait_kbd() {
    int timeout = 10000;
    while ((inp(STATUS_PORT) & 0x02) && --timeout);
}

void set_leds(unsigned char mask) {
    wait_kbd();
    outp(DATA_PORT, 0xED);
    delay(50);
    wait_kbd();
    outp(DATA_PORT, mask);
    delay(50);
}

int main() {
    unsigned char scan = 0;
    unsigned char old_mask;

    clrscr();
    printf("--- STEP 1: LED BLINK ---\n");
    set_leds(0x02); delay(300); set_leds(0x00);

    printf("\n--- STEP 2: SCAN CODE MONITOR ---\n");
    printf("BIOS keyboard driver is now DISABLED.\n");
    printf("Press 'ESC' to exit and restore system.\n\n");

    /* сохранение маски */
    old_mask = inp(PIC_MASK_PORT);
    outp(PIC_MASK_PORT, old_mask | 0x02); 

    /* Очистка буфера перед стартом */
    while (inp(STATUS_PORT) & 0x01) inp(DATA_PORT);

    while (scan != 0x01) { /* выход по ESC */
        /* Опрашиваем порт напрямую */
        if (inp(STATUS_PORT) & 0x01) {
            scan = inp(DATA_PORT);
            
            if (scan != 0) {
                if (scan & 0x80) printf("Released: %02Xh\n", scan);
                else printf("Pressed:  %02Xh\n", scan);
            }
        }
        delay(1);
    }

    /* восстановление маски */
    outp(PIC_MASK_PORT, old_mask);

    set_leds(0x00);
    printf("\nSystem restored. Exit.\n");
    return 0;
}
