#include "build.h"
#include "networkAddress.h"
#include "networkManagerImpl.h"

#pragma comment ( lib, "Ws2_32.lib" )

namespace net
{
	ManagerImpl::ManagerImpl()
		: m_initializedFlag( false )
		, m_shutdownFlag( false )
		, m_idAllocator( 100 )
		, m_hThread( NULL )
	{

	}

	void ManagerImpl::Initialize()
	{
		// initialize only once
		if ( m_initializedFlag.exchange( true ) )
			return;

		// start thread
		BindWakeSocket();

		// create thread
		m_hThread = CreateThread( NULL, 0, &StaticThreadFunc, this, CREATE_SUSPENDED, NULL );
		DEBUG_CHECK( m_hThread != NULL );

		// start thread
		ResumeThread( m_hThread );
	}

	void ManagerImpl::Shutdown()
	{
		// do not shutdown if not yet initialized
		if ( !m_initializedFlag )
			return;

		// send shutdown notification, only once
		if ( m_shutdownFlag.exchange( true ) )
			return;

		// wake up thread
		WakeThread();

		// wait for the thread to finish
		if ( m_hThread )
		{
			WaitForSingleObject( m_hThread, 2000 );
			CloseHandle( m_hThread );
			m_hThread = NULL;
		}
	}

	TConnectionID ManagerImpl::CreateConnection( const Address& address, IConnectionInterface* callbackInterface )
	{
		// invalid interface
		if ( callbackInterface == nullptr )
			return 0;

		// allocate connection ID - this is here so we allocate even in case of a failed connection
		// this makes tracking failure a little bit easier
		const auto allocatedId = m_idAllocator++;
		const TConnectionID id = (allocatedId << FLAG_SHIFT) | FLAG_CONNECTION;

		// Contention part
		{
			std::lock_guard< std::mutex > lock( m_lock );

			Socket sock;

			// create socket
			if ( !sock.CreateTCP() )
				return false;

			// connect to given address
			if ( !sock.Connect( address ) )
				return false;

			// create connection object
			RawConnection obj;
			obj.m_assignedID = id;
			obj.m_owner = nullptr;
			obj.m_address = address;
			obj.m_interface = callbackInterface;
			obj.m_socket = sock;
			obj.m_remove = false;

			m_connections.push_back( obj );
		}

		// wake the thread so we can listen for data
		WakeThread();

		// return allocated ID
		return id;
	}

	TListenerID ManagerImpl::CreateListener( const unsigned short localPort, IListenerInterface* callbackInterface )
	{
		// invalid interface
		if ( callbackInterface == nullptr )
			return 0;

		// allocate listener ID - this is here so we allocate even in case of a failed connection
		// this makes tracking failure a little bit easier
		const auto allocatedId = m_idAllocator++;
		const TListenerID id = (allocatedId << FLAG_SHIFT) | FLAG_LISTENER;

		// Contention part
		{
			std::lock_guard< std::mutex > lock( m_lock );

			// create socket
			Socket sock;
			if ( !sock.CreateTCP() )
				return 0; // connection failed

			// bind socket to local port
			Address boundAddress;
			if ( !sock.Bind( localPort, boundAddress ) )
			{
				sock.Close();
				return 0;
			}

			// start listening
			if ( !sock.Listen() )
			{
				sock.Close();
				return 0;
			}

			// setup connection object
			RawListener obj;
			obj.m_assignedID = id;
			obj.m_interface = callbackInterface;
			obj.m_socket = sock;
			obj.m_localAddress = boundAddress;
			obj.m_remove = false;
			m_listeners.push_back( obj );
		}

		// wake the thread so we can listen for data
		WakeThread();

		// return allocated ID
		return id;
	}

