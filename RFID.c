#include <stdio.h>
#include <string.h>  
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "inc/ssd1306.h"
#include "MFRC522.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"

#define WIFI_SSID "JesusVaiVoltar_5G"  // Substitua pelo nome da sua rede Wi-Fi
#define WIFI_PASS "25300607"     // Substitua pela senha da sua rede Wi-Fi
#define CHANNEL_API_KEY "7QSQSIQ1QBXAHIWX" 
static char servidor[512] = "Aguardando resposta do servidor...";

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
    i2c_init(i2c1, 400000);  // 400 kHz
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


// Callback para processar a resposta do servidor
static err_t receive_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Armazena a resposta para exibição no servidor HTTP
    snprintf(servidor, sizeof(servidor), "Resposta do ThingSpeak: %.*s", p->len, (char *)p->payload);
    
    // Liberar o buffer e fechar conexão
    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

static void send_data_to_thingspeak() {
    char post_data[256];

    // Formato correto: "api_key=XXXX&field1=XX&field2=XX"
    snprintf(post_data, sizeof(post_data),
             "api_key=7QSQSIQ1QBXAHIWX",
             CHANNEL_API_KEY);

    // Criar conexão TCP
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB TCP\n");
        return;
    }

    // Endereço IP do ThingSpeak (api.thingspeak.com)
    ip_addr_t ip;
    IP4_ADDR(&ip, 184, 106, 153, 149);

    // Conectar ao servidor na porta 80
    err_t err = tcp_connect(pcb, &ip, 80, NULL);
    if (err != ERR_OK) {
        printf("Erro ao conectar ao ThingSpeak\n");
        tcp_close(pcb);
        return;
    }

    // Montar a requisição HTTP
    char request[512];
    snprintf(request, sizeof(request),
             "POST /update.json HTTP/1.1\r\n"
             "Host: api.thingspeak.com\r\n"
             "Connection: close\r\n"
             "Content-Type: application/x-www-form-urlencoded\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s",
             (int)strlen(post_data), post_data);

    // Enviar a requisição
    err_t send_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    if (send_err != ERR_OK) {
        printf("Erro ao enviar os dados para o ThingSpeak\n");
        tcp_close(pcb);
        return;
    } else{
        printf("Dados enviados para ThingSpeak\n");        
    }

    // Garantir que os dados sejam enviados
    tcp_output(pcb);

    // Registrar callback para receber resposta
    tcp_recv(pcb, receive_callback);
}

// Callback para responder requisições HTTP
static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) {
        tcp_close(tpcb);
        return ERR_OK;
    }
}


//função para criação para a resposta do HTTP


//

// Callback de conexão para o servidor HTTP
static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_callback);  // Associa o callback HTTP
    return ERR_OK;
}

// Função de setup do servidor HTTP
static void start_http_server(void) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) {
        printf("Erro ao criar PCB\n");
        return;
    }

    // Liga o servidor na porta 80
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);  // Coloca o PCB em modo de escuta
    tcp_accept(pcb, connection_callback);  // Associa o callback de conexão

    printf("Servidor HTTP rodando na porta 80...\n");
}



// Função para ler a tag RFID
void Tag(MFRC522Ptr_t mfrc522) {
    uint8_t success;
    uint8_t tagUID[10];

    // Limpa o buffer do RFID antes de ler uma nova tag
    memset(tagUID, 0, sizeof(tagUID));

    // Verifica se uma nova tag está presente
    if (PICC_IsNewCardPresent(mfrc522)) {
        // Tenta ler o UID da tag
        if (PICC_ReadCardSerial(mfrc522)) {
            printf("Tag detectada! UID: ");
            for (uint8_t i = 0; i < mfrc522->uid.size; i++) {
                tagUID[i] = mfrc522->uid.uidByte[i];
                printf("%X ", tagUID[i]);
            }
            printf("\n");

            char *success_text[] = {"Registro do Id", "realizado com" , "sucesso!"};
            exibir_texto(ssd, &frame_area, success_text, 3);
            controle_LED_Buzzer(2);
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
    MFRC522Ptr_t mfrc522 = MFRC522_Init();
    PCD_Init(mfrc522, SPI_PORT);

    // Loop principal
    while (true) {
        sleep_ms(2000);  // Aguarda 2 segundos antes de ler a tag
        Tag(mfrc522);   // Chama a função de leitura e registro
    }

    return 0;
}