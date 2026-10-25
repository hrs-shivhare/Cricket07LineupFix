#include "../src/selection.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void setup(C07Player *players, size_t count)
{
    size_t i;
    memset(players, 0, sizeof(*players) * count);
    for (i = 0; i < count; ++i) {
        players[i].id = (uint16_t)(101 + i);
        players[i].skill = (uint8_t)(30 - i);
        players[i].accuracy = (uint8_t)i;
    }
}

static int contains(const uint16_t *ids, size_t count, uint16_t id)
{
    size_t i;
    for (i = 0; i < count; ++i)
        if (ids[i] == id)
            return 1;
    return 0;
}

static void valid_result(const C07Player *players, size_t count, const C07Selection *result)
{
    size_t i, j, total = 0;
    assert(result->bench_count == count - C07_XI);
    assert(contains(result->xi, C07_XI, result->keeper));
    for (i = 0; i < C07_XI; ++i) {
        assert(result->xi[i]);
        for (j = 0; j < i; ++j)
            assert(result->xi[i] != result->xi[j]);
        assert(!contains(result->bench, result->bench_count, result->xi[i]));
    }
    for (i = 0; i < result->bench_count; ++i)
        for (j = 0; j < i; ++j)
            assert(result->bench[i] != result->bench[j]);
    for (i = 0; i < count; ++i) {
        total += contains(result->xi, C07_XI, players[i].id);
        total += contains(result->bench, result->bench_count, players[i].id);
    }
    assert(total == count);
    for (i = 0; i < C07_BOWLERS; ++i) {
        if (result->bowling[i]) {
            assert(result->bowling[i] != result->keeper);
            assert(contains(result->xi, C07_XI, result->bowling[i]));
        }
    }
}

static void test_keeper_tiers(void)
{
    C07Player p[12];
    C07Selection s;
    setup(p, 12);
    p[0].role = 6; p[0].skill = 40;
    p[1].role = 7; p[1].skill = 50;
    p[2].role = 8; p[2].skill = 50;
    p[3].primary_field = 1; p[3].skill = 63;
    assert(c07_begin(p, 12, &s));
    assert(s.keeper == p[2].id); /* Highest Skill, last equal keeper. */
    assert(s.xi[0] == p[2].id);
    p[0].role = p[1].role = p[2].role = 0;
    assert(c07_begin(p, 12, &s)); assert(s.keeper == p[3].id);
    p[3].primary_field = 0; p[4].secondary_field = 1;
    assert(c07_begin(p, 12, &s)); assert(s.keeper == p[4].id);
    p[4].secondary_field = 0; p[5].legacy_keeper = 4;
    assert(c07_begin(p, 12, &s)); assert(s.keeper == p[5].id);
    p[5].legacy_keeper = 0;
    assert(c07_begin(p, 12, &s)); assert(s.keeper == p[3].id);
}

static void test_openers_and_unreserved_keeper(void)
{
    C07Player p[14];
    C07Selection s;
    setup(p, 14);
    p[0].role = 8; p[0].skill = 20;
    p[1].role = 2; p[1].skill = 50;
    p[2].role = 2; p[2].skill = 60;
    assert(c07_begin(p, 14, &s));
    assert(s.xi[0] == p[0].id && s.xi[1] == p[2].id);
    assert(c07_finish(p, 14, 0, 0, NULL, 0, &s));
    valid_result(p, 14, &s);
    p[1].role = p[2].role = 0;
    assert(c07_begin(p, 14, &s));
    assert(s.xi[0] == p[0].id && !s.xi[1]);
    assert(c07_finish(p, 14, 0, 0, NULL, 0, &s));
    assert(s.xi[0] == p[0].id && s.xi[1] == p[2].id);
    valid_result(p, 14, &s);
    p[1].role = p[2].role = 2;
    p[0].role = 6; p[0].skill = 63;
    assert(c07_begin(p, 14, &s));
    assert(s.xi[0] == p[1].id && s.xi[1] == p[2].id);
    assert(c07_finish(p, 14, 0, 0, NULL, 0, &s));
    assert(s.xi[2] == p[0].id); /* No Skill-80 keeper promotion. */
    valid_result(p, 14, &s);
    p[1].role = 0;
    assert(c07_begin(p, 14, &s));
    assert(!s.xi[0] && s.xi[1] == p[2].id);
    assert(c07_finish(p, 14, 0, 0, NULL, 0, &s));
    assert(s.xi[0] == p[0].id); /* Fallback is Skill-ranked, not keeper rule. */
    p[2].role = 0;
    assert(c07_begin(p, 14, &s)); assert(!s.xi[0] && !s.xi[1]);
    assert(c07_finish(p, 14, 0, 0, NULL, 0, &s));
    assert(s.xi[0] == p[0].id && s.xi[1] == p[2].id);
    valid_result(p, 14, &s);
}

