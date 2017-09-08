/*
  Rafael V. Aroca - aroca@ufscar.br

  12/11/2016

  This software is open source, unde the GPL Licence. Use at your own risk!

  Where am I? / Onde Estou?

  Electronics:
  Arduino Uno, Leonardo or Mega + Ethernet Shield + LCD Keypad Shield
  Tested with DF Robot LCD Keypad
  and Chinese Ethernet Shield (compatible with the orieingal)

  Important: Arduino pin 10 is used both for the LCD display and for the
  Ethernet shield. So I had to bend the pin from the LCD display shield,
  in order to avoid it to conflict/interfere with the Ethernet shield


  This sketch continuous queries a "calendar server" and shows the current, date, time and event on the LCD display

  An intermeddiate server, in PHP, queries events from a Google Calendar. See PHP file.

*/



#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>
#include "avr/wdt.h"

//Para definir o timeout da Ethernet
#include <utility/w5100.h>


int firstTime = 1;

//Simple state machine for the response reception
int fsm_reception;

//IP Address for the webserver where the PHP page resides
char serverHost[] = "10.7.5.1";

// define some values used by the panel and buttons
const int LCD_COLS = 16;
int OFFSET = 18; //Offset entre a data e hora e a mensagem da segunda linha enviada pelo servidor
int auto_scroll=0;
int lcd_limit;

int lcd_key     = 0;
int adc_key_in  = 0;
int lcd_start = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

unsigned long previousMillis = 0;

//Time between requets to the calendar server
//3 minute update is ok for me (1800000 seconds)
const long interval = 120000;

char LCD_message[128];
//char Last_LCD_message[128];
//char LCD_message[40];
int mIndex;

//Ethernet shields pinout
#define SS_SD_CARD   4
#define SS_ETHERNET 10

int MakeRequest;
int RequestSent;

//MAC address for the Ethernet interface
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xF2, 0xE9
};

//Default IP address, if DHCP fails to work
//IPAddress ip(192, 168, 1, 24);
IPAddress ip(10, 7, 5, 31);


//EthernetClient, to connect to the webserver (PHP page that gives the calendar event)
EthernetClient client;

//Auto reset - Number of seconds before auto-reset taks action
#define RESET_TIME 18000 // 5 horas

void setup() {

  //Ativa o Watchdog Timer (WDT)
  //Possibilidades: WDTO_8S , 4S, 21, 1S, 500MS
  wdt_enable(WDTO_8S);
  wdt_reset();

  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  //SPI.begin();      // Inicia  SPI bus

  //Shield configuration to use the Ethernet chip
  pinMode(SS_SD_CARD, OUTPUT);
  pinMode(SS_ETHERNET, OUTPUT);
  digitalWrite(SS_SD_CARD, HIGH);  // SD Card not active
  digitalWrite(SS_ETHERNET, LOW);  // Ethernet ACTIVE

  // Open serial communications and wait for port to open:
  Serial.begin(9600);

#ifdef DHCP
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
#endif
Ethernet.begin(mac, ip);

  //Timeout de 3 segundos para conexão
  W5100.setRetransmissionTime(0x07D0);
  W5100.setRetransmissionCount(3);
  wdt_reset();

  //server.begin();
  Serial.print("Current IP address: ");
  Serial.println(Ethernet.localIP());
  wdt_reset();

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Print a message to the LCD.
  lcd.print("    UFSCar   ");

  //Set LCD brightness
  //analogWrite (6, 200);

  //Initializes the LCD display and writes a welcome message
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("  Iniciando...       ");
  lcd.setCursor(0, 1);
  lcd.print("Agenda: R. Aroca            ");
  wdt_reset();

  //To request a web update when the system is started
  MakeRequest = 1;

  //Mensagem inicial do LCD
  //strcpy(LCD_message, "Aguardando \ndisponibilidade de conexao com o servidor...");
  strcpy(LCD_message, "                                                            ");

  wdt_reset();
  delay(800);
  wdt_reset();
  //lcd.clear();
}


