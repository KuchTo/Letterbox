#include <LiquidCrystal.h>
#include <avr/sleep.h>
 
//PIN    Function         CPU Port   CPUPin  
//0      Ser RXD 2        PD0         2
//1      Ser TXD 3        PD1         3
//2      INT 0            PD2         4
//3      INT 1            PD3         5
//4      T0               PD4         6
//5      T1               PD5         11
//6      AIN0             PD6         12
//7      AIN1             PD7        13
//8      ICP              PB0         14
//9      OC1/PCINT1      PB1         15
//10     SS(slave Select)PB2         16
//11     MOSI             PB3         17
//12     MISO             PB4         18
//13     SLK              PB5         19
//na    PCINT6/OSC1      PB6        9
//na    PCINT7/OSC2      PB7        10
//A1     ADC0            PC0         23
//A2     ADC1             PC1        24
//A3     ADC2             PC2        25
//A4     ADC3             PC3        26
//A5     ADC4/SDA I2C     PC4        27
//A6     ADC5/SCL         PC5        28
//na    Reset/PCINT14   PC6        1
//na    VCC                        7
//na    GND                        8
//na    GND                        22
//na    ADVCC Reference            20
//na    AREF AD Converter          21

//Constants & Definitions
const float Warnvoltage = 4.0;
const float Sleepvoltage = 3.2;
const int Scrollspeed = 700;
const long interval = 1000;    // Definition of a Second in Milliseconds

//const byte VoltTestPin = 0;  // Test Analogeingang 0
const byte VoltTestPin = 2; // Produktiv Alalogeingang 2

//Real Time Clock 
long previousMillis = 0;       // will store last time was measured
byte seconds = 0; 
byte minute = 0;
byte hour = 0;

// String Defitions
String LetterBoxOwner = "  Hans Otto   ";  // Bitte in eigenen Namen ändern !
String StandardScolltext = "Bitte keine Werbung und keine kostenlosen Zeitungen einwerfen. Danke ! -   ";
String LetterArrivedScolltext = "Vielen Dank für die Zustellung der Postsendung. Der Empfänger wird benachrichtigt.   ";
String BattInfo;

//Program logic
byte CharPointers[200];
float value;
int Voltage;
boolean BatteryEmpty = false;
boolean IsSensorActive = false; // Bei einem Reset wird ein erkanntes High Signal (Geöffnneter Schalter Ignoriert), wenn kein Schalter von Anfang an ! exisiert (Dauerhaft HighPegel)
byte Displaymode = 0;
byte oldDisplaymode = 0;
boolean MailAlreadyDelivered = false;
int Switchstate = 1;
byte DispLATextOneTime = 0;
byte DispLATextOneTimeWASDisp = 0;
int cmpMsgWith = 0;
int MAMsgDC = 0;
byte MatC = 0;     // 12 Stunden Anzeige des Symbole "Neue Nachricht"

byte newChar[8] = {
  0b11111,
  0b10011,
  0b10101,
  0b11001,
  0b11001,
  0b10101,
  0b10011,
  0b11111
};

// LiquidCrystal(rs, enable, d4, d5, d6, d7);
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void InitLcdScreen ()
{
lcd.clear();
lcd.setCursor(0,0);
lcd.print("++ Letterbox ++ ");
lcd.setCursor(0,1);
lcd.print(" tobias.kuch at ");
delay(2000);
lcd.setCursor(0,0);
lcd.print(LetterBoxOwner);
lcd.setCursor(0,1);
lcd.print(" googlemail.com ");
delay(3000);
}

void setup() 
     {
     for (int a=0; a <200;a++)
     {
     CharPointers[a] = a; 
     }
     lcd.createChar(0, newChar);
     lcd.begin(16, 2);              // start the library
     pinMode(10,INPUT_PULLUP);              // for new Mail arrival Detection LOW: Klappe zu HIGH: klappe offen
     Switchstate = digitalRead(10);
     InitLcdScreen();
     analogReference(DEFAULT);
     if (!Switchstate) {IsSensorActive = true; } 
}



void NormalScrolltext ()
{
 int cmpMsgWith = StandardScolltext.length()- 2;
 Displaymode = 0;
 delay (Scrollspeed);
 lcd.setCursor(0,1);
 for (int a=0; a <17;a++)
     {
     lcd.print(StandardScolltext[CharPointers[a]]);
     }

     for (int a=0; a <cmpMsgWith;a++)
     {
     CharPointers[a] = CharPointers[a] + 1; 
     if (CharPointers[a] > cmpMsgWith) {CharPointers[a] = 0; }
     }  
}

