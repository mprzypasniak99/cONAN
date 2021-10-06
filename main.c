#include "main.h"
#include "conan_errand.h"
#include "librarian_main.h"
#include "conan_main.h"
#include "queue.h"
/* wątki */
#include <pthread.h>

int lamport;

conan_state stan=Ready;
librarian_state l_stan=Preparing;
volatile char end = FALSE;
int size,rank; /* nie trzeba zerować, bo zmienna globalna statyczna */
int my_priority;
int zlecenie_dla;
int sent_eq_acks;
int sent_laundry_acks;

int BIBLIOTEKARZE;
int CONANI;
int PRALNIA;
int STROJE;

int* zlecenia;
int* zebrane_ack;
int* zebrane_eq_req;
int* zebrane_eq_ack;
int* zebrane_laundry_req;
int* zebrane_laundry_ack;
errand* errandQueue;

MPI_Datatype MPI_PAKIET_T;
pthread_t threadErrand;

pthread_mutex_t stateMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t washMut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamportMut = PTHREAD_MUTEX_INITIALIZER;

int incLamport() {
	pthread_mutex_lock(&lamportMut);
	if(stan == Exit) {
		pthread_mutex_unlock(&lamportMut);
		return lamport;
	}
	lamport++; int tmp = lamport;
	pthread_mutex_unlock(&lamportMut);
	return tmp;
}

int setMaxLamport(int nowy) {
	pthread_mutex_lock(&lamportMut);
	if(stan == Exit) {
		pthread_mutex_unlock(&lamportMut);
		return lamport;
	}
	
	lamport = (nowy > lamport) ? nowy : lamport;
	lamport++; int tmp = lamport;
	pthread_mutex_unlock(&lamportMut);

	return tmp;
}

void check_thread_support(int provided)
{
    printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided) {
        case MPI_THREAD_SINGLE: 
            printf("Brak wsparcia dla wątków, kończę\n");
            /* Nie ma co, trzeba wychodzić */
	    fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
	    MPI_Finalize();
	    exit(-1);
	    break;
        case MPI_THREAD_FUNNELED: 
            printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
	    break;
        case MPI_THREAD_SERIALIZED: 
            /* Potrzebne zamki wokół wywołań biblioteki MPI */
            printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
	    break;
        case MPI_THREAD_MULTIPLE: printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
	    break;
        default: printf("Nikt nic nie wie\n");
    }
}

/* srprawdza, czy są wątki, tworzy typ MPI_PAKIET_T
*/
void inicjuj(int *argc, char ***argv)
{
    zlecenia = malloc(sizeof(int) * BIBLIOTEKARZE);
    zebrane_ack = malloc(sizeof(int) * CONANI);
    zebrane_eq_req = malloc(sizeof(int) * CONANI);
    zebrane_eq_ack = malloc(sizeof(int) * CONANI);
    zebrane_laundry_req = malloc(sizeof(int) * CONANI);
    zebrane_laundry_ack = malloc(sizeof(int) * CONANI);
    errandQueue = malloc(sizeof(errand) * BIBLIOTEKARZE);

    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);
    for (int i = 0; i < BIBLIOTEKARZE; i++) {
        errandQueue[i] = (errand){0, TRUE, 0, 0};
    }
    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems=5; /* bo packet_t ma trzy pola */
    int       blocklengths[5] = {1,1,1,1,1};
    MPI_Datatype typy[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[5]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, priority);
    offsets[3] = offsetof(packet_t, data);
    offsets[4] = offsetof(packet_t, errandNum);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if(rank >= BIBLIOTEKARZE) {
        pthread_create( &threadErrand, NULL, conanErrandThread, 0);
    }
    debug("jestem");
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    free(zlecenia);
    free(zebrane_ack);
    free(zebrane_eq_ack);
    free(zebrane_eq_req);
    free(zebrane_laundry_ack);
    free(zebrane_laundry_req);
    free(errandQueue);

    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadErrand,NULL);
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}


/* opis patrz main.h */
void sendPacket(packet_t *pkt, int destination, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;
    pkt->ts = incLamport();
    pkt->priority = my_priority;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt) free(pkt);
}

void sendErrandPacket(packet_t *pkt, int destination, int errandNum, int tag)
{
    int freepkt=0;
    if (pkt==0) { pkt = malloc(sizeof(packet_t)); freepkt=1;}
    pkt->src = rank;
    pkt->ts = incLamport();
    pkt->priority = my_priority;
    pkt->errandNum = errandNum;
    MPI_Send( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    if (freepkt) free(pkt);
}

void forwardPacket(packet_t *pkt, int destination, int tag) {
    if (pkt != NULL) {
        MPI_Send ( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    }
}

void changeConanState( conan_state newState )
{
    pthread_mutex_lock( &stateMut );
    if (stan==Exit) { 
	pthread_mutex_unlock( &stateMut );
        return;
    }
    stan = newState;
    pthread_mutex_unlock( &stateMut );
}

void changeLibrarianState( librarian_state newState )
{
    pthread_mutex_lock( &stateMut );
    if (l_stan==Exit) { 
	pthread_mutex_unlock( &stateMut );
        return;
    }
    l_stan = newState;
    pthread_mutex_unlock( &stateMut );
}

void *wash() {
    sleep(15);
    debug("Washing ended");
    pthread_mutex_lock(&washMut);
    while(equipmentQueue != NULL) {
        sendPacket(0, equipmentQueue->destination, ACK_EQ);
        debug("ACK_EQ sent to %d", equipmentQueue->destination);
        deleteFromQueue(&equipmentQueue, equipmentQueue->destination);
    }
    while(laundryQueue != NULL) {
        sendPacket(0, laundryQueue->destination, ACK_LAUNDRY);
        debug("ACK_LAUNDRY sent to %d", laundryQueue->destination);
        deleteFromQueue(&laundryQueue, laundryQueue->destination);
    }
    sent_eq_acks = 0;
    sent_laundry_acks = 0;
    debug("Washing handled");
    pthread_mutex_unlock( &washMut );
}

void sendMutedAck(int dest, int tag, int *acks) {
    pthread_mutex_lock(&washMut);
    //(*acks)++;
    sendPacket(0, dest, tag);
    pthread_mutex_unlock( &washMut );
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        printf("Not enough arguments to start");
        return -1;
    }

    BIBLIOTEKARZE = atoi(argv[1]);
    CONANI = atoi(argv[2]);
    PRALNIA = atoi(argv[3]);
    STROJE = atoi(argv[4]);

    inicjuj(&argc,&argv);
    if (rank < BIBLIOTEKARZE)
    {
        librarianMainLoop();
    } else {
        conanMainLoop();
    }

    finalizuj();
    return 0;
}

