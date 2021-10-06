#pragma once

#include "Vector.h"
#include "Canvas.h"
#include "network_simplex_simple.h"
#include "TextToLines.h"
#include "Property.h"
#include <iostream>
#include <ostream>
#include <wx/stdpaths.h>
#include "nanosvg.h"

using namespace lemon;
class Spline;

static const float arrow2[] = {7.59, 0, -2.36, 4.06, 0, 0, -2.36, -4.06};
static int cur_index;



template<typename T>
std::string to_string_with_precision(const T a_value, const int n = 6) {
	std::ostringstream out;
	out.precision(n);
	out << a_value;
	return out.str();
}

static inline void drawBox(Canvas& canvas, int x1, int y1, int x2, int y2) {
	if (y2 < 0) return;
	if (x2 < 0) return;
	if (x1 > canvas.W - 1) return;
	if (y1 > canvas.H - 1) return;

	if (y1 >= 0) {
		for (int i = std::max(0, x1); i < std::min(canvas.W, x2); i++) {
			canvas(i, y1, 0) = 0;
			canvas(i, y1, 1) = 0;
			canvas(i, y1, 2) = 0;
		}
	}
	if (y2 <= canvas.H - 1) {
		for (int i = std::max(0, x1); i < std::min(canvas.W, x2); i++) {
			canvas(i, y2, 0) = 0;
			canvas(i, y2, 1) = 0;
			canvas(i, y2, 2) = 0;
		}
	}
	if (x1 >= 0) {
		for (int i = std::max(0, y1); i < std::min(canvas.H, y2); i++) {
			canvas(x1, i, 0) = 0;
			canvas(x1, i, 1) = 0;
			canvas(x1, i, 2) = 0;
		}
	}
	if (x2 <= canvas.W - 1) {
		for (int i = std::max(0, y1); i < std::min(canvas.H, y2); i++) {
			canvas(x2, i, 0) = 0;
			canvas(x2, i, 1) = 0;
			canvas(x2, i, 2) = 0;
		}
	}
}

static inline void drawSquare(Canvas& canvas, int x, int y, int size) {
	drawBox(canvas, x - size / 2, y - size / 2, x + size / 2, y + size / 2);
}

// https://zingl.github.io/bresenham.html
// https://zingl.github.io/bresenham.c




static inline void drawAntiAliasedLine1old(Canvas& canvas, int x0, int y0, int x1, int y1, const Vec3u& color = Vec3u(0, 0, 0), float wd = 1, bool fill = false) {
	auto setPixelAA = [&canvas, &color](int x, int y, float mixing) -> void {
		if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;
		canvas(x, y, 0) = (unsigned char)(canvas(x, y, 0)*(mixing / 256.) + color[0] * (1.f - mixing / 256.));
		canvas(x, y, 1) = (unsigned char)(canvas(x, y, 1)*(mixing / 256.) + color[1] * (1.f - mixing / 256.));
		canvas(x, y, 2) = (unsigned char)(canvas(x, y, 2)*(mixing / 256.) + color[2] * (1.f - mixing / 256.));
	};
	auto setPixel = [&canvas, &color](int x, int y) -> void {
		if (x<0 || y<0 || x>canvas.W - 1 || y>canvas.H - 1) return;
		canvas(x, y, 0) = color[0];
		canvas(x, y, 1) = color[1];
		canvas(x, y, 2) = color[2];
	};
	int th = wd * 256;
	int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
	long dx = abs(x1 - x0), dy = abs(y1 - y0);
	long err = dx < dy ? dx : dy, e2 = dx < dy ? dy : dx; /* min / max */
#define BKGD (255L<<16) /* max pixel value = background */
	//if (th <= 256 || e2 == 0) return plotLineAA(x0, y0, x1, y1); /* assert */
	e2 = BKGD / (e2 + 2 * err*err*e2 / (4 * e2*e2 + err * err)); /* sqrt approximation */
	th = (th - 256) << 16; dx *= e2; dy *= e2; /* scale values */
	if (dx < dy) { /* steep line */
		x1 = (BKGD + th / 2) / dy; /* start offset */
		err = x1 * dy - th / 2; /* shift error value to offset width */
		for (x0 -= x1 * sx; ; y0 += sy) {
			setPixelAA(x1 = x0, y0, err >> 16); /* aliasing pre-pixel */
			for (e2 = dy - err - th; e2 + dy < BKGD; e2 += dy)
				setPixel(x1 += sx, y0); /* pixel on thick line */
			setPixelAA(x1 + sx, y0, e2 >> 16); /* aliasing post-pixel */
			if (y0 == y1) break;
			err += dx; /* y-step */
			if (err > BKGD) { err -= dy; x0 += sx; } /* x-step */
		}
	} else { /* flat line */
		y1 = (BKGD + th / 2) / dx; /* start offset */
		err = y1 * dx - th / 2; /* shift error value to offset width */
		for (y0 -= y1 * sy; ; x0 += sx) {
			setPixelAA(x0, y1 = y0, err >> 16); /* aliasing pre-pixel */
			for (e2 = dx - err - th; e2 + dx < BKGD; e2 += dx)
				setPixel(x0, y1 += sy); /* pixel on thick line */
			setPixelAA(x0, y1 + sy, e2 >> 16); /* aliasing post-pixel */
			if (x0 == x1) break;
			err += dy; /* x-step */
			if (err > BKGD) { err -= dx; y0 += sy; } /* y-step */
		}
	}
}



static inline void drawAntiAliasedLine1(Canvas& canvas, int x0, int y0, int x1, int y1, const Vec3u& color = Vec3u(0, 0, 0), float wd = 1, bool fill = false) {
	cairo_t *cr = canvas.cr;
	cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
	cairo_set_line_width(cr, wd);
	cairo_move_to(cr, x0, y0);
	cairo_line_to(cr, x1, y1);
	cairo_stroke(cr);
}

void drawAntiAliasedLine(Canvas& canvas, int x0, int y0, int x1, int y1, const Vec3u& color = Vec3u(0, 0, 0), float wd = 1, float tanAlpha0 = 0.0001, float tanAlpha1 = 0.0001);




// border is the normalized outwards half vector with the previous/next edge
/*static inline void drawLineForInsidePolyline(Canvas& canvas, int x0, int y0, int x1, int y1, const FastVec2f& border1dir, const FastVec2f& border2dir, const Vec3u& color = Vec3u(0, 0, 0), float wd = 1) {

	float x0b = x0;
	float y0b = y0;
	float x1b = x1;
	float y1b = y1;
	FastVec2f b1dir(border1dir);
	FastVec2f b2dir(border2dir);
	if (isnan(border1dir[0]) || isnan(border1dir[1]) || isnan(border2dir[0]) || isnan(border2dir[0])) {
		b1dir = FastVec2f(-y1 + y0, -x0 + x1);
		b1dir = b1dir / norm(b1dir);
		b2dir = b1dir;
	}

	if (wd <= 2) {
		drawAntiAliasedLine(canvas, x0b, y0b, x1b, y1b, color, wd);
	} else {
		drawAntiAliasedLine(canvas, x0b, y0b, x1b, y1b, color, 1);
		x0b -= b1dir[0];
		y0b -= b1dir[1];
		x1b -= b2dir[0];
		y1b -= b2dir[1];
		for (int i = 1; i < wd - 1; i++) {
			drawAntiAliasedLine(canvas, x0b, y0b, x1b, y1b, color, 1, true);
			x0b -= b1dir[0];
			y0b -= b1dir[1];
			x1b -= b2dir[0];
			y1b -= b2dir[1];
		}
		drawAntiAliasedLine(canvas, x0b, y0b, x1b, y1b, color, 1);
	}

}*/


class Shape {
public:
	Shape(std::string type = "", bool visible = true) :shapeType(type), visible(visible) {
		parameters.push_back((Property*)&this->visible);
		id = numShapes;
		numShapes++;
	};
	Shape(const Shape& b) {
		visible = b.visible;
		shapeType = b.shapeType;
		parameters.push_back((Property*)&this->visible);
	}
	virtual void Draw(Canvas& canvas, float time, bool displayControls) const = 0;
	virtual bool Contains(const Vec2f& coord, float time) const = 0;
	virtual Shape* Clone() = 0;
	virtual void SetPosition(float time, const Vec2f& coord) = 0;
	virtual Vec2f GetPosition(float time) = 0;
	virtual void SetScale(float time, float value) = 0;
	virtual float GetScale(float time) = 0;

	friend std::ostream& operator<<(std::ostream& os, const Shape& v) {
		os << v.parameters.size() << std::endl;
		for (int i = 0; i < v.parameters.size(); i++)
			os << v.parameters[i] << std::endl;
		os << v.visible << std::endl;
		os << v.id;
		return os;
	}
	friend std::istream& operator>>(std::istream& is, Shape& v) {
		int nvalues;
		is >> nvalues;
		v.parameters.resize(nvalues);
		for (int i = 0; i < nvalues; i++) {
			char line[255];
			std::string l; // read the parameter type. It should not have changed from the existing parameter set.  => assert(l == v.parameters[i].type)
			do {
				is.getline(line, 255);
				l = std::string(line);
				l = lefttrim(l);
			} while (l == "");
			is >> *(v.parameters[i]);
		}
		is >> v.visible;
		is >> v.id;
		return is;
	}

	std::vector<Property*> parameters;
	BoolProperty visible;
	int id;
	std::string shapeType;
	static int numShapes;
};

std::ostream& operator<<(std::ostream& os, const Shape* v);
std::istream& operator>>(std::istream& is, Shape* &v);

