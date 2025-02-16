#include <stdio.h>
#include <string.h>  
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "inc/ssd1306.h"
#include "MFRC522.h"

// Definições dos pinos SPI
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  4

// Definições dos pinos dos LEDs e Buzzer
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER 21

// Definições do I2C para o OLED
const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Buffer para o display OLED
uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;

// Função para inicializar o display OLED
void init_display(uint8_t *ssd, struct render_area *frame_area) {
    // Inicialização do I2C
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display SSD1306
    ssd1306_init();

    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
    *frame_area = (struct render_area){
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    // Calcula o tamanho do buffer para a área de renderização
    calculate_render_area_buffer_length(frame_area);

    // Zera o display inteiro
    memset(ssd, 0, ssd1306_buffer_length);

    // Atualiza o display com o conteúdo vazio
    render_on_display(ssd, frame_area);
}

// Função para exibir texto no display OLED
void exibir_texto(uint8_t *ssd, struct render_area *frame_area, char *text[], uint linhas) {
    // Limpa o buffer do display
    memset(ssd, 0, ssd1306_buffer_length);

    int y = 0;  // Posição inicial no eixo Y

    // Exibe as linhas de texto no display
    for (uint i = 0; i < linhas; i++) {
        ssd1306_draw_string(ssd, 3, y, text[i]);  // Desenha cada linha de texto
        y += 8;  // Move para a próxima linha (cada linha tem 8 pixels de altura)
    }

    // Atualiza o display com o novo conteúdo
    render_on_display(ssd, frame_area);
}

// Função para controlar LEDs e Buzzer
void controle_LED_Buzzer(int tipo) {
    switch (tipo) {
        case 1:  // Aguardando
            gpio_put(LED_R, 1);
            gpio_put(LED_B, 1);
            gpio_put(BUZZER, 0);
            break;
        case 2:  // Sucesso
            gpio_put(LED_G, 1);
            gpio_put(LED_R, 0);
            gpio_put(BUZZER, 1);
            sleep_ms(1000);
            gpio_put(BUZZER, 0);
            break;
        case 3:  // Falha
            gpio_put(LED_R, 1);
            gpio_put(LED_G, 0);
            gpio_put(BUZZER, 1);
            sleep_ms(1000);
            gpio_put(BUZZER, 0);
            break;
    }
}

// Função para ler a tag RFID
void Tag(MFRC522 *mfrc522) {
    uint8_t success;
    uint8_t tagUID[10];

    // Limpa o buffer do RFID antes de ler uma nova tag
    memset(tagUID, 0, sizeof(tagUID));

    success = MFRC522_read_card(mfrc522);
    if (success == 0) {
        printf("Tag detectada! UID: ");
        for (uint8_t i = 0; i < mfrc522->uid_size; i++) {
            tagUID[i] = mfrc522->uid[i];
            printf("%X ", tagUID[i]);
        }
        printf("\n");

        char *success_text[] = {"Registro do Id", "realizado com sucesso!"};
        exibir_texto(ssd, &frame_area, success_text, 2);
        controle_LED_Buzzer(2);
    } else {
        char *failure_text[] = {"Falha no registro"};
        exibir_texto(ssd, &frame_area, failure_text, 1);
        controle_LED_Buzzer(3);
    }
}

// Função principal
int main() {
    stdio_init_all();

    // Inicializa os LEDs e o Buzzer
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    // Inicializa o display OLED
    init_display(ssd, &frame_area);

    // Exibe a mensagem inicial no OLED
    char *initial_text[] = {"Aguardando tag..."};
    exibir_texto(ssd, &frame_area, initial_text, 1);
    controle_LED_Buzzer(1);

    // Inicializa o MFRC522 (RFID)
    MFRC522 mfrc522 = {PIN_SCK, PIN_MOSI, PIN_MISO, PIN_CS, PIN_RST};
    MFRC522_init(&mfrc522, PIN_SCK, PIN_MOSI, PIN_MISO, PIN_CS, PIN_RST);

    // Loop principal
    while (true) {
        sleep_ms(2000);  // Aguarda 2 segundos antes de ler a tag
        Tag(&mfrc522);   // Chama a função de leitura e registro
    }

    return 0;
}