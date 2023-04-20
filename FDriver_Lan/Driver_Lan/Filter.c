
#include "Filter.h"

const wchar_t FilterName[] = L"FHacklan";
const wchar_t FriendlyName[] = L"Hacklan Network Monitor";
const wchar_t FilterGUID[] = L"{C81D7471-CEBA-4B35-958C-F4F504E5DABA}";

int RegisterFilterDriver(PDRIVER_OBJECT Driver)
{
	NDIS_STATUS Status;

	FilterRegistered = FALSE;
	FilterHandle = NULL;
	//RtlZeroMemory(&miniports, sizeof(miniports_));
	RtlZeroMemory(&FilterDriverChar, sizeof(NDIS_FILTER_DRIVER_CHARACTERISTICS));
	FilterDriverChar.MajorNdisVersion = NDIS_FILTER_MINIMUM_MAJOR_VERSION;
	FilterDriverChar.MinorNdisVersion = 0;
	FilterDriverChar.MajorDriverVersion = 1;
	FilterDriverChar.MinorDriverVersion = 0;
	FilterDriverChar.Header.Type = NDIS_OBJECT_TYPE_FILTER_DRIVER_CHARACTERISTICS;
	FilterDriverChar.Header.Revision = NDIS_FILTER_CHARACTERISTICS_REVISION_2;
	FilterDriverChar.Header.Size = NDIS_SIZEOF_FILTER_DRIVER_CHARACTERISTICS_REVISION_2;
	FilterDriverChar.Flags = 0;
	RtlInitUnicodeString(&FilterDriverChar.FriendlyName, FriendlyName);
	RtlInitUnicodeString(&FilterDriverChar.ServiceName, FilterName);
	RtlInitUnicodeString(&FilterDriverChar.UniqueName, FilterGUID);

	FilterDriverChar.SetOptionsHandler = &FilterSetOptions;
	FilterDriverChar.SetFilterModuleOptionsHandler = &SetFilterModuleOptions;
	FilterDriverChar.AttachHandler = &FilterAttach;
	FilterDriverChar.DetachHandler = &FilterDetach;
	FilterDriverChar.RestartHandler = &FilterRestart;
	FilterDriverChar.PauseHandler = &FilterPause;
	FilterDriverChar.SendNetBufferListsHandler = &FilterSendNetBufferLists;
	FilterDriverChar.SendNetBufferListsCompleteHandler = &FilterSendNetBufferListsComplete;
	FilterDriverChar.CancelSendNetBufferListsHandler = &FilterCancelSendNetBufferLists;
	FilterDriverChar.ReceiveNetBufferListsHandler = &FilterReceiveNetBufferLists;
	FilterDriverChar.ReturnNetBufferListsHandler = &FilterReturnNetBufferLists;
	FilterDriverChar.OidRequestHandler = &FilterOidRequest;
	FilterDriverChar.OidRequestCompleteHandler = &FilterOidRequestComplete;
	FilterDriverChar.CancelOidRequestHandler = &FilterCancelOidRequest;
	FilterDriverChar.DevicePnPEventNotifyHandler = &FilterDevicePnpEventNotify;
	FilterDriverChar.NetPnPEventHandler =&FilterNetPnpEvent;
	FilterDriverChar.StatusHandler = &FilterStatus;
	FilterDriverChar.DirectOidRequestHandler = NULL; // &FilterDirectOidRequest;
	FilterDriverChar.DirectOidRequestCompleteHandler = NULL; // &FilterDirectOidRequestComplete;
	FilterDriverChar.CancelDirectOidRequestHandler = NULL; // &FilterCancelDirectOidRequest;
	Status = NdisFRegisterFilterDriver(Driver, (NDIS_HANDLE)Driver, &FilterDriverChar, &FilterHandle);
	if(Status == NDIS_STATUS_SUCCESS)
	{
		FilterRegistered = TRUE;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter Driver Register  success......");
		return 0;
	} else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter Driver Register  failed status %x......",Status);

	return 100;
}

