#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : numCombs(6), input(nullptr), output(nullptr), data(nullptr), reverb(), playing(false), reverbOn(false), tail(1000), width(1100), height(700), sample(0), total_samples(0)
{
    // file selection component
    fileComp.reset (new juce::FilenameComponent ("fileComp",
                                                 {},                       // current file
                                                 false,                    // can edit file name,
                                                 false,                    // is directory,
                                                 false,                    // is for saving,
                                                 {},                       // browser wildcard suffix,
                                                 {},                       // enforced suffix,
                                                 "Select file to open"));  // text when nothing selected
    addAndMakeVisible(fileComp.get());
    fileComp->addListener(this);
    
    // file selection text
    fileText.reset (new juce::TextEditor());
    fileText->setMultiLine(true);
    fileText->setReadOnly(true);
    fileText->setCaretVisible(false);
    addAndMakeVisible(fileText.get());
    
    // play
    play.setButtonText("Play");
    InitButton(&play, bID, [this]{ PlayClicked(); });
    
    // reverb on/off
    reverbOnOff.setButtonText("Reverb On");
    InitButton(&reverbOnOff, tID, [this]{ UpdateToggleState(&reverbOnOff, "reverbOn"); });
    reverbOnOff.setBounds(600, 160, reverbOnOff.getWidth(), 30);
    reverbOnOff.changeWidthToFitText();
    
    // parameter header
    paramHeader.setFont(juce::Font (26.0f, juce::Font::bold | juce::Font::underlined));
    paramHeader.setText("Reverb Parameters", juce::dontSendNotification);
    addAndMakeVisible(paramHeader);
    
    // general parameters
    InitHeader(&ratioHeader, "Dry/Wet Ratio");
    drySlider.label.setText("K", juce::dontSendNotification);
    drySlider.slider.onValueChange = [this] {reverb.SetDryPercetage(drySlider.slider.getValue());
        wetSlider.slider.setValue(100 - drySlider.slider.getValue(), juce::dontSendNotification); };
    InitSlider(&drySlider.slider, &drySlider.label, 0, 100, 90, "%", 0, 1);
    
    wetSlider.label.setText("1-K", juce::dontSendNotification);
    wetSlider.slider.onValueChange = [this] { drySlider.slider.setValue(100 - wetSlider.slider.getValue()); };
    InitSlider(&wetSlider.slider, &wetSlider.label, 0, 100, 10, "%", 0, 1);
    
    // allpass filter parameters
    InitHeader(&apHeader, "All-Pass Filter");
    
    aSlider.label.setText("a", juce::dontSendNotification);
    InitSlider(&aSlider.slider, &aSlider.label, 0, 1, reverb.GetAllPassCoeff(), "", 2);
    aSlider.slider.onValueChange = [this] {reverb.SetAllPassCoeff(aSlider.slider.getValue()); };
    
    mSlider.label.setText("m", juce::dontSendNotification);
    InitSlider(&mSlider.slider, &mSlider.label, 0, 100, reverb.GetAllPassDelay(), "ms", 0, 1);
    mSlider.slider.onValueChange = [this] {reverb.SetAllPassDelay(mSlider.slider.getValue()); };

    
    // comb filter parameters
    for (int i = 0; i < numCombs; ++i) {
        
        CombSliderGroup * group = new CombSliderGroup;
        group->ID = i;
        InitHeader(&group->Header, "Comb" + std::to_string(i + 1)); // init group header
        
        // L value sliders
        group->L.label.setText("L", juce::dontSendNotification);
        InitSlider(&group->L.slider, &group->L.label, 0, 100, reverb.GetCombDelay(i), "ms", 0, 1);
        group->L.slider.onValueChange = [this, group] {reverb.SetCombDelay(group->L.slider.getValue(), group->ID);};
        
        // g value sliders
        group->G.label.setText("g", juce::dontSendNotification);
        InitSlider(&group->G.slider, &group->G.label, 0, .99, reverb.GetCombLowPassCoeff(i), "", 4);
        group->G.slider.onValueChange = [this, group] {reverb.SetCombLowPassCoeff(group->G.slider.getValue(), group->ID); UpdateSliderGroup(group);};
        
        // R value sliders
        group->R.label.setText("R", juce::dontSendNotification);
        InitSlider(&group->R.slider, &group->R.label, 0, .99, reverb.GetCombGainConstant(i), "", 4);
        group->R.slider.onValueChange = [this, group] {reverb.SetCombGainConstant(group->R.slider.getValue(), group->ID); UpdateSliderGroup(group);};
        
        // ZF value sliders
        group->ZF.label.setText("R/(1-g)", juce::dontSendNotification);
        InitSlider(&group->ZF.slider, &group->ZF.label, 0, 0.99, reverb.GetCombZeroFreqGain(i), "", 2, 0.01f);
        group->ZF.slider.onValueChange = [this, group] {reverb.SetCombZeroFreqGain(group->ZF.slider.getValue(), group->ID); UpdateSliderGroup(group);};
        
        combGroups.push_back(group);
    }
    
    
    // set the size of the component after adding child components
    setSize(width, height);
    
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (0, 2);
    }
    
    // want to be able to specify sampling rate
    setup = deviceManager.getAudioDeviceSetup();
    deviceManager.setAudioDeviceSetup(setup, true);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
    
    if (input) {
        delete input;
        input = nullptr;
    }
    if (output) {
        delete output;
        output = nullptr;
    }
    
    for (CombSliderGroup * csg : combGroups) {
        delete csg;
        csg = nullptr;
    }
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    // This function will be called when the audio device is started, or when
    // its settings (i.e. sample rate, block size, etc) are changed.

    // You can use this function to initialise any resources you might need,
    // but be careful - it will be called on the audio thread, not the GUI thread.

    // For more details, see the help for AudioProcessor::prepareToPlay()
    playing = false;
    data = nullptr;
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Right now we are not producing any data, in which case we need to clear the buffer
    // (to prevent the output of random noise)
    if (!playing) {
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    else if (sample >= total_samples) {
        playing = false;
        data = nullptr;
        bufferToFill.clearActiveBufferRegion();
        return;
    }
    
    int nChannels = bufferToFill.buffer->getNumChannels();
    int nSamples = bufferToFill.numSamples;

    int hold = sample;
    
    // send filtered signal to output
    for (int j = 0; j < nChannels; ++j) {
        auto* buffer = bufferToFill.buffer->getWritePointer(j, bufferToFill.startSample);
        sample = hold;
        
        for (int i = 0; i < nSamples; ++i, ++sample) {
            if (sample >= total_samples) {
                buffer[i] = 0.0;
                continue;
            }
            buffer[i] = data->sample(sample);
        }
        
    }
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
    playing = false;
    data = nullptr;
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    int x = 50;  // component top left x position
    int y = 25;  // component top left y position
    int w = 200; // component width
    int h = 30;  // component height
    
    int vert_hold = y;
    
    // file selection
    fileComp->setSize(w, h);
    fileComp->setTopLeftPosition(x, y);
    fileText->setSize(w, h - 10);
    fileText->setTopLeftPosition(x, y + 35);
    
    // playback
    y += 100;
    reverbOnOff.setTopLeftPosition(x, y);
    play.setSize(95, 30);
    play.setTopLeftPosition(x, y + 40);
    
    x = 320;
    y = vert_hold;
    w = 50;  // slider width
    h = 120; // slider height
    int row_spacer = h + 20;
    int col_spacer = w + 5;

    
    // general parameters
    ResizeHeader(&paramHeader, x, y);
    y += ratioHeader.getFont().getHeight() + 20;
    vert_hold = y;
    int horz_hold = x;
    
    // dry/wet percentage
    ResizeHeader(&ratioHeader, x, y);
    y += ratioHeader.getFont().getHeight() + drySlider.label.getHeight();
    ResizeSlider(&drySlider.slider, x, y, w, h);
    ResizeSlider(&wetSlider.slider, x + col_spacer, y, w, h);

    // allpass parameters
    y = vert_hold;
    x += col_spacer * 3;
    
    ResizeHeader(&apHeader, x, y);
    y += apHeader.getFont().getHeight() + aSlider.label.getHeight();
    ResizeSlider(&aSlider.slider, x, y, w, h);
    ResizeSlider(&mSlider.slider, x + col_spacer, y, w, h);
    
    // comb parameters
    y += row_spacer;
    vert_hold = y;
    x = horz_hold;

    // comb slider groups
    for (int i = 0; i < numCombs; ++i) {
        
        CombSliderGroup * group = combGroups[i];
        ResizeHeader(&group->Header, x, y);
        
        y += group->Header.getFont().getHeight() + group->L.label.getHeight();
        
        // L value slider
        ResizeSlider(&group->L.slider, x, y, w, h);
        
        x += col_spacer;
        
        // g value slider
        ResizeSlider(&group->G.slider, x, y, w, h);
        
        x += col_spacer;
        
        // R value slider
        ResizeSlider(&group->R.slider, x, y, w, h);
        
        x += col_spacer;
        
        // ZF value slider
        ResizeSlider(&group->ZF.slider, x, y, w, h);
        
        // next column
        if ((i + 1) % 3 != 0) {
            y = vert_hold;
            x += col_spacer * 2;
        }
        else { // next row
            x = horz_hold;
            y += row_spacer;
            vert_hold = y;
        }
    }
    
}

