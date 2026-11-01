#include "input_controller.h"

namespace nova {
uint32_t decode_calculator_keys(const uint16_t rows[8]) {
    // Verified against the Ndless SDK's CX touchpad-keyboard tpad_row/tpad_col.
    // In particular the old Clickpad arrow row is NOT a CX direction source.
    uint32_t buttons=0;
    if ((rows[6]&0x040)||(rows[1]&0x002)) buttons|=ButtonUp|ButtonThrust; // 8, W
    if ((rows[6]&0x010)||(rows[1]&0x040)) buttons|=ButtonDown; // 2, S
    if ((rows[2]&0x080)||(rows[4]&0x040)) buttons|=ButtonLeft; // 4, A
    if ((rows[2]&0x008)||(rows[4]&0x004)) buttons|=ButtonRight; // 6, D
    if (rows[0]&0x003) buttons|=ButtonConfirm; // Enter, Return
    if (rows[6]&0x080) buttons|=ButtonBack; // Esc
    if ((rows[5]&0x040)||(rows[7]&0x200)||(rows[0]&0x030)) buttons|=ButtonFire; // 5, Ctrl, Space, Z
    if ((rows[1]&0x001)||(rows[7]&0x100)) buttons|=ButtonThrust; // X, Shift
    if (rows[2]&0x004) buttons|=ButtonPause; // P alias
    if (rows[6]&0x028) buttons|=ButtonMenu; // Menu, Doc
    if ((rows[3]&0x080)||(rows[6]&0x200)) buttons|=ButtonTab; // 7, Tab
    if ((rows[5]&0x004)||(rows[2]&0x001)) buttons|=ButtonReroll; // Minus, R
    if ((rows[3]&0x008)||(rows[1]&0x010)) buttons|=ButtonUpgrade; // 9, U
    if (rows[0]&0x080) buttons|=ButtonPointerMode; // 0
    if (rows[5]&0x200) buttons|=ButtonBanish; // Del
    return buttons;
}

static uint32_t pad_button(int x,int y) {
    // Same thirds as Ndless touchpad_getarrow(), after conversion to screen
    // coordinates. Latch the sector for the whole press: sliding a held rim
    // click through the center must never turn it into an accidental Confirm.
    uint32_t buttons=0;
    if (x<=ScreenWidth/3) buttons|=ButtonLeft;
    else if (x>=ScreenWidth*2/3) buttons|=ButtonRight;
    if (y<=ScreenHeight/3) buttons|=ButtonUp|ButtonThrust;
    else if (y>=ScreenHeight*2/3) buttons|=ButtonDown;
    return buttons?buttons:ButtonConfirm;
}

void InputController::reset() {
    bindings_=KeyBindings{};capturing_=false;rebind_status_=RebindStatus::Idle;capture_action_=BindThrust;
    for(unsigned i=0;i<3;++i)previous_physical_[i]=blocked_physical_[i]=0;
    initialized_=false;pointer_mode_=false;
    pointer_activity_=pointer_steering_=false;
    previous_pointer_=previous_raw_pointer_=previous_raw_contact_=false;
    blocked_pointer_=blocked_contact_=false;
    context_=previous_buttons_=previous_raw_buttons_=blocked_buttons_=pad_buttons_=0;
}

void InputController::setPointerMode(bool enabled) {
    if (enabled==pointer_mode_) return;
    pointer_mode_=enabled;blocked_pointer_=previous_raw_pointer_;
    blocked_contact_=previous_raw_contact_;pad_buttons_=0;
    pointer_activity_=pointer_steering_=false;
}

bool InputController::setBindings(const KeyBindings& bindings) {
    if(!valid_bindings(bindings))return false;
    bindings_=bindings;
    for(unsigned i=0;i<3;++i)blocked_physical_[i]|=previous_physical_[i];
    blocked_buttons_|=previous_raw_buttons_;blocked_pointer_=previous_raw_pointer_;
    blocked_contact_=previous_raw_contact_;return true;
}
void InputController::resetBindings() { (void)setBindings(KeyBindings{}); }
bool InputController::loadBindings() {
    KeyBindings loaded;if(!load_key_bindings(loaded))return false;return setBindings(loaded);
}
bool InputController::saveBindings() const {return save_key_bindings(bindings_);}
bool InputController::beginRebind(BindAction action) {
    if(action>=BindCount)return false;
    capture_action_=action;capturing_=true;rebind_status_=RebindStatus::Waiting;
    for(unsigned i=0;i<3;++i)blocked_physical_[i]|=previous_physical_[i];
    blocked_buttons_|=previous_raw_buttons_;blocked_pointer_=previous_raw_pointer_;
    blocked_contact_=previous_raw_contact_;return true;
}

Input InputController::update(const Input& raw,uint32_t context,bool gameplay) {
    const bool first=!initialized_,changed=initialized_&&context_!=context;
    const bool pad_down=raw.pointer_down&&raw.pointer_present;
    if(first) {
        initialized_=true;blocked_buttons_=raw.held;
        blocked_pointer_=pad_down;blocked_contact_=raw.pointer_present;
    } else if(changed) {
        blocked_buttons_|=previous_raw_buttons_&raw.held;
        blocked_pointer_=blocked_pointer_||previous_raw_pointer_;pad_buttons_=0;
    }
    context_=context;
    uint32_t physical[3];
    for(unsigned i=0;i<3;++i) {
        if(first)blocked_physical_[i]|=raw.physical_keys[i];
        else if(changed)blocked_physical_[i]|=previous_physical_[i]&raw.physical_keys[i];
        blocked_physical_[i]&=raw.physical_keys[i];
        physical[i]=raw.physical_keys[i]&~blocked_physical_[i];
    }
    blocked_buttons_&=raw.held;
    if(!pad_down)blocked_pointer_=false;
    if(!raw.pointer_present&&!raw.pointer_down)blocked_contact_=false;
    uint32_t buttons=raw.physical_valid?bound_buttons(physical,bindings_,gameplay):(raw.held&~blocked_buttons_);

    if(capturing_) {
        if((raw.physical_valid&&physical_down(physical,KeyEscape)&&!physical_down(previous_physical_,KeyEscape))||
           (!raw.physical_valid&&(buttons&ButtonBack)&&!(previous_raw_buttons_&ButtonBack))) {
            capturing_=false;rebind_status_=RebindStatus::Cancelled;
        } else if(raw.physical_valid) {
            for(uint8_t key=1;key<=92;++key)if(physical_down(physical,key)&&!physical_down(previous_physical_,key)) {
                if(!binding_key_allowed(capture_action_,key))rebind_status_=RebindStatus::Reserved;
                else {
                    bool conflict=false;
                    for(unsigned i=0;i<BindCount;++i)if(i!=capture_action_&&bindings_.key[i]==key)conflict=true;
                    if(conflict)rebind_status_=RebindStatus::Conflict;
                    else {bindings_.key[capture_action_]=key;capturing_=false;rebind_status_=RebindStatus::Applied;}
                }
                break;
            }
        }
        // The key just captured is consumed and must be released before it can
        // operate either the binding list or the page opened after confirmation.
        if(!capturing_) {
            for(unsigned i=0;i<3;++i)blocked_physical_[i]|=raw.physical_keys[i];
            blocked_buttons_|=raw.held;blocked_pointer_=pad_down;blocked_contact_=raw.pointer_present;
        }
        Input out=raw;out.held=out.pressed=out.released=0;
        out.pointer_present=out.pointer_down=out.pointer_pressed=out.pointer_released=false;
        pointer_activity_=pointer_steering_=false;previous_buttons_=0;previous_pointer_=false;
        previous_raw_buttons_=raw.held;previous_raw_pointer_=pad_down;previous_raw_contact_=raw.pointer_present;
        for(unsigned i=0;i<3;++i)previous_physical_[i]=raw.physical_keys[i];
        return out;
    }

    const bool mode_press=(buttons&ButtonPointerMode)&&!(raw.physical_valid?physical_down(previous_physical_,Key0):(previous_raw_buttons_&ButtonPointerMode)!=0);
    if(mode_press) {
        pointer_mode_=!pointer_mode_;blocked_pointer_=pad_down;
        blocked_contact_=raw.pointer_present;pad_buttons_=0;
        // A source-mode switch is an explicit input boundary just like a page
        // change. Held fire/thrust cannot carry through until physically lifted.
        for(unsigned i=0;i<3;++i)blocked_physical_[i]|=raw.physical_keys[i];
        blocked_buttons_|=raw.held;buttons=0;
    }
    // Numeric navigation has no direct-purchase aliases, including legacy
    // host event masks. Only an explicit Confirm or fresh pointer click buys.
    buttons&=~(ButtonPointerMode|Button1|Button2|Button3);
    const bool pointer_allowed=!blocked_pointer_&&!blocked_contact_;
    const bool active_down=pad_down&&pointer_allowed;
    if(!pad_down)pad_buttons_=0;
    if(!pointer_mode_&&raw.pointer_is_touchpad&&active_down) {
        if(!previous_raw_pointer_)pad_buttons_=pad_button(raw.pointer_x,raw.pointer_y);
        // Physical keys own each direction axis while held. A resting/held rim
        // click must not oppose a deliberate numeric key, but an independent
        // pad axis (and numeric Fire) can still be used at the same time.
        uint32_t pad=pad_buttons_;
        if(buttons&(ButtonLeft|ButtonRight)) {
            pad&=~(ButtonLeft|ButtonRight);
            if(!gameplay)pad_buttons_&=~(ButtonLeft|ButtonRight);
        }
        if(buttons&(ButtonUp|ButtonThrust|ButtonDown)) {
            pad&=~(ButtonUp|ButtonThrust|ButtonDown);
            if(!gameplay)pad_buttons_&=~(ButtonUp|ButtonThrust|ButtonDown);
        }
        // Menu navigation must never simultaneously accept the selected item
        // because the pad center was already being pressed.
        if(buttons&(ButtonUp|ButtonThrust|ButtonDown|ButtonLeft|ButtonRight)) {
            pad&=~ButtonConfirm;pad_buttons_&=~ButtonConfirm;
        }
        buttons|=pad;
    }
    Input out=raw;out.held=buttons;out.pressed=buttons&~previous_buttons_;out.released=previous_buttons_&~buttons;
    out.pointer_present=pointer_mode_&&raw.pointer_present&&pointer_allowed;
    out.pointer_down=pointer_mode_&&active_down;
    out.pointer_pressed=out.pointer_down&&!previous_pointer_&&!out.pressed;
    out.pointer_released=!out.pointer_down&&previous_pointer_;
    pointer_activity_=out.pointer_pressed;pointer_steering_=out.pointer_present&&!(buttons&(ButtonLeft|ButtonRight));
    previous_buttons_=buttons;previous_pointer_=out.pointer_down;
    previous_raw_buttons_=raw.held;previous_raw_pointer_=pad_down;previous_raw_contact_=raw.pointer_present;
    for(unsigned i=0;i<3;++i)previous_physical_[i]=raw.physical_keys[i];
    return out;
}
}
