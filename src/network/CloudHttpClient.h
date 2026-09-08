#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

#include "network/HttpDownloader.h"

/**
 * CloudHttpClient
 *
 * A specialized, robust HTTPS client designed for cloud REST API interactions
 * on ESP32-S3 (Notion, Readwise, Todoist, Open-Meteo, Gemini, Wallabag, etc.).
 *
 * Key Capabilities:
 * 1. Bearer Token Authorization: Automatically attaches `Authorization: Bearer <token>`
 *    and standard `User-Agent` headers.
 * 2. High-Performance Transport: Leverages `esp_http_client` with hardware TLS/wolfSSL
 *    and ESP-IDF CRT bundle root certificate verification.
 * 3. JSON Helper APIs: Convenient `fetchJson()` and `postJson()` methods.
 * 4. Streaming Support: Direct callbacks via `HttpDownloader::DataCallback` to process
 *    large payloads without allocating full responses in heap RAM.
 */
class CloudHttpClient {
 public:
  CloudHttpClient() = default;

  /**
   * Sets the Bearer authentication token.
   * When non-empty, requests include `Authorization: Bearer <token>`.
   */
  void setBearer(std::string_view token) { bearer = std::string(token); }

  /**
   * Clears the current Bearer authentication token.
   */
  void clearBearer() { bearer.clear(); }

  /**
   * Performs an HTTPS GET request and buffers the response body into `outContent`.
   *
   * @param url Target endpoint URL.
   * @param outContent String where response body is stored.
   * @return True if HTTP status was 2xx and transport completed cleanly.
   */
  bool get(const std::string& url, std::string& outContent) const;

  /**
   * Performs a streaming HTTPS GET request without buffering the entire body into memory.
   *
   * @param url Target endpoint URL.
   * @param onData Callback invoked for each received chunk.
   * @return True on success.
   */
  bool get(const std::string& url, const HttpDownloader::DataCallback& onData) const;

  /**
   * Convenience wrapper for GET operations retrieving JSON data.
   */
  bool fetchJson(const std::string& url, std::string& outJson) const;

  /**
   * Performs an HTTPS POST request with a JSON body (`Content-Type: application/json`).
   *
   * @param url Target endpoint URL.
   * @param body JSON request payload string.
   * @param outResponse Buffer populated with the response body.
   * @return HTTP status code (e.g. 200, 201), or -1 on transport failure.
   */
  int postJson(const std::string& url, const std::string& body, std::string& outResponse) const;

  /**
   * Utility to construct a URL with query parameters:
   * e.g., buildQuery("https://api.example.com", {{"q", "sticky"}, {"limit", "10"}})
   */
  static std::string buildQuery(std::string_view base,
                                std::initializer_list<std::pair<const char*, const char*>> params);

 private:
  std::string bearer;
};
