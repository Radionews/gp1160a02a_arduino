/* бибилиотека для работы с экраном gp1160a02a по стандартному RS232 для ардуино
* Написана Науменко А.А. aka Radionews
* https://github.com/Radionews/gp1160a02a_arduino
* Последняя дата изменения 03.05.2021*/

#ifndef vfd_lib_h
#define vfd_lib_h
#include <Arduino.h>

// класс vfd дисплея

#define BYTE_SIZE 8
#define DISPLAY_WIDTH 256
#define DISPLAY_WIDTH_BYTES DISPLAY_WIDTH / 8
#define DISPLAY_HEIGHT 64

#define MODE_8_ROWS 1
#define MODE_3_ROWS 2
#define MODE_2_ROWS 3
#define MODE_1_2_ROWS 4
#define MODE_4_ROWS 5
#define MODE_D_3_ROWS 6
#define MODE_3_D_ROWS 7

class VFD {
public:
    VFD();
    void clear();
    void send_str(const char* data, uint16_t x, uint8_t y, bool setc = true);
    void set_dimming(uint8_t level);
    void set_mode(uint8_t mode);
    void set_scroll(const char* data, uint8_t row);
    void print_graph(uint16_t x, uint8_t y, uint8_t h, uint16_t w, const unsigned char(*data));

private:
    void add_ansii_num(char* str, uint16_t num, bool semicolon = true);
    void set_cursor(uint16_t x, uint8_t y);
    uint8_t mode_save = 2;
    uint8_t scroll_en = 0;
};

VFD::VFD() {
    mode_save = 2;
    scroll_en = 0;
}

//функция перевода числа в симвлоы asci и добавления их в конец строки
void VFD::add_ansii_num(char* str, uint16_t num, bool semicolon = true) {
    char num_buffer[3]{};

    itoa(num, num_buffer, 10);
    strcat(str, num_buffer);
    if(semicolon) {
        str[strlen(str)] = ';';
    }
}

//функции очистки экрана
void VFD::clear() {
    Serial.write("\x1B\x5C\x3F\x4C\x4D\x45");
    Serial.write("\x1b\x5b\x32\x4a");
}

//функция вывода строки на экран с декодированием кирилицы
void VFD::send_str(const char* data, uint16_t x, uint8_t y, bool setc = true) {
    if(scroll_en == 1) {
        Serial.write("\x1B\x5C\x3F\x4C\x4D\x45");
        scroll_en = 0;
    }
    if(setc)
        set_cursor(x, y);
    char temp = 0;
    for(unsigned int i = 0, end = strlen(data); i < end; i++) {
        temp = data[i] & 0xFF;
        if(temp == 0xD0) {
            i++;
            temp = data[i] & 0xFF;

            Serial.write(0x84);
            if(temp == 0x81)
                Serial.write(0x46);
            else {
                temp = ((unsigned int)data[i] - 0x90) & 0xFF;
                if(temp < 0x06)
                    Serial.write(0x40 + temp);
                else if((temp >= 0x06) && (temp < 0x20))
                    Serial.write(0x40 + temp + 1);
                else if((temp >= 0x20) && (temp < 0x26))
                    Serial.write(0x40 + temp + 16);
                else if((temp >= 0x26) && (temp < 0x2E))
                    Serial.write(0x40 + temp + 17);
                else if((temp >= 0x2E) && (temp < 0x31))
                    Serial.write(0x40 + temp + 18);
            }
        } else if(temp == 0xD1) {
            i++;
            temp = data[i] & 0xFF;
            Serial.write(0x84);
            if(temp == 0x91)
                Serial.write(0x76);
            else {
                temp = ((unsigned int)data[i] - 0x80) & 0xFF;
                Serial.write(0x82 + temp);
            }
        } else {
            Serial.write(data[i]);
        }
    }
}

//установка режима экрана
void VFD::set_mode(uint8_t mode) {
    mode_save = mode;
    char code_buffer[10]{};

    add_ansii_num(code_buffer, mode, false);

    Serial.write("\x1b\x5c\x3f\x4c\x53");
    Serial.write(code_buffer, strlen(code_buffer));
}

