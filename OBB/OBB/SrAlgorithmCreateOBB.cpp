/************************************************************************		
\link	www.twinklingstar.cn
\author Twinkling Star
\date	2014/04/06
****************************************************************************/
#include "SrAlgorithmCreateOBB.h"
#include "include/gmm/gmm_dense_qr.h"



void SrAlgorithmCreateOBB::createOBB(tHull* hull , SrVector3* axis, SrVector3& halfLength, SrPoint3D& center)
{
	float *		area = new float[hull->numFacet];
	SrPoint3D*	mass = new SrPoint3D[hull->numFacet];
	SrVector3 normal;
	SrPoint3D p0,p1,p2,tmp;
	int i;
	for( i=0 ; i<hull->numFacet ; i++ )
	{
		p0 = hull->vertex[hull->facet[i].vertexIndex[0]];
		p1 = hull->vertex[hull->facet[i].vertexIndex[1]];
		p2 = hull->vertex[hull->facet[i].vertexIndex[2]];

		normal = (p1 - p0).cross(p2 - p0);
		tmp = p0.cross(p1) + p1.cross(p2) + p2.cross(p0);

		area[i] = tmp.dot(normal) / 2;
		mass[i] = (p0 + p1 + p2) / 3;
	}

	float		sumArea = 0.0;
	SrPoint3D	sumMass = SrPoint3D(0,0,0);
	for( i=0 ; i<hull->numFacet ; i++ )
	{
		sumArea += area[i];
		sumMass += area[i]*mass[i];
	}
	sumMass = sumMass/sumArea;
	SrPoint3D t = SrPoint3D(0,0,0);

	float covarMatrx[3][3];
	int j , k;
	for( i=0 ; i<3 ; i++ )
	{
		for( j=i ; j<3 ; j++ )
		{
			covarMatrx[i][j] = covarMatrx[j][i] = 0;
			for( k=0 ; k<hull->numFacet ; k++ )
			{
				p0 = hull->vertex[hull->facet[i].vertexIndex[0]];
				p1 = hull->vertex[hull->facet[i].vertexIndex[1]];
				p2 = hull->vertex[hull->facet[i].vertexIndex[2]];
				covarMatrx[i][j] += area[k] * (9*mass[k][i]*mass[k][j] + p0[i]*p0[j] + p1[i]*p1[j] + p2[i]*p2[j]);
			}
			covarMatrx[i][j] = sumArea * covarMatrx[i][j] / 12.0f + sumMass[i]*sumMass[j];
			covarMatrx[j][i] = covarMatrx[i][j];
		}
	}

	gmm::dense_matrix<double> cMatrix(3,3);

	cMatrix(0,0) = covarMatrx[0][0]; cMatrix(0,1) = covarMatrx[0][1]; cMatrix(0,2) = covarMatrx[0][2];
	cMatrix(1,0) = covarMatrx[1][0]; cMatrix(1,1) = covarMatrx[1][1]; cMatrix(1,2) = covarMatrx[1][2];
	cMatrix(2,0) = covarMatrx[2][0]; cMatrix(2,1) = covarMatrx[2][1]; cMatrix(2,2) = covarMatrx[2][2];

	gmm::dense_matrix<double>	eigvec(3,3);
	std::vector<double>			eigval(3);

	gmm::symmetric_qr_algorithm( cMatrix, eigval, eigvec );

	// find the right, up and forward vectors from the eigenvectors
	SrVector3 x( (float)eigvec(0,0), (float)eigvec(1,0), (float)eigvec(2,0) );
	SrVector3 y( (float)eigvec(0,1), (float)eigvec(1,1), (float)eigvec(2,1) );
	SrVector3 z( (float)eigvec(0,2), (float)eigvec(1,2), (float)eigvec(2,2) );
	x.normalize(); y.normalize();z.normalize();

	SrVector3 minLen , maxLen;
	SrVector3 tmpLen;
	minLen.set(x.dot(hull->vertex[0]),y.dot(hull->vertex[0]),z.dot(hull->vertex[0]));
	maxLen = minLen;
	for( i = 1 ; i<hull->numVertex ; i++ )
	{
		tmpLen.set(x.dot(hull->vertex[i]),y.dot(hull->vertex[i]),z.dot(hull->vertex[i]));
		minLen.min(tmpLen);
		maxLen.max(tmpLen);
	}


	axis[0] = x;
	axis[1] = y;
	axis[2] = z;

	halfLength[0] = (maxLen.x - minLen.x)/2;
	halfLength[1] = (maxLen.y - minLen.y)/2;
	halfLength[2] = (maxLen.z - minLen.z)/2;

	center = ((minLen.x + maxLen.x)/2)*x + ((minLen.y + maxLen.y)/2)*y + ((minLen.z + maxLen.z)/2)*z;

	delete []area;
	delete []mass;

}