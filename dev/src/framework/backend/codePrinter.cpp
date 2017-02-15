#include "build.h"
#include "internalUtils.h"
#include "internalFile.h"
#include "codePrinter.h"

namespace code
{

	//---------------------------------------------------------------------------

	Printer::Page::Page( const uint32 capacity )
		: m_size( 0 )
		, m_capacity( capacity )
	{
		m_data = (uint8*) malloc( capacity );
	}

	Printer::Page::~Page()
	{
		free( m_data );
	}

	bool Printer::Page::Append( const char ch )
	{
		m_data[ m_size ] = (uint8&)ch;
		m_size += 1;
		return m_size < m_capacity;
	}

	bool Printer::Page::Write( const void* data, uint32& currentOffset, const uint32 size )
	{
		const uint32 capacityLeft = m_capacity - m_size;
		const uint32 sizeLeft = size - currentOffset;
		const uint32 writeSize = Min( capacityLeft, sizeLeft );
		memcpy( m_data + m_size, (const char*)data + currentOffset, writeSize );
		currentOffset += writeSize;
		m_size += writeSize;

		return m_size < m_capacity;
	}

	//---------------------------------------------------------------------------

	Printer::Printer()
		: m_indentLevel( 0 )
		, m_totalSize( 0 )
		, m_newLine( false )
	{
		// create first page
		m_currentPage = new Page( PAGE_SIZE );
		m_allPages.push_back( m_currentPage );
	}

	Printer::~Printer()
	{
		DeleteVector( m_allPages );
		DeleteVector( m_freePages );
		m_currentPage = 0;
	}

	void Printer::Reset()
	{
		for (uint32 i=0; i<m_allPages.size(); ++i)
		{
			Page* page = m_allPages[i];
			page->m_size = 0;
			m_freePages.push_back(page);
		}

		m_allPages.clear();

		m_currentPage = AllocPage();
		m_allPages.push_back(m_currentPage);
	}

	Printer::Page* Printer::AllocPage()
	{
		if (!m_freePages.empty())
		{
			Page* page = m_freePages.back();
			m_freePages.pop_back();
			page->m_size = 0;
			return page;
		}

		return new Page( PAGE_SIZE );
	}

	void Printer::AppendRaw( const char ch )
	{
		if ( !m_currentPage->Append( ch ) )
		{
			m_currentPage = AllocPage();
			m_allPages.push_back( m_currentPage );
		}
	}

	void Printer::PrintRaw( const void* data, const uint32 length )
	{
		uint32 currentOffset = 0;
		while ( currentOffset < length )
		{
			// write to current page
			if ( m_currentPage->Write( data, currentOffset, length ) )
			{
				break;
			}

			// allocate new page
			m_currentPage = AllocPage();
			m_allPages.push_back( m_currentPage );
		}
	}

	void Printer::Indent( const int delta )
	{
		if ( delta < 0 )
		{
			if ( m_indentLevel > -delta )
			{
				m_indentLevel += delta;
			}
			else
			{
				m_indentLevel = 0;
			}
		}
		else
		{
			m_indentLevel += delta;
		}
	}

	void Printer::FlushNewLine()
	{
		if ( m_newLine )
		{
			for ( int i=0; i<m_indentLevel; ++i )
			{
				AppendRaw( '\t' );

			}

			m_newLine = false;
		}
	}

	void Printer::Print( const char* txt )
	{
		while ( *txt != 0 )
		{
			const char ch = *txt++;

			if ( ch >= ' ' )
			{
				FlushNewLine();
				AppendRaw( ch );
			}
			else if ( ch == '\r' )
			{
				// nothing
			}
			else if ( ch == '\t' )
			{
				AppendRaw( ch );
			}
			else if ( ch == '\n' )
			{
				AppendRaw( ch );
				m_newLine = true;
			}
		}
	}

	void Printer::Printf( const char* txt, ... )
	{
		char buffer[ 8192 ];
		va_list args;

		va_start( args, txt );
		vsprintf_s( buffer, sizeof(buffer), txt, args );
		va_end( args );

		Print( buffer );
	}

	bool Printer::Save( const wchar_t* filePath ) const
	{
		// compare content
		{
			FILE* f = NULL;
			_wfopen_s( &f, filePath, L"rb" );
			if ( f )
			{
				uint8 compareBuffer[ PAGE_SIZE ];

				// compare pages
				bool different = false;
				for ( uint32 i=0; i<m_allPages.size(); ++i )
				{
					const uint32 pageSize = m_allPages[i]->m_size;

					// read data
					const uint32 read = (uint32)fread( compareBuffer, 1, pageSize, f );
					if ( read != pageSize )
					{
						different = true;
						break;
					}

					// compare data
					if ( 0 != memcmp( compareBuffer, m_allPages[i]->m_data, pageSize ) )
					{
						different = true;
						break;
					}
				}

				fclose( f );

				// do not resave if data is the same
				if ( !different )
				{
					return true;
				}
			}
		}

		// creat file path
		if ( !CreateFilePath( filePath ) )
		{
			return false;
		}

		// create output file
		FILE* f = NULL;
		_wfopen_s( &f, filePath, L"wb" );
		if ( !f ) return false;

		// flush data from all pages
		for ( uint32 i=0; i<m_allPages.size(); ++i )
		{
			fwrite( m_allPages[i]->m_data, m_allPages[i]->m_size, 1, f );
		}

		// done
		fclose(f);
		return true;
	}

} // code