#include "io2d.h"
#include "xio2dhelpers.h"
#include "xcairoenumhelpers.h"

using namespace std;
using namespace std::experimental::io2d;

void circle::center(const vector_2d& ctr) noexcept {
	_Center = ctr;
}

void circle::radius(double rad) noexcept {
	_Radius = rad;
}