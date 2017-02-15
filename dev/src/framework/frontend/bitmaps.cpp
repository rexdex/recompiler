#include "build.h"

class CBitmapCache
{
private:
	struct Entry
	{
		wxString		m_name;
		unsigned int	m_flags;
		wxBitmap*		m_bitmap;

		Entry( const wxString& name, unsigned int flags )
			: m_name( name )
			, m_flags( flags )
		{
			m_bitmap = new wxBitmap( 16, 16, 32 );
		}

		Entry( const wxString& name, unsigned int flags, const wxImage& image )
			: m_name( name )
			, m_flags( flags )
		{
			m_bitmap = new wxBitmap( image, 32 );
		}

		~Entry()
		{
			delete m_bitmap;
		}
	};
		
private:
	wxString				m_basePath;
	std::vector< Entry* >	m_entries;

public:
	CBitmapCache()
	{
		// use the editor path for images
		m_basePath = wxTheApp->GetEditorPath();
	}

	~CBitmapCache()
	{
		for ( size_t i=0; i<m_entries.size(); ++i )
		{
			delete m_entries[i];
		}
	}

	static inline unsigned char BlendUint8( unsigned char src, unsigned char dest, unsigned char alpha )
	{
		unsigned int x = ((unsigned int)src * (alpha)) + ((unsigned int)dest * (255-alpha) );
		return x / 255;
	}

	void MergeBitmap( unsigned int maxX, unsigned int maxY, unsigned char* colors, unsigned char* alphas, const wxString& iconName )
	{
		wxImage iconImage;
		wxString filePath = m_basePath + iconName + wxT(".png");
		iconImage.LoadFile( filePath, wxBITMAP_TYPE_PNG );
		if ( iconImage.IsOk() )
		{
			if ( !iconImage.HasAlpha() )
			{
				iconImage.InitAlpha();
			}

			const unsigned int sizeX = iconImage.GetWidth();
			const unsigned int sizeY = iconImage.GetHeight();
			const unsigned int offsetX = maxX - sizeX;
			const unsigned int offsetY = maxY - sizeY;
			for ( unsigned int y=0; y<sizeY; ++y )
			{
				unsigned char* srcColors = &colors[ (offsetX + (offsetY+y)*maxX)* 3];
				unsigned char* srcAlphas = &alphas[ (offsetX + (offsetY+y)*maxX)* 1];
				for ( unsigned int x=0; x<sizeX; ++x, srcColors += 3, srcAlphas += 1 )
				{
					const unsigned char iconR = iconImage.GetRed(x,y);
					const unsigned char iconG = iconImage.GetGreen(x,y);
					const unsigned char iconB = iconImage.GetBlue(x,y);
					const unsigned char iconA = iconImage.GetAlpha(x,y);
					if ( iconA > 0 )
					{
						srcColors[0] = BlendUint8( iconR, srcColors[0], iconA );
						srcColors[1] = BlendUint8( iconG, srcColors[1], iconA );
						srcColors[2] = BlendUint8( iconB, srcColors[2], iconA );
						srcAlphas[0] = BlendUint8( iconA, srcAlphas[0], iconA );
					}
				}
			}
		}
	}

