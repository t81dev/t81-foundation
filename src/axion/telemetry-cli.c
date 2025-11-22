/*
 telemetry-calc.cweb | CLI for Advanced Ternary Calculations and Introspection
   This tool is a command-line interface for performing secure, AI-aware ternary math
   using Axion's T81BigInt data types. It builds upon HanoiVM telemetry infrastructure
   and provides scientific, logical, and scripting-based operations from the terminal.
   
   Supported Commands:
   - raw:        Print raw JSON telemetry (pretty printed)
   - json:       Print compact JSON telemetry
   - get key:    Fetch nested key from telemetry JSON using dot notation
   - add A B:    Add two base-3 numbers
   - mul A B:    Multiply two base-3 numbers
   - sqrt A:     Square root of a ternary number
   - lua code:   Run inline Lua snippet
   - lua -f file.lua: Run Lua file
   - help:       Show this help text
*/
/*


