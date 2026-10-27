/* Cloud90's offline bridge. Native calls use the guest's System V ABI.
 * Version-specific addresses are gated before the first hook is installed. */
#include "runtime.h"
#include "runtime_cloud90.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include "cloud90_json.h"
#ifdef _WIN32
#include <windows.h>
static unsigned char *guest;
static void (ABI *original_update)(void *);
static const wchar_t *inbox_path;
static const char *session;
static uint64_t next_poll,next_state,last_heartbeat;
static _Atomic uint64_t invert_until,no_gas_until,no_items_until,shield_until;
static _Atomic int in_play,local_pause;
static int last_paused=1,key_down;
static char seen[256][81];
static unsigned seen_count;
static void *last_racer;
static FILE *bridge_output;
static void report(const char *line) {
    puts(line);fflush(stdout);
    if(bridge_output) { fputs(line,bridge_output);fputc('\n',bridge_output);fflush(bridge_output); }
}
static int readable(const void *p,size_t n) {
    MEMORY_BASIC_INFORMATION m;
    if(!p || (uintptr_t)p<UINT64_C(0x0800000000) || (uintptr_t)p+n>UINT64_C(0x10000000000)) return 0;
    if(!VirtualQuery(p,&m,sizeof m) || m.State!=MEM_COMMIT || (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))) return 0;
    return (uintptr_t)p+n<=(uintptr_t)m.BaseAddress+m.RegionSize;
}
static void *ptr(const void *p,size_t offset) {
    if(!p || (uintptr_t)p>UINTPTR_MAX-offset) return NULL;
    const void *field=(const char *)p+offset;
    return readable(field,8) ? *(void *const *)field : NULL;
}
static int number(const void *p,size_t offset) { return p && readable((const char *)p+offset,4) ? *(const int *)((const char *)p+offset) : -1; }
static int read_byte(const void *p,size_t offset) { return p && readable((const char *)p+offset,1) ? *((const unsigned char *)p+offset) : -1; }
static int is_object(void *p,size_t vtable,size_t size) { return readable(p,size) && ptr(p,0)==guest+vtable; }
/* Observe only already resolved engine handles. Never mutate a lazy handle from
 * outside its owner and never retain a pointer beyond this frame. */
