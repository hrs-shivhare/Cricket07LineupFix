#include "../src/selection.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void setup(C07Player *p, size_t count)
{
    memset(p,0,count*sizeof(*p));
    for (size_t i=0;i<count;++i) {
        p[i].id=(uint16_t)(101+i);
        p[i].skill=(uint8_t)(20-i);
        p[i].accuracy=(uint8_t)i;
    }
}
static int has(const uint16_t *ids,size_t count,uint16_t id)
{
    for(size_t i=0;i<count;++i) if(ids[i]==id) return 1;
    return 0;
}
static size_t index_of(const C07Player *p,size_t count,uint16_t id)
{
    for(size_t i=0;i<count;++i) if(p[i].id==id) return i;
    assert(!"Unknown player"); return 0;
}
static void check(const C07Player *p,size_t count,const C07Selection *s)
{
    int previous_opener=1,previous_skill=64;
    assert(s->bench_count==count-11);
    assert(has(s->xi,11,s->keeper));
    for(size_t i=0;i<11;++i) {
        size_t n=index_of(p,count,s->xi[i]);
        int opener=p[n].role==2 || p[n].role==8;
        assert(!has(s->xi,i,s->xi[i]));
        assert(!has(s->bench,s->bench_count,s->xi[i]));
        assert(previous_opener>=opener);
        if(previous_opener==opener) assert(previous_skill>=p[n].skill);
        previous_opener=opener; previous_skill=p[n].skill;
    }
    for(size_t i=0;i<6;++i) if(s->bowling[i]) {
        assert(has(s->xi,11,s->bowling[i]));
        assert(s->bowling[i]!=s->keeper);
        assert(!has(s->bowling,i,s->bowling[i]));
        if(i) assert(s->bowling[i-1]);
    }
    previous_skill=64;
    for(size_t i=0;i<s->bench_count;++i) {
        size_t n=index_of(p,count,s->bench[i]);
        assert(!has(s->bench,i,s->bench[i]));
        assert(previous_skill>=p[n].skill); previous_skill=p[n].skill;
    }
    for(size_t i=0;i<count;++i)
        assert(has(s->xi,11,p[i].id)+has(s->bench,s->bench_count,p[i].id)==1);
}
static void standard(C07Player *p)
{
    setup(p,20);
    p[0].role=6; p[0].skill=63;
    p[1].role=2; p[1].skill=50;
    p[2].role=2; p[2].skill=49;
    p[3].role=1; p[3].skill=48;
    p[4].role=9; p[4].skill=47;
    p[5].role=5; p[5].skill=46;
    p[6].role=10; p[6].skill=1; p[6].accuracy=63;
    p[7].role=10; p[7].skill=1; p[7].accuracy=62;
    p[8].role=3; p[8].skill=1; p[8].accuracy=61;
    p[9].role=4; p[9].skill=1; p[9].accuracy=60;
    p[10].role=9; p[10].skill=1; p[10].accuracy=59;
    p[11].role=9; p[11].skill=1; p[11].accuracy=58;
    p[12].role=5; p[12].skill=1; p[12].accuracy=57;
}
static void test_keeper_tiers(void)
{
    C07Player p[20]; C07Selection s;
    setup(p,20);
    p[0].role=6;p[0].skill=40;
    p[1].role=7;p[1].skill=50;
    p[2].role=8;p[2].skill=50;
    p[3].primary_field=1;p[3].skill=63;
    assert(c07_select(p,20,0,0,&s));assert(s.keeper==p[2].id);check(p,20,&s);
    p[0].role=p[1].role=p[2].role=0;
    assert(c07_select(p,20,0,0,&s));assert(s.keeper==p[3].id);
    p[3].primary_field=0;p[4].secondary_field=1;
    assert(c07_select(p,20,0,0,&s));assert(s.keeper==p[4].id);
    p[4].secondary_field=0;p[5].legacy_keeper=4;
    assert(c07_select(p,20,0,0,&s));assert(s.keeper==p[5].id);
    p[5].legacy_keeper=0;
    assert(c07_select(p,20,0,0,&s));assert(s.keeper==p[3].id);
}
static void test_all_quota_first_and_provenance(void)
{
    C07Player p[20];C07Selection s;
    standard(p);p[4].accuracy=63;p[13].role=1;p[13].skill=45;
    assert(c07_select(p,20,0,0,&s));
    assert(s.bowling[4]==p[4].id);assert(has(s.xi,11,p[13].id));
    assert(!has(s.xi,11,p[10].id));
    standard(p);p[4].accuracy=63;p[5].accuracy=62;
    p[13].role=p[14].role=1;p[13].skill=45;p[14].skill=44;
    assert(c07_select(p,20,8,0,&s));
    assert(s.bowling[3]==p[4].id&&s.bowling[4]==p[5].id);
    assert(has(s.xi,11,p[13].id)&&has(s.xi,11,p[14].id));

    /* With no ALL/ASP, an OBW chosen by any-role fallback still belongs to the
       ALL quota. The OBW pass must find two other players / replacements. */
    standard(p);
    p[4].role=p[5].role=p[10].role=p[11].role=p[12].role=0;
    assert(c07_select(p,20,0,0,&s));
    const uint16_t seed[]={p[7].id,p[8].id,p[9].id,p[10].id,p[6].id,0};
    assert(!memcmp(s.bowling,seed,sizeof(seed)));check(p,20,&s);
}
static void test_missing_obw_combined_pool_and_spin(void)
{
    C07Player p[20];C07Selection s;
    standard(p);p[6].role=p[7].role=0;p[4].role=p[5].role=1;
    assert(c07_select(p,20,0,0,&s));
    /* Deferred missing OBW seats use BWL/ALL first, before any role. */
    const uint16_t seed[]={p[8].id,p[11].id,p[6].id,p[9].id,p[10].id,0};
    assert(!memcmp(s.bowling,seed,sizeof(seed)));
    standard(p);p[12].accuracy=63;p[10].accuracy=62;p[11].accuracy=61;
    assert(c07_select(p,20,8,0,&s));
    assert(s.bowling[2]==p[9].id); /* ASP was already consumed by ALL quota. */
    assert(s.bowling[3]==p[12].id&&s.bowling[4]==p[10].id);
    check(p,20,&s);
}
static void test_keeper_remaining_after_bowling(void)
{
    C07Player p[20];C07Selection s;
    setup(p,20);
    p[0].role=6;p[0].skill=63;p[0].accuracy=63;
    p[1].role=7;p[1].skill=60;p[1].accuracy=0;
    p[2].role=2;p[2].skill=59;p[2].accuracy=0;
    p[3].role=2;p[3].skill=58;p[3].accuracy=0;
    for(size_t i=4;i<7;++i){p[i].role=1;p[i].skill=(uint8_t)(61-i);}
    p[7].role=p[8].role=10;p[7].accuracy=62;p[8].accuracy=61;
    p[9].role=3;p[9].accuracy=60;p[10].role=4;p[10].accuracy=59;
    assert(c07_select(p,20,0,0,&s));
    assert(s.bowling[4]==p[0].id); /* Best batting WK consumed by ALL fallback. */
    assert(s.keeper==p[1].id);check(p,20,&s);
}
static void test_keeper_opener_competition_and_missing(void)
{
    C07Player p[20];C07Selection s;
    standard(p);p[0].role=8;p[0].skill=45;p[13].role=1;p[13].skill=44;
    assert(c07_select(p,20,0,0,&s));
    assert(s.keeper==p[0].id&&s.xi[0]==p[1].id&&s.xi[1]==p[2].id);
    assert(s.xi[2]==p[0].id&&!has(s.xi,11,p[13].id)); /* Keeper is third opener. */
    p[0].skill=63;p[2].skill=1;p[13].skill=45;
    assert(c07_select(p,20,0,0,&s));
    assert(s.xi[0]==p[0].id&&s.xi[1]==p[1].id);
    assert(has(s.xi,11,p[13].id)&&!has(s.xi,11,p[2].id)); /* Four batting picks. */
    for(unsigned missing=0;missing<=2;++missing){
        standard(p);
        if(missing)p[2].role=0;
        if(missing==2)p[1].role=0;
        for(size_t i=13;i<16;++i){p[i].role=1;p[i].skill=(uint8_t)(58-i);}
        assert(c07_select(p,20,0,0,&s));
        assert(has(s.xi,11,p[13].id)==(missing>=1));
        assert(has(s.xi,11,p[14].id)==(missing>=2));
        assert(!has(s.xi,11,p[15].id));check(p,20,&s);
    }
}
static void test_batting_pool_test_flex_and_fallback(void)
{
    C07Player p[20];C07Selection s;
    const unsigned roles[]={1,2,5,6,7,8,9};
    for(size_t r=0;r<sizeof(roles)/sizeof(roles[0]);++r){
        standard(p);p[13].role=(uint8_t)roles[r];p[13].skill=45;p[13].accuracy=0;
        assert(c07_select(p,20,0,1,&s));assert(has(s.xi,11,p[13].id));
        check(p,20,&s);
    }
    standard(p);p[3].role=p[4].role=p[5].role=0;
    assert(c07_select(p,20,0,0,&s));
    assert(has(s.xi,11,p[11].id)&&has(s.xi,11,p[12].id)); /* Broad pool first. */
    assert(has(s.xi,11,p[3].id)&&!has(s.xi,11,p[4].id)); /* Any-role Skill fallback. */
    /* Test flex is merged: equal-Skill BAT earlier in squad beats ALL. */
    standard(p);p[10].role=1;p[10].skill=40;p[11].skill=40;
    assert(c07_select(p,20,0,1,&s));assert(has(s.xi,11,p[10].id));
    assert(!has(s.xi,11,p[11].id));
}
static void test_pitch_quotas_and_order(void)
{
    C07Player p[20];C07Selection s;
    for(int pitch=0;pitch<17;++pitch)for(int test=0;test<2;++test){
        size_t n=0;int green=pitch==4||pitch==8||pitch==9||pitch==16;
        standard(p);assert(c07_select(p,20,pitch,test,&s));
        while(n<6&&s.bowling[n])++n;
        assert(n==(test?(green?5u:4u):5u));check(p,20,&s);
    }
    standard(p);p[3].role=2;p[3].skill=48;
    assert(c07_select(p,20,0,0,&s));
    assert(s.xi[0]==p[1].id&&s.xi[1]==p[2].id&&s.xi[2]==p[3].id);
    assert(s.xi[3]==p[0].id); /* Every selected opener precedes non-openers. */
}
static uint32_t rng=0xc07;
static unsigned random_value(void){rng=rng*1664525u+1013904223u;return rng;}
static void test_varied_and_invalid(void)
{
    C07Player p[20];C07Selection s,saved;
    for(unsigned trial=0;trial<6000;++trial) {
        size_t count=11+random_value()%10;setup(p,count);
        for(size_t i=0;i<count;++i) {
            p[i].role=(uint8_t)(random_value()%11);
            p[i].skill=(uint8_t)(random_value()%64);p[i].accuracy=(uint8_t)(random_value()%64);
        }
        assert(c07_select(p,count,(int)(trial%17),(int)(trial%2),&s));check(p,count,&s);
    }
    setup(p,20); /* No eligible batters: defer the holes, then fill the final XI. */
    assert(c07_select(p,20,0,0,&s));check(p,20,&s);
    memset(&s,0xa5,sizeof(s));saved=s;
    assert(!c07_select(p,10,0,0,&s));assert(!memcmp(&s,&saved,sizeof(s)));
    assert(!c07_select(p,20,17,0,&s));assert(!memcmp(&s,&saved,sizeof(s)));
    p[1].id=p[0].id;assert(!c07_select(p,20,0,0,&s));p[1].id=102;
    p[0].role=11;assert(!c07_select(p,20,0,0,&s));p[0].role=0;
    p[0].skill=64;assert(!c07_select(p,20,0,0,&s));p[0].skill=20;
    p[0].id=0x1cb7;
    assert(c07_select(p,11,0,0,&s));assert(has(s.xi,11,0x1cb7));
}
int main(void)
{
    test_keeper_tiers();test_all_quota_first_and_provenance();
    test_missing_obw_combined_pool_and_spin();test_keeper_remaining_after_bowling();
    test_keeper_opener_competition_and_missing();test_batting_pool_test_flex_and_fallback();
    test_pitch_quotas_and_order();test_varied_and_invalid();
    puts("Cricket07Lineup 3.0 core tests passed (6000 varied squads plus directed rules).");
    return 0;
}
