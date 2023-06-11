#pragma once

#ifndef CMPU_INCLUDED
#define CMPU_INCLUDED
#include <stdlib.h>

// For debugging
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

// Special reference-counting pointer container.
// These should be allocated on the heap.
struct cmpu_reference_counted {
	void* ptr;
	void (*destructor)(void*);
	int refCount;
};

// Linked list of allocated pointers within a scope.
// These should be allocated on the heap.
struct cmpu_allocated_pointers {
	struct cmpu_reference_counted* ptr;
	struct cmpu_allocated_pointers* next;
};

// Decrease the reference count of the given reference counted object.
// If the reference count reaches zero, destroy the object.
static inline void CMPUDecreaseReferenceCount(struct cmpu_reference_counted* const ptr)
{
	// Decrement reference count.
	// If it reaches 0 (!0 = true) then trigger Garbage Collection.
	if (!--(ptr->refCount)) {
		puts("deleting object");

		// If there is a destructor, call the destructor
		if (ptr->destructor) {
			ptr->destructor(ptr->ptr);
		}
		// Don't free the destructor. It's a function. It's not heap-allocated.

		// Free the object being referenced
		free(ptr->ptr);
		
		// Free the reference counter itself
		free(ptr);
	}
}

static inline void CMPULocalCleanup(const struct cmpu_allocated_pointers* allocated)
{
	// Iterate through each pointer and clean up
	while (allocated) {
		CMPUDecreaseReferenceCount(allocated->ptr);

		// Traverse up the linked list
		const struct cmpu_allocated_pointers* const old_allocated = allocated;
		allocated = allocated->next;

		// Destroy previous container of linked list
		free(old_allocated);
	}
}

// Creates a dynamic heap scope for the given function
// TODO dynamic scopes for struct properties, even if a bit more verbose
#define dynamicheap(X) {\
	struct cmpu_allocated_pointers* _cmpu_local_scope = NULL;\
	X\
	CMPULocalCleanup(_cmpu_local_scope);\
}

#define returndynamic(X)
#define fetchdynamic(X)
#define createdynamic(X)

#endif CMPU_INCLUDED