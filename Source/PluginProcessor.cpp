#include "PluginProcessor.h"
#include "PluginEditor.h"

class PadSound : public juce::SynthesiserSound {
public:
    PadSound(juce::AudioFormatReader& reader, int midiNote) : note(midiNote) {
        sourceSampleRate = reader.sampleRate > 0 ? reader.sampleRate : 44100.0;
        buffer.setSize(reader.numChannels, (int)reader.lengthInSamples);
        reader.read(&buffer, 0, (int)reader.lengthInSamples, 0, true, true);
    }
    bool appliesToNote(int midiNote) override { return midiNote == note; }
    bool appliesToChannel(int) override { return true; }
    juce::AudioBuffer<float> buffer;
    int note;
    double sourceSampleRate;
};

class SfxSound : public juce::SynthesiserSound {
public:
    SfxSound(juce::AudioFormatReader& reader, int midiNote) : note(midiNote) {
        sourceSampleRate = reader.sampleRate > 0 ? reader.sampleRate : 44100.0;
        buffer.setSize(reader.numChannels, (int)reader.lengthInSamples);
        reader.read(&buffer, 0, (int)reader.lengthInSamples, 0, true, true);
    }
    bool appliesToNote(int midiNote) override { return midiNote == note; }
    bool appliesToChannel(int) override { return true; }
    juce::AudioBuffer<float> buffer;
    int note;
    double sourceSampleRate;
};

class AmbientVoice : public juce::SynthesiserVoice {
public:
    AmbientVoice(std::atomic<float>& xfade) : xfadeParam(xfade) {}
    bool canPlaySound(juce::SynthesiserSound* sound) override { return dynamic_cast<PadSound*>(sound) != nullptr; }

    void startNote(int, float, juce::SynthesiserSound* sound, int) override {
        auto* padSound = static_cast<PadSound*>(sound);
        if (padSound) {
            double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
            pitchRatio = padSound->sourceSampleRate / sr;
        }

        vA.active = true; vA.pos = 0.0; vA.isFirst = true;
        vB.active = false; vB.pos = 0.0; vB.isFirst = false;

        isReleasing = false;
        volEnv = 0.0f;
        
        double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
        float xfade = xfadeParam.load() < 0.2f ? 0.2f : xfadeParam.load();
        
        attackRate = 1.0f / (float)(sr * xfade);
        releaseRate = 1.0f / (float)(sr * xfade);
    }

