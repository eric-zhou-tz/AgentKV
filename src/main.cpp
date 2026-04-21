#include <iostream>

#include "parser/command_parser.h"
#include "persistence/snapshot.h"
#include "persistence/wal.h"
#include "server/cli_server.h"
#include "store/kv_store.h"

/**
 * @brief Creates the application components and starts the CLI loop.
 *
 * @return Exit status code for the operating system.
 */
int main() {
  // Wire persistence into the store before replay so subsequent CLI mutations
  // are durably logged.
  std::cout << "[INFO] Initializing store..." << std::endl;
  kv::persistence::WriteAheadLog wal;
  kv::persistence::Snapshot snapshot;
  kv::store::KVStore store(&wal, &snapshot);

  // Recovery is layered: load the last full checkpoint first, then replay only
  // WAL records written after the byte offset covered by that snapshot.
  std::cout << "[INFO] Loading snapshot..." << std::endl;
  const kv::persistence::SnapshotLoadResult snapshot_result =
      store.LoadSnapshot(snapshot);
  if (snapshot_result.loaded) {
    std::cout << "[INFO] Loaded snapshot entries: "
              << snapshot_result.entry_count << std::endl;
  } else {
    std::cout << "[INFO] No snapshot found" << std::endl;
  }

  std::cout << "[INFO] Replaying WAL..." << std::endl;
  const std::size_t recovered_operations =
      store.ReplayFromWal(wal, snapshot_result.wal_offset);
  if (recovered_operations == 0) {
    std::cout << "[INFO] No WAL entries to replay" << std::endl;
  } else {
    std::cout << "[INFO] Replayed WAL records: " << recovered_operations
              << std::endl;
  }
  std::cout << "[INFO] Store ready" << std::endl;

  kv::parser::CommandParser parser;
  kv::server::CliServer server(parser, store);

  server.Run(std::cin, std::cout);
  return 0;
}
