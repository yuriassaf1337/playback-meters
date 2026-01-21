#pragma once

#include <cmath>
#include <string>

namespace pm
{

	inline float freq_to_midi( float freq )
	{
		if ( freq <= 0.0f )
			return 0.0f;
		return 69.0f + 12.0f * std::log2( freq / 440.0f );
	}

	inline float midi_to_freq( float midi )
	{
		return 440.0f * std::pow( 2.0f, ( midi - 69.0f ) / 12.0f );
	}

	// get note name from midi
	inline const char* midi_to_note_name( int midi )
	{
		static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
		int note                   = midi % 12;
		if ( note < 0 )
			note += 12;
		return names[ note ];
	}

	// get octave from midi
	inline int midi_to_octave( int midi )
	{
		return ( midi / 12 ) - 1;
	}

	// get cents deviation from nearest note
	inline int freq_to_cents( float freq )
	{
		float midi  = freq_to_midi( freq );
		int nearest = static_cast< int >( std::round( midi ) );
		return static_cast< int >( std::round( ( midi - nearest ) * 100.0f ) );
	}

	// format frequency as note string: e.g "A4 + 12 Cents"
	inline std::string freq_to_note_string( float freq )
	{
		if ( freq < 20.0f )
			return "";

		float midi  = freq_to_midi( freq );
		int nearest = static_cast< int >( std::round( midi ) );
		int cents   = static_cast< int >( std::round( ( midi - nearest ) * 100.0f ) );

		char buf[ 32 ];
		if ( cents == 0 ) {
			snprintf( buf, sizeof( buf ), "%s%d", midi_to_note_name( nearest ), midi_to_octave( nearest ) );
		} else if ( cents > 0 ) {
			snprintf( buf, sizeof( buf ), "%s%d + %d Cents", midi_to_note_name( nearest ), midi_to_octave( nearest ), cents );
		} else {
			snprintf( buf, sizeof( buf ), "%s%d - %d Cents", midi_to_note_name( nearest ), midi_to_octave( nearest ), -cents );
		}
		return buf;
	}

} // namespace pm
