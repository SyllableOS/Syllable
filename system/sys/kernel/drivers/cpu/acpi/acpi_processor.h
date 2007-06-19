#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#define ACPI_PROCESSOR_MAX_C2_LATENCY	100
#define ACPI_PROCESSOR_MAX_C3_LATENCY	1000


typedef struct {
	uint8 nDescriptor;
	uint16 nLength;
	uint8 nSpaceId;
	uint8 nBitWidth;
	uint8 nBitOffset;
	uint8 nReserved;
	uint64 nAddress;
} __attribute__ ((packed)) ACPIPowerRegister_s;


typedef struct {
	uint8 nDescriptor;
	uint16 nLength;
	uint8 nSpaceId;
	uint8 nBitWidth;
	uint8 nBitOffset;
	uint8 nReserved;
	uint64 nAddress;
} __attribute__ ((packed)) ACPIPctRegister_s;

typedef struct {
	acpi_integer nCoreFreq;
	acpi_integer nPower;
	acpi_integer nTransLatency;
	acpi_integer nBusMasterLatency;
	acpi_integer nControl;
	acpi_integer nStatus;
} __attribute__ ((packed)) ACPIPerfState_s;

#define ACPI_MAX_C_STATES 8

typedef struct {
	int nIndex;
	int nType;
	uint32 nLatency;
	uint32 nAddr;
	uint32 nUsage;
} ACPICState_s;

typedef struct
{
	acpi_handle hHandle;
	int nId;
	int nAcpiId;
	uint32 nCPU;
	uint32 nCPUModel;
	bool bConstantTSC;
	bool bBMCheck;
	acpi_io_address nPblkAddr;
	bool bSpeedStep;
	
	int nCStateCount;
	ACPICState_s asCStates[ACPI_MAX_C_STATES];
	int nCurrentCState;
	int nPromotionCount;
	uint32 nBMHistory;
	atomic_t nC3CpuCount;
	bigtime_t nLastBMCheck;	
	
	int nPStateCount;
	ACPIPctRegister_s sPCtrlReg;
	ACPIPctRegister_s sPStsReg;
	ACPIPerfState_s* apsPStates;
	uint64 nOrigCoreSpeed;
	uint32 nOrigDelayCount;
	uint64 nCurrentFreq;
} ACPIProc_s;

extern ACPI_bus_s* g_psBus;

void acpi_get_cstate_support( ACPIProc_s* psProc );
void acpi_get_pstate_support( ACPIProc_s* psProc );
void acpi_get_freq_support( ACPIProc_s* psProc );
status_t acpi_set_global_pstate( int nIndex );


#endif


















