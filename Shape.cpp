#include "Shape.h"
#include "animator.h"
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"


void PolygonShape::BSplineSubdivide(float time) {
	float s = GetScale(time);
	float a = angle.getDisplayValue(time);
	Vec2f trans = GetPosition(time);
	VerticesList verticesEval = vertices.getDisplayValue();

	VerticesList newList;
	newList.Clear();
	for (int ct = 0; ct < verticesEval.contourList.size(); ct++) {

		int N;
		int startV, endV = verticesEval.contourList[ct];
		if (ct == 0) {
			startV = 0;
		} else {
			startV = verticesEval.contourList[ct - 1] + 1;
		}
		N = endV - startV + 1;

		Spline sp(Vec2s("0", "0"), "1", Vec3u(0, 0, 0));
		sp.controlPoints.addVertex(time, verticesEval.vertices[endV]);
		for (int i = 0; i < N; i++) {
			sp.controlPoints.addVertex(time, verticesEval.vertices[startV + i]);
		}
		sp.controlPoints.addVertex(time, verticesEval.vertices[startV]);
		sp.controlPoints.addVertex(time, verticesEval.vertices[startV + 1]);

		for (int i = 0; i < 2 * (N); i++) {
			float tparam = i / (2.0*N);  // in [0, 1[			
			Vec2f pp = sp.evalBSpline(tparam*(N + 1.) / (N + 3.), sp.controlPoints.getDisplayValue());
			newList.addVertex(pp, i == 0);
		}
	}
	if (newList.contourList.size() >= 2) {
		if (newList.contourList[newList.contourList.size() - 1] == newList.contourList[newList.contourList.size() - 2]) {
			newList.contourList.resize(newList.contourList.size() - 1);
		}
	}
	//vertices.Clear();
	//p->vertices = newList;
	vertices.setDisplayValue(newList);
}


