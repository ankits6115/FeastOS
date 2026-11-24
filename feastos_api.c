/* FeastOS local C API: PostgreSQL-backed restaurant/menu foundation.
 * Development-only: binds to loopback, has no authentication, and handles one
 * request at a time. Do not expose this server to a network or production.
 route_request
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <wincrypt.h>
typedef SOCKET socket_t;
#define close_socket closesocket
#define str_case_cmp _stricmp
#define str_n_case_cmp _strnicmp
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define close_socket close
#define str_case_cmp strcasecmp
#define str_n_case_cmp strncasecmp
#endif
#include "feastos_os_stats_bridge.h"
#include "cli_priority.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#define API_PORT 8787
#define REQUEST_CAP 65536
#define HEADER_CAP 8192

static PGconn *g_db = NULL;

/* Keep PostgreSQL result checks consistent across API routes. */
static int db_result_is(PGresult *result, ExecStatusType expected) {
    return result != NULL && PQresultStatus(result) == expected;
}

#define FEASTOS_PI 3.14
static double haversine_km(double lat1, double lon1, double lat2, double lon2) {
    double rlat1 = lat1 * FEASTOS_PI / 180.0;
    double rlat2 = lat2 * FEASTOS_PI / 180.0;
    double dlat = (lat2 - lat1) * FEASTOS_PI / 180.0;
    double dlon = (lon2 - lon1) * FEASTOS_PI / 180.0;
    double a = sin(dlat / 2.0) * sin(dlat / 2.0) +
               cos(rlat1) * cos(rlat2) * sin(dlon / 2.0) * sin(dlon / 2.0);
    if (a < 0.0) a = 0.0;
    if (a > 1.0) a = 1.0;
    return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}


static int send_all(socket_t client, const char *data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(client, data + sent, (int)(len - sent), 0);
#else
        ssize_t n = send(client, data + sent, len - sent, 0);
#endif
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static void send_response(socket_t client, int status, const char *reason,
                          const char *content_type, const char *body) {
    char header[1024];
    size_t body_len = strlen(body);
    int n = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s; charset=utf-8\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n\r\n",
        status, reason, content_type, body_len);
    if (n > 0 && (size_t)n < sizeof(header)) {
        send_all(client, header, (size_t)n);
        send_all(client, body, body_len);
    }
}

static void send_json(socket_t c, int status, const char *reason, const char *body) {
    send_response(c, status, reason, "application/json", body);
}

static int json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    if (out_size > 0) out[0] = '\0';
    char needle[128];
    if (snprintf(needle, sizeof(needle), "\"%s\"", key) >= (int)sizeof(needle)) return 0;
    const char *p = strstr(json, needle);
    if (!p) return 0;
    p += strlen(needle);
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p++ != ':') return 0;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p++ != '"') return 0;
    size_t used = 0;
    while (*p && *p != '"') {
        char ch = *p++;
        if (ch == '\\' && *p) {
            char esc = *p++;
            switch (esc) {
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;
                case '"': ch = '"'; break;
                case '\\': ch = '\\'; break;
                default: ch = esc; break;
            }
        }
        if (used + 1 < out_size) out[used++] = ch;
    }
    if (*p != '"') return 0;
    out[used] = '\0';
    return 1;
}

static int json_get_number_text(const char *json, const char *key, char *out, size_t out_size,
                                const char *fallback) {
    char needle[128];
    if (snprintf(needle, sizeof(needle), "\"%s\"", key) >= (int)sizeof(needle)) return 0;
    const char *p = strstr(json, needle);
    if (!p) { snprintf(out, out_size, "%s", fallback); return 0; }
    p += strlen(needle);
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p++ != ':') { snprintf(out, out_size, "%s", fallback); return 0; }
    while (*p && isspace((unsigned char)*p)) p++;
    size_t n = 0;
    if (*p == '-') out[n++] = *p++;
    while (*p && (isdigit((unsigned char)*p) || *p == '.')) {
        if (n + 1 < out_size) out[n++] = *p;
        p++;
    }
    if (n == 0) { snprintf(out, out_size, "%s", fallback); return 0; }
    out[n] = '\0';
    return 1;
}

static int json_get_bool(const char *json, const char *key, const char *fallback) {
    char needle[128];
    if (snprintf(needle, sizeof(needle), "\"%s\"", key) >= (int)sizeof(needle)) return strcmp(fallback, "true") == 0;
    const char *p = strstr(json, needle);
    if (!p) return strcmp(fallback, "true") == 0;
    p += strlen(needle);
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p++ != ':') return strcmp(fallback, "true") == 0;
    while (*p && isspace((unsigned char)*p)) p++;
    return strncmp(p, "true", 4) == 0;
}

static int db_ok(void) {
    if (!g_db || PQstatus(g_db) != CONNECTION_OK) return 0;
    PGresult *r = PQexec(g_db, "SELECT 1");
    int ok = r && PQresultStatus(r) == PGRES_TUPLES_OK;
    if (r) PQclear(r);
    return ok;
}

typedef struct {
    long long item_id;
    int quantity;
} DemoCartItem;

static int parse_demo_items(const char *body, DemoCartItem *items, size_t cap, size_t *count) {
    *count = 0;
    const char *p = strstr(body, "\"items\"");
    if (!p || !(p = strchr(p, '['))) return 0;
    p++;
    while (*p && *p != ']') {
        if (*p == ',') { p++; continue; }
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (*p != '{' || *count >= cap) return 0;
        const char *start = p;
        const char *end = strchr(start, '}');
        if (!end) return 0;
        size_t len = (size_t)(end - start + 1);
        if (len >= 2048) return 0;
        char object[2048];
        memcpy(object, start, len); object[len] = '\0';
        char id_text[64], qty_text[32];
        if (!json_get_number_text(object, "item_id", id_text, sizeof(id_text), "0") ||
            !json_get_number_text(object, "quantity", qty_text, sizeof(qty_text), "0")) return 0;
        char *id_end = NULL, *qty_end = NULL;
        long long id = strtoll(id_text, &id_end, 10);
        long qty = strtol(qty_text, &qty_end, 10);
        if (!id_end || *id_end || !qty_end || *qty_end || id <= 0 || qty <= 0 || qty > 99) return 0;
        items[*count].item_id = id;
        items[*count].quantity = (int)qty;
        (*count)++;
        p = end + 1;
    }
    return *count > 0 && *p == ']';
}

static void db_rollback(void) {
    PGresult *r = PQexec(g_db, "ROLLBACK");
    if (r) PQclear(r);
}

