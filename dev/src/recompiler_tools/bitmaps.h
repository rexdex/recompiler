#pragma once

namespace tools
{

	enum EBitmapEffect
	{
		BE_Grayed = 1 << 1,
		BE_HalfOpaque = 1 << 2,
		BE_TintRed = 1 << 3,
		BE_TintGreen = 1 << 4,
		BE_RedCross = 1 << 5,
		BE_Bullet_Error = 1 << 6,
		BE_Bullet_Modified = 1 << 7,
		BE_Bullet_Add = 1 << 8,
		BE_Bullet_Star = 1 << 9,
		BE_Bullet_Import = 1 << 10,
	};

	// load bitmap and icons
	extern wxBitmap LoadWXBitmap(const char* name, unsigned int flag = 0);
	extern wxBitmap LoadWXIcon(const char* name, unsigned int flags = 0);

} // tools