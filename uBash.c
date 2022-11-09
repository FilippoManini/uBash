/* 
    Laboratorio uBash, componenti del gruppo:
    
    Dellepiane Emanuele - 4876072
    Manini Filippo - 4798004
    Miggiano Davide - 4840761
*/

#include <errno.h>      // Per controllare il ritorno della perror
#include <stdio.h>      // Libreria standard per I/O
#include <fcntl.h>      // Opzioni di controllo dei file
#include <stdint.h>     // Per la stampa dei pid portabile (%jd)
#include <string.h>     // Libreria standard per la gestione stringhe
#include <unistd.h>     // Interfaccia con il sistema Unix - https://digilander.libero.it/uzappi/C/librerie/C-unistd.html
#include <stdlib.h>     // Per EXIT_FAILURE
#include <stdbool.h>    // Per il tipo bool
#include <sys/wait.h>   // wait(&s);
#include <sys/types.h>  // pid_t getpid(void) e pid_t getppid(void)

const int BYTES = 1024;
const int COMMAND_NOT_FOUND = 2020;
char* const PIPE_ERROR = "ERROR_ON_PIPE";

struct node
{
    char *payload;
    bool isCommand;
    struct node *next;
};
typedef struct node *command;
//╔══════════════════════════════════════════════════════════════════════╗
//║                           CHECK FUNCTIONS                            ║
//╚══════════════════════════════════════════════════════════════════════╝
void checkMalloc(void* address)
{
    if(address == NULL)
    {
        perror("malloc: ");
        exit(EXIT_FAILURE);
    }
}

void checkWait(pid_t pid)
{
    int s;

    // printf("\tParent process: PID=%jd (PPID=%jd), child's PID=%jd\n", (intmax_t)getpid(), (intmax_t)getppid(), (intmax_t)pid);
    if(wait(&s) == -1)
    {
        perror("wait: ");
        exit(EXIT_FAILURE);
    }
    else if(WIFEXITED(s))
    {
        if(WEXITSTATUS(s) != 0)
        {
            printf("\tChild (PID=%jd) terminated with exit status: %d\n", (intmax_t)pid, WEXITSTATUS(s));  
        }    
    }
    else if(WIFSIGNALED(s))
    {
        int signo = WTERMSIG(s);
        printf("\tChild terminated by signal %d (%s)\n", signo, strsignal(signo));       
    }
    else
    {
        printf("\tI have no idea about what's going on... :-O\n");       
    }
}
//╔══════════════════════════════════════════════════════════════════════╗
//║                            LIST FUNCTIONS                            ║
//╚══════════════════════════════════════════════════════════════════════╝
command createCommand(int payloadSize)
{
    command temp = (command)malloc(sizeof(struct node));
    checkMalloc(temp);

    // Non serve allocare memoria per temp->payload poichè già allocata con la malloc della struct
    temp->isCommand = false;
    temp->next = NULL;

    return temp;
}

void addCommand(command head, command newNode)
{
    command temp = head;

    if(temp->next == NULL)
    {
        temp->next = newNode;
    }
    else
    {
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = newNode;
    }
}

void deleteCommand(command head)
{
    command temp = head;
    command oldCommand = head->next;

    if(oldCommand->next == NULL)
    {
        temp->next = NULL;
    }
    else
    {
        temp->next = temp->next->next;
    }
    free(oldCommand);
}

command getNextCommand(command input)
{
    command temp = input->next;

    while(temp != NULL) 
    {
        if(temp->isCommand)
        {
            return temp;       
        }
        temp = temp->next;
    }
    return NULL;
}

int nArgument(command input)
{
    command temp = input->next;
    int argumentNumber = 1;

    while(temp != NULL) 
    {
        if(temp->isCommand)
        {
            return argumentNumber;
        }
        argumentNumber++;
        temp = temp->next;
    }
    // Ritorna il numero di argomenti di un comando contando il comando stesso 
    return argumentNumber;
}

int nCommand(command input)
{
    command temp = input;
    int commandNumber = 0;

    while(temp != NULL)
    {
        commandNumber++;
        temp = getNextCommand(temp);
    }
    // Ritorna il numero dei comandi divisi da pipe (|)
    return commandNumber;
}

