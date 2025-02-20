#ifndef LAMPSTATETAG_H
#define LAMPSTATETAG_H

#include "inet/common/TagBase_m.h"

namespace inet {

class INET_API LampStateTag : public TagBase
{
  protected:
    bool isOn = false;

  public:
    LampStateTag() {}
    virtual ~LampStateTag() {}

    void setIsOn(bool on) { isOn = on; }
    bool getIsOn() const { return isOn; }

    virtual LampStateTag *dup() const override { return new LampStateTag(*this); }
};

} // namespace inet

#endif // LAMPSTATETAG_H