	bool ManagerImpl::CloseConnection( const TConnectionID connectionID )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// find and close matching connection
		bool removed = false;
		{
			std::lock_guard< std::mutex > lock( m_lock );

			for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					// close connection without notification since we are closing on user's request
					it->m_owner = nullptr;
					it->m_interface = nullptr;
					it->m_remove = true;
					it->m_socket.Close();

					// remove from connection list
					removed = true;
					break;
				}
			}
		}

		// wake up thread to refresh list of the connections
		if ( removed )
			WakeThread();

		// return true if removed
		return removed;
	}

	bool ManagerImpl::CloseListener( const TListenerID listenerID )
	{
		// not a listener ID
		if ( !(listenerID & FLAG_LISTENER) )
			return false;

		// find and close matching connection
		bool removed = false;
		{
			std::lock_guard< std::mutex > lock( m_lock );

			for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
			{
				if ( it->m_assignedID == listenerID )
				{
					// remove connections matching this listener
					for ( auto jt = m_connections.begin(); jt != m_connections.end(); ++jt )
					{
						if ( jt->m_owner && jt->m_owner->m_assignedID == listenerID )
						{
							// cleanup connection
							jt->m_assignedID = 0;
							jt->m_interface = nullptr;
							jt->m_owner = nullptr;
							jt->m_remove = true;
							jt->m_socket.Close();
						}
					}

					// close connection without notification since we are closing on user's request
					it->m_interface = nullptr;
					it->m_remove = true;
					it->m_socket.Close();

					// remove from connection list
					removed = true;
					break;
				}
			}
		}

		// wake up thread to refresh list of the connections
		if ( removed )
			WakeThread();

		// return true if removed
		return removed;
	}

	bool ManagerImpl::GetConnectionAddress( const TConnectionID connectionID, Address& outAddress )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// contention region
		{
			std::lock_guard< std::mutex > lock( m_lock );

			for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					outAddress = it->m_address;
					return true;
				}
			}
		}

		// connection not found
		return false;
	}

	int ManagerImpl::Send( const TConnectionID connectionID, const void* data, const int dataSize )
	{
		// not a connection ID
		if ( !(connectionID & FLAG_CONNECTION) )
			return false;

		// contention region
		{
			std::lock_guard< std::mutex > lock( m_lock );

			for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
			{
				if ( it->m_assignedID == connectionID )
				{
					return it->m_socket.Send( data, dataSize ); // errors handled in the loop
				}
			}
		}

		// not found or nothing sent
		return 0;
	}

	void ManagerImpl::BindWakeSocket()
	{
		// connect to given address
		m_threadWaker.CreateUDP();

		// try to bind to ports
		for ( uint16 port = WAKE_THREAD_PORT_RANGE_START; port <= WAKE_THREAD_PORT_RANGE_END; ++port )
		{
			// bind socket to local port
			if ( m_threadWaker.Bind( port, m_threadWakerAddress ) )
				break;
		}
	}

	void ManagerImpl::WakeThread()
	{
		unsigned int buffer = 0x666;
	}

	void ManagerImpl::WaitForAction( fd_set& readSet, fd_set& writeSet, fd_set& errorSet )
	{
		FD_ZERO( &writeSet );
		FD_ZERO( &readSet );
		FD_ZERO( &errorSet );

		// add thread waker to the set
		SOCKET highestSocketId = m_threadWaker.GetSocket();
		FD_SET( m_threadWaker.GetSocket(), &readSet );

		// contention point
		{
			std::lock_guard< std::mutex > lock( m_lock );

			// listeners
			for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
			{
				if ( !it->m_remove && it->m_socket.IsConnected() )
				{
					FD_SET( it->m_socket.GetSocket(), &readSet );

					auto socketId = it->m_socket.GetSocket();
					if ( socketId > highestSocketId )
						highestSocketId = socketId;
				}
			}

			// connections
			for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
			{
				if ( !it->m_remove && it->m_socket.IsConnected() )
				{
					auto socketId = it->m_socket.GetSocket();
					if ( socketId > highestSocketId )
						highestSocketId = socketId;

					FD_SET( it->m_socket.GetSocket(), &readSet );
				}
			}
		}

		// Has to be the value of the highest socket descriptor + 1
		++highestSocketId;

		// Wait for sockets
		select( (int)highestSocketId, &readSet, &writeSet, &errorSet, nullptr );
	}

	void  ManagerImpl::ProcessNewConnections_NoLock( fd_set& readSet )
	{
		for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
		{
			// removed listener
			if ( it->m_remove )
				continue;

			// something new on this socket ?
			if ( !FD_ISSET( it->m_socket.GetSocket(), &readSet ) )
				continue;

			// connection dropped
			if ( !it->m_socket.IsConnected() )
			{
				// remove from list of listeners
				// NOTE: this will not break the iteration
				auto callback = it->m_interface;
				auto assignedId = it->m_assignedID;

				// cleanup
				it->m_interface = nullptr;
				it->m_assignedID = 0;
				it->m_remove = true;
				it->m_socket.Close();

				// notify that the listener got closed
				callback->OnClosed( assignedId );				
				continue;
			}

			// accept the connection
			Address newSocketAddr;
			Socket newSocket = it->m_socket.Accept( newSocketAddr );
			if ( !newSocket.IsConnected() )
				continue;

			// allocate ID
			const auto allocatedId = m_idAllocator++;
			const TConnectionID connectionID = (allocatedId << FLAG_SHIFT) | FLAG_CONNECTION;
			const TListenerID listenerID = it->m_assignedID;

			// ask the interface if it allows such connection
			IConnectionInterface* connectionInterface = nullptr;
			if ( !it->m_interface->OnConnection( newSocketAddr, listenerID, connectionID, connectionInterface ) || !connectionInterface )
			{
				newSocket.Close();
				continue;
			}

			// create connection 
			RawConnection obj;
			obj.m_assignedID = connectionID;
			obj.m_interface = connectionInterface;
			obj.m_address = newSocketAddr;
			obj.m_owner = &*it;
			obj.m_socket = newSocket;
			obj.m_remove = false;
			m_connections.push_back( obj );
		}
	}

	void ManagerImpl::ProcessNewData_NoLock( fd_set& readSet )
	{
		for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
		{
			// connection is about to be removed
			if ( it->m_remove )
				continue;

			// valid socket ?
			if ( it->m_socket.IsConnected() )
			{
				// something new on this socket ?
				if ( !FD_ISSET( it->m_socket.GetSocket(), &readSet ) )
					continue;

				// try to read data
				unsigned char buffer[ MAX_PACKET+10 ];
				int size = it->m_socket.Receive( buffer, ARRAYSIZE(buffer) );
				while ( it->m_socket.IsConnected() && size > 0 )
				{
					// process data
					if ( it->m_interface != nullptr )
					{
						it->m_interface->OnData( buffer, size, it->m_address, it->m_assignedID );
					}

					// try to read next packet
					size = it->m_socket.Receive( buffer, MAX_PACKET );
				}
			}

			// lost ? - we need to recheck because we may have been closed while in reading
			if ( !it->m_socket.IsConnected() )
			{
				// remove from list of connection
				// NOTE: this will not break the iteration
				auto callback = it->m_interface;
				auto assignedId = it->m_assignedID;

				// cleanup
				it->m_assignedID = 0;
				it->m_interface = nullptr;
				it->m_owner = nullptr;
				it->m_remove = true;

				// notify that the listener got closed
				callback->OnDisconnected( assignedId );
				continue;
			}
		}
	}

	void ManagerImpl::RemovePendingObjects_NoLock()
	{
		// cleanup connections
		for ( auto it = m_connections.begin(); it != m_connections.end(); ++it )
		{
			if ( it->m_remove )
			{
				it->m_socket.Close();
				it = m_connections.erase( it );
			}
		}

		// cleanup listeners
		for ( auto it = m_listeners.begin(); it != m_listeners.end(); ++it )
		{
			if ( it->m_remove )
			{
				it->m_socket.Close();
				it = m_listeners.erase( it );
			}
		}
	}

	void ManagerImpl::ThreadFunc()
	{		
		while ( !m_shutdownFlag )
		{
			// Wait for the sockets
			fd_set readSet, writeSet, errorSet;
			WaitForAction( readSet, writeSet, errorSet );

			// Purge waker buffer
			if ( FD_ISSET( m_threadWaker.GetSocket(), &readSet ) )
			{
				Address localhost;
				unsigned char buffer[ 32 ];
				m_threadWaker.ReceiveFrom( buffer, sizeof( buffer ), localhost );
			}

			// Contention block
			{
				std::lock_guard< std::mutex > lock( m_lock );

				// Process incoming connections
				ProcessNewConnections_NoLock( readSet );

				// Process incoming data
				ProcessNewData_NoLock( readSet );

				// Remove connections and listeners
				RemovePendingObjects_NoLock();
			}
		}
	}

	DWORD WINAPI ManagerImpl::StaticThreadFunc( LPVOID lpThreadParameter )
	{
		ManagerImpl* impl = (ManagerImpl*) lpThreadParameter;
		impl->ThreadFunc();
		return 0;
	}

	Manager* Manager::Create()
	{
		ManagerImpl* ret = new ManagerImpl();
		ret->Initialize();
		return ret;
	}
}

