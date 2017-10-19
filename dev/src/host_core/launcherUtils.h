#pragma once
#include "launcherBase.h"

namespace launcher
{

	//---------------------------------------------------------------------------

	class LAUNCHER_API BinaryConsumer
	{
	public:
		BinaryConsumer(const void* data, const uint32 size);

		inline const uint32 GetSize() const { return m_size; }
		inline const uint32 GetOffset() const { return m_offset; }

		inline const bool Eof() const { return (m_size == m_offset); }

		const uint32 Read(void* outBuffer, const uint32 sizeToRead);

	private:
		const uint8*	m_data;
		uint32			m_size;
		uint32			m_offset;

		BinaryConsumer(const BinaryConsumer& other);
		BinaryConsumer& operator=(const BinaryConsumer& other);
	};

	//---------------------------------------------------------------------------

	class LAUNCHER_API BinaryWriter
	{
	public:
		BinaryWriter(void* data, const uint32 maxSize);

		inline const uint32 GetSize() const { return m_size; }
		inline const uint32 GetOffset() const { return m_offset; }

		inline const bool IsFull() const { return (m_size == m_offset); }

		const uint32 Write(const void* data, const uint32 sizeToWrite);

	private:
		uint8*		m_data;
		uint32		m_size;
		uint32		m_offset;

		BinaryWriter(const BinaryWriter& other);
		BinaryWriter& operator=(const BinaryWriter& other);
	};

	//---------------------------------------------------------------------------

	// convert string to lower case string
	extern LAUNCHER_API void MakeLower(char* dest, const uint32 destSize, const char* src);

	// conform path string
	extern LAUNCHER_API void ConformPath(char* dest, const uint32 destSize, const char* src);

	// convert ansi to unicode
	extern LAUNCHER_API void AnsiToUnicode(wchar_t* dest, const uint32 destSize, const char* src);

	// convert unicode to ansi
	extern LAUNCHER_API void UnicodeToAnsi(char* dest, const uint32 destSize, const wchar_t* src);

	// convert ansi string to unicode string
	extern LAUNCHER_API std::wstring AnsiToUnicode(const std::string& src);

	// convert unicode string to ansi string
	extern LAUNCHER_API std::string UnicodeToAnsi(const std::wstring& src);

	// decompress data buffer
	extern LAUNCHER_API bool DecompressData(const void* src, const uint32 srcSize, void* destData, uint32& destDataSize);

	//---------------------------------------------------------------------------

} // launcher