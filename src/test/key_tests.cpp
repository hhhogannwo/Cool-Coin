// Copyright (c) 2012-2022 The CoolCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key.h>
#include <consensus/hybrid_auth.h>
#include <consensus/hybrid_admission.h>
#include <consensus/hybrid_utxo.h>
#include <consensus/hybrid_spend_guard.h>
#include <consensus/hybrid_lifecycle.h>
#include <consensus/hybrid_owner_proof.h>
#include <consensus/hybrid_state_undo.h>
#include <consensus/hybrid_replay.h>
#include <consensus/hybrid_state_commitment.h>
#include <consensus/hybrid_height_ledger.h>
#include <consensus/hybrid_height_replay.h>
#include <consensus/hybrid_event_envelope.h>
#include <consensus/hybrid_tx_intent.h>
#include <consensus/hybrid_bond_auth.h>
#include <consensus/hybrid_exit_auth.h>
#include <consensus/hybrid_bond_preflight.h>
#include <consensus/hybrid_tx_auth.h>
#include <consensus/hybrid_withdraw_preflight.h>
#include <consensus/hybrid_subsidy_schedule.h>
#include <consensus/hybrid_reward_math.h>
#include <consensus/hybrid_ledger_rewards.h>
#include <consensus/hybrid_reward_payments.h>
#include <consensus/hybrid_validator_payout.h>
#include <consensus/hybrid_coinbase_payouts.h>

#include <common/system.h>
#include <key_io.h>
#include <streams.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/strencodings.h>
#include <util/string.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

static const std::string strSecret1 = "5HxWvvfubhXpYYpS3tJkw6fq9jE9j18THftkZjHHfmFiWtmAbrj";
static const std::string strSecret2 = "5KC4ejrDjv152FGwP386VD1i2NYc5KkfSMyv1nGy1VGDxGHqVY3";
static const std::string strSecret1C = "Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw";
static const std::string strSecret2C = "L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g";
static const std::string addr1 = "1QFqqMUD55ZV3PJEJZtaKCsQmjLT6JkjvJ";
static const std::string addr2 = "1F5y5E5FMc5YzdJtB9hLaUe43GDxEKXENJ";
static const std::string addr1C = "1NoJrossxPBKfCHuJXT4HadJrXRE9Fxiqs";
static const std::string addr2C = "1CRj2HyM1CXWzHAXLQtiGLyggNT9WQqsDs";

static const std::string strAddressBad = "1HV9Lc3sNHZxwj4Zk6fB38tEmBryq2cBiF";


