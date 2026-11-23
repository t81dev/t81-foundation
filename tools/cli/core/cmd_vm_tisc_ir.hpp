#pragma once

#include "tools/cli/core/cli_args.hpp"

int run_tensor_command(const char* command_name, const Args& args);
int run_vm_command(const Args& args);
int run_profile_command(const Args& args);
int run_tier_command(const Args& args);
int run_tisc_command(const Args& args);
int run_ir_command(const Args& args);
