#pragma once

#include <string>

// System configuration read from config.txt by the "initialize" command.
// Numeric fields are 64-bit so the [0, 2^32] ranges can be represented.
struct Config
{
    long long numCpu = 0;
    std::string scheduler;
    long long quantumCycles = 0;
    long long batchProcessFreq = 0;
    long long minIns = 0;
    long long maxIns = 0;
    long long delayPerExec = 0;
};

// Parses the "key value" pairs found in `path` into `config`. Returns false and
// fills `errorMessage` when the file cannot be read, a required key is missing,
// or a value is not valid.
bool loadConfig(const std::string &path, Config &config, std::string &errorMessage);

// Checks every field against the values the simulator accepts:
//   num-cpu            [1, 128]
//   scheduler          "fcfs" or "rr"
//   quantum-cycles     [1, 2^32]
//   batch-process-freq [1, 2^32]
//   min-ins            [1, 2^32]
//   max-ins            [1, 2^32]
//   delay-per-exec     [0, 2^32]
bool validateConfig(const Config &config, std::string &errorMessage);
