#!/bin/bash
echo "Crash program 1"
sleep 5

# Utiliser un nombre aléatoire pour décider (environ 50% de chance d'échec)
if [ $((RANDOM % 2)) -eq 0 ]; then
    echo "Je vais échouer cette fois!"
    exit 3
else
    echo "Je vais réussir cette fois!"
    sleep 10
    exit 0
fi