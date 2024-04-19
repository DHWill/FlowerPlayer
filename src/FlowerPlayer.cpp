#include <cstdlib>
#include <iostream>
#include <gst/gst.h>
#include <glib-object.h>
#include <unistd.h>
#include <pthread.h>
#include "statemachine.h"
#include "RampGenerator.h"
#include "Sensor/SensorGrabber.h"
#include <mutex>

gint fps = 60;				//FRAMERATE!
gfloat g_frame_interval = 1./fps*1000;
gint64 gstInterval = GST_SECOND / fps;


gint64 lastFrame = 0;
bool speedUp = false;
bool slowDown = false;
RampGenerator rampGen, rampGenColor, rampGenTransform;


typedef struct{
	SensorMan *sensorMan = nullptr;
	StateMachine *stateMachine = nullptr;
	GstElement *videosink = nullptr;
	GstElement *demuxer = nullptr;
	GstElement *decoder= nullptr;
	GstElement *pipeline = nullptr;
	GstElement *pipeRate = nullptr;
	GstElement *textOverlay = nullptr;
	GMainLoop *loop = nullptr;
	GValue *textToOverlay = nullptr;
	float rate = 2;

} PlayerData;


/*
 * GLImage sink appears to work very well at 1920x1920, whereas waylandsink drops a lot of frames
 * Wayland sink appears to work better @ 4K whereas whereas GLImagesink drops a lot of frames
 */

/*=====================================================================================
 * Pipeline Functions
 * ==================================================================================*/
static void update_rate(GstElement *pipeline, float rate) {

    GstEvent *seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, (GstSeekFlags)(GST_SEEK_FLAG_INSTANT_RATE_CHANGE | GST_SEEK_FLAG_SKIP),
    											GST_SEEK_TYPE_NONE, 0,
												GST_SEEK_TYPE_NONE, 0);


    if(gst_element_send_event(pipeline, seek_event)){
//    	std::cout << "chnagingRate: " << rate << std::endl;
    }
    else{
    	std::cout << "could not change rate" << std::endl;
    }
}

static bool updateStateMachine(gpointer _playerData){
	PlayerData *playerData = (PlayerData*) _playerData;
	int distance = playerData->sensorMan->getPositionDistance();
	switch(distance) {
	  case 0:
		//Far
		  playerData->stateMachine->setTargetPosition(0);
	    break;
	  case 1:
	    //Near
		  playerData->stateMachine->setTargetPosition(0);
	    break;
	  case 2:
	    //Close
		  playerData->stateMachine->setTargetPosition(1);
	    break;
	  default:
		  break;
	}
}

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data){
    GstPad *sinkpad;
    GstElement *decoder = (GstElement *) data;
    std::cout << "Dynamic pad created, linking demuxer/decoder" << std::endl;
    sinkpad = gst_element_get_static_pad (decoder, "sink");
    gst_pad_link (pad, sinkpad);
    gst_object_unref (sinkpad);
}

