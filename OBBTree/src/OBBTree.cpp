#include "OBBTree.h"

#include <memory>

NAMESPACE_BEGAIN(CollisionAlgorithm)

OBB::OBB(const OBB&& obb) noexcept
{
	using namespace std;
	_axis_x = move(obb._axis_x), _axis_y = move(obb._axis_y), _axis_z = move(obb._axis_z);
	_minp = move(obb._minp);
}

OBB& OBB::operator=(const OBB&& obb)
{
	return *this;
}

OBB::OBB(const PointType& axis_x, const PointType& axis_y, const PointType& axis_z,
	const PointType& minp, const PointType& maxp) noexcept
{
	_minp = minp, _maxp = maxp;
}

NAMESPACE_END(CollisionAlgorithm)