#include "build.h"
#include "xmlReader.h"
#include "platformExports.h"

namespace platform
{

	//---------------------------------------------------------------------------

	static bool ParseImportIndex(const char* str, int& outNumber)
	{
		// hex ?
		int base = 10;
		if (str[0] == '0' && str[1] == 'x')
		{
			str += 2;
			base = 16;
		}

		// get digits
		int numDigits = 0;
		char digits[16];
		while (*str != 0)
		{
			if (numDigits > sizeof(digits))
			{
				return false;
			}

			const char ch = *str++;

			if (ch >= '0' && ch <= '9')
			{
				digits[numDigits++] = (ch - '0');
			}
			else if ((ch == 'a' || ch == 'A') && base == 16)
			{
				digits[numDigits++] = 10;
			}
			else if ((ch == 'b' || ch == 'B') && base == 16)
			{
				digits[numDigits++] = 11;
			}
			else if ((ch == 'c' || ch == 'C') && base == 16)
			{
				digits[numDigits++] = 12;
			}
			else if ((ch == 'd' || ch == 'D') && base == 16)
			{
				digits[numDigits++] = 13;
			}
			else if ((ch == 'e' || ch == 'E') && base == 16)
			{
				digits[numDigits++] = 14;
			}
			else if ((ch == 'f' || ch == 'F') && base == 16)
			{
				digits[numDigits++] = 15;
			}
			else
			{
				return false;
			}
		}

		// calculate final value
		int val = 0;
		int mul = 1;
		for (int i = numDigits - 1; i >= 0; --i)
		{
			val += digits[i] * mul;
			mul *= base;
		}

		// return final value
		outNumber = val;
		return true;
	}

	//---------------------------------------------------------------------------

	bool ExportLibrary::ExportInfo::Parse(ILogOutput& log, class xml::Reader* xml)
	{
		// parse the name
		const std::string name = xml->GetAttribute("name");
		if (name.empty())
		{
			log.Error("ImageLib: Expecting export name");
			return false;
		}

		// parse the ordinal
		const std::string id = xml->GetAttribute("id");
		if (id.empty())
		{
			log.Error("ImageLib: Expecting ordinal id for export '%s'", name.c_str());
			return false;
		}

		// parse the number
		int ordinal = -1;
		if (!ParseImportIndex(id.c_str(), ordinal))
		{
			log.Error("ImageLib: Failed to pase ordinal number for export '%s' from '%s'", name.c_str(), id.c_str());
			return false;
		}

		// ok
		m_name = name;
		m_offset = 0;
		m_ordinal = ordinal;
		m_alias = "";
		return true;
	}

	//---------------------------------------------------------------------------

	ExportLibrary::ImageInfo::ImageInfo()
		: m_minMatchingVersion(0)
		, m_maxMatchingVersion(INT_MAX)
	{
	}

	const ExportLibrary::ExportInfo* ExportLibrary::ImageInfo::FindExportByOrdinal(const uint32 ordinal) const
	{
		for (TExports::const_iterator it = m_exports.begin();
		it != m_exports.end(); ++it)
		{
			const ExportInfo& info = (*it);
			if (info.GetOrdinal() == ordinal)
			{
				return &info;
			}
		}

		// not found
		return nullptr;
	}

	bool ExportLibrary::ImageInfo::Parse(ILogOutput& log, xml::Reader* xml)
	{
		// get the name
		m_name = xml->GetAttribute("name");
		if (m_name.empty())
		{
			log.Error("ImageLib: Expecting image name");
			return false;
		}

		// minimal matching version
		m_minMatchingVersion = xml->GetAttributeInt("minVersion", m_minMatchingVersion);
		m_maxMatchingVersion = xml->GetAttributeInt("maxVersion", m_maxMatchingVersion);

		// load the exports
		while (xml->Iterate("entry"))
		{
			ExportInfo info;
			if (!info.Parse(log, xml))
			{
				return false;
			}

			m_exports.push_back(info);
		}

		// valid
		return true;
	}

	//---------------------------------------------------------------------------

	ExportLibrary::ExportLibrary()
	{
	}

	ExportLibrary::~ExportLibrary()
	{
	}

	const ExportLibrary::ImageInfo* ExportLibrary::FindImageByName(const uint32 version, const char* name) const
	{
		// linear search
		for (uint32 i = 0; i < m_images.size(); ++i)
		{
			const ImageInfo& info = m_images[i];
			if (version >= info.GetMinVersion() && version <= info.GetMaxVersion())
			{
				if (0 == _stricmp(info.GetName(), name))
				{
					return &info;
				}
			}
		}

		// try without the extension
		const char* extString = strchr(name, '.');
		if (NULL != extString)
		{
			std::string tempName(name, extString);

			for (uint32 i = 0; i < m_images.size(); ++i)
			{
				const ImageInfo& info = m_images[i];
				if (version >= info.GetMinVersion() && version <= info.GetMaxVersion())
				{
					if (0 == _stricmp(info.GetName(), tempName.c_str()))
					{
						return &info;
					}
				}
			}
		}

		// not found
		return nullptr;
	}

	ExportLibrary* ExportLibrary::LoadFromXML(ILogOutput& log, const std::wstring& filePath)
	{
		// load xml
		std::auto_ptr< xml::Reader > xml(xml::Reader::LoadXML(filePath));
		if (!xml.get())
			return nullptr;

		// invalid library
		if (!xml->Open("library"))
		{
			log.Error("ImageLib: File '%ls' is not an export library", filePath.c_str());
			return nullptr;
		}
		
		// create the output object
		ExportLibrary* ret = new ExportLibrary();

		// load the images
		uint32 totalExports = 0;
		while (xml->Iterate("image"))
		{
			ImageInfo info;
			if (!info.Parse(log, xml.get()))
			{
				delete ret;
				return false;
			}

			// add to local list
			ret->m_images.push_back(info);
			totalExports += info.GetNumExports();
		}

		// stats
		log.Log("ImageLib: Loaded export library with %u images and %u exports",
			(uint32)ret->m_images.size(), totalExports);

		// return DB
		return ret;
	}

} // platform