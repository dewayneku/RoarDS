/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <algorithm>
#include <cmath>
#include <QKeyEvent>
#include <SDL2/SDL.h>

#include "Platform.h"
#include "SDL_gamecontroller.h"
#include "SDL_sensor.h"
#include "main.h"
#include "Config.h"

using namespace melonDS;

const char* EmuInstance::buttonNames[12] =
{
    "A",
    "B",
    "Select",
    "Start",
    "Right",
    "Left",
    "Up",
    "Down",
    "R",
    "L",
    "X",
    "Y"
};

const char* EmuInstance::hotkeyNames[HK_MAX] =
{
    "HK_Lid",
    "HK_Mic",
    "HK_Pause",
    "HK_Reset",
    "HK_FastForward",
    "HK_FrameLimitToggle",
    "HK_FullscreenToggle",
    "HK_SwapScreens",
    "HK_SwapScreenEmphasis",
    "HK_SolarSensorDecrease",
    "HK_SolarSensorIncrease",
    "HK_FrameStep",
    "HK_PowerButton",
    "HK_VolumeUp",
    "HK_VolumeDown",
    "HK_AudioMuteToggle",
    "HK_SlowMo",
    "HK_FastForwardToggle",
    "HK_SlowMoToggle",
    "HK_GuitarGripGreen",
    "HK_GuitarGripRed",
    "HK_GuitarGripYellow",
    "HK_GuitarGripBlue",
    "HK_StylusUp",
    "HK_StylusDown",
    "HK_StylusLeft",
    "HK_StylusRight",
    "HK_StylusTouch",

    "HK_SaveState1",
    "HK_SaveState2",
    "HK_SaveState3",
    "HK_SaveState4",
    "HK_SaveState5",
    "HK_SaveState6",
    "HK_SaveState7",
    "HK_SaveState8",
    "HK_SaveStateFile",
    "HK_LoadState1",
    "HK_LoadState2",
    "HK_LoadState3",
    "HK_LoadState4",
    "HK_LoadState5",
    "HK_LoadState6",
    "HK_LoadState7",
    "HK_LoadState8",
    "HK_LoadStateFile",
    "HK_UndoStateLoad",
    "HK_OpenROM",
    "HK_Quit",
    "HK_CycleScreenLayout",
    "HK_CycleScreenSizing"
};

std::shared_ptr<SDL_mutex> EmuInstance::joyMutexGlobal = nullptr;


void EmuInstance::inputInit()
{
    if (!joyMutexGlobal)
    {
        SDL_mutex* mutex = SDL_CreateMutex();
        joyMutexGlobal = std::shared_ptr<SDL_mutex>(mutex, SDL_DestroyMutex);
    }
    joyMutex = joyMutexGlobal;

    keyInputMask = 0xFFF;
    joyInputMask = 0xFFF;
    inputMask = 0xFFF;

    keyHotkeyMask = 0;
    keyHotkeyLatch = 0;
    joyHotkeyMask = 0;
    hotkeyMask = 0;
    lastHotkeyMask = 0;

    isTouching = false;
    touchX = 0;
    touchY = 0;

    joystick = nullptr;
    controller = nullptr;
    hasRumble = false;
    hasAccelerometer = false;
    hasGyroscope = false;
    isRumbling = false;

    inputLoadConfig();
}

void EmuInstance::inputDeInit()
{
    SDL_LockMutex(joyMutex.get());
    closeJoystick();
    SDL_UnlockMutex(joyMutex.get());
}

