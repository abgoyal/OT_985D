

#ifndef _BUSLOGIC_H
#define _BUSLOGIC_H


#ifndef PACKED
#define PACKED __attribute__((packed))
#endif


#define BusLogic_MaxHostAdapters		16



#define BusLogic_MaxTargetDevices		16



#define BusLogic_ScatterGatherLimit		128



#define BusLogic_MaxTaggedQueueDepth		64
#define BusLogic_MaxAutomaticTaggedQueueDepth	28
#define BusLogic_MinAutomaticTaggedQueueDepth	7
#define BusLogic_TaggedQueueDepthBB		3
#define BusLogic_UntaggedQueueDepth		3
#define BusLogic_UntaggedQueueDepthBB		2



#define BusLogic_DefaultBusSettleTime		2



#define BusLogic_MaxMailboxes			211



#define BusLogic_CCB_AllocationGroupSize	7



#define BusLogic_LineBufferSize			100
#define BusLogic_MessageBufferSize		9700



enum BusLogic_MessageLevel {
	BusLogic_AnnounceLevel = 0,
	BusLogic_InfoLevel = 1,
	BusLogic_NoticeLevel = 2,
	BusLogic_WarningLevel = 3,
	BusLogic_ErrorLevel = 4
};

static char *BusLogic_MessageLevelMap[] = { KERN_NOTICE, KERN_NOTICE, KERN_NOTICE, KERN_WARNING, KERN_ERR };



