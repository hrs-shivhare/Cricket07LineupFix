#include "selection.h"
#include <string.h>

enum {
    BAT = 1, OBT = 2, BWL = 3, SBW = 4, ASP = 5,
    WK = 6, WKB = 7, OWK = 8, ALL = 9, OBW = 10
};
#define ROLE(r) (1u << (r))
#define BATTING (ROLE(BAT)|ROLE(OBT)|ROLE(ASP)|ROLE(WK)|ROLE(WKB)|ROLE(OWK)|ROLE(ALL))

typedef struct Work {
    const C07Player *players;
    size_t count, selected, bowling_count;
    unsigned char picked[C07_MAX_PLAYERS];
    C07Selection result;
} Work;

static int valid_players(const C07Player *players, size_t count)
{
    if (!players || count < C07_XI || count > C07_MAX_PLAYERS) return 0;
    for (size_t i=0; i<count; ++i) {
        if (!players[i].id || players[i].role>10 || players[i].skill>63 ||
            players[i].accuracy>63 || players[i].primary_field>15 ||
            players[i].secondary_field>15 || players[i].legacy_keeper>63) return 0;
        for (size_t j=0; j<i; ++j) if (players[i].id==players[j].id) return 0;
    }
    return 1;
}

static int best(const Work *w, unsigned roles, int accuracy)
{
    int winner=-1;
    for (size_t i=0; i<w->count; ++i) {
        int rating, previous;
        const C07Player *p=&w->players[i];
        if (w->picked[i] || (roles && !(roles & ROLE(p->role)))) continue;
        rating=accuracy?p->accuracy:p->skill;
        previous=winner<0?-1:accuracy?w->players[winner].accuracy:w->players[winner].skill;
        if (rating>previous) winner=(int)i;
    }
    return winner;
}

static int keepers(const C07Player *p, int tier)
{
    switch (tier) {
    case 0: return p->role==WK || p->role==WKB || p->role==OWK;
    case 1: return p->primary_field==1;
    case 2: return p->secondary_field==1;
    case 3: return p->legacy_keeper==4;
    default: return 1;
    }
}

static void add(Work *w, int index, int bowling)
{
    w->picked[index]=1;
    w->result.xi[w->selected++]=w->players[index].id;
    if (bowling && w->bowling_count<C07_BOWLERS)
        w->result.bowling[w->bowling_count++]=w->players[index].id;
}

/* Priority pools are exhausted in order. A role restriction never prevents a
 * bowling seat from being filled: the final fallback is any remaining role. */
static void bowling_group(Work *w, size_t quota, unsigned roles, unsigned backup)
{
    for (size_t i=0; i<quota && w->selected<C07_XI; ++i) {
        int index=best(w,roles,1);
        if (index<0 && backup) index=best(w,backup,1);
        if (index<0) index=best(w,0,1);
        if (index<0) break;
        add(w,index,1);
    }
}