	const wxBitmap* LoadBitmap( const wxString& name, unsigned int flags )
	{
		// Get existing bitmap
		for ( unsigned int i=0; i<m_entries.size(); ++i )
		{
			Entry* entry = m_entries[i];
			if ( 0 == wxStricmp( entry->m_name, name ) )
			{
				if ( entry->m_flags == flags )
				{
					return entry->m_bitmap;
				}
			}
		}

		// Load bitmap
		wxImage loadedBitmap;
		wxString filePath = m_basePath + name + wxT(".png");
		loadedBitmap.LoadFile( filePath, wxBITMAP_TYPE_PNG );
		if ( !loadedBitmap.IsOk() )
		{
			// Use empty bitmap
			Entry* fakeEntry = new Entry( name, flags );
			m_entries.push_back( fakeEntry );
			return fakeEntry->m_bitmap;
		}

		// image::Binary with no alpha
		if ( !loadedBitmap.HasAlpha() )
		{
			loadedBitmap.InitAlpha();
		}

		// Apply some extra filters
		if ( flags != 0 )
		{
			// Get current image size
			const unsigned int maxX = loadedBitmap.GetWidth();
			const unsigned int maxY = loadedBitmap.GetHeight();

			// Create image buffers
			unsigned char* colors = (unsigned char*) malloc( maxX*maxY*3 );
			memset( colors, 255, maxX*maxY*3 );
			unsigned char* alphas = (unsigned char*) malloc( maxX*maxY*1 );
			memset( alphas, 255, maxX*maxY*1 );

			// Copy current image
			{
				unsigned char* writeColors = colors;
				unsigned char* writeAlpha = alphas;
				for ( unsigned int y=0; y<maxY; ++y )
				{
					for ( unsigned int x=0; x<maxX; ++x, writeColors += 3, writeAlpha += 1 )
					{
						writeColors[0] = loadedBitmap.GetRed(x,y);
						writeColors[1] = loadedBitmap.GetGreen(x,y);
						writeColors[2] = loadedBitmap.GetBlue(x,y);

						if ( loadedBitmap.HasAlpha() )
						{
							writeAlpha[0] = loadedBitmap.GetAlpha(x,y);
						}
					}
				}
			}

			// Make image half transparent
			if ( flags & BE_HalfOpaque )
			{
				unsigned char* writeColors = colors;
				unsigned char* writeAlpha = alphas;
				for ( unsigned int y=0; y<maxY; ++y )
				{
					for ( unsigned int x=0; x<maxX; ++x, writeColors += 3, writeAlpha += 1 )
					{
						writeAlpha[0] = writeAlpha[0] / 2;
					}
				}
			}

			// Final filter - gray it out
			if ( flags & BE_Grayed )
			{
				unsigned char* writeColors = colors;
				unsigned char* writeAlpha = alphas;
				for ( unsigned int y=0; y<maxY; ++y )
				{
					for ( unsigned int x=0; x<maxX; ++x, writeColors += 3, writeAlpha += 1 )
					{
						const unsigned char g = writeColors[1];
						writeColors[0] = g;
						writeColors[1] = g;
						writeColors[2] = g;
					}
				}
			}

			// Tint it red
			if ( flags & BE_TintRed )
			{
				unsigned char* writeColors = colors;
				unsigned char* writeAlpha = alphas;
				for ( unsigned int y=0; y<maxY; ++y )
				{
					for ( unsigned int x=0; x<maxX; ++x, writeColors += 3, writeAlpha += 1 )
					{
						writeColors[0] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[0] * 5 / 4), 0, 255 );
						writeColors[1] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[1] * 3 / 4), 0, 255 );
						writeColors[2] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[2] * 3 / 4), 0, 255 );
					}
				}
			}

			// Tint it green
			if ( flags & BE_TintGreen )
			{
				unsigned char* writeColors = colors;
				unsigned char* writeAlpha = alphas;
				for ( unsigned int y=0; y<maxY; ++y )
				{
					for ( unsigned int x=0; x<maxX; ++x, writeColors += 3, writeAlpha += 1 )
					{
						writeColors[0] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[0] * 3 / 4), 0, 255 );
						writeColors[1] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[1] * 5 / 4), 0, 255 );
						writeColors[2] = (unsigned char) TemplateClamp< unsigned int >( ((unsigned int)writeColors[2] * 3 / 4), 0, 255 );
					}
				}
			}

			// "Red cross"
			if ( flags & BE_RedCross )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\cross" );
			}

			// "Add" bullet
			if ( flags & BE_Bullet_Add )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\bullet_add" );
			}

			// "Modified" bullet
			if ( flags & BE_Bullet_Modified )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\bullet_error" );
			}

			// "Error" bullet
			if ( flags & BE_Bullet_Error )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\bullet_red" );
			}

			// "Star" bullet
			if ( flags & BE_Bullet_Star )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\bullet_star" );
			}

			// "Imported" bullet
			if ( flags & BE_Bullet_Import )
			{
				MergeBitmap( maxX, maxY, colors, alphas, "icons\\bullet_import" );
			}

			// Create new image
			loadedBitmap.Create( wxSize( maxX, maxY ), colors, alphas, true );

			// Store in cache
			Entry* newEntry = new Entry( name, flags, loadedBitmap );
			m_entries.push_back( newEntry );

			// Cleanup memory
			free( colors );
			free( alphas );

			// Return created bitmap
			return newEntry->m_bitmap;
		}
		else
		{
			// Store in cache
			Entry* newEntry = new Entry( name, flags, loadedBitmap );
			m_entries.push_back( newEntry );
			return newEntry->m_bitmap;
		}
	}

	static CBitmapCache& GetInstance()
	{
		static CBitmapCache TheInstance;
		return TheInstance;
	}
};

wxBitmap LoadWXBitmap( const char* name, unsigned int flags/*=0*/ )
{
	return *CBitmapCache::GetInstance().LoadBitmap( name, flags );
}

wxBitmap LoadWXIcon( const char* name, unsigned int flags/*=0*/ )
{
	const wxString fullName = wxString( wxT("icons\\") ) + name;
	return *CBitmapCache::GetInstance().LoadBitmap( fullName, flags );
}