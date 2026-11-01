#pragma once
#include "platform.h"
#include "input_bindings.h"

namespace nova {
// Decode the CX/CX II touchpad-keyboard matrix at 0x900e0010..0x900e001e.
// Touchpad directions are I2C events, never keyboard-row aliases.
uint32_t decode_calculator_keys(const uint16_t rows[8]);

// Explicit input ownership. The default is the calculator keypad; 0 switches
// touch aiming on/off. A page/state change consumes held controls until release.
// Give each application page + gameplay state a distinct context value.
class InputController {
public:
    void reset();
    Input update(const Input& raw, uint32_t context, bool gameplay=false);
    bool beginRebind(BindAction action);
    bool capturingBinding() const { return capturing_; }
    RebindStatus rebindStatus() const { return rebind_status_; }
    void clearRebindStatus() { if(!capturing_)rebind_status_=RebindStatus::Idle; }
    BindAction captureAction() const { return capture_action_; }
    const KeyBindings& bindings() const { return bindings_; }
    bool setBindings(const KeyBindings& bindings);
    void resetBindings();
    bool loadBindings();
    bool saveBindings() const;
    void setPointerMode(bool enabled);
    bool pointerMode() const { return pointer_mode_; }
    bool pointerActivity() const { return pointer_activity_; }
    bool pointerSteering() const { return pointer_steering_; }
private:
    KeyBindings bindings_{};
    uint32_t previous_physical_[3]={},blocked_physical_[3]={};
    BindAction capture_action_=BindThrust;
    RebindStatus rebind_status_=RebindStatus::Idle;
    bool capturing_=false;
    bool initialized_=false, pointer_mode_=false;
    bool pointer_activity_=false, pointer_steering_=false;
    bool previous_pointer_=false, previous_raw_pointer_=false, previous_raw_contact_=false;
    bool blocked_pointer_=false, blocked_contact_=false;
    uint32_t context_=0, previous_buttons_=0, previous_raw_buttons_=0;
    uint32_t blocked_buttons_=0, pad_buttons_=0;
};
}
