/************************************************************************		
\link	www.twinklingstar.cn
\author Twinkling Star
\date	2014/04/06
****************************************************************************/
#ifndef	SR_ALGORITHM_CREATE_OBB_H_
#define SR_ALGORITHM_CREATE_OBB_H_

#include "SrAlgorithmQuickHull3D.h"
/** \addtogroup algorithms
  @{
*/

class SrAlgorithmCreateOBB
{
public:
	static void createOBB(tHull* hull , SrVector3* axis, SrVector3& halfLength, SrPoint3D& center);

};

/** @} */
#endif