void drawAntiAliasedLine(Canvas& canvas, int x0, int y0, int x1, int y1, const Vec3u& color, float wd, float tanAlpha0, float tanAlpha1) {
	auto setPixelAA = [&canvas, &color](int x, int y, float mixing) -> void {
		if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;
		canvas(x, y, 0) = (unsigned char)(canvas(x, y, 0)*(mixing) + color[0] * (1.f - mixing));
		canvas(x, y, 1) = (unsigned char)(canvas(x, y, 1)*(mixing) + color[1] * (1.f - mixing));
		canvas(x, y, 2) = (unsigned char)(canvas(x, y, 2)*(mixing) + color[2] * (1.f - mixing));
	};
	auto setPixel = [&canvas, &color](int x, int y) -> void {
		if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;
		canvas(x, y, 0) = color[0];
		canvas(x, y, 1) = color[1];
		canvas(x, y, 2) = color[2];
	};
	if (isnan(tanAlpha0)||isinf(tanAlpha0)) tanAlpha0 = 100;
	if (isnan(tanAlpha1) || isinf(tanAlpha1)) tanAlpha1 = 100;
	tanAlpha0 = -tanAlpha0;
	tanAlpha1 = -tanAlpha1;
	//int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
	//float tanAlpha0 = tan(-M_PI + 2 * M_PI*myApp->animatorPanel->scene->currentTime+0.0001);// 1.732; // tan(3.1416 / 2 + 0.5);
	//float tanAlpha1 = tan(-M_PI + 2 * M_PI*myApp->animatorPanel->scene->currentTime+0.0001);// 1.732; // tan(3.1416 / 2 + 0.5);

	long dx = abs(x1 - x0), dy = abs(y1 - y0);

	if (dx > dy) {
		if (x1 < x0) {
			std::swap(x0, x1);
			std::swap(y0, y1);
			std::swap(tanAlpha0, tanAlpha1);
		}

		if (dx == 0) return;
		int incy = (y1 < y0) ? -1 : 1;

		float e = 0;
		float ex = dy / (float)dx;
		float ey = -1.f;
		float realSlope = (float)(y1 - y0) / (float)(x1 - x0);
		float H = wd / 2.*sqrt(dx*dx + dy * dy) / dx;
		if (tanAlpha0 + realSlope == 0) return;
		if (tanAlpha1 + realSlope == 0) return;

		float signTanAlpha0 = tanAlpha0 < 0 ? -1 : 1;
		float offsetX0 = signTanAlpha0 * incy*H / (tanAlpha0 + realSlope);

		float signTanAlpha1 = tanAlpha1 < 0 ? -1 : 1;
		float offsetX1 = signTanAlpha1 * incy*H / (tanAlpha1 + realSlope);

		int y = y0 -  std::abs(offsetX0)*realSlope;
		//std::cout << "realSlope:" << realSlope << "   tanAlpha0:" << tanAlpha0 << "  offsetX0:" << offsetX0 << "   y=" << y << "    H=" << H << std::endl;

		for (int x = x0 - abs(offsetX0); x < x1 + abs(offsetX1); x++) {
			int starti = y - H;
			int endi = y + H;
			if (x - x0 < abs(offsetX0)) {
				if (tanAlpha0 + realSlope > 0)
					starti = (x0 - x) * tanAlpha0 + y0;
				else
					endi = (x0 - x) * tanAlpha0 + y0;
			}
			if (abs(x - x1) < abs(offsetX1)) {
				if (tanAlpha1 + realSlope > 0)
					endi = (x1 - x) * tanAlpha1 + y1;
				else
					starti = (x1 - x) * tanAlpha1 + y1;
			}
			setPixelAA(x, starti, 0.5f + incy * e);
			for (int i = starti + 1; i < endi; i++) {
				setPixel(x, i);
			}
			setPixelAA(x, endi, 0.5f - incy * e);
			e += ex;
			if (e >= 0.5) {
				y += incy;
				e += ey;
			}
			
		}
	} else {
		tanAlpha0 = 1. / tanAlpha0;
		tanAlpha1 = 1. / tanAlpha1;
		if (y1 < y0) {
			std::swap(x0, x1);
			std::swap(y0, y1);
			std::swap(tanAlpha0, tanAlpha1);
		}

		if (dy == 0) return;
		int incx = (x1 < x0) ? -1 : 1;

		float e = 0;
		float ey = dx / (float)dy;
		float ex = -1.f;
		float realSlope = (float)(x1 - x0) / (float)(y1 - y0);
		float H = wd / 2.*sqrt(dx*dx + dy * dy) / dy;
		if (tanAlpha0 + realSlope == 0) return;
		if (tanAlpha1 + realSlope == 0) return;
		float signTanAlpha0 = tanAlpha0 < 0 ? -1 : 1;
		float offsetY0 = signTanAlpha0 * incx*H / (tanAlpha0 + realSlope);

		float signTanAlpha1 = tanAlpha1 < 0 ? -1 : 1;
		float offsetY1 = signTanAlpha1 * incx*H / (tanAlpha1 + realSlope);

		int x = x0 - std::abs(offsetY0)* realSlope;

		for (int y = y0 - abs(offsetY0); y < y1 + abs(offsetY1); y++) {
			int starti = x - H;
			int endi = x + H;
			if (y - y0 < abs(offsetY0)) {
				if (tanAlpha0 + realSlope > 0)
					starti = (y0 - y) * tanAlpha0 + x0;
				else
					endi = (y0 - y) * tanAlpha0 + x0;
			}
			if (abs(y - y1) < abs(offsetY1)) {
				if (tanAlpha1 + realSlope > 0)
					endi = (y1 - y) * tanAlpha1 + x1;
				else
					starti = (y1 - y) * tanAlpha1 + x1;
			}
			setPixelAA(starti, y, 0.5f + incx * e);
			for (int i = starti + 1; i < endi; i++) {
				setPixel(i, y);
			}
			setPixelAA(endi, y, 0.5f - incx * e);
			e += ey;
			if (e >= 0.5) {
				x += incx;
				e += ex;
			}
		}
	}


}


std::ostream& operator<<(std::ostream& os, const Shape* v) {
	os << v->shapeType << std::endl;
	os << *v ;
	return os;
}
std::istream& operator>>(std::istream& is, Shape* &v) {
	std::string type;
	is >> type;
	v = NULL;
	if (type == "Disk") {
		v = new Disk();
	}
	if (type == "PolygonShape") {
		v = new PolygonShape();
	}
	if (type == "PolygonMorph") {
		v = new PolygonMorph();
	}
	if (type == "Spline") {
		v = new Spline();
	}
	if (type == "Plotter1D") {
		v = new Plotter1D();
	}
	if (type == "Grid") {
		v = new Grid();
	}
	if (type == "PointSet") {
		v = new PointSet();
	}
	if (type == "Latex") {
		v = new Latex();
	}
	if (type == "Image") {
		v = new Image();
	}
	if (!v) {
		std::cout << "invalid Shape" << std::endl;
		return is;
	}
	is >> *v;
	return is;
}

