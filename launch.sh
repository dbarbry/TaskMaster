#!/bin/bash
sudo apt update
sudo apt install libreadline-dev

g++ ./daemon/main.cpp -o daemon
./daemon/daemon &
sleep 1
g++ ./client/main.cpp -lreadline -o client
./client/client
