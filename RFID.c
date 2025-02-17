#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "inc/ssd1306.h"
#include "MFRC522.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

// Credenciais de Wi-Fi e ThingSpeak
#define WIFI_SSID "login do wifi"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "senha do wifi"     // Substitua pela senha da sua rede Wi-Fi
#define CHANNEL_API_KEY "sua api do thingspeak" //API da channel que você está utilizando

static char servidor[512] = "Aguardando resposta do servidor...";

#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19
#define PIN_RST  4

#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER 21

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

uint8_t ssd[ssd1306_buffer_length];
struct render_area frame_area;

char tag_id[20] = "Nenhuma tag lida";

// Função para inicializar o Wi-Fi
bool init_wifi() {
    if (cyw43_arch_init()) {
        printf("Erro ao inicializar o Wi-Fi\n");
        return false;
    }

    cyw43_arch_enable_sta_mode(); // Habilita o modo estação (conectar a um Wi-Fi)

    printf("Conectando ao Wi-Fi: %s...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Falha ao conectar ao Wi-Fi\n");
        return false;
    }

    printf("Conectado ao Wi-Fi!\n");
    return true;
}

void init_display(uint8_t *ssd, struct render_area *frame_area) {
    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    *frame_area = (struct render_area){
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(frame_area);
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, frame_area);
}

void exibir_texto(uint8_t *ssd, struct render_area *frame_area, char *text[], uint linhas) {
    memset(ssd, 0, ssd1306_buffer_length);

    int y = 0;

    for (uint i = 0; i < linhas; i++) {
        ssd1306_draw_string(ssd, 3, y, text[i]);
        y += 8;
    }

    render_on_display(ssd, frame_area);
}

void controle_LED_Buzzer(int tipo) {
    switch (tipo) {
        case 1:  // Aguardando
            gpio_put(LED_R, 1);
            gpio_put(LED_B, 1);
            gpio_put(BUZZER, 0);
            break;
        case 2:  // Sucesso
            gpio_put(LED_R, 0);
            gpio_put(LED_G, 1);
            gpio_put(LED_B, 0); 
            gpio_put(BUZZER, 1);
            sleep_ms(1000);
            gpio_put(BUZZER, 0);
            break;
        case 3:  // Falha
            gpio_put(LED_R, 1);
            gpio_put(LED_G, 0);
            gpio_put(LED_B, 0); 
            gpio_put(BUZZER, 1);
            sleep_ms(1000);
            gpio_put(BUZZER, 0);
            break;
    }
}

void send_data_to_thingspeak(const char *tag_id) {
    char post_data[256];
    snprintf(post_data, sizeof(post_data), "api_key=%s&field1=%s", CHANNEL_API_KEY, tag_id);

    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }

    ip_addr_t ip;
    IP4_ADDR(&ip, 184, 106, 153, 149);

    err_t err = tcp_connect(pcb, &ip, 80, NULL);
    if (err != ERR_OK) {
        printf("Erro ao conectar ao ThingSpeak\n");
        tcp_close(pcb);
        return;
    }

    char request[512];
    snprintf(request, sizeof(request),
             "POST /update.json HTTP/1.1\r\n"
             "Host: api.thingspeak.com\r\n"
             "Connection: close\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s",
             (int)strlen(post_data), post_data);

    err_t send_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (send_err != ERR_OK) {
        printf("Erro ao enviar os dados para o ThingSpeak\n");
        tcp_close(pcb);
        return;
    }

    tcp_output(pcb);
    printf("Dados enviados para ThingSpeak: %s\n", tag_id);
}

void Tag(MFRC522Ptr_t mfrc522) {
    uint8_t success;
    uint8_t tagUID[10];

    memset(tagUID, 0, sizeof(tagUID));

    if (PICC_IsNewCardPresent(mfrc522)) {
        if (PICC_ReadCardSerial(mfrc522)) {
            printf("Tag detectada! UID: ");
            for (uint8_t i = 0; i < mfrc522->uid.size; i++) {
                tagUID[i] = mfrc522->uid.uidByte[i];
                printf("%X ", tagUID[i]);
            }
            printf("\n");

            snprintf(tag_id, sizeof(tag_id), "%02X%02X%02X%02X",
                     tagUID[0], tagUID[1], tagUID[2], tagUID[3]);

            char *success_text[] = {"Registro do Id", "realizado com" , "sucesso!"};
            exibir_texto(ssd, &frame_area, success_text, 3);
            controle_LED_Buzzer(2);

            // Envia os dados para o ThingSpeak
            send_data_to_thingspeak(tag_id);
        } else {
            char *failure_text[] = {"Falha na leitura da tag"};
            exibir_texto(ssd, &frame_area, failure_text, 1);
            controle_LED_Buzzer(3);
        }
    } else {
        char *waiting_text[] = {"Aguardando tag..."};
        exibir_texto(ssd, &frame_area, waiting_text, 1);
        controle_LED_Buzzer(1);
    }
}

int main() {
    stdio_init_all();

    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    init_display(ssd, &frame_area);

    char *initial_text[] = {"Aguardando tag..."};
    exibir_texto(ssd, &frame_area, initial_text, 1);
    controle_LED_Buzzer(1);

    if (!init_wifi()) {
        printf("Falha ao conectar ao Wi-Fi. Verifique as credenciais.\n");
        while (1) {
            controle_LED_Buzzer(3);
            sleep_ms(1000);
        }
    }

    MFRC522Ptr_t mfrc522 = MFRC522_Init();
    PCD_Init(mfrc522, SPI_PORT);

    while (true) {
        sleep_ms(2000);
        Tag(mfrc522);
    }

    return 0;
}
