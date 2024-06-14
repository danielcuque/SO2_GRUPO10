#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "cJSON.h"
#include <unistd.h> // Para la función sleep

#define NUM_THREADS 3

typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} MySemaphore;

// Inicializar el semáforo
void init_semaphore(MySemaphore *semaphore, int value) {
    semaphore->value = value;
    pthread_mutex_init(&semaphore->mutex, NULL);
    pthread_cond_init(&semaphore->condition, NULL);
}

// Decrementar el semáforo (esperar)
void wait_semaphore(MySemaphore *semaphore) {
    pthread_mutex_lock(&semaphore->mutex);
    while (semaphore->value <= 0) {
        pthread_cond_wait(&semaphore->condition, &semaphore->mutex);
    }
    semaphore->value--;
    pthread_mutex_unlock(&semaphore->mutex);
}

// Incrementar el semáforo (señalizar)
void signal_semaphore(MySemaphore *semaphore) {
    pthread_mutex_lock(&semaphore->mutex);
    semaphore->value++;
    pthread_cond_signal(&semaphore->condition);
    pthread_mutex_unlock(&semaphore->mutex);
}

// Estructura para pasar argumentos a los hilos
typedef struct {
    cJSON *json;
    int *verified_elements;
    MySemaphore *semaphore;
    int thread_id;
    int num_elements;
    int processed_records;
} ThreadArgs;

// Función que procesa una porción del JSON
void* process_json(void* args) {
    ThreadArgs *thread_args = (ThreadArgs*)args;

    cJSON *json = thread_args->json;
    int *verified_elements = thread_args->verified_elements;
    MySemaphore *semaphore = thread_args->semaphore;
    int thread_id = thread_args->thread_id;
    int num_elements = thread_args->num_elements;
    int *processed_records = &(thread_args->processed_records);

    cJSON *element;

    while (1) {
        wait_semaphore(semaphore); // Esperar a que haya un elemento disponible para procesar

        pthread_mutex_lock(&semaphore->mutex); // Bloquear el mutex para acceder a la lista de elementos verificados

        int i;
        for (i = 0; i < num_elements; i++) {
            if (!verified_elements[i]) {
                verified_elements[i] = 1; // Marcar el elemento como verificado
                pthread_mutex_unlock(&semaphore->mutex); // Liberar el mutex
                cJSON_ArrayForEach(element, json) {
                    if (i == 0) {
                        // TODO: save account in memory
                        printf("Thread %d - No. Cuenta: %d\n", thread_id, cJSON_GetObjectItemCaseSensitive(element, "no_cuenta")->valueint);
                        //printf("Thread %d - Nombre: %s\n", thread_id, cJSON_GetObjectItemCaseSensitive(element, "nombre")->valuestring);
                        //printf("Thread %d - Saldo: %.2f\n\n", thread_id, cJSON_GetObjectItemCaseSensitive(element, "saldo")->valuedouble);
                        (*processed_records)++;
                        break;
                    }
                    i--;
                }
                pthread_mutex_lock(&semaphore->mutex); // Bloquear el mutex antes de modificar la lista de elementos verificados
                break;
            }
        }

        pthread_mutex_unlock(&semaphore->mutex); // Liberar el mutex

        signal_semaphore(semaphore); // Incrementar el semáforo para indicar que se liberó un elemento

        // Salir del bucle si todos los elementos han sido verificados
        int all_verified = 1;
        for (i = 0; i < num_elements; i++) {
            if (!verified_elements[i]) {
                all_verified = 0;
                break;
            }
        }
        if (all_verified) {
            break;
        }
    }

    pthread_exit(NULL);
}

int main() {
    // Abrir el archivo JSON
    FILE *file = fopen("usuarios.json", "r");
    if (!file) {
        fprintf(stderr, "No se pudo abrir el archivo.\n");
        return 1;
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
        return 1;
    }

    // Check if it's a valid JSON array
    if(!cJSON_IsArray(json)){
        perror("JSON is not Array");
        return 1;
    }

    // Contar el número de elementos en el arreglo
    int num_elements = cJSON_GetArraySize(json);

    // Inicializar la lista de elementos verificados
    int *verified_elements = (int *)calloc(num_elements, sizeof(int));

    // Inicializar el semáforo
    MySemaphore semaphore;
    init_semaphore(&semaphore, num_elements);

    // Crear los hilos
    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].json = json;
        thread_args[i].verified_elements = verified_elements;
        thread_args[i].semaphore = &semaphore;
        thread_args[i].thread_id = i + 1;
        thread_args[i].num_elements = num_elements;
        thread_args[i].processed_records = 0;
        if (pthread_create(&threads[i], NULL, process_json, &thread_args[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Esperar a que los hilos terminen
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        printf("Thread %d: Procesados %d registros.\n", thread_args[i].thread_id, thread_args[i].processed_records);
    }

    // Liberar la memoria y destruir el semáforo
    cJSON_Delete(json);
    free(json_content);
    free(verified_elements);
    pthread_mutex_destroy(&semaphore.mutex);
    pthread_cond_destroy(&semaphore.condition);

    return 0;
}
