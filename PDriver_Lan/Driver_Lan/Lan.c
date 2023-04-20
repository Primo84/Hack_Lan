
#include "Lan.h"

NDIS_MEDIUM medium[] = {NdisMedium802_3,NdisMediumNative802_11,NdisMedium802_5,NdisMediumFddi,NdisMediumWan,NdisMediumWirelessWan };

wchar_t key[] = L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards";

int GetNetworkCards()
{
	NDIS_STATUS st;
	int i;
	OBJECT_ATTRIBUTES objA, ValueO;

	RtlInitUnicodeString(&netKeyU, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards");

	InitializeObjectAttributes(&objA, &netKeyU, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);
	st = ZwOpenKey(&hn, KEY_ALL_ACCESS, &objA);
	if (st != STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc klucza rejestru wiersz:49");
		return STATUS_UNSUCCESSFUL;
	}

	//*********************** Odczytywanie informacji o ilosci podkluczy rejestru**************************
	rozm = sizeof(KEY_FULL_INFORMATION);
	ret = 0;
	key_info = (PKEY_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);

	if (key_info == NULL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla klucza");
		ZwClose(hn);
		return STATUS_UNSUCCESSFUL;
	}

	st = ZwQueryKey(hn, KeyFullInformation, key_info, rozm, &ret);

	if (st == STATUS_BUFFER_OVERFLOW || st == STATUS_BUFFER_TOO_SMALL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Buffer overflow zainicjowano %d a  potrebne %d", rozm, ret);
		ExFreePool(key_info);
		rozm = ret;
		key_info = (PKEY_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
		if (key_info == NULL)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla klucza");
			ZwClose(hn);
			return STATUS_UNSUCCESSFUL;
		}

		RtlZeroMemory(key_info, rozm);
		st = ZwQueryKey(hn, KeyFullInformation, key_info, rozm, &ret);
		if (st != STATUS_SUCCESS)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Totalny Blad funkcji ZwQueryKey");
			ExFreePool(key_info);
			ZwClose(hn);
			return STATUS_UNSUCCESSFUL;
		}
	}
	//********************Jesli sukces to wylicza klucze i odczytuje potrzebne informacje do struktury*****************


	if (st == STATUS_SUCCESS)
	{
		adapterCount = key_info->SubKeys;
		ExFreePool(key_info);

		for (i = 0; i < adapterCount; i++)
		{
			cards[i].NetCardsCount = adapterCount;
			rozm = sizeof(KEY_BASIC_INFORMATION);
			ret = 0;
			KeyBasicInfo = (PKEY_BASIC_INFORMATION)ExAllocatePool(NonPagedPool, rozm);

			if (KeyBasicInfo == NULL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla KeyBasicInfo");
				ZwClose(hn);
				return STATUS_UNSUCCESSFUL;
			}
			memset(KeyBasicInfo, 0, rozm);
			st = ZwEnumerateKey(hn, i, KeyBasicInformation, KeyBasicInfo, rozm, &ret);

			if (st == STATUS_BUFFER_OVERFLOW || st == STATUS_BUFFER_TOO_SMALL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Za maly bufor KeyBasicInfo...Realokuje pamiec...");
				ExFreePool(KeyBasicInfo);
				rozm = ret;

				KeyBasicInfo = (PKEY_BASIC_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
				if (KeyBasicInfo == NULL)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla KeyBasicInfo");
					ZwClose(hn);
					return STATUS_UNSUCCESSFUL;
				}
				memset(KeyBasicInfo, 0, rozm);
				st = ZwEnumerateKey(hn, i, KeyBasicInformation, KeyBasicInfo, rozm, &ret);
			}
			if (st != STATUS_SUCCESS)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Totalny Blad funkcji ZwEnumerateKey");
				ExFreePool(KeyBasicInfo);
				ZwClose(hn);
				return STATUS_UNSUCCESSFUL;
			}

			//sprintf(cards[i].keyPath, "%s\\%ws", key, KeyBasicInfo->Name);
			wcscpy(cards[i].keyPath, key);
			wcscpy(&cards[i].keyPath[wcslen(cards[i].keyPath)], L"\\");
			memcpy(&cards[i].keyPath[wcslen(cards[i].keyPath)], KeyBasicInfo->Name, KeyBasicInfo->NameLength);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Dlugosc nazwy klucza : %d", KeyBasicInfo->NameLength);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nazwa podklucza : %ws", cards[i].keyPath);
			ExFreePool(KeyBasicInfo);

			RtlInitUnicodeString(&ValueU, cards[i].keyPath);

			InitializeObjectAttributes(&ValueO, &ValueU, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

			if (!NT_SUCCESS(ZwOpenKey(&hn1, KEY_ALL_ACCESS, &ValueO)))
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc podklucza %d", i);
				ZwClose(hn);
				return STATUS_UNSUCCESSFUL;
			}
			else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Otworzono podklucz %d", i);


			//******************Odczytuje wartosc Description(Nazwa karty scieciowej) podklucza rejestru******************



			RtlInitUnicodeString(&Value, L"Description");
			rozm = sizeof(KEY_VALUE_FULL_INFORMATION);
			ret = 0;

			ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
			if (ValueInfo == NULL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla danych wartosci klcza");
				ZwClose(hn);
				ZwClose(hn1);
				return STATUS_UNSUCCESSFUL;
			}

			RtlZeroMemory(ValueInfo, rozm);


			st = ZwQueryValueKey(hn1, &Value, KeyValueFullInformation, ValueInfo, rozm, &ret);

			if (st == STATUS_BUFFER_OVERFLOW || st == STATUS_BUFFER_TOO_SMALL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Buffer overflow zainicjowano %d a  potrebne %d", rozm, ret);
				ExFreePool(ValueInfo);
				rozm = ret;
				ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
				if (ValueInfo == NULL)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla danych wartosci klcza");
					ZwClose(hn);
					ZwClose(hn1);
					return STATUS_UNSUCCESSFUL;
				}

				RtlZeroMemory(ValueInfo, rozm);
				st = ZwQueryValueKey(hn1, &Value, KeyValueFullInformation, ValueInfo, rozm, &ret);
			}
			if (st != STATUS_SUCCESS)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Totalny Blad");
			}

			data = (LPWSTR)ExAllocatePool(NonPagedPool, ValueInfo->DataLength);
			RtlCopyMemory((PVOID)data, (PVOID)((char*)ValueInfo + ValueInfo->DataOffset), ValueInfo->DataLength);
			RtlInitUnicodeString(&Value, data);
			RtlUnicodeToUTF8N((PCHAR)cards[i].Description, 150, &ret, Value.Buffer, Value.Length);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Description : %s", cards[i].Description);

			ExFreePool(data);
			ExFreePool(ValueInfo);

			//*************Odczytuje wartoswc ServiceName(Nazwa urzadenia do ktorego mozna sie podlaczyc) podklucza rejestru**********************

			RtlInitUnicodeString(&Value, L"ServiceName");

			rozm = sizeof(KEY_VALUE_FULL_INFORMATION);
			ret = 0;
			ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
			RtlZeroMemory(ValueInfo, rozm);

			st = ZwQueryValueKey(hn1, &Value, KeyValueFullInformation, ValueInfo, rozm, &ret);

			if (st == STATUS_BUFFER_OVERFLOW || st == STATUS_BUFFER_TOO_SMALL)
			{
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Buffer overflow zainicjowano %d a  potrebne %d", rozm, ret);
				ExFreePool(ValueInfo);
				rozm = ret;
				ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(NonPagedPool, rozm);
				if (ValueInfo == NULL)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie mozna otworzyc zaalokowac pamieci dla danych wartosci klcza");
					ZwClose(hn);
					ZwClose(hn1);
					return STATUS_UNSUCCESSFUL;
				}
				RtlZeroMemory(ValueInfo, rozm);
				st = ZwQueryValueKey(hn1, &Value, KeyValueFullInformation, ValueInfo, rozm, &ret);
			}

			data = (LPWSTR)ExAllocatePool(NonPagedPool, ValueInfo->DataLength);
			RtlCopyMemory((PVOID)data, (PVOID)((char*)ValueInfo + ValueInfo->DataOffset), ValueInfo->DataLength);
			RtlInitUnicodeString(&Value, data);
			RtlUnicodeToUTF8N((PCHAR)cards[i].ServiceName, 150, &ret, Value.Buffer, Value.Length);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ServiceName : %s", cards[i].ServiceName);

			ExFreePool(data);
			ExFreePool(ValueInfo);
			ZwClose(hn1);
		}



		ZwClose(hn);

	}
	else
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Blad funkcij EnumerateKey error: %d", (int)st);
		return STATUS_UNSUCCESSFUL;
	}
	return 0;
}