class Disk : public Shape {
public:
	Disk(const Vec2s& position = Vec2s("0", "0"), std::string radius = "10", const Vec3u &color = Vec3u(255, 0, 0), const Vec3u &colorEdge = Vec3u(0, 0, 255), std::string thicknessEdge = "1", bool visible = true) :Shape("Disk", visible), pos(position), radius(radius), color(color), colorEdge(colorEdge), thicknessEdge(thicknessEdge) {
		this->colorEdge.setName("Border Color");
		this->radius.setDefaults("Radius", 0.f, 10000.f, 0.05f);
		this->thicknessEdge.setDefaults("Edge thickness", 0.f, 1000.f, 1.f);

		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->radius);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thicknessEdge);
		parameters.push_back((Property*)&this->colorEdge);
	}

	Disk(const Disk& d) : Shape(d.shapeType, true), pos(d.pos), radius(d.radius), color(d.color), colorEdge(d.colorEdge), thicknessEdge(d.thicknessEdge) {
		this->colorEdge.setName("Border Color");
		this->radius.setDefaults("Radius", 0.f, 10000.f, 0.05f);
		this->thicknessEdge.setDefaults("Edge thickness", 0.f, 1000.f, 1.f);

		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->radius);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thicknessEdge);
		parameters.push_back((Property*)&this->colorEdge);

	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		float r = radius.getDisplayValue(time);
		Vec2f p = pos.getDisplayValue(time);
		float th = thicknessEdge.getDisplayValue(time);
		float r2 = sqr(r);
		float r2i = sqr(r - th);
		float r2ib = sqr(r - th - 1);
		float r2b = sqr(r + 1);
		Vec3u color = this->color.getDisplayValue(time);
		Vec3u colorEdge = this->colorEdge.getDisplayValue(time);

		if (visible.getDisplayValue()) {
			int startj = std::max(0, (int)(p[0] - r - 1));
			int endj = std::min(canvas.W, (int)(p[0] + r + 2));
#pragma omp parallel for
			for (int i = std::max(0, (int)(p[1] - r - 1)); i < std::min(canvas.H, (int)(p[1] + r + 2)); i++) {
				for (int j = startj; j < endj; j++) {
					float rad = sqr(i - p[1]) + sqr(j - p[0]);
					if (rad <= r2) {
						if (rad <= r2i) {
							if (rad <= r2ib) {
								canvas(j, i, 0) = color[0];
								canvas(j, i, 1) = color[1];
								canvas(j, i, 2) = color[2];
							} else {
								float dist = sqrt(rad) - r + th + 1;
								canvas(j, i, 0) = colorEdge[0] * (dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = colorEdge[1] * (dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = colorEdge[2] * (dist)+color[2] * (1 - dist);
							}
						} else {
							canvas(j, i, 0) = colorEdge[0];
							canvas(j, i, 1) = colorEdge[1];
							canvas(j, i, 2) = colorEdge[2];
						}
					} else {
						if (rad <= r2b) {
							float dist = sqrt(rad) - r;
							if (th > 0) {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+colorEdge[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+colorEdge[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+colorEdge[2] * (1 - dist);
							} else {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+color[2] * (1 - dist);
							}
						}
					}
				}
			}
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, p[0] - r, p[1] - r, p[0] + r, p[1] + r);
		}

	}

	virtual Shape* Clone() {
		return new Disk(*this);
	}

	virtual bool Contains(const Vec2f& coord, float time) const {
		float r = radius.getDisplayValue(time);
		Vec2f p = pos.getDisplayValue(time);
		if (sqr(coord[1] - p[1]) + sqr(coord[0] - p[0]) <= sqr(r)) {
			return true;
		}
		return false;
	}

	virtual void SetPosition(float time, const Vec2f& coord) {

		/*double lval;
		for (int i = 0; i < 2; i++) {
			if (wxString(pos[i]).ToDouble(&lval)) {
				pos[i] = std::to_string(coord[i]);
			}
		}*/
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));  // NEED TO CHECK WHY I DID ABOVE

	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}

	virtual void SetScale(float time, float value) {
		//radius[0] = value;  
		/*double lval;
		if (wxString(radius[0]).ToDouble(&lval)) {
			radius[0] = std::to_string(value);
		}*/
		radius.setDisplayValue(Expr(std::to_string(value)));  // NEED TO CHECK WHY I DID ABOVE
	}
	virtual float GetScale(float time) {
		return radius.getDisplayValue(time);
	}
	PositionProperty pos;
	ColorProperty color, colorEdge;
	FloatProperty radius, thicknessEdge;
};

