#include "selection.h"

#include <string.h>

enum { ROLE_BAT = 1, ROLE_OBT = 2, ROLE_ASP = 5,
       ROLE_WK = 6, ROLE_WKB = 7, ROLE_OWK = 8, ROLE_ALL = 9 };

static int valid_players(const C07Player *players, size_t count)
{
    size_t i, j;
    if (!players || count < C07_XI || count > C07_MAX_PLAYERS)
        return 0;
    for (i = 0; i < count; ++i) {
        if (!players[i].id || players[i].role > 10 ||
            players[i].skill > 63 || players[i].accuracy > 63 ||
            players[i].primary_field > 63 || players[i].secondary_field > 63 ||
            players[i].legacy_keeper > 63)
            return 0;
        for (j = 0; j < i; ++j)
            if (players[i].id == players[j].id)
                return 0;
    }
    return 1;
}

static int index_of(const C07Player *players, size_t count, uint16_t id)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (players[i].id == id)
            return (int)i;
    return -1;
}

static int keeper_tier(const C07Player *player, int tier)
{
    switch (tier) {
    case 0: return player->role == ROLE_WK || player->role == ROLE_WKB ||
                   player->role == ROLE_OWK;
    case 1: return player->primary_field == 1;
    case 2: return player->secondary_field == 1;
    case 3: return player->legacy_keeper == 4;
    default: return 1;
    }
}

int c07_begin(const C07Player *players, size_t count, C07Selection *selection)
{
    C07Selection result;
    int keeper = -1, tier, best, openers[2] = { -1, -1 };
    size_t i, n;
    if (!selection || !valid_players(players, count))
        return 0;
    memset(&result, 0, sizeof(result));
    for (tier = 0; tier < 5 && keeper < 0; ++tier) {
        for (i = 0; i < count; ++i) {
            if (keeper_tier(&players[i], tier) &&
                (keeper < 0 || players[i].skill >= players[keeper].skill))
                keeper = (int)i;
        }
    }
    result.keeper = players[keeper].id;
    for (n = 0; n < (players[keeper].role == ROLE_OWK ? 1u : 2u); ++n) {
        best = -1;
        for (i = 0; i < count; ++i) {
            if ((int)i == keeper || (int)i == openers[0] ||
                (players[i].role != ROLE_OBT && players[i].role != ROLE_OWK))
                continue;
            if (best < 0 || players[i].skill > players[best].skill)
                best = (int)i;
        }
        openers[n] = best;
    }
    if (players[keeper].role == ROLE_OWK)
        result.xi[0] = players[keeper].id;
    if (openers[0] >= 0)
        result.xi[1] = players[openers[0]].id;
    if (players[keeper].role != ROLE_OWK && openers[1] >= 0)
        result.xi[0] = players[openers[1]].id;
    *selection = result;
    return 1;
}

/* Select from currently unpicked players; equal ratings retain squad order.
 * mode: 0=any, 1=BAT, 2=ALL, 3=ALL or ASP, 4=BAT or ALL,
 *       5=batting pool: BAT, OBT, ALL, ASP, WK, WKB, OWK. */
static int best_player(const C07Player *players, size_t count,
                       const unsigned char *picked, int mode, int accuracy)
{
    size_t i;
    int best = -1, rating, previous;
    for (i = 0; i < count; ++i) {
        if (picked[i] ||
            (mode == 1 && players[i].role != ROLE_BAT) ||
            (mode == 2 && players[i].role != ROLE_ALL) ||
            (mode == 3 && players[i].role != ROLE_ALL && players[i].role != ROLE_ASP) ||
            (mode == 4 && players[i].role != ROLE_BAT && players[i].role != ROLE_ALL) ||
            (mode == 5 && players[i].role != ROLE_BAT && players[i].role != ROLE_OBT &&
             players[i].role != ROLE_ALL && players[i].role != ROLE_ASP &&
             players[i].role != ROLE_WK && players[i].role != ROLE_WKB &&
             players[i].role != ROLE_OWK))
            continue;
        rating = accuracy ? players[i].accuracy : players[i].skill;
        previous = best < 0 ? -1 : accuracy ? players[best].accuracy : players[best].skill;
        if (rating > previous ||
            (mode == 4 && rating == previous && best >= 0 &&
             players[i].role == ROLE_ALL && players[best].role == ROLE_BAT))
            best = (int)i;
    }
    return best;
}

static int two_allrounders(int pitch)
{
    return pitch == 1 || pitch == 6 || pitch == 7 || pitch == 15 ||
           pitch == 4 || pitch == 8 || pitch == 9 || pitch == 16;
}

static int test_extra_batter(int pitch)
{
    return pitch != 4 && pitch != 8 && pitch != 9 && pitch != 16;
}

