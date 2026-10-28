/* Native adapters tested against an isolated fake engine, never the game. */
#include <stddef.h>
#include "../src/runtime.h"
static void *test_native(size_t offset);
#define C90_CALL(offset,type) ((type)test_native(offset))
#include "../src/runtime_cloud90.c"
#include <assert.h>
static unsigned char *physics,*world,*bodies,*original,*copy,*damage,*respawn,*kart;
static void *current;
static int hits,falls,releases,protected,derived_physics;
static ABI void *get_component(void *entity,void *meta,int flags) {
    assert(entity==kart && flags==0);
    if(meta==physics && !derived_physics)return physics;
    if(meta==physics+0x90 && derived_physics)return physics;
    if(meta==damage)return damage;
    if(meta==respawn)return respawn;
    return NULL;
}
static ABI void *get_properties(void *body) {assert(body==physics+0x40);return current;}
static ABI void set_properties(void *body,void *props) {assert(body==physics+0x40);current=props;}
static ABI void clone_properties(void **out,void *from) {assert(from==original);memcpy(copy,from,0x40);*(unsigned *)(copy+8)=1;*out=copy;}
static ABI void release_object(void *p) {assert(p==physics||p==original||p==copy);assert(*(unsigned *)((char *)p+8)>0);--*(unsigned *)((char *)p+8);++releases;}
static ABI int can_damage(void *p,int bypass) {assert(p==damage&&bypass==0);return !protected;}
static ABI int hit(void *p,int type,void *attacker,int bypass) {assert(p==damage&&type==8&&!attacker&&!bypass);*(int *)(damage+0x40)=8;++hits;return 1;}
static ABI void fall(void *p,int type,void *point) {assert(p==respawn&&type==0&&!point);*(int *)(respawn+0x5c)=1;++falls;}
static void *test_native(size_t offset) {
    switch(offset) {
        case 0x3dbf90:return (void *)get_component;
        case 0xa8be70:return (void *)get_properties;
        case 0xa8a9c0:return (void *)set_properties;
        case 0xa8d490:return (void *)clone_properties;
        case 0x149d260:return (void *)release_object;
        case 0xe346f0:return (void *)can_damage;
        case 0xe330e0:return (void *)hit;
        case 0xe7dd00:return (void *)fall;
        default:assert(!"unexpected native call");return NULL;
    }
}
int main(void) {
    guest=VirtualAlloc((void *)UINT64_C(0x0800000000),0x2b00000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(guest);
    physics=guest+0x10000;world=guest+0x11000;bodies=guest+0x12000;original=guest+0x13000;copy=guest+0x14000;damage=guest+0x15000;respawn=guest+0x16000;kart=guest+0x17000;
    *(void **)physics=guest+0x2589f00;*(unsigned *)(physics+8)=1;*(unsigned *)(physics+0x40)=1;*(void **)(physics+0x48)=world;
    *(void **)(world+0x20)=bodies;*(unsigned *)(world+0x2c)=4;bodies[0x90+0x40]=1;
    *(unsigned *)(original+8)=1;*(float *)(original+0x18)=1;current=original;
    *(void **)(guest+0x2acfc10)=physics;*(void **)(guest+0x2ac5018)=damage;*(void **)(guest+0x2ac5948)=respawn;
    *(void **)(guest+0x2ab10e0)=physics+0x90;
    *(void **)damage=guest+0x2587410;*(int *)(damage+0x40)=-1;*(void **)(damage+0xb0)=original;
    *(void **)respawn=guest+0x2589460;
    assert(start_gravity(kart,6000,1000)==NULL);
    assert(current==copy && *(float *)(copy+0x18)==0.5f && *(float *)(original+0x18)==1);
    assert(start_gravity(kart,6000,1100)!=NULL);
    tick_troll(6999);assert(current==copy);tick_troll(7000);assert(current==original&&!gravity_physics);
    assert(*(unsigned *)(physics+8)==1 && *(unsigned *)(original+8)==1 && *(unsigned *)(copy+8)==0 && releases==3);
    assert(start_gravity(kart,6000,8000)==NULL);clear_timed();assert(current==original);
    assert(start_gravity(kart,6000,9000)==NULL);current=original;clear_timed();assert(current==original);
    /* Actual local kart: derived behavior shape, no base component result. */
    derived_physics=1;*(void **)physics=guest+0x258fc38;
    assert(start_gravity(kart,6000,10000)==NULL);
    assert(current==copy && *(float *)(copy+0x18)==0.5f);
    tick_troll(16000);assert(current==original&&!gravity_physics);
    assert(*(unsigned *)(physics+8)==1 && *(unsigned *)(original+8)==1);
    *(void **)physics=guest+0x258fc40;
    assert(start_gravity(kart,6000,17000)!=NULL && current==original);
    *(void **)physics=guest+0x258fc38;
    bodies[0x90+0x40]=0;
    assert(start_gravity(kart,6000,17000)!=NULL && current==original);
    bodies[0x90+0x40]=1;
    assert(apply_troll(kart,"chaos","",8000,10000,0)==NULL);
    assert(apply_troll(kart,"chaos","",8000,10001,0)!=NULL);
    unsigned last=chaos_phase;
    for(uint64_t t=12000;t<18000;t+=2000) {tick_troll(t);assert(chaos_phase<3&&chaos_phase!=last);assert(chaos_phase_until<=t+(chaos_phase==2?1000:2000));last=chaos_phase;}
    tick_troll(18000);assert(!chaos_until);
    assert(apply_troll(kart,"chaos","",10001,10000,0)!=NULL);
    assert(apply_troll(kart,"low_gravity","",0,10000,0)!=NULL);
    assert(apply_troll(kart,"fall_void","",1,10000,1)!=NULL && !falls);
    live_no_death=0;live_destructive=1;
    assert(apply_troll(kart,"fall_void","",1,10000,0)!=NULL && !falls);
    protected=1;assert(apply_troll(kart,"squash","",1,10000,0)!=NULL&&!hits);
    protected=0;assert(apply_troll(kart,"squash","",1,10000,0)==NULL&&hits==1);
    assert(apply_troll(kart,"squash","",1,10000,0)!=NULL&&hits==1);
    *(int *)(damage+0x40)=-1;
    assert(apply_troll(kart,"fall_void","",1,10000,1)==NULL&&falls==1);
    assert(apply_troll(kart,"fall_void","",1,10001,1)!=NULL&&falls==1);
    clear_timed();
    wchar_t tmp[MAX_PATH],path[MAX_PATH];assert(GetTempPathW(MAX_PATH,tmp));
    swprintf(path,MAX_PATH,L"%lsctr-troll-%lu.json",tmp,GetCurrentProcessId());
    inbox_path=path;session="troll-test";strcpy(race_epoch,"troll-test-1");
    unsigned char *racer=guest+0x18000,*kart_handle=guest+0x19000;
    *(void **)(racer+0x20)=kart_handle;*(void **)(kart_handle+0x28)=kart;
    for(int attempt=0;attempt<3;attempt++) {
        uint64_t now=GetTickCount64();
        FILE *f=_wfopen(path,L"wb");assert(f);
        fprintf(f,"{\"session\":\"troll-test\",\"heartbeatMs\":%llu,\"paused\":false,\"effects\":[{\"id\":\"%s\",\"effect\":\"chaos\",\"arg\":\"\",\"ms\":8000,\"safe\":true,\"epoch\":\"%s\",\"expiresMs\":%llu}]}",(unsigned long long)now,attempt?"valid":"stale",attempt?race_epoch:"old-race",(unsigned long long)(now+5000));
        fclose(f);poll(racer,now);
        if(attempt==1)assert(chaos_until>now);else assert(!chaos_until);
        clear_timed();
    }
    assert(DeleteFileW(path));
    VirtualFree(guest,0,MEM_RELEASE);
    puts("CTR trolls: independent gravity clone/restore/refcounts, native-state guards, fall permissions, bounded chaos passed");
    return 0;
}