class PolygonShape : public Shape { // Polygon already taken in wingdi.h (5127)...
public:
	PolygonShape(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", std::string angle = "0", const Vec3u &color = Vec3u(255, 0, 0), const Vec3u &edgeColor = Vec3u(0, 0, 255), std::string edgeThickness = "1", bool visible = true) :Shape("PolygonShape", visible), pos(position), scale(scale), color(color), angle(angle), edgeColor(edgeColor), edgeThickness(edgeThickness) {

		this->edgeColor.setName("Edge Color");
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->angle.setDefaults("Angle (rad)", -1000.f, 1000.f, 0.01f);
		this->edgeThickness.setDefaults("Edge thickness", 0.f, 1000.f, 1.f);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->angle);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->edgeColor);
		parameters.push_back((Property*)&this->edgeThickness);
		parameters.push_back((Property*)&this->vertices);
	}

	PolygonShape(const PolygonShape& d) : Shape(d.shapeType, true), pos(d.pos), scale(d.scale), color(d.color), angle(d.angle), vertices(d.vertices), edgeColor(d.edgeColor), edgeThickness(d.edgeThickness) {
		this->edgeColor.setName("Edge Color");
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->angle.setDefaults("Angle (rad)", -1000.f, 1000.f, 0.01f);
		this->edgeThickness.setDefaults("Edge thickness", 0.f, 1000.f, 1.f);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->angle);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->edgeColor);
		parameters.push_back((Property*)&this->edgeThickness);
		parameters.push_back((Property*)&this->vertices);
	}

	void BSplineSubdivide(float time);

	void addVertex(float time, const Vec2f& v, bool createNewContour = false) {
		vertices.addVertex(time, v, createNewContour);
	}
	void insertVertex(float time, const Vec2f& v, int prevVertex) {
		vertices.insertVertex(time, v, prevVertex);
	}
	int getComponentVtxCount(float time, int component) {
		if (component >= this->vertices.getDisplayValue().contourList.size()) return 0;
		int start = 0;
		if (component > 0) {
			start = this->vertices.getDisplayValue().contourList[component - 1] + 1;
		}
		return this->vertices.getDisplayValue().contourList[component] - start + 1;
	}

	virtual Shape* Clone() {
		return new PolygonShape(*this);
	}

	void setText(float time, const char* text) {
		VerticesList vertices;
		TextToLine t(text);
		std::vector<float> co;
		std::vector<int> contours;
		t.convert(co, contours);
		vertices.Clear();
		int curContour = 0;
		for (int i = 0; i < co.size() / 2; i++) {
			if (i == contours[curContour]) {
				vertices.addVertex(Vec2f(co[i * 2], co[i * 2 + 1]), true);
				curContour++;
			} else {
				vertices.addVertex(Vec2f(co[i * 2], co[i * 2 + 1]), false);
			}
		}
		vertices.contourList = contours;
		this->vertices.setDisplayValue(vertices);

	}


	virtual bool Contains(const Vec2f& initPoint, float s, float a, const Vec2f &p, float time) const {
		Vec2f point = rotate((initPoint - p) / s, -a);

		const int X = 0;
		const int Y = 1;
		int j, yflag0, yflag1, inside_flag, xflag0;
		float ty, tx;
		VerticesList vertices = this->vertices.getDisplayValue();
		const Vec2f *vtx0, *vtx1;
		const Vec2f* pgon = &vertices.vertices[0];
		int numverts = vertices.vertices.size();

		tx = point[X];
		ty = point[Y];

		//vtx0 = &pgon[numverts - 1].coords[0];
		vtx0 = &pgon[vertices.contourList[0]];
		/* get test bit for above/below X axis */
		yflag0 = ((*vtx0)[Y] >= ty);
		vtx1 = &pgon[0];

		inside_flag = 0;
		int vtxId = 0;
		int numContour = 0;
		for (j = numverts + 1; --j; ) {

			yflag1 = ((*vtx1)[Y] >= ty);
			/* check if edge's endpoints straddle (are on opposite sides) of X axis
			 * (i.e. the Y's differ); if so, +X ray could intersect this edge.
			 */
			if (yflag0 != yflag1) {
				xflag0 = ((*vtx0)[X] >= tx);
				/* check if endpoints are on same side of the Y axis (i.e. X's
				 * are the same); if so, it's easy to test if edge hits or misses.
				 */
				if (xflag0 == ((*vtx1)[X] >= tx)) {

					/* if edge's X values both right of the point, must hit */
					if (xflag0) inside_flag = !inside_flag;
				} else {
					/* compute intersection of polygon edge with +X ray, note
					 * if >= point's X; if so, the ray hits it.
					 */
					if (((*vtx1)[X] - ((*vtx1)[Y] - ty) *
						((*vtx0)[X] - (*vtx1)[X]) / ((*vtx0)[Y] - (*vtx1)[Y])) >= tx) {
						inside_flag = !inside_flag;
					}
				}
				/* small improvement over original code: update yflag0 only when
				 * we know it differs from yflag1.
				 */
				yflag0 = yflag1;
			}

			/* move to next pair of vertices, retaining info as possible */
			vtx0 = vtx1;
			vtx1++;

			if (vtxId == vertices.contourList[numContour]) {

				//vtx0 = vtx1;
				//vtx1 += 2;
				if (numContour + 1 <= vertices.contourList.size() - 1)
					vtx0 = &pgon[vertices.contourList[numContour + 1]];
				else
					vtx0 = &pgon[vertices.contourList[numContour]];
				//vtxId++;
				yflag0 = ((*vtx0)[Y] >= ty);
				numContour++;
				//continue;
			}
			vtxId++;
		}

		return(inside_flag);


#if 0
		float tx, ty, u0, u1, u2, v0, v1, vx0, vy0, alpha, beta, denom;
		int inside_flag;
		const int X = 0;
		const int Y = 1;
		const Vec2f* pgon = &vertices.vertices[0];
		int numverts = vertices.vertices.size();
		tx = point[X];
		ty = point[Y];
		vx0 = pgon[0][X];
		vy0 = pgon[0][Y];
		u0 = tx - vx0;
		v0 = ty - vy0;

		inside_flag = 0;
		pgend = &pgon[numverts - 1];
		for (pg1 = &pgon[1], pg2 = &pgon[2]; pg1 != pgend; pg1 += 1, pg2 += 1) {

			u1 = (*pg1)[X] - vx0;
			if (u1 == 0.0) {

				/* 0 and 1 vertices have same X value */

				/* zero area test - can be removed for convex testing */
				u2 = (*pg2)[X] - vx0;
				if ((u2 == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = u0 / u2) <= 0.0) ||
					(beta > 1.0) ||

					/* zero area test - remove for convex testing */
					((v1 = (*pg1)[Y] - vy0) == 0.0) ||

					/* compute alpha and check bounds */
					((alpha = (v0 - beta *
					((*pg2)[Y] - vy0)) / v1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}

			} else {
				/* 0 and 1 vertices have different X value */

				/* compute denom and check for zero area triangle - check
				 * is not needed for convex polygon testing
				 */
				u2 = (*pg2)[X] - vx0;
				v1 = (*pg1)[Y] - vy0;
				denom = ((*pg2)[Y] - vy0) * u1 - u2 * v1;
				if ((denom == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = (v0 * u1 - u0 * v1) / denom) <= 0.0) ||
					(beta > 1.0) ||

					/* compute alpha & check bounds */
					((alpha = (u0 - beta * u2) / u1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}
			}

			/* check gamma */
			if (alpha + beta <= 1.0) {
				/* survived */
				inside_flag = !inside_flag;
			}

		NextTri:;
		}
		return(inside_flag);
#endif
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		return Contains(initPoint, scale.getDisplayValue(time), angle.getDisplayValue(time), pos.getDisplayValue(time), time);
	}

	virtual bool ContainsForDraw(const FastVec2f& point, const VerticesList& vertices) const {
#if 0
		const FastVec2f *pg1, *pg2, *pgend;
		float tx, ty, u0, u1, u2, v0, v1, vx0, vy0, alpha, beta, denom;
		int inside_flag;
		const int X = 0;
		const int Y = 1;
		const FastVec2f* pgon = &fastTransformedVertices[0];
		int numverts = fastTransformedVertices.size();
		tx = point[X];
		ty = point[Y];
		vx0 = pgon[0][X];
		vy0 = pgon[0][Y];
		u0 = tx - vx0;
		v0 = ty - vy0;

		inside_flag = 0;
		pgend = &pgon[numverts - 1];
		for (pg1 = &pgon[1], pg2 = &pgon[2]; pg1 != pgend; pg1 += 1, pg2 += 1) {

			u1 = (*pg1)[X] - vx0;
			if (u1 == 0.0) {

				/* 0 and 1 vertices have same X value */

				/* zero area test - can be removed for convex testing */
				u2 = (*pg2)[X] - vx0;
				if ((u2 == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = u0 / u2) <= 0.0) ||
					(beta > 1.0) ||

					/* zero area test - remove for convex testing */
					((v1 = (*pg1)[Y] - vy0) == 0.0) ||

					/* compute alpha and check bounds */
					((alpha = (v0 - beta *
					((*pg2)[Y] - vy0)) / v1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}

			} else {
				/* 0 and 1 vertices have different X value */

				/* compute denom and check for zero area triangle - check
				 * is not needed for convex polygon testing
				 */
				u2 = (*pg2)[X] - vx0;
				v1 = (*pg1)[Y] - vy0;
				denom = ((*pg2)[Y] - vy0) * u1 - u2 * v1;
				if ((denom == 0.0) ||

					/* compute beta and check bounds */
					/* we use "<= 0.0" so that points on the shared interior
					 * edge will (generally) be inside only one polygon.
					 */
					((beta = (v0 * u1 - u0 * v1) / denom) <= 0.0) ||
					(beta > 1.0) ||

					/* compute alpha & check bounds */
					((alpha = (u0 - beta * u2) / u1) < 0.0)) {

					/* whew! missed! */
					goto NextTri;
				}
			}

			/* check gamma */
			if (alpha + beta <= 1.0) {
				/* survived */
				inside_flag = !inside_flag;
			}

		NextTri:;
		}
		return(inside_flag);
#endif

		const int X = 0;
		const int Y = 1;
		int j, yflag0, yflag1, inside_flag, xflag0;
		float ty, tx;
		const float *vtx0, *vtx1;
		const FastVec2f* pgon = &fastTransformedVertices[0];
		int numverts = fastTransformedVertices.size();

		tx = point[X];
		ty = point[Y];

		//vtx0 = &pgon[numverts - 1].coords[0];
		vtx0 = &pgon[vertices.contourList[0]].coords[0];
		/* get test bit for above/below X axis */
		yflag0 = (vtx0[Y] >= ty);
		vtx1 = &pgon[0].coords[0];

		inside_flag = 0;
		int vtxId = 0;
		int numContour = 0;
		for (j = numverts + 1; --j; ) {

			yflag1 = (vtx1[Y] >= ty);
			/* check if edge's endpoints straddle (are on opposite sides) of X axis
			 * (i.e. the Y's differ); if so, +X ray could intersect this edge.
			 */
			if (yflag0 != yflag1) {
				xflag0 = (vtx0[X] >= tx);
				/* check if endpoints are on same side of the Y axis (i.e. X's
				 * are the same); if so, it's easy to test if edge hits or misses.
				 */
				if (xflag0 == (vtx1[X] >= tx)) {

					/* if edge's X values both right of the point, must hit */
					if (xflag0) inside_flag = !inside_flag;
				} else {
					/* compute intersection of polygon edge with +X ray, note
					 * if >= point's X; if so, the ray hits it.
					 */
					if ((vtx1[X] - (vtx1[Y] - ty) *
						(vtx0[X] - vtx1[X]) / (vtx0[Y] - vtx1[Y])) >= tx) {
						inside_flag = !inside_flag;
					}
				}
				/* small improvement over original code: update yflag0 only when
				 * we know it differs from yflag1.
				 */
				yflag0 = yflag1;
			}

			/* move to next pair of vertices, retaining info as possible */
			vtx0 = vtx1;
			vtx1 += 2;

			if (vtxId == vertices.contourList[numContour]) {

				//vtx0 = vtx1;
				//vtx1 += 2;
				if (numContour + 1 <= vertices.contourList.size() - 1)
					vtx0 = &pgon[vertices.contourList[numContour + 1]].coords[0];
				else
					vtx0 = &pgon[vertices.contourList[numContour]].coords[0];
				//vtxId++;
				yflag0 = (vtx0[Y] >= ty);
				numContour++;
				//continue;
			}
			vtxId++;
		}

		return(inside_flag);
	}

	float distFromNearestEdgeForDraw(const FastVec2f& v, const VerticesList& vertices) const {  // s = evaluated scale		
		float d = 1E9;
		int N = fastTransformedVertices.size();
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			const FastVec2f &v1 = fastTransformedVertices[nextPoint];
			const FastVec2f &v2 = fastTransformedVertices[i];
			float absciProj = dot(v - v1, v2 - v1);
			if (absciProj <= 0) {  // dist point point
				d = std::min(d, norm2(v - v1));
			} else {
				absciProj /= norm2(v2 - v1);
				if (absciProj >= 1) {
					d = std::min(d, norm2(v - v2));
				} else { // dist point line
					FastVec2f p = v1 + absciProj * (v2 - v1);
					d = std::min(d, norm2(p - v));
				}
			}
		}
		return sqrt(d);
	}

	Vec2f nearestPoint(const Vec2f& initPoint, float s, float a, const Vec2f &p, float time) const {  // s = evaluated scale
		Vec2f v = rotate((initPoint - p) / s, -a);
		VerticesList vertices = this->vertices.getDisplayValue();
		float d = 1E9;
		int N = vertices.vertices.size();
		Vec2f result;
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			Vec2f v1 = vertices.vertices[nextPoint];
			Vec2f v2 = vertices.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = v1;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = v2;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(p - v);
					if (dp < d) {
						d = dp;
						result = p;
					}
				}
			}
		}
		return result;
	}

	int nearestVertex(const Vec2f& initPoint, float s, float a, const Vec2f &p, float searchRadius, float time) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		VerticesList vertices = this->vertices.getDisplayValue();
		int N = vertices.vertices.size();
		int result = -1;
		for (int i = 0; i < N; i++) {
			Vec2f v1 = vertices.vertices[i];
			float d1 = norm(v - v1);
			if (d1 < d && d1 < searchRadius) {
				d = d1;
				result = i;
			}
		}
		return result;
	}

	int nearestSegment(const Vec2f& initPoint, float s, float a, const Vec2f &p, float time) const {  // s = evaluated scale ; segment = [result;result+1]
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		VerticesList vertices = this->vertices.getDisplayValue();
		int N = vertices.vertices.size();
		int result;
		int curContour = 0;
		for (int i = 0; i < N; i++) {
			int nextPoint = i + 1;
			if (i == vertices.contourList[curContour]) {
				if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = vertices.contourList[curContour - 1] + 1;
				}
				curContour++;
			}
			Vec2f v1 = vertices.vertices[nextPoint];
			Vec2f v2 = vertices.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = i;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = (i + 1) % N;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(v - p);
					if (dp < d) {
						d = dp;
						result = i;
					}
				}
			}
		}
		return result;
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		VerticesList evalVertices = vertices.getDisplayValue();
		Vec3u color = this->color.getDisplayValue(time);
		Vec3u edgeColor = this->edgeColor.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		float a = angle.getDisplayValue(time);
		FastVec2f p(pos.getDisplayValue(time));
		int minX = 1E9, maxX = -1E9, minY = 1E9, maxY = -1E9;
		int N = evalVertices.vertices.size();
		fastTransformedVertices.resize(N);
		FastVec2f center(0, 0);
		for (int i = 0; i < N; i++) {
			FastVec2f v = rotate(FastVec2f(evalVertices.vertices[i]), a);
			fastTransformedVertices[i] = v * s + p;
			center = center + fastTransformedVertices[i];
			minX = std::min(minX, (int)(fastTransformedVertices[i][0]));
			maxX = std::max(maxX, (int)(fastTransformedVertices[i][0]));
			minY = std::min(minY, (int)(fastTransformedVertices[i][1]));
			maxY = std::max(maxY, (int)(fastTransformedVertices[i][1]));
		}
		center = center / (float)N;
		int ominX = minX, ominY = minY, omaxX = maxX, omaxY = maxY;
		minX = std::max(minX - 1, 0);
		maxX = std::min(maxX - 1, canvas.W - 1);
		minY = std::max(minY + 1, 0);
		maxY = std::min(maxY + 1, canvas.H - 1);
		float edgeThick = edgeThickness.getDisplayValue(time);
		const unsigned char* cc = &color.coords[0];
		if (visible.getDisplayValue()) {
#pragma omp parallel for
			for (int i = minY; i <= maxY; i++) {
				for (int j = minX; j <= maxX; j++) {
					if (ContainsForDraw(Vec2f(j, i), evalVertices)) {
						/*float dist = distFromNearestEdgeForDraw(Vec2f(j, i), evalVertices);
						if (dist < edgeThick) {
							canvas(j, i, 0) = edgeColor[0];
							canvas(j, i, 1) = edgeColor[1];
							canvas(j, i, 2) = edgeColor[2];
						} else {*/
						/*canvas(j, i, 0) = color[0];
						canvas(j, i, 1) = color[1];
						canvas(j, i, 2) = color[2];*/
						memcpy(&canvas(j, i, 0), cc, 3 * sizeof(char));
						//}
					} else {
						float dist = distFromNearestEdgeForDraw(Vec2f(j, i), evalVertices);
						if (dist < 1) {
							if (edgeThick > 0) {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+edgeColor[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+edgeColor[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+edgeColor[2] * (1 - dist);
							} else {
								canvas(j, i, 0) = canvas(j, i, 0)*(dist)+color[0] * (1 - dist);
								canvas(j, i, 1) = canvas(j, i, 1)*(dist)+color[1] * (1 - dist);
								canvas(j, i, 2) = canvas(j, i, 2)*(dist)+color[2] * (1 - dist);
							}
						}
					}
				}
			}

			int curContour = 0;
			cairo_t *cr = canvas.cr;
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_set_line_width(cr, s*edgeThick);
			cairo_move_to(cr, fastTransformedVertices[0][0], fastTransformedVertices[0][1]);
			int initPoint = 0;
			for (int i = 1; i < fastTransformedVertices.size(); i++) {

				cairo_line_to(cr, fastTransformedVertices[i][0], fastTransformedVertices[i][1]);

				if (i == evalVertices.contourList[curContour]) {
					//cairo_line_to(cr, fastTransformedVertices[initPoint][0], fastTransformedVertices[initPoint][1]);
					cairo_close_path(cr);
					initPoint = i + 1;
					if (i != fastTransformedVertices.size() - 1)
						cairo_move_to(cr, fastTransformedVertices[i + 1][0], fastTransformedVertices[i + 1][1]);
					curContour++;
				}

			}
			cairo_stroke(cr);
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, ominX, ominY, omaxX, omaxY);
			for (int i = 0; i < evalVertices.vertices.size(); i++) {
				const FastVec2f &pp = fastTransformedVertices[i];
				drawSquare(canvas, pp[0], pp[1], 5);
			}
		}


	}

	virtual void SetPosition(float time, const Vec2f& coord) {
		//pos[0] = coord[0];
		//pos[1] = coord[1];
		/*double lval;
		for (int i = 0; i < 2; i++) {
			if (wxString(pos[i]).ToDouble(&lval)) {
				pos[i] = std::to_string(coord[i]);
			}
		}*/
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));  // NEED TO CHECK WHY I DID ABOVE
	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}
	virtual void SetScale(float time, float value) {
		/*double lval;
		if (wxString(scale[0]).ToDouble(&lval)) {
			scale[0] = std::to_string(value);
		}*/
		scale.setDisplayValue(Expr(std::to_string(value)));  // NEED TO CHECK WHY I DID ABOVE
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}

	PositionProperty pos;
	ColorProperty color, edgeColor;
	FloatProperty scale, angle, edgeThickness;
	VerticesListProperty vertices;
	mutable std::vector<FastVec2f> fastTransformedVertices;
};

