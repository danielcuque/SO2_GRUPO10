#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include <unistd.h> // Para la función sleep

#define NUM_THREADS 3
#define NUM_THREADS_2 4

typedef struct {
    int no_cuenta;
    char nombre[100];
    double saldo;
    pthread_mutex_t mutex;
} Cuenta;

typedef struct {
    cJSON *json;
    int initial_index;
    int final_index;
    int processed_records;
} UserThreadArgs;

typedef struct {
    cJSON *json;
    int retiros;
    int depositos;
    int transferencias;
    int operaciones;
} TransactionThreadArgs;


Cuenta *cuentas = NULL;
int num_cuentas = 0;
pthread_mutex_t cuentas_mutex = PTHREAD_MUTEX_INITIALIZER;
int current_transaction = 0;

void inicializar_cuenta(Cuenta *cuenta, int num, const char *nombre, double saldo) {
    cuenta->no_cuenta = num;
    strcpy(cuenta->nombre, nombre);
    cuenta->saldo = saldo;
    pthread_mutex_init(&cuenta->mutex, NULL);
}

void agregar_cuenta(int num, const char *nombre, double saldo) {
    pthread_mutex_lock(&cuentas_mutex);
    num_cuentas++;
    cuentas = (Cuenta *)realloc(cuentas, num_cuentas * sizeof(Cuenta));
    if (cuentas == NULL) {
        perror("Error al asignar memoria para la cuenta");
        exit(EXIT_FAILURE);
    }
    inicializar_cuenta(&cuentas[num_cuentas - 1], num, nombre, saldo);
    pthread_mutex_unlock(&cuentas_mutex);
}

int existe_cuenta(int num) {
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == num) {
            return 1;
        }
    }
    return 0;
}

char* obtener_fecha_formateada() {
    static char fecha[20];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(fecha, sizeof(fecha), "%04d_%02d_%02d-%02d_%02d_%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
             tm.tm_hour, tm.tm_min, tm.tm_sec);
    return fecha;
}

void generar_archivo(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo.\n");
        return;
    }

    fprintf(file, content);
    fclose(file);
}


void reporte_usuarios(UserThreadArgs *thread_args) {
    char nombre_archivo[100];

    char *fecha = obtener_fecha_formateada();
    snprintf(nombre_archivo, sizeof(nombre_archivo),
             "carga_%s.log", fecha);

    // Generar el cuerpo del reporte
    char body[1024];
    int total_records = 0;
    snprintf(body, sizeof(body), "---------- Carga de usuarios ----------\n");
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "Fecha: %s\n\n", fecha);
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "Usuarios cargados:\n\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        snprintf(body + strlen(body), sizeof(body) - strlen(body), "Hilo %d: %d registros.\n", i, thread_args[i].processed_records);
        total_records += thread_args[i].processed_records;
    }
    // Add total number of records
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "\nTotal: %d registros.\n\n", total_records);
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "\nErrores:\n\n");

    // Generar el contenido completo
    char content[2048];
    snprintf(content, sizeof(content), "%s", body);

    generar_archivo(nombre_archivo, content);
    // free(content);
}

void reporte_transacciones(TransactionThreadArgs *thread_args) {
    char nombre_archivo[100];

    char *fecha = obtener_fecha_formateada();
    snprintf(nombre_archivo, sizeof(nombre_archivo),
             "operaciones_%s.log", fecha);


    int total_depositos = 0;
    int total_retiros = 0;
    int total_transferencias = 0;

    for (int i = 0; i < NUM_THREADS_2; i++) {
        total_depositos += thread_args[i].depositos;
        total_retiros += thread_args[i].retiros;
        total_transferencias += thread_args[i].transferencias;
    }

    int total = total_depositos + total_retiros + total_transferencias;

    char body[1024];
    snprintf(body, sizeof(body), "---------- Resumen de operaciones ----------\n");
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "Fecha: %s\n\n", fecha);
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "Operaciones realizadas:\n"
             "Depositos: %d\n"
             "Retiros: %d\n"
             "Transferencias: %d\n"
             "Total: %d\n\n",
                total_depositos, total_retiros, total_transferencias, total);
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "Operaciones por hilo:\n\n");
    for (int i = 0; i < NUM_THREADS_2; i++) {
        snprintf(body + strlen(body), sizeof(body) - strlen(body), "Hilo %d: %d\n", i, thread_args[i].depositos + thread_args[i].retiros + thread_args[i].transferencias);
    }
    snprintf(body + strlen(body), sizeof(body) - strlen(body), "\nErrores:\n\n");   
    generar_archivo(nombre_archivo, body);
}


