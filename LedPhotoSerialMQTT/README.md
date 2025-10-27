
---

# Проект `LedPhotoSerialMQTT`: Распределенная IoT-система управления освещением

## 1. Обзор проекта и цели

### 1.1. Концепция
Этот документ описывает процесс создания и тестирования распределенной системы Интернета Вещей (IoT). Проект моделирует практический сценарий автоматизации, в котором показания датчика освещенности, полученные с одного микроконтроллера, передаются через облачный сервис для управления исполнительным устройством (светодиодом) на другом, физически удаленном микроконтроллере.

Ключевым элементом архитектуры является использование протокола MQTT, который обеспечивает асинхронный и надежный обмен данными между компонентами системы через интернет.

### 1.2. Архитектура "Издатель-Подписчик"
В основе проекта лежит архитектура "Издатель-Подписчик" (Publisher-Subscriber). Это позволяет полностью отделить логику сбора данных от логики исполнения команд.
*   **Издатель (Publisher):** Python-скрипт, подключенный к сенсору, отправляет данные в облако по определенному "адресу" (топику), не зная, кто их получит.
*   **Подписчик (Subscriber):** Python-скрипт, подключенный к исполнителю, подписывается на этот "адрес" и реагирует на поступающие данные, также не зная, кто их отправил.

Такая модель обеспечивает исключительную гибкость и масштабируемость: в систему можно легко добавить десятки новых датчиков или исполнителей, не изменяя существующие компоненты.

## 2. Архитектура системы

Система состоит из следующих логических компонентов, взаимодействующих по четко определенным протоколам:

```
[Узел-Сенсор] <---UART---> [ПК-Шлюз: data_publisher.py]
                               |
                               v
                       [MQTT-брокер (broker.emqx.io)]
                               |
                               v
[Узел-Исполнитель] <--UART--> [ПК-Контроллер: light_controller.py]

                               ^
                               |
                       [ПК-Монитор: mqtt_sniffer.py]
```

## 3. Программная реализация

### 3.1. Протокол обмена данными

Для взаимодействия между ПК и микроконтроллерами был разработан следующий протокол:

**Команды для Узла-Сенсора (PC1 -> MCU):**
*   `r`: Запросить однократное считывание значения (`read`).
*   `c`: Включить режим непрерывной потоковой передачи данных (`continuous`).

**Ответы Узла-Сенсора (MCU -> PC1):**
*   При получении `r` -> `LDR;<value>\n`
*   При получении `c` -> `STREAM_MODE_ON\n`, затем `LDR;<value>\n` каждые 2 секунды.

**Команды для Узла-Исполнителя (PC2 -> MCU):**
*   `1`: Включить светодиод (логическая "единица").
*   `0`: Выключить светодиод (логический "ноль").
*   `2`: Включить режим мигания.

**Ответы Узла-Исполнителя (MCU -> PC2):**
*   При получении `1` -> `STATE_ON\n`
*   При получении `0` -> `STATE_OFF\n`
*   При получении `2` -> `STATE_BLINKING\n`

**Топики MQTT:**
*   `iot/v2/home/light_level`: Для публикации значений освещенности.
*   `iot/v2/home/light_actual_state`: Для публикации текущего состояния света (ON/OFF).
*   `iot/v2/home/sensor_gateway_status` и `iot/v2/home/actuator_controller_status`: Для сообщений о состоянии клиентов.

### 3.2. Настройка Python-окружения
Перед запуском скриптов необходимо установить две библиотеки:
```bash
pip install paho-mqtt pyserial
```

## 4. Тестирование и верификация

### 4.1. Различия подходов: Симуляция и Физическая сборка

**Этап 1: Симуляция в Tinkercad**
Онлайн-симулятор Tinkercad является изолированной средой ("песочницей") и **не поддерживает** сетевые подключения к внешним MQTT-брокерам или взаимодействие с локальными Python-скриптами через COM-порты. Поэтому на этом этапе была проведена **модульная проверка**:
*   Проверена корректность электронных схем.
*   Протестирована логика каждой прошивки Arduino по отдельности. Взаимодействие между ними имитировалось вручную через "Монитор порта".

**Этап 2: Тестирование на реальном оборудовании**
Только физическая сборка позволяет проверить всю систему в сборе. Две платы Arduino были подключены к разным USB-портам, что позволило запустить для каждой свой Python-скрипт и протестировать полный цикл передачи данных через интернет.

