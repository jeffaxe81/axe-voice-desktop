"""Local SIP Digest/TLS acceptance fixture; never contacts a real PBX."""
import hashlib
import pathlib
import re
import socket
import ssl
import subprocess
import sys
import tempfile
import threading


def digest(value):
    return hashlib.md5(value.encode('utf-8')).hexdigest()


def scenario(exe, mode, context=None, ca=None):
    listener = socket.socket()
    udp = context is None
    if udp:
        listener.close()
        listener = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    listener.bind(('127.0.0.1', 0))
    if not udp:
        listener.listen(1)
    listener.settimeout(0.2)
    port = listener.getsockname()[1]
    done = threading.Event()
    accepted = []
    received = []
    errors = []

    def respond(message):
        lines = message.split('\r\n')
        headers = {}
        for line in lines[1:]:
            if ':' in line:
                name, value = line.split(':', 1)
                headers.setdefault(name.lower(), []).append(value.strip())
        received.append(lines[0].split()[0])
        if not lines[0].startswith('REGISTER '):
            raise AssertionError('Unexpected non-REGISTER SIP operation')
        if mode == 'timeout':
            return None
        auth = headers.get('authorization', [''])[0]
        valid = False
        if auth:
            fields = dict((m[0], m[1] or m[2]) for m in re.findall(r'(\w+)=(?:"([^"]*)"|([^,\s]+))', auth))
            ha1 = digest('1001:axe-loopback:local-test-secret')
            ha2 = digest('REGISTER:' + fields.get('uri', ''))
            expected = digest(':'.join((ha1, 'axe-test-nonce', fields.get('nc', ''), fields.get('cnonce', ''), 'auth', ha2)))
            valid = fields.get('username') == '1001' and fields.get('realm') == 'axe-loopback' and fields.get('nonce') == 'axe-test-nonce' and fields.get('qop') == 'auth' and fields.get('response') == expected
        status = '200 OK' if valid else ('403 Forbidden' if auth else '401 Unauthorized')
        out = ['SIP/2.0 ' + status]
        for name in ('via', 'from', 'to', 'call-id', 'cseq'):
            out.extend(name + ': ' + value for value in headers[name])
        if valid:
            out.extend('Contact: ' + re.sub(r';expires=\d+', '', value) + ';expires=2' for value in headers.get('contact', []))
            out.append('Expires: 2')
            accepted.append(True)
        elif not auth:
            out.append('WWW-Authenticate: Digest realm="axe-loopback", nonce="axe-test-nonce", algorithm=MD5, qop="auth"')
        out.extend(('Content-Length: 0', '', ''))
        return '\r\n'.join(out).encode()

    def serve():
        connection = None
        buffer = b''
        try:
            while not done.is_set():
                try:
                    if udp:
                        message, peer = listener.recvfrom(65535)
                    else:
                        if connection is None:
                            raw, _ = listener.accept()
                            raw.settimeout(0.2)
                            try:
                                connection = context.wrap_socket(raw, server_side=True)
                            except (ssl.SSLError, TimeoutError, ConnectionError):
                                raw.close()
                                continue
                        part = connection.recv(65535)
                        if not part:
                            connection.close()
                            connection = None
                            continue
                        buffer += part
                        if b'\r\n\r\n' not in buffer:
                            continue
                        message, buffer = buffer.split(b'\r\n\r\n', 1)
                    reply = respond(message.decode())
                    if reply:
                        listener.sendto(reply, peer) if udp else connection.sendall(reply)
                except socket.timeout:
                    continue
                except (ssl.SSLError, ConnectionError):
                    if connection:
                        connection.close()
                    connection = None
        except Exception as error:
            errors.append(type(error).__name__ + ': ' + str(error))
        finally:
            if connection:
                connection.close()

    worker = threading.Thread(target=serve)
    worker.start()
    args = [exe, str(port), mode]
    if ca:
        args.append(str(ca))
    try:
        result = subprocess.run(args, timeout=25, capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(f'{mode}: native engine exit {result.returncode}; REGISTERs={len(received)} authenticated={len(accepted)} fixture_errors={errors}; {result.stdout[-1000:]} {result.stderr[-1000:]}')
        if errors:
            raise AssertionError(errors)
        if mode in ('success', 'missing-audio') and len(accepted)<2:
            raise AssertionError('No authenticated REGISTER accepted')
        if mode in ('wrong-host', 'untrusted') and received:
            raise AssertionError('SIP REGISTER reached an invalid TLS peer')
        if mode not in ('success', 'missing-audio') and accepted:
            raise AssertionError('Unexpected successful authentication')
        print(f'PASS {"TLS" if ca else "UDP"} {mode}', flush=True)
    finally:
        done.set()
        worker.join(timeout=3)
        listener.close()
        if worker.is_alive():
            raise AssertionError('Loopback fixture did not stop')


if __name__ == '__main__':
    exe = sys.argv[1]
    for mode in ('success', 'wrong-password', 'timeout', 'missing-audio'):
        scenario(exe, mode)
    with tempfile.TemporaryDirectory(prefix='axe-tls-') as directory:
        root = pathlib.Path(directory)
        config = root / 'openssl.cnf'
        config.write_text('[req]\ndistinguished_name=dn\n[dn]\nCN=AXE-loopback\n')
        for name, san in (('trusted', 'IP:127.0.0.1'), ('wrong-host', 'DNS:wrong.example'), ('untrusted', 'IP:127.0.0.1')):
            subprocess.run(['openssl', 'req', '-config', str(config), '-x509', '-newkey', 'rsa:2048', '-nodes', '-keyout', str(root / (name + '.key')), '-out', str(root / (name + '.pem')), '-days', '1', '-subj', '/CN=AXE-loopback', '-addext', 'subjectAltName=' + san], check=True, capture_output=True)
        for name in ('trusted', 'wrong-host', 'untrusted'):
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.minimum_version = ssl.TLSVersion.TLSv1_2
            context.load_cert_chain(root / (name + '.pem'), root / (name + '.key'))
            scenario(exe, 'success' if name == 'trusted' else name, context, root / (('wrong-host' if name == 'wrong-host' else 'trusted') + '.pem'))