void EmuInstance::inputLoadConfig()
{
    SDL_LockMutex(joyMutex.get());

    Config::Table keycfg = localCfg.GetTable("Keyboard");
    Config::Table joycfg = localCfg.GetTable("Joystick");

    for (int i = 0; i < 12; i++)
        keyMapping[i] = keycfg.GetInt(buttonNames[i]);

    for (int i = 0; i < HK_MAX; i++)
        hkKeyMapping[i] = keycfg.GetInt(hotkeyNames[i]);

    // joystick mappings come from this instance's own table, unless a
    // controller profile applies (see applyJoyProfile)
    defaultJoyMapping = readJoyMappings(joycfg);

    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Table profiles = globalCfg.GetTable("ControllerProfiles");
    std::vector<std::string> profileNames = profiles.GetKeys();
    auto profileExists = [&](const std::string& name)
    {
        return std::find(profileNames.begin(), profileNames.end(), name) != profileNames.end();
    };

    instanceJoyProfile = localCfg.GetString("JoystickProfile");
    instanceJoyProfileMapping.clear();
    if (!instanceJoyProfile.empty())
    {
        if (profileExists(instanceJoyProfile))
        {
            Config::Table tbl = profiles.GetTable(instanceJoyProfile);
            instanceJoyProfileMapping = readJoyMappings(tbl);
        }
        else
            instanceJoyProfile = "";
    }

    linkedJoyProfiles.clear();
    Config::Table links = globalCfg.GetTable("ControllerLinks");
    for (const std::string& guid : links.GetKeys())
    {
        std::string name = links.GetString(guid);
        if (!profileExists(name)) continue;

        Config::Table tbl = profiles.GetTable(name);
        linkedJoyProfiles[guid] = {name, readJoyMappings(tbl)};
    }

    setJoystick(localCfg.GetInt("JoystickID"));
    SDL_UnlockMutex(joyMutex.get());
}

void EmuInstance::inputRumbleStart(melonDS::u32 len_ms)
{
    SDL_LockMutex(joyMutex.get());

    if (controller && hasRumble && !isRumbling)
    {
        SDL_GameControllerRumble(controller, 0xFFFF, 0xFFFF, len_ms);
        isRumbling = true;
    }

    SDL_UnlockMutex(joyMutex.get());
}

void EmuInstance::inputRumbleStop()
{
    SDL_LockMutex(joyMutex.get());

    if (controller && hasRumble && isRumbling)
    {
        SDL_GameControllerRumble(controller, 0, 0, 0);
        isRumbling = false;
    }

    SDL_UnlockMutex(joyMutex.get());
}

float EmuInstance::inputMotionQuery(melonDS::Platform::MotionQueryType type)
{
    float values[3];
    SDL_LockMutex(joyMutex.get());
    if (type <= melonDS::Platform::MotionAccelerationZ)
    {
        if (controller && hasAccelerometer)
        {
            if (SDL_GameControllerGetSensorData(controller, SDL_SENSOR_ACCEL, values, 3) == 0)
            {
                // Map values from DS console orientation to SDL controller orientation.
                SDL_UnlockMutex(joyMutex.get());
                switch (type)
                {
                case melonDS::Platform::MotionAccelerationX:
                    return values[0];
                case melonDS::Platform::MotionAccelerationY:
                    return -values[2];
                case melonDS::Platform::MotionAccelerationZ:
                    return values[1];
                default:
                    break;
                }
            }
        }
    }
    else if (type <= melonDS::Platform::MotionRotationZ)
    {
        if (controller && hasGyroscope)
        {
            if (SDL_GameControllerGetSensorData(controller, SDL_SENSOR_GYRO, values, 3) == 0)
            {
                // Map values from DS console orientation to SDL controller orientation.
                SDL_UnlockMutex(joyMutex.get());
                switch (type)
                {
                case melonDS::Platform::MotionRotationX:
                    return values[0];
                case melonDS::Platform::MotionRotationY:
                    return -values[2];
                case melonDS::Platform::MotionRotationZ:
                    return values[1];
                default:
                    break;
                }
            }
        }
    }
    SDL_UnlockMutex(joyMutex.get());
    if (type == melonDS::Platform::MotionAccelerationZ)
        return SDL_STANDARD_GRAVITY;
    return 0.0f;
}


std::vector<int> EmuInstance::readJoyMappings(Config::Table& table)
{
    std::vector<int> ret(kJoyMappingCount);
    for (int i = 0; i < 12; i++)
        ret[i] = table.GetInt(buttonNames[i]);
    for (int i = 0; i < HK_MAX; i++)
        ret[12 + i] = table.GetInt(hotkeyNames[i]);
    return ret;
}

void EmuInstance::writeJoyMappings(Config::Table& table, const std::vector<int>& mappings)
{
    for (int i = 0; i < 12; i++)
        table.SetInt(buttonNames[i], mappings[i]);
    for (int i = 0; i < HK_MAX; i++)
        table.SetInt(hotkeyNames[i], mappings[12 + i]);
}

