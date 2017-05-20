#ifndef OBBTREE_INCLUDE_OBBTREE_H
#define OBBTREE_INCLUDE_OBBTREE_H

#include <Eigen/Dense>

#ifndef NAMESPACE_BEGAIN

#define NAMESPACE_BEGAIN(name) namespace name {
#define NAMESPACE_END(name) }

#endif // !NAMESPACE_BEGAIN

NAMESPACE_BEGAIN(CollisionAlgorithm)

using PointType = Eigen::Vector3d;

using NormalType = Eigen::Vector3d;

class OBB
{
public:

	OBB() = default;

	OBB(const OBB& obb) = default;

	OBB& operator=(const OBB& obb) = default;

	OBB(const PointType& axis_x, const PointType& axis_y, const PointType& axis_z,
		const PointType& center, const PointType& extent) noexcept;

	inline PointType center() const { return _center; }

	inline PointType extent() const { return _extent; }

	inline PointType axis_x() const { return _axis_x; }

	inline PointType axis_y() const { return _axis_y; }

	inline PointType axis_z() const { return _axis_z; }

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	PointType _center, _extent;
	NormalType _axis_x, _axis_y, _axis_z;
};

NAMESPACE_END(CollisionAlgorithm)

#endif //OBBTREE_INCLUDE_OBBTREE_H