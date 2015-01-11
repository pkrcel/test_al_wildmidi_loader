#include "wildmidi_loader.h"

#include <string.h>

ALLEGRO_DEBUG_CHANNEL("acodec")

#define WM_DEFAULT_CONFIGURATION_FILE "wildmidi.cfg"
#define WM_DEFAULT_SAMPLERATE 22050
#define WM_DEFAULT_MIXEROPTS WM_MO_ENHANCED_RESAMPLING

#define WM_QUIT_FEED_THREAD ALLEGRO_GET_EVENT_TYPE('W','M','S','T')


typedef struct _WM_Info _WM_Info;

struct WM_LIB_STATUS{
   bool           initialized;
   char           *cfgfile;
   uint16_t       samplerate;
   uint16_t       mixeropts;
} ;

struct WM_FILE_DATA{
   ALLEGRO_FILE   *fp;
   uint8_t        *midifilebuffer;
   midi           *handle;
   _WM_Info       *wmfdata;
   double         loop_start;       //
   double         loop_end;         // These are fro stream implementation
   ALLEGRO_THREAD *feed_thread;     //
   bool           quit_feed_thread; //
} ;


static WM_LIB_STATUS cur_WM_opts = {
                     false,
                     NULL,
                     WM_DEFAULT_SAMPLERATE,
                     WM_DEFAULT_MIXEROPTS
};



bool WM_init(void){

   if (cur_WM_opts.initialized)
      return true;

   if (!cur_WM_opts.cfgfile){
      const char* defcfg = WM_DEFAULT_CONFIGURATION_FILE;
      cur_WM_opts.cfgfile = al_malloc(strlen(defcfg) + 1);
      strcpy(cur_WM_opts.cfgfile, defcfg);
   }

   /*
    * WildMidi_Init returns 0 if it's all ok....so we negate the outcome.
    */
   cur_WM_opts.initialized = !WildMidi_Init(cur_WM_opts.cfgfile,
                                           cur_WM_opts.samplerate,
                                           cur_WM_opts.mixeropts);

      return cur_WM_opts.initialized;
}

bool WM_shutdown(void){

   if (!cur_WM_opts.initialized)
      return true;

   cur_WM_opts.initialized = false;
   al_free(cur_WM_opts.cfgfile);
   cur_WM_opts.cfgfile = NULL;
   cur_WM_opts.samplerate = WM_DEFAULT_SAMPLERATE;
   cur_WM_opts.mixeropts = WM_DEFAULT_MIXEROPTS;
   return WildMidi_Shutdown();
}

WM_FILE_DATA *WM_open(ALLEGRO_FILE *fp){

   WM_FILE_DATA   *wmdata;
   _WM_Info       *wminfo;
   midi           *handle;
   uint8_t        *midifilebuffer;
   size_t         midibufsize;
   int ret = 0;

   if (!fp){
      ALLEGRO_WARN("WM_open: Failed to open file !\n");
      return NULL;
   }
   /*
    * Handle initialization here.
    * We're counting on condition shortcircuit so if not inited shall call
    * WM_init(), avoiding it otherwise.
    */
   if (!cur_WM_opts.initialized && !WM_init()) {
      ALLEGRO_WARN("Failed to properly Initialize WildMidi!\n");
      return NULL;
   }

   /*
    * Transfer file to memory buffer, so we can handle that with
    * WildMidi_OpenBuffer(), cause otherwise this could not have an
    * ALLEGRO_FILE implementation.
    * This MIGHT be suboptimal, still MIDI and XMI files are usually small.
    */
   midibufsize = al_fsize(fp);
   midifilebuffer = al_malloc(midibufsize);
   al_fread(fp, midifilebuffer, midibufsize);
   handle = WildMidi_OpenBuffer(midifilebuffer, midibufsize);
   if (!handle) {
      ALLEGRO_WARN("WM_open: Coudl not access MIDI data\n");
      return NULL;
   }
   wminfo = WildMidi_GetInfo(handle);
   wmdata = al_malloc(sizeof *wmdata);
   wmdata->fp = fp;
   wmdata->handle = handle;
   wmdata->midifilebuffer = midifilebuffer;
   wmdata->wmfdata = wminfo;

   return wmdata;
}

