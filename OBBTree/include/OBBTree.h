#ifndef OBBTREE_INCLUDE_OBBTREE_H
#define OBBTREE_INCLUDE_OBBTREE_H

#include <Eigen/Dense>

#define NAMESPACE_BEGAIN(name) namespace name {
#define NAMESPACE_END(name) }

NAMESPACE_BEGAIN(CollisionAlgorithm)

using NormalType = Eigen::Vector3d;

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

	// TODO: need static class function for intersecting between obb1 and obb2

	inline NormalType axis_x() const;

	inline NormalType axis_y() const;

	inline NormalType axis_z() const;

	inline PointType minp() const;

	inline PointType maxp() const;

	inline PointType center() const;

	inline PointType extent() const;

	// TODO: need member function for intersecting between this and obb

	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
	NormalType _axis_x, _axis_y, _axis_z;
	PointType _minp, _maxp;
};

NAMESPACE_END(CollisionAlgorithm)

#endif //OBBTREE_INCLUDE_OBBTREE_H
