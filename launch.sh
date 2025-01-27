#!/bin/bash
./daemon/main.cpp &
sleep 1
./client/main.cpp
