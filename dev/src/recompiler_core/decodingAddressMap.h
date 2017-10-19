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
		AddressMap(class MemoryMap* map);
		~AddressMap();

		// Get the saved branch address (absolute) for given code address, returns 0 if failed
		const uint64 GetReferencedAddress(const uint64 addr) const;

		// Get addresses that are referencing given address
		void GetAddressReferencers(const uint64 addr, std::vector<uint64>& outAddresses) const;

		// Set branch address for given line
		void SetReferencedAddress(const uint64 addr, const uint64 targetAddress);

		// Save the memory map
		void Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		// Load the memory map
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

	private:
		// memory map
		class MemoryMap* m_memoryMap;

		// branch map
		typedef std::map<uint64, uint64> TAddressMap;
		TAddressMap m_addressMap;

		// internal dirty flag
		mutable bool m_isModified;
	};

} // decoding
