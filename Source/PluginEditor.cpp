#include "PluginEditor.h"

using namespace juce;

namespace ht
{
namespace col
{
    static const Colour cyan   (0xff2ee6ff);
    static const Colour blue   (0xff1e78ff);
    static const Colour silver (0xffc3d0e0);
    static const Colour dim    (0xff5f7692);
    static const Colour edge   (0xff2a3b55);
    static const Colour red    (0xffff3b5c);
    static const Colour amber  (0xffffb43a);
}

static Font makeFont (float h, bool bold = true)
{
    return Font (FontOptions (h, bold ? Font::bold : Font::plain));
}

static void drawScrew (Graphics& g, Point<float> c, float r)
{
    g.setGradientFill (ColourGradient (Colour (0xffd5deea), c.x - r * 0.4f, c.y - r * 0.4f,
                                       Colour (0xff1c232d), c.x + r, c.y + r, true));
    g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colour (0xff05080d));
    g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.f);
    const float a = (c.x * 13.f + c.y * 7.f) * 0.1f;
    g.drawLine (c.x - std::cos (a) * r * 0.7f, c.y - std::sin (a) * r * 0.7f,
                c.x + std::cos (a) * r * 0.7f, c.y + std::sin (a) * r * 0.7f, 1.4f);
}

static void drawPanel (Graphics& g, Rectangle<float> b, const String& title)
{
    g.setGradientFill (ColourGradient (Colour (0xff131f34), 0, b.getY(), Colour (0xff060a12), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 6.f);
    g.setColour (Colour (0xff000000));
    g.drawRoundedRectangle (b.expanded (1.f), 7.f, 2.f);
    g.setGradientFill (ColourGradient (Colour (0xff4a5d7a), b.getX(), b.getY(), Colour (0xff131c2a), b.getX(), b.getBottom(), false));
    g.drawRoundedRectangle (b, 6.f, 1.5f);
    g.setColour (Colour (0x1a2ee6ff));
    g.drawRoundedRectangle (b.reduced (3.f), 4.f, 1.f);
    drawScrew (g, { b.getX() + 10.f, b.getY() + 10.f }, 4.f);
    drawScrew (g, { b.getRight() - 10.f, b.getY() + 10.f }, 4.f);
    drawScrew (g, { b.getX() + 10.f, b.getBottom() - 10.f }, 4.f);
    drawScrew (g, { b.getRight() - 10.f, b.getBottom() - 10.f }, 4.f);
    if (title.isNotEmpty())
    {
        g.setColour (Colour (0xffd6e8ff));
        g.setFont (makeFont (13.f));
        g.drawText (title, b.withTrimmedLeft (26).withHeight (28.f), Justification::centredLeft);
        g.setColour (col::cyan.withAlpha (0.35f));
        g.fillRect (b.getX() + 24.f, b.getY() + 28.f, b.getWidth() - 48.f, 1.f);
    }
}

static void drawLED (Graphics& g, Point<float> c, float level, Colour colour)
{
    level = jlimit (0.f, 1.f, level);
    if (level > 0.02f)
    {
        g.setGradientFill (ColourGradient (colour.withAlpha (0.45f * level), c.x, c.y, colour.withAlpha (0.f), c.x + 13.f, c.y, true));
        g.fillEllipse (c.x - 13.f, c.y - 13.f, 26.f, 26.f);
    }
    g.setColour (Colour (0xff06090f));
    g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11.f, 11.f);
    g.setColour (colour.interpolatedWith (Colour (0xff0b1220), 1.f - level));
    g.fillEllipse (c.x - 4.f, c.y - 4.f, 8.f, 8.f);
    g.setColour (Colours::white.withAlpha (0.35f * level));
    g.fillEllipse (c.x - 2.5f, c.y - 3.f, 3.f, 2.f);
}

