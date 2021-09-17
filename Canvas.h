#pragma once
#include <vector>



class Canvas {
public:
	Canvas():W(0),H(0) {};
	Canvas(int W, int H) :W(W), H(H) {
		pixels.resize(W*H * 3);
		erase();
	};
	void erase() {
		memset(&pixels[0], 255, W*H*3 * sizeof(pixels[0]));
	}
	unsigned char& operator()(int x, int y, int c) { return pixels[y*W * 3 + x * 3 + c]; }
	unsigned char operator()(int x, int y, int c) const { return pixels[y*W * 3 + x * 3 + c]; }

	void saveToFile(const char* filename);

	int W, H;
	std::vector<unsigned char> pixels;
};