static void finalize_due_deliveries(void) {
    PGresult *r = PQexec(g_db, "BEGIN");
    if (!db_result_is(r, PGRES_COMMAND_OK)) {
        if (r) PQclear(r);
        return;
    }
    PQclear(r);

    r = PQexec(g_db,
        "UPDATE order_agent_assignments "
        "SET status='delivered', completed_at=GREATEST(NOW(), assigned_at) "
        "WHERE status IN ('assigned','picked_up') "
        "AND estimated_completion_at IS NOT NULL "
        "AND estimated_completion_at <= NOW()");

    if (!db_result_is(r, PGRES_COMMAND_OK)) {
        if (r) PQclear(r);
        db_rollback();
        return;
    }
    PQclear(r);

    r = PQexec(g_db,
        "UPDATE delivery_agents d SET status='available' "
        "WHERE d.status='on_delivery' "
        "AND NOT EXISTS ("
        "SELECT 1 FROM order_agent_assignments a "
        "WHERE a.agent_id=d.agent_id "
        "AND a.status IN ('assigned','picked_up'))");

    if (!db_result_is(r, PGRES_COMMAND_OK)) {
        if (r) PQclear(r);
        db_rollback();
        return;
    }
    PQclear(r);


    /*
     * Retry waiting orders after deliveries free up drivers.
     * Oldest eligible order first; orders with any assignment history
     * are excluded so delivered/cancelled orders are never reassigned.
     */
    for (int attempt = 0; attempt < 100; ++attempt) {
        r = PQexec(g_db,
            "SELECT o.order_id::text "
            "FROM orders o "
            "WHERE o.status IN ('placed','confirmed') AND (o.payment_status='paid' OR o.payment_method='COD') "
            "AND NOT EXISTS ("
            "  SELECT 1 FROM order_agent_assignments a "
            "  WHERE a.order_id=o.order_id"
            ") "
            "ORDER BY LEAST(100, "
            "  COALESCE((SELECT MAX(mi.perishability_score) "
            "    FROM order_items oi JOIN menu_items mi ON mi.item_id=oi.menu_item_id "
            "    WHERE oi.order_id=o.order_id),35) + "
            "  LEAST(25, GREATEST(2, (COALESCE((SELECT MAX(mi.preparation_time_min) "
            "    FROM order_items oi JOIN menu_items mi ON mi.item_id=oi.menu_item_id "
            "    WHERE oi.order_id=o.order_id), "
            "    (SELECT eta_min FROM restaurants WHERE restaurant_id=o.restaurant_id),30)*25)/35)) + "
            "  LEAST(20, GREATEST(1, (COALESCE(o.distance_km,1.0)*1.8)::integer))"
            ") DESC, o.created_at ASC, o.order_id ASC "
            "LIMIT 1 FOR UPDATE OF o SKIP LOCKED");

        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            db_rollback();
            return;
        }

        if (PQntuples(r) == 0) {
            PQclear(r);
            break;
        }

        char waiting_order_id[32];
        snprintf(waiting_order_id, sizeof(waiting_order_id), "%s",
                 PQgetvalue(r, 0, 0));
        PQclear(r);

        r = PQexec(g_db,
            "SELECT agent_id::text FROM delivery_agents "
            "WHERE status='available' "
            "ORDER BY agent_id "
            "FOR UPDATE SKIP LOCKED LIMIT 1");

        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            db_rollback();
            return;
        }

        if (PQntuples(r) == 0) {
            PQclear(r);
            break;
        }

        char waiting_agent_id[32];
        snprintf(waiting_agent_id, sizeof(waiting_agent_id), "%s",
                 PQgetvalue(r, 0, 0));
        PQclear(r);

        const char *agent_values[] = { waiting_agent_id };
        r = PQexecParams(g_db,
            "UPDATE delivery_agents SET status='on_delivery' "
            "WHERE agent_id=$1::bigint AND status='available'",
            1, NULL, agent_values, NULL, NULL, 0);

        if (!db_result_is(r, PGRES_COMMAND_OK) ||
            strcmp(PQcmdTuples(r), "1") != 0) {
            if (r) PQclear(r);
            db_rollback();
            return;
        }
        PQclear(r);

        const char *assignment_values[] = {
            waiting_order_id, waiting_agent_id
        };
        r = PQexecParams(g_db,
            "INSERT INTO order_agent_assignments "
            "(order_id,agent_id,status,estimated_completion_at) "
            "VALUES ($1::bigint,$2::bigint,'assigned', "
            "NOW() + make_interval(mins => "
            "COALESCE((SELECT eta_min FROM restaurants "
            "WHERE restaurant_id=(SELECT restaurant_id FROM orders "
            "WHERE order_id=$1::bigint)),30) + "
            "CEIL(COALESCE((SELECT distance_km FROM orders "
            "WHERE order_id=$1::bigint),1.0))::integer))",
            2, NULL, assignment_values, NULL, NULL, 0);

        if (!db_result_is(r, PGRES_COMMAND_OK)) {
            if (r) PQclear(r);
            db_rollback();
            return;
        }
        PQclear(r);
    }

    r = PQexec(g_db, "COMMIT");
    if (r) {
        if (PQresultStatus(r) != PGRES_COMMAND_OK) db_rollback();
        PQclear(r);
    }
}

#ifdef _WIN32
static int razorpay_create_order(const char *key_id, const char *key_secret,
                                long long amount, const char *receipt,
                                char *razorpay_order_id, size_t id_cap) {
    wchar_t w_host[] = L"api.razorpay.com", w_path[] = L"/v1/orders";
    HINTERNET session = WinHttpOpen(L"FeastOS/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return 0;
    HINTERNET connect = WinHttpConnect(session, w_host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    HINTERNET request = connect ? WinHttpOpenRequest(connect, L"POST", w_path, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
    if (!request) { if(connect)WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return 0; }
    wchar_t headers[] = L"Content-Type: application/json\r\n";
    /* Use an explicit Basic header; credentials are never placed in the URL. */
    char raw[1024], encoded[2048]; DWORD encoded_len = sizeof(encoded);
    snprintf(raw, sizeof(raw), "%s:%s", key_id, key_secret);
    if (!CryptBinaryToStringA((BYTE*)raw, (DWORD)strlen(raw), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
                              encoded, &encoded_len)) { WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session); return 0; }
    wchar_t auth[2300];
    swprintf(auth, 2300, L"Authorization: Basic %S\r\n", encoded);
    WinHttpAddRequestHeaders(request, headers, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    WinHttpAddRequestHeaders(request, auth, (DWORD)-1, WINHTTP_ADDREQ_FLAG_ADD);
    char payload[512];
    snprintf(payload, sizeof(payload), "{\"amount\":%lld,\"currency\":\"INR\",\"receipt\":\"%s\",\"notes\":{\"source\":\"FeastOS\"}}", amount, receipt);
    int ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, payload,
                                (DWORD)strlen(payload), (DWORD)strlen(payload), 0) && WinHttpReceiveResponse(request, NULL);
    char response[8192] = {0}; DWORD got=0, used=0;
    if (ok) while (used < sizeof(response)-1 && WinHttpReadData(request, response+used, (DWORD)(sizeof(response)-1-used), &got) && got) used += got;
    if (ok) ok = json_get_string(response, "id", razorpay_order_id, id_cap) && strncmp(razorpay_order_id,"order_",6)==0;
    WinHttpCloseHandle(request); WinHttpCloseHandle(connect); WinHttpCloseHandle(session);
    SecureZeroMemory(raw,sizeof(raw)); SecureZeroMemory(encoded,sizeof(encoded));
    return ok;
}

static int razorpay_verify_signature(const char *key_secret, const char *order_id,
                                     const char *payment_id, const char *signature) {
    char message[256], expected[65]; unsigned char hash[32];
    snprintf(message,sizeof(message),"%s|%s",order_id,payment_id);
    BCRYPT_ALG_HANDLE alg=NULL; BCRYPT_HASH_HANDLE h=NULL; DWORD objlen=0, cb=0;
    if (BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,BCRYPT_ALG_HANDLE_HMAC_FLAG)<0) return 0;
    if (BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,(PUCHAR)&objlen,sizeof(objlen),&cb,0)<0) { BCryptCloseAlgorithmProvider(alg,0); return 0; }
    PUCHAR obj=(PUCHAR)HeapAlloc(GetProcessHeap(),0,objlen);
    if (!obj || BCryptCreateHash(alg,&h,obj,objlen,(PUCHAR)key_secret,(ULONG)strlen(key_secret),0)<0 ||
        BCryptHashData(h,(PUCHAR)message,(ULONG)strlen(message),0)<0 || BCryptFinishHash(h,hash,sizeof(hash),0)<0) {
        if(h)BCryptDestroyHash(h); if(obj)HeapFree(GetProcessHeap(),0,obj); BCryptCloseAlgorithmProvider(alg,0); return 0;
    }
    for(int i=0;i<32;i++) snprintf(expected+i*2,3,"%02x",hash[i]); expected[64]=0;
    int valid = strlen(signature)==64;
    unsigned char diff=0; for(int i=0;i<64 && valid;i++) diff |= (unsigned char)(tolower((unsigned char)expected[i]) ^ tolower((unsigned char)signature[i]));
    BCryptDestroyHash(h); HeapFree(GetProcessHeap(),0,obj); BCryptCloseAlgorithmProvider(alg,0); SecureZeroMemory(hash,sizeof(hash));
    return valid && diff==0;
}
#endif