// ===================================================================== LookAndFeel
class HTLookAndFeel : public LookAndFeel_V4
{
public:
    HTLookAndFeel()
    {
        setColour (Slider::textBoxTextColourId, Colour (0xffe6f4ff));
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (ComboBox::textColourId, Colour (0xffe6f4ff));
        setColour (PopupMenu::backgroundColourId, Colour (0xff0a1322));
        setColour (PopupMenu::textColourId, Colour (0xffcfe6ff));
        setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff134a8a));
        setColour (PopupMenu::highlightedTextColourId, Colours::white);
        setColour (AlertWindow::backgroundColourId, Colour (0xff0a1322));
        setColour (AlertWindow::textColourId, Colour (0xffcfe6ff));
        setColour (TextEditor::backgroundColourId, Colour (0xff050a12));
        setColour (TextEditor::textColourId, Colour (0xffe6f4ff));
        setColour (TextEditor::outlineColourId, Colour (0xff2a3b55));
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&) override
    {
        auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float diam = jmin (bounds.getWidth(), bounds.getHeight());
        auto area = bounds.withSizeKeepingCentre (diam, diam).reduced (2.f);
        const auto c = area.getCentre();
        const float r = area.getWidth() * 0.5f;
        const float trackR = r - 9.f, bodyR = r - 17.f;
        const float ang = a0 + pos * (a1 - a0);

        for (int i = 0; i <= 10; ++i)
        {
            const float a = a0 + (a1 - a0) * (float) i / 10.f;
            const float s = std::sin (a), co = -std::cos (a);
            g.setColour ((float) i / 10.f <= pos + 0.001f ? col::cyan.withAlpha (0.9f) : col::dim.withAlpha (0.45f));
            g.drawLine (c.x + s * (r - 3.5f), c.y + co * (r - 3.5f), c.x + s * r, c.y + co * r, 1.5f);
        }

        Path track;
        track.addCentredArc (c.x, c.y, trackR, trackR, 0.f, a0, a1, true);
        g.setColour (Colour (0xff050a14));
        g.strokePath (track, PathStrokeType (6.f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colour (0xff1b2a41));
        g.strokePath (track, PathStrokeType (1.f));

        if (pos > 0.002f)
        {
            Path val;
            val.addCentredArc (c.x, c.y, trackR, trackR, 0.f, a0, ang, true);
            g.setColour (col::cyan.withAlpha (0.10f)); g.strokePath (val, PathStrokeType (13.f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (col::cyan.withAlpha (0.22f)); g.strokePath (val, PathStrokeType (9.f,  PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (col::cyan);                   g.strokePath (val, PathStrokeType (4.f,  PathStrokeType::curved, PathStrokeType::rounded));
        }

        // cuerpo metálico
        g.setColour (Colour (0xff000000).withAlpha (0.6f));
        g.fillEllipse (c.x - bodyR - 1.f, c.y - bodyR + 2.f, (bodyR + 1.f) * 2, (bodyR + 1.f) * 2);
        g.setGradientFill (ColourGradient (Colour (0xffe4ecf6), c.x - bodyR, c.y - bodyR, Colour (0xff2a323d), c.x + bodyR, c.y + bodyR, false));
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
        for (int i = 0; i < 48; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / 48.f;
            g.setColour (Colours::black.withAlpha (0.28f));
            g.drawLine (c.x + std::sin (a) * (bodyR - 3.f), c.y - std::cos (a) * (bodyR - 3.f),
                        c.x + std::sin (a) * bodyR, c.y - std::cos (a) * bodyR, 1.f);
        }
        const float fr = bodyR - 5.f;
        g.setGradientFill (ColourGradient (Colour (0xff3f4d61), c.x - fr * 0.5f, c.y - fr * 0.6f, Colour (0xff070b12), c.x + fr, c.y + fr, true));
        g.fillEllipse (c.x - fr, c.y - fr, fr * 2, fr * 2);
        g.setColour (Colour (0xff0a0f18));
        g.drawEllipse (c.x - fr, c.y - fr, fr * 2, fr * 2, 1.f);

        const float s = std::sin (ang), co = -std::cos (ang);
        g.setColour (col::cyan.withAlpha (0.3f));
        g.drawLine (c.x + s * fr * 0.30f, c.y + co * fr * 0.30f, c.x + s * fr * 0.88f, c.y + co * fr * 0.88f, 6.f);
        g.setColour (col::cyan);
        g.drawLine (c.x + s * fr * 0.30f, c.y + co * fr * 0.30f, c.x + s * fr * 0.88f, c.y + co * fr * 0.88f, 2.6f);
    }

    Label* createSliderTextBox (Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (makeFont (13.f));
        l->setJustificationType (Justification::centred);
        return l;
    }

    void drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        const auto top = on ? Colour (0xff1c6fd6) : Colour (0xff1a2638);
        const auto bot = on ? Colour (0xff0b2f66) : Colour (0xff0a111c);
        g.setGradientFill (ColourGradient (top.brighter (over ? 0.15f : 0.f), 0, r.getY(), bot, 0, r.getBottom(), false));
        if (down) g.setGradientFill (ColourGradient (bot, 0, r.getY(), top, 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.f);
        if (on) { g.setColour (col::cyan.withAlpha (0.25f)); g.drawRoundedRectangle (r.expanded (1.5f), 5.f, 2.f); }
        g.setColour (on ? col::cyan : Colour (0xff3b4f6e));
        g.drawRoundedRectangle (r, 4.f, 1.2f);
    }

    void drawButtonText (Graphics& g, TextButton& b, bool, bool) override
    {
        g.setFont (makeFont (12.f));
        g.setColour (! b.isEnabled() ? col::dim.withAlpha (0.6f) : b.getToggleState() ? Colours::white : Colour (0xffbfd4ec));
        g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
    }

    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&) override
    {
        auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.f);
        g.setGradientFill (ColourGradient (Colour (0xff060b14), 0, 0, Colour (0xff0d1626), 0, (float) h, false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (col::cyan.withAlpha (0.55f));
        g.drawRoundedRectangle (r, 4.f, 1.2f);
    }
    Font getComboBoxFont (ComboBox&) override { return makeFont (14.f); }
    void positionComboBoxText (ComboBox& box, Label& l) override
    {
        l.setBounds (4, 1, box.getWidth() - 8, box.getHeight() - 2);
        l.setFont (getComboBoxFont (box));
        l.setJustificationType (Justification::centred);
    }
};

// ===================================================================== medidor
class MeterBar : public Component
{
public:
    void setLevelDb (float db)
    {
        const float n = jlimit (0.f, 1.f, (db + 60.f) / 60.f);
        level = jmax (n, level * 0.86f);
        peak  = jmax (n, peak - 0.012f);
        repaint();
    }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (Colour (0xff04070d)); g.fillRoundedRectangle (b, 3.f);
        g.setColour (Colour (0xff25354e)); g.drawRoundedRectangle (b.reduced (0.5f), 3.f, 1.f);
        constexpr int segs = 24;
        const float sh = (b.getHeight() - 8.f) / segs;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float) i / segs;
            const bool lit = f < level || (int) (peak * segs) == i;
            const Colour base = f > 0.88f ? col::red : f > 0.7f ? col::amber : col::cyan;
            g.setColour (lit ? base : base.withAlpha (0.10f));
            g.fillRoundedRectangle (4.f, b.getBottom() - 4.f - (float) (i + 1) * sh + 1.f, b.getWidth() - 8.f, sh - 2.f, 1.5f);
        }
    }
private:
    float level = 0.f, peak = 0.f;
};

// ===================================================================== osciloscopio
class ScopeView : public Component
{
public:
    explicit ScopeView (HardwareTrapProcessor& p) : proc (p) {}
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff07182e), 0, 0, Colour (0xff030813), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 5.f);
        g.setColour (col::cyan.withAlpha (0.07f));
        for (int i = 1; i < 12; ++i) g.drawVerticalLine ((int) (b.getWidth() * i / 12.f), 0.f, b.getHeight());
        for (int i = 1; i < 6; ++i)  g.drawHorizontalLine ((int) (b.getHeight() * i / 6.f), 0.f, b.getWidth());
        g.setColour (col::cyan.withAlpha (0.2f));
        g.drawHorizontalLine ((int) (b.getHeight() * 0.5f), 0.f, b.getWidth());

        const int n = HardwareTrapProcessor::scopeSize;
        const int w0 = proc.scopeWrite.load();
        Path p;
        for (int i = 0; i < n; ++i)
        {
            const float v = jlimit (-1.f, 1.f, proc.scope[(size_t) ((w0 + i) % n)].load (std::memory_order_relaxed));
            const float x = b.getWidth() * (float) i / (float) (n - 1);
            const float y = b.getCentreY() - v * b.getHeight() * 0.45f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (col::cyan.withAlpha (0.12f)); g.strokePath (p, PathStrokeType (7.f, PathStrokeType::curved));
        g.setColour (col::cyan.withAlpha (0.28f)); g.strokePath (p, PathStrokeType (3.5f, PathStrokeType::curved));
        g.setColour (Colour (0xffd8fbff));         g.strokePath (p, PathStrokeType (1.4f, PathStrokeType::curved));

        g.setColour (col::cyan.withAlpha (0.8f));
        g.setFont (makeFont (11.f));
        g.drawText ("OUTPUT MONITOR", 12, 8, 200, 14, Justification::centredLeft);
        g.setColour (col::cyan.withAlpha (0.4f));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.f, 1.f);
    }