static void WM_close(WM_FILE_DATA *wmfdata){

   WildMidi_Close(wmfdata->handle);
//   al_fclose(wmfdata->fp);   should be handled by caller.
   al_free(wmfdata->midifilebuffer);
}

/*
 * Returns -1 in case of problems, else the actual number of bytes xfer'd to
 * the destination buffer.
 */
static size_t WM_read(WM_FILE_DATA *wmdata, uint8_t *data, size_t bytes){

   return WildMidi_GetOutput(wmdata->handle, data, bytes);

}

static bool WM_stream_seek(ALLEGRO_AUDIO_STREAM * stream, double time)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA *) al_get_audio_stream_userdata(stream);

//   int align = al_get_audio_depth_size(ALLEGRO_AUDIO_DEPTH_INT16)
//               * al_get_channel_count(ALLEGRO_CHANNEL_CONF_2);
   /* following the desired seek position in samples */
   unsigned long cpos= time * (double)cur_WM_opts.samplerate;

   if (cpos >= wmfile->wmfdata->approx_total_samples ||
       time >= wmfile->loop_end)
      return false;

   return WildMidi_FastSeek(wmfile->handle, &cpos);
}

static bool WM_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA *) al_get_audio_stream_userdata(stream);
   return WM_stream_seek(stream, wmfile->loop_start);
}

static double WM_stream_get_position(ALLEGRO_AUDIO_STREAM * stream)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);
   wmfile->wmfdata = WildMidi_GetInfo(wmfile->handle);
   double pos = 0.0;
   pos += (double)wmfile->wmfdata->current_sample;
   pos /= (double)cur_WM_opts.samplerate;
   return pos;
}

static double WM_stream_get_length(ALLEGRO_AUDIO_STREAM * stream)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);
   /* this might work but needs documentation proof from Mindwerks
    * looking into wildmidi code it does not seem to hold any actual info.
    * double total_time = (double)wmfile->wmfdata->total_midi_time;
    */
   double total_time = 0.0;
   total_time += (double)wmfile->wmfdata->approx_total_samples;
   total_time /= (double)cur_WM_opts.samplerate;
   return total_time;
}

static bool WM_stream_set_loop(ALLEGRO_AUDIO_STREAM * stream, double start, double end)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);
   wmfile->loop_start = start;
   wmfile->loop_end = end;
   return true;
}

/* WM_stream_update:
 *  Updates 'stream' with the next chunk of data.
 *  Returns the actual number of bytes written.
 */
static size_t WM_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
   size_t buf_size)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);
   ALLEGRO_PLAYMODE loopmode= al_get_audio_stream_playmode(stream);
   const int bytes_per_sample = 4;
   int samples, samples_read;
   double ctime, btime;

   ctime = WM_stream_get_position(stream);
   btime = 0.0;
   btime += (double)buf_size / (double)bytes_per_sample;
   btime /= (double)cur_WM_opts.samplerate;

   if (loopmode == _ALLEGRO_PLAYMODE_STREAM_ONEDIR
       && ctime + btime > wmfile->loop_end) {
      samples = ((wmfile->loop_end - ctime) * (double)(cur_WM_opts.samplerate));
   }
   else {
      samples = buf_size / bytes_per_sample;
   }
   if (samples < 0)
      return 0;

   samples_read = WM_read(wmfile, data, samples);

   return samples_read * bytes_per_sample;
}

