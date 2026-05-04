#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <sqlite3.h>
#include "gstnvdsmeta.h"
#include "nvds_yml_parser.h"

// SQLite handle
static sqlite3 *db;

static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    NvDsBatchMeta *batch_meta = gst_nvds_get_batch_meta((GstBuffer *)info->data);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == 0) { // License Plate
                // Production Logic:
                // 1. Save crop (conceptually via nvds_obj_enc)
                // 2. Insert into DB
                char *zErrMsg = 0;
                const char* sql = "INSERT INTO hits (timestamp_iso8601, plate_text, plate_confidence, hub_uuid, sha256_hash) VALUES (datetime('now'), ?, ?, 'HUB-001', 'HASH-PLACEHOLDER');";
                sqlite3_stmt *stmt;
                sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                sqlite3_bind_text(stmt, 1, obj_meta->obj_label, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 2, obj_meta->confidence);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                printf("PLATE HIT: %s Saved to DB.\n", obj_meta->obj_label);
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    sqlite3_open("/mnt/sabre_storage/sabre_hub.db", &db);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("sabre-pipeline");

    // Elements as defined in ds_config.txt
    GstElement *source = gst_element_factory_make("rtspsrc", "src");
    GstElement *streammux = gst_element_factory_make("nvstreammux", "mux");
    GstElement *pgie = gst_element_factory_make("nvinfer", "pgie");
    GstElement *sgie = gst_element_factory_make("nvinfer", "sgie");
    GstElement *nvvidconv = gst_element_factory_make("nvvideoconvert", "conv");
    GstElement *nvosd = gst_element_factory_make("nvdsosd", "osd");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    if (!pipeline || !source || !streammux || !pgie || !sgie || !nvvidconv || !nvosd || !sink) {
        return -1;
    }

    g_object_set(G_OBJECT(streammux), "batch-size", 2, "width", 1920, "height", 1080, NULL);
    g_object_set(G_OBJECT(pgie), "config-file-path", "config/config_infer_primary_yolo.txt", NULL);
    g_object_set(G_OBJECT(sgie), "config-file-path", "config/config_infer_secondary_ymmv.txt", NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, streammux, pgie, sgie, nvvidconv, nvosd, sink, NULL);

    // Request pad linking for streammux
    GstPad *sink_pad = gst_element_get_request_pad(streammux, "sink_0");
    // rtspsrc requires dynamic linking via "pad-added" signal in production

    gst_element_link_many(streammux, pgie, sgie, nvvidconv, nvosd, sink, NULL);

    GstPad *osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_pad_buffer_probe, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    sqlite3_close(db);
    return 0;
}
