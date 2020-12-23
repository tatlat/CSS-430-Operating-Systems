#include <stdlib.h>
#include <stdio.h>
#include "task.h"
#include "schedulers.h"
#include "list.h"
#include "cpu.h"

struct node* head = NULL;
int size = 0;

void add(char *name, int priority, int burst) {
    Task *temp = malloc(sizeof(struct node));
	temp->name = name;
	temp->priority = priority;
	temp->burst = burst;

    // if empty or temp shorter than head task, set head
    if (size == 0 || temp->burst < head->task->burst) {
        insert(&head, temp);
    }

    // if temp matches head's burst time but temp comes before head in dictionary order
    // then replace and shift head
    else if (temp->burst == head->task->burst 
    && comesBefore(temp->name, head->task->name)) {
        insert(&head, temp);
    }

    // insertion sort
    else {
        struct node *curr = head->next, *prev = head, *newNode;
        // find insertion point
        while (curr != NULL && curr->task->burst <= temp->burst 
        && comesBefore(curr->task->name, temp->name)) {
            prev = curr;
            curr = curr->next;
        }

        // insert at point
        newNode = insert(&curr, temp);
        prev->next = newNode;
    }
    size++;
}

void schedule() {
    // while not empty
    while (size > 0) {
        Task *temp = head->task;
        run(temp, temp->burst); // run and remove task
        delete(&head, temp);
        size--;
    }
}