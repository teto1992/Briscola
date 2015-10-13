/** \file users.c
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */


#include <stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>
#include"users.h"


static void storeUsersRec ( FILE* fout, nodo_t* r, int *n ); /* prototipo static interno al file */

/** A partire da una stringa 

     nome_user:password

    crea una nuova struttura utente allocando la memoria
    corrispondente ed inserendo utente e password nei rispettivi campi
    
    \param r buffer da convertire
    \param l lunghezza del buffer (numero max di caratteri che si
    possono leggere senza incorrere in overrun)

  \retval p (puntatore alla nuova struttura) 
  \retval NULL se si e' verificato un errore (setta errno)
 */
user_t* stringToUser(char* r, unsigned int l){
	
	user_t *p = NULL; 				/* si dichiara il puntatore a user p */
	int i = 0, j, controllopw;		/* indici utili a indirizzare r (i,j) e a controllare che la password non sia vuota */
	
	if ( r != NULL && l <= LUSER + LPWD + 2 ){ /* se l'allocazione è andata a buon fine e il buffer non è più lungo di LUSER+LPWD+2 (i due punti e il terminatore di stringa */
	
		p = malloc( sizeof( user_t ) ); /* si alloca p */
		
		if (p != NULL ){ /*l'allocazione è andata a buon fine?*/
			
			while ( r[i] != ':' && i < l && i <= LUSER ){ /*finchè non si arriva ai due punti o si supera la lunghezza dal buffer o il nomeutente è troppo lungo */
				p->name[i] = r[i]; /* si copia r[i] in p */
				i = i + 1;
			}
			
			if ( i == 0 || i >= l || i  > LUSER || r[i] != ':' ) { /* se uno dei controlli di sopra ha portato a trovare un errore */
				free ( p ); /* si libera p*/
				errno = EINVAL; /* errore negli argomenti */
				return NULL; /* si restituisce NULL */
			}
			
			p->name[i] = '\0'; /* altrimenti si termina la stringa p->name */
			i++; /* si incrementa i ( al primo carattere di pwd ) */
			j = 0; /* si setta j a zero per indirizzare l'array p->passwd */
			controllopw = i; /* si pone controllopwd = i per verificare dopo che ci sia stato qualcosa da copiare in p->passwd*/
			
			while ( r[i] != '\0' && i < l && j <= LPWD){ /* finchè non si arriva in fondo alla stringa o si supera il numero dei cartatteri leggibili o la password è troppo lunga*/
				p->passwd[j] = r[i]; /* si copia r[i] in p */
				i = i + 1; /* si incrementa i */
				j++; /*si incrementa j*/
			}
			
			if ( i == controllopw || j == 0 || i >= l || r[i] != '\0' || j > LPWD ){ /* se uno dei controlli di sopra ha portato a trovare un errore*/
				free( p );/* si libera p*/
				errno = EINVAL;/* errore negli argomenti */
				return NULL;/* si restituisce NULL */
			}
			
			p->passwd[j] = '\0';/* si chiude la stringa p->passwd */
		
			return p; /* si restituisce p */
		}
	
		else { /* se p era NULL errore nella malloc: errno settato in automatico*/
			return NULL; /* si restituisce NULL */

		}
	}
	
	else {
		errno = EINVAL; /*se la stringa è NULL o il buffer troppo lungo: errore negli argomenti*/
		return NULL;/* si restituisce NULL */
		}
}	
		

/** A partire da una struttura utente viene creata una stringa

     nome_user:password

     la funzione alloca la memoria necessaria e termina correttamente
     la stringa risultato con il terminatore di stringa

     \param puser struttura da convertire


     \retval p (puntatore alla nuova stringa) 
     \retval NULL se si e' verificato un errore (setta errno)
 */
