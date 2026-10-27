/* Local runtime checks: no game, renderer, network or private dump. */
#include "../src/runtime_cloud90.c"
#include <assert.h>
int main(void) {
    unsigned char *memory=VirtualAlloc((void *)UINT64_C(0x0800000000),0x4000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    assert(memory);
    *(void **)(memory+0x1800)=memory+0x2000;
    DWORD old;
    assert(VirtualProtect(memory,0x1000,PAGE_READONLY,&old));
    /* A field in another committed region must be checked at the field's
     * address, not as one span starting at the image's first code page. */
    assert(ptr(memory,0x1800)==memory+0x2000);
    assert(ptr(NULL,0x1800)==NULL);
    assert(ptr((void *)(UINTPTR_MAX-4),16)==NULL);
    assert(!readable(memory+0x5000,8));
    assert(identifier("c90-test_123",0));
    assert(!identifier("x\nack injected ok",0));
    assert(!identifier("",0));
    assert(identifier("",1));
    invert_until=123;no_gas_until=234;no_items_until=345;shield_until=456;
    clear_timed();assert(!invert_until&&!no_gas_until&&!no_items_until&&!shield_until);
    original_update=(void (ABI *)(void *))(uintptr_t)1;
    uint32_t buttons=0x6600;
    uint8_t steering=40,accelerator=255;
    invert_until=no_gas_until=no_items_until=GetTickCount64()+10000;
    in_play=1;
    runtime_cloud90_input(&buttons,&steering,&accelerator);
    assert(steering==215 && accelerator==0 && buttons==0);
    local_pause=1;buttons=0x6600;steering=40;accelerator=255;
    runtime_cloud90_input(&buttons,&steering,&accelerator);
    assert(steering==40 && accelerator==255 && buttons==0x6600);
    wchar_t temp[MAX_PATH],path[MAX_PATH];
    assert(GetTempPathW(MAX_PATH,temp));
    swprintf(path,MAX_PATH,L"%lscloud90-output-%lu-\u00f1.log",temp,GetCurrentProcessId());
    bridge_output=_wfopen(path,L"wb");assert(bridge_output);
    report("CLOUD90 state 1");ack("test-1",NULL);
    fclose(bridge_output);bridge_output=NULL;
    FILE *output=_wfopen(path,L"rb");assert(output);
    char received[128]={0};assert(fread(received,1,sizeof(received)-1,output)>0);fclose(output);
    assert(!strcmp(received,"CLOUD90 state 1\nCLOUD90 ack test-1 ok\n"));assert(DeleteFileW(path));
    swprintf(path,MAX_PATH,L"%lscloud90-%lu-\u00f1.json",temp,GetCurrentProcessId());
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    assert(file!=INVALID_HANDLE_VALUE);
    char envelope[256];
    uint64_t now=GetTickCount64();
    int length=snprintf(envelope,sizeof envelope,"{\"session\":\"test\",\"heartbeatMs\":%llu,\"paused\":false,\"effects\":[]}",(unsigned long long)now);
    DWORD written;
    assert(WriteFile(file,envelope,(DWORD)length,&written,NULL));CloseHandle(file);
    inbox_path=path;session="test";invert_until=now+10000;
    poll((void *)(uintptr_t)1,now);
    assert(invert_until==now+10000);
    assert(DeleteFileW(path));
    poll((void *)(uintptr_t)1,now+100);
    assert(invert_until==now+10000);
    poll((void *)(uintptr_t)1,now+2001);
    assert(!invert_until);
    local_pause=0;clear_timed();
    runtime_cloud90_input(&buttons,&steering,&accelerator);
    assert(steering==40 && accelerator==255 && buttons==0x6600);
    VirtualFree(memory,0,MEM_RELEASE);
    puts("Cloud90 runtime: memory-region boundaries, identifiers and panic cleanup passed");
    return 0;
}
