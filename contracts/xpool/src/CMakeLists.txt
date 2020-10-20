project(xpool)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( xpool xpool xpool.cpp )
target_include_directories( xpool PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( xpool ${CMAKE_SOURCE_DIR}/../ricardian )