BOOST_FIXTURE_TEST_SUITE(key_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(key_test1)
{
    CKey key1  = DecodeSecret(strSecret1);
    BOOST_CHECK(key1.IsValid() && !key1.IsCompressed());
    CKey key2  = DecodeSecret(strSecret2);
    BOOST_CHECK(key2.IsValid() && !key2.IsCompressed());
    CKey key1C = DecodeSecret(strSecret1C);
    BOOST_CHECK(key1C.IsValid() && key1C.IsCompressed());
    CKey key2C = DecodeSecret(strSecret2C);
    BOOST_CHECK(key2C.IsValid() && key2C.IsCompressed());
    CKey bad_key = DecodeSecret(strAddressBad);
    BOOST_CHECK(!bad_key.IsValid());

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(DecodeDestination(addr1)  == CTxDestination(PKHash(pubkey1)));
    BOOST_CHECK(DecodeDestination(addr2)  == CTxDestination(PKHash(pubkey2)));
    BOOST_CHECK(DecodeDestination(addr1C) == CTxDestination(PKHash(pubkey1C)));
    BOOST_CHECK(DecodeDestination(addr2C) == CTxDestination(PKHash(pubkey2C)));

    for (int n=0; n<16; n++)
    {
        std::string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg);

        // normal signatures

        std::vector<unsigned char> sign1, sign2, sign1C, sign2C;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

        std::vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

    // test deterministic signing

    std::vector<unsigned char> detsig, detsigc;
    std::string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg);
    BOOST_CHECK(key1.Sign(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("304402205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d022014ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.Sign(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("3044022052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd5022061d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c5dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(detsigc == ParseHex("205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c52d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(detsigc == ParseHex("2052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
}

BOOST_AUTO_TEST_CASE(key_signature_tests)
{
    // When entropy is specified, we should see at least one high R signature within 20 signatures
    CKey key = DecodeSecret(strSecret1);
    std::string msg = "A message to be signed";
    uint256 msg_hash = Hash(msg);
    std::vector<unsigned char> sig;
    bool found = false;

    for (int i = 1; i <=20; ++i) {
        sig.clear();
        BOOST_CHECK(key.Sign(msg_hash, sig, false, i));
        found = sig[3] == 0x21 && sig[4] == 0x00;
        if (found) {
            break;
        }
    }
    BOOST_CHECK(found);

    // When entropy is not specified, we should always see low R signatures that are less than or equal to 70 bytes in 256 tries
    // The low R signatures should always have the value of their "length of R" byte less than or equal to 32
    // We should see at least one signature that is less than 70 bytes.
    bool found_small = false;
    bool found_big = false;
    bool bad_sign = false;
    for (int i = 0; i < 256; ++i) {
        sig.clear();
        std::string msg = "A message to be signed" + ToString(i);
        msg_hash = Hash(msg);
        if (!key.Sign(msg_hash, sig)) {
            bad_sign = true;
            break;
        }
        // sig.size() > 70 implies sig[3] > 32, because S is always low.
        // But check both conditions anyway, just in case this implication is broken for some reason
        if (sig[3] > 32 || sig.size() > 70) {
            found_big = true;
            break;
        }
        found_small |= sig.size() < 70;
    }
    BOOST_CHECK(!bad_sign);
    BOOST_CHECK(!found_big);
    BOOST_CHECK(found_small);
}

BOOST_AUTO_TEST_CASE(key_key_negation)
{
    // create a dummy hash for signature comparison
    unsigned char rnd[8];
    std::string str = "CoolCoin key verification\n";
    GetRandBytes(rnd);
    uint256 hash{Hash(str, rnd)};

    // import the static test key
    CKey key = DecodeSecret(strSecret1C);

    // create a signature
    std::vector<unsigned char> vch_sig;
    std::vector<unsigned char> vch_sig_cmp;
    key.Sign(hash, vch_sig);

    // negate the key twice
    BOOST_CHECK(key.GetPubKey().data()[0] == 0x03);
    key.Negate();
    // after the first negation, the signature must be different
    key.Sign(hash, vch_sig_cmp);
    BOOST_CHECK(vch_sig_cmp != vch_sig);
    BOOST_CHECK(key.GetPubKey().data()[0] == 0x02);
    key.Negate();
    // after the second negation, we should have the original key and thus the
    // same signature
    key.Sign(hash, vch_sig_cmp);
    BOOST_CHECK(vch_sig_cmp == vch_sig);
    BOOST_CHECK(key.GetPubKey().data()[0] == 0x03);
}

static CPubKey UnserializePubkey(const std::vector<uint8_t>& data)
{
    DataStream stream{};
    stream << data;
    CPubKey pubkey;
    stream >> pubkey;
    return pubkey;
}

static unsigned int GetLen(unsigned char chHeader)
{
    if (chHeader == 2 || chHeader == 3)
        return CPubKey::COMPRESSED_SIZE;
    if (chHeader == 4 || chHeader == 6 || chHeader == 7)
        return CPubKey::SIZE;
    return 0;
}

static void CmpSerializationPubkey(const CPubKey& pubkey)
{
    DataStream stream{};
    stream << pubkey;
    CPubKey pubkey2;
    stream >> pubkey2;
    BOOST_CHECK(pubkey == pubkey2);
}

BOOST_AUTO_TEST_CASE(pubkey_unserialize)
{
    for (uint8_t i = 2; i <= 7; ++i) {
        CPubKey key = UnserializePubkey({0x02});
        BOOST_CHECK(!key.IsValid());
        CmpSerializationPubkey(key);
        key = UnserializePubkey(std::vector<uint8_t>(GetLen(i), i));
        CmpSerializationPubkey(key);
        if (i == 5) {
            BOOST_CHECK(!key.IsValid());
        } else {
            BOOST_CHECK(key.IsValid());
        }
    }
}

BOOST_AUTO_TEST_CASE(bip340_test_vectors)
{
    static const std::vector<std::pair<std::array<std::string, 3>, bool>> VECTORS = {
        {{"F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9", "0000000000000000000000000000000000000000000000000000000000000000", "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0"}, true},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A"}, true},
        {{"DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8", "7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C", "5831AAEED7B44BB74E5EAB94BA9D4294C49BCF2A60728D8B4C200F50DD313C1BAB745879A5AD954A72C45A91C3A51D3C7ADEA98D82F8481E0E1E03674A6F3FB7"}, true},
        {{"25D1DFF95105F5253C4022F628A996AD3A0D95FBF21D468A1B33F8C160D8F517", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "7EB0509757E246F19449885651611CB965ECC1A187DD51B64FDA1EDC9637D5EC97582B9CB13DB3933705B32BA982AF5AF25FD78881EBB32771FC5922EFC66EA3"}, true},
        {{"D69C3509BB99E412E68B0FE8544E72837DFA30746D8BE2AA65975F29D22DC7B9", "4DF3C3F68FCC83B27E9D42C90431A72499F17875C81A599B566C9889B9696703", "00000000000000000000003B78CE563F89A0ED9414F5AA28AD0D96D6795F9C6376AFB1548AF603B3EB45C9F8207DEE1060CB71C04E80F593060B07D28308D7F4"}, true},
        {{"EEFDEA4CDB677750A420FEE807EACF21EB9898AE79B9768766E4FAA04A2D4A34", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6CFF5C3BA86C69EA4B7376F31A9BCB4F74C1976089B2D9963DA2E5543E17776969E89B4C5564D00349106B8497785DD7D1D713A8AE82B32FA79D5F7FC407D39B"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "FFF97BD5755EEEA420453A14355235D382F6472F8568A18B2F057A14602975563CC27944640AC607CD107AE10923D9EF7A73C643E166BE5EBEAFA34B1AC553E2"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "1FA62E331EDBC21C394792D2AB1100A7B432B013DF3F6FF4F99FCB33E0E1515F28890B3EDB6E7189B630448B515CE4F8622A954CFE545735AAEA5134FCCDB2BD"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6CFF5C3BA86C69EA4B7376F31A9BCB4F74C1976089B2D9963DA2E5543E177769961764B3AA9B2FFCB6EF947B6887A226E8D7C93E00C5ED0C1834FF0D0C2E6DA6"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "0000000000000000000000000000000000000000000000000000000000000000123DDA8328AF9C23A94C1FEECFD123BA4FB73476F0D594DCB65C6425BD186051"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "00000000000000000000000000000000000000000000000000000000000000017615FBAF5AE28864013C099742DEADB4DBA87F11AC6754F93780D5A1837CF197"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "4A298DACAE57395A15D0795DDBFD1DCB564DA82B0F269BC70A74F8220429BA1D69E89B4C5564D00349106B8497785DD7D1D713A8AE82B32FA79D5F7FC407D39B"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F69E89B4C5564D00349106B8497785DD7D1D713A8AE82B32FA79D5F7FC407D39B"}, false},
        {{"DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6CFF5C3BA86C69EA4B7376F31A9BCB4F74C1976089B2D9963DA2E5543E177769FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141"}, false},
        {{"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC30", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6CFF5C3BA86C69EA4B7376F31A9BCB4F74C1976089B2D9963DA2E5543E17776969E89B4C5564D00349106B8497785DD7D1D713A8AE82B32FA79D5F7FC407D39B"}, false}
    };

    for (const auto& test : VECTORS) {
        auto pubkey = ParseHex(test.first[0]);
        auto msg = ParseHex(test.first[1]);
        auto sig = ParseHex(test.first[2]);
        BOOST_CHECK_EQUAL(XOnlyPubKey(pubkey).VerifySchnorr(uint256(msg), sig), test.second);
    }

    static const std::vector<std::array<std::string, 5>> SIGN_VECTORS = {
        {{"0000000000000000000000000000000000000000000000000000000000000003", "F9308A019258C31049344F85F89D5229B531C845836F99B08601F113BCE036F9", "0000000000000000000000000000000000000000000000000000000000000000", "0000000000000000000000000000000000000000000000000000000000000000", "E907831F80848D1069A5371B402410364BDF1C5F8307B0084C55F1CE2DCA821525F66A4A85EA8B71E482A74F382D2CE5EBEEE8FDB2172F477DF4900D310536C0"}},
        {{"B7E151628AED2A6ABF7158809CF4F3C762E7160F38B4DA56A784D9045190CFEF", "DFF1D77F2A671C5F36183726DB2341BE58FEAE1DA2DECED843240F7B502BA659", "0000000000000000000000000000000000000000000000000000000000000001", "243F6A8885A308D313198A2E03707344A4093822299F31D0082EFA98EC4E6C89", "6896BD60EEAE296DB48A229FF71DFE071BDE413E6D43F917DC8DCF8C78DE33418906D11AC976ABCCB20B091292BFF4EA897EFCB639EA871CFA95F6DE339E4B0A"}},
        {{"C90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B14E5C9", "DD308AFEC5777E13121FA72B9CC1B7CC0139715309B086C960E18FD969774EB8", "C87AA53824B4D7AE2EB035A2B5BBBCCC080E76CDC6D1692C4B0B62D798E6D906", "7E2D58D8B3BCDF1ABADEC7829054F90DDA9805AAB56C77333024B9D0A508B75C", "5831AAEED7B44BB74E5EAB94BA9D4294C49BCF2A60728D8B4C200F50DD313C1BAB745879A5AD954A72C45A91C3A51D3C7ADEA98D82F8481E0E1E03674A6F3FB7"}},
        {{"0B432B2677937381AEF05BB02A66ECD012773062CF3FA2549E44F58ED2401710", "25D1DFF95105F5253C4022F628A996AD3A0D95FBF21D468A1B33F8C160D8F517", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF", "7EB0509757E246F19449885651611CB965ECC1A187DD51B64FDA1EDC9637D5EC97582B9CB13DB3933705B32BA982AF5AF25FD78881EBB32771FC5922EFC66EA3"}},
    };

    for (const auto& [sec_hex, pub_hex, aux_hex, msg_hex, sig_hex] : SIGN_VECTORS) {
        auto sec = ParseHex(sec_hex);
        auto pub = ParseHex(pub_hex);
        uint256 aux256(ParseHex(aux_hex));
        uint256 msg256(ParseHex(msg_hex));
        auto sig = ParseHex(sig_hex);
        unsigned char sig64[64];

        // Run the untweaked test vectors above, comparing with exact expected signature.
        CKey key;
        key.Set(sec.begin(), sec.end(), true);
        XOnlyPubKey pubkey(key.GetPubKey());
        BOOST_CHECK(std::equal(pubkey.begin(), pubkey.end(), pub.begin(), pub.end()));
        bool ok = key.SignSchnorr(msg256, sig64, nullptr, aux256);
        BOOST_CHECK(ok);
        BOOST_CHECK(std::vector<unsigned char>(sig64, sig64 + 64) == sig);
        // Verify those signatures for good measure.
        BOOST_CHECK(pubkey.VerifySchnorr(msg256, sig64));

        // Do 10 iterations where we sign with a random Merkle root to tweak,
        // and compare against the resulting tweaked keys, with random aux.
        // In iteration i=0 we tweak with empty Merkle tree.
        for (int i = 0; i < 10; ++i) {
            uint256 merkle_root;
            if (i) merkle_root = InsecureRand256();
            auto tweaked = pubkey.CreateTapTweak(i ? &merkle_root : nullptr);
            BOOST_CHECK(tweaked);
            XOnlyPubKey tweaked_key = tweaked->first;
            aux256 = InsecureRand256();
            bool ok = key.SignSchnorr(msg256, sig64, &merkle_root, aux256);
            BOOST_CHECK(ok);
            BOOST_CHECK(tweaked_key.VerifySchnorr(msg256, sig64));
        }
    }
}

BOOST_AUTO_TEST_CASE(key_ellswift)
{
    for (const auto& secret : {strSecret1, strSecret2, strSecret1C, strSecret2C}) {
        CKey key = DecodeSecret(secret);
        BOOST_CHECK(key.IsValid());

        uint256 ent32 = InsecureRand256();
        auto ellswift = key.EllSwiftCreate(AsBytes(Span{ent32}));

        CPubKey decoded_pubkey = ellswift.Decode();
        if (!key.IsCompressed()) {
            // The decoding constructor returns a compressed pubkey. If the
            // original was uncompressed, we must decompress the decoded one
            // to compare.
            decoded_pubkey.Decompress();
        }
        BOOST_CHECK(key.GetPubKey() == decoded_pubkey);
    }
}


// Development-only hybrid PoS authentication checks. Uses ephemeral keys,
// not the legacy Bitcoin-format WIF fixtures or any wallet secrets.
BOOST_AUTO_TEST_CASE(hybrid_validator_authentication)
{
    CKey signer;
    signer.MakeNewKey(true);
    CKey impostor;
    impostor.MakeNewKey(true);
    const CPubKey validator_key = signer.GetPubKey();
    const CPubKey wrong_key = impostor.GetPubKey();

    uint256 genesis;
    genesis.begin()[0] = 0x42;
    uint256 stake_txid;
    stake_txid.begin()[0] = 0x24;
    uint256 nonce;
    nonce.begin()[0] = 0x11;

    const CAmount amount = 1000 * COIN;
    const uint256 digest = Consensus::Hybrid::RegistrationDigest(
        genesis, validator_key, stake_txid, 0, amount, 50000, nonce);

    std::vector<unsigned char> signature;
    BOOST_REQUIRE(signer.Sign(digest, signature));
    BOOST_REQUIRE(!signature.empty());
    BOOST_CHECK(Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, digest, signature));
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(wrong_key, digest, signature));

    // Each changed field must alter the signed digest, invalidating the proof.
    const auto altered_amount = Consensus::Hybrid::RegistrationDigest(
        genesis, validator_key, stake_txid, 0, amount + COIN, 50000, nonce);
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, altered_amount, signature));
    const auto altered_vout = Consensus::Hybrid::RegistrationDigest(
        genesis, validator_key, stake_txid, 1, amount, 50000, nonce);
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, altered_vout, signature));
    uint256 next_nonce = nonce;
    next_nonce.begin()[0] ^= 1;
    const auto altered_nonce = Consensus::Hybrid::RegistrationDigest(
        genesis, validator_key, stake_txid, 0, amount, 50000, next_nonce);
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, altered_nonce, signature));
    const auto altered_chain = Consensus::Hybrid::RegistrationDigest(
        uint256{}, validator_key, stake_txid, 0, amount, 50000, nonce);
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, altered_chain, signature));

    signature[signature.size() / 2] ^= 1;
    BOOST_CHECK(!Consensus::Hybrid::VerifyRegistrationKeyProof(validator_key, digest, signature));
}


BOOST_AUTO_TEST_CASE(hybrid_stake_admission_bookkeeping)
{
    using namespace Consensus::Hybrid;
    StakeAdmissionBook registry;
    uint256 txid;
    txid.begin()[0] = 42;
    uint256 nonce;
    nonce.begin()[0] = 7;
    ValidatedStakeEvidence evidence;
    evidence.outpoint = StakeOutpoint{txid, 0};
    evidence.amount = 1000 * COIN;
    evidence.confirmations = PROPOSED_MIN_CONFIRMATIONS;
    evidence.unspent = true;
    evidence.owner_authorized = true;
    evidence.lock_enforced = true;

    BOOST_CHECK(registry.Register("alice", evidence, nonce));
    BOOST_CHECK(registry.IsCommitted(evidence.outpoint));
    BOOST_CHECK(!registry.Register("alice", evidence, nonce));
    BOOST_CHECK(!registry.Register("bob", evidence, nonce));
    BOOST_CHECK(registry.Unregister("alice"));
    BOOST_CHECK(!registry.IsCommitted(evidence.outpoint));
    // Used nonce remains consumed even after unregistering.
    BOOST_CHECK(!registry.Register("alice", evidence, nonce));
    nonce.begin()[0] = 8;
    BOOST_CHECK(registry.Register("alice", evidence, nonce));

    StakeAdmissionBook invalid;
    uint256 fresh_nonce;
    fresh_nonce.begin()[0] = 10;
    auto probe = evidence;
    probe.unspent = false;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    probe = evidence;
    probe.owner_authorized = false;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    probe = evidence;
    probe.lock_enforced = false;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    probe = evidence;
    probe.confirmations = PROPOSED_MIN_CONFIRMATIONS - 1;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    probe = evidence;
    probe.amount = MINIMUM_STAKE - 1;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    probe = evidence;
    probe.amount = MAX_MONEY + 1;
    BOOST_CHECK(!invalid.Register("bob", probe, fresh_nonce));
    BOOST_CHECK(invalid.Register("bob", evidence, fresh_nonce));
}


BOOST_AUTO_TEST_CASE(hybrid_utxo_inspection)
{
    using namespace Consensus::Hybrid;
    CCoinsView base;
    CCoinsViewCache cache(&base);
    uint256 txid;
    txid.begin()[0] = 51;
    const StakeOutpoint stake{txid, 0};
    const COutPoint outpoint{Txid::FromUint256(txid), 0};

    auto missing = InspectStakeOutpoint(cache, stake, 200);
    BOOST_CHECK(!missing.unspent);
    BOOST_CHECK(!missing.owner_authorized);
    BOOST_CHECK(!missing.lock_enforced);

    CTxOut output;
    output.nValue = 2000 * COIN;
    output.scriptPubKey = CScript() << OP_TRUE;
    cache.AddCoin(outpoint, Coin(std::move(output), 100, false), false);

    auto evidence = InspectStakeOutpoint(cache, stake, 199);
    BOOST_CHECK(evidence.unspent);
    BOOST_CHECK_EQUAL(evidence.amount, 2000 * COIN);
    BOOST_CHECK_EQUAL(evidence.confirmations, 100U);
    BOOST_CHECK(!evidence.owner_authorized);
    BOOST_CHECK(!evidence.lock_enforced);
    StakeAdmissionBook book;
    uint256 nonce;
    nonce.begin()[0] = 4;
    BOOST_CHECK(!book.Register("validator", evidence, nonce));

    auto premature = InspectStakeOutpoint(cache, stake, 99);
    BOOST_CHECK(!premature.unspent);
    auto age_one = InspectStakeOutpoint(cache, stake, 100);
    BOOST_CHECK_EQUAL(age_one.confirmations, 1U);

    // Provably unspendable outputs, empty scripts and insufficient value
    // must never be interpreted as eligible staking deposits.
    uint256 bad_txid = txid;
    bad_txid.begin()[0] = 52;
    COutPoint bad_outpoint{Txid::FromUint256(bad_txid), 0};
    CTxOut bad_output;
    bad_output.nValue = 2000 * COIN;
    bad_output.scriptPubKey = CScript() << OP_RETURN;
    cache.AddCoin(bad_outpoint, Coin(std::move(bad_output), 101, false), false);
    BOOST_CHECK(!InspectStakeOutpoint(cache, StakeOutpoint{bad_txid, 0}, 200).unspent);

    bad_txid.begin()[0] = 53;
    bad_outpoint = COutPoint{Txid::FromUint256(bad_txid), 0};
    CTxOut low_output;
    low_output.nValue = MINIMUM_STAKE - 1;
    low_output.scriptPubKey = CScript() << OP_TRUE;
    cache.AddCoin(bad_outpoint, Coin(std::move(low_output), 101, false), false);
    BOOST_CHECK(!InspectStakeOutpoint(cache, StakeOutpoint{bad_txid, 0}, 200).unspent);

    uint256 minted_txid = txid;
    minted_txid.begin()[0] = 54;
    const COutPoint minted_outpoint{Txid::FromUint256(minted_txid), 0};
    CTxOut minted_output;
    minted_output.nValue = 2000 * COIN;
    minted_output.scriptPubKey = CScript() << OP_TRUE;
    cache.AddCoin(minted_outpoint, Coin(std::move(minted_output), 100, true), false);
    BOOST_CHECK(!InspectStakeOutpoint(cache, StakeOutpoint{minted_txid, 0}, 198).unspent);
    auto mature = InspectStakeOutpoint(cache, StakeOutpoint{minted_txid, 0}, 199);
    BOOST_CHECK(mature.unspent);
    BOOST_CHECK_EQUAL(mature.confirmations, 100U);

    cache.SpendCoin(outpoint);
    auto spent = InspectStakeOutpoint(cache, stake, 200);
    BOOST_CHECK(!spent.unspent);
}


BOOST_AUTO_TEST_CASE(hybrid_stake_spend_conflict)
{
    using namespace Consensus::Hybrid;
    StakeAdmissionBook book;
    uint256 txid;
    txid.begin()[0] = 0x67;
    uint256 nonce;
    nonce.begin()[0] = 0x21;
    ValidatedStakeEvidence proof;
    proof.outpoint = StakeOutpoint{txid, 2};
    proof.amount = 1500 * COIN;
    proof.confirmations = PROPOSED_MIN_CONFIRMATIONS;
    proof.unspent = true;
    proof.owner_authorized = true; // SIMULATED evidence only; not mainnet-safe.
    proof.lock_enforced = true; // SIMULATED evidence only.
    BOOST_REQUIRE(book.Register("validator-A", proof, nonce));

    CMutableTransaction ordinary;
    ordinary.vin.emplace_back(COutPoint{Txid::FromUint256(txid), 3});
    BOOST_CHECK(!SpendsCommittedStake(CTransaction(ordinary), book));
    ordinary.vin.emplace_back(COutPoint{Txid::FromUint256(txid), 2});
    BOOST_CHECK(SpendsCommittedStake(CTransaction(ordinary), book));
    BOOST_CHECK(book.Unregister("validator-A"));
    BOOST_CHECK(!SpendsCommittedStake(CTransaction(ordinary), book));
}


BOOST_AUTO_TEST_CASE(hybrid_bond_lifecycle)
{
    using namespace Consensus::Hybrid;
    BondLifecycle lifecycle;
    uint256 txid;
    txid.begin()[0] = 0x42;
    const StakeOutpoint bond{txid, 1};
    BOOST_CHECK(lifecycle.Bond("A", bond, 100, 100));
    BOOST_CHECK(lifecycle.IsLocked(bond));
    BOOST_CHECK(!lifecycle.IsEligible("A"));
    BOOST_CHECK(!lifecycle.Bond("B", bond, 100, 100));
    BOOST_CHECK(!lifecycle.Withdraw("A", 200));
    BOOST_CHECK(!lifecycle.RequestExit("A", 190, 50));
    BOOST_CHECK(!lifecycle.Bond("A", StakeOutpoint{txid, 2}, 120, 100));
    BOOST_CHECK(!lifecycle.Activate("A", 199));
    BOOST_CHECK(lifecycle.Activate("A", 200));
    BOOST_CHECK(lifecycle.IsEligible("A"));
    BOOST_CHECK(!lifecycle.RequestExit("A", 200, UINT32_MAX));
    BOOST_CHECK(!lifecycle.RequestExit("A", 201, 143));
    BOOST_CHECK(lifecycle.RequestExit("A", 201, PROPOSED_UNBONDING_BLOCKS));
    BOOST_CHECK(!lifecycle.Activate("A", 300));
    BOOST_CHECK(!lifecycle.RequestExit("A", 202, 50));
    BOOST_CHECK(!lifecycle.IsEligible("A"));
    BOOST_CHECK(lifecycle.IsLocked(bond));
    BOOST_CHECK(!lifecycle.Withdraw("A", 344));
    BOOST_CHECK(lifecycle.Withdraw("A", 345));
    BOOST_CHECK(!lifecycle.IsLocked(bond));
    BOOST_CHECK(!lifecycle.Withdraw("A", 300));
    BOOST_CHECK(lifecycle.Bond("B", bond, 300, 100));
    BOOST_CHECK(!lifecycle.Bond("C", StakeOutpoint{txid, 2}, UINT32_MAX, 1));
}


BOOST_AUTO_TEST_CASE(hybrid_p2wpkh_owner_proof)
{
    using namespace Consensus::Hybrid;
    CKey owner;
    owner.MakeNewKey(true);
    const CPubKey pub = owner.GetPubKey();
    const uint160 keyhash = Hash160(pub);
    CTxOut deposit;
    deposit.nValue = 2000 * COIN;
    deposit.scriptPubKey = CScript() << OP_0 <<
        std::vector<unsigned char>(keyhash.begin(), keyhash.end());
    uint256 genesis;
    genesis.begin()[0] = 1;
    uint256 txid;
    txid.begin()[0] = 2;
    uint256 nonce;
    nonce.begin()[0] = 3;
    const uint256 digest = RegistrationDigest(genesis, pub, txid, 0, deposit.nValue, 50000, nonce);
    std::vector<unsigned char> signature;
    BOOST_REQUIRE(owner.Sign(digest, signature));
    BOOST_CHECK(VerifyP2WPKHRegistration(deposit, pub, digest, signature));

    CKey attacker;
    attacker.MakeNewKey(true);
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, attacker.GetPubKey(), digest, signature));
    uint256 wrong_digest = digest;
    wrong_digest.begin()[0] ^= 1;
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub, wrong_digest, signature));
    // The signature binds chain, outpoint, amount, height and nonce.
    uint256 foreign_genesis = genesis;
    foreign_genesis.begin()[0] ^= 1;
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(foreign_genesis, pub, txid, 0, deposit.nValue, 50000, nonce), signature));
    uint256 different_txid = txid;
    different_txid.begin()[0] ^= 1;
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(genesis, pub, different_txid, 0, deposit.nValue, 50000, nonce), signature));
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(genesis, pub, txid, 1, deposit.nValue, 50000, nonce), signature));
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(genesis, pub, txid, 0, deposit.nValue + 1, 50000, nonce), signature));
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(genesis, pub, txid, 0, deposit.nValue, 50001, nonce), signature));
    uint256 another_nonce = nonce;
    another_nonce.begin()[0] ^= 1;
    BOOST_CHECK(!VerifyP2WPKHRegistration(deposit, pub,
        RegistrationDigest(genesis, pub, txid, 0, deposit.nValue, 50000, another_nonce), signature));
    CTxOut wrong_script = deposit;
    wrong_script.scriptPubKey = CScript() << OP_TRUE;
    BOOST_CHECK(!VerifyP2WPKHRegistration(wrong_script, pub, digest, signature));
}


