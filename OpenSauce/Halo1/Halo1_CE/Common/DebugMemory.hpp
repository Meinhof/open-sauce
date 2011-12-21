/*
	Yelo: Open Sauce SDK
		Halo 1 (CE) Edition

	See license\OpenSauce\Halo1_CE for specific license information
*/
#pragma once

//#define API_DEBUG_MEMORY

#ifdef API_DEBUG_MEMORY

class c_memory_tracker
{
	struct s_memory_block
	{
		unsigned int m_allocation_index;
		void* m_pointer;
		size_t m_size;
	};

	unsigned int m_allocation_index;
	unsigned int m_total_memory;
	unsigned int m_block_count;
	s_memory_block* m_memory_blocks;

public:
	void AddBlock(void* pointer, size_t size);
	void RemoveBlock(void* pointer);

	void DumpAllocatedMemory();
};

static c_memory_tracker g_mem_tracker;

void DumpAllocatedMemory();

void* _cdecl operator new(size_t size);
void _cdecl operator delete(void* pointer);
// these may cause undefined behaviour, the original operators seemed to do something specific to arrays,
// but would not use the new and delete operators above so array memory information was being lost
void* _cdecl operator new[](size_t size);
void _cdecl operator delete[](void* pointer);

#endif
