import time
import argparse
import configparser
import paho.mqtt.client as mqtt

# Config file
CONFIGFILE = "signal_tower.ini"
# Loop delay in seconds
DELAY = 300

# Process config file
config = configparser.ConfigParser()
config.read(CONFIGFILE)

# Extract config settings for our MQTT broker
host = config['BROKER']['host']
port = int(config['BROKER']['port'])  # must be an integer
user = config['BROKER']['user']
passwd = config['BROKER']['passwd']

# Extract topics to use in publishing signal tower control messages
redtopic = config['TOPICS']['redtopic']
greentopic = config['TOPICS']['greentopic']
bluetopic = config['TOPICS']['bluetopic']
yellowtopic = config['TOPICS']['yellowtopic']
towertopic = config['TOPICS']['towertopic']

# Establish connection to MQTT broker
client = mqtt.Client()
client.username_pw_set(user,passwd)
client.connect(host,port,60)

status = 1

while True:
    # print(status, flush=True)
    client.publish(towertopic,str(status))
    if status >= 15:
        status = 0;
    else:
        status = status + 1
    time.sleep(DELAY)

