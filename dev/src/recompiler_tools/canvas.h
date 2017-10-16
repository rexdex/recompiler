#pragma once

namespace tools
{
	namespace canvas
	{
		/// canvas font
		enum EFontType
		{
			eFontType_Normal,
			eFontType_Bold,
		};

		/// canvas vertical alignment
		enum EVerticalAlign
		{
			eVerticalAlign_Top,
			eVerticalAlign_Center,
			eVerticalAlign_Bottom,
		};

		/// canvas horizontal alignment
		enum EHorizontalAlign
		{
			eHorizontalAlign_Left,
			eHorizontalAlign_Center,
			eHorizontalAlign_Right,
		};

		/// canvas mouse capture mode
		enum EMouseCaptureMode
		{
			eMouseCapture_None,
			eMouseCapture_Clip,
			eMouseCapture_Capture,
		};

		/// canvas image
		class IEdCanvasImage
		{
		protected:
			virtual ~IEdCanvasImage() {};

		public:
			//! Add reference
			virtual void AddRef() = 0;

			//! Remove reference
			virtual void Release() = 0;

			//! Get image width
			virtual const uint32 GetWidth() const = 0;

			//! Get image height
			virtual const uint32 GetHeight() const = 0;
		};

		/// 2D canvas
		class CanvasPanel : public wxWindow
		{
			DECLARE_EVENT_TABLE();

		public:
			//! Get the position that was last clicked ( client space )
			inline const wxPoint& GetLastClickPoint() const { return m_lastClickPoint; }

		public:
			CanvasPanel(wxWindow* parent, long style);
			~CanvasPanel();

			//! Repaint canvas
			void Repaint();

			//! Capture mouse
			void CaptureMouse(EMouseCaptureMode mode);

			//! Set canvas scale
			void SetScale(float scale);

			//! Set canvas offset
			void SetOffset(float x, float y);

			//! Scroll
			void Scroll(const float dx, const float dy, const bool respectScale);

		public:
			//! Convert from point in the canvas space to client space
			wxPoint CanvasToClient(wxPoint point) const;

			//! Convert from point in the canvas space to client space
			//FVector2 CanvasToClient( FVector2 point ) const;

			//! Convert from point in the client space to the canvas space
			wxPoint ClientToCanvas(wxPoint point) const;

			//! Convert from point in the client space to the canvas space
			//FVector2 ClientToCanvas( FVector2 point ) const;

			//! Convert rect from cavas space to client space
			wxRect CanvasToClient(wxRect rect) const;

			//! Convert rect from client space to canvas space
			wxRect ClientToCanvas(wxRect rect) const;

			// Calculate text extents
			wxPoint CalcTextExtents(const EFontType font, const char* text);

			//! Get text size
			wxRect CalcTextRect(const EFontType font, const char* text);

		public:
			//! Create image from wxWidgets bitmap
			static IEdCanvasImage* CreateImage(const wxBitmap& sourceBitmap);

			//! Create image from file on disk
			static IEdCanvasImage* CreateImage(const wchar_t* absoluteFilePath);

		public:
			//! Set clipping
			void SetClip(const wxRect &rect);

			//! Reset clipping
			void ResetClip();

		public:
			//! Clear canvas to given color
			void Clear(const wxColour& color);

			//! Draw text
			void DrawText(const wxPoint& offset, EFontType font, const char* text, const wxColour &color);

			//! Draw text with alignment
			void DrawText(const wxPoint& offset, EFontType font, const char* text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign);

			//! Draw text
			void DrawText(const wxPoint& topLeft, uint32 width, uint32 height, EFontType font, const char* text, const wxColour &color);

			//! Draw wireframe rectangle
			void DrawRect(const wxRect& rect, const wxColour& color, float width = 1.0f);

			//! Draw wireframe rectangle
			void DrawRect(int x, int y, int w, int h, const wxColour& color, float width = 1.0f);

			//! Draw filled rectangle
			void FillRect(const wxRect& rect, const wxColour& color);

			//! Draw filled rectangle
			void FillRect(int x, int y, int w, int h, const wxColour& color);

			//! Draw rectangle with horizontal gradient
			void FillGradientHorizontalRect(const wxRect& rect, const wxColour& leftColor, const wxColour& rightColor);

			//! Draw rectangle with vertical gradient
			void FillGradientVerticalRect(const wxRect& rect, const wxColour& upColor, const wxColour& downColor);

			//! Draw line
			void DrawLine(int x1, int y1, int x2, int y2, const wxColour& color, float width = 1.0f);

			//! Draw line
			void DrawLine(const wxPoint& start, const wxPoint &end, const wxColour& color, float width = 1.0f);

			//! Draw lines
			void DrawLines(const wxPoint* points, const int numPoints, const wxColour& color, float width = 1.0f);

			//! Draw arc
			void DrawArc(const wxRect &rect, float startAngle, float sweepAngle, const wxColour& color, float width = 1.0f);

			//! Draw circle
			void DrawCircle(int x, int y, int radius, const wxColour& color, float width = 1.0f);

