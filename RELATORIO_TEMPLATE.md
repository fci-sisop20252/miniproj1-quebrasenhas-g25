# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** Nome (David Rodrigues de Oliveira), Nome (10410867),,,  
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**
Cada worker recebe um intervalo de índices correspondente a senhas lexicograficamente ordenadas. Caso haja resto da divisão, os primeiros workers recebem uma senha extra.

[Explique seu algoritmo de divisão]

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
// Cole seu código de divisão aqui
long long passwords_per_worker = total_space / num_workers;
long long remaining = total_space % num_workers;

long long start_index = 0;
for (int i = 0; i < num_workers; i++) {
    long long end_index = start_index + passwords_per_worker - 1;
    if (i < remaining) {
        end_index++;  // Distribui o resto
    }
    // Conversão de índices para senhas
    index_to_password(start_index, charset, charset_len, password_len, start_password);
    index_to_password(end_index, charset, charset_len, password_len, end_password);

    start_index = end_index + 1;
}
```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

[Explique em um parágrafo como você criou os processos, passou argumentos e esperou pela conclusão]
Para criar workers, usamos fork(). O processo filho usa execl() para executar o worker com seus argumentos específicos (hash alvo, intervalo de senhas, charset, tamanho e id). O processo pai armazena os PIDs e, depois, usa wait() em loop para aguardar cada worker terminar

**Código do fork/exec:**
```c
// Cole aqui seu loop de criação de workers
for (int i = 0; i < num_workers; i++) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("Erro no fork");
        exit(1);
    } else if (pid == 0) {
        // Processo filho
        execl("./worker", "./worker", target_hash, start_password, end_password, charset,
              argv[2], argv[i+1], (char *)NULL);
        perror("Erro no execl");
        exit(1);
    } else {
        // Processo pai
        workers[i] = pid;
    }
}

// Espera todos os workers terminarem
for (int i = 0; i < num_workers; i++) {
    int status;
    waitpid(workers[i], &status, 0);
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**
Uso da abertura de arquivo com O_CREAT | O_EXCL | O_WRONLY no worker. Isso garante que apenas o primeiro worker que encontrar a senha consegue criar o arquivo password_found.txt

[Explique como você implementou uma escrita atômica e como isso evita condições de corrida]
Leia sobre condições de corrida (aqui)[https://pt.stackoverflow.com/questions/159342/o-que-%C3%A9-uma-condi%C3%A7%C3%A3o-de-corrida]

**Como o coordinator consegue ler o resultado?**

[Explique como o coordinator lê o arquivo de resultado e faz o parse da informação]
O coordinator, após a conclusão de todos os workers, lê o arquivo password_found.txt, faz parse do formato worker_id:password e pode verificar o hash usando md5_string() para garantir que é a senha correta.

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | __0.45s | __0.25 | ___0.15s | ___3.0 |
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | ___1.8s | ___1.0s | ___0.6s | ___3.0|

**O speedup foi linear? Por quê?**
[Analise se dobrar workers realmente dobrou a velocidade e explique o overhead de criar processos]

O speedup não foi exatamente linear. Criar processos tem overhead, e a comunicação mínima de verificação do arquivo de resultado também adiciona atraso
---

## 5. Desafios e Aprendizados
**Qual foi o maior desafio técnico que você enfrentou?**
O maior desafio foi implementar o incremento de senha corretamente para percorrer todo o espaço de busca.
[Descreva um problema e como resolveu. Ex: "Tive dificuldade com o incremento de senha, mas resolvi tratando-o como um contador em base variável"]

---

## Comandos de Teste Utilizados

```bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4
```
---

**Checklist de Entrega:**
- [ ] Código compila sem erros
- [ ] Todos os TODOs foram implementados
- [ ] Testes passam no `./tests/simple_test.sh`
- [ ] Relatório preenchido
