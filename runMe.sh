#!/bin/bash

let numClients=1

if [ ! -z ${1+x} ]; then
    numClients=$1
fi

let numClients=$numClients+1

list="localhost 2000 localhost 2001"

let j=1

let bla=$numClients-1
while [[ $j -lt $bla ]]; do
    let j=$j+1
    list="$list localhost 200$j"
done

server="./network.exec server 0 $list"
echo "$server"
#$server &

let i=1
while [[ $i -lt $numClients ]]; do
    runClient="./network.exec client $i $list"
    echo "$runClient"
    #$runClient
    let i=$i+1
done
#./network.exec client 2 $list &