void *cargar_usuarios_thread(void *args) {
    UserThreadArgs *thread_args = (UserThreadArgs *)args;
    cJSON *json = thread_args->json;
    int initial_index = thread_args->initial_index;
    int final_index = thread_args->final_index;

    for (int i = initial_index; i < final_index; i++) {
        cJSON *element = cJSON_GetArrayItem(json, i);
        if (!element) {
            continue;
        }

        cJSON *no_cuenta = cJSON_GetObjectItem(element, "no_cuenta");

        if (existe_cuenta(no_cuenta->valueint)) {
            printf("La cuenta %d ya existe.\n", no_cuenta->valueint);
            continue;
        }

        cJSON *nombre = cJSON_GetObjectItem(element, "nombre");
        cJSON *saldo = cJSON_GetObjectItem(element, "saldo");

        if (!no_cuenta || !nombre || !saldo) {
            continue;
        }

        agregar_cuenta(no_cuenta->valueint, nombre->valuestring, saldo->valuedouble);
        thread_args->processed_records++;
    }
}

void cargar_usuarios(const char *filename) {
    // Abrir el archivo JSON
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo.\n");
        return;
    }

    // Obtener el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Leer el contenido del archivo
    char *json_content = (char *)malloc(file_size + 1);
    fread(json_content, 1, file_size, file);
    json_content[file_size] = '\0';

    // Cerrar el archivo
    fclose(file);

    // Analizar el JSON
    cJSON *json = cJSON_Parse(json_content);
    if (!json) {
        fprintf(stderr, "Error al analizar el JSON.\n");
        free(json_content);
        return;
    }

    // Check if it's a valid JSON array
    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        return;
    }

    // Contar el número de elementos en el arreglo
    int num_elements = cJSON_GetArraySize(json);

    // Crear los hilos
    pthread_t threads[NUM_THREADS];
    UserThreadArgs thread_args[NUM_THREADS];
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].json = json;
        thread_args[i].initial_index = i * num_elements / NUM_THREADS;
        thread_args[i].final_index = (i + 1) * num_elements / NUM_THREADS;
        thread_args[i].processed_records = 0;
        pthread_create(&threads[i], NULL, cargar_usuarios_thread, &thread_args[i]);
    }

    // Esperar a que los hilos terminen
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d: Procesados %d registros.\n", i, thread_args[i].processed_records);
    }

    reporte_usuarios(thread_args);

    // Liberar la memoria y destruir el semáforo
    cJSON_Delete(json);
    free(json_content);
}

void deposito_transaccion(int no_cuenta, double monto) {
    // Realizar el depósito
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == no_cuenta) {
            pthread_mutex_lock(&cuentas[i].mutex);
            cuentas[i].saldo += monto;
            pthread_mutex_unlock(&cuentas[i].mutex);
            break;
        }
    }
}

void deposito_individual(){

    // Obtener el número de cuenta
    int no_cuenta;
    printf("Número de cuenta: ");
    scanf("%d", &no_cuenta);

    // Verificar si la cuenta existe
    if (!existe_cuenta(no_cuenta)) {
        printf("La cuenta no existe.\n");
        return;
    }

    // Obtener el monto del depósito
    double monto;
    printf("Monto a depositar: ");
    scanf("%lf", &monto);

    // Validar el monto
    if (monto <= 0) {
        printf("Monto inválido. El monto debe ser mayor a cero.\n");
        return;
    }

    // Realizar el depósito
    deposito_transaccion(no_cuenta, monto);
   
    printf("Depósito realizado con éxito.\n");
}

