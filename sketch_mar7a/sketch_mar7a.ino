#include "FastLED.h"
#include <WiFi.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#include <HTTPClient.h>
#include "Update.h"
#include <esp_task_wdt.h>

const char*    ssid = "SSID";               // your network SSID (name)
const char*    password = "password";       // your network password
const String  host = "https://aviationweather.gov/cgi-bin/data/dataserver.php?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=1.25&&mostRecentForEachStation=constraint&stationString=";
// const String  host_update = "http://....../update/2sketch_mar7a.ino.bin"; 
// ajouter le chemin du fichier update si on veut le mettre à jour, 
// décommenter alors getupdate dans setup, le programme se mettra à jour


// Define Timeout watchdog
#define WDT_TIMEOUT         330      // 330 secondes - 5mn30s

// Define Leds
#define No_Stations          34      // Number of Stations also Number of LEDs
#define DATA_PIN              5      // Connect to pin D5/P5 with 330 to 500 Ohm Resistor
#define LED_TYPE         WS2812      // WD2811 or WS2812 or NEOPIXEL
#define COLOR_ORDER         GRB      // WD2811 are RGB or WS2812 are GRB
#define BRIGHTNESS           10      // Master LED Brightness (<12=Dim 20=ok >20=Too Bright/Much Power)
CRGB leds[No_Stations];              // Number of LEDs


// Define STATIONS & Global Variables   
const std::vector<String> Stations {    //   << Set Up   - Do NOT change this line
  "NULL, STATION NAME         ",        // 0 << Reserved - Do NOT change this line
  "LPPT, LISBONE, GA          ",        // 1
  "LEMD, MADRID, GA           ",        // 2
  "LEVC, VALENCE, GA          ",        // 3
  "LEBL, BARCELONE, GA        ",        // 4
  "LFBO, TOULOUSE, GA         ",        // 5
  "LFMT, MONTPELLIER, GA      ",        // 6
  "LFTW, UZES NIMES, GA       ",        // 7
  "LFML, MARSEILLE, GA        ",        // 8
  "LFMN, NICE, GA             ",        // 9
  "LFKJ, AJACCIO, GA          ",        // 10
  "LIEE, CAGLIARI, GA         ",        // 11
  "LIRN, NAPLES, GA           ",        // 12
  "LIRP, PISE, GA             ",        // 13
  "LIPZ, VENISE, GA           ",        // 14
  "LIMC, MILAN, GA            ",        // 15
  "LSGG, GENEVE, GA           ",        // 16
  "LFLL, LYON, GA             ",        // 17
  "LFLC, CLERMONT FERRAND, GA ",        // 18
  "LFPO, ORLY, GA             ",        // 19
  "LFPG, CDG, GA              ",        // 20
  "LFST, STRASBOURG, GA       ",        // 21
  "LSZH, ZURICH, GA           ",        // 22
  "EDDM, MUNICH, GA           ",        // 23
  "EDDF, FRANKFURT, GA        ",        // 24
  "ESGG, GOTEBORG, GA         ",        // 25
  "EHAM, AMSTERDAM, GA        ",        // 26
  "EGLL, LONDRE, GA           ",        // 27
  "EIDW, DUBLIN, GA           ",        // 28
  "LFRC, CHERBOURG, GA        ",        // 29
  "LFRB, BREST, GA            ",        // 30
  "LFRZ, STNAZAIRE, GA        ",        // 31
  "LFBH, LAROCHELLE, GA       ",        // 32
  "LFBD, BORDEAUX, GA         ",        // 33
  "LFBA, AGEN, GA             ",        // 34
};                                      // << Do NOT change this line

String         rem[No_Stations + 1];  // Remarks
String        wind[No_Stations + 1];  // wind speed
String    category[No_Stations + 1];  // NULL   VFR    MVFR   IFR    LIFR
//..............................................Black  Green   Blue   Red    Magenta


#define LED_BUILTIN 2  // ON Board LED GPIO 2
String metar;          // Raw METAR data
int httpCode;          // Error Code
bool paimp = true;     // Flag clignotement led (pair - impair)


void setup() {
  esp_task_wdt_init(WDT_TIMEOUT, true);   // Init watchdog 5mn30s
  esp_task_wdt_add(NULL);
  pinMode(LED_BUILTIN, OUTPUT);         // the onboard LED
  Init_LEDS();                             // Initialize LEDs
  digitalWrite(LED_BUILTIN, HIGH);          // ON
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(300);                          // Wait a little bit
  }
  digitalWrite(LED_BUILTIN, LOW);         // OFF
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  digitalWrite(LED_BUILTIN, HIGH);         // ON
  delay(200);                             // Wait a smidgen
  digitalWrite(LED_BUILTIN, LOW);          // OFF
  // Getupdate(host_update);
}

