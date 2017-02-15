#include "build.h"
#include "image.h"
#include "internalUtils.h"
#include "internalFile.h"

namespace image
{

	//---------------------------------------------------------------------------

	Section::Section()
		: m_name( "none" )
		, m_cpuName( "none" )
		, m_virtualAddress( 0 )
		, m_virtualSize( 0 )
		, m_physicalOffset( 0 )
		, m_physicalSize( 0 )
		, m_isReadable( false )
		, m_isWritable( false )
		, m_isExecutable( false )
	{
	}

	Section::Section( Binary* image, 
			const char* name, 
			const uint32 virtualAddress, 
			const uint32 virtualSize, 
			const uint32 physicalAddress, 
			const uint32 physicalSize, 
			const bool isReadable, 
			const bool isWritable, 
			const bool isExecutable, 
			const char* cpuName )
		: m_name( name )
		, m_cpuName( cpuName )
		, m_virtualAddress( virtualAddress )
		, m_virtualSize( virtualSize )
		, m_physicalOffset( physicalAddress )
		, m_physicalSize( physicalSize )
		, m_isReadable( isReadable )
		, m_isWritable( isWritable )
		, m_isExecutable( isExecutable )
		, m_image( image )
	{
	}

	bool Section::Save( class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_ImageSection );
		if ( !chunk ) return false;

		writer << m_name;
		writer << m_virtualAddress;
		writer << m_virtualSize;
		writer << m_physicalOffset;
		writer << m_physicalSize;
		writer << m_isReadable;
		writer << m_isWritable;
		writer << m_isExecutable;
		writer << m_cpuName;