static void route_razorpay_verify(socket_t client, const char *body) {
#ifndef _WIN32
    send_json(client, 501, "Not Implemented", "{\"error\":\"Razorpay verification is supported on Windows builds only\"}");
#else
    const char *secret=getenv("RAZORPAY_KEY_SECRET");
    char order_id[128], payment_id[128], signature[128], db_order[128];
    if(!secret || !*secret || !json_get_string(body,"razorpay_order_id",order_id,sizeof(order_id)) ||
       !json_get_string(body,"razorpay_payment_id",payment_id,sizeof(payment_id)) ||
       !json_get_string(body,"razorpay_signature",signature,sizeof(signature))) {
        send_json(client,400,"Bad Request","{\"error\":\"Missing payment verification fields or server credentials\"}"); return;
    }
    const char *vals[]={order_id};
    PGresult *r=PQexecParams(g_db,"SELECT order_id,total_paise FROM orders WHERE razorpay_order_id=$1 AND payment_status='pending'",1,NULL,vals,NULL,NULL,0);
    if(!db_result_is(r,PGRES_TUPLES_OK)||PQntuples(r)!=1){if(r)PQclear(r);send_json(client,409,"Conflict","{\"error\":\"Order is not awaiting payment\"}");return;}
    snprintf(db_order,sizeof(db_order),"%s",PQgetvalue(r,0,0)); PQclear(r);
    if(!razorpay_verify_signature(secret,order_id,payment_id,signature)){send_json(client,400,"Bad Request","{\"error\":\"Payment signature verification failed\"}");return;}
    const char *up[]={payment_id,db_order};
    r=PQexec(g_db,"BEGIN"); if(!db_result_is(r,PGRES_COMMAND_OK)){if(r)PQclear(r);send_json(client,500,"Internal Server Error","{\"error\":\"Could not begin payment verification transaction\"}");return;} PQclear(r);
    r=PQexecParams(g_db,"UPDATE orders SET payment_status='paid',status='placed' WHERE order_id=$2::bigint AND payment_status='pending' AND razorpay_order_id=$1",2,NULL,up,NULL,NULL,0);
    if(!db_result_is(r,PGRES_COMMAND_OK)||atoi(PQcmdTuples(r))!=1){if(r)PQclear(r);PQexec(g_db,"ROLLBACK");send_json(client,409,"Conflict","{\"error\":\"Order payment state changed; refresh order history\"}");return;} PQclear(r);
    r=PQexecParams(g_db,"UPDATE payment_transactions SET transaction_ref=$1,status='captured',note='Razorpay payment signature verified' WHERE order_id=$2::bigint AND status='created'",2,NULL,up,NULL,NULL,0);
    if(!db_result_is(r,PGRES_COMMAND_OK)||atoi(PQcmdTuples(r))!=1){if(r)PQclear(r);PQexec(g_db,"ROLLBACK");send_json(client,500,"Internal Server Error","{\"error\":\"Could not update payment transaction\"}");return;} PQclear(r); r=PQexec(g_db,"COMMIT"); if(!db_result_is(r,PGRES_COMMAND_OK)){if(r)PQclear(r);PQexec(g_db,"ROLLBACK");send_json(client,500,"Internal Server Error","{\"error\":\"Could not commit verified payment\"}");return;} PQclear(r);
    finalize_due_deliveries(); send_json(client,200,"OK","{\"ok\":true,\"payment_status\":\"paid\"}");
#endif
}

static void json_escape(const char *src, char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    size_t j = 0;
    for (size_t i = 0; src && src[i] && j + 2 < dst_size; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            dst[j++] = '\\';
            dst[j++] = src[i];
        } else if (src[i] == '\n') {
            dst[j++] = '\\';
            dst[j++] = 'n';
        } else if (src[i] == '\r') {
            dst[j++] = '\\';
            dst[j++] = 'r';
        } else if (src[i] == '\t') {
            dst[j++] = '\\';
            dst[j++] = 't';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j < dst_size ? j : dst_size - 1] = '\0';
}

static void route_auth_check_email(socket_t client, const char *body) {
    char email[256];
    if (!json_get_string(body, "email", email, sizeof(email)) || !*email) {
        send_json(client, 400, "Bad Request", "{\"ok\":false,\"error\":\"Email is required\"}");
        return;
    }
    const char *vals[] = { email };
    PGresult *r = PQexecParams(g_db,
        "SELECT user_id, full_name, email FROM users WHERE lower(email) = lower($1)",
        1, NULL, vals, NULL, NULL, 0);
    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);
        send_json(client, 500, "Internal Server Error", "{\"ok\":false,\"error\":\"Database query failed\"}");
        return;
    }
    if (PQntuples(r) > 0) {
        char name[256];
        json_escape(PQgetvalue(r, 0, 1), name, sizeof(name));
        char resp[512];
        snprintf(resp, sizeof(resp), "{\"ok\":true,\"exists\":true,\"name\":\"%s\"}", name);
        send_json(client, 200, "OK", resp);
    } else {
        send_json(client, 200, "OK", "{\"ok\":true,\"exists\":false}");
    }
    PQclear(r);
}

static void route_auth_login(socket_t client, const char *body) {
    char email[256], password[256];
    if (!json_get_string(body, "email", email, sizeof(email)) || !*email ||
        !json_get_string(body, "password", password, sizeof(password)) || !*password) {
        send_json(client, 400, "Bad Request", "{\"ok\":false,\"error\":\"Email and password are required\"}");
        return;
    }
    const char *vals[] = { email, password };
    PGresult *r = PQexecParams(g_db,
        "SELECT user_id::text, email, full_name FROM users "
        "WHERE lower(email) = lower($1) "
        "  AND password_hash = encode(sha256(($2 || lower($1))::bytea), 'hex')",
        2, NULL, vals, NULL, NULL, 0);
    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);
        send_json(client, 500, "Internal Server Error", "{\"ok\":false,\"error\":\"Database query failed\"}");
        return;
    }
    if (PQntuples(r) == 0) {
        PQclear(r);
        send_json(client, 401, "Unauthorized", "{\"ok\":false,\"error\":\"Invalid email or password\"}");
        return;
    }
    char uid[32], em[256], nm[256];
    json_escape(PQgetvalue(r, 0, 0), uid, sizeof(uid));
    json_escape(PQgetvalue(r, 0, 1), em, sizeof(em));
    json_escape(PQgetvalue(r, 0, 2), nm, sizeof(nm));
    char resp[1024];
    snprintf(resp, sizeof(resp),
        "{\"ok\":true,\"user\":{\"user_id\":%s,\"email\":\"%s\",\"name\":\"%s\"}}",
        uid, em, nm);
    PQclear(r);
    send_json(client, 200, "OK", resp);
}

static void route_auth_signup(socket_t client, const char *body) {
    char email[256], password[256], name[256];
    if (!json_get_string(body, "email", email, sizeof(email)) || !*email ||
        !json_get_string(body, "password", password, sizeof(password)) || !*password) {
        send_json(client, 400, "Bad Request", "{\"ok\":false,\"error\":\"Email and password are required\"}");
        return;
    }
    if (strlen(password) < 4) {
        send_json(client, 400, "Bad Request", "{\"ok\":false,\"error\":\"Password must be at least 4 characters\"}");
        return;
    }
    if (!json_get_string(body, "name", name, sizeof(name)) || !*name) {
        snprintf(name, sizeof(name), "%s", email);
        char *at = strchr(name, '@');
        if (at) *at = '\0';
    }
    const char *chk_vals[] = { email };
    PGresult *chk = PQexecParams(g_db,
        "SELECT user_id FROM users WHERE lower(email) = lower($1)",
        1, NULL, chk_vals, NULL, NULL, 0);
    if (!db_result_is(chk, PGRES_TUPLES_OK)) {
        if (chk) PQclear(chk);
        send_json(client, 500, "Internal Server Error", "{\"ok\":false,\"error\":\"Database query failed\"}");
        return;
    }
    if (PQntuples(chk) > 0) {
        PQclear(chk);
        send_json(client, 409, "Conflict", "{\"ok\":false,\"error\":\"An account with this email already exists. Please log in.\"}");
        return;
    }
    PQclear(chk);

    const char *vals[] = { email, name, password };
    PGresult *r = PQexecParams(g_db,
        "INSERT INTO users(email, full_name, password_hash) "
        "VALUES(lower($1), $2, encode(sha256(($3 || lower($1))::bytea), 'hex')) "
        "RETURNING user_id::text, email, full_name",
        3, NULL, vals, NULL, NULL, 0);
    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);
        send_json(client, 500, "Internal Server Error", "{\"ok\":false,\"error\":\"Could not create user account\"}");
        return;
    }
    char uid[32], em[256], nm[256];
    json_escape(PQgetvalue(r, 0, 0), uid, sizeof(uid));
    json_escape(PQgetvalue(r, 0, 1), em, sizeof(em));
    json_escape(PQgetvalue(r, 0, 2), nm, sizeof(nm));
    char resp[1024];
    snprintf(resp, sizeof(resp),
        "{\"ok\":true,\"user\":{\"user_id\":%s,\"email\":\"%s\",\"name\":\"%s\"}}",
        uid, em, nm);
    PQclear(r);
    send_json(client, 201, "Created", resp);
}

