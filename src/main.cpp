
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <otadrive_esp.h>
#include <WiFiManager.h>
#include "stm32_programmer.h"
#include <Preferences.h>

Preferences wifi_storage;

#define APIKEY "30efe461-4722-4439-98ed-96a35e312fb2"
#define FILESYS SPIFFS
#define ESP32_CORE 0
#define STM32_CORE 1
#define WM_DEBUG_LEVEL DEBUG_MAX
#define ENABLE_DEBUG_UART 1
/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */
#define FORMAT_SPIFFS_IF_FAILED true

extern uint8_t DATA_BUF[ETX_OTA_PACKET_MAX_SIZE];
extern uint8_t APP_BIN[ETX_OTA_DATA_MAX_SIZE];

File fsUploadFile;
// File fsMD5HashFile;
uint8_t binread[256];
uint8_t download_core = ESP32_CORE;
int bini = 0;
bool checkFwFlag = false;
bool dfuMode = false;
bool STM32_DFU_PENDING = false;
bool secIncFlag = true;
bool remote_button_press = true;
uint8_t wait_sec = 0;
uint8_t secInc = 0;
// WiFiManager, Local intialization.
// Once its business is done, there is no need to keep it around
WiFiManager wm;
String str;
int timeout = 60 * 3; // seconds to run for(seconds)
String ver = "1.2.6";
uint32_t updateCounter = 0;
uint8_t retries = 3;

void update();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

int flashStm32Device(void);
bool esp32EraseWifInfo(void);
static void saveConfigCallback(void);
String stm32CommandPackageJson(String wifi_name);

bool doesFileExists(fs::FS &fs, const char *path)
{

    if (fs.exists(path))
    {
#ifdef ENABLE_DEBUG_UART
        Serial.printf("File Exists\r\n");
#endif
        return true;
    }
    else
    {
#ifdef ENABLE_DEBUG_UART
        Serial.printf("File does not Exists\r\n");
#endif
    }

    return false;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{

#ifdef ENABLE_DEBUG_UART
    Serial.printf("Writing file: %s\r\n", path);
    Serial.printf("File content: %s\r\n", message);
#endif

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("- failed to open file for writing");
#endif

        return;
    }

    if (file.print(message))
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("- file written");
#endif
    }
    else
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("- write failed");
#endif
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char *path)
{
#ifdef ENABLE_DEBUG_UART
    Serial.printf("Deleting file: %s\n\r", path);
#endif

    if (fs.remove(path))
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("File deleted\n\r");
#endif
    }
    else
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("Delete failed\n\r");
#endif
    }
}

void deleteExistingFile()
{
    deleteFile(SPIFFS, "/stm32/stmfw.bin");
}

std::vector<String> WifiScanName;

typedef struct
{
    String name;
    String password;
} user_wifi_info_t;

std::vector<user_wifi_info_t> WifiStorageFlash;
uint8_t WifiCountExisting = 0;
uint8_t WifiCountRead = 0;

#define WIFI_INFO_COUNT "WifiCount"

#define WIFI_INFO_SSID "SSID"
#define WIFI_INFO_PASS "Pass"

#define MAX_WIFI_INFO_SUPPORTED 5
void user_wifi_read_info(void)
{
    user_wifi_info_t info;
    String nvs_ssid_prefix = WIFI_INFO_SSID;
    String nvs_pass_prefix = WIFI_INFO_PASS;

    wifi_storage.begin("WifiInfo", false);

    WifiCountExisting = wifi_storage.getUInt(WIFI_INFO_COUNT, 0);
    if (WifiCountExisting == 0)
        return;
    for (size_t i = 0; i < WifiCountExisting; i++)
    {
        nvs_ssid_prefix = nvs_ssid_prefix + i;
        nvs_pass_prefix = nvs_pass_prefix + i;

        Serial.println("nvs_ssid_prefix: " + nvs_ssid_prefix);
        Serial.println("nvs_pass_prefix: " + nvs_pass_prefix);

        info.name = wifi_storage.getString(nvs_ssid_prefix.c_str(), "");
        info.password = wifi_storage.getString(nvs_pass_prefix.c_str(), "");
        if (info.name != "" && info.password != "")
        {
            WifiStorageFlash.push_back(info);
            Serial.println("name = " + info.name);
            Serial.println("password = " + info.password);
        }
        // reset prefix
        nvs_ssid_prefix = WIFI_INFO_SSID;
        nvs_pass_prefix = WIFI_INFO_PASS;
    }

    wifi_storage.end();
}

