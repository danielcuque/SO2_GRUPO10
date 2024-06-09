# Manual Técnico - Práctica 1

## Introducción
Este manual técnico proporciona una guía detallada sobre la implementación y funcionamiento de la práctica 1, que consiste en la creación de un proceso principal y dos procesos hijo utilizando llamadas al sistema `fork()`. El proceso principal monitorea las llamadas de sistema realizadas por los procesos hijo y registra la información en un archivo de registro. Además, se proporciona un script SystemTap para interceptar estas llamadas al sistema.

## Requisitos
- Compilador de C
- Sistema operativo Linux
- SystemTap instalado

## Estructura del proyecto
El proyecto está compuesto por los siguientes archivos:

1. `parent.c`: Código fuente del proceso padre.
2. `child.c`: Código fuente del proceso hijo.
3. `monitor_syscalls.stp`: Script SystemTap para monitorear las llamadas al sistema.

## Proceso padre (`parent.c`)
El proceso padre crea dos procesos hijo utilizando la llamada al sistema `fork()`. Luego, inicia el monitoreo de las llamadas al sistema realizadas por los procesos hijo utilizando SystemTap. Al recibir la señal SIGINT (Ctrl + C), el proceso padre imprime las estadísticas de las llamadas al sistema y finaliza su ejecución.

Claro, aquí tienes una explicación de los bloques de código más importantes:

1. **Manejo de señales (`sigint_handler`):**
   Este bloque de código define una función de manejo de señales llamada `sigint_handler`. Esta función se activa cuando el proceso recibe la señal SIGINT (Ctrl + C). Dentro de esta función, se abre el archivo de registro `syscalls.log` y se cuentan las llamadas al sistema realizadas por los procesos hijos. Luego, imprime las estadísticas y termina la ejecución del programa.

2. **Función principal (`main`):**
   Aquí comienza la ejecución del programa. Primero, se establece un manejador de señales para capturar SIGINT y llamar a la función `sigint_handler` cuando ocurra. Luego, se obtiene el PID del proceso padre. Después, se crea un archivo de registro `syscalls.log` y se cierra de inmediato.

3. **Creación de procesos hijos:**
   Se crean dos procesos hijos utilizando la llamada al sistema `fork()`. Cada hijo ejecuta un archivo binario llamado `child.bin` utilizando `execl()`. Si la creación de los procesos hijos falla, se imprime un mensaje de error.

4. **Ejecución de SystemTap:**
   Se crea un comando SystemTap utilizando `sprintf()` para ejecutar el script `monitor_syscalls.stp`, pasando los PID de los procesos hijos y el nombre del archivo de registro como argumentos. Este comando se ejecuta en segundo plano utilizando `system()`.

5. **Impresión de información:**
   Se imprime en la terminal el PID del proceso padre, así como los PID de los procesos hijos. Se indica que el proceso padre está esperando a que los hijos terminen su ejecución y se solicita al usuario que presione Ctrl + C para obtener las estadísticas.

6. **Espera de procesos hijos:**
   El proceso padre espera a que los dos procesos hijos terminen su ejecución utilizando la llamada al sistema `wait(NULL)` dos veces.

## Procesos hijo (`child.c`)
Cada proceso hijo realiza operaciones de manipulación de archivos sobre el archivo "practica1.txt". Estas operaciones incluyen abrir, escribir y leer desde el archivo con un intervalo de tiempo aleatorio entre 1 y 3 segundos.

Por supuesto, aquí tienes una explicación detallada de los bloques de código más importantes:

1. **Función `random_string`:**
   Esta función genera una cadena aleatoria de caracteres alfanuméricos de longitud `size - 1` y la almacena en el puntero `str`. Utiliza la función `rand()` para generar números aleatorios y seleccionar caracteres del conjunto definido en `setStr[]`. Finalmente, agrega el carácter nulo `\0` al final de la cadena para asegurar su terminación.

2. **Función `child_process`:**
   Esta función representa el código ejecutado por los procesos hijo. Primero, inicializa la semilla para la generación de números aleatorios utilizando el PID del proceso hijo. Luego, abre el archivo "practica1.txt" en modo escritura (`O_WRONLY`) o lo crea si no existe (`O_CREAT`) y lo trunca a cero bytes de longitud (`O_TRUNC`). Si la apertura del archivo falla, se muestra un mensaje de error y el proceso hijo termina con un código de salida de fallo.

   A continuación, entra en un bucle infinito donde realiza una de tres acciones de manera aleatoria:

   - **Caso 0 (`open`):** Cierra el archivo y lo vuelve a abrir en modo escritura, lo que efectivamente limpia su contenido anterior.
   - **Caso 1 (`write`):** Genera una cadena aleatoria utilizando la función `random_string` y la escribe en el archivo.
   - **Caso 2 (`read`):** Se posiciona al principio del archivo (`lseek`) y lee una cadena de 8 caracteres del archivo.

   Después de cada acción, el proceso hijo espera un tiempo aleatorio entre 1 y 3 segundos utilizando `sleep()` antes de realizar la siguiente acción.

   Finalmente, el proceso hijo cierra el descriptor de archivo y sale con un código de salida de éxito.

3. **Función `main`:**
   En la función `main`, simplemente se llama a la función `child_process()`, que es la función que contiene el código ejecutado por los procesos hijo. Esto inicia la ejecución del código del proceso hijo. La función `main` devuelve 0, lo que indica una salida exitosa.

## Script SystemTap (`monitor_syscalls.stp`)
Este script se utiliza para interceptar las llamadas al sistema realizadas por los procesos hijo. Se definen sondas para las llamadas `read`, `write` y `open`. El script se ejecuta desde el proceso padre para monitorear estas llamadas específicas.

## Compilación y ejecución
Para compilar el proyecto, ejecute los siguientes comandos:

```bash
gcc parent.c -o parent
gcc child.c -o child.bin
```

Para ejecutar el proyecto, ejecute el proceso padre (como sudo si fuera necesario):

```bash
sudo ./parent
```
Para detener la ejecución y obtener las estadísticas de las llamadas al sistema, presione Ctrl + C.

