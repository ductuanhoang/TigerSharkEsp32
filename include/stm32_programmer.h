#ifndef _STM32_PROGRAMMER_H_
#define _PROGRAMMER_H_
#endif
#include <Arduino.h>

//#define STM_FACTORY_BOOTLOADER
//#define ENABLE_DEBUG_UART 

#ifdef STM_FACTORY_BOOTLOADER 

/* This works with STM factory bootloader*/
#define STERR     		"ERROR"
#define STM32INIT  		0x7F          // Send Init command
#define STM32ACK  		0x79          // return ACK answer
#define STM32NACK  		0x1F          // return NACK answer
#define STM32GET  		0             // get version command
#define STM32GVR      0x01          // get read protection status           never used in here
#define STM32ID       0x02          // get chip ID command
#define STM32RUN  		0x21          // Restart and Run programm
#define STM32RD  		  0x11          // Read flash command                   never used in here
#define STM32WR  		  0x31          // Write flash command
#define STM32ERASE    0x43          // Erase flash command
#define STM32ERASEN   0x44          // Erase extended flash command
#define STM32WP       0x63          // Write protect command                never used in here
#define STM32WP_NS    0x64          // Write protect no-stretch command     never used in here
#define STM32UNPCTWR  0x73          // Unprotect WR command                 never used in here
#define STM32UW_NS    0x74          // Unprotect write no-stretch command   never used in here
#define STM32RP       0x82          // Read protect command                 never used in here
#define STM32RP_NS    0x83          // Read protect no-stretch command      never used in here
#define STM32UR       0x92          // Unprotect read command               never used in here
#define STM32UR_NS    0x93          // Unprotect read no-stretch command    never used in here

//#define ENABLE_DEBUG_UART
#define STM32STADDR  	0x8000000     // STM32 codes start address, you can change to other address if use custom bootloader like 0x8002000
#define STM32ERR  		0

//String STM32_CHIPNAME[8] = { 
//  "Unknown Chip",
//  "STM32F03xx4/6",
//  "STM32F030x8/05x",
//  "STM32F030xC",
//  "STM32F103x4/6",
//  "STM32F103x8/B", 
//  "STM32F103xC/D/E",
//  "STM32F105/107"  
//};

void stm32SendCommand(unsigned char commd);
unsigned char stm32Erase();
unsigned char stm32Erasen();
unsigned char stm32Read(unsigned char * rdbuf, unsigned long rdaddress, unsigned char rdlen);
unsigned char stm32Address(unsigned long addr);
unsigned char stm32SendData(unsigned char * data, unsigned char wrlen);
unsigned char getChecksum( unsigned char * data, unsigned char datalen);
unsigned char stm32GetId();
unsigned char stm32Run();
char stm32Version();
#else

/*
 * etx_ota_update.h
 *
 *  Created on: 26-Jul-2021
 *      Author: EmbeTronicX
 */

#include <stdbool.h>
//#include "main.h"

#define ETX_OTA_SOF  0xAA    // Start of Frame
#define ETX_OTA_EOF  0xBB    // End of Frame
#define ETX_OTA_ACK  0x00    // ACK
#define ETX_OTA_NACK 0x01    // NACK

#define ETX_APP_FLASH_ADDR        0x08040000   //Application's Flash Address
#define ETX_APP_SLOT0_FLASH_ADDR  0x080C0000   //App slot 0 address
#define ETX_APP_SLOT1_FLASH_ADDR  0x08140000   //App slot 1 address
#define ETX_CONFIG_FLASH_ADDR     0x08020000   //Configuration's address

#define ETX_NO_OF_SLOTS           2            //Number of slots
#define ETX_SLOT_MAX_SIZE        (512 * 1024)  //Each slot size (512KB)

#define ETX_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define ETX_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define ETX_OTA_PACKET_MAX_SIZE ( ETX_OTA_DATA_MAX_SIZE + ETX_OTA_DATA_OVERHEAD )

#define ETX_SD_CARD_FW_PATH "ETX_FW/app.bin"    //Firmware name present in SD card

/*
 * Reboot reason
 */
