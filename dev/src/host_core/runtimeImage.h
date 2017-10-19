#pragma once

#include "launcherBase.h"
#include "runtimeImageInfo.h"

namespace runtime
{
	class CodeTable;
	class Symbols;

	/// image wrapper
	class LAUNCHER_API Image
	{
	public:
		// Get the code table
		inline const CodeTable* GetCodeTable() const { return m_codeTable; }

		// Get the entry point
		inline const uint64 GetEntryPoint() const { return m_imageInfo->m_entryAddress; }

		// Get image data
		inline const void* GetImageData() const { return m_imageData; }
		inline const uint32 GetImageSize() const { return m_imageSize; }
		inline const uint64 GetImageLoadAddress() const { return m_imageInfo->m_imageLoadAddress; }

	public:
		Image();
		~Image();

		// load the image from absolute file
		bool Load(const std::wstring& imageFilePath);

		// bind image internals (interrupts, functions, ports, etc) with provided symbols
		bool Bind(const Symbols& symbols);

	private:
		// image binding information
		const ImageInfo*	m_imageInfo;

		// decompressed source image data (placed at load offset)
		void*				m_imageData;
		uint32				m_imageSize;

		// image code module
		HMODULE				m_codeLibrary;
		CodeTable*			m_codeTable;
	};

} // runtime