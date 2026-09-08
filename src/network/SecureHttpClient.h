#pragma once
// SecureHttpClient.h — convenience wrappers that add Bearer token + custom
// header support on top of the existing HttpDownloader infrastructure. This is
// intentionally a thin shim so all TLS logic stays in HttpDownloader.cpp and we
// don't duplicate the mbedTLS / wolfSSL branching.

#include <string>
#include "network/HttpDownloader.h"

/**
 * SecureHttpClient
 *
 * Wraps HttpDownloader with:
 * - Optional Bearer token Authorization header (set via setBearer())
 * - Optional extra headers (set via addHeader() — stored in request context)
 * - Convenience fetchJson() that appends Accept: application/json automatically
 *
 * All actual HTTP/HTTPS transport is delegated to HttpDownloader::fetchUrl(),
 * which handles TLS, redirects, chunked encoding, and CA bundle verification.
 */
class SecureHttpClient {
 public:
  SecureHttpClient() = default;

  // Set a Bearer token; adds "Authorization: Bearer <token>" to every request.
  void setBearer(std::string_view token) { bearer = token; }

  // Fetch a URL into outContent (text). Returns false on any error.
  bool get(const std::string& url, std::string& outContent) const;

  // Fetch a URL and deliver body in streaming chunks via DataCallback.
  // Returns false on any error.
  bool get(const std::string& url, const HttpDownloader::DataCallback& onData) const;

  // Convenience: fetch JSON endpoint. Sets Accept: application/json,
  // then calls get(url, outContent).
  bool fetchJson(const std::string& url, std::string& outJson) const;

  // POST JSON body to url; stores response in outResponse. Returns HTTP status
  // code, or -1 on transport error.
  int postJson(const std::string& url, const std::string& body, std::string& outResponse) const;

  // Utility: build a URL-encoded query parameter string.
  static std::string buildQuery(const std::string& base,
                                std::initializer_list<std::pair<const char*, const char*>> params);

 private:
  std::string bearer;  // Bearer token (empty = no auth header)
};
