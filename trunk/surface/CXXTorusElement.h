/* 
 * 
 * Copyright 2004 by The University of Oxford
 * Author: Martin Noble, Jan Gruber
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */
/*
 *  CXXTorusElement.h
 *  CXXSurface
 *
 *  Created by Martin Noble on Mon Feb 09 2004.
 *  
 *
 */
#ifndef CXXTorusElement_included
#define CXXTorusElement_included
//#include <CXXTorusTriangle.h>
#include "CXXTorusNode.h"
#include "CXXTriangle.h"
#include "CXXCircleNode.h"

#include <vector>
#include <list>

class CXXSurface;
class CXXTriangle;
class CXXCircle;
#include "CXXCoord.h"

using std::vector;
 
class CXXTorusElement {
private:
	static CXXCircle nullCircle;
	const CXXCircle &theCircle;
	vector <CXXTorusNode> nodes;
	vector <CXXTriangle> flatTriangles;
	//omega1 and omega2 are the limits of the arc on the surface of a torus
	//	that is in the plane of the axis of the torus
	double omega1;
	double omega2;
	//When triangulated, the torus is divided up into 2**N (N>=0) in omega and
	//theta.  The corresponding angular step sizes are stored here
	int nOmegaSteps;
	double deltaOmega;
	int nThetaSteps;
	double deltaTheta;
	//absoluteStartOmega is the value of the omega angle of the start
	//of this arc, defined with respect to the referenceUnitVector of the
	//parent circle
	double absoluteStartOmega;
	//theta1 and theta2 are the limits of the arc on the surface of a torus
	//	that is in the plane of the axis of the torus
	double theta1;
	double theta2;
	//Here a set of triangles that point into the flatTriangles array to identify
	//triangles that constitute the edge strip (i.e. last step in theta
	list<int> edgeTriangles;
	// v1unit and v2unit are unit vectors from the centre of the circle that defines
	//the trajectory of the probe around a torus, to the start and end point around that
	//orbit
	CXXCoord v1unit;
	CXXCoord v2unit;
	CXXCoord n1unit;
	//torusUnit and torusCentre are the axis and the centre of the torus respectively
	CXXCoord torusAxis;
	CXXCoord torusCentre;
	//rProbe and rTraj are the two radiuses that define the shape of the torus
	double rProbe, rTraj;
	void init();
	int debug;
	
	CXXCoord *pointyBit;
public:
		CXXTorusElement();
	~CXXTorusElement();
	CXXTorusElement(const CXXCircle &aCircle, int iEdge, double delta, double probeRadius);
	void deleteLastTriangle(void);
	int addNode(CXXTorusNode &aNode);
	const int numberOfTorusNodes(void);
	const CXXCoord coordFromThetaOmega(double theta, double omega) const;
	const CXXCoord normalToProbeAtTheta(CXXCoord &p, double theta) const;
	const CXXCoord probeAtOmega(double omega) const;
	const CXXTorusNode &getNode(const int i) const;
	int upload(CXXSurface *aSurface);
	void addEdgeVertex(CXXCircleNode &aNode);
	int getNOmegaSteps() const {
		return nOmegaSteps;
	};
	double getDeltaOmega() const {
		return deltaOmega; 
	};
	double getAbsoluteStartOmega() const {
		return absoluteStartOmega;
	};
	const CXXCircle &getCircle() const {
		return theCircle;
	};
	double getTheta2() const {
		return theta2;
	};
	const CXXCoord *getPointyBit() const {
		return pointyBit;
	};
	
};

#endif