int c07_select(const C07Player *players, size_t count, int pitch, int is_test,
               C07Selection *selection)
{
    Work w, opening_search;
    unsigned char ordered[C07_MAX_PLAYERS]={0};
    uint16_t allrounders[2]={0};
    size_t all_count, opener_count=0, opening_bowlers=0, batting_quota;
    int keeper=-1, keeper_opens=0, unfilled_flex=0;
    int dusty, green;
    if (!selection || !valid_players(players,count) || pitch<0 || pitch>16 ||
        (is_test!=0 && is_test!=1)) return 0;
    memset(&w,0,sizeof(w));
    w.players=players; w.count=count;
    dusty=pitch==1 || pitch==6 || pitch==7 || pitch==15;
    green=pitch==4 || pitch==8 || pitch==9 || pitch==16;

    /* ALL-quota membership is selected first, including any-role fallbacks.
     * Defer only its bowling-order placement so OBW seats remain first. */
    if(!is_test) bowling_group(&w,dusty||green?2u:1u,ROLE(ALL)|ROLE(ASP),0);
    all_count=w.bowling_count;
    memcpy(allrounders,w.result.bowling,all_count*sizeof(*allrounders));
    memset(w.result.bowling,0,sizeof(w.result.bowling));
    w.bowling_count=0;

    /* Applicable Tests reserve one BAT/ALL by Skill before bowling picks.
     * A missing role candidate leaves this seat for the final batting pool;
     * a flex pick is not a bowling-order seed. */
    if(is_test && !green) {
        int index=best(&w,ROLE(ALL)|ROLE(BAT),0);
        if(index>=0) add(&w,index,0);
        else unfilled_flex=1;
    }

    /* Only actual OBW candidates are used here. Missing seats are deferred. */
    for(;opening_bowlers<2;++opening_bowlers) {
        int index=best(&w,ROLE(OBW),1);
        if(index<0) break;
        add(&w,index,1);
    }
    /* Missing OBW places join the additional BWL/ALL pool before any-role
     * fallback. First replacements fill the vacant opening-bowling seats. */
    bowling_group(&w,(dusty?0u:green?(is_test?2u:0u):1u)+(2-opening_bowlers),
                  ROLE(BWL)|ROLE(ALL),0);
    bowling_group(&w,dusty&&is_test?2u:1u,ROLE(SBW)|ROLE(ASP),0);
    for(size_t i=0;i<all_count;++i)
        w.result.bowling[w.bowling_count++]=allrounders[i];

    /* No keeper is protected before bowling selection. Apply the original
     * priority tiers only to players still available now. */
    for(int tier=0;tier<5 && keeper<0;++tier) {
        for(size_t i=0;i<count;++i)
            if(!w.picked[i] && keepers(&players[i],tier) &&
                (keeper<0 || players[i].skill>=players[keeper].skill)) keeper=(int)i;
    }
    if(keeper<0) return 0;
    w.result.keeper=players[keeper].id;
    add(&w,keeper,0);

    /* Keeper competes on Skill with remaining OBT/OWK; role alone no longer
     * guarantees he is one of the selected opening candidates. */
    opening_search=w;
    opening_search.picked[keeper]=0;
    for(;opener_count<2;++opener_count) {
        int index=best(&opening_search,ROLE(OBT)|ROLE(OWK),0);
        if(index<0) break;
        opening_search.picked[index]=1;
        if(index==keeper) keeper_opens=1;
        else add(&w,index,0);
    }
    batting_quota=3u+(size_t)keeper_opens+(2-opener_count)+(size_t)unfilled_flex;
    for(size_t i=0;i<batting_quota && w.selected<C07_XI;++i) {
        int index=best(&w,BATTING,0);
        if(index<0) break;
        add(&w,index,0);
    }
    /* Fill any remaining batting shortage from any role by Skill. */
    while(w.selected<C07_XI) {
        int index=best(&w,0,0);
        if(index<0) return 0;
        add(&w,index,0);
    }

    /* Every selected OBT/OWK leads, including the keeper if he is an opener.
     * Sort each group by Skill; equal ratings retain original squad order. */
    for (size_t slot=0; slot<C07_XI; ++slot) {
        int winner=-1, winner_opener=-1;
        for (size_t i=0; i<count; ++i) {
            int opener=players[i].role==OBT || players[i].role==OWK;
            if (!w.picked[i] || ordered[i]) continue;
            if (winner<0 || opener>winner_opener ||
                (opener==winner_opener && players[i].skill>players[winner].skill)) {
                winner=(int)i; winner_opener=opener;
            }
        }
        if (winner<0) return 0;
        w.result.xi[slot]=players[winner].id;
        ordered[winner]=1;
    }
    for (;;) {
        int index=best(&w,0,0);
        if (index<0) break;
        w.result.bench[w.result.bench_count++]=players[index].id;
        w.picked[index]=1;
    }
    *selection=w.result;
    return 1;
}
