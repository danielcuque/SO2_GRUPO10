#!/bin/bash

# Archivos de código fuente
PADRE_SRC="parent.c"
HIJO_SRC="child.c"

# Archivos ejecutables
PADRE_EXE="parent"
HIJO_EXE="child.bin"

# Archivo de log
LOG_FILE="syscalls.log"

# Archivo de script de SystemTap
STAP_SCRIPT="monitor_syscalls.stp"

# Compilar el proceso hijo
echo "Compilando el proceso hijo..."
gcc "$HIJO_SRC" -o "$HIJO_EXE"
if [ $? -ne 0 ]; then
    echo "Error compilando el proceso hijo."
    exit 1
fi

# Compilar el proceso padre
echo "Compilando el proceso padre..."
gcc "$PADRE_SRC" -o "$PADRE_EXE"
if [ $? -ne 0 ]; then
    echo "Error compilando el proceso padre."
    exit 1
fi

