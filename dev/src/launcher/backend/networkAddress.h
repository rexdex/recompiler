#pragma once

#include <WinSock2.h>
#include <ws2ipdef.h>

namespace net
{
	/// Raw network address - debug only
	class Address
	{
	public:
		Address();
		Address( const sockaddr_in& addr );
		Address( const sockaddr_in6& addr );

		// is valid address ?
		bool IsValid() const;

		// change port
		void SetPort( unsigned short newPort );

		// get address port
		unsigned short GetPort() const;

		// convert to string
		std::string ToString() const;

		// make IP4 address
		static Address MakeIp4( const char* ip, unsigned short port );

		// make IP6 address (I'm testing this shit locally, you know, we run out of IPs so the IP6 will become the standard eventually)
		static Address MakeIp6( const char* ip, unsigned short port );

		// get raw data
		const sockaddr* GetRaw() const;

		// get size of raw address
		const int GetRawSize() const;

	private:
		sockaddr_storage		m_data;
	};

} // net