char* userToString(user_t* puser){

	char *s = NULL; /* stringa s da riempire */
	int i = 0, j = 0; /*indici per indirizzare gli array*/
	
	if ( puser != NULL ){ /* se l'utente passato non è NULL */
	
		s = malloc( ( strlen(puser->name) + strlen(puser->passwd) + 2 ) * sizeof(char)); /* si alloca la stringa s della giusta dimensione aggiungendo 2 per il terminatore e per i due punti */
	
		while ( puser->name[i] != '\0' ){ /* finchè non si arriva in fondo a puser->name */
			s[i] = puser->name[i]; /* lo si copia in s[i] */
			i++; /* si incrementa i*/
		}
		
		s[i] = ':'; /* si inseriscono i due punti nella stringa dopo il nomeutente*/
		i++;/*si incrementa i per copiare la password*/
		
		while ( puser->passwd[j] != '\0' ){ /* finchè non si arriva in fondo a puser->password */
			s[i] = puser->passwd[j]; /* si copia in s[i] la cella di puser->passwd indirizzata da j */
			i++; /* incrementi */
			j++;
		}
		
		s[i] = '\0';/* si chiude s*/
		
		return s; /*si restituisce s*/
	}
	
	else{ /* se l'utente passato è NULL */
		errno = EINVAL; /* errore negli argomenti */
		return NULL; /* si restituisce NULL */
	}	
}

/** Stampa l'albero su stdout
    \param r radice dell'albero da stampare
 */
void printTree(nodo_t* r){ /* classica visita simmetrica che garantisce che l'albero sia stampato in ordine alfabetico*/
	
	if ( r != NULL ){ /* se r non è NULL, altrimenti non si fa niente: CASO BASE */
	
		printTree( r->left ); /* ricorsione a sinistra */
		
		printf( "%s-%s-%d-%d", r->user->name, r->user->passwd, r->status, r->channel ); /* stampa nodo corrente */
		
		printf("\n");
		
		printTree( r->right ); /* ricorsione a destra*/
	}	

}

/** Aggiunge un nuovo utente all'albero mantenendolo ordinato lessicograficamente rispetto al campo users (ordinamento di strcmp). Se l'utente e' gia' presente non viene inserito.

 \param r puntatore alla radice dell'albero
 \param puser puntatore utente da inserire

 \retval 0 se l'inserzione e' andata a buon fine
 \retval 1 se l'utente e' gia' presente
 \retval -1 se si e' verificato un errore (setta errno)
 
 
 */
int addUser(nodo_t** r, user_t* puser){

	nodo_t *new = NULL; /* dichiarazione nuovo nodo */
	
	if ( r != NULL && puser != NULL && puser->name != NULL && puser->passwd != NULL && strlen( puser->name ) <= LUSER && strlen( puser->passwd ) <= LPWD  ) { /* r è diverso da NULL?, puser è diverso da NULL?,la password e l'utente sono diversi da NULL e hanno lunghezze lecite?  */
		
		if ( (*r) == NULL ){ /* se puser è il primo nodo da inserire oppure siamo arrivati alla foglia giusta in cui porre user*/
		
			new = malloc( sizeof( nodo_t ) ); /* si alloca new */
			
			if ( new != NULL ){ 			/*allocazione andata a buon fine?*/
				new->user = puser;		/*si assegna puser a *new */
				new->status = DISCONNECTED; /*puser è disconnesso*/
				new->channel = -1;		/* non è collegato */
				new->left = NULL;		/*si "chiude" l'albero a dx e a sx*/
				new->right = NULL;
				(*r) = new; /* si assegna new a (*r) */
				return 0;
			}
			
			else{
				
				return (-1); /* malloc setta errno */
				
			}
		}
		
		else{ /* se non è il primo nodo da inserire si cerca nell'albero la posizione corretta con chiamate ricorsive opportune o a sinistra o a destra */
			
			if ( strcmp( puser->name, (*r)->user->name ) < 0 ){ /* l'user da inserire è minore del nodo corrente */
				return addUser( &( (*r)->left ), puser );
			}
			if ( strcmp( puser->name, (*r)->user->name ) > 0 ){ /* l'user da inserire è maggiore del nodo corrente */
					return addUser( &( (*r)->right ) , puser );
			}
		
			if ( strcmp( puser->name, (*r)->user->name ) == 0 ) return 1; /* l'user da inserire è gia presente: retval 1 */
		
		}
	}
	
	else { /* else del primo if: c'è un errore negli argomenti */
		errno = EINVAL; 
		return (-1); /* restituisce -1 */
	}
	
	return -1;
	
}

