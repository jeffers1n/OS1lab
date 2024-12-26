#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cerrno>

#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

volatile sig_atomic_t sighup_received = 0;
int client_socket = -1;

void handle_sighup(int sig) {
    sighup_received = 1;
}

void close_socket(int sock) {
    if (sock != -1) {
        close(sock);
        std::cerr << "Сокет закрыт.n";
    }
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sighup;
    if (sigaction(SIGHUP, &sa, nullptr) == -1) {
        perror("Ошибка установки обработчика сигнала");
        return 1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Ошибка создания сокета");
        return 1;
    }

    sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Ошибка привязки сокета");
        close_socket(server_fd);
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("Ошибка прослушивания порта");
        close_socket(server_fd);
        return 1;
    }

    std::cout << "Сервер запущен, порт " << SERVER_PORT << std::endl;

    fd_set readfds;
    sigset_t blocked_signals, original_mask;

    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &original_mask) == -1) {
        perror("Ошибка блокировки сигналов");
        close_socket(server_fd);
        return 1;
    }


    while (true) {
        if (sighup_received) {
            std::cout << "Получен сигнал SIGHUP. Завершение работы.n";
            sighup_received = 0;
            close_socket(server_fd);
            close_socket(client_socket);
            sigprocmask(SIG_SETMASK, &original_mask, NULL);
            return 0;
        }

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        if (client_socket != -1)
            FD_SET(client_socket, &readfds);

        int max_fd = std::max(server_fd, client_socket);

        int sel_result = pselect(max_fd + 1, &readfds, nullptr, nullptr, nullptr, &original_mask);
        if (sel_result == -1) {
            if (errno == EINTR) continue;
            perror("Ошибка pselect");
            break;
        }


        if (FD_ISSET(server_fd, &readfds)) {
            sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);
            int new_client_socket = accept(server_fd, (sockaddr*)&client_address, &client_len);
            if (new_client_socket == -1) {
                perror("Ошибка принятия соединения");
                continue;
            }
            std::cout << "Новое соединение принято.n";
            close_socket(client_socket);
            client_socket = new_client_socket;

        }

        if (client_socket != -1 && FD_ISSET(client_socket, &readfds)) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_read = recv(client_socket, buffer, BUFFER_SIZE -1, 0);
            if (bytes_read > 0) {
                buffer[bytes_read] = '0';
                std::cout << "Получено " << bytes_read << " байт: " << buffer << std::endl;
            } else if (bytes_read == 0) {
                std::cout << "Клиент закрыл соединение.n";
                close_socket(client_socket);
                client_socket = -1;
            } else {
                perror("Ошибка чтения данных");
            }
        }
    }

    close_socket(server_fd);
    return 0;
}
