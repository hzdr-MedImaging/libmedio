/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef SCANNER_H
#define SCANNER_H

/*
 * This file describes the hardware components of the used PET scanner
 *
 * In fact this file describes the components of the ECAT EXACT HR+
 * scanner used at the PET center of Rossendorf, Germany.
*/

// normally math.h should have a PI define but we can't rely on it
#if !defined(PI)
#define PI	3.14159265358979323846264338327950288
#endif
#if !defined(FPIHALF)
#define FPIHALF	(2*PI+PI/2)
#endif

///////////////////////////////////////////////////////////////////////
// Hardware constants of scanner (ECAT EXACT HR+ 962)
#define SCANNER_RINGS								4		// number of detector rings			(4)
#define SCANNER_CRYSTALS_PER_RING		576	// number of detectors per ring (576 = 72x8)
#define SCANNER_CRYSTALS_PER_BLOCK	8		// number of crystals per block	(8)
#define SCANNER_CRYSTALS_AXIAL			8		// number of axial crystals			(8)
#define SCANNER_CRYSTALS_ANGULAR		8		// number of anguar crystals		(8)
#define SCANNER_BLOCKS_PER_BUCKET		3		// number of blocks per bucket	(3)
#define SCANNER_2D_PLANES						63	// number of planes in 2D mode	(32 direct + 31 indirect)

#define SCANNER_CRYSTALS_PER_BUCKET ((SCANNER_BLOCKS_PER_BUCKET)*(SCANNER_CRYSTALS_PER_BLOCK)) // number of crystals per bucket	(24)
#define SCANNER_CRYSTAL_RINGS				((SCANNER_RINGS)*(SCANNER_CRYSTALS_AXIAL))								 // number of crystal rings				(32)
#define SCANNER_BLOCKS_PER_RING			((SCANNER_CRYSTALS_PER_RING)/(SCANNER_CRYSTALS_PER_BLOCK)) // number blocks per ring				(72)
#define SCANNER_BUCKETS_PER_RING		((SCANNER_BLOCKS_PER_RING)/(SCANNER_BLOCKS_PER_BUCKET))		 // number buckets per ring				(24)
#define SCANNER_BUCKETS							((SCANNER_BUCKETS_PER_RING)*(SCANNER_RINGS))							 // number of buckets							(96)

#define SCANNER_PLANE_DEPTH					2.425														// physical depth of one plane (mm)
#define SCANNER_RING_DEPTH					(SCANNER_PLANE_DEPTH*2.0)				// depth of a single ring (mm)  
#define SCANNER_RING_RADIUS					412.5														// radius of scanner (mm); normally 412.5 but CTI's own software uses 419
#define SCANNER_RING_Z0							-77.6														// z0 location of first ring (mm)   
#define SCANNER_FOV_ANGLE						(PI/4.0)												// ratio of FOV radius to DEF_R_RING

// number of elements defaults         
#define SCANNER_MAX_ANGLES					288	// number of angles with no mash
#define SCANNER_MAX_ELEMENTS				288 // number of elements (bins)
#define SCANNER_MAX_RINGDIFF				22	// maximum allowed ring difference
#define SCANNER_3D_SPAN							9		// span 9 is the default for 3D measurement

#define SCANNER_MAX_BINWIDTH				(0.93*SCANNER_RING_RADIUS*sin(2.0*SCANNER_FOV_ANGLE/SCANNER_MAX_ELEMENTS))
#define SCANNER_DANGLE							((PI*2.0)/SCANNER_CRYSTALS_PER_RING)	// ??? (0.10908)
#define SCANNER_D10									((SCANNER_CRYSTALS_PER_RING*7)/8)			// ??? (504)
#define SCANNER_D20									((SCANNER_CRYSTALS_PER_RING*5)/8)			// ??? (360)

#define SCANNER_X_RESOLUTION				0.224983 // physical width of view sample (cm)

// some helping macros for converting a
// LOR to the corresponding crystal number
#define getCrystalA(re, angle)		(((angle) + (((re)-(SCANNER_MAX_ELEMENTS/2))/2) + 2*SCANNER_MAX_ANGLES) % (2*SCANNER_MAX_ANGLES))
#define getCrystalB(re, angle)		(((angle) - ((((re)-(SCANNER_MAX_ELEMENTS/2))+1)/2) + 3*SCANNER_MAX_ANGLES) % (2*SCANNER_MAX_ANGLES))
#define getBucket(crystal, ring)	((crystal)/SCANNER_CRYSTALS_PER_BUCKET)+SCANNER_BUCKETS_PER_RING*((ring)/SCANNER_CRYSTALS_AXIAL)

#endif /* SCANNER_H */
