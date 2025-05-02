#include "VirtualHelper.h"

using namespace VirtualBenchmark;

void Circle::Move(double x, double y)
{
    move += 2.0 * (x + y);
}

void Circle::Draw()
{
    draw += 1.0;
}

void Rectangle::Move(double x, double y)
{
    move += x + y;
}

void Rectangle::Draw()
{
    draw += 1.0;
}