static void seek_to_frame(GstElement *pipeline, int frame_start, int frame_end, bool isInit, float rate) {

	GstSeekFlags seekFlags = (GstSeekFlags)(GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_SNAP_AFTER | GST_SEEK_FLAG_SEGMENT);

	if(isInit){
		seekFlags = (GstSeekFlags)(GST_SEEK_FLAG_KEY_UNIT | GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SEGMENT);
	}

    gint64 time_nanoseconds_start = (frame_start) * gstInterval;
    gint64 time_nanoseconds_end = (frame_end) * gstInterval;

    GstEvent *seek_event = gst_event_new_seek(rate, GST_FORMAT_TIME, seekFlags,
                                              GST_SEEK_TYPE_SET, time_nanoseconds_start,
                                              GST_SEEK_TYPE_SET, time_nanoseconds_end);

    if(gst_element_send_event(pipeline, seek_event)){
    	std::cout << "Seeking! StartFrame:" << frame_start << " EndFrame :" << frame_end << rate << std::endl;
    	update_rate(pipeline, rate);
    }
    else{
    	std::cout << "Seek Failed!" << std::endl;
    }
}
/*===================================================================================
 * Keyoard

bool isPaused = 0;
static gboolean handle_keyboard (GIOChannel * source, GIOCondition cond, gpointer *_playerData){
	PlayerData *playerData = (PlayerData*) _playerData;
    gchar *str = NULL;

    if (g_io_channel_read_line (source, &str, NULL, NULL,
                               NULL) != G_IO_STATUS_NORMAL) {
            std::cout << str[0] << std::endl;
            return TRUE;
    }

    switch (g_ascii_tolower (str[0])) {
    case 'f':
//    	(pbRate <= 2) ? pbRate += 0.1 : pbRate = 2;
//    	update_rate((GstElement*)playerData->demuxer, pbRate);
//    	rampGen.generateRamp(10);
    	slowDown = false;
    	break;

    case 's':
//    	(pbRate >= 0.1) ? pbRate -=0.1 : pbRate = 0.1;
//    	update_rate((GstElement*)playerData->demuxer, pbRate);
//    	rampGen.generateRamp(10);
    	slowDown = true;
    	break;

    case 'z':
    	update_rate((GstElement*)playerData->demuxer, 2);
    	break;
    case 'x':
    	update_rate((GstElement*)playerData->demuxer, 1);
    	break;

    case 'p':
            if(!isPaused){
                gst_element_set_state((GstElement*)playerData->pipeline, GST_STATE_PAUSED);
                std::cout << "pause" << std::endl;
            }
            else{
                gst_element_set_state((GstElement*)playerData->pipeline, GST_STATE_PLAYING);
                std::cout << "play" << std::endl;
            }
            isPaused = !isPaused;
            break;

    case 'e':
    	seek_to_frame(playerData->pipeline, playerData->stateMachine->currentSegment.endTime - 30, playerData->stateMachine->currentSegment.endTime, true, playerData->rate);
            break;
    default:
            break;
    }

    g_free (str);

    return TRUE;
}
 * ==================================================================================*/

/*===================================================================================
 * Player Logic
 * ==================================================================================*/
static gboolean on_end_of_seek(GstBus *bus, GstMessage *message, gpointer *_playerData) {
	PlayerData *playerData = (PlayerData*) _playerData;

	switch (GST_MESSAGE_TYPE(message)){
		case GST_MESSAGE_SEGMENT_DONE: {
			updateStateMachine(_playerData);
			playerData->stateMachine->updateSegment();
			seek_to_frame(playerData->demuxer,playerData->stateMachine->currentSegment.startTime,playerData->stateMachine->currentSegment.endTime, false, playerData->rate);
			break;
		}
		default:
			break;
	}

	return TRUE;
}



int outOfFrameCounter = 0;
static gboolean query_position(gpointer *_playerData){
	PlayerData *playerData = (PlayerData*) _playerData;
	gint64 pos;

    if(gst_element_query_position(playerData->pipeline, GST_FORMAT_TIME, &pos)){
    	gint64 frame = pos / gstInterval;
    	g_value_set_string(playerData->textToOverlay, g_strdup_printf("%i", frame));
    	g_object_set_property(G_OBJECT(playerData->textOverlay), "text", playerData->textToOverlay);


    	//-------------------Sensor------------------------------
//		if(outOfFrameCounter >= fps/2){
//			if(playerData->sensorMan->getPositionDistance() == -1){
//				slowDown = false;
//			}
//			else {
//				slowDown = false;
//			}
//			outOfFrameCounter = 0;
//	    }


    	if(frame != lastFrame){
    		//------------------Init----------------------------------
			if (playerData->stateMachine->getIsInit()) {
				playerData->stateMachine->updateSegment();
				seek_to_frame(playerData->demuxer,playerData->stateMachine->currentSegment.startTime,playerData->stateMachine->currentSegment.endTime, true, playerData->rate);
				return TRUE;
			}


			//------------------SpeedRamp------------------------------
			if(outOfFrameCounter % ((fps/16) + 1) == 0){
				if(slowDown){
					if(rampGen.loop(0.5)){
						update_rate(playerData->pipeline, rampGen.posXI);
//						std::cout << rampGen.posXI << std::endl;
					}
				}
				else{
					if(rampGen.loop(1.0)){
						update_rate(playerData->pipeline, rampGen.posXI);
//						std::cout << rampGen.posXI << std::endl;
					}
				}
			}
    	}
    }
    outOfFrameCounter ++;
//    outOfFrameCounter %= fps;
    return TRUE;
}


/*===========================================================================
 * PlayerMachine
 * ========================================================================*/

