#pragma once

#include <vector>
#include <string>

struct Plugin
{
    Plugin();
    Plugin(const Plugin &) = delete;

    virtual const char *name() const = 0;
    virtual bool enabled() const = 0;
    virtual bool reg() = 0;
    virtual int priority() const { return 0; } // TODO: Replace with more flexible dependency-based mechanism.

    // TODO: Merge cmd into args, so we just have an arg vector (like main functions).
    // TODO: Consider making the result an ostream, not a string.
    virtual bool command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) { return false; }
};

extern std::vector<Plugin *> plugins;
