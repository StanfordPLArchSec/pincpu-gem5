#include "plugin.hh"

std::vector<Plugin *> plugins;

Plugin::Plugin()
{
    plugins.push_back(this);
}

namespace {

struct DummyPlugin : Plugin
{
    bool reg() override { return false; }
    bool command(const std::string &cmd, const std::vector<std::string> &args, std::string &result) override { return false; }
};

}
