#include "PluginProcessor.h"
#include "PluginEditor.h"

LivePadRigAudioProcessorEditor::LivePadRigAudioProcessorEditor (LivePadRigAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);

    titleLabel.setText("L I V E P A D   R I G", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(28.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ffff));
    addAndMakeVisible(titleLabel);

    // -- PERBAIKAN v1.2: Update Versi --
    subtitleLabel.setText("Created by Ryan Dharmawan | PRO DSP VERSION v1.2", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Italic")));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(subtitleLabel);

    locateFolderBtn.setButtonText("LOCATE AUDIO");
    locateFolderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    locateFolderBtn.addListener(this);
    addAndMakeVisible(locateFolderBtn);

    padPresetLabel.setText("PAD PRESET", juce::dontSendNotification);
    padPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(padPresetLabel);
    padPresetCombo.addListener(this);
    addAndMakeVisible(padPresetCombo);

    sfxKitLabel.setText("SFX KIT", juce::dontSendNotification);
    sfxKitLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sfxKitLabel);
    sfxKitCombo.addListener(this);
    addAndMakeVisible(sfxKitCombo);

    myPresetLabel.setText("MY PRESET", juce::dontSendNotification);
    myPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(myPresetLabel);
    myPresetCombo.addListener(this);
    addAndMakeVisible(myPresetCombo);

    saveBtn.setButtonText("SAVE");
    saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7b68ee));
    saveBtn.addListener(this);
    addAndMakeVisible(saveBtn);

    delMyPresetBtn.setButtonText("DEL");
    delMyPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    delMyPresetBtn.addListener(this);
    addAndMakeVisible(delMyPresetBtn);

    midiPresetLabel.setText("MIDI MAP", juce::dontSendNotification);
    midiPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(midiPresetLabel);
    midiPresetCombo.addListener(this);
    addAndMakeVisible(midiPresetCombo);

    saveMidiPresetBtn.setButtonText("SAVE");
    saveMidiPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7b68ee));
    saveMidiPresetBtn.addListener(this);
    addAndMakeVisible(saveMidiPresetBtn);

    delMidiPresetBtn.setButtonText("DEL");
    delMidiPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    delMidiPresetBtn.addListener(this);
    addAndMakeVisible(delMidiPresetBtn);

    midiLearnBtn.setButtonText("MIDI LEARN");
    midiLearnBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    midiLearnBtn.setClickingTogglesState(true);
    midiLearnBtn.addListener(this);
    addAndMakeVisible(midiLearnBtn);

    clearMidiBtn.setButtonText("CLEAR MIDI");
    clearMidiBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    clearMidiBtn.addListener(this);
    addAndMakeVisible(clearMidiBtn);

    updateDropdowns();
    updatePresetDropdown();
    updateMidiPresetDropdown();

    juce::StringArray noteNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i) {
        auto btn = std::make_unique<juce::TextButton>(noteNames[i]);
        btn->setClickingTogglesState(true); 
        btn->setToggleState(audioProcessor.padState[i], juce::dontSendNotification);
        btn->addListener(this);
        addAndMakeVisible(btn.get());
        padButtons.push_back(std::move(btn));
    }

    // -- PERBAIKAN v1.2: SFX Label di atas fader --
    for (int i = 0; i < 8; ++i) {
        juce::String sfxName = audioProcessor.currentSfxNames[i];
        
        // Singkat teks jadi maksimal 5 karakter agar tidak numpuk
        juce::String shortName = sfxName.substring(0, 5); 
        
        auto sfxBtn = std::make_unique<juce::TextButton>(sfxName);
        sfxBtn->addListener(this);
        addAndMakeVisible(sfxBtn.get());
        sfxButtons.push_back(std::move(sfxBtn));

        auto sfxLbl = std::make_unique<juce::Label>("", shortName);
        sfxLbl->setFont(juce::Font(11.0f)); // Perkecil ukuran font
        sfxLbl->setJustificationType(juce::Justification::centred);
        sfxLbl->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(sfxLbl.get());
        sfxLabels.push_back(std::move(sfxLbl));

        auto sfxFd = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::NoTextBox);
        sfxFd->setRange(0.0, 1.0, 0.01);
        sfxFd->setValue(audioProcessor.sfxIndivVolParam[i].load());
        sfxFd->onValueChange = [this, sfxFd_ptr = sfxFd.get(), i]() {
            audioProcessor.sfxIndivVolParam[i].store((float)sfxFd_ptr->getValue());
        };
        addAndMakeVisible(sfxFd.get());
        sfxFaders.push_back(std::move(sfxFd));
    }

    auto setupHorizontalSlider = [this](juce::Slider& slider, juce::Label& lbl, double min, double max, float& var, juce::String text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(lbl);
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        slider.setRange(min, max, 0.1);
        slider.setValue(var);
        slider.onValueChange = [this, &slider, &var] { var = (float)slider.getValue(); };
        addAndMakeVisible(slider);
    };

    float& tParam = reinterpret_cast<float&>(audioProcessor.toneParam);
    float& hParam = reinterpret_cast<float&>(audioProcessor.hpfParam);
    float& xParam = reinterpret_cast<float&>(audioProcessor.xfadeParam);
    setupHorizontalSlider(toneSlider, toneLabel, 1200.0, 12000.0, tParam, "TONE");
    setupHorizontalSlider(hpfSlider, hpfLabel, 20.0, 200.0, hParam, "HPF");
    setupHorizontalSlider(xfadeSlider, xfadeLabel, 0.2, 8.0, xParam, "X-FADE");

    auto setupMixerFader = [this](juce::Slider& slider, juce::Label& lbl, float& var, juce::String text) {
        lbl.setText(text, juce::dontSendNotification);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(lbl);
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        slider.setRange(0.0, 1.0, 0.01);
        slider.setValue(var);
        slider.onValueChange = [this, &slider, &var] { var = (float)slider.getValue(); };
        addAndMakeVisible(slider);
    };

    float& mVol = reinterpret_cast<float&>(audioProcessor.masterVolParam);
    float& pVol = reinterpret_cast<float&>(audioProcessor.padVolParam);
    float& sVol = reinterpret_cast<float&>(audioProcessor.sfxVolParam);
    setupMixerFader(masterFader, masterLabel, mVol, "MASTER");
    setupMixerFader(padFader, padMixLabel, pVol, "PAD");
    setupMixerFader(sfxFader, sfxMixLabel, sVol, "SFX");

    footerLabel.setText("Developed by Ryan Dharmawan | IG: @ryan.dharmawan | FREEWARE", juce::dontSendNotification);
    footerLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(footerLabel);

    stopAudioBtn.setButtonText("STOP ALL AUDIO");
    stopAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    stopAudioBtn.addListener(this);
    addAndMakeVisible(stopAudioBtn);

    // -- PERBAIKAN v1.2: Matikan Resizable --
    setSize (800, 550); // Ukuran default
    setResizable (false, false); // Resizer dimatikan
    
    startTimerHz(20);
}

