#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>
#include <vector>

namespace ht
{
struct Params
{
    float destroy  = 0.f;   // 0..1
    float movement = 0.f;   // 0..1
    float texture  = 0.f;   // 0..1
    float unstable = 0.f;   // 0..1
    float mix      = 1.f;   // 0..1
    float outputDb = 0.f;   // dB
};

/** Motor de mutación: glitch/stutter -> destroy -> bitcrush -> DC block -> mix -> output -> limitador.
    Sin asignaciones de memoria en process(). Mono o estéreo. */
class Engine
{
public:
    static constexpr int maxChannels = 2;

    void prepare (double sr)
    {
        sampleRate = sr;
        ringSize = (int) sr + 16;
        for (auto& r : ring) r.assign ((size_t) ringSize, 0.f);

        for (auto* s : { &sDestroy, &sMovement, &sTexture, &sUnstable, &sMix, &sOut })
            s->reset (sr, 0.025);

        reset();
    }

    void reset()
    {
        for (auto& r : ring) std::fill (r.begin(), r.end(), 0.f);
        writePos = 0;
        glitch = {};
        lfoPhase = 0.f;
        drift = driftTarget = 0.f;
        driftCount = 0;
        for (int c = 0; c < maxChannels; ++c)
        {
            heldSample[c] = 0.f;
            holdPhase[c]  = 1.0e6f;
            dcX[c] = dcY[c] = 0.f;
        }
    }

    void setTargets (const Params& p, bool snap)
    {
        const float outGain = juce::Decibels::decibelsToGain (p.outputDb);
        if (snap)
        {
            sDestroy.setCurrentAndTargetValue (p.destroy);   sMovement.setCurrentAndTargetValue (p.movement);
            sTexture.setCurrentAndTargetValue (p.texture);   sUnstable.setCurrentAndTargetValue (p.unstable);
            sMix.setCurrentAndTargetValue (p.mix);           sOut.setCurrentAndTargetValue (outGain);
        }
        else
        {
            sDestroy.setTargetValue (p.destroy);   sMovement.setTargetValue (p.movement);
            sTexture.setTargetValue (p.texture);   sUnstable.setTargetValue (p.unstable);
            sMix.setTargetValue (p.mix);           sOut.setTargetValue (outGain);
        }
    }

    // Indicadores para la interfaz (se leen tras cada bloque)
    float getLfo() const noexcept          { return lfoValue; }
    bool  wasGlitching() const noexcept    { return glitchSeen; }
    bool  wasLimiting() const noexcept     { return limiterSeen; }