private:
    HardwareTrapProcessor& proc;
};

// ===================================================================== SAMPLE LAB: forma de onda
class SampleView : public Component, public FileDragAndDropTarget
{
public:
    explicit SampleView (HardwareTrapProcessor& p) : proc (p) {}
    std::function<void (const File&)> onFile;

    void resized() override { peaksDirty = true; }

    void paint (Graphics& g) override
    {
        refresh();
        auto b = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff07182e), 0, 0, Colour (0xff030813), 0, b.getBottom(), false));
        g.fillRoundedRectangle (b, 5.f);
        g.setColour (col::cyan.withAlpha (0.2f));
        g.drawHorizontalLine ((int) b.getCentreY(), 0.f, b.getWidth());

        if (data == nullptr)
        {
            g.setColour (col::dim);
            g.setFont (makeFont (15.f));
            g.drawText ("ARRASTRÁ UN WAV ACÁ  o  PRESIONÁ LOAD WAV", getLocalBounds(), Justification::centred);
        }
        else
        {
            const float rs = proc.regionStart.load(), re = proc.regionEnd.load();
            const float xs = rs * b.getWidth(), xe = re * b.getWidth();
            g.setColour (col::cyan.withAlpha (0.10f));
            g.fillRect (xs, 0.f, xe - xs, b.getHeight());

            const float half = b.getHeight() * 0.5f * 0.92f;
            for (int x = 0; x < (int) peaks.size(); ++x)
            {
                const bool inside = (float) x >= xs && (float) x <= xe;
                g.setColour (inside ? col::cyan : col::blue.withAlpha (0.45f));
                g.drawVerticalLine (x, b.getCentreY() - peaks[(size_t) x].second * half - 0.5f,
                                       b.getCentreY() - peaks[(size_t) x].first * half + 0.5f);
            }
            g.setColour (Colours::white.withAlpha (0.9f));
            g.fillRect (xs - 1.f, 0.f, 2.f, b.getHeight());
            g.fillRect (xe - 1.f, 0.f, 2.f, b.getHeight());
            g.fillRect (xs - 1.f, 0.f, 8.f, 3.f);
            g.fillRect (xe - 7.f, b.getHeight() - 3.f, 8.f, 3.f);

            if (proc.previewPlaying.load())
            {
                const float px = proc.previewPosNorm.load() * b.getWidth();
                g.setColour (col::amber);
                g.fillRect (px - 1.f, 0.f, 2.f, b.getHeight());
            }
        }
        g.setColour (col::cyan.withAlpha (0.4f));
        g.drawRoundedRectangle (b.reduced (0.5f), 5.f, 1.f);
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (data == nullptr) return;
        const float n = jlimit (0.f, 1.f, (float) e.x / (float) getWidth());
        const float px = 8.f / (float) getWidth();
        if (std::abs (n - proc.regionStart.load()) < px)      mode = 1;
        else if (std::abs (n - proc.regionEnd.load()) < px)   mode = 2;
        else { mode = 3; anchor = n; proc.regionStart = n; proc.regionEnd = jmin (1.f, n + 0.0005f); }
        repaint();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (data == nullptr) return;
        const float n = jlimit (0.f, 1.f, (float) e.x / (float) getWidth());
        const float minW = 0.0005f;
        if (mode == 1)      proc.regionStart = jmin (n, proc.regionEnd.load() - minW);
        else if (mode == 2) proc.regionEnd   = jmax (n, proc.regionStart.load() + minW);
        else if (mode == 3)
        {
            const float a = jmin (anchor, n), c = jmax (anchor, n);
            proc.regionStart = jmax (0.f, jmin (a, 1.f - minW));
            proc.regionEnd   = jmin (1.f, jmax (c, a + minW));
        }
        repaint();
    }
    void mouseDoubleClick (const MouseEvent&) override { proc.regionStart = 0.f; proc.regionEnd = 1.f; repaint(); }

    bool isInterestedInFileDrag (const StringArray& files) override
    {
        for (auto& f : files)
            if (File (f).hasFileExtension ("wav;wave;aif;aiff;flac;ogg")) return true;
        return false;
    }
    void filesDropped (const StringArray& files, int, int) override
    {
        if (files.size() > 0 && onFile) onFile (File (files[0]));
    }

    std::shared_ptr<const SampleData> data;