static void route_auth_email(socket_t client, const char *body) {
    char action[64] = {0};
    json_get_string(body, "action", action, sizeof(action));
    if (strcmp(action, "check") == 0) {
        route_auth_check_email(client, body);
    } else if (strcmp(action, "login") == 0) {
        route_auth_login(client, body);
    } else if (strcmp(action, "signup") == 0) {
        route_auth_signup(client, body);
    } else {
        char email[256], password[256];
        if (!json_get_string(body, "email", email, sizeof(email)) || !*email) {
            send_json(client, 400, "Bad Request", "{\"ok\":false,\"error\":\"Email is required\"}");
            return;
        }
        if (!json_get_string(body, "password", password, sizeof(password)) || !*password) {
            route_auth_check_email(client, body);
            return;
        }
        const char *chk_vals[] = { email };
        PGresult *chk = PQexecParams(g_db,
            "SELECT user_id FROM users WHERE lower(email) = lower($1)",
            1, NULL, chk_vals, NULL, NULL, 0);
        if (!db_result_is(chk, PGRES_TUPLES_OK)) {
            if (chk) PQclear(chk);
            send_json(client, 500, "Internal Server Error", "{\"ok\":false,\"error\":\"Database query failed\"}");
            return;
        }
        int exists = PQntuples(chk) > 0;
        PQclear(chk);
        if (exists) {
            route_auth_login(client, body);
        } else {
            route_auth_signup(client, body);
        }
    }
}

static void route_demo_order(socket_t client, const char *body, int cash_on_delivery) {
    DemoCartItem items[100];
    size_t item_count = 0;
    if (!parse_demo_items(body, items, 100, &item_count)) {
        send_json(client, 400, "Bad Request", "{\"error\":\"Provide a non-empty items array with numeric item_id and quantity values\"}");
        return;
    }
    char delivery_lat_text[64], delivery_lng_text[64], delivery_label[512];
    if (!json_get_number_text(body, "latitude", delivery_lat_text, sizeof(delivery_lat_text), "") ||
        !json_get_number_text(body, "longitude", delivery_lng_text, sizeof(delivery_lng_text), "") ||
        !json_get_string(body, "label", delivery_label, sizeof(delivery_label))) {
        send_json(client, 400, "Bad Request", "{\"error\":\"delivery_location.latitude, longitude and label are required\"}");
        return;
    }
    char *lat_end = NULL, *lng_end = NULL;
    double delivery_lat = strtod(delivery_lat_text, &lat_end);
    double delivery_lng = strtod(delivery_lng_text, &lng_end);
    if (!lat_end || *lat_end || !lng_end || *lng_end || !isfinite(delivery_lat) || !isfinite(delivery_lng) ||
        delivery_lat < -90.0 || delivery_lat > 90.0 || delivery_lng < -180.0 || delivery_lng > 180.0 || delivery_label[0] == '\0') {
        send_json(client, 400, "Bad Request", "{\"error\":\"Delivery coordinates or label are invalid\"}");
        return;
    }

    PGresult *r = PQexec(g_db, "BEGIN");
    if (!db_result_is(r, PGRES_COMMAND_OK)) {
        if (r) PQclear(r);
        send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not begin demo order transaction\"}");
        return;
    }
    PQclear(r);

    char user_email[256] = {0};
    json_get_string(body, "user_email", user_email, sizeof(user_email));
    const char *payment_method = cash_on_delivery ? "COD" : "RAZORPAY";
    const char *order_values[] = { delivery_label, delivery_lat_text, delivery_lng_text, payment_method, user_email };
    r = PQexecParams(g_db,
        "INSERT INTO orders(status,payment_method,payment_status,subtotal_paise,delivery_fee_paise,platform_fee_paise,total_paise,delivery_address,delivery_latitude,delivery_longitude,user_email) "
        "VALUES('placed',$4,'pending',0,2900,500,0,$1,$2::double precision,$3::double precision,$5) RETURNING order_id",
        5, NULL, order_values, NULL, NULL, 0);
    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);
        db_rollback();
        send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not create payment order; apply the updated schema.sql first\"}");
        return;
    }
    char order_id[32];
    snprintf(order_id, sizeof(order_id), "%s", PQgetvalue(r, 0, 0));
    PQclear(r);

    long long subtotal = 0;
    char order_restaurant_id[32] = "";
    double straight_line_km = 0.0;
    for (size_t i = 0; i < item_count; i++) {
        char item_id_text[32], qty_text[16];
        snprintf(item_id_text, sizeof(item_id_text), "%lld", items[i].item_id);
        snprintf(qty_text, sizeof(qty_text), "%d", items[i].quantity);
        const char *lookup_values[] = { item_id_text };
        r = PQexecParams(g_db,
            "SELECT m.item_id, c.restaurant_id, m.name, m.price_paise, rs.latitude, rs.longitude "
            "FROM menu_items m JOIN menu_categories c ON c.category_id=m.category_id "
            "JOIN restaurants rs ON rs.restaurant_id=c.restaurant_id "
            "WHERE m.item_id=$1::bigint AND m.is_available=true AND rs.status='active'",
            1, NULL, lookup_values, NULL, NULL, 0);
        if (!db_result_is(r, PGRES_TUPLES_OK) || PQntuples(r) != 1) {
            if (r) PQclear(r);
        db_rollback();
            send_json(client, 400, "Bad Request", "{\"error\":\"One or more menu items are unavailable or not in the database\"}");
            return;
        }
        char restaurant_id[32], db_item_id[32], item_name[512], price_text[32];
        snprintf(db_item_id, sizeof(db_item_id), "%s", PQgetvalue(r, 0, 0));
        snprintf(restaurant_id, sizeof(restaurant_id), "%s", PQgetvalue(r, 0, 1));
        snprintf(item_name, sizeof(item_name), "%s", PQgetvalue(r, 0, 2));
        snprintf(price_text, sizeof(price_text), "%s", PQgetvalue(r, 0, 3));
        double restaurant_lat = strtod(PQgetvalue(r, 0, 4), NULL);
        double restaurant_lng = strtod(PQgetvalue(r, 0, 5), NULL);
        long long price = strtoll(price_text, NULL, 10);
        PQclear(r);
        if (order_restaurant_id[0] == '\0') {
            snprintf(order_restaurant_id, sizeof(order_restaurant_id), "%s", restaurant_id);
            straight_line_km = haversine_km(restaurant_lat, restaurant_lng, delivery_lat, delivery_lng);
        } else if (strcmp(order_restaurant_id, restaurant_id) != 0) {
            db_rollback();
            send_json(client, 400, "Bad Request", "{\"error\":\"One demo order can contain items from only one restaurant\"}");
            return;
        }
        if (price < 0 || (items[i].quantity > 0 && price > (LLONG_MAX / items[i].quantity))) {
            PQexec(g_db, "ROLLBACK");
            send_json(client, 400, "Bad Request", "{\"error\":\"Invalid menu price or quantity\"}");
            return;
        }
        long long line_total = price * items[i].quantity;
        if (subtotal > LLONG_MAX - line_total) {
            PQexec(g_db, "ROLLBACK");
            send_json(client, 400, "Bad Request", "{\"error\":\"Order total exceeds supported range\"}");
            return;
        }
        subtotal += line_total;
        char line_text[32];
        snprintf(line_text, sizeof(line_text), "%lld", line_total);
        const char *line_values[] = { order_id, restaurant_id, db_item_id, item_name, price_text, qty_text, line_text };
        r = PQexecParams(g_db,
            "INSERT INTO order_items(order_id,restaurant_id,menu_item_id,item_name_snapshot,unit_price_paise,quantity,line_total_paise) "
            "VALUES($1::bigint,$2::bigint,$3::bigint,$4,$5::bigint,$6::integer,$7::bigint)",
            7, NULL, line_values, NULL, NULL, 0);
        if (!db_result_is(r, PGRES_COMMAND_OK)) {
            if (r) PQclear(r);
        db_rollback();
            send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not save demo order items\"}");
            return;
        }
        PQclear(r);
    }

    const long long delivery_fee = 2900, platform_fee = 500;
    if (subtotal > LLONG_MAX - delivery_fee - platform_fee) {
        PQexec(g_db, "ROLLBACK");
        send_json(client, 400, "Bad Request", "{\"error\":\"Order total exceeds supported range\"}");
        return;
    }
    long long total = subtotal + delivery_fee + platform_fee;
    char subtotal_text[32], total_text[32];
    snprintf(subtotal_text, sizeof(subtotal_text), "%lld", subtotal);
    snprintf(total_text, sizeof(total_text), "%lld", total);
    char distance_text[64];
    snprintf(distance_text, sizeof(distance_text), "%.6f", straight_line_km);
    const char *update_values[] = { subtotal_text, total_text, order_id, order_restaurant_id, distance_text };
    r = PQexecParams(g_db,
        "UPDATE orders SET subtotal_paise=$1::bigint,total_paise=$2::bigint,restaurant_id=$4::bigint,distance_km=$5::double precision WHERE order_id=$3::bigint",
        5, NULL, update_values, NULL, NULL, 0);
    if (!db_result_is(r, PGRES_COMMAND_OK)) {
        if (r) PQclear(r);
        db_rollback();
        send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not calculate demo order total\"}");
        return;
    }
    PQclear(r);

    if (cash_on_delivery) {
        char response[256];
        r = PQexec(g_db, "COMMIT");
        if (!db_result_is(r, PGRES_COMMAND_OK)) {
            if (r) PQclear(r);
            db_rollback();
            send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not commit cash-on-delivery order\"}");
            return;
        }
        PQclear(r);
        finalize_due_deliveries();
        snprintf(response, sizeof(response),
                 "{\"ok\":true,\"order_id\":%s,\"payment_method\":\"COD\",\"payment_status\":\"pending\",\"status\":\"placed\",\"distance_km\":%.3f}",
                 order_id, straight_line_km);
        send_json(client, 201, "Created", response);
        return;
    }

