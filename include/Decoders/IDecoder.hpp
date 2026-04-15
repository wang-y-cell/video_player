#pragma once

#include <string>

//解码器基类
class IDecoder {
public:
    virtual ~IDecoder() = default;
    virtual void close() = 0;
    virtual const std::string& lastError() const = 0;
};
