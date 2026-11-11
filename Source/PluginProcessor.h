#pragma once

#include <JuceHeader.h>
#include <atomic>

class LivePadRigAudioProcessor  : public juce::AudioProcessor
{
public:
    LivePadRigAudioProcessor();
    ~LivePadRigAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- PARAMETERS ---
    std::atomic<float> toneParam {12000.0f};
    std::atomic<float> hpfParam {20.0f};
    std::atomic<float> xfadeParam {4.0f};
    std::atomic<float> masterVolParam {0.8f};
    std::atomic<float> padVolParam {0.8f};
    std::atomic<float> sfxVolParam {0.8f};
    std::atomic<float> sfxIndivVolParam[8];

    // padState diakses dari audio thread (processBlock) dan message thread (GUI),
    // jadi harus atomic agar tidak terjadi data race.
    std::atomic<bool> padState[12];
    void setPadState(int index, bool isPlaying);
    // Set pad dengan logika EKSKLUSIF (monophonic): saat pad menyala,
    // semua pad aktif lainnya otomatis dimatikan (smooth fade out via X-FADE).
    void setPadExclusive(int index, bool isPlaying);
    // Toggle pad (dipakai handler MIDI note) dengan logika eksklusif yang sama
    // seperti klik mouse pada GUI.
    void togglePadExclusive(int index);
    void triggerSFX(int sfxIndex);
    void stopAllAudio();

    // --- FOLDERS & PRESETS ---
    void setRootSoundFolder(const juce::File& folder);
    juce::File getRootSoundFolder() const;
    void saveSettings();
    void loadSettings();

    juce::StringArray getAvailablePadPresets();
    juce::StringArray getAvailableSfxKits();

    void loadPadPreset(const juce::String& folderName);
    void changePadPreset(const juce::String& folderName);
    void loadSfxKit(const juce::String& folderName);

    void saveUserPreset(const juce::String& presetName);
    void loadUserPreset(const juce::String& presetName);
    void deleteUserPreset(const juce::String& presetName);
    juce::StringArray getUserPresetNames();

    // Nama user preset / midi preset yang terakhir aktif (untuk sinkronisasi UI saat startup).
    juce::String lastUserPresetName {""};
    juce::String lastMidiPresetName {""};
    // Nama pad preset / sfx kit yang terakhir dipakai (dimuat dari settings.xml saat startup).
    juce::String lastPadPresetName {""};
    juce::String lastSfxKitName {""};

    // --- MIDI PRESETS & LEARN ---
    std::atomic<bool> isMidiLearning {false};
    std::atomic<bool> needToSaveMidi {false};
    int learnTargetType = -1; 
    int learnTargetIndex = -1;
    int padMidiMap[12];
    int sfxMidiMap[8];
    
    void startMidiLearn(int type, int index);
    void cancelMidiLearn();
    void saveMidiConfig();
    void loadMidiConfig();
    void clearMidiConfig();

    void saveMidiPreset(const juce::String& presetName);
    void loadMidiPreset(const juce::String& presetName);
    void deleteMidiPreset(const juce::String& presetName);
    juce::StringArray getMidiPresetNames();

    juce::StringArray currentSfxNames;

    juce::String currentPadPresetName {""};
    juce::String currentSfxKitName {""};

    // Nilai CC terakhir per controller (0..127) untuk deteksi tepi (rising edge),
    // agar CC > 63 tidak men-trigger ulang terus-menerus saat bergerak.
    int lastCCValue[128] {};

private:
    juce::AudioFormatManager formatManager;
    double currentSampleRate = 44100.0;

    juce::Synthesiser padSynth;
    juce::Synthesiser sfxSynth;

    juce::CriticalSection midiLock;
    juce::MidiBuffer incomingMidi;
    juce::CriticalSection audioLock;

    juce::File rootSoundDir;
    juce::AudioBuffer<float> padBuffer;
    juce::AudioBuffer<float> sfxBuffer;

    void setupSynths();
    void loadSampleSFX(juce::Synthesiser& synth, const juce::String& path, int midiNote);

    juce::IIRFilter padLpf[2];
    juce::IIRFilter padHpf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LivePadRigAudioProcessor)
};