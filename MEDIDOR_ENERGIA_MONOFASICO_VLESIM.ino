/*
                    VLESIM - VIRTUAL LABORATORIES ELECTRONICS SIMULATOR
                             Sensor de Potencia Monofásico UART1
El dispositivo costa de un sensor de potencia monofasico, una tarjeta de desarrollo ESP32, una pantalla
LCD 16x2 y un modulo I2C para la pantalla. Este dispositivo es capaz de medir hasta 100A. Las medidas 
que se obtienen son: Voltaje, Corriente, potencia, energia,frecuencia y factor de potencia. 

Los valores de energia se miden en Kw/h y el valor se guarda en la EPROM del ESP32, para reiniciar los valores 
se descomenta la instruccion  // pzem.resetEnergy().

PZEM-004T : Sensor de potencia hasta 100A
Pantalla LCD16x2 : Display LCD
Modulo I2C : modulo cominicacion I2C entre el ESP32 y la pantalla LCD16x2 (Direccion: 0x3F)

*/

// Librerias
#include "Vlesim.h"
#include "EnvironmentSensors.h"

// PANTALLA OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Comunicación con vlesim
Scheduler scheduler;
DateTime datetime;
Storage storage;
DateTime currentDate;

char apiToken[] = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJ0b29sIjoiNjIyNTNiNmEyNTUyYjY1MGU3Y2E5NmEyIiwiaWF0IjoxNjQ2NjA3MjEwfQ.dPyZ01bhyF0TgJUZvTZR8z3gFgi-8J4eukoJbLxJZPW_xo0gm8AbHmUGj7-BlBfF1_Z2pJcOZxRl75UBPZ7I6DbNMZdwhBl-9oloXhf5D5LZUXIWyHgWRUY-XT0S0aNozCf6bi_CikQsy5u2kR_Nlca5taK40o_hrCQqYzgek9yx7eaxbiNx5lVpx8ENV76JZ_1MCN9H8l-6KXyrUH8H1vq-eLF_Ogn9Dz7_Q7-delFADriP3Eqcmm-gLvJTqmI9OGUuejmckpEOVDtXQtpiRpmwuokKH8VmnDfTqo-z5sgH4JfE1mg3n03lkkmhUkrOk6AUmnOSPr-ulozOL52nxw";
Vlesim vlesim("UNE 4G LTE-8739", "IDENAR2019", apiToken);
//Vlesim vlesim("FLIA-VARGAS", "FamiliaVargas1086019199", apiToken);

// variables medidas
float voltage = 0;
float current = 0;
float power = 0;
float energy = 0;
float frequency = 0;
float pf = 0;
unsigned long timestamp = 0;
double values[5] = {0, 0, 0, 0, 0}; 

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


#include <PZEM004Tv30.h>

//#define I2C_SDA 18          
//#define I2C_SCL 19

#define PZEM_RX_PIN 16         
#define PZEM_TX_PIN 17

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 23
#define PZEM_TX_PIN 22
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif


#if defined(ESP32)
/*************************
 *  ESP32 initialization
 * ---------------------
 * 
 * The ESP32 HW Serial interface can be routed to any GPIO pin 
 * Here we initialize the PZEM on Serial2 with RX/TX pins 16 and 17
 */
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);
#elif defined(ESP8266)
/*************************
 *  ESP8266 initialization
 * ---------------------
 * 
 * Not all Arduino boards come with multiple HW Serial ports.
 * Serial2 is for example available on the Arduino MEGA 2560 but not Arduino Uno!
 * The ESP32 HW Serial interface can be routed to any GPIO pin 
 * Here we initialize the PZEM on Serial2 with default pins
 */
//PZEM004Tv30 pzem(Serial1);
#else
/*************************
 *  Arduino initialization
 * ---------------------
 * 
 * Not all Arduino boards come with multiple HW Serial ports.
 * Serial2 is for example available on the Arduino MEGA 2560 but not Arduino Uno!
 * The ESP32 HW Serial interface can be routed to any GPIO pin 
 * Here we initialize the PZEM on Serial2 with default pins
 */
PZEM004Tv30 pzem(PZEM_SERIAL);
#endif

// PANTALLA OLED
#define ANCHO_PANTALLA 128 // Ancho de la pantalla OLED
#define ALTO_PANTALLA 64 // Alto de la pantalla OLED

#define OLED_RESET     -1 // Pin reset incluido en algunos modelos de pantallas (-1 si no disponemos de pulsador). 
#define DIRECCION_PANTALLA 0x3C //Dirección de comunicacion: 0x3D para 128x64, 0x3C para 128x32

Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, OLED_RESET);

#define LOGO_WIDTH    70
#define LOGO_HEIGHT   46    

