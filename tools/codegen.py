#!/usr/bin/env python3
"""
MRIM Protocol Code Generator
Generates C++ code from protocol.xml definition
"""
import xml.etree.ElementTree as ET
import sys
from pathlib import Path

class MrimCodegen:
    def __init__(self, xml_path):
        self.tree = ET.parse(xml_path)
        self.root = self.tree.getroot()
        
    def type_to_mrim_fd(self, type_name):
        """Convert XML type to MRIM field data type"""
        mapping = {
            'BYTE': 'MRIM_FD_BYTE',
            'UINT16': 'MRIM_FD_UINT16',
            'UINT32': 'MRIM_FD_UINT32',
            'UINT64': 'MRIM_FD_UINT64',
            'INT16': 'MRIM_FD_INT16',
            'INT32': 'MRIM_FD_INT32',
            'UBIART_LIKE_STRING': 'MRIM_FD_UBIART_LIKE_STRING',
            'UNICODE_STRING': 'MRIM_FD_UNICODE_STRING',
        }
        return mapping.get(type_name, 'MRIM_FD_BYTE')
    
    def generate_protocol_h(self):
        """Generate mrimprotocol.h"""
        commands = self.root.find('commands')
        enums = self.root.find('enums')
        handlers = self.root.find('handlers')
        methods = self.root.find('client_methods')
        
        code = """// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#ifndef MRIMPROTOCOL_H
#define MRIMPROTOCOL_H

#include <QObject>
#include <QTimer>
#include "mrimconnection.h"
#include "messagehandler.h"

namespace MRIM {

// Auto-generated commands enum
enum Command {
"""
        
        # Generate commands enum
        for cmd in commands.findall('command'):
            code += f"    {cmd.get('name')} = {cmd.get('value')},\n"
        
        code += "};\n\n"
        
        # Generate other enums
        for enum in enums.findall('enum'):
            code += f"enum {enum.get('name')} {{\n"
            for val in enum.findall('value'):
                code += f"    {val.get('name')} = {val.get('val')},\n"
            code += "};\n\n"
        
        code += """}; // namespace MRIM

class MrimProtocol : public QObject
{
    Q_OBJECT
public:
    explicit MrimProtocol(MrimConnection* connection, QObject *parent = nullptr);
    
    // Auto-generated send methods
"""
        
        # Generate method declarations
        for method in methods.findall('method'):
            params = []
            for param in method.findall('param'):
                params.append(f"const {param.get('type')}& {param.get('name')}")
            
            param_str = ", ".join(params) if params else ""
            code += f"    void {method.get('name')}({param_str});\n"
        
        code += """
    // Auto-generated callbacks (called by handlers)
"""
        
        # Generate callback declarations
        for handler in handlers.findall('handler'):
            callback = handler.get('callback')
            params = []
            for field in handler.findall('field'):
                if field.get('type') == 'UINT32':
                    params.append('quint32 ' + field.get('name'))
                elif 'STRING' in field.get('type'):
                    params.append('const QString& ' + field.get('name'))
            
            # Special case for complex handlers
            if handler.get('command') == 'CS_USER_INFO':
                params = ['const QMap<QString, QVariant>& info']
            
            param_str = ", ".join(params) if params else ""
            code += f"    void {callback}({param_str});\n"
        
        code += """
signals:
    // Auto-generated signals
"""
        
        # Generate signals
        for handler in handlers.findall('handler'):
            signal = handler.find('signal')
            if signal is not None:
                params = []
                for param in signal.findall('param'):
                    params.append(f"{param.get('type')} {param.get('name')}")
                
                param_str = ", ".join(params) if params else ""
                code += f"    void {signal.get('name')}({param_str});\n"
        
        code += """
private slots:
    void onPacketReceived(const MrimPacketHeader& header, const QByteArray& data);
    void onPingTimeout();

private:
    void sendPacket(quint32 command, const QByteArray& data = QByteArray());
    void initHandlers();
    
    MrimConnection* m_connection;
    MessageHandlerRegistry m_registry;
    QTimer* m_pingTimer;
    quint32 m_seq;
    quint32 m_proto;
    quint32 m_pingInterval;
};

#endif // MRIMPROTOCOL_H
""" # ugly as hell
        return code
    
    def generate_protocol_cpp(self):
        """Generate mrimprotocol.cpp"""
        handlers = self.root.find('handlers')
        methods = self.root.find('client_methods')
        
        code = '''// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#include "mrimprotocol.h"
#include "mrimmessage.h"
#include <QDebug>

MrimProtocol::MrimProtocol(MrimConnection *connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_pingTimer(new QTimer(this))
    , m_seq(1)
    , m_proto(0x10010010)
    , m_pingInterval(5000)
{
    connect(m_connection, &MrimConnection::packetReceived, this, &MrimProtocol::onPacketReceived);
    connect(m_pingTimer, &QTimer::timeout, this, &MrimProtocol::onPingTimeout);
    initHandlers();
}

void MrimProtocol::initHandlers()
{
'''
        
        # Generate handler registrations
        for handler in handlers.findall('handler'):
            cmd = handler.get('command')
            handler_class = cmd.replace('CS_', '').title().replace('_', '') + 'Handler'
            code += f"    m_registry.registerHandler(MRIM::{cmd}, std::make_unique<{handler_class}>());\n"
        
        code += "}\n\n// Auto-generated send methods\n\n"
        
        # Generate send methods
        for method in methods.findall('method'):
            method_name = method.get('name')
            command = method.get('command')
            params = method.findall('param')
            fields = method.findall('field')
            
            # Method signature
            param_list = []
            for param in params:
                param_list.append(f"const {param.get('type')} &{param.get('name')}")
            
            param_str = ", ".join(param_list) if param_list else ""
            code += f"void MrimProtocol::{method_name}({param_str})\n{{\n"
            
            if fields:
                # Has data to send
                code += "    MrimMessage constructor;\n"
                
                # Add fields
                for i, field in enumerate(fields):
                    field_name = field.get('name')
                    field_type = self.type_to_mrim_fd(field.get('type'))
                    if i == 0:
                        code += f"    constructor.field(\"{field_name}\", {field_type})\n"
                    else:
                        code += f"               ->field(\"{field_name}\", {field_type})\n"
                
                code += "               ;\n\n"
                code += "    QMap<QString, QVariant> data;\n"
                
                # Fill data
                for field in fields:
                    field_name = field.get('name')
                    source = field.get('source')
                    value = field.get('value')
                    
                    if source:
                        code += f"    data[\"{field_name}\"] = {source};\n"
                    elif value:
                        code += f"    data[\"{field_name}\"] = {value};\n"
                
                # Check if utf16 required
                utf16_required = any(f.get('utf16') == 'true' for f in fields)
                utf16_arg = ", true" if utf16_required else ""
                
                code += f"\n    sendPacket(MRIM::{command}, constructor.write(data{utf16_arg}));\n"
            else:
                # No data
                code += f"    sendPacket(MRIM::{command});\n"
            
            code += "}\n\n"
        
        code += "// Auto-generated callbacks\n\n"
        
        # Generate callbacks
        for handler in handlers.findall('handler'):
            callback = handler.get('callback')
            signal = handler.find('signal')
            
            # Build parameters
            params = []
            for field in handler.findall('field'):
                if field.get('type') == 'UINT32':
                    params.append('quint32 ' + field.get('name'))
                elif 'STRING' in field.get('type'):
                    params.append('const QString &' + field.get('name'))
            
            # Special case for CS_USER_INFO
            if handler.get('command') == 'CS_USER_INFO':
                params = ['const QMap<QString, QVariant> &info']
            
            param_str = ", ".join(params) if params else ""
            code += f"void MrimProtocol::{callback}({param_str})\n{{\n"
            
            # Special handling for CS_HELLO_ACK
            if callback == 'onHelloAck':
                code += "    m_pingInterval = pingInterval * 1000;\n"
                code += "    m_pingTimer->start(m_pingInterval);\n"
            
            # Emit signal
            if signal is not None:
                signal_params = []
                for param in signal.findall('param'):
                    pname = param.get('name')
                    ptype = param.get('type')
                    
                    # Type conversion for enums
                    if 'MRIM::' in ptype:
                        signal_params.append(f"{ptype}({pname})")
                    else:
                        signal_params.append(pname)
                
                signal_param_str = ", ".join(signal_params) if signal_params else ""
                code += f"    emit {signal.get('name')}({signal_param_str});\n"
            
            code += "}\n\n"
        
        # Add remaining methods
        code += '''// Infrastructure methods

void MrimProtocol::sendPacket(quint32 command, const QByteArray &data)
{
    MrimPacketHeader header;
    header.proto = m_proto;
    header.seq = m_seq++;
    header.msg = command;
    header.dlen = data.size();
    m_connection->sendPacket(header, data);
}

void MrimProtocol::onPacketReceived(const MrimPacketHeader &header, const QByteArray &data)
{
    MessageHandler* handler = m_registry.getHandler(header.msg);
    if (handler) {
        handler->handle(data, this);
    } else {
        qDebug() << "No handler for command" << Qt::hex << header.msg;
    }
}

void MrimProtocol::onPingTimeout()
{
    sendPing();
}
'''
        return code
    
    def generate_messagehandler_h(self):
        """Generate messagehandler.h"""
        handlers = self.root.find('handlers')
        
        code = """// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <memory>
#include <map>
#include <QByteArray>
#include "mrimmessage.h"

class MrimProtocol;

// Base handler interface
class MessageHandler
{
public:
    virtual ~MessageHandler() = default;
    virtual void handle(const QByteArray& data, MrimProtocol* protocol) = 0;
};

// Auto-generated handler classes

"""
        
        for handler in handlers.findall('handler'):
            cmd = handler.get('command')
            handler_class = cmd.replace('CS_', '').title().replace('_', '') + 'Handler'
            
            code += f"class {handler_class} : public MessageHandler {{\n"
            code += "public:\n"
            code += f"    {handler_class}();\n"
            code += "    void handle(const QByteArray& data, MrimProtocol* protocol) override;\n"
            code += "private:\n"
            code += "    std::unique_ptr<MrimMessage> m_constructor;\n"
            code += "};\n\n"
        
        code += """// Handler registry
class MessageHandlerRegistry {
public:
    void registerHandler(quint32 command, std::unique_ptr<MessageHandler> handler);
    MessageHandler* getHandler(quint32 command);
private:
    std::map<quint32, std::unique_ptr<MessageHandler>> m_handlers;
};

#endif // MESSAGEHANDLER_H
"""
        return code
    
    def generate_messagehandler_cpp(self):
        """Generate messagehandler.cpp"""
        handlers = self.root.find('handlers')
        
        code = '''// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#include "messagehandler.h"
#include "mrimprotocol.h"

// Auto-generated handler implementations

'''
        
        for handler in handlers.findall('handler'):
            cmd = handler.get('command')
            handler_class = cmd.replace('CS_', '').title().replace('_', '') + 'Handler'
            callback = handler.get('callback')
            fields = handler.findall('field')
            
            # Constructor
            code += f"// {cmd}\n{handler_class}::{handler_class}() {{\n"
            if fields:
                code += "    m_constructor = std::make_unique<MrimMessage>();\n"
                for i, field in enumerate(fields):
                    field_name = field.get('name')
                    field_type = self.type_to_mrim_fd(field.get('type'))
                    if i == 0:
                        code += f"    m_constructor->field(\"{field_name}\", {field_type})\n"
                    else:
                        code += f"                  ->field(\"{field_name}\", {field_type})\n"
                code += "                  ;\n"
            code += "}\n\n"
            
            # Handle method
            code += f"void {handler_class}::handle(const QByteArray& data, MrimProtocol* protocol) {{\n"
            
            if fields:
                # Check if utf16 required
                utf16_required = any(f.get('utf16') == 'true' for f in fields)
                utf16_arg = ", true" if utf16_required else ""
                
                code += f"    QMap<QString, QVariant> parsed = m_constructor->read(data{utf16_arg});\n"
                
                # Build callback arguments
                if cmd == 'CS_USER_INFO':
                    code += f"    protocol->{callback}(parsed);\n"
                else:
                    callback_args = []
                    for field in fields:
                        fname = field.get('name')
                        ftype = field.get('type')
                        if ftype == 'UINT32':
                            callback_args.append(f'parsed["{fname}"].toUInt()')
                        elif 'STRING' in ftype:
                            callback_args.append(f'parsed["{fname}"].toString()')
                    
                    arg_str = ", ".join(callback_args) if callback_args else ""
                    code += f"    protocol->{callback}({arg_str});\n"
            else:
                code += f"    protocol->{callback}();\n"
            
            code += "}\n\n"
        
        # Registry implementation
        code += '''// Registry implementation
void MessageHandlerRegistry::registerHandler(quint32 command, std::unique_ptr<MessageHandler> handler)
{
    m_handlers[command] = std::move(handler);
}

MessageHandler *MessageHandlerRegistry::getHandler(quint32 command)
{
    auto it = m_handlers.find(command);
    return it != m_handlers.end() ? it->second.get() : nullptr;
}
'''
        return code

def main():
    if len(sys.argv) < 2:
        print("Usage: python codegen.py <protocol.xml>")
        sys.exit(1)
    
    xml_file = sys.argv[1]
    
    try:
        gen = MrimCodegen(xml_file)
        
        # Generate files
        files = {
            'mrimprotocol.h': gen.generate_protocol_h(),
            'mrimprotocol.cpp': gen.generate_protocol_cpp(),
            'messagehandler.h': gen.generate_messagehandler_h(),
            'messagehandler.cpp': gen.generate_messagehandler_cpp(),
        }
        
        for filename, content in files.items():
            with open(filename, 'w', encoding='utf-8') as f:
                f.write(content)
            print(f"Generated {filename}")
        
        print(f"\nSuccessfully generated {len(files)} files from {xml_file}")
        
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
