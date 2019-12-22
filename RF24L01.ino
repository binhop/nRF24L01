/*****************************************************************************
 *   Funções da comunicação SPI
 *****************************************************************************/
void RF24L01_SPI_Init()
{
  digitalWrite(CSN_PIN, LOW);
}

void RF24L01_SPI_End()
{
  digitalWrite(CSN_PIN, HIGH);
}

/*
 * Envia um endereço e um comando e retorna uma resposta
 */
uint8_t RF24L01_SPI_Cmd(uint8_t address, uint8_t cmd)
{
  RF24L01_SPI_Init();
  SPI.transfer(address);
  uint8_t response = SPI.transfer(cmd);
  RF24L01_SPI_End();

  return response;
}

/*
 * Escreve um valor em um registrador do RF24L01
 */
void RF24L01_SPI_Write(uint8_t address, uint8_t cmd)
{
  // Comando de escrita: 001X XXXX
  RF24L01_SPI_Cmd(0x20 | (0x1F & address), cmd);
}

/*
 * Escreve n valores em um registrador do RF24L01
 */
void RF24L01_SPI_Write_N(uint8_t address, uint8_t *cmd, uint8_t n)
{
  // Escrita de N bytes
  RF24L01_SPI_Init();
  
  SPI.transfer(0x20 | (0x1F & address));

  while(n--)
  {
    SPI.transfer(*cmd++);
  }

  RF24L01_SPI_End();
}

/*
 * Lê um valor em um registrador do RF24L01
 */
uint8_t RF24L01_SPI_Read(uint8_t address)
{
  // Comando de leitura: 000X XXXX
  return RF24L01_SPI_Cmd((0x1F & address), 0);
}

/*DEBUG
 void RF24L01_SPI_Read_N(uint8_t address, uint8_t tam)
{
  // Leitura de N bytes
  RF24L01_SPI_Init();
  
  SPI.transfer(0x1F & address);

  while (tam--)
  {
    Serial.println(SPI.transfer(0));
  }

  RF24L01_SPI_End();
}*/


/*****************************************************************************
 *   Funções de configuração
 *****************************************************************************/

/*
 * Ativa o uso dos seguintes comandos do RF24L01: 
 * R_RX_PL_WID (Lê tamanho do payload)
 * W_ACK_PAYLOAD (Escrever payload)
 * W_TX_PAYLOAD_NOACK (Desabilita o AUTOACK)
 */
void RF24L01_Activate(void)
{
  RF24L01_SPI_Cmd(CMD_ACTIVATE, 0x73);
}

/*
 * Limpa o FIFO Tx
 */
void RF24L01_Flush_Tx(void)
{
  RF24L01_SPI_Init();
  SPI.transfer(CMD_FLUSH_TX);
  RF24L01_SPI_End();
}

/*
 * Limpa o FIFO Rx
 */
void RF24L01_Flush_Rx(void)
{
  // Comando para limpar o FIFO Rx
  RF24L01_SPI_Init();
  SPI.transfer(CMD_FLUSH_RX);
  RF24L01_SPI_End();
}

/*
 * Retorna o valor do registrador de status
 * Isto é feito enviando o comando NOP
 */
uint8_t RF24L01_Status(void)
{
  uint8_t status;
  RF24L01_SPI_Init();
  status = SPI.transfer(CMD_RF24_NOP);
  RF24L01_SPI_End();

  return status;
}

/*
 * Inicializa o modo power up do RF24L01
 */
void RF24L01_PowerUp(void)
{
  uint8_t cfg = RF24L01_SPI_Read(RF_NRF_CONFIG);

  // Só ativa o modo power up se já não estiver ativo
  if (!(cfg & 1)) {
    RF24L01_SPI_Write(RF_NRF_CONFIG, cfg | 2);

    // Aguarda um tempo para o rádio inicializar por completo
    delay(5);
  }
}

/*
 * Configure e inicializa vários parâmetros do RF24L01
 * e da comunicação SPI
 */
uint8_t RF24L01_Begin(uint8_t *chave)
{
  // Configura a comunicação SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  // Define o pino CSN como saida
  pinMode(CE_PIN, OUTPUT);
  pinMode(CSN_PIN, OUTPUT);
  digitalWrite(CE_PIN, LOW);
  digitalWrite(CSN_PIN, HIGH);

  delay(5); // Aguarda um tempo para o rádio inicializar por completo

  // Habilita CRC de 16 bits e coloca no modo PWR_DOWN
  RF24L01_SPI_Write(RF_NRF_CONFIG, 0x0C);


  // Auto retransmissão:
  // 15 retransmissões com delay de 1,5ms entre cada
  RF24L01_SPI_Write(RF_SETUP_RETR, (5 & 0xf) << 4 | (15 & 0xf));
  // Sem retransmissão
  //RF24L01_SPI_Write(RF_SETUP_RETR, 0);

  // Configura comunicação em 1 Mbps e Pt em -18dBm com LowNoiseAmplifier
  uint8_t setup = RF24L01_SPI_Read(RF_SETUP);
  RF24L01_SPI_Write(RF_SETUP, (setup & 0xF1));
  setup = RF24L01_SPI_Read(RF_SETUP);

  // Desabilita o ACK
  //RF24L01_SPI_Write(0x1, 0);

  // Ativa comandos adicionais do RF24L01
  RF24L01_Activate();

  // Reseta o status atual da transmissão/recepção
  RF24L01_SPI_Write(RF_NRF_STATUS, _BV(6) | _BV(5) | _BV(4));


  // Define o canal (76) da comunicação
  RF24L01_SPI_Write(RF_CH, 76);

  // Inicializa
  RF24L01_PowerUp();

  // Enable PTX, do not write CE high so radio will remain in standby I mode ( 130us max to transition to RX or TX instead of 1500us from powerUp )
  // PTX should use only 22uA of power
  RF24L01_SPI_Write(RF_NRF_CONFIG, (RF24L01_SPI_Read(RF_NRF_CONFIG)) & ~(1));

  // Configura as chaves de transmissão e recepção
  RF24L01_SPI_Write_N(RF_RX_ADDR_P0, chave, 5);
  RF24L01_SPI_Write_N(RF_TX_ADDR, chave, 5);
  // Configura o tamanho do payload do pipe 0
  RF24L01_SPI_Write(RF_RX_PW_P0, 32);

  // Retorna 1 se o setup lido for diferente de 0 ou 0xFF
  return (setup != 0 && setup != 0xFF);
}


