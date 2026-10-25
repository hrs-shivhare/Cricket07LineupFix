/* Regression fixture from the read-only live South Africa snapshot of 2026-10-23.
 * Uses the portable core with the native specialist pool identified for the
 * recorded pitch branch. No game process or game files are changed here. */
#include "../src/selection.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static const C07Player squad[] = {
    {1969,2,56,34,0,0,0}, /* G.Smith */
    {11653,6,49,8,1,0,0}, /* M.Boucher */
    {19535,10,36,52,4,0,0}, /* C.Langeveldt */
    {47,1,52,12,1,0,0}, /* A.de Villiers */
    {13615,2,52,12,0,4,0}, /* B.Dippenaar */
    {13108,1,56,38,0,0,0}, /* H.Gibbs */
    {25003,9,49,54,0,0,0}, /* A.Hall */
    {2390,9,57,56,2,0,0}, /* J.Kallis */
    {11896,9,45,52,0,0,0}, /* J.Kemp */
    {153,1,51,22,0,0,0}, /* L.Bosman */
    {12788,3,34,48,4,0,0}, /* D.Steyn */
    {8944,3,35,52,4,0,0}, /* A.Nel */
    {4941,10,37,56,4,0,0}, /* M.Ntini */
    {154,3,44,48,0,0,0}, /* R.Telemachus */
    {3152,10,48,58,5,0,0}, /* S.Pollock */
    {12388,1,54,45,0,0,0}, /* J.Rudolph */
    {29933,5,47,54,7,0,0}, /* N.Boje */
    {2318,1,50,12,3,0,0}, /* A.Prince */
    {155,3,39,51,4,0,0}, /* J.van der Wath */
    {4266,1,43,3,0,0,0}, /* H.Amla */
};
int main(void) {
    const uint16_t normal_pool[] = {3152,4941,8944,29933};
    const uint16_t expected[] = {13615,1969,2390,13108,12388,47,11653,3152,29933,4941,8944};
    const int pitches[] = {0,5,10};
    C07Selection result;
    for (size_t i=0;i<sizeof(pitches)/sizeof(pitches[0]);++i) {
        assert(c07_begin(squad,20,&result));
        memcpy(result.bowling,normal_pool,sizeof(normal_pool));
        assert(c07_finish(squad,20,pitches[i],0,normal_pool,4,&result));
        assert(!memcmp(result.xi,expected,sizeof(expected)));
        assert(result.xi[5]==47); /* de Villiers now No.6. */
    }
    puts("Live South Africa fixture passed: de Villiers No.6; Hall replaced on Normal/Dry/Hard limited overs.");
    return 0;
}
