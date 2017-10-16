#pragma once

namespace platform
{

	/// Artifical (XML based) list of images and their imports, used by some platforms
	class RECOMPILER_API ExportLibrary
	{
	public:
		// export information
		class RECOMPILER_API ExportInfo
		{
		private:
			std::string		m_name;
			std::string		m_alias;
			uint32			m_ordinal;
			uint32			m_offset;

		public:
			inline const char* GetName() const { return m_name.c_str(); }
			inline const char* GetAlias() const { return m_alias.c_str(); }
			inline const uint32 GetOrdinal() const { return m_ordinal; }
			inline const uint32 GetOffset() const { return m_offset; }

			bool Parse(ILogOutput& log, class xml::Reader* reader);
		};

		// image information
		class RECOMPILER_API ImageInfo
		{
		private:
			std::string		m_name;

			// version range
			uint32			m_minMatchingVersion;
			uint32			m_maxMatchingVersion;

			// exports
			typedef std::vector< ExportInfo > TExports;
			TExports		m_exports;

		public:
			ImageInfo();

			inline const char* GetName() const { return m_name.c_str(); }

			inline const uint32 GetMinVersion() const { return m_minMatchingVersion; }
			inline const uint32 GetMaxVersion() const { return m_maxMatchingVersion; }

			inline uint32 GetNumExports() const { return (uint32)m_exports.size(); }
			inline const ExportInfo& GetExport(const uint32 index) const { return m_exports[index]; }

			const ExportInfo* FindExportByOrdinal(const uint32 ordinal) const;

			bool Parse(ILogOutput& log, class xml::Reader* reader);
		};

	public:
		ExportLibrary();
		virtual ~ExportLibrary();

		//! Find the image info by name
		const ImageInfo* FindImageByName(const uint32 version, const char* name) const;

		//! Load the image library
		static std::shared_ptr<ExportLibrary> LoadFromXML(ILogOutput& log, const std::wstring& filePath);

	private:
		// pseudo images and their exports
		typedef std::vector< ImageInfo >		TImages;
		TImages					m_images;
	};

} // platform