# Реализация распределённой IoT-системы "LedPhotoSerialMQTT"

## 1. Введение и постановка задачи

### 1.1. Цель проекта

Целью данного проекта является разработка и демонстрация полноценной распределённой IoT-системы. Система моделирует классический сценарий "умного дома" или автоматизированной теплицы, где данные об уровне освещенности, собранные в одном месте, используются для принятия решений и управления исполнительным устройством (светодиодом) в другом, физически удаленном месте.

Ключевой особенностью архитектуры является использование протокола MQTT в качестве связующего звена, что обеспечивает асинхронный и надежный обмен данными между компонентами системы через интернет.

### 1.2. Проблема и предложенное решение

Задача управления удаленными устройствами на основе показаний датчиков требует надежного канала связи. Прямое соединение микроконтроллеров непрактично на больших расстояниях. Предложенное решение использует архитектуру "Издатель-Подписчик" (Publisher-Subscriber) на базе протокола MQTT. Это позволяет полностью разделить (decouple) компонент-датчик и компонент-исполнитель. Они не знают о существовании друг друга и обмениваются информацией исключительно через центрального посредника — MQTT-брокера.

Такой подход обеспечивает высокую масштабируемость, отказоустойчивость и гибкость системы.

## 2. Архитектура системы

Система состоит из следующих логических компонентов, взаимодействующих по четко определенным протоколам:

```
[Sensor MCU] <--UART--> [PC1: Python + MQTT Publisher]
                              |
                              v
                         [MQTT Broker]
                              |
                              v
[Actuator MCU] <--UART--> [PC2: Python + MQTT Subscriber]

                              ^
                              |
                         [Monitor PC] (подписан на все топики)
```

*   **Sensor MCU (Arduino Uno):** Постоянно измеряет уровень освещенности с помощью фоторезистора и отправляет данные на ПК1 по запросу через UART (Serial).
*   **PC1 (Publisher):** Python-скрипт, который опрашивает Sensor MCU, получает данные об освещенности и публикует их в соответствующий топик на MQTT-брокере.
*   **MQTT Broker:** Общедоступный облачный сервис (`broker.emqx.io`), выступающий в роли посредника для обмена сообщениями.
*   **PC2 (Subscriber):** Python-скрипт, который подписывается на топик с данными об освещенности, анализирует их и отправляет команды управления на Actuator MCU через UART.
*   **Actuator MCU (Arduino Uno/Nano):** Получает команды от ПК2 и управляет состоянием светодиода (включить, выключить, мигать).
*   **Monitor:** Отдельный Python-скрипт, который подписывается на все топики для мониторинга и отладки обмена сообщениями в системе.

## 3. Программная реализация

### 3.1. Протокол обмена данными

**Команды для Sensor MCU (PC1 -> MCU):**
*   `p`: Запросить одно значение с фоторезистора.
*   `s`: Начать потоковую передачу данных.

**Ответы Sensor MCU (MCU -> PC1):**
*   При получении `p` -> `SENSOR_VALUE:<value>\n`
*   При получении `s` -> `STREAM_STARTED\n`, затем `SENSOR_VALUE:<value>\n` каждые 2с.

**Команды для Actuator MCU (PC2 -> MCU):**
*   `u`: Включить светодиод.
*   `d`: Выключить светодиод.
*   `b`: Включить режим мигания.

**Ответы Actuator MCU (MCU -> PC2):**
*   При получении `u` -> `LED_GOES_ON\n`
*   При получении `d` -> `LED_GOES_OFF\n`
*   При получении `b` -> `LED_WILL_BLINK\n`

**Топики MQTT:**
*   `iot/home/luminosity`: Для публикации значений освещенности.
*   `iot/home/light_status`: Для публикации текущего состояния света (ON/OFF).
*   `iot/home/sensor_status` и `iot/home/actuator_status`: Для сообщений о состоянии клиентов.

### 3.2. Настройка Python-окружения
Перед запуском скриптов необходимо установить две библиотеки:
```bash
pip install paho-mqtt
pip install pyserial
```

## 4. Тестирование и верификация

### 4.1. Различия в тестировании: Tinkercad vs. Реальное оборудование

