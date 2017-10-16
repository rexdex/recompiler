#pragma once

class IBinaryFileWriter;
class IBinaryFileReader;

namespace decoding
{
	class MemoryMap;

	/// Optional comments per line
	class RECOMPILER_API CommentMap
	{
	public:
		// are the comments modified ?
		inline bool IsModified() const { return m_isModified; }

	public:
		CommentMap(class MemoryMap* map);
		~CommentMap();

		// Get comment for line (string is not safe, copy it ASAP)
		const char* GetComment(const uint64 addr) const;

		// Set comment for given line
		// This also sets the 'comment' flag in the memory map for given address
		// Calling with empty string (NULL or "") clears the comment
		void SetComment(const uint64 addr, const char* comment);

		// Clear comment from given line
		// This also clears the 'comment' flag in the memory map for given address
		void ClearComment(const uint64 addr);

		// Save the memory map
		void Save(ILogOutput& log, IBinaryFileWriter& writer) const;

		// Load the memory map
		bool Load(ILogOutput& log, IBinaryFileReader& reader);

	private:
		// memory map
		class MemoryMap*					m_memoryMap;

		// optional comment for each address
		typedef std::map< uint64, std::string > TCommentMap;
		TCommentMap							m_commentMap;

		// internal dirty flag
		mutable bool						m_isModified;
	};

} // decoding