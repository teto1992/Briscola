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


/** Gestisce la partita lato client 
 *   \param fd file descriptor della socket
 *   \param turno indica se il turno è di chi ha chiamato la funzione (=1) o meno (=0)
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore 
 */

int joinMatch ( int fd, int turno, char* u){
	
	message_t newMIn;
	message_t newMOut;
	char avversario[LUSER+1];
	char *cartaGiocata;
	char c1[3], c2[3], c3[3];
	char cartaTmp[3];
	int cartaGiusta = 0;
	int i;
	char aux;
	char *pointsPtr;
	int esito; /*registra l'esito della receiveMessage*/
	
		
	esito = receiveMessage(fd,&newMIn);
	
	if( esito == ENOTCONN ){
		closeConnection(fd);
		perror("Errore di Connessione");
		return -1;
	}
	
	if ( esito == -1 ){
		perror("Errore nello Scambio di Messaggi");
		return -1;
	}
	
	if ( newMIn.type != MSG_STARTGAME ) {
		return -1;
	}
	/*salvataggio del nome dell'avversario*/		
	strncpy(avversario, (newMIn.buffer) + 9, LUSER + 1);	
	printf("Giochiamo con %s\nBriscola: %c\n", avversario, newMIn.buffer[0]);
	
	/*costruzione prima mano*/
	
	c1[0] = newMIn.buffer[2];
	c1[1] = newMIn.buffer[3];
	c1[2] = '\0';
	c2[0] = newMIn.buffer[4];
	c2[1] = newMIn.buffer[5];
	c2[2] = '\0';
	c3[0] = newMIn.buffer[6];
	c3[1] = newMIn.buffer[7];
	c3[2] = '\0';
	
	i = 0;
	
	/*finchè il server non comunica la fine della partita...*/			
	while ( newMIn.type != MSG_ENDGAME ){
		/*verifica su chi gioca per primo nei turni intermedi, sostituzione carta giocata con carta pescata*/
		if ( newMIn.type == MSG_CARD ){
			if ( newMIn.buffer[0] == 't' ) turno = 1;
			
			else turno = 0;
			
			strncpy(cartaGiocata, newMIn.buffer + 2, 3);
		}
		
		cartaGiusta = 0;
		
		printf( "Carte: %s %s %s\n", c1, c2, c3);
				/*l'utente gioca per primo*/
				if (turno == 1 ){
					
					/*finchè non si è scelta una carta tra quelle nella mano*/
					printf("Turno: %s --? ", u);
					
					while ( !cartaGiusta ){
					
						fgets(cartaTmp, 3, stdin);
						while ( getchar() != '\n' );
						
						/*la carta è uguale a una di quelle possedute dall'utente?*/
						
						if ( strncmp(cartaTmp, c1, 3) == 0) {
							cartaGiocata = c1;
							cartaGiusta = 1;
						}
						
						 if ( strncmp(cartaTmp, c2, 3) == 0) {
							cartaGiocata = c2;
							cartaGiusta = 1;
						}
						
						 if ( strncmp(cartaTmp, c3, 3) == 0) {
							cartaGiocata = c3;
							cartaGiusta = 1;
						}		
						
						if ( strncmp(cartaTmp, "--", 3) == 0 ){
							cartaGiusta = 0;
						}
						
						
						if(!cartaGiusta) printf("\nInserisci una carta tra quelle in tuo possesso!\n");		
						
					} 
					
					
					
					/*preparazione del messaggio contenente la carta da giocare*/
					newMOut.type = MSG_PLAY;
					newMOut.length = 3;
					newMOut.buffer = malloc( sizeof(char) * (newMOut.length) );
					strcpy ( newMOut.buffer, cartaTmp );
					/*invio al server*/
					if ( sendMessage( fd, &newMOut ) == -1 ) return -1;	
					free(newMOut.buffer);
					
					printf("Turno: %s --> ", avversario);
					
					/*ricezione dal server del messaggio relativo all'avversario*/
					free(newMIn.buffer);
					esito = receiveMessage(fd,&newMIn);
					if( esito == ENOTCONN ){
						closeConnection(fd);
						perror("Errore di Connessione");
						return -1;
					}
	
					if ( esito == -1 ){
						perror("Errore nello Scambio di Messaggi");
						return -1;
					}
			
					/*stampa della carta giocata dall'avversario*/
					if ( newMIn.type == MSG_PLAY ){
						printf("%s\n", newMIn.buffer);
					}
					else return -1;
				}
			/*l'avversario gioca per primo, il turno si svolge specularmente al precedente*/	
			if ( turno == 0 ){
					printf("Turno: %s --> ", avversario);
				    /*ricezione dal server del messaggio relativo all'avversario*/
				    free(newMIn.buffer);
					esito = receiveMessage(fd,&newMIn);
					if( esito == ENOTCONN ){
						closeConnection(fd);
						perror("Errore di Connessione");
						return -1;
					}
	
					if ( esito == -1 ){
						perror("Errore nello Scambio di Messaggi");
						return -1;
					}
					/*stampa della carta giocata dall'avversario*/
					if ( newMIn.type == MSG_PLAY ){
						printf("%s\n", newMIn.buffer);
					}
					else return -1;
					/*finchè non si è scelta una carta tra quelle nella mano*/
					
					fprintf( stdout, "Turno: %s --? ", u);
					
					while ( !cartaGiusta ){
										
						fgets(cartaTmp, 3, stdin);
						while ( getchar() != '\n' );
						
						/*la carta è uguale a una di quelle possedute dall'utente?*/
						if ( strncmp(cartaTmp, c1, 3) == 0) {
							cartaGiocata = c1;
							cartaGiusta = 1;
						}
						
						else if ( strncmp(cartaTmp, c2, 3) == 0) {
							cartaGiocata = c2;
							cartaGiusta = 1;
						}
						
						else if ( strncmp(cartaTmp, c3, 3) == 0) {
							cartaGiocata = c3;
							cartaGiusta = 1;
						}	
						
						if ( strncmp(cartaTmp, "--", 3) == 0 ){
							cartaGiusta = 0;
						}
						
						
						if(!cartaGiusta) printf("\nInserisci una carta tra quelle in tuo possesso!\n");			
						
					} 
					
					/*preparazione del messaggio contenente la carta da giocare*/
					newMOut.type = MSG_PLAY;
					newMOut.length = 3;
					newMOut.buffer = malloc( sizeof(char) * (newMOut.length) );
					strcpy ( newMOut.buffer, cartaTmp );
					/*invio al server*/
					if ( sendMessage( fd, &newMOut ) == -1 ) return -1;	
					free(newMOut.buffer);
			}
		/*ricezione messaggio successivo: MSG_CARD o MSG_ENDGAME*/
					free(newMIn.buffer);
					esito = receiveMessage(fd,&newMIn);
					if( esito == ENOTCONN ){
						closeConnection(fd);
						perror("Errore di Connessione");
						return -1;
					}
	
					if ( esito == -1 ){
						perror("Errore nello Scambio di Messaggi");
						return -1;
					}	

	}
	
	
	if (newMIn.type == MSG_ENDGAME){
		
		pointsPtr = strtok( newMIn.buffer, ":");
		pointsPtr = strtok(NULL, ":");
	
		printf("Vince %s con %s!\nBye!\n", newMIn.buffer, pointsPtr);
		
		free(newMIn.buffer);
		return 0;

	}
	
	return -1;

}


