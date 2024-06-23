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

    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), fp) != NULL) {
        
        char *ismmap = strtok(line, ",");
        char *pid = strtok(NULL, ",");
        char *execname = strtok(NULL, ",");
        char *gettimeofday_s = strtok(NULL, ",");
        char *length = strtok(NULL, ",");

        
        long pid_val = atol(pid);  
        long unix_time_val = atol(gettimeofday_s);  
        int is_mmap_val = atoi(ismmap);  

        
        char query[MAX_LINE_LENGTH];
        sprintf(query, "INSERT INTO memory_requests (pid, process_name, is_mmap, unix_time) VALUES (%ld, '%s', %d, %ld)",
                pid_val, execname, is_mmap_val, unix_time_val);

        
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
