#include <dos.h>     
#include <stdio.h>    
#include <conio.h>    

volatile unsigned long msCounter = 0; 
volatile int alarmFlag = 0;           
void interrupt far(*oldInt70h)(void);  

const char* DAYS[] = { "", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

void interrupt far NewInt70Handler(void);  
int BCDToInteger(unsigned char bcd);       
unsigned char IntToBCD(int value);         
void WaitClockIsFree();                   
void GetTime();                            
void SetTime();                            
void CustomDelay();                        
void SetAlarm();                           


// Чтение значения из CMOS-памяти
// reg - номер регистра CMOS 
// @return значение из указанного регистра
unsigned char ReadCMOS(unsigned char reg) {
    outp(0x70, reg);     
    return inp(0x71);     
}

/**
 * Запись значения в CMOS-память
 * @param reg - номер регистра CMOS
 * @param value - записываемое значение
 */
void WriteCMOS(unsigned char reg, unsigned char value) {
    outp(0x70, reg);      // Выбор регистра
    outp(0x71, value);    // Запись данных
}

/**
 * Обработчик прерывания RTC
 * Вызывается при каждом прерывании от часов реального времени
 */
void interrupt far NewInt70Handler(void) {
    unsigned char statusC;
    outp(0x70, 0x0C);                // Выбираем регистр состояния C
    statusC = inp(0x71);             // Читаем его (это сбрасывает флаги прерываний)

    // Проверяем биты в регистре состояния C
    if (statusC & 0x40) msCounter++; // Бит 6 (0x40) - периодическое прерывание (каждую миллисекунду)
    if (statusC & 0x20) alarmFlag = 1; // Бит 5 (0x20) - прерывание от будильника

    // Отправляем EOI (End Of Interrupt) контроллерам прерываний
    outp(0xA0, 0x20);  // Для второго контроллера (IRQ8-15)
    outp(0x20, 0x20);  // Для первого контроллера (IRQ0-7)
}

void main() {
    char c = 0;
    clrscr();  

    printf("RTC Lab Work Control (Date & Time Edition)\n");
    printf("1 - Show Date & Time\n2 - Set Date & Time\n3 - Delay (ms)\n4 - Set Alarm\nESC - Exit\n");

    while (c != 27) {
        if (kbhit()) {           
            c = getch();        
            switch (c) {
            case '1': GetTime(); break;      
            case '2': SetTime(); break;     
            case '3': CustomDelay(); break;  
            case '4': SetAlarm(); break;    
            }
        }
    }
}
   

/**
 * Ожидание, пока часы не освободятся для доступа
 * Бит 7 регистра 0x0A показывает, идет ли обновление времени
 */
void WaitClockIsFree() {
    while (ReadCMOS(0x0A) & 0x80);  // Ждем, пока бит UIP (Update In Progress) не станет 0
}

/**
 * Получение и вывод текущей даты и времени из CMOS
 */
void GetTime() {
    unsigned char s, m, h, dayW, dayM, mon, year;
    WaitClockIsFree();  // Ждем, пока часы не освободятся для чтения

    // Чтение всех необходимых регистров CMOS
    s = ReadCMOS(0x00);      // Секунды
    m = ReadCMOS(0x02);      // Минуты
    h = ReadCMOS(0x04);      // Часы
    dayW = ReadCMOS(0x06);   // День недели (1-7)
    dayM = ReadCMOS(0x07);   // Число месяца (1-31)
    mon = ReadCMOS(0x08);    // Месяц (1-12)
    year = ReadCMOS(0x09);   // Год (0-99)

    // Вывод даты и времени с преобразованием BCD в десятичные числа
    printf("\nDate: %02d.%02d.20%02d (%s)",
        BCDToInteger(dayM), BCDToInteger(mon), BCDToInteger(year),
        (dayW > 0 && dayW < 8) ? DAYS[dayW] : "Unknown");
    printf("\nTime: %02d:%02d:%02d\n", BCDToInteger(h), BCDToInteger(m), BCDToInteger(s));
}

/**
 * Установка даты и времени в CMOS
 * Пользователь вводит значения, которые затем записываются в регистры
 */
void SetTime() {
    int h, m, s, dayW, dayM, mon, yr;
    unsigned char regB;

    printf("\nEnter Time (h m s): ");
    scanf("%d %d %d", &h, &m, &s);
    printf("Enter Date (day month year_2_digits): ");
    scanf("%d %d %d", &dayM, &mon, &yr);
    printf("Enter Day of Week (1-Sun, 2-Mon, ..., 7-Sat): ");
    scanf("%d", &dayW);

    WaitClockIsFree();                    // Ждем освобождения часов
    regB = ReadCMOS(0x0B);                // Читаем регистр управления B
    WriteCMOS(0x0B, regB | 0x80);         // Устанавливаем бит SET (бит 7) - останавливаем обновление времени

    // Записываем новые значения в BCD формате
    WriteCMOS(0x00, IntToBCD(s));    // Секунды
    WriteCMOS(0x02, IntToBCD(m));    // Минуты
    WriteCMOS(0x04, IntToBCD(h));    // Часы
    WriteCMOS(0x06, IntToBCD(dayW)); // День недели
    WriteCMOS(0x07, IntToBCD(dayM)); // Число
    WriteCMOS(0x08, IntToBCD(mon));  // Месяц
    WriteCMOS(0x09, IntToBCD(yr));   // Год

    WriteCMOS(0x0B, regB & 0x7F);    // Снимаем бит SET - возобновляем обновление времени
    printf("Date and Time updated.\n");
}

/**
 * Функция задержки в миллисекундах с использованием RTC
 * Настраивает периодические прерывания от часов
 */
void CustomDelay() {
    unsigned long delayMs;
    unsigned char oldB, oldMask, oldA;

    printf("\nEnter delay in ms: ");
    scanf("%lu", &delayMs);

    disable();                                      // Запрещаем прерывания
    oldInt70h = getvect(0x70);                      // Сохраняем старый обработчик IRQ8 (прерывание 0x70)
    setvect(0x70, NewInt70Handler);                 // Устанавливаем свой обработчик

    oldA = ReadCMOS(0x0A);                          // Сохраняем регистр A
    WriteCMOS(0x0A, (oldA & 0xF0) | 0x06);          // Настраиваем частоту прерываний (0x06 = 1024 Гц)

    oldMask = inp(0xA1);                            // Сохраняем маску прерываний
    outp(0xA1, oldMask & 0xFE);                     // Разрешаем IRQ8 (бит 0 маски)

    oldB = ReadCMOS(0x0B);                          // Сохраняем регистр B
    WriteCMOS(0x0B, oldB | 0x40);                   // Включаем периодические прерывания (бит 6)

    msCounter = 0;                                  // Обнуляем счетчик
    enable();                                       // Разрешаем прерывания

    printf("Waiting...");
    while (msCounter < delayMs);                    // Ждем, пока не наберется нужное количество миллисекунд
    printf("\n[Done] %lu ms passed!\n", delayMs);

    disable();                                      // Запрещаем прерывания
    WriteCMOS(0x0B, oldB);                          // Восстанавливаем регистр B
    WriteCMOS(0x0A, oldA);                          // Восстанавливаем регистр A
    outp(0xA1, oldMask);                            // Восстанавливаем маску прерываний
    setvect(0x70, oldInt70h);                       // Восстанавливаем старый обработчик
    enable();                                       // Разрешаем прерывания
}

/**
 * Установка будильника
 * При достижении заданного времени срабатывает прерывание
 */
void SetAlarm() {
    int h, m, s;
    unsigned char oldB, oldMask;

    printf("\nEnter Alarm Time (h m s): ");
    scanf("%d %d %d", &h, &m, &s);

    disable();                                      // Запрещаем прерывания
    oldInt70h = getvect(0x70);                      // Сохраняем старый обработчик
    setvect(0x70, NewInt70Handler);                 // Устанавливаем свой обработчик
    oldMask = inp(0xA1);                            // Сохраняем маску
    outp(0xA1, oldMask & 0xFE);                     // Разрешаем IRQ8

    // Записываем время будильника в регистры CMOS
    WriteCMOS(0x05, IntToBCD(h));    // Часы будильника
    WriteCMOS(0x03, IntToBCD(m));    // Минуты будильника
    WriteCMOS(0x01, IntToBCD(s));    // Секунды будильника

    oldB = ReadCMOS(0x0B);                          // Сохраняем регистр B
    WriteCMOS(0x0B, oldB | 0x20);                   // Включаем прерывание будильника (бит 5)

    alarmFlag = 0;                                  // Сбрасываем флаг
    enable();                                       // Разрешаем прерывания

    printf("Alarm armed. Press ESC to cancel.\n");
    // Ожидаем срабатывания будильника или нажатия ESC
    while (!alarmFlag) {
        if (kbhit() && getch() == 27) break;        // Выход при нажатии ESC
    }

    if (alarmFlag) printf("\n>>> BEEP BEEP! ALARM! <<<\n");

    disable();                                      // Запрещаем прерывания
    WriteCMOS(0x0B, oldB);                          // Восстанавливаем регистр B
    outp(0xA1, oldMask);                            // Восстанавливаем маску
    setvect(0x70, oldInt70h);                       // Восстанавливаем обработчик
    enable();                                       // Разрешаем прерывания
}


int BCDToInteger(unsigned char bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

unsigned char IntToBCD(int value) {
    return (unsigned char)(((value / 10) << 4) | (value % 10));
}