void wildmidi_stop_feed_thread(ALLEGRO_AUDIO_STREAM *stream)
{
   ALLEGRO_EVENT quit_event;
   WM_FILE_DATA *wmfile = al_get_audio_stream_userdata(stream);

   quit_event.type = WM_QUIT_FEED_THREAD;
   al_emit_user_event(al_get_audio_stream_event_source(stream), &quit_event, NULL);
   al_join_thread(wmfile->feed_thread, NULL);
   al_destroy_thread(wmfile->feed_thread);

   wmfile->feed_thread = NULL;
}


/* WM_stream_close:
 *  Closes the 'stream'.
 */
static void WM_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);

   wildmidi_stop_feed_thread(stream);

   al_fclose(wmfile->fp);
   WM_close(wmfile);
   wmfile->feed_thread = NULL;
   al_set_audio_stream_userdata(stream, NULL);
}

/* wildmidi_feed_stream:
 * This runs in another thread and feeds the stream as necessary.
 * Uses WM_stream backend in this translation unit.
 */
void *WM_feed_stream(ALLEGRO_THREAD *self, void *vstream)
{
   ALLEGRO_AUDIO_STREAM *stream = vstream;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_EVENT event;
   (void)self;
   WM_FILE_DATA *wmfile = (WM_FILE_DATA*) al_get_audio_stream_userdata(stream);
   volatile bool stream_is_draining = false;

   ALLEGRO_DEBUG("Stream feeder thread started for stream at %x.\n", stream);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_audio_stream_event_source(stream));

   wmfile->quit_feed_thread = false;

   while (!wmfile->quit_feed_thread) {
      char *fragment;
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT
          && !stream_is_draining) {
         unsigned long bytes;
         unsigned long bytes_written;

         fragment = al_get_audio_stream_fragment(stream);
         if (!fragment) {
            /* This is not an error. */
            continue;
         }

         bytes = al_get_audio_stream_length(stream) *
               al_get_channel_count(ALLEGRO_CHANNEL_CONF_2) *
               al_get_audio_depth_size(ALLEGRO_AUDIO_DEPTH_INT16);

         //maybe_lock_mutex(stream->spl.mutex);
         bytes_written = WM_stream_update(stream, fragment, bytes);
         //maybe_unlock_mutex(stream->spl.mutex);

        /* In case it reaches the end of the stream source, stream feeder will
         * fill the remaining space with silence. If we should loop, rewind the
         * stream and override the silence with the beginning.
         * In extreme cases we need to repeat it multiple times.
         */
         while (bytes_written < bytes &&
                al_get_audio_stream_playmode(stream) == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
            size_t bw;
            WM_stream_rewind(stream);
            //maybe_lock_mutex(stream->spl.mutex);
            bw = WM_stream_update(stream, fragment + bytes_written,
               bytes - bytes_written);
            bytes_written += bw;
            //maybe_unlock_mutex(stream->spl.mutex);
         }

         if (!al_set_audio_stream_fragment(stream, fragment)) {
            ALLEGRO_ERROR("Error setting stream buffer.\n");
            continue;
         }

         /* The streaming source doesn't feed any more, drain buffers and quit. */
         if (bytes_written != bytes &&
            al_get_audio_stream_playmode(stream) == _ALLEGRO_PLAYMODE_STREAM_ONCE) {
            al_drain_audio_stream(stream);
            stream_is_draining = true;
            wmfile->quit_feed_thread = true;
         }
      }
      else if (event.type == WM_QUIT_FEED_THREAD) {
         wmfile->quit_feed_thread = true;
      }
   }

   event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FINISHED;
   event.user.timestamp = al_get_time();
   al_emit_user_event(al_get_audio_stream_event_source(stream), &event, NULL);

   al_destroy_event_queue(queue);

   ALLEGRO_DEBUG("Stream feeder thread finished.\n");

   return NULL;
}


/* public functions */

bool WM_setopts(const char *cfg, uint16_t rate, uint16_t mixeropts){

   cur_WM_opts.cfgfile = al_realloc(cur_WM_opts.cfgfile, strlen(cfg) + 1);
   strcpy(cur_WM_opts.cfgfile, cfg);
   cur_WM_opts.samplerate = rate;
   cur_WM_opts.mixeropts = mixeropts;
}


