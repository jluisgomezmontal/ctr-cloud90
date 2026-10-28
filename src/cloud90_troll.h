/* Version-specific CTR troll adapters. Included only in the Windows bridge. */
static uint64_t gravity_until,chaos_next;
static _Atomic uint64_t chaos_until,chaos_phase_until;
static unsigned chaos_seed;
static _Atomic unsigned chaos_phase;
static void *gravity_physics,*gravity_original,*gravity_copy;
static uint32_t gravity_body;
static void *gravity_world;
static char race_epoch[101];
static unsigned race_generation;
static int live_no_death=1,live_destructive;

static int writable(void *p,size_t n) {
    MEMORY_BASIC_INFORMATION m;
    if(!readable(p,n) || !VirtualQuery(p,&m,sizeof m))return 0;
    return (m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
static int body_valid(void *physics) {
    /* Player karts use CBehaviorPhysicsGenericShapeComponent, a derived
     * CPhysicsComponent with the same native body at +0x40. Checking only
     * the base vtable rejects the actual kart before reading its body. */
    if(!is_object(physics,0x2589f00,0x60) && !is_object(physics,0x258fc38,0x88))return 0;
    uint32_t id=(uint32_t)number(physics,0x40);
    void *world=ptr(physics,0x48),*bodies=ptr(world,0x20);
    uint32_t count=(uint32_t)number(world,0x2c)&UINT32_C(0x3fffffff);
    return id< count && id<UINT32_C(1000000) && readable((char *)bodies+(size_t)id*0x90,0x90) && (read_byte((char *)bodies+(size_t)id*0x90,0x40)&3)!=0;
}
static void object_hold(void *p) {atomic_fetch_add((_Atomic unsigned *)((char *)p+8),1);}
static void object_release(void *p) {if(p) {typedef void(ABI *Release)(void *);C90_CALL(0x149d260,Release)(p);}}
static void *motion_properties(void *p) {typedef void *(ABI *Get)(void *);return C90_CALL(0xa8be70,Get)((char *)p+0x40);}
static void set_motion(void *p,void *properties) {typedef void(ABI *Set)(void *,void *);C90_CALL(0xa8a9c0,Set)((char *)p+0x40,properties);}
static void restore_gravity(void) {
    if(gravity_physics && body_valid(gravity_physics) && (uint32_t)number(gravity_physics,0x40)==gravity_body && ptr(gravity_physics,0x48)==gravity_world && motion_properties(gravity_physics)==gravity_copy)
        set_motion(gravity_physics,gravity_original);
    object_release(gravity_copy);object_release(gravity_original);object_release(gravity_physics);
    gravity_copy=gravity_original=gravity_physics=NULL;gravity_until=0;
}
static const char *start_gravity(void *kart,unsigned ms,uint64_t now) {
    if(gravity_until>now)return "gravedad ya activa";
    restore_gravity();
    void *physics=component(kart,0x2ab10e0,0x258fc38,0x88);
    if(!physics)physics=component(kart,0x2acfc10,0x2589f00,0x60);
    if(!body_valid(physics) || !writable(physics,0x60))return "fisica no disponible";
    void *original=motion_properties(physics),*copy=NULL;
    if(!writable(original,0x40))return "gravedad no disponible";
    float factor;memcpy(&factor,(char *)original+0x18,4);
    if(!(factor>0.01f && factor<100.0f))return "gravedad no compatible";
    /* Native clone creates independent motion properties, as used by the
     * game's own gravity toggle. Never edit a shared tuning asset. */
    typedef void(ABI *Clone)(void **,void *);
    C90_CALL(0xa8d490,Clone)(&copy,original);
    if(!writable(copy,0x40)) {object_release(copy);return "no se pudo clonar gravedad";}
    factor*=0.5f;memcpy((char *)copy+0x18,&factor,4);
    object_hold(physics);object_hold(original);
    gravity_physics=physics;gravity_original=original;gravity_copy=copy;
    gravity_body=(uint32_t)number(physics,0x40);gravity_world=ptr(physics,0x48);
    set_motion(physics,copy);
    void *applied=motion_properties(physics);
    if(applied!=copy) {
        if(!writable(applied,0x40)) {set_motion(physics,original);restore_gravity();return "gravedad no aplicada";}
        object_hold(applied);object_release(copy);gravity_copy=applied;
    }
    float actual;memcpy(&actual,(char *)gravity_copy+0x18,4);
    if(!(actual>factor*0.999f && actual<factor*1.001f)) {restore_gravity();return "gravedad no aplicada";}
    gravity_until=now+(ms<10000?ms:10000);
    return NULL;
}
static unsigned chaos_random(void) {
    chaos_seed^=chaos_seed<<13;chaos_seed^=chaos_seed>>17;chaos_seed^=chaos_seed<<5;
    return chaos_seed;
}
static void tick_troll(uint64_t now) {
    if(gravity_physics && (now>=gravity_until || !body_valid(gravity_physics) || motion_properties(gravity_physics)!=gravity_copy))restore_gravity();
    if(chaos_until<=now) {chaos_until=0;return;}
    if(now>=chaos_next) {
        unsigned next=chaos_random()%3;
        if(next==chaos_phase)next=(next+1)%3;
        chaos_phase=next;chaos_next=now+2000;
        chaos_phase_until=now+(next==2?1000:2000);
    }
}
static const char *apply_troll(void *kart,const char *effect,const char *arg,unsigned ms,uint64_t now,int authorized) {
    if(*arg)return "opcion desconocida";
    if((!strcmp(effect,"low_gravity")||!strcmp(effect,"chaos")) && (ms==0||ms>10000))return "duracion no admitida";
    if(!strcmp(effect,"fall_void") && (!authorized || live_no_death || !live_destructive))return "caida al vacio desactivada";
    if(!strcmp(effect,"low_gravity"))return start_gravity(kart,ms,now);
    if(!strcmp(effect,"chaos")) {
        if(chaos_until>now)return "caos ya activo";
        chaos_until=now+(ms<10000?ms:10000);chaos_next=now;chaos_phase=3;
        chaos_seed=(unsigned)now^0x90c7f123; if(!chaos_seed)chaos_seed=1;
        tick_troll(now);return NULL;
    }
    void *respawn=component(kart,0x2ac5948,0x2589460,0x90);
    if(!respawn || number(respawn,0x5c)!=0)return "rescate en curso o no disponible";
    void *damage=component(kart,0x2ac5018,0x2587410,0xe0);
    if(!damage || number(damage,0x40)!=-1)return "kart protegido o recibiendo dano";
    typedef int(ABI *CanDamage)(void *,int);
    if(!C90_CALL(0xe346f0,CanDamage)(damage,0))return "kart protegido";
    if(!strcmp(effect,"squash")) {
        /* Native kart collision call uses damage type 8, attacker handle NULL,
         * and no invulnerability bypass. Native duration/recovery are retained. */
        if(!ptr(damage,0x70+8*8))return "aplastamiento no disponible";
        typedef int(ABI *Hit)(void *,int,void *,int);
        C90_CALL(0xe330e0,Hit)(damage,8,NULL,0);
        return number(damage,0x40)==8 ? NULL : "aplastamiento rechazado";
    }
    if(!strcmp(effect,"fall_void")) {
        if(!authorized || live_no_death || !live_destructive)return "caida al vacio desactivada";
        clear_timed();
        /* Type 0 and NULL point match the automatic fall handler: let the
         * engine choose its valid rescue point, preserving race progress. */
        typedef void(ABI *Fall)(void *,int,void *);
        C90_CALL(0xe7dd00,Fall)(respawn,0,NULL);
        return number(respawn,0x5c)!=0 ? NULL : "rescate no iniciado";
    }
    return "efecto desconocido";
}
