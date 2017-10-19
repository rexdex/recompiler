#include "build.h"
#include "htmlBuilder.h"

namespace tools
{

	HTMLBuilder::HTMLBuilder(const bool bright)
		: m_blockStart(false)
	{
		Open("html");

		Open("body");

		if (bright)
		{
			const auto clientColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
			const auto textColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
			Attr("bgcolor", clientColor.GetAsString().c_str());
			Attr("text", textColor.GetAsString().c_str());
			//Open("font");
			//Attr("size", "10");
		}
		else
		{
			Attr("bgcolor", "#282828");// clientColor.GetAsString().c_str());
			Attr("text", "#808080");// clientColor.GetAsString().c_str());
		}
	}

	void HTMLBuilder::Open(const char* block)
	{
		StartValue();

		m_out.Append("<");
		m_out.Append(block);
		m_out.Append(" ");

		m_stack.push_back(block);
		m_blockStart = true;
	}

	void HTMLBuilder::TableColumn(const char* name, int width)
	{
		Open("th");
		Attr("width", "%d", width);
		Print(name);
		Close();
	}

	static wxString Encode(const wxString& data)
	{
		wxString buffer;
		buffer.reserve(data.size());

		const auto* cur = data.wc_str();
		while (*cur)
		{
			const auto ch = *cur++;

			switch (ch)
			{
				case '&':  
					buffer.append("&amp;");
					break;

				case '\"': 
					buffer.append("&quot;");
					break;

				case '\'': 
					buffer.append("&apos;");
					break;

				case '<': 
					buffer.append("&lt;");
					break;

				case '>':
					buffer.append("&gt;");
					break;

				default:
					buffer.append(ch);
			}
		}
		return buffer;
	}

	void HTMLBuilder::Attr(const char* name, const char* valueBuf, ...)
	{
		if (m_blockStart)
		{
			char buf[4096];
			va_list args;

			va_start(args, valueBuf);
			vsprintf_s(buf, valueBuf, args);
			va_end(args);
			
			m_out.Append(name);
			m_out.Append("=\"");
			m_out.Append(Encode(buf));
			m_out.Append("\" ");
		}
	}

