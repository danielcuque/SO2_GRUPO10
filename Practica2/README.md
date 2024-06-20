# Manual Técnico - Práctica 2

### 1. Introducción
La siguiente práctica consiste de la implementación de  sistema bancario básico que permite la carga de cuentas de usuarios desde un archivo JSON, así como la realización de operaciones como depósitos, retiros y transferencias entre cuentas. Además, genera reportes de carga de usuarios y de operaciones en archivos de texto y JSON.

### 2. Estructuras de Datos

#### `Cuenta`
- **Descripción**: Estructura que representa una cuenta bancaria.
- **Campos**:
  - `no_cuenta`: Número de cuenta.
  - `nombre`: Nombre del titular de la cuenta.
  - `saldo`: Saldo disponible en la cuenta.
  - `mutex`: Mutex para garantizar la exclusión mutua al acceder a la cuenta.

#### `UserThreadArgs`
- **Descripción**: Argumentos para los hilos que cargan usuarios.
- **Campos**:
  - `json`: Objeto cJSON que contiene los datos a procesar.
  - `initial_index`, `final_index`: Índices inicial y final para dividir el trabajo entre hilos.
  - `processed_records`: Número de registros procesados por el hilo.

#### `TransactionThreadArgs`
- **Descripción**: Argumentos para los hilos que manejan operaciones.
- **Campos**:
  - `json`: Objeto cJSON que contiene las operaciones a procesar.
  - `retiros`, `depositos`, `transferencias`, `operaciones`: Contadores para estadísticas de operaciones.

#### `ReportError`
- **Descripción**: Estructura para almacenar errores de validación.
- **Campos**:
  - `message`: Mensaje de error.
  - `line`: Número de línea donde ocurrió el error.

### 3. Funciones Principales

#### `inicializar_cuenta`
- **Descripción**: Inicializa una estructura de tipo `Cuenta`.
- **Parámetros**:
  - `cuenta`: Puntero a la estructura `Cuenta`.
  - `num`: Número de cuenta.
  - `nombre`: Nombre del titular de la cuenta.
  - `saldo`: Saldo inicial de la cuenta.

#### `agregar_cuenta`
- **Descripción**: Agrega una nueva cuenta al sistema.
- **Parámetros**:
  - `num`: Número de cuenta.
  - `nombre`: Nombre del titular de la cuenta.
  - `saldo`: Saldo inicial de la cuenta.

#### `agregar_error`
- **Descripción**: Registra un error de validación.
- **Parámetros**:
  - `message`: Descripción del error.
  - `line`: Número de línea donde ocurrió el error.

#### `existe_cuenta`
- **Descripción**: Verifica si una cuenta ya existe en el sistema.
- **Parámetros**:
  - `num`: Número de cuenta a verificar.

#### `obtener_fecha_formateada`
- **Descripción**: Obtiene la fecha y hora actuales formateadas como una cadena de caracteres.
- **Retorno**: Cadena de caracteres con la fecha y hora.

#### `generar_archivo`
- **Descripción**: Crea un archivo con el contenido especificado.
- **Parámetros**:
  - `filename`: Nombre del archivo a crear.
  - `content`: Contenido a escribir en el archivo.

#### `reporte_usuarios`
- **Descripción**: Genera un reporte de carga de usuarios en un archivo de texto.
- **Parámetros**:
  - `thread_args`: Arreglo de estructuras `UserThreadArgs` con los datos de cada hilo.

#### `reporte_transacciones`
- **Descripción**: Genera un reporte de operaciones en un archivo de texto.
- **Parámetros**:
  - `thread_args`: Arreglo de estructuras `TransactionThreadArgs` con los datos de cada hilo.

### 4. Funciones de Hilos

#### `cargar_usuarios_thread`
- **Descripción**: Función ejecutada por cada hilo para cargar usuarios desde un archivo JSON.
- **Parámetros**:
  - `args`: Argumentos del hilo de tipo `UserThreadArgs`.

#### `cargar_usuarios`
- **Descripción**: Carga usuarios desde un archivo JSON utilizando múltiples hilos.
- **Parámetros**:
  - `filename`: Nombre del archivo JSON con los datos de los usuarios.

#### `cargar_operaciones_masiva_thread`
- **Descripción**: Función ejecutada por cada hilo para procesar operaciones masivas desde un archivo JSON.
- **Parámetros**:
  - `args`: Argumentos del hilo de tipo `TransactionThreadArgs`.

#### `cargar_operaciones_masiva`
- **Descripción**: Carga operaciones masivas desde un archivo JSON utilizando múltiples hilos.
- **Parámetros**:
  - `filename`: Nombre del archivo JSON con las operaciones.

### 5. Funciones Auxiliares

#### `clear_errors`
- **Descripción**: Libera la memoria utilizada para almacenar errores.

#### `deposito_transaccion`, `retiro_transaccion`, `transferencia_transaccion`
- **Descripción**: Realizan depósitos, retiros y transferencias entre cuentas respectivamente.

#### `mostrar_cuenta`
- **Descripción**: Muestra la información de una cuenta específica.

### 6. Funciones de Interfaz de Usuario

#### `deposito_individual`, `retiro_individual`, `transferencia_individual`
- **Descripción**: Funciones que manejan las operaciones individuales de depósito, retiro y transferencia desde la interfaz de usuario.

### 7. Funciones Principales

#### `main`
- **Descripción**: Función principal que gestiona el flujo principal del programa. No proporcionado en el código, pero asumido para el contexto general.

### 8. Compilación y Ejecución

Para compilar el programa, es necesario vincular tanto el archivo `main.c` como la biblioteca `cJSON`:
```bash
gcc main.c cJSON.c -o program
```

Para ejecutar el programa:
```bash
./program
```

### 9. Consideraciones Finales

Este código hace uso intensivo de hilos (`pthread`) para operaciones concurrentes y mutexes (`pthread_mutex_t`) para asegurar la exclusión mutua en operaciones compartidas. También maneja la lectura y escritura de archivos JSON utilizando la biblioteca `cJSON`. Asegúrate de tener instaladas las dependencias necesarias y de compilar el programa correctamente antes de ejecutarlo.

Esta documentación proporciona una visión general del código y sus componentes principales. Para una comprensión más profunda, es recomendable revisar la implementación detallada de cada función y su interacción con las estructuras de datos definidas.

