/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SynthUsingMidiInputTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Synthesiser with midi input.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2019, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once
double MinSec = 15.0/14.0;
double MajSec = 8.0 / 7.0;
double MinThi = 6.0/5.0;
double MajThi = 5.0 / 4.0;
double Fou = 4.0/3.0;
double TT = 7.0/5.0;
double Fif = 3.0/2.0;
double MinSix = 8.0/5.0;
double MajSix = 5.0/3.0;
double MinSev = 7.0/4.0;
double MajSev = 15.0/8.0;
double Oct = 2.0;
int tempNum = -2;
double hertzNum = 0.0;
int minimum = -2;
bool firstTime = true;

std::vector<int> notes;
#pragma once

//==============================================================================
struct SineWaveSound   : public juce::SynthesiserSound
{
    SineWaveSound() {}

    bool appliesToNote    (int) override        { return true; }
    bool appliesToChannel (int) override        { return true; }
};

//==============================================================================
struct SineWaveVoice   : public juce::SynthesiserVoice
{
    SineWaveVoice() {}

    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }
    
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        // base case for 1st note pressed (simply play the note as if it's equal temperament)
        if (firstTime == true) {
            tempNum = midiNoteNumber;
            hertzNum = juce::MidiMessage::getMidiNoteInHertz(tempNum);
            firstTime = false;
        }
        // case for next notes
        else {
            // only alter note to tune to if there is harmony
            if (notes.size() > 1) {
                // find the lowest note (bass note)
                auto min = min_element(notes.begin(), notes.end());
                minimum = *min;

                // tune any note that isn't the bass note according to the current tuning system
                if (tempNum != -2 && tempNum != minimum) {
                    int tempInterval = minimum - tempNum;
                    int tempOctave = (tempInterval - (tempInterval % 12)) / 12;
                    tempInterval = tempInterval % 12;
                    if (tempInterval <= 0) {
                        tempInterval += 12;
                        tempOctave -= 1;                    }
                    hertzNum *= ratioTable(tempInterval) * (pow(Oct, tempOctave));
                    tempNum = minimum;
                }

            }
        }

        
        intervalNum = midiNoteNumber - tempNum;
        octaveNum = (intervalNum - (intervalNum % 12)) / 12;
        intervalNum = intervalNum % 12;
        if (intervalNum < 0) {
            intervalNum += 12;
            octaveNum -= 1;
        }
        ratioNum = ratioTable(intervalNum);
        if (intervalNum == 0) {
            ratioNum = 1.0;

        }
        
        double cyclesPerSecond = hertzNum * ratioNum * (pow(Oct, octaveNum));
        double cyclesPerSample = (cyclesPerSecond) / getSampleRate();
        angleDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        
    }

    double ratioTable(int interval) {
        if (interval == 1) {
            return MinSec;
        }
        else if (interval == 2) {
            return MajSec;
        }
        else if (interval == 3) {
            return MinThi;
        }
        else if (interval == 4) {
            return MajThi;
        }
        else if (interval == 5) {
            return Fou;
        }
        else if (interval == 6) {
            return TT;
        }
        else if (interval == 7) {
            return Fif;
        }
        else if (interval == 8) {
            return MinSix;
        }
        else if (interval == 9) {
            return MajSix;
        }
        else if (interval == 10) {
            return MinSev;
        }
        else if (interval == 11) {
            return MajSev;
        }
        return Oct;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;
            
        }
        else
        {   
            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0) // [7]
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (std::sin (currentAngle) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99; // [8]

                    if (tailOff <= 0.005)
                    {
                        clearCurrentNote(); // [9]

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0) // [6]
                {
                    auto currentSample = (float) (std::sin (currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }
    using SynthesiserVoice::renderNextBlock;

private:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
    int intervalNum = 0;
    int octaveNum = 0;
    double ratioNum = 1.0;
};

//=============================================================================

class MySamplerVoice : public juce::SamplerVoice {
public:
    MySamplerVoice() {}

    // Destructor
    ~MySamplerVoice() override {}

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void startNote(int midiNoteNumber, float velocity,
        juce::SynthesiserSound* s, int /*currentPitchWheelPosition*/) override
    {
        const SamplerSound* const sound = dynamic_cast<const SamplerSound*>(s);
        jassert(sound != 0);
        if (sound != 0) {
            if (firstTime == true) {
                hertzNum = 1.0;
                tempNum = midiNoteNumber;
                if (tempNum != -2 && tempNum != 60) {
                    int tempInterval = tempNum - 60;
                    int tempOctave = (tempInterval - (tempInterval % 12)) / 12;
                    tempInterval = tempInterval % 12;
                    if (tempInterval <= 0) {
                        tempInterval += 12;
                        tempOctave -= 1;
                    }
                    hertzNum *= ratioTable(tempInterval) * (pow(Oct, tempOctave));
                }
                firstTime = false;
            }
            else {
                if (notes.size() > 1) {
                    auto min = min_element(notes.begin(), notes.end());
                    minimum = *min;
                    if (tempNum != -2 && tempNum != minimum) {
                        int tempInterval = minimum - tempNum;
                        int tempOctave = (tempInterval - (tempInterval % 12)) / 12;
                        tempInterval = tempInterval % 12;
                        if (tempInterval <= 0) {
                            tempInterval += 12;
                            tempOctave -= 1;
                        }
                        hertzNum *= ratioTable(tempInterval) * (pow(Oct, tempOctave));
                        tempNum = minimum;
                    }

                }
            }


            intervalNum = midiNoteNumber - tempNum;
            octaveNum = (intervalNum - (intervalNum % 12)) / 12;
            intervalNum = intervalNum % 12;
            if (intervalNum < 0) {
                intervalNum += 12;
                octaveNum -= 1;
            }
            ratioNum = ratioTable(intervalNum);
            if (intervalNum == 0) {
                ratioNum = 1.0;

            }

            pitchRatio = ratioNum * (pow(Oct, octaveNum)) * hertzNum;
            sourceSamplePosition = 0.0;
            lgain = velocity;
            rgain = velocity;

            adsr.setSampleRate(sound->sourceSampleRate);
            adsr.setParameters(sound->params);

            adsr.noteOn();
        }
    }

    double ratioTable(int interval) {
        if (interval == 1) {
            return MinSec;
        }
        else if (interval == 2) {
            return MajSec;
        }
        else if (interval == 3) {
            return MinThi;
        }
        else if (interval == 4) {
            return MajThi;
        }
        else if (interval == 5) {
            return Fou;
        }
        else if (interval == 6) {
            return TT;
        }
        else if (interval == 7) {
            return Fif;
        }
        else if (interval == 8) {
            return MinSix;
        }
        else if (interval == 9) {
            return MajSix;
        }
        else if (interval == 10) {
            return MinSev;
        }
        else if (interval == 11) {
            return MajSev;
        }
        return Oct;
    }
    
    using SynthesiserVoice::renderNextBlock;

private:
    int intervalNum = 0;
    int octaveNum = 0;
    double ratioNum = 1.0;
};


//==============================================================================
class SynthAudioSource   : public juce::AudioSource

{
public:
    SynthAudioSource (juce::MidiKeyboardState& keyState)
        : keyboardState (keyState)
    {
        
        for (auto i = 0; i < 12; ++i) {               // [1]
            synth.addVoice(new SineWaveVoice());
            synth.addVoice(new MySamplerVoice());
        }
        setUsingSineWaveSound(); // [2]
    }

    juce::MidiMessageCollector* getMidiCollector()
    {
        return &midiCollector;
    }

    void setUsingSineWaveSound()
    {
        firstTime = true;
        synth.clearSounds();
        synth.addSound(new SineWaveSound());
    }

    void setUsingSampledSound()
    {
        firstTime = true;
        synth.clearSounds();
        mFormatManager.registerBasicFormats();
        myChooser = std::make_unique<juce::FileChooser>("Please select the wav you want to load...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav");

        auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories;
        myChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
            {
                juce::File wavFile = chooser.getResult();
                    mFormatReader = mFormatManager.createReaderFor(wavFile);
                    if (mFormatReader != nullptr) {
                        BigInteger range;
                        range.setRange(0, 128, true);
                        
                        synth.addSound(new SamplerSound("demo sound",
                            *mFormatReader,
                            range,
                            60,   // root midi note
                            0.0,  // attack time
                            0.1,  // release time
                            1000.0  // maximum sample length
                        ));  
                    }
            });
    }

    void prepareToPlay(int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        midiCollector.reset(sampleRate); // [10]
    }

    void releaseResources() override {}

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        bufferToFill.clearActiveBufferRegion();

        juce::MidiBuffer incomingMidi;
        midiCollector.removeNextBlockOfMessages(incomingMidi, bufferToFill.numSamples); // [11]

        keyboardState.processNextMidiBuffer(incomingMidi, bufferToFill.startSample,
            bufferToFill.numSamples, true);

        synth.renderNextBlock(*bufferToFill.buffer, incomingMidi,
            bufferToFill.startSample, bufferToFill.numSamples);
    }
private:
    juce::MidiKeyboardState& keyboardState;
    juce::Synthesiser synth;
    juce::MidiMessageCollector midiCollector;
    AudioFormatManager mFormatManager;
    AudioFormatReader* mFormatReader{ nullptr };
    std::unique_ptr<FileChooser> myChooser;
};

