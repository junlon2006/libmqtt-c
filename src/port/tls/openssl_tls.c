/**
 * @file openssl_tls.c
 * @brief OpenSSL implementation for MQTT TLS layer
 */

#include "mqtt_tls.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    SSL_CTX* ctx;
} openssl_context_t;

typedef struct {
    SSL* ssl;
    int fd;
} openssl_session_t;

static mqtt_tls_context_t openssl_init_impl(const mqtt_tls_config_t* config) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    openssl_context_t* ctx = calloc(1, sizeof(openssl_context_t));
    if (!ctx) return NULL;
    
    ctx->ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx->ctx) {
        free(ctx);
        return NULL;
    }
    
    // Set verification mode
    int verify_mode = SSL_VERIFY_NONE;
    if (config->verify_mode == 1) {
        verify_mode = SSL_VERIFY_PEER;
    } else if (config->verify_mode == 2) {
        verify_mode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
    SSL_CTX_set_verify(ctx->ctx, verify_mode, NULL);
    
    // Load CA certificate
    if (config->ca_cert) {
        BIO* bio = BIO_new_mem_buf(config->ca_cert, config->ca_cert_len);
        X509* cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
        if (cert) {
            X509_STORE* store = SSL_CTX_get_cert_store(ctx->ctx);
            X509_STORE_add_cert(store, cert);
            X509_free(cert);
        }
        BIO_free(bio);
    } else {
        // Use system default CA certificates
        SSL_CTX_set_default_verify_paths(ctx->ctx);
    }
    
    // Load client certificate and key
    if (config->client_cert && config->client_key) {
        BIO* cert_bio = BIO_new_mem_buf(config->client_cert, config->client_cert_len);
        X509* cert = PEM_read_bio_X509(cert_bio, NULL, NULL, NULL);
        if (cert) {
            SSL_CTX_use_certificate(ctx->ctx, cert);
            X509_free(cert);
        }
        BIO_free(cert_bio);
        
        BIO* key_bio = BIO_new_mem_buf(config->client_key, config->client_key_len);
        EVP_PKEY* pkey = PEM_read_bio_PrivateKey(key_bio, NULL, NULL, NULL);
        if (pkey) {
            SSL_CTX_use_PrivateKey(ctx->ctx, pkey);
            EVP_PKEY_free(pkey);
        }
        BIO_free(key_bio);
    }
    
    return (mqtt_tls_context_t)ctx;
}

static void openssl_cleanup_impl(mqtt_tls_context_t ctx) {
    openssl_context_t* context = (openssl_context_t*)ctx;
    SSL_CTX_free(context->ctx);
    free(context);
}

static mqtt_tls_session_t openssl_connect_impl(mqtt_tls_context_t ctx, const char* hostname, int fd) {
    openssl_context_t* context = (openssl_context_t*)ctx;
    openssl_session_t* session = calloc(1, sizeof(openssl_session_t));
    if (!session) return NULL;
    
    session->ssl = SSL_new(context->ctx);
    if (!session->ssl) {
        free(session);
        return NULL;
    }
    
    session->fd = fd;
    SSL_set_fd(session->ssl, fd);
    
    // Set SNI hostname
    if (hostname) {
        SSL_set_tlsext_host_name(session->ssl, hostname);
    }
    
    // Perform TLS handshake
    if (SSL_connect(session->ssl) != 1) {
        SSL_free(session->ssl);
        free(session);
        return NULL;
    }
    
    return (mqtt_tls_session_t)session;
}

static void openssl_disconnect_impl(mqtt_tls_session_t session) {
    openssl_session_t* sess = (openssl_session_t*)session;
    SSL_shutdown(sess->ssl);
    SSL_free(sess->ssl);
    free(sess);
}

static int openssl_send_impl(mqtt_tls_session_t session, const uint8_t* buf, size_t len) {
    openssl_session_t* sess = (openssl_session_t*)session;
    return SSL_write(sess->ssl, buf, len);
}

static int openssl_recv_impl(mqtt_tls_session_t session, uint8_t* buf, size_t len) {
    openssl_session_t* sess = (openssl_session_t*)session;
    int ret = SSL_read(sess->ssl, buf, len);
    if (ret == 0) {
        int err = SSL_get_error(sess->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        }
    }
    return ret;
}

static const mqtt_tls_api_t openssl_tls_api = {
    .init = openssl_init_impl,
    .cleanup = openssl_cleanup_impl,
    .connect = openssl_connect_impl,
    .disconnect = openssl_disconnect_impl,
    .send = openssl_send_impl,
    .recv = openssl_recv_impl
};

void mqtt_openssl_init(void) {
    mqtt_tls_init(&openssl_tls_api);
}
