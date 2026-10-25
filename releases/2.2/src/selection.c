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

static int best(const Work *w, unsigned roles, int accuracy, int flex_tie)
{
    int winner=-1;
    for (size_t i=0; i<w->count; ++i) {
        int rating, previous;
        const C07Player *p=&w->players[i];
        if (w->picked[i] || (roles && !(roles & ROLE(p->role)))) continue;
        rating=accuracy?p->accuracy:p->skill;
        previous=winner<0?-1:accuracy?w->players[winner].accuracy:w->players[winner].skill;
        if (rating>previous || (flex_tie && rating==previous && winner>=0 &&
            p->role==ALL && w->players[winner].role==BAT)) winner=(int)i;
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
        int index=best(w,roles,1,0);
        if (index<0 && backup) index=best(w,backup,1,0);
        if (index<0) index=best(w,0,1,0);
        if (index<0) break;
        add(w,index,1);
    }
}

static void allrounder_group(Work *w, size_t quota, const unsigned char *batting_selected)
{
    Work ranking=*w;
    size_t credits=0;
    /* Rank the whole squad's eligible ALL/ASP. Only the initial batting group
     * earns Skill replacements; later bowling picks do not. Keeper remains
     * excluded. New replacements are not credited a second time. */
    memset(ranking.picked,0,sizeof(ranking.picked));
    for(size_t i=0;i<w->count;++i)
        if(w->players[i].id==w->result.keeper) ranking.picked[i]=1;
    for(size_t i=0;i<quota;++i) {
        int index=best(&ranking,ROLE(ALL)|ROLE(ASP),1,0);
        if(index<0) break;
        ranking.picked[index]=1;
        if(batting_selected[index]) ++credits;
    }
    for(size_t i=0;i<credits && w->selected<C07_XI;++i) {
        int index=best(w,0,0,0);
        if(index<0) break;
        add(w,index,0);
    }
    bowling_group(w,quota-credits,ROLE(ALL)|ROLE(ASP),0);
}

int c07_select(const C07Player *players, size_t count, int pitch, int is_test,
               C07Selection *selection)
{
    Work w;
    unsigned char ordered[C07_MAX_PLAYERS]={0};
    unsigned char batting_selected[C07_MAX_PLAYERS];
    int keeper=-1;
    int dusty, green;
    if (!selection || !valid_players(players,count) || pitch<0 || pitch>16 ||
        (is_test!=0 && is_test!=1)) return 0;
    memset(&w,0,sizeof(w));
    w.players=players; w.count=count;
    for (int tier=0; tier<5 && keeper<0; ++tier) {
        for (size_t i=0; i<count; ++i)
            if (keepers(&players[i],tier) &&
                (keeper<0 || players[i].skill>=players[keeper].skill)) keeper=(int)i;
    }
    w.result.keeper=players[keeper].id;
    add(&w,keeper,0);

    /* A designated keeper of either opening role counts as one opener. */
    for (size_t i=(players[keeper].role==OBT || players[keeper].role==OWK)?1u:0u;
         i<2; ++i) {
        int index=best(&w,ROLE(OBT)|ROLE(OWK),0,0);
        if (index<0) break;
        add(&w,index,0);
    }
    /* Missing opening places increase the number of these Skill picks. */
    while (w.selected<6) {
        int index=best(&w,BATTING,0,0);
        if (index<0) break;
        add(&w,index,0);
    }

    memcpy(batting_selected,w.picked,sizeof(batting_selected));
    dusty=pitch==1 || pitch==6 || pitch==7 || pitch==15;
    green=pitch==4 || pitch==8 || pitch==9 || pitch==16;
    bowling_group(&w,2,ROLE(OBW),ROLE(BWL));
    bowling_group(&w,dusty?0u:green?(is_test?2u:0u):1u,ROLE(BWL)|ROLE(ALL),0);
    bowling_group(&w,dusty&&is_test?2u:1u,ROLE(SBW)|ROLE(ASP),0);
    if (!is_test) {
        allrounder_group(&w,dusty||green?2u:1u,batting_selected);
    } else if (!green && w.selected<C07_XI) {
        int index=best(&w,ROLE(BAT)|ROLE(ALL),0,1);
        if (index>=0) add(&w,index,0);
    }
    /* Unfilled batting/flex places remain available until all bowling passes
     * have run; fill any final vacancies by Skill from every remaining role. */
    while (w.selected<C07_XI) {
        int index=best(&w,0,0,0);
        if (index<0) return 0;
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
        int index=best(&w,0,0,0);
        if (index<0) break;
        w.result.bench[w.result.bench_count++]=players[index].id;
        w.picked[index]=1;
    }
    *selection=w.result;
    return 1;
}
