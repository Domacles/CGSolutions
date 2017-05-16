#ifndef OBBTREE_INCLUDE_OBBTREE_H
#define OBBTREE_INCLUDE_OBBTREE_H

#include <Eigen/Dense>

#define NAMESPACE_BEGAIN(name) namespace name {
#define NAMESPACE_END(name) }

NAMESPACE_BEGAIN(CollisionAlgorithm)

using Normal = Eigen::Vector3d;

using PointType = Eigen::Vector3d;

class OBB
{
public:

	OBB() = default;

	OBB(const OBB& obb) = default;

	OBB& operator=(const OBB& obb) = default;

	OBB(const OBB&& obb) noexcept;

	OBB& operator=(const OBB&& obb);

	OBB(const PointType& axis_x, const PointType& axis_y, const PointType& axis_z,
		const PointType& minp, const PointType& maxp) noexcept;

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	Normal _axis_x, _axis_y, _axis_z;
	PointType _minp, _maxp;
};

NAMESPACE_END(CollisionAlgorithm)

#endif //OBBTREE_INCLUDE_OBBTREE_H