void user_wifi_info_save(String name, String password)
{
    wifi_storage.begin("WifiInfo", false);
    String nvs_ssid_prefix = WIFI_INFO_SSID;
    String nvs_pass_prefix = WIFI_INFO_PASS;

    if (WifiCountExisting < MAX_WIFI_INFO_SUPPORTED)
    {
        nvs_ssid_prefix = nvs_ssid_prefix + WifiCountExisting;
        nvs_pass_prefix = nvs_pass_prefix + WifiCountExisting;

        wifi_storage.putUInt(WIFI_INFO_COUNT, WifiCountExisting + 1);
        wifi_storage.putString(nvs_ssid_prefix.c_str(), name);
        wifi_storage.putString(nvs_pass_prefix.c_str(), password);
    }
    else
    {
        nvs_ssid_prefix = nvs_ssid_prefix + 0;
        nvs_pass_prefix = nvs_pass_prefix + 0;

        wifi_storage.putString(nvs_ssid_prefix.c_str(), name);
        wifi_storage.putString(nvs_pass_prefix.c_str(), password);
    }

    wifi_storage.end();
}

uint8_t WifiScanCount = 0;
void user_wifi_scan(void)
{
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            WifiScanCount++;
            WifiScanName.push_back(WiFi.SSID(i));
            Serial.println(WiFi.SSID(i));
            delay(10);
        }
    }
    Serial.println("");
}

user_wifi_info_t user_wifi_check_wifi_available(void)
{
    delay(100);
    uint8_t count = 0;
    user_wifi_info_t wifi_ifo;
    wifi_ifo.name = "";
    wifi_ifo.password = "";

    Serial.print("WifiScanCount  = ");
    Serial.println(WifiScanName.size());

    Serial.print("WifiStorageFlash  = ");
    Serial.println(WifiStorageFlash.size());

    for (size_t i = 0; i < WifiScanName.size(); i++)
    {
        for (size_t j = 0; j < WifiStorageFlash.size(); j++)
        {
            // Serial.println("compare between " + WifiStorageFlash.at(j).name + " and " + WifiScanName[i]);
            if (WifiStorageFlash.at(j).name == WifiScanName[i])
            {
                Serial.print(" wifi essist at ");
                Serial.println(j);
                wifi_ifo.name = WifiStorageFlash.at(j).name;
                wifi_ifo.password = WifiStorageFlash.at(j).password;
                return wifi_ifo;
            }
        }
    }
    return wifi_ifo;
}

void setup()
{
    bool res;
    gpio_pad_select_gpio(GPIO_NUM_1);
    gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT_OD);
    gpio_set_level(GPIO_NUM_1, 0);

    // explicitly set mode, esp defaults to STA+AP
    WiFi.mode(WIFI_STA);
// OTADRIVE.useMD5Matcher(false);
#ifdef ENABLE_DEBUG_UART
    Serial.begin(115200);
    Serial.setTimeout(10);
#endif
    Serial1.begin(115200, SERIAL_8E1);
    Serial1.setTimeout(10);

    // WiFi.begin("Astek2G", "<6L&R8+7XRFh)");
    // WiFi.begin("FlashJackson2022", "Adam2326!");
#ifdef ENABLE_DEBUG_UART
    Serial.println("");
#endif
    user_wifi_read_info();
    user_wifi_scan();

    user_wifi_info_t info = user_wifi_check_wifi_available();

    wm.setBreakAfterConfig(true);
    wm.setSaveConfigCallback(saveConfigCallback);

    WiFi.mode(WIFI_STA);
    WiFi.begin(info.name, info.password);

    uint8_t timeout_wifi_available = 0;
    while ((WiFi.status() != WL_CONNECTED) && (timeout_wifi_available < 10))
    {
        Serial.print('.');
        delay(1000);
        timeout_wifi_available++;
    }

    if (timeout_wifi_available != 0)
        res = wm.autoConnect("TigerSharkAP", "password"); // password protected ap

    if (!res)
    {
        ESP.restart();
    }
