
#include <string.h>
#include <stdio.h>

#include <wildmidi_lib.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "wildmidi_loader.h"


char *getFileName(char *_path){
  // sanitize application name - only windows for now
  // path separator is '\' even in MSYS2 (and since Cygwin 1.7 seems)

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
   const uint32_t samplerate = 44100;
   int ret;
   if (argc < 2) {
      printf("Usage: %s <filename>\n", getFileName(argv[0]));
      return -0xFFFFFFF;
   }
   if (!al_init()) {
      printf("Error: cannot initialize Allegro\n");
      return -1;
   }
   if (!al_init_acodec_addon()) {
      printf("Error: cannot initialize Allegro ACODEC loader\n");
      return -2;
   }
   if (!al_install_audio()) {
      printf("Error: cannot initialize Allegro Audio\n");
      return -2;
   }
   ret = al_reserve_samples(10);
   if (!ret){
      printf("ERROR cannot reserve samples\n");
      return -50;
   }
   if (!al_install_keyboard()){
      printf("ERROR cannot install keyboard\n");
      return -60;
   }

   ALLEGRO_MIXER *mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                                          ALLEGRO_CHANNEL_CONF_2);
   ALLEGRO_VOICE *main_voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                                               ALLEGRO_CHANNEL_CONF_2);

   al_attach_mixer_to_voice(mixer, main_voice);

   ALLEGRO_AUDIO_STREAM *stream = NULL;
   ret = al_register_sample_loader(".xmi", load_sample_wildmidi);
   ret = al_register_sample_loader_f(".xmi", load_sample_wildmidi_f);
   ret = al_register_audio_stream_loader(".xmi", wildmidi_load_audio_stream);

   WM_setopts("wildmidi.cfg", 44100, WM_MO_LOG_VOLUME);

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
   fflush(stdout);

   while(!getchar());{};

   al_destroy_audio_stream(stream);
   al_destroy_mixer(mixer);
   al_destroy_voice(main_voice);

   return 0;
}

