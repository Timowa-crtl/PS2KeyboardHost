#include "ps2_Keyboard.h"
#include "ps2_SimpleDiagnostics.h"
#include "ps2_UsbTranslator.h"
#include "HID-Project.h"

class Diagnostics : public ps2::SimpleDiagnostics<512, 60>
{
    typedef ps2::SimpleDiagnostics<512, 60> base;

    enum class UsbTranslatorAppCode : uint8_t {
        sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
        sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
    };

public:
    void sentUsbKeyDown(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyDown, b); }
    void sentUsbKeyUp(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyUp, b); }
};

static Diagnostics diagnostics;
static ps2::UsbTranslator<Diagnostics> keyMapping(diagnostics);
static ps2::Keyboard<4,2,1, Diagnostics> ps2Keyboard(diagnostics);
static ps2::UsbKeyboardLeds ledValueLastSentToPs2 = ps2::UsbKeyboardLeds::none;

void setup() {
    ps2Keyboard.begin();
    BootKeyboard.begin();
}

void loop() {
    ps2::UsbKeyboardLeds newLedState = (ps2::UsbKeyboardLeds)BootKeyboard.getLeds();
    if (newLedState != ledValueLastSentToPs2)
    {
        ps2Keyboard.sendLedStatus(keyMapping.translateLeds(newLedState));
        ledValueLastSentToPs2 = newLedState;
    }

    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode != ps2::KeyboardOutput::none && scanCode != ps2::KeyboardOutput::garbled)
    {
        ps2::UsbKeyAction action = keyMapping.translatePs2Keycode(scanCode);
        KeyboardKeycode hidCode = (KeyboardKeycode)action.hidCode;

        switch (action.gesture) {
            case ps2::UsbKeyAction::KeyDown:
                BootKeyboard.press(hidCode);
                break;
            case ps2::UsbKeyAction::KeyUp:
                BootKeyboard.release(hidCode);
                break;
        }
    }
}
