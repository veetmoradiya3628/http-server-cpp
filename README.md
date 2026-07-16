[![progress-banner](https://backend.codecrafters.io/progress/http-server/6b3401ac-235e-414d-85aa-81c0253739ef)](https://app.codecrafters.io/users/veetmoradiya3628?r=2qF)

This project is a C++ implementation of a simple HTTP/1.1 server built as part of the
[Build Your Own HTTP Server](https://app.codecrafters.io/courses/http-server/overview) challenge.

# What has been implemented so far

The server currently supports:

- A basic TCP server listening on port 4221
- A simple request parser for HTTP methods, paths, and headers
- A root endpoint returning a basic response for `/`
- An echo endpoint at `/echo/{str}` that returns the requested string in the response body
- A `/user-agent` endpoint that returns the incoming `User-Agent` header value
- Basic file handling for `/files/{name}`:
  - `POST` stores a file on disk
  - `GET` reads and returns the stored file content
- Gzip compression support for `/echo/{str}` when the client sends `Accept-Encoding: gzip`
- Persistent HTTP connections so multiple requests can be served over the same TCP connection by default
- Basic connection handling that closes the socket only when the client explicitly requests `Connection: close`

# Running the server

From the project root, run:

```sh
./your_program.sh
```

You can test it with `curl`, for example:

```sh
curl http://localhost:4221/
curl http://localhost:4221/echo/banana
curl -H "User-Agent: my-client" http://localhost:4221/user-agent
```

# Future scope / enhancements (Case 4 and beyond)

The current implementation is a solid foundation and can be improved in several ways:

- Add more complete HTTP/1.1 support, including better parsing for multiple headers and edge cases
- Improve keep-alive handling with proper timeout and request limit management
- Support additional methods such as `HEAD`
- Return more accurate status codes and error responses
- Add MIME type detection for files based on extension
- Add directory traversal protection for file operations
- Improve concurrency and connection handling for higher traffic
- Add logging, metrics, and request tracing
- Explore HTTPS/TLS support as a later-stage enhancement
- Add support for more compression formats or content negotiation in the future
