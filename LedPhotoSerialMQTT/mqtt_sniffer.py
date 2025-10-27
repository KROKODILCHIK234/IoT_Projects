import paho.mqtt.client as mqtt
import random
import time

MQTT_BROKER = "broker.emqx.io"
MQTT_PORT = 1883
CLIENT_ID = f'sniffer-{random.randint(0, 1000)}'
TOPIC_TO_SNIFF = "iot/v2/home/#"

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
    sniffer = MqttSniffer(BROKER, 1883, TOPIC_TO_SNIFF)
    sniffer.start()