BOOST_AUTO_TEST_CASE(hybrid_stake_block_undo)
{
    using namespace Consensus::Hybrid;
    StakeStateUndoPrototype state;
    uint256 txid; txid.begin()[0] = 44;
    const StakeOutpoint stake{txid, 0};
    uint256 first; first.begin()[0] = 1;
    uint256 second; second.begin()[0] = 2;
    uint256 third; third.begin()[0] = 3;
    StakeBlockUndo undo1, undo2;
    BOOST_REQUIRE(state.ApplyBlock(uint256{}, first,
        {{StakeEventType::BOND, "A", stake, 1000 * COIN}}, undo1));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_CHECK(!state.ApplyBlock(first, second,
        {{StakeEventType::BOND, "B", stake, 1000 * COIN}}, undo2));
    BOOST_CHECK(state.Tip() == first);
    BOOST_CHECK(!state.ApplyBlock(first, second,
        {{StakeEventType::EXIT, "A", stake},
         {StakeEventType::BOND, "B", stake, 1000 * COIN}}, undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_REQUIRE(state.ApplyBlock(first, second,
        {{StakeEventType::EXIT, "A", stake}}, undo2));
    BOOST_CHECK(!state.IsBonded("A"));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_CHECK(!state.DisconnectBlock(undo1));
    BOOST_REQUIRE(state.DisconnectBlock(undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_CHECK(state.Tip() == first);
    BOOST_CHECK(!state.ApplyBlock(first, third,
        {{StakeEventType::EXIT, "A", stake},
         {StakeEventType::WITHDRAW, "A", stake}}, undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_REQUIRE(state.ApplyBlock(first, third,
        {{StakeEventType::EXIT, "A", stake}}, undo2));
    uint256 fourth; fourth.begin()[0] = 4;
    StakeBlockUndo undo3;
    BOOST_REQUIRE(state.ApplyBlock(third, fourth,
        {{StakeEventType::WITHDRAW, "A", stake}}, undo3));
    BOOST_CHECK(!state.IsClaimed(stake));
    BOOST_REQUIRE(state.DisconnectBlock(undo3));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_REQUIRE(state.DisconnectBlock(undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_REQUIRE(state.DisconnectBlock(undo1));
    BOOST_CHECK(!state.IsBonded("A"));
    BOOST_CHECK(!state.IsClaimed(stake));
    BOOST_CHECK(state.Tip().IsNull());
}


BOOST_AUTO_TEST_CASE(hybrid_stake_replay)
{
    using namespace Consensus::Hybrid;
    uint256 txid; txid.begin()[0] = 55;
    const StakeOutpoint stake{txid, 0};
    uint256 a; a.begin()[0] = 11;
    uint256 b; b.begin()[0] = 12;
    uint256 c; c.begin()[0] = 13;
    const std::vector<StakeReplayBlock> history{
        {uint256{}, a, {{StakeEventType::BOND, "A", stake, 1000 * COIN}}},
        {a, b, {{StakeEventType::EXIT, "A", stake}}},
        {b, c, {{StakeEventType::WITHDRAW, "A", stake}}}
    };
    StakeStateUndoPrototype before_restart;
    BOOST_REQUIRE(ReplayStakeBlocks(history, before_restart));
    BOOST_CHECK(before_restart.Tip() == c);
    BOOST_CHECK(!before_restart.IsClaimed(stake));
    StakeStateUndoPrototype reconstructed;
    BOOST_REQUIRE(ReplayStakeBlocks(history, reconstructed));
    BOOST_CHECK(reconstructed.Tip() == before_restart.Tip());
    BOOST_CHECK(!reconstructed.IsClaimed(stake));
    const std::vector<StakeReplayBlock> bad_history{
        {uint256{}, a, {{StakeEventType::BOND, "A", stake, 1000 * COIN}}},
        {c, b, {{StakeEventType::EXIT, "A", stake}}}
    };
    BOOST_CHECK(!ReplayStakeBlocks(bad_history, reconstructed));
    BOOST_CHECK(reconstructed.Tip() == c);
    BOOST_CHECK(!reconstructed.IsClaimed(stake));
}


BOOST_AUTO_TEST_CASE(hybrid_stake_state_commitment)
{
    using namespace Consensus::Hybrid;
    uint256 txid1; txid1.begin()[0] = 1;
    uint256 txid2; txid2.begin()[0] = 2;
    const StakeStateRecord one{StakeOutpoint{txid1, 0}, false};
    const StakeStateRecord two{StakeOutpoint{txid2, 1}, false};
    std::map<std::string, StakeStateRecord> a, b;
    a.emplace("Alice", one); a.emplace("Bob", two);
    b.emplace("Bob", two); b.emplace("Alice", one);
    const uint256 commitment = StakeStateCommitment(a);
    BOOST_CHECK(commitment == StakeStateCommitment(b));
    b["Bob"].exiting = true;
    BOOST_CHECK(commitment != StakeStateCommitment(b));
    b["Bob"].exiting = false;
    b["Bob"].outpoint.vout = 2;
    BOOST_CHECK(commitment != StakeStateCommitment(b));
    BOOST_CHECK(StakeStateCommitment(a) != StakeStateCommitment({}));
}


BOOST_AUTO_TEST_CASE(hybrid_state_commitment_reorg)
{
    using namespace Consensus::Hybrid;
    uint256 txid1; txid1.begin()[0] = 61;
    uint256 txid2; txid2.begin()[0] = 62;
    const StakeOutpoint one{txid1, 0};
    const StakeOutpoint two{txid2, 1};
    uint256 a; a.begin()[0] = 21;
    uint256 b; b.begin()[0] = 22;
    uint256 alternate; alternate.begin()[0] = 23;

    // Construct a canonical expected state for each branch.
    const std::map<std::string, StakeStateRecord> only_alice{
        {"Alice", StakeStateRecord{one, false}}};
    const std::map<std::string, StakeStateRecord> original_branch{
        {"Alice", StakeStateRecord{one, true}}};
    const std::map<std::string, StakeStateRecord> alternate_branch{
        {"Alice", StakeStateRecord{one, false}},
        {"Bob", StakeStateRecord{two, false}}};

    StakeStateUndoPrototype state;
    StakeBlockUndo undo_a, undo_b, undo_alt;
    BOOST_REQUIRE(state.ApplyBlock(uint256{}, a,
        {{StakeEventType::BOND, "Alice", one, 1000 * COIN}}, undo_a));
    BOOST_CHECK(state.Tip() == a);
    BOOST_CHECK(StakeStateCommitment(state.Records()) == StakeStateCommitment(only_alice));

    BOOST_REQUIRE(state.ApplyBlock(a, b,
        {{StakeEventType::EXIT, "Alice", one}}, undo_b));
    BOOST_CHECK(!state.IsBonded("Alice"));
    BOOST_CHECK(StakeStateCommitment(state.Records()) == StakeStateCommitment(original_branch));
    BOOST_CHECK(StakeStateCommitment(original_branch) != StakeStateCommitment(only_alice));

    BOOST_REQUIRE(state.DisconnectBlock(undo_b));
    BOOST_CHECK(state.Tip() == a);
    BOOST_CHECK(state.IsBonded("Alice"));
    BOOST_CHECK(StakeStateCommitment(state.Records()) == StakeStateCommitment(only_alice));

    BOOST_REQUIRE(state.ApplyBlock(a, alternate,
        {{StakeEventType::BOND, "Bob", two, 1000 * COIN}}, undo_alt));
    BOOST_CHECK(state.Tip() == alternate);
    BOOST_CHECK(state.IsBonded("Alice"));
    BOOST_CHECK(state.IsBonded("Bob"));
    BOOST_CHECK(StakeStateCommitment(state.Records()) == StakeStateCommitment(alternate_branch));
    BOOST_CHECK(StakeStateCommitment(alternate_branch) != StakeStateCommitment(original_branch));

    // Reconstruct the competing branch from a fresh empty state.
    StakeStateUndoPrototype rebuilt;
    BOOST_REQUIRE(ReplayStakeBlocks({
        {uint256{}, a, {{StakeEventType::BOND, "Alice", one, 1000 * COIN}}},
        {a, alternate, {{StakeEventType::BOND, "Bob", two, 1000 * COIN}}}
    }, rebuilt));
    BOOST_CHECK(rebuilt.Tip() == state.Tip());
    BOOST_CHECK(StakeStateCommitment(rebuilt.Records()) == StakeStateCommitment(state.Records()));
    BOOST_CHECK(rebuilt.IsBonded("Alice") == state.IsBonded("Alice"));
    BOOST_CHECK(rebuilt.IsBonded("Bob") == state.IsBonded("Bob"));
}


BOOST_AUTO_TEST_CASE(hybrid_height_ledger_withdrawal)
{
    using namespace Consensus::Hybrid;
    HeightStakeLedger ledger;
    uint256 txid; txid.begin()[0] = 81;
    const StakeOutpoint stake{txid, 0};
    uint256 current; current.begin()[0] = 10;
    HeightBlockUndo bond_undo;
    BOOST_REQUIRE(ledger.Apply(uint256{}, current, 100,
        {{StakeEventType::BOND, "A", stake, 1000 * COIN}}, bond_undo));
    BOOST_CHECK(ledger.IsLocked(stake));
    BOOST_CHECK(!ledger.IsBonded("A"));
    BOOST_CHECK(!ledger.Apply(current, txid, 102, {}, bond_undo));
    BOOST_CHECK(ledger.Tip() == current);

    uint256 next;
    for (uint32_t height = 101; height <= 200; ++height) {
        next.SetNull();
        next.begin()[0] = 11;
        next.begin()[1] = static_cast<unsigned char>(height);
        HeightBlockUndo undo;
        BOOST_REQUIRE(ledger.Apply(current, next, height, {}, undo));
        current = next;
        BOOST_CHECK(ledger.IsBonded("A") == (height >= 200));
    }
    uint256 exit_block; exit_block.begin()[0] = 12;
    HeightBlockUndo exit_undo;
    BOOST_REQUIRE(ledger.Apply(current, exit_block, 201,
        {{StakeEventType::EXIT, "A", stake}}, exit_undo));
    BOOST_CHECK(!ledger.IsBonded("A"));
    BOOST_CHECK(ledger.IsLocked(stake));
    BOOST_CHECK(!ledger.CanWithdraw("A", 344));
    BOOST_CHECK(ledger.CanWithdraw("A", 345));
    uint256 invalid; invalid.begin()[0] = 13;
    HeightBlockUndo withdrawal_undo;
    BOOST_CHECK(!ledger.Apply(exit_block, invalid, 202,
        {{StakeEventType::WITHDRAW, "A", stake}}, withdrawal_undo));
    BOOST_CHECK(ledger.Tip() == exit_block);
    BOOST_CHECK(ledger.IsLocked(stake));
    // Invalid attempts must not poison the next valid block.
    BOOST_CHECK(!ledger.Apply(exit_block, invalid, 202,
        {{StakeEventType::BOND, "B", stake, 1000 * COIN}}, withdrawal_undo));

    // Reaching the withdrawal height must be represented by contiguous blocks.
    current = exit_block;
    for (uint32_t height = 202; height <= 344; ++height) {
        next.SetNull();
        next.begin()[0] = 14;
        next.begin()[1] = static_cast<unsigned char>(height);
        HeightBlockUndo undo;
        BOOST_REQUIRE(ledger.Apply(current, next, height, {}, undo));
        current = next;
    }
    uint256 final_block; final_block.begin()[0] = 15;
    HeightBlockUndo final_undo;
    BOOST_REQUIRE(ledger.Apply(current, final_block, 345,
        {{StakeEventType::WITHDRAW, "A", stake}}, final_undo));
    BOOST_CHECK(!ledger.IsLocked(stake));
    // A fabricated or inconsistent snapshot must not be accepted as undo.
    HeightBlockUndo forged = final_undo;
    forged.previous_claims.clear();
    BOOST_CHECK(!ledger.Disconnect(forged));
    BOOST_CHECK(ledger.Tip() == final_block);
    forged = final_undo;
    forged.previous_height = 100;
    BOOST_CHECK(!ledger.Disconnect(forged));
    BOOST_CHECK(ledger.Tip() == final_block);
    BOOST_REQUIRE(ledger.Disconnect(final_undo));
    BOOST_CHECK(ledger.IsLocked(stake));
    BOOST_CHECK(ledger.Tip() == current);
    // Undo must restore the pending exit, allowing a different block at 345.
    uint256 alt; alt.begin()[0] = 16;
    HeightBlockUndo alternate_undo;
    BOOST_REQUIRE(ledger.Apply(current, alt, 345, {}, alternate_undo));
    BOOST_CHECK(ledger.IsLocked(stake));
    BOOST_CHECK(ledger.CanWithdraw("A", 345));
    BOOST_REQUIRE(ledger.Disconnect(alternate_undo));
    BOOST_CHECK(ledger.Tip() == current);
}


BOOST_AUTO_TEST_CASE(hybrid_height_replay_atomicity)
{
    using namespace Consensus::Hybrid;
    uint256 txid; txid.begin()[0] = 91;
    const StakeOutpoint stake{txid, 0};
    uint256 a; a.begin()[0] = 31;
    uint256 last = a;
    std::vector<HeightStakeReplayBlock> history{
        {uint256{}, a, 100, {{StakeEventType::BOND, "A", stake, 1000 * COIN}}}
    };
    // 100 confirmation blocks followed by the exit and unbonding interval.
    for (uint32_t height = 101; height <= 345; ++height) {
        uint256 block;
        block.begin()[0] = 32;
        block.begin()[1] = static_cast<unsigned char>(height);
        const std::vector<StakeEvent> events = height == 201 ?
            std::vector<StakeEvent>{{StakeEventType::EXIT, "A", stake}} :
            (height == 345 ? std::vector<StakeEvent>{{StakeEventType::WITHDRAW, "A", stake}} :
                             std::vector<StakeEvent>{});
        history.push_back({last, block, height, events});
        last = block;
    }
    HeightStakeLedger state;
    BOOST_REQUIRE(ReplayHeightStakeBlocks(history, state));
    BOOST_CHECK(state.Tip() == last);
    BOOST_CHECK(!state.IsLocked(stake));

    auto corrupt = history;
    corrupt[3].height += 1; // Discontinuous block heights.
    BOOST_CHECK(!ReplayHeightStakeBlocks(corrupt, state));
    BOOST_CHECK(state.Tip() == last);
    BOOST_CHECK(!state.IsLocked(stake));

    corrupt = history;
    // Move the exit to height 202: a withdrawal at 345 is now too early.
    corrupt[101].events.clear();
    corrupt[102].events = {{StakeEventType::EXIT, "A", stake}};
    BOOST_CHECK(!ReplayHeightStakeBlocks(corrupt, state));
    BOOST_CHECK(state.Tip() == last);

    corrupt = history;
    corrupt[2].parent.SetNull(); // Broken parent chain.
    BOOST_CHECK(!ReplayHeightStakeBlocks(corrupt, state));
    BOOST_CHECK(state.Tip() == last);
}


BOOST_AUTO_TEST_CASE(hybrid_height_spend_guard)
{
    using namespace Consensus::Hybrid;
    HeightStakeLedger ledger;
    uint256 txid; txid.begin()[0] = 104;
    const StakeOutpoint stake{txid, 1};
    uint256 a; a.begin()[0] = 105;
    HeightBlockUndo bond_undo;
    BOOST_REQUIRE(ledger.Apply(uint256{}, a, 100,
        {{StakeEventType::BOND, "A", stake, 1000 * COIN}}, bond_undo));

    CMutableTransaction spending;
    spending.vin.emplace_back(COutPoint{Txid::FromUint256(txid), 0});
    BOOST_CHECK(!SpendsHeightLockedStake(CTransaction(spending), ledger));
    spending.vin.emplace_back(COutPoint{Txid::FromUint256(txid), 1});
    BOOST_CHECK(SpendsHeightLockedStake(CTransaction(spending), ledger));

    uint256 parent = a;
    for (uint32_t height = 101; height <= 200; ++height) {
        uint256 block;
        block.begin()[0] = 106;
        block.begin()[1] = static_cast<unsigned char>(height);
        HeightBlockUndo undo;
        BOOST_REQUIRE(ledger.Apply(parent, block, height, {}, undo));
        parent = block;
    }
    uint256 exit; exit.begin()[0] = 107;
    HeightBlockUndo exit_undo;
    BOOST_REQUIRE(ledger.Apply(parent, exit, 201,
        {{StakeEventType::EXIT, "A", stake}}, exit_undo));
    BOOST_CHECK(SpendsHeightLockedStake(CTransaction(spending), ledger));
    BOOST_REQUIRE(ledger.Disconnect(exit_undo));
    BOOST_CHECK(SpendsHeightLockedStake(CTransaction(spending), ledger));
    BOOST_REQUIRE(ledger.Disconnect(bond_undo) == false); // Not the current tip.
}


BOOST_AUTO_TEST_CASE(hybrid_block_spend_guard)
{
    using namespace Consensus::Hybrid;
    HeightStakeLedger ledger;
    uint256 stake_id; stake_id.begin()[0] = 111;
    uint256 other_id; other_id.begin()[0] = 112;
    const StakeOutpoint stake{stake_id, 0};
    const StakeOutpoint other{other_id, 1};
    CMutableTransaction spend;
    spend.vin.emplace_back(COutPoint{Txid::FromUint256(stake_id), 0});
    const std::vector<CTransactionRef> transactions{MakeTransactionRef(spend)};
    BOOST_CHECK(!BlockSpendsBondedStake(transactions, {}, ledger));
    BOOST_CHECK(BlockSpendsBondedStake(transactions,
        {{StakeEventType::BOND, "A", stake, 1000 * COIN}}, ledger));
    BOOST_CHECK(!BlockSpendsBondedStake(transactions,
        {{StakeEventType::BOND, "B", other}}, ledger));
    BOOST_CHECK(BlockSpendsBondedStake({CTransactionRef{}}, {}, ledger));

    uint256 block; block.begin()[0] = 113;
    HeightBlockUndo undo;
    BOOST_REQUIRE(ledger.Apply(uint256{}, block, 100,
        {{StakeEventType::BOND, "A", stake, 1000 * COIN}}, undo));
    BOOST_CHECK(BlockSpendsBondedStake(transactions, {}, ledger));
    BOOST_CHECK(BlockSpendsBondedStake(transactions,
        {{StakeEventType::WITHDRAW, "A", stake}}, ledger));
    BOOST_REQUIRE(ledger.Disconnect(undo));
    BOOST_CHECK(!BlockSpendsBondedStake(transactions, {}, ledger));
}


BOOST_AUTO_TEST_CASE(hybrid_stake_envelope)
{
    using namespace Consensus::Hybrid;
    StakeEnvelope original;
    original.intent = StakeIntent::BOND;
    original.outpoint.txid.begin()[0] = 0xab;
    original.outpoint.txid.begin()[31] = 0xcd;
    original.outpoint.vout = 0x12345678;
    original.validator_id.begin()[0] = 0xef;

    const CScript encoded = EncodeStakeEnvelope(original);
    StakeEnvelope decoded;
    BOOST_REQUIRE(DecodeStakeEnvelope(encoded, decoded));
    BOOST_CHECK(decoded.intent == original.intent);
    BOOST_CHECK(decoded.outpoint == original.outpoint);
    BOOST_CHECK(decoded.validator_id == original.validator_id);

    for (const StakeIntent intent : {StakeIntent::EXIT, StakeIntent::WITHDRAW}) {
        original.intent = intent;
        BOOST_REQUIRE(DecodeStakeEnvelope(EncodeStakeEnvelope(original), decoded));
        BOOST_CHECK(decoded.intent == intent);
    }
    BOOST_CHECK(!DecodeStakeEnvelope(CScript() << OP_TRUE, decoded));

    // A second opcode or alternate push encoding is not canonical.
    const CScript trailing = EncodeStakeEnvelope(original) << OP_TRUE;
    BOOST_CHECK(!DecodeStakeEnvelope(trailing, decoded));

    // Wrong version, opcode or magic fail closed.
    std::vector<unsigned char> payload;
    opcodetype opcode;
    CScript::const_iterator cursor = encoded.begin();
    BOOST_REQUIRE(encoded.GetOp(cursor, opcode));
    BOOST_REQUIRE(encoded.GetOp(cursor, opcode, payload));
    payload[4] = 2;
    BOOST_CHECK(!DecodeStakeEnvelope(CScript() << OP_RETURN << payload, decoded));
    payload[4] = 1;
    payload[5] = 4;
    BOOST_CHECK(!DecodeStakeEnvelope(CScript() << OP_RETURN << payload, decoded));
    payload[5] = 1;
    payload[0] = 'X';
    BOOST_CHECK(!DecodeStakeEnvelope(CScript() << OP_RETURN << payload, decoded));
    payload[0] = 'C';
    payload.pop_back();
    BOOST_CHECK(!DecodeStakeEnvelope(CScript() << OP_RETURN << payload, decoded));
}


BOOST_AUTO_TEST_CASE(hybrid_transaction_intent_scan)
{
    using namespace Consensus::Hybrid;
    StakeEnvelope event;
    event.outpoint.txid.begin()[0] = 44;
    event.outpoint.vout = 3;
    event.validator_id.begin()[0] = 77;
    CMutableTransaction mtx;
    mtx.vout.emplace_back(0, EncodeStakeEnvelope(event));
    StakeEnvelope output;
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::ONE);
    BOOST_CHECK(output.outpoint == event.outpoint);
    BOOST_CHECK(output.validator_id == event.validator_id);

    mtx.vout.emplace_back(0, EncodeStakeEnvelope(event));
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::INVALID);
    mtx.vout.pop_back();
    mtx.vout[0].nValue = 1;
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::INVALID);
    mtx.vout[0].nValue = 0;
    const CScript valid = mtx.vout[0].scriptPubKey;
    CScript with_trailing_opcode = valid;
    with_trailing_opcode << OP_TRUE;
    mtx.vout[0].scriptPubKey = with_trailing_opcode;
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::INVALID);

    mtx.vout[0].scriptPubKey = CScript() << OP_RETURN <<
        std::vector<unsigned char>{'N','O','T','S','T','A','K','E'};
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::NONE);
    mtx.vout[0].scriptPubKey = valid;
    BOOST_CHECK(ScanStakeIntent(CTransaction(mtx), output) == StakeIntentScanResult::ONE);
}


BOOST_AUTO_TEST_CASE(hybrid_bond_parent_utxo_owner)
{
    using namespace Consensus::Hybrid;
    CCoinsView base;
    CCoinsViewCache cache(&base);
    CKey key;
    key.MakeNewKey(true);
    const CPubKey pub = key.GetPubKey();
    const uint160 owner = Hash160(pub);
    StakeEnvelope intent;
    intent.intent = StakeIntent::BOND;
    intent.validator_id = owner;
    intent.outpoint.txid.begin()[0] = 0xa1;
    intent.outpoint.vout = 2;
    const COutPoint outpoint{Txid::FromUint256(intent.outpoint.txid), intent.outpoint.vout};
    CTxOut deposit;
    deposit.nValue = 2000 * COIN;
    deposit.scriptPubKey = CScript() << OP_0 <<
        std::vector<unsigned char>(owner.begin(), owner.end());
    cache.AddCoin(outpoint, Coin(std::move(deposit), 100, false), false);
    uint256 genesis; genesis.begin()[0] = 0x42;
    uint256 nonce; nonce.begin()[0] = 0x54;
    const auto digest = RegistrationDigest(genesis, pub, intent.outpoint.txid,
        intent.outpoint.vout, 2000 * COIN, 200, nonce);
    std::vector<unsigned char> signature;
    BOOST_REQUIRE(key.Sign(digest, signature));
    BOOST_CHECK(VerifyBondIntentOwner(cache, intent, pub, signature,
        genesis, nonce, 199, 200));
    BOOST_CHECK(!VerifyBondIntentOwner(cache, intent, pub, signature,
        genesis, nonce, 198, 199));
    uint256 wrong = genesis;
    wrong.begin()[0] ^= 0x01;
    BOOST_CHECK(!VerifyBondIntentOwner(cache, intent, pub, signature,
        wrong, nonce, 199, 200));
    intent.validator_id.begin()[0] ^= 0x01;
    BOOST_CHECK(!VerifyBondIntentOwner(cache, intent, pub, signature,
        genesis, nonce, 199, 200));
    intent.validator_id = owner;
    intent.intent = StakeIntent::EXIT;
    BOOST_CHECK(!VerifyBondIntentOwner(cache, intent, pub, signature,
        genesis, nonce, 199, 200));
}


BOOST_AUTO_TEST_CASE(hybrid_bond_transaction_preflight)
{
    using namespace Consensus::Hybrid;
    CCoinsView base;
    CCoinsViewCache cache(&base);
    CKey key;
    key.MakeNewKey(true);
    const CPubKey pub = key.GetPubKey();
    StakeEnvelope intent;
    intent.intent = StakeIntent::BOND;
    intent.validator_id = Hash160(pub);
    intent.outpoint.txid.begin()[0] = 0xb1;
    intent.outpoint.vout = 1;
    CTxOut utxo;
    utxo.nValue = 2500 * COIN;
    utxo.scriptPubKey = CScript() << OP_0 <<
        std::vector<unsigned char>(intent.validator_id.begin(), intent.validator_id.end());
    cache.AddCoin(COutPoint{Txid::FromUint256(intent.outpoint.txid), 1},
        Coin(std::move(utxo), 100, false), false);
    uint256 genesis; genesis.begin()[0] = 0x42;
    uint256 nonce; nonce.begin()[0] = 0x55;
    std::vector<unsigned char> signature;
    BOOST_REQUIRE(key.Sign(RegistrationDigest(genesis, pub, intent.outpoint.txid,
        intent.outpoint.vout, 2500 * COIN, 200, nonce), signature));

    CMutableTransaction mtx;
    mtx.vout.emplace_back(0, EncodeStakeEnvelope(intent));
    BOOST_CHECK(PreflightBondTransaction(CTransaction(mtx), cache, pub, signature,
        genesis, nonce, 199, 200));
    mtx.vin.emplace_back(COutPoint{Txid::FromUint256(intent.outpoint.txid), 1});
    BOOST_CHECK(!PreflightBondTransaction(CTransaction(mtx), cache, pub, signature,
        genesis, nonce, 199, 200));
    mtx.vin.clear();
    mtx.vout.emplace_back(0, EncodeStakeEnvelope(intent));
    BOOST_CHECK(!PreflightBondTransaction(CTransaction(mtx), cache, pub, signature,
        genesis, nonce, 199, 200));
    mtx.vout.pop_back();
    intent.intent = StakeIntent::EXIT;
    mtx.vout[0].scriptPubKey = EncodeStakeEnvelope(intent);
    BOOST_CHECK(!PreflightBondTransaction(CTransaction(mtx), cache, pub, signature,
        genesis, nonce, 199, 200));
}


BOOST_AUTO_TEST_CASE(hybrid_embedded_bond_proof)
{
    using namespace Consensus::Hybrid;
    CCoinsView base;
    CCoinsViewCache view(&base);
    CKey owner; owner.MakeNewKey(true);
    const CPubKey pub = owner.GetPubKey();
    StakeEnvelope intent;
    intent.outpoint.txid.begin()[0] = 0xd1;
    intent.outpoint.vout = 1;
    intent.validator_id = Hash160(pub);
    CTxOut deposit;
    deposit.nValue = 3000 * COIN;
    deposit.scriptPubKey = CScript() << OP_0 <<
        std::vector<unsigned char>(intent.validator_id.begin(), intent.validator_id.end());
    view.AddCoin(COutPoint{Txid::FromUint256(intent.outpoint.txid), 1},
                 Coin(std::move(deposit), 100, false), false);
    uint256 genesis; genesis.begin()[0] = 0xd2;
    uint256 nonce; nonce.begin()[0] = 0xd3;
    StakeProofCarrier proof;
    proof.owner = pub;
    proof.nonce = nonce;
    // Sign the transaction without its proof carrier to avoid circular signing.
    CMutableTransaction mtx;
    mtx.vout.emplace_back(0, EncodeStakeEnvelope(intent));

    const uint256 digest = BondTransactionDigest(
        CTransaction(mtx), intent, pub, genesis, nonce,
        3000 * COIN, 200);

    BOOST_REQUIRE(owner.Sign(digest, proof.signature));

    const CScript encoded = EncodeStakeProofCarrier(proof);
    BOOST_REQUIRE(!encoded.empty());

    StakeProofCarrier decoded;
    BOOST_CHECK(DecodeStakeProofCarrier(encoded, decoded));
    BOOST_CHECK(decoded.nonce == nonce);

    mtx.vout.emplace_back(0, encoded);
    BOOST_CHECK(PreflightEmbeddedBondTransaction(CTransaction(mtx),
        view, genesis, 199, 200));
    // SECURITY REGRESSION: modifying transaction inputs must
    // invalidate the original embedded authorization.
    CMutableTransaction altered_input = mtx;

    uint256 unrelated_txid;
    unrelated_txid.begin()[0] = 0xee;

    altered_input.vin.emplace_back(
        COutPoint{Txid::FromUint256(unrelated_txid), 0});

    BOOST_CHECK(!PreflightEmbeddedBondTransaction(
        CTransaction(altered_input),
        view, genesis, 199, 200));


    // SECURITY REGRESSION: a copied proof must not authorize a
    // different transaction with the same bond registration details.
    CMutableTransaction transplanted = mtx;
    transplanted.nLockTime = 12345;
    BOOST_CHECK(!PreflightEmbeddedBondTransaction(CTransaction(transplanted),
        view, genesis, 199, 200));

    // SECURITY REGRESSION: changing a non-proof output must
    // invalidate the original transaction-bound authorization.
    CMutableTransaction altered_output = mtx;
    altered_output.vout.emplace_back(
        1 * COIN, CScript() << OP_TRUE);

    BOOST_CHECK(!PreflightEmbeddedBondTransaction(
        CTransaction(altered_output),
        view, genesis, 199, 200));

    mtx.vout.emplace_back(0, encoded);
    BOOST_CHECK(!PreflightEmbeddedBondTransaction(CTransaction(mtx),
        view, genesis, 199, 200));
    mtx.vout.pop_back();
    mtx.vout[1].nValue = 1;
    BOOST_CHECK(!PreflightEmbeddedBondTransaction(CTransaction(mtx),
        view, genesis, 199, 200));
    mtx.vout[1].nValue = 0;
    uint256 other_genesis = genesis;
    other_genesis.begin()[0] ^= 1;
    BOOST_CHECK(!PreflightEmbeddedBondTransaction(CTransaction(mtx),
        view, other_genesis, 199, 200));
}


BOOST_AUTO_TEST_CASE(hybrid_bond_transaction_digest)
{
    using namespace Consensus::Hybrid;
    CKey owner; owner.MakeNewKey(true);
    const CPubKey pub = owner.GetPubKey();
    StakeEnvelope intent;
    intent.outpoint.txid.begin()[0] = 0xc1;
    intent.validator_id = Hash160(pub);
    uint256 genesis; genesis.begin()[0] = 0xc2;
    uint256 nonce; nonce.begin()[0] = 0xc3;
    CMutableTransaction mtx;
    mtx.vin.emplace_back(COutPoint{Txid::FromUint256(intent.outpoint.txid), 2});
    mtx.vout.emplace_back(0, EncodeStakeEnvelope(intent));
    const auto digest = [&]() {
        return BondTransactionDigest(CTransaction(mtx), intent, pub, genesis,
            nonce, 2000 * COIN, 200);
    };
    const uint256 original = digest();
    std::vector<unsigned char> signature;
    BOOST_REQUIRE(owner.Sign(original, signature));
    BOOST_CHECK(pub.Verify(digest(), signature));
    mtx.vin[0].nSequence -= 1;
    BOOST_CHECK(digest() != original);
    BOOST_CHECK(!pub.Verify(digest(), signature));
    mtx.vin[0].nSequence += 1;
    mtx.vout.emplace_back(1 * COIN, CScript() << OP_TRUE);
    BOOST_CHECK(digest() != original);
    mtx.vout.pop_back();
    BOOST_CHECK(digest() == original);
    uint256 foreign = genesis;
    foreign.begin()[0] ^= 1;
    BOOST_CHECK(BondTransactionDigest(CTransaction(mtx), intent, pub,
        foreign, nonce, 2000 * COIN, 200) != original);
}


BOOST_AUTO_TEST_CASE(hybrid_proposed_subsidy_schedule)
{
    using namespace Consensus::Hybrid;
    const auto before = ProposedSubsidySplit(ACTIVATION_HEIGHT - 1, 500000);
    BOOST_CHECK(before.pow == 64 * COIN);
    BOOST_CHECK(before.pos == 0);
    const auto at = ProposedSubsidySplit(ACTIVATION_HEIGHT, 500000);
    BOOST_CHECK(at.pow == 32 * COIN);
    BOOST_CHECK(at.pos == 32 * COIN);
    BOOST_CHECK(at.total == 64 * COIN);
    const auto half = ProposedSubsidySplit(500000, 500000);
    BOOST_CHECK(half.pow == 16 * COIN);
    BOOST_CHECK(half.pos == 16 * COIN);
    BOOST_CHECK(half.pow + half.pos == half.total);
    BOOST_CHECK(ProposedSubsidySplit(-1, 500000).total == 0);
    BOOST_CHECK(ProposedSubsidySplit(50000, 0).total == 0);
    BOOST_CHECK(EffectiveStake(999 * COIN) == 0);
    BOOST_CHECK(EffectiveStake(1000 * COIN) == 1000 * COIN);
    BOOST_CHECK(EffectiveStake(800000 * COIN) == 250000 * COIN);
}


BOOST_AUTO_TEST_CASE(hybrid_halving_aware_validator_rewards)
{
    using namespace Consensus::Hybrid;

    constexpr int halving_interval = 500000;

    const auto before = ProposedSubsidySplit(49999, halving_interval);
    BOOST_CHECK_EQUAL(before.pos, 0);

    const auto activation = ProposedSubsidySplit(50000, halving_interval);
    BOOST_CHECK_EQUAL(activation.pos, 32 * COIN);

    const auto pre_halving = ProposedSubsidySplit(499999, halving_interval);
    BOOST_CHECK_EQUAL(pre_halving.pos, 32 * COIN);

    const auto first_halving = ProposedSubsidySplit(500000, halving_interval);
    BOOST_CHECK_EQUAL(first_halving.pos, 16 * COIN);

    const auto second_halving = ProposedSubsidySplit(1000000, halving_interval);
    BOOST_CHECK_EQUAL(second_halving.pos, 8 * COIN);

    for (const auto budget : {
             activation.pos,
             first_halving.pos,
             second_halving.pos}) {
        const auto rewards = AllocateRewards(
            {1000 * COIN, 1000 * COIN}, budget);

        BOOST_REQUIRE(rewards.valid);
        BOOST_REQUIRE_EQUAL(rewards.entries.size(), 2U);

        BOOST_CHECK_EQUAL(rewards.entries[0].reward, budget / 2);
        BOOST_CHECK_EQUAL(rewards.entries[1].reward, budget / 2);
        BOOST_CHECK_EQUAL(
            rewards.distributed + rewards.undistributed,
            budget);
    }
}

BOOST_AUTO_TEST_CASE(hybrid_validator_payout_script)
{
    using namespace Consensus::Hybrid;

    uint160 validator;
    validator.begin()[0] = 0x42;

    const std::string id = validator.GetHex();

    CScript payout;
    BOOST_REQUIRE(ValidatorPayoutScript(id, payout));

    const CScript expected = CScript() << OP_0 <<
        std::vector<unsigned char>(validator.begin(), validator.end());

    BOOST_CHECK(payout == expected);

    CScript rejected;
    BOOST_CHECK(!ValidatorPayoutScript("Alice", rejected));
    BOOST_CHECK(!ValidatorPayoutScript("", rejected));
    BOOST_CHECK(!ValidatorPayoutScript(std::string(40, 'g'), rejected));
    BOOST_CHECK(!ValidatorPayoutScript(std::string(40, 'A'), rejected));

    BOOST_CHECK(rejected.empty());
}

BOOST_AUTO_TEST_CASE(hybrid_validator_payment_halving_budgets)
{
    using namespace Consensus::Hybrid;

    uint160 alice;
    alice.begin()[0] = 0x41;

    uint160 bob;
    bob.begin()[0] = 0x42;

    LedgerRewardAllocation allocation;
    allocation.valid = true;

    for (const CAmount budget : {
             32 * COIN, 16 * COIN, 8 * COIN}) {

        allocation.distributed = budget;
        allocation.undistributed = 0;

        allocation.validators = {
            {alice.GetHex(), 1000 * COIN, 1000 * COIN, budget / 2},
            {bob.GetHex(), 1000 * COIN, 1000 * COIN, budget / 2}
        };

        const auto plan = BuildValidatorPaymentPlan(
            allocation, budget);

        BOOST_REQUIRE(plan.valid);
        BOOST_REQUIRE_EQUAL(plan.payments.size(), 2U);
        BOOST_CHECK_EQUAL(plan.distributed, budget);
        BOOST_CHECK_EQUAL(plan.undistributed, 0);

        BOOST_CHECK_EQUAL(plan.payments[0].amount, budget / 2);
        BOOST_CHECK_EQUAL(plan.payments[1].amount, budget / 2);

        // The allocation cannot claim a larger budget.
        BOOST_CHECK(!BuildValidatorPaymentPlan(
            allocation, budget / 2).valid);

        // The allocation must account for the entire budget.
        allocation.undistributed = 1;
        BOOST_CHECK(!BuildValidatorPaymentPlan(
            allocation, budget).valid);
    }

    BOOST_CHECK(!BuildValidatorPaymentPlan(
        allocation, -1).valid);

    BOOST_CHECK(!BuildValidatorPaymentPlan(
        allocation, 33 * COIN).valid);

    // No eligible validators: the entire budget remains unpaid.
    allocation.validators.clear();
    allocation.distributed = 0;
    allocation.undistributed = 8 * COIN;

    const auto empty = BuildValidatorPaymentPlan(
        allocation, 8 * COIN);

    BOOST_REQUIRE(empty.valid);
    BOOST_CHECK(empty.payments.empty());
    BOOST_CHECK_EQUAL(empty.undistributed, 8 * COIN);
}

BOOST_AUTO_TEST_CASE(hybrid_coinbase_payout_verification)
{
    using namespace Consensus::Hybrid;

    uint160 alice;
    alice.begin()[0] = 0x51;

    uint160 bob;
    bob.begin()[0] = 0x52;

    LedgerRewardAllocation allocation;
    allocation.valid = true;
    allocation.distributed = 32 * COIN;
    allocation.undistributed = 0;

    allocation.validators = {
        {alice.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN},
        {bob.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN}
    };

    const auto plan = BuildValidatorPaymentPlan(allocation);

    BOOST_REQUIRE(plan.valid);
    BOOST_REQUIRE_EQUAL(plan.payments.size(), 2U);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();

    coinbase.vout.resize(3);
    coinbase.vout[0].nValue = 32 * COIN;
    coinbase.vout[0].scriptPubKey = CScript() << OP_TRUE;

    for (size_t i = 0; i < plan.payments.size(); ++i) {
        coinbase.vout[i + 1].nValue = plan.payments[i].amount;
        coinbase.vout[i + 1].scriptPubKey = plan.payments[i].script;
    }

    BOOST_CHECK(VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));

    // A missing validator output must fail.
    coinbase.vout.pop_back();
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));

    // Restore the second validator payment.
    coinbase.vout.resize(3);
    coinbase.vout[2].nValue = plan.payments[1].amount;
    coinbase.vout[2].scriptPubKey = plan.payments[1].script;

    // Incorrect reward amount must fail.
    coinbase.vout[2].nValue -= 1;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));

    coinbase.vout[2].nValue += 1;

    // One output cannot satisfy two expected payments.
    ValidatorPaymentPlan duplicate = plan;
    duplicate.payments[1] = duplicate.payments[0];

    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), duplicate, 32 * COIN, 32 * COIN, 0));

    // Miner cannot claim more than the PoW budget.
    coinbase.vout[0].nValue += 1;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));
    coinbase.vout[0].nValue -= 1;

    // Duplicate validator output must fail.
    coinbase.vout.push_back(coinbase.vout[1]);
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));
    coinbase.vout.pop_back();

    // Extra miner output cannot exceed the total PoW budget.
    coinbase.vout.push_back(coinbase.vout[0]);
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));
    coinbase.vout.pop_back();

    // A negative output must fail.
    coinbase.vout[0].nValue = -1;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));
    coinbase.vout[0].nValue = 32 * COIN;

    // Invalid subsidy budgets must fail.
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 33 * COIN, 32 * COIN, 0));

    // A valid transaction fee may increase the miner reward.
    coinbase.vout[0].nValue += 1;
    BOOST_CHECK(VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 1));
    coinbase.vout[0].nValue -= 1;

    // A non-coinbase transaction must fail.
    coinbase.vin[0].prevout.n = 0;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, 32 * COIN, 32 * COIN, 0));
}