NDIS_STATUS FilterSetOptions(IN NDIS_HANDLE NdisDriverHandle, IN NDIS_HANDLE DriverContext)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter Set Options execute......");
	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS SetFilterModuleOptions(IN  NDIS_HANDLE FilterModuleContext)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter SetFilterModuleOptions execute......");
	return NDIS_STATUS_SUCCESS;
}
NDIS_STATUS FilterAttach(IN NDIS_HANDLE NdisFilterHandle, IN NDIS_HANDLE FilterDriverContext, IN PNDIS_FILTER_ATTACH_PARAMETERS AttachParameters)
{
	int i;
	miniport* _Miniport;
	PLIST_ENTRY PLE;
	filterModuleHandle * fmodule;
	int finded;
	NDIS_STATUS status;
	ANSI_STRING AS;
	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterAttach execute size name %d......", AttachParameters->BaseMiniportInstanceName->Length);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Miniport Name : %ws", AttachParameters->BaseMiniportInstanceName->Buffer);
	
	finded = 0;
	if (miniportsCount > 0)
	{
		PLE=(PLIST_ENTRY)MiniportsEntries.Flink;
		//_Miniport = (miniport*)CONTAINING_RECORD(PLE, miniport, ListE);

		//while (_Miniport != &MiniportsEntries)
		while(PLE!=&MiniportsEntries)
		{
			_Miniport = (miniport*)CONTAINING_RECORD(PLE, miniport, ListE);
			RtlUnicodeStringToAnsiString(&AS, AttachParameters->BaseMiniportInstanceName, TRUE);
			if (strcmp(_Miniport->MiniportName, AS.Buffer) == 0)
			{
				RtlFreeAnsiString(&AS);

				fmodule = (filterModuleHandle*)ExAllocatePool(NonPagedPool, sizeof(filterModuleHandle));
				fmodule->MiniportHandle = NdisFilterHandle;
				memcpy(&fmodule->FAP, AttachParameters, sizeof(NDIS_FILTER_ATTACH_PARAMETERS));
				fmodule->MP = (PVOID)_Miniport;
				fmodule->State = FilterAttaching;
				fmodule->RecvPacket = 0;
				fmodule->SendPacket = 0;
				fmodule->PowerState = NdisDeviceStateUnspecified;
				RtlZeroMemory(&fmodule->FilterAttrib, sizeof(NDIS_FILTER_ATTRIBUTES));
				fmodule->FilterAttrib.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
				fmodule->FilterAttrib.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
				fmodule->FilterAttrib.Header.Size = NDIS_SIZEOF_FILTER_ATTRIBUTES_REVISION_1;


				_Miniport->HandleCount++;

				InsertTailList(&_Miniport->ModeuleEntries, &fmodule->ListE);
				finded = 1;

				status = NdisFSetAttributes(NdisFilterHandle, (NDIS_HANDLE)fmodule, &fmodule->FilterAttrib);

				AcqSpinLock(&_Miniport->SpinLock_, FALSE);

				if (status == STATUS_SUCCESS)
					fmodule->State = FilterPaused;
				else fmodule->State = FilterDetached;
				
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Flter Module : %x -  Set Attributes...", fmodule->MiniportHandle);
				if (status == NDIS_STATUS_SUCCESS)
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Status Success");
				else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Status failed");

				RelSpinLock(&_Miniport->SpinLock_, FALSE);

				break;
			}
			RtlFreeAnsiString(&AS);
			PLE = _Miniport->ListE.Flink;
			//= (miniport*)_Miniport->ListE.Flink;
		}
	}
	if (finded == 0)
	{
		_Miniport = (miniport*)ExAllocatePool(NonPagedPool, sizeof(miniport));
		memset(_Miniport, 0, sizeof(miniport));
		//wcscpy(_Miniport->MiniportName, AttachParameters->BaseMiniportInstanceName->Buffer);
		RtlUnicodeStringToAnsiString(&AS, AttachParameters->BaseMiniportInstanceName, TRUE);
		if (AS.Length < 250)
			memcpy(_Miniport->MiniportName, AS.Buffer, AS.Length);
		RtlFreeAnsiString(&AS);
		_Miniport->SPacket = (EHead_802_11*)ExAllocatePool(NonPagedPool, sizeof(EHead_802_11) * PACKET_COUNT);
		_Miniport->RPacket = (EHead_802_11*)ExAllocatePool(NonPagedPool, sizeof(EHead_802_11) * PACKET_COUNT);
		_Miniport->SendCount = 0;
		_Miniport->RecvCount = 0;
		_Miniport->SendLicz = 0;
		_Miniport->RecvLicz = 0;

		NdisAllocateSpinLock(&_Miniport->SpinLock_);
		fmodule = (filterModuleHandle*)ExAllocatePool(NonPagedPool, sizeof(filterModuleHandle));
		fmodule->MiniportHandle = NdisFilterHandle;
		memcpy(&fmodule->FAP, AttachParameters, sizeof(NDIS_FILTER_ATTACH_PARAMETERS));
		fmodule->MP = (PVOID)_Miniport;
		fmodule->State = FilterAttaching;
		fmodule->RecvPacket = 0;
		fmodule->SendPacket = 0;
		fmodule->PowerState = NdisDeviceStateUnspecified;
		RtlZeroMemory(&fmodule->FilterAttrib, sizeof(NDIS_FILTER_ATTRIBUTES));
		fmodule->FilterAttrib.Header.Type = NDIS_OBJECT_TYPE_FILTER_ATTRIBUTES;
		fmodule->FilterAttrib.Header.Revision = NDIS_FILTER_ATTRIBUTES_REVISION_1;
		fmodule->FilterAttrib.Header.Size = NDIS_SIZEOF_FILTER_ATTRIBUTES_REVISION_1;

		InitializeListHead(&_Miniport->ModeuleEntries);

		_Miniport->HandleCount = 1;
		miniportsCount++;
		_Miniport->Index = miniportsCount;

		InsertTailList(&_Miniport->ModeuleEntries, &fmodule->ListE);
		InsertTailList(&MiniportsEntries, &_Miniport->ListE);

		status = NdisFSetAttributes(NdisFilterHandle, (NDIS_HANDLE)fmodule, &fmodule->FilterAttrib);
		AcqSpinLock(&_Miniport->SpinLock_, FALSE);

		if (status == STATUS_SUCCESS)
			fmodule->State = FilterPaused;
		else fmodule->State = FilterDetached;


		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Flter Module : %x -  Set Attributes...", fmodule->MiniportHandle);
		if (status == NDIS_STATUS_SUCCESS)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Status Success");
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Status failed");

		RelSpinLock(&_Miniport->SpinLock_, FALSE);
	}
	return NDIS_STATUS_SUCCESS;
}
void  FilterDetach(IN NDIS_HANDLE FilterModuleContext)
{
	filterModuleHandle* fmh;
	miniport* mp;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterDetach execute......");


	fmh = (filterModuleHandle*)FilterModuleContext;
	mp = (miniport*)fmh->MP;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter Module Name : %s",mp->MiniportName);

	RemoveEntryList(&fmh->ListE);
	
	ExFreePool(fmh);
	mp->HandleCount--;
	if (mp->HandleCount == 0)
	{
		RemoveEntryList(&mp->ListE);
		NdisFreeSpinLock(&mp->SpinLock_);
		ExFreePool(mp->SPacket);
		ExFreePool(mp->RPacket);
		ExFreePool(mp);
		if (miniportsCount > 0)
			miniportsCount--;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Miniport relased ");
	}
}

