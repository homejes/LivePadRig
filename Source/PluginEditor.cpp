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

    subtitleLabel.setText("Created by Ryan Dharmawan | PRO DSP VERSION", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Italic")));
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(subtitleLabel);

    locateFolderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    locateFolderBtn.addListener(this);
    addAndMakeVisible(locateFolderBtn);

    padPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(padPresetLabel);
    padPresetCombo.addListener(this);
    addAndMakeVisible(padPresetCombo);

    sfxKitLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sfxKitLabel);
    sfxKitCombo.addListener(this);
    addAndMakeVisible(sfxKitCombo);

    myPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(myPresetLabel);
    myPresetCombo.addListener(this);
    addAndMakeVisible(myPresetCombo);

    saveBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7b68ee));
    saveBtn.addListener(this);
    addAndMakeVisible(saveBtn);

    delMyPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    delMyPresetBtn.addListener(this);
    addAndMakeVisible(delMyPresetBtn);

    midiPresetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(midiPresetLabel);
    midiPresetCombo.addListener(this);
    addAndMakeVisible(midiPresetCombo);

    saveMidiPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7b68ee));
    saveMidiPresetBtn.addListener(this);
    addAndMakeVisible(saveMidiPresetBtn);

    delMidiPresetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    delMidiPresetBtn.addListener(this);
    addAndMakeVisible(delMidiPresetBtn);

    midiLearnBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a50));
    midiLearnBtn.setClickingTogglesState(true);
    midiLearnBtn.addListener(this);
    addAndMakeVisible(midiLearnBtn);

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

    for (int i = 0; i < 8; ++i) {
        juce::String sfxName = audioProcessor.currentSfxNames[i];
        auto sfxBtn = std::make_unique<juce::TextButton>(sfxName);
        sfxBtn->addListener(this);
        addAndMakeVisible(sfxBtn.get());
        sfxButtons.push_back(std::move(sfxBtn));

        auto sfxLbl = std::make_unique<juce::Label>("", sfxName);
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

    auto setupHorizontalSlider = [this](juce::Slider& slider, juce::Label& lbl, double min, double max, float& var) {
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
    setupHorizontalSlider(toneSlider, toneLabel, 1200.0, 12000.0, tParam);
    setupHorizontalSlider(hpfSlider, hpfLabel, 20.0, 200.0, hParam);
    setupHorizontalSlider(xfadeSlider, xfadeLabel, 0.2, 8.0, xParam);

    auto setupMixerFader = [this](juce::Slider& slider, juce::Label& lbl, float& var) {
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
    setupMixerFader(masterFader, masterLabel, mVol);
    setupMixerFader(padFader, padMixLabel, pVol);
    setupMixerFader(sfxFader, sfxMixLabel, sVol);

    footerLabel.setText("Developed by Ryan Dharmawan | IG: @ryan.dharmawan | FREEWARE", juce::dontSendNotification);
    footerLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(footerLabel);

    stopAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    stopAudioBtn.addListener(this);
    addAndMakeVisible(stopAudioBtn);

    setSize (1000, 720);
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
    g.fillRoundedRectangle(20, 165, 720, 240, 10.0f); 
    g.fillRoundedRectangle(20, 420, 720, 220, 10.0f); 
    g.fillRoundedRectangle(760, 165, 220, 130, 10.0f); 
    g.fillRoundedRectangle(760, 305, 220, 335, 10.0f); 

    g.setColour(juce::Colours::grey);
    g.setFont(12.0f);
    g.drawText("PAD AMBIENT", 30, 175, 300, 20, juce::Justification::left);
    g.drawText("SFX / DRUM TRIGGERS", 30, 430, 200, 20, juce::Justification::left);
    g.drawText("PAD DSP  TRANSITION", 770, 175, 200, 20, juce::Justification::left);
    g.drawText("OUTPUT MIXER", 770, 315, 200, 20, juce::Justification::left);
}

void LivePadRigAudioProcessorEditor::resized()
{
    titleLabel.setBounds(20, 15, 300, 35);
    subtitleLabel.setBounds(20, 50, 400, 20);
    locateFolderBtn.setBounds(860, 20, 120, 30);

    int row1Y = 82;
    padPresetLabel.setBounds(20, row1Y, 80, 30);
    padPresetCombo.setBounds(100, row1Y, 110, 30);
    
    sfxKitLabel.setBounds(220, row1Y, 55, 30);
    sfxKitCombo.setBounds(280, row1Y, 110, 30);
    
    myPresetLabel.setBounds(400, row1Y, 75, 30);
    myPresetCombo.setBounds(480, row1Y, 130, 30);
    saveBtn.setBounds(615, row1Y, 50, 30);
    delMyPresetBtn.setBounds(670, row1Y, 35, 30);

    int row2Y = 120;
    midiPresetLabel.setBounds(20, row2Y, 70, 30);
    midiPresetCombo.setBounds(100, row2Y, 150, 30);
    saveMidiPresetBtn.setBounds(255, row2Y, 50, 30);
    delMidiPresetBtn.setBounds(310, row2Y, 35, 30);
    
    midiLearnBtn.setBounds(360, row2Y, 140, 30);
    clearMidiBtn.setBounds(510, row2Y, 100, 30);

    for (int i = 0; i < 12; ++i) {
        int row = i / 6; int col = i % 6;
        padButtons[i]->setBounds(40 + col * 115, 205 + row * 90, 100, 75);
    }

    for (int i = 0; i < 8; ++i) {
        int row = i / 4; int col = i % 4; 
        sfxButtons[i]->setBounds(40 + col * 85, 460 + row * 85, 75, 70);
        int faderX = 400 + (i * 40);
        sfxLabels[i]->setBounds(faderX - 10, 450, 60, 20);
        sfxFaders[i]->setBounds(faderX, 470, 40, 140);
    }

    toneLabel.setBounds(770, 200, 50, 20); toneSlider.setBounds(820, 195, 150, 30);
    hpfLabel.setBounds(770, 230, 50, 20); hpfSlider.setBounds(820, 225, 150, 30);
    xfadeLabel.setBounds(770, 260, 50, 20); xfadeSlider.setBounds(820, 255, 150, 30);

    masterLabel.setBounds(775, 340, 60, 20); masterFader.setBounds(780, 360, 50, 250);
    padMixLabel.setBounds(845, 340, 60, 20); padFader.setBounds(850, 360, 50, 250);
    sfxMixLabel.setBounds(915, 340, 60, 20); sfxFader.setBounds(920, 360, 50, 250);

    footerLabel.setBounds(20, 680, 400, 20);
    stopAudioBtn.setBounds(830, 675, 150, 30);
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
        sfxLabels[i]->setText(sfxName, juce::dontSendNotification);
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

            // SINKRONISASI SEMUA SLIDER & FADER KE UI
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