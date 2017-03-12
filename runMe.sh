#!/bin/bash

echo "Running"
./network.exec server 0 localhost 2000 localhost 2001 &
echo "0 spawned"
./network.exec client 1 localhost 2000 localhost 2001 &
echo "1 spawned"
