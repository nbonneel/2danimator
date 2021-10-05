
#include "TextToLines.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <freetype/ftoutln.h>
#include <freetype/ftbbox.h>

inline
FreeTypeLibrary::FreeTypeLibrary() {
	FT_Error error = FT_Init_FreeType(&m_ftLibrary);

	if (error)
		throw std::runtime_error("Couldn't initialize the library:"
			" FT_Init_FreeType() failed");
}


inline
FreeTypeLibrary::~FreeTypeLibrary() {
	FT_Done_FreeType(m_ftLibrary);
}


inline
FreeTypeLibrary::operator FT_Library() const {
	return m_ftLibrary;
}


inline
FreeTypeFace::FreeTypeFace(const FreeTypeLibrary &library,
	const char *filename) {
	// For simplicity, always use the first face index.
	FT_Error error = FT_New_Face(library, filename, 0, &m_ftFace);

	if (error)
		throw std::runtime_error("Couldn't load the font file:"
			" FT_New_Face() failed");
}


inline
FreeTypeFace::~FreeTypeFace() {
	FT_Done_Face(m_ftFace);
}


inline
FreeTypeFace::operator FT_Face() const {
	return m_ftFace;
}


inline
OutlinePrinter::OutlinePrinter(const char *filename)
	: m_face(m_library, filename),
	m_xMin(0),
	m_yMin(0),
	m_width(0),
	m_height(0) {
	// Empty body.
}


int
OutlinePrinter::Run(const char *symbol) {
	LoadGlyph(symbol);

	// Check whether outline exists.
	bool outlineExists = OutlineExists();

	if (!outlineExists) // Outline doesn't exist.
		throw std::runtime_error("Outline check failed.\n"
			"Please, inspect your font file or try another one,"
			" for example LiberationSerif-Bold.ttf");

	FlipOutline();

	ExtractOutline();

	ComputeViewBox();

	PrintSVG();

	return 0;
}

void
OutlinePrinter::LoadGlyph(const char *symbol) const {
	FT_ULong code = symbol[0];

	// For simplicity, use the charmap FreeType provides by default;
	// in most cases this means Unicode.
	FT_UInt index = FT_Get_Char_Index(m_face, code);

	FT_Error error = FT_Load_Glyph(m_face,
		index,
		FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);

	if (error)
		throw std::runtime_error("Couldn't load the glyph: FT_Load_Glyph() failed");
}


// While working on this example, we found fonts with no outlines for
// printable characters such as `A', i.e., `outline.n_contours' and
// `outline.n_points' were zero.  FT_Outline_Check() returned `true'.
// FT_Outline_Decompose() also returned `true' without walking the outline.
// That is, we had no way of knowing whether the outline existed and could
// be (or was) decomposed.  Therefore, we implemented this workaround to
// check whether the outline does exist and can be decomposed.
bool
OutlinePrinter::OutlineExists() const {
	FT_Face face = m_face;
	FT_GlyphSlot slot = face->glyph;
	FT_Outline &outline = slot->outline;

	if (slot->format != FT_GLYPH_FORMAT_OUTLINE)
		return false; // Should never happen.  Just an extra check.

	if (outline.n_contours <= 0 || outline.n_points <= 0)
		return false; // Can happen for some font files.

	FT_Error error = FT_Outline_Check(&outline);

	return error == 0;
}


// This function flips outline around x-axis. We need it because
// FreeType and SVG use opposite vertical directions.
void
OutlinePrinter::FlipOutline() const {
	const FT_Fixed multiplier = 65536L;

	FT_Matrix matrix;

	matrix.xx = 1L * multiplier;
	matrix.xy = 0L * multiplier;
	matrix.yx = 0L * multiplier;
	matrix.yy = -1L * multiplier;

	FT_Face face = m_face;
	FT_GlyphSlot slot = face->glyph;
	FT_Outline &outline = slot->outline;

	FT_Outline_Transform(&outline, &matrix);
}


