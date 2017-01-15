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
byte battery_pin = 6;    // сюда подключен аккумулятор для измерения напряжения
boolean RXstate;

byte receiv_data[2];

int fuse_time = 1500;  // время в миллисекундах, которое ток будет подаваться  на спираль
int battery_check = 2800; // нижняя граница заряда аккумулятора для защиты, в милливольтах!

byte crypt_key = 123;    // уникальный ключ для защиты связи
int check = 111;         // условный код, для обратной связи

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
  byte pipeNo;
  while ( radio.available(&pipeNo)) {             // слушаем эфир со всех труб
    radio.read( &receiv_data, sizeof(receiv_data) );      // читаем входящий сигнал

    if (receiv_data[0] == crypt_key) {

      if (receiv_data[1] == check) {
        int voltage = analogRead(battery_pin) * readVcc() / 1024;
        if (voltage > battery_check) RXstate = 1; else RXstate = 0;

        radio.writeAckPayload(pipeNo, &RXstate, sizeof(RXstate) );
        Serial.println("Check state sent");
      } else {
        if (FLAGS[receiv_data[1]] == 0) {
          FLAGS[receiv_data[1]] = 1;                           // поднять флаг для мосфета, по входящему сигналу
          TIMES[receiv_data[1]] = millis();                    // запомнить время прихода сигнала
          digitalWrite(MOSFET[receiv_data[1]], HIGH);          // подать питание на мосфет (на запал)

          Serial.print("Fuse #"); Serial.print(receiv_data[1]); Serial.println(" ON");
        }
      }
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

long readVcc() { //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин)
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result; // расчёт реального VCC
  return result; // возвращает VCC
}