class PolygonMorph : public Shape {
public:
	PolygonMorph(PolygonShape* s1 = NULL, PolygonShape* s2 = NULL, std::string t = std::string("0"), bool visible = true) : Shape("PolygonMorph", visible), s1(s1), s2(s2), t(t) {
		this->t.setDefaults("Interpolation time", -100.f, 100.f, 0.05f);
		parameters.push_back((Property*)&this->t);
	};
	PolygonMorph(const PolygonMorph& d) : Shape(d.shapeType, true), t(d.t), s1(d.s1), s2(d.s2) {
		this->t.setDefaults("Interpolation time", -100.f, 100.f, 0.05f);
		parameters.push_back((Property*)&this->t);
	}

	PolygonShape interpolatedShape(float time) const {
		float tval = t.getDisplayValue(time);
		float scale1 = s1->GetScale(time);
		float scale2 = s2->GetScale(time);
		Vec2f pos1 = s1->GetPosition(time);
		Vec2f pos2 = s2->GetPosition(time);
		float angle1 = s1->angle.getDisplayValue(time);
		float angle2 = s2->angle.getDisplayValue(time);
		Vec2f pos = (1.f - tval)*pos1 + tval * pos2;
		float scale = (1.f - tval)*scale1 + tval * scale2;
		float angle = (1.f - tval)*angle1 + tval * angle2;
		float thickness = (1.f - tval)*s1->edgeThickness.getDisplayValue(time) + tval * s2->edgeThickness.getDisplayValue(time);
		Vec3u color1 = s1->color.getDisplayValue(time);
		Vec3u color2 = s2->color.getDisplayValue(time);
		Vec3u edgeColor1 = s1->edgeColor.getDisplayValue(time);
		Vec3u edgeColor2 = s2->edgeColor.getDisplayValue(time);
		Vec3u col;
		col[0] = (1.f - tval)*color1[0] + tval * color2[0];
		col[1] = (1.f - tval)*color1[1] + tval * color2[1];
		col[2] = (1.f - tval)*color1[2] + tval * color2[2];
		Vec3u edgecol;
		edgecol[0] = (1.f - tval)*edgeColor1[0] + tval * edgeColor2[0];
		edgecol[1] = (1.f - tval)*edgeColor1[1] + tval * edgeColor2[1];
		edgecol[2] = (1.f - tval)*edgeColor1[2] + tval * edgeColor2[2];
		PolygonShape s(Vec2s(std::to_string(pos[0]), std::to_string(pos[1])), std::to_string(scale), std::to_string(angle), col, edgecol, std::to_string(thickness));

		typedef FullBipartiteDigraph Digraph;
		DIGRAPH_TYPEDEFS(FullBipartiteDigraph);

		VerticesList vertices1 = s1->vertices.getDisplayValue();
		VerticesList vertices2 = s2->vertices.getDisplayValue();

		int Nct = std::max(vertices1.contourList.size(), vertices2.contourList.size());
		for (int ct = 0; ct < Nct; ct++) {
			int n1 = s1->getComponentVtxCount(time, ct);
			int n2 = s2->getComponentVtxCount(time, ct);
			std::vector<Vec2f> v1; v1.reserve(n1 + 3);
			std::vector<Vec2f> v2; v2.reserve(n2 + 3);
			if (n1 > 0) {
				int start = 0;
				if (ct > 0) {
					start = vertices1.contourList[ct - 1] + 1;
				}
				int end = vertices1.contourList[ct];
				for (int i = start; i <= end; i++) {
					v1.push_back(vertices1.vertices[i]);
				}
			} else {
				Vec2f center(0, 0);
				int start = 0;
				if (ct > 0) {
					start = vertices2.contourList[ct - 1] + 1;
				}
				int end = vertices2.contourList[ct];
				for (int i = start; i <= end; i++) {
					center = center + vertices2.vertices[i];
				}
				center = center / (float)(end - start + 1.f);
				v1.push_back(center + Vec2f(0.001f, 0.001f));
				v1.push_back(center + Vec2f(-0.001f, 0.001f));
				v1.push_back(center + Vec2f(0.001f, -0.001f));
				n1 = 3;
			}
			if (n2 > 0) {
				int start = 0;
				if (ct > 0) {
					start = vertices2.contourList[ct - 1] + 1;
				}
				int end = vertices2.contourList[ct];
				for (int i = start; i <= end; i++) {
					v2.push_back(vertices2.vertices[i]);
				}
			} else {
				Vec2f center(0, 0);
				int start = 0;
				if (ct > 0) {
					start = vertices1.contourList[ct - 1] + 1;
				}
				int end = vertices1.contourList[ct];
				for (int i = start; i <= end; i++) {
					center = center + vertices1.vertices[i];
				}
				center = center / (float)(end - start + 1.f);
				v2.push_back(center + Vec2f(0.001f, 0.001f));
				v2.push_back(center + Vec2f(-0.001f, 0.001f));
				v2.push_back(center + Vec2f(0.001f, -0.001f));
				n2 = 3;
			}
			Digraph di(n1, n2);
			NetworkSimplexSimple<Digraph, double, double, int> net(di, true, n1 + n2, n1*n2);

			int idarc = 0;
			for (int i = 0; i < n1; i++) {
				Vec2f pt1 = rotate((v1[i] - pos1) / scale1, -angle1);
				for (int j = 0; j < n2; j++) {
					double d = norm2(pt1 - rotate((v2[j] - pos2) / scale2, -angle2));
					net.setCost(di.arcFromId(idarc), d);
					idarc++;
				}
			}

			std::vector<double> weights1(n1, 1. / n1);
			std::vector<double> weights2(n2, -1. / n2);
			net.supplyMap(&weights1[0], n1, &weights2[0], n2);
			int ret = net.run();

			if (n1 > n2) {
				for (int i = 0; i < n1; i++) {
					Vec2f dest(0.f, 0.f);
					float sumAmount = 1;
					float prevAmount = -1;
					for (int j = 0; j < n2; j++) {
						float amount = net.flow(di.arcFromId(i*n2 + j));
						/*if (fabs(amount) > 1E-7) {
							dest = dest + amount * v2[j];
							sumAmount += amount;
						}*/
						if (amount > prevAmount) {
							prevAmount = amount;
							dest = v2[j];
						}
					}
					s.addVertex(time, (1.f - tval)*v1[i] + tval * dest / sumAmount, i == 0);
				}
			} else {
				for (int i = 0; i < n2; i++) {
					Vec2f dest(0.f, 0.f);
					float sumAmount = 1;
					float prevAmount = -1;
					for (int j = 0; j < n1; j++) {
						float amount = net.flow(di.arcFromId(j*n2 + i));
						/*if (fabs(amount) > 1E-7) {
							dest = dest + amount * v1[j];
							sumAmount += amount;
						}*/
						if (amount > prevAmount) {
							prevAmount = amount;
							dest = v1[j];
						}
					}
					s.addVertex(time, (1.f - tval)*dest / sumAmount + tval * v2[i], i == 0);
				}
			}

		}

		/*for (int i = 0; i < std::min(s1->vertices.vertices.size(), s2->vertices.vertices.size()); i++) {
			s.addVertex((1.f - tval)*s1->vertices.vertices[i] + tval * s2->vertices.vertices[i]);
		}*/
		return s;
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		if (visible.getDisplayValue()) {
			PolygonShape s(interpolatedShape(time));

			s.Draw(canvas, time, displayControls);
		}
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		PolygonShape s(interpolatedShape(time));
		return s.Contains(initPoint, time);
	}

	virtual Shape* Clone() {
		return new PolygonMorph(*this);
	}


	virtual void SetPosition(float time, const Vec2f& coord) {
		//pos[0] = coord[0];
		//pos[1] = coord[1];
	}
	virtual Vec2f GetPosition(float time) {
		float tval = t.getDisplayValue(time);
		return (1.f - tval)*s1->GetPosition(time) + tval * s2->GetPosition(time);
	}
	virtual void SetScale(float time, float value) {
		//scale[0] = value;
	}
	virtual float GetScale(float time) {
		float tval = t.getDisplayValue(time);
		return (1.f - tval)*s1->GetScale(time) + tval * s2->GetScale(time);
	}

	FloatProperty t;
	PolygonShape *s1, *s2;
};

