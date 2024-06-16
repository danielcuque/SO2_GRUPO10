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
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
} MySemaphore;

typedef struct {
    cJSON *json;
    MySemaphore *semaphore;
    int thread_id;
    int num_elements;
    int processed_records;
} ThreadArgs;

Cuenta *cuentas;
int num_cuentas = 0;
pthread_mutex_t cuentas_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_semaphore(MySemaphore *semaphore, int value) {
    semaphore->value = value;
    pthread_mutex_init(&semaphore->mutex, NULL);
    pthread_cond_init(&semaphore->condition, NULL);
}

void wait_semaphore(MySemaphore *semaphore) {
    pthread_mutex_lock(&semaphore->mutex);
    while (semaphore->value <= 0) {
        pthread_cond_wait(&semaphore->condition, &semaphore->mutex);
    }
    semaphore->value--;
    pthread_mutex_unlock(&semaphore->mutex);
}

void signal_semaphore(MySemaphore *semaphore) {
    pthread_mutex_lock(&semaphore->mutex);
    semaphore->value++;
    pthread_cond_signal(&semaphore->condition);
    pthread_mutex_unlock(&semaphore->mutex);
}

int buscar_cuenta(int no_cuenta) {
    for (int i = 0; i < num_cuentas; i++) {
        if (cuentas[i].no_cuenta == no_cuenta) {
            return i;
        }
    }
    return -1;
}

int deposito(int no_cuenta, double monto) {
    pthread_mutex_lock(&cuentas_mutex);
    int idx = buscar_cuenta(no_cuenta);
    if (idx == -1) {
        pthread_mutex_unlock(&cuentas_mutex);
        return -1;
    }
    pthread_mutex_lock(&cuentas[idx].mutex);
    pthread_mutex_unlock(&cuentas_mutex);

    if (monto < 0) {
        pthread_mutex_unlock(&cuentas[idx].mutex);
        return -2;
    }

    cuentas[idx].saldo += monto;
    pthread_mutex_unlock(&cuentas[idx].mutex);
    return 0;
}

int retiro(int no_cuenta, double monto) {
    pthread_mutex_lock(&cuentas_mutex);
    int idx = buscar_cuenta(no_cuenta);
    if (idx == -1) {
        pthread_mutex_unlock(&cuentas_mutex);
        return -1;
    }
    pthread_mutex_lock(&cuentas[idx].mutex);
    pthread_mutex_unlock(&cuentas_mutex);

    if (monto < 0 || cuentas[idx].saldo < monto) {
        pthread_mutex_unlock(&cuentas[idx].mutex);
        return -2;
    }

    cuentas[idx].saldo -= monto;
    pthread_mutex_unlock(&cuentas[idx].mutex);
    return 0;
}

int transferencia(int cuenta_origen, int cuenta_destino, double monto) {
    if (retiro(cuenta_origen, monto) == 0) {
        if (deposito(cuenta_destino, monto) == 0) {
            return 0;
        } else {
            deposito(cuenta_origen, monto); // Rollback
        }
    }
    return -1;
}

void consultar_cuenta(int no_cuenta) {
    pthread_mutex_lock(&cuentas_mutex);
    int idx = buscar_cuenta(no_cuenta);
    if (idx != -1) {
        pthread_mutex_lock(&cuentas[idx].mutex);
        printf("Cuenta: %d\nNombre: %s\nSaldo: %.2f\n", cuentas[idx].no_cuenta, cuentas[idx].nombre, cuentas[idx].saldo);
        pthread_mutex_unlock(&cuentas[idx].mutex);
    } else {
        printf("Cuenta no encontrada.\n");
    }
    pthread_mutex_unlock(&cuentas_mutex);
}

void *process_json(void *args) {
    ThreadArgs *thread_args = (ThreadArgs *)args;
    cJSON *json = thread_args->json;
    MySemaphore *semaphore = thread_args->semaphore;
    int thread_id = thread_args->thread_id;
    int num_elements = thread_args->num_elements;
    int *processed_records = &(thread_args->processed_records);

    cJSON *element;

    while (1) {
        wait_semaphore(semaphore); // Esperar a que haya un elemento disponible para procesar

        pthread_mutex_lock(&semaphore->mutex); // Bloquear el mutex para acceder a la lista de elementos verificados

        cJSON_ArrayForEach(element, json) {
            int operacion = cJSON_GetObjectItemCaseSensitive(element, "operacion")->valueint;
            int cuenta1 = cJSON_GetObjectItemCaseSensitive(element, "cuenta1")->valueint;
            int cuenta2 = cJSON_GetObjectItemCaseSensitive(element, "cuenta2") ? cJSON_GetObjectItemCaseSensitive(element, "cuenta2")->valueint : -1;
            double monto = cJSON_GetObjectItemCaseSensitive(element, "monto")->valuedouble;

            int resultado = -1;
            switch (operacion) {
                case 1:
                    resultado = deposito(cuenta1, monto);
                    break;
                case 2:
                    resultado = retiro(cuenta1, monto);
                    break;
                case 3:
                    resultado = transferencia(cuenta1, cuenta2, monto);
                    break;
            }

            if (resultado == 0) {
                (*processed_records)++;
            } else {
                // Manejo de errores (agregar a un reporte, etc.)
            }
        }

        pthread_mutex_unlock(&semaphore->mutex); // Liberar el mutex

        signal_semaphore(semaphore); // Incrementar el semáforo para indicar que se liberó un elemento

        // Salir del bucle si todos los elementos han sido verificados
        if (*processed_records >= num_elements) {
            break;
        }
    }

    pthread_exit(NULL);
}

