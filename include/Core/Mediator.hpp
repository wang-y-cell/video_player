#pragma once

#include <string>

class BaseComponent;

class Mediator {
public:
    virtual ~Mediator() = default;
    virtual void Notify(BaseComponent* sender, const std::string& event) = 0;
};

//此类是用来协调组件之间的通信的,是各个组件的基类
class BaseComponent {
public:
    explicit BaseComponent(Mediator* mediator = nullptr) : mediator_(mediator) {}
    virtual ~BaseComponent() = default;

    //设置中介者,用于组件之间的通信
    //每个组件都需要设置一个中介者,用于接收其他组件的事件
    void set_mediator(Mediator* mediator) { mediator_ = mediator; }

protected:
    Mediator* mediator_ = nullptr;
};