//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent,
                               private juce::MidiKeyboardStateListener,
                               private juce::Timer
{
public:
    MainContentComponent()
        : synthAudioSource  (keyboardState),
          keyboardComponent (keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)

    {
        addAndMakeVisible (keyboardComponent);
        keyboardState.addListener (this);
        setAudioChannels (0, 2);
        addAndMakeVisible(sineButton);
        sineButton.setRadioGroupId(321);
        sineButton.setToggleState(true, dontSendNotification);
        sineButton.onClick = [this] { synthAudioSource.setUsingSineWaveSound(); };

        addAndMakeVisible(sampledButton);
        sampledButton.setRadioGroupId(321);
        sampledButton.onClick = [this] { synthAudioSource.setUsingSampledSound(); };

        addAndMakeVisible(resetButton);
        resetButton.setToggleable(false);
        resetButton.onClick = [this] {firstTime = true;  };

        audioSourcePlayer.setSource(&synthAudioSource);

        setSize (600, 160);
        startTimer (400);
        //shiri irish marhc 17 **IMPORTANT**
        addAndMakeVisible(limitInputListLabel);
        limitInputListLabel.setFont(textFont);
        addAndMakeVisible(limitInputList);
        limitInputList.addItem("3-Limit (Pythagorean)", 1);
        limitInputList.addItem("5-Limit", 2);
        limitInputList.addItem("7-Limit", 3);
        limitInputListLabel.attachToComponent(&limitInputList, true);
        limitInputList.onChange = [this] { limitInputListChanged(); };

        addAndMakeVisible(midiInputListLabel);
        midiInputListLabel.setText("MIDI Input:", juce::dontSendNotification);
        midiInputListLabel.attachToComponent(&midiInputList, true);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        addAndMakeVisible(midiInputList);
        midiInputList.setTextWhenNoChoicesAvailable("No MIDI Inputs Enabled");
        juce::StringArray midiInputNames;
        for (auto input : midiInputs)
            midiInputNames.add(input.name);

        midiInputList.addItemList(midiInputNames, 1);
        midiInputList.onChange = [this] { setMidiInput(midiInputList.getSelectedItemIndex()); };

        for (auto input : midiInputs)
        {
            if (deviceManager.isMidiInputDeviceEnabled(input.identifier))
            {
                setMidiInput(midiInputs.indexOf(input));
                break;
            }
        }

        if (midiInputList.getSelectedId() == 0)
            setMidiInput(0);

    }

    ~MainContentComponent() override
    {
        audioSourcePlayer.setSource(nullptr);
        shutdownAudio();
    }

    void limitInputListChanged() {
        switch (limitInputList.getSelectedId())
        {
        case 1:
            MinSec = 256.0/243.0;
            MajSec = 9.0 / 8.0;
            MinThi = 32.0/27.0;
            MajThi = 81.0/64.0;
            Fou = 4.0 / 3.0;
            TT = 729.0/512.0;
            Fif = 3.0 / 2.0;
            MinSix = 128.0/81.0;
            MajSix = 27.0/16.0;
            MinSev = 16.0/9.0;
            MajSev = 243.0/128.0;
            Oct = 2.0;
            break;
        case 2:
            MinSec = 16.0 / 15.0;
            MajSec = 9.0/8.0;
            MinThi = 6.0 / 5.0;
            MajThi = 5.0 / 4.0;
            Fou = 4.0 / 3.0;
            TT = 25.0/18.0;
            Fif = 3.0 / 2.0;
            MinSix = 8.0 / 5.0;
            MajSix = 5.0 / 3.0;
            MinSev = 9.0/5.0;
            MajSev = 15.0 / 8.0;
            Oct = 2.0;
            break;
        case 3:
            MinSec = 15.0 / 14.0;
            MajSec = 8.0 / 7.0;
            MinThi = 6.0 / 5.0;
            MajThi = 5.0 / 4.0;
            Fou = 4.0 / 3.0;
            TT = 7.0 / 5.0;
            Fif = 3.0 / 2.0;
            MinSix = 8.0 / 5.0;
            MajSix = 5.0 / 3.0;
            MinSev = 7.0 / 4.0;
            MajSev = 15.0 / 8.0;
            Oct = 2.0;
            break;
        default: break;
        }
    }

    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override {
        notes.push_back(midiNoteNumber);
    }

    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/) override {
        notes.erase(std::remove(notes.begin(), notes.end(), midiNoteNumber), notes.end());
    }

    void setMidiInput(int index)
    {
        auto list = juce::MidiInput::getAvailableDevices();

        deviceManager.removeMidiInputDeviceCallback(list[lastInputIndex].identifier,
            synthAudioSource.getMidiCollector()); // [12]

        auto newInput = list[index];

        if (!deviceManager.isMidiInputDeviceEnabled(newInput.identifier))
            deviceManager.setMidiInputDeviceEnabled(newInput.identifier, true);

        deviceManager.addMidiInputDeviceCallback(newInput.identifier, synthAudioSource.getMidiCollector()); // [13]
        midiInputList.setSelectedId(index + 1, juce::dontSendNotification);

        lastInputIndex = index;
    }

    void resized() override
    {
        midiInputList.setBounds(50, 10, getWidth() - 210, 20);
        limitInputList.setBounds(50, 30, getWidth() - 350, 20);
        sineButton.setBounds(16, getHeight() - 50, 150, 24);
        sampledButton.setBounds(16, getHeight() - 30, 150, 24);
        resetButton.setBounds(180, getHeight() - 45, 150, 36);
        keyboardComponent.setBounds(10, 50, getWidth() - 20, getHeight() - 100);
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
        synthAudioSource.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override
    {
        synthAudioSource.releaseResources();
    }

private:
    void timerCallback() override
    {
        keyboardComponent.grabKeyboardFocus();
        stopTimer();
    }

    //==========================================================================
    juce::MidiKeyboardState keyboardState;
    AudioSourcePlayer audioSourcePlayer;
    SynthAudioSource synthAudioSource;
    juce::MidiKeyboardComponent keyboardComponent;
    ToggleButton sineButton{ "Use sine wave" };
    ToggleButton sampledButton{ "Use sampled sound" };
    TextButton resetButton{ "Reset Pitch Drift" };
    juce::ComboBox limitInputList;
    juce::ComboBox midiInputList;
    juce::Label limitInputListLabel { {}, "Choose Limit:"};
    juce::Label midiInputListLabel;
    juce::Font textFont { 12.0f };
    int lastInputIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
