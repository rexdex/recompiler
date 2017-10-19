#include "build.h"
#include "canvas.h"

namespace tools
{
	namespace canvas
	{
		BEGIN_EVENT_TABLE(CanvasPanel, wxWindow)
			EVT_PAINT(CanvasPanel::OnPaint)
			EVT_ERASE_BACKGROUND(CanvasPanel::OnEraseBackground)
			EVT_MOTION(CanvasPanel::OnMouseMove)
			EVT_LEFT_DOWN(CanvasPanel::OnMouseEvent)
			EVT_LEFT_DCLICK(CanvasPanel::OnMouseEvent)
			EVT_LEFT_UP(CanvasPanel::OnMouseEvent)
			EVT_RIGHT_DOWN(CanvasPanel::OnMouseEvent)
			EVT_RIGHT_DCLICK(CanvasPanel::OnMouseEvent)
			EVT_RIGHT_UP(CanvasPanel::OnMouseEvent)
			EVT_SIZE(CanvasPanel::OnSize)
		END_EVENT_TABLE()

		CanvasPanel::CanvasPanel(wxWindow* parent, long style)
			: wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
			, m_scale(1.0f)
			, m_invScale(1.0f)
			, m_offsetX(0.0f)
			, m_offsetY(0.0f)
			, m_bufferDevice(NULL)
			, m_bitmapBuffer(NULL)
			, m_captureMode(eMouseCapture_None)
		{
			// Initialize device
			const uint32 width = GetClientSize().x;
			const uint32 height = GetClientSize().y;
			RecreateDevice(width, height);
		}

		CanvasPanel::~CanvasPanel()
		{
			// Delete bitmap buffer
			if (m_bitmapBuffer)
			{
				delete m_bitmapBuffer;
				m_bitmapBuffer = NULL;
			}

			// Delete buffer device
			if (m_bufferDevice)
			{
				delete m_bufferDevice;
				m_bufferDevice = NULL;
			}
		}

		void CanvasPanel::Repaint()
		{
			Refresh(false);
		}

		void CanvasPanel::CaptureMouse(EMouseCaptureMode mode)
		{
			// Process only if something changes
			if (mode != m_captureMode)
			{
				// Uncapture mouse
				if (m_captureMode != eMouseCapture_None)
				{
					// Restore position
					::SetCursorPos(m_preCapturePosition.x, m_preCapturePosition.y);

					// Release capture
					::ReleaseCapture();

					// Release cursor
					if (m_captureMode == eMouseCapture_Clip)
					{
						// Destroy cursor clipping
						::ClipCursor(NULL);
					}
					else
					{
						// Show cursor
						while (::ShowCursor(TRUE) < 0) {}
					}
				}

				// Capture mouse
				if (mode == eMouseCapture_Clip)
				{
					// Grab current mouse position
					::GetCursorPos((POINT*)&m_preCapturePosition);

					// Capture mouse
					::SetCapture((HWND)GetHandle());

					// Create clip region
					RECT rect;
					::GetClientRect((HWND)GetHandle(), &rect);
					POINT topLeft, bottomRight;
					topLeft.x = rect.left;
					topLeft.y = rect.top;
					bottomRight.x = rect.right;
					bottomRight.y = rect.bottom;
					::ClientToScreen((HWND)GetHandle(), &topLeft);
					::ClientToScreen((HWND)GetHandle(), &bottomRight);
					rect.left = topLeft.x;
					rect.top = topLeft.y;
					rect.right = bottomRight.x;
					rect.bottom = bottomRight.y;
					::ClipCursor(&rect);
				}
				else if (mode == eMouseCapture_Capture)
				{
					// Grab current mouse position
					::GetCursorPos((POINT*)&m_preCapturePosition);

					// Capture mouse
					::SetCapture((HWND)GetHandle());

					// Hide cursor
					while (::ShowCursor(false) >= 0) {};
				}

				//! Toggle rendering quality
				bool useHiQuality = true;//( mode != eMouseCapture_None );
				ToggleQualityMode(useHiQuality);

				// Refresh
				Refresh(false);

				// Remember capture mode
				m_captureMode = mode;
			}
		}

