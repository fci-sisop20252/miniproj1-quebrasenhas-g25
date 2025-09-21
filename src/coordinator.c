#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 *
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) total *= charset_len;
    return total;
}

void index_to_password(long long index, const char *charset, int charset_len,
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

int main(int argc, char *argv[]) {

    /* ==== Validação de Argumentos ==== */
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash_md5> <tamanho> <charset> <num_workers>\n", argv[0]);
        return 1;
    }

    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);

    if (password_len < 1 || password_len > 10) {
        fprintf(stderr, "Erro: tamanho da senha deve estar entre 1 e 10\n");
        return 1;
    }
    if (num_workers < 1 || num_workers > MAX_WORKERS) {
        fprintf(stderr, "Erro: numero de workers deve estar entre 1 e %d\n", MAX_WORKERS);
        return 1;
    }
    if (charset_len == 0) {
        fprintf(stderr, "Erro: charset não pode ser vazio\n");
        return 1;
    }

    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);

    /* ==== Preparação do Espaço de Busca ==== */
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);

    unlink(RESULT_FILE);  // remove resultado anterior
    time_t start_time = time(NULL);

    long long base = total_space / num_workers;
    long long resto = total_space % num_workers;

    pid_t workers[MAX_WORKERS];

    printf("Iniciando workers...\n");
    for (int i = 0; i < num_workers; i++) {
        long long start_index = i * base + (i < resto ? i : resto);
        long long end_index   = start_index + base - 1;
        if (i < resto) end_index++; // distribui o resto

        char start_password[password_len + 1];
        char end_password[password_len + 1];
        index_to_password(start_index, charset, charset_len, password_len, start_password);
        index_to_password(end_index,   charset, charset_len, password_len, end_password);

        pid_t pid = fork();
        if (pid < 0) {
            perror("Erro no fork");
            return 1;
        }
        else if (pid == 0) {
            // filho: executa o worker
            char len_arg[12], id_arg[12];
            snprintf(len_arg, sizeof(len_arg), "%d", password_len);
            snprintf(id_arg,  sizeof(id_arg),  "%d", i);
            execl("./worker", "worker", target_hash,
                  start_password, end_password,
                  charset, len_arg, id_arg, (char *)NULL);
            perror("Erro ao executar worker");
            exit(1);
        } else {
            workers[i] = pid;
            printf("Worker %d iniciado (PID %d) - intervalo: %s a %s\n",
                   i, pid, start_password, end_password);
        }
    }

    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");

    /* ==== Espera pelos Workers ==== */
    int finished = 0;
    while (finished < num_workers) {
        int status;
        pid_t done = wait(&status);
        if (done > 0) {
            finished++;
            if (WIFEXITED(status))
                printf("Worker PID %d terminou com código %d\n",
                       done, WEXITSTATUS(status));
            else
                printf("Worker PID %d terminou de forma anormal\n", done);
        }
    }

    time_t end_time = time(NULL);
    double elapsed = difftime(end_time, start_time);

    printf("\n=== Resultado ===\n");
    int fd = open(RESULT_FILE, O_RDONLY);
    if (fd >= 0) {
        char buffer[256];
        int n = read(fd, buffer, sizeof(buffer)-1);
        close(fd);
        if (n > 0) {
            buffer[n] = '\0';
            char *sep = strchr(buffer, ':');
            if (sep) {
                char *found = sep + 1;
                found[strcspn(found, "\n")] = '\0';
                char hash_check[33];
                md5_string(found, hash_check);
                if (strcmp(hash_check, target_hash) == 0) {
                    printf("Senha encontrada: %s\n", found);
                    printf("Tempo total: %.2f segundos\n", elapsed);
                } else {
                    printf("Arquivo encontrado, mas hash não confere.\n");
                }
            }
        }
    } else {
        printf("Nenhum worker encontrou a senha.\n");
    }

    return 0;
}
