#pragma once

namespace native
{
	class IFileSystem;
	class IMemory;
	class IKernel;

	struct LAUNCHER_API Systems
	{
		IFileSystem*	m_fileSystem;
		IMemory*		m_memory;
		IKernel*		m_kernel;

		Systems();
	};

} // native