/** controlla la password di un utente
 \param r radice dell'albero
 \param user  utente di cui controllare la passwd

 \retval TRUE  se l'utente e' presente e la password corrisponde
 \retval FALSE se non e' presente o la password e' errata
 */ 
bool_t checkPwd(nodo_t* r, user_t* user){

	if ( r == NULL || user == NULL ) return FALSE; /* se l'albero è NULL o siamo arrivati alle foglie OPPURE se lo user è null si restituisce FALSE */
	
	if ( strcmp ( r->user->name, user->name ) == 0 && strcmp( r->user->passwd, user->passwd) == 0 ){
		return TRUE; /* se il nome utente è nell'albero e la password è giusta*/
	}
	
	if ( strcmp ( r->user->name, user->name ) == 0 && strcmp( r->user->passwd, user->passwd) != 0 ){
		return FALSE; /* se il nome utente è nell'albero e la password è errata */
	}
	
	if ( strcmp ( r->user->name, user->name ) < 0 ){ /* se l'utente è maggiore della radice si cerca a destra ricorsivamente */
		return checkPwd ( (r->right), user );
	}
	
	else { /*altrimenti si cerca a sinistra ricorsivamente */
		return checkPwd ( (r->left), user );
	}	

}

/** Rimuove le informazioni relative ad un certo utente (se l'utente
    e' presente e la password coincide ...)(se la
    password e' a NULL non viene controllata)

    \param r puntatore alla radice dell'albero
    \param puser puntatore utente da rimuovere
    
    \retval NOUSR se l'utente non e' presente
    \retval WRPWD se la password e' errata
    \retval 0 se la rimozione e' avvenuta correttamente
    \retval -1 se si e' verificato un errore (setta errno)
    
*/
 
 		
int removeUser(nodo_t** r, user_t* puser){ 
			
		nodo_t *padre, *son; /* dichiarazione dei nodi per scorrere l'albero: padre e figlio */
		padre = NULL;
		
		if ( (*r) == NULL ) return NOUSR; /* se (*r) è NULL, l'albero è vuoto o non contiene lo user da eliminare*/
		
		if ( r == NULL || puser == NULL ) { /* se uno dei due parametri è NULL */
			errno = EINVAL; /* errore negli argomenti */
			return -1; /* si restituisce -1 */
		}
		
		if ( strcmp( puser->name, (*r)->user->name ) < 0 ){ /*altrimenti, se i parametri sono validi e lo user ricercato è minore del nodo corrente, ricerca ricorsiva a sinistra */
				return removeUser ( &((*r)->left), puser );
			}
			
	   if (strcmp( puser->name, (*r)->user->name ) > 0){ /* viceversa, ricerca ricorsiva a destra */
				return removeUser ( &((*r)->right), puser );
		}
	
		if ( puser->passwd != NULL && strcmp( (*r)->user->passwd, puser->passwd) != 0) return WRPWD; /* abbiamo trovato l'utente in questione: se la password non è NULL e è diversa da quella nell'albero si restituisce WRPWD */
			
		if ( (*r)->left == NULL && (*r)->right == NULL ){ /* se il nodo da eliminare non ha figli, è foglia */
			free( (*r)->user ); /* si libera lo user (struttura dinamica) */
			free( *r ); 			/* si libera (*r) */
			(*r) = NULL; /* lo si pone a NULL ( la foglia non c'è più */
			return 0; /* retval 0 */
		}
			
		if ( (*r)->left == NULL && (*r)->right != NULL ){ /* se ha solo il figlio destro */
			nodo_t *tmp = (*r); /* si dichiara un nodo temporaneo che memorizza il nodo da eliminare */
			(*r) = (*r)->right;	/* si crea il bypass opportuno con il figlio destro del nodo da rimuovere */		
			free( tmp->user ); /* si libera lo user (struttura dinamica) */
			free( tmp ); /*si libera il nodo da eliminare*/
			return 0; 	/* retval 0 */
			}
			
		if ( (*r)->left != NULL && (*r)->right == NULL ){ /* caso speculare al precedente*/ 
			nodo_t *tmp = (*r);
			(*r) = (*r)->left;
			free( tmp->user );
			free( tmp );
			return 0;
		}
			
		else{ /* se il nodo da rimuovere ha due figli */
			
			son = (*r)->right; /* si memorizza in son il figlio destro del nodo da rimuovere */
			
			if ( son->left == NULL ){  /* se non ha figli sinistri è il minimo del sottoalbero destro del nodo da rimuovere */
			
				son->left = (*r)->left; /* il sottoalbero sinistro di son prende il sottoalbero sinistro di r */
				padre = *r; /* assegno *r a padre per eliminarlo in seguito */
				(*r) = son; /* son diventa *r */
				free ( padre->user ); /* libero padre->user */
				free ( padre ); /*libero padre*/
				return 0; /*retval 0*/
				
			}
			
			else{ 
				
				while ( son->left != NULL ){ /* cerco il minimo del sottoalbero destro */
					padre = son;
					son = son->left;	
				}
				
				son->left = (*r)->left; /* il sottoalbero del minimo del sottoalbero destro prende il sottoalbero sinistro del nodo da eliminare */
				padre->left = son->right; /* il sottoalbero destro del padre del minimo prende il sottoalbero destro del minimo */
				son->right = (*r)->right; /* il sottoalbero destro del minimo diventa il sottoalbero destro del nodo da eliminare */
				
				free( (*r)->user ); /* libero l'utente */
				free( (*r) ); 	/* e il nodo da eliminare */
				(*r) = son; /* il puntatore del nodo eliminato prende son */
					
				return 0;
				
			}
		}
				
		
}