void loop() {

  //Sofware Engineers will hate this, but...as Linus Torvalds said, you cant make a bootloader without gotos
loop_start:

  //====================================================
  //System watchdog reset and system auto-reset each 4 hours
  unsigned long currentMillisWdt = millis();
  unsigned long seconds = currentMillisWdt / 1000;
  if (seconds < RESET_TIME ) { //A cada 10 horas
    wdt_reset();
  }

  lcd_key = read_LCD_buttons();  // read the buttons


  //If button SELECT is pressed, updates the display immediately
  if (lcd_key == btnSELECT) {
    lcd.setCursor(0, 0);
    lcd.print(" Atualizando...         ");
    MakeRequest = 1;
    delay(300);
    wdt_reset();
  }

  if (lcd_key == btnUP) {
    auto_scroll=1;
    lcd.clear();
    lcd.print("Auto Scroll ON");
    delay(500);
  }
  
  if (lcd_key == btnDOWN) {
    auto_scroll=0;
    lcd.clear();
    lcd.print("Auto Scroll OFF");
    delay(500);
  }

/*
  lcd.setCursor(0, 1);
  lcd.print("                        ");
  lcd.setCursor(0, 0);
  lcd.print("                        ");
*/
  
  int lcd_right_scroll = LOW;
  int lcd_left_scroll = LOW;

  if (lcd_key == btnRIGHT)
    lcd_right_scroll = HIGH;

  if (lcd_key == btnLEFT)
    lcd_left_scroll = HIGH;  

  //Check if the elapsed interval has passed and if it is time to request the current event
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    MakeRequest = 1;
  }

  //Inicio da escrita no LCD com opção de deslocar para esquerda/direita / scroll
  if (HIGH == lcd_left_scroll)
  {
    if (lcd_start > 0)
    {
      auto_scroll=0;
      lcd_start--;

      delay(200);
    }
  }

  int text_len = strlen(LCD_message);
  int text_len2 = strlen(LCD_message);
  if (text_len >= 18) {
    text_len2 = text_len - 18;
  }

  if (auto_scroll) {
    lcd_limit = text_len2 - LCD_COLS;
    if (lcd_start < lcd_limit)
      lcd_start++;
    else {
      lcd_start=0;
      auto_scroll=0;
    }
    delay(200);
  }

  lcd_limit = text_len2 - LCD_COLS;
  // pushed button 2 -> try to scroll right or turn LED on if can't
  if (HIGH == lcd_right_scroll)
  {
    
    
    if (lcd_start < lcd_limit)
    {
      //Serial.println("Somando");
      auto_scroll=0;
      lcd_start++;
      delay(200);
    }
  }

  // -- print text on the LCD --

  int line = 0;
  int b;
  //Serial.print("Vamos tentar escrever: ");
  //Serial.println(LCD_message);
  //Serial.println(text_len);
  lcd.setCursor(0, 0);
  for ( b = 0; b <= text_len; b++) {
    //Changel LCD line if a new line is received
    if (LCD_message[b] == '\n') {
      //Serial.println("Pulando linha");
      lcd.setCursor(0, 1);
      line = 1;
    }
    else {
      if (line == 0) {
        if (LCD_message[b] > 0) {
          if (LCD_message[b] == '_')
            lcd.print(' ');
          else
            lcd.print(LCD_message[b]);
          wdt_reset();
        }
      }
      if (line == 1) {
        goto second_line;
      }
    }
  }

second_line:

  if (line == 1) {
    
    int ofk=0;
    if (text_len2 <=16)
      ofk=2;

    int i;
    for (i = 0; i < LCD_COLS-ofk; i++)
    {
      lcd.setCursor(i, 1);
      if (LCD_message[lcd_start + i + b] == '_')
        lcd.print(' ');
      else  
        if (LCD_message[lcd_start + i + b] > 0)
          lcd.print(LCD_message[lcd_start + i + b]);
    }
    
  }

/*
  Serial.print("Escrevendo");
  if (text_len2 < 16) {
    for (int o=18-text_len2; o<=18; o++) {
      Serial.print("Len: "); Serial.println(text_len2);
      Serial.print("Limit: "); Serial.println(lcd_limit);
      Serial.print("o: "); Serial.println(o);
  
      lcd.setCursor(o, 1);
      lcd.print(' ');
    }
  }
*/

  //Fim da escrita no LCD com opção de deslocar para esquerda/direita / scroll

  delay(200);
  wdt_reset();


  //We'll only continue if the interval is completed
  if (!MakeRequest)
    goto loop_start;

  MakeRequest = 0;

#ifdef DHCP
  //Not needed, but just for the sake of my paranoid - always restart the Ethernet interface and get an IP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);
  }
