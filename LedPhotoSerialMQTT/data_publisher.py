import serial
import time
import paho.mqtt.client as mqtt
import random
import threading

SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'iot-project-sensor-publisher-{random.randint(0, 1000)}'

LUMINOSITY_TOPIC = "iot/v2/home/light_level"
STATUS_TOPIC = "iot/v2/home/sensor_gateway_status"

class SensorGateway:
    def __init__(self, serial_port, baud_rate, broker_address, broker_port):
        self.serial_port = serial_port
        self.baud_rate = baud_rate
        self.broker_address = broker_address
        self.broker_port = broker_port

        self.client_id = f'sensor-gateway-{random.randint(0, 1000)}'
        self.luminosity_topic = "iot/v2/home/light_level"
        self.status_topic = "iot/v2/home/sensor_gateway_status"
        
        self.device = self._connect_to_device()
        self.mqtt_client = self._connect_to_mqtt()

    def _connect_to_device(self):
        try:
            ser = serial.Serial(self.serial_port, self.baud_rate, timeout=2)
            print(f"Успешное подключение к устройству на порту {self.serial_port}")
            time.sleep(2) # Ожидание для инициализации Arduino
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