/** Dealloca l'albero 

 \param r radice dell'albero da deallocare
*/

void freeTree(nodo_t* r){

	if ( r != NULL ){ /* se r non è una foglia o l'albero vuoto */
		freeTree ( r->left ); /* chiamata ricorsiva a sinistra */
		freeTree ( r->right ); /* chiamata ricorsiva a destra */
		free ( r->user ); /* si liberano user e nodo corrente */
		free ( r );
	}
	
}

/** Legge il file che contiene gli utenti ammessi e le loro password e
 *  li aggiunge all'albero di ricerca passato come parametro. 

 Gli utenti sono memorizzati su file nel formato

 nome_utente:password

 e separati da un '\n' 
 
 \param fin il file di ingresso
 \param r il puntatore al puntatore alla radice dell'albero

 \retval n il numero di utenti letti ed inseriti nell'albero se tutto e' andato a buon fine
 \retval -1 se si e' verificato un errore (setta errno)
 */
int loadUsers(FILE* fin, nodo_t** r){

	char *s = malloc( (2+LUSER+LPWD)*sizeof(char));
	user_t *U; 
	int c = 0; /* contatore degli utenti messi nell'albero */
	
	if ( s != NULL ){  /*l'allocazione è andata a buon fine?*/
	
		if ( fin != NULL && r != NULL ){ 
		/* lettura del file riga per riga e inserimento utenti */
			while ( !feof( fin ) ){ 
				fscanf( fin, "%s\n", s); 
				if (( U = stringToUser ( s, strlen (s) + 1 ))==NULL){
					errno = EINVAL;
					return -1;
				} 
				addUser(r, U); 
				c++; 
			}
		}
			
		else { /* altrimenti errore nei parametri*/
			errno = EINVAL;
			return -1;
		}
		
		free (s);
		return c;
		
		}
	
	else return -1; /* malloc setta automaticamente ENOMEM */
}
			