    void stopNote(float, bool allowTailOff) override {
        if (allowTailOff) isReleasing = true;
        else clearCurrentNote();
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        auto* sound = static_cast<PadSound*>(getCurrentlyPlayingSound().get());
        if (sound == nullptr || sound->buffer.getNumSamples() == 0) return;

        int totalSamples = sound->buffer.getNumSamples();
        int xfadeLen = (int)(getSampleRate() * xfadeParam.load());
        
        if (xfadeLen > totalSamples / 3) xfadeLen = totalSamples / 3;
        if (xfadeLen < 1) xfadeLen = 1;

        for (int i = 0; i < numSamples; ++i) {
            if (isReleasing) {
                volEnv -= releaseRate;
                if (volEnv <= 0.0f) { clearCurrentNote(); return; }
            } else {
                if (volEnv < 1.0f) {
                    volEnv += attackRate;
                    if (volEnv > 1.0f) volEnv = 1.0f;
                }
            }

            float outL = 0.0f, outR = 0.0f;

            auto processVoice = [&](PingPongVoice& v, PingPongVoice& partner) {
                if (!v.active) return;
                
                int pos1 = (int)v.pos;
                int pos2 = pos1 + 1;
                if (pos2 >= totalSamples) pos2 = totalSamples - 1;
                
                float frac = (float)(v.pos - pos1);
                float gain = 1.0f;

                if (pos1 < xfadeLen && !v.isFirst) {
                    float alpha = (float)v.pos / (float)xfadeLen;
                    gain = std::sin(alpha * juce::MathConstants<float>::halfPi);
                } else if (pos1 >= totalSamples - xfadeLen) {
                    float alpha = (float)(v.pos - (totalSamples - xfadeLen)) / (float)xfadeLen;
                    gain = std::cos(alpha * juce::MathConstants<float>::halfPi);

                    bool crossed = (v.pos - pitchRatio < totalSamples - xfadeLen) && (v.pos >= totalSamples - xfadeLen);
                    if (crossed) {
                        partner.active = true;
                        partner.pos = 0.0;
                        partner.isFirst = false;
                    }
                }

                if (pos1 < totalSamples) {
                    float s1L = sound->buffer.getSample(0, pos1);
                    float s2L = sound->buffer.getSample(0, pos2);
                    outL += (s1L + frac * (s2L - s1L)) * gain;
                    
                    int rightCh = juce::jmin(1, sound->buffer.getNumChannels() - 1);
                    float s1R = sound->buffer.getSample(rightCh, pos1);
                    float s2R = sound->buffer.getSample(rightCh, pos2);
                    outR += (s1R + frac * (s2R - s1R)) * gain;
                }

                v.pos += pitchRatio;
                if (v.pos >= totalSamples) v.active = false;
            };

            processVoice(vA, vB);
            processVoice(vB, vA);

            outputBuffer.addSample(0, startSample + i, outL * volEnv);
            if (outputBuffer.getNumChannels() > 1) {
                outputBuffer.addSample(1, startSample + i, outR * volEnv);
            }
        }
    }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

private:
    std::atomic<float>& xfadeParam;
    double pitchRatio = 1.0;
    struct PingPongVoice {
        double pos = 0.0;
        bool active = false;
        bool isFirst = true;
    };
    PingPongVoice vA, vB;
    float volEnv = 0.0f;
    float attackRate = 0.0f;
    float releaseRate = 0.0f;
    bool isReleasing = false;
};

class SfxVoice : public juce::SynthesiserVoice {
public:
    SfxVoice(std::atomic<float>* v) : vols(v) {}
    bool canPlaySound(juce::SynthesiserSound* sound) override { return dynamic_cast<SfxSound*>(sound) != nullptr; }
    
    void startNote(int midiNote, float, juce::SynthesiserSound* sound, int) override {
        auto* sfxSound = static_cast<SfxSound*>(sound);
        if (sfxSound) {
            double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
            pitchRatio = sfxSound->sourceSampleRate / sr;
        }
        position = 0.0;
        sfxIndex = midiNote - 80;
        if (sfxIndex < 0 || sfxIndex > 7) sfxIndex = 0;
    }
    
    void stopNote(float, bool allowTailOff) override { if (!allowTailOff) clearCurrentNote(); }
    
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        auto* sound = static_cast<SfxSound*>(getCurrentlyPlayingSound().get());
        if (sound == nullptr || sound->buffer.getNumSamples() == 0) return;

        float currentVol = vols[sfxIndex].load();
        for (int i = 0; i < numSamples; ++i) {
            int pos1 = (int)position;
            int pos2 = pos1 + 1;
            if (pos2 >= sound->buffer.getNumSamples()) pos2 = sound->buffer.getNumSamples() - 1;
            
            float frac = (float)(position - pos1);
            if (pos1 >= sound->buffer.getNumSamples()) { clearCurrentNote(); return; }
            
            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
                int srcCh = juce::jmin(ch, sound->buffer.getNumChannels() - 1);
                float s1 = sound->buffer.getSample(srcCh, pos1);
                float s2 = sound->buffer.getSample(srcCh, pos2);
                outputBuffer.addSample(ch, startSample + i, (s1 + frac * (s2 - s1)) * currentVol);
            }
            position += pitchRatio;
        }
    }
    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}
