#include "build.h"
#include "winMemory.h"

namespace win
{

	Memory::Memory()
	{
	}

	Memory::~Memory()
	{
	}

	void* Memory::AllocVirtualMemory(const uint64_t prefferedBaseAddres, const uint64_t size)
	{
		GLog.Log("Win: Requested OS to allocate %u bytes of virtual memory", size);

		const auto ret = ::VirtualAlloc((void*)prefferedBaseAddres, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		if (!ret)
		{
			GLog.Err("Win: OS virtual memory allocation has failed");
			return nullptr;
		}

		return ret;
	}

	void Memory::FreeVirtualMemory(void* address, const uint64_t size)
	{
		GLog.Log("Win: Requested OS to free %u bytes of virtual memory", size);
		::VirtualFree(address, size, MEM_DECOMMIT | MEM_RELEASE);
	}

} // win