static void setup_allrounders(C07Player *p, uint16_t *pool)
{
    size_t i;
    setup(p, 20);
    p[0].role = 6; p[0].skill = 10;
    p[1].role = p[2].role = 2;
    for (i = 3; i < 6; ++i) { p[i].role = 10; pool[i - 3] = p[i].id; }
    for (i = 6; i < 9; ++i) { p[i].role = 1; p[i].skill = (uint8_t)(68 - i); }
    p[9].role = 9; p[9].skill = 5; p[9].accuracy = 63;
    p[10].role = 5; p[10].skill = 60; p[10].accuracy = 20;
    p[11].role = 9; p[11].skill = 50; p[11].accuracy = 50;
    p[12].role = 9; p[12].skill = 40; p[12].accuracy = 40;
    p[13].role = 0; p[13].skill = 63;
}

static void test_limited_allrounders_and_pitch_groups(void)
{
    const int pitches[] = {0, 1, 4, 6, 7, 8, 9, 15, 16, 2, 3, 5};
    C07Player p[20];
    C07Selection s;
    uint16_t pool[3];
    size_t n;
    for (n = 0; n < sizeof(pitches) / sizeof(pitches[0]); ++n) {
        int two = pitches[n] == 1 || pitches[n] == 4 || pitches[n] == 6 ||
                  pitches[n] == 7 || pitches[n] == 8 || pitches[n] == 9 ||
                  pitches[n] == 15 || pitches[n] == 16;
        setup_allrounders(p, pool);
        assert(c07_begin(p, 20, &s));
        s.bowling[0] = pool[0]; s.bowling[1] = pool[1]; s.bowling[2] = pool[2];
        assert(c07_finish(p, 20, pitches[n], 0, pool, 3, &s));
        assert(contains(s.xi, C07_XI, p[9].id)); /* Accuracy winner, weak batting. */
        assert(s.bowling[3] == p[9].id);
        if (two) {
            assert(contains(s.xi, C07_XI, p[10].id)); /* ASP wins second by Skill. */
            assert(s.bowling[4] == p[10].id);
            assert(!contains(s.xi, C07_XI, p[13].id));
        } else {
            assert(contains(s.xi, C07_XI, p[13].id)); /* Generic shortage fill. */
            assert(!contains(s.xi, C07_XI, p[10].id));
        }
        valid_result(p, 20, &s);
    }
}

static void test_asp_dedup_and_shortages(void)
{
    C07Player p[20];
    C07Selection s;
    uint16_t pool[3];
    size_t i;
    setup_allrounders(p, pool);
    p[3].role = 5; p[3].accuracy = 63;
    assert(c07_begin(p, 20, &s));
    assert(c07_finish(p, 20, 1, 0, pool, 3, &s));
    assert(contains(s.xi, C07_XI, p[3].id));
    assert(contains(s.xi, C07_XI, p[9].id));
    assert(contains(s.xi, C07_XI, p[10].id));
    valid_result(p, 20, &s);
    for (i = 0; i < 20; ++i) p[i].role = 0;
    assert(c07_begin(p, 20, &s));
    assert(c07_finish(p, 20, 1, 0, NULL, 0, &s));
    valid_result(p, 20, &s);
    for (i = 1; i < C07_XI; ++i) {
        size_t j; int previous = -1, current = -1;
        for (j = 0; j < 20; ++j) {
            if (p[j].id == s.xi[i - 1]) previous = p[j].skill;
            if (p[j].id == s.xi[i]) current = p[j].skill;
        }
        assert(previous >= current);
    }
}