**Симуляция в Tinkercad:**
Онлайн-симулятор Tinkercad является изолированной средой и **не поддерживает** сетевые подключения к внешним MQTT-брокерам или взаимодействие с локальными Python-скриптами через COM-порты. Поэтому в симуляторе была проверена только аппаратная часть (правильность схем) и логика работы каждой прошивки Arduino в отдельности. Взаимодействие между платами имитировалось вручную путем копирования вывода из одного "Монитора порта" и вставки команд в другой.

**Тестирование на реальном оборудовании:**
Только при работе с физическими платами возможно реализовать полную архитектуру системы. Две платы Arduino были подключены к разным USB-портам компьютера, и для каждого был запущен свой Python-скрипт, обеспечивающий связь с MQTT-брокером. Этот подход позволил протестировать всю цепочку передачи данных в реальных условиях.

### 4.2. Результаты
В ходе тестирования на реальном оборудовании система продемонстрировала полную работоспособность в соответствии с поставленной задачей.
1.  Скрипт `sensor_pc.py` успешно опрашивал Sensor MCU и публиковал значения освещенности в топик `iot/home/luminosity`.
2.  Скрипт `monitor.py` корректно отображал все сообщения, проходящие через брокер.
3.  Скрипт `actuator_pc.py` получал данные, и при падении освещенности ниже порога `300`, отправлял команду `u` на Actuator MCU.
4.  Actuator MCU корректно включал светодиод и отправлял ответ `LED_GOES_ON`, который также фиксировался в терминале скрипта.
5.  При повышении освещенности система аналогичным образом отправляла команду `d` и выключала светодиод.

Все команды и статусы соответствовали разработанному протоколу.

## 5. Исходные коды и ресурсы

### 5.1. Код для микроконтроллеров

**Код для Sensor MCU (`sensor_mcu.ino`):**
```cpp
const int photoresistorPin = A0;
bool isStreaming = false;
unsigned long previousMillis = 0;
const long streamInterval = 2000;

void setup() {
  Serial.begin(9600);
  pinMode(photoresistorPin, INPUT);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'p') {
      isStreaming = false;
      sendSensorValue();
    } else if (cmd == 's') {
      isStreaming = true;
      Serial.println("STREAM_STARTED");
    }
  }

  if (isStreaming) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= streamInterval) {
      previousMillis = currentMillis;
      sendSensorValue();
    }
  }
}

void sendSensorValue() {
  int sensorValue = analogRead(photoresistorPin);
  Serial.print("SENSOR_VALUE:");
  Serial.println(sensorValue);
}
```

**Код для Actuator MCU (`actuator_mcu.ino`):**
```cpp
const int ledPin = 13;
char mode = 'd';
bool ledState = LOW;
unsigned long previousMillis = 0;
const long blinkInterval = 500;

void setup() {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 'u') {
      mode = 'u';
      digitalWrite(ledPin, HIGH);
      Serial.println("LED_GOES_ON");
    } else if (cmd == 'd') {
      mode = 'd';
      digitalWrite(ledPin, LOW);
      Serial.println("LED_GOES_OFF");
    } else if (cmd == 'b') {
      mode = 'b';
      Serial.println("LED_WILL_BLINK");
    }
  }

  if (mode == 'b') {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= blinkInterval) {
      previousMillis = currentMillis;
      ledState = !ledState;
      digitalWrite(ledPin, ledState);
    }
  }
}
```

### 5.2. Код для Python-скриптов
**Код для Monitor (`monitor.py`):**
```py
import paho.mqtt.client as mqtt
import random
import time

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-monitor-{random.randint(0, 1000)}'
# Символ '#' - это wildcard (маска), который означает "подписаться на все подтопики"
# внутри iot/home/.
TOPIC = "iot/home/#"  

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT."""
    if rc == 0:
        print("Монитор успешно подключен к MQTT брокеру.")
        # Подписываемся на все топики после успешного подключения
        client.subscribe(TOPIC)
        print(f"Подписан на все топики в ветке: {TOPIC}")
    else:
        print(f"Ошибка подключения монитора, код: {rc}")

def on_message(client, userdata, msg):
    """Callback при получении любого сообщения."""
    # Получаем текущее время для лога
    timestamp = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())
    # Выводим сообщение в удобном формате
    print(f"[{timestamp}] [{msg.topic}]: {msg.payload.decode('utf-8')}")

if __name__ == "__main__":
    client = mqtt.Client(client_id=CLIENT_ID)
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    
    try:
        print("Запуск MQTT монитора. Все сообщения из топиков iot/home/* будут отображаться здесь.")
        print("Нажмите CTRL+C для выхода.")
        client.loop_forever()
    except KeyboardInterrupt:
        print("\nМонитор остановлен.")
    finally:
        client.disconnect()
```