static void reserve_bowling(C07Selection *selection, uint16_t id)
{
    size_t i;
    for (i = 0; i < C07_BOWLERS; ++i)
        if (selection->bowling[i] == id)
            return;
    for (i = 2; i < C07_BOWLERS; ++i) {
        if (!selection->bowling[i]) {
            selection->bowling[i] = id;
            return;
        }
    }
}

int c07_finish(const C07Player *players, size_t count, int pitch, int is_test,
               const uint16_t *bowling_pool, size_t bowling_count,
               C07Selection *selection)
{
    C07Selection result;
    unsigned char picked[C07_MAX_PLAYERS] = { 0 };
    unsigned char remaining[C07_MAX_PLAYERS] = { 0 };
    size_t i, j, selected = 0, ordered = 0, slot = 0, quota;
    size_t batting_allrounders = 0, remaining_allrounders;
    int keeper, index, best;
    if (!selection || !valid_players(players, count) ||
        bowling_count > C07_BOWLERS || (bowling_count && !bowling_pool))
        return 0;
    result = *selection;
    keeper = index_of(players, count, result.keeper);
    if (keeper < 0)
        return 0;
    for (i = 2; i < C07_XI; ++i)
        if (result.xi[i])
            return 0;
    for (i = 0; i < 2; ++i) {
        if (!result.xi[i])
            continue;
        index = index_of(players, count, result.xi[i]);
        if (index < 0 || picked[index])
            return 0;
        picked[index] = 1;
        ++selected;
    }
    for (i = 0; i < bowling_count; ++i) {
        index = index_of(players, count, bowling_pool[i]);
        if (index < 0 || picked[index] || index == keeper)
            return 0;
        picked[index] = 1;
        ++selected;
    }
    for (i = 0; i < C07_BOWLERS; ++i) {
        if (!result.bowling[i])
            continue;
        index = index_of(players, count, result.bowling[i]);
        if (index < 0 || index == keeper || !picked[index])
            return 0;
        for (j = 0; j < i; ++j)
            if (result.bowling[j] == result.bowling[i])
                return 0;
    }
    if (!picked[keeper]) {
        picked[keeper] = 1;
        ++selected;
    }
    if (selected > C07_XI)
        return 0;
    for (i = 0; i < 3 && selected < C07_XI; ++i) {
        best = best_player(players, count, picked, 5, 0);
        if (best < 0)
            break;
        picked[best] = 1;
        ++selected;
        if (players[best].role == ROLE_ALL || players[best].role == ROLE_ASP)
            ++batting_allrounders;
    }
    quota = is_test ? (size_t)test_extra_batter(pitch) : two_allrounders(pitch) ? 2u : 1u;
    remaining_allrounders = quota > batting_allrounders ? quota - batting_allrounders : 0;
    for (i = 0; i < quota && selected < C07_XI; ++i) {
        /* Only the new batting trio credits the limited ALL/ASP requirement.
         * Retain the number of additional picks: freed places accept any role.
         * First additional pick uses Accuracy; the second uses batting Skill.
         * Tests keep their original BAT/ALL flexible pick. */
        int mode = is_test ? 4 : i < remaining_allrounders ? 3 : 0;
        best = best_player(players, count, picked, mode, !is_test && i == 0);
        if (best < 0)
            break;
        picked[best] = 1;
        ++selected;
        if (!is_test)
            reserve_bowling(&result, players[best].id);
    }
    while (selected < C07_XI) {
        best = best_player(players, count, picked, 0, 0);
        if (best < 0)
            return 0;
        picked[best] = 1;
        ++selected;
    }
    /* Keep the two fixed slots; sort all other selected players by Skill. */
    memcpy(remaining, picked, sizeof(remaining));
    for (i = 0; i < 2; ++i) {
        if (result.xi[i]) {
            index = index_of(players, count, result.xi[i]);
            remaining[index] = 0;
            ++ordered;
        }
    }
    while (ordered < C07_XI) {
        best = -1;
        for (i = 0; i < count; ++i)
            if (remaining[i] && (best < 0 || players[i].skill > players[best].skill))
                best = (int)i;
        if (best < 0)
            return 0;
        while (slot < C07_XI && result.xi[slot])
            ++slot;
        if (slot == C07_XI)
            return 0;
        result.xi[slot] = players[best].id;
        remaining[best] = 0;
        ++ordered;
    }
    memset(result.bench, 0, sizeof(result.bench));
    result.bench_count = 0;
    while ((best = best_player(players, count, picked, 0, 0)) >= 0) {
        result.bench[result.bench_count++] = players[best].id;
        picked[best] = 1;
    }
    *selection = result;
    return 1;
}
