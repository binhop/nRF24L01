#ifndef RF24L01_H
#define RF24L01_H

#include <SPI.h>

// Registradores do nRF24L01
#define RF_NRF_CONFIG    0x00
#define RF_EN_AA         0x01
#define RF_EN_RXADDR     0x02
#define RF_SETUP_RETR    0x04
#define RF_CH            0x05
#define RF_SETUP         0x06
#define RF_NRF_STATUS    0x07
#define RF_RX_ADDR_P0    0x0A
#define RF_TX_ADDR       0x10
#define RF_RX_PW_P0      0x11
#define RF_FIFO_STATUS   0x17
#define RF_FEATURE       0x1D

// Instruções SPI
#define CMD_ACTIVATE      0x50 //
#define CMD_R_RX_PAYLOAD  0x61 //
#define CMD_W_TX_PAYLOAD  0xA0 //
#define CMD_FLUSH_TX      0xE1 //
#define CMD_FLUSH_RX      0xE2 //
#define CMD_RF24_NOP      0xFF //

// Pinos da comunicação SPI
#define CE_PIN  9
#define CSN_PIN 10

/*
   Funções da comunicação SPI
*/
uint8_t RF24L01_SPI_Cmd(uint8_t addrs, uint8_t cmd);
void RF24L01_SPI_Write(uint8_t address, uint8_t cmd);
void RF24L01_SPI_Write_N(uint8_t address, uint8_t *cmd, uint8_t n);
uint8_t RF24L01_SPI_Read(uint8_t address, uint8_t cmd);


/*
   Funções de configuração
*/
void RF24L01_Activate(void);
void RF24L01_Flush_Tx(void);
void RF24L01_Flush_Rx(void);
uint8_t RF24L01_Status(void);
void RF24L01_PowerUp(void);
uint8_t RF24L01_Begin(uint8_t *chave);


/*
   Funções de transmissao e recepção
*/
void RF24L01_Start_Listen(void);
void RF24L01_Stop_Listen(void);
uint8_t RF24L01_Send_Payload(uint8_t *msg, uint8_t n);
uint8_t RF24L01_Read_Payload(uint8_t *msg, uint8_t n);
void RF24L01_Send(uint8_t *msg, uint8_t n);
void RF24L01_Receive(uint8_t *msg, uint8_t n);

#endif // RF24L01_H