void BatteryWarningScrolltext ()
{
 BattInfo = "- Achtung! Batteriespannung: " + String(Voltage * 3) + " Volt. Bitte Akku aufladen. - " + StandardScolltext + "  ";
 int cmpMsgWith = BattInfo.length()- 2;
 Displaymode = 2;
 delay (Scrollspeed /2 );
 lcd.setCursor(0,1);
 for (int a=0; a <17;a++)
     {
     lcd.print(BattInfo[CharPointers[a]]);
     }

     for (int a=0; a <cmpMsgWith;a++)
     {
     CharPointers[a] = CharPointers[a] + 1; 
     if (CharPointers[a] > cmpMsgWith) {CharPointers[a] = 0; }
     }  
}


void LetterArrScrolltext ()
{
 int cmpMsgWith = LetterArrivedScolltext.length()- 2;
 Displaymode = 3;
 delay (Scrollspeed -50 );
 lcd.setCursor(0,1);
 for (int a=0; a <17;a++)
     {
     lcd.print(LetterArrivedScolltext[CharPointers[a]]); 
     }

     for (int a=0; a <cmpMsgWith;a++)
     {
     CharPointers[a] = CharPointers[a] + 1; 
     if (CharPointers[a] > cmpMsgWith) 
     {
     CharPointers[a] = 0;
     DispLATextOneTimeWASDisp = 1;
     }
     }  
}

void Powerdown ()  // Toggle Powerdown, if critical Voltage is reached.
{
 lcd.setCursor(0,1);
 lcd.print("                  ");
 lcd.setCursor(0,1);
 lcd.print("Sys in SleepMode ");
 set_sleep_mode(SLEEP_MODE_PWR_DOWN);
 cli();
 sleep_enable();
 sei();
 sleep_cpu();
 //restart
}

boolean BattWarning ()
{  
  value = analogRead(VoltTestPin);    // read the input pin
  value = value  * 0.00488; 
  if (value <= Sleepvoltage) {
  Voltage = value;
  Powerdown();
  } 
  if (value <= Warnvoltage) {
  Voltage = value;
    return true; 
  } 
  {
  return false;
  }  
}

void runrealTimeClock()
{
  // Real Time Clock & Countdown
unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > interval)
  {
     previousMillis = currentMillis;
// Countdown    
// Countup 24 St Hr.
     seconds = seconds+1;
     if (seconds > 59)
      {
          seconds = 0;
          minute=minute+1;         
          BatteryEmpty = BattWarning ();
          if (minute > 59)
          {
           minute =0;
           if (MatC > 1) { MatC --;} //Anzeige der des Nachricht Symbols 
           hour=hour + 1;          
          } 
          if (hour > 23)
          {
           hour = 0;
          }
      }    
  }
}

void ChangeDisplayMode()
{
  if ( Displaymode > oldDisplaymode )
  {
    oldDisplaymode = Displaymode;
     for (int a=0; a <200;a++)
     {
     CharPointers[a] = a; 
     }
  }
    if ( Displaymode < oldDisplaymode )
  {
    oldDisplaymode = Displaymode;
     for (int a=0; a <200;a++)
     {
     CharPointers[a] = a; 
     }
  }
}

void DisplayMailArrivedSig(boolean trzstat)
{
byte custHeartChar = 0; 
  if (trzstat)
{    
    lcd.setCursor(15,0);
    lcd.write(custHeartChar);
} else
{  
  lcd.setCursor(15,0);
  lcd.write(" ");
} 
}

void NewLetterArrivedCheck()
{
int inPin = 10;
Switchstate = digitalRead(inPin);    
// if (Switchstate && !MailAlreadyDelivered)
if (Switchstate == true )
  { 
  MailAlreadyDelivered = true;
  DisplayMailArrivedSig(true);
  MatC = 11;  // Mail Arrived Zeitanzeiger (Vorladung)
     // MAMsgDC = LetterArrivedScolltext.length();
   DispLATextOneTime = 1;     
  } 
  if (MatC == 1)
    {
      DisplayMailArrivedSig(false);
      MailAlreadyDelivered = false;
      MatC = 0;
     DispLATextOneTime = 0;
     DispLATextOneTimeWASDisp = 0;
    }
}
      
void loop()
{
runrealTimeClock();
if (!BatteryEmpty)
{
  if ( DispLATextOneTime == 1 && DispLATextOneTimeWASDisp == 0)
 {
   LetterArrScrolltext ();
 } 
 else
{ NormalScrolltext (); }
}
if (BatteryEmpty)
{
 BatteryWarningScrolltext ();
}
ChangeDisplayMode();
if  (IsSensorActive) {NewLetterArrivedCheck(); }            // test if a new letter was arrived if so, mark it in Display for next 12 Hours
}
