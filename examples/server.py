import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind(('127.0.0.1', 12346))

server_socket.listen(1)

print("Сервер запущен и ожидает подключений...")

while 1:
    client_socket, client_address = server_socket.accept()
    print(f"Подключение установлено с {client_address}")

    while 1:
        data = client_socket.recv(1024)
        if not data:
            break
        print(f"Получены данные: {data}")

        client_socket.sendall(b'Hello, client!')

    client_socket.close()