private:
    void refresh()
    {
        const int v = proc.sampleVersion.load();
        if (v != version) { version = v; data = proc.getSample(); peaksDirty = true; }
        if (peaksDirty && data != nullptr && getWidth() > 0)
        {
            const int w = getWidth(), len = data->audio.getNumSamples(), nch = data->audio.getNumChannels();
            peaks.assign ((size_t) w, { 0.f, 0.f });
            for (int x = 0; x < w; ++x)
            {
                const int a = (int) ((int64) len * x / w), bnd = jmax (a + 1, (int) ((int64) len * (x + 1) / w));
                const int step = jmax (1, (bnd - a) / 64);
                float mn = 0.f, mxv = 0.f;
                for (int c = 0; c < nch; ++c)
                {
                    const float* s = data->audio.getReadPointer (c);
                    for (int i = a; i < bnd && i < len; i += step) { mn = jmin (mn, s[i]); mxv = jmax (mxv, s[i]); }
                }
                peaks[(size_t) x] = { mn, mxv };
            }
            peaksDirty = false;
        }
    }

    HardwareTrapProcessor& proc;
    std::vector<std::pair<float, float>> peaks;
    bool peaksDirty = true;
    int version = -1, mode = 0;
    float anchor = 0.f;
};
} // namespace ht

