/*para rodar, compile o codigo usando: mpicc -o roteamento_distribuido roteamento_distribuido.c
e execute usando: mpirun -np 7 --oversubscribe ./roteamento_distribuido */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define INF 1000000 // valor grande para representar "infinito"
#define NUM_NODES 7 //definição do numero de nós

// Matriz de adjacência inicial para o exemplo especifico escolhido
const int matriz_adjacencia[NUM_NODES][NUM_NODES] = {
    {0, 7, INF, INF, INF, INF, INF},
    {7, 0, INF, 6, 2, INF, INF},
    {INF, INF, 0, 9, INF, INF, INF},
    {INF, 6, 9, 0, INF, INF, INF},
    {INF, 2, INF, INF, 0, 8, INF},
    {INF, INF, INF, INF, 8, 0, 3},
    {INF, INF, INF, INF, INF, 3, 0}
};

//inicialização do mpi
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    //salva nas variaveis o rank do processo atual e a quantidade de processos totais
    int rank, tamanho;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &tamanho);

    //checa se o numero de processos é suficiente para a quantia de nós que iremos trabalhar, predefinida na constante NUM_NODES
    if (tamanho != NUM_NODES) {
        if (rank == 0) {
            fprintf(stderr, "O número de processos deve ser %d.\n", NUM_NODES);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    //inicializa o vetor de distancia
    int vetor_distancia[NUM_NODES];
    for (int i = 0; i < NUM_NODES; i++) {
        //o preenche inicialmente com infinito exceto na distancia do nó com ele mesmo que é 0
        vetor_distancia[i] = INF;
    }
    vetor_distancia[rank] = 0; 

    //variavel que sera utilizada para saber se não existem novos valores no vetor distancia, indicando que ja encontramos os melhores caminhos
    int atualizado;

    do {
        atualizado = 0;

        // Enviar vetor de distância para os vizinhos
        for (int vizinho = 0; vizinho < NUM_NODES; vizinho++) {
            if (matriz_adjacencia[rank][vizinho] != INF && vizinho != rank) {
                MPI_Send(vetor_distancia, NUM_NODES, MPI_INT, vizinho, 0, MPI_COMM_WORLD);
            }
        }

        // Receber vetores de distância dos vizinhos e atualizar
        for (int vizinho = 0; vizinho < NUM_NODES; vizinho++) {
            if (matriz_adjacencia[rank][vizinho] != INF && vizinho != rank) {
                int vetor_vizinho[NUM_NODES];
                MPI_Recv(vetor_vizinho, NUM_NODES, MPI_INT, vizinho, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int i = 0; i < NUM_NODES; i++) {
                    int nova_distancia = vetor_vizinho[i] + matriz_adjacencia[rank][vizinho];
                    if (vetor_distancia[i] > nova_distancia) {
                        vetor_distancia[i] = nova_distancia;
                        atualizado = 1;
                    }
                }
            }
        }

        // salva em atualizado 1 caso ainda tenham sido feitas alterações, caso não salva 0 e termina o loop, sabemos que ja temos as melhores distancias
        int atualizado_global;
        MPI_Allreduce(&atualizado, &atualizado_global, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        atualizado = atualizado_global;

    } while (atualizado);

    // Coletar os vetores de distância finais no processo 0 para montar a matriz de distancias
    int matriz_final[NUM_NODES][NUM_NODES];
    MPI_Gather(vetor_distancia, NUM_NODES, MPI_INT, matriz_final, NUM_NODES, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nMatriz de roteamento final:\n\n");
        printf("    A   B   C   D   E   F   G\n");
        printf("  -----------------------------\n");
        for (int i = 0; i < NUM_NODES; i++) {
            printf("%c |", 'A' + i);
            for (int j = 0; j < NUM_NODES; j++) {
                printf("%2d |", matriz_final[i][j]);
            }
            printf("\n");
        }
    }

    MPI_Finalize();
    return 0;
}
