/************************************************************************		
\link	www.twinklingstar.cn
\author Twinkling Star
\date	2014/04/03
****************************************************************************/
#ifndef	SR_ALGORITHM_QUICK_HULL_3D_H_
#define SR_ALGORITHM_QUICK_HULL_3D_H_
#include <assert.h>
#include "SrSimpleTypes.h"
#include "SrMath.h"
#include "SrDataType.h"
#include <list>
#include <map>
/** \addtogroup algorithms
  @{
*/


typedef struct
{
	int	vertexIndex[3];
}tFacet;

typedef struct
{
	SrPoint3D*	vertex;
	tFacet*		facet;
	int			numVertex;
	int			numFacet;
}tHull;


class SrAlgorithmQuickHull3D
{
private:

#if _DEBUG
#define  ASSERT(exp) assert((exp))
#else
#define  ASSERT(exp)
#endif

	class cFacet;
	class cEdge;
	class cOutsideSet;
	class cVertex;

	typedef SrPoint3D					Point;
	typedef SrVector3					Vector;

	typedef std::list<cVertex*>			VertexList;
	typedef VertexList::iterator		VertexIterator;

	typedef std::list<cFacet*>			FacetList;
	typedef FacetList::iterator			FacetIterator;


	typedef std::list<cEdge*>			EdgeList;
	typedef EdgeList::iterator			EdgeIterator;

	typedef std::list<cOutsideSet>		OutsideSetList;
	typedef OutsideSetList::iterator	OutsideSetIterator;

	typedef std::map<cVertex*,cEdge*>		BoundaryEdgeMap;
	typedef BoundaryEdgeMap::iterator	BoundaryEdgeIterator;

	class cVertex
	{
	public:
		cVertex()
		{
			mOnHull = false;
			mPoint.x = mPoint.y = mPoint.z = 0.0f;
		}
		~cVertex()
		{
		}
	public:
		bool			mOnHull;			//Indicate whether or not the point is the extreme point of the convex polyhedron.
		Point			mPoint;				//The location information of this vertex.
	};

	class cOutsideSet
	{
	public:
		cOutsideSet(){}
		~cOutsideSet()
		{
			mVertexList.clear();
		}
	public:
		VertexList	mVertexList;			//Container of outside point set.
	};


	class cPlane
	{
	public:
		cPlane(){}
		~cPlane(){}
		bool initPlane(const Point& p0, const Point& p1, const Point& p2)
		{
			normal	= (p1 - p0).cross(p2 - p0);
			if( normal.isZero() )
				return false;
			//It's not necessary to normalize the vector here.
			//normal.normalize();
			d		= -normal.dot(p0);
			return true;
		}
		bool isValid() const
		{
			return !normal.isZero();
		}
		float distance(const Point& point) const
		{
			return normal.dot(point) + d;
		}
		bool isOnPositiveSide(const Point& p) const
		{
			return distance(p)>0.0f;
		}

	public:
		Vector		normal;
		float		d;
	};

#define  FACET_NULL			0x00
#define  FACET_VISITED		0x01
#define  FACET_BORDER		0x02

