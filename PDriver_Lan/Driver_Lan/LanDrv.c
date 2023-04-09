
#pragma once

#include "Lan.h"
#include "LanDrv.h"





NDIS_DEVICE_OBJECT_ATTRIBUTES ObjectAttributes;


#pragma NDIS_INIT_FUNCTION(DriverEntry)



_Use_decl_annotations_ NTSTATUS DriverEntry(IN PDRIVER_OBJECT Driver, IN PUNICODE_STRING str)
{
	Driver->DriverUnload = &OnUnload;

	Init_Adapter = 0;

	//Driver_Hack = Driver;
	
	adapterCount = 0;

	PACKET_COUNT = 1000;

	DataSize = sizeof(EHead_802_11);
	PoolHandle = NULL;
	Buffer_List = NULL;
	allocated = 0;

	NdisInitializeEvent(&CloseEvent);
	NdisInitializeEvent(&RequestEvent);

	Device_Hack = NULL;

	AllocateBufferList();

	for (i = 0; i < 25; i++)
	{
		
		memset(&cards[i],0,sizeof(net_cards));
	}

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolDrv start....Path Driver : %ws\n",Driver->DriverExtension->ServiceKeyName.Buffer);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolDrv start....Path str : %ws\n", str->Buffer);

	GetNetworkCards();

	for (i = 0; i < adapterCount; i++)
	{
		//cards[i].Packet = (EHead_802_11*)ExAllocatePool(NonPagedPool, PACKET_COUNT * sizeof(EHead_802_11));
		cards[i].licznik = 0;
		cards[i].openIndex = 0;
		cards[i].BindingContext = i;
		cards[i].Buffer = (NDIS_HANDLE)ExAllocatePool(NonPagedPool, PACKET_COUNT * sizeof(EHead_802_11));
		cards[i].isOpened = FALSE;
		NdisAllocateSpinLock(&cards[i].SpinLockA);
		DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "Zaalokowano Bufor pakietow dla karty %s",cards[i].Description);
	}

	
	
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
	
	
	i = RegisterProtocol(NULL, Service_Name);
	if (i == 0)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Zarejestrowano protokó³ pomyœlnie z nazwa %ws\n", Service_Name.Buffer);
		Init_Adapter = 1;
	}
	else
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mo¿na zarejestrowaæ protoko³u kod b³êdu : %d\n", i);
		//return STATUS_UNSUCCESSFUL;
	}


	return STATUS_SUCCESS;

}


//*******************************Service Stop******************************