		void CanvasPanel::SetScale(float scale)
		{
			// New scale ?
			if (scale != m_scale)
			{
				// Set new scale
				m_invScale = 1.0f / scale;
				m_scale = scale;

				// Update transformation
				UpdateTransformMatrix();
			}
		}

		void CanvasPanel::SetOffset(const float x, const float y)
		{
			// New offset ?
			if ((m_offsetX != x) || (m_offsetY != y))
			{
				m_offsetX = x;
				m_offsetY = y;

				// Update transformation
				UpdateTransformMatrix();
			}
		}

		void CanvasPanel::Scroll(const float dx, const float dy, const bool respectScale)
		{
			if (dx || dy)
			{
				m_offsetX += dx * (respectScale ? m_scale : 1.0f);
				m_offsetY += dy * (respectScale ? m_scale : 1.0f);
				UpdateTransformMatrix();
			}
		}

		wxPoint CanvasPanel::CanvasToClient(wxPoint point) const
		{
			int sx = (point.x + m_offsetX) * m_scale;
			int sy = (point.y + m_offsetY) * m_scale;
			return wxPoint(sx, sy);
		}

		/*FVector2 CanvasPanel::CanvasToClient( FVector2 point ) const
		{
			float sx = (point.x + m_offsetX) * m_scale;
			float sy = (point.y + m_offsetY) * m_scale;
			return FVector2( sx, sy );
		}*/

		wxPoint CanvasPanel::ClientToCanvas(wxPoint point) const
		{
			int sx = ((float)point.x * m_invScale) - m_offsetX;
			int sy = ((float)point.y * m_invScale) - m_offsetY;
			return wxPoint(sx, sy);
		}

		/*FVector2 CanvasPanel::ClientToCanvas( FVector2 point ) const
		{
			float sx = (point.x * m_invScale) - m_offsetX;
			float sy = (point.y * m_invScale) - m_offsetY;
			return FVector2( sx, sy );
		}*/

		wxRect CanvasPanel::CanvasToClient(wxRect rect) const
		{
			// Transform corners
			wxPoint a = CanvasToClient(wxPoint(rect.GetLeft(), rect.GetTop()));
			wxPoint b = CanvasToClient(wxPoint(rect.GetRight(), rect.GetBottom()));

			// Return transformed rect
			return wxRect(a.x, a.y, b.x - a.x, b.y - a.y);
		}

		wxRect CanvasPanel::ClientToCanvas(wxRect rect) const
		{
			// Transform corners
			wxPoint a = ClientToCanvas(wxPoint(rect.GetLeft(), rect.GetTop()));
			wxPoint b = ClientToCanvas(wxPoint(rect.GetRight(), rect.GetBottom()));

			// Return transformed rect
			return wxRect(a.x, a.y, b.x - a.x, b.y - a.y);
		}

		wxPoint CanvasPanel::CalcTextExtents(EFontType font, const char* text)
		{
			Gdiplus::RectF boundingRect(0, 0, 0, 0);

			// Measure text size
			if (text && *text)
			{
				Gdiplus::Font* drawFont = SelectFont(font);
				if (drawFont)
				{
					const wxString tempString(text);
					m_bufferDevice->MeasureString(tempString, -1, drawFont, Gdiplus::PointF(0, 0), &boundingRect);
				}
			}

			// Return measured text size
			return wxPoint(boundingRect.Width, boundingRect.Height);
		}

		wxRect CanvasPanel::CalcTextRect(EFontType font, const char* text)
		{
			wxPoint size = CalcTextExtents(font, text);
			return wxRect(0, 0, size.x, size.y);
		}