	void HTMLBuilder::Print(const char* valueBuf, ...)
	{
		char buf[4096];
		va_list args;

		va_start(args, valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		StartValue();

		m_out.Append(buf);
	}

	void HTMLBuilder::PrintEncoded(const char* valueBuf, ...)
	{
		char buf[4096];
		va_list args;

		va_start(args, valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		StartValue();

		m_out.Append(Encode(buf));
	}

	void HTMLBuilder::LineBreak()
	{
		m_out.Append("<br>");
	}

	void HTMLBuilder::PrintBlock(const char* blockName, const char* valueBuf, ...)
	{
		char buf[4096];
		va_list args;

		va_start(args, valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		Open(blockName);
		StartValue();
		m_out.Append(Encode(buf));
		Close();
	}

	void HTMLBuilder::Close()
	{
		if (!m_stack.empty())
		{
			StartValue();

			const char* topName = m_stack.back();
			m_out.Append("</");
			m_out.Append(topName);
			m_out.Append(">");
			m_stack.pop_back();
		}
	}

	void HTMLBuilder::StartValue()
	{
		if (m_blockStart)
		{
			m_out.Append(" >");
			m_blockStart = false;
		}
	}

	void HTMLBuilder::Image(const char* imageName, const int forceWidth /*= -1*/, const int forceHeight /*= -1*/)
	{
		const auto fullPath = wxFileName(wxTheApp->GetEditorPath() + imageName);
		if (fullPath.Exists())
		{
			const auto src = fullPath.GetFullPath();
			Open("img");
			Attr("src", "%ls", src.wc_str());
			if (forceWidth > 0)
				Attr("width", "%d", forceWidth);
			if (forceHeight > 0)
				Attr("height", "%d", forceHeight);
			Close();
		}
	}

	wxString HTMLBuilder::Extract()
	{
		while (!m_stack.empty())
			Close();

		const auto ret = m_out;
		m_out.Clear();

		return ret;
	}

	//----

	HTMLScope::HTMLScope(HTMLBuilder& b, const char* name /*= nullptr*/)
		: m_builder(&b)
		, m_opened(name && *name)
	{
		if (m_opened)
			GetBuilder()->Open(name);
	}

	HTMLScope::HTMLScope(const HTMLScope& b, const char* name /*= nullptr*/)
		: m_builder(b.m_builder)
		, m_opened(name && *name)
	{
		if (m_opened)
			GetBuilder()->Open(name);
	}

	HTMLScope::~HTMLScope()
	{
		if (m_opened)
			GetBuilder()->Close();
	}

	void HTMLScope::Attr(const char* name, const char* valueBuf, ...)
	{
		if (m_opened)
		{
			char buf[4096];
			va_list args;

			va_start(args, valueBuf);
			vsprintf_s(buf, valueBuf, args);
			va_end(args);

			GetBuilder()->Attr(name, buf);
		}
	}

	//----

	HTMLGrid::HTMLGrid(const HTMLScope& b, const uint32 numColumns /*= 1*/, const int32 border /*= -1*/, const bool shading /*= false*/, const int forceWidth /*= -1*/, const int cellPadding /*= -1*/)
		: HTMLScope(b, "table")
		, m_headerDone(false)
		, m_headerStarted(false)
		, m_curColumn(0)
		, m_rowIndex(0)
		, m_inCell(false)
		, m_inRow(false)
		, m_shading(shading)
	{
		if (border > 0)
		{
			Attr("border", "%d", border);
			Attr("color", "#606060");
		}

		if (forceWidth > 0)
			Attr("width", "%d", forceWidth);
		if (cellPadding > 0)
			Attr("cellpadding", "%d", cellPadding);
	}

	HTMLGrid::~HTMLGrid()
	{
		CloseRow();
	}

	void HTMLGrid::AddColumn(const char* name, const int forceWidth /*= -1*/)
	{
		if (m_headerDone)
			return;

		if (!m_headerStarted)
		{
			GetBuilder()->Open("tr");
			m_headerStarted = true;
		}

		if (m_curColumn < m_numColumns)
		{		
			GetBuilder()->Open("th");

			if (m_shading)
				GetBuilder()->Attr("bgcolor", "#404040");

			if (forceWidth > 0)
				GetBuilder()->Attr("width", "%d", forceWidth);
			GetBuilder()->Print(name);
			GetBuilder()->Close(); // th
		}
	}

	void HTMLGrid::CloseHeader()
	{
		if (m_headerDone)
			return;

		if (m_headerStarted)
			GetBuilder()->Close();

		m_headerDone = true;
	}

	void HTMLGrid::CloseCell()
	{
		if (m_inCell)
		{
			GetBuilder()->Close(); // td
			m_curColumn += 1;
			m_inCell = false;
		}
	}

	void HTMLGrid::CloseRow()
	{
		if (m_inRow)
		{
			CloseCell();

			GetBuilder()->Close(); // tr
			m_inRow = false;
			m_rowIndex += 1;
		}
	}

	void HTMLGrid::BeginRow()
	{
		CloseHeader();
		CloseRow();

		wxASSERT(!m_inCell);
		wxASSERT(!m_inRow);

		m_curColumn = 0;
		GetBuilder()->Open("tr");
		m_inRow = true;

		if (m_shading)
			GetBuilder()->Attr("bgcolor", (m_rowIndex&1) ? "#606060" : "#505050");
	}

	void HTMLGrid::BeginCell()
	{
		wxASSERT(m_inRow);

		CloseCell();

		if (m_curColumn < m_numColumns)
		{
			GetBuilder()->Open("td");
			m_inCell = true;
		}
	}

	//----

	HTMLLink::HTMLLink(const HTMLScope& b)
		: HTMLScope(b, "a")
	{}

	HTMLLink::~HTMLLink()
	{

	}

	void HTMLLink::Link(const char* valueBuf, ...)
	{
		char buf[4096];
		va_list args;

		va_start(args, valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		Attr("href", buf);
	}

	void HTMLLink::Text(const char* valueBuf, ...)
	{
		char buf[4096];
		va_list args;

		va_start(args, valueBuf);
		vsprintf_s(buf, valueBuf, args);
		va_end(args);

		GetBuilder()->Open("b");
		GetBuilder()->Open("font");
		GetBuilder()->Attr("color", "#458B43");
		GetBuilder()->Print(buf);
		GetBuilder()->Close();
		GetBuilder()->Close();
	}

	//----

} // tools