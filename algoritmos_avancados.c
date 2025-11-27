#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 101
#define MAX_INPUT 128

/* ---------- Estruturas ---------- */

/* Árvore de cômodos (mansão) */
typedef struct Sala {
    char *nome;
    struct Sala *esq;
    struct Sala *dir;
} Sala;

/* BST para pistas coletadas */
typedef struct PistaNode {
    char *pista;
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Nó para tabela hash (encadeamento) */
typedef struct HashNode {
    char *pista;
    char *suspeito;
    struct HashNode *next;
} HashNode;

/* A tabela hash é apenas um array de ponteiros para HashNode */
typedef struct HashTable {
    HashNode *buckets[HASH_SIZE];
} HashTable;

/* ---------- Protótipos ---------- */

/* Criar sala (alocação dinâmica) */
Sala* criarSala(const char *nome);

/* Explorar salas (navegação interativa) */
void explorarSalas(Sala *raiz, PistaNode **pistasColetadas, HashTable *hash);

/* Inserir pista na BST (wrapper e função real) */
void inserirPista(PistaNode **raiz, const char *pista);
PistaNode* adicionarPista(PistaNode *raiz, const char *pista);

/* Imprimir pistas coletadas em ordem (in-order) */
void imprimirPistasInOrder(PistaNode *raiz);

/* Funções de hash: inserir e procurar */
unsigned int hashString(const char *s);
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito);
char* encontrarSuspeito(HashTable *ht, const char *pista);

/* Verificar suspeito final: checa se pelo menos duas pistas apontam para o acusado */
int verificarSuspeitoFinal(PistaNode *pistasColetadas, HashTable *ht, const char *acusado);

/* Utilitárias */
char* obterPistaPorSala(const char *nomeSala);
int pistaJaColetada(PistaNode *raiz, const char *pista);
void liberarPistas(PistaNode *raiz);
void liberarSala(Sala *s);
void liberarHash(HashTable *ht);

/* ---------- Implementações ---------- */

/*
 criarSala()
 Cria dinamicamente um nó de sala com o nome informado.
 Retorna ponteiro para a sala criada.
*/
Sala* criarSala(const char *nome) {
    Sala *s = (Sala*)malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro de alocacao ao criar sala '%s'\n", nome);
        exit(EXIT_FAILURE);
    }
    s->nome = strdup(nome);
    s->esq = s->dir = NULL;
    return s;
}

/*
 explorarSalas()
 Navega pela árvore de salas de forma interativa.
 Ao visitar uma sala, identifica e exibe uma pista (se houver) e tenta armazená-la
 na árvore BST de pistas coletadas.
 Parâmetros:
  - raiz: sala inicial (ponto onde o jogador começa)
  - pistasColetadas: endereço do ponteiro para a BST de pistas (modifica-se globalmente)
  - hash: tabela hash com as associações pista->suspeito (apenas leitura aqui)
*/
void explorarSalas(Sala *raiz, PistaNode **pistasColetadas, HashTable *hash) {
    if (!raiz) return;

    Sala *atual = raiz;
    char entrada[MAX_INPUT];

    printf("Bem-vindo à mansão! Explore e colete pistas.\n");
    printf("Controles: esquerda (e), direita (d), sair da exploração (s)\n");

    while (1) {
        printf("\nVoce esta na sala: %s\n", atual->nome);

        /* Ao visitar, mostrar pista (se existir) e coletar */
        char *pista = obterPistaPorSala(atual->nome);
        if (pista) {
            if (!pistaJaColetada(*pistasColetadas, pista)) {
                printf("Você encontrou uma pista: \"%s\". Ela foi adicionada ao seu caderno.\n", pista);
                inserirPista(pistasColetadas, pista);
            } else {
                printf("Você já tem a pista desta sala: \"%s\" (não duplicada).\n", pista);
            }
        } else {
            printf("Nenhuma pista encontrada nesta sala.\n");
        }

        /* Opcões de navegação */
        printf("Para onde quer ir? (e=esquerda, d=direita, s=sair): ");
        if (!fgets(entrada, sizeof(entrada), stdin)) {
            clearerr(stdin);
            continue;
        }

        /* pegar primeira letra válida */
        char cmd = '\0';
        for (int i = 0; entrada[i] != '\0'; ++i) {
            if (!isspace((unsigned char)entrada[i])) { cmd = tolower((unsigned char)entrada[i]); break; }
        }
        if (cmd == 's') {
            printf("Saindo da exploracao...\n");
            break;
        } else if (cmd == 'e') {
            if (atual->esq) {
                atual = atual->esq;
            } else {
                printf("Nao ha sala a esquerda. Permanece em %s.\n", atual->nome);
            }
        } else if (cmd == 'd') {
            if (atual->dir) {
                atual = atual->dir;
            } else {
                printf("Nao ha sala a direita. Permanece em %s.\n", atual->nome);
            }
        } else {
            printf("Comando desconhecido. Use 'e', 'd' ou 's'.\n");
        }
    }

    /* Ao terminar, mostrar as pistas coletadas */
    printf("\nPistas coletadas (em ordem):\n");
    if (!*pistasColetadas) {
        printf("  (nenhuma pista coletada)\n");
    } else {
        imprimirPistasInOrder(*pistasColetadas);
    }
}