std::string EmuInstance::joystickGUIDForIndex(int index)
{
    if (index < 0 || index >= SDL_NumJoysticks())
        return "";

    char buf[33];
    SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(index), buf, sizeof(buf));
    return buf;
}

std::string EmuInstance::getActiveJoyProfile()
{
    SDL_LockMutex(joyMutex.get());
    std::string ret = activeJoyProfile;
    SDL_UnlockMutex(joyMutex.get());
    return ret;
}

// pick the joystick mappings for the open controller: a profile linked to
// this controller type wins, then this instance's chosen profile, then the
// instance's own mappings. Called with joyMutex held.
void EmuInstance::applyJoyProfile()
{
    const std::vector<int>* mappings = &defaultJoyMapping;
    std::string name;

    if (joystick)
    {
        char buf[33];
        SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joystick), buf, sizeof(buf));
        auto it = linkedJoyProfiles.find(buf);
        if (it != linkedJoyProfiles.end())
        {
            name = it->second.first;
            mappings = &it->second.second;
        }
    }

    if (name.empty() && !instanceJoyProfileMapping.empty())
    {
        name = instanceJoyProfile;
        mappings = &instanceJoyProfileMapping;
    }

    if (mappings->size() == kJoyMappingCount)
    {
        for (int i = 0; i < 12; i++)
            joyMapping[i] = (*mappings)[i];
        for (int i = 0; i < HK_MAX; i++)
            hkJoyMapping[i] = (*mappings)[12 + i];
    }

    activeJoyProfile = name;
}

void EmuInstance::setJoystick(int id)
{
    SDL_LockMutex(joyMutex.get());
    joystickID = id;
    openJoystick();
    SDL_UnlockMutex(joyMutex.get());
}

void EmuInstance::openJoystick()
{
    closeJoystick();

    int num = SDL_NumJoysticks();
    if (num < 1)
    {
        controller = nullptr;
        joystick = nullptr;
        hasRumble = false;
        hasAccelerometer = false;
        hasGyroscope = false;
        applyJoyProfile();
        return;
    }

    if (joystickID >= num)
        joystickID = 0;

    joystick = SDL_JoystickOpen(joystickID);

    if (SDL_IsGameController(joystickID))
    {
        controller = SDL_GameControllerOpen(joystickID);
    }

    if (controller)
    {
        if (SDL_GameControllerHasRumble(controller))
        {
            hasRumble = true;
        }
        if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_ACCEL))
        {
            hasAccelerometer = SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_ACCEL, SDL_TRUE) == 0;
        }
        if (SDL_GameControllerHasSensor(controller, SDL_SENSOR_GYRO))
        {
            hasGyroscope = SDL_GameControllerSetSensorEnabled(controller, SDL_SENSOR_GYRO, SDL_TRUE) == 0;
        }
    }

    applyJoyProfile();
}

void EmuInstance::closeJoystick()
{
    if (controller)
    {
        SDL_GameControllerClose(controller);
        controller = nullptr;
        hasRumble = false;
        hasAccelerometer = false;
        hasGyroscope = false;
    }
    if (joystick)
    {
        SDL_JoystickClose(joystick);
        joystick = nullptr;
    }
}


// distinguish between left and right modifier keys (Ctrl, Alt, Shift)
// Qt provides no real cross-platform way to do this, so here we go
// for Windows and Linux we can distinguish via scancodes (but both
// provide different scancodes)
bool isRightModKey(QKeyEvent* event)
{
#ifdef __WIN32__
    quint32 scan = event->nativeScanCode();
    return (scan == 0x11D || scan == 0x138 || scan == 0x36);
#elif __APPLE__
    quint32 scan = event->nativeVirtualKey();
    return (scan == 0x36 || scan == 0x3C || scan == 0x3D || scan == 0x3E);
#else
    quint32 scan = event->nativeScanCode();
    return (scan == 0x69 || scan == 0x6C || scan == 0x3E);
#endif
}

int getEventKeyVal(QKeyEvent* event)
{
    int key = event->key();
    int mod = event->modifiers();
    bool ismod = (key == Qt::Key_Control ||
                  key == Qt::Key_Alt ||
                  key == Qt::Key_AltGr ||
                  key == Qt::Key_Shift ||
                  key == Qt::Key_Meta);

    if (!ismod)
        key |= mod;
    else if (isRightModKey(event))
        key |= (1<<31);

    return key;
}