int retiro_transaccion(int no_cuenta, double monto) {
    // Realizar el retiro
    int retiro_realizado = 0;
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == no_cuenta) {
            
            pthread_mutex_lock(&cuentas[i].mutex);           
            if (cuentas[i].saldo < monto) {
                pthread_mutex_unlock(&cuentas[i].mutex);
                break;
            } 

            cuentas[i].saldo -= monto;
            pthread_mutex_unlock(&cuentas[i].mutex);
            retiro_realizado = 1;
            break;
        }
    }
    return retiro_realizado;
}


void retiro_individual() {
    // Obtener el número de cuenta
    int no_cuenta;
    printf("Número de cuenta: ");
    scanf("%d", &no_cuenta);

    // Verificar si la cuenta existe
    if (!existe_cuenta(no_cuenta)) {
        printf("La cuenta no existe.\n");
        return;
    }

    // Obtener el monto del retiro
    double monto;
    printf("Monto a retirar: ");
    scanf("%lf", &monto);

    // Validar el monto
    if (monto <= 0) {
        printf("Monto inválido. El monto debe ser mayor a cero.\n");
        return;
    }

    // Realizar el retiro
    if (retiro_transaccion(no_cuenta, monto)) {
        printf("Retiro realizado con éxito.\n");
    } else {
        printf("Saldo insuficiente.\n");
    }
}

int transferencia_transaccion(int no_cuenta_origen, int no_cuenta_destino, double monto) {
    // Bloquear las cuentas
    int cuenta_origen_index = -1;
    int cuenta_destino_index = -1;
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == no_cuenta_origen) {
            cuenta_origen_index = i;
        }
        if (cuentas[i].no_cuenta == no_cuenta_destino) {
            cuenta_destino_index = i;
        }
    }

    if (cuenta_origen_index == -1 || cuenta_destino_index == -1) {
        return 0;
    }

    pthread_mutex_lock(&cuentas[cuenta_origen_index].mutex);
    pthread_mutex_lock(&cuentas[cuenta_destino_index].mutex);

    // Realizar la transferencia
    if (cuentas[cuenta_origen_index].saldo < monto) {
        pthread_mutex_unlock(&cuentas[cuenta_origen_index].mutex);
        pthread_mutex_unlock(&cuentas[cuenta_destino_index].mutex);
        return 0;
    }

    cuentas[cuenta_origen_index].saldo -= monto;
    cuentas[cuenta_destino_index].saldo += monto;

    // Desbloquear las cuentas
    pthread_mutex_unlock(&cuentas[cuenta_origen_index].mutex);
    pthread_mutex_unlock(&cuentas[cuenta_destino_index].mutex);

    return 1;
}

void transferencia_individual() {
    // Obtener el número de cuenta origen
    int no_cuenta_origen;
    printf("Número de cuenta origen: ");
    scanf("%d", &no_cuenta_origen);

    // Verificar si la cuenta origen existe
    if (!existe_cuenta(no_cuenta_origen)) {
        printf("La cuenta origen no existe.\n");
        return;
    }

    // Obtener el número de cuenta destino
    int no_cuenta_destino;
    printf("Número de cuenta destino: ");
    scanf("%d", &no_cuenta_destino);

    // Verificar si la cuenta destino existe
    if (!existe_cuenta(no_cuenta_destino)) {
        printf("La cuenta destino no existe.\n");
        return;
    }

    // Obtener el monto de la transferencia
    double monto;
    printf("Monto a transferir: ");
    scanf("%lf", &monto);

    // Validar el monto
    if (monto <= 0) {
        printf("Monto inválido. El monto debe ser mayor a cero.\n");
        return;
    }

    // Realizar la transferencia
    if (transferencia_transaccion(no_cuenta_origen, no_cuenta_destino, monto)) {
        printf("Transferencia realizada con éxito.\n");
    } else {
        printf("Saldo insuficiente.\n");
    }
}

void mostrar_cuenta(int no_cuenta) {
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == no_cuenta) {
            printf("Cuenta %d: %s, Saldo: %.2f\n", cuentas[i].no_cuenta, cuentas[i].nombre, cuentas[i].saldo);
            return;
        }
    }
    printf("La cuenta no existe.\n");
}