static void test_test_flexible_and_no_asp_pool(void)
{
    C07Player p[20];
    C07Selection s;
    uint16_t pool[4];
    setup_allrounders(p, pool);
    pool[3] = p[14].id;
    p[9].skill = 40; p[10].skill = 60; p[11].skill = 40;
    p[6].skill = 63; p[7].skill = 62; p[8].skill = 61;
    p[12].role = 1; p[12].skill = 40;
    p[13].skill = 1;
    assert(c07_begin(p, 20, &s));
    assert(c07_finish(p, 20, 1, 1, pool, 4, &s));
    assert(contains(s.xi, C07_XI, p[9].id)); /* ALL wins tied flexible slot. */
    assert(!contains(s.xi, C07_XI, p[10].id)); /* ASP outside batting trio does not enter flex pool. */
    assert(!contains(s.xi, C07_XI, p[12].id));
    assert(!contains(s.bowling, C07_BOWLERS, p[9].id));
    valid_result(p, 20, &s);
    /* Green Test: five native bowlers fill the XI; no flexible slot. */
    {
        uint16_t green_pool[5] = { p[3].id, p[4].id, p[5].id, p[14].id, p[15].id };
        assert(c07_begin(p, 20, &s));
        assert(c07_finish(p, 20, 4, 1, green_pool, 5, &s));
        assert(!contains(s.xi, C07_XI, p[9].id));
        valid_result(p, 20, &s);
    }
}

static void test_batting_pool_both_formats(void)
{
    const unsigned roles[] = {1, 2, 5, 6, 7, 8, 9};
    C07Player p[20];
    C07Selection s;
    uint16_t pool[5];
    size_t role, i;
    int is_test;
    for (role = 0; role < sizeof(roles) / sizeof(roles[0]); ++role) {
        for (is_test = 0; is_test <= 1; ++is_test) {
            setup_allrounders(p, pool);
            pool[3] = p[14].id; pool[4] = p[15].id;
            for (i = 0; i < 20; ++i) p[i].skill = 1;
            p[0].skill = 63; p[1].skill = 62; p[2].skill = 61;
            p[6].role = (uint8_t)roles[role]; p[6].skill = 60;
            p[7].skill = 59; p[8].skill = 58;
            /* Higher batting ratings cannot admit excluded specialist roles
             * to the three batting picks, when all other seats are occupied. */
            p[13].role = 0; p[13].skill = 63;
            p[16].role = 3; p[16].skill = 63;
            p[17].role = 4; p[17].skill = 63;
            p[18].role = 10; p[18].skill = 63;
            assert(c07_begin(p, 20, &s));
            assert(s.keeper == p[0].id);
            assert(c07_finish(p, 20, 8, is_test, pool, 5, &s));
            for (i = 6; i < 9; ++i) assert(contains(s.xi, C07_XI, p[i].id));
            assert(!contains(s.xi, C07_XI, p[13].id));
            for (i = 16; i < 19; ++i) assert(!contains(s.xi, C07_XI, p[i].id));
            valid_result(p, 20, &s);
        }
    }
}

