#pragma once

namespace trace
{

	//---------------------------------------------------------------------------

	class PagedFileReader;

	//---------------------------------------------------------------------------

	/// Page file interface (paged, multithreaded access to large file)
	class RECOMPILER_API PagedFile
	{
	public:
		PagedFile();
		~PagedFile();
	
		// get file size
		const uint64 GetSize() const;

		// create reader
		class PagedFileReader* CreateReader() const;

		// open file
		static PagedFile* OpenFile(const std::wstring& filePath);

	private:
		static const uint32 PAGE_SIZE = 4096;

		FILE*	m_file;
		uint64	m_fileSize;

		// read page data
		const bool ReadPage(const uint32 pageIndex, uint8* outBuffer, uint32& outBufferSize) const;

		friend class PagedFileReader;
	};

	//---------------------------------------------------------------------------

	/// Page file reader
	class RECOMPILER_API PagedFileReader
	{
	public:
		PagedFileReader(const PagedFile* file);
		~PagedFileReader();

		// get file offset
		const uint64 GetOffset() const;

		// get file size
		const uint64 GetSize() const;

		// read data
		const bool Read(void* outData, const uint32 dataSize);

		// skip forward
		const void Skip(const uint32 size);

		// seek buffer to new position
		const void Seek(const uint64 newOffset);

	private:
		const PagedFile*		m_file;

		uint8			m_buffer[PagedFile::PAGE_SIZE];
		uint32			m_bufferSize;
		uint32			m_bufferOffset;
		uint32			m_bufferPage;

		uint32			m_pageIndex;
	};

	//-------------------------------------------------------------------------

} // trace