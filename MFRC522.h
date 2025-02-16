#ifndef MFRC522_H
#define MFRC522_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

// Definições dos pinos SPI
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_RST  4

// Comandos e registros do MFRC522
#define PCD_IDLE        0x00
#define PCD_TRANSCEIVE  0x0C
#define FIFO_DATA_REG   0x09
#define FIFO_LEVEL_REG  0x0A
#define COMMAND_REG     0x01
#define COMM_IRQ_REG    0x04
#define DIV_IRQ_REG     0x05
#define ERROR_REG       0x06
#define CONTROL_REG     0x0C
#define BIT_FRAMING_REG 0x0D
#define STATUS2_REG     0x08
#define PICC_REQIDL     0x26  // Requisição de cartão em modo IDLE
#define PICC_ANTICOLL   0x93  // Detecção de anticolisão

// Estrutura do MFRC522
typedef struct {
    uint sck;
    uint mosi;
    uint miso;
    uint cs;
    uint rst;
    uint8_t uid[10];
    uint8_t uid_size;
} MFRC522;

// Protótipos das funções
void MFRC522_init(MFRC522 *device, uint sck, uint mosi, uint miso, uint cs, uint rst);
void MFRC522_write_register(MFRC522 *device, uint8_t reg, uint8_t val);
uint8_t MFRC522_read_register(MFRC522 *device, uint8_t reg);
void MFRC522_reset(MFRC522 *device);
uint8_t MFRC522_request(MFRC522 *device, uint8_t *buffer);
uint8_t MFRC522_anticoll(MFRC522 *device, uint8_t *buffer);
uint8_t MFRC522_read_card(MFRC522 *device);

#endif // MFRC522_H