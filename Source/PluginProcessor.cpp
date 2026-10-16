#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ht;

static juce::String percentText (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; }

juce::AudioProcessorValueTreeState::ParameterLayout HardwareTrapProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using PID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto pct = [&] (const char* id, const char* name, float def)
    {
        auto attrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction (percentText);
        l.add (std::make_unique<P> (PID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 1.f, 0.001f), def, attrs));
    };
    pct (ParamID::destroy,  "Destroy",  0.f);
    pct (ParamID::movement, "Movement", 0.f);
    pct (ParamID::texture,  "Texture",  0.f);
    pct (ParamID::unstable, "Unstable", 0.f);
    pct (ParamID::mix,      "Mix",      1.f);

    auto dbAttrs = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (v, 1) + " dB"; });
    l.add (std::make_unique<P> (PID { ParamID::output, 1 }, "Output",
                                juce::NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f, dbAttrs));
    return l;
}

HardwareTrapProcessor::HardwareTrapProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HARDWARETRAP", createLayout())
{
    pDestroy  = apvts.getRawParameterValue (ParamID::destroy);
    pMovement = apvts.getRawParameterValue (ParamID::movement);
    pTexture  = apvts.getRawParameterValue (ParamID::texture);
    pUnstable = apvts.getRawParameterValue (ParamID::unstable);
    pMix      = apvts.getRawParameterValue (ParamID::mix);
    pOutput   = apvts.getRawParameterValue (ParamID::output);
    for (auto& s : scope) s.store (0.f);
    formatManager.registerBasicFormats();
    apvts.state.setProperty ("presetName", "Init", nullptr);
}

bool HardwareTrapProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && layouts.getMainInputChannelSet() == out;
}

ht::Params HardwareTrapProcessor::currentParams() const
{
    return { pDestroy->load(), pMovement->load(), pTexture->load(), pUnstable->load(), pMix->load(), pOutput->load() };
}

void HardwareTrapProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr;
    engine.prepare (sr);
    engine.setTargets (currentParams(), true);
    scopeCounter = 0; scopeAccum = 0.f;
    restartPreview = true;
}

// ---------------------------------------------------------------- audio
void HardwareTrapProcessor::renderPreview (juce::AudioBuffer<float>& buffer, int n, int nch)
{
    juce::SpinLock::ScopedTryLockType lock (sampleLock);
    if (! lock.isLocked()) { buffer.clear(); return; }

    const SampleData* sd = sample.get();
    if (sd == nullptr || sd->audio.getNumSamples() < 4) { buffer.clear(); previewPlaying = false; return; }

    const int len = sd->audio.getNumSamples();
    double s0 = (double) regionStart.load() * len;
    double e0 = (double) regionEnd.load() * len;
    if (e0 - s0 < 4.0) { s0 = juce::jmax (0.0, e0 - 4.0); e0 = juce::jmin ((double) len, s0 + 4.0); }

    if (restartPreview.exchange (false) || playPos < s0 || playPos >= e0) playPos = s0;

    const double inc = sd->sampleRate / currentSampleRate;
    const float* L = sd->audio.getReadPointer (0);
    const float* R = sd->audio.getNumChannels() > 1 ? sd->audio.getReadPointer (1) : L;
    float* o0 = buffer.getWritePointer (0);
    float* o1 = nch > 1 ? buffer.getWritePointer (1) : nullptr;
    const bool loop = previewLoop.load();

    for (int i = 0; i < n; ++i)
    {
        if (playPos >= e0 - 1.0)
        {
            if (loop) playPos = s0;
            else
            {
                for (int k = i; k < n; ++k) { o0[k] = 0.f; if (o1) o1[k] = 0.f; }
                previewPlaying = false; playPos = s0;
                break;
            }
        }
        const int idx = (int) playPos;
        const float fr = (float) (playPos - idx);
        const float a = L[idx] + (L[idx + 1] - L[idx]) * fr;
        const float b = R[idx] + (R[idx + 1] - R[idx]) * fr;
        if (o1) { o0[i] = a; o1[i] = b; } else o0[i] = 0.5f * (a + b);
        playPos += inc;
    }
    previewPosNorm = (float) (playPos / len);
}

void HardwareTrapProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    for (int c = nch; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);

    if (previewPlaying.load()) renderPreview (buffer, n, nch);

    float inPk = 0.f;
    for (int c = 0; c < nch; ++c) inPk = juce::jmax (inPk, buffer.getMagnitude (c, 0, n));
    inputPeak = juce::jmax (inputPeak.load(), inPk);

    engine.setTargets (currentParams(), false);
    engine.process (buffer.getArrayOfWritePointers(), nch, n);

    float outPk = 0.f;
    for (int c = 0; c < nch; ++c) outPk = juce::jmax (outPk, buffer.getMagnitude (c, 0, n));
    outputPeak = juce::jmax (outputPeak.load(), outPk);
    lfoDisplay = engine.getLfo();
    if (engine.wasGlitching()) glitchFlag = 1.f;
    if (engine.wasLimiting())  limiterFlag = 1.f;

    // osciloscopio (≈ 45 ms de ventana)
    const int decim = juce::jmax (1, (int) (currentSampleRate * 0.045 / scopeSize));
    const float* d0 = buffer.getReadPointer (0);
    const float* d1 = nch > 1 ? buffer.getReadPointer (1) : d0;
    int w = scopeWrite.load();
    for (int i = 0; i < n; ++i)
    {
        scopeAccum += 0.5f * (d0[i] + d1[i]);
        if (++scopeCounter >= decim)
        {
            scope[(size_t) w].store (scopeAccum / (float) decim, std::memory_order_relaxed);
            w = (w + 1) % scopeSize;
            scopeCounter = 0; scopeAccum = 0.f;
        }
    }
    scopeWrite.store (w);
}

// ---------------------------------------------------------------- sample lab
std::shared_ptr<const SampleData> HardwareTrapProcessor::getSample() const
{
    const juce::SpinLock::ScopedLockType l (sampleLock);
    return sample;
}

bool HardwareTrapProcessor::loadSample (const juce::File& file, juce::String& error)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr) { error = "No se pudo leer el archivo (usá WAV/AIFF/FLAC)."; return false; }
    if (reader->lengthInSamples < 16) { error = "El archivo está vacío o es demasiado corto."; return false; }
    if (reader->lengthInSamples > (juce::int64) (reader->sampleRate * 600.0)) { error = "Máximo 10 minutos por sample."; return false; }

    auto sd = std::make_shared<SampleData>();
    const int ch = reader->numChannels > 1 ? 2 : 1;
    sd->audio.setSize (ch, (int) reader->lengthInSamples);
    if (! reader->read (&sd->audio, 0, (int) reader->lengthInSamples, 0, true, ch > 1)) { error = "Error leyendo el audio."; return false; }
    sd->sampleRate = reader->sampleRate;
    sd->file = file;

    stopPreview();
    {
        const juce::SpinLock::ScopedLockType l (sampleLock);
        sample.swap (sd);          // el sample viejo se libera fuera del lock, en el hilo de UI
    }
    regionStart = 0.f; regionEnd = 1.f; previewPosNorm = 0.f;
    ++sampleVersion;
    return true;
}

void HardwareTrapProcessor::startPreview()
{
    if (getSample() == nullptr) return;
    restartPreview = true;
    previewPlaying = true;
}

bool HardwareTrapProcessor::exportRegion (const juce::File& dest, juce::String& error)
{
    auto sd = getSample();
    if (sd == nullptr) { error = "No hay ningún sample cargado."; return false; }

    const int len = sd->audio.getNumSamples();
    const int s0 = juce::jlimit (0, len - 1, (int) ((double) regionStart.load() * len));
    const int e0 = juce::jlimit (s0 + 1, len, (int) ((double) regionEnd.load() * len));
    const int ch = sd->audio.getNumChannels();

    Engine eng;
    eng.prepare (sd->sampleRate);
    eng.setTargets (currentParams(), true);

    dest.deleteFile();
    std::unique_ptr<juce::FileOutputStream> fos (dest.createOutputStream());
    if (fos == nullptr || fos->failedToOpen()) { error = "No se pudo escribir en esa ubicación."; return false; }

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (fos.get(), sd->sampleRate, (unsigned int) ch, 24, {}, 0));
    if (writer == nullptr) { error = "No se pudo crear el escritor WAV."; return false; }
    fos.release();   // el writer es dueño del stream

    constexpr int block = 1024;
    juce::AudioBuffer<float> tmp (ch, block);
    for (int pos = s0; pos < e0; pos += block)
    {
        const int cnt = juce::jmin (block, e0 - pos);
        for (int c = 0; c < ch; ++c) tmp.copyFrom (c, 0, sd->audio, c, pos, cnt);
        eng.process (tmp.getArrayOfWritePointers(), ch, cnt);
        if (! writer->writeFromAudioSampleBuffer (tmp, 0, cnt)) { error = "Error al escribir el WAV."; return false; }
    }
    return true;
}

