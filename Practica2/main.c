#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include "cJSON.h"
#include <unistd.h> // Para la función sleep

#define NUM_THREADS 4

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
} ThreadArgs;


Cuenta *cuentas = NULL;
int num_cuentas = 0;
pthread_mutex_t cuentas_mutex = PTHREAD_MUTEX_INITIALIZER;


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
    pthread_mutex_lock(&cuentas_mutex);
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == num) {
            pthread_mutex_unlock(&cuentas_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&cuentas_mutex);
    return 0;
}

void *cargar_usuarios_thread(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    cJSON *json = thread_args->json;
    int initial_index = thread_args->initial_index;
    int final_index = thread_args->final_index;

    for (int i = initial_index; i < final_index; i++) {
        cJSON *element = cJSON_GetArrayItem(json, i);
        if (!element) {
            continue;
        }

        cJSON *no_cuenta = cJSON_GetObjectItem(element, "no_cuenta");
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
    ThreadArgs thread_args[NUM_THREADS];
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

    // Liberar la memoria y destruir el semáforo
    cJSON_Delete(json);
    free(json_content);
}


int main() {

    char filename[100];
    printf("Nombre del archivo de operaciones: ");
    scanf("%s", filename);
    cargar_usuarios(filename);
    
    // Ejecutar el menú
    // menu();

    // Imprimir las cuentas
    for (int i = 0; i < num_cuentas; i++) {
        printf("Cuenta %d: %s, Saldo: %.2f\n", cuentas[i].no_cuenta, cuentas[i].nombre, cuentas[i].saldo);
    }

    // Liberar la memoria
    free(cuentas);
    pthread_mutex_destroy(&cuentas_mutex);

    return 0;
}
