#include "../src/cloud90_json.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    C90Json j;char value[8];uint64_t n;int b;
    const char *text="{\"session\":\"abc\",\"heartbeatMs\":123,\"paused\":false,\"effects\":[{\"id\":\"a\"}]}";
    assert(c90_parse(&j,text,(int)strlen(text)));
    assert(c90_equal(&j,c90_field(&j,0,"session"),"abc"));
    assert(c90_string(&j,c90_field(&j,0,"session"),value,sizeof value));
    assert(!strcmp(value,"abc"));
    assert(c90_number(&j,c90_field(&j,0,"heartbeatMs"),&n)&&n==123);
    assert(c90_bool(&j,c90_field(&j,0,"paused"),&b)&&b==0);
    const char *bad[]={"{}junk","{\"a\":}","{\"a\":1,}","{\"a\":01}","{\"a\":-1}","{\"a\":\"\\u0000\"}","{\"a\":[1,]}","{\"a\":truee}"};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];++i) assert(!c90_parse(&j,bad[i],(int)strlen(bad[i])));
    text="{\"x\":1,\"x\":2}";assert(c90_parse(&j,text,(int)strlen(text)));assert(c90_field(&j,0,"x")==-1);
    text="{\"x\":18446744073709551616}";assert(c90_parse(&j,text,(int)strlen(text)));assert(!c90_number(&j,c90_field(&j,0,"x"),&n));
    puts("Cloud90 JSON: malformed, duplicate and overflowing fields rejected");
    return 0;
}