NDIS_STATUS FilterRestart(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_FILTER_RESTART_PARAMETERS RestartParameters)
{
	filterModuleHandle* fmh;
	miniport* MP;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterRestart execute......");

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	NdisAcquireSpinLock(&MP->SpinLock_);

	fmh->State = FilterRunning;
	fmh->PowerState = NdisDeviceStateD0;
	NdisReleaseSpinLock(&MP->SpinLock_);

	NdisFRestartComplete(fmh->MiniportHandle, NDIS_STATUS_SUCCESS);

	return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS FilterPause(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_FILTER_PAUSE_PARAMETERS PauseParameters)
{
	filterModuleHandle *fmh;
	miniport* MP;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	NdisAcquireSpinLock(&MP->SpinLock_);
	fmh->State = FilterPaused;
	NdisReleaseSpinLock(&MP->SpinLock_);

	NdisFPauseComplete(fmh->MiniportHandle);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterPause execute...... %s",((miniport*)fmh->MP)->MiniportName);
	return NDIS_STATUS_SUCCESS;
}

_Use_decl_annotations_ void FilterSendNetBufferLists(IN NDIS_HANDLE FilterModuleContext, IN PNET_BUFFER_LIST NetBufferList, IN NDIS_PORT_NUMBER PortNumber, IN ULONG SendFlags)
{
	filterModuleHandle* fmh;
	BOOLEAN isDispatch;
	PNET_BUFFER_LIST NBL;
	PNET_BUFFER  NB;
	PMDL mdl_ch;
	PVOID Buffer;
	int ByteCount;
	miniport* MP;
	ULONG SPacket;

	ULONG SFlags;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;
	NBL = NetBufferList;


	isDispatch = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendFlags);

	SPacket = 0;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterSendNetBufferLists execute....");

	AcqSpinLock(&MP->SpinLock_, isDispatch);

	if (fmh->State != FilterRunning || fmh->PowerState!= NdisDeviceStateD0)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterSendNetBufferLists Paused-----");
		RelSpinLock(&MP->SpinLock_, isDispatch);
		while (NBL != NULL)
		{
			//NBL->Status = NDIS_STATUS_PAUSED;
			NET_BUFFER_LIST_STATUS(NBL) = NDIS_STATUS_PAUSED;
			NBL = NET_BUFFER_LIST_NEXT_NBL(NBL);
		}
		if (isDispatch)
			SFlags = NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL; else SFlags = 0;
		NdisFSendNetBufferListsComplete(fmh->MiniportHandle, NetBufferList, SFlags);
	}
	else
	{
		RelSpinLock(&MP->SpinLock_, isDispatch);

		while (NBL != NULL)
		{
			SPacket++;

			NB = NET_BUFFER_LIST_FIRST_NB(NBL);
			while (NB != NULL)
			{
				mdl_ch = NET_BUFFER_FIRST_MDL(NB);

				while (mdl_ch != NULL)
				{
					Buffer = NULL;

					//if (mdl_ch->MdlFlags& MDL_SOURCE_IS_NONPAGED_POOL)
					//MmBuildMdlForNonPagedPool(mdl_ch);
					
					NdisQueryMdl(mdl_ch,&Buffer,&ByteCount, NormalPagePriority)
					//Buffer = MmGetMdlVirtualAddress(mdl_ch);
					if (Buffer != NULL && ByteCount > 0 && ByteCount < sizeof(EHead_802_11))
					{
						AcqSpinLock(&MP->SpinLock_, isDispatch);

						if (MP->SendCount > 0 || MP->SendCount < PACKET_COUNT)
						{

							//	MmProbeAndLockPages(mdl_ch, KernelMode, IoReadAccess);
								//memset(&MiniP->Packet[MiniP->SendCount], 0, sizeof(EHead_802_11));
							RtlZeroMemory(&MP->SPacket[MP->SendCount], sizeof(EHead_802_11));
							//memcpy(&MiniP->Packet[MiniP->SendCount], Buffer, mdl_ch->ByteCount);
							RtlCopyMemory(&MP->SPacket[MP->SendCount], Buffer, ByteCount);


							//	MmUnlockPages(mdl_ch);

								//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterSendNetBufferLists %s SCount :  %d", MiniP->MiniportName, MiniP->SendCount);

							if (MP->SendCount == PACKET_COUNT - 1) MP->SendCount = 0;
							else  MP->SendCount++;
							
							if (MP->SendLicz == PACKET_COUNT) MP->SendLicz = 1;
							else MP->SendLicz++;
							
						}

						RelSpinLock(&MP->SpinLock_, isDispatch);
					}
					mdl_ch = mdl_ch->Next;
				}
				NB = NET_BUFFER_NEXT_NB(NB);
			}
			NBL = NET_BUFFER_LIST_NEXT_NBL(NBL);
		}
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterSendNetBufferLists SPacket = %d",fmh->SendPacket);
		AcqSpinLock(&MP->SpinLock_, isDispatch);

		fmh->SendPacket = fmh->SendPacket + SPacket;

		RelSpinLock(&MP->SpinLock_, isDispatch);

		NdisFSendNetBufferLists(fmh->MiniportHandle, NetBufferList, PortNumber, SendFlags);
	}
	


}