#ifdef ENABLE_DEBUG_UART
    Serial.println("\n Starting");
    Serial.println("Version " + ver);
#endif
    Serial1.println("Version " + ver);

    if (WiFi.status() == WL_CONNECTED)
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println(WiFi.localIP());
#endif

        Serial1.println(WiFi.localIP());
    }

    // initialize FileSystem
    if (!SPIFFS.begin(true))
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("SPIFFS Mount Failed");
#endif
        return;
    }

#ifdef ENABLE_DEBUG_UART
    Serial.println("File system Mounted");
#endif

#if 0
  deleteFile(SPIFFS, "/stm32/stmfw.bin");
  deleteFile(SPIFFS, "/stmfw.txt");
#else
    if (!doesFileExists(SPIFFS, "/stmfw.txt"))
    {
        writeFile(SPIFFS, "/stmfw.txt", "DFU_CURRENT");
    }
#endif
    listDir(SPIFFS, "/", 0);

#if 0
 //for debugging purpose only
      // CRC TEST

        meta_info ota_info;
        uint32_t chcksum =  0xFFFFFFFF;
        uint16_t size = 0;

        fsUploadFile = SPIFFS.open("/stm32/stmfw.bin", "r");
        uint32_t app_size = fsUploadFile.size();
        Serial.printf("File size = %d\r\n", app_size);

        for( uint32_t i = 0; i < app_size; ){

          fsUploadFile.read(APP_BIN, ETX_OTA_DATA_MAX_SIZE);

          if( ( app_size - i ) >= ETX_OTA_DATA_MAX_SIZE ){
            size = ETX_OTA_DATA_MAX_SIZE;
          }
          else{
            size = app_size - i;
          }

          i += size;
         
          chcksum = CalcCRCFlash(APP_BIN, size, chcksum);

        }
        
        fsUploadFile.close();
         
        Serial.printf("crc: 0x%04X \r\n",chcksum);

        // END CRC TEST
#endif

    OTADRIVE.setInfo(APIKEY, "v@" + ver);
    OTADRIVE.onUpdateFirmwareProgress(onUpdateProgress);
}