//==============================================================================
// File Selection

// filename component change listener callback
void MainComponent::filenameComponentChanged(juce::FilenameComponent* fileComponentThatHasChanged)
{
    fileText->setText(""); // reset error text
    
    if (fileComponentThatHasChanged == fileComp.get()) {
        ReadFile(fileComp->getCurrentFile());
    }
}

// read in a file
void MainComponent::ReadFile(juce::File file)
{
    // dont read in a new file while audio is playing
    if (playing) {
        fileText->setText("Error: Cannot load file while audio is playing");
        return;
    }
    
    // make sure file exists
    if (!file.existsAsFile()) {
        fileText->setText("Error: File does not exist");
        return;
    }
    
    // make sure file is wav
    if (!file.hasFileExtension("wav")) {
        fileText->setText("Error: File is not a wav file");
        return;
    }
    
    // open wav file
    UpdateAudioData(file.getFullPathName().toStdString());
    
}

void MainComponent::UpdateAudioData(std::string filename)
{
    if (input) {
        delete input;
        input = nullptr;
    }
    
    input = new AudioData(filename.c_str());
    if (input == nullptr) {
        fileText->setText("Error: File failed to load");
    }
}


//==============================================================================
// Sliders

// initialize slider group header
void MainComponent::InitHeader(juce::Label * label, juce::String text)
{
    label->setFont(juce::Font (16.0f, juce::Font::bold));
    label->setText(text, juce::dontSendNotification);
    addAndMakeVisible(label);
}