//установка положения курсора, х - по горизонтали, y - по вертикали
void VFD::set_cursor(uint16_t x, uint8_t y) {
    char code_buffer[10]{};

    add_ansii_num(code_buffer, y);
    add_ansii_num(code_buffer, x, false);
    code_buffer[strlen(code_buffer)] = 'H';

    Serial.write("\x1b\x5b");
    Serial.write(code_buffer, strlen(code_buffer));
}

//установка яркости экрана
void VFD::set_dimming(uint8_t level) {
    char code_buffer[10]{};

    if(level > 5)
        level = 5;
    add_ansii_num(code_buffer, level, false);

    Serial.write("\x1b\x5c\x3f\x4c\x44");
    Serial.write(code_buffer, strlen(code_buffer));
}

//задать бегущую строку
void VFD::set_scroll(const char* data, uint8_t row) {
    Serial.write("\x1B\x5C\x3F\x4C\x4D\x45");
    Serial.write("\x1B\x5C\x3F\x4C\x4D\x53");
    set_cursor(0, row);

    char code_buffer[20]{};

    if(mode_save < 4)
        add_ansii_num(code_buffer, mode_save); //1-5x7, 2-16x16, 3-24x24
    else
        add_ansii_num(code_buffer, 2); //1-5x7, 2-16x16, 3-24x24

    add_ansii_num(code_buffer, row); //(1-8,1-3,1-2)
    add_ansii_num(code_buffer, 1); //1-10mS,2-20mS

    Serial.write("\x1B\x5C\x3F\x4C\x4D");
    Serial.write(code_buffer, strlen(code_buffer));
    Serial.write(strlen(data));
    Serial.write("\x3B");

    send_str(data, 0, row, false);
    Serial.write("\x1B\x5C\x3F\x4C\x4D\x47");
    scroll_en = 1;
}

const char* gfx_code = "\x1b\x5c\x3f\x4c\x47";
void VFD::print_graph(uint16_t x, uint8_t y, uint8_t h, uint16_t w, const unsigned char(*data)) {
    Serial.write("\x1B\x5C\x3F\x4C\x4D\x45");
    char msg_prefix[20]{};

    memcpy(msg_prefix, gfx_code, strlen(gfx_code));

    add_ansii_num(msg_prefix, x);
    add_ansii_num(msg_prefix, y);
    add_ansii_num(msg_prefix, h);
    add_ansii_num(msg_prefix, w);

    const uint8_t rows_in_bytes = (uint16_t)ceil(abs(w) / BYTE_SIZE);
    const uint16_t data_size = rows_in_bytes * abs(h);
    const uint8_t prefix_size = strlen(msg_prefix);
    const uint16_t message_size = prefix_size + data_size + 1;

    Serial.write(msg_prefix, prefix_size);

    for(int i = 0; i < 256 * 8; i++) {
        Serial.write(pgm_read_byte(&data[i]));
    }
}

