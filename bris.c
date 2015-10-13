/** \file bris.c
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<errno.h>
#include"bris.h"

/** stampa una carta nel formato a due caratteri (AQ 2F etc ...) 
    \param fpd stream di uscita
    \param c  puntatore carta da stampare
*/
void printCard(FILE* fpd,carta_t *c){

	char valCarta[] = { 'A', '2', '3', '4', '5', '6', '7', 'J', 'Q', 'K'}; /* array dei valori da indirizzare con il c->val */
	char semeCarta[] = {'C','Q','F', 'P'}; /* array dei semi da indirizzare con il c->seme */
	
	if ( c != NULL ){ /* il puntatore c è non nullo? */
	
	if ( fprintf(fpd,"%c%c\n", valCarta[ c->val ], semeCarta[ c->seme ] ) < 0) printf("Errore in fprintf!"); /*stampa su file e controlla che fprintf sia arrivata a buon fine*/
	
	}
	else return; /* se il puntatore c è nullo si esce */
}

/** stampa una carta nel formato a due caratteri (AQ 2F etc ...) su stringa
    \param s stringa di uscita (deve essere lunga almeno 3 caratteri)
    \param c  puntatore carta da stampare
*/
void cardToString(char* s, carta_t *c){

	char valCarta[] = { 'A', '2', '3', '4', '5', '6', '7', 'J', 'Q', 'K'};/* array dei valori da indirizzare con il c->val */
	char semeCarta[] = {'C','Q','F', 'P'}; /* array dei semi da indirizzare con il c->seme */
	
	if ( s != NULL && (sizeof(s) / sizeof(char)) >= 3){ /* la stringa è diversa da NULL e lunga almeno 3? */
	
			s[0] = valCarta[c->val]; /*si inserisce nella stringa il valore*/
			s[1] = semeCarta[c->seme]; /*si inserisce nella stringa il seme*/
			s[2] = '\0'; /*si chiude la stringa*/
	
	}
	
	else return; /* se la stringa non è NULL e non ha almeno tre caratteri si esce */
}

/** converte una carta dal formato stringa in 2 caratteri a struttura
    \param str stringa di ingresso (deve essere lunga almeno 2, converte i primi 2 caratteri)
    \retval c  puntatore carta creata (alloca memoria)
    \retval NULL  se si e' verificato un errore (setta errno)
*/
carta_t* stringToCard(char* str) {

	carta_t *tmp = malloc ( sizeof( carta_t ) ); /* si alloca un puntatore a carta tmp */
	char valCarta[] = { 'A', '2', '3', '4', '5', '6', '7', 'J', 'Q', 'K'};/* array dei valori da indirizzare con il c->val */
	char semeCarta[] = {'C','Q','F', 'P'}; /* array dei semi da indirizzare con il c->seme */
	int i = 0; /* contatore per indirizzare gli array */
	
	if ( tmp != NULL && str != NULL ){ /* tmp è diverso da NULL? str è diverso da NULL?*/
	
	while ( valCarta[i] != '\0' && valCarta[i] != str[0] ) i++; /* finchè non si arriva alla cella del vettore valCarta che contiene il valore in str[0] si itera, se si arriva in fondo s[0] non è un valore ammissibile*/
	
	if ( valCarta[i] != '\0' ) tmp->val = i; /* se non siamo arrivati alla fine dell'array valCarta si inserisce il valore in tmp*/
	
	else { 				/*altrimenti si setta errno e si libera tmp restituendo NULL*/
		errno = EINVAL;
		free ( tmp );
		return NULL;
	}
	
	i = 0; /* si rimette i a zero */
	
	while ( semeCarta[i] != '\0' && semeCarta[i] != str[1] ) i++; /* finchè non si arriva alla cella del vettore semeCarta che contiene il valore in str[1] si itera, se si arriva in fondo s[1] non è un valore ammissibile*/
	
	if ( valCarta[i] != '\0' ) tmp->seme = i; /* se non siamo arrivati alla fine dell'array semeCarta si inserisce il valore in tmp*/
	
	else {					/*altrimenti si setta errno e si libera tmp restituendo NULL*/
		errno = EINVAL;
		free ( tmp );
		return NULL;
	}
	
	}
	
	return tmp; /*si restituisce tmp, la carta completa*/
}
	

