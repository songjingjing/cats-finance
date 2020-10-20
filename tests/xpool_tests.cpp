#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/genesis_state.hpp>
#include <fc/filesystem.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"

#include <fc/variant_object.hpp>
#include <fstream>

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;
using eosio::chain::uint128_t;

class xpool_tester : public tester
{
public:
  void basic_setup()
  {
    produce_blocks(2);

    create_accounts({N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake),
                     N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.rex)});

    produce_blocks(100);
    deploy_token(N(eosio.token));
  }

  void deploy_token(const name token)
  {
    set_code(token, contracts::token_wasm());
    set_abi(token, contracts::token_abi().data());
    {
      const auto &accnt = control->db().get<account_object, by_name>(token);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_token_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
    }
  }

  void create_currency(name contract, name manager, asset maxsupply)
  {
    auto act = mvo()("issuer", manager)("maximum_supply", maxsupply);
    base_tester::push_action(contract, N(create), contract, act);
  }

  void issue(const name contract, const asset &amount, const name &manager, const name &to)
  {
    base_tester::push_action(contract, N(issue), manager, mvo()("to", to)("quantity", amount)("memo", ""));
  }

  void create_core_token(symbol core_symbol = symbol{CORE_SYM})
  {
    FC_ASSERT(core_symbol.decimals() == 4, "create_core_token assumes core token has 4 digits of precision");
    create_currency(N(eosio.token), config::system_account_name, asset(100000000000000, core_symbol));
    issue(N(eosio.token), asset(10000000000000, core_symbol), N(eosio), N(eosio));
    BOOST_REQUIRE_EQUAL(asset(10000000000000, core_symbol), get_token_balance(N(eosio.token), "eosio", core_symbol));
  }

  void create_token(name contract, name manager, uint64_t maxsupply, symbol sym)
  {
    create_currency(contract, manager, asset(maxsupply, sym));
    issue(contract, asset(maxsupply, sym), manager, manager);
    BOOST_REQUIRE_EQUAL(asset(maxsupply, sym), get_token_balance(contract, manager.to_string(), sym));
  }

  void transfer_token(const name &code, const name &from, const name &to, const asset &amount, const string &memo)
  {
    base_tester::push_action(code, N(transfer), from, mvo()("from", from)("to", to)("quantity", amount)("memo", memo));
  }

  action_result push_token_action(const account_name &code, const account_name &signer, const action_name &name, const variant_object &data)
  {
    string action_type_name = abi_token_ser.get_action_type(name);

    action act;
    act.account = code;
    act.name = name;
    act.data = abi_token_ser.variant_to_binary(action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time));

    return base_tester::push_action(std::move(act), signer.to_uint64_t());
  }

  action_result tf_token(name code, account_name from, account_name to, asset quantity, string memo)
  {
    return push_token_action(code, from, N(transfer), mvo()("from", from)("to", to)("quantity", quantity)("memo", memo));
  }

  void deploy_system_contract(bool call_init = true)
  {
    set_code(config::system_account_name, contracts::system_wasm());
    set_abi(config::system_account_name, contracts::system_abi().data());
    if (call_init)
    {
      base_tester::push_action(config::system_account_name, N(init),
                               config::system_account_name, mutable_variant_object()("version", 0)("core", CORE_SYM_STR));
    }
    {
      const auto &accnt = control->db().get<account_object, by_name>(config::system_account_name);
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_system_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
    }
  }

  void init_accounts()
  {
    create_account_with_resources(N(rabbitstoken), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(rabbitspoolx), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(rabbitsadmin), config::system_account_name, core_sym::from_string("100.0000"), false);

    create_account_with_resources(N(tethertether), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(eosdmdtokens), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(tokenaceosdt), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(tokenaceusde), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(tokenacctaaa), config::system_account_name, core_sym::from_string("100.0000"), false);

    create_account_with_resources(N(rabbitsuser1), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(rabbitsuser2), config::system_account_name, core_sym::from_string("100.0000"), false);
    create_account_with_resources(N(rabbitsuser3), config::system_account_name, core_sym::from_string("100.0000"), false);

    set_authority(N(rabbitspoolx), config::active_name,
                  authority(1, {{get_public_key(N(rabbitspoolx), "active"), 1}},
                            {{{N(rabbitspoolx), config::eosio_code_name}, 1}}),
                  config::owner_name,
                  {{N(rabbitspoolx), config::active_name}},
                  {get_private_key(N(rabbitspoolx), "active")});
  }

  void deploy_xpool()
  {
    set_code(N(rabbitspoolx), contracts::xpool_wasm());
    set_abi(N(rabbitspoolx), contracts::xpool_abi().data());
    {
      const auto &accnt = control->db().get<account_object, by_name>(N(rabbitspoolx));
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_xpool_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
    }
  }

  void setup_token_RAB()
  {
    const symbol sym = symbol(SY(4, CAT));
    set_code(N(rabbitstoken), contracts::token_wasm());
    set_abi(N(rabbitstoken), contracts::token_abi().data());
    {
      const auto &accnt = control->db().get<account_object, by_name>(N(rabbitstoken));
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_token_ser.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
    }

    create_currency(N(rabbitstoken), N(rabbitspoolx), asset(2'1000'0000, sym));
  }

  void setup_token_EOS()
  {
    const symbol sym = symbol{CORE_SYM};
    const asset amt = asset(10000'0000, sym);
    transfer_token(N(eosio.token), N(eosio), N(rabbitsuser1), amt, "");
    transfer_token(N(eosio.token), N(eosio), N(rabbitsuser2), amt, "");
    transfer_token(N(eosio.token), N(eosio), N(rabbitsuser3), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(eosio.token), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(eosio.token), "rabbitsuser2", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(eosio.token), "rabbitsuser3", sym));
  }

  void setup_token_USDT()
  {
    const symbol sym = symbol(SY(4, USDT));
    deploy_token(N(tethertether));
    create_token(N(tethertether), N(tethertether), 10000'0000'0000, sym);

    const asset amt = asset(10000'0000, sym);
    transfer_token(N(tethertether), N(tethertether), N(rabbitsuser1), amt, "");
    transfer_token(N(tethertether), N(tethertether), N(rabbitsuser2), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tethertether), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tethertether), "rabbitsuser2", sym));

    // Fake Token
    const symbol symX = symbol(SY(4, USDTT));
    create_token(N(tethertether), N(tethertether), 10000'0000'0000, symX);

    const asset amtX = asset(10000'0000, symX);
    transfer_token(N(tethertether), N(tethertether), N(rabbitsuser1), amtX, "");
    transfer_token(N(tethertether), N(tethertether), N(rabbitsuser2), amtX, "");
    BOOST_REQUIRE_EQUAL(amtX, get_token_balance(N(tethertether), "rabbitsuser1", symX));
    BOOST_REQUIRE_EQUAL(amtX, get_token_balance(N(tethertether), "rabbitsuser2", symX));
  }

  void setup_token_USDE()
  {
    const symbol sym = symbol(SY(4, USDE));
    deploy_token(N(tokenaceusde));
    create_token(N(tokenaceusde), N(tokenaceusde), 10000'0000'0000, sym);

    const asset amt = asset(10000'0000, sym);
    transfer_token(N(tokenaceusde), N(tokenaceusde), N(rabbitsuser1), amt, "");
    transfer_token(N(tokenaceusde), N(tokenaceusde), N(rabbitsuser2), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenaceusde), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenaceusde), "rabbitsuser2", sym));
  }

  void setup_token_EOSDT()
  {
    const symbol sym = symbol(SY(4, EOSDT));
    deploy_token(N(tokenaceosdt));
    create_token(N(tokenaceosdt), N(tokenaceosdt), 10000'0000'0000, sym);

    const asset amt = asset(10000'0000, sym);
    transfer_token(N(tokenaceosdt), N(tokenaceosdt), N(rabbitsuser1), amt, "");
    transfer_token(N(tokenaceosdt), N(tokenaceosdt), N(rabbitsuser2), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenaceosdt), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenaceosdt), "rabbitsuser2", sym));
  }

  void setup_token_DMD()
  {
    const symbol sym = symbol(SY(10, DMD));
    deploy_token(N(eosdmdtokens));
    create_token(N(eosdmdtokens), N(eosdmdtokens), 10000'00'0000'0000, sym);

    const asset amt = asset(1000'00'0000'0000, sym);
    transfer_token(N(eosdmdtokens), N(eosdmdtokens), N(rabbitsuser1), amt, "");
    transfer_token(N(eosdmdtokens), N(eosdmdtokens), N(rabbitsuser2), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(eosdmdtokens), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(eosdmdtokens), "rabbitsuser2", sym));
  }

  void setup_token_AAA()
  {
    const symbol sym = symbol(SY(4, AAA));
    deploy_token(N(tokenacctaaa));
    create_token(N(tokenacctaaa), N(tokenacctaaa), 10'0000'0000, sym);

    const asset amt = asset(1'0000'0000, sym);
    transfer_token(N(tokenacctaaa), N(tokenacctaaa), N(rabbitsuser1), amt, "");
    transfer_token(N(tokenacctaaa), N(tokenacctaaa), N(rabbitsuser2), amt, "");
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenacctaaa), "rabbitsuser1", sym));
    BOOST_REQUIRE_EQUAL(amt, get_token_balance(N(tokenacctaaa), "rabbitsuser2", sym));
  }

  xpool_tester()
  {
    basic_setup();
    create_core_token();
    deploy_system_contract();
    init_accounts();

    setup_token_EOS();
    setup_token_USDT();
    setup_token_USDE();
    setup_token_EOSDT();
    setup_token_DMD();
    setup_token_RAB();
    deploy_xpool();
    setup_token_AAA();
  }

  transaction_trace_ptr create_account_with_resources(account_name a, account_name creator, asset ramfunds, bool multisig,
                                                      asset net = core_sym::from_string("10.0000"), asset cpu = core_sym::from_string("10.0000"))
  {
    signed_transaction trx;
    set_transaction_headers(trx);

    authority owner_auth;
    if (multisig)
    {
      // multisig between account's owner key and creators active permission
      owner_auth = authority(2, {key_weight{get_public_key(a, "owner"), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
    }
    else
    {
      owner_auth = authority(get_public_key(a, "owner"));
    }

    trx.actions.emplace_back(vector<permission_level>{{creator, config::active_name}},
                             newaccount{
                                 .creator = creator,
                                 .name = a,
                                 .owner = owner_auth,
                                 .active = authority(get_public_key(a, "active"))});

    trx.actions.emplace_back(get_action(config::system_account_name, N(buyram), vector<permission_level>{{creator, config::active_name}},
                                        mvo()("payer", creator)("receiver", a)("quant", ramfunds)));

    trx.actions.emplace_back(get_action(config::system_account_name, N(delegatebw), vector<permission_level>{{creator, config::active_name}},
                                        mvo()("from", creator)("receiver", a)("stake_net_quantity", net)("stake_cpu_quantity", cpu)("transfer", 0)));

    set_transaction_headers(trx);
    trx.sign(get_private_key(creator, "active"), control->get_chain_id());
    return push_transaction(trx);
  }

  vector<char> get_row_by_primary_key(name code, name scope, name table, const uint64_t key) const
  {
    vector<char> data;
    const auto &db = control->db();
    const auto *t_id = db.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(code, scope, table));
    if (!t_id)
    {
      return data;
    }
    FC_ASSERT(t_id != 0, "object not found");
    const auto &idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

    auto itr = idx.lower_bound(boost::make_tuple(t_id->id, key));
    if (itr == idx.end() || itr->t_id != t_id->id || key != itr->primary_key)
    {
      return data;
    }

    data.resize(itr->value.size());
    memcpy(data.data(), itr->value.data(), data.size());
    return data;
  }

  asset get_token_balance(const name code, const account_name &act, symbol balance_symbol = symbol{CORE_SYM})
  {
    vector<char> data = get_row_by_account(code, act, N(accounts), account_name(balance_symbol.to_symbol_code().value));
    return data.empty() ? asset(0, balance_symbol) : abi_token_ser.binary_to_variant("account", data, abi_serializer::create_yield_function(abi_serializer_max_time))["balance"].as<asset>();
  }

  asset get_token_balance(const name code, std::string_view act, symbol balance_symbol = symbol{CORE_SYM})
  {
    return get_token_balance(code, account_name(act), balance_symbol);
  }

  action_result push_xpool_action(const account_name &signer, const action_name &name, const variant_object &data)
  {
    string action_type_name = abi_xpool_ser.get_action_type(name);

    action act;
    act.account = N(rabbitspoolx);
    act.name = name;
    act.data = abi_xpool_ser.variant_to_binary(action_type_name, data, abi_serializer::create_yield_function(abi_serializer_max_time));

    return base_tester::push_action(std::move(act), signer.to_uint64_t());
  }

  fc::variant get_xpool_pool(const uint64_t pool_id)
  {
    vector<char> data = get_row_by_primary_key(N(rabbitspoolx), N(rabbitspoolx), N(pools), pool_id);
    if (data.empty())
      std::cout << "\nData is empty\n"
                << std::endl;
    return data.empty() ? fc::variant() : abi_xpool_ser.binary_to_variant("pool", data, abi_serializer::create_yield_function(abi_serializer_max_time));
  }

  fc::variant get_xpool_miner(const name owner, const uint64_t pool_id)
  {
    vector<char> data = get_row_by_account(N(rabbitspoolx), name(pool_id), N(miners), owner);
    if (data.empty())
      std::cout << "\nData is empty\n"
                << std::endl;
    return data.empty() ? fc::variant() : abi_xpool_ser.binary_to_variant("miner", data, abi_serializer::create_yield_function(abi_serializer_max_time));
  }

  action_result xpool_create(name contract, symbol sym, asset reward, uint32_t epoch_time, uint32_t duration, asset min_staked, uint8_t type)
  {
    return push_xpool_action(N(rabbitsadmin), N(create), mvo()("contract", contract)("sym", sym)("reward", reward)("epoch_time", epoch_time)("duration", duration)("min_staked", min_staked)("type", type));
  }

  action_result xpool_claim(name owner, uint64_t pool_id)
  {
    return push_xpool_action(owner, N(claim), mvo()("owner", owner)("pool_id", pool_id));
  }

  action_result xpool_harvest(uint64_t pool_id, uint32_t nonce)
  {
    return push_xpool_action(N(rabbitsadmin), N(harvest), mvo()("pool_id", pool_id)("nonce", nonce));
  }

  abi_serializer abi_token_ser;
  abi_serializer abi_xpool_ser;
  abi_serializer abi_system_ser;
};

BOOST_AUTO_TEST_SUITE(xpool_tests)

BOOST_FIXTURE_TEST_CASE(create_tests, xpool_tester)
try
{
  const uint32_t epoch = 1596626691;
  const uint32_t duration = 604800;
  BOOST_REQUIRE_EQUAL(error("missing authority of rabbitsadmin"),
                      push_xpool_action(N(eosio), N(create), mvo()("contract", N(eosio.token))("sym", "4,EOS")("reward", asset::from_string("13000.0000 CAT"))("epoch_time", epoch)("duration", duration)("min_staked", asset::from_string("1.0000 EOS"))("type", 0)));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Reward symbol error"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 RABX"), epoch, duration, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Min staked symbol error"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOSX"), 0));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Invalid epoch"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), 0, duration, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Invalid duration"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, 0, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Reach the max circulation"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("21000.0001 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 0));

  auto pool = get_xpool_pool(1);
  BOOST_REQUIRE_EQUAL(pool["id"], 1);
  BOOST_REQUIRE_EQUAL(pool["type"], 0);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosio.token");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOS");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "13000.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);

  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Token exists"),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 0));

  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tethertether), symbol(SY(4, USDT)), asset::from_string("1930.0000 CAT"), epoch, duration, asset::from_string("1.0000 USDT"), 0));

  pool = get_xpool_pool(2);
  BOOST_REQUIRE_EQUAL(pool["id"], 2);
  BOOST_REQUIRE_EQUAL(pool["type"], 0);
  BOOST_REQUIRE_EQUAL(pool["contract"], "tethertether");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,USDT");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000 USDT");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "1930.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 USDT");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);

  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceosdt), symbol(SY(4, EOSDT)), asset::from_string("1030.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOSDT"), 0));

  pool = get_xpool_pool(3);
  BOOST_REQUIRE_EQUAL(pool["id"], 3);
  BOOST_REQUIRE_EQUAL(pool["type"], 0);
  BOOST_REQUIRE_EQUAL(pool["contract"], "tokenaceosdt");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOSDT");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000 EOSDT");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "1030.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOSDT");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);

  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceusde), symbol(SY(4, USDE)), asset::from_string("1030.0000 CAT"), epoch, duration, asset::from_string("1.0000 USDE"), 0));

  pool = get_xpool_pool(4);
  BOOST_REQUIRE_EQUAL(pool["id"], 4);
  BOOST_REQUIRE_EQUAL(pool["type"], 0);
  BOOST_REQUIRE_EQUAL(pool["contract"], "tokenaceusde");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,USDE");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000 USDE");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "1030.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 USDE");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);

  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosdmdtokens), symbol(SY(10, DMD)), asset::from_string("70.0000 CAT"), epoch, duration, asset::from_string("0.0200000000 DMD"), 0));

  pool = get_xpool_pool(5);
  BOOST_REQUIRE_EQUAL(pool["id"], 5);
  BOOST_REQUIRE_EQUAL(pool["type"], 0);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosdmdtokens");
  BOOST_REQUIRE_EQUAL(pool["sym"], "10,DMD");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000000000 DMD");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "70.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "0.0200000000 DMD");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);

  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("2700.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 1));

  pool = get_xpool_pool(6);
  BOOST_REQUIRE_EQUAL(pool["id"], 6);
  BOOST_REQUIRE_EQUAL(pool["type"], 1);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosio.token");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOS");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "0.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "2700.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["last_harvest_time"], epoch);
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(deposit_tests, xpool_tester)
try
{
  const uint32_t epoch = 1630426200;
  const uint32_t duration = 604800;
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tethertether), symbol(SY(4, USDT)), asset::from_string("1930.0000 CAT"), epoch, duration, asset::from_string("1.0000 USDT"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceosdt), symbol(SY(4, EOSDT)), asset::from_string("1030.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOSDT"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceusde), symbol(SY(4, USDE)), asset::from_string("1030.0000 CAT"), epoch, duration, asset::from_string("1.0000 USDE"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosdmdtokens), symbol(SY(10, DMD)), asset::from_string("70.0000 CAT"), 1597231491, duration, asset::from_string("0.0200000000 DMD"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("2700.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 1));

  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Pool not found"),
                      tf_token(N(tokenacctaaa), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("1.0000 AAA"), ""));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Pool not found"),
                      tf_token(N(tethertether), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("1.0000 USDTT"), ""));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("The amount of staked is too small"),
                      tf_token(N(eosio.token), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("0.0001 EOS"), ""));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Mining is over"),
                      tf_token(N(eosdmdtokens), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("0.1000000000 DMD"), ""));

  // Deposit
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("10.0000 EOS"), ""));
  BOOST_REQUIRE_EQUAL(asset::from_string("1.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsadmin", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 EOS"), get_token_balance(N(eosio.token), "rabbitspoolx", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("9999.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsuser1", symbol(SY(4, EOS))));

  auto miner = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner["staked"], "9.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner["unclaimed"], "0.0000 CAT");

  // Deposit
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("10.0000 EOS"), ""));
  BOOST_REQUIRE_EQUAL(asset::from_string("2.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsadmin", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 EOS"), get_token_balance(N(eosio.token), "rabbitspoolx", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("9998.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsuser1", symbol(SY(4, EOS))));

  miner = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner["unclaimed"], "0.0000 CAT");

  // Deposit RAM
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser2), N(rabbitspoolx), asset::from_string("10.0000 EOS"), "1"));
  BOOST_REQUIRE_EQUAL(asset::from_string("3.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsadmin", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 EOS"), get_token_balance(N(eosio.token), "rabbitspoolx", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("9990.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsuser2", symbol(SY(4, EOS))));

  miner = get_xpool_miner(N(rabbitsuser2), 6);
  BOOST_REQUIRE_EQUAL(miner["owner"], "rabbitsuser2");
  BOOST_REQUIRE_EQUAL(miner["staked"], "9.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner["unclaimed"], "0.0000 CAT");
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(harvest_tests, xpool_tester)
try
{
  const uint32_t epoch = 1598889600;
  const uint32_t duration = 604800;
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("13000.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tethertether), symbol(SY(4, USDT)), asset::from_string("1930.0000 CAT"), epoch, duration, asset::from_string("1.0000 USDT"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceosdt), symbol(SY(4, EOSDT)), asset::from_string("1030.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOSDT"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(tokenaceusde), symbol(SY(4, USDE)), asset::from_string("1030.0000 CAT"), 1630928988, duration, asset::from_string("1.0000 USDE"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosdmdtokens), symbol(SY(10, DMD)), asset::from_string("70.0000 CAT"), 1597231491, duration, asset::from_string("0.0200000000 DMD"), 0));
  BOOST_REQUIRE_EQUAL(success(),
                      xpool_create(N(eosio.token), symbol(SY(4, EOS)), asset::from_string("2700.0000 CAT"), epoch, duration, asset::from_string("1.0000 EOS"), 1));

  // rabbitsuser1 deposit
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser1), N(rabbitspoolx), asset::from_string("20.0000 EOS"), ""));
  BOOST_REQUIRE_EQUAL(asset::from_string("2.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsadmin", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 EOS"), get_token_balance(N(eosio.token), "rabbitspoolx", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("9998.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsuser1", symbol(SY(4, EOS))));

  auto miner1 = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner1["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner1["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner1["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner1["unclaimed"], "0.0000 CAT");

  // rabbitsuser2 deposit
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser2), N(rabbitspoolx), asset::from_string("20.0000 EOS"), ""));
  BOOST_REQUIRE_EQUAL(asset::from_string("4.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsadmin", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("0.0000 EOS"), get_token_balance(N(eosio.token), "rabbitspoolx", symbol(SY(4, EOS))));
  BOOST_REQUIRE_EQUAL(asset::from_string("9998.0000 EOS"), get_token_balance(N(eosio.token), "rabbitsuser2", symbol(SY(4, EOS))));

  auto miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["owner"], "rabbitsuser2");
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "0.0000 CAT");

  // Harvest
  const uint64_t pool_id = 1;
  uint64_t nonce = 1;
  BOOST_REQUIRE_EQUAL(error("missing authority of rabbitsadmin"),
                      push_xpool_action(N(eosio), N(harvest), mvo()("pool_id", 1)("nonce", 1)));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Pool not exists"),
                      xpool_harvest(0, nonce));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Mining hasn't started yet"),
                      xpool_harvest(4, nonce));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Mining is over"),
                      xpool_harvest(5, nonce));

  produce_blocks(10 * 60 * 2);
  BOOST_REQUIRE_EQUAL(success(), xpool_harvest(1, nonce));

  auto pool = get_xpool_pool(1);
  BOOST_REQUIRE_EQUAL(pool["id"], 1);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosio.token");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOS");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "36.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "13000.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "630.3584 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOS");

  BOOST_REQUIRE_EQUAL(asset::from_string("630.3584 CAT"), get_token_balance(N(rabbitstoken), "rabbitspoolx", symbol(SY(4, CAT))));

  miner1 = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner1["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner1["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner1["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner1["unclaimed"], "315.1792 CAT");

  miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["owner"], "rabbitsuser2");
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "315.1792 CAT");

  // 100 seconds
  produce_blocks(99 * 2);
  BOOST_REQUIRE_EQUAL(success(), xpool_harvest(1, nonce));
  pool = get_xpool_pool(1);
  BOOST_REQUIRE_EQUAL(pool["id"], 1);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosio.token");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOS");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "36.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "13000.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "632.4984 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOS");

  BOOST_REQUIRE_EQUAL(asset::from_string("632.4984 CAT"), get_token_balance(N(rabbitstoken), "rabbitspoolx", symbol(SY(4, CAT))));
  miner1 = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner1["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner1["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner1["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner1["unclaimed"], "316.2492 CAT");

  miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["owner"], "rabbitsuser2");
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "316.2492 CAT");

  // 100 seconds
  produce_blocks(100 * 2);
  BOOST_REQUIRE_EQUAL(success(), xpool_harvest(1, nonce));
  pool = get_xpool_pool(1);
  BOOST_REQUIRE_EQUAL(pool["id"], 1);
  BOOST_REQUIRE_EQUAL(pool["contract"], "eosio.token");
  BOOST_REQUIRE_EQUAL(pool["sym"], "4,EOS");
  BOOST_REQUIRE_EQUAL(pool["total_staked"], "36.0000 EOS");
  BOOST_REQUIRE_EQUAL(pool["total_reward"], "13000.0000 CAT");
  BOOST_REQUIRE_EQUAL(pool["released_reward"], "634.6384 CAT");
  BOOST_REQUIRE_EQUAL(pool["epoch_time"], epoch);
  BOOST_REQUIRE_EQUAL(pool["duration"], duration);
  BOOST_REQUIRE_EQUAL(pool["min_staked"], "1.0000 EOS");
  BOOST_REQUIRE_EQUAL(asset::from_string("634.6384 CAT"), get_token_balance(N(rabbitstoken), "rabbitspoolx", symbol(SY(4, CAT))));

  miner1 = get_xpool_miner(N(rabbitsuser1), 1);
  BOOST_REQUIRE_EQUAL(miner1["owner"], "rabbitsuser1");
  BOOST_REQUIRE_EQUAL(miner1["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner1["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner1["unclaimed"], "317.3192 CAT");

  miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["owner"], "rabbitsuser2");
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "317.3192 CAT");

  // Claim
  BOOST_REQUIRE_EQUAL(error("missing authority of rabbitsuser2"),
                      push_xpool_action(N(rabbitsuser1), N(claim), mvo()("owner", N(rabbitsuser2))("pool_id", 1)));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("Pool not exists"),
                      xpool_claim(N(rabbitsuser1), 16));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("No this miner"),
                      xpool_claim(N(rabbitsuser3), 1));
  BOOST_REQUIRE_EQUAL(success(), tf_token(N(eosio.token), N(rabbitsuser3), N(rabbitspoolx), asset::from_string("20.0000 EOS"), ""));
  BOOST_REQUIRE_EQUAL(wasm_assert_msg("No unclaimed"),
                      xpool_claim(N(rabbitsuser3), 1));

  BOOST_REQUIRE_EQUAL(success(), xpool_claim(N(rabbitsuser2), 1));
  miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "317.3192 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "0.0000 CAT");
  BOOST_REQUIRE_EQUAL(asset::from_string("317.3192 CAT"), get_token_balance(N(rabbitstoken), "rabbitsuser2", symbol(SY(4, CAT))));

  produce_blocks(98 * 2);
  BOOST_REQUIRE_EQUAL(success(), xpool_harvest(1, nonce));
  miner2 = get_xpool_miner(N(rabbitsuser2), 1);
  BOOST_REQUIRE_EQUAL(miner2["staked"], "18.0000 EOS");
  BOOST_REQUIRE_EQUAL(miner2["claimed"], "317.3192 CAT");
  BOOST_REQUIRE_EQUAL(miner2["unclaimed"], "0.7133 CAT");
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
