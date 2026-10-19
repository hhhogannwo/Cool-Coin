// Standalone experimental test; compile with c++ -std=c++17 -Isrc.
#include <consensus/hybrid_registry.h>
#include <cassert>
int main() {
    using namespace Consensus::Hybrid;
    ValidatorRegistry reg;
    assert(!reg.Register("", 1000 * COIN));
    assert(!reg.Register("small", 999 * COIN));
    assert(reg.Register("alice", 800000 * COIN));
    assert(!reg.Register("alice", 1200 * COIN));
    assert(reg.Register("bob", 1000 * COIN));
    auto snap = reg.Snapshot();
    assert(snap.size() == 2 && snap[0].validator_id == "alice");
    assert(snap[0].effective == 250000 * COIN);
    assert(snap[1].effective == 1000 * COIN);
    assert(!reg.UpdateCommitted("bob", 999 * COIN));
    assert(reg.UpdateCommitted("bob", 260000 * COIN));
    assert(reg.Snapshot()[1].effective == 250000 * COIN);
    assert(reg.Unregister("alice"));
    assert(!reg.Unregister("alice"));
    assert(reg.Snapshot().size() == 1);
}
