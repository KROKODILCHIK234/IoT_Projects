import paho.mqtt.client as mqtt
import time

broker = "broker.emqx.io"
port = 1883

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT Broker!")
    client.subscribe("esp8266/sensor")

def on_message(client, userdata, msg):
    print(f"Received from ESP ({msg.topic}): {msg.payload.decode()}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

print(f"Connecting to broker {broker}:{port}...")
client.connect(broker, port)
client.loop_start()

commands = ["ON", "OFF"]
try:
    for cmd in commands * 5:
        print(f"Sending command to ESP: {cmd}")
        client.publish("esp8266/command", cmd)
        time.sleep(2)
    print("Test finished. Listening for sensor data for a few more seconds...")
    time.sleep(5)
except KeyboardInterrupt:
    print("Exiting...")

client.loop_stop()