LivePadRigAudioProcessorEditor::~LivePadRigAudioProcessorEditor() { 
    stopTimer();
    setLookAndFeel(nullptr); 
}

void LivePadRigAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.needToSaveMidi.load()) {
        audioProcessor.saveMidiConfig();
        audioProcessor.needToSaveMidi.store(false);
        midiLearnBtn.setToggleState(false, juce::dontSendNotification);
        midiLearnBtn.setButtonText("MIDI LEARN");
    }

    for (int i = 0; i < 12; ++i) {
        if (padButtons[i]->getToggleState() != audioProcessor.padState[i]) {
            padButtons[i]->setToggleState(audioProcessor.padState[i], juce::dontSendNotification);
        }
    }
}

void LivePadRigAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour(0xff121218)); 
    g.setColour(juce::Colour(0xff1e1e28));
    
    // Background UI Panels
    g.fillRoundedRectangle(20, 140, 560, 200, 10.0f);  // Pad Panel
    g.fillRoundedRectangle(20, 350, 560, 150, 10.0f);  // SFX Panel
    g.fillRoundedRectangle(590, 140, 190, 120, 10.0f); // DSP Panel
    g.fillRoundedRectangle(590, 270, 190, 230, 10.0f); // Mixer Panel

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("PAD AMBIENT", 30, 145, 300, 20, juce::Justification::left);
    g.drawText("SFX / DRUM TRIGGERS", 30, 355, 200, 20, juce::Justification::left);
    g.drawText("PAD DSP", 600, 145, 100, 20, juce::Justification::left);
    g.drawText("OUTPUT MIXER", 600, 275, 100, 20, juce::Justification::left);
}

void LivePadRigAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    titleLabel.setBounds(20, 10, 300, 35);
    subtitleLabel.setBounds(20, 40, 400, 20);
    locateFolderBtn.setBounds(getWidth() - 140, 15, 120, 30);

    int row1Y = 70;
    padPresetLabel.setBounds(20, row1Y, 80, 25);
    padPresetCombo.setBounds(100, row1Y, 110, 25);
    
    sfxKitLabel.setBounds(220, row1Y, 55, 25);
    sfxKitCombo.setBounds(280, row1Y, 110, 25);
    
    myPresetLabel.setBounds(400, row1Y, 75, 25);
    myPresetCombo.setBounds(480, row1Y, 130, 25);
    saveBtn.setBounds(615, row1Y, 50, 25);
    delMyPresetBtn.setBounds(670, row1Y, 35, 25);

    int row2Y = 100;
    midiPresetLabel.setBounds(20, row2Y, 70, 25);
    midiPresetCombo.setBounds(100, row2Y, 150, 25);
    saveMidiPresetBtn.setBounds(255, row2Y, 50, 25);
    delMidiPresetBtn.setBounds(310, row2Y, 35, 25);
    
    midiLearnBtn.setBounds(360, row2Y, 140, 25);
    clearMidiBtn.setBounds(510, row2Y, 100, 25);

    for (int i = 0; i < 12; ++i) {
        int row = i / 6; int col = i % 6;
        padButtons[i]->setBounds(35 + col * 88, 175 + row * 75, 75, 60);
    }

    for (int i = 0; i < 8; ++i) {
        int row = i / 4; int col = i % 4; 
        sfxButtons[i]->setBounds(35 + col * 65, 385 + row * 55, 55, 45);
        int faderX = 310 + (i * 32);
        sfxLabels[i]->setBounds(faderX - 10, 375, 50, 20);
        sfxFaders[i]->setBounds(faderX, 395, 30, 95);
    }

    toneLabel.setBounds(600, 170, 50, 20); toneSlider.setBounds(650, 170, 120, 20);
    hpfLabel.setBounds(600, 200, 50, 20);  hpfSlider.setBounds(650, 200, 120, 20);
    xfadeLabel.setBounds(600, 230, 50, 20); xfadeSlider.setBounds(650, 230, 120, 20);

    masterLabel.setBounds(605, 300, 50, 20); masterFader.setBounds(605, 320, 50, 160);
    padMixLabel.setBounds(660, 300, 50, 20); padFader.setBounds(660, 320, 50, 160);
    sfxMixLabel.setBounds(715, 300, 50, 20); sfxFader.setBounds(715, 320, 50, 160);

    footerLabel.setBounds(20, getHeight() - 30, 400, 20);
    stopAudioBtn.setBounds(getWidth() - 170, getHeight() - 35, 150, 25);
}

void LivePadRigAudioProcessorEditor::updateDropdowns() {
    padPresetCombo.clear(juce::dontSendNotification);
    auto pads = audioProcessor.getAvailablePadPresets();
    for (int i = 0; i < pads.size(); ++i) padPresetCombo.addItem(pads[i], i + 1);
    padPresetCombo.setSelectedId(1, juce::dontSendNotification);

    sfxKitCombo.clear(juce::dontSendNotification);
    auto kits = audioProcessor.getAvailableSfxKits();
    for (int i = 0; i < kits.size(); ++i) sfxKitCombo.addItem(kits[i], i + 1);
    sfxKitCombo.setSelectedId(1, juce::dontSendNotification);
}