### 4.2. Итоги тестирования
В ходе тестов на реальном оборудовании система продемонстрировала полную работоспособность.
1.  Скрипт **`data_publisher.py`** успешно опрашивал Sensor MCU и публиковал значения освещенности в топик `iot/v2/home/light_level`.
2.  Скрипт **`mqtt_sniffer.py`** корректно отображал все сообщения, проходящие через брокер, с указанием времени, топика и содержимого.
3.  Скрипт **`light_controller.py`** получал данные, и при падении освещенности ниже порога `400`, отправлял команду `1` на Actuator MCU.
4.  Actuator MCU включал светодиод и отправлял ответ `STATE_ON`.
5.  При повышении освещенности система аналогичным образом отправляла команду `0`, и светодиод выключался.

Все команды и статусы соответствовали разработанному протоколу, система работала стабильно и предсказуемо.

## 5. Исходные коды и ресурсы

### 5.1. Код для микроконтроллеров

**Код для Узла-Сенсора (файл `light_sensor_node.ino`):**
```cpp
/**
 * @file light_sensor_node.ino
 * @brief Прошивка для микроконтроллера, считывающего данные с фоторезистора.
 */
#define LDR_PIN A0
#define STREAM_INTERVAL 2000

unsigned long lastStreamTime = 0;
bool isContinuousMode = false;

void setup() {
  Serial.begin(9600);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  handleSerialCommands();
  if (isContinuousMode) {
    streamLuminosityData();
  }
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch (command) {
      case 'r':
        isContinuousMode = false;
        reportLuminosity();
        break;
      case 'c':
        isContinuousMode = true;
        Serial.println("STREAM_MODE_ON");
        break;
    }
  }
}

void streamLuminosityData() {
  if (millis() - lastStreamTime >= STREAM_INTERVAL) {
    lastStreamTime = millis();
    reportLuminosity();
  }
}

void reportLuminosity() {
  int luminosityValue = analogRead(LDR_PIN);
  Serial.print("LDR;");
  Serial.println(luminosityValue);
}
```

**Код для Узла-Исполнителя (файл `led_actuator_node.ino`):**
```cpp
/**
 * @file led_actuator_node.ino
 * @brief Прошивка для микроконтроллера, управляющего светодиодом.
 */
#define LED_PIN 13
#define BLINK_RATE 500

enum LedMode { OFF, ON, BLINK };
LedMode currentMode = OFF;

unsigned long lastBlinkTime = 0;
bool ledCurrentState = LOW;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    switch (command) {
      case '1':
        currentMode = ON;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("STATE_ON");
        break;
      case '0':
        currentMode = OFF;
        digitalWrite(LED_PIN, LOW);
        Serial.println("STATE_OFF");
        break;
      case '2':
        currentMode = BLINK;
        Serial.println("STATE_BLINKING");
        break;
    }
  }

  if (currentMode == BLINK) {
    handleBlinking();
  }
}

void handleBlinking() {
  if (millis() - lastBlinkTime >= BLINK_RATE) {
    lastBlinkTime = millis();
    ledCurrentState = !ledCurrentState;
    digitalWrite(LED_PIN, ledCurrentState);
  }
}
```

### 5.2. Код для Python-скриптов

**Код для Монитора (файл `mqtt_sniffer.py`):**
```python
import paho.mqtt.client as mqtt
import random
import time

class MqttSniffer:
    def __init__(self, broker_address, broker_port, topic_wildcard):
        self.broker_address = broker_address
        self.broker_port = broker_port
        self.topic_wildcard = topic_wildcard
        self.client_id = f'sniffer-{random.randint(0, 1000)}'
        
        self.client = mqtt.Client(client_id=self.client_id)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Сниффер подключен к брокеру {self.broker_address}.")
            client.subscribe(self.topic_wildcard)
            print(f"Прослушивается топик: {self.topic_wildcard}")
        else:
            print(f"Ошибка подключения сниффера, код: {rc}")

    def _on_message(self, client, userdata, msg):
        timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
        try:
            payload_str = msg.payload.decode('utf-8')
            print(f"[{timestamp}] Topic: {msg.topic} | Payload: {payload_str}")
        except Exception as e:
            print(f"Не удалось декодировать сообщение в топике {msg.topic}: {e}")

    def start(self):
        try:
            self.client.connect(self.broker_address, self.broker_port, 60)
            print("Запуск сниффера... Нажмите CTRL+C для выхода.")
            self.client.loop_forever()
        except KeyboardInterrupt:
            print("\nОстановка сниффера.")
        except Exception as e:
            print(f"Произошла ошибка: {e}")
        finally:
            self.client.disconnect()
            print("Сниффер отключен.")

if __name__ == "__main__":
    BROKER = "broker.emqx.io"
    TOPIC_TO_SNIFF = "iot/v2/home/#"
    
    sniffer = MqttSniffer(BROKER, 1883, TOPIC_TO_SNIFF)
    sniffer.start()
```