/*
 adicionarPista()
 Insere uma pista (string) na BST de pistas. Retorna nova raiz após inserção.
 Caso a pista já exista (string idêntica), não insere duplicata.
*/
PistaNode* adicionarPista(PistaNode *raiz, const char *pista) {
    if (!raiz) {
        PistaNode *n = (PistaNode*)malloc(sizeof(PistaNode));
        n->pista = strdup(pista);
        n->esq = n->dir = NULL;
        return n;
    }
    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) {
        /* já existe */
        return raiz;
    } else if (cmp < 0) {
        raiz->esq = adicionarPista(raiz->esq, pista);
    } else {
        raiz->dir = adicionarPista(raiz->dir, pista);
    }
    return raiz;
}

/*
 inserirPista()
 Wrapper pedido nos requisitos para inserir uma pista na árvore de pistas.
*/
void inserirPista(PistaNode **raiz, const char *pista) {
    if (!raiz) return;
    *raiz = adicionarPista(*raiz, pista);
}

/* Imprime as pistas (in-order) */
void imprimirPistasInOrder(PistaNode *raiz) {
    if (!raiz) return;
    imprimirPistasInOrder(raiz->esq);
    printf("  - %s\n", raiz->pista);
    imprimirPistasInOrder(raiz->dir);
}

/* Hash simples - djb2-like */
unsigned int hashString(const char *s) {
    unsigned long hash = 5381;
    int c;
    while ((c = *s++))
        hash = ((hash << 5) + hash) + (unsigned char)c; /* hash * 33 + c */
    return (unsigned int)(hash % HASH_SIZE);
}

/*
 inserirNaHash()
 Insere a associacao pista -> suspeito na tabela hash.
 Se a pista ja existe, atualiza o suspeito (substitui).
*/
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!ht || !pista || !suspeito) return;
    unsigned int idx = hashString(pista);
    HashNode *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            /* atualiza suspeito */
            free(cur->suspeito);
            cur->suspeito = strdup(suspeito);
            return;
        }
        cur = cur->next;
    }
    /* nao encontrado -> inserir no inicio */
    HashNode *n = (HashNode*)malloc(sizeof(HashNode));
    n->pista = strdup(pista);
    n->suspeito = strdup(suspeito);
    n->next = ht->buckets[idx];
    ht->buckets[idx] = n;
}

/*
 encontrarSuspeito()
 Consulta a tabela hash para retornar o suspeito associado à pista.
 Retorna NULL se não existir associação.
*/
char* encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!ht || !pista) return NULL;
    unsigned int idx = hashString(pista);
    HashNode *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            return cur->suspeito;
        }
        cur = cur->next;
    }
    return NULL;
}

