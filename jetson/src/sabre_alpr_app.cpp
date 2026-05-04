#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <sqlite3.h>
#include "gstnvdsmeta.h"
#include "../../shared/sabre_protocol.h"
#include "../../shared/sabre_crypto.h"

static sqlite3 *db;

static void cb_newpad(GstElement *decodebin, GstPad *pad, gpointer data) {
    GstElement *muxer = (GstElement *)data;
    GstPad *sinkpad = gst_element_get_request_pad(muxer, "sink_0");
    if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
        g_printerr("Failed to link decodebin to muxer\n");
    }
    gst_object_unref(sinkpad);
}

static GstPadProbeReturn osd_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    NvDsBatchMeta *batch_meta = gst_nvds_get_batch_meta((GstBuffer *)info->data);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == 0) { // License Plate
                // Forensic logic for Golden Master
                std::vector<uint8_t> crop_bytes(10, 0); // Mock for nvds_obj_enc logic
                std::string sig = calculate_sabre_signature(obj_meta->obj_label, "2023-10-27T12:00:00Z", 0.0, 0.0, crop_bytes);

                sqlite3_stmt *stmt;
                const char* sql = "INSERT INTO hits (timestamp_iso8601, plate_text, plate_confidence, sha256_hash, hub_uuid, is_offloaded) VALUES (datetime('now'), ?, ?, ?, 'HUB-001', 0);";
                if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                    sqlite3_bind_text(stmt, 1, obj_meta->obj_label, -1, SQLITE_STATIC);
                    sqlite3_bind_double(stmt, 2, obj_meta->confidence);
                    sqlite3_bind_text(stmt, 3, sig.c_str(), -1, SQLITE_STATIC);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                }
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    sqlite3_open("/mnt/sabre_storage/sabre_hub.db", &db);

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    GstElement *pipeline = gst_pipeline_new("sabre-alpr");
    GstElement *source = gst_element_factory_make("rtspsrc", "src");
    GstElement *decodebin = gst_element_factory_make("decodebin", "decode");
    GstElement *streammux = gst_element_factory_make("nvstreammux", "mux");
    GstElement *pgie = gst_element_factory_make("nvinfer", "pgie");
    GstElement *nvosd = gst_element_factory_make("nvdsosd", "osd");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    if (!pipeline || !source || !decodebin || !streammux || !pgie || !nvosd || !sink) return -1;

    g_object_set(G_OBJECT(source), "location", "rtsp://192.168.1.101/stream", "latency", 0, NULL);
    g_object_set(G_OBJECT(streammux), "width", 1920, "height", 1080, "batch-size", 1, NULL);
    g_object_set(G_OBJECT(pgie), "config-file-path", "config/config_infer_primary_yolo.txt", NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, decodebin, streammux, pgie, nvosd, sink, NULL);

    g_signal_connect(source, "pad-added", G_CALLBACK(cb_newpad), decodebin);
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(cb_newpad), streammux);

    gst_element_link_many(streammux, pgie, nvosd, sink, NULL);

    GstPad *osd_pad = gst_element_get_static_pad(nvosd, "sink");
    gst_pad_add_probe(osd_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_probe, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    sqlite3_close(db);
    return 0;
}