void freeCommandList(command head)
{
    if(head->next == NULL)
    {
        free(head);
    }
    else
    {
        command currNode = head->next, nextNode = NULL;
        while(currNode != NULL)
        {
            nextNode = currNode->next;
            free(currNode);
            currNode = nextNode;
        }
        free(head);
    }
}

//╔══════════════════════════════════════════════════════════════════════╗
//║                           UTILITY FUNCTIONS                          ║
//╚══════════════════════════════════════════════════════════════════════╝
char *removeFirstCharacter(char *string)
{
    return string+1;
}

char* getFilename(command input, char specialCharacter)
{
    command testa = input;
    char* nameFile = NULL;

    while(testa != NULL && testa->next != NULL)
    {
        if(testa->next->payload[0] == specialCharacter)
        {
            nameFile = removeFirstCharacter(testa->next->payload);
            deleteCommand(testa);   // Toglie dalla lista degli argomenti la ridirezione
            return nameFile;
        }
        testa = testa->next;
    }
    return nameFile;
}

int nSpecialCharacter(command input, char specialCharacter)
{
    command testa = input;
    int totalOccurrences = 0;

    while(testa != NULL) 
    {
        if(testa->payload[0] == specialCharacter)
        {
            totalOccurrences++;
        }
        testa = testa->next;
    }
    // Ritorna il numero di occorrenze di un carattere speciale dato in input
    return totalOccurrences;
}
//╔══════════════════════════════════════════════════════════════════════╗
//║                           BASH FUNCTIONS                             ║
//╚══════════════════════════════════════════════════════════════════════╝
void printCurrentDirectory()
{
    size_t bufferSize = sizeof(char) * BYTES;
    char *buffer = (char*)malloc(bufferSize);
    checkMalloc(buffer);

    if((getcwd(buffer, bufferSize)) != NULL)
    {
        printf("%s$ ", buffer);
    }
    else
    {
        perror("getcwd: ");
        free(buffer);
        exit(EXIT_FAILURE);
    }
    free(buffer);
}

char* readCommand()
{
    size_t bufferSize = sizeof(char) * BYTES;
    char *input = (char*)malloc(bufferSize);
    checkMalloc(input);

    if(fgets(input, bufferSize, stdin) == NULL)
    {
        perror("fgets: ");
        free(input);
        return NULL;   
    }
    else
    {
        strtok(input, "\n");
        return input;
    }
}

int expandAmbientVariable(command input)
{
    command testa = input;
    char *stringAux = (char*)malloc(sizeof(char) * BYTES);
    checkMalloc(stringAux);

    while(testa != NULL) 
    {
        if(testa->payload[0] == '$')
        {
            testa->payload = removeFirstCharacter(testa->payload);
            stringAux = getenv(testa->payload);
            if(stringAux == NULL)
            {
                fprintf(stderr, "expandAmbientVariable(): $%s doesn't exist\n", testa->payload);
                return EXIT_FAILURE;
            }
            else
            {
                strcpy(testa->payload, stringAux);
            }
        }
        testa = testa->next;
    }
    return EXIT_SUCCESS;
}

bool checkRead(char *input)
{
    // Per controllare spazi casuali e pipe casuali (es: |   ||| |  |)
    int pipeCounter = 0, spaceCounter = 0, i;
    int anotherCharacter = 0;
    int size = strlen(input);

    if(size == 0 || input == NULL || input[0] == '-')
    {
        fprintf(stderr, "checkRead(): cattiva formattazione input\n");
        return false;
    }

    for(i = 0; i < size; ++i)
    {
        switch(input[i])
        {
            case ' ':
                spaceCounter++; 
                break;
            case '|':
                if(strcmp( input+(size-2),"| ") == 0 )
                {
                    fprintf(stderr, "checkRead(): nessun comando e spazio dopo pipe\n");
                    return false;
                }
                pipeCounter++;
                break;
            case '\n':
                return false;   // L'utente schiaccia "Invio" senza nessun comando
                break;
            default:
                anotherCharacter++;
                break; 
        }
    }

    if(input[i-1] == '|')
    {
        return false;   // Pipe come ultimo carattere
    }
    if(spaceCounter == i)
    {
        return false;   // Inseriti solo spazi
    }
    if(pipeCounter + spaceCounter == i)
    {
        return false;   // Inserite solo pipe
    }
    return true;
}

