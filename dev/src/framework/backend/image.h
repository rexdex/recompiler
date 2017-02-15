#pragma once

//---------------------------------------------------------------------------

#include "object.h"

//---------------------------------------------------------------------------

namespace image
{

	//---------------------------------------------------------------------------

	class Section;
	class Export;
	class Import;
	class Reolocation;
	class Binary;

	//---------------------------------------------------------------------------

	/// General image section, needs to be implemented in the loader (so it's not abstract)
	class RECOMPILER_API Section : public CObject
	{
		friend class Binary;

	public:
		inline Binary* GetImage() const { return m_image; }

		inline const std::string& GetName() const { return m_name; }
		inline const uint32 GetVirtualAddress() const { return m_virtualAddress; }
		inline const uint32 GetVirtualSize() const { return m_virtualSize; }
		inline const uint32 GetPhysicalOffset() const { return m_physicalOffset; }
		inline const uint32 GetPhysicalSize() const { return m_physicalSize; }
		inline const bool CanRead() const { return m_isReadable; }
		inline const bool CanWrite() const { return m_isWritable; }
		inline const bool CanExecute() const { return m_isExecutable; }
		inline const std::string& GetCPUName() const { return m_cpuName; }

		Section();
		Section( Binary* parent, 
			const char* name, 
			const uint32 virtualAddress, 
			const uint32 virtualSize, 
			const uint32 physicalAddress, 
			const uint32 physicalSize, 
			const bool isReadable, 
			const bool isWritable, 
			const bool isExecutable, 
			const char* cpuName );

		// save/load the object
		virtual bool Save( class IBinaryFileWriter& writer ) const override;
		virtual bool Load( class IBinaryFileReader& reader ) override;

		// Check if specified offset is valid offset within the section
		const bool IsValidOffset( const uint32 offset ) const;

	private:
		Binary*			m_image;
		std::string		m_name;

		uint32			m_virtualAddress;
		uint32			m_virtualSize;

		uint32			m_physicalOffset;
		uint32			m_physicalSize;

		bool			m_isReadable;
		bool			m_isWritable;
		bool			m_isExecutable;

		std::string		m_cpuName;
	};

	//---------------------------------------------------------------------------

	/// General image import, needs to be implemented in the loader (so it's not abstract)
	class RECOMPILER_API Import : public CObject
	{
		friend class Binary;

	public:
		enum EImportType
		{
			eImportType_Unknown,
			eImportType_Function,
			eImportType_Data
		};

		inline Binary* GetImage() const { return m_image; }

		inline const uint32 GetExportIndex() const { return m_exportIndex; }
		inline const char* GetExportName() const { return m_exportName.c_str(); }
		inline const char* GetExportImageName() const { return m_exportImageName.c_str(); }

		inline const EImportType GetType() const { return m_type; }
		inline const uint32 GetTableAddress() const { return m_tableAddress; }
		inline const uint32 GetEntryAddress() const { return m_entryAddress; }

		Import();
		Import( Binary* parent, const uint32 index, const char* exportImageName, const uint32 tableAddress, const uint32 entryAddress, const EImportType type );
		Import( Binary* parent, const char* exportName, const char* exportImageName, const uint32 tableAddress, const uint32 entryAddress, const EImportType type );

		// save/load the object
		virtual bool Save( class IBinaryFileWriter& writer ) const override;
		virtual bool Load( class IBinaryFileReader& reader ) override;

	private:
		Binary*			m_image;

		uint32			m_exportIndex;
		std::string		m_exportName;
		std::string		m_exportImageName;

		EImportType		m_type;
		uint32			m_tableAddress;
		uint32			m_entryAddress;
	};

	//---------------------------------------------------------------------------

	/// Binary export, needs to be implemented in the loader (so it's not abstract)
	class RECOMPILER_API Export : public CObject
	{
		friend class Binary;

	public:
		inline Binary* GetImage() const { return m_image; }

		inline const uint32 GetIndex() const { return m_index; }
		inline const char* GetName() const { return m_name.c_str(); }
	
		inline const uint32 GetEntryPointAddress() const { return m_entryPointAddress; }

		inline const Import* GetForwardedImport() const { return m_forwardedImport; }