// ===================================================================== Editor
using namespace ht;

HardwareTrapEditor::HardwareTrapEditor (HardwareTrapProcessor& p)
    : AudioProcessorEditor (&p), proc (p), lnf (std::make_unique<HTLookAndFeel>())
{
    setLookAndFeel (lnf.get());
    setSize (920, 620);

    const char* ids[6]  = { ParamID::destroy, ParamID::movement, ParamID::texture, ParamID::unstable, ParamID::mix, ParamID::output };
    const char* names[6] = { "DESTROY", "MOVEMENT", "TEXTURE", "UNSTABLE", "MIX", "OUTPUT" };
    const char* subs[6]  = { "DRIVE · FOLD · CLIP", "RHYTHMIC MOD", "BITCRUSH · DECIMATE", "GLITCH · DRIFT", "DRY / WET", "LEVEL" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.name = names[i]; k.sub = subs[i];
        k.slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 96, 20);
        k.slider.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
        k.slider.setMouseDragSensitivity (220);
        addAndMakeVisible (k.slider);
        k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);
        if (auto* prm = proc.apvts.getParameter (ids[i]))
            k.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    }

    for (auto* b : { &liveTab, &labTab })
    {
        b->setRadioGroupId (101);
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    liveTab.setToggleState (true, dontSendNotification);
    liveTab.onClick = [this] { setMode (false); };
    labTab.onClick  = [this] { setMode (true); };

    for (auto* b : { &prevPreset, &nextPreset, &savePreset }) addAndMakeVisible (*b);
    addAndMakeVisible (presetBox);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) { proc.loadPreset (idx); lastPresetName = proc.getCurrentPresetName(); }
    };
    prevPreset.onClick = [this]
    {
        const int n = presetBox.getNumItems();
        if (n > 0) presetBox.setSelectedId ((presetBox.getSelectedId() - 2 + n) % n + 1, sendNotification);
    };
    nextPreset.onClick = [this]
    {
        const int n = presetBox.getNumItems();
        if (n > 0) presetBox.setSelectedId (presetBox.getSelectedId() % n + 1, sendNotification);
    };
    savePreset.onClick = [this] { promptSavePreset(); };
    refreshPresets();

    meterIn  = std::make_unique<MeterBar>();
    meterOut = std::make_unique<MeterBar>();
    scopeView = std::make_unique<ScopeView> (proc);
    sampleView = std::make_unique<SampleView> (proc);
    sampleView->onFile = [this] (const File& f) { loadSampleFile (f); };
    addAndMakeVisible (*meterIn); addAndMakeVisible (*meterOut); addAndMakeVisible (*scopeView);
    addChildComponent (*sampleView);

    for (auto* b : { &loadButton, &playButton, &loopButton, &exportButton }) addChildComponent (*b);
    loopButton.setClickingTogglesState (true);
    loopButton.setToggleState (proc.previewLoop.load(), dontSendNotification);
    loopButton.onClick = [this] { proc.previewLoop = loopButton.getToggleState(); };
    loadButton.onClick = [this] { chooseSample(); };
    exportButton.onClick = [this] { chooseExport(); };
    playButton.onClick = [this]
    {
        if (proc.previewPlaying.load()) proc.stopPreview(); else proc.startPreview();
    };

    startTimerHz (30);
}

