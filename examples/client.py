import socket
import time

# Создаем сокет
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Подключаемся к серверу
client_socket.connect(('127.0.0.1', 12346))

# Отправляем данные серверу
for i in range(5):
    client_socket.sendall(b'Hello, server!')
    response = client_socket.recv(1024)
    print(f"Получен ответ от сервера: {response}")
    time.sleep(1)

# Закрываем соединение
client_socket.close()