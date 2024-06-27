#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>  

#define MAX_LINE_LENGTH 256

int main() {
    const char *command = "sudo stap memory_requests.stp";

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error abriendo el proceso con sudo");
        return 1;
    }

    
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    const char *server = "roundhouse.proxy.rlwy.net";
    const char *user = "root";
    const char *password = "fIfkYvtSvlmpSptMUpqxrmtBemKzoizk";
    const char *database = "so2p1";
    unsigned int port = 37220;

    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "Error al inicializar la conexión: %s\n", mysql_error(conn));
        return 1;
    }

    if (mysql_real_connect(conn, server, user, password, database, port, NULL, 0) == NULL) {
        fprintf(stderr, "Error al conectar con la base de datos: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    // Erase all the records in the tables
    if (mysql_query(conn, "DELETE FROM memory_requests")) {
        fprintf(stderr, "Error al ejecutar la consulta: %s\n", mysql_error(conn));
    }else{
        printf("Se eliminaron correctamente los registros\n");
    }

    if (mysql_query(conn, "DELETE FROM virtual_memory")) {
        fprintf(stderr, "Error al ejecutar la consulta: %s\n", mysql_error(conn));
    }else{
        printf("Se eliminaron correctamente los registros\n");
    }

    // Get the total virtual memory with free -t and insert it into the database
    const char *command2 = "free -t | grep Inter | awk '{print $2}'";
    FILE *fp2 = popen(command2, "r");
    if (fp2 == NULL) {
        perror("Error abriendo el proceso con free");
        return 1;
    }

    char line2[MAX_LINE_LENGTH];
    fgets(line2, sizeof(line2), fp2);
    long total_virtual_memory = atol(line2);

    char query2[MAX_LINE_LENGTH];
    sprintf(query2, "INSERT INTO virtual_memory (amount) VALUES (%ld)", total_virtual_memory);

    if (mysql_query(conn, query2)) {
        fprintf(stderr, "Error al ejecutar la consulta: %s\n", mysql_error(conn));
    }else{
        printf("Se insertó correctamente el registro\n");
    }
    pclose(fp2);


    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), fp) != NULL) {
        
        char *ismmap = strtok(line, ",");
        char *pid = strtok(NULL, ",");
        char *execname = strtok(NULL, ",");
        char *gettimeofday_s = strtok(NULL, ",");
        char *length = strtok(NULL, ",");

        // only if all exists
        if(ismmap == NULL || pid == NULL || execname == NULL || gettimeofday_s == NULL || length == NULL){
            continue;
        }

        long pid_val = atol(pid);  
        long unix_time_val = atol(gettimeofday_s);  
        long size = atol(length);

        int is_mmap_val = atoi(ismmap);  

        
        char query[MAX_LINE_LENGTH];
        sprintf(query, "INSERT INTO memory_requests (pid, process_name, is_mmap, unix_time, size) VALUES (%ld, '%s', %d, %ld, %ld)",
                pid_val, execname, is_mmap_val, unix_time_val, size);

        
        if (mysql_query(conn, query)) {
            fprintf(stderr, "Error al ejecutar la consulta: %s\n", mysql_error(conn));
        }else{
            printf("Se insertó correctamente el registro\n");
        }
    }

    
    mysql_close(conn);
    pclose(fp);

    return 0;
}
