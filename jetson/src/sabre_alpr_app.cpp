#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <sqlite3.h>
#include "gstnvdsmeta.h"
#include "../../shared/sabre_protocol.h"
#include "../../shared/sabre_crypto.h"

static sqlite3 *db;

static GstPadProbeReturn osd_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    NvDsBatchMeta *batch_meta = gst_nvds_get_batch_meta((GstBuffer *)info->data);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == 0) { // Plate
                // Generate Signature
                std::vector<uint8_t> mock_image(100, 0); // In production, extract from nvbufsurface
                std::string sig = calculate_sabre_signature(obj_meta->obj_label, "2023-10-27T12:00:00Z", 29.42, -98.49, mock_image);

                // DB Insert
                sqlite3_stmt *stmt;
                const char* sql = "INSERT INTO hits (timestamp_iso8601, plate_text, plate_confidence, sha256_hash, hub_uuid) VALUES (datetime('now'), ?, ?, ?, 'HUB-001');";
                sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                sqlite3_bind_text(stmt, 1, obj_meta->obj_label, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 2, obj_meta->confidence);
                sqlite3_bind_text(stmt, 3, sig.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
    }
    return GST_PAD_PROBE_OK;
}

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);
    sqlite3_open("/mnt/sabre_storage/sabre_hub.db", &db);

    GstElement *pipeline = gst_pipeline_new("sabre-alpr");
    GstElement *src = gst_element_factory_make("rtspsrc", "src");
    GstElement *mux = gst_element_factory_make("nvstreammux", "mux");
    GstElement *gie = gst_element_factory_make("nvinfer", "gie");
    GstElement *osd = gst_element_factory_make("nvdsosd", "osd");
    GstElement *sink = gst_element_factory_make("fakesink", "sink");

    gst_bin_add_many(GST_BIN(pipeline), src, mux, gie, osd, sink, NULL);
    // Elements linking logic here...

    GstPad *osd_pad = gst_element_get_static_pad(osd, "sink");
    gst_pad_add_probe(osd_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_probe, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(g_main_loop_new(NULL, FALSE));
    return 0;
}