private:
    std::atomic<float>* vols;
    double pitchRatio = 1.0;
    double position = 0.0;
    int sfxIndex = 0;
};

LivePadRigAudioProcessor::LivePadRigAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
    for (int i = 0; i < 8; i++) sfxIndivVolParam[i].store(0.8f);
    
    for (int i = 0; i < 12; i++) padMidiMap[i] = -1;
    for (int i = 0; i < 8; i++)  sfxMidiMap[i] = -1;

    formatManager.registerBasicFormats();
    loadSettings();
    loadMidiConfig();
    setupSynths();
}

LivePadRigAudioProcessor::~LivePadRigAudioProcessor() { saveSettings(); }

void LivePadRigAudioProcessor::setRootSoundFolder(const juce::File& folder) {
    if (folder.isDirectory()) { rootSoundDir = folder; saveSettings(); }
}
juce::File LivePadRigAudioProcessor::getRootSoundFolder() const { return rootSoundDir; }

void LivePadRigAudioProcessor::saveSettings() {
    juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig");
    appDir.createDirectory();
    juce::XmlElement xml("SETTINGS");
    xml.setAttribute("rootSoundDir", rootSoundDir.getFullPathName());
    xml.writeTo(appDir.getChildFile("settings.xml"));
}

void LivePadRigAudioProcessor::loadSettings() {
    juce::File settingsFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("settings.xml");
    if (settingsFile.existsAsFile()) {
        auto xml = juce::XmlDocument::parse(settingsFile);
        if (xml != nullptr && xml->hasTagName("SETTINGS")) {
            juce::String path = xml->getStringAttribute("rootSoundDir");
            if (path.isNotEmpty() && juce::File(path).isDirectory()) { rootSoundDir = juce::File(path); return; }
        }
    }
    rootSoundDir = juce::File("D:/AG PROJECT/assets");
    if (!rootSoundDir.exists()) rootSoundDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
}

juce::StringArray LivePadRigAudioProcessor::getAvailablePadPresets() {
    juce::StringArray list;
    juce::File padsDir = rootSoundDir.getChildFile("audio").getChildFile("pads");
    if (!padsDir.exists()) padsDir = rootSoundDir.getChildFile("pads");
    for (auto& dir : padsDir.findChildFiles(juce::File::findDirectories, false)) list.add(dir.getFileName());
    if (list.isEmpty()) list.add("darker_pad");
    return list;
}

juce::StringArray LivePadRigAudioProcessor::getAvailableSfxKits() {
    juce::StringArray list;
    juce::File drumsDir = rootSoundDir.getChildFile("audio").getChildFile("drums");
    if (!drumsDir.exists()) drumsDir = rootSoundDir.getChildFile("drums");
    for (auto& dir : drumsDir.findChildFiles(juce::File::findDirectories, false)) list.add(dir.getFileName());
    if (list.isEmpty()) list.add("acoustic_kit");
    return list;
}

void LivePadRigAudioProcessor::setupSynths() {
    padSynth.clearVoices(); sfxSynth.clearVoices();
    for (int i = 0; i < 32; ++i) padSynth.addVoice(new AmbientVoice(xfadeParam));
    for (int i = 0; i < 16; ++i) sfxSynth.addVoice(new SfxVoice(sfxIndivVolParam));
    
    auto pads = getAvailablePadPresets();
    auto drums = getAvailableSfxKits();
    if (!pads.isEmpty()) loadPadPreset(pads[0]);
    if (!drums.isEmpty()) loadSfxKit(drums[0]);
}

