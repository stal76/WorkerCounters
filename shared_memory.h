#pragma once

#include <stdint.h>

void* CreateBufferInSharedMemory(const char* filename, uint64_t size, bool useHugeMem);
void* OpenBufferInSharedMemory(const char* filename, bool forWriting, bool useHugeMem, uint64_t* size);