/*
 verificarSuspeitoFinal()
 Conta quantas pistas coletadas apontam para 'acusado' usando a tabela hash.
 Retorna o numero de pistas que apontam para o acusado.
*/
int verificarSuspeitoFinal(PistaNode *pistasColetadas, HashTable *ht, const char *acusado) {
    if (!acusado) return 0;

    /* travessia em ordem e contagem */
    int contador = 0;
    if (!pistasColetadas) return 0;

    /* usar uma pilha recursiva (in-order traversal recursivo) */
    if (pistasColetadas->esq)
        contador += verificarSuspeitoFinal(pistasColetadas->esq, ht, acusado);

    char *sus = encontrarSuspeito(ht, pistasColetadas->pista);
    if (sus && strcmp(sus, acusado) == 0) {
        contador++;
    }

    if (pistasColetadas->dir)
        contador += verificarSuspeitoFinal(pistasColetadas->dir, ht, acusado);

    return contador;
}

/* Retorna a pista associada a uma sala (regras codificadas) */
char* obterPistaPorSala(const char *nomeSala) {
    if (!nomeSala) return NULL;
    /* Define regras aqui */
    if (strcmp(nomeSala, "Hall") == 0) return "pegada de lama na soleira";
    if (strcmp(nomeSala, "Library") == 0) return "marca de dedo no livro raro";
    if (strcmp(nomeSala, "Dining Room") == 0) return "taça quebrada com resquicios";
    if (strcmp(nomeSala, "Kitchen") == 0) return "fio de tecido preso no cortador";
    if (strcmp(nomeSala, "Study") == 0) return "bilhete rasgado com iniciais";
    if (strcmp(nomeSala, "Basement") == 0) return "mancha de tinta fresca";
    if (strcmp(nomeSala, "Ballroom") == 0) return "programa de concerto dobrado";
    if (strcmp(nomeSala, "Conservatory") == 0) return "folha de planta que nao pertence ao jardim";
    /* salas sem pista explícita */
    return NULL;
}

/* Verifica se pista já está na BST (procura simples) */
int pistaJaColetada(PistaNode *raiz, const char *pista) {
    if (!raiz) return 0;
    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) return 1;
    if (cmp < 0) return pistaJaColetada(raiz->esq, pista);
    return pistaJaColetada(raiz->dir, pista);
}

/* Funções de liberação de memória */
void liberarPistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarPistas(raiz->esq);
    liberarPistas(raiz->dir);
    free(raiz->pista);
    free(raiz);
}
void liberarSala(Sala *s) {
    if (!s) return;
    liberarSala(s->esq);
    liberarSala(s->dir);
    free(s->nome);
    free(s);
}
void liberarHash(HashTable *ht) {
    if (!ht) return;
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashNode *cur = ht->buckets[i];
        while (cur) {
            HashNode *next = cur->next;
            free(cur->pista);
            free(cur->suspeito);
            free(cur);
            cur = next;
        }
        ht->buckets[i] = NULL;
    }
}