void EmuInstance::onKeyPress(QKeyEvent* event)
{
    int keyHK = getEventKeyVal(event);
    int keyKP = keyHK;
    if (event->modifiers() != Qt::KeypadModifier)
        keyKP &= ~event->modifiers();

    for (int i = 0; i < 12; i++)
        if (keyKP == keyMapping[i])
            keyInputMask &= ~(1<<i);

    for (int i = 0; i < HK_MAX; i++)
        if (keyHK == hkKeyMapping[i])
        {
            keyHotkeyMask |= (1ULL<<i);
            keyHotkeyLatch.fetch_or(1ULL<<i);
        }
}

void EmuInstance::onKeyRelease(QKeyEvent* event)
{
    int keyHK = getEventKeyVal(event);
    int keyKP = keyHK;
    if (event->modifiers() != Qt::KeypadModifier)
        keyKP &= ~event->modifiers();

    for (int i = 0; i < 12; i++)
        if (keyKP == keyMapping[i])
            keyInputMask |= (1<<i);

    // Match hotkeys on the base key only: if a modifier is let go before
    // the key itself (e.g. Shift released before F1 for Shift+F1), the
    // release event no longer carries the modifier, and the hotkey would
    // otherwise stay stuck down and never trigger again.
    const int modmask = Qt::KeyboardModifierMask;
    for (int i = 0; i < HK_MAX; i++)
        if (hkKeyMapping[i] != -1 && (keyHK & ~modmask) == (hkKeyMapping[i] & ~modmask))
            keyHotkeyMask &= ~(1ULL<<i);
}

void EmuInstance::keyReleaseAll()
{
    keyInputMask = 0xFFF;
    keyHotkeyMask = 0;
    keyHotkeyLatch = 0;
}

bool EmuInstance::joystickButtonDown(int val)
{
    if (val == -1) return false;

    bool hasbtn = ((val & 0xFFFF) != 0xFFFF);

    if (hasbtn)
    {
        if (val & 0x100)
        {
            int hatnum = (val >> 4) & 0xF;
            int hatdir = val & 0xF;
            Uint8 hatval = SDL_JoystickGetHat(joystick, hatnum);

            bool pressed = false;
            if      (hatdir == 0x1) pressed = (hatval & SDL_HAT_UP);
            else if (hatdir == 0x4) pressed = (hatval & SDL_HAT_DOWN);
            else if (hatdir == 0x2) pressed = (hatval & SDL_HAT_RIGHT);
            else if (hatdir == 0x8) pressed = (hatval & SDL_HAT_LEFT);

            if (pressed) return true;
        }
        else
        {
            int btnnum = val & 0xFFFF;
            Uint8 btnval = SDL_JoystickGetButton(joystick, btnnum);

            if (btnval) return true;
        }
    }

    if (val & 0x10000)
    {
        int axisnum = (val >> 24) & 0xF;
        int axisdir = (val >> 20) & 0xF;
        Sint16 axisval = SDL_JoystickGetAxis(joystick, axisnum);

        switch (axisdir)
        {
            case 0: // positive
                if (axisval > 16384) return true;
                break;

            case 1: // negative
                if (axisval < -16384) return true;
                break;

            case 2: // trigger
                if (axisval > 0) return true;
                break;
        }
    }

    return false;
}

void EmuInstance::inputProcess()
{
    SDL_LockMutex(joyMutex.get());
    SDL_JoystickUpdate();

    if (joystick)
    {
        if (!SDL_JoystickGetAttached(joystick))
        {
            SDL_JoystickClose(joystick);
            joystick = nullptr;
        }
    }
    if (!joystick && (SDL_NumJoysticks() > 0))
    {
        openJoystick();
        if (joystick && !activeJoyProfile.empty())
            osdAddMessage(0, "Controller profile: %s", activeJoyProfile.c_str());
    }

    joyInputMask = 0xFFF;
    if (joystick)
    {
        for (int i = 0; i < 12; i++)
            if (joystickButtonDown(joyMapping[i]))
                joyInputMask &= ~(1 << i);
    }

    inputMask = keyInputMask & joyInputMask;

    joyHotkeyMask = 0;
    if (joystick)
    {
        for (int i = 0; i < HK_MAX; i++)
            if (joystickButtonDown(hkJoyMapping[i]))
                joyHotkeyMask |= (1ULL << i);
    }

    hotkeyMask = keyHotkeyMask | joyHotkeyMask;
    // a latched press counts even if the key was already released again
    // (the latch is only set on real key-down events; auto-repeat is filtered out)
    hotkeyPress = (hotkeyMask & ~lastHotkeyMask) | keyHotkeyLatch.exchange(0);
    hotkeyRelease = lastHotkeyMask & ~hotkeyMask;
    lastHotkeyMask = hotkeyMask;

    stylusProcess();
    SDL_UnlockMutex(joyMutex.get());
}

