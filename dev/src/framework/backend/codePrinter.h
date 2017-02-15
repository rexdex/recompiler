#pragma once

namespace code
{

	/// universal code printer
	class RECOMPILER_API Printer
	{
	private:
		struct Page
		{
			uint8*		m_data;
			uint32		m_size;
			uint32		m_capacity;

			Page( const uint32 capacity );
			~Page();

			bool Write( const void* data, uint32& currentOffset, const uint32 size );
			bool Append( const char ch );
		};

		static const uint32 PAGE_SIZE = 65536;

		std::vector< Page* >		m_allPages;
		std::vector< Page* >		m_freePages;
		Page*						m_currentPage;

		uint32						m_totalSize;
		bool						m_newLine;

		int							m_indentLevel;

	public:
		Printer();
		~Printer();

		//! Reset printer state
		void Reset();

		//! Indent the text
		void Indent( const int delta );

		//! Unformatted print (uses the current indent)
		void Print( const char* txt );

		//! Formatted print (uses the current indent)
		void Printf( const char* txt, ... );

		//! Save generated text to file, does not modify the file if it's the same
		bool Save( const wchar_t* filePath ) const;

	private:
		Page* AllocPage();

		void FlushNewLine();
		void AppendRaw( const char ch );
		void PrintRaw( const void* data, const uint32 length );
	};

} // code