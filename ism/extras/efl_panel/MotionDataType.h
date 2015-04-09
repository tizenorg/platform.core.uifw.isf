#ifndef _MOTIONDATATYPE_H
#define _MOTIONDATATYPE_H

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#ifndef MAX
#define MAX(a, b) ( ( (a) > (b) ) ? (a) : (b) )
#endif
#ifndef MIN
#define MIN(a, b) ( ( (a) < (b) ) ? (a) : (b) )
#endif

struct Point2D
{
	int x;
	int y;

	Point2D() : x(0), y(0){};
	Point2D(int x, int y) : x(x), y(y){};
};
struct Point2Df
{
	double x;
	double y;

	Point2Df() : x(0), y(0){};
	Point2Df(double x, double y) : x(x), y(y){};
};
struct Point3Df
{
	double x;
	double y;
	double z;

	Point3Df() : x(0), y(0), z(0){};
	Point3Df(double x, double y, double z) : x(x), y(y), z(z){};
};

#endif //MotionDataType
