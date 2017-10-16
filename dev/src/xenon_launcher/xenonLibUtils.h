#pragma once

#define RETURN() return (uint32)regs.LR; // return from native function
#define RETURN_ARG(arg) { regs.R3 = arg; return (uint32)regs.LR; } 
#define RETURN_DEFAULT() { GLog.Warn( "Visited empty function '%s'", __FUNCTION__ ); regs.R3 = 0; return (uint32)regs.LR; } 

static inline uint64 ToPlatform(const uint64 val)
{
	return _byteswap_uint64(val);
}

static inline uint32 ToPlatform(const uint32 val)
{
	return _byteswap_ulong(val);
}

static inline uint16 ToPlatform(const uint16 val)
{
	return _byteswap_ushort(val);
}

static inline uint8 ToPlatform(const uint8 val)
{
	return val;
}

static inline void Append( char*& stream, char* streamEnd, char value )
{
	*stream = value;
	stream = (stream < streamEnd) ? stream+1 : stream;
}

static inline void AppendString( char*& stream, char* streamEnd, const char* value )
{
	while (*value)
	{
		Append(stream, streamEnd, *value);
		++value;
	}
}

static inline void AppendHex( char*& stream, char* streamEnd, const uint64 value )
{
	char buf[64];
	sprintf_s( buf, "%llX", value );
	AppendString( stream, streamEnd, buf );
}

static inline void AppendInt( char*& stream, char* streamEnd, const uint64 value )
{
	char buf[64];
	uint32 u32 = (uint32)value;
	sprintf_s( buf, "%d", (int&)u32 );
	AppendString( stream, streamEnd, buf );
}

static inline void AppendUint( char*& stream, char* streamEnd, const uint64 value )
{
	char buf[64];
	uint32 u32 = (uint32)value;
	sprintf_s( buf, "%u", u32 );
	AppendString( stream, streamEnd, buf );
}

//---------------------------------------------------------------------------

template< typename T >
static inline T* GetPointer( const TReg reg )
{
	T* ptr = (T*)(uint32)(reg);
	return ptr;
}

//---------------------------------------------------------------------------
