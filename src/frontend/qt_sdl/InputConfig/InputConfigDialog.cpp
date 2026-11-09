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

#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QKeyEvent>
#include <QDebug>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>

#include <SDL2/SDL.h>

#include "types.h"
#include "Platform.h"

#include "InputConfigDialog.h"
#include "ui_InputConfigDialog.h"
#include "MapButton.h"


using namespace melonDS;
InputConfigDialog* InputConfigDialog::currentDlg = nullptr;

const int dskeyorder[12] = {0, 1, 10, 11, 5, 4, 6, 7, 9, 8, 2, 3};
const char* dskeylabels[12] = {"A", "B", "X", "Y", "Left", "Right", "Up", "Down", "L", "R", "Select", "Start"};

InputConfigDialog::InputConfigDialog(QWidget* parent) : QDialog(parent), ui(new Ui::InputConfigDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    emuInstance = ((MainWindow*)parent)->getEmuInstance();

    Config::Table& instcfg = emuInstance->getLocalConfig();
    Config::Table keycfg = instcfg.GetTable("Keyboard");
    Config::Table joycfg = instcfg.GetTable("Joystick");

    for (int i = 0; i < keypad_num; i++)
    {
        const char* btn = EmuInstance::buttonNames[dskeyorder[i]];
        keypadKeyMap[i] = keycfg.GetInt(btn);
        keypadJoyMap[i] = joycfg.GetInt(btn);
    }

    int i = 0;
    for (int hotkey : hk_addons)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        addonsKeyMap[i] = keycfg.GetInt(btn);
        addonsJoyMap[i] = joycfg.GetInt(btn);
        i++;
    }

    i = 0;
    for (int hotkey : hk_general)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        hkGeneralKeyMap[i] = keycfg.GetInt(btn);
        hkGeneralJoyMap[i] = joycfg.GetInt(btn);
        i++;
    }

    i = 0;
    for (int hotkey : hk_menu)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        hkMenuKeyMap[i] = keycfg.GetInt(btn);
        hkMenuJoyMap[i] = joycfg.GetInt(btn);
        i++;
    }

    populatePage(ui->tabAddons, hk_addons_labels, addonsKeyMap, addonsJoyMap);
    populatePage(ui->tabHotkeysGeneral, hk_general_labels, hkGeneralKeyMap, hkGeneralJoyMap);

    // save state / menu shortcuts get their own tab, scrollable since the list is long
    {
        QWidget* menuPage = new QWidget();
        populatePage(menuPage, hk_menu_labels, hkMenuKeyMap, hkMenuJoyMap);

        QScrollArea* scroll = new QScrollArea();
        scroll->setWidget(menuPage);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        ui->tabWidget->addTab(scroll, "Save states && menu");
    }

    joystickID = instcfg.GetInt("JoystickID");

    int njoy = SDL_NumJoysticks();
    if (njoy > 0)
    {
        for (int i = 0; i < njoy; i++)
        {
            const char* name = SDL_JoystickNameForIndex(i);
            ui->cbxJoystick->addItem(QString(name));
        }
        ui->cbxJoystick->setCurrentIndex(joystickID);
    }
    else
    {
        ui->cbxJoystick->addItem("(no joysticks available)");
        ui->cbxJoystick->setEnabled(false);
    }

    setupKeypadPage();
    setupProfileUI();

    int inst = emuInstance->getInstanceID();
    if (inst > 0)
        ui->lblInstanceNum->setText(QString("Configuring mappings for instance %1").arg(inst+1));
    else
        ui->lblInstanceNum->hide();
}

InputConfigDialog::~InputConfigDialog()
{
    delete ui;
}

