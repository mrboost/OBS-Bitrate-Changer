#include <obs-frontend-api.h>
#include <obs-module.h>
#include <plugin-support.h>
#include <time.h>
#include <shlobj.h>
#include <obs.h>
#include <pthread.h>
#include <windows.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")
long long original_bitrate = 0;

void set_video_bitrate(int new_bitrate);

void *change_bitrate(void *data) {
    (void)data;

    Sleep(10000);

    set_video_bitrate(20000);

return NULL;
}

void set_video_bitrate(int new_bitrate) {
    blog(LOG_INFO, "Changing Bitrate!");

    obs_output_t *streaming_output = obs_frontend_get_streaming_output();
    if (!streaming_output) {
        blog(LOG_WARNING, "No streaming output found.");
        return;
    }

    obs_encoder_t *video_encoder = obs_output_get_video_encoder(streaming_output);
    if (!video_encoder) {
        blog(LOG_WARNING, "No video encoder found.");
        obs_output_release(streaming_output);
        return;
    }

    obs_data_t *settings = obs_encoder_get_settings(video_encoder);
    if (!settings) {
        blog(LOG_WARNING, "Failed to get encoder settings.");
        obs_encoder_release(video_encoder);
        obs_output_release(streaming_output);
        return;
    }

    long long current_bitrate = obs_data_get_int(settings, "bitrate");
    blog(LOG_INFO, "Current bitrate: %lld kbps", current_bitrate);

    original_bitrate = obs_data_get_int(settings, "bitrate");
    blog(LOG_INFO, "Current bitrate: %lld kbps", original_bitrate);

    if (new_bitrate != original_bitrate) {
        obs_data_set_int(settings, "bitrate", new_bitrate);
        obs_encoder_update(video_encoder, settings);
        blog(LOG_INFO, "Bitrate changed to %d kbps", new_bitrate);
    }

    obs_data_set_int(settings, "bitrate", new_bitrate);


    obs_encoder_update(video_encoder, settings);

    blog(LOG_INFO, "Bitrate changed to %d kbps", new_bitrate);
}

static void stream_start() {
    pthread_t bitrate_thread;
    pthread_create(&bitrate_thread, NULL, change_bitrate, NULL);
    pthread_detach(bitrate_thread);
}

static void stream_stop() {
    if (original_bitrate > 0) {
        set_video_bitrate((int)original_bitrate);
        blog(LOG_INFO, "Bitrate reset to original value: %lld kbps", original_bitrate);
    }
}

void callback(enum obs_frontend_event event, void* data)
{
    (void)data;

    switch(event)
    {
        case OBS_FRONTEND_EVENT_STREAMING_STARTED:
            stream_start();
            break;

        case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
            stream_stop();
            break;
    }
}

bool obs_module_load(void)
{
    blog(LOG_INFO, "plugin loaded successfully (version %s)", PLUGIN_VERSION);

    obs_frontend_add_event_callback(callback, 0);
    return true;
}

void obs_module_unload()
{
    obs_frontend_remove_event_callback(callback, 0);

    blog(LOG_INFO, "plugin unloaded");
}
