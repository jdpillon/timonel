/*
  main.cpp
  ========
  Timonel library test program
  ----------------------------
  This demo shows how to control and update several microscontrollers (Tiny85)
  running the Timonel bootloader from an ESP8266 Master.
  It uses a serial console configured at 9600 N 8 1 for feedback.
  ---------------------------
  2019-03-11 Gustavo Casanova
  ---------------------------
*/

/*
 Working routine:
 ----------------
   1) Scans the TWI bus in search of all devices running Timonel.
   2) Creates an array of Timonel objects, one per device.
   3) List firmware version of each device.
   4) Use payload-A to modify the Timonel running on each device to FC 168.
   5) Starts the main loop, which allows to use serial commands.
   6) If the reset command is entered, steps 1 to 3 are repeated but this time:
   7) Use payload-B to modify the Timonel running on each device to FC 162.
   8) Repeat the routine.
*/

#include <Memory>
#include "payload.h"
#include "NbMicro.h"
#include "TimonelTwiM.h"
#include <Arduino.h>

#define USE_SERIAL Serial
//#define TML_ADDR 8  /* Bootloader I2C address*/
//#define APP_ADDR 36 /* Application I2C address*/
#define SDA 0       /* I2C SDA pin */
#define SCL 2       /* I2C SCL pin */
#define MAX_TWI_DEVS 28

#ifndef T_SIGNATURE
#define T_SIGNATURE 84
#endif

// Prototypes
void setup(void);
void loop(void);
bool CheckApplUpdate(void);
void ListTwiDevices(byte sda = 0, byte scl = 0);
byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size, byte sda = 0, byte scl = 0);
void PrintStatus(Timonel timonel);
void ThreeStarDelay(void);
void ReadChar(void);
word ReadWord(void);
void ShowHeader(void);
void ShowMenu(void);
void ClrScr(void);

// Global Variables
byte slave_address = 0;
byte block_rx_size = 0;
bool new_key = false;
bool new_byte = false;
bool new_word = false;
bool app_mode = false;
char key = '\0';
word flash_page_addr = 0x0;
word timonel_start = 0xFFFF; /* Timonel start address, 0xFFFF means 'not set' */


// Setup block
void setup() {
    USE_SERIAL.begin(9600); /* Initialize the serial port for debugging */
    Wire.begin(SDA, SCL);
    ClrScr();
    ShowHeader();
    // 1)
    //------ListTwiDevices();
    // 2)
    //Timonel tml_pool[MAX_TWI_DEVS] = { 0 };
    //GetAllTimonels(tml_pool, MAX_TWI_DEVS);
    Timonel t1(19);
    delay(150);
    // Timonel t2(22);
    // delay(150);    

    PrintStatus(t1);
    USE_SERIAL.printf_P("\n\rNext: t1 DeleteApplication ...\n\r");
    t1.DeleteApplication();
    delay(3000);
    Wire.begin(SDA, SCL);
    USE_SERIAL.printf_P("\n\rNext: t1 RunApplication ...\n\r");
    t1.RunApplication();
    delay(500);
    t1.UploadApplication(payload, (int)sizeof(payload), 0x0);
    PrintStatus(t1);

    // PrintStatus(t2);
    // USE_SERIAL.printf_P("\n\rNext: t2 DeleteApplication ...\n\r");
    // t2.DeleteApplication();
    // delay(3000);
    // Wire.begin(SDA, SCL);
    // USE_SERIAL.printf_P("\n\rNext: t2 RunApplication ...\n\r");
    // t2.RunApplication();
    // delay(1500);
    //t2.UploadApplication(payload, (int)sizeof(payload), 0x0);
    //PrintStatus(t2);

}

