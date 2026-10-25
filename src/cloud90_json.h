/* Small bounded JSON reader for the local bridge's ASCII identifiers.
 * No allocation, execution, escape decoding or third-party dependencies. */
#ifndef CLOUD90_JSON_H
#define CLOUD90_JSON_H
#include <stdint.h>
#include <string.h>
#include <stddef.h>
typedef struct { int start,end,next; char kind; } C90Token;
typedef struct { const char *text; int size,pos,count; C90Token tokens[512]; } C90Json;
static void c90_space(C90Json *j) { while(j->pos<j->size && strchr(" \r\n\t",j->text[j->pos])) ++j->pos; }
static int c90_value(C90Json *j,int depth) {
    c90_space(j);
    if(depth>8 || j->count==512 || j->pos>=j->size) return -1;
    int n=j->count++; C90Token *t=&j->tokens[n];
    t->start=j->pos; t->kind=j->text[j->pos++];
    if(t->kind=='{' || t->kind=='[') {
        char close=t->kind=='{' ? '}' : ']';
        c90_space(j);
        if(j->pos<j->size && j->text[j->pos]==close) ++j->pos;
        else for(;;) {
            if(t->kind=='{') {
                c90_space(j);
                if(j->pos>=j->size || j->text[j->pos]!='"' || c90_value(j,depth+1)<0) return -1;
                c90_space(j);
                if(j->pos>=j->size || j->text[j->pos++]!=':') return -1;
            }
            if(c90_value(j,depth+1)<0) return -1;
            c90_space(j);
            if(j->pos>=j->size) return -1;
            char c=j->text[j->pos++];
            if(c==close) break;
            if(c!=',') return -1;
        }
    } else if(t->kind=='"') {
        while(j->pos<j->size && j->text[j->pos]!='"') {
            unsigned char c=(unsigned char)j->text[j->pos++];
            if(c<32 || c>=127 || c=='\\') return -1;
        }
        if(j->pos>=j->size) return -1;
        ++j->pos;
    } else if(t->kind>='0' && t->kind<='9') {
        while(j->pos<j->size && j->text[j->pos]>='0' && j->text[j->pos]<='9') ++j->pos;
        if(t->kind=='0' && j->pos-t->start>1) return -1;
    } else {
        const char *v=t->kind=='t' ? "true" : t->kind=='f' ? "false" : "";
        size_t length=strlen(v);
        if(!length || t->start+(int)length>j->size || memcmp(j->text+t->start,v,length)) return -1;
        j->pos=t->start+(int)length;
    }
    t->end=j->pos; t->next=j->count; return n;
}
static int c90_parse(C90Json *j,const char *text,int size) {
    memset(j,0,sizeof *j); j->text=text; j->size=size;
    if(size<2 || size>16384 || c90_value(j,0)!=0 || j->tokens[0].kind!='{') return 0;
    c90_space(j); return j->pos==size;
}
static int c90_equal(const C90Json *j,int n,const char *s) {
    if(n<0 || n>=j->count || j->tokens[n].kind!='"') return 0;
    const C90Token *t=&j->tokens[n];
    return (size_t)(t->end-t->start-2)==strlen(s) && !memcmp(j->text+t->start+1,s,strlen(s));
}
static int c90_field(const C90Json *j,int n,const char *key) {
    if(n<0 || n>=j->count || j->tokens[n].kind!='{') return -1;
    int found=-1;
    for(int k=n+1;k<j->tokens[n].next;) {
        int value=k+1;
        if(c90_equal(j,k,key)) { if(found>=0) return -1; found=value; }
        k=j->tokens[value].next;
    }
    return found;
}
static int c90_string(const C90Json *j,int n,char *out,size_t size) {
    if(n<0 || n>=j->count || j->tokens[n].kind!='"') return 0;
    const C90Token *t=&j->tokens[n]; size_t len=(size_t)(t->end-t->start-2);
    if(len>=size) return 0;
    memcpy(out,j->text+t->start+1,len);out[len]=0;return 1;
}
static int c90_number(const C90Json *j,int n,uint64_t *out) {
    if(n<0 || n>=j->count || j->tokens[n].kind<'0' || j->tokens[n].kind>'9') return 0;
    uint64_t v=0;
    for(int i=j->tokens[n].start;i<j->tokens[n].end;++i) {
        unsigned digit=(unsigned)(j->text[i]-'0');
        if(v>(UINT64_MAX-digit)/10) return 0;
        v=v*10+digit;
    }
    *out=v;return 1;
}
static int c90_bool(const C90Json *j,int n,int *out) {
    if(n<0 || n>=j->count || (j->tokens[n].kind!='t' && j->tokens[n].kind!='f')) return 0;
    *out=j->tokens[n].kind=='t';return 1;
}
#endif