int AllocateBufferList()
{

	PoolParam.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	PoolParam.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
	PoolParam.Header.Size = sizeof(PoolParam);
	PoolParam.ProtocolId = NDIS_PROTOCOL_ID_DEFAULT;
	PoolParam.fAllocateNetBuffer = TRUE;
	PoolParam.ContextSize = 0;  //MEMORY_ALLOCATION_ALIGNMENT;
	PoolParam.PoolTag = 'hcKl';
	PoolParam.DataSize = 0;

	PoolHandle = NULL;
	Buffer_List = NULL;
	PoolHandle = NdisAllocateNetBufferListPool(Protocol_Handle, &PoolParam);
	if (PoolHandle != NULL)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PoolHandle Alocated ....");

		Buffer = NULL;
		Buffer = ExAllocatePool(NonPagedPool, DataSize);
		if (Buffer != NULL)
		{
			mdl = NULL;
			mdl = IoAllocateMdl(Buffer, DataSize, FALSE, FALSE, NULL);
			if (mdl != NULL)
			{
				//	MmBuildMdlForNonPagedPool(mdl);
				mdl->MdlFlags = mdl->MdlFlags | MDL_NETWORK_HEADER;
				Buffer_List = NdisAllocateNetBufferAndNetBufferList(PoolHandle, 0, 0, mdl, 0, DataSize);
			}
		}

		//Buffer_List = NdisAllocateNetBufferList(PoolHandle, 0, 0);

		if (Buffer_List != NULL)
		{
			//if(NdisAllocateNetBufferListContext(Buffer_List, MEMORY_ALLOCATION_ALIGNMENT, MEMORY_ALLOCATION_ALIGNMENT, 123456)== NDIS_STATUS_SUCCESS)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "PoolHandle and Buffer_List Alocated ....");
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Mdl Flags = %d ....", mdl->MdlFlags);
		}

	}

	return 0;
}

