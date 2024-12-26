#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cerrno>

#define PORT 8888
#define BUF_SIZE 1024

volatile sig_atomic_t hangup_flag = 0;
int active_client_socket = -1;

void sighup_handler(int sig) {
    hangup_flag = 1;
}

void shutdown_socket(int sock) {
    if (sock != -1) {
        close(sock);
        std::cerr << "Сокет было закрыт." << std::endl;
    }
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sighup_handler;
    
    if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        perror("Не удалось установить обработчик сигнала");
        return 1;
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Не удалось создать сокет");
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Ошибка при привязке сокета");
        shutdown_socket(server_socket);
        return 1;
    }

    if (listen(server_socket, 1) == -1) {
        perror("Ошибка при прослушивании порта");
        shutdown_socket(server_socket);
        return 1;
    }

    std::cout << "Сервер успешно запущен на порту " << PORT << std::endl;

    fd_set read_fds;
    sigset_t block_mask, original_mask;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGHUP);
    
    if (sigprocmask(SIG_BLOCK, &block_mask, &original_mask) == -1) {
        perror("Ошибка при блокировке сигналов");
        shutdown_socket(server_socket);
        return 1;
    }

    while (true) {
        if (hangup_flag) {
            std::cout << "Получен сигнал SIGHUP. Завершение работы." << std::endl;
            hangup_flag = 0;
            shutdown_socket(server_socket);
            shutdown_socket(active_client_socket);
            sigprocmask(SIG_SETMASK, &original_mask, NULL);
            return 0;
        }

        FD_ZERO(&read_fds);
        FD_SET(server_socket, &read_fds);
        if (active_client_socket != -1)
            FD_SET(active_client_socket, &read_fds);

        int max_fd = std::max(server_socket, active_client_socket);
        int select_result = pselect(max_fd + 1, &read_fds, nullptr, nullptr, nullptr, &original_mask);
        if (select_result == -1) {
            if (errno == EINTR) continue;
            perror("Ошибка при выборке дескрипторов");
            break;
        }

        if (FD_ISSET(server_socket, &read_fds)) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_socket = accept(server_socket, (sockaddr*)&client_addr, &client_len);
            if (new_socket == -1) {
                perror("Ошибка при принятии соединения");
                continue;
            }
            std::cout << "Клиент успешно подключен." << std::endl;
            shutdown_socket(active_client_socket);
            active_client_socket = new_socket;
        }

        if (active_client_socket != -1 && FD_ISSET(active_client_socket, &read_fds)) {
            char buffer[BUF_SIZE];
            ssize_t bytes_received = recv(active_client_socket, buffer, BUF_SIZE - 1, 0);
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                std::cout << "Получено " << bytes_received << " байт: " << buffer << std::endl;
            } else if (bytes_received == 0) {

                std::cout << "Клиент закрыл соединение." << std::endl;
                shutdown_socket(active_client_socket);
                active_client_socket = -1;
            } else {
                perror("Ошибка при получении данных");
            }
        }
    }

    shutdown_socket(server_socket);
    return 0;
}
