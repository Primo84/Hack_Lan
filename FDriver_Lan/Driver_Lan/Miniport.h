#pragma once

#include"FileDef.h"



NDIS_DEVICE_OBJECT_ATTRIBUTES ObjectAttributes;

PDRIVER_OBJECT Driver_Hack;

NDIS_HANDLE DeviceHandle;


NDIS_HANDLE MiniportInit(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath, NDIS_HANDLE DriverContext);


NDIS_MINIPORT_DRIVER_CHARACTERISTICS MiniportCharacteristics;

MINIPORT_SET_OPTIONS SetMiniportOptionsHandler;
MINIPORT_INITIALIZE MiniportInitializeHandler;
MINIPORT_HALT MiniportHalt;
MINIPORT_UNLOAD MiniportUnload;
MINIPORT_PAUSE MiniportPause;
MINIPORT_RESTART MiniportRestart;
MINIPORT_OID_REQUEST OidRequest;
MINIPORT_SEND_NET_BUFFER_LISTS MiniportSendNetBufferLists;
MINIPORT_RETURN_NET_BUFFER_LISTS MiniportReturnNetBufferLists;
MINIPORT_CANCEL_SEND MiniportCancelSend;
MINIPORT_CHECK_FOR_HANG MiniportCheckForHang;
MINIPORT_RESET MiniportReset;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY MiniportDevicePnpEventNotify;
MINIPORT_SHUTDOWN MiniportShutdown;
MINIPORT_CANCEL_OID_REQUEST MiniportCancelOidRequest;
MINIPORT_DIRECT_OID_REQUEST MiniportDirectOidRequest;;
MINIPORT_CANCEL_DIRECT_OID_REQUEST MiniportCancelDirectOidRequest;

NDIS_STATUS MiniportStatus;
NDIS_HANDLE MiniportHandle;
BOOLEAN MiniportRegistered;



MINIPORT_ADD_DEVICE MiniportAddDeviceHandler;
MINIPORT_REMOVE_DEVICE MiniportRemoveDeviceHandler;
MINIPORT_PNP_IRP MiniportPnpIrp;

NDIS_MINIPORT_PNP_CHARACTERISTICS PNPCharacteristics;
NDIS_PCI_DEVICE_CUSTOM_PROPERTIES PciProperties;

int CreateFilee(wchar_t *path);

int InitParameters(PNDIS_BIND_PARAMETERS BindP);