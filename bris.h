/** \file bris.h
       \author Stefano Forti
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

/**
   \file
   \author lso13
   \brief tipi carte progetto bris
 */

#ifndef __BRIS__H
#define __BRIS__H

#include <stdio.h>

/** tipo booleano ...*/
typedef enum bool { FALSE, TRUE } bool_t;

/** i valori delle carte */
typedef enum valori{ASSO,DUE,TRE,QUATTRO,CINQUE,SEI,SETTE,FANTE,DONNA,RE} valori_t;

/** i semi delle carte */
typedef enum semi{CUORI,QUADRI,FIORI,PICCHE } semi_t;

/** una carta e' data da un valore ed un seme */
typedef struct carta {
  /** valore */
  valori_t val; 
  /** seme */
  semi_t seme;  
} carta_t;

/** numero di carte in un mazzo da briscola */
#define NCARTE 40
/** un mazzo e' un insieme di NCARTE carte in un certo ordine*/
typedef struct mazzo {
  /** carte mischiate */
  carta_t carte [NCARTE]; 
  /** indice della prossima carta da pescare */
  unsigned int next ;     
  /** briscola (seme ultima carta del mazzo)*/
  semi_t briscola;        
} mazzo_t;

/** genera un mazzo di carte mischiato
    richiede che la srand() sia stata gia' invocata con un seed adeguato

    ATTENZIONE: questa funzione e' fornita gia' implementata dai docenti 
    nel modulo oggetto newMazzo.o da inserire nella libreria libbris.a

    \retval p puntatore al nuovo mazzo 
    \retval NULL se si e' verificato un errore (setta errno)
*/
mazzo_t * newMazzo(void);



/** stampa una carta nel formato a due caratteri (AQ 2F etc ...) 
    \param fpd stream di uscita
    \param c  puntatore carta da stampare
*/
void printCard(FILE* fpd,carta_t *c);


/** stampa una carta nel formato a due caratteri (AQ 2F etc ...) su stringa
    \param s stringa di uscita (deve essere lunga almeno 3 caratteri)
    \param c  puntatore carta da stampare
*/
void cardToString(char* s, carta_t *c);

/** converte una carta dal formato stringa 2 caratteri a struttura
    \param str stringa di ingresso (deve essere lunga almeno 2, converte i primi 2 caratteri)
    \retval c  puntatore carta creata (alloca memoria)
    \retval NULL  se si e' verificato un errore (setta errno)
*/
carta_t* stringToCard(char* str) ;

/** pesca la prossima carta dal mazzo (aggiustando il campo next)
    \param m mazzo

    \retval NULL se tutte le carte del mazzo sono gia' state pescate (errno = 0)
    \retval NULL se si e' verificato un errore (errno != 0)
    \retval pc puntatore alla prossima carta (questa e' una copia della carta, allocata all'interno della funzione)
*/
carta_t* getCard(mazzo_t* m);

/** stampa un mazzo di carte 
    \param fpd stream di uscita
    \param pm  puntatore al mazzo da stampare
*/
void printMazzo(FILE* fpd, mazzo_t *pm);

/** dealloca mazzo di carte 
    \param pm  puntatore al mazzo
*/
void freeMazzo(mazzo_t *pm);
  


/** confronta due carte data la briscola
   \param briscola il seme di briscola
   \param ca, cb carte da confrontare (ca giocata prima di cb)
   \retval TRUE se ca batte cb
   \retval FALSE altrimenti
 */
bool_t compareCard(semi_t briscola, carta_t* ca, carta_t* cb);

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
int computePoints(carta_t**c, int n) ;
#endif
