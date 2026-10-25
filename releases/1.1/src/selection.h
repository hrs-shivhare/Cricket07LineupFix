#ifndef CRICKET07_LINEUP_SELECTION_H
#define CRICKET07_LINEUP_SELECTION_H

#include <stddef.h>
#include <stdint.h>

#define C07_MAX_PLAYERS 20
#define C07_XI 11
#define C07_BOWLERS 6

typedef struct C07Player {
    uint16_t id;
    uint8_t role; /* Native role code, 0..10; OBW is 10. */
    uint8_t skill;
    uint8_t accuracy;
    uint8_t primary_field;
    uint8_t secondary_field;
    uint8_t legacy_keeper;
} C07Player;

typedef struct C07Selection {
    uint16_t keeper;
    uint16_t xi[C07_XI];
    uint16_t bowling[C07_BOWLERS];
    uint16_t bench[C07_MAX_PLAYERS];
    size_t bench_count;
} C07Selection;

/* Raw skill/accuracy are the game's six-bit values, not displayed percentages.
 * Both functions return 1 on success and 0 on invalid input. Failure leaves
 * the output unchanged. begin fixes only opening positions. Native bowling
 * selection then supplies bowling_pool and the existing bowling-order state;
 * finish selects and orders the rest of the XI. */
int c07_begin(const C07Player *players, size_t count, C07Selection *selection);
int c07_finish(const C07Player *players, size_t count, int pitch, int is_test,
               const uint16_t *bowling_pool, size_t bowling_count,
               C07Selection *selection);

#endif
