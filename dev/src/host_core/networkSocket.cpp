#include "build.h"
#include "networkAddress.h"
#include "networkSocket.h"

namespace net
{

	Socket::Socket()
		: m_socket( INVALID_SOCKET )
		, m_connected( false )
	{}

	Socket::Socket( const Socket& other )
		: m_socket( other.m_socket )
		, m_connected( other.m_connected )
	{}

	void Socket::Close()
	{
		if ( m_socket != INVALID_SOCKET )
		{
			::closesocket( m_socket );
			m_socket = INVALID_SOCKET;
		}

		m_connected = false;
	}

	bool Socket::CreateTCP()
	{
		// already setup
		if ( m_socket != INVALID_SOCKET )
			return false;

		// connect to given address
		SOCKET s = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if ( s == INVALID_SOCKET )
			return false; // connection failed		

		// setup
		m_connected = true;
		m_socket = s;
		return true;
	}

	bool Socket::CreateUDP()
	{
		// already setup
		if ( m_socket != INVALID_SOCKET )
			return false;

		// connect to given address
		SOCKET s = ::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
		if ( s == INVALID_SOCKET )
			return false; // connection failed

		// setup
		m_connected = true;
		m_socket = s;
		return true;
	}

	bool Socket::Bind( const unsigned short port, Address& outBoundAddress )
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return false;

		// setup local address
		sockaddr_in localAddrses;
		localAddrses.sin_family = AF_INET;
		localAddrses.sin_addr.s_addr = INADDR_ANY;
		localAddrses.sin_port = htons( port );

		// bind to that address
		if ( SOCKET_ERROR == ::bind( m_socket, (const sockaddr*) &localAddrses, sizeof(localAddrses) ) )
		{
			m_connected = false;
			return false;
		}

		// set bounded address
		outBoundAddress = localAddrses;

		// bounded
		return true;
	}

	bool Socket::Listen()
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return false;

		// listen
		if ( SOCKET_ERROR == ::listen( m_socket, SOMAXCONN ) )
		{
			m_connected = false;
			return false;
		}

		// ok
		return true;
	}

	bool Socket::Connect( const Address& address )
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return false;

		// connect to address
		if ( SOCKET_ERROR == connect( m_socket, address.GetRaw(), address.GetRawSize() ) )
		{
			Close();
			return false;
		}

		// wait for socket to be created
		{
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;

			fd_set writeset;
			FD_ZERO( &writeset );

			fd_set errorset;
			FD_ZERO( &errorset );

			FD_SET( m_socket, &writeset );
			FD_SET( m_socket, &errorset );

			select( (int)m_socket, nullptr, &writeset, &errorset, &timeout );

			// socket dropped
			if ( !FD_ISSET( m_socket, &writeset ) )
			{
				Close();
				return false;
			}
		}
		
		// connected
		return true;
	}

	bool Socket::IsConnected()
	{
		return (m_socket != INVALID_SOCKET) && m_connected;
	}

	int Socket::Send( const void* data, const unsigned int dataSize )
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return 0;

		// send data
		const int ret = send( m_socket, static_cast< const char* >( data ), dataSize, 0 );

		// failed ?
		if( ret < 0 )
		{
			const int code = WSAGetLastError();
			if ( code != WSAEWOULDBLOCK )
			{
				Close(); // connection dropped
				return 0;
			}
		}

		// ok
		return ret;
	}

	int Socket::Receive( void* data, const unsigned int maxSize )
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return 0;

		// get data from network
		int result = ::recv( m_socket, (char*) data, maxSize, 0 );
		if ( result < 0 )
		{
			const auto errorCode = WSAGetLastError();
			if ( errorCode != WSAEWOULDBLOCK )
			{
				Close();
			}
			else
			{
				// No bytes received, connection still active
				result = 0;
			}
		}
		else if ( result == 0 && maxSize != 0 )
		{
			Close();
		}

		return result;
	}

	int Socket::SendTo( const Address& address, const void* data, const unsigned int dataSize )
	{
		return sendto( m_socket, (const char*)&data, dataSize, 0, (const sockaddr*) address.GetRaw(), sizeof(sockaddr_in) );
	}

	int Socket::ReceiveFrom( void* data, const unsigned int maxSize, Address& outAddress )
	{
		sockaddr sourceAddress; 
		int len = sizeof(sourceAddress);
		const int ret = ::recvfrom( m_socket, (char*)data, maxSize, 0, &sourceAddress, &len );

		if ( ret < 0 )
		{
			const auto errorCode = WSAGetLastError();
			if ( errorCode != WSAEWOULDBLOCK)
			{
				Close();
			}

			return 0;
		}

		return ret;
	}

	Socket Socket::Accept( Address& outRemoteAddress )
	{
		// invalid socket
		if ( m_socket == INVALID_SOCKET || !m_connected )
			return Socket();

		// prepare storage for the connection address
		sockaddr_storage destinationAddress;
		memset( &destinationAddress, 0, sizeof( destinationAddress ) );
		int sizeofDestinationAddress = sizeof( destinationAddress );

		// get the connection socket from low level
		SOCKET connectedSocket = ::accept( m_socket, (sockaddr*) &destinationAddress, &sizeofDestinationAddress );
		if ( connectedSocket == INVALID_SOCKET )
		{
			const auto errorCode = WSAGetLastError();	// connection dropped
			if ( errorCode != WSAEWOULDBLOCK )
			{
				Close();
			}

			return Socket();
		}

		// setup output address
		if ( destinationAddress.ss_family == AF_INET )
			outRemoteAddress = Address( *(const sockaddr_in*) &destinationAddress );
		else if ( destinationAddress.ss_family == AF_INET6 )
			outRemoteAddress = Address( *(const sockaddr_in6*) &destinationAddress );

		// create socket
		Socket ret;
		ret.m_socket = connectedSocket;
		ret.m_connected = true;
		return ret;
	}

} // net