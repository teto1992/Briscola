  /** \file brsclient.c
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
#include"newMazzo_r.h"
#define SOCK_PATH "./tmp/briscola.skt"
#define FOUT "brs.checkpoint"

int manageMatch(char* playerA, char* playerB, int fda, int fdb, bool_t test, int nPart ){ /* playerA è il giocatore che inizia, playerB l'avversario*/

	char semeCarta[] = {'C','Q','F', 'P'};
	message_t msg;
	char tmp[3];
	int i;
	mazzo_t *m;
	/*creazione mazzo*/
	m = (mazzo_t*) newMazzo_r(test);
	carta_t *cartaA, *cartaB, *cartaTmp;
	FILE *fout;/*file di log*/
	carta_t **puntiA, **puntiB; /*talloni dei due giocatori*/
	int a=0, punteggioA = 0; /*indice per il tallone di A e variabile in cui salvare il punteggio di A*/
	int b=0, punteggioB = 0; /*indice per il tallone di B e variabile in cui salvare il punteggio di A*/
	char punteggioWinner[4]; /*array per memorizzare il punteggio del vincitore come stringa*/
	char buffA[10+LUSER];
	char buffB[10+LUSER];
	bool_t turnoA = TRUE;
	char fileName[255];/*conterrà il nome del file di log, in UNIX i file hanno nomi di al più 256 caratteri*/
	int j;
	int esito; /*registra l'esito di receiveMessage*/
	
	/*apertura del file di log*/
	sprintf(fileName, "./BRS-%d.log", nPart);
	if ( (fout = fopen(fileName, "w")) == NULL ) return -1;
	fprintf(fout, "%s:%s\n", playerA, playerB); 

	/*si allocano i due talloni*/
	if ((puntiA = malloc(NCARTE*sizeof(carta_t*)))==NULL) return -1;
	if ((puntiB = malloc(NCARTE*sizeof(carta_t*)))==NULL) return -1;
	/*inizializzazione a NULL di tutte le carte dei talloni*/
	for (i=0; i < NCARTE;i++){
		puntiA[i]=NULL;
		puntiB[i]=NULL;
	}

	fprintf(fout,"BRISCOLA:%c\n", semeCarta[m->briscola]);

	buffA[0] = semeCarta[m->briscola];
	buffA[1] = ':';

	buffB[0] = semeCarta[m->briscola];
	buffB[1] = ':';

	for (i = 2; i < 7; i = i + 2){

		if ((cartaTmp = getCard(m)) == NULL) return -1;

		cardToString(tmp,cartaTmp);

		buffA[i]=tmp[0];
		buffA[i+1]=tmp[1];

		if ((cartaTmp = getCard(m)) == NULL) return -1;

		cardToString(tmp,cartaTmp);

		buffB[i]=tmp[0];
		buffB[i+1]=tmp[1];
	}

	buffA[8] = ':';
	buffB[8] = ':';

	strncpy (buffA + 9, playerB, LUSER+1 );
	strncpy (buffB + 9, playerA, LUSER+1 );

	msg.type = MSG_STARTGAME;
	msg.length = strlen(buffA) + 1;
	msg.buffer = malloc(msg.length*sizeof(char));
	msg.buffer = strcpy(msg.buffer, buffA);

	if (sendMessage(fda, &msg) == -1) return -1;
	free(msg.buffer);

	msg.type = MSG_STARTGAME ;
	msg.length = strlen(buffB) + 1;
	msg.buffer = malloc(msg.length*sizeof(char));
	msg.buffer = strcpy(msg.buffer, buffB);

	if (sendMessage(fdb, &msg) == -1) return -1;
	free(msg.buffer);

	/* le prime diciassette mani prevedono che si peschino carte dal mazzo*/
	for (i = 0; i < 17; i++){
		
		if (turnoA==TRUE){
			/*si riceve il MSG_PLAY di A, si salva la carta, lo si inoltra a B*/
			esito = receiveMessage(fda, &msg );
			
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			
			cartaA = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerA, msg.buffer);
			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);

			/*si riceve il MSG_PLAY di B, si salva la carta, lo si inoltra a A*/
			esito = receiveMessage(fdb, &msg );
			
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s\n", playerB, msg.buffer);
			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaA, cartaB)==TRUE)
				turnoA = TRUE;
			else 
				turnoA = FALSE;
			
		}
		else {
			esito=receiveMessage(fdb, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerB, msg.buffer);
			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);

			esito = receiveMessage(fda, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			cartaA = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s\n", playerA, msg.buffer);
			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaB, cartaA)==TRUE)
				turnoA=FALSE;
			else 
				turnoA = TRUE;
		}

		if(turnoA==TRUE){
			
			if ((cartaTmp = getCard(m)) == NULL) return -1;

			cardToString(tmp,cartaTmp);

			if ( (puntiA[a] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiA[a + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiA[a]->seme = cartaA->seme;
			puntiA[a]->val = cartaA->val;
			puntiA[a+1]->seme = cartaB->seme;
			puntiA[a+1]->val = cartaB->val;

			free(cartaA);
			free(cartaB);
			a = a + 2;

			msg.type = MSG_CARD;
			msg.length = strlen(tmp) + 3;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 't';
			msg.buffer[1] = ':';
			msg.buffer[2] = tmp[0];
			msg.buffer[3] = tmp[1];
			msg.buffer[4] = '\0';

			if(sendMessage(fda, &msg)==-1)return -1;
			free(msg.buffer);

			/*B pesca per secondo*/
			if ((cartaTmp = getCard(m)) == NULL) return -1;

			cardToString(tmp,cartaTmp);

			msg.type = MSG_CARD;
			msg.length = strlen(tmp) + 3;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 'a';
			msg.buffer[1] = ':';
			msg.buffer[2] = tmp[0];
			msg.buffer[3] = tmp[1];
			msg.buffer[4] = '\0';

			if(sendMessage(fdb, &msg)==-1)return -1;
			free(msg.buffer);

		}

		else {
			/*ha vinto B, B pesca per primo*/
			if ((cartaTmp = getCard(m)) == NULL) return -1;

			cardToString(tmp,cartaTmp);
			
			if ( (puntiB[b] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiB[b + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiB[b]->seme = cartaB->seme;
			puntiB[b]->val = cartaB->val;
			puntiB[b+1]->seme = cartaA->seme;
			puntiB[b+1]->val = cartaA->val;

			free(cartaA);
			free(cartaB);

			b = b + 2;

			

			msg.type = MSG_CARD;
			msg.length = strlen(tmp) + 3;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 't';
			msg.buffer[1] = ':';
			msg.buffer[2] = tmp[0];
			msg.buffer[3] = tmp[1];
			msg.buffer[4] = '\0';

			if(sendMessage(fdb, &msg)==-1)return -1;
			free(msg.buffer);

			/*A pesca per secondo*/
			if ((cartaTmp = getCard(m)) == NULL) return -1;

			cardToString(tmp,cartaTmp);

			msg.type = MSG_CARD;
			msg.length = strlen(tmp) + 3;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 'a';
			msg.buffer[1] = ':';
			msg.buffer[2] = tmp[0];
			msg.buffer[3] = tmp[1];
			msg.buffer[4] = '\0';

			if(sendMessage(fda, &msg)==-1)return -1;
			free(msg.buffer);
		}
	}
	
	/*prime due mani senza pescare carte*/
	for ( i = 0 ; i<2; i++){
	
		if(turnoA==TRUE){
			/*si riceve il MSG_PLAY di A, si salva la carta, lo si inoltra a B*/
			esito = receiveMessage(fda, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			cartaA = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerA, msg.buffer);

			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);

			/*si riceve il MSG_PLAY di B, si salva la carta, lo si inoltra a A*/
			esito = receiveMessage(fdb, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s\n", playerB, msg.buffer);
		
			
		
			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaA, cartaB)==TRUE)
				turnoA = TRUE;
			else 
				turnoA = FALSE;
		}

		else{
			/*si riceve il MSG_PLAY di B, si salva la carta, lo si inoltra a A*/
			receiveMessage(fdb, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerB, msg.buffer);

			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);

			/*si riceve il MSG_PLAY di A, si salva la carta, lo si inoltra a B*/
			esito = receiveMessage(fda, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			cartaA = stringToCard(msg.buffer);
			
			fprintf(fout, "%s:%s\n", playerA, msg.buffer);
			
			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaB, cartaA)==TRUE)
				turnoA=FALSE;
			else 
				turnoA = TRUE;

		}

		if (turnoA==TRUE){
			/*ha vinto A*/

			if ( (puntiA[a] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiA[a + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiA[a]->seme = cartaA->seme;
			puntiA[a]->val = cartaA->val;
			puntiA[a+1]->seme = cartaB->seme;
			puntiA[a+1]->val = cartaB->val;

			free(cartaA);
			free(cartaB);

			a = a + 2;

			msg.type = MSG_CARD;
			msg.length = 5;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 't';
			msg.buffer[1] = ':';
			msg.buffer[2] = '-';
			msg.buffer[3] = '-';
			msg.buffer[4] = '\0';

			if(sendMessage(fda, &msg)==-1)return -1;
			free(msg.buffer);

			/*B giocherà per secondo*/
			msg.type = MSG_CARD;
			msg.length = 5;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 'a';
			msg.buffer[1] = ':';
			msg.buffer[2] = '-';
			msg.buffer[3] = '-';
			msg.buffer[4] = '\0';

			if(sendMessage(fdb, &msg)==-1)return -1;
			free(msg.buffer);
		}

		else {
			/*ha vinto B*/

			if ( (puntiB[b] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiB[b + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiB[b]->seme = cartaB->seme;
			puntiB[b]->val = cartaB->val;
			puntiB[b+1]->seme = cartaA->seme;
			puntiB[b+1]->val = cartaA->val;

			free(cartaA);
			free(cartaB);

			b = b + 2;

			msg.type = MSG_CARD;
			msg.length = 5;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 't';
			msg.buffer[1] = ':';
			msg.buffer[2] = '-';
			msg.buffer[3] = '-';
			msg.buffer[4] = '\0';

			if(sendMessage(fdb, &msg)==-1)return -1;
			free(msg.buffer);

			/*A giocherà per secondo*/

			msg.type = MSG_CARD;
			msg.length = 5;
			msg.buffer = malloc(msg.length*sizeof(char));
			msg.buffer[0] = 'a';
			msg.buffer[1] = ':';
			msg.buffer[2] = '-';
			msg.buffer[3] = '-';
			msg.buffer[4] = '\0';
			if(sendMessage(fda, &msg)==-1)return -1;
			free(msg.buffer);

		}

	}
	

	/*ultima mano*/
	if(turnoA==TRUE){
			/*si riceve il MSG_PLAY di A, si salva la carta, lo si inoltra a B*/
			esito=receiveMessage(fda, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			cartaA = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerA, msg.buffer);

			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);

			/*si riceve il MSG_PLAY di B, si salva la carta, lo si inoltra a A*/
			esito=receiveMessage(fdb, &msg );
			
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s\n", playerB, msg.buffer);
			
			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaA, cartaB)==TRUE)
				turnoA = TRUE;
			else 
				turnoA = FALSE;
		}

		else{
			/*si riceve il MSG_PLAY di B, si salva la carta, lo si inoltra a A*/
			esito = receiveMessage(fdb, &msg );
			
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fda, &msg) == -1) return -1;
				closeConnection(fda);
				free(msg.buffer);
				return -1;
			}
			
			cartaB = stringToCard(msg.buffer);
			fprintf(fout, "%s:%s#", playerB, msg.buffer);

			if( sendMessage(fda, &msg) == -1) return -1;
			free(msg.buffer);

			/*si riceve il MSG_PLAY di A, si salva la carta, lo si inoltra a B*/
			esito=receiveMessage(fda, &msg );
			if ( esito == -1 ){
				perror("Errore nello Scambio di Messaggi");
				return -1;
			}
			
			if ( esito == ENOTCONN ) {
				perror("Avversario non Connesso");
				msg.type = MSG_ENDGAME;
				msg.length = 50;
				msg.buffer = malloc( msg.length*sizeof(char));
				msg.buffer = "L'avversario è uscito";
				if( sendMessage(fdb, &msg) == -1) return -1;
				closeConnection(fdb);
				free(msg.buffer);
				return -1;
			}
			cartaA = stringToCard(msg.buffer);
			
			fprintf(fout, "%s:%s\n", playerA, msg.buffer);
			
			if( sendMessage(fdb, &msg) == -1) return -1;
			free(msg.buffer);
			
			if (compareCard(m->briscola, cartaB, cartaA)==TRUE)
				turnoA=FALSE;
			else 
				turnoA = TRUE;

		}
		
		
		if (turnoA==TRUE){
			/*ha vinto A*/

			if ( (puntiA[a] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiA[a + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiA[a]->seme = cartaA->seme;
			puntiA[a]->val = cartaA->val;
			puntiA[a+1]->seme = cartaB->seme;
			puntiA[a+1]->val = cartaB->val;
			
			a = a + 2;

			free(cartaA);
			free(cartaB);

		}

		else {
			/*ha vinto B*/

			if ( (puntiB[b] = malloc(sizeof(carta_t))) == NULL) return -1;
			if ( (puntiB[b + 1] = malloc(sizeof(carta_t))) == NULL) return -1;
			puntiB[b]->seme = cartaB->seme;
			puntiB[b]->val = cartaB->val;
			puntiB[b+1]->seme = cartaA->seme;
			puntiB[b+1]->val = cartaA->val;
			
			b = b + 2;

			free(cartaA);
			free(cartaB);

		}
	
	/*computo dei punteggi conseguiti da A e B*/
	punteggioA = computePoints(puntiA, a);
	punteggioB = computePoints(puntiB, b);
	
	freeMazzo(m);
	for(j=0; j<a; j++){
		free(puntiA[j]);
	}
	
	for(j=0; j<b; j++){
		free(puntiB[j]);
	}
	
	
	/*ha vinto A, vince A anche nel caso in cui si pareggi*/
	if ( punteggioA >= punteggioB ){
		/*preparazione e invio del messaggio END_GAME a entrambi*/
		fprintf(fout, "WINS:%s\nPOINTS:%d\n", playerA, punteggioA);
		sprintf(punteggioWinner, "%d", punteggioA);
		msg.type = MSG_ENDGAME;
		msg.length = strlen(playerA) + strlen(punteggioWinner) + 2;
		msg.buffer = malloc(msg.length*sizeof(char));
		strcpy(msg.buffer, playerA);
		strcat(msg.buffer,":" );
		strcat(msg.buffer, punteggioWinner);

		if(sendMessage(fda, &msg)==-1)return -1;
		if(sendMessage(fdb, &msg)==-1)return -1;
		free(msg.buffer);

	}
	/* ha vinto B*/
	else {
		/*preparazione e invio del messaggio END_GAME a entrambi*/
		fprintf(fout, "WINS:%s\nPOINTS:%d\n", playerB, punteggioB);
		sprintf(punteggioWinner, "%d", punteggioB);
		msg.type = MSG_ENDGAME;
		msg.length = strlen(playerB) + strlen(punteggioWinner) + 2;
		msg.buffer = malloc(msg.length*sizeof(char));
		strcpy(msg.buffer, playerB);
		strcat(msg.buffer,":" );
		strcat(msg.buffer, punteggioWinner);
		
		if(sendMessage(fdb, &msg)==-1)return -1;
		if(sendMessage(fda, &msg)==-1)return -1;
		free(msg.buffer);

	}
	
	fclose(fout);
	return 0;
}