#ifndef _WIN32
    db_rollback(); send_json(client,501,"Not Implemented","{\"error\":\"Razorpay integration requires Windows\"}"); return;
#else
    const char *key_id=getenv("RAZORPAY_KEY_ID"), *key_secret=getenv("RAZORPAY_KEY_SECRET");
    if(!key_id || !*key_id || !key_secret || !*key_secret){db_rollback();send_json(client,503,"Service Unavailable","{\"error\":\"Set RAZORPAY_KEY_ID and RAZORPAY_KEY_SECRET in the backend environment\"}");return;}
    char razorpay_order_id[128]={0};
    if(!razorpay_create_order(key_id,key_secret,total,order_id,razorpay_order_id,sizeof(razorpay_order_id))){db_rollback();send_json(client,502,"Bad Gateway","{\"error\":\"Razorpay order creation failed; no payment was started\"}");return;}
    const char *save_values[]={order_id,razorpay_order_id,total_text};
    r=PQexecParams(g_db,"UPDATE orders SET razorpay_order_id=$2 WHERE order_id=$1::bigint",2,NULL,save_values,NULL,NULL,0);
    if(!db_result_is(r,PGRES_COMMAND_OK)){if(r)PQclear(r);db_rollback();send_json(client,500,"Internal Server Error","{\"error\":\"Could not save Razorpay order reference\"}");return;} PQclear(r);
    r=PQexecParams(g_db,"INSERT INTO payment_transactions(order_id,transaction_ref,transaction_type,provider,status,amount_paise,note) VALUES($1::bigint,$2,'RAZORPAY_PAYMENT','RAZORPAY','created',$3::bigint,'Awaiting Razorpay checkout and server verification')",3,NULL,save_values,NULL,NULL,0);
    if(!db_result_is(r,PGRES_COMMAND_OK)){if(r)PQclear(r);db_rollback();send_json(client,500,"Internal Server Error","{\"error\":\"Could not save pending payment transaction\"}");return;} PQclear(r);
    char response[1024];
    r=PQexec(g_db,"COMMIT"); if(!db_result_is(r,PGRES_COMMAND_OK)){if(r)PQclear(r);db_rollback();send_json(client,500,"Internal Server Error","{\"error\":\"Could not commit pending order\"}");return;} PQclear(r);
    snprintf(response,sizeof(response),"{\"ok\":true,\"order_id\":%s,\"razorpay_order_id\":\"%s\",\"key_id\":\"%s\",\"amount\":%s,\"currency\":\"INR\",\"distance_km\":%.3f}",order_id,razorpay_order_id,key_id,total_text,straight_line_km);
    send_json(client,201,"Created",response);
#endif
}

static void route_order_history(socket_t client) {
    finalize_due_deliveries();
    const char *sql =
        "SELECT COALESCE(json_agg(json_build_object("
        "'order_id', q.order_id, "
        "'status', q.status, "
        "'payment_status', q.payment_status, "
        "'payment_method', q.payment_method, "
        "'user_email', COALESCE(q.user_email, ''), "
        "'subtotal_paise', q.subtotal_paise, "
        "'delivery_fee_paise', q.delivery_fee_paise, "
        "'platform_fee_paise', q.platform_fee_paise, "
        "'total_paise', q.total_paise, "
        "'delivery_address', q.delivery_address, "
        "'distance_km', q.distance_km, "
        "'created_at', q.created_at, "
        "'restaurant_name', COALESCE(q.restaurant_name, 'Restaurant unavailable'), "
        "'assignment_status', (SELECT a.status FROM order_agent_assignments a "
        "WHERE a.order_id=q.order_id ORDER BY a.assigned_at DESC LIMIT 1), "
        "'agent_name', (SELECT d.full_name FROM order_agent_assignments a "
        "JOIN delivery_agents d ON d.agent_id=a.agent_id "
        "WHERE a.order_id=q.order_id ORDER BY a.assigned_at DESC LIMIT 1), "
        "'estimated_completion_at', (SELECT a.estimated_completion_at "
        "FROM order_agent_assignments a WHERE a.order_id=q.order_id "
        "ORDER BY a.assigned_at DESC LIMIT 1), "
        "'items', q.items"
        ") ORDER BY q.created_at DESC), '[]'::json)::text "
        "FROM ("
        " SELECT o.order_id, o.status, o.payment_status, o.payment_method, o.user_email, "
        "        o.subtotal_paise, o.delivery_fee_paise, "
        "        o.platform_fee_paise, o.total_paise, "
        "        o.delivery_address, o.distance_km, o.created_at, "
        "        r.name AS restaurant_name, "
        "        COALESCE(("
        "          SELECT json_agg(json_build_object("
        "            'order_item_id', oi.order_item_id, "
        "            'item_name', oi.item_name_snapshot, "
        "            'quantity', oi.quantity, "
        "            'unit_price_paise', oi.unit_price_paise, "
        "            'line_total_paise', oi.line_total_paise"
        "          ) ORDER BY oi.order_item_id)"
        "          FROM order_items oi "
        "          WHERE oi.order_id = o.order_id"
        "        ), '[]'::json) AS items "
        " FROM orders o "
        " LEFT JOIN restaurants r "
        "   ON r.restaurant_id = o.restaurant_id "
        " ORDER BY o.created_at DESC "
        " LIMIT 50"
        ") q";

    PGresult *r = PQexec(g_db, sql);

    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);

        send_json(
            client,
            500,
            "Internal Server Error",
            "{\"error\":\"Could not read order history; check database schema\"}"
        );
        return;
    }

    const char *json = PQgetvalue(r, 0, 0);

    send_json(client, 200, "OK", (json && *json) ? json : "[]");

    PQclear(r);
}

