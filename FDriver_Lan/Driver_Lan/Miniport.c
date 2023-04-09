
#include "Miniport.h"




NDIS_HANDLE MiniportInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, NDIS_HANDLE DeviceContext)
{
	MiniportRegistered = FALSE;
	MiniportHandle = NULL;
	NdisZeroMemory(&MiniportCharacteristics, sizeof(NDIS_MINIPORT_DRIVER_CHARACTERISTICS));
	MiniportCharacteristics.Flags = NDIS_INTERMEDIATE_DRIVER	;
	MiniportCharacteristics.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_DRIVER_CHARACTERISTICS;
	MiniportCharacteristics.Header.Revision = NDIS_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
	MiniportCharacteristics.Header.Size = NDIS_SIZEOF_MINIPORT_DRIVER_CHARACTERISTICS_REVISION_2;
	MiniportCharacteristics.MinorNdisVersion= 0;
	MiniportCharacteristics.MajorNdisVersion= NDIS_MINIPORT_MAJOR_VERSION;
	MiniportCharacteristics.MajorDriverVersion = 1;
	MiniportCharacteristics.MinorDriverVersion = 0;

	MiniportCharacteristics.SetOptionsHandler = &SetMiniportOptionsHandler;
	MiniportCharacteristics.InitializeHandlerEx = &MiniportInitializeHandler;
	MiniportCharacteristics.HaltHandlerEx = &MiniportHalt;
	MiniportCharacteristics.UnloadHandler = &MiniportUnload;
	MiniportCharacteristics.PauseHandler = &MiniportPause;
	MiniportCharacteristics.RestartHandler = &MiniportRestart; 
	MiniportCharacteristics.OidRequestHandler = &OidRequest;
	MiniportCharacteristics.SendNetBufferListsHandler = &MiniportSendNetBufferLists;
	MiniportCharacteristics.ReturnNetBufferListsHandler = &MiniportReturnNetBufferLists;
	MiniportCharacteristics.CancelSendHandler = &MiniportCancelSend;
	MiniportCharacteristics.CheckForHangHandlerEx = &MiniportCheckForHang;
	MiniportCharacteristics.ResetHandlerEx = &MiniportReset;
	MiniportCharacteristics.DevicePnPEventNotifyHandler = &MiniportDevicePnpEventNotify;
	MiniportCharacteristics.ShutdownHandlerEx = &MiniportShutdown;
	MiniportCharacteristics.CancelOidRequestHandler = &MiniportCancelOidRequest;
	MiniportCharacteristics.DirectOidRequestHandler = &MiniportDirectOidRequest;
	MiniportCharacteristics.CancelDirectOidRequestHandler = &MiniportCancelDirectOidRequest;
	
	MiniportStatus = NdisMRegisterMiniportDriver(DriverObject, RegistryPath, DeviceContext, &MiniportCharacteristics, &MiniportHandle);
	if (MiniportStatus == NDIS_STATUS_SUCCESS)
	{
		MiniportRegistered = TRUE;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Miniport Register Protocol success......");
		return MiniportHandle;
	}
	
	return NULL;
}

_Use_decl_annotations_ NDIS_STATUS SetMiniportOptionsHandler(IN NDIS_HANDLE NdisDriverHandle, IN NDIS_HANDLE DriverContext)
{
	wchar_t p[] = L"C:\\minporsetoptionalH.txt";
	CreateFilee(p);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SetOptionsHandler execute........");
	
	memset(&PNPCharacteristics, 0, sizeof(PNPCharacteristics));
	PNPCharacteristics.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_PNP_CHARACTERISTICS;
	PNPCharacteristics.Header.Revision = NDIS_MINIPORT_PNP_CHARACTERISTICS_REVISION_1;
	PNPCharacteristics.Header.Size = sizeof(PNPCharacteristics); //NDIS_SIZEOF_MINIPORT_PNP_CHARACTERISTICS_REVISION_1;
	PNPCharacteristics.MiniportAddDeviceHandler = &MiniportAddDeviceHandler;
	PNPCharacteristics.MiniportRemoveDeviceHandler = &MiniportRemoveDeviceHandler;
	PNPCharacteristics.MiniportFilterResourceRequirementsHandler = &MiniportPnpIrp;
	PNPCharacteristics.MiniportStartDeviceHandler = &MiniportPnpIrp;
	
	if(NdisSetOptionalHandlers(NdisDriverHandle, (PNDIS_DRIVER_OPTIONAL_HANDLERS)&PNPCharacteristics)==NDIS_STATUS_SUCCESS)
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SetOptionsHandler success........");
	else DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "SetOptionsHandler failoed........");
	
	return NDIS_STATUS_SUCCESS;
}