**Код для Sensor PC (`sensor_pc.py`):**
```py
import serial
import time
import paho.mqtt.client as mqtt
import random

SERIAL_PORT = 'COM8' 
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
# Уникальный ID клиента, чтобы избежать конфликтов на публичном брокере
CLIENT_ID = f'iot-project-sensor-publisher-{random.randint(0, 1000)}'

# Топики для публикации
LUMINOSITY_TOPIC = "iot/home/luminosity"
STATUS_TOPIC = "iot/home/sensor_status"

def on_connect(client, userdata, flags, rc):
    """Callback-функция, вызываемая при подключении к MQTT брокеру."""
    if rc == 0:
        print("Подключено к MQTT брокеру!")
        # Публикуем сообщение о том, что клиент-датчик подключился
        client.publish(STATUS_TOPIC, "Sensor client connected", qos=1, retain=True)
    else:
        print(f"Не удалось подключиться, код ошибки: {rc}")

def setup_serial():
    """Настраивает и открывает Serial-соединение с Arduino."""
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"Подключено к Arduino на порту {SERIAL_PORT}")
        time.sleep(2) # Даем время на перезагрузку Arduino после открытия порта
        return ser
    except serial.SerialException as e:
        print(f"Ошибка: Не удалось открыть порт {SERIAL_PORT}. Убедитесь, что порт правильный и не занят другой программой (например, Монитором порта в Arduino IDE).")
        print(e)
        exit()

if __name__ == "__main__":
    # Настройка MQTT клиента
    client = mqtt.Client(client_id=CLIENT_ID)
    client.on_connect = on_connect
    # "Последняя воля": если клиент отключится некорректно, брокер опубликует это сообщение
    client.will_set(STATUS_TOPIC, "Sensor client disconnected unexpectedly", qos=1, retain=True)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start() # Запускаем сетевой цикл в фоновом потоке

    # Настройка Serial порта
    arduino = setup_serial()

    try:
        # Бесконечный цикл опроса датчика
        while True:
            # Отправляем команду 'p' для получения одного значения
            arduino.write(b'p')
            
            # Ждем ответа от Arduino
            if arduino.in_waiting > 0:
                response = arduino.readline().decode('utf-8').strip()
                
                # Проверяем, что ответ имеет правильный формат
                if response.startswith("SENSOR_VALUE:"):
                    try:
                        # Извлекаем числовое значение
                        value = response.split(':')[1]
                        print(f"Получено значение освещенности: {value}")
                        
                        # Публикуем значение в MQTT топик
                        client.publish(LUMINOSITY_TOPIC, value, qos=1)
                        print(f"Опубликовано в топик {LUMINOSITY_TOPIC}")
                        
                    except (IndexError, ValueError) as e:
                        print(f"Ошибка парсинга ответа от Arduino: {response}, {e}")
            
            # Пауза перед следующим опросом
            time.sleep(5) 

    except KeyboardInterrupt:
        print("\nПрограмма завершена пользователем.")
    finally:
        # Корректно завершаем работу
        client.publish(STATUS_TOPIC, "Sensor client disconnected", qos=1, retain=True)
        client.disconnect()
        client.loop_stop()
        arduino.close()
        print("Соединения закрыты.")
```