**Код для PC-Датчика (файл `data_publisher.py`):**
```python
import serial
import time
import paho.mqtt.client as mqtt
import random

# --- НАСТРОЙКИ ---
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-sensor-publisher-{random.randint(0, 1000)}'

LUMINOSITY_TOPIC = "iot/v2/home/light_level"
STATUS_TOPIC = "iot/v2/home/sensor_gateway_status"
# --- КОНЕЦ НАСТРОЕК ---

class SensorGateway:
    def __init__(self, serial_port, baud_rate, broker_address, broker_port):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.broker_address = broker_address
        self.broker_port = broker_port
        self.client_id = f'sensor-gateway-{random.randint(0, 1000)}'
        self.luminosity_topic = LUMINOSITY_TOPIC
        self.status_topic = STATUS_TOPIC
        self.device = self._connect_to_device()
        self.mqtt_client = self._connect_to_mqtt()

    def _connect_to_device(self):
        try:
            ser = serial.Serial(self.serial_port, self.baud_rate, timeout=2)
            print(f"Успешное подключение к устройству на порту {self.serial_port}")
            time.sleep(2)
            return ser
        except serial.SerialException as e:
            print(f"Критическая ошибка: Не удалось подключиться к {self.serial_port}. {e}")
            print("Возможно, нужно выполнить 'sudo usermod -a -G dialout $USER' и перезагрузиться.")
            exit()

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Успешное подключение к MQTT брокеру.")
            client.publish(self.status_topic, "ONLINE", qos=1, retain=True)
        else:
            print(f"Ошибка подключения к MQTT: код {rc}")

    def _connect_to_mqtt(self):
        client = mqtt.Client(client_id=self.client_id)
        client.on_connect = self._on_connect
        client.will_set(self.status_topic, "OFFLINE", qos=1, retain=True)
        try:
            client.connect(self.broker_address, self.broker_port, 60)
            return client
        except Exception as e:
            print(f"Критическая ошибка: Не удалось подключиться к MQTT брокеру. {e}")
            exit()

    def run(self):
        self.mqtt_client.loop_start()
        print("Шлюз запущен. Запрос данных каждые 5 секунд...")
        try:
            while True:
                self.device.write(b'r')
                response = self.device.readline().decode('utf-8').strip()
                if response.startswith("LDR;"):
                    try:
                        value = response.split(';')[1]
                        print(f"Считано значение: {value}")
                        self.mqtt_client.publish(self.luminosity_topic, value, qos=1)
                        print(f"Значение опубликовано в топик {self.luminosity_topic}")
                    except IndexError:
                        print(f"Получен некорректный ответ: {response}")
                time.sleep(5)
        except KeyboardInterrupt:
            print("\nЗавершение работы...")
        finally:
            self.shutdown()

    def shutdown(self):
        self.mqtt_client.publish(self.status_topic, "OFFLINE", qos=1, retain=True)
        self.mqtt_client.disconnect()
        self.mqtt_client.loop_stop()
        self.device.close()
        print("Все соединения корректно закрыты.")

if __name__ == "__main__":
    gateway = SensorGateway(SERIAL_PORT, 9600, MQTT_BROKER, 1883)
    gateway.run()
```