		return CObject::Save( writer );
	}

	bool Section::Load( class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_ImageSection );
		if ( !chunk ) return false;

		reader >> m_name;
		reader >> m_virtualAddress;
		reader >> m_virtualSize;
		reader >> m_physicalOffset;
		reader >> m_physicalSize;
		reader >> m_isReadable;
		reader >> m_isWritable;
		reader >> m_isExecutable;
		reader >> m_cpuName;

		return CObject::Load( reader );
	}

	const bool Section::IsValidOffset( const uint32 offset ) const
	{
		return (offset >= m_virtualAddress) && (offset < (m_virtualAddress+m_virtualSize));
	}

	//---------------------------------------------------------------------------

	Import::Import()
		: m_image( NULL )
		, m_exportIndex( 0 )
		, m_tableAddress( 0 )
		, m_entryAddress( 0 )
		, m_type( eImportType_Unknown )
	{
	}

	Import::Import( Binary* image, const uint32 index, const char* exportImageName, const uint32 tableAddress, const uint32 entryAddress, const EImportType type )
		: m_image( image )
		, m_exportIndex( index )
		, m_exportImageName( exportImageName )
		, m_tableAddress( tableAddress )
		, m_entryAddress( entryAddress )
		, m_type( type )
	{
	}

	Import::Import( Binary* image, const char* exportName, const char* exportImageName, const uint32 tableAddress, const uint32 entryAddress, const EImportType type )
		: m_image( image )
		, m_exportIndex( 0 )
		, m_exportName( exportName )
		, m_exportImageName( exportImageName )
		, m_tableAddress( tableAddress )
		, m_entryAddress( entryAddress )
		, m_type( type )
	{
	}

	bool Import::Save( class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		writer << m_exportIndex;
		writer << m_exportName;
		writer << m_exportImageName;
		writer << m_tableAddress;
		writer << m_entryAddress;
		writer << (uint8)m_type;

		return CObject::Save( writer );
	}

	bool Import::Load( class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		reader >> m_exportIndex;
		reader >> m_exportName;
		reader >> m_exportImageName;
		reader >> m_tableAddress;
		reader >> m_entryAddress;

		uint8 type;
		reader >> type;
		m_type = (EImportType)type;

		return CObject::Load( reader );
	}

	//---------------------------------------------------------------------------

	Export::Export()
		: m_image( NULL )
		, m_index( 0 )
		, m_entryPointAddress( 0 )
		, m_forwardedImport( NULL )
	{
	}

	Export::Export( Binary* image, const uint32 index, const char* name, const uint32 entryPointAddress )
		: m_image( image )
		, m_index( index )
		, m_name( name )
		, m_entryPointAddress( entryPointAddress )
		, m_forwardedImport( NULL )
	{
	}

	Export::Export( Binary* image, const uint32 index, const char* name, Import* forwardedImport )
		: m_image( image )
		, m_index( index )
		, m_name( name )
		, m_entryPointAddress( 0 )
		, m_forwardedImport( forwardedImport )
	{
	}

	Export::~Export()
	{
		if ( m_forwardedImport != NULL )
		{
			delete m_forwardedImport;
			m_forwardedImport = NULL;
		}
	}

	bool Export::Save( class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		writer << m_index;
		writer << m_name;
		writer << m_entryPointAddress;
		writer << m_forwardedImport;

		return CObject::Save( writer );
	}

	bool Export::Load( class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		reader >> m_index;
		reader >> m_name;
		reader >> m_entryPointAddress;
		reader >> m_forwardedImport;

		return CObject::Load( reader );
	}

	//---------------------------------------------------------------------------

	Symbol::Symbol()
		: m_image( nullptr )
		, m_index( 0 )
		, m_address( 0 )
		, m_type( 0 )
	{
	}

	Symbol::Symbol( Binary* image, const uint32 index, const ESymbolType type, const uint32 address, const char* name, const char* object )
		: m_image( image )
		, m_index( index )
		, m_type( (uint8) type )
		, m_address( address )
		, m_name( name )
		, m_object( object )
	{
	}

	Symbol::~Symbol()
	{
	}

	bool Symbol::Save( class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_ImageSymbol );
		if ( !chunk ) return false;

		writer << m_index;
		writer << m_address;
		writer << m_type;
		writer << m_name;
		writer << m_object;

		return CObject::Save( writer );
	}

	bool Symbol::Load( class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_ImageSymbol );
		if ( !chunk ) return false;

		reader >> m_index;
		reader >> m_address;
		reader >> m_type;
		reader >> m_name;
		reader >> m_object;

		return CObject::Load( reader );
	}

	//---------------------------------------------------------------------------

	Binary::Binary()
		: m_memorySize( 0 )
		, m_memoryData( NULL )
		, m_baseAddress( 0 )
		, m_entryAddress( 0 )
	{
	}

	Binary::~Binary()
	{
		DeleteVector( m_sections );
		DeleteVector( m_exports );
		DeleteVector( m_imports );
		DeleteVector( m_symbols );

		delete [] m_memoryData;
	}

	bool Binary::Save( class ILogOutput& log, class IBinaryFileWriter& writer ) const
	{
		CBinaryFileChunkScope chunk( writer, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		// general stuff
		writer << m_path;
		writer << m_memorySize;
		writer << m_baseAddress;
		writer << m_entryAddress;
		writer << m_sections;
		writer << m_imports;
		writer << m_exports;
		writer << m_symbols;

		// image data
		{
			CBinaryFileChunkScope chunk2( writer, eFileChunk_ImageMemory );
			if ( !chunk ) return false;

			writer.Save( m_memoryData, m_memorySize );
		}

		// properties
		return CObject::Save( writer );
	}

	bool Binary::Load( class ILogOutput& log, class IBinaryFileReader& reader )
	{
		CBinaryFileChunkScope chunk( reader, eFileChunk_ImageImport );
		if ( !chunk ) return false;

		// general stuff
		reader >> m_path;
		reader >> m_memorySize;
		reader >> m_baseAddress;
		reader >> m_entryAddress;
		reader >> m_sections;
		reader >> m_imports;
		reader >> m_exports;
		reader >> m_symbols;

		// link back
		for ( uint32 i=0; i<m_sections.size(); ++i )
		{
			m_sections[i]->m_image = this;
		}
		for ( uint32 i=0; i<m_imports.size(); ++i )
		{
			m_imports[i]->m_image = this;
		}
		for ( uint32 i=0; i<m_exports.size(); ++i )
		{
			m_exports[i]->m_image = this;
		}
		for ( uint32 i=0; i<m_symbols.size(); ++i )
		{
			m_symbols[i]->m_image = this;
		}

		// image data
		{
			CBinaryFileChunkScope chunk2( reader, eFileChunk_ImageMemory );
			if ( !chunk ) return false;

			// allocate memory
			m_memoryData = new uint8 [m_memorySize];
			if ( !m_memoryData )
			{
				log.Error( "Image: Failed to allocate %u bytes for image data", m_memorySize );
				return false;
			}

			reader.Load( (void*)m_memoryData, m_memorySize );
		}

		// properties
		return CObject::Load( reader );
	}

	const bool Binary::IsValidOffset( const uint32 offset ) const
	{
		if ( offset < m_memorySize )
		{
			return true;
		}

		return false;
	}

	const bool Binary::IsValidAddress( const uint32 address ) const
	{
		if ( address >= m_baseAddress && address < (m_baseAddress + m_memorySize) )
		{
			return true;
		}

		return false;
	}

	const Section* Binary::FindSectionForOffset( const uint32 offset ) const
	{
		if ( offset < m_memorySize )
		{
			const uint32 numSections = GetNumSections();
			for ( uint32 i=0; i<numSections; ++i )
			{
				const Section* section = GetSection(i);
				if ( section->IsValidOffset( offset ) )
				{
					return section;
				}
			}
		}

		// nothing found
		return nullptr;
	}

	const Section* Binary::FindSectionForAddress( const uint32 address ) const
	{
		if ( address >= m_baseAddress && address < (m_baseAddress + m_memorySize) )
		{
			const uint32 offset = address - m_baseAddress;
			return FindSectionForOffset( offset );
		}

		// nothing found
		return nullptr;
	}

	//---------------------------------------------------------------------------

} // image