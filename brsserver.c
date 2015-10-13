/** \file brrserver.c
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<pthread.h>
#include<string.h>
#include<sys/socket.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/un.h>
#include<sys/types.h>
#include<sys/un.h>
#include<pthread.h>
#include<signal.h>
#include"bris.h"
#include"users.h"
#include"comsock.h"
#define SOCK_PATH "./tmp/briscola.skt"
#define FOUT "brs.checkpoint"
#include"matchServer.h"

static bool_t sigBool = TRUE;

typedef struct el { pthread_t tid; struct el * next; } el_t;

static nodo_t** r = NULL;/*albero utenti*/

int nPartite = 0;

static pthread_mutex_t semPartite = PTHREAD_MUTEX_INITIALIZER;/*mutua esclusione sul numero di partite*/

static pthread_mutex_t semTree = PTHREAD_MUTEX_INITIALIZER; /*mutua esclusione sull'albero*/

bool_t testing = FALSE; /*test di brsserver, viene messo a TRUE nel main nel caso sia presente l'opzione -t*/

el_t *listaT = NULL;

/*thread worker, si occupa di gestire registrazioni, cancellazioni e connessioni*/
static void * worker ( void* sock ){

	int sockInt; 
	message_t msgIn; /*messaggio in entrata*/
	message_t msgOut; /*messaggio in uscita*/
	user_t *utente = NULL; /*utente da allocare*/
	int check; /*memorizza il risultato di addUser*/
	int checkStatus; /*memorizza lo stato di un utente*/
	bool_t checkBool;/*memorizza il risultato di checkPwd*/
	char *userList;
	int sockOpponent; /*socket dell'avversario*/
	int part;
	int esito;/*registra l'esito delle receiveMessage*/
	bool_t trovatoAvversario = FALSE; /*registra se è stato inserito un avversario valido*/
	
	sockInt = (int)(long)sock;
	
	/*si riceve il messaggio dalla socket*/
	esito = receiveMessage( sockInt, &msgIn );
	
	if( esito == -1 ){
		perror("Errore nello scambio di messaggi!\n");
		pthread_exit(NULL);
	}
	
	if ( esito == ENOTCONN ){
		perror("Utente disconnesso!\n");
		pthread_exit(NULL);
	}
	
	/*si alloca l'utente*/
	if  (( utente = malloc(sizeof(user_t) )) == NULL ){
		perror("Can't allocate User\n");
		pthread_exit(NULL);
	}
	
	/*si crea l'utente che ha fatto la richiesta*/
	if (( utente = stringToUser( msgIn.buffer, msgIn.length )) == NULL) pthread_exit(NULL);

	/*ricevuta richiesta di registrazione*/
	if ( msgIn.type == MSG_REG ){

		/*aggiunta dell'utente all'albero in mutua esclusione*/
		pthread_mutex_lock(&semTree);

		check = addUser( r, utente );

		pthread_mutex_unlock(&semTree);

		/*invio del messaggio in caso di errore nell'addUser*/
		if ( check == -1 ){
			msgOut.type = MSG_ERR;
			msgOut.buffer = malloc(50*sizeof(char));
			strcpy(msgOut.buffer, "addUser.failed!");
			msgOut.length = strlen(msgOut.buffer)+1;
			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);

		}
		/*invio del messaggio nel caso in cui l'utente sia già registrato*/
		else if ( check == 1 ){
			msgOut.type = MSG_NO;
			msgOut.buffer = malloc(50*sizeof(char));
			strcpy(msgOut.buffer, "userAlreadyRegistered!");
			msgOut.length = strlen(msgOut.buffer)+1;
			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);
		}
		/*invio del messaggio nel caso in cui l'operazione è andata a buon fine*/
		else if ( check == 0 ){
			msgOut.type = MSG_OK;
			msgOut.length = 0;
			msgOut.buffer = NULL;
			if ( sendMessage( sockInt, &msgOut ) == -1 ) pthread_exit(NULL);
		}

	}

	/*richiesta rimozione utente*/
	else if ( (msgIn.type) == MSG_CANC){
	
		/*accesso all'albero in mutua esclusione*/

		pthread_mutex_lock(&semTree);

		check = removeUser( r, utente );

		pthread_mutex_unlock(&semTree);

		/*invio messaggio:password sbagliata*/
		if ( check ==  WRPWD ){
			msgOut.type = MSG_ERR;
			msgOut.buffer = malloc(50*sizeof(char));
			msgOut.buffer = "removeUser.wrongPassword!";
			msgOut.length = strlen(msgOut.buffer)+1;
			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);
		}
		/*invio messaggio:utente non presente*/
		if ( check == NOUSR ){
			msgOut.type = MSG_ERR;
			msgOut.buffer = malloc(50*sizeof(char));
			msgOut.buffer = "removeUser.userNotRegistered!";
			msgOut.length = strlen(msgOut.buffer)+1;
			if ( sendMessage( sockInt, &msgOut ) == -1 ) pthread_exit(NULL);

		}
		/*invio messaggio:errore*/
		if ( check == -1 ){
			msgOut.type = MSG_ERR;
			msgOut.buffer = malloc(50*sizeof(char));
			msgOut.buffer = "removeUser.failed!";
			msgOut.length = strlen(msgOut.buffer)+1;
			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);

		}
		/*invio messaggio: operazione andata a buon fine*/
		if ( check == 0 ){
			msgOut.type = MSG_OK;
			msgOut.length = 0;
			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);
		}
	}

	/*gestione MSG_CONNECT*/
	else if (msgIn.type == MSG_CONNECT) {

		/*controllo password fornita, mutua esclusione*/
		pthread_mutex_lock(&semTree);

		checkBool = checkPwd(*r,utente);

		pthread_mutex_unlock(&semTree);

		if(!checkBool){

			/*password errata, invia MSG_NO*/
			msgOut.type = MSG_NO;
			msgOut.buffer = malloc(50*sizeof(char));
			strcpy(msgOut.buffer, "checkPwd.failed!");
			msgOut.length = strlen(msgOut.buffer)+1;

			if ( sendMessage( sockInt, &msgOut ) == -1 ){
				free(msgOut.buffer);
				pthread_exit(NULL);
			}
			free(msgOut.buffer);
		}
		else {
			/*password corretta*/
			/*se l'utente è già connesso si invia MSG_NO*/
			pthread_mutex_lock(&semTree);
			if( getUserStatus(*r, utente->name) != DISCONNECTED ){
				pthread_mutex_unlock(&semTree);
				msgOut.type = MSG_NO;
				msgOut.buffer = malloc(50*sizeof(char));
				strcpy(msgOut.buffer, "userAlreadyConnected!");
				msgOut.length = strlen(msgOut.buffer)+1;

				if ( sendMessage( sockInt, &msgOut ) == -1 ){
				perror("Errore nello scambio di messaggi!\n");
				free(msgOut.buffer);
				pthread_exit(NULL);
				}
				free(msgOut.buffer);
				
				pthread_exit(NULL);
				
			} 
			else {
				/*se l'utente non era connesso si mette il suo stato a PLAYING*/
				setUserStatus(*r, utente->name, PLAYING);
				pthread_mutex_unlock(&semTree);
			}
	
			do{
				
				/*prepara lista utenti in mutua esclusione*/
				pthread_mutex_lock(&semTree);
			
				userList = getUserList(*r, WAITING);

				pthread_mutex_unlock(&semTree);

				/*se lista non è vuota*/
				if (userList != NULL){
					/*invia MSG_OK con lista*/
					msgOut.type = MSG_OK;
					msgOut.buffer = userList;
					msgOut.length = strlen(userList) + 1;
					fprintf(stderr,"%s", msgOut.buffer);
					if ( sendMessage( sockInt, &msgOut ) == -1 ){
						free(msgOut.buffer);
						pthread_exit(NULL);
				}
				
				free(msgOut.buffer);
			
			
				/*attende risposta dal client*/
				if ( msgIn.buffer != NULL )
				free(msgIn.buffer);
				
				
				esito = receiveMessage( sockInt, &msgIn);
				if( esito == -1 ){
					perror("Errore nello scambio di messaggi!\n");
					pthread_exit(NULL);
				}
	
				if ( esito == ENOTCONN ){
					perror("Utente disconnesso!\n");
					pthread_exit(NULL);
				}

				/*il client ha comunicato un avversario da sfidare*/
				if ( msgIn.type == MSG_OK ) {
					
					/*verifica se l'avversario è disponibile*/

					pthread_mutex_lock(&semTree);

					checkStatus = getUserStatus(*r, msgIn.buffer);
					
					/* se l'avversario è in attesa allora è valido, inizia la partita*/
					if (checkStatus == WAITING) {
						
						trovatoAvversario = TRUE;
						
						setUserStatus( *r, msgIn.buffer, PLAYING );
						sockOpponent = getUserChannel(*r, msgIn.buffer);

						pthread_mutex_unlock(&semTree);
			
						msgOut.type = MSG_OK;
						msgOut.length = 0;
						msgOut.buffer = NULL;

						if ( sendMessage( sockOpponent, &msgOut ) == -1 ){
							free(msgOut.buffer);
							pthread_exit(NULL);
						}
						
						if ( sendMessage( sockInt, &msgOut ) == -1 ){
							free(msgOut.buffer);
							pthread_exit(NULL);
						}
						
						pthread_mutex_lock(&semPartite);
						nPartite++;
						part = nPartite;
						pthread_mutex_unlock(&semPartite); 
						if(manageMatch(utente->name, msgIn.buffer, sockInt, sockOpponent, testing, part) == -1){ 
							pthread_mutex_lock(&semTree);
							setUserStatus(*r, utente->name, DISCONNECTED);
							setUserStatus(*r, msgIn.buffer, DISCONNECTED);
							pthread_mutex_unlock(&semTree);
							pthread_exit(NULL);
						}
						pthread_mutex_lock(&semTree);
						setUserStatus(*r, utente->name, DISCONNECTED);
						setUserStatus(*r, msgIn.buffer, DISCONNECTED);
						pthread_mutex_unlock(&semTree);
					}
					else {
						pthread_mutex_unlock(&semTree);
						/*l'avversario non è valido, invia MSG_NO*/
						msgOut.type = MSG_NO;
						msgOut.buffer = malloc(50*sizeof(char));
						msgOut.buffer = "notValidOpponent!"; 
						msgOut.length = strlen(msgOut.buffer)+1;

						if ( sendMessage( sockInt, &msgOut ) == -1 ){
							free(msgOut.buffer);
							pthread_exit(NULL);
						}


					}

				}
			/*il client preferisce attendere*/
			else if ( msgIn.type == MSG_WAIT ){
				
					trovatoAvversario = TRUE;

					pthread_mutex_lock(&semTree);

					check = setUserStatus(*r, utente->name, WAITING);
					checkBool = setUserChannel(*r, utente->name, sockInt);

					pthread_mutex_unlock(&semTree);

					msgOut.type = MSG_WAIT;
					msgOut.buffer = NULL;
					msgOut.length = 0;
					if ( sendMessage( sockInt, &msgOut ) == -1 ){
						free(msgOut.buffer);
						pthread_exit(NULL);
					}
					free(msgOut.buffer);
				}
			}
			/*se non ci sono avversari*/
			else {
				/*pone l'utente in attesa di una sfida*/
				
				trovatoAvversario = TRUE;
				
				pthread_mutex_lock(&semTree);
				check = setUserStatus(*r, utente->name, WAITING);
				setUserChannel(*r, utente->name, sockInt);
				pthread_mutex_unlock(&semTree);
				
				msgOut.type = MSG_WAIT;
				msgOut.buffer = NULL;
				msgOut.length = 0;
				if ( sendMessage( sockInt, &msgOut ) == -1 ){
					free(msgOut.buffer);
					pthread_exit(NULL);
				}
			}
		} while(!trovatoAvversario);
		} 
	}

	pthread_exit(NULL);

}

