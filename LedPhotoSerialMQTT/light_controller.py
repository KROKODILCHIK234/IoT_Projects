import serial
import time
import paho.mqtt.client as mqtt
import random

SERIAL_PORT = '/dev/ttyUSB1'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'actuator-controller-{random.randint(0, 1000)}'

LUMINOSITY_TOPIC = "iot/v2/home/light_level"
LIGHT_STATE_TOPIC = "iot/v2/home/light_actual_state"
STATUS_TOPIC = "iot/v2/home/actuator_controller_status"

LUMINOSITY_THRESHOLD = 400

class ActuatorController:
    def __init__(self, serial_port, baud_rate, broker_address, broker_port):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.broker_address = broker_address
        self.broker_port = broker_port
        self.device = None
        self.client_id = f'actuator-controller-{random.randint(0, 1000)}'
        self.luminosity_topic = "iot/v2/home/light_level"
        self.light_state_topic = "iot/v2/home/light_actual_state"
        self.status_topic = "iot/v2/home/actuator_controller_status"
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