_Use_decl_annotations_ void FilterSendNetBufferListsComplete(IN NDIS_HANDLE FilterModuleContext, IN PNET_BUFFER_LIST NetBufferList, IN ULONG SendCompleteFlags)
{
	filterModuleHandle* fmh;
	PNET_BUFFER_LIST NBL;
	BOOLEAN isDispatch;
	ULONG SPacket;
	miniport* MP;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	NBL = NetBufferList;

	isDispatch = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(SendCompleteFlags);
	//MiniP = (miniport*)fmh->MP;
	
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterSendNetBufferListsComplete MiniportName= %ws , isDispatch: %d , Flags %d",MiniP->MiniportName,isDispatch, SendCompleteFlags);

	SPacket = 0;
	while (NBL != NULL)
	{
		SPacket++;
		NBL = NET_BUFFER_LIST_NEXT_NBL(NBL);
	
	}
	AcqSpinLock(&MP->SpinLock_, isDispatch);
	fmh->SendPacket = fmh->SendPacket - SPacket;
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterSendNetBufferListsComplete MiniportName= %ws",((miniport*)fmh->MP)->MiniportName);
	RelSpinLock(&MP->SpinLock_, isDispatch);

	NdisFSendNetBufferListsComplete(fmh->MiniportHandle, NetBufferList, SendCompleteFlags);
}

