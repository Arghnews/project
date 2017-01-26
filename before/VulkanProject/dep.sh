#!/bin/bash

file="Camera.d"
i=0
# obj -> object file that needs recompile or not
declare -a dependencies
for line in $(cat "$file"); do
    valid=1
    if [ $i -eq 0 ]; then
        obj=$(echo "$line" | sed -E "s/^(.*):$/\1/g")
    elif [ "$line" == \\ ]; then
        :
    else
        valid=0
    fi
    if [ $valid -eq 0 ]; then
        dependencies[$i]="$line"
    fi
    let i=$i+1
done

# ${#arrayname[@]} -> length of array

for ele in "${dependencies[@]}"; do
    if [ "$ele" -nt "$obj" ]; then
        echo "$obj needs recompile"
        exit 1
    fi
done

exit 0