BOOST_AUTO_TEST_CASE(hybrid_activation_coinbase_payouts)
{
    using namespace Consensus::Hybrid;

    const auto before = ProposedSubsidySplit(49999, 500000);
    const auto active = ProposedSubsidySplit(50000, 500000);

    BOOST_CHECK_EQUAL(before.pow, 64 * COIN);
    BOOST_CHECK_EQUAL(before.pos, 0);
    BOOST_REQUIRE_EQUAL(active.pow, 32 * COIN);
    BOOST_REQUIRE_EQUAL(active.pos, 32 * COIN);

    uint160 alice;
    alice.begin()[0] = 0x61;

    uint160 bob;
    bob.begin()[0] = 0x62;

    LedgerRewardAllocation allocation;
    allocation.valid = true;
    allocation.distributed = active.pos;
    allocation.undistributed = 0;
    allocation.validators = {
        {alice.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN},
        {bob.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN}
    };

    const auto plan = BuildValidatorPaymentPlan(allocation, active.pos);
    BOOST_REQUIRE(plan.valid);
    BOOST_REQUIRE_EQUAL(plan.payments.size(), 2U);

    CMutableTransaction coinbase;
    coinbase.vin.resize(1);
    coinbase.vin[0].prevout.SetNull();
    coinbase.vout.resize(3);

    coinbase.vout[0].nValue = active.pow;
    coinbase.vout[0].scriptPubKey = CScript() << OP_TRUE;

    for (size_t i = 0; i < plan.payments.size(); ++i) {
        coinbase.vout[i + 1].nValue = plan.payments[i].amount;
        coinbase.vout[i + 1].scriptPubKey = plan.payments[i].script;
    }

    BOOST_CHECK(VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, active.pow, active.pos, 0));

    // The miner cannot claim the validator subsidy.
    coinbase.vout[0].nValue += 1;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, active.pow, active.pos, 0));
    coinbase.vout[0].nValue -= 1;

    // A validator cannot claim an extra satoshi.
    coinbase.vout[1].nValue += 1;
    BOOST_CHECK(!VerifyValidatorCoinbasePayments(
        CTransaction(coinbase), plan, active.pow, active.pos, 0));
}