class Spline : public Shape {
public:
	Spline(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", const Vec3u &color = Vec3u(255, 0, 0), bool visible = true, std::string thickness = "1") : Shape("Spline", visible), pos(position), scale(scale), color(color), thickness(thickness), capStart(1), capEnd(1), progressStart(Expr("0")), progressEnd(Expr("100")), lineType(1) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		std::string options[] = { "Square", "Round", "Arrow" };
		std::string options2[] = { "Straight", "BSpline" };
		this->capStart.setDefaults("Cap Start", options, 3);
		this->capEnd.setDefaults("Cap End", options, 3);
		this->lineType.setDefaults("Line Type", options2, 2);
		this->progressStart.setDefaults("Progress start (%)", 0, 100, 0.5);
		this->progressEnd.setDefaults("Progress end (%)", 0, 100, 0.5);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);
		parameters.push_back((Property*)&this->lineType);
		parameters.push_back((Property*)&this->capStart);
		parameters.push_back((Property*)&this->capEnd);
		parameters.push_back((Property*)&this->progressStart);
		parameters.push_back((Property*)&this->progressEnd);
		parameters.push_back((Property*)&this->controlPoints);		
	}
	Spline(const Spline& d) : Shape(d.shapeType, true), pos(d.pos), scale(d.scale), color(d.color), controlPoints(d.controlPoints), thickness(d.thickness), capStart(d.capStart), capEnd(d.capEnd), progressStart(d.progressStart), progressEnd(d.progressEnd), lineType(d.lineType) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		std::string options[] = { "None", "Round", "Arrow" };
		std::string options2[] = { "Straight", "BSpline" };
		this->capStart.setDefaults("Cap Start", options, 3);
		this->capEnd.setDefaults("Cap End", options, 3);
		this->lineType.setDefaults("Line Type", options2, 2);
		this->progressStart.setDefaults("Progress start (%)", 0, 100, 0.5);
		this->progressEnd.setDefaults("Progress end (%)", 0, 100, 0.5);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);
		parameters.push_back((Property*)&this->lineType);
		parameters.push_back((Property*)&this->capStart);
		parameters.push_back((Property*)&this->capEnd);
		parameters.push_back((Property*)&this->progressStart);
		parameters.push_back((Property*)&this->progressEnd);
		parameters.push_back((Property*)&this->controlPoints);
	}

	void addVertex(float time, const Vec2f& v) {
		controlPoints.addVertex(time, v);
	}

	Vec2f evalLine(float t, const VerticesList& controlPoints) const {
		float totalLength = 0;
		for (int j = 1; j < controlPoints.size(); j++) {
			totalLength += norm(controlPoints.vertices[j] - controlPoints.vertices[j - 1]);
		}
		t*= totalLength;
		float curLength = 0;
		for (int j = 1; j < controlPoints.size(); j++) {
			float segLength = norm(controlPoints.vertices[j] - controlPoints.vertices[j - 1]);
			curLength += segLength;
			if (curLength >= t-0.00001) {
				Vec2f p = controlPoints.vertices[j - 1] + (t-(curLength-segLength))/segLength *(controlPoints.vertices[j] - controlPoints.vertices[j - 1]);
				return p;
			}
		}
		return controlPoints.vertices[controlPoints.vertices.size() - 1];
	}

	//0 1/6 2/6 3/6 4/6 5/6 6/6
	Vec2f evalBSpline(float t, const VerticesList& controlPoints) const { // t in 0..1
		Vec2f result;
		int N = controlPoints.vertices.size();
		int i = t * (N - 1);
		t = t * (N - 1) - i;
		if (i >= 1 && i <= N - 3) {
			result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 2])*(t*t*t / 6.f)
				+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
				+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
				+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
		} else {
			if (i == 0) {
				result = (-controlPoints.vertices[i] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 2])*(t*t*t / 6.f)
					+ (3.f*controlPoints.vertices[i] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
					+ (-3.f*controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
					+ (controlPoints.vertices[i] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
			} else {
				if (i == N - 2) {
					result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i + 1] + controlPoints.vertices[i + 1])*(t*t*t / 6.f)
						+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i + 1])*(t*t / 6.f)
						+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i + 1])*(t / 6.f)
						+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i + 1])*(1 / 6.f);
				} else { // i==N-1
					result = (-controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i] - 3.f* controlPoints.vertices[i] + controlPoints.vertices[i])*(t*t*t / 6.f)
						+ (3.f*controlPoints.vertices[i - 1] - 6.f* controlPoints.vertices[i] + 3.f* controlPoints.vertices[i])*(t*t / 6.f)
						+ (-3.f*controlPoints.vertices[i - 1] + 3.f* controlPoints.vertices[i])*(t / 6.f)
						+ (controlPoints.vertices[i - 1] + 4.f* controlPoints.vertices[i] + controlPoints.vertices[i])*(1 / 6.f);
				}
			}
		}
		return result;
	}

	void setupArrowPath(cairo_t *cr, const Vec2f& arrowDir, const Vec2f& arrowStart, float arrowSize) const {

		Vec2f perp(-arrowDir[1], arrowDir[0]);
		for (int i = 0; i < 4; i++) {
			Vec2f ar(arrow2[i * 2], arrow2[i * 2 + 1]);			
			Vec2f arcoord = (ar[0] * arrowDir + ar[1] * perp)*arrowSize + arrowStart;
			if (i == 0) {
				cairo_move_to(cr, arcoord[0], arcoord[1]);
			} else {
				cairo_line_to(cr, arcoord[0], arcoord[1]);
			}
		}
		cairo_close_path(cr);
	}




	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		VerticesList evalControlPoints = controlPoints.getDisplayValue();
		float s = scale.getDisplayValue(time);
		Vec2f p = pos.getDisplayValue(time);
		Vec3u color = this->color.getDisplayValue(time);
		int minX = 1E9, maxX = -1E9, minY = 1E9, maxY = -1E9;
		for (int i = 0; i < evalControlPoints.vertices.size(); i++) {
			minX = std::min(minX, (int)((evalControlPoints.vertices[i][0] * s + p[0])));
			maxX = std::max(maxX, (int)((evalControlPoints.vertices[i][0] * s + p[0]) + 0.5));
			minY = std::min(minY, (int)((evalControlPoints.vertices[i][1] * s + p[1])));
			maxY = std::max(maxY, (int)((evalControlPoints.vertices[i][1] * s + p[1]) + 0.5));
		}
		int ominX = minX, ominY = minY, omaxX = maxX, omaxY = maxY;
		minX = std::max(minX - 1, 0);
		maxX = std::min(maxX - 1, canvas.W - 1);
		minY = std::max(minY + 1, 0);
		maxY = std::min(maxY + 1, canvas.H - 1);


		if (visible.getDisplayValue()) {
			int cpStart = capStart.getDisplayValue(time);
			int cpEnd = capEnd.getDisplayValue(time);
			float edgeThick = thickness.getDisplayValue(time);
			cairo_t *cr = canvas.cr;
			cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);

			if (lineType.getDisplayValue(time) == 1) { // Splines
				
				float Ndisc = 200.f;
				int istart = (progressStart.getDisplayValue(time) * Ndisc) / 100;
				int iend = (progressEnd.getDisplayValue(time) * Ndisc) / 100;

				if (cpStart == 2) {
					cairo_set_line_width(cr, 1);
					Vec2f vendm1 = evalBSpline(istart / Ndisc, evalControlPoints)*s + p;
					Vec2f vend = evalBSpline((istart + 1.f) / Ndisc, evalControlPoints)*s + p;
					Vec2f ardir = getNormalized(vendm1 - vend);
					setupArrowPath(cr, ardir, vendm1, s*edgeThick);					
					cairo_fill(cr);
				}

				if (cpStart == 0)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
				if (cpStart == 1)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);


				cairo_set_line_width(cr, s*edgeThick);
				Vec2f v = evalBSpline(istart / Ndisc, evalControlPoints)*s + p;
				cairo_move_to(cr, v[0], v[1]);

				for (int i = istart; i < iend; i++) {
					Vec2f v = evalBSpline(i / Ndisc, evalControlPoints)*s + p;
					if (i >= 1) {
						Vec2f v0 = evalBSpline((i - 0.66) / Ndisc, evalControlPoints)*s + p;
						Vec2f v1 = evalBSpline((i - 0.33) / Ndisc, evalControlPoints)*s + p;
						cairo_curve_to(cr, v0[0], v0[1], v1[0], v1[1], v[0], v[1]);
						if (i == (istart + iend) / 2) {
							cairo_stroke(cr);
							cairo_move_to(cr, v[0], v[1]);
						}
					} else {
						cairo_line_to(cr, v[0], v[1]);
					}
				}
				if (cpEnd == 0)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
				if (cpEnd == 1)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
				cairo_stroke(cr);
				if (cpEnd == 2) {
					cairo_set_line_width(cr, 1);
					Vec2f vendm1 = evalBSpline((iend - 1.f) / Ndisc, evalControlPoints)*s + p;
					Vec2f vend = evalBSpline(iend / Ndisc, evalControlPoints)*s + p;
					Vec2f ardir = getNormalized(vend - vendm1);
					setupArrowPath(cr, ardir, vendm1, s*edgeThick);
					cairo_fill(cr);
				}
			} else { // Straight Line

				float totalLength = 0;
				for (int i = 1; i < evalControlPoints.size(); i++) {
					totalLength += norm(evalControlPoints.vertices[i] - evalControlPoints.vertices[i - 1]);
				}
				float lengthStart = totalLength * progressStart.getDisplayValue(time) / 100.f;
				float lengthEnd = totalLength * progressEnd.getDisplayValue(time) / 100.f;

				float curLength = 0, iLengthStart, iLengthEnd;
				int iStart, iEnd;
				Vec2f pStart, pEnd;
				for (int i = 1; i < evalControlPoints.size(); i++) {		
					float segLength = norm(evalControlPoints.vertices[i] - evalControlPoints.vertices[i - 1]);
					curLength += segLength;
					if (curLength >= lengthStart - 0.0001) {
						iStart = i - 1; iLengthStart = curLength; 
						pStart = evalControlPoints.vertices[i - 1] + (lengthStart - (curLength - segLength)) / segLength * (evalControlPoints.vertices[i] - evalControlPoints.vertices[i - 1]);
						break;
					}
				}
				curLength = 0;
				for (int i = 1; i < evalControlPoints.size(); i++) {
					float segLength = norm(evalControlPoints.vertices[i] - evalControlPoints.vertices[i - 1]);
					curLength += segLength;
					if (curLength >= lengthEnd-0.0001) {
						iEnd = i; iLengthEnd = curLength;
						pEnd = evalControlPoints.vertices[i - 1] + (lengthEnd - (curLength - segLength)) / segLength * (evalControlPoints.vertices[i] - evalControlPoints.vertices[i - 1]);
						break;
					}
				}

				if (cpStart == 2) {
					cairo_set_line_width(cr, 1);
					Vec2f vendm1 = pStart*s + p;
					Vec2f vend = evalControlPoints.vertices[iStart+1]*s + p;
					Vec2f ardir = getNormalized(vendm1 - vend);
					setupArrowPath(cr, ardir, vendm1, s*edgeThick);
					cairo_fill(cr);
				}

				if (cpStart == 0)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
				if (cpStart == 1)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);


				cairo_set_line_width(cr, s*edgeThick);
				Vec2f v = pStart*s + p;
				cairo_move_to(cr, v[0], v[1]);

				for (int i = iStart+1; i < iEnd; i++) {
					Vec2f v = evalControlPoints.vertices[i] *s + p;
					cairo_line_to(cr, v[0], v[1]);
				}
				v = pEnd * s + p;
				cairo_line_to(cr, v[0], v[1]);
				if (cpEnd == 0)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
				if (cpEnd == 1)
					cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
				cairo_stroke(cr);
				if (cpEnd == 2) {
					cairo_set_line_width(cr, 1);
					Vec2f vendm1 = evalControlPoints.vertices[iEnd-1] * s + p; 
					Vec2f vend = pEnd * s + p;
					Vec2f ardir = getNormalized(vend - vendm1);
					setupArrowPath(cr, ardir, vend, s*edgeThick);
					cairo_fill(cr);
				}

			}

			cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
		}

		if (displayControls) { // draw Bbox
			drawBox(canvas, ominX, ominY, omaxX, omaxY);
			int N = evalControlPoints.vertices.size();
			cairo_t *cr = canvas.cr;
			Vec2f v = evalControlPoints.vertices[0] * s + p;
			cairo_set_source_rgb(cr, 0.f, 0.f, 0.f);
			cairo_set_line_width(cr, 1.f);
			cairo_move_to(cr, v[0], v[1]);
			for (int i = 1; i < N; i++) {
				Vec2f v = evalControlPoints.vertices[i] * s + p;
				cairo_line_to(cr, v[0], v[1]);
			}
			cairo_stroke(cr);
			for (int i = 0; i < evalControlPoints.vertices.size(); i++) {
				Vec2f pp = evalControlPoints.vertices[i] * s + p;
				drawSquare(canvas, pp[0], pp[1], 5);
			}
		}
	}

	virtual Shape* Clone() {
		return new Spline(*this);
	}

	virtual bool Contains(const Vec2f& initPoint, float time) const {
		VerticesList controlPoints = this->controlPoints.getDisplayValue();
		float s = scale.getDisplayValue(time);
		Vec2f p = pos.getDisplayValue(time);
		float d = 1E9;
		for (int i = 0; i < 50; i++) {
			d = std::min(d, norm2(evalBSpline(i / 50.f, controlPoints)*s + p - initPoint));
		}
		d = sqrt(d);
		if (d < 5) return true;
		return false;
	}

	void insertControlPoint(float time, const Vec2f& v, int prevVertex) {
		controlPoints.insertVertex(time, v, prevVertex);
	}

	int nearestSegment(const Vec2f& initPoint, float s, float a, const Vec2f &p, float time) const {  // s = evaluated scale ; segment = [result;result+1]
		VerticesList controlPoints = this->controlPoints.getDisplayValue();
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = controlPoints.vertices.size();
		int result;
		int curContour = 0;
		for (int i = 0; i < N - 1; i++) {
			int nextPoint = i + 1;
			if (i == controlPoints.contourList[curContour]) {
				/*if (curContour == 0) {
					nextPoint = 0;
				} else {
					nextPoint = controlPoints.contourList[curContour - 1] + 1;
				}*/
				curContour++;
				continue;
			}
			Vec2f v1 = controlPoints.vertices[nextPoint];
			Vec2f v2 = controlPoints.vertices[i];
			float absciProj = dot(v - v1, v2 - v1) / norm2(v2 - v1);
			float d1 = norm(v - v1);
			float d2 = norm(v - v2);
			if (absciProj <= 0) {  // dist point point
				if (d1 < d) {
					d = d1;
					result = i;
				}
			} else {
				if (absciProj >= 1) {
					if (d2 < d) {
						d = d2;
						result = (i + 1) % N;
					}
				} else { // dist point line
					Vec2f p = v1 + absciProj * (v2 - v1);
					float dp = norm(v - p);
					if (dp < d) {
						d = dp;
						result = i;
					}
				}
			}
		}
		return result;
	}

	int nearestControlPoint(const Vec2f& initPoint, float s, float a, const Vec2f &p, float searchRadius, float time) const {  // s = evaluated scale ; segment = [result;result+1]
		VerticesList controlPoints = this->controlPoints.getDisplayValue();
		Vec2f v = rotate((initPoint - p) / s, -a);
		float d = 1E9;
		int N = controlPoints.vertices.size();
		int result = -1;
		for (int i = 0; i < N; i++) {
			Vec2f v1 = controlPoints.vertices[i];
			float d1 = norm(v - v1);
			if (d1 < d && d1 < searchRadius) {
				d = d1;
				result = i;
			}
		}
		return result;
	}

	virtual void SetPosition(float time, const Vec2f& coord) {
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));
	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}
	virtual void SetScale(float time, float value) {
		scale.setDisplayValue(std::to_string(value));
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}

	PositionProperty pos;
	ColorProperty color;
	FloatProperty scale, thickness, progressStart, progressEnd;
	VerticesListProperty controlPoints;
	EnumProperty capStart, capEnd, lineType;
};