bool checkCommand(command cmd)
{
    command aux = cmd;
    int size = nCommand(cmd);
    int totalArgument = 0, loopCounter = 0;

    if(nSpecialCharacter(cmd, '$') > 0)
    {
        if(expandAmbientVariable(cmd) != EXIT_SUCCESS)
        {
            fprintf(stderr, "checkCommand(): error at expandAmbientVariable()\n");
            return false;
        }
    }

    if(nSpecialCharacter(cmd, '<') > 1)
    {
        fprintf(stderr, "errore: ridirezione input superiore a 1\n");
        return false;
    }
    else if(nSpecialCharacter(cmd, '>') > 1)
    {
        fprintf(stderr, "errore: ridirezione output superiore a 1\n");
        return false;
    }

    while(aux != NULL) 
    {
        if(strcmp(aux->payload, PIPE_ERROR) == 0)
        {
            fprintf(stderr, "errore: comando \"vuoto\" (doppia | senza niente in mezzo)\n");
            return false;
        }

        if(strcmp(aux->payload, "<") == 0)
        {
            fprintf(stderr, "errore: non è specificato il file per la ridirezione dello standard input\n");
            return false;
        }

        if(strcmp(aux->payload, ">") == 0)
        {
            fprintf(stderr, "errore: non è specificato il file per la ridirezione dello standard output\n");
            return false;
        }

        if(strcmp(aux->payload, "|") == 0)
        {
            fprintf(stderr, "errore: comando \"vuoto\" (doppia | senza niente in mezzo)\n");
            return false;
        }
        
        if(aux->payload[0] == '<' && loopCounter > 1)
        {
            fprintf(stderr, "errore: solo il primo comando può avere redirezione dell'input\n");
            return false; 
        }

        if(aux->payload[0] == '>' && loopCounter != size)
        {
            fprintf(stderr, "errore: solo l'ultimo comando può avere redirezione dell'output\n");
            return false; 
        }

        if(aux->isCommand)
        {
            loopCounter++;
            totalArgument += nArgument(aux);
        }
        aux = aux->next;
    }

    if((nSpecialCharacter(cmd, '>') + nSpecialCharacter(cmd, '<')) == totalArgument)
    {
        fprintf(stderr,"errore: sono presenti solo redirezioni\n");
        return false;
    }

    aux = cmd;
    if(size == 0)
    {
        fprintf(stderr, "errore: cattiva formattazione lista\n");
        return false;
    }
    else if(size == 1)
    {
        if(strcmp(aux->payload, "cd") == 0)
        {
            if((nSpecialCharacter(aux, '>') > 0) || (nSpecialCharacter(aux, '<') > 0))
            {
                fprintf(stderr, "errore: il comando cd non supporta la redirezione\n");
                return false;   
            }

            if(aux->next == NULL)
            {
                fprintf(stderr, "cd(): manca percorso\n");
                return false;
            }

            if(aux->next->next != NULL)
            {
                fprintf(stderr, "errore: il comando cd ha solo un argomento\n");
                return false;
            }
        }
    }
    else
    {
        while(aux != NULL) 
        {
            if(strcmp(aux->payload, "cd") == 0)
            {
                fprintf(stderr, "errore: il comando cd deve essere usato da solo\n");
                return false;
            }
            aux = getNextCommand(aux);
        }
    }
    return true;
}

command parseString(char *input)
{
    char* context = NULL;
    char* token = strtok_r(input, " ", &context);

    command testa = createCommand(strlen(token));
    testa->payload = token;
    testa->isCommand = true;
    
    token = strtok_r(NULL, " ", &context); // Questo mi permette di non passare NULL a createCommand

    while(token != NULL)
    {
        if(strcmp(token, "|") == 0)
        {
            token = strtok_r(NULL, " ", &context);          
            command tempNode = createCommand(strlen(token));
            tempNode->payload = token;
            tempNode->isCommand = true;
            addCommand(testa, tempNode);

            token = strtok_r(NULL, " ", &context);
        }
        else
        {
            command tempNode = createCommand(strlen(token));
            if(strcmp(token, "||") == 0)
            {
                tempNode->payload = PIPE_ERROR;
            }
            else
            {
                tempNode->payload = token;
            }
            addCommand(testa, tempNode);
            token = strtok_r(NULL, " ", &context);
        }
    }
    return testa;
}