		Export();
		Export( Binary* parent, const uint32 index, const char* name, const uint32 entryPointAddress );
		Export( Binary* parent, const uint32 index, const char* name, Import* forwardedImport );
		~Export();

		// save/load the object
		virtual bool Save( class IBinaryFileWriter& writer ) const override;
		virtual bool Load( class IBinaryFileReader& reader ) override;

	private:
		Binary*			m_image;

		uint32			m_index;
		std::string		m_name;

		uint32			m_entryPointAddress;

		Import*			m_forwardedImport;
	};

	//---------------------------------------------------------------------------

	/// Binary debug information - info about memory address
	class RECOMPILER_API Symbol : public CObject
	{
		friend class Binary;

	public:
		enum ESymbolType
		{
			eSymbolType_Unknown=0,
			eSymbolType_Function=1,
			eSymbolType_Variable=2,
		};

	public:
		inline Binary* GetImage() const { return m_image; }

		inline const uint32 GetIndex() const { return m_index; }
		inline const uint32 GetAddress() const { return m_address; }
		inline const char* GetName() const { return m_name.c_str(); }
		inline const char* GetObject() const { return m_object.c_str(); }

		inline const ESymbolType GetType() const { return (ESymbolType) m_type; }

	public:
		Symbol();
		Symbol( Binary* parent, const uint32 index, const ESymbolType type, const uint32 address, const char* name, const char* object );
		~Symbol();

		// save/load the object
		virtual bool Save( class IBinaryFileWriter& writer ) const override;
		virtual bool Load( class IBinaryFileReader& reader ) override;

	protected:
		Binary*			m_image;

		uint32			m_index;
		uint32			m_address;
		std::string		m_name;
		std::string		m_object;

		uint8			m_type;
	};

	//---------------------------------------------------------------------------

	/// Binary abstraction, needs to be implemented in the loader (so it's not abstract)
	class RECOMPILER_API Binary : public CObject
	{
	public:
		inline const std::wstring& GetPath() const { return m_path; }

		inline const uint32 GetMemorySize() const { return m_memorySize; }
		inline const uint8* GetMemory() const { return m_memoryData; }

		inline const uint32 GetBaseAddress() const { return m_baseAddress; }
		inline const uint32 GetEntryAddress() const { return m_entryAddress; }

		inline const uint32 GetNumSections() const { return (uint32)m_sections.size(); }
		inline const Section* GetSection( const uint32 index ) const { return m_sections[index]; }

		inline const uint32 GetNumImports() const { return (uint32)m_imports.size(); }
		inline const Import* GetImport( const uint32 index ) const { return m_imports[index]; }

		inline const uint32 GetNumExports() const { return (uint32)m_exports.size(); }
		inline const Export* GetExports( const uint32 index ) const { return m_exports[index]; }

		inline const uint32 GetNumSymbols() const { return (uint32)m_symbols.size(); }
		inline const Symbol* GetSymbol( const uint32 index ) const { return m_symbols[index]; }

	public:
		Binary();
		~Binary();

		// save/load the object
		bool Save( class ILogOutput& log, class IBinaryFileWriter& writer ) const;
		bool Load( class ILogOutput& log, class IBinaryFileReader& reader );

		// Check if specified offset is valid offset within the section
		const bool IsValidOffset( const uint32 offset ) const;

		// Check if specified address is valid offset within the section
		const bool IsValidAddress( const uint32 address ) const;

		// Find section for given image offset (may return NULL)
		const Section* FindSectionForOffset( const uint32 offset ) const;

		// Find section for given address (may return NULL)
		const Section* FindSectionForAddress( const uint32 address ) const;

	protected:
		std::wstring			m_path;

		uint32					m_memorySize;
		const uint8*			m_memoryData;

		uint32					m_baseAddress;
		uint32					m_entryAddress;

		typedef std::vector< Section* >	TSections;
		TSections				m_sections;

		typedef std::vector< Import* >		TImports;
		TImports				m_imports;

		typedef std::vector< Export* >		TExports;
		TExports				m_exports;

		typedef std::vector< Symbol* >	TSymbols;
		TSymbols				m_symbols;
	};

	//---------------------------------------------------------------------------

} // image