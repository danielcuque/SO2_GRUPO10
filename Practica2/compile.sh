#!/bin/bash

echo "Compilando programa..."
gcc main.c cJSON.c -o program
if [ $? -ne 0 ]; then
    echo "Error compilando programa."
    exit 1
fi

