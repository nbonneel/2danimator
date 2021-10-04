#pragma once
#include <vector>
#include "cairo-features.h"
#include "cairo.h"
#include "cairo-win32.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class Canvas {
public:
	Canvas():W(0),H(0) {};
	Canvas(int W, int H) :W(W), H(H) {
		pixels.resize(W*H * 4);
		imgNoAlpha.resize(W*H * 3);
		erase();

		//surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);

		int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, W);

		surface = cairo_image_surface_create_for_data(&pixels[0], CAIRO_FORMAT_ARGB32,
			W, H,
			stride);

		cr = cairo_create(surface);
	};
	void erase() {
		memset(&pixels[0], 255, W*H*4 * sizeof(pixels[0]));
		memset(&imgNoAlpha[0], 255, W*H * 3 * sizeof(pixels[0]));
	}
	unsigned char& operator()(int x, int y, int c) { return pixels[y*W * 4 + x * 4 + c+0]; }
	unsigned char operator()(int x, int y, int c) const { return pixels[y*W * 4 + x * 4 + c+0]; }
	void removeAlpha() {
		for (int i = 0; i < W*H; i++) {
			imgNoAlpha[i * 3] = pixels[i * 4 + 0];
			imgNoAlpha[i * 3 + 1] = pixels[i * 4 + 1];
			imgNoAlpha[i * 3 + 2] = pixels[i * 4 + 2];
		}
	}

	void saveToFile(const char* filename);

	int W, H;
	std::vector<unsigned char> pixels;
	std::vector<unsigned char> imgNoAlpha;
	cairo_surface_t *surface;
	cairo_t *cr;
};