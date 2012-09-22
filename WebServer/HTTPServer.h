#include <Ethernet.h>

typedef enum {
  HTTP_NO_RESPONSE = 0,
  HTTP_BAD_REQUEST = 400, // Bad Request
  HTTP_NOT_FOUND = 404, // Not Found
  HTTP_METHOD_NOT_ALLOWED = 405, // Method Not Allowed
  HTTP_REQUEST_URI_TOO_LARGE = 414, // Request-URI Too Large
} HTTPResponseCode;

typedef enum {
  HTTP_UNKNOWN_METHOD = 0,
  HTTP_GET,
  HTTP_POST,
} HTTPMethod;

class HTTPRequest;
class HTTPEntity;

class HTTPEntity : public Stream {
    HTTPRequest *request;
  public:
    HTTPEntity(HTTPRequest *aRequest);
    int
      available(),
      pending(),
      peek(),
      read();
    size_t write(uint8_t val);
    void flush();
  private:
    int bytes_read;
};

class HTTPRequest {
  public:
    EthernetClient *client;
    HTTPMethod method;
    char url[5];
    int content_length;
    HTTPResponseCode error_code;

    HTTPRequest(EthernetClient *aClient);
  private:
    boolean
      read_method(),
      read_url(),
      read_protocol(),
      read_headers();
};

