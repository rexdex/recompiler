#pragma once

#include <atomic>
#include <mutex>

#include "networkManager.h"
#include "networkAddress.h"
#include "networkSocket.h"

namespace net
{
	class Address;

	/// Manager for RAW network - implementation
	class ManagerImpl : public Manager
	{
	public:
		ManagerImpl();

		// Initialize raw network
		void Initialize();

		// Shutdown raw network
		virtual void Shutdown() override;

		//! Open a TCP connection to given address
		//! Specifying a callback interface is required
		//! This returns valid connection ID if successful or 0 if not
		//! This is a blocking call
		virtual TConnectionID CreateConnection( const Address& address, IConnectionInterface* callbackInterface ) override;

		//! Open a TCP listener at given port
		//! Specifying a callback interface is required
		//! This returns valid listener ID if successful or 0 if not
		//! This is a blocking call
		virtual TListenerID CreateListener( const unsigned short localPort, IListenerInterface* callbackInterface ) override;

		//! Close given connection, returns true if successful
		//! Requires valid connection to be specified
		//! This is a blocking call
		virtual bool CloseConnection( const TConnectionID connectionID ) override;

		//! Close given listener, returns true if successful
		//! Requires valid listener to be specified
		//! This is a blocking call
		virtual bool CloseListener( const TListenerID listenerID ) override;

		//! Get remove address of connection, returns true if successful
		//! Requires valid connection to be specified
		//! This is a non blocking call
		virtual bool GetConnectionAddress( const TConnectionID connectionID, Address& outAddress ) override;

		//! Send data through connection, returns number of bytes sent
		//! Requires valid listener to be specified
		//! This can fail and call OnDisconnected on the connection
		//! This is a non blocking call
		virtual int Send( const TConnectionID connectionID, const void* data, const int dataSize ) override;

	private:
		// Create internal wake socket
		void BindWakeSocket();

		// Wake up the processing thread
		void WakeThread();

		// Process socket events
		void WaitForAction( fd_set& readSet, fd_set& writeSet, fd_set& errorSet );
		void ProcessNewConnections_NoLock( fd_set& readSet );
		void ProcessNewData_NoLock( fd_set& readSet );
		void RemovePendingObjects_NoLock();

		static const unsigned int WAKE_THREAD_PORT_RANGE_START = 36150;
		static const unsigned int WAKE_THREAD_PORT_RANGE_END = 36250;

		static const unsigned int MAX_CONNECTIONS = 45;
		static const unsigned int MAX_LISTENERS = 16;

		static const unsigned int MAX_PACKET = 4096;

		static const unsigned int FLAG_SHIFT = 4;					// bit shift for flag mask vs ID
		static const unsigned int FLAG_MASK = 0xF;				// mask for flags in the ID
		static const unsigned int FLAG_CONNECTION = 0x1;			// connection flag
		static const unsigned int FLAG_LISTENER = 0x2;			// listener flag

		struct RawListener
		{
			Socket						m_socket;			// network socket
			TListenerID					m_assignedID;		// assigned ID, zero if not assigned
			IListenerInterface*			m_interface;		// communication interface
			Address						m_localAddress;		// local address
			bool						m_remove;			// request to remove this listener from list
		};

		struct RawConnection
		{
			Socket						m_socket;			// connection
			TConnectionID				m_assignedID;		// assigned ID, zero if not assigned
			RawListener*				m_owner;			// null if manual connection
			IConnectionInterface*		m_interface;		// communication interface
			Address						m_address;			// remote address
			bool						m_remove;			// request to remove this connection from list
		};

		//! Dummy socket used to trigger thread wakeup when we're sending data
		Socket m_threadWaker;
		Address m_threadWakerAddress;

		//! Initialized/Shutdown flag 
		std::atomic< bool > m_initializedFlag;
		std::atomic< bool > m_shutdownFlag;

		//! ID allocator for connections and listeners
		std::atomic< int > m_idAllocator;

		// For both connections and pending connections
		mutable std::mutex m_lock;

		//! Active outgoing connections
		std::vector< RawConnection > m_connections;

		//! Active listeners
		std::vector< RawListener > m_listeners;

		//! Worker thread
		HANDLE		m_hThread;

		// Thread function
		static DWORD WINAPI StaticThreadFunc( LPVOID lpThreadParameter );
		void ThreadFunc();
	};

} // net