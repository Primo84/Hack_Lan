
#pragma once

#include "LanDrv.h"
#include "Filter.h"




NDIS_DEVICE_OBJECT_ATTRIBUTES ObjectAttributes;



#pragma NDIS_INIT_FUNCTION(DriverEntry)



_Use_decl_annotations_ NTSTATUS DriverEntry(IN PDRIVER_OBJECT Driver, IN PUNICODE_STRING str)
{

	Driver->DriverUnload = &OnUnload;
	
	FilterHandle = 0;
	miniportsCount = 0;
	PACKET_COUNT = 1000;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "FilterDrv start....Path str : %ws\n", str->Buffer);

	InitializeListHead(&MiniportsEntries);

	
	


	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)Driver->MajorFunction[i] = &HDeviceSkip;
	Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &HDeviceIOControl;
	Driver->MajorFunction[IRP_MJ_CREATE] = &HDeviceCreate;
	Driver->MajorFunction[IRP_MJ_CLOSE] = &HDeviceClose;
	Driver->MajorFunction[IRP_MJ_CLEANUP] = &HDeviceCleanUp;
	//Driver->MajorFunction[IRP_MJ_PNP] = &HDevicePnP;
	Driver->MajorFunction[IRP_MJ_READ] = &HDeviceRead;
	//Driver->MajorFunction[IRP_MJ_QUERY_INFORMATION] = &HDeviceQueryInformation;
	
	

	DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "Pryzpisanie funkcji sterownika....");

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Tworzenie Objektu Device");

	
	
	
	RtlInitUnicodeString((PUNICODE_STRING)&DevN, Dev_Name);
	RtlInitUnicodeString((PUNICODE_STRING)&DevDN, Dev_Dos_Name);

	
	
	st = IoCreateDevice(Driver, 0, &DevN, FILE_DEVICE_NETWORK, FILE_DEVICE_SECURE_OPEN, FALSE, &Device_Hack);

	if (st == STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Utworzono DeviceObject");
		
	}
	else
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nieutworzono DeviceObject");
		if (st == STATUS_INSUFFICIENT_RESOURCES) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Bload tworzenia Device : STATUS_INSUFFICIENT_RESOURCES");
		else if (st == STATUS_OBJECT_NAME_COLLISION) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Bload tworzenia Device : STATUS_OBJECT_NAME_COLLISION");
	}

	
	if (IoCreateSymbolicLink(&DevDN,&DevN) == STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Utworzono SymbolicLink");
	}
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NieUtworzono SymbolicLink");

	Device_Hack->Flags = Device_Hack->Flags | DO_DIRECT_IO;
	Device_Hack->Flags = Device_Hack->Flags | DO_BUFFERED_IO;

	RegisterFilterDriver(Driver);

	return STATUS_SUCCESS;

}

VOID OnUnload(IN PDRIVER_OBJECT Driver)
{
	NDIS_STATUS status;
	int i;
	
	if (FilterHandle != NULL && FilterRegistered == TRUE)
	{
		NdisFDeregisterFilterDriver(FilterHandle);
		//Sleep(5000);
		FilterRegistered = FALSE;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter Driver wyrejestrowano pomyœlnie");
	}
	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, " Ending LanDrv....");

	if (Driver->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink((PUNICODE_STRING)&DevDN);
		IoDeleteDevice(Driver->DeviceObject);
	}
}




//*************************DeviceIOControl*********************************


