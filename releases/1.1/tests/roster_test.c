/* Replays the installed roster through the portable core. Bowling selection
 * here is a static reference model of the recovered native role/Accuracy
 * stages; this test does not execute Cricket07.exe or its native helpers. */
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

static int dusty(int pitch)
{
    return pitch == 1 || pitch == 6 || pitch == 7 || pitch == 15;
}

static int green(int pitch)
{
    return pitch == 4 || pitch == 8 || pitch == 9 || pitch == 16;
}

static int role_matches(unsigned role, int stage)
{
    if (stage == 0) return role == 10;
    if (stage == 1) return role == 3;
    if (stage == 2) return role == 4 || role == 5;
    return role == 3 || role == 4 || role == 5 || role == 10;
}

static int best_bowler(const C07Player *players, size_t count,
                       const C07Selection *selection, const uint16_t *pool,
                       size_t pool_count, int stage)
{
    size_t i;
    int best = -1;
    for (i = 0; i < count; ++i) {
        if (players[i].id == selection->keeper ||
            players[i].id == selection->xi[0] ||
            players[i].id == selection->xi[1] ||
            includes(pool, pool_count, players[i].id) ||
            !role_matches(players[i].role, stage))
            continue;
        if (best < 0 || players[i].accuracy > players[best].accuracy)
            best = (int)i;
    }
    return best;
}

static size_t bowling_reference(const C07Player *players, size_t count,
                                int pitch, int is_test, C07Selection *selection,
                                uint16_t *pool)
{
    size_t n = 0, i, opening_count, quota, target;
    int best;
    for (i = 0; i < 2; ++i) {
        best = best_bowler(players, count, selection, pool, n, 0);
        if (best < 0) break;
        pool[n++] = players[best].id;
    }
    opening_count = n;
    quota = (dusty(pitch) ? 0u : green(pitch) ? (is_test ? 2u : 0u) : 1u)
            + (2 - opening_count);
    for (i = 0; i < quota; ++i) {
        best = best_bowler(players, count, selection, pool, n, 1);
        if (best < 0) break;
        pool[n++] = players[best].id;
    }
    quota = dusty(pitch) && is_test ? 2u : 1u;
    for (i = 0; i < quota; ++i) {
        best = best_bowler(players, count, selection, pool, n, 2);
        if (best < 0) break;
        pool[n++] = players[best].id;
    }
    target = dusty(pitch) ? (is_test ? 4u : 3u) :
             green(pitch) ? (is_test ? 5u : 3u) : 4u;
    while (n < target) {
        best = best_bowler(players, count, selection, pool, n, 3);
        if (best < 0) break;
        pool[n++] = players[best].id;
    }
    assert(n <= 5);
    /* Opening selections occupy the opening bowling slots; subsequent native
     * selections occupy following slots. finish must retain these entries. */
    for (i = 0; i < n; ++i)
        selection->bowling[i] = pool[i];
    return n;
}

static void check_result(const C07Player *players, size_t count,
                         const C07Selection *before, const C07Selection *after,
                         const uint16_t *pool, size_t pool_count)
{
    size_t i, j, index, total = 0;
    int previous_skill = 64;
    assert(after->keeper == before->keeper);
    assert(includes(after->xi, C07_XI, after->keeper));
    assert(after->bench_count == count - C07_XI);
    for (i = 0; i < C07_XI; ++i) {
        assert(after->xi[i]);
        for (j = 0; j < i; ++j)
            assert(after->xi[i] != after->xi[j]);
        assert(!includes(after->bench, after->bench_count, after->xi[i]));
        index = player_index(players, count, after->xi[i]);
        if (i < 2 && before->xi[i]) {
            assert(before->xi[i] == after->xi[i]);
        } else {
            assert(previous_skill >= players[index].skill);
            previous_skill = players[index].skill;
        }
    }
    index = player_index(players, count, before->keeper);
    if (players[index].role == 8)
        assert(after->xi[0] == before->keeper);
    for (i = 0; i < pool_count; ++i) {
        assert(includes(after->xi, C07_XI, pool[i]));
        assert(after->bowling[i] == before->bowling[i]);
    }
    for (i = 0; i < C07_BOWLERS; ++i) {
        if (!after->bowling[i]) continue;
        assert(after->bowling[i] != after->keeper);
        assert(includes(after->xi, C07_XI, after->bowling[i]));
        for (j = 0; j < i; ++j)
            assert(after->bowling[i] != after->bowling[j]);
    }
    for (i = 0; i < count; ++i) {
        total += includes(after->xi, C07_XI, players[i].id);
        total += includes(after->bench, after->bench_count, players[i].id);
    }
    assert(total == count);
    previous_skill = 64;
    for (i = 0; i < after->bench_count; ++i) {
        index = player_index(players, count, after->bench[i]);
        assert(previous_skill >= players[index].skill);
        previous_skill = players[index].skill;
    }
}

static void print_xi(const RosterPlayer *roster, size_t team, int pitch,
                     const C07Selection *selection)
{
    size_t i;
    int index;
    printf("Reference replay %s limited %s (team %u, pitch %d):\n",
           team == 0 ? "Australia" : "India", pitch == 0 ? "Normal" : "Green",
           (unsigned)team, pitch);
    for (i = 0; i < C07_XI; ++i) {
        index = roster_index(roster, selection->xi[i]);
        assert(index >= 0);
        printf("  %2u. %-20s ID=%5u role=%2u Skill(raw)=%2u Accuracy(raw)=%2u%s\n",
               (unsigned)i + 1, roster[index].name,
               (unsigned)selection->xi[i], (unsigned)roster[index].player.role,
               (unsigned)roster[index].player.skill,
               (unsigned)roster[index].player.accuracy,
               selection->xi[i] == selection->keeper ? " [keeper]" : "");
    }
}