HardwareTrapEditor::~HardwareTrapEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HardwareTrapEditor::setMode (bool lab)
{
    sampleMode = lab;
    scopeView->setVisible (! lab);
    meterIn->setVisible (! lab);
    meterOut->setVisible (! lab);
    sampleView->setVisible (lab);
    for (auto* b : { &loadButton, &playButton, &loopButton, &exportButton }) b->setVisible (lab);
    if (! lab) proc.stopPreview();
    repaint();
}

void HardwareTrapEditor::refreshPresets()
{
    presetBox.clear (dontSendNotification);
    auto names = proc.getPresetNames();
    for (int i = 0; i < names.size(); ++i) presetBox.addItem (names[i], i + 1);
    lastPresetName = proc.getCurrentPresetName();
    const int idx = names.indexOf (lastPresetName);
    if (idx >= 0) presetBox.setSelectedId (idx + 1, dontSendNotification);
    else presetBox.setText (lastPresetName, dontSendNotification);
}

void HardwareTrapEditor::promptSavePreset()
{
    auto* w = new AlertWindow ("Guardar preset", "Nombre del preset:", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.getCurrentPresetName());
    w->addButton ("Guardar", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancelar", 0, KeyPress (KeyPress::escapeKey));
    Component::SafePointer<HardwareTrapEditor> safe (this);
    w->enterModalState (true, ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (safe == nullptr || r != 1) return;
        if (safe->proc.saveUserPreset (w->getTextEditorContents ("name"))) safe->refreshPresets();
        else safe->statusText = "No se pudo guardar el preset.";
    }), true);
}

void HardwareTrapEditor::loadSampleFile (const File& f)
{
    String err;
    if (proc.loadSample (f, err)) statusText = f.getFileName();
    else statusText = err;
    repaint (displayPanel());
}

void HardwareTrapEditor::chooseSample()
{
    chooser = std::make_unique<FileChooser> ("Cargar sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg");
    Component::SafePointer<HardwareTrapEditor> safe (this);
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [safe] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (safe != nullptr && f.existsAsFile()) safe->loadSampleFile (f);
                          });
}

void HardwareTrapEditor::chooseExport()
{
    auto sd = proc.getSample();
    if (sd == nullptr) { statusText = "Primero cargá un sample."; repaint (displayPanel()); return; }
    auto def = sd->file.getParentDirectory().getChildFile (sd->file.getFileNameWithoutExtension() + "_HARDWARETRAP.wav");
    chooser = std::make_unique<FileChooser> ("Exportar WAV procesado", def, "*.wav");
    Component::SafePointer<HardwareTrapEditor> safe (this);
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (safe == nullptr || f == File()) return;
                              f = f.withFileExtension ("wav");
                              String err;
                              safe->statusText = safe->proc.exportRegion (f, err) ? "Exportado: " + f.getFileName() : err;
                              safe->repaint (safe->displayPanel());
                          });
}

void HardwareTrapEditor::timerCallback()
{
    const float inDb  = Decibels::gainToDecibels (proc.inputPeak.exchange (0.f), -60.f);
    const float outDb = Decibels::gainToDecibels (proc.outputPeak.exchange (0.f), -60.f);
    meterIn->setLevelDb (inDb);
    meterOut->setLevelDb (outDb);

    ledSignal = jmax (outDb > -50.f ? 1.f : 0.f, ledSignal * 0.85f);
    const float mv = proc.apvts.getRawParameterValue (ParamID::movement)->load();
    ledMove   = mv > 0.01f ? proc.lfoDisplay.load() : 0.f;
    ledGlitch = jmax (proc.glitchFlag.exchange (0.f), ledGlitch * 0.8f);
    ledLimit  = jmax (proc.limiterFlag.exchange (0.f), ledLimit * 0.85f);

    if (sampleMode)
    {
        const bool playing = proc.previewPlaying.load();
        playButton.setButtonText (playing ? "STOP" : "PLAY");
        playButton.setEnabled (proc.getSample() != nullptr);
        exportButton.setEnabled (proc.getSample() != nullptr);
        sampleView->repaint();
        repaint (displayPanel().withTrimmedTop (232));
    }
    else
    {
        scopeView->repaint();
        repaint (displayPanel().withTrimmedTop (240));
    }

    const auto name = proc.getCurrentPresetName();
    if (name != lastPresetName) refreshPresets();
}