void loop()
{

    if (Serial1.available() > 0)
    {
        str = Serial1.readString();
    }

    if (WiFi.status() == WL_CONNECTED)
    {

        if (str == "CHECKFIRMWARE")
        {

            // Every 30 seconds
            // if(OTADRIVE.timeTick(30))
            //{

            // deleteFile(SPIFFS, "/stm32/stmfw.bin");
            // delay(3000);

            Serial1.print("progress_dwnld_core1\n\r");
            download_core = STM32_CORE;
#ifdef ENABLE_DEBUG_UART
            // sync local files with OTAdrive server
            Serial.println("\n\nchecking for new Resources\r\n");
#endif

            do
            {
#ifdef ENABLE_DEBUG_UART
                // stm32 download fw attempts
                Serial.printf("\n\nstm dwn attempt %d\r\n", 4 - retries);
#endif
                if (OTADRIVE.syncResources())
                {
                    break;
                }
            } while (retries--);

            // list local files to serial port
            listDir(OTA_FILE_SYS, "/", 0);

// get configuration of device
// String c = OTADRIVE.getConfigs();
#ifdef ENABLE_DEBUG_UART
// Serial.printf("\nconfiguration: %s\n\r", c.c_str());
#endif

            Serial1.print("progress_install_core2\n\r");
            download_core = ESP32_CORE;

#ifdef ENABLE_DEBUG_UART
            // retrive firmware info from OTAdrive server
            Serial.println("\n\nchecking for new firmware\r\n");
#endif
            updateInfo inf = OTADRIVE.updateFirmwareInfo();

#ifdef ENABLE_DEBUG_UART
            Serial.printf("\nfirmware info: %s\r\n%d Bytes\r\n%s\r\n",
                          inf.version.c_str(), inf.size, inf.available ? "New version available" : "No newer version");
#endif

            // update firmware if newer available
            if (inf.available)
            {
                // keep power on
                gpio_set_level(GPIO_NUM_1, 0);
                delay(1000);
                OTADRIVE.updateFirmware();
            }
            else
            {
                Serial1.print("set_progress_id\n\r");
                if (STM32_DFU_PENDING)
                {
                    // keep power on
                    gpio_set_level(GPIO_NUM_1, 0);
                    delay(1000);
                    Serial1.print("DFU\n\r");

#ifdef ENABLE_DEBUG_UART
                    // retrive firmware info from OTAdrive server
                    Serial.println("\n\nNo New ESP32 Fw\r\n");
#endif
                }
            }

            // }
        }
    }

    if (str == "WIFIMANAGER")
    {
        bool res;

        // reset settings - wipe stored credentials for testing
        // these are stored by the esp library
        wm.resetSettings();

        // set configportal timeout
        wm.setConfigPortalTimeout(timeout);

        res = wm.autoConnect("TigerSharkAP", "password"); // password protected ap

        if (!res)
        {
#ifdef ENABLE_DEBUG_UART
            Serial.println("Failed to connect");
#endif
            // ESP.restart();
        }

        String ipv4;
#ifdef ENABLE_DEBUG_UART
        // ipv4 = "</IP>";
        // ipv4 += WiFi.localIP();
        // ipv4 += "<IP>";
        Serial1.println(WiFi.localIP());
#endif
    }
    else if (str == "DFU_MODE")
    {

#ifdef ENABLE_DEBUG_UART
        Serial.println("STARTING-HOSTSIDE-DFU");
#endif

#ifdef STM_FACTORY_BOOTLOADER
        Serial1.print("DFU\n\r");
#endif

        delay(1000);
        flashStm32Device();
    }
    else if (str == "KEEP_ALIVE_ON")
    {
        gpio_set_level(GPIO_NUM_1, 0);
    }
    else if (str == "KEEP_ALIVE_OFF")
    {
        gpio_set_level(GPIO_NUM_1, 1);
    }
    else if (str == "CLR_STM_FW")
    {
        deleteExistingFile();
    }
    else
    {

        if (secInc > 20)
        {

#ifdef ENABLE_DEBUG_UART
            Serial.println("Reading stmfw.txt content...");
#endif
            File fHndlr = SPIFFS.open("/stmfw.txt", "r");
            if (fHndlr)
            {
                secInc = 0;
                secIncFlag = false;
                String msg = fHndlr.readString();
#ifdef ENABLE_DEBUG_UART
                Serial.println(msg);
#endif
                fHndlr.close();

                if (msg.equals("DFU_UPGRADE"))
                {
#ifdef ENABLE_DEBUG_UART
                    Serial.println("INIATING DFU SEQUENCE");
#endif
                    Serial1.print("DFU\n\r");
                }
                else
                {
#ifdef ENABLE_DEBUG_UART
                    Serial.println("FW UP TO DATE");
#endif
                }
            }
        }
        else
        {
            if (secIncFlag)
            {
                secInc++;
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
#ifdef ENABLE_DEBUG_UART
            Serial.print(".");
#endif
        }
    }
    str = "NOOP";
    delay(1000);
}

void onUpdateProgress(int progress, int totalt)
{
    static int last = 0;
    int progressPercent = (100 * progress) / totalt;

    if (last != progressPercent && progressPercent % 10 == 0)
    {
// print every 10%
#ifdef ENABLE_DEBUG_UART
        Serial.printf("%d\n\r", progressPercent);
#endif

        Serial1.printf("%d\n\r", progressPercent);
    }
    last = progressPercent;
    if ((download_core == STM32_CORE) && (last == 100))
    {
#ifdef ENABLE_DEBUG_UART
        Serial.printf("Settting stmfw to DFU_UPGRADE\n\r");
#endif
        writeFile(SPIFFS, "/stmfw.txt", "DFU_UPGRADE");
        STM32_DFU_PENDING = true;
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
#ifdef ENABLE_DEBUG_UART
    Serial.printf("Listing directory: %s\r\n", dirname);
#endif

    File root = fs.open(dirname, "r");
    if (!root)
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println("- failed to open directory");
#endif
        return;
    }
    if (!root.isDirectory())
    {
#ifdef ENABLE_DEBUG_UART
        Serial.println(" - not a directory");
#endif
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
#ifdef ENABLE_DEBUG_UART
            Serial.print("  DIR : ");
            Serial.println(file.name());
#endif

            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
#ifdef ENABLE_DEBUG_UART
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
#endif
        }
        file = root.openNextFile();
    }
}

