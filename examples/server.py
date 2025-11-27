import socket

# Создаем сокет
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Привязываем сокет к IP-адресу и порту
server_socket.bind(('127.0.0.1', 12346))

# Слушаем входящие соединения
server_socket.listen(1)

print("Сервер запущен и ожидает подключений...")

# Принимаем входящее соединение
client_socket, client_address = server_socket.accept()
print(f"Подключение установлено с {client_address}")

# Получаем данные от клиента
data = client_socket.recv(1024)
print(f"Получены данные: {data}")

client_socket.sendall(b'Hello, client!')

# Закрываем соединения
client_socket.close()
server_socket.close()