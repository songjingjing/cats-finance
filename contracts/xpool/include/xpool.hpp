#include <utils.hpp>
#include <safemath.hpp>

CONTRACT xpool : public contract
{
public:
  using contract::contract;
  static constexpr uint8_t POOL_TYPE_NORMAL = 0;
  static constexpr uint8_t POOL_TYPE_RAM = 1;
  static constexpr name MINED_TOKEN = name("eoscatstoken");
  static constexpr name ADMIN = name("eoscatsadmin");
  static constexpr name FUND = name("eoscatsdever");
  static constexpr symbol MINED_SYMBOL = symbol("CAT", 4);
  static constexpr symbol_code MINED_SYMBOL_CODE = symbol_code("CAT");
  static constexpr int64_t MAX_SUPPLY = 2'1000'0000;

  ACTION create(name contract, symbol sym, asset reward, uint32_t epoch_time, uint32_t duration, asset min_staked, uint8_t type);
  ACTION claim(name owner, uint64_t pool_id);
  ACTION harvest(uint64_t pool_id, uint32_t nonce);

  void handle_transfer(name from, name to, asset quantity, string memo, name code);

private:
  TABLE pool
  {
    uint64_t id;
    uint8_t type;
    name contract;
    symbol sym;
    asset total_staked;
    asset total_reward;
    asset released_reward;
    uint32_t epoch_time;
    uint32_t duration;
    asset min_staked;
    uint32_t last_harvest_time;
    uint64_t primary_key() const { return id; }
  };

  TABLE miner
  {
    name owner;
    asset staked;
    asset claimed;
    asset unclaimed;
    uint64_t primary_key() const { return owner.value; }
  };

  typedef eosio::multi_index<"pools"_n, pool> pools_mi;
  typedef eosio::multi_index<"miners"_n, miner> miners_mi;
};
