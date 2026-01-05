# libmrim

Client (library) implementation of MRIM (Mail.Ru Agent Instant Messaing) protocol using Qt.


(WIP)


## Dependencies

- Qt6 (Core, Network, Core5Compat)
- C++17-able compiler (e.g. anything these days)


## Building

```bash
git clone https://github.com/relativemodder/libmrim
cd libmrim && mkdir build && cd build
cmake .. && cmake --build .
```

## How to use `libmrim` in other CMake projects

### CMake usage

```cmake
add_subdirectory(/path/to/libmrim libmrim)

...

target_include_directories(testclient PUBLIC /path/to/libmrim)
```

### Test client
```cpp
#include <QCoreApplication>
#include <QDebug>
#include <mrimmessage.h>
#include <mrimclient.h>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qputenv("QT_ASSUME_STDERR_HAS_CONSOLE", "1"); // showing debug messages
    qDebug("Hello");

    MrimClient client;
    client.connectToServer("mrim.example.com", 2041);

    QObject::connect(&client, &MrimClient::connected, [&client]() {
        client.login("login@mail.ru", "password", MRIM::STATUS_ONLINE);
    });

    QObject::connect(&client, &MrimClient::userInfoReceived, [&client](const QMap<QString, QVariant>& info) {
        qDebug() << "Received user info!" << info;
    });

    QObject::connect(&client, &MrimClient::loginSuccessful, [&client]() {
        client.sendMessage("someuser@mail.ru", "Hello mate, I'm texting from MRIM Qt client.");
    });

    return a.exec();
}

```