class Plotter1D : public Shape {
public:
	Plotter1D(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", const Vec3u &color = Vec3u(255, 0, 0), bool visible = true, std::string thickness = "1") : Shape("Plotter1D", visible), pos(position), scale(scale), color(color), thickness(thickness), XExtent(Vec2s("0", "1")), YExtent(Vec2s("0", "1")), axisXVisible(true), axisYVisible(true), xticks(5), yticks(5), showGrid(true) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		this->XExtent.name = "X Range";
		this->YExtent.name = "Y Range";
		this->axisXVisible.name = "X Axis visibility";
		this->axisYVisible.name = "Y Axis visibility";
		this->xticks.name = "Nb X ticks";
		this->yticks.name = "Nb Y ticks";
		this->showGrid.name = "Show grid";

		parameters.push_back((Property*)&this->expression);
		parameters.push_back((Property*)&this->XExtent);
		parameters.push_back((Property*)&this->YExtent);
		parameters.push_back((Property*)&this->axisXVisible);
		parameters.push_back((Property*)&this->axisYVisible);
		parameters.push_back((Property*)&this->xticks);
		parameters.push_back((Property*)&this->yticks);
		parameters.push_back((Property*)&this->showGrid);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);

	}
	Plotter1D(const Plotter1D& d) : Shape(d.shapeType, true), pos(d.pos), scale(d.scale), color(d.color), expression(d.expression), thickness(d.thickness), XExtent(d.XExtent), YExtent(d.YExtent), axisXVisible(d.axisXVisible), axisYVisible(d.axisYVisible), xticks(d.xticks), yticks(d.yticks), showGrid(d.showGrid) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		this->XExtent.name = "X Range";
		this->YExtent.name = "Y Range";
		this->axisXVisible.name = "X Axis visibility";
		this->axisYVisible.name = "Y Axis visibility";
		this->xticks.name = "Nb X ticks";
		this->yticks.name = "Nb Y ticks";
		this->showGrid.name = "Show grid";
		parameters.push_back((Property*)&this->expression);
		parameters.push_back((Property*)&this->XExtent);
		parameters.push_back((Property*)&this->YExtent);
		parameters.push_back((Property*)&this->axisXVisible);
		parameters.push_back((Property*)&this->axisYVisible);
		parameters.push_back((Property*)&this->xticks);
		parameters.push_back((Property*)&this->yticks);
		parameters.push_back((Property*)&this->showGrid);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);

	}

	Vec2f evalExpr(float t, const Vec2f &position, const Vec2f &relativeCenter, const Vec2f &xRange, const Vec2f &yRange, float s, float time) const {

		std::string e = expression.getDisplayValue(time);
		std::string newstring = replace_variable(e, t, 'x');
		std::string newstring2 = replace_variable(newstring, time, 't');
		newstring = replace_variable(newstring2, cur_index, 'i');

		float value = ceval_result2(newstring2);

		return Vec2f((t - xRange[0]) / (xRange[1] - xRange[0]) * 200, -((value - yRange[0]) / (yRange[1] - yRange[0])) * 200)*s + position - relativeCenter;
	}



	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {

		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);
		Vec2f xrange = XExtent.getDisplayValue(time);
		Vec2f yrange = YExtent.getDisplayValue(time);
		float x0 = -xrange[0] / (xrange[1] - xrange[0]) * 200 * s + position[0] - relativeCenter[0];
		float y0 = yrange[0] / (yrange[1] - yrange[0]) * 200 * s + position[1] - relativeCenter[1];



		if (visible.getDisplayValue()) {
			float edgeThick = thickness.getDisplayValue(time);
			bool showgrid = showGrid.getDisplayValue();

			cairo_t *cr = canvas.cr;
			cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
			cairo_set_line_width(cr, s);

			unsigned char black[3] = { 0,0,0 };
			unsigned char lightgrey[3] = { 230,230,230 };
			float xscale = 200 * s / (xrange[1] - xrange[0]);
			if (axisXVisible.getDisplayValue() && std::ceil(yrange[0]) <= 0 && std::floor(yrange[1]) >= 0) {
				if (xticks.getDisplayValue(time) > 0) {
					float step = (xrange[1] - xrange[0]) / (xticks.getDisplayValue(time) - 1);
					for (float i = xrange[0]; i <= xrange[1]; i += step) {
						std::string label = to_string_with_precision(i, 2);
						TextToLine::drawText(label, canvas, &black[0], x0 + i * xscale - 3 * s*label.size(), y0 + 15 * s, 3 * s);
						if (showgrid)
							drawAntiAliasedLine1(canvas, x0 + i * xscale, position[1] - relativeCenter[1], x0 + i * xscale, position[1] - 200 * s - relativeCenter[1], Vec3u(lightgrey[0], lightgrey[1], lightgrey[2]), s);
						drawAntiAliasedLine1(canvas, x0 + i * xscale, y0 - 3 * s, x0 + i * xscale, y0 + 3 * s, Vec3u(0, 0, 0), s);
					}
				}

			}


			if (axisYVisible.getDisplayValue() && std::ceil(xrange[0]) <= 0 && std::floor(yrange[1]) >= 0) {
				if (yticks.getDisplayValue(time) > 0) {
					float step = (yrange[1] - yrange[0]) / (yticks.getDisplayValue(time) - 1);
					for (float i = yrange[0]; i <= yrange[1]; i += step) {
						std::string label = to_string_with_precision(i, 2);
						TextToLine::drawText(label, canvas, &black[0], x0 - 10 - 5 * s*label.size(), y0 - i * 200 * s / (yrange[1] - yrange[0]) + 3 * s, 3 * s);
						if (showgrid)
							drawAntiAliasedLine1(canvas, position[0] - relativeCenter[0], y0 - i * 200 * s / (yrange[1] - yrange[0]), position[0] + 200 * s - relativeCenter[0], y0 - i * 200 * s / (yrange[1] - yrange[0]), Vec3u(lightgrey[0], lightgrey[1], lightgrey[2]), s);
						drawAntiAliasedLine1(canvas, x0 - 3 * s, y0 - i * 200 * s / (yrange[1] - yrange[0]), x0 + 3 * s, y0 - i * 200 * s / (yrange[1] - yrange[0]), Vec3u(0, 0, 0), s);
					}
				}
				drawAntiAliasedLine1(canvas, x0, position[1] - relativeCenter[1], x0, position[1] - 200 * s - relativeCenter[1], Vec3u(0, 0, 0), s);
			}
			if (axisXVisible.getDisplayValue() && std::ceil(yrange[0]) <= 0 && std::floor(yrange[1]) >= 0) { // draws the axis /after/ the grid, if any
				//drawLine(canvas, position[0] - 100 * s - relativeCenter[0], y0, position[0] + 100 * s - relativeCenter[0], y0, Vec3u(0, 0, 0), s);
				drawAntiAliasedLine1(canvas, position[0] - relativeCenter[0], y0, position[0] + 200 * s - relativeCenter[0], y0, Vec3u(0, 0, 0), s);
			}

			const int Ndisc = 300;
			float invNdisc = 1.f / (float)Ndisc;


			cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
			cairo_set_line_width(cr, s*edgeThick);
			Vec2f v = evalExpr(0 * invNdisc *(xrange[1] - xrange[0]) + xrange[0], position, relativeCenter, xrange, yrange, s, time);
			cairo_move_to(cr, v[0], v[1]);

			for (int i = 0; i < Ndisc; i++) {
				cur_index = i;
				Vec2f v = evalExpr(i *invNdisc *(xrange[1] - xrange[0]) + xrange[0], position, relativeCenter, xrange, yrange, s, time);
				if (i > 1) {
					Vec2f v0 = evalExpr((i - 0.66) *invNdisc *(xrange[1] - xrange[0]) + xrange[0], position, relativeCenter, xrange, yrange, s, time);
					Vec2f v1 = evalExpr((i - 0.33) *invNdisc *(xrange[1] - xrange[0]) + xrange[0], position, relativeCenter, xrange, yrange, s, time);
					cairo_curve_to(cr, v0[0], v0[1], v1[0], v1[1], v[0], v[1]);
				} else {
					cairo_line_to(cr, v[0], v[1]);
				}
			}
			cairo_stroke(cr);
		}

		if (displayControls) {
			drawBox(canvas, position[0] - relativeCenter[0], position[1] - 200 * s - relativeCenter[1], position[0] + 200 * s - relativeCenter[0], position[1] - relativeCenter[1]);
		}

	}
	virtual bool Contains(const Vec2f& coord, float time) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);
		Vec2f xrange = XExtent.getDisplayValue(time);
		Vec2f yrange = YExtent.getDisplayValue(time);
		float x0 = position[0] - relativeCenter[0];
		float y0 = position[1] - relativeCenter[1];
		float x1 = position[0] + 200 * s - relativeCenter[0];
		float y1 = position[1] - 200 * s - relativeCenter[1];
		if (coord[0] > x0 && coord[0]<x1 && coord[1]>y1 && coord[1] < y0) return true;


		return false;
	}
	virtual Shape* Clone() {
		return new Plotter1D(*this);
	}
	virtual void SetPosition(float time, const Vec2f& coord) {
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));
	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}
	virtual void SetScale(float time, float value) {
		scale.setDisplayValue(std::to_string(value));
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}


	PositionProperty pos;
	PositionProperty XExtent, YExtent;
	ColorProperty color;
	FloatProperty scale, thickness;
	ExprProperty expression;
	BoolProperty axisXVisible, axisYVisible, showGrid;
	IntProperty xticks, yticks;
};



