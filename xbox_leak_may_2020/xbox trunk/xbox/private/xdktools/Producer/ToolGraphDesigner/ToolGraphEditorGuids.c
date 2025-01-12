// GraphEditorGuids.c : Contains GUIDs not found in MIDL generated GraphDesigner_i.c
//

#ifdef __cplusplus
extern "C"{
#endif 

#ifndef GUID_DEFINED
#define GUID_DEFINED

typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;

#endif /* GUID_DEFINED */

const GUID CLSID_ToolGraphComponent = {0xeab971ee,0x6601,0x4f70,{0x94,0x34,0x32,0xce,0x56,0x8a,0xe3,0xf3}};

const GUID GUID_ToolGraphNode = {0x5a5a8e5,0xde1b,0x4bad,{0xb2,0x40,0xf5,0xa9,0xf2,0x11,0x7,0x86}};
const GUID GUID_ToolGraphRefNode = {0x5f28e0c4,0xde1c,0x492a,{0x93,0x2,0x75,0x38,0xba,0xc2,0xfb,0xc2}};

const GUID TOOLGRAPH_NameChange = {0xf1007af8,0xb98f,0x4a4a,{0x88,0xcf,0x7b,0x38,0x8f,0xdc,0x66,0xcf}};
const GUID TOOLGRAPH_ChangeNotification = {0x6681d08e,0xfde0,0x4006,{0xb6,0xbd,0x9c,0x1f,0x5f,0x16,0x1f,0xb3}};

#ifdef __cplusplus
}
#endif

