#ifndef ISETUPABLE_H
#define ISETUPABLE_H
class ISetupable {
public:
    virtual ~ISetup() = default;
    virtual void setup() = 0;
};
#endif