BOOST_AUTO_TEST_CASE(hybrid_exit_authorization_security)
{
    using namespace Consensus::Hybrid;

    CKey owner;
    owner.MakeNewKey(true);

    CKey attacker;
    attacker.MakeNewKey(true);

    const CPubKey owner_pubkey = owner.GetPubKey();
    const CPubKey attacker_pubkey = attacker.GetPubKey();

    uint256 genesis;
    genesis.begin()[0] = 0x11;

    uint256 tx_hash;
    tx_hash.begin()[0] = 0x22;

    uint256 stake_txid;
    stake_txid.begin()[0] = 0x33;

    uint256 nonce;
    nonce.begin()[0] = 0x44;

    const StakeOutpoint stake{stake_txid, 0};
    const std::string validator = "validator-alice";
    constexpr uint32_t exit_height = 50100;

    const uint256 digest = ExitAuthorizationDigest(
        genesis, tx_hash, validator, stake, exit_height, nonce);

    std::vector<unsigned char> signature;
    BOOST_REQUIRE(owner.Sign(digest, signature));

    BOOST_CHECK(VerifyExitAuthorizationSignature(
        owner_pubkey, digest, signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        attacker_pubkey, digest, signature));

    uint256 other_genesis = genesis;
    other_genesis.begin()[0] ^= 1;

    uint256 other_tx = tx_hash;
    other_tx.begin()[0] ^= 1;

    uint256 other_nonce = nonce;
    other_nonce.begin()[0] ^= 1;

    const StakeOutpoint other_stake{stake_txid, 1};

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            other_genesis, tx_hash, validator, stake, exit_height, nonce),
        signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            genesis, other_tx, validator, stake, exit_height, nonce),
        signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            genesis, tx_hash, "validator-bob", stake, exit_height, nonce),
        signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            genesis, tx_hash, validator, other_stake, exit_height, nonce),
        signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            genesis, tx_hash, validator, stake, exit_height + 1, nonce),
        signature));

    BOOST_CHECK(!VerifyExitAuthorizationSignature(
        owner_pubkey,
        ExitAuthorizationDigest(
            genesis, tx_hash, validator, stake, exit_height, other_nonce),
        signature));
}

