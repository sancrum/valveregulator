/*Программа управления газовым регулятора Honeywell Atmix VK47/vk##
* Участвуют Энкодер, экран 1602, шаговый двигатель, твердотельное реле
* 
* Версия 1.2 bis
* цифровой преобразователь для термопар заменен с max6675 на max31855
*                                                                                                                                                                                                                                                                                                                  
* Версия 1.1                                                                                                                                                                                                                                                                                                                 
* добавлена версионность
* добавлен коэффициент мощности 0,85 (100% на энкодере эквивалентно 85% открытия (92*0,85=72 шага максимум)
*/
byte subversion = 2; //1.1
  
#include <ModbusRtu.h>
#include <SPI.h>
#include <Rotary.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MAX31855.h>

////////////////////////////////////////////////настройка LCD
LiquidCrystal_I2C lcd(0x3f, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

////////////////////////////////////////////////настройка энкодера
Rotary r = Rotary(7, 6);
int encoderPin = 8;

/////////////////////////////////////////////////настройка Modbus-slave
#define ID 1
Modbus slave(ID, 0, 0); // this is slave ID and RS-232 or USB-FTDI
// data array for modbus network sharing
uint16_t au16data[10];
int8_t state = 0;    //
int setPower=0;

//////////////////////////////////////////////////настройка электродвигателя
//драйвер шагового двигателя
#define IN1 2
#define IN2 3
#define IN3 4
#define IN4 5

int Steps = 4;   //начальное положение stepper до инициализации
int steps_left;
boolean Direction = true;
boolean stepperReady;   //флаг законченности шага
boolean stepperCCWstop; //флаг корректировки закрывающих шагов
boolean stepperIdle; //флаг перехода в простой
unsigned long last_time;
unsigned long currentMillis ;

int currentPosition=96; //ВАЖНО! ПОЗИЦИЯ ДЛЯ ИНИЦИАЛИЗАЦИИ РАВНО 96!!!!!!
int stepsPerRevolution = 92;
int setPosition;

//////////////////////////////////////////////////настройка SPI MAX6675
////2 модуля max6675 temperature sensor
//int thermoSO = 12;  // MISO
//int thermoCLK = 13; // SCK
//int tc1CS = 10;   //SS
//int tc2CS = 9;   //SS2
MAX31855 tc1(10); ////////////////////////////////настройка SPI max31855
MAX31855 tc2(9); 


//Замер проб температуры
uint16_t currentTemp;
unsigned long probedelay =0;
unsigned long tempus =0;   //

////minimal max6675.h inside this project
//double readCelsius(uint8_t cs){
//    uint16_t v;
//    digitalWrite(cs, LOW);
//    v = SPI.transfer(0x00);
//    v <<= 8;
//    v |= SPI.transfer(0x00);
//    digitalWrite(cs, HIGH);
//    if (v & 0x4) { return NAN;} // uh oh, no thermocouple attached!
//    v >>= 3;
//    return v*0.25;
//}

////////////////// настройка управления твердотельным реле для второго клапана крана
int relayPin=14;

void setup() {
  slave.begin(19200);
  SPI.begin();
//I2C LCD init
  lcd.begin(16,2);
  lcd.setCursor(0,0); 
  lcd.println("NevaCoffee");
  lcd.print("1.");lcd.print(subversion);
 tc1.begin(); // Set thermocouple
 tc2.begin(); 
  while ((tc1.getChipID() != MAX31855_ID)and(tc2.getChipID() != MAX31855_ID))  {
    Serial.println(F("MAX31855 error")); //(F()) saves string to flash & keeps dynamic memory free
    delay(5000);
  }
  lcd.setCursor(0,0); 
  lcd.print("powered by");
  lcd.setCursor(0,1); 
  lcd.print("vladkomarov.tk");
  delay(2000);
  lcd.setCursor(0,0);
  lcd.print("ET=   C, ");
  lcd.setCursor(9,0); 
  lcd.print("BT=   C");
  lcd.setCursor(0,1); 
  lcd.print("Power    %    ");

//set Step motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  setPosition = 0;
  Steps = 7;
// set Relay valve open
  pinMode(relayPin, OUTPUT);
  digitalWrite (relayPin, HIGH);
// set encoder button
  pinMode(encoderPin, INPUT_PULLUP);


//set max6675 slaves   
//  pinMode(tc1CS, OUTPUT);
 // pinMode(tc2CS, OUTPUT);
//  digitalWrite(tc1CS, HIGH);
//  digitalWrite(tc2CS, HIGH);

  tempus = millis() + 100;
  probedelay = millis()+3000;
}

void loop() {
//poll MODBUS
 state = slave.poll( au16data, 11 );
/* 3 - tc1; 4 tc2
 * 6 - power percentage with 0.85 multiplier
 * 
 * 
 */ 

 
  unsigned char result = r.process();
  if (result == DIR_CW) {
    au16data[6] = au16data[6] + 500; 
    if (au16data[6] > 10000) {au16data[6]=10000;}
  }
  if ((result == DIR_CCW ) && (au16data[6] > 1)) {
    au16data[6] = au16data[6] - 500;
   // if (au16data[6] < 600) {au16data[6] = 0;}
  }

//get temperature every probeDelay  
 if (millis() > probedelay) {  
  au16data[3]=tc1.getTemperature()*100;
  au16data[4]=tc2.getTemperature()*100;
  probedelay = millis() + 1000;
   
  lcd.setCursor(3,0);  lcd.print("   "); lcd.setCursor(3,0);  lcd.print(au16data[3]/100);
  lcd.setCursor(12,0); lcd.print("   "); lcd.setCursor(12,0); lcd.print(au16data[4]/100);
 }

 setPower = au16data[6]*100*85/100; 
 setPower = (stepsPerRevolution)*setPower;
 setPower = setPower /100;
 if (setPower != currentPosition) { 
  setMotor(setPower); 
 // Serial.print(setPower);Serial.print("   ");Serial.println(au16data[6]);
 }
}

void setMotor( int setPosition)
{
  lcd.setCursor(6,1); lcd.print("   "); lcd.setCursor(6,1); lcd.print(au16data[6]/100);
 
  currentMillis = millis();
  if (setPosition > stepsPerRevolution) {setPosition = stepsPerRevolution;}
  if (setPosition < 0) {setPosition = 0;}
  if (setPosition != currentPosition){
    stepperIdle = false;
    if (currentPosition > setPosition) {
      Direction=false;
      steps_left=abs(setPosition-currentPosition);
    }
    else { 
      Direction=true;
      steps_left=abs(setPosition-currentPosition);
    }
    
    if (stepperReady) {
      Serial.print(",");
      stepper();
      stepperReady = false;
      steps_left--;
      if (Direction) currentPosition++;
      else currentPosition--;

    }
    if ((steps_left == 0) && (!Direction) && (!stepperCCWstop)) { 
        Serial.print("><");
        setPosition++;
        stepperCCWstop = true;
    }
//Mark step done moment
   if(currentMillis-last_time>=70){
      stepperReady = true;
  
      last_time=millis();
    }
  }
//Stay idle
  else if (!stepperIdle && (currentMillis-last_time>=125)){
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW); 
    stepperIdle = true;

  }
}

void stepper(){
    SetDirection();
    switch(Steps){
    case 0:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 1:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 2:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    case 3:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, HIGH);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case 4:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      break;
    case 5:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, HIGH);
      break;
    case 6:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    case 7:
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, HIGH);
      break;
    default:
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, LOW);
      digitalWrite(IN4, LOW);
      break;
    }

}

void SetDirection(){
  if(Direction==1){ Steps++;}
  if(Direction==0){ Steps--; }
  if(Steps>7){Steps=0;}
  if(Steps<0){Steps=7; }
}
