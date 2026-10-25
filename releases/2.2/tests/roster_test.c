/* Replays actual extracted roster data through the replacement selection core.
 * Does not execute native game code or native bowling-order completion. */
#include "../src/selection.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { PLAYER_COUNT = 1200, PLAYER_SIZE = 360, TEAM_COUNT = 67, TEAM_SIZE = 2808 };

typedef struct RosterPlayer {
    C07Player player;
    char name[61];
} RosterPlayer;

static unsigned read16(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static uint32_t read32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int includes(const uint16_t *ids, size_t count, uint16_t id)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (ids[i] == id)
            return 1;
    return 0;
}

static int roster_index(const RosterPlayer *roster, uint16_t id)
{
    size_t i;
    if (!id) return -1;
    for (i = 0; i < PLAYER_COUNT; ++i)
        if (roster[i].player.id == id)
            return (int)i;
    return -1;
}

static size_t player_index(const C07Player *players, size_t count, uint16_t id)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (players[i].id == id)
            return i;
    assert(!"XI member not in squad");
    return 0;
}

static void check_result(const C07Player *players,size_t count,const C07Selection *s)
{
    int previous_opener=1,previous_skill=64;
    size_t total=0;
    assert(s->bench_count==count-11);
    assert(includes(s->xi,11,s->keeper));
    for(size_t i=0;i<11;++i) {
        size_t n=player_index(players,count,s->xi[i]);
        int opener=players[n].role==2||players[n].role==8;
        assert(!includes(s->xi,i,s->xi[i]));
        assert(!includes(s->bench,s->bench_count,s->xi[i]));
        assert(previous_opener>=opener);
        if(previous_opener==opener)assert(previous_skill>=players[n].skill);
        previous_opener=opener;previous_skill=players[n].skill;
    }
    for(size_t i=0;i<6;++i)if(s->bowling[i]){
        assert(includes(s->xi,11,s->bowling[i])&&s->bowling[i]!=s->keeper);
        assert(!includes(s->bowling,i,s->bowling[i]));
        if(i)assert(s->bowling[i-1]);
    }
    previous_skill=64;
    for(size_t i=0;i<s->bench_count;++i){
        size_t n=player_index(players,count,s->bench[i]);
        assert(previous_skill>=players[n].skill);previous_skill=players[n].skill;
    }
    for(size_t i=0;i<count;++i){
        total+=includes(s->xi,11,players[i].id);
        total+=includes(s->bench,s->bench_count,players[i].id);
    }
    assert(total==count);
}
static void print_xi(const RosterPlayer *roster,size_t team,int pitch,const C07Selection *s)
{
    printf("Roster replay team=%u limited pitch=%d:\n",(unsigned)team,pitch);
    for(size_t i=0;i<11;++i){
        int index=roster_index(roster,s->xi[i]);
        assert(index>=0);
        printf(" %2u. %-20s ID=%u role=%u Skill(raw)=%u Accuracy(raw)=%u%s\n",
            (unsigned)i+1,roster[index].name,s->xi[i],roster[index].player.role,
            roster[index].player.skill,roster[index].player.accuracy,
            s->xi[i]==s->keeper?" [keeper]":"");
    }
}
int main(int argc,char **argv)
{
    const char *path=argc>1?argv[1]:"analysis/batting_20261021/frontend__database__cricket.0.0.12.0.cdf";
    const size_t size=4u+PLAYER_COUNT*PLAYER_SIZE+TEAM_COUNT*TEAM_SIZE;
    unsigned char *data=malloc(size);
    RosterPlayer *roster=calloc(PLAYER_COUNT,sizeof(*roster));
    FILE *file=fopen(path,"rb");
    size_t teams=0,cases=0,skipped=0;
    assert(data&&roster);
    if(!file){fprintf(stderr,"Cannot read roster: %s\n",path);return 1;}
    assert(fread(data,1,size,file)==size);assert(fgetc(file)==EOF);fclose(file);
    for(size_t i=0;i<PLAYER_COUNT;++i){
        const unsigned char *p=data+4+i*PLAYER_SIZE;
        roster[i].player.id=(uint16_t)read16(p);
        roster[i].player.role=p[0x15f]&15;
        roster[i].player.skill=(uint8_t)((read32(p+0x6c)>>6)&63);
        roster[i].player.accuracy=(uint8_t)((read32(p+0x74)>>12)&63);
        roster[i].player.primary_field=(uint8_t)((read32(p+0x84)>>10)&15);
        roster[i].player.secondary_field=(uint8_t)((read32(p+0x84)>>14)&15);
        roster[i].player.legacy_keeper=(uint8_t)(read32(p+0x8c)&63);
        memcpy(roster[i].name,p+4,60);
    }
    for(size_t t=0;t<TEAM_COUNT;++t){
        C07Player players[C07_MAX_PLAYERS];uint16_t ids[C07_MAX_PLAYERS];size_t count=0;
        const unsigned char *team=data+4+PLAYER_COUNT*PLAYER_SIZE+t*TEAM_SIZE;
        for(size_t i=0;i<20;++i){
            uint16_t id=(uint16_t)read16(team+2*i);
            if(!id)continue;
            int n=roster_index(roster,id);
            if(n<0||roster[n].player.role>10||includes(ids,count,id)){++skipped;continue;}
            ids[count]=id;players[count++]=roster[n].player;
        }
        if(count<11)continue;
        ++teams;
        for(int pitch=0;pitch<17;++pitch)for(int test=0;test<2;++test){
            C07Selection s;assert(c07_select(players,count,pitch,test,&s));
            check_result(players,count,&s);
            if((t==0||t==13||t==30)&&!test&&(pitch==0||pitch==8))print_xi(roster,t,pitch,&s);
            ++cases;
        }
    }
    printf("Roster replay passed: %u teams, %u cases (17 pitches x 2 formats), %u invalid entries skipped.\n",
        (unsigned)teams,(unsigned)cases,(unsigned)skipped);
    puts("Evidence scope: actual extracted roster + replacement core; no native game execution.");
    free(roster);free(data);return 0;
}