void InputConfigDialog::setupKeypadPage()
{
    for (int i = 0; i < keypad_num; i++)
    {
        QPushButton* pushButtonKey = this->findChild<QPushButton*>(QStringLiteral("btnKey") + dskeylabels[i]);
        QPushButton* pushButtonJoy = this->findChild<QPushButton*>(QStringLiteral("btnJoy") + dskeylabels[i]);

        KeyMapButton* keyMapButtonKey = new KeyMapButton(&keypadKeyMap[i], false);
        JoyMapButton* keyMapButtonJoy = new JoyMapButton(&keypadJoyMap[i], false);
        joyButtons.push_back(keyMapButtonJoy);

        pushButtonKey->parentWidget()->layout()->replaceWidget(pushButtonKey, keyMapButtonKey);
        pushButtonJoy->parentWidget()->layout()->replaceWidget(pushButtonJoy, keyMapButtonJoy);

        delete pushButtonKey;
        delete pushButtonJoy;

        if (ui->cbxJoystick->isEnabled())
        {
            ui->stackMapping->setCurrentIndex(1);
        }
    }
}

void InputConfigDialog::populatePage(QWidget* page,
    const std::initializer_list<const char*>& labels,
    int* keymap, int* joymap)
{
    // kind of a hack
    bool ishotkey = (page != ui->tabInput);

    QHBoxLayout* main_layout = new QHBoxLayout();

    QGroupBox* group;
    QGridLayout* group_layout;

    group = new QGroupBox("Keyboard mappings:");
    main_layout->addWidget(group);
    group_layout = new QGridLayout();
    group_layout->setSpacing(1);
    int i = 0;
    for (const char* labelStr : labels)
    {
        QLabel* label = new QLabel(QString(labelStr)+":");
        KeyMapButton* btn = new KeyMapButton(&keymap[i], ishotkey);

        group_layout->addWidget(label, i, 0);
        group_layout->addWidget(btn, i, 1);
        i++;
    }
    group_layout->setRowStretch(labels.size(), 1);
    group->setLayout(group_layout);
    group->setMinimumWidth(275);

    group = new QGroupBox("Joystick mappings:");
    main_layout->addWidget(group);
    group_layout = new QGridLayout();
    group_layout->setSpacing(1);
    i = 0;
    for (const char* labelStr : labels)
    {
        QLabel* label = new QLabel(QString(labelStr)+":");
        JoyMapButton* btn = new JoyMapButton(&joymap[i], ishotkey);
        joyButtons.push_back(btn);

        group_layout->addWidget(label, i, 0);
        group_layout->addWidget(btn, i, 1);
        i++;
    }
    group_layout->setRowStretch(labels.size(), 1);
    group->setLayout(group_layout);
    group->setMinimumWidth(275);

    page->setLayout(main_layout);
}

void InputConfigDialog::on_InputConfigDialog_accepted()
{
    // the instance's own joystick mappings are what the "" entry holds;
    // put them back in the arrays so they get saved below
    workingProfiles[shownProfile] = gatherJoyMappings();
    scatterJoyMappings(workingProfiles[""]);

    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Table profiles = globalCfg.GetTable("ControllerProfiles");
    for (const std::string& name : deletedProfiles)
        profiles.Erase(name);
    for (auto& [name, mappings] : workingProfiles)
    {
        if (name.empty()) continue;
        Config::Table tbl = profiles.GetTable(name);
        EmuInstance::writeJoyMappings(tbl, mappings);
    }

    Config::Table links = globalCfg.GetTable("ControllerLinks");
    for (const std::string& guid : links.GetKeys())
        links.Erase(guid);
    for (auto& [guid, name] : workingLinks)
        links.SetString(guid, name);

    Config::Table& instcfg = emuInstance->getLocalConfig();
    instcfg.SetString("JoystickProfile", instanceProfile);
    Config::Table keycfg = instcfg.GetTable("Keyboard");
    Config::Table joycfg = instcfg.GetTable("Joystick");

    for (int i = 0; i < keypad_num; i++)
    {
        const char* btn = EmuInstance::buttonNames[dskeyorder[i]];
        keycfg.SetInt(btn, keypadKeyMap[i]);
        joycfg.SetInt(btn, keypadJoyMap[i]);
    }

    int i = 0;
    for (int hotkey : hk_addons)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        keycfg.SetInt(btn, addonsKeyMap[i]);
        joycfg.SetInt(btn, addonsJoyMap[i]);
        i++;
    }

    i = 0;
    for (int hotkey : hk_general)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        keycfg.SetInt(btn, hkGeneralKeyMap[i]);
        joycfg.SetInt(btn, hkGeneralJoyMap[i]);
        i++;
    }

    i = 0;
    for (int hotkey : hk_menu)
    {
        const char* btn = EmuInstance::hotkeyNames[hotkey];
        keycfg.SetInt(btn, hkMenuKeyMap[i]);
        joycfg.SetInt(btn, hkMenuJoyMap[i]);
        i++;
    }

    instcfg.SetInt("JoystickID", joystickID);
    Config::Save();

    emuInstance->inputLoadConfig();

    closeDlg();
}

