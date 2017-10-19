#pragma once

namespace net
{
	class Address;
	class Socket;

	typedef int TConnectionID;
	typedef int TListenerID;

	// raw connection interface
	class IConnectionInterface
	{
	public:
		virtual ~IConnectionInterface() {};

		//! Called when connection is dropped, not called if that's us that closed the connection, not called on exit
		//! This callback is asynchronous and can happen at any time but is not reentrant
		virtual void OnDisconnected( const TConnectionID connectionID ) = 0;

		//! Called whenever we receive data, it's up to us to process it
		//! This callback is asynchronous and can happen at any time but is not reentrant
		virtual void OnData( const void* data, const unsigned int dataSize, const Address& incomingAddress, const TConnectionID connectionID ) = 0;
	};

	// raw listener interface
	class IListenerInterface
	{
	public:
		virtual ~IListenerInterface() {};

		//! Called when listener is closed due to some problems, not called when listener is closes by us or on exit
		//! This callback is asynchronous and can happen at any time but is not reentrant
		virtual void OnClosed( const TListenerID listenerID ) = 0;

		//! Called whenever we receive data, it's up to us to process it, returning FALSE will deny the connection
		//! This callback is asynchronous and can happen at any time but is not reentrant
		virtual bool OnConnection( const Address& incomingAddress, const TListenerID listenerID, const TConnectionID connectionID, IConnectionInterface*& outConnectionInterface ) = 0;				
	};

	/// Manager for RAW network
	class Manager
	{
	public:
		// create local manager
		static Manager* Create();

		// Shutdown raw network
		virtual void Shutdown() = 0;

		//! Open a TCP connection to given address
		//! Specifying a callback interface is required
		//! This returns valid connection ID if successful or 0 if not
		//! This is a blocking call
		virtual TConnectionID CreateConnection( const Address& address, IConnectionInterface* callbackInterface ) = 0;

		//! Open a TCP listener at given port
		//! Specifying a callback interface is required
		//! This returns valid listener ID if successful or 0 if not
		//! This is a blocking call
		virtual TListenerID CreateListener( const unsigned short localPort, IListenerInterface* callbackInterface ) = 0;

		//! Close given connection, returns true if successful
		//! Requires valid connection to be specified
		//! This is a blocking call
		virtual bool CloseConnection( const TConnectionID connectionID ) = 0;

		//! Close given listener, returns true if successful
		//! Requires valid listener to be specified
		//! This is a blocking call
		virtual bool  CloseListener( const TListenerID listenerID ) = 0;

		//! Get remove address of connection, returns true if successful
		//! Requires valid connection to be specified
		//! This is a non blocking call
		virtual bool GetConnectionAddress( const TConnectionID connectionID, Address& outAddress ) = 0;

		//! Send data through connection, returns number of bytes sent
		//! Requires valid listener to be specified
		//! This can fail and call OnDisconnected on the connection
		//! This is a non blocking call
		virtual int Send( const TConnectionID connectionID, const void* data, const int dataSize ) = 0;

	protected:
		virtual ~Manager() {};
	};

} // net