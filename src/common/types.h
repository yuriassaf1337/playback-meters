#pragma once

// types and constants

#include <cstddef>
#include <cstdint>

namespace pm
{

	// audio sample type (32-bit float for WASAPI compatibility)
	using sample_t = float;

	// stereo sample pair
	struct stereo_sample {
		sample_t left;
		sample_t right;
	};

	// default audio settings
	constexpr int k_default_sample_rate = 48000;
	constexpr int k_default_buffer_size = 1024;
	constexpr int k_default_channels    = 2;

	// FFT sizes
	constexpr size_t k_fft_size_1024  = 1024;
	constexpr size_t k_fft_size_2048  = 2048;
	constexpr size_t k_fft_size_4096  = 4096;
	constexpr size_t k_fft_size_8192  = 8192;
	constexpr size_t k_fft_size_16384 = 16384;

	// loudness window sizes (in milliseconds)
	constexpr int k_lufs_momentary_ms  = 400;
	constexpr int k_lufs_short_term_ms = 3000;
	constexpr int k_rms_fast_ms        = 300;
	constexpr int k_rms_slow_ms        = 1000;

	// clipping buffer sizes (in seconds)
	constexpr int k_clip_buffer_10s = 10;
	constexpr int k_clip_buffer_60s = 60;

	// hearing range
	static constexpr float k_min_freq = 20.0f;
	static constexpr float k_max_freq = 20000.0f;

	// device types for audio engine
	enum class device_type {
		input,
		output
	};

} // namespace pm
