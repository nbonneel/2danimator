#include "Canvas.h"
#include <string>
#include <sstream>
#include <algorithm>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Canvas::saveToFile(const char* filename) {

	removeAlpha();

	std::string s(filename), ls(filename);
	std::transform(s.begin(), s.end(), ls.begin(), ::tolower);

	if (ls.find(".bmp") != std::string::npos) {
		stbi_write_bmp(filename, W, H, 3, &imgNoAlpha[0]);
		return;
	}
	if (ls.find(".tga") != std::string::npos) {
		stbi_write_tga(filename, W, H, 3, &imgNoAlpha[0]);
		return;
	}
	if (ls.find(".jpg") != std::string::npos) {
		stbi_write_jpg(filename, W, H, 3, &imgNoAlpha[0], 100);
		return;
	}
	if (ls.find(".png") != std::string::npos) {
		stbi_write_png(filename, W, H, 3, &imgNoAlpha[0], 0);
		return;
	}
}