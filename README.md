# libmrim

Client (library) implementation of MRIM (Mail.Ru Agent Instant Messaing) protocol using Qt.


(WIP)


## Dependencies

- Qt6 (Core, Network, Core5Compat)
- C++17-able compiler (e.g. anything these days)
- Python 3 (to generate code from XML protocol definition)


## Building

```bash
git clone https://github.com/relativemodder/libmrim
cd libmrim

python3 tools/codegen.py protocol.xml

mkdir build && cd build
cmake .. && cmake --build .
```

## How to use `libmrim` in other Qt C++ projects

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


## Adding new stuff

```xml
<!-- Add a new command -->
<command name="CS_NEW_COMMAND" value="0x2000" direction="client_to_server"/>

<!-- Add handler for the inbound command -->
<handler command="CS_NEW_RESPONSE" callback="onNewResponse">
    <field name="data" type="UINT32"/>
    <signal name="newResponseReceived">
        <param name="data" type="quint32"/>
    </signal>
</handler>

<!-- Add a method to send it -->
<method name="sendNewCommand" command="CS_NEW_COMMAND">
    <param name="value" type="quint32"/>
    <field name="value" type="UINT32" source="value"/>
</method>
```