_Use_decl_annotations_ void FilterCancelSendNetBufferLists(IN NDIS_HANDLE FilterModuleContext, IN PVOID CancelId)
{
	filterModuleHandle* fmh;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterCancelSendNetBufferLists execute......");

	fmh = (filterModuleHandle*)FilterModuleContext;
	NdisFCancelSendNetBufferLists(fmh->MiniportHandle, CancelId);
}

void FilterReceiveNetBufferLists(IN NDIS_HANDLE FilterModuleContext, IN PNET_BUFFER_LIST NetBufferLists, IN NDIS_PORT_NUMBER PortNumber, IN ULONG NumberOfNetBufferLists, IN ULONG ReceiveFlags)
{
	filterModuleHandle* fmh;
	ULONG RetFlags;
	BOOLEAN isDispatch;
	PNET_BUFFER  NB;
	PMDL mdl_ch;
	PVOID Buffer;
	int ByteCount;
	PNET_BUFFER_LIST NBL;
	miniport* MP;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterReceiveNetBufferLists execute......");

	isDispatch= NDIS_TEST_SEND_AT_DISPATCH_LEVEL(ReceiveFlags);
	NBL = NetBufferLists;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	AcqSpinLock(&MP->SpinLock_, isDispatch);

	if (fmh->State != FilterRunning || fmh->PowerState != NdisDeviceStateD0)
	{
		RelSpinLock(&MP->SpinLock_, isDispatch);
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter Recive is  Not Running State.....");

		if (NDIS_TEST_RECEIVE_CAN_PEND(ReceiveFlags))
		{
			RetFlags = 0;
			if (NDIS_TEST_SEND_AT_DISPATCH_LEVEL(ReceiveFlags))
			{
				NDIS_SET_RETURN_FLAG(RetFlags, NDIS_RECEIVE_FLAGS_DISPATCH_LEVEL);
			}

			NdisFReturnNetBufferLists(fmh->MiniportHandle, NetBufferLists, ReceiveFlags);
		}
	}
	else
	{
		RelSpinLock(&MP->SpinLock_, isDispatch);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter  is Running State.....");

		if (NumberOfNetBufferLists >= 1)
		{
			AcqSpinLock(&MP->SpinLock_, isDispatch)

			fmh->RecvPacket = fmh->RecvPacket + NumberOfNetBufferLists;

			RelSpinLock(&MP->SpinLock_, isDispatch);
			
			
			while (NBL != NULL)
			{
				NB = NET_BUFFER_LIST_FIRST_NB(NBL);

				while (NB != NULL)
				{
					mdl_ch = NET_BUFFER_FIRST_MDL(NB);

					while (mdl_ch != NULL)
					{
						Buffer = NULL;

						//if (mdl_ch->MdlFlags& MDL_SOURCE_IS_NONPAGED_POOL)
						//MmBuildMdlForNonPagedPool(mdl_ch);

						NdisQueryMdl(mdl_ch, &Buffer, &ByteCount, NormalPagePriority)
							//Buffer = MmGetMdlVirtualAddress(mdl_ch);
						if (Buffer != NULL && ByteCount>0 && ByteCount< sizeof(EHead_802_11))
						{
							AcqSpinLock(&MP->SpinLock_, isDispatch);

							if (MP->RecvCount > 0 || MP->RecvCount < PACKET_COUNT)
							{

								//	MmProbeAndLockPages(mdl_ch, KernelMode, IoReadAccess);
									//memset(&MiniP->Packet[MiniP->SendCount], 0, sizeof(EHead_802_11));
								RtlZeroMemory(&MP->RPacket[MP->RecvCount], sizeof(EHead_802_11));
								//memcpy(&MiniP->Packet[MiniP->SendCount], Buffer, mdl_ch->ByteCount);
								RtlCopyMemory(&MP->RPacket[MP->RecvCount], Buffer, ByteCount);


								//	MmUnlockPages(mdl_ch);

								//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterReceiveNetBufferLists %s RecvCount :  %d", MP->MiniportName, MP->RecvCount);

								if (MP->RecvCount == PACKET_COUNT - 1) MP->RecvCount = 0;
								else MP->RecvCount++;

								if (MP->RecvLicz == PACKET_COUNT) MP->RecvLicz = 1;
								else MP->RecvLicz++;
								
							}

							RelSpinLock(&MP->SpinLock_, isDispatch);
						}
						mdl_ch = mdl_ch->Next;
					}
					NB = NET_BUFFER_NEXT_NB(NB);
				}
				NBL = NET_BUFFER_LIST_NEXT_NBL(NBL);
			}
			
			NdisFIndicateReceiveNetBufferLists(fmh->MiniportHandle, NetBufferLists, PortNumber, NumberOfNetBufferLists, ReceiveFlags);
			
			if (NDIS_TEST_RECEIVE_CANNOT_PEND(ReceiveFlags))
			{
				AcqSpinLock(&MP->SpinLock_, isDispatch)

				fmh->RecvPacket = fmh->RecvPacket - NumberOfNetBufferLists;

				RelSpinLock(&MP->SpinLock_, isDispatch);

				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NDIS_RECEIVE_FLAGS_RESOURCES is set");
			} 

		}

	}


}

