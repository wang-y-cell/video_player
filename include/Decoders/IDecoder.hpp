#pragma once

#include <string>

class IDecoder {
public:
    virtual ~IDecoder() = default;
    virtual void close() = 0;
    virtual const std::string& lastError() const = 0;
};