int cd(command input)
{
    command pathname = input->next;

    if(chdir(pathname->payload) == -1)
    {
		fprintf(stderr, "uBash: cd: %s: File o directory non esistente\n", pathname->payload);
        return EXIT_FAILURE;
	}
    return EXIT_SUCCESS;
}

char **convertList(command input)
{
    command temp = input;
    int i, size = nArgument(temp);

    char **m = (char**)malloc((size + 1) * sizeof(char *)); // (size + 1) per il terminatore NULL
    checkMalloc(m);

    for(i = 0; i < size; ++i)
    {
        if(temp == NULL)
        {
            fprintf(stderr,"convertList(): grandezza lista superata\n");
            return NULL;
        }
        *(m+i) = temp->payload;
        temp = temp->next;
    }
    // Non serve aumentare i perchè esce quando difatti supera il valore prestabilito, quindi è già incrementata
    *(m+i) = NULL;  // Aggiungo il terminatore NULL che mi serve alla execvp
    return m;
}

int startCommand(char **command)
{
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork: ");
        return EXIT_FAILURE;
    }
    else if(pid == 0) 
    {
        // printf("\tchild process: PID=%d (PPID=%d)\n", getpid(), getppid());
        int status_code = execvp(command[0], command);

        if(status_code == -1)
        {
            if(strcmp(strerror(errno), "No such file or directory") == 0)
            {
                // Così so quando viene digitato un comando inesistente
                return COMMAND_NOT_FOUND;
            }
            else
            {
                perror("execvp: ");
                return EXIT_FAILURE;
            }
        }
    }
    else 
    {
        checkWait(pid);
    }
    return EXIT_SUCCESS;
}

int executeRedirection_aux(char* nameFile, int redirectType)
{
    int fd;

    if(redirectType == STDIN_FILENO)
    {
        fd = open(nameFile, O_RDONLY);
    }
    else if(redirectType == STDOUT_FILENO)
    {
        fd = open(nameFile, O_RDWR|O_CREAT|O_TRUNC, 444);
    }
    else
    {
        fprintf(stderr, "executeRedirection_aux(): wrong redirectType");
        return EXIT_FAILURE;
    }

    if(fd < 0)
    {
        fprintf(stderr,"executeRedirection_aux(): an error occurred while opening the file %s\n", nameFile);
        close(fd);
        exit(EXIT_FAILURE);
    }

    if(dup2(fd, redirectType) == -1)
    {
        perror("dup2: ");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
    return EXIT_SUCCESS;
}

int executeRedirection(command input, int redirectType)
{
    char* nameFile = NULL;

    if(redirectType == STDIN_FILENO)
    {
        nameFile = getFilename(input, '<');
    }
    else if(redirectType == STDOUT_FILENO)
    {
        nameFile = getFilename(input, '>');
    }
    
    if(nameFile == NULL)
    {
        fprintf(stderr,"executeRedirection(): error at getFilename()\n");
        return EXIT_FAILURE;
    }

    char **command = convertList(input);
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork: ");
        return EXIT_FAILURE;
    }
    else if(pid == 0)
    {
        // printf("\tchild process: PID=%d (PPID=%d)\n", getpid(), getppid() );
        if(executeRedirection_aux(nameFile, redirectType) != EXIT_SUCCESS)
        {
            fprintf(stderr, "executeRedirection(): error at executeRedirection_aux()\n");
            return EXIT_FAILURE;
        }
        int status_code = execvp(command[0], command);
        if(status_code == -1)
        {
            perror("execvp: ");
            return EXIT_FAILURE;
        }
    }
    else
    {
        checkWait(pid);
        free(command);
    }
    return EXIT_SUCCESS;
}

