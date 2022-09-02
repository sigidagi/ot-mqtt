#!/bin/bash

if ! command -v mosquitto_pub &> /dev/null; then
    echo "mosquitto_pub could not be found. Install using your package manager"
    exit
fi

USER=${1:-}
PASS=${2:-}
PORT=1883
HOST=test.mosquitto.org
TOPIC=claire/bedroom/temperature
MSG=Hello_claire

echo "Username: ${USER}"

mosquitto_pub -h $HOST -u $USER -P $PASS -t $TOPIC -m $MSG
