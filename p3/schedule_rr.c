#include <stdlib.h>
#include <stdio.h>
#include "task.h"
#include "schedulers.h"
#include "list.h"
#include "cpu.h"

struct node *head = NULL, *tail = NULL;
int size = 0;

void add(char *name, int priority, int burst) {
    Task *temp = malloc(sizeof(struct node));
	temp->name = name;
	temp->priority = priority;
	temp->burst = burst;

     // if empty, set head
    if (size == 0) {
        insert(&head, temp);
    }

    // if task is alphabetically before head, replace head and shift old head
    else if (comesBefore(temp->name, head->task->name)) {
        insert(&head, temp);
    }

    // insertion sort
    else {
        struct node *curr = head->next, *prev = head, *newNode;
        // traverse list until insertion point found
        while (curr != NULL && comesBefore(curr->task->name, temp->name)) {
            prev = curr;
            curr = curr->next;
        }
        
        // insert before current node
        newNode = insert(&curr, temp);
        prev->next = newNode;
    }
    size++;
}

void schedule() {
    struct node *temp = head;
    // while not empty
    while (size > 0) {
        Task *task = temp->task;
        // if task's burst time greater than quantum
        if (task->burst > QUANTUM) {
            run(task, QUANTUM); // run for quantum
            task->burst -= QUANTUM; // calculate remaining burst time
            temp = temp->next; 
        }

        // otherwise run for remaining burst and remove task
        else {
            run(task, task->burst);
            temp = temp->next;
            delete(&head, task);
            size--;
        }

        // if end of list
        if (temp == NULL) {
            temp = head; // go back to head
        }
    }
}