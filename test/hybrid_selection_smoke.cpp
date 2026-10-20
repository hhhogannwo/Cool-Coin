// Experimental weighted selector test, not consensus-grade randomness.
#include <consensus/hybrid_selection.h>
#include <cassert>
int main() {
    using namespace Consensus::Hybrid;
    ValidatorRegistry r;
    assert(r.Register("alice", 800000 * COIN));
    assert(r.Register("bob", 250000 * COIN));
    assert(r.Register("carol", 1000 * COIN));
    auto s = r.Snapshot();
    const uint64_t a = 250000ULL * COIN, b = 250000ULL * COIN;
    assert(SelectByTicket(s, 0).validator_id == "alice");
    assert(SelectByTicket(s, a - 1).validator_id == "alice");
    assert(SelectByTicket(s, a).validator_id == "bob");
    assert(SelectByTicket(s, a + b).validator_id == "carol");
    assert(!SelectByTicket(s, a + b + 1000ULL * COIN).valid);
    assert(!SelectByTicket({}, 0).valid);
    // For each possible integer ticket, validator selection is proportional
    // to the effective stake (not the uncapped holdings).
    assert(SelectByTicket(s, 0).total_weight == 501000ULL * COIN);
}