**Код для PC-Исполнителя (файл `light_controller.py`):**
```python
import serial
import time
import paho.mqtt.client as mqtt
import random

# --- НАСТРОЙКИ ---
SERIAL_PORT = '/dev/ttyUSB1'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'actuator-controller-{random.randint(0, 1000)}'

LUMINOSITY_TOPIC = "iot/v2/home/light_level"
LIGHT_STATE_TOPIC = "iot/v2/home/light_actual_state"
STATUS_TOPIC = "iot/v2/home/actuator_controller_status"

LUMINOSITY_THRESHOLD = 400
# --- КОНЕЦ НАСТРОЕК ---

class ActuatorController:
    def __init__(self, serial_port, baud_rate, broker_address, broker_port):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.broker_address = broker_address
        self.broker_port = broker_port
        self.device = None
        self.client_id = f'actuator-controller-{random.randint(0, 1000)}'
        self.luminosity_topic = LUMINOSITY_TOPIC
        self.light_state_topic = LIGHT_STATE_TOPIC
        self.status_topic = STATUS_TOPIC
        self.threshold = LUMINOSITY_THRESHOLD

    def _connect_to_device(self):
        try:
            self.device = serial.Serial(self.serial_port, self.baud_rate, timeout=2)
            print(f"Успешное подключение к устройству на порту {self.serial_port}")
            time.sleep(2)
            return True
        except serial.SerialException as e:
            print(f"Критическая ошибка: Не удалось подключиться к {self.serial_port}. {e}")
            print("Возможно, нужно выполнить 'sudo usermod -a -G dialout $USER' и перезагрузиться.")
            return False

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print("Успешное подключение к MQTT брокеру.")
            client.publish(self.status_topic, "ONLINE", qos=1, retain=True)
            client.subscribe(self.luminosity_topic)
            print(f"Подписка на топик: {self.luminosity_topic}")
        else:
            print(f"Ошибка подключения к MQTT: код {rc}")

    def _on_message(self, client, userdata, msg):
        try:
            luminosity_value = int(msg.payload.decode('utf-8'))
            print(f"Получен новый уровень освещенности: {luminosity_value}")
            if not self.device or not self.device.is_open:
                print("Ошибка: нет связи с Arduino.")
                return

            if luminosity_value < self.threshold:
                print("Темно. Отправка команды на включение света (1)...")
                self.device.write(b'1')
                client.publish(self.light_state_topic, "ON", qos=1, retain=True)
            else:
                print("Светло. Отправка команды на выключение света (0)...")
                self.device.write(b'0')
                client.publish(self.light_state_topic, "OFF", qos=1, retain=True)
        except (ValueError, TypeError):
            print(f"Получены некорректные данные: {msg.payload}")

    def run(self):
        if not self._connect_to_device():
            return
        self.mqtt_client = mqtt.Client(client_id=self.client_id)
        self.mqtt_client.on_connect = self._on_connect
        self.mqtt_client.on_message = self._on_message
        self.mqtt_client.will_set(self.status_topic, "OFFLINE", qos=1, retain=True)
        self.mqtt_client.connect(self.broker_address, self.broker_port, 60)
        try:
            print("Контроллер запущен. Ожидание сообщений...")
            self.mqtt_client.loop_forever()
        except KeyboardInterrupt:
            print("\nЗавершение работы...")
        finally:
            self.shutdown()

    def shutdown(self):
        if self.mqtt_client:
            self.mqtt_client.publish(self.status_topic, "OFFLINE", qos=1, retain=True)
            self.mqtt_client.disconnect()
        if self.device:
            self.device.close()
        print("Все соединения корректно закрыты.")

if __name__ == "__main__":
    controller = ActuatorController(SERIAL_PORT, 9600, MQTT_BROKER, 1883)
    controller.run()
```

### 5.3. Ссылки

*   **[Ссылка на проект в Tinkercad ]
*   ([https://www.tinkercad.com/things/kJ1fYI45jP3-ledphotoserialmqtt?sharecode=OP_Hn8TEL4GAGT6yfM8pQaBNLzBAjWGCRpqsuxa5EgM](https://www.tinkercad.com/things/kJ1fYI45jP3-ledphotoserialmqtt?sharecode=OP_Hn8TEL4GAGT6yfM8pQaBNLzBAjWGCRpqsuxa5EgM))**

## 6. Вывод

Данный проект успешно демонстрирует создание полноценной распределенной IoT-системы с использованием доступных компонентов и технологий. Были реализованы все поставленные задачи: асинхронный обмен данными между микроконтроллерами и ПК, взаимодействие между ПК через облачный MQTT-брокер и логика принятия решений на основе данных с датчика.

Разделение системы на независимые модули (сенсор, шлюз, контроллер, исполнитель) и использование стандартных протоколов (UART, MQTT) доказывает гибкость и масштабируемость такого подхода. Проект является отличной практической демонстрацией ключевых концепций Интернета Вещей.
