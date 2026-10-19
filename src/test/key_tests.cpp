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
    BOOST_CHECK(lifecycle.RequestExit("A", 201, 50));
    BOOST_CHECK(!lifecycle.Activate("A", 300));
    BOOST_CHECK(!lifecycle.RequestExit("A", 202, 50));
    BOOST_CHECK(!lifecycle.IsEligible("A"));
    BOOST_CHECK(lifecycle.IsLocked(bond));
    BOOST_CHECK(!lifecycle.Withdraw("A", 250));
    BOOST_CHECK(lifecycle.Withdraw("A", 251));
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
        {{StakeEventType::BOND, "A", stake}}, undo1));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_CHECK(!state.ApplyBlock(first, second,
        {{StakeEventType::BOND, "B", stake}}, undo2));
    BOOST_CHECK(state.Tip() == first);
    BOOST_CHECK(!state.ApplyBlock(first, second,
        {{StakeEventType::EXIT, "A", stake},
         {StakeEventType::BOND, "B", stake}}, undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_REQUIRE(state.ApplyBlock(first, second,
        {{StakeEventType::EXIT, "A", stake}}, undo2));
    BOOST_CHECK(!state.IsBonded("A"));
    BOOST_CHECK(state.IsClaimed(stake));
    BOOST_CHECK(!state.DisconnectBlock(undo1));
    BOOST_REQUIRE(state.DisconnectBlock(undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_CHECK(state.Tip() == first);
    BOOST_REQUIRE(state.ApplyBlock(first, third,
        {{StakeEventType::EXIT, "A", stake},
         {StakeEventType::WITHDRAW, "A", stake}}, undo2));
    BOOST_CHECK(!state.IsClaimed(stake));
    BOOST_REQUIRE(state.DisconnectBlock(undo2));
    BOOST_CHECK(state.IsBonded("A"));
    BOOST_REQUIRE(state.DisconnectBlock(undo1));
    BOOST_CHECK(!state.IsBonded("A"));
    BOOST_CHECK(!state.IsClaimed(stake));
    BOOST_CHECK(state.Tip().IsNull());
}

BOOST_AUTO_TEST_SUITE_END()