void LivePadRigAudioProcessor::loadPadPreset(const juce::String& folderName) {
    currentPadPresetName = folderName;

    juce::File padsDir = rootSoundDir.getChildFile("audio").getChildFile("pads");
    if (!padsDir.exists()) padsDir = rootSoundDir.getChildFile("pads");
    juce::File presetDir = padsDir.getChildFile(folderName);
    if (!presetDir.exists() || !presetDir.isDirectory()) return;

    juce::Array<juce::File> files = presetDir.findChildFiles(juce::File::findFiles, false, "*");

    struct NoteMap { juce::String token; int noteIndex; };
    NoteMap searchOrder[12] = {
        { "cs", 1 }, { "ds", 3 }, { "fs", 6 }, { "gs", 8 }, { "as", 10 },
        { "c",  0 }, { "d",  2 }, { "e",  4 }, { "f",  5 }, { "g",  7 }, { "a", 9 }, { "b", 11 }
    };

    std::vector<juce::SynthesiserSound*> tempSounds;

    for (auto& file : files) {
        if (!file.hasFileExtension("wav")) continue;
        juce::String fileName = file.getFileNameWithoutExtension().toLowerCase();

        for (int i = 0; i < 12; ++i) {
            juce::String t = searchOrder[i].token;
            int idx = searchOrder[i].noteIndex;
            if (fileName == t || fileName.endsWith("_" + t) || fileName.startsWith(t + "_") ||
                fileName.contains("-" + t) || fileName.contains(t + "-") || 
                fileName.contains(" " + t) || fileName.contains(t + " ")) 
            {
                std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
                if (reader != nullptr) {
                    tempSounds.push_back(new PadSound(*reader, 60 + idx));
                }
                break;
            }
        }
    }

    juce::ScopedLock sl(audioLock);
    padSynth.clearSounds();
    for (auto* s : tempSounds) padSynth.addSound(s);
}

void LivePadRigAudioProcessor::changePadPreset(const juce::String& folderName) {
    int activePad = -1;
    for (int i = 0; i < 12; i++) {
        if (padState[i]) {
            activePad = i;
            break;
        }
    }

    if (activePad != -1) {
        juce::ScopedLock sl(audioLock);
        for (int i = 0; i < padSynth.getNumVoices(); ++i) {
            if (auto* v = dynamic_cast<AmbientVoice*>(padSynth.getVoice(i))) {
                if (v->isVoiceActive()) {
                    v->stopNote(0.0f, true); 
                }
            }
        }
    }

    loadPadPreset(folderName);

    if (activePad != -1) {
        juce::ScopedLock sl(midiLock);
        incomingMidi.addEvent(juce::MidiMessage::noteOn(1, 60 + activePad, (juce::uint8)127), 0);
    }
}

void LivePadRigAudioProcessor::loadSfxKit(const juce::String& folderName) {
    currentSfxKitName = folderName;

    juce::File drumsDir = rootSoundDir.getChildFile("audio").getChildFile("drums");
    if (!drumsDir.exists()) drumsDir = rootSoundDir.getChildFile("drums");
    juce::File kitDir = drumsDir.getChildFile(folderName);

    std::vector<juce::SynthesiserSound*> tempSounds;
    currentSfxNames.clear();

    if (kitDir.exists() && kitDir.isDirectory()) {
        juce::Array<juce::File> files = kitDir.findChildFiles(juce::File::findFiles, false, "*.wav");
        files.sort();
        for (int i = 0; i < juce::jmin(8, files.size()); ++i) {
            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(files[i]));
            if (reader != nullptr) tempSounds.push_back(new SfxSound(*reader, 80 + i));
            currentSfxNames.add(files[i].getFileNameWithoutExtension());
        }
    }
    while (currentSfxNames.size() < 8) currentSfxNames.add("-");
    
    juce::ScopedLock sl(audioLock);
    sfxSynth.clearSounds();
    for (auto* s : tempSounds) sfxSynth.addSound(s);
}

void LivePadRigAudioProcessor::loadSampleSFX(juce::Synthesiser& synth, const juce::String& path, int midiNote) {
    juce::ignoreUnused(synth, path, midiNote);
}

