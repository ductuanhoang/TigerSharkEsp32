#include "stm32_programmer.h"


#ifdef STM_FACTORY_BOOTLOADER
 
void stm32SendCommand(unsigned char commd) {    // Tested
  Serial1.write(commd);
  Serial1.write(~commd);
}

unsigned char stm32Erase() {     // Tested
  stm32SendCommand(STM32ERASE);
  while (!Serial1.available());
  if (Serial1.read() == STM32ACK)
  {
    Serial1.write(0xFF);
    Serial1.write(0x00);
  }
  else return STM32ERR;
  while (!Serial1.available());
  return Serial1.read();
}

unsigned char stm32Erasen() {     // Tested
  stm32SendCommand(STM32ERASEN);
  while (!Serial1.available());
  if (Serial1.read() == STM32ACK)
  {
    Serial1.write(0xFF);
    Serial1.write(0xFF);
    Serial1.write(0x00);
  }
  else return STM32ERR;
  while (!Serial1.available());
  return Serial1.read();
}

// No test yet
unsigned char stm32Run()   {
  stm32SendCommand(STM32RUN);
  while (!Serial1.available());
  if (Serial1.read() == STM32ACK) {
    stm32Address(STM32STADDR);
    return STM32ACK;
  }
  else
    return STM32ERR;
}

// No test yet
unsigned char stm32Read(unsigned char * rdbuf, unsigned long rdaddress, unsigned char rdlen) {
  stm32SendCommand(STM32RD);
  while (!Serial1.available());
  if (Serial1.read() == STM32ACK)
    stm32Address(rdaddress);
  else return STM32ERR;
  while (!Serial1.available());
  if (Serial1.read() == STM32ACK)
    stm32SendCommand(rdlen);
  while (!Serial1.available());
  size_t getlen = Serial1.available();
  Serial1.readBytes(rdbuf, getlen);
  return STM32ACK;
}

unsigned char stm32Address(unsigned long addr) {    // Tested
  unsigned char sendaddr[4];
  unsigned char addcheck = 0;
  sendaddr[0] = addr >> 24;
  sendaddr[1] = (addr >> 16) & 0xFF;
  sendaddr[2] = (addr >> 8) & 0xFF;
  sendaddr[3] = addr & 0xFF;
  for (int i = 0; i <= 3; i++) {
    Serial1.write(sendaddr[i]);
    addcheck ^= sendaddr[i];
  }
  Serial1.write(addcheck);
  while (!Serial1.available());
  return Serial1.read();
}

unsigned char stm32SendData(unsigned char * data, unsigned char wrlen) {     // Tested
  Serial1.write(wrlen);
  for (int i = 0; i <= wrlen; i++) {
    Serial1.write(data[i]);
  }
  Serial1.write(getChecksum(data, wrlen));
  while (!Serial1.available());
  return Serial1.read();
}

char stm32Version() {     // Tested
  unsigned char vsbuf[14];
  stm32SendCommand(STM32GET);
  while (!Serial1.available());
  vsbuf[0] = Serial1.read();
  if (vsbuf[0] != STM32ACK)
    return STM32ERR;
  else {
    Serial1.readBytesUntil(STM32ACK, vsbuf, 14);
    return vsbuf[1];
  }
}

unsigned char stm32GetId() {     // Tested
  int getid = 0;
  unsigned char sbuf[5];
  stm32SendCommand(STM32ID);
  while (!Serial1.available());
  sbuf[0] = Serial1.read();
  if (sbuf[0] == STM32ACK) {
    Serial1.readBytesUntil(STM32ACK, sbuf, 4);
    getid = sbuf[1];
    getid = (getid << 8) + sbuf[2];
    if (getid == 0x440)
      return 1;
    if (getid == 0x444)
      return 2;
    if (getid == 0x442)
      return 3;
    if (getid == 0x445)
      return 4;
    if (getid == 0x448)
      return 5;
    if (getid == 0x442)
      return 6;
    if (getid == 0x412)
      return 7;
    if (getid == 0x410)
      return 8;
    if (getid == 0x414)
      return 9;
    if (getid == 0x420)
      return 10;
    if (getid == 0x428)
      return 11;
    if (getid == 0x418)
      return 12;
    if (getid == 0x430)
      return 13;
    if (getid == 0x411)
      return 14;
    if (getid == 0x432)
      return 15;
    if (getid == 0x422)
      return 16;
    if (getid == 0x439)
      return 17;
    if (getid == 0x438)
      return 18;
    if (getid == 0x446)
      return 19;
    if (getid == 0x413)
      return 20;
    if (getid == 0x419)
      return 21;
    if (getid == 0x423)
      return 22;
    if (getid == 0x433)
      return 23;
    if (getid == 0x458)
      return 24;
    if (getid == 0x431)
      return 25;
    if (getid == 0x441)
      return 26;
    if (getid == 0x421)
      return 27;
    if (getid == 0x434)
      return 28;
    if (getid == 0x463)
      return 29;
    if (getid == 0x452)
      return 30;
    if (getid == 0x449)
      return 31;
    if (getid == 0x451)
      return 32;
    if (getid == 0x450)
      return 33;
    if (getid == 0x457)
      return 34;
    if (getid == 0x425)
      return 35;
    if (getid == 0x417)
      return 36;
    if (getid == 0x447)
      return 37;
    if (getid == 0x416)
      return 38;
    if (getid == 0x429)
      return 39;
    if (getid == 0x427)
      return 40;
    if (getid == 0x436)
      return 41;
    if (getid == 0x437)
      return 42;
    if (getid == 0x435)
      return 43;
    if (getid == 0x462)
      return 44;
    if (getid == 0x415)
      return 45;
    if (getid == 0x461)
      return 46;
  }
  else
    return 0;
}

