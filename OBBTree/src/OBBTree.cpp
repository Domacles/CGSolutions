#include "OBBTree.h"

#include <memory>

NAMESPACE_BEGAIN(CollisionAlgorithm)

Normal OBB::axis_x() const { return _axis_x; }

Normal OBB::axis_y() const { return _axis_y; }

Normal OBB::axis_z() const { return _axis_z; }

PointType OBB::minp() const { return _minp; }

PointType OBB::maxp() const { return _maxp; }

PointType OBB::extent() const { return _maxp - _minp; }

PointType OBB::center() const { return (_minp + _maxp) / 2.0; }

OBB::OBB(const OBB&& obb) noexcept
{
	using namespace std;
	_axis_x = move(obb._axis_x), _axis_y = move(obb._axis_y), _axis_z = move(obb._axis_z);
	_minp = move(obb._minp), _maxp = move(obb._maxp);
}

OBB& OBB::operator=(const OBB&& obb)
{
	using namespace std;
	_axis_x = move(obb._axis_x), _axis_y = move(obb._axis_y), _axis_z = move(obb._axis_z);
	_minp = move(obb._minp), _maxp = move(obb._maxp);
	return *this;
}

OBB::OBB(const PointType& axis_x, const PointType& axis_y, const PointType& axis_z,
	const PointType& minp, const PointType& maxp) noexcept
{
	_axis_x = axis_x, _axis_y = axis_y, _axis_z = axis_z;
	_minp = minp, _maxp = maxp;
}

NAMESPACE_END(CollisionAlgorithm)