		void CanvasPanel::SetClip(const wxRect &rect)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Rect clipRect(rect.x, rect.y, rect.width, rect.height);
				m_bufferDevice->SetClip(clipRect);
			}
		}

		void CanvasPanel::ResetClip()
		{
			if (m_bufferDevice)
			{
				m_bufferDevice->ResetClip();
			}
		}

		static Gdiplus::Color ToColor(const wxColour& color)
		{
			return Gdiplus::Color(color.Alpha(), color.Red(), color.Green(), color.Blue());
		}

		void CanvasPanel::Clear(const wxColour& color)
		{
			if (m_bufferDevice)
			{
				m_bufferDevice->Clear(ToColor(color));
			}
		}

		void CanvasPanel::DrawText(const wxPoint& offset, EFontType font, const char* text, const wxColour &color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Font* drawFont = SelectFont(font);
				if (drawFont)
				{
					Gdiplus::SolidBrush textBrush(ToColor(color));
					const wxString tempString(text);
					m_bufferDevice->DrawString(tempString, -1, drawFont, Gdiplus::PointF(offset.x, offset.y), &textBrush);
				}
			}
		}

		void CanvasPanel::DrawText(const wxPoint& offset, EFontType font, const char* text, const wxColour &color, EVerticalAlign vAlign, EHorizontalAlign hAlign)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Font* drawFont = SelectFont(font);
				if (drawFont)
				{
					wxPoint alignedOffset = offset;
					wxPoint stringSize = CalcTextExtents(font, text);

					// Calculate vertical position
					if (vAlign == eVerticalAlign_Center)
					{
						alignedOffset.y -= stringSize.y / 2;
					}
					else if (vAlign == eVerticalAlign_Bottom)
					{
						alignedOffset.y -= stringSize.y;
					}

					// Calculate horizontal position
					if (hAlign == eHorizontalAlign_Center)
					{
						alignedOffset.x -= stringSize.x / 2;
					}
					else if (hAlign == eHorizontalAlign_Right)
					{
						alignedOffset.x -= stringSize.x;
					}

					// Draw
					const wxString tempString(text);
					const Gdiplus::SolidBrush textBrush(ToColor(color));
					m_bufferDevice->DrawString(tempString, -1, drawFont, Gdiplus::PointF(alignedOffset.x, alignedOffset.y), &textBrush);
				}
			}
		}

		void CanvasPanel::DrawText(const wxPoint& topLeft, uint32 width, uint32 height, EFontType font, const char* text, const wxColour &color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Font* drawFont = SelectFont(font);
				if (drawFont)
				{
					Gdiplus::SolidBrush textBrush(ToColor(color));
					Gdiplus::RectF textRect(topLeft.x, topLeft.y, width, height);
					wxString tempString(text);
					m_bufferDevice->DrawString(tempString.wc_str(), -1, drawFont, textRect, NULL, &textBrush);
				}
			}
		}

		void CanvasPanel::DrawRect(const wxRect& rect, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawRectangle(&drawPen, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::DrawRect(int x, int y, int w, int h, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawRectangle(&drawPen, x, y, w, h);
			}
		}

		void CanvasPanel::FillRect(const wxRect& rect, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillRectangle(&drawBrush, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::FillRect(int x, int y, int w, int h, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillRectangle(&drawBrush, x, y, w, h);
			}
		}

		void CanvasPanel::FillGradientHorizontalRect(const wxRect& rect, const wxColour& leftColor, const wxColour& rightColor)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Point leftPoint(rect.GetX(), 0);
				Gdiplus::Point rightPoint(rect.GetX() + rect.GetWidth(), 0);
				Gdiplus::LinearGradientBrush linearGradientBrush(leftPoint, rightPoint, ToColor(leftColor), ToColor(rightColor));
				m_bufferDevice->FillRectangle(&linearGradientBrush, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::FillGradientVerticalRect(const wxRect& rect, const wxColour& upColor, const wxColour& downColor)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Point upPoint(0, rect.GetY());
				Gdiplus::Point downPoint(0, rect.GetY());
				Gdiplus::LinearGradientBrush linearGradientBrush(upPoint, downPoint, ToColor(upColor), ToColor(downColor));
				m_bufferDevice->FillRectangle(&linearGradientBrush, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::DrawLine(int x1, int y1, int x2, int y2, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawLine(&drawPen, x1, y1, x2, y2);
			}
		}

		void CanvasPanel::DrawLine(const wxPoint& start, const wxPoint &end, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawLine(&drawPen, start.x, start.y, end.x, end.y);
			}
		}

		void CanvasPanel::DrawLines(const wxPoint* points, const int numPoints, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawLines(&drawPen, (const Gdiplus::Point*) points, numPoints);
			}
		}

		void CanvasPanel::DrawArc(const wxRect &rect, float startAngle, float sweepAngle, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawArc(&drawPen, rect.x, rect.y, rect.width, rect.height, startAngle, sweepAngle);
			}
		}

		void CanvasPanel::DrawCircle(int x, int y, int radius, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawEllipse(&drawPen, x, y, radius, radius);
			}
		}

		void CanvasPanel::DrawCircle(const wxPoint &center, int radius, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawEllipse(&drawPen, center.x, center.y, radius, radius);
			}
		}

		void CanvasPanel::FillCircle(int x, int y, int radius, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillEllipse(&drawBrush, x, y, radius, radius);
			}
		}

		void CanvasPanel::FillCircle(const wxPoint &center, int radius, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillEllipse(&drawBrush, center.x, center.y, radius, radius);
			}
		}

		void CanvasPanel::DrawCircle(int x, int y, int w, int h, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawEllipse(&drawPen, x, y, w, h);
			}
		}

		void CanvasPanel::DrawCircle(const wxRect& rect, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawEllipse(&drawPen, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::FillCircle(int x, int y, int w, int h, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillEllipse(&drawBrush, x, y, w, h);
			}
		}

		void CanvasPanel::FillCircle(const wxRect& rect, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillEllipse(&drawBrush, rect.x, rect.y, rect.width, rect.height);
			}
		}

		void CanvasPanel::DrawCurve(const wxPoint *points, const wxColour& color, float width /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				Gdiplus::Pen drawPen(ToColor(color), width);
				m_bufferDevice->DrawBezier(&drawPen, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y, points[3].x, points[3].y);
			}
		}

		bool CanvasPanel::HitTestCurve(const wxPoint *points, const wxPoint& point, const float range /*=1.0f*/)
		{
			/*const float testPoint( point.x, point.y );

			// Initialize bezier blend weights on first use
			const uint32 numPoints = 20;
			static FVector4 bezierWeights[ numPoints+1 ];
			if ( !bezierWeights[0].x )
			{
				for ( uint32 i=0; i<=numPoints; i++ )
				{
					float t = i / (float )numPoints;
					bezierWeights[i].w = t*t*t;
					bezierWeights[i].z = 3.0f*t*t*(1.0f-t);
					bezierWeights[i].y = 3.0f*t*(1.0f-t)*(1.0f-t);
					bezierWeights[i].x = (1.0f-t)*(1.0f-t)*(1.0f-t);
				}
			}

			// Test bounding box
			int minX = Algorithms::Min( Algorithms::Min( points[0].x, points[1].x ), Algorithms::Min( points[2].x, points[3].x ) ) - range;
			int minY = Algorithms::Min( Algorithms::Min( points[0].y, points[1].y ), Algorithms::Min( points[2].y, points[3].y ) ) - range;
			int maxX = Algorithms::Min( Algorithms::Min( points[0].x, points[1].x ), Algorithms::Min( points[2].x, points[3].x ) ) + range;
			int maxY = Algorithms::Min( Algorithms::Min( points[0].y, points[1].y ), Algorithms::Min( points[2].y, points[3].y ) ) + range;
			if ( point.x >= minX && point.y >= minY && point.x <= maxX && point.y <= maxY )
			{
				// Hit test curve
				FVector2 lastPoint;
				for ( uint32 i=0; i<=numPoints; i++ )
				{
					const FVector4 weights = bezierWeights[i];
					const float px = (points[0].x * weights.x) + (points[1].x * weights.y) + (points[2].x * weights.z) + (points[3].x * weights.w);
					const float py = (points[0].y * weights.x) + (points[1].y * weights.y) + (points[2].y * weights.z) + (points[3].y * weights.w);
					const FVector2 point( px, py );

					// Calculate distance to curve
					if ( i )
					{
						// Project on curve segment
						const FVector2 delta = point - lastPoint;
						const float s = FVector2::dot( lastPoint, delta );
						const float e = FVector2::dot( point, delta );
						const float t = FVector2::dot( testPoint, delta );

						// CAlculate distance to curve segment
						float dist;
						if ( t < s )
						{
							dist = lastPoint.distance( testPoint );
						}
						else if ( t > e )
						{
							dist = point.distance( testPoint );
						}
						else
						{
							const FVector2 projected = lastPoint + delta * (t - s);
							dist = projected.distance( testPoint );
						}

						// Close enough
						if ( dist < range )
						{
							return true;
						}
					}

					// Remember for next pass
					lastPoint = point;
				}
			}*/

			// Not hit
			return false;
		}

		void CanvasPanel::DrawRoundedRect(const wxRect& rect, const wxColour& color, int radius, float borderWidth /*=1.0f*/)
		{
			DrawRoundedRect(rect.x, rect.y, rect.width, rect.height, color, radius, borderWidth);
		}

		void CanvasPanel::DrawRoundedRect(int x, int y, int width, int height, const wxColour& color, int radius, float borderWidth /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x + radius, y, x + width - (radius * 2), y);
				gp.AddArc(x + width - (radius * 2), y, radius * 2, radius * 2, 270, 90);
				gp.AddLine(x + width, y + radius, x + width, y + height - (radius * 2));
				gp.AddArc(x + width - (radius * 2), y + height - (radius * 2), radius * 2, radius * 2, 0, 90);
				gp.AddLine(x + width - (radius * 2), y + height, x + radius, y + height);
				gp.AddArc(x, y + height - (radius * 2), radius * 2, radius * 2, 90, 90);
				gp.AddLine(x, y + height - (radius * 2), x, y + radius);
				gp.AddArc(x, y, radius * 2, radius * 2, 180, 90);
				gp.CloseFigure();

				// Draw path
				Gdiplus::Pen drawPen(ToColor(color), borderWidth);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::FillRoundedRect(const wxRect& rect, const wxColour& color, int radius)
		{
			FillRoundedRect(rect.x, rect.y, rect.width, rect.height, color, radius);
		}

		void CanvasPanel::FillRoundedRect(int x, int y, int width, int height, const wxColour& color, int radius)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x + radius, y, x + width - (radius * 2), y);
				gp.AddArc(x + width - (radius * 2), y, radius * 2, radius * 2, 270, 90);
				gp.AddLine(x + width, y + radius, x + width, y + height - (radius * 2));
				gp.AddArc(x + width - (radius * 2), y + height - (radius * 2), radius * 2, radius * 2, 0, 90);
				gp.AddLine(x + width - (radius * 2), y + height, x + radius, y + height);
				gp.AddArc(x, y + height - (radius * 2), radius * 2, radius * 2, 90, 90);
				gp.AddLine(x, y + height - (radius * 2), x, y + radius);
				gp.AddArc(x, y, radius * 2, radius * 2, 180, 90);
				gp.CloseFigure();

				// Draw path
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillPath(&drawBrush, &gp);
			}
		}

		void CanvasPanel::DrawCuttedRect(const wxRect& rect, const wxColour& color, int radius, float borderWidth /*=1.0f*/)
		{
			DrawCuttedRect(rect.x, rect.y, rect.width, rect.height, color, radius, borderWidth);
		}

		void CanvasPanel::DrawCuttedRect(int x, int y, int width, int height, const wxColour& color, int radius, float borderWidth /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x + radius, y, x + width - radius, y);
				gp.AddLine(x + width - radius, y, x + width, y + radius);
				gp.AddLine(x + width, y + radius, x + width, y + height - radius);
				gp.AddLine(x + width, y + height - radius, x + width - radius, y + height);
				gp.AddLine(x + width - radius, y + height, x + radius, y + height);
				gp.AddLine(x + radius, y + height, x, y + height - radius);
				gp.AddLine(x, y + height - radius, x, y + radius);
				gp.AddLine(x, y + radius, x + radius, y);
				gp.CloseFigure();

				// Draw path
				Gdiplus::Pen drawPen(ToColor(color), borderWidth);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::FillCuttedRect(const wxRect& rect, const wxColour& color, int radius)
		{
			FillCuttedRect(rect.x, rect.y, rect.width, rect.height, color, radius);
		}

		void CanvasPanel::FillCuttedRect(int x, int y, int width, int height, const wxColour& color, int radius)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x + radius, y, x + width - radius, y);
				gp.AddLine(x + width - radius, y, x + width, y + radius);
				gp.AddLine(x + width, y + radius, x + width, y + height - radius);
				gp.AddLine(x + width, y + height - radius, x + width - radius, y + height);
				gp.AddLine(x + width - radius, y + height, x + radius, y + height);
				gp.AddLine(x + radius, y + height, x, y + height - radius);
				gp.AddLine(x, y + height - radius, x, y + radius);
				gp.AddLine(x, y + radius, x + radius, y);
				gp.CloseFigure();

				// Draw path
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillPath(&drawBrush, &gp);
			}
		}

		void CanvasPanel::DrawDownArrow(const wxRect& rect, const wxColour& backColor, const wxColour& borderColor)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(rect.x, rect.y, rect.x + rect.width, rect.y);
				gp.AddLine(rect.x + rect.width, rect.y, rect.x + rect.width / 2, rect.y + rect.height);
				gp.AddLine(rect.x + rect.width / 2, rect.y + rect.height, rect.x, rect.y);
				gp.CloseFigure();

				// Draw path
				Gdiplus::SolidBrush drawBrush(ToColor(backColor));
				m_bufferDevice->FillPath(&drawBrush, &gp);
				Gdiplus::Pen drawPen(ToColor(borderColor), 1);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::DrawUpArrow(const wxRect& rect, const wxColour& backColor, const wxColour& borderColor)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);
				gp.AddLine(rect.x + rect.width, rect.y, rect.x + rect.width / 2, rect.y);
				gp.AddLine(rect.x + rect.width / 2, rect.y + rect.height, rect.x, rect.y + rect.height);
				gp.CloseFigure();

				// Draw path
				Gdiplus::SolidBrush drawBrush(ToColor(backColor));
				m_bufferDevice->FillPath(&drawBrush, &gp);
				Gdiplus::Pen drawPen(ToColor(borderColor), 1);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const wxColour& color, float borderWidth /*=1.0f*/)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x1, y1, x2, y2);
				gp.AddLine(x2, y2, x3, y3);
				gp.AddLine(x3, y3, x1, y1);
				gp.CloseFigure();

				// Draw triangle 
				Gdiplus::Pen drawPen(ToColor(color), borderWidth);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::FillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, const wxColour& color)
		{
			if (m_bufferDevice)
			{
				// Assemble bounds
				Gdiplus::GraphicsPath gp;
				gp.AddLine(x1, y1, x2, y2);
				gp.AddLine(x2, y2, x3, y3);
				gp.AddLine(x3, y3, x1, y1);
				gp.CloseFigure();

				// Draw triangle
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillPath(&drawBrush, &gp);
			}
		}

		void CanvasPanel::DrawTipRect(int x1, int y1, int x2, int y2, const wxColour& borderColor, const wxColour& interiorColor, uint32 size)
		{
			{
				wxPoint tip(x1, y1);
				FillTriangle(tip.x + size, tip.y + size, tip.x - size, tip.y + size, tip.x + size, tip.y - size, interiorColor);
				DrawTriangle(tip.x + size, tip.y + size, tip.x - size, tip.y + size, tip.x + size, tip.y - size, borderColor);
			}

			{
				wxPoint tip(x2, y1);
				FillTriangle(tip.x - size, tip.y + size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, interiorColor);
				DrawTriangle(tip.x - size, tip.y + size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, borderColor);
			}

			{
				wxPoint tip(x1, y2);
				FillTriangle(tip.x + size, tip.y - size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, interiorColor);
				DrawTriangle(tip.x + size, tip.y - size, tip.x + size, tip.y + size, tip.x - size, tip.y - size, borderColor);
			}

			{
				wxPoint tip(x2, y2);
				FillTriangle(tip.x - size, tip.y - size, tip.x + size, tip.y - size, tip.x - size, tip.y + size, interiorColor);
				DrawTriangle(tip.x - size, tip.y - size, tip.x + size, tip.y - size, tip.x - size, tip.y + size, borderColor);
			}
		}

		void CanvasPanel::DrawPoly(const wxPoint *points, uint32 numPoints, const wxColour& borderColor, float borderWidth /*=1.0f*/)
		{
			if (m_bufferDevice && numPoints)
			{
				Gdiplus::GraphicsPath gp;

				// Convert points
				Gdiplus::Point* gdiPoints = (Gdiplus::Point*) _alloca(sizeof(Gdiplus::Point) * numPoints);
				for (uint32 i = 0; i < numPoints; i++)
				{
					gdiPoints[i].X = points[i].x;
					gdiPoints[i].Y = points[i].y;
				}

				// Add polygon
				gp.AddPolygon(gdiPoints, numPoints);
				gp.CloseFigure();

				// Draw path
				Gdiplus::Pen drawPen(ToColor(borderColor), borderWidth);
				m_bufferDevice->DrawPath(&drawPen, &gp);
			}
		}

		void CanvasPanel::FillPoly(const wxPoint *points, uint32 numPoints, const wxColour& color)
		{
			if (m_bufferDevice && numPoints)
			{
				Gdiplus::GraphicsPath gp;

				// Convert points
				Gdiplus::Point* gdiPoints = (Gdiplus::Point*) _alloca(sizeof(Gdiplus::Point) * numPoints);
				for (uint32 i = 0; i < numPoints; i++)
				{
					gdiPoints[i].X = points[i].x;
					gdiPoints[i].Y = points[i].y;
				}

				// Add polygon
				gp.AddPolygon(gdiPoints, numPoints);
				gp.CloseFigure();

				// Draw path
				Gdiplus::SolidBrush drawBrush(ToColor(color));
				m_bufferDevice->FillPath(&drawBrush, &gp);
			}
		}

		void CanvasPanel::OnPaint(wxPaintEvent& event)
		{
			wxPaintDC paintDC(this);

			// Get current size
			int w, h;
			GetClientSize(&w, &h);

			// Recreate device
			RecreateDevice(w, h);

			// Draw buffered
			if (m_bufferDevice)
			{
				// Draw window content to buffered device
				OnPaintCanvas(w, h);

				// Blit to screen
				Gdiplus::Graphics graphics((HDC)paintDC.GetHDC());
				graphics.DrawImage(m_bitmapBuffer, 0, 0, w, h);
			}
		}

		void CanvasPanel::OnEraseBackground(wxEraseEvent& event)
		{
			// Do not redraw background
		}

		void CanvasPanel::OnSize(wxSizeEvent& event)
		{
			if (m_bitmapBuffer)
			{
				const uint32 newWidth = event.GetSize().x;
				const uint32 newHeight = event.GetSize().x;
				if (newWidth != m_bitmapBuffer->GetWidth() || newHeight != m_bitmapBuffer->GetHeight())
				{
					// Repaint whole window after sizing
					Refresh(false);
				}
			}
		}

		void CanvasPanel::OnMouseEvent(wxMouseEvent& event)
		{
			if (!event.RightUp() && !event.LeftUp())
			{
				// Always focus the canvas window when clicked, needed for mouse wheel to work
				// Focus shouldn't be set, when button is released - this returns focus to the
				// window, from which we may have just left
				SetFocus();
			}

			// Process event
			OnMouseClick(event);
		}

		void CanvasPanel::OnMouseMove(wxMouseEvent& event)
		{
			// Not captured, no delta
			if (m_captureMode == eMouseCapture_None)
			{
				OnMouseTrack(event);
				return;
			}

			// Calculate delta
			POINT curPos;
			::GetCursorPos(&curPos);
			wxPoint delta(0, 0);
			delta.x = curPos.x - m_preCapturePosition.x;
			delta.y = curPos.y - m_preCapturePosition.y;

			// Process cursor position
			if (m_captureMode == eMouseCapture_Capture)
			{
				// Move cursor back so we can do large moves
				::SetCursorPos(m_preCapturePosition.x, m_preCapturePosition.y);
			}
			else
			{
				// Remember so we have relative move
				m_preCapturePosition.x = curPos.x;
				m_preCapturePosition.y = curPos.y;
			}

			// Call event
			OnMouseMove(event, delta);
		}

		void CanvasPanel::UpdateTransformMatrix()
		{
			if (m_bufferDevice)
			{
				Gdiplus::Matrix trans(m_scale, 0.0f, 0.0f, m_scale, m_offsetX * m_scale, m_offsetY * m_scale);
				m_bufferDevice->SetTransform(&trans);
			}
		}

		void CanvasPanel::RecreateDevice(uint32 newWidth, uint32 newHeight)
		{
			// Process only if there is something to do
			if (!m_bitmapBuffer || newWidth != m_bitmapBuffer->GetWidth() || newHeight != m_bitmapBuffer->GetHeight())
			{
				// Destroy old buffered device
				if (m_bufferDevice)
				{
					delete m_bufferDevice;
					m_bufferDevice = NULL;
				}

				// Destroy old buffer bitmap
				if (m_bitmapBuffer)
				{
					delete m_bitmapBuffer;
					m_bitmapBuffer = NULL;
				}

				// Create new GDI objects
				m_bitmapBuffer = new Gdiplus::Bitmap(newWidth, newHeight, PixelFormat32bppARGB);
				m_bufferDevice = new Gdiplus::Graphics(m_bitmapBuffer);

				// Restore transformation matrix
				UpdateTransformMatrix();

				// Update quality mode
				ToggleQualityMode(true);
			}
		}

		void CanvasPanel::ToggleQualityMode(bool hiQuality)
		{
			if (m_bufferDevice)
			{
				if (hiQuality)
				{
					// Setup rendering options, highest quality
					m_bufferDevice->SetInterpolationMode(Gdiplus::InterpolationModeDefault);
					m_bufferDevice->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
					m_bufferDevice->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
					m_bufferDevice->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
					m_bufferDevice->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
				}
				else
				{
					// Setup rendering options, fastest quality
					m_bufferDevice->SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
					m_bufferDevice->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
					m_bufferDevice->SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault);
					m_bufferDevice->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
					m_bufferDevice->SetSmoothingMode(Gdiplus::SmoothingModeNone);
				}
			}
		}

		Gdiplus::Font* CanvasPanel::SelectFont(EFontType font)
		{
			static Gdiplus::Font* drawFont = NULL;
			static Gdiplus::Font* boldFont = NULL;

			// Bond font
			if (font == eFontType_Bold)
			{
				// Create font
				if (!boldFont)
				{
					// Create font
					wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
					font.SetWeight(wxFONTWEIGHT_BOLD);
					boldFont = new Gdiplus::Font(::GetDC(NULL), (HFONT)font.GetHFONT());
				}

				// Return the font
				return boldFont;
			}

			// Normal font
			if (font == eFontType_Normal)
			{
				// Create font
				if (!drawFont)
				{
					// Create font
					wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
					drawFont = new Gdiplus::Font(::GetDC(NULL), (HFONT)font.GetHFONT());
				}

				// Return the font
				return drawFont;
			}

			// No font
			return NULL;
		}

		void CanvasPanel::OnPaintCanvas(int width, int height)
		{
		}

		void CanvasPanel::OnMouseClick(wxMouseEvent& event)
		{

		}

		void CanvasPanel::OnMouseMove(wxMouseEvent& event, wxPoint delta)
		{

		}

		void CanvasPanel::OnMouseTrack(wxMouseEvent& event)
		{

		}

	} // canvas
} // tools