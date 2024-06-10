#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

void send_fd(int socket, int fd_to_send) {
  struct msghdr msg = {};
  struct iovec iov;
  char buf[1] = {0}; // Buffer for single byte of data to send
  iov.iov_base = buf;
  iov.iov_len = sizeof(buf);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  char ancillary_element_buffer[CMSG_SPACE(sizeof(int))];
  msg.msg_control = ancillary_element_buffer;
  msg.msg_controllen = sizeof(ancillary_element_buffer);

  struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  *((int *)CMSG_DATA(cmsg)) = fd_to_send;

  if (sendmsg(socket, &msg, 0) < 0) {
    perror("Failed to send message");
    exit(EXIT_FAILURE);
  }
}

int main() {
  const char *shm_name = "/my_shared_memory";
  int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
  if (shm_fd < 0) {
    perror("shm_open");
    return 1;
  }
  ftruncate(shm_fd, 4096);

  void *addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    close(shm_fd);
    shm_unlink(shm_name);
    return 1;
  }

  // Write a message to the shared memory
  const char *message = "Hello from Process A!";
  memcpy(addr, message, strlen(message) + 1);

  const char *socket_path = "/tmp/fd_pass.socket";
  int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket creation failed");
    return 1;
  }

  struct sockaddr_un addr_un;
  memset(&addr_un, 0, sizeof(addr_un));
  addr_un.sun_family = AF_UNIX;
  strcpy(addr_un.sun_path, socket_path);
  unlink(socket_path);

  if (bind(socket_fd, (struct sockaddr *)&addr_un, sizeof(addr_un)) < 0) {
    perror("bind failed");
    return 1;
  }

  listen(socket_fd, 1);
  std::cout << "Waiting for a connection..." << std::endl;
  int client_fd = accept(socket_fd, NULL, NULL);
  if (client_fd < 0) {
    perror("accept failed");
    return 1;
  }

  send_fd(client_fd, shm_fd);

  close(client_fd);
  close(socket_fd);
  munmap(addr, 4096);
  close(shm_fd);
  unlink(socket_path); // Optionally unlink the shared memory here, or after
                       // it's no longer used

  return 0;
}