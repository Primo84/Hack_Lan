#pragma once

//#include "Lan.h"
#include "FileDef.h"
//#include "wdf.h"


#define FILE_DEV_DRVF		0x00002A7A


#define IO_MINIPORTS_INIT				(ULONG)CTL_CODE(FILE_DEVICE_NETWORK,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)


NTSTATUS HDeviceIOControl(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceCreate(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceClose(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceCleanUp(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDevicePnP(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceRead(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceQueryInformation(__in DEVICE_OBJECT* Device, __inout IRP* irp);
NTSTATUS HDeviceSkip(__in DEVICE_OBJECT* Device, __inout IRP* irp);


PDEVICE_OBJECT Device_Hack,DeviceAttach;


wchar_t Dev_Name[] = L"\\Device\\FHacklan";
wchar_t Dev_Dos_Name[] = L"\\DosDevices\\FHacklan";

UNICODE_STRING DevN, DevDN;
NTSTATUS st;
int i;

//NDIS_MEDIUM medium[] = { NdisMediumNative802_11,NdisMedium802_3,NdisMedium802_5,NdisMediumFddi,NdisMediumWan,NdisMediumWirelessWan };



void Sleep(ULONG milisec);
