#include <stdio.h>
#include <dos.h>

#define NOTE1 392u
#define NOTE2 329u
#define NOTE3 349u

#define duration 15000u   
#define indent   15000u    


unsigned notes[][3] = {
    {NOTE1, duration, indent},  // 392 Гц
    {NOTE2, duration, indent},  // 329 Гц
    {NOTE2, duration, indent},  // 329 Гц
    {NOTE2, duration, indent},  // 329 Гц
    {NOTE1, duration, indent},  // 392 Гц
    {NOTE1, duration, indent},  // 392 Гц
    {NOTE1, duration, indent},  // 392 Гц
    {NOTE1, duration, indent},  // 392 Гц
    {NOTE3, duration, 0},       // 349 Гц (последняя нота без паузы)
};

// Автоматический подсчёт количества нот
#define NOTES_AMOUNT (sizeof(notes) / sizeof(notes[0]))

// Чтение состояния таймера
void read_timer_status(void) {
    unsigned channel;
    unsigned char status;
    unsigned int ce_value;
    int ports[] = {0x40, 0x41, 0x42};
    char status_binary[9];
    int i;

    printf("\n=== Timer channels status ===\n");
    
    for (channel = 0; channel < 3; channel++) {
        // Чтение слова состояния
        outp(0x43, 0xE2 | (channel ? (channel == 1 ? 0x04 : 0x08) : 0x00));
        status = inp(ports[channel]);
        
        // Вывод состояния в двоичном виде
        for (i = 7; i >= 0; i--) {
            status_binary[i] = (status % 2) + '0';
            status /= 2;
        }
        status_binary[8] = '\0';
        printf("Channel %d status: %s (binary)\n", channel, status_binary);
        
        // Чтение текущего значения счётчика CE
        outp(0x43, 0x00 | (channel << 6));
        ce_value = inp(ports[channel]);
        ce_value |= (inp(ports[channel]) << 8);
        
        printf("Channel %d CE: 0x%04X (hex)\n\n", channel, ce_value);
    }
    printf("==============================\n");
}

// Установка частоты для второго канала таймера
void set_frequency(unsigned int frequency) {
    unsigned int divider;
    
    // Расчёт коэффициента деления
    divider = 1193180UL / frequency;
    
    // Защита от переполнения
    if (divider > 65535) {
        divider = 65535;
    }
    
    // Управляющее слово для канала 2 (режим 3 - меандр)
    outp(0x43, 0xB6);
    
    // Запись делителя: младший, затем старший байт
    outp(0x42, divider & 0xFF);
    outp(0x42, (divider >> 8) & 0xFF);
}

// Воспроизведение мелодии
void play_music(void) {
    int i;
    unsigned char speaker_status;
    
    printf("\nPlaying melody...\n");
    
    for (i = 0; i < NOTES_AMOUNT; i++) {
        // Установка частоты ноты
        set_frequency(notes[i][0]);
        
        // Включение динамика
        speaker_status = inp(0x61);
        outp(0x61, speaker_status | 0x03);
        
        // Звучание ноты
        delay(notes[i][1]);
        
        // Выключение динамика
        speaker_status = inp(0x61);
        outp(0x61, speaker_status & 0xFC);
        
        // Пауза (если есть)
        if (notes[i][2] > 0) {
            delay(notes[i][2]);
        }
    }
    
    printf("Melody finished.\n");
}

int main(void) {
    // Состояние ДО
    read_timer_status();
    
    play_music();
    
    // Состояние ПОСЛЕ
    read_timer_status();
    
    return 0;
}