#include <gst/gst.h>
#include <glib.h>


static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad;
  GstElement *sink = (GstElement *) data;

  g_print ("Dynamic pad created, linking source/sink\n");

  sinkpad = gst_element_get_static_pad (sink, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}

int
main (int   argc,
      char *argv[])
{
  GMainLoop *loop;

  GstElement *pipeline, *source, *sink;
  GstBus *bus;
  guint bus_watch_id;

  /* Initialisation */
  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);


  // /* Check input arguments */
  // if (argc != 2) {
  //   g_printerr ("Usage: %s <Ogg/Vorbis filename>\n", argv[0]);
  //   return -1;
  // }


  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("rtsp-player");
  source   = gst_element_factory_make ("rtspsrc",  "src-source");
  sink     = gst_element_factory_make ("filesink", "sink-output");

  if (!pipeline || !source || !sink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

  /* Set up the pipeline */

  /* we set the input filename to the source element */
  g_object_set (G_OBJECT (source), 
    //"location", "rtsp://184.72.239.149/vod/mp4:BigBuckBunny_175k.mov", 
    "location", "rtsp://0.0.0.0:8554/live.sdp",
    "protocols", 4,//(1 << 2),
    NULL);

  GstElement *capsfilter = gst_element_factory_make ("capsfilter", NULL); 
  GstCaps *caps = gst_caps_from_string ("application/x-rtp"); 
  g_object_set (G_OBJECT(capsfilter)
    , "caps", caps, 
    NULL); 

  g_object_set (G_OBJECT (sink), 
    "location", "filesink.output", 
    NULL);

  /* we add a message handler */
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  /* we add all elements into the pipeline */
  gst_bin_add_many (GST_BIN (pipeline),
                    source, capsfilter, sink, NULL);

  /* we link the elements together */
  //gst_element_link (source, sink);
  gst_element_link_many (source, capsfilter, sink, NULL);
  g_signal_connect (source, "pad-added", G_CALLBACK (on_pad_added), sink);

  /* Set the pipeline to "playing" state*/
  g_print ("Now playing: %s\n", argv[1]);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);


  /* Iterate */
  g_print ("Running...\n");
  g_main_loop_run (loop);


  /* Out of the main loop, clean up nicely */
  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
