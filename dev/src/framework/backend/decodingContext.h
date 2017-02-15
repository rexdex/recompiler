#pragma once

namespace image
{
	class Binary;
	class Section;
}

namespace platform
{
	class Definition;
}

namespace decoding
{
	class MemoryMap;
	class CommentMap;
	class AddressMap;
	class NameMap;
	class Instruction;
	class InstructionCache;

	/// Decoding context - main data holder for the module being decoded
	class RECOMPILER_API Context
	{
	public:
		// get the image being decoded
		inline const image::Binary* GetImage() const { return m_image; }

		/// get the platform definition this context is operating on
		inline const platform::Definition* GetPlatform() const { return m_platform; }

		// get the loading address of the image
		inline const uint32 GetBaseOffsetRVA() const { return m_baseOffsetRVA; }

		// get the image entry point
		inline const uint32 GetEntryPointRVA() const { return m_entryPointRVA; }

		// get the memory map
		inline MemoryMap& GetMemoryMap() { return *m_memoryMap; }
		inline const MemoryMap& GetMemoryMap() const { return *m_memoryMap; }

		// get the comment map
		inline CommentMap& GetCommentMap() { return *m_commentMap; }
		inline const CommentMap& GetCommentMap() const { return *m_commentMap; }

		// get the address map
		inline AddressMap& GetAddressMap() { return *m_addressMap; }
		inline const AddressMap& GetAddressMap() const { return *m_addressMap; }

		// get the name map
		inline NameMap& GetNameMap() { return *m_nameMap; }
		inline const NameMap& GetNameMap() const { return *m_nameMap; }

	public:
		~Context();

		// clear cached data (decoded instructions, etc)
		void ClearCachedData();

		// test if address contains valid instruction, if so, size of the instruction in bytes will be returned
		const uint32 ValidateInstruction( ILogOutput& log, const uint32 codeAddress, const bool cached = true );

		// decode full representation of instruction (can be cached to save performance)
		const uint32 DecodeInstruction( ILogOutput& log, const uint32 codeAddress, Instruction& outInstruction, const bool cached = true );

		// get function name for given code address (if known, if not known an automatic function name is returned)
		const bool GetFunctionName( const uint32 codeAddress, std::string& outFunctionName, uint32& outFunctionStart ) const;

		// create decoding context for given image
		static Context* Create( ILogOutput& log, const image::Binary* moduleImage, const platform::Definition* platformDefinition );

	private:
		Context( const image::Binary* module, const platform::Definition* platformDefinition );

		// image being decoded
		const image::Binary*	m_image;

		// platform 
		const platform::Definition*	m_platform;

		// base offset (as claimed from the image)
		uint32					m_baseOffsetRVA;

		// entry point offset
		uint32					m_entryPointRVA;

		// memory map, used to render this stuff
		MemoryMap*				m_memoryMap;

		// comment map
		CommentMap*				m_commentMap;

		// address map (interconnections)
		AddressMap*				m_addressMap;

		// name map (identifiers, imports & function names)
		NameMap*				m_nameMap;

		// instruction caches, one for each CPU in the platform, used for faster decoding
		typedef std::vector< InstructionCache* > TInstructionCaches;
		TInstructionCaches		m_codeSectionInstructionCaches;

		// code sections - all executable sections, each section can have a different CPU assigned
		typedef std::vector< const image::Section* > TExecutableSections;
		TExecutableSections		m_codeSections;

		//---

		mutable const image::Section*	m_currentSection;
		mutable InstructionCache*		m_currentInstructionCache;

		// prepare instruction cache for decoding the instruction
		InstructionCache* PrepareInstructionCache( const uint32 codeAddress ) const;
	};

	//---------------------------------------------------------------------------

} // decoding