#ifndef COVENLIB_H
#define COVENLIB_H

#include "cbase.h"

void VectorRotate2DPoint(const Vector &in, const Vector &point, float angle, Vector *out, bool bRadians = false);
void PointRotate2DPoint(float &x, float &y, float px, float py, float angle, bool bRadians = false);
void VectorRotate2D(const Vector &in, float angle, Vector *out, bool bRadians = false);
bool LocationIsBetween(const Vector &location, const Vector &low, const Vector &high);
void OrderVectors(Vector &low, Vector &high);
float Hysteresis(float x, float factor = 0.5f, float range = 1.0f);

#endif