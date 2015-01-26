/*
* WildMIDI Audio Loader with Allegro 5 Sound backend.
*
* Implements both ALLEGRO_SAMPLE amd ALLEGRO_AUDIO_STREAM loader;
* Streams implement also the same auto-feed mechanism the library
* Allegro5 itself implements.
* This is done througa CUSTOM (to date) implementation for:
*    - al_set_audio_stream_userdata
*    - al_get_audio_stream_userdata
*    - al_get_audio_stream_mutex
* which can be found in the 'audio-stream-userdata-access' branch
* in the following git repository:
*
*   - https://github.com/pkrcel/allegro5/tree/audio-stream-userdata-access
*
* Requires Wildimidi    <http://www.mindwerks.net/projects/wildmidi/>
*                       <https://github.com/Mindwerks/wildmidi>
*          Allegro5     <http://liballeg.org>
*                       <http://sourceforge.net/p/alleg/allegro/ci/5.1/tree/>
*
* author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
*
* Revisions:
*           2015-01-09 Initial Release.
*           2015-01-18 Working first release.
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

/* public interface to the loader
 *
 * This is the interface for the Wildmidi Loader
 * The loader (alas wildmidi itself) needs a Wildmidi config file
 * in which it's described where to find the GUS patches to be
 * used in synthetizing the sounds.
 * Unfortunately WildMidi lib has a global state which is valid across calls.
 * We have to manage it Setting WM options, Initializing Wildmidi and Shutting
 * it down globally.
 * For this we provide an interface that initializes the library and registers
 * the loaders into Allegro Sound Interface.
 */
bool WM_loader_init(bool al_register);
bool WM_register_loaders(void);
bool WM_loader_setopts(const char* cfg_file, uint16_t samplerate,
                       uint16_t mixeropts);
bool WM_unregister_loaders(void);
bool WM_loader_shutdown(void);

/*
 * Loader for Samples
 */
ALLEGRO_SAMPLE *load_sample_wildmidi(const char *filename);
ALLEGRO_SAMPLE *load_sample_wildmidi_f(ALLEGRO_FILE *fp);

/*
 * Loader for Audio streams
 * Current implementation copies to a memory buffer the whole file contents
 * and uses WildMidi_OpenBuffer() to get a handle of the MIDI file.
 *
 */
ALLEGRO_AUDIO_STREAM *load_wildmidi_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples);
ALLEGRO_AUDIO_STREAM *load_wildmidi_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples);


#endif // WILDMIDI_LOADER_H