_Use_decl_annotations_ NDIS_STATUS MiniportInitializeHandler( IN NDIS_HANDLE NdisMiniportHandle, IN NDIS_HANDLE MiniportDriverContext, IN PNDIS_MINIPORT_INIT_PARAMETERS MiniportInitParameters)
{
	NDIS_MINIPORT_ADAPTER_ATTRIBUTES Attrib;
	NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES RegisterAttrib;
	PNDIS_BIND_PARAMETERS BindP;
	int i;
	wchar_t p[] = L"C:\\minportInit.txt";
	NDIS_STATUS Stat;

	CreateFilee(p);
	/*
	BindP = (PNDIS_BIND_PARAMETERS)MiniportDriverContext;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportInitialize execute........");
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Adapter Name : %ws",BindP->AdapterName->Buffer);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "IfType : %d", BindP->IfType);

	memset(&RegisterAttrib, 0, sizeof(NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES));

	RegisterAttrib.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES;
	RegisterAttrib.Header.Revision = NDIS_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
	RegisterAttrib.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_REGISTRATION_ATTRIBUTES_REVISION_1;
	RegisterAttrib.MiniportAdapterContext = NULL;
	RegisterAttrib.CheckForHangTimeInSeconds = 4;
	RegisterAttrib.AttributeFlags = NDIS_MINIPORT_ATTRIBUTES_BUS_MASTER | NDIS_MINIPORT_ATTRIBUTES_NO_HALT_ON_SUSPEND;
	RegisterAttrib.InterfaceType = NdisInterfacePci; // BindP->IfType;
	
	//Attrib.GeneralAttributes.AccessType = BindP->AccessType;

	
	
	
	Stat=NdisMSetMiniportAttributes(MiniportHandle, &Attrib);
	if(Stat==NDIS_STATUS_SUCCESS) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Register success........");
	else 
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Register failed error : %x",Stat);
	
	*/
	return NDIS_STATUS_SUCCESS;
}

_Use_decl_annotations_ void MiniportHalt( IN NDIS_HANDLE MiniportAdapterContext, IN NDIS_HALT_ACTION HaltAction)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportHalt execute........");
}

_Use_decl_annotations_ void MiniportUnload( IN PDRIVER_OBJECT DriverObject)
{
	wchar_t p[] = L"C:\\MiniportUnload.txt";

	CreateFilee(p);

	if (MiniportRegistered == TRUE)
		NdisMDeregisterMiniportDriver(MiniportHandle);

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportUnload execute........");
	MiniportRegistered = FALSE;
	OnUnload(DriverObject);
}

_Use_decl_annotations_ NDIS_STATUS MiniportPause( IN NDIS_HANDLE MiniportAdapterContext, IN PNDIS_MINIPORT_PAUSE_PARAMETERS PauseParameters)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportPause execute........");
	return NDIS_STATUS_SUCCESS;
}



_Use_decl_annotations_ NDIS_STATUS MiniportRestart( IN NDIS_HANDLE MiniportAdapterContext, PNDIS_MINIPORT_RESTART_PARAMETERS RestartParameters)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportRestart execute........");
	return NDIS_STATUS_SUCCESS;
}

_Use_decl_annotations_ NDIS_STATUS OidRequest( IN NDIS_HANDLE MiniportAdapterContext, IN PNDIS_OID_REQUEST OidRequest)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportRestart execute........");
	return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_ void MiniportSendNetBufferLists(IN NDIS_HANDLE MiniportAdapterContext, IN PNET_BUFFER_LIST NetBufferList, IN NDIS_PORT_NUMBER PortNumber, IN ULONG SendFlags)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportSendNetBufferLists execute........");
}



_Use_decl_annotations_ void MiniportReturnNetBufferLists( IN NDIS_HANDLE MiniportAdapterContext, IN PNET_BUFFER_LIST NetBufferLists, IN ULONG ReturnFlags)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportReturnNetBufferLists execute........");
}



_Use_decl_annotations_ void MiniportCancelSend( IN NDIS_HANDLE MiniportAdapterContext, IN PVOID CancelId
)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportCancelSend execute........");
}



_Use_decl_annotations_ BOOLEAN MiniportCheckForHang( IN NDIS_HANDLE MiniportAdapterContext)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportCheckForHang execute........");
	return TRUE;
}


_Use_decl_annotations_ NDIS_STATUS MiniportReset( IN NDIS_HANDLE MiniportAdapterContext, OUT PBOOLEAN AddressingReset)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportReset execute........");
	return NDIS_STATUS_SUCCESS;
}



_Use_decl_annotations_ void MiniportDevicePnpEventNotify( IN NDIS_HANDLE MiniportAdapterContext, IN PNET_DEVICE_PNP_EVENT NetDevicePnPEvent)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportDevicePnpEventNotify execute........");
}



_Use_decl_annotations_ void MiniportShutdown(IN NDIS_HANDLE MiniportAdapterContext, IN NDIS_SHUTDOWN_ACTION ShutdownAction)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportShutdown execute........");
}



_Use_decl_annotations_ void MiniportCancelOidRequest( IN NDIS_HANDLE MiniportAdapterContext,IN PVOID RequestId)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportCancelOidRequest execute........");
}



_Use_decl_annotations_ NDIS_STATUS MiniportDirectOidRequest(IN NDIS_HANDLE MiniportAdapterContext,IN PNDIS_OID_REQUEST OidRequest)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportDirectOidRequest execute........");
	return NDIS_STATUS_SUCCESS;
}



