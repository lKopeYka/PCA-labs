#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>

#define TIMER_REG_AMOUNT 3
#define TIME_REG_AMOUNT 6

typedef unsigned char byte;

enum time_registers {
  sec = 0x00,
  sec_timer = 0x01,
  min = 0x02,
  min_timer = 0x03,
  hour = 0x04,
  hour_timer = 0x05,
  day = 0x07,
  month = 0x08,
  year = 0x09,
};

enum state_registers {
  A = 0x0A,
  B = 0x0B,
  C = 0x0C,
  D = 0x0D,
};

unsigned time[TIME_REG_AMOUNT];
const byte time_regs[TIME_REG_AMOUNT] = {sec, min, hour, day, month, year};
const byte timer_regs[TIMER_REG_AMOUNT] = {sec_timer, min_timer, hour_timer};

volatile int alarm_flag = 0;

unsigned bcd_to_dec(unsigned bcd) { return (bcd / 16 * 10) + (bcd % 16); }
unsigned dec_to_bcd(unsigned dec) { return (dec / 10 * 16) + (dec % 10); }

void wait_UIP() {
  do {
    outp(0x70, A);
  } while (inp(0x71) & 0x80);
}

void print_time(void) {
  int i;

  for (i = 0; i < TIME_REG_AMOUNT; ++i) {
    wait_UIP();

    outp(0x70, time_regs[i]);
    time[i] = bcd_to_dec(inp(0x71));
  }

  printf("%02u:%02u:%02u %02u.%02u.20%02u\n",
    time[2], time[1], time[0], time[3], time[4], time[5]);
}

void set_time(void) {
  int i;
  byte reg;

  puts("Enter time (hh:mm:ss):");
  scanf("%u:%u:%u", &time[2], &time[1], &time[0]);

  puts("Enter date (dd.mm.yy):");
  scanf("%u.%u.%u", &time[3], &time[4], &time[5]);

  for (i = 0; i < TIME_REG_AMOUNT; ++i)
    time[i] = dec_to_bcd(time[i]);

  disable();

  wait_UIP();

  // читаем регистр B
  outp(0x70, B);
  reg = inp(0x71);

  // запрещаем обновление
  outp(0x70, B);
  outp(0x71, reg | 0x80);

  // запись времени
  for (i = 0; i < TIME_REG_AMOUNT; i++) {
    outp(0x70, time_regs[i]);
    outp(0x71, time[i]);
  }

  // включаем обновление обратно
  outp(0x70, B);
  outp(0x71, reg & 0x7F);

  enable();
}

void interrupt(*old_interrupt)(void);

void interrupt new_interrupt(void) {
  outp(0x70, C);
  if (inp(0x71) & 0x20) {
    alarm_flag = 1; // только флаг!
  }

  outp(0x20, 0x20);
  outp(0xA0, 0x20);
}

void set_alarm(void) {
  int i;

  puts("Enter alarm time (hh:mm:ss):");
  scanf("%u:%u:%u", &time[2], &time[1], &time[0]);

  for (i = 0; i < TIMER_REG_AMOUNT; ++i)
    time[i] = dec_to_bcd(time[i]);

  disable();

  old_interrupt = getvect(0x70);
  setvect(0x70, new_interrupt);

  // разрешаем IRQ8
  outp(0xA1, inp(0xA1) & 0xFE);

  wait_UIP();

  for (i = 0; i < TIMER_REG_AMOUNT; i++) {
    outp(0x70, timer_regs[i]);
    outp(0x71, time[i]);
  }

  // включаем будильник
  outp(0x70, B);
  outp(0x71, inp(0x71) | 0x20);

  enable();

  puts("Alarm set");
}

void set_delay(void) {
  byte reg;

  disable();

  wait_UIP();

  outp(0x70, A);
  reg = inp(0x71);

  outp(0x70, A);
  outp(0x71, (reg & 0xF0) | 0x06); // 1024 Гц

  enable();

  puts("Delay frequency set to 1024 Hz");
}

int main(void) {
  while (1) {
    if (alarm_flag) {
      puts("ALARM!");
      alarm_flag = 0;
    }

    puts("1 - print time");
    puts("2 - set time");
    puts("3 - set alarm");
    puts("4 - set delay");
    puts("0 - exit");

    switch (getch()) {
      case '1': print_time(); break;
      case '2': set_time(); break;
      case '3': set_alarm(); break;
      case '4': set_delay(); break;
      case '0': exit(0);
    }
  }
}