/** pesca la prossima carta dal mazzo (aggiustando il campo next)
    \param m mazzo

    \retval NULL se tutte le carte del mazzo sono gia' state pescate (errno = 0)
    \retval NULL se si e' verificato un errore (errno != 0)
    \retval pc puntatore alla prossima carta (questa e' una copia della carta, allocata all'interno della funzione)
*/

carta_t* getCard(mazzo_t* m){

	carta_t *pc = malloc( sizeof( carta_t ) ); /* si alloca pc */
	
	if ( pc == NULL ) return NULL; /* malloc setta automaticamente errno */
	
	if ( m != NULL ){ /* l'allocazione è andata a buon fine? il mazzo è diverso da NULL? */
		
		if ( m->next < NCARTE ){ /* se non è finito il mazzo si pesca la carta */
		
			pc->val = (m->carte[m->next]).val; 	/* si copia il valore della prima carta del mazzo in pc */
			pc->seme = (m->carte[m->next]).seme; /* si copia il seme della prima carta del mazzo in pc */
			m->next = (m->next) + 1; /* si incrementa il campo next del mazzo*/

			return pc; /*si restituisce pc*/
		
		}
		
		else{ /* se il mazzo è finito */
		
			errno = 0; /* errno a zero*/
			free( pc ); /* si libera pc*/
			return NULL; /* retval NULL */
		}
	}
		
	else{ /* se il mazzo è NULL */
	
		errno = EINVAL; /* errore negli argomenti */
		free( pc); /* si libera pc*/
		return NULL; /*retval NULL*/
	}
}


/** stampa un mazzo di carte 
    \param fpd stream di uscita
    \param pm  puntatore al mazzo da stampare
*/
void printMazzo(FILE* fpd, mazzo_t *pm){ 

	int i; /* indice per scorrere l'array delle carte */
	
	for ( i=0; i<NCARTE; i++ )
		printCard( fpd, &(pm->carte[i])); /* si richiama printCard su ogni carta da stampare*/
	
	
}
	

/** dealloca mazzo di carte 
    \param pm  puntatore al mazzo
*/
void freeMazzo(mazzo_t *pm){
	
	if ( pm != NULL ){ /* l'unica struttura dinamica allocata è il mazzo, lo si libera */
		free( pm );
	}
}  


/** confronta due carte data la briscola
   \param briscola il seme di briscola
   \param ca, cb carte da confrontare (ca giocata prima di cb)
   \retval TRUE se ca batte cb
   \retval FALSE altrimenti
 */
bool_t compareCard(semi_t briscola, carta_t* ca, carta_t* cb){

	int punti[] = { 11, 0, 10, 0, 0, 0, 0, 2, 3, 4 }; /* array dei punteggi di ogni carta, da indirizzare col valore della carta*/
		
	if ( ca->seme == cb->seme ) { /*se le carte hanno lo stesso seme, vince il punteggio più alto*/
		if ( punti[ca->val] > punti[cb->val] ) return TRUE; /* se ca ha il punteggio più alto vince: TRUE*/
		else return FALSE;/*altrimenti vince cb*/
	}
	
	else{
		if ( cb->seme != briscola ) return TRUE; /*altrimenti se cb non gioca la briscola ca comanda e vince: TRUE*/
		else return FALSE;/*nel caso contrario vince cb:FALSE*/
	}
}

/** Calcola l'ammontare complessivo dei punti secondo le regole della briscola
    ASSO 11
    TRE  10
    RE    4
    DONNA 3
    FANTE 2
    DUE QUATTRO CINQUE SEI SETTE 0

    \param c array carte da valutare
    \param n lunghezza array

    \retval -1 se si e' verificato un errore (setta errno)
    \retval np numero complessivo di punti
 */
int computePoints(carta_t**c, int n){

	int punti[] = { 11, 0, 10, 0, 0, 0, 0, 2, 3, 4 }; /* vettore dei punteggi */
	int np = 0; /* punteggio complessivo a 0 */
	int i; /* indice per scorrere il mazzetto dei punti */
	
	if ( c!= NULL && n >= 0){ /* se il mazzetto dei punti non è NULL e il numero di carte è positivo */
	
		for ( i = 0; i < n; i++ ){ /* si scorre il mazzetto*/
		
			np = np + punti[c[i]->val]; /* si sommano via via i punti indirizzando l'array punti coi valori delle carte*/
			
		}
	
		return np; /*si restituisce np*/
	}
			
	else { /* altrimenti errore negli argomenti, retval -1*/
		errno = EINVAL;
		return -1;
	}

}
				