ALLEGRO_SAMPLE *load_sample_wildmidi(const char *filename){

   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
//   ASSERT(filename);

   ALLEGRO_INFO("Loading WM sample %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Failed reading %s.\n", filename);
      return NULL;
   }

   spl = load_sample_wildmidi_f(f);

   al_fclose(f);

   return spl;
}



ALLEGRO_SAMPLE *load_sample_wildmidi_f(ALLEGRO_FILE *fp){

   WM_FILE_DATA *wmdata;
   ALLEGRO_SAMPLE *spl = NULL;
   size_t midiPCMbufsize;
   uint8_t *midiPCMbuffer;
   int ret;

   /*
    * Open the MIDI data from file into the WM_FILE_DATA struct.
    */
   wmdata = WM_open(fp);

   /*
    * It takes 4 bytes for each sample cause these are Stereo 16 Bits Signed PCM
    * data each one.
    * We then proceed to extract synth PCM data from the midi buffer.
    */
   midiPCMbufsize = wmdata->wmfdata->approx_total_samples * 4;
   midiPCMbuffer = al_malloc(midiPCMbufsize);
   ret = WM_read(wmdata, midiPCMbuffer, midiPCMbufsize);
   if (ret == -1) {
      ALLEGRO_WARN("load_wildmidi sample: error on extracitng PCM data\n");
      return NULL;

   }

   spl = al_create_sample(midiPCMbuffer,
                          wmdata->wmfdata->approx_total_samples,
                          cur_WM_opts.samplerate,
                          ALLEGRO_AUDIO_DEPTH_INT16,
                          ALLEGRO_CHANNEL_CONF_2,
                          true
                          );
   if (!spl) {
      ALLEGRO_WARN("Error on creating sample from PCM data.\n");
      return NULL;
   }

   al_free(wmdata);

   return spl;
}

/*
 * Audio Stream Implementation, templated from Allegro's
 * Of course not being "into" Allegro we have to rely on the streaming API.
 */


ALLEGRO_AUDIO_STREAM *wildmidi_load_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   //ASSERT(filename);

   f = al_fopen(filename, "rb");
   if (!f)
      return NULL;

   stream = wildmidi_load_audio_stream_f(f, buffer_count, samples);
   if (!stream) {
      al_fclose(f);
   }

   return stream;
}

ALLEGRO_AUDIO_STREAM *wildmidi_load_audio_stream_f(ALLEGRO_FILE* f,
   size_t buffer_count, unsigned int samples)
{
   WM_FILE_DATA* wmfiledata;
   ALLEGRO_AUDIO_STREAM* stream;

   wmfiledata = WM_open(f);

   if (wmfiledata == NULL)
      return NULL;

   stream = al_create_audio_stream(buffer_count, samples,
                                   cur_WM_opts.samplerate,
                                   ALLEGRO_AUDIO_DEPTH_INT16,
                                   ALLEGRO_CHANNEL_CONF_2);

   if (stream) {
      al_set_audio_stream_userdata(stream, wmfiledata);
      wmfiledata->loop_start = 0.0;
      wmfiledata->loop_end = WM_stream_get_length(stream);
      wmfiledata->feed_thread = al_create_thread(WM_feed_stream, stream);
//      stream->feeder = wav_stream_update;
//      stream->unload_feeder = wav_stream_close;
//      stream->rewind_feeder = wav_stream_rewind;
//      stream->seek_feeder = wav_stream_seek;
//      stream->get_feeder_position = wav_stream_get_position;
//      stream->get_feeder_length = wav_stream_get_length;
//      stream->set_feeder_loop = wav_stream_set_loop;
      al_start_thread(wmfiledata->feed_thread);
   }
   else {
      WM_close(wmfiledata);
   }

   return stream;
}