#endif
Ethernet.begin(mac, ip);
  //Timeout de 3 segundos para conexão
  W5100.setRetransmissionTime(0x07D0);
  W5100.setRetransmissionCount(3);

  wdt_reset();

  Serial.print("Current IP: ");
  Serial.println(Ethernet.localIP());

  Serial.print("Millis: ");
  Serial.println(millis());
  Serial.println("Connecting to webserver...");

  if (firstTime) {
    lcd.setCursor(0, 0);
    lcd.print("Conectando...          ");
    lcd.setCursor(0, 1);
    lcd.print(Ethernet.localIP());
    lcd.print("                 ");
    firstTime = 0;
  }

  wdt_reset();

  // if you get a connection, report back via serial:
  if (client.connect(serverHost, 80)) {
    Serial.println("Successfully connected to webserver. ");

    wdt_reset();

    //Specify here the URL for the PHP webpage that gives the calendar event summay
    client.println("GET /~aroca/ical/index.php HTTP/1.0");

    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();

    //Sets a control variable to let the system wait for the response
    RequestSent = 1;
  }
  else {
    //If connection was not successfull
    Serial.println("Web connection failed. ");

    /*
      lcd.setCursor(0, 0);
      lcd.print("      Erro                 ");
      lcd.setCursor(0, 1);
      lcd.print("   Sem conexao        ");
    */
    
    //for (int q = 0; q <= 127; q++)
    //        LCD_message[q] = ' ';
    strcpy(LCD_message, "______Erro______\n__Sem conexao");
    
    wdt_reset();


  }

  if (RequestSent) {

    //"State machine" to wait for the response to be written on the LCD
    //We expect the sequence |:,. before the text that will be written on the display
    fsm_reception = 0;


    //Wait for the response, byte per byte
    //TODO: add a timeout verification
    while (RequestSent) {

      // if there are incoming bytes available
      if (client.available()) {
        char c = client.read();

        //For debuuging only - show each received byte from the server
        //Serial.print(c);

        //Expecting |:,.
        if (c == '|') {
          fsm_reception = 1;
        }

        if ((c == ':') && (fsm_reception == 1)) {
          fsm_reception = 2;
        }


        if ((c == ',') && (fsm_reception == 2)) {
          fsm_reception = 3;
        }

        if (fsm_reception == 4) {

          if (c == ']') {

            //We'll enter here when a complete message is received
            wdt_reset();

            LCD_message[mIndex] = 0;

            Serial.println("== Message received == ");


            int b;
            //Print char by char on the LCD display (just to jump to the new line when needed)

            //This will write the message to the LCD, but we moved to the start of the loop to scroll the LCD
#ifdef NO_SCROLL
            lcd.setCursor(0, 0);
            for (b = 0; b <= mIndex; b++) {
              //Changel LCD line if a new line is received
              if (LCD_message[b] == '\n') {
                lcd.setCursor(0, 1);
                OFFSET = mIndex;
              }
              else {
                if (LCD_message[b] > 0)
                  //lcd.print(LCD_message[b]);
                  wdt_reset();
              }
            }
#endif

            //If the message received is less than 18 chars, them there is no event - only date and time
            //So put a message on the display saying that we have nothing to do!
            if (mIndex <= 18) {
              //lcd.setCursor(0, 1);
              //lcd.print ("  Agenda livre  ");
              //for (int q = 0; q <= 127; q++)
              //LCD_message[q] = ' ';
              strcat(LCD_message, "________________");
              wdt_reset();
            }
            //lcd.print("                     ");
            Serial.println(LCD_message);

            Serial.println("== End of received message ==");

            //Reset the state machine to receive again
            fsm_reception = 0;

            lcd_start = 0;
            auto_scroll = 1;
          }
          else {

            //The message is not still completely received
            //We enter here for each received byte

            LCD_message[mIndex] = c;
            mIndex++;

            wdt_reset();

            //Limited to the number the display size
            //Mine is 16x2 = 32, but some lets let some additional chars for control, such as \n
            if (mIndex >= 127) //era 35
              mIndex = 127; //era 35
          }
        }

        //Laste state of the FSM, before actually starting to receive bytes
        if ((c == '.') && (fsm_reception == 3)) {
          fsm_reception = 4;
          mIndex = 0;
          for (int q = 0; q <= 127; q++)
            LCD_message[q] = ' ';
        }

      } // End of  if (client.available())

      // if the server's disconnected, stop the client:
      if (!client.connected()) {
        Serial.println();
        Serial.println("Disconnecting from server. ");
        client.stop();
        RequestSent = 0;
        wdt_reset();
      }
    }
  } //RequestSent end

}

//Helpder function for the LCD display
int read_LCD_buttons()
{
  adc_key_in = analogRead(0);      // read the value from the button

  // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // we add approx 50 to those values and check to see if we are close
  if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result

  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 195)  return btnUP;
  if (adc_key_in < 380)  return btnDOWN;
  if (adc_key_in < 555)  return btnLEFT;
  if (adc_key_in < 790)  return btnSELECT;

  return btnNONE;
}

