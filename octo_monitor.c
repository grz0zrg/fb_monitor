#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>


#include <curl/curl.h>

#include "nanojpeg.c"
#include "fbgraphics.h"

int keep_running = 1;

void int_handler(int dummy) {
    keep_running = 0;
}

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t wmem_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

CURL *curl_handle;
CURLcode res;
struct MemoryStruct chunk;

void initCurl() {
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);

    curl_handle = curl_easy_init();
}

void freeCurl() {
    curl_easy_cleanup(curl_handle);

    free(chunk.memory);

    curl_global_cleanup();
}

int getRemoteImage(unsigned char *dst, const char *url) {
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, wmem_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    njInit();

    res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        return 0;
    }

    if (njDecode(chunk.memory, chunk.size)) {
        fprintf(stdout, "Error decoding the input file.\n");
        return 0;
    } else {
        unsigned char *data = njGetImage();

        memcpy(dst, data, njGetImageSize());
    }

    njDone();

    curl_easy_reset(curl_handle);

    free(chunk.memory);
    chunk.memory = malloc(1);
    chunk.size = 0;

    return 1;
}

int getFileResultAsInt(int f) {
    int result;

    char buffer[2048];

    char *buf = &buffer[0];

    int n;

    lseek(f, 0, SEEK_SET);

    while ((n = read(f, buf, 1))) {
        buf += 1;
    }

    *buf = '\0';

    sscanf(&buffer[0], "%i", &result);

    return result;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, int_handler);

    struct _fbg *fbg = fbg_setup("/dev/fb-st7789s",1);
    if (fbg == NULL) {
        return 0;
    }

    struct _fbg_img *bb_font_img = fbg_loadPNG(fbg, "bbmode1_8x8.png");

    struct _fbg_font *bbfont = fbg_createFont(fbg, bb_font_img, 8, 8, 33);

    initCurl();

    struct _fbg_img *mon_image = fbg_createImage(fbg, 640, 480);

    njInit();

    int cpu_tempf = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);

    char text_buffer[40];

    int get_err = 1;

    struct timeval snap_start;
    struct timeval snap_curr;

    gettimeofday(&snap_start, NULL);

    fbg_textColorKey(fbg, 0);
    fbg_textBackground(fbg, 0, 0, 0, 0);

    do {
        fbg_draw(fbg);

        gettimeofday(&snap_curr, NULL);
        double ms = (snap_curr.tv_sec - snap_start.tv_sec) * 1000000.0 - (snap_curr.tv_usec - snap_start.tv_usec);
        if (ms >= 500.0) {
            gettimeofday(&snap_start, NULL);

            get_err = getRemoteImage(mon_image->data, "http://192.168.0.60:8080/?action=snapshot");

            fflush(stderr);
        }

        fbg_imageScale(fbg, mon_image, 0, 0, 0.5f, 0.5f);
        //

        // draw infos
        int cpu_temp = getFileResultAsInt(cpu_tempf) / 1000;

        snprintf(text_buffer, 40, "%dC", cpu_temp);

        fbg_write(fbg, text_buffer, 320 - strlen(text_buffer) * 8 - 4, 240 - 8);

        // status
        if (!get_err) {
            fbg_text(fbg, bbfont, "@", 4, 2, 0, 0, 255);
        } else {
            fbg_text(fbg, bbfont, "@", 4, 2, 0, 255, 0);
        }

        fbg_flip(fbg);

    } while (keep_running);

    close(cpu_tempf);

    freeCurl();

    fbg_freeImage(mon_image);
    fbg_freeImage(bb_font_img);
    fbg_freeFont(bbfont);

    fbg_close(fbg);

    return 0;
}
