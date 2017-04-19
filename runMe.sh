#!/bin/bash

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:lib

let numClients=1

#executable="./network.exec"
executable="./game.exec"

if [ ! -z ${1+x} ]; then
    if [ "$1" == "local" ];then
        executable="$executable local"
        $executable
        exit 0
    fi
    numClients=$1
fi

#[client/server] [0-9] 2000 localhost 2010 2001 localhost 2001

let numClients=$numClients

clients=()

list=""

serverStartPort=2000
let clientsStartPort=$serverStartPort+10

sport=$serverStartPort
addr="localhost"
cport=$clientsStartPort

i=0
let j=$numClients+1
while [ $i -lt $j ];do
    list="$list $sport $addr $cport"
    let sport=$sport+1
    let cport=$cport+1
    let i=$i+1
done

i=0
let j=$numClients

while [ $i -lt $j ];do
    let i=$i+1
    client="$executable client"
    client="$client $i"
    client="$client $list"

    clients+=("$client")
done

server="$executable server 0 $list"
echo "$server"
$server &

for client in "${clients[@]}"; do
    echo "$client"
    $client &
done
