#include <stdio.h>
#include <stdlib.h>
#include <limits.h> 
#include <sys/types.h>
#include <string.h>
#define MEMSIZE 80

// the start and end memory address of a hole
typedef struct Hole
{
    int start;
    int end;
} hole;

// the name and start & end memory address of a process
typedef struct Process
{
    char name;
    int start;
    int end;
} process;

char memory[MEMSIZE + 1]; // string representing memory allocation
process processes[MEMSIZE]; // list of processes in memory
hole holes[MEMSIZE]; // list of holes in memory
int numHoles = 1; // number of holes
int allocations = 0; // number of chunks of memory allocated
int maxHole = MEMSIZE; // largest hole size
char *args[MEMSIZE]; // buffer for commands

void executeCommand(); // prototype

// converts alphabetical characters in line to uppercase
void makeUppercase(char *line) {
    int i = 0;
    while (line[i] != 0) {
        if (i == 0 && (line[i] == 'r' || line[i] == 'R')) {
            if (line[i] == 'r') line[i] = 'R';
            break;
        }
        if (line[i] >= 'a' && line[i] <= 'z') {
            line[i] = line[i] - 32;
        }
        i++;
    }
}

// initializes the memory string to be full (no processes)
void initMem() {
    for (int i = 0; i < MEMSIZE; i++) {
        memory[i] = '.'; // set each block to '.' to represent lack of process
    }
    memory[MEMSIZE] = '\0'; // null terminated string
    holes[0].start = 0; // first and only hole is entire memory
    holes[0].end = MEMSIZE - 1;
}

// returns max int between n1 and n2
int max(int n1, int n2) {
    return n1 > n2 ? n1 : n2;
}

// prints command in args buffer
void printArgs() {
    if (args[0] != NULL) printf("%s", args[0]);
    int i = 1;
    while (args[i] != NULL) {
        printf(" "); printf("%s", args[i++]);
    }
}

// splits string param line at spaces and places tokens into args buffer
void tokenize(char *line) {
    char *token, *whitespace = " ";
    int index = 0;
    token = strtok(line, whitespace); // get first token
    while (token != NULL) {
        args[index++] = token; // add token to args
        token = strtok(NULL, whitespace); // get next token
    }
    args[index] = NULL; // set end of args
}

// reads user input until return is pressed
// places input into args buffer
void getInput() {
    char *line = NULL;
    ssize_t bufsize = 0; // getline allocates buffer
    int size = getline(&line, &bufsize, stdin); // get input until user enters return
    if (size > 0) {
        line[size - 1] = 0;
        makeUppercase(line);
        tokenize(line);
    }
}

// returns 1 if size can fit into the hole
// returns 0 otherwise
int bigEnough(hole h, int size) {
    return (h.end - h.start + 1) >= size;
}

// updates the holes list as well as maxHole
void updateHoles() {
    int prev = 0; // beginning address of new hole
    numHoles = 0, maxHole = INT_MIN;
    // loop over processes list
    for (int i = 0; i < allocations; i++) {
        process p = processes[i];
        // if first process's start address greater than new holes start
        if (p.start > prev) {
            hole h = {prev, p.start - 1}; // create new hole
            holes[numHoles++] = h; // add hole to list
            maxHole = max(h.end - h.start + 1, maxHole); // update maxHole
        }
        prev = p.end + 1; // next hole starts after p ends
    }
    // if next hole's start isn't end of memory
    if (prev <= MEMSIZE - 1) {
        hole h = {prev, MEMSIZE - 1}; // create new hole for remaining memory
        holes[numHoles++] = h;
        maxHole = max(h.end - h.start + 1, maxHole);
    }
}

// insertion sorts process p into processes list
void insertSort(process p) {
    int wait = 1, i = 0;
    // loop over processes list
    for (i = 0; i < allocations; i++) {
        // if p starts before current process
        if (p.start < processes[i].start) {
            // shift all processes from current to last to the right
            for (int j = allocations; j > i; j--) {
                processes[j] = processes[j - 1];
            }
            processes[i] = p; // insert p
            allocations++;
            wait = 0;
            break;
        }
    }
    // if p comes after every process in list, then add it to end of list
    if (wait) {
        processes[i] = p;
        allocations++;
    }
}

// allocates process with name (param) with the specified start memory address
void allocate(char name, int start, int size) {
    int end = start + size - 1;
    // loop over memory from start to end
    for (int i = start; i <= end; i++) {
        memory[i] = name; // assign memory to process
    }
    process p = {name, start, end};
    insertSort(p); // add process to list
    updateHoles();
}