/*****************************************************************************
 *  Funções de transmissao e recepção
 *****************************************************************************/
/*
 * Prepara o RF24L01 para receber mensagens
 */
void RF24L01_Start_Listen(void)
{
  // Ativa o recebimento
  RF24L01_SPI_Write(RF_NRF_CONFIG, RF24L01_SPI_Read(RF_NRF_CONFIG) | 1);
  
  // Limpa as flags
  RF24L01_SPI_Write(RF_NRF_STATUS, _BV(6) | _BV(5) | _BV(4));

  // CE_PIN em 1 para não entrar no modo stadby II
  digitalWrite(CE_PIN, HIGH);

  //RF24L01_SPI_Write_N(RF_RX_ADDR_P0, "chave", 5);

  // Se o payload com ACK está habilitado
  if (RF24L01_SPI_Read(RF_FEATURE) & _BV(1)) {
    RF24L01_Flush_Tx();
  }

  // Habilita RX no pipe 0
  RF24L01_SPI_Write(RF_EN_RXADDR, RF24L01_SPI_Read(RF_EN_RXADDR) | 1);
}

/*
 * Pausa o recebimento de mensagens e prepara para o modo de envio (fica no standby I)
 */
void RF24L01_Stop_Listen(void)
{
  digitalWrite(CE_PIN, LOW);

  // Ativa o modo envio
  RF24L01_SPI_Write(RF_NRF_CONFIG, RF24L01_SPI_Read(RF_NRF_CONFIG) & 0xFE);

  // Se o payload com ACK está habilitado
  if (RF24L01_SPI_Read(RF_FEATURE) & _BV(1)) {
    delayMicroseconds(200);
    RF24L01_Flush_Tx();
  }

  // Habilita RX no pipe 0
  //RF24L01_SPI_Write(RF_EN_RXADDR, RF24L01_SPI_Read(RF_EN_RXADDR) | 1);
}

/*
 * Envia um conjunto de n bytes para o FIFO Tx
 */
uint8_t RF24L01_Send_Payload(uint8_t *msg, uint8_t n)
{
  uint8_t status;

  n = n <= 32 ? n : 32;

  RF24L01_SPI_Init();

  status = SPI.transfer(CMD_W_TX_PAYLOAD);

  while (n--)
  {
    SPI.transfer(*msg++);
  }

  RF24L01_SPI_End();


  return status;
}

/*
 * Lê um conjunto de n bytes do FIFO Rx
 */
uint8_t RF24L01_Read_Payload(uint8_t *msg, uint8_t n)
{
  uint8_t status;

  n = n <= 32 ? n : 32;

  RF24L01_SPI_Init();

  status = SPI.transfer(CMD_R_RX_PAYLOAD);

  while (n--)
  {
    *msg++ = SPI.transfer(0xFF);
  }

  RF24L01_SPI_End();
  
  return status;
}

/*
 * Transmite uma mensagem de tamanho n
 */
void RF24L01_Send(uint8_t *msg, uint8_t n)
{
  // Começa a escrita
  RF24L01_Send_Payload(msg, n) ;

  digitalWrite(CE_PIN, HIGH);

  // Aguarda terminar o envio
  while (!(RF24L01_Status()  & (_BV(5) | _BV(4))));

  digitalWrite(CE_PIN, LOW);
  uint8_t status = RF24L01_Status();
  // Limpa as flags
  RF24L01_SPI_Write(RF_NRF_STATUS, _BV(6) | _BV(5) | _BV(4));

  if( status & _BV(4)){
    RF24L01_Flush_Tx(); //Only going to be 1 packet int the FIFO at a time using this method, so just flush
    return 0;
  }
  return 1;
}

/*
 * Informa se há dados disponíveis para leitura
 */
uint8_t RF24L01_Available()
{
  // Verifica se o FIFO de recebimento não está vazio
  if (!(RF24L01_SPI_Read(RF_FIFO_STATUS) & 1))
  {
    return 1;
  }

  return 0;
}

/*
 * Lê n bytes do FIFO de recebimento do PIPE 0
 */
void RF24L01_Receive(uint8_t *msg, uint8_t n)
{
  RF24L01_Read_Payload(msg, n);

  // Limpa as flags
  RF24L01_SPI_Write(RF_NRF_STATUS, _BV(6) | _BV(5) | _BV(4));
}
