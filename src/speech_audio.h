#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

struct UnitClip {
    float*  pcm;           // interleaved f32
    ma_uint64 frameCount;  // frames (not samples)
    ma_uint32 channels;
    ma_uint32 sampleRate;
};

bool LoadClipF32(UnitClip* c, const char* path, ma_uint32 channels, ma_uint32 sampleRate) {
    c->channels   = channels;
    c->sampleRate = sampleRate;

    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, channels, sampleRate);

    void* pcmFrames = 0;
    ma_uint64 frameCount = 0;

    ma_result r = ma_decode_file(path, &cfg, &frameCount, &pcmFrames);
    if (r != MA_SUCCESS) {
        return false;
    }

    c->pcm = (float*)pcmFrames;
    c->frameCount = frameCount;
    return true;
}

void FreeClip(UnitClip* c) {
    if (c->pcm) { 
        ma_free(c->pcm, 0);
    }
}

struct RenderedAudio {
    float* pcm;
    ma_uint64 frameCount;
    ma_uint32 channels;
    ma_uint32 sampleRate;
};

RenderedAudio RenderConcatenated(
    const UnitClip* clips, const float* gains, int clipCount,
    ma_uint32 channels, ma_uint32 sampleRate,
    ma_uint32 xfadeFrames)
{
    // Compute total frames accounting for overlap.
    ma_uint64 total = 0;
    for (int i = 0; i < clipCount; i++) {
        total += clips[i].frameCount;
        if (i > 0) {
            ma_uint64 overlap = (clips[i-1].frameCount < xfadeFrames) ? clips[i-1].frameCount : xfadeFrames;
            overlap = (clips[i].frameCount < overlap) ? clips[i].frameCount : overlap;
            total -= overlap;
        }
    }

    RenderedAudio out = {0};
    out.channels = channels;
    out.sampleRate = sampleRate;
    out.frameCount = total;
    out.pcm = (float*)ma_malloc((size_t)(total * channels * sizeof(float)), NULL);
    if (!out.pcm) return out;

    // Zero output.
    memset(out.pcm, 0, (size_t)(total * channels * sizeof(float)));

    ma_uint64 writeFrame = 0;

    for (int i = 0; i < clipCount; i++) {
        const UnitClip* c = &clips[i];
        const float gain = gains ? gains[i] : 1.0f;

        // Determine overlap with previous clip.
        ma_uint64 overlap = 0;
        if (i > 0) {
            overlap = xfadeFrames;
            if (clips[i-1].frameCount < overlap) overlap = clips[i-1].frameCount;
            if (c->frameCount < overlap) overlap = c->frameCount;
        }

        // Non-overlapped region start in output:
        // we back up by overlap frames to blend.
        ma_uint64 dstStart = (i == 0) ? writeFrame : (writeFrame - overlap);

        // Copy/blend.
        for (ma_uint64 f = 0; f < c->frameCount; f++) {
            float w = gain;

            // Apply fade-in for the overlapped start.
            if (overlap > 0 && f < overlap) {
                float t = (float)f / (float)(overlap - 1 ? overlap - 1 : 1);
                // fade-in weight for new clip; old clip already in buffer gets complementary fade-out
                w *= t;
            }

            ma_uint64 dstF = dstStart + f;
            if (dstF >= out.frameCount) break;

            for (ma_uint32 ch = 0; ch < channels; ch++) {
                float s = c->pcm[f * channels + ch] * w;
                out.pcm[dstF * channels + ch] += s;
            }
        }

        // Now apply fade-out to previous material already in buffer (complement).
        // We do it by scaling the overlapped region that was previously written.
        if (overlap > 0) {
            ma_uint64 fadeStart = writeFrame - overlap;
            for (ma_uint64 f = 0; f < overlap; f++) {
                float t = (float)f / (float)(overlap - 1 ? overlap - 1 : 1);
                float oldScale = 1.0f - t;
                ma_uint64 dstF = fadeStart + f;
                for (ma_uint32 ch = 0; ch < channels; ch++) {
                    // Scale only the portion that existed before this clip.
                    // This is a simple approach; in practice you might separate buffers.
                    out.pcm[dstF * channels + ch] *= oldScale;
                }
            }
        }

        // Advance writeFrame (minus overlap already accounted for).
        writeFrame = dstStart + c->frameCount;
    }

    return out;
}

void FreeRendered(RenderedAudio* a)
{
    if (a->pcm) ma_free(a->pcm, 0);
}

typedef struct Playback {
    ma_audio_buffer buf;
    ma_sound sound;
    RenderedAudio audio; // owns pcm; must outlive playback
} Playback;

void PlayRendered(ma_engine* engine, RenderedAudio* a) {
    Playback* p = (Playback*)ma_malloc(sizeof(Playback), NULL);
    memset(p, 0, sizeof(*p));
    p->audio = *a;

    ma_audio_buffer_config cfg = ma_audio_buffer_config_init(ma_format_f32, p->audio.channels, p->audio.frameCount, p->audio.pcm, NULL);
    if (ma_audio_buffer_init(&cfg, &p->buf) != MA_SUCCESS) {
        FreeRendered(&p->audio);
        ma_free(p, NULL);
        return;
    }

    if (ma_sound_init_from_data_source(engine, &p->buf, 0, NULL, &p->sound) != MA_SUCCESS) {
        ma_audio_buffer_uninit(&p->buf);
        FreeRendered(&p->audio);
        ma_free(p, NULL);
        return;
    }
    
    ma_sound_set_pitch(&p->sound, 1.25f); 
    ma_sound_start(&p->sound);
}