	class cFacet
	{
	public:
		cFacet()
		{
			mNeighbors[0] = NULL;
			mNeighbors[1] = NULL;
			mNeighbors[2] = NULL;
			mVertex[0]	  = NULL;
			mVertex[1]	  = NULL;
			mVertex[2]	  = NULL;
			mOutsideSet	 = NULL;
			mVisitFlag	= FACET_NULL;
			mIterator	  = NULL;

		}
		void clear()
		{
			mNeighbors[0] = NULL;
			mNeighbors[1] = NULL;
			mNeighbors[2] = NULL;
			mVertex[0]	  = NULL;
			mVertex[1]	  = NULL;
			mVertex[2]	  = NULL;
			mOutsideSet	 = NULL;
			mVisitFlag	= FACET_NULL;

		}
		~cFacet()
		{
			if( mIterator )
			{
				delete mIterator;
				mIterator = NULL;
			}
			if( mOutsideSet )
			{
				delete mOutsideSet;
				mOutsideSet = NULL;
			}
		}
		void		initFace(cVertex* p0, cVertex* p1,cVertex* p2)
		{
			mVertex[0] = p0;
			mVertex[1] = p1;
			mVertex[2] = p2;
		}
		void		setNeighbors(cFacet* f0,cFacet* f1,cFacet* f2)
		{
			mNeighbors[0] = f0;
			mNeighbors[1] = f1;
			mNeighbors[2] = f2;
		}
		const VertexIterator furthestVertex()
		{
			ASSERT(mOutsideSet!=NULL);
			cPlane plane;
			ASSERT(plane.initPlane(mVertex[0]->mPoint,mVertex[1]->mPoint,mVertex[2]->mPoint));
			VertexIterator iter = mOutsideSet->mVertexList.begin();
			VertexIterator iterVertex;
			float maxDist = 0.0f , dist;
			for( ; iter!=mOutsideSet->mVertexList.end();iter++ )
			{
				dist = plane.normal.dot((*iter)->mPoint) + plane.d;
				if(maxDist < dist)
				{
					maxDist = dist;
					iterVertex = iter;
				}
			}

			return iterVertex;
		}
	public:
		cOutsideSet*	mOutsideSet;			//The outside point set.
		cVertex*		mVertex[3];				//The point information of this facet.
		cFacet*			mNeighbors[3];			//Indicate the neighbor facets of this one.
		unsigned char	mVisitFlag;				//Indicate the flag in the process of determining the visible face set.
		FacetIterator*	mIterator;				//Indicate the location in the pending face list.
	};



	class cEdge
	{
	public:
		cEdge()
		{
			point[0] = NULL;
			point[1] = NULL;
			neighbors[0] = NULL;
			neighbors[1] = NULL;
		}
		~cEdge()
		{

		}
		void setEndpoint(cVertex* p0, cVertex* p1)
		{
			point[0] = p0;
			point[1] = p1;
		}
		void setNeighbor( cFacet* f0, cFacet* f1)
		{
			neighbors[0] = f0;
			neighbors[1] = f1;
		}
	public:
		cVertex*		point[2];				//The point information of this edge.
		cFacet*		neighbors[2];				//The neighbor facets of this edge.
	};

public:
	SrAlgorithmQuickHull3D(){}
	bool quickHull(SrPoint3D* points, int numPoint, tHull* resultHull);


private:
	bool collinear(const Point&p0, const Point& p1,const Point& p2);
	bool coplanar(const Point&p0, const Point& p1,const Point& p2,const Point& p3);
	void deallocate(FacetList& ftList);
	void findVisibleFacet(cVertex* furPoint,cFacet* f,FacetList& visibleSet,BoundaryEdgeMap& boudaryMap);
	void constructNewFacets(cVertex* point,FacetList& newFacetList,BoundaryEdgeMap& boundary);
	void determineOutsideSet(FacetList& facetList, VertexList& allVertex);
	void updateFacetPendList(FacetList& facetPendList , FacetList& newFacetList,cFacet*& head);
	void partitionOutsideSet(FacetList& facetPendList,FacetList& newFacetList,VertexList& allVertex,cFacet*& head);
	void gatherOutsideSet(FacetList& facetPendList, FacetList& visFacetList,VertexList& visOutsideSet);
	void quickHullScan(FacetList& facetPendList,cFacet*& head);
	bool initTetrahedron(VertexList& vertexList,FacetList& hull);
	void recursiveExport(cFacet* head,std::map<cVertex*,int>& vMap,int& vertexId,std::list<cFacet*>& fList);
	void exportHull(cFacet* head, tHull* hull);

	int mVisibleNum;
	int mBorderNum;
};

/** @} */
#endif