**Код для Actuator PC (`actuator_pc.py`):**
```py
import serial
import time
import paho.mqtt.client as mqtt
import random

SERIAL_PORT = 'COM10'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-actuator-subscriber-{random.randint(0, 1000)}'

# Топики
LUMINOSITY_TOPIC = "iot/home/luminosity"
LIGHT_STATUS_TOPIC = "iot/home/light_status"
ACTUATOR_STATUS_TOPIC = "iot/home/actuator_status"

# Порог освещенности для включения/выключения света
LUMINOSITY_THRESHOLD = 10

# Глобальная переменная для доступа к Serial порту из callback-функции
arduino = None

def on_connect(client, userdata, flags, rc):
    """Callback при подключении к MQTT."""
    if rc == 0:
        print("Подключено к MQTT брокеру!")
        client.publish(ACTUATOR_STATUS_TOPIC, "Actuator client connected", qos=1, retain=True)
        # Подписываемся на топик с данными об освещенности
        client.subscribe(LUMINOSITY_TOPIC)
        print(f"Подписан на топик: {LUMINOSITY_TOPIC}")
    else:
        print(f"Не удалось подключиться, код ошибки: {rc}")

def on_message(client, userdata, msg):
    """Callback при получении сообщения из подписанного топика."""
    global arduino
    try:
        # Декодируем сообщение и преобразуем в число
        luminosity = int(msg.payload.decode('utf-8'))
        print(f"Получено значение освещенности: {luminosity}")

        # Проверяем, что Arduino подключена
        if arduino and arduino.is_open:
            if luminosity < LUMINOSITY_THRESHOLD:
                # Если темно, отправляем команду 'u' (up/on)
                arduino.write(b'u')
                # Публикуем текущий статус света
                client.publish(LIGHT_STATUS_TOPIC, "ON", qos=1, retain=True)
                print("Команда Arduino: включить свет (u)")
            else:
                # Если светло, отправляем команду 'd' (down/off)
                arduino.write(b'd')
                client.publish(LIGHT_STATUS_TOPIC, "OFF", qos=1, retain=True)
                print("Команда Arduino: выключить свет (d)")
    except (ValueError, TypeError) as e:
        print(f"Не удалось обработать сообщение: {msg.payload}, {e}")
    except serial.SerialException as e:
        print(f"Ошибка записи в Serial порт: {e}")

def setup_serial():
    """Настраивает и открывает Serial-соединение с Arduino."""
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        print(f"Подключено к Arduino на порту {SERIAL_PORT}")
        time.sleep(2)
        return ser
    except serial.SerialException as e:
        print(f"Ошибка: Не удалось открыть порт {SERIAL_PORT}. Проверьте подключение и номер порта.")
        return None

if __name__ == "__main__":
    arduino = setup_serial()
    
    # Запускаем MQTT клиент только если удалось подключиться к Arduino
    if arduino:
        client = mqtt.Client(client_id=CLIENT_ID)
        client.on_connect = on_connect
        client.on_message = on_message
        client.will_set(ACTUATOR_STATUS_TOPIC, "Actuator client disconnected unexpectedly", qos=1, retain=True)
        client.connect(MQTT_BROKER, MQTT_PORT, 60)
        
        try:
            # loop_forever() - это блокирующий цикл, который слушает сеть и вызывает callbacks.
            # Идеально для подписчика, которому нужно только реагировать на сообщения.
            client.loop_forever() 
        except KeyboardInterrupt:
            print("\nПрограмма завершена пользователем.")
        finally:
            client.publish(ACTUATOR_STATUS_TOPIC, "Actuator client disconnected", qos=1, retain=True)
            client.disconnect()
            arduino.close()
            print("Соединения закрыты.")
    else:
        print("Не удалось подключиться к Arduino. Программа завершена.")
```

### 5.3. Симуляция в Tinkercad

**[Ссылка для доступа к симуляции]([https://www.tinkercad.com/things/1cAyJMh7SFY-shiny-stantia?sharecode=Z2p4zGidT4t0a3Fup3Dwgy1tz33hdG9VQ8VCMRLEIl8](https://www.tinkercad.com/things/kJ1fYI45jP3-ledphotoserialmqtt?sharecode=undefined))**


## 6. Вывод

Данный проект успешно демонстрирует создание полноценной распределенной IoT-системы с использованием доступных компонентов и технологий. Были реализованы все поставленные задачи: асинхронный обмен данными между микроконтроллерами и ПК, взаимодействие между ПК через облачный MQTT-брокер и логика принятия решений на основе данных с датчика.

Разделение системы на модули и использование стандартных протоколов (UART, MQTT) доказывает гибкость и масштабируемость такого подхода. Проект является отличной практической демонстрацией ключевых концепций Интернета Вещей.



на примере этого напиши с моим кодом и абсолютно другими словами README.md файл , чтобы были все мои примеры и вот ссылка на мой tinkerkad : https://www.tinkercad.com/things/kJ1fYI45jP3-ledphotoserialmqtt?sharecode=OP_Hn8TEL4GAGT6yfM8pQaBNLzBAjWGCRpqsuxa5EgM
