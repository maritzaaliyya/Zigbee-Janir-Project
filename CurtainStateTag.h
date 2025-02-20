#ifndef CURTAINSTATETAG_H
#define CURTAINSTATETAG_H

#include "inet/common/TagBase_m.h"

namespace inet {

class INET_API CurtainStateTag : public TagBase
{
  protected:
    bool isOn = false;

  public:
    CurtainStateTag() {}
    virtual ~CurtainStateTag() {}

    void setIsOn(bool on) { isOn = on; }
    bool getIsOn() const { return isOn; }

    virtual CurtainStateTag *dup() const override { return new CurtainStateTag(*this); }
};

} // namespace inet

#endif
