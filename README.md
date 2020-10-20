# eoscats.io

## Introduction
CAT is a DeFi projects based on the high performance blcok chain EOS. The private key is burned and the contract code is open sourced. The project has no pre-mining, no founder shares, no VC interests.CAT uses TVI (Total Value Inflow) as the project value assessment. There is no pool of funds in the smart contract, so there is no need to worry about the security of funds. 90% of your invested assets will be automatically returned to your EOS account, and 10% will be donated for subsequent development and operation.

## Roadmap
- CAT will be officially launched nearly at 2020-10-17 20:00:00(CST), please pay attention to the official lunch time.
- After one week of mining CAT, FISH liquidity pool will be deployed, which will empower CAT and FISH, it will contains CAT single mining pool, FISH single mining pool and other LP minig pools. Also there won't be any assets staked in pools to ensure 100% security.
- Within one week after FISH liquidiy mining launching, a game project will be deployed to empower CAT and FISH. The game is currently confidential and is in the final test procedure, which will include NFT.
- After stable operation of metioned projects above, DAO will be opened gradually according to community opinions.
- Project governance will be carried out according to the opinions of community DAO.

## CAT Token Info
- Token name: CAT
- Total supply: 21000.0000
- Token contract: [eoscatstoken](https://bloks.io/account/eoscatstoken)
- Pool contract: [eoscatspools](https://bloks.io/account/eoscatspools)

##  FISH Token Info
- Token name: FISH
- Total supply: 105000.0000
- Token contract: [fisheostoken](https://bloks.io/account/fisheostoken)

## CAT Pools:
- EOS: 13000 CAT
- USDT: 1500 CAT
- USN: 1500 CAT
- BOX: 2500 CAT
- OGX: 1500 CAT
- DFS: 1000 CAT
- CAT will be distributed in only four weeks.

## Reasons for token selection
- Stable coins issued based on the EOS network  (USDT, USN)
- DEX based on the EOS network  (BOX，DFS)
- Compound assets based on the EOS network  (OGX)
## Security

### No pool of funds
CRT uses the TVI model as a project value assessment, and will not lock any funds in the contract, so it can ensure the 100% security of the contract.

### Private keys burned
CRT has given the authority to the system account `eosio.null`, the project has no control and cannot update the contract.

### How to verify the contract

- EOSIO Version: 2.0.5
- EOS.CDT Version: 1.7.0

```
cleos get code eoscatstoken > eoscatstoken.hash
cleos get code eoscatspools > eoscatspools.hash
```

Build from source:
```
./build.sh
```

Compare the hash:
`shasum -a 256 build/contracts/xpool/xpool.wasm` of `eoscatspools.hash`
`shasum -a 256 build/contracts/eosio.token/eosio.token.wasm` of `eoscatstoken.hash`

- Pool Hash: dba223a5411b887b7f7f4ff04dac7aa282a9d419103e886a31974f0ffceccebb
- Token Hash: f6a2939074d69fc194d4b7b5a4d2c24e2766046ddeaa58b63ddfd579a0193623