static void test_batting_allrounders_credit_limited_quota(void)
{
    C07Player p[20];
    C07Selection s;
    uint16_t pool[3];
    unsigned a, two, i;
    for (a = 0; a <= 3; ++a) for (two = 0; two <= 1; ++two) {
        setup_allrounders(p, pool);
        for (i = 0; i < 20; ++i) { p[i].skill = 1; p[i].accuracy = 1; }
        p[0].skill = 20; p[1].skill = 30; p[2].skill = 29;
        for (i = 6; i < 9; ++i) {
            p[i].skill = (uint8_t)(66 - i);
            p[i].role = i - 6 < a ? (i == 7 ? 5 : 9) : 1;
        }
        p[9].role = 9; p[9].skill = 1; p[9].accuracy = 50;
        p[10].role = 5; p[10].skill = 2; p[10].accuracy = 40;
        p[11].role = 9; p[11].skill = 3; p[11].accuracy = 30;
        p[12].role = 3; p[12].skill = 4; p[12].accuracy = 63;
        p[13].role = 0; p[13].skill = 63;
        assert(c07_begin(p, 20, &s));
        memcpy(s.bowling, pool, sizeof(pool));
        assert(c07_finish(p, 20, two ? 8 : 0, 0, pool, 3, &s));
        for (i = 6; i < 9; ++i) assert(contains(s.xi, C07_XI, p[i].id));
        if (!a) {
            assert(contains(s.xi, C07_XI, p[9].id));
            assert(contains(s.xi, C07_XI, p[two ? 11 : 13].id));
            assert(!contains(s.xi, C07_XI, p[12].id));
        } else if (a == 1 && two) {
            assert(contains(s.xi, C07_XI, p[9].id));
            assert(contains(s.xi, C07_XI, p[13].id));
            assert(!contains(s.xi, C07_XI, p[12].id));
        } else {
            assert(contains(s.xi, C07_XI, p[12].id));
            assert(contains(s.xi, C07_XI, p[13].id));
            assert(!contains(s.xi, C07_XI, p[9].id));
        }
        valid_result(p, 20, &s);
    }
}

static void test_accuracy_winner_in_batting_trio(void)
{
    C07Player p[20];
    C07Selection s;
    uint16_t pool[4];
    unsigned two, asp, scenario;
    for (two = 0; two <= 1; ++two) for (asp = 0; asp <= 1; ++asp)
    for (scenario = 0; scenario < 4; ++scenario) {
        int credit = scenario < 2;
        setup_allrounders(p, pool);
        pool[3] = p[15].id;
        p[6].role = (uint8_t)(asp ? 5 : 9);
        p[6].accuracy = scenario == 0 ? 63 : scenario == 2 ? 61 : 62;
        p[9].accuracy = 62;
        p[10].skill = 15;
        p[12].role = 3; p[12].skill = 1; p[12].accuracy = 63;
        p[14].role = 1; p[14].skill = 58;
        if (scenario == 3) {
            /* Earlier equal-Accuracy ALL is not in the batting trio. */
            p[6].skill = 1;
            p[9].skill = 62;
        }
        assert(c07_begin(p, 20, &s));
        memcpy(s.bowling, pool, (two ? 3u : 4u) * sizeof(*pool));
        assert(c07_finish(p, 20, two ? 8 : 0, 0, pool, two ? 3 : 4, &s));
        if (credit) {
            assert(contains(s.xi, C07_XI, p[6].id));
            assert(contains(s.xi, C07_XI, p[13].id));
            assert(!contains(s.xi, C07_XI, p[12].id));
            assert(!contains(s.bowling, C07_BOWLERS, p[13].id));
            if (two) assert(contains(s.xi, C07_XI, p[14].id));
        } else if (two) {
            assert(contains(s.xi, C07_XI, p[scenario == 3 ? 6 : 9].id));
            assert(contains(s.xi, C07_XI, p[13].id));
            assert(!contains(s.xi, C07_XI, p[12].id));
        } else {
            assert(contains(s.xi, C07_XI, p[12].id));
            assert(!contains(s.xi, C07_XI, p[13].id));
        }
        valid_result(p, 20, &s);
    }
}

static void test_extra_opener_order_both_formats(void)
{
    C07Player p[20];
    C07Selection s;
    uint16_t pool[3];
    int is_test;
    for (is_test = 0; is_test <= 1; ++is_test) {
        setup_allrounders(p, pool);
        p[0].skill = 63; p[1].skill = 62; p[2].skill = 61;
        /* Eleven-player squad forces every player into the XI. */
        p[9].role = 2; p[9].skill = 5;
        p[10].role = 8; p[10].skill = 4;
        assert(c07_begin(p, 11, &s));
        assert(c07_finish(p, 11, 0, is_test, pool, 3, &s));
        assert(s.xi[0] == p[2].id && s.xi[1] == p[1].id);
        assert(s.xi[2] == p[9].id && s.xi[3] == p[10].id);
        assert(s.xi[4] == p[0].id); /* Higher-Skill keeper follows extra openers. */
        valid_result(p, 11, &s);
    }
}

static uint32_t random_state = 0xC07;

