#pragma once

#define NDIS620
#define NDIS620_MINIPORT 
#define NDIS_MINIPORT_DRIVER 1
#define NDIS_WDM 
#define NDIS_LEGACY_MINIPORT 1
#define NDIS_SUPPORT_NDIS60 1
#ifndef __NDISPROT__H
#define __NDISPROT__H
#endif
#define NDIS_LEGACY_PROTOCOL 1
//#define UM_NDIS620



#include"ndis.h"
#include"ntddk.h"
#include "wdm.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"



#define AcqSpinLock(SpinL,Dispatch) {							\
	if(Dispatch)												\
	{															\
		NdisDprAcquireSpinLock(SpinL);							\
																\
	} else NdisAcquireSpinLock(SpinL);							\
																\
}

#define RelSpinLock(SpinL,Dispatch) {							\
	if(Dispatch)												\
	{															\
		NdisDprReleaseSpinLock(SpinL);							\
																\
	} else NdisReleaseSpinLock(SpinL);							\
																\
}

typedef enum _PHYSICALMEDIUM
{
	PhysicalMediumUnspecified,
	PhysicalMediumWirelessLan,
	PhysicalMediumCableModem,
	PhysicalMediumPhoneLine,
	PhysicalMediumPowerLine,
	PhysicalMediumDSL,      // includes ADSL and UADSL (G.Lite)
	PhysicalMediumFibreChannel,
	PhysicalMedium1394,
	PhysicalMediumWirelessWan,
	PhysicalMediumNative802_11,
	PhysicalMediumBluetooth,
	PhysicalMediumInfiniband,
	PhysicalMediumWiMax,
	PhysicalMediumUWB,
	PhysicalMedium802_3,
	PhysicalMedium802_5,
	PhysicalMediumIrda,
	PhysicalMediumWiredWAN,
	PhysicalMediumWiredCoWan,
	PhysicalMediumOther,
	PhysicalMediumNative802_15_4,
	PhysicalMediumMax
} PHYSICALMEDIUM;

typedef enum _MEDIA_CONNECT_STATE
{
	ConnectStateUnknown,
	ConnectStateConnected,
	ConnectStateDisconnected
} MEDIA_CONNECT_STATE;

typedef enum _ADAPTER_STATE {
	NdisprotInitializing,
	NdisprotRunning,
	NdisprotPausing,
	NdisprotPaused,
	NdisprotRestarting,
	NdisprotClosing
} ADAPTER_STATE;

typedef struct Device_Lan
{
	char name[150];
	char service_name[150];
	int NetCardsCount;
	ULONG64 BindingContext;
	ULONG ModeCap;
	int NetMonSuported;
	ULONG MtuSize;
	ULONG64 MaxXmitLinkSpeed;
	ULONG64 XmitLinkSpeed;
	ULONG64 MaxRcvLinkSpeed;
	ULONG64 RcvLinkSpeed;
	USHORT MacAddressLength;
	UCHAR CurrentMacAddress[32];
	PHYSICALMEDIUM PhysicalMediumType;
	MEDIA_CONNECT_STATE MediaConnectState;
}Dev_Lan;

typedef struct _Ethernet_Header_802_3
{
	unsigned char Source_Mac[6];
	unsigned char Destination_Mac[6];
	unsigned char Typ[2];
	unsigned char Data[1500];
	char FCS[4];
}EHead_802;

typedef struct Frame_Control
{
	unsigned char Protocol_Version : 2;
	unsigned char Type : 2;
	unsigned char SybType : 4;
	unsigned char ToDis : 1;
	unsigned char FromDis : 1;
	unsigned char MoreFlag : 1;
	unsigned char Retry : 1;
	unsigned char PowerMgmt : 1;
	unsigned char MoreData : 1;
	unsigned char WEP : 1;
	unsigned char Order : 1;
}FrameControl;

typedef struct _Ethernet_Header_802_11
{
	//unsigned char FrameControl[2];
	FrameControl FramContr;
	unsigned char DurationId[2];
	unsigned char Adres1[6];
	unsigned char Adres2[6];
	unsigned char Adres3[6];
	unsigned char SequenceControl[2];
	unsigned char Adres4[6];
	unsigned char NetworkData[2312];
	unsigned char FCS[4];
}EHead_802_11;

typedef struct _net_cards
{
	wchar_t keyPath[150];
	char Description[150];
	char ServiceName[150];
	ULONG ModeCap;
	int NetMonSuported;
	NDIS_BIND_PARAMETERS BindParam;

// Items for driver purposes only		

	int  NetCardsCount;
	NDIS_HANDLE Adapter_Handle;
	ULONG64 BindingContext;
	NDIS_HANDLE Buffer;
	NDIS_HANDLE PBContext;
	NDIS_SPIN_LOCK SpinLockA;
	int licznik,indeks;
	int openIndex;
	BOOLEAN UnbindExecute;
	NDIS_STATUS UnbindStatus;
	BOOLEAN isOpened;
	ADAPTER_STATE AState;
	NDIS_DEVICE_POWER_STATE PowerState;
}net_cards;



ULONG32 PACKET_COUNT;

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD OnUnload;