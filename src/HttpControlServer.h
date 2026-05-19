#pragma once

#include <QtCore/QObject>
#include <cstdint>

class QTcpServer;
class QTcpSocket;

// Localhost-only HTTP/1.1 control endpoint for Stream Deck and similar
// external integrations. Equivalent to the macOS HTTPControlServer; uses
// Qt's networking primitives instead of raw BSD sockets so it stays
// portable across Qt versions and Windows networking stacks.
//
// Routes (all GET):
//     /health
//     /mic/state, /mic/toggle
//     /speaker/state, /speaker/toggle
//     /away/state, /away/toggle[?channel=Lobby/AFK]
//     /silent/state, /silent/toggle[?channel=Lobby/AFK]
//     /channel/current
//     /channel/move?path=Lobby/AFK
//     /quit
class HttpControlServer : public QObject {
    Q_OBJECT
public:
    explicit HttpControlServer(QObject* parent = nullptr);
    ~HttpControlServer() override;

    // Bind to 127.0.0.1:<port>. Returns false if bind/listen fails (port
    // taken, permission denied). On success start() is idempotent.
    bool start(uint16_t port);
    void stop();

    bool running() const { return m_running; }
    uint16_t port() const { return m_port; }

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    void handleRequest(QTcpSocket* sock, const QString& method,
                       const QString& fullPath);
    void writeResponse(QTcpSocket* sock, int status,
                       const QByteArray& contentType,
                       const QByteArray& body);
    QByteArray jsonState(const char* key, int value) const;

    QTcpServer* m_server = nullptr;
    bool m_running = false;
    uint16_t m_port = 0;
};
