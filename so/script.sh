#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

char=$1
count=0

while IFS= read -r line || [ -n "$line" ]; do
    if [[ $line =~ ^[A-Z] ]] && 
       [[ $line =~ [\.!?]$ ]] && 
       [[ $line == *"$char"* ]] && 
       [[ $line =~ ^[^,]*$ ]] || [[ $line =~ ^[^,]*și[^,]*$ ]]; then 
        ((count++))
    fi
done

echo $count
