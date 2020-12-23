#include <stdlib.h>
#include <stdio.h>
#include "task.h"
#include "schedulers.h"
#include "list.h"
#include "cpu.h"

struct node* head;
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
    struct node *temp = head;
    int maxP = 0; // max priority in list

    // while list not empty
    while (size > 0) {
        Task *task = temp->task;
        maxP = task->priority;

        // if remaining burst greater than quantum
        if (task->burst > QUANTUM) {
            run(task, QUANTUM); // run for quantum
            task->burst -= QUANTUM; // update burst
            temp = temp->next;
        }

        // otherwise run for remaining time
        else {
            run(task, task->burst);
            temp = temp->next;
            delete(&head, task); // remove task
            size--;
            if (size > 0) {
                maxP = head->task->priority; // update max priority
            }
        }

        // if end of list or current priority less than max priority
        if (temp == NULL || temp->task->priority < maxP) {
            temp = head; // go back to head
        }
    }
}