#include "build.h"
#include "tracePagedFile.h"

namespace trace
{

	//-------------------------------------------------------------------------

	PagedFile::PagedFile()
		: m_file(NULL)
		, m_fileSize(0)
	{
	}

	PagedFile::~PagedFile()
	{
		// close the file
		if (NULL != m_file)
		{
			fclose(m_file);
			m_file = NULL;
		}
	}
	
	const uint64 PagedFile::GetSize() const
	{
		return m_fileSize;
	}

	class PagedFileReader* PagedFile::CreateReader() const
	{
		return new PagedFileReader(this);
	}

	const bool PagedFile::ReadPage(const uint32 pageIndex, uint8* outBuffer, uint32& outBufferSize) const
	{
		// move to page position
		const fpos_t pos = (uint64)pageIndex * (uint64)PAGE_SIZE;
		fsetpos(m_file, &pos);

		// read data
		const uint32 dataRead = (uint32)fread(outBuffer, 1, PAGE_SIZE, m_file);
		if (!dataRead)
			return false;

		// store size
		outBufferSize = dataRead;
		return true;
	}

	PagedFile* PagedFile::OpenFile(const std::wstring& filePath)
	{
		// open file
		FILE* f = NULL;
		_wfopen_s(&f, filePath.c_str(), L"rb");
		if (!f)
			return NULL;

		// get file size
		fseek(f, 0, SEEK_END);
		fpos_t pos = 0;
		fgetpos(f, &pos);
		fseek(f, 0, SEEK_SET);

		// create wrapper
		PagedFile* ret = new PagedFile();
		ret->m_file = f;
		ret->m_fileSize = (uint64)pos;
		return ret;
	}

	//-------------------------------------------------------------------------

	PagedFileReader::PagedFileReader(const PagedFile* file)
		: m_file(file)
		, m_bufferSize(0)
		, m_bufferOffset(0)
		, m_bufferPage(0)
		, m_pageIndex(0)
	{
	}

	PagedFileReader::~PagedFileReader()
	{
	}

	const uint64 PagedFileReader::GetOffset() const
	{
		const uint64 pageBase = (uint64)m_bufferPage * PagedFile::PAGE_SIZE;
		return pageBase + m_bufferOffset;
	}

	const uint64 PagedFileReader::GetSize() const
	{
		return m_file->GetSize();
	}

	const bool PagedFileReader::Read(void* outData, const uint32 dataSize)
	{
		uint8* dataWritePtr = (uint8*)outData;
		uint32 dataLeft = dataSize;
		while (dataLeft > 0)
		{
			// maximum number of data readable from this buffer
			uint32 maxRead = (m_bufferSize>m_bufferOffset) ? m_bufferSize - m_bufferOffset : 0;
			if (maxRead > dataLeft) maxRead = dataLeft;

			// no data left in current buffer, read next page from source file
			if (maxRead == 0)
			{
				// go to next page
				if (m_bufferOffset >= m_bufferSize && m_bufferSize>0)
				{
					m_bufferOffset = 0;
					m_bufferPage += 1;
				}

				// read page data
				if (!m_file->ReadPage(m_bufferPage, &m_buffer[0], m_bufferSize))
					return false; // EOF

				continue;
			}

			// copy data
			memcpy(dataWritePtr, &m_buffer[m_bufferOffset], maxRead);
			dataWritePtr += maxRead;
			m_bufferOffset += maxRead;
			dataLeft -= maxRead;
		}

		// data was fully read
		return true;
	}

	const void PagedFileReader::Skip(const uint32 size)
	{
		// fits in the same buffer ?
		if (m_bufferOffset + size < m_bufferSize)
		{
			m_bufferOffset += size;
			return;
		}

		// full seak
		uint64 currentOffset = GetOffset();
		currentOffset += size;
		Seek(currentOffset);
	}

	const void PagedFileReader::Seek(const uint64 newOffset)
	{
		// within the same page ?
		const uint32 newPageIndex = (uint32)(newOffset / PagedFile::PAGE_SIZE);
		if (newPageIndex == m_bufferPage)
		{
			m_bufferOffset = newOffset % PagedFile::PAGE_SIZE;
		}
		else
		{
			// calculate new page index, reset buffer size
			m_bufferPage = newPageIndex;
			m_bufferOffset = newOffset % PagedFile::PAGE_SIZE;
			m_bufferSize = 0; // nothing yet
		}
	}

	//-------------------------------------------------------------------------

} // trace