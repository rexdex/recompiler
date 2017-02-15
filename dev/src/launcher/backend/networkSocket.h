#pragma once

namespace net
{
	class Address;

	// simple class wrapper
	class Socket
	{
	public:
		Socket();
		Socket( const Socket& other );

		void Close();

		bool CreateTCP(); // waits
		bool CreateUDP(); // waits

		bool Bind( const unsigned short port, Address& outBoundAddress );
		bool Listen();

		bool Connect( const Address& address );

		bool IsConnected(); // may go false if socket is dropped

		// TCP
		int Send( const void* data, const unsigned int dataSize );
		int Receive( void* data, const unsigned int maxSize );

		// UDP
		int SendTo( const Address& address, const void* data, const unsigned int dataSize );
		int ReceiveFrom( void* data, const unsigned int maxSize, Address& outAddress );

		// Accept connection
		Socket Accept( Address& outRemoteAddress );

		// get socket
		inline SOCKET GetSocket() const { return m_socket; }

	private:
		SOCKET	m_socket;
		bool	m_connected;
	};
}