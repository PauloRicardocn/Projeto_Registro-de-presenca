#include "MFRC522.h"

// Inicializa o MFRC522
void MFRC522_init(MFRC522 *device, uint sck, uint mosi, uint miso, uint cs, uint rst) {
    device->sck = sck;
    device->mosi = mosi;
    device->miso = miso;
    device->cs = cs;
    device->rst = rst;

    // Inicializa o SPI
    spi_init(SPI_PORT, 1000000);
    gpio_set_function(device->sck, GPIO_FUNC_SPI);
    gpio_set_function(device->mosi, GPIO_FUNC_SPI);
    gpio_set_function(device->miso, GPIO_FUNC_SPI);

    // Configura os pinos CS e RST
    gpio_init(device->cs);
    gpio_set_dir(device->cs, GPIO_OUT);
    gpio_put(device->cs, 1);

    gpio_init(device->rst);
    gpio_set_dir(device->rst, GPIO_OUT);
    gpio_put(device->rst, 1);

    // Reseta o MFRC522
    MFRC522_reset(device);
}

// Escreve em um registro do MFRC522
void MFRC522_write_register(MFRC522 *device, uint8_t reg, uint8_t val) {
    gpio_put(device->cs, 0);
    uint8_t data[2] = { (uint8_t)((reg << 1) & 0x7E), val };
    spi_write_blocking(SPI_PORT, data, 2);
    gpio_put(device->cs, 1);
}

// Lê de um registro do MFRC522
uint8_t MFRC522_read_register(MFRC522 *device, uint8_t reg) {
    gpio_put(device->cs, 0);
    uint8_t address = (uint8_t)(((reg << 1) & 0x7E) | 0x80);
    uint8_t value = 0;
    spi_write_blocking(SPI_PORT, &address, 1);
    spi_read_blocking(SPI_PORT, 0x00, &value, 1);
    gpio_put(device->cs, 1);
    return value;
}

// Reseta o MFRC522
void MFRC522_reset(MFRC522 *device) {
    gpio_put(device->rst, 0);
    sleep_ms(50);
    gpio_put(device->rst, 1);
    sleep_ms(50);
}

// Requisita uma tag (PICC_REQIDL)
uint8_t MFRC522_request(MFRC522 *device, uint8_t *buffer) {
    MFRC522_write_register(device, COMMAND_REG, PCD_IDLE);  // Para qualquer comando anterior
    MFRC522_write_register(device, BIT_FRAMING_REG, 0x07);  // Configura o tipo de frame

    // Envia o comando para requisitar uma tag
    MFRC522_write_register(device, COMMAND_REG, PICC_REQIDL);
    uint8_t status = MFRC522_read_register(device, COMM_IRQ_REG);  // Lê o status do IRQ

    if (status & 0x01) {  // Tag detectada
        return 0;
    }
    return 1;  // Nenhuma tag detectada
}

// Detecção de anticolisão (PICC_ANTICOLL)
uint8_t MFRC522_anticoll(MFRC522 *device, uint8_t *buffer) {
    MFRC522_write_register(device, COMMAND_REG, PCD_IDLE);  // Para qualquer comando anterior
    MFRC522_write_register(device, BIT_FRAMING_REG, 0x00);  // Limpa dados anteriores

    // Envia o comando de anticolisão
    MFRC522_write_register(device, COMMAND_REG, PICC_ANTICOLL);

    // Lê o UID da tag
    for (int i = 0; i < 4; i++) {
        buffer[i] = MFRC522_read_register(device, FIFO_DATA_REG);  // Lê do FIFO
    }

    return 0;  // Sem erro
}

// Lê o UID da tag
uint8_t MFRC522_read_card(MFRC522 *device) {
    uint8_t status;
    uint8_t buffer[4];

    status = MFRC522_request(device, buffer);  // Procura por uma tag
    if (status == 0) {
        status = MFRC522_anticoll(device, buffer);  // Detecção de anticolisão
        if (status == 0) {
            // Armazena o UID
            for (int i = 0; i < 4; i++) {
                device->uid[i] = buffer[i];
            }
            device->uid_size = 4;  // Tamanho do UID (4 bytes)
        }
    }
    return status;
}