void cargar_operaciones(const char *filename) {
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

    // Inicializar el semáforo
    MySemaphore semaphore;
    init_semaphore(&semaphore, num_elements);

    // Crear los hilos
    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];
    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        thread_args[i].json = json;
        thread_args[i].semaphore = &semaphore;
        thread_args[i].thread_id = i + 1;
        thread_args[i].num_elements = num_elements;
        thread_args[i].processed_records = 0;
        if (pthread_create(&threads[i], NULL, process_json, &thread_args[i]) != 0) {
            perror("Failed to create thread");
            return;
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
    pthread_mutex_destroy(&semaphore.mutex);
    pthread_cond_destroy(&semaphore.condition);
}

void generar_reporte_cuentas(const char *filename) {
    cJSON *reporte_json = cJSON_CreateArray();
    pthread_mutex_lock(&cuentas_mutex);
    for (int i = 0; i < num_cuentas; i++) {
        cJSON *cuenta_json = cJSON_CreateObject();
        pthread_mutex_lock(&cuentas[i].mutex);
        cJSON_AddNumberToObject(cuenta_json, "no_cuenta", cuentas[i].no_cuenta);
        cJSON_AddStringToObject(cuenta_json, "nombre", cuentas[i].nombre);
        cJSON_AddNumberToObject(cuenta_json, "saldo", cuentas[i].saldo);
        pthread_mutex_unlock(&cuentas[i].mutex);
        cJSON_AddItemToArray(reporte_json, cuenta_json);
    }
    pthread_mutex_unlock(&cuentas_mutex);

    char *reporte_string = cJSON_Print(reporte_json);
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "%s", reporte_string);
        fclose(file);
    }

    free(reporte_string);
    cJSON_Delete(reporte_json);
}

void menu() {
    int opcion;
    while (1) {
        printf("Menu:\n1. Deposito\n2. Retiro\n3. Transferencia\n4. Consultar cuenta\n5. Cargar operaciones\n6. Generar reporte\n7. Salir\n");
        scanf("%d", &opcion);

        int no_cuenta, cuenta_origen, cuenta_destino;
        double monto;
        char filename[100];

        switch (opcion) {
            case 1:
                printf("Numero de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a depositar: ");
                scanf("%lf", &monto);
                if (deposito(no_cuenta, monto) == 0) {
                    printf("Deposito exitoso.\n");
                } else {
                    printf("Error en el deposito.\n");
                }
                break;
            case 2:
                printf("Numero de cuenta: ");
                scanf("%d", &no_cuenta);
                printf("Monto a retirar: ");
                scanf("%lf", &monto);
                if (retiro(no_cuenta, monto) == 0) {
                    printf("Retiro exitoso.\n");
                } else {
                    printf("Error en el retiro.\n");
                }
                break;
            case 3:
                printf("Numero de cuenta origen: ");
                scanf("%d", &cuenta_origen);
                printf("Numero de cuenta destino: ");
                scanf("%d", &cuenta_destino);
                printf("Monto a transferir: ");
                scanf("%lf", &monto);
                if (transferencia(cuenta_origen, cuenta_destino, monto) == 0) {
                    printf("Transferencia exitosa.\n");
                } else {
                    printf("Error en la transferencia.\n");
                }
                break;
            case 4:
                printf("Numero de cuenta: ");
                scanf("%d", &no_cuenta);
                consultar_cuenta(no_cuenta);
                break;
            case 5:
                printf("Nombre del archivo de operaciones: ");
                scanf("%s", filename);
                cargar_operaciones(filename);
                break;
            case 6:
                printf("Nombre del archivo de reporte: ");
                scanf("%s", filename);
                generar_reporte_cuentas(filename);
                break;
            case 7:
                exit(0);
            default:
                printf("Opcion no valida.\n");
        }
    }
}

int main() {
    // Inicialización de cuentas (por simplicidad se crean algunas cuentas de ejemplo)
    num_cuentas = 3;
    cuentas = (Cuenta *)malloc(num_cuentas * sizeof(Cuenta));

    cuentas[0].no_cuenta = 123;
    strcpy(cuentas[0].nombre, "Juan Perez");
    cuentas[0].saldo = 1000.0;
    pthread_mutex_init(&cuentas[0].mutex, NULL);

    cuentas[1].no_cuenta = 456;
    strcpy(cuentas[1].nombre, "Maria Gomez");
    cuentas[1].saldo = 2000.0;
    pthread_mutex_init(&cuentas[1].mutex, NULL);

    cuentas[2].no_cuenta = 789;
    strcpy(cuentas[2].nombre, "Carlos Ruiz");
    cuentas[2].saldo = 1500.0;
    pthread_mutex_init(&cuentas[2].mutex, NULL);

    // Ejecutar el menú
    menu();

    // Liberar recursos
    for (int i = 0; i < num_cuentas; i++) {
        pthread_mutex_destroy(&cuentas[i].mutex);
    }
    free(cuentas);
    pthread_mutex_destroy(&cuentas_mutex);

    return 0;
}
