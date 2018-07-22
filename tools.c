#include <stdarg.h>

#include <jansson.h>
#define U_DISABLE_WEBSOCKET
#include <ulfius.h>

#include <curl/curl.h>

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

// utility function to get a remote HTTP JPEG image (with curl)
int getRemoteImage(unsigned char *dst, const char *url, long timeout, int bgr) {
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 300);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, timeout);
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

        if (bgr) {
            unsigned char *pix_pointer = data;
            unsigned char *pix_pointer2 = data;

            int i;
            for (i = 0; i < njGetImageSize(); i += 3) {
                int b = *pix_pointer2++;
                *pix_pointer2++;
                int r = *pix_pointer2++;

                *pix_pointer++ = r;
                *pix_pointer++;
                *pix_pointer++ = b;
            }
        }

        memcpy(dst, data, njGetImageSize());
    }

    njDone();

    curl_easy_reset(curl_handle);

    free(chunk.memory);
    chunk.memory = malloc(1);
    chunk.size = 0;

    return 1;
}

// utility function to read from devices
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

// utility function to do a simple GET request (with or without parameters) and X-Api-Key header value
json_t *simpleRESTRequest(char *access_point_url, char *api_key, const char *fmt, ...) {
    static struct _u_map headers;
    static struct _u_map url_params;

    u_map_init(&headers);
    u_map_init(&url_params);

    va_list args;
    va_start(args, fmt);

    if (fmt) {
        while (*fmt != '\0') {
            if (*fmt == 's') {
                const char *sk = va_arg(args, char *);
                const char *sv = va_arg(args, char *);

                u_map_put(&url_params, sk, sv);
            }

            ++fmt;
        }
    }

    u_map_put(&headers, "X-Api-Key", api_key);

    static struct _u_request req;
    static struct _u_response response;

    ulfius_init_request(&req);

    req.http_verb = strdup("GET");
    req.http_url = strdup(access_point_url);
    req.timeout = 10;
    u_map_copy_into(req.map_url, &url_params);
    u_map_copy_into(req.map_header, &headers);

    ulfius_init_response(&response);

    int res = ulfius_send_http_request(&req, &response);

    json_t *json_data = NULL;
    json_error_t json_error;
    
    if (res == U_OK) {
        json_data = ulfius_get_json_body_response(&response, &json_error);
#ifdef DEBUG
        if (!json_data) {
            fprintf(stderr, "ulfius_get_json_body_response failed for %s with error : \n%s\nat line %i\n", access_point_url, json_error.text, json_error.line);
        }
#endif
        //fprintf(stderr, "%s\n", json_dumps(json_data, JSON_INDENT(2)));
    } else {
        fprintf(stderr, "simpleRESTRequest failed for %s with code %d\n", access_point_url, res);
    }

    va_end(args);

    ulfius_clean_response(&response);
    u_map_clean(&headers);
    u_map_clean(&url_params);
    ulfius_clean_request(&req);

    return json_data;
}

// timer utility
struct _my_timer {
    struct timeval start;
    struct timeval curr;
};

struct _my_timer *createTimer() {
    struct _my_timer *t = (struct _my_timer *)calloc(1, sizeof(struct _my_timer));

    gettimeofday(&t->start, NULL);

    return t;
}

void resetTimer(struct _my_timer *t) {
    gettimeofday(&t->start, NULL);
}

double updateTimer(struct _my_timer *t) {
    gettimeofday(&t->curr, NULL);

    return (t->curr.tv_sec - t->start.tv_sec) * 1000000.0 - (t->curr.tv_usec - t->start.tv_usec);
}

void freeTimer(struct _my_timer *t) {
    free(t);
}