static void route_recommendations(socket_t client, const char *method, const char *body) {
    char email[256] = {0};
    if (strcmp(method, "POST") == 0 && body && *body) {
        json_get_string(body, "email", email, sizeof(email));
    }

    /* 1. Try personalized recommendations if email was provided */
    if (email[0] != '\0') {
        const char *user_sql =
            "SELECT json_build_object("
            "'ok', true, "
            "'source', 'personalized', "
            "'title', 'Recommended Based on Your Orders', "
            "'subtitle', 'Your favourites and dishes from restaurants you love', "
            "'items', COALESCE(( "
            "  SELECT json_agg(json_build_object("
            "    'id', rec.item_id::text, "
            "    'name', rec.name, "
            "    'desc', rec.description, "
            "    'price', rec.price_paise / 100.0, "
            "    'image', rec.image_url, "
            "    'veg', rec.is_veg, "
            "    'restaurantId', rec.restaurant_id, "
            "    'restaurantName', rec.restaurant_name, "
            "    'cuisine', rec.cuisine, "
            "    'rating', rec.rating, "
            "    'time', (rec.eta_min::text || '\xE2\x80\x93' || (rec.eta_min + 10)::text || ' min'), "
            "    'badge', rec.badge, "
            "    'reason', rec.reason"
            "  )) "
            "  FROM ( "
            "    (SELECT "
            "      m.item_id, m.name, m.description, m.price_paise, COALESCE(NULLIF(m.image_url, ''), r.hero_image_url) AS image_url, m.is_veg, "
            "      r.restaurant_id, r.name AS restaurant_name, r.cuisine, r.rating, r.eta_min, "
            "      'Past Favourite' AS badge, "
            "      'Ordered ' || SUM(oi.quantity) || ' time' || (CASE WHEN SUM(oi.quantity) > 1 THEN 's' ELSE '' END) || ' by you' AS reason, "
            "      1 AS priority, SUM(oi.quantity) AS weight "
            "    FROM order_items oi "
            "    JOIN orders o ON o.order_id = oi.order_id "
            "    JOIN menu_items m ON m.item_id = oi.menu_item_id "
            "    JOIN restaurants r ON r.restaurant_id = oi.restaurant_id "
            "    WHERE lower(o.user_email) = lower($1) "
            "      AND m.is_available = true AND r.status = 'active' "
            "    GROUP BY m.item_id, m.name, m.description, m.price_paise, m.image_url, m.is_veg, r.restaurant_id, r.name, r.cuisine, r.rating, r.eta_min, r.hero_image_url "
            "    ORDER BY weight DESC LIMIT 6) "
            "    UNION ALL "
            "    (SELECT "
            "      m.item_id, m.name, m.description, m.price_paise, COALESCE(NULLIF(m.image_url, ''), r.hero_image_url) AS image_url, m.is_veg, "
            "      r.restaurant_id, r.name AS restaurant_name, r.cuisine, r.rating, r.eta_min, "
            "      'You Might Like' AS badge, "
            "      'Top pick from ' || r.name AS reason, "
            "      2 AS priority, COALESCE(pop.cnt, 0) AS weight "
            "    FROM menu_items m "
            "    JOIN menu_categories c ON c.category_id = m.category_id "
            "    JOIN restaurants r ON r.restaurant_id = c.restaurant_id "
            "    LEFT JOIN ( "
            "      SELECT menu_item_id, SUM(quantity) AS cnt FROM order_items GROUP BY menu_item_id "
            "    ) pop ON pop.menu_item_id = m.item_id "
            "    WHERE r.restaurant_id IN ( "
            "      SELECT DISTINCT o.restaurant_id FROM orders o WHERE lower(o.user_email) = lower($1) AND o.restaurant_id IS NOT NULL "
            "    ) "
            "    AND m.item_id NOT IN ( "
            "      SELECT DISTINCT oi.menu_item_id FROM order_items oi JOIN orders o ON o.order_id = oi.order_id WHERE lower(o.user_email) = lower($1) AND oi.menu_item_id IS NOT NULL "
            "    ) "
            "    AND m.is_available = true AND r.status = 'active' "
            "    ORDER BY weight DESC, r.rating DESC LIMIT 4) "
            "  ) rec "
            "), '[]'::json)"
            ")::text";

        const char *vals[] = { email };
        PGresult *r = PQexecParams(g_db, user_sql, 1, NULL, vals, NULL, NULL, 0);
        if (db_result_is(r, PGRES_TUPLES_OK) && PQntuples(r) > 0) {
            const char *res_json = PQgetvalue(r, 0, 0);
            if (res_json && strstr(res_json, "\"items\" : []") == NULL && strstr(res_json, "\"items\":[]") == NULL && strstr(res_json, "\"items\": []") == NULL) {
                send_json(client, 200, "OK", res_json);
                PQclear(r);
                return;
            }
        }
        if (r) PQclear(r);
    }

    /* 2. Global trending recommendations from all previous orders in the system */
    const char *trending_sql =
        "SELECT json_build_object("
        "'ok', true, "
        "'source', 'previous_orders_trending', "
        "'title', 'Trending Based on Previous Orders', "
        "'subtitle', 'Most re-ordered dishes across your neighbourhood', "
        "'items', COALESCE(( "
        "  SELECT json_agg(json_build_object("
        "    'id', rec.item_id::text, "
        "    'name', rec.name, "
        "    'desc', rec.description, "
        "    'price', rec.price_paise / 100.0, "
        "    'image', rec.image_url, "
        "    'veg', rec.is_veg, "
        "    'restaurantId', rec.restaurant_id, "
        "    'restaurantName', rec.restaurant_name, "
        "    'cuisine', rec.cuisine, "
        "    'rating', rec.rating, "
        "    'time', (rec.eta_min::text || '\xE2\x80\x93' || (rec.eta_min + 10)::text || ' min'), "
        "    'badge', rec.badge, "
        "    'reason', rec.reason"
        "  )) "
        "  FROM ( "
        "    SELECT "
        "      m.item_id, m.name, m.description, m.price_paise, COALESCE(NULLIF(m.image_url, ''), r.hero_image_url) AS image_url, m.is_veg, "
        "      r.restaurant_id, r.name AS restaurant_name, r.cuisine, r.rating, r.eta_min, "
        "      'Most Re-ordered' AS badge, "
        "      'Ordered ' || SUM(oi.quantity) || ' times by local foodies' AS reason "
        "    FROM order_items oi "
        "    JOIN orders o ON o.order_id = oi.order_id "
        "    JOIN menu_items m ON m.item_id = oi.menu_item_id "
        "    JOIN restaurants r ON r.restaurant_id = oi.restaurant_id "
        "    WHERE m.is_available = true AND r.status = 'active' "
        "    GROUP BY m.item_id, m.name, m.description, m.price_paise, m.image_url, m.is_veg, r.restaurant_id, r.name, r.cuisine, r.rating, r.eta_min, r.hero_image_url "
        "    ORDER BY SUM(oi.quantity) DESC, r.rating DESC LIMIT 10 "
        "  ) rec "
        "), '[]'::json)"
        ")::text";

    PGresult *tr = PQexec(g_db, trending_sql);
    if (db_result_is(tr, PGRES_TUPLES_OK) && PQntuples(tr) > 0) {
        const char *res_json = PQgetvalue(tr, 0, 0);
        if (res_json && strstr(res_json, "\"items\" : []") == NULL && strstr(res_json, "\"items\":[]") == NULL && strstr(res_json, "\"items\": []") == NULL) {
            send_json(client, 200, "OK", res_json);
            PQclear(tr);
            return;
        }
    }
    if (tr) PQclear(tr);

    /* 3. Fallback: top-rated dishes from active restaurants */
    const char *fallback_sql =
        "SELECT json_build_object("
        "'ok', true, "
        "'source', 'top_rated', "
        "'title', 'Chef Recommendations', "
        "'subtitle', 'Top-rated culinary delights near you', "
        "'items', COALESCE(( "
        "  SELECT json_agg(json_build_object("
        "    'id', rec.item_id::text, "
        "    'name', rec.name, "
        "    'desc', rec.description, "
        "    'price', rec.price_paise / 100.0, "
        "    'image', rec.image_url, "
        "    'veg', rec.is_veg, "
        "    'restaurantId', rec.restaurant_id, "
        "    'restaurantName', rec.restaurant_name, "
        "    'cuisine', rec.cuisine, "
        "    'rating', rec.rating, "
        "    'time', (rec.eta_min::text || '\xE2\x80\x93' || (rec.eta_min + 10)::text || ' min'), "
        "    'badge', rec.badge, "
        "    'reason', rec.reason"
        "  )) "
        "  FROM ( "
        "    SELECT "
        "      m.item_id, m.name, m.description, m.price_paise, COALESCE(NULLIF(m.image_url, ''), r.hero_image_url) AS image_url, m.is_veg, "
        "      r.restaurant_id, r.name AS restaurant_name, r.cuisine, r.rating, r.eta_min, "
        "      'Top Pick' AS badge, "
        "      'Rated ' || r.rating || ' \xE2\x98\x85 \xC2\xB7 ' || r.cuisine AS reason "
        "    FROM menu_items m "
        "    JOIN menu_categories c ON c.category_id = m.category_id "
        "    JOIN restaurants r ON r.restaurant_id = c.restaurant_id "
        "    WHERE m.is_available = true AND r.status = 'active' "
        "    ORDER BY r.rating DESC, m.price_paise ASC LIMIT 10 "
        "  ) rec "
        "), '[]'::json)"
        ")::text";

    PGresult *fr = PQexec(g_db, fallback_sql);
    if (db_result_is(fr, PGRES_TUPLES_OK) && PQntuples(fr) > 0) {
        const char *res_json = PQgetvalue(fr, 0, 0);
        send_json(client, 200, "OK", res_json && *res_json ? res_json : "{\"ok\":true,\"source\":\"none\",\"items\":[]}");
    } else {
        send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not load recommendations\"}");
    }
    if (fr) PQclear(fr);
}