// resize header
void MainComponent::ResizeHeader(juce::Label * header, int x, int y)
{
    juce::Font font = header->getFont();
    int hh = font.getHeight(); // header height
    header->setSize(font.getStringWidthFloat(header->getText()), hh);
    header->setTopLeftPosition(x, y);
}

void MainComponent::ResizeSlider(juce::Slider* slider, int x, int y, int w, int h)
{
    slider->setSize(w, h);
    slider->setTopLeftPosition(x, y);
}

// initialize a slider
void MainComponent::InitSlider(juce::Slider *slider, juce::Label * label, float min, float max, float def, juce::String suffix, unsigned decimals, float interval, std::function<void()> callback)
{
    slider->setSliderStyle(juce::Slider::LinearVertical);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, slider->getTextBoxWidth(), slider->getTextBoxHeight());
    slider->setTextValueSuffix(suffix);
    slider->setRange(min, max, interval);
    slider->setValue(def, juce::dontSendNotification);
    slider->setNumDecimalPlacesToDisplay(decimals);
    addAndMakeVisible(slider);
    
    if (callback) {
        slider->onValueChange = callback;
    }
    
    if (label) {
        label->setJustificationType(juce::Justification::centredTop);
        label->attachToComponent(slider, false);
        addAndMakeVisible(label);
    }
}

// update grouped sliders
void MainComponent::UpdateSliderGroup(CombSliderGroup * group)
{
    group->R.slider.setValue(reverb.GetCombGainConstant(group->ID), juce::dontSendNotification);
    group->G.slider.setValue(reverb.GetCombLowPassCoeff(group->ID), juce::dontSendNotification);
    group->ZF.slider.setValue(reverb.GetCombZeroFreqGain(group->ID), juce::dontSendNotification);
}

//==============================================================================
// Buttons

void MainComponent::InitButton(juce::Button * button, int groupID, std::function<void()> callback)
{
    button->setRadioGroupId(groupID);
    addAndMakeVisible(button);
    
    if (callback) {
        button->onClick = callback;
    }
    
}

// play was clicked
void MainComponent::PlayClicked()
{
    // already playing or no file loaded
    if (playing || input == nullptr) {
        return;
    }
    
    // update sampling rate
    setup.sampleRate = input->rate();
    deviceManager.setAudioDeviceSetup(setup, true);

    if (reverbOn) {
        // reset reverb filter
        reverb.SetSamplingRate(input->rate());
        reverb.Reset();
        
        if (output) {
            delete output;
            output = nullptr;
        }
        
        ApplyReverb();
        data = output;
    }
    else {
        data = input;
    }

    // ready to play
    sample = 0;
    total_samples = data->frames();
    playing = true;
}

void MainComponent::UpdateToggleState(juce::Button* button, juce::String name)
{
    if (reinterpret_cast<juce::ToggleButton*>(button) == &reverbOnOff) {
        reverbOn = !reverbOn;
        reverbOnOff.setToggleState(reverbOn, juce::dontSendNotification);
    }
}


void MainComponent::ApplyReverb()
{
    unsigned rate = input->rate();
    unsigned channels = input->channels();
    unsigned extra = rate * tail / 1000;

    // store reverb data for channels separetly
    std::vector<Reverb> revs;
    for (unsigned j = 0; j < channels; ++j) {
        revs.push_back(reverb);
    }
    
    // output signal
    output = new AudioData(input->frames() + extra, rate, channels);
    
    unsigned size = input->frames();
    unsigned i = 0;

    for (unsigned j = 0; j < channels; ++j) {
        Reverb &rev = revs[j];
        for (; i < size; ++i) {
            output->sample(i, j) = rev(input->sample(i, j));
        }
    }
    
    // add tail
    size = output->frames();
    for (; i < size; ++i) {
        for (unsigned j = 0; j < channels; ++j) {
            output->sample(i, j) = revs[j](0);
        }
    }
    
    // normalize
    normalize(*output, -1.5);
}