void InputConfigDialog::on_InputConfigDialog_rejected()
{
    Config::Table& instcfg = emuInstance->getLocalConfig();
    emuInstance->setJoystick(instcfg.GetInt("JoystickID"));

    closeDlg();
}

void InputConfigDialog::on_btnKeyMapSwitch_clicked()
{
    ui->stackMapping->setCurrentIndex(0);
}

void InputConfigDialog::on_btnJoyMapSwitch_clicked()
{
    ui->stackMapping->setCurrentIndex(1);
}

void InputConfigDialog::on_cbxJoystick_currentIndexChanged(int id)
{
    // prevent a spurious change
    if (ui->cbxJoystick->count() < 2) return;

    joystickID = id;
    emuInstance->setJoystick(id);

    // a different controller may have its own profile
    if (!cbxProfile) return; // still being set up
    workingProfiles[shownProfile] = gatherJoyMappings();
    showProfile(profileForController(currentJoystickGUID()));
}

static const char* kDefaultProfileLabel = "(this instance's own mappings)";

void InputConfigDialog::setupProfileUI()
{
    QHBoxLayout* row = new QHBoxLayout();
    QLabel* label = new QLabel("Controller profile:");
    cbxProfile = new QComboBox();
    cbxProfile->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    cbxProfile->setToolTip("Named sets of joystick mappings, shared by all instances.\n"
                           "Edits are saved to the selected profile when you press OK.");
    btnProfileSaveAs = new QPushButton("Save as new...");
    btnProfileDelete = new QPushButton("Delete");
    row->addWidget(label);
    row->addWidget(cbxProfile);
    row->addWidget(btnProfileSaveAs);
    row->addWidget(btnProfileDelete);

    chkProfileLink = new QCheckBox("Always use this profile when this controller is connected");
    chkProfileLink->setToolTip("Links the profile to this kind of controller, so it loads automatically "
                               "whenever the controller is plugged in or selected.");

    // the joystick selector is row 7 and the button box row 8; put the
    // profile controls between them
    ui->gridLayout_4->removeWidget(ui->buttonBox);
    ui->gridLayout_4->addLayout(row, 8, 0, 1, 2);
    ui->gridLayout_4->addWidget(chkProfileLink, 9, 0, 1, 2);
    ui->gridLayout_4->addWidget(ui->buttonBox, 10, 0, 1, 2);

    // load everything into memory
    shownProfile = "";
    workingProfiles[""] = gatherJoyMappings();

    Config::Table globalCfg = Config::GetGlobalTable();
    Config::Table profiles = globalCfg.GetTable("ControllerProfiles");
    for (const std::string& name : profiles.GetKeys())
    {
        if (name.empty()) continue;
        Config::Table tbl = profiles.GetTable(name);
        workingProfiles[name] = EmuInstance::readJoyMappings(tbl);
    }
    savedProfiles = workingProfiles;

    Config::Table links = globalCfg.GetTable("ControllerLinks");
    for (const std::string& guid : links.GetKeys())
    {
        std::string name = links.GetString(guid);
        if (!name.empty() && workingProfiles.count(name))
            workingLinks[guid] = name;
    }

    instanceProfile = emuInstance->getLocalConfig().GetString("JoystickProfile");
    if (!workingProfiles.count(instanceProfile))
        instanceProfile = "";

    updatingProfileUI = true;
    cbxProfile->addItem(kDefaultProfileLabel);
    for (auto& [name, mappings] : workingProfiles)
    {
        if (!name.empty())
            cbxProfile->addItem(QString::fromStdString(name));
    }
    updatingProfileUI = false;

    connect(cbxProfile, &QComboBox::currentIndexChanged, this, &InputConfigDialog::onProfileChanged);
    connect(btnProfileSaveAs, &QPushButton::clicked, this, &InputConfigDialog::onProfileSaveAs);
    connect(btnProfileDelete, &QPushButton::clicked, this, &InputConfigDialog::onProfileDelete);
    connect(chkProfileLink, &QCheckBox::toggled, this, &InputConfigDialog::onProfileLinkToggled);

    showProfile(profileForController(currentJoystickGUID()));
}