void
OutlinePrinter::ExtractOutline() {
	m_path << "  <path d='\n";

	FT_Outline_Funcs callbacks;

	callbacks.move_to = MoveToFunction;
	callbacks.line_to = LineToFunction;
	callbacks.conic_to = ConicToFunction;
	callbacks.cubic_to = CubicToFunction;

	callbacks.shift = 0;
	callbacks.delta = 0;

	FT_Face face = m_face;
	FT_GlyphSlot slot = face->glyph;
	FT_Outline &outline = slot->outline;

	FT_Error error = FT_Outline_Decompose(&outline, &callbacks, this);

	if (error)
		throw std::runtime_error("Couldn't extract the outline:"
			" FT_Outline_Decompose() failed");

	m_path << "          '\n"
		"        fill='red'/>\n";
}


void
OutlinePrinter::ComputeViewBox() {
	FT_Face face = m_face;
	FT_GlyphSlot slot = face->glyph;
	FT_Outline &outline = slot->outline;

	FT_BBox boundingBox;

	FT_Outline_Get_BBox(&outline, &boundingBox);

	FT_Pos xMin = boundingBox.xMin;
	FT_Pos yMin = boundingBox.yMin;
	FT_Pos xMax = boundingBox.xMax;
	FT_Pos yMax = boundingBox.yMax;

	m_xMin = xMin;
	m_yMin = yMin;
	m_width = xMax - xMin;
	m_height = yMax - yMin;
}


void
OutlinePrinter::PrintSVG() const {
	std::cout << "<svg xmlns='http://www.w3.org/2000/svg'\n"
		"     xmlns:xlink='http://www.w3.org/1999/xlink'\n"
		"     viewBox='"
		<< m_xMin << ' ' << m_yMin << ' ' << m_width << ' ' << m_height
		<< "'>\n"
		<< m_path.str()
		<< "</svg>"
		<< std::endl;
}


int
OutlinePrinter::MoveToFunction(const FT_Vector *to,
	void *user) {
	OutlinePrinter *self = static_cast<OutlinePrinter *>(user);

	FT_Pos x = to->x;
	FT_Pos y = to->y;

	self->m_path << "           "
		"M " << x << ' ' << y << '\n';

	return 0;
}


int
OutlinePrinter::LineToFunction(const FT_Vector *to,
	void *user) {
	OutlinePrinter *self = static_cast<OutlinePrinter *>(user);

	FT_Pos x = to->x;
	FT_Pos y = to->y;

	self->m_path << "           "
		"L " << x << ' ' << y << '\n';

	return 0;
}


int
OutlinePrinter::ConicToFunction(const FT_Vector *control,
	const FT_Vector *to,
	void *user) {
	OutlinePrinter *self = static_cast<OutlinePrinter *>(user);

	FT_Pos controlX = control->x;
	FT_Pos controlY = control->y;

	FT_Pos x = to->x;
	FT_Pos y = to->y;

	self->m_path << "           "
		"Q " << controlX << ' ' << controlY << ", "
		<< x << ' ' << y << '\n';

	return 0;
}


int
OutlinePrinter::CubicToFunction(const FT_Vector *controlOne,
	const FT_Vector *controlTwo,
	const FT_Vector *to,
	void *user) {
	OutlinePrinter *self = static_cast<OutlinePrinter *>(user);

	FT_Pos controlOneX = controlOne->x;
	FT_Pos controlOneY = controlOne->y;

	FT_Pos controlTwoX = controlTwo->x;
	FT_Pos controlTwoY = controlTwo->y;

	FT_Pos x = to->x;
	FT_Pos y = to->y;

	self->m_path << "           "
		"C " << controlOneX << ' ' << controlOneY << ", "
		<< controlTwoX << ' ' << controlTwoY << ", "
		<< x << ' ' << y << '\n';

	return 0;
}

