#pragma once

#include <JuceHeader.h>
#include <vector>
#include "AudioData.h" // audio buffer
#include "Reverb.h"    // moorer reverb filter

//==============================================================================
class MainComponent  : public juce::AudioAppComponent, private juce::FilenameComponentListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // variables
    const unsigned numCombs; // number of parallel comb filters
    AudioData * input;
    AudioData * output;
    AudioData * data;
    Reverb reverb;  // moorer reverb filter
    bool playing;   // audio is playing
    bool reverbOn;  // turn reverb on/off
    unsigned tail;  // reverb tail in ms
    
    const int width;  // default window width
    const int height; // default window height
    
    int sample;
    int total_samples;
    
    // audio device settings
    juce::AudioDeviceManager::AudioDeviceSetup setup;
        
    void UpdateAudioData(std::string filename);
    
    //==============================================================================
    // GUI objects

    // file selection
    std::unique_ptr<juce::FilenameComponent> fileComp;
    std::unique_ptr<juce::TextEditor> fileText;
    
    enum ButtonID { bID = 1001, tID = 1002 };
    
    // playback
    juce::ToggleButton reverbOnOff;
    juce::Label reverbOnLabel;
    juce::TextButton play;
    
    // slider info
    struct MrSlider
    {
        juce::Slider slider;
        juce::Label label;
    };
    
    // comb filter parameters
    struct CombSliderGroup
    {
        int ID; // group ID
        MrSlider L; //
        MrSlider R;
        MrSlider G;
        MrSlider ZF;
        
        juce::Label Header;
    };
    
    
    
    juce::Label paramHeader; // parameter header
    juce::Label apHeader;    // allpass header
    MrSlider aSlider;        // allpass coefficient
    MrSlider mSlider;        // allpass delay (ms)
    
    juce::Label ratioHeader; // dry:wet ratio header
    MrSlider drySlider;      // dry signal ratio (%)
    MrSlider wetSlider;      // wet signal ratio (%)
    
    std::vector<CombSliderGroup*> combGroups; // comb filter sliders
    
    //==============================================================================
    // funtions
    void filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged) override;
    void ReadFile(juce::File file);
    void TryOpenWAV(std::string filename);
    
    void InitHeader(juce::Label * label, juce::String text);
    void InitSlider(juce::Slider *slider, juce::Label * label, float min, float max, float def,
                    juce::String suffix, unsigned decimals, float interval = 0,
                    std::function<void()> callback = nullptr);
    
    
    void ResizeHeader(juce::Label * header, int x, int y);
    void ResizeSlider(juce::Slider* slider, int x, int y, int w, int h);
    
    void UpdateSliderGroup(CombSliderGroup * group);
    
//    void sliderValueChanged(juce::Slider *slider) override;
    
    void InitButton(juce::Button * button, int groupID, std::function<void()> callback = nullptr);
//    void buttonClicked(juce::Button* button) override;
    void UpdateToggleState(juce::Button* button, juce::String name);
   
    void PlayClicked();
    
    
    void ApplyReverb();
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
