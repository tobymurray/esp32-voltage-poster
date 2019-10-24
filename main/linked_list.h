// Taken from https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm

#ifndef linked_list_h
#define linked_list_h

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct node {
   int data;
   int key;
   struct node *next;
};

//display the list
void printList();
//insert link at the first location
void insertFirst(int key, int data);
//delete first item
struct node* deleteFirst();
//is list empty
bool isEmpty();
int length();
//find a link with given key
struct node* find(int key);
//delete a link with given key
struct node* delete(int key);
void sort();
void reverse(struct node** head_ref);

#endif