void LivePadRigAudioProcessorEditor::updatePresetDropdown() {
    myPresetCombo.clear(juce::dontSendNotification);
    myPresetCombo.addItem("-- Custom Presets --", 1);
    auto presets = audioProcessor.getUserPresetNames();
    for (int i = 0; i < presets.size(); ++i) myPresetCombo.addItem(presets[i], i + 2);
    myPresetCombo.setSelectedId(1, juce::dontSendNotification);
}

void LivePadRigAudioProcessorEditor::updateMidiPresetDropdown() {
    midiPresetCombo.clear(juce::dontSendNotification);
    midiPresetCombo.addItem("-- Default Mapping --", 1);
    auto midiPresets = audioProcessor.getMidiPresetNames();
    for (int i = 0; i < midiPresets.size(); ++i) midiPresetCombo.addItem(midiPresets[i], i + 2);
    midiPresetCombo.setSelectedId(1, juce::dontSendNotification);
}

void LivePadRigAudioProcessorEditor::updateSfxUI() {
    for (int i = 0; i < 8; ++i) {
        juce::String sfxName = audioProcessor.currentSfxNames[i];
        sfxButtons[i]->setButtonText(sfxName);
        
        // -- PERBAIKAN v1.2: Singkat label fader saat memuat preset baru --
        juce::String shortName = sfxName.substring(0, 5);
        sfxLabels[i]->setText(shortName, juce::dontSendNotification);
    }
}

void LivePadRigAudioProcessorEditor::updateToggleStates() {
    for (int i = 0; i < 12; ++i) {
        padButtons[i]->setToggleState(audioProcessor.padState[i], juce::dontSendNotification);
    }
}

void LivePadRigAudioProcessorEditor::comboBoxChanged (juce::ComboBox* comboBox) 
{ 
    if (comboBox == &padPresetCombo) audioProcessor.changePadPreset(comboBox->getText());
    else if (comboBox == &sfxKitCombo) {
        audioProcessor.loadSfxKit(comboBox->getText());
        updateSfxUI();
    }
    else if (comboBox == &myPresetCombo) {
        if (comboBox->getSelectedId() > 1) {
            audioProcessor.loadUserPreset(comboBox->getText());
            
            for (int i = 0; i < padPresetCombo.getNumItems(); ++i) {
                if (padPresetCombo.getItemText(i) == audioProcessor.currentPadPresetName) {
                    padPresetCombo.setSelectedId(i + 1, juce::dontSendNotification); break;
                }
            }
            for (int i = 0; i < sfxKitCombo.getNumItems(); ++i) {
                if (sfxKitCombo.getItemText(i) == audioProcessor.currentSfxKitName) {
                    sfxKitCombo.setSelectedId(i + 1, juce::dontSendNotification); break;
                }
            }
            updateSfxUI();

            toneSlider.setValue(audioProcessor.toneParam.load(), juce::dontSendNotification);
            hpfSlider.setValue(audioProcessor.hpfParam.load(), juce::dontSendNotification);
            xfadeSlider.setValue(audioProcessor.xfadeParam.load(), juce::dontSendNotification);
            masterFader.setValue(audioProcessor.masterVolParam.load(), juce::dontSendNotification);
            padFader.setValue(audioProcessor.padVolParam.load(), juce::dontSendNotification);
            sfxFader.setValue(audioProcessor.sfxVolParam.load(), juce::dontSendNotification);
            for (int i = 0; i < 8; ++i) {
                sfxFaders[i]->setValue(audioProcessor.sfxIndivVolParam[i].load(), juce::dontSendNotification);
            }
        }
    }
    else if (comboBox == &midiPresetCombo) {
        if (comboBox->getSelectedId() == 1) {
            audioProcessor.loadMidiPreset("default_map");
        } else {
            audioProcessor.loadMidiPreset(comboBox->getText());
        }
    }
}