BOOST_AUTO_TEST_CASE(hybrid_validator_payment_security)
{
    using namespace Consensus::Hybrid;

    uint160 alice;
    alice.begin()[0] = 0x31;

    uint160 bob;
    bob.begin()[0] = 0x32;

    LedgerRewardAllocation allocation;
    allocation.valid = true;
    allocation.distributed = 32 * COIN;
    allocation.undistributed = 0;

    allocation.validators = {
        {alice.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN},
        {bob.GetHex(), 1000 * COIN, 1000 * COIN, 16 * COIN}
    };

    BOOST_CHECK(BuildValidatorPaymentPlan(allocation).valid);

    // Duplicate validator identities must be rejected.
    allocation.validators[1].validator = alice.GetHex();
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.validators[1].validator = bob.GetHex();

    // Distributed rewards cannot exceed the PoS budget.
    allocation.distributed = 33 * COIN;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.distributed = 32 * COIN;

    // Distributed plus undistributed cannot exceed the budget.
    allocation.undistributed = COIN;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.undistributed = 0;

    // Negative undistributed amounts are invalid.
    allocation.undistributed = -1;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.undistributed = 0;

    // Negative validator rewards must be rejected.
    allocation.validators[0].reward = -1;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.validators[0].reward = 16 * COIN;

    BOOST_CHECK(BuildValidatorPaymentPlan(allocation).valid);
}

