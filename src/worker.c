#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO TRABALHADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Uso: ./worker <hash_alvo> <senha_inicial> <senha_final> <charset> <tamanho> <worker_id>
 */

#define RESULT_FILE "password_found.txt"
#define PROGRESS_INTERVAL 100000  // Reportar progresso a cada N senhas

/**
 * Incrementa uma senha para a próxima na ordem lexicográfica (aaa -> aab -> aac...)
 *
 * @param password Senha atual (será modificada)
 * @param charset Conjunto de caracteres permitidos
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @return 1 se incrementou com sucesso, 0 se chegou ao limite (overflow)
 */
int increment_password(char *password, const char *charset, int charset_len, int password_len) {
    for (int i = password_len - 1; i >= 0; i--) {
        // Encontrar índice do caractere atual no charset
        int pos = 0;
        while (pos < charset_len && charset[pos] != password[i]) pos++;

        if (pos == charset_len - 1) {
            // Overflow nesta posição: reinicia e vai para a próxima à esquerda
            password[i] = charset[0];
        } else {
            // Incrementa e termina
            password[i] = charset[pos + 1];
            return 1;
        }
    }
    // Se chegou aqui, estourou todas as posições
    return 0;
}

/**
 * Compara duas senhas lexicograficamente
 * 
 * @return -1 se a < b, 0 se a == b, 1 se a > b
 */
int password_compare(const char *a, const char *b) {
    return strcmp(a, b);
}

/**
 * Verifica se o arquivo de resultado já existe
 * Usado para parada antecipada se outro worker já encontrou a senha
 */
int check_result_exists() {
    return access(RESULT_FILE, F_OK) == 0;
}

/**
 * Salva a senha encontrada no arquivo de resultado
 * Usa O_CREAT | O_EXCL para garantir escrita atômica (apenas um worker escreve)
 */
void save_result(int worker_id, const char *password) {
    int fd = open(RESULT_FILE, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd >= 0) {
        char buffer[128];
        int n = snprintf(buffer, sizeof(buffer), "%d:%s\n", worker_id, password);
        write(fd, buffer, n);
        close(fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Uso interno: %s <hash> <start> <end> <charset> <len> <id>\n", argv[0]);
        return 1;
    }

    const char *target_hash   = argv[1];
    char *start_password      = argv[2];
    const char *end_password  = argv[3];
    const char *charset       = argv[4];
    int password_len          = atoi(argv[5]);
    int worker_id             = atoi(argv[6]);
    int charset_len           = strlen(charset);

    printf("[Worker %d] Iniciado: %s até %s\n", worker_id, start_password, end_password);

    char current_password[32]; // buffer para senha atual
    strcpy(current_password, start_password);

    char computed_hash[33];    // buffer para hash calculado
    long long passwords_checked = 0;

    time_t start_time = time(NULL);

    while (1) {
        // Verifica se outro worker já encontrou a senha
        if (passwords_checked % PROGRESS_INTERVAL == 0 && check_result_exists()) {
            printf("[Worker %d] Outro worker encontrou a senha. Encerrando.\n", worker_id);
            break;
        }

        // Calcula o hash MD5 da senha atual
        md5_string(current_password, computed_hash);

        // Compara com o hash alvo
        if (strcmp(computed_hash, target_hash) == 0) {
            save_result(worker_id, current_password);
            printf("[Worker %d] Senha encontrada: %s\n", worker_id, current_password);
            break;
        }

        // Verifica se chegou ao fim do intervalo
        if (!increment_password(current_password, charset, charset_len, password_len)) {
            break;
        }
        if (password_compare(current_password, end_password) > 0) {
            break;
        }

        passwords_checked++;
    }

    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);

    printf("[Worker %d] Finalizado. Total: %lld senhas em %.2f segundos",
           worker_id, passwords_checked, total_time);
    if (total_time > 0)
        printf(" (%.0f senhas/s)", passwords_checked / total_time);
    printf("\n");

    return 0;
}