unsigned long previousMillis;
const unsigned long period = 1000*60*5;   // 5 mn


void loop() {
  GetAllMetars();                        // Get All Metars and Display Categories
  previousMillis = millis();
  while ((millis() - previousMillis) < period) {
    Weather_LEDS();                     // Display Weather_LEDS();
  }
  esp_task_wdt_reset();                   // Reset program watchdog si Time > 5mn
}

// *********** Initialize LEDs
void Init_LEDS() {
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, No_Stations).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, No_Stations, CRGB::Black);
  FastLED.show();
  delay(100);                           // Wait a smidgen
}

// *********** GET All Metars in Chunks
void GetAllMetars() {  
  int Step = 7;                         // 5 Stations at a time
  for (int j = 0; j < No_Stations; j = j + 1 + Step) {
    int Start  = j;
    int Finish = Start + Step;
    if (Finish > No_Stations)  Finish = No_Stations;
    String urls = "";
    for (int i = Start; i <= Finish; i++) {
      String station = Stations[i].substring(0, 5);
      //if (station == "NULL")   return;

      urls = urls + String (station);
    }
    int len = urls.length();
    urls = urls.substring(0, len - 1);   // Remove last "comma"
    GetData(urls); 
    delay(750);                          // GET Some Metar Data 5 Stations at a time
    for (int i = Start; i <= Finish; i++) {
      ParseMetar(i);                     // Parse Metar Data one Station at a time
    }
  }  
}

// *********** GET Some Metar Data/Name
void GetData(String urls) {
  metar = "";                            // Reset Metar Data
  if (wifiMulti.run() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);      // ON
    HTTPClient https;
    https.begin(host + urls);
    httpCode = https.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        metar = https.getString();
      }
      https.end();
      digitalWrite(LED_BUILTIN, LOW);     // OFF
    } else {
      https.end();                        // Communication HTTP Error
      digitalWrite(LED_BUILTIN, LOW);     // OFF
    }
  } 
}

// ***********   Parse Metar Data
void ParseMetar(int i) {
  String parsedmetar = "";
  String station = Stations[i].substring(0, 4);
  if (station == "NULL")   return;
  int data_Start = metar.indexOf(station);                    // Search for station id
  int data_end  = metar.indexOf("</METAR>", data_Start + 1);  // Search for "data end"
  if (data_Start > 0 && data_end > 0)    {
    parsedmetar = metar.substring(data_Start, data_end);      // Parse Metar Data
    Decodedata(i, station, parsedmetar);                      // DECODE the Station DATA
  } else {
    category[i] = "NF";                 // Not Found
    rem[i] = "Station Not Reporting";   // Not Found
    if (httpCode < 0)  rem[i] = "Connection Error";
    wind[i] = "NA";                     // Not Found
    Display_LED(i, 20);                 // Display Station LED
  }
}

// *********** DECODE the Station DATA
void Decodedata(int i, String station, String parsedmetar) {

  // Searching Remarks
  int search_Strt = parsedmetar.indexOf(station, 0) + 7;       // Start of Remark
  int search_End = parsedmetar.indexOf("</raw_text", 0);       // End of Remark
  rem[i] = parsedmetar.substring(search_Strt, search_End);

  // Searching flight_category
  int search0 = parsedmetar.indexOf("<flight_category") + 17;
  int search1 = parsedmetar.indexOf("</flight_category");
  if (search0 < 17) category[i] = "NA";  else  category[i] = parsedmetar.substring(search0, search1);

  // Searching wind_speed_kt
  search0 = parsedmetar.indexOf("<wind_speed_kt") + 15;
  search1 = parsedmetar.indexOf("</wind_speed");
  if (search0 < 15) wind[i] = "NA";  else  wind[i] = parsedmetar.substring(search0, search1); // + " KT";
  Display_LED(i, 20);                   // Display One Station LED
}

// *********** Display One Station LED
void Display_LED(int index, int wait) {
  if (index == 0)  return;
  leds[index - 1] = CRGB::Black;
  FastLED.show();
  delay(wait);
  Set_Cat_LED(index);                   // Set Category for This Station LED
  FastLED.show();
}

