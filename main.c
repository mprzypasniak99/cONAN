#include "main.h"
#include "conan_communication.h"
#include "librarian_main.h"
#include "conan_main.h"
#include "monitor.h"
/* wątki */
#include <pthread.h>

/* sem_init sem_destroy sem_post sem_wait */
//#include <semaphore.h>
/* flagi dla open */
//#include <fcntl.h>

int lamport;

conan_state stan=Ready;
librarian_state l_stan=Preparing;
volatile char end = FALSE;
int size,rank; /* nie trzeba zerować, bo zmienna globalna statyczna */
int my_priority;
int zlecenie_dla;
int sent_eq_acks;
int sent_laundry_acks;
int zlecenia[BIBLIOTEKARZE];
int zebrane_ack[CONANI];
int zebrane_eq_req[CONANI];
int zebrane_eq_ack[CONANI];
int zebrane_laundry_req[CONANI];
int zebrane_laundry_ack[CONANI];
MPI_Datatype MPI_PAKIET_T;
pthread_t threadKom, threadMon;

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
    int provided;
    MPI_Init_thread(argc, argv,MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);


    /* Stworzenie typu */
    /* Poniższe (aż do MPI_Type_commit) potrzebne tylko, jeżeli
       brzydzimy się czymś w rodzaju MPI_Send(&typ, sizeof(pakiet_t), MPI_BYTE....
    */
    /* sklejone z stackoverflow */
    const int nitems=4; /* bo packet_t ma trzy pola */
    int       blocklengths[4] = {1,1,1,1};
    MPI_Datatype typy[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[4]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, priority);
    offsets[3] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);

    if(rank >= BIBLIOTEKARZE) {
        pthread_create( &threadKom, NULL, conanCommunicationThread, 0);
    }
    if (rank==0) {
	    pthread_create( &threadMon, NULL, startMonitor, 0);
    }
    debug("jestem");
}

/* usunięcie zamkków, czeka, aż zakończy się drugi wątek, zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    pthread_mutex_destroy( &stateMut);
    /* Czekamy, aż wątek potomny się zakończy */
    println("czekam na wątek \"komunikacyjny\"\n" );
    pthread_join(threadKom,NULL);
    if (rank==0) pthread_join(threadMon,NULL);
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

void forwardPacket(packet_t *pkt, int destination, int tag) {
    if (pkt != NULL) {
        MPI_Send ( pkt, 1, MPI_PAKIET_T, destination, tag, MPI_COMM_WORLD);
    }
}

// void changeTallow( int newTallow )
// {
//     pthread_mutex_lock( &tallowMut );
//     if (stan==InFinish) { 
// 	pthread_mutex_unlock( &tallowMut );
//         return;
//     }
//     tallow += newTallow;
//     pthread_mutex_unlock( &tallowMut );
// }

void changeState( conan_state newState )
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
    sleep(7);
    pthread_mutex_lock(&washMut);
    if(equipmentQueue != NULL) {
        sendPacket(0, equipmentQueue->destination, ACK_EQ);
        debug("Sent ACK_EQ");
    } else sent_eq_acks--;
    if(laundryQueue != NULL) {
        sendPacket(0, laundryQueue->destination, ACK_LAUNDRY);
        debug("Sent ACK_LAUNDRY");
    } else sent_laundry_acks--;
    pthread_mutex_unlock( &washMut );
    debug("Washing handled");
}

void sendMutedAck(int dest, int tag, int *acks) {
    pthread_mutex_lock(&washMut);
    (*acks)++;
    sendPacket(0, dest, tag);
    debug("Sent ACK %d to %d", tag, dest);
    pthread_mutex_unlock( &washMut );
}

void collect_laundry() {
    pthread_mutex_lock(&washMut);
    //sent_laundry_acks -= rzeczy_do_odebrania;
    //sent_eq_acks -= rzeczy_do_odebrania;
    //rzeczy_do_odebrania = 0;
    //useless
    pthread_mutex_unlock( &washMut );
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc,&argv); // tworzy wątek komunikacyjny w "watek_komunikacyjny.c"
    if (rank < BIBLIOTEKARZE)
    {
        librarianMainLoop();
    } else {
        conanMainLoop();
    }
             // w pliku "watek_glowny.c"

    finalizuj();
    return 0;
}

