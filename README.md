## О проекте
Мини-сервер обрабатывающий запросы _DNS lookup_. Простыми словами: на вход подаются доменные имена, сервер возвращает список _ip_ адресов связанных с данным доменом. К серверу одновременно может подключаться множество клиентов, есть ограничение по колличеству активных соединений с одного _ip_. Также есть ограничение 1024 байта на один запрос.

## Протокол взаимодействия
 - __Клиент__ присылает доменные имена разбитые `CRLF`
 - __Сервер__ отвечает _ip_ адресами разделенными `CRLF`
 
## Сборка
Сборка проекта возможна только на _Linux_ системах, так как в коде активно используются системные вызовы.
```shell-script
mkdir -p cmake-build && cd cmake-build
cmake -G "Unix Makefiles" .. && make
```

## Запуск
Сервер будет доступен по адресу __127.0.0.1:1337__
```shell-script
cmake-build/dns-lookup-server
```