void FilterReturnNetBufferLists(IN NDIS_HANDLE FilterModuleContext, IN PNET_BUFFER_LIST NetBufferLists, IN ULONG ReturnFlags)
{
	filterModuleHandle* fmh;
	BOOLEAN isDispatch;
	ULONG NumberBufferList;
	PNET_BUFFER_LIST NBL;
	miniport* MP;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterReturnNetBufferLists execute......");

	isDispatch = NDIS_TEST_SEND_AT_DISPATCH_LEVEL(ReturnFlags);

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	NBL = NetBufferLists;

	NumberBufferList = 0;

	while (NBL)
	{
		NumberBufferList++;

		NBL = NET_BUFFER_LIST_NEXT_NBL(NBL);
	}

	NdisFReturnNetBufferLists(fmh->MiniportHandle, NetBufferLists, ReturnFlags);

	AcqSpinLock(&MP->SpinLock_, isDispatch);

	fmh->RecvPacket = fmh->RecvPacket - NumberBufferList;

	RelSpinLock(&MP->SpinLock_, isDispatch);


}

NDIS_STATUS FilterOidRequest(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_OID_REQUEST OidRequest)
{
	filterModuleHandle* fmh;
	PNDIS_OID_REQUEST COidRequest = 0;
	NDIS_OID_REQUEST** Context;
	NDIS_STATUS status;
	miniport* MP;

//	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterOidRequest execute......");

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	//COidRequest = (PNDIS_OID_REQUEST)ExAllocatePool(NonPagedPool, sizeof(NDIS_OID_REQUEST));
	if (OidRequest->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_PACKET_FILTER && OidRequest->RequestType == NdisRequestSetInformation)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterOidRequest OID_GEN_CURRENT_PACKET_FILTER......");
	}
	status=NdisAllocateCloneOidRequest(fmh->MiniportHandle, OidRequest, 'FHck', &COidRequest);
	
	if (status == NDIS_STATUS_SUCCESS)
	{
		Context = (NDIS_OID_REQUEST**)(&COidRequest->SourceReserved[0]);
		*Context = OidRequest;
		COidRequest->RequestId = OidRequest->RequestId;

		AcqSpinLock(&MP->SpinLock_, FALSE);
		
		fmh->PRequest = COidRequest;

		RelSpinLock(&MP->SpinLock_, FALSE);

		status=NdisFOidRequest(fmh->MiniportHandle, COidRequest);

		if(status!=STATUS_PENDING)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NdisFOidRequest  !!!!STATUS_PENDING ");
			FilterOidRequestComplete(fmh, COidRequest, status);
			status = NDIS_STATUS_PENDING;
			
		}//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Statu PENDING");
	}
	return status;
}

