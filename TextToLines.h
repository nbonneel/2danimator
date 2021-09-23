#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include <sstream>
#include <vector>
#include "Canvas.h"

class TextToLine {
public:
	TextToLine(std::string text):text(text) {

	}
	void convert(std::vector<float>& coords, std::vector<int> &contours);

	static void drawText(const std::string &text, Canvas& canvas, unsigned char* color, int x, int y, int charsize);

	std::string text;
};

// A minimal wrapper for RAII (`resource acquisition is initialization').
class FreeTypeLibrary
{
public:
	FreeTypeLibrary();
	~FreeTypeLibrary();

	operator FT_Library () const;

private:
	FreeTypeLibrary(const FreeTypeLibrary &);
	FreeTypeLibrary &operator=(const FreeTypeLibrary &);

private:
	FT_Library m_ftLibrary;
};


// Another minimal wrapper for RAII.
class FreeTypeFace
{
public:
	FreeTypeFace(const FreeTypeLibrary &library,
		const char *filename);
	~FreeTypeFace();

	operator FT_Face() const;

private:
	FreeTypeFace(const FreeTypeFace &);
	FreeTypeFace &operator =(const FreeTypeFace &);

private:
	FT_Face m_ftFace;
};



class OutlinePrinter
{
public:
	OutlinePrinter(const char *filename);
	int Run(const char *symbol);

//private:
	OutlinePrinter(const OutlinePrinter &);
	OutlinePrinter &operator =(const OutlinePrinter &);

//private:
	void LoadGlyph(const char *symbol) const;
	bool OutlineExists() const;
	void FlipOutline() const;
	void ExtractOutline();
	void ComputeViewBox();
	void PrintSVG() const;

	static int MoveToFunction(const FT_Vector *to,
		void *user);
	static int LineToFunction(const FT_Vector *to,
		void *user);
	static int ConicToFunction(const FT_Vector *control,
		const FT_Vector *to,
		void *user);
	static int CubicToFunction(const FT_Vector *controlOne,
		const FT_Vector *controlTwo,
		const FT_Vector *to,
		void *user);

//private:
	// These two lines initialize the library and the face;
	// the order is important!
	FreeTypeLibrary m_library;
	FreeTypeFace m_face;

	std::ostringstream m_path;

	// These four variables are for the `viewBox' attribute.
	FT_Pos m_xMin;
	FT_Pos m_yMin;
	FT_Pos m_width;
	FT_Pos m_height;
};

