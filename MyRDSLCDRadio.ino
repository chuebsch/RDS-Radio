#include <OneButton.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include "radio.h"
#include "RDSParser.h"
#include "SI4703.h"

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

OneButton menuButton(12, true);
OneButton upButton  (11, true);
OneButton downButton(10, true);
OneButton okButton  ( 9, true);
#define STEREO_PIN  8
#define TRAFICP_PIN 7
#define TRAFICA_PIN 6
#define MUSIC_PIN   5
#define MUC " Music"

#define DARKDISPLAY 5
bool ticktack = false;
//int resetPin = 2;//not here
//int SDIO = A4;
//int SCLK = A5;

uint8_t nfreqm = 0;
uint8_t menumode = 0;
int8_t sender = 0;
uint8_t mFreq[30] = {168, 128, 157, 32, 85, 119, 148, 196, 18, 42, 152, 192, 5, 52, 141, 77, 94, 37, 146, 139, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static unsigned long menuTimeOut = 0ul;
//104.3 100.3 103.2 90.7 96.0 99.4 102.3 107.1 89.3 91.7 102.7 106.7  88.0  92.7  101.6 95.2  96.9
//168,  128,  157,  32,  85,  119, 148,  196,  18,  42,  152,  192,   5,      52, 141,  77,   94,   37,
//DC1E  D210  D318  D311 D312 D313 D314  D315  D220 D6F1 D3F8 |D3F9  |DA1A |1B12| D41C |D3C3 |D3C2 |D311
//MAIN  DLF   ANTB  BR 1 BR 2 BR 3 BR 4  BR 5  DLFK MDRT ANTT |LATH  |EURO |GAXY| KULM |MDR  |MDRJ |BR 1
//101.4 102.1 //A213 FM4

SI4703   radio;    // Create an instance of a SI4703 chip radio.
/// get a RDS parser
RDSParser rds;
int lauf = 0;
char text2[70];

void showAF() {
  char fff[8];
  memset(text2, 0, sizeof(text2));
  int f;
  for (int i = 0; i < 40; i++) {
    if (rds.afList[i] == 0)break;
    f = (875 + rds.afList[i]);
    if (f > 1080) continue;
    sprintf(fff, "%3d.%1d ", f / 10, f % 10);
    if (strlen(text2) < 60) strcat(text2, fff);
  }
}

void displayVolume() {
  uint8_t v, vol = radio.getVolume();
  EEPROM.get(1, v);
  if (v != vol) EEPROM.put(1, vol);
  lcd.setCursor(0, 3);
  lcd.print(F("Vol: "));
  if ((vol >= 0) && (vol < 16)) for (uint8_t i = 0; i < vol; i++) lcd.print("#");
  if (vol < 15) lcd.print(" ");//delete previous #
}

void displayMono() {
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(F("setMonoMode: "));
  lcd.print(radio.getMono());
}

void displaySM() {
  lcd.setCursor(0, 3);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print(F("setSoftMute: "));
  lcd.print(radio.getSoftMute());
}

void doMenuClick() {
  menuTimeOut = millis();
  Serial.print("MenuClick ");
  Serial.print(menumode);
  Serial.print(" => ");
  //draw a menu /or hide the menu
  menumode++;
  Serial.println(menumode);
  switch (menumode) {
    case 0:
      break;
    case 1:
      displayVolume();
      break;
    case 2:
      displayMono();
      break;
    case 3:
      displaySM();
      break;
    case 4:
      lcd.setCursor(0, 3);
      lcd.print("Alt.Frequencies    ");
      showAF();
      break;
    case DARKDISPLAY:
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.noBacklight();
      lcd.noDisplay();
      break;
    default:
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.backlight();
      lcd.display();
      menumode = 0;
      break;
  }
}



void doUpClick() {

  menuTimeOut = millis();
  switch (menumode) {
    case 0:
      //radio.seekUp(true);//to next sender
      sender++;
      if (!mFreq[sender])sender = 0;
      radio.setFrequency((RADIO_FREQ) (8750 + mFreq[sender] * 10));
      EEPROM.put(0, sender);
      break;
    case 1:
      radio.setVolume((radio.getVolume() + 1) % 16);
      displayVolume();
      break;
    case 2:
      radio.setMono(!radio.getMono());
      displayMono();
      break;

    case 3:
      radio.setSoftMute(!radio.getSoftMute());
      displaySM();
      break;
    default:

      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.backlight();
      lcd.display();
      menumode = 0;
      break;
  }
}


void doDownClick() {
  menuTimeOut = millis();
  switch (menumode) {
    case 0:
      radio.seekDown(true);//to next sender

      sender--;
      sender = (sender < 0) ? 29 : sender;
      while (!mFreq[sender]) sender--;
      radio.setFrequency((RADIO_FREQ) (8750 + mFreq[sender] * 10));
      EEPROM.put(0, sender);
      break;
    case 1:
      radio.setVolume(max((radio.getVolume() - 1), 0));
      displayVolume();
      break;
    case 2:
      radio.setMono(!radio.getMono());
      displayMono();
      break;
    case 3:
      radio.setSoftMute(!radio.getSoftMute());
      displaySM();
      break;
    default:
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.backlight();
      lcd.display();
      menumode = 0;
      break;
  }
}

void doOKClick() {
  menuTimeOut = millis();
  if (menumode == 1) {
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    //lcd.print("01234567890123456790");
  }
  lcd.backlight();
  lcd.display();
  menumode = 0;
  ticktack = true;
}

/// Update the Frequency on the LCD display.
void DisplayFrequency(RADIO_FREQ f) {
  char s[12];
  rds.setTunedFreq((f - 8750) / 10);
  radio.formatFrequency(s, sizeof(s));
  Serial.print("FREQ:"); Serial.println(s);
  lcd.setCursor(10, 1);
  lcd.print(s);
} // DisplayFrequency()

/// Update the ServiceName text on the LCD display when in RDS mode.
void DisplayServiceName(char *name) {
  Serial.print("RDS:"); Serial.println(name);
  lcd.setCursor(0, 1);
  lcd.print(name);
} // DisplayServiceName()

void DisplayText(char *text) {
  if (menumode == 4) return;
  for (int i = 0; i < 64; i++) {

    text[i] = (((uint8_t)text[i]) == 0x8d) ? 0xe2  : text[i]; //sz
    text[i] = (((uint8_t)text[i]) == 0x91) ? 0xe1 : text[i]; //ae
    text[i] = (((uint8_t)text[i]) == 0x97) ? 0xef : text[i]; //oe
    text[i] = (((uint8_t)text[i]) == 0x99) ? 0xf5 : text[i]; //ue
  }
  for (int i = 63; i >= 0; i--) {
    if (((uint8_t)text[i]) < 0x21)text[i] = '\0';
    else break;
  }
  strncpy(text2, text, 64);
  strcat(text2, "{ ");
  for (int i = strlen(text) + 2; i < 70; i++) text2[i] = '\0';
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  for (uint8_t i = 0; i < 20; i++)if (text[i] != '\0') {
      lcd.setCursor(i, 2);
      lcd.print(text[i]);
    }
    else break;
} // DisplayText

void DisplayText2() {
  int ll = strlen(text2);
  lauf++;
  for (uint8_t i = 0; i < 20; i++)if (text2[(lauf + i) % ll] != '\0') {
      lcd.setCursor(i, 2);
      lcd.print(text2[(lauf + i) % ll]);
    }
    else break;
} // DisplayText

void DisplayTime(uint16_t mjd, uint16_t minutes) {
  lauf = 0;
  static uint16_t K, Y, M, D;
  Y = uint16_t ((mjd - 15078.2) / 365.25);
  M = int ((mjd - 14956.1 - uint16_t(Y * 365.25)) / 30.6001);
  D = mjd - 14956 - int(Y * 365.25) - uint16_t(M * 30.6001);
  //If M' = 14 or M' = 15, then K = 1; else K = 0
  K = ((M == 14) || (M == 15)) ? 1 : 0;
  Y += K + 1900;
  M -= K * 12;
  M -= 2;
  uint8_t WD = (mjd + 2) % 7;
  const char wt[7][3] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
  const char mo[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  if (M > 11) return;
  if (Y < 2019) return;
  lcd.setCursor(0, 0);
  lcd.print(wt[WD]);
  lcd.print(",");
  if (D < 10) lcd.print("0");
  lcd.print(D);
  lcd.print(".");
  lcd.print(mo[M]);
  lcd.print(".");
  lcd.print(Y);
  lcd.print(" ");
  int h = (minutes / 60) % 24;
  /*
    Serial.print("Zeit:");
    Serial.print(mjd); Serial.print("wd");
    Serial.print(WD); Serial.print(" D");
    Serial.print(D); Serial.print(" M");
    Serial.print(M); Serial.print(" Y");
    Serial.print(Y); Serial.print(" ");
    Serial.print(h); Serial.print(" ");
    Serial.println(minutes % 60);*/
  if (h < 10)lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if ((minutes % 60) < 10)lcd.print("0");
  lcd.print(minutes % 60);
} // DisplayTime()

void RDS_process(uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4) {
  rds.processData(block1, block2, block3, block4);
}

void signalDisplay(uint8_t sig) {
  uint8_t out = map(constrain(sig, 5, 60), 5, 60, 1, 8);
  lcd.setCursor(9, 1);
  switch (out) {
    case 8: lcd.printByte(1); break;
    case 7: lcd.printByte(2); break;
    case 6: lcd.printByte(3); break;
    case 5: lcd.printByte(4); break;
    case 4: lcd.printByte(5); break;
    case 3: lcd.printByte(6); break;
    case 2: lcd.printByte(7); break;
    case 1: lcd.printByte(8); break;
  }
}
void setup() {

  EEPROM.get(0, nfreqm);
  sender = nfreqm;

  //pinMode(12,INPUT_PULLUP);
  //pinMode(11,INPUT_PULLUP);
  //pinMode(10,INPUT_PULLUP);
  //pinMode( 9,INPUT_PULLUP);

  memset(text2, 0, sizeof(text2));
  pinMode(STEREO_PIN , OUTPUT);
  pinMode(TRAFICP_PIN, OUTPUT);
  pinMode(TRAFICA_PIN, OUTPUT);
  pinMode(MUSIC_PIN  , OUTPUT);
  digitalWrite(STEREO_PIN , 1);
  digitalWrite(TRAFICP_PIN, 1);
  digitalWrite(TRAFICA_PIN, 1);
  digitalWrite(MUSIC_PIN  , 1);
  // put your setup code here, to run once:
  // open the Serial port
  Serial.begin(57600);

  // Initialize the lcd
  lcd.init();                      // initialize the lcd
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.clear();
  //Set up custom chars for signal

  uint8_t s1[8]  = {0x1f,  0x01,  0x0f, 0x01, 0x07, 0x01, 0x03, 0x01};
  uint8_t s2[8]  = {0x00,  0x01,  0x0f, 0x01, 0x07, 0x01, 0x03, 0x01};
  uint8_t s3[8]  = {0x00,  0x00,  0x0f, 0x01, 0x07, 0x01, 0x03, 0x01};
  uint8_t s4[8]  = {0x00,  0x00,  0x00, 0x01, 0x07, 0x01, 0x03, 0x01};
  uint8_t s5[8]  = {0x00,  0x00,  0x00, 0x00, 0x07, 0x01, 0x03, 0x01};
  uint8_t s6[8]  = {0x00,  0x00,  0x00, 0x00, 0x00, 0x01, 0x03, 0x01};
  uint8_t s7[8]  = {0x00,  0x00,  0x00, 0x00, 0x00, 0x00, 0x03, 0x01};
  uint8_t s8[8]  = {0x00,  0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  lcd.createChar(1, s1);
  lcd.createChar(2, s2);
  lcd.createChar(3, s3);
  lcd.createChar(4, s4);
  lcd.createChar(5, s5);
  lcd.createChar(6, s6);
  lcd.createChar(7, s7);
  lcd.createChar(8, s8);


  // Initialize the Radio
  radio.init();

  // Enable information to the Serial port
  //radio.debugEnable();

  radio.setBandFrequency(RADIO_BAND_FM, 8750 + mFreq[nfreqm] * 10);
  rds.setTunedFreq(mFreq[nfreqm]);

  // Setup the buttons
  // link the doMenuClick function to be called on a click event.
  menuButton.attachClick(doMenuClick);
  upButton.attachClick(doUpClick);
  downButton.attachClick(doDownClick);
  okButton.attachClick(doOKClick);

  delay(100);

  radio.setMono(false);
  radio.setMute(false);
  uint8_t vol = 9;
  EEPROM.get(1, vol);
  vol = vol & 0x0F;
  radio.setVolume(vol);
  displayVolume();
  radio.attachReceiveRDS(RDS_process);
  rds.attachServicenNameCallback(DisplayServiceName);
  rds.attachTextCallback(DisplayText);
  rds.attachTimeCallback(DisplayTime);
}

void loop() {
top:
  static uint8_t opty = 31;
  uint8_t pty = 0;
  static uint16_t pi, opi;
  unsigned long nowis = millis();
  RADIO_INFO info;
  static unsigned long nextFreqTime = 0ul;
  static unsigned long nextRadioInfoTime = 0ul;
  static unsigned long nextLEDtime = 0ul;
  static unsigned long nextTextTime = 0ul;
  if (ticktack) {
    nextFreqTime = 0ul;
    nextRadioInfoTime = 0ul;
    nextLEDtime = 0ul;
    ticktack = false;
  }
  static RADIO_FREQ lastf = 0;
  RADIO_FREQ f = 0;
  char c;
  menuButton.tick();
  upButton.tick();
  downButton.tick();
  okButton.tick();
  if (menumode == DARKDISPLAY) goto top;//sleep display mode
  else if (nowis - menuTimeOut > 20000ul) {
    if (menumode) {
      menumode = 127;
      Serial.print("Menu timout");
      Serial.println(menumode);
      doMenuClick();
    }
  }
  radio.checkRDS();
  if ((nowis - nextLEDtime) > 100ul) {
    digitalWrite(TRAFICP_PIN, rds.tp());
    digitalWrite(TRAFICA_PIN, rds.ta());
    digitalWrite(MUSIC_PIN, rds.music());
    nextLEDtime = nowis;
  }
  if ((nowis - nextFreqTime) > 400ul) {
    f = radio.getFrequency();
    if (f != lastf) {
      rds.clearRDS();
      DisplayFrequency(f);
      lastf = f;
    } // if
    nextFreqTime = nowis;
  } // if

  if ((nowis - nextTextTime) > 250ul) {
    if (strlen(text2) > 20) DisplayText2();
    nextTextTime = nowis;
  }
  if ((nowis - nextRadioInfoTime) > 900ul) {
    radio.getRadioInfo(&info);
    digitalWrite(STEREO_PIN, (info.stereo) ? HIGH : LOW);
    signalDisplay(info.rssi);
    pty = rds.pty();
    pi = rds.rdsPI();
    if (menumode) {
      opi = 0xFFFF;
      opty = 0x1F;
    }
    if ((menumode == 0) && ((pty != opty) || (opi != pi))) {
      opty = pty;
      opi = pi;
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      switch (pty) {
        case  0: break;
        case  1: lcd.print(F("News")); break;//5
        case  2: lcd.print(F("Current Affairs")); break;//17
        case  3: lcd.print(F("Information")); break;//13
        case  4: lcd.print(F("Sport")); break;//6
        case  5: lcd.print(F("Education")); break;//11
        case  6: lcd.print(F("Drama")); break;//6
        case  7: lcd.print(F("Culture")); break;//9
        case  8: lcd.print(F("Science")); break;//8
        case  9: lcd.print(F("Varied Speech")); break;//15;
        case 10: lcd.print(F("Pop" MUC)); break;
        case 11: lcd.print(F("Rock" MUC)); break;
        case 12: lcd.print(F("Easy Listening")); break;
        case 13: lcd.print(F("Light Classics")); break;
        case 14: lcd.print(F("Serious Classics")); break;
        case 15: lcd.print(F("Other" MUC)); break;
        case 16: lcd.print(F("Weather")); break;
        case 17: lcd.print(F("Finance")); break;
        case 18: lcd.print(F("Children")); break;
        case 19: lcd.print(F("Social Affairs")); break;
        case 20: lcd.print(F("Religion")); break;
        case 21: lcd.print(F("Phone In")); break;
        case 22: lcd.print(F("Travel")); break;
        case 23: lcd.print(F("Hobby")); break;
        case 24: lcd.print(F("Jazz" MUC)); break;
        case 25: lcd.print(F("Country" MUC)); break;
        case 26: lcd.print(F("National" MUC)); break;
        case 27: lcd.print(F("Oldies" MUC)); break;
        case 28: lcd.print(F("Folk" MUC)); break;
        case 29: lcd.print(F("Documentary")); break;
        default: lcd.print(F("ALARM!")); break;
      }/// */
      lcd.setCursor(16, 3);
      lcd.print(pi, HEX);
    }
    nextRadioInfoTime = millis();
  } // update

}