const unsigned char PROGMEM logo[] = {
// 'logo vlesimbn@4x', 70x46px
0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xfc, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x07, 0xf0, 0x00, 
0xff, 0xff, 0x80, 0x00, 0x00, 0x00, 0x0f, 0xf8, 0x01, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xbf, 
0xfc, 0x03, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x3e, 0x07, 0xff, 0xfc, 0x00, 0x00, 0x00, 
0x00, 0x3e, 0x3f, 0x0f, 0xff, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x3f, 0x1f, 0xff, 0xf0, 0x00, 
0x00, 0x00, 0x00, 0x0f, 0xfe, 0x3f, 0xff, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfc, 0x7f, 0xff, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xf8, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 
0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x7f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xf8, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x1f, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0xe0, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 
0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x38, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x66, 0x0f, 0xc7, 
0x8c, 0x7f, 0xc0, 0x00, 0x00, 0x0c, 0x66, 0x0c, 0xcc, 0xcc, 0xce, 0xc0, 0x00, 0x00, 0x04, 0xc6, 
0x0c, 0x4c, 0xcc, 0xcc, 0xc0, 0x00, 0x00, 0x06, 0xc6, 0x0c, 0x0e, 0x0c, 0xcc, 0xc0, 0x00, 0x00, 
0x06, 0xc6, 0x0f, 0x07, 0x0c, 0xcc, 0xc0, 0x00, 0x00, 0x06, 0xc6, 0x0f, 0x03, 0x8c, 0xcc, 0xc0, 
0x00, 0x00, 0x07, 0xc6, 0x0c, 0x01, 0xcc, 0xcc, 0xc0, 0x00, 0x00, 0x03, 0x86, 0x0c, 0x00, 0xcc, 
0xcc, 0xc0, 0x00, 0x00, 0x03, 0x86, 0x0c, 0xcc, 0xcc, 0xcc, 0xc0, 0x00, 0x00, 0x03, 0x87, 0xcf, 
0xcf, 0xcc, 0xcc, 0xc0, 0x00, 0x00, 0x01, 0x00, 0xc3, 0x83, 0x8c, 0x0c, 0x00, 0x00
};


#define ButtonPin 13 // BOTON para cambiar el modod de vizualizacion medidas/maxMinAvg

//#define btn1 13
//#define btn2 26
//#define btn3 33
//#define btn4 32

#define DELAY_BTN 500   // 500ms

int band =0;
int bEncendido = 0;
int btnPress = 0;
long lastTime = 0;

void ISR(){
  if (!digitalRead(ButtonPin)) btnPress = 1;
  //else if (!digitalRead(btn2)) btnPress = 2;
  //else if (!digitalRead(btn3)) btnPress = 3;
  //else if (!digitalRead(btn4)) btnPress = 4;
  else btnPress = 0;
}

void setup() {
  //Wire.begin(I2C_SDA,I2C_SCL);
  // Debugging Serial port
  Serial.begin(115200);
  // Initialize the storage
  storage.begin();
  // END para terminar la lista de argumentos
  storage.setHeader("/data.csv", "timestamp", "voltage", "current","power","energy","frequency", "FP", END); 
  // Comunicación con el server
  vlesim.begin();
  //vlesim.setCallback(callback);
  vlesim.setClient("62253b6a2552b650e7ca96a2");
  vlesim.setCategory("ElectricMeterPZEM");
  scheduler.every(5).seconds().doJob(task1);
  scheduler.every(120).seconds().doJob(storageTask);
  
   // interrupcion ISR
  pinMode(ButtonPin, INPUT_PULLUP);       // INPUT_PULLDOWN/INPUT_PULLUP
  attachInterrupt(ButtonPin, ISR, FALLING);  // LOW/HIGH/FALLING/RISING/CHANGE
  


  if(!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_PANTALLA)) {   
    Serial.println(F("Fallo en la asignacion de SSD1306"));
  }
  
  // ENCABEZADO
  display.clearDisplay(); 
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(" MEDICION DE ENERGIA");
  display.setTextSize(1);
  display.setCursor(0,9);
  display.println("     MONOFASICO ");
  // LINEA
  display.drawLine(0, 17, 128, 17, WHITE);
  // LOGO
  display.drawBitmap( (display.width() - LOGO_WIDTH ) / 2,((display.height()- LOGO_HEIGHT) / 2 )+10, logo, LOGO_WIDTH, LOGO_HEIGHT, WHITE);
  // LINEA    
  //display.drawLine(0, 48, 128, 48, WHITE);
  display.display();
  
  delay(5000);   
}