    void process (float* const* data, int numChannels, int numSamples)
    {
        numChannels = juce::jmin (numChannels, maxChannels);
        const float sr = (float) sampleRate;
        const float driftCoef = 1.f - std::exp (-1.f / (0.03f * sr));
        const float dcR = 1.f - juce::MathConstants<float>::twoPi * 5.f / sr;   // bloqueo de DC ~5 Hz
        glitchSeen = limiterSeen = false;

        for (int i = 0; i < numSamples; ++i)
        {
            const float d = sDestroy.getNextValue();
            const float m = sMovement.getNextValue();
            const float t = sTexture.getNextValue();
            const float u = sUnstable.getNextValue();
            const float mx = sMix.getNextValue();
            const float og = sOut.getNextValue();

            // --- LFO de movimiento (0.4 Hz .. ~14 Hz) ---
            lfoPhase += (0.4f + 14.f * m * m) / sr;
            if (lfoPhase >= 1.f) lfoPhase -= 1.f;
            const float lfo  = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * lfoPhase);
            const float lfoI = 1.f - lfo;
            lfoValue = lfo;

            // --- deriva aleatoria suave (unstable) ---
            if (--driftCount <= 0)
            {
                driftCount = (int) (sr * 0.012f) + 1;
                driftTarget = rng.nextFloat() * 2.f - 1.f;
            }
            drift += (driftTarget - drift) * driftCoef;

            updateGlitch (u, sr);
            if (glitch.active) glitchSeen = true;

            const float driveEff = juce::jlimit (0.f, 1.f, d * (1.f - m * 0.85f * lfoI) * (1.f + 0.5f * u * drift));
            const float texEff   = juce::jlimit (0.f, 1.f, t * (1.f - m * 0.5f * lfo) + u * 0.25f * std::abs (drift) * (t > 0.f ? 1.f : 0.f));
            const float wetMod   = 1.f - m * 0.55f * lfoI;

            const float gain  = 1.f + driveEff * driveEff * 60.f;
            const float bias  = 0.25f * driveEff;
            const float tanhB = std::tanh (bias);
            const float clipB = juce::jlimit (-1.f, 1.f, bias);
            const float foldW = juce::jmax (0.f, (driveEff - 0.6f) / 0.4f) * 0.5f;

            const float qLevels = std::exp2 (15.f - 13.f * texEff);          // 16 -> 3 bits
            const float qInv    = 1.f / qLevels;
            const float hold    = 1.f + texEff * texEff * 30.f;               // decimación 1..31x

            float win = 1.f, gGain = 0.f;
            if (glitch.active)
            {
                const float f = (float) juce::jmax (1, juce::jmin (48, glitch.loopLen / 4));
                win = juce::jmin (1.f, juce::jmin ((float) glitch.pos + 1.f, (float) (glitch.loopLen - glitch.pos)) / f);
                const float fin  = juce::jmin (1.f, (float) (glitch.total - glitch.remaining) / 96.f);
                const float fout = juce::jmin (1.f, (float) glitch.remaining / 96.f);
                gGain = juce::jmin (fin, fout);
            }

            for (int c = 0; c < numChannels; ++c)
            {
                const float dry = data[c][i];
                auto& rb = ring[c];
                rb[(size_t) writePos] = dry;

                float x = dry;
                if (glitch.active)
                {
                    float g = 0.f;
                    if (glitch.type != 3)
                    {
                        const int p = glitch.type == 2 ? (glitch.loopLen - 1 - glitch.pos) : glitch.pos;
                        int idx = glitch.loopStart + p;
                        if (idx >= ringSize) idx -= ringSize;
                        g = rb[(size_t) idx] * win;
                    }
                    x = dry + (g - dry) * gGain;
                }

                // DESTROY: saturación asimétrica + hard clip + wavefolder
                const float pre  = x * gain + bias;
                const float sat  = std::tanh (pre) - tanhB;
                const float hard = juce::jlimit (-1.f, 1.f, pre) - clipB;
                float y = sat + (hard - sat) * 0.5f * driveEff;
                y += (std::sin (pre * 1.2f) - y) * foldW;
                y = x + (y - x) * juce::jmin (1.f, driveEff * 10.f);   // DESTROY=0 => transparente
                y *= 1.f - 0.1f * driveEff;

                // TEXTURE: bitcrush + sample-rate reduction
                holdPhase[c] += 1.f;
                if (holdPhase[c] >= hold)
                {
                    holdPhase[c] -= hold;
                    heldSample[c] = std::round (y * qLevels) * qInv;
                }
                y = heldSample[c];

                // bloqueo de DC
                const float dc = y - dcX[c] + dcR * dcY[c];
                dcX[c] = y; dcY[c] = dc;

                const float wet = dc * wetMod;
                float out = (dry * (1.f - mx) + wet * mx) * og;

                // limitador suave de seguridad
                const float a = std::abs (out);
                if (a > 0.95f)
                {
                    out = std::copysign (0.95f + 0.05f * std::tanh ((a - 0.95f) / 0.05f), out);
                    limiterSeen = true;
                }
                data[c][i] = out;
            }

            if (glitch.active)
            {
                if (++glitch.pos >= glitch.loopLen) glitch.pos = 0;
                if (--glitch.remaining <= 0) glitch.active = false;
            }
            if (++writePos >= ringSize) writePos = 0;
        }
    }

private:
    struct Glitch
    {
        bool active = false;
        int type = 0, loopLen = 1, loopStart = 0, pos = 0, remaining = 0, total = 0;
    };

    void updateGlitch (float u, float sr)
    {
        if (glitch.active || u < 0.002f) return;
        if (rng.nextFloat() >= (u * u * 9.f) / sr) return;

        glitch.type = rng.nextInt (4);
        switch (glitch.type)
        {
            case 1:  glitch.loopLen = (int) (sr * (0.002f + 0.006f * rng.nextFloat())); break;   // grano corto
            case 3:  glitch.loopLen = 1; break;                                                   // gate
            default: glitch.loopLen = (int) (sr * (0.015f + 0.15f * rng.nextFloat())); break;     // stutter / reverse
        }
        glitch.loopLen = juce::jmax (1, glitch.loopLen);
        glitch.total = glitch.type == 3 ? (int) (sr * (0.02f + 0.06f * rng.nextFloat()))
                                        : glitch.loopLen * (2 + rng.nextInt (5));
        glitch.total = juce::jlimit (glitch.loopLen, (int) (sr * 0.5f), glitch.total);
        glitch.remaining = glitch.total;
        glitch.pos = 0;
        glitch.loopStart = writePos - glitch.loopLen;
        if (glitch.loopStart < 0) glitch.loopStart += ringSize;
        glitch.active = true;
    }

    double sampleRate = 44100.0;
    int ringSize = 44116, writePos = 0;
    std::array<std::vector<float>, maxChannels> ring;
    Glitch glitch;
    juce::Random rng { 0x4854 };

    juce::SmoothedValue<float> sDestroy, sMovement, sTexture, sUnstable, sMix, sOut;

    float lfoPhase = 0.f, lfoValue = 0.f;
    float drift = 0.f, driftTarget = 0.f;
    int driftCount = 0;
    std::array<float, maxChannels> heldSample {}, holdPhase {}, dcX {}, dcY {};
    bool glitchSeen = false, limiterSeen = false;
};
} // namespace ht