BOOST_AUTO_TEST_CASE(hybrid_validator_reward_payments)
{
    using namespace Consensus::Hybrid;

    uint160 alice;
    alice.begin()[0] = 0x11;

    uint160 bob;
    bob.begin()[0] = 0x22;

    LedgerRewardAllocation allocation;
    allocation.valid = true;
    allocation.distributed = 32 * COIN;
    allocation.undistributed = 0;

    allocation.validators = {
        {alice.GetHex(), 1000 * COIN, 1000 * COIN, 8 * COIN},
        {bob.GetHex(), 3000 * COIN, 3000 * COIN, 24 * COIN}
    };

    const auto plan = BuildValidatorPaymentPlan(allocation);

    BOOST_REQUIRE(plan.valid);
    BOOST_REQUIRE_EQUAL(plan.payments.size(), 2U);
    BOOST_CHECK_EQUAL(plan.payments[0].amount, 8 * COIN);
    BOOST_CHECK_EQUAL(plan.payments[1].amount, 24 * COIN);
    BOOST_CHECK_EQUAL(plan.distributed, 32 * COIN);

    CScript alice_script;
    BOOST_REQUIRE(ValidatorPayoutScript(alice.GetHex(), alice_script));
    BOOST_CHECK(plan.payments[0].script == alice_script);

    allocation.validators[1].validator = "Bob";
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.validators[1].validator = bob.GetHex();
    allocation.validators[1].reward = 0;
    allocation.distributed = 8 * COIN;
    allocation.undistributed = 24 * COIN;

    const auto zero = BuildValidatorPaymentPlan(allocation);
    BOOST_REQUIRE(zero.valid);
    BOOST_REQUIRE_EQUAL(zero.payments.size(), 1U);

    allocation.distributed = 32 * COIN;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);

    allocation.valid = false;
    BOOST_CHECK(!BuildValidatorPaymentPlan(allocation).valid);
}

BOOST_AUTO_TEST_CASE(hybrid_proportional_staking_rewards)
{
    using namespace Consensus::Hybrid;

    const auto none = AllocateRewards({});
    BOOST_REQUIRE(none.valid);
    BOOST_CHECK_EQUAL(none.distributed, 0);
    BOOST_CHECK_EQUAL(none.undistributed, 32 * COIN);

    const auto equal = AllocateRewards({1000 * COIN, 1000 * COIN});
    BOOST_REQUIRE(equal.valid);
    BOOST_REQUIRE_EQUAL(equal.entries.size(), 2U);
    BOOST_CHECK_EQUAL(equal.entries[0].reward, 16 * COIN);
    BOOST_CHECK_EQUAL(equal.entries[1].reward, 16 * COIN);

    const auto weighted = AllocateRewards({1000 * COIN, 3000 * COIN});
    BOOST_REQUIRE(weighted.valid);
    BOOST_REQUIRE_EQUAL(weighted.entries.size(), 2U);
    BOOST_CHECK_EQUAL(weighted.entries[0].reward, 8 * COIN);
    BOOST_CHECK_EQUAL(weighted.entries[1].reward, 24 * COIN);

    const auto minimum = AllocateRewards({999 * COIN, 1000 * COIN});
    BOOST_REQUIRE(minimum.valid);
    BOOST_REQUIRE_EQUAL(minimum.entries.size(), 2U);
    BOOST_CHECK_EQUAL(minimum.entries[0].reward, 0);
    BOOST_CHECK_EQUAL(minimum.entries[1].reward, 32 * COIN);

    const auto capped = AllocateRewards({800000 * COIN, 250000 * COIN});
    BOOST_REQUIRE(capped.valid);
    BOOST_REQUIRE_EQUAL(capped.entries.size(), 2U);
    BOOST_CHECK_EQUAL(capped.entries[0].effective_stake, 250000 * COIN);
    BOOST_CHECK_EQUAL(capped.entries[0].reward, 16 * COIN);
    BOOST_CHECK_EQUAL(capped.entries[1].reward, 16 * COIN);

    const auto rounded = AllocateRewards({1000 * COIN, 1000 * COIN}, 1);
    BOOST_REQUIRE(rounded.valid);
    BOOST_CHECK_EQUAL(rounded.distributed, 0);
    BOOST_CHECK_EQUAL(rounded.undistributed, 1);

    BOOST_CHECK(!AllocateRewards({-1}).valid);
    BOOST_CHECK(!AllocateRewards({MAX_MONEY + 1}).valid);
    BOOST_CHECK(!AllocateRewards({1000 * COIN}, -1).valid);
    BOOST_CHECK(!AllocateRewards({1000 * COIN}, TOTAL_SUBSIDY + 1).valid);
}

BOOST_AUTO_TEST_CASE(hybrid_three_validator_rounding)
{
    using namespace Consensus::Hybrid;

    const auto allocation = AllocateRewards(
        {1000 * COIN, 1000 * COIN, 1000 * COIN},
        32 * COIN);

    BOOST_REQUIRE(allocation.valid);
    BOOST_REQUIRE_EQUAL(allocation.entries.size(), 3U);

    for (const auto& entry : allocation.entries) {
        BOOST_CHECK_EQUAL(entry.reward, 1066666666);
    }

    BOOST_CHECK_EQUAL(allocation.distributed, 3199999998);
    BOOST_CHECK_EQUAL(allocation.undistributed, 2);
    BOOST_CHECK_EQUAL(
        allocation.distributed + allocation.undistributed,
        32 * COIN);
}

BOOST_AUTO_TEST_CASE(hybrid_withdraw_structure_preflight)
{
    using namespace Consensus::Hybrid;
    HeightStakeLedger ledger;
    uint160 validator;
    validator.begin()[0] = 0x61;
    const std::string id = validator.GetHex();
    uint256 txid;
    txid.begin()[0] = 0x62;
    const StakeOutpoint stake{txid, 2};
    uint256 parent;
    uint256 block;
    block.begin()[0] = 1;
    HeightBlockUndo undo;
    BOOST_REQUIRE(ledger.Apply(parent, block, 100,
        {{StakeEventType::BOND, id, stake, 1000 * COIN}}, undo));
    parent = block;
    for (uint32_t height = 101; height <= 200; ++height) {
        block.SetNull();
        block.begin()[0] = 2;
        block.begin()[1] = static_cast<unsigned char>(height);
        BOOST_REQUIRE(ledger.Apply(parent, block, height, {}, undo));
        parent = block;
    }
    block.SetNull();
    block.begin()[0] = 3;
    BOOST_REQUIRE(ledger.Apply(parent, block, 201,
        {{StakeEventType::EXIT, id, stake}}, undo));

    StakeEnvelope envelope;
    envelope.intent = StakeIntent::WITHDRAW;
    envelope.outpoint = stake;
    envelope.validator_id = validator;
    CMutableTransaction tx;
    tx.vin.emplace_back(COutPoint{Txid::FromUint256(txid), stake.vout});
    tx.vout.emplace_back(0, EncodeStakeEnvelope(envelope));
    BOOST_CHECK(!PreflightWithdrawalStructure(CTransaction(tx), ledger, 344));
    BOOST_CHECK(PreflightWithdrawalStructure(CTransaction(tx), ledger, 345));
    tx.vin.emplace_back(COutPoint{Txid::FromUint256(txid), stake.vout});
    BOOST_CHECK(!PreflightWithdrawalStructure(CTransaction(tx), ledger, 345));
    tx.vin.pop_back();
    tx.vin[0].prevout.n = 1;
    BOOST_CHECK(!PreflightWithdrawalStructure(CTransaction(tx), ledger, 345));
    tx.vin[0].prevout.n = stake.vout;
    envelope.intent = StakeIntent::EXIT;
    tx.vout[0].scriptPubKey = EncodeStakeEnvelope(envelope);
    BOOST_CHECK(!PreflightWithdrawalStructure(CTransaction(tx), ledger, 345));
}


BOOST_AUTO_TEST_CASE(hybrid_height_ledger_committed_amount)
{
    using namespace Consensus::Hybrid;

    HeightStakeLedger ledger;

    uint256 txid;
    txid.begin()[0] = 0x71;
    const StakeOutpoint stake{txid, 0};

    uint256 block;
    block.begin()[0] = 0x72;

    HeightBlockUndo undo;

    BOOST_REQUIRE(ledger.Apply(
        uint256{}, block, 100,
        {{StakeEventType::BOND, "Alice", stake, 800000 * COIN}},
        undo));

    const auto& records = ledger.Records();
    const auto it = records.find("Alice");

    BOOST_REQUIRE(it != records.end());
    BOOST_CHECK_EQUAL(it->second.committed, 800000 * COIN);
    BOOST_CHECK_EQUAL(EffectiveStake(it->second.committed), 250000 * COIN);

    BOOST_REQUIRE(ledger.Disconnect(undo));
    BOOST_CHECK(ledger.Records().empty());
}


