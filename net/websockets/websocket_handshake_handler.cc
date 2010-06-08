// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/websockets/websocket_handshake_handler.h"

#include "base/md5.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "googleurl/src/gurl.h"
#include "net/http/http_util.h"

namespace {

const size_t kRequestKey3Size = 8U;
const size_t kResponseKeySize = 16U;

void ParseHandshakeHeader(
    const char* handshake_message, int len,
    std::string* status_line,
    std::string* headers) {
  size_t i = base::StringPiece(handshake_message, len).find_first_of("\r\n");
  if (i == base::StringPiece::npos) {
    *status_line = std::string(handshake_message, len);
    *headers = "";
    return;
  }
  // |status_line| includes \r\n.
  *status_line = std::string(handshake_message, i + 2);
  // |handshake_message| includes tailing \r\n\r\n.
  // |headers| doesn't include 2nd \r\n.
  *headers = std::string(handshake_message + i + 2, len - (i + 2) - 2);
}

void FetchHeaders(const std::string& headers,
                  const char* const headers_to_get[],
                  size_t headers_to_get_len,
                  std::vector<std::string>* values) {
  net::HttpUtil::HeadersIterator iter(headers.begin(), headers.end(), "\r\n");
  while (iter.GetNext()) {
    for (size_t i = 0; i < headers_to_get_len; i++) {
      if (LowerCaseEqualsASCII(iter.name_begin(), iter.name_end(),
                               headers_to_get[i])) {
        values->push_back(iter.values());
      }
    }
  }
}

bool GetHeaderName(std::string::const_iterator line_begin,
                   std::string::const_iterator line_end,
                   std::string::const_iterator* name_begin,
                   std::string::const_iterator* name_end) {
  std::string::const_iterator colon = std::find(line_begin, line_end, ':');
  if (colon == line_end) {
    return false;
  }
  *name_begin = line_begin;
  *name_end = colon;
  if (*name_begin == *name_end || net::HttpUtil::IsLWS(**name_begin))
    return false;
  net::HttpUtil::TrimLWS(name_begin, name_end);
  return true;
}

// Similar to HttpUtil::StripHeaders, but it preserves malformed headers, that
// is, lines that are not formatted as "<name>: <value>\r\n".
std::string FilterHeaders(
    const std::string& headers,
    const char* const headers_to_remove[],
    size_t headers_to_remove_len) {
  std::string filtered_headers;

  StringTokenizer lines(headers.begin(), headers.end(), "\r\n");
  while (lines.GetNext()) {
    std::string::const_iterator line_begin = lines.token_begin();
    std::string::const_iterator line_end = lines.token_end();
    std::string::const_iterator name_begin;
    std::string::const_iterator name_end;
    bool should_remove = false;
    if (GetHeaderName(line_begin, line_end, &name_begin, &name_end)) {
      for (size_t i = 0; i < headers_to_remove_len; ++i) {
        if (LowerCaseEqualsASCII(name_begin, name_end, headers_to_remove[i])) {
          should_remove = true;
          break;
        }
      }
    }
    if (!should_remove) {
      filtered_headers.append(line_begin, line_end);
      filtered_headers.append("\r\n");
    }
  }
  return filtered_headers;
}

// Gets a key number for |key_name| in |headers| and appends the number to
// |challenge|.
// The key number (/part_N/) is extracted as step 4.-8. in
// 5.2. Sending the server's opening handshake of
// http://www.ietf.org/id/draft-ietf-hybi-thewebsocketprotocol-00.txt
void GetKeyNumber(net::HttpRequestHeaders* headers, const char* key_name,
                  std::string* challenge) {
  std::string key;
  headers->GetHeader(key_name, &key);
  headers->RemoveHeader(key_name);

  uint32 key_number = 0;
  uint32 spaces = 0;
  for (size_t i = 0; i < key.size(); ++i) {
    if (isdigit(key[i]))
      key_number = key_number * 10 + key[i] - '0';
    else if (key[i] == ' ')
      ++spaces;
  }
  // spaces should not be zero in valid handshake request.
  if (spaces == 0)
    return;
  key_number /= spaces;

  char part[4];
  for (int i = 0; i < 4; i++) {
    part[3 - i] = key_number & 0xFF;
    key_number >>= 8;
  }
  challenge->append(part, 4);
}

}  // anonymous namespace

