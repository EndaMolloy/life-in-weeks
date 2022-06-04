/*******************************************************************
    A life in weeks counter inspired by blog waitbutwhy
    It's designed for a 64x64 matrix
    Parts Used:
      ESP32 Trinity - https://github.com/witnessmenow/ESP32-Trinity
    Written by Enda Molloy

 *******************************************************************/
// ----------------------------
// Standard Libraries - Already Installed if you have ESP32 set up
// ----------------------------

#include <WiFi.h>
#include <FS.h>
#include <SPIFFS.h>
#include "time.h"
#include "math.h"

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <WiFiManager.h>
// Captive portal for configuring the WiFi

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
// This is the library for interfacing with the display

// Can be installed from the library manager (Search for "ESP32 MATRIX DMA")
// https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA


// -------------------------------------
// ------- Replace the following! ------
// -------------------------------------

// Wifi network station credentials
char ssid[] = "";     // your network SSID (name)
char password[] = ""; // your network key


// -------------------------------------
// -------   Matrix Config   ------
// -------------------------------------

const int panelResX = 64;  // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;  // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1; // Total number of panels chained one to another

// See the "displaySetup" method for more display config options

//------------------------------------------------------------------------------------------------------------------

MatrixPanel_I2S_DMA *dma_display = nullptr;

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myRED = dma_display->color565(139, 30, 30);
uint16_t myGrey = dma_display->color565(30, 30, 30);
uint16_t myGREEN = dma_display->color565(102, 102, 0);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;   //Replace with your GMT offset (seconds)
const int   daylightOffset_sec = 0;  //Replace with your daylight offset (seconds)
unsigned long unix_start_time, unix_death_time;
unsigned int years_to_death;
unsigned int start_age;

void displaySetup()
{
  HUB75_I2S_CFG mxconfig(
      panelResX,  // module width
      panelResY,  // module height
      panel_chain // Chain length
  );

  // If you are using a 64x64 matrix you need to pass a value for the E pin
  // The trinity connects GPIO 18 to E.
  // This can be commented out for any smaller displays (but should work fine with it)
  mxconfig.gpio.e = 18;

  // May or may not be needed depending on your matrix
  // Example of what needing it looks like:
  // https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-I2S-DMA/issues/134#issuecomment-866367216
  mxconfig.clkphase = false;

  // Some matrix panels use different ICs for driving them and some of them have strange quirks.
  // If the display is not working right, try this.
  //mxconfig.driver = HUB75_I2S_CFG::FM6126A;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
}

void setup()
{

  Serial.begin(115200);
  unix_start_time=1354233600; //2013-01-01 00:00
  start_age = 26; //max age you are during year of unix_start_time
  unix_death_time=3376699200; //2077-01-01 04:00 start time + 64 years (64 pixels)

  years_to_death=(floor(unix_death_time-unix_start_time)/31540000); //years required for num rows

  displaySetup();
  dma_display->clearScreen();

  WiFiManager wifiManager;
  //reset saved settings
   
  wifiManager.autoConnect("AutoConnectAP");
  //if you get here you have connected to the WiFi

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print(F("connected. Local IP: "));
    Serial.println(WiFi.localIP());
  }

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

}

void loop()
{
  int weeks_elapsed;
  weeks_elapsed = get_weeks_elapsed();

  // years on y-axis, weeks on x-axis
  for (int years = 0; years < years_to_death; years++) {
    for (int weeks = 0; weeks<= 52; weeks++){
      paint_matrix(years, weeks, weeks_elapsed);
    }
    paint_decade(years);
  }
  delay(86400000); //once a day
}

void paint_matrix(int row, int col, int weeks_elapsed){
  int week_num = row*52 + col;

  if (week_num < weeks_elapsed){
    dma_display->drawPixel(6 + col, row, myRED); 
  }else{
    dma_display->drawPixel(6 + col, row, myGrey); 
  }
}

void paint_decade(int row){

  if((row + start_age)%10 == 0){
    dma_display->drawPixel(5, row, myGREEN); 
  }else{
    dma_display->drawPixel(5, row, myBLACK); 
  }
   
}

int get_weeks_elapsed()
{
  time_t now;
  int weeks_elapsed;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  Serial.print("Weeks Elapsed: ");
  weeks_elapsed=floor((now-unix_start_time)/604800);
  Serial.println(weeks_elapsed);
  return weeks_elapsed; 
}
