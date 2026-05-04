#include <iostream>
#include <gst/gst.h>
#include <glib.h>
#include <sqlite3.h>
#include "gstnvdsmeta.h"
#include "../../shared/sabre_protocol.h"

// SQLite handle
static sqlite3 *db;

static GstPadProbeReturn osd_sink_pad_buffer_probe(GstPad *pad, GstPadProbeInfo *info, gpointer u_data) {
    NvDsBatchMeta *batch_meta = gst_nvds_get_batch_meta((GstBuffer *)info->data);
    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next) {
        NvDsFrameMeta *frame_meta = (NvDsFrameMeta *)(l_frame->data);
        for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next) {
            NvDsObjectMeta *obj_meta = (NvDsObjectMeta *)(l_obj->data);
            if (obj_meta->class_id == 0) { // License Plate
                // 1. Insert into Hub DB
                sqlite3_stmt *stmt;
                const char* sql = "INSERT INTO hits (timestamp_iso8601, plate_text, plate_confidence, hub_uuid, sha256_hash) VALUES (datetime('now'), ?, ?, 'HUB-001', 'HASH-PLACEHOLDER');";
                sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
                sqlite3_bind_text(stmt, 1, obj_meta->obj_label, -1, SQLITE_STATIC);
                sqlite3_bind_double(stmt, 2, obj_meta->confidence);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                // 2. Trigger ESP32 context save via UART
                // This would be handled by a dedicated UART utility in production
                printf("ALPR HIT: %s Saved to Database.\n", obj_meta->obj_label);
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
    // Pipeline construction as per previous phases...

    // Setup Probe
    // GstPad *osd_sink_pad = gst_element_get_static_pad(nvosd, "sink");
    // gst_pad_add_probe(osd_sink_pad, GST_PAD_PROBE_TYPE_BUFFER, osd_sink_pad_buffer_probe, NULL, NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    sqlite3_close(db);
    return 0;
}