void EmuInstance::touchScreen(int x, int y)
{
    touchX = x;
    touchY = y;
    isTouching = true;

    // mouse/touch input moves the stylus cursor too, so switching between
    // the two doesn't make the cursor jump
    if (!stylusTouching)
    {
        stylusPosX = (float)x;
        stylusPosY = (float)y;
        stylusX = x;
        stylusY = y;
    }
}

// how strongly a stylus hotkey is held, 0..1. An analog stick axis gives a
// proportional value; keys, buttons and hats are all-or-nothing.
// Called with joyMutex held.
float EmuInstance::stylusInputAmount(int hotkey)
{
    float amount = 0.f;
    if (keyHotkeyMask & (1ULL << hotkey))
        amount = 1.f;

    int val = hkJoyMapping[hotkey];
    if (!joystick || val == -1)
        return amount;

    // button/hat part of the mapping
    if (joystickButtonDown(val & ~0x10000))
        amount = 1.f;

    // axis part
    if (val & 0x10000)
    {
        int axisnum = (val >> 24) & 0xF;
        int axisdir = (val >> 20) & 0xF;
        float axisval = SDL_JoystickGetAxis(joystick, axisnum) / 32767.f;

        float a = 0.f;
        if (axisdir == 0 || axisdir == 2) a = axisval;  // positive / trigger
        else if (axisdir == 1) a = -axisval;            // negative

        const float deadzone = 0.15f;
        a = (a - deadzone) / (1.f - deadzone);
        a = std::clamp(a, 0.f, 1.f);
        amount = std::max(amount, a);
    }

    return amount;
}

// move the stylus cursor and handle the stylus touch button.
// Called with joyMutex held, once per emu thread loop.
void EmuInstance::stylusProcess()
{
    melonDS::u64 now = SDL_GetTicks64();
    float dt = (stylusLastTick == 0) ? 0.f : (now - stylusLastTick) / 1000.f;
    stylusLastTick = now;
    if (dt > 0.1f) dt = 0.1f; // don't jump after a stall

    float dx = stylusInputAmount(HK_StylusRight) - stylusInputAmount(HK_StylusLeft);
    float dy = stylusInputAmount(HK_StylusDown) - stylusInputAmount(HK_StylusUp);

    bool moving = (dx != 0.f || dy != 0.f);
    if (moving)
    {
        // squared response: small tilts for precise aiming, full tilt crosses
        // the screen width in about a second
        const float speed = 256.f;
        float mag = std::min(1.f, std::sqrt(dx*dx + dy*dy));
        float scale = speed * mag * dt;

        stylusPosX = std::clamp(stylusPosX + dx * scale, 0.f, 255.f);
        stylusPosY = std::clamp(stylusPosY + dy * scale, 0.f, 191.f);
        stylusX = (int)stylusPosX;
        stylusY = (int)stylusPosY;
    }

    bool touchHeld = (hotkeyMask & (1ULL << HK_StylusTouch)) != 0;
    if (touchHeld)
    {
        stylusTouching = true;
        touchX = stylusX;
        touchY = stylusY;
        isTouching = true;
    }
    else if (stylusTouching)
    {
        stylusTouching = false;
        isTouching = false;
    }

    if (moving || touchHeld)
        stylusVisibleUntil = now + 2000;
}

bool EmuInstance::getStylusCursor(int& x, int& y)
{
    if (SDL_GetTicks64() >= stylusVisibleUntil)
        return false;

    x = stylusX;
    y = stylusY;
    return true;
}

void EmuInstance::releaseScreen()
{
    isTouching = false;
}