unsigned char getChecksum( unsigned char * data, unsigned char datalen) {    // Tested
  unsigned char lendata = datalen;
  for (int i = 0; i <= datalen; i++)
    lendata ^= data[i];
  return lendata;
}

#else

uint8_t DATA_BUF[ETX_OTA_PACKET_MAX_SIZE];
uint8_t APP_BIN[ETX_OTA_DATA_MAX_SIZE];

static const uint32_t crc_table[0x100] = {
  0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 
  0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD, 
  0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 
  0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 
  0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 0x0808D07D, 0x0CC9CDCA, 
  0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 
  0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 
  0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A, 
  0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
  0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 
  0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3, 
  0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
  0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 
  0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654, 
  0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
  0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4, 
};

uint32_t CalcCRC(uint8_t * pData, uint32_t DataLength)
{
    uint32_t Checksum = 0xFFFFFFFF;
    for(unsigned int i=0; i < DataLength; i++)
    {
        uint8_t top = (uint8_t)(Checksum >> 24);
        top ^= pData[i];
        Checksum = (Checksum << 8) ^ crc_table[top];
    }
    return Checksum;
}

uint32_t CalcCRCFlash(uint8_t * pData, uint32_t DataLength, uint32_t seed)
{
    uint32_t Checksum = seed;
    for(unsigned int i=0; i < DataLength; i++)
    {
        uint8_t top = (uint8_t)(Checksum >> 24);
        top ^= pData[i];
        Checksum = (Checksum << 8) ^ crc_table[top];
    }
    return Checksum;
}

void sendBytes(uint8_t data) {

  uint8_t err = 0;
  //while (!Serial1.available());
  Serial1.write(data);
}

int poolResponse(unsigned char *buf, int size)
{
  int n;
  while (!Serial1.available());

   n =Serial1.readBytes(buf, size);
  if(n < 0){
    return 0;
  }
  return n;
}

/* read the response */
bool is_ack_resp_received( )
{
  bool is_ack  = false;

  memset(DATA_BUF, 0, ETX_OTA_PACKET_MAX_SIZE);

  uint16_t len =  poolResponse( DATA_BUF, sizeof(ETX_OTA_RESP_));

  if( len > 0 )
  {
    ETX_OTA_RESP_ *resp = (ETX_OTA_RESP_*) DATA_BUF;
    if( resp->packet_type == ETX_OTA_PACKET_TYPE_RESPONSE )
    {
      if( resp->crc == CalcCRC(&resp->status, 1) )
      {
        if( resp->status == ETX_OTA_ACK )
        {
          //ACK received
          is_ack = true;
        }
      }
    }
  }

  return is_ack;
}

/* Build the OTA START command */
int send_ota_start()
{
  uint16_t len;
  ETX_OTA_COMMAND_ *ota_start = (ETX_OTA_COMMAND_*)DATA_BUF;
  int ex = 0;

  memset(DATA_BUF, 0, ETX_OTA_PACKET_MAX_SIZE);

  ota_start->sof          = ETX_OTA_SOF;
  ota_start->packet_type  = ETX_OTA_PACKET_TYPE_CMD;
  ota_start->data_len     = 1;
  ota_start->cmd          = ETX_OTA_CMD_START;
  ota_start->crc          = CalcCRC( &ota_start->cmd, 1);
  ota_start->eof          = ETX_OTA_EOF;

  len = sizeof(ETX_OTA_COMMAND_);

  //send OTA START
  for(int i = 0; i < len; i++)
  {
    delay(1);
    sendBytes(DATA_BUF[i]);
  }

  if( ex >= 0 )
  {
    if( !is_ack_resp_received( ) )
    {
      //Received NACK
      #ifdef ENABLE_DEBUG_UART 
      Serial.print("OTA START : NACK\n");
      #endif
      ex = -1;
    }
  }
  #ifdef ENABLE_DEBUG_UART 
  Serial.printf("OTA START [ex = %d]\r\n", ex);
  #endif
  return ex;
}

