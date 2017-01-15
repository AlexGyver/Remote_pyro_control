/*   Данный скетч делает следующее: передатчик (TX) отправляет массив
     данных, который генерируется согласно показаниям с кнопки и с
     двух потенциомтеров. Приёмник (RX) получает массив, и записывает
     данные на реле, сервомашинку и генерирует ШИМ сигнал на транзистор.
    by AlexGyver 2016
*/

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

float my_vcc_const = 1.1; // начальное значение константы должно быть 1.1

byte MOSFET[10] = {18, 4, 5, 6, 7, 8, 14, 15, 16, 17};  //массив пинов, к которым подключены мосфеты
boolean FLAGS[10]; // массив, хранящий время для таймера каждого мосфета? по умолчанию {0,0,0,0,0,0,0,0}
unsigned long TIMES[10]; // массив, хранящий состояния мосфетов

int fuse_time = 1500;  // время в миллисекундах, которое ток будет подаваться  на спираль

RF24 radio(9, 10); // "создать" модуль на пинах 9 и 10 Для Уно
//RF24 radio(9,53); // для Меги

byte address[][6] = {"1Node", "2Node", "3Node", "4Node", "5Node", "6Node"}; //возможные номера труб

void setup() {
  Serial.begin(9600); //открываем порт для связи с ПК

  // настраиваем пины мосфетов как выходы по массиву
  for (int i = 0; i <= 9; i++) {
    pinMode(MOSFET[i], OUTPUT);
  }

  radio.begin(); //активировать модуль
  radio.setAutoAck(1);         //режим подтверждения приёма, 1 вкл 0 выкл
  radio.setRetries(0, 15);    //(время между попыткой достучаться, число попыток)
  radio.enableAckPayload();    //разрешить отсылку данных в ответ на входящий сигнал
  radio.setPayloadSize(32);     //размер пакета, в байтах

  radio.openReadingPipe(1, address[0]);     //хотим слушать трубу 0
  radio.setChannel(0x60);  //выбираем канал (в котором нет шумов!)

  radio.setPALevel (RF24_PA_MAX); //уровень мощности передатчика. На выбор RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX
  radio.setDataRate (RF24_1MBPS); //скорость обмена. На выбор RF24_2MBPS, RF24_1MBPS, RF24_250KBPS
  //должна быть одинакова на приёмнике и передатчике!
  //при самой низкой скорости имеем самую высокую чувствительность и дальность!!

  radio.powerUp(); //начать работу
  radio.startListening();  //начинаем слушать эфир, мы приёмный модуль
}

void loop() {
  byte pipeNo, in_data;
  while ( radio.available(&pipeNo)) {             // слушаем эфир со всех труб
    radio.read( &in_data, sizeof(in_data) );      // читаем входящий сигнал

    if (FLAGS[in_data] == 0) {
      FLAGS[in_data] = 1;                           // поднять флаг для мосфета, по входящему сигналу
      TIMES[in_data] = millis();                    // запомнить время прихода сигнала
      digitalWrite(MOSFET[in_data], HIGH);          // подать питание на мосфет (на запал)

      Serial.print("Fuse #"); Serial.print(in_data); Serial.println(" ON");
    }
  }

  for (int i = 0; i <= 9; i++) {                  // пройтись по всем 10ти мосфетам
    if (millis() - TIMES[i] > fuse_time && FLAGS[i] == 1) {  // если время с момента открытия мосфета > заданного
      FLAGS[i] = 0;                                                 // опустить флаг
      digitalWrite(MOSFET[i], LOW);                                 // закрыть мосфет, погасить запал
      Serial.print("Fuse #"); Serial.print(i); Serial.println(" OFF");
    }
  }
}
