#pragma once

static inline bool ParseWhiteSpaces( const char*& stream )
{
	const char* txt = stream;

	while ( *txt <= ' ' )
	{
		if ( !*txt ) return false;
		txt += 1;
	}

	stream = txt;
	return true;
}

static inline bool ParseStringFixed( const char*& stream, char* outBuffer, const uint32 maxSize )
{
	const char* txt = stream;

	if ( !ParseWhiteSpaces( txt ) )
	{
		return false;
	}

	// quotes support
	uint32 curSize = 0;
	if ( *txt == '\"' ||  *txt == '\'' )
	{
		const char esc = *txt;
		txt += 1;

		while ( *txt != 0 )
		{
			// stop parsing only when exactly the same char is encountered
			if ( *txt == esc )
			{
				txt += 1;
				break;
			}

			// mind the size
			if ( curSize >= maxSize-1 ) return false;

			// add everything else to the text
			outBuffer[ curSize++ ] = *txt;
			txt += 1;
		}
	}
	else
	{
		while ( *txt != 0 )
		{
			if ( *txt == '.' ) break;
			if ( *txt == '(' ) break;
			if ( *txt == '=' ) break;
			if ( *txt == ',' ) break;
			if ( *txt == ')' ) break;
			if ( *txt <= 32 ) break;

			// mind the size
			if ( curSize >= maxSize-1 ) return false;

			// add everything else to the text
			outBuffer[ curSize++ ] = *txt;
			txt += 1;
		}
	}

	// end of buffer reached
	if ( *txt == 0 && curSize == 0 ) return false;

	// zero end!
	outBuffer[ curSize ] = 0;

	// valid result
	stream = txt;
	return true;
}

static inline bool ParseStringAny( const char*& stream, std::string& outBuffer )
{
	const char* txt = stream;

	if ( !ParseWhiteSpaces( txt ) )
	{
		return false;
	}

	// string support
	if ( *txt == '\"' ||  *txt == '\'' )
	{
		const char esc = *txt;
		txt += 1;

		while ( *txt != 0 )
		{
			// stop parsing only when exactly the same char is encountered
			if ( *txt == esc )
			{
				txt += 1;
				break;
			}

			// output everything else
			char temp[2] = { *txt, 0 };
			outBuffer += temp;
			txt += 1;
		}
	}
	else
	{
		while ( *txt != 0 )
		{
			if ( *txt == '.' ) break;
			if ( *txt == '(' ) break;
			if ( *txt == ')' ) break;
			if ( *txt == '=' ) break;
			if ( *txt == ',' ) break;
			if ( *txt <= 32 ) break;

			// output everything else
			char temp[2] = { *txt, 0 };
			outBuffer += temp;
			txt += 1;
		}
	}

	// end of buffer reached
	if ( *txt == 0 && outBuffer.empty() ) return false;

	// valid result
	stream = txt;
	return true;
}

static inline bool ParseKeyword( const char*& stream, const char* match )
{
	const char* txt = stream;

	if ( !ParseWhiteSpaces( txt ) )
	{
		return false;
	}

	const uint32 len = (uint32)strlen( match );
	if ( 0 != strncmp( txt, match, len ) )
	{
		return false;
	}

	// matched
	stream = txt + len;
	return true;
}

