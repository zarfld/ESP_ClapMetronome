# HTTP Client Tests (Isolated)

## Purpose

Isolated tests for ESP32 HTTPClient functionality to diagnose connection issues independently from the WebServer implementation.

## Tests

### test_simple_http.cpp

Basic HTTPClient functionality tests using external endpoints (httpbin.org):

- **test_http_client_basic**: Single HTTP GET request
- **test_http_client_multiple_gets**: Sequential GET requests
- **test_http_client_reuse**: Connection reuse/pooling

## Usage

### Run on ESP32 Hardware

```bash
pio test -e esp32dev --filter test_http_client --upload-port COM4
```

### Expected Results

All tests should pass if:
- ESP32 is connected to WiFi
- Internet connection is available
- httpbin.org is reachable

## Troubleshooting

**All tests fail with -11 error:**
- Check WiFi credentials in `test/credentials.h`
- Verify internet connectivity
- Try increasing timeout in tests

**Tests timeout:**
- Check firewall settings
- Verify DNS resolution works
- Try different external endpoint

## Purpose in CI/CD

These isolated tests help identify whether HTTP client issues are:
1. **HTTPClient library bugs** → Tests fail here
2. **ESPAsyncWebServer routing issues** → Tests pass here, fail in test_web_integration
3. **Network/WiFi issues** → All tests fail

## Related

- Main integration tests: `test/test_web_integration/`
- WebServer implementation: `src/web/WebServer.cpp`
