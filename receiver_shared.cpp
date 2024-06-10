#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int recv_fd(int socket) {
  struct msghdr msg = {};
  struct iovec iov;
  char buf[1];
  iov.iov_base = buf;
  iov.iov_len = sizeof(buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
  msg.msg_control = ancillary_element_buffer;
  msg.msg_controllen = sizeof(ancillary_element_buffer);

  if (recvmsg(socket, &msg, 0) < 0) {
    perror("Failed to receive message");
    exit(EXIT_FAILURE);
  }

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  if (cmsg && cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
    return *((int *)CMSG_DATA(cmsg));
  }

  return -1; // Return an invalid fd on failure
}

int main() {
  const char *socket_path = "/tmp/fd_pass.socket";
  int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("Socket creation failed");
    return 1;
  }

  struct sockaddr_un sock_addr;
  memset(&sock_addr, 0, sizeof(sock_addr));
  sock_addr.sun_family = AF_UNIX;
  strcpy(sock_addr.sun_path, socket_path);

  if (connect(socket_fd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) <
      0) {
    perror("Connect failed");
    return 1;
  }

  int received_fd = recv_fd(socket_fd);
  std::cout << "Received fd: " << received_fd << std::endl;

  // Map the shared memory
  void *shared_mem_addr =
      mmap(NULL, 4096, PROT_READ, MAP_SHARED, received_fd, 0);
  if (shared_mem_addr == MAP_FAILED) {
    perror("mmap");
    return 1;
  }

  // Read and print the message from shared memory
  std::cout << "Message from Process A: "
            << static_cast<char *>(shared_mem_addr) << std::endl;

  munmap(shared_mem_addr, 4096);
  close(received_fd); // Close the shared memory fd
  close(socket_fd);

  return 0;
}