// clears the memory from start to end memory address
void freeMem(int start, int end) {
    for (int i = start; i <= end; i++) {
        memory[i] = '.';
    }
}

// allocates memory to process with name using the specified algorithm
void request(char name, int size, char algo) {
    // if size of process greater than largest hole
    if (size > maxHole) {
        printf("Not enough memory\n");
        return;
    }

    // if algorithm is first fit
    if (algo == 'F') {
        // loop over holes
        for (int i = 0; i < numHoles; i++) {
            hole h = holes[i];
            // allocate at first hole that is big enough
            if (bigEnough(h, size)) {
                allocate(name, h.start, size);
                break;
            }
        }
    }

    // if best fit
    else if (algo == 'B') {
        int m = INT_MAX, minIndex = -1; // m is size of smallest hole, minIndes is start address of m
        for (int i = 0; i < numHoles; i++) { // loop over holes
            hole h = holes[i];
            if (bigEnough(h, size) && h.end - h.start + 1 < m) { // if big enough and smaller than m
                m = h.end - h.start + 1; // update m and minIndex
                minIndex = i;
            }
        }

        if (minIndex > -1) { // if minIndex found, then allocate memory
            allocate(name, holes[minIndex].start, size);
        }
    }

    // if worst fit
    else if (algo == 'W') {
        for (int i = 0; i < numHoles; i++) {
            hole h = holes[i];
            if (bigEnough(h, size) && h.end - h.start + 1 == maxHole) { // if hole is largest hole
                //m = h.end - h.start + 1;
                //maxIndex = i;
                allocate(name, holes[i].start, size);
                break;
            }
        }
    }

    else {
        printf("Unknown algorithm\n");
    }
}

// frees memory of all processes with name
void release(char name) {
    for (int i = 0; i < allocations; i++) { // loop over processes
        process p = processes[i];
        if (p.name == name) { // if matching name found
            freeMem(p.start, p.end); // free memory
            for (int j = i; j < allocations; j++) { // shift processes left
                processes[j] = processes[j + 1];
            }
            i--;
            allocations--;
            updateHoles();
        }
    }
}

// compact shifts all processes left until there are zero holes between processes
// afterwards, there should be no more than 1 hole in memory
void compact() {
    int prev = 0; // start of next process
    for (int i = 0; i < allocations; i++) { // loop over processes
        int size = processes[i].end - processes[i].start;
        processes[i].start = prev; // shift process to left
        processes[i].end = processes[i].start + size;
        prev = processes[i].end + 1; // start of next process
    }

    initMem(); // clear memory
    for (int i = 0; i < allocations; i++) { // loop over processses
        int start = processes[i].start, end = processes[i].end;
        for (int j = start; j <= end; j++) { // add processes from list to memory
            memory[j] = processes[i].name;
        }
    }
    updateHoles();
}

// displays memory
void report() {
    printf("%s", memory);
    printf("\n");
}

// read opens a file and executes the command from each line
// assumes file is in correct format
void read() {
    char line[MEMSIZE];
    FILE *file = fopen(args[1], "r"); // open file
    if (file == NULL) { // file could not be opened
        printf("Could not open file\n");
        return;
    }
    while (fgets(line, sizeof(line), file)) { // while not eof, get line
        makeUppercase(line);
        tokenize(line); // tokenize line and store in args buffer
        printArgs(); // display args
        executeCommand(); // execute args
    }

    fclose(file); // close file
}

// executes the command in args buffer
void executeCommand() {
    if (args[0] == NULL) return; // return if no args

    // if allocate command, then check for valid args and then execute
    if (*args[0] == 'A' && args[1] != NULL && args[2] != NULL 
    && args[3] != NULL && atoi(args[2]) > 0) {
        request(*args[1], atoi(args[2]), *args[3]);
    } else if (*args[0] == 'F') { // Free memory command
        release(*args[1]);
    } else if (*args[0] == 'S') { // Show memory command
        report();
    } else if (*args[0] == 'R') { // Read from file command
        read();
    } else if (*args[0] == 'C') { // Compact command
        compact();
    } else {                      // invalid command
        printf("Invalid command\n");
    }
}

// main infitely waits for user input and executes command
// terminate using ctrl+c
int main() {
    initMem();
    while (1) {
        printf("allocator>");
        fflush(stdout);
        getInput();
        executeCommand();
    }
}