void HardwareTrapEditor::resized()
{
    liveTab.setBounds (648, 18, 110, 30);
    labTab.setBounds (764, 18, 110, 30);
    prevPreset.setBounds (318, 18, 28, 30);
    presetBox.setBounds (350, 18, 190, 30);
    nextPreset.setBounds (544, 18, 28, 30);
    savePreset.setBounds (580, 18, 54, 30);

    auto dp = displayPanel();
    meterIn->setBounds (dp.getX() + 18, dp.getY() + 38, 30, 196);
    meterOut->setBounds (dp.getRight() - 48, dp.getY() + 38, 30, 196);
    scopeView->setBounds (dp.getX() + 60, dp.getY() + 38, dp.getWidth() - 120, 196);
    sampleView->setBounds (dp.getX() + 16, dp.getY() + 38, dp.getWidth() - 32, 190);

    const int by = dp.getY() + 240;
    loadButton.setBounds   (dp.getX() + 16, by, 110, 28);
    playButton.setBounds   (dp.getX() + 134, by, 80, 28);
    loopButton.setBounds   (dp.getX() + 222, by, 70, 28);
    exportButton.setBounds (dp.getX() + 300, by, 120, 28);

    auto kp = knobPanel();
    const float cw = (float) kp.getWidth() / 6.f;
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i].slider.setBounds ((int) (kp.getX() + cw * (float) i + (cw - 130.f) * 0.5f), kp.getY() + 34, 130, 150);
}

void HardwareTrapEditor::drawStatic (Graphics& g)
{
    const auto full = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff0e1a2e), 0, 0, Colour (0xff03060b), 0, full.getBottom(), false));
    g.fillAll();

    Random rnd (7);
    for (int y = 0; y < getHeight(); y += 2)
    {
        g.setColour (Colours::white.withAlpha (rnd.nextFloat() * 0.025f));
        g.drawHorizontalLine (y, 0.f, (float) getWidth());
    }

    // marco industrial
    g.setGradientFill (ColourGradient (Colour (0xff6b7a90), 0, 0, Colour (0xff12181f), 0, full.getBottom(), false));
    g.drawRect (full, 5.f);
    g.setColour (Colour (0xff000000)); g.drawRect (full.reduced (5.f), 1.f);
    for (auto pt : { Point<float> (12, 12), Point<float> (full.getRight() - 12, 12),
                     Point<float> (12, full.getBottom() - 12), Point<float> (full.getRight() - 12, full.getBottom() - 12) })
        drawScrew (g, pt, 5.f);

    // cabecera
    {
        Path h;   // logo "H"
        g.setColour (col::cyan.withAlpha (0.25f)); g.fillRoundedRectangle (22.f, 14.f, 46.f, 46.f, 6.f);
        g.setGradientFill (ColourGradient (Colour (0xff1c2c46), 0, 14, Colour (0xff070c14), 0, 60, false));
        g.fillRoundedRectangle (24.f, 16.f, 42.f, 42.f, 5.f);
        g.setColour (col::cyan);
        g.fillRect (32.f, 24.f, 6.f, 26.f); g.fillRect (52.f, 24.f, 6.f, 26.f); g.fillRect (32.f, 34.f, 26.f, 6.f);
        g.setColour (Colour (0xffe6f1ff));
        g.setFont (makeFont (27.f));
        g.drawText (String::charToString ('H') + "ARDWARE", 80, 12, 170, 32, Justification::centredLeft);
        g.setColour (col::cyan);
        g.drawText ("TRAP", 218, 12, 80, 32, Justification::centredLeft);
        g.setColour (col::dim);
        g.setFont (makeFont (11.f, false));
        g.drawText ("M U T A T I O N   E N G I N E", 82, 42, 230, 14, Justification::centredLeft);
    }

    // paneles
    drawPanel (g, displayPanel().toFloat(), {});
    drawPanel (g, knobPanel().toFloat(), "MUTATION CONTROLS");

    const auto kp = knobPanel();
    const float cw = (float) kp.getWidth() / 6.f;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const float cx = (float) kp.getX() + cw * ((float) i + 0.5f);
        g.setColour (Colour (0xffe6f4ff)); g.setFont (makeFont (14.f));
        g.drawText (knobs[i].name, Rectangle<float> (cx - 70.f, (float) kp.getY() + 186.f, 140.f, 18.f), Justification::centred);
        g.setColour (col::dim); g.setFont (makeFont (9.f, false));
        g.drawText (knobs[i].sub, Rectangle<float> (cx - 70.f, (float) kp.getY() + 203.f, 140.f, 12.f), Justification::centred);
        if (i > 0)
        {
            g.setColour (col::edge.withAlpha (0.6f));
            g.drawVerticalLine ((int) ((float) kp.getX() + cw * (float) i), (float) kp.getY() + 40.f, (float) kp.getBottom() - 14.f);
        }
    }

    // pie
    g.setColour (col::dim); g.setFont (makeFont (10.f, false));
    g.drawText ("HARDWARETRAP  ·  MUTATION ENGINE  ·  VST3 64-BIT  ·  v1.0", 24, 596, 500, 16, Justification::centredLeft);
}

