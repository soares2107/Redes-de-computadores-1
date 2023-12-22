//TRABALHO APRESENTADO POR JOÃO GABRIEL SOARES E EDUARDO GONÇALVES
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h> // Adicionando inclusão necessária para o tipo pid_t
#include <sys/wait.h> // Adicionando inclusão necessária para waitpid

using namespace std;

struct PacoteEthernet {
    int endereco_destino[6];
    int endereco_origem[6];
    int tipo;
    int dados[375];
};

struct CaboEthernet {
    bool em_uso;
    int contador;
    PacoteEthernet pacoteEnviado;
};

void imprimirPacote(const PacoteEthernet& pacote) {
    cout << "Endereço MAC Remetente: ";
    for (int i = 0; i < 6; i++) {
        cout << pacote.endereco_origem[i] << " ";
    }
    cout << endl;
    cout << "Endereço MAC Destino: ";
    for (int i = 0; i < 6; i++) {
        cout << pacote.endereco_destino[i] << " ";
    }
    cout << endl << endl;
}

int main() {
    srand(static_cast<unsigned>(time(0)));

    key_t chave = ftok("/tmp", 'X');
    int idMemoriaCompartilhada = shmget(chave, sizeof(CaboEthernet), IPC_CREAT | 0666);
    CaboEthernet *cabo = static_cast<CaboEthernet*>(shmat(idMemoriaCompartilhada, nullptr, 0));

    const int num_processos = 64;
    cabo->em_uso = false;
    cabo->contador = 1;

    cout << "NUMERO DE PROCESSOS CRIADOS: " << num_processos << endl;

    pid_t processos_filhos[num_processos];

    for (int i = 0; i < num_processos; i++) {
        processos_filhos[i] = fork();
        if (processos_filhos[i] == 0) {
            while (true) {
                if (!cabo->em_uso) {
                    cabo->em_uso = true;
                    PacoteEthernet pacote;
                    for (int i = 0; i < 6; i++) {
                        pacote.endereco_destino[i] = (rand() % 256 + cabo->contador++) % 100;
                        pacote.endereco_origem[i] = (rand() % 256 + cabo->contador++) % 100;
                    }
                    pacote.tipo = rand() % 256;
                    for (int i = 0; i < 1500; i++) {
                        pacote.dados[i] = rand() % 256;
                    }

                    cabo->pacoteEnviado = pacote;

                    if (memcmp(cabo->pacoteEnviado.endereco_destino, pacote.endereco_destino, sizeof(pacote.endereco_destino)) != 0 ||
                        memcmp(cabo->pacoteEnviado.endereco_origem, pacote.endereco_origem, sizeof(pacote.endereco_origem)) != 0 ||
                        cabo->pacoteEnviado.tipo != pacote.tipo ||
                        memcmp(cabo->pacoteEnviado.dados, pacote.dados, sizeof(pacote.dados)) != 0) {
                        write(1, "COLISAO\n", 8);  // colisão
                    } else {
                        write(1, "PACOTE ENVIADO\n", 15);
                        imprimirPacote(pacote);
                        cabo->em_uso = false;
                    }
                }
                usleep(1200000);
            }
        }
    }

    // Aguarda o término de todos os processos filhos
    for (int i = 0; i < num_processos; i++) {
        waitpid(processos_filhos[i], NULL, 0);
    }

    return 0;
}
