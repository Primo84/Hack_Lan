

#pragma once


#include "FileDef.h"
//#include "miniport.h"
//#include "wdfminiport.h"




int allocated;

ULONG * type;

UNICODE_STRING Adapter_U_Name;

ANSI_STRING ValueA, Adapter_A_Name;
OBJECT_ATTRIBUTES obj;
NTSTATUS st;
int i,adapterCount;


//NET_FRAME_TYPE frameT[]={0x800, 0x806, 0x86DD, 0x8137};
NDIS_OPEN_PARAMETERS OpenParam;

NDIS_HANDLE Protocol_Handle;

NDIS_STATUS status_handler, error_status_handler, status_unbind;

//NDIS_MEDIUM medium[] = {NdisMediumWirelessWan,NdisMediumNative802_11};

NDIS_HANDLE PoolHandle;
NET_BUFFER_LIST_POOL_PARAMETERS PoolParam;
NET_BUFFER_LIST * Buffer_List;
UINT32 DataSize;

NDIS_PROTOCOL_DRIVER_CHARACTERISTICS ProtocolDriverCharacteristics;
int Init_Adapter;
UINT medium_index;

PVOID Buffer;
PMDL mdl;

ULONG ret, rozm;
PKEY_FULL_INFORMATION key_info;
PKEY_BASIC_INFORMATION KeyBasicInfo;
PKEY_VALUE_FULL_INFORMATION ValueInfo;
LPWSTR data;
ULONG rozmiar_buf;
UNICODE_STRING netKeyU, ValueU, Value;
net_cards cards[25];

HANDLE hn, hn1;
NDIS_EVENT CloseEvent, RequestEvent;
char AName[250];
BOOLEAN BindingExecute;

PROTOCOL_BIND_ADAPTER_EX ProtocolBindAdapterEx;
PROTOCOL_UNBIND_ADAPTER_EX ProtocolUnbindAdapterEx;
PROTOCOL_SET_OPTIONS SetOptions;
PROTOCOL_OPEN_ADAPTER_COMPLETE_EX ProtocolOpenAdapterCompleteEx;
PROTOCOL_CLOSE_ADAPTER_COMPLETE_EX ProtocolCloseAdapterCompleteEx;
PROTOCOL_SEND_NET_BUFFER_LISTS_COMPLETE ProtocolSendNetBufferListsComplete;
PROTOCOL_OID_REQUEST_COMPLETE ProtocolOidRequestComplete;
PROTOCOL_RECEIVE_NET_BUFFER_LISTS ProtocolReceiveNetBufferLists;
PROTOCOL_STATUS_EX ProtocolStatusEx;
PROTOCOL_NET_PNP_EVENT ProtocolNetPnpEvent;
PROTOCOL_UNINSTALL ProtocolUninstall;
PROTOCOL_DIRECT_OID_REQUEST_COMPLETE ProtocolDirectOidRequestComplete;




int AllocateBufferList();
int RegisterProtocol(NDIS_HANDLE DriverContext, NDIS_STRING Service_Name);
int OpenAdapter(Dev_Lan Dev);

int GetNetworkCards();

int CloseAdapters();

void Sleep(ULONG milisec);

//-------------------------------------------------------------------------