			//! Draw circle
			void DrawCircle(const wxPoint &center, int radius, const wxColour& color, float width = 1.0f);

			//! Fill cricle
			void FillCircle(int x, int y, int radius, const wxColour& color);

			//! Fill cricle
			void FillCircle(const wxPoint &center, int radius, const wxColour& color);

			//! Draw circle
			void DrawCircle(int x, int y, int w, int h, const wxColour& color, float width = 1.0f);

			//! Draw circle
			void DrawCircle(const wxRect& rect, const wxColour& color, float width = 1.0f);

			//! Fill circle
			void FillCircle(int x, int y, int w, int h, const wxColour& color);

			//! Fill circle
			void FillCircle(const wxRect& rect, const wxColour& color);

			//! Draw image
			void DrawImage(IEdCanvasImage* bitmap, int x, int y);

			//! Draw image
			void DrawImage(IEdCanvasImage* bitmap, const wxPoint& point);

			//! Draw scaled image
			void DrawImage(IEdCanvasImage* bitmap, int x, int y, int w, int h);

			//! Draw scaled image
			void DrawImage(IEdCanvasImage* bitmap, const wxRect& rect);

			//! Draw curve
			void DrawCurve(const wxPoint *points, const wxColour& color, float width = 1.0f);

			// Hit tests a cruve
			bool HitTestCurve(const wxPoint *points, const wxPoint& point, const float range = 1.0f);

			//! Draw rounded rectangle
			void DrawRoundedRect(const wxRect& rect, const wxColour& color, int radius, float borderWidth = 1.0f);

			//! Draw rounded rectangle
			void DrawRoundedRect(int x, int y, int w, int h, const wxColour& color, int radius, float borderWidth = 1.0f);

			//! Fill rounded rect
			void FillRoundedRect(const wxRect& rect, const wxColour& color, int radius);

			//! Fill rounded rect
			void FillRoundedRect(int x, int y, int w, int h, const wxColour& color, int radius);

			//! Draw slanted rectangle
			void DrawCuttedRect(const wxRect& rect, const wxColour& color, int radius, float borderWidth = 1.0f);

			//! Draw slanted rectangle
			void DrawCuttedRect(int x, int y, int w, int h, const wxColour& color, int radius, float borderWidth = 1.0f);

			//! Fill slanted rectangle
			void FillCuttedRect(const wxRect& rect, const wxColour& color, int radius);

			//! Fill slanted rectangle
			void FillCuttedRect(int x, int y, int w, int h, const wxColour& color, int radius);

			//! Draw down arrow
			void DrawDownArrow(const wxRect& rect, const wxColour& backColor, const wxColour& borderColor);

			//! Draw up arrow
			void DrawUpArrow(const wxRect& rect, const wxColour& backColor, const wxColour& borderColor);

			//! Draw triangle
			void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const wxColour& color, float borderWidth = 1.0f);

			//! Fill triangle
			void FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const wxColour& color);

			//! Draw "tip" rect
			void DrawTipRect(int x1, int y1, int x2, int y2, const wxColour& borderColor, const wxColour& interiorColor, uint32 size);

			//! Draw custom polygon
			void DrawPoly(const wxPoint *points, uint32 numPoints, const wxColour& borderColor, float borderWidth = 1.0f);

			//! Fill custom polygon
			void FillPoly(const wxPoint *points, uint32 numPoints, const wxColour& color);

		public:
			//! Paint canvas
			virtual void OnPaintCanvas(int width, int height);

			//! Mouse click event
			virtual void OnMouseClick(wxMouseEvent& event);

			//! Mouse move event ( when captured )
			virtual void OnMouseMove(wxMouseEvent& event, wxPoint delta);

			//! Mouse track event ( when not captured )
			virtual void OnMouseTrack(wxMouseEvent& event);

		private:
			//! Paint
			void OnPaint(wxPaintEvent& event);

			//! Background erase
			void OnEraseBackground(wxEraseEvent& event);

			//! Resize event
			void OnSize(wxSizeEvent& event);

			//! Mouse event
			void OnMouseEvent(wxMouseEvent& event);

			//! Mouse move event
			void OnMouseMove(wxMouseEvent& event);

		private:
			//! Update transformation matrix
			void UpdateTransformMatrix();

			//! Recreate GDI+ device
			void RecreateDevice(const uint32 newWidth, const uint32 newHeight);

			//! Toggle rendering quality
			void ToggleQualityMode(bool hiQuality);

			//! Select font
			static Gdiplus::Font* SelectFont(EFontType font);

			Gdiplus::Bitmap*	m_bitmapBuffer;		//!< Back buffer
			Gdiplus::Graphics*	m_bufferDevice;		//!< GDI+ device

			EMouseCaptureMode	m_captureMode;				//!< Current capture position
			wxPoint				m_preCapturePosition;		//!< Mouse position before capture
			wxPoint				m_lastClickPoint;			//!< Last click

			float				m_scale;			//!< Scale of canvas
			float				m_invScale;			//!< 1/scale
			float				m_offsetX;			//!< Offset in X axis
			float				m_offsetY;			//!< Offset in Y axis
		};

	} // canvas 
} // tools