std::vector<int> InputConfigDialog::gatherJoyMappings()
{
    // start from what's stored so hotkeys not shown in this dialog are kept
    std::vector<int> ret;
    auto it = workingProfiles.find(shownProfile);
    if (it != workingProfiles.end() && it->second.size() == EmuInstance::kJoyMappingCount)
        ret = it->second;
    else
        ret.assign(EmuInstance::kJoyMappingCount, -1);

    for (int i = 0; i < keypad_num; i++)
        ret[dskeyorder[i]] = keypadJoyMap[i];

    int i = 0;
    for (int hotkey : hk_addons) ret[12 + hotkey] = addonsJoyMap[i++];
    i = 0;
    for (int hotkey : hk_general) ret[12 + hotkey] = hkGeneralJoyMap[i++];
    i = 0;
    for (int hotkey : hk_menu) ret[12 + hotkey] = hkMenuJoyMap[i++];

    return ret;
}

void InputConfigDialog::scatterJoyMappings(const std::vector<int>& mappings)
{
    if (mappings.size() != EmuInstance::kJoyMappingCount) return;

    for (int i = 0; i < keypad_num; i++)
        keypadJoyMap[i] = mappings[dskeyorder[i]];

    int i = 0;
    for (int hotkey : hk_addons) addonsJoyMap[i++] = mappings[12 + hotkey];
    i = 0;
    for (int hotkey : hk_general) hkGeneralJoyMap[i++] = mappings[12 + hotkey];
    i = 0;
    for (int hotkey : hk_menu) hkMenuJoyMap[i++] = mappings[12 + hotkey];

    for (JoyMapButton* btn : joyButtons)
        btn->refreshText();
}

// show a profile's mappings in the editors (the current edits must already
// have been stored in workingProfiles)
void InputConfigDialog::showProfile(const std::string& name)
{
    std::string target = workingProfiles.count(name) ? name : "";
    shownProfile = target;
    scatterJoyMappings(workingProfiles[target]);

    updatingProfileUI = true;
    int idx = target.empty() ? 0 : cbxProfile->findText(QString::fromStdString(target));
    cbxProfile->setCurrentIndex(idx < 0 ? 0 : idx);
    updatingProfileUI = false;

    updateProfileControls();
}

void InputConfigDialog::updateProfileControls()
{
    std::string guid = currentJoystickGUID();
    bool isProfile = !shownProfile.empty();

    btnProfileDelete->setEnabled(isProfile);

    updatingProfileUI = true;
    auto it = workingLinks.find(guid);
    chkProfileLink->setChecked(!guid.empty() && it != workingLinks.end() && it->second == shownProfile);
    chkProfileLink->setEnabled(isProfile && !guid.empty());
    updatingProfileUI = false;
}

std::string InputConfigDialog::profileForController(const std::string& guid)
{
    auto it = workingLinks.find(guid);
    if (!guid.empty() && it != workingLinks.end())
        return it->second;
    return instanceProfile;
}

std::string InputConfigDialog::currentJoystickGUID()
{
    if (!ui->cbxJoystick->isEnabled()) return "";
    return EmuInstance::joystickGUIDForIndex(joystickID);
}