#ifdef STM_FACTORY_BOOTLOADER

/* This works with the STM factory bootloader */
void flashStm32Device()
{
    uint8_t err = 0;
    String FileName, flashwr;
    int lastbuf = 0;
    uint8_t cflag, fnum = 0xFF;

#if 0
  Serial.println("  Opening /stm32/stmfw.bin ");
#else
    fsUploadFile = SPIFFS.open("/stm32/stmfw.bin", "r");
    Serial.println("  FILE: ");

    if (fsUploadFile)
    {

        Serial.println("  FILE: Opened");
        bini = fsUploadFile.size() / 256;
        lastbuf = fsUploadFile.size() % 256;
        flashwr = String(bini) + "-" + String(lastbuf) + "<br>";
        Serial.println(flashwr);

        /* SYNC Device */
        Serial1.write(0x7F);
        while (!Serial1.available())
            ;
        if (Serial1.read() != STM32ACK)
        {
            fsUploadFile.close();
            return;
        }

        /* Get Version */
        stm32Version();
        /* Erase Device */
        stm32Erase();
        stm32Erasen();

        for (int i = 0; i < bini; i++)
        {
            fsUploadFile.read(binread, 256);
            stm32SendCommand(STM32WR);

            while (!Serial1.available())
                ;
            cflag = Serial1.read();

            if (cflag == STM32ACK)
            {
                if (stm32Address(STM32STADDR + (256 * i)) == STM32ACK)
                {
                    if (stm32SendData(binread, 255) == STM32ACK)
                        Serial.print(".");
                    else
                    {
                        Serial.print("!");
                        err = 1;
                    }
                }
            }
        }

        fsUploadFile.read(binread, lastbuf);
        stm32SendCommand(STM32WR);
        while (!Serial1.available())
            ;
        cflag = Serial1.read();
        if (cflag == STM32ACK)
            if (stm32Address(STM32STADDR + (256 * bini)) == STM32ACK)
            {
                if (stm32SendData(binread, lastbuf) == STM32ACK)
                    Serial.print(".");
                else
                {
                    Serial.print("!");
                    err = 1;
                }
            }

        fsUploadFile.close();

        if (err)
            Serial.println("Error Occurred !!!");
        else
        {
            Serial.println("Successful ");
            stm32Run();
        }
    }
    else
    {
        Serial.println("Error Occurred, could not open file !!!");
    }

#endif
}
#else
int flashStm32Device()
{
    int ex = 0;

    fsUploadFile = SPIFFS.open("/stm32/stmfw.bin", "r");
    // #ifdef ENABLE_DEBUG_UART
    // Serial.println("  FILE: ");
    // #endif

    if (fsUploadFile)
    {

#ifdef ENABLE_DEBUG_UART
        Serial.println("  FILE: Opened");
#endif
        // bini = fsUploadFile.size() / 256;
        // lastbuf = fsUploadFile.size() % 256;
        // flashwr = String(bini) + "-" + String(lastbuf) + "<br>";
        // Serial.println(flashwr);

        do
        {

            // send OTA Start command
            ex = send_ota_start();

            if (ex < 0)
            {
#ifdef ENABLE_DEBUG_UART
                Serial.print("send_ota_start Err\r\n");
#endif

                break;
            }

            uint32_t app_size = fsUploadFile.size();
#ifdef ENABLE_DEBUG_UART
            Serial.printf("File size = %d\r\n", app_size);
#endif

            // Send OTA Header
            meta_info ota_info;
            uint32_t size = 0;
            uint32_t chcksum = 0xFFFFFFFF;
            ota_info.package_size = app_size;

            for (uint32_t i = 0; i < app_size;)
            {

                fsUploadFile.read(APP_BIN, ETX_OTA_DATA_MAX_SIZE);

                if ((app_size - i) >= ETX_OTA_DATA_MAX_SIZE)
                {
                    size = ETX_OTA_DATA_MAX_SIZE;
                }
                else
                {
                    size = app_size - i;
                }

                i += size;

                chcksum = CalcCRCFlash(APP_BIN, size, chcksum);
            }

            /* reset position to beginning of file*/
            fsUploadFile.seek(0, SeekSet);

            // fsUploadFile.close();
            ota_info.package_crc = chcksum;

            ex = send_ota_header(&ota_info);
            if (ex < 0)
            {
#ifdef ENABLE_DEBUG_UART
                Serial.print("send_ota_header Err\r\n");
#endif
                break;
            }

            size = 0;

            for (uint32_t i = 0; i < app_size;)
            {
#ifdef ENABLE_DEBUG_UART
                Serial.print(".");
#endif
                fsUploadFile.read(APP_BIN, ETX_OTA_DATA_MAX_SIZE);

                if ((app_size - i) >= ETX_OTA_DATA_MAX_SIZE)
                {
                    size = ETX_OTA_DATA_MAX_SIZE;
                }
                else
                {
                    size = app_size - i;
                }

#ifdef ENABLE_DEBUG_UART
                Serial.printf("[%d/%d]\r\n", i / ETX_OTA_DATA_MAX_SIZE, app_size / ETX_OTA_DATA_MAX_SIZE);
#endif

                ex = send_ota_data(&APP_BIN[0], size);
                if (ex < 0)
                {
#ifdef ENABLE_DEBUG_UART
                    printf("send_ota_data Err [i=%d]\n", i);
#endif
                    break;
                }

                i += size;
            }

            if (ex < 0)
            {
                break;
            }

            // send OTA END command
            ex = send_ota_end();
            if (ex < 0)
            {
#ifdef ENABLE_DEBUG_UART
                Serial.print("send_ota_end Err\n");
#endif
                break;
            }

        } while (false);

        fsUploadFile.close();

        if (ex < 0)
        {
#ifdef ENABLE_DEBUG_UART
            Serial.print("OTA ERROR\n");
#endif
        }
        else
        {
            STM32_DFU_PENDING = false;
            writeFile(SPIFFS, "/stmfw.txt", "DFU_CURRENT");
        }

        return (ex);
    }
    else
    {
        ex = -1;
#ifdef ENABLE_DEBUG_UART
        Serial.print("FAILED TO OPEN FILE\n");
#endif
    }

    return (ex);
}
#endif