/** Scrive tutti i permessi nell'albero su file in ordine
    lessicografico. Ogni utente e' scritto nel formato nome_utente:password
 separati da '\n'
 
 \param fout il file su cui scrivere 
 \param r il puntatore alla radice dell'albero
 
 \retval n il numero di utenti registrati nel file
 \retval -1 se si e' verificato un errore   (setta errno)
 */

static void storeUsersRec ( FILE* fout, nodo_t* r, int *n )
{
	/* funzione ricorsiva ausiliaria alla storeUsers: stampa su file con visita simmetrica e in ordine lessicografico */
	if ( r != NULL ){ 
		storeUsersRec( fout, r->left, n );
		fprintf( fout, "%s:%s\n", r->user->name, r->user->passwd );
		(*n)++;
		storeUsersRec( fout, r->right, n );
	}
	
	else return;		
}

int storeUsers(FILE* fout, nodo_t* r){

int N = 0; /* conta gli utenti registrati nel file */
	
	if ( fout != NULL ){
		
			storeUsersRec( fout, r, &N ); 
			return N;
			
	}	
	
	else { /* input sbagliato */ 
	
		errno = EINVAL;
		return -1;
		
	}
	
}


/** utente non registrato */
#define NOTREG (-10)

/** Restituisce lo stato di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare

 \retval s stato dell'utente se l'utente e' presente
 \retval NOTREG se non e' presente
*/
status_t getUserStatus(nodo_t* r, char* u){


	if ( r != NULL ){ /* foglia o albero vuoto? seguono le opportune chiamate ricorsive finchè non si trova l'utente */
	
		if ( strcmp( u, r->user->name ) < 0 ){
		
			return getUserStatus( r->left, u );
				
		}
			
		if (strcmp( u, r->user->name  ) > 0){
			
			return getUserStatus( r->right, u );
				
		}
		
		if (strcmp( u, r->user->name  ) == 0){
			return (r->status);
		}
	}
	
 	return NOTREG;/* se l'utente non c'è retval NOTREG */
	
}

/** Restituisce il canale di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare

 \retval ch canale su cui e' connesso l'utente se l'utente e' collegato
 \retval NOTREG se non e' presente
*/
int getUserChannel(nodo_t* r, char* u){
	
	if ( r != NULL ){ /* foglia o albero vuoto: analogo a sopra */
	
		if ( strcmp( u, r->user->name ) < 0 ){
		
			return getUserChannel( r->left, u );
				
		}
			
		if (strcmp( u, r->user->name  ) > 0){
			
			return getUserChannel( r->right, u );
				
		}
		
		if (strcmp( u, r->user->name  ) == 0){
			return (r->channel);
		}
	}
	
	return NOTREG;
}


/** Setta lo stato di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare
 \param s stato da settare 

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t setUserStatus(nodo_t* r, char* u, status_t s){

	if ( r != NULL ){ /* foglia o albero vuoto: analogo a sopra */
	
		if ( strcmp( u, r->user->name ) < 0 ){
		
			return setUserStatus( r->left, u, s );
				
		}
			
		if (strcmp( u, r->user->name  ) > 0){
			
			return setUserStatus( r->right, u, s );
				
		}
		
		if (strcmp( u, r->user->name  ) == 0){ /* trovato l'utente si cambia lo stato in s */
			r->status = s;
			return TRUE;
		}
	}
	
	return FALSE; /* l'utente non c'è */


}

/** Setta il canale di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare
 \param ch canale da settare 

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t setUserChannel(nodo_t* r, char* u, int ch){
	/* analoga alle precedenti */
	if ( r != NULL ){
	
		if ( strcmp( u, r->user->name ) < 0 ){
		
			return setUserChannel( r->left, u, ch );
				
		}
			
		if (strcmp( u, r->user->name  ) > 0){
			
			return setUserChannel( r->right, u, ch );
				
		}
		
		if (strcmp( u, r->user->name  ) == 0){
			r->channel = ch;
			return TRUE;
		}
	}
	
	return FALSE;

}