//
// Main loop
void loop() {
    if (new_key == true) {
        new_key = false;
        USE_SERIAL.printf_P("\n\r");
        switch (key) {
            // *********************************
            // * Test App ||| STDPB1_1 Command *
            // *********************************
            case 'a':
            case 'A': {
                //SetPB1On();
                break;
            }
            // *********************************
            // * Test App ||| STDPB1_0 Command *
            // *********************************
            case 's':
            case 'S': {
                //SetPB1Off();
                break;
            }
            // *********************************
            // * Test App ||| RESETINY Command *
            // *********************************
            case 'x':
            case 'X': {
                //ResetTiny();
                USE_SERIAL.printf_P("\n  .\n\r . .\n\r. . .\n\n\r");
                delay(2000);
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ******************
            // * Restart Master *
            // ******************
            case 'z':
            case 'Z': {
                USE_SERIAL.printf_P("\nResetting ESP8266 ...\n\r\n.\n.\n.\n");
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ********************************
            // * Timonel ::: GETTMNLV Command *
            // ********************************
            case 'v':
            case 'V': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Get bootloader version ...\r\n");
                // tml.GetStatus();
                // PrintStatus(tml);
                break;
            }
            // ********************************
            // * Timonel ::: EXITTMNL Command *
            // ********************************
            case 'r':
            case 'R': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Run Application ...\r\n");
                //tml.RunApplication();
                USE_SERIAL.printf_P("\n. . .\n\r . .\n\r  .\n\n\r");
                app_mode = true;
                delay(2000);
#if ESP8266
                ESP.restart();
#else
                resetFunc();
#endif /* ESP8266 */
                break;
            }
            // ********************************
            // * Timonel ::: DELFLASH Command *
            // ********************************
            case 'e':
            case 'E': {
                //USE_SERIAL.printf_P("\nBootloader Cmd >>> Delete app firmware from T85 flash memory ...\r\n");
                //tml.DeleteApplication();
                delay(750);
                //tml.GetStatus();
                //PrintStatus(tml);
                //TwoStepInit(750);
                break;
            }
            // ********************************
            // * Timonel ::: STPGADDR Command *
            // ********************************
            case 'b':
            case 'B': {
                //byte resetFirstByte = 0;
                //byte resetSecondByte = 0;
                Timonel::Status sts /*= tml.GetStatus()*/;
                if ((sts.features_code & 0x08) == false) {
                    USE_SERIAL.printf_P("\n\rSet address command not supported by current Timonel features ...\n\r");
                    break;
                }
                USE_SERIAL.printf_P("\n\rPlease enter the flash memory page base address: ");
                while (new_word == false) {
                    flash_page_addr = ReadWord();
                }
                if (sts.bootloader_start > MCU_TOTAL_MEM) {
                    USE_SERIAL.printf_P("\n\n\rWarning: Timonel bootloader start address unknown, please run 'version' command to find it !\n\r");
                    //new_word = false;
                    break;
                }
                if ((flash_page_addr > (timonel_start - 64)) | (flash_page_addr == 0xFFFF)) {
                    USE_SERIAL.printf_P("\n\n\rWarning: The highest flash page addreess available is %d (0x%X), please correct it !!!", timonel_start - 64, timonel_start - 64);
                    new_word = false;
                    break;
                }
                if (new_word == true) {
                    USE_SERIAL.printf_P("\r\nFlash memory page base address: %d\r\n", flash_page_addr);
                    USE_SERIAL.printf_P("Address high byte: %d (<< 8) + Address low byte: %d\n\r", (flash_page_addr & 0xFF00) >> 8,
                                        flash_page_addr & 0xFF);
                    new_word = false;
                }
                break;
            }
            // ********************************
            // * Timonel ::: WRITPAGE Command *
            // ********************************
            case 'w':
            case 'W': {
                //tml.UploadApplication(payload, sizeof(payload), flash_page_addr);
                break;
            }
            // ********************************
            // * Timonel ::: READFLSH Command *
            // ********************************
            case 'm':
            case 'M': {
                //byte dataSize = 0;    // flash data size requested to ATtiny85
                //byte dataIX = 0;  // Requested flash data start position
                //tml.DumpMemory(MCU_TOTAL_MEM, RX_DATA_SIZE, VALUES_PER_LINE);
                //DumpFlashMem(MCUTOTALMEM, 8, 32);
                new_byte = false;
                break;
            }
            // ******************
            // * ? Help Command *
            // ******************
            case '?': {
                USE_SERIAL.printf_P("\n\rHelp ...\n\r========\n\r");
                //ShowHelp();
                break;
            }
            // *******************
            // * Unknown Command *
            // *******************
            default: {
                USE_SERIAL.printf_P("[Main] Command '%d' unknown ...\n\r", key);
                break;
            }
                USE_SERIAL.printf_P("\n\r");
                USE_SERIAL.printf_P("Menusero\n\r");
        }
        ShowMenu();
    }
    ReadChar();
}

// Determine if there is a user application update available
bool CheckApplUpdate(void) {
    return true;
}

// Function ReadChar
void ReadChar() {
    if (USE_SERIAL.available() > 0) {
        key = USE_SERIAL.read();
        new_key = true;
    }
}

// Function ReadWord
word ReadWord(void) {
    Timonel::Status sts;    /*= tml.GetStatus() */
    word last_page = 0;     /*= (sts.bootloader_start - PAGE_SIZE) */
    const byte data_length = 16;
    char serial_data[data_length];  /* array to store the received data */
    static byte ix = 0;
    char rc, endMarker = 0xD;  /* standard is: char endMarker = '\n' */
    while (USE_SERIAL.available() > 0 && new_word == false) {
        rc = USE_SERIAL.read();
        if (rc != endMarker) {
            serial_data[ix] = rc;
            USE_SERIAL.printf_P("%c", serial_data[ix]);
            ix++;
            if (ix >= data_length) {
                ix = data_length - 1;
            }
        } else {
            serial_data[ix] = '\0';  /* terminate the string */
            ix = 0;
            new_word = true;
        }
    }
    if ((atoi(serial_data) < 0 || atoi(serial_data) > last_page) && new_word == true) {
        for (int i = 0; i < data_length; i++) {
            serial_data[i] = 0;
        }
        USE_SERIAL.printf_P("\n\r");
        USE_SERIAL.printf_P("WARNING! Word memory positions must be between 0 and %d -> Changing to %d", last_page, (word)atoi(serial_data));
    }
    return ((word)atoi(serial_data));
}

