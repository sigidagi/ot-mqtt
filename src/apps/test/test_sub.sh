#!/bin/bash

if ! command -v mosquitto_sub &> /dev/null; then
    echo "mosquitto_sub could not be found. Install using your package manager"
    exit
fi

USER=${1:-}
PASS=${2:-}
PORT=1883
HOST=test.mosquitto.org
TOPIC=claire/bedroom/temperature

mosquitto_sub -h $HOST -u $USER -P $PASS -t $TOPIC 
