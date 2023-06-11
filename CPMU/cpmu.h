#pragma once

#ifndef CPMU_INCLUDED
#define CPMU_INCLUDED
#include <stdlib.h>

// For debugging
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

// Special reference-counting pointer container.
// These should be allocated on the heap.
struct cpmu_reference_counted {
	void* ptr;
	void (*destructor)(void*);
	int refCount;
};

// Linked list of allocated pointers within a scope.
// These should be allocated on the heap.
struct cpmu_allocated_pointers {
	struct cpmu_reference_counted* ptr;
	struct cpmu_allocated_pointers* next;
};

// Decrease the reference count of the given reference counted object.
// If the reference count reaches zero, destroy the object.
static inline void CPMUDecreaseReferenceCount(struct cpmu_reference_counted* const ptr)
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

static inline void CPMULocalCleanup(const struct cpmu_allocated_pointers* allocated)
{
	// Iterate through each pointer and clean up
	while (allocated) {
		CPMUDecreaseReferenceCount(allocated->ptr);

		// Traverse up the linked list
		const struct cpmu_allocated_pointers* const old_allocated = allocated;
		allocated = allocated->next;

		// Destroy previous container of linked list
		free(old_allocated);
	}
}

// Creates a dynamic heap scope for the given function
// TODO dynamic scopes for struct properties, even if a bit more verbose
#define dynamicheap(X) {\
	struct cpmu_allocated_pointers* _cpmu_local_scope = NULL;\
	X\
	CPMULocalCleanup(_cpmu_local_scope);\
}

#define returndynamic(X)
#define fetchdynamic(X)

// Create an object and allocates a reference counter for it in the current local scope.
// This macro also creates the variable.
// TODO better solution than exit for out-of-memory. Perhaps force someone to declare an OOM handler.
#define createdynamic(TYPE, VAR_NAME)\
	TYPE* VAR_NAME = calloc(1, sizeof(TYPE));\
	if (!VAR_NAME) exit(-1); \
	struct cpmu_reference_counted* _cpmu_ref_##VAR_NAME = malloc(sizeof(struct cpmu_reference_counted)); \
	if (!_cpmu_ref_##VAR_NAME) exit(-1); \
	_cpmu_ref_##VAR_NAME->ptr = VAR_NAME; \
	_cpmu_ref_##VAR_NAME->refCount = 1; \
	_cpmu_ref_##VAR_NAME->destructor = NULL; \
	{\
		struct cpmu_allocated_pointers* cpmu_last_allocated_pointer = _cpmu_local_scope; \
		_cpmu_local_scope = malloc(sizeof(struct cpmu_allocated_pointers)); \
		_cpmu_local_scope->next = cpmu_last_allocated_pointer;\
		_cpmu_local_scope->ptr = _cpmu_ref_##VAR_NAME;\
	}

#endif CPMU_INCLUDED