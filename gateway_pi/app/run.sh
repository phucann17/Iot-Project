#!/bin/bash

gcc app.c -o app -lmosquitto -lcjson
./app