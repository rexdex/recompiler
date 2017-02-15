#pragma once

class IBinaryFileWriter;
class IBinaryFileReader;

namespace decoding
{
	class MemoryMap;

	/// Optional map for referenced memory, also adds branches
	class RECOMPILER_API AddressMap
	{
	public:
		// are the comments modified ?
		inline bool IsModified() const { return m_isModified; }

	public:
		AddressMap( class MemoryMap* map );
		~AddressMap();

		// Get the saved branch address (absolute) for given code address, returns 0 if falied
		const uint32 GetReferencedAddress( const uint32 addr ) const;

		// Get addresses that are referencing given address
		void GetAddressReferencers( const uint32 addr, std::vector< uint32 >& outAddresses ) const;

		// Set branch address for given line
		void SetReferencedAddress( const uint32 addr, const uint32 targetAddress );

		// Save the memory map
		bool Save( ILogOutput& log, IBinaryFileWriter& writer ) const;

		// Load the memory map
		bool Load( ILogOutput& log, IBinaryFileReader& reader );

	private:
		// memory map
		class MemoryMap*					m_memoryMap;

		// branch map
		typedef std::map< uint32, uint32 > TAddressMap;
		TAddressMap							m_addressMap;

		// internal dirty flag
		mutable bool						m_isModified;
	};

} // decoding
