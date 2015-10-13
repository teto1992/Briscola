/**
   \file
   \author lso13

   \brief struttura ad albero relativa agli utenti connessi
*/ 

#ifndef __USERS__H
#define __USERS__H

#include "bris.h"

/** Lunghezza massima username */
#define LUSER 20
/** Lunghezza massima password */
#define LPWD  8
/** errore utente inesistente */
#define NOUSR -2
/** errore password errata */
#define WRPWD -3


/** tipo connessione...
   DISCONNECTED disconnesso
   WAITING in attesa di sfida
   PLAYING impegnato in una partita
*/
typedef enum connection { DISCONNECTED, WAITING, PLAYING } status_t;

/** Dati utente **/
typedef struct user {
  /** username */
  char name[LUSER + 1]; 
  /** password */
  char passwd[LPWD + 1];
} user_t;

/** Nodo dell'albero che rappresenta l'utente registrato al servizio,
    ogni utente puo' essere connesso o disconnesso e se connesso puo'
    essere in attesa di una sfida o coinvolto in una partita -- 
    l'albero e' un albero di ricerca ordinato lessicograficamente (strcmp()) 
    rispetto al campo user */
typedef struct nodo {
  /** dati utente */
  user_t* user;
  /** connesso ? in partita ?*/
  status_t status;  
  /** canale di comunicazione, significativo solo se connesso altrimenti (-1)*/
  int channel;
  /** figlio destro */  
  struct nodo* left;       
  /** figlio sinistro */ 
  struct nodo* right;      
} nodo_t;

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
user_t* stringToUser(char* r, unsigned int l);

/** A partire da una struttura utente viene creata una stringa

     nome_user:password

     la funzione alloca la memoria necessaria e termina correttamente
     la stringa risultato con il terminatore di stringa

     \param puser struttura da convertire


     \retval p (puntatore alla nuova stringa) 
     \retval NULL se si e' verificato un errore (setta errno)
 */
char* userToString(user_t* puser);


/** Stampa l'albero su stdout
    \param r radice dell'albero da stampare
 */
void printTree(nodo_t* r);

/** Aggiunge un nuovo utente all'albero mantenendolo ordinato lessicograficamente rispetto al campo users (ordinamento di strcmp). Se l'utente e' gia' presente non viene inserito.

 \param r puntatore alla radice dell'albero
 \param puser puntatore utente da inserire

 \retval 0 se l'inserzione e' andata a buon fine
 \retval 1 se l'utente e' gia' presente
 \retval -1 se si e' verificato un errore (setta errno)
 */
int addUser(nodo_t** r, user_t* puser);


/** controlla la password di un utente
 \param r radice dell'albero
 \param user  utente di cui controllare la passwd

 \retval TRUE  se l'utente e' presente e la password corrisponde
 \retval FALSE se non e' presente o la password e' errata
 */ 
bool_t checkPwd(nodo_t* r, user_t* user);

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
int removeUser(nodo_t** r, user_t* puser);



/** Dealloca l'albero 

 \param r radice dell'albero da deallocare
*/
void freeTree(nodo_t* r);

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
int loadUsers(FILE* fin, nodo_t** r);

/** Scrive tutti i permessi nell'albero su file in ordine
    lessicografico. Ogni utente e' scritto nel formato nome_utente:password
 separati da '\n'
 
 \param fout il file su cui scrivere 
 \param r il puntatore alla radice dell'albero
 
 \retval n il numero di utenti registrati nel file
 \retval -1 se si e' verificato un errore   (setta errno)
 */
int storeUsers(FILE* fout, nodo_t* r);



/** utente non registrato */
#define NOTREG (-10)

/** Restituisce lo stato di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare

 \retval s stato dell'utente se l'utente e' presente
 \retval NOTREG se non e' presente
*/
status_t getUserStatus(nodo_t* r, char* u);

/** Restituisce il canale di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare

 \retval ch canale su cui e' connesso l'utente se l'utente e' collegato
 \retval NOTREG se non e' presente
*/
int getUserChannel(nodo_t* r, char* u);

/** Setta lo stato di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare
 \param s stato da settare 

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t setUserStatus(nodo_t* r, char* u, status_t s);

/** Setta il canale di un utente (se esiste)
 \param r radice dell'albero
 \param u  utente da cercare
 \param ch canale da settare 

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t setUserChannel(nodo_t* r, char* u, int ch);

/** Controlla se un utente e' registrato nell'albero
 \param r radice dell'albero
 \param u  utente da cercare

 \retval TRUE se l'utente e' presente
 \retval FALSE se non e' presente
*/
bool_t isUser(nodo_t* r, char* u);

/** Fornisce la lista degli utenti connessi sull'albero con stato == st
    \param r radice dell'albero
    \param st stato da ricercare

    \retval s la stringa con gli utenti secondo il formato 
              user1:user2:...:userN
    \retval NULL se non ci sono utenti nello stato richiesto
 */
char *  getUserList(nodo_t* r, status_t st);
#endif