static void route_scheduling_stats(socket_t client) {
    finalize_due_deliveries();
    const char *sql =
        "SELECT recent.order_id, recent.prep_min, "
        "       recent.distance_km, recent.perishability_score, "
        "       FLOOR(EXTRACT(EPOCH FROM "
        "         (recent.created_at - MIN(recent.created_at) OVER ())"
        "       ) / 60)::text AS arrival_min "
        "FROM ("
        " SELECT o.order_id::text AS order_id, "
        "        COALESCE(MAX(mi.preparation_time_min), r.eta_min, 30)::text AS prep_min, "
        "        COALESCE(o.distance_km, 1.0)::text AS distance_km, "
        "        COALESCE(MAX(mi.perishability_score), 35)::text AS perishability_score, "
        "        o.created_at "
        " FROM orders o "
        " LEFT JOIN restaurants r "
        "   ON r.restaurant_id = o.restaurant_id "
        " LEFT JOIN order_items oi "
        "   ON oi.order_id = o.order_id "
        " LEFT JOIN menu_items mi "
        "   ON mi.item_id = oi.menu_item_id "
        " LEFT JOIN menu_categories mc "
        "   ON mc.category_id = mi.category_id "
        " WHERE o.status IN ('placed','confirmed') AND (o.payment_status='paid' OR o.payment_method='COD') "
        " AND NOT EXISTS ("
        "   SELECT 1 FROM order_agent_assignments aa "
        "   WHERE aa.order_id=o.order_id"
        " ) "
        " GROUP BY o.order_id, r.eta_min, "
        "          o.distance_km, o.created_at "
        " ORDER BY o.created_at DESC "
        " LIMIT 100"
        ") recent "
        "ORDER BY recent.created_at ASC";

    PGresult *r = PQexec(g_db, sql);

    if (!db_result_is(r, PGRES_TUPLES_OK)) {
        if (r) PQclear(r);

        send_json(
            client,
            500,
            "Internal Server Error",
            "{\"error\":\"Could not load orders for scheduling; check database schema\"}"
        );
        return;
    }

    int count = PQntuples(r);

    if (count == 0) {
        PQclear(r);

        send_json(
            client,
            200,
            "OK",
            "{\"ok\":true,\"orderCount\":0,\"algorithms\":[],"
            "\"message\":\"No saved orders to schedule yet\"}"
        );
        return;
    }

    if (count > MAX_ORDERS) {
        count = MAX_ORDERS;
    }

    
    Order orders[MAX_ORDERS];
    FeastOSPriorityBreakdown breakdown[MAX_ORDERS];

    memset(orders, 0, sizeof(orders));
    memset(breakdown, 0, sizeof(breakdown));

    for (int i = 0; i < count; ++i) {
        const char *id_text = PQgetvalue(r, i, 0);
        const char *prep_text = PQgetvalue(r, i, 1);
        const char *distance_text = PQgetvalue(r, i, 2);
        const char *perishability_text = PQgetvalue(r, i, 3);
        const char *arrival_text = PQgetvalue(r, i, 4);

        long parsed_id = strtol(id_text, NULL, 10);

        orders[i].id =
            (parsed_id > 0 && parsed_id <= INT_MAX)
            ? (int)parsed_id
            : (i + 1);

        orders[i].prep_time_min = (int)strtol(prep_text, NULL, 10);

        if (orders[i].prep_time_min < 1) {
            orders[i].prep_time_min = 1;
        }

        orders[i].distance_km = (float)strtod(distance_text, NULL);

        if (orders[i].distance_km < 0.0f) {
            orders[i].distance_km = 0.0f;
        }

        orders[i].arrival_time = (int)strtol(arrival_text, NULL, 10);

        if (orders[i].arrival_time < 0) {
            orders[i].arrival_time = 0;
        }

        int perishability_score =
            (int)strtol(perishability_text, NULL, 10);

        if (perishability_score < 0) perishability_score = 0;
        if (perishability_score > 100) perishability_score = 100;

        int prep_points = (orders[i].prep_time_min * 25) / 35;

        if (prep_points > 25) prep_points = 25;
        if (prep_points < 2) prep_points = 2;

        int distance_points =
            (int)(orders[i].distance_km * 1.8f);

        if (distance_points > 20) distance_points = 20;
        if (distance_points < 1) distance_points = 1;

        orders[i].delivery_tier = TIER_STANDARD;
        orders[i].is_express = 0;
        orders[i].food_type = FOOD_HOT_MEAL;

        orders[i].priority_score =
            perishability_score + prep_points + distance_points;

        if (orders[i].priority_score > 100) {
            orders[i].priority_score = 100;
        }

        /* Store the individual priority components for the frontend. */
        breakdown[i].order_id = orders[i].id;
        breakdown[i].perishability_score = perishability_score;
        breakdown[i].preparation_time_min = orders[i].prep_time_min;
        breakdown[i].distance_km = orders[i].distance_km;
        breakdown[i].preparation_points = prep_points;
        breakdown[i].distance_points = distance_points;
        breakdown[i].priority_score = orders[i].priority_score;
    }

    PQclear(r);

    size_t output_size = 131072;
    char *output = (char *)malloc(output_size);

    if (!output) {
        send_json(
            client,
            500,
            "Internal Server Error",
            "{\"error\":\"Not enough memory to calculate scheduling statistics\"}"
        );
        return;
    }

    if (feastos_scheduling_stats_json(
            orders, breakdown, count, output, output_size) != 0) {
        free(output);

        send_json(
            client,
            500,
            "Internal Server Error",
            "{\"error\":\"Could not serialize scheduling statistics\"}"
        );
        return;
    }


    send_json(client, 200, "OK", output);

    free(output);
}

