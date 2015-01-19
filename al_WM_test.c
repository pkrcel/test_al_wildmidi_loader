/*
* WildMIDI Audio Loader with Allegro 5 Sound backend TEST PROGRAM
*
* Plays the given midi or xmidi sample with Allegro sound backend
*
* usage:
*
*    al_WM_test <midifile> [implementation]
*
* where [implementation] can be:
*
*           sample     to use the ALLEGRO_SAMPLE implementation <default>
*           stream     to use the ALLEGRO_AUDIO_STREAM implementation
*
* Requires Wildimidi    <http://www.mindwerks.net/projects/wildmidi/>
*                       <https://github.com/Mindwerks/wildmidi>
*          Allegro5     <http://liballeg.org>
*                       <http://sourceforge.net/p/alleg/allegro/ci/5.1/tree/>
*
* author: pkrcel (aka Andrea Provasi) <pkrcel@gmail.com>
*
* Revisions:
*           2015-01-18 Initial Release.
*
*/

#include <string.h>
#include <stdio.h>

#include <wildmidi_lib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "wildmidi_loader.h"


char *getFileName(char *_path){

  /*
   * sanitize application name - should work in both *NIX and Win32/64,
   * including MSYS2 command line and QtCreator's debugger stub.
   */

  char *first = strrchr(_path, '\\');
  if (!first) first = strrchr(_path, '/');
  ++first;
  char *last = strrchr(_path, '.');
  size_t size = last - first + 1;
  char *ret = malloc(size);
  strncpy(ret, first, size);
  ret[size - 1] = '\0';
  free(_path);
  _path = ret;
  return _path;
}

int main(int argc, char *argv[])
{
   ALLEGRO_SAMPLE *sample = NULL;
   ALLEGRO_AUDIO_STREAM *stream = NULL;
   ALLEGRO_SAMPLE_INSTANCE *splinst = NULL;
   const uint32_t samplerate = 44100;
   int ret;
   bool usestream = false;

   if (argc < 2) {
      printf("Usage: %s <filename>\n [implementation]", getFileName(argv[0]));
      printf("where [implementation] can be:\n");
      printf("           sample     to use the ALLEGRO_SAMPLE implementation <default>\n");
      printf("           stream     to use the ALLEGRO_AUDIO_STREAM implementation\n");
      return -0xFFFFFFF;
   }
   if (argc>2 && !strcmp(argv[2], "stream"))
      usestream = true;
   /*
    * Initialize Allegro5
    * We don't need a display
    */
   if (!al_init()) {
      printf("Error: cannot initialize Allegro\n");
      return -1;
   }
   if (!al_install_audio()) {
      printf("Error: cannot initialize Allegro Audio\n");
      return -2;
   }
   if (!al_install_keyboard()){
      printf("ERROR cannot install keyboard in Allegro\n");
      return -3;
   }

   /*
    * Provide a default mixer and voice, shoudl be pretty portable.
    */
   ALLEGRO_MIXER *mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                                          ALLEGRO_CHANNEL_CONF_2);
   ALLEGRO_VOICE *main_voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                                               ALLEGRO_CHANNEL_CONF_2);

   al_attach_mixer_to_voice(mixer, main_voice);

   ret = WM_loader_setopts("wildmidi.cfg", samplerate, WM_MO_ENHANCED_RESAMPLING);
   if (!ret) {
      printf("ERROR cannot set WM_loader options\n");
      return -4;
   }
   ret = WM_loader_init(true);
   if (!ret) {
      printf("ERROR cannot initialize WM_loader\n");
      return -5;
   }


   if (usestream) {
      stream = al_load_audio_stream(argv[1], 5, 2000);
      if (!stream) {
         printf("ERROR: cannot load stream %s\n", argv[1]);
         return -100;
      }
      al_attach_audio_stream_to_mixer(stream, mixer);
      al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_LOOP);
      ret = al_set_audio_stream_playing(stream, true);
      if (!ret){
         printf("ERROR cannot play audio stream %s\n", argv[1]);
         return -50;
      }
      printf("Streaming %s \n" , argv[1]);
   } else {
      sample = al_load_sample(argv[1]);
      splinst = al_create_sample_instance(sample);
      al_attach_sample_instance_to_mixer(splinst, mixer);
      ret = al_set_sample_instance_playing(splinst, true);
      if (!ret){
         printf("ERROR cannot play sample %s\n", argv[1]);
         return -50;
      }
      printf("Playing Sample %s \n" , argv[1]);
   }

   fflush(stdout);
   while(!getchar());{};


   al_destroy_sample_instance(splinst);
   al_destroy_sample(sample);
   al_destroy_mixer(mixer);
   al_destroy_voice(main_voice);

   al_destroy_audio_stream(stream);
   WM_loader_shutdown();

   return 0;
}

