#include <stdlib.h>
#include <stdio.h>
#include "task.h"
#include "schedulers.h"
#include "list.h"
#include "cpu.h"

struct node* head = NULL;
int size = 0;

// adds a task to the list
void add(char *name, int priority, int burst) {
    Task *temp = malloc(sizeof(struct node)); // create new task and assign param to fields
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
    // while not empty
    while (size > 0) {
        Task *temp = head->task;
        run(temp, temp->burst); // run head
        delete(&head, temp); // remove head
        size--;
    }
}