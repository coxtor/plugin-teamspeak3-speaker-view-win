#include "HttpControlServer.h"

#include "Log.h"
#include "PluginContext.h"
#include "TS3Bridge.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include <cstdio>
#include <cstdlib>

HttpControlServer::HttpControlServer(QObject* parent) : QObject(parent) {}

HttpControlServer::~HttpControlServer() {
    stop();
}

bool HttpControlServer::start(uint16_t port) {
    if (m_running && m_port == port) return true;
    if (m_server) stop();

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection,
            this, &HttpControlServer::onNewConnection);

    // 127.0.0.1 binding only — never expose to the LAN. No auth on the
    // wire; the contract is "if you can reach localhost on this machine,
    // you can already control TS3 anyway".
    if (!m_server->listen(QHostAddress::LocalHost, port)) {
        sv_log(QStringLiteral("HttpControlServer: listen on port %1 failed: %2")
                   .arg(port).arg(m_server->errorString()));
        // Synchronous delete only — see stop() rationale.
        delete m_server;
        m_server = nullptr;
        return false;
    }
    m_port = port;
    m_running = true;
    sv_log(QStringLiteral("HttpControlServer listening on 127.0.0.1:%1").arg(port));
    return true;
}

void HttpControlServer::stop() {
    if (m_server) {
        m_server->close();
        // Synchronous delete only. deleteLater() crashes TS3 on shutdown
        // and on Reload All: TS3 unloads our DLL right after
        // ts3plugin_shutdown returns, then Qt's event loop later tries to
        // dispatch the queued delete-events into freed code. The same
        // rationale as PluginContext::teardownOnShutdown.
        delete m_server;
        m_server = nullptr;
    }
    m_running = false;
    m_port = 0;
}

void HttpControlServer::onNewConnection() {
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket* sock = m_server->nextPendingConnection();
        if (!sock) break;
        connect(sock, &QTcpSocket::readyRead,
                this, &HttpControlServer::onClientReadyRead);
        connect(sock, &QTcpSocket::disconnected,
                this, &HttpControlServer::onClientDisconnected);
        // Hard timeout in case a client connects and never sends bytes.
        QTimer::singleShot(5000, sock, [sock] {
            if (sock && sock->state() != QAbstractSocket::UnconnectedState) {
                sock->abort();
            }
        });
    }
}

void HttpControlServer::onClientReadyRead() {
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;

    // We only need the request line; cap at 4 KiB to ignore malformed
    // clients trying to flood us.
    QByteArray buf = sock->peek(4096);
    int eolHeader = buf.indexOf("\r\n\r\n");
    if (eolHeader < 0) eolHeader = buf.indexOf("\n\n");
    if (eolHeader < 0) {
        // Headers not complete yet; wait for more bytes.
        if (buf.size() >= 4096) {
            writeResponse(sock, 400, "text/plain", "headers too large\n");
            sock->disconnectFromHost();
        }
        return;
    }

    // Consume the headers we just peeked.
    sock->read(eolHeader + 4 > buf.size() ? buf.size() : eolHeader + 4);

    int eolFirst = buf.indexOf("\r\n");
    if (eolFirst < 0) eolFirst = buf.indexOf("\n");
    if (eolFirst < 0) {
        writeResponse(sock, 400, "text/plain", "malformed request line\n");
        sock->disconnectFromHost();
        return;
    }
    QString reqLine = QString::fromUtf8(buf.left(eolFirst));
    QStringList parts = reqLine.split(' ');
    if (parts.size() < 2) {
        writeResponse(sock, 400, "text/plain", "malformed request line\n");
        sock->disconnectFromHost();
        return;
    }
    QString method = parts[0];
    QString path   = parts[1];

    sv_log(QStringLiteral("HTTP %1 %2").arg(method, path));
    handleRequest(sock, method, path);
}

void HttpControlServer::onClientDisconnected() {
    QTcpSocket* sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;
    // QTcpSocket parented to QTcpServer (so it dies with the server on
    // teardown) — but we still want to free the per-connection memory
    // promptly when the peer disconnects mid-session. deleteLater is
    // safe here because the event loop is alive at this point; on
    // shutdown we never reach this path because stop() deletes the
    // server (and all its socket children) synchronously first.
    sock->deleteLater();
}

QByteArray HttpControlServer::jsonState(const char* key, int value) const {
    if (value < 0) {
        return QByteArray("{\"") + key + "\":null,\"connected\":false}";
    }
    return QByteArray("{\"") + key + "\":" + (value ? "true" : "false")
           + ",\"connected\":true}";
}

// Minimal JSON-string escape sufficient for TS3 channel names (control
// chars, quotes, backslashes). UTF-8 is passed through unchanged.
static QByteArray jsonEscape(const QString& in) {
    QByteArray utf = in.toUtf8();
    QByteArray out;
    out.reserve(utf.size() + 2);
    for (char c : utf) {
        unsigned char u = static_cast<unsigned char>(c);
        switch (u) {
            case '"':  out.append("\\\""); break;
            case '\\': out.append("\\\\"); break;
            case '\b': out.append("\\b");  break;
            case '\f': out.append("\\f");  break;
            case '\n': out.append("\\n");  break;
            case '\r': out.append("\\r");  break;
            case '\t': out.append("\\t");  break;
            default:
                if (u < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", u);
                    out.append(buf);
                } else {
                    out.append(static_cast<char>(u));
                }
        }
    }
    return out;
}

