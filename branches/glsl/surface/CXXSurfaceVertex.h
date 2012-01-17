/*
 *  CXXSurfaceVertex.h
 *  CXXSurface
 *
 *  Created by Martin Noble on Fri Jan 23 2004.
 *  Copyright (c) 2004 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef CXXSurfaceVertex_included
#define CXXSurfaceVertex_included
#include "CXXCoord.h"
#include <vector>
#include <deque>
#include "CXXAlloc.h"

using namespace std;

class CXXSurfaceVertex {
private:
	vector<void *, CXX::CXXAlloc<void *> > pointers;
	vector<CXXCoord, CXX::CXXAlloc<CXXCoord> > vectors;
	vector<double, CXX::CXXAlloc<double> > scalars;
	void init();
	static CXX::CXXAlloc<CXXSurfaceVertex> allocator;
public:
	CXXSurfaceVertex() {init();};
	
	int setXyz(unsigned int coordType, double *xyz);
	int setXyzr(unsigned int coordType, double *xyzr);
	int setCoord(unsigned int coordType, const CXXCoord &crd);
	int setScalar(unsigned int scalarType, double value);
	int setPointer(unsigned int pointerType, void *value);

	double x(unsigned int coordType);
	double y(unsigned int coordType);
	double z(unsigned int coordType);
	double r(unsigned int coordType);
	CXXCoord_ftype *xyzPntr(unsigned int coordType);
	int getXyz(unsigned int coordType, double *x);
	int getXyzr(unsigned int coordType, double *x);
	double scalar(unsigned int scalarType) const;
	void *pointer(unsigned int pointerType) const;
	const CXXCoord *coordPntr(unsigned int coordType) const;
	const CXXCoord &coordRef(unsigned int coordType) const;	
	int nPointers() const;
	int nVectors() const;
	int nScalars() const;
	static void *operator new(size_t nObjects) {
		return allocator.allocate(nObjects, 0);
	};
	static void operator delete(void *pntr, size_t objectSize=0) {
		allocator.deallocate(static_cast<CXXSurfaceVertex *>(pntr), 0);
	};
};
#endif




