  /** \file comsock.c
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */
     
#include"comsock.h"

/* -= FUNZIONI =- */
/** Crea una socket AF_UNIX
 *  \param  path pathname della socket
 *
 *  \retval s    il file descriptor della socket  (s>0)
 *  \retval -1   in altri casi di errore (setta errno)
 *               errno = E2BIG se il nome eccede UNIX_PATH_MAX
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
int createServerChannel(char* path){
	
	int s = 0;
	struct sockaddr_un sa;
	
	/* verifica che gli argomenti passati siano accettabili*/
	
	if ( path == NULL  ||  strcmp(path, "") == 0  ) {
		errno=EINVAL;
		return -1;
	}	
	
	if ( strlen(path) > UNIX_PATH_MAX ) {
		errno = E2BIG;
		return (-1);
	}
	
	/*inizializza la socket*/
	
	sa.sun_family = AF_UNIX;
	strcpy (sa.sun_path, path);
	
	/* esegue socket, bind, si mette in ascolto con listen sulla socket */
	
	if ( (s = socket(AF_UNIX, SOCK_STREAM, 0) ) != -1 ) {
		if ( bind(s, (struct sockaddr *)&sa, sizeof(sa)) != -1 ){ 
			if ( listen (s, SOMAXCONN ) != -1 ){
				return s;
			}
			else{/*gestione fallimento listen*/
				close(s);
				unlink(path);
				return (-1);
			}	
		}
		/*gestione fallimento bind*/
		else { 
			close(s);
			unlink(path);
			return (-1);
		}
	}
	else {
	/*gestione fallimento socket*/
		return (-1);
	}
	
} 
	

/** Chiude un canale lato server (rimuovendo la socket dal file system) 
 *   \param path path della socket
 *   \param s file descriptor della socket
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore (setta errno)
 */
int closeServerChannel(char* path, int s){
	/*verifica che i parametri passati siano corretti*/
	if ( s<=0 || path == NULL || strcmp(path, "") == 0 ) {
		errno=EINVAL;
		return -1;
	}	
	/*esegue unlink e chiude la socket*/
	if ( unlink(path) != -1 && close(s) != -1 ) return 0;
	return -1;
}

/** accetta una connessione da parte di un client
 *  \param  s socket su cui ci mettiamo in attesa di accettare la connessione
 *
 *  \retval  c il descrittore della socket su cui siamo connessi
 *  \retval  -1 in casi di errore (setta errno)
 */
int acceptConnection(int s){
	/*verifica che i parametri passati siano corretti*/
	if ( s<=0 ) {
			errno=EINVAL;
			return -1;
		}	
	/* esegue l'accept (i valori di ritorno coincidono)*/
	return accept(s, NULL, 0 );
	
}

/** legge un messaggio dalla socket --- attenzione si richiede che il messaggio sia adeguatamente spacchettato e trasferito nella struttura msg
 *  \param  sc  file descriptor della socket
 *  \param msg  indirizzo della struttura che conterra' il messagio letto 
 *		(deve essere allocata all'esterno della funzione) il campo 
                 buffer viene allocato all'interno della funzione in base 
		 alla lunghezza ricevuta)
 *
 *  \retval lung  lunghezza del buffer letto, se OK 
 *  \retval  -1   in caso di errore (setta errno)
 *                 errno = ENOTCONN se il peer ha chiuso la connessione 
 *                   (non ci sono piu' scrittori sulla socket)
 *      
 */
int receiveMessage(int sc, message_t * msg){
	
	int lung = 0;
	char num[11];
	int n = 0;
	/*verifica che i parametri passati siano corretti*/
	if ( msg == NULL || sc <= 0 ) {
		errno = EINVAL;
		return -1;
	}
	/*legge un char per il tipo, lung diventa 1*/	
	if ( ( lung = read( sc, &(msg->type), sizeof(char)) ) == -1 ) return -1;
	/*verifica che il peer non abbia chiuso la connessione*/
	if ( lung == 0 ) {
		errno = ENOTCONN;
		return -1;
	}
	/*legge dieci char per la lunghezza*/	
	if (  ( n = read(sc, num, 10*sizeof(char) ) )  == -1 )  return -1;
		/*verifica che il peer non abbia chiuso la connessione*/
		if ( n == 0 ) {
			errno = ENOTCONN;
			return -1;
		}
	/*chiude il vettore contenente la lunghezza del buffer*/
	num[10]='\0';
	/* copia num[] nel campo length del messaggio */
	sscanf( num, "%u", &(msg->length) );
	
	msg->buffer = NULL;
	
	/*se il messaggio è lungo 0 (è NULL) restituisce il numero di caratteri letti: 11*/
	if ((msg->length) == 0){	
		return 11;
	}
	
	/*somma i caratteri letti a lung*/
	lung = lung + n;
	
	n = 0;
	/*alloca il buffer*/
	msg->buffer = malloc((msg->length) * sizeof(char));
	/*copia nel buffer il contenuto del messaggio*/
	if (  ( n = read(sc, (msg->buffer), (msg->length) ) )  == -1 )  return -1;
	
	/*somma i caratteri letti a lung*/	
	lung = lung + n;
	/*verifica che il peer non abbia chiuso la connessione*/
		if ( n == 0 ) {
			errno = ENOTCONN;
			return -1;
		}
		

	return lung;
		
}

