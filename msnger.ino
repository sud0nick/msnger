#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 13, 9, 4, 5, 6, 7);

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!

// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  19
#define ADAFRUIT_CC3000_CS    2

// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "SSID"
#define WLAN_PASS       "password"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

// Timeout for connection to web server
#define IDLE_TIMEOUT_MS  10000

// Define the web page to grab
#define WEBSITE      "www.puffycode.com"
#define DEF_WEBPAGE  "/messenger/msgrecv.php"

// Define button presses
#define UP 141
#define DOWN 326
#define LEFT1 502
#define LEFT2 503
#define RIGHT 0
#define SELECT 738

// Define the max num of characters allowed on screen
#define CHAR_LIMIT 39

/* VARIABLE DECLARATIONS */
boolean connectionState = false;  // Variable for keeping track of the connection to the AP
boolean lcdIndex = 0;             // Keep track if messages[0] or messages[1] is displayed
unsigned long timer;              // Timer for refreshing the messages at an interval
unsigned short msgIndex;          // Index of current message
unsigned short msgCount;          // Total number of messages
String messages[2] = {"", ""};    // Array to hold the message ([0] Message, [1] Overflow of LCD)
String displayCount = "";         // String to hold the index / msgCount
String author = "";               // String for the author's name
unsigned short lcdCycle = 0;      // Integer for counting cycles

uint32_t ip;

void setup(void)
{
  //Serial.begin(115200);
  lcd.begin(40,2);
  pinMode(A0, INPUT);

  //Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  // Start and setup the CC3000
  setupCC3000();
 
  // Pull the web page.
  getWebpage(DEF_WEBPAGE);
}

void loop(void)
{
  if (connectionState) {
    if (!timer) {
      timer = millis();
    } else if ((millis() - timer) >= 300000UL) {
      getWebpage(DEF_WEBPAGE);
      timer = NULL;
    }
  }
  // Read the state of the buttons on the LCD shield
  switch (analogRead(A0)) {
    case UP:
      connectAP();
      getWebpage(DEF_WEBPAGE);
      break;
    case DOWN:
      disconnectAP();
      break;
    case RIGHT:
      nextMsg();
      break;
    case LEFT1:
    case LEFT2:
      if (msgIndex == 1) {break;}
      prevMsg();
      break;
    case SELECT:
      getWebpage(DEF_WEBPAGE);
      break;
    default:
      break;
  }
  
  if (lcdCycle++ == CHAR_LIMIT && messages[1].length() > 0) {
    // Swap the messages on screen
    if (lcdIndex) {
      setScreen(messages[0], displayCount, 0);
      lcdIndex = 0;
    } else {
      setScreen(messages[1], "", 0);
      lcdIndex = 1;
    }
    lcdCycle = 0;
  }
  lcd.scrollDisplayLeft();
  delay(250);
}

void setScreen(String topline, String bottomline, int pos) {
  lcd.clear();
  lcd.setCursor(pos,0);
  lcd.print(topline);
  lcd.setCursor(pos,1);
  lcd.print(bottomline);
}

void setupCC3000() {
  /* Initialise the module */
  //Serial.println(F("\nInitializing..."));
  setScreen("Initializing", "CC3000...", 0);
  if (!cc3000.begin())
  {
    //Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  setScreen("Setting the Mac", "Address", 0);
  
  /* Optional: Update the Mac Address to a known value */
  uint8_t macAddress[6] = { 0x08, 0x00, 0x28, 0x01, 0x79, 0xB7 };
   if (!cc3000.setMacAddress(macAddress))
   {
     //Serial.println(F("Failed trying to update the MAC address"));
     while(1);
   }
   
   // Connect to the AP
   connectAP();
}

void connectAP() {
  setScreen("Connecting to ", WLAN_SSID, 0);
  //Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    //Serial.println(F("Failed!"));
    while(1);
  }
  
  // Connection established
  //Serial.println(F("Connected!"));
  setScreen("Connected...", "", 0);
  connectionState = true;
  
  /* Wait for DHCP to complete */
  //Serial.println(F("Request DHCP"));
  setScreen("Requesting IP", "Address...", 0);
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }
  ip = 0;
  // Try looking up the website's IP address
  //Serial.print(WEBSITE); Serial.print(F(" -> "));
  setScreen("Resolving", WEBSITE, 0);
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      //Serial.println(F("Couldn't resolve!"));
      setScreen("Couldn't resolve...", "", 0);
    }
    delay(500);
  }
  cc3000.printIPdotsRev(ip);
}