NTSTATUS HDeviceRead(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	PIO_STACK_LOCATION Stack;
	int licznik, licznik1,count;
	char* buf;
	PVOID buf_out;
	ULONG_PTR ByteReturned;
	int BindingIndex;
	int Operacja;
	EHead_802_11* Packet=NULL;
	int i;
	PLIST_ENTRY PL;
	miniport* MP=NULL;
	BOOLEAN finded;
	USHORT *licz;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "HDeviceRead execute...");

	Stack = IoGetCurrentIrpStackLocation(irp);

	licznik = 0;
	licznik1 = 0;
	ByteReturned = 0;
	finded = FALSE;

	switch (Stack->MajorFunction)
	{
	case IRP_MJ_READ:
	{
		buf_out = irp->AssociatedIrp.SystemBuffer;
		licznik1 = Stack->Parameters.Read.ByteOffset.LowPart;
		licz = (USHORT*)&Stack->Parameters.Read.ByteOffset.HighPart;
		BindingIndex = (int)*licz;
		licz++;
		Operacja = (int)*licz;

		
		PL = MiniportsEntries.Flink;

		while (PL != &MiniportsEntries)
		{
			MP = (miniport*)CONTAINING_RECORD(PL, miniport, ListE);

			AcqSpinLock(&MP->SpinLock_, FALSE);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MP =%s", MP->MiniportName);
			
			if (MP->Index == BindingIndex)
			{
				finded = TRUE;
			//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MP finded... %s",MP->MiniportName);
				RelSpinLock(&MP->SpinLock_, FALSE);
				break;
			}
			
			RelSpinLock(&MP->SpinLock_, FALSE);
			PL = PL->Flink;
		}
	
		
		if (finded)
		{
			AcqSpinLock(&MP->SpinLock_, FALSE);

			if (Operacja == 0)
			{
				Packet = (EHead_802_11*)MP->RPacket;
				licznik = MP->RecvLicz;
			}
			else if (Operacja == 1)
			{
				Packet = (EHead_802_11*)MP->SPacket;
				licznik = MP->SendLicz;
			}
			else finded = FALSE;

			RelSpinLock(&MP->SpinLock_, FALSE);

			if (finded)
			{

				if (licznik1 != licznik)
				{
					if (licznik1 >= 1000)licznik1 = 0;

					if (licznik1 < licznik)
					{
						count = licznik - licznik1;

						AcqSpinLock(&MP->SpinLock_, FALSE);

						memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
						buf = (char*)buf_out;
						buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
						memcpy((void*)buf, (void*)&count, sizeof(int));

						RelSpinLock(&MP->SpinLock_, FALSE);
					}

					else
					{
						count = PACKET_COUNT - licznik1;

						AcqSpinLock(&MP->SpinLock_, FALSE);

						memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
						buf = (char*)buf_out;
						buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
						memcpy((void*)buf, (void*)&count, sizeof(int));

						RelSpinLock(&MP->SpinLock_, FALSE);

					}
					ByteReturned = Stack->Parameters.Read.Length;
				}
			}
		}
		
		break;
	}
	default: break;
	}

	irp->IoStatus.Information = ByteReturned;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceSkip(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceSkip execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceQueryInformation(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceQueryInformation execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceCreate(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceCreate execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceClose(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceClose execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS HDeviceCleanUp(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceCleanUp execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceIOControl(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	PIO_STACK_LOCATION stack;
	ULONG code;
	PVOID buf_in, buf_out;
	ULONG in_len, out_len;
	char str[25] = "";
	int rozmiar_buf;
	int i;
	PLIST_ENTRY PL;
	miniport* MP;
	Userminiport *UMP;

	stack = IoGetCurrentIrpStackLocation(irp);
	code = stack->Parameters.DeviceIoControl.IoControlCode;
	in_len = stack->Parameters.DeviceIoControl.InputBufferLength;
	out_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
	buf_in = irp->AssociatedIrp.SystemBuffer;
	buf_out = irp->UserBuffer;


	if (stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceIoControl execute...");
		switch(code)
		{
			case IO_MINIPORTS_INIT:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IO_DEV_INIT execute...");
				if (out_len < 5) break;
				memset(buf_out, 0, out_len);
				if (miniportsCount == 0)
				{
					sprintf(str, "BBBsB");
					memcpy(buf_out, str, strlen(str));
					break;
				}
				rozmiar_buf = miniportsCount * sizeof(Userminiport);
				if (rozmiar_buf > out_len)
				{
					sprintf(str, "AAAsA%d", rozmiar_buf);
					memcpy(buf_out, str, strlen(str));
					break;
				}
				PL = MiniportsEntries.Flink;
				i = 0;
				UMP = (Userminiport*)buf_out;
				while (PL != &MiniportsEntries)
				{
					MP = (miniport*)CONTAINING_RECORD(PL, miniport, ListE);
					memcpy(UMP->MiniportName, MP->MiniportName,250);
					UMP->HandleCount = MP->HandleCount;
					UMP->Index = MP->Index;
					UMP->miniportCount = miniportsCount;
					UMP->Hooked = 0;
					UMP->licznik = 0;
					PL = PL->Flink;
					if (PL != &MiniportsEntries)
						UMP++;
				}
				break;
			}
		}
	}
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDevicePnP(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
	PIO_STACK_LOCATION status;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DevicePnP execute...");
	status = IoGetCurrentIrpStackLocation(irp);
	if (status->MajorFunction== IRP_MJ_PNP)
	{
		switch (status->MinorFunction)
		{
			case  IRP_MN_START_DEVICE:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Start Device execute...");
			}
			case  IRP_MN_STOP_DEVICE:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Stop Device execute...");
			}

			case  IRP_MN_QUERY_STOP_DEVICE:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Query Stop Device execute...");
			}
			case  IRP_MN_QUERY_REMOVE_DEVICE:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Query Remove Device execute...");
			}

			case  IRP_MN_REMOVE_DEVICE:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Remove Device execute...");
			}
		}
	}
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}



void Sleep(ULONG milisec)
{
	KTIMER Timer;
	LARGE_INTEGER li;

	li.QuadPart = (LONGLONG)milisec;
	li.QuadPart = li.QuadPart * -10000;

	KeInitializeTimer(&Timer);
	KeSetTimer(&Timer, li,NULL);
	KeWaitForSingleObject((PVOID)&Timer, Executive, KernelMode, FALSE, NULL);
}


int CreateFile(wchar_t* FileName)
{
	wchar_t FName[250];
	UNICODE_STRING UStr;
	OBJECT_ATTRIBUTES Obj;
	HANDLE FHn;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS St;

	memset(FName, 0, 250 * sizeof(wchar_t));

	wcscpy(FName, L"\\DosDevices\\");
	wcscpy(&FName[wcslen(FName)], FileName);

	RtlInitUnicodeString(&UStr, FName);
	InitializeObjectAttributes(&Obj, &UStr, OBJ_CASE_INSENSITIVE, NULL, NULL);
	FHn = NULL;
	st = ZwCreateFile(&FHn, GENERIC_ALL, &Obj, &IoStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OVERWRITE_IF, 0, NULL, 0);
	if (st == STATUS_SUCCESS)

	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CreateFile success...");
		if (FHn != NULL)
			ZwClose(FHn);
	}
	return 0;
}