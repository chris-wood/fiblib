#include <stdlib.h>
#include <stdio.h>

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_SafeMemory.h>

#include "patricia.h"

struct _patricia_node;
typedef struct _patricia_node {
    PARCBuffer *label;
    void *value;
    bool isLeaf;

    int numEdges;
    struct _patricia_node **edges;
} _PatriciaNode;

_PatriciaNode *
_patriciaNode_CreateLeaf(PARCBuffer *label, void *value)
{
    _PatriciaNode *node = (_PatriciaNode *) malloc(sizeof(_PatriciaNode));
    if (node != NULL) {
        node->label = parcBuffer_Acquire(label); // key used to get here..
        node->isLeaf = true;
        node->numEdges = 0;
        node->edges = NULL;
        node->value = value;
    }
    return node;
}

_PatriciaNode *
_patriciaNode_Copy(_PatriciaNode *other)
{
    _PatriciaNode *copy = (_PatriciaNode *) malloc(sizeof(_PatriciaNode));
    if (copy != NULL) {
        copy->label = parcBuffer_Acquire(other->label);
        copy->isLeaf = other->isLeaf;
        copy->numEdges = other->numEdges;
        copy->edges = other->edges;
        copy->value = other->value;
    }
    return copy;
}

void
_patriciaNode_AddEdge(_PatriciaNode *node, _PatriciaNode *edge) 
{
    node->numEdges++;
    if (node->edges == NULL) { 
        node->edges = (_PatriciaNode **) malloc(sizeof(_PatriciaNode *) * node->numEdges);
    } else {
        node->edges = (_PatriciaNode **) realloc(node->edges, sizeof(_PatriciaNode *) * node->numEdges);
    }
    node->edges[node->numEdges - 1] = edge;
    node->isLeaf = false;
}

static void
_patriciaNode_Display(_PatriciaNode *node, int indentation)
{
    printf("|");
    for (int i = 0; i < indentation; i++) {
        printf("-");
    }
    char *label = parcBuffer_ToString(node->label);
    printf("> %s\n", label);
    parcMemory_Deallocate(&label);

    for (int i = 0; i < node->numEdges; i++) {
        _patriciaNode_Display(node->edges[i], indentation + 2);
    };
}

struct patricia {
    _PatriciaNode *head;
};

Patricia *
patricia_Create()
{
    Patricia *patricia = (Patricia *) malloc(sizeof(Patricia));
    if (patricia != NULL) {
        PARCBuffer *empty = parcBuffer_Allocate(0);
        patricia->head = _patriciaNode_CreateLeaf(empty, NULL);
        parcBuffer_Release(&empty);
    }
    return patricia;
}

void 
patricia_Destroy(Patricia **patriciaP)
{
    Patricia *patricia = *patriciaP;
    free(patricia);
    *patriciaP = NULL;
}

static void
_patricia_Display(Patricia *trie)
{
    _patriciaNode_Display(trie->head, 0);
}