// *********** Display Remarques LED
void Display_RemLED (int i){
      leds[i - 1] = CRGB::Green; 
      if (rem[i].indexOf("FEW") != -1){leds[i - 1] = CRGB(50,255,10);}       // Quelques nuages
      if (rem[i].indexOf("SCT") != -1){leds[i - 1] = CRGB(80,255,10);}       // Nuages
      if (rem[i].indexOf("BR") != -1){leds[i - 1] = CRGB::DarkGray;}         // Brume
      if (rem[i].indexOf("BKN") != -1){leds[i - 1] = CRGB::Yellow;}          // Fragmenté 
      if (rem[i].indexOf("FG") != -1){leds[i - 1] = CRGB::DarkGray;}         // Brouillard
      if (rem[i].indexOf("OVC") != -1){leds[i - 1] = CRGB::Yellow;}          // Couvert   
      if (rem[i].indexOf("CB") != -1){leds[i - 1] = CRGB::DarkOrange;}       // Cumulonimbus
      if (rem[i].indexOf("TCU") != -1){leds[i - 1] = CRGB::DarkOrange;}      // Imposants Cumulonimbus
      if (rem[i].indexOf("DZ") != -1){leds[i - 1] = CRGB::DarkOrange;}       // Bruine
      if (rem[i].indexOf("RA") != -1){leds[i - 1] = CRGB::DarkRed;}          // Pluie
      if (rem[i].indexOf("SHRA") != -1){leds[i - 1] = CRGB::OrangeRed;}      // Averse
      if (rem[i].indexOf("TSRA") != -1){leds[i - 1] = CRGB::OrangeRed;}      // Orage Pluie
      if (rem[i].indexOf("+SHRA") != -1){leds[i - 1] = CRGB::DarkRed;}       // Forte averse
      if (rem[i].indexOf("+TSRA") != -1){leds[i - 1] = CRGB::DarkRed;}       // Fort Orage Pluie
      if (rem[i].indexOf("SG") != -1){leds[i - 1] = CRGB::DarkViolet;}       // Grésil
      if (rem[i].indexOf("GS") != -1){leds[i - 1] = CRGB::DarkViolet;}       // Neige roulée
      if (rem[i].indexOf("GR") != -1){leds[i - 1] = CRGB::DarkViolet;}       // Grêle
      if (rem[i].indexOf("FZDZ") != -1){leds[i - 1] = CRGB::DarkCyan;}       // Bruine verglaçante
      if (rem[i].indexOf("FZFG") != -1){leds[i - 1] = CRGB::DarkCyan;}       // Brouillard givrant
      if (rem[i].indexOf("SN") != -1){leds[i - 1] = CRGB::DarkBlue;}         // Neige
}

// ***********  Display Weather
void Weather_LEDS(){
   for(int j = 0; j < 16; j++){
      for (int i = 1; i < (No_Stations + 1); i++)  {
         Display_RemLED(i);
         int Wind = wind[i].toInt();        
         if(Wind>12){
            if(paimp){
              if((i & 1) == 0){
                 leds[i - 1].fadeLightBy(17*j);       
              }
              else{
                 Display_RemLED(i);
                 leds[i - 1].fadeLightBy(255-(17*j));    
              }
            }else{
              if((i & 1) == 0){
                 Display_RemLED(i);
                 leds[i - 1].fadeLightBy(255-(17*j));   
              }
              else{
                 leds[i - 1].fadeLightBy(17*j);     
              }      
            }
         }
      }
      if(j==15){
         FastLED.delay(700);
      }else{
         FastLED.delay(15);          
      }
   }         
   paimp = !paimp;
}

// *********** Set Category for One Station LED
void Set_Cat_LED(int i)  {
  if (category[i] == "NF" )  leds[i - 1] = CRGB(20, 20, 0); // DIM  Yellowish
  if (category[i] == "NA" )  leds[i - 1] = CRGB(20, 20, 0); // DIM  Yellowish
  if (category[i] == "VFR" ) leds[i - 1] = CRGB::DarkGreen;
  if (category[i] == "MVFR") leds[i - 1] = CRGB::DarkBlue;
  if (category[i] == "IFR" ) leds[i - 1] = CRGB::DarkRed;
  if (category[i] == "LIFR") leds[i - 1] = CRGB::DarkMagenta;
} 

int totalLength;                           // total size of firmware
int currentLength = 0;                     // current size of written firmware

// *********** Update Firmware
void updateFirmware(uint8_t *data, size_t len){
  Update.write(data, len);
  currentLength += len;
  if(currentLength != totalLength) return;
  Update.end(true);
  ESP.restart();
}

// *********** GET Update
void Getupdate(String source_up){
  if (wifiMulti.run() == WL_CONNECTED) {
      HTTPClient client;
      client.begin(source_up);
      // client.setTimeout(1);
      int resp = client.GET(); 
      if(resp > 0){
         totalLength = client.getSize();
         int len = totalLength;
         Update.begin(UPDATE_SIZE_UNKNOWN);
         uint8_t buff[128] = { 0 };
         WiFiClient * stream = client.getStreamPtr();
         while(client.connected() && (len > 0 || len == -1)) {
            size_t size = stream->available();
            if(size) {
               int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
               updateFirmware(buff, c);
               if(len > 0) {
                  len -= c;
               }
            }
            delay(1);
         }
      }
      client.end();
   }
}

