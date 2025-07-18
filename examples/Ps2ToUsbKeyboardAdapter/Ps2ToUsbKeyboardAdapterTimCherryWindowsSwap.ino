/*
Copyright (C) 2017 Steve Benz <s8878992@hotmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
USA
*/
#include "ps2_Keyboard.h"
#include "ps2_SimpleDiagnostics.h"
#include "ps2_UsbTranslator.h"
// This sketch requires you to have the "HIDProject" Arduino library installed.
#include "HID-Project.h"

// Create a log of all the data going to and from the keyboard and the host.
class Diagnostics
    : public ps2::SimpleDiagnostics<512, 60>
{
    typedef ps2::SimpleDiagnostics<512, 60> base;

    enum class UsbTranslatorAppCode : uint8_t {
        // no errors

        sentUsbKeyDown = 0 + base::firstUnusedInfoCode,
        sentUsbKeyUp = 1 + base::firstUnusedInfoCode,
    };

public:
    void sentUsbKeyDown(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyDown, b); }
    void sentUsbKeyUp(byte b) { this->push(UsbTranslatorAppCode::sentUsbKeyUp, b); }
};

static Diagnostics diagnostics;
static ps2::UsbTranslator<Diagnostics> keyMapping(diagnostics);
// Using Pin 4 isstead of 3 in OriginalExample
static ps2::Keyboard<4,2,1, Diagnostics> ps2Keyboard(diagnostics);
static ps2::UsbKeyboardLeds ledValueLastSentToPs2 = ps2::UsbKeyboardLeds::none;

// This example demonstrates how to create keyboard translations (in this case, how the caps lock
//  and other modifier keys are laid out).  The example reads switch1Pin to toggle whether this
//  behavior is on or off.

// the setup function runs once when you press reset or power the board
void setup() {

    ps2Keyboard.begin();
    //activated this line after having issues with hotplugging, it seems to help!
    ps2Keyboard.awaitStartup();

    BootKeyboard.begin();
}

static KeyboardKeycode noTranslation(KeyboardKeycode usbKeystroke)
{
    return usbKeystroke;
}


static KeyboardKeycode capsLockToLeftGui(KeyboardKeycode usbKeystroke)
{
    switch (usbKeystroke)
    {
    case HID_KEYBOARD_CAPS_LOCK:
        return KEY_LEFT_GUI;

    default:
        return usbKeystroke;
    }
}


void loop() {
    ps2::UsbKeyboardLeds newLedState = (ps2::UsbKeyboardLeds)BootKeyboard.getLeds();
    if (newLedState != ledValueLastSentToPs2)
    {
        ps2Keyboard.sendLedStatus(keyMapping.translateLeds(newLedState));
        ledValueLastSentToPs2 = newLedState;
    }


    // On many Arduino's, pin 13 is connected to an on-board LED.  On the Pro-Micro, which this is
    //  developed on, there isn't a dedicated user-facing LED, but you can piggy-back on the TX & RX lights.
    diagnostics.setLedIndicator<LED_BUILTIN_RX, ps2::DiagnosticsLedBlink::blinkOnError>();

    ps2::KeyboardOutput scanCode = ps2Keyboard.readScanCode();
    if (scanCode != ps2::KeyboardOutput::none && scanCode != ps2::KeyboardOutput::garbled)
    {
        ps2::UsbKeyAction action = keyMapping.translatePs2Keycode(scanCode);
        KeyboardKeycode hidCode = (KeyboardKeycode)action.hidCode;

        hidCode = capsLockToLeftGui(hidCode);

        switch (action.gesture) {
            case ps2::UsbKeyAction::KeyDown:
                if (hidCode == KeyboardKeycode::KEY_SCROLL_LOCK) {
                    // Hide this keypress.
                }
                else {
                    diagnostics.sentUsbKeyDown(hidCode);
                    BootKeyboard.press(hidCode);
                }
                break;
            case ps2::UsbKeyAction::KeyUp:
                if (hidCode == KeyboardKeycode::KEY_SCROLL_LOCK) {
                    // Real use-cases for using the Scroll Lock key are thin on the ground, so hijacking
                    //  it for diagnostics.  Ideally, if you can spare a button or some other external
                    //  signal, that'd be better.  Doing it on keyup so that there shouldn't be any PS2
                    //  activity while we're pumping out the report.
                    diagnostics.sendReport(BootKeyboard);
                    diagnostics.reset();
                }
                else {
                    diagnostics.sentUsbKeyUp(hidCode);
                    BootKeyboard.release(hidCode);
                }
                break;
        }
    }
}