BOOST_AUTO_TEST_CASE(hybrid_height_ledger_stake_boundaries)
{
    using namespace Consensus::Hybrid;

    const CAmount amounts[] = {
        0,
        999 * COIN,
        1000 * COIN,
        250000 * COIN,
        800000 * COIN,
        MAX_MONEY + 1
    };

    const bool accepted[] = {
        false, false, true, true, true, false
    };

    for (size_t i = 0; i < 6; ++i) {
        HeightStakeLedger ledger;

        uint256 txid;
        txid.begin()[0] = static_cast<unsigned char>(i + 1);

        const StakeOutpoint stake{txid, 0};

        uint256 block;
        block.begin()[0] = static_cast<unsigned char>(i + 20);

        HeightBlockUndo undo;

        const bool result = ledger.Apply(
            uint256{}, block, 100,
            {{StakeEventType::BOND, "Alice", stake, amounts[i]}},
            undo);

        BOOST_CHECK_EQUAL(result, accepted[i]);

        if (accepted[i]) {
            BOOST_CHECK(ledger.IsLocked(stake));
            BOOST_CHECK_EQUAL(
                ledger.Records().at("Alice").committed,
                amounts[i]);
            BOOST_CHECK_EQUAL(
                EffectiveStake(amounts[i]),
                amounts[i] > MAXIMUM_EFFECTIVE_STAKE
                    ? MAXIMUM_EFFECTIVE_STAKE
                    : amounts[i]);
        } else {
            BOOST_CHECK(ledger.Records().empty());
            BOOST_CHECK(ledger.Tip().IsNull());
            BOOST_CHECK(!ledger.IsLocked(stake));
        }
    }
}


BOOST_AUTO_TEST_CASE(hybrid_ledger_halving_rewards)
{
    using namespace Consensus::Hybrid;

    HeightStakeLedger ledger;

    uint256 alice_tx;
    alice_tx.begin()[0] = 0xD1;

    uint256 bob_tx;
    bob_tx.begin()[0] = 0xD2;

    uint256 block;
    block.begin()[0] = 0xD3;

    HeightBlockUndo undo;

    BOOST_REQUIRE(ledger.Apply(
        uint256{}, block, 100,
        {
            {StakeEventType::BOND, "Alice", {alice_tx, 0}, 1000 * COIN},
            {StakeEventType::BOND, "Bob", {bob_tx, 0}, 1000 * COIN}
        },
        undo));

    uint256 parent = block;

    for (uint32_t height = 101; height <= 200; ++height) {
        block.SetNull();
        block.begin()[0] = 0xD4;
        block.begin()[1] = static_cast<unsigned char>(height);

        BOOST_REQUIRE(ledger.Apply(
            parent, block, height, {}, undo));

        parent = block;
    }

    for (const int height : {50000, 500000, 1000000}) {
        const auto split = ProposedSubsidySplit(height, 500000);
        const auto rewards = AllocateLedgerRewards(ledger, split.pos);

        BOOST_REQUIRE(rewards.valid);
        BOOST_REQUIRE_EQUAL(rewards.validators.size(), 2U);

        BOOST_CHECK_EQUAL(rewards.validators[0].reward, split.pos / 2);
        BOOST_CHECK_EQUAL(rewards.validators[1].reward, split.pos / 2);

        BOOST_CHECK_EQUAL(rewards.distributed, split.pos);
        BOOST_CHECK_EQUAL(rewards.undistributed, 0);
    }
}


BOOST_AUTO_TEST_CASE(hybrid_regtest_activation_override)
{
    using namespace Consensus::Hybrid;

    constexpr int regtest_halving = 150;
    constexpr int regtest_activation = 100;

    const auto before = ProposedSubsidySplit(99, regtest_halving, regtest_activation);
    BOOST_CHECK_EQUAL(before.pow, 64 * COIN);
    BOOST_CHECK_EQUAL(before.pos, 0);
    BOOST_CHECK_EQUAL(before.total, 64 * COIN);

    const auto active = ProposedSubsidySplit(100, regtest_halving, regtest_activation);
    BOOST_CHECK_EQUAL(active.pow, 32 * COIN);
    BOOST_CHECK_EQUAL(active.pos, 32 * COIN);
    BOOST_CHECK_EQUAL(active.total, 64 * COIN);

    const auto halved = ProposedSubsidySplit(150, regtest_halving, regtest_activation);
    BOOST_CHECK_EQUAL(halved.pow, 16 * COIN);
    BOOST_CHECK_EQUAL(halved.pos, 16 * COIN);
    BOOST_CHECK_EQUAL(halved.total, 32 * COIN);

    const auto mainnet_default = ProposedSubsidySplit(100, 500000);
    BOOST_CHECK_EQUAL(mainnet_default.pow, 64 * COIN);
    BOOST_CHECK_EQUAL(mainnet_default.pos, 0);
    BOOST_CHECK_EQUAL(mainnet_default.total, 64 * COIN);
}

BOOST_AUTO_TEST_CASE(hybrid_ledger_reward_eligibility)
{
    using namespace Consensus::Hybrid;

    HeightStakeLedger ledger;

    uint256 txid;
    txid.begin()[0] = 0x91;
    const StakeOutpoint stake{txid, 0};

    uint256 block;
    block.begin()[0] = 0x92;

    HeightBlockUndo undo;

    BOOST_REQUIRE(ledger.Apply(
        uint256{}, block, 100,
        {{StakeEventType::BOND, "Alice", stake, 1000 * COIN}},
        undo));

    const auto pending = AllocateLedgerRewards(ledger);
    BOOST_REQUIRE(pending.valid);
    BOOST_CHECK(pending.validators.empty());
    BOOST_CHECK_EQUAL(pending.distributed, 0);

    uint256 parent = block;

    for (uint32_t height = 101; height <= 200; ++height) {
        block.SetNull();
        block.begin()[0] = 0x93;
        block.begin()[1] = static_cast<unsigned char>(height);

        BOOST_REQUIRE(ledger.Apply(
            parent, block, height, {}, undo));

        parent = block;
    }

    const auto bonded = AllocateLedgerRewards(ledger);
    BOOST_REQUIRE(bonded.valid);
    BOOST_REQUIRE_EQUAL(bonded.validators.size(), 1U);
    BOOST_CHECK_EQUAL(bonded.validators[0].reward, POS_SUBSIDY);
    BOOST_CHECK_EQUAL(bonded.distributed, POS_SUBSIDY);

    block.SetNull();
    block.begin()[0] = 0x94;

    BOOST_REQUIRE(ledger.Apply(
        parent, block, 201,
        {{StakeEventType::EXIT, "Alice", stake}},
        undo));

    const auto exiting = AllocateLedgerRewards(ledger);
    BOOST_REQUIRE(exiting.valid);
    BOOST_CHECK(exiting.validators.empty());
    BOOST_CHECK_EQUAL(exiting.distributed, 0);
}


BOOST_AUTO_TEST_CASE(hybrid_ledger_multiple_validator_rewards)
{
    using namespace Consensus::Hybrid;

    HeightStakeLedger ledger;

    uint256 alice_tx;
    alice_tx.begin()[0] = 0xA1;

    uint256 bob_tx;
    bob_tx.begin()[0] = 0xA2;

    uint256 whale_tx;
    whale_tx.begin()[0] = 0xA3;

    uint256 block;
    block.begin()[0] = 0xB1;

    HeightBlockUndo undo;

    BOOST_REQUIRE(ledger.Apply(
        uint256{}, block, 100,
        {
            {StakeEventType::BOND, "Alice", {alice_tx, 0}, 1000 * COIN},
            {StakeEventType::BOND, "Bob", {bob_tx, 0}, 3000 * COIN},
            {StakeEventType::BOND, "Whale", {whale_tx, 0}, 800000 * COIN}
        },
        undo));

    const auto pending = AllocateLedgerRewards(ledger);
    BOOST_REQUIRE(pending.valid);
    BOOST_CHECK(pending.validators.empty());
    BOOST_CHECK_EQUAL(pending.undistributed, POS_SUBSIDY);

    uint256 parent = block;

    for (uint32_t height = 101; height <= 200; ++height) {
        block.SetNull();
        block.begin()[0] = 0xB2;
        block.begin()[1] = static_cast<unsigned char>(height);

        BOOST_REQUIRE(ledger.Apply(
            parent, block, height, {}, undo));

        parent = block;
    }

    const auto rewards = AllocateLedgerRewards(ledger);

    BOOST_REQUIRE(rewards.valid);
    BOOST_REQUIRE_EQUAL(rewards.validators.size(), 3U);

    BOOST_CHECK_EQUAL(rewards.validators[0].validator, "Alice");
    BOOST_CHECK_EQUAL(rewards.validators[1].validator, "Bob");
    BOOST_CHECK_EQUAL(rewards.validators[2].validator, "Whale");

    BOOST_CHECK_EQUAL(rewards.validators[0].effective, 1000 * COIN);
    BOOST_CHECK_EQUAL(rewards.validators[1].effective, 3000 * COIN);
    BOOST_CHECK_EQUAL(rewards.validators[2].effective, 250000 * COIN);

    BOOST_CHECK_EQUAL(rewards.validators[2].committed, 800000 * COIN);

    BOOST_CHECK_EQUAL(
        rewards.distributed + rewards.undistributed,
        POS_SUBSIDY);

    BOOST_CHECK(rewards.validators[2].reward >
                rewards.validators[1].reward);

    BOOST_CHECK(rewards.validators[1].reward >
                rewards.validators[0].reward);

    BOOST_CHECK(rewards.distributed <= POS_SUBSIDY);
}


BOOST_AUTO_TEST_CASE(hybrid_ledger_reward_redistribution_after_exit)
{
    using namespace Consensus::Hybrid;

    HeightStakeLedger ledger;

    uint256 alice_tx;
    alice_tx.begin()[0] = 0xC1;

    uint256 bob_tx;
    bob_tx.begin()[0] = 0xC2;

    const StakeOutpoint alice{alice_tx, 0};
    const StakeOutpoint bob{bob_tx, 0};

    uint256 block;
    block.begin()[0] = 0xC3;

    HeightBlockUndo undo;

    BOOST_REQUIRE(ledger.Apply(
        uint256{}, block, 100,
        {
            {StakeEventType::BOND, "Alice", alice, 1000 * COIN},
            {StakeEventType::BOND, "Bob", bob, 3000 * COIN}
        },
        undo));

    uint256 parent = block;

    for (uint32_t height = 101; height <= 200; ++height) {
        block.SetNull();
        block.begin()[0] = 0xC4;
        block.begin()[1] = static_cast<unsigned char>(height);

        BOOST_REQUIRE(ledger.Apply(
            parent, block, height, {}, undo));

        parent = block;
    }

    const auto before = AllocateLedgerRewards(ledger);

    BOOST_REQUIRE(before.valid);
    BOOST_REQUIRE_EQUAL(before.validators.size(), 2U);

    BOOST_CHECK_EQUAL(before.validators[0].validator, "Alice");
    BOOST_CHECK_EQUAL(before.validators[1].validator, "Bob");

    BOOST_CHECK_EQUAL(before.validators[0].reward, 8 * COIN);
    BOOST_CHECK_EQUAL(before.validators[1].reward, 24 * COIN);
    BOOST_CHECK_EQUAL(before.distributed, POS_SUBSIDY);
    BOOST_CHECK_EQUAL(before.undistributed, 0);

    block.SetNull();
    block.begin()[0] = 0xC5;

    BOOST_REQUIRE(ledger.Apply(
        parent, block, 201,
        {{StakeEventType::EXIT, "Bob", bob}},
        undo));

    const auto after = AllocateLedgerRewards(ledger);

    BOOST_REQUIRE(after.valid);
    BOOST_REQUIRE_EQUAL(after.validators.size(), 1U);

    BOOST_CHECK_EQUAL(after.validators[0].validator, "Alice");
    BOOST_CHECK_EQUAL(after.validators[0].reward, POS_SUBSIDY);
    BOOST_CHECK_EQUAL(after.distributed, POS_SUBSIDY);
    BOOST_CHECK_EQUAL(after.undistributed, 0);
}

BOOST_AUTO_TEST_SUITE_END()