int OpenAdapter(Dev_Lan Dev)
{
	int i;
	
	for (i = 0; i < adapterCount; i++)
	{
		if (Dev.BindingContext == cards[i].BindingContext)
		{
			NdisAcquireSpinLock(&cards[i].SpinLockA);

			cards[i].openIndex++;

			NdisReleaseSpinLock(&cards[i].SpinLockA);

			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Open : %s  Open Index = %d",cards[i].Description,cards[i].openIndex);
			
			return 0;
		}

	}
	
		
	return 1;
}

int RegisterProtocol(NDIS_HANDLE DriverContext, NDIS_STRING Service_Name)
{
	NDIS_STATUS status;
	NDIS_OBJECT_HEADER Header_Object;
	char name[150];
	RtlZeroMemory(&ProtocolDriverCharacteristics, sizeof(NDIS_PROTOCOL_DRIVER_CHARACTERISTICS));
	//ProtocolDriverCharacteristics.MinorDriverVersion = 9;
	//ProtocolDriverCharacteristics.MajorDriverVersion = 1;
	ProtocolDriverCharacteristics.MinorNdisVersion = 0; // NDIS_PROTOCOL_MINIMUM_MINOR_VERSION;
	ProtocolDriverCharacteristics.MajorNdisVersion = NDIS_PROTOCOL_MINIMUM_MAJOR_VERSION;
	ProtocolDriverCharacteristics.Name = Service_Name;
	ProtocolDriverCharacteristics.SetOptionsHandler = &SetOptions;
	ProtocolDriverCharacteristics.OpenAdapterCompleteHandlerEx = &ProtocolOpenAdapterCompleteEx;
	ProtocolDriverCharacteristics.CloseAdapterCompleteHandlerEx = &ProtocolCloseAdapterCompleteEx;
	ProtocolDriverCharacteristics.SendNetBufferListsCompleteHandler = &ProtocolSendNetBufferListsComplete;
	ProtocolDriverCharacteristics.OidRequestCompleteHandler = &ProtocolOidRequestComplete;
	ProtocolDriverCharacteristics.ReceiveNetBufferListsHandler = &ProtocolReceiveNetBufferLists;
	ProtocolDriverCharacteristics.StatusHandlerEx = &ProtocolStatusEx;
	ProtocolDriverCharacteristics.BindAdapterHandlerEx = &ProtocolBindAdapterEx;
	ProtocolDriverCharacteristics.UnbindAdapterHandlerEx = &ProtocolUnbindAdapterEx;
	ProtocolDriverCharacteristics.NetPnPEventHandler = &ProtocolNetPnpEvent;
	ProtocolDriverCharacteristics.UninstallHandler = ProtocolUninstall;
	ProtocolDriverCharacteristics.DirectOidRequestCompleteHandler = NULL; // &ProtocolDirectOidRequestComplete;
#if(NDIS_SUPPORT_NDIS640)
	ProtocolDriverCharacteristics.Header.Type = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS;
	ProtocolDriverCharacteristics.Header.Revision = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
	ProtocolDriverCharacteristics.Header.Size = NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_2;
#elif (NDIS_SUPPORT_NDIS6)

	ProtocolDriverCharacteristics.Header.Type = NDIS_OBJECT_TYPE_PROTOCOL_DRIVER_CHARACTERISTICS,
		ProtocolDriverCharacteristics.Header.Size = NDIS_SIZEOF_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1;
	ProtocolDriverCharacteristics.Header.Revision = NDIS_PROTOCOL_DRIVER_CHARACTERISTICS_REVISION_1;
#endif
	memset(name, 0, 150);
	wcstombs(name, Service_Name.Buffer, 150);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Rejestrowanie protokolu nazwa : %s", name);
	status = NdisRegisterProtocolDriver(DriverContext, &ProtocolDriverCharacteristics, &Protocol_Handle);
	if (status == NDIS_STATUS_SUCCESS) return 0;
	else
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Nie udalo sie zarejestrowaæ protoko³u error code : %x", status);
		if (status == NDIS_STATUS_BAD_VERSION) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Bad Version");
		return 1;
	}



}

NDIS_STATUS SetOptions(NDIS_HANDLE NdisDriverHandle, NDIS_HANDLE DriverContext)
{

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SetOptions....");

	return NDIS_STATUS_SUCCESS;
}
_Use_decl_annotations_

