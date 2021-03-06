#ifndef GLOBALH
#define GLOBALH

#define _GNU_SOURCE
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "conan_state.h"
/* odkomentować, jeżeli się chce DEBUGI */
//#define DEBUG 
/* boolean */
#define TRUE 1
#define FALSE 0

#define TAKEN 2

extern int BIBLIOTEKARZE;
extern int CONANI;
extern int PRALNIA;
extern int STROJE;

/* używane w wątku głównym, determinuje jak często i na jak długo zmieniają się stany */
#define STATE_CHANGE_PROB 50
#define SEC_IN_STATE 2

#define ROOT 0

/* stany procesu */
typedef enum { Waiting, Preparing, Exit_Librarian } librarian_state;
extern conan_state stan;
extern librarian_state l_stan;
extern int rank;
extern int size;
extern int my_priority;
extern int zlecenie_dla;
extern int sent_eq_acks;
extern int sent_laundry_acks;
extern int* zlecenia;
extern int* zebrane_ack;
extern int* zebrane_eq_req;
extern int* zebrane_eq_ack; // wydaję mi się, że dwie struktury są potrzebne, bo dopiero po zebraniu od wszystkich ACK LUB REQ my wysyłamy ACK i w jednej strukturze by się psuło, bo żeby przejść dalej musimy wiedzieć, że dostaliśmy od wszystkich WYŁĄCZNIE ACK (za długa linia, wiem, bujaj się)
extern int* zebrane_laundry_req;
extern int* zebrane_laundry_ack;
extern errand* errandQueue;

extern int lamport;
int incLamport();
int setMaxLamport(int nowy);

/* stan globalny wykryty przez monitor */
extern int globalState;
/* ilu już odpowiedziało na GIVEMESTATE */
extern int numberReceived;

/* to może przeniesiemy do global... */
typedef struct {
    int ts;       /* timestamp (zegar lamporta */
    int src;      /* pole nie przesyłane, ale ustawiane w main_loop */
    int priority;
    int data;
    int errandNum;     /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

/* Typy wiadomości */
#define ERRAND 1
#define REQ_ERRAND 2
#define ACK_ERRAND 3
#define REQ_EQ 4
#define ACK_EQ 5
#define REQ_LIB 6
#define ACK_LIB 7
#define REQ_LAUNDRY 8
#define ACK_LAUNDRY 9
#define FINISH 10

#define START_INTERNAL 11
#define END_INTERNAL 12

/* macro debug - działa jak printf, kiedy zdefiniowano
   DEBUG, kiedy DEBUG niezdefiniowane działa jak instrukcja pusta 
   
   używa się dokładnie jak printfa, tyle, że dodaje kolorków i automatycznie
   wyświetla rank

   w związku z tym, zmienna "rank" musi istnieć.

   w printfie: definicja znaku specjalnego "%c[%d;%dm [%d]" escape[styl bold/normal;kolor [RANK]
                                           FORMAT:argumenty doklejone z wywołania debug poprzez __VA_ARGS__
					   "%c[%d;%dm"       wyczyszczenie atrybutów    27,0,37
                                            UWAGA:
                                                27 == kod ascii escape. 
                                                Pierwsze %c[%d;%dm ( np 27[1;10m ) definiuje styl i kolor literek
                                                Drugie   %c[%d;%dm czyli 27[0;37m przywraca domyślne kolory i brak pogrubienia (bolda)
                                                ...  w definicji makra oznacza, że ma zmienną liczbę parametrów
                                            
*/
#ifdef DEBUG
#define debug(FORMAT,...) printf("%c[%d;%dm [%d][tid %d][prio %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, lamport, my_priority,  ##__VA_ARGS__, 27,0,37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm",27,1,37);
#define P_BLACK printf("%c[%d;%dm",27,1,30);
#define P_RED printf("%c[%d;%dm",27,1,31);
#define P_GREEN printf("%c[%d;%dm",27,1,33);
#define P_BLUE printf("%c[%d;%dm",27,1,34);
#define P_MAGENTA printf("%c[%d;%dm",27,1,35);
#define P_CYAN printf("%c[%d;%d;%dm",27,1,36);
#define P_SET(X) printf("%c[%d;%dm",27,1,31+(6+X)%7);
#define P_CLR printf("%c[%d;%dm",27,0,37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d][tid %d]: " FORMAT "%c[%d;%dm\n",  27, (1+(rank/7))%2, 31+(6+rank)%7, rank, ##__VA_ARGS__, 27,0,37);

/* wysyłanie pakietu, skrót: wskaźnik do pakietu (0 oznacza stwórz pusty pakiet), do kogo, z jakim typem */
void sendPacket(packet_t *pkt, int destination, int tag);
void sendErrandPacket(packet_t *pkt, int destination, int errandNum, int tag);
void forwardPacket(packet_t *pkt, int destination, int tag);
void changeConanState( conan_state );
void changeLibrarianState( librarian_state newState );
void *wash();
void collect_laundry();
void sendMutedAck(int dest, int tag, int *acks);
#endif
