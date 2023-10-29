/*
   Автор: Петров Александр Алексеевич
   Дата: 29 Октября 2023

   Для измерения был выбран датчик CO2 AGS10, но он оказадся не исправным.
   Код написан для абстрактного сенсора, в роле которого выступил потенциометр.

   Тестировалось на отладочной плате WEMOS LOLIN32 Lite.
*/

#define RED_LED_PIN 19      // Пин красного светодиода
#define YELLOW_LED_PIN 23   // Пин жёлтого светодиода
#define GREEN_LED_PIN 18    // Пин зелёного светодиода

#define BUZZER_PIN 2        // Пин пищалки 
// Для включение замкнуть на массу, предпологается использование писчалки со встроенным контроллером

#define SDA_PIN 17          // Пин данных I2C
#define SCL_PIN 16          // Пин тактирования I2C

#define SENSOR_PIN 15       // Пин аналогового датчика

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Перечисление состояний устройства
typedef enum {
  Error,
  Ok,
  Control,
  Critical
} State;

// порог некритического превышения уровня газа
uint16_t medium_threshold = 100;

// порог критического превышения уровня газа
uint16_t high_threshold = 200;

void setup() {
  // Установка пинов в качестве выходов
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  analogReadResolution(12);

  // Инициализация I2C на пинах
  Wire.begin(SDA_PIN, SCL_PIN);

  // Инициализация дисплея
  lcd.init();
  lcd.backlight();
}

void loop() {
  State state = updateState();
  setLED(state);
  updateLCD(state);
  buzzerHandler(state);
  delay(1);
}


/*
   Функция обновления состояния.
   Принимает: ничего
   Возвращает: состояние
   Считывает значение, фильтрует, выводит в порт, определяет состояние
*/
State updateState() {
  uint16_t val = 0;
  uint16_t now_val = analogRead(SENSOR_PIN);

  // фильтр низких частот
  val += (now_val - val) / 10;

  if (val < medium_threshold) {
    return Ok;
  }
  if (val < high_threshold) {
    return Control;
  }
  return Critical;
}


/*
   Функция обновления дисплея.
   Принимает: состояние
   Возвращает: ничего
   Выводит на дисплей новое состояние.
*/
void updateLCD(State state) {
  static State old_state;
  if (state != old_state) {
    lcd.clear();
    switch (state) {
      case Ok:
        lcd.setCursor(7, 0);
        lcd.print("OK");
        break;
      case Control:
        lcd.setCursor(5, 0);
        lcd.print("CONTROL");
        break;
      case Critical:
        lcd.setCursor(5, 0);
        lcd.print("CRITICAL");
        break;
    }
    old_state = state;
  }
}


/*
   Функция установки светодиода.
   Принимает: состояние
   Возвращает: ничего
   Зажигает соответствующий состоянию светодиод.
*/
void setLED(State state) {
  digitalWrite(GREEN_LED_PIN, state == Ok);
  digitalWrite(YELLOW_LED_PIN, state == Control);
  digitalWrite(RED_LED_PIN, state == Critical);
}

/*
   Функция писка.
   Принимает: состояние
   Возвращает: ничего
   Писщит соответственно режиму.
*/
void buzzerHandler(State state) {
  uint32_t time = millis();
  switch (state) {
    case Ok:
      // выкл
      digitalWrite(BUZZER_PIN, HIGH);
      break;
    case Control:
      // 2Гц 10%
      time %= 500;
      digitalWrite(BUZZER_PIN, time > 50);
      break;
    case Critical:
      // 10Гц 50%
      time %= 100;
      digitalWrite(BUZZER_PIN, time > 50);
      break;
  }
}
