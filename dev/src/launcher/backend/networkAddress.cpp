#include "build.h"
#include "networkAddress.h"
#include <ws2tcpip.h>

namespace net 
{

	Address::Address()
	{
		memset( &m_data, 0, sizeof(m_data) );
	}

	Address::Address( const sockaddr_in& addr )
	{
		memcpy( &m_data, &addr, sizeof(addr) );
		m_data.ss_family = AF_INET;
	}

	Address::Address( const sockaddr_in6& addr )
	{
		memcpy( &m_data, &addr, sizeof(addr) );
		m_data.ss_family = AF_INET6;
	}

	bool Address::IsValid() const
	{
		return (m_data.ss_family == AF_INET) || (m_data.ss_family == AF_INET6);
	}

	void Address::SetPort( unsigned short newPort )
	{
		if ( m_data.ss_family == AF_INET )
		{
			sockaddr_in* addr = (sockaddr_in*) &m_data;
			addr->sin_port = htons( newPort );
		}
		else if ( m_data.ss_family == AF_INET6 )
		{
			sockaddr_in6* addr = (sockaddr_in6*) &m_data;
			addr->sin6_port = htons( newPort );
		}
		else
		{
			GLog.Err( "Invalid family" );
		}
	}

	const sockaddr* Address::GetRaw() const
	{
		return (const sockaddr*) &m_data; 
	}

	const int Address::GetRawSize() const
	{
		if ( m_data.ss_family == AF_INET )
		{
			return sizeof(sockaddr_in);
		}
		else if ( m_data.ss_family == AF_INET6 )
		{
			return sizeof(sockaddr_in6);
		}
		else
		{
			return 0;
		}
	}

	unsigned short Address::GetPort() const
	{
		if ( m_data.ss_family == AF_INET )
		{
			const auto* addr = (const sockaddr_in*) &m_data;
			return ntohs( addr->sin_port );
		}
		else if ( m_data.ss_family == AF_INET6 )
		{
			const auto* addr = (const sockaddr_in6*) &m_data;
			return ntohs( addr->sin6_port );
		}
		else
		{
			GLog.Err( "Invalid family" );
			return 0;
		}
	}

	Address Address::MakeIp4( const char* ip, unsigned short port )
	{
		sockaddr_in addr;
		memset( &addr, 0, sizeof(addr) );

		if ( 1 != InetPtonA( AF_INET, ip, &addr ) )
			return Address();

		return Address( addr );
	}

	Address Address::MakeIp6( const char* ip, unsigned short port )
	{
		sockaddr_in6 addr;
		memset( &addr, 0, sizeof(addr) );

		if ( 1 != InetPtonA( AF_INET6, ip, &addr ) )
			return Address();

		return Address( addr );
	}

	std::string Address::ToString() const
	{
		char buf[ 512 ];
		InetNtopA( m_data.ss_family, (void*) &m_data, buf, ARRAYSIZE(buf) );
		return buf;
	}

} // net