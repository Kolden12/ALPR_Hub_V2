#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <sqlite3.h>
#include "gstnvdsmeta.h"

static void cb_newpad(GstElement *decodebin, GstPad *pad, gpointer data) {
    GstElement *muxer = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_request_pad(muxer, "sink_0");
    gst_pad_link(pad, sinkpad);
    gst_object_unref(sinkpad);
}

static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    NvDsBatchMeta *batch_meta = gst_nvds_get_batch_meta((GstBuffer *)info->data);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == 0) { // License Plate
                // Production: Save hit to SQLite Hub DB
                printf("ALPR HIT: %s\n", obj_meta->obj_label);
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    GstElement *pipeline = gst_pipeline_new("sabre-pipeline");
    GstElement *source = gst_element_factory_make("rtspsrc", "src");
    GstElement *decodebin = gst_element_factory_make("decodebin", "decode");
    GstElement *streammux = gst_element_factory_make("nvstreammux", "mux");
    GstElement *pgie = gst_element_factory_make("nvinfer", "pgie");
    GstElement *nvosd = gst_element_factory_make("nvdsosd", "osd");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    g_object_set(G_OBJECT(source), "location", "rtsp://192.168.1.101/stream", "latency", 0, NULL);
    g_object_set(G_OBJECT(streammux), "width", 1920, "height", 1080, "batch-size", 1, NULL);
    g_object_set(G_OBJECT(pgie), "config-file-path", "config/config_infer_primary_yolo.txt", NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, decodebin, streammux, pgie, nvosd, sink, NULL);

    g_signal_connect(source, "pad-added", G_CALLBACK(cb_newpad), decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(cb_newpad), streammux);
    gst_element_link_many(streammux, pgie, nvosd, sink, NULL);

    GstPad *osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_pad_buffer_probe, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);
    return 0;
}
