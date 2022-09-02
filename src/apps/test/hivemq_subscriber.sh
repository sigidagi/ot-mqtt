#!/bin/bash

if ! command -v mosquitto_sub &> /dev/null; then
    echo "mosquitto_sub could not be found. Install using your package manager"
    exit
fi

USER=${1:-}
PASS=${2:-}
PORT=8883
HOST=ffa48553d3bd43ccaec31f521ecb192e.s2.eu.hivemq.cloud
TOPIC=claire/bedroom/temperature

mosquitto_sub -h $HOST -u $USER -P $PASS -p $PORT -t $TOPIC