void InputConfigDialog::onProfileChanged(int index)
{
    if (updatingProfileUI) return;

    workingProfiles[shownProfile] = gatherJoyMappings();
    std::string name = (index <= 0) ? "" : cbxProfile->itemText(index).toStdString();

    // if this controller has a linked profile, the link follows the
    // selection; otherwise the selection becomes this instance's profile
    std::string guid = currentJoystickGUID();
    if (!guid.empty() && workingLinks.count(guid))
    {
        if (name.empty())
            workingLinks.erase(guid);
        else
            workingLinks[guid] = name;
    }
    else
        instanceProfile = name;

    showProfile(name);
}

void InputConfigDialog::onProfileSaveAs()
{
    QString suggestion = ui->cbxJoystick->isEnabled() ? ui->cbxJoystick->currentText() : "My profile";

    for (;;)
    {
        bool ok = false;
        QString qname = QInputDialog::getText(this, "Save controller profile",
            "Save the current joystick mappings as a new profile named:",
            QLineEdit::Normal, suggestion, &ok);
        if (!ok) return;

        qname = qname.trimmed();
        suggestion = qname;
        QString problem;
        if (qname.isEmpty())
            problem = "Please enter a name.";
        else if (qname.contains('.') || qname.contains('"') || qname.contains('\\'))
            problem = "Profile names can't contain periods, quotes or backslashes.";
        else if (qname.length() > 48)
            problem = "Please use a name of 48 characters or fewer.";
        else if (qname == kDefaultProfileLabel || workingProfiles.count(qname.toStdString()))
            problem = "A profile with that name already exists.";

        if (!problem.isEmpty())
        {
            QMessageBox::warning(this, "Save controller profile", problem);
            continue;
        }

        std::string name = qname.toStdString();

        // the edits go to the new profile; the profile they were made on
        // goes back to how it was
        std::vector<int> mappings = gatherJoyMappings();
        if (savedProfiles.count(shownProfile))
            workingProfiles[shownProfile] = savedProfiles[shownProfile];
        workingProfiles[name] = mappings;
        deletedProfiles.erase(name);

        // keep the list sorted the same way std::map is
        int pos = 1;
        while (pos < cbxProfile->count() && cbxProfile->itemText(pos).toStdString() < name)
            pos++;
        updatingProfileUI = true;
        cbxProfile->insertItem(pos, qname);
        updatingProfileUI = false;

        std::string guid = currentJoystickGUID();
        if (!guid.empty() && workingLinks.count(guid))
            workingLinks[guid] = name;
        else
            instanceProfile = name;

        showProfile(name);
        return;
    }
}

void InputConfigDialog::onProfileDelete()
{
    if (shownProfile.empty()) return;

    QString qname = QString::fromStdString(shownProfile);
    if (QMessageBox::question(this, "Delete controller profile",
            QString("Delete the profile \"%1\"? It's removed when you press OK.").arg(qname))
        != QMessageBox::Yes)
        return;

    std::string name = shownProfile;
    workingProfiles.erase(name);
    deletedProfiles.insert(name);
    for (auto it = workingLinks.begin(); it != workingLinks.end(); )
    {
        if (it->second == name) it = workingLinks.erase(it);
        else ++it;
    }
    if (instanceProfile == name)
        instanceProfile = "";

    updatingProfileUI = true;
    cbxProfile->removeItem(cbxProfile->findText(qname));
    updatingProfileUI = false;

    shownProfile = "";
    showProfile(profileForController(currentJoystickGUID()));
}

void InputConfigDialog::onProfileLinkToggled(bool checked)
{
    if (updatingProfileUI) return;

    std::string guid = currentJoystickGUID();
    if (guid.empty() || shownProfile.empty()) return;

    if (checked)
        workingLinks[guid] = shownProfile;
    else
    {
        workingLinks.erase(guid);
        instanceProfile = shownProfile;
    }
}

SDL_Joystick* InputConfigDialog::getJoystick()
{
    return emuInstance->getJoystick();
}

std::shared_ptr<SDL_mutex> InputConfigDialog::getJoyMutex()
{
    return emuInstance->getJoyMutex();
}