/**
 * @brief wifi erase WiFi Config
 *
 * @return true
 * @return false
 */
bool esp32EraseWifInfo(void)
{
    wifi_storage.putUInt(WIFI_INFO_COUNT, 0);
    return wm.erase();
}

/**
 * @brief wifi save wifi config callback function
 *
 */
static void saveConfigCallback(void)
{
    Serial.println("Should save config");

    Serial.println("getWiFiPass: ");
    Serial.println(wm.getWiFiPass());
    Serial.println("getWiFiSSID: ");
    Serial.println(wm.getWiFiSSID());

    user_wifi_info_save(wm.getWiFiSSID(), wm.getWiFiPass());
}

// Returns true if input points to a valid JSON string
static bool validateJson(const char *input)
{
    StaticJsonDocument<0> doc, filter;
    return deserializeJson(doc, input, DeserializationOption::Filter(filter)) == DeserializationError::Ok;
}

static void stm32CommandParserJson(String message)
{
    // {"command":"wifi","New item":["tuan123","123tuan"]}
    if (validateJson(message.c_str()) == false)
        return;
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, message);

}

String stm32CommandPackageJson(String wifi_name)
{
    String message;
    DynamicJsonDocument doc(512);
    doc["command"] = "wifi";
    doc["wifi_name"] = wifi_name;
    return message;
}