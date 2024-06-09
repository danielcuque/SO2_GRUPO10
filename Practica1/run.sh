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

# Limpiar el archivo de log
echo "Limpiando el archivo de log..."
> "$LOG_FILE"

# Ejecutar el proceso padre en segundo plano para obtener los PID de los hijos
echo "Ejecutando el proceso padre..."
./"$PADRE_EXE" &
PADRE_PID=$!

# Esperar un momento para asegurarse de que los procesos hijos se han creado
sleep 1

# Obtener los PID de los procesos hijos
CHILD_PID1=$(pgrep -P $PADRE_PID | head -n 1)
CHILD_PID2=$(pgrep -P $PADRE_PID | tail -n 1)

echo "PID del primer proceso hijo: $CHILD_PID1"
echo "PID del segundo proceso hijo: $CHILD_PID2"

# Actualizar el script de SystemTap con los PID de los hijos
echo "Actualizando el script de SystemTap..."
sed -i "s/target_pid1 = .*/target_pid1 = $CHILD_PID1/" $STAP_SCRIPT
sed -i "s/target_pid2 = .*/target_pid2 = $CHILD_PID2/" $STAP_SCRIPT

# Ejecutar el script de SystemTap con los PID de los hijos
echo "Ejecutando el script de SystemTap..."
stap -o "$LOG_FILE" -v $STAP_SCRIPT &

# Obtener el PID del script de SystemTap
STAP_PID=$!

# Esperar a que termine el proceso padre
wait $PADRE_PID

# Detener el script de SystemTap
kill $STAP_PID
wait $STAP_PID

echo "Ejecución completa. Revisa el archivo $LOG_FILE para ver los detalles de las llamadas al sistema."
