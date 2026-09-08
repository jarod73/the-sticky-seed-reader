#include "CloudHttpClient.h"

#include <Arduino.h>
#include <Logging.h>
#include <Memory.h>
#include <esp_crt_bundle.h>
#include <esp_http_client.h>

namespace {
constexpr int HTTP_TIMEOUT_MS = 20000;
constexpr int HTTP_RX_BUF = 2048;
constexpr int HTTP_TX_BUF = 1024;
}  // namespace

bool CloudHttpClient::get(const std::string& url, std::string& outContent) const {
  if (bearer.empty()) {
    return HttpDownloader::fetchUrl(url, outContent);
  }

  // When Bearer token is set, configure esp_http_client with auth header and CRT bundle
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.buffer_size = HTTP_RX_BUF;
  config.buffer_size_tx = HTTP_TX_BUF;
  config.timeout_ms = HTTP_TIMEOUT_MS;
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.keep_alive_enable = true;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    LOG_ERR("CLOUD_HTTP", "Failed to init HTTP client");
    return false;
  }

  esp_http_client_set_header(client, "User-Agent", "StickySeedReader/1.0 (ESP32-S3)");
  std::string authHeader = "Bearer " + bearer;
  esp_http_client_set_header(client, "Authorization", authHeader.c_str());

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    LOG_ERR("CLOUD_HTTP", "Open failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return false;
  }

  esp_http_client_fetch_headers(client);
  const int status = esp_http_client_get_status_code(client);
  if (status < 200 || status >= 300) {
    LOG_ERR("CLOUD_HTTP", "HTTP error status: %d", status);
    esp_http_client_cleanup(client);
    return false;
  }

  outContent.clear();
  char buf[512];
  while (true) {
    int read = esp_http_client_read(client, buf, sizeof(buf));
    if (read < 0) {
      LOG_ERR("CLOUD_HTTP", "Read error");
      esp_http_client_cleanup(client);
      return false;
    }
    if (read == 0) break;
    outContent.append(buf, read);
  }

  esp_http_client_cleanup(client);
  return true;
}

bool CloudHttpClient::get(const std::string& url, const HttpDownloader::DataCallback& onData) const {
  return HttpDownloader::fetchUrl(url, onData);
}

bool CloudHttpClient::fetchJson(const std::string& url, std::string& outJson) const {
  return get(url, outJson);
}

int CloudHttpClient::postJson(const std::string& url, const std::string& body, std::string& outResponse) const {
  esp_http_client_config_t config = {};
  config.url = url.c_str();
  config.method = HTTP_METHOD_POST;
  config.buffer_size = HTTP_RX_BUF;
  config.buffer_size_tx = HTTP_TX_BUF;
  config.timeout_ms = HTTP_TIMEOUT_MS;
  config.crt_bundle_attach = esp_crt_bundle_attach;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    LOG_ERR("CLOUD_HTTP", "POST client init failed");
    return -1;
  }

  esp_http_client_set_header(client, "Content-Type", "application/json");
  esp_http_client_set_header(client, "User-Agent", "StickySeedReader/1.0 (ESP32-S3)");
  if (!bearer.empty()) {
    std::string authHeader = "Bearer " + bearer;
    esp_http_client_set_header(client, "Authorization", authHeader.c_str());
  }

  esp_err_t err = esp_http_client_open(client, body.length());
  if (err != ESP_OK) {
    LOG_ERR("CLOUD_HTTP", "POST open failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return -1;
  }

  int written = esp_http_client_write(client, body.c_str(), body.length());
  if (written < 0) {
    LOG_ERR("CLOUD_HTTP", "POST write failed");
    esp_http_client_cleanup(client);
    return -1;
  }

  esp_http_client_fetch_headers(client);
  const int status = esp_http_client_get_status_code(client);

  outResponse.clear();
  char buf[512];
  while (true) {
    int read = esp_http_client_read(client, buf, sizeof(buf));
    if (read <= 0) break;
    outResponse.append(buf, read);
  }

  esp_http_client_cleanup(client);
  return status;
}

std::string CloudHttpClient::buildQuery(std::string_view base,
                                        std::initializer_list<std::pair<const char*, const char*>> params) {
  std::string query(base);
  query.reserve(base.size() + 128);

  bool first = (base.find('?') == std::string_view::npos);
  for (const auto& [k, v] : params) {
    query.push_back(first ? '?' : '&');
    first = false;
    query.append(k);
    query.push_back('=');
    query.append(v);
  }

  return query;
}
