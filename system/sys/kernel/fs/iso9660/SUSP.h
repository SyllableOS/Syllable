#ifndef _SUSP_H
#define _SUSP_H


enum RRIPSystemUseEntry
{
	POSIXFileAttributes = 'PX',
	POSIXDeviceNumber = 'PN',
	SymbolicLink = 'SL',
	AlternateName = 'NM',
	ChildLink = 'CL',
	ParentLink = 'PL',
	RelocatedDirectory = 'RE',
	TimeStamp = 'TF',
	SparseFile = 'SF'
};
	

enum SUSPSystenUseEntry
{
	ContinuationArea = 'CE',
	PaddingField = 'PD',
	SUSPIndicator = 'SP',
	SUSPTerminator = 'ST',
	ExtensionReference = 'ER',
	ExtensionSelector = 'ES'
};




#endif //_SUSP_H