static bool
_isPrefix(PARCBuffer *x, PARCBuffer *y) 
{
    if (parcBuffer_Remaining(x) > parcBuffer_Remaining(y)) {
        return false;
    }

    for (size_t i = 0; i < parcBuffer_Remaining(x); i++) {
        if (parcBuffer_GetAtIndex(x, i) != parcBuffer_GetAtIndex(y, i)) {
            return false;
        }
    }

    return true;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static int
_sharedPrefix(PARCBuffer *x, PARCBuffer *y) 
{
    int minComponents = MIN(parcBuffer_Remaining(x), parcBuffer_Remaining(y));   

    int shared = 0;
    for (size_t i = 0; i < minComponents; i++) {
        if (parcBuffer_GetAtIndex(x, parcBuffer_Position(x) + i) != parcBuffer_GetAtIndex(y, parcBuffer_Position(y) + i)) {
            return shared;
        }
        shared++;
    }

    return shared;
}

void 
patricia_Insert(Patricia *trie, PARCBuffer *key, void *item)
{
    int elementsFound = 0;
    size_t labelLength = parcBuffer_Remaining(key);
    _PatriciaNode *prev = NULL;
    int prevEdgeIndex = 0;
    _PatriciaNode *current = trie->head;
    if (current->isLeaf) {
        current = NULL;   
    }

    // Search until we can make no further progress.
    PARCBuffer *prefix = parcBuffer_Copy(key);
    while (current != NULL && !current->isLeaf && elementsFound < labelLength) {
        _PatriciaNode *curr = current;
        current = NULL;
        for (int i = 0; i < curr->numEdges; i++) {
            _PatriciaNode *next = curr->edges[i];

            int prefixCount = parcBuffer_Remaining(prefix);
            int sharedCount = _sharedPrefix(prefix, next->label);

            if (prefixCount == sharedCount) { // jump to the next level
                prevEdgeIndex = i;
                prev = curr;
                current = next;
                size_t nextLabelLength = parcBuffer_Remaining(next->label);
                elementsFound += nextLabelLength; 
                prefix = parcBuffer_Slice(prefix);
                parcBuffer_SetPosition(prefix, parcBuffer_Position(prefix) + nextLabelLength);
                break;
            } else if (sharedCount > 0) { // split up the node
                // abc | abd -> ab, c, d
                PARCBuffer *sharedPrefix = parcBuffer_Slice(next->label); // "ab"
                PARCBuffer *leftPrefix = parcBuffer_Copy(sharedPrefix); // "c"
                parcBuffer_SetLimit(sharedPrefix, sharedCount);
                parcBuffer_SetPosition(leftPrefix, parcBuffer_Position(leftPrefix) + sharedCount);
                PARCBuffer *rightPrefix = parcBuffer_Slice(prefix); // "d"
                parcBuffer_SetPosition(rightPrefix, parcBuffer_Position(rightPrefix) + sharedCount);
                
                _PatriciaNode *splitRight = _patriciaNode_CreateLeaf(rightPrefix, item);
                _PatriciaNode *splitLeft = _patriciaNode_CreateLeaf(sharedPrefix, NULL); // no item goes here
                
                curr->edges[i] = splitLeft;
                // Add the left node to the split
                next->label = leftPrefix;
                _patriciaNode_AddEdge(splitLeft, next);

                // Add the right node to the split    
                _patriciaNode_AddEdge(splitLeft, splitRight);

                prev = splitLeft;
                current = splitRight;
                parcBuffer_SetPosition(prefix, parcBuffer_Position(prefix) + parcBuffer_Remaining(prefix));
                return;
            } else { // nothing in common
                current = NULL; // does nothing.
            }
        }
    }
 
    // Handle the insertion into the trie
    if (current == NULL) { 
        _PatriciaNode *newEdge = _patriciaNode_CreateLeaf(prefix, item);
        _PatriciaNode *target = prev == NULL ? trie->head : prev;
        _patriciaNode_AddEdge(target, newEdge);
    } else if (current->isLeaf) {
        _PatriciaNode *newEdge = _patriciaNode_CreateLeaf(prefix, item);
        _patriciaNode_AddEdge(current, newEdge);
    }
}

void *
patricia_Get(Patricia *trie, PARCBuffer *key)
{
    int elementsFound = 0;
    size_t labelLength = parcBuffer_Remaining(key);
    _PatriciaNode *current = trie->head;

    PARCBuffer *prefix = parcBuffer_Copy(key);
    while (current != NULL && !current->isLeaf && elementsFound < labelLength) {
        _PatriciaNode *curr = current;
        current = NULL;
        for (int i = 0; i < curr->numEdges; i++) {
            _PatriciaNode *next = curr->edges[i];
            
            int sharedCount = _sharedPrefix(prefix, next->label);
            if (sharedCount > 0) { // jump to the next level
                current = next;
                size_t nextLabelLength = parcBuffer_Remaining(next->label);
                elementsFound += nextLabelLength; 
                parcBuffer_SetPosition(prefix, parcBuffer_Position(prefix) + nextLabelLength);
                break;
            } 
        }
    }
    
    if (current == NULL) {
        return NULL;
    } else if (current->isLeaf && elementsFound == labelLength) {
        return current->value;
    } else {
        return NULL;
    }
}