#define ETX_FIRST_TIME_BOOT       ( 0xFFFFFFFF )      //First time boot
#define ETX_NORMAL_BOOT           ( 0xBEEFFEED )      //Normal Boot
#define ETX_OTA_REQUEST           ( 0xDEADBEEF )      //OTA request by application
#define ETX_LOAD_PREV_APP         ( 0xFACEFADE )      //App requests to load the previous version

/*
 * Exception codes
 */
typedef enum
{
  ETX_OTA_EX_OK       = 0,    // Success
  ETX_OTA_EX_ERR      = 1,    // Failure
}ETX_OTA_EX_;

/*
 * Exception codes for SD Card
 */
typedef enum
{
  ETX_SD_EX_OK       = 0,    // Updated firmware successfully
  ETX_SD_EX_NO_SD    = 1,    // No SD card Found
  ETX_SD_EX_FU_ERR   = 2,    // Failure during Firmware update
  ETX_SD_EX_ERR      = 3,    // Other Failure
}ETX_SD_EX_;

/*
 * OTA process state
 */
typedef enum
{
  ETX_OTA_STATE_IDLE    = 0,
  ETX_OTA_STATE_START   = 1,
  ETX_OTA_STATE_HEADER  = 2,
  ETX_OTA_STATE_DATA    = 3,
  ETX_OTA_STATE_END     = 4,
}ETX_OTA_STATE_;

/*
 * Packet type
 */
typedef enum
{
  ETX_OTA_PACKET_TYPE_CMD       = 0,    // Command
  ETX_OTA_PACKET_TYPE_DATA      = 1,    // Data
  ETX_OTA_PACKET_TYPE_HEADER    = 2,    // Header
  ETX_OTA_PACKET_TYPE_RESPONSE  = 3,    // Response
}ETX_OTA_PACKET_TYPE_;

/*
 * OTA Commands
 */
typedef enum
{
  ETX_OTA_CMD_START = 0,    // OTA Start command
  ETX_OTA_CMD_END   = 1,    // OTA End command
  ETX_OTA_CMD_ABORT = 2,    // OTA Abort command
}ETX_OTA_CMD_;

/*
 * Slot table
 */
typedef struct
{
    uint8_t  is_this_slot_not_valid;  //Is this slot has a valid firmware/application?
    uint8_t  is_this_slot_active;     //Is this slot's firmware is currently running?
    uint8_t  should_we_run_this_fw;   //Do we have to run this slot's firmware?
    uint32_t fw_size;                 //Slot's firmware/application size
    uint32_t fw_crc;                  //Slot's firmware/application CRC
    uint32_t reserved1;
    uint32_t reserved2;
    uint32_t reserved3;
}__attribute__((packed)) ETX_SLOT_;

/*
 * General configuration
 */
typedef struct
{
    uint32_t  reboot_cause;
    ETX_SLOT_ slot_table[ETX_NO_OF_SLOTS];
}__attribute__((packed)) ETX_GNRL_CFG_;

/*
 * OTA meta info
 */
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) meta_info;

/*
 * OTA Command format
 *
 * ________________________________________
 * |     | Packet |     |     |     |     |
 * | SOF | Type   | Len | CMD | CRC | EOF |
 * |_____|________|_____|_____|_____|_____|
 *   1B      1B     2B    1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   cmd;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_COMMAND_;

/*
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  meta_info   meta_data;
  uint32_t    crc;
  uint8_t     eof;
}__attribute__((packed)) ETX_OTA_HEADER_;

/*
 * OTA Data format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint8_t     *data;
}__attribute__((packed)) ETX_OTA_DATA_;

/*
 * OTA Response format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len | Status | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   status;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_RESP_;

ETX_OTA_EX_ etx_ota_download_and_flash( void );
void load_new_app( void );
ETX_SD_EX_ check_update_frimware_SD_card( void );

void sendBytes(uint8_t data);
int poolResponse(unsigned char *buf, int size);
int send_ota_start(void);
uint16_t send_ota_end(void);
int send_ota_header(meta_info *ota_info);
int send_ota_data(uint8_t *data, uint16_t data_len);
void onUpdateProgress(int progress, int totalt);
void deleteExistingFile(void);
uint32_t CalcCRC(uint8_t * pData, uint32_t DataLength);
uint32_t CalcCRCFlash(uint8_t * pData, uint32_t DataLength, uint32_t seed);
#endif