const unsigned char test_fire[] PROGMEM = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
    0xff, 0xdf, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xfe, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
    0xff, 0xdf, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xf9, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xfe, 0x3f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff,
    0xff, 0x9f, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xfb, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff,
    0xff, 0x9f, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xf3, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
    0xff, 0x9f, 0xff, 0xb9, 0xff, 0xff, 0xff, 0xf3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
    0xff, 0x9f, 0xff, 0xb9, 0xff, 0xff, 0xff, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x8f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
    0xff, 0x0f, 0xff, 0x98, 0xff, 0xff, 0xff, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0xff, 0x87, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
    0xff, 0x0f, 0xff, 0x98, 0xff, 0xff, 0xff, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0x3f, 0xc3, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7c, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0xff,
    0xff, 0x07, 0xff, 0x9c, 0x7f, 0xff, 0xfb, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0x3f, 0xc3, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x7c, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff,
    0xff, 0x87, 0xff, 0x9c, 0x3f, 0xff, 0xfb, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0x3f, 0xc1, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff,
    0xff, 0x83, 0xff, 0x9c, 0x3f, 0xff, 0xfb, 0xe3, 0xfe, 0x3f, 0xff, 0xff, 0xff, 0x3f, 0xe1, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xff,
    0xff, 0xc1, 0xff, 0x1e, 0x1f, 0xff, 0xfb, 0xe3, 0xfe, 0x1f, 0xff, 0xff, 0xff, 0x3f, 0xe1, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7c, 0x7f,
    0xff, 0xc1, 0xff, 0x1e, 0x1f, 0xff, 0xfb, 0xe1, 0xff, 0x1f, 0xff, 0xff, 0xfe, 0x3f, 0xe1, 0xef,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x3e, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7c, 0x3f,
    0xff, 0xe0, 0xfe, 0x1f, 0x0f, 0xff, 0xfb, 0xf1, 0xff, 0x0f, 0xff, 0xff, 0xfe, 0x3f, 0xe1, 0xef,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x1e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3f,
    0xef, 0xe0, 0xfe, 0x1f, 0x0f, 0xff, 0xfb, 0xf0, 0xff, 0x8f, 0xff, 0xff, 0xfe, 0x1f, 0xc1, 0xcf,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x1e, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7e, 0x1f,
    0xcf, 0xf0, 0xfe, 0x3f, 0x07, 0xff, 0xf3, 0xf0, 0x7f, 0x8f, 0xff, 0xff, 0xfe, 0x1f, 0xc3, 0xcf,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0x07, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0x0f,
    0xcf, 0xf0, 0x7e, 0x3f, 0x87, 0xff, 0xf3, 0xf0, 0x7f, 0xc7, 0xff, 0xff, 0xfe, 0x0f, 0x83, 0xcf,
    0xff, 0xff, 0xff, 0xff, 0xfe, 0x1f, 0x03, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0x0f,
    0xcf, 0xf0, 0x7c, 0x3f, 0x87, 0xff, 0xf3, 0xf8, 0x7f, 0xc7, 0xff, 0xff, 0xfe, 0x00, 0x07, 0x8f,
    0xff, 0xff, 0xff, 0xff, 0xfc, 0x1f, 0x03, 0xfd, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xfc, 0x7f, 0x87,
    0xcf, 0xf0, 0xfc, 0x3f, 0x87, 0xff, 0xe3, 0xf8, 0x3f, 0xc7, 0xff, 0xff, 0xff, 0x00, 0x07, 0x8f,
    0xff, 0xff, 0xff, 0xff, 0xf8, 0x1f, 0x81, 0xf9, 0xff, 0xe7, 0xff, 0xfe, 0xff, 0xf8, 0x7f, 0x87,
    0xcf, 0xf0, 0xfc, 0x3f, 0x83, 0xff, 0xe3, 0xf8, 0x3f, 0xc3, 0xff, 0xff, 0xff, 0x00, 0x0f, 0x8f,
    0xff, 0xff, 0xff, 0xff, 0xf0, 0x3f, 0x81, 0xf9, 0xff, 0xef, 0xff, 0xfe, 0xff, 0xf0, 0x7f, 0x87,
    0xc7, 0xf0, 0xfc, 0x3f, 0x87, 0xff, 0xe3, 0xf8, 0x3f, 0xc3, 0xff, 0xff, 0xff, 0x00, 0x0f, 0x87,
    0xff, 0xff, 0xef, 0xff, 0xe0, 0x3f, 0x81, 0xf9, 0xff, 0xef, 0xff, 0xfc, 0xff, 0xe0, 0x7f, 0x87,
    0xc3, 0xf0, 0xfe, 0x1f, 0x07, 0xdf, 0xc3, 0xf8, 0x3f, 0xc3, 0xff, 0xff, 0xff, 0x80, 0x1f, 0x87,
    0xff, 0xff, 0xef, 0xff, 0xe0, 0x3f, 0x81, 0xf1, 0xff, 0xcf, 0xff, 0xfc, 0xff, 0xe0, 0x7f, 0x83,
    0xc1, 0xe1, 0xfe, 0x04, 0x07, 0xdf, 0xc3, 0xf0, 0x3f, 0xc3, 0xff, 0xff, 0xff, 0x80, 0x1f, 0x83,
    0xff, 0xff, 0xcf, 0xff, 0xc0, 0x7f, 0x83, 0xf1, 0xff, 0xcf, 0xff, 0xf8, 0xff, 0xc0, 0x7f, 0x83,
    0xc0, 0x01, 0xfe, 0x00, 0x07, 0x9f, 0xc3, 0xf0, 0x3f, 0xc3, 0xff, 0xff, 0xff, 0x80, 0x1f, 0xc3,
    0xff, 0xff, 0xcf, 0xff, 0xc0, 0x7f, 0x83, 0xf1, 0xff, 0xcf, 0xf7, 0xf8, 0xff, 0xc0, 0x7f, 0x03,
    0xc0, 0x01, 0xff, 0x00, 0x0f, 0x9f, 0xc3, 0xf0, 0x3f, 0x83, 0xff, 0xff, 0x7f, 0xc0, 0x1f, 0xc1,
    0xff, 0xff, 0x8f, 0xff, 0xc0, 0x7f, 0x83, 0xf1, 0xff, 0xcf, 0xf7, 0xf8, 0xff, 0xc0, 0x7e, 0x03,
    0xe0, 0x01, 0xff, 0x00, 0x0f, 0xbf, 0xc3, 0xe0, 0x1f, 0x07, 0xef, 0xff, 0x7f, 0xc0, 0x1f, 0xc1,
    0xff, 0xff, 0x9f, 0xff, 0xc0, 0x7f, 0x83, 0xf1, 0xff, 0x8f, 0xe7, 0xf8, 0xff, 0xc0, 0x3c, 0x07,
    0xe0, 0x03, 0xff, 0x00, 0x1f, 0x3f, 0xc1, 0xc0, 0x0e, 0x07, 0xcf, 0xfe, 0x7f, 0xe0, 0x1f, 0xc1,
    0xff, 0xff, 0x9f, 0xff, 0xc0, 0x3f, 0x07, 0xf1, 0xff, 0x8f, 0xef, 0xf0, 0x7f, 0xc0, 0x00, 0x07,
    0xf0, 0x03, 0xff, 0x80, 0x1f, 0x3f, 0xc0, 0x00, 0x00, 0x07, 0x9f, 0xfe, 0x7f, 0xe0, 0x0f, 0x81,
    0xf7, 0xff, 0x1f, 0xff, 0xc0, 0x3e, 0x07, 0xf0, 0xff, 0x8f, 0xcf, 0xf0, 0x7f, 0xe0, 0x00, 0x07,
    0xf8, 0x03, 0xff, 0x80, 0x3f, 0x3f, 0xc0, 0x00, 0x00, 0x0f, 0x9f, 0xfc, 0x7f, 0xe0, 0x07, 0x01,
    0xf7, 0xff, 0x1f, 0xff, 0xc0, 0x00, 0x07, 0xf0, 0xff, 0x8f, 0xcf, 0xf0, 0x3f, 0xe0, 0x00, 0x0f,
    0xf8, 0x01, 0xff, 0x80, 0x3f, 0x3f, 0xe0, 0x00, 0x00, 0x1f, 0x9f, 0xfc, 0x7f, 0xf0, 0x00, 0x01,
    0xe7, 0xff, 0x1f, 0xff, 0xc0, 0x00, 0x0f, 0xf0, 0x7f, 0x8f, 0xcf, 0xf8, 0x3f, 0xf0, 0x00, 0x1f,
    0xfc, 0x01, 0xff, 0x80, 0x7f, 0x1f, 0xe0, 0x00, 0x00, 0x1f, 0x1f, 0xfc, 0x7f, 0xe0, 0x00, 0x01,
    0xe7, 0xfe, 0x1f, 0xff, 0xe0, 0x00, 0x0f, 0xf0, 0x7f, 0x8f, 0xc7, 0xf8, 0x1f, 0xf8, 0x00, 0x1f,
    0xfc, 0x01, 0xff, 0x80, 0x7f, 0x1f, 0xe0, 0x00, 0x00, 0x3f, 0x1f, 0xfc, 0x7f, 0xe0, 0x00, 0x01,
    0xe7, 0xfe, 0x1f, 0xff, 0xe0, 0x00, 0x0f, 0xf0, 0x3f, 0x8f, 0xc7, 0xf8, 0x0f, 0xf8, 0x00, 0x3f,
    0xfc, 0x00, 0xff, 0x80, 0xff, 0x8f, 0xf0, 0x00, 0x00, 0x7f, 0x0f, 0xf8, 0x3f, 0xe0, 0x00, 0x01,
    0xc7, 0xfe, 0x1f, 0xff, 0xf0, 0x00, 0x1f, 0xf0, 0x1f, 0x87, 0xc3, 0xfc, 0x0f, 0xfc, 0x00, 0x3f,
    0xfc, 0x00, 0x7f, 0x00, 0xff, 0x8f, 0xf0, 0x00, 0x00, 0x7f, 0x8f, 0xf8, 0x3f, 0xe0, 0x00, 0x03,
    0xc7, 0xfe, 0x1f, 0xff, 0xf8, 0x00, 0x1f, 0xf0, 0x1f, 0x87, 0xe3, 0xfe, 0x07, 0xfe, 0x00, 0x7f,
    0xfc, 0x00, 0x3c, 0x00, 0xff, 0x87, 0xf0, 0x00, 0x00, 0xff, 0x87, 0xf8, 0x1f, 0xc0, 0x00, 0x03,
    0xc7, 0xfe, 0x07, 0xff, 0xfc, 0x00, 0x0f, 0xf0, 0x0f, 0x87, 0xe1, 0xfe, 0x07, 0xfe, 0x00, 0x7f,
    0xf8, 0x00, 0x00, 0x00, 0xff, 0x87, 0xf8, 0x00, 0x00, 0xff, 0x83, 0xf8, 0x07, 0x00, 0x00, 0x07,
    0xc3, 0xfe, 0x00, 0xff, 0xfe, 0x00, 0x0f, 0xe0, 0x0f, 0xc3, 0xe0, 0xfe, 0x03, 0xfe, 0x00, 0x7f,
    0xf0, 0x00, 0x00, 0x00, 0xff, 0x87, 0xf8, 0x00, 0x00, 0xff, 0xc3, 0xf8, 0x00, 0x00, 0x00, 0x0f,
    0xc1, 0xfe, 0x00, 0x3f, 0xfe, 0x00, 0x07, 0xc0, 0x0f, 0xc3, 0xe0, 0xfe, 0x03, 0xfe, 0x00, 0x7f,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x83, 0xfc, 0x00, 0x00, 0xff, 0xc1, 0xf8, 0x00, 0x00, 0x00, 0x0f,
    0xe1, 0xfe, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x0f, 0xc1, 0xc0, 0x7f, 0x03, 0xfe, 0x00, 0x3f,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x83, 0xfc, 0x00, 0x00, 0xff, 0xc1, 0xfc, 0x00, 0x00, 0x00, 0x0f,
    0xe0, 0xff, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x1f, 0xc0, 0x00, 0xff, 0x03, 0xfe, 0x00, 0x1e,
    0x00, 0x00, 0x00, 0x00, 0x7f, 0x83, 0xfc, 0x00, 0x00, 0xff, 0x81, 0xfc, 0x00, 0x00, 0x00, 0x1f,
    0xe0, 0x7f, 0x00, 0x1f, 0xff, 0x00, 0x00, 0x00, 0x1f, 0xe0, 0x00, 0xff, 0x03, 0xfe, 0x00, 0x0c,
    0x00, 0x00, 0x00, 0x00, 0x3f, 0x03, 0xfe, 0x00, 0x00, 0xff, 0x81, 0xfe, 0x00, 0x00, 0x00, 0x1f,
    0xe0, 0x7f, 0x80, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x3f, 0xe0, 0x00, 0xff, 0x03, 0xfe, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x7f, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x1f,
    0xe0, 0x3f, 0x80, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x3f, 0xe0, 0x01, 0xff, 0x03, 0xfc, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x7e, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x0f,
    0xe0, 0x3f, 0xc0, 0x0f, 0xff, 0x00, 0x00, 0x00, 0x7f, 0xf0, 0x01, 0xff, 0x01, 0xf8, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x1c, 0x01, 0xff, 0x00, 0x00, 0x00, 0x0f,
    0xc0, 0x3f, 0xe0, 0x0f, 0xff, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x01, 0xff, 0x01, 0xf0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x03,
    0x80, 0x3f, 0xe0, 0x0f, 0xff, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x01, 0xfe, 0x00, 0xe0, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00, 0x00, 0x01, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3f, 0xe0, 0x0f, 0xfe, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x01, 0xfe, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x3f, 0xf0, 0x07, 0xfe, 0x00, 0x00, 0x00, 0xff, 0xf8, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x03, 0xff, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x3f, 0xf0, 0x07, 0xfc, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfc, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x7f, 0xf0, 0x03, 0xf8, 0x00, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3f, 0xe0, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x03, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xfe, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xfc, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 //
};

#endif
