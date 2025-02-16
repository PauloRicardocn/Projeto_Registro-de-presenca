#include "inc/ssd1306.h"
#include <string.h>

void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

void ssd1306_send_command(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // Co = 0, D/C = 0
    i2c_write_blocking(i2c1, SSD1306_I2C_ADDRESS, buf, 2, false);
}

void ssd1306_send_data(uint8_t *data, size_t length) {
    uint8_t buf[length + 1];
    buf[0] = 0x40;  // Co = 0, D/C = 1
    memcpy(buf + 1, data, length);
    i2c_write_blocking(i2c1, SSD1306_I2C_ADDRESS, buf, length + 1, false);
}

void ssd1306_init() {
    // Inicializa o display com comandos específicos
    uint8_t init_commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0x2E, 0xAF
    };
    for (int i = 0; i < sizeof(init_commands); i++) {
        ssd1306_send_command(init_commands[i]);
    }
    ssd1306_clear();
}

void ssd1306_clear() {
    uint8_t clear_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8] = {0};
    struct render_area area = {0, SSD1306_WIDTH - 1, 0, (SSD1306_HEIGHT / 8) - 1};
    calculate_render_area_buffer_length(&area);
    render_on_display(clear_buffer, &area);
}

void ssd1306_draw_char(uint8_t *buffer, int16_t x, int16_t y, char character) {
    if (character < 32 || character > 127) return;
    static const uint8_t font_5x7[] = {
        0x7C, 0x12, 0x11, 0x12, 0x7C,  // 'A'
        // Adicione outros caracteres conforme necessário
    };
    uint8_t char_index = character - 32;
    for (int i = 0; i < 5; i++) {
        uint8_t line = font_5x7[char_index * 5 + i];
        for (int j = 0; j < 7; j++) {
            if (line & 0x01) {
                buffer[(y + j) * SSD1306_WIDTH + x + i] |= (1 << (y % 8));
            }
            line >>= 1;
        }
    }
}

void ssd1306_draw_string(uint8_t *buffer, int16_t x, int16_t y, const char *string) {
    while (*string) {
        ssd1306_draw_char(buffer, x, y, *string);
        x += 6;
        string++;
    }
}

void render_on_display(uint8_t *buffer, struct render_area *area) {
    ssd1306_send_command(0x21);  // Defina coluna
    ssd1306_send_command(area->start_column);
    ssd1306_send_command(area->end_column);

    ssd1306_send_command(0x22);  // Defina página
    ssd1306_send_command(area->start_page);
    ssd1306_send_command(area->end_page);

    ssd1306_send_data(buffer, area->buffer_length);
}