class Grid : public Shape {
public:
	Grid(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", const Vec3u &color = Vec3u(255, 0, 0), bool visible = true, std::string thickness = "1") : Shape("Grid", visible), pos(position), scale(scale), color(color), thickness(thickness), axisXVisible(true), axisYVisible(true), closeX(true), closeY(true), xticks(5), yticks(5), ratio(Expr("1")) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		this->axisXVisible.name = "Horizontal";
		this->axisYVisible.name = "Vertical";
		this->closeX.name = "Horz. closed";
		this->closeY.name = "Vert. closed";
		this->xticks.name = "Nb horz. lines";
		this->yticks.name = "Nb vert. lines";
		this->ratio.name = "Vert/Horz ratio";


		parameters.push_back((Property*)&this->axisXVisible);
		parameters.push_back((Property*)&this->axisYVisible);
		parameters.push_back((Property*)&this->xticks);
		parameters.push_back((Property*)&this->yticks);
		parameters.push_back((Property*)&this->ratio);
		parameters.push_back((Property*)&this->closeX);
		parameters.push_back((Property*)&this->closeY);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);

	}
	Grid(const Grid& d) : Shape(d.shapeType, true), ratio(d.ratio), pos(d.pos), scale(d.scale), color(d.color), thickness(d.thickness), axisXVisible(d.axisXVisible), axisYVisible(d.axisYVisible), xticks(d.xticks), yticks(d.yticks), closeX(d.closeX), closeY(d.closeY) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->thickness.setDefaults("Thickness", 0.f, 1000.f, 1.f);
		this->axisXVisible.name = "Horizontal";
		this->axisYVisible.name = "Vertical";
		this->closeX.name = "Horz. closed";
		this->closeY.name = "Vert. closed";
		this->xticks.name = "Nb horz. lines";
		this->yticks.name = "Nb vert. lines";
		this->ratio.name = "Vert/Horz ratio";


		parameters.push_back((Property*)&this->axisXVisible);
		parameters.push_back((Property*)&this->axisYVisible);
		parameters.push_back((Property*)&this->xticks);
		parameters.push_back((Property*)&this->yticks);
		parameters.push_back((Property*)&this->ratio);
		parameters.push_back((Property*)&this->closeX);
		parameters.push_back((Property*)&this->closeY);
		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->thickness);

	}


	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {

		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);


		if (visible.getDisplayValue()) {
			float edgeThick = thickness.getDisplayValue(time)*s;

			cairo_t *cr = canvas.cr;
			cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
			cairo_set_line_width(cr, edgeThick);

			unsigned char black[3] = { 0,0,0 };
			unsigned char lightgrey[3] = { 230,230,230 };


			if (axisYVisible.getDisplayValue() && yticks.getDisplayValue(time) > 0) {
				int starti = 0;
				int endi = yticks.getDisplayValue(time) - 1;
				float xscale = 200 * s / (endi - starti);
				if (!closeY.getDisplayValue()) {
					starti++; endi++;
					xscale = 200 * s / (endi - starti + 2);
				}

				for (int i = starti; i <= endi; i++) {
					drawAntiAliasedLine1(canvas, position[0] - relativeCenter[0] + i * xscale, position[1] - relativeCenter[1], position[0] - relativeCenter[0] + i * xscale, position[1] - 200 * s - relativeCenter[1], Vec3u(color[0], color[1], color[2]), edgeThick);
				}
			}

			if (axisXVisible.getDisplayValue() && xticks.getDisplayValue(time) > 0) {
				int starti = 0;
				int endi = xticks.getDisplayValue(time) - 1;
				float yscale = 200 * s / (endi - starti)*ratio.getDisplayValue(time);
				if (!closeX.getDisplayValue()) {
					starti++; endi++;
					yscale = 200 * s / (endi - starti + 2);
				}

				for (int i = starti; i <= endi; i++) {
					drawAntiAliasedLine1(canvas, position[0] - relativeCenter[0], position[1] - relativeCenter[1] - i * yscale, position[0] + 200 * s - relativeCenter[0], position[1] - relativeCenter[1] - i * yscale, Vec3u(color[0], color[1], color[2]), edgeThick);
				}
			}
		}

		if (displayControls) {
			drawBox(canvas, position[0] - relativeCenter[0], position[1] - 200 * s - relativeCenter[1], position[0] + 200 * s - relativeCenter[0], position[1] - relativeCenter[1]);
		}
	}

	virtual bool Contains(const Vec2f& coord, float time) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);
		float x0 = position[0] - relativeCenter[0];
		float y0 = position[1] - relativeCenter[1];
		float x1 = position[0] + 200 * s - relativeCenter[0];
		float y1 = position[1] - 200 * s - relativeCenter[1];
		if (coord[0] > x0 && coord[0]<x1 && coord[1]>y1 && coord[1] < y0) return true;

		return false;
	}
	virtual Shape* Clone() {
		return new Grid(*this);
	}
	virtual void SetPosition(float time, const Vec2f& coord) {
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));
	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}
	virtual void SetScale(float time, float value) {
		scale.setDisplayValue(std::to_string(value));
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}


	PositionProperty pos;
	ColorProperty color;
	FloatProperty scale, thickness, ratio;
	BoolProperty axisXVisible, axisYVisible, closeX, closeY;
	IntProperty xticks, yticks;
};


