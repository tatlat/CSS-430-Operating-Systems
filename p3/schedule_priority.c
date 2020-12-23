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

    // if empty or temp greater priority than head task, set head
    if (size == 0 || temp->priority > head->task->priority) {
        insert(&head, temp);
    }

    // if temp equals head priority but temp comes before head lexicographically
    // then replace and shift head
    else if (temp->priority == head->task->priority 
    && comesBefore(temp->name, head->task->name)) {
        insert(&head, temp);
    }

    // insertion sort
    else {
        struct node *curr = head->next, *prev = head, *newNode;
        while (curr != NULL && curr->task->priority >= temp->priority
        && comesBefore(curr->task->name, temp->name)) {
            prev = curr;
            curr = curr->next;
        }

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