void HttpControlServer::handleRequest(QTcpSocket* sock, const QString& method,
                                      const QString& fullPath) {
    Q_UNUSED(method);

    TS3Bridge* bridge = PluginContext::instance().bridge();

    // Split "/foo?a=b&c=d" into path + decoded query map. QUrl handles the
    // percent-decoding; we feed it just the request-target so it doesn't try
    // to interpret the leading slash as a scheme.
    QUrl url;
    url.setUrl(fullPath, QUrl::TolerantMode);
    QString path = url.path();
    QUrlQuery query(url.query());
    QString afkPath = query.queryItemValue(
        QStringLiteral("channel"), QUrl::FullyDecoded);

    auto reply = [&](int v, const char* key) {
        // ObjC-style "send to nil returns 0" doesn't apply in C++; we just
        // gate the call on bridge != nullptr in the callers.
        QByteArray body = jsonState(key, v);
        writeResponse(sock, 200, "application/json", body);
    };
    auto withBridge = [&](auto fn, const char* key) {
        int v = bridge ? fn(bridge) : -1;
        reply(v, key);
    };

    if (path == "/" || path == "/health") {
        writeResponse(sock, 200, "text/plain", "speakerview-control ok\n");
        return;
    }
    if (path == "/mic/toggle") {
        withBridge([](TS3Bridge* b){ return b->toggleSelfMicMuted(); }, "muted");
        return;
    }
    if (path == "/mic/state") {
        withBridge([](TS3Bridge* b){ return b->selfMicMuted(); }, "muted");
        return;
    }
    if (path == "/speaker/toggle") {
        withBridge([](TS3Bridge* b){ return b->toggleSelfSpeakerMuted(); }, "muted");
        return;
    }
    if (path == "/speaker/state") {
        withBridge([](TS3Bridge* b){ return b->selfSpeakerMuted(); }, "muted");
        return;
    }
    if (path == "/away/toggle") {
        withBridge([&afkPath](TS3Bridge* b){ return b->toggleSelfAwayWithAfkPath(afkPath); }, "away");
        return;
    }
    if (path == "/away/state") {
        withBridge([](TS3Bridge* b){ return b->selfAway(); }, "away");
        return;
    }
    if (path == "/silent/toggle") {
        withBridge([&afkPath](TS3Bridge* b){ return b->toggleSelfSilentModeWithAfkPath(afkPath); }, "silent");
        return;
    }
    if (path == "/silent/state") {
        withBridge([](TS3Bridge* b){ return b->selfSilentMode(); }, "silent");
        return;
    }
    if (path == "/channel/current") {
        if (!bridge) {
            writeResponse(sock, 200, "application/json",
                          "{\"id\":null,\"name\":null,\"connected\":false}");
            return;
        }
        uint64_t cid = 0;
        QString name;
        int rc = bridge->currentChannelID(&cid, &name);
        if (rc != 1) {
            writeResponse(sock, 200, "application/json",
                          "{\"id\":null,\"name\":null,\"connected\":false}");
            return;
        }
        QByteArray body;
        body.append("{\"id\":")
            .append(QByteArray::number(static_cast<qulonglong>(cid)))
            .append(",\"name\":\"")
            .append(jsonEscape(name))
            .append("\",\"connected\":true}");
        writeResponse(sock, 200, "application/json", body);
        return;
    }
    if (path == "/channel/move") {
        QString p = query.queryItemValue(QStringLiteral("path"),
                                         QUrl::FullyDecoded);
        if (p.isEmpty()) {
            writeResponse(sock, 400, "application/json",
                          "{\"error\":\"missing path\"}");
            return;
        }
        if (!bridge) {
            writeResponse(sock, 200, "application/json",
                          "{\"id\":null,\"connected\":false}");
            return;
        }
        uint64_t target = bridge->resolveChannelPath(p);
        if (target == 0) {
            writeResponse(sock, 404, "application/json",
                          "{\"error\":\"channel not found\",\"connected\":true}");
            return;
        }
        int rc = bridge->moveSelfToChannelID(target);
        if (rc < 0) {
            writeResponse(sock, 500, "application/json",
                          "{\"error\":\"move failed\",\"connected\":true}");
            return;
        }
        QByteArray body;
        body.append("{\"id\":")
            .append(QByteArray::number(static_cast<qulonglong>(target)))
            .append(",\"connected\":true}");
        writeResponse(sock, 200, "application/json", body);
        return;
    }
    if (path == "/quit") {
        writeResponse(sock, 200, "text/plain", "bye\n");
        sock->flush();
        // Give Stream Deck (or curl) a tick to read the response, then
        // hard-exit. Same semantics as the Mac plugin.
        QTimer::singleShot(50, []() {
            sv_log("Quit TeamSpeak requested via HTTP");
            std::exit(0);
        });
        return;
    }

    writeResponse(sock, 404, "text/plain", "unknown route\n");
}

void HttpControlServer::writeResponse(QTcpSocket* sock, int status,
                                      const QByteArray& contentType,
                                      const QByteArray& body) {
    const char* reason = (status == 200) ? "OK"
                       : (status == 400) ? "Bad Request"
                       : (status == 404) ? "Not Found"
                       : (status == 500) ? "Internal Server Error"
                                         : "Error";
    QByteArray header;
    header.reserve(256);
    header.append("HTTP/1.1 ").append(QByteArray::number(status)).append(' ')
          .append(reason).append("\r\n")
          .append("Content-Type: ").append(contentType).append("\r\n")
          .append("Content-Length: ").append(QByteArray::number(body.size())).append("\r\n")
          .append("Cache-Control: no-store\r\n")
          .append("Connection: close\r\n")
          .append("Access-Control-Allow-Origin: *\r\n")
          .append("\r\n");
    sock->write(header);
    sock->write(body);
    sock->flush();
    sock->disconnectFromHost();
}
