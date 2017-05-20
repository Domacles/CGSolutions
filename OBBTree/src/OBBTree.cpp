#include "OBBTree.h"

#include <memory>

NAMESPACE_BEGAIN(CollisionAlgorithm)

OBB::OBB(const PointType& axis_x, const PointType& axis_y, const PointType& axis_z,
	const PointType& center, const PointType& extent) noexcept
{
	_axis_x = axis_x, _axis_y = axis_y, _axis_z = axis_z;
	_center = center, _extent = extent;
}

NAMESPACE_END(CollisionAlgorithm)