class PointSet : public Shape {
public:
	PointSet(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", const Vec3u &color = Vec3u(255, 0, 0), bool visible = true, std::string pointSize = "1") : Shape("PointSet", visible), pos(position), scale(scale), color(color), dim1(0), dim2(1), dim3(-1), pointSize(Expr("5")), rotX(Expr("0")), rotY(Expr("0")), rotZ(Expr("0")), showBoundary(true) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->pointSize.setDefaults("Point size", 0.f, 1000.f, 1.f);
		this->showBoundary.name = "Show domain boundary";
		this->dim1.setDefaults("Dimension 1", 0, 10000);
		this->dim2.setDefaults("Dimension 2", 0, 10000);
		this->dim3.setDefaults("Dimension 3", -1, 10000);
		this->rotX.setDefaults("Rot. X", -3600, 3600, 0.01);
		this->rotY.setDefaults("Rot. Y", -3600, 3600, 0.01);
		this->rotZ.setDefaults("Rot. Z", -3600, 3600, 0.01);

		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->pointset);
		parameters.push_back((Property*)&this->dim1);
		parameters.push_back((Property*)&this->dim2);
		parameters.push_back((Property*)&this->dim3);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->pointSize);
		parameters.push_back((Property*)&this->rotX);
		parameters.push_back((Property*)&this->rotY);
		parameters.push_back((Property*)&this->rotZ);
		parameters.push_back((Property*)&this->showBoundary);

	}
	PointSet(const PointSet& d) : Shape(d.shapeType, true), pos(d.pos), scale(d.scale), color(d.color), pointSize(d.pointSize), showBoundary(d.showBoundary), pointset(d.pointset), dim1(d.dim1), dim2(d.dim2), dim3(d.dim3), rotX(d.rotX), rotY(d.rotY), rotZ(d.rotZ) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		this->pointSize.setDefaults("Point size", 0.f, 1000.f, 1.f);
		this->showBoundary.name = "Show domain boundary";
		this->dim1.setDefaults("Dimension 1", 0, 10000);
		this->dim2.setDefaults("Dimension 2", 0, 10000);
		this->dim3.setDefaults("Dimension 3", -1, 10000);
		this->rotX.setDefaults("Rot. X", -3600, 3600, 0.01);
		this->rotY.setDefaults("Rot. Y", -3600, 3600, 0.01);
		this->rotZ.setDefaults("Rot. Z", -3600, 3600, 0.01);


		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->pointset);
		parameters.push_back((Property*)&this->dim1);
		parameters.push_back((Property*)&this->dim2);
		parameters.push_back((Property*)&this->dim3);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->pointSize);
		parameters.push_back((Property*)&this->rotX);
		parameters.push_back((Property*)&this->rotY);
		parameters.push_back((Property*)&this->rotZ);
		parameters.push_back((Property*)&this->showBoundary);

	}

	void getBBox(float time, float &minX, float &minY, float &maxX, float &maxY) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Points pts = pointset.getDisplayValue(time);
		if (pts.dim == 0) return;
		int d1 = dim1.getDisplayValue(time);
		int d2 = dim2.getDisplayValue(time);
		minX = 1E9;
		minY = 1E9;
		maxX = -1E9;
		maxY = -1E9;

		for (std::map<float, Points>::const_iterator it = pointset.points.values.begin(); it != pointset.points.values.end(); ++it) {
			for (int i = 0; i < it->second.npoints(); i++) {
				float v1 = it->second.coords[i*it->second.dim + d1];
				float v2 = -it->second.coords[i*it->second.dim + d2];
				minX = std::min(minX, v1);
				maxX = std::max(maxX, v1);
				minY = std::min(minY, v2);
				maxY = std::max(maxY, v2);
			}
		}
		for (int i = 0; i < pts.npoints(); i++) {
			float v1 = pts.coords[i*pts.dim + d1];
			float v2 = -pts.coords[i*pts.dim + d2];
			minX = std::min(minX, v1);
			maxX = std::max(maxX, v1);
			minY = std::min(minY, v2);
			maxY = std::max(maxY, v2);
		}
	}

	Vec3f transform(const Vec3f& p) const {
		//return Vec2f(p[0] / p[2], p[1] / p[2]);
		return rotateZ(rotateY(rotateX(p, rx), ry), rz);
		//return Vec2f(rot[0], rot[1]);
	}

	void getBBoxProj(float time, float &minX, float &minY, float &maxX, float &maxY) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Points pts = pointset.getDisplayValue(time);
		if (pts.dim == 0) return;
		int d1 = dim1.getDisplayValue(time);
		int d2 = dim2.getDisplayValue(time);
		int d3 = dim3.getDisplayValue(time);
		minX = 1E9;
		minY = 1E9;
		maxX = -1E9;
		maxY = -1E9;

		for (std::map<float, Points>::const_iterator it = pointset.points.values.begin(); it != pointset.points.values.end(); ++it) {
			for (int i = 0; i < it->second.npoints(); i++) {
				float v1 = it->second.coords[i*it->second.dim + d1];
				float v2 = -it->second.coords[i*it->second.dim + d2];
				float v3 = 0;
				if (d3>=0)
					v3 = it->second.coords[i*it->second.dim + d3];
				Vec3f p = transform(Vec3f(v1, v2, v3));
				minX = std::min(minX, p[0]);
				maxX = std::max(maxX, p[0]);
				minY = std::min(minY, p[1]);
				maxY = std::max(maxY, p[1]);
			}
		}
		for (int i = 0; i < pts.npoints(); i++) {
			float v1 = pts.coords[i*pts.dim + d1];
			float v2 = -pts.coords[i*pts.dim + d2];
			float v3 = 0;
			if (d3 >= 0)
				v3 = pts.coords[i*pts.dim + d3];
			Vec3f p = transform(Vec3f(v1, v2, v3));
			minX = std::min(minX, p[0]);
			maxX = std::max(maxX, p[0]);
			minY = std::min(minY, p[1]);
			maxY = std::max(maxY, p[1]);
		}
	}

	void getBBox3(float time, float &minX, float &minY, float &minZ, float &maxX, float &maxY, float &maxZ) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Points pts = pointset.getDisplayValue(time);
		if (pts.dim == 0) return;
		int d1 = dim1.getDisplayValue(time);
		int d2 = dim2.getDisplayValue(time);
		int d3 = dim3.getDisplayValue(time);
		minX = 1E9;
		minY = 1E9;
		maxX = -1E9;
		maxY = -1E9;

		for (std::map<float, Points>::const_iterator it = pointset.points.values.begin(); it != pointset.points.values.end(); ++it) {
			for (int i = 0; i < it->second.npoints(); i++) {
				float v1 = it->second.coords[i*it->second.dim + d1];
				float v2 = -it->second.coords[i*it->second.dim + d2];
				float v3 = 0;
				if (d3 >= 0)
					v3 = it->second.coords[i*it->second.dim + d3];
				minX = std::min(minX, v1);
				maxX = std::max(maxX, v1);
				minY = std::min(minY, v2);
				maxY = std::max(maxY, v2);
				minZ = std::min(minZ, v3);
				maxZ = std::max(maxZ, v3);

			}
		}
		for (int i = 0; i < pts.npoints(); i++) {
			float v1 = pts.coords[i*pts.dim + d1];
			float v2 = -pts.coords[i*pts.dim + d2];
			float v3 = 0;
			if (d3 >= 0)
				v3 = pts.coords[i*pts.dim + d3];
			minX = std::min(minX, v1);
			maxX = std::max(maxX, v1);
			minY = std::min(minY, v2);
			maxY = std::max(maxY, v2);
			minZ = std::min(minZ, v3);
			maxZ = std::max(maxZ, v3);
		}
	}

	mutable float rx, ry, rz;

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {

		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);		

		if (visible.getDisplayValue()) {

			float diskSize = pointSize.getDisplayValue(time)*s;
			std::string coltext = "";
			if (this->color.colorPicker) coltext = this->color.colorPicker->GetTextCtrl()->GetValue().ToStdString(); // to debug

			int d1 = dim1.getDisplayValue(time);
			int d2 = dim2.getDisplayValue(time);
			int d3 = dim3.getDisplayValue(time);
			Points pts = pointset.getDisplayValue(time);
			//reLoad(time);
			if (pts.coords.size() > 0) {				
				    rx = rotX.getDisplayValue(time);
					ry = rotY.getDisplayValue(time);
					rz = rotZ.getDisplayValue(time);
					float minX = 1E9, minY = 1E9, maxX = -1E9, maxY = -1E9, minZ = 1E9, maxZ=-1E9;
					getBBox3(time, minX, minY, minZ, maxX, maxY, maxZ);
					
					float maxSize = std::max(std::max(maxX - minX, std::abs(maxY - minY)), maxZ-minZ) ;
					float cX = (minX + maxX)*0.5f;
					float cY = (minY + maxY)*0.5f;
					float cZ = (minZ + maxZ)*0.5f;

					cairo_t *cr = canvas.cr;

					for (int i = 0; i < pts.npoints(); i++) {
						if (coltext == "id")
							cairo_set_source_rgb(cr, ((i*1234+456)%256) / 255.f, ((i * 3123 + 252) % 256) / 255.f, ((i * 4123 + 56) % 256) / 255.f);
						else
							cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
						float v1 = pts.coords[i*pts.dim + d1];
						float v2 = -pts.coords[i*pts.dim + d2];
						float v3 = 0;
						if (d3>=0) v3 = pts.coords[i*pts.dim + d3];
						v1 -= cX;
						v2 -= cY;
						v3 -= cZ;
						Vec3f p = transform(Vec3f(v1, v2, v3));

						p[0] = (p[0]/maxSize+0.5)* 200.f*s;
						p[1] = (p[1]/ maxSize+0.5) * 200.f*s;
						cairo_arc(cr, p[0] + position[0] - relativeCenter[0], p[1] + position[1] + relativeCenter[1], diskSize, 0, 2 * M_PI);
						cairo_fill(cr);
					}
			}
		}
		if (displayControls) {
			drawBox(canvas, position[0] - relativeCenter[0], position[1] - 200 * s - relativeCenter[1], position[0] + 200 * s - relativeCenter[0], position[1] - relativeCenter[1]);
		}
	}

	virtual bool Contains(const Vec2f& coord, float time) const {
		Vec2f position = pos.getDisplayValue(time);
		float s = scale.getDisplayValue(time);
		Vec2f relativeCenter = Vec2f(100, -100)*s;
		Vec3u color = this->color.getDisplayValue(time);
		float x0 = position[0] - relativeCenter[0];
		float y0 = position[1] - relativeCenter[1];
		float x1 = position[0] + 200 * s - relativeCenter[0];
		float y1 = position[1] - 200 * s - relativeCenter[1];
		if (coord[0] > x0 && coord[0]<x1 && coord[1]>y1 && coord[1] < y0) return true;

		return false;
	}
	virtual Shape* Clone() {
		return new PointSet(*this);
	}
	virtual void SetPosition(float time, const Vec2f& coord) {
		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));
	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}
	virtual void SetScale(float time, float value) {
		scale.setDisplayValue(std::to_string(value));
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}


	PositionProperty pos;
	ColorProperty color;
	FloatProperty scale, pointSize, rotX, rotY, rotZ;
	BoolProperty showBoundary;
	IntProperty dim1, dim2, dim3;
	PointSetProperty pointset;
	//mutable std::vector<float> coords;
	//mutable int ndims;
};


class Latex : public Shape {
public:
	Latex(const Vec2s& position = Vec2s("0", "0"), std::string scale = "1", const Vec3u &color = Vec3u(255, 0, 0), bool visible = true) :Shape("Latex", visible), pos(position), scale(scale), color(color), text(to_latex("Equation:\n$$\\sqrt{1+x}$$")) {

		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);
		image = NULL;

		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->text);
	}

	Latex(const Latex& d) : Shape(d.shapeType, true), pos(d.pos), scale(d.scale), color(d.color), text(d.text), image(NULL) {
		this->scale.setDefaults("Scale", 0.f, 10000.f, 0.05f);

		parameters.push_back((Property*)&this->pos);
		parameters.push_back((Property*)&this->scale);
		parameters.push_back((Property*)&this->color);
		parameters.push_back((Property*)&this->text);

	}

	~Latex() {
		nsvgDelete(image);
	}

	std::string to_latex(std::string s) const {
		std::string result = "\\documentclass{article}\n\\usepackage[utf8]{inputenc}\n\\thispagestyle{empty}\n\\begin{document}\n" + s + "\n\\end{document}";
		return result;
	}

	void parse(float time) const {

		if (image) nsvgDelete(image);
		//std::string st = to_latex("\\sqrt{5+2x}");
		std::string st = text.getDisplayValue(time);

		FILE* f = fopen("tmp_123456.tex", "w+");
		fprintf(f, "%s", st.c_str());
		fclose(f);

		wxFileName fn(wxStandardPaths::Get().GetExecutablePath());
		wxString appPath(fn.GetPath());
		system("latex tmp_123456.tex & dvisvgm -n tmp_123456.dvi");


		image = nsvgParseFromFile("tmp_123456.svg", "px", 96);
		//printf("size: %f x %f\n", image->width, image->height);
	}

	virtual void Draw(Canvas& canvas, float time, bool displayControls) const {
		float s = scale.getDisplayValue(time);
		Vec2f p = pos.getDisplayValue(time);
		Vec3u color = this->color.getDisplayValue(time);

		if (visible.getDisplayValue()) {
			std::string txt = text.getDisplayValue(time);
			if (txt != lastText) {
				parse(time);
				lastText = txt;
			}

			cairo_t *cr = canvas.cr;
			cairo_set_source_rgb(cr, color[2] / 255.f, color[1] / 255.f, color[0] / 255.f);
			cairo_set_line_width(cr, 1);

			for (auto shape = image->shapes; shape != NULL; shape = shape->next) {
				for (auto path = shape->paths; path != NULL; path = path->next) {
					float* pt = &path->pts[0];
					cairo_move_to(cr, pt[0] * s + p[0], pt[1] * s + p[1]);
					for (int i = 0; i < path->npts - 3; i += 3) {
						float* pt = &path->pts[i * 2];
						cairo_curve_to(cr, pt[2] * s + p[0], pt[3] * s + p[1], pt[4] * s + p[0], pt[5] * s + p[1], pt[6] * s + p[0], pt[7] * s + p[1]);
					}
					if (path->closed)
						cairo_close_path(cr);
				}
				cairo_fill(cr);
			}

		}

		if (displayControls) { // draw Bbox
			//drawBox(canvas, p[0] - r, p[1] - r, p[0] + r, p[1] + r);
		}

	}

	virtual Shape* Clone() {
		return new Latex(*this);
	}

	virtual bool Contains(const Vec2f& coord, float time) const {
		return true;
	}

	virtual void SetPosition(float time, const Vec2f& coord) {

		pos.setDisplayValue(Vec2s(std::to_string(coord[0]), std::to_string(coord[1])));  // NEED TO CHECK WHY I DID ABOVE

	}
	virtual Vec2f GetPosition(float time) {
		return pos.getDisplayValue(time);
	}

	virtual void SetScale(float time, float value) {
		scale.setDisplayValue(Expr(std::to_string(value)));  // NEED TO CHECK WHY I DID ABOVE
	}
	virtual float GetScale(float time) {
		return scale.getDisplayValue(time);
	}

	mutable struct NSVGimage* image;
	mutable std::string lastText;
	PositionProperty pos;
	ColorProperty color;
	FloatProperty scale;
	TextAreaProperty text;
};
