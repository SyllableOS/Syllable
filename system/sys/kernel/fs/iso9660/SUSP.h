#ifndef _SUSP_H
#define _SUSP_H

#define SUSP_TAG(a,b)	(((a)<<8)+(b))

enum RRIPSystemUseEntry
{
	POSIXFileAttributes = SUSP_TAG( 'P', 'X' ),
	POSIXDeviceNumber = SUSP_TAG( 'P', 'N' ),
	SymbolicLink = SUSP_TAG( 'S', 'L' ),
	AlternateName = SUSP_TAG( 'N', 'M' ),
	ChildLink = SUSP_TAG( 'C', 'L' ),
	ParentLink = SUSP_TAG( 'P', 'L' ),
	RelocatedDirectory = SUSP_TAG( 'R', 'E' ),
	TimeStamp = SUSP_TAG( 'T', 'F' ),
	SparseFile = SUSP_TAG( 'S', 'F' )
};


enum SUSPSystenUseEntry
{
	ContinuationArea = SUSP_TAG( 'C', 'E' ),
	PaddingField = SUSP_TAG( 'P', 'D' ),
	SUSPIndicator = SUSP_TAG( 'S', 'P' ),
	SUSPTerminator = SUSP_TAG( 'S', 'T' ),
	ExtensionReference = SUSP_TAG( 'E', 'R' ),
	ExtensionSelector = SUSP_TAG( 'E', 'S' )
};

#endif //_SUSP_H
