[Laboratorio uBash]

Componenti del gruppo:
Dellepiane Emanuele - 4876072
Manini Filippo - 4798004
Miggiano Davide - 4840761

uBash processa i comandi, leggendoli da standard input. 
A differenza delle shell comunemente usate, non vengono gestiti:
- L’espansione dei nomi di file
- Le sequenze di escape
- Le stringhe
- I gruppi di processi
- Processi in foreground/background
- ...

La nostra proposta di implementazione è così articolata:
Il comando, dopo essere stato letto da standard input, dovrà superare un controllo di validità rispettando diverse regole di correttezza da noi imposte 
(ad esempio se la stringa letta è uno spazio o un'andata a capo, ...).

In caso di corretto superamento del controllo, l'input, viene parsato tramite la funzione parseString.
Per rappresentare in memoria l'input, ovvero una successione di comandi, il gruppo ha optato
per l'uso di una linked list poichè, dall'analisi fatta, è venuto fuori quanto segue:
Se prendiamo, per esempio, il comando "ls -la | grep ubash | sort -r" possiamo notare come esso sia formato da diverse stringhe "semplici" dove
la prima stringa, o la prima stringa dopo una pipe, rappresenta il "comando" vero e proprio, mentre quelle che seguono sono gli eventuali paramentri.
La lista, quindi, dovrà essere formata da:
- Un puntatore a stringa, che contiene il "payload" (esempio "ls" , "-la", "cd", "grep", ">seti.txt"
- Un campo speciale, che indica "è un comando" o "è un argomento": in questo caso un bool che, se settato a true, indica che il payload della lista è un comando (viceversa è un argomento)

struct node
{
    char *payload;
    bool isCommand;
    struct node *next;
};
typedef struct node *command;

La funzione parseString si occuperà di individuare gli spazi / eventuali pipe e splittarli creando N nodi della lista, inizializzando correttamente i suoi campi usando come appoggio diverse funzioni
che operano sulla linked list (come creazione di un nodo, eliminazione, aggiunta di un nodo in coda, deallocazione della lista, ...)

Il comando parsato, un volta uscito dalla parseString, dovrà sottoporsi a un altro controllo di validità, questa voltà più rigido rispetto al precedente, dove verrano verificate tutte le regole di
buona formattazione (no doppie pipe con comandi vuoti, no cd in pipe con altri comandi, ...)

Se il comando passerà anche questo controllo significa che è pronto per essere eseguito, verrà quindi passato alla funzione executeCommand che riconoscerà il "tipo" di comando (comando singolo, pipe, redirezione singola, ...)
e lo eseguirà. In tutto il codice vengono "sollevate" perror o fprintf su STDERR in caso di qualsiasi tipo di malfunzionamento, grazie alla gestione degli errori a "cascata" sarà più facile individuare l'eventuale
errore da fixare seguendo il percorso fatto.

Tutto le funzioni sopra descritte vengono gestite dal main(), che le eseguirà dentro un ciclo infinito (nel nostro caso for(;;)) e potrà uscire da esso se e solo se l'utente digiterà la stringa (o comando "built in") exit (come avviene
nelle normali bash). Una volta uscito dal main, prima di terminare, vengono effettuate tutte le deallocazioni del caso evitando così di lasciare "appese" delle variabili in memoria.

