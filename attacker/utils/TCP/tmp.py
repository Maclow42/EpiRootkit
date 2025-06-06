def receive(self, sock: socket.socket) -> str | bool:
        """
        Réassemble tous les chunks envoyés par le module C, en lisant :
         1) le header (4 octets),
         2) le payload(utilité + EOT),
         3) on ignore le padding.
        Puis on décrypte l’ensemble.
        """
        buffer = bytearray()
        received_flags = None
        total_chunks = None

        try:
            while True:
                sock.settimeout(10)
                head = self._recv_exact(sock, self._header_size)
                if head is None:
                    print("[RECEIVE ERROR] Timeout ou socket fermé avant le header")
                    return False

                total_chunks_read = head[0]
                chunk_index = head[1]
                chunk_len = (head[2] << 8) | head[3]

                # Vérifier cohérence minimale
                needed = self._header_size + chunk_len + 1  # +1 pour EOT
                if needed > self._buffer_size:
                    print(f"[RECEIVE ERROR] chunk_len {chunk_len} incohérent (need {needed} > {self._buffer_size})")
                    return False

                # --- Étape 2 : lire payload utile + EOT ---
                payload_plus_eot = self._recv_exact(sock, chunk_len + 1)
                if payload_plus_eot is None:
                    print(f"[RECEIVE ERROR] Timeout ou socket fermé pendant le payload du chunk {chunk_index}")
                    return False

                # Vérifier le marqueur EOT (dernier octet = 0x04)
                if payload_plus_eot[-1] != 0x04:
                    print(f"[RECEIVE ERROR] EOT manquant ou mal placé pour chunk {chunk_index}")
                    return False

                # --- Étape 3 : ignorer le reste du padding jusqu’à 1024 ---
                # On sait que le chunk entier fait 1024 octets, on a déjà consommé needed = 4+chunk_len+1.
                # reste_pad = 1024 - needed octets, que l’on lit mais n’utilise pas.
                reste_pad = self._buffer_size - needed
                if reste_pad > 0:
                    pad = self._recv_exact(sock, reste_pad)
                    if pad is None:
                        print(f"[RECEIVE ERROR] Timeout/fermeture en lisant le padding du chunk {chunk_index}")
                        return False
                    # ne rien faire avec pad, c’est juste du 0

                # --- Initialisation du suivi au premier chunk ---
                if total_chunks is None:
                    total_chunks = total_chunks_read
                    if total_chunks <= 0:
                        print(f"[RECEIVE ERROR] total_chunks invalide ={total_chunks}")
                        return False
                    received_flags = [False] * total_chunks

                # --- Valider chunk_index ---
                if not (0 <= chunk_index < total_chunks):
                    print(f"[RECEIVE ERROR] chunk_index {chunk_index} hors bornes (total {total_chunks})")
                    return False
                if received_flags[chunk_index]:
                    print(f"[RECEIVE ERROR] chunk dupliqué {chunk_index}")
                    return False

                # --- Extraire et accumuler le ciphertext ---
                ciphertext = payload_plus_eot[:chunk_len]
                buffer.extend(ciphertext)
                received_flags[chunk_index] = True

                # --- Si tous les chunks sont là, sortir ---
                if all(received_flags):
                    break

            # --- Déchiffrer l’intégralité du ciphertext reconstitué ---
            return self._crypto.decrypt(bytes(buffer))

        except Exception as e:
            print(f"[RECEIVE EXCEPTION] {e}")
            return False