NDIS_STATUS ProtocolBindAdapterEx(NDIS_HANDLE ProtocolDriverContext, NDIS_HANDLE BindContext, PNDIS_BIND_PARAMETERS BindParameters)
{

	char devName[250];

	ANSI_STRING AStr;
	UNICODE_STRING UStr;
	BOOLEAN finded;
	int OP_AD;

	BindingExecute = TRUE;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolBindAdapterEx....");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Physical medium type : %d", BindParameters->PhysicalMediumType);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Name === %ws", BindParameters->AdapterName->Buffer);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Selection === %ws", BindParameters->ProtocolSection->Buffer);
	finded = FALSE;

	for (i = 0; i < adapterCount; i++)
	{
		memset(devName, 0, 250);
		sprintf(devName, "\\DEVICE\\%s", cards[i].ServiceName);
		RtlInitAnsiString(&AStr, devName);
		RtlAnsiStringToUnicodeString(&UStr, &AStr, TRUE);
		if (RtlCompareUnicodeString(&UStr, BindParameters->AdapterName, FALSE) == 0)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Finded", BindParameters->AdapterName->Buffer);
			OP_AD = i;
			RtlFreeUnicodeString(&UStr);
			finded = TRUE;
			break;
		}
		RtlFreeUnicodeString(&UStr);

	}

	if (finded == FALSE)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter not Finded.....");
		return NDIS_STATUS_RESOURCES;
	}

	RtlZeroMemory(&OpenParam, sizeof(OpenParam));
	OpenParam.AdapterName = BindParameters->AdapterName; // &Adapter_U_Name;
	OpenParam.Header.Type = NDIS_OBJECT_TYPE_OPEN_PARAMETERS;
	OpenParam.Header.Revision = NDIS_OPEN_PARAMETERS_REVISION_1;
	OpenParam.Header.Size = NDIS_SIZEOF_OPEN_PARAMETERS_REVISION_1;
	OpenParam.MediumArray = medium;
	OpenParam.MediumArraySize = 6;
	OpenParam.SelectedMediumIndex = &medium_index;
	OpenParam.FrameTypeArray = NULL;
	OpenParam.FrameTypeArraySize = 0;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Opened Name: %s\n", cards[OP_AD].Description);

	NdisAcquireSpinLock(&cards[OP_AD].SpinLockA);

	cards[OP_AD].Adapter_Handle = NULL;
	cards[OP_AD].PBContext = BindContext;
	cards[OP_AD].AState = NdisprotInitializing;
	memcpy(&cards[OP_AD].BindParam, BindParameters, sizeof(NDIS_BIND_PARAMETERS));

	NdisReleaseSpinLock(&cards[OP_AD].SpinLockA);

	//status_handler = NdisOpenAdapterEx(Protocol_Handle, cards[OP_AD].BindingContext, &OpenParam, BindContext, &cards[OP_AD].Adapter_Handle);
	status_handler = NdisOpenAdapterEx(Protocol_Handle, &cards[OP_AD], &OpenParam, BindContext, &cards[OP_AD].Adapter_Handle);

	if (status_handler != NDIS_STATUS_PENDING)
	{

		if (status_handler == NDIS_STATUS_SUCCESS)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Opened");
			if (cards[OP_AD].Adapter_Handle != NULL)
			{
				//status_handler = NDIS_STATUS_PENDING;
				ProtocolOpenAdapterCompleteEx(&cards[OP_AD], status_handler);
			}
			else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter_Handle == NULL");
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter_Handle=NULL error code : %x", status_handler);
		}


	}
	else
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Opening with pending", status_handler);
	
	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Diver Path : %ws", BindParameters->PhysicalDeviceObject->DriverObject->HardwareDatabase->Buffer);
	

	RtlFreeUnicodeString(&Adapter_U_Name);

	return status_handler;
}

NDIS_STATUS ProtocolUnbindAdapterEx(NDIS_HANDLE UnbindContext, NDIS_HANDLE ProtocolBindingContext)
{
	NDIS_OID_REQUEST request;
	ULONG type;
	int i,OP_AD;
	NDIS_STATUS status_;
	net_cards *Adapter;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolUnbindAdapterEx....");
	
	
	Adapter = (net_cards*)ProtocolBindingContext;

	NdisAcquireSpinLock(&Adapter->SpinLockA);

	Adapter->PBContext = UnbindContext;
	Adapter->UnbindExecute = TRUE;
	Adapter->AState = NdisprotClosing;

	NdisReleaseSpinLock(&Adapter->SpinLockA);

/*
	request.Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
	request.Header.Revision = NDIS_OID_REQUEST_REVISION_1;
	request.Header.Size = NDIS_SIZEOF_OID_REQUEST_REVISION_1;
	request.Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

	request.PortNumber = 0;

	
	memset(&PciProp, 0, sizeof(PciProp));
	request.RequestType = NdisRequestSetInformation;
	request.DATA.QUERY_INFORMATION.Oid = OID_802_3_MULTICAST_LIST;
	request.DATA.QUERY_INFORMATION.InformationBuffer = NULL;
	request.DATA.QUERY_INFORMATION.InformationBufferLength = 0;

	status_ = NdisOidRequest(cards[Open_Adapter].Adapter_Handle, &request);

	if (status_ != NDIS_STATUS_PENDING)
	{
		if (status_ == NDIS_STATUS_SUCCESS)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "request OID_802_3_MULTICAST_LIST success....");
			ProtocolOidRequestComplete(cards[Open_Adapter].Adapter_Handle, &request, NDIS_STATUS_SUCCESS);
		}
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request OID_802_3_MULTICAST_LIST failed....");
	}
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request OID_802_3_MULTICAST_LIST pending....");

	


	type = 0;

	request.RequestType = NdisRequestSetInformation;
	request.DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
	request.DATA.SET_INFORMATION.InformationBuffer = &type;
	request.DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);
	request.Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

	status_ = NdisOidRequest(cards[Open_Adapter].Adapter_Handle, &request);

	if (status_ != NDIS_STATUS_PENDING)
	{
		if (status_ == NDIS_STATUS_SUCCESS)
		{
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status success....");
			ProtocolOidRequestComplete(cards[Open_Adapter].Adapter_Handle, &request, NDIS_STATUS_SUCCESS);
		}
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request status failed....");
	}
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status pending....");
*/
	Adapter->UnbindStatus = NDIS_STATUS_PENDING;

	status_handler = NdisCloseAdapterEx(Adapter->Adapter_Handle);
	if (status_handler == NDIS_STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Closed....");
		Adapter->UnbindStatus = NDIS_STATUS_SUCCESS;
		ProtocolCloseAdapterCompleteEx(ProtocolBindingContext);

	}
	else if (status_handler == NDIS_STATUS_PENDING)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Closing with pending....");
	}

	return status_handler;
}


void ProtocolOpenAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext, NDIS_STATUS Status)
{
	
	PNDIS_OID_REQUEST PRequest;
	ULONG* type;
	DOT11_CURRENT_OPERATION_MODE * OperationMode;
	NDIS_STATUS status_;
	DOT11_OPERATION_MODE_CAPABILITY * ModeCapability;
	net_cards *Adapter;
	
	int i;

	Adapter = (net_cards*)ProtocolBindingContext;
	NdisAcquireSpinLock(&Adapter->SpinLockA);

	//cards[OP_AD].openIndex++;
	Adapter->licznik = 0;
	Adapter->indeks = 0;
	Adapter->isOpened = TRUE;
	Adapter->PowerState = NdisDeviceStateD0;

	NdisReleaseSpinLock(&Adapter->SpinLockA);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolOpenAdapterCompleteEx....");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Selected medium index is %d", medium_index);


	PRequest = (PNDIS_OID_REQUEST)ExAllocatePool(NonPagedPool, sizeof(NDIS_OID_REQUEST));
	type = (ULONG*)ExAllocatePool(NonPagedPool, sizeof(ULONG));

	*type = NDIS_PACKET_TYPE_PROMISCUOUS;

	PRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
	PRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
	PRequest->Header.Size = NDIS_SIZEOF_OID_REQUEST_REVISION_1;
	PRequest->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

	PRequest->PortNumber = 0;

	PRequest->RequestType = NdisRequestSetInformation;
	PRequest->DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
	PRequest->DATA.SET_INFORMATION.InformationBuffer = type;
	PRequest->DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);
	PRequest->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

	status_ = NdisOidRequest(Adapter->Adapter_Handle, PRequest);

	if (status_ != NDIS_STATUS_PENDING)
	{
		if (status_ == NDIS_STATUS_SUCCESS)
		{
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status success....");
			ProtocolOidRequestComplete(ProtocolBindingContext, PRequest, NDIS_STATUS_SUCCESS);
		}
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request status failed....");
	}
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status pending....");
	
	NdisWaitEvent(&RequestEvent,5000);
	NdisResetEvent(&RequestEvent);
	ExFreePool(type);
	ExFreePool(PRequest);


	PRequest = (PNDIS_OID_REQUEST)ExAllocatePool(NonPagedPool, sizeof(NDIS_OID_REQUEST));
	ModeCapability = (DOT11_OPERATION_MODE_CAPABILITY*)ExAllocatePool(NonPagedPool, sizeof(DOT11_OPERATION_MODE_CAPABILITY));


	PRequest->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
	PRequest->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
	PRequest->Header.Size = NDIS_SIZEOF_OID_REQUEST_REVISION_1;
	PRequest->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

	PRequest->PortNumber = 0;
	

	memset(ModeCapability, 0, sizeof(DOT11_OPERATION_MODE_CAPABILITY));

	PRequest->RequestType = NdisRequestQueryInformation;
	PRequest->DATA.QUERY_INFORMATION.Oid = OID_DOT11_OPERATION_MODE_CAPABILITY;
	PRequest->DATA.QUERY_INFORMATION.InformationBuffer = ModeCapability;
	PRequest->DATA.QUERY_INFORMATION.InformationBufferLength = sizeof(DOT11_OPERATION_MODE_CAPABILITY);
	PRequest->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;
	
	status_ = NdisOidRequest(Adapter->Adapter_Handle, PRequest);

	if (status_ != NDIS_STATUS_PENDING)
	{
		if (status_ == NDIS_STATUS_SUCCESS)
		{
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Monitor Mode status success....");
			ProtocolOidRequestComplete(ProtocolBindingContext, PRequest, NDIS_STATUS_SUCCESS);
			
		}
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Query Monitor Mode status failed....");
	}
	
	NdisWaitEvent(&RequestEvent, 5000);
	NdisResetEvent(&RequestEvent);
	ExFreePool(ModeCapability);
	ExFreePool(PRequest);


	if (Status == NDIS_STATUS_PENDING)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolOpenAdapterCompleteEx.... with status pending");
		NdisCompleteBindAdapterEx(Adapter->PBContext, NDIS_STATUS_SUCCESS);
	}
}


void ProtocolCloseAdapterCompleteEx(NDIS_HANDLE ProtocolBindingContext)
{
	int i;
	net_cards* Adapter;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolCloseAdapterCompleteEx....");
	
	Adapter = (net_cards*)ProtocolBindingContext;

	if (Adapter->UnbindStatus == NDIS_STATUS_PENDING)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Close with pending %s....", Adapter->Description);
		NdisCompleteUnbindAdapterEx(Adapter->PBContext);
	}
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Close complete %s....", Adapter->Description);
	
	
	NdisSetEvent(&CloseEvent);

}


