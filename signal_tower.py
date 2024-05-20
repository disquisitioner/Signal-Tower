import argparse
import configparser
import paho.mqtt.client as mqtt

CONFIGFILE = "signal_tower.ini"

# Process args first, so everything there is validated
parser = argparse.ArgumentParser()

# Require values for any specified lamp control options, either 'ON' or 'OFF
parser.add_argument("-r","--red",    choices=['ON','OFF'],help="Set red lamp ON or OFF")
parser.add_argument("-g","--green",  choices=['ON','OFF'],help="Set green lamp ON or OFF")
parser.add_argument("-b","--blue",   choices=['ON','OFF'],help="Set blue lamp ON or OFF")
parser.add_argument("-y","--yellow", choices=['ON','OFF'],help="Set yellow lamp ON or OFF")

# Parse command line options (to make sure they're provided correcctly)
args = parser.parse_args()

# Now process config file
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

# Send control messages as specified in our command line arguments
if args.red:
    print(f"Red lamp {args.red} via {redtopic}")
    client.publish(redtopic,args.red)

if args.green:
    print(f"Green lamp {args.green} via {greentopic}")
    client.publish(greentopic,args.green)

if args.blue:
    print(f"Blue lamp {args.blue} via {bluetopic}")
    client.publish(bluetopic,args.blue)

if args.yellow:
    print(f"Yellow lamp {args.yellow} via {yellowtopic}")
    client.publish(yellowtopic,args.yellow)
