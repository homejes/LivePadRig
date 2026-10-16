#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LivePadRigLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LivePadRigLookAndFeel()
    {
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff121218));
        setColour(juce::TextButton::buttonColourId, juce::Colour(0xff22222f));
        setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00ffff)); 
        setColour(juce::TextButton::textColourOnId, juce::Colour(0xff121218)); 
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff7b68ee)); 
        setColour(juce::Slider::trackColourId, juce::Colour(0xff121218));
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff22222f));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff121218));
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a50));
    }
};

class LivePadRigAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Button::Listener,
                                        private juce::ComboBox::Listener,
                                        private juce::Timer
{
public:
    LivePadRigAudioProcessorEditor (LivePadRigAudioProcessor&);
    ~LivePadRigAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void buttonClicked (juce::Button* button) override;
    void comboBoxChanged (juce::ComboBox* comboBox) override;
    
    void updateDropdowns();
    void updateSfxUI(); 
    void updateToggleStates(); 
    void updatePresetDropdown();
    void updateMidiPresetDropdown();

    LivePadRigAudioProcessor& audioProcessor;
    LivePadRigLookAndFeel customLookAndFeel;

    std::shared_ptr<juce::FileChooser> presetSaver;
    std::shared_ptr<juce::FileChooser> midiPresetSaver;
    std::shared_ptr<juce::FileChooser> folderChooser;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::TextButton locateFolderBtn{"LOCATE FOLDER"};
    
    juce::Label padPresetLabel{"", "PAD PRESET"};
    juce::ComboBox padPresetCombo;
    juce::Label sfxKitLabel{"", "SFX KIT"};
    juce::ComboBox sfxKitCombo;
    
    juce::Label myPresetLabel{"", "MY PRESET"};
    juce::ComboBox myPresetCombo;
    juce::TextButton saveBtn{"SAVE"};
    juce::TextButton delMyPresetBtn{juce::CharPointer_UTF8("\xF0\x9F\x97\x91")};

    juce::Label midiPresetLabel{"", "MIDI MAP"};
    juce::ComboBox midiPresetCombo;
    juce::TextButton saveMidiPresetBtn{"SAVE"};
    juce::TextButton delMidiPresetBtn{juce::CharPointer_UTF8("\xF0\x9F\x97\x91")};
    juce::TextButton midiLearnBtn{"MIDI LEARN"};
    juce::TextButton clearMidiBtn{"CLEAR MIDI"};

    std::vector<std::unique_ptr<juce::TextButton>> padButtons;
    std::vector<std::unique_ptr<juce::TextButton>> sfxButtons;
    std::vector<std::unique_ptr<juce::Slider>> sfxFaders;
    std::vector<std::unique_ptr<juce::Label>> sfxLabels;

    juce::Slider toneSlider;
    juce::Slider hpfSlider;
    juce::Slider xfadeSlider;
    juce::Label toneLabel{"", "TONE"};
    juce::Label hpfLabel{"", "HPF"};
    juce::Label xfadeLabel{"", "X-FADE"};

    juce::Slider masterFader;
    juce::Slider padFader;
    juce::Slider sfxFader;
    juce::Label masterLabel{"", "MASTER"};
    juce::Label padMixLabel{"", "PAD"};
    juce::Label sfxMixLabel{"", "SFX"};

    juce::Label footerLabel;
    juce::TextButton stopAudioBtn{"STOP ALL AUDIO"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivePadRigAudioProcessorEditor)
};