void LivePadRigAudioProcessor::setPadState(int index, bool isPlaying) {
    padState[index] = isPlaying;
    juce::ScopedLock sl(midiLock);
    if (isPlaying) incomingMidi.addEvent(juce::MidiMessage::noteOn(1, 60 + index, (juce::uint8)127), 0);
    else incomingMidi.addEvent(juce::MidiMessage::noteOff(1, 60 + index, (juce::uint8)0), 0);
}

void LivePadRigAudioProcessor::triggerSFX(int sfxIndex) {
    juce::ScopedLock sl(midiLock);
    incomingMidi.addEvent(juce::MidiMessage::noteOn(1, 80 + sfxIndex, (juce::uint8)127), 0);
}

void LivePadRigAudioProcessor::stopAllAudio() {
    juce::ScopedLock sl(midiLock);
    for (int i = 0; i < 12; i++) {
        padState[i] = false;
        incomingMidi.addEvent(juce::MidiMessage::noteOff(1, 60 + i, (juce::uint8)0), 0);
    }
    for (int i = 0; i < 8; i++) incomingMidi.addEvent(juce::MidiMessage::noteOff(1, 80 + i, (juce::uint8)0), 0);
}

void LivePadRigAudioProcessor::saveUserPreset(const juce::String& presetName) {
    juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("presets");
    appDir.createDirectory();
    juce::XmlElement xml("USER_PRESET");
    
    xml.setAttribute("name", presetName);
    xml.setAttribute("padPreset", currentPadPresetName);
    xml.setAttribute("sfxKit", currentSfxKitName);

    // MEREKAM SEMUA FADER & KNOB SECARA LENGKAP
    xml.setAttribute("tone", toneParam.load());
    xml.setAttribute("hpf", hpfParam.load());
    xml.setAttribute("xfade", xfadeParam.load());
    xml.setAttribute("masterVol", masterVolParam.load());
    xml.setAttribute("padVol", padVolParam.load());
    xml.setAttribute("sfxVol", sfxVolParam.load());
    for (int i = 0; i < 8; ++i) {
        xml.setAttribute("sfxVol" + juce::String(i), sfxIndivVolParam[i].load());
    }
    
    xml.writeTo(appDir.getChildFile(presetName + ".xml"));
}

void LivePadRigAudioProcessor::loadUserPreset(const juce::String& presetName) {
    juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("presets").getChildFile(presetName + ".xml");
    if (file.existsAsFile()) {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr && xml->hasTagName("USER_PRESET")) {
            
            juce::String savedPad = xml->getStringAttribute("padPreset", "");
            if (savedPad.isNotEmpty() && savedPad != currentPadPresetName) {
                changePadPreset(savedPad);
            }
            
            juce::String savedSfx = xml->getStringAttribute("sfxKit", "");
            if (savedSfx.isNotEmpty() && savedSfx != currentSfxKitName) {
                loadSfxKit(savedSfx);
            }

            // MEMUAT KEMBALI SEMUA FADER & KNOB
            toneParam.store((float)xml->getDoubleAttribute("tone", 12000.0));
            hpfParam.store((float)xml->getDoubleAttribute("hpf", 20.0));
            xfadeParam.store((float)xml->getDoubleAttribute("xfade", 4.0));
            masterVolParam.store((float)xml->getDoubleAttribute("masterVol", 0.8));
            padVolParam.store((float)xml->getDoubleAttribute("padVol", 0.8));
            sfxVolParam.store((float)xml->getDoubleAttribute("sfxVol", 0.8));
            for (int i = 0; i < 8; ++i) {
                sfxIndivVolParam[i].store((float)xml->getDoubleAttribute("sfxVol" + juce::String(i), 0.8));
            }
        }
    }
}

void LivePadRigAudioProcessor::deleteUserPreset(const juce::String& presetName) {
    juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("presets").getChildFile(presetName + ".xml");
    if (file.existsAsFile()) file.deleteFile();
}

