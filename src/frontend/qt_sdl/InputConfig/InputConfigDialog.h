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

#ifndef INPUTCONFIGDIALOG_H
#define INPUTCONFIGDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "Config.h"
#include "EmuInstance.h"

static constexpr int keypad_num = 12;

static constexpr std::initializer_list<int> hk_addons =
{
    HK_SolarSensorIncrease,
    HK_SolarSensorDecrease,
    HK_GuitarGripGreen,
    HK_GuitarGripRed,
    HK_GuitarGripYellow,
    HK_GuitarGripBlue,
};

static constexpr std::initializer_list<const char*> hk_addons_labels =
{
    "[Boktai] Sunlight + ",
    "[Boktai] Sunlight - ",
    "[Guitar Grip] Green",
    "[Guitar Grip] Red",
    "[Guitar Grip] Yellow",
    "[Guitar Grip] Blue",
};

static_assert(hk_addons.size() == hk_addons_labels.size());

static constexpr std::initializer_list<int> hk_general =
{
    HK_Pause,
    HK_Reset,
    HK_FrameStep,
    HK_FastForward,
    HK_FastForwardToggle,
    HK_SlowMo,
    HK_SlowMoToggle,
    HK_FrameLimitToggle,
    HK_FullscreenToggle,
    HK_Lid,
    HK_Mic,
    HK_SwapScreens,
    HK_SwapScreenEmphasis,
    HK_PowerButton,
    HK_VolumeUp,
    HK_VolumeDown,
    HK_AudioMuteToggle,
    HK_CycleScreenLayout,
    HK_CycleScreenSizing
};

static constexpr std::initializer_list<const char*> hk_general_labels =
{
    "Pause/resume",
    "Reset",
    "Frame step",
    "Fast forward",
    "Toggle fast forward",
    "Slow mo",
    "Toggle slow mo",
    "Toggle FPS limit",
    "Toggle fullscreen",
    "Close/open lid",
    "Microphone",
    "Swap screens",
    "Swap screen emphasis",
    "DSi Power button",
    "DSi Volume up",
    "DSi Volume down",
    "Toggle audio mute",
    "Cycle screen layout",
    "Cycle screen sizing"
};

static_assert(hk_general.size() == hk_general_labels.size());

static constexpr std::initializer_list<int> hk_menu =
{
    HK_SaveState1, HK_SaveState2, HK_SaveState3, HK_SaveState4,
    HK_SaveState5, HK_SaveState6, HK_SaveState7, HK_SaveState8,
    HK_SaveStateFile,
    HK_LoadState1, HK_LoadState2, HK_LoadState3, HK_LoadState4,
    HK_LoadState5, HK_LoadState6, HK_LoadState7, HK_LoadState8,
    HK_LoadStateFile,
    HK_UndoStateLoad,
    HK_OpenROM,
    HK_Quit
};

static constexpr std::initializer_list<const char*> hk_menu_labels =
{
    "Save state 1", "Save state 2", "Save state 3", "Save state 4",
    "Save state 5", "Save state 6", "Save state 7", "Save state 8",
    "Save state to file",
    "Load state 1", "Load state 2", "Load state 3", "Load state 4",
    "Load state 5", "Load state 6", "Load state 7", "Load state 8",
    "Load state from file",
    "Undo state load",
    "Open ROM",
    "Quit"
};

static_assert(hk_menu.size() == hk_menu_labels.size());


namespace Ui { class InputConfigDialog; }
class InputConfigDialog;

class InputConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputConfigDialog(QWidget* parent);
    ~InputConfigDialog();

    SDL_Joystick* getJoystick();
    std::shared_ptr<SDL_mutex> getJoyMutex();

    static InputConfigDialog* currentDlg;
    static InputConfigDialog* openDlg(QWidget* parent)
    {
        if (currentDlg)
        {
            currentDlg->activateWindow();
            return currentDlg;
        }

        currentDlg = new InputConfigDialog(parent);
        currentDlg->open();
        return currentDlg;
    }
    static void closeDlg()
    {
        currentDlg = nullptr;
    }

private slots:
    void on_InputConfigDialog_accepted();
    void on_InputConfigDialog_rejected();

    void on_btnKeyMapSwitch_clicked();
    void on_btnJoyMapSwitch_clicked();
    void on_cbxJoystick_currentIndexChanged(int id);

    void onProfileChanged(int index);
    void onProfileSaveAs();
    void onProfileDelete();
    void onProfileLinkToggled(bool checked);

private:
    void setupProfileUI();
    std::vector<int> gatherJoyMappings();
    void scatterJoyMappings(const std::vector<int>& mappings);
    void showProfile(const std::string& name);
    void updateProfileControls();
    std::string profileForController(const std::string& guid);
    std::string currentJoystickGUID();

    void populatePage(QWidget* page,
        const std::initializer_list<const char*>& labels,
        int* keymap, int* joymap);
    void setupKeypadPage();

    Ui::InputConfigDialog* ui;

    EmuInstance* emuInstance;

    int keypadKeyMap[12], keypadJoyMap[12];
    int addonsKeyMap[hk_addons.size()], addonsJoyMap[hk_addons.size()];
    int hkGeneralKeyMap[hk_general.size()], hkGeneralJoyMap[hk_general.size()];
    int hkMenuKeyMap[hk_menu.size()], hkMenuJoyMap[hk_menu.size()];
    int joystickID;

    // controller profiles, edited in memory and written out on OK.
    // the "" entry is this instance's own joystick mappings.
    QComboBox* cbxProfile = nullptr;
    QPushButton* btnProfileSaveAs = nullptr;
    QPushButton* btnProfileDelete = nullptr;
    QCheckBox* chkProfileLink = nullptr;
    std::vector<class JoyMapButton*> joyButtons;
    std::map<std::string, std::vector<int>> workingProfiles;
    std::map<std::string, std::vector<int>> savedProfiles;
    std::set<std::string> deletedProfiles;
    std::map<std::string, std::string> workingLinks; // controller GUID -> profile
    std::string instanceProfile;
    std::string shownProfile;
    bool updatingProfileUI = false;
};


#endif // INPUTCONFIGDIALOG_H