VOID OnUnload(IN PDRIVER_OBJECT Driver)
{
	NDIS_STATUS status;
	int i;
	
	CloseAdapters();
	
	if (Init_Adapter == 1)
	{

		NdisDeregisterProtocolDriver(Protocol_Handle);
		Init_Adapter = 0;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protokó³ wyrejestrowano pomyœlnie");
	}
	

	for (i = 0; i < adapterCount; i++)
	{

		ExFreePool(cards[i].Buffer);
		NdisFreeSpinLock(&cards[i].SpinLockA);
		DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "Zwolniono Bufor pakietow dla karty %s", cards[i].Description);
	}

	if (Buffer_List != NULL)
	{	
		if (Buffer_List->Context != NULL)
		{
			NdisFreeNetBufferListContext(Buffer_List, MEMORY_ALLOCATION_ALIGNMENT);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Context Buffer_List is relased.....");
		}
		NdisFreeNetBufferList(Buffer_List);

		if (PoolHandle != NULL)
		{
			NdisFreeNetBufferListPool(PoolHandle);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PoolHandle and Buffer_List relased....");
		}
		
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
	int licznik,licznik1,OP_AD,count;
	char *buf;
	PVOID buf_out;
	ULONG_PTR ByteReturned;
	ULONG64 BindingContext;
	EHead_802_11 *Packet;
	int i;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "HDeviceRead execute...");

	Stack=IoGetCurrentIrpStackLocation(irp);

	licznik = 0;
	licznik1 = 0;
	ByteReturned = 0;
	OP_AD = 0;

	switch (Stack->MajorFunction)
	{
		case IRP_MJ_READ:
		{
			buf_out = irp->AssociatedIrp.SystemBuffer;
			licznik1 = Stack->Parameters.Read.ByteOffset.LowPart;
			BindingContext = Stack->Parameters.Read.ByteOffset.HighPart;

			for (i = 0; i < adapterCount; i++)
			{
				NdisAcquireSpinLock(&cards[i].SpinLockA);

				if (BindingContext == cards[i].BindingContext)
				{
					OP_AD = i;
					NdisReleaseSpinLock(&cards[i].SpinLockA);
					break;
				}
				NdisReleaseSpinLock(&cards[i].SpinLockA);
			}

			NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

			Packet = (EHead_802_11*)cards[OP_AD].Buffer;
			licznik = cards[OP_AD].licznik;

			NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

			if (licznik1 != licznik)
			{
				if (licznik1 >= 1000)licznik1 = 0;

				if (licznik1 < licznik)
				{
					count = licznik - licznik1;

					NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

					memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
					buf = (char*)buf_out;
					buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
					memcpy((void*)buf, (void*)&count, sizeof(int));

					NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);
				}

				else
				{
					count = PACKET_COUNT - licznik1;
					
					NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

					memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
					buf = (char*)buf_out;
					buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
					memcpy((void*)buf, (void*)&count, sizeof(int));

					NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

				}
				ByteReturned = Stack->Parameters.Read.Length;
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
	wchar_t p[] = L"C:\\DeviceQueryInformation.txt";
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceQueryInformation execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceCreate(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceCreate execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS HDeviceClose(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceClose execute...");
	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
NTSTATUS HDeviceCleanUp(__in DEVICE_OBJECT* Device, __inout IRP* irp)
{
//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceCleanUp execute...");
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
	char str[25] = "";
	ULONG in_len, out_len;
	Dev_Lan* bt,bt1;
	int i,count;
	ULONG64 BindingContext;
	char* buf;

	PNET_BUFFER NetBuffer;
	PMDL mdl_;
	PVOID EBuffer = NULL;
	EHead_802_11* Packet;
	int licznik,licznik1;
	int OP_AD;
	ULONG ByteReturned;
	

	ByteReturned = 0;
	licznik1 = 0;

	stack = IoGetCurrentIrpStackLocation(irp);

	code = stack->Parameters.DeviceIoControl.IoControlCode;
	in_len = stack->Parameters.DeviceIoControl.InputBufferLength;
	out_len = stack->Parameters.DeviceIoControl.OutputBufferLength;
	buf_in = irp->AssociatedIrp.SystemBuffer;
	buf_out = irp->UserBuffer;


	if (stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DeviceIoControl execute...");
		switch (code)
		{
			case IO_DEV_INIT:
			{
				if (out_len <5) break;
				memset(buf_out, 0, out_len);
				if (adapterCount == 0)
				{
					sprintf(str, "BBBsB");
					memcpy(buf_out, str, strlen(str));
					ByteReturned = strlen(str);
					break;
				}
				rozmiar_buf = adapterCount * sizeof(Dev_Lan);
				if (rozmiar_buf > out_len)
				{
					sprintf(str, "AAAsA%d", rozmiar_buf);
					memcpy(buf_out, str, strlen(str));
					ByteReturned = strlen(str);
				}
				else
				{
			
					bt = (Dev_Lan*)buf_out;
					
					for (i = 0; i < adapterCount; i++)
					{
						NdisAcquireSpinLock(&cards[i].SpinLockA);

						memcpy(bt->name, cards[i].Description, 150);
						memcpy(bt->service_name, cards[i].ServiceName, 150);
						bt->BindingContext = cards[i].BindingContext;
						bt->NetCardsCount = adapterCount;
						bt->ModeCap = cards[i].ModeCap;
						bt->NetMonSuported = cards[i].NetMonSuported;
						bt->MtuSize = cards[i].BindParam.MtuSize;
						bt->MaxXmitLinkSpeed = cards[i].BindParam.MaxXmitLinkSpeed;
						bt->XmitLinkSpeed = cards[i].BindParam.XmitLinkSpeed;
						bt->MaxRcvLinkSpeed = cards[i].BindParam.MaxRcvLinkSpeed;
						bt->MacAddressLength = cards[i].BindParam.MacAddressLength;
						bt->PhysicalMediumType = (PHYSICALMEDIUM)cards[i].BindParam.PhysicalMediumType;
						bt->MediaConnectState = (MEDIA_CONNECT_STATE)cards[i].BindParam.MediaConnectState;
						memcpy(bt->CurrentMacAddress, cards[i].BindParam.CurrentMacAddress, 32);

						NdisReleaseSpinLock(&cards[i].SpinLockA);
						if (i < adapterCount - 1) bt++;
					}

					ByteReturned = sizeof(net_cards) * adapterCount;
				}
				break;
			}
			case IO_OPEN_ADAPTER:
			{
				if (out_len < 5)break;
				memset(buf_out, 0, out_len);
				if (in_len < sizeof(Dev_Lan))
				{
					sprintf(str, "AAAsA");
					memcpy(buf_out, str, strlen(str));
					ByteReturned = strlen(str);
					break;
				}
				if (Init_Adapter == 1)
				{
					bt = (Dev_Lan*)buf_in;
					
					if (OpenAdapter(*bt) == 0)
					{
						sprintf(str, "Adapter O");
						memcpy(buf_out, str, strlen(str));
						ByteReturned = strlen(str);
					}
				}
				break;
			}
			
			case IO_CLOSE_ADAPTER:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IO_CLOSE_ADAPTER execute ");
				bt = (Dev_Lan*)buf_in;
				for (i = 0; i < adapterCount; i++)
				{
					if (bt->BindingContext == cards[i].BindingContext)
					{
						DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter to close finded i = %d ",i);
						OP_AD = i;
						break;
					}
				}
				if (cards[OP_AD].openIndex > 0)
				{
					NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);
					cards[OP_AD].openIndex--;
					NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Openindex == %d ", cards[OP_AD].openIndex);
				}
				break;
			}
				
			case IO_SET_RECIVE_HANDLER :
			{
				buf = (char*)buf_in;
				BindingContext = *((ULONG64*)buf);
				buf = buf + sizeof(ULONG64);
				licznik1 = *((int*)buf);

				for (i = 0; i < adapterCount; i++)
				{
					NdisAcquireSpinLock(&cards[i].SpinLockA);

					if (BindingContext == cards[i].BindingContext)
					{
						OP_AD = i;
						NdisReleaseSpinLock(&cards[i].SpinLockA);
						break;
					}
					NdisReleaseSpinLock(&cards[i].SpinLockA);
				}

				NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);
			
				Packet = (EHead_802_11*) cards[OP_AD].Buffer;
				licznik = cards[OP_AD].licznik;

				NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

				if (licznik1 != licznik)
				{
					if (licznik1 >= 1000)licznik1 = 0;

					if (licznik1 < licznik)
					{
						count = licznik - licznik1;

						NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

						memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
						buf = (char*)buf_out;
						buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
						memcpy((void*)buf, (void*)&count, sizeof(int));

						NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

						ByteReturned = (count * sizeof(EHead_802_11))+ sizeof(int);
						//licznik1 = cards[Open_Adapter].licznik;
					}
						
					else
					{
						count = PACKET_COUNT - licznik1;
						if (count > 0)
						{
							NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

							memcpy(buf_out, &Packet[licznik1], count * sizeof(EHead_802_11));
							buf = (char*)buf_out;
							buf = buf + (PACKET_COUNT * sizeof(EHead_802_11));
							memcpy((void*)buf, (void*)&count, sizeof(int));

							NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

							ByteReturned = (count * sizeof(EHead_802_11)) + sizeof(int);
						}
						
					}
			
				}
			//	NdisReleaseSpinLock(&cards[Open_Adapter].SpinLockA);
				break;
			}
			case IO_SEND_PACKET:
			{
				
				buf = (char*)buf_in;
				BindingContext = *((ULONG64*)buf);
				buf = buf + sizeof(ULONG64);
			
				for (i = 0; i < adapterCount; i++)
				{
					NdisAcquireSpinLock(&cards[i].SpinLockA);

					if (BindingContext == cards[i].BindingContext)
					{
						OP_AD = i;
						NdisReleaseSpinLock(&cards[i].SpinLockA);
						break;
					}

					NdisReleaseSpinLock(&cards[i].SpinLockA);
				}
			//	NdisAcquireSpinLock(&cards[Open_Adapter].SpinLockA);
				if (cards[OP_AD].openIndex>0)
				{
					mdl_ = NULL;
					NetBuffer = NULL;
					EBuffer = NULL;
					Buffer_List->Next = NULL;
					NetBuffer = NET_BUFFER_LIST_FIRST_NB(Buffer_List);
					if (NetBuffer != NULL)
					{
						NetBuffer->Next = NULL;
						DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Packet is sending......");
						mdl_ = NET_BUFFER_CURRENT_MDL(NetBuffer);
						if (mdl_ != NULL)
						{
							EBuffer = MmGetMdlVirtualAddress(mdl_);
							if (EBuffer != NULL)
							{
								memset(EBuffer, 0, mdl_->ByteCount);
								memcpy(EBuffer,buf, mdl_->ByteCount);
								NetBuffer->DataLength = mdl_->ByteCount;
								Buffer_List->SourceHandle = cards[OP_AD].Adapter_Handle;
								DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Data size=%d  ......  Data offset=%d......Mdl size = %d...........Mdl Offset=%d", NetBuffer->DataLength, NetBuffer->DataOffset, mdl->ByteCount, mdl_->ByteOffset);
								NdisSendNetBufferLists(cards[OP_AD].Adapter_Handle, Buffer_List, NDIS_DEFAULT_PORT_NUMBER,0);
								DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Packet is sended......");
							}
						}
					}
					
				}
			//	NdisReleaseSpinLock(&cards[Open_Adapter].SpinLockA);
				break;
			}

			case IO_QUERY_CONECTED:
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IO_QUERY_CONECTED execute ");
				bt = (Dev_Lan*)buf_in;
				for (i = 0; i < adapterCount; i++)
				{
					NdisAcquireSpinLock(&cards[i].SpinLockA);

					if (bt->BindingContext == cards[i].BindingContext)
					{
						DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter %s - openindex = %d ",cards[i].Description,cards[i].openIndex);
						memcpy(buf_out, &cards[i].openIndex, sizeof(int));
						NdisReleaseSpinLock(&cards[i].SpinLockA);
						break;
					}
					NdisReleaseSpinLock(&cards[i].SpinLockA);
				}
				break;
			}
		}
	}
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
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
	KeSetTimer(&Timer, li, NULL);
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
	st=ZwCreateFile(&FHn, GENERIC_ALL, &Obj, &IoStatus, NULL, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OVERWRITE_IF, 0, NULL, 0);
	if (st == STATUS_SUCCESS)

	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "CreateFile success...");
		if (FHn != NULL)
			ZwClose(FHn);
	}
	return 0;
}