/** Controlla se un utente e' registrato nell'albero
 \param r radice dell'albero
 \param u  utente da cercare

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t isUser(nodo_t* r, char* u){
	/* analoga alle precedenti */
	if ( r != NULL ){
	
		if ( strcmp( u, r->user->name ) < 0 ){
		
			return isUser( r->left, u );
				
		}
			
		if (strcmp( u, r->user->name  ) > 0){
			
			return isUser( r->right, u );
				
		}
		
		if (strcmp( u, r->user->name  ) == 0){
			return TRUE;
		}
	}
	
	return FALSE;

}
	
	

/** Fornisce la lista degli utenti connessi sull'albero con stato == st
    \param r radice dell'albero
    \param st stato da ricercare

    \retval s la stringa con gli utenti secondo il formato 
              user1:user2:...:userN
    \retval NULL se non ci sono utenti nello stato richiesto
 */
 
char * getUserList(nodo_t* r, status_t st){

	char *s1=NULL, *s2=NULL, *s3=NULL, *s = NULL; /* stringhe per la chiamata attuale, per la chiamata a sinistra, a destra e stringa da restituire */

	int len = 0; /* conta la lunghezza a cui allocare s */

	if(r==NULL) return NULL; /* l'albero è vuoto */

	/* chiamate ricorsive */
	s2 = getUserList( r->left, st );
	s3 = getUserList( r->right, st );

	/*se lo stato dell'utente corrente è uguale a st lo si salva in s1 */
	if ( r->status == st ){

	s1 = malloc( sizeof( char ) * ( strlen(r->user->name) + 1 ) );

	if ( s1 != NULL ) strcpy(s1,r->user->name);

	}

	/* computo di len */
	if (s1 != NULL) len = len + strlen( s1 );
	if (s2 != NULL) len = len + strlen( s2 );
	if (s3 != NULL) len = len + strlen( s3 );
	
	/* le tre chiamate non restituiscono stringhe */

	if ( s1 == NULL && s2 == NULL && s3 == NULL) return NULL;
	
	/* le tre chiamate restituiscono ciascuna una stringa, si procede alla concatenazione in s e si liberano le stringhe ausiliarie, retval s */

	if ( s1 != NULL && s2 != NULL && s3 != NULL) {
		s = malloc( sizeof(char) * ( len + 3 ) );

		strcpy ( s, s2 ); 
		strcat ( s, ":");
		strcat ( s, s1 );
		strcat ( s, ":");
		strcat ( s, s3 );

		free(s1);
		free(s2);
		free(s3);

	return s;
	
	}
	
	/* due chiamate su tre restituiscono */
	
	if ( s1 != NULL && s2 == NULL && s3 != NULL) {
		s = malloc( sizeof(char) * ( len + 2 ) );
		strcpy ( s, s1 ); 
		strcat ( s, ":");
		strcat ( s, s3 );

		free(s1);
		free(s3);

		return s;
		}

	if ( s1 == NULL && s2 != NULL && s3 != NULL) {
		s = malloc( sizeof(char) * ( len + 2 ) );
		strcpy ( s, s2 ); 
		strcat ( s, ":");
		strcat ( s, s3 );

		free(s2);
		free(s3);

		return s;
	}

	

	if ( s1 != NULL && s2 != NULL && s3 == NULL) {
	
		s = malloc( sizeof(char) * ( len + 2 ) );

		strcpy ( s, s1 ); 
		strcat ( s, ":");
		strcat ( s, s2 );

		free(s1);
		free(s2);

		return s;
	}
	
	/* una chiamata su tre restituisce */

	if ( s1 == NULL && s2 == NULL && s3 != NULL) return s3;
	

	if ( s1 == NULL && s2 != NULL && s3 == NULL) return s2;
	

	if ( s1 != NULL && s2 == NULL && s3 == NULL) return s1;
	
	return NULL;
	

}