// ---------------------------------------------------------------- presets
const std::array<FactoryPreset, 8>& HardwareTrapProcessor::factoryPresets()
{
    static const std::array<FactoryPreset, 8> p {{
        { "Init",            0.00f, 0.00f, 0.00f, 0.00f, 1.00f,  0.f },
        { "Trap Crunch",     0.45f, 0.15f, 0.18f, 0.05f, 1.00f, -2.f },
        { "Rage Fold",       0.85f, 0.30f, 0.35f, 0.15f, 0.90f, -4.f },
        { "Jerk Stutter",    0.35f, 0.40f, 0.25f, 0.70f, 1.00f, -2.f },
        { "Crank Bitcrush",  0.30f, 0.20f, 0.80f, 0.20f, 1.00f, -3.f },
        { "Ambient Dust",    0.15f, 0.65f, 0.55f, 0.35f, 0.60f,  0.f },
        { "Broken Tape",     0.25f, 0.55f, 0.45f, 0.80f, 0.85f, -2.f },
        { "Total Mutation",  1.00f, 0.80f, 0.90f, 0.90f, 1.00f, -6.f } }};
    return p;
}

juce::File HardwareTrapProcessor::presetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("HARDWARETRAP").getChildFile ("Presets");
}

juce::Array<juce::File> HardwareTrapProcessor::userPresetFiles() const
{
    auto files = presetFolder().findChildFiles (juce::File::findFiles, false, "*.htpreset");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
    return files;
}

juce::StringArray HardwareTrapProcessor::getPresetNames() const
{
    juce::StringArray names;
    for (auto& f : factoryPresets()) names.add (f.name);
    for (auto& f : userPresetFiles()) names.add (f.getFileNameWithoutExtension());
    return names;
}

void HardwareTrapProcessor::applyParams (float d, float m, float t, float u, float mx, float outDb)
{
    auto set = [this] (const char* id, float v)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    set (ParamID::destroy, d);  set (ParamID::movement, m); set (ParamID::texture, t);
    set (ParamID::unstable, u); set (ParamID::mix, mx);     set (ParamID::output, outDb);
}

void HardwareTrapProcessor::loadPreset (int index)
{
    const int nf = (int) factoryPresets().size();
    if (index >= 0 && index < nf)
    {
        auto& p = factoryPresets()[(size_t) index];
        applyParams (p.destroy, p.movement, p.texture, p.unstable, p.mix, p.outputDb);
        apvts.state.setProperty ("presetName", p.name, nullptr);
        return;
    }
    auto files = userPresetFiles();
    const int ui = index - nf;
    if (ui < 0 || ui >= files.size()) return;

    if (auto xml = juce::XmlDocument::parse (files[ui]))
    {
        auto g = [&] (const char* id, float def) { return (float) xml->getDoubleAttribute (id, def); };
        applyParams (g (ParamID::destroy, 0.f), g (ParamID::movement, 0.f), g (ParamID::texture, 0.f),
                     g (ParamID::unstable, 0.f), g (ParamID::mix, 1.f), g (ParamID::output, 0.f));
        apvts.state.setProperty ("presetName", files[ui].getFileNameWithoutExtension(), nullptr);
    }
}

bool HardwareTrapProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty()) return false;
    auto folder = presetFolder();
    if (! folder.createDirectory().wasOk()) return false;

    juce::XmlElement xml ("HTPRESET");
    for (auto* id : { ParamID::destroy, ParamID::movement, ParamID::texture, ParamID::unstable, ParamID::mix, ParamID::output })
        xml.setAttribute (id, (double) apvts.getRawParameterValue (id)->load());

    if (! xml.writeTo (folder.getChildFile (name + ".htpreset"))) return false;
    apvts.state.setProperty ("presetName", name, nullptr);
    return true;
}

juce::String HardwareTrapProcessor::getCurrentPresetName() const
{
    return apvts.state.getProperty ("presetName", "Init").toString();
}

// ---------------------------------------------------------------- estado
void HardwareTrapProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto sd = getSample())
        state.setProperty ("samplePath", sd->file.getFullPathName(), nullptr);
    state.setProperty ("regionStart", (double) regionStart.load(), nullptr);
    state.setProperty ("regionEnd",   (double) regionEnd.load(),   nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void HardwareTrapProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto state = juce::ValueTree::fromXml (*xml);
    apvts.replaceState (state);

    const juce::String path = state.getProperty ("samplePath", "").toString();
    if (path.isNotEmpty())
    {
        juce::File f (path);
        auto cur = getSample();
        if (f.existsAsFile() && (cur == nullptr || cur->file != f))
        {
            juce::String err;
            loadSample (f, err);
        }
    }
    regionStart = juce::jlimit (0.f, 1.f, (float) (double) state.getProperty ("regionStart", 0.0));
    regionEnd   = juce::jlimit (0.f, 1.f, (float) (double) state.getProperty ("regionEnd", 1.0));
    if (regionEnd.load() <= regionStart.load()) { regionStart = 0.f; regionEnd = 1.f; }
}

juce::AudioProcessorEditor* HardwareTrapProcessor::createEditor() { return new HardwareTrapEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HardwareTrapProcessor(); }
