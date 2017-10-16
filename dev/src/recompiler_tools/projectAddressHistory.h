#pragma once

namespace tools
{
	/// Address history of the project (similar to the undo stack)
	class ProjectAdddrsesHistory
	{
	public:
		struct AddrInfo
		{
			uint64 m_addr;
			bool m_manual;
		};

		///--

		ProjectAdddrsesHistory();

		// Reset whole stack to single address
		void Reset(const uint64 rvaAddress);

		// Update current address in the address history
		void UpdateAddress(const uint64 rvaAddress, const bool manualJump);

		// Get current navigation address
		uint64 GetCurrentAddress() const;

		// Get address from the top of the stack
		uint64 GetTopAddress() const;

		// Can we navigate back in the history ?
		bool CanNavigateBack() const;

		// Navigate back in the address history, returns new address of INVALID_ADDRESS if not possible
		uint64 NavigateBack();

		// Can we navigate forward in the history ?
		bool CanNavigateForward() const;

		// Navigate forward in the address history
		uint64 NavigateForward();

		// Get the whole address history
		void GetFullHistory(std::vector<uint64>& outAddresses) const;

		//--

		// Save to file
		void Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		// Load from file
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

	private:
		// visited addresses (as stack)
		typedef std::vector<AddrInfo>		TAddressStack;
		TAddressStack m_addressHistory;

		// current stack position (usually the last)
		uint32 m_currentHistoryItem;
	};

} // tools