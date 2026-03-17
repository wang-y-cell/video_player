#pragma once

#include <string>

class BaseComponent;

class Mediator {
public:
    virtual ~Mediator() = default;
    virtual void Notify(BaseComponent* sender, const std::string& event) = 0;
};

class BaseComponent {
public:
    explicit BaseComponent(Mediator* mediator = nullptr) : mediator_(mediator) {}
    virtual ~BaseComponent() = default;

    void set_mediator(Mediator* mediator) { mediator_ = mediator; }

protected:
    Mediator* mediator_ = nullptr;
};