namespace net {

WebSocketHandshakeRequestHandler::WebSocketHandshakeRequestHandler()
    : original_length_(0),
      raw_length_(0) {}

bool WebSocketHandshakeRequestHandler::ParseRequest(
    const char* data, int length) {
  DCHECK_GT(length, 0);
  std::string input(data, length);
  int input_header_length =
      HttpUtil::LocateEndOfHeaders(input.data(), input.size(), 0);
  if (input_header_length <= 0 ||
      input_header_length + kRequestKey3Size > input.size())
    return false;

  ParseHandshakeHeader(input.data(),
                       input_header_length,
                       &status_line_,
                       &headers_);

  // draft-hixie-thewebsocketprotocol-76 or later will send /key3/
  // after handshake request header.
  // Assumes WebKit doesn't send any data after handshake request message
  // until handshake is finished.
  // Thus, |key3_| is part of handshake message, and not in part
  // of WebSocket frame stream.
  DCHECK_EQ(kRequestKey3Size,
            input.size() -
            input_header_length);
  key3_ = std::string(input.data() + input_header_length,
                      input.size() - input_header_length);
  original_length_ = input.size();
  return true;
}

size_t WebSocketHandshakeRequestHandler::original_length() const {
  return original_length_;
}

void WebSocketHandshakeRequestHandler::AppendHeaderIfMissing(
    const std::string& name, const std::string& value) {
  DCHECK(headers_.size() > 0);
  HttpUtil::AppendHeaderIfMissing(name.c_str(), value, &headers_);
}

void WebSocketHandshakeRequestHandler::RemoveHeaders(
    const char* const headers_to_remove[],
    size_t headers_to_remove_len) {
  DCHECK(headers_.size() > 0);
  headers_ = FilterHeaders(
      headers_, headers_to_remove, headers_to_remove_len);
}

HttpRequestInfo WebSocketHandshakeRequestHandler::GetRequestInfo(
    const GURL& url, std::string* challenge) {
  HttpRequestInfo request_info;
  request_info.url = url;
  base::StringPiece method = status_line_.data();
  size_t method_end = base::StringPiece(
      status_line_.data(), status_line_.size()).find_first_of(" ");
  if (method_end != base::StringPiece::npos)
    request_info.method = std::string(status_line_.data(), method_end);

  request_info.extra_headers.Clear();
  request_info.extra_headers.AddHeadersFromString(headers_);

  request_info.extra_headers.RemoveHeader("Upgrade");
  request_info.extra_headers.RemoveHeader("Connection");

  challenge->clear();
  GetKeyNumber(&request_info.extra_headers, "Sec-WebSocket-Key1", challenge);
  GetKeyNumber(&request_info.extra_headers, "Sec-WebSocket-Key2", challenge);
  challenge->append(key3_);

  return request_info;
}

std::string WebSocketHandshakeRequestHandler::GetRawRequest() {
  DCHECK(status_line_.size() > 0);
  DCHECK(headers_.size() > 0);
  DCHECK_EQ(kRequestKey3Size, key3_.size());
  std::string raw_request = status_line_ + headers_ + "\r\n" + key3_;
  raw_length_ = raw_request.size();
  return raw_request;
}

size_t WebSocketHandshakeRequestHandler::raw_length() const {
  DCHECK_GT(raw_length_, 0);
  return raw_length_;
}

WebSocketHandshakeResponseHandler::WebSocketHandshakeResponseHandler()
    : original_header_length_(0) {
}

size_t WebSocketHandshakeResponseHandler::ParseRawResponse(
    const char* data, int length) {
  DCHECK_GT(length, 0);
  if (HasResponse()) {
    DCHECK(status_line_.size() > 0);
    DCHECK(headers_.size() > 0);
    DCHECK_EQ(kResponseKeySize, key_.size());
    return 0;
  }

  size_t old_original_length = original_.size();

  original_.append(data, length);
  // TODO(ukai): fail fast when response gives wrong status code.
  original_header_length_ = HttpUtil::LocateEndOfHeaders(
      original_.data(), original_.size(), 0);
  if (!HasResponse())
    return length;

  ParseHandshakeHeader(original_.data(),
                       original_header_length_,
                       &status_line_,
                       &headers_);
  key_ = std::string(original_.data() + original_header_length_,
                     kResponseKeySize);

  return original_header_length_ + kResponseKeySize - old_original_length;
}

bool WebSocketHandshakeResponseHandler::HasResponse() const {
  return original_header_length_ > 0 &&
      original_header_length_ + kResponseKeySize <= original_.size();
}

bool WebSocketHandshakeResponseHandler::ParseResponseInfo(
    const HttpResponseInfo& response_info,
    const std::string& challenge) {
  if (!response_info.headers.get())
    return false;

  std::string response_message;
  response_message = response_info.headers->GetStatusLine();
  response_message += "\r\n";
  response_message += "Upgrade: WebSocket\r\n";
  response_message += "Connection: Upgrade\r\n";
  void* iter = NULL;
  std::string name;
  std::string value;
  while (response_info.headers->EnumerateHeaderLines(&iter, &name, &value)) {
    response_message += name + ": " + value + "\r\n";
  }
  response_message += "\r\n";

  MD5Digest digest;
  MD5Sum(challenge.data(), challenge.size(), &digest);

  const char* digest_data = reinterpret_cast<char*>(digest.a);
  response_message.append(digest_data, sizeof(digest.a));

  return ParseRawResponse(response_message.data(),
                          response_message.size()) == response_message.size();
}

void WebSocketHandshakeResponseHandler::GetHeaders(
    const char* const headers_to_get[],
    size_t headers_to_get_len,
    std::vector<std::string>* values) {
  DCHECK(HasResponse());
  DCHECK(status_line_.size() > 0);
  DCHECK(headers_.size() > 0);
  DCHECK_EQ(kResponseKeySize, key_.size());

  FetchHeaders(headers_, headers_to_get, headers_to_get_len, values);
}

void WebSocketHandshakeResponseHandler::RemoveHeaders(
    const char* const headers_to_remove[],
    size_t headers_to_remove_len) {
  DCHECK(HasResponse());
  DCHECK(status_line_.size() > 0);
  DCHECK(headers_.size() > 0);
  DCHECK_EQ(kResponseKeySize, key_.size());

  headers_ = FilterHeaders(headers_, headers_to_remove, headers_to_remove_len);
}

std::string WebSocketHandshakeResponseHandler::GetResponse() {
  DCHECK(HasResponse());
  DCHECK(status_line_.size() > 0);
  DCHECK(headers_.size() > 0);
  DCHECK_EQ(kResponseKeySize, key_.size());

  return status_line_ + headers_ + "\r\n" + key_;
}

}  // namespace net