NDIS_STATUS ProtocolNetPnpEvent(NDIS_HANDLE ProtocolBindingContext, PNET_PNP_EVENT_NOTIFICATION NetPnPEventNotification)
{
	NDIS_STATUS RetStatus;
	net_cards* Adapter;
	NDIS_DEVICE_POWER_STATE DevPowerState;

	Adapter = (net_cards*)ProtocolBindingContext;

	RetStatus = NDIS_STATUS_SUCCESS;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolNetPnpEvent....");


	switch (NetPnPEventNotification->NetPnPEvent.NetEvent)
	{
		case NetEventSetPower:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventSetPower....");
			if (NetPnPEventNotification->NetPnPEvent.Buffer != NULL && NetPnPEventNotification->NetPnPEvent.BufferLength == sizeof(NDIS_DEVICE_POWER_STATE))
			{
				DevPowerState = *((NDIS_DEVICE_POWER_STATE*)NetPnPEventNotification->NetPnPEvent.Buffer);
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "Protocol change powes state success......");
				AcqSpinLock(&Adapter->SpinLockA, FALSE);
				Adapter->PowerState = DevPowerState;
				RelSpinLock(&Adapter->SpinLockA, FALSE);
			}
			break;
		}
		case NetEventQueryPower:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventQueryPower....");
			break;
		}
		case NetEventQueryRemoveDevice:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventQueryRemoveDevice....");
			break;
		}
		case NetEventCancelRemoveDevice:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventCancelRemoveDevice....");
			break;
		}
		case NetEventReconfigure:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventReconfigure....");
			break;
		}
		case NetEventBindList:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventBindList....");
			break;
		}


		case NetEventBindsComplete:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventBindsComplete....");
			RetStatus = NDIS_STATUS_SUCCESS;
			break;
		}
		case NetEventPnPCapabilities:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventPnPCapabilities....");
			break;
		}

		case NetEventPause:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventPause....");

			AcqSpinLock(&Adapter->SpinLockA,FALSE);

			Adapter->AState = NdisprotPaused;

			RelSpinLock(&Adapter->SpinLockA,FALSE);
			break;
		}


		case NetEventRestart:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventRestart....");

			AcqSpinLock(&Adapter->SpinLockA, FALSE);

			Adapter->AState = NdisprotRunning;

			RelSpinLock(&Adapter->SpinLockA, FALSE);

			break;
		}


		case NetEventPortActivation:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventPortActivation....");
			break;
		}


		case NetEventPortDeactivation:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventPortDeactivation....");
			break;
		}

		case NetEventIMReEnableDevice:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventIMReEnableDevice....");
			break;
		}

		case NetEventNDKEnable:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventNDKEnable....");
			break;

		}

		case NetEventNDKDisable:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventNDKDisable....");
			break;

		}

		case NetEventFilterPreDetach:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventFilterPreDetach....");
			break;

		}

		case NetEventBindFailed:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventBindFailed....");
			break;

		}


		case NetEventSwitchActivate:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventSwitchActivate....");
			break;

		}


		case NetEventInhibitBindsAbove:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventInhibitBindsAbove....");
			break;

		}


		case NetEventAllowBindsAbove:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventAllowBindsAbove....");
			break;

		}




		case NetEventRequirePause:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventRequirePause....");
			break;

		}




		case NetEventAllowStart:
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetEventAllowStart....");
			break;

		}

	}


	return RetStatus;
}

void ProtocolUninstall(VOID)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolUninstall....");
}


void ProtocolOidRequestComplete(NDIS_HANDLE ProtocolBindingContext, PNDIS_OID_REQUEST OidRequest, NDIS_STATUS Status)
{
	ULONG Type;
	DOT11_CURRENT_OPERATION_MODE OperationMode;
	DOT11_OPERATION_MODE_CAPABILITY* ModeCapability;
	int i;
	net_cards* Adapter;

	Adapter = (net_cards*)ProtocolBindingContext;

	if (OidRequest->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_PACKET_FILTER && OidRequest->RequestType==NdisRequestSetInformation)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Packet Filter Normal complete");
		if (Status == NDIS_STATUS_SUCCESS)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS SUCCESS");
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS FAILED error code : %x", Status);
	}
	
	else if (OidRequest->DATA.QUERY_INFORMATION.Oid == OID_DOT11_OPERATION_MODE_CAPABILITY && OidRequest->RequestType == NdisRequestQueryInformation)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Network Query Mode complete");
		if (Status == NDIS_STATUS_SUCCESS)
		{
			ModeCapability = (DOT11_OPERATION_MODE_CAPABILITY*)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer;
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Network Mode : %x", ModeCapability->uOpModeCapability);
			Adapter->ModeCap = ModeCapability->uOpModeCapability;
			if (ModeCapability->uOpModeCapability & DOT11_OPERATION_MODE_NETWORK_MONITOR)
			{
				Adapter->NetMonSuported = 1;
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Network Monitor is suported ");
			}
			else
			{
				Adapter->NetMonSuported = 0;
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Network Monitor is not suported ");
			}
			
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS FAILED CODE ERROR: %x", Status);

		}
	}
	
	else if (OidRequest->DATA.SET_INFORMATION.Oid == OID_DOT11_CURRENT_OPERATION_MODE && OidRequest->RequestType == NdisRequestSetInformation)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Network Monitor Set Mode complete");
		if (Status == NDIS_STATUS_SUCCESS)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS SUCCESS");
		
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS FAILED CODE ERROR: %x", Status);

		}
	}

	if (OidRequest->DATA.QUERY_INFORMATION.Oid == OID_GEN_PCI_DEVICE_CUSTOM_PROPERTIES)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Get Pci Device Properties completed...");
		if (Status == NDIS_STATUS_SUCCESS)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS SUCCESS");
			
		}
		else
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS FAILED CODE ERROR: %x", Status);

		}
	}
	NdisSetEvent(&RequestEvent);