void getWebpage(char *webpage) {
  setScreen("Retrieving", "messages...", 0);
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(webpage);
    //Serial.print("Requesting webpage -> ");
    //Serial.println(webpage);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    //Serial.println(F("Connection failed"));
    setScreen("Connection failed...", "", 0);
    return;
  }
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  boolean inMsg, inMsgIndex, inMsgCount, inAuthor;
  inMsg = inMsgIndex = inMsgCount = inAuthor = false;
  author = "";
  messages[0] = messages[1] = "";
  unsigned int i = 0;     // Used to keep track of loops when assigning msgIndex and msgCount
  int tempIndex[5] = {0,0,0,0,0};
  //Serial.println("\n");
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      if (c == '%') {
        inMsg = (inMsg == false) ? true : false;
        continue;
      }
      if (inMsg) {
        // Check for the first delimiter
        if (c == '^') {
          inMsgIndex = (inMsgIndex == false) ? true : false;
          
          // At this point if we are in the msgCount area this means
          // we have reached the end of the delimited space and need to 
          // convert the int array to a single integer
          if (inMsgCount) {
            msgCount = convertArrayToInt(tempIndex, i);
            
            // Now set the inAuthor boolean to true because the next
            // part of the packet is the author's name
            inAuthor = true;
            //Serial.print("msgCount = ");
            //Serial.println(msgCount, DEC);
          }
          continue;
        }
        
        if (c == ':') {
          inAuthor = false;
          continue;
        }
        
        // Check for the second delimiter
        if (c == '/') {
          // At this point we have collected the index
          msgIndex = convertArrayToInt(tempIndex, i);
          //Serial.print("msgIndex = ");
          //Serial.println(msgIndex, DEC);
          
          // We are now in msgCount and need to reset the i counter
          inMsgCount = (inMsgCount == false) ? true : false;
          i = 0;
          memset(tempIndex, 5, sizeof(int));
          continue;
        }
        
        if (inMsgIndex) {
          if (inMsgCount) {
            tempIndex[i] = c - '0';
            i++;
          } else {
            tempIndex[i] = c - '0';
            i++;
          }
          continue;
        }
        
        if (inAuthor) {
          author += c;
          continue;
        }
        
        // Copy the character to the message
        if (messages[0].length() < CHAR_LIMIT) {
          messages[0] += c;
        } else {
          messages[1] += c;
        }
      }
      lastRead = millis();
    }
  }
  www.close();
  
  displayCount = (String)"Msg " + msgIndex + "/" + msgCount + "  By: " + author;
  lcdIndex = lcdCycle = 0;
  setScreen(messages[0], displayCount, 0);
  //Serial.println(message);
}

void disconnectAP() {
  // Disconnect from the AP
  //Serial.println(F("\n\nDisconnecting"));
  cc3000.disconnect();
  
  // Set the connection state to false and stop the timer
  connectionState = false;
  timer = NULL;
  
  // Set the LCD display to disconnected
  setScreen("Disconnected from AP...", "", 0);
}

void prevMsg() {
  char temp[64];
  sprintf(temp, "%s?index=%d", DEF_WEBPAGE, msgIndex - 1);
  getWebpage(temp);
}

void nextMsg() {
  char temp[64];
  sprintf(temp, "%s?index=%d", DEF_WEBPAGE, msgIndex + 1);
  getWebpage(temp);
}

int convertArrayToInt(int *a, int aSize) {
  int i, b = 0;
  for (i = 0; i < aSize; i++) {
    b = (b * 10) + a[i];
  }
  return b;
}
