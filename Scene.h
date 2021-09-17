#pragma once

#include "Shape.h"

class Scene {
public:
	Scene() {
		currentShape = NULL;
		currentTime = 0.f;
	};

	void Draw(Canvas &canvas, bool displayControls) const {
		canvas.erase();
		for (int i = 0; i < shapes.size(); i++) {
			shapes[i]->Draw(canvas, currentTime, displayControls && shapes[i]== currentShape);
		}
	}

	int SelectedShape(const Vec2f &coord) {
		int selected = -1;
		for (int i = 0; i < shapes.size(); i++) {
			if (shapes[i]->Contains(coord, currentTime)) {
				selected = i;
			}
		}
		return selected;
	}

	void Add(Shape* sh) {
		shapes.push_back(sh);
	}

	std::vector<Shape*> shapes;
	Shape *currentShape, *previousShape;
	Vec2f oldPos;
	float currentTime;
};