static void *handle(void *h) {
    for(int depth=0;depth<8;++depth) {
        if(!readable(h,0x30)) return NULL;
        unsigned flags=*(unsigned *)h;
        if(flags&0x10000000) return NULL;
        void *p=ptr(h,0x28);
        if(!(flags&0x20000000)) return p;
        h=ptr(p,0x10);
    }
    return NULL;
}
#ifndef C90_CALL
#define C90_CALL(offset,type) ((type)(guest+(offset)))
#endif
static void *component(void *entity,size_t meta,size_t vtable,size_t size) {
    if(!readable(entity,0x100)) return NULL;
    void *m=ptr(guest,meta);
    if(!readable(m,0x60)) return NULL;
    typedef void *(ABI *Get)(void *,void *,int);
    void *p=C90_CALL(0x3dbf90,Get)(entity,m,0);
    return is_object(p,vtable,size) ? p : NULL;
}
static void restore_gravity(void);
static void clear_troll(void);
static void clear_timed(void) { invert_until=no_gas_until=no_items_until=shield_until=0; clear_troll(); }
static void ack(const char *id,const char *reason) {
    char line[256];snprintf(line,sizeof line,"CLOUD90 ack %s %s",id,reason ? reason : "ok");report(line);
}
static int identifier(const char *s,int empty) {
    if(!*s) return empty;
    for(;*s;++s) if(!((*s>='a'&&*s<='z')||(*s>='A'&&*s<='Z')||(*s>='0'&&*s<='9')||*s=='_'||*s=='-')) return 0;
    return 1;
}
static void *player_one(void *clock) {
    if(read_byte(clock,0x28)!=0) return NULL;
    typedef void *(ABI *Client)(void);
    void *client=((Client)(guest+0x94efa0))();
    /* CClient::_gameMode: 8 is Local Arcade. Exclude adventure,
     * time trials, battle and the network modes even with BB_ONLINE=0. */
    if(number(client,0x144)!=8) return NULL;
    /* COctaneGameManager::_raceMode: native race mode 1 is a normal race. */
    if(number(ptr(guest,0x2acde48),0xd4c)!=1)return NULL;
    void *manager=ptr(guest,0x2ad4b70),*list=ptr(manager,0x10),*entries=ptr(list,0x20);
    int count=number(list,0xc),humans=0;
    if(count<2 || count>8 || !readable(entries,(size_t)count*8)) return NULL;
    void *one=NULL;
    for(int i=0;i<count;++i) {
        void *r=ptr(entries,(size_t)i*8);
        if(!is_object(r,0x26d5f78,0x178)) return NULL;
        if(read_byte(r,0x40)!=0) continue;
        ++humans;
        void *p=handle(ptr(r,0x10));
        if(readable(p,0xf8) && number(p,0xb4)==0 && read_byte(r,0x8c)==0 && read_byte(r,0xc0)==0 && number(r,0x4c)>=0) one=r;
    }
    if(humans!=1 || !one)return NULL;
    void *kart=handle(ptr(one,0x20));
    void *respawn=component(kart,0x2ac5948,0x2589460,0x90);
    return respawn && number(respawn,0x5c)==0 ? one : NULL;
}
#include "cloud90_troll.h"
static void clear_troll(void) { chaos_until=0;restore_gravity(); }
static const char *apply(void *racer,const char *effect,const char *arg,unsigned ms,uint64_t now,int authorized) {
    int troll=!strcmp(effect,"low_gravity")||!strcmp(effect,"squash")||!strcmp(effect,"fall_void")||!strcmp(effect,"chaos")||!strcmp(effect,"wumpa_remove")||!strcmp(effect,"remove_item")||!strcmp(effect,"invert_steering")||!strcmp(effect,"no_accelerate")||!strcmp(effect,"no_items");
    if(troll && now<shield_until) return "escudo activo";
    if(!strcmp(effect,"shield")) {invert_until=no_gas_until=no_items_until=0;clear_troll();shield_until=now+(ms<30000?ms:30000);return NULL;}
    if(!strcmp(effect,"invert_steering")) {invert_until=now+(ms<10000?ms:10000);return NULL;}
    if(!strcmp(effect,"no_accelerate")) {no_gas_until=now+(ms<3000?ms:3000);return NULL;}
    if(!strcmp(effect,"no_items")) {no_items_until=now+(ms<10000?ms:10000);return NULL;}
    if(!strcmp(effect,"wumpa_add") || !strcmp(effect,"wumpa_full") || !strcmp(effect,"wumpa_remove")) {
        int current=number(racer,0x9c),target=current,amount=3;
        if(current<0 || current>10) return "contador Wumpa no disponible";
        if(*arg) {if(!strcmp(arg,"1")) amount=1;else if(!strcmp(arg,"3")) amount=3;else if(!strcmp(arg,"5")) amount=5;else if(!strcmp(arg,"all")&&!strcmp(effect,"wumpa_remove")) amount=10;else return "opcion desconocida";}
        if(!strcmp(effect,"wumpa_full")) target=10;
        else if(!strcmp(effect,"wumpa_add")) target=current+amount;
        else target=current-amount;
        if(target<0)target=0;if(target>10)target=10;
        if(target==current)return "Wumpa sin cambios";
        /* CRacer subtracts a signed amount, clamps and emits the real HUD/game
         * adjustment message. Negative subtraction adds fruit through that path. */
        typedef void (ABI *Adjust)(void *,int);
        ((Adjust)(guest+0xff4a00))(racer,current-target);
        return number(racer,0x9c)==target ? NULL : "el juego rechazo Wumpa";
    }
    void *kart=handle(ptr(racer,0x20));
    if(!strcmp(effect,"low_gravity")||!strcmp(effect,"squash")||!strcmp(effect,"fall_void")||!strcmp(effect,"chaos"))return apply_troll(kart,effect,arg,ms,now,authorized);
    if(!strcmp(effect,"boost_now")) {
        void *boost=component(kart,0x2ac4018,0x2589190,0x90);
        typedef void *(ABI *Factory)(void);
        void *factory=((Factory)(guest+0xe36990))();
        void *data=handle(ptr(factory,0x10));
        if(!data) {
            /* CPowerUpFactory's normal fallback to the Octane configuration. */
            void *config=((Factory)(guest+0x9b9ec0))();
            data=ptr(config,0xf8);
        }
        void *turbo=ptr(data,0x58);
        void *boost_data=ptr(turbo,number(racer,0x9c)>=10 ? 0x20 : 0x18);
        if(!boost || !readable(boost_data,0x40))return "turbo no disponible";
        /* The same CKartBoost native API used by TurboBoostPowerUpComponent.
         * Keep the player's inventory and the engine's existing boost assets. */
        void *result=NULL;
        typedef void (ABI *AddBoost)(void **,void *,void *,void *,void *,float);
        ((AddBoost)(guest+0xe2dd10))(&result,boost,boost_data,NULL,NULL,0.0f);
        if(!result)return "el juego rechazo el turbo";
        void *created=handle(result);
        /* Engine strong-handle release, matching the native turbo call site. */
        unsigned previous=atomic_fetch_sub((_Atomic unsigned *)result,1);
        unsigned final_count=(previous&0x90000000) ? 3u : 2u;
        if((previous&0xffff)==final_count) {
            typedef void (ABI *Release)(void **);
            ((Release)(guest+0x149f280))(&result);
        }
        return created ? NULL : "el juego rechazo el turbo";
    }
    void *controller=component(kart,0x2addd90,0x2590a48,0x80);
    if(!controller)return "inventario no disponible";
    if(read_byte(controller,0x40)!=0)return "ruleta de objetos activa";
    void *stored=handle(ptr(controller,0x30));
    if(!strcmp(effect,"remove_item")) {
        if(!readable(stored,0x50))return "no tienes un objeto";
        typedef void (ABI *Remove)(void *,void *);
        ((Remove)(guest+0xbaf1a0))(ptr(stored,0x18),stored);
        return handle(ptr(controller,0x30)) ? "el juego no retiro el objeto" : NULL;
    }
    if(strcmp(effect,"give_item"))return "efecto desconocido";
    if(stored)return "ya tienes un objeto";
    static const struct {const char *id;int type;} options[]={
        {"bomb",0},{"missile",1},{"tnt",2},{"beaker",3},{"mask",4},{"clock",5},{"turbo",6},{"warp_orb",7},{"power_shield",8}
    };
    int type=1;
    if(*arg) {type=-1;for(size_t i=0;i<sizeof options/sizeof options[0];++i)if(!strcmp(arg,options[i].id))type=options[i].type;}
    if(type<0)return "objeto desconocido";
    typedef void (ABI *Give)(void *,int,int,int,int,float);
    ((Give)(guest+0xe366b0))(controller,type,0,0,0,0.0f);
    stored=handle(ptr(controller,0x30));
    if(!stored)return "el juego no entrego el objeto";
    return NULL;
}
static void poll(void *racer,uint64_t now) {
    char text[16385];
    HANDLE file=CreateFileW(inbox_path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE) {if(!last_heartbeat || now-last_heartbeat>2000)clear_timed();return;}
    DWORD bytes=0;
    int read_ok=ReadFile(file,text,sizeof text,&bytes,NULL);
    CloseHandle(file);
    size_t n=bytes;
    /* The writer may publish between the frame timestamp and this read. */
    now=GetTickCount64();
    C90Json j;
    if(!read_ok || n>16384 || !c90_parse(&j,text,(int)n)) {clear_timed();return;}
    uint64_t heartbeat;
    int paused;
    if(!c90_equal(&j,c90_field(&j,0,"session"),session) ||
       !c90_number(&j,c90_field(&j,0,"heartbeatMs"),&heartbeat) ||
       heartbeat>now || now-heartbeat>2000 ||
       !c90_bool(&j,c90_field(&j,0,"paused"),&paused)) {clear_timed();return;}
    int no_death=1,allow=0;
    int nd=c90_field(&j,0,"noDeath"),ad=c90_field(&j,0,"allowDestructive");
    if((nd>=0&&!c90_bool(&j,nd,&no_death)) || (ad>=0&&!c90_bool(&j,ad,&allow))) {clear_timed();return;}
    live_no_death=no_death;live_destructive=allow;
    last_heartbeat=heartbeat;
    if(last_paused && !paused) local_pause=0;
    last_paused=paused;
    if(paused || local_pause || !racer)clear_timed();
    int array=c90_field(&j,0,"effects");
    if(array<0 || j.tokens[array].kind!='[')return;
    int requests=0;
    for(int t=array+1;t<j.tokens[array].next;t=j.tokens[t].next) {
        char id[81],effect[41],arg[41];uint64_t ms,expires;
        if(++requests>20)break;
        if(!c90_string(&j,c90_field(&j,t,"id"),id,sizeof id) || !identifier(id,0) ||
           !c90_string(&j,c90_field(&j,t,"effect"),effect,sizeof effect) || !identifier(effect,0) ||
           !c90_string(&j,c90_field(&j,t,"arg"),arg,sizeof arg) || !identifier(arg,1) ||
           !c90_number(&j,c90_field(&j,t,"ms"),&ms) || ms>120000 ||
           !c90_number(&j,c90_field(&j,t,"expiresMs"),&expires))continue;
        int safe=1,allow_command=0;
        int permission=c90_field(&j,t,"allowDestructive");
        if(!c90_bool(&j,c90_field(&j,t,"safe"),&safe) || (permission>=0&&!c90_bool(&j,permission,&allow_command)))continue;
        int epoch_ok=c90_equal(&j,c90_field(&j,t,"epoch"),race_epoch);
        int duplicate=0;for(unsigned k=0;k<256;++k)if(!strcmp(seen[k],id)){duplicate=1;break;}
        if(duplicate)continue;
        strcpy(seen[seen_count++%256],id);
        if(expires<now || expires-now>10000)ack(id,"efecto vencido");
        else if(paused||local_pause)ack(id,"en pausa");
        else if(!racer)ack(id,"no disponible ahora");
        else if(!epoch_ok)ack(id,"carrera anterior o cliente desactualizado");
        else ack(id,apply(racer,effect,arg,(unsigned)ms,now,!safe&&allow_command));
    }
}
static ABI void update(void *clock) {
    original_update(clock);
    uint64_t now=GetTickCount64();
    int down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    if(down&&!key_down) {local_pause=!local_pause;clear_timed();report(local_pause ? "CLOUD90 pause 1" : "CLOUD90 pause 0");}
    key_down=down;
    void *racer=player_one(clock);
    if(racer!=last_racer){clear_timed();last_racer=racer;snprintf(race_epoch,sizeof race_epoch,"%s-%u",session,++race_generation);char context[128];snprintf(context,sizeof context,"CLOUD90 context %s",race_epoch);report(context);}
    if(!racer || local_pause || !last_heartbeat || now-last_heartbeat>2000)clear_timed();
    else {
        void *kart=handle(ptr(racer,0x20));
        void *respawn=component(kart,0x2ac5948,0x2589460,0x90);
        if(!respawn || number(respawn,0x5c)!=0)clear_timed();
        else tick_troll(now);
    }
    int playing=racer!=NULL;
    if(playing!=in_play || now>=next_state) {
        in_play=playing;next_state=now+1000;
        report(in_play ? "CLOUD90 state 1" : "CLOUD90 state 0");
    }
    if(now>=next_poll) {next_poll=now+50;poll(racer,now);}
}
void runtime_cloud90_start(void *image, size_t size) {
    session=getenv("CLOUD90_CTR_SESSION");
    inbox_path=_wgetenv(L"CLOUD90_CTR_INBOX");
    const char *online=getenv("BB_ONLINE");
    const char *image_hash=getenv("CLOUD90_CTR_IMAGE_SHA256");
    if(!image_hash || strcmp(image_hash,"9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31"))return;
    if (!session || !identifier(session,0) || strlen(session)>80 || !inbox_path || !*inbox_path || !online || strcmp(online,"0") || size<0x2b00000) return;
    guest=image;
    static const unsigned char signature[]={0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53};
    uintptr_t *slot=(uintptr_t *)(guest+0x254ca68);
    if (memcmp(guest+0xa31a10,signature,sizeof signature) || *slot!=(uintptr_t)(guest+0xa31a10)) {
        puts("CLOUD90 incompatible runtime image"); return;
    }
    original_update=(void (ABI *)(void *))*slot;
    *slot=(uintptr_t)update;
    const wchar_t *output_path=_wgetenv(L"CLOUD90_CTR_OUTPUT");
    if(output_path) bridge_output=_wfopen(output_path,L"ab");
    report("CLOUD90 bridge cloud90-ctr-2 offline");
    report("CLOUD90 capabilities wumpa_add,wumpa_full,wumpa_remove,give_item,remove_item,boost_now,invert_steering,no_accelerate,no_items,shield,low_gravity,squash,fall_void,chaos");
    report("CLOUD90 pause 0");
}
void runtime_cloud90_input(uint32_t *buttons, uint8_t *steering, uint8_t *accelerator) {
    if(!in_play || local_pause || !original_update)return;
    uint64_t now=GetTickCount64();
    int chaotic=now<chaos_until&&now<chaos_phase_until;
    if(now<invert_until || (chaotic&&chaos_phase==0))*steering=(uint8_t)(255-*steering);
    if(now<no_gas_until || (chaotic&&chaos_phase==2)) {*buttons&=~(UINT32_C(0x4000)|UINT32_C(0x200));*accelerator=0;}
    if(now<no_items_until || (chaotic&&chaos_phase==1))*buttons&=~(UINT32_C(0x2000)|UINT32_C(0x400));
}
#else
void runtime_cloud90_start(void *image,size_t size) { (void)image; (void)size; }
void runtime_cloud90_input(uint32_t *buttons,uint8_t *steering,uint8_t *accelerator) { (void)buttons; (void)steering; (void)accelerator; }
#endif
