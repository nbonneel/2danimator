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

	void Clear() {
		shapes.clear();
		currentShape = NULL;
	}

	friend std::ostream& operator<<(std::ostream& os, const Scene& v) {
		os << v.shapes.size() << std::endl;
		for (int i = 0; i < v.shapes.size(); i++) {			
			os << v.shapes[i] << std::endl;
		}
			
		os << v.currentTime << std::endl;
		return os;
	}
	friend std::istream& operator>>(std::istream& is, Scene& v) {
		int nvalues;
		is >> nvalues;
		v.shapes.resize(nvalues);
		for (int i = 0; i < nvalues; i++) {
			is >> v.shapes[i];
			//v.shapes[i] = Shape::create(is);
		}
		is >> v.currentTime;
		return is;
	}

	std::vector<Shape*> shapes;
	Shape *currentShape, *previousShape;
	Vec2f oldPos;
	float currentTime;
};