#define BusLogic_Announce(Format, Arguments...) \
  BusLogic_Message(BusLogic_AnnounceLevel, Format, ##Arguments)

#define BusLogic_Info(Format, Arguments...) \
  BusLogic_Message(BusLogic_InfoLevel, Format, ##Arguments)

#define BusLogic_Notice(Format, Arguments...) \
  BusLogic_Message(BusLogic_NoticeLevel, Format, ##Arguments)

#define BusLogic_Warning(Format, Arguments...) \
  BusLogic_Message(BusLogic_WarningLevel, Format, ##Arguments)

#define BusLogic_Error(Format, Arguments...) \
  BusLogic_Message(BusLogic_ErrorLevel, Format, ##Arguments)



enum BusLogic_HostAdapterType {
	BusLogic_MultiMaster = 1,
	BusLogic_FlashPoint = 2
} PACKED;

#define BusLogic_MultiMasterAddressCount	4
#define BusLogic_FlashPointAddressCount		256

static int BusLogic_HostAdapterAddressCount[3] = { 0, BusLogic_MultiMasterAddressCount, BusLogic_FlashPointAddressCount };



#ifdef CONFIG_SCSI_FLASHPOINT

#define BusLogic_MultiMasterHostAdapterP(HostAdapter) \
  (HostAdapter->HostAdapterType == BusLogic_MultiMaster)

#define BusLogic_FlashPointHostAdapterP(HostAdapter) \
  (HostAdapter->HostAdapterType == BusLogic_FlashPoint)

#else

#define BusLogic_MultiMasterHostAdapterP(HostAdapter) \
  (true)

#define BusLogic_FlashPointHostAdapterP(HostAdapter) \
  (false)

#endif



enum BusLogic_HostAdapterBusType {
	BusLogic_Unknown_Bus = 0,
	BusLogic_ISA_Bus = 1,
	BusLogic_EISA_Bus = 2,
	BusLogic_PCI_Bus = 3,
	BusLogic_VESA_Bus = 4,
	BusLogic_MCA_Bus = 5
} PACKED;

static char *BusLogic_HostAdapterBusNames[] = { "Unknown", "ISA", "EISA", "PCI", "VESA", "MCA" };

static enum BusLogic_HostAdapterBusType BusLogic_HostAdapterBusTypes[] = {
	BusLogic_VESA_Bus,	/* BT-4xx */
	BusLogic_ISA_Bus,	/* BT-5xx */
	BusLogic_MCA_Bus,	/* BT-6xx */
	BusLogic_EISA_Bus,	/* BT-7xx */
	BusLogic_Unknown_Bus,	/* BT-8xx */
	BusLogic_PCI_Bus	/* BT-9xx */
};


enum BusLogic_BIOS_DiskGeometryTranslation {
	BusLogic_BIOS_Disk_Not_Installed = 0,
	BusLogic_BIOS_Disk_Installed_64x32 = 1,
	BusLogic_BIOS_Disk_Installed_128x32 = 2,
	BusLogic_BIOS_Disk_Installed_255x63 = 3
} PACKED;



struct BusLogic_ByteCounter {
	unsigned int Units;
	unsigned int Billions;
};



struct BusLogic_ProbeInfo {
	enum BusLogic_HostAdapterType HostAdapterType;
	enum BusLogic_HostAdapterBusType HostAdapterBusType;
	unsigned long IO_Address;
	unsigned long PCI_Address;
	struct pci_dev *PCI_Device;
	unsigned char Bus;
	unsigned char Device;
	unsigned char IRQ_Channel;
};


struct BusLogic_ProbeOptions {
	bool NoProbe:1;		/* Bit 0 */
	bool NoProbeISA:1;	/* Bit 1 */
	bool NoProbePCI:1;	/* Bit 2 */
	bool NoSortPCI:1;	/* Bit 3 */
	bool MultiMasterFirst:1;/* Bit 4 */
	bool FlashPointFirst:1;	/* Bit 5 */
	bool LimitedProbeISA:1;	/* Bit 6 */
	bool Probe330:1;	/* Bit 7 */
	bool Probe334:1;	/* Bit 8 */
	bool Probe230:1;	/* Bit 9 */
	bool Probe234:1;	/* Bit 10 */
	bool Probe130:1;	/* Bit 11 */
	bool Probe134:1;	/* Bit 12 */
};


struct BusLogic_GlobalOptions {
	bool TraceProbe:1;	/* Bit 0 */
	bool TraceHardwareReset:1;	/* Bit 1 */
	bool TraceConfiguration:1;	/* Bit 2 */
	bool TraceErrors:1;	/* Bit 3 */
};


struct BusLogic_LocalOptions {
	bool InhibitTargetInquiry:1;	/* Bit 0 */
};


#define BusLogic_ControlRegisterOffset		0	/* WO register */
#define BusLogic_StatusRegisterOffset		0	/* RO register */
#define BusLogic_CommandParameterRegisterOffset	1	/* WO register */
#define BusLogic_DataInRegisterOffset		1	/* RO register */
#define BusLogic_InterruptRegisterOffset	2	/* RO register */
#define BusLogic_GeometryRegisterOffset		3	/* RO register */


union BusLogic_ControlRegister {
	unsigned char All;
	struct {
		unsigned char:4;	/* Bits 0-3 */
		bool SCSIBusReset:1;	/* Bit 4 */
		bool InterruptReset:1;	/* Bit 5 */
		bool SoftReset:1;	/* Bit 6 */
		bool HardReset:1;	/* Bit 7 */
	} cr;
};


union BusLogic_StatusRegister {
	unsigned char All;
	struct {
		bool CommandInvalid:1;		/* Bit 0 */
		bool Reserved:1;		/* Bit 1 */
		bool DataInRegisterReady:1;	/* Bit 2 */
		bool CommandParameterRegisterBusy:1;	/* Bit 3 */
		bool HostAdapterReady:1;	/* Bit 4 */
		bool InitializationRequired:1;	/* Bit 5 */
		bool DiagnosticFailure:1;	/* Bit 6 */
		bool DiagnosticActive:1;	/* Bit 7 */
	} sr;
};


union BusLogic_InterruptRegister {
	unsigned char All;
	struct {
		bool IncomingMailboxLoaded:1;	/* Bit 0 */
		bool OutgoingMailboxAvailable:1;/* Bit 1 */
		bool CommandComplete:1;		/* Bit 2 */
		bool ExternalBusReset:1;	/* Bit 3 */
		unsigned char Reserved:3;	/* Bits 4-6 */
		bool InterruptValid:1;		/* Bit 7 */
	} ir;
};


union BusLogic_GeometryRegister {
	unsigned char All;
	struct {
		enum BusLogic_BIOS_DiskGeometryTranslation Drive0Geometry:2;	/* Bits 0-1 */
		enum BusLogic_BIOS_DiskGeometryTranslation Drive1Geometry:2;	/* Bits 2-3 */
		unsigned char:3;	/* Bits 4-6 */
		bool ExtendedTranslationEnabled:1;	/* Bit 7 */
	} gr;
};


enum BusLogic_OperationCode {
	BusLogic_TestCommandCompleteInterrupt = 0x00,
	BusLogic_InitializeMailbox = 0x01,
	BusLogic_ExecuteMailboxCommand = 0x02,
	BusLogic_ExecuteBIOSCommand = 0x03,
	BusLogic_InquireBoardID = 0x04,
	BusLogic_EnableOutgoingMailboxAvailableInt = 0x05,
	BusLogic_SetSCSISelectionTimeout = 0x06,
	BusLogic_SetPreemptTimeOnBus = 0x07,
	BusLogic_SetTimeOffBus = 0x08,
	BusLogic_SetBusTransferRate = 0x09,
	BusLogic_InquireInstalledDevicesID0to7 = 0x0A,
	BusLogic_InquireConfiguration = 0x0B,
	BusLogic_EnableTargetMode = 0x0C,
	BusLogic_InquireSetupInformation = 0x0D,
	BusLogic_WriteAdapterLocalRAM = 0x1A,
	BusLogic_ReadAdapterLocalRAM = 0x1B,
	BusLogic_WriteBusMasterChipFIFO = 0x1C,
	BusLogic_ReadBusMasterChipFIFO = 0x1D,
	BusLogic_EchoCommandData = 0x1F,
	BusLogic_HostAdapterDiagnostic = 0x20,
	BusLogic_SetAdapterOptions = 0x21,
	BusLogic_InquireInstalledDevicesID8to15 = 0x23,
	BusLogic_InquireTargetDevices = 0x24,
	BusLogic_DisableHostAdapterInterrupt = 0x25,
	BusLogic_InitializeExtendedMailbox = 0x81,
	BusLogic_ExecuteSCSICommand = 0x83,
	BusLogic_InquireFirmwareVersion3rdDigit = 0x84,
	BusLogic_InquireFirmwareVersionLetter = 0x85,
	BusLogic_InquirePCIHostAdapterInformation = 0x86,
	BusLogic_InquireHostAdapterModelNumber = 0x8B,
	BusLogic_InquireSynchronousPeriod = 0x8C,
	BusLogic_InquireExtendedSetupInformation = 0x8D,
	BusLogic_EnableStrictRoundRobinMode = 0x8F,
	BusLogic_StoreHostAdapterLocalRAM = 0x90,
	BusLogic_FetchHostAdapterLocalRAM = 0x91,
	BusLogic_StoreLocalDataInEEPROM = 0x92,
	BusLogic_UploadAutoSCSICode = 0x94,
	BusLogic_ModifyIOAddress = 0x95,
	BusLogic_SetCCBFormat = 0x96,
	BusLogic_WriteInquiryBuffer = 0x9A,
	BusLogic_ReadInquiryBuffer = 0x9B,
	BusLogic_FlashROMUploadDownload = 0xA7,
	BusLogic_ReadSCAMData = 0xA8,
	BusLogic_WriteSCAMData = 0xA9
};


struct BusLogic_BoardID {
	unsigned char BoardType;	/* Byte 0 */
	unsigned char CustomFeatures;	/* Byte 1 */
	unsigned char FirmwareVersion1stDigit;	/* Byte 2 */
	unsigned char FirmwareVersion2ndDigit;	/* Byte 3 */
};


struct BusLogic_Configuration {
	unsigned char:5;	/* Byte 0 Bits 0-4 */
	bool DMA_Channel5:1;	/* Byte 0 Bit 5 */
	bool DMA_Channel6:1;	/* Byte 0 Bit 6 */
	bool DMA_Channel7:1;	/* Byte 0 Bit 7 */
	bool IRQ_Channel9:1;	/* Byte 1 Bit 0 */
	bool IRQ_Channel10:1;	/* Byte 1 Bit 1 */
	bool IRQ_Channel11:1;	/* Byte 1 Bit 2 */
	bool IRQ_Channel12:1;	/* Byte 1 Bit 3 */
	unsigned char:1;	/* Byte 1 Bit 4 */
	bool IRQ_Channel14:1;	/* Byte 1 Bit 5 */
	bool IRQ_Channel15:1;	/* Byte 1 Bit 6 */
	unsigned char:1;	/* Byte 1 Bit 7 */
	unsigned char HostAdapterID:4;	/* Byte 2 Bits 0-3 */
	unsigned char:4;	/* Byte 2 Bits 4-7 */
};


struct BusLogic_SynchronousValue {
	unsigned char Offset:4;	/* Bits 0-3 */
	unsigned char TransferPeriod:3;	/* Bits 4-6 */
	bool Synchronous:1;	/* Bit 7 */
};

struct BusLogic_SetupInformation {
	bool SynchronousInitiationEnabled:1;	/* Byte 0 Bit 0 */
	bool ParityCheckingEnabled:1;		/* Byte 0 Bit 1 */
	unsigned char:6;	/* Byte 0 Bits 2-7 */
	unsigned char BusTransferRate;	/* Byte 1 */
	unsigned char PreemptTimeOnBus;	/* Byte 2 */
	unsigned char TimeOffBus;	/* Byte 3 */
	unsigned char MailboxCount;	/* Byte 4 */
	unsigned char MailboxAddress[3];	/* Bytes 5-7 */
	struct BusLogic_SynchronousValue SynchronousValuesID0to7[8];	/* Bytes 8-15 */
	unsigned char DisconnectPermittedID0to7;	/* Byte 16 */
	unsigned char Signature;	/* Byte 17 */
	unsigned char CharacterD;	/* Byte 18 */
	unsigned char HostBusType;	/* Byte 19 */
	unsigned char WideTransfersPermittedID0to7;	/* Byte 20 */
	unsigned char WideTransfersActiveID0to7;	/* Byte 21 */
	struct BusLogic_SynchronousValue SynchronousValuesID8to15[8];	/* Bytes 22-29 */
	unsigned char DisconnectPermittedID8to15;	/* Byte 30 */
	unsigned char:8;	/* Byte 31 */
	unsigned char WideTransfersPermittedID8to15;	/* Byte 32 */
	unsigned char WideTransfersActiveID8to15;	/* Byte 33 */
};


struct BusLogic_ExtendedMailboxRequest {
	unsigned char MailboxCount;	/* Byte 0 */
	u32 BaseMailboxAddress;	/* Bytes 1-4 */
} PACKED;



enum BusLogic_ISACompatibleIOPort {
	BusLogic_IO_330 = 0,
	BusLogic_IO_334 = 1,
	BusLogic_IO_230 = 2,
	BusLogic_IO_234 = 3,
	BusLogic_IO_130 = 4,
	BusLogic_IO_134 = 5,
	BusLogic_IO_Disable = 6,
	BusLogic_IO_Disable2 = 7
} PACKED;

struct BusLogic_PCIHostAdapterInformation {
	enum BusLogic_ISACompatibleIOPort ISACompatibleIOPort;	/* Byte 0 */
	unsigned char PCIAssignedIRQChannel;	/* Byte 1 */
	bool LowByteTerminated:1;	/* Byte 2 Bit 0 */
	bool HighByteTerminated:1;	/* Byte 2 Bit 1 */
	unsigned char:2;	/* Byte 2 Bits 2-3 */
	bool JP1:1;		/* Byte 2 Bit 4 */
	bool JP2:1;		/* Byte 2 Bit 5 */
	bool JP3:1;		/* Byte 2 Bit 6 */
	bool GenericInfoValid:1;/* Byte 2 Bit 7 */
	unsigned char:8;	/* Byte 3 */
};


struct BusLogic_ExtendedSetupInformation {
	unsigned char BusType;	/* Byte 0 */
	unsigned char BIOS_Address;	/* Byte 1 */
	unsigned short ScatterGatherLimit;	/* Bytes 2-3 */
	unsigned char MailboxCount;	/* Byte 4 */
	u32 BaseMailboxAddress;	/* Bytes 5-8 */
	struct {
		unsigned char:2;	/* Byte 9 Bits 0-1 */
		bool FastOnEISA:1;	/* Byte 9 Bit 2 */
		unsigned char:3;	/* Byte 9 Bits 3-5 */
		bool LevelSensitiveInterrupt:1;	/* Byte 9 Bit 6 */
		unsigned char:1;	/* Byte 9 Bit 7 */
	} Misc;
	unsigned char FirmwareRevision[3];	/* Bytes 10-12 */
	bool HostWideSCSI:1;		/* Byte 13 Bit 0 */
	bool HostDifferentialSCSI:1;	/* Byte 13 Bit 1 */
	bool HostSupportsSCAM:1;	/* Byte 13 Bit 2 */
	bool HostUltraSCSI:1;		/* Byte 13 Bit 3 */
	bool HostSmartTermination:1;	/* Byte 13 Bit 4 */
	unsigned char:3;	/* Byte 13 Bits 5-7 */
} PACKED;


enum BusLogic_RoundRobinModeRequest {
	BusLogic_AggressiveRoundRobinMode = 0,
	BusLogic_StrictRoundRobinMode = 1
} PACKED;



#define BusLogic_BIOS_BaseOffset		0
#define BusLogic_AutoSCSI_BaseOffset		64

struct BusLogic_FetchHostAdapterLocalRAMRequest {
	unsigned char ByteOffset;	/* Byte 0 */
	unsigned char ByteCount;	/* Byte 1 */
};


struct BusLogic_AutoSCSIData {
	unsigned char InternalFactorySignature[2];	/* Bytes 0-1 */
	unsigned char InformationByteCount;	/* Byte 2 */
	unsigned char HostAdapterType[6];	/* Bytes 3-8 */
	unsigned char:8;	/* Byte 9 */
	bool FloppyEnabled:1;		/* Byte 10 Bit 0 */
	bool FloppySecondary:1;		/* Byte 10 Bit 1 */
	bool LevelSensitiveInterrupt:1;	/* Byte 10 Bit 2 */
	unsigned char:2;	/* Byte 10 Bits 3-4 */
	unsigned char SystemRAMAreaForBIOS:3;	/* Byte 10 Bits 5-7 */
	unsigned char DMA_Channel:7;	/* Byte 11 Bits 0-6 */
	bool DMA_AutoConfiguration:1;	/* Byte 11 Bit 7 */
	unsigned char IRQ_Channel:7;	/* Byte 12 Bits 0-6 */
	bool IRQ_AutoConfiguration:1;	/* Byte 12 Bit 7 */
	unsigned char DMA_TransferRate;	/* Byte 13 */
	unsigned char SCSI_ID;	/* Byte 14 */
	bool LowByteTerminated:1;	/* Byte 15 Bit 0 */
	bool ParityCheckingEnabled:1;	/* Byte 15 Bit 1 */
	bool HighByteTerminated:1;	/* Byte 15 Bit 2 */
	bool NoisyCablingEnvironment:1;	/* Byte 15 Bit 3 */
	bool FastSynchronousNegotiation:1;	/* Byte 15 Bit 4 */
	bool BusResetEnabled:1;		/* Byte 15 Bit 5 */
	 bool:1;		/* Byte 15 Bit 6 */
	bool ActiveNegationEnabled:1;	/* Byte 15 Bit 7 */
	unsigned char BusOnDelay;	/* Byte 16 */
	unsigned char BusOffDelay;	/* Byte 17 */
	bool HostAdapterBIOSEnabled:1;		/* Byte 18 Bit 0 */
	bool BIOSRedirectionOfINT19Enabled:1;	/* Byte 18 Bit 1 */
	bool ExtendedTranslationEnabled:1;	/* Byte 18 Bit 2 */
	bool MapRemovableAsFixedEnabled:1;	/* Byte 18 Bit 3 */
	 bool:1;		/* Byte 18 Bit 4 */
	bool BIOSSupportsMoreThan2DrivesEnabled:1;	/* Byte 18 Bit 5 */
	bool BIOSInterruptModeEnabled:1;	/* Byte 18 Bit 6 */
	bool FlopticalSupportEnabled:1;		/* Byte 19 Bit 7 */
	unsigned short DeviceEnabled;	/* Bytes 19-20 */
	unsigned short WidePermitted;	/* Bytes 21-22 */
	unsigned short FastPermitted;	/* Bytes 23-24 */
	unsigned short SynchronousPermitted;	/* Bytes 25-26 */
	unsigned short DisconnectPermitted;	/* Bytes 27-28 */
	unsigned short SendStartUnitCommand;	/* Bytes 29-30 */
	unsigned short IgnoreInBIOSScan;	/* Bytes 31-32 */
	unsigned char PCIInterruptPin:2;	/* Byte 33 Bits 0-1 */
	unsigned char HostAdapterIOPortAddress:2;	/* Byte 33 Bits 2-3 */
	bool StrictRoundRobinModeEnabled:1;	/* Byte 33 Bit 4 */
	bool VESABusSpeedGreaterThan33MHz:1;	/* Byte 33 Bit 5 */
	bool VESABurstWriteEnabled:1;	/* Byte 33 Bit 6 */
	bool VESABurstReadEnabled:1;	/* Byte 33 Bit 7 */
	unsigned short UltraPermitted;	/* Bytes 34-35 */
	unsigned int:32;	/* Bytes 36-39 */
	unsigned char:8;	/* Byte 40 */
	unsigned char AutoSCSIMaximumLUN;	/* Byte 41 */
	 bool:1;		/* Byte 42 Bit 0 */
	bool SCAM_Dominant:1;	/* Byte 42 Bit 1 */
	bool SCAM_Enabled:1;	/* Byte 42 Bit 2 */
	bool SCAM_Level2:1;	/* Byte 42 Bit 3 */
	unsigned char:4;	/* Byte 42 Bits 4-7 */
	bool INT13ExtensionEnabled:1;	/* Byte 43 Bit 0 */
	 bool:1;		/* Byte 43 Bit 1 */
	bool CDROMBootEnabled:1;	/* Byte 43 Bit 2 */
	unsigned char:5;	/* Byte 43 Bits 3-7 */
	unsigned char BootTargetID:4;	/* Byte 44 Bits 0-3 */
	unsigned char BootChannel:4;	/* Byte 44 Bits 4-7 */
	unsigned char ForceBusDeviceScanningOrder:1;	/* Byte 45 Bit 0 */
	unsigned char:7;	/* Byte 45 Bits 1-7 */
	unsigned short NonTaggedToAlternateLUNPermitted;	/* Bytes 46-47 */
	unsigned short RenegotiateSyncAfterCheckCondition;	/* Bytes 48-49 */
	unsigned char Reserved[10];	/* Bytes 50-59 */
	unsigned char ManufacturingDiagnostic[2];	/* Bytes 60-61 */
	unsigned short Checksum;	/* Bytes 62-63 */
} PACKED;


struct BusLogic_AutoSCSIByte45 {
	unsigned char ForceBusDeviceScanningOrder:1;	/* Bit 0 */
	unsigned char:7;	/* Bits 1-7 */
};


#define BusLogic_BIOS_DriveMapOffset		17

struct BusLogic_BIOSDriveMapByte {
	unsigned char TargetIDBit3:1;	/* Bit 0 */
	unsigned char:2;	/* Bits 1-2 */
	enum BusLogic_BIOS_DiskGeometryTranslation DiskGeometry:2;	/* Bits 3-4 */
	unsigned char TargetID:3;	/* Bits 5-7 */
};


enum BusLogic_SetCCBFormatRequest {
	BusLogic_LegacyLUNFormatCCB = 0,
	BusLogic_ExtendedLUNFormatCCB = 1
} PACKED;


enum BusLogic_ActionCode {
	BusLogic_OutgoingMailboxFree = 0x00,
	BusLogic_MailboxStartCommand = 0x01,
	BusLogic_MailboxAbortCommand = 0x02
} PACKED;



enum BusLogic_CompletionCode {
	BusLogic_IncomingMailboxFree = 0x00,
	BusLogic_CommandCompletedWithoutError = 0x01,
	BusLogic_CommandAbortedAtHostRequest = 0x02,
	BusLogic_AbortedCommandNotFound = 0x03,
	BusLogic_CommandCompletedWithError = 0x04,
	BusLogic_InvalidCCB = 0x05
} PACKED;


enum BusLogic_CCB_Opcode {
	BusLogic_InitiatorCCB = 0x00,
	BusLogic_TargetCCB = 0x01,
	BusLogic_InitiatorCCB_ScatterGather = 0x02,
	BusLogic_InitiatorCCB_ResidualDataLength = 0x03,
	BusLogic_InitiatorCCB_ScatterGatherResidual = 0x04,
	BusLogic_BusDeviceReset = 0x81
} PACKED;



enum BusLogic_DataDirection {
	BusLogic_UncheckedDataTransfer = 0,
	BusLogic_DataInLengthChecked = 1,
	BusLogic_DataOutLengthChecked = 2,
	BusLogic_NoDataTransfer = 3
};



enum BusLogic_HostAdapterStatus {
	BusLogic_CommandCompletedNormally = 0x00,
	BusLogic_LinkedCommandCompleted = 0x0A,
	BusLogic_LinkedCommandCompletedWithFlag = 0x0B,
	BusLogic_DataUnderRun = 0x0C,
	BusLogic_SCSISelectionTimeout = 0x11,
	BusLogic_DataOverRun = 0x12,
	BusLogic_UnexpectedBusFree = 0x13,
	BusLogic_InvalidBusPhaseRequested = 0x14,
	BusLogic_InvalidOutgoingMailboxActionCode = 0x15,
	BusLogic_InvalidCommandOperationCode = 0x16,
	BusLogic_LinkedCCBhasInvalidLUN = 0x17,
	BusLogic_InvalidCommandParameter = 0x1A,
	BusLogic_AutoRequestSenseFailed = 0x1B,
	BusLogic_TaggedQueuingMessageRejected = 0x1C,
	BusLogic_UnsupportedMessageReceived = 0x1D,
	BusLogic_HostAdapterHardwareFailed = 0x20,
	BusLogic_TargetFailedResponseToATN = 0x21,
	BusLogic_HostAdapterAssertedRST = 0x22,
	BusLogic_OtherDeviceAssertedRST = 0x23,
	BusLogic_TargetDeviceReconnectedImproperly = 0x24,
	BusLogic_HostAdapterAssertedBusDeviceReset = 0x25,
	BusLogic_AbortQueueGenerated = 0x26,
	BusLogic_HostAdapterSoftwareError = 0x27,
	BusLogic_HostAdapterHardwareTimeoutError = 0x30,
	BusLogic_SCSIParityErrorDetected = 0x34
} PACKED;



enum BusLogic_TargetDeviceStatus {
	BusLogic_OperationGood = 0x00,
	BusLogic_CheckCondition = 0x02,
	BusLogic_DeviceBusy = 0x08
} PACKED;


enum BusLogic_QueueTag {
	BusLogic_SimpleQueueTag = 0,
	BusLogic_HeadOfQueueTag = 1,
	BusLogic_OrderedQueueTag = 2,
	BusLogic_ReservedQT = 3
};


#define BusLogic_CDB_MaxLength			12

typedef unsigned char SCSI_CDB_T[BusLogic_CDB_MaxLength];



struct BusLogic_ScatterGatherSegment {
	u32 SegmentByteCount;	/* Bytes 0-3 */
	u32 SegmentDataPointer;	/* Bytes 4-7 */
};


enum BusLogic_CCB_Status {
	BusLogic_CCB_Free = 0,
	BusLogic_CCB_Active = 1,
	BusLogic_CCB_Completed = 2,
	BusLogic_CCB_Reset = 3
} PACKED;



struct BusLogic_CCB {
	/*
	   MultiMaster Firmware and FlashPoint SCCB Manager Common Portion.
	 */
	enum BusLogic_CCB_Opcode Opcode;	/* Byte 0 */
	unsigned char:3;	/* Byte 1 Bits 0-2 */
	enum BusLogic_DataDirection DataDirection:2;	/* Byte 1 Bits 3-4 */
	bool TagEnable:1;	/* Byte 1 Bit 5 */
	enum BusLogic_QueueTag QueueTag:2;	/* Byte 1 Bits 6-7 */
	unsigned char CDB_Length;	/* Byte 2 */
	unsigned char SenseDataLength;	/* Byte 3 */
	u32 DataLength;		/* Bytes 4-7 */
	u32 DataPointer;	/* Bytes 8-11 */
	unsigned char:8;	/* Byte 12 */
	unsigned char:8;	/* Byte 13 */
	enum BusLogic_HostAdapterStatus HostAdapterStatus;	/* Byte 14 */
	enum BusLogic_TargetDeviceStatus TargetDeviceStatus;	/* Byte 15 */
	unsigned char TargetID;	/* Byte 16 */
	unsigned char LogicalUnit:5;	/* Byte 17 Bits 0-4 */
	bool LegacyTagEnable:1;	/* Byte 17 Bit 5 */
	enum BusLogic_QueueTag LegacyQueueTag:2;	/* Byte 17 Bits 6-7 */
	SCSI_CDB_T CDB;		/* Bytes 18-29 */
	unsigned char:8;	/* Byte 30 */
	unsigned char:8;	/* Byte 31 */
	unsigned int:32;	/* Bytes 32-35 */
	u32 SenseDataPointer;	/* Bytes 36-39 */
	/*
	   FlashPoint SCCB Manager Defined Portion.
	 */
	void (*CallbackFunction) (struct BusLogic_CCB *);	/* Bytes 40-43 */
	u32 BaseAddress;	/* Bytes 44-47 */
	enum BusLogic_CompletionCode CompletionCode;	/* Byte 48 */
#ifdef CONFIG_SCSI_FLASHPOINT
	unsigned char:8;	/* Byte 49 */
	unsigned short OS_Flags;	/* Bytes 50-51 */
	unsigned char Private[48];	/* Bytes 52-99 */
#endif
	/*
	   BusLogic Linux Driver Defined Portion.
	 */
	dma_addr_t AllocationGroupHead;
	unsigned int AllocationGroupSize;
	u32 DMA_Handle;
	enum BusLogic_CCB_Status Status;
	unsigned long SerialNumber;
	struct scsi_cmnd *Command;
	struct BusLogic_HostAdapter *HostAdapter;
	struct BusLogic_CCB *Next;
	struct BusLogic_CCB *NextAll;
	struct BusLogic_ScatterGatherSegment
	 ScatterGatherList[BusLogic_ScatterGatherLimit];
};


struct BusLogic_OutgoingMailbox {
	u32 CCB;		/* Bytes 0-3 */
	unsigned int:24;	/* Bytes 4-6 */
	enum BusLogic_ActionCode ActionCode;	/* Byte 7 */
};


struct BusLogic_IncomingMailbox {
	u32 CCB;		/* Bytes 0-3 */
	enum BusLogic_HostAdapterStatus HostAdapterStatus;	/* Byte 4 */
	enum BusLogic_TargetDeviceStatus TargetDeviceStatus;	/* Byte 5 */
	unsigned char:8;	/* Byte 6 */
	enum BusLogic_CompletionCode CompletionCode;	/* Byte 7 */
};



struct BusLogic_DriverOptions {
	unsigned short TaggedQueuingPermitted;
	unsigned short TaggedQueuingPermittedMask;
	unsigned short BusSettleTime;
	struct BusLogic_LocalOptions LocalOptions;
	unsigned char CommonQueueDepth;
	unsigned char QueueDepth[BusLogic_MaxTargetDevices];
};


struct BusLogic_TargetFlags {
	bool TargetExists:1;
	bool TaggedQueuingSupported:1;
	bool WideTransfersSupported:1;
	bool TaggedQueuingActive:1;
	bool WideTransfersActive:1;
	bool CommandSuccessfulFlag:1;
	bool TargetInfoReported:1;
};


#define BusLogic_SizeBuckets			10

typedef unsigned int BusLogic_CommandSizeBuckets_T[BusLogic_SizeBuckets];

struct BusLogic_TargetStatistics {
	unsigned int CommandsAttempted;
	unsigned int CommandsCompleted;
	unsigned int ReadCommands;
	unsigned int WriteCommands;
	struct BusLogic_ByteCounter TotalBytesRead;
	struct BusLogic_ByteCounter TotalBytesWritten;
	BusLogic_CommandSizeBuckets_T ReadCommandSizeBuckets;
	BusLogic_CommandSizeBuckets_T WriteCommandSizeBuckets;
	unsigned short CommandAbortsRequested;
	unsigned short CommandAbortsAttempted;
	unsigned short CommandAbortsCompleted;
	unsigned short BusDeviceResetsRequested;
	unsigned short BusDeviceResetsAttempted;
	unsigned short BusDeviceResetsCompleted;
	unsigned short HostAdapterResetsRequested;
	unsigned short HostAdapterResetsAttempted;
	unsigned short HostAdapterResetsCompleted;
};


#define FlashPoint_BadCardHandle		0xFFFFFFFF

typedef unsigned int FlashPoint_CardHandle_T;



struct FlashPoint_Info {
	u32 BaseAddress;	/* Bytes 0-3 */
	bool Present;		/* Byte 4 */
	unsigned char IRQ_Channel;	/* Byte 5 */
	unsigned char SCSI_ID;	/* Byte 6 */
	unsigned char SCSI_LUN;	/* Byte 7 */
	unsigned short FirmwareRevision;	/* Bytes 8-9 */
	unsigned short SynchronousPermitted;	/* Bytes 10-11 */
	unsigned short FastPermitted;	/* Bytes 12-13 */
	unsigned short UltraPermitted;	/* Bytes 14-15 */
	unsigned short DisconnectPermitted;	/* Bytes 16-17 */
	unsigned short WidePermitted;	/* Bytes 18-19 */
	bool ParityCheckingEnabled:1;	/* Byte 20 Bit 0 */
	bool HostWideSCSI:1;		/* Byte 20 Bit 1 */
	bool HostSoftReset:1;		/* Byte 20 Bit 2 */
	bool ExtendedTranslationEnabled:1;	/* Byte 20 Bit 3 */
	bool LowByteTerminated:1;	/* Byte 20 Bit 4 */
	bool HighByteTerminated:1;	/* Byte 20 Bit 5 */
	bool ReportDataUnderrun:1;	/* Byte 20 Bit 6 */
	bool SCAM_Enabled:1;	/* Byte 20 Bit 7 */
	bool SCAM_Level2:1;	/* Byte 21 Bit 0 */
	unsigned char:7;	/* Byte 21 Bits 1-7 */
	unsigned char Family;	/* Byte 22 */
	unsigned char BusType;	/* Byte 23 */
	unsigned char ModelNumber[3];	/* Bytes 24-26 */
	unsigned char RelativeCardNumber;	/* Byte 27 */
	unsigned char Reserved[4];	/* Bytes 28-31 */
	unsigned int OS_Reserved;	/* Bytes 32-35 */
	unsigned char TranslationInfo[4];	/* Bytes 36-39 */
	unsigned int Reserved2[5];	/* Bytes 40-59 */
	unsigned int SecondaryRange;	/* Bytes 60-63 */
};


struct BusLogic_HostAdapter {
	struct Scsi_Host *SCSI_Host;
	struct pci_dev *PCI_Device;
	enum BusLogic_HostAdapterType HostAdapterType;
	enum BusLogic_HostAdapterBusType HostAdapterBusType;
	unsigned long IO_Address;
	unsigned long PCI_Address;
	unsigned short AddressCount;
	unsigned char HostNumber;
	unsigned char ModelName[9];
	unsigned char FirmwareVersion[6];
	unsigned char FullModelName[18];
	unsigned char Bus;
	unsigned char Device;
	unsigned char IRQ_Channel;
	unsigned char DMA_Channel;
	unsigned char SCSI_ID;
	bool IRQ_ChannelAcquired:1;
	bool DMA_ChannelAcquired:1;
	bool ExtendedTranslationEnabled:1;
	bool ParityCheckingEnabled:1;
	bool BusResetEnabled:1;
	bool LevelSensitiveInterrupt:1;
	bool HostWideSCSI:1;
	bool HostDifferentialSCSI:1;
	bool HostSupportsSCAM:1;
	bool HostUltraSCSI:1;
	bool ExtendedLUNSupport:1;
	bool TerminationInfoValid:1;
	bool LowByteTerminated:1;
	bool HighByteTerminated:1;
	bool BounceBuffersRequired:1;
	bool StrictRoundRobinModeSupport:1;
	bool SCAM_Enabled:1;
	bool SCAM_Level2:1;
	bool HostAdapterInitialized:1;
	bool HostAdapterExternalReset:1;
	bool HostAdapterInternalError:1;
	bool ProcessCompletedCCBsActive;
	volatile bool HostAdapterCommandCompleted;
	unsigned short HostAdapterScatterGatherLimit;
	unsigned short DriverScatterGatherLimit;
	unsigned short MaxTargetDevices;
	unsigned short MaxLogicalUnits;
	unsigned short MailboxCount;
	unsigned short InitialCCBs;
	unsigned short IncrementalCCBs;
	unsigned short AllocatedCCBs;
	unsigned short DriverQueueDepth;
	unsigned short HostAdapterQueueDepth;
	unsigned short UntaggedQueueDepth;
	unsigned short CommonQueueDepth;
	unsigned short BusSettleTime;
	unsigned short SynchronousPermitted;
	unsigned short FastPermitted;
	unsigned short UltraPermitted;
	unsigned short WidePermitted;
	unsigned short DisconnectPermitted;
	unsigned short TaggedQueuingPermitted;
	unsigned short ExternalHostAdapterResets;
	unsigned short HostAdapterInternalErrors;
	unsigned short TargetDeviceCount;
	unsigned short MessageBufferLength;
	u32 BIOS_Address;
	struct BusLogic_DriverOptions *DriverOptions;
	struct FlashPoint_Info FlashPointInfo;
	FlashPoint_CardHandle_T CardHandle;
	struct list_head host_list;
	struct BusLogic_CCB *All_CCBs;
	struct BusLogic_CCB *Free_CCBs;
	struct BusLogic_CCB *FirstCompletedCCB;
	struct BusLogic_CCB *LastCompletedCCB;
	struct BusLogic_CCB *BusDeviceResetPendingCCB[BusLogic_MaxTargetDevices];
	struct BusLogic_TargetFlags TargetFlags[BusLogic_MaxTargetDevices];
	unsigned char QueueDepth[BusLogic_MaxTargetDevices];
	unsigned char SynchronousPeriod[BusLogic_MaxTargetDevices];
	unsigned char SynchronousOffset[BusLogic_MaxTargetDevices];
	unsigned char ActiveCommands[BusLogic_MaxTargetDevices];
	unsigned int CommandsSinceReset[BusLogic_MaxTargetDevices];
	unsigned long LastSequencePoint[BusLogic_MaxTargetDevices];
	unsigned long LastResetAttempted[BusLogic_MaxTargetDevices];
	unsigned long LastResetCompleted[BusLogic_MaxTargetDevices];
	struct BusLogic_OutgoingMailbox *FirstOutgoingMailbox;
	struct BusLogic_OutgoingMailbox *LastOutgoingMailbox;
	struct BusLogic_OutgoingMailbox *NextOutgoingMailbox;
	struct BusLogic_IncomingMailbox *FirstIncomingMailbox;
	struct BusLogic_IncomingMailbox *LastIncomingMailbox;
	struct BusLogic_IncomingMailbox *NextIncomingMailbox;
	struct BusLogic_TargetStatistics TargetStatistics[BusLogic_MaxTargetDevices];
	unsigned char *MailboxSpace;
	dma_addr_t MailboxSpaceHandle;
	unsigned int MailboxSize;
	unsigned long CCB_Offset;
	char MessageBuffer[BusLogic_MessageBufferSize];
};


struct BIOS_DiskParameters {
	int Heads;
	int Sectors;
	int Cylinders;
};


struct SCSI_Inquiry {
	unsigned char PeripheralDeviceType:5;	/* Byte 0 Bits 0-4 */
	unsigned char PeripheralQualifier:3;	/* Byte 0 Bits 5-7 */
	unsigned char DeviceTypeModifier:7;	/* Byte 1 Bits 0-6 */
	bool RMB:1;		/* Byte 1 Bit 7 */
	unsigned char ANSI_ApprovedVersion:3;	/* Byte 2 Bits 0-2 */
	unsigned char ECMA_Version:3;	/* Byte 2 Bits 3-5 */
	unsigned char ISO_Version:2;	/* Byte 2 Bits 6-7 */
	unsigned char ResponseDataFormat:4;	/* Byte 3 Bits 0-3 */
	unsigned char:2;	/* Byte 3 Bits 4-5 */
	bool TrmIOP:1;		/* Byte 3 Bit 6 */
	bool AENC:1;		/* Byte 3 Bit 7 */
	unsigned char AdditionalLength;	/* Byte 4 */
	unsigned char:8;	/* Byte 5 */
	unsigned char:8;	/* Byte 6 */
	bool SftRe:1;		/* Byte 7 Bit 0 */
	bool CmdQue:1;		/* Byte 7 Bit 1 */
	 bool:1;		/* Byte 7 Bit 2 */
	bool Linked:1;		/* Byte 7 Bit 3 */
	bool Sync:1;		/* Byte 7 Bit 4 */
	bool WBus16:1;		/* Byte 7 Bit 5 */
	bool WBus32:1;		/* Byte 7 Bit 6 */
	bool RelAdr:1;		/* Byte 7 Bit 7 */
	unsigned char VendorIdentification[8];	/* Bytes 8-15 */
	unsigned char ProductIdentification[16];	/* Bytes 16-31 */
	unsigned char ProductRevisionLevel[4];	/* Bytes 32-35 */
};



static inline void BusLogic_SCSIBusReset(struct BusLogic_HostAdapter *HostAdapter)
{
	union BusLogic_ControlRegister ControlRegister;
	ControlRegister.All = 0;
	ControlRegister.cr.SCSIBusReset = true;
	outb(ControlRegister.All, HostAdapter->IO_Address + BusLogic_ControlRegisterOffset);
}

static inline void BusLogic_InterruptReset(struct BusLogic_HostAdapter *HostAdapter)
{
	union BusLogic_ControlRegister ControlRegister;
	ControlRegister.All = 0;
	ControlRegister.cr.InterruptReset = true;
	outb(ControlRegister.All, HostAdapter->IO_Address + BusLogic_ControlRegisterOffset);
}

static inline void BusLogic_SoftReset(struct BusLogic_HostAdapter *HostAdapter)
{
	union BusLogic_ControlRegister ControlRegister;
	ControlRegister.All = 0;
	ControlRegister.cr.SoftReset = true;
	outb(ControlRegister.All, HostAdapter->IO_Address + BusLogic_ControlRegisterOffset);
}

static inline void BusLogic_HardReset(struct BusLogic_HostAdapter *HostAdapter)
{
	union BusLogic_ControlRegister ControlRegister;
	ControlRegister.All = 0;
	ControlRegister.cr.HardReset = true;
	outb(ControlRegister.All, HostAdapter->IO_Address + BusLogic_ControlRegisterOffset);
}

static inline unsigned char BusLogic_ReadStatusRegister(struct BusLogic_HostAdapter *HostAdapter)
{
	return inb(HostAdapter->IO_Address + BusLogic_StatusRegisterOffset);
}

static inline void BusLogic_WriteCommandParameterRegister(struct BusLogic_HostAdapter
							  *HostAdapter, unsigned char Value)
{
	outb(Value, HostAdapter->IO_Address + BusLogic_CommandParameterRegisterOffset);
}

static inline unsigned char BusLogic_ReadDataInRegister(struct BusLogic_HostAdapter *HostAdapter)
{
	return inb(HostAdapter->IO_Address + BusLogic_DataInRegisterOffset);
}

static inline unsigned char BusLogic_ReadInterruptRegister(struct BusLogic_HostAdapter *HostAdapter)
{
	return inb(HostAdapter->IO_Address + BusLogic_InterruptRegisterOffset);
}

static inline unsigned char BusLogic_ReadGeometryRegister(struct BusLogic_HostAdapter *HostAdapter)
{
	return inb(HostAdapter->IO_Address + BusLogic_GeometryRegisterOffset);
}


static inline void BusLogic_StartMailboxCommand(struct BusLogic_HostAdapter *HostAdapter)
{
	BusLogic_WriteCommandParameterRegister(HostAdapter, BusLogic_ExecuteMailboxCommand);
}


static inline void BusLogic_Delay(int Seconds)
{
	mdelay(1000 * Seconds);
}


static inline u32 Virtual_to_Bus(void *VirtualAddress)
{
	return (u32) virt_to_bus(VirtualAddress);
}

static inline void *Bus_to_Virtual(u32 BusAddress)
{
	return (void *) bus_to_virt(BusAddress);
}


static inline u32 Virtual_to_32Bit_Virtual(void *VirtualAddress)
{
	return (u32) (unsigned long) VirtualAddress;
}


static inline void BusLogic_IncrementErrorCounter(unsigned short *ErrorCounter)
{
	if (*ErrorCounter < 65535)
		(*ErrorCounter)++;
}


static inline void BusLogic_IncrementByteCounter(struct BusLogic_ByteCounter
						 *ByteCounter, unsigned int Amount)
{
	ByteCounter->Units += Amount;
	if (ByteCounter->Units > 999999999) {
		ByteCounter->Units -= 1000000000;
		ByteCounter->Billions++;
	}
}


static inline void BusLogic_IncrementSizeBucket(BusLogic_CommandSizeBuckets_T CommandSizeBuckets, unsigned int Amount)
{
	int Index = 0;
	if (Amount < 8 * 1024) {
		if (Amount < 2 * 1024)
			Index = (Amount < 1 * 1024 ? 0 : 1);
		else
			Index = (Amount < 4 * 1024 ? 2 : 3);
	} else if (Amount < 128 * 1024) {
		if (Amount < 32 * 1024)
			Index = (Amount < 16 * 1024 ? 4 : 5);
		else
			Index = (Amount < 64 * 1024 ? 6 : 7);
	} else
		Index = (Amount < 256 * 1024 ? 8 : 9);
	CommandSizeBuckets[Index]++;
}


#define FlashPoint_FirmwareVersion		"5.02"


#define FlashPoint_NormalInterrupt		0x00
#define FlashPoint_InternalError		0xFE
#define FlashPoint_ExternalBusReset		0xFF


static const char *BusLogic_DriverInfo(struct Scsi_Host *);
static int BusLogic_QueueCommand(struct scsi_cmnd *, void (*CompletionRoutine) (struct scsi_cmnd *));
static int BusLogic_BIOSDiskParameters(struct scsi_device *, struct block_device *, sector_t, int *);
static int BusLogic_ProcDirectoryInfo(struct Scsi_Host *, char *, char **, off_t, int, int);
static int BusLogic_SlaveConfigure(struct scsi_device *);
static void BusLogic_QueueCompletedCCB(struct BusLogic_CCB *);
static irqreturn_t BusLogic_InterruptHandler(int, void *);
static int BusLogic_ResetHostAdapter(struct BusLogic_HostAdapter *, bool HardReset);
static void BusLogic_Message(enum BusLogic_MessageLevel, char *, struct BusLogic_HostAdapter *, ...);
static int __init BusLogic_Setup(char *);

#endif				/* _BUSLOGIC_H */