_Use_decl_annotations_ void MiniportCancelDirectOidRequest(IN NDIS_HANDLE MiniportAdapterContext,IN PVOID RequestId
)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportCancelDirectOidRequest execute........");
}

_Use_decl_annotations_ MiniportSynchronousOidRequest(IN NDIS_HANDLE MiniportAdapterContext,IN NDIS_OID_REQUEST* OidRequest)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportSynchronousOidRequest execute........");
	return NDIS_STATUS_SUCCESS;
}



//********************************************   PNP characteristisc functions   **************************************



NDIS_STATUS MiniportAddDeviceHandler( IN NDIS_HANDLE NdisMiniportHandle, IN NDIS_HANDLE MiniportDriverContext)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportAddDeviceHandler execute........");
	return NDIS_STATUS_SUCCESS;
}

void MiniportRemoveDeviceHandler(IN NDIS_HANDLE MiniportAddDeviceContext)
{
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportRemoveDeviceHandler execute........");
}




NDIS_STATUS MiniportPnpIrp(IN NDIS_HANDLE MiniportAddDeviceContext, IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "MiniportAddDeviceHandler execute........");
	Stack=IoGetCurrentIrpStackLocation(Irp);
	return NDIS_STATUS_SUCCESS;
}


int CreateFilee(wchar_t *path)
{
	OBJECT_ATTRIBUTES obj;
	UNICODE_STRING str;
	HANDLE file;
	wchar_t Fpath[150];
	IO_STATUS_BLOCK status;

	if (path == NULL) 
		return 1;
	memset(Fpath, 0, 150 * sizeof(wchar_t));
	wcscpy(Fpath, L"\\DosDevices\\");
	wcscpy(&Fpath[wcslen(Fpath)], path);
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Create File %ws........",Fpath);
	RtlInitUnicodeString(&str, Fpath);
	InitializeObjectAttributes(&obj, &str, OBJ_KERNEL_HANDLE, NULL, NULL);
	if (ZwCreateFile(&file, GENERIC_READ | GENERIC_WRITE, &obj, &status, NULL, FILE_ATTRIBUTE_NORMAL, 0, FILE_OPEN_IF, 0, NULL, 0) == STATUS_SUCCESS)
		ZwClose(file);
	return 0;
}







int InitParameters(PNDIS_BIND_PARAMETERS BindP)
{

	CM_RESOURCE_LIST *ResL;
	ULONG ret;
	NTSTATUS status;
	NDIS_MINIPORT_INIT_PARAMETERS InitP;
	NDIS_PORT_AUTHENTICATION_PARAMETERS PortA;

	memset(&InitP, 0, sizeof(NDIS_MINIPORT_INIT_PARAMETERS));
	InitP.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_INIT_PARAMETERS;
	InitP.Header.Revision = NDIS_MINIPORT_INIT_PARAMETERS_REVISION_1;
	InitP.Header.Size = NDIS_SIZEOF_MINIPORT_INIT_PARAMETER_REVISION_1;
	ResL = ExAllocatePool(NonPagedPool, sizeof(CM_RESOURCE_LIST));
	status=IoGetDeviceProperty(BindP->PhysicalDeviceObject, DevicePropertyBootConfiguration, sizeof(ResL), (PVOID)ResL, &ret);
	if (status != STATUS_SUCCESS)
	{
		if (status == STATUS_BUFFER_TOO_SMALL)
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Buffer to small... Realocate buffer.......");
			ExFreePool(ResL);
			ResL = ExAllocatePool(NonPagedPool, ret);
			status = IoGetDeviceProperty(BindP->PhysicalDeviceObject, DevicePropertyBootConfiguration, ret, (PVOID)ResL, &ret);
		}
	}
	if (status == STATUS_SUCCESS)
	{
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Resource list is completed......");
		InitP.AllocatedResources = &ResL->List->PartialResourceList;
	}
	InitP.IMDeviceInstanceContext = NULL;
	InitP.MiniportAddDeviceContext = NULL;
	InitP.IfIndex = BindP->BoundIfIndex;
	InitP.NetLuid = BindP->BoundIfNetluid;
	memset(&PortA, 0, sizeof(PortA));

	PortA.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
	PortA.Header.Revision = NDIS_PORT_AUTHENTICATION_PARAMETERS_REVISION_1;
	PortA.Header.Size = NDIS_SIZEOF_PORT_AUTHENTICATION_PARAMETERS_REVISION_1;
	PortA.RcvAuthorizationState = BindP->ActivePorts->PortCharacteristics.RcvAuthorizationState;
	PortA.RcvControlState = BindP->ActivePorts->PortCharacteristics.RcvControlState;
	PortA.SendAuthorizationState = BindP->ActivePorts->PortCharacteristics.SendAuthorizationState;
	PortA.SendControlState = BindP->ActivePorts->PortCharacteristics.SendControlState;
	InitP.DefaultPortAuthStates = &PortA;
	InitP.PciDeviceCustomProperties = &PciProperties;
	MiniportInitializeHandler(MiniportHandle, (NDIS_HANDLE)BindP, &InitP);
	ExFreePool(ResL);
	return 0;
	
}