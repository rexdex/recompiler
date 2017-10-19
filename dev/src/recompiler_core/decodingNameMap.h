#pragma once

class IBinaryFileWriter;
class IBinaryFileReader;

namespace decoding
{

	class MemoryMap;

	/// Optional debug name for each address
	class RECOMPILER_API NameMap
	{
	public:
		struct SymbolInfo
		{
			const char* m_name;
			uint64 m_address;
		};

		class ISymbolConsumer
		{
		public:
			virtual ~ISymbolConsumer() {};
			virtual void ProcessSymbols(const SymbolInfo* symbols, const uint32 numSymbols) = 0;
		};

	public:
		// are the comments modified ?
		inline bool IsModified() const { return m_isModified; }

	public:
		NameMap(class MemoryMap* map);
		~NameMap();

		// Get stored name for given address 
		const char* GetName(const uint64 addr) const;

		// Set name given address
		void SetName(const uint64 addr, const char* name);

		// Save the name map
		void Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		// Load the name map
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

		// Enumerate all symbols
		void EnumerateSymbols(const char* nameFilter, ISymbolConsumer& reporter) const;

	private:
		// memory map
		class MemoryMap*					m_memoryMap;

		// names
		typedef std::map< uint64, std::string > TNameMap;
		TNameMap							m_nameMap;

		// internal dirty flag
		mutable bool						m_isModified;
	};

} // decoding