int executeDoubleRedirection(command input)
{
    char *nameFileInput = getFilename(input, '<');
    char *nameFileOutput = getFilename(input, '>');

    if(nameFileInput == NULL && nameFileOutput == NULL)
    {
        fprintf(stderr, "executeDoubleRedirection(): error at getFilename()\n");
        return EXIT_FAILURE;
    }

    char **command = convertList(input);
    pid_t pid = fork();
    if(pid < 0)
    {
        perror("fork: ");
        return EXIT_FAILURE;
    }
    else if(pid == 0)
    {
        // printf("\tchild process: PID=%d (PPID=%d)\n", getpid(), getppid() );
        executeRedirection_aux(nameFileInput, STDIN_FILENO);
        executeRedirection_aux(nameFileOutput, STDOUT_FILENO);

        int status_code = execvp(command[0], command);
        if(status_code == -1)
        {
            perror("execvp: ");
            return EXIT_FAILURE;
        }
    }
    else
    {
        checkWait(pid);
        free(command);
    }
    return EXIT_SUCCESS;
}

int executePipe(command input)
{
    int totalChilds = nCommand(input);
    command cmd = input;

    if(totalChilds < 2)
    {
        fprintf(stderr, "executePipe(): too few arguments\n");
        return EXIT_FAILURE;
    }

    pid_t *pid = (pid_t*)malloc(sizeof(pid_t) * totalChilds);
    checkMalloc(pid);

    int **filedes = (int**)malloc(totalChilds*sizeof(int*));
    checkMalloc(filedes);

    for(int i = 0; i < totalChilds; ++i)
    {
        filedes[i] = (int*)malloc(2*sizeof(int));
        checkMalloc(filedes[i]);

        if(i != totalChilds - 1)
        {
            if(pipe(filedes[i]) == -1)
            {
                perror("pipe: ");
                return EXIT_FAILURE;
            }
        }

        pid[i] = fork();
        if(pid[i] < 0)
        {
            perror("fork: ");
            return EXIT_FAILURE;
        }
        else if(pid[i] == 0)
        {
            // printf("\tChild process [%d]: PID = %d (PPID = %d)\n", i, getpid(), getppid());
            if(i == 0)
            {
                // Codice eseguito solo per il primo comando
                if(nSpecialCharacter(cmd, '<') == 1)
                {
                    char* nameFile = getFilename(input, '<');
                    if(nameFile == NULL)
                    {
                        fprintf(stderr, "executePipe(): error at getFilename()\n");
                        return EXIT_FAILURE;
                    }

                    if(executeRedirection_aux(nameFile, STDIN_FILENO) != EXIT_SUCCESS)
                    {
                        fprintf(stderr, "executePipe(): error at executeRedirection_aux(STDIN_FILENO):\n");
                        return EXIT_FAILURE;
                    }
                }
                if(dup2(filedes[i][1], STDOUT_FILENO) == -1)
                {
                    fprintf(stderr,"executePipe(): first dup2 failed!\n");
                    return EXIT_FAILURE;
                }
                close(filedes[i][0]);   // 0 LETTURA PIPE
                close(filedes[i][1]);   // 1 SCRITTURA PIPE
            }
            else if(i == totalChilds-1)
            {
                // Codice eseguito solo per l'ultimo comando
                if(nSpecialCharacter(cmd, '>') == 1)
                {
                    char* nameFile = getFilename(input, '>');
                    if(nameFile == NULL)
                    {
                        fprintf(stderr, "executePipe(): error at getFilename()\n");
                        return EXIT_FAILURE;
                    }

                    if(executeRedirection_aux(nameFile, STDOUT_FILENO) != EXIT_SUCCESS)
                    {
                        fprintf(stderr, "executePipe(): error at executeRedirection_aux(STDOUT_FILENO):\n");
                        return EXIT_FAILURE;
                    }
                }

                if(dup2(filedes[i-1][0], STDIN_FILENO) == -1)
                {
                    fprintf(stderr, "executePipe(): last dup2 failed!\n");
                    return EXIT_FAILURE;
                }
                close(filedes[i-1][1]);
                close(filedes[i-1][0]);
            }
            else
            {
                // Codice eseguito solo per i comandi in "mezzo"
                if(dup2(filedes[i-1][0], STDIN_FILENO) == -1)
                {
                    fprintf(stderr, "executePipe(): middle 1 dup2 failed!\n");
                    return EXIT_FAILURE;
                }
                close(filedes[i-1][1]);
                close(filedes[i][0]);

                if(dup2(filedes[i][1], STDOUT_FILENO) == -1)
                {
                    fprintf(stderr, "executePipe(): middle 2 dup2 failed!\n");
                    return EXIT_FAILURE;
                }
                close(filedes[i-1][0]);
                close(filedes[i][1]);
            }

            char **command = convertList(cmd);
            int status_code = execvp(command[0], command);
            if (status_code == -1)
            {
                // Se un comando faila bisogna terminare tutto e ripulire la memoria allocata
                perror("execvp: ");

                for(int j = 0; j < i + 1; ++j)
                {
                    kill(pid[j], SIGKILL);
                    free(filedes[i]);
                }
                free(pid);
                free(filedes);

                return EXIT_FAILURE;
            }
        }
        else
        {
            // Codice eseguito dal padre
            if(i >= 1)
            {
                if(totalChilds == 2)
                {
                    close(filedes[0][0]);
                    close(filedes[0][1]);  
                }
                else
                {
                    close(filedes[i-1][0]);
                    close(filedes[i-1][1]);
                }
            }
            cmd = getNextCommand(cmd);
        }
    }

    for(int i = 0; i < totalChilds; ++i)
    {
        pid_t child = pid[i];
        checkWait(child);
    }

    free(pid);
    for(int i = 0; i < totalChilds; ++i)
    {
        free(filedes[i]);
    }
    free(filedes);

    return EXIT_SUCCESS;
}