void HardwareTrapEditor::paint (Graphics& g)
{
    const float scale = (float) g.getInternalContext().getPhysicalPixelScaleFactor();
    if (staticLayer.isNull() || std::abs (scale - staticScale) > 0.01f)
    {
        staticScale = scale;
        staticLayer = Image (Image::ARGB, roundToInt ((float) getWidth() * scale), roundToInt ((float) getHeight() * scale), true);
        Graphics gi (staticLayer);
        gi.addTransform (AffineTransform::scale (scale));
        drawStatic (gi);
    }
    g.drawImage (staticLayer, getLocalBounds().toFloat());

    // elementos dinámicos del panel de visualización
    const auto dp = displayPanel();
    g.setColour (Colour (0xffd6e8ff)); g.setFont (makeFont (13.f));
    g.drawText (sampleMode ? "SAMPLE LAB" : "LIVE MODE", dp.getX() + 26, dp.getY(), 300, 28, Justification::centredLeft);
    g.setColour (col::cyan.withAlpha (0.35f));
    g.fillRect (dp.getX() + 24, dp.getY() + 28, dp.getWidth() - 48, 1);

    g.setFont (makeFont (10.f));
    if (! sampleMode)
    {
        g.setColour (col::dim);
        g.drawText ("IN",  meterIn->getX(), dp.getY() + 14, 30, 12, Justification::centred);
        g.drawText ("OUT", meterOut->getX(), dp.getY() + 14, 30, 12, Justification::centred);

        const float y = (float) dp.getY() + 258.f;
        struct L { const char* n; float v; Colour c; };
        const L leds[] = { { "SIGNAL", ledSignal, col::cyan }, { "MOVE", ledMove, col::blue },
                           { "GLITCH", ledGlitch, col::amber }, { "LIMIT", ledLimit, col::red } };
        float x = (float) dp.getX() + 90.f;
        for (auto& l : leds)
        {
            drawLED (g, { x, y }, l.v, l.c);
            g.setColour (Colour (0xffbfd4ec));
            g.drawText (l.n, (int) x + 12, (int) y - 8, 60, 16, Justification::centredLeft);
            x += 130.f;
        }
    }
    else
    {
        String info = statusText;
        if (auto sd = proc.getSample())
        {
            const double len = sd->audio.getNumSamples() / sd->sampleRate;
            const double a = proc.regionStart.load() * len, b = proc.regionEnd.load() * len;
            info = (statusText.isNotEmpty() ? statusText + "   |   " : String()) + "REGIÓN " + String (a, 3) + " s – " + String (b, 3)
                   + " s   (" + String (b - a, 3) + " s)";
        }
        g.setColour (Colour (0xffbfd4ec));
        g.drawText (info, dp.getX() + 436, dp.getY() + 240, dp.getWidth() - 452, 28, Justification::centredLeft);
        g.setColour (col::dim);
        g.drawText ("Arrastrá sobre la onda para elegir región · doble click = todo", dp.getX() + 436, dp.getY() + 258, dp.getWidth() - 452, 18, Justification::centredLeft);
    }
}
