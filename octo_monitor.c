#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "nanojpeg.c"
#include "fbgraphics.h"

#include "tools.c"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

// display any values with X axis alignment
void fbg_writeValue(struct _fbg *fbg, int x, int y, int xalign, int r, int g, int b, char *fmt, ...) {
    static char text_buffer[40]; // 40 is based on 8px glyph width with 320px wide display

    va_list args;
    va_start(args, fmt);
    vsnprintf(text_buffer, sizeof(text_buffer), fmt, args);
    va_end(args);

    if (xalign == 1) {
        fbg_text(fbg, &fbg->current_font, text_buffer, x - strlen(text_buffer) * fbg->current_font.glyph_width, y, r, g, b);
    } else if (xalign == 2) {
        fbg_text(fbg, &fbg->current_font, text_buffer, fbg->width / 2 - (strlen(text_buffer) * fbg->current_font.glyph_width / 2), y, r, g, b);
    } else {
        fbg_text(fbg, &fbg->current_font, text_buffer, x, y, r, g, b);
    }
}

int main(int argc, char* argv[]) {
    int i = 0, j = 0;

    // user-defined setup
    int snapshot_width = 640;
    int snapshot_height = 480;
    double snapshot_poll_ms = 500;
    double temp_graph_update_ms = 1500;

    double octoprint_rest_poll_ms = 1000;

    // prusa MK3
    double printer_tool_max_temp = 320;
    double printer_bed_max_temp = 100;

    char *octoprint_api_key = "56BF85E4C4BF4A198AB9CB3F6E7AD636";
    //

    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_setup("/dev/fb-st7789s",1);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "bbmode1_8x8.png");
    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    // create snapshot buffer
    struct _fbg_img *mon_image = fbg_createImage(fbg, snapshot_width, snapshot_height);

    // create stats buffer
    struct _fbg_img *stats_image = fbg_createImage(fbg, fbg->width, fbg->height);

    initCurl();
    njInit();

    // board temp
    int cpu_tempf = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);

    int get_err = 1;

    struct _my_timer *snap_timer = createTimer();
    struct _my_timer *rest_timer = createTimer();
    struct _my_timer *temp_graph_timer = createTimer();

    fbg_textColorKey(fbg, 0);

    json_t *printer_data = NULL;
    json_t *printer_job_data = NULL;

    // job variable
    double completion_percent = 0;
    double tool_temp = 0, bed_temp = 0, tool_temp_prev = 0, bed_temp_prev = 0;
    int print_time = 0, print_time_left = 0;

    const char *printer_job_data_fail = "Job data error";
    const char *printer_data_fail = "Printer data error";

    do {
        fbg_draw(fbg);

        // get remote snapshot every N ms
        int snap_ms_diff = updateTimer(snap_timer);
        if (snap_ms_diff >= snapshot_poll_ms) {
            resetTimer(snap_timer);

            get_err = getRemoteImage(mon_image->data, "http://192.168.0.60:8080/?action=snapshot");

            fflush(stderr);
        }

        // display snapshot image
        fbg_imageScale(fbg, mon_image, 0, 0, (float)fbg->width / snapshot_width, (float)fbg->height / snapshot_height);

        // poll REST API every N ms & 
        int rest_ms_diff = updateTimer(rest_timer);
        if (rest_ms_diff >= octoprint_rest_poll_ms) {
            resetTimer(rest_timer);

            if (printer_data) {
                json_decref(printer_data);
            }

            if (printer_job_data) {
                json_decref(printer_job_data);
            }

            // get infos from octoprint REST API
            printer_data = simpleRESTRequest("http://192.168.0.60/api/printer", octoprint_api_key, NULL);
            printer_job_data = simpleRESTRequest("http://192.168.0.60/api/job", octoprint_api_key, NULL);
        }

        fbg_textBackground(fbg, 0, 0, 0, 200);

        // display connection status (from snapshot result)
        if (!get_err) {
            fbg_writeValue(fbg, -4, 240 - 16 - 2, 1, 0, 0, 255, "Stream DOWN");
        } else {
            fbg_writeValue(fbg, -4, 240 - 16 - 2, 1, 0, 255, 0, "Stream UP");
        }

        // display stats from printer data
        if (printer_data) {
            fbg_imageColorkey(fbg, stats_image, 0, 0, 0, 0, 0);

            fbg_textBackground(fbg, 0, 0, 0, 224);

            const char *job_file_name = NULL;
            if (printer_job_data) {
                json_unpack(printer_job_data, "{s:{s:{s:s}},s:{s:f,s:i,s:i}}",
                    "job", "file", "name", &job_file_name,
                    "progress", "completion", &completion_percent,
                    "printTime", &print_time,
                    "printTimeLeft", &print_time_left);
            } else {
                job_file_name = printer_job_data_fail;
            }

            const char *state_str = NULL;
            if (printer_data) {
                json_unpack(printer_data, "{s:{s:{s:f},s:{s:f}},s:{s:s}}", 
                    "temperature", "tool0", "actual", &tool_temp,
                    "bed", "actual", &bed_temp,
                    "state", "text", &state_str);
            } else {
                state_str = printer_data_fail;
            }

            fbg_textBackground(fbg, 0, 0, 255, 200);
            fbg_writeValue(fbg, 4, 240 - 8 * 2 - 2, 0, 255, 255, 255, "Tool: %.1fC", tool_temp);
            fbg_textBackground(fbg, 255, 0, 0, 200);
            fbg_writeValue(fbg, 4, 240 - 8 - 2, 0, 255, 255, 255, " Bed: %.1fC", bed_temp);
            
            fbg_textBackground(fbg, 0, 0, 0, 255);
            fbg_writeValue(fbg, 4, 14, 0, 255, 255, 255, "%s", state_str);
            fbg_textBackground(fbg, 0, 0, 0, 224);

            fbg_writeValue(fbg, 8, 14 + 8 + 4, 0, 255, 255, 255, "Job: %s", job_file_name);

            int print_time_seconds = print_time % 60;
            int print_time_minutes = (print_time % 3600) / 60;
            int print_time_hours = (print_time % 86400) / 3600;
            int print_time_days = (print_time % (86400 * 30)) / 86400;
            int print_time_left_seconds = print_time_left % 60;
            int print_time_left_minutes = (print_time_left % 3600) / 60;
            int print_time_left_hours = (print_time_left % 86400) / 3600;
            int print_time_left_days = (print_time_left % (86400 * 30)) / 86400;

            fbg_writeValue(fbg, 8, 14 + 8 + 8 + 4 + 2, 0, 255, 255, 255, "Time: %id:%ih:%im:%is", print_time_days, print_time_hours, print_time_minutes, print_time_seconds);
            fbg_writeValue(fbg, 8, 14 + 8 + 8 + 8 + 4 + 2 + 2, 0, 255, 255, 255, "Left: %id:%ih:%im:%is", print_time_left_days, print_time_left_hours, print_time_left_minutes, print_time_left_seconds);

            fbg_recta(fbg, 0, 0, (double)fbg->width * (completion_percent / 100.0), 12, 0, 255, 0, 224);
            fbg_writeValue(fbg, 0, 2, 2, 255, 255, 255, "%.2f%%", completion_percent);

            fflush(stderr);

            // update stats
            int temp_graph_ms_diff = updateTimer(temp_graph_timer);
            if (temp_graph_ms_diff >= temp_graph_update_ms) {
                resetTimer(temp_graph_timer);

                fbg_drawInto(fbg, stats_image->data);
                // offset image by 1px horizontally
                for (i = 0; i < stats_image->height; i += 1) {
                    for (j = 1; j < stats_image->width; j += 1) {
                        unsigned char *src_pointer = (unsigned char *)(stats_image->data + ((j + i * stats_image->width) * fbg->components));
                        unsigned char *dst_pointer = (unsigned char *)(stats_image->data + (((j - 1) + i * stats_image->width) * fbg->components));

                        *dst_pointer++ = *src_pointer++;
                        *dst_pointer++ = *src_pointer++;
                        *dst_pointer++ = *src_pointer++;
                    }
                }

                // clear last vertical slices
                for (j = 0; j < stats_image->height; j += 1) {
                    unsigned char *dst_pointer = (unsigned char *)(stats_image->data + (((stats_image->width - 1) + j * stats_image->width) * fbg->components));
                    *dst_pointer++ = 0;
                    *dst_pointer++ = 0;
                    *dst_pointer++ = 0;

                    dst_pointer = (unsigned char *)(stats_image->data + (((stats_image->width - 2) + j * stats_image->width) * fbg->components));
                    *dst_pointer++ = 0;
                    *dst_pointer++ = 0;
                    *dst_pointer++ = 0;
                }

                int stats_y_offset = 14 + 8 * 10;
                double tool_temp_y = ((1.0 - tool_temp / printer_tool_max_temp) * (fbg->height - stats_y_offset)) + stats_y_offset - 8 * 3;
                double bed_temp_y = ((1.0 - bed_temp / printer_bed_max_temp) * (fbg->height - stats_y_offset)) + stats_y_offset - 8 * 3;

                fbg_hline(fbg, 0, fbg->height - 8 * 3, fbg->width, 128, 128, 128);
                fbg_hline(fbg, 0, stats_y_offset - 8 * 3, fbg->width, 128, 128, 128);

                //fbg_pixela(fbg, fbg->width - 1, tool_temp_y, 0, 0, 255, 240);
                //fbg_pixela(fbg, fbg->width - 1, bed_temp_y, 255, 0, 0, 240);
                if (tool_temp_prev > 0) {
                    fbg_line(fbg, fbg->width - 2, tool_temp_prev, fbg->width - 1, tool_temp_y, 0, 0, 255);
                    fbg_line(fbg, fbg->width - 2, bed_temp_prev, fbg->width - 1, bed_temp_y, 255, 0, 0);
                }
                fbg_drawInto(fbg, NULL);

                tool_temp_prev = tool_temp_y;
                bed_temp_prev = bed_temp_y;
            }
        } else {
            fbg_writeValue(fbg, 4, 2, 0, 255, 255, 255, "Printer OFF");
        }

        fbg_textBackground(fbg, 0, 0, 0, 200);

        // get infos from devices in realtime & display result
        int cpu_temp = getFileResultAsInt(cpu_tempf) / 1000;
        if (cpu_temp >= 50) {
            fbg_writeValue(fbg, 320 - 4, 240 - 8 - 2, 1, 0, 0, 255, "%iC", cpu_temp);
        } else {
            fbg_writeValue(fbg, 320 - 4, 240 - 8 - 2, 1, 0, 255, 0, "%iC", cpu_temp);
        }

        fbg_flip(fbg);
    } while (keep_running);

    freeTimer(snap_timer);
    freeTimer(rest_timer);
    freeTimer(temp_graph_timer);

    close(cpu_tempf);

    freeCurl();

    fbg_freeImage(stats_image);
    fbg_freeImage(mon_image);
    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}