static unsigned check_batting_trio(const C07Player *players, size_t count,
                                   const C07Selection *before, const C07Selection *after,
                                   const uint16_t *pool, size_t pool_count)
{
    uint16_t trio[3] = {0};
    size_t n, i;
    unsigned credits = 0;
    for (n = 0; n < 3; ++n) {
        int best = -1;
        for (i = 0; i < count; ++i) {
            unsigned role = players[i].role;
            if (players[i].id == before->keeper ||
                includes(before->xi, 2, players[i].id) ||
                includes(pool, pool_count, players[i].id) ||
                includes(trio, n, players[i].id) ||
                !(role == 1 || role == 2 || (role >= 5 && role <= 9))) continue;
            if (best < 0 || players[i].skill > players[best].skill) best = (int)i;
        }
        if (best < 0) break;
        trio[n] = players[best].id;
        assert(includes(after->xi, C07_XI, trio[n]));
        if (players[best].role == 5 || players[best].role == 9) ++credits;
    }
    return credits;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] :
        "analysis/batting_20261021/frontend__database__cricket.0.0.12.0.cdf";
    const size_t size = 4u + PLAYER_COUNT * PLAYER_SIZE + TEAM_COUNT * TEAM_SIZE;
    unsigned char *data;
    RosterPlayer *roster;
    FILE *file;
    size_t i, j, t, count, teams = 0, cases = 0, skipped = 0;
    int pitch, is_test, index;
    unsigned credit_examples[4] = {0};
    file = fopen(path, "rb");
    if (!file) { fprintf(stderr, "Cannot read roster: %s\n", path); return 1; }
    data = (unsigned char *)malloc(size);
    roster = (RosterPlayer *)calloc(PLAYER_COUNT, sizeof(*roster));
    assert(data && roster);
    assert(fread(data, 1, size, file) == size);
    assert(fgetc(file) == EOF);
    fclose(file);
    for (i = 0; i < PLAYER_COUNT; ++i) {
        const unsigned char *p = data + 4 + i * PLAYER_SIZE;
        roster[i].player.id = (uint16_t)read16(p);
        roster[i].player.role = p[0x15F] & 15;
        roster[i].player.skill = (uint8_t)((read32(p + 0x6C) >> 6) & 63);
        roster[i].player.accuracy = (uint8_t)((read32(p + 0x74) >> 12) & 63);
        roster[i].player.primary_field = (uint8_t)((read32(p + 0x84) >> 10) & 15);
        roster[i].player.secondary_field = (uint8_t)((read32(p + 0x84) >> 14) & 15);
        roster[i].player.legacy_keeper = (uint8_t)(read32(p + 0x8C) & 63);
        memcpy(roster[i].name, p + 4, 60);
    }
    for (t = 0; t < TEAM_COUNT; ++t) {
        C07Player players[C07_MAX_PLAYERS];
        uint16_t ids[C07_MAX_PLAYERS];
        const unsigned char *team = data + 4 + PLAYER_COUNT * PLAYER_SIZE + t * TEAM_SIZE;
        count = 0;
        for (i = 0; i < C07_MAX_PLAYERS; ++i) {
            uint16_t id = (uint16_t)read16(team + 2 * i);
            if (!id) continue;
            index = roster_index(roster, id);
            if (index < 0 || roster[index].player.role > 10 || includes(ids, count, id)) {
                ++skipped;
                continue;
            }
            ids[count] = id;
            players[count++] = roster[index].player;
        }
        if (count < C07_XI) continue;
        ++teams;
        for (pitch = 0; pitch <= 16; ++pitch) {
            for (is_test = 0; is_test <= 1; ++is_test) {
                C07Selection selection, before;
                uint16_t pool[C07_BOWLERS] = { 0 };
                size_t pool_count;
                assert(c07_begin(players, count, &selection));
                pool_count = bowling_reference(players, count, pitch, is_test, &selection, pool);
                before = selection;
                assert(c07_finish(players, count, pitch, is_test, pool, pool_count, &selection));
                check_result(players, count, &before, &selection, pool, pool_count);
                {
                    unsigned credits = check_batting_trio(players, count, &before, &selection,
                                                         pool, pool_count);
                    if (!is_test && pitch == 8 && !credit_examples[credits]) {
                        printf("Batting trio credit example: team=%u pitch=%d credits=%u\n",
                               (unsigned)t, pitch, credits);
                        credit_examples[credits] = 1;
                    }
                }
                for (j = 0; j < pool_count; ++j)
                    assert(includes(selection.xi, C07_XI, pool[j]));
                if ((t == 0 || t == 13) && !is_test && (pitch == 0 || pitch == 8))
                    print_xi(roster, t, pitch, &selection);
                ++cases;
            }
        }
    }
    printf("Roster reference replay passed: %u teams, %u cases (17 pitches x 2 formats), "
           "%u invalid/duplicate squad entries skipped.\n",
           (unsigned)teams, (unsigned)cases, (unsigned)skipped);
    puts("Evidence scope: actual roster data + portable core + modeled native bowling stages; "
         "not execution of native game code.");
    free(roster);
    free(data);
    return 0;
}