static unsigned next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static void test_many_valid_squads(void)
{
    C07Player p[C07_MAX_PLAYERS];
    C07Selection s;
    uint16_t pool[5], fixed[2];
    size_t trial, i, pool_count, wanted, count;
    for (trial = 0; trial < 3000; ++trial) {
        count = C07_XI + next_random() % (C07_MAX_PLAYERS - C07_XI + 1);
        setup(p, count);
        for (i = 0; i < count; ++i) {
            p[i].role = (uint8_t)(next_random() % 11);
            p[i].skill = (uint8_t)(next_random() % 64);
            p[i].accuracy = (uint8_t)(next_random() % 64);
        }
        assert(c07_begin(p, count, &s));
        fixed[0] = s.xi[0]; fixed[1] = s.xi[1];
        wanted = next_random() % 6; pool_count = 0;
        for (i = 0; i < count && pool_count < wanted; ++i)
            if (p[i].id != s.keeper && p[i].id != fixed[0] && p[i].id != fixed[1])
                pool[pool_count++] = p[i].id;
        for (i = 0; i < pool_count; ++i)
            s.bowling[i] = pool[i];
        assert(c07_finish(p, count, (int)(trial % 17), (int)(trial % 2), pool, pool_count, &s));
        valid_result(p, count, &s);
        if (fixed[0]) assert(s.xi[0] == fixed[0]);
        if (fixed[1]) assert(s.xi[1] == fixed[1]);
        for (i = 0; i < pool_count; ++i) {
            assert(contains(s.xi, C07_XI, pool[i]));
            assert(s.bowling[i] == pool[i]);
        }
        {
            int previous = 64, previous_opener = 1;
            size_t j;
            for (i = 0; i < C07_XI; ++i) {
                if (i < 2 && fixed[i]) continue;
                for (j = 0; j < count; ++j)
                    if (p[j].id == s.xi[i]) {
                        int opener = p[j].role == 2 || p[j].role == 8;
                        assert(previous_opener >= opener);
                        if (previous_opener == opener) assert(previous >= p[j].skill);
                        previous = p[j].skill;
                        previous_opener = opener;
                    }
            }
        }
    }
}

static void test_exclusions_and_invalid_inputs(void)
{
    C07Player p[11];
    C07Selection s, saved;
    uint16_t pool[2];
    setup(p, 11);
    p[0].id = 0x1CB7; /* Native hardcoded-ban ID remains eligible. */
    p[0].role = 10;
    assert(c07_begin(p, 11, &s));
    assert(c07_finish(p, 11, 0, 0, NULL, 0, &s));
    assert(contains(s.xi, C07_XI, 0x1CB7)); valid_result(p, 11, &s);
    memset(&s, 0xA5, sizeof(s)); saved = s;
    assert(!c07_begin(p, 10, &s)); assert(!memcmp(&s, &saved, sizeof(s)));
    p[0].role = 11;
    assert(!c07_begin(p, 11, &s)); assert(!memcmp(&s, &saved, sizeof(s)));
    p[0].role = 10; p[0].skill = 64;
    assert(!c07_begin(p, 11, &s));
    p[0].skill = 30; p[1].id = p[0].id;
    assert(!c07_begin(p, 11, &s));
    p[1].id = 102;
    assert(c07_begin(p, 11, &s)); saved = s;
    pool[0] = pool[1] = p[2].id;
    assert(!c07_finish(p, 11, 0, 0, pool, 2, &s));
    assert(!memcmp(&s, &saved, sizeof(s)));
    assert(!c07_finish(p, 11, 0, 0, NULL, 2, &s));
    assert(!memcmp(&s, &saved, sizeof(s)));
}

int main(void)
{
    test_keeper_tiers();
    test_openers_and_unreserved_keeper();
    test_limited_allrounders_and_pitch_groups();
    test_asp_dedup_and_shortages();
    test_test_flexible_and_no_asp_pool();
    test_batting_pool_both_formats();
    test_batting_allrounders_credit_limited_quota();
    test_accuracy_winner_in_batting_trio();
    test_extra_opener_order_both_formats();
    test_exclusions_and_invalid_inputs();
    test_many_valid_squads();
    puts("Cricket07Lineup core tests passed.");
    return 0;
}
