#include <iostream>
#include <string>

#include "parser/cli_parser.h"
#include "parser/json_enforcer.h"
#include "persistence/snapshot.h"
#include "persistence/wal.h"
#include "server/cli_server.h"
#include "store/kv_store.h"

/**
 * @brief Creates the application components and starts the CLI loop.
 *
 * @return Exit status code for the operating system.
 */
int main(int argc, char* argv[]) {

    // Check if receiving JSON
    bool json_mode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--json") {
            json_mode = true;
            break;
        }
    }

    // Wire persistence into the store before replay so subsequent mutations are
    // durably logged.
    kv::persistence::WriteAheadLog wal;
    kv::persistence::Snapshot snapshot;
    kv::store::KVStore store(&wal, &snapshot);

    if (!json_mode) {
        std::cout << "Loading snapshot...\n";
    }

    // Load snapshot no matter what
    const kv::persistence::SnapshotLoadResult snapshot_result =
        store.LoadSnapshot(snapshot);
    if (!json_mode) {
        std::cout << "Loaded " << snapshot_result.entry_count
                << " snapshot entrie(s)\n";

        std::cout << "Replaying WAL...\n";
    }

    // Recover from WAL no matter what
    const std::size_t recovered_operations =
        store.ReplayFromWal(wal, snapshot_result.wal_offset);
    if (!json_mode) {
        std::cout << "Recovered " << recovered_operations << " operation(s)\n";
    }

    // Pass raw JSON mode input through the lightweight parser-layer enforcer.
    if (json_mode) {
        std::string raw_input;
        std::string line;
        while (std::getline(std::cin, line)) {
            raw_input += line;
            raw_input += '\n';
        }

        kv::parser::JsonEnforcer enforcer;
        if (enforcer.IsValidJson(raw_input)) {
            std::cout << "{\"status\":\"ok\",\"message\":\"valid JSON\"}\n";
            return 0;
        }

        std::cout << "{\"status\":\"error\",\"message\":\"invalid JSON\"}\n";
        return 1;
    }

    kv::parser::CliParser parser;
    kv::server::CliServer server(parser, store);

    server.Run(std::cin, std::cout);
    return 0;
}
