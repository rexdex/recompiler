#pragma once
#include "../recompiler_core/traceMemorySlice.h"

namespace tools
{

	/// memory endianess
	enum class MemoryEndianess
	{
		BigEndian,
		LittleEndian,
	};

	/// memory display mode
	enum class MemoryDisplayMode
	{
		Hex8,
		Hex16,
		Hex32,
		Hex64,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int8,
		Int16,
		Int32,
		Int64,
		Float,
		Double,
		AnsiChars,
		UniChars,
	};

	/// window capable of showing content of traced memory slice
	class MemoryTraceView : public wxScrolledWindow
	{
		DECLARE_EVENT_TABLE();

	public:
		MemoryTraceView(wxWindow* parent, const trace::DataFile* traceData);

		//---

		// clear selection
		void ClearSelection();

		// get selection, returns false if there's no selection
		const bool GetSelection(uint64& outStartAddress, uint64& outEndAddress) const;

		//---

		/// set memory display mode
		void SetDisplayMode(const MemoryDisplayMode mode, const uint32_t rowSize, const MemoryEndianess endianess);

		/// set time marker
		void SetTraceFrame(const TraceFrameID seq);

		/// set the top address of the memory window
		void SetAddress(const uint64_t address);

	private:
		static const uint64_t WINDOW_SIZE = 0x400000;
		static const uint64_t WINDOW_SHIFT_SIZE = 0x100000;

		// display mode, decides how the text is formatted
		MemoryDisplayMode m_displayMode;
		MemoryEndianess m_displayOrder;
		uint32_t m_displayRowSize; // number of memory addresses per line in window

		// char metrics
		wxSize m_drawCharSize;
		uint32 m_lineHeight;

		// address range for the window
		uint64_t m_windowBaseAddress;

		// source trace data
		const trace::DataFile* m_traceData;

		// current sequence
		TraceFrameID m_seq;

		//--

		struct RenderField
		{
			wxString m_text;
			uint64_t m_baseAddress;
			uint32_t m_size;
			wxRect m_drawRect;
			uint8_t m_changed;
			uint64_t m_seqHash;

			inline RenderField()
				: m_baseAddress(0)
				, m_size(0)
				, m_changed(0)
				, m_seqHash(0)
			{}
		};

		struct RenderLine
		{
			uint64 m_baseAddress;
			std::vector<trace::MemoryCell> m_cells;
			std::vector<RenderField> m_fields;
			wxRect m_drawRect;
		};

		const RenderField* m_selectionStart;
		const RenderField* m_selectionEnd;

		bool m_selectionDrag;

		std::unordered_map<uint64_t, RenderLine*> m_renderLines;

		static const uint32_t CalcNumberOfCharsPerLine(const MemoryDisplayMode displayMode, const uint32_t rowSize);
		static const uint32_t GetCharGroupSize(const MemoryDisplayMode displayMode);
		static const uint32_t GetMemoryGroupSize(const MemoryDisplayMode displayMode);
		static const wxString GetMemoryDisplayText(const void* data, const MemoryDisplayMode displayMode, const MemoryEndianess endianess);
		static const wxString GetMemoryDisplayText(const void* data, const MemoryDisplayMode displayMode);

		const RenderField* GetFieldAt(const wxPoint& point) const;
		void InvalidSelection();

		void InvalidateRenderLineCache();
		void UpdateRenderLineCaptions(RenderLine& line);
		const bool RewindRenderLine(RenderLine& line);
		void RenderRenderLine(wxDC& dc, const RenderLine& line) const;
		const bool ResetRenderLinesChangeState();

		RenderLine* CompileRenderLine(const uint64_t baseAddress);
		RenderLine* FetchRenderLine(const uint64_t baseAddress);

		void UpdateScrollRange();

		void OnPaint(wxPaintEvent& evt);
		void OnEraseBackground(wxEraseEvent& evt);
		void OnMouseClick(wxMouseEvent& evt);
	};

} // tools