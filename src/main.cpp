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
#include <thread>
#include <unordered_map>
#include <fstream>

const int PORT = 4221;
const int BUFFER_SIZE = 4096;

std::string target_directory = "";

std::string extractUrl(const std::string &req)
{
  std::istringstream iss(req);
  std::string method, url, version;
  iss >> method >> url >> version;
  return url;
}

std::string readFileContent(const std::string &full_path, bool &file_exists)
{
  std::ifstream file(full_path, std::ios::binary);
  if (!file.is_open())
  {
    file_exists = false;
    return "";
  }
  file_exists = true;
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
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

size_t getContentLength(const std::unordered_map<std::string, std::string> &headers)
{
  auto it = headers.find("Content-Length");
  if (it == headers.end())
  {
    return 0;
  }

  try
  {
    return static_cast<size_t>(std::stoul(it->second));
  }
  catch (const std::exception &)
  {
    return 0;
  }
}

std::string readRequest(int client_socket)
{
  std::string request_data;
  char buffer[BUFFER_SIZE] = {0};

  while (true)
  {
    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0)
    {
      break;
    }

    request_data.append(buffer, bytes_read);

    size_t header_end = request_data.find("\r\n\r\n");
    if (header_end == std::string::npos)
    {
      continue;
    }

    std::unordered_map<std::string, std::string> headers = extractAllHeaders(request_data.substr(0, header_end + 4));
    size_t content_length = getContentLength(headers);
    size_t body_start = header_end + 4;

    if (request_data.size() - body_start >= content_length)
    {
      break;
    }
  }

  return request_data;
}

std::string extractRequestBody(const std::string &request)
{
  size_t header_end = request.find("\r\n\r\n");
  if (header_end == std::string::npos)
  {
    return "";
  }

  return request.substr(header_end + 4);
}

void handleClient(int client_socket)
{
  std::string request_data = readRequest(client_socket);

  if (!request_data.empty())
  {
    std::istringstream request_stream(request_data);
    std::string method, requested_url, version;
    request_stream >> method >> requested_url >> version;

    std::string file_route_prefix = "/files/";
    std::string body = extractRequestBody(request_data);

    std::string response;
    if (requested_url == "/")
    {
      response = buildResponse("200 OK", "", "text/plain");
    }
    else if (requested_url.rfind("/echo/", 0) == 0)
    {
      std::string echo_body = requested_url.substr(6);
      response = buildResponse("200 OK", echo_body, "text/plain");
    }
    else if (requested_url.rfind("/user-agent", 0) == 0)
    {
      std::unordered_map<std::string, std::string> headers = extractAllHeaders(request_data);
      std::string user_agent = headers.count("User-Agent") ? headers["User-Agent"] : "";

      response = buildResponse("200 OK", user_agent, "text/plain");
    }
    else if (method == "POST" && requested_url.rfind(file_route_prefix, 0) == 0)
    {
      std::string filename = requested_url.substr(file_route_prefix.length());
      std::string base_directory = target_directory.empty() ? "." : target_directory;
      std::string full_file_path = base_directory + "/" + filename;

      std::ofstream output_file(full_file_path, std::ios::binary | std::ios::trunc);
      if (output_file.is_open())
      {
        output_file.write(body.c_str(), static_cast<std::streamsize>(body.size()));
        output_file.close();
        response = buildResponse("201 Created", "", "text/plain");
      }
      else
      {
        response = buildResponse("500 Internal Server Error", "", "text/plain");
      }
    }
    else if (requested_url.rfind(file_route_prefix, 0) == 0)
    {
      std::string filename = requested_url.substr(file_route_prefix.length());
      std::string full_file_path = target_directory + "/" + filename;

      bool file_exists = false;
      std::string file_content = readFileContent(full_file_path, file_exists);

      if (file_exists)
      {
        response = buildResponse("200 OK", file_content, "application/octet-stream");
      }
      else
      {
        response = buildResponse("404 Not Found", "", "text/plain");
      }
    }
    else
    {
      response = buildResponse("404 Not Found", "", "text/plain");
    }

    send(client_socket, response.c_str(), response.size(), 0);
  }

  close(client_socket);
}

int main(int argc, char **argv)
{
  // Flush after every std::cout / std::cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == "--directory" && i + 1 < argc)
    {
      target_directory = argv[i + 1];
      break;
    }
  }

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

    std::thread t(handleClient, client_fd);
    t.detach();
  }
  close(server_fd);

  return 0;
}
