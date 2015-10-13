  /** \file brsclient.c
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/un.h>
#include"bris.h"
#include"users.h"
#include"comsock.h"
#include"match.h"

#define SOCK_PATH "./tmp/briscola.skt"

int main ( int argc, char** argv ){
		
		int fd;
		char nomePassword[LUSER+LPWD+2];
		message_t newMIn;
		message_t newMOut;
		char sfidato[LUSER+1];
		int checkSfidato;
		char aux;
		int esito; /*registra l'esito delle receiveMessage*/

		/*controllo sugli argomenti*/
		if ( argc < 3 || argc > 4 ) exit(EXIT_FAILURE);
		
		/*apertura della connessione*/
		if ( ( fd = openConnection(SOCK_PATH, NTRIAL, NSEC ) ) == -1 ) {
			perror(argv[1]);
			exit(EXIT_FAILURE);
		}
		
		/*tre argomenti: richiesta di connessione*/
		if ( argc == 3 ) {
			/*prepara il messaggio per richiedere la connessione*/
			strcpy( nomePassword, argv[1] );
			strcat( nomePassword, ":" );
			strcat( nomePassword, argv[2]);
			
			newMOut.type = MSG_CONNECT;
			newMOut.length = strlen( nomePassword ) + 1;
			newMOut.buffer = malloc( sizeof(char) * newMOut.length );
			strcpy ( newMOut.buffer, nomePassword );
				
			/* invio messaggio al server */	
			if ( sendMessage( fd, &newMOut ) == -1 ) exit(EXIT_FAILURE);
			free(newMOut.buffer);
			do{	
			
			/*ricezione del messaggio in risposta e gestione errore; casi errore nella receive, MSG_OK, MSG_NO, MSG_ERR, MSG_WAIT*/
			esito = receiveMessage( fd, &newMIn );
			
			if( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				exit(EXIT_FAILURE);
			}	
			
			if (esito == ENOTCONN){
				closeConnection(fd);
				perror("Errore di Connessione");
				exit(EXIT_FAILURE);
			}	
				
			if ( newMIn.type == MSG_NO ){
				fprintf(stdout, "%s\n", newMIn.buffer);
				exit(EXIT_SUCCESS);
			}
		
			if ( newMIn.type == MSG_ERR ){
				fprintf(stderr, "%s\n", newMIn.buffer);
				exit(EXIT_FAILURE);
			}
		
			if ( newMIn.type == MSG_OK ) {
				/*invia un messaggio MSG_OK con l'utente che vuole sfidare, oppure MSG_WAIT se preferisce essere sfidato*/
				checkSfidato = 0;/*controlla se il client preferisce mettersi in attesa di uno sfidante*/
				
				/*pone a MSG_NO il tipo del messaggio in ingresso: quando cambierà uscirà dal ciclo*/
				newMIn.type = MSG_NO;
				
				/*richiede all'utente di scrivere il nome di un avversario tra quelli possibili o di mettersi in attesa, legge l'input*/
					
						fprintf( stdout, "Possibili avversari: %s\nWAIT per attendere una sfida\nScelta --? ", newMIn.buffer );
						scanf(  "%s", sfidato );
						scanf("%c",&aux);
						
					/*l'utente sceglie di attendere*/
				  	if ( strcmp ( sfidato, "WAIT" ) == 0 ){
			  			checkSfidato = 1; 
			  		}
					/*preparazione del messaggio per la sfida*/
					if (!checkSfidato){

						newMOut.type = MSG_OK;
						newMOut.length = strlen( sfidato ) + 1;
						newMOut.buffer = malloc( sizeof(char) * (newMOut.length) );
						strcpy ( newMOut.buffer, sfidato );

					}
					
					else { /*preparazione del messaggio per l'attesa*/

						newMOut.type = MSG_WAIT;
						newMOut.length = strlen( argv[1] ) + 1;
						newMOut.buffer = malloc( sizeof(char) * (newMOut.length) );
						strcpy ( newMOut.buffer, argv[1] );

					}
					/*invio messaggio*/	
					if ( sendMessage( fd, &newMOut ) == -1 ) exit(EXIT_FAILURE);
					free(newMOut.buffer);
					
					esito = receiveMessage( fd, &newMIn );
			
					if( esito == -1 ){
						perror("Errore nello Scambio di Messaggi");
						exit(EXIT_FAILURE);
					}	
					
					if (esito == ENOTCONN){
						closeConnection(fd);
						perror("Errore di Connessione");
						exit(EXIT_FAILURE);
					}	
				
				}
			}while (newMIn.type == MSG_NO && !checkSfidato);
			
		if ( newMIn.type == MSG_WAIT ){ /*se il messaggio iniziale era un MSG_WAIT o se il server conferma che l'utente può mettersi in attesa*/
			free(newMIn.buffer);
			esito = receiveMessage( fd, &newMIn );
			
			if( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				exit(EXIT_FAILURE);
			}	
			
			if (esito == ENOTCONN){
				closeConnection(fd);
				perror("Errore di Connessione");
				exit(EXIT_FAILURE);
			}	
			/*inizia la partita, turno dello sfidante*/		
			if( newMIn.type == MSG_OK ){
				if (joinMatch(fd, 0, argv[1]) == -1) exit(EXIT_FAILURE);
			}
				
		}
		/* si riceve MSG_OK, inizia la partita, turno dell'utente*/	
		else if ( newMIn.type == MSG_OK ){
				if(joinMatch(fd, 1, argv[1])==-1) exit(EXIT_FAILURE);
			}
		/*il server invia un messaggio privo di significato*/
		else {
			fprintf(stderr, "%c-%d-%s\n", newMIn.type, newMIn.length, newMIn.buffer);
		}
	}
			
		/*quattro argomenti?*/
		else if ( argc == 4 ){
			/*registrazione: preparazione messaggio per il server*/
			if ( strcmp( argv[3], "-r") == 0 ){
			
					strcpy( nomePassword, argv[1] );
					strcat( nomePassword, ":" );
					strcat( nomePassword, argv[2]);
				
					newMOut.type = MSG_REG;
					newMOut.length = strlen( nomePassword ) + 1;
					newMOut.buffer = malloc( sizeof(char) * (newMOut.length) );
					strcpy ( newMOut.buffer, nomePassword );
			 
			}
			/*cancellazione: preparazione messaggio per il server*/
			if ( strcmp( argv[3], "-c") == 0 ){
				
					strcpy( nomePassword, argv[1] );
					strcat( nomePassword, ":" );
					strcat( nomePassword, argv[2]);
				
					newMOut.type = MSG_CANC;
					newMOut.length = strlen( nomePassword ) + 1;
					newMOut.buffer = malloc( sizeof(char) * newMOut.length );
					strcpy ( newMOut.buffer, nomePassword );
			}
		
			/* invio messaggio al server */	
			if ( sendMessage( fd, &newMOut ) == -1 ) exit(EXIT_FAILURE);
			free(newMOut.buffer);
		
			/*ricezione del messaggio in risposta e gestione errore; casi errore nella receive, MSG_OK, MSG_NO, MSG_ERR*/
			esito = receiveMessage( fd, &newMIn );
			
			if( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				exit(EXIT_FAILURE);
			}	
			
			if (esito == ENOTCONN){
				closeConnection(fd);
				perror("Errore di Connessione");
				exit(EXIT_FAILURE);
			}	
			
			if ( newMIn.type == MSG_OK ) {
				fprintf(stdout, "Success!!!\n");
				exit(EXIT_SUCCESS);
			}
		
			if ( newMIn.type == MSG_NO ){
				fprintf(stdout, "%s\n", newMIn.buffer);
				exit(EXIT_SUCCESS);
			}
		
			if ( newMIn.type == MSG_ERR ){
				fprintf(stderr, "%s\n", newMIn.buffer);
				exit(EXIT_FAILURE);
			}			
				
		}
		if ( closeConnection((int)fd) == -1 ) exit(EXIT_FAILURE);
		exit(EXIT_SUCCESS);
}