void LivePadRigAudioProcessorEditor::buttonClicked (juce::Button* button) 
{
    if (button == &saveBtn) {
        juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("presets");
        appDir.createDirectory();
        
        presetSaver = std::make_shared<juce::FileChooser>("Save Custom Preset", appDir.getChildFile("My Setup.xml"), "*.xml");
        presetSaver->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File()) {
                    audioProcessor.saveUserPreset(file.getFileNameWithoutExtension());
                    juce::MessageManager::callAsync([this]() { updatePresetDropdown(); });
                }
            });
        return;
    }

    if (button == &delMyPresetBtn) {
        if (myPresetCombo.getSelectedId() > 1) {
            audioProcessor.deleteUserPreset(myPresetCombo.getText());
            updatePresetDropdown();
        }
        return;
    }

    if (button == &saveMidiPresetBtn) {
        juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("midi_presets");
        appDir.createDirectory();
        
        midiPresetSaver = std::make_shared<juce::FileChooser>("Save MIDI Mapping Preset", appDir.getChildFile("Controller Mapping.xml"), "*.xml");
        midiPresetSaver->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File()) {
                    audioProcessor.saveMidiPreset(file.getFileNameWithoutExtension());
                    juce::MessageManager::callAsync([this]() { updateMidiPresetDropdown(); });
                }
            });
        return;
    }

    if (button == &delMidiPresetBtn) {
        if (midiPresetCombo.getSelectedId() > 1) {
            audioProcessor.deleteMidiPreset(midiPresetCombo.getText());
            updateMidiPresetDropdown();
            audioProcessor.loadMidiPreset("default_map");
        }
        return;
    }

    if (button == &midiLearnBtn) {
        if (midiLearnBtn.getToggleState()) {
            audioProcessor.startMidiLearn(-1, -1);
            midiLearnBtn.setButtonText("1. SELECT PAD/SFX");
        } else {
            midiLearnBtn.setButtonText("MIDI LEARN");
            audioProcessor.cancelMidiLearn();
        }
        return;
    }

    if (button == &clearMidiBtn) {
        audioProcessor.clearMidiConfig();
        midiLearnBtn.setToggleState(false, juce::dontSendNotification);
        midiLearnBtn.setButtonText("MIDI LEARN");
        return;
    }

    if (button == &locateFolderBtn)
    {
        folderChooser = std::make_shared<juce::FileChooser> (
            "Select LivePadRig Sound Folder", audioProcessor.getRootSoundFolder(), "*");
        folderChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this] (const juce::FileChooser& fc) {
                auto result = fc.getResult();
                if (result.isDirectory()) {
                    audioProcessor.setRootSoundFolder(result);
                    juce::MessageManager::callAsync([this]() {
                        updateDropdowns();
                        auto pads = audioProcessor.getAvailablePadPresets();
                        auto kits = audioProcessor.getAvailableSfxKits();
                        if (!pads.isEmpty()) audioProcessor.changePadPreset(pads[0]);
                        if (!kits.isEmpty()) { audioProcessor.loadSfxKit(kits[0]); updateSfxUI(); }
                    });
                }
            });
        return;
    }

    for (int i = 0; i < 12; ++i) {
        if (button == padButtons[i].get()) {
            if (midiLearnBtn.getToggleState()) {
                audioProcessor.startMidiLearn(0, i);
                midiLearnBtn.setButtonText("2. PRESS MIDI KEY...");
                return;
            }

            bool isNowOn = button->getToggleState();
            if (isNowOn) {
                for (int j = 0; j < 12; ++j) {
                    if (i != j && padButtons[j]->getToggleState()) {
                        padButtons[j]->setToggleState(false, juce::dontSendNotification);
                        audioProcessor.setPadState(j, false);
                    }
                }
            }
            audioProcessor.setPadState(i, isNowOn);
            return;
        }
    }
    
    for (int i = 0; i < 8; ++i) {
        if (button == sfxButtons[i].get()) {
            if (midiLearnBtn.getToggleState()) {
                audioProcessor.startMidiLearn(1, i);
                midiLearnBtn.setButtonText("2. PRESS MIDI KEY...");
                return;
            }
            audioProcessor.triggerSFX(i);
            return;
        }
    }
    
    if (button == &stopAudioBtn) {
        audioProcessor.stopAllAudio();
        updateToggleStates();
    }
}