/* ---------- main: monta mansão, tabela hash, inicia exploração e julgamento ---------- */
int main(void) {
    /* Montar mansão (árvore binária de salas) - mapa fixo */
    Sala *root = criarSala("Hall");
    root->esq = criarSala("Library");
    root->dir = criarSala("Dining Room");

    root->esq->esq = criarSala("Study");
    root->esq->dir = criarSala("Conservatory");

    root->dir->esq = criarSala("Kitchen");
    root->dir->dir = criarSala("Ballroom");

    /* adicionar profundidade extra */
    root->esq->esq->esq = criarSala("Basement");

    /* Criar tabela hash e popular com pista -> suspeito */
    HashTable ht;
    for (int i = 0; i < HASH_SIZE; ++i) ht.buckets[i] = NULL;

    /* Ex.: pistas e suspeitos (definidos aqui, como regras do jogo) */
    inserirNaHash(&ht, "pegada de lama na soleira", "Sr. Green");
    inserirNaHash(&ht, "marca de dedo no livro raro", "Srta. Scarlet");
    inserirNaHash(&ht, "taça quebrada com resquicios", "Mrs. Peacock");
    inserirNaHash(&ht, "fio de tecido preso no cortador", "Mr. Black");
    inserirNaHash(&ht, "bilhete rasgado com iniciais", "Sr. Green");
    inserirNaHash(&ht, "mancha de tinta fresca", "Mr. Black");
    inserirNaHash(&ht, "programa de concerto dobrado", "Srta. Scarlet");
    inserirNaHash(&ht, "folha de planta que nao pertence ao jardim", "Mrs. Peacock");

    /* Arvore BST para pistas coletadas (inicialmente vazia) */
    PistaNode *pistasColetadas = NULL;

    /* Exploração interativa */
    explorarSalas(root, &pistasColetadas, &ht);

    /* Fase de acusação */
    printf("\n--- Fase de Acusacao ---\n");
    printf("Suspeitos conhecidos no caso:\n");
    printf("  - Sr. Green\n  - Srta. Scarlet\n  - Mrs. Peacock\n  - Mr. Black\n");

    char acusacao[MAX_INPUT];
    printf("Digite o nome do suspeito que deseja acusar (exato): ");
    if (!fgets(acusacao, sizeof(acusacao), stdin)) {
        printf("Entrada invalida. Encerrando.\n");
        liberarPistas(pistasColetadas);
        liberarSala(root);
        liberarHash(&ht);
        return 0;
    }
    /* remover newline */
    size_t len = strlen(acusacao);
    if (len > 0 && acusacao[len-1] == '\n') acusacao[len-1] = '\0';

    /* verificar quantas pistas apontam para este acusado */
    int contador = verificarSuspeitoFinal(pistasColetadas, &ht, acusacao);

    printf("\nVoce acusou: %s\n", acusacao);
    printf("Pistas coletadas que apontam para %s: %d\n", acusacao, contador);

    if (contador >= 2) {
        printf("Resultado: Acusacao valida! Ha evidencias suficientes para sustentar a acusacao.\n");
    } else {
        printf("Resultado: Acusacao fraca. Voce precisa de pelo menos 2 pistas que apontem para o suspeito.\n");
    }

    /* opcional: mostrar quais pistas coletadas apontam para qual suspeito */
    printf("\nResumo das pistas coletadas e seus suspeitos:\n");
    if (!pistasColetadas) {
        printf("  (nenhuma pista coletada)\n");
    } else {
        /* travessia in-order com impressao */
        /* reutilizar uma função recursiva localmente */
        /* Implementar uma travessia que imprime "pista -> suspeito" */
        /* pequena função lambda-ish usando recursão clássica: */
        /* para simplicidade, duplico código aqui: */
        /* in-order traversal */
        /* using stackless recursion */
        /* I'll write a helper nested function by creating another recursive function */
    }

    /* Função local para imprimir resumo */
    /* (não declarada no topo pois é usada apenas aqui) */
    void imprimirResumoRec(PistaNode *no, HashTable *ht) {
        if (!no) return;
        imprimirResumoRec(no->esq, ht);
        char *sus = encontrarSuspeito(ht, no->pista);
        if (sus)
            printf("  - \"%s\" -> %s\n", no->pista, sus);
        else
            printf("  - \"%s\" -> (sem suspeito associado)\n", no->pista);
        imprimirResumoRec(no->dir, ht);
    }

    imprimirResumoRec(pistasColetadas, &ht);

    /* liberar alocacoes */
    liberarPistas(pistasColetadas);
    liberarSala(root);
    liberarHash(&ht);

    printf("\nObrigado por jogar Detective Quest (modo textual)!\n");
    return 0;
}