void TextToLine::convert(std::vector<float>& coords, std::vector<int> &contours) {
	OutlinePrinter printer("C:\\Windows\\Fonts\\arialbd.ttf");
	//OutlinePrinter printer("Arial Bold.ttf");
	//char car = 'A' + rand() % 26;
	int prevContour = 0;
	float prevW = 0;
	coords.reserve(100 * 2);
	contours.reserve(100);
	float minX = 1E9, maxX = -1E9, minY = 1E9, maxY = -1E9;
	for (int cid = 0; cid < text.size(); cid++) {
		if (text[cid] == ' ') {
			prevW += printer.m_width*1.1;
			continue;
		}
		printer.Run(&text[cid]);
		FT_Face face = printer.m_face;
		FT_GlyphSlot slot = face->glyph;
		FT_Outline &outline = slot->outline;
		printer.ComputeViewBox();
		int curContour = 0;
		for (int i = 0; i < outline.n_points; i++) {
			coords.push_back(outline.points[i].x-printer.m_xMin+ prevW);
			coords.push_back(outline.points[i].y);
			minX = std::min(minX, outline.points[i].x - printer.m_xMin + prevW);
			maxX = std::max(maxX, outline.points[i].x - printer.m_xMin + prevW);
			minY = std::min(minY, (float)outline.points[i].y);
			maxY = std::max(maxY, (float)outline.points[i].y);
		}
		prevW += printer.m_width*1.1;
		for (int i = 0; i < outline.n_contours; i++) {
			contours.push_back(outline.contours[i]+ prevContour);
		}
		prevContour = contours[contours.size() - 1]+1;
	}
	float w = maxX - minX;
	float scale = 200. / w;
	for (int i = 0; i < coords.size() / 2; i++) {
		coords[i * 2] = coords[i * 2] * scale - 100;
		coords[i * 2+1] = coords[i * 2+1] * scale;
	}
}

void my_draw_bitmap(Canvas& canvas, FT_Bitmap* bitmap, int x, int y, unsigned char* color) {
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;


	/* for simplicity, we assume that `bitmap->pixel_mode' */
	/* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

	for (i = x, p = 0; i < x_max; i++, p++) {
		for (j = y, q = 0; j < y_max; j++, q++) {
			if (i < 0 || j < 0 ||
				i >= canvas.W || j >= canvas.H)
				continue;

			float v = bitmap->buffer[q * bitmap->width + p] / 255.;
			canvas(i,j, 0) = canvas(i, j, 0)*(1-v) + v*color[0];
			canvas(i, j, 1) = canvas(i, j, 1)*(1 - v) + v * color[1];
			canvas(i, j, 2) = canvas(i, j, 2)*(1 - v) +  v * color[2];
		}
	}
}

void TextToLine::drawText(const std::string &text, Canvas &canvas, unsigned char* color, int x, int y, int charsize) {

	FT_GlyphSlot  slot;
	FT_Matrix     matrix;              /* transformation matrix */
	FT_UInt       glyph_index;
	FT_Vector     pen;                 /* untransformed origin */
	int           n;
	
	int my_target_height = charsize;

	FreeTypeLibrary library;
	FreeTypeFace facetype(library, "C:\\Windows\\Fonts\\arialbd.ttf");

	FT_Set_Char_Size(
		facetype,    /* handle to face object           */
		0,       /* char_width in 1/64th of points  */
		charsize * 64,   /* char_height in 1/64th of points */
		300,     /* horizontal device resolution    */
		300);   /* vertical device resolution      */

	FT_Face face = facetype;
	slot = face->glyph;                /* a small shortcut */

	float angle = 0;
		/* set up matrix */
	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

	/* the pen position in 26.6 cartesian space coordinates */
	/* start at (300,200)                                   */
	pen.x = x * 64;
	pen.y = (my_target_height - y) * 64;

	for (n = 0; n < text.size(); n++) {
		/* set transformation */
		FT_Set_Transform(face, &matrix, &pen);

		/* load glyph image into the slot (erase previous one) */
		FT_Error error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
		if (error)
			continue;  /* ignore errors */

		  /* now, draw to our target surface (convert position) */
		my_draw_bitmap(canvas, &slot->bitmap,
			slot->bitmap_left,
			my_target_height - slot->bitmap_top, color);

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

}
/*
int
main(int argc,
	char **argv) {
	if (argc != 3) {
		const char *program = argv[0];

		std::cerr << "This program prints a single character's outline"
			" in the SVG format to stdout.\n"
			"Usage: " << program << " font symbol\n"
			"Example: " << program << " LiberationSerif-Bold.ttf A" << std::endl;

		return 1;
	}

	const char *symbol = argv[2];

	// For simplicity, only accept single-byte characters like `A'.
	if (strlen(symbol) != 1 || isspace(*symbol)) {
		std::cerr << "Error: '" << symbol
			<< "' is not a single printable character" << std::endl;

		return 2;
	}

	int status;

	try {
		const char *filename = argv[1];

		OutlinePrinter printer(filename);

		status = printer.Run(symbol);
	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;

		status = 3;
	}

	return status;
}*/