void *cargar_operaciones_masiva_thread(void *args) {
    TransactionThreadArgs *thread_args = (TransactionThreadArgs *)args;
    cJSON *json = thread_args->json;

    while (1) {
        pthread_mutex_lock(&cuentas_mutex);
        if (current_transaction >= cJSON_GetArraySize(json)) {
            pthread_mutex_unlock(&cuentas_mutex);
            break;
        }

        cJSON *element = cJSON_GetArrayItem(json, current_transaction);
        thread_args->operaciones++;
        current_transaction++;
        
        if (!element) {
            pthread_mutex_unlock(&cuentas_mutex);
            continue;
        }


        // 1 -> Deposito; 2 -> Retiro; 3 -> Transferencia
        cJSON *operacion = cJSON_GetObjectItem(element, "operacion");
        cJSON *cuenta1 = cJSON_GetObjectItem(element, "cuenta1");
        cJSON *cuenta2 = cJSON_GetObjectItem(element, "cuenta2");
        cJSON *monto = cJSON_GetObjectItem(element, "monto");

        if (!operacion) {
            pthread_mutex_unlock(&cuentas_mutex);
            continue;
        }


        // TODO: AGREGAR ERRORES
        if (operacion->valueint == 1) {
            // Validaciones de depósito
            if (!cuenta1 || !monto) {
                printf("Para deposito se necesita cuenta y monto\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }
            
            // existencia de cuenta
            if (!existe_cuenta(cuenta1->valueint)) {
                fprintf(stderr, "La cuenta %d no existe.\n", cuenta1->valueint);
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            // Validar el monto
            if (monto->valuedouble <= 0) {
                fprintf(stderr, "El monto debe ser mayor a cero.\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }           

            deposito_transaccion(cuenta1->valueint, monto->valuedouble);
            fprintf(stderr, "Deposito realizado con exito\n");
            thread_args->depositos++;
        } else if (operacion->valueint == 2) {
            // Validaciones de retiro
            if (!cuenta1 || !monto) {
                fprintf(stderr, "Para retiro se necesita cuenta y monto\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            // existencia de cuenta
            if (!existe_cuenta(cuenta1->valueint)) {
                fprintf(stderr, "La cuenta %d no existe.\n", cuenta1->valueint);
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            // Validar el monto
            if (monto->valuedouble <= 0) {
                fprintf(stderr, "El monto debe ser mayor a cero.\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            if (!retiro_transaccion(cuenta1->valueint, monto->valuedouble)){
                fprintf(stderr, "Saldo insuficiente\n");
            } else {
                fprintf(stderr, "Retiro realizado con exito\n");
                thread_args->retiros++;
            }
        } else if (operacion->valueint == 3) {

            // Validaciones de transferencia
            if (!cuenta1 || !cuenta2 || !monto) {
                fprintf(stderr, "Para transferencia se necesita cuenta1, cuenta2 y monto\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            // existencia de cuenta
            if (!existe_cuenta(cuenta1->valueint) || !existe_cuenta(cuenta2->valueint)) {
                fprintf(stderr, "La cuenta %d o %d no existe.\n", cuenta1->valueint, cuenta2->valueint);
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            // Validar el monto
            if (monto->valuedouble <= 0) {
                fprintf(stderr, "El monto debe ser mayor a cero.\n");
                pthread_mutex_unlock(&cuentas_mutex);
                continue;
            }

            if (!transferencia_transaccion(cuenta1->valueint, cuenta2->valueint, monto->valuedouble)){
                fprintf(stderr, "Saldo insuficiente\n");
            } else {
                fprintf(stderr, "Transferencia realizada con exito\n");
                thread_args->transferencias++;
            }
        }

        pthread_mutex_unlock(&cuentas_mutex);
    }
}

void cargar_operaciones_masiva(){
    char filename[100];
    printf("Nombre del archivo de operaciones: ");
    scanf("%s", filename);

    // Abrir el archivo JSON
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo.\n");
        return;
    }

    // Obtener el tamaño del archivo
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Leer el contenido del archivo
    char *json_content = (char *)malloc(file_size + 1);
    fread(json_content, 1, file_size, file);
    json_content[file_size] = '\0';

    // Cerrar el archivo
    fclose(file);

    // Analizar el JSON
    cJSON *json = cJSON_Parse(json_content);

    if (!json) {
        fprintf(stderr, "Error al analizar el JSON.\n");
        free(json_content);
        return;
    
    }

    // Check if it's a valid JSON array
    if (!cJSON_IsArray(json)) {
        perror("JSON is not Array");
        return;
    }

    // Vamos a trabajar de forma concurrente con 3 hilos
    pthread_t threads[NUM_THREADS_2];
    TransactionThreadArgs thread_args[NUM_THREADS_2];
    current_transaction = 0;

    // Crear los hilos
    for (int i = 0; i < NUM_THREADS_2; i++) {
        thread_args[i].json = json;
        thread_args[i].retiros = 0;
        thread_args[i].depositos = 0;
        thread_args[i].transferencias = 0;
        thread_args[i].operaciones = 0;
        pthread_create(&threads[i], NULL, cargar_operaciones_masiva_thread, &thread_args[i]);
    }

    // Esperar a que los hilos terminen
    for (int i = 0; i < NUM_THREADS_2; i++) {
        pthread_join(threads[i], NULL);
    }

    reporte_transacciones(thread_args);

    // Liberar la memoria y destruir el semáforo
    cJSON_Delete(json);
    free(json_content);
}

void generar_reporte_json(const char *filename, Cuenta *cuentas, int num_cuentas) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo %s.\n", filename);
        return;
    }

    // Comenzar a escribir el array JSON
    fprintf(file, "[\n");

    for (int i = 0; i < num_cuentas; i++) {
        fprintf(file, "  {\n");
        fprintf(file, "    \"no_cuenta\": %d,\n", cuentas[i].no_cuenta);
        fprintf(file, "    \"nombre\": \"%s\",\n", cuentas[i].nombre);
        fprintf(file, "    \"saldo\": %.2f\n", cuentas[i].saldo);

        // Coma después de cada objeto, excepto el último
        if (i < num_cuentas - 1) {
            fprintf(file, "  },\n");
        } else {
            fprintf(file, "  }\n");
        }
    }

    // Finalizar el array JSON
    fprintf(file, "]\n");

    fclose(file);
}



void menu() {
    int option;
    do {
        printf("Seleccione una opción:\n");
        printf("1. Cargar usuarios\n");
        printf("2. Deposito [Individual]\n");
        printf("3. Retiro [Individual]\n");
        printf("5. Transferencia [Individual]\n");
        printf("6. Consultar cuenta\n");
        printf("7. Carga masiva de operaciones\n");
        printf("8. Reporte de operaciones masivas\n");
        printf("9. Salir\n");
        printf("Opción: ");
        scanf("%d", &option);
        switch (option) {
            case 1:
                char filename[100];
                printf("Nombre del archivo de operaciones: ");
                scanf("%s", filename);
                cargar_usuarios(filename);
                break;
            case 2:
                deposito_individual();
                break;
            case 3:
                retiro_individual();
                break;
            case 5:
                transferencia_individual();
                break;
            case 6:
                int no_cuenta;
                printf("Número de cuenta: ");
                scanf("%d", &no_cuenta);
                mostrar_cuenta(no_cuenta);
                break;
            case 7:
                cargar_operaciones_masiva();
                break;
            case 8:
                // Generar el reporte en formato JSON
                generar_reporte_json("cuentas.json", cuentas, num_cuentas);
                break;
            case 9:
                break;
            default:
                printf("Opción inválida.\n");
                break;
        }
    } while (option != 9);
}


int main() {

    // char filename[100];
    // printf("Nombre del archivo de operaciones: ");
    // scanf("%s", filename);
    // cargar_usuarios(filename);
    
    // Ejecutar el menú
    menu();

    // Imprimir las cuentas
    for (int i = 0; i < num_cuentas; i++) {
        printf("Cuenta %d: %s, Saldo: %.2f\n", cuentas[i].no_cuenta, cuentas[i].nombre, cuentas[i].saldo);
    }

    // Liberar la memoria

    for (int i = 0; i < num_cuentas; i++) {
        pthread_mutex_destroy(&cuentas[i].mutex);
    }

    free(cuentas);
    pthread_mutex_destroy(&cuentas_mutex);

    return 0;
}