void* gest (void* s){

	sigset_t gest_pset;
	int signum;
	int erroreWait;
	FILE* fileOut;
	el_t *aux = NULL;

	/*si inizializza la maschera dei segnali per il gestore*/
	if ( ( sigfillset(&gest_pset) ) == -1 ){
		perror("sigemptyset failed:");
		exit(EXIT_FAILURE);
	}
	/*aggiunta segnali al sigset*/
	if ( ( sigaddset( &gest_pset, SIGUSR1 ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}
	if ( ( sigaddset( &gest_pset, SIGINT ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}
	if ( ( sigaddset( &gest_pset, SIGTERM ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}

	while(1){
		/*ricezione del segnale settato nella mask*/
		if ( (erroreWait = sigwait ( &gest_pset, &signum )) != 0 ){
			errno = erroreWait;
			perror("sigwait failed:");
			pthread_exit(NULL);
		}
		
		/*gestione di SIGUSR1*/
		if ( signum == SIGUSR1 ){

			if ( (fileOut = fopen(FOUT, "w")) == NULL ){
				perror("fopen fileOut failed");
				pthread_exit(NULL);
			}

			/*accesso all'albero degli utenti in mutua esclusione*/
				pthread_mutex_lock(&semTree);
				
			/*si copiano gli utenti e i permessi in FOUT*/
			if ( storeUsers(fileOut, *r) == -1 ){
				pthread_mutex_unlock(&semTree);
				perror("storeUsers failed");
				pthread_exit(NULL);
			} 
			/*rilascio mutua esclusione*/
			pthread_mutex_unlock(&semTree);
			/*chiusura file*/
			fclose(fileOut);
		}	
		
	if ( signum == SIGTERM || signum == SIGINT ){	
		
		sigBool = FALSE;
		
		/*si attende la terminazione di tutte le partite*/
		while( aux != NULL){
			pthread_join(aux->tid, NULL);
		}
		
		/*apertura file per salvare gli utenti*/
		if ( (fileOut = fopen(FOUT, "w")) == NULL ){
			perror("fopen fileOut failed");
			pthread_exit(NULL);
		}

		/*accesso all'albero degli utenti in mutua esclusione*/
		 pthread_mutex_lock(&semTree);
		 
		/*si copiano gli utenti e i permessi in FOUT, si libera l'albero*/
		if ( storeUsers(fileOut, *r) == -1 ){
			perror("storeUsers failed");
			pthread_mutex_unlock(&semTree);
			pthread_exit(NULL);
		} 
		
		freeTree(*r);
		
		/*rilascio mutua esclusione*/
		pthread_mutex_unlock(&semTree);

		/*chiusura file*/
		fclose(fileOut);
		closeServerChannel(SOCK_PATH,(long)(void*) s);
		exit(EXIT_SUCCESS);

	}
  }
 return NULL;
}


int main ( int argc, char** argv ){

	sigset_t main_pset;
	int fd;/*socket main*/
	int thread_fd; /* socket thread*/
	pthread_t worker_tid, gest_tid;
	FILE *users;
	message_t msg;

	if (argc==3 && strcmp(argv[2],"-t")==0) testing = TRUE;

	/*inizializzazione del sigset del main*/
	if ( ( sigemptyset(&main_pset) ) == -1 ){
		perror("sigemptyset failed:");
		exit(EXIT_FAILURE);
	}
	/*aggiunta del segnale SIGUSR1, SIGINT, SIGTERM alla mask*/
	if ( ( sigaddset( &main_pset, SIGUSR1 ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}
	if ( ( sigaddset( &main_pset, SIGINT ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}
	if ( ( sigaddset( &main_pset, SIGTERM ) == -1 )){
		perror("sigaddset failed:");
		exit(EXIT_FAILURE);
	}
	/*la nuova signal mask viene settata: blocco i segnali */
	if ( ( pthread_sigmask( SIG_SETMASK, &main_pset, NULL) != 0 ) ){
		perror("sigmask failed:");
		exit(EXIT_FAILURE);
	}
	/*controlla il numero di argomenti passati*/
	if ( argc < 2 ) exit(EXIT_FAILURE);
	/*apertura file utenti*/
	if ( ( users = fopen(argv[1], "r") ) == NULL ) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}
	/*allocazione albero utenti*/
	r = malloc(sizeof(nodo_t*));

	if ( r == NULL ) exit(EXIT_FAILURE);

	*r=NULL;

	/*caricamento utenti nell'albero*/
	if ( loadUsers(users, r) == -1 ) {
		perror("loadUsers");
		exit(EXIT_FAILURE);
	}
	
	
	
	/*creazione canale socket*/
	if ( ( fd = createServerChannel(SOCK_PATH) ) <= 0 ) {
		perror("createChannel");
		exit(EXIT_FAILURE);
	}
	
	/*creazione thread gestore dei segnali*/
	if ( (pthread_create(&gest_tid, NULL, &gest, (void*)(long)fd)) == -1 ) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}
		
	/*chiamate per multithreading*/
	while ( 1 ) {
		el_t *newEl = malloc(sizeof(el_t));
		if ( (thread_fd = acceptConnection( fd )) == -1 ) exit(EXIT_FAILURE);
		if(sigBool){
			if (pthread_create( &worker_tid, NULL, &worker, (void *)(long)thread_fd )!=0) perror("pthread_create.Failed!") ;
			newEl->tid = worker_tid;
			newEl->next = listaT;
			listaT = newEl;
		}
		else {
			msg.type = MSG_NO;
			msg.length = 50;
			msg.buffer = malloc(msg.length*(sizeof(char)));
			msg.buffer = "Server non più attivo!";
			sendMessage(fd, &msg);
			free(msg.buffer);
			closeConnection(fd);
		}
	}
	
	exit(EXIT_SUCCESS);
}