int executeCommand(command command)
{
    char **command_argument_list = NULL;
    int commandSize = nCommand(command);
    int nRedirectIn, nRedirectOut;

    if(commandSize == 1)
    {
        nRedirectIn = nSpecialCharacter(command, '<');
        nRedirectOut = nSpecialCharacter(command, '>');

        if(nRedirectIn == 1 && nRedirectOut == 0)
        {
            if(executeRedirection(command, STDIN_FILENO) != EXIT_SUCCESS)
            {
                fprintf(stderr, "executeCommand(): error at executeRedirection(STDIN_FILENO)\n");
                return EXIT_FAILURE;
            }
        }
        else if(nRedirectIn == 0 && nRedirectOut == 1)
        {
            if(executeRedirection(command, STDOUT_FILENO) != EXIT_SUCCESS)
            {
                fprintf(stderr, "executeCommand(): error at executeRedirection(STDOUT_FILENO)\n");
                return EXIT_FAILURE;
            }
        }
        else if(nRedirectIn == 1 && nRedirectOut == 1)
        {
            if(executeDoubleRedirection(command) != EXIT_SUCCESS)
            {
                fprintf(stderr, "executeCommand(): error at executeDoubleRedirection()\n");
                return EXIT_FAILURE;
            }
        }
        else if(strcmp(command->payload, "cd") == 0)
        {
            if(cd(command) != EXIT_SUCCESS)
            {
                fprintf(stderr, "executeCommand(): error at cd()\n");
                return EXIT_FAILURE;
            }
        }
        else
        {
            // Se è un comando unico senza ridirezioni (es. ls -l)
            command_argument_list = convertList(command);
            int retValue = startCommand(command_argument_list);
            if(retValue == EXIT_FAILURE)
            {
                free(command_argument_list);
                fprintf(stderr, "executeCommand(): error at startCommand()\n");
                return EXIT_FAILURE;
            }
            else if(retValue == COMMAND_NOT_FOUND)
            {
                free(command_argument_list);
                fprintf(stderr, "comando %s non trovato\n", command->payload);
                return EXIT_FAILURE;
            }
            free(command_argument_list);
        }
    }
    else
    {
        // Se è una pipe di dimensione N (es. ls -l | grep ubash | ... | comandoN)
        if(executePipe(command) != EXIT_SUCCESS)
        {
            fprintf(stderr, "executeCommand(): error at executePipe()\n");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
//╔══════════════════════════════════════════════════════════════════════╗
//║                                 MAIN                                 ║
//╚══════════════════════════════════════════════════════════════════════╝
int main()
{
    for(;;)
    {
        printCurrentDirectory();
        char *input = readCommand();

        if(checkRead(input))
        {
            if(strcmp(input, "exit") != 0)
            {
                command parsedInput = parseString(input);
                if(checkCommand(parsedInput))
                {
                    if(executeCommand(parsedInput) == EXIT_FAILURE)
                    {
                        fprintf(stderr, "main(): error at executeCommand()\n");
                    }
                }
                freeCommandList(parsedInput);
            }
            else
            {
                exit(EXIT_SUCCESS);
            }
        }
        free(input);
    }
}