#include "cbase.h"
#include "covenlib.h"

void VectorRotate2DPoint(const Vector &in, const Vector &point, float angle, Vector *out, bool bRadians)
{
	if (out)
	{
		if (!bRadians)
			angle = M_PI * angle / 180.0f;
		float cs;
		float sn;
		SinCos(angle, &sn, &cs);

		float translated_x = in.x - point.x;
		float translated_y = in.y - point.y;

		float result_x = translated_x * cs - translated_y * sn;
		float result_y = translated_x * sn + translated_y * cs;

		result_x += point.x;
		result_y += point.y;

		(*out).x = result_x;
		(*out).y = result_y;
	}
}

void VectorRotate2D(const Vector &in, float angle, Vector *out, bool bRadians)
{
	if (out)
	{
		if (!bRadians)
			angle = M_PI * angle / 180.0f;
		float cs;
		float sn;
		SinCos(angle, &sn, &cs);

		(*out).x = in.x * cs - in.y * sn;
		(*out).y = in.x * sn + in.y * cs;
	}
}

//Low and high must be completely ordered!
bool LocationIsBetween(const Vector &location, const Vector &low, const Vector &high)
{
	return location.x >= low.x && location.x <= high.x && location.y >= low.y && location.y <= high.y && location.z >= low.z && location.z <= high.z;
}

//Completely order two vectors
void OrderVectors(Vector &low, Vector &high)
{
	for (int i = 0; i < 3; i++)
	{
		float flLow = min(low[i], high[i]);
		float flHigh = max(low[i], high[i]);
		low[i] = flLow;
		high[i] = flHigh;
	}
}