/* Build and Send the OTA END command */
uint16_t send_ota_end()
{
  uint16_t len;
  ETX_OTA_COMMAND_ *ota_end = (ETX_OTA_COMMAND_*)DATA_BUF;
  int ex = 0;

  memset(DATA_BUF, 0, ETX_OTA_PACKET_MAX_SIZE);

  ota_end->sof          = ETX_OTA_SOF;
  ota_end->packet_type  = ETX_OTA_PACKET_TYPE_CMD;
  ota_end->data_len     = 1;
  ota_end->cmd          = ETX_OTA_CMD_END;
  ota_end->crc          = CalcCRC( &ota_end->cmd, 1);
  ota_end->eof          = ETX_OTA_EOF;

  len = sizeof(ETX_OTA_COMMAND_);

  //send OTA END
  for(int i = 0; i < len; i++)
  {
    delay(1);
    sendBytes(DATA_BUF[i]);
  }

  if( ex >= 0 )
  {
    if( !is_ack_resp_received() )
    {
      #ifdef ENABLE_DEBUG_UART 
      //Received NACK
      Serial.print("OTA END : NACK\n");
      #endif
      ex = -1;
    }
  }
  #ifdef ENABLE_DEBUG_UART 
  Serial.printf("OTA END [ex = %d]\n", ex);
  #endif

  return ex;
}

/* Build and send the OTA Header */
int send_ota_header( meta_info *ota_info)
{
  uint16_t len;
  ETX_OTA_HEADER_ *ota_header = (ETX_OTA_HEADER_*)DATA_BUF;
  int ex = 0;

  memset(DATA_BUF, 0, ETX_OTA_PACKET_MAX_SIZE);

  ota_header->sof          = ETX_OTA_SOF;
  ota_header->packet_type  = ETX_OTA_PACKET_TYPE_HEADER;
  ota_header->data_len     = sizeof(meta_info);
  ota_header->crc          = CalcCRC( (uint8_t*)ota_info, sizeof(meta_info));
  ota_header->eof          = ETX_OTA_EOF;

  memcpy(&ota_header->meta_data, ota_info, sizeof(meta_info) );

  len = sizeof(ETX_OTA_HEADER_);

  //send OTA Header
  for(int i = 0; i < len; i++)
  {
    delay(1);
    sendBytes(DATA_BUF[i]);
  }

  if( ex >= 0 )
  {
    if( !is_ack_resp_received( ) )
    {
      #ifdef ENABLE_DEBUG_UART 
      //Received NACK
      Serial.print("OTA HEADER : NACK\r\n");
      #endif
      ex = -1;
    }
  }
  #ifdef ENABLE_DEBUG_UART 
  Serial.printf("OTA HEADER [ex = %d]\r\n", ex);
  #endif
  return ex;
}

/* Build and send the OTA Data */
int send_ota_data( uint8_t *data, uint16_t data_len)
{
  uint16_t len;
  ETX_OTA_DATA_ *ota_data = (ETX_OTA_DATA_*)DATA_BUF;
  int ex = 0;

  memset(DATA_BUF, 0, ETX_OTA_PACKET_MAX_SIZE);

  ota_data->sof          = ETX_OTA_SOF;
  ota_data->packet_type  = ETX_OTA_PACKET_TYPE_DATA;
  ota_data->data_len     = data_len;

  len = 4;

  //Copy the data
  memcpy(&DATA_BUF[len], data, data_len );
  len += data_len;

  //Calculate and Copy the crc
  uint32_t crc = CalcCRC( data, data_len);
  memcpy(&DATA_BUF[len], (uint8_t*)&crc, sizeof(crc) );
  len += sizeof(crc);

  //Add the EOF
  DATA_BUF[len] = ETX_OTA_EOF;
  len++;
  #ifdef ENABLE_DEBUG_UART 
  Serial.printf("Sending %d Data\r\n", len);
  #endif

  //send OTA Data
  for(int i = 0; i < len; i++)
  {
    delay(1);
    sendBytes(DATA_BUF[i]);
  }

  if( ex >= 0 )
  {
    if( !is_ack_resp_received() )
    {
      #ifdef ENABLE_DEBUG_UART 
      //Received NACK
      Serial.print("OTA DATA : NACK\n");
      #endif
      ex = -1;
    }
  }
  //printf("OTA DATA [ex = %d]\n", ex);
  return ex;
}

#endif