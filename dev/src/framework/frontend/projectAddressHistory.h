#pragma once

/// Address history of the project (similar to the undo stack)
class ProjectAdddrsesHistory
{
public:
	static const uint32 INVALID_ADDRESS = 0xFFFFFFFF;

private:
	struct AddrInfo
	{
		uint32		m_addr;
		bool		m_manual;
	};

	// visited addresses (as stack)
	typedef std::vector< AddrInfo >		TAddressStack;
	TAddressStack			m_addressHistory;

	// current stack position (usually the last)
	uint32					m_currentHistoryItem;

public:
	ProjectAdddrsesHistory();

	// Reset whole stack to single address
	void Reset( const uint32 rvaAddress );
	
	// Update current address in the address history
	void UpdateAddress( const uint32 rvaAddress, const bool manualJump );

	// Get current navigation addres
	uint32 GetCurrentAddress() const;

	// Get address from the top of the stack
	uint32 GetTopAddress() const;

	// Can we navigate back in the history ?
	bool CanNavigateBack() const;

	// Navigate back in the address history, returns new address of INVALID_ADDRESS if not possible
	uint32 NavigateBack();

	// Can we navigate forward in the history ?
	bool CanNavigateForward() const;

	// Navigate forward in the address history
	uint32 NavigateForward();

public:
	// Get the whole address history
	void GetFullHistory( std::vector< uint32 >& outAddresses ) const;
};