void FilterOidRequestComplete(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_OID_REQUEST OidRequest, IN NDIS_STATUS Status)
{
	filterModuleHandle* fmh;
	NDIS_OID_REQUEST *OrigReq;
	NDIS_OID_REQUEST ** Context;
	int i;
	PVOID Buffer=NULL;
	miniport* MP;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterOidRequestComplete execute......");

	if (Status == NDIS_STATUS_INDICATION_REQUIRED)
	{
		CreateFile(L"C:\\IndicationRequest.txt");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NDIS_STATUS_INDICATION_REQUIRED ");
	}


	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	Context = (NDIS_OID_REQUEST**)(&OidRequest->SourceReserved[0]);
	OrigReq = *Context;

	if (OrigReq == NULL)
	{

		return;
	}

	AcqSpinLock(&MP->SpinLock_, FALSE);

	if (fmh->PRequest == OidRequest)
	{
		fmh->PRequest = NULL;
		RelSpinLock(&MP->SpinLock_, FALSE);
		
		if (OidRequest->RequestType == NdisRequestMethod)
		{
		
			OrigReq->DATA.METHOD_INFORMATION.OutputBufferLength = OidRequest->DATA.METHOD_INFORMATION.OutputBufferLength;
			OrigReq->DATA.METHOD_INFORMATION.BytesRead = OidRequest->DATA.METHOD_INFORMATION.BytesRead;
			OrigReq->DATA.METHOD_INFORMATION.BytesWritten = OidRequest->DATA.METHOD_INFORMATION.BytesWritten;
			OrigReq->DATA.METHOD_INFORMATION.BytesNeeded = OidRequest->DATA.METHOD_INFORMATION.BytesNeeded;
		}
		else if (OidRequest->RequestType == NdisRequestSetInformation)
		{
			OrigReq->DATA.SET_INFORMATION.BytesRead = OidRequest->DATA.SET_INFORMATION.BytesRead;
			OrigReq->DATA.SET_INFORMATION.BytesNeeded = OidRequest->DATA.SET_INFORMATION.BytesNeeded;
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NdisRequestSetInformation.");
		}
			
		else if (OidRequest->RequestType == NdisRequestQueryInformation || OidRequest->RequestType == NdisRequestQueryStatistics)
		{
			OrigReq->DATA.QUERY_INFORMATION.BytesWritten = OidRequest->DATA.QUERY_INFORMATION.BytesWritten;
			OrigReq->DATA.QUERY_INFORMATION.BytesNeeded = OidRequest->DATA.QUERY_INFORMATION.BytesNeeded;
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NdisRequestQueryInformation.");
		}

		(*Context) = NULL;
		NdisFreeCloneOidRequest(fmh->MiniportHandle, OidRequest);
		NdisFOidRequestComplete(fmh->MiniportHandle, OrigReq, Status);

	}else RelSpinLock(&MP->SpinLock_, FALSE);
}