juce::StringArray LivePadRigAudioProcessor::getUserPresetNames() {
    juce::StringArray list;
    juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("presets");
    if (appDir.isDirectory()) {
        for (auto& f : appDir.findChildFiles(juce::File::findFiles, false, "*.xml")) list.add(f.getFileNameWithoutExtension());
    }
    return list;
}

void LivePadRigAudioProcessor::startMidiLearn(int type, int index) {
    learnTargetType = type; 
    learnTargetIndex = index;
    isMidiLearning.store(true);
}

void LivePadRigAudioProcessor::cancelMidiLearn() {
    isMidiLearning.store(false); 
    learnTargetType = -1; 
    learnTargetIndex = -1;
}

void LivePadRigAudioProcessor::saveMidiConfig() {
    saveMidiPreset("default_map");
}

void LivePadRigAudioProcessor::loadMidiConfig() {
    loadMidiPreset("default_map");
}

void LivePadRigAudioProcessor::clearMidiConfig() {
    for (int i = 0; i < 12; i++) padMidiMap[i] = -1;
    for (int i = 0; i < 8; i++)  sfxMidiMap[i] = -1;
    saveMidiConfig();
}

void LivePadRigAudioProcessor::saveMidiPreset(const juce::String& presetName) {
    juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("midi_presets");
    appDir.createDirectory();
    juce::XmlElement xml("MIDI_MAP");
    for (int i = 0; i < 12; i++) xml.setAttribute("pad_" + juce::String(i), padMidiMap[i]);
    for (int i = 0; i < 8; i++)  xml.setAttribute("sfx_" + juce::String(i), sfxMidiMap[i]);
    xml.writeTo(appDir.getChildFile(presetName + ".xml"));
}

void LivePadRigAudioProcessor::loadMidiPreset(const juce::String& presetName) {
    juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("midi_presets").getChildFile(presetName + ".xml");
    if (file.existsAsFile()) {
        auto xml = juce::XmlDocument::parse(file);
        if (xml != nullptr && xml->hasTagName("MIDI_MAP")) {
            for (int i = 0; i < 12; i++) padMidiMap[i] = xml->getIntAttribute("pad_" + juce::String(i), -1);
            for (int i = 0; i < 8; i++)  sfxMidiMap[i] = xml->getIntAttribute("sfx_" + juce::String(i), -1);
            return;
        }
    }
    for (int i = 0; i < 12; i++) padMidiMap[i] = -1;
    for (int i = 0; i < 8; i++)  sfxMidiMap[i] = -1;
}

void LivePadRigAudioProcessor::deleteMidiPreset(const juce::String& presetName) {
    juce::File file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("midi_presets").getChildFile(presetName + ".xml");
    if (file.existsAsFile()) file.deleteFile();
}

juce::StringArray LivePadRigAudioProcessor::getMidiPresetNames() {
    juce::StringArray list;
    juce::File appDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("LivePadRig").getChildFile("midi_presets");
    if (appDir.isDirectory()) {
        for (auto& f : appDir.findChildFiles(juce::File::findFiles, false, "*.xml")) {
            if (f.getFileNameWithoutExtension() != "default_map")
                list.add(f.getFileNameWithoutExtension());
        }
    }
    return list;
}

const juce::String LivePadRigAudioProcessor::getName() const { return JucePlugin_Name; }
bool LivePadRigAudioProcessor::acceptsMidi() const { return true; }
bool LivePadRigAudioProcessor::producesMidi() const { return false; }
bool LivePadRigAudioProcessor::isMidiEffect() const { return false; }
double LivePadRigAudioProcessor::getTailLengthSeconds() const { return 999999.0; }
int LivePadRigAudioProcessor::getNumPrograms() { return 1; }
int LivePadRigAudioProcessor::getCurrentProgram() { return 0; }
void LivePadRigAudioProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }
const juce::String LivePadRigAudioProcessor::getProgramName (int index) { juce::ignoreUnused (index); return {}; }
void LivePadRigAudioProcessor::changeProgramName (int index, const juce::String& newName) { juce::ignoreUnused (index, newName); }

void LivePadRigAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = sampleRate;
    padSynth.setCurrentPlaybackSampleRate(sampleRate);
    sfxSynth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < 2; ++i) { padLpf[i].reset(); padHpf[i].reset(); }
}

void LivePadRigAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LivePadRigAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const { 
    juce::ignoreUnused (layouts); 
    return true; 
}
#endif

void LivePadRigAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        bool isTrigger = msg.isNoteOn() || (msg.isController() && msg.getControllerValue() > 63);

        if (isTrigger) {
            int noteNum = msg.isNoteOn() ? msg.getNoteNumber() : (msg.getControllerNumber() + 128);
            
            if (isMidiLearning.load() && learnTargetType != -1) {
                if (learnTargetType == 0 && learnTargetIndex >= 0 && learnTargetIndex < 12) padMidiMap[learnTargetIndex] = noteNum;
                else if (learnTargetType == 1 && learnTargetIndex >= 0 && learnTargetIndex < 8) sfxMidiMap[learnTargetIndex] = noteNum;
                
                isMidiLearning.store(false);
                needToSaveMidi.store(true);
                continue;
            }
            
            for (int i = 0; i < 12; ++i) {
                if (padMidiMap[i] != -1 && padMidiMap[i] == noteNum) setPadState(i, !padState[i]); 
            }
            for (int i = 0; i < 8; ++i) {
                if (sfxMidiMap[i] != -1 && sfxMidiMap[i] == noteNum) triggerSFX(i);
            }
        }
    }

    buffer.clear();

    juce::MidiBuffer synthMidi;
    {
        juce::ScopedLock sl(midiLock);
        synthMidi.addEvents(incomingMidi, 0, buffer.getNumSamples(), 0);
        incomingMidi.clear();
    }

    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0) return;

    padBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    sfxBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(), false, false, true);
    padBuffer.clear(); sfxBuffer.clear();

    {
        juce::ScopedLock sl(audioLock);
        padSynth.renderNextBlock(padBuffer, synthMidi, 0, buffer.getNumSamples());
        sfxSynth.renderNextBlock(sfxBuffer, synthMidi, 0, buffer.getNumSamples());
    }

    float t = toneParam.load();
    float h = hpfParam.load();
    float maxCutoff = (float)(currentSampleRate * 0.48);
    if (maxCutoff < 100.0f) maxCutoff = 20000.0f;
    t = juce::jlimit(20.0f, maxCutoff, t);
    h = juce::jlimit(20.0f, maxCutoff, h);

    for (int ch = 0; ch < 2; ++ch) {
        padLpf[ch].setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, t));
        padHpf[ch].setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, h));
    }

    for (int ch = 0; ch < padBuffer.getNumChannels(); ++ch) {
        if (ch < 2) {
            padLpf[ch].processSamples(padBuffer.getWritePointer(ch), padBuffer.getNumSamples());
            padHpf[ch].processSamples(padBuffer.getWritePointer(ch), padBuffer.getNumSamples());
        }
    }

    float mVol = masterVolParam.load(); float pVol = padVolParam.load(); float sVol = sfxVolParam.load();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        buffer.addFrom(ch, 0, padBuffer, ch, 0, buffer.getNumSamples(), pVol * mVol);
        buffer.addFrom(ch, 0, sfxBuffer, ch, 0, buffer.getNumSamples(), sVol * mVol);
    }
}

bool LivePadRigAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LivePadRigAudioProcessor::createEditor() { return new LivePadRigAudioProcessorEditor (*this); }
void LivePadRigAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { juce::ignoreUnused (destData); }
void LivePadRigAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { juce::ignoreUnused (data, sizeInBytes); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LivePadRigAudioProcessor(); }