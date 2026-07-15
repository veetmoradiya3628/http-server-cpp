#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <vector>
#include <unordered_map>

const int PORT = 4221;
const int BUFFER_SIZE = 4096;

std::string extractUrl(const std::string &req)
{
  std::istringstream iss(req);
  std::string method, url, version;
  iss >> method >> url >> version;
  return url;
}

std::unordered_map<std::string, std::string> extractAllHeaders(const std::string &request)
{
  std::unordered_map<std::string, std::string> headers;
  std::istringstream stream(request);
  std::string line;

  if (!std::getline(stream, line))
  {
    return headers;
  }

  while (std::getline(stream, line))
  {
    if (!line.empty() && line.back() == '\r')
    {
      line.pop_back();
    }
    if (line.empty())
    {
      break;
    }
    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos)
    {
      std::string header_name = line.substr(0, colon_pos);
      std::string header_value = line.substr(colon_pos + 1);
      size_t first_non_space = header_value.find_first_not_of(" \t");
      if (first_non_space != std::string::npos)
      {
        header_value = header_value.substr(first_non_space);
      }
      else
      {
        header_value = "";
      }
      headers[header_name] = header_value;
    }
  }
  return headers;
}

std::string buildResponse(const std::string &status, const std::string &body, const std::string &content_type)
{
  std::ostringstream response;
  response << "HTTP/1.1 " << status << "\r\n";
  response << "Content-Type: " << content_type << "\r\n";
  response << "Content-Length: " << body.size() << "\r\n";
  response << "Connection: close\r\n";
  response << "\r\n";
  response << body;
  return response.str();
}

int main(int argc, char **argv)
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
  {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0)
  {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0)
  {
    std::cerr << "listen failed\n";
    return 1;
  }
  while (true)
  {

    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    std::cout << "Waiting for a client to connect...\n";

    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    if (client_fd < 0)
    {
      std::cerr << "failed to accept connection\n";
      continue;
    }
    std::cout << "Client connected\n";

    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

    std::string raw_request(buffer, bytes_read >= 0 ? bytes_read : 0);
    std::string requested_url = extractUrl(raw_request);

    std::string response;
    if (requested_url == "/")
    {
      response = buildResponse("200 OK", "", "text/plain");
    }
    else if (requested_url.rfind("/echo/", 0) == 0)
    {
      std::string body = requested_url.substr(6);
      response = buildResponse("200 OK", body, "text/plain");
    }
    else if (requested_url.rfind("/user-agent", 0) == 0)
    {
      std::unordered_map<std::string, std::string> headers = extractAllHeaders(raw_request);
      std::string user_agent = headers.count("User-Agent") ? headers["User-Agent"] : "";

      response = buildResponse("200 OK", user_agent.c_str(), "text/plain");
    }
    else
    {
      response = buildResponse("404 Not Found", "", "text/plain");
    }
    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
  }
  close(server_fd);

  return 0;
}
