#pragma once

namespace tools
{

	/// helper class to build usable HTML document
	class HTMLBuilder
	{
	public:
		HTMLBuilder();

		void Open(const char* block);

		void TableColumn(const char* name, int width);

		void Attr(const char* name, const char* valueBuf, ...);

		void Print(const char* valueBuf, ...);

		void PrintBlock(const char* blockName, const char* valueBuf, ...);

		void LineBreak();

		void Image(const char* imageName, const int forceWidth=-1, const int forceHeight = -1);

		void Close();

		wxString Extract();

	private:
		wxString m_out;
		std::vector<const char*> m_stack;
		bool m_blockStart;

		void StartValue();
	};

	/// simple grid (table)
	class HTMLScope
	{
	public:
		HTMLScope(HTMLBuilder& b, const char* name = nullptr);
		HTMLScope(const HTMLScope& b, const char* name = nullptr);		
		~HTMLScope();

		void Attr(const char* name, const char* valueBuf, ...);

		inline HTMLBuilder* GetBuilder()
		{
			return m_builder;
		}

		inline HTMLBuilder* operator->()
		{
			return m_builder;
		}

	private:
		HTMLBuilder* m_builder;
		bool m_opened;
	};

	/// simple grid (table)
	class HTMLGrid : public HTMLScope
	{
	public:
		HTMLGrid(const HTMLScope& b, const uint32 numColumns=1, const int32 border=-1, const bool shading=false, const int forceWidth=-1, const int cellPadding=-1);
		~HTMLGrid();

		void AddColumn(const char* name, const int forceWidth = -1);

		void BeginRow();
		void BeginCell();

	private:
		uint32 m_curColumn;
		uint32 m_numColumns;
		bool m_headerStarted;
		bool m_headerDone;
		bool m_shading;
		bool m_inRow;
		bool m_inCell;
		uint32  m_rowIndex;

		void CloseHeader();
		void CloseCell();
		void CloseRow();
	};

	/// simple grid (table)
	class HTMLLink : public HTMLScope
	{
	public:
		HTMLLink(const HTMLScope& b);
		~HTMLLink();

		void Link(const char* txt, ...);
		void Text(const char* txt, ...);
	};

} // tools