/** scrive un messaggio sulla socket --- attenzione devono essere inviati SOLO i byte significativi del campo buffer (msg->length byte) --  si richiede che il messaggio venga scritto con un'unica write dopo averlo adeguatamente impacchettato
 *   \param  sc file descriptor della socket
 *   \param msg indirizzo della struttura che contiene il messaggio da scrivere 
 *   
 *   \retval  n    il numero di caratteri inviati (se scrittura OK)
 *   \retval -1   in caso di errore (setta errno)
 *                 errno = ENOTCONN se il peer ha chiuso la connessione 
 *                   (non ci sono piu' lettori sulla socket)
 */
int sendMessage(int sc, message_t *msg){
	
	int nchar;/*contatore del numero di caratteri letti*/
	char* message = NULL; /*conterrà il messaggio serializzato*/
	char taglia[11];/*conterrà la lunghezza del buffer del messaggio msg*/
	
	/*controllo dei parametri passati come argomento*/
	if ( msg == NULL || sc <=0 ){
		errno = EINVAL;
		return -1;
	}
	/*alloca il messaggio serializzato*/
	message = malloc((1+10+msg->length)*sizeof(char));
	
	if ( message == NULL ) return -1;
	/*inserisce il tipo e chiude la stringa*/
	message[0]=(msg->type);
	message[1]='\0';
	/*inserisce la lunghezza del messaggio in taglia, anteponendo simboli 0 e rappresentando il numero su dieci cifre*/
	sprintf( taglia, "%010u", msg->length);
	/*concatena taglia a message*/
	strcat(message, taglia);
	/*copia msg->buffer (se diverso da NULL) in message, ultimando la serializzazione e eseguendo la write su sc */
	if ( msg->buffer != NULL ){	
		strncat(message, msg->buffer, msg->length);
		if ( ( nchar =  write( sc, message, 11 + msg->length ) ) == 11 + msg->length ) {
			free(message);
			return 11+msg->length;
		}
		else{
		/* verifica se ci sono ancora lettori */
			if ( nchar == 0 )
				errno = ENOTCONN;
	/* se si è verificato un errore libera message e restituisce -1*/	
			free(message);
			return -1;
		}
	}
	/*se il messaggio è NULL esegue la write del messaggio con solo tipo e lunghezza  */ 
	if ( ( nchar =  write( sc, message, 11 ) ) == 11  ) {
			free(message);
			return 11;
		}
	else{
	/* verifica se ci sono ancora lettori */
		if ( nchar == 0 )
			errno = ENOTCONN;
	/* se si è verificato un errore libera message e restituisce -1*/	
		free(message);
		return -1;
	}


}



/** crea una connessione al socket del server. In caso di errore funzione ritenta ntrial volte la connessione (a distanza di k secondi l'una dall'altra) prima di ritornare errore.
 *   \param  path  nome del socket su cui il server accetta le connessioni
 *   \param  ntrial numeri di tentativi prima di restituire errore (ntrial <=MAXTRIAL)
 *   \param  k secondi l'uno dell'altro (k <=MAXSEC)
 *   
 *   \return fd il file descriptor della connessione
 *            se la connessione ha successo
 *   \retval -1 in caso di errore (setta errno)
 *               errno = E2BIG se il nome eccede UNIX_PATH_MAX
 *
 *  in caso di errore ripristina la situazione inziale: rimuove eventuali socket create e chiude eventuali file descriptor rimasti aperti
 */
 
int openConnection(char* path, int ntrial, int k){

	int fd;/*fd della socket*/
	int failed = 0; /* numero di tentativi falliti di connessione*/
	int saveEr;/*temporanea per salvare l'errore*/
	
	struct sockaddr_un sa;
	/*controlli sull'input*/
	if ( ntrial <= 0 || path == NULL || strcmp(path,"") == 0 || k <= 0 || ntrial > MAXTRIAL || k > MAXSEC ) {
		errno=EINVAL;
		return -1;
	}	
	
	if ( strlen( path ) >= UNIX_PATH_MAX ) {
		errno = E2BIG;
		return -1;
	}
	/*inizializza la socket*/
	sa.sun_family = AF_UNIX;
	strcpy (sa.sun_path, path);
	
	if ( ( fd = socket(AF_UNIX, SOCK_STREAM, 0) ) == -1 ) {
			return (-1);
		}
		
	/*tenta ntrial volte la connessione attendendo k tra un tentativo e l'altro*/
	while ( failed < ntrial ){
		if ( ( connect ( fd, (struct sockaddr *)&sa, sizeof(sa) ) ) == -1 ){
			failed++;
			sleep(k);
		}
		else return fd;
	}
	/* se non è riuscito a connettersi salva l'errore che dovrà assegnare a errno*/
	saveEr = errno;
	/* connessione non riuscita, unlink e close */
		unlink(path);
		close(fd);
		errno=saveEr;
		return -1;
			
}

/** Chiude una connessione
 *   \param s file descriptor della socket relativa alla connessione
 *
 *   \retval 0  se tutto ok, 
 *   \retval -1  se errore (setta errno)
 */
int closeConnection(int s){
	/*verifica l'input*/
	if ( s > 0 ){
		return close(s);
	}
	
	else{
		errno = EINVAL;
		return -1;
	}
		
}

