#include "Canvas.h"
#include <string>
#include <sstream>
#include <algorithm>
#include "animator.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


extern AnimatorApp* myApp;

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

void Canvas::erase() {
	//memset(&pixels[0], 255, W*H*4 * sizeof(pixels[0]));
	if (!myApp || !myApp->animatorPanel || !myApp->animatorPanel->scene || !myApp->animatorPanel->scene->bgColor) {
		memset(&pixels[0], 255, W*H * 4 * sizeof(pixels[0]));
		return;
	}
	Vec3u col = myApp->animatorPanel->scene->bgColor->getDisplayValue(myApp->animatorPanel->scene->currentTime);
	for (int i = 0; i < W*H; i++) {
		for (int j = 0; j < 3; j++) {
			pixels[i * 4 + j] = col[j];
		}
		pixels[i * 4 + 3] = 255;
	}
	//memset(&imgNoAlpha[0], 255, W*H * 3 * sizeof(pixels[0]));	
}