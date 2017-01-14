/*
  Created 2017
  by AlexGyver
  AlexGyver Home Labs Inc.
*/
#include "Keypad.h"       //библиотека клавиатуры
#include <SPI.h>          // библиотека для работы с шиной SPI
#include "nRF24L01.h"     // библиотека радиомодуля
#include "RF24.h"         // ещё библиотека радиомодуля

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно
byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

byte redLED = 14;
byte greenLED = 15;

int check = 111;
boolean check_answer;

const byte ROWS = 4; //4 строки у клавиатуры
const byte COLS = 3; //три столбца
char keys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; //Подключены строки (4 пина)
byte colPins[COLS] = {4, 3, 2}; //подключены столбцы (3 пина)
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); //создать клавиатуру
boolean flag;

void setup() {
  Serial.begin(9600);

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED, HIGH);    // зажечь зелёный светодиод

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openWritingPipe(address[0]);   //мы - труба 0, открываем канал для передачи данных
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.stopListening();  //не слушаем радиоэфир, мы передатчик
}

void loop() {
  byte gotByte;
  char key = keypad.waitForKey();      // ожидать ввод с клавиатуры, принять символ
  int keyInt = key - '0';              // перевести символ в целочисленный тип

  if (keyInt >= 0 && keyInt <= 9 && flag == 1) {  // если число от 0 до 9 и флаг поднят
    digitalWrite(greenLED, HIGH);                         // включить зелёный светодиод
    digitalWrite(redLED, LOW);                            // погасить красный светодиод
    radio.write(&keyInt, sizeof(keyInt));                 // отправить цифру с клавиатуры по радио
    delay(10);
    digitalWrite(greenLED, LOW);                          // погасить зелёный светодиод
    digitalWrite(redLED, HIGH);                           // зажечь красный светодиод
        
  } else if (key == '*') {                                // если символ *
    flag = 0;                                             // опустить флаг
    
    digitalWrite(redLED, LOW);                            // погасить красный светодиод
    digitalWrite(greenLED, LOW);                          // погасить зелёный светодиод
    delay(100);                                           // задержка для эпичности
    
    if ( radio.write(&check, sizeof(check)) ) {           // отправляем значение 111
      if (!radio.available()) {                           // если не получаем ответ
        Serial.println("No answer");
        digitalWrite(greenLED, HIGH);                     // включить зелёный светодиод
      } else {
        while (radio.available() ) {                      // если в ответе что-то есть
          radio.read( &check_answer, 1);                  // читаем
          if (check_answer == 1) {                        // если статус = 1 (готов к работе)
            flag = 1;                                     // поднять флаг готовности к работе
            digitalWrite(redLED, HIGH);                   // зажечь красный светодиод
            Serial.println("Status OK");
          } else {                                        // если статус = 0 (акум разряжен)
            Serial.println("Status BAD");
            for (int i = 0; i < 5; i++) {                 // моргнуть 6 раз зелёным светодиодом
              digitalWrite(greenLED, HIGH);
              delay(200);
              digitalWrite(greenLED, LOW);
              delay(100);
            }
            digitalWrite(greenLED, HIGH);                 // включить зелёный светодиод при ошибке
          }
        }
      }
    } else {
      Serial.println("Sending failed");              // ошибка отправки
      digitalWrite(greenLED, HIGH);                  // включить зелёный светодиод
    }

  } else if (key == '#') {                        // если символ №
    flag = 0;                                     // опустить флаг
    digitalWrite(greenLED, HIGH);                 // зажечь зелёный светодиод
    digitalWrite(redLED, LOW);                    // погасить красный светодиод
  }
}