int main(int argc, char *argv[]) {

    PlayerData *playerData = g_new(PlayerData, 1);

	SensorMan _sensorMan;
	SensorMan *sensorManPt = &_sensorMan;
	playerData->sensorMan = sensorManPt;
	playerData->sensorMan->startSensors();

	rampGen.vSpeed = 0.1;

 	StateMachine _stateMachine;
	StateMachine *stateMachinePt = &_stateMachine;

    playerData->stateMachine=stateMachinePt;
    playerData->rate= 1.0;

	std::string fileLocation = "";
    if (argc < 3) {
          g_print("Usage: %s <image-file> <state-file>\n", argv[0]);
          return -1;
      }
    else{
    	fileLocation = argv[1];
    	playerData->stateMachine->parseFile(argv[2]);
    }

    gst_init(&argc, &argv);
    std::cout << "CreateElements" << std::endl;
    playerData->pipeline = gst_pipeline_new("pipeline");
    GstElement *filesrc = gst_element_factory_make("filesrc", "filesrc");
    playerData->demuxer = gst_element_factory_make("qtdemux", "qtdemux");
    GstElement *queue1 = gst_element_factory_make("queue", "queue1");
    GstElement *queue0 = gst_element_factory_make("queue", "queue0");
    GstElement *h264parse = gst_element_factory_make("h264parse", "h264parse");
    GstElement *imxvideoconvert_g2d = gst_element_factory_make("imxvideoconvert_g2d", "imxvideoconvert_g2d");
//    playerData->videosink = gst_element_factory_make("waylandsink", "waylandsink");
    playerData->pipeRate = gst_element_factory_make("videorate", "videorate");
    playerData->videosink = gst_element_factory_make("glimagesink", "glimagesink");
    playerData->decoder = gst_element_factory_make("v4l2h264dec", "v4l2h264dec");
    GstElement *capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    GstElement *videorate = gst_element_factory_make("videorate", "videorate");
    playerData->textOverlay = gst_element_factory_make("textoverlay", "textoverlay");




    std::cout << "CreateCaps" << std::endl;
    GstCaps *caps = gst_caps_new_simple (
       "video/x-h264",
       "framerate", GST_TYPE_FRACTION, 60, 1,
       "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
       "width", G_TYPE_INT, 1920,
       "height", G_TYPE_INT, 1920,
       NULL);

    std::cout << "SetCaps" << std::endl;
    g_object_set(G_OBJECT(capsfilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    std::cout << "CreateWidth" << std::endl;
    //2160 3840
    // Create a GValue for width
    GValue width = G_VALUE_INIT;
    g_value_init(&width, G_TYPE_INT);
    g_value_set_int(&width, 1920);	//birds

    // Create a GValue for height
    GValue height = G_VALUE_INIT;
    g_value_init(&height, G_TYPE_INT);
    g_value_set_int(&height,1920);	//birds

    // Create a GValue for height
    GValue top = G_VALUE_INIT;
    g_value_init(&top, G_TYPE_INT);
    g_value_set_int(&top, 0);

    GValue new_dimensions = G_VALUE_INIT;
    g_value_init(&new_dimensions, GST_TYPE_ARRAY);
    gst_value_array_append_value(&new_dimensions, &top);
    gst_value_array_append_value(&new_dimensions, &top);
    gst_value_array_append_value(&new_dimensions, &width);
    gst_value_array_append_value(&new_dimensions, &height);
    g_object_set_property(G_OBJECT(playerData->videosink), "render-rectangle", &new_dimensions);


    std::cout << "Set:Width/Height" << std::endl;
//    g_object_set_property(G_OBJECT(playerData->videosink), "window-height", &height);
//    g_object_set_property(G_OBJECT(playerData->videosink), "window-width", &width);


    std::cout << "Set:Sync" << std::endl;
    GValue sync = G_VALUE_INIT;
    g_value_init(&sync, G_TYPE_INT);
    g_value_set_int(&sync, 0);
    g_object_set_property(G_OBJECT(playerData->videosink), "sync", &sync);

//    std::cout << "Set:Qos for dodgy V4l2Dec" << std::endl;
//    GValue qos = G_VALUE_INIT;
//    g_value_init(&qos, G_TYPE_INT);
//    g_value_set_int(&qos, 1);
//    g_object_set_property(G_OBJECT(playerData->videosink), "qos", &sync);

    std::cout << "Set:Queue" << std::endl;
    GValue queueName = G_VALUE_INIT;
    g_value_init(&queueName, G_TYPE_STRING);
    g_value_set_string(&queueName, "queue1");
    g_object_set_property(G_OBJECT(queue1), "name", &queueName);

    std::cout << "Set:File" << std::endl;
    g_object_set(G_OBJECT(filesrc), "location", fileLocation.c_str(), NULL);

    GValue textToOverlayPt = G_VALUE_INIT;
    playerData->textToOverlay = &textToOverlayPt;
    g_value_init(playerData->textToOverlay, G_TYPE_STRING);
    g_value_set_string(playerData->textToOverlay, "0");
    g_object_set_property(G_OBJECT(playerData->textOverlay), "text", playerData->textToOverlay);

    std::cout << "AddElements" << std::endl;
    gst_bin_add_many(GST_BIN(playerData->pipeline), filesrc, playerData->demuxer, queue0, h264parse, capsfilter, playerData->decoder, imxvideoconvert_g2d, queue1, playerData->textOverlay, playerData->videosink, NULL);

    /* we link the elements together */
    /* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -> alsa-output */
    std::cout << "Link Filesrc/Demuxer" << std::endl;
    gst_element_link_many (filesrc, playerData->demuxer, NULL);
    gst_element_link(playerData->decoder, capsfilter);
    gst_element_link(capsfilter, playerData->videosink);
    std::cout << "Link Rest of elements" << std::endl;
    gst_element_link_many (queue0, h264parse, playerData->decoder, imxvideoconvert_g2d, queue1, playerData->textOverlay, playerData->videosink, NULL);
    g_signal_connect(playerData->demuxer, "pad-added", G_CALLBACK (on_pad_added), queue0);

    std::cout << "Set bus Message Watch" << std::endl;
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(playerData->pipeline));
    gst_bus_add_signal_watch(bus);

    std::cout << "Create gLoop" << std::endl;
    playerData->loop=g_main_loop_new(NULL, FALSE);
    g_signal_connect(bus, "message", G_CALLBACK(on_end_of_seek), playerData);



    /////////////////////////////////////////////////////////////////
//    GstPad *src_pad = gst_element_get_static_pad(filesrc, "src");
//    GstCaps *caps = gst_pad_get_current_caps(src_pad);
//
//    if (!caps) {
//        g_printerr("Failed to get caps. Exiting.\n");
////        return -1;
//    }
//
//    // Extract frame rate from caps
//    gint num, denom;
//    GstStructure *structure = gst_caps_get_structure(caps, 0);
//    gst_structure_get_fraction(structure, "framerate", &num, &denom);
//
//    g_print("Frame rate: %d/%d\n", num, denom);
//
//    // Clean up
//    gst_caps_unref(caps);
    /////////////////////////////////////////////////////////////////


    //Keybaord THIS HAS TO BE REMOVED IF RAN AS SERVICE
    /*
    GIOChannel *io_stdin;
    io_stdin = g_io_channel_unix_new (fileno (stdin));
    g_io_add_watch (io_stdin, G_IO_IN, (GIOFunc) handle_keyboard, playerData);
    */
    //Keybaord THIS HAS TO BE REMOVED IF RAN AS SERVICE

    // Get the caps from filesrc
//     GstCaps *caps;
//     g_object_get(G_OBJECT(filesrc), "caps", &caps, NULL);
//
//     // Print the caps information
//     gchar *capsString = gst_caps_to_string(caps);
//     g_print("Caps: %s\n", capsString);

    std::cout << "Set Pipe Playing" << std::endl;
    std::cout << "Set Pipe Playing" << std::endl;
    std::cout << "Set Pipe Playing" << std::endl;
    std::cout << "Set Pipe Playing" << std::endl;
    gst_element_set_state(playerData->pipeline, GST_STATE_PLAYING);


    std::cout << "Set Timeout query position" << std::endl;
    g_timeout_add(g_frame_interval , (GSourceFunc) query_position, playerData);     //1.0 is a Gst clock



    std::cout << "Run main GLoop" << std::endl;
    g_main_loop_run(playerData->loop);


    gst_element_set_state(playerData->pipeline, GST_STATE_NULL);
    gst_object_unref(playerData->pipeline);

    return 0;
}
