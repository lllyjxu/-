#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <microhttpd.h>
#include <jansson.h> // JSON parsing
#include <curl/curl.h> // For RPC calls

#define PORT 9527
#define MAX_KEY_LEN 128
#define MAX_VALUE_LEN 1024
#define NODE_COUNT 3

// 数据结构存储缓存
typedef struct {
    char key[MAX_KEY_LEN];
    char value[MAX_VALUE_LEN];
} KeyValue;

KeyValue store[100];
int store_size = 0;
pthread_mutex_t lock;

// 哈希分配策略
int hash_key(const char *key) {
    unsigned long hash = 5381;
    for (const char *ptr = key; *ptr; ++ptr) {
        hash = ((hash << 5) + hash) + *ptr;   // hash = hash * 33 + *ptr
    }
    return hash % NODE_COUNT;
}

// 获取缓存值
char *get_value(const char *key) {
    for (int i = 0; i < store_size; i++) {
        if (strcmp(store[i].key, key) == 0) {
            return store[i].value;
        }
    }
    return NULL;
}

// 设置或更新缓存数据
void set_value(const char *key, const char *value) {
    pthread_mutex_lock(&lock);  // 加锁，保护共享资源
    for (int i = 0; i < store_size; i++) {
        if (strcmp(store[i].key, key) == 0) {
            strncpy(store[i].value, value, MAX_VALUE_LEN);
            pthread_mutex_unlock(&lock);
            return;
        }
    }
    strncpy(store[store_size].key, key, MAX_KEY_LEN);
    strncpy(store[store_size].value, value, MAX_VALUE_LEN);
    store_size++;
    pthread_mutex_unlock(&lock);
}

// 删除缓存数据
int delete_key(const char *key) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < store_size; i++) {
        if (strcmp(store[i].key, key) == 0) {
            store[i] = store[store_size - 1];
            store_size--;
            pthread_mutex_unlock(&lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

// 处理HTTP请求
int http_handler(void *cls, struct MHD_Connection *connection,
                 const char *url, const char *method, const char *version,
                 const char *upload_data, size_t *upload_data_size, void **con_cls) {
    struct MHD_Response *response;
    int ret;

    if (strcmp(method, "GET") == 0) {
        char *value = get_value(url + 1);  // 跳过 '/'
        if (value) {
            json_t *res = json_pack("{s:s}", url + 1, value);
            char *response_str = json_dumps(res, 0);

            response = MHD_create_response_from_buffer(strlen(response_str), (void *)response_str, MHD_RESPMEM_MUST_FREE);
            MHD_add_response_header(response, "Content-Type", "application/json");

            ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            json_decref(res);
        } else {
            response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
            ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
        }
        return ret; // 返回结果
    }

    else if (strcmp(method, "POST") == 0) {
        if (*upload_data_size > 0) {
            json_error_t error;
            json_t *json = json_loads(upload_data, 0, &error);
            if (json) {
                const char *key = json_object_iter_key(json_object_iter(json));
                const char *value = json_string_value(json_object_get(json, key));
                set_value(key, value);
                *upload_data_size = 0;
                json_decref(json);
                response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
                ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
            } else {
                response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
                ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
            }
        }
        return ret;
    }

    else if (strcmp(method, "DELETE") == 0) {
        int result = delete_key(url + 1); // 跳过 '/'
        char response_message[50];
        snprintf(response_message, 50, "%d", result);

        response = MHD_create_response_from_buffer(strlen(response_message), (void *)response_message, MHD_RESPMEM_PERSISTENT);
        ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    }
    return ret;
}

// 启动HTTP服务
int main(int argc, char *argv[]) {
    struct MHD_Daemon *daemon;
    pthread_mutex_init(&lock, NULL);

    // 启动HTTP服务
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &http_handler, NULL, MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }

    printf("Server is running on port %d\n", PORT);

    // 停止HTTP服务
    MHD_stop_daemon(daemon);
    pthread_mutex_destroy(&lock);
    return 0;
}