void loop() {
  // RUn VLESIM
  scheduler.runPending();
  vlesim.runLoop();
  
  //---------------INTERRUPCIONES-------------------------
  if (btnPress)
  {
    if (millis()-lastTime > DELAY_BTN)
    {
      Serial.print("¡Interrupción ");
      Serial.print(btnPress);
      Serial.println("!");
      lastTime = millis();

     // Activacion Alarma
      if(btnPress == 1){
        bEncendido++;
        if(bEncendido>=2){
          bEncendido =0;
          }
        }
    }
    btnPress = 0;
  }
//----------------------------------------------------
  //Serial.println(frequency);
  delay(200);
}

void task1() {
  
// Print the custom address of the PZEM
  //Serial.print("Custom Address:");
  //Serial.println(pzem.readAddress(), HEX);
  Serial.println("======================================================");
  // Read the data from the sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();

  // Check if the data is valid
  if(isnan(voltage)){

    switch(bEncendido){
      case 0:
        Serial.println("Error reading voltage");
        // OLED 128 x 64
        // ENCABEZADO
        display.clearDisplay(); 
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.println("SISTEMA DE GESTION DE");
        display.setCursor(0,9);
        display.println(" ENERGIA-BIFASICO");
        
        display.drawLine(0, 17, 128, 17, WHITE);
    
        display.setCursor(0,24);
        display.println("Error de lectura");
        
        display.display();
        band = 0;
        break;
        
      case 1:
        if(band == 0){
          Serial.println("Apagar Oled");
          display.clearDisplay();
          display.display();
          band=1;
          delay(100);
        }
        break;
      
      }   
    
      
  } else if (isnan(current)) {
      Serial.println("Error reading current");
  } else if (isnan(power)) {
      Serial.println("Error reading power");
  } else if (isnan(energy)) {
      Serial.println("Error reading energy");
  } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
  } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
  } else {

    switch(bEncendido){
      case 0:
        Serial.println("Mostrar OLED");
        // Print the values to the Serial console
        Serial.print("Voltage: ");      Serial.print(voltage);      Serial.println("V");
        Serial.print("Current: ");      Serial.print(current);      Serial.println("A");
        Serial.print("Power: ");        Serial.print(power);        Serial.println("W");
        Serial.print("Energy: ");       Serial.print(energy,3);     Serial.println("kWh");
        Serial.print("Frequency: ");    Serial.print(frequency, 1); Serial.println("Hz");
        Serial.print("PF: ");           Serial.println(pf);

        Serial.println("======================================================");

        // Print the values to display OLED 128 x 64
        display.clearDisplay();
        // ENCABEZADO
        display.clearDisplay(); 
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0,0);
        display.println("SISTEMA DE GESTION DE");
        display.setCursor(0,9);
        display.println(" ENERGIA-MONOFASICO");
        // Voltaje 
        display.setCursor(0,17);
        display.println("Voltaje   = ");
        display.setCursor(70,17);
        display.println(voltage,1);
        display.setCursor(100,17);
        display.println("V");
        // Corriente
        display.setCursor(0,25);
        display.println("Corriente = ");
        display.setCursor(70,25);
        display.println(current);
        display.setCursor(100,25);
        display.println("A");

        // Potencia
        display.setCursor(0,33);
        display.println("Potencia  = ");
        display.setCursor(70,33);
        display.println(power);
        display.setCursor(100,33);
        display.println("W");
        

        // Energia
        display.setCursor(0,41);
        display.println("Energia   = ");
        display.setCursor(70,41);
        display.println(energy,3);
        display.setCursor(100,41);
        display.println("kWh");

        // Frecuencia
        display.setCursor(0,49);
        display.println("Frecuencia= ");
        display.setCursor(70,49);
        display.println(frequency,1);
        display.setCursor(100,49);
        display.println("Hz");

        // Factor de potencia
        display.setCursor(0,57);
        display.println("Fp        = ");
        display.setCursor(70,57);
        display.println(pf);
        display.display();
        
        band =0;
        delay(2000);
        break;

      case 1:
        if(band == 0){
          Serial.println("Apagar Oled");
          display.clearDisplay();
          display.display();
          band=1;
          delay(100);
        }
        break;
      }

       // ENVIAR A LA PLATAFORMA DE VLESIM PROTOCOLO MQTT
        currentDate = datetime.now();
        vlesim.setDate(currentDate.timestamp());  
        vlesim.add("voltage", voltage);
        vlesim.add("current", current);
        vlesim.add("power", power);
        vlesim.add("energy", energy);
        vlesim.add("frequency", frequency);
        vlesim.add("FP", pf);
        Serial.print("PublishStatus = ");
        Serial.println(vlesim.vlesimPublish());
        Serial.println("======================================================");
        vlesim.vlesimPublish();
      
  }  
  
  
  }

void storageTask(){
  // END para terminar la lista de argumentos
  values[0]=voltage;
  values[1]=current;
  values[2]=power;
  values[3]=energy;
  values[4]= frequency;
  values[5]= pf;
  storage.save("/data.csv", currentDate.format(), values, 6);
  }