/*
	if (OidRequest->RequestType == NdisRequestSetInformation)
		ExFreePool(OidRequest->DATA.SET_INFORMATION.InformationBuffer);
	else if (OidRequest->RequestType == NdisRequestQueryInformation)
		ExFreePool(OidRequest->DATA.QUERY_INFORMATION.InformationBuffer);

	ExFreePool(OidRequest);
*/
}


void ProtocolStatusEx(NDIS_HANDLE ProtocolBindingContext, PNDIS_STATUS_INDICATION StatusIndication)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolStatusEx....");
}


void ProtocolReceiveNetBufferLists(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferLists, NDIS_PORT_NUMBER PortNumber,
	ULONG NumberOfNetBufferLists, ULONG ReceiveFlags)
{

	PNET_BUFFER N_Buffer;
	PVOID E_Head;
	PMDL mdl_ch;
	char* bf;
	UINT32 d;
	PNET_BUFFER_LIST BufferList;
	int i;
	int ByteCount;
	EHead_802_11* Packet;
	BOOLEAN isDispatch;
	net_cards* Adapter;
	ULONG RetFlags;

	//UNREFERENCED_PARAMETER(NumberOfNetBufferLists);

	Adapter = (net_cards*)ProtocolBindingContext;
	RetFlags = 0;


	BufferList = NetBufferLists;

	if (NDIS_TEST_RETURN_AT_DISPATCH_LEVEL(ReceiveFlags))
	{
		isDispatch = TRUE;
		NDIS_SET_RETURN_FLAG(RetFlags, NDIS_RETURN_FLAGS_DISPATCH_LEVEL);
	}

	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Recive Packet....................%s", Adapter->Description);

	if (Adapter->AState != NdisprotRunning && Adapter->PowerState != NdisDeviceStateD0)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter is Low Power or not Running State.....");
		if (NDIS_TEST_RECEIVE_FLAG(ReceiveFlags, NDIS_RECEIVE_FLAGS_RESOURCES) == 0)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolReceive FLAGS  NDIS_RECEIVE_FLAGS_RESOURCES.............................");

			NdisReturnNetBufferLists(Adapter->Adapter_Handle,NetBufferLists,RetFlags);
		}
		return;
	}

	
	for (i = 0; i < NumberOfNetBufferLists; i++)
	{
		//if (i > 0) BufferList = NET_BUFFER_LIST_NEXT_NBL(BufferList);

		N_Buffer = NET_BUFFER_LIST_FIRST_NB(BufferList);

		while (N_Buffer != NULL)
		{
			mdl_ch = NULL;
			//mdl_ch = NET_BUFFER_CURRENT_MDL(N_Buffer);
			mdl_ch = NET_BUFFER_FIRST_MDL(N_Buffer);
			while (mdl_ch != NULL)
			{
				E_Head = NULL;
				//E_Head = MmGetMdlVirtualAddress(mdl_ch);
				NdisQueryMdl(mdl_ch, &E_Head, &ByteCount, NormalPagePriority);
				if (E_Head != NULL && ByteCount>0 && ByteCount<sizeof(EHead_802_11))
				{
					AcqSpinLock(&Adapter->SpinLockA, isDispatch);


					Packet = (EHead_802_11*)Adapter->Buffer;
					//memset(&Packet[Adapter->indeks], 0, sizeof(EHead_802_11));
					//memcpy(&Packet[Adapter->indeks], E_Head, mdl_ch->ByteCount);
					RtlZeroMemory(&Packet[Adapter->indeks], sizeof(EHead_802_11));
					RtlCopyMemory(&Packet[Adapter->indeks], E_Head, ByteCount);
					//memcpy(&Packet[Adapter->indeks], E_Head,ByteCount);

					/*
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Packet licznik :  %d", cards[Open_Adapter].licznik);
					bf = (char*)&(cards[Open_Adapter].Packet[cards[Open_Adapter].licznik].Adres1);
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adres1 :%x-%x-%x-%x-%x-%x", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5]);
					bf = (char*)&(cards[Open_Adapter].Packet[cards[Open_Adapter].licznik].Adres2);
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adres2 :%x-%x-%x-%x-%x-%x", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5]);
					bf = (char*)&(cards[Open_Adapter].Packet[cards[Open_Adapter].licznik].Adres3);
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adres3 :%x-%x-%x-%x-%x-%x", bf[0], bf[1], bf[2], bf[3], bf[4], bf[5]);
					*/

					if(Adapter->indeks== PACKET_COUNT-1) Adapter->indeks = 0;
					else Adapter->indeks++;

					if (Adapter->licznik == PACKET_COUNT) Adapter->licznik = 1;
					else Adapter->licznik++;

					RelSpinLock(&Adapter->SpinLockA, isDispatch);

					//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Recive Packet....................%s, licznik : %d", Adapter->Description, Adapter->licznik);
				}
				//N_mdl = NULL;
				//NdisGetNextMdl(mdl_ch, N_mdl);
				//mdl_ch = *N_mdl;
				mdl_ch = mdl_ch->Next;
			}
			N_Buffer = NET_BUFFER_NEXT_NB(N_Buffer);
		}
		BufferList = NET_BUFFER_LIST_NEXT_NBL(BufferList);
	}
	
	if (NDIS_TEST_RECEIVE_FLAG(ReceiveFlags, NDIS_RECEIVE_FLAGS_RESOURCES)==0)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolReceive FLAGS  NDIS_RECEIVE_FLAGS_RESOURCES............................." );
		NdisReturnNetBufferLists(Adapter->Adapter_Handle, NetBufferLists, RetFlags);
	}
	
}