// Function Clear Screen
void ClrScr() {
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[2J");  // clear screen command
    USE_SERIAL.write(27);        // ESC command
    USE_SERIAL.printf_P("[H");   // cursor to home command
}

// Print Timonel instance status
void PrintStatus(Timonel timonel) {
    Timonel::Status tml_status = timonel.GetStatus(); /*Get the instance id parameters received from the ATTiny85 */
    if ((tml_status.signature == T_SIGNATURE) && ((tml_status.version_major != 0) || (tml_status.version_minor != 0))) {
        byte version_major = tml_status.version_major;
        USE_SERIAL.printf_P("\n\r Timonel v%d.%d", version_major, tml_status.version_minor);
        switch (version_major) {
            case 0: {
                USE_SERIAL.printf_P(" Pre-release \n\r");
                break;
            }
            case 1: {
                USE_SERIAL.printf_P(" \"Sandra\" \n\r");
                break;
            }
            default: {
                USE_SERIAL.printf_P(" Unknown \n\r");
                break;
            }
        }
        USE_SERIAL.printf_P(" ====================================\n\r");
        //Timonel::Status tml_status = timonel.GetStatus(); /* Get the instance status parameters received from the ATTiny85 */
        USE_SERIAL.printf_P(" Bootloader address: 0x%X\n\r", tml_status.bootloader_start);
        word app_start = tml_status.application_start;
        if (app_start != 0xFFFF) {
            USE_SERIAL.printf_P("  Application start: 0x%X (0x%X)\n\r", app_start, tml_status.trampoline_addr);
        } else {
            USE_SERIAL.printf_P("  Application start: 0x%X (Not Set)\n\r", app_start);
        }
        USE_SERIAL.printf_P("      Features code: %d\n\n\r", tml_status.features_code);
    }
}

// Function ThreeStarDelay
void ThreeStarDelay(void) {
    delay(2000);
    for (byte i = 0; i < 3; i++) {
        USE_SERIAL.printf_P("*");
        delay(1000);
    }
}

// Function ShowHeader
void ShowHeader(void) {
    //ClrScr();
    delay(250);
    USE_SERIAL.printf_P("\n\rTimonel TWI master libraries test for updating multiple slave ATTiny85s (v1.2 twim-ms)\n\n\r");
}

// Function ShowMenu
void ShowMenu(void) {
    if (app_mode == true) {
        USE_SERIAL.printf_P("Application command ('a', 's', 'z' reboot, 'x' reset T85, '?' help): ");
    } else {
        Timonel::Status sts /*= tml.GetStatus()*/;
        USE_SERIAL.printf_P("Timonel booloader ('v' version, 'r' run app, 'e' erase flash, 'w' write flash");
        if ((sts.features_code & 0x08) == 0x08) {
            USE_SERIAL.printf_P(", 'b' set addr");
        }
        if ((sts.features_code & 0x80) == 0x80) {
            USE_SERIAL.printf_P(", 'm' mem dump");
        }
        USE_SERIAL.printf_P("): ");
    }
}

// Function ListTwidevices
void ListTwiDevices(byte sda, byte scl) {
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[MAX_TWI_DEVS];
    twi.ScanBus(dev_info_arr, MAX_TWI_DEVS);
    for (byte i = 0; i < MAX_TWI_DEVS; i++) {
        USE_SERIAL.printf_P("...........................................................\n\r");
        USE_SERIAL.printf_P("Pos: %02d | ", i + 1);
        if (dev_info_arr[i].firmware != "") {
            USE_SERIAL.printf_P("TWI Addr: %02d | ", dev_info_arr[i].addr);
            USE_SERIAL.printf_P("Firmware: %s | ", dev_info_arr[i].firmware.c_str());
            USE_SERIAL.printf_P("Version %d.%d\n\r", dev_info_arr[i].version_major, dev_info_arr[i].version_minor);
        } else {
            USE_SERIAL.printf_P("No device found\n\r");
        }
        delay(50);
    }
    USE_SERIAL.printf_P("...........................................................\n\n\r");
}

byte GetAllTimonels(Timonel tml_arr[], byte tml_arr_size, byte sda, byte scl) {
    USE_SERIAL.printf_P("\n\n\rGetza !!!\n\r*********\n\n\r");
    TwiBus twi(sda, scl);
    TwiBus::DeviceInfo dev_info_arr[MAX_TWI_DEVS];
    twi.ScanBus(dev_info_arr, MAX_TWI_DEVS);
    for (byte i = 0; i < MAX_TWI_DEVS; i++) {
        if (dev_info_arr[i].firmware == "Timonel") {
            byte tml_addr = dev_info_arr[i].addr;
            tml_arr[i].SetObjTwiAddress(tml_addr);
            delay(250);
            tml_arr[i].DeleteApplication();
            delay(1500);
            PrintStatus(tml_arr[i]);
            delay(150);
        }
        delay(50);
    }
    USE_SERIAL.printf_P("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n\r");
    //Timonel tml_array[timonels];
    return 0;
}