void FilterCancelOidRequest(IN NDIS_HANDLE FilterModuleContext, IN PVOID RequestId)
{
	filterModuleHandle* fmh;
	NDIS_OID_REQUEST** Context;
	PNDIS_OID_REQUEST OrigReq;
	miniport* MP;

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterCancelOidRequest execute......");
	
	OrigReq = NULL;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	AcqSpinLock(&MP->SpinLock_, FALSE);

	if (fmh->PRequest != NULL)
	{
		Context = (NDIS_OID_REQUEST**)&(fmh->PRequest->SourceReserved[0]);
		OrigReq = *Context;
	}
	if (OrigReq != NULL && OrigReq->RequestId == RequestId)
	{
		RelSpinLock(&MP->SpinLock_, FALSE);
		NdisFCancelOidRequest(fmh->MiniportHandle, RequestId);
	}
	else RelSpinLock(&MP->SpinLock_, FALSE);
}

void FilterDevicePnpEventNotify(IN NDIS_HANDLE FilterModuleContext, IN PNET_DEVICE_PNP_EVENT NetDevicePnPEvent)
{
	filterModuleHandle* fmh;

	//CreateFile(L"C:\\FilterDevicePnpEventNotify.txt");

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterDevicePnpEventNotify execute......");

	fmh = (filterModuleHandle*)FilterModuleContext;
	NdisFDevicePnPEventNotify(fmh->MiniportHandle, NetDevicePnPEvent);
}

NDIS_STATUS FilterNetPnpEvent(IN NDIS_HANDLE FilterModuleContext, IN PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification)
{
	NDIS_DEVICE_POWER_STATE DevPowerState;
	filterModuleHandle* fmh;
	miniport* MP;

	fmh = (filterModuleHandle*)FilterModuleContext;
	MP = (miniport*)fmh->MP;

	switch (NetPnPEventNotification->NetPnPEvent.NetEvent)
	{
		case NetEventPause:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter Event Pause......%s",((miniport*)fmh->MP)->MiniportName);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "MP Name: %s",((miniport*)fmh->MP)->MiniportName);
			AcqSpinLock(&MP->SpinLock_, FALSE);
			fmh->State = FilterPaused;
			RelSpinLock(&MP->SpinLock_, FALSE);

			break;
		}
		case NetEventRestart:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterNetPnpEvent Restart......");
			break;
		}
		case NetEventSetPower:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "FilterNetPnpEvent NetEventSetPower......");
			if (NetPnPEventNotification->NetPnPEvent.Buffer != NULL && NetPnPEventNotification->NetPnPEvent.BufferLength == sizeof(NDIS_DEVICE_POWER_STATE))
			{
				DevPowerState = *((NDIS_DEVICE_POWER_STATE*)NetPnPEventNotification->NetPnPEvent.Buffer);
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter change powes state success......");
				AcqSpinLock(&MP->SpinLock_, FALSE);
				fmh->PowerState = DevPowerState;
				RelSpinLock(&MP->SpinLock_, FALSE);
			}

			break; 
		}

	}

	fmh = (filterModuleHandle*)FilterModuleContext;
	NdisFNetPnPEvent(fmh->MiniportHandle, NetPnPEventNotification);

	return NDIS_STATUS_SUCCESS;
}

void FilterStatus(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_STATUS_INDICATION StatusIndication)
{
	filterModuleHandle* fmh;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterStatus execute......");
	if (StatusIndication->StatusCode == NDIS_STATUS_INDICATION_REQUIRED)
	{
		CreateFile(L"C:\\IndicationStatus.txt");
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "NDIS_STATUS_INDICATION_REQUIRED ");
	}

	fmh = (filterModuleHandle*)FilterModuleContext;
	NdisFIndicateStatus(fmh->MiniportHandle, StatusIndication);
}

NDIS_STATUS FilterDirectOidRequest(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_OID_REQUEST OidRequest)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterDirectOidRequest execute......");
	return NDIS_STATUS_SUCCESS;
}

void FilterDirectOidRequestComplete(IN NDIS_HANDLE FilterModuleContext, IN PNDIS_OID_REQUEST OidRequest, IN NDIS_STATUS Status)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterDirectOidRequestComplete execute......");
}

void FilterCancelDirectOidRequest(IN NDIS_HANDLE FilterModuleContext, IN PVOID RequestId)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Filter FilterCancelDirectOidRequest execute......");
}