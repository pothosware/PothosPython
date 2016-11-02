// Copyright (c) 2016-2016 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include <Pothos/Config.hpp>
#include <Poco/Logger.h>

class PothosPythonLogger
{
public:
    PothosPythonLogger(const std::string &name):
        _logger(Poco::Logger::get(name))
    {
        return;
    }

    void log(const std::string &source, const std::string &text, const std::string &level)
    {
        _logger.log(Poco::Message(source, text, levelToPrio(level)));
    }

private:

    static Poco::Message::Priority levelToPrio(const std::string &level)
    {
        if (level == "FATAL") return Poco::Message::PRIO_FATAL;
        if (level == "CRITICAL") return Poco::Message::PRIO_CRITICAL;
        if (level == "ERROR") return Poco::Message::PRIO_ERROR;
        if (level == "WARNING") return Poco::Message::PRIO_WARNING;
        if (level == "INFO") return Poco::Message::PRIO_INFORMATION;
        if (level == "DEBUG") return Poco::Message::PRIO_DEBUG;
        return Poco::Message::PRIO_INFORMATION;
    }

    Poco::Logger &_logger;
};

#include <Pothos/Managed.hpp>

static auto managedPothosPythonLogger = Pothos::ManagedClass()
    .registerConstructor<PothosPythonLogger, std::string>()
    .registerMethod(POTHOS_FCN_TUPLE(PothosPythonLogger, log))
    .commit("Pothos/Python/Logger");
