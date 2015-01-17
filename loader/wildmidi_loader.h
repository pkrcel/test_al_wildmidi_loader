/*
* WildMIDI Audio Loader with Allegro 5 Sound backend.
*
* Requires Wildimidi    <http://www.mindwerks.net/projects/wildmidi/>
*                       <https://github.com/Mindwerks/wildmidi>
*          Allegro5     <http://sourceforge.net/p/alleg/allegro/ci/5.1/tree/>
*
* author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
*
* Revisions:
*           2015-01-09 Initial Release.
*
*/

#ifndef WILDMIDI_LOADER_H
#define WILDMIDI_LOADER_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include <wildmidi_lib.h>


/* types */
typedef struct WM_LIB_STATUS WM_LIB_STATUS;
typedef struct WM_FILE_DATA WM_FILE_DATA;

/* public functions
 *
 * This is the interface for the Wildmidi Loader
 * The loader (alas wildmidi itself) needs a Wildmidi config file
 * in which it's described where to find the GUS patches to be
 * used in synthetizing the sounds.
 */

bool WM_setopts(const char* cfg_file, uint16_t samplerate, uint16_t mixeropts);

ALLEGRO_SAMPLE *load_sample_wildmidi(const char *filename);
ALLEGRO_SAMPLE *load_sample_wildmidi_f(ALLEGRO_FILE *fp);

ALLEGRO_AUDIO_STREAM *wildmidi_load_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *wildmidi_load_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);


#endif // WILDMIDI_LOADER_H

