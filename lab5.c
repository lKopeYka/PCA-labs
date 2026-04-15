#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#define KEYBOARD_INTERRUPT 0x09
#define KEYBOARD_LIGHTS_CODE 0xED

#define NONE 0x00
#define SCROLL_LOCK 0x01
#define NUM_LOCK 0x02
#define CAPS_LOCK 0x04

typedef unsigned char byte;

// Глобальные переменные для обмена с обработчиком
volatile byte last_response = 0;
volatile int response_ready = 0;

void interrupt (*old_handler)(void);

/**
 * Обработчик прерывания клавиатуры
 * Только сохраняет полученный код, без вывода на экран!
 */
void interrupt new_handler(void) {
    byte scan_code = inp(0x60);  // читаем скан-код или код возврата
    
    // Сохраняем ответ для основной программы
    last_response = scan_code;
    response_ready = 1;
    
    // Отправляем EOI контроллеру прерываний
    outp(0x20, 0x20);
    
    // Вызываем старый обработчик (для нормальной работы клавиатуры)
    old_handler();
}

/**
 * Ожидание ответа от клавиатуры с таймаутом
 * Возвращает: полученный байт или 0xFF при ошибке
 */
byte wait_for_response(int timeout_ms) {
    response_ready = 0;
    
    while (timeout_ms > 0) {
        if (response_ready) {
            response_ready = 0;
            return last_response;
        }
        delay(10);
        timeout_ms -= 10;
    }
    return 0xFF;  // таймаут
}

/**
 * Ожидание готовности клавиатуры (бит 1 порта 64h = 0)
 * Входной буфер клавиатуры пуст, можно отправлять команду
 */
void wait_keyboard_ready(void) {
    while (inp(0x64) & 0x02);
}

/**
 * Ожидание, когда у клавиатуры есть данные для чтения (бит 0 порта 64h = 1)
 */
void wait_keyboard_data(void) {
    while (!(inp(0x64) & 0x01));
}

/**
 * Отправка команды и маски на клавиатуру с контролем ошибок
 * Возвращает: 1 - успех, 0 - ошибка после 3 попыток
 */
int send_led_mask(byte mask) {
    int attempt;
    byte response;
    
    for (attempt = 0; attempt < 3; attempt++) {
        // 1. Ждём готовности клавиатуры
        wait_keyboard_ready();
        
        // 2. Отправляем команду 0xED
        outp(0x60, KEYBOARD_LIGHTS_CODE);
        
        // 3. Ждём ответ (FA или FE)
        response = wait_for_response(500);
        if (response == 0xFA) {
            // Команда принята, теперь отправляем маску
            wait_keyboard_ready();
            outp(0x60, mask);
            
            // Ждём ответ на маску
            response = wait_for_response(500);
            if (response == 0xFA) {
                printf("OK: mask 0x%02X set successfully\n", mask);
                return 1;
            } else if (response == 0xFE) {
                printf("Error: mask 0x%02X - resending (attempt %d)\n", mask, attempt + 1);
                continue;
            }
        } else if (response == 0xFE) {
            printf("Error: command 0xED - resending (attempt %d)\n", attempt + 1);
            continue;
        }
    }
    
    printf("FAILED: mask 0x%02X after 3 attempts\n", mask);
    return 0;
}

/**
 * Мигание индикаторами в произвольном порядке
 */
void blink_lights(void) {
    // Зажигаем NUM LOCK
    printf("\n[1] NUM LOCK ON\n");
    send_led_mask(NUM_LOCK);
    delay(1500);
    
    // Зажигаем CAPS LOCK
    printf("[2] CAPS LOCK ON\n");
    send_led_mask(CAPS_LOCK);
    delay(1500);
    
    // Зажигаем SCROLL LOCK
    printf("[3] SCROLL LOCK ON\n");
    send_led_mask(SCROLL_LOCK);
    delay(1500);
    
    // Гасим всё
    printf("[4] ALL OFF\n");
    send_led_mask(NONE);
    delay(1500);
    
    // Зажигаем все три
    printf("[5] ALL ON\n");
    send_led_mask(SCROLL_LOCK | NUM_LOCK | CAPS_LOCK);
    delay(1500);
    
    // Гасим всё
    printf("[6] ALL OFF\n");
    send_led_mask(NONE);
    delay(1500);
    
    // Снова гасим (для паузы)
    send_led_mask(NONE);
    delay(1500);
    
    // Снова все три
    printf("[7] ALL ON\n");
    send_led_mask(SCROLL_LOCK | NUM_LOCK | CAPS_LOCK);
    delay(1500);
    
    // Финальное выключение
    printf("[8] FINAL OFF\n");
    send_led_mask(NONE);
}

int main(void) {
    printf("\n=== Keyboard LED Control ===\n");
    printf("The program will blink keyboard indicators\n");
    printf("Press any key to start...\n");
    getch();
    
    // Сохраняем старый обработчик
    old_handler = getvect(KEYBOARD_INTERRUPT);
    
    // Устанавливаем свой обработчик
    setvect(KEYBOARD_INTERRUPT, new_handler);
    
    // Мигаем индикаторами
    blink_lights();
    
    // Восстанавливаем старый обработчик
    setvect(KEYBOARD_INTERRUPT, old_handler);
    
    printf("\n=== Program finished ===\n");
    return 0;
}