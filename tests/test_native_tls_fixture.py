"""Exercise the loopback fixture with a deliberately delayed real TLS client."""
import pathlib
import socket
import ssl
import subprocess
import tempfile
import threading
import time
import unittest

from run_native_sip_loopback import wrap_tls_connection


class NativeTlsFixtureTest(unittest.TestCase):
    def test_handshake_outlives_read_poll_and_restores_poll_timeout(self):
        with tempfile.TemporaryDirectory(prefix='axe-tls-fixture-') as directory:
            root = pathlib.Path(directory)
            cert, key = root / 'cert.pem', root / 'key.pem'
            config = root / 'openssl.cnf'
            config.write_text('[req]\ndistinguished_name=dn\n[dn]\nCN=localhost\n')
            subprocess.run([
                'openssl', 'req', '-config', str(config), '-x509', '-newkey',
                'rsa:2048', '-nodes', '-keyout', str(key), '-out', str(cert),
                '-days', '1', '-subj', '/CN=localhost', '-addext',
                'subjectAltName=DNS:localhost',
            ], check=True, capture_output=True)
            server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            server_context.load_cert_chain(cert, key)
            # Keep both sides at TLS 1.2 so receiving the server flight proves
            # the server is waiting for the delayed client Finished message.
            server_context.maximum_version = ssl.TLSVersion.TLSv1_2
            client_context = ssl.create_default_context(cafile=str(cert))
            client_context.maximum_version = ssl.TLSVersion.TLSv1_2
            incoming, outgoing = ssl.MemoryBIO(), ssl.MemoryBIO()
            client = client_context.wrap_bio(incoming, outgoing, server_hostname='localhost')
            errors, timeouts = [], []
            with socket.socket() as listener:
                listener.bind(('127.0.0.1', 0))
                listener.listen(1)
                listener.settimeout(3)

                def serve():
                    try:
                        raw, _ = listener.accept()
                        with raw:
                            with wrap_tls_connection(server_context, raw) as connection:
                                timeouts.append(connection.gettimeout())
                                connection.sendall(b'accepted')
                    except Exception as error:
                        errors.append(error)

                worker = threading.Thread(target=serve)
                worker.start()
                try:
                    with socket.create_connection(listener.getsockname(), timeout=3) as transport:
                        with self.assertRaises(ssl.SSLWantReadError):
                            client.do_handshake()
                        transport.sendall(outgoing.read())  # ClientHello
                        incoming.write(transport.recv(65535))
                        # Deliberately exceed the old 0.2s handshake deadline.
                        time.sleep(0.4)
                        try:
                            while True:
                                try:
                                    client.do_handshake()
                                    break
                                except ssl.SSLWantReadError:
                                    pending = outgoing.read()
                                    if pending:
                                        transport.sendall(pending)
                                    data = transport.recv(65535)
                                    if not data:
                                        raise EOFError('fixture closed before TLS handshake completed')
                                    incoming.write(data)
                            pending = outgoing.read()
                            if pending:
                                transport.sendall(pending)
                            while True:
                                try:
                                    self.assertEqual(client.read(8), b'accepted')
                                    break
                                except ssl.SSLWantReadError:
                                    data = transport.recv(65535)
                                    if not data:
                                        raise EOFError('fixture closed before acceptance')
                                    incoming.write(data)
                        except (OSError, EOFError) as error:
                            errors.append(error)
                finally:
                    worker.join(timeout=4)
                self.assertFalse(worker.is_alive(), 'TLS fixture thread did not stop')
                self.assertEqual(errors, [], 'delayed TLS handshake must complete')
                self.assertEqual(timeouts, [0.2], 'restore short polling after handshake')


if __name__ == '__main__':
    unittest.main()