static void route_request(socket_t client, const char *method, const char *path,
                          const char *body) {
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(client, 204, "No Content", "application/json", "");
        return;
    }

    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/health") == 0) {
        if (db_ok()) send_json(client, 200, "OK", "{\"ok\":true,\"database\":\"connected\"}");
        else send_json(client, 503, "Service Unavailable", "{\"ok\":false,\"database\":\"disconnected\"}");
        return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/check-email") == 0) {
        route_auth_check_email(client, body); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/login") == 0) {
        route_auth_login(client, body); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/signup") == 0) {
        route_auth_signup(client, body); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/auth/email") == 0) {
        route_auth_email(client, body); return;
    }
    if ((strcmp(method, "GET") == 0 || strcmp(method, "POST") == 0) &&
        strcmp(path, "/api/recommendations") == 0) {
        route_recommendations(client, method, body);
        return;
    }
    if (strcmp(method, "GET") == 0 &&
    strcmp(path, "/api/orders/history") == 0) {
    route_order_history(client);
    return;
    }

    if (strcmp(method, "GET") == 0 &&
    strcmp(path, "/api/orders/scheduling-stats") == 0) {
    route_scheduling_stats(client);
    return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/orders/razorpay/create") == 0) {
        route_demo_order(client, body, 0); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/orders/cod/create") == 0) {
        route_demo_order(client, body, 1); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/orders/demo") == 0) {
        route_demo_order(client, body, 0); return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/payments/razorpay/verify") == 0) { route_razorpay_verify(client, body); return; }
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/demo-transactions") == 0) {
        PGresult *r = PQexec(g_db,
            "SELECT COALESCE(json_agg(json_build_object('transaction_id',t.transaction_id,'reference',t.transaction_ref,"
            "'order_id',t.order_id,'status',t.status,'amount_paise',t.amount_paise,'currency',t.currency,"
            "'provider',t.provider,'note',t.note,'created_at',t.created_at) ORDER BY t.transaction_id DESC), '[]'::json)::text "
            "FROM (SELECT * FROM payment_transactions ORDER BY transaction_id DESC LIMIT 50) t");
        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            send_json(client, 500, "Internal Server Error", "{\"error\":\"Could not read demo transactions; apply schema.sql first\"}");
            return;
        }
        const char *json = PQgetvalue(r, 0, 0);
        send_json(client, 200, "OK", json && *json ? json : "[]");
        PQclear(r);
        return;
    }
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/restaurants") == 0) {
        const char *sql =
            "SELECT COALESCE(json_agg(json_build_object("
            "'id', r.restaurant_id, 'name', r.name, 'cuisine', r.cuisine,"
            "'rating', r.rating, 'time', (r.eta_min::text || '\xE2\x80\x93' || (r.eta_min + 10)::text || ' min'),"
            "'fee', ('\xE2\x82\xB9' || trim(to_char(r.delivery_fee_paise / 100.0, 'FM999999990.00')) || ' delivery'),"
            "'offer', r.offer, 'tag', r.tag, 'lat', r.latitude, 'lng', r.longitude, 'hero', r.hero_image_url,"
            "'items', COALESCE((SELECT json_agg(json_build_object("
            "'id', m.item_id::text, 'name', m.name, 'desc', m.description,"
            "'price', m.price_paise / 100.0, 'image', COALESCE(NULLIF(m.image_url, ''), r.hero_image_url), 'veg', m.is_veg,'perishabilityScore', m.perishability_score, 'preparationTimeMin', m.preparation_time_min) ORDER BY m.item_id)"
            " FROM menu_items m JOIN menu_categories c ON c.category_id=m.category_id"
            " WHERE c.restaurant_id=r.restaurant_id AND m.is_available=true), '[]'::json))"
            " ORDER BY r.restaurant_id), '[]'::json)::text FROM restaurants r WHERE r.status='active'";
        PGresult *r = PQexec(g_db, sql);
        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            send_json(client, 500, "Internal Server Error", "{\"error\":\"Database query failed\"}");
            return;
        }
        const char *json = PQgetvalue(r, 0, 0);
        send_json(client, 200, "OK", json && *json ? json : "[]");
        PQclear(r);
        return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/restaurants") == 0) {
        char name[256], cuisine[256], address[512], phone[64], offer[256], tag[128];
        char rating[32], eta[32], fee[32], lat[32], lng[32];
        if (!json_get_string(body, "name", name, sizeof(name)) ||
            !json_get_string(body, "cuisine", cuisine, sizeof(cuisine)) ||
            !json_get_string(body, "address", address, sizeof(address))) {
            send_json(client, 400, "Bad Request", "{\"error\":\"name, cuisine and address are required strings\"}");
            return;
        }
        json_get_string(body, "phone", phone, sizeof(phone));
        json_get_string(body, "offer", offer, sizeof(offer));
        json_get_string(body, "tag", tag, sizeof(tag));
        json_get_number_text(body, "rating", rating, sizeof(rating), "0");
        json_get_number_text(body, "eta_min", eta, sizeof(eta), "30");
        json_get_number_text(body, "delivery_fee_paise", fee, sizeof(fee), "0");
        json_get_number_text(body, "lat", lat, sizeof(lat), "12.9716");
        json_get_number_text(body, "lng", lng, sizeof(lng), "77.5946");
        const char *values[] = { name, cuisine, address, phone, rating, eta, fee, offer, tag, lat, lng };
        const char *sql = "INSERT INTO restaurants(name,cuisine,address,phone,rating,eta_min,delivery_fee_paise,offer,tag,latitude,longitude,status) "
                           "VALUES($1,$2,$3,NULLIF($4,''),$5::numeric,$6::integer,$7::bigint,$8,$9,$10::double precision,$11::double precision,'active') "
                           "RETURNING restaurant_id";
        PGresult *r = PQexecParams(g_db, sql, 11, NULL, values, NULL, NULL, 0);
        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            send_json(client, 400, "Bad Request", "{\"error\":\"Could not create restaurant; check fields and database schema\"}");
            return;
        }
        char response[128];
        snprintf(response, sizeof(response), "{\"ok\":true,\"id\":%s}", PQgetvalue(r, 0, 0));
        PQclear(r);
        send_json(client, 201, "Created", response);
        return;
    }
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/menu-items") == 0) {
        char restaurant_id[32], category[128], name[256], description[1024], price[32], image[1024];
        char perishability[16], prep_time[16];
        json_get_number_text(body, "restaurant_id", restaurant_id, sizeof(restaurant_id), "0");
        json_get_number_text(body, "price_paise", price, sizeof(price), "-1");
        json_get_number_text(body, "perishability_score", perishability, sizeof(perishability), "-1");
        json_get_number_text(body, "preparation_time_min", prep_time, sizeof(prep_time), "-1");
        if (!json_get_string(body, "category", category, sizeof(category)) ||
            !json_get_string(body, "name", name, sizeof(name)) ||
            !json_get_string(body, "description", description, sizeof(description)) ||
            !json_get_string(body, "image_url", image, sizeof(image)) ||
            strcmp(restaurant_id, "0") == 0 || strcmp(price, "-1") == 0 ||
            strcmp(perishability, "-1") == 0 || strcmp(prep_time, "-1") == 0) {
            send_json(client, 400, "Bad Request", "{\"error\":\"restaurant_id, category, name, description, image_url, price_paise, perishability_score and preparation_time_min are required\"}");
            return;
        }
        const char *veg = json_get_bool(body, "is_veg", "true") ? "true" : "false";
        const char *values[] = { restaurant_id, category, name, description, price, image, veg, perishability, prep_time };
        const char *sql =
            "WITH cat AS (INSERT INTO menu_categories(restaurant_id,name) VALUES($1::bigint,$2) "
            "ON CONFLICT(restaurant_id,name) DO UPDATE SET name=EXCLUDED.name RETURNING category_id) "
            "INSERT INTO menu_items(category_id,name,description,price_paise,image_url,is_veg,perishability_score,preparation_time_min) "
            "SELECT category_id,$3,$4,$5::bigint,$6,$7::boolean,$8::smallint,$9::smallint FROM cat RETURNING item_id";
        PGresult *r = PQexecParams(g_db, sql, 9, NULL, values, NULL, NULL, 0);
        if (!db_result_is(r, PGRES_TUPLES_OK)) {
            if (r) PQclear(r);
            send_json(client, 400, "Bad Request", "{\"error\":\"Could not create menu item; verify restaurant_id and fields\"}");
            return;
        }
        char response[128];
        snprintf(response, sizeof(response), "{\"ok\":true,\"id\":%s}", PQgetvalue(r, 0, 0));
        PQclear(r);
        send_json(client, 201, "Created", response);
        return;
    }
    send_json(client, 404, "Not Found", "{\"error\":\"Route not found\"}");
}

static void handle_client(socket_t client) {
    char request[REQUEST_CAP + 1];
    size_t used = 0;
    size_t expected_total = 0;
    size_t content_length = 0;
    int headers_parsed = 0;
    memset(request, 0, sizeof(request));
    while (used < REQUEST_CAP) {
#ifdef _WIN32
        int n = recv(client, request + used, (int)(REQUEST_CAP - used), 0);
#else
        ssize_t n = recv(client, request + used, REQUEST_CAP - used, 0);
#endif
        if (n <= 0) break;
        used += (size_t)n;
        request[used] = '\0';
        char *header_end = strstr(request, "\r\n\r\n");
        if (header_end && !headers_parsed) {
            headers_parsed = 1;
            size_t header_len = (size_t)(header_end + 4 - request);
            char *line = strstr(request, "\r\n");
            while (line && line < header_end) {
                line += 2;
                if (line >= header_end) break;
                char *line_end = strstr(line, "\r\n");
                if (!line_end || line_end > header_end) break;
                if ((size_t)(line_end - line) >= 15 && str_n_case_cmp(line, "Content-Length:", 15) == 0) {
                    content_length = (size_t)strtoul(line + 15, NULL, 10);
                }
                line = line_end;
            }
            if (content_length > REQUEST_CAP - HEADER_CAP) {
                send_json(client, 413, "Payload Too Large", "{\"error\":\"Request body too large\"}");
                return;
            }
            expected_total = header_len + content_length;
        }
        if (headers_parsed && used >= expected_total) break;
    }
    if (used == 0) return;
    char method[16] = {0}, path[512] = {0};
    if (sscanf(request, "%15s %511s", method, path) != 2) {
        send_json(client, 400, "Bad Request", "{\"error\":\"Malformed request\"}");
        return;
    }
    char *query = strchr(path, '?');
    if (query) *query = '\0';
    char *body = strstr(request, "\r\n\r\n");
    body = body ? body + 4 : (char *)"";
    route_request(client, method, path, body);
}

int main(void) {
    const char *conninfo = getenv("FEASTOS_DATABASE_URL");
    if (conninfo && *conninfo) {
        g_db = PQconnectdb(conninfo);
    } else {
        const char *pgdb = getenv("PGDATABASE");
        if (!pgdb || !*pgdb) {
            g_db = PQconnectdb("dbname=feastos");
        } else {
            g_db = PQconnectdb(""); /* Uses standard PGHOST/PGPORT/PGDATABASE/PGUSER/PGPASSWORD env vars. */
        }
    }
    if (!g_db || PQstatus(g_db) != CONNECTION_OK) {
        fprintf(stderr, "PostgreSQL connection failed: %s\n", g_db ? PQerrorMessage(g_db) : "out of memory");
        if (g_db) PQfinish(g_db);
        return 1;
    }
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n"); PQfinish(g_db); return 1;
    }
#endif
    socket_t server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        fprintf(stderr, "Could not create server socket\n"); PQfinish(g_db); return 1;
    }
    int yes = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(API_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); /* Local machine only. */
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR || listen(server, 16) == SOCKET_ERROR) {
        fprintf(stderr, "Could not bind to 127.0.0.1:%d (is another API already running?)\n", API_PORT);
        close_socket(server); PQfinish(g_db);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    printf("FeastOS C API listening at http://127.0.0.1:%d\n", API_PORT);
    printf("PostgreSQL connected. Press Ctrl+C to stop.\n");
    for (;;) {
        socket_t client = accept(server, NULL, NULL);
        if (client == INVALID_SOCKET) continue;
        handle_client(client);
        close_socket(client);
    }
    close_socket(server);
    PQfinish(g_db);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