void ProtocolSendNetBufferListsComplete(NDIS_HANDLE ProtocolBindingContext, PNET_BUFFER_LIST NetBufferList, ULONG SendCompleteFlags)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolSendNetBufferListsComplete....");
}

void ProtocolDirectOidRequestComplete(NDIS_HANDLE ProtocolBindingContext, PNDIS_OID_REQUEST OidRequest, NDIS_STATUS Status)
{
	ULONG Type;
	DOT11_CURRENT_OPERATION_MODE OperationMode;
	DOT11_OPERATION_MODE_CAPABILITY* ModeCapability;

	if (OidRequest->DATA.SET_INFORMATION.Oid == OID_GEN_CURRENT_PACKET_FILTER)
	{
		Type = *((ULONG*)OidRequest->DATA.SET_INFORMATION.InformationBuffer);
		if (Type == NDIS_PACKET_TYPE_PROMISCUOUS)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Packet Type Promiscous complete");
		else if (Type == NDIS_PACKET_TYPE_802_11_RAW_DATA)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Protocol Packet Type RAW DATA complete");

		if (Status == NDIS_STATUS_SUCCESS)
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS SUCCESS");
		else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "STATUS FAILED");

	}
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "ProtocolDirectOidRequestComplete....");
}


int CloseAdapters()
{
	int i;
	NDIS_STATUS status, status_;
	PNDIS_OID_REQUEST request;
	ULONG *type;

	for (i = 0; i < adapterCount; i++)
	{
		if (cards[i].isOpened == TRUE)
		{
			DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "Unbind Adapter...%s", cards[i].Description);

			request = (PNDIS_OID_REQUEST)ExAllocatePool(NonPagedPool, sizeof(NDIS_OID_REQUEST));

			request->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
			request->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
			request->Header.Size = NDIS_SIZEOF_OID_REQUEST_REVISION_1;
			request->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

			request->PortNumber = 0;

			request->RequestType = NdisRequestSetInformation;
			request->DATA.QUERY_INFORMATION.Oid = OID_802_3_MULTICAST_LIST;
			request->DATA.QUERY_INFORMATION.InformationBuffer = NULL;
			request->DATA.QUERY_INFORMATION.InformationBufferLength = 0;

			status_ = NdisOidRequest(cards[i].Adapter_Handle, request);

			if (status_ != NDIS_STATUS_PENDING)
			{
				if (status_ == NDIS_STATUS_SUCCESS)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "request OID_802_3_MULTICAST_LIST success....");
					ProtocolOidRequestComplete(&cards[i], request, NDIS_STATUS_SUCCESS);
				}
				else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request OID_802_3_MULTICAST_LIST failed....");
			}
			else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request OID_802_3_MULTICAST_LIST pending....");

			NdisWaitEvent(&RequestEvent, 5000);
			NdisResetEvent(&RequestEvent);

			ExFreePool(request);


			request = (PNDIS_OID_REQUEST)ExAllocatePool(NonPagedPool, sizeof(NDIS_OID_REQUEST));
			type = (ULONG*)ExAllocatePool(NonPagedPool, sizeof(ULONG));

			request->Header.Type = NDIS_OBJECT_TYPE_OID_REQUEST;
			request->Header.Revision = NDIS_OID_REQUEST_REVISION_1;
			request->Header.Size = NDIS_SIZEOF_OID_REQUEST_REVISION_1;
			request->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

			request->PortNumber = 0;

			request->RequestType = NdisRequestSetInformation;
			request->DATA.SET_INFORMATION.Oid = OID_GEN_CURRENT_PACKET_FILTER;
			request->DATA.SET_INFORMATION.InformationBuffer = type;
			request->DATA.SET_INFORMATION.InformationBufferLength = sizeof(ULONG);
			request->Timeout = NDIS_OID_REQUEST_TIMEOUT_INFINITE;

			*type = 0;

			status_ = NdisOidRequest(cards[i].Adapter_Handle, request);

			if (status_ != NDIS_STATUS_PENDING)
			{
				if (status_ == NDIS_STATUS_SUCCESS)
				{
					//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status success....");
					ProtocolOidRequestComplete(&cards[i], request, NDIS_STATUS_SUCCESS);
				}
				else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request status failed....");
			}
			else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Request Promiscous status pending....");

			NdisWaitEvent(&RequestEvent, 5000);
			NdisResetEvent(&RequestEvent);

			ExFreePool(type);
			ExFreePool(request);

			cards[i].UnbindExecute = FALSE;
			status = NdisUnbindAdapter(cards[i].Adapter_Handle);
			NdisWaitEvent(&CloseEvent, 5000);
			NdisResetEvent(&CloseEvent);
			if (cards[i].UnbindExecute == FALSE)
			{
				DbgPrintEx(DPFLTR_IHVVIDEO_ID, DPFLTR_INFO_LEVEL, "UnbindExecute = FALSE. Closing now adapter ...%s", cards[i].Description);
				status = NdisCloseAdapterEx(cards[i].Adapter_Handle);
				if (status == NDIS_STATUS_SUCCESS)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Closed....");

					ProtocolCloseAdapterCompleteEx(&cards[i]);
					status = NDIS_STATUS_PENDING;
				}
				else if (status == NDIS_STATUS_PENDING)
				{
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Closing with pending....");
				}
			}
		}
	}
}