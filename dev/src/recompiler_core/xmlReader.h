#pragma once

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"

namespace xml
{

	/// Trivial XML reader
	class RECOMPILER_API Reader
	{
	public:
		typedef rapidxml::xml_document<char> TDoc;
		typedef rapidxml::xml_attribute<char> TAttr;
		typedef rapidxml::xml_node<char> TNode;

		typedef std::vector< const TNode* > TNodeStack;

	public:
		//! get the last error string (empty if everything is OK)
		inline const std::string& GetError() const { return m_error; }

	public:
		Reader(const char* buffer, const bool makeCopy = false);
		~Reader();

		//! Open section
		const bool Open(const char* name);

		//! Iterate over sections with the same name
		const bool Iterate(const char* name);

		//! Close section
		void Close();

		//! Get current node name
		const char* GetName() const;

		//! Get current node name
		const char* GetValue() const;

		//! Get the inner XML as string, returns text length added to "outString"
		const uint32 GetInnerXML(std::string& outString) const;

		//! Get number of attributes
		const uint32 GetNumAttributes() const;

		//! Get n-th attribute
		const std::string GetAttribute(const uint32 index, std::string* outName) const;

		//! Get attribute as string
		const std::string GetAttribute(const char* name, const std::string& defaultValue = std::string()) const;

		//! Get attribute as string
		const std::wstring GetAttributeW(const char* name, const std::wstring& defaultValue = std::wstring()) const;

		//! Get attribute as raw string
		const char* GetAttributeRaw(const char* name, const char* defaultValue = "") const;

		//! Get attribute as integer
		const int GetAttributeInt(const char* name, const int defaultValue = 0) const;

		//! Get attribute as unsigned integer
		const uint32 GetAttributeUint(const char* name, const uint32 defaultValue = 0) const;

		///----

		//! Load XML from absolute file
		static std::shared_ptr<Reader> LoadXML(const std::string& filePath);

		//! Load XML from absolute file
		static std::shared_ptr<Reader> LoadXML(const std::wstring& filePath);

	private:
		// source data buffer
		const char*		m_buffer;
		bool			m_bufferOwned;

		// XML document
		TDoc			m_doc;

		// Node stack
		TNodeStack		m_stack;

		// Error message
		std::string		m_error;
	};

} // xml