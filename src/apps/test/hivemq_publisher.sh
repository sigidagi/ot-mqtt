#!/bin/bash

if ! command -v mosquitto_pub &> /dev/null; then
    echo "mosquitto_pub could not be found. Install using your package manager"
    exit
fi


USER=${1:-}
PASS=${2:-}
PORT=8883
HOST=ffa48553d3bd43ccaec31f521ecb192e.s2.eu.hivemq.cloud
TOPIC=claire/bedroom/temperature
MSG=Hello_claire

mosquitto_pub -h $HOST -p $PORT -u $USER -P $PASS -t $TOPIC -m $MSG
